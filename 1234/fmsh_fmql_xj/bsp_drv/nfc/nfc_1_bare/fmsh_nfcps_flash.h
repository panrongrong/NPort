#ifndef _FMSH_NFCPS_FLASH_H_ /* prevent circular inclusions */ 
#define _FMSH_NFCPS_FLASH_H_

#ifdef __cplusplus
extern "C" {
#endif
    
/**********************************Include File*********************************/
    
/**********************************Constant Definition**************************/
#define NANDFLASH_COMMAND_NUM (sizeof(s_CmdTbl_nfc) / sizeof(FNfcPs_Command_T))

#define NAND_SPANSION_ID                             0x01
#define NAND_MICRON_ID                               0x2C
#define NAND_FMSH_ID                                 0xA1
    
#define NANDFLASH_ONFI_ID                           (0x49464E4F) 

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
    
/**********************************Type Definition******************************/

/**********************************Macro (inline function) Definition***********/

/**********************************Variable Definition**************************/

/**********************************Function Prototype***************************/
int Nandflash_GetFlashInfo(FNfcPs_T* nfc);
int Nandflash_Reset(FNfcPs_T* nfc);
int Nandflash_ReadId(FNfcPs_T* nfc, u32* id1, u32* id2);
u32 Nandflash_ReadOnfiId(FNfcPs_T* nfc);
int Nandflash_ReadParaPage(FNfcPs_T* nfc, void* ParaPage);
u8 Nandflash_ReadStatus(FNfcPs_T* nfc);
u32 Nandflash_GetFeature(FNfcPs_T* nfc, u8 Feature);
int Nandflash_SetFeature(FNfcPs_T* nfc, u8 Feature, u32 Value);

int Nandflash_EraseBlock(FNfcPs_T* nfc, u32 Block);

int Nandflash_WritePage(FNfcPs_T* nfc, u16 Block, u16 Page, u16 Offset, void *Data, void *Spare);
int Nandflash_ProgramPage1(FNfcPs_T* nfc, u16 block, u16 page, u16 offset, signed int byte_count, void* send_buffer);
int Nandflash_ChangeWriteColumn(FNfcPs_T* nfc, u32 col_addr, signed int byte_count, void* send_buffer);
int Nandflash_ProgramPage(FNfcPs_T* nfc);

int Nandflash_ReadPage(FNfcPs_T* nfc, u16 Block, u16 Page, u16 Offset, void* Data, void* Spare);
int Nandflash_ChangeReadColumn(FNfcPs_T* nfc, u32 col_addr, signed int byte_count, void* recv_buffer);
int Nandflash_ReadPageCacheLast(FNfcPs_T* nfc);

int Nandflash_IntDataMove(FNfcPs_T* nfc, u32 src_row,  u32 dest_row);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* prevent circular inclusions */e