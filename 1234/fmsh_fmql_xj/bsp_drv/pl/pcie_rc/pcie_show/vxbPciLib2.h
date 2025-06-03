/* vxbPciLib.h - PCI lib header file for vxBus */

/*
 * Copyright (c) 2013-2016 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01a,23sep21,jc  written.
*/

#ifndef __INCvxbPciLib2H
#define __INCvxbPciLib2H

/*#include <types/vxTypesOld.h>*/
/*#include <dllLib.h>*/
/*#include <hwif/vxBus.h>*/
/*#include <subsys/int/vxbIntLib.h>*/

#ifdef _WRS_KERNEL
/*#include <subsys/int/vxbDyncIntLib.h>*/
#endif

#if defined (_WRS_KERNEL) && defined(_WRS_CONFIG_PCIE_AER)
/*#include <hwif/buslib/vxbPcieAer.h>*/
#endif /* _WRS_KERNEL && _WRS_CONFIG_PCIE_AER */

/* jc*/
#include "pciDefines2.h"


typedef struct
    {
    DL_NODE node;
    UINT64   start;
    UINT64   size;
    UINT64   align;
    } PCI_ADDR_RESOURCE;


typedef struct
    {
    UINT64  size;
    UINT64  align;
    UINT64  addr;
    } PCI_MMIO_DESC;

typedef struct
    {
    UINT64   base;
    UINT64   size;
    DL_LIST  list;
    UINT32   nodeCnt;
    } PCI_BAR_ADR_POOL;

enum
    {
    PCI_DEV_BAR0_INDEX2         =  0,
    PCI_DEV_BAR1_INDEX2         =  1,
    PCI_DEV_BAR2_INDEX2         =  2,
    PCI_DEV_BAR3_INDEX2         =  3,
    PCI_DEV_BAR4_INDEX2         =  4,
    PCI_DEV_BAR5_INDEX2         =  5,
    PCI_BRIDGE_MEM_BASE_INDEX2  =  6,
    PCI_BRIDGE_IO_BASE_INDEX2   =  7,
    PCI_BRIDGE_PRE_MEM_INDEX2   =  8,
    PCI_ROM_BASE_INDEX2         =  9,
    PCI_MAX_RESOURCE_NUMBER2    =  10,
    PCI_DEVICE_TYPE2            =  11,
    PCI_BRIDGE_TYPE2            =  12,
    };

enum
    {
    PCI_ROOT_RES_MEM32_IDX2           =  0,
    PCI_ROOT_RES_IO_IDX2              =  1,
    PCI_ROOT_RES_PREMEM32_IDX2        =  2,
    PCI_ROOT_RES_MEM64_IDX2           =  3,
    PCI_ROOT_RES_PREMEM64_IDX2        =  4,
    PCI_ROOT_RES_MAX_IDX2             =  5
    };

#define PCI_X86_IO_BASE             0x1000
#define PCI_RESOURCE_ERROR          0x80000000


struct barRes
    {
    UINT64     base;
    UINT64     baseParent;
    UINT64     len;
    UINT64     adjust;
    };

/* (BAR0-BAR5), Bridge MEM/LIMIT 6,  IO 8, PRE MEM/LIMIT 8,, ROM 9 */

typedef struct pciResource
    {
    DL_NODE   node;
    UINT64    start;
    UINT64    cpuBase;
    UINT64    end;
    UINT64    size;
    UINT64    align;
    UINT64    adjust;
    UINT32    type;
    DL_LIST   listSubResSort; /* sub bridge resource list */
    struct pciResource * parentRes;
    PCI_BAR_ADR_POOL     pool; /* bridge resource address pool */
    } PCI_RES;

typedef struct vxbPciDev
    {
    DL_NODE      node;       /* children list node */
    UINT8        bus;
    UINT8        device;
    UINT8        func;
    UINT8        type;       /* bridge or device */
    UINT8        preIndex;   /* prefetch index mem/prem32/prem64*/
    struct vxbPciDev * parent;       /* parent pci device */
    DL_LIST      children;   /* pci device on children bus */
    PCI_RES      resource[PCI_MAX_RESOURCE_NUMBER2];
    } PCI_DEVICE;

typedef struct pciRootRes
    {
    PCI_RES      barRes[PCI_ROOT_RES_MAX_IDX2];
    PCI_DEVICE * pPciDev;
    UINT8        baseBusNumber;
    UINT8        endBusNumber;
    struct       pciRootChain * pRChan;
    } PCI_ROOT_RES;

typedef struct pciRootChain
    {
    PCI_ROOT_RES *     pRootRes;
    VXB_DEV_ID         pRootDev;
    UINT32             segCount;
    BOOL               autoConfig;
    VXB_RESOURCE *     intRes;
#if defined (_WRS_KERNEL) && defined(_WRS_CONFIG_PCIE_AER)
    AER_INFO *         aerInfo;
    ISR_DEFER_QUEUE_ID queueId;
    ISR_DEFER_JOB      isrDef;
    SEM_ID             rpAerSem;
#endif /* _WRS_KERNEL && _WRS_CONFIG_PCIE_AER */
    } PCI_ROOT_CHAIN;

typedef struct vxbPciCtrl {
    SL_NODE node;
    VXB_DEV_ID pDev;
} VXB_PCI_CTRL;

#if 0
typedef struct vxbPciDevice
    {
    UINT8   pciBus;
    UINT8   pciDev;
    UINT8   pciFunc;
    UINT8   pciIntPin;
    UINT16  pciDevId;
    UINT16  pciVendId;
    VXB_RESOURCE_LIST vxbResList;
    VXB_INTR_ENTRY * pciIntxEntry;
#ifdef _WRS_KERNEL
    VXB_DYNC_INT_ENTRY * pciDyncIntEntry;
#endif
    UINT32  pciDyncIntCnt;
    } PCI_HARDWARE;
#endif

typedef struct vxbPciDevMatchEntry {
    UINT16      deviceId;
    UINT16      vendorId;
    const void *data;
} VXB_PCI_DEV_MATCH_ENTRY;


#define VXB_BUSID_PCI       BUSTYPE_ID(vxbPciBus) /* Nexus class device */

#define NO_ALLOCATION       0xffffffff
#define PCI_CONFIG_ABSENT_F 0xffff
#define PCI_CONFIG_ABSENT_0 0x0000

#define VXB_PCI_BUS_CFG_READ(pDev, byteOffset, transactionSize, data)   \
    (void) VXB_PCI_CFG_READ(vxbDevParent(pDev), (PCI_HARDWARE *)vxbDevIvarsGet(pDev), \
        byteOffset, transactionSize,  (void *)&data)

#define VXB_PCI_BUS_CFG_WRITE(pDev, byteOffset, transactionSize, data)  \
    (void) VXB_PCI_CFG_WRITE(vxbDevParent(pDev), (PCI_HARDWARE *)vxbDevIvarsGet(pDev), \
        byteOffset, transactionSize, (UINT32) data)

/*  // jc
IMPORT STATUS vxbPciAutoConfig
    (
    VXB_DEV_ID  busCtrlID
    );
*/

IMPORT STATUS vxbPciBusAddDev
    (
    VXB_DEV_ID     pDev,
    UINT8          bus
    );

IMPORT STATUS vxbPciBusShutdown
    (
    VXB_DEV_ID     pDev
    );

IMPORT STATUS vxbPciBusDetach
    (
    VXB_DEV_ID     pDev
    );

IMPORT void vxbPciResourceGet
    (
    VXB_DEV_ID     pDev,
    PCI_DEVICE *   pPciDev
    );

IMPORT void vxbPciSetBarResPool
    (
    PCI_ROOT_RES  * pciRootRes,
    struct barRes * res
    );

IMPORT int vxbPciConfigBdfPack
    (
    int busNo,    /* bus number */
    int deviceNo, /* device number */
    int funcNo    /* function number */
    );

IMPORT STATUS vxbPciDevMatch
    (
    VXB_DEV_ID                      pDev,             /* device to do match */
    const VXB_PCI_DEV_MATCH_ENTRY * pMatchTbl,        /* pointer to match table */
    VXB_PCI_DEV_MATCH_ENTRY **      pMatchedEntry     /* best matched entry */
    );

IMPORT STATUS vxbPcieExtCapFind
    (
    VXB_DEV_ID  pDev,
    UINT16      capability,
    UINT16 *    capreg
    );
    
IMPORT int vxbPcieDevTypeGet
    (
    VXB_DEV_ID pDev
    );
	 	
IMPORT STATUS vxbPciBusIoctl (VXB_DEV_ID pDev, int cmd, void * pArg);
IMPORT STATUS vxbPciExtCapFind (VXB_DEV_ID pDev, UINT8 requestedCapId, UINT8 * pOffset);

IMPORT STATUS vxbPciInit (void);

typedef struct vxbPciCfgArgs
    {
    VXB_KEY		vxbNodeKey;
    VXB_KEY		vxbTargetKey;
    UINT32		vxbOffset;
    UINT32		vxbWidth;
    UINT32		vxbVal;
    } VXB_PCI_CFG_ARGS;

/*#include <hwif/methods/vxbPciMethod.h>*/

#endif /* __INCvxbPciLib2H */
