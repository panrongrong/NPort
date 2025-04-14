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
01a,24may13,sye  written.
*/

#ifndef __INCvxbAltSocGen5DwMshch
#define __INCvxbAltSocGen5DwMshch

#include <vxWorks.h>
#include <hwif/vxbus/vxBus.h>
#include <../h/storage/vxbSdMmcLib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* defines */

/* registers */

#define ALT_MSHC_CTRL                   (0x000)
#define ALT_MSHC_PWREN                  (0x004)
#define ALT_MSHC_CLKDIV                 (0x008)
#define ALT_MSHC_CLKSRC                 (0x00c)
#define ALT_MSHC_CLKENA                 (0x010)
#define ALT_MSHC_TMOUT                  (0x014)
#define ALT_MSHC_CTYPE                  (0x018)
#define ALT_MSHC_BLKSIZ                 (0x01c)
#define ALT_MSHC_BYTCNT                 (0x020)
#define ALT_MSHC_INTMASK                (0x024)
#define ALT_MSHC_CMDARG                 (0x028)
#define ALT_MSHC_CMD                    (0x02c)
#define ALT_MSHC_RESP0                  (0x030)
#define ALT_MSHC_RESP1                  (0x034)
#define ALT_MSHC_RESP2                  (0x038)
#define ALT_MSHC_RESP3                  (0x03c)
#define ALT_MSHC_MINTSTS                (0x040)
#define ALT_MSHC_RINTSTS                (0x044)
#define ALT_MSHC_STATUS                 (0x048)
#define ALT_MSHC_FIFOTH                 (0x04c)
#define ALT_MSHC_CDETECT                (0x050)
#define ALT_MSHC_WRTPRT                 (0x054)
#define ALT_MSHC_GPIO                   (0x058)
#define ALT_MSHC_TCBCNT                 (0x05c)
#define ALT_MSHC_TBBCNT                 (0x060)
#define ALT_MSHC_DEBNCE                 (0x064)
#define ALT_MSHC_USRID                  (0x068)
#define ALT_MSHC_VERID                  (0x06c)
#define ALT_MSHC_HCON                   (0x070)
#define ALT_MSHC_UHS_REG                (0x074)
#define ALT_MSHC_BMOD                   (0x080)
#define ALT_MSHC_PLDMND                 (0x084)
#define ALT_MSHC_DBADDR                 (0x088)
#define ALT_MSHC_IDSTS                  (0x08c)
#define ALT_MSHC_IDINTEN                (0x090)
#define ALT_MSHC_DSCADDR                (0x094)
#define ALT_MSHC_BUFADDR                (0x098)

#define ALT_MSHC_CARDTHRCTL             (0x100)
#define ALT_MSHC_BACK_END_POWER_R       (0x104)

/* 
version lower than 2.40a is 0x100, alt soc gen5 is 2.40a 
*/
#define ALT_MSHC_DATA                   (0x200)

/* register bits */

/* control */

#define ALT_MSHC_CTRL_USE_IDMAC         (1 << 25)
#define ALT_MSHC_CTRL_CEATA_INT_STATUS  (1 << 11)
#define ALT_MSHC_CTRL_SEND_ASTOP_CCSD   (1 << 10)
#define ALT_MSHC_CTRL_SEND_CCSD         (1 << 9)
#define ALT_MSHC_CTRL_ABORT_READ_DATA   (1 << 8)
#define ALT_MSHC_CTRL_SEND_IRQ_RESPONSE (1 << 7)
#define ALT_MSHC_CTRL_READ_WAIT         (1 << 6)
#define ALT_MSHC_CTRL_DMA_ENABLE        (1 << 5)
#define ALT_MSHC_CTRL_INT_ENABLE        (1 << 4)
#define ALT_MSHC_CTRL_DMA_RESET         (1 << 2)
#define ALT_MSHC_CTRL_FIFO_RESET        (1 << 1)
#define ALT_MSHC_CTRL_RESET             (1 << 0)

/* clock enable */

#define ALT_MSHC_CLKENA_CCLK_LOW_PWR    (1 << 16)
#define ALT_MSHC_CLKENA_CCLK_ENABLE     (1 << 0)

/* time out */

#define ALT_MSHC_TMOUT_DATA(time)       ((time) & 0xffffff) << 8)
#define ALT_MSHC_TMOUT_RESPONSE(time)   ((time) & 0xff)

/* card type */

#define ALT_MSHC_CTYPE_8BIT             (1 << 16)
#define ALT_MSHC_CTYPE_4BIT             (1 << 0)
#define ALT_MSHC_CTYPE_1BIT             (0)

/* interrupt status & mask */

#define ALT_MSHC_INT_SDIO               (1 << 16)
#define ALT_MSHC_INT_EBE                (1 << 15)
#define ALT_MSHC_INT_ACD                (1 << 14)
#define ALT_MSHC_INT_SBE                (1 << 13)
#define ALT_MSHC_INT_HLE                (1 << 12)
#define ALT_MSHC_INT_FRUN               (1 << 11)
#define ALT_MSHC_INT_HTO                (1 << 10)
#define ALT_MSHC_INT_DRT                (1 << 9)
#define ALT_MSHC_INT_RTO                (1 << 8)
#define ALT_MSHC_INT_DCRC               (1 << 7)
#define ALT_MSHC_INT_RCRC               (1 << 6)
#define ALT_MSHC_INT_RXDR               (1 << 5)
#define ALT_MSHC_INT_TXDR               (1 << 4)
#define ALT_MSHC_INT_DTO                (1 << 3)
#define ALT_MSHC_INT_CMD                (1 << 2)
#define ALT_MSHC_INT_RE                 (1 << 1)
#define ALT_MSHC_INT_CD                 (1 << 0)

#define ALT_MSHC_INT_CMD_ERROR          (ALT_MSHC_INT_RE | ALT_MSHC_INT_RCRC | \
                                         ALT_MSHC_INT_RTO)
#define ALT_MSHC_INT_DATA_ERROR         (ALT_MSHC_INT_DRT | ALT_MSHC_INT_EBE | \
                                         ALT_MSHC_INT_SBE | ALT_MSHC_INT_DCRC)
#define ALT_MSHC_INT_ERROR              (ALT_MSHC_INT_CMD_ERROR              | \
                                         ALT_MSHC_INT_DATA_ERROR             | \
                                         ALT_MSHC_INT_HLE)

#define ALT_MSHC_INT_PIO_DEFAULTS       (ALT_MSHC_INT_RXDR | ALT_MSHC_INT_CD | \
                                         ALT_MSHC_INT_DTO | ALT_MSHC_INT_CMD | \
                                         ALT_MSHC_INT_ERROR | ALT_MSHC_INT_TXDR)
#define ALT_MSHC_INT_DMA_DEFAULTS       (ALT_MSHC_INT_CMD | ALT_MSHC_INT_DTO | \
                                         ALT_MSHC_INT_CD |  ALT_MSHC_INT_ERROR)

/* bmod */

#define ALT_MSHC_BMOD_SWR               (1 << 0)
#define ALT_MSHC_BMOD_FB                (1 << 1)
#define ALT_MSHC_BMOD_DSL               (1 << 2)
#define ALT_MSHC_BMOD_DE                (1 << 7)
#define ALT_MSHC_BMOD_PBL               (1 << 8)

/* command */

#define ALT_MSHC_CMD_START              (1 << 31)
#define ALT_MSHC_CMD_USE_HOLD_REG       (1 << 29)
#define ALT_MSHC_CMD_CCS_EXP            (1 << 23)
#define ALT_MSHC_CMD_CEATA_RD           (1 << 22)
#define ALT_MSHC_CMD_UPD_CLK            (1 << 21)
#define ALT_MSHC_CMD_INIT               (1 << 15)
#define ALT_MSHC_CMD_STOP               (1 << 14)
#define ALT_MSHC_CMD_PRV_DAT_WAIT       (1 << 13)
#define ALT_MSHC_CMD_SEND_AUTO_STOP     (1 << 12)
#define ALT_MSHC_CMD_STREAM_MODE        (1 << 11)
#define ALT_MSHC_CMD_DATA_WR            (1 << 10)
#define ALT_MSHC_CMD_DATA_EXP           (1 << 9)
#define ALT_MSHC_CMD_RESP_CRC           (1 << 8)
#define ALT_MSHC_CMD_RESP_LONG          (1 << 7)
#define ALT_MSHC_CMD_RESP_EXP           (1 << 6)
#define ALT_MSHC_CMD_INDX(index)        ((index) & 0x3f)

/* status */

#define ALT_MSHC_STATUS_FIFO_COUNT(reg) (((reg) >> 17) & 0x1fff)
#define ALT_MSHC_STATUS_DATA3_PRESENT   (1 << 8)
#define ALT_MSHC_STATUS_FIFO_FULL       (1 << 3)
#define ALT_MSHC_STATUS_FIFO_EMPTY      (1 << 2)

/* internal DMA interrupt */

#define ALT_MSHC_IDMA_INT_AI            (1 << 9)
#define ALT_MSHC_IDMA_INT_NI            (1 << 8)
#define ALT_MSHC_IDMA_INT_CES           (1 << 5)
#define ALT_MSHC_IDMA_INT_DU            (1 << 4)
#define ALT_MSHC_IDMA_INT_FBE           (1 << 2)
#define ALT_MSHC_IDMA_INT_RI            (1 << 1)
#define ALT_MSHC_IDMA_INT_TI            (1 << 0)

#define ALT_MSHC_DEFAULT_FIFO_LEN       (64) /* 32 -> 64 words*/

#define ALT_MSHC_DEFAULT_FIFO_TX_WMARK  (ALT_MSHC_DEFAULT_FIFO_LEN / 2)
#define ALT_MSHC_DEFAULT_FIFO_RX_WMARK  (ALT_MSHC_DEFAULT_FIFO_LEN / 2 - 1)
#define ALT_MSHC_DEFAULT_FIFO_MSIZE     (0x2)
#define ALT_MSHC_DEFAULT_FIFO_THRESH    ((ALT_MSHC_DEFAULT_FIFO_TX_WMARK)    | \
                                         (ALT_MSHC_DEFAULT_FIFO_MSIZE << 28) | \
                                         (ALT_MSHC_DEFAULT_FIFO_RX_WMARK << 16))
#define ALT_MSHC_FIFO_STEPS             (4)
#define ALT_MSHC_DEFAULT_RESP_TIMEOUT   (0x64)
#define ALT_MSHC_DEFAULT_DATA_TIMEOUT   (0xffffff)
#define ALT_MSHC_DEFAULT_TIMEOUT        (ALT_MSHC_DEFAULT_RESP_TIMEOUT | \
                                         ALT_MSHC_DEFAULT_DATA_TIMEOUT << 8)
#define ALT_MSHC_MAX_RETRIES            (50000)

/* max size is 8191, but here keep it multiple of 512 */

#define ALT_MSHC_IDMA_BUFFER_SIZE       (7680)
#define ALT_MSHC_MAX_RW_SECTORS         (ALT_MSHC_IDMA_BUFFER_SIZE * 2)

/* 
 * Dual buffer mode, SDMMC_BLOCK_SIZE descriptors.
 * (ALT_MSHC_MAX_RW_SECTORS/(2*ALT_MSHC_IDMA_BUFFER_SIZE)) * SDMMC_BLOCK_SIZE
 */

#define ALT_MSHC_IDMA_DESC_NUM          (SDMMC_BLOCK_SIZE)

#define ALT_MSHC_IDMA_DES0_DIC          (1 << 1)
#define ALT_MSHC_IDMA_DES0_LD           (1 << 2)
#define ALT_MSHC_IDMA_DES0_FD           (1 << 3)
#define ALT_MSHC_IDMA_DES0_CH           (1 << 4)
#define ALT_MSHC_IDMA_DES0_ER           (1 << 5)
#define ALT_MSHC_IDMA_DES0_CES          (1 << 30)
#define ALT_MSHC_IDMA_DES0_OWN          (1 << 31)
#define ALT_MSHC_IDMA_DES1_BS1(size)    (size & 0x1fff)
#define ALT_MSHC_IDMA_DES1_BS2(size)    ((size & 0x1fff) << 13)
#define ALT_MSHC_IDMA_DES1(size1, size2)(ALT_MSHC_IDMA_DES1_BS1 (size1) | \
                                         ALT_MSHC_IDMA_DES1_BS2 (size2))

/* various feature flags */

/* snoop enabled, no need to cache flush or invalidate */

#define ALT_MSHC_FLAGS_SNOOP_ENABLED    (1 << 0)

/* no card detection, card always present, no hot plug support */

#define ALT_MSHC_FLAGS_CARD_PRESENT     (1 << 1)

/* no card write protect pin connected, always writable */
#define ALT_MSHC_FLAGS_CARD_WRITABLE    (1 << 2)

/* driver name */

#define ALT_DW_MSHC_NAME                "altSocGen5DwMshc"

/* card monitor task name */

#define ALT_DW_MSHC_CARD_MON_TASK_NAME  "mshcCardMon"

/* card monitor task priority */

#define ALT_DW_MSHC_CARD_MON_TASK_PRI   (100)

/* card monitor task stack size */

#define ALT_DW_MSHC_CARD_MON_TASK_STACK (8192)

/* seconds to wait for command or data done */

#define ALT_DW_MSHC_CMD_WAIT_IN_SECS    (10)

/* card monitor task check status change delay */

#define ALT_DW_MSHC_CARDMON_DELAY_SECS  (2)

/* RW mode */

#define ALT_DW_MSHC_RW_MODE_DMA         (0)
#define ALT_DW_MSHC_RW_MODE_PIO         (1)
#define ALT_DW_MSHC_RW_MODE_DEFAULT     (ALT_DW_MSHC_RW_MODE_PIO)

/* typdefs */

typedef struct altSocGen5DwMshcIdmaDesc
    {
    UINT32              des0;   /* control descriptor */
    UINT32              des1;   /* buffer sizes */
    UINT32              des2;   /* buffer 1 physical address */
    UINT32              des3;   /* buffer 2 physical address */
    }ALT_MSHC_IDMA_DESC;

typedef struct altSocGen5DwMshcDrvCtrl
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
    ALT_MSHC_IDMA_DESC * idmaDesc;
	
    SDMMC_HOST          host;
    SDMMC_CARD          card;
    } ALT_MSHC_DRV_CTRL;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INCvxbAltSocGen5DwMshch */

