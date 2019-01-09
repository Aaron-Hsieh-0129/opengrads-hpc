/*
 NestedVM wrapper for GrADS. This setup the necessary hooks for the Swing based graphics.

 This file was originally created by Arlindo. To make it client - server,
 this file has been modified.  The client part of the code (accepting
 commands and calling gxJ graphics primitives) has been moved to
 GVMclient.java, the part of code that calls the NestedVM has moved
 over to GradsVM.java

 Modification: Pedro Tsai, Jan 29, 2009

*/

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

public class GVMclient
{
   MsgQ            cmd_msg_q;      /* Queue to pass user command to
                                      socket write thread. */

   JGradsUserInput jgrads_input;   /* Read user input command */

   JGradsCmd       jgrads_cmd;     /* Process user command and handle
                                      commnuication with GradsVM server. */

   JGfxRender      gfx_render;

   SockIPC         cmd_sock;       /* Communication link to send/recv cmd
                                      with GradVM server. */

   ConfigInfo      config;

   static Runtime ga;           // GrADS runtime
   static JFrame  f;            // Graphics "window"
   static Dimension fSize;      // Current size of frame
   static gxJ     g;            // Graphics canvas

   static boolean batch=false;  // batch mode flag

   // Debug print info for tracing the program action.
   private final static DebugInfo   dbg=new DebugInfo();

   /* Initialization block for static variables. */
   static
   {
      /* enable debug print and additional information such as
         current system time and thread information to be appended
         in the print statements.
      */
      dbg.setDebugLevel(DebugInfo.DBG_ALL);
      dbg.show_systime(true);
      dbg.show_thread_info(true);
   }

   public GVMclient(String[] args)
   {
      /* create a msgq object with maximum of 32 msg, for each
         message, it's payload type is String, with String len
         of 128 char max.
      */
      this.cmd_msg_q = new MsgQ(32, MsgQ.PAYLOAD_TYPE_STRING, 128 );

      /* create a user input command process
         and pass to it the msg queue object.
      */
      this.jgrads_input = new JGradsUserInput(this.cmd_msg_q);

      /* Here we capture the original command arguments before
         it is pass to server Runtime() */
      this.config=new ConfigInfo();
      this.config.load_grads_args(args);

      /* create a gfx render object. */
      this.gfx_render = new JGfxRender(this.config);

      /* create a command processor  */
      this.jgrads_cmd = new JGradsCmd(this.cmd_msg_q, this.gfx_render);

   }

   public void start_all_threads()
   {
      /* start command dispatch thread to do socket communication.  */
      Thread dispatch_thread = new Thread (jgrads_cmd);
      dispatch_thread.start();

      /* start the user command thread */
      Thread cmd_thread = new Thread (jgrads_input);
      cmd_thread.start();

      /* start graphics rendering thread. */
      Thread render_thread = new Thread(this.gfx_render);
      render_thread.start();

   }

   public static void main(String[] args) throws Exception
   {
      System.out.println("Starting JGrADS under Java VM v"
                         + System.getProperty("java.vm.version") + " on "
                         + System.getProperty("os.name") + " "
                         + System.getProperty("os.version"));


      GVMclient   jg = new GVMclient(args);
      jg.start_all_threads();


      while (true)
      {
         Thread.sleep(5000);
      }

      // All done
      // System.exit(ga.exitStatus());
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
                /* ga.call("gxHdrw"); */
                fSize.width  = now.width;
                fSize.height = now.height;
            }
        }
    }
}

