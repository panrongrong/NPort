 
#ifndef _FMSH_NFCPS_SKIPBLOCK_H_   /* prevent circular inclusions */
#define _FMSH_NFCPS_SKIPBLOCK_H_

/**********************************Include File*********************************/

/**********************************Constant Definition**************************/

/**********************************Type Definition******************************/

/**********************************Macro (inline function) Definition***********/

/**********************************Variable Definition**************************/

/**********************************Function Prototype***************************/
int SkipBlockNandflash_EraseBlock(FNfcPs_T* nfc, u64 Offset, u32 ByteCount); 
int SkipBlockNandflash_Read(FNfcPs_T* nfc, u64 SrcAddr, u32 ByteCount, u8* DestPtr);
int SkipBlockNandflash_Write(FNfcPs_T* nfc, u64 DestAddr, u32 ByteCount, u8* SrcPtr);
             
#endif	/* prevent circular inclusions */c