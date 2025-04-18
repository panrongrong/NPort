/* vxPing.c - send ICMP ECHO_REPLY packets to a particular network host*/  
 
 /* Copyright 1984-2001 Wind River Systems, Inc. */
 
 /*
 modification history
 --------------------
 01j,09aug01,vvv close socket on error; replaced LSB4 (ipPtr->ip_v_hl) with
 		ipPtr->ip_hl
 01i,14nov97,ns removed packetSize from the function xmtMsg
 01h,14nov97,ns added bzero in constructIcmpMsg function 
 01g,14nov97,ns initalized msgNum to 0 , removed u_char pointer from 
             constructIcmpMsg function
 01f,06nov97,mm added copyright.
 01e,10Oct97,mm cast arg 1 of taskIdFigure
 01d,10Oct97,mm definded transmitEchoMessage, xmtMsg and configureSocket
 01c,10Oct97,mm included "netShow.h", "usrLib.h", "hostLib.h", <stdio.h>,                        <string.h> and "sockLib.h"
 01b,10Oct97,mm changed ip_hl to ip_v_hl 
 01a,18Feb94,ms extensively modified for VxDemo.   
 */
 
 /* INCLUDES */
 #include "vxWorks.h"
 #include "socket.h"
 #include "sockLib.h"
 #include "netinet/in_systm.h"
 #include "in.h"
 #include "netinet/ip.h"
 #include "netinet/ip_icmp.h"
 #include "taskLib.h"
 #include "netShow.h"
 #include "usrLib.h"
 #include "hostLib.h"
 #include <stdio.h>
 #include <string.h>
 #include "inetLib.h"
 
 #define TASK_PRI       100   /* Priority of spawned tasks */
 #define TASK_STACK_SIZE  10000 /* stack size of the spawned tasks */
 #define ICMP_HDRLEN      sizeof (struct icmp)
 #define MY_ICMP_ID     0x1234
  
 /* globals */
 LOCAL int        msgNum = 0 ;
 LOCAL struct sockaddr_in  destAddr;
 LOCAL int        sockFd;
 LOCAL char       dstHost [20];  /* Name of the remote host to ping */
 LOCAL int        numReceived = 0 ;
 LOCAL int        origNumPackets;
 LOCAL int        numPkts;
 
 /* forward declarations */
 LOCAL STATUS startPing();
 STATUS rcvPings ();
 LOCAL void constructIcmpMsg (struct icmp  *);
 LOCAL void pingResponse ();
 LOCAL u_short inCksm (u_short *);
 
 STATUS transmitEchoMessage();
 STATUS xmtMsg(struct icmp* msg);
 STATUS configureSocket(char* nameofHost);

 UINT32 tick_start, tick_end;

 
/* struct Ip_timeval tv1; */
/* struct Ip_timeval tv2;*/
 
 struct timespec ts1; 
 struct timespec ts2;
 
 /* Needs to be compiled with -DUSE_BITFIELDS flag */
 
 /*****************************************************************************
  * vxPing - send ICMP ECHO_REPLY packets to a particular network host to 
  *        elicit an ICMP ECHO_RESPONSE from that specified network host
  *        and prints "<host> is alive" on the standard output if the host
  *        responds, otherwise prints ICMP error messages.
  *        
  *        When using ping, make sure the host you want to ping is already
  *        in the host table. If not, add the host to the host table using
  *        hostAdd. If the host you want to ping is in a different network
  *        than your VxWorks system, make sure route is established to that
  *        host. If route is not already established, use routeAdd to add
  *        the route. 
  *
  *  EXAMPLE:
  *
  *     To run ping, from the VxWorks shell do as follows:
  *     -> vxPing ("nameOfTheHost", count)
  *
  *     where nameOfTheHost is the name of the host you want to ping and 
  *     count is the number of requests to send ( example: ping ("sevana", 5) )
  */
 
 STATUS vxPing
    (
    char *hostName,    /* Name of the host to ping */
    int numPackets     /* Number of requests to send */
    )
 
    {
    int num_packs;
    int status;
 
    /* get user's parameters */
    if (hostName == NULL) 
        {
        printf ("Must enter a host name to Ping\n") ;
        return (ERROR);
        }
 
    if (numPackets > 1) 
        num_packs = numPackets;
    else
        num_packs = 1;
 
    numPkts = num_packs;
    origNumPackets = num_packs;
    msgNum = 0;
 
    /* configure for network communications */
    /*status = configureSocket (hostName) ;*/
    status = configureSocket2 (hostName) ;  /* hostNmae: ip_addr, such as: 192.168.46.77*/
    
    if (status == ERROR)
       {
       printf ("error while trying to configure socket\n") ;
       return (ERROR);
       }
 
    if (taskSpawn ("tStartPing", TASK_PRI, 0, TASK_STACK_SIZE, 
                   (FUNCPTR) startPing, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
       {
       close (sockFd);
       perror ("spawning startPing task filed");
       return (ERROR);
       }
 
    if (taskSpawn ("tRcvPing", TASK_PRI, 0, TASK_STACK_SIZE, 
                   (FUNCPTR) rcvPings, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
       {
       close (sockFd);
       perror ("spawning rcvPings task filed");
       return (ERROR);
       }
 
    return (OK);
    }
 
 
 /*****************************************************************************
  * configureSocket - Configures socket for ICMP protocol 
  *
  * RETURNS : OK or ERROR
  *
  */
 
 STATUS configureSocket
     (
    char *nameOfHost
     )
     {
     printf ("Name of the host to ping is -> %s\n", nameOfHost);
 
     bzero ((char *) &destAddr, sizeof (destAddr));
	 
     destAddr.sin_family = AF_INET ;
	 
     if ((destAddr.sin_addr.s_addr = hostGetByName (nameOfHost)) == ERROR)
         {
         if ( (destAddr.sin_addr.s_addr = inet_addr (nameOfHost)) == ERROR)
             {
             perror ("bad host!");
             return (ERROR) ;
             }
         }
 
     strcpy (dstHost, nameOfHost);
 
     sockFd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
     if (sockFd < 0)
         {
         perror ("socket failed in configure_socket");
         return (ERROR);
         }
     else
         {
         return (OK) ;
         }
     }
 
  STATUS configureSocket2
     (
    char * ip_addr
     )
     {
     printf ("Name of the host to ping is -> %s\n", ip_addr);
 
     bzero ((char *) &destAddr, sizeof (destAddr));
	 
     destAddr.sin_family = AF_INET ;
	 
     if ((destAddr.sin_addr.s_addr = inet_addr(ip_addr)) == ERROR)
         {
         if ( (destAddr.sin_addr.s_addr = inet_addr(ip_addr)) == ERROR)
             {
             perror ("bad host!");
             return (ERROR) ;
             }
         }
 
     strcpy (dstHost, ip_addr);
 
     sockFd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
     if (sockFd < 0)
         {
         perror ("socket failed in configure_socket");
         return (ERROR);
         }
     else
         {
         return (OK) ;
         }
     }
 /*****************************************************************************
  * startPing - Initiates ping
  *
  * RETURNS : OK or ERROR
  *
  */
 
LOCAL STATUS startPing()
{ 
     while (1)
     {
         if (numPkts > 0)
         {
             numPkts--;
			 
			 tick_start = tickGet();  /* jc*/
			 
			/* struct timespec ts;*/
			 (void)clock_gettime(CLOCK_MONOTONIC, &ts1);
			 /*tv1->tv_sec  = ts.tv_sec;*/
			 /*tv1->tv_usec = ts.tv_nsec / 1000;*/
			 
             if (transmitEchoMessage() == ERROR)
             {
                 printf ("Error in transmitEchoMessage\n");
                 return (ERROR);
             }
         }
         else
             break ;
		 
	 	taskDelay(5) ;
     }
	 
	taskDelay(5) ;
    pingResponse ();
	 
    return (OK);
}
 
 /*****************************************************************************
  * transmitEchoMessage - Fill ICMP header and transmit ECHO_REQUEST datagrams
  *
  * RETURNS : OK or ERROR
  *
  */
 
 STATUS transmitEchoMessage()
{
    struct icmp  icmpMsg;
 
    constructIcmpMsg (&icmpMsg);
	
    if (xmtMsg (&icmpMsg) == ERROR)
        return (ERROR);
	
    return (OK);
} 
 
 
 /*****************************************************************************
  * constructIcmpMsg - Fills ICMP header
  *
  */
 
 LOCAL void constructIcmpMsg
     (
     struct icmp  *icmpMessage
     )
     {
     bzero ( (char *) icmpMessage, sizeof (struct icmp) );
 
     icmpMessage->icmp_type = ICMP_ECHO;
     icmpMessage->icmp_code = 0;
     icmpMessage->icmp_cksum = 0;
     icmpMessage->icmp_id = MY_ICMP_ID;
     icmpMessage->icmp_seq = msgNum++;
 
     icmpMessage->icmp_cksum = inCksm ((u_short *) icmpMessage);
     }
 
 /*****************************************************************************
  * xmtMsg - transmit ECHO_REQUEST datagrams
  *
  * RETURNS : OK or ERROR
  *
  */
 
STATUS xmtMsg
     (
     struct icmp *msg 
     ) 
{
	int result;
	int tx_len = ICMP_HDRLEN; /* */

	/*if ((result = sendto (sockFd, (caddr_t) msg, ICMP_HDRLEN, 0, 
	       (struct sockaddr *) &destAddr, sizeof(destAddr))) < 0) 
	       */
   /*
   ssize_t sendto (
	   int			   s,
	   const void	   *buf,
	   size_t		   bufLen,
	   int			   flags,
	   const struct sockaddr *to,
	   socklen_t	   tolen )
	*/
	if ((result = sendto (sockFd, (caddr_t) msg, tx_len, 0, 
	       (struct sockaddr *) &destAddr, sizeof(destAddr))) < 0) 
	{
	    perror ("sendto error");
	    return (ERROR);
	}
	else 
	{   
		if (result != sizeof (msg))
	    {    
	    	printf ("icmp_tx %s %d bytes, return = %d\n", dstHost, 
	                  (int)tx_len, result);
		}
	}
	
	return (OK);
}
 
 
 /*****************************************************************************
  * rcvPings - receive ICMP ECHO_RESPONSE from the specified network host
  *
  */
 
STATUS rcvPings()
{
	int                 remAddrlen;
	struct sockaddr_in  remAddr;
	int                 iphdrlen;
	struct ip           *ipPtr;
	struct icmp         *icmpPtr;
	u_char     recvpack[4096] ;
	int rx_len = 0;

	numReceived = 0 ;
	remAddrlen = sizeof(remAddr) ;

	while (1)  /* loop until we break out */
	{
	#if 0
		int recvfrom
			  (
			  int				 sd,	   /* socket descriptor of receiving socket */
			  char *			 buf,	   /* pointer to buffer for receiving data */
			  int				 bufLen,   /* length of buffer */
			  int				 flags,    /* flags to underlying protocols */
			  struct sockaddr *  from,	   /* return parameter for sender's address */
			  int * 			 fromLen   /* return param for length of address */
			  )
	#endif
		if ((rx_len = recvfrom (sockFd, recvpack, sizeof (recvpack), 0, 
			            (struct sockaddr *) &remAddr, &remAddrlen)) < 0)
		{
			perror ("recvfrom error");
			return (ERROR);
		}
		else
		{
			printf("icmp rx: %d bytes \n", rx_len);
			
			ipPtr = (struct ip *) recvpack ;
			iphdrlen = ipPtr->ip_hl << 2;
			icmpPtr = (struct icmp *) (recvpack + iphdrlen);
			
			if ((icmpPtr->icmp_type == ICMP_ECHOREPLY) 
			     && (icmpPtr->icmp_id == MY_ICMP_ID))
			{
				numReceived++ ;

				/* jc*/
				tick_end = tickGet();
				printf("time: (%d)%d ms ", (tick_end-tick_start), \
					   ((tick_end-tick_start)*1000/sysClkRateGet()));

				/*struct timespec ts;*/
				(void)clock_gettime(CLOCK_MONOTONIC, &ts2);
				/*tv2->tv_sec  = ts.tv_sec;*/
				/*tv2->tv_usec = ts.tv_nsec / 1000;*/
				printf("- %d ns \n", (ts2.tv_nsec - ts1.tv_nsec));
					   
				
				if (numReceived == origNumPackets)
				{
					close (sockFd);
					return (OK);
				}
			}
		}
	}
}
 
 /*****************************************************************************
  * pingResponse - prints "<host> is alive" on the standard output if the host
  *                responds, otherwise prints ICMP error messages.
  *        
  */
 
 LOCAL void pingResponse()
{
    if (numReceived == origNumPackets)
         printf("\nhost %s is alive\n", dstHost);
    else
	{
		printf("\ntrouble response with %s: snd %d packet%c, received %d packet%c\n\n",
		      dstHost, origNumPackets, ((origNumPackets != 1) ? 's' : ' '),
		      numReceived, ((numReceived != 1) ? 's' : ' ') ) ;

		/* icmpstatShow ();*/

		if (taskDelete (taskIdFigure ((int)"tRcvPing")) == ERROR)
		{
			close (sockFd);
			perror ("Error in taskDelete");
			return;
		}
    }
	
 	close (sockFd);
}
 
 
 /*****************************************************************************
  * inCksm - compute checksum
  *        
  * RETURNS the calculated checksum (ones complement) of the icmp struct
  */
 
 u_short inCksm
     (
     register u_short  *msg_ptr
     )
     
     {
     register long    sum;
     register u_short answer;
     int              index;
 
     sum = 0 ;
 
     for (index = 0 ; index < 4 ; index++)
         {
         sum = sum + *msg_ptr ;
         msg_ptr++ ;
         }
 
     sum = (sum >> 16) + (sum & 0xffff) ;
     sum = sum + (sum >> 16) ;
     answer = ~sum ;
     return (answer) ;
     }
