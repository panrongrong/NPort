/* vxbAltSocGen5DwMshc.h - Altera SoC DesignWare MSHC driver header */

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
01a, 15Aug19, jc  written.
*/

#ifndef __INCvxbAltSocGen5DwMshch
#define __INCvxbAltSocGen5DwMshch

#include <vxWorks.h>
#include <hwif/vxbus/vxBus.h>

#if 0  /* lib h-file - ok*/
#include <../h/storage/vxbSdMmcLib.h>
#else  /* bsp h-file for test*/
#include "vxbSdMmcLib.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* defines */

/* registers */

#define FM_SDMMC_CTRL                   (0x000)
#define FM_SDMMC_PWREN                  (0x004)
#define FM_SDMMC_CLKDIV                 (0x008)
#define FM_SDMMC_CLKSRC                 (0x00c)
#define FM_SDMMC_CLKENA                 (0x010)
#define FM_SDMMC_TMOUT                  (0x014)
#define FM_SDMMC_CTYPE                  (0x018)
#define FM_SDMMC_BLKSIZ                 (0x01c)
#define FM_SDMMC_BYTCNT                 (0x020)
#define FM_SDMMC_INTMASK                (0x024)
#define FM_SDMMC_CMDARG                 (0x028)
#define FM_SDMMC_CMD                    (0x02c)
#define FM_SDMMC_RESP0                  (0x030)
#define FM_SDMMC_RESP1                  (0x034)
#define FM_SDMMC_RESP2                  (0x038)
#define FM_SDMMC_RESP3                  (0x03c)
#define FM_SDMMC_MINTSTS                (0x040)
#define FM_SDMMC_RINTSTS                (0x044)
#define FM_SDMMC_STATUS                 (0x048)
#define FM_SDMMC_FIFOTH                 (0x04c)
#define FM_SDMMC_CDETECT                (0x050)
#define FM_SDMMC_WRTPRT                 (0x054)
#define FM_SDMMC_GPIO                   (0x058)
#define FM_SDMMC_TCBCNT                 (0x05c)
#define FM_SDMMC_TBBCNT                 (0x060)
#define FM_SDMMC_DEBNCE                 (0x064)
#define FM_SDMMC_USRID                  (0x068)
#define FM_SDMMC_VERID                  (0x06c)
#define FM_SDMMC_HCON                   (0x070)
#define FM_SDMMC_UHS_REG                (0x074)
#define FM_SDMMC_BMOD                   (0x080)

#define FM_SDMMC_PLDMND_84H                 (0x084)  /* */
#define FM_SDMMC_DBADDR_88H                 (0x088)  /* */
#define FM_SDMMC_IDSTS_8CH                  (0x08c)  /* */
#define FM_SDMMC_IDINTEN_90H                (0x090)  /* */

#define FM_SDMMC_DSCADDR                (0x094)
#define FM_SDMMC_BUFADDR                (0x098)

#define FM_SDMMC_CARDTHRCTL             (0x100)
#define FM_SDMMC_BACK_END_POWER_R       (0x104)

/* version lower than 2.40a is 0x100, alt soc gen5 is 2.40a */
#if 0 /* soc 5 */
/*
data on page 15-142 0x200 32 RW 0x0 Data FIFO Access
*/
#define FM_SDMMC_DATA_FIFO                   (0x200)
#else
/* 
DATA	>= 0x100	R/W	数据FIFO读/写 Reset Value: 32’hx 
*/
#define FM_SDMMC_DATA_FIFO                   (0x100)
#endif

/* register bits */

/* control */

#define FM_SDMMC_CTRL_USE_IDMAC         (1 << 25)
#define FM_SDMMC_CTRL_CEATA_INT_STATUS  (1 << 11)
#define FM_SDMMC_CTRL_SEND_ASTOP_CCSD   (1 << 10)
#define FM_SDMMC_CTRL_SEND_CCSD         (1 << 9)
#define FM_SDMMC_CTRL_ABORT_READ_DATA   (1 << 8)
#define FM_SDMMC_CTRL_SEND_IRQ_RESPONSE (1 << 7)
#define FM_SDMMC_CTRL_READ_WAIT         (1 << 6)
#define FM_SDMMC_CTRL_DMA_ENABLE        (1 << 5)
#define FM_SDMMC_CTRL_INT_ENABLE        (1 << 4)
#define FM_SDMMC_CTRL_DMA_RESET         (1 << 2)
#define FM_SDMMC_CTRL_FIFO_RESET        (1 << 1)
#define FM_SDMMC_CTRL_RESET             (1 << 0)

/* clock enable */

#define FM_SDMMC_CLKENA_CCLK_LOW_PWR    (1 << 16)
#define FM_SDMMC_CLKENA_CCLK_ENABLE     (1 << 0)

/* time out */

#define FM_SDMMC_TMOUT_DATA(time)       ((time) & 0xffffff) << 8)
#define FM_SDMMC_TMOUT_RESPONSE(time)   ((time) & 0xff)

/* card type */

#define FM_SDMMC_CTYPE_8BIT             (1 << 16)
#define FM_SDMMC_CTYPE_4BIT             (1 << 0)
#define FM_SDMMC_CTYPE_1BIT             (0)

/* interrupt status & mask */

#define FM_SDMMC_INT_SDIO               (1 << 16)
#define FM_SDMMC_INT_EBE                (1 << 15)
#define FM_SDMMC_INT_ACD                (1 << 14)
#define FM_SDMMC_INT_SBE                (1 << 13)
#define FM_SDMMC_INT_HLE                (1 << 12)
#define FM_SDMMC_INT_FRUN               (1 << 11)
#define FM_SDMMC_INT_HTO                (1 << 10)
#define FM_SDMMC_INT_DRT                (1 << 9)
#define FM_SDMMC_INT_RTO                (1 << 8)
#define FM_SDMMC_INT_DCRC               (1 << 7)
#define FM_SDMMC_INT_RCRC               (1 << 6)
#define FM_SDMMC_INT_RXDR               (1 << 5)
#define FM_SDMMC_INT_TXDR               (1 << 4)
#define FM_SDMMC_INT_DTO                (1 << 3)
#define FM_SDMMC_INT_CMD_DONE           (1 << 2)
#define FM_SDMMC_INT_RE                 (1 << 1)
#define FM_SDMMC_INT_CD                 (1 << 0)

#define FM_SDMMC_INT_CMD_ERROR          (FM_SDMMC_INT_RE | FM_SDMMC_INT_RCRC | \
                                         FM_SDMMC_INT_RTO)
                                         
#define FM_SDMMC_INT_DATA_ERROR         (FM_SDMMC_INT_DRT | FM_SDMMC_INT_EBE | \
                                         FM_SDMMC_INT_SBE | FM_SDMMC_INT_DCRC)
                                         
#define FM_SDMMC_INT_ERROR              (FM_SDMMC_INT_CMD_ERROR              | \
                                         FM_SDMMC_INT_DATA_ERROR             | \
                                         FM_SDMMC_INT_HLE)

#define FM_SDMMC_INT_PIO_DEFAULTS       (FM_SDMMC_INT_RXDR | FM_SDMMC_INT_CD | \
                                         FM_SDMMC_INT_DTO | FM_SDMMC_INT_CMD_DONE | \
                                         FM_SDMMC_INT_ERROR | FM_SDMMC_INT_TXDR)
                                         
#define FM_SDMMC_INT_DMA_DEFAULTS       (FM_SDMMC_INT_CMD_DONE | FM_SDMMC_INT_DTO | \
                                         FM_SDMMC_INT_CD |  FM_SDMMC_INT_ERROR)

/* bmod */

#define FM_SDMMC_BMOD_SWR               (1 << 0)
#define FM_SDMMC_BMOD_FB                (1 << 1)
#define FM_SDMMC_BMOD_DSL               (1 << 2)
#define FM_SDMMC_BMOD_DE                (1 << 7)
#define FM_SDMMC_BMOD_PBL               (1 << 8)

/* command */

#define FM_SDMMC_CMD_START              (1 << 31)
#define FM_SDMMC_CMD_USE_HOLD_REG       (1 << 29)
#define FM_SDMMC_CMD_CCS_EXP            (1 << 23)
#define FM_SDMMC_CMD_CEATA_RD           (1 << 22)
#define FM_SDMMC_CMD_UPD_CLK            (1 << 21)
#define FM_SDMMC_CMD_INIT               (1 << 15)
#define FM_SDMMC_CMD_STOP               (1 << 14)
#define FM_SDMMC_CMD_PRV_DAT_WAIT       (1 << 13)
#define FM_SDMMC_CMD_SEND_AUTO_STOP     (1 << 12)
#define FM_SDMMC_CMD_STREAM_MODE        (1 << 11)
#define FM_SDMMC_CMD_DATA_WR            (1 << 10)
#define FM_SDMMC_CMD_DATA_EXP           (1 << 9)
#define FM_SDMMC_CMD_RESP_CRC           (1 << 8)
#define FM_SDMMC_CMD_RESP_LONG          (1 << 7)
#define FM_SDMMC_CMD_RESP_EXP           (1 << 6)
#define FM_SDMMC_CMD_INDX(index)        ((index) & 0x3f)

/* status */

#define FM_SDMMC_STATUS_FIFO_COUNT(reg) (((reg) >> 17) & 0x1fff)
#define FM_SDMMC_STATUS_DATA3_PRESENT   (1 << 8)
#define FM_SDMMC_STATUS_FIFO_FULL       (1 << 3)
#define FM_SDMMC_STATUS_FIFO_EMPTY      (1 << 2)

/* internal DMA interrupt */

#define FM_SDMMC_IDMA_INT_AI            (1 << 9)
#define FM_SDMMC_IDMA_INT_NI            (1 << 8)
#define FM_SDMMC_IDMA_INT_CES           (1 << 5)
#define FM_SDMMC_IDMA_INT_DU            (1 << 4)
#define FM_SDMMC_IDMA_INT_FBE           (1 << 2)
#define FM_SDMMC_IDMA_INT_RI            (1 << 1)
#define FM_SDMMC_IDMA_INT_TI            (1 << 0)

#define FM_SDMMC_DEFAULT_FIFO_LEN       (16)  /* 1024 -> 16 */

#define FM_SDMMC_DEFAULT_FIFO_TX_WMARK  (FM_SDMMC_DEFAULT_FIFO_LEN / 2)
#define FM_SDMMC_DEFAULT_FIFO_RX_WMARK  (FM_SDMMC_DEFAULT_FIFO_LEN / 2 - 1)

#define FM_SDMMC_DEFAULT_FIFO_MSIZE     (0x2)
#define FM_SDMMC_DEFAULT_FIFO_THRESH    ((FM_SDMMC_DEFAULT_FIFO_TX_WMARK)    | \
                                         (FM_SDMMC_DEFAULT_FIFO_MSIZE << 28) | \
                                         (FM_SDMMC_DEFAULT_FIFO_RX_WMARK << 16))
                                         
#define FM_SDMMC_FIFO_STEPS             (4)
#define FM_SDMMC_DEFAULT_RESP_TIMEOUT   (0x64)
#define FM_SDMMC_DEFAULT_DATA_TIMEOUT   (0xffffff)

#if 1
/*
0x14:
bit15~8: data_timeout: 0xFF
bit7~0 : response_timeout: 0x40
*/
#define FM_SDMMC_DEFAULT_TIMEOUT        (FM_SDMMC_DEFAULT_RESP_TIMEOUT | \
                                         FM_SDMMC_DEFAULT_DATA_TIMEOUT << 8)
#else
#define FM_SDMMC_DEFAULT_TIMEOUT        (0xFFFFFFFF)
#endif

#define FM_SDMMC_MAX_RETRIES            (50000)  /* 50000 -> 100000 */

/* max size is 8191, but here keep it multiple of 512 */
#define FM_SDMMC_IDMA_BUFFER_SIZE       (7680)
/* #define FM_SDMMC_MAX_RW_SECTORS         (FM_SDMMC_IDMA_BUFFER_SIZE * 2)  */
#define FM_SDMMC_MAX_RW_SECTORS         (1)

/* 
 * Dual buffer mode, SDMMC_BLOCK_SIZE descriptors.
 * (FM_SDMMC_MAX_RW_SECTORS/(2*FM_SDMMC_IDMA_BUFFER_SIZE)) * SDMMC_BLOCK_SIZE
 */

#define FM_SDMMC_IDMA_DESC_NUM          (SDMMC_BLOCK_SIZE)

#define FM_SDMMC_IDMA_DES0_DIC          (1 << 1)
#define FM_SDMMC_IDMA_DES0_LD           (1 << 2)
#define FM_SDMMC_IDMA_DES0_FD           (1 << 3)
#define FM_SDMMC_IDMA_DES0_CH           (1 << 4)
#define FM_SDMMC_IDMA_DES0_ER           (1 << 5)
#define FM_SDMMC_IDMA_DES0_CES          (1 << 30)
#define FM_SDMMC_IDMA_DES0_OWN          (1 << 31)
#define FM_SDMMC_IDMA_DES1_BS1(size)    (size & 0x1fff)
#define FM_SDMMC_IDMA_DES1_BS2(size)    ((size & 0x1fff) << 13)
#define FM_SDMMC_IDMA_DES1(size1, size2)(FM_SDMMC_IDMA_DES1_BS1 (size1) | \
                                         FM_SDMMC_IDMA_DES1_BS2 (size2))

/* various feature flags */

/* snoop enabled, no need to cache flush or invalidate */
#define FM_SDMMC_FLAGS_SNOOP_ENABLED    (1 << 0)

/* no card detection, card always present, no hot plug support */
#define FM_SDMMC_FLAGS_CARD_PRESENT     (1 << 1)

/* no card write protect pin connected, always writable */
#define FM_SDMMC_FLAGS_CARD_WRITABLE    (1 << 2)

/* driver name */
/* #define FM_SDMMC_NAME                "altSocGen5DwMshc"  */
#define FM_SDMMC_NAME                  "fmSdmmc"
#define ALT_DW_MSHC_NAME                "altSocGen5DwMshc"


/* card monitor task name */
#define FM_SDMMC_CARD_MON_TASK_NAME  "mshcCardMon"

/* card monitor task priority */
#define FM_SDMMC_CARD_MON_TASK_PRI   (100)

/* card monitor task stack size */
#define FM_SDMMC_CARD_MON_TASK_STACK (8192)

/* seconds to wait for command or data done */
#define FM_SDMMC_CMD_WAIT_IN_SECS    (20)  /* 10 */

/* card monitor task check status change delay */
#define FM_SDMMC_CARDMON_DELAY_SECS  (4)  /* 2 */

/* RW mode */
#define FM_SDMMC_RW_MODE_DMA         (0)
#define FM_SDMMC_RW_MODE_PIO         (1)
#define FM_SDMMC_RW_MODE_DEFAULT     (FM_SDMMC_RW_MODE_PIO)

/* typdefs */

typedef struct fmSdmmcIdmaDesc
{
    UINT32              des0;   /* control descriptor */
    UINT32              des1;   /* buffer sizes */
    UINT32              des2;   /* buffer 1 physical address */
    UINT32              des3;   /* buffer 2 physical address */
} FM_SDMMC_IDMA_DESC;

typedef struct fmSdmmcDrvCtrl
    {
    VXB_DEVICE_ID       pDev;
	
    void *              regBase;
    void *              regHandle;
	
    UINT32              clkFreq;
    FUNCPTR             clkFreqSetup;
	
    SEM_ID              devChange;
    SEM_ID              cmdDone;
    SEM_ID              dataDone;
	
    BOOL                polling;
    BOOL                cardIns;
    UINT32              rwMode;
    UINT32              intMask;
    UINT32              flags;
    UINT32              remainBytes;
	
    FM_SDMMC_IDMA_DESC * idmaDesc;
	
    SDMMC_HOST          host;
    SDMMC_CARD          card;
    } FM_SDMMC_DRV_CTRL;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INCvxbAltSocGen5DwMshch */

