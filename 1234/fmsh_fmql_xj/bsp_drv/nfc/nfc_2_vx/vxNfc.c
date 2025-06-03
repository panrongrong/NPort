/* vxNfc.c - fmsh 7020/7045 pNfc driver */

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
01a, 24Dec19, jc  written.
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

#include "vxNandFlash.h"
#include "vxNfc.h"

/*
defines 
*/
#if 1

#undef VX_DBG
#undef VX_DBG2

#define NFC_DBG

#ifdef NFC_DBG

#define VX_DBG(string, a, b, c, d, e, f)                    \
        if (_func_logMsg != NULL)                           \
           printf(string, a, b, c, d, e, f)  /* (* _func_logMsg)(string, a, b, c, d, e, f) */
#else
#define VX_DBG(string, a, b, c, d, e, f)
#endif

/* ret info log */
#define VX_DBG2(string, a, b, c, d, e, f) printf(string, a, b, c, d, e, f)
#endif

extern UINT32 FlashCmdIdx_to_NfcRegVal(UINT32 flash_cmd_idx);

/* 
pNfc: 
*/
static vxT_NFC_CTRL vxNfc_Ctrl_S = {0};
static vxT_NFC vx_pNfc_S = {0};
vxT_NFC * g_pNfc_S = (vxT_NFC *)(&vx_pNfc_S.nfc_x);


#if 1


void nfcCtrl_Wr_CfgReg32(vxT_NFC* pNfc, UINT32 offset, UINT32 value)
{
	UINT32 tmp32 = pNfc->pNfcCtrl->cfgBaseAddr;
	FMQL_WRITE_32((tmp32 + offset), value);
	return;
}

UINT32 nfcCtrl_Rd_CfgReg32(vxT_NFC* pNfc, UINT32 offset)
{
	UINT32 tmp32 = pNfc->pNfcCtrl->cfgBaseAddr;
	return FMQL_READ_32(tmp32 + offset);
}

/***********************************
 * FIFO
 ******************/
void nfcCtrl_Wr_Fifo(vxT_NFC* pNfc, UINT32 data)
{
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_FIFO_DATA_OFFSET, data);
	return;
}

UINT32 nfcCtrl_Rd_Fifo(vxT_NFC* pNfc)
{
    return nfcCtrl_Rd_CfgReg32(pNfc, NFCPS_FIFO_DATA_OFFSET);
}

void nfcCtrl_Clr_Fifo(vxT_NFC* pNfc)
{
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_FIFO_INIT_OFFSET, NFCPS_FIFO_INIT_MASK);
	return;
}

/***************************
* CTRL
*************/
void nfcCtrl_Wr_CtrlReg(vxT_NFC* pNfc, UINT32 ctrl_val)
{
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_CONTROL_OFFSET, ctrl_val);
	return;
}

UINT32 nfcCtrl_Rd_CtrlReg(vxT_NFC* pNfc)
{
    return nfcCtrl_Rd_CfgReg32(pNfc, NFCPS_CONTROL_OFFSET);
}

/***********************************
 * INT_MASK
 *****************/
void nfcCtrl_Wr_IrqMask(vxT_NFC* pNfc, UINT32 mask)
{ 
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_INT_MASK_OFFSET, mask);
	return;
}

UINT32 nfcCtrl_Rd_IrqMask(vxT_NFC* pNfc)
{ 
    return nfcCtrl_Rd_CfgReg32(pNfc, NFCPS_INT_MASK_OFFSET);
}

/***********************************
 * INT_STATUS
 *****************/
void nfcCtrl_Clr_IrqStatus(vxT_NFC* pNfc)
{
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_INT_STATUS_OFFSET, (~NFCPS_INTR_ALL));
	return;
}

UINT32 nfcCtrl_Rd_IrqStatus(vxT_NFC* pNfc)
{  
    return nfcCtrl_Rd_CfgReg32(pNfc, NFCPS_INT_STATUS_OFFSET);
}


/**********************
 * STATUS_MASK
 *********************/
void nfcCtrl_Wr_StatusMask(vxT_NFC* pNfc, UINT32 mask)
{  
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_STATUS_MASK_OFFSET, mask);
	return;
}

/*****************
 * TIMING
 ****************/
void nfcCtrl_Set_Timing(vxT_NFC* pNfc)
{
    UINT32 cfgReg = 0;
	
    cfgReg = ((FPS_NFC_TIMING_TWHR & 0x3f) << 24) | 
                ((FPS_NFC_TIMING_TRHW & 0x3f) << 16) | 
                ((FPS_NFC_TIMING_TADL & 0x3f) << 8) | 
                ((FPS_NFC_TIMING_TCCS & 0x3f) << 0);
	
   nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_TIME_SEQ_0_OFFSET, cfgReg);
    
    cfgReg = ((FPS_NFC_TIMING_TWW & 0x3f) << 16) | 
                ((FPS_NFC_TIMING_TRR & 0x3f) << 8) | 
                ((FPS_NFC_TIMING_TWB & 0x3f) << 0);
	
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_TIME_SEQ_1_OFFSET, cfgReg);
    
    cfgReg = ((FPS_NFC_TIMING_TRWH & 0xf) << 4) | 
                ((FPS_NFC_TIMING_TRWP & 0xf));
	
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_TIMINGS_ASYN_OFFSET, cfgReg);
	
	return;
}



int nfcCtrl_Get_FlashInfo(vxT_NFC* pNfc, int log_flag)
{ 
    int ret;
    UINT32 first_word, second_word;
    UINT8 Id1, Id2, Id3, Id4, Id5;
	
	UINT32 flashID[2]= {0};	
	
	vxT_PARAMPAGE_FLASH ParamPage = {0};

	/**/
    /* Reset Nandflash chip*/
    /**/
    ret = nfcFlash_Rst_NandChip(pNfc);
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }

	/**/
    /* read id*/
    /**/
    ret = nfcFlash_Get_ChipId(pNfc, (UINT32*)(&flashID[0]));
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
	
    /* calculate ID*/
    Id1 = flashID[0] & 0xff;
    Id2 = (flashID[0] >> 8)  & 0xff;
    Id3 = (flashID[0] >> 16) & 0xff;
    Id4 = (flashID[0] >> 24) & 0xff;
	
    Id5 = flashID[1] & 0xff;

	if (log_flag == 1)
	{
		printf("<vx> nand_id(MT29F1G16A 0x5580B12C-00000004): 0x%08X-%08X \n", flashID[0], flashID[1]);
		
		printf("Manufacture: 0x%X \n", pNfc->pNfcCtrl->NAND.Manufacture);
		printf("DeviceId: 0x%X \n\n", pNfc->pNfcCtrl->NAND.DevId);
	}

	/**/
    /* Set NandflashModel*/
    /**/
    pNfc->pNfcCtrl->NAND.Manufacture = Id1;
    pNfc->pNfcCtrl->NAND.DevId = Id2;
    pNfc->pNfcCtrl->NAND.Options = 0;
	
    /* Fetch from the extended ID4
    * ID4 D5  D4 BlockSizeKB || D1  D0  PageSize
    *     0   0   64K      || 0   0   1K
    *     0   1   128K     || 0   1   2K
    *     1   0   256K     || 1   0   4K
    *     1   1   512K     || 1   1   8k 
    */
    
    /* Page Size*/
    switch (Id4 & 0x03) 
	{
    case 0x00: 
		pNfc->pNfcCtrl->NAND.PageSize = 1024; 
		break;
    case 0x01: 
		pNfc->pNfcCtrl->NAND.PageSize = 2048; 
		break;
    case 0x02: 
		pNfc->pNfcCtrl->NAND.PageSize = 4096; 
		break;
    case 0x03: 
		pNfc->pNfcCtrl->NAND.PageSize = 8192; 
		break;
    }	
	if (log_flag == 1)
	{
		printf("PageSize: %d \n", pNfc->pNfcCtrl->NAND.PageSize);
	}
	
    /* Spare Size*/
    switch(Id4 & 0x04)
	{
    case 0x00: 
		pNfc->pNfcCtrl->NAND.SpareSize = 32; 
		break;
    case 0x04: 
		pNfc->pNfcCtrl->NAND.SpareSize = 64; 
		break;
    }	
	if (log_flag == 1)
	{
		printf("SpareSize: %d \n", pNfc->pNfcCtrl->NAND.SpareSize);
	}	
	
    /* Block Size*/
    switch (Id4 & 0x30) 
	{
    case 0x00: 
		pNfc->pNfcCtrl->NAND.BlockSizeKB = 64;  
		break;
    case 0x10: 
		pNfc->pNfcCtrl->NAND.BlockSizeKB = 128; 
		break;
    case 0x20: 
		pNfc->pNfcCtrl->NAND.BlockSizeKB = 256; 
		break;
    case 0x30: 
		pNfc->pNfcCtrl->NAND.BlockSizeKB = 512; 
		break;
    }	
	if (log_flag == 1)
	{
		printf("BlockSizeKB: %d KB \n", pNfc->pNfcCtrl->NAND.BlockSizeKB);
	}
	
    /* IO width*/
    switch (Id4 & 0x40)
	{
    case 0x00: 
		pNfc->pNfcCtrl->NAND.IoWidth = 8;  
		break;
    case 0x40: 
		pNfc->pNfcCtrl->NAND.IoWidth = 16; 
		break;
    }	
	if (log_flag == 1)
	{
		printf("IoWidth: %d bit \n\n", pNfc->pNfcCtrl->NAND.IoWidth);
	}
	
    /* calculate other parameters*/
    pNfc->pNfcCtrl->NAND.PagePerBlock = (UINT16)(((UINT32)pNfc->pNfcCtrl->NAND.BlockSizeKB << 10) / pNfc->pNfcCtrl->NAND.PageSize);
    pNfc->pNfcCtrl->NAND.BlockPerLun = NAND_MAX_BLOCKS;
    pNfc->pNfcCtrl->NAND.LunNum = 0x1;
	
    pNfc->pNfcCtrl->NAND.BlockNum =  pNfc->pNfcCtrl->NAND.BlockPerLun * pNfc->pNfcCtrl->NAND.LunNum;
    pNfc->pNfcCtrl->NAND.PageNum = pNfc->pNfcCtrl->NAND.PagePerBlock * pNfc->pNfcCtrl->NAND.BlockPerLun * pNfc->pNfcCtrl->NAND.LunNum;
    pNfc->pNfcCtrl->NAND.DevSizeMB = (UINT16)((pNfc->pNfcCtrl->NAND.BlockSizeKB * pNfc->pNfcCtrl->NAND.BlockPerLun * pNfc->pNfcCtrl->NAND.LunNum) >> 10);

	pNfc->pNfcCtrl->NAND.RowAddrCycle = 3;
    pNfc->pNfcCtrl->NAND.ColAddrCycle = 2;
	if (log_flag == 1)
	{	
		printf("PagePerBlock: %d \n", pNfc->pNfcCtrl->NAND.PagePerBlock);
		printf("BlockNum: %d \n", pNfc->pNfcCtrl->NAND.BlockNum);
		printf("PageNum: %d \n", pNfc->pNfcCtrl->NAND.PageNum);
		printf("DeviceSizeInMegaBytes: %d MB \n", pNfc->pNfcCtrl->NAND.DevSizeMB);

		printf("\n");
	}


	/**/
    /* Get Nandflash Id(check if support ONFI)*/
    /**/
    /*
    if (nfcFlash_Get_OnfiId(pNfc) ==  NANDFLASH_ONFI_ID)
	{
		//
        // device support onfi then read parameter page(256 bytes)
        //
        ret = nfcFlash_Get_ParamPage(pNfc, &ParamPage);
        if (ret == FMSH_FAILURE)
		{
            return FMSH_FAILURE;
        }
		
        // Initialize parameter
        pNfc->pNfcCtrl->NAND.Manufacture = ParamPage.JedecManufacturerId;    
        pNfc->pNfcCtrl->NAND.PageSize = ParamPage.BytesPerPage;
        pNfc->pNfcCtrl->NAND.SpareSize = ParamPage.SpareBytesPerPage;
        pNfc->pNfcCtrl->NAND.PagePerBlock = ParamPage.PagesPerBlock;
        pNfc->pNfcCtrl->NAND.BlockSizeKB = (ParamPage.PagesPerBlock * ParamPage.BytesPerPage)>>10;
        pNfc->pNfcCtrl->NAND.BlockPerLun = ParamPage.BlocksPerLun;
        pNfc->pNfcCtrl->NAND.DevSizeMB = (ParamPage.NumLuns * ParamPage.BlocksPerLun * pNfc->pNfcCtrl->NAND.BlockSizeKB)>>10;
        pNfc->pNfcCtrl->NAND.PageNum = ParamPage.PagesPerBlock * ParamPage.BlocksPerLun * ParamPage.NumLuns;
        pNfc->pNfcCtrl->NAND.BlockNum =  ParamPage.BlocksPerLun * ParamPage.NumLuns;
        pNfc->pNfcCtrl->NAND.LunNum = ParamPage.NumLuns;
        
        pNfc->pNfcCtrl->NAND.RowAddrCycle = ParamPage.AddrCycles & 0xf;
        pNfc->pNfcCtrl->NAND.ColAddrCycle = (ParamPage.AddrCycles >> 4) & 0xf;
    }
	*/
	
    pNfc->pNfcCtrl->busWidth = pNfc->pNfcCtrl->NAND.IoWidth;
    
    return FMSH_SUCCESS;
}

int nfcCtrl_Wait_ExeOk(vxT_NFC* pNfc, UINT32 offset, UINT32 regMask, UINT8 ok_flag)
{
    UINT32 tmp32 = 0;
    int timeout = 4500;
    
    do
	{
        sysUsDelay(1);
		
        tmp32 = nfcCtrl_Rd_CfgReg32(pNfc, offset) & regMask; 
		switch (ok_flag)
		{
		case OK_FLAG_1:
			if (tmp32 == regMask)
			{
				return FMSH_SUCCESS;
			} 
			break;
			
		case OK_FLAG_0:
			if (tmp32 == 0x00)
			{
				return FMSH_SUCCESS;
			}			
			break;
		}
		
        timeout--;
    } while (timeout > 0);

	return FMSH_FAILURE;
}


int nfcCtrl_Set_IoWidth(vxT_NFC* pNfc, UINT32 width)
{
    UINT32 cfgReg = 0;
       
    /* 
    validity check 
	*/
    if ((width != 8) && (width != 16))
    {
        return FMSH_FAILURE;
    }
    
    cfgReg = nfcCtrl_Rd_CfgReg32(pNfc, NFCPS_CONTROL_OFFSET);
    if (width == 8)
    {
        cfgReg &= ~NFCPS_IOWIDTH16_MASK;
    }
    else if(width == 16)
    {
        cfgReg |= NFCPS_IOWIDTH16_MASK;
    }
	
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_CONTROL_OFFSET, cfgReg);    
    
    return FMSH_SUCCESS;
}


int nfcCtrl_Set_BlockSize(vxT_NFC* pNfc, UINT32 size)
{
    UINT32 cfgReg = 0, sizeCfg = 0;
    
    /* validity check */
    if (size == 32)
    {
        sizeCfg = 0;
    }
    else if (size == 64)
    {
        sizeCfg = 1;
    }
    else if (size == 128)
    {
        sizeCfg = 2;
    }
    else if (size == 256)
    {
        sizeCfg = 3;
    }
    else
    {
        return FMSH_FAILURE;
    } 
    
    cfgReg = nfcCtrl_Rd_CfgReg32(pNfc, NFCPS_CONTROL_OFFSET);
	
    cfgReg &= ~NFCPS_BLOCK_SIZE_MASK;
    cfgReg |= (sizeCfg << NFCPS_BLOCK_SIZE_SHIFT) & NFCPS_BLOCK_SIZE_MASK;
	
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_CONTROL_OFFSET, cfgReg);
    
    return FMSH_SUCCESS;
}


int nfcCtrl_Set_EccBlockSize(vxT_NFC* pNfc, UINT32 size)
{
    UINT32 cfgReg = 0, sizeCfg = 0;
    
    /* validity check */
    if(size == 256)
    {
        sizeCfg = 0;
    }
    else if(size == 512)
    {
        sizeCfg = 1;
    }
    else if(size == 1024)
    {
        sizeCfg = 2;
    }
    else if(size == 2048)
    {
        sizeCfg = 3;
    }
    else
    {
        return FMSH_FAILURE;
    }
    
    cfgReg = nfcCtrl_Rd_CfgReg32(pNfc, NFCPS_CONTROL_OFFSET);
	
    cfgReg &= ~NFCPS_ECC_BLOCK_SIZE_MASK;
    cfgReg |= (sizeCfg << NFCPS_ECC_BLOCK_SIZE_SHIFT) & NFCPS_ECC_BLOCK_SIZE_MASK;
	
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_CONTROL_OFFSET, cfgReg);
    
    return FMSH_SUCCESS;
}

void nfcCtrl_En_Ecc(vxT_NFC* pNfc)
{
    UINT32 cfgReg = 0;
    
    cfgReg = nfcCtrl_Rd_CfgReg32(pNfc, NFCPS_CONTROL_OFFSET);
    cfgReg |= NFCPS_ECC_EN_MASK;
	
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_CONTROL_OFFSET, cfgReg);    
    return;
}

void nfcCtrl_Dis_Ecc(vxT_NFC* pNfc)
{
    UINT32 cfgReg = 0;
    
    cfgReg = nfcCtrl_Rd_CfgReg32(pNfc, NFCPS_CONTROL_OFFSET);
    cfgReg &= ~NFCPS_ECC_EN_MASK;
	
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_CONTROL_OFFSET, cfgReg);    
    return;
}

/***********************************
 * ECC_STAT
 * 
 *
 ****************/
void nfcCtrl_Wr_EccOffset(vxT_NFC* pNfc, UINT16 offset)
{
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ECC_OFFSET_OFFSET, offset);
    return;
}

UINT16 nfcCtrl_Rd_EccOffset(vxT_NFC* pNfc)
{
    return nfcCtrl_Rd_CfgReg32(pNfc, NFCPS_ECC_OFFSET_OFFSET);
}


/*************************************
 * ECC_CTRL
 * all configuration parameters required by the ECC
 * IntrSrc-correctable ret/uncorrectable ret/acceptable errors level overflow
 * Errlvl-number of errors that is acceptable for the host system
 * EccCap-2/4/8/16/24/32
 ************************/
int nfcCtrl_Set_EccCtrl(vxT_NFC* pNfc, UINT32 sel, UINT32 threshold, UINT32 cap)
{
    UINT32 cfgReg = 0;
    
    /* validity check */
    if ((sel != NFCPS_ECC_ERROR_STATE)    \
		&& (sel != NFCPS_ECC_UNC_STATE)   \
		&& (sel != NFCPS_ECC_OVER_STATE))
    {
        return FMSH_FAILURE;
    }
	   
    if (threshold > 0x3f)
    {
        return FMSH_FAILURE;
    }
	
    if ((cap != NFCPS_ECC_CAP2_STATE) && (cap != NFCPS_ECC_CAP4_STATE)   &&  \
        (cap != NFCPS_ECC_CAP8_STATE) && (cap != NFCPS_ECC_CAP16_STATE)  &&  \
        (cap != NFCPS_ECC_CAP24_STATE) && (cap != NFCPS_ECC_CAP32_STATE))
    {
        return FMSH_FAILURE;
    }
    
    cfgReg = (sel << NFCPS_ECC_SEL_SHIFT) | (threshold << NFCPS_ECC_THRESHOLD_SHIFT) | cap;  	   
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ECC_CTRL_OFFSET, cfgReg);
    
    return FMSH_SUCCESS;
}


int nfcCtrl_Set_EccMode(vxT_NFC* pNfc, UINT32 eccMode)
{
	int ret = 0;
	UINT32 feature;
	UINT32 offset;

	nfcCtrl_Dis_Ecc(pNfc);

	switch (eccMode)
	{
	case NFCPS_ECC_NONE_VX:
		break;

	case NFCPS_ECC_ONDIE_VX:
		feature = nfcFlash_Get_Feature(pNfc, 0x90);
		if ((feature & 0x08) != 0x08)
		{ 
			ret = nfcFlash_Set_Feature(pNfc, 0x90, 0x08);
			if (ret == FMSH_FAILURE)
			{
				return FMSH_FAILURE;
			}
		}
		break;

	case NFCPS_ECC_HW_VX:
		nfcCtrl_En_Ecc(pNfc);
		
		offset = SLCR_REG_BASE + nandInfo_Get_PageSize(pNfc) + NFCPS_ECC_HW_OFFSET;
	
		if (pNfc->pNfcCtrl->NAND.IoWidth == 8)
		{
			nfcCtrl_Wr_EccOffset(pNfc, offset);
		} 
		else if(pNfc->pNfcCtrl->NAND.IoWidth == 16)
		{
			nfcCtrl_Wr_EccOffset(pNfc, (offset >> 1));
		}
		else
		{
			return FMSH_FAILURE;
		}
		
		ret = nfcCtrl_Set_EccCtrl(pNfc, NFCPS_ECC_UNC_STATE, 2, NFCPS_ECC_CAP2_STATE);   
		if (ret == FMSH_FAILURE)
		{
		  return FMSH_FAILURE;
		}
		break;
		
	case NFCPS_ECC_SW_VX:
		/* not implemented */
		break;
		
	default: 
		break;
	}

	return FMSH_SUCCESS;
}


/******************************
 * BBM
 *****************/

/*
RMP_INIT	[0]	remapping初始标志。 如果设置此标志，强制BBM模块在软件更新后重新读取remapping表。该标志由软件置1并由硬件清零。
*/
void nfcCtrl_Init_ReMap(vxT_NFC* pNfc)
{
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_BBM_CTRL_OFFSET, 0x1);
	return;
}

/*
PTR_ADDR	[31: 0]	remapping表指针。 该字段包含系统内存中的重映射表的地址。
*/
void nfcCtrl_Set_Dev0PTR(vxT_NFC* pNfc, UINT32 address)
{
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_DEV0_PTR_OFFSET, address);
	return;
}

void nfcCtrl_Set_Dev0Size(vxT_NFC* pNfc, UINT32 size)
{
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_DEV0_SIZE_OFFSET, size);
	return;
}

UINT32 nfcCtrl_Get_HwProt(vxT_NFC* pNfc)
{ 
    UINT32 cfgReg = 0;
	
    cfgReg = nfcCtrl_Rd_CfgReg32(pNfc, NFCPS_MEM_CTRL_OFFSET);
	
    cfgReg &= NFCPS_CTRL_MEM0WP_MASK;
    cfgReg >>= NFCPS_CTRL_MEM0WP_SHIFT;
	
    return cfgReg;   
}

#endif



#if 1
int nfcFlash_Rst_NandChip(vxT_NFC* pNfc)
{
    int ret = 0;
    UINT32 reg_val = 0;
    
    /* 
    Set reg_val (Reset) 
	*/ 
    reg_val = FlashCmdIdx_to_NfcRegVal(NANDFLASH_RESET);
	
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_COMMAND_OFFSET, reg_val);
	
    /* 
    Wait for Ready 
	*/
    ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);

	if (ret == FMSH_FAILURE)
    {
        return FMSH_FAILURE;
    }
	else
	{
	    return FMSH_SUCCESS;
	}    
}


UINT32 nfcFlash_Get_OnfiId(vxT_NFC* pNfc)
{
    int ret = 0;
    UINT32 reg_val;
    UINT32 onfi_id;
    
    /* Set Address*/
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ADDR0_COL_OFFSET, 0x20);
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ADDR0_ROW_OFFSET, 0);
	
    /* 0x40: Set Data Reg Size (4 Bytes)*/
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_DATA_REG_SIZE_OFFSET, 0x3); /* bit[1~0]: 11 - all four bytes valid */
	
    /* Set reg_val (Data Reg & Read ONFI ID)*/
    /*
    bit[7]: DATA_SEL
		0 – the FIFO module selected
		1 – the DATA register selected
	*/
    reg_val = FlashCmdIdx_to_NfcRegVal(NANDFLASH_READ_ID);
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_COMMAND_OFFSET, (reg_val | (0x1<<7)));  /* use data reg*/
    
    /* Wait For Ready*/
    /*
	MEM0_ST [0] 设备0状态标志：
			1 - 设备ready
			0 - 设备busy
	*/
    ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);
    if (ret == FMSH_FAILURE)
    {
        return FMSH_FAILURE;
    }
	
    /* Wait For Data Reg Avaliable*/
    /*
	DATA_REG_ST [10]	DATA_REG：只有通过读取DATA_REG寄存器中的数据才能复位该标志。
			1 - DATA_REG中的数据可用
			0 - DATA_REG中的数据不可用
	*/
    ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_STATUS_OFFSET, NFCPS_STAT_DATA_REG_RDY_MASK, OK_FLAG_1);
    if (ret == FMSH_FAILURE)
    {
        return FMSH_FAILURE;
    }
	
    /* Get Status*/
    onfi_id = nfcCtrl_Rd_CfgReg32(pNfc, NFCPS_DATA_REG_OFFSET);
    
    return onfi_id;
}


int nfcFlash_Get_ChipId(vxT_NFC* pNfc, UINT32* flashID)
{
    int ret = 0;
    UINT32 reg_val = 0;
    
    /* Set Address*/
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ADDR0_COL_OFFSET, 0);
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ADDR0_ROW_OFFSET, 0);
	
    /* 0x84: Set Data Size*/
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_DATA_SIZE_OFFSET, 5);
	
    /* Set reg_val ( FIFO & SIU & RDID)*/
    reg_val = FlashCmdIdx_to_NfcRegVal(NANDFLASH_READ_ID);
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_COMMAND_OFFSET, reg_val);
	
    /* Wait For FIFO Empty*/
    /*
	CF_EMPTY	[2]	命令FIFO 空标志。
					该位指示实际的命令FIFO状态。它不能用于检查下一次传输是否会被接受。
	*/
    ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_FIFO_STATE_OFFSET, NFCPS_STAT_CFIFO_EMPTY_MASK, OK_FLAG_1);
    if (ret == FMSH_FAILURE)
    {
        return FMSH_FAILURE;
    }
	
    /* Wait For Status Ready*/
    /*
	MEM0_ST [0] 设备0状态标志：
		1 - 设备ready
		0 - 设备busy
	*/
    ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);
    if (ret == FMSH_FAILURE)
    {
        return FMSH_FAILURE;
    }
	
    /* Wait for FIFO DFR not empty*/
    /*
	DF_R_EMPTY	[0]	FIFO空 状态位。
					该位表示FIFO中没有数据可用。该标志对读取方向有效。
	*/
    ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_FIFO_STATE_OFFSET, NFCPS_STAT_DFIFO_R_EMPTY_MASK, OK_FLAG_0);
    if (ret == FMSH_FAILURE)
    {
        return FMSH_FAILURE;
    }
	
    /* Get Id*/
    flashID[0] = nfcCtrl_Rd_Fifo(pNfc); 
    flashID[1] = nfcCtrl_Rd_Fifo(pNfc); 
    
    return FMSH_SUCCESS;
}


int nfcFlash_Get_ParamPage(vxT_NFC* pNfc, void* ParamPage)
{
    int ret = 0;
    UINT32 reg_val;
    int ByteCount;
    
    UINT32 * pBuf32 = (UINT32 *)ParamPage;
    
    /* Set Address*/
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ADDR0_COL_OFFSET, 0);
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ADDR0_ROW_OFFSET, 0);
	
    /* Set Data Size*/
    ByteCount = 256;
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_DATA_SIZE_OFFSET, ByteCount);

	/**/
    /* Set reg_val ( FIFO & SIU & RD PARA PAGE)*/
    /**/
    reg_val = FlashCmdIdx_to_NfcRegVal(NANDFLASH_READ_PARAMETER_PAGE);
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_COMMAND_OFFSET, reg_val);
	
	
    /* Wait For FIFO Empty*/
    /*
	CF_EMPTY	[2]	命令FIFO 空标志。
					该位指示实际的命令FIFO状态。它不能用于检查下一次传输是否会被接受。
	*/
    ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_FIFO_STATE_OFFSET, NFCPS_STAT_CFIFO_EMPTY_MASK, OK_FLAG_1);
    if (ret == FMSH_FAILURE)
    {
        return FMSH_FAILURE;
    }
	
    /* Wait For Status Ready*/
    /*
	MEM0_ST [0] 设备0状态标志：
		1 - 设备ready
		0 - 设备busy
	*/
    ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);
    if (ret == FMSH_FAILURE)
    {
        return FMSH_FAILURE;
    }	
	
    /* Wait for FIFO DFR not empty*/
    /*
	DF_R_EMPTY	[0]	FIFO空 状态位。
					该位表示FIFO中没有数据可用。该标志对读取方向有效。
	*/
    ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_FIFO_STATE_OFFSET, NFCPS_STAT_DFIFO_R_EMPTY_MASK, OK_FLAG_0);
    if (ret == FMSH_FAILURE)
    {
        return FMSH_FAILURE;
    }

	/**/
    /* Get ParamPage*/
    /**/
    while (ByteCount > 0)
    {
        *pBuf32 = nfcCtrl_Rd_Fifo(pNfc); 
        pBuf32++;
		
        ByteCount -= 4;
    }
    
    return FMSH_SUCCESS;
}

int nfcFlash_Chk_FlashId(vxT_NFC* pNfc)
{
	UINT8 maker = pNfc->pNfcCtrl->NAND.Manufacture;
	UINT8 ioWidth = pNfc->pNfcCtrl->NAND.IoWidth;

	/* 
	validity check 
	*/
	if ((maker != NAND_MICRON_ID)      \
		&& (maker != NAND_SPANSION_ID) \
		&& (maker != NAND_FMSH_ID))
	{
		return FMSH_FAILURE;
	} 
	  
	if ((ioWidth != 8) && (ioWidth != 16)) 
	{
		return FMSH_FAILURE;
	}

	return FMSH_SUCCESS;
}


UINT32 nfcFlash_Get_Feature(vxT_NFC* pNfc, UINT8 feature_in)
{
    int ret = 0;
    UINT32 reg_val = 0;
    UINT32 feature;
	
    /* Set Address*/
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ADDR0_COL_OFFSET, feature_in);
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ADDR0_ROW_OFFSET, 0);
	
    /* Set Data Reg Size (4 Bytes)*/
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_DATA_REG_SIZE_OFFSET, 0x3);
	
    /* Set Command (Data Reg & Get Feature)*/
    reg_val = FlashCmdIdx_to_NfcRegVal(NANDFLASH_GET_FEATURE);
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_COMMAND_OFFSET, (reg_val | (0x1 << 7)));
	
    /* Wait For Ready*/
    /*
	MEM0_ST [0] 设备0状态标志：
			1 - 设备ready
			0 - 设备busy
	*/
    ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);
    if (ret == FMSH_FAILURE)
    {
        return FMSH_FAILURE;
    }
		
    /* Wait For Data Reg Avaliable*/
    /*
	DATA_REG_ST [10]	DATA_REG：只有通过读取DATA_REG寄存器中的数据才能复位该标志。
			1 - DATA_REG中的数据可用
			0 - DATA_REG中的数据不可用
	*/
    ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_STATUS_OFFSET, NFCPS_STAT_DATA_REG_RDY_MASK, OK_FLAG_1);
    if (ret == FMSH_FAILURE)
    {
        return FMSH_FAILURE;
    }
	
    /* Get Status*/
    feature =  nfcCtrl_Rd_CfgReg32(pNfc, NFCPS_DATA_REG_OFFSET);
	
    return feature;
}


int nfcFlash_Set_Feature(vxT_NFC* pNfc, UINT8 feature, UINT32 Value)
{
    int ret = 0;
    UINT32 reg_val = 0;
    
    /* Set Address*/
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ADDR0_COL_OFFSET, feature);  
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ADDR0_ROW_OFFSET, 0);
	
    /* Set Data Size*/
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_DATA_SIZE_OFFSET, 4);
	
    /* Set FIFO*/
    nfcCtrl_Clr_Fifo(pNfc);
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_FIFO_DATA_OFFSET, Value);
	
    /* Set Command (Program)*/
    reg_val = FlashCmdIdx_to_NfcRegVal(NANDFLASH_SET_FEATURE);
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_COMMAND_OFFSET, reg_val);
	
    /* Wait For CFIFO Empty*/
    /*
    DF_W_EMPTY	[7]	FIFO空状态位。
    				该位表示FIFO中没有数据可用。该标志对写入方向有效。
	CF_EMPTY	[2]	命令FIFO 空标志。
					该位指示实际的命令FIFO状态。它不能用于检查下一次传输是否会被接受。
	*/
    ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_FIFO_STATE_OFFSET, \
                             (NFCPS_STAT_DFIFO_W_EMPTY_MASK | NFCPS_STAT_CFIFO_EMPTY_MASK), OK_FLAG_1);
    if (ret == FMSH_FAILURE)
    {
        return FMSH_FAILURE;
    }
		
    /* Wait For Ready*/
    /*
	MEM0_ST [0] 设备0状态标志：
			1 - 设备ready
			0 - 设备busy
	*/
    ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);
    if (ret == FMSH_FAILURE)
    {
        return FMSH_FAILURE;
    }
    
    return FMSH_SUCCESS;
}

/**
* nfcFlash_Read_Page: read a page
* Nandflash_ReadSpare: read pSpareBuf area
*/
int nfcFlash_Read_Page(vxT_NFC* pNfc, UINT16 block, UINT16 page, UINT16 offset, void* pDataBuf, void* pSpareBuf)
{
    int ret;
	
    UINT32 ctrl;
    UINT32 reg_val;
    UINT32 colAddr, rowAddr;
    UINT32 byteCnt, cnt; 
    UINT32 *data32, *spare32;    

	/* 
	page data_area 
	*/
    if (pDataBuf)
    {
        data32 = (UINT32*)pDataBuf; 
		
        /* ByteCount is page size */                       
        byteCnt= pNfc->pNfcCtrl->NAND.PageSize;
		
        /* Set Address */
        if (pNfc->pNfcCtrl->NAND.IoWidth == 8)
        {
            colAddr = offset;
        }
        else if (pNfc->pNfcCtrl->NAND.IoWidth == 16)
        {
            colAddr = offset >> 1;
        }
        else
        {
            return FMSH_FAILURE;
        }
		
        rowAddr = (block << 6) | page;
		
        nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ADDR0_COL_OFFSET, colAddr);
        nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ADDR0_ROW_OFFSET, rowAddr);
		
        /* Set Data Size to be transfered */
        nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_DATA_SIZE_OFFSET, byteCnt);
		
        nfcCtrl_Clr_Fifo(pNfc);
		
        /* Set Command (Read) */
        reg_val = FlashCmdIdx_to_NfcRegVal(NANDFLASH_READ_PAGE);
        nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_COMMAND_OFFSET, reg_val);
		
        /* Wait For CFIFO Empty */
        ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_FIFO_STATE_OFFSET, NFCPS_STAT_CFIFO_EMPTY_MASK, OK_FLAG_1);
        if (ret == FMSH_FAILURE)
        {
            return FMSH_FAILURE;
        }
		
        /* Wait For Ready */
        ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);
        if (ret == FMSH_FAILURE)
        {
            return FMSH_FAILURE;
        }
		
        /* Read pDataBuf from fifo if dfifo is not empty*/
        while(byteCnt > 0)
        {
            ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_FIFO_STATE_OFFSET, NFCPS_STAT_DFIFO_R_EMPTY_MASK, OK_FLAG_0);
			if (ret == FMSH_FAILURE)
			{
				return FMSH_FAILURE;
			}
			
            *data32 = nfcCtrl_Rd_CfgReg32(pNfc, NFCPS_FIFO_DATA_OFFSET); 
            data32++;
			
            byteCnt -= 4;  
        }  
    }
	
	/* 
	page spare_area 
	*/
    if (pSpareBuf)
    {
        spare32 = (UINT32*)pSpareBuf;
		
        ctrl = nfcCtrl_Rd_CtrlReg(pNfc);
		
        /* Disable Ecc */
        if ((ctrl & NFCPS_ECC_EN_MASK) == 1)
        {
            nfcCtrl_Wr_CtrlReg(pNfc, ctrl & ~NFCPS_ECC_EN_MASK);
        }
		
        /* Set Address */
        if(pNfc->pNfcCtrl->NAND.IoWidth == 8)
        {
            colAddr = pNfc->pNfcCtrl->NAND.PageSize;
        }
        else if(pNfc->pNfcCtrl->NAND.IoWidth == 16)
        {
            colAddr = pNfc->pNfcCtrl->NAND.PageSize >> 1;
        }
        else
        {
            return FMSH_FAILURE;
        }
		
        rowAddr = (block << 6) | page;
		
        nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ADDR0_COL_OFFSET, colAddr);
        nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ADDR0_ROW_OFFSET, rowAddr);
		
        /* Set Data Size */
        cnt = pNfc->pNfcCtrl->NAND.SpareSize;
        nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_DATA_SIZE_OFFSET, cnt);
        nfcCtrl_Clr_Fifo(pNfc);
		
        /* Set Command (Read) */
        reg_val = FlashCmdIdx_to_NfcRegVal(NANDFLASH_READ_PAGE);
        nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_COMMAND_OFFSET, reg_val);
		
        /* Wait For FIFO Empty */
        ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_FIFO_STATE_OFFSET, NFCPS_STAT_CFIFO_EMPTY_MASK, OK_FLAG_1);
        if (ret == FMSH_FAILURE)
        {
            return FMSH_FAILURE;
        }
		
        /* Wait For Ready */
        ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);
        if (ret == FMSH_FAILURE)
        {
            return FMSH_FAILURE;
        }
		
        /* Read pDataBuf from fifo */
        while(cnt > 0)
        {
            ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_FIFO_STATE_OFFSET, NFCPS_STAT_DFIFO_R_EMPTY_MASK, OK_FLAG_0);
            if (ret == FMSH_FAILURE)
            {
                return FMSH_FAILURE;
            }  
			
            *spare32 = nfcCtrl_Rd_CfgReg32(pNfc, NFCPS_FIFO_DATA_OFFSET); 
            spare32++;
			
            cnt -= 4;  
        } 
		
        /* Re-Enable Ecc */
        if ((ctrl & NFCPS_ECC_EN_MASK) == 1)
        {
            nfcCtrl_Wr_CtrlReg(pNfc, ctrl);
        }
    }
    
    return FMSH_SUCCESS;
}

/**
* Nandflash_WritePage: write a page
*/
int nfcFlash_Write_Page(vxT_NFC* pNfc, UINT16 block, UINT16 page, UINT16 offset, void* pDataBuf, void* pSpareBuf)
{
    int ret;
    UINT32 ctrl;
    UINT32 reg_val;
    UINT32 colAddr, rowAddr;
    UINT32 byteCnt, cnt;
    UINT32* data32, *spare32;      
    
    if (pDataBuf)
    {
        data32 = (UINT32*)pDataBuf;
		
        /* ByteCount is page size */
        byteCnt = pNfc->pNfcCtrl->NAND.PageSize;
		
        while (byteCnt > 0)
        {
            /* Set Address */
            if(pNfc->pNfcCtrl->NAND.IoWidth == 8)
            {
                colAddr = offset;
            }
            else if(pNfc->pNfcCtrl->NAND.IoWidth == 16)
            {
                colAddr = offset >> 1;
            }
            else
            {
                return FMSH_FAILURE;
            }
			
            rowAddr = (block << 6) | page;
			
            nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ADDR0_COL_OFFSET, colAddr);
            nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ADDR0_ROW_OFFSET, rowAddr);
			
            /* Set Data Size to be transfered */
            if(byteCnt > NFCPS_FIFO_DEPTH)
            {
                /* page size is larger than fifo depth (2048 Bytes) */
                cnt = NFCPS_FIFO_DEPTH;
            }
            else
            {
                cnt = byteCnt;
            }
			
            offset += cnt;
            byteCnt -= cnt;
			
            nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_DATA_SIZE_OFFSET, cnt);
			
            /* Set FIFO */
            nfcCtrl_Clr_Fifo(pNfc);
			
            while (cnt > 0)
            {
                nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_FIFO_DATA_OFFSET, *data32);
                data32++;
				
                cnt -= 4;
            }
			
            /* Set Command (Program) */
            reg_val = FlashCmdIdx_to_NfcRegVal(NANDFLASH_PROGRAM_PAGE);
            nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_COMMAND_OFFSET, reg_val);
			
            /* Wait for FIFO_STATE.DF_EMPTY */
            ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_FIFO_STATE_OFFSET,  \
                                     (NFCPS_STAT_DFIFO_W_EMPTY_MASK | NFCPS_STAT_CFIFO_EMPTY_MASK), OK_FLAG_1);
            if (ret == FMSH_FAILURE)
            {
                return FMSH_FAILURE;
            }
			
            /* Wait For Ready */
            ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);
            if (ret == FMSH_FAILURE)
            {
                return FMSH_FAILURE;
            }
        }
    }  
	
    if(pSpareBuf)
    {
        spare32 = (UINT32*)pSpareBuf;
		
        ctrl = nfcCtrl_Rd_CtrlReg(pNfc);
		
        /* Disable Ecc */
        if ((ctrl & NFCPS_ECC_EN_MASK) == 1)
        {
            nfcCtrl_Wr_CtrlReg(pNfc, ctrl & ~NFCPS_ECC_EN_MASK);
        }
		
        /* Set Address */
        if(pNfc->pNfcCtrl->NAND.IoWidth == 8)
        {
            colAddr = pNfc->pNfcCtrl->NAND.PageSize;
        }
        else if(pNfc->pNfcCtrl->NAND.IoWidth == 16)
        {
            colAddr = pNfc->pNfcCtrl->NAND.PageSize >> 1;
        }
		
        rowAddr = (block << 6) | page;
		
        nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ADDR0_COL_OFFSET, colAddr);
        nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ADDR0_ROW_OFFSET, rowAddr);
		
        /* Set Data Size */
        cnt = pNfc->pNfcCtrl->NAND.SpareSize;
        nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_DATA_SIZE_OFFSET, cnt);
		
        /* Set FIFO */
        nfcCtrl_Clr_Fifo(pNfc);
		
        while (cnt > 0)
        {
            nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_FIFO_DATA_OFFSET, *spare32);
            spare32++;
			
            cnt -= 4;
        }
		
        /* Set Command (Program) */
        reg_val = FlashCmdIdx_to_NfcRegVal(NANDFLASH_PROGRAM_PAGE);
        nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_COMMAND_OFFSET, reg_val);
		
        /* Wait for FIFO_STATE.DF_EMPTY */
        ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_FIFO_STATE_OFFSET, \
                                 (NFCPS_STAT_DFIFO_W_EMPTY_MASK | NFCPS_STAT_CFIFO_EMPTY_MASK), OK_FLAG_1);
        if (ret == FMSH_FAILURE)
        {
            return FMSH_FAILURE;
        }
		
        /* Wait For Ready */
        ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);
        if (ret == FMSH_FAILURE)
        {
            return FMSH_FAILURE;
        }
		
        /* Re-Enable Ecc */
        if((ctrl & NFCPS_ECC_EN_MASK) == 1)
        {
            nfcCtrl_Wr_CtrlReg(pNfc, ctrl);
        }
    }
    
    return FMSH_SUCCESS; 
}

int nfcFlash_Read_MultiPage(vxT_NFC* pNfc, UINT64 srcAddr, UINT32 byteCnt, UINT8* pDstBuf, UINT8* pUsrSpare)
{
	int ret;
	UINT64 devSize;
	UINT16 block, page, offset;
	UINT32 length;

	UINT8 pDataBuf[NAND_MAX_PAGE_DATASIZE];
	UINT8 pSpareBuf[NAND_MAX_PAGE_SPARESIZE];


	/* validity check */
	/*devSize = nandInfo_Get_DevSize_Bytes(pNfc);*/
	devSize = (UINT64)pNfc->pNfcCtrl->NAND.DevSizeMB << 20;
	
	if (byteCnt == 0)
	{
		return FMSH_FAILURE;
	}

	if ((srcAddr + byteCnt) > devSize)
	{
		return FMSH_FAILURE;
	}

	/* remaining pDataBuf to be read */
	while (byteCnt > 0)
	{
		/* calculate address */
		nandInfo_Parse_FlashAddr(pNfc, srcAddr, byteCnt, &block, &page, &offset);

		/* read entire page from nandflash */
		ret = nfcFlash_Read_Page(pNfc, block, page, 0, pDataBuf, pSpareBuf);
		if(ret == FMSH_FAILURE)
		{
		  return FMSH_FAILURE;
		}

		/* maxium length of bytes needed */
		length = pNfc->pNfcCtrl->NAND.PageSize - offset;

		/* check has more pDataBuf to read */
		if (byteCnt > length)
		{
			memcpy(pDstBuf, &pDataBuf[offset], length);
			pDstBuf += length;
			srcAddr += length;
			byteCnt -= length;
		} 
		else
		{
			memcpy(pDstBuf, &pDataBuf[offset], byteCnt);
			byteCnt = 0;
		}
	}

	if (pUsrSpare)
	{
		memcpy(pUsrSpare, &pSpareBuf[0], pNfc->pNfcCtrl->NAND.SpareSize);
	}

	return FMSH_SUCCESS;
}


int nfcFlash_Write_MultiPage(vxT_NFC* pNfc, UINT64 dstAddr, UINT32 byteCnt, UINT8 *pSrcBuf, UINT8 *pUsrSpare)
{
	int ret;

	UINT64 devSize;
	UINT16 block, page, offset;
	UINT32 length;

	UINT8 pDataBuf[NAND_MAX_PAGE_DATASIZE];
	UINT8 pSpareBuf[NAND_MAX_PAGE_SPARESIZE];


	/* validity check */
	/*devSize = nandInfo_Get_DevSize_Bytes(pNfc);*/
	devSize = (UINT64)pNfc->pNfcCtrl->NAND.DevSizeMB << 20;
	
	if (byteCnt == 0)
	{
		return FMSH_FAILURE;
	}
	
	if ((dstAddr + byteCnt) > devSize)
	{
		return FMSH_FAILURE;
	}
	
	/* 
	check write protect status 
	*/
	if (nfcCtrl_Get_HwProt(pNfc) == 0)
	{
		return FMSH_FAILURE;
	}

	/* copy pSpareBuf buffer */
	if (pUsrSpare)
	{
		memcpy(&pSpareBuf[0], pUsrSpare, pNfc->pNfcCtrl->NAND.SpareSize);
	} 
	else
	{
		memset(&pSpareBuf[0], 0xff, NAND_MAX_PAGE_SPARESIZE);
	}

	while (byteCnt > 0)
	{
		/* calculate address */
		nandInfo_Parse_FlashAddr(pNfc, dstAddr, byteCnt, &block, &page, &offset);
		
		/* length of bytes to be send to nandflash */
		length = pNfc->pNfcCtrl->NAND.PageSize - offset;
		if (byteCnt < length)
		{
		  length = byteCnt;
		}
		
		/* init & fill buffer */
		memset(&pDataBuf[0], 0xff, NAND_MAX_PAGE_DATASIZE);
		memcpy(&pDataBuf[offset], pSrcBuf, length);
		
		pSrcBuf += length;
		
		/* write to nandflash */
		ret = nfcFlash_Write_Page(pNfc, block, page, 0, pDataBuf, pSpareBuf);
		if (ret == FMSH_FAILURE)
		{
		  return FMSH_FAILURE;
		}
		
		/* check has more pDataBuf in buffer to write */
		if (byteCnt > length)
		{
			dstAddr += length;
			byteCnt -= length;
		} 
		else
		{
			byteCnt = 0;
		}
	}

	return FMSH_SUCCESS;
}

/*******************************
* Erase Blcok
* @block 
***********/
int nfcFlash_Erase_Block(vxT_NFC* pNfc, UINT32 block_idx) 
{
    int ret = 0;
    UINT32 reg_val;
    
    /* Set Address */
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ADDR0_COL_OFFSET, 0);
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_ADDR0_ROW_OFFSET, (block_idx << 6));
	
    /* Set Command(seq14) */
    reg_val = FlashCmdIdx_to_NfcRegVal(NANDFLASH_ERASE_BLOCK);
    nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_COMMAND_OFFSET, reg_val);
	
    /* Wait for Flash Ready */
    ret = nfcCtrl_Wait_ExeOk(pNfc, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);
    if (ret == FMSH_FAILURE)
    {
        return FMSH_FAILURE;
    }
    
    return FMSH_SUCCESS;
}



#endif




#if 1

void nfcSlcr_Rst_NfcCtrl(vxT_NFC* pNfc)
{  
	UINT32 tmp32 = 0;

	tmp32 = slcr_read(SLCR_NFC_CTRL);
	tmp32 |= (NFC_AHB_RST_MASK);
	slcr_write(SLCR_NFC_CTRL, tmp32);
	
	tmp32 = slcr_read(SLCR_NFC_CTRL);
	tmp32 |= (NFC_REF_RST_MASK);
	slcr_write(SLCR_NFC_CTRL, tmp32);

	/*delay_1ms();*/
	
	tmp32 = slcr_read(SLCR_NFC_CTRL);
	tmp32 &= ~(NFC_AHB_RST_MASK);
	slcr_write(SLCR_NFC_CTRL, tmp32);
	
	tmp32 = slcr_read(SLCR_NFC_CTRL);
	tmp32 &= ~(NFC_REF_RST_MASK);
	slcr_write(SLCR_NFC_CTRL, tmp32);
	
    /* 
    clear RETRY_EN bit
	*/
	nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_CONTROL_OFFSET, 0x0);
	
    /* 
    disable protect
	*/
	nfcCtrl_Wr_CfgReg32(pNfc, NFCPS_MEM_CTRL_OFFSET, 0x0100);

	return;
}

extern int nfcSlcr_Set_NfcMIO_2(void);

void nfcSlcr_Set_NfcMIO_e(void)
{
	UINT32 tmp32 = 0, tmp32_2 = 0;
	UINT32 offset = SLCR_REG_BASE + 0, val32 = 0;	

	
    /* UNLOCK_KEY = 0XDF0D767B*/
    /*EMIT_WRITE(SLCR_REG_BASE + 0x008, 0xDF0D767BU),*/
	offset = SLCR_REG_BASE + 0x008;
	val32 = 0xDF0D767BU;
	FMQL_WRITE_32(offset, val32);
    
	/* MIO_PIN_0*/
	/* .. SIGNAL   = nand_cs*/
	/* .. DIR	   = out*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x700, 0x0000FFFFU, 0x00003610U),*/
	offset = SLCR_REG_BASE + 0x700;
	val32 = 0x00003610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_2*/
	/* .. SIGNAL   = nand_alen*/
	/* .. DIR	   = out*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = disabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x708, 0x0000FFFFU, 0x00002610U),*/
	offset = SLCR_REG_BASE + 0x708;
	val32 = 0x00002610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_3*/
	/* .. SIGNAL   = nand_web*/
	/* .. DIR	   = out*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = disabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x70C, 0x0000FFFFU, 0x00002610U),*/
	offset = SLCR_REG_BASE + 0x70C;
	val32 = 0x00002610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_4*/
	/* .. SIGNAL   = nand_io[2]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = disabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x710, 0x0000FFFFU, 0x00002610U),*/
	offset = SLCR_REG_BASE + 0x710;
	val32 = 0x00002610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_5*/
	/* .. SIGNAL   = nand_io[0]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = disabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x714, 0x0000FFFFU, 0x00002610U),*/
	offset = SLCR_REG_BASE + 0x714;
	val32 = 0x00002610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_6*/
	/* .. SIGNAL   = nand_io[1]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = disabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x718, 0x0000FFFFU, 0x00002610U),*/
	offset = SLCR_REG_BASE + 0x718;
	val32 = 0x00002610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_7*/
	/* .. SIGNAL   = nand_cleb*/
	/* .. DIR	   = out*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = disabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x71C, 0x0000FFFFU, 0x00002610U),*/
	offset = SLCR_REG_BASE + 0x71C;
	val32 = 0x00002610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;	
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_8*/
	/* .. SIGNAL   = nand_rdb*/
	/* .. DIR	   = out*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = disabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x720, 0x0000FFFFU, 0x00002610U),*/
	offset = SLCR_REG_BASE + 0x720;
	val32 = 0x00002610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;	
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_9*/
	/* .. SIGNAL   = nand_io[4]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x724, 0x0000FFFFU, 0x00003610U),*/
	offset = SLCR_REG_BASE + 0x724;
	val32 = 0x00003610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;	
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_10*/
	/* .. SIGNAL   = nand_io[5]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x728, 0x0000FFFFU, 0x00003610U),*/
	offset = SLCR_REG_BASE + 0x728;
	val32 = 0x00003610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;	
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_11*/
	/* .. SIGNAL   = nand_io[6]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x72C, 0x0000FFFFU, 0x00003610U),*/
	offset = SLCR_REG_BASE + 0x72C;
	val32 = 0x00003610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;	
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_12*/
	/* .. SIGNAL   = nand_io[7]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x730, 0x0000FFFFU, 0x00003610U),*/
	offset = SLCR_REG_BASE + 0x730;
	val32 = 0x00003610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;	
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_13*/
	/* .. SIGNAL   = nand_io[3]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x734, 0x0000FFFFU, 0x00003610U),*/
	offset = SLCR_REG_BASE + 0x734;
	val32 = 0x00003610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;	
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_14*/
	/* .. SIGNAL   = nand_busy*/
	/* .. DIR	   = in*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x738, 0x0000FFFFU, 0x00003611U),*/
	offset = SLCR_REG_BASE + 0x738;
	val32 = 0x00003610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;	
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_16*/
	/* .. SIGNAL   = nand_io[8]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x740, 0x0000FFFFU, 0x00003610U),	*/
	offset = SLCR_REG_BASE + 0x740;
	val32 = 0x00003610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;	
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_17*/
	/* .. SIGNAL   = nand_io[9]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x744, 0x0000FFFFU, 0x00003610U),*/
	offset = SLCR_REG_BASE + 0x744;
	val32 = 0x00003610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;	
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_18*/
	/* .. SIGNAL   = nand_io[10]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x748, 0x0000FFFFU, 0x00003610U),*/
	offset = SLCR_REG_BASE + 0x748;
	val32 = 0x00003610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;	
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_19*/
	/* .. SIGNAL   = nand_io[11]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x74C, 0x0000FFFFU, 0x00003610U),*/
	offset = SLCR_REG_BASE + 0x74C;
	val32 = 0x00003610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;	
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_20*/
	/* .. SIGNAL   = nand_io[12]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x750, 0x0000FFFFU, 0x00003610U),*/
	offset = SLCR_REG_BASE + 0x750;
	val32 = 0x00003610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;	
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_21*/
	/* .. SIGNAL   = nand_io[13]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x754, 0x0000FFFFU, 0x00003610U),*/
	offset = SLCR_REG_BASE + 0x754;
	val32 = 0x00003610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;	
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_22*/
	/* .. SIGNAL   = nand_io[14]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x758, 0x0000FFFFU, 0x00003610U),	*/
	offset = SLCR_REG_BASE + 0x758;
	val32 = 0x00003610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;	
	FMQL_WRITE_32(offset, tmp32_2);
	
	/* MIO_PIN_23*/
	/* .. SIGNAL   = nand_io[15]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x75C, 0x0000FFFFU, 0x00003610U),*/
	offset = SLCR_REG_BASE + 0x75C;
	val32 = 0x00003610;
	tmp32 = FMQL_READ_32(offset);
	tmp32_2 = (tmp32 & 0xFFFF0000) | val32;	
	FMQL_WRITE_32(offset, tmp32_2);
	
    /* LOCK_KEY = 0XDF0D767B*/
    /*EMIT_WRITE(SLCR_REG_BASE + 0x004, 0xDF0D767BU),*/
	offset = SLCR_REG_BASE + 0x004;
	val32 = 0xDF0D767BU;
	FMQL_WRITE_32(offset, val32);
	
	return;
}

void nfcSlcr_Set_NfcMIO(void)
{
	UINT32 tmp32 = 0, tmp32_2 = 0;
	UINT32 offset = 0, val32 = 0;	
	
	UINT32 REG_MASK = 0x00000000; /*0xFFFF0000*/
		
	/* MIO_PIN_0*/
	/* .. SIGNAL   = nand_cs*/
	/* .. DIR	   = out*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x700, 0x0000FFFFU, 0x00003610U),*/
	offset = 0x700;
	val32 = 0x00003610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;
	slcr_write(offset, val32);
	
	/* MIO_PIN_2*/
	/* .. SIGNAL   = nand_alen*/
	/* .. DIR	   = out*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = disabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x708, 0x0000FFFFU, 0x00002610U),*/
	offset = 0x708;
	val32 = 0x00002610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;
	slcr_write(offset, tmp32_2);
	
	/* MIO_PIN_3*/
	/* .. SIGNAL   = nand_web*/
	/* .. DIR	   = out*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = disabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x70C, 0x0000FFFFU, 0x00002610U),*/
	offset = 0x70C;
	val32 = 0x00002610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;
	slcr_write(offset, tmp32_2);
	
	/* MIO_PIN_4*/
	/* .. SIGNAL   = nand_io[2]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = disabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x710, 0x0000FFFFU, 0x00002610U),*/
	offset = 0x710;
	val32 = 0x00002610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;
	slcr_write(offset, tmp32_2);
	
	/* MIO_PIN_5*/
	/* .. SIGNAL   = nand_io[0]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = disabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x714, 0x0000FFFFU, 0x00002610U),*/
	offset = 0x714;
	val32 = 0x00002610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;
	slcr_write(offset, tmp32_2);
	
	/* MIO_PIN_6*/
	/* .. SIGNAL   = nand_io[1]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = disabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x718, 0x0000FFFFU, 0x00002610U),*/
	offset = 0x718;
	val32 = 0x00002610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;
	slcr_write(offset, tmp32_2);
	
	/* MIO_PIN_7*/
	/* .. SIGNAL   = nand_cleb*/
	/* .. DIR	   = out*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = disabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x71C, 0x0000FFFFU, 0x00002610U),*/
	offset = 0x71C;
	val32 = 0x00002610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;	
	slcr_write(offset, tmp32_2);
	
	/* MIO_PIN_8*/
	/* .. SIGNAL   = nand_rdb*/
	/* .. DIR	   = out*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = disabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x720, 0x0000FFFFU, 0x00002610U),*/
	offset = 0x720;
	val32 = 0x00002610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;	
	slcr_write(offset, tmp32_2);
	
	/* MIO_PIN_9*/
	/* .. SIGNAL   = nand_io[4]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x724, 0x0000FFFFU, 0x00003610U),*/
	offset = 0x724;
	val32 = 0x00003610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;	
	slcr_write(offset, tmp32_2);
	
	/* MIO_PIN_10*/
	/* .. SIGNAL   = nand_io[5]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x728, 0x0000FFFFU, 0x00003610U),*/
	offset = 0x728;
	val32 = 0x00003610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;	
	slcr_write(offset, tmp32_2);
	
	/* MIO_PIN_11*/
	/* .. SIGNAL   = nand_io[6]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x72C, 0x0000FFFFU, 0x00003610U),*/
	offset = 0x72C;
	val32 = 0x00003610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;	
	slcr_write(offset, tmp32_2);
	
	/* MIO_PIN_12*/
	/* .. SIGNAL   = nand_io[7]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x730, 0x0000FFFFU, 0x00003610U),*/
	offset = 0x730;
	val32 = 0x00003610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;	
	slcr_write(offset, tmp32_2);
	
	/* MIO_PIN_13*/
	/* .. SIGNAL   = nand_io[3]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x734, 0x0000FFFFU, 0x00003610U),*/
	offset = 0x734;
	val32 = 0x00003610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;	
	slcr_write(offset, tmp32_2);
	
	/* MIO_PIN_14*/
	/* .. SIGNAL   = nand_busy*/
	/* .. DIR	   = in*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x738, 0x0000FFFFU, 0x00003611U),*/
	offset = 0x738;
	val32 = 0x00003610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;	
	slcr_write(offset, tmp32_2);
	
	/* MIO_PIN_16*/
	/* .. SIGNAL   = nand_io[8]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x740, 0x0000FFFFU, 0x00003610U),	*/
	offset = 0x740;
	val32 = 0x00003610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;	
	slcr_write(offset, tmp32_2);
	
	/* MIO_PIN_17*/
	/* .. SIGNAL   = nand_io[9]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x744, 0x0000FFFFU, 0x00003610U),*/
	offset = 0x744;
	val32 = 0x00003610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;	
	slcr_write(offset, tmp32_2);
	
	/* MIO_PIN_18*/
	/* .. SIGNAL   = nand_io[10]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x748, 0x0000FFFFU, 0x00003610U),*/
	offset = 0x748;
	val32 = 0x00003610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;	
	slcr_write(offset, tmp32_2);
	
	/* MIO_PIN_19*/
	/* .. SIGNAL   = nand_io[11]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x74C, 0x0000FFFFU, 0x00003610U),*/
	offset = 0x74C;
	val32 = 0x00003610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;	
	slcr_write(offset, tmp32_2);
	
	/* MIO_PIN_20*/
	/* .. SIGNAL   = nand_io[12]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x750, 0x0000FFFFU, 0x00003610U),*/
	offset = 0x750;
	val32 = 0x00003610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;	
	slcr_write(offset, tmp32_2);
	
	/* MIO_PIN_21*/
	/* .. SIGNAL   = nand_io[13]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x754, 0x0000FFFFU, 0x00003610U),*/
	offset = 0x754;
	val32 = 0x00003610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;	
	slcr_write(offset, tmp32_2);
	
	/* MIO_PIN_22*/
	/* .. SIGNAL   = nand_io[14]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x758, 0x0000FFFFU, 0x00003610U),	*/
	offset = 0x758;
	val32 = 0x00003610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;	
	slcr_write(offset, tmp32_2);
	
	/* MIO_PIN_23*/
	/* .. SIGNAL   = nand_io[15]*/
	/* .. DIR	   = inout*/
	/* .. L3-0 SEL = B'0001000*/
	/* .. SPEED    = slow*/
	/* .. IO TYPE  = LVCMOS 3.3V*/
	/* .. PULLUP   = enabled*/
	/* .. DRIVE STRENTH   = 8*/
	/*EMIT_MASKWRITE(SLCR_REG_BASE + 0x75C, 0x0000FFFFU, 0x00003610U),*/
	offset = 0x75C;
	val32 = 0x00003610;
	tmp32 = slcr_read(offset);
	tmp32_2 = (tmp32 & REG_MASK) | val32;	
	slcr_write(offset, tmp32_2);
	
	return;
}


#endif

#if 1


int vxInit_Nfc(void)
{
	vxT_NFC * pNfc = NULL;
	vxT_NFC_CTRL * pNfcCtrl = NULL;
	
	int ret = 0;	
	UINT32 tmp32 = 0;
	
	UINT32 ctrlReg_val = 0;
	
#if 1 
	/*
	init the pNfc structure
	*/

	/**/
	/* spi_ctrl select*/
	/**/
	bzero((char*)(&g_pNfc_S->nfc_x), sizeof(vxT_NFC));	
	pNfc = g_pNfc_S;	
	
	bzero((char*)(&vxNfc_Ctrl_S.ctrl_x), sizeof(vxT_NFC_CTRL));
	pNfc->pNfcCtrl = (vxT_NFC_CTRL *)(&vxNfc_Ctrl_S.ctrl_x);
	
	pNfcCtrl = pNfc->pNfcCtrl;	
	
	pNfc->nfc_x = NFC_CTRL_0;	
	pNfcCtrl->ctrl_x = pNfc->nfc_x;
	pNfcCtrl->devId = pNfc->nfc_x;
	
	pNfcCtrl->cfgBaseAddr = VX_NFC_S_CFG_BASE;			
	pNfcCtrl->busWidth = FPS_NFC_S_FLASHWIDTH;
#endif	

	/**/
	/* setup nfc_MIO of slcr*/
	/**/
	/*nfcSlcr_Set_NfcMIO();*/
	/*nfcSlcr_Set_NfcMIO_2();*/

	nfcSlcr_Rst_NfcCtrl(pNfc);

	pNfcCtrl->eccMode = NFCPS_ECC_NONE_VX;
	
	nfcCtrl_Wr_StatusMask(pNfc, 0x0140);
	
	nfcCtrl_Set_Timing(pNfc);

#if 1
	nfcCtrl_Wr_StatusMask(pNfc, 0x0140);

	nfcCtrl_Get_FlashInfo(pNfc, 1);  /* 1-with log, 0-no log*/
	
	ret = nfcFlash_Chk_FlashId(pNfc);
    if (ret == FMSH_FAILURE)
    {
        return FMSH_FAILURE;
    }
	
	/* 
	set ctrl 
	*/
	ctrlReg_val = NFCPS_AUTO_READ_STAT_EN_MASK | NFCPS_READ_STATUS_EN_MASK;
	nfcCtrl_Wr_CtrlReg(pNfc, (ctrlReg_val & NFCPS_USER_CTRL_MASK));
	
	/* 
	set iowidth 
	*/
	ret = nfcCtrl_Set_IoWidth(pNfc, pNfc->pNfcCtrl->busWidth);
    if (ret == FMSH_FAILURE)
    {
        return FMSH_FAILURE;
    }
	
	/* 
	set blocksize 
	*/
	ret = nfcCtrl_Set_BlockSize(pNfc, (pNfc->pNfcCtrl->NAND.PagePerBlock));
    if (ret == FMSH_FAILURE)
    {
        return FMSH_FAILURE;
    }

	/* 
	set ecc_blocksize 
	*/
	ret = nfcCtrl_Set_EccBlockSize(pNfc, (pNfc->pNfcCtrl->NAND.PageSize >> 2)); 	
    if (ret == FMSH_FAILURE)
    {
        return FMSH_FAILURE;
    }

	/* 
	set interrupt 
	*/
	ctrlReg_val = nfcCtrl_Rd_CtrlReg(pNfc);    
	if (ctrlReg_val & NFCPS_INT_EN_MASK)
	{
	  nfcCtrl_Wr_IrqMask(pNfc, (~NFCPS_INTR_ALL));
	}
	
	/* 
	set ecc 
	*/
	if (ctrlReg_val & NFCPS_ECC_EN_MASK)
	{		 
		if(pNfc->pNfcCtrl->eccMode != NFCPS_ECC_HW_VX)
		{
			nfcCtrl_Dis_Ecc(pNfc);
		}
	}
	
	/* 
	set ecc mdoe
	*/
	nfcCtrl_Set_EccMode(pNfc, pNfc->pNfcCtrl->eccMode);

	/* 
	set bbm 
	*/
	nand_Init_BadBlkTable_Desc(pNfc);
	ret = nand_Scan_BadBlkTable(pNfc);
	if (ret == FMSH_FAILURE)
	{
		return FMSH_FAILURE;
	}
	
	if (ctrlReg_val & NFCPS_BBM_EN_MASK)
	{
		nfcCtrl_Set_Dev0PTR(pNfc, (UINT32)pNfc->pNfcCtrl->map_BB);
		nfcCtrl_Set_Dev0Size(pNfc, (NAND_MAX_BADBLKS >> 3));
		nfcCtrl_Init_ReMap(pNfc); 
	}
#endif

	pNfc->init_flag	= 1;

	return FMSH_SUCCESS;
}





#endif


#if 1

int get_FlashID(void)
{ 
	vxT_NFC* pNfc = g_pNfc_S;
		
    int ret;
    UINT32 first_word, second_word;
    UINT8 Id1, Id2, Id3, Id4, Id5;
	
	UINT32 flashID[2]= {0};	
	
	vxT_PARAMPAGE_FLASH ParamPage = {0};
	
	if (pNfc->init_flag == 0)
	{
		vxInit_Nfc();
	}
	
	/**/
    /* Reset Nandflash chip*/
    /**/
    ret = nfcFlash_Rst_NandChip(pNfc);
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }

	/**/
    /* read id*/
    /**/
    ret = nfcFlash_Get_ChipId(pNfc, (UINT32*)(&flashID[0]));
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
	
    /* calculate ID*/
    Id1 = flashID[0] & 0xff;
    Id2 = (flashID[0] >> 8)  & 0xff;
    Id3 = (flashID[0] >> 16) & 0xff;
    Id4 = (flashID[0] >> 24) & 0xff;
	
    Id5 = flashID[1] & 0xff;
	
	printf("<vx> nand_id(MT29F1G16A 0x5580B12C-00000004): 0x%08X-%08X \n", flashID[0], flashID[1]);
	
	printf("Manufacture: 0x%X \n", pNfc->pNfcCtrl->NAND.Manufacture);
	printf("DeviceId: 0x%X \n\n", pNfc->pNfcCtrl->NAND.DevId);

	/**/
    /* Set NandflashModel*/
    /**/
    pNfc->pNfcCtrl->NAND.Manufacture = Id1;
    pNfc->pNfcCtrl->NAND.DevId = Id2;
    pNfc->pNfcCtrl->NAND.Options = 0;
	
    /* Fetch from the extended ID4
    * ID4 D5  D4 BlockSizeKB || D1  D0  PageSize
    *     0   0   64K      || 0   0   1K
    *     0   1   128K     || 0   1   2K
    *     1   0   256K     || 1   0   4K
    *     1   1   512K     || 1   1   8k 
    */
    
    /* Page Size*/
    switch (Id4 & 0x03) 
	{
    case 0x00: 
		pNfc->pNfcCtrl->NAND.PageSize = 1024; 
		break;
    case 0x01: 
		pNfc->pNfcCtrl->NAND.PageSize = 2048; 
		break;
    case 0x02: 
		pNfc->pNfcCtrl->NAND.PageSize = 4096; 
		break;
    case 0x03: 
		pNfc->pNfcCtrl->NAND.PageSize = 8192; 
		break;
    }
	printf("PageSize: %d \n", pNfc->pNfcCtrl->NAND.PageSize);
	
    /* Spare Size*/
    switch(Id4 & 0x04)
	{
    case 0x00: 
		pNfc->pNfcCtrl->NAND.SpareSize = 32; 
		break;
    case 0x04: 
		pNfc->pNfcCtrl->NAND.SpareSize = 64; 
		break;
    }
	printf("SpareSize: %d \n", pNfc->pNfcCtrl->NAND.SpareSize);
	
    /* Block Size*/
    switch (Id4 & 0x30) 
	{
    case 0x00: 
		pNfc->pNfcCtrl->NAND.BlockSizeKB = 64;  
		break;
    case 0x10: 
		pNfc->pNfcCtrl->NAND.BlockSizeKB = 128; 
		break;
    case 0x20: 
		pNfc->pNfcCtrl->NAND.BlockSizeKB = 256; 
		break;
    case 0x30: 
		pNfc->pNfcCtrl->NAND.BlockSizeKB = 512; 
		break;
    }
	printf("BlockSizeKB: %d KB \n", pNfc->pNfcCtrl->NAND.BlockSizeKB);
	
    /* IO width*/
    switch (Id4 & 0x40)
	{
    case 0x00: 
		pNfc->pNfcCtrl->NAND.IoWidth = 8;  
		break;
    case 0x40: 
		pNfc->pNfcCtrl->NAND.IoWidth = 16; 
		break;
    }
	printf("IoWidth: %d bit \n\n", pNfc->pNfcCtrl->NAND.IoWidth);
	
    /* calculate other parameters*/
    pNfc->pNfcCtrl->NAND.PagePerBlock = (UINT16)(((UINT32)pNfc->pNfcCtrl->NAND.BlockSizeKB << 10) / pNfc->pNfcCtrl->NAND.PageSize);
    pNfc->pNfcCtrl->NAND.BlockPerLun = NAND_MAX_BLOCKS;
    pNfc->pNfcCtrl->NAND.LunNum = 0x1;
	
    pNfc->pNfcCtrl->NAND.BlockNum =  pNfc->pNfcCtrl->NAND.BlockPerLun * pNfc->pNfcCtrl->NAND.LunNum;
    pNfc->pNfcCtrl->NAND.PageNum = pNfc->pNfcCtrl->NAND.PagePerBlock * pNfc->pNfcCtrl->NAND.BlockPerLun * pNfc->pNfcCtrl->NAND.LunNum;
    pNfc->pNfcCtrl->NAND.DevSizeMB = (UINT16)((pNfc->pNfcCtrl->NAND.BlockSizeKB * pNfc->pNfcCtrl->NAND.BlockPerLun * pNfc->pNfcCtrl->NAND.LunNum) >> 10);

	pNfc->pNfcCtrl->NAND.RowAddrCycle = 3;
    pNfc->pNfcCtrl->NAND.ColAddrCycle = 2;
	
	printf("PagePerBlock: %d \n", pNfc->pNfcCtrl->NAND.PagePerBlock);
	printf("BlockNum: %d \n", pNfc->pNfcCtrl->NAND.BlockNum);
	printf("PageNum: %d \n", pNfc->pNfcCtrl->NAND.PageNum);
	printf("DeviceSizeInMegaBytes: %d MB \n", pNfc->pNfcCtrl->NAND.DevSizeMB);

	printf("\n");


	/**/
    /* Get Nandflash Id(check if support ONFI)*/
    /**/
    /*
    if (nfcFlash_Get_OnfiId(pNfc) ==  NANDFLASH_ONFI_ID)
	{
		//
        // device support onfi then read parameter page(256 bytes)
        //
        ret = nfcFlash_Get_ParamPage(pNfc, &ParamPage);
        if (ret == FMSH_FAILURE)
		{
            return FMSH_FAILURE;
        }
		
        // Initialize parameter
        pNfc->pNfcCtrl->NAND.Manufacture = ParamPage.JedecManufacturerId;    
        pNfc->pNfcCtrl->NAND.PageSize = ParamPage.BytesPerPage;
        pNfc->pNfcCtrl->NAND.SpareSize = ParamPage.SpareBytesPerPage;
        pNfc->pNfcCtrl->NAND.PagePerBlock = ParamPage.PagesPerBlock;
        pNfc->pNfcCtrl->NAND.BlockSizeKB = (ParamPage.PagesPerBlock * ParamPage.BytesPerPage)>>10;
        pNfc->pNfcCtrl->NAND.BlockPerLun = ParamPage.BlocksPerLun;
        pNfc->pNfcCtrl->NAND.DevSizeMB = (ParamPage.NumLuns * ParamPage.BlocksPerLun * pNfc->pNfcCtrl->NAND.BlockSizeKB)>>10;
        pNfc->pNfcCtrl->NAND.PageNum = ParamPage.PagesPerBlock * ParamPage.BlocksPerLun * ParamPage.NumLuns;
        pNfc->pNfcCtrl->NAND.BlockNum =  ParamPage.BlocksPerLun * ParamPage.NumLuns;
        pNfc->pNfcCtrl->NAND.LunNum = ParamPage.NumLuns;
        
        pNfc->pNfcCtrl->NAND.RowAddrCycle = ParamPage.AddrCycles & 0xf;
        pNfc->pNfcCtrl->NAND.ColAddrCycle = (ParamPage.AddrCycles >> 4) & 0xf;
    }
	*/
	
    pNfc->pNfcCtrl->busWidth = pNfc->pNfcCtrl->NAND.IoWidth;
    
    return FMSH_SUCCESS;
}

void nfc_id2(void)
{
	get_FlashID();
	return;
}

/*int g_init_flag_nfc2 = 0;*/

#if 0
void nfc_id2(void)
{
	int ret = 0;
	
    UINT32 first_word, second_word;
    UINT8 Id1, Id2, Id3, Id4, Id5;
	
	UINT32 flashId[2] = {0};
	
	vxT_NFC* pNfc = g_pNfc_S;
	vxT_NFC_CTRL * pNfcCtrl = NULL;

	vxT_NAND_INFO nand = {0};

#if 0
	/*extern int ps_init_2(void);*/
	/*ps_init_2();*/
	/**/
	/* setup nfc_MIO of slcr*/
	/**/
	#if 0
		nfcSlcr_Set_NfcMIO();
	#else
		nfcSlcr_Set_NfcMIO_2();
	#endif

	
    /*pNfc->config.baseAddress = FPS_NFC_S_BASEADDR;*/
	
    /* init nfc*/
    /*
    err = FNfcPs_Initialize(pNfc, FPS_NFC_S_DEVICE_ID);
    if(err) 
	{
        return FMSH_FAILURE;
    }
	*/	
	bzero((char*)(&g_pNfc_S->nfc_x), sizeof(vxT_NFC));	
	pNfc = g_pNfc_S;	
	
	bzero((char*)(&vxNfc_Ctrl_S.ctrl_x), sizeof(vxT_NFC_CTRL));
	pNfc->pNfcCtrl = (vxT_NFC_CTRL *)(&vxNfc_Ctrl_S.ctrl_x);
	
	pNfcCtrl = pNfc->pNfcCtrl;	
	
	pNfc->nfc_x = NFC_CTRL_0;	
	pNfcCtrl->ctrl_x = pNfc->nfc_x;
	pNfcCtrl->devId = pNfc->nfc_x;
	
	pNfcCtrl->cfgBaseAddr = VX_NFC_S_CFG_BASE;			
	pNfcCtrl->busWidth = FPS_NFC_S_FLASHWIDTH;

	
	/*taskDelay(10);*/
	
    /* reset nfc_ctrl*/
    /*FNfcPs_Reset(pNfc);	*/
	nfcSlcr_Rst_NfcCtrl(pNfc);
		
    /* set nfc_ctrl time_seq*/
	/*FNfcPs_SetTiming(pNfc);	*/
	nfcCtrl_Set_Timing(pNfc);
	
	/*FNfcPs_SetStatusMask(pNfc, 0x0140);	*/
	nfcCtrl_Wr_StatusMask(pNfc, 0x0140);
	
    /* Reset Nandflash chip*/
	/*
	err = Nandflash_Reset(pNfc);
    if(err){
        return FMSH_FAILURE;
    }
	*/	
	/**/
    /* Reset Nandflash chip*/
    /**/
    ret = nfcFlash_Rst_NandChip(pNfc);
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }	
#else	

	if (pNfc->init_flag == 0)
	{
		vxInit_Nfc();
	}
#endif
	
	/* read id*/
    ret = nfcFlash_Get_ChipId(pNfc, (UINT32*)(&flashId[0]));
    if (ret == FMSH_FAILURE)
	{
        return FMSH_FAILURE;
    }
	
	first_word  = flashId[0];
	second_word = flashId[1];

	printf("<vx> nand_id(MT29F1G16A 0x5580B12C-00000004): 0x%08X-%08X \n", first_word, second_word);
	
    /* calculate ID*/
    Id1 = first_word & 0xff;
    Id2 = (first_word >> 8) & 0xff;
    Id3 = (first_word >> 16) & 0xff;
    Id4 = (first_word >> 24) & 0xff;
    Id5 = second_word & 0xff;
    
    /*Set NandflashModel*/
    nand.Manufacture = Id1;
    nand.DevId = Id2;
    nand.Options = 0;

	printf("Manufacture: 0x%X \n", nand.Manufacture);
	printf("DeviceId: 0x%X \n\n", nand.DevId);
	
    /* Fetch from the extended ID4
    * ID4 D5  D4 BlockSize || D1  D0  PageSize
    *     0   0   64K      || 0   0   1K
    *     0   1   128K     || 0   1   2K
    *     1   0   256K     || 1   0   4K
    *     1   1   512K     || 1   1   8k */
    
    /* Page Size*/
    switch(Id4 & 0x03) 
	{
    case 0x00: 
		nand.PageSize = 1024; 
		break;
    case 0x01: 
		nand.PageSize = 2048; 
		break;
    case 0x02: 
		nand.PageSize = 4096; 
		break;
    case 0x03: 
		nand.PageSize = 8192; 
		break;
    }
	printf("PageSize: %d \n", nand.PageSize);
	
    /* Spare Size*/
    switch(Id4 & 0x04)
	{
    case 0x00: 
		nand.SpareSize = 32; 
		break;
    case 0x04: 
		nand.SpareSize = 64; 
		break;
    }
	printf("SpareSize: %d \n", nand.SpareSize);
	
    /* Block Size*/
    switch(Id4 & 0x30) 
	{
    case 0x00: 
		nand.BlockSizeKB = 64;  
		break;
    case 0x10: 
		nand.BlockSizeKB = 128; 
		break;
    case 0x20: 
		nand.BlockSizeKB = 256; 
		break;
    case 0x30: 
		nand.BlockSizeKB = 512; 
		break;
    }
	printf("BlockSizeKB: %d KB \n", nand.BlockSizeKB);
	
    /* IO width*/
    switch(Id4 & 0x40)
	{
    case 0x00: 
		nand.IoWidth = 8;  
		break;
    case 0x40: 
		nand.IoWidth = 16; 
		break;
    }
	printf("IoWidth: %d bit \n\n", nand.IoWidth);
	
    /* calculate other parameters*/
    nand.BlockPerLun = NAND_MAX_BLOCKS;
    nand.LunNum = 0x1;
    nand.PagePerBlock = (u16)(((u32)nand.BlockSizeKB << 10) / nand.PageSize);
    nand.BlockNum =  nand.BlockPerLun * nand.LunNum;
    nand.PageNum = nand.PagePerBlock * nand.BlockPerLun * nand.LunNum;
    nand.DevSizeMB = (u16)((nand.BlockSizeKB * nand.BlockPerLun * nand.LunNum) >> 10);
	
	printf("PagePerBlock: %d \n", nand.PagePerBlock);
	printf("BlockNum: %d \n", nand.BlockNum);
	printf("PageNum: %d \n", nand.PageNum);
	printf("DeviceSizeInMegaBytes: %d MB \n", nand.DevSizeMB);

	printf("\n");
	return;
}
#endif

void nfc_onfi_id2(void)
{
	UINT32 onfi_id = 0;	
	vxT_NFC* pNfc = g_pNfc_S;

	if (pNfc->init_flag == 0)
	{
		vxInit_Nfc();
	}

	onfi_id = nfcFlash_Get_OnfiId(pNfc);

	printf("<vx>onfi_id: 0x%08X \n", onfi_id);
	return;
}



int g_test_nfc2 = 0;

UINT8 Nfc_SndBuf2[2056] = {0};
UINT8 Nfc_RcvBuf2[2056] = {0};

void test_nfc_rd_page(int page_idx)
{
	vxT_NFC* pNfc = g_pNfc_S;
	vxT_NFC_CTRL * pNfcCtrl = NULL;
	
	int i = 0;
	
	int page_size = 0;
	UINT64 flashAddr =0;	

	if (pNfc->init_flag == 0)
	{
		vxInit_Nfc();
	}
	
	page_size = nandInfo_Get_PageSize(pNfc);
	flashAddr = (page_idx * page_size);
	
	printf("----devSize(%d)MB---- \n\n", nandInfo_Get_DevSize_MB(pNfc));
	
    /* read data*/
    if (FMSH_SUCCESS != nfcFlash_Read_MultiPage(pNfc, flashAddr, page_size, Nfc_RcvBuf2, 0))
	{
		printf("nfcFlash_Read_MultiPage fail->flashAddr: 0x%X \n", flashAddr);
		return;
	}

	printf("----page(%d)size(0x%X)---- \n", page_idx, page_size);
	for (i=0; i<256; i++)
	{
		printf("%02X", Nfc_RcvBuf2[i]);
		if ((i+1)%4 == 0)
		{
			printf(" ");
		}
		if ((i+1)%16 == 0)
		{
			printf("\n");
		}
	}
	
	printf(" ... ... ... \n");
	
	for (i=(page_size-256); i<page_size; i++)
	{
		printf("%02X", Nfc_RcvBuf2[i]);
		if ((i+1)%4 == 0)
		{
			printf(" ");
		}
		if ((i+1)%16 == 0)
		{
			printf("\n");
		}
	}	
	printf("------------------------ \n\n");

	return;
}

void test_nfc_erase_blk(int block_idx)
{
	vxT_NFC* pNfc = g_pNfc_S;
	
	if (pNfc->init_flag == 0)
	{
		vxInit_Nfc();
	}

	/* erase*/
	return nfcFlash_Erase_Block(pNfc, block_idx); 
}


void test_nfc_wr_page(int page_idx)
{
	vxT_NFC* pNfc = g_pNfc_S;
	
	int i = 0;
	
	int page_size = 0;
	UINT64 flashAddr = 0;
	
	if (pNfc->init_flag == 0)
	{
		vxInit_Nfc();
	}
	
	page_size = nandInfo_Get_PageSize(pNfc);	
	flashAddr = (page_idx * page_size);
	
    for (i=0; i<page_size; i++)
    {
        Nfc_SndBuf2[i] = g_test_nfc2 + i; 
        Nfc_RcvBuf2[i] = 0;
    }
	g_test_nfc2++;
	
    if (FMSH_SUCCESS != nfcFlash_Write_MultiPage(pNfc, flashAddr, page_size, Nfc_SndBuf2, 0))
	{
		printf("nfcFlash_Write_MultiPage Fail! page(%d)->flashAddr:0x%X, pageSize:0x%X \n\n", page_idx, flashAddr, page_size);
		return;
	}
	else
	{
		printf("nfcFlash_Write_MultiPage OK! page(%d)->flashAddr:0x%X, pageSize:%d \n\n", page_idx, flashAddr, page_size);
	}
	
	return;
}

#endif




