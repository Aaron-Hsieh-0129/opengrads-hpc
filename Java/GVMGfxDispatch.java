/*
 This class sends the Graphics draw primitive commands from GradsVM to
 JGrads client.  It uses socket to send/receive command/result
 to/from the server.

 @author  Pedro T.H. Tsai,
 @version 1.0
 @date    Feb 19, 2009

*/

import jline.*;
import java.io.*;
import java.util.*;


public class GVMGfxDispatch implements Runnable
{
   CrcBufList   crclist;        /* Circular list for the gfx cmds buffer. */

   SockIPC      gfx_sock;       /* Socket comm to receive graphics primitive
                                   calls from GradsVM server. */

   /* Debug print info object for this class. */
   private final static DebugInfo   dbg=new DebugInfo("GVMGfxDispatch");

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

   public GVMGfxDispatch(CrcBufList crclist, SockIPC gfx_sock)
   {
      this.crclist = crclist;

      /* SockIPC for sending graphics primitives from GradsVM server
         to JGrads client
      */
      this.gfx_sock = gfx_sock;
   }

   public void run()
   {
      int      ints_sent;
      int      ints_recv;
      boolean  runflag = true;
      IntBuf   gfx;

      dbg.INFO(" run()....");

      Thread.yield();

      while( runflag )
      {
         /* Wait for gfx data to become available. */
         dbg.API("Wait for gfx data.... ");
         gfx = (IntBuf)this.crclist.get_read_obj();

         dbg.API(" gfx cmd data available to sent...");

         try
         {
            if ( gfx_sock.connection_ok() )
            {
               dbg.API("Send gfx command to JGrads client... len: "+
                        gfx.length());

               gfx_sock.send_gfx_cmd(SockIPC.GRADS_GX_DRAW_REQUEST,
                                     gfx.get_int_array(), gfx.length());

               /* clear the buffer. */
               gfx.reset();
            }
         }
         catch (Exception e)
         {
            runflag = false;
         }

         dbg.API("unlock gfx commands buffer.");
         this.crclist.unlock_read_obj(gfx);

      }

      System.exit(0);
   }
}

