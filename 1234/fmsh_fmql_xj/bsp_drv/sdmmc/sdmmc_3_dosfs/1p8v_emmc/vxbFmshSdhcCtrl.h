/* vxbSdhcCtrl.h - SDHC host controller common driver library header */

/*
 * Copyright (c) 2012, 2015-2017 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01f,19sep17,d_l  add 10bit clock mode definition. (VXW6-86304)
01e,26jan16,g_x  add Octeon Sdhci (US74580)
01d,22jan16,myt  add support for SD/eMMC new driver. (VXW6-83708)
01c,08apr15,myt  fix SD clock frequency error for nano6060 (VXW6-84244)
01b,06jan15,myt  fix the error of DMA address update (VXW6-83697)
                 add support for Bay Trail (VXW6-80833)
01a,28aug12,e_d  written.
*/

#ifndef __INCvxbfmshSdhcCtrlh
#define __INCvxbfmshSdhcCtrlh

#ifdef __cplusplus
extern "C" {
#endif

#define SDMMC_CLK_400_KHZ       400000   /* 400 KHZ */
#define SDMMC_CLK_25_MHZ        25000000 /* 25 MHZ  */
#define SDMMC_CLK_50_MHZ        50000000 /* 50 MHZ  */
#define SDMMC_CLK_52_MHZ        52000000 /* 52 MHZ  */

#define POLL_TIMEOUT            0x100000

/* SDHC register offset */
/*
#define SDHC_DSADDR             0x000
#define SDHC_BLKATTR            0x004
#define SDHC_CMDARG             0x008
#define SDHC_XFERTYP            0x00C
#define SDHC_CMDRSP0            0x010
#define SDHC_CMDRSP1            0x014
#define SDHC_CMDRSP2            0x018
#define SDHC_CMDRSP3            0x01C
#define SDHC_DATPORT            0x020
#define SDHC_PRSSTAT            0x024
#define SDHC_PROCTL             0x028
#define SDHC_SYSCTL             0x02C
#define SDHC_IRQSTAT            0x030
#define SDHC_IRQSTATEN          0x034
#define SDHC_IRQSIGEN           0x038
#define SDHC_AUTOC12ERR         0x03C
#define SDHC_HOSTCAPBLT         0x040
#define SDHC_WML                0x044
#define SDHC_FEVT               0x050
#define SDHC_HOSTVER            0x0FC
#define SDHC_DCR                0x40C
#define SDHC_SRST               0x1FC
*/

/* Block Attributes Register */

#define BLKATTR_CNT_SHIFT       16

/* Block Count Size Register */

#define BLKBUFFER_SIZE_4K       0
#define BLKBUFFER_SIZE_8K       1
#define BLKBUFFER_SIZE_16K      2
#define BLKBUFFER_SIZE_32K      3
#define BLKBUFFER_SIZE_64K      4
#define BLKBUFFER_SIZE_128K     5
#define BLKBUFFER_SIZE_256K     6
#define BLKBUFFER_SIZE_512K     7
#define BLKBUFFER_SIZE_SHIFT    12
#define BLKBUFFER_512K_MASK     0xfff80000
#define BLKBUFFER_512K          0x80000

/* Transfter Type Register */

#define XFERTYP_CMDINX(idx)     (((idx) & 0x3f) << 24)
#define XFERTYP_CMDTYP_NORMAL   0x00000000
#define XFERTYP_CMDTYP_SUSPEND  0x00400000
#define XFERTYP_CMDTYP_RESUME   0x00800000
#define XFERTYP_CMDTYP_ABORT    0x00c00000
#define XFERTYP_DATA_PRESENT    0x00200000
#define XFERTYP_CICEN           0x00100000
#define XFERTYP_CCCEN           0x00080000
#define XFERTYP_RSPTYP_NORSP    0x00000000
#define XFERTYP_RSPTYP_LEN136   0x00010000
#define XFERTYP_RSPTYP_LEN48    0x00020000
#define XFERTYP_RSPTYP_LEN48B   0x00030000
#define XFERTYP_MULTI_BLK       0x00000020
#define XFERTYP_DIR_READ        0x00000010
#define XFERTYP_AC12EN          0x00000004
#define XFERTYP_BCEN            0x00000002
#define XFERTYP_DMAEN           0x00000001

/* Present State Register */

#define PRSSTAT_WPSPL           0x00080000
#define PRSSTAT_CDPL            0x00040000
#define PRSSTAT_CINS            0x00010000
#define PRSSTAT_BREN            0x00000800
#define PRSSTAT_BWEN            0x00000400
#define PRSSTAT_DLA             0x00000004
#define PRSSTAT_CDIHB           0x00000002
#define PRSSTAT_CIHB            0x00000001

/* Protocol Control Register */

#define PROCTL_EMODE_MASK       0x00000030
#define PROCTL_EMODE_SHIFT      4
#define PROCTL_D3CD             0x00000008
#define PROCTL_DTW_1BIT         0x00000000
#define PROCTL_DTW_4BIT         0x00000002
#define PROCTL_DTW_8BIT         0x00000020
#define PROCTL_DTW_8BIT_FSL     0x00000004
#define PROCTL_HS_ENABLED       0x00000004
#define PROCTL_VOLT_SEL_SHIFT   9
#define PROCTL_SD_PWR_EN        0x00000100
#define PROCTL_SD_PWR_EN_BTRAIL 0x00001000
#define PROCTL_SDBUS_VOLT_3P3   7
#define PROCTL_SDBUS_VOLT_3P0   6
#define PROCTL_SDBUS_VOLT_1P8   5

/* System Control Register */

#define SYSCTL_INITA            0x08000000
#define SYSCTL_RSTD             0x04000000
#define SYSCTL_RSTC             0x02000000
#define SYSCTL_RSTA             0x01000000
#define SYSCTL_DTOCV_MASK       0x000f0000
#define SYSCTL_DTOCV_SHIFT      16
#define SYSCTL_DTOCV_MAX        14
#define SYSCTL_CLK_MASK         0x0000fff0
#define SYSCTL_FS_SHIFT         8
#define SYSCTL_FS_MAX           256
#define SYSCTL_FS_MIN           2
#define SYSCTL_DVS_SHIFT        4
#define SYSCTL_DVS_MAX          16
#define SYSCTL_DVS_MIN          1
#define SYSCTL_SDCLKEN          0x00000008
#define SYSCTL_PEREN            0x00000004
#define SYSCTL_HCKEN            0x00000002
#define SYSCTL_IPGEN            0x00000001
#define SYSCTL_CLK_STABLE       0x00000002
#define SYSCTL_CLK_EN           0x00000004
#define SYSCTL_DIV_SHIFT        8
#define SYSCTL_10BITDIV_SHIFT   6

/* Interrupt Register */

#define IRQ_DMAE                0x10000000
#define IRQ_AC12E               0x01000000
#define IRQ_DEBE                0x00400000
#define IRQ_DCE                 0x00200000
#define IRQ_DTOE                0x00100000
#define IRQ_CIE                 0x00080000
#define IRQ_CEBE                0x00040000
#define IRQ_CCE                 0x00020000
#define IRQ_CTOE                0x00010000
#define IRQ_CINT                0x00000100
#define IRQ_CRM                 0x00000080
#define IRQ_CINS                0x00000040
#define IRQ_BRR                 0x00000020
#define IRQ_BWR                 0x00000010
#define IRQ_DINT                0x00000008
#define IRQ_BGE                 0x00000004
#define IRQ_TC                  0x00000002
#define IRQ_CC                  0x00000001

#define IRQ_DATA_ERR            (IRQ_DEBE | IRQ_DCE | IRQ_DTOE)
#define IRQ_CMD_ERR             (IRQ_CIE | IRQ_CEBE | IRQ_CCE | IRQ_CTOE)

#define IRQ_DATA                (IRQ_DATA_ERR | IRQ_TC)
#define IRQ_CMD                 (IRQ_CMD_ERR | IRQ_CC)

#define IRQ_ALL                 0xffffffff

/* Auto CMD12 Status Register */

#define AUTOC12ERR_CNIBAC12E    0x00000080
#define AUTOC12ERR_AC12IE       0x00000010
#define AUTOC12ERR_AC12CE       0x00000008
#define AUTOC12ERR_AC12EBE      0x00000004
#define AUTOC12ERR_AC12TOE      0x00000002
#define AUTOC12ERR_AC12NE       0x00000001

/* Host Controller Capabilities Register */

#define HOSTCAPBLT_VS18         0x04000000
#define HOSTCAPBLT_VS30         0x02000000
#define HOSTCAPBLT_VS33         0x01000000
#define HOSTCAPBLT_DMAS         0x00400000
#define HOSTCAPBLT_HSS          0x00200000
#define HOSTCAPBLT_MBL_MASK     0x00070000
#define HOSTCAPBLT_MBL_SHIFT    16
#define HOSTCAPBLT_MBL_512      0x00000000
#define HOSTCAPBLT_MBL_1024     0x00010000
#define HOSTCAPBLT_MBL_2048     0x00020000
#define HOSTCAPBLT_MBL_4096     0x00030000
#define HOSTCAPBLT_FREQ_SHIT    8
#define HOSTCAPBLT_FREQ_MASK    0x3f
#define HOSTCAPBLT_FREQ_MASK_V3 0xff

/* Watermark Level Register */

#define WML_WR_MAX              0x80
#define WML_RD_MAX              0x10
#define WML_RD_MAX_PIO          0x80
#define WML_WR_SHIFT            16

/* Host Controller Version Register */

#define HOSTVER_VVN_MASK        0x0000ff00
#define HOSTVER_VVN_SHIFT       8
#define HOSTVER_SVN_MASK        0x000000ff

#define HOSTVER_VVN_10          0x00000000
#define HOSTVER_VVN_20          0x00000100
#define HOSTVER_SVN_10          0x00000000
#define HOSTVER_SVN_20          0x00000001

/* DMA Control Register */

#define DCR_SNOOP               0x00000040
#define DCR_RD_SAFE             0x00000004
#define DCR_RD_PFE              0x00000002
#define DCR_RD_PF_SIZE_32BYTES  0x00000001

/* SRST Register */

#define RESET_ON                0x00000001
#define RESET_RELEASE           0x00000000

/* clock set mask */

#define CLK_SETMASK             0xFF00
#define CLK_SETMASK_10BIT       0xFFC0

/* PCI SDHC device list  */

#define INTEL_VENDOR_ID         0x8086
#define TOPCLIFF0_DEVICE_ID     0x8809
#define TOPCLIFF1_DEVICE_ID     0x880a
#define PCI_DEVICE_ID_INTEL_BAY_TRAIL_EMMC_441    0x0f14
#define PCI_DEVICE_ID_INTEL_BAY_TRAIL_SD          0x0f16
/* Max sectors per-transfer */

#define SDHC_MAX_RW_SECTORS     2048

#define SDHC_PIO_TIMEOUT        0x1000000

/*************************************************************************************************/

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

/* only for alt_soc5*/
/*#define ALT_MSHC_CARDTHRCTL             (0x100)*/
/*#define ALT_MSHC_BACK_END_POWER_R       (0x104)*/

/* version lower than 2.40a is 0x100, alt soc gen5 is 2.40a */
/* DATA	>= 0x100	R/W	数据FIFO读/写 Reset Value: 32’hx */
#define ALT_MSHC_DATA                   (0x100)  /* 0x200->0x100*/

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
#define ALT_MSHC_INT_ERROR               (ALT_MSHC_INT_CMD_ERROR              | \
                                         ALT_MSHC_INT_DATA_ERROR             | \
                                         ALT_MSHC_INT_HLE)

#define ALT_MSHC_INT_PIO_DEFAULTS       (ALT_MSHC_INT_RXDR | ALT_MSHC_INT_CD | \
                                         ALT_MSHC_INT_DTO | ALT_MSHC_INT_CMD | ALT_MSHC_INT_HTO | \
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
#define ALT_MSHC_STATUS_CARD_DATA_BUSY  (1 << 9)   /* jc*/

/* internal DMA interrupt */

#define ALT_MSHC_IDMA_INT_AI            (1 << 9)
#define ALT_MSHC_IDMA_INT_NI            (1 << 8)
#define ALT_MSHC_IDMA_INT_CES           (1 << 5)
#define ALT_MSHC_IDMA_INT_DU            (1 << 4)
#define ALT_MSHC_IDMA_INT_FBE           (1 << 2)
#define ALT_MSHC_IDMA_INT_RI            (1 << 1)
#define ALT_MSHC_IDMA_INT_TI            (1 << 0)

#define ALT_MSHC_DEFAULT_FIFO_LEN       (64) /* 32 -> 64 words*/
/*#define ALT_MSHC_DEFAULT_FIFO_LEN       (32) // 32 -> 64 words*/

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
                                         
/*#define ALT_MSHC_MAX_RETRIES            (50000)*/
#define ALT_MSHC_MAX_RETRIES             (200000)

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

#define ALT_MSHC_FLAGS_CARD_WRITABLE    (1 << 2)
#define ALT_MSHC_FLAGS_CARD_PRESENT     (1 << 1)

#define ALT_MSHC_FLAGS_SNOOP_ENABLED    (1 << 0)

#define ALT_DW_MSHC_CMD_WAIT_IN_SECS    (10)
/*************************************************************************************************/

/* SDHC PIO Description */
typedef struct _sdhcPioDesc_
    {
    char *          bufOri;
    char *          bufCur;
    UINT32          blkSize;
    } SDHC_PIO_DESC;


/* SDHC driver control */
typedef struct sdhcDrvCtrl
    {
    SD_HOST_CTRL    sdHostCtrl;
	
    void *          regBase;
    void *          regHandle;
    void *          pioHandle;
	
    struct vxbSdioInt * pIntInfo;
	
    UINT32          intSts;
    UINT32          intMask;
    UINT32          flags;
	
    BOOL            highSpeed;
    SDHC_PIO_DESC   pioDesc;
    UINT32          sdmaAddr;
    UINT32          sdmaAddrBound;
	
    FUNCPTR             clkFreqSetup;
    BOOL                cardIns;
    UINT32              remainBytes;
    SD_CMD  pSdCmdLast;
    int count;
    }FMSH_SDHC_DRV_CTRL;  /*  SDHC_DEV_CTRL; */

/* SDHC driver name */


#define FMSH_SDHC_NAME                  "fmshSdhci"


/* SDHC card monitor task priority */

#define SDHC_CARD_MON_TASK_PRI      100

/* SDHC card monitor task stack size */

#define SDHC_CARD_MON_TASK_STACK    8192

/* SDHC DMA mode supported */

#define SDHC_DMA_MODE_DEFAULT       0
#define SDHC_DMA_MODE_SDMA          0
#define SDHC_DMA_MODE_PIO           1
#define SDHC_DMA_MODE_ADMA1         2
#define SDHC_DMA_MODE_ADMA2         3

/* total number of DMA transfer modes */

#define SDHC_DMA_MODES_NR           4

/* seconds to wait for SD/MMC command or data done */

#define SDMMC_CMD_WAIT_IN_SECS      10

/* seconds to wait for SD/MMC R1B command's data interrupt */

#define SDMMC_CMD_WAIT_IN_SECS_R1B  3

/* SD/MMC card monitor task check status change delay */

#define SDMMC_CARDMON_DELAY_SECS    2

/* register low level access routines */

#define SDHC_BAR(p)         ((FMSH_SDHC_DRV_CTRL *)(p)->pDrvCtrl)->regBase
#define SDHC_HANDLE(p)      ((FMSH_SDHC_DRV_CTRL *)(p)->pDrvCtrl)->regHandle

#define CSR_READ_4(pDev, addr)              \
        vxbRead32(SDHC_HANDLE(pDev),        \
                  (volatile UINT32 *)((char *)SDHC_BAR(pDev) + addr))

#define CSR_WRITE_4(pDev, addr, data)       \
        vxbWrite32(SDHC_HANDLE(pDev),       \
                   (volatile UINT32 *)((char *)SDHC_BAR(pDev) + addr), data)

#define CSR_SETBIT_4(pDev, offset, val)     \
        CSR_WRITE_4(pDev, offset, CSR_READ_4(pDev, offset) | (val))

#define CSR_CLRBIT_4(pDev, offset, val)     \
        CSR_WRITE_4(pDev, offset, CSR_READ_4(pDev, offset) & ~(val))

/* internal buffer data port access routines */

#define PIO_HANDLE(p)       ((FMSH_SDHC_DRV_CTRL *)(p)->pDrvCtrl)->pioHandle

#define PIO_READ_4(pDev, addr)              \
        vxbRead32(PIO_HANDLE(pDev),         \
                  (UINT32 *)((char *)SDHC_BAR(pDev) + addr))

#define PIO_WRITE_4(pDev, addr, data)       \
        vxbWrite32(PIO_HANDLE(pDev),        \
                   (UINT32 *)((char *)SDHC_BAR(pDev) + addr), data)
void fmshSdhcCtrl_InstInit(VXB_DEVICE_ID);
void fmshSdhcCtrl_InstInit2 (VXB_DEVICE_ID);
STATUS fmshSdhcCtrl_InstConnect (VXB_DEVICE_ID);
STATUS fmshSdhcCtrl_Isr (VXB_DEVICE_ID);
STATUS fmshSdhcCtrl_CmdIssue (VXB_DEVICE_ID, SD_CMD *);
STATUS fmshSdhcCtrl_CmdPrepare (VXB_DEVICE_ID, SD_CMD *);
STATUS fmshSdhcCtrl_PioRead (VXB_DEVICE_ID);
STATUS fmshSdhcCtrl_PioWrite (VXB_DEVICE_ID);
STATUS fmshSdhcCtrl_CmdIssuePoll (VXB_DEVICE_ID, SD_CMD *);
STATUS fmshSdhcCtrl_Init (VXB_DEVICE_ID);
STATUS sdhcInterruptInfo (VXB_DEVICE_ID, UINT32 *);
STATUS sdhcDevControl (VXB_DEVICE_ID, pVXB_DEVCTL_HDR);

void fmshSdhcCtrl_BusWidthSetup (VXB_DEVICE_ID, UINT32);
void fmshSdhcCtrl_CardMonTaskPoll (VXB_DEVICE_ID);
void fmshSdhcCtrl_ClkFreqSetup (VXB_DEVICE_ID, UINT32);
BOOL fmshSdhcCtrl_CardWpCheck (VXB_DEVICE_ID);
BOOL fmshSdhcCtrl_CardInsertSts (VXB_DEVICE_ID);

void fmshSdhc_ClkFreqSetup(VXB_DEVICE_ID pDev,UINT32 clk);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INCvxbfmshSdhcCtrlh */


