/*
 This class handles communication between JGrads client and
 GradsVM server.  It uses socket to send/receive command/result
 to/from the server.

 @author  Pedro T.H. Tsai,
 @version 1.0
 @date    Feb 9, 2009

*/

import jline.*;
import java.io.*;
import java.util.*;


public class JGradsCmd implements Runnable
{
   MsgQ         cmd_queue = null;

   JGfxRender   gfx_render = null;

   SockIPC      cmd_sock;       /* Socket comm to send user command to
                                   GradVM server. */
   SockIPC      gfx_sock;       /* Socket comm to receive graphics primitive
                                   calls from GradsVM server. */

   /* Debug print info object for this class. */
   private final static DebugInfo   dbg=new DebugInfo("JGradsCmd");

   /* Initialization block for static variables. */
   static
   {
      /* enable debug print and additional information such as
         current system time and thread information to be appended
         in the print statements.
      */
      dbg.setDebugLevel(DebugInfo.DBG_INFO | DebugInfo.DBG_ERROR);
      dbg.show_systime(true);
      dbg.show_thread_info(true);
   }

   public JGradsCmd(MsgQ cmd_queue, JGfxRender gfx_render)
   {
      this.cmd_queue = cmd_queue;

      this.gfx_render = gfx_render;

      /* SockIPC for sending user commands to GradsVM server. */
      cmd_sock = new SockIPC(SockIPC.CMD_BUF_SIZE);

      /* SockIPC for receiving graphics primitives from GradsVM server. */
      gfx_sock = new SockIPC(SockIPC.GFX_BUF_SIZE);
   }

   public void run()
   {
      int   chars_sent;
      int   chars_recv;
      boolean  runflag = true;
      StringBuffer cmdbuf = new StringBuffer(SockIPC.CMD_BUF_SIZE);
      String   cmd;

      dbg.INFO("run()....");

      while( runflag )
      {
         /* Wait for message. */
         chars_recv = this.cmd_queue.recv(cmdbuf, cmdbuf.capacity());
         cmd=cmdbuf.toString();

         dbg.API("JGradsCmd ["+cmd+"]");


         /* process command. */
         if ( cmd.equals("quit") )
         {
            /* send the 'quit' message to the GradsVM and then

               exit. */

            runflag = false;
         }

         if ( cmd.startsWith("conn") )
         {
            StringTokenizer  token=new StringTokenizer(cmd);

            /* default IP addr is local loop-back. */
            String           ipaddr="127.0.0.1";

            /* default port number to used if user has not specified. */
            int              port = SockIPC.VMGRADS_IP_PORT;

            /* the command line pattern:  'conn  ip_addr  port' */
            try
            {
               token.nextToken(); /* consume 'conn' */

               if ( token.hasMoreTokens() )
               {
                  ipaddr = token.nextToken();
               }
               if ( token.hasMoreTokens() )
               {
                  String  custom_port=token.nextToken();
                  port = Integer.parseInt(custom_port);
               }

               /* establish connection with the server. */
               dbg.INFO("Open connection to:  "+ipaddr+" "+port);

               if ( cmd_sock.connect(ipaddr, port) != 1 )
               {
                  dbg.ERROR("Connection failed. please check if server is running on the specified host and port number.");
                  continue;
               }
               if ( gfx_sock.connect(ipaddr, port+1) != 1 )
               {
                  dbg.ERROR("Connection failed. please check if server is running on the specified host and port number.");

                  cmd_sock.close();
                  continue;
               }

               /* Graphics primitive commands socket is connected,
                  we set the gfx render object with this sockIPC so
                  it can receive gfx commands from the GradsVM.
               */
               this.gfx_render.set_gfx_sock(gfx_sock);

               /*
                  Both connections established. send the args to
                  the remote grads vm
               */


               continue;
            }
            catch (Exception e)
            {
               System.out.println(e);
            }
         }

         if ( cmd_sock.connection_ok() )
         {
            dbg.API("Send command to server... "+cmdbuf+" len: "
                     +cmdbuf.length());

            try
            {
               cmd_sock.send_ga_cmd(SockIPC.GRADS_CMD_REQUEST,
                                    cmdbuf);
            }
            catch (Exception e)
            {
               /* For now we just  exit when there is an
                  socket IO error. Fix this later.
               */
               System.exit(1);
               runflag=false;
            }
         }
      }

      System.exit(0);
   }
}

