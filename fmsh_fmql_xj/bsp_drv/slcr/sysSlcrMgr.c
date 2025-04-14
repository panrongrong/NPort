/* sysSlcrMgr.c - FMSH SLCR Manager Driver */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
25Jul19,jc  Created.
*/

/* includes */

#include <vxWorks.h>

#include "config.h"
#include "fmsh_fmql.h"

#include "../common/fmsh_ps_parameters.h"
#ifdef DRV_FM_SDMMC
#include "../sdmmc/fmsh_sdmmc_hw.h"
#endif
#include "../../fmsh_fmql.h"

#include "sysSlcrMgr.h"

UINT32 get_HARD_OSC_HZ(void)
{
	return HARD_OSC_HZ;
}

void fmqlSlcrWrite(UINT32 offset, UINT32 value)
{
    FMQL_WRITE_32((FMQL_SR_BASE + offset), value);
	return;
}

UINT32 fmqlSlcrRead(UINT32 offset)
{
	return FMQL_READ_32(FMQL_SR_BASE + offset);
}


#if 1
void slcr_write(UINT32 offset, UINT32 value)
{
	/* 	unlock slcr	*/
	fmqlSlcrWrite (FMQL_SR_UNLOCK_OFFSET, FMQL_SR_UNLOCK_ALL);  

    fmqlSlcrWrite(offset, value);

	/* lock slcr */
	fmqlSlcrWrite (FMQL_SR_LOCK_OFFSET, FMQL_SR_LOCK_ALL);  
	return;
}

UINT32 slcr_read(UINT32 offset)
{
	UINT32 tmp32 = 0;
	
	/* 	unlock slcr	*/
	fmqlSlcrWrite (FMQL_SR_UNLOCK_OFFSET, FMQL_SR_UNLOCK_ALL);  
	
    tmp32 = fmqlSlcrRead(offset);
	
	/* lock slcr */
	fmqlSlcrWrite (FMQL_SR_LOCK_OFFSET, FMQL_SR_LOCK_ALL);  

	return tmp32;
}
void slcr_show(UINT32 offset)
{
	UINT32 val = 0;
	
	val = slcr_read(offset);
	
	printf("slcr(0x%X)= 0x%04X_%04X \n------------\n", offset, ((val&0xFFFF0000)>>16), (val&0xFFFF));
	
	switch (offset)
	{
	/*gmac*/
	case 0x414:  /* GMAC_CRTL */
		switch ((val & 0x70) >> 4)
		{
		case 0:
			printf("GMAC_CRTL->GMAC_PHY_INTF_SEL2(6:4)= 000: GMII or MII \n");
			break;
		case 1:
			printf("GMAC_CRTL->GMAC_PHY_INTF_SEL2(6:4)= 001: RGMII \n");
			break;
		}
		switch (val & 0x07)
		{
		case 0:
			printf("GMAC_CRTL->GMAC_PHY_INTF_SEL1(2:0)= 000: GMII or MII \n");
			break;
		case 1:
			printf("GMAC_CRTL->GMAC_PHY_INTF_SEL1(2:0)= 001: RGMII \n");
			break;
		}		
		break;
		
	case 0x268:  /* GEM0_CLK_CTRL */
		printf("GEM0_CLK_CTRL->GEM0_TX_DIVISOR(14:9)= %d \n", ((val&0x7E00) >> 9));
		switch ((val&0x1C0) >> 6)
		{
		case 0:
			printf("GEM0_CLK_CTRL->GEM0_TX_SRCSEL(8:6)= 0x%X:IO PLL \n", ((val&0x1C0) >> 6));
			break;
		case 1:
			printf("GEM0_CLK_CTRL->GEM0_TX_SRCSEL(8:6)= 0x%X:ARM PLL \n", ((val&0x1C0) >> 6));
			break;
		default:
			printf("GEM0_CLK_CTRL->GEM0_TX_SRCSEL(8:6)= 0x%X-1xx:EMIO \n", ((val&0x1C0) >> 6));
			break;
		}		
		if (val&0x20)
		{
			printf("GEM0_CLK_CTRL->GEM0_RX_SRCSEL(5)= 1:EMIO \n");
		}
		else
		{
			printf("GEM0_CLK_CTRL->GEM0_RX_SRCSEL(5)= 0:MIO \n");
		}
		if (val&0x10)
		{
			printf("GEM0_CLK_CTRL->GEM0_AHB_CLKACT(4)= 1:enable \n");
		}
		else
		{
			printf("GEM0_CLK_CTRL->GEM0_AHB_CLKACT(4)= 0:disable \n");
		}
		if (val&0x08)
		{
			printf("GEM0_CLK_CTRL->GEM0_AXI_CLKACT(3)= 1:enable \n");
		}
		else
		{
			printf("GEM0_CLK_CTRL->GEM0_AXI_CLKACT(3)= 0:disable \n");
		}
		if (val&0x02)
		{
			printf("GEM0_CLK_CTRL->GEM0_RX_CLKACT(1)= 1:enable \n");
		}
		else
		{
			printf("GEM0_CLK_CTRL->GEM0_RX_CLKACT(1)= 0:disable \n");
		}
		if (val&0x01)
		{
			printf("GEM0_CLK_CTRL->GEM0_TX_CLKACT(0)= 1:enalbe \n");
		}
		else
		{
			printf("GEM0_CLK_CTRL->GEM0_TX_CLKACT(0)= 0:disable \n");
		}
		break;

	/* TOPSW_CLK_CTRL */
	case 0x204:
		if (val&0x08)
		{
			printf("TOPSW_CLK_CTRL->AMBA_AXI_CLKACT(3)= 1:enable \n");
		}
		else
		{
			printf("TOPSW_CLK_CTRL->AMBA_AXI_CLKACT(3)= 0:disable \n");
		}
		if (val&0x04)
		{
			printf("TOPSW_CLK_CTRL->AMBA_AHB_CLKACT(2)= 1:enable \n");
		}
		else
		{
			printf("TOPSW_CLK_CTRL->AMBA_AHB_CLKACT(2)= 0:disable \n");
		}
		if (val&0x02)
		{
			printf("TOPSW_CLK_CTRL->AMBA_APB_CLKACT(1)= 1:enable \n");
		}
		else
		{
			printf("TOPSW_CLK_CTRL->AMBA_APB_CLKACT(1)= 0:disable \n");
		}
		if (val&0x01)
		{
			printf("TOPSW_CLK_CTRL->TOPSW_CLK(0)= 1:close \n");
		}
		else
		{
			printf("TOPSW_CLK_CTRL->TOPSW_CLK(0)= 0:open \n");
		}
		break;
		
	/* TOPSW_RST_CTRL */
	case 0x208:
		if (val&0x01)
		{
			printf("TOPSW_RST_CTRL->TOPSW_RST(0)= 1:reset \n");
		}
		else
		{
			printf("TOPSW_RST_CTRL->TOPSW_RST(0)= 0:no reset \n");
		}
		break;

	/*
	ARM_CLK_CTRL	0x210	
	---------------------
	reserved	31:10	r	0x0 Reserved. 閸愭瑦妫ら弫鍫礉鐠囪璐�
	CPU_CLKACT	9	rw	0x1 "CPU閺冨爼鎸撴担鑳厴
						0閿涙矮绗夋担鑳厴						1閿涙矮濞囬懗锟�
	CPU_AXI_CLKACT	8	rw	0x1 "CPU AXI閹崵鍤庨弮鍫曟寭娴ｈ儻鍏�
						0閿涙矮绗夋担鑳厴						1閿涙矮濞囬懗锟�
	CPU_CLK_DIVISRO 7:2 rw	0x1 閻劋绨珺YPASS濡�绱℃稉瀣娴ｅ海閮寸紒鐔碱暥閻滃洢锟介敍鍫濐樉閻劋绨IC閸掑棝顣堕崳顭掔礆
	CPU_SRCSEL (persistant bits)	1:0 rw	0x0 "CPU閺冨爼鎸撳┃鎰帮拷閹封斂锟�
			0x閿涙RM PLL			10閿涙DR PLL			11閿涙O PLL"
	*/
	case 0x210:
		if (val&0x200)
		{
			printf("ARM_CLK_CTRL->CPU_CLKACT(9)= 1:enable \n");
		}
		else
		{
			printf("ARM_CLK_CTRL->CPU_CLKACT(9)= 0:disable \n");
		}
		if (val&0x100)
		{
			printf("ARM_CLK_CTRL->CPU_AXI_CLKACT(8)= 1:enable \n");
		}
		else
		{
			printf("ARM_CLK_CTRL->CPU_AXI_CLKACT(8)= 0:disable \n");
		}
		printf("ARM_CLK_CTRL->CPU_CLK_DIVISRO(7:2)= %d \n", ((val&0xFC)>>2));
		switch (val&0x03)
		{
		case 0x2:
			printf("ARM_CLK_CTRL->CPU_SRCSEL(1:0)= 10閿涙DR PLL \n");
			break;
		case 0x3:
			printf("ARM_CLK_CTRL->CPU_SRCSEL(1:0)= 11閿涙O PLL \n");
			break;
		default:
			printf("ARM_CLK_CTRL->CPU_SRCSEL(1:0)= 0x閿涙RM PLL \n");
			break;
		}
		break;
	/*
	SDIO_CLK_CTRL (SDMMC)	0x28C	reserved	31:5	rw	0x0 Reserved. 閸愭瑦妫ら弫鍫礉鐠囪璐�
			SDIO_SRCSEL         4	rw	0x0 "SDIO REF閺冨爼鎸撻柅澶嬪
				0閿涙O PLL				1閿涙RM PLL"
			SDIO1_AHB_CLKACT	3	rw	0x1 "SDIO1 AHB閺冨爼鎸撴担鑳厴
				0閿涙矮绗夋担鑳厴				1閿涙矮濞囬懗锟�
			SDIO0_AHB_CLKACT	2	rw	0x1 "SDIO0 AHB閺冨爼鎸撴担鑳厴
				0閿涙矮绗夋担鑳厴				1閿涙矮濞囬懗锟�
			SDIO1_CLKACT	    1	rw	0x1 "SDIO1 REF閺冨爼鎸撴担鑳厴
				0閿涙矮绗夋担鑳厴				1閿涙矮濞囬懗锟�
			SDIO0_CLKACT	    0	rw	0x1 "SDIO0 REF閺冨爼鎸撴担鑳厴
				0閿涙矮绗夋担鑳厴				1閿涙矮濞囬懗锟�
	*/
	case 0x28C:		
		if (val&0x10)
		{
			printf("SDIO_CLK_CTRL->SDIO_SRCSEL(4)= 1:ARM PLL \n");
		}
		else
		{
			printf("SDIO_CLK_CTRL->SDIO_SRCSEL(4)= 0:IO PLL \n");
		}		
		if (val&0x08)
		{
			printf("SDIO_CLK_CTRL->SDIO1_AHB_CLKACT(3)= 1:enable  \n");
		}
		else
		{
			printf("SDIO_CLK_CTRL->SDIO1_AHB_CLKACT(3)= 0:disable \n");
		}
		if (val&0x04)
		{
			printf("SDIO_CLK_CTRL->SDIO0_AHB_CLKACT(2)= 1:enable  \n");
		}
		else
		{
			printf("SDIO_CLK_CTRL->SDIO0_AHB_CLKACT(2)= 0:disable \n");
		}
		if (val&0x02)
		{
			printf("SDIO_CLK_CTRL->SDIO1_CLKACT(1)= 1:enable  \n");
		}
		else
		{
			printf("SDIO_CLK_CTRL->SDIO1_CLKACT(1)= 0:disable \n");
		}
		if (val&0x01)
		{
			printf("SDIO_CLK_CTRL->SDIO0_CLKACT(1)= 1:enable  \n");
		}
		else
		{
			printf("SDIO_CLK_CTRL->SDIO0_CLKACT(1)= 0:disable \n");
		}
		break;
		
	/********
	default:
		printf("slcr(0x%X)= 0x%08X \n", offset, val);
		break;	
	*********/
	}

	printf("------------\n");
	return;
}

#endif


#if 1

int g_show_flag = 0;	

UINT32 clk_ttc_get(void)
{
	UINT32 OSC_HZ = HARD_OSC_HZ;  /* 33M Hz */
	
	UINT32 tmp32 = 0, ret = 0;
	UINT32 arm_pll_fdiv = 0;
	UINT32 clk0_div = 0;
		
	tmp32 = slcr_read(ARM_PLL_CTRL);
	arm_pll_fdiv = (tmp32 & ARM_PLL_FDIV_MASK) >> ARM_PLL_FDIV_SHIFT;
	if (g_show_flag)
		printf("arm_pll_fdiv: %d \n", arm_pll_fdiv);

	tmp32 = slcr_read(ARM_PLL_CLKOUT0_DIVISOR);
	clk0_div = (tmp32 & ARM_PLL_CLKOUT0_DIV_MASK) >> ARM_PLL_CLKOUT0_DIV_SHIFT;
	if (g_show_flag)
		printf("clk0_div: %d \n", clk0_div);

	tmp32 = slcr_read(CLK_621_TRUE);
	if (tmp32 & 0x01)
	{
		ret = ((OSC_HZ * arm_pll_fdiv) / clk0_div) / (3 * 2 * 2);
		if (g_show_flag)
			printf("CLK_621_TRUE: %d (6:2:1) \n", tmp32);
	}
	else
	{
		ret = ((OSC_HZ * arm_pll_fdiv) / clk0_div) / (2 * 2 * 2);
		if (g_show_flag)
			printf("CLK_621_TRUE: %d (4:2:1) \n", tmp32);
	}

	return ret;
}

void clk_ttc_show(void)
{
	g_show_flag = 1;
	printf("clk_ttc: %d Hz \n", clk_ttc_get());
	g_show_flag = 0;
	return;
}

/*
clk0 --> cpu
*/
UINT32 clk_cpu_get(void)
{
	UINT32 OSC_HZ = HARD_OSC_HZ;  /* 33M Hz */
	
	UINT32 tmp32 = 0, ret = 0;
	UINT32 pll_fdiv = 0;	
	UINT32 clk0_div = 0;

	tmp32 = slcr_read(ARM_CLK_CTRL);
	
	switch (tmp32 & ARM_CPU_SRCSEL_MASK)
	{
	case 0x02: /* 0b10閿涙DR PLL*/
		{	
			tmp32 = slcr_read(DDR_PLL_CTRL);
			pll_fdiv = (tmp32 & DDR_PLL_FDIV_MASK) >> DDR_PLL_FDIV_SHIFT;
			
			tmp32 = slcr_read(DDR_PLL_CLKOUT0_DIVISOR);
			clk0_div = (tmp32 & DDR_PLL_CLKOUT0_DIV_MASK) >> DDR_PLL_CLKOUT0_DIV_SHIFT;
			
			if (g_show_flag)
				printf("CPU_SRCSEL: DDR PLL \n");
		}
		break;	
	case 0x03: /* 0b11閿涙O PLL */
		{	
			tmp32 = slcr_read(IO_PLL_CTRL);
			pll_fdiv = (tmp32 & IO_PLL_FDIV_MASK) >> IO_PLL_FDIV_SHIFT;
			
			tmp32 = slcr_read(IO_PLL_CLKOUT0_DIVISOR);
			clk0_div = (tmp32 & IO_PLL_CLKOUT0_DIV_MASK) >> IO_PLL_CLKOUT0_DIV_SHIFT;
			
			if (g_show_flag)
				printf("CPU_SRCSEL: IO PLL \n");		
		}
		break;
	default:   /* 0b0x閿涙RM PLL*/
		tmp32 = slcr_read(ARM_PLL_CTRL);
		pll_fdiv = (tmp32 & ARM_PLL_FDIV_MASK) >> ARM_PLL_FDIV_SHIFT;
		
		tmp32 = slcr_read(ARM_PLL_CLKOUT0_DIVISOR);
		clk0_div = (tmp32 & ARM_PLL_CLKOUT0_DIV_MASK) >> ARM_PLL_CLKOUT0_DIV_SHIFT;
		
		if (g_show_flag)
			printf("CPU_SRCSEL: ARM PLL \n");
		break;
	}
	
	ret = (OSC_HZ * pll_fdiv) / clk0_div;
	if (g_show_flag)
		printf("pll_fdiv:%d, clk0_div:%d \n", pll_fdiv, clk0_div);

	return ret;
}

void clk_cpu_show(void)
{
	g_show_flag = 1;
	printf("clk0_cpu: %d Hz \n", clk_cpu_get());
	g_show_flag = 0;
	return;
}

UINT32 clk_ddr_get(void)
{
	UINT32 OSC_HZ = HARD_OSC_HZ;  /* 33M Hz */
	
	UINT32 tmp32 = 0, ret = 0;
	UINT32 pll_fdiv = 0;	
	UINT32 clk1_div = 0, ddr_div4 = 1;

	tmp32 = slcr_read(DDR_PLL_CTRL);
	pll_fdiv = (tmp32 & DDR_PLL_FDIV_MASK) >> DDR_PLL_FDIV_SHIFT;
	
	tmp32 = slcr_read(DDR_PLL_CLKOUT1_DIVISOR);
	clk1_div = (tmp32 & DDR_PLL_CLKOUT1_DIV_MASK) >> DDR_PLL_CLKOUT1_DIV_SHIFT;

	tmp32 = slcr_read(DDR_CLK_CTRL);
	if (tmp32 & DDR_CLKACT_4X_MASK)
	{
		ddr_div4 = 1;
	}
	else
	{
		ddr_div4 = 4;
	}

	ret = ((OSC_HZ * pll_fdiv) / clk1_div) / ddr_div4 * 2;  /* ddr_ip: 2 閸婂秹顣�*/ 
	
	/* printf("((OSC_HZ * pll_fdiv) / clk1_div) / ddr_div4 \n"); */
	if (g_show_flag)
		printf("pll_fdiv:%d, clk1_div:%d ddr_div4:%d \n", pll_fdiv, clk1_div, ddr_div4);

	return ret;
}

/*
ddr data rate: bps
*/
UINT32 ddr_dRate_get(void) 
{
	return clk_ddr_get()*2;
}

void clk_ddr_show(void)
{
	UINT32 tmp32 =  clk_ddr_get();
	g_show_flag = 1;
	
	printf("clk1_ddr: %d Hz \n", tmp32);
	printf("rate_ddr: %d bps \n", (tmp32*2));
	
	g_show_flag = 0;
	return;
}

/*
clk --> gtc(generic timer)
*/
UINT32 clk_gtc_get(void)
{
	UINT32 OSC_HZ = HARD_OSC_HZ;  /* 33M Hz */
	UINT32 ret = 0;
	UINT32 clk_div = 0;

	clk_div = slcr_read(GTIMER_CLK_CTRL) & GTIMER_CLK_CTR_MASKL;
	ret = (OSC_HZ) / clk_div;
	
	if (g_show_flag)
		printf("clk_div:%d \n", clk_div);

	return ret;
}

void clk_gtc_show(void)
{
	g_show_flag = 1;
	printf("clk_gtc: %d Hz \n", clk_gtc_get());
	g_show_flag = 0;
	return;
}


/*
clk1 --> gmax_tx
*/
/*
gmac_0
*/
UINT32 clk_gem0_tx_get(void)
{
	UINT32 OSC_HZ = HARD_OSC_HZ;  /* 33M Hz */
	
	UINT32 tmp32 = 0, ret = 0;
	UINT32 pll_fdiv = 0;
	
	UINT32 clk1_div = 0;
	UINT32 gem0_tx_div = 0;

	tmp32 = slcr_read(GEM0_CLK_CTRL);

	/* 2 绾�鍒嗛*/
	gem0_tx_div = (tmp32 & GEM0_TX_DIVISOR_MASK) >> GEM0_TX_DIVISOR_SHIFT;	
	
	tmp32 = (tmp32 & GEM0_TX_SRCSEL_MASK) >> GEM0_TX_SRCSEL_SHIFT;
	if (tmp32 == 0x0)
	{	
		/* 0 绾�鍊嶉*/
		tmp32 = slcr_read(IO_PLL_CTRL);
		pll_fdiv = (tmp32 & IO_PLL_FDIV_MASK) >> IO_PLL_FDIV_SHIFT;

		/* 1 绾�鍒嗛厤*/
		tmp32 = slcr_read(IO_PLL_CLKOUT1_DIVISOR);
		clk1_div = (tmp32 & IO_PLL_CLKOUT1_DIV_MASK) >> IO_PLL_CLKOUT1_DIV_SHIFT;
		
		if (g_show_flag)
			printf("GEM0_TX_SRCSEL: IO PLL \n");		
	}
	else if (tmp32 == 0x1)
	{
		tmp32 = slcr_read(ARM_PLL_CTRL);
		pll_fdiv = (tmp32 & ARM_PLL_FDIV_MASK) >> ARM_PLL_FDIV_SHIFT;
		
		tmp32 = slcr_read(ARM_PLL_CLKOUT1_DIVISOR);
		clk1_div = (tmp32 & ARM_PLL_CLKOUT1_DIV_MASK) >> ARM_PLL_CLKOUT1_DIV_SHIFT;
		
		if (g_show_flag)
			printf("GEM0_TX_SRCSEL: ARM PLL \n");
	}
	else
	{
		if (g_show_flag)
			printf("GEM0_TX_SRCSEL: DDR PLL, return! \n");
		return 0;
	}
	
	ret = ((OSC_HZ * pll_fdiv) / clk1_div) / (gem0_tx_div);
	if (g_show_flag)
		printf("pll_fdiv:%d, clk1_div:%d, gem0_tx_div:%d \n", pll_fdiv, clk1_div, gem0_tx_div);

	return ret;
}

/*
eth_speed:
3 - 1G
2 - 100M
1 - 10M
*/
UINT32 clk_gem0_tx_div_get(int eth_speed)
{
	UINT32 OSC_HZ = HARD_OSC_HZ;  /* 33M Hz */
	
	UINT32 tmp32 = 0, ret = 0;
	UINT32 pll_fdiv = 0;
	
	UINT32 clk1_div = 0;
	UINT32 gem0_tx_div = 0;

	tmp32 = slcr_read(GEM0_CLK_CTRL);
	
	/*gem0_tx_div = (tmp32 & GEM0_TX_DIVISOR_MASK) >> GEM0_TX_DIVISOR_SHIFT;	*/
	
	tmp32 = (tmp32 & GEM0_TX_SRCSEL_MASK) >> GEM0_TX_SRCSEL_SHIFT;
	if (tmp32 == 0x0)
	{
		tmp32 = slcr_read(IO_PLL_CTRL);
		pll_fdiv = (tmp32 & IO_PLL_FDIV_MASK) >> IO_PLL_FDIV_SHIFT;
		
		tmp32 = slcr_read(IO_PLL_CLKOUT1_DIVISOR);
		clk1_div = (tmp32 & IO_PLL_CLKOUT1_DIV_MASK) >> IO_PLL_CLKOUT1_DIV_SHIFT;
	}
	else if (tmp32 == 0x1)
	{
		tmp32 = slcr_read(ARM_PLL_CTRL);
		pll_fdiv = (tmp32 & ARM_PLL_FDIV_MASK) >> ARM_PLL_FDIV_SHIFT;
		
		tmp32 = slcr_read(ARM_PLL_CLKOUT1_DIVISOR);
		clk1_div = (tmp32 & ARM_PLL_CLKOUT1_DIV_MASK) >> ARM_PLL_CLKOUT1_DIV_SHIFT;
	}

	if (0x03 == eth_speed) /* ETH_SPEED_1G*/
	{
		/*ret = ((OSC_HZ *pll_fdiv) / clk1_div) / (gem0_tx_div);*/
		gem0_tx_div = (((OSC_HZ * pll_fdiv) / clk1_div) / 1000000 + 10) / 125;
	}
	else if (0x02 == eth_speed)  /*ETH_SPEED_100M*/
	{
		gem0_tx_div = (((OSC_HZ * pll_fdiv) / clk1_div) / 1000000 + 10) / 25;
	}
	else /*ETH_SPEED_10M*/
	{
		gem0_tx_div = (((OSC_HZ * pll_fdiv) / clk1_div) / 1000000 + 1) * 10 / 25;
	}

	return gem0_tx_div;
}


void clk_gem0_show(void)
{
	g_show_flag = 1;
	printf("clk1_gem0_tx: %d Hz \n", clk_gem0_tx_get());
	g_show_flag = 0;
	return;
}

/*
gmac_1
*/
UINT32 clk_gem1_tx_get(void)
{
	UINT32 OSC_HZ = HARD_OSC_HZ;  /* 33M Hz */
	
	UINT32 tmp32 = 0, ret = 0;
	UINT32 pll_fdiv = 0;
	
	UINT32 clk1_div = 0;
	UINT32 gem1_tx_div = 0;

	tmp32 = slcr_read(GEM1_CLK_CTRL);
	
	gem1_tx_div = (tmp32 & GEM1_TX_DIVISOR_MASK) >> GEM1_TX_DIVISOR_SHIFT;	
	
	tmp32 = (tmp32 & GEM1_TX_SRCSEL_MASK) >> GEM1_TX_SRCSEL_SHIFT;
	if (tmp32 == 0x0)
	{
		tmp32 = slcr_read(IO_PLL_CTRL);
		pll_fdiv = (tmp32 & IO_PLL_FDIV_MASK) >> IO_PLL_FDIV_SHIFT;
		
		tmp32 = slcr_read(IO_PLL_CLKOUT1_DIVISOR);
		clk1_div = (tmp32 & IO_PLL_CLKOUT1_DIV_MASK) >> IO_PLL_CLKOUT1_DIV_SHIFT;
		
		if (g_show_flag)
			printf("GEM1_TX_SRCSEL: IO PLL \n");		
	}
	else if (tmp32 == 0x1)
	{
		tmp32 = slcr_read(ARM_PLL_CTRL);
		pll_fdiv = (tmp32 & ARM_PLL_FDIV_MASK) >> ARM_PLL_FDIV_SHIFT;
		
		tmp32 = slcr_read(ARM_PLL_CLKOUT1_DIVISOR);
		clk1_div = (tmp32 & ARM_PLL_CLKOUT1_DIV_MASK) >> ARM_PLL_CLKOUT1_DIV_SHIFT;
		
		if (g_show_flag)
			printf("GEM1_TX_SRCSEL: ARM PLL \n");
	}
	else
	{
		if (g_show_flag)
			printf("GEM1_TX_SRCSEL: DDR PLL, return! \n");
		
		return 0;
	}
	
	ret = ((OSC_HZ * pll_fdiv) / clk1_div) / (gem1_tx_div);
	if (g_show_flag)
		printf("pll_fdiv:%d, clk1_div:%d, gem1_tx_div:%d \n", pll_fdiv, clk1_div, gem1_tx_div);

	return ret;
}


/*
eth_speed:
3 - 1G
2 - 100M
1 - 10M
*/
UINT32 clk_gem1_tx_div_get(int eth_speed)
{
	UINT32 OSC_HZ = HARD_OSC_HZ;  /* 33M Hz */
	
	UINT32 tmp32 = 0, ret = 0;
	UINT32 pll_fdiv = 0;
	
	UINT32 clk1_div = 0;
	UINT32 gem1_tx_div = 0;

	tmp32 = slcr_read(GEM1_CLK_CTRL);
	tmp32 = (tmp32 & GEM1_TX_SRCSEL_MASK) >> GEM1_TX_SRCSEL_SHIFT;
	if (tmp32 == 0x0)
	{
		tmp32 = slcr_read(IO_PLL_CTRL);
		pll_fdiv = (tmp32 & IO_PLL_FDIV_MASK) >> IO_PLL_FDIV_SHIFT;
		
		tmp32 = slcr_read(IO_PLL_CLKOUT1_DIVISOR);
		clk1_div = (tmp32 & IO_PLL_CLKOUT1_DIV_MASK) >> IO_PLL_CLKOUT1_DIV_SHIFT;
	}
	else if (tmp32 == 0x1)
	{
		tmp32 = slcr_read(ARM_PLL_CTRL);
		pll_fdiv = (tmp32 & ARM_PLL_FDIV_MASK) >> ARM_PLL_FDIV_SHIFT;
		
		tmp32 = slcr_read(ARM_PLL_CLKOUT1_DIVISOR);
		clk1_div = (tmp32 & ARM_PLL_CLKOUT1_DIV_MASK) >> ARM_PLL_CLKOUT1_DIV_SHIFT;
	}
	
	if (0x03 == eth_speed) /* ETH_SPEED_1G*/
	{
		/*ret = ((OSC_HZ *pll_fdiv) / clk1_div) / (gem1_tx_div);*/
		gem1_tx_div = (((OSC_HZ * pll_fdiv) / clk1_div) / 1000000 + 10) / 125;
	}
	else if (0x02 == eth_speed)  /* ETH_SPEED_100M*/
	{
		gem1_tx_div = (((OSC_HZ * pll_fdiv) / clk1_div) / 1000000 + 10) / 25;
	}	
	else /* 0x01 - ETH_SPEED_10M*/
	{
		gem1_tx_div = (((OSC_HZ * pll_fdiv) / clk1_div) / 1000000 + 1) * 10 / 25;
	}

	return gem1_tx_div;
}


void clk_gem1_show(void)
{
	g_show_flag = 1;
	printf("clk1_gem1_tx: %d Hz \n", clk_gem1_tx_get());
	g_show_flag = 0;
	return;
}

void slcr_gem_reset(void)
{
	/*
	GEM_RST_CTRL	0x270	reserved	31:9	rw	0x0	Reserved. 鍐欐棤鏁堬紝璇讳负0
	---------------------
			GEM1_TX_RST	8	rw	0x0	"GMAC1 Tx鏃堕挓澶嶄綅
						0锛氫笉澶嶄綅		1锛氬浣�
						
			GEM1_RX_RST	7	rw	0x0	"GMAC1 Rx鏃堕挓澶嶄綅
						0锛氫笉澶嶄綅		1锛氬浣�
			GEM1_AHB_RST	6	rw	0x1	"GMAC1 AHB鏃堕挓澶嶄綅 (璇ュ浣嶇敱cpu閲婃斁锛�
						0锛氫笉澶嶄綅		1锛氬浣�
			GEM1_AXI_RST	5	rw	0x0	"GMAC1 AXI鏃堕挓澶嶄綅
						0锛氫笉澶嶄綅		1锛氬浣�						
			reserved	4	rw	0x0	Reserved. 鍐欐棤鏁堬紝璇讳负0
			
			GEM0_TX_RST	3	rw	0x0	"GMAC0 Tx鏃堕挓澶嶄綅
						0锛氫笉澶嶄綅		1锛氬浣�
			GEM0_RX_RST	2	rw	0x0	"GMAC0 Rx鏃堕挓澶嶄綅
						0锛氫笉澶嶄綅		1锛氬浣�
			GEM0_AHB_RST	1	rw	0x1	"GMAC0 AHB鏃堕挓澶嶄綅(璇ュ浣嶇敱cpu閲婃斁锛�
						0锛氫笉澶嶄綅		1锛氬浣�
			GEM0_AXI_RST	0	rw	0x0	"GMAC0 AXI鏃堕挓澶嶄綅
						0锛氫笉澶嶄綅		1锛氬浣�
	*/
	int i = 0;
	
	/* *(UINT32*(0xE0026000 + 0x270)) = 0x18C;*/
	slcr_write(0x270, 0x18C);
	
	for (i=0; i<1000;  i++);  /* delay*/
	
	/* *(UINT32*(0xE0026000 + 0x270)) = 0x000;*/
	slcr_write(0x270, 0x000);

	return;	
}


/*
clk5 --> uart
*/
UINT32 clk_uart_get(void)
{
	UINT32 OSC_HZ = HARD_OSC_HZ;  /* 33M Hz */
	
	UINT32 tmp32 = 0, ret = 0;
	UINT32 pll_fdiv = 0;	
	UINT32 clk5_div = 0;

	tmp32 = slcr_read(UART_CLK_CTRL);
	
	if (tmp32 & UART_SRCSEL_MASK)  /* 1閿涙RM PLL */
	{	
		tmp32 = slcr_read(ARM_PLL_CTRL);
		pll_fdiv = (tmp32 & ARM_PLL_FDIV_MASK) >> ARM_PLL_FDIV_SHIFT;
		
		tmp32 = slcr_read(ARM_PLL_CLKOUT5_DIVISOR);
		clk5_div = (tmp32 & ARM_PLL_CLKOUT5_DIV_MASK) >> ARM_PLL_CLKOUT5_DIV_SHIFT;
		
		if (g_show_flag)
			printf("UART_SRCSEL: ARM PLL \n");
	}
	else
	{
		tmp32 = slcr_read(IO_PLL_CTRL);
		pll_fdiv = (tmp32 & IO_PLL_FDIV_MASK) >> IO_PLL_FDIV_SHIFT;
		
		tmp32 = slcr_read(IO_PLL_CLKOUT5_DIVISOR);
		clk5_div = (tmp32 & IO_PLL_CLKOUT5_DIV_MASK) >> IO_PLL_CLKOUT5_DIV_SHIFT;
		
		if (g_show_flag)
			printf("UART_SRCSEL: IO PLL \n");		
	}
	
	ret = (OSC_HZ * pll_fdiv) / clk5_div;
	if (g_show_flag)
		printf("pll_fdiv:%d, clk5_div:%d \n", pll_fdiv, clk5_div);

	return ret;
}

void clk_uart_show(void)
{
	g_show_flag = 1;
	printf("clk5_uart: %d Hz \n", clk_uart_get());
	g_show_flag = 0;
	return;
}

/*
clk2 --> sdmmc
*/
UINT32 clk_sdmmc_get(void)
{
	UINT32 OSC_HZ = HARD_OSC_HZ;  /* 33M Hz */
	
	UINT32 tmp32 = 0, ret = 0;
	UINT32 pll_fdiv = 0;	
	UINT32 clk2_div = 0;

	tmp32 = slcr_read(SDIO_CLK_CTRL);
	
	if (tmp32 & SDIO_SRCSEL_MASK)  /* 1閿涙RM PLL */
	{	
		tmp32 = slcr_read(ARM_PLL_CTRL);
		pll_fdiv = (tmp32 & ARM_PLL_FDIV_MASK) >> ARM_PLL_FDIV_SHIFT;
		
		tmp32 = slcr_read(ARM_PLL_CLKOUT2_DIVISOR);
		clk2_div = (tmp32 & ARM_PLL_CLKOUT2_DIV_MASK) >> ARM_PLL_CLKOUT2_DIV_SHIFT;
		
		if (g_show_flag)
			printf("SDIO_SRCSEL: ARM PLL \n");
	}
	else
	{
		tmp32 = slcr_read(IO_PLL_CTRL);
		pll_fdiv = (tmp32 & IO_PLL_FDIV_MASK) >> IO_PLL_FDIV_SHIFT;
		
		tmp32 = slcr_read(IO_PLL_CLKOUT2_DIVISOR);
		clk2_div = (tmp32 & IO_PLL_CLKOUT2_DIV_MASK) >> IO_PLL_CLKOUT2_DIV_SHIFT;
		
		if (g_show_flag)
			printf("SDIO_SRCSEL: IO PLL \n");		
	}
	
	ret = (OSC_HZ * pll_fdiv) / clk2_div;
	if (g_show_flag)
		printf("pll_fdiv:%d, clk2_div:%d \n", pll_fdiv, clk2_div);

	return ret;
}

void clk_sdmmc_show(void)
{
	g_show_flag = 1;
	printf("clk2_sdmmc: %d Hz \n", clk_sdmmc_get());
	g_show_flag = 0;
	return;
}

void clk_sdmmc_set(UINT32 val)
{
	slcr_write(IO_PLL_CLKOUT2_DIVISOR, val);
}

/*
clk3 --> spi
*/
UINT32 clk_spi_get(void)
{
	UINT32 OSC_HZ = HARD_OSC_HZ;  /* 33M Hz */
	
	UINT32 tmp32 = 0, ret = 0;
	UINT32 pll_fdiv = 0;	
	UINT32 clk3_div = 0;

	tmp32 = slcr_read(SPI_CLK_CTRL);
	
	if (tmp32 & SPI_SRCSEL_MASK)  /* 1閿涙RM PLL */
	{	
		tmp32 = slcr_read(ARM_PLL_CTRL);
		pll_fdiv = (tmp32 & ARM_PLL_FDIV_MASK) >> ARM_PLL_FDIV_SHIFT;
		
		tmp32 = slcr_read(ARM_PLL_CLKOUT3_DIVISOR);
		clk3_div = (tmp32 & ARM_PLL_CLKOUT3_DIV_MASK) >> ARM_PLL_CLKOUT3_DIV_SHIFT;
		
		if (g_show_flag)
			printf("SPI_SRCSEL: ARM PLL \n");
	}
	else
	{
		tmp32 = slcr_read(IO_PLL_CTRL);
		pll_fdiv = (tmp32 & IO_PLL_FDIV_MASK) >> IO_PLL_FDIV_SHIFT;
		
		tmp32 = slcr_read(IO_PLL_CLKOUT3_DIVISOR);
		clk3_div = (tmp32 & IO_PLL_CLKOUT3_DIV_MASK) >> IO_PLL_CLKOUT3_DIV_SHIFT;
		
		if (g_show_flag)
			printf("SPI_SRCSEL: IO PLL \n");		
	}
	
	ret = (OSC_HZ * pll_fdiv) / clk3_div;
	if (g_show_flag)
		printf("pll_fdiv:%d, clk3_div:%d \n", pll_fdiv, clk3_div);

	return ret;
}

void clk_spi_show(void)
{
	g_show_flag = 1;
	printf("clk3_spi: %d Hz \n", clk_spi_get());
	g_show_flag = 0;
	return;
}


/*
clk4 --> qspi
*/
UINT32 clk_qspi_get(void)
{
	UINT32 OSC_HZ = HARD_OSC_HZ;  /* 33M Hz */
	
	UINT32 tmp32 = 0, ret = 0;
	UINT32 pll_fdiv = 0;	
	UINT32 clk4_div = 0;

	tmp32 = slcr_read(QSPI_CLK_CTRL);
	
	if (tmp32 & QSPI_SRCSEL_MASK)  /* 1閿涙RM PLL */
	{	
		tmp32 = slcr_read(ARM_PLL_CTRL);
		pll_fdiv = (tmp32 & ARM_PLL_FDIV_MASK) >> ARM_PLL_FDIV_SHIFT;
		
		tmp32 = slcr_read(ARM_PLL_CLKOUT4_DIVISOR);
		clk4_div = (tmp32 & ARM_PLL_CLKOUT4_DIV_MASK) >> ARM_PLL_CLKOUT4_DIV_SHIFT;

		if (g_show_flag)
			printf("QSPI_SRCSEL: ARM PLL \n");
	}
	else
	{
		tmp32 = slcr_read(IO_PLL_CTRL);
		pll_fdiv = (tmp32 & IO_PLL_FDIV_MASK) >> IO_PLL_FDIV_SHIFT;
		
		tmp32 = slcr_read(IO_PLL_CLKOUT4_DIVISOR);
		clk4_div = (tmp32 & IO_PLL_CLKOUT4_DIV_MASK) >> IO_PLL_CLKOUT4_DIV_SHIFT;
		
		if (g_show_flag)
			printf("QSPI_SRCSEL: IO PLL \n");		
	}
	
	ret = (OSC_HZ * pll_fdiv) / clk4_div;
	if (g_show_flag)
		printf("pll_fdiv:%d, clk4_div:%d \n", pll_fdiv, clk4_div);

	return ret;
}

void clk_qspi_show(void)
{
	g_show_flag = 1;
	printf("clk4_qspi: %d Hz \n", clk_qspi_get());
	
	g_show_flag = 0;	
	return;
}


/*
clk0 --> nfc
*/
UINT32 clk_nfc_get(void)
{
	UINT32 OSC_HZ = HARD_OSC_HZ;  /* 33M Hz */
	
	UINT32 tmp32 = 0, ret = 0;
	UINT32 pll_fdiv = 0;	
	UINT32 clk0_div = 0;
	UINT32 nfc_div = 0;

	tmp32 = slcr_read(ARM_CLK_CTRL);
	
	switch (tmp32 & ARM_CPU_SRCSEL_MASK)
	{
	case 0x02: /* 0b10 DDR PLL*/
		{	
			tmp32 = slcr_read(DDR_PLL_CTRL);
			pll_fdiv = (tmp32 & DDR_PLL_FDIV_MASK) >> DDR_PLL_FDIV_SHIFT;
			
			tmp32 = slcr_read(DDR_PLL_CLKOUT0_DIVISOR);
			clk0_div = (tmp32 & DDR_PLL_CLKOUT0_DIV_MASK) >> DDR_PLL_CLKOUT0_DIV_SHIFT;
			
			if (g_show_flag)
				printf("CPU_SRCSEL: DDR PLL \n");
		}
		break;	
	case 0x03: /* 0b11 IO PLL */
		{	
			tmp32 = slcr_read(IO_PLL_CTRL);
			pll_fdiv = (tmp32 & IO_PLL_FDIV_MASK) >> IO_PLL_FDIV_SHIFT;
			
			tmp32 = slcr_read(IO_PLL_CLKOUT0_DIVISOR);
			clk0_div = (tmp32 & IO_PLL_CLKOUT0_DIV_MASK) >> IO_PLL_CLKOUT0_DIV_SHIFT;
			
			if (g_show_flag)
				printf("CPU_SRCSEL: IO PLL \n");		
		}
		break;
	default:   /* 0b0x ARM PLL*/
		tmp32 = slcr_read(ARM_PLL_CTRL);
		pll_fdiv = (tmp32 & ARM_PLL_FDIV_MASK) >> ARM_PLL_FDIV_SHIFT;
		
		tmp32 = slcr_read(ARM_PLL_CLKOUT0_DIVISOR);
		clk0_div = (tmp32 & ARM_PLL_CLKOUT0_DIV_MASK) >> ARM_PLL_CLKOUT0_DIV_SHIFT;
		
		if (g_show_flag)
			printf("CPU_SRCSEL: ARM PLL \n");
		break;
	}


	tmp32 = slcr_read(NFC_CLK_CTRL);	
	nfc_div = (tmp32 & NFC_REF_CLK_DIV_MASK) >> NFC_REF_CLK_DIV_SHIFT;
	if (g_show_flag)
		printf("nfc_div: %d \n", nfc_div);
	
	tmp32 = slcr_read(CLK_621_TRUE);	
	if (tmp32 & 0x01)
	{
		ret = ((OSC_HZ * pll_fdiv) / clk0_div) / (3 * 2 * nfc_div);
		if (g_show_flag)
			printf("CLK_621_TRUE: %d (6:2:1) \n", tmp32);
	}
	else
	{
		ret = ((OSC_HZ * pll_fdiv) / clk0_div) / (2 * 2 * nfc_div);
		if (g_show_flag)
			printf("CLK_621_TRUE: %d (4:2:1) \n", tmp32);
	}

	return ret;
}

void clk_nfc_show(void)
{
	g_show_flag = 1;
	printf("clk_nfc: %d Hz \n", clk_nfc_get());
	
	g_show_flag = 0;	
	return;
}


/*
clk0 --> pcap
*/
UINT32 clk_pcap_get(void)
{
	UINT32 OSC_HZ = HARD_OSC_HZ;  /* 33M Hz */
	
	UINT32 tmp32 = 0, ret = 0;
	UINT32 pll_fdiv = 0;	
	UINT32 clk0_div = 0;
	UINT32 pcap_div = 0;

	tmp32 = slcr_read(ARM_CLK_CTRL);
	
	switch (tmp32 & ARM_CPU_SRCSEL_MASK)
	{
	case 0x02: /* 0b10 DDR PLL*/
		{	
			tmp32 = slcr_read(DDR_PLL_CTRL);
			pll_fdiv = (tmp32 & DDR_PLL_FDIV_MASK) >> DDR_PLL_FDIV_SHIFT;
			
			tmp32 = slcr_read(DDR_PLL_CLKOUT0_DIVISOR);
			clk0_div = (tmp32 & DDR_PLL_CLKOUT0_DIV_MASK) >> DDR_PLL_CLKOUT0_DIV_SHIFT;
			
			if (g_show_flag)
				printf("CPU_SRCSEL: DDR PLL \n");
		}
		break;	
	case 0x03: /* 0b11 IO PLL */
		{	
			tmp32 = slcr_read(IO_PLL_CTRL);
			pll_fdiv = (tmp32 & IO_PLL_FDIV_MASK) >> IO_PLL_FDIV_SHIFT;
			
			tmp32 = slcr_read(IO_PLL_CLKOUT0_DIVISOR);
			clk0_div = (tmp32 & IO_PLL_CLKOUT0_DIV_MASK) >> IO_PLL_CLKOUT0_DIV_SHIFT;
			
			if (g_show_flag)
				printf("CPU_SRCSEL: IO PLL \n");		
		}
		break;
	default:   /* 0b0x ARM PLL*/
		tmp32 = slcr_read(ARM_PLL_CTRL);
		pll_fdiv = (tmp32 & ARM_PLL_FDIV_MASK) >> ARM_PLL_FDIV_SHIFT;
		
		tmp32 = slcr_read(ARM_PLL_CLKOUT0_DIVISOR);
		clk0_div = (tmp32 & ARM_PLL_CLKOUT0_DIV_MASK) >> ARM_PLL_CLKOUT0_DIV_SHIFT;
		
		if (g_show_flag)
			printf("CPU_SRCSEL: ARM PLL \n");
		break;
	}


	/*tmp32 = slcr_read(NFC_CLK_CTRL);	*/
	/*nfc_div = (tmp32 & NFC_REF_CLK_DIV_MASK) >> NFC_REF_CLK_DIV_SHIFT;*/
	pcap_div = 2;   /* pcap ip default: 2_div_freq*/
	if (g_show_flag)
		printf("pcap_div: %d \n", pcap_div);

	/*
	// axi_bus_div: 2 or 3
	// ahb_bus_div: 2:
	*/	
	tmp32 = slcr_read(CLK_621_TRUE);	/* axi_bus_div: 2 or 3*/
	if (tmp32 & 0x01)
	{
		ret = ((OSC_HZ * pll_fdiv) / clk0_div) / (3 * 2 * pcap_div);
		if (g_show_flag)
			printf("CLK_621_TRUE: %d (6:2:1) \n", tmp32);
	}
	else
	{
		ret = ((OSC_HZ * pll_fdiv) / clk0_div) / (2 * 2 * pcap_div);
		if (g_show_flag)
			printf("CLK_621_TRUE: %d (4:2:1) \n", tmp32);
	}

	return ret;
}

void clk_pcap_show(void)
{
	g_show_flag = 1;
	printf("clk_pcap: %d Hz \n", clk_pcap_get());
	
	g_show_flag = 0;	
	return;
}

/*
fclk0,1,2,3 --> ps->pl
*/
UINT32 clk_fclk0_get(void)
{
	/*
	DDR_PLL_CTRL	0x130	reserved				31	rw	0x0 Reserved. 鍐欐棤鏁堬紝璇讳负0
							DDR_PLL_FDIV			22:16	rw	d48/d26 "Provide the feedback divisor for the PLL.	(1-63)
	*/
	UINT32 OSC_HZ = HARD_OSC_HZ;  /* 33M Hz */
	
	UINT32 tmp32 = 0, ret = 0;
	UINT32 pll_fdiv = 0;	
	
	UINT32 clk5_div = 0, fpga0_div = 0;
	/*UINT32 clk4_div = 0, fpga1_div = 0;*/
	/*UINT32 clk3_div = 0, fpga2_div = 0;*/
	/*UINT32 clk2_div = 0, fpga3_div = 0;*/

	
	tmp32 = slcr_read(DDR_PLL_CTRL);
	pll_fdiv = (tmp32 & DDR_PLL_FDIV_MASK) >> DDR_PLL_FDIV_SHIFT;
	if (g_show_flag)
		printf("FCLK_SRCSEL: DDR PLL(pll_fdiv-%d) \n", pll_fdiv);
	
	/* DDR_PLL_CLKOUT_5*/
	tmp32 = slcr_read(DDR_PLL_CLKOUT5_DIVISOR);
	clk5_div = (tmp32 & DDR_PLL_CLKOUT5_DIV_MASK) >> DDR_PLL_CLKOUT5_DIV_SHIFT;
	if (g_show_flag)
		printf("clk5_div: %d \n", clk5_div);

	tmp32 = slcr_read(FPGA_CLK_CTRL);	
	fpga0_div = (tmp32 & FPGA0_DIVISOR_MASK) >> FPGA0_DIVISOR_SHIFT;
	if (g_show_flag)
		printf("fpga0_div: %d \n", fpga0_div);
	
	ret = ((OSC_HZ * pll_fdiv) / clk5_div) / (fpga0_div);
	if (g_show_flag)
		printf("fclk_0: %d \n", ret);

	return ret;
}

UINT32 clk_fclk1_get(void)
{
	/*
	DDR_PLL_CTRL	0x130	reserved				31	rw	0x0 Reserved. 鍐欐棤鏁堬紝璇讳负0
							DDR_PLL_FDIV			22:16	rw	d48/d26 "Provide the feedback divisor for the PLL.	(1-63)
	*/
	UINT32 OSC_HZ = HARD_OSC_HZ;  /* 33M Hz */
	
	UINT32 tmp32 = 0, ret = 0;
	UINT32 pll_fdiv = 0;	
	
	/*UINT32 clk5_div = 0, fpga0_div = 0;*/
	UINT32 clk4_div = 0, fpga1_div = 0;
	/*UINT32 clk3_div = 0, fpga2_div = 0;*/
	/*UINT32 clk2_div = 0, fpga3_div = 0;*/

	
	tmp32 = slcr_read(DDR_PLL_CTRL);
	pll_fdiv = (tmp32 & DDR_PLL_FDIV_MASK) >> DDR_PLL_FDIV_SHIFT;
	if (g_show_flag)
		printf("FCLK_SRCSEL: DDR PLL(pll_fdiv-%d) \n", pll_fdiv);
	
	/* DDR_PLL_CLKOUT_4*/
	tmp32 = slcr_read(DDR_PLL_CLKOUT4_DIVISOR);
	clk4_div = (tmp32 & DDR_PLL_CLKOUT4_DIV_MASK) >> DDR_PLL_CLKOUT4_DIV_SHIFT;
	if (g_show_flag)
		printf("clk4_div: %d \n", clk4_div);

	tmp32 = slcr_read(FPGA_CLK_CTRL);	
	fpga1_div = (tmp32 & FPGA1_DIVISOR_MASK) >> FPGA1_DIVISOR_SHIFT;
	if (g_show_flag)
		printf("fpga1_div: %d \n", fpga1_div);
	
	ret = ((OSC_HZ * pll_fdiv) / clk4_div) / (fpga1_div);
	if (g_show_flag)
		printf("fclk_1: %d \n", ret);

	return ret;
}

UINT32 clk_fclk2_get(void)
{
	/*
	DDR_PLL_CTRL	0x130	reserved				31	rw	0x0 Reserved. 鍐欐棤鏁堬紝璇讳负0
							DDR_PLL_FDIV			22:16	rw	d48/d26 "Provide the feedback divisor for the PLL.	(1-63)
	*/
	UINT32 OSC_HZ = HARD_OSC_HZ;  /* 33M Hz */
	
	UINT32 tmp32 = 0, ret = 0;
	UINT32 pll_fdiv = 0;	
	
	/*UINT32 clk5_div = 0, fpga0_div = 0;*/
	/*UINT32 clk4_div = 0, fpga1_div = 0;*/
	UINT32 clk3_div = 0, fpga2_div = 0;
	/*UINT32 clk2_div = 0, fpga3_div = 0;*/

	
	tmp32 = slcr_read(DDR_PLL_CTRL);
	pll_fdiv = (tmp32 & DDR_PLL_FDIV_MASK) >> DDR_PLL_FDIV_SHIFT;
	if (g_show_flag)
		printf("FCLK_SRCSEL: DDR PLL(pll_fdiv-%d) \n", pll_fdiv);
	
	/* DDR_PLL_CLKOUT_3*/
	tmp32 = slcr_read(DDR_PLL_CLKOUT3_DIVISOR);
	clk3_div = (tmp32 & DDR_PLL_CLKOUT3_DIV_MASK) >> DDR_PLL_CLKOUT3_DIV_SHIFT;
	if (g_show_flag)
		printf("clk3_div: %d \n", clk3_div);

	tmp32 = slcr_read(FPGA_CLK_CTRL);	
	fpga2_div = (tmp32 & FPGA2_DIVISOR_MASK) >> FPGA2_DIVISOR_SHIFT;
	if (g_show_flag)
		printf("fpga2_div: %d \n", fpga2_div);
	
	ret = ((OSC_HZ * pll_fdiv) / clk3_div) / (fpga2_div);
	if (g_show_flag)
		printf("fclk_2: %d \n", ret);

	return ret;
}

UINT32 clk_fclk3_get(void)
{
	/*
	DDR_PLL_CTRL	0x130	reserved				31	rw	0x0 Reserved. 鍐欐棤鏁堬紝璇讳负0
							DDR_PLL_FDIV			22:16	rw	d48/d26 "Provide the feedback divisor for the PLL.	(1-63)
	*/
	UINT32 OSC_HZ = HARD_OSC_HZ;  /* 33M Hz */
	
	UINT32 tmp32 = 0, ret = 0;
	UINT32 pll_fdiv = 0;	
	
	/*UINT32 clk5_div = 0, fpga0_div = 0;*/
	/*UINT32 clk4_div = 0, fpga1_div = 0;*/
	/*UINT32 clk3_div = 0, fpga2_div = 0;*/
	UINT32 clk2_div = 0, fpga3_div = 0;

	
	tmp32 = slcr_read(DDR_PLL_CTRL);
	pll_fdiv = (tmp32 & DDR_PLL_FDIV_MASK) >> DDR_PLL_FDIV_SHIFT;
	if (g_show_flag)
		printf("FCLK_SRCSEL: DDR PLL(pll_fdiv-%d) \n", pll_fdiv);
	
	/* DDR_PLL_CLKOUT_2*/
	tmp32 = slcr_read(DDR_PLL_CLKOUT2_DIVISOR);
	clk2_div = (tmp32 & DDR_PLL_CLKOUT2_DIV_MASK) >> DDR_PLL_CLKOUT2_DIV_SHIFT;
	if (g_show_flag)
		printf("clk2_div: %d \n", clk2_div);

	tmp32 = slcr_read(FPGA_CLK_CTRL);	
	fpga3_div = (tmp32 & FPGA3_DIVISOR_MASK) >> FPGA3_DIVISOR_SHIFT;
	if (g_show_flag)
		printf("fpga3_div: %d \n", fpga3_div);
	
	ret = ((OSC_HZ * pll_fdiv) / clk2_div) / (fpga3_div);
	if (g_show_flag)
		printf("fclk_3: %d \n", ret);

	return ret;
}


/*
fclk_0, 1, 2, 3
*/
void clk_fclk_show(void)
{
	UINT32 tmp32 = 0;
	
	g_show_flag = 1;
	
	printf("clk_fclk0: %d Hz \n\n", clk_fclk0_get());
	printf("clk_fclk1: %d Hz \n\n", clk_fclk1_get());
	printf("clk_fclk2: %d Hz \n\n", clk_fclk2_get());
	printf("clk_fclk3: %d Hz \n\n", clk_fclk3_get());
	
	/*
	FPGA3_CLKACT	3		rw	0x1 	"FPGA3鏃堕挓浣胯兘
										0锛氫笉浣胯兘
										1锛氫娇鑳�
	FPGA2_CLKACT	2		rw	0x1 "FPGA2鏃堕挓浣胯兘
										0锛氫笉浣胯兘
										1锛氫娇鑳�
	FPGA1_CLKACT	1		rw	0x1 "FPGA1鏃堕挓浣胯兘
										0锛氫笉浣胯兘
										1锛氫娇鑳�
	FPGA0_CLKACT	0		rw	0x1 "FPGA0鏃堕挓浣胯兘
										0锛氫笉浣胯兘
										1锛氫娇鑳�
	*/
	tmp32 = slcr_read(FPGA_CLK_CTRL);
	if (tmp32 & FPGA0_CLKACT_MASK)
	{
		printf("FPGA0_CLKACT:%d - enable \n", ((tmp32 & FPGA0_CLKACT_MASK) >> 0));
	}
	else
	{
		printf("FPGA0_CLKACT:%d - disable \n", ((tmp32 & FPGA0_CLKACT_MASK) >> 0));
	}
	
	if (tmp32 & FPGA1_CLKACT_MASK)
	{
		printf("FPGA1_CLKACT:%d - enable \n", ((tmp32 & FPGA1_CLKACT_MASK) >> 1));
	}
	else
	{
		printf("FPGA1_CLKACT:%d - disable \n", ((tmp32 & FPGA1_CLKACT_MASK) >> 1));
	}
	
	if (tmp32 & FPGA2_CLKACT_MASK)
	{
		printf("FPGA2_CLKACT:%d - enable \n", ((tmp32 & FPGA2_CLKACT_MASK) >> 2));
	}
	else
	{
		printf("FPGA2_CLKACT:%d - disable \n", ((tmp32 & FPGA2_CLKACT_MASK) >> 2));
	}
	
	if (tmp32 & FPGA3_CLKACT_MASK)
	{
		printf("FPGA3_CLKACT:%d - enable \n", ((tmp32 & FPGA3_CLKACT_MASK) >> 3));
	}
	else
	{
		printf("FPGA3_CLKACT:%d - disable \n", ((tmp32 & FPGA3_CLKACT_MASK) >> 3));
	}
	
	g_show_flag = 0;	
	return;
}

void clk_all_show(void)
{
	clk_ttc_show();
	printf("\n\n");
		
	clk_cpu_show();
	printf("\n\n");
	
	clk_ddr_show();
	printf("\n\n");
	
	clk_qspi_show();
	printf("\n\n");
	
	clk_gtc_show();
	printf("\n\n");
	
	clk_gem0_show();
	printf("\n\n");
	
	clk_gem1_show();
	printf("\n\n");
	
	clk_uart_show();
	printf("\n\n");
		
	clk_spi_show();
	printf("\n\n");
	
	clk_nfc_show();
	printf("\n\n");
	
	clk_sdmmc_show();
	printf("\n\n");
	
	clk_pcap_show();
	printf("\n\n");
	
	clk_fclk_show();
	printf("\n\n");
	
	return;
}

#endif


#if 1

#define ETH_SPEED_1G     (0x03)
#define ETH_SPEED_100M   (0x02)
#define ETH_SPEED_10M    (0x01)

/*
GEM0_CLK_CTRL	0x268		
	GEM0_TX_DIVISOR	14:9	rw	0x32	GMAC0 Tx 鏃堕挓鍒嗛绯绘暟銆�
								(1/10/50)
	GEM0_TX_SRCSEL	8:6	rw	0x0	 GMAC0 Tx鏃堕挓婧愰�鎷╋細
							0x0锛欼O PLL
							0x1锛欰RM PLL
							1xx:   EMIO */
#define GEM0_CLK_CTRL	           (0x268) 
#define GEM0_TX_DIVISOR_MASK       (0x00007E00)  /* 14:9 rw	0x32 GMAC0 Tx 鏃堕挓鍒嗛绯绘暟銆�/10/50 */
#define GEM0_TX_DIVISOR_SHIFT      (9)
#define GEM0_TX_SRCSEL_MASK        (0x000001C0)  /* GEM0_TX_SRCSEL	8:6	rw	0x0	"GMAC0 Tx鏃堕挓婧愰�鎷╋細0x0锛欼O PLL; 0x1锛欰RM PLL; 1xx:   EMIO"; */
#define GEM0_TX_SRCSEL_SHIFT       (6)

/*
GEM1_CLK_CTRL	0x26C		
	GEM1_TX_DIVISOR	14:9	rw	0x32	GMAC0 Tx 鏃堕挓鍒嗛绯绘暟銆�
								(1/10/50)
	GEM1_TX_SRCSEL	8:6	rw	0x0	 GMAC0 Tx鏃堕挓婧愰�鎷╋細
							0x0锛欼O PLL
							0x1锛欰RM PLL
							1xx:   EMIO */
#define GEM1_CLK_CTRL	           (0x26C) 
#define GEM1_TX_DIVISOR_MASK       (0x00007E00)  /* 14:9 rw	0x32 GMAC0 Tx 鏃堕挓鍒嗛绯绘暟銆�/10/50 */
#define GEM1_TX_DIVISOR_SHIFT      (9)
#define GEM1_TX_SRCSEL_MASK        (0x000001C0)  /* GEM1_TX_SRCSEL	8:6	rw	0x0	"GMAC0 Tx鏃堕挓婧愰�鎷╋細0x0锛欼O PLL; 0x1锛欰RM PLL; 1xx:   EMIO"; */
#define GEM1_TX_SRCSEL_SHIFT       (6)

/*extern UINT32 clk_gem0_tx_div_get(int eth_speed);*/
/*extern UINT32 clk_gem1_tx_div_get(int eth_speed);*/

void eth2_clk_gem_set(int mac_x, int speed)
{
	UINT32 tmp32 = 0;
	int clk_parma = 0;
	
	switch (mac_x)
	{
	case 0:  /* gmac_0*/
		switch (speed)
		{
		case ETH_SPEED_1G:
			tmp32 = slcr_read(GEM0_CLK_CTRL);
			tmp32 &= (~GEM0_TX_DIVISOR_MASK);  /* zh*/
			/*tmp32 |= (1 << GEM0_TX_DIVISOR_SHIFT);  // 125M/1=125M for 1000M*/
			clk_parma = clk_gem0_tx_div_get(ETH_SPEED_1G);
			tmp32 |= (clk_parma << GEM0_TX_DIVISOR_SHIFT);
			slcr_write(GEM0_CLK_CTRL, tmp32);
			break;
			
		case ETH_SPEED_100M:
			tmp32 = slcr_read(GEM0_CLK_CTRL);
			tmp32 &= (~GEM0_TX_DIVISOR_MASK);  /* zh*/
			/*tmp32 |= (5 << GEM0_TX_DIVISOR_SHIFT);  // 125M/5=25M for 100M*/
			clk_parma = clk_gem0_tx_div_get(ETH_SPEED_100M);
			tmp32 |= (clk_parma << GEM0_TX_DIVISOR_SHIFT);
			slcr_write(GEM0_CLK_CTRL, tmp32);
			break;
			
		case ETH_SPEED_10M:
			tmp32 = slcr_read(GEM0_CLK_CTRL);
			tmp32 &= (~GEM0_TX_DIVISOR_MASK);
			clk_parma = clk_gem0_tx_div_get(ETH_SPEED_10M);  /* 125M/50=2.5M for 10M*/
			tmp32 |= (clk_parma << GEM0_TX_DIVISOR_SHIFT);
			slcr_write(GEM0_CLK_CTRL, tmp32);
			break;
		}
		break;
		
	case 1: /* gmac_1*/
		switch (speed)
		{
		case ETH_SPEED_1G:
			tmp32 = slcr_read(GEM1_CLK_CTRL);
			tmp32 &= (~GEM1_TX_DIVISOR_MASK);  /* zh*/
			/*tmp32 |= (1 << GEM1_TX_DIVISOR_SHIFT);*/
			clk_parma = clk_gem1_tx_div_get(ETH_SPEED_1G);
			tmp32 |= (clk_parma << GEM1_TX_DIVISOR_SHIFT);
			slcr_write(GEM1_CLK_CTRL, tmp32);
			break;
			
		case ETH_SPEED_100M:
			tmp32 = slcr_read(GEM1_CLK_CTRL);
			tmp32 &= (~GEM1_TX_DIVISOR_MASK);  /* zh*/
			/*tmp32 |= (5 << GEM1_TX_DIVISOR_SHIFT);*/
			clk_parma = clk_gem1_tx_div_get(ETH_SPEED_100M);
			tmp32 |= (clk_parma << GEM1_TX_DIVISOR_SHIFT);
			slcr_write(GEM1_CLK_CTRL, tmp32);
			break;
			
		case ETH_SPEED_10M:
			tmp32 = slcr_read(GEM1_CLK_CTRL);
			tmp32 &= (~GEM1_TX_DIVISOR_MASK);
			clk_parma = clk_gem1_tx_div_get(ETH_SPEED_10M);  /* 125M/50=2.5M for 10M*/
			tmp32 |= (clk_parma << GEM1_TX_DIVISOR_SHIFT);
			slcr_write(GEM1_CLK_CTRL, tmp32);
			break;
		}
		break;
	}
	
	return;
}
#endif



/*
functions for SD/MMC
*/
#ifdef DRV_FM_SDMMC

#define ALT_SYSMGR_SDMMC_CTRL_DRVSEL            (3)
#define ALT_SYSMGR_SDMMC_CTRL_SMPLSEL           (0)

#define ALT_SYSMGR_SDMMC_CTRL_SET(smplsel, drvsel)      \
                        (((drvsel & 0x7) << 0) | ((smplsel & 0x7) << 3))

/*******************************************************************************
*
* sysMshcClkFreqSetup - set up MSHC clock frequency
*
* This routine sets up MSHC clock frequency.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/
#define SDMMC_CLK_FREQ_400KHZ       400000
#define SDMMC_CLK_FREQ_5MHZ          5000000
#define SDMMC_CLK_FREQ_25MHZ        25000000
#define SDMMC_CLK_FREQ_50MHZ        50000000
#define SDMMC_CLK_FREQ_52MHZ        52000000

static int g_sdmmc_ctrl = 0;  /* 0, 1 */
static int g_sdemmc_25M_OR_5M_only = SDEMMC_25M_OR_5M_ONLY;  /* 0 or 1 */

void clk_switch_sdmmc_0(void)
{
	g_sdmmc_ctrl = 0;
	return;
}

void clk_switch_sdmmc_1(void)
{
	g_sdmmc_ctrl = 1;
	return;
}

UINT32 sdmmc_get_baseAddr(int ctrl_x)
{
	UINT32 baseAddr;
	
	if (ctrl_x == 0)
    {
    	baseAddr = FPS_SDMMC0_BASEADDR;  /* pSdmmc->base_address;*/
		/*baseAddr = 0xE0044000;  // for wh-yuhang: ctrl_1-->emmc;*/
	}
	else  /* g_sdmmc_ctrl == 1*/
	{
    	baseAddr = FPS_SDMMC1_BASEADDR;
	}

	return baseAddr;
}


void sysSdmmcSetup_byRate(int rate_in)
{
    unsigned long  clk_div;
    UINT32 temp, rate;
    UINT32 baseAddr, sd_clk, div_cnt;   

	rate = rate_in;
	int ctrl_x = 0;
	
	/*
	if (g_sdmmc_ctrl == 0)
    {
		printf("sdmmc_0 rate: %d \n", rate);
		printf("sdmmc ctrl_0: ");
		
    	baseAddr = FPS_SDMMC0_BASEADDR;  // pSdmmc->base_address;
		//baseAddr = 0xE0044000;  // for wh-yuhang;
	}
	else  // g_sdmmc_ctrl == 1
	{
		printf("sdmmc_1 rate: %d \n", rate);
		printf("sdmmc ctrl_1: ");
		
    	baseAddr = FPS_SDMMC1_BASEADDR;
	}
	*/
	ctrl_x = g_sdmmc_ctrl;
	baseAddr = sdmmc_get_baseAddr(ctrl_x);
	printf("sdmmc_%d rate: %d \n", ctrl_x, rate);
	printf("sdmmc ctrl_%d: ", ctrl_x);
	
    sd_clk = clk_sdmmc_get();

    if (rate >= sd_clk)
    {
        clk_div = 0;
    }
    else
    {
        if(rate != 0)
        {
            /* clk_div = (sd_clk / (2 * rate) + 1); */
    		/* for (div_cnt = 0x1; div_cnt <= SDMMC_MAX_DIV_CNT; div_cnt++) */
    		for (div_cnt = 0x1; div_cnt <= 255; div_cnt++)
            {
    			if ((sd_clk / (2 * div_cnt) <= rate))
                {
    				clk_div = div_cnt;
    				break;
    			}
    		}
        }
    }
    
	printf("clk_div: %d \n", clk_div);
	
	
    /* disable clock */
    FMSH_WriteReg(baseAddr, SDMMC_CLKENA, 0);
    temp = START_CMD + UPDATE_CLOCK_REG_ONLY + WAIT_PRVDATA_COMPLETE;
    FMSH_WriteReg(baseAddr, SDMMC_CMD_VAL, temp);

	taskDelay(1);
	
    /* set clock */
    FMSH_WriteReg(baseAddr, SDMMC_CLKSRC, 0);  /* 00 鈥�Clock divider 0*/
    FMSH_WriteReg(baseAddr, SDMMC_CLKDIV, clk_div);
    temp = START_CMD + UPDATE_CLOCK_REG_ONLY + WAIT_PRVDATA_COMPLETE;
    FMSH_WriteReg(baseAddr, SDMMC_CMD_VAL, temp);

	taskDelay(1);

    /* enable clock */
    FMSH_WriteReg(baseAddr, SDMMC_CLKENA, 0xFFFF);
    temp = START_CMD + UPDATE_CLOCK_REG_ONLY + WAIT_PRVDATA_COMPLETE;
    FMSH_WriteReg(baseAddr, SDMMC_CMD_VAL, temp);
	
 	taskDelay(10); 
	return;
}

void sysSdmmcSetup_byRate_2(int rate_in, int ctrl_x)
{
    unsigned long  clk_div;
    UINT32 temp, rate;
    UINT32 baseAddr, sd_clk, div_cnt;   

	rate = rate_in;
	
	/*
	if (ctrl_x == 0)
    {
		printf("sdmmc_0 rate: %d \n", rate);
		printf("sdmmc ctrl_0: ");
		
    	baseAddr = FPS_SDMMC0_BASEADDR;  // pSdmmc->base_address;    	
		//baseAddr = FM_SDMMC_0_BASE;  // for wh-yuhang;
	}
	else 
	{
		printf("sdmmc_1 rate: %d \n", rate);
		printf("sdmmc ctrl_1: ");
		
    	baseAddr = FPS_SDMMC1_BASEADDR;
		//baseAddr = FM_SDMMC_1_BASE;
	}
	*/
	baseAddr = sdmmc_get_baseAddr(ctrl_x);
	printf("sdmmc_%d rate: %d \n", ctrl_x, rate);
	printf("sdmmc ctrl_%d: ", ctrl_x);
	
    sd_clk = clk_sdmmc_get();

    if (rate >= sd_clk)
    {
        clk_div = 0;
    }
    else
    {
        if (rate != 0)
        {
            /* clk_div = (sd_clk / (2 * rate) + 1); */
    		/* for (div_cnt = 0x1; div_cnt <= SDMMC_MAX_DIV_CNT; div_cnt++) */
    		for (div_cnt=0x1; div_cnt<=255; div_cnt++)
            {
    			if ((sd_clk / (2 * div_cnt) <= rate))
                {
    				clk_div = div_cnt;
    				break;
    			}
    		}
        }
    }
    
	printf("clk_div: %d \n", clk_div);
	
	
    /* disable clock */
    FMSH_WriteReg(baseAddr, SDMMC_CLKENA, 0);
    temp = START_CMD + UPDATE_CLOCK_REG_ONLY + WAIT_PRVDATA_COMPLETE;
    FMSH_WriteReg(baseAddr, SDMMC_CMD_VAL, temp);

	taskDelay(1);
	
    /* set clock */
    FMSH_WriteReg(baseAddr, SDMMC_CLKSRC, 0);  /* 00 鈥�Clock divider 0*/
    FMSH_WriteReg(baseAddr, SDMMC_CLKDIV, clk_div);
    temp = START_CMD + UPDATE_CLOCK_REG_ONLY + WAIT_PRVDATA_COMPLETE;
    FMSH_WriteReg(baseAddr, SDMMC_CMD_VAL, temp);

	taskDelay(1);

    /* enable clock */
    FMSH_WriteReg(baseAddr, SDMMC_CLKENA, 0xFFFF);
    temp = START_CMD + UPDATE_CLOCK_REG_ONLY + WAIT_PRVDATA_COMPLETE;
    FMSH_WriteReg(baseAddr, SDMMC_CMD_VAL, temp);
	
 	taskDelay(10); 
	return;
}

void sysSdmmcSetup_5M
    (
    UINT32 clk
    )
{
    int rate;
    
	switch (clk)
	{
	case SDMMC_CLK_FREQ_400KHZ:
		rate = SDMMC_CLK_FREQ_400KHZ;
		break;
			
#if 0  /* 25M for emmc-64G for ruineng	*/
	case SDMMC_CLK_FREQ_25MHZ:
		rate = SDMMC_CLK_FREQ_25MHZ;
		break;
	
	case SDMMC_CLK_FREQ_50MHZ:
		rate = SDMMC_CLK_FREQ_50MHZ;
		break;
	
	case SDMMC_CLK_FREQ_52MHZ:
		rate = SDMMC_CLK_FREQ_52MHZ;
		break;
#endif

	default:
		rate = SDMMC_CLK_FREQ_5MHZ;
		break;
	}

	sysSdmmcSetup_byRate(rate);
	
	return;
}
	
void sysSdmmcSetup_25M
    (
    UINT32 clk
    )
{
    int rate;
    
	switch (clk)
	{
	case SDMMC_CLK_FREQ_400KHZ:
		rate = SDMMC_CLK_FREQ_400KHZ;
		break;
			
	case SDMMC_CLK_FREQ_25MHZ:
		rate = SDMMC_CLK_FREQ_25MHZ;
		break;
	
#if 0  /* 25M for emmc-64G for ruineng	*/
	case SDMMC_CLK_FREQ_50MHZ:
		rate = SDMMC_CLK_FREQ_50MHZ;
		break;
	
	case SDMMC_CLK_FREQ_52MHZ:
		rate = SDMMC_CLK_FREQ_52MHZ;
		break;
#endif

	default:
		rate = SDMMC_CLK_FREQ_25MHZ;
	  	/*rate = 20000000;           // 20M */
		/*rate = 12500000;           // 12.5M */
		/*rate = 10000000;           // 10M */
		/*rate = 5000000;           // 5M */
		break;
	}
	
	printf("\n");
	printf("sysSdmmcSetup_25M->clk: %d, rate:%d \n", clk, rate);
	printf("sysSdmmcSetup_25M->g_sdmmc_ctrl: %d \n", g_sdmmc_ctrl);

	sysSdmmcSetup_byRate(rate);
	
	return;
}

void sysSdmmcSetup_50M(UINT32 clk)
{
    int rate;
    
	switch (clk)
	{
	case SDMMC_CLK_FREQ_400KHZ:
		rate = SDMMC_CLK_FREQ_400KHZ;
		break;
			
	case SDMMC_CLK_FREQ_25MHZ:
		rate = SDMMC_CLK_FREQ_25MHZ;
		break;
	
	case SDMMC_CLK_FREQ_50MHZ:
		rate = SDMMC_CLK_FREQ_50MHZ;
		break;
	
	case SDMMC_CLK_FREQ_52MHZ:
		rate = SDMMC_CLK_FREQ_52MHZ;
		break;

	default:
		rate = SDMMC_CLK_FREQ_25MHZ;
		break;
	}

	sysSdmmcSetup_byRate(rate);

	return;
}

void sysSdmmcSetup_2(UINT32 clk, int ctrl_x)
{
    int rate;
    
	switch (clk)
	{
	case SDMMC_CLK_FREQ_400KHZ:
		rate = SDMMC_CLK_FREQ_400KHZ;
		break;
			
	case SDMMC_CLK_FREQ_25MHZ:
		rate = SDMMC_CLK_FREQ_25MHZ;
		break;
	
	case SDMMC_CLK_FREQ_50MHZ:
		rate = SDMMC_CLK_FREQ_50MHZ;
		break;
	
	case SDMMC_CLK_FREQ_52MHZ:
		rate = SDMMC_CLK_FREQ_52MHZ;
		break;

	default:
		rate = SDMMC_CLK_FREQ_25MHZ;
		break;
	}

	sysSdmmcSetup_byRate_2(rate, ctrl_x);

	return;
}
void sysMshcClkFreqSetup
    (
    UINT32 clk
    )
{
	g_sdemmc_25M_OR_5M_only = SDEMMC_25M_OR_5M_ONLY;
	
	if (g_sdemmc_25M_OR_5M_only == 25)  /* only for emmc 25M_speed*/
	{
		sysSdmmcSetup_25M(clk);
	}
	else if  (g_sdemmc_25M_OR_5M_only == 5)   /* only for emmc 5M_speed*/
	{
		sysSdmmcSetup_5M(clk); 
	}
	else
	{
		sysSdmmcSetup_50M(clk);	
	}

	return;
}
	
void sysMshcClkFreqSetup_2    (    UINT32 clk, int ctrl_x    )
{
	sysSdmmcSetup_2(clk, ctrl_x);	
	return;
}

UINT32 sysMshcClkFreq_Get(int ctrl_x)
{
    UINT32 baseAddr, sd_clk, clk_div;
	
	/*
	if (ctrl_x == 0)
    {
    	baseAddr = FPS_SDMMC0_BASEADDR;  // pSdmmc->base_address;
	}
	else
	{
    	baseAddr = FPS_SDMMC1_BASEADDR;
	}
	*/
	baseAddr = sdmmc_get_baseAddr(ctrl_x);
	
    sd_clk = clk_sdmmc_get();
	
	/*
	7:0 clk_divider0 0 		Clock divider-0 value. 	
		Clock division is 2*n. 
		For example, value of 0 means divide by 2*0 = 0 (no division, bypass), 
		             value of 1 means divide by 2*1 = 2,
		             value of 鈥渇f鈥�means divide by 2*255 = 510, and so on.
	*/
    clk_div = FMSH_ReadReg(baseAddr, SDMMC_CLKDIV) & 0x0F;

	return (sd_clk / (1 << clk_div));
}

#endif


#if 1

/***********************************************************************************************************************
 *
 *  Function:       slcr_ResetCAN
 *
 *  Return value:   None
 *
 *  Parameters:     None
 *
 *  Description:    Resets the CAN interconnect
 *
 **********************************************************************************************************************/
void slcr_ResetCAN (void)
{
	slcr_write(CAN_RST_CTRL, 0x03);
	taskDelay(1);
	slcr_write(CAN_RST_CTRL, 0x00);

	return;
}

/***********************************************************************************************************************
 *
 *  Function:       slcr_DisableCAN
 *
 *  Return value:   None
 *
 *  Parameters:     None
 *
 *  Description:    Disable the CAN interconnect
 *
 **********************************************************************************************************************/
void slcr_DisableCAN (void)
{
	slcr_write(CAN_CLK_CTRL, 0x00);	
	return;
}

/***********************************************************************************************************************
 *
 *  Function:       slcr_EnableCAN
 *
 *  Return value:   Error Status
 *
 *  Parameters:     None
 *
 *  Description:    Activate the CAN interconnect
 *
 **********************************************************************************************************************/
int slcr_EnableCAN (void)
{
	slcr_write(CAN_CLK_CTRL, 0x03);	
    return OK;
}



#endif


