 
#ifndef _FMSH_NFCPS_SW_H_   /* prevent circular inclusions */
#define _FMSH_NFCPS_SW_H_

/**********************************Include File*********************************/

/**********************************Type Definition******************************/

/**********************************Macro (inline function) Definition***********/

/**********************************Function Prototype***************************/
int FNfcPs_Initialize(FNfcPs_T* nfc, u16 deviceId);
int FNfcPs_InitController(FNfcPs_T* nfc, u32 ctrl);
int FNfcPs_EccMode(FNfcPs_T* nfc, u32 eccMode);
int FNfcPs_EraseBlock(FNfcPs_T* nfc, u64 DestAddr, u32 BlockCount);
int FNfcPs_Write(FNfcPs_T* nfc, u64 destAddr, u32 byteCount, u8* srcPtr, u8* userSparePtr);
int FNfcPs_WriteSpareData(FNfcPs_T* nfc, u32 block, u32 page, u8* userSparePtr);
int FNfcPs_Read(FNfcPs_T* nfc, u64 srcAddr, u32 byteCount, u8* destPtr, u8* userSparePtr);
int FNfcPs_ReadSpareData(FNfcPs_T* nfc, u32 block, u32 page, u8* userSparePtr);

int FNfcPs_DMAWrite(FNfcPs_T* nfc, int dmaMode, u32 byteCount, u64 nandAddr, u32 srcAddr);
int FNfcPs_DMARead(FNfcPs_T* nfc, int dmaMode, u32 byteCount, u64 nandAddr, u32 srcAddr);

/**********************************Constant Definition**************************/

/**********************************Variable Definition**************************/



#endif	/* prevent circular inclusions */r