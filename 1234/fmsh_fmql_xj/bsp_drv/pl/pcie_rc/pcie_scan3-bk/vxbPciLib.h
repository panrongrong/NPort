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
10jun16,wap  Fix merge issue: AER support should be conditional on _WRS_KERNEL
11apr16,wap  Add support for building in user mode
30dec15,y_y  Add pcie AER support. (F4655)
10aug15,wap  Add library init routine
06aug15,wap  Restore missing IMPORT keyword on prototypes, fix typo in
             structure name
22jul15,wap  Save dynamic interrupt count in the PCI_HARDWARE_3 structure,
             remove duplicate prototype for vxbPciIoctl(), add prototype
             for vxbPciExtCapFind()
12jun15,wap  Add device manipulation ioctls (F3973)
10jun15,wap  Adjust types for resource structures to make their sizes
             consistent regardless of architecture, add baseParent
             field to BAR resource structure
08jun15,wap  Rework to support PCI bus bridge nodes and drivers (F3973)
18aug14,l_z  fix static analysis issue. (V7PRO-1181)
25apr14,l_z  add autoConfig flag. (US37630)
             remove vxbPciBusCfgRead/vxbPciBusCfgWrite routine.
             add static & dynamic interrupt entry to PCI_HARDWARE_3.
             move vxbFdtLib.h from vxbPci.c for PCI interrupt assgin method
08jan14,y_y  code clean and add extern routine declaration;
12jul13,j_z  Created.
*/

#ifndef __INCvxbPciLib3H
#define __INCvxbPciLib3H

#include "dlllib3.h"
/*#include "vxbPciMethod3.h"*/

/**/
/* ref bar address*/
/**/
#include "../ref_613/axipcie_v3_1/xparameters.h"

/*
#define UINT64 unsigned long long
#define UINT8 unsigned char
#define UINT16 unsigned short
#define UINT32 unsigned int
*/

/*#define IMPORT extern*/

#define BOOL int

#define VXB_DEV_ID3 int

#define VXB_KEY int
#define ULONG unsigned long


#if 0  /* jc: must change by the board*/

#define PCIE_EP_BAR_MAX_SIZE 0x10000000  /*256M*/

#define PCI_MEM_BASE         0x60000000    
#define PCI_MEM_SIZE         0x10000000  /* 256M*/

#else

/*#define XPAR_AXI_PCIE_0_AXIBAR_0          0x60000000*/
/*#define XPAR_AXI_PCIE_0_AXIBAR_HIGHADDR_0 0x6FFFFFFF*/
/*#define XPAR_AXI_PCIE_0_AXIBAR_AS_0 0*/
/*#define XPAR_AXI_PCIE_0_AXIBAR2PCIEBAR_0 0x60000000*/

#define PCI_MEM_BASE         XPAR_AXI_PCIE_0_AXIBAR_0    
#define PCI_MEM_SIZE         (XPAR_AXI_PCIE_0_AXIBAR_HIGHADDR_0 - XPAR_AXI_PCIE_0_AXIBAR_0 + 1)  /* 256M*/

#define PCIE_EP_BAR_MAX_SIZE 0x10000000  /*256M*/

#endif


#define PCI_BASE_BUS_NUM     0x1


typedef struct
    {
    DL_NODE3 node;
    UINT64   start;
    UINT64   size;
    UINT32   align;
    } PCI_ADDR_RESOURCE3;


typedef struct
    {
    UINT64  size;
    UINT32  align;
    UINT64  addr;
    } PCI_MMIO_DESC3;

typedef struct
    {
    UINT64   base;
    UINT64   size;
    DL_LIST3 list;
    UINT32  nodeCnt;
    } PCI_BAR_ADR_POOL3;

enum 
    {
    PCI_DEV_BAR0_INDEX         =  0,
    PCI_DEV_BAR1_INDEX         =  1,
    PCI_DEV_BAR2_INDEX         =  2,
    PCI_DEV_BAR3_INDEX         =  3,
    PCI_DEV_BAR4_INDEX         =  4,
    PCI_DEV_BAR5_INDEX         =  5,
    PCI_BRIDGE_MEM_BASE_INDEX  =  6,
    PCI_BRIDGE_IO_BASE_INDEX   =  7,
    PCI_BRIDGE_PRE_MEM_INDEX   =  8,
    PCI_ROM_BASE_INDEX         =  9,
    PCI_MAX_RESOURCE_NUMBER    =  10,
    PCI_DEVICE_TYPE            =  11,
    PCI_BRIDGE_TYPE            =  12,
    };

enum 
    {
    PCI_ROOT_RES_MEM32_IDX           =  0,
    PCI_ROOT_RES_IO_IDX              =  1,
    PCI_ROOT_RES_PREMEM32_IDX        =  2,
    PCI_ROOT_RES_MEM64_IDX           =  3,
    PCI_ROOT_RES_PREMEM64_IDX        =  4,
    PCI_ROOT_RES_MAX_IDX             =  5
    };

#define PCI_X86_IO_BASE             0x1000
#define PCI_RESOURCE_ERROR          0x80000000


struct barRes3
{
    UINT64     base;
    UINT64     baseParent;
    UINT64     len;
    UINT64     adjust;
};

/* (BAR0-BAR5), Bridge MEM/LIMIT 6,  IO 8, PRE MEM/LIMIT 8,, ROM 9 */

typedef struct pciResource3
{
    DL_NODE3   node;
	
    UINT64    start;
    UINT64    cpuBase;
    UINT64    end;
    UINT64    size;
    UINT32    align;
    UINT64    adjust;
    UINT32    type;
	
    DL_LIST3   listSubResSort; /* sub bridge resource list */
	
    struct pciResource3 * parentRes;
	
    PCI_BAR_ADR_POOL3     pool; /* bridge resource address pool */
} PCI_RES3;

typedef struct vxbPciDev3
{
    DL_NODE3      node;       /* children list node */
	
    UINT8        bus;
    UINT8        device;
    UINT8        func;
    UINT8        type;       /* bridge or device */
    UINT8        preIndex;   /* prefetch index mem/prem32/prem64*/
	
    struct vxbPciDev3 * parent;       /* parent pci device */
	
    DL_LIST3      children;   /* pci device on children bus */
    PCI_RES3      resource[PCI_MAX_RESOURCE_NUMBER];
} PCI_DEVICE3;
	
  /* jc*/
typedef struct pciRootRes3
{
    PCI_RES3      barRes3[PCI_ROOT_RES_MAX_IDX];
    PCI_DEVICE3 * pPciDev;
	
    UINT8        baseBusNumber;
    UINT8        endBusNumber;
	
    struct       pciRootChain3 * pRChan;
	
} PCI_ROOT_RES3;
	
typedef struct pciRootChain3
{
    PCI_ROOT_RES3 *     pRootRes;
    VXB_DEV_ID3         pRootDev;
	
    UINT32             segCount;
    BOOL               autoConfig;
	
   /* VXB_RESOURCE *    intRes;*/
	
#if defined (_WRS_KERNEL) && defined(_WRS_CONFIG_PCIE_AER)
    AER_INFO *         aerInfo;
    ISR_DEFER_QUEUE_ID queueId;
    ISR_DEFER_JOB      isrDef;
    SEM_ID             rpAerSem;
    /*
	*/
#endif /* _WRS_KERNEL && _WRS_CONFIG_PCIE_AER */

} PCI_ROOT_CHAIN3;
    
typedef struct vxbPciDevice_3
{
    UINT8   pciBus;
    UINT8   pciDev;
    UINT8   pciFunc;
    UINT8   pciIntPin;
    UINT16  pciDevId;
    UINT16  pciVendId;
	
#if 0
    VXB_RESOURCE_LIST vxbResList;
    VXB_INTR_ENTRY * pciIntxEntry;
	
#ifdef _WRS_KERNEL
    VXB_DYNC_INT_ENTRY * pciDyncIntEntry;
#endif
#endif

    UINT32  pciDyncIntCnt;
} PCI_HARDWARE_3;

typedef struct vxbPciDevMatchEntry3 
{
    UINT16      deviceId;
    UINT16      vendorId;
	
    const void *data;
} VXB_PCI_DEV_MATCH_ENTRY3;

/*
*/


#define NO_ALLOCATION       0xffffffff
#define PCI_CONFIG_ABSENT_F 0xffff
#define PCI_CONFIG_ABSENT_0 0x0000


#if 0  /* jc*/

#define VXB_BUSID_PCI3       BUSTYPE_ID(vxbPciBus) /* Nexus class device */


#define VXB_PCI_BUS_CFG_READ3(pDev, byteOffset, transactionSize, data)   \
    (void) VXB_PCI_CFG_READ_3(vxbDevParent(pDev), (PCI_HARDWARE_3 *)vxbDevIvarsGet(pDev), \
        byteOffset, transactionSize,  (void *)&data)

#define VXB_PCI_BUS_CFG_WRITE3(pDev, byteOffset, transactionSize, data)  \
    (void) VXB_PCI_CFG_WRITE_3(vxbDevParent(pDev), (PCI_HARDWARE_3 *)vxbDevIvarsGet(pDev), \
        byteOffset, transactionSize, (UINT32) data)
        
#endif



#if 0

IMPORT STATUS vxbPciAutoConfig
    (
    VXB_DEV_ID3  busCtrlID
    );

IMPORT STATUS vxbPciBusAddDev
    (
    VXB_DEV_ID3     pDev,
    UINT8          bus
    );

IMPORT STATUS vxbPciBusShutdown
    (
    VXB_DEV_ID3     pDev
    );

IMPORT STATUS vxbPciBusDetach
    (
    VXB_DEV_ID3     pDev
    );

IMPORT void vxbPciResourceGet3
    (
    VXB_DEV_ID3     pDev,
    PCI_DEVICE3 *   pPciDev
    );

IMPORT void vxbPciSetBarResPool
    (
    PCI_ROOT_RES3  * pciRootRes,
    struct barRes3 * res
    );

IMPORT int vxbPciConfigBdfPack
    (
    int busNo,    /* bus number */
    int deviceNo, /* device number */
    int funcNo    /* function number */
    );

IMPORT STATUS vxbPciDevMatch
    (
    VXB_DEV_ID3                      pDev,             /* device to do match */
    const VXB_PCI_DEV_MATCH_ENTRY3 * pMatchTbl,        /* pointer to match table */
    VXB_PCI_DEV_MATCH_ENTRY3 **      pMatchedEntry     /* best matched entry */
    );

IMPORT STATUS vxbPcieExtCapFind
    (
    VXB_DEV_ID3  pDev,
    UINT16      capability,
    UINT16 *    capreg
    );
    
IMPORT int vxbPcieDevTypeGet
    (
    VXB_DEV_ID3 pDev
    );
	 	
IMPORT STATUS vxbPciBusIoctl (VXB_DEV_ID3 pDev, int cmd, void * pArg);
IMPORT STATUS vxbPciExtCapFind (VXB_DEV_ID3 pDev, UINT8 requestedCapId, UINT8 * pOffset);

IMPORT STATUS vxbPciInit (void);
#endif

typedef struct vxbPciCfgArgs3
{
    VXB_KEY		vxbNodeKey;
    VXB_KEY		vxbTargetKey;
	
    UINT32		vxbOffset;
    UINT32		vxbWidth;
    UINT32		vxbVal;
} VXB_PCI_CFG_ARGS3;
/*
*/

struct vxbPciConfig3
{
    int         pciMaxBus;	/* Max number of sub-busses */
    STATUS      pciLibInitStatus;
    int         pciMinBus;	/* Minimum number of sub-busses */
};


/* typedefs */

#define ZYNQ_AXI_PCIE_MSI_NUM           32

typedef struct zynq7kAxi2PcieDrvCtrl
{
    PCI_ROOT_CHAIN3      rc;  /* must first */
    VXB_DEV_ID3          pDev;
	
   /* SEM_ID              pciexDevSem;*/
	
    UINT32              memIo32Addr;
   
    void *              regBase;
    void *              handle;
	
    BOOL                linkStatus;
	
    struct vxbPciConfig3 * pPciConfig;
    /* struct vxbPciInt * pIntInfo;*/
    /* struct fdtPciIntr   pciIntr;*/
	
   /* DL_LIST3             interruptList;*/
   /* VXB_RESOURCE *     intRes;*/
	
    UINT32              offset;
	
    UINT32              intBase;
    UINT64              msiAdr;
    UINT32              msiBase;
	
    UINT16              msiFreeInt[ZYNQ_AXI_PCIE_MSI_NUM];
	
    int                 msiFreeIndex;
    UINT32              assertedINTx;
} ZYNQ7K_AXI2PCIE_DRV_CTRL;


#endif /* __INCvxbPciLib3H */

