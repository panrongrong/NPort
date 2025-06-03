/* vxGtc.c - fmsh 7020/7045 pGtc driver */

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
01a, 19May20, jc  written.
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

#include "vxGtc.h"

/*
defines 
*/
#if 1

#undef VX_DBG
#undef VX_DBG2

#define GTC_DBG

#ifdef GTC_DBG

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
GTC
*/
static vxT_GTC_CTRL vxGtc_Ctrl = {0};

static vxT_GTC vx_pGtc = {0};
vxT_GTC * g_pGtc = (vxT_GTC *)(&vx_pGtc.gtc_x);



#if 1

void gtcCtrl_Wr_CfgReg32(vxT_GTC* pGtc, UINT32 offset, UINT32 value)
{
	UINT32 tmp32 = pGtc->pGtcCtrl->cfgBaseAddr;
	FMQL_WRITE_32((tmp32 + offset), value);
	return;
}

UINT32 gtcCtrl_Rd_CfgReg32(vxT_GTC* pGtc, UINT32 offset)
{
	UINT32 tmp32 = pGtc->pGtcCtrl->cfgBaseAddr;
	return FMQL_READ_32(tmp32 + offset);
}

/******************************************************************************
*
*
* This function is used to enable GTC
*
* @param    dev  is a pointer to the instance of GTC.
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
int gtcCtrl_Enable_Counter(vxT_GTC * pGtc)
{
    UINT32 val;

    val = gtcCtrl_Rd_CfgReg32(pGtc, GTC_REG_CNTCR);  /* GTC_IN32P(portmap->CNTCR);*/
    gtcCtrl_Wr_CfgReg32(pGtc, GTC_REG_CNTCR, (val|0x01));   /* GTC_OUT32P(val | 0x1, portmap->CNTCR);*/

    return 0;
}

/******************************************************************************
*
*
* This function is used to disenable GTC
*
* @param    dev  is a pointer to the instance of GTC.
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
int gtcCtrl_Disable_Counter(vxT_GTC * pGtc)
{
    UINT32 val;

    /* val = GTC_IN32P(portmap->CNTCR);*/
    /* GTC_OUT32P(val & (~0x1), portmap->CNTCR);*/
    val = gtcCtrl_Rd_CfgReg32(pGtc, GTC_REG_CNTCR); 
    gtcCtrl_Wr_CfgReg32(pGtc, GTC_REG_CNTCR, (val&(~0x01))); 

    return 0;
}

/******************************************************************************
*
*
* This function is used to halt GTC
*
* @param    dev  is a pointer to the instance of GTC.
* @param    val = 1 halt, val = 0 donnot halt
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
int gtcCtrl_Halt_Counter(vxT_GTC * pGtc, UINT32 val)
{
    /*UINT32 val;*/

    val = gtcCtrl_Rd_CfgReg32(pGtc, GTC_REG_CNTCR); 
    val = (val == 1) ? (val | 0x2) : (val & (~0x2)); 
	
    gtcCtrl_Wr_CfgReg32(pGtc, GTC_REG_CNTCR, val); 

    return 0;
}

/******************************************************************************
*
*
* This function is used to get GTC is stop?
*
* @param    dev  is a pointer to the instance of GTC.
*
* @return   0 if no stop, 1 stop.
*
* @note     None.
*
******************************************************************************/
UINT32 gtcCtrl_isStop(vxT_GTC * pGtc)
{
    UINT32 ret, val;

    /*ret = GTC_IN32P(portmap->CNTSR);*/
    val = gtcCtrl_Rd_CfgReg32(pGtc, GTC_REG_CNTSR); 

    ret = (val >> 1) & 0x1;

    return ret;
}

/******************************************************************************
*
*
* This function is used to get GTC counter high
*
* @param    dev  is a pointer to the instance of GTC.
*
* @return   counter high value
*
* @note     None.
*
******************************************************************************/
UINT32 gtcCtrl_Get_CounterH(vxT_GTC * pGtc)
{
    UINT32 ret;
	
    ret = gtcCtrl_Rd_CfgReg32(pGtc, GTC_REG_CNTCV_H); 

    return ret;
}

/******************************************************************************
*
*
* This function is used to get GTC counter low
*
* @param    dev  is a pointer to the instance of GTC.
*
* @return   counter low value
*
* @note     None.
*
******************************************************************************/
UINT32 gtcCtrl_Get_CounterL(vxT_GTC * pGtc)
{
    UINT32 ret;

    ret = gtcCtrl_Rd_CfgReg32(pGtc, GTC_REG_CNTCV_L); 

    return ret;
}

#endif


#if 1

int vxInit_Gtc(void)
{
	vxT_GTC * pGtc = NULL;
	vxT_GTC_CTRL * pGtcCtrl = NULL;
	
	int ret = 0;	
	UINT32 tmp32 = 0;

	/*
	init the pGtc structure
	*/
	if (g_pGtc->pGtcCtrl->status == 1)
	{
		return 0;  /* already init*/
	}
	
	/**/
	/* spi_ctrl select*/
	/**/
	pGtc = g_pGtc;	
	bzero(pGtc, sizeof(vxT_GTC));
	
	pGtc->pGtcCtrl = (vxT_GTC_CTRL *)(&vxGtc_Ctrl.ctrl_x);
	bzero(pGtc->pGtcCtrl, sizeof(vxT_GTC_CTRL));

	pGtc->gtc_x = 0;
	pGtc->pGtcCtrl->devId = 0;
	pGtc->pGtcCtrl->cfgBaseAddr = VX_GTC_CFG_BASE;		
	
	pGtcCtrl = pGtc->pGtcCtrl;		
	pGtcCtrl->ctrl_x = pGtc->gtc_x;

	
	gtcCtrl_Enable_Counter(pGtc);
	
	pGtcCtrl->status = 1;  /* 1-init_ok, 0-no_init*/
	
	return;
}

UINT64 gtc_Get_Time(void)   
{
	vxT_GTC * pGtc = g_pGtc;
	
	UINT32 low = 0, high = 0;
	UINT64 ret = 0;

	/* Reading Generic Timer Counter Register */
	do
	{
		high = gtcCtrl_Get_CounterH(pGtc);		
		low = gtcCtrl_Get_CounterL(pGtc);
		
	} while (gtcCtrl_Get_CounterH(pGtc) != high);

	ret = (((UINT64) high) << 32U) | (UINT64) low;

	return ret;
}

#endif


#if 1

void test_gtc_delay(int delay)
{
	UINT64 tmp64_0 = 0;
	UINT64 tmp64 = 0;

	/**/
	/* init GTC*/
	/**/
	vxInit_Gtc();
	
	tmp64_0 = gtc_Get_Time();
	taskDelay(delay);
	tmp64 = gtc_Get_Time();
	
	printf("timer(%d ticks): %lld (%lld) \n", delay, tmp64, (tmp64-tmp64_0));
	
	return;
}

#endif




