/* vxBlaster.c - blasts another system with the given message size */

/*
  Jan 5th 1997:  Dr. Stephen Adler, Brookhaven National Laboratory,
                 Modified to include upd transfers and the
                 checking of sequential buffer ordering.
01b,12jun02,tis  added delay parameter to the function blaster()
01a,29jan94,ms   cleaned up for VxDemo.
11,20jun95,ldt   cleaned up for 511.
*/

/*****************************************************************************
 * blaster - client task for VxWorks target
 *
 * DESCRIPTION
 *
 *     This is a client task which connects to the server via TCP socket.
 *     It allows to configure the maximum  size  of  the  socket-level
 *     send buffer. It repeatedly sends a given size message to the given port
 *     at destination target. It stops sending the message when the global
 *     variable blasterStop is set to 1. 
 *
 * EXAMPLE:
 *
 *     To run this blaster task from the VxWorks shell do as follows: 
 *     -> sp (demoBlaster, "192.168.3.212", 7000, 1, 0x400, 16000,0); tcp blastering..
 *     -> sp (demoBlaster, "192.168.2.245", 7000, 0, 0x400, 16000,100); udp blastering..
 *     
 *      -> sp (demoBlaster, "192.168.2.245", 7000, 1, 0x2000, 16000,0); tcp blastering..
 *     -> sp (demoBlaster, "192.168.2.245", 7000, 0, 0x2000, 16000,300); udp blastering..
 *     
 *      -> sp (demoBlaster, "192.168.2.245", 7000, 1, 0x8000, 64000,0); tcp blastering..
 *     -> sp (demoBlaster, "192.168.2.245", 7000, 0, 0x8000, 64000,150); udp blastering..
 *
 *     To stop blaster task from the VxWorks shell do as follows: 
 *     -> blasterStop = 1 
 *
 */

#include "vxWorks.h"
#include "wdLib.h"
#include "socket.h"
#include "in.h"
#include "ioLib.h"
#include "stdlib.h"
#include "sockLib.h"
#include "stdio.h"
#include "fioLib.h"
#include "string.h"
#include "sysLib.h"
#include "logLib.h"
#include "taskLib.h"



int select6396port = 0;
/*extern UINT32	vxSvrGet (void); */
int blasterStop;

#define DELTA(a,b)              (abs((int)a - (int)b))

/***************************************************************************
*
* sysMsDelay - Delay by the passed number of Micro seconds 
*
* RETURNS: N/A 
*
*
*/
static void sysMSDelay
(
    UINT        delay                   /* length of time in MS to delay */
)
{
    register UINT oldval;               /* decrementer value */
    register UINT newval;               /* decrementer value */
    register UINT totalDelta;           /* Dec. delta for entire delay period */
    register UINT decElapsed;           /* cumulative decrementer ticks */

    /*
     * Calculate delta of decrementer ticks for desired elapsed time.
     * The macro DEC_CLOCK_FREQ MUST REFLECT THE PROPER 6xx BUS SPEED.
     */

    totalDelta = ((100000000 / 4) / 1000000) * delay;

    /*
     * Now keep grabbing decrementer value and incrementing "decElapsed" until
     * we hit the desired delay value.  Compensate for the fact that we may
     * read the decrementer at 0xffffffff before the interrupt service
     * routine has a chance to set in the rollover value.
     */

    decElapsed = 0;

    oldval = vxDecGet ();

    while (decElapsed < totalDelta)
	{
    	newval = vxDecGet ();

        if ( DELTA(oldval,newval) < 1000 ) decElapsed += DELTA(oldval,newval);  /* no rollover */
        else if (newval > oldval)  decElapsed += abs((int)oldval);  /* rollover */
        oldval = newval;
	}
}

void demoBlaster 
(
    char    *targetAddr, /* targetAddress is the IP address of destination 
                            target that needs to be blasted */
    int	     port,       /* port number to send to */
    int      dotcp,      /* dotcp = 1, tcp blastering; dotcp=0 udp blastering */
    int      size,       /* size of the message to be sent */
    int      blen,        /* maximum size of socket-level send buffer */
    int      delay        /* delay in Micro seconds */
)
{

	struct sockaddr_in	sin;
	int    sock;
	char   *buffer;
	unsigned long *intbuffer;
	int enable_broadcast = 1;
	unsigned long countpp = 0, countmm = 0;
     UINT32 i,j;
	bzero ((char *) &sin, sizeof (sin));
	sock = socket (AF_INET, dotcp ? SOCK_STREAM : SOCK_DGRAM, 0);
	
	if (sock < 0) 
	{
		perror ("cannot open socket");
		exit (1);
	}
	
	sin.sin_family 	= AF_INET;
	sin.sin_addr.s_addr	= inet_addr (targetAddr);
	sin.sin_port	= htons (port);

	
	if ((buffer = (char *) malloc (size)) == NULL) 
	{
	  perror ("cannot allocate buffer of size ");
	  exit (1);
	}
	bzero(buffer,size);
	
	intbuffer = (unsigned long *)buffer;
	
	if (setsockopt (sock, SOL_SOCKET, SO_SNDBUF,(char *) &blen, sizeof (blen)) < 0) 
	{
		perror ("setsockopt SO_SNDBUF failed");
		free (buffer);
		exit (1);
	}

	if (setsockopt (sock, SOL_SOCKET, SO_RCVBUF, (char *) &blen, sizeof (blen)) < 0) 
	{
		perror ("setsockopt SO_RCVBUF failed");
		free (buffer);
		exit (1);
	}
	if (dotcp) 
	{

		if (connect (sock, (struct sockaddr *) &sin, sizeof (sin)) < 0) 
		{
			perror ("connect failed");
			printf ("host %s port %d\n", inet_ntoa (sin.sin_addr),ntohs (sin.sin_port));
			free (buffer);
			exit (1);
		}
	}
	blasterStop = FALSE; 
	for (;;) 
	{
		int nBytes;
		
		if (blasterStop == TRUE)  
			break;
		
		/*if(dotcp == 0)sysMSDelay(delay);*/
		intbuffer[0] = htonl(countpp);
		intbuffer[(size/sizeof(unsigned long))-1] = htonl(countmm);
		countpp++;
		countmm--;	
		if (dotcp == 0 )
		{
			for(i=0;i<1000;i++)
			{
				j=i;
			}
		}
			
		if (dotcp) 
		{
			if ((nBytes = write(sock, buffer, size)) < 0) 
			{
				perror ("blaster write error");
				break;
			}
		} 
		else 
		{
			if ((nBytes = sendto(sock, buffer, size, 0, (struct sockaddr *)&sin,sizeof(sin))) < 0) 
			{
				perror("blaster write error");
				break;
			}
		}
	}
	if (dotcp) 
	{
		shutdown(sock,SHUT_RDWR);
	}
	close (sock);
	free (buffer);
	printf ("blaster exit.\n");
}

int udpSendTid = -1;
int tcpSendTid = -1;
void testUdpSnd(char *targetAddr)
{
	udpSendTid=taskSpawn("tUdpSnd",200,0,20000,(FUNCPTR)demoBlaster,(int)targetAddr,7000,0,1500,0x4000,0,0,0,0,0);
}

void testTcpSnd(char *targetAddr, int port)
{
	tcpSendTid=taskSpawn ("tTcpSnd",200,0,20000,(FUNCPTR)demoBlaster,(int)targetAddr,port,1,1500,0x4000,0,0,0,0,0);
}

void delUdpSendTask()
{
	if (udpSendTid != -1)
	{
		blasterStop = TRUE;
	/*	if (taskDelete(udpSendTid) == OK)
		{
			DEBUG_LOG("killed task %d\n",udpSendTid);
		}*/
		udpSendTid = -1;
	}
}

void delTcpSendTask()
{
	if (tcpSendTid != -1)
	{
		blasterStop = TRUE;
		/*if (taskDelete(tcpSendTid) == OK)
		{
			DEBUG_LOG("killed task %d\n",tcpSendTid);
		}*/

		tcpSendTid = -1;
	}
}

/*extern STATUS sysBcm5396Write_64(UINT8 page ,UINT8 reg,UINT16 datavalue,UINT16 d1,UINT16 d2,UINT16 d3);*/
/*void test5396Sgmii(int port)*/
/*{*/
/*	UINT32 PortNum[5] = {1,3,4,5,6};*/
/*	/*选择 对外的 1口和3口进行通信*/
/*	if((PortNum[port] == 1)||(PortNum[port] == 3))*/
/*	{*/
/*		select6396port = 3;*/
/*		sysBcm5396Write_64(0x31,0x28,0x1,0xffff,0x1,0x1); CPUA和 1口可以通信 ，即 1口和2口*/
/*		sysBcm5396Write_64(0x31,0x2c,0x1,0xffff,0x1,0x004); /*CPUB和 3口可以通信 ，即 9口和3口*/
/*		return;*/
/*	}*/
/*	选择 对外的 4口进行通信*/
/*	if(PortNum[port] == 4)*/
/*	{*/
/*	    sysBcm5396Write_64(0x31,0x28,0x1,0xffff,0x1,0xA);*/
/*	    sysBcm5396Write_64(0x31,0x2c,0x1,0xffff,0x1,0x120);*/
/*	    return;*/
/*	}*/
/*	/*选择 对外的 5口进行通信*/
/*	if(PortNum[port] == 5)*/
/*	{*/
/*	    sysBcm5396Write_64(0x31,0x28,0x1,0xffff,0x1,0x12);*/
/*	    sysBcm5396Write_64(0x31,0x2c,0x1,0xffff,0x1,0x120);*/
/*	    return;*/
/*	}*/
/*	选择 对外的 6口进行通信*/
/*	if(PortNum[port] == 6)*/
/*	{*/
/*	    sysBcm5396Write_64(0x31,0x28,0x1,0xffff,0x1,0x12);*/
/*	    sysBcm5396Write_64(0x31,0x2c,0x1,0xffff,0x1,0x120);*/
/*	    return;*/
/*	}*/
/*}*/
/**/
