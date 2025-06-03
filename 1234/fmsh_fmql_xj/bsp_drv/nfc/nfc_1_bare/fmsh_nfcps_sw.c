#include <string.h>
#include "fmsh_nfcps_lib.h"

static int FNfcPs_IdCheck(FNfcPs_T* nfc);

int FNfcPs_Initialize(FNfcPs_T* nfc, u16 deviceId) 
{
  FNfcPs_Config_T* cfgPtr;
  
  FMSH_ASSERT(nfc != NULL);
  
  cfgPtr = FNfcPs_LookupConfig(deviceId);
  if (cfgPtr == NULL){
    return FMSH_FAILURE;
  }  
  
  return FNfcPs_CfgInitialize(nfc, cfgPtr);  
}

/***********************************
* Write CONTROL
******************/
int FNfcPs_InitController(FNfcPs_T* nfc, u32 ctrl)
{
  int err;
  
  FMSH_ASSERT(nfc != NULL);
  
  FNfcPs_SetStatusMask(nfc, 0x0140);
  
  /* Initialize Device */
  err = Nandflash_GetFlashInfo(nfc);
  if(err)
  {
    return FMSH_FAILURE;
  }
  
  err = FNfcPs_IdCheck(nfc);
  if(err)
  {
    return FMSH_FAILURE;
  }
  
  /* set ctrl */
  FNfcPs_SetCtrl(nfc, ctrl & NFCPS_USER_CTRL_MASK);
  err = FNfcPs_SetIOWidth(nfc, nfc->config.flashWidth);
  if(err)
  {
    return FMSH_FAILURE;
  }
  
  err = FNfcPs_SetBlockSize(nfc, NFCPS_BLOCKSIZE(nfc->model.PagePerBlock));
  if(err)
  {
    return FMSH_FAILURE;
  }    
  err = FNfcPs_SetEccBlockSize(nfc, NFCPS_ECCBLOCKSIZE(nfc->model.PageSizeInBytes >> 2));     
  if(err)
  {
    return FMSH_FAILURE;
  }
  
  ctrl = FNfcPs_GetCtrl(nfc);    
  
  /* set interrupt */
  if(ctrl & NFCPS_INT_EN_MASK)
  {
    FNfcPs_SetIntrMask(nfc, ~NFCPS_INTR_ALL);
  }
  
  /* set ecc */
  if(ctrl & NFCPS_ECC_EN_MASK)
  {        
    if(nfc->eccMode != NFCPS_ECC_HW)
    {
      FNfcPs_DisableEcc(nfc);
    }
  }
  FNfcPs_EccMode(nfc, nfc->eccMode);
  
  /* set bbm */
  FNfcPs_InitBadBlockTableDesc(nfc);
  err = FNfcPs_ScanBadBlockTable(nfc);
  if(err)
  {
    return FMSH_FAILURE;
  }
  
  if(ctrl & NFCPS_BBM_EN_MASK)
  {
    FNfcPs_SetDevPtr(nfc, (u32)nfc->bbMap);
    FNfcPs_SetDevSize(nfc, (NAND_MAX_BADBLKS >> 3));
    FNfcPs_InitRemap(nfc); 
  }
  
  return FMSH_SUCCESS;
}

static int FNfcPs_IdCheck(FNfcPs_T* nfc)
{
  u8 maker = nfc->model.Manufacture;
  u8 ioWidth = nfc->model.nfDataWidth;
  
  /* validity check */
  if ((maker != NAND_MICRON_ID) && (maker != NAND_SPANSION_ID) &&
      (maker != NAND_FMSH_ID))
  {
    return FMSH_FAILURE;
  } 
  if ((ioWidth != 8) && (ioWidth != 16)) 
  {
    return FMSH_FAILURE;
  }
  
  return FMSH_SUCCESS;
}

int FNfcPs_EccMode(FNfcPs_T* nfc, u32 eccMode)
{
  int err;
  u32 feature;
  u32 offset;
  
  FNfcPs_DisableEcc(nfc);
  
  switch(eccMode)
  {
  case NFCPS_ECC_NONE:
    break;
  case NFCPS_ECC_ONDIE:
    feature = Nandflash_GetFeature(nfc, 0x90);
    if((feature & 0x08) != 0x08)
    { 
      err = Nandflash_SetFeature(nfc, 0x90, 0x08);
      if(err)
      {
        return FMSH_FAILURE;
      }
    }
    break;
  case NFCPS_ECC_HW:
    FNfcPs_EnableEcc(nfc);
    offset = FNfcPs_GetPageSizeInBytes(nfc->model) + NFCPS_ECC_HW_OFFSET;
  
    if(nfc->model.nfDataWidth == 8)
    {
      FNfcPs_SetEccOffset(nfc, offset);
    } 
    else if(nfc->model.nfDataWidth == 16)
    {
      FNfcPs_SetEccOffset(nfc, offset >> 1);
    }
    else
    {
      return FMSH_FAILURE;
    }
	
    err = FNfcPs_SetEccCtrl(nfc, NFCPS_ECC_UNC_STATE, 2, NFCPS_ECC_CAP2_STATE);   
    if(err)
    {
      return FMSH_FAILURE;
    }
    break;
  case NFCPS_ECC_SW:
    /* not implemented */
    break;
  default: 
    break;
  }
  
  return FMSH_SUCCESS;
}

int FNfcPs_EraseBlock(FNfcPs_T* nfc, u64 destAddr, u32 blockCount)
{
  int err;
  u64 deviceSize;
  u16 block;
  u32 byteCount;
  
  FMSH_ASSERT(nfc != NULL);
  deviceSize = FNfcPs_GetDeviceSizeInBytes(nfc->model);
  byteCount = blockCount * FNfcPs_GetBlockSizeInBytes(nfc->model);
  
  /* validity check */
  if(blockCount == 0)
  {
    return FMSH_FAILURE;
  }
  
  if(destAddr + byteCount > deviceSize)
  {
    return FMSH_FAILURE;
  }
  
  /* check write protect status */
  if(FNfcPs_GetHwProt(nfc) == 0)
  {
    return FMSH_FAILURE;
  }
  
  /* calculate address */
  FNfcPs_TranslateFlashAddress(nfc->model, destAddr, byteCount, 
                               &block, 0, 0);
  
  /* Remaining blocks to erase */
  while( blockCount > 0)
  {
    err = Nandflash_EraseBlock(nfc, block);
    if(err)
    {
      return FMSH_FAILURE;
    }
    block++;
    blockCount--;
  }
  
  return FMSH_SUCCESS;
}

int FNfcPs_Write(FNfcPs_T *nfc, u64 destAddr, u32 byteCount, u8 *srcPtr, u8 *userSparePtr)
{
  int err;
  u64 deviceSize;
  u16 block, page, offset;
  u32 length;
  u8 data[NAND_MAX_PAGE_DATASIZE];
  u8 spare[NAND_MAX_PAGE_SPARESIZE];
  
  FMSH_ASSERT(nfc != NULL);
  FMSH_ASSERT(srcPtr != NULL);
  
  /* validity check */
  deviceSize = FNfcPs_GetDeviceSizeInBytes(nfc->model);
  if(byteCount == 0)
  {
    return FMSH_FAILURE;
  }
  if(destAddr + byteCount > deviceSize)
  {
    return FMSH_FAILURE;
  }
  /* check write protect status */
  if(FNfcPs_GetHwProt(nfc) == 0)
  {
    return FMSH_FAILURE;
  }
  
  /* copy spare buffer */
  if(userSparePtr)
  {
    memcpy(&spare[0], userSparePtr, nfc->model.SpareSizeInBytes);
  } 
  else
  {
    memset(&spare[0], 0xff, NAND_MAX_PAGE_SPARESIZE);
  }
  
  while(byteCount > 0)
  {
    /* calculate address */
    FNfcPs_TranslateFlashAddress(nfc->model, destAddr, byteCount, 
                                 &block, &page, &offset);
    /* length of bytes to be send to nandflash */
    length = nfc->model.PageSizeInBytes - offset;
    if(byteCount < length)
    {
      length = byteCount;
    }
    /* init & fill buffer */
    memset(&data[0], 0xff, NAND_MAX_PAGE_DATASIZE);
    memcpy(&data[offset], srcPtr, length);
    srcPtr += length;
    /* write to nandflash */
    err = Nandflash_WritePage(nfc, block, page, 0, data, spare);
    if(err)
    {
      return FMSH_FAILURE;
    }
    /* check has more data in buffer to write */
    if(byteCount > length)
    {
      destAddr += length;
      byteCount -= length;
    } 
    else
    {
      byteCount = 0;
    }
  }
  
  return FMSH_SUCCESS;
}

int FNfcPs_WriteSpareData(FNfcPs_T *nfc, u32 block, u32 page, u8 *userSparePtr)
{
  int err;
  u8 spare[NAND_MAX_PAGE_SPARESIZE];
  
  FMSH_ASSERT(nfc != NULL);
  FMSH_ASSERT(userSparePtr != NULL);
  
  /* validity check */
  if(block >= FNfcPs_GetDeviceSizeInBlocks(nfc->model))
  {
    return FMSH_FAILURE;
  }
  if(page >= FNfcPs_GetBlockSizeInPages(nfc->model))
  {
    return FMSH_FAILURE;
  }
  /* check write protect status */
  if(FNfcPs_GetHwProt(nfc) == 0)
  {
    return FMSH_FAILURE;
  }
  
  /* copy spare buffer */
  memcpy(&spare[0], userSparePtr, nfc->model.SpareSizeInBytes);
  /* write to nandflash */
  err = Nandflash_WritePage(nfc, block, page, 0, 0, spare);
  if(err)
  {
    return FMSH_FAILURE;
  }
  
  return FMSH_SUCCESS;
}

int FNfcPs_Read(FNfcPs_T* nfc, u64 srcAddr, u32 byteCount, u8* destPtr, u8* userSparePtr)
{
  int err;
  u64 deviceSize;
  u16 block, page, offset;
  u32 length;
  u8 data[NAND_MAX_PAGE_DATASIZE];
  u8 spare[NAND_MAX_PAGE_SPARESIZE];
  
  FMSH_ASSERT(nfc != NULL);
  FMSH_ASSERT(destPtr != NULL);
  
  /* validity check */
  deviceSize = FNfcPs_GetDeviceSizeInBytes(nfc->model);
  if(byteCount == 0)
  {
    return FMSH_FAILURE;
  }
  if(srcAddr + byteCount > deviceSize)
  {
    return FMSH_FAILURE;
  }
  
  /* remaining data to be read */
  while(byteCount > 0)
  {
    /* calculate address */
    FNfcPs_TranslateFlashAddress(nfc->model, srcAddr, byteCount, 
                                 &block, &page, &offset);
	
    /* read entire page from nandflash */
    err = Nandflash_ReadPage(nfc, block, page, 0, data, spare);
    if(err){
      return FMSH_FAILURE;
    }
	
    /* maxium length of bytes needed */
    length = nfc->model.PageSizeInBytes - offset;
	
    /* check has more data to read */
    if(byteCount > length)
    {
      memcpy(destPtr, &data[offset], length);
      destPtr += length;
      srcAddr += length;
      byteCount -= length;
    } 
    else
    {
      memcpy(destPtr, &data[offset], byteCount);
      byteCount = 0;
    }
  }
  
  if(userSparePtr){
    memcpy(userSparePtr, &spare[0], nfc->model.SpareSizeInBytes);
  }
  
  return FMSH_SUCCESS;
}

int FNfcPs_ReadSpareData(FNfcPs_T* nfc, u32 block, u32 page, u8* userSparePtr)
{
  int err;
  u8 spare[NAND_MAX_PAGE_SPARESIZE];
  
  FMSH_ASSERT(nfc != NULL);
  FMSH_ASSERT(userSparePtr != NULL);
  
  /* validity check */
  if(block >= FNfcPs_GetDeviceSizeInBlocks(nfc->model))
  {
    return FMSH_FAILURE;
  }
  if(page >= FNfcPs_GetBlockSizeInPages(nfc->model))
  {
    return FMSH_FAILURE;
  }
  
  err = Nandflash_ReadPage(nfc, block, page, 0, 0, spare);
  if(err)
  {
    return FMSH_FAILURE;
  }
  memcpy(userSparePtr, &spare[0], nfc->model.SpareSizeInBytes);
  
  return FMSH_SUCCESS;
}

/*******************************************************************/


#if 1

static FNfcPs_T g_nfc = {0};
FNfcPs_T * pNfc1 = (FNfcPs_T *)(&g_nfc);

int g_init_flag_nfc = 0;


int nfc_init(void)
{
	int err = 0;
		
	ps_init_2();
	
    err = FNfcPs_Initialize(pNfc1, FPS_NFC_S_DEVICE_ID);
    if(err) 
	{
        return FMSH_FAILURE;
    }
	
	/*init nfc*/
    FNfcPs_Reset(pNfc1);
	
    pNfc1->eccMode = NFCPS_ECC_NONE;
	
    FNfcPs_SetStatusMask(pNfc1, 0x0140);
    FNfcPs_SetTiming(pNfc1);
	
    err = FNfcPs_InitController(pNfc1, (NFCPS_AUTO_READ_STAT_EN_MASK 
                                        | NFCPS_READ_STATUS_EN_MASK));
    if (err) 
	{
        return FMSH_FAILURE;
    }

	return FMSH_SUCCESS;
}


int vxInit_Nfc0(void)
{
	return nfc_init();
}



/*
nand_id: 0x1590AA2C-00000006 
--------
Manufacture: 0x2C 
DeviceId: 0xAA 
PageSizeInBytes: 2048 bytes 
SpareSizeInBytes: 64 bytes 
BlockSizeInKBytes: 128 KB 
nfDataWidth: 8 bit_busWidth 

PagePerBlock: 64 
BlockNum: 2048 
PageNum: 131072 
DeviceSizeInMegaBytes: 256 MB 

==============================
MT29F1G16A:

nand_id: 0x5580B12C-00000004 
--------
Manufacture: 0x2C 
DeviceId: 0xB1 
PageSizeInBytes: 2048 bytes 
SpareSizeInBytes: 64 bytes 
BlockSizeInKBytes: 128 KB 
nfDataWidth: 16 bit_busWidth 

PagePerBlock: 64 
BlockNum: 2048 
PageNum: 131072 
DeviceSizeInMegaBytes: 256 MB 
*/

int nfc_id(void)
{
	int err = 0;
    u32 first_word, second_word;
    u8 Id1, Id2, Id3, Id4, Id5;
    FNfcPs_Model_T model;
	u8 maker = 0;

#if 0
	/*extern int ps_init_2(void);*/
	ps_init_2();
	
    /*pNfc1->config.baseAddress = FPS_NFC_S_BASEADDR;*/
	
    /* init nfc*/
    err = FNfcPs_Initialize(pNfc1, FPS_NFC_S_DEVICE_ID);
    if(err) 
	{
        return FMSH_FAILURE;
    }
	/*taskDelay(10);*/
	
    /* reset nfc_ctrl*/
    FNfcPs_Reset(pNfc1);
		
    /* set nfc_ctrl time_seq*/
	FNfcPs_SetTiming(pNfc1);
	
	FNfcPs_SetStatusMask(pNfc1, 0x0140);
	
    /* Reset Nandflash chip*/
	err = Nandflash_Reset(pNfc1);
    if(err){
        return FMSH_FAILURE;
    }
	
#else	

	/*
	if (0 == g_init_flag_nfc)
	{
		nfc_init();
		g_init_flag_nfc = 1;

		delay_ms(100);
	}
	*/
#endif
	
	/* read id*/
    err = Nandflash_ReadId(pNfc1, &first_word, &second_word);
    if(err){
        return FMSH_FAILURE;
    }

	printf("nand_id(MT29F1G16A 0x5580B12C-00000004): 0x%08X-%08X \n", first_word, second_word);
	
    /* calculate ID*/
    Id1 = first_word & 0xff;
    Id2 = (first_word >> 8) & 0xff;
    Id3 = (first_word >> 16) & 0xff;
    Id4 = (first_word >> 24) & 0xff;
    Id5 = second_word & 0xff;
    
    /*Set NandflashModel*/
    model.Manufacture = Id1;
    model.DeviceId = Id2;
    model.Options = 0;

	maker = model.Manufacture;
	
	printf("Manufacture: 0x%X ", maker);
	switch (maker)
	{
	case NAND_MICRON_ID:
		printf("[MICRON] \n");
		break;
	case NAND_SPANSION_ID:
		printf("[SPANSION] \n");
		break;
	case NAND_FMSH_ID:
		printf("[FMSH] \n");
		break;
	}
		
	printf("DeviceId: 0x%X \n", model.DeviceId);
	
    /* Fetch from the extended ID4
    * ID4 D5  D4 BlockSize || D1  D0  PageSize
    *     0   0   64K      || 0   0   1K
    *     0   1   128K     || 0   1   2K
    *     1   0   256K     || 1   0   4K
    *     1   1   512K     || 1   1   8k */
    
    /* Page Size*/
    switch(Id4 & 0x03) 
	{
    case 0x00: 
		model.PageSizeInBytes = 1024; 
		break;
    case 0x01: 
		model.PageSizeInBytes = 2048; 
		break;
    case 0x02: 
		model.PageSizeInBytes = 4096; 
		break;
    case 0x03: 
		model.PageSizeInBytes = 8192; 
		break;
    }
	printf("PageSizeInBytes  : %d bytes \n", model.PageSizeInBytes);
	
    /* Spare Size*/
    switch(Id4 & 0x04)
	{
    case 0x00: 
		model.SpareSizeInBytes = 32; 
		break;
    case 0x04: 
		model.SpareSizeInBytes = 64; 
		break;
    }
	printf("SpareSizeInBytes : %d bytes \n", model.SpareSizeInBytes);
	
    /* Block Size*/
    switch(Id4 & 0x30) 
	{
    case 0x00: 
		model.BlockSizeInKBytes = 64;  
		break;
    case 0x10: 
		model.BlockSizeInKBytes = 128; 
		break;
    case 0x20: 
		model.BlockSizeInKBytes = 256; 
		break;
    case 0x30: 
		model.BlockSizeInKBytes = 512; 
		break;
    }
	printf("BlockSizeInKBytes: %d KB \n", model.BlockSizeInKBytes);
	
    /* IO width*/
    switch(Id4 & 0x40)
	{
    case 0x00: 
		model.nfDataWidth = 8;  
		break;
    case 0x40: 
		model.nfDataWidth = 16; 
		break;
    }
	printf("nfDataWidth: %d bit_busWidth \n\n", model.nfDataWidth);
	
    /* calculate other parameters*/
    model.BlockPerLun = NAND_MAX_BLOCKS;
    model.LunNum = 0x1;
    model.PagePerBlock = (u16)(((u32)model.BlockSizeInKBytes << 10) / model.PageSizeInBytes);
    model.BlockNum =  model.BlockPerLun * model.LunNum;
    model.PageNum = model.PagePerBlock * model.BlockPerLun * model.LunNum;
    model.DeviceSizeInMegaBytes = (u16)((model.BlockSizeInKBytes * model.BlockPerLun * model.LunNum) >> 10);
	
	printf("PagePerBlock: %d \n", model.PagePerBlock);
	printf("BlockNum    : %d \n", model.BlockNum);
	printf("PageNum     : %d \n", model.PageNum);
	printf("DeviceSizeInMegaBytes: %d MB \n", model.DeviceSizeInMegaBytes);

	printf("\n");
	
	return FMSH_SUCCESS;
}


void nfc_onfi_id(void)
{
	u32 onfi_id = 0;
	
	if (0 == g_init_flag_nfc)
	{
		/*nfc_init();*/
		/*g_init_flag_nfc = 1;*/
	}
	
	onfi_id =  Nandflash_ReadOnfiId(pNfc1);

	printf("onfi_id: 0x%08X \n", onfi_id);
	return;
}



u8 SndBuffer[2048];
u8 RcvBuffer[2048];
/*void test_rd_nfc(int offset)  // address*/
void test_rd_nfc(int page_idx)
{
	int block_size = 0;
	int block_idx = 0;
	int page_idx = 0;
	int i = 0;
	int page_size = 0;
	u64 srcAddr =0;
	
	int offset = 0;
	

	if (0 == g_init_flag_nfc)
	{
		nfc_init();
		g_init_flag_nfc = 1;
	}
	
	page_size = FNfcPs_GetPageSizeInBytes(pNfc1->model);
	block_size = FNfcPs_GetBlockSizeInBytes(pNfc1->model);

	offset = page_idx * page_size;
	
	block_idx = offset / block_size;
	page_idx = offset / page_size;
	
	srcAddr = (u64)((u64)block_size*block_idx + (u64)page_size*page_idx);
	
    /* read data*/
    if (FMSH_SUCCESS != FNfcPs_Read(pNfc1, srcAddr, page_size, RcvBuffer, 0))
	{
		printf("FNfcPs_Read fail!srcAddr:0x%X \n", srcAddr);
		return;
	}
	
	printf("\nblock_size:%d page_size:%d \n", block_size, page_size);

	for (i=0; i<256; i++)
	{
		printf("%02X", RcvBuffer[i]);
		if ((i+1)%4 == 0)
		{
			printf(" ");
		}
		if ((i+1)%16 == 0)
		{
			printf("\n");
		}
	}
	
	printf(" ... ... ... ... \n");
	
	for (i=(page_size-256); i<page_size; i++)
	{
		printf("%02X", RcvBuffer[i]);
		if ((i+1)%4 == 0)
		{
			printf(" ");
		}
		if ((i+1)%16 == 0)
		{
			printf("\n");
		}
	}
	

	return;
}

void test_erase_nfc(int offset)
{
	int block_size = 0;
	int block_idx = 0;
	int page_idx = 0;
	int i = 0;
	int page_size = 1024;
	u64 srcAddr =0;
	
	/*nfc_init();*/
	
	block_size = FNfcPs_GetBlockSizeInBytes(pNfc1->model);
	block_idx = offset / block_size;
	
	/* erase*/
	FNfcPs_EraseBlock(pNfc1, (u64)((u64)block_idx*block_size), 1); 
	return;
}

int g_test_base = 0;
/*void test_wr_nfc(int offset)*/
void test_wr_nfc(int page_idx)
{
	int block_size = 0;
	int block_idx = 0;
	int page_idx = 0;
	int i = 0;
	int page_size = 0;
	u64 srcAddr =0;
	int offset = 0;
	
	/*nfc_init();*/
	
	page_size = FNfcPs_GetPageSizeInBytes(pNfc1->model);
	block_size = FNfcPs_GetBlockSizeInBytes(pNfc1->model);
	
	offset = page_idx * page_size;
	
	block_idx = offset / block_size;
	
    for (i=0; i<page_size; i++)
    {
        SndBuffer[i] = g_test_base+i; 
        RcvBuffer[i] = 0;
    }
	g_test_base++;
	
    FNfcPs_Write(pNfc1, (u64)((u64)block_size*block_idx), page_size, SndBuffer, 0);
	return;
}
#endif



