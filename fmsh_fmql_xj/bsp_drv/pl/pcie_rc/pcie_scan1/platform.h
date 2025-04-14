/* $Id:platform.h v1.1 2007-7-17*/

/******************************************************************************
 *
 *  FILE: platform.h
 *
 *  MODULE: platform
 *
 *  PURPOSE:
 *
 *  AUTHOR(S):xi
 *
 *  GROUP:OS system Department(631).
 *
 *  DATE CREATED:2007/07/17
 *
 *  REFERENCE DOCUMENT ID:
 *
 *  MODIFICATIONS:
 *  Date          user Name       Description
 *  2007/07/17    whwei            Create this file
 *
 *********************************************************************************/
#ifndef __PLATFORM_PLATFORM_H__
#define __PLATFORM_PLATFORM_H__

#ifndef ASM
/*	#include <arch.h>*/

	#ifdef __cplusplus
	extern "C" {
	#endif
#else
   #include <archInt.h>
#endif


	/* LPC Controller Address Space */
	#define LPC_BASE		0x60000000	/* 128MB */
	/*--------------------------------------------------------------*/
	/*
	 * Address Space Definitions
	 */

	/* PCIe Config/IO/MEM Address Space  */
#if 0 /* ref_ft*/
	#define PCIE_CONFIG_BASE	0x40000000
	#define PCIE_CONFIG_SIZE	0x04000000	/* 64MB */
	#define PCIE_IO_BASE		0x44000000
	#define PCIE_IO_SIZE		0x04000000	/* 64MB */
	#define PCIE_MEM32_BASE		0x48000000
	#define PCIE_MEM32_SIZE		0x18000000	/* 384MB */
	#define PCIE_MEM64_BASE		0x100000000
	#define PCIE_MEM64_SIZE		0x100000000	/* 4GB */
	
#elif 1  // pl_pcie_rc: for fmql7045-zhq: 5K_6K

#define PCIE_CONFIG_BASE	0x50000000
#define PCIE_CONFIG_SIZE	0x04000000	/* 64MB */

#define PCIE_IO_BASE		0x00000000
#define PCIE_IO_SIZE		0x00000000	/* 0MB */

#define PCIE_MEM32_BASE 	0x60000000
#define PCIE_MEM32_SIZE 	0x10000000	/* 256MB */

#define PCIE_MEM64_BASE 	0x000000000
#define PCIE_MEM64_SIZE 	0x000000000 

#elif 0  // pl_pcie_rc: for 5k_8K
	
#define PCIE_CONFIG_BASE	0x50000000
#define PCIE_CONFIG_SIZE	0x10000000	/* 256MB */
	
#define PCIE_IO_BASE		0x00000000
#define PCIE_IO_SIZE		0x00000000	/* 0MB */
	
#define PCIE_MEM32_BASE 	0x80000000
#define PCIE_MEM32_SIZE 	0x10000000	/* 256MB */
	
#define PCIE_MEM64_BASE 	0x000000000
#define PCIE_MEM64_SIZE 	0x000000000 

#endif
	

	/* OnChip IO & Control Address Space */
	#define MIO_BASE		0x70000000	/* 1MB: Uart/i2c/wdt/gpio */
	#define CRU_BASE		0x70100000	/* 1MB */
	#define MCU_BASE		0x70200000	/* 64KB */
	#define LBC_BASE		0x70220000	/* 4KB */
	#define TSGW_BASE		0x70221000	/* 4KB: Time Stamp Generator */
	#define TSGR_BASE		0x70222000	/* 4KB: Time Stamp Generator */
	#define ERROR_BASE		0x70600000	/* 4KB */
	#define DMAC_BASE		0x70601000	/* 4KB */
/*	#define RTC_BASE		0x70602000	/* 4KB */
	#define GMAC0_BASE		0x70C00000	/* 8KB: Gmac0 */
	#define GMAC1_BASE		0x70C10000	/* 8KB: Gmac1 */
	#define GMAC_CONF_BASE	0x70C20000	/* 4KB */

#if 0  /* ref_ft*/
	#define PCIE_BASE		0x71000000	/* 8MB: PCIe Controller */

#else  /* jc: for fmql7046*/
	#define PCIE_BASE		0x50000000	/* 64MB: PCIe Controller */
#endif


	#define SRAM_BASE		0x71020000	/* 128KB */
	#define ICU_BASE		0x71800000	/* 4MB */
	#define RAPIDIO_BASE	0x72000000	/* 4MB */
	#define PMC_BASE		0x72400000	/* 1MB */
	#define OCN_BASE		0x72590000	/* 64KB */
	/* Debug Trace Interface Address Space */
	#define DTI_BASE		0x7C000000	/* 64MB */
	/*--------------------------------------------------------------*/
	/*
	 * Detailed Register Definitions
	 */

	/* MIO Address Space */
	#define UART0_BASE		(MIO_BASE)		/* 9 pin 16550a */
	#define UART1_BASE		(MIO_BASE + 0x1000)	/* 3 pin 16550a */
	#define I2C0_BASE		(MIO_BASE + 0x2000)
	#define I2C1_BASE		(MIO_BASE + 0x3000)
	#define WDT0_BASE		(MIO_BASE + 0x4000)
	#define WDT1_BASE		(MIO_BASE + 0x5000)
	#define GPIO_BASE		(MIO_BASE + 0x6000)
	#define OIC_BASE		(MIO_BASE + 0x7000)

	/* CRU Registers */
	#define CRU_SOFTRST_ALL        (CRU_BASE + 0x0200)
	#define CRU_CORE_RST_SOURCE    (CRU_BASE + 0x0318)
	#define CRU_HARD_WARMRST_CNT   (CRU_BASE + 0x0400)
	#define CRU_SOFT_WARMRST_CNT   (CRU_BASE + 0x0404)
	#define CRU_SOFTRST_CORE_CNT   (CRU_BASE + 0x0408)
	#define CRU_LPR_CNT            (CRU_BASE + 0x040c)
	#define CRU_FER_CNT            (CRU_BASE + 0x0410)
	#define CRU_SOFTRST_LOCAL_CNT  (CRU_BASE + 0x0414)
	#define CRU_WDT_RST_CNT        (CRU_BASE + 0x0418)
	#define RESET_PWRON			   0x0
	#define RESET_HARDWARM		   0x1
	#define RESET_SOFTWARM		   0x2
	#define RESET_COREWARM		   0x3
	#define RESET_LOWPOWER		   0x4
	#define RESET_FATALERROR	   0x5
	#define RESET_LOCAL            0x6
	#define RESET_WDT			   0x7
	#define CRU_CLKSTOP_LP_ENABLE0 (CRU_BASE + 0xb00)
	#define CRU_CLKSTOP_LP_ENABLE1 (CRU_BASE + 0xb04)

/* CRU GPIO function select register */
#define CRU_REG_PAD_SEL_0   (CRU_BASE + 0x0C0C)
#define CRU_REG_PAD_SEL_1   (CRU_BASE + 0x0C10)
#define CRU_REG_PAD_SEL_2   (CRU_BASE + 0x0C14)

/* Time Stamp Generator Registers */
#define TSGW_TSGEN0_CTLR	(TSGW_BASE)
#define TSGR_TSGEN0_READ	(TSGR_BASE)
#define TSGW_COUNTER_CTLR	(TSGW_TSGEN0_CTLR)	/* Counter Control */

/* Error Control Registers */
#define ERROR_TRIGGER_TYPE	    (ERROR_BASE + 0x0)
#define ERROR_FATAL_ENABLE	    (ERROR_BASE + 0x4)	/* Raise Fatal Error Reset */
#define ERROR_FATAL_CLEAR	    (ERROR_BASE + 0x8)
#define ERROR_FATAL_PENDING	    (ERROR_BASE + 0xc)
#define ERROR_FATAL_PENDING_ID	(ERROR_BASE + 0x10)
#define ERROR_ENABLE		    (ERROR_BASE + 0x14)	/* Raise SError Exception */
#define ERROR_CLEAR		        (ERROR_BASE + 0x18)
#define ERROR_PENDING		    (ERROR_BASE + 0x1c)
#define ERROR_PENDING_ID	    (ERROR_BASE + 0x20)
#define ERROR_TARGET		    (ERROR_BASE + 0x24)
#define	ERROR_nINTERRIRQ		0x1		/* L2 ECC Error Event */
#define	ERROR_nEXTERRIRQ		0x2		/* Write Response Error Event */
#define	ERROR_nOCNERRIRQ		0x4		/* OCN Error Event */
#define	ERROR_nMCUERRIRQ		0x8		/* MCU Error Event */

/* PMC Registers */
#define PMC_CLUSTER_STATE	    (PMC_BASE)
#define POWER_STATE_SERVE		0x0
#define POWER_STATE_STANDBY		0x1
#define POWER_STATE_SLEEP		0x2
#define POWER_STATE_TRANSITION	0x3
#define PMC_PSO_CFG		        (PMC_BASE + 0x10)
#define PMC_PSO_CFG_CLR		    (PMC_BASE + 0x14)
#define PMC_PSO_CFG_SET		    (PMC_BASE + 0x18)
#define PMC_IOC_STATE		    (PMC_BASE + 0x30)

#define PMC_PWRDWN_TAG           (PMC_BASE + 0x24)
#define PMC_PWRDWN_TAG_CLR       (PMC_BASE + 0x28)
#define PMC_PWRDWN_TAG_SET       (PMC_BASE + 0x2c)

/* PCIe Control Registers */
	#define FT2000A_PEU_VID         0x16c3
	#define FT2000A_PEU0_DID        0xc0c0
	#define FT2000A_PEU1_DID        0xc1c1
	
	#define PCIE_HPB_BASE		    (PCIE_BASE + 0x200000)
	#define PCIE_C0_BASE		    (PCIE_BASE)
	#define PCIE_C1_BASE		    (PCIE_BASE + 0x1000)
	
	#define PCIE_INTX_ENABLE	    (PCIE_HPB_BASE + 0x4)
	#define PCIE_MSI_ENABLE		    (PCIE_HPB_BASE + 0x200)
	#define PCIE_MSI_BASE32		    (PCIE_HPB_BASE + 0x204)
	#define PCIE_MSI_BASE64_HIGH    (PCIE_HPB_BASE + 0x208)
	#define PCIE_MSI_BASE64_LOW	    (PCIE_HPB_BASE + 0x20C)
    #define PCIE_MSI_CLEAR          (PCIE_HPB_BASE + 0x614)
	#define PCIE_STATUS_LINKUP_MASK	0x10000
	#define PCIE_ACCESS_TYPE	    (PCIE_HPB_BASE + 0x608)
	#define PCIE_ACCESS_PROT_C0	    (PCIE_HPB_BASE + 0x60C)
	#define PCIE_ACCESS_PROT_C1	    (PCIE_HPB_BASE + 0x61C)
	#define PCIE_SPLIT_MODE		    (PCIE_HPB_BASE + 0x800)
	#define PEU_SPLIT_MODE_X8		0x1
	#define PEU_SPLIT_MODE_X4_X4	0x2
	#define PEU_SPLIT_MODE_X4_X1	0x3
	#define PEU_SPLIT_MODE_X4_X2	0x4
    #define PCIE_DEVICE_TYPE        (PCIE_HPB_BASE + 0x804)

    #define PCIE_C1_RC_C0_RC         0x44
    #define PCIE_C1_RC_C0_EP         0x40
    #define PCIE_C1_EP_C0_RC         0x04
    #define PCIE_C1_EP_C0_EP         0x00
	#define PCIE_LANE_REVERSE	     (PCIE_HPB_BASE + 0x808)
	#define PCIE_LINK_NEGOTIATE	     (PCIE_HPB_BASE + 0x80c)
	#define PCIE_HPB_SRAM_HIGH64K_EN (PCIE_HPB_BASE + 0x05f4)
	#define PCIE_C0_LTSSM                  (PCIE_HPB_BASE + 0x540)
	#define PCIE_C0_LTSSM_STATUS_MASK       0x01f80000
	#define PCIE_C0_LTSSM_STATUS           (((*(UINT32*)PCIE_C0_LTSSM) \
	                                        & PCIE_C0_LTSSM_STATUS_MASK   ) >> 19)
	#define PCIE_C0_LTSSM_SMLH_LINK_MASK    0x00020000
	#define PCIE_C0_LTSSM_SMLH_LINK        (((*(UINT32*)PCIE_C0_LTSSM) \
			                                & PCIE_C0_LTSSM_SMLH_LINK_MASK) >> 17)
	#define PCIE_C0_LTSSM_RDLH_LINK_MASK    0x00010000
	#define PCIE_C0_LTSSM_RDLH_LINK        (((*(UINT32*)PCIE_C0_LTSSM) \
			                                & PCIE_C0_LTSSM_RDLH_LINK_MASK) >> 16)

	#define PCIE_C1_LTSSM                  (PCIE_HPB_BASE + 0x544)
	#define PCIE_C1_LTSSM_STATUS_MASK       0x01f80000
	#define PCIE_C1_LTSSM_STATUS           (((*(UINT32*)PCIE_C1_LTSSM) \
			                               & PCIE_C1_LTSSM_STATUS_MASK   ) >> 19)
	#define PCIE_C1_LTSSM_SMLH_LINK_MASK    0x00020000
	#define PCIE_C1_LTSSM_SMLH_LINK        (((*(UINT32*)PCIE_C1_LTSSM) \
			                               & PCIE_C1_LTSSM_SMLH_LINK_MASK) >> 17)
	#define PCIE_C1_LTSSM_RDLH_LINK_MASK    0x00010000
	#define PCIE_C1_LTSSM_RDLH_LINK        (((*(UINT32*)PCIE_C1_LTSSM) \
			                               & PCIE_C1_LTSSM_RDLH_LINK_MASK) >> 16)

	#define PCIE_C0_CLASS_CODE             (PCIE_C0_BASE + 0x8)
	#define PCIE_C1_CLASS_CODE             (PCIE_C1_BASE + 0x8)

	#define PEU_CLASS_CODE                 0x8
	#define PEU_IO_LIMIT_BASE_REG		   0x1c
	#define PEU_PREF_LIMIT_BASE_REG		   0x24
	#define PEU_PL_MISC_CTRL_1_OFF		   0x8bc
	#define PEU_PL_INDEX_OFF		       0x900
	#define PEU_PL_REGION_CTRL_1_OFF	   0x904
	#define PEU_PL_REGION_CTRL_1_TYPE_MEM  0x0
	#define PEU_PL_REGION_CTRL_1_TYPE_IO   0x2
	#define PEU_PL_REGION_CTRL_1_TYPE_CONFIG_TYPE_0	0x4
	#define PEU_PL_REGION_CTRL_1_TYPE_CONFIG_TYPE_1	0x5
	#define PEU_PL_REGION_CTRL_2_OFF	   0x908
	#define PEU_PL_LOWER_BASE_OFF		   0x90c
	#define PEU_PL_UPPER_BASE_OFF		   0x910
	#define PEU_PL_LIMIT_OFF		       0x914
	#define PEU_PL_LOWER_TARGET_OFF		   0x918
	#define PEU_PL_UPPER_TARGET_OFF		   0x91c

	#define PEU_DMA_WRITE_ENGINE_EN 	    0X97C
	#define PEU_DMA_WRITE_DOORBELL			0X980
	#define PEU_DMA_WRITE_INT_MASK 			0X9C4
	#define PEU_DMA_WRITE_DONE_IMWR_LOW 	0X9D0
	#define PEU_DMA_WRITE_DONE_IMWR_HIGH 	0X9D4
	#define PEU_DMA_WRITE_ABORT_IMWR_LOW 	0X9D8
	#define PEU_DMA_WRITE_ABORT_IMWR_HIGH 	0X9DC
	#define PEU_DMA_WRITE_CH01_IMWR_DATA 	0X9E0

	#define PEU_DMA_READ_ENGINE_EN 			0X99C
	#define PEU_DMA_READ_DOORBELL 			0X9A0
	#define PEU_DMA_READ_INT_MASK 			0XA18
	#define PEU_DMA_READ_DONE_IMWR_LOW 		0XA3C
	#define PEU_DMA_READ_DONE_IMWR_HIGH 	0XA40
	#define PEU_DMA_READ_ABORT_IMWR_LOW 	0XA44
	#define PEU_DMA_READ_ABORT_IMWR_HIGH 	0XA48
	#define PEU_DMA_READ_CH01_IMWR_DATA 	0XA4C

	#define PEU_DMA_VIEWPORT_SEL 			0XA6C
	#define PEU_DMA_CH_CONTROL1 			0XA70
	#define PEU_DMA_TRANSFER_SIZE 			0XA78
	#define PEU_DMA_SAR_LOW 				0XA7C
	#define PEU_DMA_SAR_HIGH 				0XA80
	#define PEU_DMA_DAR_LOW 				0XA84
	#define PEU_DMA_DAR_HIGH 				0XA88

	/* GIC Address Space */
	#define GICD_BASE		        (ICU_BASE + 0x1000)
	#define GICC_BASE		        (ICU_BASE + 0x2000)

	#define SIZE_64KB               0x10000
	#define SIZE_60KB               0xf000
	#define SIZE_32KB               0x8000
	#define SIZE_1KB                0x400
	#define EARLY_STACK_TOP         (SRAM_BASE + SIZE_32KB - SIZE_1KB)

	#define SRAM_TO_DIMM_ADDR       (SRAM_BASE + SIZE_64KB)

	/* OCN Snoop Control Register */
	#define OCN_STATUS_REG         (OCN_BASE + 0xC)
	#define CHANGE_PENDING_BIT     (1 << 0)
	#define OCN_SNOOP_CONTROL_REG  (OCN_BASE + 0x5000)
	#define SNOOP_EN_BIT           (1 << 0)

	/* spi registers */
	#define SPI_BASE                (0x1FFFFF00)
	#define SPI_ACCESS_CONTROL      (SPI_BASE + 0x1C)

	/*dmac registers*/
	#define     DMAC_MRn            DMAC_BASE + 0x100
	#define     DMAC_SRn            DMAC_BASE + 0x104
	#define     DMAC_SATRn          DMAC_BASE + 0x110
	#define     DMAC_SARn           DMAC_BASE + 0x114
	#define     DMAC_DATRn          DMAC_BASE + 0x118
	#define     DMAC_DARn           DMAC_BASE + 0x11C
	#define     DMAC_BCRn           DMAC_BASE + 0x120
	#define     DMAC_DGSR           DMAC_BASE + 0x300
	#define     DMAC_CONF           DMAC_BASE + 0x304

	#define     DMAC_1_MRn          DMAC_BASE + 0x180
	#define     DMAC_1_SRn          DMAC_BASE + 0x184
	#define     DMAC_1_SATRn        DMAC_BASE + 0x190
	#define     DMAC_1_SARn         DMAC_BASE + 0x194
	#define     DMAC_1_DATRn        DMAC_BASE + 0x198
	#define     DMAC_1_DARn         DMAC_BASE + 0x19C
	#define     DMAC_1_BCRn         DMAC_BASE + 0x1a0

	#define LBC_BR0_ADDR (LBC_BASE + 0   )
	#define LBC_BR1_ADDR (LBC_BASE + 0x8 )
	#define LBC_BR2_ADDR (LBC_BASE + 0x10)
	#define LBC_BR3_ADDR (LBC_BASE + 0x18)
	#define LBC_BR4_ADDR (LBC_BASE + 0x20)
	#define LBC_BR5_ADDR (LBC_BASE + 0x28)
	#define LBC_BR6_ADDR (LBC_BASE + 0x30)
	#define LBC_BR7_ADDR (LBC_BASE + 0x38)

	/*位宽选择*/
	#define LBC_BRn_PS_OFFSET 11
	#define LBC_BRn_PS_8BIT  (0x1 << LBC_BRn_PS_OFFSET)
	#define LBC_BRn_PS_16BIT (0x2 << LBC_BRn_PS_OFFSET)
	#define LBC_BRn_PS_32BIT (0x3 << LBC_BRn_PS_OFFSET)

	/*读写使能*/
	#define LBC_BRn_WP_OFFSET 8
	#define LBC_BRn_WP_EN    (0x1 << LBC_BRn_WP_OFFSET)

	/*模式选择*/
	#define LBC_BRn_MSEL_OFFSET 5
	#define LBC_BRn_MSEL_GPCM (0x0 << LBC_BRn_MSEL_OFFSET)
	#define LBC_BRn_MSEL_UPMA (0x4 << LBC_BRn_MSEL_OFFSET)
	#define LBC_BRn_MSEL_UPMB (0x5 << LBC_BRn_MSEL_OFFSET)

	/*bank 使能*/
	#define LBC_BRn_V_VILD 1

	#define LBC_OR0_ADDR (LBC_BASE + 0X4 )
	#define LBC_OR1_ADDR (LBC_BASE + 0xC )
	#define LBC_OR2_ADDR (LBC_BASE + 0x14)
	#define LBC_OR3_ADDR (LBC_BASE + 0x1C)
	#define LBC_OR4_ADDR (LBC_BASE + 0x24)
	#define LBC_OR5_ADDR (LBC_BASE + 0x2C)
	#define LBC_OR6_ADDR (LBC_BASE + 0x34)
	#define LBC_OR7_ADDR (LBC_BASE + 0x3C)

	/*bank 大小*/
	/*大小配置要与MMU一致*/
	#define LBC_ORn_AM_1G    0xc0000000
	#define LBC_ORn_AM_512M  0xe0000000
	#define LBC_ORn_AM_256M  0xf0000000
	#define LBC_ORn_AM_128M  0xf8000000
	#define LBC_ORn_AM_64M   0xfc000000
	#define LBC_ORn_AM_32M   0xfe000000
	#define LBC_ORn_AM_16M   0xff000000
	#define LBC_ORn_AM_8M    0xff800000
	#define LBC_ORn_AM_4M    0xffc00000
	#define LBC_ORn_AM_2M    0xffe00000
	#define LBC_ORn_AM_1M    0xfff00000
	#define LBC_ORn_AM_512K  0xfff80000
	#define LBC_ORn_AM_256K  0xfffc0000
	#define LBC_ORn_AM_128K  0xfffe0000
	#define LBC_ORn_AM_64K   0xffff0000
	#define LBC_ORn_AM_32K   0xffff8000
	/*Chip select negation time. */
	#define LBC_ORn_CSNT_OFFSET  11
	#define LBC_ORn_CSNT_EARLIER (1 << LBC_ORn_CSNT_OFFSET)

	/*Address to chip-select setup 片选建立时间*/
	#define LBC_ORn_ASC_OFFSET  9
	#define LBC_ORn_ASC_NO       0x0
	/*#define LBC_ORn_ASC_HALF    (0x1 << LBC_ORn_ASC_OFFSET)*/
	#define LBC_ORn_ASC_QUARTER (0x2 << LBC_ORn_ASC_OFFSET)
	#define LBC_ORn_ASC_HALF    (0x3 << LBC_ORn_ASC_OFFSET)

	/*Extra address to chip-select setup.*/
	#define LBC_ORn_XASC_OFFSET  8
	#define LBC_ORn_XASC_NO       0x0
	#define LBC_ORn_XASC_QUARTER (0x2 << LBC_ORn_ASC_OFFSET)
	#define LBC_ORn_XASC_HALF    (0x3 << LBC_ORn_ASC_OFFSET)

	/*Cycle length in bus clocks.*/
	#define LBC_ORn_SCY_OFFSET  4
	#define LBC_ORn_SCY_0  (0 << LBC_ORn_SCY_OFFSET)
	#define LBC_ORn_SCY_1  (1 << LBC_ORn_SCY_OFFSET)
	#define LBC_ORn_SCY_2  (2 << LBC_ORn_SCY_OFFSET)
	#define LBC_ORn_SCY_3  (3 << LBC_ORn_SCY_OFFSET)
	#define LBC_ORn_SCY_4  (4 << LBC_ORn_SCY_OFFSET)
	#define LBC_ORn_SCY_5  (5 << LBC_ORn_SCY_OFFSET)
	#define LBC_ORn_SCY_6  (6 << LBC_ORn_SCY_OFFSET)
	#define LBC_ORn_SCY_7  (7 << LBC_ORn_SCY_OFFSET)
	#define LBC_ORn_SCY_8  (8 << LBC_ORn_SCY_OFFSET)
	#define LBC_ORn_SCY_9  (9 << LBC_ORn_SCY_OFFSET)
	#define LBC_ORn_SCY_10 (10 << LBC_ORn_SCY_OFFSET)
	#define LBC_ORn_SCY_11 (11 << LBC_ORn_SCY_OFFSET)
	#define LBC_ORn_SCY_12 (12 << LBC_ORn_SCY_OFFSET)
	#define LBC_ORn_SCY_13 (13 << LBC_ORn_SCY_OFFSET)
	#define LBC_ORn_SCY_14 (14 << LBC_ORn_SCY_OFFSET)
	#define LBC_ORn_SCY_15 (15 << LBC_ORn_SCY_OFFSET)

	/*External address termination*/
	#define LBC_ORn_SETA_OFFSET 3
	#define LBC_ORn_SETA_INTER (0 << LBC_ORn_SETA_OFFSET)
	#define LBC_ORn_SETA_EXTER (1 << LBC_ORn_SETA_OFFSET)

	/*Timing relaxed.*/
	#define LBC_ORn_TRLX_OFFSET 2
	#define LBC_ORn_TRLX_NORMAL  (0 << LBC_ORn_TRLX_OFFSET)
	#define LBC_ORn_TRLX_RELAXED (1 << LBC_ORn_TRLX_OFFSET)

	/*Extended hold time on read accesses.*/
	#define LBC_ORn_EHTR_OFFSET 1
	#define LBC_ORn_EHTR_NO (0 << LBC_ORn_EHTR_OFFSET)
	#define LBC_ORn_EHTR_1C (1 << LBC_ORn_EHTR_OFFSET)
	#define LBC_ORn_EHTR_4C (0 << LBC_ORn_EHTR_OFFSET)
	#define LBC_ORn_EHTR_8C (1 << LBC_ORn_EHTR_OFFSET)

	/*External address latch delay.*/
	#define LBC_ORn_EAD_NO  0
	#define LBC_ORn_EAD_ADD 1

	#define LBC_LTESR_ADDR  (LBC_BASE_ADDR + 0xb0)
	#define LBC_LTEDR_ADDR  (LBC_BASE_ADDR + 0xb4)
	#define LBC_LTEATR_ADDR (LBC_BASE_ADDR + 0xbc)
	#define LBC_LTEAR_ADDR  (LBC_BASE_ADDR + 0xc0)
	#define LBC_CREAR_ADDR  (LBC_BASE_ADDR + 0xc4)

	/*局部总线配置寄存器*/
	#define LBC_LBCR_ADDR   (LBC_BASE_ADDR + 0xd0)
	#define LBC_LBCR_BMT_OFFSET 8
	#define LBC_LBCR_BMT (0 << LBC_LBCR_BMT_OFFSET)

	#define LBC_LBCR_UPMA_EN_OFFSET 6
	#define LBC_LBCR_UPMA_EN (1 << LBC_LBCR_UPMA_EN_OFFSET)

	#define LBC_LBCR_UPMB_EN_OFFSET 5
	#define LBC_LBCR_UPMB_EN (1 << LBC_LBCR_UPMB_EN_OFFSET)

	#define LBC_LBCR_BUSY_MM_OFFSET 3
	#define LBC_LBCR_BUSY_MM (1 << LBC_LBCR_UPMB_EN_OFFSET)

	#define LBC_LBCR_PSEX_CONF_OFFSET 2
	#define LBC_LBCR_PSEX_CONF_SIGNAL_EN (1 << LBC_LBCR_PSEX_CONF_OFFSET)

	#define LBC_LBCR_TA_EN_OFFSET 1
	#define LBC_LBCR_TA_EN (1 << LBC_LBCR_TA_EN_OFFSET)

	#define LBC_LBCR_MERGE_EN_OFFSET 0
	#define LBC_LBCR_MERGE_EN (1 << LBC_LBCR_MERGE_EN_OFFSET)

	/*局部总线时钟比率寄存器*/
	#define LBC_LCRR_ADDR   (LBC_BASE_ADDR + 0xd4)
	#define LBC_LCRR_EADC_OFFSET 16
	#define LBC_LCRR_EADC_4 (0 << LBC_LCRR_EADC_OFFSET)
	#define LBC_LCRR_EADC_1 (1 << LBC_LCRR_EADC_OFFSET)
	#define LBC_LCRR_EADC_2 (2 << LBC_LCRR_EADC_OFFSET)
	#define LBC_LCRR_EADC_3 (3 << LBC_LCRR_EADC_OFFSET)

	#define LBC_LCRR_CLKDIV_OFFSET 0
	#define LBC_LCRR_CLKDIV_2 (2 << LBC_LCRR_CLKDIV_OFFSET)
	#define LBC_LCRR_CLKDIV_4 (4 << LBC_LCRR_CLKDIV_OFFSET)
	#define LBC_LCRR_CLKDIV_8 (8 << LBC_LCRR_CLKDIV_OFFSET)

	/*安全访问寄存器*/
	#define LBC_SR_ADDR   (LBC_BASE_ADDR + 0xd8)


	/*interrupt number*/
	#define		MSTIMER_INT_NUM  	30	/*定时器*/
	#define   	COM0_INT_NUM 		36 		/*FT2000A的串口中断号*/
	#define   	COM1_INT_NUM 		37
	#define		GMAC0_INT_NUM       46
	#define		GMAC1_INT_NUM       47
	#define 	MSI_INT_NUM		    51  /*msi中断*/
	#define 	INTD_INT_NUM		52  /*pcie intd中断*/
	#define 	INTC_INT_NUM		53  /*pcie intc中断*/
	#define 	INTB_INT_NUM		54  /*pcie intb中断*/
	#define 	INTA_INT_NUM		55  /*pcie inta中断*/

	#define		EXT0_INT_NUM		56  /*外部中断*/
	#define		EXT1_INT_NUM		57
	#define		EXT2_INT_NUM		58
	#define		EXT3_INT_NUM		59
	#define		EXT4_INT_NUM		60
	#define		EXT5_INT_NUM		61
	#define		EXT6_INT_NUM		62
	#define		EXT7_INT_NUM		63
	#define		EXT8_INT_NUM		64
	#define		EXT9_INT_NUM		65
	#define		EXT10_INT_NUM		66
	#define		EXT11_INT_NUM		67
	#define		EXT12_INT_NUM		68
	#define		EXT13_INT_NUM		69
	#define		EXT14_INT_NUM		70
	#define		EXT15_INT_NUM		71
	
#define EPIC_MAX_EXT_IRQS	12
#define EPIC_MAX_IN_IRQS	48
#define EPIC_MAX_GT_IRQS	4
#define EPIC_MAX_MSG_IRQS	4
#define EPIC_MAX_SMSG_IRQS	8
#define EPIC_MAX_IPI_IRQS	4

#define EPIC_VEC_EXT_IRQ0       0
#define EPIC_VEC_IN_IRQ0        (EPIC_VEC_EXT_IRQ0 + EPIC_MAX_EXT_IRQS)
#define EPIC_VEC_GT_IRQ0        (EPIC_VEC_IN_IRQ0 + EPIC_MAX_IN_IRQS)

#define EXTERN_INT_LAST             1024


	/*********************BOARD  FC switch****************************/
	
	#define PCI_REGION_MEM		0x00000000	/* PCI memory space */
	#define PCI_REGION_IO		0x00000001	/* PCI IO space */
	#define PCI_REGION_TYPE		0x00000001
	#define PCI_REGION_PREFETCH	0x00000008	/* prefetchable PCI memory */

	#define PCI_REGION_SYS_MEMORY	0x00000100	/* System memory */
	#define PCI_REGION_RO		0x00000200	/* Read-only memory */
/* 驱动的中断向量定义 */
#define DR_SYS_TIMER_IRQ_VECTOR		    27
#define DR_SYS_AUXTIMER_IRQ_VECTOR	    2
#define DR_PERIODTIMER_IRQ_VECTOR	    3



	/**********Generic Timer ***************************/
	#define	MSTIMER_INT  MSTIMER_INT_NUM + ARM_STD_IRQ_LAST
	#define COUNTER_FREQUENCY		        (50000000)	/*50MHz*/
	#define TIMEBASE_CLK_FREQ		        (50000000)	/*50MHz*/

	/********** GMAC*************************************/
	#define GMAC_BASE	                    GMAC0_BASE
	#define PHY_ADDR	                    8
	#define FT1500A_NET_VECTOR	            GMAC1_INT_NUM + ARM_STD_IRQ_LAST

    /**********16550a ***********************************/
	#define ST16552_CHANNEL_NUM             2

	#define ST16552_CHANNEL0_BASE  			0x70000000
	#define ST16552_CHANNEL0_XTAL  			50000000
	#define ST16552_CHANNEL0_INTVECTOR		COM0_INT_NUM + ARM_STD_IRQ_LAST
	#define ST16552_CHANNEL0_ADDR_INTERVAL  4

	#define ST16552_CHANNEL1_BASE  			0x70000000
	#define ST16552_CHANNEL1_XTAL  			50000000
	#define ST16552_CHANNEL1_INTVECTOR		COM0_INT_NUM + ARM_STD_IRQ_LAST
	#define ST16552_CHANNEL1_ADDR_INTERVAL  4

	#define DR_SYS_UART_ISR_LOOP_CNT         10/* 串口中断服务程序循环查询的次数 */

	/***************I2c********************/
	#define CONFIG_SYS_I2C_SPEED            100000
	#define CONFIG_SYS_I2C_SLAVE            0x5a
	#define CONFIG_SYS_I2C_BASE             I2C1_BASE

	#define read_b(c)		(*(volatile unsigned char *)(c))
	#define read_w(c)		(*(volatile unsigned short *)(c))
	#define read_l(c)	    (*(volatile unsigned int *)(c))

	#define write_b(v,c)		(*(volatile unsigned char *)(c) = (v))
	#define write_w(v,c)		(*(volatile unsigned short *)(c) = (v))
	#define write_l(v,c)		(*(volatile unsigned int *)(c) = (v))

	/* high resolve clock definition */
	#define CONFIG_HIGH_RESOLVE_CLOCK_US  1000  /* us */


	#undef _BIT
	/* Set bit macro */
	#define _BIT(n)	(((UINT32)(1)) << (n))


/* 如果需要将ta和os直接绑定，则定义该宏。除非特殊需要，一般不建议将ta和os绑定 */
/* 注意：只适合于非调试的rom_burn方式  */
/*#define BIND_DEBUG_AGENT*/

/*----------------------------------------------------------------------------
 *
 *
 *                          内存相关的配置
 *
 *----------------------------------------------------------------------------*/

#define L2_CACHE_SUPPORT
/*  _space_malloc 定义内核的工作空间，IDLE任务栈空间，Init任务栈空间的分配方式 */
/*  pgmalloc是以4KB对齐的方式进行分配的 ,而malloc是以4字节对齐方式进行分配的 .不管是否使能MMU都可以使用malloc和pgmalloc*/
#define _space_malloc   malloc  /* 可以为malloc和pgmalloc,但是其释放操作必需使用对应的free和pgfree */

/* IDLE任务工作空间的大小 */
#define SYS_IDLE_TASK_STACK_SIZE         1024

#ifndef ASM
#ifdef __cplusplus
}
#endif
#endif
#endif
