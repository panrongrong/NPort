/* sysSlcrMgr.h - FMSH SLCR Driver header file */

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
25Jul19,jc  Created.
*/

#ifndef __INCsysSlcrMgrh
#define __INCsysSlcrMgrh

#include "../common/fmsh_common_types.h"


/*
CLK_621_TRUE  0x20C
	bit0: CPU/AXI/AHB时钟比例621
			0： 默认4:2:1 （二分频）
			1：6:2:1 （三分频）
*/
#define CLK_621_TRUE	           (0x20C)

/*
DDR_PLL_CTRL	0x130
		DDR_PLL_FDIV	22:16	rw	0x1A	"Provide the feedback divisor for the PLL.  (1-63)
											Note: Before changing this value, the PLL must first 
											be bypassed and then put into reset mode" */
#define DDR_PLL_CTRL	              (0x130)
#define DDR_PLL_FDIV_MASK	          (0x007F0000)  /* bit22~16: ARM_PLL_FDIV	倍频系数 */
#define DDR_PLL_FDIV_SHIFT	          (16)

/*
DDR_CLK_CTRL	0x218	
	DDR_CLKACT	0	rw	0x1	"DDR 1x/4x CLK使能
						0：不使能
						1：使能"
*/
#define DDR_CLK_CTRL	              (0x218)
#define DDR_CLKACT_4X_MASK	              (0x00000001)
#define DDR_CLKACT_4X_SHIFT	          (0)


/* clk0 --> cpu
DDR_PLL_CLKOUT0_DIVISOR	0x134	
	reserved	31:7	rw	0x0	Reserved. 写无效，读为0
	DDR_PLL_CLKOUT0_DIVISOR	6:0	rw	0x2f	DDR PLL CLKOUT0 分频系数(1-126)
*/
#define DDR_PLL_CLKOUT0_DIVISOR	       (0x134)
#define DDR_PLL_CLKOUT0_DIV_MASK	   (0x0000007F)
#define DDR_PLL_CLKOUT0_DIV_SHIFT	   (0)

/* clk1 --> ddr
DDR_PLL_CLKOUT1_DIVISOR	0x138	
	reserved	31:7	rw	0x0	Reserved. 写无效，读为0
	DDR_PLL_CLKOUT1_DIVISOR	6:0	rw	0x2f	DDR PLL CLKOUT1 分频系数(1-126)
*/
#define DDR_PLL_CLKOUT1_DIVISOR	       (0x138)
#define DDR_PLL_CLKOUT1_DIV_MASK	   (0x0000007F)
#define DDR_PLL_CLKOUT1_DIV_SHIFT	   (0)

/* clk2 --> fpga_1
DDR_PLL_CLKOUT2_DIVISOR	0x13C	
	reserved	31:7	rw	0x0	Reserved. 写无效，读为0
	DDR_PLL_CLKOUT2_DIVISOR	6:0	rw	0x2f	DDR PLL CLKOUT2 分频系数(1-126)
*/
#define DDR_PLL_CLKOUT2_DIVISOR	       (0x13C)
#define DDR_PLL_CLKOUT2_DIV_MASK	   (0x0000007F)
#define DDR_PLL_CLKOUT2_DIV_SHIFT	   (0)

/* clk3 --> fpga_2
DDR_PLL_CLKOUT3_DIVISOR	0x140	
	reserved	31:7	rw	0x0	Reserved. 写无效，读为0
	DDR_PLL_CLKOUT3_DIVISOR	6:0	rw	0x2f	DDR PLL CLKOUT3 分频系数(1-126)
*/
#define DDR_PLL_CLKOUT3_DIVISOR	       (0x140)
#define DDR_PLL_CLKOUT3_DIV_MASK	   (0x0000007F)
#define DDR_PLL_CLKOUT3_DIV_SHIFT	   (0)

/* clk4 --> fpga_3
DDR_PLL_CLKOUT4_DIVISOR	0x144	
	reserved	31:7	rw	0x0	Reserved. 写无效，读为0
	DDR_PLL_CLKOUT4_DIVISOR	6:0	rw	0x2f	DDR PLL CLKOUT4 分频系数(1-126)
*/
#define DDR_PLL_CLKOUT4_DIVISOR	       (0x144)
#define DDR_PLL_CLKOUT4_DIV_MASK	   (0x0000007F)
#define DDR_PLL_CLKOUT4_DIV_SHIFT	   (0)

/* clk5 --> fpga_4
DDR_PLL_CLKOUT5_DIVISOR	0x148	
	reserved	31:7	rw	0x0	Reserved. 写无效，读为0
	DDR_PLL_CLKOUT5_DIVISOR	6:0	rw	0x2f	DDR PLL CLKOUT5 分频系数(1-126)
*/
#define DDR_PLL_CLKOUT5_DIVISOR	       (0x148)
#define DDR_PLL_CLKOUT5_DIV_MASK	   (0x0000007F)
#define DDR_PLL_CLKOUT5_DIV_SHIFT	   (0)


/*
ARM_PLL_CTRL	0x100
*/
#define ARM_PLL_CTRL	              (0x100)
#define ARM_PLL_FDIV_MASK	          (0x007F0000)  /* bit22~16: ARM_PLL_FDIV	倍频系数 */
#define ARM_PLL_FDIV_SHIFT	          (16)

/*
ARM_CLK_CTRL	0x210
	CPU_SRCSEL	1:0	rw	0x0	"CPU时钟源选择。 (persistant bits)
				0x：ARM PLL
				10：DDR PLL
				11：IO PLL" */
#define ARM_CLK_CTRL	              (0x210)
#define ARM_CPU_SRCSEL_MASK	          (0x00000003)
#define ARM_CPU_SRCSEL_SHIFT	      (0)

	
/* clk0 --> clk_cpu ...
ARM_PLL_CLKOUT0_DIVISOR 0x104	reserved	31:7	rw	0x0 Reserved. 写无效，读为0
			ARM_PLL_CLKOUT0_DIVISOR 6:0 rw	0x2F	ARM PLL CLKOUT0 分频系数(1-126)
*/	
#define ARM_PLL_CLKOUT0_DIVISOR	       (0x104)
#define ARM_PLL_CLKOUT0_DIV_MASK	   (0x0000007F)
#define ARM_PLL_CLKOUT0_DIV_SHIFT	   (0)

/* clk1 --> gmax_tx	
ARM_PLL_CLKOUT1_DIVISOR 0x108	reserved	31:7	rw	0x0 Reserved. 写无效，读为0
			ARM_PLL_CLKOUT1_DIVISOR 6:0 rw	0x2F	ARM PLL CLKOUT1 分频系数(1-126)
*/	
#define ARM_PLL_CLKOUT1_DIVISOR	       (0x108)
#define ARM_PLL_CLKOUT1_DIV_MASK	   (0x0000007F)
#define ARM_PLL_CLKOUT1_DIV_SHIFT	   (0)

/*	clk2 --> sdmmc	
ARM_PLL_CLKOUT2_DIVISOR 0x10C	reserved	31:7	rw	0x0 Reserved. 写无效，读为0
			ARM_PLL_CLKOUT2_DIVISOR 6:0 rw	0x2F	ARM PLL CLKOUT2 分频系数(1-126)
*/	
#define ARM_PLL_CLKOUT2_DIVISOR	       (0x10C)
#define ARM_PLL_CLKOUT2_DIV_MASK	   (0x0000007F)
#define ARM_PLL_CLKOUT2_DIV_SHIFT	   (0)

/* clk3 --> spi	
ARM_PLL_CLKOUT3_DIVISOR 0x120	reserved	31:7	rw	0x0 Reserved. 写无效，读为0
			ARM_PLL_CLKOUT3_DIVISOR 6:0 rw	0x2F	ARM PLL CLKOUT3 分频系数(1-126)
*/	
#define ARM_PLL_CLKOUT3_DIVISOR	       (0x120)
#define ARM_PLL_CLKOUT3_DIV_MASK	   (0x0000007F)
#define ARM_PLL_CLKOUT3_DIV_SHIFT	   (0)

/* clk4 --> qspi		
ARM_PLL_CLKOUT4_DIVISOR 0x124	reserved	31:7	rw	0x0 Reserved. 写无效，读为0
			ARM_PLL_CLKOUT4_DIVISOR 6:0 rw	0x2F	ARM PLL CLKOUT4 分频系数(1-126)
*/	
#define ARM_PLL_CLKOUT4_DIVISOR	       (0x124)
#define ARM_PLL_CLKOUT4_DIV_MASK	   (0x0000007F)
#define ARM_PLL_CLKOUT4_DIV_SHIFT	   (0)

/* clk5 --> uart		
ARM_PLL_CLKOUT5_DIVISOR 0x128	reserved	31:7	rw	0x0 Reserved. 写无效，读为0
			ARM_PLL_CLKOUT5_DIVISOR 6:0 rw	0x2F	ARM PLL CLKOUT5 分频系数(1-126)
*/	
#define ARM_PLL_CLKOUT5_DIVISOR	       (0x128)
#define ARM_PLL_CLKOUT5_DIV_MASK	   (0x0000007F)
#define ARM_PLL_CLKOUT5_DIV_SHIFT	   (0)

/*
IO_PLL_CTRL	0x150	
	reserved	31	rw	0x0	Reserved. 写无效，读为0
	IO_PLL_PREDIV	30:24	rw	0x1	"Provide the ref clk pre-divisor for the PLL. (1-63, 只需开放1)
									Note: Before changing this value, the PLL must first be bypassed and then put into reset mode"
	reserved	23	rw	0x0	Reserved. 写无效，读为0
	IO_PLL_FDIV	22:16	rw	0x1A	"Provide the feedback divisor for the PLL.  (1-63)
									Note: Before changing this value, the PLL must first be bypassed and then put into reset mode"
	reserved	15:4	rw	0x0	Reserved. 写无效，读为0
	IO_PLL_BYPASS_FORCE	3	rw	0x0	"IO PLL Bypass控制：
						IO_PLL_BYPASS_QUAL=0时：
							0：不bypass
							1：bypass
						IO_PLL_BYPASS_QUAL=1时：
							0：PLL是否bypass依赖于BOOT_MODE[4] pin值
							1：bypass"
	IO_PLL_BYPASS_QUAL	2	rw	0x1	"IO PLL Bypass源选择：
						0：由IO_PLL_BYPASS_FORCE位决定
						1：由BOOT_MODE[4] pin值决定"
	IO_PLL_PWRDWN	1	rw	0x0	"IO PLL下电控制：
						0：正常工作不下电
						1：IO PLL下电
	IO_PLL_RESET	0	rw	0x0	"IO PLL复位控制：
						0：撤销（PLL正常工作）
						1：PLL复位
*/
#define IO_PLL_CTRL	               (0x150)
#define IO_PLL_FDIV_MASK	       (0x007F0000)  /* bit22~16: IO_PLL_FDIV	倍频系数 */
#define IO_PLL_FDIV_SHIFT	       (16)

	
/* 
IO_PLL_CLKOUT0_DIVISOR	0x154	
		reserved	31:7	rw	0x0	Reserved. 写无效，读为0
		IO_PLL_CLKOUT0_DIVISOR	6:0	rw	0x2F	IO PLL CLKOUT0 分频系数(1-126)*/
#define IO_PLL_CLKOUT0_DIVISOR	    (0x154)
#define IO_PLL_CLKOUT0_DIV_MASK     (0x0000007F)  
#define IO_PLL_CLKOUT0_DIV_SHIFT    (0)

/* IO_PLL_CLKOUT1_DIVISOR	0x158	
				reserved	31:7 rw 0x0  Reserved. 写无效，读为0
	IO_PLL_CLKOUT1_DIVISOR	6:0  rw 0x2F IO PLL CLKOUT1 分频系数(1-126) */
#define IO_PLL_CLKOUT1_DIVISOR	    (0x158)
#define IO_PLL_CLKOUT1_DIV_MASK     (0x0000007F)  
#define IO_PLL_CLKOUT1_DIV_SHIFT    (0)

/*
IO_PLL_CLKOUT2_DIVISOR	0x15C	
		reserved	31:7	rw	0x0	Reserved. 写无效，读为0
		IO_PLL_CLKOUT2_DIVISOR	6:0	rw	0x2F	IO PLL CLKOUT2 分频系数(1-126) */
#define IO_PLL_CLKOUT2_DIVISOR	    (0x15C)
#define IO_PLL_CLKOUT2_DIV_MASK     (0x0000007F)  
#define IO_PLL_CLKOUT2_DIV_SHIFT    (0)

/*
IO_PLL_CLKOUT3_DIVISOR	0x160	
		reserved	31:7	rw	0x0	Reserved. 写无效，读为0
		IO_PLL_CLKOUT3_DIVISOR	6:0	rw	0x2F	IO PLL CLKOUT3 分频系数(1-126) */
#define IO_PLL_CLKOUT3_DIVISOR	    (0x160)
#define IO_PLL_CLKOUT3_DIV_MASK     (0x0000007F)  
#define IO_PLL_CLKOUT3_DIV_SHIFT    (0)

/*
IO_PLL_CLKOUT4_DIVISOR	0x164	reserved	31:7	rw	0x0	Reserved. 写无效，读为0
		IO_PLL_CLKOUT4_DIVISOR	6:0	rw	0x2F	IO PLL CLKOUT4 分频系数(1-126) */
#define IO_PLL_CLKOUT4_DIVISOR	    (0x164)
#define IO_PLL_CLKOUT4_DIV_MASK     (0x0000007F)  
#define IO_PLL_CLKOUT4_DIV_SHIFT    (0)

/* clk5 --> uart
IO_PLL_CLKOUT5_DIVISOR	0x168	
		reserved	31:7	rw	0x0	Reserved. 写无效，读为0
		IO_PLL_CLKOUT5_DIVISOR	6:0	rw	0x2F	IO PLL CLKOUT5 分频系数(1-126) */	
#define IO_PLL_CLKOUT5_DIVISOR	   (0x168)
#define IO_PLL_CLKOUT5_DIV_MASK	   (0x0000007F)
#define IO_PLL_CLKOUT5_DIV_SHIFT   (0)


/*
GTIMER_CLK_CTRL	0x22C	
			reserved	31:6	rw	0x0	Reserved. 写无效，读为0
		GTIMER_DIVISOR	5:0		rx	0x2	GTIMER时钟分频系数(1-63)
*/
#define GTIMER_CLK_CTRL	           (0x22C)
#define GTIMER_CLK_CTR_MASKL	   (0x3F)
#define GTIMER_CLK_CTR_SHIFT	   (0)

/*
GEM0_CLK_CTRL	0x268		
	GEM0_TX_DIVISOR	14:9	rw	0x32	GMAC0 Tx 时钟分频系数。
								(1/10/50)
	GEM0_TX_SRCSEL	8:6	rw	0x0	 GMAC0 Tx时钟源选择：
							0x0：IO PLL
							0x1：ARM PLL
							1xx:   EMIO */
#define GEM0_CLK_CTRL	           (0x268) 
#define GEM0_TX_DIVISOR_MASK       (0x00007E00)  /* 14:9 rw	0x32 GMAC0 Tx 时钟分频系数。1/10/50 */
#define GEM0_TX_DIVISOR_SHIFT      (9)
#define GEM0_TX_SRCSEL_MASK        (0x000001C0)  /* GEM0_TX_SRCSEL	8:6	rw	0x0	"GMAC0 Tx时钟源选择：0x0：IO PLL; 0x1：ARM PLL; 1xx:   EMIO"; */
#define GEM0_TX_SRCSEL_SHIFT       (6)

/*
GEM1_CLK_CTRL	0x26C		
	GEM1_TX_DIVISOR	14:9	rw	0x32	GMAC0 Tx 时钟分频系数。
								(1/10/50)
	GEM1_TX_SRCSEL	8:6	rw	0x0	 GMAC0 Tx时钟源选择：
							0x0：IO PLL
							0x1：ARM PLL
							1xx:   EMIO */
#define GEM1_CLK_CTRL	           (0x26C) 
#define GEM1_TX_DIVISOR_MASK       (0x00007E00)  /* 14:9 rw	0x32 GMAC0 Tx 时钟分频系数。1/10/50 */
#define GEM1_TX_DIVISOR_SHIFT      (9)
#define GEM1_TX_SRCSEL_MASK        (0x000001C0)  /* GEM1_TX_SRCSEL	8:6	rw	0x0	"GMAC0 Tx时钟源选择：0x0：IO PLL; 0x1：ARM PLL; 1xx:   EMIO"; */
#define GEM1_TX_SRCSEL_SHIFT       (6)



/*
UART_CLK_CTRL:
	UART_SRCSEL	bit4	rw	0x0	"UART REF时钟选择
					0：IO PLL
					1：ARM PLL" */
#define UART_CLK_CTRL	            (0x294)
#define UART_SRCSEL_MASK	        (0x10)
#define UART_SRCSEL_SHIFT	        (4)

/*
SPI_CLK_CTRL	0x29C
	SPI_SRCSEL	4	rw	0x0	"SPI REF时钟选择
					0：IO PLL
					1：ARM PLL"*/
#define SPI_CLK_CTRL	            (0x29C)
#define SPI_SRCSEL_MASK	            (0x10)
#define SPI_SRCSEL_SHIFT	        (4)

/*
"QSPI_CLK_CTRL(qspi0/1共享时钟复位)"	0x284
	QSPI_SRCSEL	4	rw	0x0	"QSPI REF时钟选择
					0：IO PLL
					1：ARM PLL" */
#define QSPI_CLK_CTRL	            (0x284)
#define QSPI_SRCSEL_MASK	        (0x10)
#define QSPI_SRCSEL_SHIFT	        (4)

/*
SDIO_CLK_CTRL	0x28C
	SDIO_SRCSEL	4	rw	0x0	"SDIO REF时钟选择
					0：IO PLL
					1：ARM PLL */
#define SDIO_CLK_CTRL	            (0x28C)
#define SDIO_SRCSEL_MASK	        (0x10)
#define SDIO_SRCSEL_SHIFT	        (4)

/*
SPI_CLK_CTRL	0x29C	reserved	
--------------------------------
	                 31:5	rw	0x0	Reserved. 写无效，读为0
		SPI_SRCSEL	    4	rw	0x0	"SPI REF时钟选择
			0：IO PLL			
			1：ARM PLL"
		SPI1_APB_CLKACT	3	rw	0x1	"SPI1 APB时钟使能
			0：不使能
			1：使能"
		SPI0_APB_CLKACT	2	rw	0x1	"SPI0 APB时钟使能
			0：不使能
			1：使能"
		SPI1_CLKACT	    1	rw	0x1	"SPI1 REF时钟使能
			0：不使能
			1：使能"
		SPI0_CLKACT	    0	rw	0x1	"SPI0 REF时钟使能
			0：不使能
			1：使能"
*/
#define SPI_CLK_CTRL	            (0x29C)

/*
NFC_CLK_CTRL	0x27C	
		reserved			31:10	rw	0x0	Reserved. 写无效，读为0
		NFC_REF_CLK_DIVISOR	9:4	rw	0x1	NFC REF CLK分频系数。
		reserved			3:2	rw	0x0	Reserved. 写无效，读为0
		NFC_AHB_CLKACT		1	rw	0x1	"NFC AHB时钟域使能
				0：不使能
				1：使能"
		NFC_REF_CLKACT		0	rw	0x1	"NFC ECC时钟域使能
				0：不使能
				1：使能"
*/
#define NFC_CLK_CTRL	            (0x27C)
#define    NFC_REF_CLK_DIV_MASK     (0x03F0)
#define    NFC_REF_CLK_DIV_SHIFT    (4)


/*
CAN_RST_CTRL	0x308	reserved	
						31:2	rw	0x0 Reserved. 写无效，读为0
		CAN1_APB_RST	1		rw	0x0  "CAN1 APB时钟域复位
			0：不复位
			1：复位"
		CAN0_APB_RST	0		rw	0x0  "CAN0 APB时钟域复位
			0：不复位
			1：复位"
*/
#define CAN_RST_CTRL	            (0x308)

/*
CAN_CLK_CTRL	0x304	reserved	31:2	rw	0x0 Reserved. 写无效，读为0
		CAN1_APB_CLKACT 1	rw	0x1 "CAN1 APB时钟域使能
			0：不使能
			1：使能"
		CAN0_APB_CLKACT 0	rw	0x1 "CAN0 APB时钟域使能
			0：不使能
			1：使能"
*/
#define CAN_CLK_CTRL	            (0x304)



/*
DDR_PLL_CTRL	0x130	reserved				31	rw	0x0	Reserved. 写无效，读为0
						DDR_PLL_PREDIV			30:24	rw	0x1	"Provide the ref clk pre-divisor for the PLL. (1-63, 只需开放1)
															Note: Before changing this value, the PLL must first be bypassed and then put into reset mode"
						reserved				23	rw	0x0	Reserved. 写无效，读为0
						DDR_PLL_FDIV			22:16	rw	d48/d26	"Provide the feedback divisor for the PLL.  (1-63)
															Note: Before changing this value, the PLL must first be bypassed and then put into reset mode"
						reserved				15:4	rw	0x0	Reserved. 写无效，读为0
						DDR_PLL_BYPASS_FORCE	3		rw	0x0	"DDR PLL Bypass控制：
															DDR_PLL_BYPASS_QUAL=0时：
																0：不bypass
																1：bypass
															DDR_PLL_BYPASS_QUAL=1时：
																0：PLL是否bypass依赖于BOOT_MODE[4] pin值
																1：bypass"
						DDR_PLL_BYPASS_QUAL		2	rw	0x1	"DDR PLL Bypass源选择：
															0：由DDR_PLL_BYPASS_FORCE位决定
															1：由BOOT_MODE[4] pin值决定"
						DDR_PLL_PWRDWN			1	rw	0x0	"DDR PLL下电控制：
															0：正常工作不下电
															1：DDR PLL下电
															注：下电再上电。等待PLL lock后，再正常使用IP。"
						DDR_PLL_RESET			0	rw	0x0	"DDR PLL复位控制：
															0：撤销（PLL正常工作）
															1：PLL复位
----------------------------------------------------------------------------------------------------------------------															
DDR_PLL_CLKOUT2_DIVISOR	0x13C	reserved				31:7	rw	0x0	Reserved. 写无效，读为0
								DDR_PLL_CLKOUT2_DIVISOR	6:0		rw	d4/d24	DDR PLL CLKOUT2 分频系数(1-126) ->  fclk_3
								
DDR_PLL_CLKOUT3_DIVISOR	0x140	reserved				31:7	rw	0x0	Reserved. 写无效，读为0
								DDR_PLL_CLKOUT3_DIVISOR	6:0		rw	d4/d24	DDR PLL CLKOUT3 分频系数(1-126) ->  fclk_2
								
DDR_PLL_CLKOUT4_DIVISOR	0x144	reserved				31:7	rw	0x0	Reserved. 写无效，读为0
								DDR_PLL_CLKOUT4_DIVISOR	6:0		rw	d4/d24	DDR PLL CLKOUT4 分频系数(1-126) ->  fclk_1
								
DDR_PLL_CLKOUT5_DIVISOR	0x148	reserved				31:7	rw	0x0	Reserved. 写无效，读为0
								DDR_PLL_CLKOUT5_DIVISOR	6:0		rw	d4/d24	DDR PLL CLKOUT5 分频系数(1-126) ->  fclk_0
----------------------------------------------------------------------------------------------------------------------															
FPGA_CLK_CTRL	0x230	reserved		31:28	rw	0x0	Reserved. 写无效，读为0
						FPGA3_DIVISOR	27:22	rw	0x3F	FPGA3时钟分频。(1-63)
						FPGA2_DIVISOR	21:16	rw	0x3F	FPGA2时钟分频。(1-63)
						FPGA1_DIVISOR	15:10	rw	0x3F	FPGA1时钟分频。(1-63)
						FPGA0_DIVISOR	9:4		rw	0x3F	FPGA0时钟分频。(1-63)
						FPGA3_CLKACT	3		rw	0x1		"FPGA3时钟使能
															0：不使能
															1：使能"
						FPGA2_CLKACT	2		rw	0x1	"FPGA2时钟使能
															0：不使能
															1：使能"
						FPGA1_CLKACT	1		rw	0x1	"FPGA1时钟使能
															0：不使能
															1：使能"
						FPGA0_CLKACT	0		rw	0x1	"FPGA0时钟使能
															0：不使能
															1：使能"
*/
#define  FPGA_CLK_CTRL  (0x230)

	#define  FPGA3_DIVISOR_MASK      (0x0FC00000)   	/* 27:22	rw	0x3F	FPGA3时钟分频。(1-63)*/
	#define  FPGA2_DIVISOR_MASK	     (0x003F0000)       /* 21:16	rw	0x3F	FPGA2时钟分频。(1-63)*/
	#define  FPGA1_DIVISOR_MASK	     (0x0000FC00)	    /* 15:10	rw	0x3F	FPGA1时钟分频。(1-63)*/
	#define  FPGA0_DIVISOR_MASK	     (0x000003F0)	    /* 9:4 	    rw	0x3F	FPGA0时钟分频。(1-63)*/
	
	#define  FPGA3_DIVISOR_SHIFT 	 (22)		
	#define  FPGA2_DIVISOR_SHIFT 	 (16)		
	#define  FPGA1_DIVISOR_SHIFT 	 (10)		
	#define  FPGA0_DIVISOR_SHIFT	 (4)	
	
	#define  FPGA3_CLKACT_MASK      (0x08)		/*rw	0x1 "FPGA3时钟使能*/
	#define  FPGA2_CLKACT_MASK		(0x04)		/*rw	0x1 "FPGA2时钟使能*/
	#define  FPGA1_CLKACT_MASK		(0x02)		/*rw	0x1 "FPGA1时钟使能*/
	#define  FPGA0_CLKACT_MASK		(0x01)		/*rw	0x1 "FPGA0时钟使能*/



/* jc */
#define FMQL_READ_32(reg)               (*(volatile UINT32 *)(reg))
#define FMQL_WRITE_32(reg, data)        (*((volatile UINT32 *)(reg)) = (data))


extern void fmqlSlcrWrite(UINT32 offset, UINT32 value);
extern UINT32 fmqlSlcrRead(UINT32 offset);
extern void slcr_write(UINT32 offset, UINT32 value);
extern UINT32 slcr_read(UINT32 offset);

extern void FSlcrPs_ipSetRst(u32 rst_id, u32 rst_mode);
extern void FSlcrPs_ipReleaseRst(u32 rst_id, u32 rst_mode);

#endif /* __INCsysSlcrMgrh */

