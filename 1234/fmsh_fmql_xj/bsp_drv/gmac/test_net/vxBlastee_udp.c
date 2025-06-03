/* blasteeUDPvx.c - UDP ethernet transfer benchmark for VxWorks */
                    
/* Copyright 1986-2001 Wind River Systems, Inc. */

/*               
modification history
--------------------
04a,10may01,jgn  checkin for AE
01a,18May99,gow  Created from blastee.c
*/                               
                                 
/*
DESCRIPTION
   With this module, a VxWorks target ("blasteeUDP") receives
   blasts of UDP packets and reports network throughput on 10
   second intervals.

SYNOPSIS
   blasteeUDP (port, size, bufsize, zbuf)

   where:

   port = UDP port for receiving packets
   size = number of bytes per "blast"
   bufsize = size of receive-buffer for blasteeUDP's BSD socket
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
#include <socket.h>
#include <sockLib.h>
#include <stdlib.h>
#include <sysLib.h>
#include <logLib.h>
#include <errno.h>
#include <string.h> 
#include <stdio.h> 
#include "wdLib.h"
#include "in.h" 	   
#include "ioLib.h" 

static int tid;          /* task ID for cleanup purposes */
static char *buffer;     /* receive buffer */
static int sock;    	 /* receiving socket descriptor */
static int blastNum_rx_udp;   	 /* number of bytes read per 10 second interval */

LOCAL int  wdIntvl_rx_udp = 5;  /* 60s -> 10s -> 5s*/

LOCAL WDOG_ID  blastWd_rx_udp = NULL;
int    blasteeStop_udp;


/* forward declarations */

static void blastRate_rx_udp ();


/*****************************************************************
 *
 * blasteeUDP - Accepts blasts of UDP packets from blasterUDP
 * 
 * This function accepts blasts of UDP packets from blasterUDP
 * and reports the throughput every 10 seconds.
 * 
 * blasteeUDP (port, size, bufsize, zbuf)
 * 
 * where:
 * 
 * port = UDP port to connect with on "blasterUDP"
 * size = number of bytes per "blast"
 * bufsize = size of receive-buffer for blasteeUDP's BSD socket
 * zbuf = whether to use zero-copy sockets (1 = yes, 0 = no)
 * 
 * RETURNS:  N/A
 */

void blastee_rx_udp(int port, int size, int blen, int zbuf)
{
    struct sockaddr_in	serverAddr, clientAddr;
    int nrecv;  /* number of bytes received */
    int sockAddrSize = sizeof(struct sockaddr);
 /*   ZBUF_ID             zid;    /* zero-copy buffer ID */

    buffer = (char *) malloc (size);
    /*buffer = (char *memalign(256,size);*/
    
    if(buffer == NULL)
	{
    	printf("cannot allocate buffer of size %d\n", size);
        return;
	}
 
     /* Create watchdog timer */
     if ((blastWd_rx_udp = wdCreate ()) == NULL)
 	{
	 	perror ("cannot create blast watchdog");
	 	free (buffer);
	 	exit (1);
 	}
 
     /* Start watchdog after a minute*/
     wdStart (blastWd_rx_udp, sysClkRateGet () * wdIntvl_rx_udp, (FUNCPTR) blastRate_rx_udp, 0);
	

    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
    	printf("cannot open socket\n");
        wdCancel(blastWd_rx_udp);
        free(buffer);
        return;
	}
	else
	{
		printf("socket create ok! \n");
	}

    bzero((char *)&serverAddr.sin_zero, 8);  /* zero rest of struct */
	
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_port        = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY; /* auto-fill with my IP */

    if (bind(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
	{
    	printf("bind OS_ERROR\n");
        close(sock);
        wdCancel(blastWd_rx_udp);
        free(buffer);
        return;
	}
	else
	{
		printf("socket bind ok! \n");
	}

/* setting buffer size breaks UDP sockets for some reason.  This
 * happens on Linux too (but not Solaris). Hence setsockopt() is
 * commented till further notice.
 */

   if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&blen, sizeof(blen)) < 0)
	{
	   printf("setsockopt SO_RCVBUF failed\n");
	   
       close(sock);
       wdCancel(blastWd_rx_udp);
       free(buffer);
	   
       exit(1);
	}
	else
	{
		printf("setsockopt ok! \n");
	}
				   

    blastNum_rx_udp = 0;
				   
	blasteeStop_udp = FALSE;

    /* loop that reads UDP blasts */
    while (1)
	{
		
 /*       if (zbuf)
            {
            nrecv = size;
            if ((zid = zbufSockRecvfrom(sock, 0, &nrecv,
                                        (struct sockaddr *)&clientAddr,
                                        (int *)&sockAddrSize)) == NULL)
                nrecv = -1;
            else
                zbufDelete(zid);
            }
        else  */
        
        nrecv = recvfrom(sock, (char *)buffer, size, 0, 
                              (struct sockaddr *)&clientAddr,
                              (int *)&sockAddrSize);
		
/*        printf("setsockopt receive\n");/*LYT*/
        /*
        if (nrecv < 0)
        {
        	printf("blasteeUDP read OS_ERROR %d\n", errno);
        	break;
	    }
		*/
        
        if (nrecv > 0)
        {
        	blastNum_rx_udp += nrecv;
        }
		
		if(blasteeStop_udp)
			break;
	}

    close(sock);
    wdCancel(blastWd_rx_udp);
    free(buffer);
	
    printf("blasteeUDP end.\n");
	return;
}

/* watchdog handler. reports network throughput */
static void blastRate_rx_udp ()
{
    if (blastNum_rx_udp > 0)
	{
    	logMsg("udp rx %d bytes/sec \n", blastNum_rx_udp/wdIntvl_rx_udp, 2,3,4,5,6);
    	blastNum_rx_udp = 0;
	}
    else
    {	
    	logMsg("udp No bytes read in the last %d seconds. \n", wdIntvl_rx_udp, 2,3,4,5,6);
    }

     wdStart (blastWd_rx_udp, sysClkRateGet () * wdIntvl_rx_udp, (FUNCPTR) blastRate_rx_udp, 0);
}

void close_rx_udp(void)
{
	blasteeStop_udp = TRUE;	
	wdCancel (blastWd_rx_udp);
	
	return;
}

void start_rx_udp(int size)
{
	int blen = 16000;
	
	/*char ip[64] = {0};*/
	int port = 8080; 
	int zbuf = 0;

	/*sprintf(ip, "%s", "192.168.46.154");*/
		
	tid = taskSpawn("blasteeUdp", 110, VX_FP_TASK, 0x100000, (FUNCPTR)blastee_rx_udp,
			         port, size, blen, zbuf, 5, 6, 7, 8, 9, 10);

	return;
}


