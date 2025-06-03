/* vxSdmmc.h - vx_sdmmc driver header */

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
01a, 22Nov19, jc  written.
*/

#ifndef __INC_VX_SDMMC2_H__
#define __INC_VX_SDMMC2_H__


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define FM_SDMMC_NAME         "fmSdmmc"

/* snoop enabled, no need to cache flush or invalidate */
#define FM_SDMMC_FLAGS_SNOOP_ENABLED    (1 << 0)

/* no card detection, card always present, no hot plug support */
#define FM_SDMMC_FLAGS_CARD_PRESENT     (1 << 1)

/* no card write protect pin connected, always writable */
#define FM_SDMMC_FLAGS_CARD_WRITABLE    (1 << 2)


#define FMSH_DEVIP_DEV_NONE	  (0x00000000)
#define FMSH_DEVIP_APB_SMC	  (0x44572110)
#define FMSH_DEVIP_APB_DMAC	  (0x44571110)
#define FMSH_DEVIP_APB_ICTL	  (0x44571120)  /*???*/
#define FMSH_DEVIP_APB_PCI	  (0x44571130)
#define FMSH_DEVIP_APB_USB	  (0x44571140)
#define FMSH_DEVIP_APB_SDMMC  (0x44571150)
#define FMSH_DEVIP_APB_GPIO	  (0x44570160)
#define FMSH_DEVIP_APB_I2C	  (0x44570140)
#define FMSH_DEVIP_APB_ICTL	  (0x44570180)  /* ????*/
#define FMSH_DEVIP_APB_RAP	  (0x44570190)
#define FMSH_DEVIP_APB_RTC	  (0x44570130)
#define FMSH_DEVIP_APB_SSI	  (0x44570150)
#define FMSH_DEVIP_APB_TIMERS (0x44570170)
#define FMSH_DEVIP_APB_UART	  (0x44570110)
#define FMSH_DEVIP_APB_WDT	  (0x44570120)


#define  SDMMC_CTRL_0                  (0)
#define  SDMMC_CTRL_1                  (1)

#define  VX_SDMMC_0_BASE                  (0xE0043000)
#define  VX_SDMMC_1_BASE                  (0xE0044000)

/* ps -> sdio -> sdmmc_ctrl*/
#define  PS_SDIO_CLK_FREQ_HZ        (100000000)  /* 100M Hz*/

#define  MODE_XFER_FIFO        (0)  /* FIFO*/
#define  MODE_XFER_DMA         (1)  /* DMA*/

#define SDMMC_POLL_TIMEOUT      (500)


#define SDMMC_CARD_VER_1_0   (1)
#define SDMMC_CARD_VER_2_0   (2)

enum  gSdmmc_Resp_Type2
{
	CMD_RESP_TYPE_NONE = 0,
	CMD_RESP_TYPE_R1,
	CMD_RESP_TYPE_R1B,
	CMD_RESP_TYPE_R2,
	CMD_RESP_TYPE_R3,
	CMD_RESP_TYPE_R4,
	CMD_RESP_TYPE_R5,
	CMD_RESP_TYPE_R6,
	CMD_RESP_TYPE_R7
};


/* 
MMC Response length 
*/
#define CMD_RES_LEN_SHORT		6
#define CMD_RES_LEN_LONG		4

const int vxb_gSdmmc_Resp_Len_Type[] = 
{
    0,
    CMD_RES_LEN_SHORT,
    CMD_RES_LEN_SHORT,
    CMD_RES_LEN_LONG,
    CMD_RES_LEN_SHORT,
    CMD_RES_LEN_SHORT,
    CMD_RES_LEN_SHORT,
    CMD_RES_LEN_SHORT,
    CMD_RES_LEN_SHORT
};


#define SDMMC_LOOP_TIMEOUT      500

/** Default block size for SD/MMC access */
#define SDMMC_BLOCK_SIZE        512
#define SDMMC_MAX_DIV_CNT       255

/* 
CMD Response flag 
*/
#define CMD_RESP_FLAG_NONE       0x00 /* no response */
#define CMD_RESP_FLAG_DATALINE   0x01 /* transferred on dataline */
#define CMD_RESP_FLAG_NOCRC      0x02 /* no crc check */
#define CMD_RESP_FLAG_RDATA      0x04 /* data read */
#define CMD_RESP_FLAG_WDATA      0x08 /* data write */

/* 
mmc card type
*/
#define MMC	    0x1
#define SD	    0X2
#define SDIO	0x3
#define CEATA	0x4
#define EMMC    0x5

#define TYPE_SLOT    0x0
#define TYPE_CHIP    0x1

#define CARD_INSERT     0x0
#define CARD_UNINSERT   0x1

/* 
flags for stat field of the mmc_slot structure 
*/
#define MMC_WP_GRP_EN		0x00000010
#define MMC_PERM_WP		    0x00000020
#define MMC_TMP_WP		    0x00000040
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
#define MMC_EXT_CSD_BUS_WIDTH_BYTE      183  /* Bus Width Mode */

#define MMC_EXT_CSD_BUS_WIDTH_1BIT      0    /* Card is in 1 bit mode */
#define MMC_EXT_CSD_BUS_WIDTH_4BITS     1    /* Card is in 4 bit mode */
#define MMC_EXT_CSD_BUS_WIDTH_8BUTS     2    /* Card is in 8 bit mode */

#define MMC_EXT_CSD_HS_TIMING_BYTE      185
#define MMC_EXT_CSD_HS_TIMING_DEF       0
#define MMC_EXT_CSD_HS_TIMING_HIGH      1    /* Card is in high speed mode */
#define MMC_EXT_CSD_HS_TIMING_HS200     2    /* Card is in HS200 mode */

#define MMC_EXT_CSD_LEN                  512
#define MMC_EXT_CSD_CARD_TYPE            196
#define EXT_CSD_DEVICE_TYPE_HIGH_SPEED   0x2


#define MMC_4_BIT_BUS_ARG    (((u32)MMC_EXT_CSD_WRITE_BYTE << 24) \
					          | ((u32)MMC_EXT_CSD_BUS_WIDTH_BYTE << 16) \
					          | ((u32)MMC_EXT_CSD_BUS_WIDTH_4BITS << 8))

#define MMC_HIGH_SPEED_ARG  (((u32)MMC_EXT_CSD_WRITE_BYTE << 24) \
                              | ((u32)MMC_EXT_CSD_HS_TIMING_BYTE << 16) \
                              | ((u32)MMC_EXT_CSD_HS_TIMING_HIGH << 8))

/************************** Constant Definitions *****************************/

/* Standard commands                     type  argument     response */
/*-------------------------------------------------------------------*/
#define	CMD_GO_IDLE_STATE         0   /* bc                          */
#define CMD_SEND_OP_COND          1   /* bcr  [31:0] OCR         R3  */
#define CMD_ALL_SEND_CID          2   /* bcr                     R2  */
#define CMD_SET_RELATIVE_ADDR     3   /* ac   [31:16] RCA        R6  */
#define CMD_SET_DSR               4   /* bc   [31:16] RCA            */
#define CMD_IO_SEND_OP_COND       5   /*      ONLY USE SDIO CARD     */
#define CMD_SELECT_CARD           7   /* ac   [31:16] RCA        R1  */
#define CMD_SEND_IF_COND          8   /* bcr                     R7  */
#define CMD_SEND_CSD              9   /* ac   [31:16] RCA        R2  */
#define CMD_SEND_CID             10   /* ac   [31:16] RCA        R2  */
#define CMD_READ_DAT_UNTIL_STOP  11   /* adtc [31:0] dadr        R1  */
#define CMD_STOP_TRANSMISSION    12   /* ac                      R1b */
#define CMD_SEND_STATUS	         13   /* ac   [31:16] RCA        R1  */
#define CMD_GO_INACTIVE_STATE    15   /* ac   [31:16] RCA            */
#define CMD_SET_BLOCKLEN         16   /* ac   [31:0] block len   R1  */
#define CMD_READ_SINGLE_BLOCK    17   /* adtc [31:0] data addr   R1  */
#define CMD_READ_MULTIPLE_BLOCK  18   /* adtc [31:0] data addr   R1  */
#define CMD_WRITE_DAT_UNTIL_STOP 20   /* adtc [31:0] data addr   R1  */
#define CMD_SET_BLOCK_COUNT      23   /* adtc [31:0] data addr   R1  */
#define CMD_WRITE_BLOCK          24   /* adtc [31:0] data addr   R1  */
#define CMD_WRITE_MULTIPLE_BLOCK 25   /* adtc                    R1  */
#define CMD_PROGRAM_CID          26   /* adtc                    R1  */
#define CMD_PROGRAM_CSD          27   /* adtc                    R1  */
#define CMD_SET_WRITE_PROT       28   /* ac   [31:0] data addr   R1b */
#define CMD_CLR_WRITE_PROT       29   /* ac   [31:0] data addr   R1b */
#define CMD_SEND_WRITE_PROT      30   /* adtc [31:0] wpdata addr R1  */
#define CMD_ERASE_GROUP_START    35   /* ac   [31:0] data addr   R1  */
#define CMD_ERASE_GROUP_END      36   /* ac   [31:0] data addr   R1  */
#define CMD_ERASE                37   /* ac                      R1b */
#define CMD_FAST_IO              39   /* ac   <Complex>          R4  */
#define CMD_GO_IRQ_STATE         40   /* bcr                     R5  */
#define CMD_LOCK_UNLOCK          42   /* adtc                    R1b */
#define CMD_APP_CMD              55   /* ac   [31:16] RCA        R1  */
#define CMD_GEN_CMD              56   /* adtc [0] RD/WR          R1b */

/* 
Application commands 
*/
#define CMD_ACMD_SET_BUS_WIDTH   6    /* ac   [1:0] bus width    R1  */
#define ACMD_OP_COND             41   /* bcr  [31:0] OCR         R3  */
#define ACMD_SEND_SCR            51   /* adtc                    R1  */

#define CMD_SWITCH_FUNC          (6 << 4)  /* adtc                    R1  */
#define MMC_SEND_EXT_CSD         (8 << 4)  /* adtc                    R1  */

#define CMD_SWITCH_FUNC_MMC      (6)  /* jc: adtc                    R1  */

/*
MMC status in R1
Type
	e : error bit
	s : status bit
	r : detected and set for the actual command response
	x : detected and set during command execution. the host must poll
        the card by sending status command in order to read these bits.
Clear condition
	a : according to the card state
	b : always related to the previous command. Reception of
	        a valid command will clear it (with a delay of one command)
	c : clear by read
 */
#define R1_OUT_OF_RANGE		(1 << 31)	/* er, c */
#define R1_ADDRESS_ERROR	(1 << 30)	/* erx, c */
#define R1_BLOCK_LEN_ERROR	(1 << 29)	/* er, c */
#define R1_ERASE_SEQ_ERROR      (1 << 28)	/* er, c */
#define R1_ERASE_PARAM		(1 << 27)	/* ex, c */
#define R1_WP_VIOLATION		(1 << 26)	/* erx, c */
#define R1_CARD_IS_LOCKED	(1 << 25)	/* sx, a */
#define R1_LOCK_UNLOCK_FAILED	(1 << 24)	/* erx, c */
#define R1_COM_CRC_ERROR	(1 << 23)	/* er, b */
#define R1_ILLEGAL_COMMAND	(1 << 22)	/* er, b */
#define R1_CARD_ECC_FAILED	(1 << 21)	/* ex, c */
#define R1_CC_ERR		(1 << 20)	/* erx, c */
#define R1_ERR		    (1 << 19)	/* erx, c */
#define R1_UNDERRUN		(1 << 18)	/* ex, c */
#define R1_OVERRUN		(1 << 17)	/* ex, c */
#define R1_CID_CSD_OVERWRITE	(1 << 16)	/* erx, c, CID/CSD overwrite */
#define R1_WP_ERASE_SKIP	(1 << 15)	/* sx, c */
#define R1_CARD_ECC_DISABLED	(1 << 14)	/* sx, a */
#define R1_ERASE_RESET		(1 << 13)	/* sr, c */
#define R1_STATUS(x)            (x & 0xFFFFE000)
#define R1_CURRENT_STATE(x)    	((x & 0x00001E00) >> 9)	/* sx, b (4 bits) */
#define R1_READY_FOR_DATA	(1 << 8)	/* sx, a */
#define R1_APP_CMD		    (1 << 7)	/* sr, c */

/* 
card state flags of R1 
*/
#define STATE_IDLE	0x00000000	/* 0 */
#define STATE_READY	0x00000200	/* 1 */
#define STATE_IDENT	0x00000400	/* 2 */
#define STATE_STBY	0x00000600	/* 3 */
#define STATE_TRAN	0x00000800	/* 4 */
#define STATE_DATA	0x00000A00	/* 5 */
#define STATE_RCV	0x00000C00	/* 6 */
#define STATE_PRG	0x00000E00	/* 7 */
#define STATE_DIS	0x00001000	/* 8 */

#define MMC_CARD_BUSY	0x80000000	/* Card Power up status bit */

/*
 * CSD field definitions
 */
#define CSD_STRUCT_VER_1_0  0           /* Valid for system specification 1.0 - 1.2 */
#define CSD_STRUCT_VER_1_1  1           /* Valid for system specification 1.4 - 2.2 */
#define CSD_STRUCT_VER_1_2  2           /* Valid for system specification 3.1 - 3.2 - 3.31 - 4.0 - 4.1 */
#define CSD_STRUCT_EXT_CSD  3           /* Version is coded in CSD_STRUCTURE in EXT_CSD */

#define CSD_SPEC_VER_0      0           /* Implements system specification 1.0 - 1.2 */
#define CSD_SPEC_VER_1      1           /* Implements system specification 1.4 */
#define CSD_SPEC_VER_2      2           /* Implements system specification 2.0 - 2.2 */
#define CSD_SPEC_VER_3      3           /* Implements system specification 3.1 - 3.2 - 3.31 */
#define CSD_SPEC_VER_4      4           /* Implements system specification 4.0 - 4.1 */


#if 1
/*
sdmmc ctrl
*/
typedef struct _t_vxbSdmmc_ctrl_
{
	int  ctrl_x;  /* 0, 1 */	
	
	UINT32 baseAddr;		/* Base address of the device */
	UINT32 sysClkHz;		/* Input to sdmmc_ctrl clock frequency: sysclk of bus */
	UINT32 isCardDetect;	/* has Card Detect ? */
	UINT32 isWrProtect;	    /* has Write Protect ? */

	UINT32 devIP_ver;
    UINT32 devIP_type;      /* identify peripheral types: FMSH_DEVIP_APB_SDMMC */

	int xferMode;          /* 0-fifo, 1-dma */
	
} vxbT_SDMMC_CTRL;

/*
sdmmc device: card
*/
typedef struct _t_vxbSdmmc_card_
{
	int status;  /* 1-ok */

	
	UINT32 read_blk_len;      /* read block length */
	UINT32 write_blk_len;     /* write block length */
	UINT32 capacity;          /* total size of card in bytes */

	UINT32 card_type;
	UINT32 slot_type;
	
	UINT8  card_ver;   /* 1-ver1.0, 2- ver2.0 */
	UINT8  highCapacity;

	/*
	一、OCR、CID、CSD和SCR寄存器保存卡的配置信息；	
	二、RCA寄存器保存着通信过程中卡当前暂时分配的地址(只适合SD模式，SPI模式下没有)；	
	三、CSR寄存器卡状态(Card Status)和SSR寄存器SD状态(SD Status)寄存器保存着卡的状态(例如：是否写成功，通信的CRC校验是否正确等)，
	    这两个寄存器的内容与通信模式(SD模式或SPI模式)相关。
	*/
	UINT32  rca;  /* relative card address: 地址用于在卡识别后主机利用该地址与卡进行通信 */
	UINT32  ocr;  /* Operating Conditions Register: 储存了卡的 VDD 电压轮廓图 */
	
	UINT32 raw_cid[4];     /* raw card CID : 128 bit */
	UINT32 raw_csd[4];     /* raw card CSD : 128 bit */
	UINT8  raw_scr[8];     /* raw card SCR : 64 bit */
	
	struct {		
		UINT32  manfid;
		UINT8   prod_name[8];
		UINT32  serial;
		UINT16  oemid;
		UINT16  year;
		UINT8   hwrev;
		UINT8   fwrev;
		UINT8   month;
	} info_cid;
	
	struct {		
	    UINT8   csd_struct;
	    UINT8   mmca_vsn;
	    UINT16  cmdclass;
	    UINT16  taac_clks;
	    UINT32  taac_ns;
	    UINT32  r2w_factor;
	    UINT32  max_dtr;
	    UINT32  read_blkbits;
	    UINT32  write_blkbits;
	    UINT32  blockNR;
		
	    UINT32  read_partial:1,
	            read_misalign:1,
	            write_partial:1,
	            write_misalign:1;
	} info_csd;
	
	struct {		
		UINT8  sd_spec;
		UINT8  bus_width;
		UINT8  sd_spec3;
	} info_scr;
	
	UINT8 ext_csd[MMC_EXT_CSD_LEN];    /* Extended CSD information */
	
} vxbT_SDMMC_CARD;


/*
sd/mmc cmd
*/
typedef struct _t_vxbSdmmc_cmd_
{	
	UINT32 cmd_arg;   /* command argument */
	UINT32 cmd_val;   /* command data */
	
	UINT8  cmd_idx;      /* command idx */
	
	UINT8  rsp_type;    /* response type */
	UINT8  rsp_flag;    /* response flag */	
	UINT32 rsp_buf[4];      /* response buffer */
	
	UINT8 * txRxBuf;     /* pointer to data buffer for tx&rx */
	UINT32  data_len;    /* data length */
	
	UINT32 block_size;   /* block size */
	int t_resp;          /* timing between command and response */
	int t_final;         /* timing after response */
	
    int ret_status;     /* Command return status */
} vxbT_SDMMC_CMD;

#endif

typedef struct _t_sdmmc_drv_ctrl2_
{
    VXB_DEVICE_ID    pDev;
    void *           regBase;
    void *           regHandle;
	
    UINT32           clkFrequency;	
    BOOL             polling;
    BOOL             dmaMode;
	int              flags;
	
    BOOL             initDone;	
	int              initPhase;

	/* sdmmc_ctrl: host & master */
	vxbT_SDMMC_CTRL  CTRL;

	/* sdmmc_card: device */
	vxbT_SDMMC_CARD  CARD;

	/* sdmmc_cmd */
	vxbT_SDMMC_CMD   CMD;
	
	UINT32 devIP_ver;
    UINT32 devIP_type;      /* identify peripheral types:  */

	int status;

    SEM_ID           semSync;
    SEM_ID           muxSem;	
} FM_SDMMC_DRV_CTRL2;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_VX_SDMMC2_H__ */

