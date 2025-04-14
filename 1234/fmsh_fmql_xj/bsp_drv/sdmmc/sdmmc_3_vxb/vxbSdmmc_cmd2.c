/* vxbSdmmc_cmd.c - fmsh 7020/7045 sdmmc driver */

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

#include "vxbSdmmc2.h"

/* jc*/
extern void sysUsDelay(int);

const int vxbSdmmc_resp_len[] = 
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
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_cmd0(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CMD* pCmd   = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);
	
    int ret;

	memset((UINT8 *)pCmd, 0, sizeof(vxbT_SDMMC_CMD));
	
    /* 
    go IDLE state 
	*/
    pCmd->cmd_arg = 0x00;
	
    pCmd->cmd_idx = CMD_GO_IDLE_STATE;	
    pCmd->rsp_type = CMD_RESP_TYPE_NONE;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE;
	
    ret = vxbSdmmc_sndCmd(pDev, pCmd);
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
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_cmd1(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CMD* pCmd   = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);	
    vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CARD);
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxbT_SDMMC_CMD));

    pCard->ocr = 0x40FF8000;  /* relative card address */
	
    pCmd->cmd_arg = pCard->ocr;
	
    pCmd->cmd_idx = CMD_SEND_OP_COND;
    pCmd->rsp_type = CMD_RESP_TYPE_R3;
    pCmd->rsp_flag = CMD_RESP_FLAG_NOCRC;
	
    ret = vxbSdmmc_sndCmd(pDev, pCmd);
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
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_cmd2(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CMD* pCmd   = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxbT_SDMMC_CMD));
    
    pCmd->cmd_arg = 0x00;
	
    pCmd->cmd_idx = CMD_ALL_SEND_CID;
    pCmd->rsp_type = CMD_RESP_TYPE_R2;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE; 
	
    ret = vxbSdmmc_sndCmd(pDev, pCmd);
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
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_cmd3(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CMD* pCmd  = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);	
    vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CARD);
	
    int ret;
    UINT16 wNewAddr = 0;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxbT_SDMMC_CMD));
	
    if ((pCard->card_type == MMC) || (pCard->card_type == EMMC))
    {
        wNewAddr = pCard->rca + 1;
        if (wNewAddr == 0) 
		{	
			wNewAddr++;
        }        
        pCard->rca = wNewAddr;
		
        pCmd->cmd_arg = (pCard->rca << 16) & 0xFFFF0000;
    }
    else
    {    
    	pCmd->cmd_arg = 0x00;
    }
	
    pCmd->cmd_idx = CMD_SET_RELATIVE_ADDR;
	
    pCmd->rsp_type = CMD_RESP_TYPE_R6;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE; 
	
	VX_DBG2("CMD3(ask RCA) g_pSdmmcCard->rca: 0x%X \r\n", pCard->rca, 2,3,4,5,6);
	
    ret = vxbSdmmc_sndCmd(pDev, pCmd);
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
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_cmd6_sd(VXB_DEVICE_ID pDev, UINT32 cmdArg, UINT8 *pBuf)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CMD* pCmd = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);	
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxbT_SDMMC_CMD));
    
    pCmd->cmd_arg = cmdArg;
	
    pCmd->cmd_idx = CMD_SWITCH_FUNC;
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_RDATA;
	
    pCmd->txRxBuf = (UINT8 *)pBuf;
	
    pCmd->data_len = 512 / 8;
    pCmd->block_size = 512 / 8;
	
    ret = vxbSdmmc_sndCmd(pDev, pCmd);
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
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_cmd6_mmc(VXB_DEVICE_ID pDev, UINT32 cmdArg)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CMD* pCmd  = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);	
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxbT_SDMMC_CMD));
    
    pCmd->cmd_arg = cmdArg;
	
    pCmd->cmd_idx = CMD_SWITCH_FUNC_MMC;
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE;
	
    ret = vxbSdmmc_sndCmd(pDev, pCmd);
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
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_cmd7(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CMD* pCmd  = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);	
    vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CARD);

    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxbT_SDMMC_CMD));
    
    pCmd->cmd_arg = (pCard->rca << 16) & 0xFFFF0000;
	
    pCmd->cmd_idx = CMD_SELECT_CARD;
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE;
	
    ret = vxbSdmmc_sndCmd(pDev, pCmd);
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
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_cmd8_sd(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CMD* pCmd  = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);	
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxbT_SDMMC_CMD));
    
    pCmd->cmd_arg = SD_CMD8_VOL_PATTERN;
	
    pCmd->cmd_idx = CMD_SEND_IF_COND;
    pCmd->rsp_type = CMD_RESP_TYPE_R7;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE;
	
    ret = vxbSdmmc_sndCmd(pDev, pCmd);
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
*           pDev is the pointer to a sd card device.
*           pBuf is 512 byte buffer pointer for EXT_CSD data.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_cmd8_mmc(VXB_DEVICE_ID pDev, UINT8 *pBuf)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CMD* pCmd  = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);	
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxbT_SDMMC_CMD));

    pCmd->cmd_arg = 0x00;
	
    pCmd->cmd_idx = MMC_SEND_EXT_CSD;
    pCmd->rsp_type = CMD_RESP_TYPE_R7;
    pCmd->rsp_flag = CMD_RESP_FLAG_RDATA;
	
    pCmd->txRxBuf = (UINT8 *)pBuf;
    pCmd->data_len = 512;
	
    pCmd->block_size = 512;
	
    ret = vxbSdmmc_sndCmd(pDev, pCmd);
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
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_cmd9(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CMD* pCmd  = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);	
    vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CARD);
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxbT_SDMMC_CMD));
    
    VX_DBG("pCard->rca:0x%08X \n", pCard->rca, 2,3,4,5,6);
	
    pCmd->cmd_arg = (pCard->rca << 16) & 0xFFFF0000;
	
    pCmd->cmd_idx = CMD_SEND_CSD;
    pCmd->rsp_type = CMD_RESP_TYPE_R2;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE; 
	
    ret = vxbSdmmc_sndCmd(pDev, pCmd);
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
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_cmd12(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CMD* pCmd  = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);	
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxbT_SDMMC_CMD));
    
    pCmd->cmd_arg = 0x00;
	
    pCmd->cmd_idx = CMD_STOP_TRANSMISSION;
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE; 
	
    ret = vxbSdmmc_sndCmd(pDev, pCmd);
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
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_cmd13(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CMD* pCmd  = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);	
    vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CARD);

	int ret = OK;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxbT_SDMMC_CMD));
    
    pCmd->cmd_arg = (pCard->rca << 16) & 0xFFFF0000;
	
    pCmd->cmd_idx = CMD_SEND_STATUS;
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE;
	
    ret = vxbSdmmc_sndCmd(pDev, pCmd);
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
*           pDev is the pointer to a sd card device.
*           len is block length in bytes.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_cmd16(VXB_DEVICE_ID pDev, UINT32 len)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CMD* pCmd  = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);	
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxbT_SDMMC_CMD));
    
    pCmd->cmd_arg = len;
	
    pCmd->cmd_idx = CMD_SET_BLOCKLEN;
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE;
	
    ret = vxbSdmmc_sndCmd(pDev, pCmd);
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
*           pDev is the pointer to a sd card device.
*           len is number of bytes to be transfered.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_cmd23(VXB_DEVICE_ID pDev, UINT32 len)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CMD* pCmd  = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);	
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxbT_SDMMC_CMD));
	
    pCmd->cmd_arg = len >> 9;

    pCmd->cmd_idx = CMD_SET_BLOCK_COUNT;
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE;
	
    ret = vxbSdmmc_sndCmd(pDev, pCmd);
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
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_cmd55(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CMD* pCmd  = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);	
    vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CARD);
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxbT_SDMMC_CMD));
	
    /* 
     CMD55 APP_CMD Mandatory特殊指令前命令,在发送ACMD类指令前,需要发送此命令 
	*/  
	
    pCmd->cmd_arg = (pCard->rca << 16) & 0xFFFF0000;
	
    pCmd->cmd_idx = CMD_APP_CMD;
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE;
	
    ret = vxbSdmmc_sndCmd(pDev, pCmd);
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
*           pDev is the pointer to a sd card device.
*           Arg is the command's argument.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_appCmd6(VXB_DEVICE_ID pDev, UINT32 cmdArg)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CMD* pCmd  = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);	
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxbT_SDMMC_CMD));
    
    pCmd->cmd_arg = cmdArg;
	
    pCmd->cmd_idx = CMD_ACMD_SET_BUS_WIDTH;
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_NONE;
	
    ret = vxbSdmmc_sndCmd(pDev, pCmd);
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
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_appCmd41(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CMD* pCmd  = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);	
    vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CARD);
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxbT_SDMMC_CMD));

    pCard->ocr = 0x00FF8000;
	
    if(pCard->card_ver == SDMMC_CARD_VER_1_0)
    {    
    	pCmd->cmd_arg = pCard->ocr;
    }
    else
    {    
    	pCmd->cmd_arg = pCard->ocr | 0x40000000;  /* #define OCR_CARD_CAP_STS    (1 << 30):0x40000000*/
    }
	
    pCmd->cmd_idx = ACMD_OP_COND;	
    pCmd->rsp_type = CMD_RESP_TYPE_R3;
    pCmd->rsp_flag = CMD_RESP_FLAG_NOCRC;
	
    ret = vxbSdmmc_sndCmd(pDev, pCmd);
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
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_appCmd51(VXB_DEVICE_ID pDev, UINT8 *pBuf)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CMD* pCmd  = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);	
	
    int ret;
	
	memset((UINT8 *)pCmd, 0, sizeof(vxbT_SDMMC_CMD));
    
    pCmd->cmd_arg = 0x00;
	
    pCmd->cmd_idx = ACMD_SEND_SCR;
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_RDATA;
	
    pCmd->txRxBuf = (UINT8 *)pBuf;
    pCmd->data_len = 8;
    pCmd->block_size = 8;
	
    ret = vxbSdmmc_sndCmd(pDev, pCmd);
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
*           pDev is the pointer to a sd card device.
*           bus_width is bus width.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_sdACMD6(VXB_DEVICE_ID pDev, UINT32 bus_width)
{
    int ret;
	
    ret = vxbSdmmc_cmd55(pDev);
    if(ret < 0)
    {
        return ret;
    }

    ret = vxbSdmmc_appCmd6(pDev, bus_width);
    
    return ret;    
}

/*****************************************************************************
*
* @description
* This function asks card to send their operation conditions.
*
* @param
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_sdACMD41(VXB_DEVICE_ID pDev)
{
    int ret;
	
    ret = vxbSdmmc_cmd55(pDev);
    if(ret < 0)
    {
        return ret;
    }

    ret = vxbSdmmc_appCmd41(pDev);
    
    return ret;    
}

/*****************************************************************************
*
* @description
* This function reads the SD configuration register(SCR).
*
* @param
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmc_sdACMD51(VXB_DEVICE_ID pDev, UINT8 *pBuf)
{
    int ret;
	
    ret = vxbSdmmc_cmd55(pDev);
    if(ret < 0)
    {
        return ret;
    }

    ret = vxbSdmmc_appCmd51(pDev, pBuf);
    
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
static int vxbSdmmc_getResLen(int res_type)
{
    return vxbSdmmc_resp_len[res_type];
}

#endif


#if 1


int vxbSdmmc_rxData_poll(VXB_DEVICE_ID pDev, UINT8 *pBuf, int rx_len) 
{
    int timeout = 0;
    UINT32 tmp32 = 0;
    int cnt = 0;
    
    if (((UINT32)pBuf) & 0x03)  /* the address roundup at 4bytes*/
    {
        timeout = 0x100000;
        while (cnt < rx_len)
        {
            timeout--;
            if (!(vxbSdmmcCtrl_RdReg32(pDev, SDMMC_STATUS) & FIFO_EMPTY))
            {
                tmp32 = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_FIFO);
				
                pBuf[cnt++] = tmp32 & 0xff;
                pBuf[cnt++] = (tmp32 >> 8) & 0xff;
                pBuf[cnt++] = (tmp32 >> 16) & 0xff;
                pBuf[cnt++] = (tmp32 >> 24) & 0xff;
            }

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
            timeout--;
			
            if (!(vxbSdmmcCtrl_RdReg32(pDev, SDMMC_STATUS) & FIFO_EMPTY))
            {
                *(UINT32 *)(pBuf + cnt) = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_FIFO);
                cnt += 4;
            }

            if (timeout <= 0)
            {
                return (-FMSH_ETIME);
            }
        }
        
    }
    
    return cnt;
}

int vxbSdmmc_txData_poll(VXB_DEVICE_ID pDev, UINT8 *pBuf, int tx_len)
{
    UINT32 tmp32;
    int timeout = 0;
	int cnt = 0;
    
    if (((UINT32)pBuf) & 0x03)  /* the address roundup: 4 bytes */
    {
        timeout = 0x100000;
		
        while (cnt < tx_len)
        {
            timeout--;
			
            if (!(vxbSdmmcCtrl_RdReg32(pDev, SDMMC_STATUS) & FIFO_FULL))
            {
                tmp32 = pBuf[cnt++];
                tmp32 += (pBuf[cnt++] << 8);
                tmp32 += (pBuf[cnt++] << 16);
                tmp32 += (pBuf[cnt++] << 24);
				
                vxbSdmmcCtrl_WrReg32(pDev, SDMMC_FIFO, tmp32);
            }

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
            if (!(vxbSdmmcCtrl_RdReg32(pDev, SDMMC_STATUS) & FIFO_FULL))
            {
                vxbSdmmcCtrl_WrReg32(pDev, SDMMC_FIFO, *(UINT32 *)(pBuf + cnt));
                cnt += 4;
            }

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
        sysUsDelay(1);
        tmp32 = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_RINTSTS);
		
        timeout--;
        if (timeout <= 0)
        {
            return (-FMSH_ETIME);
        }
    } while (!(tmp32 & RAW_INT_STATUS_DTO)); /* bit3 - 数据传输（DTO）*/
    
    return cnt;
}


UINT32 vxbSdmmc_form_CmdVal(UINT32 cardType, UINT8 cmd_idx)
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
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int vxbSdmmc_sndCmd(VXB_DEVICE_ID pDev, vxbT_SDMMC_CMD* pSdmmcCmd)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	
    vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CARD);
    vxbT_SDMMC_CTRL* pCtrl = (vxbT_SDMMC_CTRL*)(&pDrvCtrl->CTRL);
		
    UINT32 cmd_val = 0;
    int timeout;
    int ret = OK;	

	UINT32 tmp32 = 0, raw_int = 0;	

    /**/
    /* detect card before snd cmd*/
    /**/
	if (pCtrl->isCardDetect == 1)
	{	
    	tmp32 = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_CDETECT) & CARD_DETECT;
        if (tmp32 != CARD_INSERT)
        {
			VX_DBG2("sndCmd ctrl_%d: No card insert, exit! \n", pCtrl->ctrl_x, 2,3,4,5,6);
			return (-FMSH_ENODEV);
        }
	}

    if (pSdmmcCmd->rsp_flag & (CMD_RESP_FLAG_RDATA | CMD_RESP_FLAG_WDATA))
    {
        vxbSdmmcCtrl_WrReg32(pDev, SDMMC_BYTCNT, pSdmmcCmd->data_len);
        vxbSdmmcCtrl_WrReg32(pDev, SDMMC_BLKSIZ, pSdmmcCmd->block_size);
    }
	
	raw_int = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_RINTSTS);
    vxbSdmmcCtrl_WrReg32(pDev, SDMMC_RINTSTS, raw_int);
	
    pSdmmcCmd->cmd_val = vxbSdmmc_form_CmdVal(pCard->card_type, pSdmmcCmd->cmd_idx);

	/**/
	/* response_timeout: 0xFF*/
	/**/
    vxbSdmmcCtrl_WrReg32(pDev, SDMMC_TMOUT, 0xFFFFFFFF);
	
    /* 
    Send the command (CRC calculated by host). 
	*/
    vxbSdmmcCtrl_WrReg32(pDev, SDMMC_CMDARG, pSdmmcCmd->cmd_arg);
    vxbSdmmcCtrl_WrReg32(pDev, SDMMC_CMD_VAL, pSdmmcCmd->cmd_val);

	/**/
	/* polling cmd finish*/
	/**/
    timeout = SDMMC_POLL_TIMEOUT * 2;
    do
    {
        sysUsDelay(1);
		
    	tmp32 = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_RINTSTS) & RAW_INT_STATUS_CMD_DONE; /* bit2 - 命令完成（CD） */
			
        timeout--;
        if (timeout <= 0)
        {
            VX_DBG2("fail: poll CMD[%d] done time_out! \n", pSdmmcCmd->cmd_idx,2,3,4,5,6);
            return (-FMSH_ETIME);
        }
    } while (!tmp32);
	
    /* */
    /*clear CMDD int */
	/**/
    vxbSdmmcCtrl_WrReg32(pDev, SDMMC_RINTSTS, RAW_INT_STATUS_CMD_DONE);


	/**/
	/* deal with the response of cmd*/
	/**/
    if (pSdmmcCmd->rsp_type != CMD_RESP_TYPE_NONE) 
    {
		raw_int = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_RINTSTS);
		       
        if (raw_int & (RAW_INT_STATUS_RTO | RAW_INT_STATUS_DRTO))  /* timeout */
        {
			vxbSdmmcCtrl_WrReg32(pDev, SDMMC_RINTSTS, raw_int);
            VX_DBG2("fail: snd CMD[%d] timeout-int_status:0x%08X \n", pSdmmcCmd->cmd_idx, raw_int, 3,4,5,6);
			
           return (-FMSH_EINVAL);
        }        
        else if (!(pSdmmcCmd->rsp_flag & CMD_RESP_FLAG_NOCRC)
                 && (raw_int & (RAW_INT_STATUS_RCRC | RAW_INT_STATUS_DCRC)))  /* CRC? */
        {
			vxbSdmmcCtrl_WrReg32(pDev, SDMMC_RINTSTS, raw_int);
            VX_DBG2("fail: snd CMD[%d] CRC error-int_status:0x%08X  \n", pSdmmcCmd->cmd_idx, raw_int, 3,4,5,6);
			
            return (-FMSH_EINVAL);
        }

        if (pSdmmcCmd->rsp_flag & CMD_RESP_FLAG_DATALINE) 
        {
            ret = vxbSdmmc_rxData_poll(pDev, (UINT8 *)pSdmmcCmd->rsp_buf, pSdmmcCmd->data_len);
        } 
        else 
        {
            if (vxb_gSdmmc_Resp_Len_Type[pSdmmcCmd->rsp_type] == CMD_RES_LEN_LONG) 
            {    
				pSdmmcCmd->rsp_buf[0] = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_RESP0);
				pSdmmcCmd->rsp_buf[1] = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_RESP1);
				pSdmmcCmd->rsp_buf[2] = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_RESP2);
				pSdmmcCmd->rsp_buf[3] = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_RESP3);
            }
            else 
            {    
				pSdmmcCmd->rsp_buf[0] = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_RESP0);
				pSdmmcCmd->rsp_buf[1] = 0;
				pSdmmcCmd->rsp_buf[2] = 0;
				pSdmmcCmd->rsp_buf[3] = 0;
            }
            
            if (pSdmmcCmd->rsp_flag & CMD_RESP_FLAG_RDATA) 
            {
                ret = vxbSdmmc_rxData_poll(pDev, pSdmmcCmd->txRxBuf, pSdmmcCmd->data_len);
            }
			else if (pSdmmcCmd->rsp_flag & CMD_RESP_FLAG_WDATA) 
            {
                ret = vxbSdmmc_txData_poll(pDev, pSdmmcCmd->txRxBuf, pSdmmcCmd->data_len);
            }
        }
    } 

    return ret; 
}


#endif


