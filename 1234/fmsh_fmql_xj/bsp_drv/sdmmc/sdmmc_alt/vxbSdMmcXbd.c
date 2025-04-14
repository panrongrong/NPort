/* vxbSdMmcXbd.c - Generic SD/MMC XBD block driver */

/*
 * Copyright (c) 2009, 2010, 2012, 2014-2016 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
-------------------- 
01h,18aug16,zly  add support for direct bio for SD card (VXW6-85000)
01g,18may16,zly  fix sdmmc service task priority inversion issue.(VXW6-85551) 
01f,23sep15,zly  remove taskCpuLock() in sdMmcXbdDevDelete() since taskDelete
                 will take long time that will affect other task schedule
                 (VXW6-84031)
01e,28may15,zly  change ERF_ASYNC_PROC to ERF_SYNC_PROC in sdMmcXbdDevDelete
                 for avoiding memory leak. (VXW6-84176)
01d,28jul14,zly  errno did not pass up to upper layer.(VXW6-83138)
01c,17dec12,e_d  add taskSafe to protect bio server task. (WIND00390089)
01b,27jul10,rlg  fix compiler warnings - iter19 requirement
01a,18aug09,b_m  written.
*/

/*
DESCRIPTION

This is the generic XBD block driver used by SD/MMC library to interface
with upper level file system.
*/

/* includes */

#include <vxWorks.h>
#include <stdio.h>
#include <string.h>
#include <sysLib.h>
#include <ioLib.h>
#include <semLib.h>
#include <taskLib.h>
#include <vxBusLib.h>
#include <drv/xbd/xbd.h>
#include <drv/erf/erfLib.h>
#include <../src/hwif/h/storage/vxbSdMmcLib.h>
#include <../src/hwif/h/storage/vxbSdMmcXbd.h>

/* defines */
#define SDMMC_XBD_DBG_ON

#ifdef  SDMMC_XBD_DBG_ON

#ifdef  LOCAL
#undef  LOCAL
#define LOCAL
#endif

#define SDMMC_XBD_DBG_RW            0x00000001
#define SDMMC_XBD_DBG_IOCTL         0x00000002
#define SDMMC_XBD_DBG_ALL           0xffffffff
#define SDMMC_XBD_DBG_OFF           0x00000000

UINT32 sdMmcXbdDbgMask = SDMMC_XBD_DBG_OFF;

IMPORT FUNCPTR _func_logMsg;

#define SDMMC_XBD_DBG(mask, string, a, b, c, d, e, f)               \
    if ((sdMmcXbdDbgMask & mask) || (mask == SDMMC_XBD_DBG_ALL))    \
        _func_logMsg(string, a, b, c, d, e, f)

#else
#define SDMMC_XBD_DBG(mask, string, a, b, c, d, e, f)
#endif  /* SDMMC_XBD_DBG_ON */

/* forward declarations */

LOCAL int sdMmcXbdIoctl (struct xbd *, int, void *);
LOCAL int sdMmcXbdStrategy (struct xbd *, struct bio *);
LOCAL int sdMmcXbdDump (struct xbd *, sector_t, void *, size_t);
LOCAL int sdMmcXbdGetBioPrio(SDMMC_XBD_DEV *xbd); /*alan*/
/* locals */

LOCAL struct xbd_funcs sdMmcXbdFuncs =
    {
    sdMmcXbdIoctl,
    sdMmcXbdStrategy,
    sdMmcXbdDump,
    };

LOCAL int sdMmcServTskPri = SDMMC_XBD_SVC_TASK_PRI;

/*******************************************************************************
*
* sdMmcXbdGetNextBio - get the next bio in the bio chain
*
* This routine gets the next bio in the bio chain.
*
* RETURNS: pointer to the next bio or NULL
*
* ERRNO: N/A
*/

LOCAL struct bio * sdMmcXbdGetNextBio
    (
    SDMMC_XBD_DEV * xbd
    )
    {
    struct bio * pBio;
    STATUS rc;

    pBio = xbd->bioHead;

    if (pBio != NULL)
        {
        xbd->bioHead = pBio->bio_chain;
        pBio->bio_chain = NULL;

        if (xbd->bioHead == NULL)
            xbd->bioTail = NULL;
        }

    return (pBio);
    }

/*******************************************************************************
*
* sdMmcXbdExecBio - execute a bio
*
* This routine excutes a bio.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void sdMmcXbdExecBio
    (
    SDMMC_XBD_DEV * xbd,
    struct bio *    bio
    )
    {
    unsigned blkSize;
    sector_t blkNum;
    unsigned nblocks;
    unsigned size;
    STATUS rc = ERROR;

    blkSize = xbd->xbd.xbd_blocksize;
    blkNum = xbd->xbd.xbd_nblocks;

    /* check that all of this transaction fits in the disk */

    size = bio->bio_bcount;
    nblocks = size / blkSize;

    SDMMC_XBD_DBG (SDMMC_XBD_DBG_RW, "blkNo (%d) numBlks (%d) buf (0x%08x)\n",
                   (int)bio->bio_blkno, (int)nblocks, (int)bio->bio_data,
                   0, 0, 0);

    /* if our starting block number is bad, done with error */

    if (bio->bio_blkno >= blkNum)
        {
        bio->bio_bcount = 0;
        bio_done (bio, ENOSPC);
        return;
        }

    /* if we overrun the end of the disk, truncate the block number */

    if (bio->bio_blkno + nblocks > blkNum)
        nblocks = blkNum - bio->bio_blkno;

    /* calculate the real size */

    size = nblocks * blkSize;
    bio->bio_bcount = size;

    /* if we have less than 1 block, done it */

    if (nblocks == 0)
        {
        bio->bio_bcount = 0;
        bio_done (bio, 0);
        return;
        }

    if (bio->bio_flags & BIO_READ)
        {
        rc = xbd->blkRd (xbd->card, bio->bio_blkno, nblocks, bio->bio_data);
        }
    else if (bio->bio_flags & BIO_WRITE)
        {
        rc = xbd->blkWt (xbd->card, bio->bio_blkno, nblocks, bio->bio_data);
        }

    if (rc == OK)
        {
        bio_done (bio, 0);
        }
    else
        {
        bio->bio_bcount = 0;
        if (errno == S_ioLib_DISK_NOT_PRESENT)
            bio_done (bio, ENXIO);
        else
            bio_done (bio, EIO);
        }
    }

/*******************************************************************************
*
* sdMmcXbdBioSvcTask - SD/MMC XBD bio handle task
*
* This routine is the task loop to handle bio operations.
*
* RETURN: N/A
*
* ERRNO: N/A
*/

LOCAL void sdMmcXbdBioSvcTask
    (
    SDMMC_XBD_DEV * xbd
    )
    {
    struct bio * bio;
    STATUS rc;
    int highestPrio;

    FOREVER
        {
        /* wait semaphore from strategy routine */

        rc = semTake (xbd->bioReady, WAIT_FOREVER);
        if (rc != OK)
            {
            /* return from semTake is not needed */
            (void)semTake (xbd->bioMutex, WAIT_FOREVER);

            while ((bio = sdMmcXbdGetNextBio (xbd)) != NULL)
                bio_done (bio, ENXIO);

            /* return from semGive is not needed */
            (void)semGive (xbd->bioMutex);

            printf ("Take bioReady issue, return xbd server task now \n");
            return;
            }

        /* process bio queue */

        while (1)
            {

            /* return from semTake is not needed */

            (void)semTake (xbd->bioMutex, WAIT_FOREVER);

            highestPrio = sdMmcXbdGetBioPrio (xbd);

            /* if service task priority is high */

            if ( sdMmcServTskPri < highestPrio)
                {
                sdMmcServTskPri = highestPrio;

                /* lower service task prio based on left BIOs */
                /* return from taskPrioritySet is not needed */

                (void)taskPrioritySet(0, highestPrio);
                }

            bio = sdMmcXbdGetNextBio (xbd);

            /* return from semGive is not needed */
            (void)semGive (xbd->bioMutex);

            if (bio == NULL)
                break;

            sdMmcXbdExecBio (xbd, bio);
            }      
        }
    }

/*******************************************************************************
*
* sdMmcXbdCreateSyncHandler - XBD creation handler
*
* This routine is the XBD creation handler.
*
* RETURN: N/A
*
* ERRNO: N/A
*/

LOCAL void sdMmcXbdCreateSyncHandler
    (
    UINT16  category,
    UINT16  type,
    void *  eventData,
    void *  userData
    )
    {
    SDMMC_XBD_DEV * xbd = (SDMMC_XBD_DEV *)userData;

    if (xbd == NULL)
        return;

    if (!((category == xbdEventCategory) && (type == xbdEventInstantiated)))
        return;

    erfHandlerUnregister (xbdEventCategory, xbdEventInstantiated,
                          sdMmcXbdCreateSyncHandler, userData);

    semGive (xbd->xbdSemId);
    }

/*******************************************************************************
*
* sdMmcXbdDevCreate - create an XBD device for SD/MMC
*
* This routine creates an XBD device for SD/MMC.
*
* RETURN: XBD device id
*
* ERRNO: N/A
*/

device_t sdMmcXbdDevCreate
    (
    void *  sdMmc,
    char *  devName
    )
    {
    SDMMC_CARD * card = (SDMMC_CARD *)sdMmc;
    SDMMC_XBD_DEV * xbd;
    device_t dev;
    STATUS rc;

    /* sanity test */

    if (!card->attached)
        return ((device_t)NULL);

    xbd = &card->xbdDev;

    /* install default block access routines */

    if (xbd->blkRd == NULL)
        xbd->blkRd = (FUNCPTR)sdMmcBlkRead;
    if (xbd->blkWt == NULL)
        xbd->blkWt = (FUNCPTR)sdMmcBlkWrite;

    xbd->bioMutex = semMCreate (SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
    if (xbd->bioMutex == NULL)
        goto error;

    xbd->bioReady = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
    if (xbd->bioReady == NULL)
        goto error;

    xbd->xbdSemId = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
    if (xbd->xbdSemId == NULL)
        goto error;

    if (xbd->directModeFlag == FALSE)
        {
        /* spawn service task */

        xbd->svcTaskId = taskSpawn (SDMMC_XBD_SVC_TASK_NAME, SDMMC_XBD_SVC_TASK_PRI,
                                    0, SDMMC_XBD_SVC_TASK_STACK,
                                    (FUNCPTR)sdMmcXbdBioSvcTask, (int)xbd,
                                    0, 0, 0, 0, 0, 0, 0, 0, 0);
        if (xbd->svcTaskId == ERROR)
            goto error;
        }

    xbd->xbdInserted = 1;

    /* register new event handler for base device synchronization */

    rc = erfHandlerRegister (xbdEventCategory, xbdEventInstantiated,
                             sdMmcXbdCreateSyncHandler, xbd, 0);
    if (rc != OK)
        goto error;

    /* apply one card index to system */

    rc = sdMmcCardIdxAlloc (card);
    if (rc != OK)
        goto error;

    /* attach XBD */

    if ((devName == NULL) || (strlen (devName) > (MAX_DEVNAME - 1)))
        {
        if (card->isMmc)
            sprintf ((char *)xbd->devName, "/mmc%d", card->idx);
        else
            sprintf ((char *)xbd->devName, "/sd%d", card->idx);
        }
    else
        {
        strcpy ((char *)xbd->devName, devName);
        }

    rc = xbdAttach (&xbd->xbd, &sdMmcXbdFuncs, xbd->devName, SDMMC_BLOCK_SIZE,
                    card->info.capacity / SDMMC_BLOCK_SIZE, &dev);
    if (rc != OK)
        goto error;

    /* raise an insert event */

    rc = erfEventRaise (xbdEventCategory, xbdEventPrimaryInsert, ERF_ASYNC_PROC,
                        (void *)dev, NULL);
    if (rc != OK)
        goto error;

    /* wait for FSM to send xbdEventInstantiated event */

    rc = semTake (xbd->xbdSemId, WAIT_FOREVER);
    if (rc != OK)
        goto error;

    return (dev);

error:

    xbd->xbdInserted = 0;
    if (xbd->bioMutex != NULL)
        (void)semDelete (xbd->bioMutex);
    if (xbd->bioReady != NULL)
        (void)semDelete (xbd->bioReady);
    if (xbd->xbdSemId != NULL)
        (void)semDelete (xbd->xbdSemId);
    if (xbd->svcTaskId != ERROR)
        {
        rc = taskDelete (xbd->svcTaskId);
        if (rc == ERROR)
            {
            SDMMC_XBD_DBG (SDMMC_XBD_DBG_RW,
                          "[sdMmcXbdDevCreate]: taskDelete svcTaskId error\n",
                          0, 0, 0, 0, 0, 0);
            }
        }
    return ((device_t)NULL);
    }

/*******************************************************************************
*
* sdMmcXbdDevDelete - delete an XBD device for SD/MMC
*
* This routine deletes an XBD device for SD/MMC.
*
* RETURN: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcXbdDevDelete
    (
    void *  sdMmc
    )
    {
    SDMMC_CARD * card = (SDMMC_CARD *)sdMmc;
    SDMMC_XBD_DEV * xbd;
    struct bio * bio;
    STATUS rc;

    xbd = &card->xbdDev;

    xbd->xbdInserted = 0;

    /* wait till the service task is pending before deleting */

    while (xbd->svcTaskId != 0)
        {
        if (taskIsPended (xbd->svcTaskId))
            {
            rc = taskDelete (xbd->svcTaskId);
            if (rc == ERROR)
                {
                taskDelay(1);
                continue;
                }
            xbd->svcTaskId = 0;
            }

        taskDelay (1);
        }

    /* raise a removal event */

    rc = erfEventRaise (xbdEventCategory, xbdEventRemove, ERF_SYNC_PROC,
                        (void *)xbd->xbd.xbd_dev.dv_dev, NULL);
    if (rc != OK)
        goto error;

    /* detach XBD */

    xbdDetach (&xbd->xbd);
    xbd->xbdInstantiated = FALSE;

    devUnmap ((struct device *)&xbd->xbd);

    /* done any pending bio */

    while ((bio = sdMmcXbdGetNextBio (xbd)) != NULL)
        bio_done (bio, ENXIO);

    /* delete semaphores */

    (void)semDelete (xbd->bioMutex);
    if (xbd->bioReady != NULL)
        (void)semDelete (xbd->bioReady);
    if (xbd->xbdSemId != NULL)
        (void)semDelete (xbd->xbdSemId);

    /* return the card index to system */

    sdMmcCardIdxFree (card);

    return (OK);

error:

    if (xbd->bioMutex != NULL)
        (void)semDelete (xbd->bioMutex);
    if (xbd->bioReady != NULL)
        (void)semDelete (xbd->bioReady);
    if (xbd->xbdSemId != NULL)
        (void)semDelete (xbd->xbdSemId);
    if (xbd->svcTaskId != ERROR)
        {
        rc = taskDelete (xbd->svcTaskId);
        if (rc == ERROR)
            {
            SDMMC_XBD_DBG (SDMMC_XBD_DBG_RW,
                          "[sdMmcXbdDevCreate]: taskDelete svcTaskId error\n",
                          0, 0, 0, 0, 0, 0);
            }
        }

    return (ERROR);
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
*/

LOCAL int sdMmcXbdIoctl
    (
    struct xbd *    self,
    int             cmd,
    void *          arg
    )
    {
    SDMMC_XBD_DEV * xbd;
    SDMMC_CARD * card;
    int rc = OK;

    if (self == NULL)
        return (ERROR);

    xbd = (SDMMC_XBD_DEV *)self;
    card = (SDMMC_CARD *)xbd->card;

    switch (cmd)
        {
        case XBD_GETGEOMETRY:
            {
            XBD_GEOMETRY * geo;

            SDMMC_XBD_DBG (SDMMC_XBD_DBG_IOCTL,
                           "[xbdIoctl]: xbd (0x%08x) XBD_GETGEOMETRY\n",
                           (int)xbd, 0, 0, 0, 0, 0);

            geo = (XBD_GEOMETRY *)arg;

            geo->total_blocks = card->info.capacity / SDMMC_BLOCK_SIZE;
            geo->blocksize = SDMMC_BLOCK_SIZE;

            /* setup dummy CHS information */

            geo->heads = 1;
            geo->secs_per_track = 1;
            geo->cylinders = (geo->total_blocks / 1);

            rc = OK;
            break;
            }

        case XBD_SYNC:
            {
            SDMMC_XBD_DBG (SDMMC_XBD_DBG_IOCTL,
                           "[xbdIoctl]: xbd (0x%08x) XBD_SYNC\n",
                           (int)xbd, 0, 0, 0, 0, 0);

            rc = OK;
            break;
            }

        case XBD_SOFT_EJECT:
        case XBD_HARD_EJECT:
            {
            UINT16 reType;

            SDMMC_XBD_DBG (SDMMC_XBD_DBG_IOCTL,
                           "[xbdIoctl]: xbd (0x%08x) %s\n",
                           (int)xbd,
                           (int)((cmd == XBD_HARD_EJECT) ?
                           "XBD_HARD_EJECT" : "SOFT_HARD_EJECT"),
                           0, 0, 0, 0);

            rc = erfEventRaise (xbdEventCategory, xbdEventRemove, ERF_ASYNC_PROC,
                               (void *)xbd->xbd.xbd_dev.dv_dev, NULL);

            reType = (cmd == (int)XBD_HARD_EJECT) ?
                      xbdEventPrimaryInsert : xbdEventSoftInsert;

            rc = erfEventRaise (xbdEventCategory, reType, ERF_ASYNC_PROC,
                               (void *)xbd->xbd.xbd_dev.dv_dev, NULL);

            break;
            }

        case XBD_GETBASENAME:
            {
            SDMMC_XBD_DBG (SDMMC_XBD_DBG_IOCTL,
                           "[xbdIoctl]: xbd (0x%08x) XBD_GETBASENAME\n",
                           (int)xbd, 0, 0, 0, 0, 0);

            rc = devName (xbd->xbd.xbd_dev.dv_dev, arg);

            break;
            }

        case XBD_TEST:
            {
            SDMMC_XBD_DBG (SDMMC_XBD_DBG_IOCTL,
                           "[xbdIoctl]: xbd (0x%08x) XBD_TEST\n",
                           (int)xbd, 0, 0, 0, 0, 0);

            if (!card->attached)
                rc = ERROR;
            else
                rc = OK;

            break;
            }

        case XBD_STACK_COMPLETE:
            {
            SDMMC_XBD_DBG (SDMMC_XBD_DBG_IOCTL,
                           "[xbdIoctl]: xbd (0x%08x) XBD_STACK_COMPLETE\n",
                           (int)xbd, 0, 0, 0, 0, 0);

            if (!xbd->xbdInstantiated)
                {
                rc = erfEventRaise (xbdEventCategory, xbdEventInstantiated,
                                    ERF_ASYNC_PROC,
                                    xbd->xbd.xbd_dev.dv_xname, NULL);
                xbd->xbdInstantiated = TRUE;
                }

            break;
            }

        case XBD_GETINFO:
            {
            XBD_INFO * info;

            SDMMC_XBD_DBG (SDMMC_XBD_DBG_IOCTL,
                           "[xbdIoctl]: xbd (0x%08x) XBD_GETINFO\n",
                           (int)xbd, 0, 0, 0, 0, 0);

            info = (XBD_INFO *)arg;

            info->properties = 0;

            if (card->isWp == TRUE)
                info->properties |= XBD_INFO_READONLY;

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

/*******************************************************************************
*
* sdMmcXbdStrategy - XBD block device strategy routine
*
* This routine handles the data transaction.
*
* The information in the bio is used by the device driver to transfer data.
* This is an asynchronous function in that once the data in the bio is used
* to setup the data transfer, or queued until the device is ready,
* the function returns.
*
* RETURN: OK or ERROR
*
* ERRNO: N/A
*/

LOCAL int sdMmcXbdStrategy
    (
    struct xbd *    self,
    struct bio *    bio
    )
    {
    SDMMC_XBD_DEV * xbd;
    STATUS rc;

    if (self == NULL || bio == NULL)
        return (ERROR);

    SDMMC_XBD_DBG (SDMMC_XBD_DBG_RW, "[xbdStrategy]: xbd (0x%08x) %s\n",
                   (int)self,
                   (int)((bio->bio_flags == BIO_READ) ? "read" : "write"),
                   0, 0, 0, 0);

    xbd = (SDMMC_XBD_DEV *)self;
    if (xbd->xbdInserted == FALSE)
        {
        bio_done (bio, ENXIO);
        return (ERROR);
        }

    rc = semTake (xbd->bioMutex, WAIT_FOREVER);
    if (rc != OK)
        {
        bio_done (bio, ENXIO);
        return (ERROR);
        }

    if (sdMmcServTskPri > bio->bio_priority)
        {
        sdMmcServTskPri = bio->bio_priority;

        /* raise service task priority */
        /* return from taskPrioritySet is not needed */

        (void)taskPrioritySet(xbd->svcTaskId, bio->bio_priority);
        }

    SDMMC_XBD_DBG(SDMMC_XBD_DBG_RW, "[xbdStrategy]: sdMmcServTskPri is set %d \n",
                 (int)sdMmcServTskPri, 0, 0, 0, 0, 0);

    if (xbd->directModeFlag)
        {
        sdMmcXbdExecBio(xbd, bio);
        }
    else
        {
        /* append to the bio queue */

        if (xbd->bioTail)
            {
            xbd->bioTail->bio_chain = bio;
            }
        else
            {
            xbd->bioHead = bio;
            }

        xbd->bioTail = bio;

        /* signal the service task */

        semGive (xbd->bioReady);
        }

    semGive (xbd->bioMutex);

    return (OK);
    }

/*******************************************************************************
*
* sdMmcXbdDump - XBD block device dump routine
*
* This routine handles the data transaction with interrupts off.
*
* This call is similar to the xf_strategy call, only it is executed with
* interrupts off.  This is intended to allow a filesystem to dump data to
* the device prior to a system crash.
*
* RETURN: EINVAL always
*
* ERRNO: N/A
*/

LOCAL int sdMmcXbdDump
    (
    struct xbd *    self,
    sector_t        pos,
    void *          data,
    size_t          size
    )
    {
    return (EINVAL);
    }

/*******************************************************************************
*
* sdMmcXbdGetBioPrio - Get the highest priority in the bio list.
*
* This routine get highest prority in the bio list.
*
* RETURN: highest priority value
*
* ERRNO: N/A
*/

LOCAL int sdMmcXbdGetBioPrio
    (
    SDMMC_XBD_DEV * xbd
    )
    {
    int highestPrio = sdMmcServTskPri;
    struct bio * nextBio;

    if(NULL != xbd->bioHead)
        {
        highestPrio = xbd->bioHead->bio_priority;
        for(nextBio = xbd->bioHead->bio_chain; nextBio != NULL;
            nextBio = nextBio->bio_chain)
            {
            highestPrio = (highestPrio < nextBio->bio_priority) ?
                           highestPrio:nextBio->bio_priority;
            }
        }
    SDMMC_XBD_DBG(SDMMC_XBD_DBG_RW,
                 "[sdMmcXbdGetBioPrio]: highestPrio is %d \n",
                 (int)highestPrio, 0, 0, 0, 0, 0);
    return highestPrio;
    }
