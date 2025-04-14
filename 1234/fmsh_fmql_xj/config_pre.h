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
010,12May21,l_l  created.
*/

/*
DESCRIPTION
This file contains the configuration parameters for the FMSH FMQL board.
*/

#ifndef __INC_config_part_h
#define __INC_config_part_h


/*
jc:
default: uart_0 used as console: #undef UART_1_AS_CONSOLE
		 uart_1 may be console: #define UART_1_AS_CONSOLE
*/
#if 0 /* test for wh-yuhang */
#define UART_1_AS_CONSOLE


#define SD_CTRL1_AS_EMMC  /* test ctrl_1-->emmc for wh-yuhang */
#undef SD_DRV_LIB_DEBUG_MODE 
#endif

#if 1  /*for demo_board */
#undef SD_DRV_LIB_DEBUG_MODE 
#endif

#if 0  /*test for bj_ruineng */
#define SD_DEVICE_1P8_V    /* test for bj_ruineng */
#endif


/*
 uart: 16550 standard
*/ 
#define DRV_SIO_NS16550


/*
 GMAC: tri-speed ethernet controller
*/ 
/* 
#define INCLUDE_GMAC_FM_END 
*/
#define INCLUDE_ALT_SOC_GEN5_DW_END

#define INCLUDE_AXI_ETHERNET_END
#undef INCLUDE_AXI_ETHERNET_END

/* 
 QSPI support
*/
#define DRV_FM_QSPI


/*
 dma must define befor sdmmc, because of sdmmc may use dma
*/
#define DRV_FM_DMA 


/* 
 SD/MMC support
*/
#undef DRV_FM_SDMMC
/*#define DRV_FM_SDMMC*/
/*
 USB support 
*/
#undef INCLUDE_SYNOPSYSHCI


/* 
 CAN support 
*/
#undef DRV_FM_CAN


/* 
 SPI support 
*/
#define DRV_FM_SPI


/*
 I2C
*/
#undef INCLUDE_ALT_SOC_GEN5_DW_I2C


/* 
 NFC(nandflash) support
*/
#undef DRV_FM_NFC


/* 
 GPIO support
*/
#define INCLUDE_FMSH_FMQL_GPIO


/*
 DEVC: config-PL
*/
#define DRV_FM_DEVC


/*
 SMC: sram / norflash
*/
#undef DRV_FM_SMC


/* 
 GTC support 
*/
#define INCLUDE_DRV_FM_GTC


/* 
 WDT(WatchDog Timer) support 
*/
#undef DRV_FM_WDT


/**************************************************
PL IP demo: by GP or HP port
**************************************************/
#undef PL_PFGA_RUN   /* open  - enable pl_to_ps irq, 
                         close - disable 
                      */

#define IRQ_PL_2_PS   /* open  - enable pl_to_ps irq,
                         close - disable 
                      */

#endif  /* #ifndef __INC_config_part_h */


