/* vxbPciMsi.c - PCI MSI/MSI-X libary */

/*
 * Copyright (c) 2014, 2015, 2017 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
08jun17,l_z  return ERROR when MSI width aren't matched. (V7PRO-3815)
15dec15,l_z  Enable the MSI Per-vector Mask (V7PRO-1625)
07dec15,dlk  Enable the MSI Per-vector Mask (V7PRO-1625 regression)
31aug15,wap  Remove left over testing stub routine
10aug15,wap  Add library init routine
05aug15,wap  Correct some documentation issues
24jul15,wap  Save the VXB_RESOURCE record for each dynamic vector using
             vxbIsrSetIntrEntry()
22jul15,wap  Extensive cleanup, remove vxbPciExtCapFind() (now a common
             API in vxbPci), make vxbPciMsiProgram() and vxbPciMsiXProgram()
             public APIs
17jul15,wap  Fix various problems with vxbPciMsiXFree()
16jun15,wap  Remember to include vxbPciMsi.h, correct type of 2nd
             argument to vxbPciMsiXFree(), fix compiler warnings
             when debugging is turned on
15jun15,wap  Add extra sanity check to vxbPciMsiFree() (F3973)
03jul14,y_y  Created.
*/

/*
DESCRIPTION
This library contains the support routines for PCI MSI and MSI-X.
*/

#include <vxWorks.h>
#include <ffsLib.h>
#include <hwif/vxBus.h>
#include <hwif/buslib/pciDefines.h>
#include <hwif/buslib/vxbPciLib.h>
#include <hwif/buslib/vxbPciMsi.h>

/* Debug macro */

#undef  PCI_MSI_DEBUG
#ifdef  PCI_MSI_DEBUG

#include <stdio.h>
#include <private/kwriteLibP.h>

LOCAL int debugLevel = 0;

#ifdef  LOCAL
#undef  LOCAL
#define LOCAL
#endif

#define PCI_MSI_MSG(lvl, fmt, args...)        \
    do {                                      \
        if (debugLevel >= lvl)                \
            {                                 \
            if (_func_kprintf != NULL)        \
                _func_kprintf (fmt, ##args);  \
            }                                 \
       } while (FALSE)
#else
#define PCI_MSI_MSG(lvl, fmt, args...)

#endif  /* PCI_DEBUG */

#define VXB_INT_PCI_INTX   0xaa

/*******************************************************************************
*
* vxbPciMsiInit - initialization routine for PCI MSI library
*
* This routine initializes the PCI MSI library. Currently it's just a stub
* provided as an init hook for the PCI component definition file.
*
* RETURNS: always OK
*
* ERRNO: N/A
*/

STATUS vxbPciMsiInit (void)
    {
    return (OK);
    }

/******************************************************************************
*
* vxbPciIntCapChk - Check PCI device MSI, MSI-X Capability
*
* This routine check if the device has specified number of MSI or MSI-X
* interrupts.
*
* RETURNS: OK, if it has specified MSI, MSI-X interrupt. Otherwise, ERROR.
*
* \NOMANUAL
*/

LOCAL STATUS vxbPciIntCapChk
    (
    VXB_DEV_ID     pDev,
    UINT32         type,
    UINT32         count
    )
    {
    UINT8       msiOffset = 0;
    UINT16      msiCtl    = 0;

    if (type == PCI_EXT_CAP_MSIX)
        {
        /* Check MSI-X */

        (void) vxbPciExtCapFind (pDev, PCI_EXT_CAP_MSIX, &msiOffset);

        if (msiOffset == 0)
            return ERROR;

        VXB_PCI_BUS_CFG_READ(pDev, (UINT32)(msiOffset), 2, msiCtl);

        if (count > ((msiCtl & PCI_MSIX_CTL_TABLE_SIZE) + 1))
            return ERROR;

        return OK;
        }

    if (type == PCI_EXT_CAP_MSI)
        {
        /* Check MSI */

        (void) vxbPciExtCapFind (pDev, PCI_EXT_CAP_MSI, &msiOffset);

        if (msiOffset == 0)
            return ERROR;

        VXB_PCI_BUS_CFG_READ (pDev, (UINT32)(msiOffset), 2, msiCtl);

        if (count > (1 << ((msiCtl & PCI_MSI_CTL_MSG_MAX) >> 1)))
            return ERROR;

        return OK;
        }

    return ERROR;
    }

/******************************************************************************
*
* vxbPciIntrExclusiveEnable - enable a kind of PCI interrupt mutual exclusively
*
* This routine enables a kind of PCI interrupt mutual exclusively.
*
* MSI-X/MSI/INTx are mutual exclusive. When enabling INTx, we need to make
* sure MSI/MSI-X are disabled; when enabling MSI, we need to make sure MSI-X
* and INTx are disabled; when enabling MSI-X, we need to make sure MSI and
* INTx are disabled.
*
* Clearing/setting PCI_CMD_INTX_DISABLE bit of command register can enable/disable
* INTx interrupt. MSI/MSI-X has its own control register and can be globally
* enabled/disabled. MSI-X has per-mask capability, MSI may have per-mask capability.
* refer to PCI Local Bus Specification MSI-X ECN to get the details.
*
* RETURNS: OK/ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS vxbPciIntrExclusiveEnable
    (
    VXB_DEV_ID     pDev,
    UINT32         intrType
    )
    {
    UINT16 command = 0;
    UINT16 msiCtl = 0;
    UINT8  msiOffset = 0;

    VXB_PCI_BUS_CFG_READ (pDev, PCI_CFG_COMMAND, 2, command);

    if (intrType == VXB_INT_PCI_INTX)
        {
        command = (UINT16)(command & ~PCI_CMD_INTX_DISABLE);

        PCI_MSI_MSG(1000, "vxbPciIntrExclusiveEnable - enable INTx\n");
        }
    else
        {
        command |= (PCI_CMD_INTX_DISABLE);

        PCI_MSI_MSG(1000, "vxbPciIntrExclusiveEnable - enable MSI/MSI-X command: 0x%x\n",
            command);
        }

    VXB_PCI_BUS_CFG_WRITE(pDev, PCI_CFG_COMMAND, 2, command);

    if ((intrType == VXB_INT_PCI_INTX) ||
        (intrType == PCI_EXT_CAP_MSIX))
        {
        msiOffset = 0;
        (void) vxbPciExtCapFind (pDev, PCI_EXT_CAP_MSI, &msiOffset);

        if (msiOffset != 0)
            {
            VXB_PCI_BUS_CFG_READ(pDev, (UINT32)msiOffset, 2, msiCtl);

            msiCtl = (UINT16)(msiCtl & ~PCI_MSI_CTL_ENABLE);

            PCI_MSI_MSG(1000, "vxbPciIntrExclusiveEnable - disable MSI for MSI-X/INTx\n");

            VXB_PCI_BUS_CFG_WRITE(pDev, (UINT32)msiOffset , 2, msiCtl);
            }
        }

    if ((intrType == VXB_INT_PCI_INTX) || (intrType == PCI_EXT_CAP_MSI))
        {
        msiOffset = 0;

        (void) vxbPciExtCapFind (pDev, PCI_EXT_CAP_MSIX, &msiOffset);

        if (msiOffset != 0)
            {
            VXB_PCI_BUS_CFG_READ(pDev, (UINT32)msiOffset, 2, msiCtl);

            msiCtl = (UINT16)(msiCtl & ~PCI_MSIX_CTL_ENABLE);

            PCI_MSI_MSG(1000, "vxbPciIntrExclusiveEnable - disable MSI-X for MSI/INTx\n");

            VXB_PCI_BUS_CFG_WRITE(pDev, (UINT32)msiOffset, 2, msiCtl);
            }
        }

    return OK;
    }

/******************************************************************************
*
* vxbPciMsiGet - get the message address and data
*
* This routine get the message address and data to dynaVec
*
* RETURNS: OK or ERROR when PCI device doesn't support MSI
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS vxbPciMsiGet
    (
    VXB_DEV_ID           pDev,
    VXB_DYNC_INT_ENTRY * dynaVec
    )
    {
    UINT32     tempa32;
    UINT32     tempb32 = 0;
    UINT16     temp16;
    UINT8      msiOffset = 0, msiCtl;

    /* Capabilities List Implemented: Get first capability ID */

    (void) vxbPciExtCapFind (pDev, PCI_EXT_CAP_MSI, &msiOffset);

    if (msiOffset == 0)
        return ERROR;

    VXB_PCI_BUS_CFG_READ (pDev, (UINT32)(msiOffset), 2, msiCtl);

    /*
     * This offset is for PCI_MSI_CTL which is 2bytes in advance vs value
     * expected by defs so subtract.
     */

    msiOffset = (UINT8)(msiOffset - PCI_MSI_CTL);

    VXB_PCI_BUS_CFG_READ (pDev, (UINT32)(msiOffset + PCI_MSI_ADDR_LO), 4, tempa32);

    if(msiCtl & PCI_MSI_CTL_64BIT)
        {
        VXB_PCI_BUS_CFG_READ (pDev, (UINT32)(msiOffset + PCI_MSI_ADDR_HI), 4, tempb32);

        VXB_PCI_BUS_CFG_READ (pDev, (UINT32)(msiOffset + PCI_MSI_DATA_64), 2, temp16);
        }
    else
        {
        VXB_PCI_BUS_CFG_READ (pDev, (UINT32)(msiOffset + PCI_MSI_DATA_32), 2, temp16);
        }

    dynaVec->vecAddr = (UINT64)tempa32 | ((UINT64)tempb32 << 32);

    dynaVec->vecVal = temp16;

    PCI_MSI_MSG(1000, "Got Here MSI dev 0x%x busCtrl 0x%x, "
              "vecAddr 0x%x Val 0x%x\n",
              (_Vx_usr_arg_t)pDev,
              (_Vx_usr_arg_t)vxbDevParent(pDev),
              (_Vx_usr_arg_t)dynaVec->vecAddr ,
              (_Vx_usr_arg_t)dynaVec->vecVal);

    return OK;
    }

/******************************************************************************
*
* vxbPciMsiIsCap - Check MSI Capability
*
* This routine check the device is capable of MSI. Device can be a plain
* PCI, PCI-x,PCI-e and/or PCI-e with MSI-x feature option. A PCIx is capable
* of MSI scheme only if the MSI Capability is set or it is PCI-Express
* Capable.
*
* RETURNS:  It returns TRUE, if it is MSI capable. Otherwise, FALSE.
*
* Note:
* There is no checking for MSI-X, because that category is a subclass
* of PCI-e devices.
*
* \NOMANUAL
*/

int vxbPciMsiIsCap
    (
    VXB_DEV_ID     pDev
    )
    {
    UINT8 msiOffset = 0;
    UINT8 expOffset = 0;

    /* Check if MSI is supported for PCI device */

    (void) vxbPciExtCapFind (pDev, PCI_EXT_CAP_MSI, &msiOffset);

    /* If MSI not supported, Check if the device is capable of PCI-e */

    if (msiOffset==0)
        {
        (void) vxbPciExtCapFind (pDev, PCI_EXT_CAP_EXP, &expOffset);

        if (expOffset == 0)
            {
            PCI_MSI_MSG(0, "The device 0x%p is not MSI capaple\n",
                pDev);
            return (FALSE);
            }

        PCI_MSI_MSG(0, "Device %p is PCI-e capable\n",pDev);
        }

    PCI_MSI_MSG(0, "The Device %p is MSI capable\n", pDev);

    return (TRUE);
    }

/******************************************************************************
*
* vxbPciMsiProgram - programm multiple MSI vectors
*
* This routine programs multiple MSI vectors. MSI can only support 1,2,4,8,16,32
* interrupts, all the interrupts share the same address and data register.
* So MSI data must be increase progressively based the data of the first MSI.
* The MSI control register is set to enable the MSI with the specified number.
*
* RETURNS: ERROR when MSI is unavailable, else OK
*
* ERRNO: N/A
*/

STATUS vxbPciMsiProgram
    (
    VXB_DEV_ID           pDev
    )
    {
    PCI_HARDWARE * pIvars;
    UINT8          msiOffset = 0, msiEn;
    UINT16         msiCtl, vecVal;
    UINT32         vecAddr;
    int            numVectors;
    VXB_DYNC_INT_ENTRY * pMsiInfo;
    UINT32         msiMaskReg;
    UINT32         msiMask;

    pIvars = (PCI_HARDWARE *)vxbDevIvarsGet (pDev);

    numVectors = pIvars->pciDyncIntCnt;
    pMsiInfo = pIvars->pciDyncIntEntry;

    PCI_MSI_MSG(10, "vxbPciMsiProgram: pDev %p "
                "VecAddr 0x%x Val %d numVectors %d\n",
                (_Vx_usr_arg_t)pDev,
                (_Vx_usr_arg_t)pMsiInfo->vecAddr ,
                (_Vx_usr_arg_t)pMsiInfo->vecVal,
                numVectors);

    /* MSI can support only 1, 2, 4, 8, 16, 32 number of vectors */

    switch (numVectors)
        {
        case 1:
            msiEn = 0;
            break;
        case 2:
            msiEn = 1;
            break;
        case 4:
            msiEn = 2;
            break;
        case 8:
            msiEn = 3;
            break;
        case 16:
            msiEn = 4;
            break;
        case 32:
            msiEn = 5;
            break;
        default:
            return(ERROR);
        }

    (void) vxbPciExtCapFind(pDev, PCI_EXT_CAP_MSI, &msiOffset);

    if (msiOffset == 0)
        return(ERROR);

    VXB_PCI_BUS_CFG_READ (pDev, (UINT32)msiOffset, 2, msiCtl);

    /* This offset is for MSI_CTL which is 2 bytes on so subtract to use defs */

    msiOffset = (UINT8)(msiOffset - PCI_MSI_CTL);

    vecAddr = (UINT32)pMsiInfo->vecAddr;

    VXB_PCI_BUS_CFG_WRITE(pDev, (UINT32)(msiOffset + PCI_MSI_ADDR_LO), 4, vecAddr);

    vecVal = (UINT16) pMsiInfo->vecVal;
    
    vecAddr = (UINT32)(pMsiInfo->vecAddr >> 32);

    if (msiCtl & PCI_MSI_CTL_64BIT)
        {
        VXB_PCI_BUS_CFG_WRITE(pDev, (UINT32)(msiOffset + PCI_MSI_ADDR_HI), 4, vecAddr);
        VXB_PCI_BUS_CFG_WRITE(pDev, (UINT32)(msiOffset + PCI_MSI_DATA_64), 2, vecVal);
        }
    else
        {

        if (vecAddr != 0)
            {
            VXB_PCI_BUS_CFG_WRITE(pDev, (UINT32)(msiOffset + PCI_MSI_ADDR_LO), 4, 0);
            return ERROR;
            }
        
        VXB_PCI_BUS_CFG_WRITE (pDev, (UINT32)(msiOffset + PCI_MSI_DATA_32), 2, vecVal);
        }

    msiCtl = (UINT16)(msiCtl & ~PCI_MSI_CTL_MSG_ALLOC);
    msiCtl |= PCI_MSI_CTL_ENABLE;
    msiCtl = (UINT16)(msiCtl | msiEn << 4);

    VXB_PCI_BUS_CFG_WRITE(pDev, (UINT32)(msiOffset + PCI_MSI_CTL), 2, msiCtl);

    /* If the device does not support Per-vector masking, we are done! */

    if (!(msiCtl & PCI_MSI_CTL_MASK))
        {
        return OK;
        }

    /* Handle per vector-masking */

    if (msiCtl & PCI_MSI_CTL_64BIT)
        msiMaskReg = PCI_MSI_MASK_64;
    else
        msiMaskReg = PCI_MSI_MASK_32;

    VXB_PCI_BUS_CFG_READ (pDev, (UINT32)(msiOffset + msiMaskReg), 4, msiMask);

    msiMask &= ((~1U) << (UINT32)(numVectors - 1));

    VXB_PCI_BUS_CFG_WRITE(pDev, (UINT32)(msiOffset + msiMaskReg), 4, msiMask);

    return OK;
    }

/******************************************************************************
*
* vxbPciMsiAlloc - alloc PCI/PCIe MSI interrupt
*
* This routine allocates a number of PCI/PCIe MSI interrupts as specified
* by <cnt>.
*
* RETURNS: number of allocated MSI vectors if allocation OK
*          or ERROR if allocation failed.
*
* ERRNO
*/

int vxbPciMsiAlloc
    (
    VXB_DEV_ID pDev,
    UINT32     cnt
    )
    {
    VXB_DYNC_INT_ENTRY *msiInf;
    VXB_RESOURCE_IRQ * vxbResIrq = NULL;
    VXB_RESOURCE_IRQ * vxbResIrqTmp = NULL;
    VXB_RESOURCE     * pMsiRes = NULL;
    VXB_RESOURCE     * pMsiResTmp = NULL;
    PCI_HARDWARE * pPciDev;
    int i, num=0;

    pPciDev = vxbDevIvarsGet (pDev);

    if (vxbPciIntCapChk (pDev, PCI_EXT_CAP_MSI, cnt) != OK)
        return ERROR;

    if ((msiInf = vxbMemAlloc(cnt * sizeof(VXB_DYNC_INT_ENTRY))) == NULL)
        return ERROR;

    num = vxbIntAlloc (cnt, msiInf);

    if (num <= 0)
        {
        vxbMemFree ((char *)msiInf);
        return ERROR;
        }

    pMsiRes = (VXB_RESOURCE *)vxbMemAlloc(sizeof(VXB_RESOURCE) * num);

    if (pMsiRes == NULL)
        {
        vxbIntFree (num, msiInf);
        vxbMemFree ((char *)msiInf);
        return ERROR;
        }

    vxbResIrq = (VXB_RESOURCE_IRQ *)vxbMemAlloc(sizeof(VXB_RESOURCE_IRQ) * num);

    if (vxbResIrq == NULL)
        {
        vxbIntFree (cnt, msiInf);
        vxbMemFree ((char *)msiInf);
        vxbMemFree ((char *)pMsiRes);
        return ERROR;
        }

    vxbResIrqTmp = vxbResIrq;
    pMsiResTmp = pMsiRes;

    /* add the MSI resource */

    for (i = 0; i < num; i++)
        {
        vxbResIrqTmp->flag |= VXB_INT_FLAG_DYNAMIC;

        vxbResIrqTmp->hVec = msiInf[i].hVec;
        vxbResIrqTmp->lVec = msiInf[i].lVec;

        vxbResIrqTmp->pVxbIntrEntry = (void *)&msiInf[i];

        pMsiResTmp->id = VXB_RES_ID_CREATE(VXB_RES_IRQ, i + 1);
        pMsiResTmp->pRes = (void *)vxbResIrqTmp;

        if (vxbResourceAdd (&pPciDev->vxbResList, pMsiResTmp) != OK)
            goto error;

        (void) vxbIsrSetIntrEntry (msiInf[i].lVec, (void *)pMsiResTmp);

        vxbResIrqTmp++;
        pMsiResTmp++;
        }

    /* save the dynamic interrupt entry */

    pPciDev->pciDyncIntEntry = msiInf;
    pPciDev->pciDyncIntCnt = num;

    if (vxbPciMsiProgram (pDev) != OK)
        goto error;

    if (vxbPciIntrExclusiveEnable (pDev, PCI_EXT_CAP_MSI) != OK)
        goto error;

    return num;

error:
    vxbIntFree (num, msiInf);
    vxbMemFree ((char *)msiInf);
    vxbMemFree ((char *)pMsiRes);
    vxbMemFree ((char *)vxbResIrq);
    pPciDev->pciDyncIntEntry = NULL;
    pPciDev->pciDyncIntCnt = 0;

    return ERROR;
    }

/******************************************************************************
*
* vxbPciMsiFree - erase multiple MSI vectors
*
* This routine erases the multiple MSI vectors at the same time. MSI vectors
* can't be erased individually, so this routine just disables all the MSI vectors.
*
* RETURNS: ERROR when MSI is unavailable, else OK
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS vxbPciMsiFree
    (
    VXB_DEV_ID           pDev,
    UINT32               cnt
    )
    {
    VXB_DEV_ID pBridge;
    UINT32 msiOffset = 0;
    UINT16 msiCtl;
    UINT32 vecAddr;
    UINT16 vecVal;
    VXB_DYNC_INT_ENTRY * dynaVec;
    PCI_HARDWARE * pPciDev;
    VXB_RESOURCE_IRQ * vxbResIrq = NULL;
    VXB_RESOURCE     * pMsiRes = NULL;
    UINT16 i;

    pBridge = vxbDevParent (pDev);
    pPciDev = vxbDevIvarsGet (pDev);

    dynaVec = pPciDev->pciDyncIntEntry;

    /*
     * If vxbPciMsiAlloc() failed (either because the dynamic
     * interrupt component is missing or the interrupt controller
     * doesn't support dynamic interrupts), then the pciDyncIntEntry
     * field will be NULL. We should check for this before proceeding.
     */

    if (dynaVec == NULL)
        return (ERROR);

    PCI_MSI_MSG(10, "vxbPciMsiFree : dev 0x%x bus Ctrl 0x%x"
                "vecAddr 0x%x Val 0x%x\n",
                (_Vx_usr_arg_t)pDev,
                (_Vx_usr_arg_t)pBridge,
                (_Vx_usr_arg_t)dynaVec->vecAddr ,
                (_Vx_usr_arg_t)dynaVec->vecVal);

    if (pBridge == NULL)
        return (ERROR);

    if (vxbPciIntCapChk (pDev, PCI_EXT_CAP_MSI, cnt) != OK)
        return (ERROR);

    vxbIntFree (cnt, dynaVec);

    for (i = 0; i < cnt; i++)
        (void) vxbIsrSetIntrEntry (dynaVec[i].lVec, NULL);

    /* remove the MSI resource */

    for (i = 1; i < cnt; i++)
        {
        if ((pMsiRes = vxbResourceAlloc(pDev, VXB_RES_IRQ, (UINT16)(i + 1))) == NULL)
            return ERROR;

        (void) vxbResourceRemove(&pPciDev->vxbResList, pMsiRes);
        }

    if ((pMsiRes = vxbResourceAlloc(pDev, VXB_RES_IRQ, 1)) == NULL)
        return ERROR;

    (void) vxbResourceRemove(&pPciDev->vxbResList, pMsiRes);
    vxbResIrq = pMsiRes->pRes;
    vxbMemFree (vxbResIrq);
    vxbMemFree (pMsiRes);

    /* Capabilities List Implemented: Get first capability ID */

    (void) vxbPciExtCapFind (pDev, PCI_EXT_CAP_MSI, (UINT8 *)&msiOffset);

    if (msiOffset == 0)
        return(ERROR);

    (void) VXB_PCI_CFG_READ (pBridge, pPciDev, (UINT32)msiOffset, 2, &msiCtl);

    /* This offset is for MSI_CTL which is 2bytes on so subtract to use defs */

    msiOffset = (UINT8)(msiOffset - PCI_MSI_CTL);

    vecVal = 0;
    vecAddr = 0;

    (void) VXB_PCI_CFG_WRITE (pBridge, pPciDev, (UINT32)(msiOffset + PCI_MSI_ADDR_LO), 4, vecAddr);

    if (msiCtl & PCI_MSI_CTL_64BIT)
        {
        (void) VXB_PCI_CFG_WRITE (pBridge, pPciDev, (UINT32)(msiOffset + PCI_MSI_ADDR_HI), 4, vecAddr);

        (void) VXB_PCI_CFG_WRITE (pBridge, pPciDev, (UINT32)(msiOffset + PCI_MSI_DATA_64), 2, vecVal);

        }
    else
        {
        (void) VXB_PCI_CFG_WRITE (pBridge, pPciDev, (UINT32)(msiOffset + PCI_MSI_DATA_32), 2, vecVal);
        }

    msiCtl = (UINT16)(msiCtl & ~PCI_MSI_CTL_MSG_ALLOC);

    (void) VXB_PCI_CFG_WRITE (pBridge, pPciDev, (UINT32)(msiOffset + PCI_MSI_CTL), 2, msiCtl);

    if (vxbPciIntrExclusiveEnable (pDev, VXB_INT_PCI_INTX) != OK)
        return ERROR;

    vxbMemFree((char *)dynaVec);
    pPciDev->pciDyncIntEntry = NULL;
    pPciDev->pciDyncIntCnt = 0;

    return (OK);
    }

/*******************************************************************************
*
* vxbPciMsiXProgram - programm multiple MSI-X vectors
*
* This routine programs multiple MSI-X vectors.
*
* RETURNS: ERROR when MSI-X is unavailable, else OK.
*
* ERRNO: N/A
*/

STATUS vxbPciMsiXProgram
    (
    VXB_DEV_ID           pDev
    )
    {
    PCI_HARDWARE * pIvars;
    UINT8          msixOffset=0;
    UINT16         msixCtl;
    UINT32         table, pba, tableOffset, *pTableEntry;
    ULONG          tableBase;
    int            i;
    VXB_RESOURCE * pRes;
    VXB_DYNC_INT_ENTRY *pCurMsi;
    int                  numVectors;
    VXB_DYNC_INT_ENTRY * pMsiInfo;

    pIvars = (PCI_HARDWARE *)vxbDevIvarsGet (pDev);

    numVectors = pIvars->pciDyncIntCnt;
    pMsiInfo = pIvars->pciDyncIntEntry;

    (void) vxbPciExtCapFind (pDev, PCI_EXT_CAP_MSIX, &msixOffset);

    if (msixOffset == 0)
        return ERROR;

    /*
     * This offset is for MSIX_CTL which is 2 bytes, so must be adjusted
     * to get the base offset
     */

    msixOffset = (UINT8)(msixOffset - PCI_MSIX_CTL);

    /*
     * Software reads the Table Size field from the Message Control
     * register to determine the MSI-X Table size. The field encodes
     * the number of table entries as N-1, so software must add 1 to
     * the content read from the field to calculate the number of table
     * entries N. MSI-X supports a maximum table size of 2048 entries.
     */

    VXB_PCI_BUS_CFG_READ (pDev, msixOffset + PCI_MSIX_CTL, 2, msixCtl);

    /*
     * Make sure the function mask is cleared (so we are on a per vector
     * control).
     */

    msixCtl = (UINT16)(msixCtl & ~PCI_MSIX_CTL_FUNCTION_MASK);

    /*
     * Software calculates the base address of the MSI-X Table by reading
     * the 32-bit value from the Table Offset / BIR register, masking
     * off the lower 3 BIR bits, and adding the remaining 8-byte aligned
     * 32-bit offset to the address taken from the Base Address register
     * indicated by the BIR.
     */

    VXB_PCI_BUS_CFG_READ (pDev, msixOffset + PCI_MSIX_TABLE, 4, table);

    tableOffset = (table & PCI_MSIX_TABLE_OFFSET_MASK);

    VXB_PCI_BUS_CFG_READ (pDev, msixOffset + PCI_MSIX_PBA, 4, pba);

    pRes = vxbResourceAlloc (pDev, VXB_RES_MEMORY, table & PCI_MSIX_TABLE_BIR_MASK);

    if (pRes == NULL || pRes->pRes == NULL)
        return ERROR;

    tableBase = ((VXB_RESOURCE_ADR*)pRes->pRes)->virtual;

    pRes = vxbResourceAlloc (pDev, VXB_RES_MEMORY, pba & PCI_MSIX_PBA_BIR_MASK);

    if (pRes == NULL || pRes->pRes == NULL)
        return ERROR;

    PCI_MSI_MSG(100, "%s%d: MSI-X @%p BAR%d tableBase %p"
                     " tableOffset %p tableSize %d pbaBase %p\n",
                vxbDevNameGet (pDev),
                vxbDevUnitGet (pDev),
                msixOffset,
                table & PCI_MSIX_TABLE_BIR_MASK,
                tableBase, tableOffset, (msixCtl & PCI_MSIX_CTL_TABLE_SIZE) + 1,
                ((VXB_RESOURCE_ADR*)(pRes->pRes))->virtual);

    pTableEntry = (UINT32 *)(tableBase + tableOffset);

    for (i = 0; i < numVectors; i++)
        {
        pCurMsi = &pMsiInfo[i];

        /* Message Vector Control - enable */

        pTableEntry[(i << 2) + PCI_MSIX_MSG_CTRL] =
            htole32(pTableEntry[(i << 2) + PCI_MSIX_MSG_CTRL] &
            ~(PCI_MSIX_MSG_CTRL_MASK));

        /* Message Lower Address */

        pTableEntry[(i << 2) + 0] = htole32(pCurMsi->vecAddr);

        /* Message Higher Address */

        pTableEntry[(i << 2) + 1] = htole32(pCurMsi->vecAddr >> 32);

        /* Message Data */

        pTableEntry[(i << 2) + 2] = htole32(pCurMsi->vecVal);

        PCI_MSI_MSG(100, "vxbPciMsiXProgram(): vecAddr 0x%llx val 0x%x index %d Vector Control 0x%x\n",
                    pCurMsi->vecAddr,
                    pCurMsi->vecVal,
                    i,
                    pTableEntry[(i << 2) + 3]);
        }

    /*
     * Make sure the MSI-X is enabled!
     */

    msixCtl |= PCI_MSIX_CTL_ENABLE;

    VXB_PCI_BUS_CFG_WRITE(pDev, (UINT32)(msixOffset + PCI_MSI_CTL), 2, msixCtl);

    return OK;
    }

/*******************************************************************************
*
* vxPciMsiXAlloc - allocate PCI/PCIe MSI-X interrupt.
*
* This routine allocates a number of PCI/PCIe MSI-X interrupts as specified
* by <cnt>.
*
* RETURNS: number of allocated MSI-X vectors if allocation succeeds
*          or ERROR if allocation fails.
*
* ERRNO
*/

int vxbPciMsiXAlloc
    (
    VXB_DEV_ID pDev,
    UINT32     cnt
    )
    {
    UINT32  i;
    PCI_HARDWARE * pPciDev;
    VXB_DYNC_INT_ENTRY *msiInf;
    VXB_RESOURCE_IRQ * vxbResIrq = NULL;
    VXB_RESOURCE_IRQ * vxbResIrqTmp = NULL;
    VXB_RESOURCE     * pMsiRes = NULL;
    VXB_RESOURCE     * pMsiResTmp = NULL;
    int num=0, allocatNum=0;

    if (vxbPciIntCapChk (pDev, PCI_EXT_CAP_MSIX, cnt) != OK)
        return ERROR;

    if ((msiInf = vxbMemAlloc (cnt * sizeof(VXB_DYNC_INT_ENTRY))) == NULL)
        return ERROR;

    pPciDev = vxbDevIvarsGet (pDev);

    do
        {
        allocatNum = 1 << (ffsMsb(cnt - num) - 1);

        allocatNum = vxbIntAlloc(allocatNum, msiInf + num);

        if (allocatNum <= 0)
            break;

        num += allocatNum;

        if (num == cnt)
            break;
        } while (TRUE);

    if (num <= 0)
        {
        vxbMemFree ((char *)msiInf);
        return ERROR;
        }

    pMsiRes = (VXB_RESOURCE *)vxbMemAlloc(sizeof(VXB_RESOURCE) * num);

    if (pMsiRes == NULL)
        {
        vxbIntFree (num, msiInf);
        vxbMemFree ((char *)msiInf);
        return ERROR;
        }

    vxbResIrq = (VXB_RESOURCE_IRQ *)vxbMemAlloc(sizeof(VXB_RESOURCE_IRQ) * num);

    if (vxbResIrq == NULL)
        {
        vxbIntFree (cnt, msiInf);
        vxbMemFree ((char *)msiInf);
        vxbMemFree ((char *)pMsiRes);
        return ERROR;
        }

    vxbResIrqTmp = vxbResIrq;
    pMsiResTmp = pMsiRes;

    /* add the MSIx resource */

    for (i = 0; i < num; i++)
        {
        vxbResIrqTmp->flag |= VXB_INT_FLAG_DYNAMIC;

        vxbResIrqTmp->hVec = msiInf[i].hVec;
        vxbResIrqTmp->lVec = msiInf[i].lVec;

        vxbResIrqTmp->pVxbIntrEntry = (void *)&msiInf[i];

        pMsiResTmp->id = VXB_RES_ID_CREATE(VXB_RES_IRQ, i + 1);
        pMsiResTmp->pRes = (void *)vxbResIrqTmp;

        (void) vxbResourceAdd (&pPciDev->vxbResList, pMsiResTmp);

        (void) vxbIsrSetIntrEntry (msiInf[i].lVec, (void *)pMsiResTmp);

        vxbResIrqTmp++;
        pMsiResTmp++;
        }

    /* save the dynamic interrupt entry */

    pPciDev->pciDyncIntEntry = msiInf;
    pPciDev->pciDyncIntCnt = num;

    if (vxbPciMsiXProgram (pDev) != OK)
        goto error;

    if (vxbPciIntrExclusiveEnable (pDev, PCI_EXT_CAP_MSIX) != OK)
        goto error;

    return num;

error:
    vxbIntFree (num, msiInf);
    vxbMemFree ((char *)msiInf);
    vxbMemFree ((char *)pMsiRes);
    vxbMemFree ((char *)vxbResIrq);
    pPciDev->pciDyncIntEntry = NULL;
    pPciDev->pciDyncIntCnt = 0;

    return ERROR;
    }

/*******************************************************************************
*
* vxbPciMsiXFree - erase the programmed MSI-X vectors
*
* This routine erases the multiple MSI-X vectors, that can be erased individually
* by the register for each MSI-X vector. This routine erases the data and address
* register specified by "pDynaVec".
*
* RETURNS: ERROR when MSI-X is unavailable, else OK.
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS vxbPciMsiXFree
    (
    VXB_DEV_ID           pDev,
    UINT32               cnt
    )
    {
    VXB_DEV_ID pBridge;
    UINT8 msiOffset = 0;
    UINT16 msiCtl;
    UINT32 table, tableOffset, *pTableEntry;
    ULONG tableBase =0;
    int i;
    PCI_HARDWARE * pPciDev;
    VXB_RESOURCE_IRQ * vxbResIrq = NULL;
    VXB_RESOURCE     * pMsixRes = NULL;
    VXB_DYNC_INT_ENTRY * dynaVec = NULL;

    pPciDev = vxbDevIvarsGet (pDev);
    pBridge = vxbDevParent (pDev);

    if (pBridge== NULL)
        return ERROR;

    dynaVec = pPciDev->pciDyncIntEntry;

    /*
     * If vxbPciMsiXAlloc() failed (either because the dynamic
     * interrupt component is missing or the interrupt controller
     * doesn't support dynamic interrupts), then the pciDyncIntEntry
     * field will be NULL. We should check for this before proceeding.
     */

    if (dynaVec == NULL)
        return (ERROR);

    if (vxbPciIntCapChk (pDev, PCI_EXT_CAP_MSIX, cnt) != OK)
        return ERROR;

    vxbIntFree (cnt, dynaVec);

    for (i = 0; i < cnt; i++)
        (void) vxbIsrSetIntrEntry (dynaVec[i].lVec, NULL);

    /* remove the MSIx resource */

    for (i = 1; i < cnt; i++)
        {
        pMsixRes = vxbResourceAlloc(pDev, VXB_RES_IRQ, (UINT16)(i + 1));
        if (pMsixRes == NULL)
            return ERROR;

        (void) vxbResourceRemove(&pPciDev->vxbResList, pMsixRes);
        }

    if ((pMsixRes = vxbResourceAlloc(pDev, VXB_RES_IRQ, 1)) == NULL)
        return ERROR;

    (void) vxbResourceRemove (&pPciDev->vxbResList, pMsixRes);
    vxbResIrq = pMsixRes->pRes;
    vxbMemFree(vxbResIrq);
    vxbMemFree(pMsixRes);

    /* Capabilities List Implemented: Get first capability ID */

    (void) vxbPciExtCapFind (pDev, PCI_EXT_CAP_MSIX, &msiOffset);

    if (msiOffset == 0)
        return(ERROR);

    msiOffset = (UINT8)(msiOffset - PCI_MSI_CTL);

    /*
     * Software reads the Table Size field from the Message Control
     * register to determine the MSI-X Table size. The field encodes
     * the number of table entries as N-1, so software must add 1 to
     * the value read from the field to calculate the number of table
     * entries N. MSI-X supports a maximum table size of 2048 entries.
     */

    (void) VXB_PCI_CFG_READ (pBridge, pPciDev, msiOffset + PCI_MSIX_CTL, 2, &msiCtl);

    msiCtl |= PCI_MSIX_CTL_FUNCTION_MASK;
    msiCtl = (UINT16)(msiCtl & ~(PCI_MSIX_CTL_ENABLE));

    (void) VXB_PCI_CFG_WRITE(pBridge, pPciDev, (UINT32)(msiOffset + PCI_MSIX_CTL), 2, msiCtl);

    /*
     * Software calculates the base address of the MSI-X Table by reading
     * the 32-bit value from the Table Offset / BIR register, masking
     * off the lower 3 BIR bits, and adding the remaining 8-byte aligned
     * 32-bit offset to the address taken from the Base Address register
     * indicated by the BIR.
     */

    (void) VXB_PCI_CFG_READ (pBridge, pPciDev, msiOffset + PCI_MSIX_TABLE, 4, &table);

    tableOffset = (table & PCI_MSIX_TABLE_OFFSET_MASK);

    pMsixRes = vxbResourceAlloc (pDev, VXB_RES_MEMORY, table & PCI_MSIX_TABLE_BIR_MASK);

    if (pMsixRes == NULL || pMsixRes->pRes == NULL)
        return ERROR;

    tableBase = ((VXB_RESOURCE_ADR*)pMsixRes->pRes)->virtual;

    pTableEntry = (UINT32 *)(tableBase + tableOffset);

    for (i = 0; i < cnt; i++)
        {
        /* Message Lower Address */
        pTableEntry[(i << 2) + 0] = 0;
        /* Message Higher Address */
        pTableEntry[(i << 2) + 1] = 0;
        /* Message Data */
        pTableEntry[(i << 2) + 2] = 0;
        /* Message Vector Control */
        pTableEntry[(i << 2) + 3] = 1;
        }

    if (vxbPciIntrExclusiveEnable (pDev, VXB_INT_PCI_INTX) != OK)
        return (ERROR);

    vxbMemFree ((char *)dynaVec);
    pPciDev->pciDyncIntEntry = NULL;
    pPciDev->pciDyncIntCnt = 0;

    return OK;
    }

#ifdef PCI_MSI_DEBUG
/*******************************************************************************
*
* vxbPciMsixShow - print PCI device MSI-X information
*
* This routine prints MSI-X information about the PCI devices.
*
* RETURNS: void.
*
* ERRNO
*
* \NOMANUAL
*/

LOCAL void vxbPciMsixShow
    (
    VXB_DEV_ID  pDev
    )
    {
    UINT16 ctlReg;
    UINT32 tableSize;
    UINT32 table, tableOffset, *tableEntry;
    UINT32 pba, pbaOffset;
    UINT64 * pbaEntry;
    UINT32 addrHi, addrLo, data, control;
    int i;
    ULONG tableAddr, pbaAddr;
    VXB_RESOURCE * pRes;
    VXB_RESOURCE_ADR * pResAdr = NULL;
    PCI_HARDWARE * pPciDev = vxbDevIvarsGet(pDev);
    UINT8 msixOffset=0;

    (void) vxbPciExtCapFind (vxbDevParent (pDev), pPciDev, PCI_EXT_CAP_MSIX, &msixOffset);

    if (msixOffset == 0)
        {
        printf("NO MSI-X!\r\n");
        return;
        }

    /*
     * This offset is for MSIX_CTL which is 2 bytes, so must be adjusted
     * to get the base offset
     */

    msixOffset = (UINT8)(msixOffset - PCI_MSIX_CTL);

    printf ("MSI-X msixOffset 0x%x\n",msixOffset);

    VXB_PCI_BUS_CFG_READ (pDev, msixOffset + PCI_MSIX_CTL, 2, ctlReg);

    tableSize = (ctlReg & PCI_MSIX_CTL_TABLE_SIZE) + 1;

    VXB_PCI_BUS_CFG_READ (pDev, msixOffset + PCI_MSIX_TABLE, 4, table);

    tableOffset = (table & PCI_MSIX_TABLE_OFFSET_MASK);

    pRes = vxbResourceAlloc(pDev, VXB_RES_MEMORY, table & PCI_MSIX_TABLE_BIR_MASK);

    if (pRes->pRes == NULL)
        return;

    pResAdr = (VXB_RESOURCE_ADR *)pRes->pRes;

    tableAddr = (ULONG)pResAdr->virtual;

    tableEntry = (UINT32 *)((~PCI_BAR_ALL_MASK & tableAddr) + tableOffset);

    printf("msixCapabShow(): bar%d tableAddr is 0x%llx virtual 0x%lx handle is %p"
                                " tableSize %x tableOffset %x\n",
                                table & PCI_MSIX_TABLE_BIR_MASK,
                                pResAdr->start,
                                pResAdr->virtual,
                                pResAdr->pHandle,tableSize,tableOffset);

    VXB_PCI_BUS_CFG_READ (pDev, msixOffset + PCI_MSIX_PBA, 4, pba);

    pbaOffset = (pba & PCI_MSIX_PBA_OFFSET_MASK);

    pRes = vxbResourceAlloc(pDev, VXB_RES_MEMORY, pba & PCI_MSIX_PBA_BIR_MASK);

    if (pRes->pRes == NULL)
        return;

    pResAdr = (VXB_RESOURCE_ADR *)pRes->pRes;

    pbaAddr = (ULONG)pResAdr->virtual;

    pbaEntry = (UINT64*)(pbaAddr + pbaOffset);

    printf ("        MSI-X table @%p, pba @%p control 0x%x table 0x%x\n",
            tableEntry, pbaEntry, ctlReg, table);

    printf ("        MSI-X %s with %d vectors\n",
            (ctlReg & PCI_MSIX_CTL_ENABLE) == 0x00 ? "Disabled" : "Enabled",
            tableSize);

    for (i = 0; i < tableSize; i++)
        {
        addrLo = le32toh(tableEntry[0]);
        addrHi = le32toh(tableEntry[1]);
        data = le32toh(tableEntry[2]);
        control = le32toh(tableEntry[3]);

        printf ("\t        MSI-X Table[%d] @%p - "
                "Vector Control: 0x%x Address: 0x%x:%x Data 0x%x\n",
                i, tableEntry, control, addrHi, addrLo, data);

        tableEntry += 4;
        }

    for (i = 0; i < tableSize;)
        {
        UINT64 pba_bits = le32toh(pbaEntry[i]);
        printf ("\t        PBA entries %d through %d: 0x%llx\n", i, i + 63, pba_bits);
        i += 64;
        }
    }
#endif /* PCI_MSI_DEBUG */

