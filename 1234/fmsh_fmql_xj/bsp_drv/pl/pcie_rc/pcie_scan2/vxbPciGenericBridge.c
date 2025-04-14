/* vxbPciGenericBridge.c - PCI bridge driver */

/*
 * Copyright (c) 2014, 2015 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
30dec15,y_y  add support for PCIE AER capability. (F4655)
01oct15,geo  Add support for MP table described non-root bus devices
21jul15,wap  When checking for an early match in vxbPcibIntAssign(),
             use the bus/dev/func of the child device, not its parent.
15jun15,wap  Add VxBus ioctl method (F3973)
10jun15,wap  Remove method that's not implemented yet
08jun15,wap  Code cleanups, PCI swizzle re-work
03jul14,y_y  Created
*/

/*
DESCRIPTION
This library contains the generic PCI bridge driver

*/

#include <vxWorks.h>
#include <msgQLib.h>
#include <hwif/vxBus.h>
#include <hwif/buslib/pciDefines.h>
#include <hwif/buslib/vxbPciLib.h>
#ifdef _WRS_CONFIG_PCIE_AER
#include <hwif/buslib/vxbPcieAer.h>
#endif  /* _WRS_CONFIG_PCIE_AER */ 

/* Debug macro */

#undef  PCI_BGE_DBG
#ifdef  PCI_BGE_DBG

#include <private/kwriteLibP.h>

LOCAL int debugLevel = 0;

#ifdef  LOCAL
#undef  LOCAL
#define LOCAL
#endif

#define PCI_BGE_MSG(lvl, fmt, args...)        \
    do {                                      \
        if (debugLevel >= lvl)                \
            {                                 \
            if (_func_kprintf != NULL)        \
                _func_kprintf (fmt, ##args);  \
            }                                 \
       } while (FALSE)
#else
#define PCI_BGE_MSG(lvl, fmt, args...)

#endif  /* PCI_DEBUG */

/* defines */

#define MATCH_LEVEL        0x10000

LOCAL STATUS vxbPcibProbe (VXB_DEV_ID pDev);
LOCAL STATUS vxbPcibAttach (VXB_DEV_ID pDev);
LOCAL STATUS vxbPcibCfgRead (VXB_DEV_ID, PCI_HARDWARE*, UINT32, UINT32, void*);
LOCAL STATUS vxbPcibCfgWrite (VXB_DEV_ID, PCI_HARDWARE*, UINT32, UINT32, UINT32);
LOCAL STATUS vxbPcibResFree (VXB_DEV_ID, VXB_DEV_ID, VXB_RESOURCE *);
LOCAL VXB_RESOURCE * vxbPcibResAlloc (VXB_DEV_ID, VXB_DEV_ID, UINT32);
LOCAL VXB_RESOURCE_LIST * vxbPcibResListGet (VXB_DEV_ID pDev, VXB_DEV_ID pChild);
LOCAL STATUS vxbPcibIntAssign (VXB_DEV_ID pDev, PCI_HARDWARE * pciDev, int pin,
    UINT8 * irq, VXB_INTR_ENTRY * pIntrEntry);

#ifdef _WRS_CONFIG_PCIE_AER
#define MISC_CTRL_STS_REG       0x40
#define ENABLE_SYS_ERR_FOR_AER  0x10
#define INTEL                   0x8086
LOCAL STATUS vxbPcibErrRecovery (VXB_DEV_ID pDev);
LOCAL void vxbPcieAerRpRegSet (VXB_DEV_ID pDev);
#endif

/* locals */

LOCAL VXB_DRV_METHOD vxbPcibMethods[] =
    {
    /* VxBus API */

    { VXB_DEVMETHOD_CALL(vxbDevProbe),        vxbPcibProbe},
    { VXB_DEVMETHOD_CALL(vxbDevAttach),       vxbPcibAttach},
    { VXB_DEVMETHOD_CALL(vxbDevShutdown),     vxbPciBusShutdown},
    { VXB_DEVMETHOD_CALL(vxbDevDetach),       vxbPciBusDetach},
    { VXB_DEVMETHOD_CALL(vxbDevIoctl),        vxbPciBusIoctl},

    /* PCI API */

    { VXB_DEVMETHOD_CALL(vxbPciCfgRead),      vxbPcibCfgRead},
    { VXB_DEVMETHOD_CALL(vxbPciCfgWrite),     vxbPcibCfgWrite},
    { VXB_DEVMETHOD_CALL(vxbPciIntAssign),    vxbPcibIntAssign},

    /* Resource API */

    { VXB_DEVMETHOD_CALL(vxbResourceFree),    vxbPcibResFree},
    { VXB_DEVMETHOD_CALL(vxbResourceAlloc),   (FUNCPTR)vxbPcibResAlloc},
    { VXB_DEVMETHOD_CALL(vxbResourceListGet), (FUNCPTR)vxbPcibResListGet},
    
    /* Pci Express Port Error Recover API */

#ifdef _WRS_CONFIG_PCIE_AER
    { VXB_DEVMETHOD_CALL(vxbPcieErrRecovery), (FUNCPTR)vxbPcibErrRecovery},
#endif

    VXB_DEVMETHOD_END
    };

VXB_DRV vxbPcibDrv =
    {
    { NULL } ,
    "pcib",                 /* drvName */
    "PCI generic bridge driver",    /* Description */
    VXB_BUSID_PCI,          /* Class */
    0,                      /* Flags */
    0,                      /* Reference count */
    &vxbPcibMethods[0]      /* Method table */
    };

VXB_DRV_DEF(vxbPcibDrv);

/*********************************************************************
*
* vxbPcibProbe - vxBus probe handler
*
* This routine returns a big integer value to show the Bridge driver
* is the generic one, if no more proper Bridge driver is found, just use
* this one.
*
* RETURNS: a big integer value
*
* ERRNO: N/A
*/

LOCAL STATUS vxbPcibProbe
    (
    VXB_DEV_ID pDev
    )
    {
    UINT8 headerType;

    VXB_PCI_BUS_CFG_READ (pDev, PCI_CFG_HEADER_TYPE, 1, headerType);

    /*
     * Test the header type byte for a PCI-PCI bridge or cardbus
     * bridge header format. PCIe switches also fall under the
     * PCI-PCI bridge header type.
     */

    headerType &= PCI_HEADER_TYPE_MASK;

    if ((headerType == PCI_HEADER_TYPE_BRIDGE) ||
        (headerType == PCI_HEADER_PCI_CARDBUS))
        return (MATCH_LEVEL);

    return (ERROR);
    }

/*******************************************************************************
*
* vxbPcibAttach - attach gentric PCI bridge device
*
* This routine attach PCI bridge PCI bridge  device.
*
* RETURNS: OK
*
* ERRNO: N/A
*/

LOCAL STATUS vxbPcibAttach
    (
    VXB_DEV_ID pDev
    )
    {
    PCI_ROOT_CHAIN * pPciRootChain = NULL;
    UINT8 bus = 0;
#ifdef _WRS_CONFIG_PCIE_AER
    PCI_HARDWARE * pIvars = vxbDevIvarsGet (pDev);
#endif
    /*
     * Allocate our softc context. This is used mainly
     * to store information about the root complex or host bridge
     * that is the root of the PCI device tree of which we are
     * a member.
     */

    pPciRootChain = (PCI_ROOT_CHAIN *) vxbMemAlloc (sizeof(PCI_ROOT_CHAIN));

    if (pPciRootChain == NULL)
        return ERROR;

    vxbDevSoftcSet (pDev, (void *)pPciRootChain);

    VXB_PCI_BUS_CFG_READ(pDev, PCI_CFG_SECONDARY_BUS, 1, bus);

    if (bus == 0)
        return (OK);

    /* PCI-e Root Port AER capability init if exits */
    
#ifdef _WRS_CONFIG_PCIE_AER
    vxbRpAerRegister (pDev);

    /* Intel Xeon Processor D-1500 special register set */

    if ((vxbPcieDevTypeGet(pDev) == PCI_EXP_TYPE_ROOT_PORT) && 
        (pIvars->pciVendId == INTEL))
        vxbPcieAerRpRegSet(pDev);
#endif

    /* Probe for child devices on our secondary bus. */

    if (vxbPciBusAddDev (pDev, bus) != OK)
        {
        vxbDevSoftcSet (pDev, NULL);
        vxbMemFree (pPciRootChain);
        return (ERROR);
        }

    /* PCI-e Root Port AER capability INT enable if exits */

#ifdef _WRS_CONFIG_PCIE_AER
    vxbPcieAerRpIntEnable (pDev);
#endif

    return (OK);
    }

/******************************************************************************
*
* vxbPcibCfgRead - VxBus PCI config read routine
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

LOCAL STATUS vxbPcibCfgRead
    (
    VXB_DEV_ID     pDev,        /* device info */
    PCI_HARDWARE * pPciDev,     /* PCI device info */
    UINT32         byteOffset,  /* offset into cfg space */
    UINT32         width,       /* transaction size, in bytes */
    void *         pData        /* buffer to read-from/write-to */
    )
    {
    return VXB_PCI_CFG_READ (vxbDevParent(pDev), pPciDev,
        byteOffset, width, pData);
    }

/******************************************************************************
*
* vxbPcibCfgWrite - VxBus PCI config write method
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

LOCAL STATUS vxbPcibCfgWrite
    (
    VXB_DEV_ID     pDev,        /* device info */
    PCI_HARDWARE * pPciDev,     /* PCI device info */
    UINT32         byteOffset,  /* offset into cfg space */
    UINT32         width,       /* transaction size, in bytes */
    UINT32         data         /* data write to the offset */
    )
    {
    return VXB_PCI_CFG_WRITE(vxbDevParent(pDev), pPciDev,
        byteOffset, width, (UINT32) data);
    }

/******************************************************************************
*
* vxbPcibIntAssign - VxBus PCI interrupt assignment method
*
* This function assigns a legacy INTx interrupt for a PCI device, based on its
* interrupt pin number.
*
* RETURNS: OK, or ERROR if assignment failed
*
* ERRNO: N/A
*/

LOCAL STATUS vxbPcibIntAssign
    (
    VXB_DEV_ID           pDev,
    PCI_HARDWARE *       pciDev,
    int                  pin,
    UINT8 *              irq,
    VXB_INTR_ENTRY   *   pIntrEntry
    )
    {
    PCI_HARDWARE * pIvars;
    PCI_ROOT_CHAIN * pSoftc;
    STATUS r;

    pIvars = vxbDevIvarsGet (pDev);
    pSoftc = vxbDevSoftcGet (pDev);

    /*
     * To support MP table described devices beyond the root bridge
     * bus (bus 0), we add this lookup prior to pin swizzling.
     */
    
    r = VXB_PCI_INT_ASSIGN (pSoftc->pRootDev, pciDev,
            pin, irq, pIntrEntry);
    
    if (r == OK)
    	return OK;
    
    /* Apply the PCI bridge swizzle to the pin number. */

    pin = (UINT8)((((pin - 1) + pciDev->pciDev) % 4) + 1);

    /*
     * Test to see if the root device has a mapping for the specified
     * pin at the current bridge level. This will probably only succeed
     * for the Intel platform, where there's a PCIe<->PCI bridge present.
     * In general, we want to climb up each level of bridging, applying
     * the interrupt swizzle transformation each time, until we reach
     * the root slot. In theory, only that slot will be able to provide
     * a translation between the INTA/B/C/D pin number and a host IRQ
     * number. However with PCIe, it's possible that an intermediate
     * node in the traversal will be able to provide the translation
     * instead. This is possible because PCIe uses in-band messaging
     * to emulate legacy INTx interrupts. So rather than keep going
     * until we reach the root, the logic here is to keep going until
     * we reach the first node that can successfully provide an
     * interrupt mapping.
     */

    r = VXB_PCI_INT_ASSIGN (pSoftc->pRootDev, pciDev,
            pin, irq, pIntrEntry);

    if (r == OK)
        return (OK);

    r = VXB_PCI_INT_ASSIGN (vxbDevParent (pDev), pIvars,
            pin, irq, pIntrEntry);

    return (r);
    }

/*******************************************************************************
*
* vxbPcibResAlloc - allocate resource for child device
*
* This routine allocate resource for child device and go corresponding operation
* based on its type.
*
* \NOMANUAL
*
* RETURNS: OK if allocate successfully, otherwise ERROR
*
* ERRNO: N/A
*/

LOCAL VXB_RESOURCE *  vxbPcibResAlloc
    (
    VXB_DEV_ID pDev,
    VXB_DEV_ID pChild,
    UINT32 id
    )
    {
    return VXB_RESOURCE_ALLOC(vxbDevParent(pDev), pChild, id);
    }

/*******************************************************************************
*
* vxbPcibResFree - free the resource for child device
*
* This routine free the resource allocated by calling fslPciResAlloc
*
* RETURNS: OK if free successfully, otherwise ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbPcibResFree
    (
    VXB_DEV_ID pDev,
    VXB_DEV_ID pChild,
    VXB_RESOURCE * pRes
    )
    {
    return VXB_RESOURCE_FREE(vxbDevParent(pDev), pChild, pRes);
    }

/*******************************************************************************
*
* vxbPcibResListGet - get the resource list of specific device
*
* This routine get the resource list of specific device
*
* RETURNS: resource list pointer
*
* ERRNO: N/A
*/

LOCAL VXB_RESOURCE_LIST * vxbPcibResListGet
    (
    VXB_DEV_ID pDev,
    VXB_DEV_ID pChild
    )
    {
    PCI_HARDWARE * pPciDev = (PCI_HARDWARE *)vxbDevIvarsGet(pChild);

    if (pPciDev == NULL)
        {
        return (NULL);
        }

    return &pPciDev->vxbResList;
    }

#ifdef _WRS_CONFIG_PCIE_AER
/*******************************************************************************
*
* vxbPcibErrRecovery - Pci Express Port Error Recover
*
* This routine performs Pci Express Port Error Recover.
*
* RETURNS: Returns OK on success, or ERROR on failure.
*
* ERRNO
*/
    
LOCAL STATUS vxbPcibErrRecovery
    (
    VXB_DEV_ID pDev
    )
    {
    /* do nothing */
    return OK;
    }

/******************************************************************************
*
* vxbPcieAerRpRegSet - other error regs set for Root Port
*
* This routine sets some special Regs for Root Port
*
* RETURNS: Returns OK on success, or ERROR on failure.
*
* ERRNO
*
*/

LOCAL void vxbPcieAerRpRegSet
    (
    VXB_DEV_ID pDev
    )
    {
    UINT32 reg32 = 0;
    UINT16 rpAerOffset = 0;

    /* 
     * IIO configuration register misc control and status register 0:
     * (xeon-d-1500-datasheet-vol-2,chapter 5.2.84)
     * Applies only to root port.
     * 
     * When this bit is set, the PCI Express errors
     * do not trigger an MSI or Intx interrupt,regardless of the whether MSI or
     * INTx is enabled or not.
     * When this bit is clear, PCI Express errors are reported via MSI or INTx.
     *
     * Note: this register does not define in PCI Express base spec. However it
     * has to take care when BIOS/UEFI doesn't config a right value, otherwise
     * PCI Express errors do not trigger any interrupt or other error comes.
     */

    (void) vxbPcieExtCapFind (pDev, PCIE_EXT_CAP_ID_ERR, &rpAerOffset);

    VXB_PCI_BUS_CFG_READ(pDev, rpAerOffset+MISC_CTRL_STS_REG, 4, reg32);

    if (reg32 | ENABLE_SYS_ERR_FOR_AER)
        {
        reg32 = reg32 & ~ENABLE_SYS_ERR_FOR_AER;
        VXB_PCI_BUS_CFG_WRITE(pDev, rpAerOffset+MISC_CTRL_STS_REG, 4, reg32);
        }

    return;
    }
#endif

