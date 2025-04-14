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
* @file fmsh_spips.h
* @addtogroup spips_v1_1
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
* 		     Add macro of configuration used for initialization.
*            Add type defination FSpiPs_Caps which is a struct of 
*            configuration.     
*            Add type defination FSpiPs_Dma which is a struct of 
*            dma related parameters.  
*            Modify FSpiPs_T to increase members.
*            Add FSpiPs_Initialize function prototype.      
*            Add FSpiPs_InitHw function prototype.  
*            Add FSpiPs_Transfer function prototype.  
*            Add FSpiPs_PolledTransfer function prototype.  
*                       
* </pre>
*
******************************************************************************/

#ifndef _FMSH_SPIPS_H_	    /* prevent circular inclusions */ 
#define _FMSH_SPIPS_H_	/* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif
    
/***************************** Include Files *********************************/ 

/************************** Constant Definitions *****************************/ 
#ifndef FPAR_SPIPS_0_DEVICE_ID    
    #define FPS_SPI0_DEVICE_ID          (0)   
#else
    #define FPS_SPI0_DEVICE_ID          FPAR_SPIPS_0_DEVICE_ID
#endif

#ifndef  FPS_SPI0_BASEADDR  
    #define FPS_SPI0_BASEADDR           (0xe0001000)
#endif
    
#ifndef FPAR_SPIPS_1_DEVICE_ID    
    #define FPS_SPI1_DEVICE_ID          (1)        
#else
    #define FPS_SPI1_DEVICE_ID          FPAR_SPIPS_1_DEVICE_ID  
#endif

#ifndef  FPS_SPI1_BASEADDR  
    #define FPS_SPI1_BASEADDR           (0xe0021000)
#endif
    
#define FPS_SPI_NUM_INSTANCES       (2)

#define SPIPS_TRANSFER_DONE         (0x80)

#define SPI_BUSY                    (-1)
#define SPI_INIT_FAIL               (-5)

#define SPI_CAPS(name) static FSpiPs_Caps spi_##name##_caps
#define GET_SPI_CAPS(name) &spi_##name##_caps
    
#define SPI_OPT_USRHANDLED           (0x1)
/**************************** Type Definitions *******************************/   
    
typedef void (*FSpiPs_StatusHandler) (void *CallBackRef, u32 StatusEvent,
                                      u32 ByteCount);
    
typedef struct {
    u16 deviceId;		            /**< Unique ID of device */
    u32 baseAddress;                /**< APB Base address of the device */
} FSpiPs_Config_T;

typedef struct {
    u8 isMaster;
    u8 loop;
    u8 hasIntr;
    u8 hasDma;
    u8 txEmptyLvl;
    u8 rxFullLvl;
    u8 tsfMode;
    u8 cpol;
    u8 cpha;
    u8 frameSize;
    u32 frameLen;  
    u32 baudRate; 
    u8 sampleDelay;
} FSpiPs_Caps;

typedef struct {
    u8 type;
    u8 txIf;
    u8 rxIf;
    u32 io; 
} FSpiPs_Dma;

typedef struct FSpiPs_Tag{
    FSpiPs_Config_T     config;         /**< Configuration structure */
    
    u32                 flag;
    u32                 options;
    u8                  isBusy;
    u8                  isEnable;   
    u8                  slaveSelect;
    
    u8                  isMaster; /*not used*/
    u8                  frameSize; /*not used*/
    
    FSpiPs_Caps         caps;
    FSpiPs_Dma*         dma;
    
    u32                 totalBytes;
    u32                 remainingBytes;
    u32                 requestedBytes;	 
    u8*                 sendBufferPtr;	            /**< Buffer to send (state) */
    u8*                 recvBufferPtr;	            /**< Buffer to receive (state) */
                   
    int (*Transfer)(struct FSpiPs_Tag spi, void* sendBuffer, void* recvBuffer, u32 byteCount);
    
    FSpiPs_StatusHandler statusHandler;
    void* statusRef;
    
} FSpiPs_T;

/**********************************Variable Definition**************************/


/**********************************Function Prototype***************************/

FSpiPs_Config_T* FSpiPs_LookupConfig(u16 deviceId);
int FSpiPs_CfgInitialize(FSpiPs_T* spi, FSpiPs_Config_T* ConfigPtr);
void FSpiPs_Reset(FSpiPs_T* spi);
int FSpiPs_SelfTest(FSpiPs_T* spi);
void FSpiPs_SetStatusHandler(FSpiPs_T* spi, void* callBackRef, FSpiPs_StatusHandler funcPtr);
void FSpiPs_InterruptHandler(void* instancePtr);
int FSpiPs_Initialize(FSpiPs_T* spiPtr, u16 deviceId);
int FSpiPs_HwInit(FSpiPs_T* spiPtr, FSpiPs_Caps* capsPtr);
int FSpiPs_Transfer(FSpiPs_T* spiPtr, void* sendBuffer, void* recvBuffer, u32 byteCount);
int FSpiPs_PolledTransfer(FSpiPs_T* spiPtr, void* sendBuffer, void* recvBuffer, u32 byteCount);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* prevent circular inclusions */
