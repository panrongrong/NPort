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
 *     -> sp (demoBlaster, "192.168.2.230", 7000, 1, 0x400, 16000,0); tcp blastering..
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

#include "net.h"
#include "arpLib.h"

int blasterStop;
int blasterSock;
#define DELTA(a,b)              (abs((int)a - (int)b))




/***************************************************************************
*
* sysMsDelay - Delay by the passed number of Micro seconds 
*
* RETURNS: N/A 
*
*
*/


char * svripaddr = "192.168.3.10";  
char * cliipaddr = "192.168.3.50";	


static void sysMSDelay
(
    UINT32        delay                   /* length of time in MS to delay */
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


int log_tx = 0;

void open_log_tx(void)
{
	log_tx = 1;
}

void close_log_tx(void)
{
	log_tx = 0;
}

void demoBlaster 
(
    /*char    *targetAddr, /* targetAddress is the IP address of destination 
                            target that needs to be blasted */
    int      ip,
    int	     port,       /* port number to send to */
    int      dotcp,      /* dotcp = 1, tcp blastering; dotcp=0 udp blastering */
    int      size,       /* size of the message to be sent */
    int      blen,        /* maximum size of socket-level send buffer */
    int      delay        /* delay in Micro seconds */
)
{

	struct sockaddr_in	sin;
	char   *buffer;
	unsigned long *intbuffer;
	int enable_broadcast = 1;
	unsigned long countpp = 0, countmm = 0;
	
	int i ; 
	
	int j ; 
	int t;
	bzero ((char *) &sin, sizeof (sin));
	blasterSock = socket (AF_INET, dotcp ? SOCK_STREAM : SOCK_DGRAM, 0);
	
	if (blasterSock < 0) 
	{
		perror ("cannot open socket");
		exit (1);
	}
	
	sin.sin_family 	= AF_INET;
	sin.sin_addr.s_addr	= ip;
	sin.sin_port	= htons (port);
	
	if ((buffer = (char *) malloc (size)) == NULL) 
	{
	  perror ("cannot allocate buffer of size ");
	  exit (1);
	}
	bzero(buffer,size);
	
	intbuffer = (unsigned long *)buffer;
	
	if (setsockopt (blasterSock, SOL_SOCKET, SO_SNDBUF,(char *) &blen, sizeof (blen)) < 0) 
	{
		perror ("setsockopt SO_SNDBUF failed");
		free (buffer);
		exit (1);
	}

	if (setsockopt (blasterSock, SOL_SOCKET, SO_RCVBUF, (char *) &blen, sizeof (blen)) < 0) 
	{
		perror ("setsockopt SO_RCVBUF failed");
		free (buffer);
		exit (1);
	}

	if (dotcp) 
	{

		if (connect (blasterSock, (struct sockaddr *) &sin, sizeof (sin)) < 0) 
		{
			perror ("connect failed");
			printf ("host %s port %d\n", inet_ntoa (sin.sin_addr),ntohs (sin.sin_port));
			free (buffer);
			exit (1);
		}
	}
	
	blasterStop = FALSE;
	
	/*for (i= 0 ; i < 0xfffffff;i++)*/
	while (1)
	{
		int nBytes;
		
		{	/* delay*/
			/*
			for (j = 0 ; j < (0x3e0*2) ; j++ )
			{
				t=j;
			}
			*/
		}
	
		if (blasterStop == TRUE)
		{
			close (blasterSock);
			free (buffer);
			printf ("blaster exit.\n");
			return;
		}
		
		intbuffer[0] = htonl(countpp);
		intbuffer[(size/sizeof(unsigned long))-1] = htonl(countmm);
		
		countpp++;
		countmm--;
		
		if (dotcp) 
		{
			if ((nBytes = write(blasterSock, buffer, size)) < 0) 
			{
				perror ("blaster write error");
				printf("tcp snd err: %d \n", nBytes);
				break;
			}
			else
			{
				if (log_tx == 1)
				{
					printf("tx-nBytes: %d \n", nBytes);
				}
			}
			
			vxbUsDelay(1);
		}
		else 
		{
			if ((nBytes = sendto(blasterSock, buffer, size, 0, (struct sockaddr *)&sin,sizeof(sin))) < 0) 
			{
				perror("blaster write error");
				break;
			}
		}
	}
	
	close (blasterSock);
	free (buffer);
}




void tcpcliinit ( char    *targetAddr, int port,int dotcp, int size,int blen, int delay )
{

	struct sockaddr_in	sin;
	int    sock;
	char   *buffer;
	char   *rcvbuffer;
	
	char   * ptr ; 
	unsigned long *intbuffer;
	int enable_broadcast = 1;
	unsigned long countpp = 0, countmm = 0;
	
	UINT32 beginTimeH,beginTimeL, endTimeH,endTimeL; 
	
	int i ; 
	int j ;
	int k ;
	int kk = 0 ; 

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
		  printf ("cannot allocate receive buffer of size %x", size );
	  exit (1);
	}
	
	if ((rcvbuffer = (char *) malloc (size)) == NULL) 
	{
	  printf ("cannot allocate receive buffer of size %x", size );
	  exit (1);
	}	
	
	
	bzero(buffer,size);
	bzero(rcvbuffer,size);
	
	intbuffer = (unsigned long *)buffer;
	
	if (setsockopt (sock, SOL_SOCKET, SO_SNDBUF,(char *) &blen, sizeof (blen)) < 0) 
	{
		perror ("setsockopt SO_SNDBUF failed");
		free (buffer);
		free (rcvbuffer);
		exit (1);
	}

	if (setsockopt (sock, SOL_SOCKET, SO_RCVBUF, (char *) &blen, sizeof (blen)) < 0) 
	{
		perror ("setsockopt SO_RCVBUF failed");
		free (buffer);
		free (rcvbuffer);		
		exit (1);
	}

	if (dotcp) 
	{

		if (connect (sock, (struct sockaddr *) &sin, sizeof (sin)) < 0) 
		{
			perror ("...............connect failed");
			printf ("host %s port %d\n", inet_ntoa (sin.sin_addr),ntohs (sin.sin_port));
			free (buffer);
			free (rcvbuffer);			
			exit (1);
		}
	}
	
	blasterStop = FALSE;
	
	for (;;) 
	{
		int nBytes;
		if (dotcp == 0 )
		{
				for (j = 0 ; j < 0x3e0 ; j++ )
				{
					j=j;
				}
		}
		
		if (blasterStop == TRUE)  break;
		intbuffer[0] = htonl(countpp);
		intbuffer[(size/sizeof(unsigned long))-1] = htonl(countmm);
		countpp++;
		countmm--;
		
		ptr = buffer ;
		
		for (i = 0 ; i < size ; i++)
		{
			
			*ptr ++ = 'c' ; 
			
		}
		
		if (dotcp)
		{
			if ((nBytes = write(sock, buffer, size)) < 0) 
			{
				perror ("blaster write error");
				break;
			}
    		if ((kk % 0x200) ==0 ) 		
    			{
    			}
    		kk ++ ; 
			
			ptr = buffer ;
			
			for (i = 0 ; i < size ; i++)
			{
				
				*ptr ++ = 'b' ; 
				
			}			
			
	 								
			if ((nBytes = read(sock, buffer, size)) < 0) 
			{
				perror ("blaster write error");
				break;
			}
						
						
			ptr = buffer ;
			
			for (k = 0 ; k <nBytes; k++ )
			{
			}			
			
		
		}
		else  
		{
			if ((nBytes = sendto(sock, buffer, size, 0, (struct sockaddr *)&sin,sizeof(sin))) < 0) 
			{
				perror("blaster write error\n");
				break;
			}
		}
	}
	
	close (sock);
	free (buffer);
	printf ("blaster exit.\n");
}


extern char * netdstaddr ;
extern char * netdstmac ; 

extern char * netsrcaddr ;
extern char * netsrcmac ; 





char * svrip = "192.168.3.11";
char * cliip = "192.168.3.10";

char * svrmac = "00:a0:1e:ab:aa:a0" ; 
char * climac = "00:a0:1e:aa:aa:a0" ; 

void testUdpSnd(char *targetAddr)
{
    UINT32 ip = 0;
	
	ip = inet_addr (targetAddr);
	taskSpawn("tUdpSnd",200,0,20000,(FUNCPTR)demoBlaster, ip, 7000,0,0x400,0x100000,0,0,0,0,0);
}


extern STATUS 	arpAdd (char *host, char *eaddr, int flags);

void testTcpSnd(char *targetAddr)
{
    UINT32 ip = 0;
	ip = inet_addr (targetAddr);
	taskSpawn("tTcpSnd",200,0,20000,(FUNCPTR)demoBlaster,ip,7000,1,0x400,0x100000,0,0,0,0,0);
}


void tcpclient( char * serverip )
{
	
	char * svripaddr =  "192.168.3.11";
	
	if (serverip != NULL ) 
	{
		svripaddr = serverip ; 		
	}
	
  
	taskSpawn ("tTcpSnd",200,0,20000,(FUNCPTR)tcpcliinit,(int)svripaddr,SVRPORT,TCPPROTOCAL, DATASIZE ,0x100000,0,0,0,0,0);
}



 


