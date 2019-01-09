/**
  This class implements a circular data storage buffer.  The data I/O are
  First In/Firt Out (FIFO).

  The type of data object managed is defined by the user of this class.
  The user defined data object must implement 'lock' and 'unlock' API
  interface for synchronization.

  The purpose of this Circular List manager is to allow 2 threads
  to communicate data without blocking: one thread will write data to
  objects in the circular list, the other thread will read the data from
  the objects in the circular list.

  If there is no available data to read in circular list, then read thread
  will just wait until notified by the write thread that data are ready
  to be read.

 @author Pedro T.H. Tsai
 @version 1.0
 @Date 2009 Feb 20
*/

import java.util.*;

public class CrcBufList
{
   private  int              max_capacity;
   private  int              nb_obj_locked_for_write;
   private  int              read_pos;
   private  int              write_pos;

   private  CrcBufDataObjIntface[]   objs;

   /* Debug print info object for this class. */
   private static final DebugInfo dbg = new DebugInfo("CrcBufList");

   /* Initialization block to initialize static variables. */
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

   /**
    Constructor of message queue.

    @param max_num_msg   Maximum number of message in this queue.
    @param payload_type  The type of payload in the message.  Currently,
                         For primitive type, we only support int and byte.
                         For object type, we only support Stringbuffer type.


   */
   public CrcBufList(CrcBufDataObjIntface[] objs, int capacity )
   {
      dbg.INFO("number of object loaded: "+capacity);

      /* nb of objects tat has been locked for write operation (e.g,
         potentially available to read) */
      this.nb_obj_locked_for_write = 0;

      /* maximum nb of objects in the circular list. */
      this.max_capacity = capacity;

      /* index pos in circular list for the read operation */
      this.read_pos    = -1;

      /* index pos in circular list for the write operation */
      this.write_pos   = 0;

      this.objs=objs;

      for (int i=0; i < capacity; i++ )
      {
         ((IntBuf)this.objs[i]).info();
      }
   }

   /**
    Increment the read/write pointer, handle the wrap-around case.

    @param pos   current pos

    @return      next pos.
   */
   private int increm_read_or_write_pos(int pos)
   {
      if ( (pos + 1) < this.max_capacity )
      {
         pos = pos + 1;
      }
      else
      {
         pos = 0;
      }
      return pos;
   }

   /**
    Ge the next write object from the circular list, the object is
    locked.

    @return    ref to the object.
   */
   public synchronized Object get_write_obj()
   {
      dbg.INFO("nb of object locked for write: "+this.nb_obj_locked_for_write);

      while ( this.nb_obj_locked_for_write >= this.max_capacity )
      {
         /* no available object in the circular list, so wait. */
         try {
            wait();
         }
         catch (Exception e)
         {
            System.out.println(e);
         }
      }

      int  wr = this.write_pos;

      dbg.INFO("wr pos: "+wr);

      this.objs[wr].lock();

      /* if this is very first write operation, then we need
         set the read_pos to this pos (kick start).
      */
      if ( this.read_pos == -1 )
      {
         this.read_pos = wr;
      }

      this.write_pos = increm_read_or_write_pos(this.write_pos) ;

      this.nb_obj_locked_for_write++;

      notify();
      return this.objs[wr];
   }

   /**
    Ge the next object to read from the circular list, after return
    from this API the object is locked.

    @return    ref to the object.
   */
   public synchronized Object get_read_obj()
   {
      while ( this.read_pos == -1  ||
              this.read_pos == this.write_pos )
      {
         dbg.ASSERT( (this.nb_obj_locked_for_write == 0), "nb_obj_locked_for_write != 0" );

         /* no available data in the list, so wait. */
         try {
            dbg.INFO("Waiting for data object to become available...");
            wait();
         }
         catch (Exception e)
         {
            System.out.println(e);
         }
      }

      int  rd = this.read_pos;

      dbg.INFO("Get Read Obj => Waiting for the write lock to be free....");
      objs[rd].lock();

      dbg.INFO("Get Read Obj => write lock free now we got read lock on object ["+rd+"]....");

      this.read_pos = increm_read_or_write_pos(this.read_pos) ;

      this.nb_obj_locked_for_write--;

      notify();
      return objs[rd];
   }

   /**
    Unlock the read-object. This function must NOT be 'synchronized',
    otherwise, we will create a 'dead-lock' situation.

    @return    ref to the object.
   */
   public void unlock_read_obj(CrcBufDataObjIntface obj)
   {
      obj.unlock();
   }

   /**
    Unlock the write-object. This function must NOT be 'synchronized',
    otherwise, we will create a 'dead-lock' situation.

    @return    ref to the object.
   */
   public void unlock_write_obj(CrcBufDataObjIntface obj)
   {
      obj.unlock();
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









