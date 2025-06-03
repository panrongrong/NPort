/* vxSram.c - fmsh 7020/7045 pSmc driver */

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
01a, 12Jun20, jc  written.
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

#include "../vxSmc.h"

#include "vxSram.h"

/*
defines 
*/
#if 1

#undef VX_DBG
#undef VX_DBG2

#define SRAM_DBG

#ifdef SRAM_DBG

#define VX_DBG(string, a, b, c, d, e, f)                    \
        if (_func_logMsg != NULL)                           \
           printf(string, a, b, c, d, e, f)  /* (* _func_logMsg)(string, a, b, c, d, e, f) */
#else
#define VX_DBG(string, a, b, c, d, e, f)
#endif

/* error info log */
#define VX_DBG2(string, a, b, c, d, e, f) printf(string, a, b, c, d, e, f)
#endif




#if 1

UINT8 SRAM_Read(UINT32 baseAddr, UINT32 offSet)		    
{
	UINT8 ret = 0;
	
	ret = *((volatile UINT8*)(baseAddr + offSet));	
	return ret;
}

void SRAM_Write(UINT32 baseAddr, UINT32 offSet, UINT8 data)	
{
	*((volatile UINT8*)(baseAddr + offSet)) = data;
	return;
}


#endif


#if 1

extern vxT_SMC * g_pSmc;

void smcCtrl_TimeSetup_SRAM(vxT_SMC* pSmc, int chip_sel)
{
	UINT32 tmp32 = 0;
	
	tmp32 = smcCtrl_Get_TimingReg(pSmc, chip_sel);
	
	 smcCtrl_Set_TimingReg(pSmc, chip_sel, tmp32);
	
	tmp32 = smcCtrl_Get_MaskReg(pSmc, chip_sel);
	tmp32 |= (chip_sel << 8);
	smcCtrl_Set_MaskReg(pSmc, chip_sel, tmp32);

	return;
}

int vxInit_Smc_Sram(int bus_width, int chip_sel)
{
	vxT_SMC* pSmc = g_pSmc;

	int dev_mode = SMC_DEV_SRAM;
	
	vxInit_Smc(dev_mode, bus_width, chip_sel);	

	smcCtrl_TimeSetup_SRAM(pSmc, chip_sel);
	
	return 0;
}

#endif


