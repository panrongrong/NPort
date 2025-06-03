/* blasterUDPvx.c - UDP ethernet transfer benchmark for VxWorks */

/* Copyright 1986-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
04a,10may01,jgn  checkin for AE
02b,18May99,gow  created from blaster.c
*/

/*
DESCRIPTION
   With this module, VxWorks transmits blasts of UDP packets to
   a specified target IP/port.

SYNOPSIS
   blasterUDP (targetname, port, size, bufsize, zbuf)

   where:

   targetname = network address of "blasteeUDP"
   port = UDP port to connect with on "blasteeUDP"
   size = number of bytes per "blast"
   bufsize = size of transmit-buffer for blasterUDP's BSD socket
             (usually, size == bufsize)
   zbuf = whether to use zero-copy sockets (1 = yes, 0 = no)

EXAMPLE
   To start this test, issue the following VxWorks command on the
   target:

   sp blasteeUDP,5000,1000,1000

   then issue the following UNIX commands on the host:

   gcc -O -Wall -o blasterUDP blasterUDPux.c -lsocket   (solaris)
   gcc -O -Wall -DLINUX -o blasterUDP blasterUDPux.c    (linux)
   blasterUDP 10.255.255.8 5000 1000 1000  (use appropriate IP address)

   Note: blasteeUDP should be started before blasterUDP because
   blasterUDP needs a port to send UDP packets to.

   To stop this test, call blasteeUDPQuit() in VxWorks or kill
   blasterUDP on UNIX with a control-c.

   The "blasterUDP"/"blasteeUDP" roles can also be reversed. That is,
   the VxWorks target can be the "blasterUDP" and the UNIX host can
   be the "blasteeUDP". In this case, issue the following UNIX
   commands on the host:

   gcc -O -Wall -o blasteeUDP blasteeUDPux.c -lsocket    (solaris)
   gcc -O -Wall -DLINUX -o blasteeUDP blasteeUDPux.c     (linux)
   blasteeUDP 5000 1000 1000

   and issue the following VxWorks command on the target
   (use appropriate IP address):

   sp blasterUDP,"10.255.255.4",5000,1000,1000

   To stop the test, call blasterUDPQuit() in VxWorks or kill
   blasteeUDP on Unix with a control-c.

CAVEATS
   Since this test loads the network heavily, the target and host
   should have a dedicated ethernet link.

   Be sure to compile VxWorks with zbuf support (INCLUDE_ZBUF_SOCK).

   The bufsize parameter is disabled for VxWorks. Changing UDP socket
   buffer size with setsockopt() in VxWorks somehow breaks the socket.
*/

#include <vxworks.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "vxWorks.h"
#include "socket.h"
#include <sockLib.h>
#include "arpa/inet.h"
#include "in.h"
 

int blasterStop_udp;

LOCAL int  blastNum_tx_udp;
LOCAL int  wdIntvl_tx_udp = 5;	/* 60s -> 10s -> 5s*/

LOCAL WDOG_ID  blastWd_tx_udp = NULL;

LOCAL void blastRate_tx_udp ();

/*****************************************************************
 *
 * blasterUDP - Transmit blasts of UDP packets to blasteeUDP
 * 
 * blasterUDP (targetname, port, size, bufsize, zbuf)
 * 
 * where:
 * 
 * targetname = network address of "blasteeUDP"
 * port = UDP port on "blasteeUDP" to receive packets
 * size = number of bytes per "blast"
 * bufsize = size of transmit-buffer for blasterUDP's BSD socket
 * zbuf = whether to use zero-copy sockets (1 = yes, 0 = no)
 * 
 * RETURNS:  N/A
 */

void blaster_tx_udp(char *targetAddr, int port, int size, int blen, int zbuf)
{
    struct sockaddr_in	sin;
    int s;      /* socket descriptor */
    int nsent; /* how many bytes sent */
    char *buffer;

	 /* Create watchdog timer */
	 if (blastWd_tx_udp == NULL && (blastWd_tx_udp = wdCreate ()) == NULL)
	{
		perror ("cannot create blast watchdog");
		exit (1);
	}
	
	/* Start watchdog after a minute*/
	wdStart (blastWd_tx_udp, sysClkRateGet () * wdIntvl_tx_udp, (FUNCPTR) blastRate_tx_udp, 0);
	 

    /* setup BSD socket for transmitting blasts */
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
    	printf("cannot create socket\n");
        return;
    }
	
    bzero((char *)&sin.sin_zero, 8);  /* zero the rest of the struct */

    /*sin.sin_addr.s_addr	= inet_addr(targetAddr);	*/
	sin.sin_addr.s_addr = inet_addr ("192.168.46.154");
	
    sin.sin_port	    = htons(port);
    sin.sin_family 	    = AF_INET;
	

    if((buffer = (char *)malloc(size)) == NULL)
	{
    	printf("cannot allocate buffer of size %d\n", size);
    	close(s);
        return;
	}

/* setting buffer size breaks UDP sockets for some reason.  This
 * happens on Linux too (but not Solaris). Hence setsockopt() is
 * commented till further notice.
 */

    if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (void *)&blen,
                   sizeof (blen)) < 0)
	{
    	printf("setsockopt SO_SNDBUF failed\n");
    	close(s);
		free(buffer);
        return;
	}
  

    /* Loop that transmits blasts */
	blastNum_tx_udp = 0;
	blasterStop_udp = FALSE;

    while (1)
	{
		if (blasterStop_udp)
		    break;
	
/*        if (zbuf)
            nsent = zbufSockBufSendto(s, buffer, size, NULL, 0, 0,
                                      (struct sockaddr *)&sin,
                                      sizeof(struct sockaddr_in));
        else  */
        
        nsent = sendto(s, buffer, size, 0, (struct sockaddr *)&sin, 
        			sizeof(struct sockaddr_in));

        /* sendto() and zbufSockBufSendto() both OS_ERROR-out for
         * some strange reason. Somehow this doesn't seem to
         * affect the test, and we can get away with ignoring
         * the OS_ERRORs.
         */
        if (nsent > 0)
		{
			blastNum_tx_udp += nsent;
		}
    }
    
    /* cleanup */
    close(s);
    free(buffer);
    printf("blasterUDP exit.\n");
}


LOCAL void blastRate_tx_udp ()
{
    if (blastNum_tx_udp > 0)
 	{
	 	logMsg ("udp tx %d bytes/sec\n", blastNum_tx_udp / wdIntvl_tx_udp,0, 0, 0, 0, 0);
	 	blastNum_tx_udp = 0;
 	}
    else
 	{
 		logMsg ("udp No bytes write in the last %d seconds.\n", wdIntvl_tx_udp, 0, 0, 0, 0, 0);
 	}
		
    wdStart (blastWd_tx_udp, sysClkRateGet () * wdIntvl_tx_udp, (FUNCPTR) blastRate_tx_udp, 0);
	return;
}

 void close_tx_udp(void)
{
	blasterStop_udp = TRUE;	
	wdCancel (blastWd_tx_udp);
	return;
}

void start_tx_udp(int size)
{
	int tid;
	int blen = 16000;
	
	char ip[64] = {0};
	int port = 8080; 
	int zbuf = 0;

	/*sprintf(ip, "%s", "192.168.46.154");*/
		
	tid = taskSpawn("blasterUdp", 110, VX_FP_TASK, 0x100000, (FUNCPTR)blaster_tx_udp,
			         ip, port, size, blen, zbuf, 6, 7, 8, 9, 10);

	return;
}


