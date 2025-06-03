/* norFlashCFI_cfi.c - fmsh 7020/7045 pSmc driver */

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
01a, 09Jun20, jc  written.
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

#include "norflash_cfi.h"

/*
defines 
*/
#undef TRACE_OUT

#define TRACE_OUT(flag, ...)       \
	do {						   \
		if (flag) {				   \
		printf(__VA_ARGS__);       \
		}						   \
	} while(0)


/* 
norFlashCFI_cfi
*/
#if 1

/*****************************************************************************
*
* @description
* This function dumps the Common Flash Interface Definition Table.
*
* @param    
*           pNorflash  Pointer to an NorFlash instance.
*
* @return	
*           NA.
*
* @note		NA.
*
*****************************************************************************/
void norFlashCFI_Dump_Cfg(vxT_NORFLASH* pNorflash)
{
    int i;
	
    vxT_NorFlashCFI* pNorFlashCFI = (vxT_NorFlashCFI*)(&pNorflash->cfiDescription);
		
    TRACE_OUT(DEBUG_OUT, "Common Flash Interface Definition Table\n\r");
    TRACE_OUT(DEBUG_OUT, "Addr. Data   Description \n\r");
    TRACE_OUT(DEBUG_OUT, "0x10  %04Xh  Query Unique ASCII string\n\r",
		
    pNorFlashCFI->norFlashCfiQueryInfo.queryUniqueString[0]);	
    TRACE_OUT(DEBUG_OUT, "0x11  %04Xh  \n\r",
		
    pNorFlashCFI->norFlashCfiQueryInfo.queryUniqueString[1]);
    TRACE_OUT(DEBUG_OUT, "0x12  %04Xh  \n\r",
		
    pNorFlashCFI->norFlashCfiQueryInfo.queryUniqueString[2]);
    TRACE_OUT(DEBUG_OUT, "0x13  %04Xh  Primary OEM Command Set\n\r",
		
    pNorFlashCFI->norFlashCfiQueryInfo.primaryCode);
    TRACE_OUT(DEBUG_OUT, "0x15  %04Xh  Address for Primary Extended Table\n\r",
		
    pNorFlashCFI->norFlashCfiQueryInfo.primaryAddr);
    TRACE_OUT(DEBUG_OUT, "0x17  %04Xh  Alternate OEM Command Set\n\r",
		
    pNorFlashCFI->norFlashCfiQueryInfo.alternateCode);
    TRACE_OUT(DEBUG_OUT, "0x19  %04Xh  Address for Alternate OEM Extended Table\n\r",
		
    pNorFlashCFI->norFlashCfiQueryInfo.alternateAddr);
    TRACE_OUT(DEBUG_OUT, "0x1B  %04Xh  VCC min write/erase\n\r",
		
    pNorFlashCFI->norFlashCfiQueryInfo.minVcc);
    TRACE_OUT(DEBUG_OUT, "0x1C  %04Xh  VCC max write/erase\n\r",
		
    pNorFlashCFI->norFlashCfiQueryInfo.maxVcc);
    TRACE_OUT(DEBUG_OUT, "0x1D  %04Xh  VPP min voltage\n\r",
		
    pNorFlashCFI->norFlashCfiQueryInfo.minVpp);
    TRACE_OUT(DEBUG_OUT, "0x1E  %04Xh  VPP max voltage\n\r",
		
    pNorFlashCFI->norFlashCfiQueryInfo.maxVpp);
    TRACE_OUT(DEBUG_OUT, "0x1F  %04Xh  Typical timeout per single word write\n\r",
		
    pNorFlashCFI->norFlashCfiQueryInfo.minTimeOutWrite);
    TRACE_OUT(DEBUG_OUT, "0x20  %04Xh  Typical timeout for Min. size buffer write\n\r",
		
    pNorFlashCFI->norFlashCfiQueryInfo.minTimeOutBuffer);
    TRACE_OUT(DEBUG_OUT, "0x21  %04Xh  Typical timeout per individual block erase\n\r",
		
    pNorFlashCFI->norFlashCfiQueryInfo.minTimeOutBlockErase);
    TRACE_OUT(DEBUG_OUT, "0x22  %04Xh  Typical timeout for full chip erase\n\r",
		
    pNorFlashCFI->norFlashCfiQueryInfo.minTimeOutChipErase);
    TRACE_OUT(DEBUG_OUT, "0x23  %04Xh  Max. timeout for word write\n\r",
		
    pNorFlashCFI->norFlashCfiQueryInfo.maxTimeOutWrite);
    TRACE_OUT(DEBUG_OUT, "0x24  %04Xh  Max. timeout for buffer write\n\r",
		
    pNorFlashCFI->norFlashCfiQueryInfo.maxTimeOutBuffer);
    TRACE_OUT(DEBUG_OUT, "0x25  %04Xh  Max. timeout per individual block erase\n\r",
		
    pNorFlashCFI->norFlashCfiQueryInfo.maxTimeOutBlockErase);
    TRACE_OUT(DEBUG_OUT, "0x26  %04Xh  Max. timeout for full chip erase\n\r",
		
    pNorFlashCFI->norFlashCfiQueryInfo.maxTimeOutChipErase);    
    TRACE_OUT(DEBUG_OUT, "0x27  %04Xh  Device Size = 2^N byte\n\r",
		
    pNorFlashCFI->norFlashCfiDevGeometry.devSize);
    TRACE_OUT(DEBUG_OUT, "0x28  %04Xh  Flash Device Interface description\n\r",
		
    pNorFlashCFI->norFlashCfiDevGeometry.devInterface);
    TRACE_OUT(DEBUG_OUT, "0x2A  %04Xh  Max. number of byte in multi-byte write\n\r",
		
    pNorFlashCFI->norFlashCfiDevGeometry.numMultiWrite);
    TRACE_OUT(DEBUG_OUT, "0x2C  %04Xh  Number of Erase Block Regions within device\n\r",
		
    pNorFlashCFI->norFlashCfiDevGeometry.numEraseRegion);
    for(i = 0; i < pNorFlashCFI->norFlashCfiDevGeometry.numEraseRegion; i++) 
	{
        TRACE_OUT(DEBUG_OUT, "0x%2X  %04Xh  Number of Erase Blocks of identical size within region %x \n\r",
            0x2D + i * 4, pNorFlashCFI->norFlashCfiDevGeometry.eraseRegionInfo[i].Y, i );
		
        TRACE_OUT(DEBUG_OUT, "0x%2X  %04Xh  (z) times 256 bytes within region %x \n\r",
            0x2E + i * 4, pNorFlashCFI->norFlashCfiDevGeometry.eraseRegionInfo[i].Z, i );
    }
}

/*****************************************************************************
*
* @description
* This function returns the numbers of block in all Norflash regions.
*
* @param    
*           pNorflash  Pointer to an NorFlash instance.
*
* @return	
*           total block number.
*
* @note		NA.
*
*****************************************************************************/
UINT32 norFlashCFI_Get_DevNumOfBlks(vxT_NORFLASH* pNorflash)
{
    int i;
    UINT32 blocks = 0;
    UINT16 numBlockRegion;
    
    numBlockRegion = pNorflash->cfiDescription.norFlashCfiDevGeometry.numEraseRegion;
   
    for (i=0; i<numBlockRegion; i++)
	{
        blocks += (pNorflash->cfiDescription.norFlashCfiDevGeometry.eraseRegionInfo[i].Y) + 1;
    }
	
    return blocks;
}

/*****************************************************************************
*
* @description
* This function returns the minimun block size in all Norflash regions.
*
* @param    
*           pNorflash  Pointer to an NorFlash instance.
*
* @return	
*           the minimun block size.
*
* @note		NA.
*
*****************************************************************************/
UINT32 norFlashCFI_Get_DevMinBlkSize(vxT_NORFLASH* pNorflash)
{
    int i = 0;
    UINT16 numBlockRegion;
    unsigned long size ;
	
    numBlockRegion = pNorflash->cfiDescription.norFlashCfiDevGeometry.numEraseRegion;
    
    size = (pNorflash->cfiDescription.norFlashCfiDevGeometry.eraseRegionInfo[0].Z) * 256;
    
    for (i=1; i<numBlockRegion; i++)
	{
        if (size > (pNorflash->cfiDescription.norFlashCfiDevGeometry.eraseRegionInfo[i].Z) * 256) 
		{
            size = (pNorflash->cfiDescription.norFlashCfiDevGeometry.eraseRegionInfo[i].Z) * 256;
        }
    }
	
    return size;
}

/*****************************************************************************
*
* @description
* This function returns the maximun block size in all Norflash regions.
*
* @param    
*           pNorflash  Pointer to an NorFlash instance.
*
* @return	
*           the maximun block size.
*
* @note		NA.
*
*****************************************************************************/
UINT32 norFlashCFI_Get_DevMaxBlkSize(vxT_NORFLASH* pNorflash)
{
    int i = 0;
    UINT16 numBlockRegion;
    unsigned long size ;
	
    numBlockRegion = pNorflash->cfiDescription.norFlashCfiDevGeometry.numEraseRegion;
    
    size = (pNorflash->cfiDescription.norFlashCfiDevGeometry.eraseRegionInfo[0].Z) * 256;
    
    for (i=1; i<numBlockRegion; i++) 
	{
        if (size < (pNorflash->cfiDescription.norFlashCfiDevGeometry.eraseRegionInfo[i].Z) * 256) 
		{
            size = (pNorflash->cfiDescription.norFlashCfiDevGeometry.eraseRegionInfo[i].Z) * 256 ;
        }
    }
	
    return size;
}

/*****************************************************************************
*
* @description
* This function returns the block size in giving block number.
*
* @param    
*           pNorflash  Pointer to an NorFlash instance.
*           sector  Sector number.
*
* @return	
*           block size.
*
* @note		NA.
*
*****************************************************************************/
UINT32 norFlashCFI_Get_DevBlkSize(vxT_NORFLASH* pNorflash, UINT32 sector)
{
    int i = 0;
    UINT16 j;
    UINT16 numBlockRegion,numBlockPerRegion;
    UINT32 block = 0;
    
    numBlockRegion = pNorflash->cfiDescription.norFlashCfiDevGeometry.numEraseRegion;
	
    for (i = 0; i < numBlockRegion; i++) 
	{
        numBlockPerRegion = (pNorflash->cfiDescription.norFlashCfiDevGeometry.eraseRegionInfo[i]).Y + 1;
		
        for (j = 0; j < numBlockPerRegion; j++) 
		{
            if (block == sector) 
			{
                return (pNorflash->cfiDescription.norFlashCfiDevGeometry.eraseRegionInfo[i].Z) * 256 ;
            }
			
            block++;
        }
    }
	
    return 0;
}

/*****************************************************************************
*
* @description
* This function returns sector number on specified memory offset.
*
* @param    
*           pNorflash  Pointer to an NorFlash instance.
*           memoryOffset  Memory offset.
*
* @return	
*           sector num.
*
* @note		NA.
*
*****************************************************************************/
UINT16 norFlashCFI_Get_DevSectInRegion(vxT_NORFLASH* pNorflash, UINT32 memoryOffset)
{
    UINT16 numBlockRegion,numBlockPerRegion;
    UINT16 sectorId = 0;
    UINT32 size = 0;
    UINT8 done = 0;
    int i , j;
    
    numBlockRegion = pNorflash->cfiDescription.norFlashCfiDevGeometry.numEraseRegion;
    
    for (i=0; i<numBlockRegion; i++) 
	{
        numBlockPerRegion = (pNorflash->cfiDescription.norFlashCfiDevGeometry.eraseRegionInfo[i]).Y + 1;
		
        for (j = 0; j < numBlockPerRegion; j++) 
		{
            size += (pNorflash->cfiDescription.norFlashCfiDevGeometry.eraseRegionInfo[i].Z) * 256;
			
            if (size > memoryOffset) 
			{
                done = 1;
                break;
            }
			
            sectorId++;
        }
		
        if (done) 
		{	
			break;
        }
    }
    
    return sectorId;
}

/*****************************************************************************
*
* @description
* This function returns start address of specified sector number.
*
* @param    
*           pNorflash  Pointer to an NorFlash instance.
*           sector  Sector number.
*
* @return	
*           sector address.
*
* @note		NA.
*
*****************************************************************************/
UINT32 norFlashCFI_Get_DevSectAddr(vxT_NORFLASH* pNorflash, UINT32 sector)
{
    UINT16 numBlockRegion,numBlockPerRegion;
    UINT16 sectorId = 0;
    UINT32 address = 0;
    UINT8 done = 0;
    int i, j;
    
    numBlockRegion = pNorflash->cfiDescription.norFlashCfiDevGeometry.numEraseRegion;
	
    for (i=0; i<numBlockRegion; i++) 
	{
        numBlockPerRegion = (pNorflash->cfiDescription.norFlashCfiDevGeometry.eraseRegionInfo[i]).Y + 1;
		
        for (j=0; j<numBlockPerRegion; j++) 
		{
            if (sector == sectorId) 
			{
                done = 1;
                break;
            }
			
            address+= (pNorflash->cfiDescription.norFlashCfiDevGeometry.eraseRegionInfo[i].Z) * 256;
            sectorId++;
        }
		
        if (done) 
		{	
			break;
        }
    }
    
    return address;
}

/*****************************************************************************
*
* @description
* This function converts address to byte addressing.
*
* @param    
*           pNorflash  Pointer to an NorFlash instance.
*           offset Address offset.
*
* @return	
*           byte address.
*
* @note		NA.
*
*****************************************************************************/
UINT32 norFlashCFI_Get_ByteAddr(vxT_NORFLASH* pNorflash, UINT32 offset)
{
    return (offset * pNorflash->devChipWidth);
}

/*****************************************************************************
*
* @description
* This function converts address to byte addressing and 
* return the address in chip.
*
* @param    
*           pNorflash  Pointer to an NorFlash instance.
*           offset Address offset.
*
* @return	
*           address in chip.
*
* @note		NA.
*
*****************************************************************************/
UINT32 norFlashCFI_Get_ByteAddrInChip(vxT_NORFLASH* pNorflash, UINT32 offset)
{
	offset = ((2*offset) + (!(offset & 0x1)));
    return (pNorflash->baseAddr + offset);
}

/*****************************************************************************
*
* @description
* This function returns the address in chip.
*
* @param    
*           pNorflash  Pointer to an NorFlash instance.
*           offset Address offset.
*
* @return	
*           address in chip.
*
* @note		NA.
*
*****************************************************************************/
UINT32 norFlashCFI_Get_AddrInChip(vxT_NORFLASH* pNorflash, UINT32 offset)
{
    return (pNorflash->baseAddr + offset);
}

/*****************************************************************************
*
* @description
* This function returns bus width in bits of giving device.
*
* @param    
*           pNorflash  Pointer to an NorFlash instance.
*
* @return	
*           bus width.
*
* @note		NA.
*
*****************************************************************************/
UINT8 norFlashCFI_Get_DataBusWidth( vxT_NORFLASH* pNorflash)
{
    return (pNorflash->devChipWidth * 8);
}

/*****************************************************************************
*
* @description
* This function returns the size of the whole device in bytes.
*
* @param    
*           pNorflash  Pointer to an NorFlash instance.
*
* @return	
*           device size.
*
* @note		NA.
*
*****************************************************************************/
unsigned long  norFlashCFI_Get_DevSizeInBytes(vxT_NORFLASH* pNorflash)
{
    return ((unsigned long)2 << ((pNorflash->cfiDescription.norFlashCfiDevGeometry.devSize) - 1));
}

/*****************************************************************************
*
* @description
* This function looks for query struct in Norflash common flash interface.
*
* @param    
*           pNorflash Pointer to an pNorflash instance.
*           hardwareBusWidth the bus width.
*
* @return	
*           returns 0 if a matching model has been found.
*           otherwise returns an error code.
*
* @note		NA.
*
*****************************************************************************/
UINT8 norFlashCFI_Detect(vxT_NORFLASH* pNorflash,  UINT8 hardwareBusWidth)
{
    int i = 0;
    UINT8 pCfi[256];
	
    /*UINT8 *Cfi = (UINT8*(&(pNorflash->cfiDescription));*/
    UINT32 baseAddr, CfiQryAddr, cmd_addr;
	
    pNorflash->cfiCompatible = 0;
    pNorflash->devChipWidth = hardwareBusWidth;
	
	baseAddr = pNorflash->baseAddr;
    CfiQryAddr = CFI_QUERY_OFFSET;
	cmd_addr = baseAddr + CFI_QUERY_ADDRESS * 2;
	
    for (i=0; i<sizeof(vxT_NorFlashCFI); i++)
	{
		NOR_WRITE_COMMAND8(cmd_addr, CFI_QUERY_COMMAND);
		
		ReadRawData((baseAddr + (CfiQryAddr << 1)), &pCfi[i]);
		
        CfiQryAddr++;
    }
	
    pNorflash->cfiDescription.norFlashCfiQueryInfo.queryUniqueString[0] = pCfi[0];
    pNorflash->cfiDescription.norFlashCfiQueryInfo.queryUniqueString[1] = pCfi[1];
    pNorflash->cfiDescription.norFlashCfiQueryInfo.queryUniqueString[2] = pCfi[2];
    pNorflash->cfiDescription.norFlashCfiQueryInfo.primaryCode = pCfi[4]<<8 | pCfi[3];
    pNorflash->cfiDescription.norFlashCfiQueryInfo.primaryAddr = pCfi[6]<<8 | pCfi[5];
    pNorflash->cfiDescription.norFlashCfiQueryInfo.alternateCode = pCfi[8]<<8 | pCfi[7];
    pNorflash->cfiDescription.norFlashCfiQueryInfo.alternateAddr = pCfi[10]<<8 | pCfi[9];
    pNorflash->cfiDescription.norFlashCfiQueryInfo.minVcc = pCfi[11];
    pNorflash->cfiDescription.norFlashCfiQueryInfo.maxVcc = pCfi[12];
    pNorflash->cfiDescription.norFlashCfiQueryInfo.minVpp = pCfi[13];
    pNorflash->cfiDescription.norFlashCfiQueryInfo.maxVpp = pCfi[14];
    pNorflash->cfiDescription.norFlashCfiQueryInfo.minTimeOutWrite = pCfi[15];
    pNorflash->cfiDescription.norFlashCfiQueryInfo.minTimeOutBuffer = pCfi[16];
    pNorflash->cfiDescription.norFlashCfiQueryInfo.minTimeOutBlockErase = pCfi[17];
    pNorflash->cfiDescription.norFlashCfiQueryInfo.minTimeOutChipErase = pCfi[18];
    pNorflash->cfiDescription.norFlashCfiQueryInfo.maxTimeOutWrite = pCfi[19];
    pNorflash->cfiDescription.norFlashCfiQueryInfo.maxTimeOutBuffer = pCfi[20];
    pNorflash->cfiDescription.norFlashCfiQueryInfo.maxTimeOutBlockErase = pCfi[21];
    pNorflash->cfiDescription.norFlashCfiQueryInfo.maxTimeOutChipErase = pCfi[22];
    pNorflash->cfiDescription.norFlashCfiDevGeometry.devSize =  pCfi[23];

    pNorflash->cfiDescription.norFlashCfiDevGeometry.devInterface =  pCfi[25]<<8 | pCfi[24];
    pNorflash->cfiDescription.norFlashCfiDevGeometry.numMultiWrite = pCfi[27]<<8 | pCfi[26];
    pNorflash->cfiDescription.norFlashCfiDevGeometry.numEraseRegion = pCfi[28];
	
    for (i=0; i<16*4; i+=4) 
	{
        pNorflash->cfiDescription.norFlashCfiDevGeometry.eraseRegionInfo[i>>2].Y = pCfi[30+i]<<8 | pCfi[29+i];
        pNorflash->cfiDescription.norFlashCfiDevGeometry.eraseRegionInfo[i>>2].Z = pCfi[32+i]<<8 | pCfi[31+i];
    }
	
    /* Check the query-unique ASCII string "QRY"*/
    if ((pNorflash->cfiDescription.norFlashCfiQueryInfo.queryUniqueString[0] != 'Q' )
        || (pNorflash->cfiDescription.norFlashCfiQueryInfo.queryUniqueString[1] != 'R') 
        || (pNorflash->cfiDescription.norFlashCfiQueryInfo.queryUniqueString[2] != 'Y') ) 
    {
        return NorCommon_ERROR_UNKNOWNMODEL;
    }
    
    norFlashCFI_Dump_Cfg(&(pNorflash->cfiDescription));

    if ((pNorflash->cfiDescription.norFlashCfiQueryInfo.primaryCode == CMD_SET_AMD) ||
		(pNorflash->cfiDescription.norFlashCfiQueryInfo.primaryCode == CMD_SET_AMD_EXT)) 
	{
        pNorflash->cmdSet = CMD_SET_AMD;
    }
    else if ((pNorflash->cfiDescription.norFlashCfiQueryInfo.primaryCode == CMD_SET_INTEL_EXT) || 
             (pNorflash->cfiDescription.norFlashCfiQueryInfo.primaryCode == CMD_SET_INTEL)) 
    {
        pNorflash->cmdSet = CMD_SET_INTEL;
    }
    else 
	{
        return NorCommon_ERROR_UNKNOWNMODEL;
    }
    
    pNorflash->cfiCompatible = 1;
	
    norFlash_Reset(pNorflash, 0);
	
    return 0;
}

#endif





