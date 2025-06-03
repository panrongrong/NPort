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
* @file fmsh_qspips.h
* @addtogroup qspips_v1_1
* @{
*
* This header file contains the identifiers and driver
* functions (or macros) that can be used to access the device.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who Date     Changes
* ----- --- -------- -----------------------------------------------
* 1.00  hzq 7/1/19 
* 		     First release
* 1.10  hzq 11/26/20 
* 		     Add defination of driver status.
* 		     Add macron of configuration used for initialization.
*            Add defination of flash manufacture code.
*            Add type defination FQspiPs_Table which is not used now.
*            Add type defination FQspiPs_Ops which is a struct of
*            operations.
*            Add type defination FQspiPs_Caps which is a struct of 
*            configuration.     
*            Add type defination FQspiPs_Dma which is a struct of 
*            dma related parameters.  
*            Modify FQspiPs_T to increase members such as version, type.
*            Delete FqspiPs_SetFlashReadMode function prototype.
*            Add FqspiPs_Initialize function prototype.      
*            Add FqspiPs_InitHw function prototype.  
*            Add FqspiPs_GetFlashInfo function prototype.  
*            Add FqspiPs_SetFlashMode function prototype.  
*            Add FqspiPs_SendBytes function prototype.  
*            Add FqspiPs_RecvBytes function prototype.  
*            Add FqspiPs_FastSendBytes function prototype.  
*            Add FqspiPs_FastRecvBytes function prototype.  
*            Add FqspiPs_Erase function prototype.      
*            Add FqspiPs_Write function prototype.  
*            Add FqspiPs_Read function prototype.  
*            Add FqspiPs_ResetFlash function prototype.  
*            Add FqspiPs_EnableQuad function prototype.   
*            Add FqspiPs_Unlock function prototype. 
*            Add FqspiPs_ReadId function prototype.  
*                       
* </pre>
*
******************************************************************************/

#ifndef _FMSH_QSPIPS_H_	/* prevent circular inclusions */ 
#define _FMSH_QSPIPS_H_	/* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/  
    
/************************** Constant Definitions *****************************/   

#ifndef FPAR_QSPIPS_0_DEVICE_ID    
    #define FPS_QSPI0_DEVICE_ID          (0)   
#else
    #define FPS_QSPI0_DEVICE_ID          FPAR_QSPIPS_0_DEVICE_ID
#endif

#ifndef  FPS_QSPI0_BASEADDR  
    #define FPS_QSPI0_BASEADDR           (0xe0000000)
#endif
    
#ifndef  FPS_QSPI0_D_BASEADDR  
    #define FPS_QSPI0_D_BASEADDR         (0xe8000000)
#endif
    
#ifndef FPAR_QSPIPS_1_DEVICE_ID    
    #define FPS_QSPI1_DEVICE_ID          (1)        
#else
    #define FPS_QSPI1_DEVICE_ID          FPAR_QSPIPS_1_DEVICE_ID  
#endif

#ifndef  FPS_QSPI1_BASEADDR  
    #define FPS_QSPI1_BASEADDR           (0xe0020000)
#endif
    
#ifndef  FPS_QSPI1_D_BASEADDR  
    #define FPS_QSPI1_D_BASEADDR         (0xe9000000)
#endif
    
#define FPAR_QSPIPS_NUM_INSTANCES       (2) 

#define QSPI_BUSY                       (-1)
#define QSPI_TIMEOUT                    (-2)
#define QSPI_PARAM_ERR                  (-3)
#define QSPI_DETECT_FAIL                (-4)
#define QSPI_INIT_FAIL                  (-5)   
    
#define QSPI_CAPS(name) static FQspiPs_Caps qspi_##name##_caps
#define GET_QSPI_CAPS(name) &qspi_##name##_caps
    
#define READ                            (0)
#define FAST_READ                       (1)
#define DUAL_READ                       (2)
#define QUAD_READ                       (3)
#define DUAL_IO_READ                    (4)
#define QUAD_IO_READ                    (5)    

/* flash manufacturer code*/
#define SPI_SPANSION_ID                     (0x01)
#define SPI_MICRON_ID                       (0x20)
#define SPI_WINBOND_ID                      (0xEF)
#define SPI_MACRONIX_ID                     (0xC2)
#define SPI_ISSI_ID                         (0x9D)
#define SPI_FMSH_ID                         (0xA1)
#define SPI_GD_ID                           (0xC8)
#define SPI_UNKNOWN_ID                      (0xFF)  
    
/**************************** Type Definitions *******************************/  
    
struct FQspiPs_Tag;
struct FQspiPs_Param_Tag;

typedef void (*FQspiPs_StatusHandler)(void *callBackRef, 
                                     u32 statusEvent, u32 byteCount);

typedef struct {
    int (*Erase)(struct FQspiPs_Tag* qspiPtr, u64 offset, u32 byteCount, u32 blockSize);
    int (*Write)(struct FQspiPs_Tag* qspiPtr, u64 offset, u32 byteCount, u8* sendBuffer);
    int (*Read)(struct FQspiPs_Tag* qspiPtr, u64 offset, u32 byteCount, u8* recvBuffer);
    int (*EnableQuad)(struct FQspiPs_Tag* qspiPtr);
    int (*Lock)(struct FQspiPs_Tag* qspiPtr);
    int (*Unlock)(struct FQspiPs_Tag* qspiPtr);
    int (*Reset)(struct FQspiPs_Tag* qspiPtr);
} FQspiPs_Ops;

typedef struct {
    u8 hasIntr;
    u8 hasDma;
    u8 hasXIP;
    u32 txEmptyLvl;
    u32 rxFullLvl;
    u8 rdMode;  
    u32 baudRate; 
    u8 sampleDelay;
    u8 addressBytes;
} FQspiPs_Caps;

typedef struct {
    u8 burstType;
    u8 singleType;
    u8 txIf;
    u8 rxIf;
    u32 io; 
} FQspiPs_Dma;
    
typedef struct {
  u16 deviceId;		                /**< Unique ID  of device */
  u32 dataBaseAddress;              /**< Data Base address of the device */
  u32 baseAddress;                  /**< Config Base address of the device */
} FQspiPs_Config_T;

typedef struct FQspiPs_Tag {
    
    int version;  
    char* type;
    
    FQspiPs_Config_T config;          /**< Configuration structure */ 

    u32 flag;
    u8 isBusy;   
    
  	u8 maker;                         /**< Flash maker */
  	u32 devSize;                      /**< Flash device size in bytes */
  	u32 sectorSize;                   /**< Flash sector size in bytes */ 
  
    FQspiPs_Caps         caps;
    FQspiPs_Dma*         dma;
    void*                dev;
  
    u8 *dataBufferPtr;	            /**< Buffer */
    int remainingBytes;	            /**< Number of bytes left to transfer(state) */
  
    FQspiPs_StatusHandler statusHandler;
    void* statusRef;
  
    FQspiPs_Ops* ops;
    
} FQspiPs_T;
           
/***************** Macros (Inline Functions) Definitions *********************/  
        
/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

FQspiPs_Config_T* FQspiPs_LookupConfig(u16 deviceId);
int FQspiPs_CfgInitialize(FQspiPs_T* qspiPtr, FQspiPs_Config_T* configPtr);
void FQspiPs_Reset(FQspiPs_T* qspiPtr);
void FQspiPs_SetStatusHandler(FQspiPs_T* qspiPtr, void* callBackRef, 
                              FQspiPs_StatusHandler funcPtr);
int FQspiPs_SelfTest(FQspiPs_T* qspiPtr);
void FQspiPs_InterruptHandler(void* instancePtr);

int FQspiPs_Initialize(FQspiPs_T* qspiPtr, u16 deviceId);
int FQspiPs_InitHw(FQspiPs_T* qspiPtr, FQspiPs_Caps* capsPtr);

int FQspiPs_GetFlashInfo(FQspiPs_T* qspiPtr);
int FQspiPs_SetFlashMode(FQspiPs_T* qspiPtr, u8 mode);

int FQspiPs_SendBytes(FQspiPs_T* qspiPtr, u32 offset, u32 byteCount, u8* sendBuffer);
int FQspiPs_RecvBytes(FQspiPs_T* qspiPtr, u32 offset, u32 byteCount, u8* recvBuffer);
int FQspiPs_FastSendBytes(FQspiPs_T* qspiPtr, u32 offset, u32 byteCount, u8* sendBuffer);
int FQspiPs_FastRecvBytes(FQspiPs_T* qspiPtr, u32 offset, u32 byteCount, u8* recvBuffer);

int FQspiPs_Erase(FQspiPs_T* qspiPtr, u64 address, u32 byteCount, u32 blockSize);
int FQspiPs_Write(FQspiPs_T* qspiPtr, u64 address, u32 byteCount, u8 *sendBuffer);
int FQspiPs_Read(FQspiPs_T* qspiPtr, u64 address, u32 byteCount, u8 *recvBuffer);
int FQspiPs_ResetFlash(FQspiPs_T* qspiPtr);
int FQspiPs_EnableQuad(FQspiPs_T* qspiPtr);
int FQspiPs_Lock(FQspiPs_T* qspiPtr);
int FQspiPs_Unlock(FQspiPs_T* qspiPtr);

u32 FQspiPs_ReadId(FQspiPs_T* qspiPtr, void* id); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* prevent circular inclusions */e