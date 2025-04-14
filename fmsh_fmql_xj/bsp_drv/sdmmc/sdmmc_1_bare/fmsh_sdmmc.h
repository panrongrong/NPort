/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_sdmmc.h
*
* This file contains
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 0.01   yl  12/20/2018  First Release
*</pre>
******************************************************************************/
#ifndef _FMSH_SDMMC_H_
#define _FMSH_SDMMC_H_

/***************************** Include Files *********************************/

#include "../../common/fmsh_common.h"
#include "../../common/fmsh_ps_parameters.h"

#include "fmsh_sdmmc_protocol.h"

/*#include "fmsh_dmac.h"*/
#include "../../dma/dma_1_bare/fmsh_dmac.h"


/************************** Constant Definitions *****************************/

/* Definitions for peripheral PS7_SD_0 */
#ifndef FPAR_PS7_SD_0_HAS_CD
    #define FPAR_PS7_SD_0_HAS_CD 1
#endif

#ifndef FPAR_PS7_SD_0_HAS_WP
    #define FPAR_PS7_SD_0_HAS_WP 1
#endif

/* Definitions for peripheral PS7_SD_1 */
#ifndef FPAR_PS7_SD_1_HAS_CD
    #define FPAR_PS7_SD_1_HAS_CD 1
#endif

#ifndef FPAR_PS7_SD_1_HAS_WP
    #define FPAR_PS7_SD_1_HAS_WP 1
#endif

#define SDMMC_LOOP_TIMEOUT      500

/** Default block size for SD/MMC access */
#define SDMMC_BLOCK_SIZE        512
#define SDMMC_MAX_DIV_CNT       255

/* CMD Response flag */
#define CMD_RESP_FLAG_NONE       0x00 /* no response */

#define CMD_RESP_FLAG_DATALINE   0x01 /* transferred on dataline */
#define CMD_RESP_FLAG_NOCRC      0x02 /* no crc check */
#define CMD_RESP_FLAG_RDATA      0x04 /* data read */
#define CMD_RESP_FLAG_WDATA      0x08 /* data write */

/** No error */
#define SDMMC_SUCCESS               SDMMC_OK
/** The driver is locked. */
#define SDMMC_ERROR_LOCKED          SDMMC_LOCKED
/** There was an error with the SD driver. */
#define SDMMC_ERROR                 SDMMC_ERR
/** The SD card did not answer the command. */
#define SDMMC_ERROR_NORESPONSE      SDMMC_NO_RESPONSE
/** The SD card is not initialized. */
#define SDMMC_ERROR_NOT_INITIALIZED SDMMC_NOT_INITIALIZED
/** The SD card is busy. */
#define SDMMC_ERROR_BUSY            SDMMC_BUSY
/** The input parameter error */
#define SDMMC_ERROR_PARAM           SDMMC_PARAM
/** Invalid state */
#define SDMMC_ERROR_STATE           SDMMC_STATE
/** User canceled */
#define SDMMC_ERROR_USER_CANCEL     SDMMC_USER_CANCEL
/** The operation is not supported. */
#define SDMMC_ERROR_NOT_SUPPORT     SDMMC_NOT_SUPPORTED


/* mmc card type*/
#define MMC	    0x1
#define SD	    0X2
#define SDIO	0x3
#define CEATA	0x4
#define EMMC    0x5

#define TYPE_SLOT    0x0
#define TYPE_CHIP    0x1

#define CARD_INSERT     0x0
#define CARD_UNINSERT   0x1

/* flags for stat field of the mmc_slot structure */
#define MMC_WP_GRP_EN		0x00000010
#define MMC_PERM_WP		0x00000020
#define MMC_TMP_WP		0x00000040
#define MMC_READ_PART		0x00000100
#define MMC_WRITE_PART		0x00000200

#define SDMMC_CLK_400_KHZ       400000   /* 400 KHZ */
#define SDMMC_CLK_25_MHZ        25000000 /* 25 MHZ  */
#define SDMMC_CLK_50_MHZ        50000000 /* 50 MHZ  */
#define SDMMC_CLK_52_MHZ        52000000 /* 52 MHZ  */

#define SD_CMD8_VOL_PATTERN   0x1AA
#define SD_BUS_WIDTH_1        0
#define SD_BUS_WIDTH_4        2
#define HIGH_SPEED_SUPPORT    2

#define SD_SWITCH_CMD_HS_GET    0x00FFFFF0U
#define SD_SWITCH_CMD_HS_SET    0x80FFFFF1U

#define MMC_EXT_CSD_WRITE_BYTE	        3
#define MMC_EXT_CSD_BUS_WIDTH_BYTE      183 /* Bus Width Mode */
#define MMC_EXT_CSD_BUS_WIDTH_1BIT      0   /* Card is in 1 bit mode */
#define MMC_EXT_CSD_BUS_WIDTH_4BITS     1   /* Card is in 4 bit mode */
#define MMC_EXT_CSD_BUS_WIDTH_8BUTS     2   /* Card is in 8 bit mode */

#define MMC_EXT_CSD_HS_TIMING_BYTE      185
#define MMC_EXT_CSD_HS_TIMING_DEF       0
#define MMC_EXT_CSD_HS_TIMING_HIGH      1   /* Card is in high speed mode */
#define MMC_EXT_CSD_HS_TIMING_HS200     2   /* Card is in HS200 mode */

#define MMC_EXT_CSD_LEN                  512
#define MMC_EXT_CSD_SEC_CNT              212
#define MMC_EXT_CSD_CARD_TYPE            196
#define MMC_EXT_CSD_REV		             192
#define EXT_CSD_DEVICE_TYPE_HIGH_SPEED   0x2


#define MMC_4_BIT_BUS_ARG    (((u32)MMC_EXT_CSD_WRITE_BYTE << 24) \
					          | ((u32)MMC_EXT_CSD_BUS_WIDTH_BYTE << 16) \
					          | ((u32)MMC_EXT_CSD_BUS_WIDTH_4BITS << 8))

#define MMC_HIGH_SPEED_ARG  (((u32)MMC_EXT_CSD_WRITE_BYTE << 24) \
                              | ((u32)MMC_EXT_CSD_HS_TIMING_BYTE << 16) \
                              | ((u32)MMC_EXT_CSD_HS_TIMING_HIGH << 8))

/**************************** Type Definitions *******************************/

/*****************************************************************************
* DESCRIPTION
*  This data type is used to define the return codes.
*
*****************************************************************************/
typedef enum {
    SDMMC_OK = 0,           /**< Operation OK */
    SDMMC_LOCKED = 1,       /**< Failed because driver locked */
    SDMMC_BUSY = 2,         /**< Failed because driver busy */
    SDMMC_NO_RESPONSE = 3,  /**< Failed because card not respond */
    SDMMC_CHANGED,          /**< Setting param changed due to limitation */
    SDMMC_ERR,              /**< Failed with general error */
    SDMMC_ERR_IO,           /**< Failed because of IO error */
    SDMMC_ERR_RESP,         /**< Error reported in response code */
    SDMMC_NOT_INITIALIZED,  /**< Fail to initialize */
    SDMMC_PARAM,            /**< Parameter error */
    SDMMC_STATE,            /**< State error */
    SDMMC_USER_CANCEL,      /**< Canceled by user */
    SDMMC_NOT_SUPPORTED     /**< Command(Operation) not supported */
} eSDMMC_RC;

/*****************************************************************************
* DESCRIPTION
*  This data type is used to select the transfer mode.
*
*****************************************************************************/
enum SDMMC_TransMode {
    sdmmc_trans_mode_normal = 0,
    sdmmc_trans_mode_dw_dma = 1
};

/*****************************************************************************
* DESCRIPTION
* This typedef contains configuration information for the device.
*
*****************************************************************************/
typedef struct _sdmmc_config {
	u16 DeviceId;			/* Unique ID  of device */
	u32 BaseAddress;		/* Base address of the device */
	u32 InputClockHz;		/* Input clock frequency */
	u32 CardDetect;			/* Card Detect */
	u32 WriteProtect;		/* Write Protect */
} FSdmmcPs_Config;

/*****************************************************************************
* DESCRIPTION
*  This is the primary structure used when dealing with SDMMC controller.
*  It allows this code to support more than one device of the same type
*  simultaneously.  This structure needs to be initialized with
*  meaningful values before a pointer to it is passed to a driver
*  initialization function.
* PARAMETERS
*  name            name of device
*  dataWidth       bus data width of the device
*  baseAddress     physical base address of device
*  clk_in          the working clock of sd controller
*  instance        device private data structure pointer
*  compVersion     device version identification number
*  compType        device identification number
*
*****************************************************************************/
typedef struct _sdmmc_device {
    struct _sdmmc_config config;
    struct _sdmmc_instance *instance;
    uint32_t comp_version;
    enum FMSH_compType comp_type;
    FMSH_listHead list;
}FSdmmcPs_T;

/*****************************************************************************
* DESCRIPTION
*  This structure contains the command request information.
*
*****************************************************************************/
typedef struct _sdmmc_cmd_s {
	char cmd; /* command */
	unsigned int arg; /* command argument */
	char res_type; /* response type */
	char res_flag; /* response flag */
	u32 rsp_buf[4]; /* response buffer */
	char *data; /* pointer to data buffer */
	unsigned int data_len; /* data length */
	unsigned int block_size; /* block size */
	int t_res; /* timing between command and response */
	int t_fin; /* timing after response */
    u8 ret_status; /* Command return status */
}FSdmmcPs_SdCmd_T;

/*****************************************************************************
* DESCRIPTION
*  This structure is instance data. The user is required to allocate a
*  variable of this type for every SD device in the system. A pointer
*  to a variable of this type is then passed to the driver API functions.
*
*****************************************************************************/
typedef struct _sdmmc_instance {
	FSdmmcPs_SdCmd_T sdCmd;
	FDmaPs_T *pDma;
	/* power up */
	void (*powerUp)(FSdmmcPs_T *pSdmmc);
	/* power down */
	void (*powerDown)(FSdmmcPs_T *pSdmmc);
	/* wait for reset */
	void (*waitForReset)(FSdmmcPs_T *pSdmmc);
	/* set clock rate */
	int (*setClock)(FSdmmcPs_T *pSdmmc, int rate);
	/*set buswidth*/
	void (*setBuswidth)(FSdmmcPs_T *pSdmmc, unsigned int width);
	/* send command and receive response */
	int (*sendCmd)(FSdmmcPs_T *pSdmmc);
	/* transfer data for Memory Card */
	int (*doTransfer)(FSdmmcPs_T *pSdmmc, unsigned int address, unsigned int len,  int rw);
	/* MMC card information */
	unsigned int read_blk_len;      /* read block length */
	unsigned int write_blk_len;     /* write block length */
	u64 capacity;          			/* total size of card in bytes */
	volatile unsigned int stat;     /* card status */
	unsigned int card_type;
	unsigned int slot_type;

	unsigned short rca;
	unsigned int  ocr;
	u32 raw_cid[4];     /* raw card CID */
	u32 raw_csd[4];     /* raw card CSD */
	u8  raw_scr[8];     /* raw card SCR */
	SDMMC_cid_T cid;
	SDMMC_csd_T csd;
	SDMMC_scr_T scr;
	u8 ext_csd[MMC_EXT_CSD_LEN];    /* Extended CSD information */
	unsigned int  r1;
	unsigned int transLen;
	void *priv;
	u32 version;
	unsigned char HighCapacity;
	enum SDMMC_TransMode transMode;
}FSdmmcPs_Instance_T;


/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

FSdmmcPs_Config *FSdmmcPs_LookupConfig(u16 DeviceId);
void FSdmmcPs_initDev(FSdmmcPs_T *pSdmmc, FSdmmcPs_Instance_T *pInstance, FSdmmcPs_Config *pSdCfg);
int FSdmmcPs_cardDetect(FSdmmcPs_T *pSdmmc);
void FSdmmcPs_resetController(u8 sd_index);
void FSdmmcPs_instanceReset(FSdmmcPs_T *pSdmmc, FDmaPs_T *pDmac);
void FSdmmcPs_setDmaMsize(FSdmmcPs_T *pSdmmc, u32 dma_msize);
void FSdmmcPs_setRXwmark(FSdmmcPs_T *pSdmmc, u16 rx_wmark);
void FSdmmcPs_setTXwmark(FSdmmcPs_T *pSdmmc, u16 tx_wmark);
void FSdmmcPs_setCardVoltage(FSdmmcPs_T *pSdmmc, u8 vol);
void FSdmmcPs_powerDown(FSdmmcPs_T *pSdmmc);
void FSdmmcPs_powerUp(FSdmmcPs_T *pSdmmc);
int FSdmmcPs_setClock(FSdmmcPs_T *pSdmmc, int rate);
void FSdmmcPs_setCtrlBuswidth(FSdmmcPs_T *pSdmmc, unsigned int width);
int FSdmmcPs_sdControllerReset(FSdmmcPs_T *pSdmmc);
int FSdmmcPs_sdFifoReset(FSdmmcPs_T *pSdmmc);
int FSdmmcPs_sdDmaReset(FSdmmcPs_T *pSdmmc);
u32 FSdmmcPs_getIntMask(FSdmmcPs_T *pSdmmc);
void FSdmmcPs_setIntMask(FSdmmcPs_T *pSdmmc, u32 int_mask);
u32 FSdmmcPs_getRawIntStat(FSdmmcPs_T *pSdmmc);
void FSdmmcPs_setRawIntStat(FSdmmcPs_T *pSdmmc, u32 raw_int);
u32 FSdmmcPs_getMaskIntStat(FSdmmcPs_T *pSdmmc);
void FSdmmcPs_enableInt(FSdmmcPs_T *pSdmmc);
void FSdmmcPs_enableDMA(FSdmmcPs_T *pSdmmc);
void FSdmmcPs_disableIntMask(FSdmmcPs_T *pSdmmc);
u8 FSdmmcPs_getTransferMode(FSdmmcPs_T *pSdmmc);
u32 FSdmmcPs_prepareCmd(u32 cardType, char cmd);
int FSdmmcPs_sendCmd(FSdmmcPs_T *pSdmmc);
unsigned int FSdmmcPs_getCardSizeKB(FSdmmcPs_T *pSdmmc);
unsigned int FSdmmcPs_getBlockNumbers(FSdmmcPs_T *pSdmmc);
unsigned int FSdmmcPs_getCardStatus(FSdmmcPs_T *pSdmmc);
int FSdmmcPs_setBlockLen(FSdmmcPs_T *pSdmmc, unsigned int len);
int FSdmmcPs_setCardBusWidth(FSdmmcPs_T *pSdmmc, unsigned int bus_width);
int FSdmmcPs_getMmcExtCsd(FSdmmcPs_T *pSdmmc);
int FSdmmcPs_sdEnumerate(FSdmmcPs_T *pSdmmc);
int FSdmmcPs_mmcEnumerate(FSdmmcPs_T *pSdmmc);
int FSdmmcPs_cardIdentify(FSdmmcPs_T *pSdmmc);
int FSdmmcPs_doTransfer(FSdmmcPs_T *pSdmmc, unsigned int address, unsigned int len,  int rw);
int FSdmmcPs_stopTransfer(FSdmmcPs_T *pSdmmc);
int FSdmmcPs_readFun(FSdmmcPs_T *pSdmmc, u32 address, char *pBuf, u32 len, enum SDMMC_TransMode mode);
int FSdmmcPs_writeFun(FSdmmcPs_T *pSdmmc, u32 address, char *pBuf, u32 len, enum SDMMC_TransMode mode);
int FSdmmcPs_addSdDevice(FSdmmcPs_T *pSdmmc);
int FSdmmcPs_removeSdDevice(FSdmmcPs_T *pSdmmc);
s32 FSdmmcPs_initCard(FSdmmcPs_T *pSdmmc);
s32 FSdmmcPs_cfgInitialize(FSdmmcPs_T *pSdmmc, FDmaPs_T *pDmac, enum SDMMC_TransMode trans_mode);

int SDMMC_decodeCID(FSdmmcPs_T *pSdmmc);
int SDMMC_decodeCSD(FSdmmcPs_T *pSdmmc);
int SDMMC_decodeSCR(FSdmmcPs_T *pSdmmc);
int SDMMC_getCSDInfo(FSdmmcPs_T *pSdmmc);

#endif /* #ifndef _FMSH_SDMMC_H_ */

