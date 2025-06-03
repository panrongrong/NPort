/* sdmmc_cmd.c - fmsh 7020/7045 sdmmc driver */

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
01a, 25Nov19, jc  written.
*/

/*
DESCRIPTION
	no vxbus struct
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

#include "vxSdmmc.h"

const int sdmmc_resp_len[] = 
{
    0,
    CMD_RES_LEN_SHORT,
    CMD_RES_LEN_SHORT,
    CMD_RES_LEN_LONG,
    CMD_RES_LEN_SHORT,
    CMD_RES_LEN_SHORT,
    CMD_RES_LEN_SHORT,
    CMD_RES_LEN_SHORT,
    CMD_RES_LEN_SHORT
};


#if 1


/*****************************************************************************
*
* @description
* This function sends command 0 to reset card to idle state.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_cmd0(vxT_SDMMC* pSdmmc)
{
    vxT_SDMMC_CMD * pCmd = pSdmmc->pSdmmcCmd;
	
    int ret;

	memset((UINT8 *)pCmd, 0, sizeof(vxT_SDMMC_CMD));
	
    /* 
    go IDLE state 
	*/
    pCmd->cmd_arg = 0x00;
	
    pCmd->cmd_idx = CMD_GO_IDLE_STATE;	
    pCmd->rsp_type = CMD_RESP_TYPE_NONE;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE;
	
    ret = sdmmc_sndCmd(pSdmmc, pCmd);
    if (ret < 0)
    {
        VX_DBG2("CMD0(go IDLE state) fail: %d \r\n", ret, 2,3,4,5,6);
    }

    return ret;
}

/*****************************************************************************
*
* @description
* This function sends command 1, only used for MMC/EMMC. CMD1是一个特定的同步命令
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_cmd1(vxT_SDMMC* pSdmmc)
{
    vxT_SDMMC_CMD * pCmd = pSdmmc->pSdmmcCmd;
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxT_SDMMC_CMD));

    pSdmmc->pSdmmcCard->ocr = 0x40FF8000;  /* relative card address */
	
    pCmd->cmd_arg = pSdmmc->pSdmmcCard->ocr;
	
    pCmd->cmd_idx = CMD_SEND_OP_COND;
    pCmd->rsp_type = CMD_RESP_TYPE_R3;
    pCmd->rsp_flag = CMD_RESP_FLAG_NOCRC;
	
    ret = sdmmc_sndCmd(pSdmmc, pCmd);
    if (ret < 0)
    {
        VX_DBG2("CMD1(sync) fail: %d \r\n", ret, 2,3,4,5,6);
    }

    return ret;
}

/*****************************************************************************
*
* @description
* This function sends command 2 to ask card to send CID number.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_cmd2(vxT_SDMMC* pSdmmc)
{
    vxT_SDMMC_CMD * pCmd = pSdmmc->pSdmmcCmd;
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxT_SDMMC_CMD));
    
    pCmd->cmd_arg = 0x00;
	
    pCmd->cmd_idx = CMD_ALL_SEND_CID;
    pCmd->rsp_type = CMD_RESP_TYPE_R2;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE; 
	
    ret = sdmmc_sndCmd(pSdmmc, pCmd);
    if (ret < 0)
    {
        VX_DBG2("CMD2(ask CID) fail: %d \r\n", ret, 2,3,4,5,6);
    }

    return ret;
}

/*****************************************************************************
*
* @description
* This function sends command 3 to ask SD card to publish a new
* relative address(RCA), or assign relative address to the MMC card adn EMMC.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_cmd3(vxT_SDMMC* pSdmmc)
{
    vxT_SDMMC_CMD * pCmd = pSdmmc->pSdmmcCmd;	
    vxT_SDMMC_CARD * pSdmmcCard = pSdmmc->pSdmmcCard;
	
    int ret;
    UINT16 wNewAddr = 0;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxT_SDMMC_CMD));
	
    if ((pSdmmcCard->card_type == MMC) || (pSdmmcCard->card_type == EMMC))
    {
        wNewAddr = pSdmmcCard->rca + 1;
        if (wNewAddr == 0) 
		{	
			wNewAddr++;
        }        
        pSdmmcCard->rca = wNewAddr;
		
        pCmd->cmd_arg = (pSdmmcCard->rca << 16) & 0xFFFF0000;
    }
    else
    {    
    	pCmd->cmd_arg = 0x00;
    }
	
    pCmd->cmd_idx = CMD_SET_RELATIVE_ADDR;
	
    pCmd->rsp_type = CMD_RESP_TYPE_R6;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE; 
	
	VX_DBG2("CMD3(ask RCA) g_pSdmmcCard->rca: 0x%X \r\n", pSdmmcCard->rca, 2,3,4,5,6);
	
    ret = sdmmc_sndCmd(pSdmmc, pCmd);
    if (ret < 0)
    {
        VX_DBG2("CMD3(ask RCA) fail: %d \r\n", ret, 2,3,4,5,6);
    }

    return ret;
}

/*****************************************************************************
*
* @description
* This function switches the mode of operation of the selected card,
* CMD6 is valid under the "trans" state.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_cmd6_sd(vxT_SDMMC* pSdmmc, UINT32 cmdArg, UINT8 *pBuf)
{
    vxT_SDMMC_CMD * pCmd = pSdmmc->pSdmmcCmd;
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxT_SDMMC_CMD));
    
    pCmd->cmd_arg = cmdArg;
	
    pCmd->cmd_idx = CMD_SWITCH_FUNC;
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_RDATA;
	
    pCmd->txRxBuf = (UINT8 *)pBuf;
	
    pCmd->data_len = 512 / 8;
    pCmd->block_size = 512 / 8;
	
    ret = sdmmc_sndCmd(pSdmmc, pCmd);
    if (ret < 0)
    {
        VX_DBG2("CMD6_sd(switch mode) fail: %d \r\n", ret, 2,3,4,5,6);
    }
    
    return ret;
}


/*****************************************************************************
*
* @description
* This function switches the mode of operation of the
* selected card or modifies the EXT_CSD registers.
* CMD6 is valid under the "trans" state.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_cmd6_mmc(vxT_SDMMC* pSdmmc, UINT32 cmdArg)
{
    vxT_SDMMC_CMD * pCmd = pSdmmc->pSdmmcCmd;
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxT_SDMMC_CMD));
    
    pCmd->cmd_arg = cmdArg;
	
    pCmd->cmd_idx = CMD_SWITCH_FUNC_MMC;
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE;
	
    ret = sdmmc_sndCmd(pSdmmc, pCmd);
    if (ret < 0)
    {
        VX_DBG2("CMD6_mmc(switch mode) fail: %d \r\n", ret, 2,3,4,5,6);
    }
    
    return ret;
}

/*****************************************************************************
*
* @description
* This function sends command 7 to toggle a card between the stand-by and
* transfer states or between the programming and disconnect states.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_cmd7(vxT_SDMMC* pSdmmc)
{
    vxT_SDMMC_CMD * pCmd = pSdmmc->pSdmmcCmd;
    vxT_SDMMC_CARD * pSdmmcCard = pSdmmc->pSdmmcCard;

    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxT_SDMMC_CMD));
    
    pCmd->cmd_arg = (pSdmmcCard->rca << 16) & 0xFFFF0000;
	
    pCmd->cmd_idx = CMD_SELECT_CARD;
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE;
	
    ret = sdmmc_sndCmd(pSdmmc, pCmd);
    if (ret < 0)
    {
        VX_DBG2("CMD7(SELECT-DESELECT_CARD) fail: %d \r\n", ret, 2,3,4,5,6);
    }

    return ret;
}

/*****************************************************************************
*
* @description
* This function sends command 8 to detect SD card interface condition,
* should be performed at initialization time to detect the card type.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_cmd8_sd(vxT_SDMMC* pSdmmc)
{
    vxT_SDMMC_CMD * pCmd = pSdmmc->pSdmmcCmd;
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxT_SDMMC_CMD));
    
    /*pCmd->cmd_arg = 0xAA;  // 0x1AA-> 0AA : SD_CMD8_VOL_PATTERN;	*/
    pCmd->cmd_arg = SD_CMD8_VOL_PATTERN;
    
    pCmd->cmd_idx = CMD_SEND_IF_COND;
    pCmd->rsp_type = CMD_RESP_TYPE_R7;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE;
	
    ret = sdmmc_sndCmd(pSdmmc, pCmd);
    if (ret < 0)
    {
        VX_DBG2("CMD8_sd(ask EXT_CSD) fail: %d \r\n", ret, 2,3,4,5,6);
    }

    return ret;
}

/*****************************************************************************
*
* @description
* This function sends command 8 to get the EXT_CSD register
* as a block of data, only used for MMC card 4.0.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*           pBuf is 512 byte buffer pointer for EXT_CSD data.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_cmd8_mmc(vxT_SDMMC* pSdmmc, UINT8 *pBuf)
{
    vxT_SDMMC_CMD * pCmd = pSdmmc->pSdmmcCmd;
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxT_SDMMC_CMD));

    pCmd->cmd_arg = 0x00;
	
    pCmd->cmd_idx = MMC_SEND_EXT_CSD;
    pCmd->rsp_type = CMD_RESP_TYPE_R7;
    pCmd->rsp_flag = CMD_RESP_FLAG_RDATA;
	
    pCmd->txRxBuf = (UINT8 *)pBuf;
    pCmd->data_len = 512;
	
    pCmd->block_size = 512;
	
    ret = sdmmc_sndCmd(pSdmmc, pCmd);
    if (ret < 0)
    {
        VX_DBG2("CMD8_mmc(ask EXT_CSD) fail: %d \r\n", ret, 2,3,4,5,6);
    }
    
    return ret;
}

/*****************************************************************************
*
* @description
* This function sends command 9 to ask addressed card to send its 
* card specific data(CSD).
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_cmd9(vxT_SDMMC* pSdmmc)
{
    vxT_SDMMC_CMD * pCmd = pSdmmc->pSdmmcCmd;	
    vxT_SDMMC_CARD * pSdmmcCard = pSdmmc->pSdmmcCard;
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxT_SDMMC_CMD));
    
    VX_DBG("pSdmmcCard->rca:0x%08X \n", pSdmmcCard->rca, 2,3,4,5,6);
	
    pCmd->cmd_arg = (pSdmmcCard->rca << 16) & 0xFFFF0000;
	
    pCmd->cmd_idx = CMD_SEND_CSD;
    pCmd->rsp_type = CMD_RESP_TYPE_R2;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE; 
	
    ret = sdmmc_sndCmd(pSdmmc, pCmd);
    if (ret < 0)
    {
        VX_DBG2("CMD9(ask CSD) fail: %d \r\n", ret, 2,3,4,5,6);
    }

    return ret;
}

/*****************************************************************************
*
* @description
* This function sends command 12 to force the card to stop transmission.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_cmd12(vxT_SDMMC* pSdmmc)
{
    vxT_SDMMC_CMD * pCmd = pSdmmc->pSdmmcCmd;
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxT_SDMMC_CMD));
    
    pCmd->cmd_arg = 0x00;
	
    pCmd->cmd_idx = CMD_STOP_TRANSMISSION;
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE; 
	
    ret = sdmmc_sndCmd(pSdmmc, pCmd);
    if (ret < 0)
    {
        VX_DBG2("CMD12(STOP_TRANSMISSION) fail: %d \r\n", ret, 2,3,4,5,6);
    }

    return ret;
}

/*****************************************************************************
*
* @description
* This function sends command 13 to ask addressed card to send
* its status register.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_cmd13(vxT_SDMMC* pSdmmc)
{
    vxT_SDMMC_CMD * pCmd = pSdmmc->pSdmmcCmd;
    vxT_SDMMC_CARD * pSdmmcCard = pSdmmc->pSdmmcCard;

	int ret = FMSH_SUCCESS;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxT_SDMMC_CMD));
    
    pCmd->cmd_arg = (pSdmmcCard->rca << 16) & 0xFFFF0000;
	
    pCmd->cmd_idx = CMD_SEND_STATUS;
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE;
	
    ret = sdmmc_sndCmd(pSdmmc, pCmd);
    if (ret < 0)
    {
        VX_DBG2("CMD13(ask card status) fail: %d \r\n", ret, 2,3,4,5,6);
    }

    return ret;
}

/*****************************************************************************
*
* @description
* In the case of a Standard Capacity SD Memory Card, command 16 sets the
* block length (in bytes) for all following block commands
* (read, write, lock).
* Default block length is fixed to 512 Bytes.
* Set length is valid for memory access commands only if partial block read
* operation are allowed in CSD.
* In the case of a High Capacity SD Memory Card, block length set by CMD16
* command does not affect the memory read and write commands. Always 512
* Bytes fixed block length is used. This command is effective for LOCK_UNLOCK
* command. In both cases, if block length is set larger than 512Bytes, the
* card sets the BLOCK_LEN_ERROR bit.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*           len is block length in bytes.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_cmd16(vxT_SDMMC* pSdmmc, UINT32 len)
{
    vxT_SDMMC_CMD * pCmd = pSdmmc->pSdmmcCmd;
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxT_SDMMC_CMD));
    
    pCmd->cmd_arg = len;
	
    pCmd->cmd_idx = CMD_SET_BLOCKLEN;
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE;
	
    ret = sdmmc_sndCmd(pSdmmc, pCmd);
    if (ret < 0)
    {
        VX_DBG2("CMD16(SET_BLOCKLEN) fail: %d \r\n", ret, 2,3,4,5,6);
    }

    return ret;
}

/*****************************************************************************
*
* @description
* This function sends command 23 to define the number of blocks(read/write)
* to be transfered, used only for MMC card and EMMC.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*           len is number of bytes to be transfered.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_cmd23(vxT_SDMMC* pSdmmc, UINT32 len)
{
    vxT_SDMMC_CMD * pCmd = pSdmmc->pSdmmcCmd;
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxT_SDMMC_CMD));
	
    pCmd->cmd_arg = len >> 9;

    pCmd->cmd_idx = CMD_SET_BLOCK_COUNT;
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE;
	
    ret = sdmmc_sndCmd(pSdmmc, pCmd);
    if (ret < 0)
    {
        VX_DBG2("CMD23(SET_BLOCK_COUNT) fail: %d \r\n", ret, 2,3,4,5,6);
    }

    return ret;
}

/*****************************************************************************
*
* @description
* This function sends command 55 to indicate to the card that the next
* command is an application specific command rather than a standard command.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_cmd55(vxT_SDMMC* pSdmmc)
{
    vxT_SDMMC_CMD * pCmd = pSdmmc->pSdmmcCmd;	
    vxT_SDMMC_CARD * pSdmmcCard = pSdmmc->pSdmmcCard;
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxT_SDMMC_CMD));
	
    /* 
     CMD55 APP_CMD Mandatory特殊指令前命令,在发送ACMD类指令前,需要发送此命令 
	*/  
	
    pCmd->cmd_arg = (pSdmmcCard->rca << 16) & 0xFFFF0000;
	
    pCmd->cmd_idx = CMD_APP_CMD;
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE;
	
    ret = sdmmc_sndCmd(pSdmmc, pCmd);
    if (ret < 0)
    {
        VX_DBG2("CMD55(APP_CMD Mandatory) fail: %d \r\n", ret, 2,3,4,5,6);
    }

    return ret;
}

/*****************************************************************************
*
* @description
* This function sends application command 6 to define the data bus width
* (00=1bit or 10=4bits) to be used for data transfer.
* The allowed data bus widths are given in SCR register.
* Should be invoked after Sdmmc_Cmd55().
*
* @param
*           pSdmmc is the pointer to a sd card device.
*           Arg is the command's argument.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_appCmd6(vxT_SDMMC* pSdmmc, UINT32 cmdArg)
{
    vxT_SDMMC_CMD * pCmd = pSdmmc->pSdmmcCmd;
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxT_SDMMC_CMD));
    
    pCmd->cmd_arg = cmdArg;
	
    pCmd->cmd_idx = CMD_ACMD_SET_BUS_WIDTH;
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE;
	
    ret = sdmmc_sndCmd(pSdmmc, pCmd);
    if (ret < 0)
    {
        VX_DBG2("appCmd6(ACMD_SET_BUS_WIDTH) fail: %d \r\n", ret, 2,3,4,5,6);
    }
    
    return ret;    
}

/*****************************************************************************
*
* @description
* This function sends application command 41 to ask card to send their
* operation conditions.
* Should be invoked after Sdmmc_Cmd55().
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_appCmd41(vxT_SDMMC* pSdmmc)
{
    vxT_SDMMC_CMD * pCmd = pSdmmc->pSdmmcCmd;	
    vxT_SDMMC_CARD * pSdmmcCard = pSdmmc->pSdmmcCard;
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxT_SDMMC_CMD));

    pSdmmcCard->ocr = 0x00FF8000;
	
    if(pSdmmcCard->card_ver == SDMMC_CARD_VER_1_0)
    {    
    	pCmd->cmd_arg = pSdmmcCard->ocr;
    }
    else
    {    
    	pCmd->cmd_arg = pSdmmcCard->ocr | 0x40000000;  /* #define OCR_CARD_CAP_STS    (1 << 30):0x40000000*/
    }
	
    pCmd->cmd_idx = ACMD_OP_COND;	
    pCmd->rsp_type = CMD_RESP_TYPE_R3;
    pCmd->rsp_flag = CMD_RESP_FLAG_NOCRC;
	
    ret = sdmmc_sndCmd(pSdmmc, pCmd);
    if (ret < 0)
    {
        VX_DBG2("appCmd41(ask card COND) fail: %d \r\n", ret, 2,3,4,5,6);
    }
    
    return ret;    
}

/*****************************************************************************
*
* @description
* This function sends application command 51 to read the
* SD configuration register(SCR).
* Should be invoked after Sdmmc_Cmd55().
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_appCmd51(vxT_SDMMC* pSdmmc, UINT8* pBuf)
{
    vxT_SDMMC_CMD * pCmd = pSdmmc->pSdmmcCmd;
	
    int ret;
	
	memset((UINT8*)pCmd, 0, sizeof(vxT_SDMMC_CMD));
    
    pCmd->cmd_arg = 0x00;
	
    pCmd->cmd_idx = ACMD_SEND_SCR;
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_RDATA;
	
    pCmd->txRxBuf = (UINT8*)pBuf;
    pCmd->data_len = 8;
    pCmd->block_size = 8;
	
    ret = sdmmc_sndCmd(pSdmmc, pCmd);
    if (ret < 0)
    {
        VX_DBG2("appCmd51(ask card SDR) fail: %d \r\n", ret, 2,3,4,5,6);
    }
    
    return ret;    
}

/*****************************************************************************
*
* @description
* This function defines the data bus width (00=1bit or 10=4bits)
* to be used for data transfer.
* The allowed data bus widths are given in SCR register.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*           bus_width is bus width.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_sdACMD6(vxT_SDMMC* pSdmmc, UINT32 bus_width)
{
    int ret;
	
    ret = sdmmc_cmd55(pSdmmc);
    if(ret < 0)
    {
        return ret;
    }

    ret = sdmmc_appCmd6(pSdmmc, bus_width);
    
    return ret;    
}

/*****************************************************************************
*
* @description
* This function asks card to send their operation conditions.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_sdACMD41(vxT_SDMMC* pSdmmc)
{
    int ret;
	
    ret = sdmmc_cmd55(pSdmmc);
    if(ret < 0)
    {
        return ret;
    }

    ret = sdmmc_appCmd41(pSdmmc);
    
    return ret;    
}

/*****************************************************************************
*
* @description
* This function reads the SD configuration register(SCR).
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_sdACMD51(vxT_SDMMC* pSdmmc, UINT8 *pBuf)
{
    int ret;
	
    ret = sdmmc_cmd55(pSdmmc);
    if(ret < 0)
    {
        return ret;
    }
	else
	{
		printf("sdmmc_sdACMD51->sdmmc_cmd55() ok! \n");
	}

    ret = sdmmc_appCmd51(pSdmmc, pBuf);
    
    return ret;    
}

/*****************************************************************************
*
* @description
* This function returns the response length.
*
* @param
*           res_type response type.
*
* @return
*           response length.
*
* @note     NA.
*
*****************************************************************************/
static int sdmmc_getResLen(int res_type)
{
    return sdmmc_resp_len[res_type];
}

#endif


#if 1

extern UINT32 dmaCtrl_Rd_CfgReg32(vxT_DMA* pDma, UINT32 offset);
extern UINT32 dmaCtrl_Get_FreeChn(vxT_DMA* pDma);

int sdmmc_rxData_poll(vxT_SDMMC* pSdmmc, UINT8 *pBuf, int rx_len) 
{
    int timeout = 0;
    UINT32 tmp32 = 0;
    int cnt = 0;
    
    UINT32              mshcStatus;
    UINT32              fifoSlots;
	int i = 0;
	
	/*printf("sdmmc_rxData_poll: len-%d \n", rx_len);*/
	
    if (((UINT32)pBuf) & 0x03)  /* the address roundup at 4bytes*/
    {
		timeout = 0x100000;
        while (cnt < rx_len)
        {
            if (!(sdmmcCtrl_RdReg32(pSdmmc, SDMMC_STATUS) & FIFO_EMPTY))
            {
                tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_FIFO);
				
                pBuf[cnt++] = tmp32 & 0xff;
                pBuf[cnt++] = (tmp32 >> 8) & 0xff;
                pBuf[cnt++] = (tmp32 >> 16) & 0xff;
                pBuf[cnt++] = (tmp32 >> 24) & 0xff;
            }

            timeout--;
            if (timeout <= 0)
            {
                return (-FMSH_ETIME);
            }
        }
    }
    else
    {
        timeout = 0x100000;
        while (cnt < rx_len)
        {			
		#if 0  /* speed slow*/
            if (!(sdmmcCtrl_RdReg32(pSdmmc, SDMMC_STATUS) & FIFO_EMPTY))
            {
                *(UINT32 *)(pBuf + cnt) = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_FIFO);
                cnt += 4;
            }
		#else		
			mshcStatus =  sdmmcCtrl_RdReg32(pSdmmc, SDMMC_STATUS);
            if (!(mshcStatus & FIFO_EMPTY))  /* no empty*/
            {
				fifoSlots = ALT_MSHC_STATUS_FIFO_COUNT (mshcStatus);
				
				for (i=0; i<fifoSlots; i++)
				{
	                *(UINT32*)(pBuf + cnt) = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_FIFO);
					cnt += 4;
					
					if (cnt >= rx_len)
					{
						break;
					}
				}
            }
		#endif

            timeout--;
            if (timeout <= 0)
            {
                return (-FMSH_ETIME);
            }
        }
    }
    
    /*if (trans_mode != sdmmc_trans_mode_dw_dma)*/
    {
        timeout = SDMMC_LOOP_TIMEOUT;
        while (!(sdmmcCtrl_RdReg32(pSdmmc, SDMMC_RINTSTS) & RAW_INT_STATUS_DTO))
        {
            /*delay_1us();*/
            /*vxbUsDelay(1);*/
			
            timeout--;
            if (timeout == 0)
            {
                return -FMSH_ETIME;
            }
        }
    }
	
    return cnt;
}

int sdmmc_txData_poll(vxT_SDMMC* pSdmmc, UINT8 *pBuf, int tx_len)
{
    UINT32 tmp32;
    int timeout = 0;
	int cnt = 0;
	
    UINT32              mshcStatus;
    UINT32              fifoSlots;
	int i = 0;
    
    if (((UINT32)pBuf) & 0x03)  /* the address roundup: 4 bytes */
    {
        timeout = 0x100000;
		
        while (cnt < tx_len)
        {
            if (!(sdmmcCtrl_RdReg32(pSdmmc, SDMMC_STATUS) & FIFO_FULL))
            {
                tmp32 = pBuf[cnt++];
                tmp32 += (pBuf[cnt++] << 8);
                tmp32 += (pBuf[cnt++] << 16);
                tmp32 += (pBuf[cnt++] << 24);
				
                sdmmcCtrl_WrReg32(pSdmmc, SDMMC_FIFO, tmp32);
            }

            timeout--;
            if (timeout <= 0)
            {
                return (-FMSH_ETIME);
            }
        }
    }
    else
    {
        timeout = 0x100000;
        while (cnt < tx_len)
        {
        #if 0 /* speed slow*/
            if (!(sdmmcCtrl_RdReg32(pSdmmc, SDMMC_STATUS) & FIFO_FULL))
            {
                sdmmcCtrl_WrReg32(pSdmmc, SDMMC_FIFO, *(UINT32 *)(pBuf + cnt));
                cnt += 4;
            }			
			
		#else
		
			mshcStatus =  sdmmcCtrl_RdReg32(pSdmmc, SDMMC_STATUS);
            if (!(mshcStatus & FIFO_FULL))  /* no full*/
            {
				fifoSlots = ALT_MSHC_STATUS_FIFO_COUNT (mshcStatus);
				fifoSlots = ALT_MSHC_DEFAULT_FIFO_LEN - fifoSlots;
				
				for (i=0; i<fifoSlots; i++)
				{
					sdmmcCtrl_WrReg32(pSdmmc, SDMMC_FIFO, *(UINT32 *)(pBuf + cnt));
					cnt += 4;

					if (cnt >= tx_len)
					{
						break;
					}
				}
            }
		#endif
		
			timeout--;
			if (timeout <= 0)
			{
				return (-FMSH_ETIME);
			}
        }
	}

	/**/
	/* poll */
	/**/
    timeout = SDMMC_POLL_TIMEOUT;
    do 
	{
        /*delay_1us();*/
        tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_RINTSTS);
		
        timeout--;
        if (timeout <= 0)
        {
            return (-FMSH_ETIME);
        }
    } while (!(tmp32 & RAW_INT_STATUS_DTO)); /* bit3 - 数据传输（DTO）*/
    
    return cnt;
}

int sdmmc_rxData_DMA(vxT_SDMMC* pSdmmc, UINT8* pBuf8, int rx_len) 
{
    vxT_DMA_CTRL * pDma = pSdmmc->pSdmmcDMA;
	
    vxT_DMA_CHNCFG chn_cfg = {0};  /* Channel configuration struct.*/
    int timeout = 0;
		
    int errCode = 0;	
    int chnNum;   /* chn name:0x1010, 0x2020, ...*/
	int chnIdx;   /* chn idx: 0,1,2, ...*/

	UINT32 baseAddr = pSdmmc->pSdmmcCtrl->baseAddr;
    UINT32 remain = rx_len >> 2;
    
	UINT32 tmp32 = 0;
	
	UINT32* pBuf32 = (UINT32*)(pBuf8);
	UINT32 Dma_Blk_Size = 0x0FE0;	       /* rx: 0xfe0 -> 4064 */

	/*printf("sdmmc baseAddr: 0x%X \n", baseAddr);*/
	
	/*
	Dma_Blk_Size = 0x0FE0;
	remain = rx_len >> 2;
	pBuf32 = (UINT32*)(pBuf8);
	*/
	
    while (remain)
    {
        /* Transfer characteristics*/
        chnNum = dmaCtrl_Get_FreeChn(pDma);   /* Select a DMA channel*/
        
        errCode = dmaCtrl_Set_XferType(pDma, chnNum, DMA_XFER_TYPE1_ROW1); /*Dmac_transfer_row1*/
		
        if (errCode == 0)
        {   
        	errCode = dmaCtrl_En_ChnIRQ(pDma, chnNum);
        }
		
        if (errCode == 0)
        {    
        	errCode = dmaCtrl_Get_ChnCfg(pDma, chnNum, &chn_cfg);
        }
        
        /* Set the Source and destination addresses*/
        chn_cfg.sar = baseAddr + SDMMC_FIFO;
        chn_cfg.dar = (UINT32)(pBuf32);
		/*printf("dma-rx_chn_cfg.sar:0x%X, len-%d \n", chn_cfg.sar, rx_len);*/
		
        /* Set the source and destination transfer width*/
		chn_cfg.ctl_src_tr_width = DMA_XFER_WIDTH_32;  /* Dmac_trans_width_32;*/
		chn_cfg.ctl_dst_tr_width = DMA_XFER_WIDTH_32;  /* Dmac_trans_width_32;*/
		
        /* Set the Address increment type for the source and destination*/
        chn_cfg.ctl_sinc = DMA_ADDR_MODE_NO_CHG;  /* Dmac_addr_nochange;*/
		chn_cfg.ctl_dinc = DMA_ADDR_MODE_INC;     /* Dmac_addr_increment;*/
		
        /* Set the source and destination burst transaction length*/
        chn_cfg.ctl_src_msize = DMA_MSIZE_8;  /* Dmac_msize_8;*/
        chn_cfg.ctl_dst_msize = DMA_MSIZE_8;  /* Dmac_msize_8;*/
        
        /* Set the block size for the DMA transfer*/
        /*
        Maximum block size in source transfer widths:
			Legal Values: 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095
		*/
        if (remain > Dma_Blk_Size)  /* 0xfe0 -> 4064*/
        {    
        	chn_cfg.ctl_block_ts = Dma_Blk_Size;
        }
        else
        {    
        	chn_cfg.ctl_block_ts = remain;
        }
        
        /* Set the transfer device type and flow controller*/
        /*chn_cfg.ctl_tt_fc = Dmac_prf2mem_dma;		*/
        chn_cfg.ctl_xfer_path = DMA_XFER_PATH2_DEV2MEM_DMACTRL;  /* Dmac_prf2mem_dma;*/
		
        /* Set the hardware handshaking interface*/
        chn_cfg.cfg_dst_per = DMA_HANDSHAK_IF_0;  /* Dmac_hs_if0;*/
        chn_cfg.cfg_src_per = DMA_HANDSHAK_IF_0;  /* Dmac_hs_if0;*/
		
        /* Set the handshaking select*/
        chn_cfg.cfg_hs_sel_src = DMA_HARD_IF;  /* Dmac_hs_hardware;*/
        chn_cfg.cfg_hs_sel_dst = DMA_HARD_IF;  /* Dmac_hs_hardware;*/
		
        /* Set the FIFO mode*/
        /*chn_cfg.cfg_fifo_mode = Dmac_fifo_mode_half;*/
        
        if (errCode == 0)
        {    
        	errCode = dmaCtrl_Set_ChnCfg(pDma, chnNum, &chn_cfg);
        }

        /* Enable the DMA controller and begin the interrupt driven DMA transfer.*/
        if (errCode == 0)
        {    
        	errCode = dmaCtrl_Start_Xfer(pDma, chnNum, 1);
        }
        
        if (errCode == 0)
        {    
			do
			{
				tmp32 = dmaCtrl_Rd_CfgReg32(pDma, DMAC_INT_RAW_TFR_L_OFFSET);
		        /* Heartbeat*/
		        /*VX_DBG(".", 1,2,3,4,5,6);				*/
		    } while (!tmp32);	/* transfer is in progress*/
        }

        errCode = dmaCtrl_Clr_IRQ(pDma, chnNum, DMA_IRQ_ALL);   /* Dmac_irq_all */		
        if (errCode == 0)
        {    
        	errCode = dmaCtrl_Dis_ChnX(pDma, chnNum);
        }
        
        if (errCode == 0)
        {
            if (remain > Dma_Blk_Size)
            {
                remain -= Dma_Blk_Size;
                pBuf32 += Dma_Blk_Size << 2;
            }
            else
            {
                remain = 0;
            }
        }
        else
        {     
        	/*FMSH_ASSERT(0);*/
        	return FMSH_FAILURE;
        }        
    }    
    
    return rx_len;
}


int sdmmc_txData_DMA(vxT_SDMMC* pSdmmc, UINT8* pBuf8, int tx_len)
{
    vxT_DMA_CTRL * pDma = pSdmmc->pSdmmcDMA;
	
    vxT_DMA_CHNCFG chn_cfg = {0};  /* Channel configuration struct.*/
    int timeout = 0;
		
    int errCode = 0;
    int chnNum;   /* chn name:0x1010, 0x2020, ...*/
	int chnIdx;   /* chn idx: 0,1,2, ...*/

	UINT32 baseAddr = pSdmmc->pSdmmcCtrl->baseAddr;
    UINT32 remain = tx_len >> 2;
	
    int errCode;
    UINT32 tmp32 = 0;
	
	UINT32* pBuf32 = (UINT32*)(pBuf8);
	UINT32 Dma_Blk_Size = 0x3F8;  /* tx: 0x3F8 -> 1016*/
	
    while (remain)
    {
        /* Transfer characteristics*/
        chnNum = dmaCtrl_Get_FreeChn(pDma); /* Select a DMA channel*/
        
        errCode = dmaCtrl_Set_XferType(pDma, chnNum, DMA_XFER_TYPE1_ROW1);
		
        if (errCode == 0)
        {   
        	errCode = dmaCtrl_En_ChnIRQ(pDma, chnNum);
        }
		
        if (errCode == 0)
        {    
        	errCode = dmaCtrl_Get_ChnCfg(pDma, chnNum, &chn_cfg);
        }
        
        /* Set the Source and destination addresses*/
        chn_cfg.sar = (UINT32)(pBuf32);
        chn_cfg.dar = baseAddr + SDMMC_FIFO;
		/*printf("dma-tx_chn_cfg.dar: 0x%X \n", chn_cfg.dar);*/
		
        /* Set the source and destination transfer width*/
		chn_cfg.ctl_src_tr_width = DMA_XFER_WIDTH_32;  /* Dmac_trans_width_32;*/
		chn_cfg.ctl_dst_tr_width = DMA_XFER_WIDTH_32;  /* Dmac_trans_width_32;*/
		
        /* Set the Address increment type for the source and destination*/
        chn_cfg.ctl_sinc = DMA_ADDR_MODE_INC;     /* Dmac_addr_increment;*/
        chn_cfg.ctl_dinc = DMA_ADDR_MODE_NO_CHG;  /* Dmac_addr_nochange;*/
		
        /* Set the source and destination burst transaction length*/
        chn_cfg.ctl_src_msize = DMA_MSIZE_8;  /* Dmac_msize_8;*/
        chn_cfg.ctl_dst_msize = DMA_MSIZE_8;  /* Dmac_msize_8;*/
        
        /* Set the block size for the DMA transfer*/
        if (remain > Dma_Blk_Size)
        {    
        	chn_cfg.ctl_block_ts = Dma_Blk_Size;
		}
        else
        {    
        	chn_cfg.ctl_block_ts = remain;
		}
        
        /* Set the transfer device type and flow controller*/
        chn_cfg.ctl_xfer_path = DMA_XFER_PATH1_MEM2DEV_DMACTRL;  /* Dmac_mem2prf_dma;		*/
        
        /* Set the hardware handshaking interface*/
        chn_cfg.cfg_dst_per = DMA_HANDSHAK_IF_0;  /* Dmac_hs_if0;*/
        chn_cfg.cfg_src_per = DMA_HANDSHAK_IF_0;  /* Dmac_hs_if0;*/
		
        /* Set the handshaking select*/
        chn_cfg.cfg_hs_sel_src = DMA_HARD_IF;  /* Dmac_hs_hardware;*/
        chn_cfg.cfg_hs_sel_dst = DMA_HARD_IF;  /* Dmac_hs_hardware;*/
		
        /* Set the FIFO mode*/
        /*chn_cfg.cfg_fifo_mode = Dmac_fifo_mode_half;*/

        if (errCode == 0)
        {    
        	errCode = dmaCtrl_Set_ChnCfg(pDma, chnNum, &chn_cfg);
        }

        /* Enable the DMA controller and begin the interrupt driven DMA transfer.*/
        if (errCode == 0)
        {    
        	errCode = dmaCtrl_Start_Xfer(pDma, chnNum, 1);
        }
        
        /*if (errCode == 0)*/
        /*    while (!FMSH_ReadReg(pDma->config.BaseAddress, DMAC_INT_RAW_TFR_L_OFFSET));        */
        if (errCode == 0)
        {    
			do
			{
				tmp32 = dmaCtrl_Rd_CfgReg32(pDma, DMAC_INT_RAW_TFR_L_OFFSET);
		        /* Heartbeat*/
		        /*VX_DBG(".", 1,2,3,4,5,6);				*/
		    } while (!tmp32);	/* transfer is in progress*/
        }

        errCode = dmaCtrl_Clr_IRQ(pDma, chnNum, DMA_IRQ_ALL);   /* Dmac_irq_all */
		
        if (errCode == 0)
        {    
        	errCode = dmaCtrl_Dis_ChnX(pDma, chnNum);
        }
        
        if (errCode == 0)
        {
            if (remain > Dma_Blk_Size)
            {
                remain -= Dma_Blk_Size;
                pBuf32 += (Dma_Blk_Size << 2);
            }
            else
            {
                remain = 0;
            }
        }
        else
        {     
        	/*FMSH_ASSERT(0);*/
        	return FMSH_FAILURE;
        }
    }

	
    timeout = SDMMC_LOOP_TIMEOUT;
    while (!(sdmmcCtrl_RdReg32(pSdmmc, SDMMC_RINTSTS) & RAW_INT_STATUS_DTO))
    {
        delay_1us();
		
        timeout--;
        if (timeout <= 0)
        {
            return -FMSH_ETIME;
        }
    }
    
    return tx_len;
}

int sdmmc_rxData(vxT_SDMMC* pSdmmc, UINT8* pBuf, int rx_len) 
{
	if (pSdmmc->pSdmmcCtrl->xferMode == MODE_XFER_DMA)
	{
		return sdmmc_rxData_DMA(pSdmmc, pBuf, rx_len);
	}
	else
	{
		return sdmmc_rxData_poll(pSdmmc, pBuf, rx_len);
	}
}

int sdmmc_txData(vxT_SDMMC* pSdmmc, UINT8* pBuf, int tx_len)
{
	if (pSdmmc->pSdmmcCtrl->xferMode == MODE_XFER_DMA)
	{
		return sdmmc_txData_DMA(pSdmmc, pBuf, tx_len);
	}
	else
	{
		return sdmmc_txData_poll(pSdmmc, pBuf, tx_len);
	}
}

UINT32 sdmmc_form_CmdVal(UINT32 cardType, UINT8 cmd_idx)
{
    UINT32 cmd_data = 0;
	
    switch (cmd_idx)
    {
        /*No response commands*/
        case CMD_GO_IDLE_STATE:
            cmd_data = (START_CMD + SEND_INITIALIZATION);
        	break;
		
        case CMD_SET_DSR:
        case CMD_GO_INACTIVE_STATE:
            cmd_data = cmd_idx;
        	break;
		
         /*Long response commands*/
        case CMD_ALL_SEND_CID:
        case CMD_SEND_CSD:
        case CMD_SEND_CID:
            cmd_data = (RESPONSE_EXPECT
                      + RESPONSE_LENGTH_LONG 
                      + cmd_idx);
        	break;
		
        /*Short response commands*/
        case CMD_GO_IRQ_STATE:
        case CMD_SEND_OP_COND:
        case CMD_SET_RELATIVE_ADDR:
        case CMD_SET_BLOCKLEN:
        case CMD_SET_BLOCK_COUNT:
        case CMD_SET_WRITE_PROT:
        case CMD_CLR_WRITE_PROT:
        case CMD_ERASE_GROUP_START:
        case CMD_ERASE_GROUP_END:
        case CMD_GEN_CMD:
        case ACMD_OP_COND:
        case CMD_ACMD_SET_BUS_WIDTH:
        case CMD_SELECT_CARD:
        case CMD_SEND_STATUS:
        case CMD_APP_CMD:
        /*case ACMD_SEND_SCR:*/
        case CMD_SEND_IF_COND:
            cmd_data = (RESPONSE_EXPECT
                      + cmd_idx);
        	break;
		
        /*Stop/abort command*/
        case CMD_STOP_TRANSMISSION:
            cmd_data = (RESPONSE_EXPECT
                      + STOP_ABORT_CMD
                      + cmd_idx);
        	break;
		
        /*Have data command write*/
        case CMD_WRITE_BLOCK:
            cmd_data = (WAIT_PRVDATA_COMPLETE
                      + NREADWRITE
                      + DATA_EXPECTED
                      + CHECK_RESPONSE_CRC
                      + RESPONSE_EXPECT
                      + cmd_idx) & ~SEND_AUTO_STOP;
        	break;
		
        case CMD_WRITE_MULTIPLE_BLOCK:
            if (cardType == SD)
            {    cmd_data = (SEND_AUTO_STOP
                          + WAIT_PRVDATA_COMPLETE
                          + NREADWRITE
                          + DATA_EXPECTED
                          + CHECK_RESPONSE_CRC
                          + RESPONSE_EXPECT
                          + cmd_idx);
            }
            else
            {
                cmd_data = (WAIT_PRVDATA_COMPLETE
                          + NREADWRITE
                          + DATA_EXPECTED
                          + CHECK_RESPONSE_CRC
                          + RESPONSE_EXPECT
                          + cmd_idx) & ~SEND_AUTO_STOP;
            }
        	break;
		
        /*Have data command read*/
        case CMD_READ_MULTIPLE_BLOCK:
            if (cardType == SD)
            {    cmd_data = (SEND_AUTO_STOP
                          + WAIT_PRVDATA_COMPLETE
                          + DATA_EXPECTED
                          + CHECK_RESPONSE_CRC
                          + RESPONSE_EXPECT
                          + cmd_idx);
            }
            else
            {
                cmd_data = (WAIT_PRVDATA_COMPLETE
                          + DATA_EXPECTED
                          + CHECK_RESPONSE_CRC
                          + RESPONSE_EXPECT
                          + cmd_idx) & ~SEND_AUTO_STOP;
            }
        	break;
		
        case ACMD_SEND_SCR:			
        case CMD_READ_SINGLE_BLOCK:
            cmd_data = (WAIT_PRVDATA_COMPLETE
                      + DATA_EXPECTED
                      + CHECK_RESPONSE_CRC
                      + RESPONSE_EXPECT
                      + cmd_idx);
        	break;
		
        case CMD_SWITCH_FUNC:
        case MMC_SEND_EXT_CSD:
            cmd_data = (WAIT_PRVDATA_COMPLETE
                      + DATA_EXPECTED
                      + CHECK_RESPONSE_CRC
                      + RESPONSE_EXPECT
                      + (cmd_idx >> 4));
        	break;
		
        default:
            VX_DBG2("unknown command %d.\r\n", cmd_idx, 2,3,4,5,6);
            cmd_data = 0;
        	break;
    }

	if (cmd_data != 0)
	{
	    cmd_data |= START_CMD;
	}
	
    return cmd_data;
}

/*****************************************************************************
*
* @description
* This function does card command generation.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int sdmmc_sndCmd(vxT_SDMMC* pSdmmc, vxT_SDMMC_CMD* pSdmmcCmd)
{
    vxT_SDMMC_CMD * pCmd = pSdmmcCmd;
	
    vxT_SDMMC_CTRL * pSdmmcCtrl = pSdmmc->pSdmmcCtrl;
    vxT_SDMMC_CARD * pSdmmcCard = pSdmmc->pSdmmcCard;
	
    UINT32 cmd_val = 0;
    int timeout;
    int ret = FMSH_SUCCESS;	

	UINT32 tmp32 = 0, raw_int = 0;	

    /**/
    /* detect card before snd cmd*/
    /**/
	if (pSdmmcCtrl->isCardDetect == 1)
	{	
    	tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_CDETECT) & CARD_DETECT;
        if (tmp32 != CARD_INSERT)
        {
			VX_DBG2("sndCmd ctrl_%d: No card insert, exit! \n", pSdmmcCtrl->ctrl_x, 2,3,4,5,6);
			return (-FMSH_ENODEV);
        }
	}

    if (pCmd->rsp_flag & (CMD_RESP_FLAG_RDATA | CMD_RESP_FLAG_WDATA))
    {
        sdmmcCtrl_WrReg32(pSdmmc, SDMMC_BYTCNT, pCmd->data_len);
        sdmmcCtrl_WrReg32(pSdmmc, SDMMC_BLKSIZ, pCmd->block_size);
    }

	/**/
	/*  clear*/
	/**/
	raw_int = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_RINTSTS);
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_RINTSTS, raw_int);
	
    pCmd->cmd_val = sdmmc_form_CmdVal(pSdmmcCard->card_type, pCmd->cmd_idx);

	/**/
	/* response_timeout: 0xFF*/
	/**/
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_TMOUT, 0xFFFFFFFF);
	
    /* 
    Send the command (CRC calculated by host). 
	*/
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CMDARG, pCmd->cmd_arg);
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_CMD_VAL, pCmd->cmd_val);

	/**/
	/* polling cmd finish*/
	/**/
    timeout = SDMMC_POLL_TIMEOUT * 2;
    /*timeout = SDMMC_POLL_TIMEOUT *1000;*/
    do
    {
        delay_1us();		
    	tmp32 = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_RINTSTS) & RAW_INT_STATUS_CMD_DONE; /* bit2 - 命令完成（CD） */
			
        timeout--;
        if (timeout <= 0)
        {
            VX_DBG2("fail: poll CMD[%d] done time_out! \n", pCmd->cmd_idx,2,3,4,5,6);
            return (-FMSH_ETIME);
        }
    } while (!tmp32);
	
    /* */
    /*clear CMDD int */
	/**/
    sdmmcCtrl_WrReg32(pSdmmc, SDMMC_RINTSTS, RAW_INT_STATUS_CMD_DONE);


	/**/
	/* deal with the response of cmd*/
	/**/
    if (pCmd->rsp_type != CMD_RESP_TYPE_NONE) 
    {
		raw_int = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_RINTSTS);
		       
        if (raw_int & (RAW_INT_STATUS_RTO | RAW_INT_STATUS_DRTO))  /* timeout */
        {
			sdmmcCtrl_WrReg32(pSdmmc, SDMMC_RINTSTS, raw_int);
            VX_DBG2("fail: snd CMD[%d] timeout-int_status:0x%08X \n", pCmd->cmd_idx, raw_int, 3,4,5,6);
			
           return (-FMSH_EINVAL);
        }        
        else if (!(pCmd->rsp_flag & CMD_RESP_FLAG_NOCRC)
                 && (raw_int & (RAW_INT_STATUS_RCRC | RAW_INT_STATUS_DCRC)))  /* CRC? */
        {
			sdmmcCtrl_WrReg32(pSdmmc, SDMMC_RINTSTS, raw_int);
            VX_DBG2("fail: snd CMD[%d] CRC error-int_status:0x%08X  \n", pCmd->cmd_idx, raw_int, 3,4,5,6);
			
            return (-FMSH_EINVAL);
        }

        if (pCmd->rsp_flag & CMD_RESP_FLAG_DATALINE) 
        {
            ret = sdmmc_rxData(pSdmmc, (UINT8*)(&pCmd->rsp_buf[0]), pCmd->data_len);
        } 
        else 
        {
            if (gSdmmc_Resp_Len_Type[pCmd->rsp_type] == CMD_RES_LEN_LONG) 
            {    
				pCmd->rsp_buf[0] = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_RESP0);
				pCmd->rsp_buf[1] = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_RESP1);
				pCmd->rsp_buf[2] = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_RESP2);
				pCmd->rsp_buf[3] = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_RESP3);
            }
            else 
            {    
				pCmd->rsp_buf[0] = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_RESP0);
				/*
				pCmd->rsp_buf[1] = 0;
				pCmd->rsp_buf[2] = 0;
				pCmd->rsp_buf[3] = 0;
				pCmd->rsp_buf[1] = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_RESP1);
				pCmd->rsp_buf[2] = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_RESP2);
				pCmd->rsp_buf[3] = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_RESP3);
				*/
            }
			
		#if 0
			UINT32 fifoth_bef, fifoth_aft;
			if (pCmd->cmd_idx == ACMD_SEND_SCR) 
			{
				fifoth_bef = sdmmcCtrl_RdReg32(pSdmmc, SDMMC_FIFOTH);
				
				fifoth_aft =((fifoth_bef >> 16) & 0xF000) | 0x0001;
				fifoth_aft = (fifoth_aft << 16) | (fifoth_bef & 0x0000FFFF);
				sdmmcCtrl_WrReg32(pSdmmc, SDMMC_FIFOTH, fifoth_aft);
			}
		#endif

			
            if (pCmd->rsp_flag & CMD_RESP_FLAG_RDATA) 
            {
                ret = sdmmc_rxData(pSdmmc, pCmd->txRxBuf, pCmd->data_len);
            }

			if (pCmd->rsp_flag & CMD_RESP_FLAG_WDATA) 
            {
                ret = sdmmc_txData(pSdmmc, pCmd->txRxBuf, pCmd->data_len);
            }
			
		#if 0
			if (pCmd->cmd_idx == ACMD_SEND_SCR) 
			{
				sdmmcCtrl_WrReg32(pSdmmc, SDMMC_FIFOTH, fifoth_bef);
			}			
		#endif			
        }		
    } 

    return ret; 
}


#endif


