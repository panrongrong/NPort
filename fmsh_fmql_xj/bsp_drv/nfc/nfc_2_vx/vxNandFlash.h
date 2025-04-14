/* vxNandFlash.h - vxNandFlash driver header */

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
01a, 25Dec19, jc  written.
*/

#ifndef __INC_VX_NANDFLASH_H__
#define __INC_VX_NANDFLASH_H__

#include "vxNfc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/**********************************Include File*********************************/

/**********************************Constant Definition**************************/
#define NANDFLASH_BBT_SCAN_2ND_PAGE		        1	/**< Scan the second page for bad block information */
#define NANDFLASH_BBT_BLOCK_OFFSET		        0 	/**< Block offset of Bad Block Table Desc */
#define NANDFLASH_BBT_SIG_OFFSET		        8 	/**< Bad Block Table signature offset */
#define NANDFLASH_BBT_VER_OFFSET		        12	/**< Bad block Table version offset */
#define NANDFLASH_BBT_SIG_LEN		            4	/**< Bad block Table signature length */
#define NANDFLASH_BBT_MAX_BLOCKS		        3	/**< Bad block Table max blocks */

#define NANDFLASH_BB_PATTERN_OFFSET_SMALL_PAGE	5	/**< Bad block pattern offset in a page */
#define NANDFLASH_BB_PATTERN_LENGTH_SMALL_PAGE	1	/**< Bad block pattern length */
#define NANDFLASH_BB_PATTERN_OFFSET_LARGE_PAGE	0	/**< Bad block pattern offset in a large page */
#define NANDFLASH_BB_PATTERN_LENGTH_LARGE_PAGE	2	/**< Bad block pattern length */
#define NANDFLASH_BB_PATTERN			        0xFF	/**< Bad block pattern to search in a page */  
/*
 * Block definitions for RAM based Bad Block Table (BBT)
 */
#define NANDPS_BLOCK_GOOD			0x3	/**< Block is good */
#define NANDPS_BLOCK_BAD			0x2	/**< Block is bad */
#define NANDPS_BLOCK_RESERVED       0x1
#define NANDPS_BLOCK_FACTORY_BAD    0x0

#define NANDPS_BBT_BLOCK_SHIFT		2	/**< 1 byte represent 4 block in BBT */
#define NANDPS_BLOCK_TYPE_MASK		0x03	/**< Block type mask */
#define NANDPS_BLOCK_SHIFT_MASK		0x06	/**< Block shift mask for a Bad Block Table entry byte */ 



#if 1
/* 
nandflash cmd idx 
*/
#define NAND_SPANSION_ID                             0x01
#define NAND_MICRON_ID                               0x2C
#define NAND_FMSH_ID                                 0xA1
    
#define NANDFLASH_ONFI_ID                           0x49464E4F 

/* Reset commands*/
#define NANDFLASH_RESET                         0
#define NANDFLASH_SYNCH_RESET                   1
#define NANDFLASH_LUN_RESET                     2

/* Identification operations */
#define NANDFLASH_READ_ID                       3
#define NANDFLASH_READ_PARAMETER_PAGE           4
#define NANDFLASH_UNIQUE_ID                     5

/* Configuration operations*/
#define NANDFLASH_GET_FEATURE                   6
#define NANDFLASH_SET_FEATURE                   7

/* Status Operations */
#define NANDFLASH_READ_STATUS                   8
#define NANDFLASH_SELECT_LUN_WITH_STATUS        9
#define NANDFLASH_LUN_STATUS                    10
#define NANDFLASH_DEVICE_STATUS                 11
#define NANDFLASH_VOLUME_SELECT                 12

/* Column adress operations*/
#define NANDFLASH_CHANGE_READ_COLUMN            13
#define NANDFLASH_SELECT_CACHE_REGISTER         14
#define NANDFLASH_CHANGE_WRITE_COLUMN           15
#define NANDFLASH_CHANGE_ROW_ADDRESS            16

/* Read operations*/
#define NANDFLASH_READ_PAGE                     17
#define NANDFLASH_READ_PAGE_CACHE               18
#define NANDFLASH_READ_PAGE_CACHE_LAST          19
#define NANDFLASH_READ_MULTIPLANE               20
#define NANDFLASH_TWO_PLANE_PAGE_READ           21
#define NANDFLASH_QUEUE_PAGE_READ               22

/* Program operations*/
#define NANDFLASH_PROGRAM_PAGE                  23
#define NANDFLASH_PROGRAM_PAGE_IMD              24
#define NANDFLASH_PROGRAM_PAGE_DEL              25 
#define NANDFLASH_PROGRAM_PAGE_1                26
#define NANDFLASH_PROGRAM_PAGE_CACHE            27
#define NANDFLASH_PROGRAM_MULTIPLANE            28
#define NANDFLASH_WRITE_PAGE                    29
#define NANDFLASH_WRITE_PAGE_CACHE              30
#define NANDFLASH_WRITE_MULTIPLANE              31

/* Erase operations*/
#define NANDFLASH_ERASE_BLOCK                   32
#define NANDFLASH_ERASE_MULTIPLANE              33

/* Copyback operations*/
#define NANDFLASH_COPYBACK_READ                 34
#define NANDFLASH_COPYBACK_PROGRAM              35
#define NANDFLASH_COPYBACK_PROGRAM_1            36
#define NANDFLASH_COPYBACK_MULTIPLANE           37

/* OTP operations*/
#define NANDFLASH_PROGRAM_OTP                   38
#define NANDFLASH_DATA_PROTECT_OTP              39
#define NANDFLASH_READ_PAGE_OTP                 40

/*
nandflash cmd_idx to nfc_ctrl reg_val
*/
typedef struct
{
  UINT32 cmd_idx;
  UINT32 reg_val;
} vxT_CMDIDX_TO_REGVAL;

#endif

/**********************************Type Definition******************************/

/**********************************Macro (inline function) Definition***********/

/**********************************Variable Definition**************************/

/**********************************Function Prototype***************************/

void nand_Init_BadBlkTable_Desc(vxT_NFC* pNfc);
int nand_Creat_BadBlkTable(vxT_NFC* pNfc);
int nand_Scan_BadBlkTable(vxT_NFC* pNfc);
int nand_Rd_BadBlkTable(vxT_NFC* pNfc);
int nand_Search_BadBlkTable(vxT_NFC* pNfc, vxT_BBT_DESC* pDesc);
int nand_Wr_BadBlkTable(vxT_NFC* pNfc, vxT_BBT_DESC* pDesc, vxT_BBT_DESC* pDescMirror);
int nand_Mark_BadBlkTable(vxT_NFC* pNfc, vxT_BBT_DESC* pDesc);
int nand_Update_BadBlkTable(vxT_NFC* pNfc);
int nand_Chk_BadBlock(vxT_NFC* pNfc, UINT16 block);
int nand_Mark_BadBlock(vxT_NFC* pNfc, UINT32 block);

UINT32 nandInfo_Get_DevSize_Blks(vxT_NFC* pNfc);
UINT32 nandInfo_Get_DevSize_Pages(vxT_NFC* pNfc);
UINT32 nandInfo_Get_DevSize_MB(vxT_NFC* pNfc);
UINT64 nandInfo_Get_DevSize_Bytes(vxT_NFC* pNfc);
UINT32 nandInfo_Get_BlkSize_Pages(vxT_NFC* pNfc);
UINT32 nandInfo_Get_BlkSize_KB(vxT_NFC* pNfc);
UINT32 nandInfo_Get_BlkSize(vxT_NFC* pNfc);
UINT32 nandInfo_Get_PageSize(vxT_NFC* pNfc);
UINT32 nandInfo_Get_SpareSize(vxT_NFC* pNfc);
UINT32 nandInfo_Get_DeviceId(vxT_NFC* pNfc);
UINT32 nandInfo_Get_RowAddrCycle(vxT_NFC* pNfc);
UINT32 nandInfo_Get_ColAddrCycle(vxT_NFC* pNfc);
UINT32 nandInfo_Get_BusWidth(vxT_NFC* pNfc);
int nandInfo_isSmallBlks(vxT_NFC* pNfc);
UINT32 nandInfo_Get_LunNum(vxT_NFC* pNfc);
int nandInfo_Parse_FlashAddr(vxT_NFC* pNfc, UINT64 Address, UINT32 Size, 
									UINT16* Block, UINT16* Page, UINT16* Offset);


             
#endif	/* prevent circular inclusions */


