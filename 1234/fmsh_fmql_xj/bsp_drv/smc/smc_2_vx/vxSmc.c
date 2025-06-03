/* vxSmc.c - fmsh 7020/7045 pSmc driver */

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
01a, 09Jun20, jc  written.
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

#include "vxSmc.h"

/*
defines 
*/
#if 1

#undef VX_DBG
#undef VX_DBG2

#define SMC_DBG

#ifdef SMC_DBG

#define VX_DBG(string, a, b, c, d, e, f)                    \
        if (_func_logMsg != NULL)                           \
           printf(string, a, b, c, d, e, f)  /* (* _func_logMsg)(string, a, b, c, d, e, f) */
#else
#define VX_DBG(string, a, b, c, d, e, f)
#endif

/* error info log */
#define VX_DBG2(string, a, b, c, d, e, f) printf(string, a, b, c, d, e, f)
#endif



/* 
smc_ctrl
*/
static vxT_SMC_CTRL vxSmc_Ctrl = {0};
static vxT_SMC vx_pSmc = {0};
vxT_SMC * g_pSmc = (vxT_SMC *)(&vx_pSmc.smc_x);



#if 1

void smcCtrl_Wr_CfgReg32(vxT_SMC* pSmc, UINT32 offset, UINT32 value)
{
	UINT32 tmp32 = pSmc->pSmcCtrl->cfgBaseAddr;
	FMQL_WRITE_32((tmp32 + offset), value);
	return;
}

UINT32 smcCtrl_Rd_CfgReg32(vxT_SMC* pSmc, UINT32 offset)
{
	UINT32 tmp32 = pSmc->pSmcCtrl->cfgBaseAddr;
	return FMQL_READ_32(tmp32 + offset);
}

/*****************************************************************************
*
* @description
* This function sets value to the SMC timing registers.
*
* @param    
*           pSmc is the pointer to the SMC device.
*           index is the timing register numer.
*           data is the value which writes to the timing register.
*
* @return   
*           NA.
*
* @note     
*           NA.
*
*****************************************************************************/
void smcCtrl_Set_TimingReg(vxT_SMC* pSmc, UINT8 index, UINT32 data)
{
	UINT32 offset;

	offset = SMC_REG_SMTMGR_SET0 + index * 0x4;
	
	/*FMSH_WriteReg(pSmc->base_address, offset, data);*/
	smcCtrl_Wr_CfgReg32(pSmc, offset, data);

	return;
}

/*****************************************************************************
*
* @description
* This function gets value from the SMC timing register.
*
* @param    
*           pSmc is the pointer to the SMC device.
*           index is the timing register numer.
*
* @return   
*           the value reads from the timing register.
*
* @note     NA.
*
*****************************************************************************/
UINT32 smcCtrl_Get_TimingReg(vxT_SMC* pSmc, UINT8 index)
{
	UINT32 offset, reg;

	offset = SMC_REG_SMTMGR_SET0 + index * 0x4;
	
	/*reg = FMSH_ReadReg(pSmc->base_address, offset);*/
	reg = smcCtrl_Rd_CfgReg32(pSmc, offset);

	return reg;
}

/*****************************************************************************
*
* @description
* This function sets value to the SMC address mask registers.
*
* @param    
*           pSmc is the pointer to the SMC device.
*           index is the address mask register numer.
*           data is the value which writes to the address mask register.
*
* @return   
*           NA.
*
* @note     
*           NA.
*
*****************************************************************************/
void smcCtrl_Set_MaskReg(vxT_SMC* pSmc, UINT8 index, UINT32 data)
{
	/*FMSH_WriteReg(pSmc->base_address, SMC_REG_SMSKRx(index), data);	*/
	smcCtrl_Wr_CfgReg32(pSmc, SMC_REG_SMSKRx(index), data);
	return;
}

/*****************************************************************************
*
* @description
* This function gets value from the SMC address mask register.
*
* @param    
*           pSmc is the pointer to the SMC device.
*           index is the address mask register numer.
*
* @return   
*           the value reads from the address mask register.
*
* @note     
*           NA.
*
*****************************************************************************/
UINT32 smcCtrl_Get_MaskReg(vxT_SMC* pSmc, UINT8 index)
{
	UINT32 reg;

	/*reg = FMSH_ReadReg(pSmc->base_address, SMC_SMSKRx(index));*/
	reg = smcCtrl_Rd_CfgReg32(pSmc, SMC_REG_SMSKRx(index));

	return reg;
}

/*****************************************************************************
*
* @description
* This function sets value to the SMC control registers.
*
* @param    
*           pSmc is the pointer to the SMC device.
*           data is the value which writes to the control register.
*
* @return   
*           NA.
*
* @note     
*           NA.
*
*****************************************************************************/
void smcCtrl_Set_CtrlReg(vxT_SMC* pSmc, UINT32 data)
{
	/*FMSH_WriteReg(pSmc->base_address, SMC_SMCTLR, data);*/
	smcCtrl_Wr_CfgReg32(pSmc, SMC_REG_SMCTLR, data);
	return;
}

/*****************************************************************************
*
* @description
* This function gets value from the SMC control register.
*
* @param    
*           pSmc is the pointer to the SMC device.
*
* @return   
*           the value reads from the control register.
*
* @note     
*           NA.
*
*****************************************************************************/
UINT32 smcCtrl_Get_CtrlReg(vxT_SMC* pSmc)
{
	UINT32 reg;

	/*reg = FMSH_ReadReg(pSmc->base_address, SMC_SMCTLR);*/
	reg = smcCtrl_Rd_CfgReg32(pSmc, SMC_REG_SMCTLR);

	return reg;
}

#endif

#if 1

void smcSlcr_Reset_SmcCtrl(void)
{
	int i = 0;

	/* set rst_ctrl*/
	slcr_write(0x278, 1);

	for (i = 0; i< 50; i++);
	
	/* clear rst_ctrl*/
	slcr_write(0x278, 0);	
	
	return;
}

#endif



#if 1

void smcCtrl_Init(vxT_SMC* pSmc, UINT8 dev_sel)
{
    UINT32 tmp32;
	
    tmp32 = 0x12354;
	
    smcCtrl_Wr_CfgReg32(pSmc, SMC_REG_SMTMGR_SET0, tmp32);
    smcCtrl_Wr_CfgReg32(pSmc, SMC_REG_SMTMGR_SET1, tmp32);
    smcCtrl_Wr_CfgReg32(pSmc, SMC_REG_SMTMGR_SET2, tmp32);

	/**/
    /* 4A: flash, 256Mb; */
    /* 2A: sram, 256Mb;*/
    /**/
    if (dev_sel == SMC_DEV_NORFLASH)
    {	
    	tmp32 = 0x4A;
	}
	else if (dev_sel == SMC_DEV_SRAM)
	{	
		tmp32 = 0x2A;
	}
	
    smcCtrl_Wr_CfgReg32(pSmc, SMC_REG_SMSKRx(0), tmp32);
    smcCtrl_Wr_CfgReg32(pSmc, SMC_REG_SMSKRx(1), tmp32);

    tmp32 = 0x1201;
    smcCtrl_Wr_CfgReg32(pSmc, SMC_REG_SMCTLR, tmp32);

	return;
}

void smcCtrl_Init_2(vxT_SMC* pSmc, int bus_width, int chip_sel)
{
	pSmc->pSmcCtrl->bus_width = bus_width;
	
	if (chip_sel == 0)
	{	
		pSmc->pSmcCtrl->sram_nor_baseAddr = VX_SMC_NORSRAM0_BASE;
	}
	else if (chip_sel == 1)
	{	
		pSmc->pSmcCtrl->sram_nor_baseAddr = VX_SMC_NORSRAM1_BASE;
	}
	
	return;
}


int vxInit_Smc(int dev_mode, int bus_width, int chip_sel)
{
	vxT_SMC * pSmc = NULL;
	vxT_SMC_CTRL * pSmcCtrl = NULL;
	
	int ret = 0;	
	UINT32 tmp32 = 0;

	/**/
	/* enable can_clk of slcr*/
	/**/
	/*smcSlcr_Enable_CanClk();*/
	
	/**/
	/* setup can_MIO of slcr*/
	/**/
	/*smcSlcr_Set_SmcMIO();*/
	
#if 1 
	/*
	init the pSmc structure
	*/

	/**/
	/* spi_ctrl select*/
	/**/
	pSmc = g_pSmc;	
	bzero(pSmc, sizeof(vxT_SMC));
	
	pSmc->pSmcCtrl = (vxT_SMC_CTRL *)(&vxSmc_Ctrl.ctrl_x);
	bzero(pSmc->pSmcCtrl, sizeof(vxT_SMC_CTRL));

	pSmc->smc_x = SMC_CTRL_0;
	pSmc->pSmcCtrl->devId = SMC_CTRL_0;
	pSmc->pSmcCtrl->cfgBaseAddr = VX_SMC_CFG_BASE;
	/*pSmc->pSmcCtrl->sysClk = FPAR_CANPS_0_CAN_CLK_FREQ_HZ;*/
	
	pSmcCtrl = pSmc->pSmcCtrl;		
	pSmcCtrl->ctrl_x = pSmcCtrl->devId;

	pSmcCtrl->bus_width = 32;  /* 32bit bus width*/
	
	pSmcCtrl->devIP_type = FMSH_ahb_smc;
#endif	
	
	smcCtrl_Init(pSmc, dev_mode);

	smcCtrl_Init_2(pSmc, bus_width, chip_sel);

	return;
}


#endif


