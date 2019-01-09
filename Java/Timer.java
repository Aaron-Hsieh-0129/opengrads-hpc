/**
  This is a Timer object implementation. The timer object starts a
  thread and after a caller specified time interval has passed, it
  invokes a caller provided function, and then timer thread exited.
  An option can be set to allow timer thread to continue to run, and
  call the specified function at the specified 'time interval'.

 @author Pedro T.H. Tsai
 @version 1.0
 @Date 2009 Jan 25
*/

import java.util.*;

public class Timer extends Thread
{
   TimerNfyIntface nfy_obj = null;
   Object          pv_data = null;
   int             tm_val = 0;
   boolean         once_flag = true;
   boolean         exit_flag = false;

   /* Debug print info object for this class. */
   private static final DebugInfo dbg = new DebugInfo();

   /* Initialization block to initialize static variables. */
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

   /**
    Constructor of timer object.
    @param tm_val   The elapse time (millesec) of timer,
    @param once     True: the action is execute once and then
                    timer exists.
                    False: the action is executed with interval
                    of 'tm_val'.
    @param nfy_obj  Reference to the object that implements the action to be
                    executed by the timer.
    @param pv_data  Data object passed to the callback function of 'nfy_obj'.
   */
   public Timer(int tm_val,
                boolean once,
                TimerNfyIntface nfy_obj,
                Object pv_data)
   {
      this.nfy_obj = nfy_obj;
      this.tm_val = tm_val;
      this.pv_data = pv_data;
      this.once_flag = once ;
   }

   /**
     Tell the Timer thread to exit.

     If the timer thread is set to repeat mode, e.g, calling
     the client's function at the specified 'time interval', then to
     stop the timer thread, this function must be called.

     For timer thread set to execute once only, the timer thread will
     automaticallly exit after the client function is called, so 'exit()'
     call is not needed.

   */
   public void exit()
   {
      this.exit_flag = true ;
   }

   public void run()
   {
      do
      {
         try
         {
            sleep(this.tm_val);
            if ( this.exit_flag == false )
            {
               dbg.INFO("execute action.");

               if ( this.nfy_obj != null )
               {
                  this.nfy_obj.action(this,pv_data);
               }
            }
            else
            {
               break;
            }
         }
         catch ( Exception e )
         {
            System.out.println(e);
            this.exit();
         }
      } while( this.once_flag == false && this.exit_flag == false );

      dbg.INFO("timer thread exiting.");
   }

   /**
    Usage example and Self-test function.
    @parm
   */
   public static void main(String[] args) throws Exception
   {
      /* create a timer to execute once only after 2 seconds delay. */
      Timer   timer_0=new Timer(2000, true, null, null );
      timer_0.start();

      /* create a timer to execute repeatly with 250 milliseconds interval. */
      Timer  timer_1=new Timer(250, false, null, null);
      timer_1.start();

      Thread.sleep(5000);

      /* tell the repeating timer to exit. */
      timer_1.exit();

      return;
   }
}









