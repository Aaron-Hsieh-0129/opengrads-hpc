/**
  MsgQ data buffer implementation for storing String data.

  Note: This class just extends MsgQStrBufDataBuf, the only difference
  is that it replaces copyin() method with its own implementation,
  where it casts the 'pv_data' object to Java String.

 @author Pedro T.H. Tsai
 @version 1.0
 @Date 2009 Jan 27
*/

public class MsgQStringDataBuf extends MsgQStrBufDataBuf implements  MsgQDataBufIntface
{
   public MsgQStringDataBuf(int num_ary, int ary_size)
   {
      super(num_ary, ary_size);
   }

   /**
    Copy data into storage buffer from a caller provided space
    @param write_pos  An index position in the storage buffer to write data.
    @param pv_data    The data object to be copied into storage buffer.
    @param data_size  Size of the data in 'pv_data'.
   */
   public int copyin(int write_pos, Object pv_data, int data_size)
   {
      /* cast pv_data to String type. */
      String sb=(String)pv_data;

      /* check the length of input data. */
      if ( data_size > super.arrays[write_pos].length )
      {
         data_size = super.arrays[write_pos].length;
         System.out.println("[Warning] StrBufAry::copyin() operation: the input data is too long for databuffer, so it trimmed to fix in the available space.");
      }

      sb.getChars(0, data_size, super.arrays[write_pos], 0);
      super.valid_data_size[write_pos]=data_size;

      /* return the number of primitive data values copied in. */
      return data_size;

   }

   /**
    Usage example and Self-test function.
    @parm
   */
   public static void main(String[] args) throws Exception
   {
      StringBuffer sb=new StringBuffer(128);
      int rc;


      MsgQStringDataBuf  sbary=new MsgQStringDataBuf(16, 64);
      String xx=new String("Hellow World !! This is 2009 year ");

      rc=sbary.copyin(0, xx, xx.length());
      System.out.println("num of values copied in: "+rc);

      sbary.copyout(0, sb, sb.capacity());
      System.out.println("num of values copied out: "+rc);

      System.out.println("strbuf len: "+ sb.length()+
                         " capacity: "+ sb.capacity()+
                         " contents: "+ sb.toString());

   }

}










