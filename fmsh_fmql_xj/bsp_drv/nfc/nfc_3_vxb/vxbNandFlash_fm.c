/* vxbNandFlash_fm.c - fmsh nand bad block manage driver */

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

/*
DESCRIPTION
	no vxbus struct
*/

/* includes */

#include <vxWorks.h>
#include <stdio.h>
#include <semLib.h>
#include <sysLib.h>
#include <taskLib.h>
#include <vxBusLib.h>
#include <cacheLib.h>
#include <string.h>
#include <vxbTimerLib.h>

#include "vxbNandFlash_fm.h"

static vxbT_CMDIDX_TO_REGVAL vxbT_FlashCmdIdx_to_NfcRegVal[] = 
{ 
    /*  Reset commands */
    {NANDFLASH_RESET,       0x0000ff00},                /* seq0 */
    {NANDFLASH_SYNCH_RESET, 0x0000fc00},                /* seq0 */
    {NANDFLASH_LUN_RESET,   0x0000fa34},                /* seq20 */
    
    /*  Identification operations */
    {NANDFLASH_READ_ID,             0x00009021},      /* seq1 */
    {NANDFLASH_READ_PARAMETER_PAGE, 0x0000ec22},      /* seq2 */
    {NANDFLASH_UNIQUE_ID,           0x0000ed22},      /* seq2 */
    
    /*  Configuration operations */
    {NANDFLASH_GET_FEATURE, 0x0000ee22},             /* seq2 */
    {NANDFLASH_SET_FEATURE, 0x0000ef03},             /* seq3 */
    
    /*  Status Operations */
    {NANDFLASH_READ_STATUS,            0x00007024},   /* seq4 */
    {NANDFLASH_SELECT_LUN_WITH_STATUS, 0x00007825},   /* seq5 */
    {NANDFLASH_LUN_STATUS,             0x00007125},   /* seq5 */
    {NANDFLASH_DEVICE_STATUS,          0x00007224},   /* seq4 */
    {NANDFLASH_VOLUME_SELECT,          0x0000e115},   /* seq21 */
    
    /*  Column adress operations */
    {NANDFLASH_CHANGE_READ_COLUMN,    0xe0000526},     /* seq6 */
    {NANDFLASH_SELECT_CACHE_REGISTER, 0xe0000627},     /* seq7 */
    {NANDFLASH_CHANGE_WRITE_COLUMN,   0x00008508},     /* seq8 */
    {NANDFLASH_CHANGE_ROW_ADDRESS,    0x0011850c},     /* seq12 */
    
    /*  Read operations */ 
    {NANDFLASH_READ_PAGE,            0x3000002a},    /* seq10 */
    {NANDFLASH_READ_PAGE_CACHE,      0x0000312b},    /* seq11 */
    {NANDFLASH_READ_PAGE_CACHE_LAST, 0x00003f2b},    /* seq11 */
    {NANDFLASH_READ_MULTIPLANE,      0x00320029},    /* seq9 */
    {NANDFLASH_TWO_PLANE_PAGE_READ,  0x0030002f},    /* seq15 */
    {NANDFLASH_QUEUE_PAGE_READ,      0x00370736},    /* seq22 */
    
    /*  Program operations */
    {NANDFLASH_PROGRAM_PAGE,       0x0010800c},        /* seq12 */
    {NANDFLASH_PROGRAM_PAGE_IMD,   0x00108017},        /* seq23 */
    {NANDFLASH_PROGRAM_PAGE_DEL,   0x00138017},        /* seq23 */
    {NANDFLASH_PROGRAM_PAGE_1,     0x0000800d},        /* seq13 */
    {NANDFLASH_PROGRAM_PAGE_CACHE, 0x0015800c},        /* seq12 */
    {NANDFLASH_PROGRAM_MULTIPLANE, 0x0011800c},        /* seq12 */
    {NANDFLASH_WRITE_PAGE,         0x00001000},        /* seq0 */
    {NANDFLASH_WRITE_PAGE_CACHE,   0x00001500},        /* seq0 */
    {NANDFLASH_WRITE_MULTIPLANE,   0x00001100},        /* seq0 */
    
    /*  Erase operations */
    {NANDFLASH_ERASE_BLOCK,      0x00d0600e},        /* seq14 */
    {NANDFLASH_ERASE_MULTIPLANE, 0x00d1600e},        /* seq14 */
    
    /*  Copyback operations */
    {NANDFLASH_COPYBACK_READ,       0x3500002a},      /* seq10 */
    {NANDFLASH_COPYBACK_PROGRAM,    0x00108529},      /* seq9 */
    {NANDFLASH_COPYBACK_PROGRAM_1,  0x0000850d},      /* seq13 */
    {NANDFLASH_COPYBACK_MULTIPLANE, 0x0011850c},      /* seq12 */
    
    /*  OTP operations */
    {NANDFLASH_PROGRAM_OTP,      0x0010a00c},        /* seq12 */
    {NANDFLASH_DATA_PROTECT_OTP, 0x0010a529},        /* seq9 */
    {NANDFLASH_READ_PAGE_OTP,    0x3000af2a},        /* seq10 */
    
    {0,    0},        /* end */
};

/****************************
* get command
*
***************/
UINT32 vxFlashCmdIdx_to_NfcRegVal(UINT32 flash_cmd_idx)
{
    /*  return value needed by reg_val Register */
    int idx = 0;
	int Cmd_Num_Max = sizeof(vxbT_FlashCmdIdx_to_NfcRegVal) / sizeof(vxbT_CMDIDX_TO_REGVAL);
	
    for (idx=0; idx<Cmd_Num_Max; idx++)
	{
        if (flash_cmd_idx == vxbT_FlashCmdIdx_to_NfcRegVal[idx].cmd_idx)
		{
            return vxbT_FlashCmdIdx_to_NfcRegVal[idx].reg_val;
        }
    }
	
    return ERROR;
}

#if 1
/**
* This function initializes the Bad Block Table(BBT) descriptors with a
* predefined pattern for searching Bad Block Table(BBT) in flash.
*/
void vxNand_Init_BadBlkTable_Desc(VXB_DEVICE_ID pDev)
{ 
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;
	
    int idx = 0;
        
    /* 
    Initialize primary Bad Block Table(BBT) 
	*/
    pDrvCtrl->BBT_DESC.offset_block = NANDFLASH_BBT_BLOCK_OFFSET;
	
    if (pDrvCtrl->eccMode == NFCPS_ECC_ONDIE_VX)
    {
        pDrvCtrl->BBT_DESC.offset_sig = 0x4;
        pDrvCtrl->BBT_DESC.offset_ver = 0x14;
    } 
    else 
    {
        pDrvCtrl->BBT_DESC.offset_sig = NANDFLASH_BBT_SIG_OFFSET;
        pDrvCtrl->BBT_DESC.offset_ver = NANDFLASH_BBT_VER_OFFSET;
    }
    pDrvCtrl->BBT_DESC.len_sig = NANDFLASH_BBT_SIG_LEN;
    pDrvCtrl->BBT_DESC.max_block = NANDFLASH_BBT_MAX_BLOCKS;
	
    strcpy(&pDrvCtrl->BBT_DESC.signature[0], "bbt0");
	
    pDrvCtrl->BBT_DESC.version = 0;
    pDrvCtrl->BBT_DESC.valid = 0;
    
    /* Initialize mirror Bad Block Table(BBT) */
    pDrvCtrl->BBT_DESC_MIRROR.offset_block = NANDFLASH_BBT_BLOCK_OFFSET;
	
    if (pDrvCtrl->eccMode == NFCPS_ECC_ONDIE_VX) 
    {
        pDrvCtrl->BBT_DESC_MIRROR.offset_sig = 0x4;
        pDrvCtrl->BBT_DESC_MIRROR.offset_ver = 0x14;
    } 
    else 
    {
        pDrvCtrl->BBT_DESC_MIRROR.offset_sig = NANDFLASH_BBT_SIG_OFFSET;
        pDrvCtrl->BBT_DESC_MIRROR.offset_ver = NANDFLASH_BBT_VER_OFFSET;
    }
	
    pDrvCtrl->BBT_DESC_MIRROR.len_sig = NANDFLASH_BBT_SIG_LEN;
    pDrvCtrl->BBT_DESC_MIRROR.max_block = NANDFLASH_BBT_MAX_BLOCKS;
	
    strcpy(&pDrvCtrl->BBT_DESC_MIRROR.signature[0], "1tbb");
	
    pDrvCtrl->BBT_DESC_MIRROR.version = 0;
    pDrvCtrl->BBT_DESC_MIRROR.valid = 0;
    
    /* 
    Initialize Bad block search pattern structure 
	*/
    if (pDrvCtrl->NAND.PageSize > 512) 
    {
        /* For flash page size > 512 bytes */
        pDrvCtrl->BB_PATTERN.options = NANDFLASH_BBT_SCAN_2ND_PAGE;
        pDrvCtrl->BB_PATTERN.offset = NANDFLASH_BB_PATTERN_OFFSET_LARGE_PAGE;
        pDrvCtrl->BB_PATTERN.length = NANDFLASH_BB_PATTERN_LENGTH_LARGE_PAGE;
    } 
    else 
    {
        pDrvCtrl->BB_PATTERN.options = NANDFLASH_BBT_SCAN_2ND_PAGE;
        pDrvCtrl->BB_PATTERN.offset = NANDFLASH_BB_PATTERN_OFFSET_SMALL_PAGE;
        pDrvCtrl->BB_PATTERN.length = NANDFLASH_BB_PATTERN_LENGTH_SMALL_PAGE;
    }
	
    for (idx=0; idx < 2; idx++) 
    {
        pDrvCtrl->BB_PATTERN.pattern[idx] = NANDFLASH_BB_PATTERN;
    }

	return;
}


/** This function scans the NAND flash for factory marked bad blocks and creates
*  a RAM based Bad Block Table(BBT).
*/
int vxNand_Creat_BadBlkTable(VXB_DEVICE_ID pDev)
{  
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;
	
    int ret = 0;
    
    UINT32 idx; /* loop pool in flash */
    UINT32 blockIdx; /* loop all blocks in flash */
    UINT32 blockNum; /* number of blocks in flash */
    UINT32 block; /* block No. */
    UINT32 poolBlock;
    UINT32 offset_block;
    UINT32 blockShift;
    UINT8 blockType;
    UINT16 bbInfoLen;
    UINT16 bbMapLen;
    UINT32 userBlockMargin; /* last block No. of usable blocks */
	
    bbInfoLen = NAND_MAX_BLOCKS >> 2;
    bbMapLen = NAND_MAX_BADBLKS << 2;
	
    memset(pDrvCtrl->info_BB, 0xff, bbInfoLen);
    memset(pDrvCtrl->map_BB, 0xffffffff, bbMapLen);
    
    userBlockMargin = pDrvCtrl->NAND.BlockNum - pDrvCtrl->BBT_DESC.max_block - NAND_MAX_BADBLKS - 1;
    
    /* scan all blocks for factory marked bad blocks */
    blockNum = pDrvCtrl->NAND.BlockNum - 1;
	
    for (blockIdx = 0; blockIdx < pDrvCtrl->NAND.BlockNum; blockIdx++)
    {
        block = blockNum - blockIdx;
		
        /* Block offset in Bad Block Table(BBT) entry */
        offset_block = block >> 2;
		
        /* Block shift value in the byte */
        blockShift = (block << 1) & 0x6;
		
        /* 
        Search for the bad block pattern 
		*/
        ret = vxNand_Chk_BadBlock(pDev, block);		
        if (ret == ERROR)  /* ERROR: find badblock*/
        {
            /* Marking as bad block (bbInfo[]) */
            pDrvCtrl->info_BB[offset_block] &= ((~(NANDPS_BLOCK_TYPE_MASK << blockShift)) |
                                         (NANDPS_BLOCK_FACTORY_BAD << blockShift));
            if (block <= userBlockMargin)
            {
                for (idx=0; idx<NAND_MAX_BADBLKS; idx++) 
                {
                    poolBlock = userBlockMargin + NAND_MAX_BADBLKS - idx;
					
                    offset_block = poolBlock >> 2;
                    blockShift = (poolBlock << 1) & 0x6;
					
                    blockType = (pDrvCtrl->info_BB[offset_block] >> blockShift) & NANDPS_BLOCK_TYPE_MASK;
					
                    if ((blockType == NANDPS_BLOCK_BAD) || (blockType == NANDPS_BLOCK_FACTORY_BAD))
                    {
                        continue;
                    }
                    else if(pDrvCtrl->map_BB[idx] != 0xffffffff)
                    {
                        continue;
                    }
                    else
                    {
                        pDrvCtrl->map_BB[idx] = (((poolBlock & 0xffff) << 16) | (block & 0xffff));
                        break;
                    }
                }   
            }
        }
    }
    
    return OK;
}

/**
* This function reads the Bad Block Table(BBT) if present in flash. If not it
* scans the flash for detecting factory marked bad blocks and creates a bad
* block table and write the Bad Block Table(BBT) into the flash.
*/
int vxNand_Scan_BadBlkTable(VXB_DEVICE_ID pDev)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;
	
    int ret;
    
    ret = vxNand_Rd_BadBlkTable(pDev);
	
    if (ret == ERROR) 
    {
        /* Create memory based Bad Block Table(BBT) */
        vxNand_Creat_BadBlkTable(pDev);
		
        /* 
        Write the Bad Block Table(BBT) to the flash 
		*/
        ret = vxNand_Wr_BadBlkTable(pDev, &pDrvCtrl->BBT_DESC, &pDrvCtrl->BBT_DESC_MIRROR);
        if (ret == ERROR) 
        {
            return ERROR;
        }
		
        /* 
        Write the Mirror Bad Block Table(BBT) to the flash 
		*/
        ret = vxNand_Wr_BadBlkTable(pDev, &pDrvCtrl->BBT_DESC_MIRROR, &pDrvCtrl->BBT_DESC);
        if (ret == ERROR) 
        {
            return ERROR;
        }
		
        /* 
        Mark the blocks containing Bad Block Table(BBT) as Reserved 
		*/
        vxNand_Mark_BadBlkTable(pDev, &pDrvCtrl->BBT_DESC);
        vxNand_Mark_BadBlkTable(pDev, &pDrvCtrl->BBT_DESC_MIRROR);
    } 
	
    return OK;
}


/**
* This function searches the Bad Bloock Table(BBT) in flash and loads into the
* memory based Bad Block Table(BBT).
*/
int vxNand_Rd_BadBlkTable(VXB_DEVICE_ID pDev)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;
	
    int ret, err1, err2;
	
    vxbT_BBT_DESC*  pDesc;
    vxbT_BBT_DESC*  pDescMirror;
	
    UINT8* bbInfo;
    UINT8* bbMap;
    UINT32 bbInfoLen;  
    UINT32 bbMapLen;
    UINT32 blockSize;
    UINT32 pageSize;
	
    UINT64 bbInfoAddr, bbMapAddr; /* bad block table address */
    
    pDesc = &(pDrvCtrl->BBT_DESC);
    pDescMirror = &(pDrvCtrl->BBT_DESC_MIRROR);
	
    blockSize = vxNandInfo_Get_BlkSize(pDev);
    pageSize = vxNandInfo_Get_PageSize(pDev);
	
    bbInfo = (UINT8*)pDrvCtrl->info_BB;
    bbMap = (UINT8*)pDrvCtrl->map_BB;
	
    bbInfoLen = pDrvCtrl->NAND.BlockNum >> 2;
    bbMapLen = NAND_MAX_BADBLKS << 2;
    
    /* Search the Bad Block Table(BBT) in flash & config pDesc */
    err1 = vxNand_Search_BadBlkTable(pDev, pDesc);
    err2 = vxNand_Search_BadBlkTable(pDev, pDescMirror);
	
    if ((err1 == ERROR) && (err2 == ERROR))
    {
        return ERROR;
    }
    
    if (pDesc->valid && pDescMirror->valid)
    {
        /* Valid BBT & Mirror BBT found load newer bbt*/
        if (pDesc->version > pDescMirror->version) 
        {
            bbInfoAddr = (pDesc->offset_block * blockSize);
            bbMapAddr = (pDesc->offset_block * blockSize) + pageSize;
			
            vxNfcFlash_Read_MultiPage(pDev, bbInfoAddr, bbInfoLen, bbInfo, 0);
            vxNfcFlash_Read_MultiPage(pDev, bbMapAddr, bbMapLen, bbMap, 0);
			
            /* updata mirror BBT, Write the BBT to Mirror BBT location in flash */
            pDescMirror->version = pDesc->version;
			
            ret = vxNand_Wr_BadBlkTable(pDev, pDescMirror, pDesc);
            if (ret == ERROR) 
            {
                return ERROR;
            }
        } 
        else if (pDesc->version < pDescMirror->version) 
        {
            bbInfoAddr = (pDescMirror->offset_block * blockSize);
            bbMapAddr = (pDescMirror->offset_block * blockSize) + pageSize;
			
            vxNfcFlash_Read_MultiPage(pDev, bbInfoAddr, bbInfoLen, bbInfo, 0);
            vxNfcFlash_Read_MultiPage(pDev, bbMapAddr, bbMapLen, bbMap, 0);
			
            /* updata BBT, Write the Mirror BBT to BBT location in flash */
            pDesc->version = pDescMirror->version;
			
            ret = vxNand_Wr_BadBlkTable(pDev, pDesc, pDescMirror);
            if (ret == ERROR) 
            {
                return ERROR;
            }
        } 
        else 
        {
            bbInfoAddr = (pDesc->offset_block * blockSize);
            bbMapAddr = (pDesc->offset_block * blockSize) + pageSize;
			
            /* Both are up-to-date */
            vxNfcFlash_Read_MultiPage(pDev, bbInfoAddr, bbInfoLen, bbInfo, 0);
            vxNfcFlash_Read_MultiPage(pDev, bbMapAddr, bbMapLen, bbMap, 0);    
        }
    }
    else if (pDesc->valid)
    {
        bbInfoAddr = (pDesc->offset_block * blockSize);
        bbMapAddr = (pDesc->offset_block * blockSize) + pageSize;
		
        /* Valid Primary BBT found */
        vxNfcFlash_Read_MultiPage(pDev, bbInfoAddr, bbInfoLen, bbInfo, 0);
        vxNfcFlash_Read_MultiPage(pDev, bbMapAddr, bbMapLen, bbMap, 0);   
		
        /* 
        Write the BBT to Mirror BBT location in flash 
		*/
        pDescMirror->version = pDesc->version;
		
        ret = vxNand_Wr_BadBlkTable(pDev, pDescMirror, pDesc);
		if (ret == ERROR) 
		{
			return ERROR;
		}
    } 
    else 
    {
        bbInfoAddr = (pDescMirror->offset_block * blockSize);
        bbMapAddr = (pDescMirror->offset_block * blockSize) + pageSize;
		
        vxNfcFlash_Read_MultiPage(pDev, bbInfoAddr, bbInfoLen, bbInfo, 0);
        vxNfcFlash_Read_MultiPage(pDev, bbMapAddr, bbMapLen, bbMap, 0);
		
        /* 
        Write the Mirror BBT to BBT location in flash 
		*/
        pDesc->version = pDescMirror->version;
		
        ret = vxNand_Wr_BadBlkTable(pDev, pDesc, pDescMirror);
		if (ret == ERROR) 
		{
			return ERROR;
		}
    }
	
    return OK;
}

/**
* This function searches the BBT in flash.
*/
int vxNand_Search_BadBlkTable(VXB_DEVICE_ID pDev, vxbT_BBT_DESC* pDesc)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;
	
    int ret;  
    UINT32 max_block;
    UINT32 maxBlockNum;
    UINT32 blockIdx;
    UINT32 block;
	
    UINT32 offset_sig;
    UINT32 len_sig;
    UINT32 offset_ver;
    UINT32 offset;
	
    UINT8 spare[NAND_MAX_PAGE_SPARESIZE];
    
    max_block = pDesc->max_block;
    maxBlockNum = pDrvCtrl->NAND.BlockNum - 1;
    offset_sig = pDesc->offset_sig;
    len_sig = pDesc->len_sig;
    offset_ver = pDesc->offset_ver;
    
    /* Read the last 3 blocks for Bad Block Table(BBT) signature */
    for (blockIdx = 0; blockIdx < max_block; blockIdx++) 
    {
        block = maxBlockNum - blockIdx;
        ret = vxNfcFlash_Read_Page(pDev, block, 0, 0, 0, spare);
        if (ret == ERROR) 
        {
            continue;
        }
        
        /* Check the Bad Block Table(BBT) signature */
        for (offset=0; offset<len_sig; offset++) 
        {
            if (spare[offset_sig + offset] != pDesc->signature[offset])
            {
                /* Check the next blocks */
                break;  
            }
        }
		
        if (offset >= len_sig) 
        {
            /* Bad Block Table(BBT) found */
            pDesc->offset_block = block;
            pDesc->version = spare[offset_ver];
            pDesc->valid = 1;
			
            return OK;
        }
    }
    
    /* 
    Bad Block Table(BBT) not found 
	*/
    return ERROR;
}

/**
* This function writes Bad Block Table(BBT)(Include bbInfo) from RAM to flash.
* save at 1st page in block
*/
int vxNand_Wr_BadBlkTable(VXB_DEVICE_ID pDev, vxbT_BBT_DESC* pDesc, vxbT_BBT_DESC* pDescMirror)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;
	
    int ret;
    
    UINT32 idx; 
    UINT32 blockNum; /* total number of blocks in flash */
    UINT32 block; /* block which write bad block info to */
	
    UINT32 offset_block; 
    UINT32 blockShift;
    UINT8 blockType;
	
    UINT32 bbInfoLen; /* number of bad block info bytes */
    UINT32 bbMapLen;
    UINT8* bbInfo;
    UINT8* bbMap;
	
    UINT8 spare[NAND_MAX_PAGE_SPARESIZE];
    UINT32 blockSize;
    UINT32 pageSize;
    
    bbInfo = (UINT8*)pDrvCtrl->info_BB;
    bbMap = (UINT8*)pDrvCtrl->map_BB;
	
    bbInfoLen = pDrvCtrl->NAND.BlockNum >> 2;
    bbMapLen = NAND_MAX_BADBLKS << 2;
	
    blockSize = vxNandInfo_Get_BlkSize(pDev);
    pageSize = vxNandInfo_Get_PageSize(pDev);
    
    /* start from the last block*/
    blockNum = pDrvCtrl->NAND.BlockNum - 1;
	
    /* Find a good block to write the Bad Block Table(BBT) */
    if (!pDesc->valid) 
    {
        for (idx=0; idx<pDesc->max_block; idx++) 
        {
            block  = (blockNum - idx);
			
            /* one byte contain 4 bad block info*/  			
            offset_block = block >> 2;
			
            /* each bad block info occupy 2 bits, offset0,2,4,6 */
            blockShift = (block << 1) & 0x6;
			
            blockType = (pDrvCtrl->info_BB[offset_block] >> blockShift) & NANDPS_BLOCK_TYPE_MASK;
			
            switch (blockType)
            {
            case NANDPS_BLOCK_BAD:
            case NANDPS_BLOCK_FACTORY_BAD:
                continue;
			
            default: /* Good Block */
                break;
            }
			
            /* good block found */
            pDesc->offset_block = block;
            
            if (pDesc->offset_block != pDescMirror->offset_block) 
            {
                /* Free block found */
                pDesc->valid = 1;
                break;
            }
        }
        
        /* Block not found for writing Bad Block Table(BBT) */
        if (idx >= pDesc->max_block) 
        {
            return ERROR;
        }
    } 
    else 
    {
        block = pDesc->offset_block;
    }
    
    /* Write the signature and version in the spare data area */
    memset(spare, 0xff, pDrvCtrl->NAND.SpareSize);
    memcpy(spare + pDesc->offset_sig, &pDesc->signature[0], pDesc->len_sig);
    memcpy(spare + pDesc->offset_ver, &pDesc->version, 1);
	
    /* Write the BBT to page offset */
    ret = vxNfcFlash_Erase_Block(pDev, block);
    if (ret == ERROR) 
    {
        return ERROR;
    }
	
    ret = vxNfcFlash_Write_Page(pDev, pDesc->offset_block, 0, 0, 0, spare);
    if (ret == ERROR) 
    {
        return ERROR;
    }
	
    ret = vxNfcFlash_Write_MultiPage(pDev, pDesc->offset_block * blockSize, bbInfoLen, bbInfo, 0);
    if (ret == ERROR) 
    {
        return ERROR;
    }
	
    ret = vxNfcFlash_Write_MultiPage(pDev, pDesc->offset_block * blockSize + pageSize, bbMapLen, bbMap, 0);
    if (ret == ERROR) 
    {
        return ERROR;
    }
	
    return OK;
}

/**
* This function marks the block containing Bad Block Table as reserved
* and updates the BBT.
*/
int vxNand_Mark_BadBlkTable(VXB_DEVICE_ID pDev, vxbT_BBT_DESC* pDesc)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;
	
    int ret;
    UINT32 blockNum;
    UINT32 offset_block;
    UINT32 blockShift;
    UINT32 blockIdx;
    UINT32 block;
    UINT8 oldVal, newVal;
    UINT32 updateBBT = 0;
    
    /* Mark the last 3 blocks as Reserved */
    blockNum = pDrvCtrl->NAND.BlockNum - 1;
	
    for (blockIdx=0; blockIdx<pDesc->max_block; blockIdx++,block++) 
    {
        block = blockNum - blockIdx;
		
        offset_block = block >> NANDPS_BBT_BLOCK_SHIFT;
        blockShift = (blockIdx << 1) & 0x6;
		
        oldVal = pDrvCtrl->info_BB[offset_block];
		
        newVal = oldVal & 
            ((~(NANDPS_BLOCK_TYPE_MASK << blockShift)) |
             (NANDPS_BLOCK_RESERVED << blockShift));
		
        pDrvCtrl->info_BB[offset_block] = newVal;
        
        if (oldVal != newVal) 
        {
            updateBBT = 1;
        }
    }
	
    /* Update the BBT to flash */
    if (updateBBT) 
    {
        ret = vxNand_Update_BadBlkTable(pDev);
        if (ret == ERROR) 
        {
            return ERROR;
        }
    }
	
    return OK;
}


/**
* This function updates the primary and mirror Bad Block Table(BBT) in the flash.
*/
int vxNand_Update_BadBlkTable(VXB_DEVICE_ID pDev)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;
	
    int ret;    
    UINT8 version;
    
    /* Update the version number */
    version = pDrvCtrl->BBT_DESC.version;
    pDrvCtrl->BBT_DESC.version = (version + 1) % 256;
	
    version = pDrvCtrl->BBT_DESC_MIRROR.version;
    pDrvCtrl->BBT_DESC_MIRROR.version = (version + 1) % 256;
    
    /* Update the primary Bad Block Table(BBT) in flash */
    ret = vxNand_Wr_BadBlkTable(pDev, &pDrvCtrl->BBT_DESC, &pDrvCtrl->BBT_DESC_MIRROR);
	if (ret == ERROR) 
    {
        return ERROR;
    }
	
    /* Update the mirrored Bad Block Table(BBT) in flash */
    ret = vxNand_Wr_BadBlkTable(pDev, &pDrvCtrl->BBT_DESC_MIRROR, &pDrvCtrl->BBT_DESC);
	if (ret == ERROR) 
    {
        return ERROR;
    }

    return OK;
}

int vxNand_Chk_BadBlock(VXB_DEVICE_ID pDev, UINT16 block) 
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;
	
    int ret;
	
    UINT32 numPages;
    UINT32 pageIdx; /* Page idx which stores bad block pattern */
    UINT32 len; /* Pattern length idx */
	
    UINT8 spare[NAND_MAX_PAGE_SPARESIZE];    
    
    /* Number of pages to search for bad block pattern */
    if (pDrvCtrl->BB_PATTERN.options & NANDFLASH_BBT_SCAN_2ND_PAGE)
    {
        numPages = 2;
    } 
    else 
    {
        numPages = 1;
    }
	
    /* Search for the bad block pattern */
    for (pageIdx=0; pageIdx<numPages; pageIdx++) 
    {
        ret = vxNfcFlash_Read_Page(pDev, block, pageIdx, 0, 0, spare);
        if (ret == ERROR) 
        {
            return ERROR;
        }
        
        /* 
        Read the spare bytes to check for bad block pattern 
		*/
        for (len = 0; len < pDrvCtrl->BB_PATTERN.length; len++) 
        {
            if (spare[pDrvCtrl->BB_PATTERN.offset + len] !=
                pDrvCtrl->BB_PATTERN.pattern[len])
            {
                /* Bad block found, return ret to marking as bad block */
                return ERROR;
            }
        }
    }
	
    return OK;
}

/**
* This function marks a block as bad in the RAM based Bad Block Table(pDrvCtrl->info_BB[]). It
* also updates the Bad Block Table(BBT) in the flash.
*/
int vxNand_Mark_BadBlock(VXB_DEVICE_ID pDev, UINT32 block) 
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;
	
    int ret;
    
    UINT8 numPages;
    UINT8 spare[NAND_MAX_PAGE_SPARESIZE];
    UINT8 pageIdx;
    
    UINT8 idx;
    UINT32 userBlockMargin;
    UINT32 poolBlock;
    UINT32 offset_block;/* byte offset in bbt contain bad block info */
    UINT32 blockShift;/* shift in byte save bad block info */
    UINT8 blockType;
    UINT8 data; /* contain bad block info for this block */
    UINT8 mapSize;
    UINT8 oldVal, newVal;
    UINT8 found = 0;
    
    if (block >= pDrvCtrl->NAND.BlockNum)
    {
        return ERROR;
    }
    
    /* write bad block pattern to block spare */ 
    memset(spare, 0xff, NAND_MAX_PAGE_SPARESIZE);
    memset(&spare[pDrvCtrl->BB_PATTERN.offset], 0xBA, pDrvCtrl->BB_PATTERN.length);
	
    if (pDrvCtrl->BB_PATTERN.options & NANDFLASH_BBT_SCAN_2ND_PAGE)
    {
        numPages = 2;
    } 
    else 
    {
        numPages = 1;
    }
	
    for (pageIdx=0; pageIdx<numPages; pageIdx++) 
    {
        ret = vxNfcFlash_Write_Page(pDev, block, pageIdx, 0, 0, spare);
        if (ret == ERROR) 
        {
            return ERROR;
        }
    }
	
    /* update bbt */
    userBlockMargin = pDrvCtrl->NAND.BlockNum - pDrvCtrl->BBT_DESC.max_block - NAND_MAX_BADBLKS - 1;
	
    for (idx=0; idx<NAND_MAX_BADBLKS; idx++) 
    {
        poolBlock = userBlockMargin + NAND_MAX_BADBLKS - idx;
		
        offset_block = poolBlock >> 2;
        blockShift = (poolBlock << 1) & 0x6;
		
        blockType = (pDrvCtrl->info_BB[offset_block] >> blockShift) & NANDPS_BLOCK_TYPE_MASK;
		
        if ((blockType & 0x1) && (pDrvCtrl->map_BB[idx] == 0xffffffff))
        {
            break;
        }
		
        mapSize++;
		
        /* block in pool is bad */
        if (block == (pDrvCtrl->map_BB[idx] & 0xffff))
        {
            data = pDrvCtrl->info_BB[offset_block];
            oldVal = data;
			
            data &= ((~(NANDPS_BLOCK_TYPE_MASK << blockShift)) |
                     (NANDPS_BLOCK_BAD << blockShift)); 
			
            newVal = data;
			
            pDrvCtrl->info_BB[offset_block] = data;
            pDrvCtrl->map_BB[idx] = 0xffffffff;
			
            found = 1;
        }     
    }   
	
    /* no more good block in pool */
    if (mapSize >= NAND_MAX_BADBLKS)
    {
        return ERROR;
    }
	
    /* new bad block in user block area*/
    if (!found)
    {
        /* one byte contain 4 bad block info*/
        offset_block = block >> NANDPS_BBT_BLOCK_SHIFT;
        data = pDrvCtrl->info_BB[offset_block];
		
        /* each bad block info occupy 2 bits, offset0,2,4,6 */
        blockShift = (block  << 1) & 0x6;      
		
        /* Mark the block as bad in the RAM based Bad Block Table */
        oldVal = data;
		
        data &= ((~(NANDPS_BLOCK_TYPE_MASK << blockShift)) |
                 (NANDPS_BLOCK_BAD << blockShift));
		
        newVal = data;
        pDrvCtrl->info_BB[offset_block] = data;
    }
    
    pDrvCtrl->map_BB[mapSize] = (((poolBlock & 0xffff) << 16) | (block & 0xffff));
    
    /* Update the Bad Block Table(BBT) in flash */
    if (oldVal != newVal) 
    {
        ret = vxNand_Update_BadBlkTable(pDev);
        if(ret == ERROR)
        {
            return ERROR;
        }
    }
	
    return OK;
}

#endif


#if 1

UINT32 vxNandInfo_Get_DevSize_Blks(VXB_DEVICE_ID pDev)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;	
    return (UINT32)(((UINT64)pDrvCtrl->NAND.DevSizeMB << 10) / pDrvCtrl->NAND.BlockSizeKB);
}

UINT32 vxNandInfo_Get_DevSize_Pages(VXB_DEVICE_ID pDev)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;	
    return (UINT32)(((UINT64)pDrvCtrl->NAND.DevSizeMB << 20) / pDrvCtrl->NAND.PageSize);
}

UINT32 vxNandInfo_Get_DevSize_MB(VXB_DEVICE_ID pDev)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;
    return pDrvCtrl->NAND.DevSizeMB;
}

UINT64 vxNandInfo_Get_DevSize_Bytes(VXB_DEVICE_ID pDev)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;	
    return (UINT64)pDrvCtrl->NAND.DevSizeMB << 20;
}

UINT32 vxNandInfo_Get_BlkSize_Pages(VXB_DEVICE_ID pDev)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;	
    return (UINT32)(((UINT32)pDrvCtrl->NAND.BlockSizeKB << 10) / pDrvCtrl->NAND.PageSize);
}

UINT32 vxNandInfo_Get_BlkSize_KB(VXB_DEVICE_ID pDev)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;	
    return pDrvCtrl->NAND.BlockSizeKB ;
}

UINT32 vxNandInfo_Get_BlkSize(VXB_DEVICE_ID pDev)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;	
    return (UINT32)pDrvCtrl->NAND.BlockSizeKB << 10;
}

UINT32 vxNandInfo_Get_PageSize(VXB_DEVICE_ID pDev)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;	
    return pDrvCtrl->NAND.PageSize;
}

UINT32 vxNandInfo_Get_SpareSize(VXB_DEVICE_ID pDev)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;
	
    if (pDrvCtrl->NAND.SpareSize) 
    {
        return pDrvCtrl->NAND.SpareSize;
    }
    else 
    {
        return (pDrvCtrl->NAND.PageSize >> 5);  /* Spare size is 16/512 of data size */
    }
}

UINT32 vxNandInfo_Get_DeviceId(VXB_DEVICE_ID pDev)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;	
    return pDrvCtrl->NAND.DevId;
}

UINT32 vxNandInfo_Get_RowAddrCycle(VXB_DEVICE_ID pDev)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;	
    return pDrvCtrl->NAND.RowAddrCycle;
}

UINT32 vxNandInfo_Get_ColAddrCycle(VXB_DEVICE_ID pDev)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;	
    return pDrvCtrl->NAND.ColAddrCycle;
}

UINT32 vxNandInfo_Get_BusWidth(VXB_DEVICE_ID pDev)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;	
    return ((pDrvCtrl->NAND.Options == 16) || (pDrvCtrl->NAND.IoWidth == 16)) ? 16 : 8;
}

int flashInfo_isSmallBlks(VXB_DEVICE_ID pDev)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;	
    return (pDrvCtrl->NAND.PageSize <= 512 ) ? 1: 0;
}

UINT32 vxNandInfo_Get_LunNum(VXB_DEVICE_ID pDev)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;	
    return pDrvCtrl->NAND.LunNum;
}

int vxNandInfo_Parse_FlashAddr(VXB_DEVICE_ID pDev, UINT64 Address, UINT32 Size, 
                                    UINT16* Block, UINT16* Page, UINT16* Offset)
{
    UINT16 tmpBlock, tmpPage, tmpOffset;
    UINT64 DevSize_Bytes;
    UINT32 BlkSize_Bytes;
    UINT16 PageSize_Bytes;
    
    DevSize_Bytes = vxNandInfo_Get_DevSize_Bytes(pDev);
    BlkSize_Bytes = vxNandInfo_Get_BlkSize(pDev);
    PageSize_Bytes = vxNandInfo_Get_PageSize(pDev);  
    
    if(Address + Size > DevSize_Bytes)
    {
        return ERROR;
    }

	/* block idx*/
    tmpBlock = Address / BlkSize_Bytes;  

	/* page idx*/
    Address -= tmpBlock * BlkSize_Bytes;
    tmpPage = Address / PageSize_Bytes;

	/* offset in page*/
    Address -= tmpPage*PageSize_Bytes;
    tmpOffset = Address;
    
    if (Block) 
    {
        *Block = tmpBlock;
    }
	
    if (Page)
    {
        *Page = tmpPage;
    }
	
    if (Offset)
    {
        *Offset = tmpOffset;
    } 
	
    return OK;
}

#endif


#define __SLCR_MIO_SETUP__

#if 1

#define EMIT_EXIT()                   ( (OPCODE_EXIT      << 4 ) | 0 )
#define EMIT_CLEAR(addr)              ( (OPCODE_CLEAR     << 4 ) | 1 ) , addr
#define EMIT_WRITE(addr,val)          ( (OPCODE_WRITE     << 4 ) | 2 ) , addr, val
#define EMIT_MASKWRITE(addr,mask,val) ( (OPCODE_MASKWRITE << 4 ) | 3 ) , addr, mask, val
#define EMIT_MASKPOLL(addr,mask)      ( (OPCODE_MASKPOLL  << 4 ) | 2 ) , addr, mask
#define EMIT_MASKDELAY(addr,mask)     ( (OPCODE_MASKDELAY << 4 ) | 2 ) , addr, mask
#define EMIT_MASKCOPY(addrsrc,addrdst,masksrc,maskdst)  ((OPCODE_MASKCOPY<<4)|4), addrsrc, addrdst, masksrc, maskdst

unsigned long vxNfc_init_MIO[] = 
{
    /* UNLOCK_KEY = 0XDF0D767B*/
    EMIT_WRITE(SLCR_REG_BASE + 0x008, 0xDF0D767BU),

    /* MIO_PIN_0*/
    /* .. SIGNAL   = vxNand_cs*/
    /* .. DIR      = out*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = enabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x700, 0x0000FFFFU, 0x00003610U),
    
    /* MIO_PIN_2*/
    /* .. SIGNAL   = vxNand_alen*/
    /* .. DIR      = out*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = disabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x708, 0x0000FFFFU, 0x00002610U),
    
    /* MIO_PIN_3*/
    /* .. SIGNAL   = vxNand_web*/
    /* .. DIR      = out*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = disabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x70C, 0x0000FFFFU, 0x00002610U),
    
    /* MIO_PIN_4*/
    /* .. SIGNAL   = vxNand_io[2]*/
    /* .. DIR      = inout*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = disabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x710, 0x0000FFFFU, 0x00002610U),
    
    /* MIO_PIN_5*/
    /* .. SIGNAL   = vxNand_io[0]*/
    /* .. DIR      = inout*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = disabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x714, 0x0000FFFFU, 0x00002610U),
    
    /* MIO_PIN_6*/
    /* .. SIGNAL   = vxNand_io[1]*/
    /* .. DIR      = inout*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = disabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x718, 0x0000FFFFU, 0x00002610U),
    
    /* MIO_PIN_7*/
    /* .. SIGNAL   = vxNand_cleb*/
    /* .. DIR      = out*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = disabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x71C, 0x0000FFFFU, 0x00002610U),
    
    /* MIO_PIN_8*/
    /* .. SIGNAL   = vxNand_rdb*/
    /* .. DIR      = out*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = disabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x720, 0x0000FFFFU, 0x00002610U),
    
    /* MIO_PIN_9*/
    /* .. SIGNAL   = vxNand_io[4]*/
    /* .. DIR      = inout*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = enabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x724, 0x0000FFFFU, 0x00003610U),
    
    /* MIO_PIN_10*/
    /* .. SIGNAL   = vxNand_io[5]*/
    /* .. DIR      = inout*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = enabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x728, 0x0000FFFFU, 0x00003610U),
    
    /* MIO_PIN_11*/
    /* .. SIGNAL   = vxNand_io[6]*/
    /* .. DIR      = inout*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = enabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x72C, 0x0000FFFFU, 0x00003610U),
    
    /* MIO_PIN_12*/
    /* .. SIGNAL   = vxNand_io[7]*/
    /* .. DIR      = inout*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = enabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x730, 0x0000FFFFU, 0x00003610U),
    
    /* MIO_PIN_13*/
    /* .. SIGNAL   = vxNand_io[3]*/
    /* .. DIR      = inout*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = enabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x734, 0x0000FFFFU, 0x00003610U),
    
    /* MIO_PIN_14*/
    /* .. SIGNAL   = vxNand_busy*/
    /* .. DIR      = in*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = enabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x738, 0x0000FFFFU, 0x00003611U),
    
    /* MIO_PIN_16*/
    /* .. SIGNAL   = vxNand_io[8]*/
    /* .. DIR      = inout*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = enabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x740, 0x0000FFFFU, 0x00003610U),
    
    /* MIO_PIN_17*/
    /* .. SIGNAL   = vxNand_io[9]*/
    /* .. DIR      = inout*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = enabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x744, 0x0000FFFFU, 0x00003610U),
    
    /* MIO_PIN_18*/
    /* .. SIGNAL   = vxNand_io[10]*/
    /* .. DIR      = inout*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = enabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x748, 0x0000FFFFU, 0x00003610U),
    
    /* MIO_PIN_19*/
    /* .. SIGNAL   = vxNand_io[11]*/
    /* .. DIR      = inout*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = enabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x74C, 0x0000FFFFU, 0x00003610U),
    
    /* MIO_PIN_20*/
    /* .. SIGNAL   = vxNand_io[12]*/
    /* .. DIR      = inout*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = enabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x750, 0x0000FFFFU, 0x00003610U),
    
    /* MIO_PIN_21*/
    /* .. SIGNAL   = vxNand_io[13]*/
    /* .. DIR      = inout*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = enabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x754, 0x0000FFFFU, 0x00003610U),
    
    /* MIO_PIN_22*/
    /* .. SIGNAL   = vxNand_io[14]*/
    /* .. DIR      = inout*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = enabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x758, 0x0000FFFFU, 0x00003610U),
    
    /* MIO_PIN_23*/
    /* .. SIGNAL   = vxNand_io[15]*/
    /* .. DIR      = inout*/
    /* .. L3-0 SEL = B'0001000*/
    /* .. SPEED    = slow*/
    /* .. IO TYPE  = LVCMOS 3.3V*/
    /* .. PULLUP   = enabled*/
    /* .. DRIVE STRENTH   = 8*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x75C, 0x0000FFFFU, 0x00003610U),

    /* LOCK_KEY = 0XDF0D767B*/
    EMIT_WRITE(SLCR_REG_BASE + 0x004, 0xDF0D767BU),

    EMIT_EXIT(),
};

int vxSlcr_MIO_Config(unsigned long * ps_config_init) 
{
	unsigned long *ptr = ps_config_init;
	unsigned long opcode; /* current instruction ..*/
	unsigned long args[16]; /* no opcode has so many args ...*/
	int numargs; /* number of arguments of this instruction*/
	int  j; /* general purpose index*/

	volatile unsigned long *addr;
	unsigned long  val,mask;
	volatile unsigned long *addrdst;
	unsigned long  maskdst,masktmp;

	int finish = -1 ; /* loop while this is negative !*/
	int i = 0; /* Timeout variable*/
	int srclow,dstlow;

	while (finish < 0) 
	{
		numargs = ptr[0] & 0xF;
		opcode = ptr[0] >> 4;

		for( j = 0 ; j < numargs ; j ++ )
		{  
			args[j] = ptr[j+1];
		}
		ptr += numargs + 1;

		switch ( opcode ) 
		{
		case OPCODE_EXIT:
			finish = PS_INIT_SUCCESS;
			break;
		
		case OPCODE_CLEAR:
			addr = (unsigned long*) args[0];
			*addr = 0;
			break;
			
		case OPCODE_WRITE:
			addr = (unsigned long*) args[0];
			val = args[1];
			*addr = val;
			break;
			
		case OPCODE_MASKWRITE:
			addr = (unsigned long*) args[0];
			mask = args[1];
			val = args[2];
			*addr = ( val & mask ) | ( *addr & ~mask);
			break;
			
		case OPCODE_MASKCOPY:
			addr = (unsigned long*) args[0];
			addrdst = (unsigned long*) args[1];
			mask = args[2];
			maskdst = args[3];
			masktmp = maskdst;
			val = (*addr & mask);
			srclow = 0;
			while (!(mask & 1)) 
			{ 
				mask >>= 1; 
				++srclow; 
			};
			dstlow = 0;
			while (!(masktmp & 1)) 
			{ 
				masktmp >>= 1; 
				++dstlow; 
			};
			val = (val >> srclow) << dstlow;
			*addrdst = ( val & maskdst ) | ( *addrdst & ~maskdst);
			break;

		case OPCODE_MASKPOLL:
			addr = (unsigned long*) args[0];
			mask = args[1];
			i = 0;
			while ((*addr & mask) != mask) 
			{
				if (i == PS_MASK_POLL_TIME) 
				{
					finish = PS_INIT_TIMEOUT;
					break;
				}
				i++;
			}
			break;

		case OPCODE_MASKDELAY:
			break;

		default:
			finish = PS_INIT_CORRUPT;
			break;
		}
	}

	return finish;
}

int vxNfcSlcr_Set_NfcMIO_2(void)
{
	int ret;

	/* MIO init*/
	ret = vxSlcr_MIO_Config(vxNfc_init_MIO);
	if (ret != PS_INIT_SUCCESS) 
	{	
		return ERROR;
	}

  
	/* Peripherals init*/
	/*
	ret = ps_config(ps_peripherals_init_data_nfc);
	if (ret != PS_INIT_SUCCESS) 
		return ret;
	*/

	return OK;
}

#endif

