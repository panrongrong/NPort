/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_sdmmc_protocol.h
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
#ifndef _FMSH_SDMMC_PROTOCOL_H_
#define _FMSH_SDMMC_PROTOCOL_H_

/***************************** Include Files *********************************/

/************************** Constant Definitions *****************************/

/* Standard commands                     type  argument     response */
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

/*#define CMD_ERASE                37   /* ac                      R1b */
#define CMD_ERASE                38   /* ac                      R1b */

#define CMD_FAST_IO              39   /* ac   <Complex>          R4  */
#define CMD_GO_IRQ_STATE         40   /* bcr                     R5  */
#define CMD_LOCK_UNLOCK          42   /* adtc                    R1b */
#define CMD_APP_CMD              55   /* ac   [31:16] RCA        R1  */
#define CMD_GEN_CMD              56   /* adtc [0] RD/WR          R1b */

  /* Application commands */
#define CMD_SWITCH_AND_ACMD_SET_BUS_WIDTH   6    /* ac   [1:0] bus width    R1  */
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
#define R1_ERR		(1 << 19)	/* erx, c */
#define R1_UNDERRUN		(1 << 18)	/* ex, c */
#define R1_OVERRUN		(1 << 17)	/* ex, c */
#define R1_CID_CSD_OVERWRITE	(1 << 16)	/* erx, c, CID/CSD overwrite */
#define R1_WP_ERASE_SKIP	(1 << 15)	/* sx, c */
#define R1_CARD_ECC_DISABLED	(1 << 14)	/* sx, a */
#define R1_ERASE_RESET		(1 << 13)	/* sr, c */
#define R1_STATUS(x)            (x & 0xFFFFE000)
#define R1_CURRENT_STATE(x)    	((x & 0x00001E00) >> 9)	/* sx, b (4 bits) */
#define R1_READY_FOR_DATA	(1 << 8)	/* sx, a */
#define R1_APP_CMD		(1 << 7)	/* sr, c */

/* card state flags of R1 */
#define STATE_IDLE	0x00000000	/* 0 */
#define STATE_READY	0x00000200	/* 1 */
#define STATE_IDENT	0x00000400	/* 2 */
#define STATE_STBY	0x00000600	/* 3 */
#define STATE_TRAN	0x00000800	/* 4 */
#define STATE_DATA	0x00000A00	/* 5 */
#define STATE_RCV	0x00000C00	/* 6 */
#define STATE_PRG	0x00000E00	/* 7 */
#define STATE_DIS	0x00001000	/* 8 */

#define OCR_BUSY		0x80000000	/* Card Power up status bit */
#define OCR_HCS			0x40000000  /* Card capacity status bit */



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

/* SD/MMC version bits; 8 flags, 8 major, 8 minor, 8 change */
#define SD_VERSION_SD	(1U << 31)
#define MMC_VERSION_MMC	(1U << 30)

#define MAKE_SDMMC_VERSION(a, b, c)	\
	((((u32)(a)) << 16) | ((u32)(b) << 8) | (u32)(c))
#define MAKE_SD_VERSION(a, b, c)	\
	(SD_VERSION_SD | MAKE_SDMMC_VERSION(a, b, c))
#define MAKE_MMC_VERSION(a, b, c)	\
	(MMC_VERSION_MMC | MAKE_SDMMC_VERSION(a, b, c))

#define SD_VERSION_3		MAKE_SD_VERSION(3, 0, 0)
#define SD_VERSION_2		MAKE_SD_VERSION(2, 0, 0)
#define SD_VERSION_1_0		MAKE_SD_VERSION(1, 0, 0)
#define SD_VERSION_1_10		MAKE_SD_VERSION(1, 10, 0)

#define MMC_VERSION_UNKNOWN	MAKE_MMC_VERSION(0, 0, 0)
#define MMC_VERSION_1_2		MAKE_MMC_VERSION(1, 2, 0)
#define MMC_VERSION_1_4		MAKE_MMC_VERSION(1, 4, 0)
#define MMC_VERSION_2_2		MAKE_MMC_VERSION(2, 2, 0)
#define MMC_VERSION_3		MAKE_MMC_VERSION(3, 0, 0)
#define MMC_VERSION_4		MAKE_MMC_VERSION(4, 0, 0)
#define MMC_VERSION_4_1		MAKE_MMC_VERSION(4, 1, 0)
#define MMC_VERSION_4_2		MAKE_MMC_VERSION(4, 2, 0)
#define MMC_VERSION_4_3		MAKE_MMC_VERSION(4, 3, 0)
#define MMC_VERSION_4_4		MAKE_MMC_VERSION(4, 4, 0)
#define MMC_VERSION_4_41	MAKE_MMC_VERSION(4, 4, 1)
#define MMC_VERSION_4_5		MAKE_MMC_VERSION(4, 5, 0)
#define MMC_VERSION_5_0		MAKE_MMC_VERSION(5, 0, 0)
#define MMC_VERSION_5_1		MAKE_MMC_VERSION(5, 1, 0)


/**************************** Type Definitions *******************************/

/*****************************************************************************
* DESCRIPTION
*  This structure contains the CID(Card IDentification) information.
*
*****************************************************************************/
typedef struct _mmc_cid {
	int   manfid;
	char  prod_name[8];
	int   serial;
	short oemid;
	short year;
	char  hwrev;
	char  fwrev;
	char  month;
}SDMMC_cid_T;

/*****************************************************************************
* DESCRIPTION
*  This structure contains the CSD(Card Specific Data) information.
*
*****************************************************************************/
typedef struct _mmc_csd {
    unsigned char  csd_struct;
    unsigned char  mmc_version;
    unsigned short cmdclass;
    unsigned short taac_clks;
    unsigned int   taac_ns;
    unsigned int   r2w_factor;
    unsigned int   max_dtr;
    unsigned int   read_blkbits;
    unsigned int   write_blkbits;
    unsigned int   blockNR;
    unsigned int   read_partial:1,
                   read_misalign:1,
                   write_partial:1,
                   write_misalign:1;
}SDMMC_csd_T;

/*****************************************************************************
* DESCRIPTION
*  This structure contains the SCR(SD Card Configuration Register)
*  information. It is used for SD card.
*
*****************************************************************************/
typedef struct _sd_scr {
	unsigned char sd_spec;
	unsigned char bus_width;
	unsigned char sd_spec3;
}SDMMC_scr_T;

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

#endif /* #ifndef _FMSH_SDMMC_PROTOCOL_H_ */


