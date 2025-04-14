/* pciAutoConfigLib.h - PCI bus automatic resource allocation facility */

/* Copyright 1984-2001 Wind River Systems, Inc. */
/* Copyright 1997,1998 Motorola, Inc. All Rights Reserved */

/*
modification history
--------------------
01g,16oct01,tor  Added support macros for pciAutoCfgCtl3(), see
		 pciAutoConfigLib.c for details.
01f,07may99,tm   Added pciAutoAddrAlign3 to perform alignment checking
                 pciAutoFuncDis-/En-able, pciAutoRegConfig3 public (SPR 26484)
01e,30mar99,dat  SPR 25312, changed class to pciClass
01d,29jul98,scb  added roll call capability
01c,29jul98,tm   code review changes / added _DEV_DISPLAY attribute
01b,24jul98,tm   added int routing offset to PCI_LOC3 structure 
                 added autoIntRouting boolean to PCI_SYSTEM3 structure
		 added protos for pciAutoBusNumberSet3, pciAutoDevReset3
01a,19feb98,dat  written, from Motorola.
*/

#ifndef __INCpciAutoConfigLib3h
#define __INCpciAutoConfigLib3h

#include "dllLib3.h"


#define UINT8 unsigned char
#define UINT16 unsigned short
#define UINT32 unsigned int
#define UINT unsigned int
#define STATUS int
#define UCHAR unsigned char



#ifdef __cplusplus
extern "C" {
#endif

/* PCI_LOC3 attributes */

#define PCI_AUTO_ATTR_DEV_EXCLUDE	(UINT8)(0x01) /* Exclude this device */
#define PCI_AUTO_ATTR_DEV_DISPLAY	(UINT8)(0x02) /* (S)VGA disp device  */
#define PCI_AUTO_ATTR_DEV_PREFETCH	(UINT8)(0x04) /* Device requests PF  */

#define PCI_AUTO_ATTR_BUS_PREFETCH	(UINT8)(0x08) /* Bridge implements   */
#define PCI_AUTO_ATTR_BUS_PCI		(UINT8)(0x10) /* PCI-PCI Bridge      */
#define PCI_AUTO_ATTR_BUS_HOST		(UINT8)(0x20) /* PCI Host Bridge     */
#define PCI_AUTO_ATTR_BUS_ISA		(UINT8)(0x40) /* PCI-ISA Bridge      */
#define PCI_AUTO_ATTR_BUS_4GB_IO	(UINT8)(0x80) /* 4G/64K IO Adresses  */


/* OPTION COMMANDS for use with pciAutoCfgCtl3() */

/* 0 used for pSystem structure copy */
#define PCI_PSYSTEM_STRUCT_COPY		0x0000

/* 1-7 reserved for Fast Back To Back functions */
#define PCI_FBB_ENABLE			0x0001
#define PCI_FBB_DISABLE			0x0002
#define PCI_FBB_UPDATE			0x0003
#define PCI_FBB_STATUS_GET		0x0004

/* 8-11 reserved for MAX_LAT */
#define PCI_MAX_LATENCY_FUNC_SET	0x0008
#define PCI_MAX_LATENCY_ARG_SET		0x0009
#define PCI_MAX_LAT_ALL_SET		0x000a
#define PCI_MAX_LAT_ALL_GET		0x000b

/* 12-15 reserved for message output (logMsg) */
#define PCI_MSG_LOG_SET			0x000c

/* 16-47 reserved for pSystem functionality */
#define PCI_MAX_BUS_SET			0x0010
#define PCI_MAX_BUS_GET			0x0011
#define PCI_CACHE_SIZE_SET		0x0012
#define PCI_CACHE_SIZE_GET		0x0013
#define PCI_AUTO_INT_ROUTE_SET		0x0014
#define PCI_AUTO_INT_ROUTE_GET		0x0015
#define PCI_MEM32_LOC_SET		0x0016
#define PCI_MEM32_SIZE_SET		0x0017
#define PCI_MEMIO32_LOC_SET		0x0018
#define PCI_MEMIO32_SIZE_SET		0x0019
#define PCI_IO32_LOC_SET		0x001a
#define PCI_IO32_SIZE_SET		0x001b
#define PCI_IO16_LOC_SET		0x001c
#define PCI_IO16_SIZE_SET		0x001d
#define PCI_INCLUDE_FUNC_SET		0x001e
#define PCI_INT_ASSIGN_FUNC_SET		0x001f
#define PCI_BRIDGE_PRE_CONFIG_FUNC_SET	0x0020
#define PCI_BRIDGE_POST_CONFIG_FUNC_SET	0x0021
#define PCI_ROLLCALL_FUNC_SET		0x0022

/* 48-511 reserved for other memory configuration */
#define PCI_MEM32_SIZE_GET		0x0030
#define PCI_MEMIO32_SIZE_GET		0x0031
#define PCI_IO32_SIZE_GET		0x0032
#define PCI_IO16_SIZE_GET		0x0033

/* 512-0xffff available for misc items */
#define PCI_TEMP_SPACE_SET		0x0200
#define PCI_MINIMIZE_RESOURCES		0x0201

#ifndef _ASMLANGUAGE

typedef struct pciMemPtr3
    {
    void *	pMem;
    int		memSize;
    } PCI_MEM_PTR3;

typedef struct /* PCI_LOC3, a standard bus location */
    {
    UINT8 bus;
    UINT8 device;
    UINT8 function;
    UINT8 attribute;
    UINT8 offset;       /* interrupt routing for this device */
    } PCI_LOC3;

/* PCI identification structure */

typedef struct
    {
    PCI_LOC3 loc;
    UINT devVend;
    } PCI_ID3;


/* obsolete structure */
typedef struct pciSystem3 /* PCI_SYSTEM3, auto configuration info */
{
    UINT pciMem32;		/* 32 bit prefetchable memory location */
    UINT pciMem32Size;		/* 32 bit prefetchable memory size */
    UINT pciMemIo32;		/* 32 bit non-prefetchable memory location */
    UINT pciMemIo32Size;	/* 32 bit non-prefetchable memory size */
    UINT pciIo32;		/* 32 bit io location */
    UINT pciIo32Size;		/* 32 bit io size */
    UINT pciIo16;		/* 16 bit io location */
    UINT pciIo16Size;		/* 16 bit io size */
    int maxBus;			/* Highest subbus number */
    int cacheSize;		/* cache line size */
    UINT maxLatency;		/* max latency */
    BOOL autoIntRouting;        /* automatic routing strategy */
	
    STATUS (* includeRtn3)	/* returns OK to include */
	   (
	   struct pciSystem3 * pSystem,
	   PCI_LOC3 * pLoc,
	   UINT devVend
	   );
	
    UCHAR  (* intAssignRtn3)	/* returns int line, given int pin */
	   (
	   struct pciSystem3 * pSystem,
	   PCI_LOC3 * pLoc,
	   UCHAR pin
	   );
	
    void (* bridgePreConfigInit3) /* bridge pre-enumeration initialization */
	   (
	   struct pciSystem3 * pSystem,
	   PCI_LOC3 * pLoc,
	   UINT devVend
	   );
	
    void (* bridgePostConfigInit3)/* bridge post-enumeration initialization */
	   (
	   struct pciSystem3 * pSystem,
	   PCI_LOC3 * pLoc,
	   UINT devVend
	   );
	
    STATUS (* pciRollcallRtn3) ();  /* Roll call check */
	
} PCI_SYSTEM3;


typedef int (*PCI_LOGMSG_FUNC3)(char *fmt, int i1, int i2, int i3, int i4, int i5, int i6);
typedef UINT8 (*PCI_MAX_LAT_FUNC3)(int bus, int device, int func, void *pArg);
typedef UINT32 (*PCI_MEM_BUS_EXTRA_FUNC3)(int bus, int device, int func, void *pArg);
typedef STATUS (*PCI_INCLUDE_FUNC3)(PCI_SYSTEM3 *pSystem, PCI_LOC3 *pLoc, UINT devVend);
typedef UCHAR (*PCI_INT_ASSIGN_FUNC3)(PCI_SYSTEM3 *pSystem, PCI_LOC3 *pLoc, UINT devVend);
typedef void (*PCI_BRIDGE_PRE_CONFIG_FUNC3)(PCI_SYSTEM3 *pSystem, PCI_LOC3 *pLoc, UINT devVend);
typedef void (*PCI_BRIDGE_POST_CONFIG_FUNC3)(PCI_SYSTEM3 *pSystem, PCI_LOC3 *pLoc, UINT devVend);
typedef STATUS (*PCI_ROLL_CALL_FUNC3)();

void pciAutoConfig3 (PCI_SYSTEM3 *);

STATUS pciAutoGetNextClass3 ( PCI_SYSTEM3 *pSystem, PCI_LOC3 *pciFunc,
                             UINT *index, UINT pciClass, UINT mask);

STATUS pciAutoBusNumberSet3 ( PCI_LOC3 * pPciLoc, UINT primary, UINT secondary, UINT subordinate);

STATUS pciAutoDevReset3 ( PCI_LOC3 * pPciLoc);

STATUS pciAutoAddrAlign3 ( UINT32 base, UINT32 limit, UINT32 reqSize, UINT32 *pAlignedBase );

void pciAutoFuncEnable3 ( PCI_SYSTEM3 * pSystem, PCI_LOC3 * pFunc);
void pciAutoFuncDisable3 ( PCI_LOC3 *pPciFunc);

UINT pciAutoRegConfig3 ( PCI_SYSTEM3 * pSystem, PCI_LOC3 *pPciFunc,
    		UINT baseAddr, UINT nSize, UINT addrInfo);

void * pciAutoConfigLibInit3(void * pArg);
STATUS pciAutoCfgCtl3 ( void *pCookie, int cmd, void *pArg );
STATUS pciAutoCfg3( void *pCookie );

#endif	/* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* __INCpciAutoConfigLib3h */




