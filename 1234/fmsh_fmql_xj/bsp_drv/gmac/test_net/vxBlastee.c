/* vxBlastee.c - reads given size message from the client*/
 
/*
  Jan 5th 1997:  Dr. Stephen Adler, Brookhaven National Laboratory,
                 Modified to include upd transfers and the
                 checking of sequential buffer ordering.
                 Note: udp not fully debugged....

01a,29jan94,ms   cleaned up for VxDemo.
11,20jun95,ldt   cleaned up for 511 to elminate warnings.
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

#define AlarmInterval 10
LOCAL unsigned int	blastNum;
LOCAL int 	wdIntvl = AlarmInterval;

#define	False	0
#define True	1
LOCAL int errorcnt[2] = {0,0};
LOCAL int goodcnt[2] = {0,0};
LOCAL WDOG_ID	blastWd = NULL;
int	blasteeStop;
int blasteeSock = 0;


LOCAL void demoBlastRate ();

/*****************************************************************************
 * blastee - server task for VxWorks target
 *
 * DESCRIPTION
 *
 *     This is a server program which communicates with client through a
 *     TCP socket. It allows to configure the maximum size of socket-level 
 *     receive buffer. It repeatedly reads a given size message from the client
 *     and reports the number of bytes read every minute. It stops receiving 
 *     the message when the global variable blasteeStop is set to 1. 
 *
 * EXAMPLE:
 *
 *     To run this blastee task from the VxWorks shell do as follows: 
 *     -> sp (demoBlastee, 7000, 1, 0x400, 16000); for tcp blasteeing...
 *     -> sp (demoBlastee, 7000, 0, 0x400, 16000); for udp blasteeing...
 *     
 *        sp (demoBlastee, 7000, 1, 0x2000, 16000); for tcp blasteeing...
 *     -> sp (demoBlastee, 7000, 0, 0x2000, 16000); for udp blasteeing...
 *     
 *     sp (demoBlastee, 7000, 1, 0x8000, 64000); for tcp blasteeing...
 *     -> sp (demoBlastee, 7000, 0, 0x8000, 64000); for udp blasteeing...
 *     
 *
 *     To stop blastee task from the VxWorks shell do as follows: 
 *     -> blasteeStop = 1 
 *
 */

static  void checktimer (void)
{
	
	
	UINT32 beginTimeH,beginTimeL, endTimeH,endTimeL;
	sysGetTime(&endTimeH,&endTimeL);
	logMsg("timer H = 0x%x, timer L is 0x%x \n",endTimeH, endTimeL ,3, 4, 5,6  );
	wdStart (blastWd, sysClkRateGet () * 10, (FUNCPTR) checktimer, 0);
	
	
}

static void wdTask (     int   port,      int   dotcp,     int   size,     int   blen) 
{
	struct sockaddr_in	serverAddr; /* server's address */
	struct sockaddr_in  clientAddr; /* client's address */
	char   *buffer, *buffpnt;
	int bufsize, readsize;
	unsigned long *intbuffer;
	int	   sock;
	int    snew;
	int    len;
	unsigned long countpp=0, countmm=0,firstData=0,lastData=0;

	blastWd = wdCreate (); 
	if ( blastWd  == NULL) 
	{
		perror ("cannot create blast watchdog");
		exit (1);
	}

	

		wdStart (blastWd, sysClkRateGet () * 5, (FUNCPTR) checktimer, 0);

			for (;;)
			{
				
				
			}
					
		wdCancel (blastWd);
			
		
}

int log_rx = 0;
void open_log(void)
{
	log_rx = 1;
}

void close_log(void)
{
	log_rx = 0;
}

int udpFlag = 0;
void demoBlastee 
(
    int   port,  /* the port number to read from */
    int   dotcp, /* dotcp = 1, for tcp, dotcp = 0 for udp */
    int   size,  /* size of the meesage */
    int   blen   /* maximum size of socket-level receive buffer */
)
{
	struct sockaddr_in	serverAddr; /* server's address */
	struct sockaddr_in  clientAddr; /* client's address */
	char   *buffer, *buffpnt;
	int bufsize, readsize;
	unsigned long *intbuffer;
	int    snew;
	int    len;
	unsigned long countpp=0, countmm=0,firstData=0,lastData=0;
	
	
	
	buffer = (char *) malloc (size);
	
	if (buffer == NULL) 
	{
	perror ("cannot allocate buffer of size ");
	exit (1);
	}
	
	intbuffer = (unsigned long *)buffer;
	bzero(buffer,size);

	/* Create watchdog timer */
	if (blastWd == NULL && (blastWd = wdCreate ()) == NULL) 
	{
		perror ("cannot create blast watchdog");
		free (buffer);
		exit (1);
	}

	/* Start watchdog after a minute*/		
	wdStart (blastWd, sysClkRateGet () * wdIntvl, (FUNCPTR) demoBlastRate, 0);
	

	/* Zero out the sock_addr structures.
	 * This MUST be done before the socket calls.
	 */	
	bzero ((char *) &serverAddr, sizeof (serverAddr));
	bzero ((char *) &clientAddr, sizeof (clientAddr));

	/* Open the socket. Use ARPA Internet address format and stream sockets. */
	blasteeSock = socket (AF_INET, dotcp ? SOCK_STREAM : SOCK_DGRAM, 0);

	if (blasteeSock < 0) 
	{
		perror ("cannot open socket");
		free (buffer);
		exit (1);
	}

	/* Set up our internet address, and bind it so the client can connect. */
	serverAddr.sin_family	= AF_INET;
	serverAddr.sin_port	    = htons (port);
  
	if (bind (blasteeSock, (struct sockaddr *)&serverAddr, sizeof (serverAddr)) < 0) 
	{
		perror ("bind error");
		free (buffer);
		exit (1);
	}

	if (dotcp) 
	{
		/* Listen, for the client to connect to us. */
		if (listen (blasteeSock, 2) < 0) 
		{
			perror ("listen failed");
			free (buffer);
			exit (1);
		}

		len = sizeof (clientAddr);

		snew = accept (blasteeSock, (struct sockaddr *) &clientAddr, &len);
		if (snew == ERROR) 
		{
			printf ("accept failed");
			close (blasteeSock);
			exit (1);
		}
		
		/* maximum size of socket-level receive buffer */
		if (setsockopt (snew, SOL_SOCKET, SO_RCVBUF,(char *) &blen, sizeof (blen)) < 0) 
		{
			perror ("setsockopt SO_RCVBUF failed");
			free (buffer);
			exit (1);
		}

		if (setsockopt (snew, SOL_SOCKET, SO_SNDBUF, (char *) &blen, sizeof (blen)) < 0)
		{
			perror ("setsockopt SO_SNDBUF failed");
			free (buffer);
			exit (1);
		}
	} 	
	else    
	{

		if (setsockopt (blasteeSock, SOL_SOCKET, SO_RCVBUF,(char *) &blen, sizeof (blen)) < 0) 
		{
			perror ("setsockopt SO_RCVBUF failed");
			free (buffer);
			exit (1);
		}

		if (setsockopt (blasteeSock, SOL_SOCKET, SO_SNDBUF, (char *) &blen, sizeof (blen)) < 0) 
		{
			perror ("setsockopt SO_SNDBUF failed");
			free (buffer);
			exit (1);
		}

	}

	blasteeStop = False;
	blastNum = 0;
	
	buffpnt = buffer;
	bufsize = 0;
	readsize = size;
    
	while (!blasteeStop) 
	{
		int numRead;

		intbuffer[0]=0;
		intbuffer[(size/sizeof(unsigned long))-1]=0;
		if (dotcp) 
		{
			if ((numRead = read (snew, buffpnt, readsize)) < 0) 
			{
				perror ("blastee read error");
				break;
			} 
			else 
			{
				blastNum += numRead;
				
				if (log_rx == 1)
				{
					printf("blastNum:%d-%d \n", blastNum, numRead);
				}
				
				buffpnt += numRead;
				bufsize += numRead;
				readsize -= numRead;
			}

		} 
		else 
		{
			len = sizeof(clientAddr);
			if ((numRead = recvfrom(blasteeSock, buffpnt, readsize, 0,(struct sockaddr *)&clientAddr, &len)) < 0) 
			{
				perror("recvfrom");
				break;
			} 
			else 
			{
				blastNum += numRead;
				
				if (log_rx == 1)
				{
					printf("blastNum:%d-%d \n", blastNum, numRead);
				}
				
				buffpnt += numRead;
				bufsize += numRead;
				readsize -= numRead;
			}
		}

		if (bufsize == size) 
		{

			firstData = ntohl(intbuffer[0]);
			lastData =ntohl(intbuffer[(size/sizeof(unsigned long))-1]) ;

			if ((ntohl(intbuffer[0])!=countpp)&&(ntohl(intbuffer[(size/sizeof(unsigned long))-1])!=countmm)) 
			{
				errorcnt[0]++;
				errorcnt[1]++;
				countpp = ntohl(intbuffer[0]);
				countmm = ntohl(intbuffer[(size/sizeof(unsigned long))-1]);
			} else 
			{
				goodcnt[0]++;
				goodcnt[1]++;
			}
			
			countpp++;
			countmm--;
      
			buffpnt = buffer;
			bufsize = 0;
			readsize = size;
		}
	}

	wdCancel (blastWd);
    errorcnt[0]=errorcnt[1]=goodcnt[0]=goodcnt[1] = 0;
	close (blasteeSock);
	if (dotcp) close (snew);
    
	free (buffer);
}


void demoBlastee2 
(
    int   port,  /* the port number to read from */
    int   dotcp, /* dotcp = 1, for tcp, dotcp = 0 for udp */
    int   size,  /* size of the meesage */
    int   blen   /* maximum size of socket-level receive buffer */
)
{
	struct sockaddr_in	serverAddr; /* server's address */
	struct sockaddr_in  clientAddr; /* client's address */
	char   *buffer, *buffpnt;
	int bufsize, readsize;
	unsigned long *intbuffer;
	int	   sock;
	int    snew;
	int    len;
	unsigned long countpp=0, countmm=0,firstData=0,lastData=0;;

	buffer = (char *) malloc (size);
	
	if (buffer == NULL) 
	{
	perror ("cannot allocate buffer of size ");
	exit (1);
	}
	
	intbuffer = (unsigned long *)buffer;
	bzero(buffer,size);

	/* Create watchdog timer */
	if (blastWd == NULL && (blastWd = wdCreate ()) == NULL) 
	{
		perror ("cannot create blast watchdog");
		free (buffer);
		exit (1);
	}

	/* Start watchdog after a minute*/
	wdStart (blastWd, sysClkRateGet () * wdIntvl, (FUNCPTR) demoBlastRate, 0);

	/* Zero out the sock_addr structures.
	 * This MUST be done before the socket calls.
	 */
	bzero ((char *) &serverAddr, sizeof (serverAddr));
	bzero ((char *) &clientAddr, sizeof (clientAddr));

	/* Open the socket. Use ARPA Internet address format and stream sockets. */
	sock = socket (AF_INET, dotcp ? SOCK_STREAM : SOCK_DGRAM, 0);

	if (sock < 0) 
	{
		perror ("cannot open socket");
		free (buffer);
		exit (1);
	}

	/* Set up our internet address, and bind it so the client can connect. */
	serverAddr.sin_family	= AF_INET;
	serverAddr.sin_port	        = htons (port);
  
  

	if (bind (sock, (struct sockaddr *)&serverAddr, sizeof (serverAddr)) < 0) 
	{
		perror ("bind error");
		free (buffer);
		exit (1);
	}

	if (dotcp) 
	{
		/* Listen, for the client to connect to us. */
		if (listen (sock, 2) < 0) 
		{
			perror ("listen failed");
			free (buffer);
			exit (1);
		}

		len = sizeof (clientAddr);

		snew = accept (sock, (struct sockaddr *) &clientAddr, &len);
		if (snew == ERROR) 
		{
			printf ("accept failed");
			close (sock);
			exit (1);
		}
		
		/* maximum size of socket-level receive buffer */
		if (setsockopt (snew, SOL_SOCKET, SO_RCVBUF,(char *) &blen, sizeof (blen)) < 0) 
		{
			perror ("setsockopt SO_RCVBUF failed");
			free (buffer);
			exit (1);
		}

		if (setsockopt (snew, SOL_SOCKET, SO_SNDBUF, (char *) &blen, sizeof (blen)) < 0)
		{
			perror ("setsockopt SO_SNDBUF failed");
			free (buffer);
			exit (1);
		}
	} 	else    
	{

		if (setsockopt (sock, SOL_SOCKET, SO_RCVBUF,(char *) &blen, sizeof (blen)) < 0) 
		{
			perror ("setsockopt SO_RCVBUF failed");
			free (buffer);
			exit (1);
		}

		if (setsockopt (sock, SOL_SOCKET, SO_SNDBUF, (char *) &blen, sizeof (blen)) < 0) 
		{
			perror ("setsockopt SO_SNDBUF failed");
			free (buffer);
			exit (1);
		}

	}

	blasteeStop = False;
	blastNum = 0;
	
	buffpnt = buffer;
	bufsize = 0;
	readsize = size;

	for (;;) 
	{
		int numRead;
		
		if (blasteeStop == True) 
			break;
		
		intbuffer[0]=0;
		intbuffer[(size/sizeof(unsigned long))-1]=0;
		
		if (dotcp) 
		{
			if ((numRead = read (snew, buffpnt, readsize)) < 0) 
			{
				perror ("blastee read error");
				break;
			} 
			else 
			{
				blastNum += numRead;
				
				buffpnt += numRead;
				bufsize += numRead;
				readsize -= numRead;
			}

		} 
		else 
		{
			len = sizeof(clientAddr);
			if ((numRead = recvfrom(sock, buffpnt, readsize, 0,(struct sockaddr *)&clientAddr, &len)) < 0) 
			{
				perror("recvfrom");
				break;
			} 
			else 
			{
				blastNum += numRead;
				
				buffpnt += numRead;
				bufsize += numRead;
				readsize -= numRead;
			}
		}

		if (bufsize == size) 
		{

			firstData = ntohl(intbuffer[0]);
			lastData =ntohl(intbuffer[(size/sizeof(unsigned long))-1]) ;

			goodcnt[0]++;
			goodcnt[1]++;
			
			countpp++;
			countmm--;
      
			buffpnt = buffer;
			bufsize = 0;
			readsize = size;
		}
	}

	wdCancel (blastWd);

	close (sock);
	if (dotcp) close (snew);
    
	free (buffer);
	printf ("blastee end.\n");
}



/*****************************************************************************
 * blastRate - watchdog routine executed every one minute to report number of
 *             bytes read   
 *
 */
LOCAL void demoBlastRate ()
{

	if (blastNum > 0) 
	{
		if(udpFlag == 1)
		{
			logMsg ("<udp> %d Mbytes/sec; errorcnt= %d,%d, goodcnt= %d,%d\r\n", 
						blastNum/wdIntvl/1024/1024, 0, 0, goodcnt[0], goodcnt[1],6);
			
			errorcnt[0] = 0;
			/*udpFlag = 0;*/
			errorcnt[1] = 0;
		}
		else
		{
			logMsg ("<tcp> %d Mbytes/sec; errorcnt= %d,%d, goodcnt= %d,%d\r\n", 
						blastNum/wdIntvl/1024/1024, errorcnt[0], errorcnt[1], goodcnt[0], goodcnt[1],6);
		}
		
		blastNum = 0;
		errorcnt[1] = 0;
		goodcnt[1] = 0;
	}
	else
	{
		if (udpFlag == 1)
		{
			logMsg ("udp: No bytes read in the last %d seconds.\n", wdIntvl, 0, 0, 0, 0, 0);
		}
		else
		{
			logMsg ("tcp: No bytes read in the last %d seconds.\n", wdIntvl, 0, 0, 0, 0, 0);
		}
	}
	
	if (blasteeStop==0)
	{
		wdStart (blastWd, sysClkRateGet () * wdIntvl, (FUNCPTR) demoBlastRate, 0);
	}
}


int  testUdpRcv()
{
	udpFlag = 1;
	taskSpawn ("tUdpRcv", 200, 0, 20000,(FUNCPTR)demoBlastee,7000,0,0x400,0x100000,0,0,0,0,0,0);
	return 0 ;
	
}


int testTcpRcv()
{
	taskSpawn ("tTcpRcv", 200, 0, 20000,(FUNCPTR)demoBlastee,7000,1,0x400,0x100000,0,0,0,0,0,0);
	return 0 ;
}


void tcpserverinit 
(
    int   port,  /* the port number to read from */
    int   dotcp, /* dotcp = 1, for tcp, dotcp = 0 for udp */
    int   size,  /* size of the meesage */
    int   blen   /* maximum size of socket-level receive buffer */
)
{
	struct sockaddr_in	serverAddr; /* server's address */
	struct sockaddr_in  clientAddr; /* client's address */
	
	char   *buffer, *buffpnt;
	int bufsize, readsize;
	unsigned long *intbuffer;
	int	   sock;
	int    snew;
	int    len;
	unsigned long countpp=0, countmm=0,firstData=0,lastData=0;;

	
	
	buffer = (char *) malloc (size);
	
	if (buffer == NULL) 
	{
	perror ("cannot allocate buffer of size ");
	exit (1);
	}
	
	intbuffer = (unsigned long *)buffer;
	bzero(buffer,size);

	/* Create watchdog timer */
	if (blastWd == NULL && (blastWd = wdCreate ()) == NULL) 
	{
		perror ("cannot create blast watchdog");
		free (buffer);
		exit (1);
	}

	/* Start watchdog after a minute*/
	
		
	wdStart (blastWd, sysClkRateGet () * wdIntvl, (FUNCPTR) demoBlastRate, 0);
	

	/* Zero out the sock_addr structures.
	 * This MUST be done before the socket calls.
	 */
	
	
	
	bzero ((char *) &serverAddr, sizeof (serverAddr));
	bzero ((char *) &clientAddr, sizeof (clientAddr));

	/* Open the socket. Use ARPA Internet address format and stream sockets. */
	sock = socket (AF_INET, dotcp ? SOCK_STREAM : SOCK_DGRAM, 0);

	
	
	if (sock < 0) 
	{
		perror ("cannot open socket");
		free (buffer);
		exit (1);
	}

	/* Set up our internet address, and bind it so the client can connect. */
	serverAddr.sin_family	= AF_INET;
	serverAddr.sin_port	        = htons (port);
  
  

	if (bind (sock, (struct sockaddr *)&serverAddr, sizeof (serverAddr)) < 0) 
	{
		perror ("bind error");
		free (buffer);
		exit (1);
	}

	if (dotcp) 
	{
		/* Listen, for the client to connect to us. */
		if (listen (sock, 2) < 0) 
		{
			perror ("listen failed");
			free (buffer);
			exit (1);
		}

		len = sizeof (clientAddr);

		snew = accept (sock, (struct sockaddr *) &clientAddr, &len);
		if (snew == ERROR) 
		{
			printf ("accept failed");
			close (sock);
			exit (1);
		}
		
		/* maximum size of socket-level receive buffer */
		if (setsockopt (snew, SOL_SOCKET, SO_RCVBUF,(char *) &blen, sizeof (blen)) < 0) 
		{
			perror ("setsockopt SO_RCVBUF failed");
			free (buffer);
			exit (1);
		}

		if (setsockopt (snew, SOL_SOCKET, SO_SNDBUF, (char *) &blen, sizeof (blen)) < 0)
		{
			perror ("setsockopt SO_SNDBUF failed");
			free (buffer);
			exit (1);
		}
	}else  
	{     

		if (setsockopt (sock, SOL_SOCKET, SO_RCVBUF,(char *) &blen, sizeof (blen)) < 0) 
		{
			perror ("setsockopt SO_RCVBUF failed");
			free (buffer);
			exit (1);
		}

		if (setsockopt (sock, SOL_SOCKET, SO_SNDBUF, (char *) &blen, sizeof (blen)) < 0) 
		{
			perror ("setsockopt SO_SNDBUF failed");
			free (buffer);
			exit (1);
		}

	}

	blasteeStop = False;
	blastNum = 0;
	
	buffpnt = buffer;
	bufsize = 0;
	readsize = size;

	for (;;) 
	{
		int numRead;
		
		if (blasteeStop == True) 
			break;
		
		intbuffer[0]=0;
		intbuffer[(size/sizeof(unsigned long))-1]=0;
		
		if (dotcp) 
		{
			if ((numRead = read (snew, buffpnt, readsize)) < 0) 
			{
				perror ("master read error");
				break;
			}else 
			{

			}
			
			if ((numRead = write (snew, buffpnt, numRead)) < 0) 
			{
							perror ("master write error ");
							break;
			}
				
			

		}
		else  
		{
			len = sizeof(clientAddr);
			if ((numRead = recvfrom(sock, buffpnt, readsize, 0,(struct sockaddr *)&clientAddr, &len)) < 0) 
			{
				perror("recvfrom");
				break;
			}
			else 
			{
				blastNum += numRead;
				
				buffpnt += numRead;
				bufsize += numRead;
				readsize -= numRead;
			}
		}

		if (bufsize == size) 
		{

			firstData = ntohl(intbuffer[0]);
			lastData =ntohl(intbuffer[(size/sizeof(unsigned long))-1]) ;


			countpp++;
			countmm--;
      
			buffpnt = buffer;
			bufsize = 0;
			readsize = size;
		}

	}

	wdCancel (blastWd);

	close (sock);
	if (dotcp) close (snew);
    
	free (buffer);
	printf ("blastee end.\n");
}


char * serverip = "192.168.3.11";
char * servermac = "00:a0:1e:ab:aa:a0";
char * clientip = "192.168.3.10";
char * clientmac =  "00:a0:1e:aa:aa:a0" ;


#define MNGPORT 8000 


int tcpserver()
{
	taskSpawn ("tTcpRcv", 200, 0, 20000,(FUNCPTR)tcpserverinit, MNGPORT ,1,DATASIZE,0x100000,0,0,0,0,0,0);
	return 0 ;
}





