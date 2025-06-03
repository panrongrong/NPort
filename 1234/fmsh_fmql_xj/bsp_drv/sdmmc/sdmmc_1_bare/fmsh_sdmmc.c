/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_sdmmc.c
*
* This file contains
*
* @note None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who     Date            Changes
* ----- ---- -------- ---------------------------------------------
* 0.01  yl      12/20/2018      First Release
*</pre>
******************************************************************************/

/***************************** Include Files *********************************/

#include <stdio.h>
#include <string.h>

#include "../../common/fmsh_common.h"

#include "fmsh_sdmmc_lib.h"

/*#include "fmsh_dmac_lib.h"*/
#include "../../dma/dma_1_bare/fmsh_dmac_lib.h"

/************************** Constant Definitions *****************************/

const int sdmmc_res_len[] = {
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

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

#if 1
/*****************************************************************************
*
* @description
* This function is used to check if a command has been accepted by the card.
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
static int sdmmc_waitStartCmdCleared(FSdmmcPs_T *pSdmmc)
{
    u32 timeout_cnt = SDMMC_LOOP_TIMEOUT;
    
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    while (FMSH_ReadReg(pSdmmc->config.BaseAddress, SDMMC_CMD_VAL) & START_CMD)
    {
        timeout_cnt--;
        delay_1us();
        if (timeout_cnt == 0)
        {
            return -FMSH_ETIME;
        }
    }

    return 0;
}

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
static int sdmmc_cmd0(FSdmmcPs_T *pSdmmc)
{
    int ret;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;
    
    /* go IDLE state */
    pCmd->cmd = CMD_GO_IDLE_STATE;
    pCmd->arg = 0;
    pCmd->res_type = CMD_RESP_TYPE_NONE;
    pCmd->res_flag = 0;
    ret = pInstance->sendCmd(pSdmmc);
    if (ret < 0)
    {
        TRACE_OUT(DEBUG_OUT, "CMD0 failed: %d\r\n", ret);
    }

    return ret;
}

/*****************************************************************************
*
* @description
* This function sends command 1, only used for MMC/EMMC.
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
static int sdmmc_cmd1(FSdmmcPs_T *pSdmmc)
{
    int ret;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;

    pInstance->ocr = 0x40FF8000;
    pCmd->cmd = CMD_SEND_OP_COND;
    pCmd->arg = pInstance->ocr;
    pCmd->res_type = CMD_RESP_TYPE_R3;
    pCmd->res_flag = CMD_RESP_FLAG_NOCRC;
    ret = pInstance->sendCmd(pSdmmc);
    if (ret < 0)
    {
        TRACE_OUT(DEBUG_OUT, "CMD1 failed: %d\r\n", ret);
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
static int sdmmc_cmd2(FSdmmcPs_T *pSdmmc)
{
    int ret;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;
    
    pCmd->cmd = CMD_ALL_SEND_CID;
    pCmd->arg = 0;
    pCmd->res_type = CMD_RESP_TYPE_R2;
    pCmd->res_flag = 0; 
    ret = pInstance->sendCmd(pSdmmc);
    if (ret < 0)
    {
        TRACE_OUT(DEBUG_OUT, "CMD2 failed: %d\r\n", ret);
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
static int sdmmc_cmd3(FSdmmcPs_T *pSdmmc)
{
    int ret;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;
    
    pCmd->cmd = CMD_SET_RELATIVE_ADDR;
    if ((pInstance->card_type == MMC) || (pInstance->card_type == EMMC))
    {
        u16 wNewAddr = pInstance->rca + 1;
		
        if (wNewAddr == 0) 
			wNewAddr ++;
        
        pInstance->rca = wNewAddr;
		
        pCmd->arg = (pInstance->rca << 16) & 0xFFFF0000;
    }
    else
        pCmd->arg = 0;
	
    pCmd->res_type = CMD_RESP_TYPE_R6;
    pCmd->res_flag = 0; 
    ret = pInstance->sendCmd(pSdmmc);
    if (ret < 0)
    {
        TRACE_OUT(DEBUG_OUT, "CMD3 failed: %d\r\n", ret);
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
static int sdmmc_cmd6_sd(FSdmmcPs_T *pSdmmc, u32 Arg, u8 *pBuf)
{
    int ret;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;
    
    pCmd->cmd = CMD_SWITCH_FUNC;
    pCmd->arg = Arg;
    pCmd->res_type = CMD_RESP_TYPE_R1;
    pCmd->res_flag = CMD_RESP_FLAG_RDATA;
    pCmd->data = (char *)pBuf;
    pCmd->data_len = 512 / 8;
    pCmd->block_size = 512 / 8;
    ret = pInstance->sendCmd(pSdmmc);
    
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
static int sdmmc_cmd6_mmc(FSdmmcPs_T *pSdmmc, u32 Arg)
{
    int ret;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;
    
    pCmd->cmd = CMD_SWITCH_AND_ACMD_SET_BUS_WIDTH;
    pCmd->arg = Arg;
    pCmd->res_type = CMD_RESP_TYPE_R1;
    pCmd->res_flag = 0;
    ret = pInstance->sendCmd(pSdmmc);
    
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
static int sdmmc_cmd7(FSdmmcPs_T *pSdmmc)
{
    int ret;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;
    
    pCmd->cmd = CMD_SELECT_CARD;
    pCmd->arg = (pInstance->rca << 16) & 0xFFFF0000;
    pCmd->res_type = CMD_RESP_TYPE_R1;
    pCmd->res_flag = 0;
    ret = pInstance->sendCmd(pSdmmc);
    if (ret < 0)
    {
        TRACE_OUT(DEBUG_OUT, "CMD7 failed: %d\r\n", ret);
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
static int sdmmc_cmd8_sd(FSdmmcPs_T *pSdmmc)
{
    int ret;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;
    
    pCmd->cmd = CMD_SEND_IF_COND;
    pCmd->arg = SD_CMD8_VOL_PATTERN;
    pCmd->res_type = CMD_RESP_TYPE_R7;
    pCmd->res_flag = 0;
    ret = pInstance->sendCmd(pSdmmc);

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
static int sdmmc_cmd8_mmc(FSdmmcPs_T *pSdmmc, u8 *pBuf)
{
    int ret;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;

    pCmd->cmd = MMC_SEND_EXT_CSD;
    pCmd->arg = 0;
    pCmd->res_type = CMD_RESP_TYPE_R7;
    pCmd->res_flag = CMD_RESP_FLAG_RDATA;
    pCmd->data = (char *)pBuf;
    pCmd->data_len = 512;
    pCmd->block_size = 512;
    ret = pInstance->sendCmd(pSdmmc);
    if (ret < 0)
    {
        return ret;
    }
    
    return 0;
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
static int sdmmc_cmd9(FSdmmcPs_T *pSdmmc)
{
    int ret;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;
    
    pCmd->cmd = CMD_SEND_CSD;
    pCmd->arg = (pInstance->rca << 16) & 0xFFFF0000;
    pCmd->res_type = CMD_RESP_TYPE_R2;
    pCmd->res_flag = 0; 
    ret = pInstance->sendCmd(pSdmmc);
    if (ret < 0)
    {
        TRACE_OUT(DEBUG_OUT, "CMD9 failed: %d\r\n", ret);
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
static int sdmmc_cmd12(FSdmmcPs_T *pSdmmc)
{
    int ret;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;
    
    pCmd->cmd = CMD_STOP_TRANSMISSION;
    pCmd->arg = 0;
    pCmd->res_type = CMD_RESP_TYPE_R1;
    pCmd->res_flag = 0; 
    ret = pInstance->sendCmd(pSdmmc);
    if (ret < 0)
    {
        TRACE_OUT(DEBUG_OUT, "CMD12 failed: %d\r\n", ret);
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
static int sdmmc_cmd13(FSdmmcPs_T *pSdmmc)
{
    int ret;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;
    
    pCmd->cmd = CMD_SEND_STATUS;
    pCmd->arg = (pInstance->rca << 16) & 0xFFFF0000;
    pCmd->res_type = CMD_RESP_TYPE_R1;
    pCmd->res_flag = 0;
	
    ret = pInstance->sendCmd(pSdmmc);

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
static int sdmmc_cmd16(FSdmmcPs_T *pSdmmc, unsigned int len)
{
    int ret;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;
    
    pCmd->cmd = CMD_SET_BLOCKLEN;
    pCmd->arg = len;
    pCmd->res_type = CMD_RESP_TYPE_R1;
    pCmd->res_flag = 0;
    ret = pInstance->sendCmd(pSdmmc);

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
static int sdmmc_cmd23(FSdmmcPs_T *pSdmmc, u32 len)
{
    int ret;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;

    pCmd->cmd = CMD_SET_BLOCK_COUNT;
    pCmd->arg = len >> 9;
    pCmd->res_type = CMD_RESP_TYPE_R1;
    pCmd->res_flag = 0;
    ret = pInstance->sendCmd(pSdmmc);

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
static int sdmmc_cmd55(FSdmmcPs_T *pSdmmc)
{
    int ret;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;
    
    pCmd->cmd = CMD_APP_CMD;
    pCmd->arg = (pInstance->rca << 16) & 0xFFFF0000;
    pCmd->res_type = CMD_RESP_TYPE_R1;
    pCmd->res_flag = 0;
    ret = pInstance->sendCmd(pSdmmc);
    if (ret < 0)
    {
        TRACE_OUT(DEBUG_OUT, "CMD55 failed: %d\r\n", ret);
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
static int sdmmc_appCmd6(FSdmmcPs_T *pSdmmc, unsigned int Arg)
{
    int ret;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;
    
    pCmd->cmd = CMD_SWITCH_AND_ACMD_SET_BUS_WIDTH;
    pCmd->arg = Arg;
    pCmd->res_type = CMD_RESP_TYPE_R1;
    pCmd->res_flag = 0;
    ret = pInstance->sendCmd(pSdmmc);
    
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
static int sdmmc_appCmd41(FSdmmcPs_T *pSdmmc)
{
    int ret;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;

    pInstance->ocr = 0x00FF8000;
    pCmd->cmd = ACMD_OP_COND;
	
    if(pInstance->version == SD_VERSION_2)
        pCmd->arg = pInstance->ocr | OCR_HCS;
    else
        pCmd->arg = pInstance->ocr;
	
    pCmd->res_type = CMD_RESP_TYPE_R3;
    pCmd->res_flag = CMD_RESP_FLAG_NOCRC;
	
    ret = pInstance->sendCmd(pSdmmc);
    
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
static int sdmmc_appCmd51(FSdmmcPs_T *pSdmmc, u8 *pBuf)
{
    int ret;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;
    
    pCmd->cmd = ACMD_SEND_SCR;
    pCmd->arg = 0;
    pCmd->res_type = CMD_RESP_TYPE_R1;
    pCmd->res_flag = CMD_RESP_FLAG_RDATA;
    pCmd->data = (char *)pBuf;
    pCmd->data_len = 8;
    pCmd->block_size = 8;
    ret = pInstance->sendCmd(pSdmmc);
    
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
static int sdmmc_sdACMD6(FSdmmcPs_T *pSdmmc, unsigned int bus_width)
{
    int ret;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);

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
static int sdmmc_sdACMD41(FSdmmcPs_T *pSdmmc)
{
    int ret;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);

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
static int sdmmc_sdACMD51(FSdmmcPs_T *pSdmmc, u8 *pBuf)
{
    int ret;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);

    ret = sdmmc_cmd55(pSdmmc);
    if(ret < 0)
    {
        return ret;
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
    return sdmmc_res_len[res_type];
}
#endif

/*****************************************************************************
*
* @description
* This function switches card state between STBY and TRAN
* (or CMD and TRAN).
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
static int FSdmmcPs_selectCard(FSdmmcPs_T *pSdmmc)
{
    int ret;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);

    /* switch to TRAN mode to Select the current SD/MMC*/
    ret = sdmmc_cmd7(pSdmmc);

    return ret;
}

/*****************************************************************************
*
* @description
* This function reads data from card. 
* User can read data with DMA or get from FIFO directly according to the
* transfer mode.
*
* @param
*           baseAddr is the controller base address.
*           pDma is pointer to the DMA controller device.
*           buf is data buffer whose size is at least the block size.
*           len is number of bytes to be read.
*           trans_mode is enumerated data transfer mode.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int FSdmmcPs_readData(u32 baseAddr, FDmaPs_T *pDma, char *buf, int len, enum SDMMC_TransMode trans_mode) 
{
    u32 timeout_cnt = 0;
    
    if (trans_mode == sdmmc_trans_mode_dw_dma)
    {
        int errorCode;
        enum FDmaPs_channelNumber chNum;
        FDmaPs_ChannelConfig_T ch_config;
        int *buffer = (int *)buf;
        unsigned int remain = len >> 2;
        
        while (remain)
        {
            /* Transfer characteristics*/
            chNum = FDmaPs_getFreeChannel(pDma); /* Select a DMA channel*/
            
            errorCode = FDmaPs_setTransferType(pDma, chNum, Dmac_transfer_row1);
            if (errorCode == 0)
                errorCode = FDmaPs_enableChannelIrq(pDma, chNum);
			
            if (errorCode == 0)
                errorCode = FDmaPs_getChannelConfig(pDma, chNum, &ch_config);
            
            /* Set the Source and destination addresses*/
            ch_config.sar = baseAddr + SDMMC_FIFO;
            ch_config.dar = (unsigned int )buffer;

			printf("FSdmmcPs_readData(DMA):sar-0x%X len-%d \n", ch_config.sar, len);
			
            /* Set the source and destination transfer width*/
            ch_config.ctl_src_tr_width = Dmac_trans_width_32;
            ch_config.ctl_dst_tr_width = Dmac_trans_width_32;
			
            /* Set the Address increment type for the source and destination*/
            ch_config.ctl_sinc = Dmac_addr_nochange;
            ch_config.ctl_dinc = Dmac_addr_increment;
            /* Set the source and destination burst transaction length*/
            ch_config.ctl_src_msize = Dmac_msize_8;
            ch_config.ctl_dst_msize = Dmac_msize_8;
            
            /* Set the block size for the DMA transfer*/
            if (remain > 0xfe0)
                ch_config.ctl_block_ts = 0xfe0;
            else
                ch_config.ctl_block_ts = remain;
            
            /* Set the transfer device type and flow controller*/
            ch_config.ctl_tt_fc = Dmac_prf2mem_dma;
            /* Set the hardware handshaking interface*/
            ch_config.cfg_dst_per = Dmac_hs_if0;
            ch_config.cfg_src_per = Dmac_hs_if0;
            /* Set the handshaking select*/
            ch_config.cfg_hs_sel_src = Dmac_hs_hardware;
            ch_config.cfg_hs_sel_dst = Dmac_hs_hardware;
            /* Set the FIFO mode*/
            /*ch_config.cfg_fifo_mode = Dmac_fifo_mode_half;*/
            
            if (errorCode == 0)
                errorCode = FDmaPs_setChannelConfig(pDma, chNum, &ch_config);

#if PSOC_CACHE_ENABLE
    flush_dcache_range(ch_config.sar, (ch_config.sar + remain * 4));
    invalidate_dcache_range(ch_config.dar, (ch_config.dar + remain * 4));
#endif
            /* Enable the DMA controller and begin the interrupt driven DMA transfer.*/
            if (errorCode == 0)
                errorCode = FDmaPs_startTransfer(pDma, chNum, 1, NULL);
            
           
           if (errorCode == 0)
                while (!FMSH_ReadReg(pDma->config.BaseAddress, DMAC_INT_RAW_TFR_L_OFFSET));
		   

            errorCode = FDmaPs_clearIrq(pDma, chNum, Dmac_irq_all);
            if (errorCode == 0)
                errorCode = FDmaPs_disableChannel(pDma, chNum);
            
            if (errorCode == 0)
            {
                if (remain > 0xfc0)
                {
                    remain -= 0xfc0;
                    buffer += 0xfc0 << 2;
                }
                else
                {
                    remain = 0;
                }
            }
            else
                FMSH_ASSERT(0);
        }
    }
    else
    {
        u32 temp;
        unsigned int count = 0;
		
		/*printf("FSdmmcPs_readData(Poll):len-%d \n", len);*/
        
        if (((int)buf) & 0x03)
        {
            timeout_cnt = 0x100000;
            while(count < len)
            {
                timeout_cnt--;
                if(!(FMSH_ReadReg(baseAddr, SDMMC_STATUS) & FIFO_EMPTY))
                {
                    temp = FMSH_ReadReg(baseAddr, SDMMC_FIFO);
					
                    buf[count++] = temp & 0xff;
                    buf[count++] = (temp>>8) & 0xff;
                    buf[count++] = (temp>>16) & 0xff;
                    buf[count++] = (temp>>24) & 0xff;
                }

                if (timeout_cnt == 0)
                {
                    return -FMSH_ETIME;
                }
            }
        }
        else
        {
            timeout_cnt = 0x100000;
            while(count < len)
            {
                timeout_cnt--;
                if(!(FMSH_ReadReg(baseAddr, SDMMC_STATUS) & FIFO_EMPTY))
                {
                    *(unsigned int *)(buf + count) = FMSH_ReadReg(baseAddr, SDMMC_FIFO);
                    count += 4;
                }

                if (timeout_cnt == 0)
                {
                    return -FMSH_ETIME;
                }
            }
            
        }
    }

    if (trans_mode != sdmmc_trans_mode_dw_dma)
    {
        timeout_cnt = SDMMC_LOOP_TIMEOUT;
        while(!(FMSH_ReadReg(baseAddr, SDMMC_RINTSTS) & RAW_INT_STATUS_DTO))
        {
            timeout_cnt--;
            delay_1us();
            if (timeout_cnt == 0)
            {
                return -FMSH_ETIME;
            }
        }
    }
    
    return 0;
}

/*****************************************************************************
*
* @description
* This function writes data to card. 
* User can write data with DMA or send to FIFO directly according to the
* transfer mode.
*
* @param
*           baseAddr is the controller base address.
*           pDma is pointer to the DMA controller device.
*           buf is data buffer whose size is at least the block size.
*           len is number of bytes to be write.
*           trans_mode is enumerated data transfer mode.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int FSdmmcPs_writeData(u32 baseAddr, FDmaPs_T *pDma, char *buf, int len, enum SDMMC_TransMode trans_mode)
{
    u32 tmp;
    u32 timeout_cnt = 0;

    if (trans_mode == sdmmc_trans_mode_dw_dma)
    {
        int errorCode;
        enum FDmaPs_channelNumber chNum;
        FDmaPs_ChannelConfig_T ch_config;
        int *buffer = (int *)buf;
        unsigned int remain = len >> 2;
        
        while (remain)
        {
            /* Transfer characteristics*/
            chNum = FDmaPs_getFreeChannel(pDma); /* Select a DMA channel*/
            errorCode = FDmaPs_setTransferType(pDma, chNum, Dmac_transfer_row1);
			
            if (errorCode == 0)
                errorCode = FDmaPs_enableChannelIrq(pDma, chNum);
			
            if (errorCode == 0)
                errorCode = FDmaPs_getChannelConfig(pDma, chNum, &ch_config);
            
            /* Set the Source and destination addresses*/
            ch_config.sar = (unsigned int )buffer;
            ch_config.dar = baseAddr + SDMMC_FIFO;
            /* Set the source and destination transfer width*/
            ch_config.ctl_src_tr_width = Dmac_trans_width_32;
            ch_config.ctl_dst_tr_width = Dmac_trans_width_32;
            /* Set the Address increment type for the source and destination*/
            ch_config.ctl_sinc = Dmac_addr_increment;
            ch_config.ctl_dinc = Dmac_addr_nochange;
            /* Set the source and destination burst transaction length*/
            ch_config.ctl_src_msize = Dmac_msize_8;
            ch_config.ctl_dst_msize = Dmac_msize_8;
            
            /* Set the block size for the DMA transfer*/
            if (remain > 0x400)
                ch_config.ctl_block_ts = 0x400;
            else
                ch_config.ctl_block_ts = remain;
            
            /* Set the transfer device type and flow controller*/
            ch_config.ctl_tt_fc = Dmac_mem2prf_dma;
            /* Set the hardware handshaking interface*/
            ch_config.cfg_dst_per = Dmac_hs_if0;
            ch_config.cfg_src_per = Dmac_hs_if0;
            /* Set the handshaking select*/
            ch_config.cfg_hs_sel_src = Dmac_hs_hardware;
            ch_config.cfg_hs_sel_dst = Dmac_hs_hardware;
            /* Set the FIFO mode*/
            /*ch_config.cfg_fifo_mode = Dmac_fifo_mode_half;*/

            if (errorCode == 0)
                errorCode = FDmaPs_setChannelConfig(pDma, chNum, &ch_config);

#if PSOC_CACHE_ENABLE
    flush_dcache_range(ch_config.sar, (ch_config.sar + remain * 4));
    invalidate_dcache_range(ch_config.dar, (ch_config.dar + remain * 4));
#endif
            /* Enable the DMA controller and begin the interrupt driven DMA transfer.*/
            if (errorCode == 0)
                errorCode = FDmaPs_startTransfer(pDma, chNum, 1, NULL);
            
            
             if (errorCode == 0)
                while (!FMSH_ReadReg(pDma->config.BaseAddress, DMAC_INT_RAW_TFR_L_OFFSET));
            

            errorCode = FDmaPs_clearIrq(pDma, chNum, Dmac_irq_all);
            if (errorCode == 0)
                errorCode = FDmaPs_disableChannel(pDma, chNum);
            
            if (errorCode == 0)
            {
                if (remain > 0x400)
                {
                    remain -= 0x400;
                    buffer += (0x400 << 2);
                }
                else
                {
                    remain = 0;
                }
            }
            else
                FMSH_ASSERT(0);
        }
    }
    else
    {
        unsigned int count = 0;

        if (((int)buf) & 0x03)
        {
            timeout_cnt = 0x100000;
            while(count < len)
            {
                timeout_cnt--;
                if(!(FMSH_ReadReg(baseAddr, SDMMC_STATUS) & FIFO_FULL))
                {
                    unsigned int temp;
                    temp = buf[count++];
                    temp += (buf[count++]<<8);
                    temp += (buf[count++]<<16);
                    temp += (buf[count++]<<24);
                    FMSH_WriteReg(baseAddr, SDMMC_FIFO, temp);
                }

                if (timeout_cnt == 0)
                {
                    return -FMSH_ETIME;
                }
            }
        }
        else
        {
            timeout_cnt = 0x100000;
            while(count < len)
            {
                timeout_cnt--;
                if(!(FMSH_ReadReg(baseAddr, SDMMC_STATUS) & FIFO_FULL))
                {
                    FMSH_WriteReg(baseAddr, SDMMC_FIFO, *(unsigned int *)(buf + count));
                    count += 4;
                }

                if (timeout_cnt == 0)
                {
                    return -FMSH_ETIME;
                }
            }
        }
    }

    tmp = FMSH_ReadReg(baseAddr, SDMMC_RINTSTS);
    timeout_cnt = SDMMC_LOOP_TIMEOUT;
    while(!(tmp & RAW_INT_STATUS_DTO))
    {
        tmp = FMSH_ReadReg(baseAddr, SDMMC_RINTSTS);
        timeout_cnt--;
        delay_1us();
        if (timeout_cnt == 0)
        {
            return -FMSH_ETIME;
        }
    }
    
    return 0;
}

/*****************************************************************************
*
* @description
* This function gets the short type card response.
*
* @param
*           pCmd is pointer to the card request structure.
*           baseAddr is the controller base address.
*
* @return
*           NA.
*
* @note     NA.
*
*****************************************************************************/
static void FSdmmcPs_sdGetShortReply(FSdmmcPs_SdCmd_T *pCmd, u32 baseAddr)
{
    pCmd->rsp_buf[0] = FMSH_ReadReg(baseAddr, SDMMC_RESP0);
}

/*****************************************************************************
*
* @description
* This function gets the long type card response.
*
* @param
*           pCmd is pointer to the card request structure.
*           baseAddr is the controller base address.
*
* @return
*           NA.
*
* @note     NA.
*
*****************************************************************************/
static void FSdmmcPs_sdGetLongReply(FSdmmcPs_SdCmd_T *pCmd, u32 baseAddr)
{
    pCmd->rsp_buf[0] = FMSH_ReadReg(baseAddr, SDMMC_RESP0);
    pCmd->rsp_buf[1] = FMSH_ReadReg(baseAddr, SDMMC_RESP1);
    pCmd->rsp_buf[2] = FMSH_ReadReg(baseAddr, SDMMC_RESP2);
    pCmd->rsp_buf[3] = FMSH_ReadReg(baseAddr, SDMMC_RESP3);
}

/*****************************************************************************
*
* @description
* This function is initialize the SDMMC device.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*           pInstance is the pointer to instance structure.
*           pSdCfg is the pointer to configuration information structure.
*
* @return   
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FSdmmcPs_initDev(FSdmmcPs_T *pSdmmc,
                          FSdmmcPs_Instance_T *pInstance,
                          FSdmmcPs_Config *pSdCfg)
{
    /* initial device*/
    pSdmmc->config.DeviceId = pSdCfg->DeviceId;
    pSdmmc->config.BaseAddress = pSdCfg->BaseAddress;
    pSdmmc->config.InputClockHz = pSdCfg->InputClockHz;
    pSdmmc->config.CardDetect = pSdCfg->CardDetect;
    pSdmmc->config.WriteProtect = pSdCfg->WriteProtect;
    
    pSdmmc->instance = pInstance;
    pSdmmc->comp_version = NULL;
    pSdmmc->comp_type = FMSH_ahb_sdmmc;
}

/*****************************************************************************
*
* @description
* This function returns the card detect value.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           card detect value.
*
* @note     NA.
*
*****************************************************************************/
int FSdmmcPs_cardDetect(FSdmmcPs_T *pSdmmc)
{
    int temp;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    temp = FMSH_ReadReg(pSdmmc->config.BaseAddress, SDMMC_CDETECT) & CARD_DETECT;

    return temp;
}

/*****************************************************************************
*
* @description
* Reset the SDMMC controller by SLCR registers.
*
* @param
*           sd_index select the SDMMC controller.
*
* @return   
*           NA.
*
* @note        
*           NA.
*
*****************************************************************************/
void FSdmmcPs_resetController(u8 sd_index)
{
    u32 data_reg;
    u32 i;

    /* unlock SLCR*/
    FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x008, 0xDF0D767B);
    
    /*set rst_ctrl*/
    data_reg = FMSH_ReadReg(FPS_SLCR_BASEADDR, 0x290);
    if (sd_index == 0)
    {
        data_reg |= FMSH_BIT0; /* reset sdio0*/
    }
    else
    {
        data_reg |= FMSH_BIT1; /* reset sdio1*/
    }
    FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x290, data_reg);

    for(i = 0; i< 50; i++);
    
    /*clear rst_ctrl*/
    data_reg = FMSH_ReadReg(FPS_SLCR_BASEADDR, 0x290);
    if (sd_index == 0)
    {
        data_reg &= ~FMSH_BIT0; /* unreset sdio0*/
    }
    else
    {
        data_reg &= ~FMSH_BIT1; /* unreset sdio1*/
    }
    FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x290, data_reg);

    /* relock SLCR*/
    FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x004, 0xDF0D767B);
}

/*****************************************************************************
*
* @description
* This function reset the sdmmc instance.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*           pDmac is the pointer to the DMA controller device.
*
* @return
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FSdmmcPs_instanceReset(FSdmmcPs_T *pSdmmc, FDmaPs_T *pDmac)
{
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pInstance->pDma = pDmac;
    
    memset(pInstance->raw_cid, 0, 4 * sizeof(u32));
    memset(pInstance->raw_csd, 0, 4 * sizeof(u32));
    memset(pInstance->raw_scr, 0, 8 * sizeof(u8));
    memset(&pInstance->cid, 0, sizeof(SDMMC_cid_T));
    memset(&pInstance->csd, 0, sizeof(SDMMC_csd_T));
    memset(&pInstance->scr, 0, sizeof(SDMMC_scr_T));
    memset(pInstance->ext_csd, 0, 512 * sizeof(u8));
    
    pInstance->transMode = sdmmc_trans_mode_normal;  /* jc*/
    
    pInstance->powerDown = FSdmmcPs_powerDown;
    pInstance->powerUp = FSdmmcPs_powerUp;
    pInstance->sendCmd = FSdmmcPs_sendCmd;
    pInstance->setClock = FSdmmcPs_setClock;
    pInstance->setBuswidth = FSdmmcPs_setCtrlBuswidth;
    pInstance->doTransfer = FSdmmcPs_doTransfer;
}

/*****************************************************************************
*
* @description
* This function sets the burst size of multipul transaction used with DMA.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*           dma_msize is burst size used with DMA.
*
* @return
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FSdmmcPs_setDmaMsize(FSdmmcPs_T *pSdmmc, u32 dma_msize)
{
    u32 temp;
    u32 baseAddr;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    baseAddr = pSdmmc->config.BaseAddress;
    
    temp = FMSH_ReadReg(baseAddr, SDMMC_FIFOTH)|
           (dma_msize << DW_DMA_MULTI_TRAN_SIZE_OFFSET);
    FMSH_WriteReg(baseAddr, SDMMC_FIFOTH, temp);
}

/*****************************************************************************
*
* @description
* This function sets the FIFO threshold watermark level
* when receiving data from card.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*           rx_wmark is the watermark level of receiver FIFO threshold.
*
* @return
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FSdmmcPs_setRXwmark(FSdmmcPs_T *pSdmmc, u16 rx_wmark)
{
    u32 temp;
    u32 baseAddr;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    baseAddr = pSdmmc->config.BaseAddress;

    temp = FMSH_ReadReg(baseAddr, SDMMC_FIFOTH) & ~(RX_WMARK_MASK << RX_WMARK_OFFSET);
    temp |= (rx_wmark << RX_WMARK_OFFSET);
    FMSH_WriteReg(baseAddr, SDMMC_FIFOTH, temp);
}

/*****************************************************************************
*
* @description
* This function sets the FIFO threshold watermark level
* when transmitting data to card.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*           tx_wmark is the watermark level of transmit FIFO threshold.
*
* @return
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FSdmmcPs_setTXwmark(FSdmmcPs_T *pSdmmc, u16 tx_wmark)
{
    u32 temp;
    u32 baseAddr;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    baseAddr = pSdmmc->config.BaseAddress;
    
    temp = FMSH_ReadReg(baseAddr, SDMMC_FIFOTH) & ~(TX_WMARK_MASK << TX_WMARK_OFFSET);
    temp |= (tx_wmark << TX_WMARK_OFFSET);
    FMSH_WriteReg(baseAddr, SDMMC_FIFOTH, temp);
}

/*****************************************************************************
*
* @description
* This function set the card slot's voltage.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FSdmmcPs_setCardVoltage(FSdmmcPs_T *pSdmmc, u8 vol)
{
    u32 temp;
    u32 baseAddr;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    baseAddr = pSdmmc->config.BaseAddress;
    
    temp = FMSH_ReadReg(baseAddr, SDMMC_CTRL) & ~(0x7 << 16);
    temp |= (vol << 16);
    FMSH_WriteReg(baseAddr, SDMMC_CTRL, temp);
}

/*****************************************************************************
*
* @description
* This function power down the selected card.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FSdmmcPs_powerDown(FSdmmcPs_T *pSdmmc)
{

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    FMSH_WriteReg(pSdmmc->config.BaseAddress, SDMMC_PWREN, 0);
    delay_us(1);
}

/*****************************************************************************
*
* @description
* This function power up the selected card.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FSdmmcPs_powerUp(FSdmmcPs_T *pSdmmc)
{

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    FMSH_WriteReg(pSdmmc->config.BaseAddress, SDMMC_PWREN, 0xFF);
    delay_us(1);
}

/*****************************************************************************
*
* @description
* This function sets the card's clock frequency to given value.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*           rate is clock frequency in Hz.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FSdmmcPs_setClock(FSdmmcPs_T *pSdmmc, int rate)
{
    int div_cnt, ret = 0;
    unsigned long  clk_div;
    u32 temp;
    u32 baseAddr, sd_clk;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    baseAddr = pSdmmc->config.BaseAddress;
	
    sd_clk = pSdmmc->config.InputClockHz;  /* 100M Hz*/

    if (rate >= sd_clk)
    {
        clk_div = 0;
    }
    else
    {
        if(rate != 0)
        {
            /*clk_div = (sd_clk / (2 *rate) + 1);*/
    		for (div_cnt = 0x1; div_cnt <= SDMMC_MAX_DIV_CNT; div_cnt++)
            {
    			if ((sd_clk / (2 * div_cnt) <= rate))
                {
    				clk_div = div_cnt;
    				break;
    			}
    		}
        }
    }
    
    /* disable clock */
    FMSH_WriteReg(baseAddr, SDMMC_CLKENA, 0);
    temp = START_CMD + UPDATE_CLOCK_REG_ONLY + WAIT_PRVDATA_COMPLETE;
    FMSH_WriteReg(baseAddr, SDMMC_CMD_VAL, temp);

    ret = sdmmc_waitStartCmdCleared(pSdmmc);

    /* set clock */
    FMSH_WriteReg(baseAddr, SDMMC_CLKSRC, 0);
    FMSH_WriteReg(baseAddr, SDMMC_CLKDIV, clk_div);
    temp = START_CMD + UPDATE_CLOCK_REG_ONLY + WAIT_PRVDATA_COMPLETE;
    FMSH_WriteReg(baseAddr, SDMMC_CMD_VAL, temp);

    ret = sdmmc_waitStartCmdCleared(pSdmmc);

    /* enable clock */
    FMSH_WriteReg(baseAddr, SDMMC_CLKENA, 0xFFFF);
    temp = START_CMD + UPDATE_CLOCK_REG_ONLY + WAIT_PRVDATA_COMPLETE;
    FMSH_WriteReg(baseAddr, SDMMC_CMD_VAL, temp);

    ret = sdmmc_waitStartCmdCleared(pSdmmc);

    delay_us(10);
    return ret;
}

/*****************************************************************************
*
* @description
* This function configures the bus width in card type register.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*           width is bus width.
*
* @return
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FSdmmcPs_setCtrlBuswidth(FSdmmcPs_T *pSdmmc, unsigned int width)
{

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    switch(width)
    {       
        case SD_BUS_WIDTH_4:
            FMSH_WriteReg(pSdmmc->config.BaseAddress, SDMMC_CTYPE, 0x1);
        break;      
        case SD_BUS_WIDTH_1:
            FMSH_WriteReg(pSdmmc->config.BaseAddress, SDMMC_CTYPE, 0);
        break;      
        default:
            FMSH_WriteReg(pSdmmc->config.BaseAddress, SDMMC_CTYPE, 0);
        break;
        }   
}

/*****************************************************************************
*
* @description
* This function resets controller.
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
int FSdmmcPs_sdControllerReset(FSdmmcPs_T *pSdmmc)
{
    u32 temp;
    u32 timeout_cnt = SDMMC_LOOP_TIMEOUT;
    u32 baseAddr;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    baseAddr = pSdmmc->config.BaseAddress;

    temp = FMSH_ReadReg(baseAddr, SDMMC_CTRL) | CONTROLLER_RESET;
    FMSH_WriteReg(baseAddr, SDMMC_CTRL, temp);

    temp = FMSH_ReadReg(baseAddr, SDMMC_CTRL) & CONTROLLER_RESET;
    while(temp)
    {
        timeout_cnt--;
        temp = FMSH_ReadReg(baseAddr, SDMMC_CTRL) & CONTROLLER_RESET;
        delay_1us();
        if (timeout_cnt == 0)
        {
            return -FMSH_ETIME;
        }
    }
        
    return 0;
}

/*****************************************************************************
*
* @description
* This function resets FIFO.
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
int FSdmmcPs_sdFifoReset(FSdmmcPs_T *pSdmmc)
{
    u32 temp;
    u32 timeout_cnt = SDMMC_LOOP_TIMEOUT;
    u32 baseAddr;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    baseAddr = pSdmmc->config.BaseAddress;
    
    temp = FMSH_ReadReg(baseAddr, SDMMC_CTRL) | FIFO_RESET;
    FMSH_WriteReg(baseAddr, SDMMC_CTRL, temp);

    temp = FMSH_ReadReg(baseAddr, SDMMC_CTRL) & FIFO_RESET;
    while(temp)
    {
        timeout_cnt--;
        temp = FMSH_ReadReg(baseAddr, SDMMC_CTRL) & FIFO_RESET;
        delay_1us();
        if (timeout_cnt == 0)
        {
            return -FMSH_ETIME;
        }
    }

    return 0;
}

/*****************************************************************************
*
* @description
* This function resets DMA interface.
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
int FSdmmcPs_sdDmaReset(FSdmmcPs_T *pSdmmc)
{
    u32 temp;
    u32 timeout_cnt = SDMMC_LOOP_TIMEOUT;
    u32 baseAddr;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    baseAddr = pSdmmc->config.BaseAddress;
    
    temp = FMSH_ReadReg(baseAddr, SDMMC_CTRL) | DMA_RESET;
    FMSH_WriteReg(baseAddr, SDMMC_CTRL, temp);

    temp = FMSH_ReadReg(baseAddr, SDMMC_CTRL) & DMA_RESET;
    while(temp)
    {
        timeout_cnt--;
        temp = FMSH_ReadReg(baseAddr, SDMMC_CTRL) & DMA_RESET;
        delay_1us();
        if (timeout_cnt == 0)
        {
            return -FMSH_ETIME;
        }
    }
        
        return 0;
}

/*****************************************************************************
*
* @description
* This function returns the interrupt mask value.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           interrupt mask value.
*
* @note     NA.
*
*****************************************************************************/
u32 FSdmmcPs_getIntMask(FSdmmcPs_T *pSdmmc)
{
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    return (FMSH_ReadReg(pSdmmc->config.BaseAddress, SDMMC_INTMASK));
}

/*****************************************************************************
*
* @description
* This function sets the interrupt mask value.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FSdmmcPs_setIntMask(FSdmmcPs_T *pSdmmc, u32 int_mask)
{
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    FMSH_WriteReg(pSdmmc->config.BaseAddress, SDMMC_INTMASK, int_mask);
}

/*****************************************************************************
*
* @description
* This function returns the raw interrupt status value.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           raw interrupt status value.
*
* @note     NA.
*
*****************************************************************************/
u32 FSdmmcPs_getRawIntStat(FSdmmcPs_T *pSdmmc)
{
	 /*delay_ms(1);*/
	 /*delay_us(100);*/
	/* taskDelay(1);//printf("Enter %s \n",__FUNCTION__);	*/
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    return (FMSH_ReadReg(pSdmmc->config.BaseAddress, SDMMC_RINTSTS));
}

/*****************************************************************************
*
* @description
* This function sets the raw interrupt status value.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FSdmmcPs_setRawIntStat(FSdmmcPs_T *pSdmmc, u32 raw_int)
{
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    FMSH_WriteReg(pSdmmc->config.BaseAddress, SDMMC_RINTSTS, raw_int);
}

/*****************************************************************************
*
* @description
* This function returns the masked interrupt status value.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           masked interrupt status value.
*
* @note     NA.
*
*****************************************************************************/
u32 FSdmmcPs_getMaskIntStat(FSdmmcPs_T *pSdmmc)
{
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    return (FMSH_ReadReg(pSdmmc->config.BaseAddress, SDMMC_MINTSTS));
}

/*****************************************************************************
*
* @description
* This function enables the global interrupt bit.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FSdmmcPs_enableInt(FSdmmcPs_T *pSdmmc)
{
    u32 temp;
    u32 baseAddr;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    baseAddr = pSdmmc->config.BaseAddress;

    temp = FMSH_ReadReg(baseAddr, SDMMC_CTRL) | INT_ENABLE;
    FMSH_WriteReg(baseAddr, SDMMC_CTRL, temp);
}

/*****************************************************************************
*
* @description
* This function enables DMA transfer mode.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FSdmmcPs_enableDMA(FSdmmcPs_T *pSdmmc)
{
    u32 temp;
    u32 baseAddr;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    baseAddr = pSdmmc->config.BaseAddress;

    temp = FMSH_ReadReg(baseAddr, SDMMC_CTRL) | DMA_ENABLE;
    FMSH_WriteReg(baseAddr, SDMMC_CTRL, temp);
}

/*****************************************************************************
*
* @description
* This function masks all interrupt except card detect(CD) interrupt.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FSdmmcPs_disableIntMask(FSdmmcPs_T *pSdmmc)
{
    u32 temp = 0;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);

    temp |= INT_MASK_CD;
    FMSH_WriteReg(pSdmmc->config.BaseAddress, SDMMC_INTMASK, temp);
}

/*****************************************************************************
*
* @description
* This function returns the data transfer mode.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           data transfer mode.
*
* @note     NA.
*
*****************************************************************************/
u8 FSdmmcPs_getTransferMode(FSdmmcPs_T *pSdmmc)
{
    u8 trans_mode;
    
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    trans_mode = (FMSH_ReadReg(pSdmmc->config.BaseAddress, SDMMC_CMD_VAL) & TRANSFER_MODE) >> TRANSFER_MODE_OFFSET;
    return trans_mode;
}

/*****************************************************************************
*
* @description
* This function frames the command register for a particular command.
*
* @param
*           cardType is the type of card.
*           cmd is the command to be sent.
*
* @return
*           Command register value.
*
* @note     NA.
*
*****************************************************************************/
u32 FSdmmcPs_prepareCmd(u32 cardType, char cmd)
{
    u32 cmd_data = 0;
    switch(cmd)
    {
        /*No response commands*/
        case CMD_GO_IDLE_STATE:
            cmd_data = (START_CMD 
                      + SEND_INITIALIZATION);
        break;
        case CMD_SET_DSR:
        case CMD_GO_INACTIVE_STATE:
            cmd_data = cmd;
        break;
         /*Long response commands*/
        case CMD_ALL_SEND_CID:
        case CMD_SEND_CSD:
        case CMD_SEND_CID:
            cmd_data = (RESPONSE_EXPECT
                      + RESPONSE_LENGTH_LONG 
                      + cmd);
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
        case CMD_SWITCH_AND_ACMD_SET_BUS_WIDTH:
        case CMD_SELECT_CARD:
        case CMD_SEND_STATUS:
        case CMD_APP_CMD:
        /*case ACMD_SEND_SCR:*/
        case CMD_SEND_IF_COND:
            cmd_data = (RESPONSE_EXPECT
                      + cmd);
        break;
        /*Stop/abort command*/
        case CMD_STOP_TRANSMISSION:
            cmd_data = (RESPONSE_EXPECT
                      + STOP_ABORT_CMD
                      + cmd);
        break;
		
        /*Have data command write*/
        case CMD_WRITE_BLOCK:
            cmd_data = (WAIT_PRVDATA_COMPLETE
                      + NREADWRITE
                      + DATA_EXPECTED
                      + CHECK_RESPONSE_CRC
                      + RESPONSE_EXPECT
                      + cmd) & ~SEND_AUTO_STOP;
        	break;
		
        case CMD_WRITE_MULTIPLE_BLOCK:
            if(cardType == SD)
                cmd_data = (SEND_AUTO_STOP
                          + WAIT_PRVDATA_COMPLETE
                          + NREADWRITE
                          + DATA_EXPECTED
                          + CHECK_RESPONSE_CRC
                          + RESPONSE_EXPECT
                          + cmd);
            else
                cmd_data = (WAIT_PRVDATA_COMPLETE
                          + NREADWRITE
                          + DATA_EXPECTED
                          + CHECK_RESPONSE_CRC
                          + RESPONSE_EXPECT
                          + cmd) & ~SEND_AUTO_STOP;
			
        	break;
		
        /*Have data command read*/
        case CMD_READ_MULTIPLE_BLOCK:
            if(cardType == SD)
                cmd_data = (SEND_AUTO_STOP
                          + WAIT_PRVDATA_COMPLETE
                          + DATA_EXPECTED
                          + CHECK_RESPONSE_CRC
                          + RESPONSE_EXPECT
                          + cmd);
            else
                cmd_data = (WAIT_PRVDATA_COMPLETE
                          + DATA_EXPECTED
                          + CHECK_RESPONSE_CRC
                          + RESPONSE_EXPECT
                          + cmd) & ~SEND_AUTO_STOP;

        break;
		
        case ACMD_SEND_SCR:			
        case CMD_READ_SINGLE_BLOCK:
            cmd_data = (WAIT_PRVDATA_COMPLETE
                      + DATA_EXPECTED
                      + CHECK_RESPONSE_CRC
                      + RESPONSE_EXPECT
                      + cmd);
        break;
		
        case CMD_SWITCH_FUNC:
        case MMC_SEND_EXT_CSD:
            cmd_data = (WAIT_PRVDATA_COMPLETE
                      + DATA_EXPECTED
                      + CHECK_RESPONSE_CRC
                      + RESPONSE_EXPECT
                      + (cmd >> 4));
        break;
		
        default:
            TRACE_OUT(DEBUG_OUT, "unknown command %d.\r\n", cmd);
            return -1;
        break;
    }
	
    cmd_data |= START_CMD;
	
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
int FSdmmcPs_sendCmd(FSdmmcPs_T *pSdmmc)
{
    u32 cmd_data = 0;
    u32 timeout_cnt;
    int int_raw, card_in, ret = 0;
    u32 baseAddr;
    FSdmmcPs_Instance_T *pInstance;
    FSdmmcPs_SdCmd_T *pCmd;
	u32 tmp32 = 0;
    
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);

    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;
    baseAddr = pSdmmc->config.BaseAddress;
/*    printf("%s: cmd = %02d\n",__FUNCTION__, pCmd->cmd);*/
    if (pSdmmc->config.CardDetect == 1)
    {
        card_in = FSdmmcPs_cardDetect(pSdmmc);
        if (card_in != CARD_INSERT)
        {
            TRACE_OUT(DEBUG_OUT, "No card insert.\r\n");
            return -FMSH_ENODEV;
        }
    }

    if(pCmd->res_flag & (CMD_RESP_FLAG_RDATA | CMD_RESP_FLAG_WDATA))
    {
        FMSH_WriteReg(baseAddr, SDMMC_BYTCNT, pCmd->data_len);
		
        /*FMSH_WriteReg(baseAddr, SDMMC_BLKSIZ, 512);*/
        FMSH_WriteReg(baseAddr, SDMMC_BLKSIZ, pCmd->block_size);
    }
	
	delay_ms(1);
	tmp32 = FSdmmcPs_getRawIntStat(pSdmmc);
    FSdmmcPs_setRawIntStat(pSdmmc, tmp32);

    cmd_data = FSdmmcPs_prepareCmd(pInstance->card_type, pCmd->cmd);
	
    /* Send the command (CRC calculated by host). */
    FMSH_WriteReg(baseAddr, SDMMC_CMDARG, pCmd->arg);
    FMSH_WriteReg(baseAddr, SDMMC_CMD_VAL, cmd_data);

   /* timeout_cnt = SDMMC_LOOP_TIMEOUT *2;*/
    timeout_cnt = SDMMC_LOOP_TIMEOUT * 4;

#if 0  /* old	*/
    /*
    while (!(FSdmmcPs_getRawIntStat(pSdmmc) & RAW_INT_STATUS_CMD_DONE))
    {
        timeout_cnt--;
        delay_1us();
		
       // if (timeout_cnt == 0)
        if (timeout_cnt <= 0)
        {
            TRACE_OUT(DEBUG_OUT, "FSdmmcPs_sendCmd:CMD(%d) done timeout!(%d) \r\n", pCmd->cmd, timeout_cnt);
            return -FMSH_ETIME;
        }
    }
	*/
#else
	do 
	{	
		delay_1ms();		
		tmp32 = FSdmmcPs_getRawIntStat(pSdmmc);
		
		timeout_cnt--;
		if (timeout_cnt <= 0)
        {
            TRACE_OUT(DEBUG_OUT, "FSdmmcPs_sendCmd:CMD(%d) done timeout!(%d) \r\n", pCmd->cmd, timeout_cnt);
            return -FMSH_ETIME;
        }
	} while (!(tmp32 & RAW_INT_STATUS_CMD_DONE));
#endif
	
    /* clear CMDD int */
    FSdmmcPs_setRawIntStat(pSdmmc, RAW_INT_STATUS_CMD_DONE);
    
    if (pCmd->res_type != CMD_RESP_TYPE_NONE) 
    {
        /*int_raw = FSdmmcPs_getRawIntStat(pSdmmc);*/
#if 1  /* mfq*/
		timeout_cnt = 100; /*SDMMC_LOOP_TIMEOUT *2;*/
    	do
		{
			delay_1ms();    		
    		int_raw = FSdmmcPs_getRawIntStat(pSdmmc);
			
			timeout_cnt--;			
    		if (timeout_cnt <= 0)
    		{	
    			break;	
    		}
    	} while (int_raw & (RAW_INT_STATUS_RTO | RAW_INT_STATUS_DRTO));
#endif


        /* Timeout? */
        if (int_raw & (RAW_INT_STATUS_RTO | RAW_INT_STATUS_DRTO))
        {
            TRACE_OUT(DEBUG_OUT, "mmc send CMD[%d] timeout.\r\n", pCmd->cmd);
            return -FMSH_ETIME;
        }
        /* CRC? */
        else if (!(pCmd->res_flag & CMD_RESP_FLAG_NOCRC)
                && (int_raw & (RAW_INT_STATUS_RCRC | RAW_INT_STATUS_DCRC)))
        {
            TRACE_OUT(DEBUG_OUT, "CMD[%d] CRC error\r\n", pCmd->cmd);
            return -FMSH_EINVAL;
        }

        if (pCmd->res_flag & CMD_RESP_FLAG_DATALINE) 
        {
            ret = FSdmmcPs_readData(baseAddr, pInstance->pDma, (char *)pCmd->rsp_buf, pCmd->data_len, pInstance->transMode);
        } 
        else 
        {
            if (sdmmc_getResLen(pCmd->res_type) == CMD_RES_LEN_LONG) 
                FSdmmcPs_sdGetLongReply(pCmd, baseAddr);
            else 
                FSdmmcPs_sdGetShortReply(pCmd, baseAddr);
            
            if (pCmd->res_flag & CMD_RESP_FLAG_RDATA) 
            {
                ret = FSdmmcPs_readData(baseAddr, pInstance->pDma, pCmd->data, pCmd->data_len, pInstance->transMode);
/*              FSdmmcPs_sdstopTransfer(pSdmmc);*/
            }

            if (pCmd->res_flag & CMD_RESP_FLAG_WDATA) 
            {
                ret = FSdmmcPs_writeData(baseAddr, pInstance->pDma, pCmd->data, pCmd->data_len, pInstance->transMode);
/*              FSdmmcPs_sdstopTransfer(pSdmmc);*/
            }
        }
    } 

    return ret; 
}

/*****************************************************************************
*
* @description
* This function returns the total card size in Kbytes.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           card size.
*
* @note     NA.
*
*****************************************************************************/
unsigned int FSdmmcPs_getCardSizeKB(FSdmmcPs_T *pSdmmc)
{
    FSdmmcPs_Instance_T *pInstance;
    
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    
/*    if (pInstance->capacity == 0xFFFFFFFF)*/
/*    {*/
/*        return pInstance->csd.blockNR / 2;*/
/*    }*/
    
    return pInstance->capacity / 1024;
}

/*****************************************************************************
*
* @description
* This function returns the block number.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           block number.
*
* @note     NA.
*
*****************************************************************************/
unsigned int FSdmmcPs_getBlockNumbers(FSdmmcPs_T *pSdmmc)
{
    FSdmmcPs_Instance_T *pInstance;
    
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    
    return pInstance->csd.blockNR;
}

/*****************************************************************************
*
* @description
* This function returns card status.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           card status.
*
* @note     NA.
*
*****************************************************************************/
unsigned int FSdmmcPs_getCardStatus(FSdmmcPs_T *pSdmmc)
{
    FSdmmcPs_Instance_T *pInstance;
    
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    
    return pInstance->stat;
}

/*****************************************************************************
*
* @description
* This function sets the block length to the card.
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
int FSdmmcPs_setBlockLen(FSdmmcPs_T *pSdmmc, unsigned int len)
{
    int ret;
    int retry = 100;
    
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);

    while (retry)
    {
        ret = sdmmc_cmd16(pSdmmc, len);
        if(ret < 0)
        {
            delay_us(10);
            retry--;
        }
        else
        {
            break;
        }
    }
    
    if(ret)
        TRACE_OUT(DEBUG_OUT, "cmd 16 error, set block length failled.\r\n");
    return ret;
}

/*****************************************************************************
*
* @description
* This function sets the bus width to the card.
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
int FSdmmcPs_setCardBusWidth(FSdmmcPs_T *pSdmmc, unsigned int bus_width)
{
    int ret;
    int retry = 5;
    FSdmmcPs_Instance_T *pInstance;
    
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    
    if(pInstance->card_type == SD)
    {
        while (retry)
        {
            ret = sdmmc_sdACMD6(pSdmmc, bus_width);
            if(ret < 0)
            {
                if (ret == -FMSH_ENODEV)
                {
                    TRACE_OUT(DEBUG_OUT, "ACMD6 failed!\r\n");
                    return ret;
                }
                
                retry--;
                TRACE_OUT(DEBUG_OUT, "retry to sd set buswidth\r\n");
            }
            else
            {
                break;
            }
        }
        if (ret < 0)
        {
            TRACE_OUT(DEBUG_OUT, "ACMD6 failed!\r\n");
            return ret;
        }
    }
    else
    {
        if(pInstance->version >= MMC_VERSION_4)
        {
            while (retry)
            {
                ret = sdmmc_cmd6_mmc(pSdmmc, MMC_4_BIT_BUS_ARG);
                if(ret < 0)
                {
                    if (ret == -FMSH_ENODEV)
                    {
                        return ret;
                    }
                    
                    retry--;
                    TRACE_OUT(DEBUG_OUT, "retry to mmc set buswidth\r\n");
                }
                else
                {
                    break;
                }
            }
            if (ret < 0)
            {
                return ret;
            }
        }
    }

	/**/
	/* set bus width*/
	/**/
    pInstance->setBuswidth(pSdmmc, bus_width);

    return ret;
}

/*****************************************************************************
*
* @description
* This function gets bus speed supported by card.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*           pBuf is buffer to store function group support data 
*                   returned by card.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FSdmmcPs_getBusSpeed(FSdmmcPs_T *pSdmmc, u8 *pBuf)
{
    int ret;
    u32 arg;

    arg = SD_SWITCH_CMD_HS_GET;
    ret = sdmmc_cmd6_sd(pSdmmc, arg, pBuf);

    return ret;
}

/*****************************************************************************
*
* @description
* This function to set high speed in card and host.
* Changes clock in host accordingly.
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
int FSdmmcPs_changeBusSpeed(FSdmmcPs_T *pSdmmc)
{
    int ret;
    u32 arg;
    u8 readBuff[64];
    FSdmmcPs_Instance_T *pInstance;
    
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    
    if(pInstance->card_type == SD)
    {
        arg = SD_SWITCH_CMD_HS_SET;
        ret = sdmmc_cmd6_sd(pSdmmc, arg, readBuff);
        if (ret < 0)
        {
            return ret;
        }
        
        ret = pInstance->setClock(pSdmmc, SDMMC_CLK_50_MHZ);
        if (ret < 0)
        {
            return ret;
        }
    }
    else if ((pInstance->card_type == MMC) || (pInstance->card_type == EMMC))
    {
        arg = MMC_HIGH_SPEED_ARG;
        ret = sdmmc_cmd6_mmc(pSdmmc, arg);
        if (ret < 0)
        {
            return ret;
        }
        
         ret = pInstance->setClock(pSdmmc, SDMMC_CLK_52_MHZ);
       /* ret = pInstance->setClock(pSdmmc, SDMMC_CLK_25_MHZ);*/
        if (ret < 0)
        {
            return ret;
        }
    }

    return ret;
}

/*****************************************************************************
*
* @description
* This function gets the card's SCR info.
*
* @param    
*           pSdmmc is the pointer to a sd card driver instance.
*
* @return	
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note		NA.
*
*****************************************************************************/
int FSdmmcPs_getSdSCR(FSdmmcPs_T *pSdmmc)
{
    int ret;
    u32 scr_struct;
    SDMMC_scr_T *scr;
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);

    pInstance = pSdmmc->instance;
    scr = &pInstance->scr;

    memset(pInstance->raw_scr, 0, 8 * sizeof(u8));
    ret = sdmmc_sdACMD51(pSdmmc, pInstance->raw_scr);
    if (ret < 0)
    {
        return ret;
    }
    TRACE_OUT(DEBUG_OUT, "scr is 0x%02x_%02x_%02x_%02x_%02x_%02x_%02x_%02x\r\n", \
              pInstance->raw_scr[0], pInstance->raw_scr[1], pInstance->raw_scr[2], \
              pInstance->raw_scr[3], pInstance->raw_scr[4], pInstance->raw_scr[5], \
              pInstance->raw_scr[6], pInstance->raw_scr[7]);

    scr_struct = (pInstance->raw_scr[0] >> 4) & 0xF;
    if (scr_struct != 0)
    {
        return -FMSH_EINVAL;
    }
    scr->sd_spec = pInstance->raw_scr[0] & 0x0F;
    scr->bus_width = pInstance->raw_scr[1] & 0x0F;
    scr->sd_spec3 = (pInstance->raw_scr[2] >> 7) & 0x01;
    
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
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FSdmmcPs_getMmcExtCsd(FSdmmcPs_T *pSdmmc)
{
    int ret;
    FSdmmcPs_Instance_T *pInstance;

    SDMMC_COMMON_REQUIREMENTS(pSdmmc);

    pInstance = pSdmmc->instance;
    if (pInstance->version >= MMC_VERSION_4)
    {
        /* get EXT CSD info */
        unsigned int retry = 5;
        
        while(retry)
        {
            memset(pInstance->ext_csd, 0, 512 * sizeof(u8));
            ret = sdmmc_cmd8_mmc(pSdmmc, pInstance->ext_csd);
            if (ret < 0)
            {
                retry--;
                if(retry == 0)
                    return ret;
            }
			else
            {
				break;
			}
        }
    }

    return 0;
}

/*****************************************************************************
*
* @description
* This function runs the SD card enumeration sequence.
* This function runs after the initialization and identification procedure.
* It gets all necessary information from the card.
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
int FSdmmcPs_sdEnumerate(FSdmmcPs_T *pSdmmc)
{
    int ret;
    int retries = 10000;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;
    
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;
    
    ret = sdmmc_appCmd41(pSdmmc);
    if(ret < 0)
    {
        TRACE_OUT(DEBUG_OUT, "ACMD41 failed: %d\r\n", ret);
        return ret;
    }
    
    pInstance->ocr = pCmd->rsp_buf[0];
    /*while (!(pInstance->ocr & MMC_CARD_BUSY))*/
	while (!(pInstance->ocr & OCR_BUSY))
    {
        ret = sdmmc_sdACMD41(pSdmmc);
        if(ret < 0)
        {
            TRACE_OUT(DEBUG_OUT, "ACMD41 failed: %d\r\n", ret);
            return ret;
        }
        pInstance->ocr = pCmd->rsp_buf[0];
        retries--;
        if (retries == 0)
        {
            ret = -FMSH_ETIME;
            return ret;
        }
    }
    TRACE_OUT(DEBUG_OUT, "ACMD41 response success, Card is unbusy, retry[%d]!\r\n", retries);
    
   /* if(pInstance->ocr & 0x40000000)*/
	if(pInstance->ocr & OCR_HCS)
    {
        TRACE_OUT(DEBUG_OUT, "high capacity sd card\r\n");
        pInstance->HighCapacity = 1;
    }
    else
    {
        TRACE_OUT(DEBUG_OUT, "normal capacity sd card\r\n");
        pInstance->HighCapacity = 0;
    }
	
    /* 
    ALL_SEND_CID 
	*/
    ret = sdmmc_cmd2(pSdmmc);
    if (ret < 0)
    {
        return ret;
    }
    TRACE_OUT(DEBUG_OUT, "cid[0] is 0x%08x\r\n", pCmd->rsp_buf[0]);
    TRACE_OUT(DEBUG_OUT, "cid[1] is 0x%08x\r\n", pCmd->rsp_buf[1]);
    TRACE_OUT(DEBUG_OUT, "cid[2] is 0x%08x\r\n", pCmd->rsp_buf[2]);
    TRACE_OUT(DEBUG_OUT, "cid[3] is 0x%08x\r\n", pCmd->rsp_buf[3]);
	
    memcpy(pInstance->raw_cid, pCmd->rsp_buf, 4 * sizeof(u32));
    
    /*
    SET_RELATIVE_ADDR, Stand-by State 
    */
    pInstance->rca = 1 ; /* slot id begins at 0 */
    ret = sdmmc_cmd3(pSdmmc);
    if (ret < 0)
    {
        return ret;
    }
    pInstance->rca = pCmd->rsp_buf[0] >> 16;
    if (!(pCmd->rsp_buf[0] & STATE_IDENT))
    {
        return -FMSH_ENODEV;
    }
	
    /* 
    SEND_CSD, Stand-by State 
	*/
    ret = sdmmc_cmd9(pSdmmc);
    if (ret < 0)
    {
        return ret;
    }
    TRACE_OUT(DEBUG_OUT, "csd[0] is 0x%08x\r\n", pCmd->rsp_buf[0]);
    TRACE_OUT(DEBUG_OUT, "csd[1] is 0x%08x\r\n", pCmd->rsp_buf[1]);
    TRACE_OUT(DEBUG_OUT, "csd[2] is 0x%08x\r\n", pCmd->rsp_buf[2]);
    TRACE_OUT(DEBUG_OUT, "csd[3] is 0x%08x\r\n", pCmd->rsp_buf[3]);
    memcpy(pInstance->raw_csd, pCmd->rsp_buf, 4 * sizeof(u32));

	
    ret = SDMMC_decodeCSD(pSdmmc);
    if (ret < 0)
    {
        TRACE_OUT(DEBUG_OUT, "unrecognised CSD structure version!\r\n");
        return ret;
    }
    
    ret = SDMMC_decodeCID(pSdmmc);
    if (ret < 0)
    {
        TRACE_OUT(DEBUG_OUT, "card has unknown MMCA version!\r\n");
    }
    
    ret = SDMMC_getCSDInfo(pSdmmc);
    if (ret < 0)
    {
        TRACE_OUT(DEBUG_OUT, "read_blk_len and write_blk_len are not equal\r\n");
        return ret;
    }
    TRACE_OUT(DEBUG_OUT, "-I- SD Card OK, size: %d MB\r\n", (int)FSdmmcPs_getCardSizeKB(pSdmmc)/1024);
    return 0;    
}

/*****************************************************************************
*
* @description
* This function runs the MMC card and emmc enumeration sequence.
* This function runs after the initialization and identification procedure.
* It gets all necessary information from the card.
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
int FSdmmcPs_mmcEnumerate(FSdmmcPs_T *pSdmmc)
{
    int ret;
    int retries = 10000;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;
    
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;
    
    pInstance->HighCapacity = 0;

    /* 
    SEND_OP_COND 
	*/
    /*ret = sdmmc_cmd1(pSdmmc);   // ????*/
	ret = sdmmc_cmd0(pSdmmc);
	
    if(ret == -FMSH_ENODEV)
    {
        return ret;
    }
    delay_ms(1);

    while (retries)
    {
        ret = sdmmc_cmd1(pSdmmc);
        if(ret == -FMSH_ENODEV)
        {
            return ret;
        }
		
        pInstance->ocr = pCmd->rsp_buf[0];
		if (pInstance->ocr & OCR_BUSY)
		{	
			break;
		}
			
        retries--;
        if (retries == 0)
        {
            ret = -FMSH_ETIME;
            return ret;
        }
        delay_ms(1);
    }
    TRACE_OUT(DEBUG_OUT, "CMD1 response success, Card is unbusy, retry[%d]!\r\n", retries);
	pInstance->HighCapacity = ((pInstance->ocr & OCR_HCS) == OCR_HCS);

    /* 
    ALL_SEND_CID 
	*/
    ret = sdmmc_cmd2(pSdmmc);
    if (ret < 0) 
    {
        return ret;
    }
    memcpy(pInstance->raw_cid, pCmd->rsp_buf, 4 * sizeof(u32));

    /* 
    SET_RELATIVE_ADDR, Stand-by State 
	*/
    /*pInstance->rca = 2; /* slot id begins at 0 */
    ret = sdmmc_cmd3(pSdmmc);
    if (ret < 0)
    {
        return ret;
    }
    if (!(pCmd->rsp_buf[0] & STATE_IDENT))
    {
        TRACE_OUT(DEBUG_OUT, "CMD3 response failed\r\n");
        return -FMSH_ENODEV;
    }

    /* SEND_CSD, Stand-by State */
    ret = sdmmc_cmd9(pSdmmc);
    if (ret < 0)
    {
        return ret;
    }
    TRACE_OUT(DEBUG_OUT, "csd[0] is 0x%08x\r\n", pCmd->rsp_buf[0]);
    TRACE_OUT(DEBUG_OUT, "csd[1] is 0x%08x\r\n", pCmd->rsp_buf[1]);
    TRACE_OUT(DEBUG_OUT, "csd[2] is 0x%08x\r\n", pCmd->rsp_buf[2]);
    TRACE_OUT(DEBUG_OUT, "csd[3] is 0x%08x\r\n", pCmd->rsp_buf[3]);
    memcpy(pInstance->raw_csd, pCmd->rsp_buf, 4 * sizeof(u32));
    
    ret = SDMMC_decodeCSD(pSdmmc);
    if (ret < 0)
    {
        TRACE_OUT(DEBUG_OUT, "unrecognised CSD structure version!\r\n");
        return ret;
    }
    
    ret = SDMMC_decodeCID(pSdmmc);
    if (ret < 0)
    {
        TRACE_OUT(DEBUG_OUT, "card has unknown MMCA version!\r\n");
    }
    
    ret = SDMMC_getCSDInfo(pSdmmc);
    if (ret < 0)
    {
        TRACE_OUT(DEBUG_OUT,  "read_blk_len and write_blk_len are not equal\r\n");
    }
    TRACE_OUT(DEBUG_OUT, "-I- MMC Card OK, size: %d MB\r\n", (int)FSdmmcPs_getCardSizeKB(pSdmmc)/1024);

    return 0;
}

/*****************************************************************************
*
* @description
* This function runs the initialisation procedure and the
* identification process, then it sets the card in transfer state to
* set the block length and the bus width, at last change the card clock
* to high clock.
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
int FSdmmcPs_cardIdentify(FSdmmcPs_T *pSdmmc)
{
    int ret, card_in;
	u64 capacity;
    u8 cmd6ReadBuff[64] = { 0U };
	static const u32 mmc_versions[] = {
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
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;
    
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;
    
    pInstance->rca = 0;
	
	/**/
	/* powerup*/
	/**/
    pInstance->powerUp(pSdmmc);

	/**/
	/* setclock*/
	/**/
    TRACE_OUT(DEBUG_OUT, "Switch the clock to %dHz for card identify!\r\n", SDMMC_CLK_400_KHZ);
    ret = pInstance->setClock(pSdmmc, SDMMC_CLK_400_KHZ);
    if (ret < 0)
    {
        return ret;
    }

    /* go IDLE state */
    ret = sdmmc_cmd0(pSdmmc);
    if (ret < 0)
    {
        return ret;
    }
    pInstance->version = SD_VERSION_1_0;
	
    /*check whether 2.0*/
    ret = sdmmc_cmd8_sd(pSdmmc);
    if(ret < 0)
    {
        TRACE_OUT(DEBUG_OUT, "CMD8 no response: %d\r\n", ret);
    }
    else
    {
        TRACE_OUT(DEBUG_OUT, "CMD8 get response!\r\n");
        if((pCmd->rsp_buf[0] & 0xFF) == (SD_CMD8_VOL_PATTERN & 0xFF))
        {
            TRACE_OUT(DEBUG_OUT, "card version 2.0 %x\r\n", pCmd->rsp_buf[0]);
            pInstance->version = SD_VERSION_2;
        }
        else 
        {
            TRACE_OUT(DEBUG_OUT, "card version 1.0\r\n");
        }
    }
	
    /* check whether it's a MMC or a SD card. */
    /* APP_CMD */
    ret = sdmmc_cmd55(pSdmmc);
    if (ret < 0)
    {
        if (ret == -FMSH_ENODEV)
        {
            return ret;
        }
        
        TRACE_OUT(DEBUG_OUT, "MultiMedia Card Identification.\r\n");
		
        card_in = FSdmmcPs_cardDetect(pSdmmc);
		
        if ((card_in == CARD_INSERT) && pSdmmc->config.CardDetect)
            pInstance->card_type = MMC;
        else
            pInstance->card_type = EMMC;
		
        ret = FSdmmcPs_mmcEnumerate(pSdmmc);
    }
    else
    {
        TRACE_OUT(DEBUG_OUT, "SD Memory Card Identification.\r\n");
        pInstance->card_type = SD;
		
        ret = FSdmmcPs_sdEnumerate(pSdmmc);
    }
	
    if (ret)
        return ret;
    
    /* SELECT CARD & set card state from Stand-by to Transfer */
    ret = FSdmmcPs_selectCard(pSdmmc);
    if (ret < 0)
    {
        return ret;
    }
    if (pCmd->rsp_buf[0] & (R1_CC_ERR | R1_ERR))
    {
        TRACE_OUT(DEBUG_OUT, "CMD7 response failed\r\n");
        return -FMSH_EIO;
    }
	
    /* Set high clock rate for the normal data transfer */
    TRACE_OUT(DEBUG_OUT, "Switch the clock to %dHz for data transfer\r\n", pInstance->csd.max_dtr);
    ret = pInstance->setClock(pSdmmc, pInstance->csd.max_dtr);
    if (ret < 0)
    {
        return ret;
    }

    /* select card bus width */
    ret = FSdmmcPs_setCardBusWidth(pSdmmc, SD_BUS_WIDTH_4);
    if (ret < 0)
    {
        return ret;
    }

    if ((pInstance->card_type == MMC) || (pInstance->card_type == EMMC))
    {
        delay_ms(10);
        ret = FSdmmcPs_getMmcExtCsd(pSdmmc);
        if (ret < 0)
        {
            TRACE_OUT(DEBUG_OUT, "Get MMC Ext_CSD info failed.\r\n");
            return ret;
        }
		pInstance->version = mmc_versions[pInstance->ext_csd[MMC_EXT_CSD_REV]];

		if (pInstance->version >= MMC_VERSION_4_2)
		{
			/*
			 * According to the JEDEC Standard, the value of
			 * ext_csd's capacity is valid if the value is more
			 * than 2GB
			 */
			capacity = pInstance->ext_csd[MMC_EXT_CSD_SEC_CNT] << 0
					 | pInstance->ext_csd[MMC_EXT_CSD_SEC_CNT + 1] << 8
					 | pInstance->ext_csd[MMC_EXT_CSD_SEC_CNT + 2] << 16
					 | pInstance->ext_csd[MMC_EXT_CSD_SEC_CNT + 3] << 24;
					 
			capacity *= SDMMC_BLOCK_SIZE;
			
			if (pInstance->HighCapacity == 1)
				pInstance->capacity = capacity;
		}
    }
	
	TRACE_OUT(DEBUG_OUT, "-I- Device's capacity size: %d MB\r\n", (int)FSdmmcPs_getCardSizeKB(pSdmmc)/1024);
    
    if (pInstance->card_type == SD)
    {
        /* get sd card SCR info*/
        ret = FSdmmcPs_getSdSCR(pSdmmc);
        if (ret < 0)
        {
            TRACE_OUT(DEBUG_OUT, "Get SD's SCR info error.\r\n");
            return ret;
        }

        if (pInstance->scr.sd_spec != 0)
        {
            ret = FSdmmcPs_getBusSpeed(pSdmmc, cmd6ReadBuff);
            if (ret < 0)
            {
                return ret;
            }

            if ((cmd6ReadBuff[13] & HIGH_SPEED_SUPPORT) != 0)
            {
                ret = FSdmmcPs_changeBusSpeed(pSdmmc);
                if (ret < 0)
                {
                    TRACE_OUT(DEBUG_OUT, "Failed to change SD to high speed mode!\r\n");
                    return ret;
                }
                else
                    TRACE_OUT(DEBUG_OUT, "Change SD to high speed mode succeed!\r\n");
            }
        }
    }
    else if ((pInstance->card_type == MMC) || (pInstance->card_type == EMMC))
    {
        if ((pInstance->ext_csd[MMC_EXT_CSD_CARD_TYPE] & EXT_CSD_DEVICE_TYPE_HIGH_SPEED) != 0)
        {
            ret = FSdmmcPs_changeBusSpeed(pSdmmc);
            if (ret < 0)
            {
                TRACE_OUT(DEBUG_OUT, "Failed to change MMC to high speed mode!\r\n");
                return ret;
            }
            else
                TRACE_OUT(DEBUG_OUT, "Change MMC to high speed mode succeed!\r\n");
        }
        else
        {
            TRACE_OUT(DEBUG_OUT, "MMC doesn't support high speed mode!\r\n");
        }
    }
    
    if(pInstance->HighCapacity == 0)
        FSdmmcPs_setBlockLen(pSdmmc, SDMMC_BLOCK_SIZE);

    return 0;
}

/*****************************************************************************
*
* @description
* This function moves card to transfer state. The buffer size must be at
* least 512 byte long.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*           address is address of the block to transfer.
*           len is number of bytes to be transfered.
*           rw is 1 for read data and 0 for write data.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FSdmmcPs_doTransfer(FSdmmcPs_T *pSdmmc, unsigned int address, unsigned int len,  int rw)
{
    int ret;
    FSdmmcPs_SdCmd_T *pCmd;
    FSdmmcPs_Instance_T *pInstance;
    
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    pCmd = &pInstance->sdCmd;

    if(((pInstance->card_type == MMC) || (pInstance->card_type == EMMC)) && (len > 0x200))
    {
        ret = sdmmc_cmd23(pSdmmc, len);
        if(ret)
        {
            TRACE_OUT(DEBUG_OUT, "send cmd23 error\r\n");
            return ret;
        }
    }

    if (rw) /* read-1*/
    {
        pCmd->cmd = len > 512 ? CMD_READ_MULTIPLE_BLOCK : CMD_READ_SINGLE_BLOCK;

        if(pInstance->HighCapacity)
            pCmd->arg = address; /* unit: block number*/
        else
            pCmd->arg = address * pInstance->read_blk_len; /* block size: 512Bytes*/
			
        pCmd->res_type = CMD_RESP_TYPE_R1;
        pCmd->res_flag = CMD_RESP_FLAG_RDATA;
		
        pCmd->data = (char *)pInstance->priv;
		
        pCmd->data_len = len;
        pCmd->block_size = pInstance->read_blk_len;

		/*printf(" pInstance->sendCmd(%d) len:%d \n", pCmd->cmd, len);*/
		
        ret = pInstance->sendCmd(pSdmmc);
        if (ret )
        {
            TRACE_OUT(DEBUG_OUT, "FSdmmcPs_doTransfer(rd): CMD_%d failed! \r\n", pCmd->cmd);
            return ret;
        }
    } 
    else   /* write-0*/
    {
        pCmd->cmd = len > 512 ? CMD_WRITE_MULTIPLE_BLOCK : CMD_WRITE_BLOCK;
		
        if(pInstance->HighCapacity)
            pCmd->arg = address;
        else
            pCmd->arg = address * pInstance->write_blk_len;
			
        pCmd->res_type = CMD_RESP_TYPE_R1;
        pCmd->res_flag = CMD_RESP_FLAG_WDATA;
        pCmd->data = (char *)pInstance->priv;
		
        pCmd->data_len = len;
        pCmd->block_size = pInstance->write_blk_len;
		
        ret = pInstance->sendCmd(pSdmmc);
        if (ret)
        {
            TRACE_OUT(DEBUG_OUT, "FSdmmcPs_doTransfer(wr): CMD_%d failed! \r\n", pCmd->cmd);
            return ret;
        }
    }

	int retry = 10000;
	
	/*
	if (pInstance->transMode == sdmmc_trans_mode_dw_dma)
	{
		do
		{
			// check card-state. if card-state != StandBy, return BUSY 
			ret = sdmmc_cmd13(pSdmmc);
			
			retry--;
			if (retry <= 0)
			{
				return FMSH_FAILURE;
			}
		} while (pCmd->rsp_buf[0] != 0x100);
	}
	else  // fifo mode
	*/
	{
	    do
	    {
	        /* check card-state. if card-state != StandBy, return BUSY */
	        ret = sdmmc_cmd13(pSdmmc);
			
			retry--;  /* for test */
			if (retry <= 0)
			{
				return FMSH_FAILURE;
			}
	    } while (pCmd->rsp_buf[0] != 0x900);
	}	

    return 0;
}

/*****************************************************************************
*
* @description
* This function runs to force the card to stop transmission.
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
int FSdmmcPs_stopTransfer(FSdmmcPs_T *pSdmmc)
{
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    return sdmmc_cmd12(pSdmmc);
}

/*****************************************************************************
*
* @description
* This function performs data read transmission.The buffer size must be at
* least 512 byte long.
* User can select the transfer mode to use with DMA or not.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*           address is address of the block to read.
*           pBuf is read data buffer, its size at least the block size.
*           len is number of bytes to be transfered.
*           trans_mode is enumerated data transfer mode.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FSdmmcPs_readFun(FSdmmcPs_T *pSdmmc, u32 address, char *pBuf, u32 len, enum SDMMC_TransMode mode)
{
    int status;
    FSdmmcPs_Instance_T *pInstance;
    
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;

    pInstance->priv = pBuf;
    pInstance->transMode = mode;
    status = pInstance->doTransfer(pSdmmc, address, len, 1);  /* 1 - read*/
    
    return status;
}

/*****************************************************************************
*
* @description
* This function performs data write transmission.The buffer size must be at
* least 512 byte long.
* User can select the transfer mode to use with DMA or not.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*           address is address of the block to write.
*           pBuf is read data buffer, its size at least the block size.
*           len is number of bytes to be transfered.
*           trans_mode is enumerated data transfer mode.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FSdmmcPs_writeFun(FSdmmcPs_T *pSdmmc, u32 address, char *pBuf, u32 len, enum SDMMC_TransMode mode)
{
    int status;
    FSdmmcPs_Instance_T *pInstance;
    
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;

    pInstance->priv = pBuf;
    pInstance->transMode = mode;
    status = pInstance->doTransfer(pSdmmc, address, len, 0);  /* 0 - write*/
    
    return status;
}

/*****************************************************************************
*
* @description
* This function adds a new SD/MMC/EMMC card.
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
int FSdmmcPs_addSdDevice(FSdmmcPs_T *pSdmmc)
{
    int temp;
    FSdmmcPs_Instance_T *pInstance;
    
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    
    pInstance->stat = 0;
    temp = FSdmmcPs_cardIdentify(pSdmmc);
    if(temp)
    {
        pInstance->stat = 0;
        return temp;
    }
    pInstance->stat = 1;
    return 0;
}

/*****************************************************************************
*
* @description
* This function removes a SD/MMC/EMMC card.
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
int FSdmmcPs_removeSdDevice(FSdmmcPs_T *pSdmmc)
{
    FSdmmcPs_Instance_T *pInstance;
    
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    pInstance = pSdmmc->instance;
    
    pInstance->powerDown(pSdmmc);
    pInstance->stat = 0;
    return 0;
}

/*****************************************************************************
*
* @description
* This function initializes a specific card.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*
* @return
*           FMSH_SUCCESS if the operation was successful.
*           FMSH_FAILURE if the operation was failed.
*
* @note     NA.
*
*****************************************************************************/
s32 FSdmmcPs_initCard(FSdmmcPs_T *pSdmmc)
{
    int card_in;	
    /*FSdmmcPs_Instance_T *Instance;*/
    u32 tmp32 = 0;
    
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    /*pInstance = pSdmmc->instance;*/
    
    /* clear int */
	
	delay_ms(1);
	tmp32 = FSdmmcPs_getRawIntStat(pSdmmc);
    FSdmmcPs_setRawIntStat(pSdmmc, tmp32);

    if (pSdmmc->config.CardDetect == 1)
    {
        card_in = FSdmmcPs_cardDetect(pSdmmc);
        if (card_in != CARD_INSERT)
        {
            TRACE_OUT(DEBUG_OUT, "No card insert.\r\n");
            return FMSH_FAILURE;
        }
    }
    
    if(!FSdmmcPs_addSdDevice(pSdmmc))
    {
        /*card enumerate successfull*/
        TRACE_OUT(DEBUG_OUT, "Enumerate successfull.\r\n");
        return FMSH_SUCCESS;
    }
    else
    {
        TRACE_OUT(DEBUG_OUT, "Enumerate failed.\r\n");
        FSdmmcPs_removeSdDevice(pSdmmc);
        return FMSH_FAILURE;
    }
}

/*****************************************************************************
*
* @description
* This function initializes a specific sdmmc instance
* such that the driver is ready to use.
*
* @param
*           pSdmmc is the pointer to a sd card device.
*           pDmac is the pointer to the DMA controller device.
*           trans_mode is enumerated data transfer mode.
*
* @return
*           FMSH_SUCCESS if the operation was successful.
*           FMSH_FAILURE if the operation was failed.
*
* @note     NA.
*
*****************************************************************************/
s32 FSdmmcPs_cfgInitialize(FSdmmcPs_T *pSdmmc, FDmaPs_T *pDmac, enum SDMMC_TransMode trans_mode)
{
    int ret = FMSH_SUCCESS;
    
    SDMMC_COMMON_REQUIREMENTS(pSdmmc);
    
    FSdmmcPs_instanceReset(pSdmmc, pDmac);

    /********* Initialize the SD controller **********/
	
    /* step1: Reset the chip. */
    ret = FSdmmcPs_sdControllerReset(pSdmmc);
    if (ret < 0)
        return FMSH_FAILURE;
	
    ret = FSdmmcPs_sdFifoReset(pSdmmc);
    if (ret < 0)
        return FMSH_FAILURE;
    
    /* step2: set fifo */
    FSdmmcPs_setDmaMsize(pSdmmc, 2);  /* dma MSize = 8，TX_WMark = 8，RX_WMark = 7*/
    
    FSdmmcPs_setRXwmark(pSdmmc, 7); /* fifo*/
    FSdmmcPs_setTXwmark(pSdmmc, 8);
	
    /* step3: clear int */
    FSdmmcPs_setRawIntStat(pSdmmc, 0xFFFFFFFF);
	
    /* step4: enable int */
    FSdmmcPs_enableInt(pSdmmc);
	
    /* step5: mask interrupt disable */
    FSdmmcPs_disableIntMask(pSdmmc);

    if (trans_mode == sdmmc_trans_mode_dw_dma)
    {
        FSdmmcPs_Instance_T *pInstance;
        pInstance = pSdmmc->instance;
        
        /* step6: enable DMA */
        FSdmmcPs_enableDMA(pSdmmc);
        
        /* step7: Initialize the DMA controller */
        ret = FDmaPs_init(pInstance->pDma);
        if (ret < 0)
            return FMSH_FAILURE;
        FDmaPs_enable(pInstance->pDma);
    }
    
    return FMSH_SUCCESS;
}


