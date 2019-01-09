/**
  MsgQ Data buffer for storing integer data.

 @author Pedro T.H. Tsai
 @version 1.0
 @Date 2009 Jan 27
*/

public class MsgQIntDataBuf implements MsgQDataBufIntface
{
   private int[][]  arrays;
   private int[]    valid_data_size;

   /**
    Create an array of integer arrays to store integer data.

    @param num_ary  The maximum number of integer array in this data buffer.
    @param ary_size The maximum number of integer values that can be
                    stored into individual array.  Note: all
                    integer arrays has the same maximum capacity.
   */
   public MsgQIntDataBuf(int num_ary, int ary_size)
   {
      /* Note: Java initializes array element value to 0 by default. */
      arrays=new int[num_ary][ary_size];
      valid_data_size=new int[num_ary];
   }

   /**
    Copy data into storage buffer from a caller provided space
    @param write_pos  An index position in the storage buffer to write data.
    @param pv_data    The data object to be copied into storage buffer.
    @param data_size  Number of the data item in 'pv_data' to be copied
                      into this data buffer.
    @return           Number of data item copied in.
   */
   public int copyin(int write_pos, Object pv_data, int data_size)
   {
      /* cast pv_data to integer array type. */
      int[] ai_data = (int[]) pv_data;

      if ( data_size > arrays[write_pos].length )
      {
         data_size = arrays[write_pos].length;
         System.out.println("[Warning] MsgQIntDataBuf copyin() operation: the input data is to long for databuffer, so it trimmed to fix in the available space.");
      }

      System.arraycopy(ai_data, 0, arrays[write_pos], 0, data_size);
      valid_data_size[write_pos]=data_size;

      /* return the number of primitive data values copied in. */
      return data_size;
   }

   /**
    Copy data from storage buffer into caller provided space.
    @param read_pos   An index position in the storge buffer to read data.
    @param pv_data    Reference to a data object to copy data into from
                      the storage buffer.
    @param data_size  Capacity (in number of data item) that can be stored
                      in 'pv_data' object.
    @return           Number of data items copied out.
   */
   public int copyout(int read_pos, Object pv_data, int data_size)
   {
      /* cast pv_data to integer array type. */
      int[] ai_data = (int[]) pv_data;
      int num_data_val = valid_data_size[read_pos];

      if ( num_data_val > data_size )
      {
         num_data_val = data_size;

         System.out.println("[Warning] MsgQIntDataBuf copyout() operation: the caller provided data buffer is too small to hold all the data, so it trimmed to fix in the available space.");
      }

      System.arraycopy(arrays[read_pos], 0, ai_data, 0, num_data_val);

      /* return the number of primitive data values copied out. */
      return num_data_val;
   }

   /**
    Usage example and Self-test function.
    @parm
   */
   public static void main(String[] args) throws Exception
   {
      int[] values = { 1, 2, 3, 4, 5 };
      int len = values.length;
      int rc;


      MsgQIntDataBuf   ai2d = new MsgQIntDataBuf(16,10);

      System.out.println("len= "+len);
      rc=ai2d.copyin(0, values,len);
      System.out.println("num of values copied in: "+rc);

      int[] out=new int[10];
      len = out.length;

      rc=ai2d.copyout(0, out, len);
      System.out.println("num of values copied out: "+rc);
      for (int i=0; i < rc; i++ )
      {
         System.out.print(out[i]+" ");
      }
      System.out.println("");

   }
}








