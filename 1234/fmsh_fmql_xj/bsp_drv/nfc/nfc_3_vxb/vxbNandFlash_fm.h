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

#ifndef __INC_VXB_NANDFLASH_H__
#define __INC_VXB_NANDFLASH_H__

#include "vxbNfc_fm.h"

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
} vxbT_CMDIDX_TO_REGVAL;

#endif

/**********************************Type Definition******************************/

/**********************************Macro (inline function) Definition***********/

/**********************************Variable Definition**************************/

/**********************************Function Prototype***************************/

void vxNand_Init_BadBlkTable_Desc(VXB_DEVICE_ID pDev);
int vxNand_Creat_BadBlkTable(VXB_DEVICE_ID pDev);
int vxNand_Scan_BadBlkTable(VXB_DEVICE_ID pDev);
int vxNand_Rd_BadBlkTable(VXB_DEVICE_ID pDev);
int vxNand_Search_BadBlkTable(VXB_DEVICE_ID pDev, vxbT_BBT_DESC* pDesc);
int vxNand_Wr_BadBlkTable(VXB_DEVICE_ID pDev, vxbT_BBT_DESC* pDesc, vxbT_BBT_DESC* pDescMirror);
int vxNand_Mark_BadBlkTable(VXB_DEVICE_ID pDev, vxbT_BBT_DESC* pDesc);
int vxNand_Update_BadBlkTable(VXB_DEVICE_ID pDev);
int vxNand_Chk_BadBlock(VXB_DEVICE_ID pDev, UINT16 block);
int vxNand_Mark_BadBlock(VXB_DEVICE_ID pDev, UINT32 block);

UINT32 vxNandInfo_Get_DevSize_Blks(VXB_DEVICE_ID pDev);
UINT32 vxNandInfo_Get_DevSize_Pages(VXB_DEVICE_ID pDev);
UINT32 vxNandInfo_Get_DevSize_MB(VXB_DEVICE_ID pDev);
UINT64 vxNandInfo_Get_DevSize_Bytes(VXB_DEVICE_ID pDev);
UINT32 vxNandInfo_Get_BlkSize_Pages(VXB_DEVICE_ID pDev);
UINT32 vxNandInfo_Get_BlkSize_KB(VXB_DEVICE_ID pDev);
UINT32 vxNandInfo_Get_BlkSize(VXB_DEVICE_ID pDev);
UINT32 vxNandInfo_Get_PageSize(VXB_DEVICE_ID pDev);
UINT32 vxNandInfo_Get_SpareSize(VXB_DEVICE_ID pDev);
UINT32 vxNandInfo_Get_DeviceId(VXB_DEVICE_ID pDev);
UINT32 vxNandInfo_Get_RowAddrCycle(VXB_DEVICE_ID pDev);
UINT32 vxNandInfo_Get_ColAddrCycle(VXB_DEVICE_ID pDev);
UINT32 vxNandInfo_Get_BusWidth(VXB_DEVICE_ID pDev);
int vxNandInfo_isSmallBlks(VXB_DEVICE_ID pDev);
UINT32 vxNandInfo_Get_LunNum(VXB_DEVICE_ID pDev);

int vxNandInfo_Parse_FlashAddr(VXB_DEVICE_ID pDev, UINT64 Address, UINT32 Size, 
									UINT16* Block, UINT16* Page, UINT16* Offset);
             
#endif	/* __INC_VXB_NANDFLASH_H__ */


