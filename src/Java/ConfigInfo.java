/**
  Parse and store command line argument in a table, and API for
  lookup for a specific argument parameter and its value.

 @author Pedro T.H. Tsai
 @version 1.0
*/

import java.util.*;

public class ConfigInfo {

   /* Table to store name/value */
   final Hashtable<String,String> table;

   public ConfigInfo()
   {
       this.table = new Hashtable<String,String>(8);
   }

   /**
    Process GrADS command line strings.
    @param args array of option name and its value.

   */
   final public void load_grads_args(String[]  args)
   {
      // System.out.println("command line args: ");
      for (int i=0; i < args.length; i++ )
      {
          if ( args[i].startsWith("-") )
          {
              String   key;
              String   value;

              /* Get the name of option: which is the substring
                 starts after '-' and ends with '='.
              */
              if ( args[i].equals("-g") )
              {
                  /* This option specifies the window frame size
                     and location.

                     Format is:  LLLLxHHHH+XXXX+YYYY

                     store the optional value in the table.
                     The name of this option is 'win_frame_dim'
                  */
                  i++;
                  /* If the window size not specified, then
                     this is an error. */
                  if ( i < args.length )
                  {
                     this.table.put("win_frame_dim",args[i]);
                  }
                  else
                  {
                     System.out.println(
                         "Argument error: -g must be follow by window size specification.");
                      continue;
                  }
              }

              /* For now, ignore all other options. */
          }
      }
      return;
   }

   /**
    Process command line strings.
    @param args array of string name and its value.
   */
   final public void load(String[]  args)
   {
      /*
      System.out.println("command line args: ");
      for (int i=0; i < args.length; i++ )
      {
         System.out.println(args[i]);
      }
      */

      // System.out.println("command line args: ");
      for (int i=0; i < args.length; i++ )
      {
          if ( args[i].startsWith("-") )
          {
              String   key;
              String   value;
              int      start;
              int      end;

              /* Get the name of option: which is the substring
                 starts after '-' and ends with '='.
              */
              start=1;
              end=args[i].indexOf('=');
              if ( end != -1 )
              {
                  //  System.out.println("start= "+start+" end= "+end);
                  key=args[i].substring(start,end);
                  value=args[i].substring(end+1,args[i].length());
                  // System.out.println("key= "+key+" value= "+value);
                  this.table.put(key,value);

              }
              else
              {
                  /* argument with -xxxx but without '=', treat this
                     as a series of flag
                  */
              }
          }
          else
          {
             /* argument without '-', so this just input tokens
                to the program. */
          }

      }
      return;
   }

   /**
    Return true or false if the specified name is stored in the
    Table.
    @param name Name of key.
   */
   final public boolean hasName(String name)
   {
       return this.table.containsKey(name);
   }

   /**
    Get the unsigned integer value of 'name' parameter.
    @param name
   */
   public int getInt(String name)
   {
       String value ;
       int    int_value ;

       value = this.table.get(name);

       try {
           int_value=Integer.parseInt(value,10);
       }
       catch (NumberFormatException e)
       {
           int_value = 0;
           System.out.println("Format error: expect '"+name+"' value to be integer type.");
       }
       return int_value;
   }

   /**
    Get the array of integer values for the specified 'name' option.
    @param name
   */
   public int[] getIntAry(String name)
   {
       String str ;
       int[]  dim = null ;
       int    start;
       int    end;

       try {
           if ( name.equals("win_frame_dim") == true )
           {
               /* Is this option name defined in the table. */
               str = this.table.get(name);
               if ( str == null )
               {
                  return null;
               }

               String  vstr;
               dim=new int[4];
               dim[0]=dim[1]=dim[2]=dim[3]=0;
               start = 0;

               end = str.indexOf("x",start);
               if ( end >= 0 )
               {
                   vstr=str.substring(start,end);
                   dim[0] = Integer.parseInt(vstr,10);
                   start = end+1;
                   end=str.indexOf("+",start);
                   if ( end >= 0 )
                   {
                      vstr=str.substring(start,end);
                      dim[1] = Integer.parseInt(vstr,10);

                      start = end+1;
                      end=str.indexOf("+",start);
                      if ( end >= 0 )
                      {
                         vstr=str.substring(start,end);
                         dim[2] = Integer.parseInt(vstr,10);

                         start = end+1;
                         end = str.length();
                         vstr=str.substring(start,end);
                         dim[3] = Integer.parseInt(vstr,10);
                      }
                   }
                   else
                   {
                      vstr=str.substring(start,str.length());
                      dim[1] = Integer.parseInt(vstr,10);
                   }
               }
               else
               {
                  dim = null;
               }
           }
       }
       catch (NumberFormatException e)
       {
           dim=null;
           System.out.println("Format error: expect '"+name+"' value to be integer type.");
       }
       return dim;
   }

   /**
    Self-test function.
    @param name
   */
   public static void main(String[] args) throws Exception
   {
       ConfigInfo   config=new ConfigInfo();
       config.load(args);

       for (int i=0; i< args.length; i++ )
       {
           int      start;
           int      end;
           String   key;

           /* Get the name of option: which is the substring
              starts after '-' and ends with '='.
           */
           start=1;
           end=args[i].indexOf('=');
           if ( end < 0 )
           {
              break;
           }
           key=args[i].substring(start,end);

           if ( config.hasName(key) )
           {
              int value = config.getInt(key);
              System.out.println("<"+key+">  value= "+value);
           }
       }

       ConfigInfo  grads_config=new ConfigInfo();
       grads_config.load_grads_args(args);
       int dim[];
       dim=grads_config.getIntAry("win_frame_dim");
       if ( dim != null )
       {
           for ( int i=0 ; i < dim.length; i++ )
           {
              System.out.print(dim[i]+":");
           }
           System.out.println(" ");
       }
       return;
   }
}









