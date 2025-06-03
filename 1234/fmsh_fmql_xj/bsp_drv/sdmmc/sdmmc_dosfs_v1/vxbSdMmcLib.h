/* vxbSdMmcLib.h - Generic SD/MMC Library header */

/*
 * Copyright (c) 2009-2012 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01f,11jul12,my_  add high speed support 
01e,06apr12,sye  fixed compile issue when included by a CPP file. (WIND00342562)
01d,05jul11,j_z  update to use SDMMC_CMD from host control block
                 instead of stack variable.
01c,15feb11,my_  add SD bus power setup routine.
01b,02jul10,b_m  correct wrongly packed SDMMC_CID structure.
01a,18aug09,b_m  written.
*/

#ifndef __INCvxbSdMmcLibh
#define __INCvxbSdMmcLibh

/* includes */

#include <../src/hwif/h/storage/vxbSdMmcXbd.h>
/* #include "vxbSdMmcXbd.h" */

#ifdef __cplusplus
extern "C" {
#endif

/* defines */

/*
 * SD/MMC command index (a binary coded number between 0 and 63)
 *
 * The command set of the SD Memory Card system is divided into
 * several classes. Each class supports a set of card functionalities.
 * Class 0,2,4,5,7,8 are mandatory and shall be supported by all SD
 * memory cards. The other classes are optional. The supported Card
 * Command Classes (CCC) are coded as a parameter in the card specific
 * data (CSD) register of each card.
 */

/* basic commands (class 0) */

#define SDMMC_CMD_GO_IDLE_STATE         0

#define MMC_CMD_SEND_OP_COND            1

#define SDMMC_CMD_ALL_SEND_CID          2

#define SD_CMD_SEND_RELATIVE_ADDR       3
#define MMC_CMD_SET_RELATIVE_ADDR       3

#define SDMMC_CMD_SET_DSR               4

#define MMC_CMD_SWITCH                  6
#define SD_CMD_SWITCH_FUNC              6   /* defined in SD verion 1.10 */

#define SDMMC_CMD_SELECT_CARD           7

#define MMC_CMD_SEND_EXT_CSD            8
#define SD_CMD_SEND_IF_COND             8

#define SDMMC_CMD_SEND_CSD              9
#define SDMMC_CMD_SEND_CID              10
#define SDMMC_CMD_STOP_TRANSMISSION     12
#define SDMMC_CMD_SEND_STATUS           13

#define MMC_CMD_BUSTEST_R               14

#define SDMMC_CMD_GO_INACTIVE_STATE     15

/* stream read commands (class 1) */

#define MMC_CMD_READ_DAT_UNTIL_STOP     11

/* block-oriented read commands (class 2) */

#define SDMMC_CMD_SET_BLOCKLEN          16
#define SDMMC_CMD_READ_SINGLE_BLOCK     17
#define SDMMC_CMD_READ_MULTI_BLOCK      18

/* stream write commands (class 3) */

#define MMC_CMD_WRITE_DAT_UNTIL_STOP    20

/* block-oriented write commands (class 4) */

#define MMC_CMD_SET_BLOCK_COUNT         23

#define SDMMC_CMD_WRITE_SINGLE_BLOCK    24
#define SDMMC_CMD_WRITE_MULTI_BLOCK     25

#define MMC_CMD_PROGRAM_CID             26

#define SDMMC_CMD_PROGRAM_CSD           27

/* block-oriented write protection commands (class 6) */

#define SDMMC_CMD_SET_WRITE_PROT        28
#define SDMMC_CMD_CLR_WRITE_PROT        29
#define SDMMC_CMD_SEND_WRITE_PROT       30

/* erase commands (class 5) */

#define SD_CMD_ERASE_WR_BLK_START       32
#define SD_CMD_ERASE_WR_BLK_END         33

#define MMC_CMD_ERASE_GROUP_START       35
#define MMC_CMD_ERASE_GROUP_END         36

#define SDMMC_CMD_ERASE                 38

/* lock card commands (class 7) */

#define SDMMC_CMD_LOCK_UNLOCK           42

/* application-specific commands (class 8) */

#define SDMMC_CMD_APP_CMD               55
#define SDMMC_CMD_GEN_CMD               56

/* SD ACMD index */

#define SD_ACMD_SET_BUS_WIDTH           6
#define SD_ACMD_SD_STATUS               13
#define SD_ACMD_SEND_NUM_WR_BLOCKS      22
#define SD_ACMD_SEND_OP_COND            41
#define SD_ACMD_SET_CLR_CARD_DETECT     42
#define SD_ACMD_SEND_SCR                51

#define SD_ACMD6_SET_FUNC               0x80000000 
#define SD_ACMD6_GET_FUNC               0x00000000 
#define SD_ACMD6_GROUP_ACCESS_MODE      0
#define SD_ACMD6_GROUP_CMD_SYSTEM       1
#define SD_ACMD6_GROUP_DRV_STRENGTH     2
#define SD_ACMD6_GROUP_CURRENT_LIMIT    3

/*
 * SD/MMC command response types
 * There are 5 types of responses for the SD/MMC Memory Card.
 * The SDIO Card supports additional response types named R4 and R5.
 */

#define SDMMC_CMD_RSP_LEN48     0x00000001
#define SDMMC_CMD_RSP_LEN136    0x00000002
#define SDMMC_CMD_RSP_CMDIDX    0x00000004
#define SDMMC_CMD_RSP_CRC       0x00000008
#define SDMMC_CMD_RSP_BUSY      0x00000010

#define SDMMC_CMD_RSP_NONE      0

#define SDMMC_CMD_RSP_R1       (SDMMC_CMD_RSP_LEN48 | SDMMC_CMD_RSP_CMDIDX | \
                                SDMMC_CMD_RSP_CRC)
#define SDMMC_CMD_RSP_R1B      (SDMMC_CMD_RSP_LEN48 | SDMMC_CMD_RSP_CMDIDX | \
                                SDMMC_CMD_RSP_CRC | SDMMC_CMD_RSP_BUSY)
#define SDMMC_CMD_RSP_R2       (SDMMC_CMD_RSP_LEN136 | SDMMC_CMD_RSP_CRC)
#define SDMMC_CMD_RSP_R3       (SDMMC_CMD_RSP_LEN48)
#define SDMMC_CMD_RSP_R4       (SDMMC_CMD_RSP_LEN48)
#define SDMMC_CMD_RSP_R5       (SDMMC_CMD_RSP_LEN48 | SDMMC_CMD_RSP_CMDIDX | \
                                SDMMC_CMD_RSP_CRC)
#define SDMMC_CMD_RSP_R6       (SDMMC_CMD_RSP_LEN48 | SDMMC_CMD_RSP_CMDIDX | \
                                SDMMC_CMD_RSP_CRC)
#define SDMMC_CMD_RSP_R7       (SDMMC_CMD_RSP_LEN48 | SDMMC_CMD_RSP_CMDIDX | \
                                SDMMC_CMD_RSP_CRC)


/*
 * Card Registers
 *
 * Six registers are defined within the card interface:
 * OCR, CID, CSD, RCA, DSR and SCR. These can be accessed only
 * by corresponding commands. The OCR, CID, CSD and SCR registers
 * carry the card/content specific information, while the RCA and
 * DSR registers are configuration registers storing actual
 * configuration parameters. In order to enable future extension,
 * the card shall return 0 in the reserved bits of the registers.
 */

/* OCR */

#define OCD_VDD_VOL_LOW     (1 << 7)
#define OCR_VDD_VOL_27_28   (1 << 15)
#define OCR_VDD_VOL_28_29   (1 << 16)
#define OCR_VDD_VOL_29_30   (1 << 17)
#define OCR_VDD_VOL_30_31   (1 << 18)
#define OCR_VDD_VOL_31_32   (1 << 19)
#define OCR_VDD_VOL_32_33   (1 << 20)
#define OCR_VDD_VOL_33_34   (1 << 21)
#define OCR_VDD_VOL_34_35   (1 << 22)
#define OCR_VDD_VOL_35_36   (1 << 23)
#define OCR_VDD_VOL_MASK    0x00FFFFFF
#define OCR_CARD_CAP_STS    (1 << 30)
#define OCR_CARD_PWRUP_STS  0x80000000U 

/* CID */

typedef struct
    {
    UINT8   mid;            /* manufacturer ID */
    char    oid[2];         /* OEM/application ID */
    char    pnm[5];         /* product name */
    UINT8   prv;            /* product revision */
    UINT8   psn[4];         /* product serial number */
    UINT8   mdt1;           /* manufacturing date 1 */
    UINT8   mdt2;           /* manufacturing date 2 */
    UINT8   crc;            /* CRC7 checksum */
    } SDMMC_CID;

#define CID_PRV_MAJOR(prv)  (((prv) & 0xF0) >> 4)
#define CID_PRV_MINOR(prv)  ((prv) & 0x0F)

#define CID_MDT_YEAR(mdt1, mdt2)    \
    (((((mdt1) & 0x0F) << 4) | (((mdt2) & 0xF0) >> 4 )) + 2000)
#define CID_MDT_MONTH(mdt2) ((mdt2) & 0x0F)

/* CSD */

#define CSD_VER1            0
#define CSD_VER2            1
#define CSD_VER(ver)        ((ver) >> 6)

#define CSD_TRAN_SPD_NORMAL 0x32    /* 25 MHz */
#define CSD_TRAN_SPD_HIGH   0x5A    /* 50 MHz */

#define CSD_TRAN_SPD_25MHZ  25000000
#define CSD_TRAN_SPD_50MHZ  50000000

/* SCR */

typedef struct
    {
    UINT8   spec;           /* SCR structure & spec version */
    UINT8   config;         /* various configuration */
    UINT8   rsvd[6];        /* reserved */
    } SD_SCR;

#define SCR_SD_BUS_WIDTH_1BIT       0x01
#define SCR_SD_BUS_WIDTH_4BIT       0x04

/* SD/MMC data transfered */

typedef struct
    {
    void *      buffer;
    UINT32      blkNum;
    UINT32      blkSize;
    BOOL        isRead;
    } SDMMC_DATA;

/* SD/MMC command issued */

typedef struct
    {
    UINT32      cmdIdx;
    UINT32      cmdArg;
    UINT32      rspType;
    BOOL        hasData;
    UINT32      cmdRsp[4];
    SDMMC_DATA  cmdData;
    UINT32      cmdErr;
    UINT32      dataErr;
    } SDMMC_CMD;

#define SDMMC_CMD_FORM(cmd, idx, arg, type, data)   \
    do                                              \
        {                                           \
        bzero ((char *)cmd, sizeof(SDMMC_CMD));      \
        cmd->cmdIdx  = (idx);                        \
        cmd->cmdArg  = (arg);                        \
        cmd->rspType = (type);                       \
        cmd->hasData = (data);                       \
        } while ((0));

#define SDMMC_DATA_FORM(cmd, buf, num, size, read)  \
    do                                              \
        {                                           \
        cmd->cmdData.buffer = buf;                   \
        cmd->cmdData.blkNum = num;                   \
        cmd->cmdData.blkSize = size;                 \
        cmd->cmdData.isRead = read;                  \
        } while ((0));

#define SDMMC_CMD_ARG_RCA(rca)  (((UINT32)rca) << 16)

/* command associated errors */

#define SDMMC_CMD_ERR_PROTOCOL  0x00000001
#define SDMMC_CMD_ERR_TIMEOUT   0x00000002

/* data associated errors */

#define SDMMC_DATA_ERR_PROTOCOL 0x00000001
#define SDMMC_DATA_ERR_TIMEOUT  0x00000002

/* SD command argument defines */

/* CMD8: send interface condition */

#define SD_CMD8_ARG_VHS_27_36       1
#define SD_CMD8_ARG_VHS_LOW_VOLTAGE 2
#define SD_CMD8_ARG_VHS_SHIFT       8
#define SD_CMD8_ARG_CHK_PATTERN     0xAA

/* ACMD6: set bus width */

#define SD_ACMD6_ARG_1BIT_BUS       0
#define SD_ACMD6_ARG_4BIT_BUS       2

/* clock frequency */

#define SDMMC_CLK_FREQ_400KHZ       400000
#define SDMMC_CLK_FREQ_25MHZ        25000000
#define SDMMC_CLK_FREQ_50MHZ        50000000

/* bus width */

#define SDMMC_BUS_WIDTH_1BIT        0
#define SDMMC_BUS_WIDTH_4BIT        1
#define SDMMC_BUS_WIDTH_8BIT        2
#define SDMMC_BUS_WIDTH_MAX         3

/* SD/MMC host */

typedef struct
    {
    FUNCPTR         cmdIssue;       /* issue command */
    FUNCPTR         clkFreqSetup;   /* setup the clock frequency */
    FUNCPTR         vddSetup;       /* sd power vdd setup routine */
    FUNCPTR         busWidthSetup;  /* setup the bus width */
    FUNCPTR         cardWpCheck;    /* check if card is write protected */
    } SDMMC_HOST_OPS;

typedef struct
    {
    VXB_DEVICE_ID   pDev;
    SDMMC_HOST_OPS  ops;
    UINT32          curClkFreq;
    UINT32          vdd;
    UINT32          curBusWidth;
    UINT32          capbility;
    BOOL            highSpeed;
    SDMMC_CMD       cmdIssued;
    } SDMMC_HOST;

/* SD/MMC card */

typedef struct
    {
    UINT16          version;            /* SD/MMC version */
    UINT16          rca;                /* relative card address */
	
    UINT32          voltage;            /* work voltage: ocr ? */
	
    BOOL            highCapacity;       /* high capacity > 2GB */
    SDMMC_CID       cid;                /* CID register */
    UINT8           csd[16];            /* CSD register */
    UINT32          tranSpeed;          /* transfer speed */
    BOOL            highSpeed;          /* 50 MHz */
    UINT32          readBlkLen;         /* read block length */
    UINT32          writeBlkLen;        /* write block length */
    BOOL            hasDsr;             /* DSR implemented */
    UINT64          blkNum;             /* number of blocks */
    UINT64          capacity;           /* user data capacity */
    SD_SCR          scr;                /* SCR register */
    UINT32          sdSpec;             /* SD spec version */
    UINT32          sdSec;              /* SD security verison */
    BOOL            dat4Bit;            /* DAT bus width */
    UINT32          cardSts;            /* card status */
    } SDMMC_INFO;

typedef struct
    {
    SDMMC_HOST *    host;               /* pointer to host */
    SDMMC_XBD_DEV   xbdDev;             /* XBD block device */
    SDMMC_INFO      info;               /* detailed information */
    BOOL            attached;           /* attached to host */
    BOOL            isMmc;              /* MMC card */
    BOOL            isWp;               /* write protected */
    UINT32          idx;                /* card index */
    } SDMMC_CARD;

/* SD/MMC version */

#define SD_VERSION_100              0
#define SD_VERSION_110              1
#define SD_VERSION_200              2
#define SD_VERSION_MAX              3
#define SD_VERSION_UNKNOWN          0xffff 

/* SD/MMC card status bits */

#define CARD_STS_CUR_STATE_MASK     0x00001e00
#define CARD_STS_CUR_STATE_SHIFT    9
#define CARD_STS_READY_FOR_DATA     0x00000100

/* SD/MMC card max numbers */

#define SDMMC_CARD_MAX_NUM          32

/* SD/MMC block size is 512 bytes by default */

#define SDMMC_BLOCK_SIZE            512


/* forward declarations */

STATUS sdMmcCmdGoIdleState (SDMMC_CARD * card);
STATUS sdMmcCmdAllSendCid (SDMMC_CARD * card);
STATUS sdCmdSendRelativeAddr (SDMMC_CARD * card);
STATUS sdMmcCmdSetDsr (SDMMC_CARD * card, UINT32 dsr);
STATUS sdMmcCmdSelectCard (SDMMC_CARD * card);
STATUS sdMmcCmdDeselectCard (SDMMC_CARD * card);
STATUS sdCmdSendIfCond (SDMMC_CARD * card);
STATUS sdMmcCmdSendCsd (SDMMC_CARD * card);
STATUS sdMmcCmdSendCid (SDMMC_CARD * card);
STATUS sdMmcCmdStopTransmission (SDMMC_CARD * card);
STATUS sdMmcCmdSendStatus (SDMMC_CARD * card);
STATUS sdMmcCmdGoInactiveState (SDMMC_CARD * card);
STATUS sdMmcCmdSetBlockLen (SDMMC_CARD * card);
STATUS sdMmcCmdReadSingleBlock (SDMMC_CARD * card, void * buf, UINT64 blkNo);
STATUS sdMmcCmdReadMultipleBlock (SDMMC_CARD * card, void * buf, UINT64 blkNo, UINT32 numBlks);
STATUS sdMmcCmdReadBlock (SDMMC_CARD * card, void * buf, UINT64 blkNo, UINT32 numBlks);
STATUS sdMmcCmdWriteSingleBlock (SDMMC_CARD * card, void * buf, UINT64 blkNo);
STATUS sdMmcCmdWriteMultipleBlock (SDMMC_CARD * card, void * buf, UINT64 blkNo, UINT32 numBlks);
STATUS sdMmcCmdWriteBlock (SDMMC_CARD * card, void * buf, UINT64 blkNo, UINT32 numBlks);
STATUS sdMmcCmdAppCmd (SDMMC_CARD * card);

STATUS sdACmdSetBusWidth (SDMMC_CARD * card, UINT32 width);
STATUS sdACmdSendOpCond (SDMMC_CARD * card);
STATUS sdACmdSendScr (SDMMC_CARD * card);

void sdMmcStatusWaitReadyForData (SDMMC_CARD * card);

STATUS sdMmcBlkRead (SDMMC_CARD * card, sector_t blkNo, UINT32 numBlks, void * pBuf);
STATUS sdMmcBlkWrite (SDMMC_CARD * card, sector_t blkNo, UINT32 numBlks, void * pBuf);

STATUS sdMmcIdentify (SDMMC_CARD * card);

STATUS sdMmcCardIdxAlloc (SDMMC_CARD * card);
void sdMmcCardIdxFree (SDMMC_CARD * card);

void sdMmcHostShow (SDMMC_HOST * host);
void sdMmcCardShow (SDMMC_CARD * card);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INCvxbSdMmcLibh */
