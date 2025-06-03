/* pingLib.c - Packet InterNet Grouper (PING) library */

/*
 * Copyright (c) 1994-2006 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
02o,19jun13,h_x  Fix coverity issue
02n,05feb10,h_x  Support 64bit
02m,19sep06,tlu  Clean up for IPNET integration
02l,04feb06,dlk  Fix user-side build warning.
02k,13sep05,vvv  fixed user-mode warning
02j,12apr05,vvv  added note on ping interaction with DNS resolver (SPR
                 #102903); fixed docs
02i,08mar05,dlk  Fix SPRs 106773, 106302, 106028.
02h,16feb05,vvv  removed include of netconf.h for user-side
02g,12jan05,vvv  osdep.h cleanup
02f,08nov04,syy  Doc changes
02e,30aug04,dlk  Replace LOG_ERROR() with log_err(), etc..
02d,24aug04,vvv  fixed warning
02c,23aug04,vvv  merged from COMP_WN_IPV6_BASE6_ITER5_TO_UNIFIED_PRE_MERGE
02b,12may04,rae  Correct timeout behaviour (SPR #95303)
02a,22apr04,asr  Fix compile warnings for PENTIUM2/gnu
01z,06jan04,asr  Fix for SPR# 92869
01y,08dec03,asr  updates based on code review comments
01x,20nov03,niq  osdep.h cleanup
01v,06nov03,rlm  Ran batch header update for header re-org.
01v,14Nov03,asr  Changes required for porting ping to RTP
01u,15may03,spm  Tornado 2.2 FCS merge (SPRs 66062, 69537,72910, 73570; from
                 ver 01r,22apr02,rae: TOR2_2-FCS-COPY label, tor2 branch,
                 /wind/river VOB)
01x,29apr03,asr  Changes resulting from code review. SPR# 87951
01w,21mar03,ism  changed application logger API calls
01v,02dec02,spm  merged from WINDNET_IPV6-1_0-FCS label (ver 01t,25jul02,ant)
01u,07nov02,ant  New local pingLen initialized to global _pingTxLen2 and used throughout
		 the code instead of _pingTxLen2.
01t,07oct02,wie  print stack number for virtual stack debug support
01s,07may02,ant  New global int	_pingSockBufSize2 defined (setsockopt(SO_SNDBUF) and
		 setsockopt(SO_RCVBUF)).
01r,03may02,ant  u_char	bufTx [PING_MAXPACKET] in the struct PING_STAT changed
		 to u_char *bufTx. It is allocated dynamically now. Receive
		 buffer u_char bufRx[PING_MAXPACKET] changed in the same way.
		 PING_MAXPACKET	increased to 65536. New member int timing 
		 defined in the struct PING_STAT.  
01q,07dec01,ant  applUtilLib
01p,20sep01,ant  errno changed to errnoSet() and errnoGet()
01o,23feb01,rae  merged from 01n of tor2_0.barney (base: 01k):
		 Added PING_OPT_NOHOST flag to deal with SPR 22766
                 corrected ping error for pingRxPrint2() (SPR22571)
01n,25oct00,ham  doc: cleanup for vxWorks AE 1.0.
01m,17apr00,ham  fixed compilation warnings.
01l,14jan00,ham  removed taskSpawn/watchdog/semaphore for PD support.
01k,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01j,12mar99,p_m  Fixed SPR 8742 by documentating ping() configuration global
                 variables.
01i,05feb99,dgp  document errno values
01h,17mar98,jmb  merge jmb patch of 04apr97 from HPSIM: corrected
                 creation/deletion of task delete hook.
01g,30oct97,cth  changed stack size of tPingTxn from 3000 to 6000 (SPR 8222).
01f,26aug97,spm  removed compiler warnings (SPR #7866)
01e,30sep96,spm  corrected ping error for little-endian machines (SPR #4235)
01d,13mar95,dzb  changed to use free() instead of cfree() (SPR #4113)
01c,24jan95,jdi  doc tweaks
01b,10nov94,rhp  minor edits to man pages
01a,25oct94,dzb  written
*/

/*
DESCRIPTION
This library contains the ping() utility, which tests whether a remote 
host is reachable. 

The routine ping() is typically called from the VxWorks shell to check the
network connection to another VxWorks target or to a UNIX host.  ping()
may also be used programmatically by applications that require such a test.
The remote host must be running TCP/IP networking code that responds to
ICMP echo request packets.  The ping() routine is re-entrant, thus may
be called by many tasks concurrently.

The routine pingLibInit2() initializes the ping() utility and allocates
resources used by this library.  It is called automatically when
INCLUDE_PING is defined.

INCLUDE FILES
pingLib.h
*/

/* includes */

#if 0

#ifdef _WRS_KERNEL2
#include <vxWorks.h>
#include <taskHookLib.h>	
#include <vxLib.h>	
#include <netconf.h>
#include <netinet/in_systm.h>
#else
#include <strings.h>
#endif /* ! _WRS_KERNEL2 */

#else

#include <strings.h>

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ioLib.h>
#include <selectLib.h>
#include <errnoLib.h>
#include <semLib.h>
#include <taskLib.h>
#include <hostLib.h>
#include <sockLib.h>
#include <inetLib.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h> 
/*
#include <netinet/icmp_var.h>
#include <net/raw_cb.h>
*/
#include <pingLib.h>
#include <applUtilLib.h>
#include <tickLib.h>
#include <sysLib.h>

#ifdef VIRTUAL_STACK
#include <netinet/vsLib.h>		
#include <netinet/vsData.h>		
#include <netinet/vsIcmp.h>		
#endif

/* defines */

#define pingError(pPS)	\
    do { pPS->flags |= PING_OPT_SILENT; goto release; } while (FALSE)

/* externs */

#ifdef _WRS_KERNEL2
#ifndef VIRTUAL_STACK
IMPORT struct icmpstat _icmpstat;
#endif /* VIRTUAL_STACK */
#endif /* _WRS_KERNEL2 */

/* globals */

/*
 * Nominal space allocated to a raw socket.
 */
#define	RAWSNDQ		65536  /* 8192 -> 65536*/

int	_pingTxLen2 = 4096;  /* 64;			/* size of icmp echo packet */
int	_pingTxInterval2 = PING_INTERVAL;	/* packet interval in seconds */
int	_pingTxTmo2 = PING_TMO;			/* packet timeout in seconds */
int	_pingSockBufSize2 = RAWSNDQ;		/* size of socket buffer */

/* locals */

LOCAL PING_STAT	*	pingHead2 = NULL;	/* ping list head */
LOCAL SEM_ID		pingSem2 = NULL;		/* mutex for list access */

/* Number of kernel ping tasks without PING_OPT_SILENT,
 * for deciding when to remove the task hook.
 */
#ifdef _WRS_KERNEL2
LOCAL int		pingHookCount = 0;
#endif

/* static forward declarations */

LOCAL STATUS pingRxPrint2 (PING_STAT *pPS, int len, struct sockaddr_in *from,
			  ulong_t now, BOOL finish);

LOCAL void pingFinish2 (int taskId);

STATUS pingLibInit2 (void);


#ifndef _WRS_KERNEL2
u_short checksum (u_short * pAddr, int len);
#endif

/*******************************************************************************
*
* pingLibInit2 - initialize the ping() utility
*
* This routine allocates resources used by the ping() utility.
* It is called automatically when INCLUDE_PING is defined.
*
* RETURNS: OK
*
* ERRNO: N/A
*/

STATUS pingLibInit2 (void)
    {
    if (pingSem2 == NULL)			/* already initialized ? */
	{
	if ((pingSem2 = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE |
	    SEM_INVERSION_SAFE)) == NULL)
	    return (ERROR);
        }

    return (OK);
    }

/*******************************************************************************
*
* ping - test that a remote host is reachable
*
* This routine tests that a remote host is reachable by sending ICMP
* echo request packets, and waiting for replies.  It may called from
* the VxWorks shell as follows:
* .CS
*    -> ping "remoteSystem", 1, 0
* .CE
* where <remoteSystem> is either a host name that has been previously added
* to the remote host table by a call to hostAdd(), or an Internet address in
* dot notation (for example, "90.0.0.2").
*
* The second parameter, <numPackets>, specifies the number of ICMP packets
* to receive from the remote host.  If <numPackets> is 1, this routine waits
* for a single echo reply packet, and then prints a short message
* indicating whether the remote host is reachable.  For all other values
* of <numPackets>, timing and sequence information is printed as echoed
* packets are received.  If <numPackets> is 0, this routine runs continuously.
* 
* If no replies are received within a 5-second timeout period, the
* routine exits.  An ERROR status is returned if no echo replies
* are received from the remote host.
*
* The following flags may be given through the <options> parameter:
* \is
* \i PING_OPT_SILENT
* Suppress output. This option is useful for applications that 
* use ping() programmatically to examine the return status.
* \i PING_OPT_DONTROUTE
* Do not route packets past the local network. This also prevents pinging
* local addresses (i.e. the IP address of the host itself).  The 127.x.x.x 
* addresses will still work however.
* \i PING_OPT_NOHOST
* Suppress host lookup. This is useful when you have the DNS resolver
* but the DNS server is down and not returning host names. When ping()
* receives a response it tries to convert the IP address of the peer to
* its hostname so that it can be displayed. If the entry is not available
* in the local host table, then the DNS server will be contacted. If the
* server does not respond the first time, the resolver makes multiple
* attempts to contact the server and then times out if no response is 
* received. By default it makes a maximum of 2 attempts with a starting
* timeout of 5 seconds which results in a total timeout period of 15 seconds.
* If the number of attempts is increased to 4, this timeout can extend
* upto 75 seconds (because of exponential backoff between retries). During
* this time it may appear that the ping call has stalled and it will not
* return. In that case wait a few minutes to see if the call returns. 
* Sometimes the DNS server information specified to the resolver is
* incorrect, especially if the default settings were not changed at all
* when the DNS resolver component was included. Check the settings and 
* confirm whether the server is reachable or not.
* \i PING_OPT_DEBUG
* Enables debug output.
* \ie
*
* The following global variables can be set from the target shell or Windsh
* to configure the ping() parameters:
* \is
* \i '_pingTxLen2' 
* Size of the ICMP echo packet (default 64, min 8, max 65516).
* \i '_pingSockBufSize2'
* Size of the socket buffer (default 8192).
* \i '_pingTxInterval2'
* Packet interval in seconds (default 1 second).
* \i '_pingTxTmo2'
* Packet timeout in seconds (default 5 seconds).
* \ie
*
* NOTE
* If you have included the DNS resolver, not specified the PING_OPT_NOHOST
* option and you notice that ping() is taking a long time (more than the
* timeout, _pingTxTmo2) to return, check your resolver settings to see if
* the server has been specified correctly. It is also possible that
* the server is specified correctly but it is down. See the description 
* for the PING_OPT_NOHOST option if you see this problem.
*
* RETURNS:
* OK, or ERROR if the remote host is not reachable.
*
* ERRNO:
* \is
* \i EINVAL
* <numPackets> is invalid
* \i S_pingLib_TIMEOUT
* Timed out without a response
* \ie
*/

/*
example:
--------
ping2 "192.168.46.154"  // size: 64
ping2 "192.168.46.154", 1000, 65000   // count:1000, size: 65000
*/
STATUS ping2
    (
    char *		host,		/* host to ping */
    int			numPackets,	/* number of packets to receive */
    int		ping_size		/* option flags */
    )
{
    ulong_t		options = 0;		/* option flags */

    PING_STAT *		pPS;			/* current ping stat struct */
    struct sockaddr_in	to;			/* addr of Tx packet */
    struct sockaddr_in	from;			/* addr of Rx packet */
    int			fromlen = sizeof (from);/* size of Rx addr */
    int			ix = 1;			/* bytes read */
    int			txInterval;		/* packet interval in ticks */
    STATUS		status = ERROR;		/* return status */
    struct fd_set	readFd;
    struct timeval	pingTmo;
    ulong_t		now = 0;
	int t1, t2;  /* jc*/
    int			sel;
    int			pingLen;
    ulong_t		then;                   /* previous time */
    ulong_t             tickTmo;                /* timeout in ticks */

	struct timespec ts1;
    struct timespec ts2;

	if (ping_size < 64)
	{
		_pingTxLen2 = 64;
	}
	else
	{
		_pingTxLen2 = ping_size;
	}

    if (numPackets < 0)				/* numPackets positive ? */
	{
		errnoSet(EINVAL);			
		return (ERROR);
	}

	if(strlen(host) > MAXHOSTNAMELEN)
	{
		errnoSet(EINVAL);			
		return (ERROR);
	}

    /* check the size of the ICMP echo packet */

    pingLen = _pingTxLen2;

    if (pingLen > PING_MAXPACKET - 20)
	{
		if (!(options & PING_OPT_SILENT))
	            printf ("datalen value too large, reducing to %d\n",
			    PING_MAXPACKET-20);

		_pingTxLen2 = pingLen = PING_MAXPACKET - 20;
	}
    else if (pingLen < 8)
	{
		if (!(options & PING_OPT_SILENT))
	            printf ("datalen value too small, setting to 8\n");

		_pingTxLen2 = pingLen = 8;
	}

    /*
     * Allocate size for ping statistics/info structure, as
     * well as TX and RX data buffers, each of size pingLen + 20,
     * plus 4 bytes for alignment of bufRx.
     */
    pPS = calloc (1, (size_t)(sizeof(PING_STAT) + 2 * (pingLen + 20) + 4));

    if (pPS == NULL)
		return (ERROR);

    /* The data buffers are each at least 4-byte aligned */

    pPS->bufTx = (u_char *) (pPS + 1);
    pPS->bufRx = pPS->bufTx + ROUND_UP (pingLen + 20, 4);

    /*
     * SPR #106773 / #93732: At least pPS->idRx must be initialized
     * before enqueueing the PING_STAT on the list, so that other
     * ping tasks executing pingExit() won't confuse this PING_STAT
     * with their own. pPS->flags must also be initialized so we can
     * properly manage the pingHookCount.
     */
    pPS->idRx = taskIdSelf ();			/* get own task Id  */
    pPS->flags = (int)options;			/* save flags field */

    tickTmo = _pingTxTmo2 * sysClkRateGet();
    
    /* stuffing ping parameter */

    semTake (pingSem2, WAIT_FOREVER);		/* get access to list */
    pPS->statNext = pingHead2;			/* push session onto list */

#ifdef _WRS_KERNEL2
    /*
     * We add the delete hook in the kernel unconditionally, not just
     * when PING_OPT_SILENT isn't specified.
     */
    if ((pingHookCount == 0) && (taskDeleteHookAdd ((FUNCPTR) pingFinish2) == ERROR))
	{
		free ((char *) pPS);
		semGive (pingSem2);			/* give up access to list */
		
		if (options & PING_OPT_DEBUG)
		    printf ("ping: unable to add task Delete hook\n");
		
		return (ERROR);
	}

    ++pingHookCount;

#endif /* _WRS_KERNEL2 */

    pingHead2 = pPS;
    semGive (pingSem2);				/* give up access to list */

    /* stuffing ping parameter */

    pPS->tMin = 999999999;			/* init min rt time */
    pPS->numPacket = numPackets;		/* save num to send */
    pPS->clkTick = sysClkRateGet ();		/* save sys clk rate */

    txInterval = _pingTxInterval2 * pPS->clkTick;/* init interval value */

    pingTmo.tv_sec = _pingTxTmo2;
    pingTmo.tv_usec = 0;

    /* initialize the socket address struct */

    to.sin_family = AF_INET;

    /* Fix for SPR #106302 - use inet_aton() rather than inet_addr() */

    if (inet_aton (host, &to.sin_addr) != OK)
	{
	if ((to.sin_addr.s_addr = hostGetByName (host)) ==
	    (in_addr_t)ERROR)
	    {
            if (!(options & PING_OPT_SILENT))
	        printf ("ping: unknown host %s\n", host);

	    pingError (pPS);
	    }

	inet_ntoa_b (to.sin_addr, pPS->toInetName);
        }
    
    strcpy (pPS->toHostName, host);		/* save host name */
    pPS->dataLen = pingLen - 8;		/* compute size of data */

    /*
     * Race condition: Leak if we are deleted after socket() allocates
     * the socket but before we store the resulting file descriptor.
     * The taskSafe() guards against this.
     */

    taskSafe();
    pPS->pingFd = socket (AF_INET, SOCK_RAW, IPPROTO_ICMP);
    taskUnsafe();
    
    /* open raw socket for ICMP communication */

    if (pPS->pingFd < 0)
		pingError (pPS);

    pPS->pBufIcmp = (struct icmp *) pPS->bufTx;	/* ptr. to icmp header out */
    if (pPS->dataLen >= 4)	/* can we time transfer */
	{
	pPS->timing = 1;
        pPS->pBufTime = (ulong_t *) (pPS->bufTx + 8);/* pointer to time out */
	}

    if (options & PING_OPT_DONTROUTE)		/* disallow packet routing ? */
        if (setsockopt (pPS->pingFd, SOL_SOCKET, SO_DONTROUTE, (char *) &ix,
	    sizeof (ix)) == ERROR)
	    pingError (pPS);

    /* check _pingSockBufSize2, (it should be bigger than _pingTxLen2 + 35 ?) */

    if ((pPS->dataLen + 44) > _pingSockBufSize2)
    {
		if (!(options & PING_OPT_SILENT))
		    printf ("you need to increase socket buffer size\n");
		
		pingError (pPS);
	}

    /* set the send and receive buffer of the socket if the value in the global
       _pingSockBufSize2 is different than default one */

    if (_pingSockBufSize2 != RAWSNDQ) 
	{
		if (setsockopt(pPS->pingFd, SOL_SOCKET, SO_SNDBUF,
		       (char *)&_pingSockBufSize2,
		       sizeof(_pingSockBufSize2)) < 0) 
	    {
		    log_err (LOG_ERRNO, "setsockopt(SO_SNDBUF)");
		    pingError (pPS);	
	    }

		if (setsockopt(pPS->pingFd, SOL_SOCKET, SO_RCVBUF,
		       (char *)&_pingSockBufSize2,	
		       sizeof(_pingSockBufSize2)) < 0) 
	    {
		    log_err (LOG_ERRNO, "setsockopt(SO_RCVBUF)");
		    pingError (pPS);	
	    }
  	}


    if (!(options & PING_OPT_SILENT) && pPS->numPacket != 1)
	{
        printf ("PING %s", pPS->toHostName);	/* print out dest info */
        if (pPS->toInetName[0])
            printf (" (%s)", pPS->toInetName);

        printf (" (%d=20+8+%d bytes)\n", pPS->dataLen + 28, pPS->dataLen); 
	}
    
    pPS->pBufIcmp->icmp_type = ICMP_ECHO;	/* set up Tx buffer */
    pPS->pBufIcmp->icmp_code = 0;
    pPS->pBufIcmp->icmp_id = (u_int16_t)(pPS->idRx & 0xffff);


    for (ix = 4; ix < pPS->dataLen; ix++)	/* skip 4 bytes for time */
        pPS->bufTx [8 + ix] = (char)ix;


     /* receive echo reply packets from remote host */
    while (!pPS->numPacket || (pPS->numRx != pPS->numPacket))
	{
        if (pPS->timing)
	    *pPS->pBufTime = tickGet ();	/* load current tick count */
		
        pPS->pBufIcmp->icmp_seq = (u_int16_t)pPS->numTx++;	/* increment seq number */
        pPS->pBufIcmp->icmp_cksum = 0;
		pPS->pBufIcmp->icmp_cksum = checksum ((u_short *) pPS->pBufIcmp,
					      pingLen);
	
        /* transmit ICMP packet */						  
	/*t1 = tickGet();  // jc*/
	/*printf("ping start: %d => ", t1);	*/
    /*(void)clock_gettime(CLOCK_MONOTONIC, &ts1);*/
		
	if ((ix = (int)sendto (pPS->pingFd, (char *) pPS->pBufIcmp,
			  pingLen, 0, (struct sockaddr *)&to,
			  sizeof (struct sockaddr))) != pingLen)
	    {
	    if (pPS->flags & PING_OPT_DEBUG)
	        printf ("ping: wrote %s %d chars, ret=%d, errno=0x%x\n",
			pPS->toHostName, pingLen, ix, errnoGet());
	    }

        /*
         * Update ICMP statistics for ECHO messages - this is not the best
	 * place to put it since this shifts responsibility of updating ECHO
	 * statistics to anyone writing an application that sends out ECHO
	 * messages. However, this seems to be the only place to put it.
         */ 

        /* NOTE: because there is no API available in user space
         *       to update ICMP stats, this will cause the stats
         *       to be a little off. Following #ifdef needs to be 
         *       replaced with some API call that can be used from
         *       user space also.
         */
        #ifdef _WRS_KERNEL2
        _icmpstat.icps_outhist [ICMP_ECHO]++;
        #endif
        
        then = tickGet();     /* maybe should adjust select timeout instead */
        now = 0;

check_fd_again:					/* Wait for ICMP reply */

	FD_ZERO(&readFd);
	FD_SET(pPS->pingFd, &readFd);

	sel = select (pPS->pingFd+1, &readFd, NULL, NULL, &pingTmo);
    if (sel == ERROR)
	{
	    log_err (LOG_ERRNO, "in select()");
	    break; /* goto release */
	}
	else if ((sel == 0) || ((now != 0) && ((now - then) > tickTmo)))
    {
	    if (!(options & PING_OPT_SILENT))
                printf ("ping: timeout\n");
		
        errnoSet (S_pingLib_TIMEOUT);    /* timeout error */
		
	    break; /* goto release */
	}

	if (!FD_ISSET(pPS->pingFd, &readFd))
            goto check_fd_again;

	/* the fd is ready - FD_ISSET isn't needed */	
	if ((ix = (int)recvfrom (pPS->pingFd, (char *) pPS->bufRx, pingLen + 20,
	    0, (struct sockaddr *) &from, &fromlen)) == ERROR)
    {
	    if (errnoGet() == EINTR)	
	        goto check_fd_again;
	    break; /* goto release */
	}
		
#if 0	
	now = tickGet();		
	t2 = tickGet();  /* jc	*/
	now = t2 -t1;
	printf("ping time1:%d - %d \n", t2, t1);
	
    (void)clock_gettime(CLOCK_MONOTONIC, &ts2);	
	printf("ping time2:%d ns \n", (ts2.tv_nsec -ts1.tv_nsec));
#else
	
	now = tickGet();	/* origin		*/
#endif
	
	if (pingRxPrint2 (pPS, ix, &from, now, FALSE) == ERROR)
	    goto check_fd_again;
	
		taskDelay (txInterval);
	
    }/* while () */

    if (pPS->numRx > 0)
		status = OK;				/* host is reachable */

release:

    pingFinish2 (pPS->idRx);

	t2 = tickGet();  /* jc	*/
	now = t2 -t1;
	
    (void)clock_gettime(CLOCK_MONOTONIC, &ts2);
	
	printf("ping time:%d ns \n", (ts2.tv_nsec -ts1.tv_nsec));
	
    pingRxPrint2 (pPS, ix, &from, now, TRUE);

    return (status);
    }

/*******************************************************************************
*
* pingRxPrint2 - print out information about a received packet
*
* This routine prints out information about a received ICMP echo reply
* packet.  First, the packet is checked for minimum length and
* correct message type and destination.
*
* RETURNS: N/A.
*
* \NOMANUAL
*/

LOCAL STATUS pingRxPrint2
    (
    PING_STAT *		pPS,		/* ping stats structure */
    int			len,		/* Rx message length */
    struct sockaddr_in *from,		/* Rx message address */
    ulong_t		now,
    BOOL                finish
    )
    {
    struct ip *		ip = (struct ip *) pPS->bufRx;
    long *		lp = (long *) pPS->bufRx;
    struct icmp *	icp;
    int			ix;
    int			hlen;
    int			triptime;
    LOCAL char		fromHostName [MAXHOSTNAMELEN + 1];
    char		fromInetName [INET_ADDR_LEN];
    LOCAL BOOL          hostGetByAddrDone = FALSE; 
    
    /* convert address notation */

    if (finish)
        {
        hostGetByAddrDone = FALSE;
        return (OK);
        }
    inet_ntoa_b (from->sin_addr, fromInetName);

    /* Do we lookup hosts or not? */
    if (pPS->flags & PING_OPT_NOHOST)
        *fromHostName = EOS;

       
    else if (!hostGetByAddrDone)
        {
        if ((hostGetByAddr (from->sin_addr.s_addr, fromHostName)) == ERROR)
            {
            printf ("Cannot do a reverse query");
            *fromHostName = EOS;
            }
            
        hostGetByAddrDone = TRUE;
        }
    
    hlen = ip->ip_hl << 2;

    if (len < hlen + ICMP_MINLEN)		/* at least min length ? */
	{
	if (pPS->flags & PING_OPT_DEBUG)
	    printf ("packet too short (%d bytes) from %s\n", len,
		fromInetName);
	return (ERROR);
        }

    len -= hlen;				/* strip IP header */
    icp = (struct icmp *) (pPS->bufRx + hlen);

    if (icp->icmp_type != ICMP_ECHOREPLY)	/* right message ? */
	{
	if (pPS->flags & PING_OPT_DEBUG)	/* debug odd message */
	    {
#ifdef VIRTUAL_STACK

		printf("[vs=%d] ", myStackNum);
		
#endif
	    if (*fromHostName != 0x00)
		{
                printf ("%d bytes from %s (%s): ", len, fromHostName,
		    fromInetName);
		}
	    else 
		{
	        printf ("%d bytes from %s: ", len, fromInetName);
		}
	    icp->icmp_type = (u_char)min (icp->icmp_type, ICMP_MAXTYPE); 
	    printf ("icmp_type=%d\n", icp->icmp_type);
	    for(ix = 0; ix < 12; ix++)
	        printf ("x%2.2lx: x%8.8lx\n", (ULONG)(ix * sizeof (long)),
                        (ULONG)*lp++);

	    printf ("icmp_code=%d\n", icp->icmp_code);
	    }

	return (ERROR);
        }

    /* check if the received reply is ours. */
    if (icp->icmp_id != (pPS->idRx & 0xffff))
	{
	return (ERROR);					/* wasn't our ECHO */
	}

    /* print out Rx packet stats */
    if (!(pPS->flags & PING_OPT_SILENT) && pPS->numPacket != 1)
	{
	if (*fromHostName != 0x00)
	    {
#ifdef VIRTUAL_STACK

		printf("[vs=%d] ", myStackNum);
		
#endif
	    printf ("%d bytes from %s (%s): ", len, fromHostName, fromInetName);
	    }
	else 
	    {
#ifdef VIRTUAL_STACK

		printf("[vs=%d] ", myStackNum);
		
#endif
            printf ("%d bytes from %s: ", len, fromInetName);
	    }
        printf ("icmp_seq=%d. ", icp->icmp_seq);
        if (pPS->timing)
	    {
	    #if 0
            triptime = (int)((now - *((ulong_t *) icp->icmp_data)) *   (1000 / pPS->clkTick));
            printf ("time=%d. ms\n", triptime);
		#else
            triptime = (int)((now - *((ulong_t *) pPS->pBufTime)) *   (1000 / pPS->clkTick));
			if (triptime <= 0)
			{
				printf ("time<1 ms\n", triptime);
			}
			else
			{
	            printf ("time=%d. ms\n", triptime);
			}
		#endif
			
            pPS->tSum += triptime;
            pPS->tMin = min (pPS->tMin, triptime);
            pPS->tMax = max (pPS->tMax, triptime);
	    }
	else
	    printf ("\n");
	}

    pPS->numRx++;
    return (OK);
    }

/*******************************************************************************
*
* pingFinish2 - return all allocated resources and print out final statistics
*
* This routine returns all resources allocated for the ping session, and
* prints out a stats summary.
*
* The ping session is located in the session list (pingHead2) by searching
* the ping stats structure for the receiver task ID.  This is necessary
* because this routine is passed a pointer to the task control block, and does
* not have ready access to the ping stats structure itself.  This accomodates
* the use of task delete hooks as a means of calling this routine.
*
* RETURNS: N/A.
*
* \NOMANUAL
*/

LOCAL void pingFinish2
    (
    int taskId
    )
    {
    PING_STAT *		pPS;			/* ping stats structure */
    PING_STAT **	ppPrev;			/* pointer to prev statNext */

    semTake (pingSem2, WAIT_FOREVER);		/* get list access */

    pPS = pingHead2;
    ppPrev = &pingHead2;

    /* find session in list */
    for (pPS = pingHead2; pPS != NULL; pPS = pPS->statNext)
	{
	if (pPS->idRx == taskId)
	    goto found;
        ppPrev = &pPS->statNext;
	}

    semGive (pingSem2);			/* give up list access */
    return;

found:
    *ppPrev = pPS->statNext;			/* pop session off list */

#ifdef _WRS_KERNEL2

    if ((pPS->flags & PING_OPT_SILENT) == 0)
	{
	if (--pingHookCount == 0)
	    (void) taskDeleteHookDelete ((FUNCPTR) pingFinish2);
	}
#endif /* _WRS_KERNEL2 */

    semGive (pingSem2);				/* give up list access */

    /* return all allocated/created resources */
    if (pPS->pingFd)
        (void) close (pPS->pingFd);

    if (!(pPS->flags & PING_OPT_SILENT))	/* print final report ? */
	{
        if (pPS->numRx)				/* received at least one ? */
	    {
	    if (pPS->numPacket != 1)		/* full report */
		{
#ifdef VIRTUAL_STACK

		printf("[vs=%d] ", myStackNum);
		
#endif
                printf ("----%s PING Statistics----\n", pPS->toHostName);
                printf ("%d packets transmitted, ", pPS->numTx);
                printf ("%d packets received, ", pPS->numRx);

                if (pPS->numTx)
                    printf ("%d%% packet loss", ((pPS->numTx - pPS->numRx) *
			100) / pPS->numTx);
                printf ("\n");

                if (pPS->numRx && pPS->timing)
                    printf ("round-trip (ms)  min/avg/max = %d/%d/%d\n",
			pPS->tMin, pPS->tSum / pPS->numRx, pPS->tMax);	
                }
            else				/* short report */
	        printf ("%s is alive\n", pPS->toHostName);
            }
        else
	    printf ("no answer from %s\n", pPS->toHostName);
	}

    free ((char *)pPS);		/* free stats memory space & data buffers */
    }

/* checksum function is not available in user space yet 
 * It should be removed once the support is available
 */
#ifndef _WRS_KERNEL2
/*******************************************************************************
*
* checksum - portable internet checksum calculation routine
*
* This checksums the buffer <pAddr> of length <len>.  This routine differs
* from the standard checksum algorithm (in_cksum) in that it checksums a buffer
* while the other checksums an mbuf chain.  For of a description of the
* internet checksum algorithm, please refer to RFC 1071.
*
* RETURNS: checksum
*
* \NOMANUAL
*/

u_short checksum
    (
    u_short *           pAddr,                  /* pointer to buffer  */
    int                 len                     /* length of buffer   */
    )
    {
    int         nLeft   = len;
    int         sum     = 0;
    u_short *   w       = pAddr;
    u_short     answer;

    while (nLeft > 1)
        {
        sum     += *w++;
        nLeft   -= 2;
        }

    if (nLeft == 1)
#if _BYTE_ORDER == _BIG_ENDIAN
        sum += 0 | ((*(u_char *) w) << 8);
#else
        sum += *(u_char *) w;
#endif

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = (u_short)sum;

    return (u_short)(~answer & 0xffff);
    }
#endif
