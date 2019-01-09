/*
  This class process graphics draw commands from GradsVM.

  @author Pedro T.H. Tsai
  @date   Feb 20, 2009

*/

import java.io.*;

import java.awt.*;
import java.awt.geom.*;
import java.awt.image.*;
import java.awt.event.*;
import javax.swing.*;

public class JGfxRender implements Runnable
{
   JFrame    f;            // Graphics "window"
   Dimension fSize;        // Current size of frame
   gxJ       g;            // Graphics canvas

   SockIPC   gfx_sock;   /* Communication link to send/receive
                            graphics primitive drawing commands
                            with GradsVM server. */

   ConfigInfo config;

   boolean       batch;
   /* Debug print info object for this class. */
   private final static DebugInfo   dbg=new DebugInfo("JGfxRender");

   /* Initialization block for static variables. */
   static
   {
      /* enable debug print and additional information such as
         current system time and thread information to be appended
         in the print statements.
      */
      dbg.setDebugLevel(DebugInfo.DBG_INFO);
      dbg.show_systime(true);
      dbg.show_thread_info(true);
   }

   public JGfxRender(ConfigInfo config)
   {
      this.config = config;
      this.gfx_sock = null;
   }

   public void set_gfx_sock(SockIPC gfx_sock)
   {
      this.gfx_sock=gfx_sock;
   }

   public void run()
   {
      dbg.INFO("run()....");

      int[]   win_dim_and_pos = null;

      int     ai_data_len = 1024*1024*2 ;

      int[]   ai_data = new int[ai_data_len];

      double xsz = 11.0;

      double ysz = 8.5;

      /* query the config info to see if the
         ser has specified the window size and
         location on the command line.
      */
      if ( config.hasName("win_frame_dim") == true )
      {
         win_dim_and_pos = config.getIntAry("win_frame_dim");
      }

      /* default window size and location. */
      if ( win_dim_and_pos == null )
      {
         win_dim_and_pos = new int[4];
         win_dim_and_pos[0] = 800;  /* Window width */
         win_dim_and_pos[1] = 600;  /* Window height */
         win_dim_and_pos[2] = 0;    /* Window x-pos (top-left) */
         win_dim_and_pos[3] = 0;    /* Window y-pos (top-left) */
      }

      /* Pedro: Both the size 'xsz' and 'ysz' must match
         between the JGrads client and GradsVM, so
         we need to consider this issue later.
      */
      dbg.INFO("GXDBGN: xsz: "+xsz+" ysz: "+ysz);

      gxdbgn(xsz,ysz, win_dim_and_pos);

      while ( this.gfx_sock == null )
      {
         try
         {
            /* pedro: fix this later, use wait and notify instead of
               sleep. */
            Thread.sleep(1000);
         }
         catch (Exception e) {}
      }
      dbg.API("gfx sock is ready...");

      while ( this.gfx_sock.connection_ok() == false )
      {
         try
         {
            /* pedro: fix this later, use wait and notify instead of
               sleep. */
            Thread.sleep(1000);
         }
         catch (Exception e) {}
      }

      int  nb;
      boolean  runflag = true;

      while( runflag )
      {
         try
         {
            dbg.API("waiting for gfx commands...");
            nb = this.gfx_sock.recv_gfx_cmd(ai_data, ai_data_len);
            dbg.API("Receive total of "+nb+" integer value from GVM");

            /* process gfx draw commands. */
            call(ai_data, nb);
         }
         catch (Exception e)
         {
            /* For now we just  exit when there is an
               socket IO error. Fix this later. pedro */
            System.exit(1);
            runflag=false;
         }
      }
   }

   /*
     dump graphics commands.
   */
   public static void dump(int[] ai_data, int ai_data_len, int start, int end)
   {
      for (int i=0; i < ai_data_len; i++ )
      {
         if ( i >= start && i <= end  )
         {
            dbg.INFO("index: "+i+" val: "+ ai_data[i]);
         }
      }
   }



   // Initialize graphics
   private void gxdbgn(double xsz, double ysz, int[] win_dim_and_pos)
   {
      // Create top level window to contain GrADS widget
      g = new gxJ();
      f = new JFrame("GrADS");
      f.add("Center", g ) ;

      // Set the size and location of Window Frame.
      f.setSize(win_dim_and_pos[0],  win_dim_and_pos[1]);
      f.setLocation(win_dim_and_pos[2], win_dim_and_pos[3]);

      f.setBackground(new Color(0,0,0));
      f.setVisible(true);
      fSize = f.getSize(); // save for later
      g.gxdbgn(xsz,ysz);
      g.gxdfrm(0);
   }

   // Redraw graphics window if needed
   private void redraw_if_needed() throws Exception
   {
      Dimension now;
      now = f.getSize();
      if ( fSize.width!=now.width || fSize.height!=now.height )
      {
         // g.setSize(now.width,now.height);
         // g.setBounds(0,0,now.width,now.height);
         g.setCanvasSize();
         /*
           We can not call ga.call() here, re-visit this
           issue later.

           Pedro
         */
         /* ga.call("gxHdrw"); */
         fSize.width  = now.width;
         fSize.height = now.height;
      }
   }

   /*


   */
   public int call(int[]  ai , int ai_len)
   {
      /* read the int value from the buffer. */
      int    pos;
      int    func;
      int    a;
      int    b;
      int    c;

      // Tokens for dispatcher of graphics primitives
      final int GXDBGN = 1;
      final int GXDCOL = 2;
      final int GXDACL = 3;
      final int GXDWID = 4;
      final int GXDFRM = 5;
      final int GXDREC = 6;
      final int GXDFIL = 7;
      final int GXDDRW = 8;
      final int GXDMOV = 9;
      final int GXDEND = 10;
      final int GXDBAT = 11;
      final int GXDSGL = 12;
      final int GXDDBL = 13;
      final int GXDSWP = 14;
      final int GXDXSZ = 15;
      final int GXRDRW = 16;


      pos=0;

      while ( pos < ai_len )
      {
         func = ai[pos]; pos++;

         switch(func)
         {
            case GXDBGN:
            {
               int[]   win_dim_and_pos = null;
               double xsz = (double) ai[pos]; pos++;
               double ysz = (double) ai[pos]; pos++;

               /* query the config info to see if the
                  user has specified the window size and
                  location on the command line.
               */
               if ( config.hasName("win_frame_dim") == true )
               {
                  win_dim_and_pos = config.getIntAry("win_frame_dim");
               }

               /* default window size and location. */
               if ( win_dim_and_pos == null )
               {
                  win_dim_and_pos = new int[4];
                  win_dim_and_pos[0] = 800;  /* Window width */
                  win_dim_and_pos[1] = 600;  /* Window height */
                  win_dim_and_pos[2] = 0;    /* Window x-pos (top-left) */
                  win_dim_and_pos[3] = 0;    /* Window y-pos (top-left) */
               }
               /* Pedro: for now we don't send this command
                  from GradsVM to JGrads Client, because
                  on the JGrads client side, we will go ahead
                  create the drawing frame on local window
                  system.

                  But the size 'xsz' and 'ysz' must match
                  between the JGrads client and GradsVM, so
                  we need to consider this issue later.
               */
               dbg.INFO("GXDBGN: xsz: "+xsz+" ysz: "+ysz);

               // gxdbgn(xsz,ysz, win_dim_and_pos);
            }
            break;

            case GXDCOL:
            {
               a=ai[pos]; pos++;
               g.gxdcol(a);
               dbg.API("GXDCOL: "+a);
            }
            break;

            case GXDACL:
            {
               int clr   = ai[pos];  pos++;
               int red   = ai[pos];  pos++;
               int green = ai[pos];  pos++;
               int blue  = ai[pos];  pos++;
               g.gxdacl(clr,red,green,blue);
               dbg.API("GXDACL: "+clr+" "+red+" "+green+" "+blue);
            }
            break;

            case GXDWID:
            {
               a=ai[pos];  pos++;
               g.gxdwid(a);
               dbg.API("GXDWID: "+a);
            }
            break;

            case GXDMOV:
            {
               int ix = ai[pos];  pos++;
               int iy = ai[pos];  pos++;
               double x = (double) Float.intBitsToFloat(ix);
               double y = (double) Float.intBitsToFloat(iy);
               g.gxdmov(x,y);
               dbg.API("GXDMOV: "+ix+" "+iy+" "+x+" "+y);
            }
            break;

            case GXDDRW:
            {
               int ix = ai[pos];  pos++;
               int iy = ai[pos];  pos++;
               double x = (double) Float.intBitsToFloat(ix);
               double y = (double) Float.intBitsToFloat(iy);
               g.gxddrw(x,y);
               dbg.API("GXDDRW: "+ix+" "+iy+" "+x+" "+y);
            }
            break;

            case GXDREC:
            {
               double x1 = (double) Float.intBitsToFloat(ai[pos]);  pos++;
               double x2 = (double) Float.intBitsToFloat(ai[pos]);  pos++;
               double y1 = (double) Float.intBitsToFloat(ai[pos]);  pos++;
               double y2 = (double) Float.intBitsToFloat(ai[pos]);  pos++;

               g.gxdrec(x1,x2,y1,y2);
               dbg.API("GXDREC: "+x1+" "+x2+" "+y1+" "+y2);
            }
            break;

            case GXDFIL:
            {
               int n = ai[pos];  pos++;

               double [] xy = new double[2*n];
               for ( int i=0; i<2*n; i++ )
               {
                  xy[i] = (double) Float.intBitsToFloat(ai[pos]);  pos++;
               }
               g.gxdfil(xy,n);
               dbg.API("GXDFIL: "+n);
            }
            break;

            case GXDBAT:
            {
               dbg.API("GXDBAT");
               batch=true;
            }
            break;

            case GXDFRM:
            {
               a=ai[pos];  pos++;
               g.gxdfrm(a);
               dbg.API("GXDFRM: "+a);
            }
            break;

            case GXDSGL:
            {
               dbg.API("GXDSGL");
               g.gxdsgl();
            }
            break;

            case GXDDBL:
            {
               dbg.API("GXDDBL");
               g.gxddbl();
            }
            break;

            case GXDSWP:
            {
               dbg.API("GXDSWP");
               g.gxdswp();
            }
            break;

            case GXDXSZ:
            {
               a=ai[pos];  pos++;
               b=ai[pos];  pos++;
               Dimension df = f.getSize();
               Dimension dg = g.getSize();
               f.setBounds(0,0,a,b);
               int w = (int) (0.5+( 1. * dg.width  / df.width  ) * a);
               int h = (int) (0.5+( 1. * dg.height / df.height ) * b);
               g.gxdxsz(w,h);
               dbg.API("GXDXSZ"+w+" "+h);
            }
            break;

            case GXRDRW:
            {
               a=ai[pos];  pos++;
               g.gxrdrw(a);
               dbg.API("GXRDRW: "+a);
            }
            break;

            default:
            {
            }

         } /* end of switch */
      }

      return 0;
   } /* end of call() method. */
}

