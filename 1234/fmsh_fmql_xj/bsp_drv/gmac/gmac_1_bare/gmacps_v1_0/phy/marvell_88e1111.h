#ifndef _MARVELL_88E1111_H_
#define _MARVELL_88E1111_H_
/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file  marvell_88e1111.h
*
*
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 0.01   Hansen Yang  12/24/2018  First Release
*</pre>
******************************************************************************/
/***************************** Include Files *********************************/
#include "../../../../common/fmsh_common.h"
#include "../fmsh_gmac_lib.h"

#include "fmsh_mdio.h"
/************************** Constant Definitions *****************************/
/* model ID */
#define _88E1111_ID  0xC0

#define _88E1111_TIME_OUT				FMSH_ENET_PHY_TIMEOUT
#define _88E1111_LINK_TIME_OUT			FMSH_ENET_PHY_LINK_TIMEOUT

/* interface */
#if (GMAC_INTERFACE==0)
#define PHY_PATH				PHY_ESST_HWCFG_GMII_C
#elif (GMAC_INTERFACE==1)
#define PHY_PATH				PHY_ESST_HWCFG_RGMII_C
#endif

/* PHY Reg map */
#define PHY_88E1111_CTRL 			0	/* Control Register */
#define PHY_88E1111_STAT 			1	/* Status Register */
#define PHY_88E1111_ID1 			2	/* PHY ID */
#define PHY_88E1111_ID2 			3	/* PHY ID */
#define PHY_88E1111_ANA			4	/* Auto-Neg Advertisement Register */
#define PHY_88E1111_LPA			5	/* Link Partner Ability Register */
#define PHY_88E1111_ANE			6	/* Auto-Neg Expansion Register */
#define PHY_88E1111_NPTX			7	/* Next Page Transmit Register */
#define PHY_88E1111_LPNP			8	/* ink Partner Next Page Register */
#define PHY_88E1111_1000CT			9	/* 1000BASE-T Control Register */
#define PHY_88E1111_1000ST			10	/* 1000BASE-T Status Register */
#define PHY_88E1111_ES				15	/* Extended Status Register */
#define PHY_88E1111_SCT			16	/* PHY Specific Control Register */
#define PHY_88E1111_SST			17	/* PHY Specific Status Register */
#define PHY_88E1111_IEN			18	/* Interrupt Enable Register */
#define PHY_88E1111_IST			19	/* Interrupt Status Register */
#define PHY_88E1111_ESCT			20	/* Extended PHY Specific Control Register */
#define PHY_88E1111_RECNT			21	/* Receive Error Counter Register , page 0 */
#define PHY_88E1111_CR			    21	/* control reg mac , page 2, 1116 */
#define PHY_88E1111_PAGE_ADDR		22	/* Page Address */
#define PHY_88E1111_GS				23	/* Global Status Register */
#define PHY_88E1111_LCT			24	/* LED Control Register */
#define PHY_88E1111_MLO			25	/* Manual LED Override Register */
#define PHY_88E1111_ESCT2			26	/* Extended PHY Specific Control 2 Register */
#define PHY_88E1111_ESST			27	/* Extended PHY Specific Status Register */
#define PHY_88E1111_28				28	/* MDI[0:3] Virtual Cable TesterTM Status (Pages 0-3);  */
/* 1000BASE-T Pair Skew(Page 4);  */
/* 100BASE-T Pair, 1000BASE-T Pair Swap and Polarity (Page 5) */
#define PHY_88E1111_EA_VAL			29	/* Extended Address */
#define PHY_88E1111_30				30	/* Calibration Override (Page 3); Force Gigabit (Page7);  */
/* Class A (Page 11);CRC Checker result (Page 12);  */
/* Test Enable Control (Page 16); Miscellaneous Control (Page 18) */
/****************************************************************************
*         PHY Reg 0 : Control Register
*****************************************************************************/
#define PHY_88E1111_CTRL_RST			(0x1u<<15)			/* PHY software reset:1 for reset */
#define PHY_88E1111_CTRL_LOOPBK			(0x1u<<14)			/* loopback TXD to RXD */
#define PHY_88E1111_CTRL_SPEED_L		(0x1u<<13)			/* speed select lsb [bit6,bit13]*/
/* 10=1000; 01=100; 00=10 */
#define PHY_88E1111_CTRL_AN_EN			(0x1u<<12)			/* auto-negotiation enable */
#define PHY_88E1111_CTRL_POWER_DOWN		(0x1u<<11)			/* power down */
#define PHY_88E1111_CTRL_RST_CP_AN		(0x1u<<9)			/* Restart Copper Auto-Negotiation */
#define PHY_88E1111_CTRL_CP_DPLX_MOD	(0x1u<<8)			/* Copper Duplex Mode. 1=full; 0=half */
#define PHY_88E1111_CTRL_SPEED_H		(0x1u<<6)			/* speed select hsb */
/****************************************************************************
*         PHY Reg 1 : Status Register
*****************************************************************************/
#define PHY_88E1111_STAT_ANF			(0x1u<<5)		/* AN finish:1 for finish */
#define PHY_88E1111_STAT_LNK_STAT		(0x1u<<2)		/* Copper link status */
/****************************************************************************
*         PHY Reg 3 : PHY Identifier
*****************************************************************************/
#define PHY_88E1111_ID2_MOD_MUN			(0xC0)			/* Model Number */
#define PHY_88E1111_ID2_MOD_MUN_MASK	(0x3Fu<<4)		/* [9:4] */
/****************************************************************************
*         Copper Specific Control Register 1 | Page 0, Register 16
*****************************************************************************/
#define PHY_88E1111_SCT_MDIX          (0x3u<<5)       /* MDI Crossover Mode ,
                                                        00 MDI, 01 MDIX, 11 AUTO */
/****************************************************************************
*         PHY Reg 17 : PHY Specific Status Register
*****************************************************************************/
#define PHY_88E1111_SST_SPEED			(0x3u<<14)		/* speed:10 for 1000;01 for 100;00 for 10 */
#define PHY_88E1111_SST_DUPLEX			(0x1u<<13)		/* duplex:1 for full;0 for half */
#define PHY_88E1111_SST_PG_RX			(0x1u<<12)		/* Page received */
#define PHY_88E1111_SST_SD_RSOVD		(0x1u<<11)		/* Speed and Duplex Resolved:1 for finish */
#define PHY_88E1111_SST_LINK			(0x1u<<10)		/* real time link:1 for link up */
#define PHY_88E1111_SST_CBL				(0x3u<<7)		/* cable length */
/****************************************************************************
*         PHY Reg 20 : Extended PHY Specific Control Register
*****************************************************************************/
#define PHY_88E1111_ESCR_RGMII_RTC		(0x1u<<7)		/* RGMII Rcv Timing Ctrl */
#define PHY_88E1111_ESCR_RGMII_TTC		(0x1u<<1)		/* RGMII Trams Timing Ctrl */
/****************************************************************************
*         PHY Reg 21, page 2 : Control Reg 1116
*****************************************************************************/
#define PHY_88E1111_CR_LINE_LPBK        (0x1u<<14)      /* line loopback */
#define PHY_88E1111_CR_RGMII_RTC		(0x1u<<5)		/* RGMII Rcv Timing Ctrl */
#define PHY_88E1111_CR_RGMII_TTC		(0x1u<<4)		/* RGMII Trams Timing Ctrl */
/****************************************************************************
*         PHY Reg 27 : Extended PHY Specific Status Register
*****************************************************************************/
#define PHY_88E1111_ESST_FCRES			(0x1u<<13)		/* Fiber/Copper resolution:1 for fiber;0 for copper */

#define PHY_88E1111_ESST_HWCFG_MODE		 		(0xFu)			/*  */
#define PHY_88E1111_ESST_HWCFG_SGMII_C_CLK		(0x0u)			/* SGMII with Clock with SGMII Auto-Neg to copper */
#define PHY_88E1111_ESST_HWCFG_SGMII_C_NCLK 	(0x4u)			/* SGMII without Clock with SGMII Auto-Neg to copper */
#define PHY_88E1111_ESST_HWCFG_GBIC		 		(0x8u)			/* 1000BASE-X without Clock with 1000BASE-X Auto-Neg to copper (GBIC) */
#define PHY_88E1111_ESST_HWCFG_GBIC_NCLK	 	(0xCu)			/* 1000BASE-X without Clock without 1000BASE-X Auto-Neg to copper */
#define PHY_88E1111_ESST_HWCFG_RTBI_C			(0x9u)			/* RTBI to Copper */
#define PHY_88E1111_ESST_HWCFG_TBI_C			(0xDu)			/* TBI to copper */
#define PHY_88E1111_ESST_HWCFG_NONE1			(0x2u)			/* Reserved */
#define PHY_88E1111_ESST_HWCFG_RGMII_SGMII		(0x6u)			/* RGMII to SGMII */
#define PHY_88E1111_ESST_HWCFG_NONE2			(0xAu)			/* Reserved */
#define PHY_88E1111_ESST_HWCFG_GMII_SGMII		(0xEu)			/* GMII to SGMII */
#define PHY_88E1111_ESST_HWCFG_RGMII_F			(0x3u)			/* RGMII to Fiber */
#define PHY_88E1111_ESST_HWCFG_GMII_F			(0x7u)			/* GMII to Fiber */
#define PHY_88E1111_ESST_HWCFG_RGMII_C			(0xBu)			/* RGMII/Modified MII to Copper */
#define PHY_88E1111_ESST_HWCFG_GMII_C			(0xFu)			/* GMII to copper */
/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/
#define PHY_TRACE_OUT(flag, ...)      \
do {                           \
    if(flag) {                     \
        printf(__VA_ARGS__);   \
    }                          \
} while(0)
/************************** Function Prototypes ******************************/
u32 mvl88e1111_reg_write(FGmacPs_Instance_T * pGmac,u8 page,u8 regAddr,u32 regdata);
u32 mvl88e1111_reg_read(FGmacPs_Instance_T * pGmac,u8 page,u8 regAddr);
u8 mvl88e1111_detect(FGmacPs_Instance_T * pGmac);
u8 mvl88e1111_init(FGmacPs_Instance_T * pGmac);
u8 mvl88e1111_reg_dump(FGmacPs_Instance_T * pGmac);
void mvl88e1111_MDIautoX(FGmacPs_Instance_T * pGmac, u8 mode);
u8 mvl88e1111_restart_AN(FGmacPs_Instance_T * pGmac);
u8 mvl88e1111_reset(FGmacPs_Instance_T * pGmac);
u8 mvl88e1111_cfg(FGmacPs_Instance_T * pGmac);
void mvl88e1111_timing_ctrl(FGmacPs_Instance_T *pGmac,u8 TxAddDelay,u8 RxAddDelay);
/************************** Variable Definitions *****************************/
#endif
