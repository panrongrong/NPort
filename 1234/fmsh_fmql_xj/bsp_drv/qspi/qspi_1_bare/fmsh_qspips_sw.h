#ifndef _FMSH_QSPIPS_SW_H_	/* prevent circular inclusions */ 
#define _FMSH_QSPIPS_SW_H_	/* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif
    
/**********************************Include File*********************************/

/**********************************Constant Definition**************************/
    
/**********************************Type Definition******************************/

/**********************************Macro (inline function) Definition***********/

/**********************************Variable Definition**************************/
    
/**********************************Function Prototype***************************/
int FQspiPs_Initialize(FQspiPs_T* qspi, u16 deviceId);
int FQspiPs_GetFlashInfo(FQspiPs_T* qspi);
int FQspiPs_SetFlashMode(FQspiPs_T* qspi, u8 cmd);
int FQspiPs_ResetFlash(FQspiPs_T* qspi);
int FQspiPs_UnlockFlash(FQspiPs_T* qspi);
int FQspiPs_SetFlashSegment(FQspiPs_T* qspi, u8 highAddr);
int FQspiPs_EraseChip(FQspiPs_T* qspi);
int FQspiPs_EraseSectors(FQspiPs_T* qspi, u32 offset, u32 byteCount, u32 sectorSize);
int FQspiPs_SendBytes(FQspiPs_T* qspi, u32 offset, u32 byteCount, u8* sendBuffer);
int FQspiPs_RecvBytes(FQspiPs_T* qspi, u32 offset, u32 byteCount, u8* recvBuffer);
int FQspiPs_FastSendBytes(FQspiPs_T* qspi, u32 offset, u32 byteCount, u8* sendBuffer);
int FQspiPs_FastRecvBytes(FQspiPs_T* qspi, u32 offset, u32 byteCount, u8* recvBuffer);
int FQspiPs_EnterXIP(FQspiPs_T* qspi, u8 cmd);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* prevent circular inclusions */d