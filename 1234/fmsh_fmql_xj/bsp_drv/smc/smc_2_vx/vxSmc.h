/* vxSmc.h - vxSmc driver header */

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

#ifndef __INC_VX_SMC_H__
#define __INC_VX_SMC_H__



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define  VX_SMC_CFG_BASE             (0xE0041000)
#define  VX_SMC_NORSRAM0_BASE        (0xE2000000)
#define  VX_SMC_NORSRAM1_BASE        (0xE4000000)

#define  SMC_CTRL_0   (0)

/*
smc ctrl reg
*/
/* define SMC register's offset*/
/*  x = 0~7*/
#define SMC_REG_SCONR				(0x0000)
#define SMC_REG_STMG0R				(0x0004)
#define SMC_REG_STMG1R				(0x0008)
#define SMC_REG_SCTLR				(0x000C)
#define SMC_REG_SREFR				(0x0010)
#define SMC_REG_EXN_MODE_REG		(0x00AC)

#define SMC_REG_SCRLRx_LOW(x)		(0x0014 + 0x0004 * x)
#define SMC_REG_SMSKRx(x)			(0x0054 + 0x0004 * x)

#define SMC_REG_CSALIAS0_LOW		(0x0074)
#define SMC_REG_CSALIAS1_LOW		(0x0078)
#define SMC_REG_CSREMAP0_LOW		(0x0084)
#define SMC_REG_CSREMAP1_LOW		(0x0088)

/* NOR-FLASH CTRL */
#define SMC_REG_SMTMGR_SET0			(0x0094)
#define SMC_REG_SMTMGR_SET1			(0x0098)
#define SMC_REG_SMTMGR_SET2			(0x009C)
#define SMC_REG_FLASH_TRPDR			(0x00A0)
#define SMC_REG_SMCTLR				(0x00A4)
/***/

/*
SMSKRx registers
*/
#define SMC_REG_SMSKRx_REG_SELECT		(0x7 << 8)
#define SMC_REG_SMSKRx_MEM_TYPE			(0x7 << 5)
#define SMC_REG_SMSKRx_MEM_SIZE			(0x1F << 0)

/* 
SMTMGR_SETx registers
*/
#define SMC_REG_SMSETx_SM_READ_PIPE		        (0x3 << 28)
#define SMC_REG_SMSETx_LOW_FREQ_SYNC_DEVICE		(0x1 << 27)
#define SMC_REG_SMSETx_READY_MODE		        (0x1 << 26)
#define SMC_REG_SMSETx_PAGE_SIZE		        (0x3 << 24)
#define SMC_REG_SMSETx_PAGE_MODE		        (0x1 << 23)
#define SMC_REG_SMSETx_T_PRC		            (0xF << 19)
#define SMC_REG_SMSETx_T_BTA		            (0x7 << 16)
#define SMC_REG_SMSETx_T_WP		                (0x3F << 10)
#define SMC_REG_SMSETx_T_WR		                (0x3 << 8)
#define SMC_REG_SMSETx_T_AS		                (0x3 << 6)
#define SMC_REG_SMSETx_T_RC		                (0x3F << 0)

/*
FLASH registers
*/
#define SMC_REG_FLASH_T_RPD		(0xFFF << 0)

/*
SMCTLR registers
*/
#define SMC_REG_SMCTLR_SM_DATA_WIDTH_SET2	(0x7 << 13)
#define SMC_REG_SMCTLR_SM_DATA_WIDTH_SET1	(0x7 << 10)
#define SMC_REG_SMCTLR_SM_DATA_WIDTH_SET0	(0x7 << 7)
#define SMC_REG_SMCTLR_WP_N					(0x7 << 1)
#define SMC_REG_SMCTLR_SM_RP_N				(0x1 << 0)

/*
smc dev mode
*/
#define SMC_DEV_SRAM            (0)
#define SMC_DEV_NORFLASH        (1)

/*
smc ctrl
*/
typedef struct _t_smc_ctrl_
{
	int  ctrl_x;  /* 0, 1 */

	UINT16 devId;				/**< Unique ID of device */
	UINT32 cfgBaseAddr;		    /**< APB Base address of the device */
	UINT32 sysClk;				/**< Input clock frequency */
	
    int bus_width;
	UINT32 sram_nor_baseAddr;

	UINT32 devIP_ver;
    UINT32 devIP_type;      /* identify peripheral types:  */

	int status;
} vxT_SMC_CTRL;


typedef struct _t_smc_
{
	int smc_x;
	
	vxT_SMC_CTRL * pSmcCtrl;
	
} vxT_SMC;



extern void smcCtrl_Set_TimingReg(vxT_SMC* pSmc, UINT8 index, UINT32 data);
extern UINT32 smcCtrl_Get_TimingReg(vxT_SMC* pSmc, UINT8 index);
extern void smcCtrl_Set_MaskReg(vxT_SMC* pSmc, UINT8 index, UINT32 data);
extern UINT32 smcCtrl_Get_MaskReg(vxT_SMC* pSmc, UINT8 index);
extern void smcCtrl_Set_CtrlReg(vxT_SMC* pSmc, UINT32 data);
extern UINT32 smcCtrl_Get_CtrlReg(vxT_SMC* pSmc);
extern int vxInit_Smc(int dev_mode, int bus_width, int chip_sel);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_VX_SMC_H__ */

