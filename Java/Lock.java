/**
  This class implements a simple exclusive locking mechanism, with
  wait() and notify() for synchronization.

 @author Pedro T.H. Tsai
 @version 1.0
 @Date 2009 Feb 20
*/

import java.util.*;

public class Lock
{
   private  Thread owner = null ;
   private  int    lock_count = 0 ;

   /* Debug print info object for this class. */
   private static final DebugInfo dbg = new DebugInfo("Lock");

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

   public Lock()
   {
      this.owner = null;
      this.lock_count = 0;
   }

   /**
    Constructor
    @param lock_state   true    create the lock in lock state.
   */
   public Lock(boolean  lock_state)
   {
      if ( lock_state )
      {
         lock();
      }
   }

   public synchronized int lock()
   {
      while ( try_to_get_lock() == false )
      {
         try
         {
            wait();
         }
         catch (Exception e)
         {
            System.out.println(e);
         }
      }

      return 0;
   }

   private synchronized boolean try_to_get_lock()
   {
      if ( owner == null )
      {
         this.owner = Thread.currentThread();
         this.lock_count = 1;
         return true;
      }
      if ( this.owner == Thread.currentThread() )
      {
         this.lock_count++;
         return true;
      }
      return false;
   }

   public synchronized int unlock()
   {
      if ( this.owner == Thread.currentThread() )
      {
         this.lock_count--;
         if ( this.lock_count == 0 )
         {
            owner = null;
            notify();
         }
      }
      else
      {
         dbg.ERROR("Not owner");
         return -1;
      }
      return 0;
   }

   /**
    Usage example and Self-test function.
    @parm
   */
   public static void main(String[] args) throws Exception
   {
      return;
   }
}









