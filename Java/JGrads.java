//
// NestedVM wrapper for GrADS. This setup the necessary hooks for the Swing based graphics.
//

import org.ibex.nestedvm.Runtime;
import jline.*;

import java.io.*;

import java.awt.*;
import java.awt.geom.*;
import java.awt.image.*;
import java.awt.event.*;
import javax.swing.*;

//
// This is the top JGrADS class.
//

public class JGrads {

    static Runtime ga;           // GrADS runtime
    static JFrame  f;            // Graphics "window"
    static Dimension fSize;      // Current size of frame
    static gxJ     g;            // Graphics canvas

    static boolean batch=false;  // batch mode flag

    // Debug print info
    // private final static DebugInfo   dbg=new DebugInfo(DebugInfo.DBG_ALL);


    public static void main(String[] args) throws Exception
    {

        System.out.println("Starting JGrADS under Java VM v"
                            + System.getProperty("java.vm.version") + " on "
                            + System.getProperty("os.name") + " "
                            + System.getProperty("os.version"));

        // Here we capture the original command arguments before
        // it is pass to Runtime() then back to
        // this class.
        final  ConfigInfo  config = new ConfigInfo();
        config.load_grads_args(args);

        // Instantiate the GrADS runtime under NextedVM
        ga = (Runtime) Class.forName("grads").newInstance();

        // Register the Java callback dispatcher
        ga.setCallJavaCB( new Runtime.CallJavaCB()
           {
              public int call(int func, int a, int b, int c)
              {
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

                  switch(func)
                  {
                      case GXDBGN:
                      {
                         try {
                            int[]   win_dim_and_pos = null;
                            double xsz = (double)Float.intBitsToFloat(ga.memRead(a+0));
                            double ysz = (double)Float.intBitsToFloat(ga.memRead(a+4));
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
                            gxdbgn(xsz,ysz, win_dim_and_pos);
                         } catch(Runtime.ReadFaultException e) {}
                       }
                       break;

                      case GXDCOL:
                      {
                         g.gxdcol(a);
                      }
                      break;

                      case GXDACL:
                      {
                         try {
                             int clr   = ga.memRead(a+0); // a is really a pointer here
                             int red   = ga.memRead(a+4);
                             int green = ga.memRead(a+8);
                             int blue  = ga.memRead(a+12);
                             g.gxdacl(clr,red,green,blue);
                             return 0;
                         } catch(Runtime.ReadFaultException e) { }
                      }
                      break;

                      case GXDWID:
                      {
                          g.gxdwid(a);
                      }
                      break;

                      case GXDMOV:
                      {
                          try {
                              double x = (double) Float.intBitsToFloat(ga.memRead(a+0));
                              double y = (double) Float.intBitsToFloat(ga.memRead(a+4));
                              g.gxdmov(x,y);
                          } catch(Runtime.ReadFaultException e) {}
                      }
                      break;

                      case GXDDRW:
                      {
                          try {
                              double x = (double) Float.intBitsToFloat(ga.memRead(a+0));
                              double y = (double) Float.intBitsToFloat(ga.memRead(a+4));
                              g.gxddrw(x,y); break;
                          } catch(Runtime.ReadFaultException e) {}
                      }
                      break;

                      case GXDREC:
                      {
                          try {
                              double x1 = (double) Float.intBitsToFloat(ga.memRead(a+0));
                              double x2 = (double) Float.intBitsToFloat(ga.memRead(a+4));
                              double y1 = (double) Float.intBitsToFloat(ga.memRead(a+8));
                              double y2 = (double) Float.intBitsToFloat(ga.memRead(a+12));
                              g.gxdrec(x1,x2,y1,y2);
                          } catch(Runtime.ReadFaultException e) {}
                      }
                      break;

                      case GXDFIL:
                      {
                          int n = b;
                          double [] xy = new double[2*n];
                          try {
                              for ( int i=0; i<2*n; i++ )
                                  {
                                      xy[i] = (double) Float.intBitsToFloat(ga.memRead(a+4*i));
                                  }
                          } catch(Runtime.ReadFaultException e) {}
                          g.gxdfil(xy,n);
                      }
                      break;

                      case GXDBAT:
                      {
                          batch=true;
                      }
                      break;

                      case GXDFRM:
                      {
                          g.gxdfrm(a);
                      }
                      break;

                      case GXDSGL:
                      {
                          g.gxdsgl();
                      }
                      break;

                      case GXDDBL:
                      {
                          g.gxddbl();
                      }
                      break;

                      case GXDSWP:
                      {
                          g.gxdswp();
                      }
                      break;

                      case GXDXSZ:
                      {
                          Dimension df = f.getSize();
                          Dimension dg = g.getSize();
                          f.setBounds(0,0,a,b);
                          int w = (int) (0.5+( 1. * dg.width  / df.width  ) * a);
                          int h = (int) (0.5+( 1. * dg.height / df.height ) * b);
                          g.gxdxsz(w,h);
                      }
                      break;

                      case GXRDRW:
                      {
                          g.gxrdrw(a);
                      }
                      break;

                      default:
                      {
                          return 0;
                      }

                  } /* end of switch */

                  return 0;
              } /* end of call() method. */
           }
       );

       // Now start the GrADS application: "grads" is the class name,
       // followed by the command line arguments
       String [] Args = new String[args.length+1]; Args[0] = "grads";
       for (int i=0; i<args.length; i++) Args[i+1] = args[i];
       ga.start(Args);
       ga.execute();  // start execution

        // Make sure relevant environment is available inside C code
       fixenv("GADDIR");
       fixenv("GASCRP");
       fixenv("GAUDFT");
       fixenv("GAUDXT");
       fixenv("GAGUI");
       fixenv("GRIBTAB");
       fixenv("GRIBTAB");

       // Run up to before command line loop
       ga.execute();
       int rc = ga.call("gxflag",0);

       // Loop over command line: add jLine here
       ConsoleReader reader = new ConsoleReader();
       reader.addCompletor(new FileNameCompletor());
       String cmd;
       while(rc>-1)
       {
          redraw_if_needed();
            if ((cmd = reader.readLine("ga-> "))==null) break;
            rc = ga.call("gaCmd",ga.strdup(cmd));
       }
       ga.execute();     // finish it up

       // All done
       System.exit(ga.exitStatus());
    }

    // Initialize graphics
    private static void gxdbgn(double xsz, double ysz, int[] win_dim_and_pos)
    {
        if ( !batch )
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
    }

    // Redraw graphics window if needed
    private static void redraw_if_needed() throws Exception {
        Dimension now;
        if ( !batch )
        {
            now = f.getSize();
            if ( fSize.width!=now.width || fSize.height!=now.height )
            {
                // g.setSize(now.width,now.height);
                // g.setBounds(0,0,now.width,now.height);
                g.setCanvasSize();
                ga.call("gxHdrw");
                fSize.width  = now.width;
                fSize.height = now.height;
            }
        }
    }


    // If variable is set as a property, then C code you use that as environment variable;
    // otherwise, it will use the actual environment variable, if set.
    private static void fixenv(String var) throws Exception {
        String val = System.getProperty(var);
        if ( val==null ) val = System.getenv(var);
        if ( val!=null ) ga.call("setenv_",ga.strdup(var), ga.strdup(val));
    }

}

