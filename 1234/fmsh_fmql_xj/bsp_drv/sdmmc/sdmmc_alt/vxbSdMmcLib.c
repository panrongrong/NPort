/* vxbSdMmcLib.c - Generic SD/MMC Library */

/*
 * Copyright (c) 2009-2015, 2017 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01t,01nov17,ffu  fix sd Card initialization is very slow (VXW6-86555)
01s,18jun15,zly  ACMD42 is not included in SD card driver.(VXW6-84329)
01r,20apr15,myt  fix the invalid enable SD_CLK with violation of sd spec (VXW6-84013) 
01q,23mar15,m_y  correct the status check in function sdMmcACmdSetHighSpeed
                 (VXW6-83917)
01p,11feb15,m_y  modify sdMmcACmdSetHighSpeed add check action (VXW6-83917)
01o,05dec14,lxj  enable high speed mode by SD card version (VXW6-83877)
01n,12sep14,lxj  rework SD Card clock frequency is unexpectedlyi(VXW6-80641)
                 fix SD Card can not be recognized on SMP on Zynq7k (VXW6-83405)
01m,02may13,e_d  add sd version check in sdMmcACmdSetHighSpeed. (WIND00413469)
01l,17dec12,e_d  add status check in sdMmcStatusWaitReadyForData. (WIND00390089)
01k,19oct12,sye  return error when writing to a write protected card.
                 (WIND00377583)
01j,18oct12,b_m  set ACMD41 argument to 3.3V if both 3.0V & 3.3V are not
                 reported by host capbilities.
01i,09jul12,my_  (1) add high speed support
                 (2) fix compiler warnings
01h,15oct11,syt  add special workaround needed for APM86290 SDHC.
01g,05jul11,j_z  update to use SDMMC_CMD from host control block
                 instead of stack variable.
01f,12apr11,syt  merge updates from 6.8.2 and remove interrupt lock
                 operation during get absTicks in function sdMmcIdentify().
01e,15feb11,my_  increase timeout value when identifying card.
01d,21jan11,my_  setup SD bus power before any operation.
01c,14jul10,b_m  avoid using stack variable to DMA.
01b,05jul10,b_m  ensure host operation under 1-bit bus width at first;
                 support version 1.x SD memory cards.
01a,18aug09,b_m  written.
*/

/*
DESCRIPTION

This is the generic SD/MMC library to implement neccessary SD commands for
a SD memory card which is compatible with SD memory card specification 2.0.

The library can support up to 32 MMC cards plus 32 SD cards in a system.

Please note:
MMC card, high speed SD memory card and SDIO card are not supported with
current implementation.
*/

/* includes */

#include <vxWorks.h>
#include <intLib.h>
#include <stdio.h>
#include <ioLib.h>
#include <errnoLib.h>
#include <string.h>
#include <vxBusLib.h>
#include <cacheLib.h>
#include <../src/hwif/h/storage/vxbSdMmcLib.h>

/* defines */

#ifdef  SDMMC_LIB_DBG_ON

#ifdef  LOCAL
#undef  LOCAL
#define LOCAL
#endif

#define SDMMC_LIB_DBG_CMD           0x00000001
#define SDMMC_LIB_DBG_ERR           0x00000002
#define SDMMC_LIB_DBG_ALL           0xffffffff
#define SDMMC_LIB_DBG_OFF           0x00000000

UINT32 sdMmcLibDbgMask = SDMMC_LIB_DBG_OFF;

IMPORT FUNCPTR _func_logMsg;

#define SDMMC_LIB_DBG(mask, string, a, b, c, d, e, f)               \
    if ((sdMmcLibDbgMask & mask) || (mask == SDMMC_LIB_DBG_ALL))    \
        if (_func_logMsg != NULL) \
            (* _func_logMsg)(string, a, b, c, d, e, f)

#else
#define SDMMC_LIB_DBG(mask, string, a, b, c, d, e, f)
#endif  /* SDMMC_LIB_DBG_ON */

/* imports */

IMPORT UINT32 sysClkRateGet(void);

#ifndef _WRS_CONFIG_SMP
IMPORT volatile UINT64 vxAbsTicks;
#else
IMPORT UINT64 vxAbsTicksGet (void);
#endif /* _WRS_CONFIG_SMP */

/* locals */

LOCAL STATUS sdMmcACmdSwitchFunc (SDMMC_CARD * card, void * buffer,
                                  UINT32 len, int mode, int group, int val);
LOCAL STATUS sdMmcACmdSetHighSpeed (SDMMC_CARD * card);

/*
 * The following two tables defines the maximum data transfer rate
 * per one date line - TRAN_SPEED
 */

LOCAL UINT32 sdMmcCsdTranSpdfUnit[] = {
    10000, 100000, 1000000, 10000000
};

LOCAL UINT32 sdMmcCsdTranSpdTime[] = {
     0, 10, 12, 13, 15, 20, 25, 30,
    35, 40, 45, 50, 55, 60, 70, 80,
};

/* SD/MMC bus width strings */

LOCAL char sdMmcBusWidth[SDMMC_BUS_WIDTH_MAX][6] = {
    "1-bit",
    "4-bit",
    "8-bit",
};

/* SD card version strings */

LOCAL char sdMmcVersion[SD_VERSION_MAX][5] = {
    "1.00",
    "1.10",
    "2.00",
};

/* SD/MMC card dev index */

LOCAL UINT32 mmcCardIdx;
LOCAL UINT32 sdCardIdx;

/*******************************************************************************
*
* sdMmcHostCmdIssue - host routine to issue the command
*
* This routine issues the command in a host.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS sdMmcHostCmdIssue
    (
    SDMMC_HOST * host
    )
    {
    STATUS rc;
    SDMMC_CMD *cmd = &host->cmdIssued;

    /* clear last command error */

    cmd->cmdErr = 0;
    cmd->dataErr = 0;

    /* call host routine to issue the command */

    rc = host->ops.cmdIssue (host);
    if (rc == ERROR)
        {
        SDMMC_LIB_DBG (SDMMC_LIB_DBG_ERR, "CMD%d: issue error\n",
                       cmd->cmdIdx, 0, 0, 0, 0 ,0);
        }

    SDMMC_LIB_DBG (SDMMC_LIB_DBG_CMD, "CMD%d: issued\n",
                   cmd->cmdIdx, 0, 0, 0, 0 ,0);

    return rc;
    }

/*******************************************************************************
*
* sdMmcHostClkFreqSetup - host routine to setup the clock frequency
*
* This routine setup the clock frequency for a host.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void sdMmcHostClkFreqSetup
    (
    SDMMC_HOST * host,
    UINT32       clk
    )
    {
    /* call host routine to setup the clock frequency */

    if (host->ops.clkFreqSetup != NULL)
        {
        host->ops.clkFreqSetup (host, clk);

        /* update host information */

        host->curClkFreq = clk;
        }
    }

/*******************************************************************************
*
* sdMmcHostVddSetup - host routine to setup the sd bus power and voltage.
*
* This routine setup the sd bus power and voltage.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void sdMmcHostVddSetup
    (
    SDMMC_HOST * host,
    UINT32       vdd
    )
    {
    /* call host routine to setup SD bus power */

    if (host->ops.vddSetup != NULL)
        {
        host->ops.vddSetup (host, vdd);

        /* update host information */

        host->vdd = vdd;
        }
    }

/*******************************************************************************
*
* sdMmcHostBusWidthSetup - host routine to setup the bus width
*
* This routine setup the bus width for a host.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void sdMmcHostBusWidthSetup
    (
    SDMMC_HOST * host,
    UINT32       width
    )
    {
    /* call host routine to setup the bus width */

    host->ops.busWidthSetup (host, width);

    /* update host information */

    host->curBusWidth = width;
    }

/*******************************************************************************
*
* sdMmcHostCardWpCheck - host routine to check if card is write protected
*
* This routine checks if card is write protected for a host.
*
* RETURNS: TRUE if card is write protected or FALSE if not.
*
* ERRNO: N/A
*/

LOCAL BOOL sdMmcHostCardWpCheck
    (
    SDMMC_HOST * host
    )
    {
    /* call host routine to check if card is write protected */

    return host->ops.cardWpCheck (host);
    }

/*******************************************************************************
*
* sdMmcCmdGoIdleState - issue CMD0 to reset all cards to idle state
*
* This routine issues CMD0 to reset all cards to idle state.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdGoIdleState
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD_GO_IDLE_STATE, 0, SDMMC_CMD_RSP_NONE, FALSE);

    return sdMmcHostCmdIssue (card->host);
    }

/*******************************************************************************
*
* sdMmcCmdAllSendCid - issue CMD2 to ask all cards to send their CIDs
*
* This routine issues CMD2 to ask all cards to send their CIDs.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdAllSendCid
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    STATUS rc;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD_ALL_SEND_CID, 0, SDMMC_CMD_RSP_R2, FALSE);

    rc = sdMmcHostCmdIssue (card->host);
    if (rc == ERROR)
        return ERROR;

    bcopy ((char *)cmd->cmdRsp, (char *)&(card->info.cid), sizeof(SDMMC_CID));

    return OK;
    }

/*******************************************************************************
*
* sdMmcCmdSendRelativeAddr - issue CMD3 to ask card to publish a new RCA
*
* This routine issues CMD3 to ask card to publish a new RCA.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdSendRelativeAddr
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    STATUS rc;

    SDMMC_CMD_FORM (cmd, SD_CMD_SEND_RELATIVE_ADDR, 0, SDMMC_CMD_RSP_R6, FALSE);

    rc = sdMmcHostCmdIssue (card->host);
    if (rc == ERROR)
        return ERROR;

    card->info.rca = (UINT16)(cmd->cmdRsp[0] >> 16) & 0xffff;

    return OK;
    }

/*******************************************************************************
*
* sdMmcCmdSetDsr - issue CMD4 to program the DSR of all cards
*
* This routine issues CMD4 to program the DSR of all cards.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdSetDsr
    (
    SDMMC_CARD *    card,
    UINT32          dsr
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD_SET_DSR, dsr << 16, SDMMC_CMD_RSP_NONE, FALSE);

    return sdMmcHostCmdIssue (card->host);
    }

/*******************************************************************************
*
* sdMmcCmdSelectCard - issue CMD7 to select a card
*
* This routine issues CMD7 to select a card.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdSelectCard
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD_SELECT_CARD, SDMMC_CMD_ARG_RCA (card->info.rca),
                    SDMMC_CMD_RSP_R1, FALSE);

    return sdMmcHostCmdIssue (card->host);
    }

/*******************************************************************************
*
* sdMmcCmdDeselectCard - issue CMD7 to deselect a card
*
* This routine issues CMD7 to deselect a card.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdDeselectCard
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD_SELECT_CARD, 0, SDMMC_CMD_RSP_NONE, FALSE);

    return sdMmcHostCmdIssue (card->host);
    }

/*******************************************************************************
*
* sdMmcCmdSendIfCond - issue CMD8 to ask card to send interface condition
*
* This routine issues CMD8 to ask card to send interface condition.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdSendIfCond
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_HOST * host = card->host;
    SDMMC_CMD *cmd = &host->cmdIssued;
    UINT32 arg;
    STATUS rc;

    /* sanity test to host supported voltage */

    if (!host->capbility & OCR_VDD_VOL_MASK)
        {
        SDMMC_LIB_DBG (SDMMC_LIB_DBG_ERR, "Host does not support 2.7-3.6V voltage\n",
                       0, 0, 0, 0, 0 ,0);
        return ERROR;
        }

    arg = (SD_CMD8_ARG_VHS_27_36 << SD_CMD8_ARG_VHS_SHIFT) | SD_CMD8_ARG_CHK_PATTERN;
    SDMMC_CMD_FORM (cmd, SD_CMD_SEND_IF_COND, arg, SDMMC_CMD_RSP_R7, FALSE);

    rc = sdMmcHostCmdIssue (card->host);

    if (card->host->cmdIssued.cmdErr & SDMMC_CMD_ERR_TIMEOUT)
        {
        card->info.version = SD_VERSION_100;
        return OK;
        }

    if (rc == ERROR)
        return ERROR;

    if ((cmd->cmdRsp[0] & 0xff) != SD_CMD8_ARG_CHK_PATTERN)
        card->info.version = SD_VERSION_UNKNOWN;
    else
        card->info.version = SD_VERSION_200;

    return OK;
    }

/*******************************************************************************
*
* sdMmcCmdSendCsd - issue CMD9 to ask card to send its CSD
*
* This routine issues CMD9 to ask card to send its CSD.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdSendCsd
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    UINT32 csize, csizeMulti;
    STATUS rc;
    UINT64 blkNum;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD_SEND_CSD, SDMMC_CMD_ARG_RCA (card->info.rca),
                    SDMMC_CMD_RSP_R2, FALSE);

    rc = sdMmcHostCmdIssue (card->host);
    if (rc == ERROR)
        return ERROR;

    bcopy ((char *)cmd->cmdRsp, (char *)card->info.csd, 16);

    /* decode CSD fields */

    card->info.tranSpeed = sdMmcCsdTranSpdfUnit[(card->info.csd[3] & 0x7)] *
                      sdMmcCsdTranSpdTime[((card->info.csd[3] >> 3) & 0x0f)];

    /* maximum read block length */

    card->info.readBlkLen = 1 << (card->info.csd[5] & 0x0f);

    /* per SD spec, the maximum write block length is equal to read block */

    card->info.writeBlkLen = card->info.readBlkLen;

    /* calculate user data capacity */

    if (card->info.highCapacity)
        {
        csize = ((card->info.csd[7] & 0x3f) << 16) |
                 (card->info.csd[8] << 8) |
                  card->info.csd[9];
        csizeMulti = 8;
        }
    else
        {
        csize = ((UINT32)(card->info.csd[6] & 0x03) << 10) |
                ((UINT32)(card->info.csd[7]) << 2) |
                ((card->info.csd[8] >> 6) & 0x03);
        csizeMulti = ((card->info.csd[9] & 0x03) << 1) |
                     ((card->info.csd[10] >> 7) & 0x01);
        }

    blkNum = (csize + 1) << (csizeMulti + 2);
    card->info.blkNum = blkNum;
    card->info.capacity = blkNum * card->info.readBlkLen;

    return OK;
    }

/*******************************************************************************
*
* sdMmcCmdSendCid - issue CMD10 to ask card to send its CID
*
* This routine issues CMD10 to ask card to send its CID.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdSendCid
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    STATUS rc;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD_SEND_CID, SDMMC_CMD_ARG_RCA (card->info.rca),
                    SDMMC_CMD_RSP_R2, FALSE);

    rc = sdMmcHostCmdIssue (card->host);
    if (rc == ERROR)
        return ERROR;

    bcopy ((char *)cmd->cmdRsp, (char *)&(card->info.cid), sizeof(SDMMC_CID));

    return OK;
    }

/*******************************************************************************
*
* sdMmcCmdStopTransmission - issue CMD12 to force card to stop transmission
*
* This routine issues CMD12 to force card to stop transmission.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdStopTransmission
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD_STOP_TRANSMISSION, 0, SDMMC_CMD_RSP_R1B, FALSE);

    return sdMmcHostCmdIssue (card->host);
    }

/*******************************************************************************
*
* sdMmcCmdSendStatus - issue CMD13 to ask card to send its status
*
* This routine issues CMD13 to ask card to send its status.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdSendStatus
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    STATUS rc;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD_SEND_STATUS, SDMMC_CMD_ARG_RCA (card->info.rca),
                    SDMMC_CMD_RSP_R1, FALSE);

    rc = sdMmcHostCmdIssue (card->host);
    if (rc == ERROR)
        return ERROR;

    card->info.cardSts = cmd->cmdRsp[0];

    return OK;
    }

/*******************************************************************************
*
* sdMmcCmdGoInactiveState - issue CMD15 to put card into the inactive state
*
* This routine issues CMD15 to put card into the inactive state.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdGoInactiveState
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD_GO_INACTIVE_STATE,
                    SDMMC_CMD_ARG_RCA (card->info.rca),
                    SDMMC_CMD_RSP_NONE, FALSE);

    return sdMmcHostCmdIssue (card->host);
    }

/*******************************************************************************
*
* sdMmcCmdSetBlockLen - issue CMD16 to set block length for block commands
*
* This routine issues CMD16 to set block length for block commands.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdSetBlockLen
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;

    /* block length is always 512 bytes */

    SDMMC_CMD_FORM (cmd, SDMMC_CMD_SET_BLOCKLEN, SDMMC_BLOCK_SIZE,
                    SDMMC_CMD_RSP_R1, FALSE);

    return sdMmcHostCmdIssue (card->host);
    }

/*******************************************************************************
*
* sdMmcCmdReadSingleBlock - issue CMD17 to read a single block
*
* This routine issues CMD17 to read a single block.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdReadSingleBlock
    (
    SDMMC_CARD *    card,
    void *          buf,
    UINT64          blkNo
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    UINT32 arg;

    if (card->info.highCapacity)
        arg = (UINT32)blkNo;
    else
        arg = (UINT32)blkNo * SDMMC_BLOCK_SIZE;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD_READ_SINGLE_BLOCK, arg, SDMMC_CMD_RSP_R1, TRUE);
    SDMMC_DATA_FORM (cmd, buf, 1, SDMMC_BLOCK_SIZE, TRUE);

    return sdMmcHostCmdIssue (card->host);
    }

/*******************************************************************************
*
* sdMmcCmdReadMultipleBlock - issue CMD18 to read multiple blocks
*
* This routine issues CMD18 to read multiple blocks.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdReadMultipleBlock
    (
    SDMMC_CARD *    card,
    void *          buf,
    UINT64          blkNo,
    UINT32          numBlks
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    UINT32 arg;

    if (card->info.highCapacity)
        arg = (UINT32)blkNo;
    else
        arg = (UINT32)blkNo * SDMMC_BLOCK_SIZE;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD_READ_MULTI_BLOCK, arg, SDMMC_CMD_RSP_R1, TRUE);
    SDMMC_DATA_FORM (cmd, buf, numBlks, SDMMC_BLOCK_SIZE, TRUE);

    return sdMmcHostCmdIssue (card->host);
    }

/*******************************************************************************
*
* sdMmcCmdReadBlock - issue CMD17 or CMD18 to read blocks
*
* This routine issues CMD17 or CMD18 to read blocks.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdReadBlock
    (
    SDMMC_CARD *    card,
    void *          buf,
    UINT64          blkNo,
    UINT32          numBlks
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    UINT32 idx;
    UINT32 arg;

    if (numBlks > 1)
        idx = SDMMC_CMD_READ_MULTI_BLOCK;
    else
        idx = SDMMC_CMD_READ_SINGLE_BLOCK;

    if (card->info.highCapacity)
        arg = (UINT32)blkNo;
    else
        arg = (UINT32)blkNo * SDMMC_BLOCK_SIZE;

    SDMMC_CMD_FORM (cmd, idx, arg, SDMMC_CMD_RSP_R1, TRUE);
    SDMMC_DATA_FORM (cmd, buf, numBlks, SDMMC_BLOCK_SIZE, TRUE);

    return sdMmcHostCmdIssue (card->host);
    }

/*******************************************************************************
*
* sdMmcCmdWriteSingleBlock - issue CMD24 to write a single block
*
* This routine issues CMD24 to write a single block.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdWriteSingleBlock
    (
    SDMMC_CARD *    card,
    void *          buf,
    UINT64          blkNo
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    UINT32 arg;

    if (card->info.highCapacity)
        arg = (UINT32)blkNo;
    else
        arg = (UINT32)blkNo * SDMMC_BLOCK_SIZE;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD_WRITE_SINGLE_BLOCK, arg, SDMMC_CMD_RSP_R1, TRUE);
    SDMMC_DATA_FORM (cmd, buf, 1, SDMMC_BLOCK_SIZE, FALSE);

    return sdMmcHostCmdIssue (card->host);
    }

/*******************************************************************************
*
* sdMmcCmdWriteMultipleBlock - issue CMD25 to write multiple blocks
*
* This routine issues CMD25 to writ multiple blocks.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdWriteMultipleBlock
    (
    SDMMC_CARD *    card,
    void *          buf,
    UINT64          blkNo,
    UINT32          numBlks
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    UINT32 arg;

    if (card->info.highCapacity)
        arg = (UINT32)blkNo;
    else
        arg = (UINT32)blkNo * SDMMC_BLOCK_SIZE;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD_WRITE_MULTI_BLOCK, arg, SDMMC_CMD_RSP_R1, TRUE);
    SDMMC_DATA_FORM (cmd, buf, numBlks, SDMMC_BLOCK_SIZE, FALSE);

    return sdMmcHostCmdIssue (card->host);
    }

/*******************************************************************************
*
* sdMmcCmdWriteBlock - issue CMD24 or CMD25 to write blocks
*
* This routine issues CMD24 or CMD25 to write blocks.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdWriteBlock
    (
    SDMMC_CARD *    card,
    void *          buf,
    UINT64          blkNo,
    UINT32          numBlks
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    UINT32 idx;
    UINT32 arg;

    if (numBlks > 1)
        idx = SDMMC_CMD_WRITE_MULTI_BLOCK;
    else
        idx = SDMMC_CMD_WRITE_SINGLE_BLOCK;

    if (card->info.highCapacity)
        arg = (UINT32)blkNo;
    else
        arg = (UINT32)blkNo * SDMMC_BLOCK_SIZE;

    SDMMC_CMD_FORM (cmd, idx, arg, SDMMC_CMD_RSP_R1, TRUE);
    SDMMC_DATA_FORM (cmd, buf, numBlks, SDMMC_BLOCK_SIZE, FALSE);

    return sdMmcHostCmdIssue (card->host);
    }

/*******************************************************************************
*
* sdMmcCmdAppCmd - issue CMD55 to indicate an application command followed
*
* This routine issues CMD55 to indicate an application command followed.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCmdAppCmd
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD_APP_CMD, SDMMC_CMD_ARG_RCA (card->info.rca),
                    SDMMC_CMD_RSP_R1, FALSE);

    return sdMmcHostCmdIssue (card->host);
    }

/*******************************************************************************
*
* sdMmcACmdSetBusWidth - issue ACMD6 to set the bus width for data transfer
*
* This routine issues ACMD6 to set the bus width for data transfer.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcACmdSetBusWidth
    (
    SDMMC_CARD *    card,
    UINT32          width
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    UINT32 arg;
    STATUS rc;

    rc = sdMmcCmdAppCmd (card);
    if (rc == ERROR)
        return ERROR;

    if (width == SDMMC_BUS_WIDTH_1BIT)
        arg = SD_ACMD6_ARG_1BIT_BUS;
    else
        arg = SD_ACMD6_ARG_4BIT_BUS;

    SDMMC_CMD_FORM (cmd, SD_ACMD_SET_BUS_WIDTH, arg, SDMMC_CMD_RSP_R1, FALSE);

    return sdMmcHostCmdIssue (card->host);
    }

/*******************************************************************************
*
* sdMmcACmdSendOpCond - issue ACMD41 to ask card to send its OCR
*
* This routine issues ACMD41 to ask card to send its OCR.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcACmdSendOpCond
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    UINT32 arg;
    STATUS rc;

    rc = sdMmcCmdAppCmd (card);
    if (rc == ERROR)
        return ERROR;

    arg = card->host->capbility & OCR_VDD_VOL_MASK;
    if (card->info.version == SD_VERSION_200)
        {
        if (card->host->capbility & OCR_CARD_CAP_STS)
            arg |= OCR_CARD_CAP_STS;
        }

   /*
    * Set ACMD41 argument to 3.3V if both 3.0V & 3.3V are not reported
    * by host controller's capbilities register
    */

    if (!(arg & (OCR_VDD_VOL_29_30 | OCR_VDD_VOL_30_31 | OCR_VDD_VOL_32_33 | OCR_VDD_VOL_33_34)))
        arg |= (OCR_VDD_VOL_32_33 | OCR_VDD_VOL_33_34);
    SDMMC_CMD_FORM (cmd, SD_ACMD_SEND_OP_COND, arg, SDMMC_CMD_RSP_R3, FALSE);

    rc = sdMmcHostCmdIssue (card->host);

    if (rc == OK && (cmd->cmdRsp[0] & OCR_CARD_PWRUP_STS))
        {
        card->info.voltage = cmd->cmdRsp[0] & OCR_VDD_VOL_MASK;
        card->info.highCapacity = ((cmd->cmdRsp[0] & OCR_CARD_CAP_STS) > 0) ? TRUE : FALSE;
        return OK;
        }

    return ERROR;
    }

/*******************************************************************************
* 
* sdMmcACmdClrCardDetect - issue ACMD42 to clrear card detect
*
* This routine issues ACMD42 to clrear card detect.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcACmdClrCardDetect
    (
    SDMMC_CARD *    card,
    UINT32          connect
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    UINT32 arg;
    STATUS rc;

    rc = sdMmcCmdAppCmd (card);
    if (rc == ERROR)
        return ERROR;

    arg = connect;

    SDMMC_CMD_FORM (cmd, SD_ACMD_SET_CLR_CARD_DETECT, arg, SDMMC_CMD_RSP_R1, FALSE);

    return sdMmcHostCmdIssue (card->host);
    }


/*******************************************************************************
*
* sdMmcACmdSendScr - issue ACMD51 to ask card to send its SCR
*
* This routine issues ACMD51 to ask card to send its SCR.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcACmdSendScr
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    SD_SCR * scr;
    STATUS rc;

    rc = sdMmcCmdAppCmd (card);
    if (rc == ERROR)
        return ERROR;

    scr = cacheDmaMalloc (sizeof(SD_SCR));
    if (scr == NULL)
        return ERROR;

    SDMMC_CMD_FORM (cmd, SD_ACMD_SEND_SCR, 0, SDMMC_CMD_RSP_R1, TRUE);
    SDMMC_DATA_FORM (cmd, scr, 1, sizeof(SD_SCR), TRUE);

    rc = sdMmcHostCmdIssue (card->host);
    if (rc == ERROR)
        {
        (void)cacheDmaFree ((char *)scr);
        return ERROR;
        }

    /* save SCR */

    bcopy ((char *)scr, (char *)&(card->info.scr), sizeof(SD_SCR));

    card->info.sdSpec = scr->spec & 0x0f;

    /* update the SD card version */

    card->info.version = (UINT16)card->info.sdSpec;

    card->info.sdSec = (scr->config >> 4) & 0x7;
    if (scr->config & SCR_SD_BUS_WIDTH_4BIT)
        card->info.dat4Bit = TRUE;

    (void)cacheDmaFree ((char *)scr);

    return OK;
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
*/

void sdMmcStatusWaitReadyForData
    (
    SDMMC_CARD * card
    )
    {
    STATUS rc;

    while (1)
        {
        rc = sdMmcCmdSendStatus (card);
        if (rc == ERROR)
            return;

        if (card->info.cardSts & CARD_STS_READY_FOR_DATA)
            break;
        }
    }

/*******************************************************************************
*
* sdMmcACmdSetHighSpeed - put card into high speed mode 
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
*/

LOCAL STATUS sdMmcACmdSetHighSpeed
    (
    SDMMC_CARD * card
    )
    {
    STATUS ret = OK;
    UINT32 dataVer;

    if (card->info.version < SD_VERSION_110)
        return (ENOTSUP);

    char * buffer = cacheDmaMalloc (64);
    if (!buffer)
        return ERROR;
    memset (buffer, 0, 64);

    /* Send cmd6 mode0 to check function */

    ret = sdMmcACmdSwitchFunc (card, buffer, 64,
            SD_ACMD6_GET_FUNC, SD_ACMD6_GROUP_ACCESS_MODE, 1);
    if (ret != OK)
        goto error;

    /* Read the buffer stauts */
    
    if ((buffer[13] & 0x2) == 0)
        {
        /*
         * This means the card doesnot support hight speed
         * we donot need to change
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

    ret = sdMmcACmdSwitchFunc (card, buffer, 64,
            SD_ACMD6_SET_FUNC, SD_ACMD6_GROUP_ACCESS_MODE, 1);
    
    if (ret != OK)
        goto error;


    /*
     * 379:376 mode 0 - The function which can be switched in function group 1.
     * mode 1 - The function which is result of the switch command, in function
     * group 1. 0xF shows function set error with the argument.
     */

    /* dataVer == 0x0 or 0x1 should check the switch result */
    
    if ((buffer[16] & 0x0F) != 0x1)
        {
        ret = ENOTSUP;
        goto error;
        }

    if (dataVer == 0x01)
        {
        /*
         * 287:272 Busy Status of functions in group 1. 
         * If bit [i] is set, function [i] is busy.
         * This field can be read in mode 0 and mode 1
         * High speed is function 1.
         * 0 indicates ready and 1 indicates busy
         */
        if ((buffer[28] & 0x02) != 0)
            {
            ret = ENOTSUP;
            goto error;
            }
        }

error:
    (void)cacheDmaFree (buffer);
    return ret;
    }

/*******************************************************************************
*
* sdMmcACmdSwitchFunc - send application specific command 6 
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

LOCAL STATUS sdMmcACmdSwitchFunc 
    (
    SDMMC_CARD * card, 
    void * buffer, 
    UINT32 len,
    int    mode, 
    int    group, 
    int    val
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    STATUS rc;
    unsigned int arg;
    
    arg = mode;
    arg |= 0xffffff;
    arg &= ~(0xf << (group * 4));
    arg |= (val << (group * 4)); 

    SDMMC_CMD_FORM (cmd, SD_CMD_SWITCH_FUNC, arg, SDMMC_CMD_RSP_R1, TRUE);
    SDMMC_DATA_FORM (cmd, buffer, 1, len, TRUE);

    rc = sdMmcHostCmdIssue (card->host);
    if (rc == ERROR)
        return ERROR;

    if (cmd->cmdErr)
        return ERROR;

    if (cmd->dataErr)
        return ERROR;

    return OK;
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
*/

STATUS sdMmcBlkRead
    (
    SDMMC_CARD *    card,
    sector_t        blkNo,
    UINT32          numBlks,
    void *          pBuf
    )
    {
    STATUS rc;

    return sdMmcCmdReadBlock (card, pBuf, blkNo, numBlks);
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
*/

STATUS sdMmcBlkWrite
    (
    SDMMC_CARD *    card,
    sector_t        blkNo,
    UINT32          numBlks,
    void *          pBuf
    )
    {
    STATUS rc;

    if (card->isWp)
        {
        (void)errnoSet (S_ioLib_WRITE_PROTECTED);
        return ERROR;
        }
    return sdMmcCmdWriteBlock (card, pBuf, blkNo, numBlks);
    }

/*******************************************************************************
*
* sdMmcIdentify - identify SD/MMC card in the system
*
* This routine implements the card identify process according to the SD spec.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcIdentify
    (
    SDMMC_CARD * card
    )
    {
    STATUS rc;
    UINT64 start, end;
    UINT32 workSpeed;

    /* power up SD bus */

    sdMmcHostVddSetup (card->host, card->host->vdd);
    
    /* set default host clock frequency and bus width */

    sdMmcHostClkFreqSetup (card->host, SDMMC_CLK_FREQ_400KHZ);

    /* initialization delay time, 75 clock in 400KHZ, about 200us */

    (void)vxbUsDelay(200);

    sdMmcHostBusWidthSetup (card->host, SDMMC_BUS_WIDTH_1BIT);

    /* ensure card infomration gets cleared initially */

    bzero ((char *)&(card->info), sizeof(SDMMC_INFO));

    /* CMD0: reset all cards on the bus */

    rc = sdMmcCmdGoIdleState (card);
    if (rc == ERROR)
        goto err;

    /* CMD8: request card to send interface condition */

    rc = sdMmcCmdSendIfCond (card);
    if (rc == ERROR)
        goto err;

    /* AMD41: request card to send its OCR */

#ifdef _WRS_CONFIG_SMP
    start = vxAbsTicksGet();
#else
    start = vxAbsTicks;
#endif /* _WRS_CONFIG_SMP */

    while (1)
        {
        rc = sdMmcACmdSendOpCond (card);

        if (rc == OK)
            break;

#ifdef _WRS_CONFIG_SMP
        end = vxAbsTicksGet();
#else
        end = vxAbsTicks;
#endif /* _WRS_CONFIG_SMP */

        /* stardard says 1s for timeout, here we give it 2s */

        if ((end - start) > (UINT64)(sysClkRateGet() * 2))
           {
           SDMMC_LIB_DBG (SDMMC_LIB_DBG_ERR, "ACMD41 timeout\n",
                          0, 0, 0, 0, 0, 0);
           goto err;
           }
        }

    /* CMD2: request card to send CID and put it in identify mode */

    rc = sdMmcCmdAllSendCid (card);
    if (rc == ERROR)
        goto err;

    /* CMD3: request card to send RCA */

    do
        {
        rc = sdMmcCmdSendRelativeAddr (card);
        if (rc == ERROR)
            goto err;
        } while (card->info.rca == 0);

    /* CMD9: request card to send CSD */

    rc = sdMmcCmdSendCsd (card);
    if (rc == ERROR)
        goto err;

    /* After Card Identification, the station go into Data Transfer Mode */

    if (card->info.tranSpeed > SDMMC_CLK_FREQ_400KHZ)
        {
        workSpeed = card->info.tranSpeed < SDMMC_CLK_FREQ_25MHZ ?
                    card->info.tranSpeed : SDMMC_CLK_FREQ_25MHZ;

        sdMmcHostClkFreqSetup (card->host, workSpeed);
        }
    
    card->info.highSpeed = FALSE;

    /* CMD7: select one card and put it into transfer state */

    rc = sdMmcCmdSelectCard (card);
    if (rc == ERROR)
        goto err;

    /* AMD51: request card to send its SCR */

    rc = sdMmcACmdSendScr (card);
    if (rc == ERROR)
       {
       /* 
        * Wordaround for APM86290's SDHC, ACMD51 command needed to be
        * issued twice to get correct value of SD card's OCR register
        */

        if (sdMmcACmdSendScr (card) == ERROR)
           goto err;
       }

    /* CMD16: set block len */

    if (card->info.highCapacity == FALSE)
        {
        rc = sdMmcCmdSetBlockLen (card);
        if (rc == ERROR)
            return ERROR;
        }

    /* switch to 4 bit mode if needed */

    if (card->info.dat4Bit)
        {
        rc = sdMmcACmdSetBusWidth (card, SDMMC_BUS_WIDTH_4BIT);
        if (rc == ERROR)
            goto err;

        /* setup host to enable 4-bit bus width */

        sdMmcHostBusWidthSetup (card->host, SDMMC_BUS_WIDTH_4BIT);
        }

    /* ACMD42: clear card detect and set data3 as data line */
    rc = sdMmcACmdClrCardDetect(card, SD_ACMD42_ARG_CLR_CARD_DETECT);
    if (rc == ERROR)
        goto err;	

    /* setup host to enable high speed clock (50 MHz) if needed */

    if (card->info.sdSpec >= SD_VERSION_110)
        {
        rc = sdMmcACmdSetHighSpeed (card);
        if (rc == OK)
            {
            card->info.highSpeed = TRUE;
            card->info.tranSpeed = SDMMC_CLK_FREQ_50MHZ;

            sdMmcHostClkFreqSetup (card->host, card->info.tranSpeed);
            } 
        else if (rc != ENOTSUP)
            {
            goto err;
            }
        }

    /* check if card is write protected */

    card->isWp = sdMmcHostCardWpCheck (card->host);

    return OK;

err:
    return ERROR;
    }

/*******************************************************************************
*
* sdMmcCardIdxAlloc - allocate a global card index
*
* This routine allocates a global card index.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdMmcCardIdxAlloc
    (
    SDMMC_CARD * card
    )
    {
    UINT32 * cardIdx;
    int idx;

    if (card->isMmc)
        cardIdx = &mmcCardIdx;
    else
        cardIdx = &sdCardIdx;

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
        card->idx = idx;
        return OK;
        }
    }

/*******************************************************************************
*
* sdMmcCardIdxFree - free a global card index
*
* This routine frees a global card index.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void sdMmcCardIdxFree
    (
    SDMMC_CARD * card
    )
    {
    UINT32 * cardIdx;

    if (card->isMmc)
        cardIdx = &mmcCardIdx;
    else
        cardIdx = &sdCardIdx;

    (*cardIdx) &= ~(1 << card->idx);
    }

/*******************************************************************************
*
* sdMmcHostShow - print SD/MMC host related information
*
* This routine prints SD/MMC host related information.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void sdMmcHostShow
    (
    SDMMC_HOST * host
    )
    {
    (void)printf ("\nSD/MMC Host Information:\n");

    (void)printf ("\tCurrent Clock Frequency: %dMHz\n",
                  host->curClkFreq / 1000000);
    (void)printf ("\tCurrent Bus Width:       %s\n",
                  sdMmcBusWidth[host->curBusWidth]);
    (void)printf ("\tCapbility:               %08x\n", host->capbility);
    (void)printf ("\tSupport High Speed:      %s\n", host->highSpeed ?
                  "TRUE" : "FALSE");
    }

/*******************************************************************************
*
* sdMmcCardShow - print SD/MMC card related information
*
* This routine prints SD/MMC card related information.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void sdMmcCardShow
    (
    SDMMC_CARD * card
    )
    {
    char oem[3] = {0};
    char pnm[6] = {0};

    (void)printf ("\nSD/MMC Card Information:\n");

    /* misc information */

    (void)printf ("\tCard Type:             %s\n", card->isMmc ? "MMC" : "SD");
    (void)printf ("\tWrite Protected:       %s\n", card->isWp ? "YES" : "NO");
    (void)printf ("\tCard Index:            %d\n", card->idx);

    (void)printf ("\tVersion:               %s\n", sdMmcVersion[card->info.version]);
    (void)printf ("\tRCA:                   %04x\n", card->info.rca);
    (void)printf ("\tVoltage:               %08x\n", card->info.voltage);
    (void)printf ("\tHigh Capacity:         %s\n", card->info.highCapacity ?
            "TRUE" : "FALSE");

    /* extracted CID information */

    (void)printf ("SD/MMC Card CID Register:\n");
    (void)printf ("\tManufacturer ID:       %02x\n", card->info.cid.mid);
    memcpy (oem, card->info.cid.oid, 2);
    (void)printf ("\tOEM ID:                %s\n", oem);
    memcpy (pnm, card->info.cid.pnm, 5);
    (void)printf ("\tProduct Name:          %s\n", pnm);
    (void)printf ("\tProduct Revision:      %d.%d\n",
                  CID_PRV_MAJOR(card->info.cid.prv),
                  CID_PRV_MINOR(card->info.cid.prv));
    (void)printf ("\tProduct Serial Number: %02x%02x%02x%02x\n",
                  card->info.cid.psn[0], card->info.cid.psn[1],
                  card->info.cid.psn[2], card->info.cid.psn[3]);
    (void)printf ("\tProduct Date:          %d.%d\n",
                  CID_MDT_YEAR(card->info.cid.mdt1, card->info.cid.mdt2),
                  CID_MDT_MONTH(card->info.cid.mdt2));

    /* extracted CSD information */

    (void)printf ("SD/MMC Card CSD Register:\n");

    (void)printf ("\tTransfer Speed:        %dMHz\n",
                  card->info.tranSpeed / 1000000);
    (void)printf ("\tHigh Speed:            %s\n", card->info.highSpeed ?
                  "TRUE" : "FALSE");
    (void)printf ("\tRead Block Length:     %d\n", card->info.readBlkLen);
    (void)printf ("\tWrite Block Length:    %d\n", card->info.writeBlkLen);
    (void)printf ("\tDSR Implemented:       %s\n", card->info.hasDsr ?
                  "TRUE" : "FALSE");
    (void)printf ("\tBlock Number:          %lld\n", card->info.blkNum);
    (void)printf ("\tCapacity:              %lld\n", card->info.capacity);

    if (!card->isMmc)
        {
        /* extracted SCR information */

        (void)printf ("SD Card SCR Register:\n");
        (void)printf ("\tSpec Version:          %d\n", card->info.sdSpec);
        (void)printf ("\tSecurity Version:      %d\n", card->info.sdSec);
        (void)printf ("\tDAT 4-bit Width:       %s\n", card->info.dat4Bit ?
                      "TRUE" : "FALSE");
        }
    }
