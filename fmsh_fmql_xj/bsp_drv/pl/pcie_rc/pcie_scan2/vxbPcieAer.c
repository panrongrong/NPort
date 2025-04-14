/* vxbPcieAer.c - PCI EXPRESS Root Port Advanced Error Reporting driver */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
17dec15,y_y  Created. (F4655)
*/

/*
 * DESCRIPTION
 * 
 * This file implements the PCI Express AER service driver. 
 * 
 * PCI Express defines the AER capability, which is implemented with the PCI
 * Express AER Extended Capability Structure, to allow a PCI Express agent to
 * send an error reporting message to the Root Port. The Root Port, a host
 * receiver of all error messages associated with its hierarchy, decodes an 
 * error message into error type and an agent ID and the logs these into its
 * PCI Express AER Extended Capability Structure. Depending on whether an
 * error reporting message is enabled in the Root Error Command Register.
 * Once a AER error occured, the AER driver ISR function first decodes an error 
 * message received at PCI Express Root Port into an error type and agent ID.
 * Then, find whether agent belongs to its hierarchy. If agent exists, ISR
 * would also use decoded error information to read the PCI Express AER
 * capability of the agent device to obtain more detailed about an error
 * and clear the corresponding bit in the error status register of both
 * PCI Express Root Port and the agent device. Meanwhile, console the output
 * to EDR system. 
 * 
 * To enable it, component INCLUDE_PCIE_AER and DRV_PCI_BRIDGE_GENERIC need
 * to add into VxWorks kernel image.
 * 
 * User could call the edrShow() routine to display the contents of the ED&R
 * error log of AER. To enable it, component INCLUDE_EDR_SHOW  need to add into
 * VxWorks kernel image.
 *
 * EXAMPLE
 * The following example shows output from edrShow(). It displays two records,
 * the first for a boot event with a severity level of INFO (EDR_SEVERITY_INFO)
 * and the second for a AER Unsupported Request Error event that resulted from a
 * task("tIsr0") by accessing non-exist device function. As shown, the information
 * stored and displayed in a record depends on the type of event.
 * \cs
 * -> edrShow
 * ERROR LOG
 * =========
 * Log Size:         32768 bytes (8 pages)
 * Record Size:      4096 bytes
 * Max Records:      7
 * CPU Type:         0xc7
 * Errors Missed:    0 (old) + 0 (recent)
 * Error count:      2
 * Boot count:       1
 * Generation count: 1
 * 
 * ==[1/2]==============================================================
 * Severity/Facility:   INFO/BOOT
 * Boot Cycle:          1
 * OS Version:          7.0.0
 * Time:                THU JAN 01 00:00:00 1970 (ticks = 0)
 * Task:                "tRootTask" (0xffffffff827d2680)
 * Injection Point:     edrStub.c:226
 * 
 * System Booted - cold boot
 * 
 * ==[2/2]==============================================================
 * Severity/Facility:   WARNING/INTERRUPT
 * Boot Cycle:          1
 * OS Version:          7.0.0
 * Time:                THU JAN 01 00:03:47 1970 (ticks = 13678)
 * Task:                "tIsr0" (0xffff800000027700)
 * Injection Point:     vxbPcieAer.c:971
 * 
 * <<<<<PCIE Advanced Error Reporting >>>>>
 * 
 * ROOT PORT
 * =========
 * BDF:              0-1-0
 * Device ID:        0x6f02
 * Vendor ID:        0x8086
 * Requester ID:     0x0000
 * Error Messages:   0x24
 *                   Fatal/Non-fatal Error Message Received
 *                   Non-Fatal Uncorrectable Error Message Received
 * END PORT
 * =========
 * BDF:              1-0-0
 * Device ID:        0x105e
 * Vendor ID:        0x8086
 * TLP Header:       00000000 0008200f 01070000 00000000
 * Error Type:       Unsupported Request Error
 * 
 * 
 * <<<<<Memory Map>>>>>
 * 
 * 0xffffffff8040e000 -> 0xffffffff80734420: kernel
 * 
 * <<<<<Registers>>>>>
 * 
 * rax        = 0x0000000000000000   r8         = 0x0000000000000000
 * rbx        = 0xffffffff8067270b   r9         = 0x0000000000000000
 * rcx        = 0x0000000000000000   r10        = 0x0000000000000000
 * rdx        = 0xffffffff80747e19   r11        = 0x00000000ffffffff
 * rsi        = 0xffffffff806a7806   r12        = 0xffff800000025480
 * rdi        = 0xffff800000027530   r13        = 0x0000000000000000
 * rsp        = 0xffff800000027520   r14        = 0x000000000008200f
 * rbp        = 0xffff800000027610   r15        = 0x0000000001070000
 * pc         = 0xffffffff805c1fc0   eflags     = 0x0000000000000246
 * fs         = 0xffff800000027610
 * 
 * <<<<<Traceback>>>>>
 * 
 * 0xffffffff805c3664 VXB_RESOURCE_LIST_GET+0x284: vxbPcieAerInt ()
 * 0xffffffff805c253f vxbPcieAerInt+0x8f : 0xffffffff805c1ee3 ()
 * value = 0 = 0x0
 * 
 * \ce
 *
 * Note: The AER driver verified on Intel Camelback Mountain Customer
 * Reference Board (CRB) with Broadwell DE CPU. User might easily trigger 
 * Unsupported Request Error via accessing non-exist device function.
 *
 * EXAMPLE
 * \cs
 * ->pciConfigOutLong (bus,device,function,0,0); //access non-exist device function
 * \ce
 */

/* includes */

#include <vxWorks.h>
#include <stdio.h>
#include <string.h>
#include <hwif/vxBus.h>
#include <hwif/buslib/pciDefines.h>
#include <hwif/buslib/vxbPciLib.h>
#include <edrLib.h>
#include <subsys/timer/vxbTimerLib.h>
#include <hwif/buslib/vxbPcieAer.h>
#include <hwif/buslib/vxbPciMsi.h> 

/* Debug macro */

#undef  AER_DEBUG
#ifdef  AER_DEBUG

#include <private/kwriteLibP.h>

LOCAL int debugLevel = 1000;

#ifdef  LOCAL
#undef  LOCAL
#define LOCAL
#endif

#define AER_MSG(lvl, fmt, args...)                      \
    do {                                                \
        if (debugLevel >= lvl)                          \
            {                                           \
            if (_func_kprintf != NULL)                  \
                (void) _func_kprintf("DBG %s[%d]: "fmt, \
                __FUNCTION__, __LINE__, ## args);       \
            }                                           \
       } while (FALSE)
#else
#define AER_MSG(lvl, fmt, args...)

#endif  /* AER_DEBUG */

/* locals */

LOCAL STATUS vxbPcieAerInt (VXB_DEV_ID pDev);
LOCAL void vxbPcieAerIsr (VXB_DEV_ID pDev);

/*******************************************************************************
*
* vxbPcieAerInit - initialization routine for PCI Express AER library
*
* This routine initializes the PCI  Express AER  library. Currently it's
* just a stub provided as an init hook for the PCI component definition file.
*
* RETURNS: always OK
*
* ERRNO: N/A
*/

void vxbPcieAerInit (void)
    {
    return;
    }

/*******************************************************************************
*
* vxbRpAerRegister - registers PCI Express Root Port AER into system
*
* This routine registers PCI Express Root Port AER capability into system, 
* meanwhile enable all Root Ports AER capabilities.
*
* RETURNS: null
*
* ERRNO: N/A
*/

void vxbRpAerRegister
    (
    VXB_DEV_ID pDev
    )
    {
    UINT16 rpAerOffset = 0;
    PCI_ROOT_CHAIN * pcieRp = NULL;
    AER_INFO * pcieAerInfo;
    VXB_RESOURCE * pResInt;

    if (vxbPcieExtCapFind (pDev, PCIE_EXT_CAP_ID_ERR, &rpAerOffset) != OK)
        {
        AER_MSG(100, "%s:%d(%p) does not support AER, register failed.\n",
             vxbDevNameGet (pDev), vxbDevUnitGet (pDev), pDev);
        return;
        }

    if (rpAerOffset)
        {
        /* connect and enable AER interrupt only on Root Port */

        if (vxbPcieDevTypeGet(pDev) == PCI_EXP_TYPE_ROOT_PORT)
            {
            pcieRp = (PCI_ROOT_CHAIN *) vxbDevSoftcGet (pDev);

            if (pcieRp == NULL)
                return;

            /*
             * Allocate interrupt resource. For MSI/MSI-X resources, the
             * index starts at 1. Index 0 is always the INTx interrupt.
             */

            if (vxbPciMsiXAlloc(pDev, 1) < 0)
                {
                if (vxbPciMsiAlloc(pDev, 1) < 0)
                    pResInt = vxbResourceAlloc (pDev, VXB_RES_IRQ, 0);
                else
                    pResInt = vxbResourceAlloc (pDev, VXB_RES_IRQ, 1);
                }
            else
                {
                pResInt = vxbResourceAlloc (pDev, VXB_RES_IRQ, 1);
                }

            pcieRp->intRes = pResInt;

            (void) vxbIntConnect(pDev, pcieRp->intRes, 
                                 (VOIDFUNCPTR) vxbPcieAerIsr, pDev);

            (void) vxbIntEnable (pDev, pcieRp->intRes);

            /* set the pointer of the queue for deferred work */

            pcieRp->queueId = isrDeferQueueGet (pDev, 0, 0, 0);

            if (pcieRp->queueId != NULL)
                {
                pcieRp->isrDef.func = (void *)vxbPcieAerInt;
                pcieRp->isrDef.pData = pDev;
                }
            else
                {
                AER_MSG(100, "%p post to ISR deferred queue failed.\n", pDev);
                return;
                }

            pcieRp->rpAerSem = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE |
                                      SEM_INVERSION_SAFE);

            if (pcieRp->rpAerSem == NULL)
                {
                AER_MSG (0, "semMCreate failed for rpAerSem\n");
                return;
                }

            /* allocate AER structure */

            pcieAerInfo = (AER_INFO *) vxbMemAlloc (sizeof(AER_INFO));

            if (pcieAerInfo == NULL)
                return;

            pcieRp->aerInfo = pcieAerInfo;

            pcieRp->aerInfo->aerOffset = rpAerOffset;

            }

        /* enable error message send on all ports */

        if (vxbPcieAerCapEnable(pDev) != OK)
            return;
        }

    return;
    }

/******************************************************************************
*
* vxbPcieAerRpClear - Root Port error status clean
*
* This routine clears Root Port's error status
*
* RETURNS: Returns OK on success, or ERROR on failure.
*
* ERRNO
*
*/

LOCAL STATUS vxbPcieAerRpClear
    (
    VXB_DEV_ID pDev,
    UINT16     aerOffset
    )
    {
    UINT16 reg16 = 0;
    UINT32 reg32 = 0;
    UINT8  pcieCapOffset = 0;

    /* Clear PCIe Capability's Device Status */

    (void) vxbPciExtCapFind (pDev, PCI_EXT_CAP_EXP, &pcieCapOffset);

    if (!pcieCapOffset)
        return ERROR;

    /* Disable system error generation*/

    pcieCapOffset = (UINT8)(pcieCapOffset - 2);

    VXB_PCI_BUS_CFG_READ(pDev, pcieCapOffset + PCI_EXP_RTCTL_REG, 2, reg16);

    reg16 &= (UINT16) ~(PCI_EXP_RTCTL_SECE_ENB |
                        PCI_EXP_RTCTL_SEFE_ENB |
                        PCI_EXP_RTCTL_SEFE_ENB);

    VXB_PCI_BUS_CFG_WRITE(pDev, pcieCapOffset + PCI_EXP_RTCTL_REG, 2, reg16);

    /* Clear Correctable/Non-Fatal/Fatal/Unsupported Request Error status */

    VXB_PCI_BUS_CFG_READ(pDev, pcieCapOffset + PCI_EXP_DEVSTA_REG, 2, reg16);

    VXB_PCI_BUS_CFG_WRITE(pDev, pcieCapOffset + PCI_EXP_DEVSTA_REG, 2, reg16);

    /* Clear AER error status */

    VXB_PCI_BUS_CFG_READ(pDev, aerOffset + PCIE_AER_ROOT_ERR_STATUS, 4, reg32);

    if (reg32)
        VXB_PCI_BUS_CFG_WRITE(pDev, aerOffset + PCIE_AER_ROOT_ERR_STATUS, 4, reg32);

    VXB_PCI_BUS_CFG_READ(pDev, aerOffset + PCIE_AER_CRERR_STATUS, 4, reg32);

    if (reg32)
        VXB_PCI_BUS_CFG_WRITE(pDev, aerOffset + PCIE_AER_CRERR_STATUS, 4, reg32);

    VXB_PCI_BUS_CFG_READ(pDev, aerOffset + PCIE_AER_UNCRERR_STATUS, 4, reg32);

    if (reg32)
        VXB_PCI_BUS_CFG_WRITE(pDev, aerOffset + PCIE_AER_UNCRERR_STATUS, 4, reg32);

    /* Enable system error generation*/

    VXB_PCI_BUS_CFG_READ(pDev, pcieCapOffset + PCI_EXP_RTCTL_REG, 2, reg16);

    reg16 |= (UINT16) (PCI_EXP_RTCTL_SECE_ENB |
                       PCI_EXP_RTCTL_SEFE_ENB |
                       PCI_EXP_RTCTL_SEFE_ENB);

    VXB_PCI_BUS_CFG_WRITE(pDev, pcieCapOffset + PCI_EXP_RTCTL_REG, 2, reg16);

    return OK;
    }

/******************************************************************************
*
* vxbPcieAerCapEnable - enable error message send 
*
* This routine enables PCIe device sending ERR_COR/NONFATAL/FATAL messages.
*
* RETURNS: Returns OK on success, or ERROR on failure.
*
* ERRNO
*
*/

STATUS vxbPcieAerCapEnable
    (
    VXB_DEV_ID pDev
    )
    {
    int type = 0;
    UINT16 val = 0;
    UINT8 pcieCapOffset = 0;
    UINT16 aerOffset = 0;

    if (vxbPcieExtCapFind (pDev, PCIE_EXT_CAP_ID_ERR, &aerOffset) != OK)
        {
        AER_MSG(100, "%p does not support AER, register failed.\n", pDev);
        return ERROR;
        }

    if ((type = vxbPcieDevTypeGet(pDev)) == ERROR)
        return ERROR;

    if (type == PCI_EXP_TYPE_ROOT_PORT)
        {
        if (vxbPcieAerRpClear(pDev, aerOffset) != OK)
            return ERROR;

        /* Enable reporting/forwarding of parity, and SERR. */

        VXB_PCI_BUS_CFG_READ(pDev, PCI_CFG_BRIDGE_CONTROL , 2, val);

        val |= (PCI_CFG_PARITY_ERROR | PCI_CFG_SERR);

        VXB_PCI_BUS_CFG_WRITE(pDev, PCI_CFG_BRIDGE_CONTROL, 2, val); 
        }

    if ((type == PCI_EXP_TYPE_ROOT_PORT)  ||
        (type == PCI_EXP_TYPE_UPSTREAM)   ||
        (type == PCI_EXP_TYPE_DOWNSTREAM) ||
        (type == PCI_EXP_TYPE_ENDPOINT)) 
        {
        /* Enable reporting System errors and parity errors on all devices */

        VXB_PCI_BUS_CFG_READ(pDev, PCI_CFG_COMMAND, 2, val);

        val |= PCI_CMD_SERR_ENABLE | PCI_CMD_PERR_ENABLE;

        VXB_PCI_BUS_CFG_WRITE(pDev, PCI_CFG_COMMAND, 2, val);

        (void) vxbPciExtCapFind (pDev, PCI_EXT_CAP_EXP, &pcieCapOffset);

        if (!pcieCapOffset)
            return ERROR;

        pcieCapOffset = (UINT8)(pcieCapOffset - 2);

        VXB_PCI_BUS_CFG_READ(pDev, pcieCapOffset + PCI_EXP_DEVCTL_REG, 2, val);

        /* Enable Correctable/Non-Fatal/Fatal/Unsupported Request Error Reporting */

        val |= PCI_EXP_DEVCTL_CERR_ENB  |
               PCI_EXP_DEVCTL_NFERR_ENB |
               PCI_EXP_DEVCTL_FERR_ENB  |
               PCI_EXP_DEVCTL_URREP_ENB;

        VXB_PCI_BUS_CFG_WRITE(pDev, pcieCapOffset + PCI_EXP_DEVCTL_REG, 2, val);
        }

    return OK;
    }

/******************************************************************************
*
* vxbPcieAerRpIrqEnable - enable Root Port interrupt 
*
* This routine enables the generation of an interrupt when a error is reported
* by any of the functions in the hierarchy associated with this Root Port.
*
* RETURNS: null
*
* ERRNO
*
*/

void vxbPcieAerRpIntEnable
    (
    VXB_DEV_ID pDev
    )
    {
    UINT32 reg32 = 0;
    UINT16 aerOffset = 0;
    PCI_ROOT_CHAIN * pcieRp = (PCI_ROOT_CHAIN *) vxbDevSoftcGet (pDev);

    if (pcieRp->aerInfo)
        aerOffset = pcieRp->aerInfo->aerOffset;
    else
        return;

    /* enable Root Port interrupt */

    if (vxbPcieDevTypeGet(pDev) == PCI_EXP_TYPE_ROOT_PORT)
        {
        VXB_PCI_BUS_CFG_READ(pDev, aerOffset + PCIE_AER_ROOT_ERR_COMMAND, 4, reg32);

        reg32 |= (PCI_AER_ROOT_CMD_COR_EN |
                  PCI_AER_ROOT_CMD_NONFATAL_EN |
                  PCI_AER_ROOT_CMD_FATAL_EN);

        VXB_PCI_BUS_CFG_WRITE(pDev, aerOffset + PCIE_AER_ROOT_ERR_COMMAND, 4, reg32);
        }

    return;
    }

/******************************************************************************
*
* vxbPcieAerAgentMatch - Agent match function
*
* This routine finds whether error agent exits in the bus tree by
* matching ID reported to Root Port or uncorrectable/correctable error status.
*
* RETURNS: Returns agent ID on success, or NULL on failure..
*
* ERRNO
*
*/

LOCAL VXB_DEV_ID vxbPcieAerAgentMatch
    (
    VXB_DEV_ID     pDev,
    PCI_HARDWARE * pIvars,
    AER_INFO *     aerInfo
    )
    {
    UINT16 val=0;
    UINT16 aerOffset=0;
    UINT32 status = 0;
    UINT32 mask = 0;
    UINT8  pcieCapOffset = 0;

    /* Error Source ID match? */

    if (aerInfo->id && PCI_BUS_GET(aerInfo->id))
        {
        if ((ERR_COR_SOURCE_ID(aerInfo->id) == 
            PCIE_BDF_ID(pIvars->pciBus, pIvars->pciDev, pIvars->pciFunc)) ||
            (ERR_UNCOR_SOURCE_ID(aerInfo->id)== 
            PCIE_BDF_ID(pIvars->pciBus, pIvars->pciDev, pIvars->pciFunc)))
            {
            AER_MSG(100, "Find Agent 0x%p [BDF-%x%x%x] ID[0x%x]\n",pDev,
                pIvars->pciBus, pIvars->pciDev,pIvars->pciFunc,aerInfo->id);

            return pDev;
            }
        }

    /* correctable/uncorrectable error status match? */
    /* Check if AER is enabled */

    (void) vxbPciExtCapFind (pDev, PCI_EXT_CAP_EXP, &pcieCapOffset);

    if (!pcieCapOffset)
        return NULL;

    pcieCapOffset = (UINT8)(pcieCapOffset - 2);

    VXB_PCI_BUS_CFG_READ(pDev, pcieCapOffset + PCI_EXP_DEVCTL_REG, 2, val);

    /* Check if Error Bit Enable in Device Control Register */  

    val &= PCI_EXP_DEVCTL_CERR_ENB  | PCI_EXP_DEVCTL_NFERR_ENB |
           PCI_EXP_DEVCTL_FERR_ENB  | PCI_EXP_DEVCTL_URREP_ENB;

    if (!val)
        return NULL;

    (void)vxbPcieExtCapFind (pDev, PCIE_EXT_CAP_ID_ERR, &aerOffset);

    if (!aerOffset)
        return NULL;

    /* Check error status and mask */

    if (aerInfo->rootSts & PCI_AER_ROOT_COR_RCV) 
        {
        VXB_PCI_BUS_CFG_READ(pDev, aerOffset + PCIE_AER_CRERR_STATUS, 4, status);
        VXB_PCI_BUS_CFG_READ(pDev, aerOffset + PCIE_AER_CRERR_MASK, 4, mask);
        }
    else 
        {
        VXB_PCI_BUS_CFG_READ(pDev, aerOffset + PCIE_AER_UNCRERR_STATUS, 4, status);
        VXB_PCI_BUS_CFG_READ(pDev, aerOffset + PCIE_AER_UNCRERR_MASK, 4, mask);
        }

    if (status & ~mask)
        {
        AER_MSG(100, "Match Agent[%s%d]-%p, BDF[%x%x%x] ID[0x%x] status/mask[0x%x/0x%x]\n",
            vxbDevNameGet (pDev), vxbDevUnitGet (pDev), pDev, pIvars->pciBus,
            pIvars->pciDev, pIvars->pciFunc, aerInfo->id, status, mask);

        return pDev;
        }

    return NULL;
    }

/******************************************************************************
*
* vxbPcieAerFindSubDev - find agent in End Point or upstream/downstream port
*
* This routine finds whether error agent exits in End Point or 
* upstream/downstream port by calling vxbPcieAerAgentMatch() method.
*
* RETURNS: Returns agent ID on success, or NULL on failure..
*
* ERRNO
*
*/

LOCAL VXB_DEV_ID vxbPcieAerFindSubDev
    (
    VXB_DEV_ID pDev,
    AER_INFO * aerInfo
    )
    {
    PCI_HARDWARE * pIvars;
    VXB_DEV_ID     pChild;
    VXB_DEV_ID     pErrSrc;

    if (pDev == NULL)
        return NULL;

    pIvars = vxbDevIvarsGet (pDev);

    if (pIvars == NULL)
        return NULL;

    pChild = vxbDevFirstChildGet (pDev);

    /* Is this a PCI device? VXBUS devlist not pciDevList */

    if (VXB_BUSID_MATCH(vxbDevClassGet (pChild), VXB_BUSID_PCI) == FALSE)
        return NULL;

    while (pChild != NULL)
        {
        if ((pIvars = vxbDevIvarsGet (pChild)) == NULL)
            return NULL;

        pErrSrc = vxbPcieAerAgentMatch(pChild, pIvars, aerInfo);
        if (pErrSrc)
            return pErrSrc;

        (void)vxbPcieAerFindSubDev(pChild, aerInfo);   

        pChild = vxbDevNextChildGet (pChild);
        }

    return NULL;
    }

/******************************************************************************
*
* vxbPcieAerFindAgent - find agent function
*
* This routine finds whether error agent exits in the bus tree by
* matching ID reported to Root Port or matching error status.
*
* RETURNS: Returns agent ID on success, or NULL on failure..
*
* ERRNO
*
*/

LOCAL VXB_DEV_ID vxbPcieAerFindAgent
    (
    VXB_DEV_ID pDev,
    AER_INFO * aerInfo
    )
    {
    VXB_DEV_ID     pErrSrc;
    PCI_HARDWARE * pIvars;

    if (pDev == NULL)
        return NULL;

    pIvars = vxbDevIvarsGet (pDev);

    if (pIvars == NULL)
        return NULL;

    /* Root Port match */

    pErrSrc = vxbPcieAerAgentMatch(pDev, pIvars, aerInfo);

    /* End Point or upstream/downstream port match */

    if (pErrSrc == NULL)
        pErrSrc = vxbPcieAerFindSubDev(pDev,aerInfo);

    return pErrSrc;
    }

/*******************************************************************************
*
* vxbPciSubDevAction - perform an recovery on all pci devs
*
* This routine performs the recovery action for each pci device directly connected
* to the bus downstream from the specified pDev.
*
* RETURNS: Returns OK on success, or ERROR on failure.
*
* ERRNO
*/
    
LOCAL STATUS vxbPciSubDevAction
    (
    VXB_DEV_ID pDev
    )
    {
    VXB_DEV_ID pChild;

    if (pDev == NULL)
        return ERROR;

    /* iterate through all the instances connected to the bus */

    pChild = vxbDevFirstChildGet (pDev);

    if (pChild == NULL)
        return OK;

    /* Is this a PCI device? */

    if (VXB_BUSID_MATCH(vxbDevClassGet (pChild), VXB_BUSID_PCI) == FALSE)
        return ERROR;

    while (pChild != NULL)
        {
        /* call the function for the instance */

        if (VXB_PCIE_ERR_RECOVERY(pChild) != OK)
            return ERROR;

        /*
         * if the function call returns ERROR, the
         * return status of vxbSubDevAction () will be ERROR.
         */

        if (vxbPciSubDevAction(pChild) != OK)
            return ERROR;

        /* 
         * move on to the next instance in the list 
         * of instances 
         */

        pChild = vxbDevNextChildGet (pChild);
        }

    return OK;
    }

/******************************************************************************
*
* vxbPcieAerInfoGet - get AER capability register
*
* This routine read the pcie device AER structure register
*
* RETURNS: Returns OK on success, or ERROR on failure..
*
* ERRNO
*
*/

LOCAL STATUS vxbPcieAerInfoGet
    (
    VXB_DEV_ID  pDev,
    AER_INFO *  aerInfo
    )
    {
    UINT16 pos = 0;

    if (vxbPcieExtCapFind (pDev, PCIE_EXT_CAP_ID_ERR, &pos) != OK)
        return ERROR;

    if (aerInfo->rootSts & PCI_AER_ROOT_COR_RCV) 
        {
        aerInfo->severity = CORRECTABLE;

        aerInfo->id = ERR_COR_SOURCE_ID(aerInfo->id);

        VXB_PCI_BUS_CFG_READ(pDev, pos + PCIE_AER_CRERR_STATUS, 4,
                             aerInfo->corSts);

        VXB_PCI_BUS_CFG_READ(pDev, pos + PCIE_AER_CRERR_MASK, 4,
                              aerInfo->corMask);

        if (!(aerInfo->corSts & ~aerInfo->corMask))
            return ERROR;
        }
    else if (aerInfo->rootSts & PCI_AER_ROOT_NONFATAL_RCV) 
        {
        aerInfo->severity = NONFATAL;

        aerInfo->id = ERR_UNCOR_SOURCE_ID(aerInfo->id);

        VXB_PCI_BUS_CFG_READ(pDev, pos + PCIE_AER_UNCRERR_STATUS, 4, aerInfo->uncorSts);

        VXB_PCI_BUS_CFG_READ(pDev, pos + PCIE_AER_UNCRERR_MASK, 4,
                             aerInfo->uncorMask);

        if (!(aerInfo->uncorSts & ~aerInfo->uncorMask))
            return ERROR;

        if (aerInfo->uncorSts & AER_LOG_TLP_MASKS) 
            {
            VXB_PCI_BUS_CFG_READ(pDev, pos + PCIE_AER_HEADER_LOG+4, 4, 
                                 aerInfo->headerLog1);

            VXB_PCI_BUS_CFG_READ(pDev, pos + PCIE_AER_HEADER_LOG+8, 4,
                                 aerInfo->headerLog2);

            VXB_PCI_BUS_CFG_READ(pDev, pos + PCIE_AER_HEADER_LOG+12, 4,
                                 aerInfo->headerLog3);
            }
        }
    else
        {
        aerInfo->severity = FATAL;
        }

    return OK;
    }

/******************************************************************************
*
* vxbPcieAerErrInject - register error into EDR system
*
* This routine registers the AER logs into EDR system
*
* RETURNS: null
*
* ERRNO
*
*/

LOCAL STATUS vxbPcieAerErrInject
    (
    VXB_DEV_ID pDev, 
    VXB_DEV_ID pErrDev, 
    AER_INFO * info
    )
    {
    PCI_HARDWARE * pIvars;
    UINT32 status = 0;
    int bit = 0;
    char *errorMsg;

    if ((pIvars = vxbDevIvarsGet (pDev)) == NULL)
        return ERROR;

    if ((errorMsg = malloc(EDR_TEXT_MSG_LEN +1)) == NULL)
       return ERROR;

    strcpy(errorMsg, AER_ERR_MSG_PRE);

    (void) snprintf (errorMsg+strlen(errorMsg), 100,
                     "BDF:              %x-%x-%x\n",pIvars->pciBus, 
                     pIvars->pciDev, pIvars->pciFunc);

    (void) snprintf (errorMsg+strlen(errorMsg), 100,
                     "Device ID:        0x%04x\n", pIvars->pciDevId);

    (void) snprintf (errorMsg+strlen(errorMsg), 100,
                     "Vendor ID:        0x%04x\n", pIvars->pciVendId);

    (void) snprintf (errorMsg+strlen(errorMsg), 100,
                     "Requester ID:     0x%04x\n", info->id);

    (void) snprintf (errorMsg+strlen(errorMsg), 100,
                     "Error Messages:   0x%x\r\n", info->rootSts);

    while (bit < 0x7)
        {
        if (info->rootSts & (1<<bit))
            (void) snprintf (errorMsg+strlen(errorMsg), 100,
            "                  %s\r\n", aerErrMessageString[bit]);
        bit++;
        }

    pIvars = vxbDevIvarsGet (pErrDev);

    (void) snprintf (errorMsg+strlen(errorMsg), 100,
                     "END PORT\n=========\r\n");

    (void) snprintf (errorMsg+strlen(errorMsg), 100,
                     "BDF:              %x-%x-%x\n",pIvars->pciBus,
                     pIvars->pciDev, pIvars->pciFunc);

    (void) snprintf (errorMsg+strlen(errorMsg), 100,
                     "Device ID:        0x%04x\n", pIvars->pciDevId);

    (void) snprintf (errorMsg+strlen(errorMsg), 100,
                     "Vendor ID:        0x%04x\n", pIvars->pciVendId);

    status = (info->uncorSts == 0)? info->corSts : info->uncorSts;

    for (bit = 0; bit< 32; bit++) 
        if ((status & (1 << bit)))
            break;

    switch (info->severity)
        {
        case CORRECTABLE:
        (void) snprintf (errorMsg+strlen(errorMsg), 100,
                         "Error Type:       %s\r\n",
                         aerCorErrString[bit]);
            EDR_PCIE_AER_COR_INJECT(IS_ENABLE_ST, errorMsg);
            break;
        case NONFATAL:
        (void) snprintf (errorMsg+strlen(errorMsg), 100,
                     "TLP Header:       %08x %08x %08x %08x\r\n",
                     info->headerLog0, info->headerLog1,
                     info->headerLog2, info->headerLog3);
        (void) snprintf (errorMsg+strlen(errorMsg), 100,
                         "Error Type:       %s\r\n",
                         aerUnCorErrString[bit]);
            EDR_PCIE_AER_NON_FATAL_INJECT(IS_ENABLE_ST, errorMsg);
            break;
        case FATAL:
        (void) snprintf (errorMsg+strlen(errorMsg), 100,
                         "Error Type:       Unaccessible\r\n");
            EDR_PCIE_AER_FATAL_INJECT(IS_ENABLE_ST, errorMsg);
            break;
        default:
            break;
        }

    free(errorMsg);

    return OK;
    }

/******************************************************************************
*
* vxbPcieDefLinkReset - PCIe reset link by default
*
* This routine resets link by setting "Secondary Bus Reset" bit in "Bridge Control
* Register", it will trigger a hot reset on the corresponding PCI Express Port.
*
* RETURNS: Returns OK on success, or ERROR on failure..
*
* ERRNO
*
*/

LOCAL STATUS vxbPcieDefLinkReset
    (
    VXB_DEV_ID pDev
    )
    {
    UINT32 reg32;
    UINT16 aerOffset;
    UINT16 ctrl;

    if (vxbPcieExtCapFind (pDev, PCIE_EXT_CAP_ID_ERR, &aerOffset) != OK)
        return ERROR;

    /* Disable Root's interrupt in response to error messages */

    if (!aerOffset)
        {

        VXB_PCI_BUS_CFG_READ(pDev, aerOffset + PCIE_AER_ROOT_ERR_COMMAND, 4, reg32);

        reg32 &= ~(PCI_AER_ROOT_CMD_COR_EN | 
                   PCI_AER_ROOT_CMD_NONFATAL_EN |
                   PCI_AER_ROOT_CMD_FATAL_EN);

        VXB_PCI_BUS_CFG_WRITE(pDev, aerOffset + PCIE_AER_ROOT_ERR_COMMAND, 4, reg32);
        }

    /*
     * triggers a hot reset on the corresponding PCI Express Port
     */

    VXB_PCI_BUS_CFG_READ(pDev, PCI_CFG_CB_BRIDGE_CONTROL, 2, ctrl);

    ctrl |= PCI_CFG_SEC_BUS_RESET;

    VXB_PCI_BUS_CFG_WRITE(pDev, PCI_CFG_CB_BRIDGE_CONTROL, 2, ctrl);

    /* minimum reset active time requires 1ms after power stable */

    vxbMsDelay(1);

    ctrl &= (UINT16)~PCI_CFG_SEC_BUS_RESET;

    VXB_PCI_BUS_CFG_WRITE(pDev, PCI_CFG_CB_BRIDGE_CONTROL, 2, ctrl);

    /* leave 1s to subordinate devices to be re-initialized */

    vxbMsDelay(1000);

    /* Enable Root Port's interrupt in response to error messages */

    if (!aerOffset)
        {
        /* Clear Root Error Status */

        VXB_PCI_BUS_CFG_READ(pDev, aerOffset + PCIE_AER_ROOT_ERR_STATUS, 4, reg32);

        VXB_PCI_BUS_CFG_WRITE(pDev, aerOffset + PCIE_AER_ROOT_ERR_STATUS, 4, reg32);

        VXB_PCI_BUS_CFG_READ(pDev, aerOffset + PCIE_AER_ROOT_ERR_COMMAND, 4, reg32);

        reg32 |= PCI_AER_ROOT_CMD_COR_EN |
                 PCI_AER_ROOT_CMD_NONFATAL_EN |
                 PCI_AER_ROOT_CMD_FATAL_EN;

        VXB_PCI_BUS_CFG_WRITE(pDev, aerOffset + PCIE_AER_ROOT_ERR_COMMAND, 4, reg32);
        }

    return OK;
    }

/******************************************************************************
*
* vxbPcieLinkReset - reset pcie agent link
*
* This routine resets pcie agent link. If the agent is a port, the port would 
* execute reset, if the agent is an end-point device, the port of the upstream
* link of end point device would execute reset. The reset method depends on the
* port type. 
*
* RETURNS: Returns OK on success, or ERROR on failure..
*
* ERRNO
*
*/

LOCAL STATUS vxbPcieLinkReset
    (
    VXB_DEV_ID pDev
    )
    {
    VXB_DEV_ID pResetDev;
    int type = 0;
    UINT8 headerType;

    if ((type = vxbPcieDevTypeGet(pDev)) == ERROR)
        return ERROR;

    VXB_PCI_BUS_CFG_READ (pDev, PCI_CFG_HEADER_TYPE, 1, headerType);

    headerType &= PCI_HEADER_TYPE_MASK;

    if (headerType == PCI_HEADER_TYPE_BRIDGE)
        pResetDev = pDev;
    else 
        pResetDev = vxbDevParent(pDev);

    /*
     * As for Root Port and Downstream Port, specification defines 
     * an approach to reset their downstream link. But there is no 
     * stand way to reset the downstream link under the upstream port 
     * because switches might implement different reset approaches.
     * To faclitate the link reset approach, the PCI Express bridge 
     * driver could supply a method VXB_PCIE_RESET_LINK to implement it.
     */

    if ((type == PCI_EXP_TYPE_ROOT_PORT)|| 
        (type == PCI_EXP_TYPE_DOWNSTREAM))
        {
        return vxbPcieDefLinkReset(pResetDev);
        }
    else if (type == PCI_EXP_TYPE_UPSTREAM)
        return (VXB_PCIE_LINK_RESET(pResetDev));

    return OK;
    }

/******************************************************************************
*
* vxbPcieAerRecovery - perform an error recovery procedure
*
* This routine trys to perform an error recovery procedure. It deponds on device
* type. End point device need perform an error recovery procedure only on this
* device, downstream port would do error recovery procedure on all devices 
* under this port, including all switch ports and end point under the point.
*
* RETURNS: Returns OK on success, or ERROR on failure
*
* ERRNO
*
*/

LOCAL STATUS vxbPcieAerRecovery
    (
    VXB_DEV_ID pDev,
    AER_INFO * info
    )
    {
    UINT16 pos = 0;
    STATUS status = OK;
    UINT8 headerType;

    if (info->rootSts & PCI_AER_ROOT_COR_RCV) 
        {
        /*
        * Correctable error does not need software intervention.
        * No need to go through error recovery process.
        */

        if (vxbPcieExtCapFind (pDev, PCIE_EXT_CAP_ID_ERR, &pos) != OK)
            status = ERROR;

        if (pos)
            {
            VXB_PCI_BUS_CFG_WRITE(pDev, pos + PCIE_AER_CRERR_STATUS, 4, info->corSts);
            AER_MSG(0, "Aer Correctable Error Recovery done! \n");
            } 
        }
    else
        {
        /* fatal error need reset link */

        if (info->severity == FATAL) 
            status = vxbPcieLinkReset(pDev);

        VXB_PCI_BUS_CFG_READ (pDev, PCI_CFG_HEADER_TYPE, 1, headerType);

        headerType &= PCI_HEADER_TYPE_MASK;

        if (headerType == PCI_HEADER_TYPE_BRIDGE)
            status = vxbPciSubDevAction(pDev);
        else
            status = VXB_PCIE_ERR_RECOVERY(pDev);

        if (status == OK)
            AER_MSG(0, "Aer Non-correctable Error Recovery done! \n");
        else
            AER_MSG(0, "Aer Non-correctable Error Recovery fail! \n");
        }

    return status;
    }

/******************************************************************************
*
* vxbPcieAerIsr - Aer ISR function
*
* This routine invokes when AER error trigger and generate interrupt.
*
* RETURNS: Returns OK on success, or ERROR on failure..
* 
* ERRNO
*
*/

LOCAL void vxbPcieAerIsr
    (
    VXB_DEV_ID pDev
    )
    {
    UINT16 aerOffset;
    PCI_ROOT_CHAIN * pcieRp = NULL;

    pcieRp = (PCI_ROOT_CHAIN *) vxbDevSoftcGet (pDev);

    if (pcieRp == NULL)
        return;

    aerOffset = pcieRp->aerInfo->aerOffset;

    /* Read Root Port Error Status and Requester ID */

    VXB_PCI_BUS_CFG_READ(pDev, aerOffset + PCIE_AER_ROOT_ERR_STATUS, 4, 
                         pcieRp->aerInfo->rootSts);

    /* No error report */

    if (!pcieRp->aerInfo->rootSts)
        return;

    VXB_PCI_BUS_CFG_READ(pDev, aerOffset + PCIE_AER_ROOT_CESIR, 4, 
                         pcieRp->aerInfo->id);

    AER_MSG(100, "aerOffset:0x%x, root status:0x%x, id 0x%x\n",aerOffset,
                pcieRp->aerInfo->rootSts,pcieRp->aerInfo->id);

    /* Should be correctable or uncorrectable error */

    if (!(pcieRp->aerInfo->rootSts & (PCI_AER_ROOT_UNCOR_RCV | PCI_AER_ROOT_COR_RCV)))
        return;

    /* Clear error status */

    VXB_PCI_BUS_CFG_WRITE(pDev, aerOffset + PCIE_AER_ROOT_ERR_STATUS, 4, 
                          pcieRp->aerInfo->rootSts);

     /* defer the job */

    isrDeferJobAdd (pcieRp->queueId, &pcieRp->isrDef);

    return;
    }

/******************************************************************************
*
* vxbPcieAerInt - AER Int task(tISR0)
*
* This routine would handle AER error in isrDeferJob,including find error agent,
* error inject and do recovery.
*
* RETURNS: Returns OK on success, or ERROR on failure..
*
* ERRNO
*
*/

LOCAL STATUS vxbPcieAerInt
    (
    VXB_DEV_ID pDev
    )
    {
    VXB_DEV_ID pErrDev = NULL;
    PCI_ROOT_CHAIN * pcieRp = NULL;
    STATUS status = OK; 
    pcieRp = (PCI_ROOT_CHAIN *) vxbDevSoftcGet (pDev);

    if (pcieRp == NULL)
        return ERROR;

    /* mutual exclusion begin */

    if (pcieRp->rpAerSem != NULL)
        (void) semTake (pcieRp->rpAerSem, WAIT_FOREVER);

    /* find error source */

    if ((pErrDev = vxbPcieAerFindAgent(pDev, pcieRp->aerInfo)) != NULL)
        {
        /* get error source details */

        if (vxbPcieAerInfoGet(pErrDev, pcieRp->aerInfo) != OK)
            {
            status = ERROR;
            goto error;
            }

        /* Use ED&R to inject an error. */

        if (vxbPcieAerErrInject(pDev, pErrDev, pcieRp->aerInfo) != OK)
            {
            AER_MSG(0, "vxbPcieAerErrInject failed\n");
            status = ERROR;
            goto error;
            }

        /* Do recovery */

        if (vxbPcieAerRecovery(pErrDev, pcieRp->aerInfo) != OK)
            {
            AER_MSG(0, "vxbPcieAerRecovery failed\n");
            status = ERROR;
            goto error;
            }
        }
    else
        {
        AER_MSG(0, "can't find error agent id is 0x%x\n",pcieRp->aerInfo->id);
        status = ERROR;
        goto error;
        }

error:
    if (pcieRp->rpAerSem != NULL)
        (void)semGive (pcieRp->rpAerSem);

    return status;
    }

/******************************************************************************
*
* vxbPcieAerCapDisable - disable PCIe Root Port AER
*
* This routine disable PCIe Root Port's AER
*
* RETURNS: Returns OK on success, or ERROR on failure..
*
* ERRNO
*
*/

void vxbPcieAerCapDisable
    (
    VXB_DEV_ID pDev
    )
    {
    UINT32 reg32 = 0;
    UINT8  pos = 0;
    UINT16 tmp = 0;
    PCI_ROOT_CHAIN * pcieRp = NULL;

    (void) vxbPciExtCapFind (pDev, PCI_EXT_CAP_EXP, &pos);

    if (!pos)
        return;

    pos = (UINT8)(pos - 2);

    VXB_PCI_BUS_CFG_READ(pDev, pos + PCI_EXP_DEVCTL_REG, 2, tmp);

    tmp &= (UINT16)(~(PCI_EXP_DEVCTL_CERR_ENB  |
             PCI_EXP_DEVCTL_NFERR_ENB |
             PCI_EXP_DEVCTL_FERR_ENB  |
             PCI_EXP_DEVCTL_URREP_ENB));

    VXB_PCI_BUS_CFG_WRITE(pDev, pos + PCI_EXP_DEVCTL_REG, 2, tmp);

    /* Disable Root's interrupt */

    if (vxbPcieDevTypeGet(pDev) == PCI_EXP_TYPE_ROOT_PORT)
        {
        pcieRp = (PCI_ROOT_CHAIN *) vxbDevSoftcGet (pDev);

        if (pcieRp == NULL)
            return;

        tmp = pcieRp->aerInfo->aerOffset;

        VXB_PCI_BUS_CFG_READ(pDev, tmp + PCIE_AER_ROOT_ERR_COMMAND, 4, reg32);

        reg32 &= ~(PCI_AER_ROOT_CMD_COR_EN | PCI_AER_ROOT_CMD_NONFATAL_EN |
                  PCI_AER_ROOT_CMD_FATAL_EN);

        VXB_PCI_BUS_CFG_WRITE(pDev, tmp + PCIE_AER_ROOT_ERR_COMMAND, 4, reg32);

        /* Clear Root's error status reg */

        VXB_PCI_BUS_CFG_READ(pDev,  tmp + PCIE_AER_ROOT_ERR_STATUS, 4, reg32);
        VXB_PCI_BUS_CFG_WRITE(pDev, tmp + PCIE_AER_ROOT_ERR_STATUS, 4, reg32);

        (void) vxbIntDisable(pDev, pcieRp->intRes);
        (void) vxbIntDisconnect(pDev, pcieRp->intRes);
        }
    }

#ifdef AER_DEBUG
/******************************************************************************
*
* vxbPcieAerDevScan - perform an recovery on all pci devs on bus controller
*
* This routine performs the recovery action for each pci device directly connected
* to the bus downstream from the specified pDev.
*
* RETURNS: Returns OK on success, or ERROR on failure.
*
* ERRNO
*
*/

LOCAL int rescure=0; 

LOCAL STATUS vxbPcieAerDevScan
    (
    VXB_DEV_ID pDev,
    int        verbose
    )
    {
    VXB_DEV_ID  pChild;
    PCI_HARDWARE * pIvars;
    int i = 0;
    UINT8  clsCode;
    UINT16 aerOffset = 0;
    const char *type[]={
               "PCI Express Endpoint                 ",
               "Legacy PCI Express Endpoint          ",
               "Reserved                             ",
               "Reserved                             ",
               "Root Port of PCI Express Root Complex",
               "Upstream Port of PCI Express Switch  ",
               "Downstream Port of PCI Express Switch",
               "PCI Express to PCI/PCI-X Bridge      ",
               "PCI/PCI-X to PCI Express Bridge      ",
               "Root Complex Integrated Endpoint     ",
               "Root Complex Event Collector         "};

    if (pDev == NULL)
        return ERROR;

    /* iterate through all the instances connected to the bus */

    pChild = vxbDevFirstChildGet (pDev);

    if (pChild == NULL)
        return ERROR;

    pIvars = vxbDevIvarsGet (pChild);
    if (pIvars == NULL)
        return ERROR;

    /* Is this a PCI device? */

    if (VXB_BUSID_MATCH(vxbDevClassGet (pChild), VXB_BUSID_PCI) == FALSE)
        return ERROR;

    rescure++;

   while (pChild != NULL)
        {
        pIvars = vxbDevIvarsGet (pChild);

        if (verbose > 0)
            {
            for (i=1; i<rescure; i++)
                (void) printf("++++++++");

            printf("%02x.%02x.%02x\n",
                pIvars->pciBus,pIvars->pciDev, pIvars->pciFunc);
            }
        else
            {

            /* call the function for the instance */

            printf("%02x.%02x.%02x   ",
                    pIvars->pciBus,pIvars->pciDev, pIvars->pciFunc);

            printf("%04x:%04x   ",
                    pIvars->pciVendId, pIvars->pciDevId);

            VXB_PCI_BUS_CFG_READ(pChild, PCI_CFG_CLASS, 1, clsCode);

            switch (clsCode)
                    {
                    case PCI_CLASS_PRE_PCI20:     (void) printf("BEFORE_STD                  "); break;
                    case PCI_CLASS_MASS_STORAGE:  (void) printf("Sata Controller             "); break;
                    case PCI_CLASS_NETWORK_CTLR:  (void) printf("Ethernet Controller         "); break;
                    case PCI_CLASS_DISPLAY_CTLR:  (void) printf("Graphics Controller         "); break;
                    case PCI_CLASS_MMEDIA_DEVICE: (void) printf("Audio Controller            "); break;
                    case PCI_CLASS_MEM_CTLR:      (void) printf("Dram Controller             "); break;
                    case PCI_CLASS_COMM_CTLR:     (void) printf("Communication Controller    "); break;
                    case PCI_CLASS_BASE_PERIPH:   (void) printf("Peripheral                  "); break;
                    case PCI_CLASS_INPUT_DEVICE:  (void) printf("Input                       "); break;
                    case PCI_CLASS_PROCESSOR:     (void) printf("Processor                   "); break;
                    case PCI_CLASS_SERIAL_BUS:    (void) printf("Serail Bus                  "); break;
                    case PCI_CLASS_WIRELESS:      (void) printf("Wireless                    "); break;
                    case PCI_CLASS_DAQ_DSP:       (void) printf("Signal Processing Controller"); break;
                    case PCI_CLASS_UNDEFINED:     (void) printf("Undefind                    "); break;
                    case PCI_CLASS_BRIDGE_CTLR:   (void) printf("Bridge                      "); break;
                    default:                      (void) printf("Unknown                     ");
                    }

                    i = vxbPcieDevTypeGet(pChild);

                    if (i>=0)
                        {
                        printf(" %s", type[i]);
                        aerOffset = 0;
                        if (vxbPcieExtCapFind (pChild, PCIE_EXT_CAP_ID_ERR, &aerOffset) == OK)
                            {
                            if (aerOffset)
                                printf("  \"AER(%#x)\"",aerOffset);
                            }
                        else
                                printf("              ");
                        }

            if (vxbDevMatch(pChild)==OK)
                printf("  %s\n",vxbDevNameGet(pChild));
            else
                printf("\n");
            }

            /*
             * if the function call returns ERROR, the
             * return status of vxbSubDevAction () will be ERROR.
             */

            if (vxbPcieAerDevScan(pChild, verbose) == OK)
                rescure--;

            /* 
             * move on to the next instance in the list 
             * of instances 
             */

            pChild = vxbDevNextChildGet (pChild);
        }

    return OK;
    }

/******************************************************************************
*
* vxbPcieAerShow - utilty function
*
* RETURNS: null
*
* ERRNO
*
*/

void vxbPcieAerShow
    (
    VXB_DEV_ID pDev
    )
    {
    if (pDev == NULL)
        return;

    printf("\r\n========Topology(b.d.f)========\n");

    vxbPcieAerDevScan(pDev, 1);

    rescure = 0;

    printf("\r\n=======================================================");
    printf("=======================================================\n");

    vxbPcieAerDevScan(pDev, 0);

    rescure = 0;

    return;
    }

/******************************************************************************
*
* vxbPcieAerShow - utilty function for related register show

*
* RETURNS: null
*
* ERRNO
*
*/

void vxbPcieAerRegShow
    (
    VXB_DEV_ID pDev
    )
    {
    UINT8  val8  = 0;
    UINT16 val16 = 0;
    UINT32 val32 = 0;

    if (pDev == NULL)
        return;

    printf("\r\n========AER Related Register Dump========\n");

    /* Command Register */

    VXB_PCI_BUS_CFG_READ(pDev, PCI_CFG_COMMAND, 2, val16);
    if (val16)
        printf("Command Register(Offset 04h):                   0x%08x, SERR%s\n",
            val16, ((val16&PCI_CMD_SERR_ENABLE)==PCI_CMD_SERR_ENABLE)?"+":"-");

    /* Bridge Control Register */

    VXB_PCI_BUS_CFG_READ(pDev, PCI_CFG_BRIDGE_CONTROL, 2, val16);
    if (val16)
        printf("Bridge Control Register(Offset 3Eh):            0x%08x, SERR%s\n",
            val16, ((val16&PCI_CFG_SERR)==PCI_CFG_SERR)?"+":"-");

    /* Device Control Register */

    (void) vxbPciExtCapFind (pDev, PCI_EXT_CAP_EXP, &val8);

    if (!val8)
        {
        printf("pDev 0x%p is not a PCI Express device!\r\n");
        return;
        }

    val8 = (UINT8)(val8 - 2);

    VXB_PCI_BUS_CFG_READ(pDev, val8 + PCI_EXP_DEVCTL_REG, 2, val16);

    if (val16)
        printf("Device Control Register(Offset 1Ch + %xh):      0x%08x, CERR%s,"
            " NFERR%s, FERR%s, URREP%s\n",val8, val16, 
            ((val16&PCI_EXP_DEVCTL_CERR_ENB)==PCI_EXP_DEVCTL_CERR_ENB)?"+":"-",
            ((val16&PCI_EXP_DEVCTL_NFERR_ENB)==PCI_EXP_DEVCTL_NFERR_ENB)?"+":"-",
            ((val16&PCI_EXP_DEVCTL_FERR_ENB)==PCI_EXP_DEVCTL_FERR_ENB)?"+":"-",
            ((val16&PCI_EXP_DEVCTL_URREP_ENB)==PCI_EXP_DEVCTL_URREP_ENB)?"+":"-"
            );

    /* Root Control Register */

    VXB_PCI_BUS_CFG_READ(pDev, val8 + PCI_EXP_RTCTL_REG, 2, val16);
    if (val16)
        printf("Root Control Register(Offset 08h + %xh):        0x%08x, SECE%s,"
            " SEFE%s, SEFE%s\r\n",val8, val16, 
            ((val16&PCI_EXP_RTCTL_SECE_ENB)==PCI_EXP_RTCTL_SECE_ENB)?"+":"-",
            ((val16&PCI_EXP_RTCTL_SEFE_ENB)==PCI_EXP_RTCTL_SEFE_ENB)?"+":"-",
            ((val16&PCI_EXP_RTCTL_SEFE_ENB)==PCI_EXP_RTCTL_SEFE_ENB)?"+":"-");

    /* Root Error Command Register */

    if (vxbPcieExtCapFind (pDev, PCIE_EXT_CAP_ID_ERR, &val16) != OK)
        {
        printf("pDev 0x%p does not support AER\n", pDev);
        return;
        }

    VXB_PCI_BUS_CFG_READ(pDev, val16 + PCIE_AER_ROOT_ERR_COMMAND, 4, val32);
    if (val32)
        printf("Root Error Command Register(Offset 2Ch + %xh): 0x%08x, COR%s,"
            " NONFATAL%s, FATAL%s\r\n",val16, val32, 
            ((val32&PCI_AER_ROOT_CMD_COR_EN)==PCI_AER_ROOT_CMD_COR_EN)?"+":"-",
            ((val32&PCI_AER_ROOT_CMD_NONFATAL_EN)==PCI_AER_ROOT_CMD_NONFATAL_EN)?"+":"-",
            ((val32&PCI_AER_ROOT_CMD_FATAL_EN)==PCI_AER_ROOT_CMD_FATAL_EN)?"+":"-");

    /* MISC Control and Status Register ENABLE_SYS_ERR_FOR_AER bit */

    VXB_PCI_BUS_CFG_READ(pDev, 0x188, 4, val32);
    if (val32)
        printf("MISC Control and Status Register(Offset 188h):  0x%x, "
            "Enable System Error for AER%s\r\n", val32, 
            ((val32&0x10)==0x10)?"-":"+");
    return;


    }
#endif
