/* Added in 2026 for terminal graphics display; see COPYING. */

/* Terminal display interface for Cairo -- draws into an off-screen image and
   shows it in the terminal instead of an X window.

   The picture is rendered by gxC.c into a Cairo image surface. Whenever
   GrADS is about to wait for the user (the command prompt, a script "pull",
   a "q pos"), gxdidle writes the picture to a PNG file if it changed since
   the last time. How the PNG reaches the screen depends on GA_TERM_MODE:

     tmux    A viewer (GA_TERM_VIEWER, normally libexec/grads-termview) runs
             in a tmux pane split off beside GrADS and redraws the PNG with
             the iTerm2 inline image protocol each time it changes.
     inline  The PNG is printed into the terminal below the command, like a
             notebook. Needs no tmux and no viewer.
     file    The PNG is only written; the viewer is started by hand with the
             command printed at start-up.
     auto    tmux when GrADS runs inside tmux and a viewer is available,
             inline otherwise. This is the default.

   Nothing here needs an X server, so it works over plain ssh.

   Other settings:
     GA_TERM_DIR    Directory for plot.png (default: a new temporary one)
     GA_TERM_SCALE  Pixel density factor, 1 to 4 (default 2, for Retina)
     GA_TERM_PANE   Width of the tmux viewer pane, e.g. 45% (default 50%)
     GA_TERM_WIDTH  Width of an inline image, as iTerm2 understands it
                    (default 70%)

   The picture size comes from the -g option ("-g 1200x900"), otherwise it
   is 1000 pixels along the longer side of the page.  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <cairo.h>

#include "gatypes.h"
#include "gx.h"
#include "gxC.h"

#define TERM_DEFAULT_SIZE 1000       /* pixels along the longer page side */

void gxdXflush (void);
void gxdidle (void);

static gaint batch=0;                       /* Batch mode? */
static gadouble xscl,yscl;                  /* Pixels per inch */
static gadouble xsize, ysize;               /* Page size in inches */
static gaint dblmode;                       /* single or double buffering */
static gaint width,height;                  /* Picture size in (logical) pixels */
static gadouble scale=2.0;                  /* Device pixels per logical pixel */
static cairo_surface_t *surface=NULL,*surface2=NULL; /* front and back pictures */
static char *ugeom = NULL;                  /* -g geometry string */

static gaint dirty=0;                       /* picture changed since last publish */
static gaint drawn=0;                       /* something drawn since the last clear */
static gaint seq=0;                         /* number of pictures published */
static char tdir[512];                      /* where plot.png goes */
static gaint ownsdir=0;                     /* we created tdir, remove it at exit */
static char pngpath[600], tmppath[600], seqpath[600], seqtmp[600];
static gaint mode=0;                        /* 1=tmux 2=inline 3=file */
static char pane[64];                       /* tmux pane id of the viewer */

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

/* Write len bytes of base64 for buf to f */

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

/* Print the PNG into the terminal with the iTerm2 inline image protocol */

static void inlineshow (void) {
FILE *png,*tty;
unsigned char *buf;
long len;
char *w,*tmux;

  png = fopen(pngpath,"rb");
  if (png==NULL) return;
  fseek(png,0,SEEK_END);
  len = ftell(png);
  rewind(png);
  buf = (unsigned char *)malloc(len>0 ? len : 1);
  if (buf==NULL || fread(buf,1,len,png)!=(size_t)len) {
    if (buf) free(buf);
    fclose(png);
    return;
  }
  fclose(png);

  tty = fopen("/dev/tty","w");
  if (tty==NULL) tty = stdout;
  w = getenv("GA_TERM_WIDTH");
  if (w==NULL || *w=='\0') w = "70%";
  tmux = getenv("TMUX");
  fflush(stdout);
  if (tmux && *tmux) fputs("\033Ptmux;\033\033]",tty);
  else fputs("\033]",tty);
  fprintf(tty,"1337;File=inline=1;size=%ld;width=%s;preserveAspectRatio=1:",len,w);
  b64out(tty,buf,(size_t)len);
  if (tmux && *tmux) fputs("\a\033\\",tty);
  else fputs("\a",tty);
  fputc('\n',tty);
  fflush(tty);
  if (tty!=stdout) fclose(tty);
  free(buf);
}

/* Write the picture out if it changed, then let the viewer know */

void gxdidle (void) {
FILE *f;

  if (batch || surface==NULL || !dirty) return;
  dirty = 0;
  gxCflush(1);
  cairo_surface_flush(surface);
  if (cairo_surface_write_to_png(surface,tmppath)!=CAIRO_STATUS_SUCCESS) {
    printf("Terminal display: unable to write %s\n",tmppath);
    return;
  }
  if (rename(tmppath,pngpath)) {
    printf("Terminal display: unable to write %s\n",pngpath);
    return;
  }
  seq++;
  f = fopen(seqtmp,"w");
  if (f) {
    fprintf(f,"%d\n",seq);
    fclose(f);
    rename(seqtmp,seqpath);
  }
  if (mode==2 && drawn) inlineshow();   /* a cleared page is not worth printing */
}

/* Pick the output directory and the display mode */

static void termsetup (void) {
char *d,*m,*t,*v;
gaint rc;

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
  snprintf(pngpath,sizeof(pngpath),"%s/plot.png",tdir);
  snprintf(tmppath,sizeof(tmppath),"%s/.plot.png.tmp",tdir);
  snprintf(seqpath,sizeof(seqpath),"%s/seq",tdir);
  snprintf(seqtmp,sizeof(seqtmp),"%s/.seq.tmp",tdir);

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
    printf("Terminal display: pictures are written to %s\n",pngpath);
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

  surface = newsurface();
  gxCbgn(surface,xsz,ysz,dw,dh);
  gxCfrm();      /* a new image is transparent; an X window starts out painted */
  dirty = 1;
}

void gxdend (void) {
char *argv[8];

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
    unlink(pngpath);
    unlink(tmppath);
    unlink(seqpath);
    unlink(seqtmp);
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
  dirty = drawn = 1;
}

void gxdrec (gadouble x1, gadouble x2, gadouble y1, gadouble y2) {
  gxCrec(x1,x2,y1,y2);
  dirty = drawn = 1;
}

void gxddbl (void) {                         /* turn on double buffer mode */
  gxCfrm();                                  /* clear the foreground */
  if (surface2==NULL) surface2 = newsurface();
  gxCsfc(surface2);                          /* draw on the background from now on */
  gxCfrm();
  dblmode = 1;
  dirty = 1;
}

void gxdswp (void) {                         /* copy the background to the foreground */
  if (dblmode) {
    gxCswp(surface,surface2);
    gxCfrm();                                /* clear the background */
    dirty = drawn = 1;
  }
}

void gxdsgl (void) {                         /* turn off double buffer mode */
  if (dblmode) {
    gxCsfc(surface);                         /* draw on the foreground again */
    cairo_surface_destroy(surface2);
    surface2 = NULL;
  }
  dblmode = 0;
}

void gxdfil (gadouble *xy, gaint n) {
  gxCfil (xy,n);
  dirty = drawn = 1;
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
  cairo_surface_destroy(surface);
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
  dirty = drawn = 1;
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
  if (sig==5) { gxCpop(); dirty = drawn = 1; }  /* pop and paint */
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
