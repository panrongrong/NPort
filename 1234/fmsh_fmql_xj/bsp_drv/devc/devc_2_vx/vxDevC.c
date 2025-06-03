/* vxDevC.c - fmsh 7010/7045 DevC driver */

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
01a, 02Jan20, jc  written.
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

#include "vxDevC.h"

/*
defines 
*/
#if 1

#undef VX_DBG
#undef VX_DBG2

#define DEVC_DBG

#ifdef DEVC_DBG

#define VX_DBG(string, a, b, c, d, e, f)                    \
        if (_func_logMsg != NULL)                           \
           printf(string, a, b, c, d, e, f)  /* (* _func_logMsg)(string, a, b, c, d, e, f) */
#else
#define VX_DBG(string, a, b, c, d, e, f)
#endif

/* ret info log */
#define VX_DBG2(string, a, b, c, d, e, f) printf(string, a, b, c, d, e, f)
#endif


extern void sysMsDelay(int);
/* 
pDevC: 
*/
static vxT_DEVC_CTRL vxDevC_Ctrl = {0};
static vxT_DEVC vx_pDevC = {0};
vxT_DEVC * g_pDevC = (vxT_DEVC *)(&vx_pDevC.devc_x);


#if 1


void devcCtrl_Wr_CfgReg32(vxT_DEVC* pDevC, UINT32 offset, UINT32 value)
{
	UINT32 tmp32 = pDevC->pDevcCtrl->cfgBaseAddr;
	FMQL_WRITE_32((tmp32 + offset), value);
	return;
}

UINT32 devcCtrl_Rd_CfgReg32(vxT_DEVC* pDevC, UINT32 offset)
{
	UINT32 tmp32 = pDevC->pDevcCtrl->cfgBaseAddr;
	return FMQL_READ_32(tmp32 + offset);
}


/******************************************************************************
*
* This function pull down Prog_B, then pull up
*
* @param	dev is devc handle.
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
int devcCtrl_Prog_B(vxT_DEVC* pDevC)
{
    UINT32 tmp32 = 0;   

	/**/
    /* Setting PCFG_PROG_B signal to high*/
    /*	*/
	tmp32 = devcCtrl_Rd_CfgReg32(pDevC, DEVC_REG_SAC_CFG);	
	/*if ((tmp32 & SAC_CFG_PROGRAM_B_MASK) == 0x00)*/
	{
		tmp32 |= SAC_CFG_PROGRAM_B_MASK;
		devcCtrl_Wr_CfgReg32(pDevC, DEVC_REG_SAC_CFG, tmp32);
	}
    taskDelay(1);
	
	/**/
    /* Setting PCFG_PROG_B signal to low*/
    /**/
	tmp32 = devcCtrl_Rd_CfgReg32(pDevC, DEVC_REG_SAC_CFG);	
	/*if ((tmp32 & SAC_CFG_PROGRAM_B_MASK) != 0x00)*/
	{
		tmp32 &= (~SAC_CFG_PROGRAM_B_MASK);
		devcCtrl_Wr_CfgReg32(pDevC, DEVC_REG_SAC_CFG, tmp32);
	}
    taskDelay(1);

    /**/
    /* Polling the PCAP_INIT sts for Reset  */
    /**/
	do
	{
		tmp32 = devcCtrl_Rd_CfgReg32(pDevC, DEVC_REG_SAC_STATUS);	
		
		printf("low->sac_status: 0x%X \n", tmp32);
	} while ((tmp32 & SAC_STATUS_PCFG_INIT_MASK) != 0x00);

	/**/
    /* Setting PCFG_PROG_B signal to high*/
    /**/
	tmp32 = devcCtrl_Rd_CfgReg32(pDevC, DEVC_REG_SAC_CFG);	
	/*if ((tmp32 & SAC_CFG_PROGRAM_B_MASK) == 0x00)*/
	{
		tmp32 |= SAC_CFG_PROGRAM_B_MASK;
		devcCtrl_Wr_CfgReg32(pDevC, DEVC_REG_SAC_CFG, tmp32);
	}
    taskDelay(1);

	/*    */
    /* Polling the PCAP_INIT sts for set */
    /**/
	do
	{
		tmp32 = devcCtrl_Rd_CfgReg32(pDevC, DEVC_REG_SAC_STATUS);	
		
		printf("high->sac_status: 0x%X \n", tmp32);
	} while ((tmp32 & SAC_STATUS_PCFG_INIT_MASK) == 0x00);
	

    return FMSH_SUCCESS;
}

int devcCtrl_Clr_PL_BitStream(vxT_DEVC* pDevC)
{
	int ret = devcCtrl_Prog_B(pDevC);
	return ret;
}

void devcCtrl_Set_Reg32(vxT_DEVC* pDevC, 
                                     UINT32 reg_offset, int Bit_Shift, 
                                     UINT32 mask_xbit, UINT32 set_val)
{
	UINT32 tmp32 = 0, tmp32_2 = 0, MASK = 0;

	tmp32 = devcCtrl_Rd_CfgReg32(pDevC, reg_offset);	
	
	MASK = mask_xbit << Bit_Shift;
	tmp32_2 = (tmp32 & MASK) >> Bit_Shift;
	
    if (tmp32_2 != set_val)
	{
		tmp32 &= (~MASK);
		tmp32 |= (set_val << Bit_Shift) & MASK;
		
		devcCtrl_Wr_CfgReg32(pDevC, reg_offset, tmp32);
	}

	return;
}

/*****************************************************************************/
/**
* This function sets read FIFO threshold
*
* @param	pDevC is devc handle.
* @param	mode
*       readFifoThre_hex_0x40 = 0x0,//default
*       readFifoThre_hex_0x80 = 0x1,
*       readFifoThre_hex_0xc0 = 0x2,
*       readFifoThre_hex_0x100 = 0x3
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
****************************************************************************/
int devcCtrl_Set_ReadFifoThrsh(vxT_DEVC* pDevC, UINT32 readFifoThrsh)
{ 
    UINT32 reg_offset = 0, mask_xbit = 0, set_val = 0; 
	int BIT_SHIFT = 0;

	set_val = readFifoThrsh;
	reg_offset = DEVC_REG_SAC_CFG;
	BIT_SHIFT = SAC_CFG_RFIFO_TH_SHIFT;  /* RFIFO_TH  9:8*/
	mask_xbit = MASK_2BIT;
	
	devcCtrl_Set_Reg32(pDevC, reg_offset, BIT_SHIFT, mask_xbit, set_val);
	
    return FMSH_SUCCESS;
}

/******************************************************************************
*
* This function sets write FIFO threshold
*
* @param	dev is devc handle.
* @param	mode
*       writeFifoThre_hex_0x80 = 0x0,//default
*       writeFifoThre_hex_0x60 = 0x1,
*       writeFifoThre_hex_0x40 = 0x2,
*       writeFifoThre_hex_0x10 = 0x3
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
int devcCtrl_Set_WriteFifoThrsh(vxT_DEVC* pDevC, UINT32 writeFifoThrsh)
{  
    UINT32 reg_offset = 0, mask_xbit = 0, set_val = 0; 
	int BIT_SHIFT = 0;

	set_val = writeFifoThrsh;
	reg_offset = DEVC_REG_SAC_CFG;
	BIT_SHIFT = SAC_CFG_WFIFO_TH_SHIFT;  /* WFIFO_TH  7:6	*/
	mask_xbit = MASK_2BIT;
	
	devcCtrl_Set_Reg32(pDevC, reg_offset, BIT_SHIFT, mask_xbit, set_val);
	
    return FMSH_SUCCESS;
}


/******************************************************************************
*
* This function set smap32_swap_ctrl
.
*
* @param	dev is devc handle.
* @param	mode
*        smap32_swap_disable = 0x0,
*        smap32_swap_enable = 0x1  
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
int devcCtrl_Set_Smap32_SwapCtrl(vxT_DEVC* pDevC, UINT32 smap32_swap)
{  
    UINT32 reg_offset = 0, mask_xbit = 0, set_val = 0; 
	int BIT_SHIFT = 0;

	set_val = smap32_swap;
	reg_offset = DEVC_REG_SAC_CFG;
	BIT_SHIFT = SAC_CFG_SMAP32_SWAP_CTRL_SHIFT;
	mask_xbit = MASK_1BIT;
	
	devcCtrl_Set_Reg32(pDevC, reg_offset, BIT_SHIFT, mask_xbit, set_val);
	
    return FMSH_SUCCESS;
}

/******************************************************************************
*
* This function sets read clock edge
*
* @param	dev is devc handle.
* @param	mode
*       rising_edge = 0x1,
*       failing_edge = 0x0   
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
int devcCtrl_Set_RdClk_Edge(vxT_DEVC* pDevC, UINT32 clk_edge)
{  
    UINT32 reg_offset = 0, mask_xbit = 0, set_val = 0; 
	int BIT_SHIFT = 0;

	set_val = clk_edge;
	reg_offset = DEVC_REG_SAC_CFG;
	BIT_SHIFT = SAC_CFG_RCLK_EDGE_SHIFT;
	mask_xbit = MASK_1BIT;
	
	devcCtrl_Set_Reg32(pDevC, reg_offset, BIT_SHIFT, mask_xbit, set_val);
	
    return FMSH_SUCCESS;	
}

/******************************************************************************
*
* This function sets write clock edge
*
* @param	dev is devc handle.
* @param	mode
*       rising_edge = 0x1,
*       failing_edge = 0x0   
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
int devcCtrl_Set_WrClk_Edge(vxT_DEVC* pDevC, UINT32 clk_edge)
{  
    UINT32 reg_offset = 0, mask_xbit = 0, set_val = 0; 
	int BIT_SHIFT = 0;

	set_val = clk_edge;
	reg_offset = DEVC_REG_SAC_CFG;
	BIT_SHIFT = SAC_CFG_WCLK_EDGE_SHIFT;
	mask_xbit = MASK_1BIT;
	
	devcCtrl_Set_Reg32(pDevC, reg_offset, BIT_SHIFT, mask_xbit, set_val);
	
    return FMSH_SUCCESS;
}

/******************************************************************************
*
* This function pull high CSI_B
*
* @param	dev is devc handle.
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
int devcCtrl_SetLow_CSI_B(vxT_DEVC* pDevC)
{
    UINT32 reg_offset = 0, mask_xbit = 0, set_val = 0; 
	int BIT_SHIFT = 0;

	set_val = LOW_LVL;
	reg_offset = DEVC_REG_SAC_CFG;
	BIT_SHIFT = SAC_CFG_CSI_B_SHIFT;
	mask_xbit = MASK_1BIT;
	
	devcCtrl_Set_Reg32(pDevC, reg_offset, BIT_SHIFT, mask_xbit, set_val);
	
    return FMSH_SUCCESS;
}

/******************************************************************************
*
* This function pull high CSI_B
*
* @param	dev is devc handle.
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
int devcCtrl_SetHi_CSI_B(vxT_DEVC* pDevC)
{
    UINT32 reg_offset = 0, mask_xbit = 0, set_val = 0; 
	int BIT_SHIFT = 0;

	set_val = HIGH_LVL;
	reg_offset = DEVC_REG_SAC_CFG;
	BIT_SHIFT = SAC_CFG_CSI_B_SHIFT;
	mask_xbit = MASK_1BIT;
	
	devcCtrl_Set_Reg32(pDevC, reg_offset, BIT_SHIFT, mask_xbit, set_val);
	
    return FMSH_SUCCESS;
}

/******************************************************************************
*
* This function pull low RDWR_B
*
* @param	dev is devc handle.
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
int devcCtrl_SetLow_RDWR_B(vxT_DEVC* pDevC)
{  
    UINT32 reg_offset = 0, mask_xbit = 0, set_val = 0; 
	int BIT_SHIFT = 0;

	set_val = LOW_LVL;
	reg_offset = DEVC_REG_SAC_CFG;
	BIT_SHIFT = SAC_CFG_RDWR_B_SHIFT;
	mask_xbit = MASK_1BIT;
	
	devcCtrl_Set_Reg32(pDevC, reg_offset, BIT_SHIFT, mask_xbit, set_val);
	
    return FMSH_SUCCESS;
}


/******************************************************************************
*
* This function pull high RDWR_B
*
* @param	dev is devc handle.
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
int devcCtrl_SetHi_RDWR_B(vxT_DEVC* pDevC)
{  
    /*Setting PCFG_RDWR_B signal to HIGH*/
    UINT32 reg_offset = 0, mask_xbit = 0, set_val = 0; 
	int BIT_SHIFT = 0;

	set_val = HIGH_LVL;
	reg_offset = DEVC_REG_SAC_CFG;
	BIT_SHIFT = SAC_CFG_RDWR_B_SHIFT;
	mask_xbit = MASK_1BIT;
	
	devcCtrl_Set_Reg32(pDevC, reg_offset, BIT_SHIFT, mask_xbit, set_val);
	
    return FMSH_SUCCESS;
}

/******************************************************************************
*
* This function sets rx data byte swap.
*
* @param	dev is devc handle.
* @param	mode
*       none_swap = 0x0,
*       half_word_swap = 0x1,   
*       byte_swap = 0x2,
*       bit_swap = 0x3  
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
int devcCtrl_Set_RxDataSwap(vxT_DEVC* pDevC, UINT32 data_swap)
{  
    UINT32 reg_offset = 0, mask_xbit = 0, set_val = 0; 
	int BIT_SHIFT = 0;

	set_val = data_swap;
	reg_offset = DEVC_REG_SAC_CFG;
	BIT_SHIFT = SAC_CFG_RXFIFO_DATA_SWAP_SHIFT;  /* rxfifo_di_data_swap 18:17*/
	mask_xbit = MASK_2BIT;
	
	devcCtrl_Set_Reg32(pDevC, reg_offset, BIT_SHIFT, mask_xbit, set_val);
	
    return FMSH_SUCCESS;
}


/******************************************************************************
*
* This function sets tx data byte swap.
*
* @param	dev is devc handle.
* @param	mode
*       none_swap = 0x0,
*       half_word_swap = 0x1,   
*       byte_swap = 0x2,
*       bit_swap = 0x3  
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
int devcCtrl_Set_TxDataSwap(vxT_DEVC* pDevC, UINT32 data_swap)
{  
    UINT32 reg_offset = 0, mask_xbit = 0, set_val = 0; 
	int BIT_SHIFT = 0;

	set_val = data_swap;
	reg_offset = DEVC_REG_SAC_CFG;
	BIT_SHIFT = SAC_CFG_TXFIFO_DATA_SWAP_SHIFT;  /* txfifo_do_data_swap 25:24*/
	mask_xbit = MASK_2BIT;
	
	devcCtrl_Set_Reg32(pDevC, reg_offset, BIT_SHIFT, mask_xbit, set_val);
	
    return FMSH_SUCCESS;
}

/******************************************************************************
*
* This function sets readback dummmy counter, that is where data is sampled 
*
* @param	dev is devc handle.
* @param	mode
*        smap32_swap_disable = 0x0,
*        smap32_swap_enable = 0x1  
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
int devcCtrl_Set_DummyCnt(vxT_DEVC* pDevC, UINT32 dummy_num)
{  
    UINT32 reg_offset = 0, mask_xbit = 0, set_val = 0; 
	int BIT_SHIFT = 0;

	set_val = dummy_num;
	reg_offset = DEVC_REG_SAC_CFG;
	BIT_SHIFT = SAC_CFG_RDBACK_DUMMY_NUM_SHIFT;  /* READBACK_DUMMY_NUM	15:13*/
	mask_xbit = MASK_3BIT;
	
	devcCtrl_Set_Reg32(pDevC, reg_offset, BIT_SHIFT, mask_xbit, set_val);
	
    return FMSH_SUCCESS;
}



/******************************************************************************
*
* This function sets bitstream download mode.
*
* @param	dev is devc handle.
* @param	mode
*       DOWNLOAD_BITSTREAM = 0x0,
*       READBACK_BITSTREAM = 0x2, 
*       DATA_LOOPBACK = 0x3,     
*       SECURE_DOWNLOAD_BITSTREAM = 0x08  
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
int devcCtrl_Set_PL_DwnLdMode(vxT_DEVC* pDevC, UINT32 dma_mode)
{  
    UINT32 reg_offset = 0, mask_xbit = 0, set_val = 0; 
	int BIT_SHIFT = 0;

	set_val = dma_mode;
	reg_offset = DEVC_REG_SAC_CFG;
	BIT_SHIFT = SAC_CFG_CSU_DMA_SWITCH_SHIFT;  /* CSU_DMA_SWITCH 23:20*/
	mask_xbit = MASK_4BIT;
	
	devcCtrl_Set_Reg32(pDevC, reg_offset, BIT_SHIFT, mask_xbit, set_val);
	
    return FMSH_SUCCESS;
}

/******************************************************************************
*
*
* This function clears the specified interrupts in the Interrupt Status
* Register.
*
* @param	dev is devc handle.
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None.
*
*****************************************************************************/
int devcCtrl_Clr_PcapStatus(vxT_DEVC* pDevC)
{  
	UINT32 tmp32 = 0;
	
	/**/
    /* Clear it all, so if Boot ROM comes back, it can proceed*/
    /* write 0xffffffff to INT_STS*/
    /**/
	devcCtrl_Wr_CfgReg32(pDevC, DEVC_REG_SAC_INT_STS, 0xFFFFFFFF);

	/**/
    /* Get PCAP Interrupt Status Register*/
    /**/
	tmp32 = devcCtrl_Rd_CfgReg32(pDevC, DEVC_REG_SAC_INT_STS);
	if (tmp32 & SAC_INT_STS_ERR_FLAG_MASK)
	{
		printf("SAC_INT_STS_ERR_FLAG_MASK fail! \n");
		return FMSH_FAILURE;
	}
	
	/**/
    /* Read the PCAP sts register for DMA sts*/
    /**/
	tmp32 = devcCtrl_Rd_CfgReg32(pDevC, DEVC_REG_SAC_STATUS);
	if (tmp32 & SAC_STATUS_DMA_BUSY_MASK)
	{
		printf("SAC_STATUS_DMA_BUSY_MASK fail! \n");
		return FMSH_FAILURE;
	}
	
    return FMSH_SUCCESS;  
}

/******************************************************************************
*
*
* This function polls DMA done
*
* @param	dev is devc handle.
* @param	maxcount is the max poll times
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None.
*
******************************************************************************/
int devcCtrl_Poll_DmaDone(vxT_DEVC* pDevC, UINT32 max_cnt)
{
	UINT32 tmp32 = 0;
	int cnt = 0;
	UINT32 MASK = 0;
	
	MASK = 0x01 << SAC_INT_STS_DMA_DONE_SHIFT;

	cnt = max_cnt;
	do 
	{
		tmp32 = devcCtrl_Rd_CfgReg32(pDevC, DEVC_REG_SAC_INT_STS);
		if (tmp32 & MASK)
		{
			break;  /* == 1*/
		}
		else if (tmp32 & SAC_INT_STS_ERR_FLAG_MASK)
        {
			printf("devcCtrl_Poll_DmaDone fail: status err! \n");
        	return FMSH_FAILURE;
        }
		
		cnt--;		
	} while (cnt > 0);

	if (cnt <= 0)
	{
		printf("devcCtrl_Poll_DmaDone fail: timeout! \n");
		return FMSH_FAILURE;
	}
	
	/**/
	/* clear dma done int status.*/
	/**/
	tmp32 = 0;  /* devcCtrl_Rd_CfgReg32(pDevC, DEVC_REG_SAC_INT_STS);*/
	tmp32 |= (0x01 << SAC_INT_STS_DMA_DONE_SHIFT);
    devcCtrl_Wr_CfgReg32(pDevC, DEVC_REG_SAC_INT_STS, tmp32);
	
    return FMSH_SUCCESS;
}

/******************************************************************************
*
*
* This function polls D_P done
*
* @param	dev is devc handle.
* @param	maxcount is the max poll times
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None.
*
******************************************************************************/
int devcCtrl_Poll_DmaPcap_Done(vxT_DEVC* pDevC, UINT32 max_cnt)
{
	UINT32 tmp32 = 0;
	int cnt = 0;
	UINT32 MASK = 0;
	
	MASK = 0x01 << SAC_INT_STS_DMA_PCAP_DONE_SHIFT;

	cnt = max_cnt;
	do 
	{
		tmp32 = devcCtrl_Rd_CfgReg32(pDevC, DEVC_REG_SAC_INT_STS);
		
		/*if (tmp32 & SAC_INT_STS_DMA_PCAP_DONE_MASK)*/
		if (tmp32 & MASK)
		{
			break;
		}		
		else if (tmp32 & SAC_INT_STS_ERR_FLAG_MASK)
        {
			printf("devcCtrl_Poll_DmaPcap_Done fail: status err! \n");
        	return FMSH_FAILURE;
        }
		
		cnt--;		
	} while (cnt > 0);
	
	if (cnt <= 0)
	{
		printf("devcCtrl_Poll_DmaPcap_Done fail: timeout! \n");
		return FMSH_FAILURE;
	}

	tmp32 = 0;  /* devcCtrl_Rd_CfgReg32(pDevC, DEVC_REG_SAC_INT_STS);*/
	tmp32 |= (0x01 << SAC_INT_STS_DMA_PCAP_DONE_SHIFT);
    devcCtrl_Wr_CfgReg32(pDevC, DEVC_REG_SAC_INT_STS, tmp32);

    return FMSH_SUCCESS;
}


/******************************************************************************/
/**
*
* This function initiates the DMA transfer.
*
* @param	devcDev is devc handle.
* @param	InstancePtr is a pointer to the XDcfg instance.
* @param	srcAddr contains a pointer to the source memory where the data
*		is to be transferred from.
* @param	srcWdLen is the number of words (32 bit) to be transferred
*		for the source transfer.
* @param	dstAddr contains a pointer to the destination memory
*		where the data is to be transferred to.
* @param	dstWdLen is the number of words (32 bit) to be transferred
*		for the Destination transfer.
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None.
*
****************************************************************************/
int devcCtrl_Init_PcapDma(vxT_DEVC* pDevC, UINT32 srcAddr, UINT32 dstAddr, UINT32 srcWdLen, UINT32 dstWdLen)
{
	int ret = 0;
    
	devcCtrl_Wr_CfgReg32(pDevC, DEVC_REG_CSU_DMA_SRC_ADR, srcAddr);
	devcCtrl_Wr_CfgReg32(pDevC, DEVC_REG_CSU_DMA_DST_ADR, dstAddr);
	
	devcCtrl_Wr_CfgReg32(pDevC, DEVC_REG_CSU_DMA_SRC_LEN, srcWdLen);
	devcCtrl_Wr_CfgReg32(pDevC, DEVC_REG_CSU_DMA_DST_LEN, dstWdLen);

	/**/
    /* Poll for the DMA done*/
    /**/
    ret = devcCtrl_Poll_DmaDone(pDevC, DEVC_MAX_COUNT);
	if (ret == FMSH_SUCCESS)
	{
		printf("devcCtrl_Init_PcapDma: devcCtrl_Poll_DmaDone OK \n");
	}
	else
	{
		printf("devcCtrl_Init_PcapDma: devcCtrl_Poll_DmaDone fail \n");
	}

	/**/
    /* Poll for the Dma_Pcap done*/
    /**/
    ret = devcCtrl_Poll_DmaPcap_Done(pDevC, DEVC_MAX_COUNT);
	if (ret == FMSH_SUCCESS)
	{
		printf("devcCtrl_Init_PcapDma: devcCtrl_Poll_DmaPcap_Done OK \n");
	}
	else
	{
		printf("devcCtrl_Init_PcapDma: devcCtrl_Poll_DmaPcap_Done fail \n");
	}    
    
    return FMSH_SUCCESS;
}

/******************************************************************************
*
*
* This function initiates the DMA transfer for readback.
*
* @param	dev is devc handle.
* @param	InstancePtr is a pointer to the XDcfg instance.
* @param	SourcePtr contains a pointer to the source memory where the data
*		is to be transferred from.
* @param	SrcWordLength is the number of words (32 bit) to be transferred
*		for the source transfer.
* @param	DestPtr contains a pointer to the destination memory
*		where the data is to be transferred to.
* @param	DestWordLength is the number of words (32 bit) to be transferred
*		for the Destination transfer.
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None.
*
******************************************************************************/
int devcCtrl_Init_PcapDma_RdBack(vxT_DEVC* pDevC, UINT32 srcAddr, UINT32 dstAddr, UINT32 srcWdLen, UINT32 dstWdLen)
{
	int ret = 0;
	
	devcCtrl_Wr_CfgReg32(pDevC, DEVC_REG_CSU_DMA_SRC_ADR, srcAddr);
	devcCtrl_Wr_CfgReg32(pDevC, DEVC_REG_CSU_DMA_DST_ADR, dstAddr);
	
	devcCtrl_Wr_CfgReg32(pDevC, DEVC_REG_CSU_DMA_SRC_LEN, srcWdLen);
	devcCtrl_Wr_CfgReg32(pDevC, DEVC_REG_CSU_DMA_DST_LEN, dstWdLen);

	devcCtrl_SetHi_RDWR_B(pDevC);  /* rdwr_b -> high*/
	devcCtrl_SetLow_CSI_B(pDevC);  /* csi_b  -> low*/

	sysMsDelay(50);
	
	/**/
    /* Poll for the DMA done*/
    /**/
    ret = devcCtrl_Poll_DmaDone(pDevC, DEVC_MAX_COUNT);
	if (ret == FMSH_SUCCESS)
	{
		printf("devcCtrl_Init_PcapDma_RdBack: devcCtrl_Poll_DmaDone OK \n");
	}
	else
	{
		printf("devcCtrl_Init_PcapDma_RdBack: devcCtrl_Poll_DmaDone fail \n");
	}

	/**/
    /* Poll for the Dma_Pcap done*/
    /**/
    ret = devcCtrl_Poll_DmaPcap_Done(pDevC, DEVC_MAX_COUNT);
	if (ret == FMSH_SUCCESS)
	{
		printf("devcCtrl_Init_PcapDma_RdBack: devcCtrl_Poll_DmaPcap_Done OK \n");
	}
	else
	{
		printf("devcCtrl_Init_PcapDma_RdBack: devcCtrl_Poll_DmaPcap_Done fail \n");
	}    
	
    return FMSH_SUCCESS;
}



int devcCtrl_Xfer_Pcap(vxT_DEVC* pDevC,
		                        UINT32 srcAddr, UINT32 dstAddr,
		                        UINT32 srcWdLen, UINT32 dstWdLen,
		                        UINT32 xferType)
{ 
    UINT32 tmp32 = 0;
	
	/**/
    /* Read the PCAP sts register for DMA sts*/
    /* check if DMA command queue is full*/
    /**/
	tmp32 = devcCtrl_Rd_CfgReg32(pDevC, DEVC_REG_SAC_STATUS);
	if (tmp32 & SAC_STATUS_DMA_BUSY_MASK)
	{
		printf("SAC_STATUS_DMA_BUSY_MASK:0x%08X fail! \n", tmp32);
		return FMSH_FAILURE;
	}
	
	/**/
    /* Check whether the fabric is in initialized state*/
    /**/
	tmp32 = devcCtrl_Rd_CfgReg32(pDevC, DEVC_REG_SAC_STATUS);
	if ((tmp32 & SAC_STATUS_PCFG_INIT_MASK) == 0x00)
	{
		printf("SAC_STATUS_PCFG_INIT_MASK:0x%08X fail! \n", tmp32);
		return FMSH_FAILURE;
	}
        
	/* Check for valid source pointer and length*/
	if ((!srcAddr) || (srcWdLen == 0))
	{		 
		printf("srcAddr:0x%X srcWdLen:0x%X fail! \n", srcAddr, srcWdLen);
		return FMSH_FAILURE;
	}
	
    /* We don't need to check PCFG_INIT to be high for non-encrypted loopback transfers*/
    switch (xferType)
    {
	case FMSH_SECURE_PCAP_WRITE:
	case FMSH_NON_SECURE_PCAP_WRITE: 
	    {  
	        switch (xferType) 
			{
			case FMSH_NON_SECURE_PCAP_WRITE:
				devcCtrl_Set_PL_DwnLdMode(pDevC, DMA_MODE_DWNLD_BITSTREAM);   /* set the mode:download bitstream*/
				break;

	        case FMSH_SECURE_PCAP_WRITE:      
	            devcCtrl_Set_PL_DwnLdMode(pDevC, DMA_MODE_SECUR_DWNLD_BITSTREAM);  /* set sec_down mode*/
				break;
	        }

	        devcCtrl_SetLow_RDWR_B(pDevC);
	        devcCtrl_SetLow_CSI_B(pDevC);

			printf("go to pcapDma init1: ");
	        devcCtrl_Init_PcapDma(pDevC, srcAddr, dstAddr, srcWdLen, dstWdLen);
	    }
		break;
	
    case FMSH_PCAP_READBACK:
	    {
	    	/**/
	        /* Send READ Frame command to FPGA*/
	        /* set the mode:download */
	        /**/
	        devcCtrl_Set_PL_DwnLdMode(pDevC, DMA_MODE_DWNLD_BITSTREAM);/*set the mode:download bitstream*/
	        devcCtrl_SetLow_RDWR_B(pDevC);  /* csi_b -> low*/
	        
	        devcCtrl_SetLow_CSI_B(pDevC);   /* rdwr_b -> low	        */
			printf("go to pcapDma init2: ");
	        devcCtrl_Init_PcapDma(pDevC, (UINT32)srcAddr, PCAP_RD_DATA_ADDR, srcWdLen, 0);	        
	        devcCtrl_SetHi_CSI_B(pDevC);    /* rdwr_b -> high*/

			/**/
	        /* Initiate the DMA write command.*/
	        /**/
	        devcCtrl_Set_PL_DwnLdMode(pDevC, DMA_MODE_RDBACK_BITSTREAM);  /* set the mode:readback bitstream mode*/
	        devcCtrl_Init_PcapDma_RdBack(pDevC, PCAP_WR_DATA_ADDR, (UINT32)dstAddr, 0, dstWdLen);
	    }
		break;
	
    case FMSH_NON_SECURE_PCAP_WRITE_DUMMMY:
	    {
	        devcCtrl_Set_PL_DwnLdMode(pDevC, DMA_MODE_DWNLD_BITSTREAM);/*set the mode:download bitstream*/
			
	        devcCtrl_SetHi_CSI_B(pDevC);   /* csi_b -> high*/
	        devcCtrl_SetLow_RDWR_B(pDevC); /* rdwr_b -> low*/
			
			printf("go to pcapDma init3: ");
	        devcCtrl_Init_PcapDma(pDevC, srcAddr, dstAddr, srcWdLen, dstWdLen);
	    }
		break;
    }

    return FMSH_SUCCESS;
}

/******************************************************************************
*
* This function unlocks CSU module
*
* @param	dev is devc handle.
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
int devcCtrl_UnLock_CSU(vxT_DEVC* pDevC)
{ 
    devcCtrl_Wr_CfgReg32(pDevC, DEVC_REG_CSU_UNLOCK, 0x757bdf0d);
	
    return FMSH_SUCCESS;
}

/******************************************************************************
*
* This function locks CSU module
*
* @param	dev is devc handle.
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
******************************************************************************/
int devcCtrl_Lock_CSU(vxT_DEVC* pDevC)
{   
    devcCtrl_Wr_CfgReg32(pDevC, DEVC_REG_SAC_LOCK, 0xdf0d767b);
    
    return FMSH_SUCCESS;
}

/*****************************************************************************/
/**
* This function initializes Device Configuration module, configure pcap interface work parameters
*
* @param	xferType.
*		- FMSH_PCAP_READBACK
*		- FMSH_NON_SECURE_PCAP_WRITE
*		- FMSH_PCAP_LOOPBACK
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
****************************************************************************/
int devcCtrl_Init_Pcap(vxT_DEVC* pDevC, UINT32 xferType)
{        
 	int ret = FMSH_SUCCESS;
	UINT32 tmp32 = 0;
	
	/**/
	/* UnLock CSU*/
	/**/
	devcCtrl_UnLock_CSU(pDevC);

	tmp32 =devcCtrl_Rd_CfgReg32(pDevC, DEVC_REG_SAC_CFG);
	printf("DEVC_REG_SAC_CFG: 0x%08X \n");
		
	devcCtrl_Set_Smap32_SwapCtrl(pDevC, SWAP32_SWAP_EN);
	
	devcCtrl_Set_RdClk_Edge(pDevC, FALLING_EDGE);
	devcCtrl_Set_WrClk_Edge(pDevC, FALLING_EDGE);

	devcCtrl_SetHi_CSI_B(pDevC);
	devcCtrl_SetHi_RDWR_B(pDevC);

	switch (xferType)
	{
    case FMSH_PCAP_READBACK: 
	    {
	        devcCtrl_Set_RxDataSwap(pDevC, DATA_SWAP_BYTE);
	        devcCtrl_Set_TxDataSwap(pDevC, DATA_SWAP_BYTE); 

			/**/
	        /* default 3 , for FMSH 325t the value must be set 4,*/
	        /* for fmsh 325t the value must be set 3 for readback reg, but the value must be set 4 for readback bitstream*/
	        /**/
			devcCtrl_Set_DummyCnt(pDevC, DUMMY_NUM_4);
	        
	        devcCtrl_Set_PL_DwnLdMode(pDevC, DMA_MODE_DWNLD_BITSTREAM);
			
			devcCtrl_Set_ReadFifoThrsh(pDevC, READFIFO_THRSH_HEX_40);
	    } 
		break;
	
    case FMSH_NON_SECURE_PCAP_WRITE:
	    {
	        /**/
			/* bit9-鏉ヨ嚜PL_CFG妯″潡鐨刢fg_done淇″彿*/
			/**/
	        if (devcCtrl_Rd_CfgReg32(pDevC, DEVC_REG_SAC_STATUS) & 0x200)  
	        {   
				printf("FMSH_NON_SECURE_PCAP_WRITE: FDevcPs_Prog_B \n");
	        	devcCtrl_Prog_B(pDevC);
	        }
			
	        devcCtrl_Set_RxDataSwap(pDevC, DATA_SWAP_BYTE);
			
	        devcCtrl_Set_PL_DwnLdMode(pDevC, DMA_MODE_DWNLD_BITSTREAM);
			
			devcCtrl_Set_WriteFifoThrsh(pDevC, WRITEFIFO_THRSH_HEX_80);
	    }
		break;
	
    case FMSH_PCAP_LOOPBACK: 
	    {
			devcCtrl_Set_RxDataSwap(pDevC, DATA_SWAP_BYTE);
			devcCtrl_Set_TxDataSwap(pDevC, DATA_SWAP_BYTE);
			
			devcCtrl_Set_DummyCnt(pDevC, DUMMY_NUM_4);
			devcCtrl_Set_PL_DwnLdMode(pDevC, DMA_MODE_DATA_LOOPBACK);			
	    }
		break;
	
    default:
		printf("xferType: 0x%X fail! \n", xferType);
        ret = FMSH_FAILURE;
		break;
	}

    return ret;
}


/******************************************************************************/
/**
*
* This function polls FPGA done
*
* @param	pDevC is devc handle.
* @param	maxcount is the max poll times
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None.
*
****************************************************************************/
int devcCtrl_Poll_FpgaDone(vxT_DEVC* pDevC, UINT32 max_cnt)
{
    UINT32 tmp32 = 0, cnt = 0;
    int i = 0;
	UINT32 Dummy[16]={0};
	
	/*cnt = max_cnt;*/
	cnt = 0xFFF;
   
	do 
	{
		tmp32 = devcCtrl_Rd_CfgReg32(pDevC, DEVC_REG_SAC_STATUS);
		
		if ((tmp32 & SAC_STATUS_PCFG_DONE_MASK) == SAC_STATUS_PCFG_DONE_MASK) /* pcfg_done==1 => done*/
		{
			break;
		}
		
		cnt--;
	} while (cnt > 0);

	if (cnt <= 0)
	{
		printf("devcCtrl_Poll_FpgaDone fail: timeout! \n");
		return FMSH_FAILURE;
	}
		
       
#if 1

    Dummy[i++] = 0x20000000;
    Dummy[i++] = 0x20000000;
    Dummy[i++] = 0x20000000;   /*Type 1 Write 1 Word to CMD*/
    Dummy[i++] = 0x20000000;   /*DESYNC Command*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    Dummy[i++] = 0x20000000;   /*NOOP*/
    
    devcCtrl_Xfer_Pcap(pDevC, &Dummy[0], FMSH_DMA_INVALID_ADDRESS, 12, 0, FMSH_NON_SECURE_PCAP_WRITE_DUMMMY); 
#endif

	/**/
    /* usr_level_shift*/
    /**/
    devcSlcr_En_UsrLvlShift(pDevC);
    
    /*
    enable HP interface: ddr
	*/
#if 1	
	/* 	unlock slcr	*/
	fmqlSlcrWrite (FMQL_SR_UNLOCK_OFFSET, FMQL_SR_UNLOCK_ALL);  

    tmp32 = FMSH_ReadReg(DDR_UMC_REG_BASE, 0x490);
    /*if (0x1 == tmp32)*/
    {    
    	FMSH_WriteReg(DDR_UMC_REG_BASE, 0x490, 0x1); 
    }
	
    tmp32 = FMSH_ReadReg(DDR_UMC_REG_BASE, 0x540);
	/*if (0x1 == tmp32)*/
	{	 
        FMSH_WriteReg(DDR_UMC_REG_BASE, 0x540, 0x1); 
	}
	
    tmp32 = FMSH_ReadReg(DDR_UMC_REG_BASE, 0x5f0);
	/*if (0x1 == tmp32)*/
	{	 
        FMSH_WriteReg(DDR_UMC_REG_BASE, 0x5f0, 0x1); 
	}
	
    tmp32 = FMSH_ReadReg(DDR_UMC_REG_BASE, 0x6a0);
	/*if (0x1 == tmp32)*/
	{	 
        FMSH_WriteReg(DDR_UMC_REG_BASE , 0x6a0, 0x1); 
	}
	
	/* lock slcr */
	fmqlSlcrWrite (FMQL_SR_LOCK_OFFSET, FMQL_SR_LOCK_ALL);  
 #endif
 
    return FMSH_SUCCESS;
}

/****************************************************************************/
/**
*
* This function starts the DMA transfer. This function only starts the
* operation and returns before the operation may be completed.
* If the interrupt is enabled, an interrupt will be generated when the
* operation is completed, otherwise it is necessary to poll the Status register
* to determine when it is completed. It is the responsibility of the caller to
* determine when the operation is completed by handling the generated interrupt
* or polling the Status Register.
*
* @param	pDevC is devc handle.
* @param	SourceDataPtr contains a pointer to the source memory where the data
*		is to be transferred from.
* @param	srcWdLen is the number of words (32 bit) to be transferred
*		for the source transfer.
* @param	DestinationDataPtr contains a pointer to the destination memory
*		where the data is to be transferred to.
* @param	dstWdLen is the number of words (32 bit) to be transferred
*		for the Destination transfer.
* @param	xferMode 
*           FMSH_NON_SECURE_PCAP_WRITE		0
*           FMSH_SECURE_PCAP_WRITE			1
*           FMSH_PCAP_READBACK			2
*
* @return              
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None.
*
*****************************************************************************/
int devcCtrl_Load_Pcap_BitStream(vxT_DEVC* pDevC, 
				                               UINT32 srcDataAdr, UINT32 dstDataAdr, 
				                               UINT32 dataWdLen,  UINT32 xferMode)
{
    int ret = 0;
    UINT32 tmp32 = 0;

	ret = devcCtrl_Clr_PcapStatus(pDevC);
    if (ret != FMSH_SUCCESS)
    {    
    	printf("devcCtrl_Clr_PcapStatus fail! \n");
    	return FMSH_FAILURE;
    }
	else
	{	
		printf("devcCtrl_Clr_PcapStatus ok! \n");
	}

	ret = devcCtrl_Xfer_Pcap(pDevC, srcDataAdr, dstDataAdr, dataWdLen, dataWdLen, xferMode);
    if (ret != FMSH_SUCCESS)
    {    
    	printf("devcCtrl_Xfer_Pcap() fail! \n");
    	return FMSH_FAILURE;
    }
	else
	{
		/*printf("FDevcPs_transfer(0x%08X - len: 0x%08X) ok \n\n", srcDataAdr, srcWdLen);*/
	}

	devcCtrl_SetHi_CSI_B(pDevC);
	devcCtrl_SetHi_RDWR_B(pDevC);
	
	/*printf("FDevcPs_transfer(0x%08X - len: 0x%08X) ok \n\n", srcDataAdr, dataWdLen);*/

	/**/
	/* Get PCAP Interrupt Status Register*/
	/**/
	tmp32 = devcCtrl_Rd_CfgReg32(pDevC, DEVC_REG_SAC_INT_STS);
	if (tmp32 & SAC_INT_STS_ERR_FLAG_MASK)
	{
    	printf("SAC_INT_STS_ERR_FLAG_MASK fail! \n");
		return FMSH_FAILURE;
	}
	else
	{
		printf("Check sac_int_sts for errors-> OK! \n\n");
	}

    return FMSH_SUCCESS;
}


/*
csu_rst_ctrl_reg    0x0d4   淇濈暀  31:5    wo  0x0 淇濈暀  SAC_RST 鏄�
	   dma_switch_rst  4		   鍐�鍒欏浣岲MA_SWITCH	   
	   sm4_rst         3		   鍐�鍒欏浣峉M4 	   
	   aes_rst         2		   鍐�鍒欏浣岮ES 	   
	   sha256_rst      1		   鍐�鍒欏浣峉HA256	   
	   dma_soft_rst    0		   鍐�鍒欏浣岰SU_DMA 	   
					   鑷姩娓呴浂瀵勫瓨鍣紝鍐欑浉搴斾綅鍗冲彲瑙﹀彂鐩稿簲鎿嶄綔 	   
*/
void devcCtrl_SAC_rst(void)
{	
	UINT32 addr = 0xE0040000;
	addr += 0xD4;

	*((UINT32*)addr) = 0x11;
	return;
}


#endif


#if 1

void devcSlcr_En_CfgLvlShift(vxT_DEVC* pDevC)
{  
	slcr_write(0x83C, 0x0F);  /* open the cfg-level-shifter*/
	return;
}

void devcSlcr_Dis_CfgLvlShift(vxT_DEVC* pDevC)
{  
	slcr_write(0x83C, 0x00);  /* close the cfg-level-shifter*/
	return;
}

void devcSlcr_En_UsrLvlShift(vxT_DEVC* pDevC)
{  
	slcr_write(0x838, 0x0F);  /* open the user-level-shifter*/
	return;
}

void devcSlcr_Dis_UseLvlShift(vxT_DEVC* pDevC)
{  
	slcr_write(0x838, 0x00);  /* close the user-level-shifter*/
	return;
}

unsigned long downCpuFreq_ARM_PLL[32] =
{
    /* ARM_PLL_BYPASS_FORCE = 1*/
    EMIT_MASKWRITE(FPS_SLCR_BASEADDR + 0x100, 0x00000008U, 0x00000008U),
    
    /* ARM_PLL_RESET = 1*/
    EMIT_MASKWRITE(FPS_SLCR_BASEADDR + 0x100, 0x00000001U, 0x00000001U),
    
    /* ARM_PLL_FDIV = 13*/
    /* [22:16]*/
    EMIT_MASKWRITE(FPS_SLCR_BASEADDR + 0x100, 0x007f0000U, 0x000c0000U),
    
    /* ARM_PLL_CLKOUT0_DIVISOR = 2*/
    /* [6:0]*/
    EMIT_MASKWRITE(FPS_SLCR_BASEADDR + 0x104, 0x0000007fU, 0x00000002U),
    
    /* ENABLE ARM PLL LATCH*/
    EMIT_WRITE(FPS_SLCR_BASEADDR + 0x180, 0x00000001U), 
    
    /* ARM_PLL_RESET = 0*/
    EMIT_MASKWRITE(FPS_SLCR_BASEADDR + 0x100, 0x00000001U, 0x00000000U),
    
    /* CHECK ARM PLL STATUS*/
    EMIT_MASKPOLL(FPS_SLCR_BASEADDR + 0x170, 0x00000001U),
    
    /* ARM_PLL_BYPASS_FORCE = 0*/
    EMIT_MASKWRITE(FPS_SLCR_BASEADDR + 0x100, 0x00000008U, 0x00000000U),

    EMIT_EXIT(),
};

int devcSlcr_Down_CpuFreq(vxT_DEVC* pDevC, int* down_ok_flag)
{
    int ret = FMSH_SUCCESS;
	
    UINT32 clk_div = 1;
    UINT32 pll_multi = 8;
    UINT32 pll_div = 2;
    
    /*FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x008, 0xDF0D767B);   /* unlock SLCR */
	/*
	CLK_621_TRUE	0x20C	
			reserved		31:1	rw		0x0 Reserved. 鍐欐棤鏁堬紝璇讳负0
			CLK_621_TRUE	0		rw		0x0 "CPU/AXI/AHB鏃堕挓姣斾緥621
					0锛�榛樿4:2:1 锛堜簩鍒嗛锛�
					1锛�:2:1 锛堜笁鍒嗛锛�
	*/
    if ((slcr_read(0x20C) & 0x01) == 0x01) 
    {  
    	clk_div = 12;
	}
    else
    {  
    	clk_div = 8;
	}

	/*
	ARM_PLL_CLKOUT0_DIVISOR 0x104		
	*/
    pll_div = slcr_read(0x104) & 0x7F;
	
    if ((APU_FREQ / clk_div) > 20000000)  /* 20_000_000 -> 20M Hz*/
    {
		/*pll_multi=35*lk_div*ll_div*000000/PS_CLK_FREQ;		*/
		pll_multi = (35 * clk_div * pll_div * 1000000) / PS_CLK_FREQ;
		
		downCpuFreq_ARM_PLL[11] = pll_multi << 16;
		downCpuFreq_ARM_PLL[15] = pll_div;
		
		/* PLL init */
		ret = slcrRegs_Config(downCpuFreq_ARM_PLL);
		if (ret != FMSH_SUCCESS) 
		{	
			printf("downCpuFreq fail! \n");
			return ret;
		}
		
		*down_ok_flag = 1;
    }
	
    return ret;
}


unsigned long upCpuFreq_ARM_PLL[] = 
{
    /* UNLOCK_KEY = 0XDF0D767B*/
    EMIT_WRITE(SLCR_REG_BASE + 0x008, 0xDF0D767BU),

    /* CPU_PLL_BYPASS_FORCE = 1*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x100, 0x00000008U, 0x00000008U),
    
    /* CPU_PLL_RESET = 1*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x100, 0x00000001U, 0x00000001U),
    
    /* CPU_PLL_FDIV = 80*/
    /* [22:16]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x100, 0x007f0000U, 0x00500000U),
    
    /* CPU_PLL_CLKOUT0_DIVISOR = 4*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x104, 0x0000007fU, 0x00000004U),
    
    /* CPU_PLL_CLKOUT1_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x108, 0x0000007fU, 0x0000002fU),
    
    /* CPU_PLL_CLKOUT2_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x10C, 0x0000007fU, 0x0000002fU),
    
    /* CPU_PLL_CLKOUT3_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x120, 0x0000007fU, 0x0000002fU),
    
    /* CPU_PLL_CLKOUT4_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x124, 0x0000007fU, 0x0000002fU),
    
    /* CPU_PLL_CLKOUT5_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x128, 0x0000007fU, 0x0000002fU),
    
    /* ENABLE CPU PLL LATCH*/
    EMIT_WRITE(SLCR_REG_BASE + 0x180, 0x00000001U), 



	
    /* CPU_PLL_RESET = 0*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x100, 0x00000001U, 0x00000000U),
    
    /* CHECK CPU PLL STATUS*/
    EMIT_MASKPOLL(SLCR_REG_BASE + 0x170, 0x00000001U),
    
    /* CPU_PLL_BYPASS_FORCE = 0*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x100, 0x00000008U, 0x00000000U),



	
    /* DDR_PLL_BYPASS_FORCE = 1*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x130, 0x00000008U, 0x00000008U),
    
    /* DDR_PLL_RESET = 1*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x130, 0x00000001U, 0x00000001U),
    
    /* DDR_PLL_FDIV = 60*/
    /* [22:16]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x130, 0x007f0000U, 0x003c0000U),
    
    /* DDR_PLL_CLKOUT0_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x134, 0x0000007fU, 0x0000002fU),
    
    /* DDR_PLL_CLKOUT1_DIVISOR = 5*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x138, 0x0000007fU, 0x00000005U),
    
    /* DDR_PLL_CLKOUT2_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x13C, 0x0000007fU, 0x0000002fU),
    
    /* DDR_PLL_CLKOUT3_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x140, 0x0000007fU, 0x0000002fU),
    
    /* DDR_PLL_CLKOUT4_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x144, 0x0000007fU, 0x0000002fU),
    
    /* DDR_PLL_CLKOUT5_DIVISOR = 2*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x148, 0x0000007fU, 0x00000002U),
    
    /* ENABLE DDR PLL LATCH*/
    EMIT_WRITE(SLCR_REG_BASE + 0x184, 0x00000001U), 
    
    /* DDR_PLL_RESET = 0*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x130, 0x00000001U, 0x00000000U),
    
    /* CHECK DDR PLL STATUS*/
    EMIT_MASKPOLL(SLCR_REG_BASE + 0x170, 0x00000002U),
    
    /* DDR_PLL_BYPASS_FORCE = 0*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x130, 0x00000008U, 0x00000000U),
    
    /* IO_PLL_BYPASS_FORCE = 1*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x150, 0x00000008U, 0x00000008U),
    
    /* IO_PLL_RESET = 1*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x150, 0x00000001U, 0x00000001U),
    
    /* IO_PLL_FDIV = 90*/
    /* [22:16]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x150, 0x007f0000U, 0x005a0000U),
    
    /* IO_PLL_CLKOUT0_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x154, 0x0000007fU, 0x0000002fU),
    
    /* IO_PLL_CLKOUT1_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x158, 0x0000007fU, 0x0000002fU),
    
    /* IO_PLL_CLKOUT2_DIVISOR = 30*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x15C, 0x0000007fU, 0x0000001eU),
    
    /* IO_PLL_CLKOUT3_DIVISOR = 47*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x160, 0x0000007fU, 0x0000002fU),
    
    /* IO_PLL_CLKOUT4_DIVISOR = 15*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x164, 0x0000007fU, 0x0000000fU),
    
    /* IO_PLL_CLKOUT5_DIVISOR = 30*/
    /* [6:0]*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x168, 0x0000007fU, 0x0000001eU),
    
    /* ENABLE IO PLL LATCH*/
    EMIT_WRITE(SLCR_REG_BASE + 0x188, 0x00000001U), 
    
    /* IO_PLL_RESET = 0    */
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x150, 0x00000001U, 0x00000000U),
    
    /* CHECK IO PLL STATUS*/
    EMIT_MASKPOLL(SLCR_REG_BASE + 0x170, 0x00000004U),
    
    /* IO_PLL_BYPASS_FORCE = 0*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x150, 0x00000008U, 0x00000000U),

    /* LOCK_KEY = 0XDF0D767B*/
    EMIT_WRITE(SLCR_REG_BASE + 0x004, 0xDF0D767BU),

    EMIT_EXIT(),
};

/*****************************************************************************/
/**
* This function recover the cpu freq.
*
* @param	
*
* @return
*		- 0 if successful
*		- 1 if unsuccessful
*
* @note		None
*
****************************************************************************/
int devcSlcr_Up_CpuFreq(void)
{ 
    UINT32 idx;
    UINT8 ret = FMSH_SUCCESS;
	
    unsigned long pll_data[256] = {0};
	
    for (idx=0; idx<256; idx++)
    {
        pll_data[idx] = upCpuFreq_ARM_PLL[idx];

		/**/
        /* arm pll force*/
        /**/
        if ((pll_data[idx-3]  == ((OPCODE_MASKWRITE << 4) | 3))  \
		  && (pll_data[idx-2] == SLCR_REG_BASE + 0x100)       \
		  && (pll_data[idx-1] == 0x00000008U)                 \
		  && (pll_data[idx]   == 0x00000000U))
        {
        	break;  
		}
    }
	
    if (idx >= 256)
	{	
		return FMSH_FAILURE;
    }
	
    pll_data[idx]= EMIT_EXIT();

	/**/
    /* PLL init */
    /**/
    ret = slcrRegs_Config(pll_data);
	
    if (ret != FMSH_SUCCESS) 
  	{	
  		printf("upCpuFre fail! \n");
  		return ret;
    }
    
    return FMSH_SUCCESS;
}

#endif



#if 1  /* whole-bit_stream*/

int vxInit_Devc(void)
{
	vxT_DEVC * pDevC = NULL;
	vxT_DEVC_CTRL * pDevcCtrl = NULL;
	
	int ret = 0;	
	
#if 1 
	/*
	init the pDevC structure
	*/

	/**/
	/* spi_ctrl select*/
	/**/
	bzero((char*)(&g_pDevC->devc_x), sizeof(vxT_DEVC));	
	pDevC = g_pDevC;	
	
	bzero((char*)(&vxDevC_Ctrl.ctrl_x), sizeof(vxT_DEVC_CTRL));
	pDevC->pDevcCtrl = (vxT_DEVC_CTRL *)(&vxDevC_Ctrl.ctrl_x);
	
	pDevcCtrl = pDevC->pDevcCtrl;	
	
	pDevC->devc_x = DEVC_CTRL_0;	
	pDevcCtrl->ctrl_x = pDevC->devc_x;
	
	pDevcCtrl->cfgBaseAddr = VX_DEVC_CFG_BASE;			
#endif	

	return FMSH_SUCCESS;
}

int devC_Load_BitStream_NonSecure(vxT_DEVC* pDevC, UINT32 srcAddr, UINT32 wordLen)
{
    int ret = FMSH_SUCCESS;
	
	/*
	0xE004_0000锝�xE004_0FFF	 4KB	CSU(DEVC)	AHB_SUBSYS
	*/
	
    /*printf(" start: FDevcPs_readFifoThre()... \n");*/

    devcCtrl_Set_ReadFifoThrsh(pDevC, READFIFO_THRSH_HEX_40);	
    printf(" end: devcCtrl_Set_ReadFifoThrsh()!!! \n\n");

	/*taskDelay(10);*/
	
   /* printf(" start: FDevcPs_fabricInit()... \n");*/
	/*taskDelay(10);*/
	
	devcCtrl_Init_Pcap(pDevC, FMSH_NON_SECURE_PCAP_WRITE);
	
	/*taskDelay(10);*/
    printf(" end: devcCtrl_Init_Pcap()!!! \n\n");
	

 	printf("srcAddr: 0x%08X, len-0x%X \n", srcAddr, wordLen);
	
    ret = devcCtrl_Load_Pcap_BitStream(pDevC, srcAddr, PCAP_WR_DATA_ADDR, wordLen, FMSH_NON_SECURE_PCAP_WRITE);
    if (ret != FMSH_SUCCESS)
    {
		printf(" end: devcCtrl_Load_Pcap_BitStream() fail!!! \n");
        return ret;
    }
	else
	{		
		printf(" end: devcCtrl_Load_Pcap_BitStream() OK!!! \n");
	}
    
    ret = devcCtrl_Poll_FpgaDone(pDevC, DEVC_MAX_COUNT); 
    if (ret != FMSH_SUCCESS)
    {
	   printf(" end: devcCtrl_Poll_FpgaDone() fail!!! \n");
       return ret;
    }
	else
	{		
		printf(" end: devcCtrl_Poll_FpgaDone() OK!!! \n");
	}
	
    return FMSH_SUCCESS;
}

int devC_Update_PL(vxT_DEVC* pDevC, UINT32 load_addr, UINT32 word_size)
{
	int ret = 0;
	int dwnFreqFlag = 0;
	
	devcSlcr_Dis_UseLvlShift(pDevC);
	
	sysMsDelay(50);
	
	devcSlcr_En_CfgLvlShift(pDevC);  /* cfg_level_shift 鎵撳紑 */

#if 1	
	ret = devcSlcr_Down_CpuFreq(pDevC, &dwnFreqFlag);  /* 闄嶉鐜� */
	if (ret != FMSH_SUCCESS)
	{
		printf("devcSlcr_Down_CpuFreq: reduce & down freq fail! \n\n");
		return FMSH_FAILURE;
	}
	else
	{
		clk_cpu_show();
		printf(" down cpu freq: freqFlag=%d \n\n", dwnFreqFlag);
	}
#endif	
	
	ret = devC_Load_BitStream_NonSecure(pDevC, load_addr, word_size);
	if (ret != FMSH_SUCCESS)
	{
		printf("devC_Load_BitStream_NonSecure:fail! \n\n");
		return FMSH_FAILURE;
	}
	else
	{
		printf("devC_Load_BitStream_NonSecure:OK (from:0x%08X,size:0x%X) \n\n", load_addr, word_size);
	}
	
#if 1	
	if (dwnFreqFlag != 0)
	{  
		devcSlcr_Up_CpuFreq();  /* 鍗囬鐜� */

		printf("devcSlcr_Up_CpuFreq: add & up freq! \n\n");
		clk_cpu_show();
	}
#endif
	
	clk_cpu_show();

	return FMSH_SUCCESS;
}

#endif

#if 1  /* part-bit_stream*/
int devcCtrl_Init_Pcap_part(vxT_DEVC* pDevC, UINT32 xferType)
{        
 	int ret = FMSH_SUCCESS;
	UINT32 tmp32 = 0;
	
	/**/
	/* UnLock CSU*/
	/**/
	devcCtrl_UnLock_CSU(pDevC);

	tmp32 =devcCtrl_Rd_CfgReg32(pDevC, DEVC_REG_SAC_CFG);
	printf("DEVC_REG_SAC_CFG: 0x%08X \n");
		
	devcCtrl_Set_Smap32_SwapCtrl(pDevC, SWAP32_SWAP_EN);
	
	devcCtrl_Set_RdClk_Edge(pDevC, FALLING_EDGE);
	devcCtrl_Set_WrClk_Edge(pDevC, FALLING_EDGE);

	devcCtrl_SetHi_CSI_B(pDevC);
	devcCtrl_SetHi_RDWR_B(pDevC);

	switch (xferType)
	{
    case FMSH_PCAP_READBACK: 
	    {
	        devcCtrl_Set_RxDataSwap(pDevC, DATA_SWAP_BYTE);
	        devcCtrl_Set_TxDataSwap(pDevC, DATA_SWAP_BYTE); 

			/**/
	        /* default 3 , for FMSH 325t the value must be set 4,*/
	        /* for fmsh 325t the value must be set 3 for readback reg, but the value must be set 4 for readback bitstream*/
	        /**/
			devcCtrl_Set_DummyCnt(pDevC, DUMMY_NUM_4);
	        
	        devcCtrl_Set_PL_DwnLdMode(pDevC, DMA_MODE_DWNLD_BITSTREAM);
			
			devcCtrl_Set_ReadFifoThrsh(pDevC, READFIFO_THRSH_HEX_40);
	    } 
		break;
	
    case FMSH_NON_SECURE_PCAP_WRITE:
	    {
	        /**/
			/* bit9-鏉ヨ嚜PL_CFG妯″潡鐨刢fg_done淇″彿*/
			/**/
	        if (devcCtrl_Rd_CfgReg32(pDevC, DEVC_REG_SAC_STATUS) & 0x200)  
	        {   
				printf("FMSH_NON_SECURE_PCAP_WRITE: FDevcPs_Prog_B \n");
	        	/*devcCtrl_Prog_B(pDevC); // for part-bit_stream update*/
	        }
			
	        devcCtrl_Set_RxDataSwap(pDevC, DATA_SWAP_BYTE);
			
	        devcCtrl_Set_PL_DwnLdMode(pDevC, DMA_MODE_DWNLD_BITSTREAM);
			
			devcCtrl_Set_WriteFifoThrsh(pDevC, WRITEFIFO_THRSH_HEX_80);
	    }
		break;
	
    case FMSH_PCAP_LOOPBACK: 
	    {
			devcCtrl_Set_RxDataSwap(pDevC, DATA_SWAP_BYTE);
			devcCtrl_Set_TxDataSwap(pDevC, DATA_SWAP_BYTE);
			
			devcCtrl_Set_DummyCnt(pDevC, DUMMY_NUM_4);
			devcCtrl_Set_PL_DwnLdMode(pDevC, DMA_MODE_DATA_LOOPBACK);			
	    }
		break;
	
    default:
		printf("xferType: 0x%X fail! \n", xferType);
        ret = FMSH_FAILURE;
		break;
	}

    return ret;
}

int devC_Load_BitStream_NonSecure_2(vxT_DEVC* pDevC, UINT32 srcAddr, UINT32 wordLen)
{
    int ret = FMSH_SUCCESS;
	
	/*
	0xE004_0000锝�xE004_0FFF	 4KB	CSU(DEVC)	AHB_SUBSYS
	*/
	
    /*printf(" start: FDevcPs_readFifoThre()... \n");*/

    devcCtrl_Set_ReadFifoThrsh(pDevC, READFIFO_THRSH_HEX_40);	
    printf(" end: devcCtrl_Set_ReadFifoThrsh()!!! \n\n");

	/*taskDelay(10);*/
	
   /* printf(" start: FDevcPs_fabricInit()... \n");*/
	/*taskDelay(10);*/
	
	devcCtrl_Init_Pcap_part(pDevC, FMSH_NON_SECURE_PCAP_WRITE);
	
	/*taskDelay(10);*/
    printf(" end: devcCtrl_Init_Pcap()!!! \n\n");
	

 	printf("srcAddr: 0x%08X, len-0x%X \n", srcAddr, wordLen);
	
    ret = devcCtrl_Load_Pcap_BitStream(pDevC, srcAddr, PCAP_WR_DATA_ADDR, wordLen, FMSH_NON_SECURE_PCAP_WRITE);
    if (ret != FMSH_SUCCESS)
    {
		printf(" end: devcCtrl_Load_Pcap_BitStream() fail!!! \n");
        return ret;
    }
	else
	{		
		printf(" end: devcCtrl_Load_Pcap_BitStream() OK!!! \n");
	}
    
    ret = devcCtrl_Poll_FpgaDone(pDevC, DEVC_MAX_COUNT); 
    if (ret != FMSH_SUCCESS)
    {
	   printf(" end: devcCtrl_Poll_FpgaDone() fail!!! \n");
       return ret;
    }
	else
	{		
		printf(" end: devcCtrl_Poll_FpgaDone() OK!!! \n");
	}
	
    return FMSH_SUCCESS;
}

int devC_Update_PL_part(vxT_DEVC* pDevC, UINT32 load_addr, UINT32 word_size)
{
	int ret = 0;
	int dwnFreqFlag = 0;
	
/*	
	devcSlcr_Dis_UseLvlShift(pDevC);
	
	sysMsDelay(50);
	
	devcSlcr_En_CfgLvlShift(pDevC);  // cfg_level_shift 鎵撳紑
*/

#if 1	
	ret = devcSlcr_Down_CpuFreq(pDevC, &dwnFreqFlag);  /* 闄嶉鐜�*/
	if (ret != FMSH_SUCCESS)
	{
		printf("devcSlcr_Down_CpuFreq: reduce & down freq fail! \n\n");
		return FMSH_FAILURE;
	}
	else
	{
		clk_cpu_show();
		printf(" down cpu freq: freqFlag=%d \n\n", dwnFreqFlag);
	}
#endif	

	/**/
	/* part bit_stream update*/
	/**/
	ret = devC_Load_BitStream_NonSecure_2(pDevC, load_addr, word_size);
	if (ret != FMSH_SUCCESS)
	{
		printf("devC_Load_BitStream_NonSecure:fail! \n\n");
		return FMSH_FAILURE;
	}
	else
	{
		printf("devC_Load_BitStream_NonSecure:OK (from:0x%08X,size:0x%X) \n\n", load_addr, word_size);
	}
	
#if 1	
	if (dwnFreqFlag != 0)
	{  
		devcSlcr_Up_CpuFreq();  /* 鍗囬鐜� */

		printf("devcSlcr_Up_CpuFreq: add & up freq! \n\n");
		clk_cpu_show();
	}
#endif
	
	clk_cpu_show();

	return FMSH_SUCCESS;
}

#endif

#define DDR_BUF_ADDR_FOR_PL   (0x20000000)   /* ddr 0x2000_0000 for update pl bit*/
#define DDR_BUF_SIZE_FOR_PL   (0x01000000)   /* 16M*/


#ifdef QSPI_2_VX  /* qspi_0 norflash must be exist*/

UINT32 swap_hex32_2(UINT32 input)
{
#if 1
	UINT32 tmp32 = 0;

	tmp32 = (input & 0x000000FF) << 24;
	tmp32 |= (input & 0x0000FF00) << 8;
	tmp32 |= (input & 0x00FF0000) >> 8;
	tmp32 |= (input & 0xFF000000) >> 24;

	return tmp32;
#else
	return input;
#endif
	
}

/*
A. Sure.
The Xilinx .bit format is pretty simple.  It uses keys and lengths to
divide the file.
Here is an example.  Below is a hex dump from the beginning of a .bit file:

00000000:  00 09 0f f0 0f f0 0f f0 0f f0 00 00 01 61 00 0a  *.............a..*
00000010:  78 66 6f 72 6d 2e 6e 63 64 00 62 00 0c 76 31 30  *xform.ncd.b..v10*
00000020:  30 30 65 66 67 38 36 30 00 63 00 0b 32 30 30 31  *00efg860.c..2001*
00000030:  2f 30 38 2f 31 30 00 64 00 09 30 36 3a 35 35 3a  *_08_10.d..06:55:*
00000040:  30 34 00 65 00 0c 28 18 ff ff ff ff aa 99 55 66  *04.e..(.......Uf*

Field 1
2 bytes          length 0x0009           (big endian)
9 bytes          some sort of header

Field 2
2 bytes          length 0x0001
1 byte           key 0x61                (The letter "a")

Field 3
2 bytes          length 0x000a           (value depends on file name length)
10 bytes         string design name "xform.ncd" (including a trailing 0x00)

Field 4
1 byte           key 0x62                (The letter "b")
2 bytes          length 0x000c           (value depends on part name length)
12 bytes         string part name "v1000efg860" (including a trailing 0x00)

Field 5
1 byte           key 0x63                (The letter "c")
2 bytes          length 0x000b
11 bytes         string date "2001/08/10"  (including a trailing 0x00)

Field 6
1 byte           key 0x64                (The letter "d")
2 bytes          length 0x0009
9 bytes          string time "06:55:04"    (including a trailing 0x00)

Field 7
1 byte           key 0x65                 (The letter "e")
4 bytes          length 0x000c9090        (value depends on device type,
                                           and maybe design details)
                                           
8233440 bytes    raw bit stream starting with 0xffffffff aa995566 sync
                 word documented below.
*/

/*
PL bit 娴佽皟鏁磋鍒欙細
==================================================
姣斿锛�uboot tftp ddr鍐呭師濮媌it鏁版嵁
--------------------------------------------------
0  1  2  3   4  5  6  7   8  9  10 11  12 13 14 15
--------------------------------------------------
00 09 0f f0  0f f0 0f f0  0f f0 00 00  01 61 00 25
74 6f 70 3b  55 73 65 72  49 44 3d 30  58 46 46 46
46 46 46 46  46 3b 56 65  72 73 69 6f  6e 3d 32 30
31 36 2e 34  00 62 00 0c  37 7a 30 34  35 66 66 67
39 30 30 00  63 00 0b 32  30 31 39 2f  31 32 2f 32
33 00 64 00  09 31 31 3a  32 33 3a 34  32 00 65 00
cb 44 bc 
		 ff  ff ff ff ff  ff ff ff ff  ff ff ff ff
ff ff ff ff  ff ff ff ff  ff ff ff ff  ff ff ff ff
ff ff ff 00  00 00 bb 11  22 00 44 ff  ff ff ff ff
ff ff ff aa  99 55 66 20  00 00 00 30  02 20 01 00
00 00 00 30  02 00 01 00  00 00 00 30  00 80 01 00
00 00 00 20  00 00 00 30  00 80 01 00  00 00 07 20
00 00 00 20  00 00 00 30  02 60 01 00  00 00 00 30
01 20 01 02  00 3f e5 30  01 c0 01 00  00 00 00 30
01 80 01 03  73 10 93 30  00 80 01 00  00 00 09 20
00 00 00 30  00 c0 01 00  00 04 01 30  00 a0 01 00
00 05 01 30  00 c0 01 00  00 00 00 30  03 00 01 00
00 00 00 20  00 00 00 20  00 00 00 20  00 00 00 20
...
===============================================
璋冩暣涓猴細
1.锛堣繛缁�32涓�FF锛変负鏁版嵁澶达紝
2. 4瀛楄妭瀵归綈,
--------------------------------------------------
--------------------------------------------------
0  1  2  3   4  5  6  7   8  9  10 11  12 13 14 15
--------------------------------------------------
ff ff ff ff  ff ff ff ff  ff ff ff ff  ff ff ff ff
ff ff ff ff  ff ff ff ff  ff ff ff ff  ff ff ff ff
00 00 00 bb  11 22 00 44  ff ff ff ff  ff ff ff ff
aa 99 55 66                                           -- bit-stream bin_header
			 20 00 00 00  30 02 20 01  00 00 00 00
30 02 00 01  00 00 00 00  30 00 80 01  00 00 00 00
20 00 00 00  30 00 80 01  00 00 00 07  20 00 00 00
20 00 00 00  30 02 60 01  00 00 00 00  30 01 20 01
02 00 3f e5  30 01 c0 01  00 00 00 00  30 01 80 01
03 73 10 93  30 00 80 01  00 00 00 09  20 00 00 00
30 00 c0 01  00 00 04 01  30 00 a0 01  00 00 05 01
30 00 c0 01  00 00 00 00  30 03 00 01  00 00 00 00
20 00 00 00  20 00 00 00  20 00 00 00  20 00 00 00
*/

#define PL_BIT_FILE_SRC_DDR_ADR2   (0x20000000)

/*static UINT8 pl_bit_file[0x10100] = {0};   // 64K + 256*/

int g_vxInit_Qspi_ok2 = 0;

void testvx_update_bit2(char* bit_file)
{	
	vxT_QSPI * pQspi = g_pQspi0;
	
	vxT_QSPI_CTRL * pQspiCtrl = NULL;
	vxT_QSPI_FLASH * pQspiFlash = NULL;
		
	UINT8 * pBuf8 = NULL;
	UINT32 * pBuf32 = NULL;
	
	int j = 0, byteCnt = 0;
	int sect_idx = 0, sect_size = 0;
	
	int i = 0, base_idx = 0;
	/*int bit_file_size  = 0xCB44C0;  // 0xE00000 14M  Bytes transferred = 13321503 (cb451f hex)*/
	int bit_file_size  = 0xCB44BF;  /* 0xE00000 14M  Bytes transferred = 13321503 (cb451f hex)*/
	int bit_file_size2 = 0xcb451f;  /* 0xE00000 14M  Bytes transferred = 13321503 (cb451f hex)*/

	int ret = 0;
	UINT8 fileName[64] = {0};
	int read_size = 0;
	int file_size = 0;

	int fd = 0;
	
	/*FIL file_obj;		 File object */


#if 1  /* devc_init	*/
	vxInit_Devc();
#endif
	

#if 0
	pBuf8 = (UINT8 *)(&pl_bit_file[0]);
#else

	#if 0  /* pl_top.bit at qspi_flash*/
	
		/*pBuf8 = (UINT8 *(malloc(0x1000000));   16M	*/
		pBuf8 = (UINT8 *)(PL_BIT_FILE_SRC_DDR_ADR2);  /* ddr phy_addr: 0x3000_0000*/
		pBuf32 = (UINT32*)(&pBuf8[0]);

		if (g_vxInit_Qspi_ok2 == 0)
		{
			vxInit_Qspi(0);
			
		
			pQspiCtrl = pQspi->pQspiCtrl;
			pQspiFlash = pQspi->pQspiFlash;

			byteCnt = pQspiFlash->sectSize;	
			sect_size = pQspiFlash->sectSize;	

			printf("sector size: %d \n", pQspiFlash->sectSize);

			/*for (i=0; i<(bit_file_sizeect_size); i++)*/
			for (i=0; i<(0xE00000/sect_size); i++)
			{
				qspiFlash_Read_Sect(pQspi, i, (UINT8*)(&pBuf8[i*sect_size]));
			}

			printf("----qspi(%d)-read(%d):%dKB----\n", pQspiCtrl->ctrl_x, sect_idx, (byteCnt/1024));	
			
			g_vxInit_Qspi_ok2 = 1;
		}

	#elif 0 /* pl_top.bit at ddr by tftp*/
	
		pBuf8 = (UINT8 *)(DDR_BUF_ADDR_FOR_PL);  /* ddr-phy 0x20000000*/
		/*pBuf8 = (UINT8 *(DDR_BUF_ADDR_FOR_PL+0x30);   ddr-phy 0x20000000*/
		pBuf32 = (UINT32*)(&pBuf8[0]);
		
		printf("\n--------uboot_tftp-------\n\n");	
	
	#else  /* pl_top.bit at sd card	*/
	
		pBuf8 = (UINT8 *)(DDR_BUF_ADDR_FOR_PL);	/* ddr-phy 0x20000000*/
		pBuf32 = (UINT32*)(&pBuf8[0]);
		read_size = DDR_BUF_SIZE_FOR_PL;	/* < 16M */
		
		/*sprintf((char*(&fileName[0]), "d0:0s", "pl_top.bit");*/
		sprintf((char*)(&fileName[0]), "%s", bit_file); 
		
		ret = 0;
		fd = fopen((char*)(&fileName[0]), "r");  /* read*/
		if (fd > 0)
		{
			ret = fread(pBuf8, 1, read_size, fd);
		}
		
		if (ret == 0)
		{
			printf("> File(%s) read failed: (%d) \n\n", (char*)(&fileName[0]), ret);			
			return;
		}
		else
		{
			/*printf("> File(%s) open failed: (%d) \r\n", (char*(&fileName[0]), file_size);			*/
			 bit_file_size  = ret - (0x51F-0x4BF);  /* 0xCB44BF;	 0xE00000 14M  Bytes transferred = 13321503 (cb451f hex)*/
			 bit_file_size2 = ret;                  /* 0xcb451f;	 0xE00000 14M  Bytes transferred = 13321503 (cb451f hex)*/
			printf("> File(%s) read ok: (%d)(0x%X) \n\n", (char*)(&fileName[0]), ret, ret);			
		}
		
	#endif
#endif


#if 1
	/*=========================================*/
	for (j=0; j<512; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}

	printf(" ... ... ... ... \n\n");
	
	/*for (j=(byteCnt-512); j<byteCnt; j++)*/
	for (j=(bit_file_size-512); j<bit_file_size; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf("\n ------------\n");
	
	/*=========================================*/
#endif


	/*
	fpga PL bit-stream, del the header and adjust the bytes
	*/
#if 1
	/**/
	/* find the head of data area*/
	/**/
	for (i=0; i<256; i++)
	{
		for (j=0; j<32; j++)
		{
			if (pBuf8[i+j] != 0xFF)
			{
				break;  /* for (j=0; j<32; j++)*/
			}
		}

		if (j >= 32)
		{
			base_idx = i;
			break;  /* for (i=0; i<256; i++)*/
		}
	}

	printf("\nbase_idx=%d \n\n", base_idx);

	bit_file_size = (int)((pBuf8[base_idx-4] << 24) | (pBuf8[base_idx-3] << 16) | (pBuf8[base_idx-2] << 8) | (pBuf8[base_idx-1]));

	printf("pl bit file size: 0x%X \n\n", bit_file_size);

	/**/
	/* move data to idx_0*/
	/**/
	/*for (i=0; i<byteCnt; i++)*/
	#if 0
		for (i=0; i<bit_file_size; i++)
		{
			pBuf8[i] = pBuf8[base_idx + i];
		}
	#else	
		for (i=0; i<bit_file_size; i++)
		{
			pBuf8[i] = pBuf8[base_idx + i];  /* move data for del the header of bitstream*/
		}
		(void)cacheInvalidate (DATA_CACHE, DDR_BUF_ADDR_FOR_PL, DDR_BUF_SIZE_FOR_PL); /* flush cache, it's must*/
	#endif	
#endif

	
#if 1
	/*=========================================*/
	for (j=0; j<512; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}

	printf(" ... ... ... ... \n\n");
	
	/*for (j=(byteCnt-512); j<byteCnt; j++)*/
	for (j=(bit_file_size-512); j<bit_file_size; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf("\n-----byte seq-------\n\n");
	/*=========================================*/
#endif

	printf("-------word seq-----\n");
	for (j=0; j<128; j++)
	{
		printf("%08X ", pBuf32[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%128 == 0)
		{
			printf(" \n");
		}
	}
	printf("======word seq======\n");	
	for (j=(bit_file_size/4)-128; j<(bit_file_size/4); j++)
	{
		printf("%08X ", pBuf32[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%128 == 0)
		{
			printf(" \n");
		}
	}

	taskDelay(60);

	printf("\n\n pBuf32[0]: 0x%08X len:0x%08X \n\n", &pBuf32[0], (bit_file_size/4));

	devC_Update_PL(g_pDevC, (UINT32)(&pBuf32[0]), (bit_file_size/4));


	/*
	bitstream_checksum	0x0e0	bitstream_checksum
	csu_error_code_reg	0x0e4	淇濈暀
			csu_error_code_reg
	*/
	printf("\n\n");
	printf("bitstream_checksum: 0x%08X \n", *((UINT32*)(0xE0040000+0xE0)));	
	printf("csu_error_code_reg: 0x%08X \n", *((UINT32*)(0xE0040000+0xE4)));
	printf("\n\n");
	
	return;
}


#endif


#if 1  /* update pl: with bit file*/

int pl_bitstream_parse(UINT8* pBuf8)
{
	unsigned int length;
	unsigned int swapsize = 0;
	
	unsigned char * dataptr = (unsigned char *)pBuf8;
	
	/* 
	skip the first bytes of the bitsteam, their meaning is unknown 
	-------
	Field 1
	2 bytes 		 length 0x0009			 (big endian)
	9 bytes 		 some sort of header
	*/
	length = (*dataptr << 8) + *(dataptr + 1);
	dataptr += 2;	
	dataptr += length;

	/*
	get design name (identifier, length, string)
	-------
	Field 2
	2 bytes 		 length 0x0001
	1 byte			 key 0x61				 (The letter "a")
	*/
	length = (*dataptr << 8) + *(dataptr + 1);	
	dataptr += 2;
	if (*dataptr++ != 0x61) /* 'a'*/
	{
		printf("Design name id not recognized in bitstream \n");
		return -1;
	}

	/*
	Field 3
	2 bytes 		 length 0x000a			 (value depends on file name length)
	10 bytes		 string design name "xform.ncd" (including a trailing 0x00)
	*/
	length = (*dataptr << 8) + *(dataptr + 1);	
	dataptr += 2;
	printf("  Design Filename = \"%s\" \n", dataptr);	
	dataptr += length;
	
	/* 
	get part number (identifier, length, string)
	-------
	Field 4
	1 byte			 key 0x62				 (The letter "b")
	2 bytes 		 length 0x000c			 (value depends on part name length)
	12 bytes		 string part name "v1000efg860" (including a trailing 0x00)
	*/
	if (*dataptr++ != 0x62)  /* 'b'*/
	{
		printf("Part number id not recognized in bitstream \n");
		return -1;
	}
	length = (*dataptr << 8) + *(dataptr + 1);
	dataptr += 2;
	printf("  Part Number     = \"%s\" \n", dataptr);
	dataptr += length;

	/* 
	get date (identifier, length, string) 
	-------
	Field 5
	1 byte			 key 0x63				 (The letter "c")
	2 bytes 		 length 0x000b
	11 bytes		 string date "2001/08/10"  (including a trailing 0x00)
	*/
	if (*dataptr++ != 0x63)  /* 'c'*/
	{
		printf("Date identifier not recognized in bitstream \n");
		return -1;
	}
	length = (*dataptr << 8) + *(dataptr+1);
	dataptr += 2;
	printf("  Date            = \"%s\" \n", dataptr);
	dataptr += length;

	/* 
	get time (identifier, length, string)
	-------
	Field 6
	1 byte			 key 0x64				 (The letter "d")
	2 bytes 		 length 0x0009
	9 bytes 		 string time "06:55:04"    (including a trailing 0x00)
	*/
	if (*dataptr++ != 0x64)  /* 'd'*/
	{
		printf("Time identifier not recognized in bitstream \n");
		return -1;
	}
	length = (*dataptr << 8) + *(dataptr+1);
	dataptr += 2;
	printf("  Time            = \"%s\" \n", dataptr);
	dataptr += length;

	/* 
	get fpga data length (identifier, length) 
	-------
	Field 7
	1 byte			 key 0x65				  (The letter "e")
	4 bytes 		 length 0x000c9090		  (value depends on device type, and maybe design details)
	*/
	if (*dataptr++ != 0x65)  /* 'e'*/
	{
		printf("Data length id not recognized in bitstream \n");
		return -1;
	}	
	swapsize = ((unsigned int) *dataptr << 24) +
		   ((unsigned int) *(dataptr + 1) << 16) +
		   ((unsigned int) *(dataptr + 2) << 8) +
		   ((unsigned int) *(dataptr + 3));
	dataptr += 4;	
	printf("  Bytes_Bitstream = %d\n", swapsize);

	return swapsize;
}

/*
// bit_file maybe exist at sd/emmc ( testvx_update_bit3 "/sd0:1/pl_led_key.bit" ) 
   or pc_host ( testvx_update_bit3 "pl_top.bit" )
*/
void testvx_update_bit3(char* bit_file)
{	
	UINT8 * pBuf8 = NULL;
	UINT32 * pBuf32 = NULL;
	
	int j = 0, byteCnt = 0;
	int sect_idx = 0, sect_size = 0;
	
	int i = 0, base_idx = 0;
	int bit_file_size  = 0;  /* only pl setup data size*/
	int bit_file_size2 = 0;  /* total file size*/

	int ret = 0;
	UINT8 fileName[64] = {0};
	int read_size = 0;
	int file_size = 0;

	int fd = 0;

	int tick_1 = 0, tick_2 = 0;

	/**/
	/* devc_init	*/
	/**/
	vxInit_Devc();

	/**/
	/* open pl_top.bit at sd card*/
	/**/
	pBuf8 = (UINT8 *)(DDR_BUF_ADDR_FOR_PL);	/* ddr-phy 0x20000000*/
	pBuf32 = (UINT32*)(&pBuf8[0]);
	read_size = DDR_BUF_SIZE_FOR_PL;	/* < 16M */
	
	/*sprintf((char*(&fileName[0]), "/sd0:0/%s", "pl_top.bit");	*/
	sprintf((char*)(&fileName[0]), "%s", bit_file);	

	printf("\nPS start open bit_file, pls waiting ... ... \n\n");
	
	tick_1 = tickGet();
	
	ret = 0;
	fd = fopen((char*)(&fileName[0]), "r");  /* read*/
	if (fd > 0)
	{
		ret = fread(pBuf8, 1, read_size, fd);
	}	
	if (ret == 0)
	{
		printf("> File(%s) read failed: (%d) \n\n", (char*)(&fileName[0]), ret);			
		return;
	}
	else
	{
		 bit_file_size2 = ret;                  
		printf("> File(%s) read ok: (%d)(0x%X) \n\n", (char*)(&fileName[0]), ret, ret);			
	}	
	
	tick_2 = tickGet();
	printf("\n file_open time: tick(%d) - (%d ms) \n\n", (tick_2-tick_1), (tick_2-tick_1)*1000/sysClkRateGet());


#if 1
	/*=========================================*/
	for (j=0; j<512; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf(" ... ... ... ... \n\n");
	for (j=(bit_file_size2-512); j<bit_file_size2; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf("\n ------------\n");	
	/*=========================================*/
#endif


	/*
	fpga PL bit-stream, del the header and adjust the bytes
	*/
	/**/
	/* find the head of data area*/
	/**/
	bit_file_size = pl_bitstream_parse(pBuf8);
	
	printf("pl bit file size: 0x%X\n\n", bit_file_size);

	/**/
	/* move data to idx_0*/
	/**/
	base_idx = bit_file_size2 - bit_file_size;
	for (i=0; i<bit_file_size; i++)
	{
		pBuf8[i] = pBuf8[base_idx + i];  /* move data for del the header of bitstream*/
	}

	/**/
	/* flush cache, it's must be for ddr flush new data*/
	/**/
	(void)cacheInvalidate (DATA_CACHE, DDR_BUF_ADDR_FOR_PL, DDR_BUF_SIZE_FOR_PL); 

	
#if 1
	/*=========================================*/
	for (j=0; j<512; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf(" ... ... ... ... \n\n");	
	for (j=(bit_file_size-512); j<bit_file_size; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf("\n-----byte seq-------\n\n");
	/*=========================================*/
#endif

	printf("\n\n pBuf32[0]: 0x%08X len:0x%08X \n\n", &pBuf32[0], (bit_file_size/4));

	tick_1 = tickGet();
	
	/**/
	/* update bitstream to PL*/
	/**/
	devC_Update_PL(g_pDevC, (UINT32)(&pBuf32[0]), (bit_file_size/4));
	
	tick_2 = tickGet();
	printf("\n pl loadtime: tick(%d) - (%d ms) \n", (tick_2-tick_1), (tick_2-tick_1)*1000/sysClkRateGet());

	/*
	bitstream_checksum	0x0e0	bitstream_checksum
	csu_error_code_reg	0x0e4	淇濈暀
			csu_error_code_reg
	*/
	printf("\n\n");
	printf("bitstream_checksum: 0x%08X \n", *((UINT32*)(0xE0040000+0xE0)));	
	printf("csu_error_code_reg: 0x%08X \n", *((UINT32*)(0xE0040000+0xE4)));
	printf("\n\n");
	
	return;
}


/* test_note_&_log*/
/*
->testvx_update_bit3 "/sd0:1/pl_led_key.bit"

->testvx_update_bit3 "/sd0:1/pl_test.bit"
*/
#endif


#if 1  /* update pl: with bit in ddr, download to ddr by uboot */

/*
cmd note:
=========
1. in uboot shell:
	fmsh> tftpboot 0x20000000 pl_top.bit  
	get the download bit_file size, such as: 0xcb451f, Bytes transferred = 13321503 (cb451f hex)

2. boot vxworks

3. call vx_cmd:
	->testvx_update_bit4 0xcb451f
*/

void testvx_update_bit4(int bitfile_size)
{	
	UINT8 * pBuf8 = NULL;
	UINT32 * pBuf32 = NULL;
	
	int j = 0, byteCnt = 0;
	int sect_idx = 0, sect_size = 0;
	
	int i = 0, base_idx = 0;
	int bit_file_size  = 0;  /* only pl setup data size*/
	int bit_file_size2 = 0;  /* total file size*/

	int ret = 0;
	UINT8 fileName[64] = {0};
	int read_size = 0;
	int file_size = 0;

	int fd = 0;

	int tick_1 = 0, tick_2 = 0;

	/**/
	/* devc_init	*/
	/**/
	vxInit_Devc();

	/**/
	/* open pl_top.bit at sd card*/
	/**/
	pBuf8 = (UINT8 *)(DDR_BUF_ADDR_FOR_PL);	/* ddr-phy 0x20000000*/
	pBuf32 = (UINT32*)(&pBuf8[0]);
	read_size = DDR_BUF_SIZE_FOR_PL;	/* < 16M */
/*	
	//sprintf((char*)(&fileName[0]), "/sd0:0/%s", "pl_top.bit");	
	sprintf((char*)(&fileName[0]), "%s", bit_file);	
	
	tick_1 = tickGet();
	
	ret = 0;
	fd = fopen((char*)(&fileName[0]), "r");  // read
	if (fd > 0)
	{
		ret = fread(pBuf8, 1, read_size, fd);
	}	
	if (ret == 0)
	{
		printf("> File(%s) read failed: (%d) \n\n", (char*)(&fileName[0]), ret);			
		return;
	}
	else
	{
		 bit_file_size2 = ret;                  
		printf("> File(%s) read ok: (%d)(0x%X) \n\n", (char*)(&fileName[0]), ret, ret);			
	}	
	
	tick_2 = tickGet();
	printf("\n file_open time: tick(%d) - (%d ms) \n\n", (tick_2-tick_1), (tick_2-tick_1)*1000/sysClkRateGet());
*/
	bit_file_size2 = bitfile_size;

#if 1
	/*=========================================*/
	for (j=0; j<512; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf(" ... ... ... ... \n\n");
	for (j=(bit_file_size2-512); j<bit_file_size2; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf("\n ------------\n");	
	/*=========================================*/
#endif


	/*
	fpga PL bit-stream, del the header and adjust the bytes
	*/
	/**/
	/* find the head of data area*/
	/**/
	bit_file_size = pl_bitstream_parse(pBuf8);
	
	printf("pl bit file size: 0x%X\n\n", bit_file_size);

	/**/
	/* move data to idx_0*/
	/**/
	base_idx = bit_file_size2 - bit_file_size;
	for (i=0; i<bit_file_size; i++)
	{
		pBuf8[i] = pBuf8[base_idx + i];  /* move data for del the header of bitstream*/
	}

	/**/
	/* flush cache, it's must be for ddr flush new data*/
	/**/
	(void)cacheInvalidate (DATA_CACHE, DDR_BUF_ADDR_FOR_PL, DDR_BUF_SIZE_FOR_PL); 

	
#if 1
	/*=========================================*/
	for (j=0; j<512; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf(" ... ... ... ... \n\n");	
	for (j=(bit_file_size-512); j<bit_file_size; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf("\n-----byte seq-------\n\n");
	/*=========================================*/
#endif

	printf("\n\n pBuf32[0]: 0x%08X len:0x%08X \n\n", &pBuf32[0], (bit_file_size/4));

	tick_1 = tickGet();
	
	/**/
	/* update bitstream to PL*/
	/**/
	devC_Update_PL(g_pDevC, (UINT32)(&pBuf32[0]), (bit_file_size/4));
	
	tick_2 = tickGet();
	printf("\n pl loadtime: tick(%d) - (%d ms) \n", (tick_2-tick_1), (tick_2-tick_1)*1000/sysClkRateGet());

	/*
	bitstream_checksum	0x0e0	bitstream_checksum
	csu_error_code_reg	0x0e4	淇濈暀
			csu_error_code_reg
	*/
	printf("\n\n");
	printf("bitstream_checksum: 0x%08X \n", *((UINT32*)(0xE0040000+0xE0)));	
	printf("csu_error_code_reg: 0x%08X \n", *((UINT32*)(0xE0040000+0xE4)));
	printf("\n\n");
	
	return;
}

/*
log:
====

fmsh> setenv -f ethaddr 00:01:02:03:04:05
fmsh> setenv serverip 192.168.46.154
fmsh> setenv ipaddr 192.168.40.77
fmsh> ping 192.168.40.41
Speed: 100, full duplex
Using ethernet@e0047000 device
host 192.168.40.41 is alive

fmsh> tftpboot 0x20000000 pl_top.bit

Speed: 100, full duplex
Using ethernet@e0047000 device
TFTP from server 192.168.46.154; our IP address is 192.168.40.77
Filename 'pl_top.bit'.
Load address: 0x20000000
Loading: #################################################################
         #################################################################
         ###############################################################
         2.1 MiB/s
done
Bytes transferred = 13321503 (cb451f hex)

fmsh> tftpboot 0x00200000  vxWorks.bin

Speed: 100, full duplex
Using ethernet@e0047000 device
TFTP from server 192.168.46.154; our IP address is 192.168.40.77
Filename 'vxWorks.bin'.
Load address: 0x200000
Loading: #################################################################
         #################################################################
         ###########################
         1.9 MiB/s
done
Bytes transferred = 2302100 (232094 hex)
fmsh> go  0x00200000
## Starting application at 0x00200000 ...

Early uart init: Mar 25 2021 (10:52:20) 
... ...


-> testvx_update_bit4  0xcb451f

00090FF0 0FF00FF0 0FF00000 01610025  
746F703B 55736572 49443D30 58464646  
46464646 463B5665 7273696F 6E3D3230  
31362E34 0062000C 377A3034 35666667  
39303000 63000B32 3031392F 31322F32  
33006400 0931313A 32333A34 32006500  
... ...
*/
#endif


#if 1
void testvx_open_bootbin(char* bit_file)
{	
	UINT8 * pBuf8 = NULL;
	UINT32 * pBuf32 = NULL;
	
	int j = 0, byteCnt = 0;
	int sect_idx = 0, sect_size = 0;
	
	int i = 0, base_idx = 0;
	int bit_file_size  = 0;  /* only pl setup data size*/
	int bit_file_size2 = 0;  /* total file size*/

	int ret = 0;
	UINT8 fileName[64] = {0};
	int read_size = 0;
	int file_size = 0;

	int fd = 0;

	int tick_1 = 0, tick_2 = 0;

	int fsbl_len = 0;
	UINT32 tmp32 = 0;
	UINT8* pSrc8 = NULL;
	UINT8* pDst8 = NULL;

	/**/
	/* open boot.bin at sd card*/
	/**/
	pBuf8 = (UINT8 *)(DDR_BUF_ADDR_FOR_PL);	/* ddr-phy 0x20000000*/
	pBuf32 = (UINT32*)(&pBuf8[0]);
	read_size = DDR_BUF_SIZE_FOR_PL;	/* < 16M */
	
	sprintf((char*)(&fileName[0]), "%s", bit_file);	
	
	tick_1 = tickGet();
	
	ret = 0;
	fd = fopen((char*)(&fileName[0]), "r");  /* read*/
	if (fd > 0)
	{
		ret = fread(pBuf8, 1, read_size, fd);
	}	
	if (ret == 0)
	{
		printf("> File(%s) read failed: (%d) \n\n", (char*)(&fileName[0]), ret);			
		return;
	}
	else
	{
		 bit_file_size2 = ret;                  
		printf("> File(%s) read ok: (%d)(0x%X) \n\n", (char*)(&fileName[0]), ret, ret);			
	}	
	
	tick_2 = tickGet();
	printf("\n file_open time: tick(%d) - (%d ms) \n\n", (tick_2-tick_1), (tick_2-tick_1)*1000/sysClkRateGet());


#if 1
	/*=========================================*/
	for (j=0; j<512; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf(" ... ... ... ... \n\n");
	for (j=(bit_file_size2-512); j<bit_file_size2; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf("\n ------------\n");	
	/*=========================================*/
#endif

#if 1  /* copy fsbl.bin (of boot.bin) to ocm: 0x2_0000*/

	/*
	鍋忕Щ0x24:
	--------
	bit17~16:	Partition Owner 
		0 - FSBL
		1 - UBOOT
		2鎴� 鈥�淇濈暀
	*/
	base_idx = 0x980 + 0x24;
	tmp32 = (UINT32)((pBuf8[base_idx] << 24) | (pBuf8[base_idx+1] << 16) | (pBuf8[base_idx+2] << 8) | (pBuf8[base_idx+3]));
	printf("0x980 + 0x24: 0x%X \n", tmp32);
	if ((tmp32 & 0x00030000) == 0x0)
	{
		printf("copy fsbl.bin(of boot.bin) to ocm: 0x2_0000 \n");
	}
	else if ((tmp32 & 0x00030000) == 0x00010000)
	{
		printf("copy uboot.bin(of boot.bin) to ddr: 0x0400_000 \n");
	}


	/*
	0x03C 	FSBL image length 	FSBL 鍘熷闀垮害.
	*/	
	base_idx = 0x3C;
	fsbl_len = (int)((pBuf8[base_idx+3] << 24) | (pBuf8[base_idx+2] << 16) | (pBuf8[base_idx+1] << 8) | (pBuf8[base_idx]));
	printf("fsbl_len: 0x%X \n", fsbl_len);
	
	/*
	0x030 	Source offset 	FSBL Partition璧峰鍦板潃: (0x0A40-len: 0x01dc6c) offset in boot.bin
	*/
	pSrc8 = (UINT8*)(&pBuf8[0x0A40]);	
	pDst8 = (UINT8*)(0x20000);
	
	for (i=0; i<fsbl_len; i++)
	{
		pDst8[i] = pSrc8[i];
	}
	
	printf("copy fsbl finish!!! \n\n");
#endif

#if 1  /* copy uboot.bin (of boot.bin) to ddr: 0x0400_000*/

	/*
	鍋忕Щ0x24:
	--------
	bit17~16:	Partition Owner 
		0 - FSBL
		1 - UBOOT
		2鎴� 鈥�淇濈暀
	*/
	base_idx = 0x9C0 + 0x24;
	tmp32 = (UINT32)((pBuf8[base_idx] << 24) | (pBuf8[base_idx+1] << 16) | (pBuf8[base_idx+2] << 8) | (pBuf8[base_idx+3]));
	printf("0x9C0 + 0x24: 0x%X \n", tmp32);
	if ((tmp32 & 0x00030000) == 0x0)
	{
		printf("copy fsbl.bin(of boot.bin) to ocm: 0x2_0000 \n");
	}
	else if ((tmp32 & 0x00030000) == 0x00010000)
	{
		printf("copy uboot.bin(of boot.bin) to ddr: 0x0400_000 \n");
	}

	/*
	00x10 	Destination Execution Address LO 	璇artition琚獿oad鍚庣殑鎵ц浣庡湴鍧�0x0400_0000: ddr)
	*/
	base_idx = 0x9C0 + 0x10;
	tmp32 = (UINT32)((pBuf8[base_idx+3] << 24) | (pBuf8[base_idx+2] << 16) | (pBuf8[base_idx+1] << 8) | (pBuf8[base_idx]));
	pDst8 = (UINT8*)(tmp32);  /* ddr*/
	printf("dst_addr: 0x%X \n", tmp32);

	/*
	0x20 	Actual Partition Word Offset 	璇artition鍋忕Щ浣嶇疆
	*/
	base_idx = 0x9C0 + 0x20;
	tmp32 = (UINT32)((pBuf8[base_idx+3] << 24) | (pBuf8[base_idx+2] << 16) | (pBuf8[base_idx+1] << 8) | (pBuf8[base_idx])) * 4;	
	pSrc8 = (UINT8*)(&pBuf8[tmp32]);	/* offset of boot.bin*/
	printf("src_addr: 0x%X \n", tmp32);


	/*
	0x00 	Partition Data Word Length (x4) 	Partition鐨刉ord闀垮害锛堝惈鍔犲瘑锛�
	*/	
	base_idx = 0x9C0 + 0x00;;
	fsbl_len = (int)((pBuf8[base_idx+3] << 24) | (pBuf8[base_idx+2] << 16) | (pBuf8[base_idx+1] << 8) | (pBuf8[base_idx])) * 4;
	printf("fsbl_len: 0x%X \n", fsbl_len);
	for (i=0; i<fsbl_len; i++)
	{
		pDst8[i] = pSrc8[i];
	}
	
	printf("copy uboot finish!!! \n\n");
#endif

	return;
}

#endif




/*
part bit_stream update
*/
#if 1

void testvx_update_bit_part(char* bit_file)
{	
	UINT8 * pBuf8 = NULL;
	UINT32 * pBuf32 = NULL;
	
	int j = 0, byteCnt = 0;
	int sect_idx = 0, sect_size = 0;
	
	int i = 0, base_idx = 0;
	int bit_file_size  = 0;  /* only pl setup data size*/
	int bit_file_size2 = 0;  /* total file size*/

	int ret = 0;
	UINT8 fileName[64] = {0};
	int read_size = 0;
	int file_size = 0;

	int fd = 0;

	int tick_1 = 0, tick_2 = 0;

	/**/
	/* devc_init	*/
	/**/
	vxInit_Devc();

	/**/
	/* open pl_top.bit at sd card*/
	/**/
	pBuf8 = (UINT8 *)(DDR_BUF_ADDR_FOR_PL);	/* ddr-phy 0x20000000*/
	pBuf32 = (UINT32*)(&pBuf8[0]);
	read_size = DDR_BUF_SIZE_FOR_PL;	/* < 16M */
	
	/*sprintf((char*(&fileName[0]), "/sd0:0/%s", "pl_top.bit");	*/
	sprintf((char*)(&fileName[0]), "%s", bit_file);	

	printf("\nPS start open bit_file, pls waiting ... ... \n\n");
	
	tick_1 = tickGet();
	
	ret = 0;
	fd = fopen((char*)(&fileName[0]), "r");  /* read*/
	if (fd > 0)
	{
		ret = fread(pBuf8, 1, read_size, fd);
	}	
	if (ret == 0)
	{
		printf("> File(%s) read failed: (%d) \n\n", (char*)(&fileName[0]), ret);			
		return;
	}
	else
	{
		 bit_file_size2 = ret;                  
		printf("> File(%s) read ok: (%d)(0x%X) \n\n", (char*)(&fileName[0]), ret, ret);			
	}	
	
	tick_2 = tickGet();
	printf("\n file_open time: tick(%d) - (%d ms) \n\n", (tick_2-tick_1), (tick_2-tick_1)*1000/sysClkRateGet());


#if 1
	/*=========================================*/
	for (j=0; j<512; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf(" ... ... ... ... \n\n");
	for (j=(bit_file_size2-512); j<bit_file_size2; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf("\n ------------\n");	
	/*=========================================*/
#endif


	/*
	fpga PL bit-stream, del the header and adjust the bytes
	*/
	/**/
	/* find the head of data area*/
	/**/
	bit_file_size = pl_bitstream_parse(pBuf8);
	
	printf("pl bit file size: 0x%X\n\n", bit_file_size);

	/**/
	/* move data to idx_0*/
	/**/
	base_idx = bit_file_size2 - bit_file_size;
	for (i=0; i<bit_file_size; i++)
	{
		pBuf8[i] = pBuf8[base_idx + i];  /* move data for del the header of bitstream*/
	}

	/**/
	/* flush cache, it's must be for ddr flush new data*/
	/**/
	(void)cacheInvalidate (DATA_CACHE, DDR_BUF_ADDR_FOR_PL, DDR_BUF_SIZE_FOR_PL); 

	
#if 1
	/*=========================================*/
	for (j=0; j<512; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf(" ... ... ... ... \n\n");	
	for (j=(bit_file_size-512); j<bit_file_size; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf("\n-----byte seq-------\n\n");
	/*=========================================*/
#endif

	printf("\n\n pBuf32[0]: 0x%08X len:0x%08X \n\n", &pBuf32[0], (bit_file_size/4));

	tick_1 = tickGet();
	
	/**/
	/* update bitstream to PL*/
	/**/
	devC_Update_PL_part(g_pDevC, (UINT32)(&pBuf32[0]), (bit_file_size/4));
	
	tick_2 = tickGet();
	printf("\n pl loadtime: tick(%d) - (%d ms) \n", (tick_2-tick_1), (tick_2-tick_1)*1000/sysClkRateGet());

	/*
	bitstream_checksum	0x0e0	bitstream_checksum
	csu_error_code_reg	0x0e4	淇濈暀
			csu_error_code_reg
	*/
	printf("\n\n");
	printf("bitstream_checksum: 0x%08X \n", *((UINT32*)(0xE0040000+0xE0)));	
	printf("csu_error_code_reg: 0x%08X \n", *((UINT32*)(0xE0040000+0xE4)));
	printf("\n\n");
	
	return;
}

/*
pl_bit_file in fold: pl/pl_part_update/

=>testvx_update_bit3 "test_pl_part_all.bit"  // whole - bit_stream, normal, D10(fmql7045_demo) flush fast 
=>testvx_update_bit_part "test_pl_part_1.bit"  // part - bit_stream, D10(fmql7045_demo) flush slow
=>testvx_update_bit_part "test_pl_part_2.bit"  // part - bit_stream, D10(fmql7045_demo) flush fast
*/

#endif


