/*
 This API sends/receives the primitive data (int, byte, char, etc) to
 another process via Network socket API.  Another process can be
 on another host or on the same host.

 @author  Pedro T.H. Tsai,
 @version 1.0
 @date    Jan 30, 2009

*/

import java.net.*;
import java.io.*;
import java.util.*;

public class SockIPC
{
   /* communication protocol versio id. */
   static final int     VER_ID = (1 << 28);

   /* opcode for the request. */
   static final int     GRADS_UNKNOWN_REQUEST = (VER_ID | 0);
   static final int     GRADS_CMD_REQUEST     = (VER_ID | 1);
   static final int     GRADS_GX_DRAW_REQUEST = (VER_ID | 2);

   /* constants for SOCKET communication. */
   static final int     VMGRADS_IP_PORT    = 6789 ;
   static final int     VMGRADS_IP_PORT_GX = 6790 ;

   static final int     CMD_BUF_SIZE       = 256 ; /* Max size (num of chars)
                                                      of command buffer to
                                                      send/recv. */
   static final int     GFX_BUF_SIZE       = 4096 ; /* Max size of graphics
                                                       buffer to send/recv. */

   static final String  VMGRADS_IP_ADDR    = "127.0.0.1";

   boolean   is_connected=false;
   Socket    dest_socket=null;
   Socket    data_socket=null;
   String    dest_host=null;
   int       dest_port=0;

   /* TCP timeout. */
   int       timeout = 5000;

   // I/O streams
   // BufferedOutputStream sink;
   DataOutputStream sink = null;
   BufferedInputStream source = null;

   // For the data socket
   BufferedOutputStream data_sink = null;
   BufferedInputStream  data_source = null;

   /* working buffer for bytes, chars, and integer data */
   int      BYTE_BUF_SIZE = 512;
   byte[]   byte_data = null;
   char[]   chars_buf=null;

   /* buffer to store integer value. */
   byte[]   byte_buf_for_int_val = null;

   /* Debug print info object for this class. */
   private final static DebugInfo dbg = new DebugInfo("SockIPC");

   /* Initialization block for static variables. */
   static
   {
      /* enable debug print and additional information such as
         current system time and thread information to be appended
         in the print statements.
      */
      dbg.setDebugLevel(DebugInfo.DBG_INFO | DebugInfo.DBG_ERROR);
      dbg.show_systime(true);
      dbg.show_thread_info(true);
   }

   /**
    Create a Socket object for IPC operation.

    @param: data_size specifies the maximum size of data for a send_data
            command.
   */
   public SockIPC(int max_payload_data_size)
   {
      alloc_work_buf(max_payload_data_size);
   }

   public SockIPC(Socket  sock, int max_payload_data_size)
   {
      alloc_work_buf(max_payload_data_size);
      create_iostream_from_socket(sock);
   }

   private int alloc_work_buf(int max_payload_data_size)
   {
      this.BYTE_BUF_SIZE = max_payload_data_size;
      this.byte_data = new byte[this.BYTE_BUF_SIZE];
      this.chars_buf=new char[this.BYTE_BUF_SIZE];
      this.byte_buf_for_int_val = new byte[4];

      return 1;
   }

   private int create_iostream_from_socket(Socket sock)
   {
      this.dest_socket = sock;
      dbg.INFO("Remote IP Addr: "+this.dest_socket.getInetAddress()+
               " Remote Port: "+this.dest_socket.getPort());
      try
      {
         this.sink=new DataOutputStream(
            this.dest_socket.getOutputStream());

         this.source=new BufferedInputStream(
            this.dest_socket.getInputStream());

         this.is_connected=true;
      }
      catch (IOException e)
      {
         System.out.println("create IO Stream from socket connection failed.");
         System.out.println(e);
         return -1;
      }
      return 1;
   }

   /*
     Make the connection to the server and create I/O streams

     @param dest_host  An hostname or IP address for the remote destination.
     @param dest_port  Port number on the remote destination.
   */
   public int connect(String dest_host, int dest_port)
   {
      int  rc=-1;
      this.dest_host = dest_host;
      this.dest_port = dest_port;

      dbg.INFO("Trying connection to host: "+this.dest_host+
               ", using Port Number: "+this.dest_port);

      try
      {
         Socket sock = new Socket(this.dest_host,this.dest_port);
         dbg.INFO("Connection established.");
         rc=create_iostream_from_socket(sock);
      }
      catch (IOException e)
      {
         System.out.println("SockIPC::connect() failed: "+
                            "ipaddr: "+this.dest_host+
                            " port: "+this.dest_port);
         System.out.println(e);
         return -1;
      }
      return rc;
   }

   /*
     Handle IOException from socket I/O operation,  then pass the exception
     to upper layer so application can decide what to do.

     @param e  IOException
   */
   private void handle_exception(IOException e) throws IOException
   {
      System.out.println(e);
      this.is_connected=false;
      throw e;
   }

   public int send_ga_cmd(int opcode, StringBuffer s_cmd)
      throws IOException
   {
      s_cmd.getChars(0, s_cmd.length(), this.chars_buf, 0);
      return send_ga_cmd(opcode, this.chars_buf, s_cmd.length());
   }

   /**
   Send an array of char data to destination host.

   @param  ac_data  Array of char data to be sent.
   @param  len_ac_data  The number of item in 'ac_data' array to sent.
   @return The number of item sent.
   */
   public int send_ga_cmd(int opcode, char[] ac_data, int len_ac_data)
      throws IOException
   {
      if ( this.is_connected == true && this.sink != null )
      {
         try
         {
            if ( BYTE_BUF_SIZE < len_ac_data )
            {
               dbg.ERROR("send_ga_cmd(), internal buffer is too small "+
                         "[buffer size: ]"+BYTE_BUF_SIZE+" Input data size: "+
                         len_ac_data);

               len_ac_data = BYTE_BUF_SIZE;
            }

            /*
              Convert the char[] data to byte data[],
              this means that we will drop upper 8 bites of UNICODE
              char, and only support LATIN 1/ASCII char set.
            */
            for ( int i=0; i < len_ac_data; i++ )
            {
               this.byte_data[i]=(byte) ac_data[i];
            }

            this.sink.writeInt(opcode);
            this.sink.writeInt(len_ac_data);
            this.sink.write(this.byte_data, 0, len_ac_data);
            this.sink.flush();
            dbg.API("send_ga_cmd() write "+len_ac_data+" bytes");
         }
         catch (IOException e)
         {
            handle_exception(e);
         }
      }
      else
      {
         dbg.WARNING("SockIPC: send_data() failed: no connection.");
         return -1;
      }
      return len_ac_data;
   }



   /**
   Send an array of int data to destination host.

   @param  ai_data  Array of int data to be sent.
   @param  data_len  The number of item in 'ai_data' array to sent.
   @return The number of item sent.
   */
   public int send_gfx_cmd(int opcode, int[] ai_data, int data_len)
      throws IOException
   {
      if ( this.is_connected == true && this.sink != null )
      {
         try
         {
            this.sink.writeInt(opcode);
            this.sink.writeInt(data_len);
            this.sink.flush();
            dbg.API("send_gfx_cmd() write ints : opcode: "+
                     opcode+
                     " data_len: "+data_len+
                     " total wrote [bytes] "+this.sink.size()
               );
            /*
              convert integer data to byte data
            */
            int buf_size = (int)(this.BYTE_BUF_SIZE/4);
            int start = 0;
            int remainder = data_len;
            int total_byte_sent = 0;
            int nb_per_sock_write = 0;
            do
            {
               int j=0;

               for (int i=start ; i < (start+buf_size) ; i++ )
               {
                  if ( i < data_len )
                  {
                     this.byte_data[j]= (byte) ((ai_data[i] >> 24) & 0x000000FF);
                     j++;
                     this.byte_data[j]= (byte) ((ai_data[i] >> 16) & 0x000000FF);
                     j++;
                     this.byte_data[j]= (byte) ((ai_data[i] >>  8) & 0x000000FF);
                     j++;
                     this.byte_data[j]= (byte) (ai_data[i] & 0x000000FF);
                     j++;
                  }
                  else
                  {
                     break;
                  }
               }
               nb_per_sock_write = (int) (j / 4) ;

               this.sink.write(this.byte_data, 0, (nb_per_sock_write*4));
               this.sink.flush();

               start = start + nb_per_sock_write ;
               remainder = remainder - nb_per_sock_write;
               total_byte_sent = total_byte_sent + (nb_per_sock_write*4) ;
            }
            while ( remainder > 0 );

            dbg.API("send_gfx_cmd() write "+(data_len)+" int "+
                     total_byte_sent+ " bytes");
         }
         catch (IOException e)
         {
            handle_exception(e);
         }
      }
      else
      {
         dbg.WARNING("SockIPC: send_data() failed: no connection.");
         return -1;
      }
      return data_len;
   }

   /**
      Write a 4 byte integer value (big-endian) to
      the Socket connection.

      @return  nb integer value wrote.
   */
   public int write_int(int value) throws IOException
   {
      try
      {
         this.sink.writeInt(value);
      }
      catch (IOException e)
      {
         handle_exception(e);
      }
      return 1;
   }

   /**
      Read a 4 byte integer value (big-endian) from the Socket connection.

      @return  integer value read.
   */
   public int read_int() throws IOException
   {
      int bytes_read=0;
      int byte_to_read=4;
      int value=0;

      // bytes_read=this.source.read(byte_buf_for_int_val, 0, byte_to_read);
      bytes_read=receive_byte_data(byte_buf_for_int_val, byte_to_read);
      if ( byte_to_read == bytes_read )
      {
         /* convert bytes to integer value. Big-endian format*/
         value = ( (((int) byte_buf_for_int_val[0] )  & 0x000000FF ) << 24 ) +
                 ( (((int) byte_buf_for_int_val[1] )  & 0x000000FF ) << 16 ) +
                 ( (((int) byte_buf_for_int_val[2] )  & 0x000000FF ) <<  8 ) +
                 ( (((int) byte_buf_for_int_val[3] )  & 0x000000FF )       ) ;
      }
      else
      {
         dbg.ERROR("read_int() failed. byte_to_read ["+byte_to_read+
            " bytes] and received ["+bytes_read+" bytes]");
      }
      return value;
   }

   /**
     Read byte data received from socket

     @param  byte_buffer  buffer to receive byte data
     @item_to_read  number of byte to read.
   */
   public int receive_byte_data(byte[] byte_buffer, int item_to_read)
      throws IOException
   {
      int bytes_read = 0;
      int index = 0;
      int byte_to_read = 0 ;

      try
      {
         if ( item_to_read > byte_buffer.length )
         {
            dbg.ERROR("Can not receivied data, buffer is too small ["+
                      byte_buffer.length+"]"+
                      "number of input data item: "+item_to_read);
            return -1;
         }

         if ( item_to_read > 0 )
         {
            byte_to_read=item_to_read ;
            index=0;
            bytes_read=0;
            while(true)
            {
               bytes_read=source.read(byte_buffer,index,byte_to_read);
               if ( bytes_read == -1 )
               {
                  break;
               }
               else
               {
                  index=bytes_read+index;
                  byte_to_read=byte_to_read-bytes_read;
                  if (byte_to_read <=0 )
                  {
                     break;
                  }
               }
            }
            if ( index != item_to_read )
            {
               dbg.ERROR("The amount of data read: "+index+" bytes "+
                         "is not equal to the data packet size"+ item_to_read +
                         " bytes");
               return -1;
            }
         }
      }
      catch (IOException e)
      {
         handle_exception(e);
      }
      /* return the total item read. */
      return index ;
   }

   public int convert_byte_ary_to_int_ary
   (
      byte[] byte_ary,
      int src_start,
      int[]  int_ary,
      int dest_start,
      int nb
   )
   {
      int nb_int_conv = 0;
      int i = dest_start ;
      int j = src_start ;
      int val ;

      while ( nb_int_conv < nb )
      {
         /* convert bytes to integer value. Big-endian format*/
         int_ary[i] = ( (((int) byte_ary[j]  )  & 0x000000FF ) << 24 ) +
                      ( (((int) byte_ary[j+1])  & 0x000000FF ) << 16 ) +
                      ( (((int) byte_ary[j+2])  & 0x000000FF ) <<  8 ) +
                      ( (((int) byte_ary[j+3])  & 0x000000FF )       ) ;
         j = j+4;
         i++;
         nb_int_conv++;
      }
      return nb_int_conv;
   }


   public int convert_int_ary_to_byte_ary
   (
      int[]  int_ary,
      int src_start,
      byte[] byte_ary,
      int dest_start,
      int nb
   )
   {
      int j=dest_start;
      int i;

      for (i=src_start ; i < (src_start+nb) ; i++ )
      {
         byte_ary[j]= (byte) ((int_ary[i] >> 24) & 0xFF);
         j++;

         byte_ary[j]= (byte) ((int_ary[i] >> 16) & 0xFF);
         j++;

         byte_ary[j]= (byte) ((int_ary[i] >>  8) & 0xFF);
         j++;

         byte_ary[j]= (byte) (int_ary[i] & 0xFF);
         j++;
      }
      return i;
   }



   public int recv_ga_cmd(StringBuffer s_cmd) throws IOException
   {
      int opcode = GRADS_UNKNOWN_REQUEST;
      int nb_data_item = 0;
      int nb = -1;

      if ( connection_ok() == false )
      {
         dbg.WARNING("recv_ga_cmd() failed: no connection.");
         return -1;
      }

      try
      {
         opcode = read_int();
         nb_data_item=read_int();
         dbg.API("recv_ga_cmd() opcode= "+opcode+" number of data items = " + nb_data_item);

         if ( opcode == SockIPC.GRADS_CMD_REQUEST )
         {
            nb = receive_byte_data(this.byte_data, nb_data_item);

            /* convert the 8 bits byte data to 16 bits char[] data. */
            for (int i=0; i < nb ; i++ )
            {
               this.chars_buf[i]=(char) this.byte_data[i];
            }

            s_cmd.setLength(0);
            s_cmd.append(this.chars_buf,0,nb);

         }
      }
      catch (IOException e)
      {
         handle_exception(e);
      }
      return opcode;
   }

   /**
   Send an array of int data to destination host.

   @param  ai_data  Array of int data to receive gfx cmd data.
   @param  capacity The number of item in 'ai_data' array to sent.
   @return The number of item received.
   */
   public int recv_gfx_cmd(int[] ai_data, int capacity) throws IOException
   {
      int opcode = GRADS_UNKNOWN_REQUEST;
      int nb_data_item = 0;
      int nb = -1;
      int total_int_recv = 0;

      if ( connection_ok() == false )
      {
         dbg.WARNING("recv_gfx_cmd() failed: no connection.");
         return -1;
      }

      try
      {
         opcode = read_int();
         nb_data_item=read_int();

         dbg.API("recv_gfx_cmd() opcode= "+opcode+
                  " number of data items = " + nb_data_item);

         dbg.API("opcode: "+opcode+" "+GRADS_GX_DRAW_REQUEST);

         if ( opcode == GRADS_GX_DRAW_REQUEST )
         {
            if ( capacity >= nb_data_item )
            {
               /* compute nb of int read per buffer. */
               int nb_int_per_buf = (int)(this.BYTE_BUF_SIZE/4);

               /* nb of sock read to get all the data. */
               int nb_sock_read = (int) (nb_data_item / nb_int_per_buf );

               /* remainder of integer to read. */
               int remainder = nb_data_item - (nb_sock_read * nb_int_per_buf);

               int start = 0;
               int nb_byte_read;
               int nb_convert;

               for (int i=0; i < nb_sock_read; i++ )
               {
                  nb_byte_read =
                     receive_byte_data(this.byte_data, (nb_int_per_buf * 4));

                  if ( nb_byte_read == (nb_int_per_buf * 4) )
                  {
                     nb_convert = convert_byte_ary_to_int_ary(
                        this.byte_data,
                        0,
                        ai_data,
                        start,
                        nb_int_per_buf);

                     if ( nb_convert == nb_int_per_buf )
                     {
                        start = start + nb_convert ;
                     }
                     else
                     {
                        dbg.ERROR("Wrong number of byte converted to integer.");
                        return -1;
                     }
                  }
                  else
                  {
                     /* read sock error. */
                     dbg.ERROR("Wrong of number of bytes received.");
                     return -1;
                  }
               }

               nb_byte_read =
                  receive_byte_data(this.byte_data, (remainder * 4));

               nb_convert = convert_byte_ary_to_int_ary(
                  this.byte_data,
                  0,
                  ai_data,
                  start,
                  remainder);

               total_int_recv = start + nb_convert ;

               dbg.API("recv_gfx_cmd() total nb int read: "+total_int_recv+
                        " total input data "+nb_data_item);
            }
            else
            {
               dbg.ERROR("recv_gfx_cmd() input buffer ["+capacity+
                         "] is too small. input data is ["+
                         nb_data_item+" of int]");
               return -1;
            }
         }
         else
         {
            dbg.ERROR("recv_gfx_cmd() wrong opcode: expect ["+
                      GRADS_GX_DRAW_REQUEST+"] but get this value["+
                      opcode+"]");
            return -1;

         }
      }
      catch (IOException e)
      {
         handle_exception(e);
      }
      return total_int_recv;
   }

   public boolean connection_ok()
   {
      return this.is_connected;
   }

   public void close()
   {
      finalize();
   }

   /**

     Close all sockets to free up resources.
   */
   protected void finalize()
   {
      dbg.API("SockIPC: Invoking finalize method...close all streams");

      try
      {
         if ( this.dest_socket != null )
         {
            this.dest_socket.close();
            this.dest_socket = null;
         }

         if ( this.sink != null )
         {
            this.sink.close();
            this.sink = null;
         }

         if ( this.source != null )
         {
            this.source.close();
            this.source = null;
         }

         this.is_connected=false;
      }
      catch (IOException e)
      {
         System.out.println(e.toString());
         return;
      }
      return;
   }

   // Test Program
   public static void main(String args[])
   {
      int s=123;
      SockIPC  sv=new SockIPC(512);
      byte[]  byte_ary=new byte[4];
      StringBuffer   s_cmd=new StringBuffer(512);

      byte_ary[0]=0x1;
      byte_ary[1]=0x2;
      byte_ary[2]=0x3;
      byte_ary[3]=0x4;

      int  bval= (byte_ary[0] << 24) | (byte_ary[1] << 16) | (byte_ary[2] << 8 ) | byte_ary[3] ;

      System.out.println("bval= "+bval);

      sv.connect(SockIPC.VMGRADS_IP_ADDR, SockIPC.VMGRADS_IP_PORT);

      try
      {
         sv.send_ga_cmd(SockIPC.GRADS_CMD_REQUEST,"set lat 23.5".toCharArray(),
                        12);

         sv.recv_ga_cmd(s_cmd);
         System.out.println("response from server: "+s_cmd);


         sv.send_ga_cmd(SockIPC.GRADS_CMD_REQUEST,"set lon 112.45".toCharArray(),
                        13);
         sv.recv_ga_cmd(s_cmd);
         System.out.println("response from server: "+s_cmd);
      }
      catch ( IOException e)
      {
         System.out.println(e);
      }

      try
      {
         Thread.sleep(5000);
      }
      catch ( java.lang.InterruptedException e)
      {
         System.out.println(e);
      }

      try
      {
         sv.send_ga_cmd(SockIPC.GRADS_CMD_REQUEST,"d slpr".toCharArray(), 6);
         sv.recv_ga_cmd(s_cmd);
      }
      catch ( Exception e)
      {
         System.out.println(e);
      }

      System.out.println("response from server: "+s_cmd);

      sv.finalize();

    return;
  }
}

