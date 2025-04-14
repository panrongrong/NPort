/* vxbMmcStorage.c - MMC driver file for vxBus */

/*
 * Copyright (c) 2012-2013, 2015-2018 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01k,12jul18,j_x  masking member busWidth of structure SD_HOST_SPEC for binary 
                 compatible (VXW6-86766)
01j,31may18,d_l  limit tranSpeed to 52MHZ (VXW6-86759)
01i,10feb17,d_l  set busWidth as BSP selected, default to 4 bit. (VXW6-86099)
01h,25jan16,myt  add support for direct bio for SD/eMMC (VXW6-85000)
01g,24dec15,myt  get cid info (VXW6-85012)
01f,26oct15,myt  add support of reliable write for eMMC (VXW6-84383)
01e,06jan15,myt  add support for Bay Trail (VXW6-80833)
01d,11dec13,m_y  modify to support request transfer routine
01c,20nov13,e_d  remove APIs wrongly published. (WIND00444661)
01b,24sep13,e_d  add some advanced functionalities for SD/MMC. (WIND00437987)
01a,12apr12,e_d  written based on the vxbFslI2c.c.
*/

/*
DESCRIPTION
This module implements a driver for SD card. This driver checks the device name 
to probe driver. If success, the driver will identify this device and mounts the
device to xbd layer.

\ie
SEE ALSO: vxBus
*/

/* includes */

#include <vxWorks.h>
#include <vsbConfig.h>
#include <stdlib.h>
#include <string.h>
#include <semLib.h>
#include <taskLib.h>
#include <stdio.h>
#include <vxBusLib.h>
#include <sioLib.h>
#include <ioLib.h>
#include <math.h>
#include <cacheLib.h>
#include <vxbTimerLib.h>
#include <errnoLib.h>
#include <hwif/util/hwMemLib.h>
#include <hwif/vxbus/vxBus.h>
#include <hwif/util/vxbParamSys.h>
#include <hwif/vxbus/vxbPlbLib.h>
#include <hwif/vxbus/hwConf.h>
#include <drv/xbd/xbd.h>
#include <xbdBlkDev.h>
#include <drv/erf/erfLib.h>
#include <hwif/vxbus/vxbSdLib.h>
#include <../src/hwif/h/vxbus/vxbAccess.h>

/*#include <../src/hwif/h/storage/vxbMmcStorage.h>*/
#include "vxbMmcStorage.h"   /* jc*/

/* defines */
#undef MMC_CARD_DBG_ON

#ifdef  MMC_CARD_DBG_ON

#ifdef  LOCAL
#undef  LOCAL
#define LOCAL
#endif

#define MMC_CARD_DBG_INIT           0x00000001
#define MMC_CARD_DBG_RW             0x00000002
#define MMC_CARD_DBG_SET            0x00000004
#define MMC_CARD_DBG_ALL            0xffffffff
#define MMC_CARD_DBG_OFF            0x00000000

LOCAL UINT32 mmcCardDbgMask = MMC_CARD_DBG_RW;

IMPORT FUNCPTR _func_logMsg;

/*
#define MMC_CARD_DBG(mask, string, a, b, c, d, e, f)            \
    if ((mmcCardDbgMask & mask) || (mask == MMC_CARD_DBG_ALL))  \
        if (_func_logMsg != NULL)                               \
           (* _func_logMsg)(string, a, b, c, d, e, f)
*/           
#define MMC_CARD_DBG(mask, string, a, b, c, d, e, f)   printf(string, a, b, c, d, e, f)

#else
#   define MMC_CARD_DBG(mask, string, a, b, c, d, e, f)
#endif  /* MMC_DBG_ON */

LOCAL void mmcStorageInit (VXB_DEVICE_ID);
LOCAL void mmcStorageInit2 (VXB_DEVICE_ID);
LOCAL void mmcStorageConnect (VXB_DEVICE_ID);

LOCAL STATUS mmcStorageBlkRead (void *, XBD_DATA *);
LOCAL STATUS mmcStorageBlkWrite (void *, XBD_DATA *);
LOCAL STATUS mmcStorageBlkRelWrite (void *, XBD_DATA * );
LOCAL STATUS mmcIdentify (VXB_DEVICE_ID);
LOCAL STATUS mmcStatusWaitReadyForData(VXB_DEVICE_ID);
LOCAL STATUS mmcStorageInstUnlink (VXB_DEVICE_ID, void * );
LOCAL BOOL mmcStoragDevProbe (VXB_DEVICE_ID);
LOCAL int mmcStorageIoctl (void *, int, void *);
LOCAL STATUS mmcExtCsdDecode (VXB_DEVICE_ID);
LOCAL STATUS mmcCacheEn (VXB_DEVICE_ID, UINT8);
LOCAL STATUS mmcCacheFlush (VXB_DEVICE_ID);
LOCAL STATUS mmcExtCsdSetPowClass (VXB_DEVICE_ID);
LOCAL STATUS mmcErase (VXB_DEVICE_ID, SECTOR_RANGE *);
LOCAL STATUS mmcSecErase (VXB_DEVICE_ID, SECTOR_RANGE *);
LOCAL STATUS mmcCardIdxAlloc (MMC_CARD_CTRL *);
LOCAL void mmcCardIdxFree (MMC_CARD_CTRL *);

/* locals */

LOCAL device_method_t vxbMmcStorage_methods[] =
    {
    DEVMETHOD (vxbDrvUnlink, mmcStorageInstUnlink),
    DEVMETHOD_END
    };

/* locals */

LOCAL DRIVER_INITIALIZATION mmcStoargeFuncs =
    {
    mmcStorageInit,        /* devInstanceInit */
    mmcStorageInit2,       /* devInstanceInit2 */
    mmcStorageConnect      /* devConnect */
    };

LOCAL DRIVER_REGISTRATION mmcStorageRegistration =
    {
    NULL,                   /* pNext */
    VXB_DEVID_DEVICE,       /* devID */
    VXB_BUSID_SD,           /* busID = PLB */
    VXB_VER_5_0_0,          /* vxbVersion */
    MMCCARD_NAME,           /* drvName */
    &mmcStoargeFuncs,       /* pDrvBusFuncs */
    vxbMmcStorage_methods,  /* pMethods */
    mmcStoragDevProbe,      /* devProbe */
    NULL                    /* pParamDefaults */
    };

/* SD/MMC card dev index */

LOCAL UINT32 mmcDevIdx;

/*
 * The following two tables defines the maximum data transfer rate
 * per one date line - TRAN_SPEED
 */

LOCAL UINT32 mmcCsdTranSpdfUnit[] = {
    10000, 100000, 1000000, 10000000
};

LOCAL UINT32 mmcCsdTranSpdTime[] = {
     0, 10, 12, 13, 15, 20, 26, 30,
    35, 40, 45, 52, 55, 60, 70, 80,
};
/*IMPORT UINT32 sysClkRateGet(void);*/

#ifndef _WRS_CONFIG_SMP
IMPORT volatile UINT64 vxAbsTicks;
#else
IMPORT UINT64 vxAbsTicksGet (void);
#endif /* _WRS_CONFIG_SMP */

/*******************************************************************************
*
* vxbMmcStorageRegister - register vxbSdStorage driver
*
* This is vxbSdStorage register routine.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void vxbMmcStorageRegister(void)
    {
    vxbDevRegister ((struct vxbDevRegInfo *)&mmcStorageRegistration);
    }

/*******************************************************************************
*
* mmcStoragDevProbe - vxbus probe function
*
* This function is called by vxBus to probe device.
*
* RETURNS: TRUE if probe passes and assumed a valid SD memory card
* (or compatible) device. FALSE otherwise.
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL BOOL mmcStoragDevProbe
    (
    VXB_DEVICE_ID pDev
    )
    {
    if (strcmp (pDev->pName, MMC_CARD_NAME) == 0x0)
        return TRUE;
    else
        return FALSE;
    }

/*******************************************************************************
*
* mmcStorageInit - VxBus instInit handler
*
* This function implements the VxBus instInit handler for MMC card
* device instance.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL void mmcStorageInit
    (
    VXB_DEVICE_ID pDev
    )
    {
    MMC_CARD_CTRL * pDrvCtrl;
    pDrvCtrl = (MMC_CARD_CTRL *) malloc (sizeof (MMC_CARD_CTRL));

    if (pDrvCtrl == NULL)
        return;

    bzero((char *)pDrvCtrl, sizeof(MMC_CARD_CTRL));

    /* save instance ID */

    pDrvCtrl->pInst = pDev;
    pDev->pDrvCtrl = pDrvCtrl;

    /* find the next instance */

    vxbNextUnitGet (pDev);

    return;
    }

/*******************************************************************************
*
* mmcStorageInit2 - mmcStorageInit initialization 2
*
* This routine initialize the mmcStorageInit phase 2
*
* RETURNS: N/A
*
* ERRNO
*
* \NOMANUAL
*/

LOCAL void mmcStorageInit2
    (
    VXB_DEVICE_ID pDev
    )
    {
    return;
    }

/*******************************************************************************
*
* mmcStorageConnect - third level initialization
*
* This routine performs the third level initialization of the mmcStorage
* driver. This routine will identify device parameter.
*
* RETURNS: N/A
*
* ERRNO : N/A
*
* \NOMANUAL
*/

LOCAL void mmcStorageConnect
    (
    VXB_DEVICE_ID pDev
    )
    {
    mmcIdentify(pDev);
    return;
    }

/*******************************************************************************
*
* mmcIdentify - identify MMC card in the system
*
* This routine implements the card identify process according to the MMC spec.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS mmcIdentify
    (
    VXB_DEVICE_ID pDev
    )
    {
    STATUS rc;
    UINT8 regValue = 0;
    UINT32 csize, csizeMulti;
    UINT64 blkNum;
    SD_HARDWARE * pSdHardWare;
    MMC_CARD_CTRL * pDrvCtrl;
    SD_HOST_SPEC * pHostSpec;
    MMC_INFO *pMmcInfo;
    device_t xbdSts;
    BLK_XBD_PARAMS blkXbdParams;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        return ERROR;

    pDrvCtrl = (MMC_CARD_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return (ERROR);

    pHostSpec = (SD_HOST_SPEC *)(pSdHardWare->pHostSpec);
    if (pHostSpec == NULL)
        return ERROR;

    pDrvCtrl->pInfo = malloc(sizeof(MMC_INFO));
    if (pDrvCtrl->pInfo == NULL)
        goto err;
    bzero ((char *)(pDrvCtrl->pInfo), sizeof(MMC_INFO));

    pMmcInfo = (MMC_INFO *)(pDrvCtrl->pInfo);
    bcopy((char *)(&pSdHardWare->cidValue[0]), (char *)(&pMmcInfo->cid), CID_SIZE);

    /* CMD3: set card's RCA */

    if ((pSdHardWare->ocrValue & OCR_MMC_SECTORMODE) != 0x0)
        pDrvCtrl->highCapacity = TRUE;
    else
        pDrvCtrl->highCapacity = FALSE;

    MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                "mmcIdentify() -  pDrvCtrl->highCapacity = 0x%x\n",
                pDrvCtrl->highCapacity, 0, 0, 0, 0, 0);

    pSdHardWare->rcaValue = MMC_RCA_BASE + pDrvCtrl->idx;
    rc = sdMmcCmdSetRelativeAddr (pDev);
    if (rc == ERROR)
        {
        MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                    "mmcIdentify() -  sdMmcCmdSetRelativeAddr fault\n",
                    0, 0, 0, 0, 0, 0);
        goto err;
        }

    /* CMD9: request card to send CSD */

    rc = sdCmdSendCsd (pDev, &(pMmcInfo->csd));
    if (rc == ERROR)
        {
        MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                    "mmcIdentify() -  sdCmdSendCsd fault\n",
                    0, 0, 0, 0, 0, 0);
        goto err;
        }
    else
        {
        pMmcInfo->csd.commClass = be16toh(pMmcInfo->csd.commClass);
        pMmcInfo->csd.eraseSize = be16toh(pMmcInfo->csd.eraseSize);
        pMmcInfo->csd.r2wFactor = be16toh(pMmcInfo->csd.r2wFactor);
        pMmcInfo->csd.fileFormat = be16toh(pMmcInfo->csd.fileFormat);
        MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                    "mmcIdentify() -  commClass = 0x%x\n"
                    "eraseSize = 0x%x\n"
                    "r2wFactor = 0x%x\n"
                    "fileFormat = 0x%x\n",
                    pMmcInfo->csd.commClass, pMmcInfo->csd.eraseSize,
                    pMmcInfo->csd.r2wFactor, pMmcInfo->csd.fileFormat, 0, 0);

        regValue = MMC_CSD_SPEC_VERS(pMmcInfo->csd.structure);
        MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                    "mmcIdentify() -  mmc csd spec version = %d\n",
                    regValue, 0, 0, 0, 0, 0);

        if (regValue >= MMC_CSD_SPEC_40)
            {
            pDrvCtrl->highSpeed = TRUE; 
            pDrvCtrl->isExendCsd = TRUE;
            }
        else
            {
            pDrvCtrl->highSpeed = FALSE;
            pDrvCtrl->isExendCsd = FALSE;
            }
        }

    /* CMD7: select one card and put it into transfer state */

    rc = sdCmdSelectCard (pDev);
    if (rc == ERROR)
        goto err;

    if (!pDrvCtrl->isExendCsd)
        {

        /* decode CSD fields */

        pDrvCtrl->tranSpeed = mmcCsdTranSpdfUnit[(pMmcInfo->csd.tranSpeed & 0x7)] *
                              mmcCsdTranSpdTime[((pMmcInfo->csd.tranSpeed >> 3) & 0x0f)];

        MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                    "mmcIdentify() -  pDrvCtrl->tranSpeed = %d\n",
                    pDrvCtrl->tranSpeed, 0, 0, 0, 0, 0);

        /* check for the device transmission speed does not exceed max controller speed */
        if (pHostSpec->maxTranSpeed != 0 && pDrvCtrl->tranSpeed > pHostSpec->maxTranSpeed)
            {
            pDrvCtrl->tranSpeed = pHostSpec->maxTranSpeed;
            }

        /* maximum read block length */

        pDrvCtrl->readBlkLen = 1 << (pMmcInfo->csd.commClass & 0x0f);
        MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                    "mmcIdentify() -  pDrvCtrl->readBlkLen = %d\n",
                    pDrvCtrl->readBlkLen, 0, 0, 0, 0, 0);

        /* per MMC spec, the maximum write block length is equal to read block */

        pDrvCtrl->writeBlkLen = pDrvCtrl->readBlkLen;
        pDrvCtrl->sectorSize = pDrvCtrl->readBlkLen;

        csize = ((UINT32)(pMmcInfo->csd.resvData0[0] & 0x03) << 10) |
                ((UINT32)(pMmcInfo->csd.resvData0[1]) << 2) |
                ((pMmcInfo->csd.resvData0[2] >> 6) & 0x03);
        csizeMulti = ((pMmcInfo->csd.resvData0[3] & 0x03) << 1) |
                     ((pMmcInfo->csd.eraseSize >> 7) & 0x01);
        blkNum = (csize + 1) << (csizeMulti + 2);
        pDrvCtrl->blkNum = blkNum;
        pDrvCtrl->capacity = blkNum * pDrvCtrl->sectorSize;
        MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                    "mmcIdentify() -  pDrvCtrl->blkNum = %d\n"
                    "pDrvCtrl->capacity = %d\n",
                    pDrvCtrl->blkNum, pDrvCtrl->capacity,
                    0, 0, 0, 0);
        }
        
    /* AMD51: request card to send its SCR */

    if (regValue >= MMC_CSD_SPEC_40)
        {
        rc = mmcCmdReadExtCsd (pDev, &(pMmcInfo->extCsd[0]));
        if (rc == ERROR)
            goto err;
        mmcExtCsdDecode (pDev);
        }
    
    /* CMD16: set block len */

    if (pDrvCtrl->highCapacity == FALSE)
        {
        rc = sdCmdSetBlockLen (pDev, pDrvCtrl->readBlkLen);
        if (rc == ERROR)
            return ERROR;
        }

    pSdHardWare->blockSize = SDMMC_BLOCK_SIZE;

    if (pDrvCtrl->highSpeed)
        {
        UINT8 extCsdWord196, extCsdWord192, extCsdWord194;
        extCsdWord196 = pMmcInfo->extCsd[196];
        extCsdWord192 = pMmcInfo->extCsd[192];
        extCsdWord194 = pMmcInfo->extCsd[194];
        MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                    "mmcIdentify() -  pDrvCtrl->blkNum = %d\n"
                    "pDrvCtrl->capacity = %d\n",
                    pDrvCtrl->blkNum, pDrvCtrl->capacity,
                    0, 0, 0, 0);

        if(mmcHrfsInclude == TRUE)
            {
            if (pDrvCtrl->extCsdInfo.relParam & 0x1)
                {
                if (pDrvCtrl->extCsdInfo.relSet != 0x1f)
                    {
                    rc = mmcSwitch(pDev, EXT_CSD_WR_REL_SET, MMC_EXT_CSD_REL_WRITE_ENABLE, 0x0);
                    if (rc == ERROR)
                        {
                        MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                                     "mmcIdentify() -  mmcSwitch enable HSTIME fault\n",
                                     0, 0, 0, 0, 0, 0);
                        return ERROR;
                        }
                    }
                }

            rc = mmcCmdReadExtCsd (pDev, &(pMmcInfo->extCsd[0]));
            if (rc == ERROR)
                goto err;
            MMC_CARD_DBG(MMC_CARD_DBG_INIT, "mmcIdentify() -  extCsdInfo.relSet = 0x%x\n",
                         pMmcInfo->extCsd[EXT_CSD_WR_REL_SET], 0, 0, 0, 0, 0);
            }

        rc = mmcSwitch(pDev, EXT_CSD_HS_TIMING, MMC_EXT_CSD_HSTIME_ENABLE, 0x0);
        if (rc == ERROR)
            {
            MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                         "mmcIdentify() -  mmcSwitch enable HSTIME fault\n",
                         0, 0, 0, 0, 0, 0);
            return ERROR;
            }

        rc = mmcCmdReadExtCsd (pDev, &(pMmcInfo->extCsd[0]));
        if (rc == ERROR)
            goto err;

        if (pHostSpec->vxbSdClkFreqSetup != NULL)
            {
            pHostSpec->vxbSdClkFreqSetup(pSdHardWare->pHostDev, pDrvCtrl->tranSpeed);
            }  

#ifdef SD_BUSWIDTH
        switch (pHostSpec->busWidth)
            {
            case 8:

            rc = mmcSwitch(pDev, EXT_CSD_BUS_WIDTH, MMC_EXT_CSD_BUSWIDTH_8BIT, 0x0); 
            if (rc == ERROR)
                {
                MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                         "mmcIdentify() -  mmcSwitch bus width 8bit fault\n",
                         0, 0, 0, 0, 0, 0);
                }

            if (pHostSpec->vxbSdBusWidthSetup != NULL)
                {
                pHostSpec->vxbSdBusWidthSetup(pSdHardWare->pHostDev, SDMMC_BUS_WIDTH_8BIT);
                }

                break;
            case 1:

                rc = mmcSwitch(pDev, EXT_CSD_BUS_WIDTH,
                               MMC_EXT_CSD_BUSWIDTH_1BIT,
                               0x0); 
                if (rc == ERROR)
                    return ERROR;
                if (pHostSpec->vxbSdBusWidthSetup != NULL)
                    {
                    pHostSpec->vxbSdBusWidthSetup(pSdHardWare->pHostDev, SDMMC_BUS_WIDTH_1BIT);
                    }

                break;
            case 4:
            default:

                rc = mmcSwitch(pDev, EXT_CSD_BUS_WIDTH, MMC_EXT_CSD_BUSWIDTH_4BIT,0x0); 
                if (rc == ERROR)
                    {
                    MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                             "mmcIdentify() -  mmcSwitch bus width 4bit fault\n",
                             0, 0, 0, 0, 0, 0);
                    }
                if (pHostSpec->vxbSdBusWidthSetup != NULL)
                    {
                    pHostSpec->vxbSdBusWidthSetup(pSdHardWare->pHostDev, SDMMC_BUS_WIDTH_4BIT);
                    }

                break;
            } /* switch (pHostSpec->busWidth) */
#else
         rc = mmcSwitch(pDev, EXT_CSD_BUS_WIDTH, MMC_EXT_CSD_BUSWIDTH_8BIT, 0x0);
         printf("###8bit#####rc=%d##########\n",rc);
         if (rc == ERROR)
             {
             MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                          "mmcIdentify() -  mmcSwitch bus width 8bit fault\n",
                          0, 0, 0, 0, 0, 0);
             }
  
         if (pHostSpec->vxbSdBusWidthSetup != NULL)
             {
             pHostSpec->vxbSdBusWidthSetup(pSdHardWare->pHostDev, SDMMC_BUS_WIDTH_8BIT);
             }
  
         rc = mmcCmdReadExtCsd (pDev, &(pMmcInfo->extCsd[0]));
  
         if ((extCsdWord196 != pMmcInfo->extCsd[196]) ||
             (extCsdWord192 != pMmcInfo->extCsd[192]) ||
             (extCsdWord194 != pMmcInfo->extCsd[194]) || (rc != OK))
             {
             rc = mmcSwitch(pDev, EXT_CSD_BUS_WIDTH, MMC_EXT_CSD_BUSWIDTH_4BIT,0x0);
             printf("###4bit#####rc=%d##########\n",rc);
             if (rc == ERROR)
                 {
                 MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                              "mmcIdentify() -  mmcSwitch bus width 4bit fault\n",
                              0, 0, 0, 0, 0, 0);
                 }
             if (pHostSpec->vxbSdBusWidthSetup != NULL)
                 {
                 pHostSpec->vxbSdBusWidthSetup(pSdHardWare->pHostDev, SDMMC_BUS_WIDTH_4BIT);
                 }
  
             rc = mmcCmdReadExtCsd (pDev, &(pMmcInfo->extCsd[0]));
  
             if ((extCsdWord196 != pMmcInfo->extCsd[196]) ||
                 (extCsdWord192 != pMmcInfo->extCsd[192]) ||
                 (extCsdWord194 != pMmcInfo->extCsd[194]) || (rc != OK))
                 {
                 rc = mmcSwitch(pDev, EXT_CSD_BUS_WIDTH,
                                MMC_EXT_CSD_BUSWIDTH_1BIT,
                                0x0);
                 printf("###1bit#####rc=%d##########\n",rc);
                 if (rc == ERROR)
                     return ERROR;
                 if (pHostSpec->vxbSdBusWidthSetup != NULL)
                     {
                     pHostSpec->vxbSdBusWidthSetup(pSdHardWare->pHostDev, SDMMC_BUS_WIDTH_1BIT);
                     }
#endif
                rc = mmcCmdReadExtCsd (pDev, &(pMmcInfo->extCsd[0]));

                if (rc == ERROR)
                    {
                    MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                                 "mmcIdentify() -  mmcSwitch bus width 1bit fault\n",
                                 0, 0, 0, 0, 0, 0);
                    }
                if ((extCsdWord196 != pMmcInfo->extCsd[196]) ||
                    (extCsdWord192 != pMmcInfo->extCsd[192]) ||
                    (extCsdWord194 != pMmcInfo->extCsd[194]))
                    return (ERROR);
 #ifndef SD_BUSWIDTH
                }
        }
 #endif
    }
    else
        {
        if (pHostSpec->vxbSdClkFreqSetup != NULL)
            {
            pHostSpec->vxbSdClkFreqSetup(pSdHardWare->pHostDev, pDrvCtrl->tranSpeed);
            }
        }

    /* check if card is write protected */

    if (pHostSpec->vxbSdCardWpCheck != NULL)
        {
        pDrvCtrl->isWp = pHostSpec->vxbSdCardWpCheck(pSdHardWare->pHostDev);
        }

    rc = mmcCardIdxAlloc (pDrvCtrl);
    if (rc != OK)
        {
        MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                    "sdIdentify() -  mmcCardIdxAlloc fault\n",
                    0, 0, 0, 0, 0, 0);
        goto err;
        }

    pDrvCtrl->attached = TRUE;

    /* set xbd params */
 
    blkXbdParams.xbdOps.blkRead = mmcStorageBlkRead;
    if (mmcHrfsInclude == TRUE)
        blkXbdParams.xbdOps.blkWrite = mmcStorageBlkRelWrite;
    else
        blkXbdParams.xbdOps.blkWrite = mmcStorageBlkWrite;
    blkXbdParams.xbdOps.blkDump = NULL;
    blkXbdParams.xbdOps.blkIoctl = mmcStorageIoctl;
    blkXbdParams.xbdOps.xferReq  = NULL;
    blkXbdParams.maxActiveReqs = 1;
    blkXbdParams.maxBiosPerReq = 1;
    blkXbdParams.maxXferBlks = MMC_MAX_RW_SECTORS;
    blkXbdParams.directModeFlag = pHostSpec->directBio;
    blkXbdParams.numBlks = (sector_t)(pDrvCtrl->capacity / pSdHardWare->blockSize);
    blkXbdParams.blkSize = pSdHardWare->blockSize;
    blkXbdParams.svcTskPri = SDMMC_XBD_SVC_TASK_PRI;
    blkXbdParams.pDev = (void *)pDev;
    snprintf ((char *)&(blkXbdParams.devName[0]), sizeof(blkXbdParams.devName),
              "/mmc%d", pDrvCtrl->idx);
    pDrvCtrl->attached = TRUE;
    xbdSts = blkXbdDevCreate (&(pDrvCtrl->xbdDev), &blkXbdParams);

    if (xbdSts == NULLDEV)
        {
        pDrvCtrl->attached = FALSE;
        return ERROR;
        }

    return OK;
err:
    if (pDrvCtrl->pInfo != NULL)
        free (pDrvCtrl->pInfo);
    return ERROR;
    }

/*******************************************************************************
*
* mmcExtCsdDecode - identify SD/MMC card in the system
*
* This routine implements the card identify process according to the SD spec.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS mmcExtCsdDecode
    (
    VXB_DEVICE_ID pDev
    )
    {
    MMC_CARD_CTRL * pDrvCtrl;
    MMC_INFO * pMmcInfo;    
    SD_HOST_SPEC * pHostSpec;
    SD_HARDWARE * pSdHardWare;
    UINT32 csize, csizeMulti;
    UINT64 blkNum;

    pDrvCtrl = (MMC_CARD_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return (ERROR);

    pMmcInfo = (MMC_INFO *)(pDrvCtrl->pInfo);
    if (pMmcInfo == NULL)
        return (ERROR);

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        return ERROR;

    pHostSpec = (SD_HOST_SPEC *)(pSdHardWare->pHostSpec);
    if (pHostSpec == NULL)
        return ERROR;

    MMC_CARD_DBG(MMC_CARD_DBG_INIT, "mmcExtCsdDecode() -  extCsdInfo = 0x%x\n",
                 pMmcInfo->extCsd, 0, 0, 0, 0, 0);

    /* WR_REL_PARAM */
    pDrvCtrl->extCsdInfo.relParam = pMmcInfo->extCsd[EXT_CSD_WR_REL_PARAM];
    MMC_CARD_DBG(MMC_CARD_DBG_INIT, "mmcExtCsdDecode() -  extCsdInfo.relParam = 0x%x\n",
                 pDrvCtrl->extCsdInfo.relParam, 0, 0, 0, 0, 0);

    /* EXT_CSD_REL_WR_SEC_C */
    pDrvCtrl->extCsdInfo.relSectors = pMmcInfo->extCsd[EXT_CSD_REL_WR_SEC_C];
    MMC_CARD_DBG(MMC_CARD_DBG_INIT, "mmcExtCsdDecode() -  extCsdInfo.relSectors = %d\n",
                 pDrvCtrl->extCsdInfo.relSectors, 0, 0, 0, 0, 0);

    /* EXT_CSD_WR_REL_SET */
    pDrvCtrl->extCsdInfo.relSet = pMmcInfo->extCsd[EXT_CSD_WR_REL_SET];
    MMC_CARD_DBG(MMC_CARD_DBG_INIT, "mmcExtCsdDecode() -  extCsdInfo.relSet = 0x%x\n",
                 pDrvCtrl->extCsdInfo.relSet, 0, 0, 0, 0, 0);

    pDrvCtrl->sectorSize = MMC_DEFAULT_SECTOR_SIZE;
    pDrvCtrl->extCsdInfo.rev = pMmcInfo->extCsd[EXT_CSD_REV];
    if (pDrvCtrl->extCsdInfo.rev > MMC_EXT_CSD_VERSION_2)
        {
        pDrvCtrl->extCsdInfo.sectors = pMmcInfo->extCsd[EXT_CSD_SEC_CNT + 0] << 0 |
                                       pMmcInfo->extCsd[EXT_CSD_SEC_CNT + 1] << 8 |
                                       pMmcInfo->extCsd[EXT_CSD_SEC_CNT + 2] << 16 |
                                       pMmcInfo->extCsd[EXT_CSD_SEC_CNT + 3] << 24;
        if (pDrvCtrl->extCsdInfo.sectors > (UINT32)(MMC_2G_SECTORS))
            pDrvCtrl->isBlockDevice = TRUE;
        MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                    "mmcExtCsdDecode() -  extCsdInfo.sectors = %d\n",
                    pDrvCtrl->extCsdInfo.sectors, 0, 0, 0, 0, 0);
        }
    else
        {

        /* maximum read block length */

        pDrvCtrl->readBlkLen = 1 << (pMmcInfo->csd.commClass & 0x0f);

        /* per SD spec, the maximum write block length is equal to read block */

        pDrvCtrl->writeBlkLen = pDrvCtrl->readBlkLen;
        pDrvCtrl->sectorSize = pDrvCtrl->readBlkLen;

        csize = ((UINT32)(pMmcInfo->csd.resvData0[0] & 0x03) << 10) |
                ((UINT32)(pMmcInfo->csd.resvData0[1]) << 2) |
                ((pMmcInfo->csd.resvData0[2] >> 6) & 0x03);
        csizeMulti = ((pMmcInfo->csd.resvData0[3] & 0x03) << 1) |
                     ((pMmcInfo->csd.eraseSize >> 15) & 0x01);
        blkNum = (csize + 1) << (csizeMulti + 2);
        pDrvCtrl->blkNum = blkNum;
        pDrvCtrl->capacity = blkNum * pDrvCtrl->sectorSize;

        if (pDrvCtrl->capacity < (2 * 1024 * 1024))
            pDrvCtrl->highCapacity = FALSE;
        MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                    "mmcExtCsdDecode() -  pDrvCtrl->blkNum = %d\n"
                    "pDrvCtrl->capacity = %d\n",
                    pDrvCtrl->blkNum, pDrvCtrl->capacity,
                    0, 0, 0, 0);
        }

    pDrvCtrl->extCsdInfo.deviceType = pMmcInfo->extCsd[EXT_CSD_DEVICE_TYPE] &
                                      EXT_CSD_DEVICE_TYPE_MASK;
    switch (pDrvCtrl->extCsdInfo.deviceType)
        {
        case EXT_CSD_DEVICE_TYPE_SDR_ALL:
        case EXT_CSD_DEVICE_TYPE_SDR_ALL_DDR18V:
        case EXT_CSD_DEVICE_TYPE_SDR_ALL_DDR_12V:
        case EXT_CSD_DEVICE_TYPE_SDR_ALL_DDR_52:
            pDrvCtrl->highSpeed = TRUE;
            pDrvCtrl->tranSpeed = MMC_CLK_FREQ_200MHZ;
            pDrvCtrl->voltage = (MMC_DEVICE_VOLTAGE_12V | MMC_DEVICE_VOLTAGE_18V);
            break;
        case EXT_CSD_DEVICE_TYPE_SDR12V_ALL:
        case EXT_CSD_DEVICE_TYPE_SDR12V_DDR18V:
        case EXT_CSD_DEVICE_TYPE_SDR12V_DDR12V:
        case EXT_CSD_DEVICE_TYPE_SDR12V_DDR_52:
            pDrvCtrl->highSpeed = TRUE;
            pDrvCtrl->tranSpeed = MMC_CLK_FREQ_200MHZ;
            pDrvCtrl->voltage = MMC_DEVICE_VOLTAGE_12V;
            break;
        case EXT_CSD_DEVICE_TYPE_SDR18V_ALL:
        case EXT_CSD_DEVICE_TYPE_SDR18V_DDR18V:
        case EXT_CSD_DEVICE_TYPE_SDR18V_DDR12V:
        case EXT_CSD_DEVICE_TYPE_SDR18V_DDR_52:
            pDrvCtrl->highSpeed = TRUE;
            pDrvCtrl->tranSpeed = MMC_CLK_FREQ_200MHZ;
            pDrvCtrl->voltage = MMC_DEVICE_VOLTAGE_18V;
            break;
        case EXT_CSD_DEVICE_TYPE_DDR_52 | EXT_CSD_DEVICE_TYPE_52 | 
             EXT_CSD_DEVICE_TYPE_26:
        case EXT_CSD_DEVICE_TYPE_52 | EXT_CSD_DEVICE_TYPE_26:
            pDrvCtrl->highSpeed = TRUE;
            pDrvCtrl->tranSpeed = MMC_CLK_FREQ_52MHZ;
            break;
        case EXT_CSD_DEVICE_TYPE_DDR_12V | EXT_CSD_DEVICE_TYPE_52 |
             EXT_CSD_DEVICE_TYPE_26:
            pDrvCtrl->highSpeed = TRUE;
            pDrvCtrl->tranSpeed = MMC_CLK_FREQ_52MHZ;
            pDrvCtrl->voltage = MMC_DEVICE_VOLTAGE_12V;
            break;
        case EXT_CSD_DEVICE_TYPE_DDR_18V | EXT_CSD_DEVICE_TYPE_52 |
             EXT_CSD_DEVICE_TYPE_26:
            pDrvCtrl->highSpeed = TRUE;
            pDrvCtrl->tranSpeed = MMC_CLK_FREQ_52MHZ;
            pDrvCtrl->voltage = MMC_DEVICE_VOLTAGE_18V;
            break;
        case EXT_CSD_DEVICE_TYPE_26:
            pDrvCtrl->tranSpeed = MMC_CLK_FREQ_26MHZ;
            break;
        default:
            pDrvCtrl->tranSpeed = MMC_CLK_FREQ_26MHZ;
            pDrvCtrl->highSpeed = FALSE;
            break;            
        }

    if (pDrvCtrl->tranSpeed > MMC_CLK_FREQ_52MHZ)
        {
        pDrvCtrl->tranSpeed = MMC_CLK_FREQ_52MHZ;
        }

    MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                "mmcExtCsdDecode() -  pDrvCtrl->tranSpeed = %d\n",
                pDrvCtrl->tranSpeed, 0, 0, 0, 0, 0);

    /* check for the device transmission speed does not exceed max controller speed */
    if (pHostSpec->maxTranSpeed != 0 && pDrvCtrl->tranSpeed > pHostSpec->maxTranSpeed)
        {
        pDrvCtrl->tranSpeed = pHostSpec->maxTranSpeed;
        }

    pDrvCtrl->extCsdInfo.eraseTimeoutMult = pMmcInfo->extCsd[EXT_CSD_ERASE_TIMEOUT_MULT];
    pDrvCtrl->extCsdInfo.hcEraseSize = pMmcInfo->extCsd[EXT_CSD_HC_ERASE_GRP_SIZE];

    if (pDrvCtrl->extCsdInfo.rev >= MMC_EXT_CSD_VERSION_3)
        {
        pDrvCtrl->extCsdInfo.partConfig = pMmcInfo->extCsd[EXT_CSD_PART_CONFIG];
        pDrvCtrl->extCsdInfo.partTime = MMC_EXT_CSD_TIMEUNIT *
                                        pMmcInfo->extCsd[EXT_CSD_PART_SWITCH_TIME];
        pDrvCtrl->extCsdInfo.saTimeout = 1 << pMmcInfo->extCsd[EXT_CSD_S_A_TIMEOUT];
        pDrvCtrl->extCsdInfo.eraseGroupDef = pMmcInfo->extCsd[EXT_CSD_ERASE_GROUP_DEF];
        pDrvCtrl->extCsdInfo.hcEraseTimeout = MMC_EXT_CSD_HCERASETIMEOUT_UNIT *
                                              pMmcInfo->extCsd[EXT_CSD_ERASE_TIMEOUT_MULT];
        pDrvCtrl->extCsdInfo.hcEraseSize = pMmcInfo->extCsd[EXT_CSD_HC_ERASE_GRP_SIZE] << 10;
        }

    if (pDrvCtrl->extCsdInfo.rev >= MMC_EXT_CSD_VERSION_4)
        {
        pDrvCtrl->extCsdInfo.secTrimMult = pMmcInfo->extCsd[EXT_CSD_SEC_TRIM_MULT];
        pDrvCtrl->extCsdInfo.secEraseMult = pMmcInfo->extCsd[EXT_CSD_SEC_ERASE_MULT];
        pDrvCtrl->extCsdInfo.fetureSupport = pMmcInfo->extCsd[EXT_CSD_SEC_FEATURE_SUPPORT];
        pDrvCtrl->extCsdInfo.trimTimeout = MMC_EXT_CSD_HCERASETIMEOUT_UNIT *
                                           pMmcInfo->extCsd[EXT_CSD_TRIM_MULT]; 
        pDrvCtrl->extCsdInfo.bootLock = pMmcInfo->extCsd[EXT_CSD_BOOT_WP];
        pDrvCtrl->extCsdInfo.bootLockEnable = TRUE;
        }

    if (pDrvCtrl->extCsdInfo.rev >= MMC_EXT_CSD_VERSION_5)
        {

        /* check the eMMC card supports HPI status */

        if ((pMmcInfo->extCsd[EXT_CSD_HPI_FEATURES] & MMC_EXT_HPI_SUPPORT) != 0x0)
            {
            pDrvCtrl->extCsdInfo.hpiSupport = TRUE;
            if ((pMmcInfo->extCsd[EXT_CSD_HPI_FEATURES]
                & MMC_EXT_HPI_IMPLEMENTATION) != 0x0)
                pDrvCtrl->extCsdInfo.hpiCmdSet = SDMMC_CMD_STOP_TRANSMISSION;
            else
                pDrvCtrl->extCsdInfo.hpiCmdSet = SDMMC_CMD_SEND_STATUS;

            pDrvCtrl->extCsdInfo.outIntTimeout = MMC_EXT_CSD_TIMEUNIT *
                                                 pMmcInfo->extCsd[EXT_CSD_OUT_OF_INTERRUPT_TIME];
            }
        }

    /* eMMC v4.5 or later */

    if (pDrvCtrl->extCsdInfo.rev >= MMC_EXT_CSD_VERSION_6)
        {
        pDrvCtrl->extCsdInfo.genericCmd6Time = MMC_EXT_CSD_TIMEUNIT *
                                               pMmcInfo->extCsd[EXT_CSD_GENERIC_CMD6_TIME];
        pDrvCtrl->extCsdInfo.powerOffLongtime = MMC_EXT_CSD_TIMEUNIT *
                                                pMmcInfo->extCsd[EXT_CSD_POWER_OFF_LONG_TIME];

        pDrvCtrl->extCsdInfo.cacheSize = pMmcInfo->extCsd[EXT_CSD_CACHE_SIZE + 0] << 0 |
                                         pMmcInfo->extCsd[EXT_CSD_CACHE_SIZE + 1] << 8 |
                                         pMmcInfo->extCsd[EXT_CSD_CACHE_SIZE + 2] << 16 |
                                         pMmcInfo->extCsd[EXT_CSD_CACHE_SIZE + 3] << 24;

        pDrvCtrl->extCsdInfo.cacheCtrl = pMmcInfo->extCsd[EXT_CSD_CACHE_CTRL];
        if (pMmcInfo->extCsd[EXT_CSD_DATA_SECTOR_SIZE] == 1)
            pDrvCtrl->sectorSize = 4096;
        else
            pDrvCtrl->sectorSize = 512;
        }

    MMC_CARD_DBG(MMC_CARD_DBG_INIT,
                 "mmcExtCsdDecode() -  pDrvCtrl->sectorSize = %d\n",
                 pDrvCtrl->sectorSize, 0, 0, 0, 0, 0);

    if (pDrvCtrl->extCsdInfo.rev > MMC_EXT_CSD_VERSION_2)
        pDrvCtrl->capacity = (UINT64)(pDrvCtrl->extCsdInfo.sectors) *
                             (UINT64)(pDrvCtrl->sectorSize);
 
    return (OK);
    }

/*******************************************************************************
*
* mmcExtCsdSetPowClass - set MMC device power class
*
* This routine sets MMC device power class.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS mmcExtCsdSetPowClass
    (
    VXB_DEVICE_ID pDev
    )
    {
    MMC_CARD_CTRL * pDrvCtrl;
    MMC_INFO * pMmcInfo;    
    SD_HARDWARE * pHardWare;
    UINT8 powClassIndex = 0;
    STATUS rc = OK;

    pDrvCtrl = (MMC_CARD_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return (ERROR);

    pMmcInfo = (MMC_INFO *)(pDrvCtrl->pInfo);
    if (pMmcInfo == NULL)
        return (ERROR);

    pHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pHardWare == NULL)
        return (ERROR);

    if ((pMmcInfo->csd.structure & 0x3C) < MMC_CSD_SPEC_VERS_4)
        return (ERROR);
    if ((pDrvCtrl->voltage & MMC_DEVICE_VOLTAGE_30V) != 0x0)
        {
        if (pDrvCtrl->tranSpeed <= MMC_CLK_FREQ_26MHZ)
            powClassIndex = pMmcInfo->extCsd[EXT_CSD_PWR_CL_26_360];
        else if (pDrvCtrl->tranSpeed <= MMC_CLK_FREQ_52MHZ)
            powClassIndex = pMmcInfo->extCsd[EXT_CSD_PWR_CL_52_360];            
        }
    else
        {
        if (pDrvCtrl->tranSpeed <= MMC_CLK_FREQ_26MHZ)
            powClassIndex = pMmcInfo->extCsd[EXT_CSD_PWR_CL_26_360];
        else if (pDrvCtrl->tranSpeed <= MMC_CLK_FREQ_52MHZ)
            powClassIndex = pMmcInfo->extCsd[EXT_CSD_PWR_CL_52_360];
        }
    if (powClassIndex != 0)
        {
        powClassIndex = (powClassIndex & MMC_EXT_CSD_PWR_8BIT_MASK) >>
                         MMC_EXT_CSD_PWR_8BIT_SHIFT;
        if (powClassIndex == 0x0)
            powClassIndex = (powClassIndex & MMC_EXT_CSD_PWR_4BIT_MASK) >>
                             MMC_EXT_CSD_PWR_4BIT_SHIFT;            
        }
    if (powClassIndex > 0)
        {
        rc = mmcSwitch(pDev, EXT_CSD_POWER_CLASS, powClassIndex, 0x0);
        if (rc == ERROR)
            return ERROR;
        }
    return (OK);
    }

/*******************************************************************************
*
* mmcCacheFlush - send command to flush MMC device's cache
*
* This routine sends command to flush MMC device's cache.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS mmcCacheFlush
    (
    VXB_DEVICE_ID pDev
    )
    {
    MMC_CARD_CTRL * pDrvCtrl;
    STATUS rc;

    if (pDev == NULL)
        return (ERROR);
    pDrvCtrl = (MMC_CARD_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return (ERROR);

    if ((pDrvCtrl->extCsdInfo.cacheCtrl & MMC_EXT_CSD_CACHE_EN) && 
        (pDrvCtrl->extCsdInfo.cacheSize > 0))
        {
        rc = mmcSwitch(pDev, EXT_CSD_FLUSH_CACHE, 1, 0x0);
        if (rc == ERROR)
            {
            MMC_CARD_DBG(MMC_CARD_DBG_SET,
                        "mmcCacheFlush() -  mmcSwitch set cache flush fault\n",
                        0, 0, 0, 0, 0, 0);
            return (ERROR);
            }
        }
    return OK;
    }

/*******************************************************************************
*
* mmcCacheEn - send command to enable/disable MMC device's cache
*
* This routine sends command to enable/disable MMC device's cache.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS mmcCacheEn
    (
    VXB_DEVICE_ID pDev,
    UINT8 bEnable
    )
    {
    MMC_CARD_CTRL * pDrvCtrl;
    STATUS rc = OK;

    if (pDev == NULL)
        return (ERROR);
    pDrvCtrl = (MMC_CARD_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return (ERROR);

    if (pDrvCtrl->extCsdInfo.cacheSize > 0)
        {
        rc = mmcSwitch(pDev, EXT_CSD_CACHE_CTRL, bEnable, 0x0);
        if (rc == ERROR)
            {
            MMC_CARD_DBG(MMC_CARD_DBG_SET,
                        "mmcCacheFlush() -  mmcSwitch set cache flush fault\n",
                        0, 0, 0, 0, 0, 0);
            return (ERROR);
            }
        }
    
    return (rc);
    }

/*******************************************************************************
*
* sdMmcStatusWaitReadyForData - check SD/MMC card ready for data status
*
* This routine checks SD/MMC card ready for data status.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS mmcStatusWaitReadyForData
    (
    VXB_DEVICE_ID pDev
    )
    {
    UINT32 rspValue = 0;
    STATUS rc;

    while (1)
        {
        rc = sdCmdSendStatus (pDev, &rspValue);
        if (rc == ERROR)
            return ERROR;

        if (rspValue & CARD_STS_READY_FOR_DATA)
            break;
        }
    return OK;
    }

/*******************************************************************************
*
* mmcErase - send erase/discard/trim command to mmc card
*
* This routine sends erase/discard/trim command to mmc card.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS mmcErase
    (
    VXB_DEVICE_ID pDev,
    SECTOR_RANGE * pRange
    )
    {
    MMC_CARD_CTRL * pDrvCtrl;
    STATUS rc;
    UINT32 eraseArg = 0;
    long long sectorNum;
    

    if (pRange == NULL)
        return (ERROR);

    pDrvCtrl = (MMC_CARD_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return (ERROR);

    if (pDrvCtrl->highCapacity)
        sectorNum = pRange->startSector;
    else
        sectorNum = (pRange->startSector) * pDrvCtrl->sectorSize;

    rc = mmcCmdSeEraseGrpStart(pDev, sectorNum);
    if (rc != OK)
        return (rc);
    
    if (pDrvCtrl->highCapacity)
        sectorNum = pRange->startSector + pRange->nSectors - 1;
    else
        sectorNum = (pRange->startSector + pRange->nSectors) * pDrvCtrl->sectorSize;

    rc = mmcCmdSeEraseGrpEnd (pDev, sectorNum);
    if (rc != OK)
        return (rc);

    if (pDrvCtrl->extCsdInfo.rev >= 6)
        eraseArg = MMC_ERASE_DISCARD;
    if ((pDrvCtrl->extCsdInfo.fetureSupport & MMC_EXT_CSD_FEA_SEC_GB_CL_EN) != 0x0)
        eraseArg = MMC_ERASE_TRIM;
    rc = mmcCmdSetEraseGrp(pDev, eraseArg);
    return (rc);
    }

/*******************************************************************************
*
* mmcSecErase - send secure erase/discard/trim command to mmc card
*
* This routine sends erase/discard/trim command to mmc card.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS mmcSecErase
    (
    VXB_DEVICE_ID pDev,
    SECTOR_RANGE * pRange
    )
    {
    MMC_CARD_CTRL * pDrvCtrl;
    STATUS rc;
    UINT32 eraseArg = 0;
    long long sectorNum;
    

    if (pRange == NULL)
        return (ERROR);

    pDrvCtrl = (MMC_CARD_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return (ERROR);

    if (pDrvCtrl->highCapacity)
        sectorNum = pRange->startSector;
    else
        sectorNum = (pRange->startSector) * pDrvCtrl->sectorSize;

    rc = mmcCmdSeEraseGrpStart(pDev, sectorNum);
    if (rc != OK)
        return (rc);
    
    if (pDrvCtrl->highCapacity)
        sectorNum = pRange->startSector + pRange->nSectors - 1;
    else
        sectorNum = (pRange->startSector + pRange->nSectors) * pDrvCtrl->sectorSize;

    rc = mmcCmdSeEraseGrpEnd (pDev, sectorNum);
    if (rc != OK)
        return (rc);

    if (pDrvCtrl->extCsdInfo.rev >= 6)
        eraseArg = MMC_SECURE_ERASE;
    if ((pDrvCtrl->extCsdInfo.fetureSupport & MMC_EXT_CSD_FEA_SEC_GB_CL_EN) != 0x0)
        eraseArg = MMC_SECURE_TRIMARG1;
    rc = mmcCmdSetEraseGrp(pDev, eraseArg);
    if (rc == ERROR)
        return (rc);
    else if (eraseArg == MMC_SECURE_TRIMARG1)
        {
        eraseArg = MMC_SECURE_TRIMARG2;
        rc = mmcCmdSetEraseGrp(pDev, eraseArg);
        }

    return (rc);
    }

/*******************************************************************************
*
* mmcHpiInterrput - send HPI command to device and check status
*
* This routine sends HPI command to device and check status.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS mmcHpiInterrput
    (
    VXB_DEVICE_ID pDev
    )
    {
    MMC_CARD_CTRL * pDrvCtrl;
    STATUS rc = OK;
    UINT32 rspValue = 0;

    pDrvCtrl = (MMC_CARD_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return (ERROR);

    if (!pDrvCtrl->extCsdInfo.hpiSupport)
        return (ERROR);

    rc = sdCmdSendStatus (pDev, &rspValue);
    if (rc == ERROR)
        {
        MMC_CARD_DBG (MMC_CARD_DBG_SET,
                      "mmcHpiInterrput - first get status fault\n",
                      0, 0, 0, 0 ,0 ,0);
        return (ERROR);
        }
    if (CARD_STS_CUR_GET(rspValue) == CARD_STS_PRG)
        {
        do
            {

            /*
             * We need check SD return status to know the HPI 
             * command is finished or not.
             */

            rc = mmcSendHpi (pDev, pDrvCtrl->extCsdInfo.hpiCmdSet, &rspValue);
            if (rc == ERROR)
                {
                MMC_CARD_DBG (MMC_CARD_DBG_SET,
                              "mmcHpiInterrput - mmcSendHpi Cmd fault\n",
                              0, 0, 0, 0 ,0 ,0);
                return (ERROR);
                }

            rc = sdCmdSendStatus (pDev, &rspValue);
            if (rc == ERROR)
                {
                MMC_CARD_DBG (MMC_CARD_DBG_SET,
                              "mmcHpiInterrput - sdCmdSendStatus fault\n",
                              0, 0, 0, 0 ,0 ,0);
                return (ERROR);
                }
            } while (CARD_STS_CUR_GET(rspValue) == CARD_STS_PRG);
        }
    else
        {
        MMC_CARD_DBG (MMC_CARD_DBG_SET,
                      "mmcHpiInterrput - the status is not in program \n",
                      0, 0, 0, 0 ,0 ,0);
        return (OK);
        }
    return (rc);
    }

/*******************************************************************************
*
* mmcStorageBlkRead - read block from MMC card
*
* This routine reads block from MMC card.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS mmcStorageBlkRead
    (
    void * pDrv,
    XBD_DATA * pXbdData
    )
    {
    SD_HARDWARE * pSdHardWare;
    STATUS rc;
    ULONG ix, round, remain;
    void *bufAddr;
    sector_t blkAddr;
    MMC_CARD_CTRL * pDrvCtrl;
    VXB_DEVICE_ID pDev;

    pDev = (VXB_DEVICE_ID)pDrv;
    if (pDev == NULL)
        return (ERROR);

    pDrvCtrl = (MMC_CARD_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return (ERROR);

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        return (ERROR);

    /* split the transfer */

    round = pXbdData->numBlks / MMC_MAX_RW_SECTORS;
    remain = pXbdData->numBlks % MMC_MAX_RW_SECTORS;

    MMC_CARD_DBG(MMC_CARD_DBG_RW,
                 "mmcStorageBlkRead() -  round = %d\n"
                 "remain = %d \n",
                 round, remain, 0, 0, 0, 0);

    for (ix = 0; ix < round; ix ++)
        {
        blkAddr = pXbdData->blkNum + ix * MMC_MAX_RW_SECTORS;
        bufAddr = (void *)((ULONG)pXbdData->buffer + ix * MMC_MAX_RW_SECTORS *
                           pSdHardWare->blockSize);
        MMC_CARD_DBG(MMC_CARD_DBG_RW,
                     "mmcStorageBlkRead() -  blkAddr = 0x%x\n"
                     "bufAddr = 0x%x \n",
                     blkAddr, bufAddr, 0, 0, 0, 0);

        rc = mmcCmdSetBlockNum(pDev, MMC_MAX_RW_SECTORS);
        if (rc == ERROR)
            return (rc);  
        rc = sdCmdReadBlock (pDev, bufAddr, blkAddr,
                             MMC_MAX_RW_SECTORS, pDrvCtrl->highCapacity);
        if (rc == ERROR)
            return ERROR;
        }

     if (remain != 0)
         {
         blkAddr = pXbdData->blkNum + round * MMC_MAX_RW_SECTORS;
         bufAddr = (void *)((ULONG)pXbdData->buffer +
                            round * MMC_MAX_RW_SECTORS *
                            pSdHardWare->blockSize);
         MMC_CARD_DBG(MMC_CARD_DBG_RW,
                      "mmcStorageBlkRead() -  blkAddr = 0x%x\n"
                      "bufAddr = 0x%x \n",
                      blkAddr, bufAddr, 0, 0, 0, 0);

        if (remain > 1)
            {
            rc = mmcCmdSetBlockNum(pDev, remain);
            if (rc == ERROR)
                return (rc);  
            }
         rc = sdCmdReadBlock (pDev, bufAddr, blkAddr,
                              remain, pDrvCtrl->highCapacity);
         if (rc == ERROR)
             return ERROR;
         }

    return OK;
    }

/*******************************************************************************
*
* sdMmcBlkWrite - write block to SD/MMC card
*
* This routine writes block to SD/MMC card.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS mmcStorageBlkWrite
    (
    void * pDrv,
    XBD_DATA * pXbdData
    )
    {
    SD_HARDWARE * pSdHardWare;
    STATUS rc;
    ULONG ix, round, remain;
    void *bufAddr;
    sector_t blkAddr;
    MMC_CARD_CTRL * pDrvCtrl;
    VXB_DEVICE_ID pDev;

    pDev = (VXB_DEVICE_ID)pDrv;
    if (pDev == NULL)
        return (ERROR);

    pDrvCtrl = (MMC_CARD_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return (ERROR);

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        return (ERROR);

    /* split the transfer */

    round = pXbdData->numBlks / MMC_MAX_RW_SECTORS;
    remain = pXbdData->numBlks % MMC_MAX_RW_SECTORS;

    MMC_CARD_DBG(MMC_CARD_DBG_RW,
                 "mmcStorageBlkWrite_1() -  round = %d\n"
                 "remain = %d \n",
                 round, remain, 0, 0, 0, 0);

    for (ix = 0; ix < round; ix ++)
        {
        bufAddr = (void *)((ULONG)pXbdData->buffer + ix * MMC_MAX_RW_SECTORS *
                           pSdHardWare->blockSize);
        blkAddr = pXbdData->blkNum + ix * MMC_MAX_RW_SECTORS;

        MMC_CARD_DBG(MMC_CARD_DBG_RW,
                     "mmcStorageBlkWrite() -  blkAddr = 0x%x\n"
                     "bufAddr = 0x%x \n",
                     blkAddr, bufAddr, 0, 0, 0, 0);
		
        rc = mmcCmdSetBlockNum(pDev, MMC_MAX_RW_SECTORS);
        if (rc == ERROR)
            return (rc);  
		
        rc = sdCmdWriteBlock (pDrvCtrl->pInst, bufAddr, blkAddr,
                              MMC_MAX_RW_SECTORS, pDrvCtrl->highCapacity);
        if (rc == ERROR )
            return ERROR;

        rc = mmcStatusWaitReadyForData(pDrvCtrl->pInst);
        if (rc == ERROR)
            return ERROR;
        }

    if (remain != 0)
        {
        bufAddr = (void *)((ULONG)pXbdData->buffer + round * MMC_MAX_RW_SECTORS *
                            pSdHardWare->blockSize);
        blkAddr = pXbdData->blkNum + round * MMC_MAX_RW_SECTORS;

        MMC_CARD_DBG(MMC_CARD_DBG_RW,
                     "mmcStorageBlkRead() -  blkAddr = 0x%x\n"
                     "bufAddr = 0x%x \n",
                     blkAddr, bufAddr, 0, 0, 0, 0);

        if (remain > 1)
            {
            rc = mmcCmdSetBlockNum(pDev, remain);
            if (rc == ERROR)
                return (rc);  
            }
        rc = sdCmdWriteBlock (pDrvCtrl->pInst, bufAddr, blkAddr,
                              remain, pDrvCtrl->highCapacity);
        if (rc == ERROR )
            return ERROR;

        rc = mmcStatusWaitReadyForData(pDrvCtrl->pInst);
        if (rc == ERROR)
            return ERROR;
        }

    return OK;
    }

/*******************************************************************************
*
* mmcStorageBlkRelWrite - Reliable write block to SD/MMC card
*
* This routine writes block to SD/MMC card.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS mmcStorageBlkRelWrite
    (
    void * pDrv,
    XBD_DATA * pXbdData
    )
    {
    SD_HARDWARE * pSdHardWare;
    STATUS rc;
    ULONG ix, round, remain;
    void *bufAddr;
    sector_t blkAddr;
    MMC_CARD_CTRL * pDrvCtrl;
    VXB_DEVICE_ID pDev;
    UINT32 arg;
    UINT8 enRelWr;

    pDev = (VXB_DEVICE_ID)pDrv;
    if (pDev == NULL)
        return (ERROR);

    pDrvCtrl = (MMC_CARD_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return (ERROR);

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);

    if (pSdHardWare == NULL)
        {
        MMC_CARD_DBG (MMC_CARD_DBG_RW,
                      "mmcStorageBlkRelWrite() - pSdHardWare is NULL\n",
                      0, 0, 0, 0, 0, 0);
        return ERROR;
        }
	
    enRelWr = (pDrvCtrl->extCsdInfo.relParam >> 2) & 0x1;
    MMC_CARD_DBG (MMC_CARD_DBG_RW,
                  "mmcStorageBlkRelWrite() - enRelWr:%d relParam:0x%x\n",
                  enRelWr, pDrvCtrl->extCsdInfo.relParam, 0, 0, 0, 0);
	
    /* legacy implementation for reliable write */	
    if (enRelWr == 0)
    	{
        /* set reliable write flag and block count */
        arg = (1 << 31 | 1);
		
        for (ix = 0; ix < pXbdData->numBlks; ix ++)
        	{
            bufAddr = (void *)((ULONG)pXbdData->buffer + ix *
                           pSdHardWare->blockSize);
            blkAddr = pXbdData->blkNum + ix;
			
            MMC_CARD_DBG(MMC_CARD_DBG_RW, "mmcStorageBlkRelWrite() -  blkAddr = %ld\n"
                         "bufAddr = 0x%x \n",
                         blkAddr, bufAddr, 0, 0, 0, 0);
			/* jc*/
            printf("mmcStorageBlkRelWrite() -  blkAddr = %ld\n"
                         "bufAddr = 0x%x \n",
                         blkAddr, bufAddr, 0, 0, 0, 0);
			
			rc = mmcCmdSetBlockNum(pDev, arg);
			if (rc == ERROR)
				return (rc);  
			rc = sdCmdWriteBlock (pDrvCtrl->pInst, bufAddr, (UINT32)blkAddr,
								  1, pDrvCtrl->highCapacity);
			if (rc == ERROR )
				return ERROR;
			
			rc = mmcStatusWaitReadyForData(pDrvCtrl->pInst);
			if (rc == ERROR)
				return ERROR;
            }
        }
    else  /* enhance implementation for reliable write */
        {
        /* split the transfer */
        round = pXbdData->numBlks / MMC_MAX_RW_SECTORS;
        remain = pXbdData->numBlks % MMC_MAX_RW_SECTORS;

		MMC_CARD_DBG(MMC_CARD_DBG_RW, "mmcStorageBlkRelWrite() -  round = %d\n"
				  "remain = %d \n",
				   round, remain, 0, 0, 0, 0);

		/**/
		printf("mmcStorageBlkRelWrite() -  round = %d\n"
				  "remain = %d \n",
				   round, remain, 0, 0, 0, 0);
		
		/* set reliable write flag and block count */
        arg = (1 << 31 | MMC_MAX_RW_SECTORS);

        for (ix = 0; ix < round; ix ++)
            {
            bufAddr = (void *)((ULONG)pXbdData->buffer + ix * MMC_MAX_RW_SECTORS *
                               pSdHardWare->blockSize);
            blkAddr = pXbdData->blkNum + ix * MMC_MAX_RW_SECTORS;

            MMC_CARD_DBG(MMC_CARD_DBG_RW, "mmcStorageBlkRelWrite() -  blkAddr = %ld\n"
                         "bufAddr = 0x%x \n",
                         blkAddr, bufAddr, 0, 0, 0, 0);    

            rc = mmcCmdSetBlockNum(pDev, arg);
            if (rc == ERROR)
                return (rc);  
            rc = sdCmdWriteBlock (pDrvCtrl->pInst, bufAddr, (UINT32)blkAddr,
                                  MMC_MAX_RW_SECTORS, pDrvCtrl->highCapacity);
            if (rc == ERROR )
                return ERROR;

            rc = mmcStatusWaitReadyForData(pDrvCtrl->pInst);
            if (rc == ERROR)
                return ERROR;			
            }

		if (remain != 0)
			{
			bufAddr = (void *)((ULONG)pXbdData->buffer + round * MMC_MAX_RW_SECTORS *
								pSdHardWare->blockSize);
			blkAddr = pXbdData->blkNum + round * MMC_MAX_RW_SECTORS;
		
			MMC_CARD_DBG(MMC_CARD_DBG_RW, "mmcStorageBlkRelWrite() -  blkAddr = %ld\n"
						 "bufAddr = 0x%x \n",
						 blkAddr, bufAddr, 0, 0, 0, 0);

            /* set reliable write flag and block count */
            arg = ((UINT32)(1 << 31) | remain);

		    if (remain > 1)
		        {
				rc = mmcCmdSetBlockNum(pDev, arg);
				if (rc == ERROR)
				    return (rc);  
                }
            rc = sdCmdWriteBlock (pDrvCtrl->pInst, bufAddr, (UINT32)blkAddr,
                                  (UINT32)remain, pDrvCtrl->highCapacity);
            if (rc == ERROR )
                return ERROR;
		
            rc = mmcStatusWaitReadyForData(pDrvCtrl->pInst);
            if (rc == ERROR)
                return ERROR;
			}
        }

    return OK;
    }

/*******************************************************************************
*
* sdStorageInstUnlink - unmount SD memory card from filesystem and XBD layer
*
* This routine unmount SD memory card from filesystem and XBD layer.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS mmcStorageInstUnlink
    (
    VXB_DEVICE_ID pDev,
    void * unused
    )
    {
    STATUS rc;
    MMC_CARD_CTRL * pDrvCtrl;
    pDrvCtrl = (MMC_CARD_CTRL *)pDev->pDrvCtrl;

    if (pDrvCtrl == NULL)
        return ERROR;

    pDrvCtrl->attached = FALSE;

    pDrvCtrl->xbdDev.xbdInserted = FALSE;
    rc = blkXbdDevDelete (&(pDrvCtrl->xbdDev));
    mmcCardIdxFree (pDrvCtrl);
    return (rc);
    }

/*******************************************************************************
*
* mmcCardIdxAlloc - allocate a global card index
*
* This routine allocates a global card index.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS mmcCardIdxAlloc
    (
    MMC_CARD_CTRL * pDrvCtrl
    )
    {
    UINT32 * cardIdx;
    int idx;

    cardIdx = &mmcDevIdx;

    for (idx = 0; idx < SDMMC_CARD_MAX_NUM; idx++)
        {
        if (!((*cardIdx) & (1 << idx)))
            {
            (*cardIdx) |= (1 << idx);
            break;
            }
        }

    if (idx == SDMMC_CARD_MAX_NUM)
        {
        return ERROR;
        }
    else
        {
        pDrvCtrl->idx = idx;
        return OK;
        }
    }

/*******************************************************************************
*
* mmcCardIdxFree - free a global card index
*
* This routine frees a global card index.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL void mmcCardIdxFree
    (
    MMC_CARD_CTRL * pDrvCtrl
    )
    {
    UINT32 * cardIdx;

    cardIdx = &mmcDevIdx;
    (*cardIdx) &= ~(1 << pDrvCtrl->idx);
    }

/*******************************************************************************
*
* mmcStorageIoctl - XBD block device ioctl routine
*
* This routine handles a yet to completed list of ioctl calls.
*
* RETURN: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL int mmcStorageIoctl
    (
    void *   pDrv,
    int      cmd,
    void *   arg
    )
    {
    int rc = OK;
    VXB_DEVICE_ID pDev;
    MMC_CARD_CTRL * pDrvCtrl;
    SECTOR_RANGE  * pRange;

    if (pDrv == NULL)
        return (ERROR);

    pDev = (VXB_DEVICE_ID)pDrv;
    pDrvCtrl = (MMC_CARD_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return (ERROR);

    switch (cmd)
        {
        case XBD_GETGEOMETRY:
            {
            XBD_GEOMETRY * geo;
            geo = (XBD_GEOMETRY *)arg;
            if (geo == NULL)
                return ERROR;

            geo->total_blocks = (sector_t)(pDrvCtrl->capacity / SDMMC_BLOCK_SIZE);
            geo->blocksize = SDMMC_BLOCK_SIZE;

            /* setup dummy CHS information */

            geo->heads = 1;
            geo->secs_per_track = 1;
            geo->cylinders = (pDrvCtrl->capacity / SDMMC_BLOCK_SIZE);
            rc = OK;
            break;
            }
        case XBD_TEST:
            {
            if (!pDrvCtrl->attached)
                rc = ERROR;
            else
                rc = OK;

            break;
            }

        case XBD_GETINFO:
            {
            XBD_INFO * pInfo;

            pInfo = (XBD_INFO *)arg;

            if (pDrvCtrl->isWp == TRUE)
                pInfo->properties |= XBD_INFO_READONLY;

            rc = OK;
            break;
            }
        case FIODISCARDGET:
            *(int *)arg = 0;
            break;
        case FIODISCARD:       /* file system layer is discarding sectors */
            pRange = (void *)arg;
            if (pRange == NULL)
                return (ERROR);
            rc = mmcErase (pDev, pRange);
            if (rc != OK)
                {
                errnoSet(S_ioLib_DEVICE_ERROR);
                }
            break;
        case XBD_SYNC:
            {
            rc = mmcCacheFlush(pDev);
            break;
            }
        default:
            {
            rc = ENOTSUP;
            break;
            }
        }

    return (rc);
    }

#if 1  /* jc*/

/* SD/MMC version bits; 8 flags, 8 major, 8 minor, 8 change */
#define SD_VERSION_SD	(1U << 31)
#define MMC_VERSION_MMC	(1U << 30)

#define MAKE_SDMMC_VERSION(a, b, c)	\
	((((u32)(a)) << 16) | ((u32)(b) << 8) | (u32)(c))
#define MAKE_SD_VERSION(a, b, c)	\
	(SD_VERSION_SD | MAKE_SDMMC_VERSION(a, b, c))
#define MAKE_MMC_VERSION(a, b, c)	\
	(MMC_VERSION_MMC | MAKE_SDMMC_VERSION(a, b, c))

#define EXTRACT_SDMMC_MAJOR_VERSION(x)	\
	(((u32)(x) >> 16) & 0xff)
#define EXTRACT_SDMMC_MINOR_VERSION(x)	\
	(((u32)(x) >> 8) & 0xff)
#define EXTRACT_SDMMC_CHANGE_VERSION(x)	\
	((u32)(x) & 0xff)

#define SD_VERSION_3		MAKE_SD_VERSION(3, 0, 0)
#define SD_VERSION_2		MAKE_SD_VERSION(2, 0, 0)
#define SD_VERSION_1_0		MAKE_SD_VERSION(1, 0, 0)
#define SD_VERSION_1_10		MAKE_SD_VERSION(1, 10, 0)

#define MMC_VERSION_UNKNOWN	MAKE_MMC_VERSION(0, 0, 0)
#define MMC_VERSION_1_2		MAKE_MMC_VERSION(1, 2, 0)
#define MMC_VERSION_1_4		MAKE_MMC_VERSION(1, 4, 0)
#define MMC_VERSION_2_2		MAKE_MMC_VERSION(2, 2, 0)
#define MMC_VERSION_3		MAKE_MMC_VERSION(3, 0, 0)
#define MMC_VERSION_4		MAKE_MMC_VERSION(4, 0, 0)
#define MMC_VERSION_4_1		MAKE_MMC_VERSION(4, 1, 0)
#define MMC_VERSION_4_2		MAKE_MMC_VERSION(4, 2, 0)
#define MMC_VERSION_4_3		MAKE_MMC_VERSION(4, 3, 0)
#define MMC_VERSION_4_4		MAKE_MMC_VERSION(4, 4, 0)
#define MMC_VERSION_4_41	MAKE_MMC_VERSION(4, 4, 1)
#define MMC_VERSION_4_5		MAKE_MMC_VERSION(4, 5, 0)
#define MMC_VERSION_5_0		MAKE_MMC_VERSION(5, 0, 0)
#define MMC_VERSION_5_1		MAKE_MMC_VERSION(5, 1, 0)

#define EXT_CSD_REV			    192	/* RO */
#define EXT_CSD_SEC_CNT			212	/* RO, 4 bytes */

void mmc_info(void)
{
    VXB_DEVICE_ID       pDev;	
    MMC_CARD_CTRL * pDrvCtrl;
    MMC_INFO * pMmcInfo;
	
    int i;
	int unit = 0;
	char strBuf[8] = {0};

	UINT64 capacity = 0;
	UINT32 ver;
	UINT32 csize, cmult;
	UINT32 * pBuf32;
	
    pDev = vxbInstByNameFind (MMCCARD_NAME, unit);
    if (pDev == NULL)
    {
        printf ("Can not find this sd/mmc unit(%d)!  \n", unit);
        return;
    }
	else
	{
		printf ("vxbInstByNameFind(%s): 0x%X \n", MMCCARD_NAME, pDev);
	}
	
    pDrvCtrl= (MMC_CARD_CTRL*)pDev->pDrvCtrl;	
	pMmcInfo = (MMC_INFO *)(pDrvCtrl->pInfo);
	
	printf("MMC_INFO->sdSpec:   0x%08X \n", pMmcInfo->sdSpec);    /* SD spec version */	
    printf("MMC_INFO->sdSec:    0x%08X \n", pMmcInfo->sdSec);     /* SD security verison */
    printf("MMC_INFO->busWidth: 0x%02X \n", pMmcInfo->busWidth);  /* bus width */	
    printf("MMC_INFO->cardSts:  0x%04X \n", pMmcInfo->cardSts);   /* card status */
	
	/* 
	CID register 
	*/
    printf("MMC_INFO->CID.mid:      0x%02X \n", pMmcInfo->cid.mid);       /* manufacturer ID */
    printf("MMC_INFO->CID.bga:      0x%02X \n", pMmcInfo->cid.bga);       /* CARD/BGA */
    printf("MMC_INFO->CID.oid:      0x%02X \n", pMmcInfo->cid.oid);       /* OEM/application ID */

	for (i=0; i<6; i++)
	{
		strBuf[i] = pMmcInfo->cid.pnm[i];
	}
    printf("MMC_INFO->CID.pnm[6]:   %s \n", (char*)(&strBuf[0]));        /* product revision */    
	
    printf("MMC_INFO->CID.prv:      0x%02X \n", pMmcInfo->cid.prv);      /* product revision */
	
    printf("MMC_INFO->CID.psn[4]:   0x", pMmcInfo->cid.psn);             /* product serial number */
	for (i=0; i<4; i++)
	{
		printf("%02X");
	}
	printf("\n");
	
    printf("MMC_INFO->CID.mdt:      0x%02X \n", pMmcInfo->cid.mdt);       /* manufacturing date */
    printf("MMC_INFO->CID.crc:      0x%02X \n", pMmcInfo->cid.crc);       /* CRC7 checksum */
	
	/* 
	CSD register
	*/
    printf("MMC_INFO->CSD.structure:   0x%02X \n", pMmcInfo->csd.structure);         /* csd struct */
    printf("MMC_INFO->CSD.taac:        0x%02X \n", pMmcInfo->csd.taac);              /* data read access-time */
    printf("MMC_INFO->CSD.nsac:        0x%02X \n", pMmcInfo->csd.nsac);              /* data read access-time in CLK */
    printf("MMC_INFO->CSD.tranSpeed:   0x%02X \n", pMmcInfo->csd.tranSpeed);         /* data transfer rate */
    printf("MMC_INFO->CSD.commClass:   0x%04X \n", pMmcInfo->csd.commClass);         /* command class */
                                                    /*UINT8       resvData0[4];      /* reserved data */
    printf("MMC_INFO->CSD.eraseSize:   0x%04X \n", pMmcInfo->csd.eraseSize);         /* erase size */
    printf("MMC_INFO->CSD.r2wFactor:   0x%04X \n", pMmcInfo->csd.r2wFactor);         /* write speed factor */
    printf("MMC_INFO->CSD.fileFormat:  0x%04X \n", pMmcInfo->csd.fileFormat);        /* file format */

	printf("MMC_INFO->extCsd[512]:      \n");
	for (i=0; i<512; i++)
	{
		printf("%02X", pMmcInfo->extCsd[i]);
		
		if (((i+1) % 4) == 0)
		{
			printf(" ");
		}
		
		if (((i+1) % 32) == 0)
		{
			printf(" \n");
		}		
	}
	
    printf("MMC_INFO->dsrValue: 0x%04X \n", pMmcInfo->dsrValue);      /* dsrValue;*/

	printf("------------------ \n");

	/**/
	/* version*/
	/**/
	static const u32 mmc_versions[] = 
	{
		MMC_VERSION_4,
		MMC_VERSION_4_1,
		MMC_VERSION_4_2,
		MMC_VERSION_4_3,
		MMC_VERSION_4_4,
		MMC_VERSION_4_41,
		MMC_VERSION_4_5,
		MMC_VERSION_5_0,
		MMC_VERSION_5_1
	};
	ver = mmc_versions[pMmcInfo->extCsd[192]];
	printf("MMC version: %d.%d \n", ((ver & 0xFF0000) >> 16), ((ver & 0xFF00) >> 8));

	/**/
	/* capacity*/
	/**/
 	capacity =  (pMmcInfo->extCsd[212] << 0) | 
 	            (pMmcInfo->extCsd[212 + 1] << 8) | 
 	            (pMmcInfo->extCsd[212 + 2] << 16) | 
 	            ((UINT64)pMmcInfo->extCsd[212 + 3] << 24);
	capacity = capacity * 512;
	printf("Capacity: %llu Bytes \n", capacity);  /*  57.9 GiB*/
	printf("Capacity: %.1f GiB \n", ((float)capacity/1024.0/1024.0/1024.0));  /*  57.9 GiB*/

	/*
	vxbInstByNameFind(mmcStorage): 0x43A7F8 
	MMC_INFO->sdSpec:   0x00000000 
	MMC_INFO->sdSec:    0x00000000 
	MMC_INFO->busWidth: 0x00 
	MMC_INFO->cardSts:  0x0000 
	MMC_INFO->CID.mid:   0xEA 
	MMC_INFO->CID.bga:   0x01 
	MMC_INFO->CID.oid:   0x0E 
	MMC_INFO->CID.pnm[6]:   SPeMMC 
	MMC_INFO->CID.prv:      0x10 
	MMC_INFO->CID.psn[4]:   0x1A020101
	MMC_INFO->CID.mdt:   0xC7 
	MMC_INFO->CID.crc:   0x4D 
	MMC_INFO->CSD.structure:   0xD0 
	MMC_INFO->CSD.taac:        0x4F 
	MMC_INFO->CSD.nsac:        0x01 
	MMC_INFO->CSD.tranSpeed:   0x32 
	MMC_INFO->CSD.commClass:   0x0F59 
	MMC_INFO->CSD.eraseSize:   0xFFEF 
	MMC_INFO->CSD.r2wFactor:   0x9640 
	MMC_INFO->CSD.fileFormat:  0x00D1 
	MMC_INFO->extCsd[512]:      
	00000000 00000000 00000000 00000000 0103207A CF010000 00000000 00000000  
	00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000  
	00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000  
	00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000  
	00000000 00000000 00000000 00000000 00000000 00000000 00000000 00A70900  
	07000000 0000141F 20000000 00000000 00000000 00000001 01010000 00000000  
	08000200 57010A06 00000000 00000000 00000000 80E83D07 0A17060D 0D100101  
	01062000 07111B55 02000000 00000000 010A0000 00000064 40000300 00003033  
	33363038 00000000 07404001 01010000 00000000 00000000 00000000 00000000  
	00000000 00000000 00000000 00000000 0000001F 01000000 00000000 00000000  
	00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000  
	00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000  
	00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000  
	00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000  
	00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000  
	00000000 00000100 0000000D 01010301 05010501 38380101 01000000 00000000  
	MMC_INFO->dsrValue: 0x0000 
	------------------ 
	MMC version: 5.1 
	Capacity: 62206836736 
	value = 23 = 0x17
	*/

	/*
	fmsh> mmc info
		
	Device: dwmmc@e0043000
	Manufacturer ID: ea
	OEM: 10e
	Name: SPeMM 
	Bus Speed: 52000000
	Mode : MMC High Speed (52MHz)
	Rd Block Len: 512
	MMC version 5.1
	High Capacity: Yes
	Capacity: 57.9 GiB
	Bus Width: 4-bit
	Erase Group Size: 512 KiB
	HC WP Group Size: 8 MiB
	User Capacity: 57.9 GiB WRREL
	Boot Capacity: 4 MiB ENH
	RPMB Capacity: 4 MiB ENH
	*/

	return;	
}
	
#endif



