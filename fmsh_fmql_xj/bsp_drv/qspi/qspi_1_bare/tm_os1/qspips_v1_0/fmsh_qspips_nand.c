/******************************************************************************
*
* Copyright (C) FMSH, Corp.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* FMSH BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the FMSH shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from FMSH.
*
******************************************************************************/
/*****************************************************************************/
/**
*
* @file fmsh_qspips_nand.c
* @addtogroup qspips_v1_1
* @{
*
* Contains implements the spi nand flash interface functions of 
* the FQspiPs driver.
* See fmsh_qspips_nand.h for a detailed description of the device and driver.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who Date     Changes
* ----- --- -------- -----------------------------------------------
* 1.10  hzq 11/26/20 
* 		     First release
*
* </pre>
*
******************************************************************************/

#include <string.h>
#include "fmsh_qspips_lib.h"

FQspiPs_Nand_Device qspi_nand_dev;

static void FQspiPs_Nand_InitBadBlockTableDesc(FQspiPs_T* qspiPtr);
static int FQspiPs_CreateBadBlockTable(FQspiPs_T* qspiPtr, u32 target);
static int FQspiPs_Nand_CheckBlock(FQspiPs_T* qspiPtr, u32 block) ;

FQspiPs_Caps qspi_nand_caps_default = {
    /*
    .hasIntr = 1,
    .hasDma = 0,
    .hasXIP = 0,
    .txEmptyLvl = 32,
    .rxFullLvl = 480,
    .rdMode = QOR_CMD,
    .baudRate = 4, 
    .addressBytes = 2,
    */
    1,        /* hasIntr */
    0,        /* hasDma */
    0,        /* hasXIP */
    32,       /* txEmptyLvl */
    480,      /* rxFullLvl */
    QOR_CMD,  /* rdMode */
    4,        /* baudRate */
    0,        /* sampleDelay*/
    2,        /* addressBytes */
};

/*****************************************************************************
* This function init nand device
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_Initr(FQspiPs_T* qspiPtr)
{
    int status;
    
    FQspiPs_Nand_EnableQuad(qspiPtr);
    FQspiPs_Nand_EnableECC(qspiPtr);
    /*init bbm*/
    FQspiPs_Nand_InitBadBlockTableDesc(qspiPtr);
    status = FQspiPs_CreateBadBlockTable(qspiPtr, 0);
    if(status != FMSH_SUCCESS)
    {
        return status;
    }
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function check flash id & fill nand device struct
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_GetFlashInfo(FQspiPs_T* qspiPtr, u8* id)
{
    int status;
    u8 buf[FQSPIPS_NAND_MAXPAGESIZE + FQSPIPS_NAND_MAXSPARESIZE];
    FQspiPs_Nand_ParamPage param[3];
    FQspiPs_Nand_Device* dev;
    
    FMSH_ASSERT(qspiPtr != NULL);
    
    qspiPtr->dev = &qspi_nand_dev;
    dev = (FQspiPs_Nand_Device*)qspiPtr->dev;
    
    dev->name = "FMSH SPI NAND";
    dev->id_len = 2;
    
    /* flash make */
    switch(id[1])
    {
    case SPI_FMSH_ID: 
        qspiPtr->maker = dev->maker = id[1];
        break;
    default: 
        qspiPtr->maker = dev->maker = SPI_UNKNOWN_ID; 
        break;
    }
    
    /*device size maxium*/
    dev->deviceSize = qspiPtr->devSize = SPI_UNKNOWN_SIZE;
    /*block size = 128KB*/
    dev->blockSize = qspiPtr->sectorSize = 0x1 << 17; 
    /*page size = 2KB*/
    dev->pageSize = 0x1 << 11;
    /*spare size = 64B*/
    dev->spareSize = 0x1 << 6;
    /**/
    dev->numPagesPerBlock = 64;
    dev->numBlocksPerTarget = 2048; 
    
    status = FQspiPs_Nand_ReadParaPage(qspiPtr, buf);
    memcpy(param, buf, FQSPIPS_PARAM_PAGE_SIZE * 3);
    if(status == FMSH_SUCCESS && 
       param[0].Signature[0] == 0x4F && param[0].Signature[1] == 0x4E &&
       param[0].Signature[2] == 0x46 && param[0].Signature[3] == 0x49)
    {
        /* decode parameter page */
        dev->deviceSize = qspiPtr->devSize = 
                param[0].BytesPerPage * param[0].PagesPerBlock * 
                param[0].BlocksPerLun * param[0].NumLuns;
        dev->blockSize = qspiPtr->sectorSize = 
                param[0].BytesPerPage * param[0].PagesPerBlock; 
        dev->pageSize = param[0].BytesPerPage;    
        dev->spareSize = param[0].SpareBytesPerPage; 
        dev->numPagesPerBlock = param[0].PagesPerBlock;
        dev->numBlocksPerTarget = param[0].BlocksPerLun * param[0].NumLuns;
            
        return FMSH_SUCCESS;
    }
     
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function set controller read mode(x1, x2, x4)
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_SetFlashMode(FQspiPs_T* qspiPtr, u8 mode)
{
    u32 configRead, configWrite;
    
    FMSH_ASSERT(qspiPtr != NULL);
    
    configWrite = 0x00000184; 
    switch(mode)
    {
    case READ_CMD: 
        configRead = 0x00000003; 
        break;
    case FAST_READ_CMD: 
        configRead = 0x0800000B; 
        break;
    case DOR_CMD: 
        configRead = 0x0801003B; 
        break;
    case QOR_CMD: 
        configRead = 0x0802006B; 
        configWrite = 0x00020134; 
        break;
    case DIOR_CMD: 
        configRead = 0x001110BB; 
        break;
    case QIOR_CMD: 
        configRead = 0x041220EB; 
        configWrite = 0x00220172; 
        break;
    default: 
        configRead = 0x0800000B; 
        break;
    }
    
    FQspiPs_SetAddrBytesNum(qspiPtr, qspiPtr->caps.addressBytes - 1); 
    FQspiPs_SetPageSize(qspiPtr, 512);
    FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_DWIR_OFFSET, configWrite);
    FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_DRIR_OFFSET, configRead);
    FQspiPs_DisablePoll(qspiPtr);
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function erase nand flash
*
* @param
*       -offset: start address to erase(recommand to align to block boundary)
*       -byteCount: number of bytes to erase(recommand to align to block size)
*       -blockSize: not used
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_Erase(FQspiPs_T* qspiPtr, u64 offset, u32 byteCount, 
                       u32 blockSize)
{
    u8 status;
    int blockCount;
    u32 block;
    u32 firstBlock, lastBlock;
    FQspiPs_Nand_Device* dev = (FQspiPs_Nand_Device*)qspiPtr->dev;

    FMSH_ASSERT(qspiPtr != NULL);
    FMSH_ASSERT(byteCount != 0);
    
    firstBlock = offset / dev->blockSize;
    lastBlock = (offset + byteCount-1)  / dev->blockSize;
    blockCount = lastBlock - firstBlock + 1;
    
    block = firstBlock;    
    while(blockCount > 0)
    {
        status = FQspiPs_Nand_EraseBlock(qspiPtr, block);
        if(status != FMSH_SUCCESS)
        {
            return FMSH_FAILURE;     
        }
        blockCount--;
        block++;
    }
        
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function erase nand flash block
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_EraseBlock(FQspiPs_T* qspiPtr, u32 block)
{
    int status;
    u8 value;
    FQspiPs_Nand_Device* dev = (FQspiPs_Nand_Device*)qspiPtr->dev;
    
    status = FQspiPs_Nand_WREN(qspiPtr);
    if(status != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;  
    }
        
    FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_FCAR_OFFSET, 
                  block * dev->numPagesPerBlock); /*3 bytes address*/
	FQspiPs_CmdExecute(qspiPtr, 0xD80A0000);    
    status = FQspiPs_Nand_WaitReady(qspiPtr, &value);
    if(status != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;
    }
    
    if(value & FQSPIPS_NAND_S3_EFAIL)
        return FMSH_FAILURE;
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function write nand flash
*
* @param
*       -offset: start address to program
*       -byteCount: number of bytes to program
*       -sendBuffer: point to data to program
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_Write(FQspiPs_T* qspiPtr, u64 offset, u32 byteCount, 
                       u8* data)
{
    u8 status;
    u16 colAddr;
    u16 length;
    u32 blockSize, pageSize, spareSize;
    u32 block, page;
    u8 buf[FQSPIPS_NAND_MAXPAGESIZE + FQSPIPS_NAND_MAXSPARESIZE];
    FQspiPs_Nand_Device* dev = (FQspiPs_Nand_Device*)qspiPtr->dev;
    
    FMSH_ASSERT(qspiPtr != NULL);
    FMSH_ASSERT(byteCount != 0);
    FMSH_ASSERT(data != NULL);
    
    blockSize = dev->blockSize;
    pageSize = dev->pageSize;
    spareSize = dev->spareSize;
    
    while(byteCount > 0)
    {
        colAddr = offset & (pageSize - 1);
        length = pageSize - colAddr;
        if(byteCount < length)
        {
            length = byteCount;
        }
        /* init & fill buffer */
        memset(&buf[0], 0xff, pageSize + spareSize);
        memcpy(&buf[colAddr], data, length);
        
        block = offset / blockSize;
        page = (offset & (blockSize - 1)) / pageSize;
        status = FQspiPs_Nand_ProgramPage(qspiPtr, block, page, 0, &buf[0]);
        if(status != FMSH_SUCCESS)
            return FMSH_FAILURE;

        /* check has more data in buffer to write */
        offset += length;
        data += length;
        byteCount -= length;
    }   
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function write nand flash spare page
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_WriteSpareBytes(FQspiPs_T* qspiPtr, u32 block, u32 page, 
                                 u8* data)
{
    int status;
    u8 value;
    u32 rowAddress;
    u8 buf[FQSPIPS_NAND_MAXPAGESIZE + FQSPIPS_NAND_MAXSPARESIZE];
    FQspiPs_Nand_Device* dev = (FQspiPs_Nand_Device*)qspiPtr->dev;
    u32 pageSize = dev->pageSize;
    u32 spareSize = dev->spareSize;
    u32 writeSize = dev->pageSize + dev->spareSize;

    FMSH_ASSERT(qspiPtr != NULL);
    FMSH_ASSERT(data != NULL);
    
    /* prepare data */
    memset((void*)buf, 0xff, writeSize); 
    memcpy((void*)(buf + (0x1 << pageSize)),
           (void*)data,
           (size_t)spareSize);
    
    /* send data to cache */
    qspiPtr->isBusy = TRUE;
    memcpy((void*)(qspiPtr->config.dataBaseAddress),
           (void*)buf,
           (size_t)writeSize); 
    qspiPtr->isBusy = FALSE;
    
    /* program page */
    status = FQspiPs_Nand_WREN(qspiPtr);
    if(status != FMSH_SUCCESS)
        return FMSH_FAILURE;  
    
    rowAddress = (block * dev->numPagesPerBlock) + page;
    FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_FCAR_OFFSET, rowAddress); /*3 bytes address*/
    FQspiPs_CmdExecute(qspiPtr, 0x100A0000);    
    
    status = FQspiPs_Nand_WaitReady(qspiPtr, &value);  
    if(status != FMSH_SUCCESS)
        return FMSH_FAILURE;  
    
    if(value & FQSPIPS_NAND_S3_PFAIL)
        return FMSH_FAILURE;
    
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function write nand flash page
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_ProgramPage(FQspiPs_T* qspiPtr, u32 block, u32 page, 
                             u16 offset, u8* data)
{
    int status;
    u8 value;
    u32 rowAddress;
    u8 buf[FQSPIPS_NAND_MAXPAGESIZE + FQSPIPS_NAND_MAXSPARESIZE];
    FQspiPs_Nand_Device* dev = (FQspiPs_Nand_Device*)qspiPtr->dev;
    u32 pageSize = dev->pageSize;
    u32 writeSize = dev->pageSize + dev->spareSize;

    FMSH_ASSERT(qspiPtr != NULL);
    FMSH_ASSERT(data != NULL);
    
    u8 subPageNum;
    for(subPageNum = 0; subPageNum < 4; subPageNum++)
    {
        /* prepare data */
        memset((void*)buf, 0xff, writeSize); 
        
        if(subPageNum == 0)
        {
            memcpy((void*)(buf),
                   (void*)(data),
                   (size_t)512); 
            memcpy((void*)(buf + pageSize),
                   (void*)(data + pageSize),
                   (size_t)16); 
        }
        else if(subPageNum == 1)
        {
            memcpy((void*)(buf + 512),
                   (void*)(data + 512),
                   (size_t)512); 
            memcpy((void*)(buf + pageSize + 16),
                   (void*)(data + pageSize + 16),
                   (size_t)16);         
        }
        else if(subPageNum == 2)
        {
            memcpy((void*)(buf + 1024),
                   (void*)(data + 1024),
                   (size_t)512); 
            memcpy((void*)(buf + pageSize + 32),
                   (void*)(data + pageSize + 32),
                   (size_t)16); 
        }
        else if(subPageNum == 3)
        {
            memcpy((void*)(buf + 1536),
                   (void*)(data + 1536),
                   (size_t)512); 
            memcpy((void*)(buf + pageSize + 48),
                   (void*)(data + pageSize + 48),
                   (size_t)16);         
        }
        
        /* send data to cache */
        qspiPtr->isBusy = TRUE;
        memcpy((void*)(qspiPtr->config.dataBaseAddress),
               (void*)buf,
               (size_t)writeSize); 
        qspiPtr->isBusy = FALSE;
        /*
        FQspiPs_FastSendBytes(qspiPtr, 0, writeSize, data);
        while(qspiPtr->isBusy == 1);
        */
        
        /* program page */
        status = FQspiPs_Nand_WREN(qspiPtr);
        if(status != FMSH_SUCCESS)
            return FMSH_FAILURE;  
        
        rowAddress = (block * dev->numPagesPerBlock) + page;
        FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_FCAR_OFFSET, rowAddress); /*3 bytes address*/
        FQspiPs_CmdExecute(qspiPtr, 0x100A0000);    
        
        status = FQspiPs_Nand_WaitReady(qspiPtr, &value);  
        if(status != FMSH_SUCCESS)
            return FMSH_FAILURE;  
        
        if(value & FQSPIPS_NAND_S3_PFAIL)
            return FMSH_FAILURE;
    }
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function write nand flash
*
* @param
*       -offset: start address to read
*       -byteCount: number of bytes to read
*       -sendBuffer: point to data to read
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_Read(FQspiPs_T* qspiPtr, u64 offset, u32 byteCount, u8* data)
{
    u8 status;
    u16 colAddr;
    u16 length;
    u32 blockSize, pageSize;
    u32 block, page;
    u8 buf[FQSPIPS_NAND_MAXPAGESIZE + FQSPIPS_NAND_MAXSPARESIZE];
    FQspiPs_Nand_Device* dev = (FQspiPs_Nand_Device*)qspiPtr->dev;
    
    FMSH_ASSERT(qspiPtr != NULL);
    FMSH_ASSERT(byteCount != 0);
    
    blockSize = dev->blockSize;
    pageSize = dev->pageSize;
    
    while(byteCount > 0)
    {
        colAddr = offset & (pageSize - 1);
        length = pageSize - colAddr;
        /* read entire page from nandflash */
        block = offset / blockSize;
        page = (offset & (blockSize - 1)) / pageSize;
        status = FQspiPs_Nand_ReadPage(qspiPtr, block, page, 0, &buf[0]);
        if(status != FMSH_SUCCESS)
                return FMSH_FAILURE;

        if(byteCount < length)
        {
            length = byteCount;
        }
        /* init & fill buffer */
        memcpy(data, &buf[colAddr], length);
        offset += length;
        data += length;
        byteCount -= length;
    }       
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function read nand flash spare page
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_ReadSpareBytes(FQspiPs_T* qspiPtr, u32 block, u32 page, 
                                 u8* data)
{
    int status;
    u8 value;
    u32 rowAddress;
    FQspiPs_Nand_Device* dev = (FQspiPs_Nand_Device*)qspiPtr->dev;
    u32 pageSize = dev->pageSize;
    u32 spareSize = dev->spareSize;

    /* read page */
    rowAddress = (block * dev->numPagesPerBlock) + page;
	FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_FCAR_OFFSET, rowAddress); /*3 bytes address*/
	FQspiPs_CmdExecute(qspiPtr, 0x130A0000);    
    status = FQspiPs_Nand_WaitReady(qspiPtr, &value);
    if(status != FMSH_SUCCESS)
        return QSPI_TIMEOUT;    

    switch(value & FQSPIPS_NAND_ECC_STATUS)
    {
    case 0:
        /* no error */
        break;
    case FQSPIPS_NAND_S3_ECCS0:
        /* correctable error */
        break;
    default:
        /* uncorrectable error */
        return FMSH_FAILURE;
        break;
    }
    
    /* read data from cache */
    qspiPtr->isBusy = TRUE;
    memcpy((void*)data,
           (void*)(qspiPtr->config.dataBaseAddress + pageSize),
           (size_t)spareSize); 
    qspiPtr->isBusy = FALSE;  
        
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function read nand flash page
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_ReadPage(FQspiPs_T* qspiPtr, u32 block, u32 page, 
                             u16 offset, u8* data)
{
    int status;
    u8 value;
    u32 rowAddress;
    FQspiPs_Nand_Device* dev = (FQspiPs_Nand_Device*)qspiPtr->dev;
    u32 readSize = dev->pageSize + dev->spareSize;

    /* read page */
    rowAddress = (block * dev->numPagesPerBlock) + page;
	FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_FCAR_OFFSET, rowAddress); /*3 bytes address*/
	FQspiPs_CmdExecute(qspiPtr, 0x130A0000);    
    status = FQspiPs_Nand_WaitReady(qspiPtr, &value);
    if(status != FMSH_SUCCESS)
        return QSPI_TIMEOUT;    
    
    /* read data from cache */
    qspiPtr->isBusy = TRUE;
    memcpy((void*)data,
           (void*)qspiPtr->config.dataBaseAddress,
           (size_t)readSize); 
    qspiPtr->isBusy = FALSE;  
    
    /*
    FQspiPs_FastRecvBytes(qspiPtr, 0, 2176, recvBuffer);
    while(qspiPtr->isBusy == 1);*/

    switch(value & FQSPIPS_NAND_ECC_STATUS)
    {
    case 0:
        /* no error */
        break;
    case FQSPIPS_NAND_S3_ECCS0:
        /* correctable error */
        break;
    default:
        /* uncorrectable error */
        return FMSH_FAILURE;
        break;
    }
        
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function read Unique Id page
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_ReadUniqueID(FQspiPs_T* qspiPtr, u8* data)
{
    int status;
    
    /* enable otp access*/
    FQspiPs_Nand_EnableOtp(qspiPtr);
    
    status = FQspiPs_Nand_ReadPage(qspiPtr, 0, 0, 0, data);
    
    /* disable otp access*/
    FQspiPs_Nand_DisableOtp(qspiPtr);
    
    if(status != FMSH_SUCCESS)
        return FMSH_FAILURE;
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function read Parameter page
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_ReadParaPage(FQspiPs_T* qspiPtr, u8* data)
{
    int status;
    
    /* enable otp access*/
    FQspiPs_Nand_EnableOtp(qspiPtr);
    status = FQspiPs_Nand_ReadPage(qspiPtr, 0, 1, 0, data);
    /* disable otp access*/
    FQspiPs_Nand_DisableOtp(qspiPtr);
    if(status != FMSH_SUCCESS)
        return FMSH_FAILURE;
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function read otp page
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_OtpPage(FQspiPs_T* qspiPtr, u8 page, u8* data)
{
    int status;
    
    /* enable otp access*/
    FQspiPs_Nand_EnableOtp(qspiPtr);
    
    status = FQspiPs_Nand_ReadPage(qspiPtr, 0, page, 0, data);
    
    /* disable otp access*/
    FQspiPs_Nand_DisableOtp(qspiPtr);
    
    if(status != FMSH_SUCCESS)
        return FMSH_FAILURE;
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function reset flash
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_Reset(FQspiPs_T* qspiPtr)
{
    int status;
    
    status = FQspiPs_CmdExecute(qspiPtr, 0xFF000000); 
    delay_ms(1);

    return status;
}

/*****************************************************************************
* This function get feature value
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
u8 FQspiPs_Nand_GetFeature(FQspiPs_T* qspiPtr, u8 feature)
{
    u32 value;
    
	FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_FCAR_OFFSET, feature);
    /* read 1 byte data with 1 byte address*/
	FQspiPs_CmdExecute(qspiPtr, 0x0F880000);    
    value = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_FCRDRL_OFFSET);
    
    return (u8)value;
}

/*****************************************************************************
* This function set feature value
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_SetFeature(FQspiPs_T* qspiPtr, u8 feature, u8 value)
{
    int status;
    
    FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_FCWDRL_OFFSET, (u32)value);
	FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_FCAR_OFFSET, feature); 
    /* write 1 byte data with 1 byte address*/
	status = FQspiPs_CmdExecute(qspiPtr, 0x1F088000);    
        
    return status;
}

/*****************************************************************************
* This function lock flash
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_Lock(FQspiPs_T* qspiPtr)
{
    u8 value = 0;
    
    /* set protection register/status register-1*/
    value |= FQSPIPS_NAND_S1_BP0;
    value |= FQSPIPS_NAND_S1_BP1;
    value |= FQSPIPS_NAND_S1_BP2;
    FQspiPs_Nand_SetFeature(qspiPtr, 0xA0, value);
    value = FQspiPs_Nand_GetFeature(qspiPtr, 0xA0);
    if((value & 0x38) != 0x38)
        return FMSH_FAILURE;     

    return FMSH_SUCCESS;    
}

/*****************************************************************************
* This function unlock flash
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_Unlock(FQspiPs_T* qspiPtr)
{
    u8 value = 0;

    /* set protection register/status register-1*/
    FQspiPs_Nand_SetFeature(qspiPtr, 0xA0, value);
    value = FQspiPs_Nand_GetFeature(qspiPtr, 0xA0);
    if((value & 0x38) != 0)
        return FMSH_FAILURE;      
    
    return FMSH_SUCCESS;    
}

/*****************************************************************************
* This function enable flash quad io
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_EnableQuad(FQspiPs_T* qspiPtr)
{
    int status;
    u8 value;
    
    /* set configuration register/status register-2*/
    value = FQspiPs_Nand_GetFeature(qspiPtr, 0xB0);
    value |= FQSPIPS_NAND_S2_QE;    
	status = FQspiPs_Nand_SetFeature(qspiPtr, 0xB0, value); 
    
    return status;  
}

/*****************************************************************************
* This function disable flash quad io
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_DisableQuad(FQspiPs_T* qspiPtr)
{
    int status;
    u8 value;
    
    value = FQspiPs_Nand_GetFeature(qspiPtr, 0xB0);
    value &= ~(u8)FQSPIPS_NAND_S2_QE;    
	status = FQspiPs_Nand_SetFeature(qspiPtr, 0xB0, value);
    
    return status;  
}

/*****************************************************************************
* This function enable flash ondie ecc
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_EnableECC(FQspiPs_T* qspiPtr)
{
    int status;
    u8 value;
    
    value = FQspiPs_Nand_GetFeature(qspiPtr, 0xB0);
    value |= FQSPIPS_NAND_S2_ECC_EN;    
	status = FQspiPs_Nand_SetFeature(qspiPtr, 0xB0, value); 
    
    return status;  
}

/*****************************************************************************
* This function disable flash ondie ecc
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_DisableECC(FQspiPs_T* qspiPtr)
{
    int status;
    u8 value;
    
    value = FQspiPs_Nand_GetFeature(qspiPtr, 0xB0);
    value &= ~(u8)FQSPIPS_NAND_S2_ECC_EN;    
	status = FQspiPs_Nand_SetFeature(qspiPtr, 0xB0, value); 
    
    return status;  
}

/*****************************************************************************
* This function enable otp access
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_EnableOtp(FQspiPs_T* qspiPtr)
{
    int status;
    u8 value;
    
    value = FQspiPs_Nand_GetFeature(qspiPtr, 0xB0);
    value |= FQSPIPS_NAND_S2_OTP_EN;    
	status = FQspiPs_Nand_SetFeature(qspiPtr, 0xB0, value); 
    
    return status;  
}

/*****************************************************************************
* This function disable otp access
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_DisableOtp(FQspiPs_T* qspiPtr)
{
    int status;
    u8 value;
    
    value = FQspiPs_Nand_GetFeature(qspiPtr, 0xB0);
    value &= ~(u8)FQSPIPS_NAND_S2_OTP_EN;    
	status = FQspiPs_Nand_SetFeature(qspiPtr, 0xB0, value); 
    
    return status;  
}

/*****************************************************************************
* This function wait for status OIP bit
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_WaitReady(FQspiPs_T* qspiPtr, u8* status)
{
    u8 value;
    int timeout = 0;
    
    while(1)
    {
        value = FQspiPs_Nand_GetFeature(qspiPtr, 0xC0);
        if((value & FQSPIPS_NAND_S3_OIP) == 0)
        {
            if(status != NULL)
                *status = value;
            return FMSH_SUCCESS;
        }
        delay_us(1);
        timeout++;
        if(timeout > 10000)
            return QSPI_TIMEOUT;
    }
}

/*****************************************************************************
* This function executes WREN.
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/

int FQspiPs_Nand_WREN(FQspiPs_T* qspiPtr) 
{
    int error = 0;
    u8 status;
    u32 timeout = 0; 
    /* Send WREN(0x06) Command*/
    error = FQspiPs_CmdExecute(qspiPtr, 0x06000000);    
    if(error){
        return FMSH_FAILURE;
    }
    while(1){     
        /* poll status.wel value*/
        status = FQspiPs_Nand_GetFeature(qspiPtr, 0xC0);
        if(status & FQSPIPS_NAND_S3_WEL){
            return FMSH_SUCCESS;
        }
        delay_1ms();
        timeout++;
        if(timeout > 750){
            return FMSH_FAILURE;
        }
    }
}

/*****************************************************************************
* This function executes WRDI.
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nand_WRDI(FQspiPs_T* qspiPtr)
{ 
    int error = 0;
    u8 status;
    u32 timeout = 0;
    /* Send WREN(0x04) Command*/
    error = FQspiPs_CmdExecute(qspiPtr, 0x04000000);
    if(error){
        return FMSH_FAILURE;
    }
    while(1){
        /* poll status.wel value   */
        status = FQspiPs_Nand_GetFeature(qspiPtr, 0xC0);
        status = ~status;
        if(status & FQSPIPS_NAND_S3_WEL){
            return FMSH_SUCCESS;
        }
        delay_1ms();
        timeout++;
        if(timeout > 750){
            return FMSH_FAILURE;
        }
    }
}

static void FQspiPs_Nand_InitBadBlockTableDesc(FQspiPs_T* qspiPtr)
{ 
    int index;
    FQspiPs_Nand_Device* dev = (FQspiPs_Nand_Device*)qspiPtr->dev;
    
    FMSH_ASSERT(qspiPtr != NULL);

    /* Initialize Bad block search pattern structure */
    dev->bbPattern.options = FQSPIPS_NAND_BBT_SCAN_2ND_PAGE;
    dev->bbPattern.offset = FQSPIPS_NAND_BB_PATTERN_OFFSET;
    dev->bbPattern.length = FQSPIPS_NAND_BB_PATTERN_LENGTH;
    
    for(index=0; index < FQSPIPS_NAND_BB_PATTERN_LENGTH; index++) 
    {
        dev->bbPattern.pattern[index] = FQSPIPS_NAND_BB_PATTERN_GOOD;
    }
}

static int FQspiPs_CreateBadBlockTable(FQspiPs_T* qspiPtr, u32 target)
{  
    int status;
    FQspiPs_Nand_Device* dev = (FQspiPs_Nand_Device*)qspiPtr->dev;

    u32 blockIndex; /* loop all blocks in flash */
    u32 blockOffset;
    u32 blockShift;  
    u32 numBlocks;
    u32 startBlock;

    numBlocks = FQSPIPS_NAND_NUM_BBT_CHECK_BLOCKS;
    startBlock = (target * numBlocks);
    memset(dev->bbInfo, 0xff, dev->numBlocksPerTarget);
    
    /* scan all blocks for factory marked bad blocks */
    for(blockIndex = startBlock; blockIndex < (startBlock + numBlocks); blockIndex++)
    {
        /* Block offset in Bad Block Table(BBT) entry */
        blockOffset = blockIndex >> FQSPIPS_NAND_BBT_BLOCK_SHIFT;
        /* Block shift value in the byte */
        blockShift = (blockIndex << 1) & FQSPIPS_NAND_BLOCK_SHIFT_MASK;
        /* Search for the bad block pattern */
        status = FQspiPs_Nand_CheckBlock(qspiPtr, blockIndex);
        if(status != FMSH_SUCCESS)
        {
            /* Marking as bad block (bbInfo[]) */
            dev->bbInfo[blockOffset] &= 
                ((~(FQSPIPS_NAND_BLOCK_TYPE_MASK << blockShift)) | 
                 (FQSPIPS_NAND_BLOCK_FACTORY_BAD << blockShift));   
        }
    }
    
    return FMSH_SUCCESS;
}

static int FQspiPs_Nand_CheckBlock(FQspiPs_T* qspiPtr, u32 block) 
{
    int status;
    
    u32 numPages;
    u32 pageIndex; /* Page index which stores bad block pattern */
    u32 length; /* Pattern length index */
    u8 spare[FQSPIPS_NAND_MAXSPARESIZE];
    FQspiPs_Nand_Device* dev = (FQspiPs_Nand_Device*)qspiPtr->dev;
    
    /* Number of pages to search for bad block pattern */
    if (dev->bbPattern.options & FQSPIPS_NAND_BBT_SCAN_2ND_PAGE)
    {
        numPages = 2;
    } 
    else 
    {
        numPages = 1;
    }
    
    /* Search for the bad block pattern */
    for(pageIndex = 0; pageIndex < numPages; pageIndex++) 
    {
        status = FQspiPs_Nand_ReadSpareBytes(qspiPtr, block, pageIndex, spare);
        if (status != FMSH_SUCCESS) 
        {
            return FMSH_FAILURE;
        }
        
        /* Read the spare bytes to check for bad block pattern */
        for(length = 0; length < dev->bbPattern.length; length++) 
        {
            if (spare[dev->bbPattern.offset + length] !=
                dev->bbPattern.pattern[length])
            {
                /* Bad block found, return error to marking as bad block */
                return FMSH_FAILURE;
            }
        }
    }
    
    return FMSH_SUCCESS;
}

int FQspiPs_Nand_MarkBlockBad(FQspiPs_T* qspiPtr, u32 block) 
{
    int status; 
    
    u32 blockSize;
    u32 blockOffset;/* byte offset in bbt contain bad block info */
    u32 blockShift;/* shift in byte save bad block info */
    u8 mark;
    u8 numPages;
    u8 pageIndex;
    u8 spare[FQSPIPS_NAND_MAXSPARESIZE];
    FQspiPs_Nand_Device* dev = (FQspiPs_Nand_Device*)qspiPtr->dev;
    
    FMSH_ASSERT(qspiPtr != NULL);
    
    /* update Bad Block Table(BBT) in RAM */
    blockOffset = block >> FQSPIPS_NAND_BBT_BLOCK_SHIFT;
    blockShift = (block << 1) & FQSPIPS_NAND_BLOCK_SHIFT_MASK;  
    mark = dev->bbInfo[blockOffset];
    mark &= ((~(FQSPIPS_NAND_BLOCK_TYPE_MASK << blockShift)) |
             (FQSPIPS_NAND_BLOCK_BAD << blockShift));
    dev->bbInfo[blockOffset] = mark;
    
    /* erase data in bad block */
    blockSize = dev->blockSize;
    FQspiPs_Nand_Erase(qspiPtr, block * blockSize, blockSize, blockSize);
    
    /* write bad block pattern to spare */ 
    memset(&spare[0], 0xff, dev->spareSize);
    memset(&spare[dev->bbPattern.offset], FQSPIPS_NAND_BB_PATTERN_BAD, dev->bbPattern.length);
    if (dev->bbPattern.options & FQSPIPS_NAND_BBT_SCAN_2ND_PAGE)
    {
        numPages = 2;
    } 
    else 
    {
        numPages = 1;
    }
    for(pageIndex = 0; pageIndex < numPages; pageIndex++) 
    {
        status = FQspiPs_Nand_WriteSpareBytes(qspiPtr, block, pageIndex, spare);
        if(status != FMSH_SUCCESS)
        {
            return FMSH_FAILURE;
        }
    }     

    return FMSH_SUCCESS;
}

int FQspiPs_Nand_IsBlockBad(FQspiPs_T* qspiPtr, u32 block) 
{
    u8 data;
    u32 blockOffset;
	u32 blockShift;
	u8 blockType;
    FQspiPs_Nand_Device* dev = (FQspiPs_Nand_Device*)qspiPtr->dev;
    
    FMSH_ASSERT(qspiPtr != NULL);
    
    blockOffset = block >> FQSPIPS_NAND_BBT_BLOCK_SHIFT;
    blockShift = (block << 1) & FQSPIPS_NAND_BLOCK_SHIFT_MASK;
    data = dev->bbInfo[blockOffset];
    blockType = (data >> blockShift) & FQSPIPS_NAND_BLOCK_TYPE_MASK;

	if ((blockType != FQSPIPS_NAND_BLOCK_GOOD) &&
        (blockType != FQSPIPS_NAND_BLOCK_RESERVED))
    {
        /* return failure if block bad */
		return FMSH_FAILURE;
    }
	else
    {
		return FMSH_SUCCESS;
    }
}


/*****************************************************************************/
/**
*
* This function returns the length including bad blocks from a given offset and
* length.
*
* @param	qspiPtr is the pointer to the FQspiPs_T instance.
* @param	offset is the flash data address to read from.
* @param	length is number of bytes to read.
*
* @return
*		- Return actual length including bad blocks.
*
* @note		None.
*
******************************************************************************/
static int FQspiPs_Nand_CalculateLength(FQspiPs_T* qspiPtr, u64 offset, u64 length)
{
    u32 blockSize;
	u32 remainLen;
	u32 block;
	u32 validLen = 0;
	u32 actLen = 0;
    FQspiPs_Nand_Device* dev = (FQspiPs_Nand_Device*)qspiPtr->dev;
    
    FMSH_ASSERT(qspiPtr != NULL);
    
    blockSize = dev->blockSize;
    
	while (validLen < length) 
    {
		remainLen = blockSize - (offset & (blockSize - 1));
		block = offset / blockSize;
		/* Check if the block is bad */
        if(FQspiPs_Nand_IsBlockBad(qspiPtr, block) == FMSH_SUCCESS)
        {
            /* good block */
			validLen += remainLen;
		}
		actLen += remainLen;
		offset += remainLen;
	}
    
	return FMSH_SUCCESS;
}

static int FQspiPs_Nand_CalculateBadBlockNum(FQspiPs_T* qspiPtr, u16 block)
{
    u16 blockIndex;
    u16 skipBlockNum = 0;
    
    for(blockIndex = 0; blockIndex < block + skipBlockNum; blockIndex++)
    {
        if(FQspiPs_Nand_IsBlockBad(qspiPtr, blockIndex) != FMSH_SUCCESS)
        {
            skipBlockNum++;
        }
    }
    return skipBlockNum;
}

/*****************************************************************************/
/**
*
* This function writes to the flash skip bad block.
*
* @param	nfcPtr is a pointer to the FNfcPsu instance.
* @param	destAddr is the starting offset of flash to write.
* @param	byteCount is the number of bytes to write.
* @param	srcPtr is the source data buffer to write.
*
* @return
*		- FMSH_SUCCESS if successful.
*		- FMSH_FAILURE if failed.
*
* @note		None
*
******************************************************************************/
int FQspiPs_Nand_SkipWrite(FQspiPs_T* qspiPtr, u64 address, u32 byteCount, u8* sendBuffer)
{
    int status = FMSH_FAILURE;
    
    u32 blockSize;
    u32 block;
    u32 spareSize;
    u32 pageSize;
    u32 page;
    u32 offset;
    u32 skipBlockNum = 0;
    u32 writeLen = 0;
    u8 buf[FQSPIPS_NAND_MAXPAGESIZE + FQSPIPS_NAND_MAXSPARESIZE];
    u8* srcBufPtr = (u8*)sendBuffer;
    u64 length = byteCount;
    FQspiPs_Nand_Device* dev = (FQspiPs_Nand_Device*)qspiPtr->dev;
 
    FMSH_ASSERT(qspiPtr != NULL);
    FMSH_ASSERT(srcBufPtr != NULL);
    FMSH_ASSERT(length != 0);
    
    spareSize = dev->spareSize;
    pageSize = dev->pageSize;
    blockSize = dev->blockSize;
    /* skip bad block before dest address */
    skipBlockNum = FQspiPs_Nand_CalculateBadBlockNum(qspiPtr, address / blockSize);
    address = address + skipBlockNum * blockSize;
    
	/* Check if the actual length exceeds flash size */
	status = FQspiPs_Nand_CalculateLength(qspiPtr, address, length);
	if (status != FMSH_SUCCESS) 
    {
		return FMSH_FAILURE;
	}
    
	while (length > 0) 
    {
		block = address / blockSize;
		/* Check if the block is bad */
		if(FQspiPs_Nand_IsBlockBad(qspiPtr, block) != FMSH_SUCCESS)
        {
			/* Move to next block */
			address += blockSize;
			continue;
		}
        
        /* Calculate Page and Column address values */
        page = (u32)((address & (blockSize - 1)) / pageSize);
        offset = (u32)(address & (pageSize - 1));
        
        writeLen = pageSize - offset;
        if(writeLen > length)
        {
            writeLen = length;
        }
        
        memset(buf, 0xff, pageSize + spareSize);
        memcpy(buf + offset, srcBufPtr, writeLen);
        
		/* write to the NAND flash */
		status = FQspiPs_Nand_ProgramPage(qspiPtr, block, page, 0, buf);
		if (status != FMSH_SUCCESS) 
        {
            return FMSH_FAILURE;
		}     
        
		length -= writeLen;
		address += writeLen;
		srcBufPtr += writeLen;
	}
    
	return FMSH_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function reads from the flash.
*
* @param	nfcPtr is a pointer to the FNfcPsu instance.
* @param	srcAddr is the starting offset of flash to read.
* @param	byteCount is the number of bytes to read.
* @param	destPtr is the destination data buffer to fill in.
*
* @return
*		- FMSH_SUCCESS if successful.
*		- FMSH_FAILURE if failed.
*
* @note		None
*
******************************************************************************/
int FQspiPs_Nand_SkipRead(FQspiPs_T* qspiPtr, u64 address, u32 byteCount, u8* recvBuffer)
{
    int status = FMSH_FAILURE;
    
    u32 blockSize;
    u32 block;
    u32 pageSize;
    u32 page;
    u32 offset;
    u32 skipBlockNum = 0;
    u32 readLen = 0;
    u8 buf[FQSPIPS_NAND_MAXPAGESIZE + FQSPIPS_NAND_MAXSPARESIZE];
    u8* destBufPtr = recvBuffer;
    u64 length = byteCount;
    FQspiPs_Nand_Device* dev = (FQspiPs_Nand_Device*)qspiPtr->dev;
 
    FMSH_ASSERT(qspiPtr != NULL);
    FMSH_ASSERT(destBufPtr != NULL);
    FMSH_ASSERT(length != 0);
    
    pageSize = dev->pageSize;
    blockSize = dev->blockSize;
    /* skip bad block before dest address */
    skipBlockNum = FQspiPs_Nand_CalculateBadBlockNum(qspiPtr, address / blockSize);
    address = address + skipBlockNum * blockSize;
    
	/* Check if the actual length exceeds flash size */
	status = FQspiPs_Nand_CalculateLength(qspiPtr, address, length);
	if (status != FMSH_SUCCESS) 
    {
		return FMSH_FAILURE;
	}
    
	while (length > 0) 
    {
		block = address / blockSize;
		/* Check if the block is bad */
		if(FQspiPs_Nand_IsBlockBad(qspiPtr, block) != FMSH_SUCCESS)
        {
			/* Move to next block */
			address += blockSize;
			continue;
		}
        
        /* Calculate Page and Column address values */
        page = (u32)((address & (blockSize - 1)) / pageSize);
        offset = (u32)(address & (pageSize - 1));

        readLen = pageSize - offset;
        if(readLen > length)
        {
            readLen = length;
        }
        
		/* read from the NAND flash */
		status = FQspiPs_Nand_ReadPage(qspiPtr, block, page, 0, buf);
		if (status != FMSH_SUCCESS) 
        {
            return FMSH_FAILURE;
		}          
        
        memcpy(destBufPtr, buf + offset, readLen);
        
		length -= readLen;
		address += readLen;
		destBufPtr += readLen;
	}
    
	return FMSH_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function erases the flash.
*
* @param	nfcPtr is a pointer to the FNfcPsu instance.
* @param	destAddr is the starting offset of flash to erase.
* @param	byteCount is the number of bytes to erase.
*
* @return
*		- FMSH_SUCCESS if successful.
*		- FMSH_FAILURE if failed.
*
* @note
*		The Offset and Length should be aligned to block size boundary
*		to get better results.
*
******************************************************************************/
int FQspiPsu_Nand_SkipErase(FQspiPs_T* qspiPtr, u64 offset, u32 byteCount, u32 blockSize)
{
    int status;
    
	u32 block;
    u16 skipBlockNum = 0;
    u64 address = offset;
    u64 length = byteCount;
    FQspiPs_Nand_Device* dev = (FQspiPs_Nand_Device*)qspiPtr->dev;
    
    FMSH_ASSERT(qspiPtr != NULL);
    FMSH_ASSERT(length != 0);
    
    blockSize = dev->blockSize;
    skipBlockNum = FQspiPs_Nand_CalculateBadBlockNum(qspiPtr, address / blockSize);
    address += skipBlockNum * blockSize;
    
    /* Check if the actual length exceeds flash size */
	status = FQspiPs_Nand_CalculateLength(qspiPtr, address, length);
	if (status != FMSH_SUCCESS) 
    {
		return FMSH_FAILURE;
	}    
    
    while (length > 0) 
    {
		block = address / blockSize;
        /* Check if the block is bad */
        if (FQspiPs_Nand_IsBlockBad(qspiPtr, block) != FMSH_SUCCESS) 
        {
            address += blockSize;
            continue;
        }
        
        /* Erase the Nand flash block */
        status = FQspiPs_Nand_EraseBlock(qspiPtr, block);
        if (status != FMSH_SUCCESS) 
        {
            return FMSH_FAILURE;
        }
        
        address += blockSize;
        length -= blockSize;
    }

	return FMSH_SUCCESS;
}
