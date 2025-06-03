/* vxbAltSocGen5DwMshc.c - Altera SoC DesignWare MSHC driver */

/*
 * Copyright (c) 2013 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */
 
/*
modification history
--------------------
01d,10oct13,sye  Added missed item list end. (WIND00438199)
01c,31jul13,swu  Removed unnecessary API. (WIND00428711)
01b,06jun13,swu  added support for be8
01a,24may13,sye  written.
*/

/*
DESCRIPTION

This is the vxbus compliant Altera SoC DesignWare Mobile Storage 
Host Controller driver which implements the functionality specified in its
manual.

The driver provides an interface between the host controller and the 
SD/MMC memory cards. The driver implements all the vxbus driver specific 
initialization routines like altSocGen5DwMshcInstInit(), 
altSocGen5DwMshcInstInit2() and altSocGen5DwMshcInstConnect().

EXTERNAL INTERFACE

The driver provides the standard vxbus external interface 
altSocGen5DwMshcStorageRegister(). This function registers the driver with 
the vxbus subsystem, and instances will be created as needed. As the MSHC 
device is on platform local bus, each device instance must be specified in 
the hwconf.c file in a BSP. 

The hwconf entry must specify the following parameters:

\is

\i <regBase>
Specifies the base address where the MSHC registers are mapped into the host's
address space. All register offsets are computed relative to this address.

\i <clkFreq>
Specifies the clock source frequency of the module. The clock source
frequency is platform dependent.

\i <rwMode>
Specifies the read and write mode of the module. Both DMA and PIO mode are 
supported now. If this property is not explicitly specified, the driver uses 
DMA mode by default.

\i <polling>
Specifies whether the driver uses polling mode or not. If this property is
not explicitly specified, the driver uses interrupt mode by default.

\i <flags>
Specifies various features of the host controller.
Currently, the following flags are supported:

\is

\i ALT_MSHC_FLAGS_CARD_PRESENT
Specify whether the SD card is present always. This is used for no
hardware card detection scenario, thus the driver assumes the card is always
plugged in and does not support hot plug out.

\i ALT_MSHC_FLAGS_CARD_WRITABLE
Specify whether the SD card is writable always. This is used for no
hardware card write protection scenario.

\i ALT_MSHC_FLAGS_SNOOP_ENABLED
Specify whether the host controller has hardware snoop features. Cache 
operations are not performd when this flag is set.

\ie

\i <clkFreqSetup>
Specifies board related clock frequency setup method. Some boards may need to 
change board specific settings when SD clock frequency is changed. 
The prototype of this method is :

\cs
    void (* clkFreqSetup) (UINT32 clk)
\ce

\ie

An example hwconf entry is shown below:

\cs
struct hcfResource alteraMshcResources[] = {
    { "regBase",        HCF_RES_INT,    { (void *)ALT_SDMMC_BASE } },
    { "clkFreq",        HCF_RES_INT,    { (void *)ALT_SDMMC_CLK_DIVIDED_VAL} },
    { "rwMode",         HCF_RES_INT,    { (void *)ALT_DW_MSHC_RW_MODE_DMA } },
    { "polling",        HCF_RES_INT,    { (void *)FALSE } },
    { "clkFreqSetup",   HCF_RES_ADDR,   { (void *)sysMshcClkFreqSetup } },
    { "flags",          HCF_RES_INT,    { (void *)(ALT_MSHC_FLAGS_CARD_PRESENT |
                                                   ALT_MSHC_FLAGS_CARD_WRITABLE)
                                                  } },
};
\ce

To add this driver to the vxWorks image, add the following component to the
kernel configuration, or define the following macro in config.h.

\cs
vxprj component add DRV_STORAGE_ALT_SOC_GEN5_DW_MSHC
\ce

\cs
#define DRV_STORAGE_ALT_SOC_GEN5_DW_MSHC
\ce

SEE ALSO: vxBus, xbd
\tb "Cyclone V Device Handbook Volume 3: Hard Processor System Technical Reference Manual"
\tb "SD Specifications Part 1 Physical Layer Simplified Specification Version 3.01"
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
#include <vxbTimerLib.h>
#include <hwif/vxbus/vxBus.h>
#include <hwif/vxbus/hwConf.h>
#include <hwif/util/vxbParamSys.h>
#include <../src/hwif/h/vxbus/vxbAccess.h>
#include <../src/hwif/h/storage/vxbSdMmcLib.h>
#include <../src/hwif/h/storage/vxbSdMmcXbd.h>
#include "vxbAltSocGen5DwMshc.h"

/* defines */

#define ALT_DW_MSHC_DBG_ON
#ifdef  ALT_DW_MSHC_DBG_ON

#ifdef  LOCAL
#undef  LOCAL
#define LOCAL
#endif /* LOCAL */

#define ALT_DW_MSHC_DBG_IRQ            0x00000001
#define ALT_DW_MSHC_DBG_RW             0x00000002
#define ALT_DW_MSHC_DBG_XBD            0x00000004
#define ALT_DW_MSHC_DBG_ERR            0x00000008
#define ALT_DW_MSHC_DBG_INIT           0x00000010
#define ALT_DW_MSHC_DBG_CMD            0x00000020
#define ALT_DW_MSHC_DBG_ALL            0xffffffff
#define ALT_DW_MSHC_DBG_OFF            0x00000000

LOCAL UINT32 altSocGen5DwMshcDbgMask = ALT_DW_MSHC_DBG_ERR|ALT_DW_MSHC_DBG_XBD;

IMPORT FUNCPTR _func_logMsg;

#define ALT_DW_MSHC_DBG(mask, string, a, b, c, d, e, f)                    \
    if ((altSocGen5DwMshcDbgMask & mask) || (mask == ALT_DW_MSHC_DBG_ALL)) \
        if (_func_logMsg != NULL)                                          \
           (* _func_logMsg)(string, a, b, c, d, e, f)
#else /* !ALT_DW_MSHC_DBG_ON */
#define ALT_DW_MSHC_DBG(mask, string, a, b, c, d, e, f)
#endif  /* ALT_DW_MSHC_DBG_ON */

/*
 * File system use cacheDmaMalloc to alloc buffer 
 * (file bio.c, bio_alloc() routine), so there is no need for driver to 
 * use cacheFlush and cacheInvalidate to keep buffer's coherence. 
 * When define ALT_DW_MSHC_FS_UNCACHE_BUFFER in this driver, we will ignore the
 * the sync operaton of DMA buffer.
 */

#define ALT_DW_MSHC_FS_UNCACHE_BUFFER

/* register low level access routines */

#define ALT_DW_MSHC_BAR(p)     ((ALT_MSHC_DRV_CTRL *)(p)->pDrvCtrl)->regBase
#define ALT_DW_MSHC_HANDLE(p)  ((ALT_MSHC_DRV_CTRL *)(p)->pDrvCtrl)->regHandle

#ifdef ARMBE8
#    define SWAP32 vxbSwap32
#else
#    define SWAP32 
#endif /* ARMBE8 */

#undef CSR_READ_4
#define CSR_READ_4(pDev, addr)                  \
        SWAP32(vxbRead32(ALT_DW_MSHC_HANDLE(pDev),      \
                  (UINT32 *)((char *)ALT_DW_MSHC_BAR(pDev) + addr)))

#undef CSR_WRITE_4
#define CSR_WRITE_4(pDev, addr, data)           \
        vxbWrite32(ALT_DW_MSHC_HANDLE(pDev),    \
                   (UINT32 *)((char *)ALT_DW_MSHC_BAR(pDev) + addr), SWAP32(data))

#undef CSR_SETBIT_4
#define CSR_SETBIT_4(pDev, offset, val)         \
        CSR_WRITE_4(pDev, offset, CSR_READ_4(pDev, offset) | (val))

#undef CSR_CLRBIT_4
#define CSR_CLRBIT_4(pDev, offset, val)         \
        CSR_WRITE_4(pDev, offset, CSR_READ_4(pDev, offset) & ~(val))

/* forward declarations */

LOCAL void altSocGen5DwMshcInstInit (VXB_DEVICE_ID pInst);
LOCAL void altSocGen5DwMshcInstInit2 (VXB_DEVICE_ID pInst);
LOCAL void altSocGen5DwMshcInstConnect (VXB_DEVICE_ID pInst);
LOCAL void altSocGen5DwMshcDevInit (VXB_DEVICE_ID pInst);
LOCAL void altSocGen5DwMshcIsr (VXB_DEVICE_ID pDev);
LOCAL void altSocGen5DwMshcCardMonTask (VXB_DEVICE_ID);
LOCAL STATUS altSocGen5DwMshcCmdIssue (SDMMC_HOST * host);
LOCAL void altSocGen5DwMshcBusWidthSetup (SDMMC_HOST * host, UINT32 width);
LOCAL BOOL altSocGen5DwMshcCardWpCheck (SDMMC_HOST * host);
LOCAL BOOL altSocGen5DwMshcCardDetect (VXB_DEVICE_ID pDev);

LOCAL STATUS altSocGen5DwMshcBlkRead (SDMMC_CARD * card, sector_t blkNo, 
                                      UINT32 numBlks, void * pBuf);
LOCAL STATUS altSocGen5DwMshcBlkWrite (SDMMC_CARD * card, sector_t blkNo, 
                                       UINT32 numBlks, void * pBuf);


LOCAL STATUS altSocGen5DwMshcPioRead (VXB_DEVICE_ID pDev);
LOCAL STATUS altSocGen5DwMshcPioWrite (VXB_DEVICE_ID pDev);
LOCAL void altSocGen5DwMshcClkFreqSetup (SDMMC_HOST * host, UINT32 clk);
LOCAL void altSocGen5DwMshcVddSetup (SDMMC_HOST * host, UINT32 vdd);
LOCAL void altSocGen5DwMshcWriteFifo (VXB_DEVICE_ID pDev);
LOCAL void altSocGen5DwMshcReadFifo (VXB_DEVICE_ID pDev);
LOCAL STATUS altSocGen5DwMshcPollIntsts (VXB_DEVICE_ID pDev, UINT32 mask);

/* locals */

LOCAL DRIVER_INITIALIZATION altSocGen5DwMshcFuncs =
    {
    altSocGen5DwMshcInstInit,        /* devInstanceInit */
    altSocGen5DwMshcInstInit2,       /* devInstanceInit2 */
    altSocGen5DwMshcInstConnect      /* devConnect */
    };

LOCAL DRIVER_REGISTRATION altSocGen5DwMshcPlbRegistration =
{
    NULL,                            /* pNext */
    VXB_DEVID_DEVICE,                /* devID */
    VXB_BUSID_PLB,                   /* busID = PLB */
    VXB_VER_4_0_0,                   /* vxbVersion */
    ALT_DW_MSHC_NAME,                /* drvName */
    &altSocGen5DwMshcFuncs,          /* pDrvBusFuncs */
    NULL,                            /* pMethods */
    NULL,                            /* devProbe */
    NULL                             /* pParamDefaults */
};

/* externs */

IMPORT UCHAR erfLibInitialized;

/*******************************************************************************
*
* altSocGen5DwMshcRegister - register this driver
*
* This routine registers this driver with the vxbus subsystem.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void altSocGen5DwMshcRegister (void)
    {
    vxbDevRegister (&altSocGen5DwMshcPlbRegistration);
    }

/*******************************************************************************
*
* altSocGen5DwMshcInstInit - first level initialization routine of the device
*
* This routine performs the first level initialization of the device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshcInstInit
    (
    VXB_DEVICE_ID pInst
    )
    {
    }

/*******************************************************************************
*
* altSocGen5DwMshcInstInit2 - second level initialization routine of the device
*
* This routine performs the second level initialization of the the device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshcInstInit2
    (
    VXB_DEVICE_ID       pInst
    )
    {
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    struct hcfDevice *  pHcf;
    UINT16              i;
    STATUS              status;

    if (pInst == NULL)
        return;

    pDrvCtrl = (ALT_MSHC_DRV_CTRL *)malloc (sizeof(ALT_MSHC_DRV_CTRL));
    if (pDrvCtrl == NULL)
        return;

    bzero ((char *)pDrvCtrl, sizeof(ALT_MSHC_DRV_CTRL));

    pDrvCtrl->pDev = pInst;
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

    pHcf = (struct hcfDevice *)hcfDeviceGet (pInst);

    if (pHcf != NULL)
        {   
        /*
         * resourceDesc {
         * The clkFreq resource specifies clock
         * frequency for base clock. }
         */

        status = devResourceGet (pHcf, "clkFreq", HCF_RES_INT,
                                 (void *)&pDrvCtrl->clkFreq);
        if (status == ERROR)
            goto errOut;
        
        /*
         * resourceDesc {
         * The rwMode resource specifies the read and write
         * mode. If this property is not
         * explicitly specified, the driver uses
         * DMA by default. }
         */

        status = devResourceGet (pHcf, "rwMode", HCF_RES_INT,
                                 (void *)&pDrvCtrl->rwMode);

        if (status == ERROR)
            pDrvCtrl->rwMode = ALT_DW_MSHC_RW_MODE_DEFAULT;

        /*
         * resourceDesc {
         * The polling resource specifies whether
         * the driver uses polling mode or not.
         * If this property is not explicitly
         * specified, the driver uses interrupt mode
         * by default. }
         */

        status = devResourceGet (pHcf, "polling", HCF_RES_INT,
                                 (void *)&pDrvCtrl->polling);
        if (status == ERROR)
            pDrvCtrl->polling = FALSE;

        /*
         * resourceDesc {
         * The flags resource specifies various 
         * controll flags of the host controller. }
         */

        pDrvCtrl->flags = 0;
        (void) devResourceGet (pHcf, "flags", HCF_RES_INT,
                               (void *)&pDrvCtrl->flags);

        /*
         * resourceDesc {
         * The boardTuning resource specifies the method of 
         * board specific clock tuning 
         */

        (void) devResourceGet (pHcf, "clkFreqSetup", HCF_RES_ADDR,
                               (void *)&pDrvCtrl->clkFreqSetup);

        }

    /*
     * The devChange semaphore is used by the interrupt service routine
     * to inform the card monitor task that a state change has occurred.
     * If the board has special mechanism to notify state change, BSP should 
     * implement it, such as GPIO interrupt.
     */

    pDrvCtrl->devChange = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
    if (pDrvCtrl->devChange == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "semBCreate failed for devChange\n",
                         1, 2, 3, 4, 5, 6);
        goto errOut;
        }

    pDrvCtrl->cmdDone = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
    if (pDrvCtrl->cmdDone == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "semBCreate failed for cmdDone\n",
                         1, 2, 3, 4, 5, 6);
        goto errOut;
        }

    pDrvCtrl->dataDone = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
    if (pDrvCtrl->dataDone == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "semBCreate failed for dataDone\n",
                         1, 2, 3, 4, 5, 6);
        goto errOut;
        }

#ifdef ALT_DW_MSHC_FS_UNCACHE_BUFFER
    pDrvCtrl->flags |= ALT_MSHC_FLAGS_SNOOP_ENABLED;
#endif /* ALT_DW_MSHC_FS_UNCACHE_BUFFER */

    /* success */

    return;

errOut:
    if ((pDrvCtrl->dataDone != NULL) && (semDelete(pDrvCtrl->dataDone) != OK))
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, "semDelete failed for dateDone\n",
                         1, 2, 3, 4, 5, 6);

    if ((pDrvCtrl->cmdDone != NULL) && (semDelete(pDrvCtrl->cmdDone) != OK))
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, "semDelete failed for cmdDone\n",
                         1, 2, 3, 4, 5, 6);
    if ((pDrvCtrl->devChange != NULL) && (semDelete(pDrvCtrl->devChange) != OK))
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "semDelete failed for devChange\n",
                         1, 2, 3, 4, 5, 6);
    free (pDrvCtrl);

    return;
    }

/*******************************************************************************
*
* altSocGen5DwMshcInstConnect - third level initialization routine of the device
*
* This routine performs the third level initialization of the the device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshcInstConnect
    (
    VXB_DEVICE_ID pInst
    )
    {
    ALT_MSHC_DRV_CTRL * pDrvCtrl;

    if (pInst == NULL)
        return;

    pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pInst->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return;

    /* create a monitor task that handles card state change */

    taskSpawn (ALT_DW_MSHC_CARD_MON_TASK_NAME, ALT_DW_MSHC_CARD_MON_TASK_PRI, 0,
               ALT_DW_MSHC_CARD_MON_TASK_STACK, 
               (FUNCPTR)altSocGen5DwMshcCardMonTask, 
               (_Vx_usr_arg_t)pInst,
               0, 0, 0, 0, 0, 0, 0, 0, 0);

    if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_DMA)
        {
        pDrvCtrl->idmaDesc = (ALT_MSHC_IDMA_DESC *) cacheDmaMalloc 
                             (sizeof (ALT_MSHC_IDMA_DESC) * 
                              ALT_MSHC_IDMA_DESC_NUM);    
        if (pDrvCtrl->idmaDesc == NULL)
            {
            /* use pio instead */

            pDrvCtrl->rwMode = ALT_DW_MSHC_RW_MODE_PIO;
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_INIT, 
                             "Failed to alloc dma descriptors, use PIO\n", 
                             1, 2, 3, 4, 5, 6);
            }
        bzero ((char *)pDrvCtrl->idmaDesc, 
               sizeof (ALT_MSHC_IDMA_DESC) * ALT_MSHC_IDMA_DESC_NUM);
        }

    /* per-device init */

    altSocGen5DwMshcDevInit (pInst);

    /* setup the interrupt mask */ 

    if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_DMA)
        pDrvCtrl->intMask = ALT_MSHC_INT_DMA_DEFAULTS;
    else
        pDrvCtrl->intMask = ALT_MSHC_INT_PIO_DEFAULTS;

    CSR_WRITE_4 (pInst, ALT_MSHC_INTMASK, pDrvCtrl->intMask);
   
    if (!pDrvCtrl->polling)
        {
        /* connect and enable interrupt */

        vxbIntConnect (pInst, 0, altSocGen5DwMshcIsr, pInst);
        vxbIntEnable (pInst, 0, altSocGen5DwMshcIsr, pInst);

        /* enable interrupts */

        CSR_SETBIT_4 (pInst, ALT_MSHC_CTRL, ALT_MSHC_CTRL_INT_ENABLE);

        /* card always present, notify monitor task */

        if ((pDrvCtrl->flags & ALT_MSHC_FLAGS_CARD_PRESENT) != 0)
            semGive (pDrvCtrl->devChange);
        }

    return;
    }

/*******************************************************************************
*
* altSocGen5DwMshcDevInit - per device specific initialization of ALT MSHC
*
* This routine performs per device specific initialization of ALT MSHC.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshcDevInit
    (
    VXB_DEVICE_ID pInst
    )
    {
    ALT_MSHC_DRV_CTRL *  pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pInst->pDrvCtrl;
    ALT_MSHC_IDMA_DESC * pIdmaDesc = pDrvCtrl->idmaDesc;
    void *               physAddr;

    /* reset controller */

    CSR_WRITE_4 (pInst, ALT_MSHC_CTRL, (ALT_MSHC_CTRL_RESET | 
                 ALT_MSHC_CTRL_FIFO_RESET | ALT_MSHC_CTRL_DMA_RESET));

    while ((CSR_READ_4 (pInst, ALT_MSHC_CTRL) & 
            (ALT_MSHC_CTRL_RESET | ALT_MSHC_CTRL_FIFO_RESET | 
            ALT_MSHC_CTRL_DMA_RESET)) != 0)
        ;

    if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_DMA)
        {
        /* dual buffer mode */
        pIdmaDesc->des0 = SWAP32(ALT_MSHC_IDMA_DES0_FD);
        pIdmaDesc[ALT_MSHC_IDMA_DESC_NUM - 1].des0 = 
                SWAP32(ALT_MSHC_IDMA_DES0_ER);
        physAddr = CACHE_DMA_VIRT_TO_PHYS (pIdmaDesc);
        pIdmaDesc[ALT_MSHC_IDMA_DESC_NUM - 1].des3 = 
                SWAP32(VXB_ADDR_LOW32 (physAddr)); 

        CSR_WRITE_4 (pInst, ALT_MSHC_BMOD, 4 * ALT_MSHC_BMOD_DSL);
        CSR_WRITE_4 (pInst, ALT_MSHC_BMOD, ALT_MSHC_BMOD_SWR);

        CSR_WRITE_4 (pInst, ALT_MSHC_IDINTEN, (ALT_MSHC_IDMA_INT_RI | 
                     ALT_MSHC_IDMA_INT_TI | ALT_MSHC_IDMA_INT_NI));

        CSR_WRITE_4 (pInst, ALT_MSHC_DBADDR, VXB_ADDR_LOW32 (physAddr));
        }

    /* clear interrupts */
    CSR_WRITE_4(pInst, ALT_MSHC_RINTSTS, 0xFFFFFFFF);
    CSR_WRITE_4(pInst, ALT_MSHC_INTMASK, 0);

    CSR_WRITE_4(pInst, ALT_MSHC_TMOUT, ALT_MSHC_DEFAULT_TIMEOUT);
    CSR_WRITE_4(pInst,  ALT_MSHC_FIFOTH, ALT_MSHC_DEFAULT_FIFO_THRESH);

    /* disable clock */
    CSR_WRITE_4(pInst,  ALT_MSHC_CLKENA, 0);
    CSR_WRITE_4(pInst,  ALT_MSHC_CLKSRC, 0);

#if 1 /* for vx dosfs*/
    /* fill in SDMMC_HOST structure */
    pDrvCtrl->host.pDev = pInst;
    pDrvCtrl->host.ops.cmdIssue        = (FUNCPTR)altSocGen5DwMshcCmdIssue;
	
    pDrvCtrl->host.ops.vddSetup        = (FUNCPTR)altSocGen5DwMshcVddSetup;
    pDrvCtrl->host.ops.clkFreqSetup    = (FUNCPTR)altSocGen5DwMshcClkFreqSetup;
    pDrvCtrl->host.ops.busWidthSetup   = (FUNCPTR)altSocGen5DwMshcBusWidthSetup;
    pDrvCtrl->host.ops.cardWpCheck     = (FUNCPTR)altSocGen5DwMshcCardWpCheck;

    /* voltage supported */
    pDrvCtrl->host.capbility |= (OCR_VDD_VOL_32_33 | OCR_VDD_VOL_33_34);

    /* high capacity SD card is supported */
    pDrvCtrl->host.capbility |= OCR_CARD_CAP_STS;

    /* high speed SD card supported */
    pDrvCtrl->host.highSpeed = TRUE;

    pDrvCtrl->card.host = &pDrvCtrl->host;

    /* fill in SDMMC_XBD_DEV structure */
    pDrvCtrl->card.xbdDev.blkRd = (FUNCPTR)altSocGen5DwMshcBlkRead;
    pDrvCtrl->card.xbdDev.blkWt = (FUNCPTR)altSocGen5DwMshcBlkWrite;

    pDrvCtrl->card.xbdDev.card  = &pDrvCtrl->card;
    pDrvCtrl->card.idx          = pInst->unitNumber;
#endif

    }

/*******************************************************************************
*
* altSocGen5DwMshcIsr - interrupt service routine
*
* This routine handles interrupts of the ALT MSHC controller.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshcIsr
    (
    VXB_DEVICE_ID pDev
    )
    {
    ALT_MSHC_DRV_CTRL * pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    SDMMC_HOST *        host = &pDrvCtrl->host;
    UINT32              mintSts;
    UINT32              idSts = 0;
    BOOL                needRead  = FALSE;
    BOOL                needWrite = FALSE;

    mintSts = CSR_READ_4 (pDev, ALT_MSHC_MINTSTS);
    ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_IRQ, 
                     "MINTSTS=0x%x\n", mintSts, 2, 3, 4, 5, 6);

    if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_DMA)
        {
        idSts = CSR_READ_4 (pDev, ALT_MSHC_IDSTS);
        }
    if ((mintSts == 0) && (idSts == 0))
        return;

    if ((mintSts & ALT_MSHC_INT_CMD_ERROR) != 0) 
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_IRQ, "CMD%d command error\n",
                         host->cmdIssued.cmdIdx, 2, 3, 4, 5, 6);
        host->cmdIssued.cmdErr |= SDMMC_CMD_ERR_PROTOCOL;
        }

    if ((mintSts & ALT_MSHC_INT_DATA_ERROR) != 0) 
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_IRQ, "CMD%d data error\n",
                         host->cmdIssued.cmdIdx, 2, 3, 4, 5, 6);
        logMsg ( "CMD%d data error mintSts = %x\n",
                                 host->cmdIssued.cmdIdx, mintSts, 3, 4, 5, 6);
        host->cmdIssued.cmdErr |= SDMMC_DATA_ERR_PROTOCOL;
        }

    if ((mintSts & ALT_MSHC_INT_CD) != 0) 
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_IRQ, "Card hot plug\n",
                         1, 2, 3, 4, 5, 6);
        semGive (pDrvCtrl->devChange);
        }

    if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_PIO)
        {
        if ((mintSts & ALT_MSHC_INT_RXDR) != 0)
            {
            needRead = TRUE;
            }

        if ((mintSts & ALT_MSHC_INT_TXDR) != 0)
            {
            needWrite = TRUE;
            }
        if ((mintSts & ALT_MSHC_INT_HTO) != 0)
            {
            if (host->cmdIssued.cmdData.isRead)
                needRead = TRUE;
            else
                needWrite = TRUE;
            }
        }

    if ((mintSts & ALT_MSHC_INT_DTO) != 0)
        {
        /* read remaining data for DTO */

        if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_PIO)
            {
            if (host->cmdIssued.cmdData.isRead)
                {
                needRead = TRUE;
                }
            
            /* data done for PIO read and write */
            
            semGive (pDrvCtrl->dataDone);
            }
        }

    if (((mintSts & ALT_MSHC_INT_CMD) != 0) )
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_IRQ, "CMD%d command complete\n",
                         host->cmdIssued.cmdIdx, 0, 0, 0, 0, 0);
        semGive (pDrvCtrl->cmdDone);
        }
    
    if (host->cmdIssued.cmdErr != 0)
        {
            
            logMsg ( "CMD%d data error mintSts = %x host->cmdIssued.cmdErr = %x\n",
                                             host->cmdIssued.cmdIdx, mintSts, host->cmdIssued.cmdErr, 4, 5, 6);
         }

    /* level sensitive, handle fifo before clear interrupts */

    if (needRead)
        {
        altSocGen5DwMshcReadFifo (pDev);
        }
    if (needWrite)
        {
        altSocGen5DwMshcWriteFifo (pDev);
        }

    CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, mintSts);

    /* DMA interrupts */

    if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_DMA)
        {
        if ((idSts & (ALT_MSHC_IDMA_INT_TI | ALT_MSHC_IDMA_INT_RI | 
                     ALT_MSHC_IDMA_INT_NI)) != 0)
            {
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_IRQ, 
                             "CMD%d DMA interrupt status: 0x%x\n",
                             host->cmdIssued.cmdIdx, idSts, 3, 4, 5, 6);

            /* data done for dma read and write */

            semGive (pDrvCtrl->dataDone);
            }

        CSR_WRITE_4 (pDev, ALT_MSHC_IDSTS, idSts);
        }
    }

#if 1
/*******************************************************************************
*
* altSocGen5DwMshcCardMonTask - card status monitor task
*
* This routine is the task loop to handle card insertion and removal.
*
* RETURN: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshcCardMonTask
    (
    VXB_DEVICE_ID pDev
    )
    {
    ALT_MSHC_DRV_CTRL * pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    device_t            dev;
    STATUS              rc;
    BOOL                inserted = FALSE;

    while (erfLibInitialized == FALSE)
        taskDelay (sysClkRateGet ());

    FOREVER
        {
        if (!pDrvCtrl->polling)
            {
            /* no need to check return value as the semaphore ID is correct */

            (void) semTake (pDrvCtrl->devChange, WAIT_FOREVER);
            }

        inserted = altSocGen5DwMshcCardDetect (pDev);
 
        if (inserted && (pDrvCtrl->cardIns == FALSE))
            {
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_XBD, "Card inserted\n",
                             1, 2, 3, 4, 5, 6);

            rc = sdMmcIdentify (&pDrvCtrl->card);  /* ref_712 sd dosfs*/
            /*rc = sdMmcIdentify_2 (&pDrvCtrl->card);  // sd / emmc dosfs*/
            /*rc = 1;*/
            
            if (rc == ERROR)
                {
                ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_XBD, 
                                 "SD/MMC_%d identification process failed\n",
                                 pDev->unitNumber, 2, 3, 4, 5, 6);
                continue;
                }

            /* already attached, do not create new xbd */

            if (pDrvCtrl->card.attached == TRUE)
                {
                ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_XBD, 
                                 "This should never happen unless hardware "
                                 "state is unstable\n",
                                 1, 2, 3, 4, 5, 6);
                continue;
                }

            pDrvCtrl->card.attached = TRUE;
			
#if 1 /* for vx dosfs*/

			if (pDrvCtrl->card.isMmc == 1)	/* 0-sd, 1-emmc*/
			{
				printf("sdMmcXbdDevCreate: emmc, pDev->unitNumber:%d \n", pDev->unitNumber);
			}
			else
			{
				printf("sdMmcXbdDevCreate: sd, pDev->unitNumber:%d \n", pDev->unitNumber);
			}
            dev = sdMmcXbdDevCreate (&pDrvCtrl->card, (char *)NULL);
            if (dev == NULLDEV)
                {
                ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                                 "sdMmcXbdXbdCreate() failed\n",
                                 1, 2, 3, 4, 5, 6);
                pDrvCtrl->card.attached = FALSE;
                continue;
                }
			
            /*printf("sdMmcXbdDevCreate ok \n");*/
            pDrvCtrl->cardIns = TRUE;
            }

        if (!inserted && (pDrvCtrl->cardIns == TRUE))
            {
            pDrvCtrl->card.attached = FALSE;
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_XBD, "Card removed\n",
                             1, 2, 3, 4, 5, 6);

            sdMmcXbdDevDelete (&pDrvCtrl->card);
            pDrvCtrl->cardIns = FALSE;
            }
#endif

        if (pDrvCtrl->polling)
            taskDelay (ALT_DW_MSHC_CARDMON_DELAY_SECS * sysClkRateGet ());
        }	
    }

#endif

/*******************************************************************************
*
* altSocGen5DwMshcSendCmd - send the command to the controller
*
* This routine sends the command to the controller and waits until it is 
* accepted.
*
* RETURNS: OK, or ERROR if command is not accepted by controller until time out.
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwMshcSendCmd
    (
    VXB_DEVICE_ID       pDev, 
    UINT32              cmd, 
    UINT32              arg
    )
    {
    UINT32              i;
    
    CSR_WRITE_4 (pDev, ALT_MSHC_CMDARG, arg);
    CSR_WRITE_4 (pDev, ALT_MSHC_CMD, (UINT32)ALT_MSHC_CMD_START | cmd);
 
    for (i = 0; i < ALT_MSHC_MAX_RETRIES; i++) 
        {
        if ((CSR_READ_4 (pDev, ALT_MSHC_CMD) & (UINT32)ALT_MSHC_CMD_START) == 0)
            {
            return OK;
            }
        vxbUsDelay(10);
        }

    ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                     "cmd=0x%x, arg=0x%x send timeout 0x%x\n",
                     cmd, arg, CSR_READ_4 (pDev, ALT_MSHC_CMD), 4, 5, 6);
    return ERROR;
    }

/*******************************************************************************
*
* altSocGen5DwMshcResetFifo - reset the internal fifo buffer
*
* This routine resets the internal fifo buffer.
*
* RETURNS: OK, or ERROR if fifo is not reset until time out.
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwMshcResetFifo
    (
    VXB_DEVICE_ID pDev
    )
    {
    UINT32 i;
    
    CSR_SETBIT_4 (pDev, ALT_MSHC_CTRL, ALT_MSHC_CTRL_FIFO_RESET);
    
    for (i = 0; i < ALT_MSHC_MAX_RETRIES; i++)
        {
        if ((CSR_READ_4 (pDev, ALT_MSHC_CTRL) & ALT_MSHC_CTRL_FIFO_RESET) == 0) 
            {
            return OK;
            }
        vxbUsDelay(10);
        }

    ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, "FIFO reset time out\n",
                     1, 2, 3, 4, 5, 6);
    return ERROR;
    }

/*******************************************************************************
*
* altSocGen5DwMshcIdmaStart - start the internal DMA controller of MSHC
*
* This routine prepares DMA descriptors and starts the internal DMA transfer.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshcIdmaStart
    (
    VXB_DEVICE_ID pDev
    )
    {    
    ALT_MSHC_DRV_CTRL * pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    SDMMC_HOST *        host = &pDrvCtrl->host;
    UINT32              bufAddr = (UINT32)host->cmdIssued.cmdData.buffer;
    ALT_MSHC_IDMA_DESC * pIdmaDesc = pDrvCtrl->idmaDesc;
    UINT32              round;
    UINT32              remain;
    void *              physAddr;
    UINT32              i;
    UINT32              dataLen;

    pIdmaDesc->des0 = SWAP32(ALT_MSHC_IDMA_DES0_FD);

    /* dual buffer mode */

    dataLen = host->cmdIssued.cmdData.blkNum * host->cmdIssued.cmdData.blkSize;
    round   = dataLen / (ALT_MSHC_IDMA_BUFFER_SIZE * 2);
    remain  = dataLen % (ALT_MSHC_IDMA_BUFFER_SIZE * 2);

    for (i = 0; i < round; i ++)
        {
        pIdmaDesc->des0 |= SWAP32(ALT_MSHC_IDMA_DES0_OWN | 
		                  ALT_MSHC_IDMA_DES0_DIC);
        pIdmaDesc->des1 = SWAP32(ALT_MSHC_IDMA_DES1 (ALT_MSHC_IDMA_BUFFER_SIZE,
                                 ALT_MSHC_IDMA_BUFFER_SIZE));

        physAddr = CACHE_DMA_VIRT_TO_PHYS (bufAddr);
        bufAddr += ALT_MSHC_IDMA_BUFFER_SIZE;
        pIdmaDesc->des2 = SWAP32(VXB_ADDR_LOW32 (physAddr));

        physAddr = CACHE_DMA_VIRT_TO_PHYS (bufAddr);
        bufAddr += ALT_MSHC_IDMA_BUFFER_SIZE;
        pIdmaDesc->des3 = SWAP32(VXB_ADDR_LOW32 (physAddr));

        pIdmaDesc ++;
        }

    if (remain != 0)
        {
        pIdmaDesc->des0 |= SWAP32(ALT_MSHC_IDMA_DES0_OWN | ALT_MSHC_IDMA_DES0_LD);
        pIdmaDesc->des0 &= SWAP32(~ALT_MSHC_IDMA_DES0_DIC);

        physAddr = CACHE_DMA_VIRT_TO_PHYS (bufAddr);
        pIdmaDesc->des2 = SWAP32(VXB_ADDR_LOW32 (physAddr));
        if (remain > ALT_MSHC_IDMA_BUFFER_SIZE)
            {
            bufAddr += ALT_MSHC_IDMA_BUFFER_SIZE;
            pIdmaDesc->des1 = SWAP32(ALT_MSHC_IDMA_DES1 (ALT_MSHC_IDMA_BUFFER_SIZE,
                                                  (remain - 
                                                   ALT_MSHC_IDMA_BUFFER_SIZE)));
            physAddr = CACHE_DMA_VIRT_TO_PHYS (bufAddr);
            pIdmaDesc->des3 = SWAP32(VXB_ADDR_LOW32 (physAddr));
            }
        else
            {
            pIdmaDesc->des1 = SWAP32(ALT_MSHC_IDMA_DES1 (remain, 0));
            pIdmaDesc->des3 = 0;
            }
        }
    else
        {
        pIdmaDesc --;
        pIdmaDesc->des0 |= SWAP32(ALT_MSHC_IDMA_DES0_OWN | 
                                  ALT_MSHC_IDMA_DES0_LD);
        pIdmaDesc->des0 &= SWAP32(~ALT_MSHC_IDMA_DES0_DIC);
        }

    CSR_SETBIT_4 (pDev, ALT_MSHC_CTRL, ALT_MSHC_CTRL_DMA_ENABLE |
                  ALT_MSHC_CTRL_USE_IDMAC);
    CSR_SETBIT_4 (pDev, ALT_MSHC_BMOD, ALT_MSHC_BMOD_FB | ALT_MSHC_BMOD_DE);
    CSR_WRITE_4 (pDev, ALT_MSHC_PLDMND, 1);
    
    }

/*******************************************************************************
*
* altSocGen5DwMshcIdmaStop - stop the internal DMA controller of MSHC
*
* This routine resets DMA descriptors and stop the internal DMA transfer.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshcIdmaStop
    (
    VXB_DEVICE_ID pDev
    )
    {
    ALT_MSHC_DRV_CTRL *  pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    ALT_MSHC_IDMA_DESC * idmaDesc;
    UINT32               tmp;

    idmaDesc = pDrvCtrl->idmaDesc;

    tmp = CSR_READ_4 (pDev, ALT_MSHC_RINTSTS);
    bzero ((char *)pDrvCtrl->idmaDesc, 
           sizeof (ALT_MSHC_IDMA_DESC) * ALT_MSHC_IDMA_DESC_NUM);

    tmp = CSR_READ_4 (pDev, ALT_MSHC_CTRL);
    tmp &= ~(ALT_MSHC_CTRL_DMA_ENABLE | ALT_MSHC_CTRL_USE_IDMAC);
    tmp |= ALT_MSHC_CTRL_DMA_RESET;
    CSR_WRITE_4 (pDev, ALT_MSHC_CTRL, tmp);
    
    while ((CSR_READ_4 (pDev, ALT_MSHC_CTRL) & (ALT_MSHC_CTRL_DMA_RESET)) != 0)
        ;

    CSR_CLRBIT_4 (pDev, ALT_MSHC_BMOD, ALT_MSHC_BMOD_FB | ALT_MSHC_BMOD_DE);
    }

/*******************************************************************************
*
* altSocGen5DwMshcCmdPrepare - prepare the command to be sent
*
* This routine prepares the command to be sent.
*
* RETURNS: OK, or ERROR if wrong parameters or command is not accepted.
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwMshcCmdPrepare
    (
    SDMMC_HOST * host
    )
    {
    VXB_DEVICE_ID       pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    SDMMC_CMD *         cmd;
    STATUS              status;
    UINT32              i;
    UINT32              dataLen;
    UINT32              count = 0;
    UINT32              cmdSend = 0;
    UINT32 *            buf32;

    if (host == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshcCmdPrepare: host is NULL\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    pDev = host->pDev;
    if (pDev == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshcCmdPrepare: pDev is NULL\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshcCmdPrepare: pDrvCtrl is NULL\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    cmd =&host->cmdIssued;
    if (cmd == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshcCmdPrepare: cmd is NULL\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    cmdSend = ALT_MSHC_CMD_INDX (cmd->cmdIdx);

    if (cmdSend == SDMMC_CMD_STOP_TRANSMISSION)
        cmdSend |= ALT_MSHC_CMD_STOP;
    else
        cmdSend |= ALT_MSHC_CMD_PRV_DAT_WAIT;

    if (cmd->rspType != SDMMC_CMD_RSP_NONE) 
        {
        cmdSend |= ALT_MSHC_CMD_RESP_EXP;
        if (cmd->rspType & SDMMC_CMD_RSP_LEN136)
            cmdSend |= ALT_MSHC_CMD_RESP_LONG;
        }

    if (cmd->rspType & SDMMC_CMD_RSP_CRC)
        cmdSend |= ALT_MSHC_CMD_RESP_CRC;

    if (cmd->cmdIdx == SDMMC_CMD_GO_IDLE_STATE)
        {
        cmdSend |= ALT_MSHC_CMD_INIT;
        }

    /* always use hold reg as drvsel is not zero */
    
    cmdSend |= ALT_MSHC_CMD_USE_HOLD_REG; 

    if (cmd->hasData) 
        {
        status = altSocGen5DwMshcResetFifo (pDev);
        if (status == ERROR)
            return ERROR;

        dataLen = cmd->cmdData.blkNum *cmd->cmdData.blkSize;
        buf32   = (UINT32 *)cmd->cmdData.buffer;
        if ((buf32 == NULL) || (dataLen == 0))
            {
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                             "altSocGen5DwMshcCmdPrepare: wrong cmd data\n",
                             1, 2, 3, 4, 5, 6);
            return ERROR;
            }

        CSR_WRITE_4 (pDev, ALT_MSHC_BYTCNT, dataLen);
        CSR_WRITE_4 (pDev, ALT_MSHC_BLKSIZ, cmd->cmdData.blkSize);
        pDrvCtrl->remainBytes = dataLen;

        cmdSend |= ALT_MSHC_CMD_DATA_EXP;

        if (cmd->cmdData.blkNum > 1)
            {
            cmdSend |= ALT_MSHC_CMD_SEND_AUTO_STOP;
            }
        if (!cmd->cmdData.isRead) 
            cmdSend |= ALT_MSHC_CMD_DATA_WR;

        if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_DMA)
            {
            altSocGen5DwMshcIdmaStart (pDev);
            }
        else
            {
            /* PIO mode */
            
            if (!cmd->cmdData.isRead) 
                {
                /* always multiple of ALT_MSHC_FIFO_STEPS */
                
                if ((dataLen / ALT_MSHC_FIFO_STEPS) >= 
                    ALT_MSHC_DEFAULT_FIFO_LEN) 
                    {
                    count = ALT_MSHC_DEFAULT_FIFO_LEN;
                    }
                else
                    {
                    count = dataLen / ALT_MSHC_FIFO_STEPS;
                    }

                for (i = 0; i < count; i++) 
                    {
                    CSR_WRITE_4 (pDev, ALT_MSHC_DATA, SWAP32(*buf32));
                    buf32++;
                    }

                pDrvCtrl->remainBytes -= count * ALT_MSHC_FIFO_STEPS;
                }
            }
        }

    return (altSocGen5DwMshcSendCmd (pDev, cmdSend, cmd->cmdArg));
    }

/*******************************************************************************
*
* altSocGen5DwMshcCmdIssue - issue the command
*
* This routine issues the command and checks the command status.
*
* RETURNS: OK, or ERROR if the command can not be complete successfully.
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwMshcCmdIssue
    (
    SDMMC_HOST * host
    )
    {
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    SDMMC_CMD *         cmd;
    VXB_DEVICE_ID       pDev;
    STATUS              rc;
    UINT32              mintsts;
    UINT32              idsts;
    UINT32              i;

    rc = altSocGen5DwMshcCmdPrepare (host);

    if (rc == ERROR)
        return ERROR;

    pDrvCtrl = (ALT_MSHC_DRV_CTRL *)host->pDev->pDrvCtrl;
    cmd      = &host->cmdIssued;
    pDev     = host->pDev;

    if (!pDrvCtrl->polling)
        {
        rc = semTake (pDrvCtrl->cmdDone,
                      (sysClkRateGet() * ALT_DW_MSHC_CMD_WAIT_IN_SECS));
        if (rc == ERROR)
            {
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                             "altSocGen5DwMshcCmdIssue: CMD%d timeout-(sd_%d)\n",
                             cmd->cmdIdx, host->pDev->unitNumber, 3, 4, 5, 6);
            cmd->cmdErr |= SDMMC_CMD_ERR_TIMEOUT;
            return ERROR;
            }
        else
            {
            if (cmd->cmdErr != 0)
                {
                ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                                 "altSocGen5DwMshcCmdIssue: CMD%d error\n",
                                 cmd->cmdIdx, 2, 3, 4, 5, 6);
                return ERROR;
                }
            }
        }
    else
        {
        /* poll and check status */
         
        for (i = 0; i < ALT_MSHC_MAX_RETRIES; i++) 
            {
            mintsts = CSR_READ_4 (pDev,  ALT_MSHC_MINTSTS);
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_CMD, 
                             "altSocGen5DwMshcCmdIssue: mintsts 0x%x\n",
                             mintsts, 2, 3, 4, 5, 6);
    
            if ((mintsts & ALT_MSHC_INT_CMD) != 0)
                {
                CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, mintsts);
                break;
                }

            if ((mintsts & ALT_MSHC_INT_RTO) != 0) 
            {
                  /* jc*/
                ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                                 "altSocGen5DwMshcCmdIssue: "
                                 "response time out-(sd_%d)\n",  host->pDev->unitNumber, 2, 3, 4, 5, 6);
				/**/
				
                CSR_WRITE_4(pDev, ALT_MSHC_RINTSTS, mintsts);
                cmd->cmdErr |= SDMMC_CMD_ERR_TIMEOUT;
                return ERROR;
            }

            if (((mintsts & ALT_MSHC_INT_RCRC) != 0) && 
                ((cmd->rspType & SDMMC_CMD_RSP_CRC) != 0))
                {
                ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                                 "altSocGen5DwMshcCmdIssue: "
                                 "response CRC error\n",
                                 1, 2, 3, 4, 5, 6);
                CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, mintsts);
                cmd->cmdErr |= SDMMC_CMD_ERR_PROTOCOL;
                return ERROR;
                }

            if ((mintsts & ALT_MSHC_INT_RE) != 0)
                {
                  /* jc*/
                ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                                 "altSocGen5DwMshcCmdIssue: response error\n",
                                 1, 2, 3, 4, 5, 6);
				/**/
                CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, mintsts);
                cmd->cmdErr |= SDMMC_CMD_ERR_PROTOCOL;
                return ERROR;
                }

            vxbUsDelay(10);
            }

        if (i >= ALT_MSHC_MAX_RETRIES)
            {
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                             "altSocGen5DwMshcCmdIssue: CMD%d timeout-(sd_%d)\n",
                             cmd->cmdIdx, host->pDev->unitNumber, 3, 4, 5, 6);
            cmd->cmdErr |= SDMMC_CMD_ERR_TIMEOUT;
            return ERROR;
            }
        }

    if (cmd->rspType != SDMMC_CMD_RSP_NONE)
        {
        if ((cmd->rspType & SDMMC_CMD_RSP_LEN136) != 0)
            {
            UINT32 cmdRsp[4];

            cmdRsp[0] = CSR_READ_4 (pDev, ALT_MSHC_RESP0);
            cmdRsp[1] = CSR_READ_4 (pDev, ALT_MSHC_RESP1);
            cmdRsp[2] = CSR_READ_4 (pDev, ALT_MSHC_RESP2);
            cmdRsp[3] = CSR_READ_4 (pDev, ALT_MSHC_RESP3);

            cmd->cmdRsp[0] = be32toh (cmdRsp[3]);
            cmd->cmdRsp[1] = be32toh (cmdRsp[2]);
            cmd->cmdRsp[2] = be32toh (cmdRsp[1]);
            cmd->cmdRsp[3] = be32toh (cmdRsp[0]);
            }
        else
            {
            cmd->cmdRsp[0] = CSR_READ_4 (pDev, ALT_MSHC_RESP0);
            cmd->cmdRsp[1] = 0;
            cmd->cmdRsp[2] = 0;
            cmd->cmdRsp[3] = 0;
            }
        }

    if (cmd->hasData)
        {
        if (!pDrvCtrl->polling)
            {
            /* interrupt mode */

            rc = semTake (pDrvCtrl->dataDone, 
                          (sysClkRateGet() * ALT_DW_MSHC_CMD_WAIT_IN_SECS));
            if (rc == ERROR)
                {
                ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                                 "altSocGen5DwMshcCmdIssue: data time out\n",
                                 1, 2, 3, 4, 5, 6);
                cmd->dataErr |= SDMMC_DATA_ERR_TIMEOUT;
                return ERROR;
                }
            else
                {
                if (cmd->dataErr != 0)
                    {
                    ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                                     "altSocGen5DwMshcCmdIssue: data error\n",
                                     1, 2, 0, 0, 0, 0);
                    return ERROR;
                    }
                }
            }
        else
            {
            /* poll mode */

            if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_PIO)
                {
                /* pio */

                if (cmd->cmdData.isRead)
                    {
                    rc = altSocGen5DwMshcPioRead (pDev);
                    if (rc == ERROR)
                        {
                        return ERROR;
                        }
                    }
                else
                    {
                    rc = altSocGen5DwMshcPioWrite (pDev);
                    if (rc == ERROR)
                        {
                        return ERROR;
                        }

                    rc = altSocGen5DwMshcPollIntsts (pDev, ALT_MSHC_INT_DTO);
                    if (rc == ERROR)
                        {
                        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_RW, 
                                         "PIO write waiting data over failed\n",
                                         1, 2, 3, 4, 5, 6);
                        return ERROR;
                        }
                    }
                }
            else
                {
                /* dma */

                for (i = 0; i < ALT_MSHC_MAX_RETRIES; i++) 
                    {
                    idsts = CSR_READ_4 (pDev,  ALT_MSHC_IDSTS);
                    ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_CMD, 
                                     "altSocGen5DwMshcCmdIssue: idsts 0x%x\n",
                                     idsts, 2, 3, 4, 5, 6);
            
                    if ((idsts & (ALT_MSHC_IDMA_INT_TI | ALT_MSHC_IDMA_INT_RI |
                                  ALT_MSHC_IDMA_INT_NI)) != 0)
                        {
                        CSR_WRITE_4 (pDev, ALT_MSHC_IDSTS, idsts);
                        break;
                        }

                    if ((idsts & ALT_MSHC_IDMA_INT_AI) != 0) 
                        {
                        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                                         "altSocGen5DwMshcCmdIssue: "
                                         "CMD%d dma error\n",
                                         cmd->cmdIdx, 2, 3, 4, 5, 6);
                        CSR_WRITE_4 (pDev, ALT_MSHC_IDSTS, idsts);
                        cmd->cmdErr |= SDMMC_DATA_ERR_PROTOCOL;
                        break;
                        }

                    vxbMsDelay (1);
                    }

                if (i >= ALT_MSHC_MAX_RETRIES)
                    {
                    ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                                     "altSocGen5DwMshcCmdIssue: "
                                     "CMD%d dma timeout\n",
                                     cmd->cmdIdx, 2, 3, 4, 5, 6);
                    cmd->cmdErr |= SDMMC_DATA_ERR_TIMEOUT;
                    }
                }
            }

        /* stop internal dma */
        
        if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_DMA)
            {
            altSocGen5DwMshcIdmaStop (pDev);
            if (cmd->cmdErr != 0)
                return ERROR;
            }
        }

    return OK;
    }

/*******************************************************************************
*
* altSocGen5DwMshcBusWidthSetup - setup the bus width
*
* This routine sets up the bus width.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshcBusWidthSetup
    (
    SDMMC_HOST * host,
    UINT32       width
    )
    {
    UINT32       buswidth;

    buswidth = ALT_MSHC_CTYPE_1BIT;
    
    switch (width) 
        {
        case SDMMC_BUS_WIDTH_1BIT:
            buswidth = ALT_MSHC_CTYPE_1BIT;
            break;
        case SDMMC_BUS_WIDTH_4BIT:
            buswidth = ALT_MSHC_CTYPE_4BIT;
            break;
        case SDMMC_BUS_WIDTH_8BIT:
            buswidth = ALT_MSHC_CTYPE_8BIT;
            break;
        default:
            /* should not be there but use default */

            break;
        }
            
    CSR_WRITE_4 (host->pDev, ALT_MSHC_CTYPE, buswidth);
    }

/*******************************************************************************
*
* altSocGen5DwMshcCardWpCheck - check if card is write protected
*
* This routine checks if card is write protected.
*
* RETURNS: FALSE or TRUE if card is write protected.
*
* ERRNO: N/A
*/

LOCAL BOOL altSocGen5DwMshcCardWpCheck
    (
    SDMMC_HOST * host
    )
    {
    VXB_DEVICE_ID       pDev = host->pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    BOOL                wp = FALSE;

    if ((pDrvCtrl->flags & ALT_MSHC_FLAGS_CARD_WRITABLE) != 0)
        {
        wp = FALSE;
        }
    else
        {
        wp = (CSR_READ_4 (pDev, ALT_MSHC_WRTPRT) & 1) != 0;
        }

    return wp;
    }

/*******************************************************************************
*
* altSocGen5DwMshcCardDetect - check if card is present
*
* This routine checks if card is plugged in.
*
* RETURNS: FALSE, or TRUE if card is plugged in.
*
* ERRNO: N/A
*/

LOCAL BOOL altSocGen5DwMshcCardDetect
    (
    VXB_DEVICE_ID pDev
    )
    {
    ALT_MSHC_DRV_CTRL * pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    BOOL                present = FALSE;

    if ((pDrvCtrl->flags & ALT_MSHC_FLAGS_CARD_PRESENT) != 0)
        present = TRUE;
    else
        {
        present = (CSR_READ_4 (pDev, ALT_MSHC_CDETECT) & 1) == 0;
        }

    return present;
    }

/*******************************************************************************
*
* altSocGen5DwMshcBlkRead - read block from SD/MMC card
*
* This routine reads block from SD/MMC card.
*
* RETURNS: OK, or ERROR if read failed.
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwMshcBlkRead
    (
    SDMMC_CARD *        card,
    sector_t            blkNo,
    UINT32              numBlks,
    void *              pBuf
    )
    {
    VXB_DEVICE_ID       pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    STATUS              rc;
    UINT32              ix, round, remain;
    void *              bufAddr;
    sector_t            blkAddr;

    if ((card == NULL) || (numBlks == 0) || (pBuf == NULL))
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshcBlkRead: parameter wrong\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    pDev = card->host->pDev;
    if (pDev == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshcBlkRead: parameter wrong\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshcBlkRead: parameter wrong\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    /* split the transfer */

    round = numBlks / ALT_MSHC_MAX_RW_SECTORS;
    remain = numBlks % ALT_MSHC_MAX_RW_SECTORS;

    for (ix = 0; ix < round; ix ++)
        {
        blkAddr = blkNo + ix * ALT_MSHC_MAX_RW_SECTORS;
        bufAddr = (void *)((ULONG)pBuf + ix * ALT_MSHC_MAX_RW_SECTORS *
                           SDMMC_BLOCK_SIZE); 

        rc = sdMmcBlkRead (card, blkAddr, ALT_MSHC_MAX_RW_SECTORS, bufAddr);
        if (rc == ERROR)
            return ERROR;

        sdMmcStatusWaitReadyForData (card);

        /* if hardware does not support snoop, we invalidate cache */

        if ((pDrvCtrl->flags & ALT_MSHC_FLAGS_SNOOP_ENABLED) == 0)
            CACHE_USER_INVALIDATE (bufAddr,
                                  (ALT_MSHC_MAX_RW_SECTORS * SDMMC_BLOCK_SIZE));
        }

     if (remain != 0)
         {
         blkAddr = blkNo + round * ALT_MSHC_MAX_RW_SECTORS;
         bufAddr = (void *)((ULONG)pBuf + round * ALT_MSHC_MAX_RW_SECTORS *
                            SDMMC_BLOCK_SIZE);

         rc = sdMmcBlkRead (card, blkAddr, remain, bufAddr);
         if (rc == ERROR)
             return ERROR;

         sdMmcStatusWaitReadyForData (card);

         /* if hardware does not support snoop, we invalidate cache */

         if ((pDrvCtrl->flags & ALT_MSHC_FLAGS_SNOOP_ENABLED) == 0)
             CACHE_USER_INVALIDATE (bufAddr, (remain * SDMMC_BLOCK_SIZE));
         }

    return OK;
    }

/*******************************************************************************
*
* altSocGen5DwMshcBlkWrite - write block to SD/MMC card
*
* This routine writes block to SD/MMC card.
*
* RETURNS: OK, or ERROR if write failed.
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwMshcBlkWrite
    (
    SDMMC_CARD *        card,
    sector_t            blkNo,
    UINT32              numBlks,
    void *              pBuf
    )
    {
    VXB_DEVICE_ID       pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    STATUS              rc;
    UINT32              ix, round, remain;
    sector_t            blkAddr;
    void *              bufAddr;

    if ((card == NULL) || (numBlks == 0) || (pBuf == NULL))
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshcBlkRead: parameter wrong\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    pDev = card->host->pDev;
    if (pDev == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshcBlkRead: parameter wrong\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshcBlkRead: parameter wrong\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
        }
        
    /* split the transfer */

    round = numBlks / ALT_MSHC_MAX_RW_SECTORS;
    remain = numBlks % ALT_MSHC_MAX_RW_SECTORS;

    for (ix = 0; ix < round; ix ++)
        {
        bufAddr = (void *)((ULONG)pBuf + ix * ALT_MSHC_MAX_RW_SECTORS *
                           SDMMC_BLOCK_SIZE);
        blkAddr = blkNo + ix * ALT_MSHC_MAX_RW_SECTORS;

        /* if hardware does not support snoop, we flush cache */

        if ((pDrvCtrl->flags & ALT_MSHC_FLAGS_SNOOP_ENABLED) == 0)
            CACHE_USER_FLUSH (bufAddr,
                              (ALT_MSHC_MAX_RW_SECTORS * SDMMC_BLOCK_SIZE));
        
        rc = sdMmcBlkWrite (card, blkAddr, ALT_MSHC_MAX_RW_SECTORS, bufAddr);
        if (rc == ERROR )
            return ERROR;

        sdMmcStatusWaitReadyForData (card);
        }

    if (remain != 0)
        {
        bufAddr = (void *)((ULONG)pBuf + round * ALT_MSHC_MAX_RW_SECTORS * 
                           SDMMC_BLOCK_SIZE);
        blkAddr = blkNo + round * ALT_MSHC_MAX_RW_SECTORS;

        if ((pDrvCtrl->flags & ALT_MSHC_FLAGS_SNOOP_ENABLED) == 0)
            CACHE_USER_FLUSH (bufAddr, (remain * SDMMC_BLOCK_SIZE));
        
        rc = sdMmcBlkWrite (card, blkAddr, remain, bufAddr);
        if (rc == ERROR )
            return ERROR;

        sdMmcStatusWaitReadyForData(card);
        }

    return OK;
    }

/*******************************************************************************
*
* altSocGen5DwMshcPollIntsts - poll the interrupt status register
*
* This routine polls the status of the interrupt status register.
*
* RETURNS: OK, or ERROR if there's wrong status.
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwMshcPollIntsts
    (
    VXB_DEVICE_ID       pDev,
    UINT32              mask
    )
    {
    UINT32              intsts;
    UINT32              i;
    
    for (i = 0; i < ALT_MSHC_MAX_RETRIES; i++)
        {
        intsts = CSR_READ_4 (pDev, ALT_MSHC_RINTSTS);

        if ((intsts & ALT_MSHC_INT_ERROR) != 0)
            {
            CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, intsts);
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                             "Poll MINTSTS failed with 0x%x\n", 
                             intsts, 2, 3, 4, 5, 6);
            return ERROR;
            }

        if ((intsts & mask) != 0)
            {
            CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, (intsts & mask));
            return OK;
            }

        vxbUsDelay (100);
        }

    CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, intsts);
    ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_RW, "Poll MINTSTS time out with 0x%x\n", 
                     intsts, 2, 3, 4, 5, 6);
    return ERROR;
    }

/*******************************************************************************
*
* altSocGen5DwMshcReadFifo - PIO read the internal FIFO buffer
*
* This routine reads the internal FIFO buffer using PIO method.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshcReadFifo
    (
    VXB_DEVICE_ID       pDev
    )
    {    
    ALT_MSHC_DRV_CTRL * pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    SDMMC_HOST *        host = &pDrvCtrl->host;
    UINT32 *            buf = (UINT32 *)host->cmdIssued.cmdData.buffer;
    UINT32              i;
    UINT32              bytesRead;
    UINT32              mshcStatus;
    UINT32              fifoLen;

    bytesRead = host->cmdIssued.cmdData.blkNum * host->cmdIssued.cmdData.blkSize
                - pDrvCtrl->remainBytes;
    buf += bytesRead / ALT_MSHC_FIFO_STEPS;
    
    mshcStatus = CSR_READ_4 (pDev, ALT_MSHC_STATUS);
    fifoLen = ALT_MSHC_STATUS_FIFO_COUNT (mshcStatus);

    for (i = 0; i < fifoLen; i++)
        {
        *buf = SWAP32(CSR_READ_4(pDev, ALT_MSHC_DATA));
        buf++;
        }

    pDrvCtrl->remainBytes -= fifoLen * ALT_MSHC_FIFO_STEPS;
    ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_RW, "PIO read %d bytes, remain %d bytes\n", 
                     fifoLen * 4, pDrvCtrl->remainBytes, 3, 4, 5, 6);
    }

/*******************************************************************************
*
* altSocGen5DwMshcWriteFifo - PIO write the internal FIFO buffer
*
* This routine writes the internal FIFO buffer using PIO method.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshcWriteFifo
    (
    VXB_DEVICE_ID       pDev
    )
    {
    ALT_MSHC_DRV_CTRL * pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    SDMMC_HOST *        host = &pDrvCtrl->host;
    UINT32 *            buf = (UINT32 *)host->cmdIssued.cmdData.buffer;
    UINT32              i;
    UINT32              mshcStatus;
    UINT32              bytesWritten;
    UINT32              fifoSlots;
    UINT32              writeLen;    

    bytesWritten = host->cmdIssued.cmdData.blkNum * 
                   host->cmdIssued.cmdData.blkSize - pDrvCtrl->remainBytes;
    buf += bytesWritten / ALT_MSHC_FIFO_STEPS;
 
    mshcStatus =  CSR_READ_4 (pDev, ALT_MSHC_STATUS);
    fifoSlots = ALT_MSHC_STATUS_FIFO_COUNT (mshcStatus);
    fifoSlots = ALT_MSHC_DEFAULT_FIFO_LEN - fifoSlots;
    
    if (pDrvCtrl->remainBytes > (fifoSlots * ALT_MSHC_FIFO_STEPS))
        {
        writeLen  = fifoSlots;
        } 
    else 
        {
        writeLen = pDrvCtrl->remainBytes / ALT_MSHC_FIFO_STEPS;
        }

    for (i = 0; i < writeLen; i++)
        {
        CSR_WRITE_4 (pDev, ALT_MSHC_DATA, SWAP32(*buf));
        buf++;
        }

    pDrvCtrl->remainBytes -= writeLen * ALT_MSHC_FIFO_STEPS;
    ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_RW, "PIO write %d bytes, remain %d bytes\n", 
                     writeLen * 4, pDrvCtrl->remainBytes, 3, 4, 5, 6);
    }

/*******************************************************************************
*
* altSocGen5DwMshcPioRead - read data from SD/MMC card using PIO
*
* This routine reads data from SD/MMC card using PIO.
*
* RETURNS: OK, or ERROR if read failed.
*
* ERRNO: N/A
*/
#define FIFO_EMPTY                      0x00000004
#define FIFO_FULL                       0x00000008
#define FM_SDMMC_DATA_FIFO                   (0x100)
#define RAW_INT_STATUS_DTO              0x0008
LOCAL STATUS altSocGen5DwMshcPioRead
    (
    VXB_DEVICE_ID       pDev
    )
    { 
#if 1
    ALT_MSHC_DRV_CTRL * pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    STATUS              status;

    while (pDrvCtrl->remainBytes != 0) 
        {
        status = altSocGen5DwMshcPollIntsts (pDev,
                                             ALT_MSHC_INT_RXDR | 
                                             ALT_MSHC_INT_DTO  | 
                                             ALT_MSHC_INT_HTO);
        if (status == ERROR)
            {
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                             "PIO read error, remain %d bytes\n", 
                             pDrvCtrl->remainBytes, 2, 3, 4, 5, 6);
            return ERROR;
            }

        altSocGen5DwMshcReadFifo (pDev);
        vxbUsDelay (100);
        }

    return OK;
#endif
#if 0
        UINT32 cnt = 0;
        ALT_MSHC_DRV_CTRL * pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
            SDMMC_HOST *        host = &pDrvCtrl->host;
            UINT8 *            pBuf = (UINT32 *)host->cmdIssued.cmdData.buffer;
           int timeout = 0x200000;
           if(pDrvCtrl->remainBytes == 0) return OK;
        while (pDrvCtrl->remainBytes != 0 ) 
            {
        	         timeout--;
        		
        	        if (!(CSR_READ_4 (pDev, ALT_MSHC_STATUS) & FIFO_EMPTY))
        	        {
        	        	if(pDrvCtrl->remainBytes >= 4)    			
        	        	{
        	                *(UINT32 *)(pBuf + cnt) = CSR_READ_4 (pDev, FM_SDMMC_DATA_FIFO);
        	                cnt += 4;
        	                pDrvCtrl->remainBytes -=4;
        	        	 }
        	        	else
        	        	{
        	        		printf("altSocGen5DwMshcPioRead align\n");
        	        		UINT32 tmpVal = CSR_READ_4 (pDev, FM_SDMMC_DATA_FIFO);
        			        memcpy(pBuf+cnt,&tmpVal,pDrvCtrl->remainBytes) ;    			    
        			        pDrvCtrl->remainBytes = 0;
        	        	}
        	        }
        		
        		
        	        if (timeout <= 0)
        	        {
        	        	printf("altSocGen5DwMshcPioRead : cnt = %d, pDrvCtrl->remainBytes = %d",cnt,pDrvCtrl->remainBytes);
        	        	return ERROR;
        	        }
            }
#endif    
        	return OK;   
    }

/*******************************************************************************
*
* altSocGen5DwMshcPioWrite - write data to SD/MMC card using PIO
*
* This routine writes data to SD/MMC card using PIO.
*
* RETURNS: OK, or ERROR if write failed
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwMshcPioWrite
    (
    VXB_DEVICE_ID       pDev
    )
    {
    ALT_MSHC_DRV_CTRL * pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    STATUS              status;
    
    while (pDrvCtrl->remainBytes != 0) 
        {
        status = altSocGen5DwMshcPollIntsts (pDev, 
                                             ALT_MSHC_INT_TXDR | 
                                             ALT_MSHC_INT_HTO);

        if (status == ERROR)
            {
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                             "PIO write error, remain %d bytes\n", 
                             pDrvCtrl->remainBytes, 2, 3, 4, 5, 6);
            return ERROR;
            }

        altSocGen5DwMshcWriteFifo (pDev);
        vxbUsDelay (100);
        }

    return OK;
}

/*******************************************************************************
*
* altSocGen5DwMshcShow - print MSHC controller and device related information
*
* This routine prints MSHC controller and device related information.
*
* RETURNS: OK, or ERROR if parameter is wrong.
*
* ERRNO: N/A
*/

STATUS altSocGen5DwMshcShow
    (
    int                 unit
    )
    {
    VXB_DEVICE_ID       pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl;

    pDev = vxbInstByNameFind (ALT_DW_MSHC_NAME, unit);
    if (pDev == NULL)
        return ERROR;

    pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;

    printf ("[%s]: regBase @ %p, rwMode (%s) polling (%s) flags (0x%x)\n",
            ALT_DW_MSHC_NAME,
            pDrvCtrl->regBase, 
            (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_DMA) ? "DMA" : "PIO",
            (pDrvCtrl->polling) ? "TRUE" : "FALSE",
            pDrvCtrl->flags);

    sdMmcHostShow (&pDrvCtrl->host);
    if (pDrvCtrl->cardIns)
        sdMmcCardShow (&pDrvCtrl->card);
    else
        printf ("\nNo SD/MMC card inserted\n");

    return OK;
    }

/*******************************************************************************
*
* altSocGen5DwMshcClkFreqSetup - setup the clock frequency
*
* This routine sets up the clock frequency.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshcClkFreqSetup
    (
    SDMMC_HOST *        host,
    UINT32              clk
    )
    {
    VXB_DEVICE_ID       pDev = host-> pDev ;
    ALT_MSHC_DRV_CTRL * pDrvCtrl = ( ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl ;

    if (pDrvCtrl->clkFreqSetup != NULL)
    {
        pDrvCtrl-> clkFreqSetup (clk);

    }
    else
    {
       printf ("need pDrvCtrl->clkFreqSetup!!\n" );

    }
#if 0  /*syx*/
    if (clk >= clkFreq)
        {
        div = 0;
        clk = clkFreq;
        }
    else if (clkFreq % (clk * 2))
        {
        div = ((clkFreq / clk) >> 1) + 1;
        }
    else
        {
        div = (clkFreq  / clk) >> 1;
        }

    if (freqSaved == clk)
        return;
    else
        freqSaved = clk;

    CSR_WRITE_4 (pDev, ALT_MSHC_CLKENA, 0);
    CSR_WRITE_4 (pDev, ALT_MSHC_CLKSRC, 0);
     
    altSocGen5DwMshcSendCmd (pDev, ALT_MSHC_CMD_UPD_CLK |
                            ALT_MSHC_CMD_PRV_DAT_WAIT, 0);

    if (pDrvCtrl->clkFreqSetup != NULL)
        {
        pDrvCtrl->clkFreqSetup (clk);
        }

    CSR_WRITE_4 (pDev, ALT_MSHC_CLKDIV, div);
    CSR_WRITE_4 (pDev, ALT_MSHC_CLKENA, ALT_MSHC_CLKENA_CCLK_ENABLE);

    altSocGen5DwMshcSendCmd (pDev, ALT_MSHC_CMD_UPD_CLK |
                             ALT_MSHC_CMD_PRV_DAT_WAIT, 0);
#endif
}





/*******************************************************************************
*
* altSocGen5DwMshcVddSetup - setup the SD bus voltage level and power it up
*
* This routine sets up the SD bus voltage and powers it up.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshcVddSetup
    (
    SDMMC_HOST *        host,
    UINT32              vdd
    )
    {
    CSR_WRITE_4 (host->pDev, ALT_MSHC_PWREN, 1);
    vxbMsDelay(100);
    }

/* use timestamp to test performance, must add INCLUDE_TIMESTAMP component */

#ifdef ALT_DW_MSHC_DBG_ON

#include <tickLib.h>

/*******************************************************************************
*
* altSocGen5DwMshcTest - test SD card raw read and write
*
* This routine tests SD card raw block read and write.
*
* RETURNS: OK, or ERROR if parameter is wrong.
*
* ERRNO: N/A
*
* \NOMANUAL
*
*/

STATUS altSocGen5DwMshcTest
    (
    int                 unit,
    UINT32              blkStart,
    UINT32              blkNum
    )
    {
    VXB_DEVICE_ID       pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    SDMMC_CARD *        card;
    UINT32              i, j, k;
    UINT8 *             pBuf;
    UINT32              timeStart, timeEnd;
    UINT32              tickStart, tickEnd, tickDiff, timeDiff;
    float               speed;

    pDev = vxbInstByNameFind (ALT_DW_MSHC_NAME, unit);
    if (pDev == NULL)
        {
        printf ("Can not find this sd unit %d.\n", unit);
        return ERROR;
        }
    pDrvCtrl= (ALT_MSHC_DRV_CTRL *) pDev->pDrvCtrl;
    card = &pDrvCtrl->card;

    if ((blkStart + blkNum) > card->info.blkNum)
        {
        printf ("Test block exceeds the sd card capacity.\n");
        return ERROR;
        }

    pBuf = (UINT8 *)cacheDmaMalloc (blkNum * SDMMC_BLOCK_SIZE);
    if (pBuf == NULL)
        return ERROR;

    printf ("Test buffer addr = 0x%08x.\n", pBuf);

    for (i = 0; i < blkNum * SDMMC_BLOCK_SIZE; i++)
        {
        pBuf[i] = i%256;
        }

    vxbTimestampEnable();
    timeStart = vxbTimestamp();
    tickStart = tickGet();

    if (altSocGen5DwMshcBlkWrite (card, blkStart, blkNum, pBuf) == ERROR)
        {
        printf ("Write error!\n");
        cacheDmaFree (pBuf);
        return ERROR;
        }

    timeEnd = vxbTimestamp();
    tickEnd = tickGet();
    tickDiff = tickEnd - tickStart;

    if (tickDiff > 1)
        speed = (double)blkNum / 2 / 
                       ((double)tickDiff / (double)sysClkRateGet());
    else
        {
        if (tickDiff == 0)
            timeDiff = timeEnd - timeStart;
        else
            timeDiff = timeEnd + (vxbTimestampPeriod ()  - timeStart);
        speed =  (double)blkNum / 2 / 
                        ((double)timeDiff / (double)vxbTimestampFreq());
        }

    printf ("Write finished, write speed = %f KB/s.\n", speed);

    bzero ((char *)pBuf, blkNum * SDMMC_BLOCK_SIZE);

    timeStart = vxbTimestamp();
    tickStart = tickGet();

    if (altSocGen5DwMshcBlkRead (card, blkStart, blkNum, pBuf) == ERROR)
        {
        printf ("Read error!\n");
        cacheDmaFree (pBuf);
        return ERROR;
        }

    timeEnd = vxbTimestamp();
    tickEnd = tickGet();
    tickDiff = tickEnd - tickStart;

    if (tickDiff > 1)
        speed = (double)blkNum / 2 / 
                       ((double)tickDiff / (double)sysClkRateGet());
    else
        {
        if (tickDiff == 0)
            timeDiff = timeEnd - timeStart;
        else
            timeDiff = timeEnd + (vxbTimestampPeriod ()  - timeStart);
        speed = (double) blkNum / 2 / 
                        ((double)timeDiff / (double)vxbTimestampFreq());
        }

    printf ("Read finished, read speed = %f KB/s.\n", speed);

    for (i = 0; i < blkNum * SDMMC_BLOCK_SIZE; i++)
        {
        if (pBuf[i] != i%256)
            {
            k = i / SDMMC_BLOCK_SIZE;
            printf ("r/w ERROR at %d  0x%08x %d \n", i, &pBuf[i], pBuf[i]);
            for (j = 0; j < SDMMC_BLOCK_SIZE; j++)
                {
                if (j%16 == 0)
                    printf ("\r\n");
                printf ("%x  ", pBuf[k * SDMMC_BLOCK_SIZE + j]);
                }

            printf ("\r\n");
            cacheDmaFree (pBuf);
            return ERROR;
            }
        }
    printf ("Test ok.\n");
    cacheDmaFree (pBuf);
    return OK;
    }
#endif /* ALT_DW_MSHC_DBG_ON */



#if 1

/*
2.open file and read contents from floopy disk:
-> readbuf=malloc(100)   
-> fd=open("/fd0/myfile",2)  or -> fp=fopen("/fd0/myfile","r")
-> read(fd,readbuf,100)   or -> fread(readbuf,1,50,fp)
-> printf readbuf   
-> close(fd)    or -> close(fp)
------------------------------------
-> fd=creat("/fd0/myfile",2)  or   -> fp=fopen("/fd0/myfile","w")
-> buf="what you want to write to file" 
-> write(fd,buf,strlen(buf)+1)  or -> fprintf(fp,buf)/ fwrite(buf, size, 1, fd)
-> close(fd)    or  -> fclose(fp)
------------------------------------
-> devs
drv name                
  0 /null               
  1 /tyCo/0             
  8 host:               
  9 /vio                
  3 /sd0:0              
value = 25 = 0x19
->  
*/

void test_rd_sd(char* filename)
{
	UINT8 buf[2048] = {0};
	char path_name[64] = {0};
	int fd = 0;
	int size = 1024, i = 0;

	sprintf((char*)(&path_name[0]), "/sd0:0/%s", filename);
	
	fd = fopen((char*)(&path_name[0]), "r");

	if (fd > 0)
	{
		fread(buf, 1, size, fd);
	}

	printf("read sd file: %s \n", (char*)(&path_name[0]));
	for (i=0; i<64; i++)
	{
		printf("%02X", buf[i]);
		
		if ((i+1)%8 == 0)
		{
			printf(" ");
		}
		
		if ((i+1)%32 == 0)
		{
			printf("\n");
		}
	}

	printf("\n---------\n\n");
	
	for (i=size-64; i<size; i++)
	{
		printf("%02X", buf[i]);
		
		if ((i+1)%8 == 0)
		{
			printf(" ");
		}
		
		if ((i+1)%32 == 0)
		{
			printf("\n");
		}
	}
	
	printf("\n-----end----\n\n");

	fclose(fd);

	return;
}


void test_wr_sd(char* filename)
{

	UINT8 buf[2048] = {0};
	char path_name[64] = {0};
	int fd = 0;
	int size = 1024, i = 0;
	int ret = 0;

	sprintf((char*)(&path_name[0]), "/sd0:0/%s", filename);
	
	fd = fopen((char*)(&path_name[0]), "w");

	if (fd > 0)
	{
		printf("create file:%s ok! \n", (char*)(&path_name[0]));
	}
	else
	{
		printf("create file:%d fail(%d)! \n", (char*)(&path_name[0]), fd);
		return;
	}

	for (i=0; i<size; i++)
	{
		buf[i] = i;
	}
	
	ret = fwrite(buf, size, 1, fd);
	
	fclose(fd);

	printf("write file:%s end! \n", (char*)(&path_name[0]));
	
	return;

}

#endif


