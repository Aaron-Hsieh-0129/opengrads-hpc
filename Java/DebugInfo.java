/*
  DebugInfo class:

  This class contains API to print debugging information/error message.

  Caller can set the debugging levels (none, warning, error, infomation,
  or all) to control the amount information print out.

  @author Pedro T.H. Tsai
  @version 1.0
  @date  2008 Dec 27
*/

public class DebugInfo
{
   // Debug level control bit mask
   final static int DBG_NONE     = 0 ;
   final static int DBG_ERROR    = 0x01 ;
   final static int DBG_INFO     = 0x02 ;
   final static int DBG_WARNING  = 0x04 ;
   final static int DBG_API      = 0x08 ;
   final static int DBG_ALL      = (DBG_ERROR | DBG_INFO |
                                    DBG_WARNING | DBG_API );

   /* Initial debug print level */
   private int dbg_level=DBG_NONE;

   private String prefix=null;

   /*
      Print out the thread information (e.g., thread name)
   */
   private boolean enable_thread_info = false;

   /* Print out timing info. */
   private boolean enable_timing = false;

   public  DebugInfo()  { return; }

   public  DebugInfo(int lvl)
   {
      this.dbg_level=lvl;
      return;
   }

   public  DebugInfo(String prfx_str, int lvl)
   {
      this.prefix=prfx_str;
      this.dbg_level=lvl;
      return;
   }

   public  DebugInfo(String prfx_str)
   {
      this.prefix=prfx_str;
      return;
   }

   public void setPrefixString(String prfx_str)
   {
      this.prefix=prfx_str;
      return;
   }

   public void setDebugLevel(int lvl)
   {
      dbg_level=lvl;
      return;
   }

   public String get_sys_time()
   {
      String  tm = null;
      if ( enable_timing )
      {
         tm="[systime: "+System.currentTimeMillis()+"]";
      }
      return tm;
   }

   public void show_systime(boolean flag)
   {
      enable_timing = flag;
   }

   public String get_thread_info()
   {
      String nx=null;
      if ( this.enable_thread_info )
      {
         nx="<Thread: "+Thread.currentThread().getName()+">";
      }
      return nx;
   }

   public void show_thread_info(boolean flag)
   {
      this.enable_thread_info = flag;
   }

   public void puts(String txt)
   {
      String str ;

      if ( this.prefix != null )
      {
         str = this.prefix + "::" + txt ;
      }
      else
      {
         str = txt ;
      }

      if ( enable_timing )
      {
          str = str + " " + get_sys_time();
      }
      if ( enable_thread_info )
      {
          str = str + " " + get_thread_info();
      }

      System.out.println( str );

   }

   public void ERROR(String txt)
   {
       if ( (this.dbg_level & DBG_ERROR) > 0 )
       {
           puts(txt);
       }
       return;
   }

   public void INFO(String txt)
   {
       if ( (this.dbg_level & DBG_INFO) > 0 )
       {
           puts(txt);
       }
       return;
   }

   public void WARNING(String txt)
   {
       if ( (this.dbg_level & DBG_WARNING) > 0 )
       {
           puts(txt);
       }
       return;
   }

   public void API(String txt)
   {
       if ( (this.dbg_level & DBG_API) > 0 )
       {
           puts(txt);
       }
       return;
   }

   public void ASSERT(boolean x, String txt)
   {
      if ( x==false)
      {
         puts(txt);
         System.exit(1);
      }
   }

  /* Test program */
  public static void main(String [] args) throws Exception
  {
      DebugInfo x = new DebugInfo();
      x.setDebugLevel(DBG_ALL);
      x.INFO("info data");
      x.ERROR("error msg");
      x.WARNING("warning msg");
      x.setDebugLevel(DBG_ERROR);

      x.INFO("infor data");
      x.ERROR("only see print error.");

      x.show_systime(true);
      x.ERROR("only see print error and time");

      x.show_thread_info(true);
      x.ERROR("only see print error and time and thread information");


  }
}


