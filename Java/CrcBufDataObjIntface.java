/**
  Interface for data objects stored/managed by in circular buffer list
  manager.

 @author Pedro T.H. Tsai
 @version 1.0
 @Date 2009 Feb 20
*/

public interface CrcBufDataObjIntface
{
   /**
    Lock this data object for read or write operation.
    @return  status code
   */
   public int lock();

   /**
    Unlock this data object for read or write operation.
    @return   status code
   */
   public int unlock();
}










