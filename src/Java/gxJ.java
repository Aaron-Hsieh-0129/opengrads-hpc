/* -*- mode: java; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
  gxJ is the class that implements low level interface to
  Java.

  Device interface level.  Following routines need to be
  interfaced to the Java environment.

*/

import java.io.*;
import java.awt.*;
import java.awt.event.*;

class gxJ extends Canvas
{

  // Constants for JGrADS interface

  // Marker Type
  static final int GX_BOX= 1 ;
  static final int GX_OPEN_CIRCLE = 2 ;
  static final int GX_FILLED_CIRCLE = 3;
  static final int GX_CROSSHAIR = 4 ;
  static final int GX_FILLED_SQUARE = 5 ;
  static final int GX_XMARKS = 6 ;
  static final int GX_OPEN_DIAMOND = 7 ;
  static final int GX_OPEN_TRIANGLE = 8 ;
  static final int GX_SCATTER_FILL = 9 ;
  static final int GX_BROKEN_FILL = 10 ;

  // Line Style
  static final int GX_LINE_SOLID = 30  ;
  static final int GX_LINE_LONGDASH = 31  ;
  static final int GX_LINE_SHORTDASH = 32  ;
  static final int GX_LINE_LONG_SHORTDASH = 33  ;
  static final int GX_LINE_DOTTED = 34  ;
  static final int GX_LINE_DOT_DASH = 35  ;
  static final int GX_LINE_DOT_DOT_DASH = 36  ;

  // Predefined Color Number
  static final int GX_COLOR_BLACK = 0 ;
  static final int GX_COLOR_WHITE = 1 ;
  static final int GX_COLOR_RED   = 2 ;
  static final int GX_COLOR_GREEN = 3 ;
  static final int GX_COLOR_BLUE  = 4 ;
  static final int GX_COLOR_CYAN  = 5 ;
  static final int GX_COLOR_MAGENTA = 6 ;
  static final int GX_COLOR_YELLOW  = 7 ;
  static final int GX_COLOR_ORANGE  = 8 ;
  static final int GX_COLOR_PURPLE  = 9 ;
  static final int GX_COLOR_LIGHTBLUE = 10 ;
  static final int GX_COLOR_PINK = 11 ;
  static final int GX_COLOR_GREY = 12 ;

  // Font Size
  static final int GX_SMALL_FONT = 0 ;
  static final int GX_MEDIUM_FONT = 1 ;
  static final int GX_LARGE_FONT   = 2 ;

  // Vpage placement
  static final int GX_NORTHEAST = 1;
  static final int GX_NORTHWEST = 2;
  static final int GX_SOUTHEAST = 3;
  static final int GX_SOUTHWEST = 4;
  static final int GX_FILLFRAME = 5;

  // Data I/O
  static final int IEEE_FORTRAN_SEQUENTIAL_IO=100;
  static final int IEEE_FORTRAN_DIRECT_ACCESS_IO=101;

  // Double or single buffer
  static final int GX_DOUBLE_BUFFER = 200;
  static final int GX_SINGLE_BUFFER = 201;

  private boolean initflg=false; // Initialization flag

  private int wchose=1;  // Controls technique for wide lines
  private int lcolor,lwidth,owidth; // Current attributes
  private int grflg=0;              // Greyscale flag
  private int devbck;               // Device background color
  private double xscl,yscl;         // Window Scaling
  private int xxx,yyy;              // Old position
  private double xsize,ysize;       // User specified size
  private Graphics gc;              // Java Graphics context
  private Color[] cvals=new Color[120];          // Color pixel values
  private int[] cused=new int[120];

  // Graphics window attributes
  int width,height,depth;

  // Double buffer
  boolean double_buffer;
  private Image backimage;
  private Graphics backplane, foreplane;
  private int backplane_width, backplane_height;


  // Font in use by jgrads
  int gfont, cfont ;
  // fontp: plain, fontb: bold, fonti: italic
  Font[] fontp, fontb, fonti ;



  // GrADS default color map color schemes
  private int[] reds={  0,255,250, 0, 30,  0,240,230,
                      240,160,160, 0,230,  0,130,170};
  private int[] greens={  0,255, 60,220, 60,200,  0,220,
                        130,  0,230,160,175,210,  0,170};
  private int[] blues={  0,255, 60,  0,255,200,130, 50,
                        40,200, 50,255, 45,140,220,130};
  private int[] greys={  0,255,215,140,80,110,230,170,
                       200,50,155,95,185,125,65,177};
  private int[] grrev={  0, 1,14, 3, 8, 7, 9, 5, 4, 6,
                        13,12,11,10, 2,15};

  // Debug print
  private final static DebugInfo   dbg=new DebugInfo(DebugInfo.DBG_ALL);

  // Contructor, for now does nothing.
  gxJ()
  {
    // dbg.show_systime(true);
    dbg.show_thread_info(true);
    // System.out.println("constructor gxJ called");
    double_buffer=false;
  }

  Graphics getGraphicContext() { return gc; }

  void setCanvasSize()
  {
    // Get the drawing canvas size in pixel units
    Dimension d=getSize();
    int dw=d.width;
    int dh=d.height;
    xscl=dw/xsize;
    yscl=dh/ysize;
    width=dw;
    height=dh;

    // If the visible window has change size, than create another
    // double buffer back plane.
    if (double_buffer) {
       initDoubleBuffer();
       this.gc=this.backplane;
    }
    System.out.println("Window Inset size: "+dw+" x "+dh+" pixels");
    // dbg.INFO("Window Inset size: "+dw+" x "+dh+" pixels");
    return;
  }

  void setDrawMode(int mode)
  {
     if (mode==GX_DOUBLE_BUFFER)
     {
        initDoubleBuffer();
        this.gc=this.backplane;
     }
     if (mode==GX_SINGLE_BUFFER)
     {
        this.gc=this.foreplane;
        double_buffer=false;
     }
     return;
  }

  void initDoubleBuffer()
  {
     backimage=createImage(width,height);
     backplane=backimage.getGraphics();
     backplane_width=width; backplane_height=height;
     //  Clear the screen
     backplane.setColor(cvals[devbck]);
     backplane.fillRect(0,0,backplane_width,backplane_height);
     double_buffer=true;
     return;
  }

  /**
   * Copy the backplane image to the foreground.

  */
  void swapBuffers()
  {
     if (double_buffer) {
        foreplane.drawImage(backimage,0,0,this);
        /* Clear the backplane */
        backplane.setColor(cvals[devbck]);
        backplane.fillRect(0,0,backplane_width,backplane_height);
     }
  }

  void gxdbgn(double xsz, double ysz)
  {

    if (initflg) return;
    // Set user request size.
    xsize=xsz;
    ysize=ysz;

    // System.out.println("gxdbgn initialization called");
    // Get the Graphics context
    setVisible(true);
    //if ( isVisible())
    while ( ! isShowing() ) {
      System.out.println("WARNING: waiting for toplevel window to become visible\n");
    }

    if ( isShowing() )
    {
       foreplane=getGraphics();
       gc=foreplane;

       System.out.println("Window xsize= "+xsz);
       System.out.println("Window ysize= "+ysz);
       System.out.println("Window location: "+getX()+" : "+getY());

    }
    else
    {
       System.out.println("ERROR: Canvas object is not showing on the screen\n");
       System.exit(0);
    }

   // Get the drawing canvas size in pixel units
   //  Dimension d=size();
    this.setCanvasSize();


   // Set colors
   for (int i=0; i<16; i++) {
     cvals[i] = new Color(reds[i],greens[i],blues[i]) ;
     cused[i] = 1;
   }
   for (int i=0; i<16; i++) {
     cvals[i+100] = new Color(greys[i],greys[i],greys[i]) ;
     cused[i+100] = 1;
   }

   // Set the initial background color to black
   devbck = 0;

   // Set up fonts, will do this later
   fontp=new Font[3];
   fontb=new Font[3];
   fonti=new Font[3];
   // Small
   fontp[0]=new Font("Helvetica",Font.PLAIN,8);
   fontb[0]=new Font("Helvetica",Font.BOLD,8);
   fonti[0]=new Font("Helvetica",Font.ITALIC,8 );
   // Medium
   fontp[1]=new Font("Helvetica",Font.PLAIN,12);
   fontb[1]=new Font("Helvetica",Font.BOLD,12);
   fonti[1]=new Font("Helvetica",Font.ITALIC,12);
   // Large
   fontp[2]=new Font("Helvetica",Font.PLAIN,14);
   fontb[2]=new Font("Helvetica",Font.BOLD,14);
   fonti[2]=new Font("Helvetica",Font.ITALIC,14);

   // Set initial default font size
   setFontSize();

   initflg=true;

  }

  /**
   *
      Select a font based on screen size.
  */
  void setFontSize()
  {
    if (width<601 || height<421)
    {
      gc.setFont(fontb[0]);
      gfont=0;
      // System.out.println("Set font size based on Canvas window size");
      // System.out.println("For the current window size, use small fonts");
    }
    else if (width<1001 || height<651)
    {
      gc.setFont(fontb[1]);
      gfont=1;
      // System.out.println("Set font size based on Canvas window size");
      // System.out.println("For the current window size, use medium  fonts");
    }
    else
    {
      gc.setFont(fontb[2]);
      gfont=2;
      // System.out.println("Set font size based on Canvas window size");
      // System.out.println("For the current window size, use large fonts");
    }
  }

  void setFontSize(int x)
  {
    if (x<0 || x > 2) return;
    gc.setFont(fontb[x]);
    return;
  }

  void gxdend ()
  {
    gc.dispose();
    System.out.println("GX package terminated \n");
    return;
  }

  void gxdsgl() {
      setDrawMode(GX_SINGLE_BUFFER);
  }

  void gxddbl() {
      setDrawMode(GX_DOUBLE_BUFFER);
  }

  void gxdswp() {
      swapBuffers();
  }

  /* Frame action.
     0 -- new frame (clear display with the current background
          color)
  */
  void gxdfrm(int action) {
    switch(action) {
    case 0:
    case 1:
        setBackground(cvals[devbck]);
        gc.clearRect(0,0,width,height);
        gc.setColor(cvals[devbck]);
        // gc.setColor(new Color(0.0f,0.0f,0.0f));
        // System.out.println("Clear frame");
        gc.fillRect(0,0,width,height);
        break;
    case 9:
        break;
      default:
        return;
    }
  }

  void gxdcol(int clr)
  {
    if (clr<0) clr=0;
    if (clr>255) clr=255;
    if (devbck==1) {
      if (clr==0) clr = 1;
      else if (clr==1) clr = 0;
    }
    if (clr<16 && grflg==1) {
      if (devbck==1) clr = grrev[clr];
      clr+=100;
    }
    if (cused[clr]==0) clr=15;
    gc.setColor(cvals[clr]);
    // System.out.println("Color index= "+clr+"color name= "+cvals[clr].toString());
    lcolor=clr;
    return;
  }

  void gxdacl (int clr, int red, int green, int blue)
  {
    if (clr<16 || clr>255) return;
    // if (cused[clr]==1) XFreeColors(display, cmap, &(cvals[clr]),1,0);
    // red = red*256;
    // blue = blue*256;
    // green = green*256;
    // XAllocColor(display, cmap, &cell);
    cvals[clr] = new Color(red,green,blue) ;
    // cvals[clr] = cell.pixel;
    cused[clr] = 1;
  }



  /* Move to x,y   */
  void gxdmov (double x, double y){
    xxx = (int)(x*xscl+0.5);
    yyy = height - (int)(y*yscl+0.5);
    return;
  }

  void gxddrw (double x, double y){  /* Draw to x,y */
    int i, j;
    int w,h;
    i = (int)(x*xscl+0.5);
    j = height - (int)(y*yscl+0.5);
    // System.out.println("drawline from "+xxx+" "+yyy+" "+i+" "+y);
    gc.drawLine(xxx, yyy, i, j);
    if (wchose==1 && lwidth>5) {
      w = xxx - i;
      if (w<0) w = -1*w;
      h = yyy-j;
      if (h<0) h = -1*h;
      if (w<h) {
        gc.drawLine(xxx-1, yyy, i-1, j);
        if (lwidth>11) gc.drawLine( xxx+1, yyy, i+1, j);
      } else {
        gc.drawLine(xxx, yyy-1, i, j-1);
        if (lwidth>11) gc.drawLine( xxx, yyy+1, i, j+1);
      }
    }
    xxx = i;
    yyy = j;
    // if (QLength(display)&&rstate) gxdeve(0);
  }

  void gxdrec(double x1, double x2, double y1, double y2) {
     int i1,i2,j1,j2;

     i1 = (int)(x1*xscl+0.5);
     j1 = height - (int)(y1*yscl+0.5);
     i2 = (int)(x2*xscl+0.5);
     j2 = height - (int)(y2*yscl+0.5);
     gc.fillRect(i1,j2,i2-i1,j1-j2);
     return;
  }

  void gxdfil (double[] xy, int n) {
    int i;
    int ptr_r=0;
    int ptr_t=0;
    double[] pt;
    int[] pnt_x;
    int[] pnt_y;

    pnt_x=new int[n];
    pnt_y=new int[n];
    if (pnt_x==null || pnt_y==null) {
        System.out.println ("ERROR: polygon fill routine (gxdfil), not enough memory");
        return;
    }
    pt = xy;
    for (i=0; i<n; i++) {
      pnt_x[ptr_t]=(int)(pt[ptr_r]*xscl+0.5);
      pnt_y[ptr_t]=height-(int)(pt[ptr_r+1]*yscl+0.5);
      ptr_r += 2;
      ptr_t++;
    }
    // System.out.println("Java Graphics: draw polygons");
    // XFillPolygon (display, drwbl, gc, point, n, Nonconvex, CoordModeOrigin);
    // gc.drawPolygon(pnt_x,pnt_y,n) ;

    //Graphics2D g2D = (Graphics2D) gc; // cast to 2D
    //g2D.setRenderingHint(RenderingHints.KEY_ANTIALIASING,
    //                   RenderingHints.VALUE_ANTIALIAS_OFF);
    gc.fillPolygon(pnt_x,pnt_y,n) ;
    //g2D.setRenderingHint(RenderingHints.KEY_ANTIALIASING,
    //                   RenderingHints.VALUE_ANTIALIAS_ON);

    // System.out.println("Java Graphics: draw polygons");
    //    if (QLength(display)&&rstate) gxdeve(0);
    return;
  }


  void gxdwid(int wid) { /* Set width     */
    int lw;
    lwidth=wid;
    if (wchose==1) return;
    lw = 0;
    if (lwidth>5) lw=2;
    if (lwidth>11) lw=3;
    if (lw != owidth) {
      //  XSetLineAttributes(display, gc, lw, LineSolid,
      //   CapButt, JoinBevel);
    }
    owidth = lw;
    return;
  }

  void gxdstr(String str,double x, double y)
  {
    int xxx = (int)(x*xscl+0.5);
    int yyy = height - (int)(y*yscl+0.5);
    gc.drawString(str,xxx,yyy);
  }

  void gxdxsz(int w, int h) {
      setBounds(0,0,w,h);
      setCanvasSize();
}

  void gxrdrw(int flag) {
  }

}

/*
   A reference class that is used as a place holder
   to pass x,y values to method so it can be changed.
*/
class xyposi
{
  public double x;
  public double y;

  xyposi() {
    x=0.0;y=0.0;
  }
}
/*
   A utility class that is used as a wrapper
   for a primitive type double.  It is used as an
   argument (a reference handle) to a method so that
   method can modified it's value.
*/
class DoubleValue
{
  public double value;

  DoubleValue() {
    value=0.0;
  }
}

