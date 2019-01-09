/*
  This API for GradsVM server, it listen for an connection from JGrads client,
  and create an instance of GradsVM to service the requests.

  @author  Pedro T.H. Tsai,
  @version 1.0
  @date    Feb 15, 2009

*/

import java.net.*;
import java.io.*;

public class GVMServer implements Cloneable, Runnable
{
   Thread           main_server_thread = null;

   ServerSocket     cmd_server_sock = null;

   ServerSocket     gfx_server_sock = null;

   Socket           cmd_data_sock = null;

   Socket           gfx_data_sock = null;

   int              port = 0;

   boolean          runflag  = false;



   /* Debug info object for this class. */
   private final static DebugInfo dbg = new DebugInfo("GVMServer");

   /* Initialization block for static variables. */
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

   public GVMServer()
   {

   }

   private synchronized Thread start_grads_server(int port)
   {
      this.port = port;
      dbg.API("open port number: "+this.port);

      if ( this.main_server_thread == null )
      {
         try
         {
            /* create the server socket and listen on the specified ports */
            this.cmd_server_sock     = new ServerSocket(this.port,5);
            this.gfx_server_sock     = new ServerSocket(this.port+1,5);
            this.main_server_thread  = new Thread(this);

            /* run() methos is started. */
            this.main_server_thread.start();
         }
         catch (IOException e)
         {
            System.out.println(e.toString());
         }
      }

      return this.main_server_thread;

   }

   public void run()
   {
      this.runflag = true;

      if ( this.cmd_server_sock != null &&
           this.gfx_server_sock != null )
      {
         dbg.API("listening on local port "+
                 this.cmd_server_sock.getLocalPort()+
                 "for gacmd I/O .... "
                  );
         dbg.API("listening on local port "+
                 this.gfx_server_sock.getLocalPort()+
                 "for graphics primitives I/O .... "
                  );

         while ( this.runflag == true )
         {
            try
            {
               Socket   sock_1;
               Socket   sock_2;

               dbg.API("Waiting for gacmd connection.....");
               sock_1=this.cmd_server_sock.accept();
               dbg.API("Gacmd connection made by client from host: "+
                        sock_1.getInetAddress().getHostName());

               dbg.API("Waiting for graphics commands connection.....");
               sock_2=this.gfx_server_sock.accept();
               dbg.API("Graphics commands connection made by client from host: "+
                        sock_2.getInetAddress().getHostName());

               dbg.API("clone GVMServer");
               GVMServer  new_GVMServer=(GVMServer)clone();


               new_GVMServer.cmd_server_sock=null;
               new_GVMServer.gfx_server_sock=null;
               new_GVMServer.cmd_data_sock=sock_1;
               new_GVMServer.gfx_data_sock=sock_2;

               new_GVMServer.main_server_thread = new Thread(new_GVMServer);
               new_GVMServer.main_server_thread.start();

               /* continue the while loop for more connection. */
            }
            catch (Exception e)
            {
               System.out.println(e);
               System.exit(0);

            }
         }
      }
      else
      {
         /* this is the newly cloned GVMServer object, the
            GradsVM is executed in this thread */
         start_grads_vm(this.cmd_data_sock, this.gfx_data_sock);
      }
      return;
   }

   private void start_grads_vm(Socket cmd_data_sock, Socket gfx_data_sock)
   {
      String[] args;

      dbg.INFO("GVM started ...");
      GVM     grads_vm = new GVM(cmd_data_sock, gfx_data_sock);

      args=grads_vm.get_client_args();

      try
      {
         grads_vm.run(args);
      }
      catch (Exception e)
      {
         System.out.println(e);
      }

      dbg.INFO("GVM terminated ...");
   }

   // Test program
   public static void main(String args[])
   {
      GVMServer   jgs = new GVMServer();

      Thread  t=jgs.start_grads_server(SockIPC.VMGRADS_IP_PORT);

      while (true)
      {
         try
         {
            Thread.sleep(10000);
         }
         catch (InterruptedException e)
         {
            System.out.println(e);
            break;
         }
      }
      return;
   }
}













