	/* config.h - FMSH FMQL configuration header */

/*
 * Copyright (c) 2011-2019 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
010,10jul19,l_l  created.
*/

/*
DESCRIPTION
This file contains the configuration parameters for the
FMSH FMQL board.
*/

#ifndef __INCconfigh
#define __INCconfigh



#ifdef __cplusplus
extern "C" {
#endif

/*
 * BSP version/revision identification, should be placed
 * before #include "configAll.h"
 */
#define STR1(R) #R
#define STR2(R) STR1(R)
#ifdef __GNUC__
#define GCC_VER  __GNUC__.__GNUC_MINOR__.__GNUC_PATCHLEVEL__
#define CC_VERSION         "/GNU GCC:"STR2(GCC_VER)
#else
#define CC_VERSION         "/DIAB DCC:"STR2(__VERSION_NUMBER__)
/*#define CC_VERSION         "/DIAB DCC:"__VERSION__*/
#endif /*#ifdef __GNUC__*/

#define BSP_VERSION     "6.9"
#define BSP_REV         "/20230206" CC_VERSION  /* 0 for first revision */




#include <configAll.h>

#include "fmsh_fmql.h"
#include "config_pre.h"


/*
===========================================
the bsp code adapter for two kinds of board: 
	verify_board & demo_board	
-------------------------------------------	
open : verify_board of fmsh
close: demo_borad or usr_define board
===========================================
*/
#define  APU_33M_HZ   (33333333UL)
#define  APU_30M_HZ   (30000000UL)
#define  APU_50M_HZ   (50000000UL)  /* some usr-define*/

#if 0 /* demo_board */
#define FMSH_VERIFY_BOARD
#undef FMSH_DEMO_BOARD

#else /* verify_board*/
#undef FMSH_VERIFY_BOARD
#define FMSH_DEMO_BOARD
#endif

/**/
/*  cpu OSC define*/
/**/
#ifdef FMSH_VERIFY_BOARD  /* verify_board in fmsh */
	#define HARD_OSC_HZ   (APU_30M_HZ)	/* APU_30M_HZ-7020: 30.00M Hz */
#else  /* demo_board or usr-define board */
	#define HARD_OSC_HZ   (APU_33M_HZ)	/* APU_33M_HZ or APU_50M_HZ */
	/*#define HARD_OSC_HZ   (APU_50M_HZ)	// for test ref_613*/
#endif


/*
//// for test cpu 
	core-0: run vxworks, 
	core-2: run bare-metal_app or freeRTOS
----------
ddr setup:
----------
0x0000_0000 ~ 0x2FFF_FFFF: vxworks
0x3000_0000 ~ 0x3FFF_FFFF: bare-metal or freeRTOS
*/
/*#define AMP_MODE_CORE_0_RUN_VX*/
/*************************************************** 
 for default compontents
***************************************************/ 
#define DRV_MMCSTORAGE_CARD
#define DRV_SDSTORAGE_CARD
#define INCLUDE_DMA_SYS
#define INCLUDE_IPCOM_USE_AUTH
#define INCLUDE_IPTELNETS
#define INCLUDE_SD_BUS



/*************************************************** 
 uart: 16550 standard
***************************************************/ 
#ifdef DRV_SIO_NS16550
/* 1: bare-metal */
#undef UART_1_BARE_METAL

/* 2: vx-nobus */
#define UART_2_VX

/* 3: vxbus */
#define UART_3_VXB      /* default, uart_0: console*/

/* UART_1 */
#define DRV_UART_1_OPEN  /* uart_1*/
#endif

#undef  CONSOLE_BAUD_RATE
#define CONSOLE_BAUD_RATE 115200

#define INCLUDE_VXBUS
#define INCLUDE_HWMEM_ALLOC

#define INCLUDE_PLB_BUS
#define INCLUDE_PARAM_SYS

#define INCLUDE_SIO_UTILS

#define INCLUDE_INTCTLR_LIB
#define DRV_ARM_GIC

#define INCLUDE_MII_BUS
#define INCLUDE_GENERICPHY



/*************************************************** 
 GMAC: tri-speed ethernet controller
***************************************************/ 
#ifdef INCLUDE_ALT_SOC_GEN5_DW_END
/* 1: bare-metal */
#undef GMAC_1_BARE_METAL 
	
/* 2: vx-nobus */
#undef GMAC_2_VX
	
/* 3: vxbus */
#define GMAC_3_VXB
#endif  /* #ifdef INCLUDE_ALT_SOC_GEN5_DW_END */

/*
enable the 2th gmac of ps
*/
#undef GAMC_1_ENABLE


#ifdef INCLUDE_ALT_SOC_GEN5_DW_END

#define INCLUDE_GMAC_0     /* ps gmac_0 used in demo & verify	*/
#define GAMC_1_ENABLE

#if defined(FMSH_VERIFY_BOARD) || defined(GAMC_1_ENABLE) /* verify_board */
#define INCLUDE_GMAC_1     /* ps gmac_1 used*/
#endif


/**/
/*  gmac phy select*/
/**/
#ifdef FMSH_VERIFY_BOARD /* verify_board*/
#define GMAC_PHY_KSZ9031  /* microchip*/
#else  /* demo_board*/
#define GMAC_PHY_88E1116R  /* mavell*/
#endif

/**/
/* some usr-define board*/
/**/
#undef GMAC_PHY_88E1111   /* mavell*/
#undef GMAC_PHY_YT8511    /* yutai chetong*/
#undef GMAC_PHY_RTL8211    /* ref-613 */


#if defined(FMSH_VERIFY_BOARD) || defined(GAMC_1_ENABLE)  /* verify_board of fmsh*/
	/**/
	#if 0
		#define GMAC_0_PHY_ADDR  (0x2)  /* demo-board*/
		#define GMAC_1_PHY_ADDR  (0x3)

	#elif 0
		#define GMAC_0_PHY_ADDR  (0x1)   /* for bf-xian*/
		#define GMAC_1_PHY_ADDR  (0x2)
		
	#else
		#define GMAC_0_PHY_ADDR  (0x4)
		#define GMAC_1_PHY_ADDR  (0x2)
	#endif
	
#else  /* demo_board or usr_define*/

	#define GMAC_0_PHY_ADDR  (0x7)
	#define GMAC_1_PHY_ADDR  (0xF)
#endif

#endif  /* #ifdef INCLUDE_ALT_SOC_GEN5_DW_END */

#define INCLUDE_IFCONFIG
#define	INCLUDE_IPPING_CMD
#define	INCLUDE_PING

#define INCLUDE_SHELL_BANNER

#define INCLUDE_TIMER_SYS
#define INCLUDE_TIMESTAMP



/*************************************************** 
 TTC
***************************************************/ 
#define INCLUDE_ALT_SOC_GEN5_TIMER
#ifdef INCLUDE_ALT_SOC_GEN5_TIMER
/* 3: vxbus */
#define TTC_3_VXB
#endif



/*************************************************** 
PL fpag resource:
***************************************************/ 
#define FMQL_RESOURCE_FPGA



/*************************************************** 
OCM: sram on-chip:
***************************************************/ 
#undef FMQL_RESOURCE_SRAM



#ifdef _WRS_CONFIG_SMP
#define INCLUDE_VXIPI
#endif /* _WRS_CONFIG_SMP */

#define HWMEM_POOL_SIZE 50000
#define INCLUDE_VXB_CMDLINE

#define FORCE_DEFAULT_BOOT_LINE /**/ /* jc */

#if 0
#define DEFAULT_BOOT_LINE \
    "gem(0,0)host:vxWorks.st h=192.168.46.154 e=192.168.40.77:fffffe00 \
     g=192.168.40.1 u=target pw=vxTarget f=0x0 tn=fmsh_fmql"

#else
#define DEFAULT_BOOT_LINE \
    "gem(0,0)host:vxWorks.st h=192.168.46.154 e=192.168.46.77:ffffff00 \
     g=192.168.46.1 u=fmsh pw=fmsh f=0x02 tn=fmsh_fmql"
#endif

#define INCLUDE_EARLY_MMU_ENABLE  /* early mmu must be enable for FMQL PSoc */

#ifdef _WRS_CONFIG_SMP
#   define SYS_MODEL "FMSH FMQL ARMv7 MPCore"
#else
#   define SYS_MODEL "FMSH FMQL ARMv7"
#endif /* _WRS_CONFIG_SMP */

/* Memory configuration */
#define LOCAL_MEM_LOCAL_ADRS  0x00100000
#define LOCAL_MEM_SIZE        (0x10000000 - 0x00100000)
#define LOCAL_MEM_END_ADRS    (LOCAL_MEM_LOCAL_ADRS + LOCAL_MEM_SIZE)

/*
 * The constants ROM_TEXT_ADRS and ROM_SIZE are defined
 * in config.h and Makefile.
 * All definitions for these constants must be identical.
 */
#define ROM_BASE_ADRS       0x0fc00000          /* ROM base                    */
#define ROM_TEXT_ADRS       0x0fc00000          /* code start addr in ROM      */
#define ROM_WARM_ADRS       (ROM_TEXT_ADRS + 4) /* ROM warm entry address      */

#define ROM_SIZE            0x00200000          /* size of ROM holding VxWorks */
#define ROM_SIZE_TOTAL      0x00400000          /* total size of ROM           */

/* Macro for enlarge RAM to 1G */
#define FMQL_MEM_EXTRA       (LOCAL_MEM_END_ADRS)
#define FMQL_MEM_EXTRA_SIZE  0x30000000

/* Serial port configuration */
#undef  NUM_TTY
#define NUM_TTY             N_SIO_CHANNELS

#define DRV_SIO_NS16550


#ifdef UART_1_AS_CONSOLE
#define DRV_UART_1_OPEN  /* uart_1 used as console, must be enable*/
#endif

/*
 * Cache/MMU configuration
 *
 * Note that when MMU is enabled, cache modes are controlled by
 * the MMU table entries in sysPhysMemDesc[], not the cache mode
 * macros defined here.
 */

/*
 * We use the generic architecture libraries, with caches/MMUs present. A
 * call to sysHwInit0() is needed from within usrInit before
 * cacheLibInit() is called.
 */

#ifndef _ASMLANGUAGE
IMPORT void sysHwInit0 (void);
#endif /* _ASMLANGUAGE */

#define INCLUDE_SYS_HW_INIT_0
#define SYS_HW_INIT_0()         sysHwInit0()

#ifdef _WRS_CONFIG_SMP
#define VX_SMP_NUM_CPUS 4
#endif /* _WRS_CONFIG_SMP */

/*
 * These processors can be either write-through or copyback (defines
 * whether write-buffer is enabled); cache itself is write-through.
 */

#undef  USER_I_CACHE_MODE
#define USER_I_CACHE_MODE       (CACHE_COPYBACK)

#undef  USER_D_CACHE_MODE
#define USER_D_CACHE_MODE       (CACHE_COPYBACK)

#define INCLUDE_MMU_BASIC
#define INCLUDE_MMU_FULL
#define INCLUDE_CACHE_SUPPORT

#define INCLUDE_VFP /* vector floating point support */

/*
 * interrupt mode - interrupts can be in either preemptive or non-preemptive
 * mode. For preemptive mode, change INT_MODE to INT_PREEMPT_MODEL
 */

#define INT_MODE                 INT_NON_PREEMPT_MODEL
#define ISR_STACK_SIZE           (0x2000)    /* size of ISR stack, in bytes */

#if 0
/* timer config */
#define SYSCLK_TIMER_NAME    "altTimer"
#define SYSCLK_TIMER_UNIT    0
#define SYSCLK_TIMER_NUM     0

#define AUXCLK_TIMER_NAME    "altTimer"
#define AUXCLK_TIMER_UNIT    1
#define AUXCLK_TIMER_NUM     0

#define TIMESTAMP_TIMER_NAME     "altTimer"
#define TIMESTAMP_TIMER_UNIT     0
#define TIMESTAMP_TIMER_NUM      0
#endif


/*************************************************************/
/* below define whole function for all device drivers        */
/* define for vip, undef for whole functions                 */
/*************************************************************/

/*
1-whole_functions, 0-min_system 
*/
#if 1  
#undef BUILD_VIP_MIN_SYSTEM   /* whole_functions system */
#else
#define BUILD_VIP_MIN_SYSTEM  /* vip min_system */
#endif


#ifndef BUILD_VIP_MIN_SYSTEM

/*************************************************** 
QSPI support
***************************************************/ 
#ifdef DRV_FM_QSPI
/* 1: bare-metal */
#undef QSPI_1_BARE_METAL

/* 2: vx-nobus */
#define QSPI_2_VX

/* 3: vxbus */
#undef QSPI_3_VXB
#endif /* #ifdef DRV_FM_QSPI */

#if defined(QSPI_2_VX) || defined(QSPI_3_VXB)

#define QSPI_NOR_FLASH       /* nor-flash*/
#undef QSPI_NAND_FLASH       /* nand*/

/* nor-flash*/
#ifdef QSPI_NOR_FLASH
#if 1  /* tffs-ok, fatFS-disable*/
#define INCLUDE_TFFS
#undef ENABLE_FATFS_QSPI

#else  /* fatFS-ok, tffs-disable*/
#undef INCLUDE_TFFS
#define ENABLE_FATFS_QSPI
#endif

#define INCLUDE_FM_QSPI
#define QSPI_DIRECT_MODE  (0)  /* 1-Direct_mode(SRAM-16M), 0-Indirect_mode*/

#ifdef  INCLUDE_FM_QSPI
#define QSPI_FLASH_MEM_SIZE           (SZ_32M)       /* SZ_32M SZ_128M-origin(1Gbit); S25FL256S 256 Mbit (32 Mbyte) */

#define QSPI_FLASH_SECTOR_SIZE        (SZ_64K)     /* SZ_64K: sector size of N25Q00 */
#define QSPI_FLASH_PAGE_SIZE          (0x100)       /* 256B/page size of N25Q00: 0x100 */

#define QSPI_FLASH_BASE_ADRS          (0xE0000000)
#define QSPI_FLASH_DATA_BASE_ADRS     (0xE8000000)

#define QSPI_FLASH_OVERLAY
#endif /* INCLUDE_FM_QSPI */
#endif

/*
 qspi-nand_flash
*/
#ifdef QSPI_NAND_FLASH
#endif

#endif /* #if defined(QSPI_2_VX) || defined(QSPI_3_VXB) */

/* 
FLASH_BASE_ADRS is the base address used by TFFS 
for QSPI_CTRL_0:
*/
#define FLASH_BASE_ADRS          (0xE0000000) /* QSPI_FLASH_BASE_ADRS */
#define FLASH_DATA_BASE_ADRS     (0xE8000000)  /* #define FPS_QSPI0_D_BASEADDR (0xE8000000) */
 
/*
 * FLASH_SIZE is the flash size used by TFFS.
 * This definition can be modified. 
 */
#ifndef DRV_FM_QSPI
#define FLASH_SIZE               (SZ_8M)  /* SZ_16M default for tffs*/
#define FLASH_SECTOR_SIZE        (SZ_64K)
#else
#define FLASH_SIZE               (QSPI_FLASH_MEM_SIZE)
#define FLASH_SECTOR_SIZE        (QSPI_FLASH_SECTOR_SIZE)
#endif



#undef TFFS_FLASH_8M
#define TFFS_FLASH_16M
#undef TFFS_FLASH_32M
#undef TFFS_FLASH_64M


#ifdef INCLUDE_TFFS
#define INCLUDE_TFFS_MOUNT
#define INCLUDE_TFFS_SHOW	

#define INCLUDE_DISK_UTIL
#define INCLUDE_DISK_UTIL_SHELL_CMD

#define INCLUDE_DOSFS
#define INCLUDE_DOSFS_CACHE
#define INCLUDE_DOSFS_CHKDSK
#define INCLUDE_DOSFS_DIR_FIXED
#define INCLUDE_DOSFS_DIR_VFAT
#define INCLUDE_DOSFS_FAT
#define INCLUDE_DOSFS_FMT
#define INCLUDE_DOSFS_MAIN
#define INCLUDE_DOSFS_PRTMSG_LEVEL
#define INCLUDE_DOSFS_SHOW
#define INCLUDE_DOSFS_VOL_BIO_BUFFER_SIZE

#define INCLUDE_FS_EVENT_UTIL
#define INCLUDE_FS_MONITOR

#define INCLUDE_XBD
#endif	



/*************************************************** 
 dma must define befor sdmmc, because of sdmmc may use dma
***************************************************/ 
#ifdef DRV_FM_DMA
/* 1: bare-metal */
#define DMA_1_BARE_METAL 

/* 2: vx-nobus */
#define DMA_2_VX

/* 3: vxbus */
#undef DMA_3_VXB
#endif  /* #ifdef DRV_FM_DMA */



/*************************************************** 
 SD/MMC support
***************************************************/ 
#ifdef DRV_FM_SDMMC
		/* 1: bare-metal */
	#define SDMMC_1_BARE_METAL 
	
		/* 2: vx-nobus */
	#undef SDMMC_2_VX
	
		/* 3: vxbus */
	#define SDMMC_3_VXB
	
		/* 4: vxbus-ref_alt */
	#undef SD_DOSFS_V1
		
		/*
		sdmmc data_speed: 5M or 25M or other: 50M, 52M
		*/
	#define SDEMMC_25M_OR_5M_ONLY	(25)  /* 25->25M or 5->5M or 50->50M*/
#endif  /* #ifdef DRV_FM_SDMMC */

#ifdef SDMMC_2_VX
	/*#define ENABLE_FATFS_SDMMC	/* vxbus-fatFS 	*/
	#define SDMMC_DATA_SPEED   SDMMC_CLK_50_MHZ  /* SDMMC_CLK_50_MHZ or SDMMC_CLK_25_MHZ*/
#endif

#ifdef SDMMC_3_VXB
	/*
	only one exist for fatFS or dosFS in vxbus
	*/
	#if 0  /* fatFS-ok, dosFS-disable*/
	#define SDMMC_VXB_FATFS    /* vxbus-fatFS*/
	#undef SDMMC_DOSFS_V2      /* vxbus-dosfs*/
	
	#else /* dosfs-ok, fatFS-disable*/
	#undef SDMMC_VXB_FATFS      /* vxbus-fatFS*/
	#define SDMMC_DOSFS_V2      /* vxbus-dosfs*/
	#endif

	#ifdef SDMMC_VXB_FATFS
	#define ENABLE_FATFS_SDMMC  /* vxbus-fatFS */
	#endif  /* #ifdef SDMMC_VXB_FATFS */

	#ifdef SDMMC_DOSFS_V2
	#define INCLUDE_SD_BUS
	#define DRV_SDSTORAGE_CARD   /* for sd*/
	#define DRV_MMCSTORAGE_CARD  /* for emmc*/
	#endif  /* #ifdef SDMMC_DOSFS_V2 */	
#endif  /* #ifdef SDMMC_3_VXB */



/*************************************************** 
 WDT(watchdog timer) support 
***************************************************/ 
#undef INCLUDE_ALT_SOC_GEN5_WDT



/*************************************************** 
USB support 
***************************************************/ 
#ifdef INCLUDE_SYNOPSYSHCI
#define USB_3_VXB

/* 3: vxbus */
#ifdef USB_3_VXB
#define INCLUDE_SYNOPSYSHCI_INIT
#define INCLUDE_EHCI_INIT
#define INCLUDE_HCD_BUS
#define INCLUDE_USB_PCI_STUB_CUSTOM
#define INCLUDE_USB_TRACK_SYNOPSYSHCD

#define INCLUDE_USB
#define INCLUDE_USB_INIT
#define INCLUDE_USB_SHOW

#undef INCLUDE_USB_GEN2_STORAGE      /* Storage Driver */
#undef INCLUDE_USB_GEN2_STORAGE_INIT /* Storage Driver Initialization */
#undef INCLUDE_USB_GEN2_HELPER
#endif  /* #ifdef USB_3_VXB */
#endif  /* #ifdef INCLUDE_SYNOPSYSHCI */



/*************************************************** 
 CAN support 
***************************************************/ 
#ifdef DRV_FM_CAN
/* 1: bare-metal */
#define CAN_1_BARE_METAL 

/* 2: vx-nobus */
#undef CAN_2_VX

/* 3: vxbus */
#undef CAN_3_VXB
#endif  /* #ifdef DRV_FM_CAN */



/*************************************************** 
 SPI support 
***************************************************/ 
#ifdef DRV_FM_SPI
/* 1: bare-metal */
#undef SPI_1_BARE_METAL 

/* 2: vx-nobus */
#define SPI_2_VX

/* 3: vxbus */
#undef SPI_3_VXB
#endif  /* #ifdef DRV_FM_SPI */



/*************************************************** 
 I2C
***************************************************/ 
#ifdef INCLUDE_ALT_SOC_GEN5_DW_I2C
/* 1: bare-metal */
#define I2C_1_BARE_METAL 

/* 2: vx-nobus */
#define I2C_2_VX

/* 3: vxbus */
#undef I2C_3_VXB
#endif



/*************************************************** 
 NFC(nandflash) support
***************************************************/ 
#ifdef DRV_FM_NFC
/* 1: bare-metal */
#undef NFC_1_BARE_METAL 

/* 2: vx-nobus */
#define NFC_2_VX

/* 3: vxbus */
#undef NFC_3_VXB
#endif  /* #ifdef DRV_FM_NFC */



/*************************************************** 
 GPIO support
***************************************************/ 
#ifdef INCLUDE_FMSH_FMQL_GPIO
/* 1: bare-metal */
#undef GPIO_1_BARE_METAL 

/* 2: vx-nobus */
#define GPIO_2_VX

/* 3: vxbus */
#undef GPIO_3_VXB

/* 4: legacy */
#define GPIO_4_LEGACY
#ifdef GPIO_4_LEGACY
#undef INCLUDE_VXB_LEGACY_INTERRUPTS
#define INCLUDE_VXB_LEGACY_INTERRUPTS
#endif
#endif /* INCLUDE_FMSH_FMQL_GPIO */



/*************************************************** 
 DEVC: config-PL
***************************************************/ 
#ifdef DRV_FM_DEVC
/* 1: bare-metal */
#undef DEVC_1_BARE_METAL 

/* 2: vx-nobus */
#define DEVC_2_VX

/* 3: vxbus */
#undef DEVC_3_VXB
#endif  /* #ifdef DRV_FM_DEVC */

#ifdef DEVC_2_VX
#define FMQL_RESOURCE_FPGA
#endif



/*************************************************** 
 SMC: sram / norflash
***************************************************/ 
#ifdef DRV_FM_SMC
/* 1: bare-metal */
#undef SMC_1_BARE_METAL 

/* 2: vx-nobus */
#define SMC_2_VX
#define SMC_2_NORFLASH
#undef SMC_2_SRAM

/* 3: vxbus */
#undef SMC_3_VXB
#endif  /* #ifdef DRV_FM_SMC */



/*************************************************** 
 GTC support 
***************************************************/ 
#ifdef INCLUDE_DRV_FM_GTC
/* 1: bare-metal */
#undef GTC_1_BARE_METAL 

/* 2: vx-nobus */
#define GTC_2_VX

/* 3: vxbus */
#undef GTC_3_VXB
#endif  /* #ifdef INCLUDE_DRV_FM_GTC */



/*************************************************** 
 WDT(WatchDog Timer) support 
***************************************************/ 
#ifdef DRV_FM_WDT
/* 1: bare-metal */
#undef WDT_1_BARE_METAL 

/* 2: vx-nobus */
#define WDT_2_VX

/* 3: vxbus */
#undef WDT_3_VXB
#endif  /* #ifdef DRV_FM_WDT */



/*************************************************** 
 Ramdisk support for vxWorks(some ram used as disk) 
***************************************************/ 
#undef DRV_FM_RAMDISK_VX



/**************************************************
 PL IP demo: by GP or HP port
**************************************************/
#ifdef PL_PFGA_RUN

/* pl - xadc demo */
#undef PL_XADC 

/* pl - axi_dma demo */
#undef PL_AXI_DMA

/* pl - vdma_hdmi demo */
#undef PL_VDMA_HDMI

/* pl - rs422 demo */
#define PL_RS422

/* pl - pcie-rc demo */
#undef PL_PCIE_RC
#undef PL_PCIE_RC_VXB  /* vxbus drv for pl_pcie_rc */


#if 1  /* test for lingya ok */
/* pl - axi_spi */
#define PL_AXI_SPI

/* pl - axi_gpio */
#define PL_AXI_GPIO

/* pl - axi_bram */
#define PL_AXI_BRAM
#endif


#endif  /* #ifdef PL_PFGA_RUN */


#endif  /* #ifndef BUILD_VIP_MIN_SYSTEM */
/*************************************************************/
/* over define whole function for all device drivers        */
/* define for vip, undef for whole functions                 */
/*************************************************************/



/*
vxWorks user app init entry
===========================
setup note:
==========
add at line 29:  #include "config.h",
in file: usrAppInit.c of VIP
----------
<<<<<<<<then print log:>>>>>>>

==== Image6911 build: Dec 25 2020-10:31:39 ==== 

== HARD_OSC_HZ: 33333333->	Hz !!! Demo_Board or Usr_Define !!! == 
== CPU: 666666660 Hz,DDR: 1599999984 bps == 
*/
#ifndef	USER_APPL_INIT
#define USER_APPL_INIT  app_main();
#endif  /* #ifdef	USER_APPL_INIT */

/*
in vx image booting process, while 2 key(of keyboard) pressed, 
should be ack
*/
#undef VX_BOOT_ACK_2KEY	

/* Support network devices */

#ifdef __cplusplus
}
#endif
#endif  /* __INCconfigh */

#if defined(PRJ_BUILD)
#include "prjParams.h"
#endif

#define NV_MAC_ADRS_OFFSET (0x1000000-0x100000)  /* 0x100_0000 - 0x10_0000 */



