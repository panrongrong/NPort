#ifndef _MARVELL_88E1116R_H_
#define _MARVELL_88E1116R_H_
/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file  marvell_88e1116r.h
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
#define _88E1116R_TIME_OUT			FMSH_ENET_PHY_TIMEOUT
#define _88E1116R_LINK_TIME_OUT			2000

/* PHY Reg map */
#define PHY_88E1116R_CR 			0	/* Control Register */
#define PHY_88E1116R_STAT 			1	/* Status Register */
#define PHY_88E1116R_ID1 			2	/* PHY ID */
#define PHY_88E1116R_ID2 			3	/* PHY ID */
#define PHY_88E1116R_ACA			4	/* Autonegotiation Copper Advertisement Register */
#define PHY_88E1116R_ACLPA			5	/* Autonegotion Copper Link Partner Ability Register- Base Page */
#define PHY_88E1116R_ACE			6	/* Autonegotiation Copper Expansion Register */
#define PHY_88E1116R_ACNPT			7	/* Autonegotiation Copper Next Page Transmit Register */
#define PHY_88E1116R_ACLPNP			8	/* Autonegotiation Copper Link Partner Next Page Register */
#define PHY_88E1116R_GCTRL			9	/* 1000BASE-T Control Register */
#define PHY_88E1116R_GSTAT			10	/* 1000BASE-T Status Register */
#define PHY_88E1116R_ESTAT			15	/* Extended Status Register */

#define PHY_88E1116R_CSC1			16	/* Copper Specific Control Register 1 , p0 */
#define PHY_88E1116R_MACSC1			16	/* MAC Specific Control Register 1 , p2 */
#define PHY_88E1116R_LEDFC			16	/* LED[2:0] Function Control Register , p3 */
#define PHY_88E1116R_AVCT0			16	/* Advanced VCT TX to MDI[0] Rx Coupling , p5 */
#define PHY_88E1116R_PGLLP			16	/* Packet Generation/Line loopback , p6 */

#define PHY_88E1116R_CSS1			17	/* Copper Specific Status Register 1 , p0 */
#define PHY_88E1116R_LEDPC			17	/* LED[2:0] Polarity Control Register , p3 */
#define PHY_88E1116R_AVCT1			17	/* Advanced VCT TX to MDI[1] Rx Coupling , p5 */
#define PHY_88E1116R_CRCC			17	/* CRC checker , p6 */

#define PHY_88E1116R_CSIE			18	/* Copper Specific Interrupt Enable Register ,p0 */
#define PHY_88E1116R_MACSIE			18	/* MAC Specific Interrupt Enable Register , p2 */
#define PHY_88E1116R_LEDTC			18	/* LED Timer Control Register , p3 */
#define PHY_88E1116R_AVCT2			18	/* Advanced VCT TX to MDI[2] Rx Coupling , p5 */

#define PHY_88E1116R_CSS2			19	/* Copper Specific Status Register 2 , p0 */
#define PHY_88E1116R_MACSS2			19	/* MAC Specific Status Register 2 , p2 */
#define PHY_88E1116R_AVCT3			19	/* Advanced VCT TX to MDI[3] Rx Coupling , p5 */

#define PHY_88E1116R_CSS3			20	/* Copper Specific Status Register 3 , p0 */
#define PHY_88E1116R_GPS			20	/* 1000 BASE-T Pair Skew Register , p5 */

#define PHY_88E1116R_REC			21	/* Receive Error Counter , p0 */
#define PHY_88E1116R_MACSC			21	/* MAC Specific Control Register , p2 */
#define PHY_88E1116R_GPSP			21	/* 1000BASE-T Pair Swap and Polarity , p5 */

#define PHY_88E1116R_PA		        22	/* Page Address */

#define PHY_88E1116R_GIS			23	/* Global Interrupt Status , p0 */
/*#define PHY_88E1116R_GIS			23	/* Global Interrupt Status , p2 */
#define PHY_88E1116R_AVCTC			23	/* Advance VCT Control , p5*/

#define PHY_88E1116R_RGMIIOIO		24	/* RGMII Output Impedance Override , p2 */
#define PHY_88E1116R_AVCTSPD		24	/* Advanced VCT Sample Point Distance , p5 */

#define PHY_88E1116R_RGMIIOIT		25	/* RGMII Output Impedance Target , p2 */
#define PHY_88E1116R_AVCTCPT		25	/* Advanced VCT Cross Pair Threshold , p5 */

#define PHY_88E1116R_CSCR2			26	/* Copper Specific Control Register 2 , p0 */
#define PHY_88E1116R_AVCTSPIT01		26	/* Advanced VCT Same Pair Impedance Threshold 0 and 1 , p5 */
#define PHY_88E1116R_MT 			26	/* Misc Test , p6 */

#define PHY_88E1116R_AVCTSPIT23		27	/* Advanced VCT Same Pair Impedance Threshold 2 and 3 */

#define PHY_88E1116R_AVCTSPITTPC	28	/* Advanced VCT Same Pair Impedance Threshold 4 and
                                            Transmit Pulse Control */

/****************************************************************************
*         Control Register - Copper | Page Any, Register 0
*****************************************************************************/
#define PHY_88E1116R_CR_RST			    (0x1u<<15)			/* PHY software reset:1 for reset */
#define PHY_88E1116R_CR_LOOPBK			(0x1u<<14)			/* loopback TXD to RXD */
#define PHY_88E1116R_CR_SPEED_L		    (0x1u<<13)			/* speed select LSB [bit6,bit13]*/
/* 10=1000; 01=100; 00=10 */
#define PHY_88E1116R_CR_AN_EN			(0x1u<<12)			/* auto-negotiation enable */
#define PHY_88E1116R_CR_POWER_DOWN		(0x1u<<11)			/* power down */
#define PHY_88E1116R_CR_RST_CP_AN		(0x1u<<9)			/* Restart Copper Auto-Negotiation */
#define PHY_88E1116R_CR_CP_DPLX_MOD	    (0x1u<<8)			/* Copper Duplex Mode. 1=full; 0=half */
#define PHY_88E1116R_CR_SPEED_H		    (0x1u<<6)			/* speed select hsb */
/****************************************************************************
*         Status Register - Copper Page | Any, Register 1
*****************************************************************************/
#define PHY_88E1116R_STAT_ANF			(0x1u<<5)		/* AN finish:1 for finish */
#define PHY_88E1116R_STAT_LNK_STAT		(0x1u<<2)		/* Copper link status, 1 up, 0 down */
/****************************************************************************
*         Copper Specific Control Register 1 | Page 0, Register 16
*****************************************************************************/
#define PHY_88E1116R_CSC1_MDIX          (0x3u<<5)       /* MDI Crossover Mode ,
                                                        00 MDI, 01 MDIX, 11 AUTO */
/****************************************************************************
*         PHY Identifier 2 : PHY Reg 3
*****************************************************************************/
#define PHY_88E1116R_ID2_MOD_MUN			(0x210)			/* Model Number */
#define PHY_88E1116R_ID2_MOD_MUN_MASK	(0x3Fu<<4)		/* [9:4] */
/****************************************************************************
*         Copper Specific Status Register 1 | Page 0, Register 17
*****************************************************************************/
#define PHY_88E1116R_CSS1_SPEED			(0x3u<<14)		/* speed:10 for 1000;01 for 100;00 for 10 */
#define PHY_88E1116R_CSS1_DUPLEX		(0x1u<<13)		/* duplex:1 for full;0 for half */
#define PHY_88E1116R_CSS1_PGRCV			(0x1u<<12)		/* Page received */
#define PHY_88E1116R_CSS1_SD_RSOVD		(0x1u<<11)		/* Speed and Duplex Resolved:1 for finish */
#define PHY_88E1116R_CSS1_LINK			(0x1u<<10)		/* real time link:1 for link up */
#define PHY_88E1116R_CSS1_TPE			(0x1u<<9)		/* Transmit Pause Enabled */
#define PHY_88E1116R_CSS1_RPE			(0x1u<<8)		/* Receive Pause Enabled */
#define PHY_88E1116R_CSS1_MDI_CS		(0x1u<<6)		/* MDI Crossover Status ,1 = MDIX, 0 = MDI */
/****************************************************************************
*         Control Register - MAC | Page 2, Register 21
*****************************************************************************/
#define PHY_88E1116R_MACSC_LINE_LPBK        (0x1u<<14)      /* line loopback */
#define PHY_88E1116R_MACSC_RGMII_RTC		(0x1u<<5)		/* RGMII Rcv Timing Ctrl */
#define PHY_88E1116R_MACSC_RGMII_TTC		(0x1u<<4)		/* RGMII Trams Timing Ctrl */
/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/
u8 mvl88e1116r_init(FGmacPs_Instance_T * pGmac);
u8 mvl88e1116r_reset(FGmacPs_Instance_T * pGmac);
u8 mvl88e1116r_cfg(FGmacPs_Instance_T * pGmac);
u8 mvl88e1116r_reg_dump(FGmacPs_Instance_T * pGmac);
u32 mvl88e1116r_reg_read(FGmacPs_Instance_T * pGmac,u8 page,u8 regAddr);
u32 mvl88e1116r_reg_write(FGmacPs_Instance_T * pGmac,u8 page,u8 regAddr,u32 regdata);
void mvl88e1116r_timing_ctrl(FGmacPs_Instance_T *pGmac,u8 TxAddDelay,u8 RxAddDelay);
u32 mvl88e1116r_pcs_lpbk(FGmacPs_Instance_T *pGmac, u8 enable, u8 speed);
u32 mvl88e1116r_line_lpbk(FGmacPs_Instance_T *pGmac, u8 enable);
/************************** Variable Definitions *****************************/
#endif
