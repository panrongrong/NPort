 
#include "fmsh_nfcps_lib.h"

static u32 SkipBlockNandflash_CalculateLength(FNfcPs_T* nfc, u64 Offset, u32 Length);

/*****************************************************************/
int SkipBlockNandflash_EraseBlock(FNfcPs_T* nfc, u64 offset, u32 byteCount) 
{
    int error;
    
    u32 blockSize;
	u32 startBlock;
	u32 numOfBlocks;
	u32 block;
    
    blockSize = FNfcPs_GetBlockSizeInBytes(nfc->model);
	startBlock = (offset & ~(blockSize - 1))/blockSize;
	numOfBlocks = (byteCount & ~(blockSize - 1))/blockSize;
    
	for (block = startBlock; block < (startBlock + numOfBlocks); block++) 
    {
        /* Check if the block is bad */
		error = FNfcPs_CheckBlock(nfc, block);
		if (error == 1) 
        {
			numOfBlocks++;
			/* Increase the block count for skip block method */
			continue;
		}
        
		/* Erase the Nand flash block */
		error = Nandflash_EraseBlock(nfc, block);
		if (error != 0) 
        {
			/* Erase operation error */
			return FMSH_FAILURE;
		}
	}
    
	return FMSH_SUCCESS;
}

int SkipBlockNandflash_Read(FNfcPs_T* nfc, u64 srcAddr, u32 byteCount, u8* destPtr)
{    
    int error;
    
    u64 deviceSize;
    u32 blockSize;
    u32 blockOffset;
    u32 block;
    u32 bytesLeft;
    u32 actLen;
    u32 readLen;
	u32 blockReadLen;
	u8 *bufPtr = (u8 *)destPtr;
    
    /* Calculate the actual length including bad blocks */
	actLen = SkipBlockNandflash_CalculateLength(nfc, srcAddr, byteCount);
    deviceSize = FNfcPs_GetDeviceSizeInBytes(nfc->model); 
    /* Check if the actual length cross flash size */
	if (srcAddr + actLen > deviceSize) 
    {
		return FMSH_FAILURE;
	}
    
    bytesLeft = byteCount;
    blockSize = FNfcPs_GetBlockSizeInBytes(nfc->model);
	while (bytesLeft > 0) {
        /* bytes remaind in this block*/
		blockOffset = srcAddr & (blockSize - 1);
        blockReadLen = blockSize - blockOffset;
		block = (srcAddr & ~(blockSize - 1))/blockSize;
        /* Check if the block is bad */
		error = FNfcPs_CheckBlock(nfc, block);
		if (error == 1) 
        {
			/* Move to next block */
			srcAddr += blockSize;
			continue;
		}
        /* Check if we cross block boundary */
		if (bytesLeft < blockReadLen) 
        {
			readLen = bytesLeft;
		} 
        else 
        {
			readLen = blockReadLen;
		}
        /* Read from the NAND flash */
        error = FNfcPs_Read(nfc, srcAddr, readLen, bufPtr, NULL);
		if (error != 0) 
        {
			return FMSH_FAILURE;
		}
        
		bytesLeft -= readLen;
		srcAddr += readLen;
		bufPtr += readLen;
	}
    
	return FMSH_SUCCESS;
}

int SkipBlockNandflash_Write(FNfcPs_T* nfc, u64 destAddr, u32 byteCount, u8* srcPtr)
{
    int error;
    
    u64 deviceSize;
    u32 blockSize;
    u32 blockOffset;
    u32 block;
    u32 bytesLeft; 
	u32 actLen; /* bytes including bad blcok*/
    u32 writeLen; /* bytes write to this block*/
    u32 blockWriteLen; /* bytes remaining empty in this block*/
	u8 *bufPtr = (u8 *)srcPtr;
    
	/* Calculate the actual length including bad blocks */
	actLen = SkipBlockNandflash_CalculateLength(nfc, destAddr, byteCount);
    deviceSize = FNfcPs_GetDeviceSizeInBytes(nfc->model); 
	/* Check if the actual length cross flash size */
	if (destAddr + actLen > deviceSize) 
    {
		return FMSH_FAILURE;
	}
    
    bytesLeft = byteCount;
    blockSize = FNfcPs_GetBlockSizeInBytes(nfc->model);
	while (bytesLeft > 0) 
    {
        /* bytes remaind in this block*/
		blockOffset = destAddr & (blockSize - 1);
        blockWriteLen = blockSize - blockOffset;
		block = (destAddr & ~(blockSize - 1))/blockSize;
		/* Check if the block is bad */
		error = FNfcPs_CheckBlock(nfc, block);
		if (error == 1) 
        {
			/* Move to next block */
			destAddr += blockSize;
			continue;
		}
        
		/*  Check if we cross block boundary */
		if (bytesLeft < blockWriteLen) 
        {
			writeLen = bytesLeft;
		} 
        else 
        {
			writeLen = blockWriteLen;
		}
        
		/* write to the NAND flash */
		error = FNfcPs_Write(nfc, destAddr, writeLen, bufPtr, NULL);
		if (error != 0) 
        {
			return FMSH_FAILURE;
		}
        
		bytesLeft -= writeLen;
		destAddr += writeLen;
		bufPtr += writeLen;
	}
    
	return FMSH_SUCCESS;
}

/******************************************************************************
* This function returns the length of bytes including bad blocks from a given offset and
* length.
* @offset: address of start point
* @length: byte number
***********************************/
static u32 SkipBlockNandflash_CalculateLength(FNfcPs_T* nfc, u64 offset, u32 length)
{
    int error;
    u64 deviceSize;
	u32 blockSize;
	u32 curBlockLen;
	u32 curBlock;
	u32 validLen = 0;
	u32 actLen = 0;
    
    FMSH_ASSERT(nfc != NULL);
    
    deviceSize = FNfcPs_GetDeviceSizeInBytes(nfc->model);
    blockSize = FNfcPs_GetBlockSizeInBytes(nfc->model);
    
	while (validLen < length) 
    {
		curBlockLen = blockSize - (offset & (blockSize - 1));
		curBlock = (offset & ~(blockSize - 1)) / blockSize;
		/* Check if the block is bad */
		error = FNfcPs_CheckBlock(nfc, curBlock);
		if (error == 0) 
        {
            /* good block */
			validLen += curBlockLen;
		}
		actLen += curBlockLen;
		offset += curBlockLen;
		if (offset >= deviceSize) 
        {
			break;
		}
	}
    
	return actLen;
}