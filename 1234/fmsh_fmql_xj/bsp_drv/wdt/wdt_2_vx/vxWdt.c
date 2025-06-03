/* vxWdt.c - fmsh 7020/7045 pWdt driver */

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
01a, 20May20, jc  written.
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

#include "vxWdt.h"

/*
defines 
*/
#if 1

#undef VX_DBG
#undef VX_DBG2

#define WDT_DBG

#ifdef WDT_DBG

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
static vxT_WDT_CTRL vxWdt_Ctrl = {0};

static vxT_WDT vx_pWdt = {0};
vxT_WDT * g_pWdt = (vxT_WDT *)(&vx_pWdt.wdt_x);



#if 1

void wdtCtrl_Wr_CfgReg32(vxT_WDT* pWdt, UINT32 offset, UINT32 value)
{
	UINT32 tmp32 = pWdt->pWdtCtrl->cfgBaseAddr;
	FMQL_WRITE_32((tmp32 + offset), value);
	return;
}

UINT32 wdtCtrl_Rd_CfgReg32(vxT_WDT* pWdt, UINT32 offset)
{
	UINT32 tmp32 = pWdt->pWdtCtrl->cfgBaseAddr;
	return FMQL_READ_32(tmp32 + offset);
}


/******************************************************************************
*
*
* This function set WDT enable
*
* @param    dev is a pointer to WDT device
* @param    state: FMSH_clear/FMSH_err/FMSH_set.
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
UINT8 wdtCtrl_Enable_WDT(vxT_WDT* pWdt, int enable)
{
    UINT32 reg = 0;
    
    /*reg = WDT_IN32P(portmap->cr);*/
	reg = wdtCtrl_Rd_CfgReg32(pWdt, WDT_REG_CR);
	
    /*
    if (FMSH_BIT_GET(reg, WDT_EN) != state) 
    {
        FMSH_BIT_SET(reg, WDT_EN, state);
        WDT_OUT32P(reg, portmap->cr);
    }
	*/
	
	/*
	bit0	WDT_EN	WDT使能
			0 - WDT关闭（default）
			1- WDT使能
	*/
	
	/*if ((reg & 0x01) != enable)*/
	{
		wdtCtrl_Wr_CfgReg32(pWdt, WDT_REG_CR, (reg | 0x01));
	}
	
    return 0;
}

/******************************************************************************
*
*
* This function set WDT Response mode
*
* @param    dev is a pointer to WDT device
* @param    state: FMSH_clear/FMSH_err/FMSH_set.
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
UINT8 wdtCtrl_Set_RspMode(vxT_WDT* pWdt, int state)
{
    UINT32 reg = 0;
	
    /*        
    reg = WDT_IN32P(portmap->cr);
    if(FMSH_BIT_GET(reg, RMOD) != state)
    {
        FMSH_BIT_SET(reg, RMOD, state);
        WDT_OUT32P(reg, portmap->cr);
    }
	*/
	
	/*
	bit1	RMOD	Response mode
			0 - 只产生reset信号（default）
			1 - 第一次计数完成产生中断信号，如果在第二次计数完成以前中断没有被清除，则产生reset信号
	*/
	reg = wdtCtrl_Rd_CfgReg32(pWdt, WDT_REG_CR);
	if (state == RSP_ONLY_RST)
	{
		wdtCtrl_Wr_CfgReg32(pWdt, WDT_REG_CR, (reg & (~0x02)));
	}
	else
	{
		wdtCtrl_Wr_CfgReg32(pWdt, WDT_REG_CR, (reg | (0x02)));
	}

    return 0;
}

/******************************************************************************
*
*
* This function set WDT Reset Pulse Length
*
* @param    dev is a pointer to WDT device
* @param    value is enum prl_value value _2_pclk/....../_256_pclk
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
UINT8 wdtCtrl_Set_RPL(vxT_WDT* pWdt, int value)
{
    UINT32 reg = 0;

    /*
    reg = WDT_IN32P(portmap->cr);

    if(FMSH_BIT_GET(reg, RPL) != value) 
    {
        FMSH_BIT_SET(reg, RPL, value);
        WDT_OUT32P(reg, portmap->cr);
    }
	*/
	
	reg = wdtCtrl_Rd_CfgReg32(pWdt, WDT_REG_CR);

	/*
	Bit4~2: RPL Reset Pulse Lenth. 调整reset周期数，2~256个pclk长度。
	------
		000 - 2 pclk (default)
		001 - 4 pclk
		010 - 8 pclk
		011 - 16 pclk
		100 - 32 pclk
		101 - 64 pclk
		110 - 128 pclk
		111 - 256 pclk
	*/
	if (((reg & 0x1C) >> 2) != value)
	{
		wdtCtrl_Wr_CfgReg32(pWdt, WDT_REG_CR, (reg | (value << 2)));
	}

    return 0;
}

/******************************************************************************
*
*
* This function set TOP value, as WDT restart counter value
*
* @param    dev is a pointer to WDT device
* @param    value is TOP value
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
UINT8 wdtCtrl_Set_TOP(vxT_WDT* pWdt, UINT32 value)
{
    UINT32 reg = 0;

    if (value > 15)
    {    
    	return 1;
    }

    /*
    reg = WDT_IN32P(portmap->torr);
    if(FMSH_BIT_GET(reg, TOP) != value) 
    {
        FMSH_BIT_SET(reg, TOP, value);
        WDT_OUT32P(reg, portmap->torr);
    }
	*/
	
	reg = wdtCtrl_Rd_CfgReg32(pWdt, WDT_REG_TORR);
	if ((reg & 0x0F) != value)
	{
		wdtCtrl_Wr_CfgReg32(pWdt, WDT_REG_TORR, (reg | value));
	}

    return 0;
}

/******************************************************************************
*
*
* This function set TOP_INIT value, as WDT fisrt counter value
*
* @param    dev is a pointer to WDT device
* @param    value is TOP_INIT value
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
UINT8 wdtCtrl_Set_TOPINIT(vxT_WDT* pWdt, UINT32 value)
{
    UINT32 reg = 0;

    if (value > 15)
    {    
    	return 1;
    }

   /*
   reg = WDT_IN32P(portmap->torr);
    if(FMSH_BIT_GET(reg, TOP_INIT) != value)
    {
        FMSH_BIT_SET(reg, TOP_INIT, value);
        WDT_OUT32P(reg, portmap->torr);
    }
	*/
	reg = wdtCtrl_Rd_CfgReg32(pWdt, WDT_REG_TORR);
	if (((reg & 0xF0) >> 4) != value)
	{
		wdtCtrl_Wr_CfgReg32(pWdt, WDT_REG_TORR, (reg | (value << 4)));
	}

    return 0;
}

/******************************************************************************
*
*
* This function get current counter value.
*
* @param    dev is a pointer to WDT device.
*
* @return   current counter value.
*
* @note     None.
*
******************************************************************************/
UINT32 wdtCtrl_Get_CurCntVal(vxT_WDT* pWdt)
{
    UINT32 reg = 0;

 	/* reg = WDT_IN32P(portmap->ccvr);*/
	reg = wdtCtrl_Rd_CfgReg32(pWdt, WDT_REG_CCVR);

    return reg;
}

/******************************************************************************
*
*
* This function restart WDT.
*
* @param    dev is a pointer to WDT device.
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
UINT32 wdtCtrl_Restart(vxT_WDT* pWdt)
{
   /* WDT_OUT32P(0x76, portmap->ccr);*/
	wdtCtrl_Wr_CfgReg32(pWdt, WDT_REG_CRR, 0x76);

    return 0;
}

/******************************************************************************
*
*
* This function get interrupt status.
*
* @param    dev is a pointer to WDT device.
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
UINT32 wdtCtrl_Get_IrqStatus(vxT_WDT* pWdt)
{
    UINT32 reg = 0;

    /* reg = WDT_IN32P(portmap->stat);*/
	reg = wdtCtrl_Rd_CfgReg32(pWdt, WDT_REG_STAT);

    return reg;
}

/******************************************************************************
*
*
* This function clear interrupt status, read eio clear interrupt.
*
* @param    dev is a pointer to WDT device.
*
* @return   return is unsed.
*
* @note     None.
*
******************************************************************************/
UINT32 wdtCtrl_Clear_IrqStatus(vxT_WDT* pWdt)
{
    UINT32 reg = 0;

    /*reg = WDT_IN32P(portmap->eio);*/
	reg = wdtCtrl_Rd_CfgReg32(pWdt, WDT_REG_EIO);

    return reg;
}

#endif

#if 1

/*
slcr:
=====

WDT_CLK_CTRL	0x32C	
---------------------
        reserved	31:5	rw	0x0	Reserved. 写无效，读为0
		WDT_PAUSE	4	rw	0x0	Pause WDT timer倒计时
		
		WDT_CLKACT	3	rw	0x1	"WDT REF时钟使能：
							0：不使能
							1：使能"
							
		WDT_APB_CLKACT	2	rw	0x1	"WDT APB 时钟使能：
							0：不使能
							1：使能"
							
		WDT_SRCSEL	1:0	rw	0x0	"WDT REF 时钟源选择：
							00：APB时钟
							01：OSC时钟
							1x：MIO/EMIO时钟"
							
WDT_RST_CTRL	0x330	
---------------------
        reserved	31:2	rw	0x0	Reserved. 写无效，读为0
        
		WDT_RST	1	rw	0x0	"WDT REF 时钟复位：
						0：不复位
						1：复位"
						
		WDT_APB_RST	0	rw	0x0	"WDT APB 时钟复位：
						0：不复位
						1：复位"
*/
void wdtSlcr_CLk_Init(void)
{
	slcr_write(0x32C, 0x00C1);
	return;
}

void wdtSlcr_Ctrl_Rst(void)
{
	slcr_write(0x330, 0x0003);
	taskDelay(1);
	slcr_write(0x330, 0x0000);
	return;
}

void wdtSlcr_Wdt_Pause(void)
{
	UINT32 tmp32 = 0;

	tmp32 = slcr_read(0x32C);
	tmp32 |= 0x0010;
	
	slcr_write(0x32C, tmp32);
	return;
}

void wdtSlcr_Wdt_exitPause(void)
{
	UINT32 tmp32 = 0;

	tmp32 = slcr_read(0x32C);
	tmp32 &= (~0x0010);
	
	slcr_write(0x32C, tmp32);
	return;
}



/**/
/* watchdog reset exterl peripheral*/
/**/
void wdtSlcr_Set_MIO(void)
{
   /* FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x73C, 0x00001660U);*/
   /**/
   /* MIO[15]  SWDT Reset*/
   /**/
   slcr_write(0x73C, 0x1660);
   
   return;
}

#endif

#if 1

int vxInit_Wdt(void)
{
	vxT_WDT * pWdt = NULL;
	vxT_WDT_CTRL * pWdtCtrl = NULL;
	
	int ret = 0;	
	UINT32 tmp32 = 0;

	/*
	init the pWdt structure
	*/
	if (g_pWdt->pWdtCtrl->status == 1)
	{
		return 0;  /* already init*/
	}
	
	/**/
	/* spi_ctrl select*/
	/**/
	pWdt = g_pWdt;	
	bzero(pWdt, sizeof(vxT_WDT));
	
	pWdt->pWdtCtrl = (vxT_WDT_CTRL *)(&vxWdt_Ctrl.ctrl_x);
	bzero(pWdt->pWdtCtrl, sizeof(vxT_WDT_CTRL));

	pWdt->wdt_x = 0;
	pWdt->pWdtCtrl->devId = 0;
	pWdt->pWdtCtrl->cfgBaseAddr = VX_WDT_CFG_BASE;		
	
	pWdtCtrl = pWdt->pWdtCtrl;		
	pWdtCtrl->ctrl_x = pWdt->wdt_x;
	
	pWdtCtrl->devIP_ver = wdtCtrl_Rd_CfgReg32(pWdt, WDT_REG_COMP_VER);
	pWdtCtrl->devIP_type = wdtCtrl_Rd_CfgReg32(pWdt, WDT_REG_COMP_TYPE);

    /*
    // tclk = 10MHZ 100ns
    // 2^(16+0)*100ns = 6.55ms 
    // 2^(16+10)= 2^26 * 1/tclk = 3.3536*2= 6.7s(tclk = 10MHZ)
	*/
	wdtCtrl_Set_RspMode(pWdt, 1);
	
	wdtCtrl_Set_RPL(pWdt, RPL_4_PCLK); /* RPL_256_PCLK*/
	
	wdtCtrl_Set_TOP(pWdt, 0);
	wdtCtrl_Set_TOPINIT(pWdt, 10);
	
	wdtCtrl_Enable_WDT(pWdt, WDT_ENABLE);
	
	wdtCtrl_Restart(pWdt);
	
	pWdtCtrl->status = 1;  /* 1-init_ok, 0-no_init*/
	
	return;
}

#endif


#if 1

void test_wdt_reset(void)
{
	UINT64 tmp64_0 = 0;
	UINT64 tmp64 = 0;
	
	/**/
	/* init wdt clk */
	/**/
	wdtSlcr_CLk_Init();

	/**/
	/* init WDT*/
	/**/
	vxInit_Wdt();
	
	/*wdtSlcr_Set_MIO();  // for external reset*/
	
	wdtCtrl_Restart(g_pWdt);
	
	return;
}

#endif




