/* vxbSdhcCtrl.c - SDHC host controller common driver library */

/*
 * Copyright (c) 2012 - 2013, 2015-2018 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01m,03sep18,d_l  update SDHC_DBG function name (VXW6-86922)
01l,12jul18,j_x  masking member busWidth of structure SD_HOST_SPEC for binary 
                 compatible (VXW6-86766)
01k,19sep17,d_l  add 10bit clock mode support. (VXW6-86304)
01j,14jun17,ffu  clear warnings (VXW6-85661)
01i,01mar17,d_l  add bus-width selection. (VXW6-86099)
                 fix forward null issue.
01h,26oct16,y_y  reduece data timeout for commands with busy signal. (VXW6-85760)
01g,25jan16,myt  add support for direct bio for SD/eMMC (VXW6-85000)
01f,26oct16,myt  add support of reliable write for eMMC (VXW6-84383)
01e,07jul15,j_x  add vxbDmaBufMapSync before and after DMA operation (VXW6-84648)
01d,06jan15,myt  fix the error of DMA address update (VXW6-83697)
01c,20nov13,e_d  remove APIs wrongly published. (WIND00444661)
01b,29oct13,e_d  fix prevent issue. (WIND00440964)
01a,28aug12,e_d  written.
*/

/*
DESCRIPTION

This is the vxbus compliant Secure Digital Host Controller (SDHC)
driver library which implements the functionality specified in
"SD Specifications Part A2 SD Host Controller Simplified Specification
Version 2.00"

The SDHC provides an interface between the host controller and
SD/SDIO target device.

This file is not one vxbus driver's file. And it implements some routines as
stand SDHC controller. But for a lot of SDHC controllers, they have a little
different with stand SDHC controller. If write one new SDHC controller driver,
we only write some routines that different with stand SDHC. Others we can call
stand routine in this file.

SEE ALSO: vxBus, xbd
\tb "SD Specifications Part A1 Physical Layer Simplified Specification Version 2.00"
\tb "SD Specifications Part A2 SD Host Controller Simplified Specification Version 2.00"
*/

/* includes */

#include <vxWorks.h>
#include <stdio.h>
#include <semLib.h>
#include <sysLib.h>
#include <taskLib.h>
#include <vxBusLib.h>
#include <cacheLib.h>
#include <string.h>
#include <hwif/vxbus/vxBus.h>
#include <hwif/vxbus/hwConf.h>
#include <hwif/util/vxbParamSys.h>

#include <hwif/vxbus/vxbPciLib.h>
#include <hwif/vxbus/hwConf.h>
#include <drv/pci/pciConfigLib.h>
#include <drv/pci/pciIntLib.h>

#include <hwif/vxbus/vxbSdLib.h>
#include <../src/hwif/h/vxbus/vxbAccess.h>

/*#include <../src/hwif/h/sd/vxbSdhcCtrl.h>*/
#include <../src/hwif/h/vxbus/vxbPciBus.h>

/* defines */

#ifdef  SDHC_DBG_ON

#ifdef  LOCAL
#undef  LOCAL
#define LOCAL
#endif

#define SDHC_DBG_IRQ            0x00000001
#define SDHC_DBG_RW             0x00000002
#define SDHC_DBG_XBD            0x00000004
#define SDHC_DBG_ERR            0x00000008
#define SDHC_DBG_ALL            0xffffffff
#define SDHC_DBG_OFF            0x00000000

UINT32 sdhcDbgMask = SDHC_DBG_OFF;

IMPORT FUNCPTR _func_logMsg;

#define SDHC_DBG(mask, string, a, b, c, d, e, f)        \
    if ((sdhcDbgMask & mask) || (mask == SDHC_DBG_ALL))  \
        if (_func_logMsg != NULL) \
           (* _func_logMsg)(string, a, b, c, d, e, f)
#else
#define SDHC_DBG(mask, string, a, b, c, d, e, f)
#endif  /* SDHC_DBG_ON */

/*
 * File system use cacheDmaMalloc to alloc buffer (file bio.c, bio_alloc() routine),
 * so there is no need for driver to use cacheFlush and cacheInvalidate to keep buffer's
 * coherence. When define SDHC_FS_UNCACHE_BUFFER in this driver, we will ignore the
 * the sync operaton of DMA buffer.
 */

#define SDHC_FS_UNCACHE_BUFFER

/* externs */

IMPORT UCHAR erfLibInitialized;
IMPORT void vxbUsDelay (int);
IMPORT void vxbMsDelay (int);

BOOL mmcHrfsInclude = FALSE; 

/*******************************************************************************
*
* sdhcInstInit - first level initialization routine of sdhc device
*
* This routine performs the first level initialization of the sdhc device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void sdhcCtrlInstInit
    (
    VXB_DEVICE_ID pInst
    )
    {

    /* get the next available unit number */

    if (pInst->busID == VXB_BUSID_PCI)
        vxbNextUnitGet (pInst);
    }

/*******************************************************************************
*
* sdhcInstInit2 - second level initialization routine of sdhc device
*
* This routine performs the second level initialization of the sdhc device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void sdhcCtrlInstInit2
    (
    VXB_DEVICE_ID pInst
    )
    {
    SDHC_DEV_CTRL * pDrvCtrl;
    struct hcfDevice * pHcf;
    FUNCPTR clkFunc = NULL;
    UINT16 i;
    int flag;
    bus_size_t alignment;

    pDrvCtrl = (SDHC_DEV_CTRL *)malloc (sizeof(SDHC_DEV_CTRL));
    if (pDrvCtrl == NULL)
        return;

    bzero ((char *)pDrvCtrl, sizeof(SDHC_DEV_CTRL));

    pDrvCtrl->sdHostCtrl.pDev = pInst;
    pDrvCtrl->sdHostCtrl.attached = FALSE;
    pDrvCtrl->sdHostCtrl.isInsert = FALSE;
    pInst->pDrvCtrl = pDrvCtrl;

    for (i = 0; i < VXB_MAXBARS; i++)
        {
        if (pInst->regBaseFlags[i] == VXB_REG_MEM)
            break;
        }

    if (i == VXB_MAXBARS)
        {
        free (pDrvCtrl);
        return;
        }

    pDrvCtrl->regBase = pInst->pRegBase[i];
    vxbRegMap (pInst, i, &pDrvCtrl->regHandle);

    pDrvCtrl->sdHostCtrl.sdHostDmaParentTag = vxbDmaBufTagParentGet (pInst, 0);

#ifndef  _WRS_CONFIG_LP64
    if (!(pDrvCtrl->flags & SDHC_HW_SNOOP))
        {
        flag = VXB_DMABUF_ALLOCNOW | VXB_DMABUF_NOCACHE;
        alignment = _CACHE_ALIGN_SIZE; 
        } 
    else
        {
        flag = VXB_DMABUF_MAPCREATE_NOBOUNCE_BUF | VXB_DMABUF_CACHESNOOP_BUS;
        alignment = 1;
        }
#else /* _WRS_CONFIG_LP64 */
    flag = VXB_DMABUF_ALLOCNOW | VXB_DMABUF_NOCACHE;
    alignment = _CACHE_ALIGN_SIZE; 
#endif /* _WRS_CONFIG_LP64 */

    pDrvCtrl->sdHostCtrl.sdHostDmaTag = vxbDmaBufTagCreate
                                              (pInst,
                                              pDrvCtrl->sdHostCtrl.sdHostDmaParentTag, /* parent */
                                              alignment,                               /* alignment */
                                              0,                                       /* boundary */
                                              VXB_SPACE_MAXADDR_32BIT,                 /* lowaddr */
                                              VXB_SPACE_MAXADDR,                       /* highaddr */
                                              NULL,                                    /* filter */
                                              NULL,                                    /* filterarg */
                                              SDHC_MAX_RW_SECTORS * SDMMC_BLOCK_SIZE,  /* max size */
                                              1,                                       /* nSegments */
                                              SDHC_MAX_RW_SECTORS * SDMMC_BLOCK_SIZE,  /* max seg size */
                                              flag,                                    /* flags */
                                              NULL,                                    /* lockfunc */
                                              NULL,                                    /* lockarg */
                                              NULL);
    if (pDrvCtrl->sdHostCtrl.sdHostDmaTag == NULL)
        {
        SDHC_DBG (SDHC_DBG_ERR, "sdHostDmaTag create fault\n",
                  0, 0, 0, 0, 0, 0);
        free (pDrvCtrl);
        return;
        }

    pHcf = (struct hcfDevice *)hcfDeviceGet (pInst);

    if (pHcf != NULL)
        {

        /*
         * resourceDesc {
         * The clkFreq resource specifies clock
         * frequency of SDHC base clock. }
         */

        /* Need not check return status at here */

        (void)devResourceGet (pHcf, "clkFreq", HCF_RES_ADDR,
                             (void *)&clkFunc);

        if (clkFunc)
            {
            pDrvCtrl->sdHostCtrl.curClkFreq = (*clkFunc) ();
            }

        /*
         * resourceDesc {
         * The dmaMode resource specifies the DMA
         * mode of eSDHC. If this property is not
         * explicitly specified, the driver uses
         * SDMA by default. }
         */

        /* Need not check return status at here */

        (void)devResourceGet (pHcf, "dmaMode", HCF_RES_INT,
                             (void *)&(pDrvCtrl->sdHostCtrl.dmaMode));

        /*
         * resourceDesc {
         * The polling resource specifies whether
         * the driver uses polling mode or not.
         * If this property is not explicitly
         * specified, the driver uses interrupt
         * by default. }
         */

        /* Need not check return status at here */

        (void)devResourceGet (pHcf, "polling", HCF_RES_INT,
                             (void *)&(pDrvCtrl->sdHostCtrl.polling));

        /*
         * resourceDesc {
         * The flags resource specifies various
         * controll flags of the host controller. }
         */

        /* Need not check return status at here */

        (void)devResourceGet (pHcf, "flags", HCF_RES_INT,
                             (void *)&(pDrvCtrl->flags));

        /*
         * resourceDesc {
         * The directBio resource specifies if the direct bio mode 
         * is supported by the driver. }
         */
 
        /* Need not check return status at here */
        (void)devResourceGet (pHcf, "directBio", HCF_RES_INT,
                             (void *)&(pDrvCtrl->sdHostCtrl.directBio));

#ifdef SD_BUSWIDTH

        /* 
         * All soc and eMMC devices support 4 bit width. And some of them,
         * such as Zynq 7k SOC, only supports 4 bit width.  Therefore
         * if BSP doesn't give us a bus-width, set it to 4 bit width 
         */

        /*
         * resourceDesc {
         * This resource specifies bit width used by eMMC devices. Host controller 
         * selects maximum bus-width (4bit or 8bit) it supported for eMMC device. 
         * This parameter doesn't influence SD cards. }
         */

        if (devResourceGet (pHcf, "bus-width", HCF_RES_INT,
                             (void *)
                             &(pDrvCtrl->sdHostCtrl.sdHostSpec.busWidth)) != OK)
            {
            pDrvCtrl->sdHostCtrl.sdHostSpec.busWidth = 4;
            }
 #endif
          }

    /* initialize SDIO configuration library */

    pDrvCtrl->pIntInfo = (struct vxbSdioInt *)malloc(sizeof(struct vxbSdioInt));
    if(pDrvCtrl->pIntInfo == NULL)
        return;

    vxbSdioIntLibInit(pDrvCtrl->pIntInfo);

    if (pDrvCtrl->flags & SDHC_FIFO_ENDIANESS_REVERSE)
       {

        /*
         * If endianess of internal buffer data port is differernt
         * from CPU's endiness, we swap the handle.
         */

        pDrvCtrl->pioHandle = (void *)
                               (VXB_HANDLE_SWAP ((ULONG)pDrvCtrl->regHandle));
        }

    pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdBusWidthSetup = sdhcCtrlBusWidthSetup;
    pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdCardWpCheck = sdhcCtrlCardWpCheck;
    pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdCardInsertSts = sdhcCtrlCardInsertSts;
    pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdCmdPrepare = sdhcCtrlCmdPrepare;
    pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdHostCtrlInit = sdhcCtrlInit;
    pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdClkFreqSetup = sdhcCtrlClkFreqSetup;
    pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdIsr = sdhcCtrlIsr;
    pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdVddSetup = NULL;
    pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdResumeSet = NULL;

    /* Need not check return status at here */

    (void)vxbBusAnnounce (pInst, VXB_BUSID_SD);
    }

/*******************************************************************************
*
* sdhcInstConnect - third level initialization routine of sdhc device
*
* This routine performs the third level initialization of the sdhc device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS sdhcCtrlInstConnect
    (
    SD_HOST_CTRL * pSdHostCtrl
    )
    {
    STATUS rc;
    UINT32 val;

    /*
     * The devChange semaphore is used by the interrupt service routine
     * to inform the card monitor task that a state change has occurred.
     */

    pSdHostCtrl->devChange = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
    if (pSdHostCtrl->devChange == NULL)
        {
        SDHC_DBG (SDHC_DBG_ERR, "semBCreate failed for devChange\n",
                      0, 0, 0, 0, 0, 0);
        return ERROR;
        }

    pSdHostCtrl->cmdDone = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
    if (pSdHostCtrl->cmdDone == NULL)
        {
        SDHC_DBG (SDHC_DBG_ERR, "semBCreate failed for cmdDone\n",
                      0, 0, 0, 0, 0, 0);
        goto err;
        }

    pSdHostCtrl->dataDone = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
    if (pSdHostCtrl->dataDone == NULL)
        {
        SDHC_DBG (SDHC_DBG_ERR, "semBCreate failed for dataDone\n",
                      0, 0, 0, 0, 0, 0);
        goto err;
        }

    pSdHostCtrl->pioReady = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
    if (pSdHostCtrl->pioReady == NULL)
        {
        SDHC_DBG (SDHC_DBG_ERR, "semBCreate failed for pioReady\n",
                      0, 0, 0, 0, 0, 0);
        goto err;
        }

    pSdHostCtrl->hostDevSem = semMCreate(SEM_Q_PRIORITY |
                                         SEM_DELETE_SAFE |
                                         SEM_INVERSION_SAFE);
    if (pSdHostCtrl->hostDevSem == NULL)
        {
        SDHC_DBG (SDHC_DBG_ERR, "semBCreate failed for pioReady\n",
                      0, 0, 0, 0, 0, 0);
        goto err;
        }

    /* per-device init */

    if (pSdHostCtrl->sdHostOps.vxbSdHostCtrlInit != NULL)
        {
        rc = pSdHostCtrl->sdHostOps.vxbSdHostCtrlInit(pSdHostCtrl->pDev);
        if (rc == ERROR)
            goto err;
        }

    return OK;

err:
    if (pSdHostCtrl->dataDone != NULL)
        {
        if (semDelete(pSdHostCtrl->dataDone) != OK)
            SDHC_DBG (SDHC_DBG_ERR, "semDelete failed for dateDone\n",
                      0, 0, 0, 0, 0, 0);
        }

    if (pSdHostCtrl->cmdDone != NULL)
        {
        if (semDelete(pSdHostCtrl->cmdDone) != OK)
            SDHC_DBG (SDHC_DBG_ERR, "semDelete failed for cmdDone\n",
                      0, 0, 0, 0, 0, 0);
        }
    if (pSdHostCtrl->devChange != NULL)
        {
        if (semDelete(pSdHostCtrl->devChange) != OK)
            SDHC_DBG (SDHC_DBG_ERR, "semDelete failed for devChange\n",
                      0, 0, 0, 0, 0, 0);
        }
    if (pSdHostCtrl->pioReady != NULL)
        {
        if (semDelete(pSdHostCtrl->pioReady) != OK)
            SDHC_DBG (SDHC_DBG_ERR, "semDelete failed for devChange\n",
                      0, 0, 0, 0, 0, 0);
        }
    if (pSdHostCtrl->hostDevSem != NULL)
        {
        if (semDelete(pSdHostCtrl->hostDevSem) != OK)
            SDHC_DBG (SDHC_DBG_ERR, "semDelete failed for devChange\n",
                      0, 0, 0, 0, 0, 0);
        }
    return ERROR;
    }

/*******************************************************************************
*
* sdhcCtrlInit - eSDHC per device specific initialization
*
* This routine performs per device specific initialization of eSDHC.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS sdhcCtrlInit
    (
    VXB_DEVICE_ID pInst
    )
    {
    SDHC_DEV_CTRL * pDrvCtrl = (SDHC_DEV_CTRL *)pInst->pDrvCtrl;
    UINT32 hostCap;

    /* reset eSDHC controller */

    if (pDrvCtrl == NULL)
        return ERROR;

    CSR_WRITE_4 (pInst, SDHC_SYSCTL, SYSCTL_RSTA);

    while (CSR_READ_4 (pInst, SDHC_SYSCTL) & SYSCTL_RSTA);

    vxbMsDelay(1);

    /* disable all interrupts */

    CSR_WRITE_4 (pInst, SDHC_IRQSIGEN, 0);
    CSR_WRITE_4 (pInst, SDHC_IRQSTATEN, 0);
    CSR_WRITE_4 (pInst, SDHC_IRQSTAT, IRQ_ALL);

    /* enable all clock domains */

    CSR_SETBIT_4 (pInst, SDHC_SYSCTL,
                  SYSCTL_PEREN | SYSCTL_HCKEN | SYSCTL_IPGEN);

    /* setup data transfer width to 1-bit */

    if (pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdBusWidthSetup != NULL)
        pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdBusWidthSetup(pInst, SDMMC_BUS_WIDTH_1BIT);

    /* get host controller version */

    pDrvCtrl->sdHostCtrl.version = (CSR_READ_4 (pInst, SDHC_HOSTVER) >> 16) & 0xffff;

    /* get host controller capabilities */

    hostCap = CSR_READ_4 (pInst, SDHC_HOSTCAPBLT);

    /* determind DMA mode supported */

    if (!(hostCap & HOSTCAPBLT_DMAS))
        {
        if (pDrvCtrl->sdHostCtrl.dmaMode != SDHC_DMA_MODE_PIO)
            pDrvCtrl->sdHostCtrl.dmaMode = SDHC_DMA_MODE_PIO;
        }

    else
        {
        if ((pDrvCtrl->sdHostCtrl.dmaMode != SDHC_DMA_MODE_SDMA) &&
            (pDrvCtrl->sdHostCtrl.dmaMode != SDHC_DMA_MODE_PIO))
            {
            SDHC_DBG (SDHC_DBG_ERR,
                      "ADMA1/2 not supported by this driver, using SDMA.\n",
                      0, 0, 0, 0, 0, 0);
            pDrvCtrl->sdHostCtrl.dmaMode = SDHC_DMA_MODE_SDMA;
            }
        }

    /* determine voltage supported */

    if (hostCap & HOSTCAPBLT_VS30)
        pDrvCtrl->sdHostCtrl.capbility |= (OCR_VDD_VOL_29_30 | OCR_VDD_VOL_30_31);
    if (hostCap & HOSTCAPBLT_VS33)
        pDrvCtrl->sdHostCtrl.capbility |= (OCR_VDD_VOL_32_33 | OCR_VDD_VOL_33_34);

    /* high capacity SD card is supported */

    pDrvCtrl->sdHostCtrl.capbility |= OCR_CARD_CAP_STS;

    /* determine high speed SD card supported */

    if (hostCap & HOSTCAPBLT_HSS)
        pDrvCtrl->highSpeed = TRUE;

    /* determine the highest SD bus voltage suppored by the controller */

    if (hostCap & HOSTCAPBLT_VS33)
        pDrvCtrl->sdHostCtrl.vdd = PROCTL_SDBUS_VOLT_3P3;
    else if (hostCap & HOSTCAPBLT_VS30)
        pDrvCtrl->sdHostCtrl.vdd = PROCTL_SDBUS_VOLT_3P0;
    else
        pDrvCtrl->sdHostCtrl.vdd = PROCTL_SDBUS_VOLT_1P8;

    if (vxbDmaBufMapCreate (pInst, pDrvCtrl->sdHostCtrl.sdHostDmaTag, 0,
                            &(pDrvCtrl->sdHostCtrl.sdHostDmaMap)) == NULL)
        return ERROR;
    return OK;
    }

/*******************************************************************************
*
* sdhcIsr - interrupt service routine
*
* This routine handles interrupts of eSDHC.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS sdhcCtrlIsr
    (
    VXB_DEVICE_ID pDev
    )
    {
    SDHC_DEV_CTRL * pDrvCtrl = (SDHC_DEV_CTRL *)pDev->pDrvCtrl;
    UINT32 intSts;
    UINT32 dmaAddr;

    if (pDrvCtrl == NULL)
        return ERROR;

    intSts = CSR_READ_4 (pDev, SDHC_IRQSTAT);
    pDrvCtrl->intSts = intSts;

    if (intSts == 0)
        return ERROR;

    /* clear interrupts */

    CSR_WRITE_4 (pDev, SDHC_IRQSTAT, intSts);

    /* card insertion */

    if (intSts & IRQ_CINS)
        {
        SDHC_DBG (SDHC_DBG_IRQ, "card insert interrupt \n",
                  0, 0, 0, 0, 0, 0);

        /* turn off card insertion interrup */

        CSR_CLRBIT_4 (pDev, SDHC_IRQSIGEN, IRQ_CINS);
        CSR_CLRBIT_4 (pDev, SDHC_IRQSTATEN, IRQ_CINS);
        CSR_SETBIT_4 (pDev, SDHC_IRQSIGEN, IRQ_CRM);
        CSR_SETBIT_4 (pDev, SDHC_IRQSTATEN, IRQ_CRM);

        pDrvCtrl->intMask &= ~IRQ_CINS;
        pDrvCtrl->intMask |= IRQ_CRM;

        semGive (pDrvCtrl->sdHostCtrl.devChange);
        }

    /* card removal */

    if (intSts & IRQ_CRM)
        {
        SDHC_DBG (SDHC_DBG_IRQ, "card removal interrupt \n",
                  0, 0, 0, 0, 0, 0);

        /* turn off card removal interrupt */

        CSR_CLRBIT_4 (pDev, SDHC_IRQSIGEN, IRQ_CRM);
        CSR_CLRBIT_4 (pDev, SDHC_IRQSTATEN, IRQ_CRM);
        CSR_SETBIT_4 (pDev, SDHC_IRQSIGEN, IRQ_CINS);
        CSR_SETBIT_4 (pDev, SDHC_IRQSTATEN, IRQ_CINS);

        pDrvCtrl->intMask &= ~IRQ_CRM;
        pDrvCtrl->intMask |= IRQ_CINS;

        semGive (pDrvCtrl->sdHostCtrl.devChange);
        }

    /* command interrupts */

    if (intSts & IRQ_CMD)
        {
        semGive (pDrvCtrl->sdHostCtrl.cmdDone);
        }

    /* DMA interrupt */

    if ((intSts & IRQ_DINT) != 0x0)
        {
        SDHC_DBG (SDHC_DBG_IRQ, "DMA int 0x%08x\n",
                  CSR_READ_4(pDev, SDHC_DSADDR), 0, 0, 0, 0, 0);

        CSR_WRITE_4 (pDev, SDHC_DSADDR, pDrvCtrl->sdmaAddr);

        /* setup next SDMA address */
        pDrvCtrl->sdmaAddr += pDrvCtrl->sdmaAddrBound;

        if (!(intSts & IRQ_TC))
            return (OK);
        }

    /* data interrupts */

    if (intSts & IRQ_DATA)
        {
        SDHC_DBG (SDHC_DBG_IRQ, "data interrupt \n",
                  0, 0, 0, 0, 0, 0);
        semGive (pDrvCtrl->sdHostCtrl.dataDone);
        }

    /* PIO mode */

    if (intSts & (IRQ_BRR |IRQ_BWR))
        {
        SDHC_DBG (SDHC_DBG_IRQ, "CMD buffer r/w ready\n",
                  0, 0, 0, 0, 0, 0);

        semGive (pDrvCtrl->sdHostCtrl.pioReady);
        }

    /* other errors */

    if (intSts & IRQ_AC12E)
        {
        SDHC_DBG (SDHC_DBG_ERR, "Auto CMD12 error\n",
                  0, 0, 0, 0, 0, 0);
        }
    return (OK);
    }

/*******************************************************************************
*
* sdhcCmdPrepare - prepare the command to be sent
*
* This routine prepares the command to be sent.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS sdhcCtrlCmdPrepare
    (
    VXB_DEVICE_ID pDev,
    SD_CMD * pSdCmd
    )
    {
    SD_HOST_CTRL * pSdHostCtrl;
    SDHC_DEV_CTRL * pDrvCtrl = (SDHC_DEV_CTRL *)pDev->pDrvCtrl;
    UINT32 xferTyp = 0;
    UINT32 i = 0;

    if (pDrvCtrl == NULL)
        return ERROR;
    else
        pSdHostCtrl = (SD_HOST_CTRL *)pDrvCtrl;

    /* check PRSSTAT[CDIHB] and PRSSTAT[CIHB] before writing to XFERTYP */

    while ((CSR_READ_4(pDev, SDHC_PRSSTAT) & PRSSTAT_CDIHB) ||
           (CSR_READ_4(pDev, SDHC_PRSSTAT) & PRSSTAT_CIHB))
        {
        vxbUsDelay(1);
        i++;
        if (i > 10000)
            {
            SDHC_DBG (SDHC_DBG_ERR, "CIHB/CDIHB busy \n", 1, 2, 3, 4, 5, 6);
            return ERROR;
            }
        }

    i = 0;

    while (CSR_READ_4(pDev, SDHC_PRSSTAT) & PRSSTAT_DLA)
        {
        vxbUsDelay(1);
        i++;
        if (i > 10000)
            {
            SDHC_DBG (SDHC_DBG_ERR, "DLA busy \n", 1, 2, 3, 4, 5, 6);
            return ERROR;
            }
        }

    /* setup command */

    xferTyp |= XFERTYP_CMDINX (pSdCmd->cmdIdx);

    /* setup command fields */

    if (pSdCmd->rspType & SDMMC_CMD_RSP_CMDIDX)
        xferTyp |= XFERTYP_CICEN;

    if (pSdCmd->rspType & SDMMC_CMD_RSP_CRC)
        xferTyp |= XFERTYP_CCCEN;

    if (pSdCmd->rspType & SDMMC_CMD_RSP_LEN136)
        xferTyp |= XFERTYP_RSPTYP_LEN136;
    else if (pSdCmd->rspType & SDMMC_CMD_RSP_BUSY)
        xferTyp |= XFERTYP_RSPTYP_LEN48B;
    else if (pSdCmd->rspType & SDMMC_CMD_RSP_LEN48)
        xferTyp |= XFERTYP_RSPTYP_LEN48;

    /* setup data fields */

    if (pSdCmd->hasData)
        {
        xferTyp |= XFERTYP_DATA_PRESENT;

        /* direction */

        if (pSdCmd->cmdData.isRead == TRUE)
            {
            xferTyp |= XFERTYP_DIR_READ;

            vxbDmaBufMapSync (pDev,
                      pSdHostCtrl->sdHostDmaTag,
                      pSdHostCtrl->sdHostDmaMap,
                      0,
                      pSdCmd->cmdData.blkNum * pSdCmd->cmdData.blkSize,
                      _VXB_DMABUFSYNC_DMA_PREREAD);
            }

        else if (pSdHostCtrl->dmaMode != SDHC_DMA_MODE_PIO)
            {
            vxbDmaBufMapSync (pDev,
                              pSdHostCtrl->sdHostDmaTag,
                              pSdHostCtrl->sdHostDmaMap,
                              0,
                              pSdCmd->cmdData.blkNum * pSdCmd->cmdData.blkSize,
                             _VXB_DMABUFSYNC_DMA_PREWRITE);
            }

        if (pSdHostCtrl->dmaMode != SDHC_DMA_MODE_PIO)
            xferTyp |= XFERTYP_DMAEN;

        /* multipule block transfer */

        if (pSdCmd->cmdData.blkNum > 1)
            {
            xferTyp |= (XFERTYP_MULTI_BLK | XFERTYP_BCEN);

            /* automatically issue CMD12 */

            if ((pSdCmd->rspType & SDMMC_CMD_AUTOCMD12) != 0x0)
                xferTyp |= XFERTYP_AC12EN;
            }

        if (pSdHostCtrl->dmaMode != SDHC_DMA_MODE_PIO)
            {
            pDrvCtrl->sdmaAddr = (UINT32)(pSdHostCtrl->sdHostDmaMap->fragList[0].frag);

            CSR_WRITE_4 (pDev, SDHC_DSADDR, pDrvCtrl->sdmaAddr);
            }
        else
            {
            pDrvCtrl->pioDesc.bufOri = pSdCmd->cmdData.buffer;
            pDrvCtrl->pioDesc.bufCur = pSdCmd->cmdData.buffer;
            pDrvCtrl->pioDesc.blkSize = pSdCmd->cmdData.blkSize;
            }

        CSR_WRITE_4 (pDev, SDHC_BLKATTR,
                     (pSdCmd->cmdData.blkNum << BLKATTR_CNT_SHIFT) |
                     pSdCmd->cmdData.blkSize |
                     (BLKBUFFER_SIZE_512K << BLKBUFFER_SIZE_SHIFT));

        /* save SDMA next address and boundary */
        if (pSdHostCtrl->dmaMode != SDHC_DMA_MODE_PIO)
            {
            pDrvCtrl->sdmaAddrBound = 1 << (((CSR_READ_4 (pDev, SDHC_BLKATTR) >> BLKBUFFER_SIZE_SHIFT) & 0x7) + 12);
            pDrvCtrl->sdmaAddr += pDrvCtrl->sdmaAddrBound;
            pDrvCtrl->sdmaAddr = (UINT32)(ROUND_DOWN (pDrvCtrl->sdmaAddr, pDrvCtrl->sdmaAddrBound));
            }

        /* setup transfer timeouts */

        CSR_CLRBIT_4 (pDev, SDHC_SYSCTL, SYSCTL_DTOCV_MASK);
        CSR_SETBIT_4 (pDev, SDHC_SYSCTL, SYSCTL_DTOCV_MAX << SYSCTL_DTOCV_SHIFT);
        }

    /* issue the command */

    CSR_WRITE_4 (pDev, SDHC_CMDARG, pSdCmd->cmdArg);
    CSR_WRITE_4 (pDev, SDHC_XFERTYP, xferTyp);

    return OK;
}

/*******************************************************************************
*
* sdhcCmdIssue - issue the command
*
* This routine issues the command.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS sdhcCtrlCmdIssue
    (
    VXB_DEVICE_ID pDev,
    SD_CMD * pSdCmd
    )
    {
    SDHC_DEV_CTRL * pDrvCtrl = (SDHC_DEV_CTRL *)(pDev->pDrvCtrl);
    SD_HOST_CTRL * pSdHostCtrl;
    UINT32 pollTimeOut = 0;
    STATUS rc = OK;
    UINT32 blkIndex;

    if (pDrvCtrl == NULL)
        return ERROR;
    else
        pSdHostCtrl = (SD_HOST_CTRL *) pDrvCtrl;

    if (semTake (pSdHostCtrl->hostDevSem, WAIT_FOREVER) == ERROR)
        return ERROR;

    if ((pSdHostCtrl->dmaMode != SDHC_DMA_MODE_PIO) && (pSdCmd->hasData))
        {
        rc = vxbDmaBufMapLoad (pDev, pDrvCtrl->sdHostCtrl.sdHostDmaTag,
                               pDrvCtrl->sdHostCtrl.sdHostDmaMap,
                               pSdCmd->cmdData.buffer,
                               pSdCmd->cmdData.blkNum * pSdCmd->cmdData.blkSize,
                               0);
        if (rc == ERROR)
            {
            semGive (pSdHostCtrl->hostDevSem);
            return (rc);
            }
        }
    if (pSdHostCtrl->sdHostOps.vxbSdCmdPrepare != NULL)
        {
        if (pSdHostCtrl->sdHostOps.vxbSdCmdPrepare (pDev, pSdCmd) == ERROR)
            {
            semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
            if ((pSdHostCtrl->dmaMode != SDHC_DMA_MODE_PIO) && (pSdCmd->hasData))
                vxbDmaBufMapUnload(pSdHostCtrl->sdHostDmaTag,
                                   pSdHostCtrl->sdHostDmaMap);
            return ERROR;
            }
        }

    if (pDrvCtrl->sdHostCtrl.polling == TRUE)
        {
        while (pollTimeOut < SDHC_PIO_TIMEOUT)
            {
            if ((CSR_READ_4 (pDev, SDHC_IRQSTAT) & IRQ_CC) != 0x0)
                break;
            pollTimeOut++;
            }
        if (pollTimeOut >= SDHC_PIO_TIMEOUT)
            rc = ERROR;
        }
    else
        rc = semTake (pDrvCtrl->sdHostCtrl.cmdDone,
                      (sysClkRateGet() * SDMMC_CMD_WAIT_IN_SECS));

    if (rc == ERROR)
        {
        SDHC_DBG (SDHC_DBG_ERR, "[sdhcCtrlCmdIssue]: CMD%d cmd timeout \n",
                 pSdCmd->cmdIdx, 0, 0, 0, 0, 0);
        pSdCmd->cmdErr |= SDMMC_CMD_ERR_TIMEOUT;
        }
    else
        {
        if (pDrvCtrl->intSts & IRQ_CMD_ERR)
            {
            if (pDrvCtrl->intSts & IRQ_CIE)
                {
                SDHC_DBG (SDHC_DBG_ERR, "CMD%d cmd index error\n",
                          pSdCmd->cmdIdx, 0, 0, 0, 0, 0);
                pSdCmd->cmdErr |= SDMMC_CMD_ERR_PROTOCOL;
                }

            if (pDrvCtrl->intSts & IRQ_CEBE)
                {
                SDHC_DBG (SDHC_DBG_ERR, "CMD%d cmd end of bit error\n",
                          pSdCmd->cmdIdx, 0, 0, 0, 0, 0);
                pSdCmd->cmdErr |= SDMMC_CMD_ERR_PROTOCOL;
                }

            if (pDrvCtrl->intSts & IRQ_CCE)
                {
                SDHC_DBG (SDHC_DBG_ERR, "CMD%d cmd crc error\n",
                          pSdCmd->cmdIdx, 0, 0, 0, 0, 0);
                pSdCmd->cmdErr |= SDMMC_CMD_ERR_PROTOCOL;
                }

            if (pDrvCtrl->intSts & IRQ_CTOE)
                {
                SDHC_DBG (SDHC_DBG_ERR, "CMD%d cmd timeout error\n",
                          pSdCmd->cmdIdx, 0, 0, 0, 0, 0);
                pSdCmd->cmdErr |= SDMMC_CMD_ERR_TIMEOUT;
                }
            }
        if (pSdCmd->cmdErr != 0x0)
            {
            SDHC_DBG (SDHC_DBG_ERR, "[sdhcCtrlCmdIssue]: CMD%d cmd->cmdErr \n",
                      pSdCmd->cmdIdx, 0, 0, 0, 0, 0);
            if ((CSR_READ_4(pDev, SDHC_PRSSTAT) & PRSSTAT_CIHB) != 0x0)
                {
                CSR_SETBIT_4(pDev, SDHC_SYSCTL, SYSCTL_RSTC);
                while ((CSR_READ_4(pDev, SDHC_SYSCTL) & SYSCTL_RSTC) != 0x0);
                }
            
            }
        }

    if (pSdCmd->rspType != SDMMC_CMD_RSP_NONE)
        {
        if (pSdCmd->rspType & SDMMC_CMD_RSP_LEN136)
            {
            UINT32 cmdRsp[4];

            cmdRsp[0] = CSR_READ_4 (pDev, SDHC_CMDRSP0);
            cmdRsp[1] = CSR_READ_4 (pDev, SDHC_CMDRSP1);
            cmdRsp[2] = CSR_READ_4 (pDev, SDHC_CMDRSP2);
            cmdRsp[3] = CSR_READ_4 (pDev, SDHC_CMDRSP3);

            pSdCmd->cmdRsp[0] = be32toh ((cmdRsp[3] << 8) | (cmdRsp[2] >> 24));
            pSdCmd->cmdRsp[1] = be32toh ((cmdRsp[2] << 8) | (cmdRsp[1] >> 24));
            pSdCmd->cmdRsp[2] = be32toh ((cmdRsp[1] << 8) | (cmdRsp[0] >> 24));

	    /* eliminate BE/LE confusion for Coverity */
            /* coverity[operator_confusion] */
            pSdCmd->cmdRsp[3] = be32toh (cmdRsp[0] << 8);
            }
        else
            {
            pSdCmd->cmdRsp[0] = CSR_READ_4 (pDev, SDHC_CMDRSP0);
            }
        }

    if ((pSdHostCtrl->dmaMode == SDHC_DMA_MODE_PIO) && pSdCmd->hasData)
        {
        for (blkIndex = 0; blkIndex < pSdCmd->cmdData.blkNum; blkIndex++)
            {
            rc = semTake (pSdHostCtrl->pioReady,
                          (sysClkRateGet() * SDMMC_CMD_WAIT_IN_SECS));
            if (rc == ERROR)
                {
                SDHC_DBG (SDHC_DBG_ERR, "[sdhcCtrlCmdIssue]: CMD%d PIO timeout\n",
                          pSdCmd->cmdIdx, 0, 0, 0, 0, 0);
                pSdCmd->cmdErr |= SDMMC_DATA_ERR_TIMEOUT;
                semGive (pDrvCtrl->sdHostCtrl.hostDevSem);

                return ERROR;
                }
            else
                {

                /* PIO mode */

                if (pSdCmd->cmdData.isRead == TRUE)
                    {
                    SDHC_DBG (SDHC_DBG_RW, "pio read \n", 0, 0, 0, 0, 0, 0);
                    rc = sdhcCtrlPioRead (pDev);
                    if (rc == ERROR)
                        {
                        semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
                        return ERROR;
                        }
                    }
                else
                    {
                    SDHC_DBG (SDHC_DBG_RW, "pio write \n", 0, 0, 0, 0, 0, 0);
                    rc = sdhcCtrlPioWrite (pDev);
                    if (rc == ERROR)
                        {
                        semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
                        return ERROR;
                        }
                    }
                }
            }
        }

    if ((pSdCmd->rspType & SDMMC_CMD_RSP_BUSY) && (pSdCmd->hasData == FALSE))
        {
        
        /* Need not check return status */
        /* NO data tran but has data int ack. such as sdCmdSelectCard */

        (void)semTake (pDrvCtrl->sdHostCtrl.dataDone, sysClkRateGet() / 4);

        (void)semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
        return OK;
        }

    if (pSdCmd->hasData)
        {
        rc = semTake (pDrvCtrl->sdHostCtrl.dataDone,
                      (sysClkRateGet() * SDMMC_CMD_WAIT_IN_SECS));
        if (rc == ERROR)
            {
            SDHC_DBG (SDHC_DBG_ERR, "[sdhcCtrlCmdIssue]: CMD%d data timeout\n",
                      pSdCmd->cmdIdx, 0, 0, 0, 0, 0);
            pSdCmd->dataErr |= SDMMC_DATA_ERR_TIMEOUT;
            }
        else
            {
            vxbDmaBufMapSync (pDev,
                              pSdHostCtrl->sdHostDmaTag,
                              pSdHostCtrl->sdHostDmaMap,
                              0,
                              pSdCmd->cmdData.blkNum * pSdCmd->cmdData.blkSize,
                             _VXB_DMABUFSYNC_DMA_POSTWRITE);
			
            if (pDrvCtrl->intSts & IRQ_DATA_ERR)
                {
                if (pDrvCtrl->intSts & IRQ_DEBE)
                    {
                    SDHC_DBG (SDHC_DBG_ERR, "CMD%d data end of bit error\n",
                              pSdCmd->cmdIdx, 0, 0, 0, 0, 0);
                    pSdCmd->dataErr |= SDMMC_DATA_ERR_PROTOCOL;
                    }

                if (pDrvCtrl->intSts & IRQ_DCE)
                    {
                    SDHC_DBG (SDHC_DBG_ERR, "CMD%d data crc error\n",
                              pSdCmd->cmdIdx, 0, 0, 0, 0, 0);
                    pSdCmd->dataErr |= SDMMC_DATA_ERR_PROTOCOL;
                    }

                if (pDrvCtrl->intSts & IRQ_DTOE)
                    {
                    SDHC_DBG (SDHC_DBG_ERR, "CMD%d data timeout error\n",
                              pSdCmd->cmdIdx, 0, 0, 0, 0, 0);
                    pSdCmd->dataErr |= SDMMC_DATA_ERR_TIMEOUT;
                    }
                }
            if (pSdHostCtrl->dataErr != 0x0)
                {
                SDHC_DBG (SDHC_DBG_ERR, "[sdhcCtrlCmdIssue]: CMD%d cmd->dataErr\n",
                          pSdCmd->cmdIdx, 0, 0, 0, 0, 0);
                }
            }
        if (pSdCmd->cmdData.isRead)
            {
            vxbDmaBufMapSync (pDev,
                              pSdHostCtrl->sdHostDmaTag,
                              pSdHostCtrl->sdHostDmaMap,
                              0,
                              pSdCmd->cmdData.blkNum * pSdCmd->cmdData.blkSize,
                              _VXB_DMABUFSYNC_DMA_POSTREAD);
            } 
        vxbDmaBufMapUnload(pSdHostCtrl->sdHostDmaTag, pSdHostCtrl->sdHostDmaMap);
        }
    if ((pSdCmd->dataErr != 0x0) || (pSdCmd->cmdErr != 0x0))
        {
        SDHC_DBG (SDHC_DBG_ERR,
                  "[sdhcCtrlCmdIssue]: CMD%d cmd->dataErr = 0x%x cmd->cmdErr = 0x%x\n",
                  pSdCmd->cmdIdx, pSdCmd->dataErr, pSdCmd->cmdErr, 0, 0, 0);
        rc = ERROR;
        }
    semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
    return (rc);
    }

/*******************************************************************************
*
* sdhcBusWidthSetup - setup the bus width
*
* This routine setup the bus width.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void sdhcCtrlBusWidthSetup
    (
    VXB_DEVICE_ID pDev,
    UINT32       width
    )
    {
    CSR_CLRBIT_4 (pDev, SDHC_PROCTL, PROCTL_DTW_4BIT | PROCTL_DTW_8BIT);

    switch (width)
        {
        case SDMMC_BUS_WIDTH_1BIT:
            CSR_SETBIT_4 (pDev, SDHC_PROCTL, PROCTL_DTW_1BIT);
            break;
        case SDMMC_BUS_WIDTH_4BIT:
            CSR_SETBIT_4 (pDev, SDHC_PROCTL, PROCTL_DTW_4BIT);
            break;
        case SDMMC_BUS_WIDTH_8BIT:
            CSR_SETBIT_4 (pDev, SDHC_PROCTL, PROCTL_DTW_8BIT);
            break;
        default:
            break;
        }
    return;
    }

/*******************************************************************************
*
* sdhcCardWpCheck - check if card is write protected
*
* This routine checks if card is write protected.
*
* RETURNS: TRUE if card is write protected or FALSE
*
* ERRNO: N/A
*
* \NOMANUAL
*/

BOOL sdhcCtrlCardInsertSts
    (
    VXB_DEVICE_ID pDev
    )
    {
    UINT32 prsStat;
    SD_HOST_CTRL * pSdHostCtrl;
    pSdHostCtrl = (SD_HOST_CTRL *)pDev->pDrvCtrl;

    if (pSdHostCtrl == NULL)
        return FALSE;

    if (!pSdHostCtrl->polling)
        {

        /* Need not check return status */

        (void)semTake(pSdHostCtrl->devChange, WAIT_FOREVER);
        prsStat = CSR_READ_4 (pDev, SDHC_PRSSTAT);
        if (prsStat & PRSSTAT_CINS)
            pSdHostCtrl->isInsert = TRUE;
        else
            pSdHostCtrl->isInsert = FALSE;
        }
    else
        {
        while (1)
            {
            taskDelay (10 * sysClkRateGet());
            prsStat = CSR_READ_4 (pDev, SDHC_PRSSTAT);

            if (prsStat & PRSSTAT_CINS)
                {
                if (pSdHostCtrl->isInsert != TRUE)
                    {
                    pSdHostCtrl->isInsert = TRUE;
                    break;
                    }
                }
            else
                {
                if (pSdHostCtrl->isInsert != FALSE)
                    {
                    pSdHostCtrl->isInsert = FALSE;
                    break;
                    }
                }
            }
        }
    return (pSdHostCtrl->isInsert);
    }

/*******************************************************************************
*
* sdhcCardWpCheck - check if card is write protected
*
* This routine checks if card is write protected.
*
* RETURNS: TRUE if card is write protected or FALSE
*
* ERRNO: N/A
*
* \NOMANUAL
*/

BOOL sdhcCtrlCardWpCheck
    (
    VXB_DEVICE_ID pDev
    )
    {
    BOOL wp;
    SDHC_DEV_CTRL * pDrvCtrl = (SDHC_DEV_CTRL *)pDev->pDrvCtrl;

    if (CSR_READ_4 (pDev, SDHC_PRSSTAT) & PRSSTAT_WPSPL)
        wp = FALSE;
    else
        wp = TRUE;

    return  (wp);
    }

/*******************************************************************************
*
* sdhcPioRead - read data from SD/MMC card using PIO
*
* This routine reads data from SD/MMC card using PIO.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS sdhcCtrlPioRead
    (
    VXB_DEVICE_ID   pDev
    )
    {
    SDHC_DEV_CTRL * pDrvCtrl = (SDHC_DEV_CTRL *)pDev->pDrvCtrl;
    UINT32 blkSize;
    char * buffer;
    UINT32 data = 0;
    UINT32 i;

    if (pDrvCtrl == NULL)
        return ERROR;

    blkSize = pDrvCtrl->pioDesc.blkSize;
    buffer = pDrvCtrl->pioDesc.bufCur;

    for (i = 0; i < blkSize; i += 4)
        {
        data = PIO_READ_4 (pDev, SDHC_DATPORT);

        /*
         * It seems that Freescale eSDHC version 2.0 IP has a hardware bug that
         * needs a delay between consecutive access to the buffer data port,
         * otherwise the transfer complete status will never show up.
         */

        if ((pDrvCtrl->flags & SDHC_PIO_NEED_DELAY) ||
            (pDrvCtrl->sdHostCtrl.version == (HOSTVER_VVN_20 | HOSTVER_SVN_20)))
            vxbUsDelay (100);

        *(UINT32 *)buffer = data;
        buffer += 4;
        }

    pDrvCtrl->pioDesc.bufCur = buffer;

    SDHC_DBG (SDHC_DBG_RW, "[PIO_READ]: bufOri (%p) bufCur (%p)\n",
              (_Vx_usr_arg_t)pDrvCtrl->pioDesc.bufOri,
              (_Vx_usr_arg_t)pDrvCtrl->pioDesc.bufCur,
              0, 0, 0, 0);

    return OK;
    }

/*******************************************************************************
*
* sdhcPioWrite - write data to SD/MMC card using PIO
*
* This routine writes data to SD/MMC card using PIO.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS sdhcCtrlPioWrite
    (
    VXB_DEVICE_ID   pDev
    )
    {
    SDHC_DEV_CTRL * pDrvCtrl = (SDHC_DEV_CTRL *)pDev->pDrvCtrl;
    UINT32 blkSize;
    char * buffer;
    UINT32 data;
    UINT32 i;

    if (pDrvCtrl == NULL)
        return ERROR;

    blkSize = pDrvCtrl->pioDesc.blkSize;
    buffer = pDrvCtrl->pioDesc.bufCur;

    for (i = 0; i < blkSize; i += 4)
        {
        data = *(UINT32 *)buffer;
        buffer += 4;

        PIO_WRITE_4 (pDev, SDHC_DATPORT, data);

        /*
         * It seems that Freescale eSDHC version 2.0 IP has a hardware bug that
         * needs a delay between consecutive access to the buffer data port,
         * otherwise the transfer complete status will never show up.
         */

        if ((pDrvCtrl->flags & SDHC_PIO_NEED_DELAY) ||
            (pDrvCtrl->sdHostCtrl.version == (HOSTVER_VVN_20 | HOSTVER_SVN_20)))
            vxbUsDelay (100);
        }

    pDrvCtrl->pioDesc.bufCur = buffer;

    SDHC_DBG (SDHC_DBG_RW, "[PIO_WRITE]: bufOri (%p) bufCur (%p)\n",
              (_Vx_usr_arg_t)pDrvCtrl->pioDesc.bufOri,
              (_Vx_usr_arg_t)pDrvCtrl->pioDesc.bufCur,
              0, 0, 0, 0);

    return OK;
    }

/*******************************************************************************
*
* sdhcCmdIssuePoll - issue the command using polling mode
*
* This routine issues the command using polling mode.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS sdhcCtrlCmdIssuePoll
    (
    VXB_DEVICE_ID   pDev,
    SD_CMD * pSdCmd
    )
    {
    SDHC_DEV_CTRL * pDrvCtrl = (SDHC_DEV_CTRL *)pDev->pDrvCtrl;
    volatile UINT32 intSts;
    UINT32 pollTimeOut = 0;
    STATUS rc = OK;

    if (pDrvCtrl == NULL)
        return ERROR;


    if (semTake (pDrvCtrl->sdHostCtrl.hostDevSem, WAIT_FOREVER) == ERROR)
        return ERROR;

    if (pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdCmdPrepare != NULL)
        {
        if (pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdCmdPrepare (pDev, pSdCmd) == ERROR)
            {
            semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
            return ERROR;
            }
        }

    /* wait for command to complete */

    while (pollTimeOut < POLL_TIMEOUT)
        {
        if ((CSR_READ_4 (pDev, SDHC_IRQSTAT) & IRQ_CC) != 0x0)
            break;
        pollTimeOut++;
        }

    if (pollTimeOut >= POLL_TIMEOUT)
        {
        SDHC_DBG (SDHC_DBG_ERR, "wait for command timeout\n",
                  0, 0, 0, 0, 0, 0);
        semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
        return ERROR;
        }

    intSts = CSR_READ_4 (pDev, SDHC_IRQSTAT);

    if (intSts & IRQ_CMD_ERR)
        {
        if (intSts & IRQ_CIE)
            {
            SDHC_DBG (SDHC_DBG_ERR, "CMD%d cmd index error\n",
                      pSdCmd->cmdIdx, 0, 0, 0, 0, 0);
            pSdCmd->cmdErr |= SDMMC_CMD_ERR_PROTOCOL;
            }

        if (intSts & IRQ_CEBE)
            {
            SDHC_DBG (SDHC_DBG_ERR, "CMD%d cmd end of bit error\n",
                      pSdCmd->cmdIdx, 0, 0, 0, 0, 0);
            pSdCmd->cmdErr |= SDMMC_CMD_ERR_PROTOCOL;
            }

        if (intSts & IRQ_CCE)
            {
            SDHC_DBG (SDHC_DBG_ERR, "CMD%d cmd crc error\n",
                      pSdCmd->cmdIdx, 0, 0, 0, 0, 0);
            pSdCmd->cmdErr |= SDMMC_CMD_ERR_PROTOCOL;
            }

        if (intSts & IRQ_CTOE)
            {
            SDHC_DBG (SDHC_DBG_ERR, "CMD%d cmd timeout error\n",
                      pSdCmd->cmdIdx, 0, 0, 0, 0, 0);
            pSdCmd->cmdErr |= SDMMC_CMD_ERR_TIMEOUT;
            }

        CSR_WRITE_4 (pDev, SDHC_IRQSTAT, IRQ_ALL);
        semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
        return ERROR;
        }

    if (pSdCmd->rspType != SDMMC_CMD_RSP_NONE)
        {
        if (pSdCmd->rspType & SDMMC_CMD_RSP_LEN136)
            {
            UINT32 cmdRsp[4];

            cmdRsp[0] = CSR_READ_4 (pDev, SDHC_CMDRSP0);
            cmdRsp[1] = CSR_READ_4 (pDev, SDHC_CMDRSP1);
            cmdRsp[2] = CSR_READ_4 (pDev, SDHC_CMDRSP2);
            cmdRsp[3] = CSR_READ_4 (pDev, SDHC_CMDRSP3);

            pSdCmd->cmdRsp[0] = be32toh ((cmdRsp[3] << 8) | (cmdRsp[2] >> 24));
            pSdCmd->cmdRsp[1] = be32toh ((cmdRsp[2] << 8) | (cmdRsp[1] >> 24));
            pSdCmd->cmdRsp[2] = be32toh ((cmdRsp[1] << 8) | (cmdRsp[0] >> 24));

	    /* eliminate BE/LE confusion for Coverity */
            /* coverity[operator_confusion] */
            pSdCmd->cmdRsp[3] = be32toh (cmdRsp[0] << 8);
            }
        else
            {
            pSdCmd->cmdRsp[0] = CSR_READ_4 (pDev, SDHC_CMDRSP0);
            }
        }

    if (pSdCmd->hasData)
        {
        while (1)
            {
            intSts = CSR_READ_4 (pDev, SDHC_IRQSTAT);

            if (intSts & IRQ_TC)
                {
                SDHC_DBG (SDHC_DBG_IRQ, "CMD%d data complete\n",
                          pSdCmd->cmdIdx, 0, 0, 0, 0, 0);
                break;
                }

            if (intSts & IRQ_BRR)
                {
                SDHC_DBG (SDHC_DBG_IRQ, "CMD%d data buffer read ready\n",
                          pSdCmd->cmdIdx, 0, 0, 0, 0, 0);

                if (CSR_READ_4 (pDev, SDHC_PRSSTAT) & PRSSTAT_BREN)
                    {
                    rc = sdhcCtrlPioRead (pDev);
                    if (rc == ERROR)
                        return (ERROR);
                    }
                }

            if (intSts & IRQ_BWR)
                {
                SDHC_DBG (SDHC_DBG_IRQ, "CMD%d data buffer write ready\n",
                          pSdCmd->cmdIdx, 0, 0, 0, 0, 0);

                if (CSR_READ_4 (pDev, SDHC_PRSSTAT) & PRSSTAT_BWEN)
                    {
                    rc = sdhcCtrlPioWrite (pDev);
                    if (rc == ERROR)
                        return (ERROR);
                    }
                }

            if (intSts & IRQ_DATA_ERR)
                {
                if (intSts & IRQ_DEBE)
                    {
                    SDHC_DBG (SDHC_DBG_ERR, "CMD%d data end of bit error\n",
                              pSdCmd->cmdIdx, 0, 0, 0, 0, 0);
                    pSdCmd->dataErr |= SDMMC_DATA_ERR_PROTOCOL;
                    }

                if (intSts & IRQ_DCE)
                    {
                    SDHC_DBG (SDHC_DBG_ERR, "CMD%d data crc error\n",
                              pSdCmd->cmdIdx, 0, 0, 0, 0, 0);
                    pSdCmd->dataErr |= SDMMC_DATA_ERR_PROTOCOL;
                    }

                if (intSts & IRQ_DTOE)
                    {
                    SDHC_DBG (SDHC_DBG_ERR, "CMD%d data timeout error\n",
                              pSdCmd->cmdIdx, 0, 0, 0, 0, 0);
                    pSdCmd->dataErr |= SDMMC_DATA_ERR_TIMEOUT;
                    }

                CSR_WRITE_4 (pDev, SDHC_IRQSTAT, IRQ_ALL);
                semGive (pDrvCtrl->sdHostCtrl.hostDevSem);

                return ERROR;
                }
            };
        }
    CSR_WRITE_4 (pDev, SDHC_IRQSTAT, IRQ_ALL);
    semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
    return OK;
    }

/*******************************************************************************
*
* sdhcClkFreqSetup - setup the clock frequency
*
* This routine setups the clock frequency. If BSP doesn't provide any clock
* setup routine, this one will be used.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void sdhcCtrlClkFreqSetup
    (
    VXB_DEVICE_ID   pDev,
    UINT32       clk
    )
    {
    SDHC_DEV_CTRL * pDrvCtrl = (SDHC_DEV_CTRL *)pDev->pDrvCtrl;
    SD_HOST_CTRL  * pSdHostCtrl;
    UINT32 val, dvs, sdClk;
    UINT32 clkSrc;
    UINT32 timeout = 1000;

    if (pDrvCtrl == NULL)
        return;

    pSdHostCtrl = (SD_HOST_CTRL *)(&(pDrvCtrl->sdHostCtrl));

    clkSrc = pDrvCtrl->sdHostCtrl.curClkFreq;

    /* Calculate a divisor for SD clock frequency */

    if ((pSdHostCtrl->version & HOSTVER_SVN_MASK) <= HOSTVER_SVN_20)
        {
        dvs = 1;
        while (clk < (clkSrc / dvs))
            dvs *= 2;
    
        sdClk = dvs >> 1;
        sdClk <<= SYSCTL_DIV_SHIFT;
    
        val = CSR_READ_4(pDev, SDHC_SYSCTL);
        val &= ~CLK_SETMASK;
        val &= ~SYSCTL_CLK_EN;
        }
    else
        {

        /* 10-bit Divided Clock Mode is mandatory for host controller 3.0 */

        dvs = clkSrc / clk;
        if ((clkSrc % clk) != 0)
            dvs++;

        /* 
         * Now dvs is our wanted divisor, the below calculation is 
         * to change it to register format.
         */

        if (dvs == 1)
            dvs = 0;
        else
            dvs = (dvs / 2) + (dvs % 2);
        if (dvs > 1023)
            {
            SDHC_DBG (SDHC_DBG_ERR, "sdhcCtrlClkFreqSetup: divider out of scope \
                                    of 10 bits\n", 0, 0, 0, 0, 0, 0);
            return;
            }
        sdClk = ((dvs & 0xff) << 2) | ((dvs & 0x300) >> 8);
        sdClk <<= SYSCTL_10BITDIV_SHIFT;

        val = CSR_READ_4(pDev, SDHC_SYSCTL);
        val &= ~CLK_SETMASK_10BIT;
        val &= ~SYSCTL_CLK_EN;
        }

    /* Set SDCLK Frequency Select and Internal Clock Enable*/

    val |= sdClk;
    val |= SYSCTL_IPGEN;
    CSR_WRITE_4(pDev, SDHC_SYSCTL, val);

   /*
    * Check Internal Clock Stable
    *
    * The spec doesn't say anything about this
    * internal clock stable timeout value,
    * so just a pick the value based on expierence.
    */

    while (timeout--)
        {
        val = CSR_READ_4 (pDev, SDHC_SYSCTL);
        if (val & SYSCTL_CLK_STABLE)
            break;
        }
    if (timeout == 0)
        {
        return;
        }

    /* Set SD Clock On */

    val |= SYSCTL_CLK_EN;
    CSR_WRITE_4(pDev, SDHC_SYSCTL, val);

    /* Set SDHC high speed enable */

    if (clk >= SDMMC_CLK_FREQ_25MHZ)
        CSR_SETBIT_4(pDev, SDHC_PROCTL, PROCTL_HS_ENABLED);
    }

/*******************************************************************************
*
* sdhcInterruptInfo - Returns the SDHC interrupt info ptr from the bus
* controller.
*
* \NOMANUAL
*
* RETURNS: OK, or ERROR if this library is not initialized
*
* ERRNO
*
* \NOMANUAL
*/

STATUS sdhcInterruptInfo
    (
    VXB_DEVICE_ID pInst,
    UINT32 * pArgs
    )
    {
    SDHC_DEV_CTRL * pDrvCtrl = (SDHC_DEV_CTRL *)pInst->pDrvCtrl;

    if (pDrvCtrl == NULL)
        return ERROR;
    *(UINT32 *)pArgs = (UINT32)((ULONG)pDrvCtrl->pIntInfo);

    return(OK);
    }

/*******************************************************************************
*
* sdhcDevControl - device control routine
*
* This routine handles manipulation of downstream devices, such as
* interrupt management.
*
* RETURNS: OK, or ERROR
*
* ERRNO
*
* \NOMANUAL
*/

STATUS sdhcDevControl
    (
    VXB_DEVICE_ID    devID,
    pVXB_DEVCTL_HDR  pBusDevControl
    )
    {
    VXB_DEVICE_ID busCtrlID;
    SDHC_DEV_CTRL *pDrvCtrl;
    STATUS status = OK;
    VOIDFUNCPTR * vector = NULL;
    struct plbIntrEntry * pIntr;
    pVXB_ACCESS_INTERRUPT_INFO pAccessIntInfo;
    UINT32 * pVector;

    if ((devID ==  NULL) || (pBusDevControl == NULL))
        return (ERROR);

    busCtrlID = vxbDevParent(devID);
    if (busCtrlID == NULL)
        return (ERROR);

    pDrvCtrl = ((SDHC_DEV_CTRL *)busCtrlID->pDrvCtrl);
    if (pDrvCtrl == NULL)
        return (ERROR);

    pIntr = busCtrlID->pIntrInfo;
    if (pIntr == NULL)
        return (ERROR);

    pAccessIntInfo = (pVXB_ACCESS_INTERRUPT_INFO)pBusDevControl;
    if (busCtrlID->busID == VXB_BUSID_PLB)
        {
        pVXB_ACCESS_INT_VECTOR_GET pIntVector;
        pIntVector = (pVXB_ACCESS_INT_VECTOR_GET)pAccessIntInfo;
        vector = pIntr->intVecList[pAccessIntInfo->intIndex];
        }
    if (busCtrlID->busID == VXB_BUSID_PCI)
        {
        struct pciIntrEntry * pInterruptInfo;
        pInterruptInfo = (struct pciIntrEntry *)(busCtrlID->pIntrInfo);
        vector = (VOIDFUNCPTR*)pInterruptInfo->intVecInfo[pAccessIntInfo->intIndex].intVector;
        }
    if (pAccessIntInfo->intIndex >= pIntr->numVectors)
        return (ERROR);

    switch ( pBusDevControl->vxbAccessId)
        {
        case VXB_ACCESS_INT_CONNECT:
            {

            /* connect the interrupt */

            status = vxbSdioIntConnect(devID,
                                       vector,
                                       (VOIDFUNCPTR)pAccessIntInfo->pISR,
                                       (_Vx_usr_arg_t)pAccessIntInfo->pParameter
                                       );

            return(status);
            }
        default:
            return ERROR;
        }
    }
