/************************************************************************
  NestedVM wrapper for GrADS. This setup the necessary hooks for the Swing
  based graphics.

***********************************************************************/

import org.ibex.nestedvm.Runtime;
import jline.*;

import java.io.*;
import java.net.*;

//
// This is the top GradsVM class.
//

public class GVM
{
   static Runtime ga;           // GrADS runtime

   static boolean batch=false;  // batch mode flag

   static int NB_GFX_CMD_BUF=6;

   static int SIZE_GFX_CMD_BUF=1024*128;

   // Debug print info
   private final static DebugInfo   dbg=new DebugInfo("GVM");

   /* Initialization block for static variables. */
   static
   {
      /* enable debug print and additional information such as
         current system time and thread information to be appended
         in the print statements.
      */
      dbg.setDebugLevel(DebugInfo.DBG_NONE);
      dbg.show_systime(true);
      dbg.show_thread_info(true);
   }

   SockIPC      cmd_sock;       /* Communication link to send/recv cmd
                                   with JGrads client. */

   SockIPC      gfx_sock;       /* Communication link to send/receive
                                   graphics primitive drawing commands
                                   with JGrads client. */

   ConfigInfo   config;

   IntBuf       gfx;

   IntBuf[]     ai_data;

   CrcBufList   crclist;

   public GVM(Socket cmd_data_sock, Socket gfx_data_sock)
   {
      /* create private communication link with JGrads client. */
      this.cmd_sock = new SockIPC(cmd_data_sock, SockIPC.CMD_BUF_SIZE);

      /* SockIPC for receiving graphics primitives from GradsVM server. */
      this.gfx_sock = new SockIPC(gfx_data_sock,SockIPC.GFX_BUF_SIZE);

      this.config=new ConfigInfo();

      /* create array of integer buf objs to store gfx draw
         cmd */
      this.ai_data = new IntBuf[NB_GFX_CMD_BUF];

      for (int i=0; i<ai_data.length; i++ )
      {
         this.ai_data[i]=new IntBuf(SIZE_GFX_CMD_BUF);
      }

      /* create a circular buffer list to manage
         graphics draw commands buf */
      this.crclist = new CrcBufList(ai_data, ai_data.length);

      this.gfx = (IntBuf) this.crclist.get_write_obj();

      GVMGfxDispatch gfx_send =
         new GVMGfxDispatch(this.crclist, this.gfx_sock);

      Thread  t=new Thread(gfx_send);
      t.start();
   }

   public String[] get_client_args()
   {
      /* Get the args from JGrads client. */
      String[] args=new String[0];

      /* Process the JGrads client command arguments before
         it is pass to server Runtime() */
      this.config.load_grads_args(args);

      return args;
   }

   public void run(String[] args) throws Exception
   {
      System.out.println("Starting GradsVM under Java VM v"
                         + System.getProperty("java.vm.version") + " on "
                         + System.getProperty("os.name") + " "
                         + System.getProperty("os.version"));

      // Instantiate the GrADS runtime under NextedVM
      ga = (Runtime) Class.forName("grads").newInstance();
      dbg.INFO("create an instance of grads Runtime object.");

      dbg.INFO("Register the Java callback dispatcher function.");
      // Register the Java callback dispatcher
      ga.setCallJavaCB( new Runtime.CallJavaCB()
         {
              /*
                 Description of Callback function parameters:

                    func:   Graphics primitive ops (gxJ function to call)
                    a:      pointer to the argument buffer ....
                    b:      length of data in 'a' buffer

              */
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
                          double xsz = (double)
                             Float.intBitsToFloat(ga.memRead(a+0));
                          double ysz = (double)
                             Float.intBitsToFloat(ga.memRead(a+4));

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
                       } catch(Runtime.ReadFaultException e) {}
                    }
                    break;

                    case GXDCOL:
                    {
                       gfx.add(GXDCOL,a);
                       // g.gxdcol(a);
                       dbg.API("GXDCOL: "+a);
                    }
                    break;

                    case GXDACL:
                    {
                       try {
                          int clr   = ga.memRead(a+0); // a is really a pointer here
                          int red   = ga.memRead(a+4);
                          int green = ga.memRead(a+8);
                          int blue  = ga.memRead(a+12);
                          gfx.add(GXDACL,clr,red,green,blue);
                          // g.gxdacl(clr,red,green,blue);
                          dbg.API("GXDACL: "+clr+" "+red+" "+green+" "+blue);
                       } catch(Runtime.ReadFaultException e) { }
                    }
                    break;

                    case GXDWID:
                    {
                       gfx.add(GXDWID,a);
                       // g.gxdwid(a);
                       dbg.API("GXDWID: "+a);
                    }
                    break;

                    case GXDMOV:
                    {
                       try {

                          double x = (double) Float.intBitsToFloat(
                             ga.memRead(a+0));
                          double y = (double) Float.intBitsToFloat(
                             ga.memRead(a+4));

                          int ix = ga.memRead(a+0);
                          int iy = ga.memRead(a+4);

                          gfx.add(GXDMOV,ix,iy);
                          // g.gxdmov(x,y);
                          dbg.API("GXDMOV: "+ix+" "+iy+" "+x+" "+y);
                       } catch(Runtime.ReadFaultException e) {}
                    }
                    break;

                    case GXDDRW:
                    {
                       try {
                          double x = (double) Float.intBitsToFloat(
                             ga.memRead(a+0));
                          double y = (double) Float.intBitsToFloat(
                             ga.memRead(a+4));
                          int ix = ga.memRead(a+0);
                          int iy = ga.memRead(a+4);
                          gfx.add(GXDDRW, ix, iy);
                          // g.gxddrw(x,y);
                          dbg.API("GXDDRW: "+ix+" "+iy+" "+x+" "+y);
                       } catch(Runtime.ReadFaultException e) {}
                    }
                    break;

                    case GXDREC:
                    {
                       try {
                          /*
                          double x1 = (double) Float.intBitsToFloat(
                             ga.memRead(a+0));
                          double x2 = (double) Float.intBitsToFloat(
                             ga.memRead(a+4));
                          double y1 = (double) Float.intBitsToFloat(
                             ga.memRead(a+8));
                          double y2 = (double) Float.intBitsToFloat(
                             ga.memRead(a+12));
                          */
                          int x1 = ga.memRead(a+0);
                          int x2 = ga.memRead(a+4);
                          int y1 = ga.memRead(a+8);
                          int y2 = ga.memRead(a+12);

                          gfx.add(GXDREC,x1,x2,y1,y2);
                          // g.gxdrec(x1,x2,y1,y2);
                          dbg.API("GXDREC: "+x1+" "+x2+" "+y1+" "+y2);
                       } catch(Runtime.ReadFaultException e) {}
                    }
                    break;

                    case GXDFIL:
                    {
                       int n = b;
                       /*
                       double [] xy = new double[2*n];
                       try {
                          for ( int i=0; i<2*n; i++ )
                          {
                             xy[i] = (double) Float.intBitsToFloat(
                                ga.memRead(a+4*i));
                          }
                       } catch(Runtime.ReadFaultException e) {}
                       g.gxdfil(xy,n);
                       */
                       gfx.add(GXDFIL, n);
                       try {
                          for ( int i=0; i<2*n; i++ )
                          {
                             gfx.add(ga.memRead(a+4*i));
                          }
                          dbg.API("GXDFIL: "+n);
                       } catch(Runtime.ReadFaultException e) {}

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
                       gfx.add(GXDFRM,a);
                       gfx.info();

                       /* we unlock, so the sending thread
                          can obtain the lock on the
                          gfx buffer and send the contents
                          to the rendering client.
                       */
                       dbg.INFO("Unlock write object ....");
                       crclist.unlock_write_obj(gfx);

                       Thread.yield();

                       dbg.INFO("Get next write object ....");
                       gfx = (IntBuf) crclist.get_write_obj();
                       dbg.INFO("next write object ready...");

                       // g.gxdfrm(a);
                       dbg.INFO("GXDFRM: "+a);
                    }
                    break;

                    case GXDSGL:
                    {
                       dbg.API("GXDSGL");
                       gfx.add(GXDSGL);
                       // g.gxdsgl();
                    }
                    break;

                    case GXDDBL:
                    {
                       dbg.API("GXDDBL");
                       gfx.add(GXDDBL);
                       // g.gxddbl();
                    }
                    break;

                    case GXDSWP:
                    {
                       dbg.API("GXDSWP");
                       gfx.add(GXDSWP);
                       // g.gxdswp();
                    }
                    break;

                    case GXDXSZ:
                    {
                       /*
                       Dimension df = f.getSize();
                       Dimension dg = g.getSize();
                       f.setBounds(0,0,a,b);
                       int w = (int) (0.5+( 1. * dg.width  / df.width  ) * a);
                       int h = (int) (0.5+( 1. * dg.height / df.height ) * b);
                       */
                       gfx.add(GXDXSZ,a,b);
                       // g.gxdxsz(w,h);
                       dbg.API("GXDXSZ: "+a+" "+b);
                    }
                    break;

                    case GXRDRW:
                    {
                       gfx.add(GXRDRW,a);
                       // g.gxrdrw(a);
                       dbg.API("GXRDRW: "+a);
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
       String [] Args = new String[args.length+1];
       Args[0] = "grads";
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

       /* Read command from socket connection sent by
          the JGrads clien
       */
       StringBuffer   s_cmd=new StringBuffer(SockIPC.CMD_BUF_SIZE);
       String cmd;

       while (rc > -1 )
       {
          dbg.INFO("Wait for JGrads cmd... ");
          this.cmd_sock.recv_ga_cmd(s_cmd);
          /* check for error. */

          dbg.INFO("Execute JGrads cmd: "+s_cmd);
          rc = ga.call("gaCmd",ga.strdup(s_cmd.toString()));

          /* send result of execution back. */
       }
       /*
       // Loop over command line: add jLine here
       ConsoleReader reader = new ConsoleReader();
       reader.addCompletor(new FileNameCompletor());
       String cmd;
       while(rc>-1)
       {
          redraw_if_needed();
            if ((cmd = reader.readLine("ga-> "))==null) break;
            dbg.INFO("cmd: "+cmd);
            rc = ga.call("gaCmd",ga.strdup(cmd));
       }
       */

       ga.execute();     // finish it up

       // All done
       // System.exit(ga.exitStatus());

       /* Let this thread exit */
       dbg.INFO("GradsVM thread exiting...");

    }

   /*
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
   */

    // If variable is set as a property, then C code you use that as environment variable;
    // otherwise, it will use the actual environment variable, if set.
    private static void fixenv(String var) throws Exception
    {
        String val = System.getProperty(var);
        if ( val==null ) val = System.getenv(var);
        if ( val!=null ) ga.call("setenv_",ga.strdup(var), ga.strdup(val));
    }

}

