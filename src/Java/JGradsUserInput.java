/*
 This class implement the 'GrADS user command' thread. It uses
 Readline() to get the user's GrADS command, and then send the
 commands to the socket write thread.

 @author  Pedro T.H. Tsai,
 @version 1.0
 @date    Jan 30, 2009

*/

import jline.*;
import java.io.*;

public class JGradsUserInput implements Runnable
{
   ConsoleReader reader = null ;
   MsgQ          cmd_queue = null;

   static boolean batch=false;  // batch mode flag

   /* Debug print info object for this class. */
   private final static DebugInfo   dbg=new DebugInfo("JGradsUserInput");

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

   public JGradsUserInput(MsgQ cmd_queue)
   {
      try
      {
         this.reader = new ConsoleReader();
         this.reader.addCompletor(new FileNameCompletor());
      }
      catch (java.io.IOException e)
      {
         System.out.println(e);
      }
      this.cmd_queue = cmd_queue;
   }

   public void run()
   {
      int   chars_sent;
      int   chars_to_send;
      boolean  runflag = true;

      dbg.INFO("run()....");
      String cmd;

      try
      {
         while( runflag )
         {

            // send a re-draw message if needed: redraw_if_needed();

            // add jLine here: Loop over command line:
            if ( (cmd = reader.readLine("ga-> ")) ==null )
            {
               break;
            }

            ;
            if ( (chars_to_send = cmd.trim().length()) == 0 )
            {
               /* ignore empty string */
               continue;
            }

            dbg.API("user cmd: "+cmd);
            /* send the message to socket write queue. */
            chars_sent=this.cmd_queue.send(cmd, chars_to_send);
            if ( chars_sent != chars_to_send )
            {
               dbg.ERROR("MsgQ send failed for ["+cmd+
                         "] buffer is too small: number of chars to send: "+
                         chars_to_send+" and "+chars_sent+" chars sent");
            }
         }
      }
      catch (java.io.IOException e)
      {
         System.out.println(e);
      }
   }
}

