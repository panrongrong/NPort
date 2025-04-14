/* vxbSdStorage.c - SD device driver file */

/*
 * Copyright (c) 2012, 2013, 2015-2017 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
-------------------- 
01i,22sep17,mw1  change work frequency to 25MHz after card identification 
                 (VXW6-85713)
01h,25jan16,myt  add support for direct bio for SD/eMMC (VXW6-85000)
01g,24dec15,myt  get cid info (VXW6-85012)
01f,18jun15,zly  ACMD42 is not included in SD card driver.(VXW6-84329) 
01e,04jun15,swu  fixed SD card highspeed mode detect method. (VXW6-84559)
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
#include <../src/hwif/h/storage/vxbSdStorage.h>

/* defines */
/*#define SD_CARD_DBG_ON*/

#ifdef  SD_CARD_DBG_ON

#ifdef  LOCAL
#undef  LOCAL
#define LOCAL
#endif

#define SD_CARD_DBG_INIT           0x00000001
#define SD_CARD_DBG_RW             0x00000002
#define SD_CARD_DBG_XBD            0x00000004
#define SD_CARD_DBG_ERR            0x00000008
#define SD_CARD_DBG_ALL            0xffffffff
#define SD_CARD_DBG_OFF            0x00000000

LOCAL UINT32 sdCardDbgMask = SD_CARD_DBG_ALL;

IMPORT FUNCPTR _func_logMsg;

/* 
#define SD_CARD_DBG(mask, string, a, b, c, d, e, f)           \
   if ((sdCardDbgMask & mask) || (mask == SD_CARD_DBG_ALL))  \
        if (_func_logMsg != NULL) \
           (* _func_logMsg)(string, a, b, c, d, e, f)
*/           
#define SD_CARD_DBG(mask, string, a, b, c, d, e, f)  printf(string, a, b, c, d, e, f)

#else
#   define SD_CARD_DBG(mask, string, a, b, c, d, e, f)
#endif  /* SD_CARD_DBG_ON */

LOCAL void sdStorageInit (VXB_DEVICE_ID);
LOCAL void sdStorageInit2 (VXB_DEVICE_ID);
LOCAL void sdStorageConnect (VXB_DEVICE_ID);
LOCAL void sdCardIdxFree (SD_CARD_CTRL *);
LOCAL BOOL sdStoragDevProbe (VXB_DEVICE_ID);
LOCAL STATUS sdStorageBlkRead (void *, XBD_DATA *);
LOCAL STATUS sdStorageBlkWrite (void *, XBD_DATA *);
LOCAL STATUS sdIdentify (VXB_DEVICE_ID);
LOCAL STATUS sdStorageInstUnlink (VXB_DEVICE_ID, void * );
LOCAL STATUS sdCardIdxAlloc (SD_CARD_CTRL *);
LOCAL STATUS sdStatusWaitReadyForData(VXB_DEVICE_ID);
LOCAL STATUS sdDecodeSsr (VXB_DEVICE_ID);
LOCAL STATUS sdErase (VXB_DEVICE_ID, SECTOR_RANGE *);
LOCAL int sdStorageIoctl (void *, int, void *);
LOCAL int sdACmdSetHighSpeed (VXB_DEVICE_ID);

/* locals */

LOCAL device_method_t vxbSdStorage_methods[] =
    {
    DEVMETHOD (vxbDrvUnlink, sdStorageInstUnlink),
    DEVMETHOD_END
    };

/* locals */

LOCAL DRIVER_INITIALIZATION sdStoargeFuncs =
    {
    sdStorageInit,        /* devInstanceInit */
    sdStorageInit2,       /* devInstanceInit2 */
    sdStorageConnect      /* devConnect */
    };

LOCAL DRIVER_REGISTRATION sdStorageRegistration =
{
    NULL,                                               /* pNext */
    VXB_DEVID_DEVICE,                                   /* devID */
    VXB_BUSID_SD,                                       /* busID = PLB */
    VXB_VER_5_0_0,                                      /* vxbVersion */
    SDCARD_NAME,                                        /* drvName */
    &sdStoargeFuncs,                                    /* pDrvBusFuncs */
    vxbSdStorage_methods,                               /* pMethods */
    sdStoragDevProbe,                                   /* devProbe */
    NULL                                                /* pParamDefaults */
};

/* SD/MMC card dev index */

LOCAL UINT32 sdDevIdx;

/*
 * The following two tables defines the maximum data transfer rate
 * per one date line - TRAN_SPEED
 */

LOCAL UINT32 sdCsdTranSpdfUnit[] = {
    10000, 100000, 1000000, 10000000
};

LOCAL UINT32 sdCsdTranSpdTime[] = {
     0, 10, 12, 13, 15, 20, 25, 30,
    35, 40, 45, 50, 55, 60, 70, 80,
};

/*IMPORT UINT32 sysClkRateGet(void);*/

#ifndef _WRS_CONFIG_SMP
IMPORT volatile UINT64 vxAbsTicks;
#else
IMPORT UINT64 vxAbsTicksGet (void);
#endif /* _WRS_CONFIG_SMP */

/*******************************************************************************
*
* vxbSdStorageRegister - register vxbSdStorage driver
*
* This is vxbSdStorage register routine.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void vxbSdStorageRegister(void)
    {
    vxbDevRegister ((struct vxbDevRegInfo *)&sdStorageRegistration);
    }

/*******************************************************************************
*
* sdStoragDevProbe - vxbus probe function
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

LOCAL BOOL sdStoragDevProbe
    (
    VXB_DEVICE_ID pDev
    )
    {
    if (strcmp (pDev->pName, SD_CARD_NAME) == 0x0)
        return TRUE;
    else
        return FALSE;
    }

/*******************************************************************************
*
* sdStorageInit - VxBus instInit handler
*
* This function implements the VxBus instInit handler for SD memory card
* device instance.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL void sdStorageInit
    (
    VXB_DEVICE_ID pDev
    )
    {
    SD_CARD_CTRL * pDrvCtrl;
    pDrvCtrl = (SD_CARD_CTRL *) malloc (sizeof (SD_CARD_CTRL));

    if (pDrvCtrl == NULL)
        {
        SD_CARD_DBG(SD_CARD_DBG_INIT,
                    "sdStorageInit() -  malloc SD_CARD_CTRL fault\n",
                    0, 0, 0, 0, 0, 0);
        return;
        }

    bzero((char *)pDrvCtrl, sizeof(SD_CARD_CTRL));

    /* save instance ID */

    pDrvCtrl->pInst = pDev;
    pDev->pDrvCtrl = pDrvCtrl;

    /* find the next instance */

    vxbNextUnitGet (pDev);

    return;
    }

/*******************************************************************************
*
* sdStorageInit2 - sdStorageInit initialization 2
*
* This routine initialize the sdStorageInit phase 2
*
* RETURNS: N/A
*
* ERRNO
*
* \NOMANUAL
*/

LOCAL void sdStorageInit2
    (
    VXB_DEVICE_ID pDev
    )
    {
    return;
    }

/*******************************************************************************
*
* sdStorageConnect - third level initialization
*
* This routine performs the third level initialization of the sdStorage
* driver. This routine will identify device parameter.
*
* RETURNS: N/A
*
* ERRNO : N/A
*
* \NOMANUAL
*/

LOCAL void sdStorageConnect
    (
    VXB_DEVICE_ID pDev
    )
    {
    sdIdentify(pDev);
    return;
    }

/*******************************************************************************
*
* sdIdentify - identify SD/MMC card in the system
*
* This routine implements the card identify process according to the SD spec.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS sdIdentify
    (
    VXB_DEVICE_ID pDev
    )
    {
    STATUS rc;
    UINT32 csize, csizeMulti;
    UINT64 blkNum;
    SD_CARD_CTRL * pDrvCtrl;
    SD_HARDWARE * pSdHardware;
    SD_HOST_SPEC * pHostSpec;
    SD_INFO * pSdInfo;
    device_t xbdSts;
    BLK_XBD_PARAMS blkXbdParams;

    pDrvCtrl = (SD_CARD_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return ERROR;

    pSdHardware = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardware == NULL)
        return ERROR;

    pHostSpec = (SD_HOST_SPEC *)(pSdHardware->pHostSpec);
    if (pHostSpec == NULL)
        return ERROR;

    pDrvCtrl->pInfo = malloc(sizeof(SD_INFO));
    if (pDrvCtrl->pInfo == NULL)
        goto err;
    bzero ((char *)(pDrvCtrl->pInfo), sizeof(SD_INFO));

    /* apply one card index to system */

    rc = sdCardIdxAlloc (pDrvCtrl);
    if (rc != OK)
        {
        SD_CARD_DBG(SD_CARD_DBG_INIT,
                    "sdIdentify() -  sdCardIdxAlloc fault\n",
                    0, 0, 0, 0, 0, 0);
        goto err;
        }

    pSdInfo = (SD_INFO *)(pDrvCtrl->pInfo);
    bcopy((char *)(&pSdHardware->cidValue[0]), (char *)(&pSdInfo->cid), CID_SIZE);
    pDrvCtrl->highCapacity = (((pSdHardware->ocrValue) &
                               OCR_CARD_CAP_STS) != 0x0) ? TRUE : FALSE;
    SD_CARD_DBG(SD_CARD_DBG_INIT,
                "sdIdentify() -  pDrvCtrl->highCapacity = 0x%x\n",
                pDrvCtrl->highCapacity, 0, 0, 0, 0, 0);

    /* CMD3: request card to send RCA */

    do
        {
        sdCmdSendRelativeAddr (pDev);
        } while (pSdHardware->rcaValue == 0);

    SD_CARD_DBG(SD_CARD_DBG_INIT,
                "sdIdentify() -  pSdHardware->rcaValue = 0x%x\n",
                pSdHardware->rcaValue, 0, 0, 0, 0, 0);

    /* CMD9: request card to send CSD */

    rc = sdCmdSendCsd (pDev, &(pSdInfo->csd));
    if (rc == ERROR)
        goto err;
    else
        {
        pSdInfo->csd.commClass = be16toh(pSdInfo->csd.commClass);
        pSdInfo->csd.eraseSize = be16toh(pSdInfo->csd.eraseSize);
        pSdInfo->csd.r2wFactor = be16toh(pSdInfo->csd.r2wFactor);
        pSdInfo->csd.fileFormat = be16toh(pSdInfo->csd.fileFormat);
        SD_CARD_DBG(SD_CARD_DBG_INIT,
                    "sdIdentify() -  pSdInfo->csd.commClass = 0x%x\n"
                    "pSdInfo->csd.eraseSize = 0x%x\n"
                    "pSdInfo->csd.r2wFactor = 0x%x\n"
                    "pSdInfo->csd.fileFormat = 0x%x\n",
                    pSdInfo->csd.commClass, pSdInfo->csd.eraseSize,
                    pSdInfo->csd.r2wFactor, pSdInfo->csd.fileFormat, 0, 0);

        /* decode CSD fields */

        pDrvCtrl->tranSpeed = sdCsdTranSpdfUnit[(pSdInfo->csd.tranSpeed & 0x7)] *
                              sdCsdTranSpdTime[((pSdInfo->csd.tranSpeed >> 3) & 0x0f)];
        SD_CARD_DBG(SD_CARD_DBG_INIT,
                    "sdIdentify() -  pDrvCtrl->tranSpeed = %d\n",
                    pDrvCtrl->tranSpeed, 0, 0, 0, 0, 0);

        /* always assume highSpeed is supported unless CMD6 return ERROR */

        pDrvCtrl->highSpeed = TRUE;

        /* maximum read block length */

        pDrvCtrl->readBlkLen = 1 << (pSdInfo->csd.commClass & 0x0f);
        SD_CARD_DBG(SD_CARD_DBG_INIT,
                    "sdIdentify() -  pDrvCtrl->readBlkLen = %d\n",
                    pDrvCtrl->readBlkLen, 0, 0, 0, 0, 0);

        /* per SD spec, the maximum write block length is equal to read block */

        pDrvCtrl->writeBlkLen = pDrvCtrl->readBlkLen;
        pSdHardware->blockSize = SDMMC_BLOCK_SIZE;

        /* calculate user data capacity */

        if (pDrvCtrl->highCapacity)
            {
            csize = ((pSdInfo->csd.resvData0[1] & 0x3f) << 16) |
                     (pSdInfo->csd.resvData0[2] << 8) |
                      pSdInfo->csd.resvData0[3];
            csizeMulti = 8;
            }
        else
            {
            csize = ((UINT32)(pSdInfo->csd.resvData0[0] & 0x03) << 10) |
                    ((UINT32)(pSdInfo->csd.resvData0[1]) << 2) |
                    ((pSdInfo->csd.resvData0[2] >> 6) & 0x03);
            csizeMulti = ((pSdInfo->csd.resvData0[3] & 0x03) << 1) |
                         ((pSdInfo->csd.eraseSize >> 7) & 0x01);
            }

        blkNum = (csize + 1) << (csizeMulti + 2);
        pDrvCtrl->blkNum = blkNum;
        pDrvCtrl->capacity = blkNum * pDrvCtrl->readBlkLen;
        SD_CARD_DBG(SD_CARD_DBG_INIT,
                    "sdIdentify() -  pDrvCtrl->blkNum = %d\n"
                    "pDrvCtrl->capacity = %lld\n",
                    pDrvCtrl->blkNum, pDrvCtrl->capacity,
                    0, 0, 0, 0);
        }

    /* After Card Identification, the card goes into Data Transfer Mode */

    if (pDrvCtrl->tranSpeed > SDMMC_CLK_FREQ_400KHZ)
        {
        UINT32 workSpeed;
        workSpeed = pDrvCtrl->tranSpeed < SDMMC_CLK_FREQ_25MHZ ?
                    pDrvCtrl->tranSpeed : SDMMC_CLK_FREQ_25MHZ;
        

         pHostSpec->vxbSdClkFreqSetup (pSdHardware->pHostDev, workSpeed);
        }

    /* CMD7: select one card and put it into transfer state */

    rc = sdCmdSelectCard (pDev);
    if (rc == ERROR)
        goto err;

    /* AMD51: request card to send its SCR */

    rc = sdACmdSendScr (pDev, &(pSdInfo->scr));
    if (rc == ERROR)
        goto err;
    else
        {
        pSdInfo->sdSpec = pSdInfo->scr.spec & 0x0f;
        SD_CARD_DBG(SD_CARD_DBG_INIT,
                    "sdIdentify() -  pSdInfo->sdSpec = 0x%x\n",
                    pSdInfo->sdSpec, 0, 0, 0, 0, 0);

        /* update the SD card version */

        pSdHardware->version = pSdInfo->sdSpec + 1;

        if ((pSdInfo->scr.expConfig & SCR_SD_SPEC3_MASK) != 0x0)
            {
            if (pSdHardware->version == SD_VERSION_200)
                pSdHardware->version = SD_VERSION_300;
            }
            
        pSdInfo->sdSec = (pSdInfo->scr.config >> 4) & 0x7;
        if (pSdInfo->scr.config & SCR_SD_BUS_WIDTH_4BIT)
            pSdInfo->dat4Bit = TRUE;
        }

    /* CMD16: set block len */

    if (pDrvCtrl->highCapacity == FALSE)
        {
        rc = sdCmdSetBlockLen (pDev, pDrvCtrl->readBlkLen);
        if (rc == ERROR)
            return ERROR;
        }

    /* switch to 4 bit mode if needed */

    if (pSdInfo->dat4Bit)
        {
        rc = sdACmdSetBusWidth (pDev, SDMMC_BUS_WIDTH_4BIT);
        if (rc == ERROR)
            goto err;

        /* setup host to enable 4-bit bus width */

        if (pHostSpec->vxbSdBusWidthSetup != NULL)
            {
            pHostSpec->vxbSdBusWidthSetup(pSdHardware->pHostDev, SDMMC_BUS_WIDTH_4BIT);
            }
        }

    /* ACMD42: clear card detect and set data3 as data line */
    rc = sdACmdClrCardDetect(pDev, SD_ACMD42_ARG_CLR_CARD_DETECT);
    if (rc == ERROR)
        goto err;	
        
    /* setup host to enable high speed clock (50 MHz) if needed */

    if (pDrvCtrl->highSpeed)
        {
        if ((pHostSpec->vxbSdClkFreqSetup != NULL))
            {
            rc = sdACmdSetHighSpeed(pDev);

            if (rc == OK)
                pHostSpec->vxbSdClkFreqSetup(pSdHardware->pHostDev, SDMMC_CLK_FREQ_50MHZ);
            else if (rc == ERROR)
                goto err;
            else
                {
                pHostSpec->vxbSdClkFreqSetup(pSdHardware->pHostDev, SDMMC_CLK_FREQ_25MHZ);            
                pDrvCtrl->highSpeed = FALSE;
                }
            }
        }

    /* check if card is write protected */

    if (pHostSpec->vxbSdCardWpCheck != NULL)
        {
        pDrvCtrl->isWp = pHostSpec->vxbSdCardWpCheck(pSdHardware->pHostDev);
        }

    /* set xbd params */

    blkXbdParams.xbdOps.blkRead = sdStorageBlkRead;
    blkXbdParams.xbdOps.blkWrite = sdStorageBlkWrite;
    blkXbdParams.xbdOps.blkDump = NULL;
    blkXbdParams.xbdOps.blkIoctl = sdStorageIoctl;
    blkXbdParams.xbdOps.xferReq  = NULL;
    blkXbdParams.maxActiveReqs = 1;
    blkXbdParams.maxBiosPerReq = 1;
    blkXbdParams.maxXferBlks = SDHC_MAX_RW_SECTORS;
    blkXbdParams.directModeFlag = pHostSpec->directBio;
    blkXbdParams.numBlks = (sector_t)(pDrvCtrl->capacity / pSdHardware->blockSize);
    blkXbdParams.blkSize = pSdHardware->blockSize;
    blkXbdParams.svcTskPri = SDMMC_XBD_SVC_TASK_PRI;
    blkXbdParams.pDev = (void *)pDev;
    snprintf ((char *)&(blkXbdParams.devName[0]), sizeof(blkXbdParams.devName),
              "/sd%d", pDrvCtrl->idx);
    pDrvCtrl->attached = TRUE;
    xbdSts = blkXbdDevCreate (&(pDrvCtrl->xbdDev), &blkXbdParams);

    if (xbdSts == NULLDEV)
        {
        pDrvCtrl->attached = FALSE;
        return ERROR;
        }
    return OK;
err:
    return ERROR;
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

LOCAL STATUS sdStatusWaitReadyForData
    (
    VXB_DEVICE_ID pDev
    )
    {
    STATUS rc;
    UINT32 rspValue = 0;

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
* sdACmdSetHighSpeed - put card into high speed mode 
*
* This routine puts card into high speed mode by sending
* application specific command 6. This routine will allocate an internal buffer
* to hold the returned data by ACMD6.
*
* RETURNS: OK if card is successfully put into high speed mode, or ERROR if command
*          issue failed, or ENOTSUP if card can only operate under low speed mode.
*
* ERRNO: N/A
*
* \NOMANUAL
*
* \NOMANUAL
*/

LOCAL int sdACmdSetHighSpeed
    (
    VXB_DEVICE_ID pDev
    )
    {
    STATUS ret = OK;
    SD_HARDWARE * pSdHardWare;
    char * buffer = NULL;
    UINT32 dataVer;
    
    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_CARD_DBG (SD_CARD_DBG_ERR,
                    "sdioCmdIoSendOpCond() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    if (pSdHardWare->version < SD_VERSION_110)
        return (ENOTSUP);

    buffer = cacheDmaMalloc (64);
    if (!buffer) 
        return ERROR;
    memset (buffer, 0, 64);

    ret = sdACmdSwitchFunc (pDev, buffer, 64, SD_ACMD6_GET_FUNC,
                            SD_ACMD6_GROUP_ACCESS_MODE, 1);
    if (ret != OK)
        goto error;

    /* read status buffer */

    if ((buffer[13] & 0x2) == 0)
        {
        /*
         * This means the card does not support high speed
         * we do not need to change
         */
        ret = ENOTSUP;
        goto error;
        }

    /*
     * 375:368 Data Structure Version
     * 00h bits 511:376 are defined
     * 01h bits 511:272 are defined
     * 02h-FFh Reserved
     */

    dataVer = buffer[17];

    if (dataVer == 0)
        {

        /*
         * 379:376 mode 0 - The function which can be switched in
         * function group 1. mode 1 - The function which is result
         * of the switch command, in function group 1.
         * 0xF shows function set error with the argument.
         */

        /* 
         * If the status code equal to the ARG, 
         * the status indicate that the argument is supported.
         */
         
        if ((buffer[16] & 0x0F) == 0 || (buffer[16] & 0x0F) == 0x0F)
            {
            ret = ENOTSUP;
            goto error;
            }
        }
    else if (dataVer == 0x01)
        {

        /*
          * 287:272 Busy Status of functions in group 1.
          * If bit [i] is set, function [i] is busy.
          * This field can be read in mode 0 and mode 1
          * High speed is function 1.
          */
        if (((buffer[28] & 0x02) != 0) ||
            ((buffer[16] & 0x0F) == 0) || 
            ((buffer[16] & 0x0F) == 0x0F))
            {
            ret = ENOTSUP;
            goto error;
            }
        }
    else
        {
        /* Currently, we only support ver0 and ver1 */

        ret = ENOTSUP;
        goto error;
        }

    /* reset the buffer */

    memset(buffer, 0, 64);
    
    ret = sdACmdSwitchFunc (pDev, buffer, 64, SD_ACMD6_SET_FUNC,
                            SD_ACMD6_GROUP_ACCESS_MODE, 1);
    if (ret != OK)
        goto error;

    if ((buffer[16] & 0xf) != 0x1)
        ret = ENOTSUP; 

error:
    cacheDmaFree (buffer);
    return ret;
    }

/*******************************************************************************
*
* sdDecodeSsr - Get ssr register and decode this register.
*
* This routine gets ssr register and decode this register.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS sdDecodeSsr
    (
    VXB_DEVICE_ID pDev
    )
    {
    STATUS ret = OK;
    SD_CARD_CTRL * pDrvCtrl;
    SD_HARDWARE * pSdHardware;
    SD_INFO * pSdInfo;

    pDrvCtrl = (SD_CARD_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return ERROR;

    pSdHardware = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardware == NULL)
        {
        SD_CARD_DBG (SD_CARD_DBG_ERR,
                    "sdioCmdIoSendOpCond() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    pSdInfo = (SD_INFO *)(pDrvCtrl->pInfo);
    if (pSdInfo == NULL)
        {
        SD_CARD_DBG (SD_CARD_DBG_ERR,
                    "sdDecodeSsr() - pSdInfo is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    if (!(pSdInfo->csd.commClass & CCC_APP_SPEC))
        {
        SD_CARD_DBG (SD_CARD_DBG_ERR,
                    "sdDecodeSsr() - commClass not support CCC_APP_SPEC\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    ret = sdACmdSendSsr (pDev, &(pSdInfo->ssr.ssrData[0]));
    if (ret == ERROR)
        {
        SD_CARD_DBG (SD_CARD_DBG_ERR,
                    "sdDecodeSsr() - send sdACmdSendSsr fault\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }
    pSdInfo->speedClass = pSdInfo->ssr.ssrData[8];
    pSdInfo->auSize = pSdInfo->ssr.ssrData[10] & 0xF0;
    pSdInfo->eraseSize = (UINT32)(pSdInfo->ssr.ssrData[11] << 8) | 
                                    pSdInfo->ssr.ssrData[12];
    pSdInfo->eraseTimeout = pSdInfo->ssr.ssrData[13] & 0xFC;
    pSdInfo->eraseOffset = pSdInfo->ssr.ssrData[13] & 0x03;

    return (OK);
    }

/*******************************************************************************
*
* sdErase - send erase/discard/trim command to SD card
*
* This routine sends erase/discard/trim command to SD card.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS sdErase
    (
    VXB_DEVICE_ID pDev,
    SECTOR_RANGE * pRange
    )
    {
    SD_CARD_CTRL * pDrvCtrl;
    SD_INFO * pSdInfo;
    STATUS rc;
    UINT32 eraseArg = 0;
    long long sectorNum;

    if (pRange == NULL)
        return (ERROR);

    pDrvCtrl = (SD_CARD_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return (ERROR);

    pSdInfo = (SD_INFO *)(pDrvCtrl->pInfo);
    if (pSdInfo == NULL)
        {
        SD_CARD_DBG (SD_CARD_DBG_ERR,
                    "sdErase() - pSdInfo is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    if (pDrvCtrl->highCapacity)
        sectorNum = pRange->startSector;
    else
        sectorNum = (pRange->startSector) * pSdInfo->eraseSize;

    rc = mmcCmdSeEraseGrpStart(pDev, (UINT32) sectorNum);
    if (rc != OK)
        return (rc);
    
    if (pDrvCtrl->highCapacity)
        sectorNum = pRange->startSector + pRange->nSectors - 1;
    else
        sectorNum = (pRange->startSector + pRange->nSectors) * pSdInfo->eraseSize;

    rc = mmcCmdSeEraseGrpEnd (pDev, (UINT32) sectorNum);
    if (rc != OK)
        return (rc);

    rc = mmcCmdSetEraseGrp(pDev, eraseArg);
    return (rc);
    }

/*******************************************************************************
*
* sdMmcBlkRead - read block from SD/MMC card
*
* This routine reads block from SD/MMC card.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS sdStorageBlkRead
    (
    void * pDrv,
    XBD_DATA * pXbdData
    )
    {
    STATUS rc;
    ULONG ix, round, remain;
    void *bufAddr;
    sector_t blkAddr;
    SD_CARD_CTRL * pDrvCtrl;
    SD_HARDWARE * pSdHardWare;
    VXB_DEVICE_ID pDev;

    pDev = (VXB_DEVICE_ID)pDrv;
    if (pDev == NULL)
        return (ERROR);

    pDrvCtrl = (SD_CARD_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return (ERROR);

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        return (ERROR);

    /* split the transfer */

    round = pXbdData->numBlks / SDHC_MAX_RW_SECTORS;
    remain = pXbdData->numBlks % SDHC_MAX_RW_SECTORS;

    SD_CARD_DBG(SD_CARD_DBG_RW,
                "sdStorageBlkRead() -  round = %d\n"
                "remain = %d \n",
                round, remain, 0, 0, 0, 0);

    for (ix = 0; ix < round; ix ++)
        {
        blkAddr = pXbdData->blkNum + ix * SDHC_MAX_RW_SECTORS;
        bufAddr = (void *)((ULONG)pXbdData->buffer + ix * SDHC_MAX_RW_SECTORS *
                            pSdHardWare->blockSize);
        SD_CARD_DBG(SD_CARD_DBG_RW,
                    "sdStorageBlkRead() -  blkAddr = 0x%x\n"
                    "bufAddr = 0x%x \n",
                    blkAddr, bufAddr, 0, 0, 0, 0);

        rc = sdCmdReadBlock (pDev, bufAddr, blkAddr,
                             SDHC_MAX_RW_SECTORS, pDrvCtrl->highCapacity);
        if (rc == ERROR)
            return ERROR;

        rc = sdStatusWaitReadyForData(pDev);
        if (rc == ERROR)
            return ERROR;
        }

     if (remain != 0)
         {
         blkAddr = pXbdData->blkNum + round * SDHC_MAX_RW_SECTORS;
         bufAddr = (void *)((ULONG)pXbdData->buffer +
                             round * SDHC_MAX_RW_SECTORS * 
                             pSdHardWare->blockSize);
         SD_CARD_DBG(SD_CARD_DBG_RW,
                     "sdStorageBlkRead() -  blkAddr = 0x%x\n"
                     "bufAddr = 0x%x \n",
                     blkAddr, bufAddr, 0, 0, 0, 0);

         rc = sdCmdReadBlock (pDev, bufAddr, blkAddr, remain,
                              pDrvCtrl->highCapacity);
         if (rc == ERROR)
             return ERROR;

         rc = sdStatusWaitReadyForData(pDrvCtrl->pInst);
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

STATUS sdStorageBlkWrite
    (
    void * pDrv,
    XBD_DATA * pXbdData
    )
    {
    STATUS rc;
    ULONG ix, round, remain;
    void *bufAddr;
    sector_t blkAddr;
    SD_CARD_CTRL * pDrvCtrl;
    SD_HARDWARE * pSdHardWare;
    VXB_DEVICE_ID pDev;

    pDev = (VXB_DEVICE_ID)pDrv;
    if (pDev == NULL)
        return (ERROR);

    pDrvCtrl = (SD_CARD_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return (ERROR);

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        return (ERROR);

    /* split the transfer */

    round = pXbdData->numBlks / SDHC_MAX_RW_SECTORS;
    remain = pXbdData->numBlks % SDHC_MAX_RW_SECTORS;
    SD_CARD_DBG(SD_CARD_DBG_RW,
                "sdStorageBlkWrite() -  round = %d\n"
                "remain = %d \n",
                round, remain, 0, 0, 0, 0);

    for (ix = 0; ix < round; ix ++)
        {
        bufAddr = (void *)((ULONG)pXbdData->buffer + ix * SDHC_MAX_RW_SECTORS *
                           pSdHardWare->blockSize);
        blkAddr = pXbdData->blkNum + ix * SDHC_MAX_RW_SECTORS;
        SD_CARD_DBG(SD_CARD_DBG_RW,
                    "sdStorageBlkWrite() -  blkAddr = 0x%x\n"
                    "bufAddr = 0x%x \n",
                    blkAddr, bufAddr, 0, 0, 0, 0);

        rc = sdCmdWriteBlock (pDrvCtrl->pInst, bufAddr, blkAddr,
                              SDHC_MAX_RW_SECTORS, pDrvCtrl->highCapacity);
        if (rc == ERROR )
            return ERROR;

        rc = sdStatusWaitReadyForData(pDrvCtrl->pInst);
        if (rc == ERROR)
            return ERROR;
        }

    if (remain != 0)
        {
        bufAddr = (void *)((ULONG)pXbdData->buffer + round * SDHC_MAX_RW_SECTORS *
                           pSdHardWare->blockSize);
        blkAddr = pXbdData->blkNum + round * SDHC_MAX_RW_SECTORS;
        SD_CARD_DBG(SD_CARD_DBG_RW,
                    "sdStorageBlkWrite() -  blkAddr = 0x%x\n"
                    "bufAddr = 0x%x \n",
                    blkAddr, bufAddr, 0, 0, 0, 0);
        rc = sdCmdWriteBlock (pDrvCtrl->pInst, bufAddr, blkAddr,
                              remain, pDrvCtrl->highCapacity);
        if (rc == ERROR )
            return ERROR;

         rc = sdStatusWaitReadyForData(pDrvCtrl->pInst);
         if (rc == ERROR)
            return ERROR;
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

LOCAL STATUS sdStorageInstUnlink
    (
    VXB_DEVICE_ID pDev,
    void * unused
    )
    {
    STATUS rc;
    SD_CARD_CTRL * pDrvCtrl;
    pDrvCtrl = (SD_CARD_CTRL *)pDev->pDrvCtrl;

    SD_CARD_DBG(SD_CARD_DBG_INIT,
                "sdStorageInstUnlink() -  remove card\n",
                0, 0, 0, 0, 0, 0);

    if (pDrvCtrl == NULL)
        return ERROR;
    pDrvCtrl->attached = FALSE;
    pDrvCtrl->xbdDev.xbdInserted = FALSE;
    rc = blkXbdDevDelete (&(pDrvCtrl->xbdDev));
    sdCardIdxFree (pDrvCtrl);
    return (rc);
    }

/*******************************************************************************
*
* sdCardIdxAlloc - allocate a global card index
*
* This routine allocates a global card index.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS sdCardIdxAlloc
    (
    SD_CARD_CTRL * pDrvCtrl
    )
    {
    UINT32 * cardIdx;
    int idx;

    cardIdx = &sdDevIdx;

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
* sdCardIdxFree - free a global card index
*
* This routine frees a global card index.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL void sdCardIdxFree
    (
    SD_CARD_CTRL * pDrvCtrl
    )
    {
    UINT32 * cardIdx;

    cardIdx = &sdDevIdx;
    (*cardIdx) &= ~(1 << pDrvCtrl->idx);
    }

/*******************************************************************************
*
* sdMmcXbdIoctl - XBD block device ioctl routine
*
* This routine handles a yet to completed list of ioctl calls.
*
* RETURN: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL int sdStorageIoctl
    (
    void *   pDrv,
    int      cmd,
    void *   arg
    )
    {
    int rc;
    VXB_DEVICE_ID pDev;
    SD_CARD_CTRL * pDrvCtrl;

    if (pDrv == NULL)
        return (ERROR);

    pDev = (VXB_DEVICE_ID)pDrv;
    pDrvCtrl = (SD_CARD_CTRL *)pDev->pDrvCtrl;
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

        default:
            {
            rc = ENOTSUP;
            break;
            }
        }

    return (rc);
    }
