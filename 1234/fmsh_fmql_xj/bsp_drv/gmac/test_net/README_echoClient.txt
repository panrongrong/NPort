FILE LIST -
              echoClient.c
 
 DESCRIPTION - 
 	     The TCP echo client repeatedly sends the input and then reads 
              it back using TCP socket communication.
 
 	     ECHO services are important tools that network managers use 
              to test reachability (to make sure the connection works), 
              debug protocol software, and identify routing problems.
 
 	     This also demonstrates the usage of SO_REUSEADDR, 
              SO_KEEPALIVE and TCP_NODELAY set socket options.
 
 RUNNING DEMO -
 	     You need to set/change the value of the defaultPort 
              (should be greater than 5000 - unreserved port) and the 
              defaultServer (internet address of the echo server) variables.
 
 	     Run echoTcpClientRun task on one VxWorks system as follows
 	     after starting the echoTcpServerRun task from 
              another VxWorks system.
 	     The other half of the demonstration is in echoServer.c.
 
 	     Place the echoClient.c file in your
 	     <Tornado> dir. Depending on the target
 	     you have, define the CPU in your compile line and use
 	     the Makefile in the BSP dir. to compile.
 
 	     To run this code invoke the function echoTcpClientRun()
 
 TESTED ON -  Host/Target : Solaris 2.5.1 /mv1604
 	     VxWorks     : 5.3.1
 
 EXAMPLE COMPILE LINE -
               make CPU=PPC604 echoClient.o
 
 %make CPU=PPC604 echoClient.o
 ccppc -B/petaluma1/mayur/tor101-ppc/host/sun4-solaris2/lib/gcc-lib/ 
 -mstrict-align -ansi -nostdinc -O2 -fvolatile -fno-builtin -fno-for-scope 
 -Wall -I/h   -I. -I/petaluma1/mayur/tor101-ppc/target/config/all 
 -I/petaluma1/mayur/tor101-ppc/target/h 
 -I/petaluma1/mayur/tor101-ppc/target/src/config 
 -I/petaluma1/mayur/tor101-ppc/target/src/drv -DCPU=PPC604  -DMV1600 
 -DTARGET_DIR="\"mv1604\""   -c echoClient.c
 
 OUTPUTS/LOGFILE -
 
 On VxWorks target:
 ==================
 
 -> ld <echoClient.o
 value = 917544 = 0xe0028
 -> sp (echoTcpClientRun, "147.11.41.154", 7001, 5 , "Hello World!!!")
 task spawned: id = 5c7c50, name = u14
 value = 6061136 = 0x5c7c50
 
 The output on the console window:
 =================================
 
 Binding SERVER
 Listening to client
 Server's address is 930b299a:
 Connected...
 The length of the message is - 14 bytes
 Message: Hello World!!!
 
 buffer is - Hello World!!! and numRead = 14
 echoTcpClient received echo message: Hello World!!!
 buffer is - Hello World!!! and numRead = 14
 echoTcpClient received echo message: Hello World!!!
 buffer is - Hello World!!! and numRead = 14
 echoTcpClient received echo message: Hello World!!!
 buffer is - Hello World!!! and numRead = 14
 echoTcpClient received echo message: Hello World!!!
 buffer is - Hello World!!! and numRead = 14
 echoTcpClient received echo message: Hello World!!!
 No more bytes to read 
 Closing the sockets
 
 Binding SERVER
 Listening to client
 
 
