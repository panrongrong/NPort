/* sysInc_All.h - include c file for image compile */

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

#ifndef __INC_SYSINC_ALL_H
#define __INC_SYSINC_ALL_H
#include <vxworks.h>


#include "config.h"


/*
	usr-define printf
*/
#if 0 
#include "mysprintf.c"

char fmql16550Pollprintftempstr[1000];
IMPORT void fmql_16550Pollprintf(char* str);

#define DEBUG_LOG(string,...) {                                        \
		mysprintf(fmql16550Pollprintftempstr,string,__VA_ARGS__);    \
		fmql_16550Pollprintf(fmql16550Pollprintftempstr);               \
}  
#endif

/*
all head file of drv
*/

#if 1
#include "vxBus.c"
#include "fioBaseLib.c"
#endif


/* 
common 
*/
#if 1
#include "./bsp_drv/common/fmsh_common.c"
#include "./bsp_drv/common/ps_init.c"
#endif


/***********************************************
slcr 
***********************************************/
#if 1
#include "./bsp_drv/slcr/fmsh_slcr.c"
#include "./bsp_drv/slcr/sysSlcrMgr.c"
#endif


/***********************************************
gic: interrupt 
***********************************************/
#if 1
#include "./bsp_drv/gic/gic_3_vxb/vxbArmGenIntCtlr.c"
#include "./bsp_drv/gic/gic_2_vx/vxGic.c"
#endif


/***********************************************
uart: console
***********************************************/
#ifdef DRV_SIO_NS16550

/*
1: bare-metal
*/
#ifdef UART_1_BARE_METAL
#include "./bsp_drv/uart/uart_1_bare/fmsh_uart_g.c"
#include "./bsp_drv/uart/uart_1_bare/fmsh_uart_sinit.c"
#include "./bsp_drv/uart/uart_1_bare/fmsh_uart.c"
#endif

/*
2: vx-nobus
*/
#ifdef UART_2_VX
#include "./bsp_drv/uart/uart_2_vx/vxUart.c"
#endif

/*
3: vxbus
*/
#ifdef UART_3_VXB
#include "./bsp_drv/uart/uart_3_vxb/vxbNs16550Sio.c"
#endif

#endif


/***********************************************
timer: ttc, system-tick 
***********************************************/
#ifdef INCLUDE_ALT_SOC_GEN5_TIMER
/*
3: vxbus
*/
#ifdef TTC_3_VXB
#include "./bsp_drv/timer/ttc_3_vxb/vxbAltSocGen5Timer.c"
#endif
#endif  /* #ifdef INCLUDE_ALT_SOC_GEN5_TIMER */


#if 1
#include "./sysNet.c"   /* must be open */
#endif
/*************************************************************/
/* above define: vxWorks VIP min-system                      */
/*               gic, uart, timer                            */
/*************************************************************/


/*************************************************************/
/* below define whole function for all device drivers        */
/* define for vip, undef for whole functions                 */
/*************************************************************/
#if 1  /* ndef BUILD_VIP_MIN_SYSTEM*/
/*#ifndef BUILD_VIP_MIN_SYSTEM*/

/***********************************************
gmac 
***********************************************/
#ifdef  INCLUDE_ALT_SOC_GEN5_DW_END

/*
1: bare-metal
*/
#ifdef GMAC_1_BARE_METAL
/**/
/* gmac ctrl*/
/**/
#include "./bsp_drv/gmac/gmac_1_bare/gmacps_v1_0/fmsh_gmac_g.c"
#include "./bsp_drv/gmac/gmac_1_bare/gmacps_v1_0/fmsh_gmac_sinit.c"
#include "./bsp_drv/gmac/gmac_1_bare/gmacps_v1_0/fmsh_gmac_hw.c"
#include "./bsp_drv/gmac/gmac_1_bare/gmacps_v1_0/fmsh_gmac_lib.c"

/**/
/* phy*/
/**/
#include "./bsp_drv/gmac/gmac_1_bare/gmacps_v1_0/phy/marvell_88e1116r.c"
#include "./bsp_drv/gmac/gmac_1_bare/gmacps_v1_0/phy/fmsh_mdio.c"

/**/
/* gpio for reset-of-phy*/
/**/
#include "./bsp_drv/gmac/gmac_1_bare/gmacps_v1_0/phy/gpiops_v1_0/fmsh_gpio_g.c"
#include "./bsp_drv/gmac/gmac_1_bare/gmacps_v1_0/phy/gpiops_v1_0/fmsh_gpio_sinit.c"
#include "./bsp_drv/gmac/gmac_1_bare/gmacps_v1_0/phy/gpiops_v1_0/fmsh_gpio.c"

/**/
/* test gmac code*/
/* */
#include "./bsp_drv/gmac/gmac_1_bare/test_gmac_loop.c"
#endif

/*
2: vx-nobus
*/
#ifdef GMAC_2_VX
/* nothing*/
#endif

/*
3: vxbus
*/
#ifdef GMAC_1_BARE_METAL
#undef GMAC_3_VXB  /* only for test bare-drv*/
#endif

#ifdef GMAC_3_VXB
#include "./bsp_drv/gmac/gmac_3_vxb/vxbAltSocGen5DwEnd.c"
#include "./bsp_drv/gmac/phy_3_vxb/genericPhy.c"
#endif

#if 1
#if 1  /* new ping_cmd: response_time ok*/
#include "./bsp_drv/gmac/net_tools/vxPing2.c"
#endif

/* 
test_net
*/
#if 0  /* tcp*/
#include "./bsp_drv/gmac/test_net/vxBlastee_tcp.c"  /* rx*/
#include "./bsp_drv/gmac/test_net/vxBlaster_tcp.c"  /* tx*/
#endif

#if 0 /* udp*/
#include "./bsp_drv/gmac/test_net/vxBlastee_udp.c"  /* rx*/
#include "./bsp_drv/gmac/test_net/vxBlaster_udp.c"  /* tx*/
#endif

#if 0 /* tcp_udp, only for diab-tool */
#include "./bsp_drv/gmac/test_net/vxBlastee_mfq.c"
#include "./bsp_drv/gmac/test_net/vxBlaster_mfq.c"
#endif

#endif
#endif  /* #ifdef  INCLUDE_ALT_SOC_GEN5_DW_END */


/***********************************************
QSPI--norflash
***********************************************/
#ifdef DRV_FM_QSPI
/*
1: bare-metal
*/
#ifdef QSPI_1_BARE_METAL
#include "./bsp_drv/qspi/qspi_1_bare/fmsh_spips_hw.c"
#include "./bsp_drv/qspi/qspi_1_bare/fmsh_spips_sw.c"
#include "./bsp_drv/qspi/qspi_1_bare/fmsh_spips.c"
#endif

/*
2: vx-nobus
*/
#ifdef QSPI_2_VX

#ifdef QSPI_NOR_FLASH  /* nor-flash */

/* fatFS */
#if 0  /*def ENABLE_FATFS_QSPI*/
#include "./bsp_drv/qspi/qspi_2_vx/nor/vxFatFS_st/diskio.c"
#include "./bsp_drv/qspi/qspi_2_vx/nor/vxFatFS_st/ff.c"
#include "./bsp_drv/qspi/qspi_2_vx/nor/vxFatFS_st/ff_gen_drv.c"
#include "./bsp_drv/qspi/qspi_2_vx/nor/vxFatFS_st/spiflash_diskio.c"
#include "./bsp_drv/qspi/qspi_2_vx/nor/vxFatFS_st/demo_qspi_fatfs.c"
#include "./bsp_drv/qspi/qspi_2_vx/nor/vxFatFS_st/option/syscall.c"
#endif

#ifdef INCLUDE_TFFS
/* #include "./sysTffs.c"*/
/* #include "./sysMtd.c"*/
#endif

#include "./bsp_drv/qspi/qspi_2_vx/nor/vxQspi_Norflash.c"
#endif

#ifdef QSPI_NAND_FLASH  /* nand-flash */
#include ".bsp_drv/*qspi/qspi_2_vx/nand/vxQspi_Nand.c"*/
#endif
#endif  /* #ifdef QSPI_2_VX */

/*
3: vxbus
*/
#ifdef QSPI_3_VXB
#include "./bsp_drv/qspi/qspi_3_vxb/vxbQspi_fm.c"
#endif

/*
test for tianmai_os1:
*/
#if 0
#include "./bsp_drv/qspi/qspi_1_bare/tm_os1/qspips_v1_0/fmsh_qspips_g.c"
#include "./bsp_drv/qspi/qspi_1_bare/tm_os1/qspips_v1_0/fmsh_qspips_sinit.c"
#include "./bsp_drv/qspi/qspi_1_bare/tm_os1/qspips_v1_0/fmsh_qspips_hw.c"
#include "./bsp_drv/qspi/qspi_1_bare/tm_os1/qspips_v1_0/fmsh_qspips_sw.c"
#include "./bsp_drv/qspi/qspi_1_bare/tm_os1/qspips_v1_0/fmsh_qspips.c"

#include "./bsp_drv/qspi/qspi_1_bare/tm_os1/qspips_v1_0/fmsh_qspips_nor.c"
#include "./bsp_drv/qspi/qspi_1_bare/tm_os1/qspips_v1_0/fmsh_qspips_nand.c"

#include "./bsp_drv/qspi/qspi_1_bare/tm_os1/tm_os1_qspi.c"
#endif

#endif  /* #ifdef DRV_FM_QSPI */



/***********************************************
SD/MMC 
***********************************************/
#ifdef DRV_FM_SDMMC

#undef SDMMC_DMA_BARE_METAL_DRV
#define SDMMC_DMA_NO_VXBUS_DRV

/*
1: bare-metal
*/
#ifdef SDMMC_1_BARE_METAL
#include "./bsp_drv/sdmmc/sdmmc_1_bare/fmsh_sdmmc_g.c"
#include "./bsp_drv/sdmmc/sdmmc_1_bare/fmsh_sdmmc_sinit.c"
#include "./bsp_drv/sdmmc/sdmmc_1_bare/fmsh_sdmmc.c"
#include "./bsp_drv/sdmmc/sdmmc_1_bare/fmsh_sdmmc_protocol.c"
/*#include "./bsp_drv/sdmmc/sdmmc_1_bare/sd.c"*/

#include "./bsp_drv/sdmmc/sdmmc_1_bare/fmsh_sdmmc_verify.c"

/* fatFS */
#if 0
#include "./bsp_drv/sdmmc/sdmmc_1_bare/fatFS/diskio.c"
#include "./bsp_drv/sdmmc/sdmmc_1_bare/fatFS/ff.c"
#include "./bsp_drv/sdmmc/sdmmc_1_bare/fatFS/ffsystem.c"
#include "./bsp_drv/sdmmc/sdmmc_1_bare/fatFS/ffunicode.c"
#endif
#endif  /* #ifdef SDMMC_1_BARE_METAL */

/*
2: vx-nobus and vxbus can't exist at the same
*/
#ifdef SDMMC_2_VX
#include "./bsp_drv/sdmmc/sdmmc_2_vx/vxSdmmc.c"
#include "./bsp_drv/sdmmc/sdmmc_2_vx/sdmmc_cmd.c"

#ifdef ENABLE_FATFS_SDMMC
#include "./bsp_drv/sdmmc/sdmmc_2_vx/vxFatFS2/diskio.c"
#include "./bsp_drv/sdmmc/sdmmc_2_vx/vxFatFS2/ff.c"
#include "./bsp_drv/sdmmc/sdmmc_2_vx/vxFatFS2/ffsystem.c"
#include "./bsp_drv/sdmmc/sdmmc_2_vx/vxFatFS2/ffunicode.c"
/*#include "./bsp_drv/sdmmc/sdmmc_2_vx/vxbFatFS2/demo_sdmmc_fatfs.c"*/
#endif 
#endif  /* #ifdef SDMMC_2_VX */

/*
3: vxbus driver 
*/
#ifdef SDMMC_3_VXB

/* vxbus driver1: for fatFS */
#ifdef SDMMC_VXB_FATFS
#ifdef ENABLE_FATFS_SDMMC
#include "./bsp_drv/sdmmc/sdmmc_3_vxb/vxbFatFS2/diskio.c"
#include "./bsp_drv/sdmmc/sdmmc_3_vxb/vxbFatFS2/ff.c"
#include "./bsp_drv/sdmmc/sdmmc_3_vxb/vxbFatFS2/ffsystem.c"
#include "./bsp_drv/sdmmc/sdmmc_3_vxb/vxbFatFS2/ffunicode.c"
#include "./bsp_drv/sdmmc/sdmmc_3_vxb/vxbFatFS2/demo_sdmmc_fatfs.c"
#endif
#include "./bsp_drv/sdmmc/sdmmc_3_vxb/vxbSdmmc2.c"
#include "./bsp_drv/sdmmc/sdmmc_3_vxb/vxbSdmmc_cmd2.c"
#endif

/* vxbus driver2: for dosFS */
#ifdef SDMMC_DOSFS_V2  /* ref demo_board dosfs for both sd & emmc*/

#ifdef SD_DEVICE_1P8_V  /* for ruineng 1.8V emmc-64G*/
#include "./bsp_drv/sdmmc/sdmmc_3_dosfs/1p8v_emmc/vxbFmshSdhcCtrl.c"
#ifdef SD_DRV_LIB_DEBUG_MODE
#include "./bsp_drv/sdmmc/sdmmc_3_dosfs/1p8v_emmc/vxbSdLib.c"       /* for debug*/
#include "./bsp_drv/sdmmc/sdmmc_3_dosfs/1p8v_emmc/vxbMmcStorage.c"  /* for debug-some_err*/
#endif

#else  /* 3.3v for verify_board: sd/emmc*/

#include "./bsp_drv/sdmmc/sdmmc_3_dosfs/3p3v_sdemmc/vxbFmshSdhcCtrl.c"
#ifdef SD_DRV_LIB_DEBUG_MODE
#include "./bsp_drv/sdmmc/sdmmc_3_dosfs/3p3v_sdemmc/vxbSdLib.c"  /* for debug, maybe disable for release*/
#endif
#endif  /* #ifdef SD_DEVICE_1P8_V  */
#endif  /* #ifdef SDMMC_DOSFS_V2 */
#endif  /* #ifdef SDMMC_3_VXB */

/*
4: vxbus driver - ref_alt
*/
#ifdef SD_DOSFS_V1

#if 1
#include "./bsp_drv/sdmmc/sdmmc_alt/vxbAltSocGen5DwMshc_fm.c"
#include "./bsp_drv/sdmmc/sdmmc_alt/vxbSdMmcLib_fm.c"

#include "./bsp_drv/sdmmc/sdmmc_alt/vxbSdMmcXbd.c"
#else
#include "./bsp_drv/sdmmc/sdmmc_dosfs_v1/vxbAltSocGen5DwMshc.c"
#include "./bsp_drv/sdmmc/sdmmc_dosfs_v1/vxbSdMmcLib.c"

/*#include "./bsp_drv/sdmmc/sdmmc_alt/vxbSdMmcXbd.c"*/
#endif

/* fatFS:  vxbFatFS2 */
#if 0
#include "./bsp_drv/sdmmc/sdmmc_alt/vxbFatFS2/diskio.c"
#include "./bsp_drv/sdmmc/sdmmc_alt/vxbFatFS2/ff.c"
#include "./bsp_drv/sdmmc/sdmmc_alt/vxbFatFS2/ffsystem.c"
#include "./bsp_drv/sdmmc/sdmmc_alt/vxbFatFS2/ffunicode.c"
/*#include "./bsp_drv/sdmmc/sdmmc_alt/vxbFatFS2/demo_sdmmc_fatfs.c"*/
#endif

#if 0
/* fatFS:  vxbFatFS1 */
#include "./bsp_drv/sdmmc/sdmmc_alt/vxbFatFS1/diskio.c"
#include "./bsp_drv/sdmmc/sdmmc_alt/vxbFatFS1/ff.c"
#include "./bsp_drv/sdmmc/sdmmc_alt/vxbFatFS1/ffsystem.c"
#include "./bsp_drv/sdmmc/sdmmc_alt/vxbFatFS1/ffunicode.c"
#endif

#endif  /* #ifdef SD_DOSFS_V1 */
#endif  /* #ifdef DRV_FM_SDMMC */


/***********************************************
DMA 
***********************************************/
#ifdef DRV_FM_DMA
/*
1: bare-metal
*/
#ifdef DMA_1_BARE_METAL
/*#ifdef SDMMC_DMA_BARE_METAL_DRV*/
#include "./bsp_drv/dma/dma_1_bare/fmsh_dmac.c"
#include "./bsp_drv/dma/dma_1_bare/fmsh_dmac_g.c"
#include "./bsp_drv/dma/dma_1_bare/fmsh_dmac_sinit.c"
/*#endif */
#endif /* #ifdef DMA_1_BARE_METAL */

/*
2: vx-nobus
*/
#ifdef DMA_2_VX
#ifdef SDMMC_DMA_NO_VXBUS_DRV
#include "./bsp_drv/dma/dma_2_vx/vxDma.c"
#endif
#endif  /* #ifdef DMA_2_VX */

/*
3: vxbus
*/
#ifdef DMA_3_VXB
/* have nothing*/
#endif  /* #ifdef DMA_3_VXB */
#endif /* #ifdef DRV_FM_DMA */

/***********************************************
CAN 
***********************************************/
#ifdef DRV_FM_CAN
/*
1: bare-metal
*/
#ifdef CAN_1_BARE_METAL 
#include "./bsp_drv/can/can_1_bare/fmsh_can.c"
#include "./bsp_drv/can/can_1_bare/fmsh_can_g.c"
#include "./bsp_drv/can/can_1_bare/fmsh_can_sinit.c"
#endif

/*
2: vx-nobus
*/
#ifdef CAN_2_VX
#include "./bsp_drv/can/can_2_vx/vxCan.c"
#endif

/*
3: vxbus
*/
#ifdef CAN_3_VXB
#include "./bsp_drv/can/can_3_vxb/vxbCan_fm.c"
#endif
#endif  /* #ifdef DRV_FM_CAN */


/***********************************************
SPI 
***********************************************/
#ifdef DRV_FM_SPI
/*
1: bare-metal
*/
#ifdef SPI_1_BARE_METAL
#include "./bsp_drv/spi/spi_1_bare/fmsh_spips_hw.c"
#include "./bsp_drv/spi/spi_1_bare/fmsh_spips_sw.c"
#include "./bsp_drv/spi/spi_1_bare/fmsh_spips.c"
#include "./bsp_drv/spi/spi_1_bare/fmsh_spips_sinit.c"
#include "./bsp_drv/spi/spi_1_bare/fmsh_spips_g.c"
#include "./bsp_drv/spi/spi_1_bare/fmsh_spi_example.c"
#endif

/*
2: vx-nobus
*/
#ifdef SPI_2_VX
#include "./bsp_drv/spi/spi_2_vx/vxSpi.c"
#endif

/*
3: vxbus
*/
#ifdef SPI_3_VXB
#include "./bsp_drv/spi/spi_3_vxb/vxbSpi_fm.c"
#endif
#endif  /* #ifdef DRV_FM_SPI */


/***********************************************
I2C
***********************************************/
#ifdef INCLUDE_ALT_SOC_GEN5_DW_I2C
/*
1: bare-metal
*/
#ifdef I2C_1_BARE_METAL
#include "./bsp_drv/i2c/i2c_1_bare/fmsh_i2c_sinit.c"
#include "./bsp_drv/i2c/i2c_1_bare/fmsh_i2c_g.c"
#include "./bsp_drv/i2c/i2c_1_bare/fmsh_i2c.c"

#include "./bsp_drv/i2c/i2c_1_bare/fmsh_i2c_driver.c"
#endif

/*
2: vx-nobus
*/
#ifdef I2C_2_VX
#include "./bsp_drv/i2c/i2c_2_vx/vxI2c.c"
#endif

/*
3: vxbus
*/
#ifdef I2C_3_VXB
#include "./bsp_drv/i2c/i2c_3_vxb/vxbAltSocGen5DwI2c.c"
#endif
#endif  /* #ifdef INCLUDE_ALT_SOC_GEN5_DW_I2C */

/*#include "./bsp_drv/i2c/i2c_2_vx/vxI2c.c"*/
/*#include "./bsp_drv/i2c/i2c_3_vxb/vxbAltSocGen5DwI2c.c"*/


/***********************************************
NFC(nandflash) 
***********************************************/
#ifdef DRV_FM_NFC
/*
1: bare-metal
*/
#ifdef NFC_1_BARE_METAL
#include "./bsp_drv/nfc/nfc_1_bare/fmsh_nfcps_hw.c"
#include "./bsp_drv/nfc/nfc_1_bare/fmsh_nfcps_sw.c"
#include "./bsp_drv/nfc/nfc_1_bare/fmsh_nfcps.c"
#include "./bsp_drv/nfc/nfc_1_bare/fmsh_nfcps_flash.c"
#include "./bsp_drv/nfc/nfc_1_bare/fmsh_nfcps_bbm.c"
#include "./bsp_drv/nfc/nfc_1_bare/fmsh_nfcps_skipblock.c"
#include "./bsp_drv/nfc/nfc_1_bare/fmsh_nfcps_model.c"
#endif

/*
2: vx-nobus
*/
#ifdef NFC_2_VX
#include "./bsp_drv/nfc/nfc_2_vx/vxNfc.c"
#include "./bsp_drv/nfc/nfc_2_vx/vxNandFlash.c"
#endif

/*
3: vxbus
*/
#ifdef NFC_3_VXB
#include "./bsp_drv/nfc/nfc_3_vxb/vxbNfc_fm.c"
#include "./bsp_drv/nfc/nfc_3_vxb/vxbNandFlash_fm.c"
#endif
#endif   /* #ifdef DRV_FM_NFC */


/***********************************************
GPIO 
***********************************************/
#ifdef INCLUDE_FMSH_FMQL_GPIO
/*
1: bare-metal
*/
#ifdef GPIO_1_BARE_METAL
/* nothing*/
#endif

/*
2: vx-nobus
*/
#ifdef GPIO_2_VX
#include "./bsp_drv/gpio/gpio_2_vx/vxGpio.c"
#endif

/*
3: vxbus
*/
#ifdef GPIO_3_VXB
/* nothing*/
#endif

/*
4: legacy
*/
#ifdef GPIO_4_LEGACY
#include "./bsp_drv/gpio/gpio_4_legacy/sysGpio.c"
#include "./bsp_drv/gpio/gpio_4_legacy/gpio_drv_test.c"
#endif
#endif  /* #ifdef INCLUDE_FMSH_FMQL_GPIO */


/***********************************************
DEVC: config-PL_fpga
***********************************************/
#ifdef DRV_FM_DEVC
/*
1: bare-metal
*/
#ifdef DEVC_1_BARE_METAL
#include "./bsp_drv/devc/devc_1_bare/fmsh_devc.c"
#include "./bsp_drv/devc/devc_1_bare/fmsh_devc_g.c"
#include "./bsp_drv/devc/devc_1_bare/fmsh_devc_sinit.c"
#endif

/*
2: vx-nobus
*/
#ifdef DEVC_2_VX
#include "./bsp_drv/devc/devc_2_vx/vxDevC.c"
#endif

/*
3: vxbus
*/
#ifdef DEVC_3_VXB
/* nothing*/
#endif
#endif /* DRV_FM_DEVC */


/***********************************************
SMC: sram / norflash
***********************************************/
#ifdef DRV_FM_SMC
/*
1: bare-metal
*/
#ifdef SMC_1_BARE_METAL
/* nothing*/
#endif

/*
2: vx-nobus
*/
#ifdef SMC_2_VX
#include "./bsp_drv/smc/smc_2_vx/vxSmc.c"

/* norflash */
#ifdef SMC_2_NORFLASH
#include "./bsp_drv/smc/smc_2_vx/norflash/vxNorflash.c"
#include "./bsp_drv/smc/smc_2_vx/norflash/norflash_cfi.c"
#include "./bsp_drv/smc/smc_2_vx/norflash/norflash_amd.c"
#include "./bsp_drv/smc/smc_2_vx/norflash/norflash_intel.c"
#endif
/* sram */
#ifdef SMC_2_SRAM
/* nothing*/
#endif
#endif /* #ifdef SMC_2_VX */

/*
3: vxbus
*/
#ifdef SMC_3_VXB
/* nothing*/
#endif
#endif  /* #ifdef DRV_FM_SMC */


/***********************************************
WDT(watchdog timer)
***********************************************/
#ifdef DRV_FM_WDT
/*
1: bare-metal
*/
#ifdef WDT_1_BARE_METAL
/* nothing*/
#endif

/*
2: vx-nobus
*/
#ifdef WDT_2_VX
#include "./bsp_drv/wdt/wdt_2_vx/vxWdt.c"
#endif

/*
3: vxbus
*/
#ifdef WDT_3_VXB
#include "./bsp_drv/wdt/wdt_3_vxb/vxbAltSocGen5Wdt.c"
#endif
#endif /* #ifdef DRV_FM_WDT */


/***********************************************
GTC: General Timer Control 
***********************************************/
#ifdef INCLUDE_DRV_FM_GTC
/*
1: bare-metal
*/
#ifdef GTC_1_BARE_METAL 
/* nothing*/
#endif

/*
2: vx-nobus
*/
#ifdef GTC_2_VX
#include "./bsp_drv/timer/gtc_2_vx/vxGtc.c"
#endif

/*
3: vxbus
*/
#ifdef GTC_3_VXB
#include "./bsp_drv/timer/gtc_3_vxb/vxbArmv7GenTimer.c"
#endif
#endif  /* #ifdef INCLUDE_DRV_FM_GTC */



/***********************************************
USB host: 
	D:\WindRiver69_ARM\vxworks-6.9\target
	  \src\hwif\busCtlr\usb\hcd\synopsyshcd
***********************************************/
#ifdef INCLUDE_SYNOPSYSHCI
/*
1: bare-metal
*/
#ifdef USB_1_BARE_METAL
/* nothing*/
#endif

/*
2: vx-nobus
*/
#ifdef USB_2_VX
/* nothing*/
#endif

/*
3: vxbus
*/
#ifdef USB_3_VXB
#include "./bsp_drv/usb/usb_3_vxb/usbPciStub.c"
#include "./bsp_drv/usb/usb_3_vxb/usbSynopsysHcdEventHandler.c"
#include "./bsp_drv/usb/usb_3_vxb/usbSynopsysHcdHardwareAccess.c"
#include "./bsp_drv/usb/usb_3_vxb/usbSynopsysHcdInitExit.c"
#include "./bsp_drv/usb/usb_3_vxb/usbSynopsysHcdRhEmulation.c"
#include "./bsp_drv/usb/usb_3_vxb/usbSynopsysHcdTransferManagement.c"
#include "./bsp_drv/usb/usb_3_vxb/usbSynopsysHcdUtil.c"
#endif
#endif /* #ifdef INCLUDE_SYNOPSYSHCI */




/***********************************************
test PL bit functions
***********************************************/
#ifdef PL_PFGA_RUN  /* all test PL functions */
/***********************************************/

/*
 1: xadc
*/
#ifdef PL_XADC
#include "./bsp_drv/pl/xadc/test_xadc.c"
#endif


/*
 2: axi_dma
*/
#ifdef PL_AXI_DMA
#include "./bsp_drv/pl/axi_dma/axidma_1_bare/axidma_v9_9/xaxidma_g.c"
#include "./bsp_drv/pl/axi_dma/axidma_1_bare/axidma_v9_9/xaxidma_sinit.c"
#include "./bsp_drv/pl/axi_dma/axidma_1_bare/axidma_v9_9/xaxidma_bdring.c"
#include "./bsp_drv/pl/axi_dma/axidma_1_bare/axidma_v9_9/xaxidma_bd.c"
#include "./bsp_drv/pl/axi_dma/axidma_1_bare/axidma_v9_9/xaxidma.c"
#include "./bsp_drv/pl/axi_dma/axidma_1_bare/axidma_v9_9/xaxidma_selftest.c"

#include "./bsp_drv/pl/axi_dma/axidma_1_bare/fmsh_axidma.c"
#endif  /* #ifdef PL_AXI_DMA*/


/*
 3: vdma_hdmi
*/
#ifdef PL_VDMA_HDMI
/**/
/* hdmi_lib*/
/**/
/* axi_gpio*/
#include "./bsp_drv/pl/vdma_hdmi/hdmi_lib/gpio_v4_3/xgpio_g.c"
#include "./bsp_drv/pl/vdma_hdmi/hdmi_lib/gpio_v4_3/xgpio_sinit.c"
#include "./bsp_drv/pl/vdma_hdmi/hdmi_lib/gpio_v4_3/xgpio_intr.c"
#include "./bsp_drv/pl/vdma_hdmi/hdmi_lib/gpio_v4_3/xgpio_extra.c"
#include "./bsp_drv/pl/vdma_hdmi/hdmi_lib/gpio_v4_3/xgpio.c"
#include "./bsp_drv/pl/vdma_hdmi/hdmi_lib/gpio_v4_3/xgpio_selftest.c"
/* vtc*/
#include "./bsp_drv/pl/vdma_hdmi/hdmi_lib/vtc_v7_2/xvtc_g.c"
#include "./bsp_drv/pl/vdma_hdmi/hdmi_lib/vtc_v7_2/xvtc_sinit.c"
#include "./bsp_drv/pl/vdma_hdmi/hdmi_lib/vtc_v7_2/xvtc_intr.c"
#include "./bsp_drv/pl/vdma_hdmi/hdmi_lib/vtc_v7_2/xvtc.c"
#include "./bsp_drv/pl/vdma_hdmi/hdmi_lib/vtc_v7_2/xvtc_selftest.c"
/* vdma*/
#include "./bsp_drv/pl/vdma_hdmi/hdmi_lib/axivdma_v6_4/xaxivdma_g.c"
#include "./bsp_drv/pl/vdma_hdmi/hdmi_lib/axivdma_v6_4/xaxivdma_sinit.c"
#include "./bsp_drv/pl/vdma_hdmi/hdmi_lib/axivdma_v6_4/xaxivdma_intr.c"
#include "./bsp_drv/pl/vdma_hdmi/hdmi_lib/axivdma_v6_4/xaxivdma_channel.c"
#include "./bsp_drv/pl/vdma_hdmi/hdmi_lib/axivdma_v6_4/xaxivdma.c"
#include "./bsp_drv/pl/vdma_hdmi/hdmi_lib/axivdma_v6_4/xaxivdma_selftest.c"

/**/
/* hdmi_app*/
/**/
/* hdmi_i2c*/
#if 1
#include "./bsp_drv/pl/vdma_hdmi/hdmi_app/i2c/i2cps_v1_0/fmsh_i2c_g.c"
#include "./bsp_drv/pl/vdma_hdmi/hdmi_app/i2c/i2cps_v1_0/fmsh_i2c_sinit.c"
#include "./bsp_drv/pl/vdma_hdmi/hdmi_app/i2c/i2cps_v1_0/fmsh_i2c.c"

#include "./bsp_drv/pl/vdma_hdmi/hdmi_app/i2c/PS_i2c.c"
#endif

#if 1
/* hdmi_dynclk*/
#include "./bsp_drv/pl/vdma_hdmi/hdmi_app/dynclk/dynclk.c"
/* display_ctrl*/
#include "./bsp_drv/pl/vdma_hdmi/hdmi_app/display_ctrl/display_ctrl.c"

/* jpg-to-bmp transfer functions*/
#include "./bsp_drv/pl/vdma_hdmi/jpg_to_bmp.c"

/* app: test_hdmi*/
#include "./bsp_drv/pl/vdma_hdmi/test_hdmi.c"
#endif
#endif /* #ifdef PL_VDMA_HDMI*/


/*
 4: pl_rs422
*/
#ifdef PL_RS422

#if 1  /* demo for pl_irq*/
#include "./bsp_drv/pl/axi_rs422/pl_rs422.c"
#endif

#if 0  /* pl_rs422 for bf-xian*/
#include "./bsp_drv/pl/axi_rs422/ref_bf/pl_rs422.c"
#endif

#endif  /* #ifdef PL_RS422 */


/*
 5: pl_pcie_rc
*/
#ifdef PL_PCIE_RC
#if 0  /* pl_pcie_rc: ref_qsdz*/
#include "./bsp_drv/pl/pcie_rc/ref_qs/PCIE_hw.c"
#include "./bsp_drv/pl/pcie_rc/ref_qs/pcie.c"
#endif

#if 0  /* pl_pcie_rc: ref_sz*/
#include "./bsp_drv/pl/pcie_rc/ref_sz/axipcie_v3_1/xaxipcie_g.c"
#include "./bsp_drv/pl/pcie_rc/ref_sz/axipcie_v3_1/xaxipcie_sinit.c"
#include "./bsp_drv/pl/pcie_rc/ref_sz/axipcie_v3_1/xaxipcie_intr.c"
#include "./bsp_drv/pl/pcie_rc/ref_sz/axipcie_v3_1/xaxipcie.c"

#include "./bsp_drv/pl/pcie_rc/ref_sz/pcie_gen2_enumerate.c"
#endif

#if 1  /* pl_pcie_rc: ref_613*/
#include "./bsp_drv/pl/pcie_rc/ref_613/axipcie_v3_1/xaxipcie_g.c"
#include "./bsp_drv/pl/pcie_rc/ref_613/axipcie_v3_1/xaxipcie_sinit.c"
#include "./bsp_drv/pl/pcie_rc/ref_613/axipcie_v3_1/xaxipcie_intr.c"
#include "./bsp_drv/pl/pcie_rc/ref_613/axipcie_v3_1/xaxipcie.c"

/*#include "./bsp_drv/pl/pcie_rc/ref_613/xaxipcie_rc_enumerate_example.c"*/
#include "./bsp_drv/pl/pcie_rc/ref_613/pcie_gen2_enumerate.c"
#endif


#ifdef PL_PCIE_RC_VXB /* base vxbus & vx69_lib*/
#include "./bsp_drv/pl/pcie_rc/pcie_vxb/vxbArmPlAxiPci.c"
#endif

/**/
/*  pcie_show routines*/
/**/
#if 0  
#include "./bsp_drv/pl/pcie_rc/pcie_show/vxbPciShow2.c"
#endif

/**/
/*  pcie_scan&show routines*/
/**/
#if 1  /* ref_dsp: base-vx7: ep, root, bridge is ok*/
#include "./bsp_drv/pl/pcie_rc/pcie_scan3/vxbPciAutoCfg3.c"
#include "./bsp_drv/pl/pcie_rc/pcie_scan3/vxbPci3.c"

#include "./bsp_drv/pl/pcie_rc/pcie_scan3/vxbPciMethod3.c"
#include "./bsp_drv/pl/pcie_rc/pcie_scan3/vxbPciShow3.c"
#endif

#if 0  /* ref_vx7: endpoint & root_only is ok*/
#include "./bsp_drv/pl/pcie_rc/pcie_scan2/vxbPciAutoCfg.c"
#include "./bsp_drv/pl/pcie_rc/pcie_scan2/vxbPci.c"

#include "./bsp_drv/pl/pcie_rc/pcie_scan2/vxbPciMethod.c"
#include "./bsp_drv/pl/pcie_rc/pcie_scan2/vxbPciShow.c"
#endif

#if 1  /* ref_vx69-ft:base-linux*/
#include "./bsp_drv/pl/pcie_rc/pcie_scan1/pcieInit.c"
#include "./bsp_drv/pl/pcie_rc/pcie_scan1/pciConfigLib.c"
#include "./bsp_drv/pl/pcie_rc/pcie_scan1/pcieAutoConfig.c"
#endif

#endif /* #ifdef PL_PCIE_RC */


#if 1  /* test for cd-lingya - ok */

#ifdef PL_AXI_GPIO
#include "./bsp_drv/pl/ref_lingya/gpio_v4_3/xgpio_g.c"
#include "./bsp_drv/pl/ref_lingya/gpio_v4_3/xgpio_sinit.c"
#include "./bsp_drv/pl/ref_lingya/gpio_v4_3/xgpio_extra.c"
#include "./bsp_drv/pl/ref_lingya/gpio_v4_3/xgpio_intr.c"
#include "./bsp_drv/pl/ref_lingya/gpio_v4_3/xgpio.c"
#include "./bsp_drv/pl/ref_lingya/gpio_v4_3/xgpio_selftest.c"
#endif

#ifdef PL_AXI_SPI
#include "./bsp_drv/pl/ref_lingya/spi_v4_4/xspi_g.c"
#include "./bsp_drv/pl/ref_lingya/spi_v4_4/xspi_sinit.c"
#include "./bsp_drv/pl/ref_lingya/spi_v4_4/xspi_stats.c"
#include "./bsp_drv/pl/ref_lingya/spi_v4_4/xspi_options.c"
#include "./bsp_drv/pl/ref_lingya/spi_v4_4/xspi.c"
#include "./bsp_drv/pl/ref_lingya/spi_v4_4/xspi_selftest.c"
#endif

#ifdef PL_AXI_BRAM
#include "./bsp_drv/pl/ref_lingya/bram_v4_2/xbram_g.c"
#include "./bsp_drv/pl/ref_lingya/bram_v4_2/xbram_sinit.c"
#include "./bsp_drv/pl/ref_lingya/bram_v4_2/xbram_intr.c"
#include "./bsp_drv/pl/ref_lingya/bram_v4_2/xbram.c"
#include "./bsp_drv/pl/ref_lingya/bram_v4_2/xbram_selftest.c"
#endif



#include "./bsp_drv/pl/ref_lingya/rfsoc.c"
#include "./bsp_drv/pl/ref_lingya/helloworld.c"
#include "./bsp_drv/pl/ref_lingya/xbram_example.c"
#endif  

#ifdef INCLUDE_AXI_ETHERNET_END
#include "./bsp_drv/pl/axi_net/vxbFdtFmqlAxiEnd.c"
#endif

/***********************************************/
#endif  /* #ifdef PL_PFGA_RUN */




/***********************************************
Ramdisk for vxworks 
(some ram used as disk)
***********************************************/
#ifdef DRV_FM_RAMDISK_VX
#include "./bsp_drv/ramdisk/ramdisk_vx.c"
#endif


/***********************************************
test c++ for vxworks 
***********************************************/
#if 0
#include "./ztestcpp/testc.c"
#include "./ztestcpp/testcpp.cpp"
#endif



/*
================================================
test_all:
1. test code for all device interface
2. one fold for one deivce
3. test_all is the entry for all test
================================================
*/
#if 0  /* test_all */

/* 1_can*/
#if 1
#include "./app_test/1_can/test_can.c"
#endif

/* 2_i2c*/
#if 1
#include "./app_test/2_i2c/test_i2c.c"
#endif

/* 3_spi*/
#if 1
#include "./app_test/3_spi/test_spi.c"
#endif

/* 4_qspi*/
#if 1
#include "./app_test/4_qspi/test_qspi.c"
#endif

/* 5_sdmmc*/
#if 1
#include "./app_test/5_sdmmc/test_sdmmc.c"
#endif

/* 6_nfc*/
#if 1
#include "./app_test/6_nfc/test_nfc.c"
#endif

/* 7_gpio*/
#if 0
#include "./app_test/7_gpio/test_gpio.c"
#endif

/* 8_gmac*/
#if 1
#include "./app_test/8_gmac/test_gmac.c"
#endif


/* test_all*/
#if 1
#include "./app_test/test_all.c"
#endif
#endif  /* test_all */

#endif  /* #ifndef BUILD_VIP_MIN_SYSTEM */

#endif /* __INC_SYSINC_ALL_H */


