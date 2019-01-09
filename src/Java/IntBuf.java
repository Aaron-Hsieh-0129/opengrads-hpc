/**
  Integer buffer class for storing GrADS graphics command and
  its parameter data.

 @author Pedro T.H. Tsai
 @version 1.0
 @Date 2009 Jan 27
*/

public class IntBuf implements CrcBufDataObjIntface
{
   private Lock  slock = null;

   final static int DEFAULT_INIT_SIZE = 1024*64;

   /* Debug info object for this class. */
   private final static DebugInfo dbg = new DebugInfo("IntBuf");

   /* Initialization block for static variables. */
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

   private int[]    ai;
   private int      length;
   private int      nb_cmd;
   private int      capacity;

   public void info()
   {
      int avail = this.capacity - this.length ;

      dbg.INFO("capacity: "+this.capacity+
               " used: "+this.length+
               " num of cmds: "+this.nb_cmd+
               " space available: "+avail
         );
   }

   private int realloc()
   {
      int new_capacity = this.capacity * 2;
      int[] new_ai = new int[new_capacity];
      System.arraycopy(this.ai,0,new_ai,0,this.length);
      this.ai=new_ai;
      this.capacity=new_capacity;
      dbg.INFO("Increase internal array size: new capacity: "+new_capacity);
      return new_capacity;
   }

   public IntBuf()
   {
      init(DEFAULT_INIT_SIZE);
   }

   public IntBuf(int capacity)
   {
      init(capacity);
   }

   /**
    Create an array of integer arrays to store graphics draw commands
    and its parameters.

    @param capacity  The maximum number of integer array in this data buffer.

   */
   public void init(int sz)
   {
      this.capacity = sz;
      this.length = 0;
      this.nb_cmd = 0;
      /* Note: Java initializes array element value to 0 by default. */
      this.ai = new int[capacity];

      this.slock = new Lock();

   }

   public int length()
   {
      return this.length;
   }

   public int[] get_int_array()
   {
      return this.ai;
   }

   public void reset()
   {
      this.length = 0;
      this.nb_cmd = 0;
   }

   /**
    Copy data into gfx draw command buffer.
    @param draw_code  gfx drawing command.

    @return           Number of integer data item copied in.
   */
   public int add(int draw_code)
   {
      int  rc = 1;
      if ( (this.length+rc) <= capacity )
      {
         this.ai[this.length]=draw_code;
         this.length++;

         this.nb_cmd++;
      }
      else
      {
         realloc();
         rc=add(draw_code);
      }

      /* return the number of data values copied in. */
      return rc;
   }

   /**
    Copy data into gfx draw command buffer.
    @param draw_code  gfx drawing command.
    @param a

    @return           Number of integer data item copied in.
   */
   public int add(int draw_code, int a)
   {
      int  rc = 2;
      if ( (this.length+rc) <= capacity )
      {
         this.ai[this.length]=draw_code;
         this.length++;
         this.ai[this.length]=a;
         this.length++;

         this.nb_cmd++;
      }
      else
      {
         realloc();
         rc=add(draw_code,a);
      }

      /* return the number of data values copied in. */
      return rc;
   }

   /**
    Copy data into gfx draw command buffer.
    @param draw_code  gfx drawing command.
    @param a
    @param b
    @return           Number of integer data item copied in.
   */
   public int add(int draw_code, int a, int b)
   {
      int  rc = 3;
      if ( (this.length+rc) <= capacity )
      {
         this.ai[this.length]=draw_code;
         this.length++;
         this.ai[this.length]=a;
         this.length++;
         this.ai[this.length]=b;
         this.length++;

         this.nb_cmd++;
      }
      else
      {
         realloc();
         rc=add(draw_code,a,b);
      }

      /* return the number of data values copied in. */
      return rc;
   }

   /**
    Copy data into gfx draw command buffer.
    @param draw_code  gfx drawing command.
    @param a
    @param b
    @param c
    @param d
    @return           Number of integer data item copied in.
   */
   public int add(int draw_code, int a, int b, int c, int d)
   {
      int  rc = 5;
      if ( (this.length+rc) <= capacity )
      {
         this.ai[this.length]=draw_code;
         this.length++;
         this.ai[this.length]=a;
         this.length++;
         this.ai[this.length]=b;
         this.length++;
         this.ai[this.length]=c;
         this.length++;
         this.ai[this.length]=d;
         this.length++;

         this.nb_cmd++;
      }
      else
      {
         realloc();
         rc=add(draw_code,a,b,c,d);
      }

      /* return the number of data values copied in. */
      return rc;
   }

   /**
    Lock this data object for read or write operation.
    @return  status code
   */
   public int lock()
   {
      return this.slock.lock();
   }

   /**
    Unlock this data object for read or write operation.
    @return   status code
   */
   public int unlock()
   {
      return this.slock.unlock();
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

   }
}








