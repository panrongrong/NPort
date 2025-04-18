FILE LIST -
       echoServer.c
 
 DESCRIPTION -
 	     The TCP echo service specifies that a server must accept incoming
 	     requests, read data from the connection, and write the data back
 	     over the connection until the client terminates the transfer.
 
 	     Echo server merely returns all the data it receives from a client.
 	     Echo services are important tools that network managers use to 
              test reachability (to make sure the connection works), 
              debug protocol software, and identify routing problems.
 
 	     This also demonstrates the usage of SO_REUSEADDR, SO_KEEPALIVE,
 	     TCP_NODELAY and SO_LINGER set socket options.
 
 RUNNING DEMO -
 	       Place the echoServer.c in your
 	       <Tornado> dir. Depending on the target
 	       you have, define the CPU in your compile line and use
 	       the Makefile in the BSP dir. to compile.
             
 	       Run this echoTcpServerRun task on one VxWorks system as follows
 	       before starting the echoTcpClientRun task from another 
                VxWorks system.
 	       The other half of the demonstration is in echoClient.c.
 
 	       sp (echoTcpServerRun, port)
 	       where port is the port number used by the echoTcpServer 
                (should be greater than 5000).
 
 	       To run this code invoke the function echoTcpServerRun()
 
 TESTED ON -   Host/Target : Solaris 2.5.1 / mv1604
 	      VxWorks     : 5.3.1
 
 EXAMPLE COMPILE LINE -
 	    make CPU=PPC604 echoServer.o
 
 % make CPU=PPC604 echoServer.o
 ccppc -B/petaluma1/mayur/tor101-ppc/host/sun4-solaris2/lib/gcc-lib/ 
 -mstrict-align -ansi -nostdinc -O2 -fvolatile -fno-builtin -fno-for-scope 
 -Wall -I/h   -I. -I/petaluma1/mayur/tor101-ppc/target/config/all 
 -I/petaluma1/mayur/tor101-ppc/target/h 
 -I/petaluma1/mayur/tor101-ppc/target/src/config 
 -I/petaluma1/mayur/tor101-ppc/target/src/drv -DCPU=PPC604  -DMV1600 
 -DTARGET_DIR="\"mv1604\""   -c echoServer.c
 
