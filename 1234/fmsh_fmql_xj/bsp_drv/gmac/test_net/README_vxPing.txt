FILE LIST -
       vxPing.c
 
 DESCRIPTION -
 	     ping - sends ICMP ECHO_REPLY packets to a particular network host to
 	     elicit an ICMP ECHO_RESPONSE from that specified network host
 	     and prints "<host> is alive" on the standard output if the host
 	     responds; otherwise prints ICMP error messages.
 
 	     When using ping, make sure the host you want to ping is already
 	     in the host table. If not, add the host to the host table using
 	     hostAdd. If the host you want to ping is in a different network
 	     than your VxWorks system, make sure route is established to that
 	     host. If the route is not already established, use routeAdd to add
 	     the route.
 
 RUNNING DEMO -
 	     Place the vxPing.c file in your <Tornado> dir. Depending on the
 	     target you have, define the CPU in your compile line and use
 	     the Makefile in the agent dir. to compile.
 
 	     Run ping() on vxworks target.
 
 TESTED ON -   Host/Target : Solaris 2.5.1 / mv1604
 	      VxWorks     : 5.3.1
 
 	      EXAMPLE COMPILE LINE -
                             make CPU=PPC604 vxPing.o
 
 % make CPU=PPC604 vxPing.o
 ccppc -B/petaluma1/mayur/tor101-ppc/host/sun4-solaris2/lib/gcc-lib/ 
 -mstrict-align -ansi -nostdinc -O2 -fvolatile -fno-builtin -fno-for-scope 
 -Wall -I/h   -I. -I/petaluma1/mayur/tor101-ppc/target/config/all 
 -I/petaluma1/mayur/tor101-ppc/target/h 
 -I/petaluma1/mayur/tor101-ppc/target/src/config 
 -I/petaluma1/mayur/tor101-ppc/target/src/drv -DCPU=PPC604  -DMV1600 -DTARGET_DIR="\"mv1604\"" -g -O0  -c vxPing.c
 
 
 OUTPUTS/LOGFILE -
 
 On VxWorks target:
 ==================
 
 -> ld <vxPing.o
 value = 689768 = 0xa8668 = dbgTraceStub + 0x708
 -> vxPing("petaluma",5)
 value = 0 = 0x0
 
 Output on the console:
 ======================
 Name of the host to ping is -> petaluma
 wrote petaluma 28 bytes, return = 28
 wrote petaluma 28 bytes, return = 28
 wrote petaluma 28 bytes, return = 28
 wrote petaluma 28 bytes, return = 28
 wrote petaluma 28 bytes, return = 28
 
 host petaluma is alive
 
