/**
  This class implements a circular data storage buffer.  The data I/O are
  First In/Firt Out (FIFO).

 @author Pedro T.H. Tsai
 @version 1.0
 @Date 2009 Jan 25
*/

import java.util.*;

public class MsgQ extends Thread
{

   private  int              payload_type;
   private  int              max_num_msg;

   private  int              msg_count;
   private  int              read_pos;
   private  int              write_pos;

   private  MsgQDataBufIntface   data_buf;

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

   /* enumerated constants for MsgQ class. */
   static final int PAYLOAD_TYPE_INT          = 1 ;
   static final int PAYLOAD_TYPE_STRING       = 2;
   static final int PAYLOAD_TYPE_STRINGBUFFER = 3;

   /**
    Constructor of message queue.

    @param max_num_msg   Maximum number of message in this queue.
    @param payload_type  The type of payload in the message.  Currently,
                         For primitive type, we only support int and byte.
                         For object type, we only support Stringbuffer type.

    @param payload_size  The size of payload:
                         For 'int' type:  this is max number of int in the
                         payload .
                         For 'String' type: this is max number of 'char' in the
                         payload data buffer.
                         For 'StringBuffer', this is max number of 'char'
                         for the StringBuffer.

   */
   public MsgQ(int max_num_msg, int payload_type, int payload_size )
   {
      this.payload_type = payload_type;
      this.max_num_msg = max_num_msg;
      this.read_pos    = 0;
      this.write_pos   = 0;
      this.msg_count   = 0;
      switch (payload_type)
      {
         case MsgQ.PAYLOAD_TYPE_INT:
         {
            this.data_buf = new MsgQIntDataBuf(max_num_msg, payload_size);
         }
         break;

         case MsgQ.PAYLOAD_TYPE_STRING :
         {
            this.data_buf = new MsgQStringDataBuf(max_num_msg, payload_size);
         }
         break;

         case MsgQ.PAYLOAD_TYPE_STRINGBUFFER:
         {
            this.data_buf = new MsgQStrBufDataBuf(max_num_msg, payload_size);
         }
         break;

         default:
         {
         }
         break;
      }
   }

   private int increm_read_or_write_pos(int pos)
   {
      if ( (pos + 1) < this.max_num_msg )
      {
         pos = pos + 1;
      }
      else
      {
         pos = 0;
      }
      return pos;
   }

   public int send(Object data_obj, int payload_size)
   {
      int  item_copied = 0;

      synchronized (this)
      {
         if ( (this.msg_count+1) <= this.max_num_msg )
         {
            item_copied=this.data_buf.copyin(this.write_pos,
                                             data_obj,
                                             payload_size);

            this.write_pos   = increm_read_or_write_pos(this.write_pos);
            this.msg_count   = this.msg_count + 1;
            notify();
         }
      }

      return item_copied;
   }

   public int recv(Object data_obj, int data_obj_capacity)
   {
      int  item_copied = 0;

      synchronized (this)
      {
         while ( this.msg_count == 0 )
         {
            try {
               wait();
            }
            catch (Exception e)
            {
               System.out.println(e);
            }
         }

         item_copied=this.data_buf.copyout(this.read_pos,
                                           data_obj,
                                           data_obj_capacity);

         this.read_pos   = increm_read_or_write_pos(this.read_pos);
         this.msg_count   = this.msg_count - 1;
      }

      return item_copied;
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









