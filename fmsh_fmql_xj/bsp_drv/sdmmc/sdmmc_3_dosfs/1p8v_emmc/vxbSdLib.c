/* vxbSdLib.c - Generic SD/SDIO bus Library */

/*
 * Copyright (c) 2012-2015, 2017-2018 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01n,14sep18,j_x  fix sd card initialization is very slow (VXW6-86555)
01m,07sep18,d_l  add return value check for CMD8 (VXW6-86918)
01l,03sep18,d_l  update SD_LIB_DBG function name (VXW6-86912)
01k,01nov17,d_l  Add CMD8 after CMD0. (VXW6-86301)
01j,26oct15,myt  add support of reliable write for eMMC (VXW6-84383)
01i,18jun15,zly  ACMD42 is not included in SD card driver.(VXW6-84329) 
01h,11feb15,yjl  Fix VXW6-83847, vxbSdLib not conform to JEDEC specification
01g,18nov14,m_y  modify sdDeviceAnnounce for resource leak(VXW6-83807)
01f,12nov14,m_y  modify to fix build error (VXW6-83787)
01e,06nov14,m_y  correct sdDeviceAnnounce (VXW6-83753)
01d,03nov14,m_y  modify to fix prevent issue (VXW6-83592)
01c,28oct13,e_d  fix some prevent issue. (WIND00440964)
01b,24sep13,e_d  add some advanced functionalities for SD/MMC. (WIND00437987)
01a,18aug12,e_d  written.
*/

/*
DESCRIPTION

This is the generic SD/SDIO bus library to implement neccessary SD commands for
a SD memory card or SDIO card which is compatible with SD memory card
specification 2.0.

The library can support up to 32 SD/MMC cards in a system.
*/

/* includes */

#include <vxWorks.h>
#include <intLib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vxBusLib.h>
#include <cacheLib.h>
#include <iv.h>
#include <vxbTimerLib.h>
#include <hwif/vxbus/hwConf.h>
#include <hwif/vxbus/vxbPlbLib.h>
#include <hwif/util/hwMemLib.h>
#include <hwif/util/vxbParamSys.h>
#include <hwif/vxbus/vxBus.h>
#include <hwif/vxbus/vxbSdLib.h>

/* defines */
#undef SD_LIB_DBG_ON

#undef DRV_ORIGIN

#ifdef  SD_LIB_DBG_ON

#ifdef  LOCAL
#undef  LOCAL
#define LOCAL
#endif /* LOCAL */

#define SD_LIB_DBG_BUSINIT       0x00000001
#define SD_LIB_DBG_CMD           0x00000002
#define SD_LIB_DBG_ERR           0x00000004
#define SD_LIB_DBG_INT           0x00000008
#define SD_LIB_DBG_ALL           0xffffffff
#define SD_LIB_DBG_OFF           0x00000000

UINT32 sdLibDbgMask = SD_LIB_DBG_ALL;

IMPORT FUNCPTR _func_logMsg;

/*
#define SD_LIB_DBG(mask, string, a, b, c, d, e, f)               \
    if ((sdLibDbgMask & mask) || (mask == SD_LIB_DBG_ALL))       \
        if (_func_logMsg != NULL) \
            (* _func_logMsg)(string, a, b, c, d, e, f)
            
*/
#define SD_LIB_DBG(mask, string, a, b, c, d, e, f)  printf(string, a, b, c, d, e, f)

#else
#   define SD_LIB_DBG(mask, string, a, b, c, d, e, f)
#endif  /* SDMMC_LIB_DBG_ON */

#define SD_CONTROLLER_METHOD_INTERRUPT_INFO    \
    ((VXB_METHOD_ID)sdBusCtlrInterruptInfo_desc)

/* locals */

LOCAL STATUS sdInit (void);
LOCAL STATUS sdInit2 (void);
LOCAL STATUS sdConnect (void);
LOCAL STATUS sdDeviceAnnounce (VXB_DEVICE_ID, void *);
LOCAL BOOL sdDevMatch (struct vxbDevRegInfo * pDriver, struct vxbDev * pDev);

/* local data */

LOCAL BOOL sdBusLibInitialized = FALSE;

LOCAL struct vxbBusTypeInfo sdBusType = {
    NULL,            /* pNext */
    VXB_BUSID_SD,    /* busID */
    sdInit,          /* busTypeInit */
    sdInit2,         /* busTypeInit2 */
    sdConnect,       /* busTypeConnect */
    noDev,           /* busTypeNewDevPresent */
    sdDevMatch       /* vxbDevMatch */
};

/*******************************************************************************
*
* sdRegister - register SD bus type
*
* This routine registers the SD bus type with the vxBus subsystem.
*
* RETURNS: OK, or ERROR
*
* ERRNO
*/

STATUS sdRegister(void)
    {
    STATUS rc;
    if (sdBusLibInitialized)
        {
        return(ERROR);
        }

    /* register SD as a valid bus type */

    rc = vxbBusTypeRegister (&sdBusType);
    if (rc == OK)
        {
        sdBusLibInitialized = TRUE;
        return(OK);
        }
    else
        return (ERROR);
    }

/*******************************************************************************
*
* sdDevMatch - check whether device and driver go together
*
* This routine checks that the specified device and device driver
* are a matched pair.
*
* RETURNS: TRUE if the device and driver match, FALSE otherwise
*
* ERRNO
*/

LOCAL BOOL sdDevMatch
    (
    struct vxbDevRegInfo * pDriver,    /* Pointer to device driver */
    struct vxbDev * pDev               /* Device information */
    )
    {
    struct vxbSdRegister *  pSdDriver;
    SD_HARDWARE *  pSdDev;
    VXB_SDIO_ID *  pDevVend;
    int i;

    /* the following two checks are almost duplicates of vxBus.c code */

    if ( pDriver->busID != VXB_BUSID_SD || pDev->busID != VXB_BUSID_SD )
        return(FALSE);

    pSdDev = (SD_HARDWARE *)pDev->pBusSpecificDevInfo;
    pSdDriver = (struct vxbSdRegister *)pDriver;

    if ((pSdDev == NULL) || (pSdDriver == NULL))
        return FALSE;

    if (!(pSdDev->isSdio))
        {
        if (pSdDriver->b.devProbe != NULL )
            return TRUE;
        else
            return FALSE;
        }

    /*
     * Before checking the deviceID and vendorID of the device
     * against the values registered by the driver, we check to
     * see if the driver lets us know not to do that.  In some
     * cases, drivers may want to match against a class of devices,
     * rather than against a devID/vendID pair.  In this case,
     * the driver must specify devID and vendID of -1, and include
     * a driver probe routine.  The values -1 are not legal values
     * according to the PCI spec, so this should not cause any
     * problems.
     */

    if ( (pSdDriver->idList[0].sdioDevId == 0xFFFF ) &&
         (pSdDriver->idList[0].sdioVenderId == 0xFFFF ) &&
         (pSdDriver->b.devProbe != NULL ) )
        {
        SD_LIB_DBG (SD_LIB_DBG_BUSINIT,
                    "sdDevMatch(): match unspecified devID/vendID\n",
                    0, 0, 0, 0, 0, 0);
        return(TRUE);
        }

    pDevVend = pSdDriver->idList;

    for (i = 0 ; i < pSdDriver->idListLen ; i++)
        {
        if ( (pSdDev->vxbSdioId.sdioDevId == pDevVend->sdioDevId) &&
            (pSdDev->vxbSdioId.sdioVenderId == pDevVend->sdioVenderId) )
            {
            return(TRUE);
            }

        pDevVend++;
        }

    SD_LIB_DBG (SD_LIB_DBG_BUSINIT,"sdDevMatch(): no match found\n",
                0, 0, 0, 0, 0, 0);

    return(FALSE);
    }

/*******************************************************************************
*
* sdInit - first-pass bus type initialization
*
* This function currently does nothing.
*
* RETURNS: OK, always
*
* ERRNO
*/

LOCAL STATUS sdInit(void)
    {
    return (OK);
    }

/*******************************************************************************
*
* sdInit2 - second-pass bus type initialization
*
* This function currently does nothing.
*
* RETURNS: OK, always
*
* ERRNO
*/

LOCAL STATUS sdInit2(void)
    {
    return (OK);
    }

/*******************************************************************************
*
* sdConnect - connect SD bus type to bus subsystem
*
* This function currently does nothing.
*
* RETURNS: OK, always
*
* ERRNO
*/

LOCAL STATUS sdConnect(void)
    {
    return (OK);
    }

/*******************************************************************************
*
* sdDeviceAnnounce - notify the bus subsystem of a device on SD bus
*
* This routine send SDIO cmd5 to target and check target's type. And store the
* cmdIssue pointer to SD_HARDWARE struct. The SD_HARDWARE sturct is stored to
* the pBusSpecificDevInfo field of VxBus device structure.
* finally, vxbDeviceAnnounce is called to announce the device to the system.
*
* RETURNS: OK, or ERROR
*
* ERRNO
*/

LOCAL STATUS sdDeviceAnnounce_2
    (
    VXB_DEVICE_ID pInst,
    void * pArg
    )
    {
    struct vxbDev * pDev;
    SD_HARDWARE   * pSdHardWare;
    struct vxbDev * pUpstreamDev;
    SD_HOST_SPEC  * pSdHostSpec;
    FUNCPTR pFunc;
    int i = 0;
    int j = 0;
    STATUS rc;
    UINT32  capbility;
    UINT32 timeOut = 0;

    /* initialize generic bus info */
#if 1
    pDev = vxbDevStructAlloc (WAIT_FOREVER);

    if (pDev == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdDeviceAnnounce() - vxbDevStructAlloc not success\n",
                    0, 0, 0, 0, 0 ,0);
        return(ERROR);
        }
#endif

    /* initialize bus-specific info */

    pSdHardWare = (SD_HARDWARE *) malloc(sizeof (SD_HARDWARE));

    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdDeviceAnnounce - hwMemAlloc not success for pSdHardWare\n",
                    0, 0, 0, 0, 0 ,0);
#ifndef _WRS_CONFIG_VXBUS_BASIC
        vxbDevStructFree(pDev);
#endif /* !_VXBUS_BASIC_HWMEMLIB */
        return(ERROR);
        }

    bzero ((char *)pSdHardWare, sizeof(SD_HARDWARE));

    pFunc = vxbDevMethodGet(pInst, (VXB_METHOD_ID)vxbSdSpecInfoGet_desc);
    if (pFunc == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdDeviceAnnounce - vxbDevMethodGet cmdIssue not success\n",
                    0, 0, 0, 0, 0 ,0);
        goto errExit;
        }

    pFunc(pInst, &(pSdHardWare->pHostSpec), &(pSdHardWare->vxbSdCmdIssue));
    pSdHardWare->pHostDev = pInst;
    pDev->pNext = NULL;
    pDev->pParentBus = pInst->u.pSubordinateBus;
    pDev->u.pSubordinateBus = NULL;
    pDev->busID = VXB_BUSID_SD;
    pDev->pBusSpecificDevInfo = (void *) pSdHardWare;
    pSdHostSpec = (SD_HOST_SPEC *)(pSdHardWare->pHostSpec);
    if (pSdHostSpec == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdDeviceAnnounce - vxbDevMethodGet cmdIssue not success\n",
                    0, 0, 0, 0, 0 ,0);
        goto errExit;
        }

    capbility = pSdHostSpec->capbility;
    printf("################capbility = %x\n",capbility);

    /* save interrupt info in vxbDev structure */

    pDev->pIntrInfo = NULL;
    pDev->pDriver = NULL;
    rc = sdCmdGoIdleState (pDev);
    if (rc == ERROR)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdDeviceAnnounce - sdCmdGoIdleState not success\n",
                    0, 0, 0, 0, 0 ,0);
        goto errExit;
        }

    /* CMD8 is required to support SDHC or SDXC */
    rc = sdCmdSendIfCond (pDev, 0);
    if (rc == ERROR)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdDeviceAnnounce - sdCmdSendIfCond not success\n",
                    0, 0, 0, 0, 0 ,0);
        goto errExit;
        } 

    rc = sdioCmdIoSendOpCond(pDev, capbility & OCR_VDD_VOL_MASK);

    if ((pSdHardWare->cmdErr & SDMMC_CMD_ERR_TIMEOUT) != 0x0)
        {
        rc = sdMmcCmdSendOpCond(pDev, 0); 
        
        if ((rc == ERROR) ||
            ((pSdHardWare->cmdErr & SDMMC_CMD_ERR_TIMEOUT) != 0x0))
            pSdHardWare->isMmc = FALSE;
        else
            pSdHardWare->isMmc = TRUE;

        if (pSdHardWare->isMmc)
            {
            sdCmdGoIdleState(pDev);
            while (timeOut++ < SDMMC_COMMAND_TIMEOUT)
                {
                rc = sdMmcCmdSendOpCond(pDev,
                                        (pSdHardWare->ocrValue |
                                        OCR_CARD_CAP_STS));
                if ((rc == OK) &&
                    ((pSdHardWare->ocrValue & (UINT32)OCR_CARD_PWRUP_STS) != 0x0))
                    {
                    pSdHardWare->voltage = (pSdHardWare->ocrValue) & OCR_VDD_VOL_MASK;
                    break;
                    }
                vxbMsDelay(1);
                }
            pDev->pName = MMC_CARD_NAME;
            }
        else
            {
            (void)sdCmdSendIfCond (pDev, capbility);
            if (pSdHardWare->version == SD_VERSION_UNKNOWN)
                {
                SD_LIB_DBG (SD_LIB_DBG_ERR,
                           "sdDeviceAnnounce - sdCmdGoIdleState not success\n",
                           0, 0, 0, 0, 0 ,0);
                goto errExit;
                }
            while (timeOut++ < SDMMC_COMMAND_TIMEOUT)
                {
                rc = sdACmdSendOpCond (pDev, capbility, pSdHardWare->version);
              /*  printf("pSdHardWare->ocrValue=%x rc=%d \n",pSdHardWare->ocrValue,rc);*/
                if ((rc == OK) && ((pSdHardWare->ocrValue & (UINT32)OCR_CARD_PWRUP_STS) != 0x0))
                    {
                    pSdHardWare->voltage = (pSdHardWare->ocrValue) & OCR_VDD_VOL_MASK;
                    break;
                    }
                
                vxbMsDelay(1);
                }
            pDev->pName = SD_CARD_NAME;
            }
        if (timeOut >= SDMMC_COMMAND_TIMEOUT)
            {
            SD_LIB_DBG (SD_LIB_DBG_ERR,
                       "sdDeviceAnnounce - sdACmdSendOpCond not success\n",
                       0, 0, 0, 0, 0 ,0);
            goto errExit;
            }

        rc = sdCmdAllSendCid(pDev, (void *) (&pSdHardWare->cidValue[0]));
        if ((rc != ERROR) && ((pSdHardWare->cmdErr & SDMMC_CMD_ERR_TIMEOUT) == 0x0))
            {
            SD_LIB_DBG (SD_LIB_DBG_BUSINIT,
                        "sdioDeviceAnnounce - found sd card target\n",
                        0, 0, 0, 0, 0 ,0);
            pSdHardWare->isComboCard = FALSE;
            pSdHardWare->isSdio = FALSE;
            for (i = 0; i < 9; i++)
                {
                pDev->pRegBase[i] = (void *) 0;
                pDev->regBaseSize[i] = 0;
                pDev->regBaseFlags[i] = VXB_REG_NONE;
                }

            /* retrieve the upstream bus device info */

            VXBAF_RETRIEVE_UPSTREAM_BUS_PTR (pDev, pUpstreamDev);

            /* if the parent device is invalid, return ERROR */

            if (pUpstreamDev == NULL)
                {
                SD_LIB_DBG (SD_LIB_DBG_ERR,
                            "sdioDeviceAnnounce - pUpstreamDev is NULL\n",
                            0, 0, 0, 0, 0 ,0);
                goto errExit;
                }
            vxbDeviceAnnounce (pDev);

            SD_LIB_DBG (SD_LIB_DBG_BUSINIT,
                        "sdioDeviceAnnounce - ok\n",
                        0, 0, 0, 0, 0 ,0);
            return OK;
            }
        else
            {
            goto errExit;
            }
        }

    if (rc == OK)
        {
        UINT8 funcNum = 0;

        funcNum = (pSdHardWare->ocrValue >> 28) & 0x07;
        if (funcNum > 0)
            {
            rc = sdioCmdIoSendOpCond(pDev, capbility & 0x00FFFFFF);
            if (rc == ERROR)
                {
                SD_LIB_DBG (SD_LIB_DBG_ERR,
                            "sdDeviceAnnounce - sdioCmdIoSendOpCond not success\n",
                            0, 0, 0, 0, 0 ,0);
                goto errExit;
                }

            do{
                rc = sdCmdSendRelativeAddr (pDev);
                if (rc == ERROR)
                   {
                   SD_LIB_DBG (SD_LIB_DBG_ERR,
                               "sdDeviceAnnounce - sdioCmdIoSendOpCond not success\n",
                               0, 0, 0, 0, 0 ,0);
                   goto errExit;
                   }
                } while (pSdHardWare->rcaValue == 0);

            rc = sdCmdSelectCard (pDev);
            if (rc == ERROR)
                {
                SD_LIB_DBG (SD_LIB_DBG_ERR,
                            "sdDeviceAnnounce - sdioCmdIoSendOpCond not success\n",
                            0, 0, 0, 0, 0 ,0);
                goto errExit;
                }

            for (i = 0; i < funcNum; i++)
                {
                if (pDev == NULL)
                    {
                    pDev = vxbDevStructAlloc (WAIT_FOREVER);
                    if (pDev == NULL)
                        {
                        SD_LIB_DBG (SD_LIB_DBG_ERR,
                        "sdioDeviceAnnounce() - vxbDevStructAlloc not success\n",
                        0, 0, 0, 0, 0 ,0);

                        return(ERROR);
                        }
                    }
                if (pSdHardWare == NULL)
                    {
                    pSdHardWare = (SD_HARDWARE *) malloc (sizeof (SD_HARDWARE));

                    if (pSdHardWare == NULL)
                        {
                        SD_LIB_DBG (SD_LIB_DBG_ERR,
                                    "sdioDeviceAnnounce - hwMemAlloc not"
                                    " success for pSdHardWare\n",
                                    0, 0, 0, 0, 0 ,0);
#ifndef _WRS_CONFIG_VXBUS_BASIC
                        vxbDevStructFree(pDev);
#endif /* !_WRS_CONFIG_VXBUS_BASIC */
                        return(ERROR);
                        }

                    bzero ((char *)pSdHardWare, sizeof(SD_HARDWARE));
                    pFunc = vxbDevMethodGet(pInst, (VXB_METHOD_ID)vxbSdSpecInfoGet_desc);
                    if (pFunc == NULL)
                        {
                        SD_LIB_DBG (SD_LIB_DBG_ERR,
                                   "sdioDeviceAnnounce - vxbDevMethodGet"
                                   " cmdIssue not success\n",
                                   0, 0, 0, 0, 0 ,0);
#ifndef _WRS_CONFIG_VXBUS_BASIC
                        vxbDevStructFree(pDev);
#endif /* !_WRS_CONFIG_VXBUS_BASIC */
                        free((char *) pSdHardWare);
                        return(ERROR);
                        }
                    pFunc(pInst, pSdHardWare->pHostSpec, pSdHardWare->vxbSdCmdIssue);
                    }

                pSdHardWare->pHostDev = pInst;
                pSdHardWare->isSdio = TRUE;
                pSdHardWare->isComboCard = FALSE;
                pSdHardWare->funcNum = i + 1;
                pDev->pNext = NULL;
                pDev->pParentBus = pInst->u.pSubordinateBus;
                pDev->u.pSubordinateBus = NULL;
                pDev->busID = VXB_BUSID_SD;
                pDev->pBusSpecificDevInfo = (void *) pSdHardWare;

                /* save interrupt info in vxbDev structure */

                pDev->pIntrInfo = NULL;
                pDev->pDriver = NULL;
                for (j = 0; j < 9; j++)
                    {
                    pDev->pRegBase[i] = (void *) 0;
                    pDev->regBaseSize[i] = 0;
                    pDev->regBaseFlags[i] = VXB_REG_NONE;
                    }

                /* retrieve the upstream bus device info */

                VXBAF_RETRIEVE_UPSTREAM_BUS_PTR (pDev, pUpstreamDev);

                /* if the parent device is invalid, return ERROR */

                if (pUpstreamDev == NULL)
                    {
                    SD_LIB_DBG (SD_LIB_DBG_ERR,
                                "sdioDeviceAnnounce - pUpstreamDev is NULL\n",
                                0, 0, 0, 0, 0 ,0);
#ifndef _WRS_CONFIG_VXBUS_BASIC
                    vxbDevStructFree (pDev);
#endif /* !_WRS_CONFIG_VXBUS_BASIC */
                    free ((char *) pSdHardWare);
                    return ERROR;
                    }

                /* Need not check return status at here */

                (void)vxbDeviceAnnounce (pDev);

                SD_LIB_DBG (SD_LIB_DBG_BUSINIT,
                            "sdioDeviceAnnounce - ok\n",
                            0, 0, 0, 0, 0 ,0);
                pDev = NULL;
                pSdHardWare = NULL;
                }
            return OK;
            }
        }

errExit:

#ifndef _WRS_CONFIG_VXBUS_BASIC
    vxbDevStructFree(pDev);
#endif /* !_WRS_CONFIG_VXBUS_BASIC */
    free((char *) pSdHardWare);
    return(ERROR);
    }

/*******************************************************************************
*
* sdioBusAnnounceDevices - Notify the bus subsystem of all devices on SD bus
*
* This routine get the device description with the standard VxBus devResourceGet
* API, then announce every SD device by calling sdioDeviceAnnounce.
*
* RETURNS: VOID
*
* ERRNO
*/

void sdBusAnnounceDevices_2
    (
    VXB_DEVICE_ID pDev,
    void * pCookie
    )
    {
    (void)sdDeviceAnnounce_2(pDev, pCookie);
    }

/*******************************************************************************
*
* sdCmdGoIdleState - issue CMD0 to reset all cards to idle state
*
* This routine issues CMD0 to reset all cards to idle state.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdGoIdleState
    (
    VXB_DEVICE_ID pDev
    )
    {
    SD_CMD cmd;
    STATUS rc;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdGoIdleState() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    SD_CMD_FORM (cmd, SDMMC_CMD_GO_IDLE_STATE, 0, SDMMC_CMD_RSP_NONE, FALSE);

    SD_LIB_DBG (SD_LIB_DBG_CMD,
                "sdCmdGoIdleState command\n",
                0, 0, 0, 0, 0 ,0);

    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);

    /* Need at last 1ms delay for waiting for CMD0 respond */
    vxbMsDelay(1);
    return (rc);
    }

/*******************************************************************************
*
* sdMmcCmdSendOpCond - issue CMD1 to get mmc card OCR value
*
* This routine issues CMD1 to get mmc card OCR value.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdSendOpCond
    (
    VXB_DEVICE_ID pDev,
    UINT32 ocrValue
    )
    {
    SD_CMD cmd;
    STATUS rc;
    SD_HARDWARE * pSdHardWare;
    int i = 0;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "mmcCmdSendOpCond() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }
    SD_CMD_FORM (cmd, MMC_CMD_SEND_OP_COND, ocrValue, SDMMC_CMD_RSP_R3, FALSE);

    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);
    pSdHardWare->cmdErr = cmd.cmdErr;
    pSdHardWare->dataErr = cmd.dataErr;

    if (rc == ERROR)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "mmcCmdSendOpCond() - cmdIssue return error\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    pSdHardWare->ocrValue = cmd.cmdRsp[0];
    SD_LIB_DBG (SD_LIB_DBG_CMD,
                "mmcCmdSendOpCond() - ocrValue = 0X%x\n",
                cmd.cmdRsp[0], 0, 0, 0, 0 ,0);

    return (rc);
    }

/*******************************************************************************
*
* sdCmdAllSendCid - issue CMD2 to ask all cards to send their CIDs
*
* This routine issues CMD2 to ask all cards to send their CIDs.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdAllSendCid
    (
    VXB_DEVICE_ID pDev,
    void * pCid
    )
    {
    SD_CMD cmd;
    STATUS rc;
    SD_HARDWARE * pSdHardWare;

    if (pCid == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdAllSendCid() - pCid is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdAllSendCid() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    SD_CMD_FORM (cmd, SDMMC_CMD_ALL_SEND_CID, 0, SDMMC_CMD_RSP_R2, FALSE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);
    if (rc == ERROR)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdAllSendCid() - cmdIssue return error\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    bcopy ((char *)cmd.cmdRsp, (char *)pCid, CID_SIZE);
    SD_LIB_DBG (SD_LIB_DBG_CMD,
                "sdCmdAllSendCid() - cmdRsp[0] = 0X%x\n",
                cmd.cmdRsp[0], 0, 0, 0, 0 ,0);

    return OK;
    }

/*******************************************************************************
*
* sdCmdSendRelativeAddr - issue CMD3 to ask card to publish a new RCA
*
* This routine issues CMD3 to ask card to publish a new RCA.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdSendRelativeAddr
    (
    VXB_DEVICE_ID pDev
    )
    {
    SD_CMD cmd;
    UINT16 rcaTemp = 0;
    STATUS rc;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdSendRelativeAddr() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    SD_CMD_FORM (cmd, SD_CMD_SEND_RELATIVE_ADDR, 0, SDMMC_CMD_RSP_R6, FALSE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);
    if (rc == ERROR)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdSendRelativeAddr() - cmdIssue return error\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    rcaTemp = (cmd.cmdRsp[0] >> 16) & 0xffff;
    pSdHardWare->rcaValue = rcaTemp;

    SD_LIB_DBG (SD_LIB_DBG_CMD,
                "sdCmdSendRelativeAddr() - rcaValue = 0X%x\n",
                rcaTemp, 0, 0, 0, 0 ,0);

    return (OK);
    }

/*******************************************************************************
*
* sdMmcCmdSetRelativeAddr - issue CMD3 to ask card to publish a new RCA
*
* This routine issues CMD3 to ask card to publish a new RCA.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdSetRelativeAddr
    (
    VXB_DEVICE_ID pDev
    )
    {
    SD_CMD cmd;
    UINT16 rcaTemp = 0;
    STATUS rc;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdMmcCmdSetRelativeAddr() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    if (pSdHardWare->rcaValue == 0x0)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdMmcCmdSetRelativeAddr() - pSdHardWare rcaValue is 0x0\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    SD_CMD_FORM (cmd, MMC_CMD_SET_RELATIVE_ADDR, (pSdHardWare->rcaValue << 16),
                 SDMMC_CMD_RSP_R1, FALSE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);
    if (rc == ERROR)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdMmcCmdSetRelativeAddr() - cmdIssue return error\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    return (OK);
    }

/*******************************************************************************
*
* sdCmdSetDsr - issue CMD4 to program the DSR of all cards
*
* This routine issues CMD4 to program the DSR of all cards.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdSetDsr
    (
    VXB_DEVICE_ID pDev,
    UINT32        dsr
    )
    {
    SD_CMD cmd;

    SD_CMD_FORM (cmd, SDMMC_CMD_SET_DSR, dsr << 16, SDMMC_CMD_RSP_NONE, FALSE);

    return (sdCmdIssue(pDev, &cmd));
    }

/*******************************************************************************
*
* sdioCmdIoSendOpCond - issue CMD5 to ask SDIO card to publish OCR value
*
* This routine issues CMD5 to ask SDIO card to publish OCR value.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdioCmdIoSendOpCond
    (
    VXB_DEVICE_ID pDev,
    UINT32 ocrValue
    )
    {
    SD_CMD cmd;
    STATUS rc;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdioCmdIoSendOpCond() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }


    SD_CMD_FORM (cmd, SDIO_CMD_SEND_OPCOND, ocrValue, SDMMC_CMD_RSP_R4, FALSE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);
    pSdHardWare->cmdErr = cmd.cmdErr;
    pSdHardWare->dataErr = cmd.dataErr;

    if (rc == ERROR)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdioCmdIoSendOpCond() - cmdIssue return error\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    pSdHardWare->ocrValue = cmd.cmdRsp[0];
    SD_LIB_DBG (SD_LIB_DBG_CMD,
                "sdioCmdIoSendOpCond() - ocrValue = 0X%x\n",
                cmd.cmdRsp[0], 0, 0, 0, 0 ,0);

    return (OK);
    }

/*******************************************************************************
*
* mmcSwitch - issue CMD6 to write MMC card's ext_csd register.
*
* This routine issue CMD6 to read/write MMC card's ext_csd register. Index is
* byte offset address in ext_csd register. Data is wrote to ext_csd register's
* value. Set is operation attribute.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS mmcSwitch
    (
    VXB_DEVICE_ID pDev,
    UINT8  index,
    UINT8  data,
    UINT8  set
    )
    {
    SD_CMD cmd;
    STATUS rc;
    UINT32 arg = 0;
    UINT32 rspValue = 0;
    UINT8  cmdSts = 0;
    UINT32 timeOut = 0;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "mmcSwitch() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
           (index << 16) |
           (data << 8) |
           set;

    SD_CMD_FORM (cmd, MMC_CMD_SWITCH, arg, SDMMC_CMD_RSP_R1B, FALSE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);
    if (rc == ERROR)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "mmcSwitch() - cmdIssue return error\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }
    do
        {

        /* Need not check return status */

        (void)sdCmdSendStatus (pDev, &rspValue);
        timeOut++;
        if (timeOut >= SDMMC_COMMAND_TIMEOUT)
            return (ERROR);
        }while(CARD_STS_CUR_GET(rspValue) == CARD_STS_PRG);

    return (OK);
    }

/*******************************************************************************
*
* sdACmdSwitchFunc - send application specific command 6
*
* This routine sends application specific command 6. The input buffer and
* buffer length are provided by the caller.
*
* RETURNS: OK, or ERROR if command issue failed
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS sdACmdSwitchFunc
    (
    VXB_DEVICE_ID pDev,
    void * buffer,
    UINT32 len,
    int    mode,
    int    group,
    int    val
    )
    {
    SD_CMD cmd;
    STATUS rc;
    UINT32 arg = 0;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdACmdSwitchFunc() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    arg = mode;
    arg |= 0xffffff;
    arg &= ~(0xf << (group * 4));
    arg |= (val << (group * 4));

    SD_CMD_FORM (cmd, SD_CMD_SWITCH_FUNC, arg, SDMMC_CMD_RSP_R1, TRUE);
    SD_DATA_FORM (cmd, buffer, 1, len, TRUE);

    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);
    if (rc == ERROR)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdACmdSwitchFunc() - cmdIssue return error\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    pSdHardWare->cmdErr = cmd.cmdErr;
    pSdHardWare->dataErr = cmd.dataErr;

    return OK;
    }

/*******************************************************************************
*
* sdCmdSelectCard - issue CMD7 to select a card
*
* This routine issues CMD7 to select a card.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdSelectCard
    (
    VXB_DEVICE_ID pDev
    )
{
    SD_CMD cmd;
    SD_HARDWARE * pSdHardWare;

	/* jc*/
    STATUS rc;
    UINT32 i;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdSelectCard() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    SD_CMD_FORM (cmd, SDMMC_CMD_SELECT_CARD,
                 SDMMC_CMD_ARG_RCA (pSdHardWare->rcaValue),
                 SDMMC_CMD_RSP_R1, FALSE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

#ifdef DRV_ORIGIN  /* origin*/
    return (pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd));
#else  /* jc*/
    for (i = 0; i < 10; i++)
    {
        rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);
        if (rc == ERROR)
            continue;
        else
            break;
    }

#endif
}

/*******************************************************************************
*
* sdCmdDeselectCard - issue CMD7 to deselect a card
*
* This routine issues CMD7 to deselect a card.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdDeselectCard
    (
    VXB_DEVICE_ID pDev
    )
    {
    SD_CMD cmd;
	
#ifdef DRV_ORIGIN  /* origin*/
    SD_CMD_FORM (cmd, SDMMC_CMD_SELECT_CARD, 0, SDMMC_CMD_RSP_NONE, FALSE);
#else
    SD_CMD_FORM (cmd, SDMMC_CMD_SELECT_CARD, 0, SDMMC_CMD_RSP_R1B, FALSE);
#endif

    return (sdCmdIssue(pDev, &cmd));
    }

/*******************************************************************************
*
* sdCmdSendIfCond - issue CMD8 to ask card to send interface condition
*
* This routine issues CMD8 to ask card to send interface condition.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdSendIfCond
    (
    VXB_DEVICE_ID pDev,
    UINT32 capbility
    )
    {
    SD_CMD cmd;
    UINT32 arg = 0;
    STATUS rc;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdSendIfCond() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }
	
#ifdef DRV_ORIGIN  /* jc 2020-09-28, sd/emmc verify_board*/
    arg = (SD_CMD8_ARG_VHS_27_36 << SD_CMD8_ARG_VHS_SHIFT) |
           SD_CMD8_ARG_CHK_PATTERN;
#else  /* for ruineng*/
    arg = (SD_CMD8_ARG_VHS_LOW_VOLTAGE << SD_CMD8_ARG_VHS_SHIFT) |
           SD_CMD8_ARG_CHK_PATTERN;
	/*arg = 0xAA;*/
#endif

    SD_CMD_FORM (cmd, SD_CMD_SEND_IF_COND, arg, SDMMC_CMD_RSP_R7, FALSE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);

    if (cmd.cmdErr & SDMMC_CMD_ERR_TIMEOUT)
        {
        pSdHardWare->version = SD_VERSION_100;
        SD_LIB_DBG (SD_LIB_DBG_CMD,
                    "sdCmdSendIfCond() - sd target version is SD_VERSION_100\n",
                    0, 0, 0, 0, 0 ,0);
        return (OK);
        }

    if (rc == ERROR)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdSendIfCond() - cmdIssue return error\n",
                    0, 0, 0, 0, 0 ,0);
        return (ERROR);
        }

    if ((cmd.cmdRsp[0] & 0xff) != SD_CMD8_ARG_CHK_PATTERN)
        {
        SD_LIB_DBG (SD_LIB_DBG_CMD,
                    "sdCmdSendIfCond() - sd target version is SD_VERSION_UNKNOWN\n",
                    0, 0, 0, 0, 0 ,0);
        pSdHardWare->version = SD_VERSION_UNKNOWN;
        }
    else
        {
        SD_LIB_DBG (SD_LIB_DBG_CMD,
                    "sdCmdSendIfCond() - sd target version is SD_VERSION_200\n",
                    0, 0, 0, 0, 0 ,0);
        pSdHardWare->version = SD_VERSION_200;
        }
    return (OK);
    }

/*******************************************************************************
*
* mmcCmdReadExtCsd - issue CMD8 to read MMC card's ext_csd register
*
* This routine issue CMD8 to read MMC card's ext_csd register.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS mmcCmdReadExtCsd
    (
    VXB_DEVICE_ID  pDev,
    UINT8 * pExtCsd
    )
    {
    SD_CMD cmd;
    UINT8 * pExtData;
    STATUS rc;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "mmcCmdReadExtCsd() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    pExtData = (UINT8 *)cacheDmaMalloc (sizeof(UINT8) * 512);
    bzero((char *)pExtData, sizeof(512));
    if (pExtData == NULL)
        return ERROR;

    SD_CMD_FORM (cmd, MMC_CMD_SEND_EXT_CSD, 0, SDMMC_CMD_RSP_R1, TRUE);
    SD_DATA_FORM (cmd, pExtData, 1, (sizeof(UINT8) * 512), TRUE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);

    if (rc == ERROR)
        {
    	(void)cacheDmaFree((void *)pExtData);
        return ERROR;
        }
    bcopy ((char *)pExtData, (char *)(pExtCsd), (sizeof(UINT8) * 512));
    (void)cacheDmaFree((void *)pExtData);
    return (OK);
    }

/*******************************************************************************
*
* sdCmdSendCsd - issue CMD9 to ask card to send its CSD
*
* This routine issues CMD9 to ask card to send its CSD.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdSendCsd
    (
    VXB_DEVICE_ID pDev,
    SDMMC_CSD * pCsd
    )
    {
    SD_CMD cmd;
    UINT32 csize, csizeMulti;
    STATUS rc;
    UINT64 blkNum;
    SD_HARDWARE * pSdHardWare;

    if (pCsd == NULL)
        return ERROR;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdSendCsd() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    SD_CMD_FORM (cmd, SDMMC_CMD_SEND_CSD, SDMMC_CMD_ARG_RCA (pSdHardWare->rcaValue),
                 SDMMC_CMD_RSP_R2, FALSE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);
    if (rc == ERROR)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdSendCsd() - cmdIssue return error\n",
                    0, 0, 0, 0, 0 ,0);
        return (ERROR);
        }

    bcopy ((char *)cmd.cmdRsp, (char *)pCsd, 16);
    return OK;
    }

/*******************************************************************************
*
* sdCmdSendCid - issue CMD10 to ask card to send its CID
*
* This routine issues CMD10 to ask card to send its CID.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdSendCid
    (
    VXB_DEVICE_ID pDev,
    void * pCid
    )
    {
    SD_CMD cmd;
    STATUS rc;
    SD_HARDWARE * pSdHardWare;

    if (pCid == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdSendCid() - pCid is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdSendCid() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    SD_CMD_FORM (cmd, SDMMC_CMD_SEND_CID, SDMMC_CMD_ARG_RCA (pSdHardWare->rcaValue),
                 SDMMC_CMD_RSP_R2, FALSE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);
    if (rc == ERROR)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdSendCid() - cmdIssue return error\n",
                    0, 0, 0, 0, 0 ,0);
        return (ERROR);
        }

    bcopy ((char *)cmd.cmdRsp, (char *)pCid, CID_SIZE);

    return OK;
    }

/*******************************************************************************
*
* sdCmdSwitchVoltage - issue CMD11 to set card's voltage to 1.8V
*
* This routine issue CMD11 to set card's voltage to 1.8V.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdSwitchVoltage
    (
    VXB_DEVICE_ID pDev
    )
    {
    SD_CMD cmd;

    SD_CMD_FORM (cmd, SD_CMD_SWITCH_VOLTAGE, 0, SDMMC_CMD_RSP_R1, FALSE);

    return (sdCmdIssue(pDev, &cmd));
    }

/*******************************************************************************
*
* sdCmdStopTransmission - issue CMD12 to force card to stop transmission
*
* This routine issues CMD12 to force card to stop transmission.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdStopTransmission
    (
    VXB_DEVICE_ID pDev
    )
    {
    SD_CMD cmd;

    SD_CMD_FORM (cmd, SDMMC_CMD_STOP_TRANSMISSION, 0, SDMMC_CMD_RSP_R1B, FALSE);

    return (sdCmdIssue(pDev, &cmd));
    }

/*******************************************************************************
*
* sdCmdSendStatus - issue CMD13 to ask card to send its status
*
* This routine issues CMD13 to ask card to send its status.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdSendStatus
    (
    VXB_DEVICE_ID pDev,
    UINT32 * stsData
    )
    {
    SD_CMD cmd;
    SD_HARDWARE * pSdHardWare;
    STATUS rc;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdSendStatus() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    SD_CMD_FORM (cmd, SDMMC_CMD_SEND_STATUS,
                 SDMMC_CMD_ARG_RCA (pSdHardWare->rcaValue),
                 SDMMC_CMD_RSP_R1, FALSE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);
    if (rc == ERROR)
        return (ERROR);

    *stsData = cmd.cmdRsp[0];
    return (OK);
    }

/*******************************************************************************
*
* sdCmdGoInactiveState - issue CMD15 to put card into the inactive state
*
* This routine issues CMD15 to put card into the inactive state.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdGoInactiveState
    (
    VXB_DEVICE_ID pDev
    )
    {
    SD_CMD cmd;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdGoInactiveState() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    SD_CMD_FORM (cmd, SDMMC_CMD_GO_INACTIVE_STATE,
                 SDMMC_CMD_ARG_RCA (pSdHardWare->rcaValue),
                 SDMMC_CMD_RSP_NONE, FALSE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    return (pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd));
    }

/*******************************************************************************
*
* sdCmdSetBlockLen - issue CMD16 to set block length for block commands
*
* This routine issues CMD16 to set block length for block commands.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdSetBlockLen
    (
    VXB_DEVICE_ID pDev,
    UINT32 blkLen
    )
    {
    SD_CMD cmd;

    /* block length is always 512 bytes */

    SD_CMD_FORM (cmd, SDMMC_CMD_SET_BLOCKLEN, blkLen,
                 SDMMC_CMD_RSP_R1, FALSE);

    return (sdCmdIssue(pDev, &cmd));
    }

/*******************************************************************************
*
* mmcCmdSetBlockNum - issue CMD16 to set block length for block commands
*
* This routine issues CMD16 to set block length for block commands.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS mmcCmdSetBlockNum
    (
    VXB_DEVICE_ID pDev,
    UINT32 blkNum
    )
    {
    SD_CMD cmd;
    UINT32 arg;

    if (blkNum >> 31)
        arg = blkNum;
    else
        arg = (blkNum & 0xFFFF);

    SD_CMD_FORM (cmd, MMC_CMD_SET_BLOCK_COUNT, arg,
                 SDMMC_CMD_RSP_R1, FALSE);

    return (sdCmdIssue(pDev, &cmd));
    }

/*******************************************************************************
*
* mmcCmdSeEraseGrpStart - issue CMD35 to set erase group start address
*
* This routine issues CMD35 to set erase group start address.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS mmcCmdSeEraseGrpStart
    (
    VXB_DEVICE_ID pDev,
    UINT32 startAddr
    )
    {
    SD_CMD cmd;
    UINT32 cmdIndex;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "mmcCmdSeEraseGrpStart() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }
    if (pSdHardWare->isMmc)
        cmdIndex = MMC_CMD_ERASE_GROUP_START;
    else
        cmdIndex = SD_CMD_ERASE_WR_BLK_START;

    SD_CMD_FORM (cmd, cmdIndex, startAddr,
                 SDMMC_CMD_RSP_R1, FALSE);

    return (sdCmdIssue(pDev, &cmd));
    }

/*******************************************************************************
*
* mmcCmdSeEraseGrpEnd - issue CMD36 to set erase group end address
*
* This routine issues CMD36 to set erase group end address.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS mmcCmdSeEraseGrpEnd
    (
    VXB_DEVICE_ID pDev,
    UINT32 endAddr
    )
    {
    SD_CMD cmd;
    UINT32 cmdIndex;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "mmcCmdSeEraseGrpEnd() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }
    if (pSdHardWare->isMmc)
        cmdIndex = MMC_CMD_ERASE_GROUP_END;
    else
        cmdIndex = SD_CMD_ERASE_WR_BLK_END;

    SD_CMD_FORM (cmd, cmdIndex, endAddr,
                 SDMMC_CMD_RSP_R1, FALSE);

    return (sdCmdIssue(pDev, &cmd));
    }

/*******************************************************************************
*
* mmcCmdSeEraseGrpStart - issue CMD35 to set erase group start address
*
* This routine issues CMD35 to set erase group start address.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS mmcCmdSetEraseGrp
    (
    VXB_DEVICE_ID pDev,
    UINT eraseArg
    )
    {
    SD_CMD cmd;
    UINT32 rspValue = 0;
    UINT8  cmdSts = 0;
    UINT32 timeOut = 0;
    STATUS rc = OK;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "mmcCmdSeEraseGrp() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    SD_CMD_FORM (cmd, SDMMC_CMD_ERASE, eraseArg, SDMMC_CMD_RSP_R1B, FALSE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);
    if (rc == ERROR)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "mmcCmdSeEraseGrp() - cmdIssue return error\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }
    do
        {

        /* Need not check return status */

        (void)sdCmdSendStatus (pDev, &rspValue);
        timeOut++;
        if (timeOut >= SDMMC_COMMAND_TIMEOUT)
            return (ERROR);
        }while((!(rspValue & CARD_STS_READY_FOR_DATA)) ||
               (CARD_STS_CUR_GET(rspValue) == CARD_STS_PRG));

    return (OK);
    }

/*******************************************************************************
*
* mmcSendHpi - send HPI command to device
*
* This routine sends HPI command to device.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS mmcSendHpi
    (
    VXB_DEVICE_ID pDev,
    UINT32 cmdIndex,
    UINT32 * pStatus
    )
    {
    STATUS rc = OK;
    SD_CMD sdCmd;
    UINT32 argValue = 0;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "mmcSendHpi() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;
    argValue = ((pSdHardWare->rcaValue << 16) | 0x01);

#ifdef DRV_ORIGIN  /* origin*/
    if (cmdIndex == SDMMC_CMD_STOP_TRANSMISSION)
        {
        SD_CMD_FORM (sdCmd, cmdIndex, 0, SDMMC_CMD_RSP_R1B, FALSE);
        }
    else if (cmdIndex == SDMMC_CMD_SEND_STATUS)
        {
        SD_CMD_FORM (sdCmd, cmdIndex, 0, SDMMC_CMD_RSP_R1, FALSE);
        }
#else  /* jc*/
    if (cmdIndex == SDMMC_CMD_STOP_TRANSMISSION)
        {
        SD_CMD_FORM (sdCmd, cmdIndex, argValue, SDMMC_CMD_RSP_R1B, FALSE);
        }
    else if (cmdIndex == SDMMC_CMD_SEND_STATUS)
        {
        SD_CMD_FORM (sdCmd, cmdIndex, argValue, SDMMC_CMD_RSP_R1, FALSE);
        }
#endif
	
    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &sdCmd);
    if (rc == ERROR)
        return (ERROR);

    if (pStatus != NULL)
        *pStatus = sdCmd.cmdRsp[0];
    return (OK);
    }

/*******************************************************************************
*
* sdCmdReadSingleBlock - issue CMD17 to read a single block
*
* This routine issues CMD17 to read a single block.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdReadSingleBlock
    (
    VXB_DEVICE_ID   pDev,
    void *          buf,
    UINT64          blkNo,
    BOOL            highCapa
    )
    {
    SD_CMD cmd;
    UINT32 arg;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdReadSingleBlock() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    if (highCapa)
        arg = blkNo;
    else
        arg = blkNo * pSdHardWare->blockSize;

    SD_LIB_DBG (SD_LIB_DBG_CMD,
                "sdCmdReadSingleBlock() - arg = %d\n",
                arg, 0, 0, 0, 0 ,0);

    SD_CMD_FORM (cmd, SDMMC_CMD_READ_SINGLE_BLOCK, arg, SDMMC_CMD_RSP_R1, TRUE);
    SD_DATA_FORM (cmd, buf, 1, pSdHardWare->blockSize, TRUE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;
    if (!pSdHardWare->isMmc)
        cmd.rspType |= SDMMC_CMD_AUTOCMD12;

    return (pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd));
    }

#undef SINGLE_BKL_RD_WR_MODE
/*#define SINGLE_BKL_RD_WR_MODE*/

#if 1  /* ref_uboot */
/*******************************************************************************
*
* sdCmdReadMultipleBlock - issue CMD18 to read multiple blocks
*
* This routine issues CMD18 to read multiple blocks.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdReadMultipleBlock
    (
    VXB_DEVICE_ID   pDev,
    void *          buf,
    UINT64          blkNo,
    UINT32          numBlks,
    BOOL            highCapa
    )
{
#ifdef SINGLE_BKL_RD_WR_MODE  /* multi single_block mode*/

	int i = 0, ret = 0;
	int idx = 0;
    UINT64 blkNo_in = 0;

	for (i=0; i<numBlks; i++)
	{
		blkNo_in = blkNo + i;
		idx = i * SDMMC_BLOCK_SIZE;
		ret = sdCmdReadSingleBlock(pDev, (void*)(&buf[idx]), blkNo_in, highCapa);
	}

#else  /* multi_block mode*/

	SD_CMD cmd;
    UINT32 arg;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdReadMultipleBlock() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    if (highCapa)
        arg = blkNo;
    else
        arg = blkNo * pSdHardWare->blockSize;

    SD_LIB_DBG (SD_LIB_DBG_CMD,
                "sdCmdReadMultipleBlock() - arg = %d\n",
                arg, 0, 0, 0, 0 ,0);

    SD_CMD_FORM (cmd, SDMMC_CMD_READ_MULTI_BLOCK, arg, SDMMC_CMD_RSP_R1, TRUE);
    SD_DATA_FORM (cmd, buf, numBlks, pSdHardWare->blockSize, TRUE);
	
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;
    if (!pSdHardWare->isMmc)
        cmd.rspType |= SDMMC_CMD_AUTOCMD12;
	
#if 0  /* origin*/
    return (pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd));

#else  /* jc for emmc-64g stable*/
	if (ERROR == (pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd)))
	{
		return ERROR;
	}

	 if (numBlks > 1) 
	 {
		 arg = 0;
		 SD_CMD_FORM (cmd, SDMMC_CMD_STOP_TRANSMISSION, arg, SDMMC_CMD_RSP_R1B, FALSE);
		 SD_DATA_FORM (cmd, buf, numBlks, pSdHardWare->blockSize, FALSE);
		 
		 pSdHardWare->dataErr = 0x0;
		 pSdHardWare->cmdErr = 0x0;
		 if (!pSdHardWare->isMmc)
			 cmd.rspType |= SDMMC_CMD_AUTOCMD12;
		 
		 return (pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd));
	 }

	 return OK;
#endif

#endif
}


/*******************************************************************************
*
* sdCmdReadBlock - issue CMD17 or CMD18 to read blocks
*
* This routine issues CMD17 or CMD18 to read blocks.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdReadBlock
    (
    VXB_DEVICE_ID   pDev,
    void *          buf,
    UINT64          blkNo,
    UINT32          numBlks,
    BOOL            highCapa
    )
{
#ifdef SINGLE_BKL_RD_WR_MODE  /* multi single_block mode*/
	
	int i = 0, ret = 0;
	int idx = 0;
	UINT64 blkNo_in = 0;

	for (i=0; i<numBlks; i++)
	{
		blkNo_in = blkNo + i;
		idx = i * SDMMC_BLOCK_SIZE;
		ret = sdCmdReadSingleBlock(pDev, (void*)(&buf[idx]), blkNo_in, highCapa);
	}
	
#else  /* multi_block mode*/

    SD_CMD cmd;
    UINT32 idx;
    UINT32 arg;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdReadBlock() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    if (numBlks > 1)
        idx = SDMMC_CMD_READ_MULTI_BLOCK;
    else
        idx = SDMMC_CMD_READ_SINGLE_BLOCK;

    if (highCapa)
        arg = blkNo;
    else
        arg = blkNo * pSdHardWare->blockSize;

    SD_LIB_DBG (SD_LIB_DBG_CMD,
                "sdCmdReadBlock() - arg = %d and idx = %d\n",
                arg, idx, 0, 0, 0 ,0);

    SD_CMD_FORM (cmd, idx, arg, SDMMC_CMD_RSP_R1, TRUE);
    SD_DATA_FORM (cmd, buf, numBlks, pSdHardWare->blockSize, TRUE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

#if 0 /* origin*/
    if (!pSdHardWare->isMmc)  /* sd*/
    {
        cmd.rspType |= SDMMC_CMD_AUTOCMD12;
    }
#else
    if (numBlks > 1) 
    {    
    	cmd.rspType |= SDMMC_CMD_AUTOCMD12;
    }
#endif

#if 1  /* origin*/
	return (pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd));

#else  /* jc for emmc-64g stable*/
	if (ERROR == (pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd)))
	{
		return ERROR;
	}

	 if (numBlks > 1) 
	 {
		 arg = 0;
		 SD_CMD_FORM (cmd, SDMMC_CMD_STOP_TRANSMISSION, arg, SDMMC_CMD_RSP_R1B, TRUE);
		 SD_DATA_FORM (cmd, buf, numBlks, pSdHardWare->blockSize, FALSE);
		 
		 pSdHardWare->dataErr = 0x0;
		 pSdHardWare->cmdErr = 0x0;
		 if (!pSdHardWare->isMmc)
			 cmd.rspType |= SDMMC_CMD_AUTOCMD12;
		 
		 return (pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd));
	 }

	 return OK;
#endif
#endif
}

#endif


/*******************************************************************************
*
* sdCmdWriteSingleBlock - issue CMD24 to write a single block
*
* This routine issues CMD24 to write a single block.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdWriteSingleBlock
    (
    VXB_DEVICE_ID   pDev,
    void *          buf,
    UINT64          blkNo,
    BOOL            isHighCapa
    )
    {
    SD_CMD cmd;
    UINT32 arg;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdWriteSingleBlock() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    if (isHighCapa)
        arg = blkNo;
    else
        arg = blkNo * pSdHardWare->blockSize;

    SD_LIB_DBG (SD_LIB_DBG_CMD,
                "sdCmdWriteSingleBlock() - arg = %d\n",
                arg, 0, 0, 0, 0 ,0);

    SD_CMD_FORM (cmd, SDMMC_CMD_WRITE_SINGLE_BLOCK, arg, SDMMC_CMD_RSP_R1, TRUE);
    SD_DATA_FORM (cmd, buf, 1, pSdHardWare->blockSize, FALSE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;
    if (!pSdHardWare->isMmc)
        cmd.rspType |= SDMMC_CMD_AUTOCMD12;

    return (pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd));
    }

/**/
/* jc, for mmc write stable*/
/**/
#if 1

#define MMC_STATUS_MASK		     (~0x0206BF7F)
#define MMC_STATUS_SWITCH_ERROR	 (1 << 7)
#define MMC_STATUS_RDY_FOR_DATA  (1 << 8)
#define MMC_STATUS_CURR_STATE	 (0xf << 9)
#define MMC_STATUS_ERROR	     (1 << 19)

#define MMC_STATE_PRG		     (7 << 9)

int mmc_send_status(VXB_DEVICE_ID pDev, int timeout)
{
	UINT32 stsData;
		
	int err, retries = 5;

	while (1)
	{
		if (ERROR == (err = sdCmdSendStatus(pDev, &stsData)))
		{
			return ERROR;
		}
		
		if (err != ERROR) 
		{
			if ((stsData & MMC_STATUS_RDY_FOR_DATA) && ((stsData & MMC_STATUS_CURR_STATE) != MMC_STATE_PRG))
			{	
				break;
			}

			if (stsData & MMC_STATUS_MASK) 
			{
				return ERROR;
			}
		} 
		else if (--retries < 0)
		{	
			return ERROR;
		}

		if (timeout-- <= 0)
		{	
			break;
		}

		/*udelay(1000);*/
		vxbMsDelay(1);
	}

	return OK;
}

/*******************************************************************************
*
* sdCmdWriteMultipleBlock - issue CMD25 to write multiple blocks
*
* This routine issues CMD25 to writ multiple blocks.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdWriteMultipleBlock
    (
    VXB_DEVICE_ID   pDev,
    void *          buf,
    UINT64          blkNo,
    UINT32          numBlks,
    BOOL            isHighCapa
    )
{

#ifdef SINGLE_BKL_RD_WR_MODE  /* multi single_block mode*/
	
	int i = 0, ret = 0;
	int idx = 0;
	UINT64 blkNo_in = 0;

	for (i=0; i<numBlks; i++)
	{
		blkNo_in = blkNo + i;
		idx = i * SDMMC_BLOCK_SIZE;
		ret = sdCmdWriteSingleBlock(pDev, (void*)(&buf[idx]), blkNo_in, isHighCapa);
	}
	
#else  /* multi_block mode*/

    SD_CMD cmd;
    UINT32 arg;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdWriteMultipleBlock() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }


    if (isHighCapa)
        arg = blkNo;
    else
        arg = blkNo * pSdHardWare->blockSize;

    SD_LIB_DBG (SD_LIB_DBG_CMD,
                "sdCmdWriteMultipleBlock() - arg = %d\n",
                arg, 0, 0, 0, 0 ,0);

    SD_CMD_FORM (cmd, SDMMC_CMD_WRITE_MULTI_BLOCK, arg, SDMMC_CMD_RSP_R1, TRUE);
    SD_DATA_FORM (cmd, buf, numBlks, pSdHardWare->blockSize, FALSE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;
	
    if (!pSdHardWare->isMmc)
        cmd.rspType |= SDMMC_CMD_AUTOCMD12;
	
#if 0  /* origin*/
    return (pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd));

#else /* jc, ref_uboot*/
	if (ERROR == pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd))
	{
		return ERROR;
	}
	
	if (ERROR == mmc_send_status(pDev, 1000))
	{
		return ERROR;
	}

	return OK;
#endif

#endif
}



/*******************************************************************************
*
* sdCmdWriteBlock - issue CMD24 or CMD25 to write blocks
*
* This routine issues CMD24 or CMD25 to write blocks.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdWriteBlock
    (
    VXB_DEVICE_ID   pDev,
    void *          buf,
    UINT64          blkNo,
    UINT32          numBlks,
    BOOL            isHighCapa
    )
{
#ifdef SINGLE_BKL_RD_WR_MODE  /* multi single_block mode*/
		
		int i = 0, ret = 0;
		int idx = 0;
		UINT64 blkNo_in = 0;
	
		for (i=0; i<numBlks; i++)
		{
			blkNo_in = blkNo + i;
			idx = i * SDMMC_BLOCK_SIZE;
			ret = sdCmdWriteSingleBlock(pDev, (void*)(&buf[idx]), blkNo_in, isHighCapa);
		}
		
#else  /* multi_block mode*/

    SD_CMD cmd;
    UINT32 idx;
    UINT32 arg;
    SD_HARDWARE * pSdHardWare;
	int ret = 0;  /* jc*/

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdWriteBlock() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    if (numBlks > 1)
        idx = SDMMC_CMD_WRITE_MULTI_BLOCK;
    else
        idx = SDMMC_CMD_WRITE_SINGLE_BLOCK;

    if (isHighCapa)
        arg = blkNo;
    else
        arg = blkNo * pSdHardWare->blockSize;

    SD_LIB_DBG (SD_LIB_DBG_CMD,
                "sdCmdWriteBlock() - arg = %d and idx = %d\n",
                arg, idx, 0, 0, 0 ,0);

    SD_CMD_FORM (cmd, idx, arg, SDMMC_CMD_RSP_R1, TRUE);
    SD_DATA_FORM (cmd, buf, numBlks, pSdHardWare->blockSize, FALSE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

#if 0  /* origin	*/
    if (!pSdHardWare->isMmc)
        cmd.rspType |= SDMMC_CMD_AUTOCMD12;
#else  /* jc*/
    if (numBlks > 1) 
    {    
    	cmd.rspType |= SDMMC_CMD_AUTOCMD12;
    }
#endif

#if 1  /* origin*/

	#if 1 /* origin*/
		return (pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd));  /* origin*/
	#else
		/* jc for stable*/
		ret = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);
		vxbMsDelay(10);
		return ret;
	#endif
	
#else /* jc, ref_uboot*/

	if (ERROR == pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd))
	{
		return ERROR;
	}
	
	if (ERROR == mmc_send_status(pDev, 1000))
	{
		return ERROR;
	}

	return OK;
#endif
#endif
}

#endif



/*******************************************************************************
*
* sdCmdAppCmd - issue CMD55 to indicate an application command followed
*
* This routine issues CMD55 to indicate an application command followed.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdAppCmd
    (
    VXB_DEVICE_ID  pDev
    )
    {
    SD_CMD cmd;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdCmdAppCmd() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    SD_CMD_FORM (cmd, SDMMC_CMD_APP_CMD, SDMMC_CMD_ARG_RCA (pSdHardWare->rcaValue),
                 SDMMC_CMD_RSP_R1, FALSE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    return (pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd));
    }

/*******************************************************************************
*
* sdACmdSetBusWidth - issue ACMD6 to set the bus width for data transfer
*
* This routine issues ACMD6 to set the bus width for data transfer.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdACmdSetBusWidth
    (
    VXB_DEVICE_ID   pDev,
    UINT32          width
    )
    {
    SD_CMD cmd;
    UINT32 arg;
    STATUS rc;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdACmdSetBusWidth() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    rc = sdCmdAppCmd (pDev);
    if (rc == ERROR)
        return ERROR;

    if (width == SDMMC_BUS_WIDTH_1BIT)
        arg = SD_ACMD6_ARG_1BIT_BUS;
    else
        arg = SD_ACMD6_ARG_4BIT_BUS;

    SD_CMD_FORM (cmd, SD_ACMD_SET_BUS_WIDTH, arg, SDMMC_CMD_RSP_R1, FALSE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    return (pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd));
    }

/*******************************************************************************
*
* sdACmdSendOpCond - issue ACMD41 to ask card to send its OCR
*
* This routine issues ACMD41 to ask card to send its OCR.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdACmdSendOpCond
    (
    VXB_DEVICE_ID  pDev,
    UINT32 capbility,
    UINT32 version
    )
    {
    SD_CMD cmd;
    UINT32 arg;
    STATUS rc;

    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdACmdSendOpCond() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    rc = sdCmdAppCmd (pDev);
    if (rc == ERROR)
        return (ERROR);

    arg = capbility & OCR_VDD_VOL_MASK;
    if (version == SD_VERSION_200)
        {
        if (capbility & OCR_CARD_CAP_STS)
            arg |= OCR_CARD_CAP_STS;
        }
  /*  arg |= OCR_CARD_CAP_STS;//syx*/
   /*
    * Set ACMD41 argument to 3.3V if both 3.0V & 3.3V are not reported
    * by host controller's capbilities register
    */

#ifdef DRV_ORIGIN  /* origin*/
    if (!(arg & (OCR_VDD_VOL_29_30 | OCR_VDD_VOL_30_31 |
                 OCR_VDD_VOL_32_33 | OCR_VDD_VOL_33_34)))
        arg |= (OCR_VDD_VOL_32_33 | OCR_VDD_VOL_33_34);
#else
	if (!(arg & (OCD_VDD_VOL_LOW | OCR_VDD_VOL_29_30 | OCR_VDD_VOL_30_31 |
				 OCR_VDD_VOL_32_33 | OCR_VDD_VOL_33_34)))
		arg |= (OCD_VDD_VOL_LOW | OCR_VDD_VOL_32_33 | OCR_VDD_VOL_33_34);
#endif
	
/*printf("arg = %x\n",arg);*/

    SD_CMD_FORM (cmd, SD_ACMD_SEND_OP_COND, arg, SDMMC_CMD_RSP_R3, FALSE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);

    if (rc == ERROR)
        return (ERROR);

    pSdHardWare->ocrValue = cmd.cmdRsp[0];
    return (OK);
    }

/*******************************************************************************
* 
* sdACmdClrCardDetect - issue ACMD42 to clrear card detect
*
* This routine issues ACMD42 to clrear card detect.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdACmdClrCardDetect
    (
    VXB_DEVICE_ID   pDev,
    UINT32          connect
    )
    {
    SD_CMD cmd;
    UINT32 arg;
    STATUS rc;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdACmdClrCardDetect() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }
        
    rc = sdCmdAppCmd (pDev);
    if (rc == ERROR)
        return ERROR;

    arg = connect;
	
#ifndef DRV_ORIGIN
	cmd.cmdIdx = 0;  /* jc*/
#endif

    SD_CMD_FORM (cmd, SD_ACMD_SET_CLR_CARD_DETECT, arg, SDMMC_CMD_RSP_R1, FALSE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    return (pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd));
    }
    
/*******************************************************************************
*
* sdACmdSendScr - issue ACMD51 to ask card to send its SCR
*
* This routine issues ACMD51 to ask card to send its SCR.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdACmdSendScr
    (
    VXB_DEVICE_ID  pDev,
    SDMMC_SCR * scrData
    )
    {
    SD_CMD cmd;
    SDMMC_SCR * scr;
    STATUS rc;
    SD_HARDWARE * pSdHardWare;

	/* jc*/
    STATUS rc;
    UINT32 i;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdACmdSendScr() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    scr = (SDMMC_SCR *)cacheDmaMalloc (sizeof(SDMMC_SCR));
    if (scr == NULL)
        return ERROR;
	
#ifdef DRV_ORIGIN  /* origin*/
    rc = sdCmdAppCmd (pDev);
    if (rc == ERROR)
        {
        (void)cacheDmaFree((void *)scr);
        return ERROR;
        }

    SD_CMD_FORM (cmd, SD_ACMD_SEND_SCR, 0, SDMMC_CMD_RSP_R1, TRUE);
    SD_DATA_FORM (cmd, scr, 1, sizeof(SDMMC_SCR), TRUE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);	
    if (rc == ERROR)
        {
        (void)cacheDmaFree((void *)scr);
        return ERROR;
        }

    bcopy ((char *)scr, (char *)(scrData), sizeof(SDMMC_SCR));
    (void)cacheDmaFree((void *)scr);
	
    return (OK);
	
#else  /* jc*/
    for (i = 0; i < 10; i++)
     {
        rc = sdCmdAppCmd (pDev);
        if (rc == ERROR)
        {
            vxbMsDelay(1);
            continue; 
        } 
        
        SD_CMD_FORM (cmd, SD_ACMD_SEND_SCR, 0, SDMMC_CMD_RSP_R1, TRUE);
        SD_DATA_FORM (cmd, scr, 1, sizeof(SDMMC_SCR), TRUE);
        pSdHardWare->dataErr = 0x0;
        pSdHardWare->cmdErr = 0x0;
        
		rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);	
        if (rc == ERROR)
        {
            vxbMsDelay(1);
            continue;
        }
        else
            break;
    }
        
    if (rc != ERROR)
    {    
        bcopy ((char *)scr, (char *)(scrData), sizeof(SDMMC_SCR));
        (void)cacheDmaFree((void *)scr);
        return (OK);
     }
    else
    {
        (void)cacheDmaFree((void *)scr);
        return (rc);
    }

#endif	
}

/*******************************************************************************
*
* sdACmdSendSsr - issue ACMD13 to ask card to send its SSR
*
* This routine issues ACMD13 to ask card to send its SSR.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdACmdSendSsr
    (
    VXB_DEVICE_ID  pDev,
    UINT8 * ssrData
    )
    {
    SD_CMD cmd;
    UINT8 * ssr;
    STATUS rc;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdACmdSendSsr() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    ssr = (UINT8 *)cacheDmaMalloc (sizeof(UINT8) * SD_SSR_SIZE);
    if (ssr == NULL)
        return ERROR;

    rc = sdCmdAppCmd (pDev);
    if (rc == ERROR)
        {
        (void)cacheDmaFree((void *)ssr);
        return ERROR;
        }

    SD_CMD_FORM (cmd, SD_ACMD_SD_STATUS, 0, SDMMC_CMD_RSP_R1, TRUE);
    SD_DATA_FORM (cmd, ssr, 1, sizeof(UINT8) * SD_SSR_SIZE, TRUE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);
    if (rc == ERROR)
        {
        (void)cacheDmaFree((void *)ssr);
        return ERROR;
        }

    bcopy ((char *)ssr, (char *)(ssrData), sizeof(UINT8) * SD_SSR_SIZE);
    (void)cacheDmaFree((void *)ssr);
    return (OK);
    }

/*******************************************************************************
*
* sdioCmdIoRwDirect - issue CMD52 to read/write sdio card's register.
*
* This routine issues CMD52 to read/write sdio card's register. CMD52 must be
* input register offset, function number, rwOps. rwOps parameter include two
* part, R/W flag and RAW flag. R/W flag is defined at bit31 and RAW flag is
* defined at bit27. These flags's define is same with CMD52 description.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdioCmdIoRwDirect
    (
    VXB_DEVICE_ID pDev,
    UINT32 regOffset,
    UINT32 funcNum,
    UINT32 rwOps,
    UINT8 * data
    )
    {
    SD_CMD cmd;
    STATUS rc;
    UINT32 arg = 0;
    UINT8  cmdSts = 0;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdioCmdIoRwDirect() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }
    arg |= rwOps;
    arg |= SDIO_DIRECT_IO_REGADRS(regOffset);
    arg |= SDIO_DIRECT_IO_FUNC(funcNum);
    arg |= *data;

    SD_CMD_FORM (cmd, SDIO_CMD_DIRECT_IO, arg, SDMMC_CMD_RSP_R5, FALSE);
    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);

    pSdHardWare->cmdErr = cmd.cmdErr;
    pSdHardWare->dataErr = cmd.dataErr;

    if (rc == ERROR)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdioCmdIoRwDirect() - cmdIssue return error\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }
    cmdSts = (cmd.cmdRsp[0] >> 8) & 0xFF;
    if ((cmdSts &
        (SDIO_DIRECTIO_OUTRANGE | SDIO_DIRECTIO_ERR | SDIO_DIRECTIO_FUNCERR))
        != 0x0)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdioCmdIoRwDirect() - cmdSts = 0X%x return error\n",
                    cmdSts, 0, 0, 0, 0 ,0);
        return ERROR;
        }
    if ((rwOps & (UINT32)SDIO_DIRECT_IO_RW) == 0x0)
        {
        *data = cmd.cmdRsp[0] & 0xFF;
        SD_LIB_DBG (SD_LIB_DBG_CMD,
                    "sdioCmdIoRwDirect() - REG DATA = 0X%x\n",
                    cmd.cmdRsp[0] & 0xFF, 0, 0, 0, 0 ,0);
        }
    return (OK);
    }

/*******************************************************************************
*
* sdioCmdIoRwExtend - issue CMD53 to read/write sdio card's data.
*
* This routine issues CMD53 to read/write sdio card's data. CMD53 must be
* input data offset, function number, rwOps. rwOps parameter include two
* part, R/W flag and RAW flag. R/W flag is defined at bit31 and RAW flag is
* defined at bit27. These flags's define is same with CMD53 description.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdioCmdIoRwExtend
    (
    VXB_DEVICE_ID pDev,
    UINT32 regOffset,
    UINT32 funcNum,
    UINT32 ops,
    SD_DATA * pSdData
    )
    {
    SD_CMD cmd;
    STATUS rc;
    UINT32 arg = 0;
    UINT8  cmdSts = 0;
    BOOL   isRead;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdioCmdIoRwExtend() - pSdHardWare is NULL\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }
    arg |= ops;
    arg |= SDIO_DIRECT_IO_REGADRS(regOffset);
    arg |= SDIO_DIRECT_IO_FUNC(funcNum);

    if ((ops & SDIO_DIRECT_IOEXT_BLK) == 0x0)
        arg |= (pSdData->blkSize == 512) ? 0 : pSdData->blkSize;    /* byte mode */
    else
        arg |= SDIO_DIRECT_IOEXT_BLK | pSdData->blkNum;     /* block mode */

    if ((ops & (UINT32)SDIO_DIRECT_IO_RW) == 0x0)
        isRead = TRUE;
    else
        isRead = FALSE;

    SD_CMD_FORM (cmd, SDIO_CMD_DIRECT_EXT, arg, SDMMC_CMD_RSP_R5, TRUE);
    SD_DATA_FORM (cmd, pSdData->buffer, pSdData->blkNum, pSdData->blkSize, isRead);

    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, &cmd);
    pSdHardWare->cmdErr = cmd.cmdErr;
    pSdHardWare->dataErr = cmd.dataErr;

    if (rc == ERROR)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdioCmdIoRwExtend() - cmdIssue return error\n",
                    0, 0, 0, 0, 0 ,0);
        return ERROR;
        }
    cmdSts = (cmd.cmdRsp[0] >> 8) & 0xFF;
    if ((cmdSts &
        (SDIO_DIRECTIO_OUTRANGE | SDIO_DIRECTIO_ERR | SDIO_DIRECTIO_FUNCERR))
        != 0x0)
        {
        SD_LIB_DBG (SD_LIB_DBG_ERR,
                    "sdioCmdIoRwExtend() - cmdSts = 0X%x return error\n",
                    cmdSts, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    return (OK);
    }

/*******************************************************************************
*
* sdCmdIssue - the feature to issue one new SD/SDIO command
*
* This routine issues one new command.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdCmdIssue
    (
    VXB_DEVICE_ID pDev,
    SD_CMD * pSdCmd
    )
    {
    SD_HARDWARE * pSdHardWare;
    STATUS rc;

    pSdHardWare = (SD_HARDWARE *)(pDev->pBusSpecificDevInfo);
    if (pSdHardWare == NULL)
        return ERROR;

    pSdHardWare->dataErr = 0x0;
    pSdHardWare->cmdErr = 0x0;

    rc = pSdHardWare->vxbSdCmdIssue(pSdHardWare->pHostDev, pSdCmd);
    return (rc);
    }

/*******************************************************************************
*
* vxbSdioInt - interrupt handler for SDIO target interrupt.
*
* This routine executes multiple interrupt handlers for a SDIO target interrupt.
* Each interrupt handler must check the device dependent interrupt status bit
* to determine the source of the interrupt, since it simply execute all
* interrupt handlers in the link list.
*
* This is not a user callable routine
*
* RETURNS: OK, or ERROR
*
* ERRNO
*
*/

STATUS vxbSdioInt
    (
    struct vxbSdioIntHandlerInfo *pIntHandlerInfo
    )
    {
    SDIO_INT_RTN * pRtn;
    struct vxbSdioInt *pIntInfo;
    int irq;
    SDIO_INT_RTN *dllNext,*dllFirst;

    pIntInfo = pIntHandlerInfo->pIntInfo;
    irq = pIntHandlerInfo->irq;

    if (pIntInfo == NULL)
        return (ERROR);

    SPIN_LOCK_ISR_TAKE (&pIntInfo->sdioIntLibSpinlockIsr);
    dllFirst = (SDIO_INT_RTN *)DLL_FIRST (&pIntInfo->sdioIntList[irq]);
    SPIN_LOCK_ISR_GIVE (&pIntInfo->sdioIntLibSpinlockIsr);

    for (pRtn = dllFirst; pRtn != NULL; pRtn = dllNext)
        {
        (* pRtn->routine) (pRtn->parameter);
        SPIN_LOCK_ISR_TAKE (&pIntInfo->sdioIntLibSpinlockIsr);
        dllNext = (SDIO_INT_RTN *)DLL_NEXT (&pRtn->node);
        SPIN_LOCK_ISR_GIVE (&pIntInfo->sdioIntLibSpinlockIsr);
        }
    return (OK);
    }

/*******************************************************************************
*
* vxbSdioIntConnect - connect the interrupt handler to the SDIO interrupt.
*
* This routine connects an interrupt handler to a shared SDIO interrupt vector.
* A link list is created for each shared interrupt used in the system.  It
* is created when the first interrupt handler is attached to the vector.
* Subsequent calls to vxbPciIntConnect just add their routines to the
* linked list for that vector.
*
* RETURNS:
* OK, or ERROR if the interrupt handler cannot be built.
*
* ERRNO
*
*/

STATUS vxbSdioIntConnect
    (
    VXB_DEVICE_ID pDev,
    VOIDFUNCPTR *vector,        /* interrupt vector to attach to     */
    VOIDFUNCPTR routine,        /* routine to be called              */
    _Vx_usr_arg_t parameter     /* parameter to be passed to routine */
    )
    {
    int irq = IVEC_TO_INUM(vector);
    SDIO_INT_RTN *pRtn;
    STATUS retStatus;
    struct vxbSdioIntHandlerInfo * pSdioIntHandlerInfo;
    struct vxbSdioInt * pIntInfo;
    FUNCPTR method;
    VXB_DEVICE_ID busCtrlID;
    BOOL dllEmpty;

    if(pDev == NULL)
        return (ERROR);

    busCtrlID = vxbDevParent(pDev);
    if (busCtrlID == NULL)
        return (ERROR);

    method = vxbDevMethodGet(busCtrlID ,SD_CONTROLLER_METHOD_INTERRUPT_INFO);

    if(method == NULL)
        {
        SD_LIB_DBG(SD_LIB_DBG_ERR,
                   "vxbSdioIntConnect Failed: No SDIO method for IntInfo\n",
                   0, 0, 0, 0, 0, 0);
        return(ERROR);
        }

    retStatus = (*method)(busCtrlID,&pIntInfo);

    if(retStatus == ERROR)
        {
        SD_LIB_DBG(SD_LIB_DBG_ERR,
                   "vxbSdioIntConnect Failed: Method for IntInfo doesn't work\n",
                   0, 0, 0, 0, 0, 0);
        return(ERROR);
        }

    if(pIntInfo==NULL)
        {
        SD_LIB_DBG(SD_LIB_DBG_ERR,
                   "vxbSdioIntConnect Failed: pIntInfo is NULL\n",
                   0, 0, 0, 0, 0, 0);
        return(ERROR);
        }

    pSdioIntHandlerInfo = (struct vxbSdioIntHandlerInfo *)
                          malloc(sizeof(struct vxbSdioIntHandlerInfo));

    if (pSdioIntHandlerInfo == NULL)
        {
        SD_LIB_DBG(SD_LIB_DBG_ERR,
                   "vxbSdioIntConnect Failed: malloc pSdioIntHandlerInfo fault\n",
                   0, 0, 0, 0, 0, 0);
        return(ERROR);
        }

    pSdioIntHandlerInfo->pIntInfo= pIntInfo;
    pSdioIntHandlerInfo->irq = irq;

    if (pIntInfo->sdioIntLibInitStatus != OK)
        {
        SD_LIB_DBG(SD_LIB_DBG_ERR,
                   "vxbSdioIntConnect Failed: pciIntLibInitStatus\n",
                   0, 0, 0, 0, 0, 0);
        if (pSdioIntHandlerInfo != NULL)
            free (pSdioIntHandlerInfo);
        return ERROR;
        }

    if ((pRtn = (SDIO_INT_RTN *)malloc (sizeof (SDIO_INT_RTN))) == NULL)
        {
        SD_LIB_DBG(SD_LIB_DBG_ERR,
                   "vxbSdioIntConnect Failed:  hwMemAlloc is NULL\n",
                   0, 0, 0, 0, 0, 0);
        if (pSdioIntHandlerInfo != NULL)
            free (pSdioIntHandlerInfo);
        return ERROR;
        }

    SPIN_LOCK_ISR_TAKE (&pIntInfo->sdioIntLibSpinlockIsr);
    dllEmpty = DLL_EMPTY(&pIntInfo->sdioIntList[irq]);
    SPIN_LOCK_ISR_GIVE (&pIntInfo->sdioIntLibSpinlockIsr);

    if (dllEmpty)
        {
        retStatus = intConnect(vector, (VOIDFUNCPTR)vxbSdioInt,
                              (_Vx_usr_arg_t)pSdioIntHandlerInfo);
        if (retStatus == ERROR)
            {
            free((char*)pRtn);
            free (pSdioIntHandlerInfo);
            return ERROR;
            }
        }
    pRtn->routine   = routine;
    pRtn->parameter = parameter;
    pRtn->pSdioIntHandlerInfo = pSdioIntHandlerInfo;

    SPIN_LOCK_ISR_TAKE (&pIntInfo->sdioIntLibSpinlockIsr);
    dllAdd (&pIntInfo->sdioIntList[irq], &pRtn->node);
    SPIN_LOCK_ISR_GIVE (&pIntInfo->sdioIntLibSpinlockIsr);

    retStatus = ((DLL_EMPTY(&pIntInfo->sdioIntList[irq])) ? ERROR : OK );
    if (retStatus == ERROR)
        {
        SD_LIB_DBG(SD_LIB_DBG_ERR,
                   "vxbSdioIntConnect Failed: DLL EMPTY\n",
                   0, 0, 0, 0, 0, 0);
        free((char*)pRtn);
        free (pSdioIntHandlerInfo);
        }
    return retStatus;
    }

/*******************************************************************************
*
* vxbSdioIntLibInit - initialize the vxbSdioIntLib module
*
* This routine initializes the linked lists used to chain together the SDIO
* interrupt service routines.
*
* RETURNS: OK, or ERROR upon link list failures.
*
* ERRNO
*
*/

STATUS vxbSdioIntLibInit
    (
    struct vxbSdioInt * pIntInfo
    )
    {
    int ix;

    if (pIntInfo == NULL)
        return(ERROR);

    /* Initialize shared interrupt handler chains */

    pIntInfo->sdioIntList = (DL_LIST * )hwMemAlloc(sizeof(DL_LIST) * MAX_TARGET_DEV);


    for (ix = 0; ix < MAX_TARGET_DEV; ix++)
        (void)dllInit (&pIntInfo->sdioIntList[ix]);

    SPIN_LOCK_ISR_INIT (&pIntInfo->sdioIntLibSpinlockIsr, 0);

    return (pIntInfo->sdioIntLibInitStatus = OK);
    }

/*******************************************************************************
*
* vxbSdioReadCis - get all of the CIS value from one SDIO target device
*
* This routine get all of the CIS value from one SDIO target device
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS vxbSdioReadCis
    (
    VXB_DEVICE_ID pDev
    )
    {
    STATUS ret;
    SDIO_FUNC_TUPLE * pCurr;
    SDIO_FUNC_TUPLE * pNext;
    UINT8 * pData;
    SD_HARDWARE * pSdHardWare;
    UINT8 cisData = 0;
    UINT32 i;
    UINT32 ptr = 0;
    UINT8 tplCode = 0;
    UINT8  tplLink = 0;

    pSdHardWare = (SD_HARDWARE *)pDev->pBusSpecificDevInfo;
    if (pSdHardWare == NULL)
        return ERROR;

    for (i = 0; i < 3; i++)
        {
        ret = sdioCmdIoRwDirect(pDev,
                                (UINT32)(SDIO_FBR_BASE(pSdHardWare->funcNum) +
                                SDIO_FBR_CIS + i),
                                0, 0, &cisData);
        if (ret == ERROR)
            return (ERROR);

        ptr |= cisData << (i * 8);
        }

    do {
        ret = sdioCmdIoRwDirect(pDev, ptr++, 0, 0, &tplCode);
        if (ret == ERROR)
            break;

        /* size of 0xFF means we're done */

        if (tplCode == 0xFF)
            break;

        /* 0x00 means no link field or data */

        if (tplCode == 0x00)
            continue;

        ret = sdioCmdIoRwDirect(pDev, ptr++, 0, 0, &tplLink);
        if (ret == ERROR)
            break;

        /* size of 0xFF means we're done */

        if (tplLink == 0xFF)
            break;

        pNext = (SDIO_FUNC_TUPLE *)malloc(sizeof(SDIO_FUNC_TUPLE));
        if (pNext == NULL)
            return ERROR;

        pCurr = pNext;

        if (pSdHardWare->pSdioFuncTuple == NULL)
            {
            pSdHardWare->pSdioFuncTuple = pNext;
            pCurr = pNext;
            }
        else if (pCurr != NULL)
            pCurr->next = pNext;

        pSdHardWare->pSdioFuncTuple = pCurr;

        if (tplLink != 0x0)
            {
            pSdHardWare->pSdioFuncTuple->pData = (UINT8 *)malloc(sizeof(UINT8) * tplLink);
            if (pSdHardWare->pSdioFuncTuple->pData == NULL)
                {
                free (pSdHardWare->pSdioFuncTuple);
                return ERROR;
                }
            }

        for (i = 0; i < tplLink; i++)
            {
            ret = sdioCmdIoRwDirect(pDev, ptr + i, 0, 0, (pCurr->pData + i));
            if (ret == ERROR)
                break;
            }
        if (ret == ERROR)
            {
            free(pCurr->pData);
            free(pCurr);
            break;
            }
        if (tplCode == SDIO_CISTPL_MANFID)
            {
            UINT16 * pData;
            pData = (UINT16 *)pCurr->pData;
            pSdHardWare->vxbSdioId.sdioDevId = *(pData);
            pData++;
            pSdHardWare->vxbSdioId.sdioVenderId = *(pData);
            }
        ptr += tplLink;
        } while (ret != ERROR);

    return ret;
    }

/*******************************************************************************
*
* vxbSdioSetFunc - set one SDIO function enable/disable
*
* This routine set one SDIO function enable/disable
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS vxbSdioSetFunc
    (
    VXB_DEVICE_ID pDev,
    UINT8 func,
    BOOL  isEn
    )
    {
    STATUS ret;
    UINT8 ioRegData = 0;
    UINT32 timeOut = 0;
    SD_HARDWARE * pSdHardWare;

    pSdHardWare = (SD_HARDWARE *)pDev->pBusSpecificDevInfo;
    if (pSdHardWare == NULL)
        return ERROR;

    ret = sdioCmdIoRwDirect(pDev, SDIO_IOEN_CCCR, 0, 0, &ioRegData);
    if (ret == ERROR)
        return (ERROR);
    ioRegData |= (1 << func);

    ret = sdioCmdIoRwDirect(pDev, SDIO_IOEN_CCCR, 0, (UINT32)SDIO_DIRECT_IO_RW, &ioRegData);
    if (ret == ERROR)
        return (ERROR);

    do
        {
        ret = sdioCmdIoRwDirect(pDev, SDIO_IOEN_CCCR, 0, 0, &ioRegData);
        if (ret == ERROR)
            return (ERROR);
        timeOut++;
        }while (((ioRegData & (1 << func)) == 0x0) && (timeOut > SDIO_OPREG_TIMEOUT));
    if (timeOut >= SDIO_OPREG_TIMEOUT)
        return (ERROR);
    return (OK);
    }

