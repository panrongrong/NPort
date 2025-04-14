/* vxWdt.h - vxWdt driver header */

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

#ifndef __INC_VX_WDT_H__
#define __INC_VX_WDT_H__



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*#define FPS_WDT_BASEADDR             (0xE0025000)*/
#define  VX_WDT_CFG_BASE               (0xE0025000) 


/*
gtc ctrl reg
-------------
volatile u32 cr 		  ;   // control register               (0x00) 		
volatile u32 torr		  ;   // timeout range register         (0x04)		  
volatile u32 ccvr		  ;   // current counter value register (0x08)				 
volatile u32 ccr		  ;   // counter restart register       (0x0c) 		 
volatile u32 stat		  ;   // interrupt status register      (0x10)					  
volatile u32 eio		  ;   // interrupt clear register       (0x14) 					
volatile u32 comp_version ;   // component vision  register     (0x18)									  
*/
#define  WDT_REG_CR             0x00
#define  WDT_REG_TORR           0x04
#define  WDT_REG_CCVR           0x08
#define  WDT_REG_CRR            0x0C

#define  WDT_REG_STAT           0x10
#define  WDT_REG_EIO            0x14
#define  WDT_REG_COMP_VER       0x18

#define  WDT_REG_COMP_TYPE      0xFC

#define WDT_ENABLE      (1)
#define WDT_DISABLE     (0)

/*
1	RMOD	Response mode
			0 - 只产生reset信号（default）
			1 - 第一次计数完成产生中断信号，如果在第二次计数完成以前中断没有被清除，则产生reset信号
*/
#define RSP_ONLY_RST     (0)
#define RSP_2IRQ_RST     (1)


/* WDT_CR register  */
#define bfoWDT_EN               0   
#define bfwWDT_EN               1

#define bfoRMOD                 1  
#define bfwRMOD                 1  

#define bfoRPL                  2  
#define bfwRPL                  3 

/* WDT_TOR register  */
#define bfoTOP               0   
#define bfwTOP               4
#define bfoTOP_INIT          4  
#define bfwTOP_INIT          4  

#define  RPL_2_PCLK     (0x0)
#define  RPL_4_PCLK     (0x1)
#define  RPL_8_PCLK     (0x2)
#define  RPL_16_PCLK    (0x3)
#define  RPL_32_PCLK    (0x4)
#define  RPL_64_PCLK    (0x5)
#define  RPL_128_PCLK   (0x6)
#define  RPL_256_PCLK   (0x7)


/*
GTC ctrl
*/
typedef struct _t_wdt_ctrl_
{
	int  ctrl_x;
	
	UINT16 devId;				/**< Unique ID of device */
	UINT32 cfgBaseAddr;		    /**< APB Base address of the device */
	UINT32 sysClk;				/**< Input clock frequency */
	

	UINT32 devIP_ver;
    UINT32 devIP_type;      /* identify peripheral types:  */

	int status;
} vxT_WDT_CTRL;


typedef struct _t_wdt_
{
	int wdt_x;
	
	vxT_WDT_CTRL * pWdtCtrl;
	
} vxT_WDT;
	

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_VX_WDT_H__ */


