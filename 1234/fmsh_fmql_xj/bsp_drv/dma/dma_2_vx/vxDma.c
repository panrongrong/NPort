/* vxDma.c - fmsh 7010/7045 Dma driver */

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
01a, 07Jan20, jc  written.
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

#include "vxDma.h"

/*
defines 
*/
#if 1

#undef VX_DBG
#undef VX_DBG2

#define DMA_DBG

#ifdef DMA_DBG

#define VX_DBG(string, a, b, c, d, e, f)                    \
        if (_func_logMsg != NULL)                           \
           printf(string, a, b, c, d, e, f)  /* (* _func_logMsg)(string, a, b, c, d, e, f) */
#else
#define VX_DBG(string, a, b, c, d, e, f)
#endif

/* ret info log */
#define VX_DBG2(string, a, b, c, d, e, f) printf(string, a, b, c, d, e, f)
#endif


/* 
pDma: 
*/
static vxT_DMA_CTRL vxDma_Ctrl = {0};
static vxT_DMA vx_pDma = {0};
vxT_DMA * g_pDma = (vxT_DMA *)(&vx_pDma.dma_x);


#if 1


void dmaCtrl_Wr_CfgReg32(vxT_DMA* pDma, UINT32 offset, UINT32 value)
{
	UINT32 tmp32 = pDma->pDmaCtrl->cfgBaseAddr;
	FMQL_WRITE_32((tmp32 + offset), value);
	return;
}

UINT32 dmaCtrl_Rd_CfgReg32(vxT_DMA* pDma, UINT32 offset)
{
	UINT32 tmp32 = pDma->pDmaCtrl->cfgBaseAddr;
	return FMQL_READ_32(tmp32 + offset);
}

int dmaCtrl_Get_PartParams(vxT_DMA* pDma, int chn)
{
	vxT_DMA_PARAMS * pDmaParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);

	UINT32 regVal = 0, tmp32 = 0;
	UINT32 baseAddr = 0;
	UINT32 chn_max_blk_size = 0; 
	UINT32 chn_multi_blk_type = 0; 	
	UINT32 comp_param_offset = 0;
	
	switch (chn)
	{
	case 0:
		chn_max_blk_size = DMAC_PARAM_CH0_MAX_BLK_SIZE;
		chn_multi_blk_type = DMAC_PARAM_CH0_MULTI_BLK_TYPE;
		comp_param_offset = DMAC_DMA_COMP_PARAMS_2_L_OFFSET;
		break;
	case 1:
		chn_max_blk_size = DMAC_PARAM_CH1_MAX_BLK_SIZE;
		chn_multi_blk_type = DMAC_PARAM_CH1_MULTI_BLK_TYPE;
		comp_param_offset = DMAC_DMA_COMP_PARAMS_3_H_OFFSET;
		break;
	case 2:
		chn_max_blk_size = DMAC_PARAM_CH2_MAX_BLK_SIZE;
		chn_multi_blk_type = DMAC_PARAM_CH2_MULTI_BLK_TYPE;
		comp_param_offset = DMAC_DMA_COMP_PARAMS_3_L_OFFSET;
		break;
	case 3:
		chn_max_blk_size = DMAC_PARAM_CH3_MAX_BLK_SIZE;
		chn_multi_blk_type = DMAC_PARAM_CH3_MULTI_BLK_TYPE;
		comp_param_offset = DMAC_DMA_COMP_PARAMS_4_H_OFFSET;
		break;
	case 4:
		chn_max_blk_size = DMAC_PARAM_CH4_MAX_BLK_SIZE;
		chn_multi_blk_type = DMAC_PARAM_CH4_MULTI_BLK_TYPE;
		comp_param_offset = DMAC_DMA_COMP_PARAMS_4_L_OFFSET;
		break;
	case 5:
		chn_max_blk_size = DMAC_PARAM_CH5_MAX_BLK_SIZE;
		chn_multi_blk_type = DMAC_PARAM_CH5_MULTI_BLK_TYPE;
		comp_param_offset = DMAC_DMA_COMP_PARAMS_5_H_OFFSET;
		break;
	case 6:
		chn_max_blk_size = DMAC_PARAM_CH6_MAX_BLK_SIZE;
		chn_multi_blk_type = DMAC_PARAM_CH6_MULTI_BLK_TYPE;
		comp_param_offset = DMAC_DMA_COMP_PARAMS_5_L_OFFSET;
		break;
	case 7:
		chn_max_blk_size = DMAC_PARAM_CH7_MAX_BLK_SIZE;
		chn_multi_blk_type = DMAC_PARAM_CH7_MULTI_BLK_TYPE;
		comp_param_offset = DMAC_DMA_COMP_PARAMS_6_H_OFFSET;
		break;
	}

#if 1
	regVal = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_1_L_OFFSET);
	tmp32 = (regVal & chn_max_blk_size) >> (4*chn);
	switch (tmp32)
	{
		case 0	: pDmaParams->ch_max_blk_size[chn] =    3; break;
		case 1	: pDmaParams->ch_max_blk_size[chn] =    7; break;
		case 2	: pDmaParams->ch_max_blk_size[chn] =   15; break;
		case 3	: pDmaParams->ch_max_blk_size[chn] =   31; break;
		case 4	: pDmaParams->ch_max_blk_size[chn] =   63; break;
		case 5	: pDmaParams->ch_max_blk_size[chn] =  127; break;
		case 6	: pDmaParams->ch_max_blk_size[chn] =  255; break;
		case 7	: pDmaParams->ch_max_blk_size[chn] =  511; break;
		case 8	: pDmaParams->ch_max_blk_size[chn] = 1023; break;
		case 9	: pDmaParams->ch_max_blk_size[chn] = 2047; break;
		case 10 : pDmaParams->ch_max_blk_size[chn] = 4095; break;
	}
	
	regVal = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_2_H_OFFSET);
	pDmaParams->ch_multi_blk_type[chn] = (regVal & chn_multi_blk_type) >> (4*chn);
	
	regVal = dmaCtrl_Rd_CfgReg32(pDma, comp_param_offset);
	tmp32 = regVal & DMAC_PARAM_CHX_DTW;
	switch (tmp32)
	{
		case 0 : pDmaParams->ch_dtw[chn] =   0; break;
		case 1 : pDmaParams->ch_dtw[chn] =   8; break;
		case 2 : pDmaParams->ch_dtw[chn] =  16; break;
		case 3 : pDmaParams->ch_dtw[chn] =  32; break;
		case 4 : pDmaParams->ch_dtw[chn] =  64; break;
		case 5 : pDmaParams->ch_dtw[chn] = 128; break;
		case 6 : pDmaParams->ch_dtw[chn] = 256; break;
	}
	
	tmp32 = (regVal & DMAC_PARAM_CHX_STW) >> 3;
	switch (tmp32)
	{
		case 0 : pDmaParams->ch_stw[chn] =   0; break;
		case 1 : pDmaParams->ch_stw[chn] =   8; break;
		case 2 : pDmaParams->ch_stw[chn] =  16; break;
		case 3 : pDmaParams->ch_stw[chn] =  32; break;
		case 4 : pDmaParams->ch_stw[chn] =  64; break;
		case 5 : pDmaParams->ch_stw[chn] = 128; break;
		case 6 : pDmaParams->ch_stw[chn] = 256; break;
	}
	
	pDmaParams->ch_stat_dst[chn]	 = (regVal & DMAC_PARAM_CHX_STAT_DST)     >> 6;
	pDmaParams->ch_stat_src[chn]	 = (regVal & DMAC_PARAM_CHX_STAT_SRC)     >> 7;
	pDmaParams->ch_dst_sca_en[chn]   = (regVal & DMAC_PARAM_CHX_DST_SCA_EN)   >> 8;
	pDmaParams->ch_src_gat_en[chn]   = (regVal & DMAC_PARAM_CHX_SRC_GAT_EN)   >> 9;
	pDmaParams->ch_lock_en[chn]	     = (regVal & DMAC_PARAM_CHX_LOCK_EN)      >> 10;
	pDmaParams->ch_multi_blk_en[chn] = (regVal & DMAC_PARAM_CHX_MULTI_BLK_EN) >> 11;
	pDmaParams->ch_ctl_wb_en[chn]    = (regVal & DMAC_PARAM_CHX_CTL_WB_EN)    >> 12;
	pDmaParams->ch_hc_llp[chn]	     = (regVal & DMAC_PARAM_CHX_HC_LLP)       >> 13;
	pDmaParams->ch_fc[chn]		     = (regVal & DMAC_PARAM_CHX_FC)           >> 14;
	
	tmp32 = (regVal & DMAC_PARAM_CHX_MAX_MULT_SIZE) >> 16;
	switch (tmp32)
	{
		case 0 : pDmaParams->ch_max_mult_size[chn] =	 4; break;
		case 1 : pDmaParams->ch_max_mult_size[chn] =	 8; break;
		case 2 : pDmaParams->ch_max_mult_size[chn] =	16; break;
		case 3 : pDmaParams->ch_max_mult_size[chn] =	32; break;
		case 4 : pDmaParams->ch_max_mult_size[chn] =	64; break;
		case 5 : pDmaParams->ch_max_mult_size[chn] = 128; break;
		case 6 : pDmaParams->ch_max_mult_size[chn] = 256; break;
	}
	
	pDmaParams->ch_dms[chn] = (regVal & DMAC_PARAM_CHX_DMS) >> 19;
	pDmaParams->ch_lms[chn] = (regVal & DMAC_PARAM_CHX_LMS) >> 22;
	pDmaParams->ch_sms[chn] = (regVal & DMAC_PARAM_CHX_SMS) >> 25;
	
	tmp32 = (regVal & DMAC_PARAM_CHX_FIFO_DEPTH) >> 28;
	switch(tmp32) 
	{
		case 0 : pDmaParams->ch_fifo_depth[chn] =   4; break;
		case 1 : pDmaParams->ch_fifo_depth[chn] =   8; break;
		case 2 : pDmaParams->ch_fifo_depth[chn] =  16; break;
		case 3 : pDmaParams->ch_fifo_depth[chn] =  32; break;
		case 4 : pDmaParams->ch_fifo_depth[chn] =  64; break;
		case 5 : pDmaParams->ch_fifo_depth[chn] = 128; break;
		case 6 : pDmaParams->ch_fifo_depth[chn] = 256; break;
	}
#endif

	return 0;
}
/*****************************************************************************
*
* @description
* This function attempts to automatically discover the hardware
* component parameters.
* This is usually controlled by the ADD_ENCODED_PARAMS coreConsultant
* parameter.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Get_DefaultParams(vxT_DMA* pDma)
{
	vxT_DMA_PARAMS * pDmaParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int ret = 0;
	
    UINT32 regVal, tmp32;
	UINT32 baseAddr;
	UINT32 compVersion;

	int chn = 0;

	baseAddr = pDma->pDmaCtrl->cfgBaseAddr;

	/**/
    /* Only version 2.02a and greater support hardware parameter*/
    /**/
    /* registers.*/
    /**/
    compVersion = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_ID_REG_H_OFFSET);
	
    if ((dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_ID_REG_L_OFFSET) == FMSH_ahb_dmac) \
		&& (compVersion >= 0x3230322A))
	{
        pDma->pDmaCtrl->devIP_ver = compVersion;

		regVal = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_1_H_OFFSET);
		
		pDmaParams->num_master_int = ((regVal & DMAC_PARAM_NUM_MASTER_INT) >> 11) + 0x1;
		pDmaParams->num_channels   = ((regVal & DMAC_PARAM_NUM_CHANNELS)   >> 8) + 0x1;
		pDmaParams->num_hs_int     =  (regVal & DMAC_PARAM_NUM_HS_INT)     >> 23;
		pDmaParams->intr_io        =  (regVal & DMAC_PARAM_INTR_IO)        >> 1;
		pDmaParams->mabrst         =  (regVal & DMAC_PARAM_MABRST)         >> 3;
		pDmaParams->big_endian     =  (regVal & DMAC_PARAM_BIG_ENDIAN);
		
        tmp32 = (regVal & DMAC_PARAM_S_HDATA_WIDTH) >> 13;
        switch (tmp32)
		{
            case 0 : pDmaParams->s_hdata_width =  32; break;
            case 1 : pDmaParams->s_hdata_width =  64; break;
            case 2 : pDmaParams->s_hdata_width = 128; break;
            case 3 : pDmaParams->s_hdata_width = 256; break;
        }
		
        tmp32 = (regVal & DMAC_PARAM_M1_HDATA_WIDTH) >> 15;
        switch (tmp32)
		{
            case 0 : pDmaParams->m_hdata_width[0] =  32; break;
            case 1 : pDmaParams->m_hdata_width[0] =  64; break;
            case 2 : pDmaParams->m_hdata_width[0] = 128; break;
            case 3 : pDmaParams->m_hdata_width[0] = 256; break;
        }
		
        tmp32 = (regVal & DMAC_PARAM_M2_HDATA_WIDTH) >> 17;
        switch (tmp32)
		{
            case 0 : pDmaParams->m_hdata_width[1] =  32; break;
            case 1 : pDmaParams->m_hdata_width[1] =  64; break;
            case 2 : pDmaParams->m_hdata_width[1] = 128; break;
            case 3 : pDmaParams->m_hdata_width[1] = 256; break;
        }
		
        tmp32 = (regVal & DMAC_PARAM_M3_HDATA_WIDTH) >> 19;
        switch (tmp32)
		{
            case 0 : pDmaParams->m_hdata_width[2] =  32; break;
            case 1 : pDmaParams->m_hdata_width[2] =  64; break;
            case 2 : pDmaParams->m_hdata_width[2] = 128; break;
            case 3 : pDmaParams->m_hdata_width[2] = 256; break;
        }
		
        tmp32 = (regVal & DMAC_PARAM_M4_HDATA_WIDTH) >> 21;
        switch (tmp32)
		{
            case 0 : pDmaParams->m_hdata_width[3] =  32; break;
            case 1 : pDmaParams->m_hdata_width[3] =  64; break;
            case 2 : pDmaParams->m_hdata_width[3] = 128; break;
            case 3 : pDmaParams->m_hdata_width[3] = 256; break;
        }
		
        pDmaParams->encoded_params = (regVal & DMAC_PARAM_ADD_ENCODED_PARAMS) >> 28;
        pDmaParams->static_endian_select = (regVal & DMAC_PARAM_STATIC_ENDIAN_SELECT) >> 29;

        /* 
        Channel 0 
		*/
		chn = 0;
		dmaCtrl_Get_PartParams(pDma, chn);
		
        /* 
        Channel 1 
		*/
        if (pDmaParams->num_channels >= 2)
		{
			chn = 1;
			dmaCtrl_Get_PartParams(pDma, chn);
        }
	
        /* 
        Channel 2 
		*/
        if (pDmaParams->num_channels >= 3)
		{
			chn = 2;
			dmaCtrl_Get_PartParams(pDma, chn);
        }

        /*
        Channel 3 
        */
        if (pDmaParams->num_channels >= 4) 
		{
			chn = 3;
			dmaCtrl_Get_PartParams(pDma, chn);
        }

        /* 
        Channel 4 
        */
        if (pDmaParams->num_channels >= 5) 
		{		
			chn = 4;
			dmaCtrl_Get_PartParams(pDma, chn);
        }

        /* 
        Channel 5 
        */
        if (pDmaParams->num_channels >= 6) 
		{		
			chn = 5;
			dmaCtrl_Get_PartParams(pDma, chn);
        }

        /* 
        Channel 6 
		*/
        if (pDmaParams->num_channels >= 7) 
		{		
			chn = 6;
			dmaCtrl_Get_PartParams(pDma, chn);
        }

        /* 
        Channel 7 
		*/
        if (pDmaParams->num_channels == 8) 
		{		
			chn = 7;
			dmaCtrl_Get_PartParams(pDma, chn);
        }
    }
    else
	{
        ret = -FMSH_ENOSYS;
    }
	
    return ret;
}

int dmaCtrl_Get_DefaultParams2(vxT_DMA* pDma)
{
	vxT_DMA_PARAMS * pDmaParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int ret = FMSH_SUCCESS;
	
    UINT32 reg, tmp;
	UINT32 baseAddr;
	UINT32 compVersion;
	UINT32 DmaIpID = 0;

	int chn = 0;

	baseAddr = pDma->pDmaCtrl->cfgBaseAddr;
	
    /* Only version 2.02a and greater support hardware parameter*/
    /* registers.*/
    compVersion = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_ID_REG_H_OFFSET);
	DmaIpID = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_ID_REG_L_OFFSET);
	
    if ((DmaIpID == FMSH_ahb_dmac) && (compVersion >= 0x3230322A))
	{
		printf("DmaIpID(0x44571110):0x%X \n", DmaIpID);
		printf("compVersion:0x%X>=0x3230322A \n", compVersion);
		
        pDma->pDmaCtrl->devIP_ver = compVersion;

		reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_1_H_OFFSET);
		
		pDmaParams->num_master_int = ((reg & DMAC_PARAM_NUM_MASTER_INT) >> 11) + 0x1;
		pDmaParams->num_channels   = ((reg & DMAC_PARAM_NUM_CHANNELS)   >> 8) + 0x1;
		pDmaParams->num_hs_int     =  (reg & DMAC_PARAM_NUM_HS_INT)     >> 23;
		pDmaParams->intr_io        =  (reg & DMAC_PARAM_INTR_IO)        >> 1;
		pDmaParams->mabrst         =  (reg & DMAC_PARAM_MABRST)         >> 3;
		pDmaParams->big_endian     =  (reg & DMAC_PARAM_BIG_ENDIAN);
		
        tmp = (reg & DMAC_PARAM_S_HDATA_WIDTH) >> 13;
        switch (tmp)
		{
            case 0 : pDmaParams->s_hdata_width =  32; break;
            case 1 : pDmaParams->s_hdata_width =  64; break;
            case 2 : pDmaParams->s_hdata_width = 128; break;
            case 3 : pDmaParams->s_hdata_width = 256; break;
        }
		
        tmp = (reg & DMAC_PARAM_M1_HDATA_WIDTH) >> 15;
        switch (tmp)
		{
            case 0 : pDmaParams->m_hdata_width[0] =  32; break;
            case 1 : pDmaParams->m_hdata_width[0] =  64; break;
            case 2 : pDmaParams->m_hdata_width[0] = 128; break;
            case 3 : pDmaParams->m_hdata_width[0] = 256; break;
        }
		
        tmp = (reg & DMAC_PARAM_M2_HDATA_WIDTH) >> 17;
        switch (tmp)
		{
            case 0 : pDmaParams->m_hdata_width[1] =  32; break;
            case 1 : pDmaParams->m_hdata_width[1] =  64; break;
            case 2 : pDmaParams->m_hdata_width[1] = 128; break;
            case 3 : pDmaParams->m_hdata_width[1] = 256; break;
        }
		
        tmp = (reg & DMAC_PARAM_M3_HDATA_WIDTH) >> 19;
        switch (tmp)
		{
            case 0 : pDmaParams->m_hdata_width[2] =  32; break;
            case 1 : pDmaParams->m_hdata_width[2] =  64; break;
            case 2 : pDmaParams->m_hdata_width[2] = 128; break;
            case 3 : pDmaParams->m_hdata_width[2] = 256; break;
        }
		
        tmp = (reg & DMAC_PARAM_M4_HDATA_WIDTH) >> 21;
        switch (tmp)
		{
            case 0 : pDmaParams->m_hdata_width[3] =  32; break;
            case 1 : pDmaParams->m_hdata_width[3] =  64; break;
            case 2 : pDmaParams->m_hdata_width[3] = 128; break;
            case 3 : pDmaParams->m_hdata_width[3] = 256; break;
        }
		
        pDmaParams->encoded_params       = (reg & DMAC_PARAM_ADD_ENCODED_PARAMS)   >> 28;
        pDmaParams->static_endian_select = (reg & DMAC_PARAM_STATIC_ENDIAN_SELECT) >> 29;

        /* Channel 0 */
        reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_1_L_OFFSET);
		tmp = reg & DMAC_PARAM_CH0_MAX_BLK_SIZE;
        switch (tmp)
		{
            case 0  : pDmaParams->ch_max_blk_size[0] =    3; break;
            case 1  : pDmaParams->ch_max_blk_size[0] =    7; break;
            case 2  : pDmaParams->ch_max_blk_size[0] =   15; break;
            case 3  : pDmaParams->ch_max_blk_size[0] =   31; break;
            case 4  : pDmaParams->ch_max_blk_size[0] =   63; break;
            case 5  : pDmaParams->ch_max_blk_size[0] =  127; break;
            case 6  : pDmaParams->ch_max_blk_size[0] =  255; break;
            case 7  : pDmaParams->ch_max_blk_size[0] =  511; break;
            case 8  : pDmaParams->ch_max_blk_size[0] = 1023; break;
            case 9  : pDmaParams->ch_max_blk_size[0] = 2047; break;
            case 10 : pDmaParams->ch_max_blk_size[0] = 4095; break;
        }
		
		reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_2_H_OFFSET);
		pDmaParams->ch_multi_blk_type[0] = (reg & DMAC_PARAM_CH0_MULTI_BLK_TYPE);
		
		reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_2_L_OFFSET);
		tmp = reg & DMAC_PARAM_CHX_DTW;
        switch (tmp)
		{
            case 0 : pDmaParams->ch_dtw[0] =   0; break;
            case 1 : pDmaParams->ch_dtw[0] =   8; break;
            case 2 : pDmaParams->ch_dtw[0] =  16; break;
            case 3 : pDmaParams->ch_dtw[0] =  32; break;
            case 4 : pDmaParams->ch_dtw[0] =  64; break;
            case 5 : pDmaParams->ch_dtw[0] = 128; break;
            case 6 : pDmaParams->ch_dtw[0] = 256; break;
        }
		
		tmp = (reg & DMAC_PARAM_CHX_STW) >> 3;
        switch (tmp)
		{
            case 0 : pDmaParams->ch_stw[0] =   0; break;
            case 1 : pDmaParams->ch_stw[0] =   8; break;
            case 2 : pDmaParams->ch_stw[0] =  16; break;
            case 3 : pDmaParams->ch_stw[0] =  32; break;
            case 4 : pDmaParams->ch_stw[0] =  64; break;
            case 5 : pDmaParams->ch_stw[0] = 128; break;
            case 6 : pDmaParams->ch_stw[0] = 256; break;
        }
		
		pDmaParams->ch_stat_dst[0] = (reg & DMAC_PARAM_CHX_STAT_DST) >> 6;
		pDmaParams->ch_stat_src[0] = (reg & DMAC_PARAM_CHX_STAT_SRC) >> 7;
		pDmaParams->ch_dst_sca_en[0] = (reg & DMAC_PARAM_CHX_DST_SCA_EN) >> 8;
		pDmaParams->ch_src_gat_en[0] = (reg & DMAC_PARAM_CHX_SRC_GAT_EN) >> 9;
		pDmaParams->ch_lock_en[0] = (reg & DMAC_PARAM_CHX_LOCK_EN) >> 10;
		pDmaParams->ch_multi_blk_en[0] = (reg & DMAC_PARAM_CHX_MULTI_BLK_EN) >> 11;
		pDmaParams->ch_ctl_wb_en[0] = (reg & DMAC_PARAM_CHX_CTL_WB_EN) >> 12;
		pDmaParams->ch_hc_llp[0] = (reg & DMAC_PARAM_CHX_HC_LLP) >> 13;
		pDmaParams->ch_fc[0] = (reg & DMAC_PARAM_CHX_FC) >> 14;
		
		tmp = (reg & DMAC_PARAM_CHX_MAX_MULT_SIZE) >> 16;
        switch (tmp)
		{
            case 0 : pDmaParams->ch_max_mult_size[0] =   4; break;
            case 1 : pDmaParams->ch_max_mult_size[0] =   8; break;
            case 2 : pDmaParams->ch_max_mult_size[0] =  16; break;
            case 3 : pDmaParams->ch_max_mult_size[0] =  32; break;
            case 4 : pDmaParams->ch_max_mult_size[0] =  64; break;
            case 5 : pDmaParams->ch_max_mult_size[0] = 128; break;
            case 6 : pDmaParams->ch_max_mult_size[0] = 256; break;
        }
		
		pDmaParams->ch_dms[0] = (reg & DMAC_PARAM_CHX_DMS) >> 19;
		pDmaParams->ch_lms[0] = (reg & DMAC_PARAM_CHX_LMS) >> 22;
		pDmaParams->ch_sms[0] = (reg & DMAC_PARAM_CHX_SMS) >> 25;
		
		tmp = (reg & DMAC_PARAM_CHX_FIFO_DEPTH) >> 28;
        switch(tmp) {
            case 0 : pDmaParams->ch_fifo_depth[0] =   4; break;
            case 1 : pDmaParams->ch_fifo_depth[0] =   8; break;
            case 2 : pDmaParams->ch_fifo_depth[0] =  16; break;
            case 3 : pDmaParams->ch_fifo_depth[0] =  32; break;
            case 4 : pDmaParams->ch_fifo_depth[0] =  64; break;
            case 5 : pDmaParams->ch_fifo_depth[0] = 128; break;
            case 6 : pDmaParams->ch_fifo_depth[0] = 256; break;
        }
		
        /* Channel 1 */
        if (pDmaParams->num_channels >= 2)
		{
        	reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_1_L_OFFSET); 
			tmp = (reg & DMAC_PARAM_CH1_MAX_BLK_SIZE) >> 4;
            switch (tmp)
			{
                case 0  : pDmaParams->ch_max_blk_size[1] =    3; break;
                case 1  : pDmaParams->ch_max_blk_size[1] =    7; break;
                case 2  : pDmaParams->ch_max_blk_size[1] =   15; break;
                case 3  : pDmaParams->ch_max_blk_size[1] =   31; break;
                case 4  : pDmaParams->ch_max_blk_size[1] =   63; break;
                case 5  : pDmaParams->ch_max_blk_size[1] =  127; break;
                case 6  : pDmaParams->ch_max_blk_size[1] =  255; break;
                case 7  : pDmaParams->ch_max_blk_size[1] =  511; break;
                case 8  : pDmaParams->ch_max_blk_size[1] = 1023; break;
                case 9  : pDmaParams->ch_max_blk_size[1] = 2047; break;
                case 10 : pDmaParams->ch_max_blk_size[1] = 4095; break;
            }
			reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_2_H_OFFSET);
			pDmaParams->ch_multi_blk_type[1] = (reg & DMAC_PARAM_CH1_MULTI_BLK_TYPE) >> 4;
			
			reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_3_H_OFFSET);
			tmp = reg & DMAC_PARAM_CHX_DTW;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_dtw[1] =   0; break;
                case 1 : pDmaParams->ch_dtw[1] =   8; break;
                case 2 : pDmaParams->ch_dtw[1] =  16; break;
                case 3 : pDmaParams->ch_dtw[1] =  32; break;
                case 4 : pDmaParams->ch_dtw[1] =  64; break;
                case 5 : pDmaParams->ch_dtw[1] = 128; break;
                case 6 : pDmaParams->ch_dtw[1] = 256; break;
            }
			tmp = (reg & DMAC_PARAM_CHX_STW) >> 3;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_stw[1] =   0; break;
                case 1 : pDmaParams->ch_stw[1] =   8; break;
                case 2 : pDmaParams->ch_stw[1] =  16; break;
                case 3 : pDmaParams->ch_stw[1] =  32; break;
                case 4 : pDmaParams->ch_stw[1] =  64; break;
                case 5 : pDmaParams->ch_stw[1] = 128; break;
                case 6 : pDmaParams->ch_stw[1] = 256; break;
            }
			
			pDmaParams->ch_stat_dst[1] = (reg & DMAC_PARAM_CHX_STAT_DST) >> 6;
			pDmaParams->ch_stat_src[1] = (reg & DMAC_PARAM_CHX_STAT_SRC) >> 7;
			pDmaParams->ch_dst_sca_en[1] = (reg & DMAC_PARAM_CHX_DST_SCA_EN) >> 8;
			pDmaParams->ch_src_gat_en[1] = (reg & DMAC_PARAM_CHX_SRC_GAT_EN) >> 9;
			pDmaParams->ch_lock_en[1] = (reg & DMAC_PARAM_CHX_LOCK_EN) >> 10;
			pDmaParams->ch_multi_blk_en[1] = (reg & DMAC_PARAM_CHX_MULTI_BLK_EN) >> 11;
			pDmaParams->ch_ctl_wb_en[1] = (reg & DMAC_PARAM_CHX_CTL_WB_EN) >> 12;
			pDmaParams->ch_hc_llp[1] = (reg & DMAC_PARAM_CHX_HC_LLP) >> 13;
			pDmaParams->ch_fc[1] = (reg & DMAC_PARAM_CHX_FC) >> 14;
			
			tmp = (reg & DMAC_PARAM_CHX_MAX_MULT_SIZE) >> 16;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_max_mult_size[1] =   4; break;
                case 1 : pDmaParams->ch_max_mult_size[1] =   8; break;
                case 2 : pDmaParams->ch_max_mult_size[1] =  16; break;
                case 3 : pDmaParams->ch_max_mult_size[1] =  32; break;
                case 4 : pDmaParams->ch_max_mult_size[1] =  64; break;
                case 5 : pDmaParams->ch_max_mult_size[1] = 128; break;
                case 6 : pDmaParams->ch_max_mult_size[1] = 256; break;
            }
			
			pDmaParams->ch_dms[1] = (reg & DMAC_PARAM_CHX_DMS) >> 19;
			pDmaParams->ch_lms[1] = (reg & DMAC_PARAM_CHX_LMS) >> 22;
			pDmaParams->ch_sms[1] = (reg & DMAC_PARAM_CHX_SMS) >> 25;
			
			tmp = (reg & DMAC_PARAM_CHX_FIFO_DEPTH) >> 28;
            switch (tmp)
			{
	            case 0 : pDmaParams->ch_fifo_depth[1] =   4; break;
	            case 1 : pDmaParams->ch_fifo_depth[1] =   8; break;
	            case 2 : pDmaParams->ch_fifo_depth[1] =  16; break;
	            case 3 : pDmaParams->ch_fifo_depth[1] =  32; break;
	            case 4 : pDmaParams->ch_fifo_depth[1] =  64; break;
	            case 5 : pDmaParams->ch_fifo_depth[1] = 128; break;
	            case 6 : pDmaParams->ch_fifo_depth[1] = 256; break;
            }
        }

        /* Channel 2 */
        if(pDmaParams->num_channels >= 3)
		{
        	reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_1_L_OFFSET);
			tmp = (reg & DMAC_PARAM_CH2_MAX_BLK_SIZE) >> 8;
            switch (tmp)
			{
                case 0  : pDmaParams->ch_max_blk_size[2] =    3; break;
                case 1  : pDmaParams->ch_max_blk_size[2] =    7; break;
                case 2  : pDmaParams->ch_max_blk_size[2] =   15; break;
                case 3  : pDmaParams->ch_max_blk_size[2] =   31; break;
                case 4  : pDmaParams->ch_max_blk_size[2] =   63; break;
                case 5  : pDmaParams->ch_max_blk_size[2] =  127; break;
                case 6  : pDmaParams->ch_max_blk_size[2] =  255; break;
                case 7  : pDmaParams->ch_max_blk_size[2] =  511; break;
                case 8  : pDmaParams->ch_max_blk_size[2] = 1023; break;
                case 9  : pDmaParams->ch_max_blk_size[2] = 2047; break;
                case 10 : pDmaParams->ch_max_blk_size[2] = 4095; break;
            }
			reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_2_H_OFFSET);
			pDmaParams->ch_multi_blk_type[2] = (reg & DMAC_PARAM_CH2_MULTI_BLK_TYPE) >> 8;
			
			reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_3_L_OFFSET);
			tmp = reg & DMAC_PARAM_CHX_DTW;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_dtw[2] =   0; break;
                case 1 : pDmaParams->ch_dtw[2] =   8; break;
                case 2 : pDmaParams->ch_dtw[2] =  16; break;
                case 3 : pDmaParams->ch_dtw[2] =  32; break;
                case 4 : pDmaParams->ch_dtw[2] =  64; break;
                case 5 : pDmaParams->ch_dtw[2] = 128; break;
                case 6 : pDmaParams->ch_dtw[2] = 256; break;
            }
			tmp = (reg & DMAC_PARAM_CHX_STW) >> 3;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_stw[2] =   0; break;
                case 1 : pDmaParams->ch_stw[2] =   8; break;
                case 2 : pDmaParams->ch_stw[2] =  16; break;
                case 3 : pDmaParams->ch_stw[2] =  32; break;
                case 4 : pDmaParams->ch_stw[2] =  64; break;
                case 5 : pDmaParams->ch_stw[2] = 128; break;
                case 6 : pDmaParams->ch_stw[2] = 256; break;
            }
			
			pDmaParams->ch_stat_dst[2] = (reg & DMAC_PARAM_CHX_STAT_DST) >> 6;
			pDmaParams->ch_stat_src[2] = (reg & DMAC_PARAM_CHX_STAT_SRC) >> 7;
			pDmaParams->ch_dst_sca_en[2] = (reg & DMAC_PARAM_CHX_DST_SCA_EN) >> 8;
			pDmaParams->ch_src_gat_en[2] = (reg & DMAC_PARAM_CHX_SRC_GAT_EN) >>9;
			pDmaParams->ch_lock_en[2] = (reg & DMAC_PARAM_CHX_LOCK_EN) >> 10;
			pDmaParams->ch_multi_blk_en[2] = (reg & DMAC_PARAM_CHX_MULTI_BLK_EN) >> 11;
			pDmaParams->ch_ctl_wb_en[2] = (reg & DMAC_PARAM_CHX_CTL_WB_EN) >> 12;
			pDmaParams->ch_hc_llp[2] = (reg & DMAC_PARAM_CHX_HC_LLP) >> 13;
			pDmaParams->ch_fc[2] = (reg & DMAC_PARAM_CHX_FC) >> 14;
			
			tmp = (reg & DMAC_PARAM_CHX_MAX_MULT_SIZE) >> 16;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_max_mult_size[2] =   4; break;
                case 1 : pDmaParams->ch_max_mult_size[2] =   8; break;
                case 2 : pDmaParams->ch_max_mult_size[2] =  16; break;
                case 3 : pDmaParams->ch_max_mult_size[2] =  32; break;
                case 4 : pDmaParams->ch_max_mult_size[2] =  64; break;
                case 5 : pDmaParams->ch_max_mult_size[2] = 128; break;
                case 6 : pDmaParams->ch_max_mult_size[2] = 256; break;
            }
			
			pDmaParams->ch_dms[2] = (reg & DMAC_PARAM_CHX_DMS) >> 19;
			pDmaParams->ch_lms[2] = (reg & DMAC_PARAM_CHX_LMS) >> 22;
			pDmaParams->ch_sms[2] = (reg & DMAC_PARAM_CHX_SMS) >> 25;
			
			tmp = (reg & DMAC_PARAM_CHX_FIFO_DEPTH) >> 28;
            switch (tmp)
			{
	            case 0 : pDmaParams->ch_fifo_depth[2] =   4; break;
	            case 1 : pDmaParams->ch_fifo_depth[2] =   8; break;
	            case 2 : pDmaParams->ch_fifo_depth[2] =  16; break;
	            case 3 : pDmaParams->ch_fifo_depth[2] =  32; break;
	            case 4 : pDmaParams->ch_fifo_depth[2] =  64; break;
	            case 5 : pDmaParams->ch_fifo_depth[2] = 128; break;
	            case 6 : pDmaParams->ch_fifo_depth[2] = 256; break;
            }
        }

        /* Channel 3 */
        if (pDmaParams->num_channels >= 4) 
		{
        	reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_1_L_OFFSET);
			tmp = (reg & DMAC_PARAM_CH3_MAX_BLK_SIZE) >> 12;
            switch (tmp)
			{
                case 0  : pDmaParams->ch_max_blk_size[3] =    3; break;
                case 1  : pDmaParams->ch_max_blk_size[3] =    7; break;
                case 2  : pDmaParams->ch_max_blk_size[3] =   15; break;
                case 3  : pDmaParams->ch_max_blk_size[3] =   31; break;
                case 4  : pDmaParams->ch_max_blk_size[3] =   63; break;
                case 5  : pDmaParams->ch_max_blk_size[3] =  127; break;
                case 6  : pDmaParams->ch_max_blk_size[3] =  255; break;
                case 7  : pDmaParams->ch_max_blk_size[3] =  511; break;
                case 8  : pDmaParams->ch_max_blk_size[3] = 1023; break;
                case 9  : pDmaParams->ch_max_blk_size[3] = 2047; break;
                case 10 : pDmaParams->ch_max_blk_size[3] = 4095; break;
            }
            reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_2_H_OFFSET);
			pDmaParams->ch_multi_blk_type[3] = (reg & DMAC_PARAM_CH3_MULTI_BLK_TYPE) >> 12;
			
			reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_4_H_OFFSET);
			tmp = reg & DMAC_PARAM_CHX_DTW;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_dtw[3] =   0; break;
                case 1 : pDmaParams->ch_dtw[3] =   8; break;
                case 2 : pDmaParams->ch_dtw[3] =  16; break;
                case 3 : pDmaParams->ch_dtw[3] =  32; break;
                case 4 : pDmaParams->ch_dtw[3] =  64; break;
                case 5 : pDmaParams->ch_dtw[3] = 128; break;
                case 6 : pDmaParams->ch_dtw[3] = 256; break;
            }
			tmp = (reg & DMAC_PARAM_CHX_STW) >> 3;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_stw[3] =   0; break;
                case 1 : pDmaParams->ch_stw[3] =   8; break;
                case 2 : pDmaParams->ch_stw[3] =  16; break;
                case 3 : pDmaParams->ch_stw[3] =  32; break;
                case 4 : pDmaParams->ch_stw[3] =  64; break;
                case 5 : pDmaParams->ch_stw[3] = 128; break;
                case 6 : pDmaParams->ch_stw[3] = 256; break;
            }
			pDmaParams->ch_stat_dst[3] = (reg & DMAC_PARAM_CHX_STAT_DST) >> 6;
			pDmaParams->ch_stat_src[3] = (reg & DMAC_PARAM_CHX_STAT_SRC) >> 7;
			pDmaParams->ch_dst_sca_en[3] = (reg & DMAC_PARAM_CHX_DST_SCA_EN) >> 8;
			pDmaParams->ch_src_gat_en[3] = (reg & DMAC_PARAM_CHX_SRC_GAT_EN) >> 9;
			pDmaParams->ch_lock_en[3] = (reg & DMAC_PARAM_CHX_LOCK_EN) >> 10;
			pDmaParams->ch_multi_blk_en[3] = (reg & DMAC_PARAM_CHX_MULTI_BLK_EN) >> 11;
			pDmaParams->ch_ctl_wb_en[3] = (reg & DMAC_PARAM_CHX_CTL_WB_EN) >> 12;
			pDmaParams->ch_hc_llp[3] = (reg & DMAC_PARAM_CHX_HC_LLP) >> 13;
			pDmaParams->ch_fc[3] = (reg & DMAC_PARAM_CHX_FC) >> 14;
			
			tmp = (reg & DMAC_PARAM_CHX_MAX_MULT_SIZE) >> 16;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_max_mult_size[3] =   4; break;
                case 1 : pDmaParams->ch_max_mult_size[3] =   8; break;
                case 2 : pDmaParams->ch_max_mult_size[3] =  16; break;
                case 3 : pDmaParams->ch_max_mult_size[3] =  32; break;
                case 4 : pDmaParams->ch_max_mult_size[3] =  64; break;
                case 5 : pDmaParams->ch_max_mult_size[3] = 128; break;
                case 6 : pDmaParams->ch_max_mult_size[3] = 256; break;
            }
			
			pDmaParams->ch_dms[3] = (reg & DMAC_PARAM_CHX_DMS) >> 19;
			pDmaParams->ch_lms[3] = (reg & DMAC_PARAM_CHX_LMS) >> 22;
			pDmaParams->ch_sms[3] = (reg & DMAC_PARAM_CHX_SMS) >> 25;
			
			tmp = (reg & DMAC_PARAM_CHX_FIFO_DEPTH) >> 28;
            switch (tmp)
			{
	            case 0 : pDmaParams->ch_fifo_depth[3] =   4; break;
	            case 1 : pDmaParams->ch_fifo_depth[3] =   8; break;
	            case 2 : pDmaParams->ch_fifo_depth[3] =  16; break;
	            case 3 : pDmaParams->ch_fifo_depth[3] =  32; break;
	            case 4 : pDmaParams->ch_fifo_depth[3] =  64; break;
	            case 5 : pDmaParams->ch_fifo_depth[3] = 128; break;
	            case 6 : pDmaParams->ch_fifo_depth[3] = 256; break;
            }
        }

        /* Channel 4 */
        if (pDmaParams->num_channels >= 5) 
		{
        	reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_1_L_OFFSET);
			tmp = (reg & DMAC_PARAM_CH4_MAX_BLK_SIZE) >> 16;
            switch (tmp)
			{
                case 0  : pDmaParams->ch_max_blk_size[4] =    3; break;
                case 1  : pDmaParams->ch_max_blk_size[4] =    7; break;
                case 2  : pDmaParams->ch_max_blk_size[4] =   15; break;
                case 3  : pDmaParams->ch_max_blk_size[4] =   31; break;
                case 4  : pDmaParams->ch_max_blk_size[4] =   63; break;
                case 5  : pDmaParams->ch_max_blk_size[4] =  127; break;
                case 6  : pDmaParams->ch_max_blk_size[4] =  255; break;
                case 7  : pDmaParams->ch_max_blk_size[4] =  511; break;
                case 8  : pDmaParams->ch_max_blk_size[4] = 1023; break;
                case 9  : pDmaParams->ch_max_blk_size[4] = 2047; break;
                case 10 : pDmaParams->ch_max_blk_size[4] = 4095; break;
            }
			reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_2_H_OFFSET);
			pDmaParams->ch_multi_blk_type[4] = (reg & DMAC_PARAM_CH4_MULTI_BLK_TYPE) >> 16;
			
			reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_4_L_OFFSET);
			tmp = reg & DMAC_PARAM_CHX_DTW;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_dtw[4] =   0; break;
                case 1 : pDmaParams->ch_dtw[4] =   8; break;
                case 2 : pDmaParams->ch_dtw[4] =  16; break;
                case 3 : pDmaParams->ch_dtw[4] =  32; break;
                case 4 : pDmaParams->ch_dtw[4] =  64; break;
                case 5 : pDmaParams->ch_dtw[4] = 128; break;
                case 6 : pDmaParams->ch_dtw[4] = 256; break;
            }
			tmp = (reg & DMAC_PARAM_CHX_STW) >> 3;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_stw[4] =   0; break;
                case 1 : pDmaParams->ch_stw[4] =   8; break;
                case 2 : pDmaParams->ch_stw[4] =  16; break;
                case 3 : pDmaParams->ch_stw[4] =  32; break;
                case 4 : pDmaParams->ch_stw[4] =  64; break;
                case 5 : pDmaParams->ch_stw[4] = 128; break;
                case 6 : pDmaParams->ch_stw[4] = 256; break;
            }
			
			pDmaParams->ch_stat_dst[4] = (reg & DMAC_PARAM_CHX_STAT_DST) >> 6;
			pDmaParams->ch_stat_src[4] = (reg & DMAC_PARAM_CHX_STAT_SRC) >> 7;
			pDmaParams->ch_dst_sca_en[4] = (reg & DMAC_PARAM_CHX_DST_SCA_EN) >> 8;
			pDmaParams->ch_src_gat_en[4] = (reg & DMAC_PARAM_CHX_SRC_GAT_EN) >> 9;
			pDmaParams->ch_lock_en[4] = (reg & DMAC_PARAM_CHX_LOCK_EN) >> 10;
			pDmaParams->ch_multi_blk_en[4] = (reg & DMAC_PARAM_CHX_MULTI_BLK_EN) >> 11;
			pDmaParams->ch_ctl_wb_en[4] = (reg & DMAC_PARAM_CHX_CTL_WB_EN) >> 12;
			pDmaParams->ch_hc_llp[4] = (reg & DMAC_PARAM_CHX_HC_LLP) >> 13;
			pDmaParams->ch_fc[4] = (reg & DMAC_PARAM_CHX_FC) >> 14;
			
			tmp = (reg & DMAC_PARAM_CHX_MAX_MULT_SIZE) >> 16;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_max_mult_size[4] =   4; break;
                case 1 : pDmaParams->ch_max_mult_size[4] =   8; break;
                case 2 : pDmaParams->ch_max_mult_size[4] =  16; break;
                case 3 : pDmaParams->ch_max_mult_size[4] =  32; break;
                case 4 : pDmaParams->ch_max_mult_size[4] =  64; break;
                case 5 : pDmaParams->ch_max_mult_size[4] = 128; break;
                case 6 : pDmaParams->ch_max_mult_size[4] = 256; break;
            }
			
			pDmaParams->ch_dms[4] = (reg & DMAC_PARAM_CHX_DMS) >> 19;
			pDmaParams->ch_lms[4] = (reg & DMAC_PARAM_CHX_LMS) >> 22;
			pDmaParams->ch_sms[4] = (reg & DMAC_PARAM_CHX_SMS) >> 25;
			
			tmp = (reg & DMAC_PARAM_CHX_FIFO_DEPTH) >> 28;
            switch (tmp)
			{
	            case 0 : pDmaParams->ch_fifo_depth[4] =   4; break;
	            case 1 : pDmaParams->ch_fifo_depth[4] =   8; break;
	            case 2 : pDmaParams->ch_fifo_depth[4] =  16; break;
	            case 3 : pDmaParams->ch_fifo_depth[4] =  32; break;
	            case 4 : pDmaParams->ch_fifo_depth[4] =  64; break;
	            case 5 : pDmaParams->ch_fifo_depth[4] = 128; break;
	            case 6 : pDmaParams->ch_fifo_depth[4] = 256; break;
            }
        }

        /* Channel 5 */
        if (pDmaParams->num_channels >= 6) 
		{
			reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_1_L_OFFSET);
			tmp = (reg & DMAC_PARAM_CH5_MAX_BLK_SIZE) >> 20;
            switch (tmp)
			{
                case 0  : pDmaParams->ch_max_blk_size[5] =    3; break;
                case 1  : pDmaParams->ch_max_blk_size[5] =    7; break;
                case 2  : pDmaParams->ch_max_blk_size[5] =   15; break;
                case 3  : pDmaParams->ch_max_blk_size[5] =   31; break;
                case 4  : pDmaParams->ch_max_blk_size[5] =   63; break;
                case 5  : pDmaParams->ch_max_blk_size[5] =  127; break;
                case 6  : pDmaParams->ch_max_blk_size[5] =  255; break;
                case 7  : pDmaParams->ch_max_blk_size[5] =  511; break;
                case 8  : pDmaParams->ch_max_blk_size[5] = 1023; break;
                case 9  : pDmaParams->ch_max_blk_size[5] = 2047; break;
                case 10 : pDmaParams->ch_max_blk_size[5] = 4095; break;
            }
			
			reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_2_H_OFFSET);
			pDmaParams->ch_multi_blk_type[5] = (reg & DMAC_PARAM_CH5_MULTI_BLK_TYPE) >> 20;
			
			reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_5_H_OFFSET);
			tmp = reg & DMAC_PARAM_CHX_DTW;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_dtw[5] =   0; break;
                case 1 : pDmaParams->ch_dtw[5] =   8; break;
                case 2 : pDmaParams->ch_dtw[5] =  16; break;
                case 3 : pDmaParams->ch_dtw[5] =  32; break;
                case 4 : pDmaParams->ch_dtw[5] =  64; break;
                case 5 : pDmaParams->ch_dtw[5] = 128; break;
                case 6 : pDmaParams->ch_dtw[5] = 256; break;
            }
			tmp = (reg & DMAC_PARAM_CHX_STW) >> 3;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_stw[5] =   0; break;
                case 1 : pDmaParams->ch_stw[5] =   8; break;
                case 2 : pDmaParams->ch_stw[5] =  16; break;
                case 3 : pDmaParams->ch_stw[5] =  32; break;
                case 4 : pDmaParams->ch_stw[5] =  64; break;
                case 5 : pDmaParams->ch_stw[5] = 128; break;
                case 6 : pDmaParams->ch_stw[5] = 256; break;
            }
            
			pDmaParams->ch_stat_dst[5] = (reg & DMAC_PARAM_CHX_STAT_DST) >> 6;
			pDmaParams->ch_stat_src[5] = (reg & DMAC_PARAM_CHX_STAT_SRC) >> 7;
			pDmaParams->ch_dst_sca_en[5] = (reg & DMAC_PARAM_CHX_DST_SCA_EN) >> 8;
			pDmaParams->ch_src_gat_en[5] = (reg & DMAC_PARAM_CHX_SRC_GAT_EN) >> 9;
			pDmaParams->ch_lock_en[5] = (reg & DMAC_PARAM_CHX_LOCK_EN) >> 10;
			pDmaParams->ch_multi_blk_en[5] = (reg & DMAC_PARAM_CHX_MULTI_BLK_EN) >> 11;
			pDmaParams->ch_ctl_wb_en[5] = (reg & DMAC_PARAM_CHX_CTL_WB_EN) >> 12;
			pDmaParams->ch_hc_llp[5] = (reg & DMAC_PARAM_CHX_HC_LLP) >> 13;
			pDmaParams->ch_fc[5] = (reg & DMAC_PARAM_CHX_FC) >> 14;
			
			tmp = (reg & DMAC_PARAM_CHX_MAX_MULT_SIZE) >> 16;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_max_mult_size[5] =   4; break;
                case 1 : pDmaParams->ch_max_mult_size[5] =   8; break;
                case 2 : pDmaParams->ch_max_mult_size[5] =  16; break;
                case 3 : pDmaParams->ch_max_mult_size[5] =  32; break;
                case 4 : pDmaParams->ch_max_mult_size[5] =  64; break;
                case 5 : pDmaParams->ch_max_mult_size[5] = 128; break;
                case 6 : pDmaParams->ch_max_mult_size[5] = 256; break;
            }
			
			pDmaParams->ch_dms[5] = (reg & DMAC_PARAM_CHX_DMS) >> 19;
			pDmaParams->ch_lms[5] = (reg & DMAC_PARAM_CHX_LMS) >> 22;
			pDmaParams->ch_sms[5] = (reg & DMAC_PARAM_CHX_SMS) >> 25;
			
			tmp = (reg & DMAC_PARAM_CHX_FIFO_DEPTH) >> 28;
            switch (tmp)
			{
	            case 0 : pDmaParams->ch_fifo_depth[5] =   4; break;
	            case 1 : pDmaParams->ch_fifo_depth[5] =   8; break;
	            case 2 : pDmaParams->ch_fifo_depth[5] =  16; break;
	            case 3 : pDmaParams->ch_fifo_depth[5] =  32; break;
	            case 4 : pDmaParams->ch_fifo_depth[5] =  64; break;
	            case 5 : pDmaParams->ch_fifo_depth[5] = 128; break;
	            case 6 : pDmaParams->ch_fifo_depth[5] = 256; break;
            }
        }

        /* Channel 6 */
        if (pDmaParams->num_channels >= 7) 
		{
			reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_1_L_OFFSET);
			tmp = (reg & DMAC_PARAM_CH6_MAX_BLK_SIZE) >> 24;
            switch (tmp)
			{
                case 0  : pDmaParams->ch_max_blk_size[6] =    3; break;
                case 1  : pDmaParams->ch_max_blk_size[6] =    7; break;
                case 2  : pDmaParams->ch_max_blk_size[6] =   15; break;
                case 3  : pDmaParams->ch_max_blk_size[6] =   31; break;
                case 4  : pDmaParams->ch_max_blk_size[6] =   63; break;
                case 5  : pDmaParams->ch_max_blk_size[6] =  127; break;
                case 6  : pDmaParams->ch_max_blk_size[6] =  255; break;
                case 7  : pDmaParams->ch_max_blk_size[6] =  511; break;
                case 8  : pDmaParams->ch_max_blk_size[6] = 1023; break;
                case 9  : pDmaParams->ch_max_blk_size[6] = 2047; break;
                case 10 : pDmaParams->ch_max_blk_size[6] = 4095; break;
            }
			
			reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_2_H_OFFSET);
			pDmaParams->ch_multi_blk_type[6] = (reg & DMAC_PARAM_CH6_MULTI_BLK_TYPE) >> 24;
			
			reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_5_L_OFFSET);
			tmp = reg & DMAC_PARAM_CHX_DTW;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_dtw[6] =   0; break;
                case 1 : pDmaParams->ch_dtw[6] =   8; break;
                case 2 : pDmaParams->ch_dtw[6] =  16; break;
                case 3 : pDmaParams->ch_dtw[6] =  32; break;
                case 4 : pDmaParams->ch_dtw[6] =  64; break;
                case 5 : pDmaParams->ch_dtw[6] = 128; break;
                case 6 : pDmaParams->ch_dtw[6] = 256; break;
            }
			tmp = (reg & DMAC_PARAM_CHX_STW) >> 3;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_stw[6] =   0; break;
                case 1 : pDmaParams->ch_stw[6] =   8; break;
                case 2 : pDmaParams->ch_stw[6] =  16; break;
                case 3 : pDmaParams->ch_stw[6] =  32; break;
                case 4 : pDmaParams->ch_stw[6] =  64; break;
                case 5 : pDmaParams->ch_stw[6] = 128; break;
                case 6 : pDmaParams->ch_stw[6] = 256; break;
            }
            
			pDmaParams->ch_stat_dst[6] = (reg & DMAC_PARAM_CHX_STAT_DST) >> 6;
			pDmaParams->ch_stat_src[6] = (reg & DMAC_PARAM_CHX_STAT_SRC) >> 7;
			pDmaParams->ch_dst_sca_en[6] = (reg & DMAC_PARAM_CHX_DST_SCA_EN) >> 8;
			pDmaParams->ch_src_gat_en[6] = (reg & DMAC_PARAM_CHX_SRC_GAT_EN) >> 9;
			pDmaParams->ch_lock_en[6] = (reg & DMAC_PARAM_CHX_LOCK_EN) >> 10;
			pDmaParams->ch_multi_blk_en[6] = (reg & DMAC_PARAM_CHX_MULTI_BLK_EN) >> 11;
			pDmaParams->ch_ctl_wb_en[6] = (reg & DMAC_PARAM_CHX_CTL_WB_EN) >> 12;
			pDmaParams->ch_hc_llp[6] = (reg & DMAC_PARAM_CHX_HC_LLP) >> 13;
			pDmaParams->ch_fc[6] = (reg & DMAC_PARAM_CHX_FC) >> 14;
			
			tmp = (reg & DMAC_PARAM_CHX_MAX_MULT_SIZE) >> 16;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_max_mult_size[6] =   4; break;
                case 1 : pDmaParams->ch_max_mult_size[6] =   8; break;
                case 2 : pDmaParams->ch_max_mult_size[6] =  16; break;
                case 3 : pDmaParams->ch_max_mult_size[6] =  32; break;
                case 4 : pDmaParams->ch_max_mult_size[6] =  64; break;
                case 5 : pDmaParams->ch_max_mult_size[6] = 128; break;
                case 6 : pDmaParams->ch_max_mult_size[6] = 256; break;
            }
			
			pDmaParams->ch_dms[6] = (reg & DMAC_PARAM_CHX_DMS) >> 19;
			pDmaParams->ch_lms[6] = (reg & DMAC_PARAM_CHX_LMS) >> 22;
			pDmaParams->ch_sms[6] = (reg & DMAC_PARAM_CHX_SMS) >> 25;
			
			tmp = (reg & DMAC_PARAM_CHX_FIFO_DEPTH) >> 28;
            switch (tmp)
			{
	            case 0 : pDmaParams->ch_fifo_depth[6] =   4; break;
	            case 1 : pDmaParams->ch_fifo_depth[6] =   8; break;
	            case 2 : pDmaParams->ch_fifo_depth[6] =  16; break;
	            case 3 : pDmaParams->ch_fifo_depth[6] =  32; break;
	            case 4 : pDmaParams->ch_fifo_depth[6] =  64; break;
	            case 5 : pDmaParams->ch_fifo_depth[6] = 128; break;
	            case 6 : pDmaParams->ch_fifo_depth[6] = 256; break;
            }
        }

        /* Channel 7 */
        if (pDmaParams->num_channels == 8) 
		{
			reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_1_L_OFFSET);
			tmp = (reg & DMAC_PARAM_CH7_MAX_BLK_SIZE) >> 28;
            switch (tmp)
			{
                case 0  : pDmaParams->ch_max_blk_size[7] =    3; break;
                case 1  : pDmaParams->ch_max_blk_size[7] =    7; break;
                case 2  : pDmaParams->ch_max_blk_size[7] =   15; break;
                case 3  : pDmaParams->ch_max_blk_size[7] =   31; break;
                case 4  : pDmaParams->ch_max_blk_size[7] =   63; break;
                case 5  : pDmaParams->ch_max_blk_size[7] =  127; break;
                case 6  : pDmaParams->ch_max_blk_size[7] =  255; break;
                case 7  : pDmaParams->ch_max_blk_size[7] =  511; break;
                case 8  : pDmaParams->ch_max_blk_size[7] = 1023; break;
                case 9  : pDmaParams->ch_max_blk_size[7] = 2047; break;
                case 10 : pDmaParams->ch_max_blk_size[7] = 4095; break;
            }
			reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_2_H_OFFSET);
			pDmaParams->ch_multi_blk_type[7] = (reg & DMAC_PARAM_CH7_MULTI_BLK_TYPE) >> 28;
			
			reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_COMP_PARAMS_6_H_OFFSET);
			tmp = reg & DMAC_PARAM_CHX_DTW;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_dtw[7] =   0; break;
                case 1 : pDmaParams->ch_dtw[7] =   8; break;
                case 2 : pDmaParams->ch_dtw[7] =  16; break;
                case 3 : pDmaParams->ch_dtw[7] =  32; break;
                case 4 : pDmaParams->ch_dtw[7] =  64; break;
                case 5 : pDmaParams->ch_dtw[7] = 128; break;
                case 6 : pDmaParams->ch_dtw[7] = 256; break;
            }
			tmp = (reg & DMAC_PARAM_CHX_STW) >> 3;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_stw[7] =   0; break;
                case 1 : pDmaParams->ch_stw[7] =   8; break;
                case 2 : pDmaParams->ch_stw[7] =  16; break;
                case 3 : pDmaParams->ch_stw[7] =  32; break;
                case 4 : pDmaParams->ch_stw[7] =  64; break;
                case 5 : pDmaParams->ch_stw[7] = 128; break;
                case 6 : pDmaParams->ch_stw[7] = 256; break;
            }
			
			pDmaParams->ch_stat_dst[7] = (reg & DMAC_PARAM_CHX_STAT_DST) >> 6;
			pDmaParams->ch_stat_src[7] = (reg & DMAC_PARAM_CHX_STAT_SRC) >> 7;
			pDmaParams->ch_dst_sca_en[7] = (reg & DMAC_PARAM_CHX_DST_SCA_EN) >> 8;
			pDmaParams->ch_src_gat_en[7] = (reg & DMAC_PARAM_CHX_SRC_GAT_EN) >> 9;
			pDmaParams->ch_lock_en[7] = (reg & DMAC_PARAM_CHX_LOCK_EN) >> 10;
			pDmaParams->ch_multi_blk_en[7] = (reg & DMAC_PARAM_CHX_MULTI_BLK_EN) >> 11;
			pDmaParams->ch_ctl_wb_en[7] = (reg & DMAC_PARAM_CHX_CTL_WB_EN) >> 12;
			pDmaParams->ch_hc_llp[7] = (reg & DMAC_PARAM_CHX_HC_LLP) >> 13;
			pDmaParams->ch_fc[7] = (reg & DMAC_PARAM_CHX_FC) >> 14;
			
			tmp = (reg & DMAC_PARAM_CHX_MAX_MULT_SIZE) >> 16;
            switch (tmp)
			{
                case 0 : pDmaParams->ch_max_mult_size[7] =   4; break;
                case 1 : pDmaParams->ch_max_mult_size[7] =   8; break;
                case 2 : pDmaParams->ch_max_mult_size[7] =  16; break;
                case 3 : pDmaParams->ch_max_mult_size[7] =  32; break;
                case 4 : pDmaParams->ch_max_mult_size[7] =  64; break;
                case 5 : pDmaParams->ch_max_mult_size[7] = 128; break;
                case 6 : pDmaParams->ch_max_mult_size[7] = 256; break;
            }
			
			pDmaParams->ch_dms[7] = (reg & DMAC_PARAM_CHX_DMS) >> 19;
			pDmaParams->ch_lms[7] = (reg & DMAC_PARAM_CHX_LMS) >> 22;
			pDmaParams->ch_sms[7] = (reg & DMAC_PARAM_CHX_SMS) >> 25;
			
			tmp = (reg & DMAC_PARAM_CHX_FIFO_DEPTH) >> 28;
            switch (tmp)
			{
	            case 0 : pDmaParams->ch_fifo_depth[7] =   4; break;
	            case 1 : pDmaParams->ch_fifo_depth[7] =   8; break;
	            case 2 : pDmaParams->ch_fifo_depth[7] =  16; break;
	            case 3 : pDmaParams->ch_fifo_depth[7] =  32; break;
	            case 4 : pDmaParams->ch_fifo_depth[7] =  64; break;
	            case 5 : pDmaParams->ch_fifo_depth[7] = 128; break;
	            case 6 : pDmaParams->ch_fifo_depth[7] = 256; break;
            }
        }
    }
    else
	{
        ret = -FMSH_ENOSYS;
    }
    return ret;
}

/*****************************************************************************
*
* @description
* This function enables/disables test mode in the DMAC.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*           work_mode Enumerated Enabled/Disabled work_mode.
*
* @return   
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void dmaCtrl_Set_TestMode(vxT_DMA* pDma, int mode)
{
	dmaCtrl_Wr_CfgReg32(pDma, DMAC_DMA_TEST_REG_L_OFFSET, mode);
	return;
}

/*****************************************************************************
*
* @description
* This function places each channel number into an ordered array
* based on the priority level setting for each channel.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*
* @return   
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void dmaCtrl_Set_ChnPriority(vxT_DMA* pDma)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int i, chn, order; 
	UINT32 ch_priority;	
    int priority[8];

    /* Initialise the channel order and priority*/
    for (i=0; i<DMAC_MAX_CHANNELS; i++)
	{
        priority[i] = 0;
		
        pDma->pDmaCtrl->ch_order[i] = i;
    }

    /* loop through each of the channels configured on the DMAC*/
    for (chn=0; chn<pParams->num_channels; chn++)
	{
        /* read the priority of the current channel*/
		ch_priority = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_CFG_L_OFFSET(chn));
		ch_priority = (ch_priority & DMAC_CFG_L_CH_PRIOR) >> 5;

        /* Check the channel order array to see what position*/
        /* this channel comes in the priority list.*/
        for (i=0; i<=pParams->num_channels; i++)
		{
            if (A_MAXEQ_B(ch_priority, priority[i]) || (i == pParams->num_channels))
			{
                order = i;
                break;
            }
        }

        /* Now we know the order for this channel insert in the correct*/
        /* array position and shift current priority ordering to suite.*/
        for (i=chn; i>order; i--)
		{
            priority[i] = priority[i-1];
            pDma->pDmaCtrl->ch_order[i] = pDma->pDmaCtrl->ch_order[i-1];
        }
		
        priority[order] = ch_priority;
        pDma->pDmaCtrl->ch_order[order] = chn;
    }

	return;
}

/*****************************************************************************
*
* @description
* This function checks if the specified DMA channel is in range.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
  int dmaCtrl_Chk_ChnRange(vxT_DMA* pDma, int chn)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int errCode = 0;

    if ((DMAC_CH_MASK & chn) || (chn == DMA_CHN_NONE))
	{
        errCode = -FMSH_ECHRNG;
    }
	
    return errCode;
}


/*****************************************************************************
*
* @description
* This function will disable the dma controller.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Dis_Ctrl(vxT_DMA* pDma)
{
    int errCode;
    UINT32 regVal;

    errCode = 0;

    /* Check first to see if DMA is already disabled*/
	regVal = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_CFG_REG_L_OFFSET);
	regVal &= DMAC_DMACFGREG_L_DMA_EN;

    if (regVal != 0x0)
	{
        /*        
		bit0	DMA_EN	R/W  0x0 DMAC使能位。
				0 = DMAC禁用
				1 = DMAC启用。
		*/
		dmaCtrl_Wr_CfgReg32(pDma, DMAC_DMA_CFG_REG_L_OFFSET, 0x0);

        /* Ensure that the DMA was disabled*/
        /* May not disable due to split response on one*/
        /* of the DMA channels*/
        
		if (dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_CFG_REG_L_OFFSET))
		{
			errCode = -FMSH_EBUSY;
		}
    }
	
    return errCode;
}

/*****************************************************************************
*
* @description
* This function disables the specified DMA channel(s).
* Multiple DMA channels can be specified for the FDmaPs_channelNumber argument.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Dis_ChnX(vxT_DMA* pDma, int chn)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int errCode;
    UINT32 regVal;
    UINT32 enabled_ch;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (chn == DMA_CHN_ALL)
	{
        chn &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errCode = dmaCtrl_Chk_ChnRange(pDma, chn);
	
    if (errCode == 0)
	{
        /* Check first if the channel was already disabled*/
		regVal = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CH_EN_REG_L_OFFSET);
        enabled_ch = regVal & chn;

        if (enabled_ch != 0x0)
		{
            regVal = chn & (DMAC_MAX_CH_MASK << DMAC_MAX_CHANNELS);
			dmaCtrl_Wr_CfgReg32(pDma, DMAC_CH_EN_REG_L_OFFSET, regVal);

            /* Ensure that the channel(s) was disabled.*/
            /* Channel may not disable due to a split response.*/
			if (dmaCtrl_Rd_CfgReg32(pDma, DMAC_CH_EN_REG_L_OFFSET) & chn)
			{
				errCode = -FMSH_EBUSY;
			}
        }
    }
	
    return errCode;
}

/*****************************************************************************
*
* @description
* This function checks if the specified DMA channel is Busy (enabled)
* or not. Also checks if the specified channel is in range.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Chk_ChnBusy(vxT_DMA* pDma, int chn)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int errCode = 0;
    UINT32 regVal;

    if ((DMAC_CH_MASK & chn) || (chn == DMA_CHN_NONE))
	{
        errCode = -FMSH_ECHRNG;
    }
	else
	{
		regVal = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CH_EN_REG_L_OFFSET);
        if (regVal & chn)
		{
            errCode = -FMSH_EBUSY;
        }
    }
	
    return errCode;
}


/*****************************************************************************
*
* @description
* This function disables interrupts for the selected channel(s).
* Multiple DMA channels can be specified for the FDmaPs_channelNumber argument.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Dis_ChnIRQ(vxT_DMA* pDma, int chn)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int x;
    int errCode;
    UINT32 regVal;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (chn == DMA_CHN_ALL)
	{
        chn &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errCode = dmaCtrl_Chk_ChnBusy(pDma, chn);
    if (errCode == 0)
	{
        /* Loop through each channel in turn and disable*/
        /* the channel Irq for the selected channels.*/
        for (x=0; x<pParams->num_channels; x++)
		{
            if (chn & (1 << x))
			{
				regVal = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_CTL_L_OFFSET(x));
				
				if ((regVal & DMAC_CTL_L_INT_EN) != 0x0)
				{
					regVal &= ~DMAC_CTL_L_INT_EN;					
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_CTL_L_OFFSET(x), regVal);
				}
            }
        }
    }
	
    return errCode;
}

/*****************************************************************************
*
* @description
* This function enables interrupts for the selected channel(s).
* Multiple DMA channels can be specified for the FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_En_ChnIRQ(vxT_DMA* pDma, int chn)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int x;
    int errCode;
    UINT32 reg;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (chn == DMA_CHN_ALL)
	{
        chn &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errCode = dmaCtrl_Chk_ChnBusy(pDma, chn);
	
    if (errCode == 0)
	{

        /* Loop through each channel in turn and enable*/
        /* the channel Irq for the selected channels.*/
        for (x = 0; x < pParams->num_channels; x++)
		{
            if (chn & (1 << x))
			{
				reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_CTL_L_OFFSET(x));
				
				if ((reg & DMAC_CTL_L_INT_EN) != 0x1)
				{
					reg |= DMAC_CTL_L_INT_EN;
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_CTL_L_OFFSET(x), reg);
				}
            }
        }
    }
	
    return errCode;
}


/*****************************************************************************
*
* @description
* This function clears the specified interrupt(s) on the specified channel(s).
* Multiple DMA channels can be specified for the FDmaPs_channelNumber argument.
* Multiple interrupt types can be specified for the FDmaPs_irq argument.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*           ch_irq Enumerated interrupt type.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Clr_IRQ(vxT_DMA* pDma, int chn, int irq_no)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int x;
    int errCode;
    UINT32 regVal;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (chn == DMA_CHN_ALL)
	{
        chn &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errCode = dmaCtrl_Chk_ChnRange(pDma, chn);
    if (errCode == 0)
	{
        /* Loop through and clear the selected channel Irq*/
        /* for the targeted channels.*/
        regVal = chn & DMAC_MAX_CH_MASK;
		
        for (x=0; x<DMAC_MAX_INTERRUPTS; x++)
		{
            if (irq_no & (1 << x))
			{
                switch (x)
				{
                case 0 : 
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_INT_CLEAR_TFR_L_OFFSET, regVal);
                    break;
                case 1 : 
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_INT_CLEAR_BLOCK_L_OFFSET, regVal);
                    break;
                case 2 : 
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_INT_CLEAR_SRCTRAN_L_OFFSET, regVal);
                    break;
                case 3 : 
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_INT_CLEAR_DSTTRAN_L_OFFSET, regVal);
                    break;
                case 4 : 
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_INT_CLEAR_ERR_L_OFFSET, regVal);
                    break;
            	}
            }
        }
    }
	
    return errCode;
}


/*****************************************************************************
*
* @description
* This function masks the specified interrupt(s) on the specified channel(s).
* Multiple DMA channels can be specified for the FDmaPs_channelNumber argument.
* Multiple interrupt types can be specified for the FDmaPs_irq argument.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*           ch_irq Enumerated interrupt type.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Mask_IRQ(vxT_DMA* pDma, int chn, UINT32 irq_no)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int x;	
    int errCode;
    UINT32 regVal;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (chn == DMA_CHN_ALL)
	{
        chn &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errCode = dmaCtrl_Chk_ChnRange(pDma, chn);
	
    if (errCode == 0)
	{
        /* Loop through and mask the selected channel Irq*/
        /* for the targeted channels.*/
        regVal = chn & (DMAC_MAX_CH_MASK << DMAC_MAX_CHANNELS);
		
        for (x=0; x<DMAC_MAX_INTERRUPTS; x++)
		{
            if (irq_no & (1 << x))
			{
                switch (x)
				{
                case 0 : 
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_INT_MASK_TFR_L_OFFSET, regVal);
					break;
                case 1 : 
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_INT_MASK_BLOCK_L_OFFSET, regVal);
					break;
                case 2 : 
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_INT_MASK_SRCTRAN_L_OFFSET, regVal);
                    break;
                case 3 : 
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_INT_MASK_DSTTRAN_L_OFFSET, regVal);
                    break;
                case 4 : 
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_INT_MASK_ERR_L_OFFSET, regVal);
                    break;
            	}
            }
        }
    }
	
    return errCode;
}

/*****************************************************************************
*
* @description
* This function unmasks the specified interrupt(s) on the specified channel(s).
* Multiple DMA channels can be specified for the FDmaPs_channelNumber argument.
* Multiple interrupt types can be specified for the FDmaPs_irq argument.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*           ch_irq Enumerated interrupt type.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_UnMask_IRQ(vxT_DMA* pDma, int chn, UINT32 irq_no)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int x;
    int errCode;
    UINT32 reg;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (chn == DMA_CHN_ALL)
	{
        chn &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errCode = dmaCtrl_Chk_ChnRange(pDma, chn);
	
    if (errCode == 0)
	{
        /* Loop through and unmask the selected channel Irq*/
        /* for the targeted channels.*/
        reg = chn;
        for (x = 0; x < DMAC_MAX_INTERRUPTS; x++)
		{
            if (irq_no & (1 << x))
			{
                switch (x)
				{
                case 0 : 
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_INT_MASK_TFR_L_OFFSET, reg);
                    break;
                case 1 : 
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_INT_MASK_BLOCK_L_OFFSET, reg);
                    break;
                case 2 : 
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_INT_MASK_SRCTRAN_L_OFFSET, reg);
                    break;
                case 3 : 
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_INT_MASK_DSTTRAN_L_OFFSET, reg);
                    break;
                case 4 : 
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_INT_MASK_ERR_L_OFFSET, reg);
                    break;
            	}
            }
        }
    }
	
    return errCode;
}


/*****************************************************************************
*
* @description
* This function returns a DMA channel number (enumerated) that is
* disabled. The function starts at channel 0 and increments up
* through the channels until a free channel is found.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*
* @return   
*           DMA channel number, as an enumerated type.
*
* @note     NA.
*
*****************************************************************************/
UINT32 dmaCtrl_Get_FreeChn(vxT_DMA* pDma)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int x, y;
    UINT32 reg;
    int retVal = DMA_CHN_NONE;

    /* read the channel enable register*/
	reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CH_EN_REG_L_OFFSET);
	reg &= DMAC_CHENREG_L_CH_EN_ALL;

    /* Check each channel in turn until we find one*/
    /* that is NOT enabled.  Loop checks channels*/
    /* starting at channel 0 and works up incrementally.*/
    for (x=0; x<pParams->num_channels; x++)
	{
        y = (1 << x);
		
        if (!(reg & y))
		{
            retVal = ((y << DMAC_MAX_CHANNELS) | y);
            break;
        }
    }
	
    return retVal;
}


/*****************************************************************************
*
* @description
* This function returns the channel index from the specified channel
* enumerated type.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument.
*
* @pParams    
*           chn Enumerated DMA channel number.
*
* @return   
*           The DMA channel index.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Get_ChnIdx(int chn)
{
    unsigned ch_enum = 1;
    unsigned ch_idx = 0;

    chn &= DMAC_MAX_CH_MASK;
	
    while (ch_idx < DMAC_MAX_CHANNELS)
	{
        if (ch_enum == chn)
		{
			break;
		}
		
        ch_enum *= 2; 
		
		ch_idx++;
    }
	
    return ch_idx;
}  

/*****************************************************************************
*
* @description
* This function sets the address for the first linked list item
* in the system memory for the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*           address Linked list item address.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Set_ListPtrAddr(vxT_DMA* pDma, int chn, UINT32 address)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int errCode = 0;
    int x;
    UINT32 regVal;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (chn == DMA_CHN_ALL)
	{
        chn &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errCode = dmaCtrl_Chk_ChnBusy(pDma, chn);
	
    /* Check if the register is present*/
    if (errCode == 0)
	{
        for (x = 0; x < pParams->num_channels; x++)
		{
            if (chn & (1 << x))
			{
                if (pParams->ch_hc_llp[x] == 0x1)
				{
                    errCode = -FMSH_ENOSYS;
                }
            }
        }
    }

    if (errCode == 0)
	{
        for (x = 0; x < pParams->num_channels; x++)
		{
            if (chn & (1 << x))
			{
				regVal = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_LLP_L_OFFSET(x));
				
				if (((regVal & DMAC_LLP_L_LOC) >> 2) != address)
				{
					regVal = (regVal & ~DMAC_LLP_L_LOC) | (address << 2);
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_LLP_L_OFFSET(x), regVal);
				}
            }
        }
    }
	
    return errCode;
}

/*****************************************************************************
*
* @description
* This function enables or disables the block chaining on the 
* specified source and/or destination on the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument. Both source and destination
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*           src_dst_sel Enumerated source/destination select.
*           work_mode Enumerated enable/disable work_mode.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
 int dmaCtrl_Set_BlockChain_En(vxT_DMA* pDma, int chn,  int src_dst_sel, int work_mode)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);

    int x, errCode;
    BOOL write_en;
    UINT32 regVal;
	
    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (chn == DMA_CHN_ALL)
	{
        chn &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errCode = dmaCtrl_Chk_ChnBusy(pDma, chn);
	
    if (errCode == 0)
	{
        /* Check for hard-coded values*/
        for (x = 0; x < pParams->num_channels; x++)
		{
            if (chn & (1 << x))
			{
                if ((pParams->ch_multi_blk_en[x] == 0x0) || (pParams->ch_hc_llp[x] == 0x1))
				{
                    errCode = -FMSH_ENOSYS;
                    break;
                }
            }
        }
    }
	
    if (errCode == 0)
	{
        for (x = 0; x < pParams->num_channels; x++)
		{
            if (chn & (1 << x))
			{
                write_en = FALSE;
				regVal = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_CTL_L_OFFSET(x));
				
                if ((src_dst_sel == DMA_SRC) || (src_dst_sel == DMA_SRC_DST))
				{
					if(((regVal & DMAC_CTL_L_LLP_SRC_EN) >> 28) != work_mode)
					{
						regVal = (regVal & ~DMAC_CTL_L_LLP_SRC_EN) | (work_mode << 28);
                        write_en = 1;  /* TRUE;*/
                    }
                }
				
                if ((src_dst_sel == DMA_DST) || (src_dst_sel == DMA_SRC_DST))
				{
					if(((regVal & DMAC_CTL_L_LLP_DST_EN) >> 27) != work_mode)
					{
						regVal = (regVal & ~DMAC_CTL_L_LLP_DST_EN) | (work_mode << 27);
                        write_en = 1;  /* TRUE;*/
                    }
                }
				
                /* only do the write if the value being written*/
                /* differs from the current register value*/
                if (write_en == 1)
				{
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_CTL_L_OFFSET(x), regVal);
                }
            }
        }
    }
	
    return errCode;
}

/*****************************************************************************
*
* @description
* This function enables or disables the reload feature on the
* specified source and/or destination on the specified DMA channel(s).
* Multiple DMA channels can be specified for the chn argument. Both
* source and destination can be specified for the src_dst_sel argument.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*           src_dst_sel Enumerated source/destination select.
*           work_mode Enumerated enable/disable work_mode.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Set_ReLoad(vxT_DMA* pDma, int chn,  int src_dst_sel, int work_mode)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int x, errCode;
    BOOL write_en;
    UINT32 regVal;


    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (chn == DMA_CHN_ALL)
	{
        chn &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errCode = dmaCtrl_Chk_ChnRange(pDma, chn);
	
    if (errCode == 0)
	{
        /* Check for hard-coded values*/
        for (x = 0; x < pParams->num_channels; x++)
		{
            if (chn & (1 << x))
			{
                if (pParams->ch_multi_blk_en[x] == 0x0)
				{
                    errCode = -FMSH_ENOSYS;
                    break;
                }
				
                if (pParams->ch_multi_blk_type[x] != 0x0)
				{
                    if ((src_dst_sel == DMA_SRC) || (src_dst_sel == DMA_SRC_DST))
					{
                        if ((work_mode == DMA_WORK_MODE_TEST) && 
							 ((pParams->ch_multi_blk_type[x] != DMA_RELOAD_CONT) 
							   && (pParams->ch_multi_blk_type[x] != DMA_RELOAD_LLP)))
                        {
                            errCode = -FMSH_ENOSYS;
                            break;
                        }
							   
                        if ((work_mode == DMA_WORK_MODE_NORMAL) &&
                            ((pParams->ch_multi_blk_type[x] == DMA_RELOAD_CONT)
                              || (pParams->ch_multi_blk_type[x] == DMA_RELOAD_LLP)))
                        {
                            errCode = -FMSH_ENOSYS;
                            break;
                        }
                    }
					
                    if ((src_dst_sel == DMA_DST) || (src_dst_sel == DMA_SRC_DST))
					{
                        if ((work_mode == DMA_WORK_MODE_TEST) &&
                            ((pParams->ch_multi_blk_type[x] != DMA_CONT_RELOAD) &&
                             (pParams->ch_multi_blk_type[x] != DMA_RELOAD_RELOAD) &&
                             (pParams->ch_multi_blk_type[x] != DMA_LLP_RELOAD)))
                        {
                            errCode = -FMSH_ENOSYS;
                            break;
                        }
							 
                        if ((work_mode == DMA_WORK_MODE_NORMAL) &&
                            ((pParams->ch_multi_blk_type[x] == DMA_CONT_RELOAD) ||
                             (pParams->ch_multi_blk_type[x] == DMA_RELOAD_RELOAD) ||
                             (pParams->ch_multi_blk_type[x] == DMA_LLP_RELOAD)))
                        {
                            errCode = -FMSH_ENOSYS;
                            break;
                        }
                    }
                }
            }
        }
    }
	
    if (errCode == 0)
	{
        for (x = 0; x < pParams->num_channels; x++)
		{
            if (chn & (1 << x))
			{
                write_en = FALSE;
				
				regVal = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_CFG_L_OFFSET(x));
				
                if ((src_dst_sel == DMA_SRC) || (src_dst_sel == DMA_SRC_DST))
				{
					if (((regVal & DMAC_CFG_L_RELOAD_SRC) >> 30) != work_mode)
					{
						regVal = (regVal & ~DMAC_CFG_L_RELOAD_SRC) | (work_mode << 30);
                        write_en = 1;  /* TRUE;*/
                    }
                }
				
                if ((src_dst_sel == DMA_DST) || (src_dst_sel == DMA_SRC_DST))
				{
					if (((regVal & DMAC_CFG_L_RELOAD_DST) >> 31) != work_mode)
					{
						regVal = (regVal & ~DMAC_CFG_L_RELOAD_DST) | (work_mode << 31);
                        write_en = 1;  /* TRUE;*/
                    }
                }
				
                /* only do the write if the value being written*/
                /* differs from the current register value*/
                if (write_en == 1)
				{
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_CFG_L_OFFSET(x), regVal);
                }
            }
        }
    }
	
    return errCode;
}


/*****************************************************************************
*
* @description
* This function returns the DMA transfer type for the specified DMA channel.
* The FDmaPs_transferType enumerated type describes all of the transfer
* types supported by the DMA controller.
* Only 1 DMA channel can be specified for the FDmaPs_channelNumber argument.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*
* @return   
*           Enumerated DMA transfer type.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Get_XferType(vxT_DMA* pDma, int chn)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    UINT8 chnIdx;
    UINT32 llp_reg, ctl_reg, cfg_reg;
    UINT8 row;

	int ret = 0;
	
    chnIdx = dmaCtrl_Get_ChnIdx(chn);

    row = 0x0;
	
    llp_reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_LLP_L_OFFSET(chnIdx));
    ctl_reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_CTL_L_OFFSET(chnIdx));
    cfg_reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_CFG_L_OFFSET(chnIdx));

	/*//*/
	if (cfg_reg & DMAC_CFG_L_RELOAD_DST)
	{
        row |= 0x01;
    }
	
    if (ctl_reg & DMAC_CTL_L_LLP_DST_EN)
	{
        row |= 0x02;
    }

	/*//*/
    if (cfg_reg & DMAC_CFG_L_RELOAD_SRC)
	{
        row |= 0x04;
    }
	
    if (ctl_reg & DMAC_CTL_L_LLP_SRC_EN)
	{
        row |= 0x08;
    }
	
	/*//*/
    if (llp_reg & DMAC_LLP_L_LOC)
	{
        row |= 0x10;
    }

    switch (row)
	{
        case 0x00 : 
			ret = DMA_XFER_TYPE1_ROW1 ;  
			break;
        case 0x01 : 
			ret = DMA_XFER_TYPE2_ROW2 ;  
			break;
        case 0x04 : 
			ret = DMA_XFER_TYPE3_ROW3 ;  
			break;
        case 0x05 : 
			ret = DMA_XFER_TYPE4_ROW4 ;  
			break;
        case 0x10 : 
			ret = DMA_XFER_TYPE5_ROW5 ;  
			break;
        case 0x12 : 
			ret = DMA_XFER_TYPE6_ROW6 ;  
			break;
        case 0x16 : 
			ret = DMA_XFER_TYPE7_ROW7 ;  
			break;
        case 0x18 : 
			ret = DMA_XFER_TYPE8_ROW8 ;  
			break;
        case 0x19 : 
			ret = DMA_XFER_TYPE9_ROW9 ;  
			break;
        case 0x1a : 
			ret = DMA_XFER_TYPE10_ROW10;  
			break;
    }
	
    return ret;
}


/*****************************************************************************
*
* @description
* This function sets up the specified DMA channel(s) for the specified
* transfer type. The FDmaPs_transferType enumerated type describes
* all of the transfer types supported by the DMA controller.
* Multiple DMA channels can be specified for the FDmaPs_channelNumber argument.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*           xfer_type Enumerated DMA transfer type.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Set_XferType(vxT_DMA* pDma, int chn, int xfer_type)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int errCode = 0;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (chn == DMA_CHN_ALL)
	{
        chn &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errCode = dmaCtrl_Chk_ChnBusy(pDma, chn);
    if (errCode == 0)
	{
        switch (xfer_type)
		{
            case DMA_XFER_TYPE1_ROW1:  /* Dmac_transfer_row1 :*/
                errCode = dmaCtrl_Set_ListPtrAddr(pDma, chn, 0x0);
                if (errCode == 0)
				{
                    errCode = dmaCtrl_Set_BlockChain_En(pDma, chn, DMA_SRC_DST, DMA_WORK_MODE_NORMAL);
                }
                if (errCode == 0)
				{
                    errCode = dmaCtrl_Set_ReLoad(pDma, chn, DMA_SRC_DST, DMA_WORK_MODE_NORMAL);
                }
                break;
				
            case DMA_XFER_TYPE2_ROW2:  /* Dmac_transfer_row2 :*/
                errCode = dmaCtrl_Set_ListPtrAddr(pDma, chn, 0x0);
                if (errCode == 0)
				{
                    errCode = dmaCtrl_Set_BlockChain_En(pDma, chn, DMA_SRC_DST, DMA_WORK_MODE_NORMAL);
                }
                if (errCode == 0)
				{
                    errCode = dmaCtrl_Set_ReLoad(pDma, chn, DMA_SRC, DMA_WORK_MODE_NORMAL);
                }
                if (errCode == 0)
				{
                    errCode = dmaCtrl_Set_ReLoad(pDma, chn, DMA_DST, DMA_WORK_MODE_TEST);
                }
                break;
				
            case DMA_XFER_TYPE3_ROW3:  /* Dmac_transfer_row3 :*/
                errCode = dmaCtrl_Set_ListPtrAddr(pDma, chn, 0x0);
                if (errCode == 0)
				{
                    errCode = dmaCtrl_Set_BlockChain_En(pDma, chn, DMA_SRC_DST, DMA_WORK_MODE_NORMAL);
                }
                if (errCode == 0)
				{
                    errCode = dmaCtrl_Set_ReLoad(pDma, chn, DMA_SRC, DMA_WORK_MODE_TEST);
                }
                if (errCode == 0)
				{
                    errCode = dmaCtrl_Set_ReLoad(pDma, chn, DMA_DST, DMA_WORK_MODE_NORMAL);
                }
                break;
				
            case DMA_XFER_TYPE4_ROW4:  /* Dmac_transfer_row4 :*/
                errCode = dmaCtrl_Set_ListPtrAddr(pDma, chn, 0x0);
                if (errCode == 0)
				{
                    errCode = dmaCtrl_Set_BlockChain_En(pDma, chn, DMA_SRC_DST, DMA_WORK_MODE_NORMAL);
                }
                if (errCode == 0)
				{
                    errCode = dmaCtrl_Set_ReLoad(pDma, chn, DMA_SRC_DST, DMA_WORK_MODE_TEST);
                }
                break;
				
            case DMA_XFER_TYPE5_ROW5:  /* Dmac_transfer_row5 :*/
                errCode = dmaCtrl_Set_BlockChain_En(pDma,chn, DMA_SRC_DST,DMA_WORK_MODE_NORMAL);
                if (errCode == 0)
				{
                    errCode = dmaCtrl_Set_ReLoad(pDma,chn, DMA_SRC_DST,DMA_WORK_MODE_NORMAL);
                }
                break;
				
            case DMA_XFER_TYPE6_ROW6:  /* Dmac_transfer_row6 :*/
                errCode = dmaCtrl_Set_BlockChain_En(pDma,chn, DMA_SRC,DMA_WORK_MODE_NORMAL);
                if (errCode == 0)
				{
                	errCode = dmaCtrl_Set_BlockChain_En(pDma,chn, DMA_DST,DMA_WORK_MODE_TEST);
                }
                if (errCode == 0)
				{
                    errCode = dmaCtrl_Set_ReLoad(pDma,chn, DMA_SRC_DST,DMA_WORK_MODE_NORMAL);
                }
                break;
				
            case DMA_XFER_TYPE7_ROW7:  /* Dmac_transfer_row7 :*/
                errCode = dmaCtrl_Set_BlockChain_En(pDma,chn, DMA_SRC,DMA_WORK_MODE_NORMAL);
                if (errCode == 0)
				{
                    errCode = dmaCtrl_Set_BlockChain_En(pDma,chn, DMA_DST,DMA_WORK_MODE_TEST);
                }
                if (errCode == 0)
				{
                    errCode = dmaCtrl_Set_ReLoad(pDma,chn, DMA_SRC,DMA_WORK_MODE_TEST);
                }
                if (errCode == 0)
				{
                    errCode = dmaCtrl_Set_ReLoad(pDma,chn, DMA_DST,DMA_WORK_MODE_NORMAL);
                }
                break;
				
            case DMA_XFER_TYPE8_ROW8:  /* Dmac_transfer_row8 :*/
                errCode = dmaCtrl_Set_BlockChain_En(pDma,chn, DMA_SRC,DMA_WORK_MODE_TEST);
                if (errCode == 0) 
				{
                    errCode = dmaCtrl_Set_BlockChain_En(pDma,chn, DMA_DST,DMA_WORK_MODE_NORMAL);
                }
                if (errCode == 0) 
				{
                    errCode = dmaCtrl_Set_ReLoad(pDma,chn, DMA_SRC_DST,DMA_WORK_MODE_NORMAL);
                }
                break;
				
            case DMA_XFER_TYPE9_ROW9:  /* Dmac_transfer_row9 :*/
                errCode = dmaCtrl_Set_BlockChain_En(pDma,chn, DMA_SRC,DMA_WORK_MODE_TEST);
                if (errCode == 0)
				{
                    errCode = dmaCtrl_Set_BlockChain_En(pDma,chn, DMA_DST,DMA_WORK_MODE_NORMAL);
                }
                if (errCode == 0)
				{
                    errCode = dmaCtrl_Set_ReLoad(pDma,chn, DMA_SRC,DMA_WORK_MODE_NORMAL);
                }
                if (errCode == 0)
				{
                    errCode = dmaCtrl_Set_ReLoad(pDma,chn, DMA_DST,DMA_WORK_MODE_TEST);
                }
                break;
				
            case DMA_XFER_TYPE10_ROW10:  /* Dmac_transfer_row10 :*/
                errCode = dmaCtrl_Set_BlockChain_En(pDma,chn, DMA_SRC_DST,DMA_WORK_MODE_TEST);
                if (errCode == 0)
				{
                    errCode = dmaCtrl_Set_ReLoad(pDma,chn, DMA_SRC_DST,DMA_WORK_MODE_NORMAL);
                }
                break;
        }
    }
	
    return errCode;
}


/*****************************************************************************
*
* @description
* This function returns whether scatter mode is enabled or disabled
* on the specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument.
*
* @pParams    
*           pDmac is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*
* @return   
*           Enumerated Enabled/Disabled state.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Get_XferPath(vxT_DMA* pDma, int chn)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    UINT8 chnIdx;
    UINT32 reg;
    int retVal;
	
    chnIdx = dmaCtrl_Get_ChnIdx(chn);
    
	reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_CTL_L_OFFSET(chnIdx));
	
	retVal = ((reg & DMAC_CTL_L_TT_FC) >> 20);
	
    return retVal;
}

/*****************************************************************************
*
* @description
* This function sets the transfer device type and flow control
* (TT_FC) for the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument.
*
* @pParams    
*           pDmac is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*           xfer_path Enumerated transfer device type and flow control.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Set_XferPath(vxT_DMA* pDma, int chn, int xfer_path)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int x, errCode = 0;
    UINT32 reg;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (chn == DMA_CHN_ALL)
	{
        chn &= DMAC_CH_ALL_MASK;
    }
    
    /* Check for valid channel number and not busy*/
    errCode = dmaCtrl_Chk_ChnBusy(pDma, chn);
	
    if (errCode == 0)
	{
        /* Check for hard-coded values*/
        for (x = 0; x < pParams->num_channels; x++)
		{
            if (chn & (1 << x))
			{
                if ((pParams->ch_fc[x] == DMAC_DMA_FC_ONLY) &&
                    ((xfer_path == DMA_XFER_PATH4_DEV2MEM_DEVCTRL)    ||
                     (xfer_path == DMA_XFER_PATH5_DEV2DEV_SRCDEVCTRL) ||
                     (xfer_path == DMA_XFER_PATH6_MEM2DEV_DEVCTRL)    ||
                     (xfer_path == DMA_XFER_PATH7_DEV2DEV_DSTDEVCTRL)))
                {
                    errCode = -FMSH_ENOSYS;
                    break;
                }
					 
                if ((pParams->ch_fc[x] == DMAC_SRC_FC_ONLY) &&
                    ((xfer_path != DMA_XFER_PATH4_DEV2MEM_DEVCTRL) &&
                     (xfer_path != DMA_XFER_PATH5_DEV2DEV_SRCDEVCTRL)))
                {
                    errCode = -FMSH_ENOSYS;
                    break;
                }
					 
                if (pParams->ch_fc[x] == DMAC_DST_FC_ONLY &&
                   (xfer_path != DMA_XFER_PATH6_MEM2DEV_DEVCTRL &&
                    xfer_path != DMA_XFER_PATH7_DEV2DEV_DSTDEVCTRL))
                {
                    errCode = -FMSH_ENOSYS;
                    break;
                }
            }
        }
    }
	
    if (errCode == 0)
	{
        for (x = 0; x < pParams->num_channels; x++)
		{
            if (chn & (1 << x))
			{
				reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_CTL_L_OFFSET(x));
				
				if (((reg & DMAC_CTL_L_TT_FC) >> 20) != xfer_path)
				{
					reg = (reg & ~DMAC_CTL_L_TT_FC) | (xfer_path << 20);
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_CTL_L_OFFSET(x), reg);
				}
            }
        }
    }
	
    return errCode;
}

/*****************************************************************************
*
* @description
* This function returns the handshaking mode hardware or software
* on the specified source or destination on the specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument. Only 1, source or destination,
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*           src_dst_sel Enumerated source/destination select.
*
* @return   
*           Enumerated software/hardware handshaking select.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Get_HandShakMode(vxT_DMA* pDma, int chn, int src_dst_sel)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    UINT8 chnIdx;
    UINT32 reg;
    int retVal;

    chnIdx = dmaCtrl_Get_ChnIdx(chn);    

    reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_CFG_L_OFFSET(chnIdx));
	
    if (src_dst_sel == DMA_SRC) 
	{
		retVal = ((reg & DMAC_CFG_L_HS_SEL_SRC) >> 11);
    }
	else
	{
		retVal = ((reg & DMAC_CFG_L_HS_SEL_DST) >> 10);
    }
	
    return retVal;
}


/*****************************************************************************
*
* @description
* This function sets the handshaking mode from hardware to software
* on the specified source and/or destination on the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument. Both source and destination
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*           src_dst_sel Enumerated source/destination select.
*           handshak_mode Enumerated software/hardware handshaking select.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Set_HandShakMode(vxT_DMA* pDma, int chn, int src_dst_sel, int handshak_mode)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int errCode;
    int x;
    int write_en;
    UINT32 reg;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (chn == DMA_CHN_ALL)
	{
        chn &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errCode = dmaCtrl_Chk_ChnRange(pDma, chn);
	
    if (errCode == 0)
	{
        for (x = 0; x < pParams->num_channels; x++)
		{
            if (chn & (1 << x))
			{
                write_en = 0;  /* FALSE;*/
				
				reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_CFG_L_OFFSET(x));
				
                if ((src_dst_sel == DMA_SRC) || (src_dst_sel == DMA_SRC_DST))
				{
					if (((reg & DMAC_CFG_L_HS_SEL_SRC) >> 11) != handshak_mode)
					{
						reg = (reg & ~DMAC_CFG_L_HS_SEL_SRC) | (handshak_mode << 11);
                        write_en = 1;  /* TRUE;*/
                    }
                }
				
                if ((src_dst_sel == DMA_DST) || (src_dst_sel == DMA_SRC_DST))
				{
					if (((reg & DMAC_CFG_L_HS_SEL_DST) >> 10) != handshak_mode)
					{
						reg = (reg & ~DMAC_CFG_L_HS_SEL_DST) | (handshak_mode << 10);
                        write_en = 1;  /* TRUE;*/
                    }
                }
				
                if (write_en)
				{
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_CFG_L_OFFSET(x), reg);
                }
            }
        }
    }
	
    return errCode;
}


/*****************************************************************************
*
* @description
* This function gets configuration parameters in the DMAC's
* channel registers for the specified DMA channel.
* Only 1 DMA channel can be specified for the FDmaPs_channelNumber argument.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*           ch Configuration structure handle
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Get_ChnCfg(vxT_DMA* pDma, int chn, vxT_DMA_CHNCFG* pChnCfg)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int errCode = 0;
    UINT8 chnIdx;
    UINT32 regVal;
	
    chnIdx = dmaCtrl_Get_ChnIdx(chn);

    /* Check for valid channel number - can only specify one channel */
    if ((DMAC_CH_MASK & chn) 
		|| (chn == DMA_CHN_NONE) 
		|| (chnIdx == DMAC_MAX_CHANNELS))
    {
        errCode = -FMSH_ECHRNG;
    }

    if (errCode == 0)
	{
		/**/
        /* read Control register*/
        /**/
		regVal = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_CTL_L_OFFSET(chnIdx));
		
        pChnCfg->ctl_llp_src_en     = ((regVal & DMAC_CTL_L_LLP_SRC_EN) >> 28);
        pChnCfg->ctl_llp_dst_en     = ((regVal & DMAC_CTL_L_LLP_DST_EN) >> 27);
		
        pChnCfg->ctl_sms            = ((regVal & DMAC_CTL_L_SMS) >> 25);
        pChnCfg->ctl_dms            = ((regVal & DMAC_CTL_L_DMS) >> 23);
		
        pChnCfg->ctl_src_msize      = ((regVal & DMAC_CTL_L_SRC_MSIZE) >> 14);
        pChnCfg->ctl_dst_msize      = ((regVal & DMAC_CTL_L_DEST_MSIZE) >> 11);
		
        pChnCfg->ctl_sinc           = ((regVal & DMAC_CTL_L_SINC) >> 9);
        pChnCfg->ctl_dinc           = ((regVal & DMAC_CTL_L_DINC) >> 7);
		
        pChnCfg->ctl_src_tr_width   = ((regVal & DMAC_CTL_L_SRC_TR_WIDTH) >> 4);
        pChnCfg->ctl_dst_tr_width   = ((regVal & DMAC_CTL_L_DST_TR_WIDTH) >> 1);
		
        pChnCfg->ctl_xfer_path          = ((regVal & DMAC_CTL_L_TT_FC) >> 20);
        pChnCfg->ctl_dst_scatter_en = ((regVal & DMAC_CTL_L_DST_SCATTER_EN) >> 18);
        pChnCfg->ctl_src_gather_en  = ((regVal & DMAC_CTL_L_SRC_GATHER_EN) >> 17);
        pChnCfg->ctl_int_en         =  (regVal & DMAC_CTL_L_INT_EN);


		regVal = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_CTL_H_OFFSET(chnIdx));
		
        pChnCfg->ctl_done = (enum FMSH_state)((regVal & DMAC_CTL_H_DONE) >> 12);
        pChnCfg->ctl_block_ts = regVal & DMAC_CTL_H_BLOCK_TS;

		/**/
        /* read Config register*/
        /**/
		regVal = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_CFG_L_OFFSET(chnIdx));
		
        pChnCfg->cfg_ch_prior  = ((regVal & DMAC_CFG_L_CH_PRIOR) >> 5);
		
        pChnCfg->cfg_lock_ch_l = ((regVal & DMAC_CFG_L_LOCK_CH_L) >> 12);
        pChnCfg->cfg_lock_b_l  = ((regVal & DMAC_CFG_L_LOCK_B_L) >> 14);
        pChnCfg->cfg_lock_ch   = ((regVal & DMAC_CFG_L_LOCK_CH) >> 16);
        pChnCfg->cfg_lock_b    = ((regVal & DMAC_CFG_L_LOCK_B) >> 17);
		
		pChnCfg->cfg_max_abrst =  (regVal & DMAC_CFG_L_MAX_ABRST) >> 20;
        
        pChnCfg->cfg_hs_sel_dst = ((regVal & DMAC_CFG_L_HS_SEL_DST) >> 10);
        pChnCfg->cfg_hs_sel_src = ((regVal & DMAC_CFG_L_HS_SEL_SRC) >> 11);
		
        pChnCfg->cfg_dst_hs_pol = ((regVal & DMAC_CFG_L_DST_HS_POL) >> 18);
        pChnCfg->cfg_src_hs_pol = ((regVal & DMAC_CFG_L_SRC_HS_POL) >> 19);
		
        pChnCfg->cfg_reload_dst = ((regVal & DMAC_CFG_L_RELOAD_DST) >> 31);
        pChnCfg->cfg_reload_src = ((regVal & DMAC_CFG_L_RELOAD_SRC) >> 30);

		regVal = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_CFG_H_OFFSET(chnIdx));
		
        pChnCfg->cfg_ds_upd_en = ((regVal & DMAC_CFG_H_DS_UPD_EN) >> 5);
        pChnCfg->cfg_ss_upd_en = ((regVal & DMAC_CFG_H_SS_UPD_EN) >> 6);
		
        pChnCfg->cfg_src_per   = ((regVal & DMAC_CFG_H_SRC_PER) >> 7);
        pChnCfg->cfg_dst_per   = ((regVal & DMAC_CFG_H_DEST_PER) >> 11);
		
        pChnCfg->cfg_fcmode    = (regVal & DMAC_CFG_H_FCMODE);
        pChnCfg->cfg_fifo_mode = ((regVal & DMAC_CFG_H_FIFO_MODE) >> 1);
        pChnCfg->cfg_protctl   = ((regVal & DMAC_CFG_H_PROTCTL) >> 2);

		/**/
        /* read SAR/DAR registers*/
        /**/
        pChnCfg->sar = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_SAR_L_OFFSET(chnIdx));
        pChnCfg->dar = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_DAR_L_OFFSET(chnIdx));

		/**/
        /* read LLP register*/
        /**/
        if (pParams->ch_hc_llp[chnIdx] == 0x0)
		{
			regVal = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_LLP_L_OFFSET(chnIdx));
            pChnCfg->llp_lms = (regVal & DMAC_LLP_L_LMS);
            pChnCfg->llp_loc = (regVal & DMAC_LLP_L_LOC) >> 2;
        }
		else
		{
            pChnCfg->llp_lms = 0x0;
            pChnCfg->llp_loc = 0x0;
        }

        /**/
        /* read SSTAT/DSTAT SSTATAR/DSTATAR registers*/
        /**/
        if (pParams->ch_stat_src[chnIdx] == 0x1)
		{
            pChnCfg->sstat = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_SSTAT_L_OFFSET(chnIdx));
            pChnCfg->sstatar = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_SSTATAR_L_OFFSET(chnIdx));
        }
		else
		{
            pChnCfg->sstat = 0x0;
            pChnCfg->sstatar = 0x0;
        }

        if (pParams->ch_stat_dst[chnIdx] == 0x1)
		{
            pChnCfg->dstat = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_DSTAT_L_OFFSET(chnIdx));
            pChnCfg->dstatar = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_DSTATAR_L_OFFSET(chnIdx));
        }
		else
		{
            pChnCfg->dstat = 0x0;
            pChnCfg->dstatar = 0x0;
        }

        /**/
        /* read SGR register*/
        /**/
        if (pParams->ch_src_gat_en[chnIdx] == 0x1)
		{
            regVal = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_SGR_L_OFFSET(chnIdx));
			
            pChnCfg->sgr_sgc = (regVal & DMAC_SGR_L_SGC) >> 20;
            pChnCfg->sgr_sgi = regVal & DMAC_SGR_L_SGI;
        }
		else
		{
            pChnCfg->sgr_sgc = 0x0;
            pChnCfg->sgr_sgi = 0x0;
        }

        /**/
        /* read the DSR register*/
        /**/
        if (pParams->ch_dst_sca_en[chnIdx] == 0x1)
		{
            regVal = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_DSR_L_OFFSET(chnIdx));
			
            pChnCfg->dsr_dsc = (regVal & DMAC_DSR_L_DSC) >> 20;
            pChnCfg->dsr_dsi = regVal & DMAC_DSR_L_DSI;
        }
		else
		{
            pChnCfg->dsr_dsc = 0x0;
            pChnCfg->dsr_dsi = 0x0;
        }
    }
	
    return errCode;
}

/*****************************************************************************
*
* @description
* This function sets configuration parameters in the DMAC's
* channel registers on the specified DMA channel.
* Only 1 DMA channel can be specified for the FDmaPs_channelNumber argument.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*           ch Configuration structure handle
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Set_ChnCfg(vxT_DMA* pDma, int chn, vxT_DMA_CHNCFG* pChnCfg)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int errCode = 0;
    UINT8 chnIdx;
    UINT32 regVal;

    chnIdx = dmaCtrl_Get_ChnIdx(chn);

    /* Check for valid channel number and not busy*/
    errCode = dmaCtrl_Chk_ChnBusy(pDma, chn);
	
    if (errCode == 0)
	{
        /* check for out of range values*/
        if (pChnCfg->llp_lms > (pParams->num_master_int-1))
		{
            errCode = -FMSH_EINVAL;
        }
		
        if ((pChnCfg->ctl_sms > (pParams->num_master_int-1))
			|| (pChnCfg->ctl_dms > (pParams->num_master_int-1)))
        {
            errCode = -FMSH_EINVAL;
        }
			
        if ((pParams->ch_fc[chnIdx] == DMAC_DMA_FC_ONLY) && 
			((pChnCfg->ctl_xfer_path == DMA_XFER_PATH4_DEV2MEM_DEVCTRL)   
			  || (pChnCfg->ctl_xfer_path == DMA_XFER_PATH5_DEV2DEV_SRCDEVCTRL)
			  || (pChnCfg->ctl_xfer_path == DMA_XFER_PATH6_MEM2DEV_DEVCTRL)
			  || (pChnCfg->ctl_xfer_path == DMA_XFER_PATH7_DEV2DEV_DSTDEVCTRL)))
        {
            errCode = -FMSH_EINVAL;
        }
			  
        if ((pParams->ch_fc[chnIdx] == DMAC_SRC_FC_ONLY) &&
             ((pChnCfg->ctl_xfer_path != DMA_XFER_PATH4_DEV2MEM_DEVCTRL)
               && (pChnCfg->ctl_xfer_path != DMA_XFER_PATH5_DEV2DEV_SRCDEVCTRL)))
        {
            errCode = -FMSH_EINVAL;
        }
			   
        if ((pParams->ch_fc[chnIdx] == DMAC_DST_FC_ONLY) &&
             ((pChnCfg->ctl_xfer_path != DMA_XFER_PATH6_MEM2DEV_DEVCTRL) 
               && (pChnCfg->ctl_xfer_path != DMA_XFER_PATH7_DEV2DEV_DSTDEVCTRL)))
        {
            errCode = -FMSH_EINVAL;
        }
			
        if (((1 << (pChnCfg->ctl_src_msize + 1)) > (pParams->ch_max_mult_size[chnIdx])) 
			|| ((1 << (pChnCfg->ctl_dst_msize + 1)) > (pParams->ch_max_mult_size[chnIdx])))
        {
            errCode = -FMSH_EINVAL;
        }
			
        if ((pChnCfg->cfg_dst_per >= pParams->num_hs_int) 
			|| (pChnCfg->cfg_src_per >= pParams->num_hs_int))
        {
            errCode = -FMSH_EINVAL;
        }
		   
        if (pChnCfg->cfg_ch_prior > (pParams->num_channels - 1))
		{
            errCode = -FMSH_EINVAL;
        }
    }

    if (errCode == 0x0)
	{
		/**/
        /* Set Control register*/
        /**/
		regVal = (pChnCfg->ctl_int_en) 
		      | (pChnCfg->ctl_dst_tr_width << 1)
		      | (pChnCfg->ctl_src_tr_width << 4) 
		      | (pChnCfg->ctl_dinc << 7) 
		      | (pChnCfg->ctl_sinc << 9) 
		      | (pChnCfg->ctl_dst_msize << 11)
		      | (pChnCfg->ctl_src_msize << 14) 
		      | (pChnCfg->ctl_src_gather_en << 17)
		      | (pChnCfg->ctl_dst_scatter_en << 18)
		      | (pChnCfg->ctl_xfer_path << 20) 
		      | (pChnCfg->ctl_dms << 23) 
		      | (pChnCfg->ctl_sms << 25) 
		      | (pChnCfg->ctl_llp_dst_en << 27) 
		      | (pChnCfg->ctl_llp_src_en << 28);
		
		dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_CTL_L_OFFSET(chnIdx), regVal);

        regVal = pChnCfg->ctl_block_ts | (pChnCfg->ctl_done << 12);
		dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_CTL_H_OFFSET(chnIdx), regVal);

		/**/
        /* Set Config register*/
        /**/
        regVal = (pChnCfg->cfg_ch_prior << 5) |
	        (pChnCfg->cfg_hs_sel_dst << 10) |
	        (pChnCfg->cfg_hs_sel_src << 11) |
	        (pChnCfg->cfg_lock_ch_l << 12) |
	        (pChnCfg->cfg_lock_b_l << 14) |
	        (pChnCfg->cfg_lock_ch << 16) |
	        (pChnCfg->cfg_lock_b << 17) |
	        (pChnCfg->cfg_dst_hs_pol << 18) |
	        (pChnCfg->cfg_src_hs_pol << 19) |
	        (pChnCfg->cfg_max_abrst << 20) |
	        (pChnCfg->cfg_reload_src << 30) |
	        (pChnCfg->cfg_reload_dst << 31);
		
		dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_CFG_L_OFFSET(chnIdx), regVal);

        regVal = (pChnCfg->cfg_fcmode) |
	        (pChnCfg->cfg_fifo_mode << 1) |
	        (pChnCfg->cfg_protctl << 2) |
	        (pChnCfg->cfg_ds_upd_en << 5) |
	        (pChnCfg->cfg_ss_upd_en << 6) |
	        (pChnCfg->cfg_src_per << 7) |
	        (pChnCfg->cfg_dst_per << 11);
		
		dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_CFG_H_OFFSET(chnIdx), regVal);

		/**/
        /* set SAR/DAR registers*/
        /**/
		dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_SAR_L_OFFSET(chnIdx), pChnCfg->sar);
		dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_DAR_L_OFFSET(chnIdx), pChnCfg->dar);

		/**/
        /* set LLP register (Linked List Pointer)*/
        /**/
        if (pParams->ch_hc_llp[chnIdx] == 0x0)
		{
            regVal = (pChnCfg->llp_lms & 0x3) | (pChnCfg->llp_loc << 2);
			dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_LLP_L_OFFSET(chnIdx), regVal);
        }

		/**/
        /* set SGR register (Source Gather Register)*/
        /**/
        if (pParams->ch_src_gat_en[chnIdx] == 0x1)
		{
            regVal = (pChnCfg->sgr_sgi & 0xFFFFF) | (pChnCfg->sgr_sgc << 20);
			dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_SGR_L_OFFSET(chnIdx), regVal);
        }

		/**/
        /* set DSR register (Destination Scatter Register)*/
        /**/
        if (pParams->ch_dst_sca_en[chnIdx] == 0x1)
		{
			regVal = (pChnCfg->dsr_dsi & 0xFFFFF) | (pChnCfg->dsr_dsc << 20);
			dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_DSR_L_OFFSET(chnIdx), regVal);
        }

		/**/
        /* set SSTAT/DSTAT SSTATAR/DSTATAR registers*/
        /**/
        if (pParams->ch_stat_src[chnIdx] == 0x1)
		{
			dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_SSTAT_L_OFFSET(chnIdx), pChnCfg->sstat);
			dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_SSTATAR_L_OFFSET(chnIdx), pChnCfg->sstatar);
        }
		
        if (pParams->ch_stat_dst[chnIdx] == 0x1)
		{
			dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_DSTAT_L_OFFSET(chnIdx), pChnCfg->dstat);
			dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_DSTATAR_L_OFFSET(chnIdx), pChnCfg->dstatar);
        }
    }
	
    return errCode;
}

/*****************************************************************************
*
* @description
* This function returns the specified source or destination
* transfer xfer_width on the specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument. Only 1, source or destination,
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*           src_dst_sel Enumerated source/destination select.
*
* @return   
*           Enumerated transfer xfer_width.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Get_XferWidth(vxT_DMA* pDma, int chn, int src_dst_sel)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    UINT8 chnIdx;
    UINT32 reg;
    int ret;
	
    chnIdx = dmaCtrl_Get_ChnIdx(chn);

    reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_CTL_L_OFFSET(chnIdx));
	
    if (src_dst_sel == DMA_SRC) 
	{
		ret = ((reg & DMAC_CTL_L_SRC_TR_WIDTH) >> 4);
    }
	else
	{
		ret = ((reg & DMAC_CTL_L_DST_TR_WIDTH) >> 1);
    }
	
    return ret;
}


/*****************************************************************************
*
* @description
* This function sets the specified source and/or destination
* transfer xfer_width on the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument. Both source and destination
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*           src_dst_sel Enumerated source/destination select.
*           xfer_width Enumerated transfer xfer_width.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Set_XferWidth(        vxT_DMA* pDma, int chn, int src_dst_sel, int xfer_width)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int errCode = 0;
    int x;
    int write_en;
    UINT32 reg;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (chn == DMA_CHN_ALL)
	{
        chn &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errCode = dmaCtrl_Chk_ChnBusy(pDma, chn);
	
    if (errCode == 0)
	{
        /* Check if the field is hard-coded*/
        for (x = 0; x < pParams->num_channels; x++)
		{
            if (chn & (1 << x))
			{
                if ((src_dst_sel == DMA_SRC) || (DMA_SRC_DST))
                /*if ((src_dst_sel == DMA_SRC) || (src_dst_sel == DMA_SRC_DST))*/
				{
                    if (pParams->ch_stw[x] != 0x0)
					{
                        errCode = -FMSH_ENOSYS;
                        break;
                    }
                }
				
                if ((src_dst_sel == DMA_DST) || (DMA_SRC_DST))
                /*if ((src_dst_sel == DMA_DST) || (src_dst_sel == DMA_SRC_DST))*/
				{
                    if (pParams->ch_dtw[x] != 0x0)
					{
                        errCode = -FMSH_ENOSYS;
                        break;
                    }
                }
            }
        }
    }
	
    if (errCode == 0) 
	{
        for (x = 0; x < pParams->num_channels; x++)
		{
            if (chn & (1 << x))
			{
                write_en = 0;  /* FALSE;*/
				
				reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_CTL_L_OFFSET(x));
				
                if ((src_dst_sel == DMA_SRC) || (src_dst_sel == DMA_SRC_DST))
				{
					if (((reg & DMAC_CTL_L_SRC_TR_WIDTH) >> 4) != xfer_width)
					{
						reg = (reg & ~DMAC_CTL_L_SRC_TR_WIDTH) | (xfer_width << 4);
                        write_en = 1;  /* TRUE;*/
                    }
                }
				
                if ((src_dst_sel == DMA_DST) || (src_dst_sel == DMA_SRC_DST))
				{
					if (((reg & DMAC_CTL_L_DST_TR_WIDTH) >> 1) != xfer_width)
					{
						reg = (reg & ~DMAC_CTL_L_DST_TR_WIDTH) | (xfer_width << 1);
                        write_en = 1;  /* TRUE;*/
                    }
                }
				
                if (write_en)
				{
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_CTL_L_OFFSET(x), reg);
                }
            }
        }
    }
	
    return errCode;
}



/*****************************************************************************
*
* @description
* This function returns the specified source or destination
* burst size on the specified DMA channel. 
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument. Only 1, source or destination,
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*           src_dst_sel Enumerated source/destination select.
*
* @return   
*           Enumerated burst size.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Get_BurstSize(vxT_DMA* pDma, int chn, int src_dst_sel)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    UINT8 chnIdx;
    UINT32 reg;
    int ret;
	
    chnIdx = dmaCtrl_Get_ChnIdx(chn);

    reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_CTL_L_OFFSET(chnIdx));
	
    if (src_dst_sel == DMA_SRC)
	{
        ret = ((reg & DMAC_CTL_L_SRC_MSIZE) >> 14);
    }
	else
	{
        ret = ((reg & DMAC_CTL_L_DEST_MSIZE) >> 11);
    }
	
    return ret;
}


/*****************************************************************************
*
* @description
* This function sets the specified source and/or destination
* burst size on the specified DMA channel(s). 
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument. Both source and destination
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*           src_dst_sel Enumerated source/destination select.
*           length Enumerated burst size.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Set_BurstSize(vxT_DMA* pDma, int chn, int src_dst_sel, int burst_size)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int errCode = 0;
    int x;
    int write_en;
    UINT32 reg;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (chn == DMA_CHN_ALL)
	{
        chn &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errCode = dmaCtrl_Chk_ChnBusy(pDma, chn);
	
    if (errCode == 0)
	{
        /* Check if the specified value is in range*/
        for (x = 0; x < pParams->num_channels; x++)
        {
            if (chn & (1 << x))
			{
                if ((1 << (burst_size + 1)) > pParams->ch_max_mult_size[x])
				{
                    errCode = -FMSH_EINVAL;
                    break;
                }
            }
        }
    }
	
    if (errCode == 0)
	{
        for (x = 0; x < pParams->num_channels; x++)
		{
            if (chn & (1 << x))
			{
                write_en = 0;  /* FALSE;*/
				
				reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_CHX_CTL_L_OFFSET(x));
				
                if ((src_dst_sel == DMA_SRC) || (src_dst_sel == DMA_SRC_DST))
				{
					if (((reg & DMAC_CTL_L_SRC_MSIZE) >> 14) != burst_size)
					{
						reg = (reg & ~DMAC_CTL_L_SRC_MSIZE) | (burst_size << 14);
						write_en = 1;  /* TRUE;*/
					}
                }
				
                if ((src_dst_sel == DMA_DST) || (src_dst_sel == DMA_SRC_DST))
				{
					if(((reg & DMAC_CTL_L_DEST_MSIZE) >> 11) != burst_size)
					{
						reg = (reg & ~DMAC_CTL_L_DEST_MSIZE) | (burst_size << 11);
						write_en = 1;  /* TRUE;*/
					}
                }
				
                if (write_en)
				{
					dmaCtrl_Wr_CfgReg32(pDma, DMAC_CHX_CTL_L_OFFSET(x), reg);
                }
            }
        }
    }
	
    return errCode;
}



/*****************************************************************************
*
* @description
* This function will enable the DMA controller.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*
* @return   
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void dmaCtrl_Enable_Ctrl(vxT_DMA* pDma)
{	
	/*
	0x398:
	------
	bit0: DMA_EN	R/W	0x0	DMAC使能位。
		0 = DMAC禁用
		1 = DMAC启用。
	*/
	dmaCtrl_Wr_CfgReg32(pDma, DMAC_DMA_CFG_REG_L_OFFSET, 0x1);
	return;
}

/*****************************************************************************
*
* @description
* This function will disable the dma controller.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
void dmaCtrl_Disable_Ctrl(vxT_DMA* pDma)
{
    int errCode = 0;
    UINT32 reg;

    /* Check first to see if DMA is already disabled*/
	reg = dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_CFG_REG_L_OFFSET);
	reg &= DMAC_DMACFGREG_L_DMA_EN;

    if (reg != 0x0)
	{
        /*reg = 0x0;*/
		dmaCtrl_Wr_CfgReg32(pDma, DMAC_DMA_CFG_REG_L_OFFSET, 0x0);

        /* Ensure that the DMA was disabled*/
        /* May not disable due to split response on one*/
        /* of the DMA channels        */
		if (dmaCtrl_Rd_CfgReg32(pDma, DMAC_DMA_CFG_REG_L_OFFSET))
		{
			errCode = -FMSH_EBUSY;
		}
    }
	
    return errCode;
}

/*****************************************************************************
*
* @description
* This function enables the specified DMA channel(s).
* Multiple DMA channels can be specified for the FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
 int dmaCtrl_Enable_ChnX(vxT_DMA* pDma, int chn)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int errCode;
    UINT32 reg;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (chn == DMA_CHN_ALL)
	{
        chn &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errCode = dmaCtrl_Chk_ChnBusy(pDma, chn);
	
    if (errCode == 0)
	{
        /* The FDmaPs_channelNumber enum is declared such that*/
        /* the enumerated value maps exactly to the value that*/
        /* needs to be written into the ChEnReg for enabling.*/
        reg = chn;
		dmaCtrl_Wr_CfgReg32(pDma, DMAC_CH_EN_REG_L_OFFSET, reg);
    }
	
    return errCode;
}

/*****************************************************************************
*
* @description
* This function is used to start an interrupt-driven transfer on a
* DMA channel.  Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument.
*
* The function enables DMA channel interrupts and stores
* information needed by the IRQ Handler to control the transfer.
* The DMA channel is also enabled to begin the DMA transfer. The
* following channel interrupts are enabled and unmasked by this function:
*
*    IntTfr - transfer complete interrupt
*    IntBlock - block transfer complete interrupt
*    IntErr - error response on the AMBA AHB bus
*
* If software handshaking is used on the source and the source
* device is a peripheral, the following interrupt is unmasked. If
* the transfer set up does not match that described and the user
* wants to use this interrupt, the user should unmask the
* interrupt using the FDmaPs_unmaskIrq() function prior to calling
* this function.
*  
*    IntSrcTran - source burst/single tranfer completed
*
* If software handshaking is used on the destination and the
* destination device is a peripheral, the following interrupt
* is unmasked. If the transfer setup does not match that described
* and the user wants to use this interrupt, the user should
* unmask the interrupt using the FDmaPs_unmaskIrq() function prior
* to calling this function.
*
*    IntDstTran - destination burst/single tranfer completed
*  
* All channel interrupts are masked and disabled on completion of
* the DMA transfer.
*
* If the number of blocks that make up the DMA transfer is not known,
* the user should enter 0 for the blockNum argument. The user's
* listener function is called by the FDmaPs_irqHandler() function
* each time a block interrupt occurs. The user can use the
* FDmaPs_getBlockCount() API function to fetch the number of blocks
* completed by the DMA Controller from within the listener function.
* When the total number of blocks is known, the user should call the
* FDmaPs_nextBlockIsLast() function also from within the Listener function.
* The listener function has two arguments, the DMAC device handle
* and the interrupt type (FDmaPs_irq).
*  
* At the end of the DMA transfer, the FDmaPs_irqHandler() calls
* the user's callback function if the user has specified one. The
* callback function has two arguments: the DMAC device handle and
* the number of blocks transferred by the DMA Controller.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*           chn Enumerated DMA channel number.
*           blockNum Number of blocks in the DMA transfer.
*           cb_func User callback function (can be NULL) - called by ISR.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Start_Xfer(vxT_DMA* pDma, int chn,          int blockNum)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int errCode = 0;
    UINT8 chnIdx;

    /* Update the channel instance*/
    chnIdx = dmaCtrl_Get_ChnIdx(chn);
	
    /* Allow only ONE channel to be specified*/
    if (chnIdx == DMAC_MAX_CHANNELS)
	{
        errCode = -FMSH_ECHRNG;
    }
	else
	{
        /* Check for valid channel number and not busy*/
        errCode = dmaCtrl_Chk_ChnBusy(pDma, chn);
    }

    if (errCode == 0)
	{
        /* Disable the channels interrupts*/
        errCode = dmaCtrl_Dis_ChnIRQ(pDma, chn);
    }

    if (errCode == 0)
	{
		/* set the call back function, the number of blocks*/
		/* in the transfer and the source and destination states.*/
        /*pDma->pDmaCtrl->CHN[chnIdx].userCallback = cb_func;*/
        pDma->pDmaCtrl->CHN[chnIdx].total_blocks = blockNum;
		
        pDma->pDmaCtrl->CHN[chnIdx].src_state = DMA_STATE_BURST_REGION;
        pDma->pDmaCtrl->CHN[chnIdx].dst_state = DMA_STATE_BURST_REGION;

		/* store the transfer type.*/
		pDma->pDmaCtrl->CHN[chnIdx].xfer_type = dmaCtrl_Get_XferType(pDma, chn);
	
		/* initiatize the block / byte count variables*/
        pDma->pDmaCtrl->CHN[chnIdx].block_cnt = 0;
        pDma->pDmaCtrl->CHN[chnIdx].src_byte_cnt = 0;
        pDma->pDmaCtrl->CHN[chnIdx].dst_byte_cnt = 0;

		/* set the increment value for the source when in*/
		/* the single transaction region.*/
        pDma->pDmaCtrl->CHN[chnIdx].src_single_inc = (POW2(dmaCtrl_Get_XferWidth(pDma, chn, DMA_SRC) + 3) / 8);
	
		/* set the increment value for the source when NOT in*/
		/* the single transaction region.*/
        pDma->pDmaCtrl->CHN[chnIdx].src_burst_inc  = (pDma->pDmaCtrl->CHN[chnIdx].src_single_inc * 
        										       POW2(dmaCtrl_Get_BurstSize(pDma, chn, DMA_SRC) + 1));
	
		/* set the increment value for the destination when in*/
		/* the single transaction region.*/
        pDma->pDmaCtrl->CHN[chnIdx].dst_single_inc = (POW2(dmaCtrl_Get_XferWidth(pDma, chn, DMA_DST) + 3) / 8);
	
		/* set the increment value for the destination when NOT in*/
		/* the single transaction region.*/
        pDma->pDmaCtrl->CHN[chnIdx].dst_burst_inc  = (pDma->pDmaCtrl->CHN[chnIdx].dst_single_inc *
												       POW2(dmaCtrl_Get_BurstSize(pDma, chn, DMA_DST) + 1));
	
        /* always want to unmask the tfr, block and err interrupts*/
        /*FDmaPs_unmaskIrq(pDma, chn, Dmac_irq_tfr);*/
        /*FDmaPs_unmaskIrq(pDma, chn, Dmac_irq_block);*/
        /*FDmaPs_unmaskIrq(pDma, chn, Dmac_irq_err);*/
        dmaCtrl_UnMask_IRQ(pDma, chn, DMA_IRQ_XFER_DONE);
        dmaCtrl_UnMask_IRQ(pDma, chn, DMA_IRQ_BLK_XFER_DONE);
        dmaCtrl_UnMask_IRQ(pDma, chn, DMA_IRQ_ERR);

		pDma->pDmaCtrl->CHN[chnIdx].xfer_path = dmaCtrl_Get_XferPath(pDma, chn);

		/* Unmask the srctran interrupt if the the source is using*/
		/* software handshaking and the source device is a peripheral*/
		if (dmaCtrl_Get_HandShakMode(pDma, chn, DMA_SRC) == DMA_SOFT_IF) /* Dmac_hs_software*/
	    {
        	if ((pDma->pDmaCtrl->CHN[chnIdx].xfer_path == DMA_XFER_PATH2_DEV2MEM_DMACTRL) ||
                (pDma->pDmaCtrl->CHN[chnIdx].xfer_path == DMA_XFER_PATH4_DEV2MEM_DEVCTRL) ||
                (pDma->pDmaCtrl->CHN[chnIdx].xfer_path == DMA_XFER_PATH3_DEV2DEV_DMACTRL) ||
                (pDma->pDmaCtrl->CHN[chnIdx].xfer_path == DMA_XFER_PATH5_DEV2DEV_SRCDEVCTRL) ||
                (pDma->pDmaCtrl->CHN[chnIdx].xfer_path == DMA_XFER_PATH7_DEV2DEV_DSTDEVCTRL))
        	{
                dmaCtrl_UnMask_IRQ(pDma, chn, DMA_IRQ_SRC_XFER_DONE);  /* Dmac_irq_srctran*/
	    	}
        }
	    
		/* Unmask the dsttran interrupt if the the destination is*/
		/* using software handshaking and the destination device is*/
		/* a perihperal*/
		if (dmaCtrl_Get_HandShakMode(pDma, chn, DMA_DST) == DMA_SOFT_IF)  /* Dmac_hs_software*/
		{
            if ((pDma->pDmaCtrl->CHN[chnIdx].xfer_path == DMA_XFER_PATH1_MEM2DEV_DMACTRL)    ||
                (pDma->pDmaCtrl->CHN[chnIdx].xfer_path == DMA_XFER_PATH6_MEM2DEV_DEVCTRL)    ||
                (pDma->pDmaCtrl->CHN[chnIdx].xfer_path == DMA_XFER_PATH3_DEV2DEV_DMACTRL)    ||
                (pDma->pDmaCtrl->CHN[chnIdx].xfer_path == DMA_XFER_PATH5_DEV2DEV_SRCDEVCTRL) ||
                (pDma->pDmaCtrl->CHN[chnIdx].xfer_path == DMA_XFER_PATH7_DEV2DEV_DSTDEVCTRL))
            {
                dmaCtrl_UnMask_IRQ(pDma, chn, DMA_IRQ_DST_XFER_DONE);  /* Dmac_irq_dsttran*/
	    	}
		}
 
        /* Enable the channel interrupts for the type of transfer*/
        errCode = dmaCtrl_En_ChnIRQ(pDma, chn);

		if (errCode == 0)
		{
            /* Enable the DMA channel*/
            errCode = dmaCtrl_Enable_ChnX(pDma, chn);
		}
    }
	
    return errCode;
}


/*****************************************************************************
*
* @description
* This functions resets the FDmaPs_Instance_T structure.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*
* @return   
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void dmaCtrl_PreInit(vxT_DMA* pDma)
{
	vxT_DMA_PARAMS* pParams = (vxT_DMA_PARAMS*)(&pDma->pDmaCtrl->DmaParams.header);
	
    int i;

    for (i=0; i<pParams->num_channels; i++)
	{
        switch (i)
		{
            case 0: 
				pDma->pDmaCtrl->CHN[i].chn_x = DMA_CHN_0;
				break;
            case 1: 
				pDma->pDmaCtrl->CHN[i].chn_x = DMA_CHN_1; 
				break;
            case 2:
				pDma->pDmaCtrl->CHN[i].chn_x = DMA_CHN_2; 
				break;
            case 3: 
				pDma->pDmaCtrl->CHN[i].chn_x = DMA_CHN_3; 
				break;
            case 4: 
				pDma->pDmaCtrl->CHN[i].chn_x = DMA_CHN_4; 
				break;
            case 5: 
				pDma->pDmaCtrl->CHN[i].chn_x = DMA_CHN_5; 
				break;
            case 6: 
				pDma->pDmaCtrl->CHN[i].chn_x = DMA_CHN_6; 
				break;
            case 7: 
				pDma->pDmaCtrl->CHN[i].chn_x = DMA_CHN_7; 
				break;
        }

        pDma->pDmaCtrl->CHN[i].src_state      = DMA_STATE_IDLE; /* Dmac_idle;*/
        pDma->pDmaCtrl->CHN[i].dst_state      = DMA_STATE_IDLE;
		
        pDma->pDmaCtrl->CHN[i].block_cnt      = 0;
        pDma->pDmaCtrl->CHN[i].total_blocks   = 0;
        pDma->pDmaCtrl->CHN[i].src_byte_cnt   = 0;
        pDma->pDmaCtrl->CHN[i].dst_byte_cnt   = 0;
        pDma->pDmaCtrl->CHN[i].src_single_inc = 0;
        pDma->pDmaCtrl->CHN[i].src_burst_inc  = 0;
        pDma->pDmaCtrl->CHN[i].dst_single_inc = 0;
        pDma->pDmaCtrl->CHN[i].dst_burst_inc  = 0;
		
        pDma->pDmaCtrl->CHN[i].xfer_type = DMA_XFER_TYPE1_ROW1;            /* Dmac_transfer_row1;*/
        pDma->pDmaCtrl->CHN[i].xfer_path = DMA_XFER_PATH0_MEM2MEM_DMACTRL; /* Dmac_mem2mem_dma;*/
    }

    /* Set the channel priority order*/
    dmaCtrl_Set_ChnPriority(pDma);

	return;
}



/*****************************************************************************
*
* @description
* This function is used to initialize the DMA controller. All
* interrupts are cleared and disabled; DMA channels are disabled; and
* the device instance structure is reset.
*
* @pParams    
*           pDma is the pointer to the DMA controller device.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int dmaCtrl_Init(vxT_DMA* pDma)
{
    int errCode;

    /* Remove from test mode*/
    dmaCtrl_Set_TestMode(pDma, DMA_WORK_MODE_NORMAL);
    
    /* Reset the DMA instance structure*/
    dmaCtrl_PreInit(pDma);
    
    /* Disable the DMA controller*/
    errCode = dmaCtrl_Dis_Ctrl(pDma);
    if (errCode == 0)
	{
        /* Disable all DMA channels*/
        errCode = dmaCtrl_Dis_ChnX(pDma, DMA_CHN_ALL);
    }
	
    if (errCode == 0)
	{
        /* Disable all channel interrupts*/
        errCode = dmaCtrl_Dis_ChnIRQ(pDma, DMA_CHN_ALL);
    }
	
    if (errCode == 0)
	{
        /* Mask all channel interrupts*/
        errCode = dmaCtrl_Mask_IRQ(pDma, DMA_CHN_ALL, DMA_IRQ_ALL);
    }
	
    if (errCode == 0)
	{
        /* Clear any pending interrupts*/
         errCode = dmaCtrl_Clr_IRQ(pDma, DMA_CHN_ALL, DMA_IRQ_ALL);
    }

    return errCode;
}

#endif

#if 0

FMSH_LIST_HEAD(gDMA_List);

int gDMA_ListNum = 0;

/*****************************************************************************
*
* @description
* This function creates a Linked List Item or appends a current linked
* list with a new item. The FDmaPs_ChannelConfig_T structure handle
* contains the values for the FDmaPs_lli_item structure members.
*
* @param    
*           pListHead Handle to a dw_list_head structure.
*           pLLItem Handle to a FDmaPs_lli_item structure.
*           pChnCfg Handle to a FDmaPs_ChannelConfig_T structure.
*
* @return   
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void dmaCtrl_Add_LinkListItem(        FMSH_listHead* pListHead, vxT_LINKLIST_ITEM* pLLItem,     vxT_DMA_CHNCFG* pChnCfg)
{
    vxT_LINKLIST_ITEM * prev_entry;

    FMSH_listAddTail(&pLLItem->list, pListHead);

	/**/
    /* set the LLP of the previous pLLItem to the current pLLItem*/
    /* address*/
    /**/
    prev_entry = FMSH_LIST_ENTRY(pLLItem->list.prev, vxT_LINKLIST_ITEM, list);
    prev_entry->llp = (UINT32)pLLItem;
    
    pLLItem->sar = pChnCfg->sar;
    pLLItem->dar = pChnCfg->dar;
	
    pLLItem->llp = NULL; /* set next time this function is called*/
    
    pLLItem->ctl_l = (pChnCfg->ctl_int_en) |
				    (pChnCfg->ctl_dst_tr_width << 1)  |
				    (pChnCfg->ctl_src_tr_width << 4)  |
				    (pChnCfg->ctl_dinc << 7)          |
				    (pChnCfg->ctl_sinc << 9)          |
				    (pChnCfg->ctl_dst_msize << 11)     |
				    (pChnCfg->ctl_src_msize << 14)     |
				    (pChnCfg->ctl_src_gather_en << 17) |
				    (pChnCfg->ctl_dst_scatter_en << 18)|
				    (pChnCfg->ctl_xfer_path << 20)         |
				    (pChnCfg->ctl_dms << 23)           |
				    (pChnCfg->ctl_sms << 25)           |
				    (pChnCfg->ctl_llp_dst_en << 27)    |
				    (pChnCfg->ctl_llp_src_en << 28);

    pLLItem->ctl_h = (pChnCfg->ctl_block_ts) | (0 << 12);

    pLLItem->sstat = 0;
    pLLItem->dstat = 0;

	return;
}


/*
Linked List Items
*/
int dmaCtrl_Setup_LinkListItem(vxT_DMA* pDma, int blockNum, int xferType,
                                         FMSH_listHead* pListHead, vxT_DMA_CHNCFG* pChnCfg)
{
	UINT32 i;
	UINT32 sar, dar;
	
    int ctl_llp_src_en;
    int ctl_llp_dst_en;
	
    UINT32 node_num;
	
	vxT_LINKLIST_ITEM * pLLItem;
	FMSH_listHead *pos, *p;	
	
	/* backup before LLI setting*/
	sar = pChnCfg->sar;
	dar = pChnCfg->dar;
	
	ctl_llp_src_en = pChnCfg->ctl_llp_src_en;
	ctl_llp_dst_en = pChnCfg->ctl_llp_dst_en;

    node_num = 0;
	
	for (i = 0; i < blockNum; i++)
	{
		/* create new node*/
		/*pLLItem = (FDmaPs_LliItem_T * malloc (sizeof(FDmaPs_LliItem_T));*/
		pLLItem = (vxT_LINKLIST_ITEM *) malloc (sizeof(vxT_LINKLIST_ITEM));
		/*pLLItem = (FDmaPs_LliItem * (LLI_MEMORY_BASE + i *LLI_MEMORY_OFFSET);*/
		
		if (pLLItem == NULL)
	    {
	        TRACE_OUT(DEBUG_OUT, "\nCann't create node!\r\n");
	        return FMSH_EINVAL;
	    }
		
		memset(pLLItem, 0, sizeof(vxT_LINKLIST_ITEM));
		
		FMSH_INIT_LIST_HEAD(&pLLItem->list);

		if (i == blockNum - 1)
		{
			pChnCfg->ctl_llp_dst_en = FMSH_clear;
			pChnCfg->ctl_llp_src_en = FMSH_clear;
		}
		
		dmaCtrl_Add_LinkListItem(pListHead, pLLItem, pChnCfg);

		if (xferType == DMA_XFER_TYPE7_ROW7 ) /* Dmac_transfer_row7*/
		{	
			pChnCfg->sar = pChnCfg->sar;
		}
		else
		{	
			pChnCfg->sar = pChnCfg->sar + pChnCfg->ctl_block_ts * 4;
		}

		if (xferType == DMA_XFER_TYPE9_ROW9)  /* Dmac_transfer_row9*/
		{	
			pChnCfg->dar = pChnCfg->dar;
		}
		else
		{	
			pChnCfg->dar = pChnCfg->dar + pChnCfg->ctl_block_ts * 4;
		}

        node_num++;
	}

	/* recover after LLI setting*/
	pChnCfg->ctl_llp_src_en = ctl_llp_src_en;
	pChnCfg->ctl_llp_dst_en = ctl_llp_dst_en;
	pChnCfg->sar = sar;
	pChnCfg->dar = dar;


    
    /* get the first LLI address by head node*/
    i = 0;
	pos = pListHead;
	while (i < gDMA_ListNum)
	{
		p = pos->next;
		pos = p;
        
        i++;
	}
	
   /* pLLItem = FMSH_LIST_ENTRY(pos, FDmaPs_LliItem_T, list);*/
    pLLItem = FMSH_LIST_ENTRY(pos, vxT_LINKLIST_ITEM, list);
    pChnCfg->llp_loc = pLLItem->llp >> 2;
	
#if DEBUG_LLI_OUT
    TRACE_OUT(1, "sar: 0x%08x; dar: 0x%08x; llp: 0x%08x; ctl_l: 0x%08x; ctl_h: 0x%08x; sstat: 0x%08x; dstat: 0x%08x\r\n",
           pLLItem->sar, pLLItem->dar, pLLItem->llp, pLLItem->ctl_l, pLLItem->ctl_h, pLLItem->sstat, pLLItem->dstat);
#endif

    gDMA_ListNum += node_num;

	return 0;
}

#endif

#if 1
int vxInit_Dma(void)
{
	vxT_DMA * pDma = NULL;
	vxT_DMA_CTRL * pDmaCtrl = NULL;
	
	int ret = 0;	
	
#if 1 
	/*
	init the pDma structure
	*/

	/**/
	/* spi_ctrl select*/
	/**/
	bzero((char*)(&g_pDma->dma_x), sizeof(vxT_DMA));	
	pDma = g_pDma;	
	
	bzero((char*)(&vxDma_Ctrl.ctrl_x), sizeof(vxT_DMA_CTRL));
	pDma->pDmaCtrl = (vxT_DMA_CTRL *)(&vxDma_Ctrl.ctrl_x);
	
	pDmaCtrl = pDma->pDmaCtrl;	
	
	pDma->dma_x = DMA_CTRL_0;	
	pDmaCtrl->ctrl_x = pDma->dma_x;
	
	pDmaCtrl->cfgBaseAddr = VX_DMA_CFG_BASE;	
	
	pDmaCtrl->devIP_ver = 0x3230322A;
	pDmaCtrl->devIP_type = FMSH_ahb_dmac;	
#endif	

	ret = dmaCtrl_Get_DefaultParams2(pDma);
    if (ret != FMSH_SUCCESS)
    {
    	printf("dmaCtrl_Get_DefaultParams fail! \n");
        return FMSH_FAILURE;
    }
	/*
	*/
	
	dmaCtrl_Init(pDma);

	pDma->init_flag = 1;

	return FMSH_SUCCESS;
}

int vxInit_Dma_1(void)
{
	vxT_DMA * pDma = NULL;
	vxT_DMA_CTRL * pDmaCtrl = NULL;
	
	int ret = 0;	
	
#if 1 
	/*
	init the pDma structure
	*/

	/**/
	/* spi_ctrl select*/
	/**/
	bzero((char*)(&g_pDma->dma_x), sizeof(vxT_DMA));	
	pDma = g_pDma;	
	
	bzero((char*)(&vxDma_Ctrl.ctrl_x), sizeof(vxT_DMA_CTRL));
	pDma->pDmaCtrl = (vxT_DMA_CTRL *)(&vxDma_Ctrl.ctrl_x);
	
	pDmaCtrl = pDma->pDmaCtrl;	
	
	pDma->dma_x = DMA_CTRL_0;	
	pDmaCtrl->ctrl_x = pDma->dma_x;
	
	pDmaCtrl->cfgBaseAddr = VX_DMA_CFG_BASE;	
	
	pDmaCtrl->devIP_ver = 0x3230322A;
	pDmaCtrl->devIP_type = FMSH_ahb_dmac;	
#endif	

	ret = dmaCtrl_Get_DefaultParams2(pDma);
    if (ret != FMSH_SUCCESS)
    {
    	printf("dmaCtrl_Get_DefaultParams fail! \n");
        return FMSH_FAILURE;
    }
	/*
	*/
	
	return FMSH_SUCCESS;
}

int vxInit_Dma_2(void)
{
	vxT_DMA * pDma = g_pDma;	
	
	dmaCtrl_Init(pDma);
	pDma->init_flag = 1;

	return FMSH_SUCCESS;
}

#endif


#if 1
/**/
/* Source memory location*/
/**/
#define SRC_MEMORY_BASE2          (FPS_AHB_SRAM_BASEADDR)
/**/
/* Destination memory location*/
/**/
#define DST_MEMORY_BASE2          (FPS_AHB_SRAM_BASEADDR + 0x10000)

/* */
/* for test*/
/**/
UINT32 gDMA_XferType = DMA_XFER_TYPE1_ROW1;
UINT32 gDMA_BlockSize = 2048;
UINT32 gDMA_BlockNum = 1;

/*
ahb_sram -> ahb_sram
*/
int test_dma_1(void)
{
	vxT_DMA * pDma = g_pDma;

    int i, j, errCode;
    UINT32 blockSize, blockNum, dataLen;
    UINT32 dst_data, exp_data, *dst_addr, *src_addr;
	
    /*enum FDmaPs_channelNumber chnNum;*/
    /*enum FDmaPs_transferType xferType;*/
    int chnNum, chnIdx;
    int xferType;
	
    /*FDmaPs_ChannelConfig_T chn_cfg;  // Channel configuration struct.*/
    vxT_DMA_CHNCFG chn_cfg = {0};  /* Channel configuration struct.*/

#if 1
	/**/
    /* Initialise the DMA controller*/
    /**/
    /*FDmaPs_init(pDma);*/
    /**/
	/*vxInit_Dma();*/
	/**/
	vxInit_Dma_1();
	vxInit_Dma_2();
#endif


#if 0  /* */
    /* Transfer characteristics*/
    chnNum = DMA_CHN_0;  /*Dmac_channel0; // Select a DMA channel*/
    xferType = DMA_XFER_TYPE1_ROW1;  /* Dmac_transfer_row1; // Select a transfer type*/
    
    blockSize = 2000; /* Select the size of a block*/
    blockNum = 1; /* Select the number of blocks    */
    dataLen = blockSize * blockNum;
#else

    chnNum = dmaCtrl_Get_FreeChn(pDma); /* Select a DMA channel*/
    
    xferType  = gDMA_XferType; /* Select a transfer type*/
    blockSize = gDMA_BlockSize; /* Select the size of a block*/
    blockNum  = gDMA_BlockNum;  /* Select the number of blocks*/

    dataLen = blockSize * blockNum;
#endif

	/**/
    /* Initialise the memory regions on both the source */
    /* and the destination memories.*/
    /**/
	src_addr = (UINT32 *)SRC_MEMORY_BASE2;  /* 0xE1FE0000 ahb_sram*/
	{
		for(i = 0; i < dataLen; i++)
	    {
	    	*src_addr = i + 1;
			src_addr++;
	    }
	}
	
	dst_addr = (UINT32 *)DST_MEMORY_BASE2;  /* 0xE1FE0000+0x10000 ahb_sram*/
	{
		memset(dst_addr, 0, dataLen * sizeof(UINT32));
	}

    /* Get the channel index from the enumerated type*/
    chnIdx = dmaCtrl_Get_ChnIdx(chnNum);	

#if 1
    /* START : Channel configuration*/
    /* -------*/
    errCode = dmaCtrl_Set_XferType(pDma, chnNum, xferType);
    if (errCode != 0)
	{
        TRACE_OUT(DEBUG_OUT, "ERROR: Failed to set the transfer type row %d\r\n", xferType);
    }
    
    /* Enable the interrupts on Channel x*/
    errCode = dmaCtrl_En_ChnIRQ(pDma, chnNum);
    if (errCode != 0)
	{
        TRACE_OUT(DEBUG_OUT, "ERROR: Failed to enable channel %d interrupts\r\n", chnIdx);
    }

    /* Initialise the channel configuration structure.*/
    errCode = dmaCtrl_Get_ChnCfg(pDma, chnNum, &chn_cfg);
    if (errCode != 0)
	{
        TRACE_OUT(DEBUG_OUT, "ERROR: Failed to initialise configuration ");
        TRACE_OUT(DEBUG_OUT, "structure from the DMA registers on channel %d\r\n", chnIdx);
    }

#if 1
    /* Change the configuration structure members to initialise the*/
    /* DMA channel for the chosen transfer.*/
    /* Set the Source and destination addresses*/
    chn_cfg.sar = SRC_MEMORY_BASE2;
    chn_cfg.dar = DST_MEMORY_BASE2;
	
    /* Set the source and destination transfer xfer_width*/
    chn_cfg.ctl_src_tr_width = DMA_XFER_WIDTH_32;  /* Dmac_trans_width_32;*/
    chn_cfg.ctl_dst_tr_width = DMA_XFER_WIDTH_32;  /* Dmac_trans_width_32;*/
	
    /* Set the Address increment type for the source and destination*/
    chn_cfg.ctl_sinc = DMA_ADDR_MODE_INC;  /* Dmac_addr_increment;*/
    chn_cfg.ctl_dinc = DMA_ADDR_MODE_INC;  /* Dmac_addr_increment;*/
	
    /* Set the source and destination burst transaction length*/
    chn_cfg.ctl_src_msize = DMA_MSIZE_32;  /*  Dmac_msize_32;*/
    chn_cfg.ctl_dst_msize = DMA_MSIZE_32;  /*  Dmac_msize_32;*/
	
	/* set scatter/gather enable and parameters*/
	chn_cfg.ctl_dst_scatter_en = DMA_WORK_MODE_NORMAL;  /* DMA_WORK_MODE_NORMAL;*/
	chn_cfg.ctl_src_gather_en  = DMA_WORK_MODE_NORMAL;  /* DMA_WORK_MODE_NORMAL;*/
	
    /* Set the block size for the DMA transfer*/
    /* Block size is the number of words of size Dmac_trans_width*/
    chn_cfg.ctl_block_ts = blockSize;
	
    /* Set the transfer device type and flow controller*/
    chn_cfg.ctl_xfer_path = DMA_XFER_PATH0_MEM2MEM_DMACTRL;   /* Dmac_mem2mem_dma;*/
#else

	FDmaPs_setCHxParam(&chn_cfg);
#endif

#if 0  /* for Linked List Items*/
	/* set LLI items for multi blocks*/
	if ((blockNum > 1) && (xferType >= DMA_XFER_TYPE6_ROW6))
	{
		errCode = dmaCtrl_Setup_LinkListItem(pDma, blockNum, xferType, &gDMA_List, &chn_cfg);
		if (errCode != 0)
		{
			TRACE_OUT(DEBUG_OUT, "ERROR: Failed to set LLI item\r\n");
		}
	}
#endif

    /* Write the new configuration setting into the DMA Controller device.*/
    errCode = dmaCtrl_Set_ChnCfg(pDma, chnNum, &chn_cfg);
    if (errCode != 0)
	{
        TRACE_OUT(DEBUG_OUT, "ERROR: Failed to write configuration structure ");
        TRACE_OUT(DEBUG_OUT, "into the DMA controller registers on channel %d\r\n", chnIdx);
    }
    /* -------*/
    /* END : Channel configuration*/
#endif

    /* display the transfer information.*/
    TRACE_OUT(DEBUG_OUT, "/ -----\r\n");
    TRACE_OUT(DEBUG_OUT, "  Channel Idx          %d\r\n", chnIdx);
    TRACE_OUT(DEBUG_OUT, "  Transfer type        Row_%d\r\n", xferType);
    TRACE_OUT(DEBUG_OUT, "  Block Size           %u\r\n", blockSize);
    TRACE_OUT(DEBUG_OUT, "  Number of blocks     %u\r\n", blockNum);
    TRACE_OUT(DEBUG_OUT, "  Source address       0x%08X\r\n", chn_cfg.sar);
    TRACE_OUT(DEBUG_OUT, "  Destination address  0x%08X\r\n", chn_cfg.dar);
    TRACE_OUT(DEBUG_OUT, "/ -----\n\n");

	/**/
    /* Enable the DMA controller and begin the interrupt driven DMA transfer.*/
    /**/
    dmaCtrl_Enable_Ctrl(pDma);
	
    /*errCode = FDmaPs_startTransfer(pDma, chnNum, blockNum, userCallback);*/
    errCode = dmaCtrl_Start_Xfer(pDma, chnNum, blockNum);
    if (errCode != 0)
	{
        TRACE_OUT(DEBUG_OUT, "ERROR: Failed to begin the interrupt transfer\r\n");
    }

    /* Now we monitor the transfers progress and wait for completion*/
    /* -------*/
#if 1
	UINT32 tmp;
	tmp = dmaCtrl_Rd_CfgReg32(pDma, DMAC_INT_RAW_TFR_L_OFFSET);
	while ((tmp & 0x1) == 0)	/* transfer is in progress*/
	{
        /* Heartbeat*/
        TRACE_OUT(DEBUG_OUT, ".");

        /* Just checking here that no error condition interrupts*/
        /* fire during the data transfer.*/

        /*
        if(s_DMA_errFlag == TRUE)
		{
            // Wasn't expecting this !!
            TRACE_OUT(DEBUG_OUT, "\nERROR: Recieved an err interrupt\r\n");
        }

        if (s_DMA_srcTranFlag == TRUE)
		{
            // This interrupt should not be unmasked when
            // a memory device is on the source !!
            TRACE_OUT(DEBUG_OUT, "\nERROR: Received an srcTran interrupt\r\n");
        }

        if (s_DMA_dstTranFlag == TRUE)
		{
            // This interrupt should not be unmasked when
            // a memory device is on the destination !!
            TRACE_OUT(DEBUG_OUT, "\nERROR: Received an dstTran interrupt\r\n");
        }
		*/
		
		tmp = dmaCtrl_Rd_CfgReg32(pDma, DMAC_INT_RAW_TFR_L_OFFSET);
    }
#else
	
	UINT32 tmp;
	do
	{
		tmp = dmaCtrl_Rd_CfgReg32(pDma, DMAC_INT_RAW_TFR_L_OFFSET);

		/**/
		/* Heartbeat, wait dma_done*/
		/**/
		TRACE_OUT(DEBUG_OUT, ".");
		
	} while ((tmp & 0x1) == 0);
#endif


	TRACE_OUT(DEBUG_OUT, "\r\n");

    /* -------*/
    /* Disable the DMA after transfer is complete.*/
    /* -------*/
    dmaCtrl_Disable_Ctrl(pDma);
	
    src_addr = (UINT32 *) chn_cfg.sar;
    dst_addr = (UINT32 *) chn_cfg.dar;
    for (j = 0; j < blockSize; j++)
	{
        exp_data = *(UINT32 *) src_addr++;
        dst_data = *(volatile UINT32 *) dst_addr++;
        if (exp_data != dst_data)
		{
        	TRACE_OUT(DEBUG_OUT, "dst_addr = %x : dst_data = %x : exp_data = %x\r\n",
				      dst_addr-1, dst_data, exp_data);
			return FMSH_FAILURE;
        }
    }

	TRACE_OUT(DEBUG_OUT, "DMA _TEST OK! \n\n");	

    return FMSH_SUCCESS;
}

#endif


#if 1
int test_dma_2(void)
{
	vxT_DMA * pDma = g_pDma;

    int i, j, errCode;
    UINT32 blockSize, blockNum, dataLen;
    UINT32 dst_data, exp_data, *dst_addr, *src_addr;
	
    /*enum FDmaPs_channelNumber chnNum;*/
    /*enum FDmaPs_transferType xferType;*/
    int chnNum, chnIdx;
    int xferType;
	
    /*FDmaPs_ChannelConfig_T chn_cfg;  // Channel configuration struct.*/
    vxT_DMA_CHNCFG chn_cfg = {0};  /* Channel configuration struct.*/

#if 0
	/**/
    /* Initialise the DMA controller*/
    /**/
    /*FDmaPs_init(pDma);*/
    /**/
	/*vxInit_Dma();*/
	/**/
	vxInit_Dma_1();
	vxInit_Dma_2();
#endif


#if 0  /* */
    /* Transfer characteristics*/
    chnNum = DMA_CHN_0;  /*Dmac_channel0; // Select a DMA channel*/
    xferType = DMA_XFER_TYPE1_ROW1;  /* Dmac_transfer_row1; // Select a transfer type*/
    
    blockSize = 2000; /* Select the size of a block*/
    blockNum = 1; /* Select the number of blocks    */
    dataLen = blockSize * blockNum;
#else

    chnNum = dmaCtrl_Get_FreeChn(pDma); /* Select a DMA channel*/
    
    xferType  = gDMA_XferType; /* Select a transfer type*/
    blockSize = gDMA_BlockSize; /* Select the size of a block*/
    blockNum  = gDMA_BlockNum;  /* Select the number of blocks*/

    dataLen = blockSize * blockNum;
#endif

	/**/
    /* Initialise the memory regions on both the source */
    /* and the destination memories.*/
    /**/
	src_addr = (UINT32 *)SRC_MEMORY_BASE2;  /* 0xE1FE0000 ahb_sram*/
	{
		for(i = 0; i < dataLen; i++)
	    {
	    	*src_addr = i + 1;
			src_addr++;
	    }
	}
	
	dst_addr = (UINT32 *)DST_MEMORY_BASE2;  /* 0xE1FE0000+0x10000 ahb_sram*/
	{
		memset(dst_addr, 0, dataLen * sizeof(UINT32));
	}

    /* Get the channel index from the enumerated type*/
    chnIdx = dmaCtrl_Get_ChnIdx(chnNum);	

#if 1
    /* START : Channel configuration*/
    /* -------*/
    errCode = dmaCtrl_Set_XferType(pDma, chnNum, xferType);
    if (errCode != 0)
	{
        TRACE_OUT(DEBUG_OUT, "ERROR: Failed to set the transfer type row %d\r\n", xferType);
    }
    
    /* Enable the interrupts on Channel x*/
    errCode = dmaCtrl_En_ChnIRQ(pDma, chnNum);
    if (errCode != 0)
	{
        TRACE_OUT(DEBUG_OUT, "ERROR: Failed to enable channel %d interrupts\r\n", chnIdx);
    }

    /* Initialise the channel configuration structure.*/
    errCode = dmaCtrl_Get_ChnCfg(pDma, chnNum, &chn_cfg);
    if (errCode != 0)
	{
        TRACE_OUT(DEBUG_OUT, "ERROR: Failed to initialise configuration ");
        TRACE_OUT(DEBUG_OUT, "structure from the DMA registers on channel %d\r\n", chnIdx);
    }

#if 1
    /* Change the configuration structure members to initialise the*/
    /* DMA channel for the chosen transfer.*/
    /* Set the Source and destination addresses*/
    chn_cfg.sar = SRC_MEMORY_BASE2;
    chn_cfg.dar = DST_MEMORY_BASE2;
	
    /* Set the source and destination transfer xfer_width*/
    chn_cfg.ctl_src_tr_width = DMA_XFER_WIDTH_32;  /* Dmac_trans_width_32;*/
    chn_cfg.ctl_dst_tr_width = DMA_XFER_WIDTH_32;  /* Dmac_trans_width_32;*/
	
    /* Set the Address increment type for the source and destination*/
    chn_cfg.ctl_sinc = DMA_ADDR_MODE_INC;  /* Dmac_addr_increment;*/
    chn_cfg.ctl_dinc = DMA_ADDR_MODE_INC;  /* Dmac_addr_increment;*/
	
    /* Set the source and destination burst transaction length*/
    chn_cfg.ctl_src_msize = DMA_MSIZE_32;  /*  Dmac_msize_32;*/
    chn_cfg.ctl_dst_msize = DMA_MSIZE_32;  /*  Dmac_msize_32;*/
	
	/* set scatter/gather enable and parameters*/
	chn_cfg.ctl_dst_scatter_en = DMA_WORK_MODE_NORMAL;  /* DMA_WORK_MODE_NORMAL;*/
	chn_cfg.ctl_src_gather_en  = DMA_WORK_MODE_NORMAL;  /* DMA_WORK_MODE_NORMAL;*/
	
    /* Set the block size for the DMA transfer*/
    /* Block size is the number of words of size Dmac_trans_width*/
    chn_cfg.ctl_block_ts = blockSize;
	
    /* Set the transfer device type and flow controller*/
    chn_cfg.ctl_xfer_path = DMA_XFER_PATH0_MEM2MEM_DMACTRL;   /* Dmac_mem2mem_dma;*/
#else

	FDmaPs_setCHxParam(&chn_cfg);
#endif

#if 0  /* for Linked List Items*/
	/* set LLI items for multi blocks*/
	if ((blockNum > 1) && (xferType >= DMA_XFER_TYPE6_ROW6))
	{
		errCode = dmaCtrl_Setup_LinkListItem(pDma, blockNum, xferType, &gDMA_List, &chn_cfg);
		if (errCode != 0)
		{
			TRACE_OUT(DEBUG_OUT, "ERROR: Failed to set LLI item\r\n");
		}
	}
#endif

    /* Write the new configuration setting into the DMA Controller device.*/
    errCode = dmaCtrl_Set_ChnCfg(pDma, chnNum, &chn_cfg);
    if (errCode != 0)
	{
        TRACE_OUT(DEBUG_OUT, "ERROR: Failed to write configuration structure ");
        TRACE_OUT(DEBUG_OUT, "into the DMA controller registers on channel %d\r\n", chnIdx);
    }
    /* -------*/
    /* END : Channel configuration*/
#endif

    /* display the transfer information.*/
    TRACE_OUT(DEBUG_OUT, "/ -----\r\n");
    TRACE_OUT(DEBUG_OUT, "  Channel Idx          %d\r\n", chnIdx);
    TRACE_OUT(DEBUG_OUT, "  Transfer type        Row_%d\r\n", xferType);
    TRACE_OUT(DEBUG_OUT, "  Block Size           %u\r\n", blockSize);
    TRACE_OUT(DEBUG_OUT, "  Number of blocks     %u\r\n", blockNum);
    TRACE_OUT(DEBUG_OUT, "  Source address       0x%08X\r\n", chn_cfg.sar);
    TRACE_OUT(DEBUG_OUT, "  Destination address  0x%08X\r\n", chn_cfg.dar);
    TRACE_OUT(DEBUG_OUT, "/ -----\n\n");

	/**/
    /* Enable the DMA controller and begin the interrupt driven DMA transfer.*/
    /**/
    dmaCtrl_Enable_Ctrl(pDma);
	
    /*errCode = FDmaPs_startTransfer(pDma, chnNum, blockNum, userCallback);*/
    errCode = dmaCtrl_Start_Xfer(pDma, chnNum, blockNum);
    if (errCode != 0)
	{
        TRACE_OUT(DEBUG_OUT, "ERROR: Failed to begin the interrupt transfer\r\n");
    }

    /* Now we monitor the transfers progress and wait for completion*/
    /* -------*/
#if 1
	UINT32 tmp;
	tmp = dmaCtrl_Rd_CfgReg32(pDma, DMAC_INT_RAW_TFR_L_OFFSET);
	while ((tmp & 0x1) == 0)	/* transfer is in progress*/
	{
        /* Heartbeat*/
        TRACE_OUT(DEBUG_OUT, ".");

        /* Just checking here that no error condition interrupts*/
        /* fire during the data transfer.*/

        /*
        if(s_DMA_errFlag == TRUE)
		{
            // Wasn't expecting this !!
            TRACE_OUT(DEBUG_OUT, "\nERROR: Recieved an err interrupt\r\n");
        }

        if (s_DMA_srcTranFlag == TRUE)
		{
            // This interrupt should not be unmasked when
            // a memory device is on the source !!
            TRACE_OUT(DEBUG_OUT, "\nERROR: Received an srcTran interrupt\r\n");
        }

        if (s_DMA_dstTranFlag == TRUE)
		{
            // This interrupt should not be unmasked when
            // a memory device is on the destination !!
            TRACE_OUT(DEBUG_OUT, "\nERROR: Received an dstTran interrupt\r\n");
        }
		*/
		
		tmp = dmaCtrl_Rd_CfgReg32(pDma, DMAC_INT_RAW_TFR_L_OFFSET);
    }
#else
	
	UINT32 tmp;
	do
	{
		tmp = dmaCtrl_Rd_CfgReg32(pDma, DMAC_INT_RAW_TFR_L_OFFSET);

		/**/
		/* Heartbeat, wait dma_done*/
		/**/
		TRACE_OUT(DEBUG_OUT, ".");
		
	} while ((tmp & 0x1) == 0);
#endif


	TRACE_OUT(DEBUG_OUT, "\r\n");

    /* -------*/
    /* Disable the DMA after transfer is complete.*/
    /* -------*/
   /* dmaCtrl_Disable_Ctrl(pDma);*/
	
    src_addr = (UINT32 *) chn_cfg.sar;
    dst_addr = (UINT32 *) chn_cfg.dar;
    for (j = 0; j < blockSize; j++)
	{
        exp_data = *(UINT32 *) src_addr++;
        dst_data = *(volatile UINT32 *) dst_addr++;
        if (exp_data != dst_data)
		{
        	TRACE_OUT(DEBUG_OUT, "dst_addr = %x : dst_data = %x : exp_data = %x\r\n",
				      dst_addr-1, dst_data, exp_data);
			return FMSH_FAILURE;
        }
    }

	TRACE_OUT(DEBUG_OUT, "DMA_TEST_2 OK! \n\n");	

    return FMSH_SUCCESS;
}
#endif



