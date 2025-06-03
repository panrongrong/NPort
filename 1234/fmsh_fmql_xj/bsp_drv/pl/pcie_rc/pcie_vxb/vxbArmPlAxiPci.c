/* vxbArmPlAxiPci.c - vxbArmPlAxiPci VxBus PCI driver */

/*
 * Copyright (c) 2009-2010, 2013 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01c,08mar13,c_l  Remove build warnings. (WIND00406706)
01b,04feb10,d_l  cleanup a compiler error.(WIND00160416)
01a,09jan09,fao  written
*/

/*
DESCRIPTION
This module provides VxBus driver support for the ARM_PBXA9 Northbridge AXI to 
PCIx bridge controller. This driver support for PCI memory mapped register 
access and PCI configuration access.

An example hwconf entry is shown below:

\cs
LOCAL  const struct hcfResource armPlAxiPci0Resources[] = {
    { "regBase",        HCF_RES_INT,    { (void *)PCI_REGBASE } },
    { "mem32Addr",      HCF_RES_ADDR,   { (void *)PCI_MEM_ADRS } },
    { "mem32Size",      HCF_RES_INT,    { (void *)PCI_MEM_SIZE } },
    { "memIo32Addr",    HCF_RES_ADDR,   { (void *)PCI_MEMIO_ADRS } },
    { "memIo32Size",    HCF_RES_INT,    { (void *)PCI_MEMIO_SIZE } },
    { "io32Addr",       HCF_RES_ADDR,   { (void *)PCI_IO_ADRS } },
    { "io32Size",       HCF_RES_INT,    { (void *)PCI_IO_SIZE } },
    { "io16Addr",       HCF_RES_ADDR,   { (void *)PCI_IO16_ADRS } },
    { "io16Size",       HCF_RES_INT,    { (void *)PCI_IO16_SIZE } },
    { "cacheSize",      HCF_RES_INT,    { (void *)(_CACHE_ALIGN_SIZE / 4) } },
    { "maxBusSet",      HCF_RES_INT,    { (void *)3 } },
    { "maxLatAllSet",   HCF_RES_INT,    { (void *)PCI_LAT_TIMER } },
    { "autoIntRouteSet",HCF_RES_INT,    { (void *)TRUE } },
    { "includeFuncSet", HCF_RES_ADDR,   { (void *)sysPciAutoconfigInclude } },
    { "intAssignFuncSet",HCF_RES_ADDR,  { (void *)sysPciAutoconfigIntrAssign }},
};

#define armPlAxiPci0Num NELEMENTS(armPlAxiPci0Resources)

const struct hcfDevice hcfDeviceList[] = {

        ...

    { "vxbArmPlAxiPci", 0, VXB_BUSID_PLB,0, armPlAxiPci0Num,
                                         armPlAxiPci0Resources },
        ...
};
\ce

INCLUDE FILES: vxbPciBus.h vxbAccess.h

SEE ALSO: vxbPci and VxWorks Device Driver Developer's Guide.
*/

#include <vxWorks.h>
#include <vxAtomicLib.h>
#include <drv/pci/pciIntLib.h>
#include <drv/pci/pciConfigLib.h>
#include <iv.h>
#include <intLib.h>
#include <stdlib.h>
#include <cacheLib.h>

#include <hwif/vxbus/vxBus.h>
#include <vxBusLib.h>
#include <hwif/vxbus/hwConf.h>
#include <hwif/vxbus/vxbPciLib.h>
#include <hwif/util/hwMemLib.h>
#include <../src/hwif/h/vxbus/vxbAccess.h>
#include <../src/hwif/h/vxbus/vxbPciBus.h>
#include <../src/hwif/intCtlr/vxbIntCtlrLib.h>
#include <../src/hwif/intCtlr/vxbIntDynaCtlrLib.h>

#include "vxbArmPlAxiPci.h"

/*
 * Putting device-specific structures and register information in
 * a separate header file is recommended, though not required. It helps
 * reduce clutter in the source code if the device has many registers.
 */

/* define */


#ifdef ARMAXIPCI_DEBUG

int usrArmAxiPciBusDebugLevel = 0x0;
int pciDeviceCount = 0;

    #ifdef DEBUG_USING_PRINTF
        #define ARMAXI_PCI_DEBUG_MSG(lvl,fmt,a,b,c,d,e,f) if ( usrArmAxiPciBusDebugLevel >= lvl ) printf(fmt,a,b,c,d,e,f)
    #else /* DEBUG_USING_PRINTF */
        #define ARMAXI_PCI_DEBUG_MSG(lvl,fmt,a,b,c,d,e,f) if ( usrArmAxiPciBusDebugLevel >= lvl ) logMsg(fmt,a,b,c,d,e,f)
    #endif /* DEBUG_USING_PRINTF */

#else /* ARMAXIPCI_DEBUG */

    #undef ARMAXI_PCI_DEBUG_MSG
    #define ARMAXI_PCI_DEBUG_MSG(lvl,fmt,a,b,c,d,e,f)

#endif /* ARMAXIPCI_DEBUG */



/* VxBus methods */

LOCAL void  armPlAxiPciInstInit (VXB_DEVICE_ID);
LOCAL void  armPlAxiPciInstInit2 (VXB_DEVICE_ID);
LOCAL void  armPlAxiPciInstConnect (VXB_DEVICE_ID);

/* PCI methods */

LOCAL STATUS    armPlAxiPciMethodDevCfgRead (VXB_DEVICE_ID, PCI_HARDWARE *,
    UINT32, UINT32, void *);
LOCAL STATUS    armPlAxiPciMethodDevCfgWrite (VXB_DEVICE_ID, PCI_HARDWARE *,
    UINT32, UINT32, UINT32);

LOCAL STATUS    armPlAxiPciConfigInfo (VXB_DEVICE_ID, char *);
LOCAL STATUS    armPlAxiPciInterruptInfo (VXB_DEVICE_ID, char *);

/* Access methods */

LOCAL STATUS    armPlAxiPciDevControl (VXB_DEVICE_ID, pVXB_DEVCTL_HDR);

/* Chip-specific initialization code. */

LOCAL void  armPlAxiPciBridgeInit (VXB_DEVICE_ID);

/* locals */

LOCAL struct drvBusFuncs armPlAxiPciFuncs =
    {
    armPlAxiPciInstInit,          /* devInstanceInit */
    armPlAxiPciInstInit2,         /* devInstanceInit2 */
    armPlAxiPciInstConnect        /* devInstanceConnect */
    };

LOCAL device_method_t armPlAxiPciMethodList[] =
    {

    DEVMETHOD(busCtlrDevCfgRead, armPlAxiPciMethodDevCfgRead),
    DEVMETHOD(busCtlrDevCfgWrite, armPlAxiPciMethodDevCfgWrite),
    DEVMETHOD(busCtlrDevCtlr, armPlAxiPciDevControl),

    { PCI_CONTROLLER_METHOD_CFG_INFO, (FUNCPTR)armPlAxiPciConfigInfo},
    { PCI_CONTROLLER_METHOD_INTERRUPT_INFO, (FUNCPTR)armPlAxiPciInterruptInfo},

    DEVMETHOD_END
    };

LOCAL struct vxbDevRegInfo armPlAxiPciDevRegistration =
    {
    NULL,                 /* pNext */
    VXB_DEVID_BUSCTRL,    /* pDev */
    VXB_BUSID_PLB,        /* busID = Processor Local Bus */
    VXB_VER_4_0_0,        /* vxbVersion */
    "vxbArmPlAxiPci",       /* drvName */
    &armPlAxiPciFuncs,      /* pDrvBusFuncs */
    NULL,                 /* pMethods */
    NULL,                 /* devProbe */
    NULL                  /* pParamDefaults */
    };

/******************************************************************************
*
* vxbArmAxiPciRegister - register armPlAxiPci driver
*
* This routine registers the armPlAxiPci driver with the vxBus subsystem.
*
* NOTE:
*
* This routine is called early during system initialization, and
* *MUST NOT* make calls to OS facilities such as memory allocation
* and I/O.
*
* RETURNS: N/A
*/

void vxbArmPlAxiPciRegister(void)
    {
    vxbDevRegister ((struct vxbDevRegInfo *)&armPlAxiPciDevRegistration);
    }

/******************************************************************************
*
* armPlAxiPciInstInit - initialize armPlAxiPci device
*
* This is the armPlAxiPci initialization routine.
*
* NOTE:
*
* This routine is called early during system initialization, and
* *MUST NOT* make calls to OS facilities such as memory allocation
* and I/O.
*
* RETURNS: N/A
*/

LOCAL void armPlAxiPciInstInit
    (
    VXB_DEVICE_ID pDev
    )
    {
    STATUS stat;
    VXB_ARMAXIPCI_DRV_CTRL * pDrvCtrl;
    struct hcfDevice * pHcf;

    /* Allocate pDrvCtrl */

    pDrvCtrl = (VXB_ARMAXIPCI_DRV_CTRL *)hwMemAlloc (sizeof (VXB_ARMAXIPCI_DRV_CTRL));

    if (pDrvCtrl == NULL)
        return;

    vxbRegMap (pDev, 0, &pDrvCtrl->handle);
    pDrvCtrl->regBase = pDev->pRegBase[0];
    pDrvCtrl->initDone = FALSE;
    pDrvCtrl->pInst = pDev;

    pDrvCtrl->pPciConfig =
        (struct vxbPciConfig *)hwMemAlloc(sizeof(struct vxbPciConfig));

    if (pDrvCtrl->pPciConfig == NULL)
        goto fail;

    pDrvCtrl->pIntInfo =
        (struct vxbPciInt *)hwMemAlloc(sizeof(struct vxbPciInt));

    if (pDrvCtrl->pPciConfig == NULL)
        goto fail;

    pDev->pDrvCtrl = pDrvCtrl;

    /*
     * A PCI bridge driver usually wants to perform autoconfiguration
     * of devices on the bus, particularly if VxWorks is the first thing
     * that runs when the CPU comes out of reset (i.e. there is no other
     * BIOS or firmware to do this work for us). This flag can be set
     * using a resource entry in hwconf if desired.
     */

    pDrvCtrl->autoConfig = TRUE;

    /* Initialize PCI interrupt handling */

    (void) vxbPciIntLibInit (pDrvCtrl->pIntInfo);

    pHcf = (struct hcfDevice *)hcfDeviceGet (pDev);

    if (pHcf == NULL)
        goto fail;

    /*
     * resourceDesc {
     * The autoConfig resource specifies whether
     * Auto Configuration is enabled or not. }
     */

    stat = devResourceGet(pHcf, "autoConfig", HCF_RES_INT, (void*)&pDrvCtrl->autoConfig);

    /*
     * resourceDesc {
     * The maxBusSet resource specifies the
     * highest sub-bus number. }
     */

    stat = devResourceGet(pHcf, "maxBusSet", HCF_RES_INT, (void *)&pDrvCtrl->pciMaxBus);

    if ( vxbPciConfigLibInit(pDrvCtrl->pPciConfig,  pDrvCtrl->pciMaxBus) != OK)
    goto fail;

    /* initialize PCI interrupt library */

    pDev->pMethods = &armPlAxiPciMethodList[0];

    return;

fail:

    if (pDrvCtrl->pPciConfig != NULL)
        hwMemFree ((char *)pDrvCtrl->pPciConfig);
    if (pDrvCtrl->pIntInfo != NULL)
        hwMemFree ((char *)pDrvCtrl->pIntInfo);
    hwMemFree ((char *)pDrvCtrl);

    return;
    }

/******************************************************************************
*
* armPlAxiPciInstInit2 - second stage of armAxi pci device initialization
*
* This is the routine called to perform the second stage of armPlAxiPci
* initialization.
*
* RETURNS: N/A
*
* ERRNO
*/

LOCAL void armPlAxiPciInstInit2
    (
    VXB_DEVICE_ID pDev
    )
    {
    VXB_ARMAXIPCI_DRV_CTRL * pDrvCtrl;

    pDrvCtrl = pDev->pDrvCtrl;

    /* Perform bridge hardware initialization. */

    (void) armPlAxiPciBridgeInit (pDev);

    /* Announce the creation of the bus. */

    (void) vxbBusAnnounce (pDev, VXB_BUSID_PCI);

     /* perform PCI autoconfiguration */

    if (pDrvCtrl->autoConfig == TRUE)
        vxbPciAutoConfig (pDev);

    /* call the PCI bus type initialization function */

    (void) vxbPciBusTypeInit (pDev);

    pDrvCtrl->initDone = TRUE;

    }

/******************************************************************************
*
* armPlAxiPciInstConnect - third stage of armAxi pci device initialization
*
* This is the routine called to perform the third stage of armPlAxiPci
* initialization.
*
* RETURNS: N/A
*
* ERRNO
*/

LOCAL void armPlAxiPciInstConnect
    (
    VXB_DEVICE_ID pDev
    )
    {

    /* nothing is done here */

    return;
    }

	
#if 1  /* jc*/

/*******************************************************************************
*
* armPlAxiPciLinkUp - check whether PCIe link is up
*
* This routine checks the PCIe link up/down status.
*
* RETURNS: TRUE or FALSE if link is down
*
* ERRNO: N/A
*/

LOCAL BOOL armPlAxiPciLinkUp
    (
    VXB_DEVICE_ID pDev
    )
{
    int i = 0;
    int status = 0;

    VXB_ARMAXIPCI_DRV_CTRL * pDrvCtrl = pDev->pDrvCtrl;

    if (pDrvCtrl->linkStatus)
    {
        return TRUE;
    }

    while (i < MAX_LINK_UP_CHECK_COUNT)
    {
        status = CSR_READ_4 (pDev, A2P_PHY_STATUS);
        if ((status & A2P_PHY_LINK_UP_MASK) != 0)
        {
            pDrvCtrl->linkStatus = TRUE;
            return TRUE;
        }
		
        i++;
        vxbUsDelay (10);
    }
	
    return FALSE;
}
	

/******************************************************************************
*
* armPlAxiPciBridgeInit - PCI bridge initialization
*
* This routine performs initialization of the PCI bridge hardware. This
* may include resetting the bridge device and programming its window
* translation registers.
*
* RETURNS: N/A
*
* ERRNO
*/

LOCAL void armPlAxiPciBridgeInit
    (
    VXB_DEVICE_ID pDev
    )
{
    VXB_ARMAXIPCI_DRV_CTRL * pDrvCtrl;
    struct hcfDevice * pHcf;
    UINT32 data = 0;
	int Status;
	
    pDrvCtrl = pDev->pDrvCtrl;

    /* initialize the bridge chip */
	
	/* 
	Initialize Root Complex
	*/
#if 1  /* ref_bare-code*/
	extern int pl_pcie_rc_init(void);	
	extern int pl_pcie_rc_init_2(void);	
	
	/*Status = pl_pcie_rc_init();*/
	Status = pl_pcie_rc_init_2();
	if (Status != 0/*XST_SUCCESS*/) 
	{
		printf("Failed to initialize AXI PCIe Root port\r\n");
	}
	
#else

    UINT32 val;

	/* jc*/
	if (OK == armPlAxiPciLinkUp(pDev))
	{	
		printf("PCIe Link is UP \n");
	}
	else
	{	
		printf("PCIe Link is DOWN \n");
	}

	
	/* PCIe Bus Master Enable */
	CSR_WRITE_4 (pDev, A2P_PCIE_CFG_COMMAND, 0x00000004);
	val = CSR_READ_4 (pDev, A2P_PCIE_CFG_COMMAND);

	/* Disable all interrupts */
	CSR_WRITE_4 (pDev, A2P_INTERRUPT_MASK, A2P_INTERRUPTS_DISABLE);

	/* Clear pending interrupts */
	val = CSR_READ_4(pDev, A2P_INTERRUPT_DECODE);
	val |= A2P_IMR_MASKALL; /* vx7-origin*/
	CSR_WRITE_4 (pDev,A2P_INTERRUPT_DECODE, val);

	/* Enable all the interrupts */
	CSR_WRITE_4 (pDev, A2P_INTERRUPT_MASK, A2P_IMR_MASKALL);  /* vx7-origin*/

	/* Enable the bridge */
	val = CSR_READ_4(pDev, A2P_ROOT_PORT_STATUS);
	val |= A2P_ROOT_PORT_BRIDGE_ENABLE;
	CSR_WRITE_4 (pDev, A2P_ROOT_PORT_CONTROL, val);

	/* Update the address translation register */
	pDrvCtrl->io32Addr = 0x60000000;  /* for test*/
	CSR_WRITE_4 (pDev, AXIBAR2PCIEBAR_0L, pDrvCtrl->io32Addr); /* 0x6000_0000*/
	printf("0x20C: 0x%08X \n", pDrvCtrl->io32Addr);
#endif

    return;
}

	
/*******************************************************************************
*
* armPlAxiPciParameterCheck - parameter validity checked
*
* This is the ZYNQ 7K AXI 2 PCIe CFG parameter validity check function.
*
* RETURNS: TRUE/FALSE
*
* ERRNO: N/A
*/

LOCAL BOOL armPlAxiPciParameterCheck
    (
    VXB_DEVICE_ID    pDev,
    int           bus,
    int           dev,
    int           fn,
    void        * pData
    )
{
    if ((pDev == NULL) || (pData == NULL))
    {
        return (FALSE);
    }

    if ((bus == 0) && (dev == 0))
    {
        return TRUE;
    }

    /* ECAM Size determines this value */
    if (bus > ZYNQ_AXI_PCIE_MAX_BUS_NUM)
    {
        return (FALSE);
    }

    if (!armPlAxiPciLinkUp (pDev))
    {
        printf ("%s:fatal error - NO LINK - B-D-F: [%d - %d - %d] pDev: [0x%08x]!\n",
                 "armPlAxiPciParameterCheck", bus, dev, fn, pDev, 0);

        /* if no link, only host access is allowed */
        if (bus != 0)
        {
            return (FALSE);
        }
    }

    /* 
     * check whether it is a valid access.
     * 1. every root port has only one slot.
     *    so, if (bus ==0 && devNum > 0), just return.
     * 2. there is only one device on the bus directly attached to RC's.
     *    so, if (bus ==1 && devNum > 0), just return.
     */
    if ((bus <= 1) && (dev != 0))
    {
        return (FALSE);
    }

    return TRUE;
}

/******************************************************************************
*
* armPlAxiPciMethodDevCfgRead - VxBus PCI config read method
*
* This is the PCI configuration read method exported by this driver to
* the VxBus vxbPci.c module. It performs a configuration space read on a
* given PCI target. If the target specification or register offset is
* invalid, it will return error.
*
* RETURNS: OK, or ERROR if target specification or register is invalid
*
* ERRNO: N/A
*/

LOCAL STATUS armPlAxiPciMethodDevCfgRead
    (
    VXB_DEVICE_ID pDev,     /* device info */
    PCI_HARDWARE * pPciDev, /* PCI device info */
    UINT32 byteOffset,      /* offset into cfg space */
    UINT32 transactionSize, /* transaction size, in bytes */
    void * pDataBuf         /* buffer to read-from/write-to */
    )
{
    STATUS retVal = OK;
    VXB_ARMAXIPCI_DRV_CTRL * pDrvCtrl;
	
    int bus, dev, func;
    unsigned int attr;
    unsigned int mode;
    unsigned int config_addr;

    pDrvCtrl = pDev->pDrvCtrl;

    bus = pPciDev->pciBus;
    dev = pPciDev->pciDev;
    func = pPciDev->pciFunc;

#if 0
    if (bus == 0 && dev == 31)
        return (ERROR);
	
	extern STATUS pciRemote_Cfg_Read
		(
		int busNo,	  /* bus number */
		int deviceNo, /* device number */
		int funcNo,   /* function number */
		int offset,   /* offset into the configuration space */
		int size,
		void * pData /* data read from the offset */
		);
	
	pciRemote_Cfg_Read(bus, dev, func, byteOffset, transactionSize, pDataBuf);
	
    return (OK);
	
#else

    STATUS sts = OK;
    UINT32 extReg = 0; 
	UINT32 reg, size, address;
    UINT32 val;
	
	reg = byteOffset;
	size = transactionSize;
    address = ( (bus << 20) | (dev << 15) | (func << 12) | (extReg << 8) | (reg & 0xFFFC) );

    if (!armPlAxiPciParameterCheck (pDev, bus, dev, func, pDataBuf))
    {
        printf ("%s: warning - invalid parameter B-D-F:[%d-%d-%d] \
				pDev: [0x%08x] pData: [0x%08x] \n",
				"armPlAxiPciMethodDevCfgRead", bus, dev, func, pDev, pDataBuf);
		
        *(UINT32 *) pDataBuf = 0;
		
        return (ERROR);
    }
  
    /* 
     * In autoConfig stage, BAR0/BAR1 registers been written 0xfffffff, and then
     * read back. The read back value used to calculate the RC private register
     * space. But the read value doesn't present the correct private register
     * space needed by RC.
     * Since the bridge hasn't private register, BAR0/BAR1 read should return 0 
     * to make autoConfig work correctly.
     */

    if ((bus == 0) && (reg == PCI_CFG_BASE_ADDRESS_0 || reg == PCI_CFG_BASE_ADDRESS_1 ))
    {
        *(UINT32 *) pDataBuf = 0;
        return OK;
    }

    /*(void)semTake (pDrvCtrl->pciexDevSem, WAIT_FOREVER);*/

    val = CSR_READ_4(pDev, address);

    switch (size)
    {
        case 4:
            *(UINT32 *) pDataBuf = val;
            break;
        case 2:
            *(UINT16 *) pDataBuf = (val >> (8 * (reg & 3))) & 0xffff;
            break;
        case 1:
            *(UINT8 *) pDataBuf = (val >> (8 * (reg & 3))) & 0xff;
            break;
        default:
            sts = ERROR;
            break;
    }

    /*VX_MEM_BARRIER_W();*/
    /*(void)semGive (pDrvCtrl->pciexDevSem);*/

    return (sts);
#endif
}

/******************************************************************************
*
* armPlAxiPciMethodDevCfgWrite - VxBus PCI config write method
*
* This is the PCI configuration write method exported by this driver to
* the VxBus vxbPci.c module. It performs a configuration space write on a
* given PCI target. If the target specification or register offset is
* invalid, it will return error.
*
* RETURNS: OK, or ERROR if target specification or register is invalid
*
* ERRNO: N/A
*/

LOCAL STATUS armPlAxiPciMethodDevCfgWrite
    (
    VXB_DEVICE_ID pDev,     /* device info */
    PCI_HARDWARE * pPciDev, /* PCI device info */
    UINT32 byteOffset,      /* offset into cfg space */
    UINT32 transactionSize, /* transaction size, in bytes */
    UINT32 data             /* data write to the offset */
    )
{
    STATUS retVal = OK;
    VXB_ARMAXIPCI_DRV_CTRL * pDrvCtrl;
	
    int bus, dev, func;
    unsigned int mode;
    unsigned int attr;
    unsigned int config_addr;

    pDrvCtrl = pDev->pDrvCtrl;

    bus = pPciDev->pciBus;
    dev = pPciDev->pciDev;
    func = pPciDev->pciFunc;

#if 0
    if ((bus == 0 && dev == 31))
        return (ERROR);

	
	extern STATUS pciRemote_Cfg_Write
		(
		int busNo,	  /* bus number */
		int deviceNo, /* device number */
		int funcNo,   /* function number */
		int offset,   /* offset into the configuration space */
		int size,
		UINT32 data    /* data written to the offset */
		);
	
	pciRemote_Cfg_Write(bus, dev, func, byteOffset, transactionSize, data);
	
    return (OK);
	
#else

    STATUS sts = OK;
    UINT32 extReg = 0;
	UINT32 reg, size, address;
    UINT32 val;
    int part = 0;
	
	reg = byteOffset;
	size = transactionSize;
    address = ( (bus << 20) | (dev << 15) | (func << 12) | (extReg << 8) | (reg & 0xFFFC) );

    if (!armPlAxiPciParameterCheck (pDev, bus, dev, func, &data))
    {
        printf ("%s:warning - invalid parameter\n",
                "armPlAxiPciMethodDevCfgWrite", 0, 0, 0, 0, 0);
		
        return (ERROR);
    }

    if ((bus == 0) && (reg == PCI_CFG_BASE_ADDRESS_0 || reg == PCI_CFG_BASE_ADDRESS_1))
    {
        return (OK);
    }

    /*(void)semTake (pDrvCtrl->pciexDevSem, WAIT_FOREVER);*/
    switch (size)
	{
    case 4:
        val = data;
        break;
	
    case 2:
        val = CSR_READ_4(pDev, address);
        part = (0x0002 & reg);
        if ( part == 0 )
        {
            val &= 0xFFFF0000;
            val |= data & 0xffff;
        }
        else if ( part == 2 )
        {
            val &= 0x0000FFFF;
            val |= ( (data & 0xffff) << 16 );
        }
        break;
		
    case 1:
        val = CSR_READ_4(pDev, address);
        part = (0x0003 & reg );
        if (part == 0)
        {
            val &= 0xFFFFFF00;
            val |= ( (data & 0xff) << 0 );
        }
        else if (part == 1)
        {
            val &= 0xFFFF00FF;
            val |= ( (data & 0xff) << 8 );
        }
        else if (part == 2)
        {
            val &= 0xFF00FFFF;
            val |= ( (data & 0xff) << 16 );
        }
        else if (part == 3)
        {
            val &= 0x00FFFFFF;
            val |= ( (data & 0xff) << 24 );
        }
        break;
		
    default:
        sts = ERROR;
        break;
    }

    CSR_WRITE_4(pDev, address, val);

    /*(void)semGive (pDrvCtrl->pciexDevSem);*/
    return(sts);
#endif
}

#endif

/*******************************************************************************
*
* armPlAxiPciDevControl - device control routine
*
*  This routine handles manipulation of downstream devices, such as
*  interrupt management.
*
*/

LOCAL STATUS armPlAxiPciDevControl
    (
    VXB_DEVICE_ID   devID,
    pVXB_DEVCTL_HDR pBusDevControl
    )
    {
    VXB_ASSERT(devID != NULL && pBusDevControl != NULL, ERROR)

    switch ( pBusDevControl->vxbAccessId)
        {
        case VXB_ACCESS_INT_VEC_GET:
            {
            pVXB_ACCESS_INT_VECTOR_GET  accessVectorGet;
            struct pciIntrEntry *   pInterruptInfo;

            accessVectorGet = (pVXB_ACCESS_INT_VECTOR_GET) pBusDevControl;
            pInterruptInfo = (struct pciIntrEntry *)(devID->pIntrInfo);

            /* check if the index is valid */

            if (accessVectorGet->intIndex >= 255)
                return ERROR;

            /* get the vector number */

            accessVectorGet->pIntVector =
                    INUM_TO_IVEC(pInterruptInfo->intVecInfo[accessVectorGet->intIndex].index);
            return(OK);
            }

        case VXB_ACCESS_INT_CONNECT:
            {
            pVXB_ACCESS_INTERRUPT_INFO  accessIntrInfo;
            struct pciIntrEntry *   pInterruptInfo;
            STATUS          retVal;

            /* get appropriate structures */

            accessIntrInfo = (pVXB_ACCESS_INTERRUPT_INFO) pBusDevControl;
            pInterruptInfo = (struct pciIntrEntry *)(devID->pIntrInfo);

            /* check if the index is valid */

            if (accessIntrInfo->intIndex >= 255)
                {
                return ERROR;
                }

            /* connect the interrupt */

            retVal = vxbPciIntConnect(devID,
                pInterruptInfo->intVecInfo[accessIntrInfo->intIndex].intVector,
                (VOIDFUNCPTR)accessIntrInfo->pISR,
                (int)accessIntrInfo->pParameter
                );

            return(retVal);
            }

        case VXB_ACCESS_INT_ENABLE:
            {
            pVXB_ACCESS_INTERRUPT_INFO  accessIntrInfo;
            struct pciIntrEntry *   pInterruptInfo;
            STATUS          retVal;

            accessIntrInfo = (pVXB_ACCESS_INTERRUPT_INFO) pBusDevControl;
            pInterruptInfo = (struct pciIntrEntry *)(devID->pIntrInfo);

            /* check if the index is valid */

            if (accessIntrInfo->intIndex >= 255)
                {
                return ERROR;
                }

            /* enable the interrupt */

            retVal = intEnable((int)pInterruptInfo->intVecInfo[accessIntrInfo->intIndex].intVector);

            retVal = OK;
            return(retVal);
            }

        case VXB_ACCESS_INT_DISABLE:
            {
            pVXB_ACCESS_INTERRUPT_INFO  accessIntrInfo;
            struct pciIntrEntry *   pInterruptInfo;
            STATUS          retVal;

            accessIntrInfo = (pVXB_ACCESS_INTERRUPT_INFO) pBusDevControl;
            pInterruptInfo = (struct pciIntrEntry *)(devID->pIntrInfo);

            /* check if the index is valid */

            if (accessIntrInfo->intIndex >= 255)
                {
                return ERROR;
                }

            /* disable the interrupt */

            retVal = intDisable((int)pInterruptInfo->intVecInfo[accessIntrInfo->intIndex].intVector);
            return(retVal);
            }

        case VXB_ACCESS_INT_ACKNOWLEDGE:
            return OK;

        case VXB_ACCESS_INT_DISCONN:
            return(ERROR);

        default:
            return ERROR;
        }
}

/*****************************************************************************
*
* armPlAxiPciConfigInfo - Returns the PCI config info ptr from bus controller.
*
* \NOMANUAL
*
* RETURNS: OK
*
* ERRNO: N/A
*/

LOCAL STATUS armPlAxiPciConfigInfo
    (
    VXB_DEVICE_ID pDev,
    char * pArgs
    )
    {
    VXB_ARMAXIPCI_DRV_CTRL * pDrvCtrl;

    pDrvCtrl = pDev->pDrvCtrl;
    *(UINT32*)pArgs = (UINT32)pDrvCtrl->pPciConfig;

    return (OK);
    }


/*****************************************************************************
*
* armPlAxiPciInterruptInfo - Returns the PCI interrupt info ptr from the bus
* controller.
*
* \NOMANUAL
*
* RETURNS: OK
*
* ERRNO: N/A
*/

LOCAL STATUS armPlAxiPciInterruptInfo
    (
    VXB_DEVICE_ID pDev,
    char * pArgs
    )
{
    VXB_ARMAXIPCI_DRV_CTRL * pDrvCtrl;

    pDrvCtrl = pDev->pDrvCtrl;
    *(UINT32*)pArgs = (UINT32)pDrvCtrl->pIntInfo;

    return (OK);
}


#if 1
/*
->vxbArmPlAxiPciRegister

-> pciConfigTopoShow 

[0,0,0] type=P2P BRIDGE to [1,0,0]
        base/limit:
          mem=   0x60000000/0x601fffff
          preMem=0x00000000/0x000fffff
          I/O=   0x00000000/0x00000fff
        status=0x0010 ( CAP DEVSEL=0 )
        command=0x0007 ( IO_ENABLE MEM_ENABLE MASTER_ENABLE )
[1,0,0] type=P2P BRIDGE to [2,0,0]
        base/limit:
          mem=   0x60100000/0x601fffff
          preMem=0xfff00000/0x000fffff
          I/O=   0x0000/0x0fff
        status=0x0010 ( CAP DEVSEL=0 )
        command=0x0007 ( IO_ENABLE MEM_ENABLE MASTER_ENABLE )
        bar0 in prefetchable 64-bit mem space @ 0x60000000
[2,15,0] type=OTHER DEVICE
        status=0x02a0 ( 66MHZ FBTB DEVSEL=1 )
        command=0x0007 ( IO_ENABLE MEM_ENABLE MASTER_ENABLE )
        bar0 in 32-bit mem space @ 0x60100000
        bar1 in prefetchable 32-bit mem space @ 0x60102000

-> pcie_show

[0,0,0] - (10ee 7112) type=P2P BRIDGE to [1,0,0]
        base/limit:
          mem=   0x60000000/0x601fffff
          preMem=0x00000000/0x000fffff
          I/O=   0x00000000/0x00000fff
        status=0x0010 ( CAP DEVSEL=0 )
        command=0x0007 ( IO_ENABLE MEM_ENABLE MASTER_ENABLE )
[1,0,0] - (10b5 8112) type=P2P BRIDGE to [2,0,0]
        base/limit:
          mem=   0x60100000/0x601fffff
          preMem=0xfff00000/0x000fffff
          I/O=   0x0000/0x0fff
        status=0x0010 ( CAP DEVSEL=0 )
        command=0x0007 ( IO_ENABLE MEM_ENABLE MASTER_ENABLE )
        bar0 in prefetchable 64-bit mem space @ 0x60000000
[2,15,0] - (1057 0004) type=OTHER DEVICE
        status=0x02a0 ( 66MHZ FBTB DEVSEL=1 )
        command=0x0007 ( IO_ENABLE MEM_ENABLE MASTER_ENABLE )
        bar0 in 32-bit mem space @ 0x60100000
        bar1 in prefetchable 32-bit mem space @ 0x60102000
value = 4349904 = 0x425fd0 = g_pcie_show_flag
*/

#endif
	
