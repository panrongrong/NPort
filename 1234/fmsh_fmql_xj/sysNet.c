/* sysNet.c - system-dependent Network Library */

/*
 * Copyright (c) 2012 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01b,25sep12,fao  check sysNvRamGet/Set return value
01a,25apr12,fao  created.
*/

/*
DESCRIPTION

This module provides BSP functionality to support the bootrom 'M' command to
modify MAC addresses of on-board network interfaces.
*/

/* includes */

#include <vxWorks.h>
#include <stdio.h>
#include "config.h"

/* defines */

#define MAC_OFFSET_GEMAC        0

#define MAC_OFFSET_AXINET        2


#define SYS_NET_DEV_NAME        "gem"
/* #define SYS_NET_DEV_NAME        "emac" */
#define SYS_AXI_NET_DEV_NAME        "axienet"

#define DEBUG_ON
#ifdef  DEBUG_ON
#   define DEBUG_LOG printf
#else
#   define DEBUG_LOG(format, ... )
#endif  /* DEBUG_ON */

#define MAC_ADRS_LEN         6
#define MAX_MAC_ADRS         7
#define MAX_MAC_DEVS         7

/* 
MAC address configuration 
*/
#define FMSH_ENET0           0x00
#define FMSH_ENET1           0x0a
#define FMSH_ENET2           0x35

#define CUST_ENET3           0x11
#define CUST_ENET4           0x22
#define CUST_ENET5           0x33

#define ENET_DEFAULT0 		FMSH_ENET0
#define ENET_DEFAULT1 		FMSH_ENET1
#define ENET_DEFAULT2 		FMSH_ENET2

/* globals */
const char *sysNetDevName [MAX_MAC_DEVS] = { SYS_NET_DEV_NAME };

/* locals */
LOCAL UINT8 glbEnetAddr [MAX_MAC_ADRS][MAC_ADRS_LEN + 1] = 
{
    { FMSH_ENET0, FMSH_ENET1, FMSH_ENET2, CUST_ENET3, CUST_ENET4, CUST_ENET5 },
    
#if 1  /* jc */
    { FMSH_ENET0, FMSH_ENET1, FMSH_ENET2, CUST_ENET3, CUST_ENET4, CUST_ENET5+1 },
    { FMSH_ENET0, FMSH_ENET1, FMSH_ENET2, CUST_ENET3, CUST_ENET4, CUST_ENET5+2 },
    { FMSH_ENET0, FMSH_ENET1, FMSH_ENET2, CUST_ENET3, CUST_ENET4, CUST_ENET5+3 },
    { FMSH_ENET0, FMSH_ENET1, FMSH_ENET2, CUST_ENET3, CUST_ENET4, CUST_ENET5+4 },
    { FMSH_ENET0, FMSH_ENET1, FMSH_ENET2, CUST_ENET3, CUST_ENET4, CUST_ENET5+5 },
#endif
};

LOCAL UINT8 sysInvalidAddr [2][MAC_ADRS_LEN + 1] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }
};

STATUS sysNetMacNVRamAddrGet(char *ifName,
		int ifUnit,
		UINT8 * ifMacAddr,
		int ifMacAddrLen);

/*
IMPORT STATUS qspiFlashRead(UINT32 offset,
char *      readBuffer,
UINT32         byteLen );
*/
/*******************************************************************************
*
* sysMacIndex2Dev - convert index range to device string
*
* This routine converts an index range 0..MAX_MAC_ADRS-1
* to a device string index.
*
* RETURNS: index access device name in sysNetDevName
*
* ERRNO: N/A
*/

int sysMacIndex2Dev
    (
    int index
    )
    {
    return 0;
    }

/*******************************************************************************
*
* sysMacIndex2Unit - convert index range to unit number
*
* This routine converts an index range 0..MAX_MAC_ADRS-1
* to a unit number.
*
* RETURNS: unit number of indexed device
*
* ERRNO: N/A
*/

int sysMacIndex2Unit
    (
    int index
    )
    {
    return (index - MAC_OFFSET_GEMAC);
    }

/*******************************************************************************
*
* sysMacOffsetGet - Calculate table offset
*
* This routine calculates which table entry corresponds to the specified
* interface. Two values are calculated and returned in the parameters pointed
* to by ppEnet and pOffset.
*
* RETURNS: ERROR if the interface is not known; OK otherwise
*
* ERRNO: N/A
*/

STATUS sysMacOffsetGet
    (
    char *  ifName,     /* interface name */
    int     ifUnit,     /* interface unit */
    char ** ppEnet,     /* pointer to glbEnetAddr[][] entry */
    int *   pOffset     /* offset in NVRAM */
    )
    {
	
    if (ifUnit > MAX_MAC_DEVS)
        return ERROR;

    if (strcmp (ifName, SYS_NET_DEV_NAME) == 0)
        {
        *pOffset = (ifUnit + MAC_OFFSET_GEMAC) * (MAC_ADRS_LEN + 2);
        *ppEnet  = (char*)glbEnetAddr [ifUnit + MAC_OFFSET_GEMAC];
        }
	else if (strcmp (ifName, SYS_AXI_NET_DEV_NAME) == 0)
        {
        *pOffset = (ifUnit + MAC_OFFSET_GEMAC) * (MAC_ADRS_LEN + 2);
        *ppEnet  = (char*)glbEnetAddr [ifUnit + MAC_OFFSET_GEMAC];
        }
    else
        {
        return ERROR;
        }

    return OK;
    }

/*******************************************************************************
*
* sysNetMacAddrGet - Get interface MAC address
*
* This routine gets the current MAC address from the network interface, and
* stores it in the ifMacAddr buffer provided by the caller.
*
* If the network interface cannot be queried about the MAC address,
* this routine returns ERROR.
*
* RETURNS: OK, if MAC address available, ERROR otherwise
*
* ERRNO: N/A
*/

STATUS sysNetMacAddrGet
    (
    char *  ifName,
    int     ifUnit,
    UINT8 * ifMacAddr,
    int     ifMacAddrLen
    )
    {
    /*
     * None of our interfaces can be queried directly.
     * Return ERROR to indicate that we need to use RAM/NVRAM instead.
     */
#if 0  
    return sysNetMacNVRamAddrGet(ifName,ifUnit,ifMacAddr,ifMacAddrLen);
#else  
	return ERROR;
#endif
    }

/*******************************************************************************
*
* sysNetMacAddrSet - Save interface MAC address
*
* This routine saves the MAC address specified in ifMacAddr to the appropriate
* location in NVRam (if possible) and update the specified interface to use
* the specified MAC address.
*
* If the network interface MAC address cannot be set,
* this routine returns ERROR.
*
* RETURNS: OK, if MAC address available, ERROR otherwise
*
* ERRNO: N/A
*/

STATUS sysNetMacAddrSet
    (
    char *  ifName,
    int     ifUnit,
    UINT8 * ifMacAddr,
    int     ifMacAddrLen
    )
    {

    int   offset;
    char *pEnet;
    UINT8  nvAddr[8];

#if 0 

    /* fetch address line & offset from glbEnetAddr[] table */
    if (sysMacOffsetGet (ifName, ifUnit, &pEnet, &offset) != OK)
        return ERROR;

#if (NV_RAM_SIZE != NONE)

    if (sysNvRamGet ((char *)nvAddr, ifMacAddrLen,
                     NV_MAC_ADRS_OFFSET + offset) != OK)
        return ERROR;

    if (nvAddr[0] != ENET_DEFAULT0 ||
        nvAddr[1] != ENET_DEFAULT1 ||
        nvAddr[2] != ENET_DEFAULT2)
        {
        DEBUG_LOG ("%s%d: warning: using hard-coded "
                "address %02x:%02x:%02x:%02x:%02x:%02x\n",
                ifName, ifUnit, pEnet[0], pEnet[1], pEnet[2],
                pEnet[3], pEnet[4], pEnet[5]);
        }
    else
        memcpy (pEnet, nvAddr, MAC_ADRS_LEN);

    if (memcmp (ifMacAddr, pEnet, ifMacAddrLen) == 0)
        {
        /* same address so don't erase and rewrite flash */

        DEBUG_LOG ("Address unchanged\n");
        return OK;
        }

    /*if (sysNvRamSet ((char *)ifMacAddr, ifMacAddrLen,
                     NV_MAC_ADRS_OFFSET + offset) != OK)
        return ERROR;*/
#endif

#endif

    /* mac address in memory only */
#if 0
    memcpy (ifMacAddr, pEnet, ifMacAddrLen); 
#else
    UINT8 mac[6] = {0};
	mac[0] = 0x00; mac[1] = 0x0A; mac[2] = 0x35;
	mac[3] = 0x01; mac[4] = 0x02; mac[5] = 0x03;
    memcpy (ifMacAddr, (UINT8 *)(&mac[0]), 6);
#endif

    return OK;
    }

/*******************************************************************************
*
* sysNetMacNVRamAddrGet - Get interface MAC address
*
* This routine gets the current MAC address from the Non Volatile RAM, and store
* it in the ifMacAddr buffer provided by the caller.
*
* It is not required for the BSP to provide NVRAM to store the MAC address.
* Also, some interfaces do not allow the MAC address to be set by software.
* In either of these cases, this routine simply returns ERROR.
*
* Given a MAC address m0:m1:m2:m3:m4:m5, the byte order of ifMacAddr is:
*       m0 @ ifMacAddr
*       m1 @ ifMacAddr + 1
*       m2 @ ifMacAddr + 2
*       m3 @ ifMacAddr + 3
*       m4 @ ifMacAddr + 4
*       m5 @ ifMacAddr + 5
*
* RETURNS: OK, if MAC address available, ERROR otherwise
*
* ERRNO: N/A
*/
#define NV_RAM_SIZE NONE
STATUS sysNetMacNVRamAddrGet
    (
    char *  ifName,
    int     ifUnit,
    UINT8 * ifMacAddr,
    int     ifMacAddrLen
    )
    {
    int   offset;
    char *pEnet;
#if (NV_RAM_SIZE != NONE)
    UINT8  nvAddr[8];
#endif

#if 0
    /* fetch address line & offset from glbEnetAddr[] table */

    if (sysMacOffsetGet (ifName, ifUnit, &pEnet, &offset) != OK)
        return ERROR;

#if (NV_RAM_SIZE != NONE)

#if 0
	DEBUG_LOG ("qspi MAC addr get start at : 0x%x\n",NV_MAC_ADRS_OFFSET+offset);
    /*qspiFlashRead(offset, (char *)nvAddr, ifMacAddrLen);*/
    if (qspiFlashRead(NV_MAC_ADRS_OFFSET+offset, (char *)nvAddr, ifMacAddrLen)!= OK){
    	DEBUG_LOG ("qspiFlashRead err, return\n");
        return ERROR;
    }
    DEBUG_LOG ("qspi mac address - %02x:%02x:%02x:%02x:%02x:%02x\n",
    		nvAddr[0], nvAddr[1], nvAddr[2], nvAddr[3], nvAddr[4], nvAddr[5]);
#else
    if (sysNvRamGet ((char *)nvAddr, ifMacAddrLen,
                     NV_MAC_ADRS_OFFSET + offset) != OK)
        return ERROR;

#endif

    if (nvAddr[0] != ENET_DEFAULT0 ||
        nvAddr[1] != ENET_DEFAULT1 ||
        nvAddr[2] != ENET_DEFAULT2)
        {
        DEBUG_LOG ("%s%d: warning: using hard-coded "
                "address %02x:%02x:%02x:%02x:%02x:%02x\n",
                ifName, ifUnit, pEnet[0], pEnet[1], pEnet[2],
                pEnet[3], pEnet[4], pEnet[5]);
        }
    else
        memcpy (pEnet, nvAddr, MAC_ADRS_LEN);
	
#endif

    if (memcmp (pEnet, sysInvalidAddr[0], MAC_ADRS_LEN) == 0)
        {
    	DEBUG_LOG ("Inv mac address\n");
        return ERROR;
        }
    if (memcmp (pEnet, sysInvalidAddr[1], MAC_ADRS_LEN) == 0)
        {
    	DEBUG_LOG ("Inv mac address\n");
        return ERROR;
        }

    /* mac address in memory only */

    memcpy (ifMacAddr, pEnet, ifMacAddrLen);
    DEBUG_LOG ("qspi MAC addr get ok\n");
	
#else

	/* fetch address line & offset from glbEnetAddr[] table */	
	if (sysMacOffsetGet (ifName, ifUnit, &pEnet, &offset) != OK)
	{	
		return ERROR;
	}

#if (NV_RAM_SIZE != NONE)
	if (sysNvRamGet ((char *)nvAddr, ifMacAddrLen, NV_MAC_ADRS_OFFSET + offset) != OK)
	{	
		return ERROR;
	}

	if (nvAddr[0] != ENET_DEFAULT0 ||
		nvAddr[1] != ENET_DEFAULT1 ||
		nvAddr[2] != ENET_DEFAULT2)
	{
		DEBUG_LOG ("%s%d: warning: using hard-coded "
				"address %02x:%02x:%02x:%02x:%02x:%02x\n",
				ifName, ifUnit, pEnet[0], pEnet[1], pEnet[2],
				pEnet[3], pEnet[4], pEnet[5]);
	}
	else
	{
		memcpy (pEnet, nvAddr, MAC_ADRS_LEN);
	}
#endif

	if (memcmp (pEnet, sysInvalidAddr[0], MAC_ADRS_LEN) == 0)
	{
		return ERROR;
	}
	
	if (memcmp (pEnet, sysInvalidAddr[1], MAC_ADRS_LEN) == 0)
	{
		return ERROR;
	}

	/* mac address in memory only */
	memcpy (ifMacAddr, pEnet, ifMacAddrLen);
#endif

    return OK;
}
	
