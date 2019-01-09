/**
  MsgQ data buffer implementation for storing Java StringBuffer data.

  Internally, we will store Java String data in 'char' array

 @author Pedro T.H. Tsai
 @version 1.0
 @Date 2009 Jan 27
*/

public class MsgQStrBufDataBuf implements MsgQDataBufIntface
{
   /* We made these protected fields, because later when
      we create MsgQStringDataBuf class, which extends this
      class,  we need to access the internal fields below
      from MsgQStringDataBuf class.
   */
   protected char[][]  arrays;
   protected int[]     valid_data_size;

   /**
    Create an array of char arrays to store String data.

    @param num_ary  The maximum number of String in this data buffer.
    @param ary_size The maximum number of 'char' that can be
                    stored for String object.  Note:  maximum capacity
                    is set to the same for all String.
   */
   public MsgQStrBufDataBuf(int num_ary, int ary_size)
   {
      /* Note: Java initializes array to 0 by default. */
      arrays=new char[num_ary][ary_size];
      valid_data_size = new int[ary_size];
   }

   /**
    Copy data into storage buffer from a caller provided space
    @param write_pos  An index position in the storage buffer to write data.
    @param pv_data    The data object to be copied into storage buffer.
    @param data_size  Size of the data in 'pv_data'.
    @return           Number of data item copied in.
   */
   public int copyin(int write_pos, Object pv_data, int data_size)
   {
      /* cast pv_data to StringBuffer type. */
      StringBuffer sb=(StringBuffer)pv_data;

      /* check the length of input data. */
      if ( data_size > arrays[write_pos].length )
      {
         data_size = arrays[write_pos].length;
         System.out.println("[Warning] MsgQStrBufDataBuf::copyin() operation: the input data is to long for databuffer, so it trimmed to fix in the available space.");
      }

      sb.getChars(0, data_size, arrays[write_pos], 0);
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
                      in 'pv_data'.
    @return           Number of data item copied out.
   */
   public int copyout(int read_pos, Object pv_data, int data_size)
   {
      /* cast pv_data to StringBuffer type. */
      StringBuffer sb = (StringBuffer) pv_data;
      int num_data_val = valid_data_size[read_pos];

      if ( num_data_val > data_size )
      {
         num_data_val = data_size;

         System.out.println("[Warning] MsgQStrBufDataBuf::copyout() operation: the caller provided data buffer is too small to hold all the data, so it trimmed to fix in the available space.");
      }
      sb.setLength(0);
      sb.append(arrays[read_pos], 0, num_data_val);

      /* return the number of primitive data values copied out. */
      return num_data_val;

   }

   /**
    Usage example and Self-test function.
    @parm
   */
   public static void main(String[] args) throws Exception
   {
      StringBuffer sb=new StringBuffer(128);
      int rc;

      System.out.println("strbuf len: "+ sb.length()+
                         " capacity: "+ sb.capacity()+
                         " contents: "+ sb.toString());

      if ( ! (args.length > 0) )
      {
         System.out.println("Usage: need at least one argument string, try java MsgQStrBufDataBuf  hello.");
         System.exit(0);
      }

      sb.append(args[0]);
      System.out.println("strbuf len: "+ sb.length()+
                         " capacity: "+ sb.capacity()+
                         " contents: "+ sb.toString());

      MsgQStrBufDataBuf  sbary=new MsgQStrBufDataBuf(16, 12);
      StringBuffer xx=new StringBuffer("Hellow World !! ");

      rc=sbary.copyin(0, xx, xx.length());
      System.out.println("num of values copied in: "+rc);

      rc=sbary.copyout(0, sb, sb.capacity());
      System.out.println("num of values copied out: "+rc);

      System.out.println("strbuf len: "+ sb.length()+
                         " capacity: "+ sb.capacity()+
                         " contents: "+ sb.toString());


      char[] chars = args[0].toCharArray();

      int len = chars.length;

      System.out.println("len= "+len);

      for (int i=0; i < len; i++ )
      {
         System.out.print(chars[i]+" ");
      }
      System.out.println("");


      System.out.println("create a char array...");
      char[] schar=new char[7];
      len=schar.length;
      System.out.println("the capacity of char array: "+len);


      System.out.println("get the char array from string buffer.");
      args[0].getChars(0,args[0].length(),schar,0);


      System.out.println("print the chars array...");
      for (int i=0; i < chars.length; i++ )
      {
         System.out.print(chars[i]+" ");
      }
      System.out.println("");

      byte one_byte;

      one_byte=(byte) chars[0];
      System.out.println("byte one = "+one_byte);


      byte[] xbytes;

      xbytes=args[0].getBytes();
      System.out.println("print the byte array...len="+xbytes.length);
      for (int i=0; i < xbytes.length; i++ )
      {
         System.out.print(xbytes[i]+" ");
      }
      System.out.println("");


   }

}










