/**
  Interface for Message Queue data buffer.

 @author Pedro T.H. Tsai
 @version 1.0
 @Date 2009 Jan 27
*/

public interface MsgQDataBufIntface
{
   /**
    Copy data into storage buffer from a caller provided space
    @param write_pos  An index position in the storage buffer to write data.
    @param pv_data    The data object to be copied into storage buffer.
    @param data_size  Size of data in 'pv_data', given in number of data items.
    @return           Number of data item copied in.
   */
   public int copyin(int write_pos, Object pv_data, int data_size);

   /**
    Copy data from storage buffer into caller provided space.
    @param read_pos   An index position in the storge buffer to read data.
    @param pv_data    Reference to a data object to copy data into from
                      the storage buffer.
    @param data_size  Capacity (in number of data item) that can be stored
                      in 'pv_data'.
    @return           Number of data item copied out.
   */
   public int copyout(int read_pos, Object pv_data, int data_size);
}










