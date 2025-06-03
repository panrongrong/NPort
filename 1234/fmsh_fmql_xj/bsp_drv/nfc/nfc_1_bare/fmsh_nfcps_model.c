 
#include "fmsh_nfcps_lib.h"

int FNfcPs_TranslateFlashAddress(const FNfcPs_Model_T model, u64 Address, unsigned int Size, 
                                    u16 *Block, u16 *Page, u16 *Offset)
{
    u16 tempBlock, tempPage, tempOffset;
    u64 DeviceSizeInBytes;
    u32 BlockSizeInBytes;
    u16 PageSizeInBytes;
    
    DeviceSizeInBytes = FNfcPs_GetDeviceSizeInBytes(model);
    BlockSizeInBytes = FNfcPs_GetBlockSizeInBytes(model);
    PageSizeInBytes = FNfcPs_GetPageSizeInBytes(model);  
    
    if(Address + Size > DeviceSizeInBytes)
    {
        return FMSH_FAILURE;
    }

	/* block idx*/
    tempBlock = Address / BlockSizeInBytes;  

	/* page idx*/
    Address -= tempBlock * BlockSizeInBytes;
    tempPage = Address / PageSizeInBytes;

	/* offset in page*/
    Address -= tempPage*PageSizeInBytes;
    tempOffset = Address;
    
    if (Block) 
    {
        *Block = tempBlock;
    }
	
    if (Page)
    {
        *Page = tempPage;
    }
	
    if (Offset)
    {
        *Offset = tempOffset;
    } 
	
    return FMSH_SUCCESS;
}

u16 FNfcPs_GetDeviceSizeInBlocks(const FNfcPs_Model_T model)
{
    return ((u64)model.DeviceSizeInMegaBytes << 10) / model.BlockSizeInKBytes;
}

u32 FNfcPs_GetDeviceSizeInPages(const FNfcPs_Model_T model)
{
    return ((u64)model.DeviceSizeInMegaBytes << 20) / model.PageSizeInBytes;
}

u16 FNfcPs_GetDeviceSizeInMegaBytes(const FNfcPs_Model_T model)
{
    return model.DeviceSizeInMegaBytes;
}

u64 FNfcPs_GetDeviceSizeInBytes(const FNfcPs_Model_T model)
{
    return (u64)model.DeviceSizeInMegaBytes << 20;
}

u16 FNfcPs_GetBlockSizeInPages(const FNfcPs_Model_T model)
{
    return ((u32)model.BlockSizeInKBytes << 10) / model.PageSizeInBytes;
}

u16 FNfcPs_GetBlockSizeInKBytes(const FNfcPs_Model_T model)
{
    return model.BlockSizeInKBytes ;
}

u32 FNfcPs_GetBlockSizeInBytes(const FNfcPs_Model_T model)
{
    return (u32)model.BlockSizeInKBytes << 10;
}

u16 FNfcPs_GetPageSizeInBytes(const FNfcPs_Model_T model)
{
    return model.PageSizeInBytes;
}

u16 FNfcPs_GetPageSpareSize(const FNfcPs_Model_T model)
{
    if (model.SpareSizeInBytes) 
    {
        return model.SpareSizeInBytes;
    }
    else 
    {
        return (model.PageSizeInBytes >> 5); /* Spare size is 16/512 of data size */
    }
}

u8 FNfcPs_GetDeviceId(const FNfcPs_Model_T model)
{
    return model.DeviceId;
}

u8 FNfcPs_GetRowAddrCycle(const FNfcPs_Model_T model)
{
    return model.RowAddrCycle;
}

u8 FNfcPs_GetColAddrCycle(const FNfcPs_Model_T model)
{
    return model.ColAddrCycle;
}

u8 FNfcPs_GetDataBusWidth(const FNfcPs_Model_T model)
{
    return ((model.Options==16) || (model.nfDataWidth==16))? 16: 8;
}

u8 FNfcPs_HasSmallBlocks(const FNfcPs_Model_T model)
{
    return (model.PageSizeInBytes <= 512 )? 1: 0;
}

u8 FNfcPs_GetNumLun(const FNfcPs_Model_T model)
{
    return model.LunNum;
}
