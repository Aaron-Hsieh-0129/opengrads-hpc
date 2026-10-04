/* Added in 2026 for terminal graphics display; see COPYING. */

/* Terminal display interface for Cairo -- draws into an off-screen image and
   shows it in the terminal instead of an X window.

   The picture is rendered by gxC.c into a Cairo image surface. Whenever
   GrADS is about to wait for the user (the command prompt, a script "pull",
   a "q pos"), gxdidle hands a copy of the picture to a worker thread if it
   changed since the last time. The worker encodes it and tells the viewer,
   so the prompt comes back without waiting for the encoding. How the
   picture reaches the screen depends on GA_TERM_MODE:

     tmux    A viewer (GA_TERM_VIEWER, normally libexec/grads-termview) runs
             in a tmux pane split off beside GrADS and redraws the picture
             with the iTerm2 inline image protocol each time it changes.
     inline  The picture is printed into the terminal below the command,
             like a notebook. Needs no tmux and no viewer.
     file    The picture is only written; the viewer is started by hand
             with the command printed at start-up.
     auto    tmux when GrADS runs inside tmux and a viewer is available,
             inline otherwise. This is the default.

   Animation. A frame ends where the picture is replaced: at a "swap" in
   double-buffer mode, or when a drawn page is cleared. Frames are shown as
   they are made (except inline), and a command that swaps two or more
   frames -- a "set dbuff on" loop, or "set looping on" -- leaves behind a
   looping animated GIF, which iTerm2 plays by itself. GA_TERM_ANIM picks
   the behaviour:

     auto    as above (the default)
     gif     any command with two or more frames loops, cleared ones too
     live    frames are shown as they are made, but nothing loops
     off     only the picture at the prompt is shown

   Nothing here needs an X server, so it works over plain ssh.

   Other settings:
     GA_TERM_DIR        Directory for the pictures (default: a new
                        temporary one)
     GA_TERM_SCALE      Pixel density factor, 1 to 4 (default 2, for Retina)
     GA_TERM_PANE       Width of the tmux viewer pane, e.g. 45% (default 50%)
     GA_TERM_WIDTH      Width of an inline image, as iTerm2 understands it
                        (default 70%)
     GA_TERM_ANIM_DELAY Seconds per animation frame (default 0.2)
     GA_TERM_ANIM_MAX   Most frames kept in one animation (default 300)
     GA_TERM_ANIM_SCALE Size of animation frames relative to the page, 0.25
                        to 1 (default 1); 0.5 roughly halves the data
     GA_TERM_SYNC       1 waits for each picture to be written before going
                        on, for scripts and tests that read it at once

   The picture size comes from the -g option ("-g 1200x900"), otherwise it
   is 1000 points along the longer side of the page. Animation frames are
   kept at that size; the still picture has GA_TERM_SCALE times as many
   pixels each way.  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <cairo.h>
#include <zlib.h>

#include "gatypes.h"
#include "gx.h"
#include "gxC.h"

#define TERM_DEFAULT_SIZE 1000       /* points along the longer page side */
#define SEQ_LIMIT 1000000            /* iTerm2 and tmux drop control sequences
                                        over 1 MiB, so larger files go in parts */
#define SEQ_PART  65536              /* size of each part */
#define FRAMEQ    3                  /* animation frames waiting for the worker */

void gxdXflush (void);
void gxdidle (void);

static gaint batch=0;                       /* Batch mode? */
static gadouble xscl,yscl;                  /* Pixels per inch */
static gadouble xsize, ysize;               /* Page size in inches */
static gaint dblmode;                       /* single or double buffering */
static gaint width,height;                  /* Picture size in points */
static gadouble scale=2.0;                  /* Device pixels per point */
static cairo_surface_t *surface=NULL,*surface2=NULL; /* front and back pictures */
static char *ugeom = NULL;                  /* -g geometry string */

/* State of the visible picture; main thread only */
static gaint dirty=0;                       /* changed since it was last sent */
static gaint drawn=0;                       /* something drawn since the last clear */
static gaint backdrawn=0;                   /* drawn on the back buffer since the last swap */
static gaint ngif=0;                        /* of those, frames queued for a GIF */
static gaint gifcut=0;                      /* frames left out past GA_TERM_ANIM_MAX */

/* Settings */
static char tdir[512];                      /* where the pictures go */
static gaint ownsdir=0;                     /* we created tdir, remove it at exit */
static char seqpath[600], seqtmp[600], fifopath[600];
static gaint mode=0;                        /* 1=tmux 2=inline 3=file */
static gaint anim=1;                        /* 0=off 1=auto 2=gif 3=live */
static gaint animdelay=20;                  /* hundredths of a second per frame */
static gaint animmax=300;                   /* most frames in one GIF */
static gadouble animscale=1.0;              /* GIF frame size relative to the page */
static gaint syncwrite=0;                   /* wait for each picture */
static char pane[64];                       /* tmux pane id of the viewer */

/* ---- work handed to the worker thread ---- */

#define JOB_PNG     1                       /* encode and show a still picture */
#define JOB_FRAME   2                       /* add a frame to the animation */
#define JOB_GIFEND  3                       /* finish the animation and show it */
#define JOB_GIFDROP 4                       /* forget the animation */

struct tjob {
  gaint kind;
  unsigned char *px;                        /* copy of the ARGB32 picture */
  gaint w,h,stride;                         /* its size in device pixels */
  gaint lw,lh;                              /* size in points, for GIF frames */
  gaint first;                              /* first frame of a new animation */
  struct tjob *next;
};

static pthread_t worker;
static gaint workeron=0;
static pthread_mutex_t tlock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t twake = PTHREAD_COND_INITIALIZER;  /* work for the worker */
static pthread_cond_t tdone = PTHREAD_COND_INITIALIZER;  /* the worker made progress */
static struct tjob *live=NULL;              /* latest still picture; newer ones replace it */
static struct tjob *qhead=NULL,*qtail=NULL; /* animation work, in order */
static gaint qframes=0;                     /* frames in the queue */
static gaint busy=0;                        /* the worker is working */
static gaint stopping=0;                    /* finish the queue and exit */
static gaint seq=0;                         /* pictures shown; worker only */
static char lastfile[64];                   /* file name of the latest picture */

/* Make a surface of the current picture size */

static cairo_surface_t *newsurface (void) {
cairo_surface_t *s;
  s = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
          (gaint)(width*scale+0.5), (gaint)(height*scale+0.5));
  cairo_surface_set_device_scale (s, scale, scale);
  return (s);
}

/* Quote a string for /bin/sh */

static void shquote (char *out, size_t len, const char *in) {
size_t n=0;
  if (len<3) { *out='\0'; return; }
  out[n++] = '\'';
  while (*in && n+5<len) {
    if (*in=='\'') { memcpy(out+n,"'\\''",4); n+=4; }
    else out[n++] = *in;
    in++;
  }
  out[n++] = '\'';
  out[n] = '\0';
}

/* Run a command without a shell. Its first line of output goes into out.
   Returns the exit status, or -1 when it could not be run. */

static gaint runcmd (char *const argv[], char *out, size_t len) {
gaint fd[2],status,devnull;
pid_t pid;
ssize_t n;
size_t got=0;
char buf[256],*nl;

  if (out && len) *out = '\0';
  if (pipe(fd)) return (-1);
  pid = fork();
  if (pid<0) { close(fd[0]); close(fd[1]); return (-1); }
  if (pid==0) {
    devnull = open("/dev/null",O_RDWR);
    if (devnull>=0) { dup2(devnull,0); dup2(devnull,2); }
    dup2(fd[1],1);
    close(fd[0]); close(fd[1]);
    execvp(argv[0],argv);
    _exit(127);
  }
  close(fd[1]);
  while ((n=read(fd[0],buf,sizeof(buf)))>0) {
    if (out && got+1<len) {
      if ((size_t)n > len-1-got) n = len-1-got;
      memcpy(out+got,buf,n);
      got += n;
      out[got] = '\0';
    }
  }
  if (out && len && (nl=strchr(out,'\n'))!=NULL) *nl = '\0';
  close(fd[0]);
  if (waitpid(pid,&status,0)<0) return (-1);
  return (WIFEXITED(status) ? WEXITSTATUS(status) : -1);
}

/* Is the viewer script there to run? */

static char *viewer (void) {
char *v;
  v = getenv("GA_TERM_VIEWER");
  if (v==NULL || *v=='\0') return (NULL);
  if (access(v,X_OK)) return (NULL);
  return (v);
}

/* Split a tmux pane off for the viewer */

static gaint tmuxsplit (void) {
char cmd[2048],qv[700],qd[700],size[32],pct[32];
char *v,*p;
char *argv[16];
gaint i,rc;

  v = viewer();
  if (v==NULL) return (1);
  shquote(qv,sizeof(qv),v);
  shquote(qd,sizeof(qd),tdir);
  snprintf(cmd,sizeof(cmd),"exec %s %s %d",qv,qd,(gaint)getpid());

  p = getenv("GA_TERM_PANE");
  if (p==NULL || *p=='\0') p = "50%";
  snprintf(size,sizeof(size),"%s",p);
  if (strchr(size,'%')==NULL) strncat(size,"%",sizeof(size)-strlen(size)-1);

  i = 0;
  argv[i++] = "tmux"; argv[i++] = "split-window"; argv[i++] = "-h";
  argv[i++] = "-d"; argv[i++] = "-P"; argv[i++] = "-F"; argv[i++] = "#{pane_id}";
  argv[i++] = "-l"; argv[i++] = size; argv[i++] = cmd; argv[i] = NULL;
  rc = runcmd(argv,pane,sizeof(pane));
  if (rc!=0 || pane[0]!='%') {
    /* tmux before 3.1 only understands a percentage given with -p */
    snprintf(pct,sizeof(pct),"%d",atoi(size));
    argv[7] = "-p"; argv[8] = pct;
    rc = runcmd(argv,pane,sizeof(pane));
  }
  if (rc!=0 || pane[0]!='%') {
    pane[0] = '\0';
    return (1);
  }

  /* the viewer talks to iTerm2 through tmux; tmux 3.3 and later drop such
     sequences unless the pane allows them. Older tmux has no such option. */
  i = 0;
  argv[i++] = "tmux"; argv[i++] = "set-option"; argv[i++] = "-p";
  argv[i++] = "-t"; argv[i++] = pane; argv[i++] = "allow-passthrough";
  argv[i++] = "on"; argv[i] = NULL;
  runcmd(argv,NULL,0);
  return (0);
}

/* ---- PNG ---- */

static void be32 (unsigned char *p, unsigned long v) {
  p[0] = (v>>24)&255; p[1] = (v>>16)&255; p[2] = (v>>8)&255; p[3] = v&255;
}

static gaint pngchunk (FILE *f, const char *type, const unsigned char *data, size_t len) {
unsigned char hdr[8],crc[4];
uLong c;
  be32(hdr,(unsigned long)len);
  memcpy(hdr+4,type,4);
  c = crc32(0L,(const Bytef *)type,4);
  if (len) c = crc32(c,data,(uInt)len);
  be32(crc,c);
  if (fwrite(hdr,1,8,f)!=8) return (1);
  if (len && fwrite(data,1,len,f)!=len) return (1);
  return (fwrite(crc,1,4,f)!=4);
}

/* Write an ARGB32 picture as an RGB PNG. Rows are not filtered: for plots,
   which are mostly flat colour, that is both faster and smaller than
   letting the encoder try every filter, as cairo_surface_write_to_png does. */

static gaint pngwrite (const char *fn, const unsigned char *px, gaint w, gaint h, gaint stride) {
static const unsigned char sig[8] = {137,80,78,71,13,10,26,10};
unsigned char ihdr[13],*row,*out;
const unsigned int *p;
z_stream zs;
FILE *f;
gaint x,y,zrc,err=0;
const size_t outsz = 65536;

  f = fopen(fn,"wb");
  if (f==NULL) return (1);
  row = (unsigned char *)malloc(1+3*(size_t)w);
  out = (unsigned char *)malloc(outsz);
  memset(&zs,0,sizeof(zs));
  if (row==NULL || out==NULL || deflateInit(&zs,6)!=Z_OK) {
    free(row); free(out); fclose(f);
    return (1);
  }
  be32(ihdr,w); be32(ihdr+4,h);
  ihdr[8] = 8; ihdr[9] = 2; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
  if (fwrite(sig,1,8,f)!=8 || pngchunk(f,"IHDR",ihdr,13)) err = 1;

  zs.next_out = out;
  zs.avail_out = outsz;
  for (y=0; y<=h && !err; y++) {
    if (y<h) {
      p = (const unsigned int *)(px + (size_t)y*stride);
      row[0] = 0;
      for (x=0; x<w; x++) {
        row[1+3*x] = (p[x]>>16)&255;
        row[2+3*x] = (p[x]>>8)&255;
        row[3+3*x] = p[x]&255;
      }
      zs.next_in = row;
      zs.avail_in = 1+3*w;
    }
    do {
      zrc = deflate(&zs, y<h ? Z_NO_FLUSH : Z_FINISH);
      if (zrc==Z_STREAM_ERROR) { err = 1; break; }
      if (zs.avail_out==0 || (y==h && zrc==Z_STREAM_END)) {
        if (pngchunk(f,"IDAT",out,outsz-zs.avail_out)) { err = 1; break; }
        zs.next_out = out;
        zs.avail_out = outsz;
      }
    } while (y<h ? zs.avail_in>0 : zrc!=Z_STREAM_END);
  }
  deflateEnd(&zs);
  if (!err && pngchunk(f,"IEND",NULL,0)) err = 1;
  if (fclose(f)) err = 1;
  free(row);
  free(out);
  return (err);
}

/* ---- animated GIF; worker thread only ---- */

static struct {
  unsigned char *buf;                       /* the GIF so far */
  size_t len,cap;
  unsigned char *prev;                      /* previous frame, RGB */
  gaint w,h;                                /* frame size */
  gaint frames;
  size_t delayat;                           /* where the last frame's delay is */
  gaint failed;
} gif;

static void gput (const void *p, size_t n) {
unsigned char *nb;
size_t nc;
  if (gif.failed) return;
  if (gif.len+n > gif.cap) {
    nc = gif.cap ? gif.cap : 65536;
    while (nc < gif.len+n) nc *= 2;
    nb = (unsigned char *)realloc(gif.buf,nc);
    if (nb==NULL) { gif.failed = 1; return; }
    gif.buf = nb;
    gif.cap = nc;
  }
  memcpy(gif.buf+gif.len,p,n);
  gif.len += n;
}

static void gbyte (gaint b) {
unsigned char c = (unsigned char)b;
  gput(&c,1);
}

static void gword (gaint v) {
  gbyte(v&255);
  gbyte((v>>8)&255);
}

static void gifreset (void) {
  free(gif.buf);
  free(gif.prev);
  memset(&gif,0,sizeof(gif));
}

/* Colour reduction. Colours are counted in 6-bit-per-channel bins; the
   palette takes the most common bins, skipping ones too close to a colour
   already chosen so that small features in a colour of their own (a marker,
   a line) are not swallowed by the shades of anti-aliased edges. */

#define QBINS (1<<18)
static unsigned int *qcount=NULL, *qsum=NULL;
static short *qmap=NULL;
static gaint *qused=NULL;

static gaint qbycount (const void *a, const void *b) {
unsigned int ca = qcount[*(const gaint *)a], cb = qcount[*(const gaint *)b];
  return (ca<cb) - (ca>cb);
}

static gaint quantize (const unsigned char *rgb, gaint w, gaint x0, gaint y0, gaint bw, gaint bh,
                       unsigned char *pal, unsigned char *idx) {
gaint x,y,i,j,k,nused=0,npal=0,best,pass;
long d,bd,dr,dg,db;
const unsigned char *p;
unsigned int key,n;

  if (qcount==NULL) {
    qcount = (unsigned int *)calloc(QBINS,sizeof(unsigned int));
    qsum = (unsigned int *)calloc(3*(size_t)QBINS,sizeof(unsigned int));
    qmap = (short *)malloc(QBINS*sizeof(short));
    qused = (gaint *)malloc(QBINS*sizeof(gaint));
    if (!qcount || !qsum || !qmap || !qused) return (1);
  }
  for (y=y0; y<y0+bh; y++) {
    p = rgb + 3*((size_t)y*w + x0);
    for (x=0; x<bw; x++, p+=3) {
      key = ((unsigned int)(p[0]>>2)<<12) | ((unsigned int)(p[1]>>2)<<6) | (p[2]>>2);
      if (qcount[key]++==0) qused[nused++] = key;
      qsum[3*key] += p[0]; qsum[3*key+1] += p[1]; qsum[3*key+2] += p[2];
    }
  }
  memset(pal,0,768);
  if (nused<=256) {
    for (i=0; i<nused; i++) {
      key = qused[i]; n = qcount[key];
      pal[3*i] = qsum[3*key]/n; pal[3*i+1] = qsum[3*key+1]/n; pal[3*i+2] = qsum[3*key+2]/n;
      qmap[key] = i;
    }
  } else {
    qsort(qused,nused,sizeof(gaint),qbycount);
    for (pass=0; pass<2 && npal<256; pass++) {
      for (i=0; i<nused && npal<256; i++) {
        key = qused[i];
        if (qcount[key]==0) continue;          /* taken in the first pass */
        n = qcount[key];
        dr = qsum[3*key]/n; dg = qsum[3*key+1]/n; db = qsum[3*key+2]/n;
        if (pass==0) {
          for (j=0; j<npal; j++) {
            d = (dr-pal[3*j])*(dr-pal[3*j]) + (dg-pal[3*j+1])*(dg-pal[3*j+1])
              + (db-pal[3*j+2])*(db-pal[3*j+2]);
            if (d<300) break;
          }
          if (j<npal) continue;
        }
        pal[3*npal] = dr; pal[3*npal+1] = dg; pal[3*npal+2] = db;
        npal++;
        qsum[3*key] = dr; qsum[3*key+1] = dg; qsum[3*key+2] = db;  /* now the average */
        qcount[key] = 0;                       /* mark as taken */
      }
    }
    /* every bin maps to its nearest palette colour */
    for (i=0; i<nused; i++) {
      key = qused[i];
      n = qcount[key];
      if (n) { dr = qsum[3*key]/n; dg = qsum[3*key+1]/n; db = qsum[3*key+2]/n; }
      else   { dr = qsum[3*key];   dg = qsum[3*key+1];   db = qsum[3*key+2]; }
      best = 0; bd = -1;
      for (k=0; k<npal; k++) {
        d = (dr-pal[3*k])*(dr-pal[3*k]) + (dg-pal[3*k+1])*(dg-pal[3*k+1])
          + (db-pal[3*k+2])*(db-pal[3*k+2]);
        if (bd<0 || d<bd) { bd = d; best = k; if (d==0) break; }
      }
      qmap[key] = best;
    }
  }
  for (y=y0, i=0; y<y0+bh; y++) {
    p = rgb + 3*((size_t)y*w + x0);
    for (x=0; x<bw; x++, p+=3) {
      key = ((unsigned int)(p[0]>>2)<<12) | ((unsigned int)(p[1]>>2)<<6) | (p[2]>>2);
      idx[i++] = (unsigned char)qmap[key];
    }
  }
  for (i=0; i<nused; i++) {
    key = qused[i];
    qcount[key] = 0;
    qsum[3*key] = qsum[3*key+1] = qsum[3*key+2] = 0;
  }
  return (0);
}

/* GIF's LZW, 8-bit codes, written as data sub-blocks */

static struct {
  unsigned long acc;
  gaint nbits,cs;
  unsigned char blk[256];
  gaint blen;
} lz;

static void lzflush (void) {
  if (lz.blen) {
    lz.blk[0] = lz.blen;
    gput(lz.blk,lz.blen+1);
    lz.blen = 0;
  }
}

static void lzput (gaint code) {
  lz.acc |= (unsigned long)code << lz.nbits;
  lz.nbits += lz.cs;
  while (lz.nbits>=8) {
    lz.blk[1+lz.blen++] = lz.acc&255;
    lz.acc >>= 8;
    lz.nbits -= 8;
    if (lz.blen==255) lzflush();
  }
}

#define LZHASH 8192
static gaint lzkey[LZHASH];                 /* key+1, 0 when empty */
static short lzval[LZHASH];

static void lzw (const unsigned char *idx, size_t n) {
size_t i;
gaint next,prefix,key,h;

  memset(&lz,0,sizeof(lz));
  memset(lzkey,0,sizeof(lzkey));
  gbyte(8);                                 /* minimum code size */
  lz.cs = 9;
  next = 258;
  lzput(256);                               /* clear */
  if (n) {
    prefix = idx[0];
    for (i=1; i<n; i++) {
      key = (prefix<<8) | idx[i];
      h = (gaint)(((unsigned int)key*2654435761u)>>19) & (LZHASH-1);
      while (lzkey[h] && lzkey[h]!=key+1) h = (h+1)&(LZHASH-1);
      if (lzkey[h]) { prefix = lzval[h]; continue; }
      lzput(prefix);
      if (next<4096) {
        lzkey[h] = key+1;
        lzval[h] = next++;
        if (next > (1<<lz.cs) && lz.cs<12) lz.cs++;
      } else {
        lzput(256);                         /* table full: start again */
        memset(lzkey,0,sizeof(lzkey));
        lz.cs = 9;
        next = 258;
      }
      prefix = idx[i];
    }
    lzput(prefix);
  }
  lzput(257);                               /* end of information */
  if (lz.nbits) {                           /* the last, partial byte */
    lz.blk[1+lz.blen++] = lz.acc&255;
    lz.acc = 0;
    lz.nbits = 0;
    if (lz.blen==255) lzflush();
  }
  lzflush();
  gbyte(0);                                 /* block terminator */
}

/* Add a frame. Only the rectangle that changed since the previous frame is
   stored; an unchanged frame just lengthens the previous one. */

static void gifframe (const unsigned char *rgb, gaint w, gaint h) {
gaint x,y,x0,y0,x1,y1,bw,bh,d;
const unsigned char *a,*b;
unsigned char pal[768],*idx;

  if (gif.failed) return;
  if (gif.frames==0) {
    gif.w = w; gif.h = h;
    gput("GIF89a",6);
    gword(w); gword(h);
    gbyte(0x70); gbyte(0); gbyte(0);        /* no global colour table */
    gput("\041\377\013NETSCAPE2.0\003\001\000\000\000",19);   /* loop forever */
    gif.prev = (unsigned char *)malloc(3*(size_t)w*h);
    if (gif.prev==NULL) { gif.failed = 1; return; }
    x0 = 0; y0 = 0; x1 = w-1; y1 = h-1;
  } else {
    if (w!=gif.w || h!=gif.h) return;       /* the page was resized mid-way */
    x0 = w; y0 = h; x1 = -1; y1 = -1;
    for (y=0; y<h; y++) {
      a = rgb + 3*(size_t)y*w;
      b = gif.prev + 3*(size_t)y*w;
      if (!memcmp(a,b,3*(size_t)w)) continue;
      if (y<y0) y0 = y;
      y1 = y;
      for (x=0; x<w; x++) {
        if (a[3*x]!=b[3*x] || a[3*x+1]!=b[3*x+1] || a[3*x+2]!=b[3*x+2]) {
          if (x<x0) x0 = x;
          if (x>x1) x1 = x;
        }
      }
    }
    if (x1<0) {                             /* nothing changed */
      d = gif.buf[gif.delayat] | (gif.buf[gif.delayat+1]<<8);
      d += animdelay;
      if (d>65535) d = 65535;
      gif.buf[gif.delayat] = d&255;
      gif.buf[gif.delayat+1] = (d>>8)&255;
      return;
    }
  }
  bw = x1-x0+1;
  bh = y1-y0+1;
  idx = (unsigned char *)malloc((size_t)bw*bh);
  if (idx==NULL || quantize(rgb,w,x0,y0,bw,bh,pal,idx)) {
    free(idx);
    gif.failed = 1;
    return;
  }
  gput("\041\371\004\004",4);               /* graphic control: keep previous */
  gif.delayat = gif.len;
  gword(animdelay);
  gbyte(0); gbyte(0);
  gbyte(0x2c);                              /* image descriptor */
  gword(x0); gword(y0); gword(bw); gword(bh);
  gbyte(0x87);                              /* local colour table of 256 */
  gput(pal,768);
  lzw(idx,(size_t)bw*bh);
  free(idx);
  memcpy(gif.prev,rgb,3*(size_t)w*h);
  gif.frames++;
}

/* Shrink an ARGB32 picture to w x h RGB by averaging */

static unsigned char *downsample (const struct tjob *j, gaint w, gaint h) {
unsigned char *out,*o;
const unsigned int *p;
gaint x,y,sx,sy,sx0,sx1,sy0,sy1;
unsigned long r,g,b,n;

  out = (unsigned char *)malloc(3*(size_t)w*h);
  if (out==NULL) return (NULL);
  o = out;
  for (y=0; y<h; y++) {
    sy0 = (gaint)((long)y*j->h/h);
    sy1 = (gaint)((long)(y+1)*j->h/h);
    if (sy1<=sy0) sy1 = sy0+1;
    for (x=0; x<w; x++) {
      sx0 = (gaint)((long)x*j->w/w);
      sx1 = (gaint)((long)(x+1)*j->w/w);
      if (sx1<=sx0) sx1 = sx0+1;
      r = g = b = n = 0;
      for (sy=sy0; sy<sy1; sy++) {
        p = (const unsigned int *)(j->px + (size_t)sy*j->stride);
        for (sx=sx0; sx<sx1; sx++) {
          r += (p[sx]>>16)&255; g += (p[sx]>>8)&255; b += p[sx]&255; n++;
        }
      }
      *o++ = r/n; *o++ = g/n; *o++ = b/n;
    }
  }
  return (out);
}

/* ---- worker thread ---- */

/* Record the new picture and wake the viewer */

static void shown (const char *name) {
FILE *f;
gaint fd,n;

  pthread_mutex_lock(&tlock);
  n = ++seq;
  snprintf(lastfile,sizeof(lastfile),"%s",name);
  pthread_mutex_unlock(&tlock);
  f = fopen(seqtmp,"w");
  if (f) {
    fprintf(f,"%d %s\n",n,name);
    fclose(f);
    rename(seqtmp,seqpath);
  }
  /* the viewer keeps the FIFO open; without one, open fails at once */
  fd = open(fifopath,O_WRONLY|O_NONBLOCK);
  if (fd>=0) {
    if (write(fd,"\n",1)<0) { /* full or gone: the viewer catches up anyway */ }
    close(fd);
  }
}

static void putfile (const char *name, const unsigned char *data, size_t len) {
char fn[700],tmp[700];
FILE *f;
gaint ok;
  snprintf(fn,sizeof(fn),"%s/%s",tdir,name);
  snprintf(tmp,sizeof(tmp),"%s/.%s.tmp",tdir,name);
  f = fopen(tmp,"wb");
  if (f==NULL) return;
  ok = fwrite(data,1,len,f)==len;
  if (fclose(f)) ok = 0;
  if (ok && !rename(tmp,fn)) shown(name);
  else unlink(tmp);
}

static void dojob (struct tjob *j) {
char fn[700],tmp[700];
unsigned char *rgb;

  if (j->kind==JOB_PNG) {
    snprintf(fn,sizeof(fn),"%s/plot.png",tdir);
    snprintf(tmp,sizeof(tmp),"%s/.plot.png.tmp",tdir);
    if (!pngwrite(tmp,j->px,j->w,j->h,j->stride) && !rename(tmp,fn)) shown("plot.png");
    else unlink(tmp);
  }
  else if (j->kind==JOB_FRAME) {
    if (j->first) gifreset();
    rgb = downsample(j,j->lw,j->lh);
    if (rgb) gifframe(rgb,j->lw,j->lh);
    else gif.failed = 1;
    free(rgb);
  }
  else if (j->kind==JOB_GIFEND) {
    if (gif.frames>0 && !gif.failed) {
      gbyte(0x3b);
      if (!gif.failed) putfile("plot.gif",gif.buf,gif.len);
    }
    gifreset();
  }
  else if (j->kind==JOB_GIFDROP) gifreset();
}

static void freejob (struct tjob *j) {
  if (j) {
    free(j->px);
    free(j);
  }
}

static void *work (void *arg) {
struct tjob *j;

  pthread_mutex_lock(&tlock);
  while (1) {
    while (!qhead && !live && !stopping) pthread_cond_wait(&twake,&tlock);
    if (qhead) {                            /* animation work goes first */
      j = qhead;
      qhead = j->next;
      if (!qhead) qtail = NULL;
      if (j->kind==JOB_FRAME) qframes--;
    } else if (live) {
      j = live;
      live = NULL;
    } else break;                           /* stopping, nothing left */
    busy = 1;
    pthread_cond_broadcast(&tdone);         /* room in the queue */
    pthread_mutex_unlock(&tlock);
    dojob(j);
    freejob(j);
    pthread_mutex_lock(&tlock);
    busy = 0;
    pthread_cond_broadcast(&tdone);
  }
  pthread_mutex_unlock(&tlock);
  gifreset();
  free(qcount); free(qsum); free(qmap); free(qused);
  qcount = NULL; qsum = NULL; qmap = NULL; qused = NULL;
  return (NULL);
}

static void startworker (void) {
sigset_t all,old;
  /* the worker takes no signals: Ctrl-C belongs to GrADS, and a viewer that
     went away must not kill us with SIGPIPE */
  sigfillset(&all);
  pthread_sigmask(SIG_SETMASK,&all,&old);
  workeron = !pthread_create(&worker,NULL,work,NULL);
  pthread_sigmask(SIG_SETMASK,&old,NULL);
  if (!workeron) printf("Terminal display: unable to start the picture writer thread\n");
}

static void stopworker (void) {
  if (!workeron) return;
  pthread_mutex_lock(&tlock);
  stopping = 1;
  pthread_cond_signal(&twake);
  pthread_mutex_unlock(&tlock);
  pthread_join(worker,NULL);
  workeron = 0;
}

/* Wait until everything handed to the worker is written */

static void waitworker (void) {
  if (!workeron) return;
  pthread_mutex_lock(&tlock);
  while (qhead || live || busy) pthread_cond_wait(&tdone,&tlock);
  pthread_mutex_unlock(&tlock);
}

/* Copy the visible picture for the worker */

static struct tjob *snapshot (gaint kind) {
struct tjob *j;
size_t n;

  j = (struct tjob *)calloc(1,sizeof(struct tjob));
  if (j==NULL) return (NULL);
  j->kind = kind;
  j->lw = (gaint)(width*animscale+0.5);
  j->lh = (gaint)(height*animscale+0.5);
  if (j->lw<16) j->lw = 16;
  if (j->lh<16) j->lh = 16;
  if (kind==JOB_PNG || kind==JOB_FRAME) {
    gxCflush(1);
    cairo_surface_flush(surface);
    j->w = cairo_image_surface_get_width(surface);
    j->h = cairo_image_surface_get_height(surface);
    j->stride = cairo_image_surface_get_stride(surface);
    n = (size_t)j->stride*j->h;
    j->px = (unsigned char *)malloc(n);
    if (j->px==NULL) { free(j); return (NULL); }
    memcpy(j->px,cairo_image_surface_get_data(surface),n);
  }
  return (j);
}

/* Hand work to the worker. A still picture replaces one not yet started;
   animation work is queued in order, waiting for room if need be. Without
   a worker the work is done here and now. */

static void post (struct tjob *j) {
  if (j==NULL) return;
  if (!workeron) {
    dojob(j);
    freejob(j);
    return;
  }
  pthread_mutex_lock(&tlock);
  if (j->kind==JOB_PNG) {
    freejob(live);
    live = j;
  } else {
    if (j->kind==JOB_GIFEND) {              /* the animation is the last word */
      freejob(live);
      live = NULL;
    }
    while (j->kind==JOB_FRAME && qframes>=FRAMEQ) pthread_cond_wait(&tdone,&tlock);
    j->next = NULL;
    if (qtail) qtail->next = j;
    else qhead = j;
    qtail = j;
    if (j->kind==JOB_FRAME) qframes++;
  }
  pthread_cond_signal(&twake);
  pthread_mutex_unlock(&tlock);
  if (syncwrite) waitworker();
}

/* ---- frames and the idle point; main thread ---- */

/* The visible picture is about to be replaced, or has just been by a swap */

static void frameend (gaint swapped) {
struct tjob *j;
gaint forgif;

  if (batch || surface==NULL || !dirty || !drawn) return;
  forgif = (anim==2) || (anim==1 && swapped);
  if (forgif) {
    if (ngif<animmax) {
      j = snapshot(JOB_FRAME);
      if (j) {
        j->first = (ngif==0);
        post(j);
        ngif++;
      }
    } else gifcut++;
  }
  if (anim!=0 && mode!=2) {                /* show it now */
    post(snapshot(JOB_PNG));
    dirty = 0;
  }
}

/* Print a picture into the terminal with the iTerm2 inline image protocol */

static void b64out (FILE *f, const unsigned char *buf, size_t len) {
static const char tab[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
size_t i;
unsigned long v;
  for (i=0; i+2<len; i+=3) {
    v = ((unsigned long)buf[i]<<16) | ((unsigned long)buf[i+1]<<8) | buf[i+2];
    fputc(tab[(v>>18)&63],f); fputc(tab[(v>>12)&63],f);
    fputc(tab[(v>>6)&63],f);  fputc(tab[v&63],f);
  }
  if (i<len) {
    v = (unsigned long)buf[i]<<16;
    if (i+1<len) v |= (unsigned long)buf[i+1]<<8;
    fputc(tab[(v>>18)&63],f); fputc(tab[(v>>12)&63],f);
    fputc(i+1<len ? tab[(v>>6)&63] : '=',f);
    fputc('=',f);
  }
}

static void oscopen (FILE *f, gaint tmux) {
  fputs(tmux ? "\033Ptmux;\033\033]" : "\033]",f);
}

static void oscclose (FILE *f, gaint tmux) {
  fputs(tmux ? "\a\033\\" : "\a",f);
}

static void inlineshow (const char *name) {
char fn[700];
FILE *in,*tty;
unsigned char *buf;
long len;
size_t off,n;
char *w,*t,args[200];
gaint tmux;

  snprintf(fn,sizeof(fn),"%s/%s",tdir,name);
  in = fopen(fn,"rb");
  if (in==NULL) return;
  fseek(in,0,SEEK_END);
  len = ftell(in);
  rewind(in);
  buf = (unsigned char *)malloc(len>0 ? len : 1);
  if (buf==NULL || fread(buf,1,len,in)!=(size_t)len) {
    free(buf);
    fclose(in);
    return;
  }
  fclose(in);

  tty = fopen("/dev/tty","w");
  if (tty==NULL) tty = stdout;
  w = getenv("GA_TERM_WIDTH");
  if (w==NULL || *w=='\0') w = "70%";
  t = getenv("TMUX");
  tmux = (t && *t);
  snprintf(args,sizeof(args),"inline=1;size=%ld;width=%s;preserveAspectRatio=1",len,w);
  fflush(stdout);
  if (((size_t)len+2)/3*4 + 200 < SEQ_LIMIT) {
    oscopen(tty,tmux);
    fprintf(tty,"1337;File=%s:",args);
    b64out(tty,buf,(size_t)len);
    oscclose(tty,tmux);
  } else {
    /* iTerm2 3.5 and later take a large file in parts */
    oscopen(tty,tmux);
    fprintf(tty,"1337;MultipartFile=%s",args);
    oscclose(tty,tmux);
    for (off=0; off<(size_t)len; off+=n) {
      n = (size_t)len-off;
      if (n > SEQ_PART/4*3) n = SEQ_PART/4*3;
      oscopen(tty,tmux);
      fputs("1337;FilePart=",tty);
      b64out(tty,buf+off,n);
      oscclose(tty,tmux);
    }
    oscopen(tty,tmux);
    fputs("1337;FileEnd",tty);
    oscclose(tty,tmux);
  }
  fputc('\n',tty);
  fflush(tty);
  if (tty!=stdout) fclose(tty);
  free(buf);
}

/* GrADS is about to wait for the user: show the picture if it changed */

static gaint shownseq (char *name, size_t len) {
gaint n;
  pthread_mutex_lock(&tlock);
  n = seq;
  if (name) snprintf(name,len,"%s",lastfile);
  pthread_mutex_unlock(&tlock);
  return (n);
}

void gxdidle (void) {
gaint before,frames;
char name[64];

  if (batch || surface==NULL) return;
  before = shownseq(NULL,0);
  frames = ngif;

  /* in "gif" mode the picture left at the end is the last frame */
  if (ngif>0 && anim==2) frameend(0);

  if (ngif>=2) {
    post(snapshot(JOB_GIFEND));
    dirty = 0;
  } else {
    if (ngif==1) post(snapshot(JOB_GIFDROP));
    if (dirty && (drawn || mode!=2)) post(snapshot(JOB_PNG));
    dirty = 0;
  }
  if (gifcut) {
    printf("Terminal display: the animation keeps its first %d frames; %d more were left out\n",
           animmax,gifcut);
    printf("Terminal display: raise GA_TERM_ANIM_MAX to keep more\n");
  }
  ngif = 0;
  gifcut = 0;

  if (mode==2) {
    waitworker();
    if (shownseq(name,sizeof(name))!=before && (drawn || frames>=2)) inlineshow(name);
  }
}

/* Pick the output directory and the display mode */

static void termsetup (void) {
char *d,*m,*t,*v,*a;
gaint rc;
gadouble f;

  d = getenv("GA_TERM_DIR");
  ownsdir = 0;
  if (d && *d) {
    snprintf(tdir,sizeof(tdir),"%s",d);
    if (mkdir(tdir,0700) && errno!=EEXIST) {
      printf("Terminal display: unable to create %s\n",tdir);
    }
  } else {
    t = getenv("TMPDIR");
    if (t==NULL || *t=='\0') t = "/tmp";
    snprintf(tdir,sizeof(tdir),"%s/grads-term-XXXXXX",t);
    if (mkdtemp(tdir)==NULL) {
      snprintf(tdir,sizeof(tdir),"/tmp/grads-term-XXXXXX");
      if (mkdtemp(tdir)==NULL) {
        printf("Terminal display: unable to create a temporary directory\n");
        exit(-1);
      }
    }
    ownsdir = 1;
  }
  snprintf(seqpath,sizeof(seqpath),"%s/seq",tdir);
  snprintf(seqtmp,sizeof(seqtmp),"%s/.seq.tmp",tdir);
  snprintf(fifopath,sizeof(fifopath),"%s/notify",tdir);
  if (mkfifo(fifopath,0600) && errno!=EEXIST) fifopath[0] = '\0';

  a = getenv("GA_TERM_ANIM");
  if (a==NULL || *a=='\0' || !strcmp(a,"auto")) anim = 1;
  else if (!strcmp(a,"gif")) anim = 2;
  else if (!strcmp(a,"live")) anim = 3;
  else if (!strcmp(a,"off")) anim = 0;
  else printf("Terminal display: unknown GA_TERM_ANIM \"%s\"; using auto\n",a);
  a = getenv("GA_TERM_ANIM_DELAY");
  if (a && *a) {
    f = atof(a);
    if (f>=0.02 && f<=600.0) animdelay = (gaint)(f*100.0+0.5);
    else printf("Terminal display: GA_TERM_ANIM_DELAY must be 0.02 to 600 seconds\n");
  }
  a = getenv("GA_TERM_ANIM_MAX");
  if (a && *a) {
    if (atoi(a)>=2) animmax = atoi(a);
    else printf("Terminal display: GA_TERM_ANIM_MAX must be at least 2\n");
  }
  a = getenv("GA_TERM_ANIM_SCALE");
  if (a && *a) {
    f = atof(a);
    if (f>=0.25 && f<=1.0) animscale = f;
    else printf("Terminal display: GA_TERM_ANIM_SCALE must be 0.25 to 1\n");
  }
  a = getenv("GA_TERM_SYNC");
  syncwrite = (a && !strcmp(a,"1"));

  m = getenv("GA_TERM_MODE");
  if (m==NULL || *m=='\0') m = "auto";
  t = getenv("TMUX");
  v = viewer();
  pane[0] = '\0';
  if (!strcmp(m,"file")) mode = 3;
  else if (!strcmp(m,"inline")) mode = 2;
  else if (!strcmp(m,"tmux") || (!strcmp(m,"auto") && t && *t && v)) {
    if (t==NULL || *t=='\0') {
      printf("Terminal display: GA_TERM_MODE=tmux, but GrADS is not running inside tmux.\n");
      printf("Terminal display: showing pictures inline instead.\n");
      mode = 2;
    } else {
      rc = tmuxsplit();
      if (rc) {
        if (v==NULL) printf("Terminal display: viewer not found; set GA_TERM_VIEWER.\n");
        else printf("Terminal display: unable to open a tmux pane for the viewer.\n");
        printf("Terminal display: showing pictures inline instead.\n");
        mode = 2;
      } else mode = 1;
    }
  }
  else {
    if (strcmp(m,"auto"))
      printf("Terminal display: unknown GA_TERM_MODE \"%s\"; showing pictures inline.\n",m);
    mode = 2;
  }

  if (mode==3) {
    printf("Terminal display: pictures are written to %s\n",tdir);
    if (v) printf("Terminal display: view them with  %s %s\n",v,tdir);
  }
}

/* tell the interface that we are in batch mode */

void gxdbat (void) {
  batch = 1;
}

/* User-defined picture size, "WIDTHxHEIGHT"; any "+x+y" part is ignored.
   Must be called before gxdbgn to have any effect. */

void gxdgeo (char *arg) {
  ugeom = arg;
}

void gxdbgn (gadouble xsz, gadouble ysz) {
gaint dw,dh,uw,uh;
char *s;
gadouble f;

  xsize = xsz;
  ysize = ysz;

  if (xsize >= ysize) {
    dw = TERM_DEFAULT_SIZE;
    dh = (gaint)((gadouble)dw*ysz/xsz + 0.5);
  } else {
    dh = TERM_DEFAULT_SIZE;
    dw = (gaint)((gadouble)dh*xsz/ysz + 0.5);
  }
  if (ugeom && sscanf(ugeom,"%dx%d",&uw,&uh)==2 && uw>0 && uh>0) {
    dw = uw;
    dh = uh;
  }

  s = getenv("GA_TERM_SCALE");
  if (s && *s) {
    f = atof(s);
    if (f>=1.0 && f<=4.0) scale = f;
    else printf("Terminal display: GA_TERM_SCALE must be between 1 and 4; using %g\n",scale);
  }

  width = dw;
  height = dh;
  xscl = (gadouble)(dw)/xsize;
  yscl = (gadouble)(dh)/ysize;
  dblmode = 0;

  termsetup();
  startworker();                    /* after the tmux split, which forks */

  surface = newsurface();
  gxCbgn(surface,xsz,ysz,dw,dh);
  gxCfrm();      /* a new image is transparent; an X window starts out painted */
  dirty = 1;
}

void gxdend (void) {
char *argv[8];
char fn[700];

  stopworker();                     /* finishes what it was given */
  gxCend();
  if (surface) {
    cairo_surface_finish (surface);
    cairo_surface_destroy (surface);
    surface = NULL;
  }
  if (mode==1 && pane[0]) {
    argv[0] = "tmux"; argv[1] = "kill-pane"; argv[2] = "-t";
    argv[3] = pane; argv[4] = NULL;
    runcmd(argv,NULL,0);
  }
  if (ownsdir) {
    snprintf(fn,sizeof(fn),"%s/plot.png",tdir);      unlink(fn);
    snprintf(fn,sizeof(fn),"%s/.plot.png.tmp",tdir); unlink(fn);
    snprintf(fn,sizeof(fn),"%s/plot.gif",tdir);      unlink(fn);
    snprintf(fn,sizeof(fn),"%s/.plot.gif.tmp",tdir); unlink(fn);
    unlink(seqpath);
    unlink(seqtmp);
    if (fifopath[0]) unlink(fifopath);
    rmdir(tdir);
  }
}

/* Frame action.  Values for action are:
      0 -- new frame (clear display), wait before clearing.
      1 -- new frame, no wait.
      2 -- New frame in double buffer mode.
      7 -- new frame, but just clear graphics.
      8 -- clear only the event queue.
      9 -- flush the request buffer
   There are no events here, so only clearing matters. */

void gxdfrm (gaint iact) {
  if (iact==0 || iact==1 || iact==7) {
    frameend(0);                             /* the drawn page is a finished frame */
    gxCfrm();
    dirty = 1;
    drawn = 0;
  }
}

/* There is no mouse. When asked to wait for a click, show the picture and
   wait for Enter instead, so scripts that pause on "q pos" still pause. */

void gxdbtn (gaint flag, gadouble *xpos, gadouble *ypos,
	     gaint *mbtn, gaint *type, gaint *info, gadouble *rinfo) {
char line[256];
gaint i;

  *xpos = -999.9;
  *ypos = -999.9;
  *mbtn = -1;
  *type = -1;
  for (i=0; i<10; i++) *(info+i) = 0;
  for (i=0; i<4; i++) *(rinfo+i) = 0.0;
  if (batch || !flag) return;
  gxdidle();
  printf("Terminal display has no mouse; press Enter to continue ");
  fflush(stdout);
  if (fgets(line,sizeof(line),stdin)) *mbtn = 1;
}

/* Drawing changes the visible picture, or in double-buffer mode the
   hidden one that the next swap shows */

static void touched (void) {
  if (dblmode) backdrawn = 1;
  else dirty = drawn = 1;
}

gaint gxdacol (gaint clr, gaint red, gaint green, gaint blue, gaint alpha) {
  return(0);
}

void gxdcol (gaint clr) {
  gxCcol(clr);
}

void gxdwid (gaint wid){
  gxCwid(wid);
}

void gxdmov (gadouble x, gadouble y){
  gxCmov(x,y);
}

void gxddrw (gadouble x, gadouble y) {
  gxCdrw (x,y);
  touched();
}

void gxdrec (gadouble x1, gadouble x2, gadouble y1, gadouble y2) {
  gxCrec(x1,x2,y1,y2);
  touched();
}

void gxddbl (void) {                         /* turn on double buffer mode */
  frameend(0);                               /* the page is about to be cleared */
  gxCfrm();                                  /* clear the foreground */
  if (surface2==NULL) surface2 = newsurface();
  gxCsfc(surface2);                          /* draw on the background from now on */
  gxCfrm();
  dblmode = 1;
  backdrawn = 0;
  dirty = 1;
  drawn = 0;
}

void gxdswp (void) {                         /* copy the background to the foreground */
  if (dblmode) {
    gxCflush(1);
    if (!backdrawn) frameend(0);             /* the page goes blank */
    gxCswp(surface,surface2);
    dirty = 1;
    drawn = backdrawn;
    if (backdrawn) frameend(1);              /* a new frame is showing */
    gxCfrm();                                /* clear the background */
    backdrawn = 0;
  }
}

void gxdsgl (void) {                         /* turn off double buffer mode */
  if (dblmode) {
    gxCsfc(surface);                         /* draw on the foreground again */
    cairo_surface_destroy(surface2);
    surface2 = NULL;
  }
  dblmode = 0;
  backdrawn = 0;
}

void gxdfil (gadouble *xy, gaint n) {
  gxCfil (xy,n);
  touched();
}

/* "set xsize": make a picture of the new size and redraw into it */

void gxdxsz (gaint xx, gaint yy) {
  if (batch) return;
  if (xx<=0 || yy<=0 || (xx==width && yy==height)) return;
  if (dblmode) gxdsgl();
  width = xx;
  height = yy;
  xscl = (gadouble)(width)/xsize;
  yscl = (gadouble)(height)/ysize;
  cairo_surface_destroy(surface);            /* the worker has its own copies */
  surface = newsurface();
  gxCsfc(surface);
  gxCrsiz(width,height);
  gxhdrw(0,0);
  dirty = 1;
}

/* widgets need a window; all are no-ops */

void gxdpbn (gaint bnum, struct gbtn *pbn, gaint redraw, gaint btnrel, gaint nstat) {
  printf("Warning: The terminal display does not support buttons\n");
}

void gxdrmu (gaint mnum, struct gdmu *pmu, gaint redraw, gaint nstat) {
  printf("Warning: The terminal display does not support drop menus\n");
}

void gxdrbb (gaint num, gaint type, gadouble xlo, gadouble ylo, gadouble xhi, gadouble yhi, gaint mbc) {
  printf("Warning: The terminal display does not support rubber band widgets\n");
}

char *gxdlg (struct gdlg *qry) {
  printf("Warning: The terminal display does not support dialog boxes\n");
  return (NULL);
}

void gxrs1wd (int wdtyp, int wdnum) {
}

void gxdssv (int frame) {
  printf("Warning: The terminal display does not support the screen command\n");
}
void gxdssh (int cnt) {
  printf("Warning: The terminal display does not support the screen command\n");
}
void gxdsfr (int frame) {
  printf("Warning: The terminal display does not support the screen command\n");
}

void gxdptn (int typ, int den, int ang) {
}

void gxdbb(char *filename) {
}

void gxdfb(char *filename)  {
}

gaint win_data (struct xinfo *xinf) {
  return (0);
}

/* Given x,y page location, return picture pixel location */

void gxdgcoord (gadouble x, gadouble y, gaint *i, gaint *j) {
  if (batch) {
    *i = -999;
    *j = -999;
    return;
  }
  *i = (gaint)(x*xscl+0.5);
  *j = height - (gaint)(y*yscl+0.5);
}

void gxdimg(gaint *im, gaint imin, gaint jmin, gaint isiz, gaint jsiz) {
  printf("Warning: The terminal display does not support 'gxout imap'\n");
}

gadouble gxdqchl (char ch, gaint fn, gadouble w) {
  return (gxCqchl(ch,fn,w));
}

gadouble gxdch (char ch, gaint fn, gadouble x, gadouble y, gadouble w, gadouble h, gadouble rot) {
  touched();
  return (gxCch(ch,fn,x,y,w,h,rot));
}

/* Called by gxC.c when drawing is complete; the picture is written out when
   GrADS next waits for the user (gxdidle), not after every command. */
void gxdXflush (void) {
}

void gxdopt (gaint opt) {
  if (opt==4) gxCflush(1);
}

void gxsetpatt (gaint pnum) {
  gxCpattrset(pnum);
}

void gxdsignal (gaint sig) {
  if (sig==1) gxCflush(1);   /* finish rendering */
  if (sig==2) gxCaa(0);      /* disable anti-aliasing */
  if (sig==3) gxCaa(1);      /* enable anti-aliasing */
  if (sig==4) gxCpush();     /* push */
  if (sig==5) { gxCpop(); touched(); }  /* pop and paint */
}

void gxdclip (gadouble xlo, gadouble xhi, gadouble ylo, gadouble yhi) {
  gxCclip (xlo,xhi,ylo,yhi);
}

void gxdcfg (void) {
  printf("Terminal ");
  gxCcfg();
}

gaint gxdckfont (void) {
  return (1);
}
