/* $Header: /usr/cvsroot/target/h/wrn/wm/demo/netconf.h,v 1.2 2002/02/27 21:42:08 josh Exp $ */

/*
 * Copyright (C) 1999-2005 Wind River Systems, Inc.
 * All rights reserved.  Provided under license only.
 * Distribution or other use of this software is only
 * permitted pursuant to the terms of a license agreement
 * from Wind River Systems (and is otherwise prohibited).
 * Refer to that license agreement for terms of use.
 */


/****************************************************************************
 *  Copyright 1993-1997 Epilogue Technology Corporation.
 *  Copyright 1998 Integrated Systems, Inc.
 *  All rights reserved.
 ****************************************************************************/

/*
 * $Log: netconf.h,v $
 * Revision 1.2  2002/02/27 21:42:08  josh
 * fix include path
 *
 * Revision 1.1  2001/11/08 16:00:57  tneale
 * Adding header files originally from snark/lib
 *
 * Revision 2.10  2001/01/19 22:23:46  paul
 * Update copyright.
 *
 * Revision 2.9  2000/03/17 00:12:40  meister
 * Update copyright message
 *
 * Revision 2.8  1998/02/25 04:57:27  sra
 * Update copyrights.
 *
 * Revision 2.7  1997/05/20 02:08:06  mrf
 * ipaddr_t, ldb macro and routing api conversion of media layer and arp code
 *
 * Revision 2.6  1997/03/20 06:52:58  sra
 * DFARS-safe copyright text.  Zap!
 *
 * Revision 2.5  1997/02/25 10:58:16  sra
 * Update copyright notice, dust under the bed.
 *
 * Revision 2.4  1996/05/08 18:18:09  lowell
 * "gratuitous ARP" shouldn't break non-ARP nets now
 *
 * Revision 2.3  1996/04/30  20:29:33  josh
 * Damage control -- forcing SNARK_GRATUITOUS_ARPt to 0
 *
 * Revision 2.2  1996/04/23  22:05:58  lowell
 * added ARP-for-my-own-address on startup
 * (to solve problems associated with making up ethernet addresses
 * out of thin air)
 *
 * Revision 2.1  1996/03/22  10:05:39  sra
 * Update copyrights prior to Attache 3.2 release.
 *
 * Revision 2.0  1995/05/10  22:38:15  sra
 * Attache release 3.0.
 *
 * Revision 1.4  1995/01/06  00:52:48  sra
 * Update copyright notice for 2.1 release.
 *
 * Revision 1.3  1994/09/04  23:55:50  sra
 * Get rid of most of the ancient NO_PP cruft.
 *
 * Revision 1.2  1994/01/09  23:57:29  sra
 * Add support for NIT under SunOS 4.1.2.
 *
 * Revision 1.1  1993/07/05  21:53:30  sra
 * Initial revision
 *
 */

/*
 * Generic network initialization routines for snark.
 */

/* [clearcase]
modification history
-------------------
01a,20apr05,job  update copyright notices
*/


#ifndef _NETCONF_H_
#define _NETCONF_H_

/*
 * Stuff provided by generic snark code.
 */
extern void netconfig (void);
extern boolean_t netconfig_if (struct net *);

/*
 * Each port of snark has to provide this routine.
 */
extern void netconfig_find (void);

/*
 * The BPF code needs to send out ARPs because it gets a new hardware
 * address every time it starts up.  If any other systems need to do
 * this too, they can be added here.
 */
#if INSTALL_SNARK_BSD_BPF
#define SNARK_GRATUITOUS_ARP 1
void snark_gratuitous_arp(net_if *, ipaddr_t *);
#include <wrn/wm/attache/arp.h>
#else 
#define SNARK_GRATUITOUS_ARP 0
#endif /* INSTALL_SNARK_BSD_BPF */

#endif /* _NETCONF_H_ */
