/* vxbSdMmcLib_fm.c - Generic SD/MMC Library */

/*
 * Copyright (c) 2009-2014 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01p,11feb15,m_y  modify sdmmc_ACmd_SetHighSpeed add check action (VXW6-83917)
01o,05dec14,lxj  enable high speed mode by SD card version (VXW6-83877)
01n,12sep14,lxj  rework SD Card clock frequency is unexpectedlyi(VXW6-80641)
                 fix SD Card can not be recognized on SMP on Zynq7k (VXW6-83405)
01m,02may13,e_d  add sd version check in sdmmc_ACmd_SetHighSpeed. (WIND00413469)
01l,17dec12,e_d  add status check in sdmmc_StatusWaitReadyForData. (WIND00390089)
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
                 operation during get absTicks in function sdmmc_Identify().
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

/* #include <../src/hwif/h/storage/vxbSdMmcLib.h> */
#include "vxbSdMmcLib_fm.h"

/* defines */
/*#define SDMMC_LIB_DBG_ON*/

#ifdef  SDMMC_LIB_DBG_ON

#ifdef  LOCAL
#undef  LOCAL
#define LOCAL
#endif

#define SDMMC_LIB_DBG_CMD           0x00000001
#define SDMMC_LIB_DBG_ERR           0x00000002
#define SDMMC_LIB_DBG_ALL           0xffffffff
#define SDMMC_LIB_DBG_OFF           0x00000000

UINT32 sdMmcLibDbgMask = SDMMC_LIB_DBG_ALL; /* SDMMC_LIB_DBG_OFF */

IMPORT FUNCPTR _func_logMsg;

#define SD_DBG(mask, string, a, b, c, d, e, f)               \
    if ((sdMmcLibDbgMask & mask) || (mask == SDMMC_LIB_DBG_ALL))    \
        if (_func_logMsg != NULL) \
            (* _func_logMsg)(string, a, b, c, d, e, f)

#else
#define SD_DBG(mask, string, a, b, c, d, e, f) printf(string, a, b, c, d, e, f)
#endif  /* SDMMC_LIB_DBG_ON */

/* imports */

/* IMPORT UINT32 sysClkRateGet(void); */
IMPORT int sysClkRateGet(void);

#ifndef _WRS_CONFIG_SMP
IMPORT volatile UINT64 vxAbsTicks;
#else
IMPORT UINT64 vxAbsTicksGet (void);
#endif /* _WRS_CONFIG_SMP */

/* locals */

LOCAL STATUS sdmmc_ACmd6_SwitchFunc (SDMMC_CARD * card, void * buffer,
                                  UINT32 len, int mode, int group, int val);
LOCAL STATUS sdmmc_ACmd_SetHighSpeed (SDMMC_CARD * card);

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

static const UINT32 tran_exp2[] = 
{
    100000,         1000000,                10000000,       100000000,
    0,              0,              0,              0
};

static const UINT8 tran_mant2[] =
{
    0,      10,     12,     13,     15,     20,     25,     30,
    35,     40,     45,     50,     55,     60,     70,     80,
};

static const UINT32 taac_exp2[] = 
{
	1,	10,	100,	1000,	10000,	100000,	1000000, 10000000,
};

static const UINT32 taac_mant2[] = 
{
	0,	10,	12,	13,	15,	20,	26,	30,
	35,	40,	45,	52,	55,	60,	70,	80,
};




/* SD/MMC card dev index */

LOCAL UINT32 mmcCardIdx;
LOCAL UINT32 sdCardIdx;

#if 1
/*******************************************************************************
*
* sdmmc_Host_CmdIssue - host routine to issue the command
*
* This routine issues the command in a host.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

extern STATUS altSocGen5DwMshc_CmdIssue(SDMMC_HOST * host);

LOCAL STATUS sdmmc_Host_CmdIssue
	
    (
    SDMMC_HOST * host
    )
{
    STATUS rc;
    SDMMC_CMD *cmd = &host->cmdIssued;

    /* clear last command error */
    cmd->cmdErr = 0;
    cmd->dataErr = 0;

    /* 
    call host routine to issue the command 
	*/
#if 0
    rc = host->ops.cmdIssue (host);
#else
    rc = altSocGen5DwMshc_CmdIssue(host);
#endif
    if (rc == ERROR)
    {
        SD_DBG (SDMMC_LIB_DBG_ERR, "CMD%d: issue error\n",
                       cmd->cmdIdx, 0, 0, 0, 0 ,0);
		return ERROR;
    }
	
#if 0
    SD_DBG (SDMMC_LIB_DBG_CMD, "CMD%d: issued ok\n",
                   cmd->cmdIdx, 0, 0, 0, 0 ,0);
#endif

    return rc;
}

/*******************************************************************************
*
* sdmmc_Host_ClkFreqSetup - host routine to setup the clock frequency
*
* This routine setup the clock frequency for a host.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void sdmmc_Host_ClkFreqSetup
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
* sdmmc_Host_VddSetup - host routine to setup the sd bus power and voltage.
*
* This routine setup the sd bus power and voltage.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void sdmmc_Host_VddSetup
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
* sdmmc_Host_BusWidthSetup - host routine to setup the bus width
*
* This routine setup the bus width for a host.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void sdmmc_Host_BusWidthSetup
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
* sdmmc_Host_CardWpCheck - host routine to check if card is write protected
*
* This routine checks if card is write protected for a host.
*
* RETURNS: TRUE if card is write protected or FALSE if not.
*
* ERRNO: N/A
*/

LOCAL BOOL sdmmc_Host_CardWpCheck
    (
    SDMMC_HOST * host
    )
    {
    /* call host routine to check if card is write protected */

    return host->ops.cardWpCheck (host);
    }

#endif

#if 1
/*******************************************************************************
*
* sdmmc_Cmd0_GoIdleState - issue CMD0 to reset all cards to idle state
*
* This routine issues CMD0 to reset all cards to idle state.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Cmd0_GoIdleState(SDMMC_CARD * card)
{
    SDMMC_CMD *cmd = &card->host->cmdIssued;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;
		
    SDMMC_CMD_FORM (cmd, SDMMC_CMD0_GO_IDLE_STATE, cmdArg, SDMMC_CMD_RSP_NONE, FALSE, rspFlag);

    return sdmmc_Host_CmdIssue (card->host);
}
	

STATUS sdmmc_Cmd1_SendOpCond(SDMMC_CARD * card)
{
	SDMMC_CMD *cmd = &card->host->cmdIssued;	
	UINT32 ocr = OCR_MMC_RSP_40FF8000;  /* relative card address */
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NOCRC;

	SDMMC_CMD_FORM (cmd, MMC_CMD1_SEND_OP_COND, ocr, SDMMC_CMD_RSP_R3, FALSE, rspFlag);

	return sdmmc_Host_CmdIssue (card->host);
}
	


/*******************************************************************************
*
* sdmmc_Cmd2_AllSendCid - issue CMD2 to ask all cards to send their CIDs
*
* This routine issues CMD2 to ask all cards to send their CIDs.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Cmd2_AllSendCid
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    STATUS rc;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD2_ALL_SEND_CID, 0, SDMMC_CMD_RSP_R2, FALSE, rspFlag);

    rc = sdmmc_Host_CmdIssue (card->host);
    if (rc == ERROR)
        return ERROR;

    bcopy ((char *)cmd->cmdRsp, (char *)&(card->info.cid), sizeof(SDMMC_CID));

    return OK;
    }

/*******************************************************************************
*
* sdmmc_Cmd3_SendRelativeAddr - issue CMD3 to ask card to publish a new RCA
*
* This routine issues CMD3 to ask card to publish a new RCA.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Cmd3_SendRelativeAddr
    (
    SDMMC_CARD * card
    )
{
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    STATUS rc;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    SDMMC_CMD_FORM (cmd, SD_CMD3_SEND_RELATIVE_ADDR, 0, SDMMC_CMD_RSP_R6, FALSE, rspFlag);

    rc = sdmmc_Host_CmdIssue (card->host);
    if (rc == ERROR)
        return ERROR;

    card->info.rca = (UINT16)(cmd->cmdRsp[0] >> 16) & 0xffff;

    return OK;
}
	
STATUS sdmmc_Cmd3_SendRelativeAddr_MMC
    (
    SDMMC_CARD * card
    )
{
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    STATUS rc;

	UINT32 rca = 0;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

	/*rca = 1;*/
	/*rca = (1 << 16) & 0xFFFF0000;;*/
	rca = 0x00010000;;

    SDMMC_CMD_FORM (cmd, MMC_CMD3_SET_RELATIVE_ADDR, rca, SDMMC_CMD_RSP_R6, FALSE, rspFlag);

    rc = sdmmc_Host_CmdIssue (card->host);
    if (rc == ERROR)
        return ERROR;

    card->info.rca = (UINT16)(cmd->cmdRsp[0] >> 16) & 0xffff;

    return OK;
}

/*******************************************************************************
*
* sdmmc_Cmd4_SetDsr - issue CMD4 to program the DSR of all cards
*
* This routine issues CMD4 to program the DSR of all cards.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Cmd4_SetDsr
    (
    SDMMC_CARD *    card,
    UINT32          dsr
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD4_SET_DSR, dsr << 16, SDMMC_CMD_RSP_NONE, FALSE, rspFlag);

    return sdmmc_Host_CmdIssue (card->host);
    }

/*******************************************************************************
*
* sdmmc_Cmd7_SelectCard - issue CMD7 to select a card
*
* This routine issues CMD7 to select a card.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Cmd7_SelectCard(SDMMC_CARD * card)
{
    SDMMC_CMD *cmd = &card->host->cmdIssued;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD7_SELECT_CARD, SDMMC_CMD_ARG_RCA (card->info.rca),
                    SDMMC_CMD_RSP_R1B, FALSE, rspFlag);

    return sdmmc_Host_CmdIssue (card->host);
}
	
STATUS sdmmc_Cmd7_SelectCard_MMC(SDMMC_CARD * card)
{
    SDMMC_CMD *cmd = &card->host->cmdIssued;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD7_SELECT_CARD, SDMMC_CMD_ARG_RCA (card->info.rca),
                    SDMMC_CMD_RSP_R1, FALSE, rspFlag);

    return sdmmc_Host_CmdIssue (card->host);
}

/*******************************************************************************
*
* sdmmc_Cmd7_DeselectCard - issue CMD7 to deselect a card
*
* This routine issues CMD7 to deselect a card.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Cmd7_DeselectCard
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD7_SELECT_CARD, 0, SDMMC_CMD_RSP_R1B, FALSE, rspFlag);

    return sdmmc_Host_CmdIssue (card->host);
    }

/*******************************************************************************
*
* sdmmc_Cmd8_SendIfCond - issue CMD8 to ask card to send interface condition
*
* This routine issues CMD8 to ask card to send interface condition.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Cmd8_SendIfCond
    (
    SDMMC_CARD * card
    )
{
    SDMMC_HOST * host = card->host;
    SDMMC_CMD *cmd = &host->cmdIssued;
    UINT32 arg;
    STATUS rc;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

	arg = SD_CMD8_ARG_CHK_PATTERN;
    SDMMC_CMD_FORM (cmd, SD_CMD8_SEND_IF_COND, arg, SDMMC_CMD_RSP_R7, FALSE, rspFlag);

    rc = sdmmc_Host_CmdIssue (card->host);
    if (rc == ERROR)
    {    
    	card->info.version = SD_VERSION_100;
    }
	
	SD_DBG (SDMMC_LIB_DBG_ALL, "SendIfCond: cmdRsp[0] is 0x%08X \n",   card->idx, cmd->cmdRsp[0], 3,4,5,6);
	SD_DBG (SDMMC_LIB_DBG_ALL, "SendIfCond: cmdRsp[1] is 0x%08X \n",   card->idx, cmd->cmdRsp[1], 3,4,5,6);
	SD_DBG (SDMMC_LIB_DBG_ALL, "SendIfCond: cmdRsp[2] is 0x%08X \n",   card->idx, cmd->cmdRsp[2], 3,4,5,6);
	SD_DBG (SDMMC_LIB_DBG_ALL, "SendIfCond: cmdRsp[3] is 0x%08X \n\n", card->idx, cmd->cmdRsp[3], 3,4,5,6);

	if ((cmd->cmdRsp[0] & 0xff) == (SD_CMD8_ARG_CHK_PATTERN & 0xFF))
    {    
    	card->info.version = SD_VERSION_200;
	}
    else
    {    
    	card->info.version = SD_VERSION_100;
    }

    return OK;
}

#define MMC_SEND_EXT_CSD         (8 << 4)  /* adtc                    R1  */

STATUS sdmmc_Cmd8_MMC(SDMMC_CARD * card, UINT8 *pBuf)
{
    SDMMC_HOST * host = card->host;
    SDMMC_CMD *cmd = &host->cmdIssued;
    UINT32 arg;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_RDATA;
	
	arg = 0x00;
	
	
    SDMMC_CMD_FORM (cmd, MMC_SEND_EXT_CSD, arg, SDMMC_CMD_RSP_R7, TRUE, rspFlag);
	
	cmd->cmdData.buffer = pBuf;	
    cmd->cmdData.dataLen = 512;	
    cmd->cmdData.blkSize = 512;

    return sdmmc_Host_CmdIssue (card->host);;
}

/*******************************************************************************
*
* sdmmc_Cmd9_SendCsd - issue CMD9 to ask card to send its CSD
*
* This routine issues CMD9 to ask card to send its CSD.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Cmd9_SendCsd
    (
    SDMMC_CARD * card
    )
{
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    UINT32 csize, csizeMulti;
    STATUS rc;
    UINT64 blkNum;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD9_SEND_CSD, SDMMC_CMD_ARG_RCA (card->info.rca),
                    SDMMC_CMD_RSP_R2, FALSE, rspFlag);

    rc = sdmmc_Host_CmdIssue (card->host);
    if (rc == ERROR)
        return ERROR;

    bcopy ((char *)cmd->cmdRsp, (char *)card->info.csd, 16);

    return OK;
}


STATUS sdmmc_Cmd9_SendCsd_MMC
    (
    SDMMC_CARD * card
    )
{
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    UINT32 csize, csizeMulti;
    STATUS rc;
    UINT64 blkNum;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

	card->info.rca = 1;  /* jc*/

    SDMMC_CMD_FORM (cmd, SDMMC_CMD9_SEND_CSD, SDMMC_CMD_ARG_RCA (card->info.rca),
                    SDMMC_CMD_RSP_R2, FALSE, rspFlag);

    rc = sdmmc_Host_CmdIssue (card->host);
    if (rc == ERROR)
        return ERROR;

	
	SD_DBG (SDMMC_LIB_DBG_ALL, "ctrl_%d: csd[0] is 0x%08X \n", card->idx, card->host->cmdIssued.cmdRsp[0], 3,4,5,6);
	SD_DBG (SDMMC_LIB_DBG_ALL, "ctrl_%d: csd[1] is 0x%08X \n", card->idx, card->host->cmdIssued.cmdRsp[1], 3,4,5,6);
	SD_DBG (SDMMC_LIB_DBG_ALL, "ctrl_%d: csd[2] is 0x%08X \n", card->idx, card->host->cmdIssued.cmdRsp[2], 3,4,5,6);
	SD_DBG (SDMMC_LIB_DBG_ALL, "ctrl_%d: csd[3] is 0x%08X \n\n", card->idx, card->host->cmdIssued.cmdRsp[3], 3,4,5,6);

    bcopy ((char *)cmd->cmdRsp, (char *)card->info.csd, 16);

    return OK;
}

/*
functions
*/
int UNSTUFF_BITS2(u32 *rsp_buf, unsigned int start, unsigned int size)
{
	const unsigned int __mask = (1 << (size)) - 1;
	const int __off = /*3 - */((start) / 32);	 
	const int __shft = (start) & 31;	 
	unsigned int __res;  
	__res = rsp_buf[__off] >> __shft; 
	if ((size) + __shft >= 32)	 
		__res |= rsp_buf[__off+1] << (32 - __shft);	
	return (__res & __mask);
}


int sdmmc_decode_CSD_MMC2(SDMMC_CARD * card)
{
	UINT32 e, m;
	UINT32 * pResp;	
	
	pResp = (UINT32 *)(&card->info.csd[0]);
	
	/* mmc / eMMC */
	{
		card->info.info_csd.csd_struct = UNSTUFF_BITS2(pResp, 126, 2);
		if (card->info.info_csd.csd_struct == 0)
		{
			return ERROR;
		}

		card->info.info_csd.mmca_vsn	 = UNSTUFF_BITS2(pResp, 122, 4);
		
		m = UNSTUFF_BITS2(pResp, 115, 4);
		e = UNSTUFF_BITS2(pResp, 112, 3);
		card->info.info_csd.taac_ns	 = (taac_exp2[e] * taac_mant2[m] + 9) / 10;
		card->info.info_csd.taac_clks	 = UNSTUFF_BITS2(pResp, 104, 8) * 100;

		m = UNSTUFF_BITS2(pResp, 99, 4);
		e = UNSTUFF_BITS2(pResp, 96, 3);
		card->info.info_csd.max_dtr	  = tran_exp2[e] * tran_mant2[m]/10;		
		card->info.info_csd.cmdclass  = UNSTUFF_BITS2(pResp, 84, 12);

		e = UNSTUFF_BITS2(pResp, 47, 3);
		m = UNSTUFF_BITS2(pResp, 62, 2) | (UNSTUFF_BITS2(pResp, 64, 10) << 2);
		card->info.info_csd.blockNR	  = (1 + m) << (e + 2);
		
		card->info.info_csd.read_blkbits   = UNSTUFF_BITS2(pResp, 80, 4);
		card->info.info_csd.read_partial   = UNSTUFF_BITS2(pResp, 79, 1);
		card->info.info_csd.write_misalign = UNSTUFF_BITS2(pResp, 78, 1);
		card->info.info_csd.read_misalign  = UNSTUFF_BITS2(pResp, 77, 1);
		card->info.info_csd.r2w_factor     = UNSTUFF_BITS2(pResp, 26, 3);
		card->info.info_csd.write_blkbits  = UNSTUFF_BITS2(pResp, 22, 4);
		card->info.info_csd.write_partial  = UNSTUFF_BITS2(pResp, 21, 1);
	}
	
	return OK;
}

int sdmmc_decode_CSD_SD(SDMMC_CARD * card)
{
	UINT32 e, m;
	UINT32 * pResp;
	
	pResp = (UINT32 *)(&card->info.csd[0]);

	/*if (card->card_type == SD)*/
	{
		card->info.info_csd.csd_struct = UNSTUFF_BITS2(pResp, 126, 2);

		m = UNSTUFF_BITS2(pResp, 115, 4);
		e = UNSTUFF_BITS2(pResp, 112, 3);		
		card->info.info_csd.taac_ns	 = (taac_exp2[e] * taac_mant2[m] + 9) / 10;
		card->info.info_csd.taac_clks	 = UNSTUFF_BITS2(pResp, 104, 8) * 100;
		
		m = UNSTUFF_BITS2(pResp, 99, 4);
		e = UNSTUFF_BITS2(pResp, 96, 3);
		card->info.info_csd.max_dtr	  = tran_exp2[e] * tran_mant2[m]/10;
		card->info.info_csd.cmdclass	  = UNSTUFF_BITS2(pResp, 84, 12);
		
		if (card->info.highCapacity == 0)
		{
			e = UNSTUFF_BITS2(pResp, 47, 3);
	/*		m = UNSTUFF_BITS2(pResp, 62, 12);*/
			m = UNSTUFF_BITS2(pResp, 62, 2) | (UNSTUFF_BITS2(pResp, 64, 10) << 2);
			card->info.info_csd.blockNR	  = (1 + m) << (e + 2);
		}
		else if(card->info.highCapacity == 1)
		{
			e = UNSTUFF_BITS2(pResp, 48, 22);
			card->info.info_csd.blockNR = (e + 1)<<10;
		}
		
		card->info.info_csd.read_blkbits = UNSTUFF_BITS2(pResp, 80, 4);
		card->info.info_csd.read_partial = UNSTUFF_BITS2(pResp, 79, 1);
		card->info.info_csd.write_misalign = UNSTUFF_BITS2(pResp, 78, 1);
		card->info.info_csd.read_misalign = UNSTUFF_BITS2(pResp, 77, 1);
		card->info.info_csd.r2w_factor = UNSTUFF_BITS2(pResp, 26, 3);
		card->info.info_csd.write_blkbits = UNSTUFF_BITS2(pResp, 22, 4);
		card->info.info_csd.write_partial = UNSTUFF_BITS2(pResp, 21, 1);
	}
	
	return 0;
}

int sdmmc_decode_CID_MMC2(SDMMC_CARD * card)
{
	UINT32 * pResp;
	
	pResp = (UINT32 *)(&card->info.csd[0]);
	
	/* mmc / eMMC */
	{
		/*
		 * The selection of the format here is based upon published
		 * specs from sandisk and from what people have reported.
		 */
		switch (card->info.info_csd.mmca_vsn)
		{
			case CSD_SPEC_VER_0: /* MMC v1.0 - v1.2 */
			case CSD_SPEC_VER_1: /* MMC v1.4 */
				card->info.cid.manfid	    = UNSTUFF_BITS2(pResp, 104, 24);
				card->info.cid.prod_name[0]	= UNSTUFF_BITS2(pResp, 96, 8);
				card->info.cid.prod_name[1]	= UNSTUFF_BITS2(pResp, 88, 8);
				card->info.cid.prod_name[2]	= UNSTUFF_BITS2(pResp, 80, 8);
				card->info.cid.prod_name[3]	= UNSTUFF_BITS2(pResp, 72, 8);
				card->info.cid.prod_name[4]	= UNSTUFF_BITS2(pResp, 64, 8);
				card->info.cid.prod_name[5]	= UNSTUFF_BITS2(pResp, 56, 8);
				card->info.cid.prod_name[6]	= UNSTUFF_BITS2(pResp, 48, 8);
				card->info.cid.hwrev		= UNSTUFF_BITS2(pResp, 44, 4);
				card->info.cid.fwrev		= UNSTUFF_BITS2(pResp, 40, 4);
				card->info.cid.serial	    = UNSTUFF_BITS2(pResp, 16, 24);
				card->info.cid.month		= UNSTUFF_BITS2(pResp, 12, 4);
				card->info.cid.year		    = UNSTUFF_BITS2(pResp, 8, 4) + 1997;
				break;

			case CSD_SPEC_VER_2: /* MMC v2.0 - v2.2 */
			case CSD_SPEC_VER_3: /* MMC v3.1 - v3.3 */
			case CSD_SPEC_VER_4: /* MMC v4 */
				card->info.cid.manfid	    = UNSTUFF_BITS2(pResp, 120, 8);
				card->info.cid.oemid		= UNSTUFF_BITS2(pResp, 104, 16);
				card->info.cid.prod_name[0]	= UNSTUFF_BITS2(pResp, 96, 8);
				card->info.cid.prod_name[1]	= UNSTUFF_BITS2(pResp, 88, 8);
				card->info.cid.prod_name[2]	= UNSTUFF_BITS2(pResp, 80, 8);
				card->info.cid.prod_name[3]	= UNSTUFF_BITS2(pResp, 72, 8);
				card->info.cid.prod_name[4]	= UNSTUFF_BITS2(pResp, 64, 8);
				card->info.cid.prod_name[5]	= UNSTUFF_BITS2(pResp, 56, 8);
				card->info.cid.serial	    = UNSTUFF_BITS2(pResp, 16, 32);
				card->info.cid.month		= UNSTUFF_BITS2(pResp, 12, 4);
				card->info.cid.year		    = UNSTUFF_BITS2(pResp, 8, 4) + 1997;
				break;

			default:
				return ERROR;
		}
	}
	
	return OK;
}


int sdmmc_decode_CID_SD(SDMMC_CARD * card)
{
	UINT32 * pResp;
	
	pResp = (UINT32 *)(&card->info.csd[0]);
	
	/*if (pCard->card_type == SD)*/
	{
		/*
		 * SD doesn't currently have a version field so we will
		 * have to assume we can parse this.
		 */
		card->info.cid.manfid		= UNSTUFF_BITS2(pResp, 120, 8);
		card->info.cid.oemid		= UNSTUFF_BITS2(pResp, 104, 16);
		card->info.cid.prod_name[0]	= UNSTUFF_BITS2(pResp, 96, 8);
		card->info.cid.prod_name[1]	= UNSTUFF_BITS2(pResp, 88, 8);
		card->info.cid.prod_name[2]	= UNSTUFF_BITS2(pResp, 80, 8);
		card->info.cid.prod_name[3]	= UNSTUFF_BITS2(pResp, 72, 8);
		card->info.cid.prod_name[4]	= UNSTUFF_BITS2(pResp, 64, 8);
		card->info.cid.hwrev		= UNSTUFF_BITS2(pResp, 60, 4);
		card->info.cid.fwrev		= UNSTUFF_BITS2(pResp, 56, 4);
		card->info.cid.serial		= UNSTUFF_BITS2(pResp, 24, 32);
		card->info.cid.year			= UNSTUFF_BITS2(pResp, 12, 8);
		card->info.cid.month		= UNSTUFF_BITS2(pResp, 8, 4);

		card->info.cid.year += 2000; /* SD cards year offset */
	}
	
	return 0;
}

int sdmmcGet_Info_CSD2(SDMMC_CARD * card) 
{
	UINT32 * pResp;
	
	pResp = (UINT32 *)(&card->info.csd[0]);
	
	/* read/write block length */
	card->read_blk_len	= 1 << card->info.info_csd.read_blkbits;
	card->write_blk_len = 1 << card->info.info_csd.write_blkbits;

	if (card->read_blk_len != card->write_blk_len)
	{
		return (ERROR);
	}

	/* partial block read/write I/O support */
	if (card->info.info_csd.read_partial) 
		card->status |= MMC_READ_PART;
	
	if (card->info.info_csd.write_partial) 
		card->status |= MMC_WRITE_PART;

	/* calculate total card size in bytes */
    if (card->highCapacity == 1)
	{	
		card->capacity = 0xFFFFFFFF; /* SDHC/SDXC may larger than 32bits*/
    }
	else
	{	
		card->capacity = card->info.info_csd.blockNR * card->read_blk_len;
	}

	return 0;
}


int sdmmcGet_Info_SCR2(SDMMC_CARD * card)
{
    int rc;
	int i = 0;
    UINT32 scr_struct;

    /*memset(card->info.scr, 0, (8*izeof(UINT8)));*/
	
    /*rc = sdmmc_sdACMD51(pSdmmcCard->raw_scr);*/
	rc = sdmmc_ACmd51_SendScr (card);
    if (rc == ERROR)
    {
        return ERROR;
    }

	SD_DBG (SDMMC_LIB_DBG_ALL, "scr is: 0x", 1,2,3,4,5,6);
	for (i=0; i<=7; i++)
	{
		SD_DBG (SDMMC_LIB_DBG_ALL, "%02X_", card->info.scr[i], 2,3,4,5,6);
	}
    SD_DBG (SDMMC_LIB_DBG_ALL, " \n", 1,2,3,4,5,6);

    scr_struct = (card->info.scr[0] >> 4) & 0xF;
    if (scr_struct != 0)
    {
        return ERROR;
    }
	
    card->info.info_scr.sd_spec   = card->info.scr[0] & 0xF;
    card->info.info_scr.bus_width = card->info.scr[1] & 0xF;
    card->info.info_scr.sd_spec3  = (card->info.scr[2] >> 7) & 0x1;
    
    return OK;
}

int sdmmcGet_ExtCsd_MMC2(SDMMC_CARD * card)
{
    int ret;
	int retry = 5;
	
    if (card->info.info_csd.mmca_vsn >= CSD_SPEC_VER_4)
    {
        /* get EXT CSD info */
        retry = 5;
        
        while (retry)
        {
            memset(card->ext_csd, 0, (512*sizeof(UINT8)));
			
            ret = sdmmc_Cmd8_MMC(card, card->ext_csd);
            if (ret < 0)
            {
                retry--;
                if (retry <= 0)
                {    
                	return ret;
                }
            }
            break;
        }
    }

    return 0;
}

/*******************************************************************************
*
* sdmmc_Cmd10_SendCid - issue CMD10 to ask card to send its CID
*
* This routine issues CMD10 to ask card to send its CID.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Cmd10_SendCid(SDMMC_CARD * card)
{
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    STATUS rc;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD10_SEND_CID, SDMMC_CMD_ARG_RCA (card->info.rca),
                    SDMMC_CMD_RSP_R2, FALSE, rspFlag);

    rc = sdmmc_Host_CmdIssue (card->host);
    if (rc == ERROR)
        return ERROR;

    bcopy ((char *)cmd->cmdRsp, (char *)&(card->info.cid), sizeof(SDMMC_CID));

    return OK;
}

/*
#define MMC_HIGH_SPEED_ARG  (((u32)MMC_EXT_CSD_WRITE_BYTE << 24) \
                              | ((u32)MMC_EXT_CSD_HS_TIMING_BYTE << 16) \
                              | ((u32)MMC_EXT_CSD_HS_TIMING_HIGH << 8))
*/
#define MMC_HIGH_SPEED_ARG_2  (((UINT32)3 << 24) | ((UINT32)185 << 16) | ((UINT32)1 << 8))

LOCAL STATUS sdmmc_Cmd6_SwitchFunc(SDMMC_CARD * card)
{
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    STATUS rc;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    SDMMC_CMD_FORM (cmd, MMC_CMD6_SWITCH, MMC_HIGH_SPEED_ARG_2, SDMMC_CMD_RSP_R2, FALSE, rspFlag);

    rc = sdmmc_Host_CmdIssue (card->host);
    if (rc == ERROR)
        return ERROR;

    return OK;
}

STATUS sdmmc_Cmd6_SD(SDMMC_CARD * card, UINT32 arg, UINT8 *pBuf)
{
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    STATUS rc;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_RDATA;
	
    cmd->cmdData.buffer = (UINT8 *)pBuf;
	
    cmd->cmdData.dataLen = 512 / 8;
    cmd->cmdData.blkSize = 512 / 8;

    SDMMC_CMD_FORM (cmd, CMD_SWITCH_FUNC, arg, SDMMC_CMD_RSP_R1, TRUE, rspFlag);

    rc = sdmmc_Host_CmdIssue (card->host);
    if (rc == ERROR)
        return ERROR;

    return OK;
}

#define MMC_4_BIT_BUS_ARG_2    (((UINT32)3 << 24) | ((UINT32)183 << 16) | ((UINT32)1 << 8))

LOCAL STATUS sdmmc_Cmd6_MMC(SDMMC_CARD * card)
{
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    STATUS rc;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    SDMMC_CMD_FORM (cmd, MMC_CMD6_SWITCH, MMC_4_BIT_BUS_ARG_2, SDMMC_CMD_RSP_R1, FALSE, rspFlag);

    rc = sdmmc_Host_CmdIssue (card->host);
    if (rc == ERROR)
        return ERROR;

    return OK;
}

/*******************************************************************************
*
* sdmmc_Cmd12_StopTransmission - issue CMD12 to force card to stop transmission
*
* This routine issues CMD12 to force card to stop transmission.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Cmd12_StopTransmission
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD12_STOP_TRANSMISSION, 0, SDMMC_CMD_RSP_R1B, FALSE, rspFlag);

    return sdmmc_Host_CmdIssue (card->host);
    }

/*******************************************************************************
*
* sdmmc_Cmd13_SendStatus - issue CMD13 to ask card to send its status
*
* This routine issues CMD13 to ask card to send its status.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Cmd13_SendStatus
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    STATUS rc;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD13_SEND_STATUS, SDMMC_CMD_ARG_RCA (card->info.rca),
                    SDMMC_CMD_RSP_R1, FALSE, rspFlag);

    rc = sdmmc_Host_CmdIssue (card->host);
    if (rc == ERROR)
        return ERROR;

    card->info.cardSts = cmd->cmdRsp[0];

    return OK;
    }

/*******************************************************************************
*
* sdmmc_Cmd15_GoInactiveState - issue CMD15 to put card into the inactive state
*
* This routine issues CMD15 to put card into the inactive state.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Cmd15_GoInactiveState
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD15_GO_INACTIVE_STATE,
                    SDMMC_CMD_ARG_RCA (card->info.rca),
                    SDMMC_CMD_RSP_NONE, FALSE, rspFlag);

    return sdmmc_Host_CmdIssue (card->host);
    }

/*******************************************************************************
*
* sdmmc_Cmd16_SetBlockLen - issue CMD16 to set block length for block commands
*
* This routine issues CMD16 to set block length for block commands.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Cmd16_SetBlockLen
    (
    SDMMC_CARD * card
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    /* block length is always 512 bytes */

    SDMMC_CMD_FORM (cmd, SDMMC_CMD16_SET_BLOCKLEN, SDMMC_BLOCK_SIZE,
                    SDMMC_CMD_RSP_R1, FALSE, rspFlag);

    return sdmmc_Host_CmdIssue (card->host);
    }

/*******************************************************************************
*
* sdmmc_Cmd17_ReadSingleBlock - issue CMD17 to read a single block
*
* This routine issues CMD17 to read a single block.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Cmd17_ReadSingleBlock
    (
    SDMMC_CARD *    card,
    void *          buf,
    UINT64          blkNo
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    UINT32 arg;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    if (card->info.highCapacity)
        arg = (UINT32)blkNo;
    else
        arg = (UINT32)blkNo * SDMMC_BLOCK_SIZE;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD17_READ_SINGLE_BLOCK, arg, SDMMC_CMD_RSP_R1, TRUE, rspFlag);
    SDMMC_DATA_FORM (cmd, buf, 1, SDMMC_BLOCK_SIZE, TRUE, SDMMC_BLOCK_SIZE);

    return sdmmc_Host_CmdIssue (card->host);
    }

/*******************************************************************************
*
* sdmmc_Cmd18_ReadMultipleBlock - issue CMD18 to read multiple blocks
*
* This routine issues CMD18 to read multiple blocks.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Cmd18_ReadMultipleBlock
    (
    SDMMC_CARD *    card,
    void *          buf,
    UINT64          blkNo,
    UINT32          numBlks
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    UINT32 arg;
	
	UINT32 cmdArg = 0, rspFlag = 0, dataLen = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    if (card->info.highCapacity)
        arg = (UINT32)blkNo;
    else
        arg = (UINT32)blkNo * SDMMC_BLOCK_SIZE;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD18_READ_MULTI_BLOCK, arg, SDMMC_CMD_RSP_R1, TRUE, rspFlag);
	dataLen = numBlks * SDMMC_BLOCK_SIZE;
    SDMMC_DATA_FORM (cmd, buf, numBlks, SDMMC_BLOCK_SIZE, TRUE, dataLen);

    return sdmmc_Host_CmdIssue (card->host);
    }

/*******************************************************************************
*
* sdmmc_Cmd_ReadBlock - issue CMD17 or CMD18 to read blocks
*
* This routine issues CMD17 or CMD18 to read blocks.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Cmd_ReadBlock
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
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    if (numBlks > 1)
        idx = SDMMC_CMD18_READ_MULTI_BLOCK;
    else
        idx = SDMMC_CMD17_READ_SINGLE_BLOCK;

    if (card->info.highCapacity)
        arg = (UINT32)blkNo;
    else
        arg = (UINT32)blkNo * SDMMC_BLOCK_SIZE;

    SDMMC_CMD_FORM (cmd, idx, arg, SDMMC_CMD_RSP_R1, TRUE, rspFlag);
    SDMMC_DATA_FORM (cmd, buf, numBlks, SDMMC_BLOCK_SIZE, TRUE, SDMMC_BLOCK_SIZE);

    return sdmmc_Host_CmdIssue (card->host);
    }

/*******************************************************************************
*
* sdmmc_Cmd24_WriteSingleBlock - issue CMD24 to write a single block
*
* This routine issues CMD24 to write a single block.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Cmd24_WriteSingleBlock
    (
    SDMMC_CARD *    card,
    void *          buf,
    UINT64          blkNo
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    UINT32 arg;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    if (card->info.highCapacity)
        arg = (UINT32)blkNo;
    else
        arg = (UINT32)blkNo * SDMMC_BLOCK_SIZE;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD24_WRITE_SINGLE_BLOCK, arg, SDMMC_CMD_RSP_R1, TRUE, rspFlag);
	
    SDMMC_DATA_FORM (cmd, buf, 1, SDMMC_BLOCK_SIZE, FALSE, SDMMC_BLOCK_SIZE);

    return sdmmc_Host_CmdIssue (card->host);
    }

/*******************************************************************************
*
* sdmmc_Cmd25_WriteMultipleBlock - issue CMD25 to write multiple blocks
*
* This routine issues CMD25 to writ multiple blocks.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Cmd25_WriteMultipleBlock
    (
    SDMMC_CARD *    card,
    void *          buf,
    UINT64          blkNo,
    UINT32          numBlks
    )
    {
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    UINT32 arg;
	
	UINT32 cmdArg = 0, rspFlag = 0, dataLen = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    if (card->info.highCapacity)
        arg = (UINT32)blkNo;
    else
        arg = (UINT32)blkNo * SDMMC_BLOCK_SIZE;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD25_WRITE_MULTI_BLOCK, arg, SDMMC_CMD_RSP_R1, TRUE, rspFlag);
	
	dataLen = numBlks * SDMMC_BLOCK_SIZE;
    SDMMC_DATA_FORM (cmd, buf, numBlks, SDMMC_BLOCK_SIZE, FALSE, dataLen);

    return sdmmc_Host_CmdIssue (card->host);
    }

/*******************************************************************************
*
* sdmmc_Cmd_WriteBlock - issue CMD24 or CMD25 to write blocks
*
* This routine issues CMD24 or CMD25 to write blocks.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Cmd_WriteBlock
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
	
	UINT32 cmdArg = 0, rspFlag = 0, dataLen = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    if (numBlks > 1)
        idx = SDMMC_CMD25_WRITE_MULTI_BLOCK;
    else
        idx = SDMMC_CMD24_WRITE_SINGLE_BLOCK;

    if (card->info.highCapacity)
        arg = (UINT32)blkNo;
    else
        arg = (UINT32)blkNo * SDMMC_BLOCK_SIZE;

    SDMMC_CMD_FORM (cmd, idx, arg, SDMMC_CMD_RSP_R1, TRUE, rspFlag);
	dataLen = numBlks * SDMMC_BLOCK_SIZE;
    SDMMC_DATA_FORM (cmd, buf, numBlks, SDMMC_BLOCK_SIZE, FALSE, dataLen);

    return sdmmc_Host_CmdIssue (card->host);
    }

/*******************************************************************************
*
* sdmmc_Cmd55_AppCmd - issue CMD55 to indicate an application command followed
*
* This routine issues CMD55 to indicate an application command followed.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Cmd55_AppCmd(SDMMC_CARD * card)
{
    SDMMC_CMD *cmd = &card->host->cmdIssued;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    SDMMC_CMD_FORM (cmd, SDMMC_CMD55_APP_CMD, SDMMC_CMD_ARG_RCA (card->info.rca),
                    SDMMC_CMD_RSP_R1, FALSE, rspFlag);

    return sdmmc_Host_CmdIssue (card->host);
}
	
STATUS sdmmc_Cmd55_MMC(SDMMC_CARD * card)
{
    SDMMC_CMD *cmd = &card->host->cmdIssued;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

	card->info.rca = 0; /* jc*/
	
    SDMMC_CMD_FORM (cmd, SDMMC_CMD55_APP_CMD, SDMMC_CMD_ARG_RCA (card->info.rca),
                    SDMMC_CMD_RSP_R1, FALSE, rspFlag);

    return sdmmc_Host_CmdIssue (card->host);
}

int sdmmcGet_BusSpeed_SD(SDMMC_CARD * card, UINT8 * pBuf)
{
    int ret;
    UINT32 arg;

    arg = SD_SWITCH_CMD_HS_GET;
    ret = sdmmc_Cmd6_SD(card, arg, pBuf);

    return ret;
}


int sdmmc_change_BusSpeed_SD(SDMMC_CARD * card, UINT32 speed)
{
    int rc;
    UINT8 readBuf[64] = {0};
    UINT32 arg;

    arg = SD_SWITCH_CMD_HS_SET;
	 
    rc = sdmmc_Cmd6_SD(card, arg, readBuf);	
    if (rc == ERROR)
    {
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc_Cmd6_SD fail! \n", 1,2,3,4,5,6);    	
        return rc;
    }
    
    /*sysMshcClkFreqSetup(SDMMC_CLK_FREQ_25MHZ);*/
    sysMshcClkFreqSetup(speed);

    return rc;
}


int sdmmc_change_BusSpeed_MMC2(SDMMC_CARD * card, UINT32 speed)
{
    int rc;
    
    rc = sdmmc_Cmd6_MMC(card);	
    if (rc == ERROR)
    {
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc_Cmd6_MMC fail! \n", 1,2,3,4,5,6);    	
        return rc;
    }
    
    /*sysMshcClkFreqSetup(SDMMC_CLK_FREQ_25MHZ);*/
    sysMshcClkFreqSetup(speed);

    return rc;
}


#endif

#if 1
/*******************************************************************************
*
* sdmmc_ACmd6_SetBusWidth - issue ACMD6 to set the bus width for data transfer
*
* This routine issues ACMD6 to set the bus width for data transfer.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_ACmd6_SetBusWidth(SDMMC_CARD *    card, UINT32          width)
{
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    UINT32 arg;
    STATUS rc;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;

    rc = sdmmc_Cmd55_AppCmd (card);
    if (rc == ERROR)
        return ERROR;

    if (width == SDMMC_BUS_WIDTH_1BIT)
        arg = SD_ACMD6_ARG_1BIT_BUS;
    else
        arg = SD_ACMD6_ARG_4BIT_BUS;

    SDMMC_CMD_FORM (cmd, SD_ACMD6_SET_BUS_WIDTH, arg, SDMMC_CMD_RSP_R1, FALSE, rspFlag);

    return sdmmc_Host_CmdIssue (card->host);
}


STATUS sdmmc_ACmd6_SetBusWidth_MMC(SDMMC_CARD *    card, UINT32          width)
{
    int ret;
    int retry = 5;
    VXB_DEVICE_ID  pDev = card->host->pDev;
	
    if (card->info.info_csd.mmca_vsn >= CSD_SPEC_VER_4)
    {
        while (retry)
        {
            ret = sdmmc_Cmd6_MMC(card);
            if (ret == ERROR)
            {                
                retry--;					
				SD_DBG (SDMMC_LIB_DBG_ALL, "retry to MMC set buswidth \n", 1,2,3,4,5,6);
            }
            else
            {
                break;
            }
        }
		
        if (ret == ERROR)
        {
			SD_DBG (SDMMC_LIB_DBG_ALL, "ACMD6_mmc fail! \n", 1,2,3,4,5,6);
            return ERROR;
        }		
		else
		{
			SD_DBG (SDMMC_LIB_DBG_ALL, "ACMD6_mmc ok! \n", 1,2,3,4,5,6);
		}
    }
	
    switch (width)
    {       
    case SDMMC_BUS_WIDTH_4BIT:
       CSR_WRITE_4 (pDev, ALT_MSHC_CTYPE, 0x1);
		SD_DBG (SDMMC_LIB_DBG_ALL, "set SD_BUS_WIDTH_4 ok! \n", 1,2,3,4,5,6);
    	break;  
	
    case SDMMC_BUS_WIDTH_1BIT:
		CSR_WRITE_4 (pDev, ALT_MSHC_CTYPE, 0x0);
		SD_DBG (SDMMC_LIB_DBG_ALL, "set SD_BUS_WIDTH_1 ok! \n", 1,2,3,4,5,6);
    	break;  
	
    default:
		CSR_WRITE_4 (pDev, ALT_MSHC_CTYPE, 0x0);
		SD_DBG (SDMMC_LIB_DBG_ALL, "set SD_BUS_WIDTH_1(default) ok! \n", 1,2,3,4,5,6);
    	break;
    }   

    return OK;
}

int sdmmcSet_CardBus_Width_SD(SDMMC_CARD *    card, UINT32          width)
{
    int ret;
    int retry = 5;
    VXB_DEVICE_ID  pDev = card->host->pDev;
    
    /*if (pSdmmcCard->card_type == SD)*/
    {
        while (retry)
        {
            ret = sdmmc_ACmd6_SetBusWidth(card, width);   /* SDMMC_BUS_WIDTH_4BIT */
            if (ret == ERROR)
            {                
                retry--;				
				/*SD_DBG (SDMMC_LIB_DBG_ALL, "retry to SD set buswidth \n", 1,2,3,4,5,6);*/
            }
            else
            {
                break;
            }
        }
		
        if (ret == ERROR)
        {
			SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc_ACmd6_SetBusWidth fail! \n", 1,2,3,4,5,6);
            return ERROR;
        }
    }
	
    switch (width)
    {       
    case SDMMC_BUS_WIDTH_4BIT:
        CSR_WRITE_4 (pDev, ALT_MSHC_CTYPE, 0x1);
		SD_DBG (SDMMC_LIB_DBG_ALL, "set SD_BUS_WIDTH_4 ok! \n", 1,2,3,4,5,6);
    	break;  
	
    case SDMMC_BUS_WIDTH_1BIT:
		CSR_WRITE_4 (pDev, ALT_MSHC_CTYPE, 0x0);
		SD_DBG (SDMMC_LIB_DBG_ALL, "set SD_BUS_WIDTH_1 ok! \n", 1,2,3,4,5,6);
    	break;  
	
    default:
		CSR_WRITE_4 (pDev, ALT_MSHC_CTYPE, 0x0);
		SD_DBG (SDMMC_LIB_DBG_ALL, "set SD_BUS_WIDTH_1(default) ok! \n", 1,2,3,4,5,6);
    	break;
    }   

    return OK;
}


/*******************************************************************************
*
* sdmmc_ACmd41_SendOpCond - issue ACMD41 to ask card to send its OCR
*
* This routine issues ACMD41 to ask card to send its OCR.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_ACmd41_SendOpCond
    (
    SDMMC_CARD * card
    )
{
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    UINT32 arg;
    STATUS rc;

	UINT32 ocr = 0;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NOCRC;

    rc = sdmmc_Cmd55_AppCmd (card);  /* cmd55*/
    if (rc == ERROR)
        return ERROR;

	ocr = 0x00FF8000;
	
    if (card->info.version == SD_VERSION_100)
    {
    	arg = ocr;
    }
	else
	{
		arg = ocr | 0x40000000;
	}
	
    SDMMC_CMD_FORM (cmd, SD_ACMD41_SEND_OP_COND, arg, SDMMC_CMD_RSP_R3, FALSE, rspFlag);

    rc = sdmmc_Host_CmdIssue (card->host);
    if (rc == OK)
    {
        card->info.ocr = cmd->cmdRsp[0];
        return OK;
    }
	else
	{
		SD_DBG (SDMMC_LIB_DBG_ALL, "appCmd41(ask card COND) fail! \n", 1,2,3,4,5,6);
    	return ERROR;
	}
}

/*******************************************************************************
*
* sdmmc_ACmd51_SendScr - issue ACMD51 to ask card to send its SCR
*
* This routine issues ACMD51 to ask card to send its SCR.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_ACmd51_SendScr(SDMMC_CARD * card)
{
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    /*SD_SCR *scr;*/
    STATUS rc;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_RDATA;

    rc = sdmmc_Cmd55_AppCmd (card);  /* cmd55*/
    if (rc == ERROR)
        return ERROR;

   /* scr = cacheDmaMalloc (sizeof(SD_SCR));*/
   /* if (scr == NULL)*/
   /*     return ERROR;*/

	
    SDMMC_CMD_FORM (cmd, SD_ACMD51_SEND_SCR, 0, SDMMC_CMD_RSP_R1, TRUE, rspFlag);
   /* SDMMC_DATA_FORM (cmd, card->info.scr, 1, sizeof(SD_SCR), TRUE, 0);*/
    SDMMC_DATA_FORM (cmd, card->info.scr, 1, 8, TRUE, 8);

    rc = sdmmc_Host_CmdIssue (card->host);
    if (rc == ERROR)
    {
        /*(void)cacheDmaFree ((char *scr);*/
        return ERROR;
    }

#if 0
    /* save SCR */
    bcopy ((char *)scr, (char *)&(card->info.scr), sizeof(SD_SCR));

    card->info.sdSpec = scr->spec & 0x0f;

    /* update the SD card version */
    card->info.version = (UINT16)card->info.sdSpec;

    card->info.sdSec = (scr->config >> 4) & 0x7;
    if (scr->config & SCR_SD_BUS_WIDTH_4BIT)
        card->info.dat4Bit = TRUE;
#else
    /* save SCR */
    /*bcopy ((char *scr, (char *&(card->info.scr), sizeof(SD_SCR));*/

#endif

   /* (void)cacheDmaFree ((char *scr);*/

    return OK;
}



/*******************************************************************************
*
* sdmmc_ACmd_SetHighSpeed - put card into high speed mode 
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

LOCAL STATUS sdmmc_ACmd_SetHighSpeed
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
    ret = sdmmc_ACmd6_SwitchFunc (card, buffer, 64,
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
        if ((buffer[16] & 0x0F) != 0)
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
        if (((buffer[28] & 0x02) != 0) ||((buffer[16] & 0x0F) != 0))
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

    ret = sdmmc_ACmd6_SwitchFunc (card, buffer, 64,
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
* sdmmc_ACmd6_SwitchFunc - send application specific command 6 
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

LOCAL STATUS sdmmc_ACmd6_SwitchFunc 
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
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_NONE;
    
    arg = mode;
    arg |= 0xffffff;
    arg &= ~(0xf << (group * 4));
    arg |= (val << (group * 4)); 

    SDMMC_CMD_FORM (cmd, SD_CMD6_SWITCH_FUNC, arg, SDMMC_CMD_RSP_R1, TRUE, rspFlag);
    SDMMC_DATA_FORM (cmd, buffer, 1, len, TRUE, 0);

    rc = sdmmc_Host_CmdIssue (card->host);
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
* sdmmc_StatusWaitReadyForData - check SD/MMC card ready for data status
*
* This routine checks SD/MMC card ready for data status.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_StatusWaitReadyForData
    (
    SDMMC_CARD * card
    )
{
    STATUS rc;

    while (1)
    {
        rc = sdmmc_Cmd13_SendStatus (card);
        if (rc == ERROR)
            return ERROR;

        if (card->info.cardSts & CARD_STS_READY_FOR_DATA)
            break;
    }

	return OK;
}	

#endif


UINT32 sdmmcGet_CardSize_KB2(SDMMC_CARD * card)
{
    if (card->capacity == 0xFFFFFFFF)
    {
        return card->info.info_csd.blockNR / 2;
    }
    
    return card->capacity / 1024;
}

/*******************************************************************************
*
* sdmmc_BlkRead - read block from SD/MMC card
*
* This routine reads block from SD/MMC card.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_BlkRead
    (
    SDMMC_CARD *    card,
    sector_t        blkNo,
    UINT32          numBlks,
    void *          pBuf
    )
    {
    STATUS rc;

    return sdmmc_Cmd_ReadBlock (card, pBuf, blkNo, numBlks);
    }

/*******************************************************************************
*
* sdmmc_BlkWrite - write block to SD/MMC card
*
* This routine writes block to SD/MMC card.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_BlkWrite
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
    return sdmmc_Cmd_WriteBlock (card, pBuf, blkNo, numBlks);
    }

/*******************************************************************************
*
* sdmmc_Identify - identify SD/MMC card in the system
*
* This routine implements the card identify process according to the SD spec.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_Identify
    (
    SDMMC_CARD * card
    )
{
    STATUS rc;
    UINT64 start, end;
    UINT32 workSpeed;
	
    int retry = 20000;

	SD_DBG (SDMMC_LIB_DBG_ALL, "\n\n ---sdmmc_Identify:--- \n", 0, 0, 0, 0, 0, 0);
	
	/* jc*/
	if (card->host->pDev->unitNumber == 0)
	{
		clk_switch_sdmmc_0();
	}
	else if (card->host->pDev->unitNumber == 1)
	{
		clk_switch_sdmmc_1();
	}

    /* power up SD bus */
    sdmmc_Host_VddSetup (card->host, card->host->vdd);

    /* set default host clock frequency and bus width */
    sdmmc_Host_ClkFreqSetup (card->host, SDMMC_CLK_FREQ_400KHZ);
	
    sdmmc_Host_BusWidthSetup (card->host, SDMMC_BUS_WIDTH_1BIT);  /* for snd cmd of all kinds*/
    /*sdmmc_Host_BusWidthSetup (card->host, SDMMC_BUS_WIDTH_4BIT);*/

    /* ensure card infomration gets cleared initially */
    bzero ((char *)&(card->info), sizeof(SDMMC_INFO));


    /* 
    CMD0: reset all cards on the bus, go IDLE state 
	*/
    rc = sdmmc_Cmd0_GoIdleState (card);
    if (rc == ERROR)
        goto err;

    /* 
    CMD8: request card to send interface condition 
    */
    rc = sdmmc_Cmd8_SendIfCond (card);
    if (rc == ERROR)
        goto err;

    /* 
    AMD41: request card to send its OCR
    */
    
#ifdef _WRS_CONFIG_SMP
    start = vxAbsTicksGet();
#else
    start = vxAbsTicks;
#endif /* _WRS_CONFIG_SMP */

    /*while (1)*/
    do
    {
    	/* 
    	ACMD41: to ask card to send its OCR 
		*/
        rc = sdmmc_ACmd41_SendOpCond (card);
        if (rc == OK)
        {    
        	break;
        }
#if 0
#ifdef _WRS_CONFIG_SMP
        end = vxAbsTicksGet();
#else
        end = vxAbsTicks;
#endif /* _WRS_CONFIG_SMP */

        /* stardard says 1s for timeout, here we give it 2s */
        if ((end - start) > (UINT64)(sysClkRateGet() * 2))
           {
           SD_DBG (SDMMC_LIB_DBG_ERR, "ACMD41 timeout\n",
                          0, 0, 0, 0, 0, 0);
           goto err;
           }
#endif
		
		retry--;
		if (retry <= 0)
		{
			return ERROR;
		}
    }while (rc == ERROR);


    /* 
    CMD2: request card to send CID and put it in identify mode 
	*/
    rc = sdmmc_Cmd2_AllSendCid (card);
    if (rc == ERROR)
        goto err;

    /* 
    CMD3: request card to send RCA 
    */
    do
    {
        rc = sdmmc_Cmd3_SendRelativeAddr (card);
        if (rc == ERROR)
            goto err;
    } while (card->info.rca == 0);

    /* 
    CMD9: request card to send CSD 
	*/
    rc = sdmmc_Cmd9_SendCsd (card);
    if (rc == ERROR)
        goto err;

    /* After Card Identification, the station go into Data Transfer Mode */
    if (card->info.tranSpeed > SDMMC_CLK_FREQ_400KHZ)
    {
        workSpeed = card->info.tranSpeed < SDMMC_CLK_FREQ_25MHZ ?
                    card->info.tranSpeed : SDMMC_CLK_FREQ_25MHZ;

        sdmmc_Host_ClkFreqSetup (card->host, workSpeed);
    }
    
    card->info.highSpeed = FALSE;

    /* 
    CMD7: select one card and put it into transfer state 
    */
    rc = sdmmc_Cmd7_SelectCard (card);
    if (rc == ERROR)
        goto err;

    /* 
    AMD51: request card to send its SCR 
	*/
    rc = sdmmc_ACmd51_SendScr (card);
    if (rc == ERROR)
    {
       /* 
        * Wordaround for APM86290's SDHC, ACMD51 command needed to be
        * issued twice to get correct value of SD card's OCR register
        */
        if (sdmmc_ACmd51_SendScr (card) == ERROR)
           goto err;
    }

    /* CMD16: set block len */
    if (card->info.highCapacity == FALSE)
    {
        rc = sdmmc_Cmd16_SetBlockLen (card);
        if (rc == ERROR)
            return ERROR;
    }

    /* switch to 4 bit mode if needed */
    if (card->info.dat4Bit)
    {
        rc = sdmmc_ACmd6_SetBusWidth (card, SDMMC_BUS_WIDTH_4BIT);
        if (rc == ERROR)
            goto err;

        /* setup host to enable 4-bit bus width */
        sdmmc_Host_BusWidthSetup (card->host, SDMMC_BUS_WIDTH_4BIT);
    }

    /* setup host to enable high speed clock (50 MHz) if needed */
#if 1  /* jc  */
    if (card->info.sdSpec >= SD_VERSION_110)
    {
        rc = sdmmc_ACmd_SetHighSpeed (card);
        if (rc == OK)
        {
            card->info.highSpeed = TRUE;
            /*card->info.tranSpeed = SDMMC_CLK_FREQ_50MHZ;  // origin */
            card->info.tranSpeed = SDMMC_CLK_FREQ_25MHZ;

            sdmmc_Host_ClkFreqSetup (card->host, card->info.tranSpeed);
        } 
        else if (rc != ENOTSUP)
        {
            goto err;
        }
    }
#endif

    /* check if card is write protected */
    card->isWp = sdmmc_Host_CardWpCheck (card->host);

    return OK;

err:
    return ERROR;
}



STATUS sdmmc_Identify_2
    (
    SDMMC_CARD * card
    )
{
    STATUS rc;
    UINT64 start, end;
    UINT32 workSpeed;
	
    int retry = 20000;

	extern int g_sdmmc_ctrl;
	SD_DBG (SDMMC_LIB_DBG_ALL, "\n\n ---sdmmc_Identify(sd:%d):--- \n\n", g_sdmmc_ctrl, 0, 0, 0, 0, 0);
	taskDelay(1);
	
	/* jc*/
	if (card->host->pDev->unitNumber == 0)
	{
		clk_switch_sdmmc_0();
	}
	else if (card->host->pDev->unitNumber == 1)
	{
		clk_switch_sdmmc_1();
	}
	
    /* power up SD bus */
    sdmmc_Host_VddSetup (card->host, card->host->vdd);

    /* set default host clock frequency and bus width */
    sdmmc_Host_ClkFreqSetup (card->host, SDMMC_CLK_FREQ_400KHZ);
	
    sdmmc_Host_BusWidthSetup (card->host, SDMMC_BUS_WIDTH_1BIT);  /* for snd cmd of all kinds*/

    /* ensure card infomration gets cleared initially */
    bzero ((char *)&(card->info), sizeof(SDMMC_INFO));

    /* 
    CMD0: reset all cards on the bus, go IDLE state 
	*/
    rc = sdmmc_Cmd0_GoIdleState (card);
    if (rc == ERROR)
    {    
    	goto err;
    }
	else
	{
		SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd0 ok- \n\n", 0, 0, 0, 0, 0, 0);
	}
	

    /* 
    CMD8: request card to send interface condition 
    */
    rc = sdmmc_Cmd8_SendIfCond (card);
    if (rc == ERROR)
    {    
    	goto err;	
    }
	else
	{
		SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd8 ok(version:%d)- \n\n", card->info.version, 0, 0, 0, 0, 0);
	}

    /* 
    AMD41: request card to send its OCR
    */
    
    /* 
    APP_CMD: cmd55 
	*/
    /*rc = sdmmc_Cmd55_AppCmd(card);	*/
    rc = sdmmc_Cmd55_MMC(card);	
	SD_DBG (SDMMC_LIB_DBG_ALL, "\n-cmd55: \n", 0, 0, 0, 0, 0, 0);
    if (rc == OK)
    {
 
		/* 
    	card->isMmc = FALSE; // SD;
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: SD Card Identification: \n\n", card->idx, 2,3,4,5,6);
    }
    else
    {
    */    
		card->isMmc = TRUE;  /* mmc /emmc*/
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: eMMC Identification: \n\n", card->idx, 2,3,4,5,6);
    }	

	/******************************
	SD / eMMC
	******************************/
	if (card->isMmc == FALSE) /* SD*/
	{
		
#ifdef _WRS_CONFIG_SMP
	    start = vxAbsTicksGet();
#else
	    start = vxAbsTicks;
#endif /* _WRS_CONFIG_SMP */

	    /*while (1)*/
	    do
	    {
	    	/* 
	    	ACMD41: to ask card to send its OCR 
			*/
	        rc = sdmmc_ACmd41_SendOpCond (card);
	        if (rc == OK)
	        {    
	        	break;
	        }
#if 0
#ifdef _WRS_CONFIG_SMP
	        end = vxAbsTicksGet();
#else
	        end = vxAbsTicks;
#endif /* _WRS_CONFIG_SMP */

	        /* stardard says 1s for timeout, here we give it 2s */
	        if ((end - start) > (UINT64)(sysClkRateGet() * 2))
	           {
	           SD_DBG (SDMMC_LIB_DBG_ERR, "ACMD41 timeout\n",
	                          0, 0, 0, 0, 0, 0);
	           goto err;
	           }
#endif
			
			retry--;
			if (retry <= 0)
			{			
				SD_DBG (SDMMC_LIB_DBG_ALL, "-Acmd41 err- \n", 0, 0, 0, 0, 0, 0);
				return ERROR;
			}
	    }while (rc == ERROR);


	    /* 
	    CMD2: request card to send CID and put it in identify mode 
		*/
	    rc = sdmmc_Cmd2_AllSendCid (card);
	    if (rc == ERROR)
	    {    
	    	goto err;
		}
		else
		{
			SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd2 ok- \n", 0, 0, 0, 0, 0, 0);
		}

	    /* 
	    CMD3: request card to send RCA 
	    */
	    do
	    {
	        rc = sdmmc_Cmd3_SendRelativeAddr (card);
	        if (rc == ERROR)
	        {    
				SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd3 err- \n", 0, 0, 0, 0, 0, 0);
	        	goto err;
	        }
	    } while (card->info.rca == 0);
		SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd3 ok- \n", 0, 0, 0, 0, 0, 0);

	    /* 
	    CMD9: request card to send CSD 
		*/
	    rc = sdmmc_Cmd9_SendCsd (card);
	    if (rc == ERROR)
	    {    
	    	goto err;
		}
		else
		{
			SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd9 ok- \n", 0, 0, 0, 0, 0, 0);
		}


	    /**************************************************************** 
	    After Card Identification, the station go into Data Transfer Mode 
		****************************************************************/
	    if (card->info.tranSpeed > SDMMC_CLK_FREQ_400KHZ)
	    {
	        workSpeed = card->info.tranSpeed < SDMMC_CLK_FREQ_25MHZ ?
	                    card->info.tranSpeed : SDMMC_CLK_FREQ_25MHZ;

	        sdmmc_Host_ClkFreqSetup (card->host, workSpeed);
	    }
	    
	   /* card->info.highSpeed = FALSE;*/

	    /* 
	    CMD7: select one card and put it into transfer state 
	    */
	    rc = sdmmc_Cmd7_SelectCard (card);
	    if (rc == ERROR)
	    {
	    	goto err;
		}
		else
		{
			SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd7 ok- \n", 0, 0, 0, 0, 0, 0);
		}

	#if 0
	    /* 
	    AMD51: request card to send its SCR 
		*/
	    rc = sdmmc_ACmd51_SendScr (card);
	    if (rc == ERROR)
	    {
	       /* 
	        * Wordaround for APM86290's SDHC, ACMD51 command needed to be
	        * issued twice to get correct value of SD card's OCR register
	        */
	        if (sdmmc_ACmd51_SendScr (card) == ERROR)
	           goto err;
	    }

	    /* CMD16: set block len */
	    if (card->info.highCapacity == FALSE)
	    {
	        rc = sdmmc_Cmd16_SetBlockLen (card);
	        if (rc == ERROR)
	            return ERROR;
	    }
#endif
	
	  	/* 
	  	ACMD6 
	  	*/
	    /* switch to 4 bit mode if needed */
	    if (card->info.dat4Bit)
	    {
	        rc = sdmmc_ACmd6_SetBusWidth (card, SDMMC_BUS_WIDTH_4BIT);
	        if (rc == ERROR)
	        {    
	        	goto err;			
			}
			else
			{
				SD_DBG (SDMMC_LIB_DBG_ALL, "-Acmd6 ok- \n", 0, 0, 0, 0, 0, 0);
			}

	        /* setup host to enable 4-bit bus width */
	        sdmmc_Host_BusWidthSetup (card->host, SDMMC_BUS_WIDTH_4BIT);
	    }

	    /* setup host to enable high speed clock (50 MHz) if needed */
#if 1  /* jc  */
	    if (card->info.sdSpec >= SD_VERSION_110)
	    {
	        rc = sdmmc_ACmd_SetHighSpeed (card);
	        if (rc == OK)
	        {
	            card->info.highSpeed = TRUE;
	            /*card->info.tranSpeed = SDMMC_CLK_FREQ_50MHZ;  // origin */
	            card->info.tranSpeed = SDMMC_CLK_FREQ_25MHZ;

	            sdmmc_Host_ClkFreqSetup (card->host, card->info.tranSpeed);

				SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd6-highspeed ok- \n", 0, 0, 0, 0, 0, 0);
	        } 
	        else if (rc != ENOTSUP)
	        {
	            goto err;
	        }
	    }
#endif
	    /* check if card is write protected */
	    card->isWp = sdmmc_Host_CardWpCheck (card->host);

	#if 1	
		/* CMD16: set block len */
		if (card->info.highCapacity == FALSE)
		{
			rc = sdmmc_Cmd16_SetBlockLen (card);
			if (rc == ERROR)
			{	
				goto err;			
			}
			else
			{
				SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd16 ok- \n", 0, 0, 0, 0, 0, 0);
			}
		}
	#endif
		/*******************************************************************/
	}
	else  /* eMMC*/
	{		
		/* 
		cmd1: SEND_OP_COND 
		*/
		UINT32 ocr = 0;
		do
		{
			rc = sdmmc_Cmd1_SendOpCond(card);	
			if (rc == ERROR)
				goto err;
			
			ocr = card->host->cmdIssued.cmdRsp[0];
			
			retry--;
			if (retry <= 0)
			{
				goto err;
			}
			
			taskDelay(1);
		} while (!(ocr & 0x80000000) );  /* MMC_CARD_BUSY */
		SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd1 ok- \n\n", 0, 0, 0, 0, 0, 0);

	    /* 
	    cmd2: ALL_SEND_CID 
		*/
	    rc = sdmmc_Cmd2_AllSendCid(card);
		if (rc == ERROR)
		{	
			goto err;		
		}
		else
		{
			SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd2 ok- \n\n", 0, 0, 0, 0, 0, 0);
		}
	    /*bcopy ((char *cmd->cmdRsp, (char *&(card->info.cid), sizeof(SDMMC_CID));*/

	    /* 
	    cmd3: SET_RELATIVE_ADDR, Stand-by State 
		*/
	    rc = sdmmc_Cmd3_SendRelativeAddr_MMC(card);
		if (rc == ERROR)
		{	
			goto err;		
		}
		else
		{
			/*SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd3 ok- \n\n", 0, 0, 0, 0, 0, 0);*/
		}		
		if (!(card->host->cmdIssued.cmdRsp[0] & 0x00000400))   /* STATE_IDENT */
		{
			SD_DBG (SDMMC_LIB_DBG_ALL, "CMD3 response fail! \n", 0, 0, 0, 0, 0, 0);
			return ERROR;
		}
		SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd3 ok- \n\n", 0, 0, 0, 0, 0, 0);

		
		/* 
		cmd9: SEND_CSD, Stand-by State 
		*/
		rc = sdmmc_Cmd9_SendCsd_MMC(card);
		if (rc == ERROR)
		{	
			goto err;		
		}
		else
		{
			SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd9 ok- \n\n", 0, 0, 0, 0, 0, 0);
		}
		
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: csd[0] is 0x%08V \n", card->idx, card->host->cmdIssued.cmdRsp[0], 3,4,5,6);
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: csd[1] is 0x%08V \n", card->idx, card->host->cmdIssued.cmdRsp[1], 3,4,5,6);
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: csd[2] is 0x%08V \n", card->idx, card->host->cmdIssued.cmdRsp[2], 3,4,5,6);
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: csd[3] is 0x%08V \n\n", card->idx, card->host->cmdIssued.cmdRsp[3], 3,4,5,6);

		/*======================================*/
	    /* 
	    CMD7: select one card and put it into transfer state 
	    */
	    rc = sdmmc_Cmd7_SelectCard_MMC (card);
	    if (rc == ERROR)
	        goto err;
		
	  	/* 
	  	ACMD6 
	  	*/
	    /* switch to 4 bit mode if needed */
	    #if 0
		if (card->info.dat4Bit)
	    {
	        rc = sdmmc_ACmd6_SetBusWidth_MMC (card, SDMMC_BUS_WIDTH_4BIT);
	        if (rc == ERROR)
	            goto err;

	        /* setup host to enable 4-bit bus width */
	        sdmmc_Host_BusWidthSetup (card->host, SDMMC_BUS_WIDTH_4BIT);
	    }
		#endif
		
		retry = 5;
		while (retry)
        {
	        rc = sdmmc_ACmd6_SetBusWidth_MMC (card, SDMMC_BUS_WIDTH_4BIT);
            if (rc == ERROR)
            {               
                retry--;	
				if (retry <= 0)
				{
					SD_DBG (SDMMC_LIB_DBG_ALL, "MMC set buswidth fail! \n", 1,2,3,4,5,6);
					return ERROR;
				}
				
				SD_DBG (SDMMC_LIB_DBG_ALL, "retry to MMC set buswidth (%d)! \n", (5-retry), 2,3,4,5,6);
            }
            else
            {
                break;
            }
        }

#if 0
		/*if (card->info.sdSpec >= SD_VERSION_110)*/
		{
			rc = sdmmc_ACmd_SetHighSpeed (card);
			if (rc == OK)
			{
				card->info.highSpeed = TRUE;
				card->info.tranSpeed = SDMMC_CLK_FREQ_50MHZ;	/* origin */

				sdmmc_Host_ClkFreqSetup (card->host, card->info.tranSpeed);
			} 
			else if (rc != ENOTSUP)
			{
				goto err;
			}
		}
#else
		rc = sdmmc_Cmd6_SwitchFunc (card);
	    if (rc == ERROR)
	        goto err;
		
		sysMshcClkFreqSetup(25000000);  /* SDMMC_CLK_25_MHZ */
#endif

		/* check if card is write protected */
		card->isWp = sdmmc_Host_CardWpCheck (card->host);
		
	#if 1	
		/* CMD16: set block len */
		if (card->info.highCapacity == FALSE)
		{
			rc = sdmmc_Cmd16_SetBlockLen (card);
			if (rc == ERROR)
				return ERROR;
		}
	#endif		
		/******************************************************************/
	}	

    return OK;

err:
    return ERROR;
}


STATUS sdmmc_Identify_3
    (
    SDMMC_CARD * card
    )
{
    STATUS rc;
    UINT64 start, end;
    UINT32 workSpeed;
	
    UINT8 cmd6_RdBuf[64] = { 0U };
	
    int retry = 20000;

	extern int g_sdmmc_ctrl;
	SD_DBG (SDMMC_LIB_DBG_ALL, "\n\n ---sdmmc_Identify_3(sd:%d):--- \n\n", g_sdmmc_ctrl, 0, 0, 0, 0, 0);
	taskDelay(1);
	
	/* jc*/
	if (card->host->pDev->unitNumber == 0)
	{
		clk_switch_sdmmc_0();
	}
	else if (card->host->pDev->unitNumber == 1)
	{
		clk_switch_sdmmc_1();
	}
	
    /* 
    power up SD bus 
	*/
    /*sdmmc_Host_VddSetup (card->host, card->host->vdd);*/
	CSR_WRITE_4 (card->host->pDev, ALT_MSHC_PWREN, 0xFFFF);

    /* 
    set default host clock frequency and bus width 
    */
    /*sdmmc_Host_ClkFreqSetup (card->host, SDMMC_CLK_FREQ_400KHZ);*/
    sysMshcClkFreqSetup(SDMMC_CLK_FREQ_400KHZ);
	
    sdmmc_Host_BusWidthSetup (card->host, SDMMC_BUS_WIDTH_1BIT);  /* for snd cmd of all kinds*/
    /*CSR_WRITE_4 (card->host->pDev, ALT_MSHC_CTYPE, SDMMC_BUS_WIDTH_1BIT);*/

    /* ensure card infomration gets cleared initially */
    bzero ((char *)&(card->info), sizeof(SDMMC_INFO));

    /* 
    CMD0: reset all cards on the bus, go IDLE state 
	*/
    rc = sdmmc_Cmd0_GoIdleState (card);
    if (rc == ERROR)
    {    
		SD_DBG (SDMMC_LIB_DBG_ALL, "-sdmmc_Cmd0_GoIdleState err- \n\n", 0, 0, 0, 0, 0, 0);
    	goto err;
    }
	else
	{
		SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd0 ok- \n\n", 0, 0, 0, 0, 0, 0);
	}
	

    /* 
    CMD8: request card to send interface condition 
    */
    rc = sdmmc_Cmd8_SendIfCond (card);
    if (rc == ERROR)
    {    
		SD_DBG (SDMMC_LIB_DBG_ALL, "-sdmmc_Cmd8_SendIfCond err- \n\n", 0, 0, 0, 0, 0, 0);
    	goto err;	
    }
	else
	{
		SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd8 ok(version:%d)- \n\n", card->info.version, 0, 0, 0, 0, 0);
	}

    /* 
    AMD41: request card to send its OCR
    */
    
    /* 
    APP_CMD: cmd55 
	*/
#if 1
    rc = sdmmc_Cmd55_AppCmd(card);	
	SD_DBG (SDMMC_LIB_DBG_ALL, "\n-cmd55: \n", 0, 0, 0, 0, 0, 0);
    if (rc == OK)
    {
    	card->isMmc = FALSE; /* SD;*/
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: SD Card Identification: \n\n", card->idx, 2,3,4,5,6);
    }
    else
    {        
		/**/ 
		card->isMmc = TRUE;  /* mmc /emmc*/
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: eMMC Identification: \n\n", card->idx, 2,3,4,5,6);
    }	
	
#else  /* for bare_read&write sector ok*/
    /*rc = sdmmc_Cmd55_AppCmd(card);	*/
    rc = sdmmc_Cmd55_MMC(card);	
	SD_DBG (SDMMC_LIB_DBG_ALL, "\n-cmd55: \n", 0, 0, 0, 0, 0, 0);
    if (rc == OK)
    {
    	card->isMmc = FALSE; /* SD;*/
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: SD Card Identification: \n\n", card->idx, 2,3,4,5,6);
    }
    else
    {        
		/**/ 
		card->isMmc = TRUE;  /* mmc /emmc*/
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: eMMC Identification: \n\n", card->idx, 2,3,4,5,6);
    }	
#endif


	/******************************
	SD / eMMC
	******************************/
	if (card->isMmc == FALSE) /* SD*/
	{
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: SD Card init ... \n\n", card->idx, 2,3,4,5,6);
		
	    rc = sdmmc_ACmd41_SendOpCond(card);
	    if(rc == ERROR)
	    {
			SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: ACMD41 fail \n", card->idx, 2,3,4,5,6);
	        return ERROR;
	    }

		retry = 20000;
		while (!(card->info.ocr & MMC_CARD_BUSY))
	    {
	        rc = sdmmc_ACmd41_SendOpCond(card);
	        if (rc == ERROR)
	        {
				SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: ACMD41 fail \n", card->idx, 2,3,4,5,6);
	            return ERROR;
	        }
			
	        retry--;
	        if (retry <= 0)
	        {
	            return ERROR;
	        }
	    }		
		SD_DBG (SDMMC_LIB_DBG_ALL, "ACMD41 response success, Card is unbusy, retry[%d]! \n", retry, 2,3,4,5,6);

	    if (card->info.ocr & 0x40000000)
	    {
	        card->highCapacity = 1;
			SD_DBG (SDMMC_LIB_DBG_ALL, "high capacity sd card \n", 1,2,3,4,5,6);
	    }
	    else
	    {
	        card->highCapacity = 0;
			SD_DBG (SDMMC_LIB_DBG_ALL, "normal capacity sd card \n", 1,2,3,4,5,6);
	    }

		
	    /* 
	    ALL_SEND_CID 
		*/
	    rc = sdmmc_Cmd2_AllSendCid(card);
	    if (rc == ERROR)
	    {
			SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc_Cmd2_AllSendCid fail \n", 1,2,3,4,5,6);
	        return ERROR;
	    }
			
	    SD_DBG (SDMMC_LIB_DBG_ALL, "cid[0] is 0x%08X \n", card->host->cmdIssued.cmdRsp[0], 2,3,4,5,6);
	    SD_DBG (SDMMC_LIB_DBG_ALL, "cid[1] is 0x%08X \n", card->host->cmdIssued.cmdRsp[1], 2,3,4,5,6);
	    SD_DBG (SDMMC_LIB_DBG_ALL, "cid[2] is 0x%08X \n", card->host->cmdIssued.cmdRsp[2], 2,3,4,5,6);
	    SD_DBG (SDMMC_LIB_DBG_ALL, "cid[3] is 0x%08X \n", card->host->cmdIssued.cmdRsp[3], 2,3,4,5,6);

		
	    /*
	    SET_RELATIVE_ADDR, Stand-by State 
	    */
	    card->info.rca = 1 ; /* slot id begins at 0 */
		
	    rc = sdmmc_Cmd3_SendRelativeAddr(card);
	    if (rc == ERROR)
	    {
			SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc_Cmd3_SendRelativeAddr fail \n", 1,2,3,4,5,6);
	        return ERROR;
	    }
		
	    /*card->info.rca = card->host->cmdIssued.cmdRsp[0] >> 16;		*/
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc_Cmd3_SendRelativeAddr->rca: 0x%X \n", card->info.rca,2,3,4,5,6);
		
	    if (!(card->info.rca & STATE_IDENT))
	    {
			SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc_Cmd3_SendRelativeAddr-STATE_IDENT: 0x%X \n", card->info.rca,2,3,4,5,6);
	        return ERROR;
	    }
		
		
	    /* 
	    SEND_CSD, Stand-by State 
		*/
	    rc = sdmmc_Cmd9_SendCsd(card);
	    if (rc == ERROR)
	    {
			SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc_Cmd9_SendCsd fail \n", 1,2,3,4,5,6);
	        return ERROR;
	    }
		
	    SD_DBG (SDMMC_LIB_DBG_ALL, "\n", 1,2,3,4,5,6);
	    SD_DBG (SDMMC_LIB_DBG_ALL, "csd[0] is 0x%08X \n", card->host->cmdIssued.cmdRsp[0], 2,3,4,5,6);
	    SD_DBG (SDMMC_LIB_DBG_ALL, "csd[1] is 0x%08X \n", card->host->cmdIssued.cmdRsp[1], 2,3,4,5,6);
	    SD_DBG (SDMMC_LIB_DBG_ALL, "csd[2] is 0x%08X \n", card->host->cmdIssued.cmdRsp[2], 2,3,4,5,6);
	    SD_DBG (SDMMC_LIB_DBG_ALL, "csd[3] is 0x%08X \n", card->host->cmdIssued.cmdRsp[3], 2,3,4,5,6);
		
		rc = sdmmc_decode_CSD_SD(card);
		if (rc == ERROR)
		{
			SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: unrecognised CSD structure version! \n", card->idx, 2,3,4,5,6);
			return ERROR;
		}
		
		rc = sdmmc_decode_CID_SD(card);
		if (rc == ERROR)
		{
			SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: card has unknown MMCA version! \n", card->idx, 2,3,4,5,6);
		}
		
		rc = sdmmcGet_Info_CSD2(card);
		if (rc == ERROR)
		{
			SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: read_blk_len and write_blk_len are not equal! \n", card->idx, 2,3,4,5,6);
		}
		
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: -I- SD Card OK, size: %d MB! \n", \
			   card->idx, sdmmcGet_CardSize_KB2(card)/1024, 3,4,5,6);

		/*======================================*/
		/* 
		CMD7: select one card and put it into transfer state 
		*/
		rc = sdmmc_Cmd7_SelectCard (card);
		if (rc == ERROR)
			goto err;

		rc = sdmmcSet_CardBus_Width_SD (card, SDMMC_BUS_WIDTH_4BIT);
		if (rc == ERROR)
		{				
				SD_DBG (SDMMC_LIB_DBG_ALL, "MMC set buswidth fail! \n", 1,2,3,4,5,6);
				return ERROR;
		}
		
		/* 
		Set high clock rate for the normal data transfer 
		*/
		SD_DBG (SDMMC_LIB_DBG_ALL, "info_csd.max_dtr:%d \n", card->info.info_csd.max_dtr,2,3,4,5,6);
		sysMshcClkFreqSetup(card->info.info_csd.max_dtr);

		rc = sdmmcGet_Info_SCR2(card);
        if (rc == ERROR)
        {
			SD_DBG (SDMMC_LIB_DBG_ALL, "SD Get SD's SCR info error! \n", 1,2,3,4,5,6);
            return ERROR;
        }
		
		if (card->info.info_scr.sd_spec != 0)
		{
			rc = sdmmcGet_BusSpeed_SD(card, cmd6_RdBuf);
			if (rc == ERROR)
			{
				SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmcGet_BusSpeed_SD fail! \n", 1,2,3,4,5,6);
				return ERROR;
			}
			
			if ((cmd6_RdBuf[13] & HIGH_SPEED_SUPPORT) != 0)
			{
				rc = sdmmc_change_BusSpeed_SD(card, SDMMC_CLK_FREQ_25MHZ);
				if (rc == ERROR)
                {
					SD_DBG (SDMMC_LIB_DBG_ALL, "Fail to change SD to high speed mode! \n", 1,2,3,4,5,6);
                    return ERROR;
                }
                else
                { 
					SD_DBG (SDMMC_LIB_DBG_ALL, "Change SD to high speed mode succeed! \n", 1,2,3,4,5,6);
                }
			}
		}
		
		if (card->info.highCapacity == FALSE)
		{
			rc = sdmmc_Cmd16_SetBlockLen (card);
			if (rc == ERROR)
			{			
				SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc_Cmd16_SetBlockLen fail! \n", 1,2,3,4,5,6);
				return ERROR;
			}
			else
			{
				SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc_Cmd16_SetBlockLen ok! \n", 1,2,3,4,5,6);
			}
		}		
	}
	else  /* eMMC*/
	{		
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: MCC init ... \n\n", card->idx, 2,3,4,5,6);
		/* 
		cmd1: SEND_OP_COND 
		*/
		UINT32 ocr = 0;
		do
		{
			rc = sdmmc_Cmd1_SendOpCond(card);	
			/*if (rc == ERROR)*/
			/*	goto err;*/
			
			ocr = card->host->cmdIssued.cmdRsp[0];
			
			retry--;
			if (retry <= 0)
			{
				goto err;
			}
			
			taskDelay(1);
		} while (!(ocr & 0x80000000) );  /* MMC_CARD_BUSY: Card Power up status bit */
		SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd1 ok- \n\n", 0, 0, 0, 0, 0, 0);

	    /* 
	    cmd2: ALL_SEND_CID 
		*/
	    rc = sdmmc_Cmd2_AllSendCid(card);
		if (rc == ERROR)
		{	
			goto err;		
		}
		else
		{
			SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd2 ok- \n\n", 0, 0, 0, 0, 0, 0);
		}
	    /*bcopy ((char *cmd->cmdRsp, (char *&(card->info.cid), sizeof(SDMMC_CID));*/

	    /* 
	    cmd3: SET_RELATIVE_ADDR, Stand-by State 
		*/
	    rc = sdmmc_Cmd3_SendRelativeAddr_MMC(card);
		if (rc == ERROR)
		{	
			goto err;		
		}
		else
		{
			/*SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd3 ok- \n\n", 0, 0, 0, 0, 0, 0);*/
		}		
		if (!(card->host->cmdIssued.cmdRsp[0] & 0x00000400))   /* STATE_IDENT */
		{
			SD_DBG (SDMMC_LIB_DBG_ALL, "CMD3 response fail! \n", 0, 0, 0, 0, 0, 0);
			return ERROR;
		}
		SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd3 ok- \n\n", 0, 0, 0, 0, 0, 0);

		
		/* 
		cmd9: SEND_CSD, Stand-by State 
		*/
		rc = sdmmc_Cmd9_SendCsd_MMC(card);
		if (rc == ERROR)
		{	
			goto err;		
		}
		else
		{
			SD_DBG (SDMMC_LIB_DBG_ALL, "-cmd9 ok- \n\n", 0, 0, 0, 0, 0, 0);
		}
		
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: csd[0] is 0x%08X \n", card->idx, card->host->cmdIssued.cmdRsp[0], 3,4,5,6);
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: csd[1] is 0x%08X \n", card->idx, card->host->cmdIssued.cmdRsp[1], 3,4,5,6);
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: csd[2] is 0x%08X \n", card->idx, card->host->cmdIssued.cmdRsp[2], 3,4,5,6);
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: csd[3] is 0x%08X \n\n", card->idx, card->host->cmdIssued.cmdRsp[3], 3,4,5,6);


		rc = sdmmc_decode_CSD_MMC2(card);
		if (rc == ERROR)
		{
			SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: unrecognised CSD structure version! \n", card->idx, 2,3,4,5,6);
			return ERROR;
		}
		
		rc = sdmmc_decode_CID_MMC2(card);
		if (rc == ERROR)
		{
			SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: card has unknown MMCA version! \n", card->idx, 2,3,4,5,6);
		}
		
		rc = sdmmcGet_Info_CSD2(card);
		if (rc == ERROR)
		{
			SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: read_blk_len and write_blk_len are not equal! \n", card->idx, 2,3,4,5,6);
		}
		
		SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: -I- MMC Card OK, size: %d MB! \n", \
			   card->idx, sdmmcGet_CardSize_KB2(card)/1024, 3,4,5,6);


		/*======================================*/
	    /* 
	    CMD7: select one card and put it into transfer state 
	    */
	    rc = sdmmc_Cmd7_SelectCard_MMC (card);
	    if (rc == ERROR)
	        goto err;
		
	  	/* 
	  	ACMD6 
	  	*/
	    /* switch to 4 bit mode if needed */
	    #if 0
		if (card->info.dat4Bit)
	    {
	        rc = sdmmc_ACmd6_SetBusWidth_MMC (card, SDMMC_BUS_WIDTH_4BIT);
	        if (rc == ERROR)
	            goto err;

	        /* setup host to enable 4-bit bus width */
	        sdmmc_Host_BusWidthSetup (card->host, SDMMC_BUS_WIDTH_4BIT);
	    }
		#endif
		
        rc = sdmmc_ACmd6_SetBusWidth_MMC (card, SDMMC_BUS_WIDTH_4BIT);
        if (rc == ERROR)
        {               
				SD_DBG (SDMMC_LIB_DBG_ALL, "MMC set buswidth fail! \n", 1,2,3,4,5,6);
				return ERROR;
        }
		
		taskDelay(1);
		
		rc = sdmmcGet_ExtCsd_MMC2(card);
        if (rc == ERROR)
        {               
				SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc ctrl_%d: Get MMC Ext_CSD info failed! \n", card->idx, 2,3,4,5,6);
				return ERROR;
        }
		/*
		*/
		
		/* 
		Set high clock rate for the normal data transfer 
		*/
		sysMshcClkFreqSetup(card->info.info_csd.max_dtr);
		
		
		/*sysMshcClkFreqSetup(25000000);  /* SDMMC_CLK_25_MHZ */
		sdmmc_change_BusSpeed_MMC2(card, SDMMC_CLK_FREQ_25MHZ);

		/* check if card is write protected */
		/*card->isWp = sdmmc_Host_CardWpCheck (card->host);*/
		
	#if 1	
		/* CMD16: set block len */
		card->info.highCapacity = FALSE;
		if (card->info.highCapacity == FALSE)
		{
			rc = sdmmc_Cmd16_SetBlockLen (card);
			if (rc == ERROR)
			{			
				SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc_Cmd16_SetBlockLen fail! \n", 1,2,3,4,5,6);
				return ERROR;
			}
			else
			{
				SD_DBG (SDMMC_LIB_DBG_ALL, "sdmmc_Cmd16_SetBlockLen ok! \n", 1,2,3,4,5,6);
			}
		}
	#endif		
		/******************************************************************/
	}	

    return OK;

err:
    return ERROR;
}



/*******************************************************************************
*
* sdmmc_CardIdxAlloc - allocate a global card index
*
* This routine allocates a global card index.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS sdmmc_CardIdxAlloc
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
* sdmmc_CardIdxFree - free a global card index
*
* This routine frees a global card index.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void sdmmc_CardIdxFree
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
* sdmmc_Host_Show - print SD/MMC host related information
*
* This routine prints SD/MMC host related information.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void sdmmc_HostShow
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
* sdmmc_CardShow - print SD/MMC card related information
*
* This routine prints SD/MMC card related information.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void sdmmc_CardShow(SDMMC_CARD * card)
{

#if 0    
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
#endif

	return;
}

	
