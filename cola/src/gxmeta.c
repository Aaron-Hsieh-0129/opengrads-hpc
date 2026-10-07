/* Copyright (C) 1988-2018 by George Mason University. See file COPYRIGHT for more information. */

/* Routines related to hardcopy (metafile) output. */

#ifdef HAVE_CONFIG_H
#include "config.h"

/* If autoconfed, only include malloc.h when it's present */
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#else /* undef HAVE_CONFIG_H */

#include <malloc.h>

#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "gatypes.h"
#include "gx.h"

void *galloc(size_t,char *);

/* Struct to form linked list for the meta buffer */
/* The buffer area is allocated as float, to insure at least four bytes
   per element.  In some cases, ints and chars will get stuffed into a 
   float (via pointer casting).  */

/* Don't use gafloat or gaint for meta buffer stuffing.  */

struct gxmbuf {
  struct gxmbuf *fpmbuf;         /* Forward pointer */
  float *buff;                   /* Buffer area */
  gaint len;                     /* Length of Buffer area */
  gaint used;                    /* Amount of buffer used */
};

/* Buffer chain anchor here; also a convenience pointer to the last buffer
   in the chain.  Times 2, for double buffering.  mbufanch and mbuflast 
   always point to the buffer currently being added to.  In double buffering
   mode, that will be the background buffer (and mbufanch2 points to the 
   buffer representing the currently display image). */

static struct gxmbuf *mbufanch=NULL;  /* Buffer Anchor */
static struct gxmbuf *mbuflast=NULL;  /* Last struct in chain */

static struct gxmbuf *mbufanch2=NULL;  /* Buffer Anchor */
static struct gxmbuf *mbuflast2=NULL;  /* Last struct in chain */

static gaint dbmode;                   /* double buffering flag */
static struct gxpsubs *psubs=NULL;     /* function pointers for printing */
static struct gxdsubs *dsubs=NULL;     /* function pointers for display */

#define BWORKSZ 250000

static gaint mbuferror = 0;    /* Indicate an error state; suspends buffering */

/* Undo support.  The meta buffer is a replayable record of everything drawn
   since the last frame action, so a position in it is all that is needed to
   put the picture back: rewind to the position and redraw.  A position is
   only meaningful within the frame it was taken in, so a reset of the
   buffer chain drops the saved steps and gives the chain a new generation,
   which also catches a position taken before a reset and offered after one.

   A clear is the exception.  When the clear command asks for it
   (gxhundoframe), the chain is not reset but set aside, and the step of the
   command that cleared keeps it: undoing that step puts the old chain back,
   generation and all, so the steps taken in it apply again.  A command that
   clears more than once (a script) sets aside the first frame only.

   Each step also carries the caller's state from before its command, which
   gxhundo hands back for the caller to put back; a step that is dropped
   instead hands it to the routine named with gxhundofn. */

struct gxhmark {
  struct gxmbuf *buf;          /* Buffer holding the end of the plot */
  gaint used;                  /* How much of that buffer was in use */
  gaint gen;                   /* Chain generation the position belongs to */
};

struct gxhstep {
  struct gxhmark mark;         /* Where to rewind to, for a step that drew */
  void *state;                 /* The caller's, from before the command */
  gaint frame;                 /* 1 for a clear: the frame before it follows */
  struct gxmbuf *anch,*last;   /* Its chain, NULL if it was empty */
  gaint gen;                   /* and that chain's generation */
};

#define UNDOSTEPMAX 10000      /* Sanity cap on the number of steps kept */

static gaint undolim = 0;            /* Steps to keep; 0 is off, the default */
static gaint undocnt = 0;            /* Steps currently available */
static gaint undogen = 0;            /* Generation of the current chain */
static gaint undoseq = 0;            /* Last generation handed out */
static struct gxhstep *undostk=NULL; /* undolim steps, oldest first */
static struct gxhmark undopend;      /* Position before the running command */
static gaint undopndflg = 0;         /* 1 when undopend holds a position */
static gaint undoarm = 0;            /* The coming clear sets the frame aside */
static struct gxhstep undoclr;       /* A frame the running command set aside */
static gaint undoclrflg = 0;         /* 1 when undoclr holds one */
static void (*undofree) (void *) = NULL;   /* Frees the caller's state */

/* Initialize any buffering, etc. when GrADS starts up */

void gxhnew (gadouble xsiz, gadouble ysiz, gaint hbufsz) {
gaint rc;
  mbufanch = NULL;
  mbuflast = NULL;
  mbuferror = 0;
  if (sizeof(int) > sizeof(float)) {
    printf ("Error in gx initialization: Incompatable int and float sizes\n");
    mbuferror = 99;
    return;
  }
  rc = mbufget();
  if (rc) {
    printf ("Error in gx initialization: Unable to allocate meta buffer\n");
    mbuferror = 99;
  }
}


/* Add command with 0 args to metafile buffer */

void hout0 (gaint cmd) {
gaint rc;
signed char *ch;
  if (mbuferror) return;
  if (mbuflast->len - mbuflast->used <3) {
    rc = mbufget();
    if (rc) {
      gxmbuferr();
      return;
    }
  }
  ch = (signed char *)(mbuflast->buff+mbuflast->used);
  *ch = (signed char)99;
  *(ch+1) = (signed char)cmd;
  mbuflast->used++;
}

/* Add a command with one small integer argument to the metafile buffer.
   The argument is assumed to fit into a signed char (-127 to 128).  */

void hout1c (gaint cmd, gaint opt) {
gaint rc;
signed char *ch;
  if (mbuferror) return;
  if (mbuflast->len - mbuflast->used <4) {
    rc = mbufget();
    if (rc) {
      gxmbuferr();
      return;
    }
  }
  ch = (signed char *)(mbuflast->buff+mbuflast->used);
  *ch = (signed char)99;
  *(ch+1) = (signed char)cmd;
  mbuflast->used++;
  *(ch+2) = (signed char)opt;
  mbuflast->used++;
}

/* Add command with one integer argument to metafile buffer */

void hout1 (gaint cmd, gaint opt) {
gaint rc;
signed char *ch;
int *iii;
  if (mbuferror) return;
  if (mbuflast->len - mbuflast->used <4) {
    rc = mbufget();
    if (rc) {
      gxmbuferr();
      return;
    }
  }
  ch = (signed char *)(mbuflast->buff+mbuflast->used);
  *ch = (signed char)99;
  *(ch+1) = (signed char)cmd;
  mbuflast->used++;
  iii = (int *)(mbuflast->buff+mbuflast->used);
  *iii = (int)opt;
  mbuflast->used++;
}

/* Metafile buffer, command plus two double args */

void hout2 (gaint cmd, gadouble x, gadouble y) {
gaint rc;
signed char *ch;
  if (mbuferror) return;
  if (mbuflast->len - mbuflast->used <5) {
    rc = mbufget();
    if (rc) {
      gxmbuferr();
      return;
    }
  }
  ch = (signed char *)(mbuflast->buff+mbuflast->used);
  *ch = (signed char)99;
  *(ch+1) = (signed char)cmd;
  mbuflast->used++;
  *(mbuflast->buff+mbuflast->used) = (float)x;
  mbuflast->used++;
  *(mbuflast->buff+mbuflast->used) = (float)y;
  mbuflast->used++;
}

/* Metafile buffer, command plus two integer args */

void hout2i (gaint cmd, gaint i1, gaint i2) {
gaint rc;
signed char *ch;
int *iii;
  if (mbuferror) return;
  if (mbuflast->len - mbuflast->used <5) {
    rc = mbufget();
    if (rc) {
      gxmbuferr();
      return;
    }
  }
  ch = (signed char *)(mbuflast->buff+mbuflast->used);
  *ch = (signed char)99;
  *(ch+1) = (signed char)cmd;
  mbuflast->used++;
  iii = (int *)(mbuflast->buff+mbuflast->used);
  *iii = (int)i1;
  mbuflast->used++;
  iii = (int *)(mbuflast->buff+mbuflast->used);
  *iii = (int)i2;
  mbuflast->used++;
}

/* Metafile buffer, command plus three integer args */

void hout3i (gaint cmd, gaint i1, gaint i2, gaint i3) {
gaint rc;
signed char *ch;
int *iii;
  if (mbuferror) return;
  if (mbuflast->len - mbuflast->used <6) {
    rc = mbufget();
    if (rc) {
      gxmbuferr();
      return;
    }
  }
  ch = (signed char *)(mbuflast->buff+mbuflast->used);
  *ch = (signed char)99;
  *(ch+1) = (signed char)cmd;
  mbuflast->used++;
  iii = (int *)(mbuflast->buff+mbuflast->used);
  *iii = (int)i1;
  mbuflast->used++;
  iii = (int *)(mbuflast->buff+mbuflast->used);
  *iii = (int)i2;
  mbuflast->used++;
  iii = (int *)(mbuflast->buff+mbuflast->used);
  *iii = (int)i3;
  mbuflast->used++;
}

/* Metafile buffer, command plus four integer args */

void hout5i (gaint cmd, gaint i1, gaint i2, gaint i3, gaint i4, gaint i5) {
gaint rc;
signed char *ch;
int *iii;
  if (mbuferror) return;
  if (mbuflast->len - mbuflast->used <8) {
    rc = mbufget();
    if (rc) {
      gxmbuferr();
      return;
    }
  }
  ch = (signed char *)(mbuflast->buff+mbuflast->used);
  *ch = (signed char)99;
  *(ch+1) = (signed char)cmd;
  mbuflast->used++;
  iii = (int *)(mbuflast->buff+mbuflast->used);
  *iii = (int)i1;
  mbuflast->used++;
  iii = (int *)(mbuflast->buff+mbuflast->used);
  *iii = (int)i2;
  mbuflast->used++;
  iii = (int *)(mbuflast->buff+mbuflast->used);
  *iii = (int)i3;
  mbuflast->used++;
  iii = (int *)(mbuflast->buff+mbuflast->used);
  *iii = (int)i4;
  mbuflast->used++;
  iii = (int *)(mbuflast->buff+mbuflast->used);
  *iii = (int)i5;
  mbuflast->used++;
}

/* Metafile buffer, command plus four double args */

void hout4 (gaint cmd, gadouble xl, gadouble xh, gadouble yl, gadouble yh) {
gaint rc;
signed char *ch;
  if (mbuferror) return;
  if (mbuflast->len - mbuflast->used <7) {
    rc = mbufget();
    if (rc) {
      gxmbuferr();
      return;
    }
  }
  ch = (signed char *)(mbuflast->buff+mbuflast->used);
  *ch = (signed char)99;
  *(ch+1) = (signed char)cmd;
  mbuflast->used++;
  *(mbuflast->buff+mbuflast->used) = (float)xl;
  mbuflast->used++;
  *(mbuflast->buff+mbuflast->used) = (float)xh;
  mbuflast->used++;
  *(mbuflast->buff+mbuflast->used) = (float)yl;
  mbuflast->used++;
  *(mbuflast->buff+mbuflast->used) = (float)yh;
  mbuflast->used++;
}

/* Add a single character to the metafile buffer, along with the font number (less
   than 100), location (x,y), and size/rotation specs (4 floats).  Uses -21 as a
   cmd value.   */

void houtch (char ch, gaint fn, gadouble x, gadouble y,
         gadouble w, gadouble h, gadouble ang) {
gaint rc;
signed char *ccc;
char *ucc;
  if (mbuferror) return;
  if (mbuflast->len - mbuflast->used <8) {
    rc = mbufget();
    if (rc) {
      gxmbuferr();
      return;
    }
  }
  ccc = (signed char *)(mbuflast->buff+mbuflast->used);
  ucc = (char *)(ccc+2);
  *ccc = (signed char)99;
  *(ccc+1) = (signed char)(-21);
  *ucc = ch; 
  *(ccc+3) = (signed char)fn;
  mbuflast->used++;
  *(mbuflast->buff+mbuflast->used) = (float)x;
  mbuflast->used++;
  *(mbuflast->buff+mbuflast->used) = (float)y;
  mbuflast->used++;
  *(mbuflast->buff+mbuflast->used) = (float)w;
  mbuflast->used++;
  *(mbuflast->buff+mbuflast->used) = (float)h;
  mbuflast->used++;
  *(mbuflast->buff+mbuflast->used) = (float)ang;
  mbuflast->used++;
}

/* Free a chain of buffers that a clear set aside */

static void mbufchfree (struct gxmbuf *pmbuf) {
struct gxmbuf *next;
  while (pmbuf) {
    next = pmbuf->fpmbuf;
    if (pmbuf->buff) gree (pmbuf->buff,"gxmbuf");
    gree (pmbuf,"mbufbuff");
    pmbuf = next;
  }
}

/* Let a step go: hand its state to the caller's routine, and free the frame
   it set aside. */

static void gxhundodrop (struct gxhstep *step) {
  if (step->state && undofree) undofree (step->state);
  step->state = NULL;
  if (step->frame) mbufchfree (step->anch);
  step->anch = NULL;
  step->last = NULL;
}

/* Name the routine that frees the state a step carries */

void gxhundofn (void (*fn) (void *)) {
  undofree = fn;
}

/* Is a saved position still usable?  It has to belong to the current
   generation, and it must sit at or before the end of the chain: a position
   past the end would make a redraw replay buffer contents that have already
   been handed back. */

static gaint gxhundookay (struct gxhmark *mark) {
struct gxmbuf *pmbuf;

  if (mark->gen!=undogen) return (0);
  if (mark->buf==NULL) return (1);         /* Start of the chain */
  pmbuf = mbufanch;
  while (pmbuf) {
    if (pmbuf==mark->buf) return (mark->used<=pmbuf->used);
    if (pmbuf==mbuflast) return (0);       /* Position is past the end */
    pmbuf = pmbuf->fpmbuf;
  }
  return (0);
}

/* Forget every saved step.  Called whenever the buffer chain is reset other
   than by a clear that sets the frame aside, which is what makes the saved
   positions meaningless. */

void gxhundoclr (void) {
gaint i;
  for (i=0; i<undocnt; i++) gxhundodrop (undostk+i);
  undocnt = 0;
  undopndflg = 0;
  undoarm = 0;
  if (undoclrflg) gxhundodrop (&undoclr);
  undoclrflg = 0;
  undogen = ++undoseq;
}

/* Set how many undo steps to keep.  A count below one turns undo off.
   Returns 1 if the stack could not be allocated, otherwise 0. */

gaint gxhundoset (gaint steps) {
  gxhundoclr();
  if (undostk) {
    gree (undostk,"undostk");
    undostk = NULL;
  }
  undolim = 0;
  if (steps<1) return (0);
  if (steps>UNDOSTEPMAX) steps = UNDOSTEPMAX;
  undostk = (struct gxhstep *)galloc(sizeof(struct gxhstep)*steps,"undostk");
  if (undostk==NULL) return (1);
  undolim = steps;
  return (0);
}

/* Note where the plot ends, before running a command that might add to it. */

void gxhundomark (void) {
  undopndflg = 0;
  undoarm = 0;
  if (undoclrflg) gxhundodrop (&undoclr);
  undoclrflg = 0;
  if (undolim<1 || mbuferror) return;
  undopend.buf = mbuflast;
  undopend.used = mbuflast ? mbuflast->used : 0;
  undopend.gen = undogen;
  undopndflg = 1;
}

/* The clear about to happen may be undone: have it set the frame aside
   instead of resetting the chain.  Only within a command whose position was
   noted, and not in double buffering, where frames come and go. */

void gxhundoframe (void) {
  undoarm = (undopndflg && undolim>0 && !mbuferror && !dbmode);
}

/* A clear set up by gxhundoframe: keep the chain as the frame before the
   clear, and start a new one from the buffers past its end.  A clear of an
   empty frame changes nothing to keep, and a second clear in the same
   command only resets the new chain: the frame from before the command is
   the one to go back to. */

static void gxhundoaside (void) {
struct gxmbuf *spare;

  undoarm = 0;
  if (mbufanch==NULL) return;
  if (undoclrflg || (mbuflast==mbufanch && mbufanch->used==0)) {
    mbufanch->used = 0;
    mbuflast = mbufanch;
    if (undoclrflg) undogen = ++undoseq;
    return;
  }
  spare = mbuflast->fpmbuf;
  mbuflast->fpmbuf = NULL;
  undoclr.frame = 1;
  undoclr.state = NULL;
  undoclr.anch = mbufanch;
  undoclr.last = mbuflast;
  undoclr.gen = undogen;
  undoclrflg = 1;
  mbufanch = spare;
  mbuflast = spare;
  undogen = ++undoseq;
  if (spare) spare->used = 0;
  else if (mbufget()) gxmbuferr();
}

/* Keep a step for the command that just finished, with the caller's state
   from before it: the frame a clear in it set aside, or else the position
   noted by gxhundomark, if the command added to the plot and nothing reset
   the chain while it ran.  Returns 1 when the step was kept, state with it;
   0 when there is no step, and state is still the caller's. */

gaint gxhundokeep (void *state) {
struct gxhstep step;
struct gxmbuf *end;
gaint used, i, pnd;

  pnd = undopndflg;
  undopndflg = 0;
  undoarm = 0;
  if (undoclrflg) {                         /* The command cleared the frame */
    step = undoclr;
    undoclrflg = 0;
    if (!pnd || undolim<1 || mbuferror) {
      gxhundodrop (&step);
      return (0);
    }
  } else {
    if (!pnd || undolim<1 || mbuferror) return (0);
    if (!gxhundookay(&undopend)) return (0);
    end = mbuflast;
    used = end ? end->used : 0;
    if (end==undopend.buf && used==undopend.used) return (0);   /* Nothing drawn */
    step.mark = undopend;
    step.frame = 0;
    step.anch = NULL;
    step.last = NULL;
    step.gen = undopend.gen;
  }
  step.state = state;
  if (undocnt==undolim) {                   /* Full, so drop the oldest */
    gxhundodrop (undostk);
    for (i=0; i<undolim-1; i++) undostk[i] = undostk[i+1];
    undocnt--;
  }
  undostk[undocnt] = step;
  undocnt++;
  return (1);
}

/* Rewind the plot one step.  For a step that drew, hand the buffers filled
   since its position back to the pool: they stay allocated, and mbufget
   resets them when it reuses them.  For a clear, put the frame from before
   it back; the chain drawn since becomes its spare buffers.  state, if not
   NULL, receives the step's state, which is then the caller's.  Returns 0
   when the buffer was rewound and 1 when there was nothing to undo. */

gaint gxhundo (void **state) {
struct gxhstep step;

  if (state) *state = NULL;
  if (undolim<1 || undocnt<1) return (1);
  undocnt--;
  step = undostk[undocnt];
  if (step.frame) {
    if (step.anch) {
      step.last->fpmbuf = mbufanch;
      mbufanch = step.anch;
      mbuflast = step.last;
    } else if (mbufanch) {
      mbufanch->used = 0;
      mbuflast = mbufanch;
    }
    undogen = step.gen;
  } else {
    if (!gxhundookay(&step.mark)) {
      if (step.state && undofree) undofree (step.state);
      gxhundoclr();
      return (1);
    }
    if (step.mark.buf==NULL) {              /* Back to an empty plot */
      if (mbufanch) mbufanch->used = 0;
      mbuflast = mbufanch;
    } else {
      step.mark.buf->used = step.mark.used;
      mbuflast = step.mark.buf;
    }
  }
  if (state) *state = step.state;
  else if (step.state && undofree) undofree (step.state);
  return (0);
}

/* Report the undo settings.  Any pointer may be NULL.  words is how much of
   the meta buffer the current plot occupies. */

void gxhundoq (gaint *limit, gaint *depth, gaint *words) {
struct gxmbuf *pmbuf;
gaint total;

  if (limit) *limit = undolim;
  if (depth) *depth = undocnt;
  if (words) {
    total = 0;
    pmbuf = mbufanch;
    while (pmbuf) {
      total += pmbuf->used;
      if (pmbuf==mbuflast) break;
      pmbuf = pmbuf->fpmbuf;
    }
    *words = total;
  }
}

/* User has issued a clear.  
   This may also indicate the start or end of double buffering.  
   If we are not double buffering, just free up the memory buffer and return.  
   If we are starting up double buffering, we need another buffer chain.  
   If we are ending double buffering, all memory needs to be released.  
   If we are in the midst of double buffering, do a "swap" and free the foreground buffer. 

   Values for action are:
      0 -- new frame (clear display), wait before clearing.
      1 -- new frame, no wait.
      2 -- New frame in double buffer mode.  If not supported
           has same result as action=1.  Usage involves multiple
           calls with action=2 to obtain an animation effect.  
      7 -- new frame, but just clear graphics.  Do not clear  
           event queue; redraw buttons. 
      8 -- clear only the event queue.
      9 -- clear only the X request buffer
*/ 

void gxhfrm (gaint iact) {
struct gxmbuf *pmbuf, *pmbufl;

  /* Start up double buffering */
  if (iact==2 && dbmode==0) { 
    mbufrel(1); 
    if (mbufanch==NULL) mbufget();
    mbufanch2 = mbufanch;
    mbuflast2 = mbuflast;
    mbufanch = NULL;
    mbuflast = NULL;
    mbufget();
    dbmode = 1; 
  }

  /* End of double buffering */
  if (iact!=2 && dbmode==1) {
    mbufrel(0);
    mbufanch = mbufanch2;
    mbufrel(1);
    dbmode = 0;
    mbuferror = 0;
    return;
  }

  /* If double buffering, swap buffers */
  if (dbmode) {
    pmbuf = mbufanch;     /* Save pointer to background buffer */
    pmbufl = mbuflast;
    mbufanch = mbufanch2;
    mbufrel(1);           /* Get rid of former foreground buffer */
    mbufanch2 = pmbuf;    /* Set foreground to former background */
    mbuflast2 = pmbufl; 
  } 
  else {
    /* Not double buffering: set the frame aside for undo, or free buffers */
    if (undoarm) gxhundoaside();
    else mbufrel(1);
  }
  if (!dbmode) mbuferror = 0;        /* Reset error state on clear command */
}


/* Redraw based on contents of current buffers.  Items that persist from plot
   to plot ARE NOT IN THE META BUFFER; these items are set in the hardware attribute
   database and are queried by the backend. 

   This routine is called from gxX (ie, a lower level of the backend rendering), 
   and this routine calls back into gxX.  This is not, however, implemented as
   true recursion -- events are disabled in gxX during this redraw, so addtional
   levels of recursion are not allowed.  

   If dbflg, draw from the background buffer.  Otherwise draw from the 
   foreground buffer. */

void gxhdrw (gaint dbflg, gaint pflg) {
struct gxmbuf *pmbuf;
float *buff;
int *iii;
gadouble r,s,x,y,w,h,ang;
gadouble *xybuf;
gaint ppp,cmd,op1,op2,op3,op4,op5,fflag,xyc=0,fn,sig;
signed char *ch;
char ccc,*uch;

  if (dbflg && !dbmode) {
    printf ("Logic error 0 in Redraw.  Contact Developer.\n");
    return;
  }

  if (psubs==NULL) psubs = getpsubs();  /* get ptrs to the graphics printing functions */
  if (dsubs==NULL) dsubs = getdsubs();  /* get ptrs to the graphics display functions */
 
  if (dbflg) pmbuf = mbufanch2;
  else pmbuf = mbufanch; 

  fflag = 0;
  xybuf = NULL;

  while (pmbuf) {
    ppp = 0;
    while (ppp < pmbuf->used) {

      /* Get message type */
 
      ch = (signed char *)(pmbuf->buff + ppp);
      cmd = (gaint)(*ch);
      if (cmd != 99) {
        printf ("Metafile buffer is corrupted\n");
        printf ("Unable to complete redraw and/or print operation\n");
        return;
      }
      cmd = (gaint)(*(ch+1));
      ppp++;


      /* Handle various message types */
      /* -9 is end of file.  Should not happen. */

      if (cmd==-9) {
        printf ("Logic Error 4 in Redraw.  Notify Developer\n");
        return;
      }

      /*  -1 indicates start of file.  Should not occur. */

      else if (cmd==-1) {
        printf ("Logic Error 8 in Redraw.  Notify Developer\n");
        return;
      }
  
      /* -2 indicates new frame.  Also should not occur */

      else if (cmd==-2) {
        printf ("Logic Error 12 in Redraw.  Notify Developer\n");
        return;
      }

      /* -3 indicates new color.  One arg; color number.  */
  
      else if (cmd==-3) {
        iii = (int *)(pmbuf->buff + ppp);
        op1 = (gaint)(*iii);
	if (pflg) 
	  psubs->gxpcol (op1);          /* for printing */
	else 
	  dsubs->gxdcol (op1);          /* for hardware */
        ppp++;
      }

      /* -4 indicates new line thickness.  It has two arguments */
 
      else if (cmd==-4) {
        iii = (int *)(pmbuf->buff + ppp);
        op1 = (gaint)(*iii);
	if (pflg)
	  psubs->gxpwid (op1);          /* for printing */
	else
	  dsubs->gxdwid (op1);          /* for hardware */
        ppp += 2;
      }

      /*  -5 defines a new color, in rgb.  It has five int args */

      else if (cmd==-5){
        iii = (int *)(pmbuf->buff + ppp);
        op1 = (gaint)(*iii);
        iii = (int *)(pmbuf->buff + ppp + 1);
        op2 = (gaint)(*iii);
        iii = (int *)(pmbuf->buff + ppp + 2);
        op3 = (gaint)(*iii);
        iii = (int *)(pmbuf->buff + ppp + 3);
        op4 = (gaint)(*iii);
        iii = (int *)(pmbuf->buff + ppp + 4);
        op5 = (gaint)(*iii);
        gxdbacol (op1,op2,op3,op4,op5);   /* update the data base */
	if (pflg) 
	  psubs->gxpacol (op1);                 /* for printing (no-op for cairo) */
	else 
	  dsubs->gxdacol (op1,op2,op3,op4,op5); /* for hardware (no-op for cairo) */
        ppp += 5;
      }

      /* -6 is for a filled rectangle.  It has four args. */ 
 
      else if (cmd==-6){
        buff = pmbuf->buff + ppp;
        r = (gadouble)(*buff);
        s = (gadouble)(*(buff+1));
        x = (gadouble)(*(buff+2));
        y = (gadouble)(*(buff+3));
	if (pflg) 
	  psubs->gxprec(r,s,x,y);          /* for printing */
	else
	  dsubs->gxdrec(r,s,x,y);          /* for hardware */
        ppp += 4;
      }

      /* -7 indicates the start of a polygon fill.  It has one arg, 
         the length of the polygon.  We allocate an array for the entire
         polygon, so we can present it to the hardware backend in 
         on piece. */

      else if (cmd==-7) {
        iii = (int *)(pmbuf->buff + ppp);
        op1 = (gaint)(*iii);
        xybuf = (gadouble *)galloc(sizeof(gadouble)*op1*2,"gxybuf");
        if (xybuf==NULL) {
          printf ("Memory allocation error: Redraw\n");
          return;
        }
        xyc = 0;
        fflag = 1;
        ppp += 1;
	/* tell printing layer about new polygon. */
	if (pflg) psubs->gxpbpoly();  
      }

      /* -8 is to terminate polygon fill.  It has no args */

      else if (cmd==-8) {
        if (xybuf==NULL) {
          printf ("Logic Error 16 in Redraw.  Notify Developer\n");
          return;
        }
	if (pflg) 
	  psubs->gxpepoly (xybuf,xyc);  /* for printing */
	else
	  dsubs->gxdfil (xybuf,xyc);    /* for hardware */
        gree (xybuf,"gxybuf");
        xybuf = NULL;
        fflag = 0;
      }

      /* -10 is a move to instruction.  It has two double args */ 

      else if (cmd==-10) {
        buff = pmbuf->buff + ppp;
        x = (gadouble)(*buff);
        y = (gadouble)(*(buff+1));
	if (fflag) {
	  xybuf[xyc*2] = x;
	  xybuf[xyc*2+1] = y;
	  xyc++;
	}
	if (pflg) 
	  psubs->gxpmov(x,y);            /* for printing */
	else         
	  dsubs->gxdmov(x,y);            /* for hardware */
        ppp += 2;
      }

      /*  -11 is draw to.  It has two double args. */  
        
      else if (cmd==-11) {
        buff = pmbuf->buff + ppp;
        x = (gadouble)(*buff);
        y = (gadouble)(*(buff+1));
	if (fflag) {
	  xybuf[xyc*2] = x;
	  xybuf[xyc*2+1] = y;
	  xyc++;
	}
	if (pflg) 
	  psubs->gxpdrw(x,y);            /* for printing */
	else 
	  dsubs->gxddrw(x,y);            /* for hardware */
        ppp += 2;
      }
      
      /* -12 indicates new fill pattern.  It has three arguments. */
 
      else if (cmd==-12) {
	/* This is a no-op for cairo; X-based pattern drawing */
        buff = pmbuf->buff + ppp;
	dsubs->gxdptn ((gaint)*(buff+0),(gaint)*(buff+1),(gaint)*(buff+2));
	if (pflg) 
	  psubs->gxpflush(); 
        ppp += 3;
      }

      /* -20 is a draw widget.  We will redraw it in current state. */

      else if (cmd==-20) {
	/* This is a no-op for cairo; X-based buttonwidget drawing */
        buff = pmbuf->buff + ppp;
	dsubs->gxdpbn ((gaint)*(buff+0),NULL,1,0,-1);
	if (pflg) 
	  psubs->gxpflush(); 
        ppp += 1;
      }

      /* -21 is for drawing a single character in the indicated font and size */

      else if (cmd==-21) {
        ch = (signed char *)(pmbuf->buff + ppp - 1);
        fn = (gaint)(*(ch+3));
        uch = (char *)(pmbuf->buff + ppp - 1);
        ccc = *(uch+2);
        buff = pmbuf->buff + ppp;
        x = (gadouble)(*buff);
        y = (gadouble)(*(buff+1));
        w = (gadouble)(*(buff+2));
        h = (gadouble)(*(buff+3));
        ang = (gadouble)(*(buff+4));
	if (pflg) 
	  r = psubs->gxpch (ccc,fn,x,y,w,h,ang);     /* print a character */
	else 
	  r = dsubs->gxdch (ccc,fn,x,y,w,h,ang);     /* draw a character */
        ppp += 5;
      }

      /* -22 is for a signal. It has one signed character argument */

      else if (cmd==-22) {
	ch = (signed char *)(pmbuf->buff + ppp - 1);
	sig = (gaint)(*(ch+2));
	if (pflg) 
	  psubs->gxpsignal(sig); 
	else 
	  dsubs->gxdsignal(sig);
	ppp++;
      }

      /* -23 is for the clipping area. It has four args. */ 
 
      else if (cmd==-23){
        buff = pmbuf->buff + ppp;
        r = (gadouble)(*buff);
        s = (gadouble)(*(buff+1));
        x = (gadouble)(*(buff+2));
        y = (gadouble)(*(buff+3));
	if (pflg) 
	  psubs->gxpclip(r,s,x,y);          /* for printing */
	else
	  dsubs->gxdclip(r,s,x,y);          /* for hardware */
        ppp += 4;
      }

      /* Any other command would be invalid */

      else {
         printf ("Logic Error 20 in Redraw.  Notify Developer\n");
        return;
      }
    } 
    if (pmbuf == mbuflast) break;
    pmbuf = pmbuf->fpmbuf;
  }
  /* tell hardware and printing layer we are finished */
  if (pflg) psubs->gxpflush();  
  dsubs->gxdopt(4);
}


/* Allocate and chain another buffer area */

gaint mbufget (void) {
struct gxmbuf *pmbuf;

  if (mbufanch==NULL) {
    pmbuf = (struct gxmbuf *)galloc(sizeof(struct gxmbuf),"mbufanch");  
    if (pmbuf==NULL) return (1);
    mbufanch = pmbuf;                  /* set the new buffer structure as the anchor */
    mbuflast = pmbuf;                  /* ... and also as the last one */
    pmbuf->buff = (float *)galloc(sizeof(float)*BWORKSZ,"anchbuff");  /* allocate a buffer */
    if (pmbuf->buff==NULL) return(1);
    pmbuf->len = BWORKSZ;              /* set the buffer length */
    pmbuf->used = 0;                   /* initialize the buffer as unused */
    pmbuf->fpmbuf = NULL;              /* terminate the chain */
  }
  else {
    if (mbuflast->fpmbuf==NULL) {      /* no more buffers in the chain */
      pmbuf = (struct gxmbuf *)galloc(sizeof(struct gxmbuf),"mbufnew");  
      if (pmbuf==NULL) return (1);
      mbuflast->fpmbuf = pmbuf;        /* add the new buffer structure to the chain */
      mbuflast = pmbuf;                /* reset mbuflast to the newest buffer structure in the chain */
      pmbuf->buff = (float *)galloc(sizeof(float)*BWORKSZ,"newbuff");  /* allocate a buffer */
      if (pmbuf->buff==NULL) return(1);
      pmbuf->len = BWORKSZ;            /* set the buffer length */
      pmbuf->used = 0;                 /* initialize the buffer as unused */
      pmbuf->fpmbuf = NULL;            /* terminate the chain */
    }
    else {                             /* we'll just re-use what's already been chained up */
      pmbuf = mbuflast->fpmbuf;        /* get the next buffer in the chain */
      pmbuf->used = 0;                 /* reset this buffer to unused */
      mbuflast = pmbuf;                /* set mbuflast to point to this buffer */
    }
  }
  return (0);
}

/* Free buffer chain.  
   If flag is 1, leave allocated buffers alone and mark the anchor as unused 
   If flag is 0, free all buffers, including the anchor
*/

void mbufrel (gaint flag) {
struct gxmbuf *pmbuf,*pmbuf2;
gaint i;

  gxhundoclr();          /* Saved undo positions do not survive a reset */
  i = flag;
  pmbuf = mbufanch;                /* point at the anchor */
  while (pmbuf) {
    pmbuf2 = pmbuf->fpmbuf;        /* get next link in chain */
    if (!i) {                      /* this part only gets executed when flag is 0 */
      if (pmbuf->buff) gree (pmbuf->buff,"gxmbuf");
      gree (pmbuf,"mbufbuff");     /* free the pmbuf link */
      i = 0; 
    }
    pmbuf = pmbuf2;                /* move up the chain */
  }
  if (!flag) {
    mbufanch = NULL;               /* no more metabuffer */
  } 
  else {
    if (mbufanch) mbufanch->used = 0;
  }
  mbuflast = mbufanch;
}

void gxmbuferr() {
  printf ("Error in gxmeta: Unable to allocate meta buffer\n");
  printf ("                 Buffering for the current plot is disabled\n");
  mbuferror = 1;
  mbufrel(0);
}

