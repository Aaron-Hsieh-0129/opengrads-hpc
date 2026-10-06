/* Added in 2026 for terminal graphics display; see COPYING. */

/* Terminal display interface for Cairo -- draws into an off-screen image and
   shows it in the terminal instead of an X window.

   The picture is rendered by gxC.c into a Cairo image surface. Whenever
   GrADS is about to wait for the user (the command prompt, a script "pull",
   a "q pos"), gxdidle hands a copy of the picture to a worker thread if it
   changed since the last time. The worker encodes it and sends it to the
   terminal with the iTerm2 inline image protocol, so the prompt comes back
   without waiting. Where the picture goes depends on GA_TERM_MODE:

     tmux    A pane is split off beside GrADS (it runs GA_TERM_VIEWER --hold,
             normally libexec/grads-termview, only to keep the pane open) and
             the worker draws each picture into it.
     inline  The picture is printed into the terminal below the command,
             like a notebook. Needs no tmux.
     file    The picture is only written; grads-termview can show it
             elsewhere, started by hand with the command printed at start-up.
     auto    tmux when GrADS runs inside tmux and a viewer is available,
             inline otherwise. This is the default.

   Sending. Inside tmux the image sequence carries its own cursor movement,
   because tmux does not place passthrough output at the pane. tmux keeps
   whatever it is given and sends it on as fast as the link allows, so the
   worker writes a picture in pieces and waits for the tmux client's terminal
   to have room before the next piece: pictures queue here, where Ctrl-C can
   drop them, instead of in tmux. With iTerm2 (LC_TERMINAL=iTerm2) a picture
   goes in parts, which keeps the previous picture up until the new one is
   complete, and when the link is slow or the picture large, iTerm2's own
   progress bar (OSC 9;4) is updated between the parts, so it shows what has
   actually arrived.

   Animation. A frame ends where the picture is replaced: at a "swap" in
   double-buffer mode, or when a drawn page is cleared. Every frame is sent,
   in order, as it is made, as an X window would show it; when the link is
   slower than the drawing, the drawing waits. Ctrl-C stops the command,
   drops the frames not yet sent, and sends nothing more for it.
   GA_TERM_ANIM picks the behaviour:

     live    frames are shown as they are made (the default)
     gif     as live, and a command that swaps two or more frames (a
             "set dbuff on" loop, "set looping on") also leaves a looping
             animated GIF, which iTerm2 plays by itself
     off     only the picture at the prompt is shown

   Nothing here needs an X server, so it works over plain ssh.

   Other settings:
     GA_TERM_DIR        Directory for the pictures (default: a new
                        temporary one)
     GA_TERM_SCALE      Pixel density factor, 1 to 4 (default 2, for Retina)
     GA_TERM_PANE       Width of the tmux viewer pane, e.g. 45% (default 50%)
     GA_TERM_WIDTH      Width of an inline image, as iTerm2 understands it
                        (default 70%)
     GA_TERM_PROGRESS   auto (default), on (for every picture), or off (no
                        progress bar, and pictures in one piece when they fit)
     GA_TERM_ANIM_DELAY Seconds per GIF frame (default 0.2)
     GA_TERM_ANIM_MAX   Most frames kept in one GIF (default 300)
     GA_TERM_ANIM_SCALE Size of GIF frames relative to the page, 0.25 to 1
                        (default 1); 0.5 roughly halves the data
     GA_TERM_SYNC       1 waits for each picture to be written and sent
                        before going on, for scripts and tests

   The picture size comes from the -g option ("-g 1200x900"), otherwise it
   is 1000 points along the longer side of the page. GIF frames are kept at
   that size; the still picture has GA_TERM_SCALE times as many pixels each
   way.  */

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
#include <poll.h>
#include <time.h>
#include <spawn.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#include <cairo.h>
#include <zlib.h>

#include "gatypes.h"
#include "gx.h"
#include "gxC.h"

extern char **environ;

#define TERM_DEFAULT_SIZE 1000       /* points along the longer page side */
#define SEQ_LIMIT 1000000            /* iTerm2 and tmux drop control sequences
                                        over 1 MiB, so larger files go in parts */
#define SEQ_PART  65536              /* base64 characters in each part */
#define PACE_QUIET 0.025             /* seconds the link must stay clear */
#define PROGRESS_MIN 1048576         /* a picture this large always shows progress */
#define FRAMEQ    2                  /* frames waiting for the worker */

void gxdXflush (void);
void gxdidle (void);
void gxdintr (void);

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
static gaint ngif=0;                        /* frames queued for a GIF in this command */
static gaint gifcut=0;                      /* frames left out past GA_TERM_ANIM_MAX */
static volatile sig_atomic_t intr=0;        /* Ctrl-C interrupted the command */

/* Settings */
static char tdir[512];                      /* where the pictures go */
static gaint ownsdir=0;                     /* we created tdir, remove it at exit */
static char seqpath[600], seqtmp[600], fifopath[600];
static gaint mode=0;                        /* 1=tmux 2=inline 3=file */
static gaint anim=1;                        /* 0=off 1=live 2=gif */
static gaint animdelay=20;                  /* hundredths of a second per GIF frame */
static gaint animmax=300;                   /* most frames in one GIF */
static gadouble animscale=1.0;              /* GIF frame size relative to the page */
static gaint syncwrite=0;                   /* wait for each picture */
static gaint iterm=0;                       /* the terminal is iTerm2 */
static gaint progressopt=1;                 /* 0=off 1=auto 2=on */
static char pane[64];                       /* tmux pane id of the viewer */
static gaint panefd=-1;                     /* the viewer pane's terminal */

/* ---- work handed to the worker thread ---- */

#define JOB_STILL    1                      /* a still picture; a newer one replaces it */
#define JOB_FRAME    2                      /* an animation frame; never skipped */
#define JOB_GIFFRAME 3                      /* add a frame to the looping GIF */
#define JOB_GIFEND   4                      /* finish the GIF and show it */
#define JOB_GIFDROP  5                      /* forget the GIF */

#define ISFRAME(k) ((k)==JOB_FRAME || (k)==JOB_GIFFRAME)

struct tjob {
  gaint kind;
  unsigned char *px;                        /* copy of the ARGB32 picture */
  gaint w,h,stride;                         /* its size in device pixels */
  gaint lw,lh;                              /* size in points, for GIF frames */
  gaint first;                              /* first frame of a new GIF */
  struct tjob *next;
};

static pthread_t worker;
static gaint workeron=0;
static pthread_mutex_t tlock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t twake = PTHREAD_COND_INITIALIZER;  /* work for the worker */
static pthread_cond_t tdone = PTHREAD_COND_INITIALIZER;  /* the worker made progress */
static struct tjob *qhead=NULL,*qtail=NULL; /* work, in order */
static gaint qframes=0;                     /* frames in the queue */
static gaint busy=0;                        /* the worker is working */
static volatile gaint stopping=0;           /* finish the queue and exit */
static volatile gaint quitting=0;           /* GrADS is ending: write, don't send */
static gaint seq=0;                         /* pictures shown */
static unsigned char *lastpic=NULL;         /* and its contents */
static size_t lastpiclen=0;

/* Worker only: what the pane shows, and the link */
static gaint sentrows=0,sentcols=0;         /* pane size the picture was sent for */
static gaint panefresh=1;                   /* the pane needs clearing first */
static double slowuntil=0.0;                /* the link was slow until about then */

static double now (void) {
struct timespec t;
  clock_gettime(CLOCK_MONOTONIC,&t);
  return (t.tv_sec + t.tv_nsec*1e-9);
}

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

static void cloexec (gaint fd) {
  if (fd>=0) fcntl(fd,F_SETFD,FD_CLOEXEC);
}

/* Run a command without a shell. Its first line of output goes into out.
   Returns the exit status, or -1 when it could not be run. posix_spawn,
   not fork, because the worker thread runs commands too. */

static gaint runcmd (char *const argv[], char *out, size_t len) {
gaint fd[2],status,rc;
pid_t pid;
ssize_t n;
size_t got=0;
char buf[256],*nl;
posix_spawn_file_actions_t fa;
posix_spawnattr_t at;
sigset_t none;

  if (out && len) *out = '\0';
  if (pipe(fd)) return (-1);
  cloexec(fd[0]);
  cloexec(fd[1]);
  posix_spawn_file_actions_init(&fa);
  posix_spawn_file_actions_addopen(&fa,0,"/dev/null",O_RDONLY,0);
  posix_spawn_file_actions_addopen(&fa,2,"/dev/null",O_WRONLY,0);
  posix_spawn_file_actions_adddup2(&fa,fd[1],1);
  posix_spawnattr_init(&at);
  sigemptyset(&none);                       /* the worker blocks every signal */
  posix_spawnattr_setsigmask(&at,&none);
  posix_spawnattr_setflags(&at,POSIX_SPAWN_SETSIGMASK);
  rc = posix_spawnp(&pid,argv[0],&fa,&at,argv,environ);
  posix_spawn_file_actions_destroy(&fa);
  posix_spawnattr_destroy(&at);
  close(fd[1]);
  if (rc) { close(fd[0]); return (-1); }
  while ((n=read(fd[0],buf,sizeof(buf)))!=0) {
    if (n<0) { if (errno==EINTR) continue; break; }
    if (out && got+1<len) {
      if ((size_t)n > len-1-got) n = len-1-got;
      memcpy(out+got,buf,n);
      got += n;
      out[got] = '\0';
    }
  }
  if (out && len && (nl=strchr(out,'\n'))!=NULL) *nl = '\0';
  close(fd[0]);
  while (waitpid(pid,&status,0)<0) if (errno!=EINTR) return (-1);
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

/* Where a tmux pane is on the client's screen, and the client's terminal */

struct paneinfo {
  gaint row,col;                            /* top left, 0-based, on the screen */
  gaint rows,cols;
  char ctty[256];                           /* the tmux client's terminal */
};

static gaint tmuxinfo (const char *target, struct paneinfo *pi) {
char out[600],*f[8],*p,*argv[8];
gaint i,n,status;

  memset(pi,0,sizeof(*pi));
  i = 0;
  argv[i++] = "tmux"; argv[i++] = "display-message"; argv[i++] = "-p";
  if (target && *target) { argv[i++] = "-t"; argv[i++] = (char *)target; }
  argv[i++] = "#{pane_left}|#{pane_top}|#{pane_width}|#{pane_height}|"
              "#{client_tty}|#{status-position}|#{status}";
  argv[i] = NULL;
  if (runcmd(argv,out,sizeof(out))) return (1);
  for (n=0, p=out; n<7; n++) {               /* fields may be empty */
    f[n] = p;
    p = strchr(p,'|');
    if (p==NULL) break;
    *p++ = '\0';
  }
  if (n<6) return (1);
  pi->col = atoi(f[0]);
  pi->row = atoi(f[1]);
  pi->cols = atoi(f[2]);
  pi->rows = atoi(f[3]);
  snprintf(pi->ctty,sizeof(pi->ctty),"%s",f[4]);
  if (!strcmp(f[5],"top")) {                 /* the status line comes first */
    if (!strcmp(f[6],"on")) status = 1;
    else if (!strcmp(f[6],"off")) status = 0;
    else status = atoi(f[6]);
    pi->row += status;
  }
  return (pi->cols<=0 || pi->rows<=0);
}

/* Split a tmux pane off for the pictures */

static gaint tmuxsplit (void) {
char cmd[2048],qv[700],qd[700],size[32],pct[32],tty[256];
char *v,*p;
char *argv[16];
gaint i,rc;

  v = viewer();
  if (v==NULL) return (1);
  shquote(qv,sizeof(qv),v);
  shquote(qd,sizeof(qd),tdir);
  snprintf(cmd,sizeof(cmd),"exec %s --hold %s %d",qv,qd,(gaint)getpid());

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

  /* the pictures go straight to the pane's terminal */
  i = 0;
  argv[i++] = "tmux"; argv[i++] = "display-message"; argv[i++] = "-p";
  argv[i++] = "-t"; argv[i++] = pane; argv[i++] = "#{pane_tty}"; argv[i] = NULL;
  if (runcmd(argv,tty,sizeof(tty)) || tty[0]!='/' ||
      (panefd=open(tty,O_WRONLY|O_NOCTTY))<0) {
    argv[0] = "tmux"; argv[1] = "kill-pane"; argv[2] = "-t";
    argv[3] = pane; argv[4] = NULL;
    runcmd(argv,NULL,0);
    pane[0] = '\0';
    return (1);
  }
  cloexec(panefd);

  /* tmux 3.3 and later drop passthrough sequences unless the pane allows
     them. Older tmux has no such option. */
  i = 0;
  argv[i++] = "tmux"; argv[i++] = "set-option"; argv[i++] = "-p";
  argv[i++] = "-t"; argv[i++] = pane; argv[i++] = "allow-passthrough";
  argv[i++] = "on"; argv[i] = NULL;
  runcmd(argv,NULL,0);
  return (0);
}

/* ---- PNG ---- */

struct mbuf {
  unsigned char *p;
  size_t n,cap;
  gaint failed;
};

static void mput (struct mbuf *b, const void *d, size_t n) {
unsigned char *np;
size_t nc;
  if (b->failed) return;
  if (b->n+n > b->cap) {
    nc = b->cap ? b->cap : 262144;
    while (nc < b->n+n) nc *= 2;
    np = (unsigned char *)realloc(b->p,nc);
    if (np==NULL) { b->failed = 1; return; }
    b->p = np;
    b->cap = nc;
  }
  memcpy(b->p+b->n,d,n);
  b->n += n;
}

static void be32 (unsigned char *p, unsigned long v) {
  p[0] = (v>>24)&255; p[1] = (v>>16)&255; p[2] = (v>>8)&255; p[3] = v&255;
}

static void pngchunk (struct mbuf *b, const char *type, const unsigned char *data, size_t len) {
unsigned char hdr[8],crc[4];
uLong c;
  be32(hdr,(unsigned long)len);
  memcpy(hdr+4,type,4);
  c = crc32(0L,(const Bytef *)type,4);
  if (len) c = crc32(c,data,(uInt)len);
  be32(crc,c);
  mput(b,hdr,8);
  if (len) mput(b,data,len);
  mput(b,crc,4);
}

/* Encode an ARGB32 picture as an RGB PNG. Rows are not filtered: for plots,
   which are mostly flat colour, that is both faster and smaller than
   letting the encoder try every filter, as cairo_surface_write_to_png does. */

static gaint pngencode (const unsigned char *px, gaint w, gaint h, gaint stride, struct mbuf *b) {
static const unsigned char sig[8] = {137,80,78,71,13,10,26,10};
unsigned char ihdr[13],*row,*out;
const unsigned int *p;
z_stream zs;
gaint x,y,zrc,err=0;
const size_t outsz = 65536;

  row = (unsigned char *)malloc(1+3*(size_t)w);
  out = (unsigned char *)malloc(outsz);
  memset(&zs,0,sizeof(zs));
  if (row==NULL || out==NULL || deflateInit(&zs,6)!=Z_OK) {
    free(row); free(out);
    return (1);
  }
  be32(ihdr,w); be32(ihdr+4,h);
  ihdr[8] = 8; ihdr[9] = 2; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
  mput(b,sig,8);
  pngchunk(b,"IHDR",ihdr,13);

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
        pngchunk(b,"IDAT",out,outsz-zs.avail_out);
        zs.next_out = out;
        zs.avail_out = outsz;
      }
    } while (y<h ? zs.avail_in>0 : zrc!=Z_STREAM_END);
  }
  deflateEnd(&zs);
  pngchunk(b,"IEND",NULL,0);
  free(row);
  free(out);
  return (err || b->failed);
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

/* ---- sending pictures to the terminal ---- */

struct wout {
  gaint fd;                                 /* where the bytes go */
  gaint pacefd;                             /* the terminal to pace against, or -1 */
  gaint tmux;                               /* wrap sequences for tmux */
  unsigned char buf[16384];
  size_t n;
  double waited;                            /* time spent waiting on the link */
  gaint err;
};

static void wflush (struct wout *w) {
size_t off=0;
ssize_t r;
double t0;

  t0 = now();
  while (off<w->n && !w->err) {
    r = write(w->fd,w->buf+off,w->n-off);
    if (r<0) {
      if (errno==EINTR) continue;
      if (errno==EAGAIN) { usleep(2000); continue; }
      w->err = 1;
      break;
    }
    off += r;
  }
  w->waited += now()-t0;
  w->n = 0;
}

/* tmux keeps everything it is given and sends it on as fast as the link
   allows. Before a picture, wait until tmux has passed on what it holds,
   so that pictures queue here, where Ctrl-C can drop them: the tmux
   client's terminal then has room, and keeps it. While tmux still has
   more to send it fills that room again within a millisecond or two, so
   the room has to last PACE_QUIET to count. */

static void pace (struct wout *w) {
struct pollfd p;
double t0,clear=-1.0,t;

  if (w->pacefd<0) return;
  t0 = now();
  while (!stopping) {
    p.fd = w->pacefd;
    p.events = POLLOUT;
    p.revents = 0;
    if (poll(&p,1,0)<0) {
      if (errno==EINTR) continue;
      break;
    }
    if (p.revents & (POLLERR|POLLHUP|POLLNVAL)) break;
    t = now();
    if (p.revents & POLLOUT) {
      if (clear<0) clear = t;
      if (t-clear >= PACE_QUIET) break;
    } else clear = -1.0;
    if (t-t0 > 30.0) break;                 /* never hang on a stuck link */
    usleep(2500);
  }
  w->waited += now()-t0-PACE_QUIET;
}

static void wraw (struct wout *w, const void *p, size_t n) {
const unsigned char *s = (const unsigned char *)p;
size_t k;
  while (n && !w->err) {
    k = sizeof(w->buf)-w->n;
    if (k>n) k = n;
    memcpy(w->buf+w->n,s,k);
    w->n += k; s += k; n -= k;
    if (w->n==sizeof(w->buf)) wflush(w);
  }
}

static void wstr (struct wout *w, const char *s) {
  wraw(w,s,strlen(s));
}

/* An escape sequence for the outer terminal. Inside tmux it travels in a
   passthrough sequence, where each ESC is doubled. */
static void wesc (struct wout *w, const char *s) {
  if (!w->tmux) { wstr(w,s); return; }
  for (; *s; s++) {
    if (*s=='\033') wraw(w,"\033\033",2);
    else wraw(w,s,1);
  }
}

static void ptopen (struct wout *w) {
  if (w->tmux) wstr(w,"\033Ptmux;");
}

static void ptclose (struct wout *w) {
  if (w->tmux) wstr(w,"\033\\");
}

static void b64put (struct wout *w, const unsigned char *d, size_t len) {
static const char tab[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
char out[4096];
size_t i,o=0;
unsigned long v;
  for (i=0; i+2<len; i+=3) {
    v = ((unsigned long)d[i]<<16) | ((unsigned long)d[i+1]<<8) | d[i+2];
    out[o++] = tab[(v>>18)&63]; out[o++] = tab[(v>>12)&63];
    out[o++] = tab[(v>>6)&63];  out[o++] = tab[v&63];
    if (o==sizeof(out)) { wraw(w,out,o); o = 0; }
  }
  if (i<len) {
    v = (unsigned long)d[i]<<16;
    if (i+1<len) v |= (unsigned long)d[i+1]<<8;
    out[o++] = tab[(v>>18)&63]; out[o++] = tab[(v>>12)&63];
    out[o++] = i+1<len ? tab[(v>>6)&63] : '=';
    out[o++] = '=';
  }
  wraw(w,out,o);
}

/* Send a picture. Given a pane, it fills the pane, placed by absolute
   cursor movement inside the image sequence: tmux does not move the
   terminal's cursor to the pane for passthrough output. Otherwise the
   picture goes where the cursor is, iwidth wide. */

static void sendpic (gaint fd, gaint pacefd, gaint tmux, const struct paneinfo *pi,
                     gaint clear, const unsigned char *data, size_t len, const char *iwidth) {
struct wout *w;
char args[256],pos[64],tmp[64];
size_t b64len,off,n,part;
gaint parts,showprog,pct,lastpct=-1;

  w = (struct wout *)calloc(1,sizeof(struct wout));
  if (w==NULL) return;
  w->fd = fd;
  w->pacefd = pacefd;
  w->tmux = tmux;
  b64len = (len+2)/3*4;
  if (pi) snprintf(args,sizeof(args),"inline=1;size=%lu;width=%d;height=%d;preserveAspectRatio=1",
                   (unsigned long)len,pi->cols,pi->rows>1 ? pi->rows-1 : 1);
  else snprintf(args,sizeof(args),"inline=1;size=%lu;width=%s;preserveAspectRatio=1",
                (unsigned long)len,iwidth);
  pos[0] = '\0';
  if (pi) snprintf(pos,sizeof(pos),"\0337\033[%d;%dH",pi->row+1,pi->col+1);

  pace(w);
  if (w->waited>0.15) slowuntil = now()+20.0;
  w->waited = 0.0;

  /* iTerm2 takes a picture in parts, which also lets the progress bar
     follow it; elsewhere only a picture too large for one sequence is
     split, which needs iTerm2 3.5 anyway */
  parts = (b64len+200 >= SEQ_LIMIT) || (iterm && progressopt);
  showprog = parts && progressopt &&
             (progressopt==2 || b64len>=PROGRESS_MIN || now()<slowuntil);

  if (pi && clear) wstr(w,"\033[H\033[2J");  /* tmux clears the pane's cells */
  if (!parts) {
    ptopen(w);
    wesc(w,pos);
    wesc(w,"\033]1337;File=");
    wstr(w,args);
    wstr(w,":");
    b64put(w,data,len);
    wesc(w,"\a");
    if (pi) wesc(w,"\0338");
    ptclose(w);
  } else {
    ptopen(w);
    wesc(w,pos);
    wesc(w,"\033]1337;MultipartFile=");
    wstr(w,args);
    wesc(w,"\a");
    if (pi) wesc(w,"\0338");
    ptclose(w);
    part = SEQ_PART/4*3;
    for (off=0; off<len && !w->err; off+=n) {
      n = len-off;
      if (n>part) n = part;
      ptopen(w);
      wesc(w,"\033]1337;FilePart=");
      b64put(w,data+off,n);
      wesc(w,"\a");
      ptclose(w);
      /* writing blocks on a slow link outside tmux: show progress */
      if (!showprog && progressopt && w->waited>0.15) showprog = 1;
      if (showprog) {
        pct = (gaint)((off+n)*100/len);
        if (pct!=lastpct) {
          snprintf(tmp,sizeof(tmp),"\033]9;4;1;%d\a",pct);
          ptopen(w); wesc(w,tmp); ptclose(w);
          lastpct = pct;
        }
      }
    }
    ptopen(w);
    wesc(w,pos);
    wesc(w,"\033]1337;FileEnd\a");
    if (pi) wesc(w,"\0338");
    ptclose(w);
    if (lastpct>=0) { ptopen(w); wesc(w,"\033]9;4;0\a"); ptclose(w); }
  }
  if (!pi) wstr(w,"\n");
  wflush(w);
  if (w->waited>0.15) slowuntil = now()+20.0;
  free(w);
}

/* The tmux client's terminal, to pace against */

static gaint ctyfd=-1;
static char ctypath[256];

static gaint pacefor (const char *path) {
  if (path==NULL || *path!='/') return (-1);
  if (ctyfd>=0 && !strcmp(path,ctypath)) return (ctyfd);
  if (ctyfd>=0) close(ctyfd);
  ctyfd = open(path,O_WRONLY|O_NOCTTY|O_NONBLOCK);
  cloexec(ctyfd);
  snprintf(ctypath,sizeof(ctypath),"%s",path);
  return (ctyfd);
}

/* Draw the latest picture into the viewer pane; worker only */

static void panesend (gaint clear) {
struct paneinfo pi;

  if (panefd<0 || lastpic==NULL || quitting) return;
  if (tmuxinfo(pane,&pi)) return;           /* without its place it would land anywhere */
  if (pi.rows!=sentrows || pi.cols!=sentcols) clear = 1;
  sentrows = pi.rows;
  sentcols = pi.cols;
  sendpic(panefd,pacefor(pi.ctty),1,&pi,clear || panefresh,lastpic,lastpiclen,NULL);
  panefresh = 0;
}

/* The pane changed size: draw the picture again to fit */

static void checkresize (void) {
struct winsize ws;
  if (panefd<0 || lastpic==NULL) return;
  if (ioctl(panefd,TIOCGWINSZ,&ws)) return;
  if (ws.ws_row!=sentrows || ws.ws_col!=sentcols) panesend(1);
}

/* ---- worker thread ---- */

/* Record the new picture and wake a viewer started by hand */

static void shown (const char *name) {
FILE *f;
gaint fd,n;

  pthread_mutex_lock(&tlock);
  n = ++seq;
  pthread_mutex_unlock(&tlock);
  f = fopen(seqtmp,"w");
  if (f) {
    fprintf(f,"%d %s\n",n,name);
    fclose(f);
    rename(seqtmp,seqpath);
  }
  /* a viewer keeps the FIFO open; without one, open fails at once */
  fd = open(fifopath,O_WRONLY|O_NONBLOCK);
  if (fd>=0) {
    if (write(fd,"\n",1)<0) { /* full or gone: the viewer catches up anyway */ }
    close(fd);
  }
}

/* Write a picture file, keep it as the latest picture (taking the buffer),
   and show it */

static void publish (const char *name, unsigned char *data, size_t len) {
char fn[700],tmp[700];
FILE *f;
gaint ok;

  snprintf(fn,sizeof(fn),"%s/%s",tdir,name);
  snprintf(tmp,sizeof(tmp),"%s/.%s.tmp",tdir,name);
  f = fopen(tmp,"wb");
  ok = 0;
  if (f) {
    ok = fwrite(data,1,len,f)==len;
    if (fclose(f)) ok = 0;
  }
  if (ok && !rename(tmp,fn)) {
    pthread_mutex_lock(&tlock);
    free(lastpic);
    lastpic = data;
    lastpiclen = len;
    pthread_mutex_unlock(&tlock);
    shown(name);
    panesend(0);
  } else {
    unlink(tmp);
    free(data);
  }
}

static void dojob (struct tjob *j) {
struct mbuf mb;
unsigned char *rgb;

  if (j->kind==JOB_STILL || j->kind==JOB_FRAME) {
    memset(&mb,0,sizeof(mb));
    if (pngencode(j->px,j->w,j->h,j->stride,&mb)) free(mb.p);
    else publish("plot.png",mb.p,mb.n);
  }
  else if (j->kind==JOB_GIFFRAME) {
    if (j->first) gifreset();
    rgb = downsample(j,j->lw,j->lh);
    if (rgb) gifframe(rgb,j->lw,j->lh);
    else gif.failed = 1;
    free(rgb);
  }
  else if (j->kind==JOB_GIFEND) {
    if (gif.frames>0 && !gif.failed) gbyte(0x3b);
    if (gif.frames>0 && !gif.failed) {
      publish("plot.gif",gif.buf,gif.len);  /* the buffer is the picture's now */
      gif.buf = NULL;
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
struct timespec ts;

  pthread_mutex_lock(&tlock);
  while (1) {
    if (!qhead) {
      if (stopping) break;
      /* wake now and then to notice the pane being resized */
      clock_gettime(CLOCK_REALTIME,&ts);
      ts.tv_nsec += 300000000L;
      if (ts.tv_nsec>=1000000000L) { ts.tv_sec++; ts.tv_nsec -= 1000000000L; }
      if (pthread_cond_timedwait(&twake,&tlock,&ts)==ETIMEDOUT && !qhead && !stopping) {
        pthread_mutex_unlock(&tlock);
        checkresize();
        pthread_mutex_lock(&tlock);
      }
      continue;
    }
    j = qhead;
    qhead = j->next;
    if (!qhead) qtail = NULL;
    if (ISFRAME(j->kind)) qframes--;
    busy = 1;
    pthread_cond_broadcast(&tdone);         /* room in the queue */
    pthread_mutex_unlock(&tlock);
    if (!intr || j->kind==JOB_GIFDROP) dojob(j);   /* after Ctrl-C, nothing more is sent */
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
  /* the worker takes no signals: Ctrl-C belongs to GrADS, and a terminal
     that went away must not kill us with SIGPIPE */
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

/* Wait until everything handed to the worker is written and sent */

static void waitworker (void) {
  if (!workeron) return;
  pthread_mutex_lock(&tlock);
  while (qhead || busy) pthread_cond_wait(&tdone,&tlock);
  pthread_mutex_unlock(&tlock);
}

/* Forget the work not yet started; with tlock held */

static void dropqueue (gaint stillsonly) {
struct tjob **pp,*k;
  for (pp=&qhead; *pp; ) {
    k = *pp;
    if (!stillsonly || k->kind==JOB_STILL) {
      *pp = k->next;
      if (ISFRAME(k->kind)) qframes--;
      freejob(k);
    } else pp = &k->next;
  }
  for (qtail=qhead; qtail && qtail->next; qtail=qtail->next) ;
  pthread_cond_broadcast(&tdone);
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
  if (kind==JOB_STILL || ISFRAME(kind)) {
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

/* Hand work to the worker, in order. A still picture replaces one not yet
   started. A frame waits for room, as a slow link holds up an X window;
   Ctrl-C ends the wait and the frame is dropped. Without a worker the work
   is done here and now. */

static void post (struct tjob *j) {
struct timespec ts;

  if (j==NULL) return;
  if (intr && j->kind!=JOB_GIFDROP) { freejob(j); return; }
  if (!workeron) {
    dojob(j);
    freejob(j);
    return;
  }
  pthread_mutex_lock(&tlock);
  if (j->kind==JOB_STILL || j->kind==JOB_GIFEND) dropqueue(1);
  while (ISFRAME(j->kind) && qframes>=FRAMEQ && !intr) {
    clock_gettime(CLOCK_REALTIME,&ts);      /* look at intr now and then */
    ts.tv_nsec += 100000000L;
    if (ts.tv_nsec>=1000000000L) { ts.tv_sec++; ts.tv_nsec -= 1000000000L; }
    pthread_cond_timedwait(&tdone,&tlock,&ts);
  }
  if (intr && j->kind!=JOB_GIFDROP) {
    pthread_mutex_unlock(&tlock);
    freejob(j);
    return;
  }
  j->next = NULL;
  if (qtail) qtail->next = j;
  else qhead = j;
  qtail = j;
  if (ISFRAME(j->kind)) qframes++;
  pthread_cond_signal(&twake);
  pthread_mutex_unlock(&tlock);
  if (syncwrite) waitworker();
}

/* ---- frames, Ctrl-C and the idle point; main thread ---- */

/* The visible picture is about to be replaced, or has just been by a swap */

static void frameend (gaint swapped) {
struct tjob *j;

  if (batch || surface==NULL || !dirty || !drawn || intr) return;
  if (anim==2 && swapped) {
    if (ngif<animmax) {
      j = snapshot(JOB_GIFFRAME);
      if (j) {
        j->first = (ngif==0);
        post(j);
        ngif++;
      }
    } else gifcut++;
  }
  if (anim!=0 && mode!=2) {                 /* show it now */
    post(snapshot(JOB_FRAME));
    dirty = 0;
  }
}

/* Ctrl-C while a command runs; called from the signal handler */

void gxdintr (void) {
  intr = 1;
}

/* Print the latest picture at the cursor */

static void inlineshow (void) {
unsigned char *d;
size_t n;
gaint fd,tmux,pacefd=-1;
struct paneinfo pi;
char *w,*t;

  pthread_mutex_lock(&tlock);
  n = lastpiclen;
  d = n ? (unsigned char *)malloc(n) : NULL;
  if (d) memcpy(d,lastpic,n);
  pthread_mutex_unlock(&tlock);
  if (d==NULL) return;
  t = getenv("TMUX");
  tmux = (t && *t);
  if (tmux && !tmuxinfo(getenv("TMUX_PANE"),&pi)) pacefd = pacefor(pi.ctty);
  w = getenv("GA_TERM_WIDTH");
  if (w==NULL || *w=='\0') w = "70%";
  fd = open("/dev/tty",O_WRONLY|O_NOCTTY);
  fflush(stdout);
  sendpic(fd>=0 ? fd : 1,pacefd,tmux,NULL,0,d,n,w);
  if (fd>=0) close(fd);
  free(d);
}

static gaint shownseq (void) {
gaint n;
  pthread_mutex_lock(&tlock);
  n = seq;
  pthread_mutex_unlock(&tlock);
  return (n);
}

/* GrADS is about to wait for the user: show the picture if it changed */

void gxdidle (void) {
gaint before,frames;

  if (batch || surface==NULL) return;

  if (intr) {                               /* Ctrl-C: send nothing more */
    pthread_mutex_lock(&tlock);
    dropqueue(0);
    pthread_mutex_unlock(&tlock);
    intr = 0;
    if (ngif) post(snapshot(JOB_GIFDROP));
    ngif = 0;
    gifcut = 0;
    dirty = 0;                              /* the screen keeps the last picture sent */
    return;
  }

  before = shownseq();
  frames = ngif;
  if (ngif>=2) {
    post(snapshot(JOB_GIFEND));
    dirty = 0;
  } else {
    if (ngif==1) post(snapshot(JOB_GIFDROP));
    if (dirty && (drawn || mode!=2)) post(snapshot(JOB_STILL));
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
    if (shownseq()!=before && (drawn || frames>=2)) inlineshow();
  }
  intr = 0;     /* a Ctrl-C while the picture went out was about that picture */
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
  if (a==NULL || *a=='\0' || !strcmp(a,"live") || !strcmp(a,"auto")) anim = 1;
  else if (!strcmp(a,"gif")) anim = 2;
  else if (!strcmp(a,"off")) anim = 0;
  else printf("Terminal display: unknown GA_TERM_ANIM \"%s\"; using live\n",a);
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
  a = getenv("GA_TERM_PROGRESS");
  if (a==NULL || *a=='\0' || !strcmp(a,"auto")) progressopt = 1;
  else if (!strcmp(a,"on")) progressopt = 2;
  else if (!strcmp(a,"off")) progressopt = 0;
  else printf("Terminal display: unknown GA_TERM_PROGRESS \"%s\"; using auto\n",a);
  a = getenv("GA_TERM_SYNC");
  syncwrite = (a && !strcmp(a,"1"));
  a = getenv("LC_TERMINAL");
  iterm = (a && !strcmp(a,"iTerm2"));

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
        else printf("Terminal display: unable to open a tmux pane for the pictures.\n");
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

  quitting = 1;                     /* finish writing pictures, send nothing more */
  stopworker();
  gxCend();
  if (surface) {
    cairo_surface_finish (surface);
    cairo_surface_destroy (surface);
    surface = NULL;
  }
  if (panefd>=0) { close(panefd); panefd = -1; }
  if (ctyfd>=0) { close(ctyfd); ctyfd = -1; }
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
