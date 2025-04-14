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
* @file fmsh_qspips.c
* @addtogroup qspips_v1_1
* @{
*
* Contains implements the interface functions of the FQspiPs driver.
* See fmsh_qspips.h for a detailed description of the device and driver.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who Date     Changes
* ----- --- -------- -----------------------------------------------
* 1.00  hzq 7/1/19 
* 		     First release
* 1.10  hzq 11/26/20 
* 		     The string.h header has benn increased.
*            The FQspiPs_SelfTest API has been modified to increase 
*		     the variables to save the value in Device Delay Register 
*		     to restore it after self-test.
* 		     The FQspiPs_SetFlashReadMode API has been removed.
* 		     The FQspiPs_InterruptHandler API has been modified to increase  
* 		     data transfer operation.
* 		     The qspi_dma_default has been increased which is a struct of
* 		     dma related parameters.
* 		     The qspi_ops has been increased which is a struct of 
* 		     qspi basic operations.
* 		     The FQspiPs_Initialize API has been increased to initialize 
* 		     FQspiPs with default configuration.
* 		     The FQspiPs_InitHw API has been increased to initialize 
* 		     FQspiPs with user defined configuration.
* 		     The FQspiPs_SendBytes API has been increased to send data to 
* 		     AHB interface using direct mode.
* 		     The FQspiPs_RecvBytes API has been increased to read data form 
* 		     AHB interface using direct mode. 		     
* 		     The FQspiPs_FastSendBytes API has been increased to send data to 
* 		     AHB interface using indirect mode.
* 		     The FQspiPs_FastRecvBytes API has been increased to read data from 
* 		     AHB interface using indirect mode. 		     
* 		     The FQspiPs_ReadId API has been increased.
* 		     The FQspiPs_GetFlashInfo API has been increased.
* 		     The FQspiPs_SetFlashMode API has been increased.
* 		     The FQspiPs_ResetFlash API has been increased.
* 		     The FQspiPs_EnableQuad API has been increased.
* 		     The FQspiPs_Unlock API has been increased.
* 		     The FQspiPs_Erase API has been increased.
* 		     The FQspiPs_Write API has been increased.
* 		     The FQspiPs_Read API has been increased.
*
* </pre>
*
******************************************************************************/

#include <string.h>
#include "fmsh_ps_parameters.h"
#include "fmsh_slcr.h"
#include "fmsh_qspips_lib.h"

static void StubStatusHandler(void *callBackRef, u32 statusEvent, unsigned byteCount);
static int FQspiPs_Calibration(FQspiPs_T* qspiPtr, u32 baudRate);
static int FQspiPs_Initr(FQspiPs_T* qspiPtr);

/*****************************************************************************
* This function initializes a specific FQspiPs_T device/instance. This function
* must be called prior to using the device to read or write any data.
*
* @param	spi is a pointer to the FQspiPs_T instance.
* @param	configPtr points to the FQspiPs_T device configuration structure.
*
* @return
*		- FMSH_SUCCESS if successful.
*		- FMSH_FAILURE if fail.
*
* @note		The user needs to first call the FQspiPs_LookupConfig() API
*		which returns the Configuration structure pointer which is
*		passed as a parameter to the FQspiPs_CfgInitialize() API.
*
******************************************************************************/
int FQspiPs_CfgInitialize(FQspiPs_T *qspiPtr, FQspiPs_Config_T *configPtr)
{
    FMSH_ASSERT(qspiPtr != NULL);
    FMSH_ASSERT(configPtr != NULL);        
    
    memset(qspiPtr, 0, sizeof(FQspiPs_T));
    /* set default value */
    qspiPtr->config.deviceId = configPtr->deviceId;
    qspiPtr->config.baseAddress = configPtr->baseAddress;
    qspiPtr->config.dataBaseAddress = configPtr->dataBaseAddress;
    
    qspiPtr->type = "NOR"; /* default NOR flash*/
    
    qspiPtr->statusHandler = StubStatusHandler;
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function resets qspi device registers to default value.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_Reset(FQspiPs_T* qspiPtr)
{    
    if(qspiPtr->config.deviceId == FPS_QSPI0_DEVICE_ID)
    {
        FSlcrPs_ipSetRst(SLCR_QSPI0_CTRL, QSPI_APB_RST);
        FSlcrPs_ipSetRst(SLCR_QSPI0_CTRL, QSPI_AHB_RST);
        FSlcrPs_ipSetRst(SLCR_QSPI0_CTRL, QSPI_REF_RST);
        FSlcrPs_ipReleaseRst(SLCR_QSPI0_CTRL, QSPI_APB_RST);
        FSlcrPs_ipReleaseRst(SLCR_QSPI0_CTRL, QSPI_AHB_RST);
        FSlcrPs_ipReleaseRst(SLCR_QSPI0_CTRL, QSPI_REF_RST);
    }
    else if(qspiPtr->config.deviceId == FPS_QSPI1_DEVICE_ID)
    {
        FSlcrPs_ipSetRst(SLCR_QSPI1_CTRL, QSPI_APB_RST);
        FSlcrPs_ipSetRst(SLCR_QSPI1_CTRL, QSPI_AHB_RST);
        FSlcrPs_ipSetRst(SLCR_QSPI1_CTRL, QSPI_REF_RST);
        FSlcrPs_ipReleaseRst(SLCR_QSPI1_CTRL, QSPI_APB_RST);
        FSlcrPs_ipReleaseRst(SLCR_QSPI1_CTRL, QSPI_AHB_RST);
        FSlcrPs_ipReleaseRst(SLCR_QSPI1_CTRL, QSPI_REF_RST);
    }
}

/*****************************************************************************
* This function sets point to status handler as well as its callback parameter .
*
* @param
*
* @return
*
* @note		
*       - this function is usually used called in interrupt 
*       - implemented by user
*
******************************************************************************/
void FQspiPs_SetStatusHandler(FQspiPs_T* qspiPtr, void* callBackRef, FQspiPs_StatusHandler funcPtr)
{
    FMSH_ASSERT(qspiPtr != NULL);
    FMSH_ASSERT(funcPtr != NULL);
    
    qspiPtr->statusHandler = funcPtr;
    qspiPtr->statusRef = callBackRef;
}

/*****************************************************************************
* This is a stub for the status callback. The stub is here in case the upper
* layers forget to set the handler.
*
* @param	CallBackRef is a pointer to the upper layer callback reference
* @param	StatusEvent is the event that just occurred.
* @param	ByteCount is the number of bytes transferred up until the event
*		occurred.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
static void StubStatusHandler(void *callBackRef, u32 statusEvent, unsigned byteCount)
{
    (void) callBackRef;
    (void) statusEvent;
    (void) byteCount;
}

/*****************************************************************************
* This function tests if spi device exists.
*
* @param
*
* @return
*		- FMSH_SUCCESS if qspi device exists.
*		- FMSH_FAILURE if qspi device not exists.
*
* @note		
*
******************************************************************************/
int FQspiPs_SelfTest(FQspiPs_T* qspiPtr)
{
    u8 delayCSDA, delayCSDADS, delayCSEOT, delayCSSOT;
    u8 delayCSDA_old, delayCSDADS_old, delayCSEOT_old, delayCSSOT_old;
    
    FQspiPs_GetDelays(qspiPtr, &delayCSDA_old, &delayCSDADS_old, &delayCSEOT_old, &delayCSSOT_old);
    
    delayCSDA = 0x5A;
    delayCSDADS = 0xA5;
    delayCSEOT = 0xAA;
    delayCSSOT = 0x55;
    
    FQspiPs_SetDelays(qspiPtr, delayCSDA, delayCSDADS, delayCSEOT, delayCSSOT);
    FQspiPs_GetDelays(qspiPtr, &delayCSDA, &delayCSDADS, &delayCSEOT, &delayCSSOT);
    if((0x5A != delayCSDA) || (0xA5 != delayCSDADS) || 
       (0xAA != delayCSEOT) || (0x55 != delayCSSOT))
    {
        return FMSH_FAILURE;
    }
    
    FQspiPs_SetDelays(qspiPtr, delayCSDA_old, delayCSDADS_old, delayCSEOT_old, delayCSSOT_old);
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* The interrupt handler for QSPI interrupts. This function must be connected
* by the user to an interrupt source. This function does not save and restore
* the processor context such that the user must provide this processing.
*
* The interrupts that are handled are:
*
* @param	InstancePtr is a pointer to the FQspiPs_T instance to be worked on.
*
* @return	None.
*
* @note
*
*
******************************************************************************/
void FQspiPs_InterruptHandler(void* instancePtr)
{
    FQspiPs_T* qspiPtr = (FQspiPs_T*)instancePtr;
    u32 configReg;
    u32 intrStatus;
    
    /* Get & Clear interrupt status */
    intrStatus = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_ISR_OFFSET);  
    FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_ISR_OFFSET, QSPIPS_INTR_ALL);
    
    /* legacy mode */ 
    
    /* tx fifo underflow */
    if (intrStatus & QSPIPS_INTR_TX_UNDERFLOW_MASK)
    {
        qspiPtr->isBusy = FALSE;
        /*upper layer has something else to do*/
		qspiPtr->statusHandler(qspiPtr->statusRef, 
                               intrStatus,
                               qspiPtr->remainingBytes);  
    }
    /* rx fifo overflow */
    if (intrStatus & QSPIPS_INTR_RX_OVERFLOW_MASK)
    {
        qspiPtr->isBusy = FALSE;
        /*upper layer has something else to do*/
        qspiPtr->statusHandler(qspiPtr->statusRef, 
                               intrStatus,
                               qspiPtr->remainingBytes);          
    }
    /* tx fifo full */
    if (intrStatus & QSPIPS_INTR_TX_FULL_MASK)
    {
        /*upper layer has something else to do*/
        qspiPtr->statusHandler(qspiPtr->statusRef, 
                               intrStatus,
                               qspiPtr->remainingBytes);          
    }
    /* tx fifo can be written */
    if (intrStatus & QSPIPS_INTR_TX_NOT_FULL_MASK)
    {
        /*upper layer has something else to do*/
        qspiPtr->statusHandler(qspiPtr->statusRef, 
                               intrStatus,
                               qspiPtr->remainingBytes);    
    }
    /* rx fifo can be read */
    if ((intrStatus & QSPIPS_INTR_RX_NOT_EMPTY_MASK) || 
        (intrStatus & QSPIPS_INTR_RX_FULL_MASK))
    { 
        /*upper layer has something else to do*/
        qspiPtr->statusHandler(qspiPtr->statusRef, 
                               intrStatus,
                               qspiPtr->remainingBytes);
    }
    
    /* request rejected */
    
    /* 2 requests already in queue */
    if (intrStatus & QSPIPS_INTR_REQ_REJECT_MASK)
    {            
        qspiPtr->isBusy = FALSE;
        qspiPtr->statusHandler(qspiPtr->statusRef, 
                               intrStatus,
                               qspiPtr->remainingBytes);          
    }
    /* try to write data to protected address */
    if (intrStatus & QSPIPS_INTR_WRITE_PROTECT_MASK)
    {
        qspiPtr->isBusy = FALSE;
        qspiPtr->statusHandler(qspiPtr->statusRef, 
                               intrStatus,
                               qspiPtr->remainingBytes);         
    }
    /* ahb access error */
    if (intrStatus & QSPIPS_INTR_ILLEGAL_AHB_ACCESS_MASK)
    {
        qspiPtr->isBusy = FALSE;
        qspiPtr->statusHandler(qspiPtr->statusRef, 
                               intrStatus, 
                               qspiPtr->remainingBytes);          
    }
    /* poll expired */
    if (intrStatus & QSPIPS_INTR_POLL_EXPIRED_MASK)
    {
        qspiPtr->isBusy = FALSE;
        qspiPtr->statusHandler(qspiPtr->statusRef, 
                               intrStatus,
                               qspiPtr->remainingBytes);          
    }
    
    /* indirect mode transfer running */
    if ((intrStatus & QSPIPS_INTR_WATERMARK_LEVEL_BREACHED_MASK) || 
        (intrStatus & QSPIPS_INTR_SRAM_READ_FULL_MASK))
    {
        u8 remainder;
        u32 wordCount;
        u32 cnt = 0;
        
        /*read progress*/
        configReg = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_IRTCR_OFFSET);
        if(configReg & 0x4)
        {
            /* Read  Watermark */
            wordCount = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_SFLR_OFFSET);
            remainder = qspiPtr->remainingBytes & 0x3;
            
            /* Read data from SRAM */
            for(cnt = 0; cnt < wordCount; cnt++)
            {
                if(qspiPtr->remainingBytes >= 4)
                {
                    *(u32*)(qspiPtr->dataBufferPtr) = *(u32*)(qspiPtr->config.dataBaseAddress);
                    (qspiPtr->dataBufferPtr)+=4;
                    (qspiPtr->remainingBytes)-=4;  
                }
                else
                {                    
                    if(remainder == 0)
                        break;
                    
                    u32 value = *(u32*)(qspiPtr->config.dataBaseAddress);
                    while(remainder > 0)
                    {
                        *(u8*)(qspiPtr->dataBufferPtr) = value & 0xff;
                        value = value >>8;
                        (qspiPtr->dataBufferPtr)++;
                        (qspiPtr->remainingBytes)--; 
                        remainder--;
                    }
                    
                    break;
                }
            }
        }
        /*write progress*/
        configReg = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_IWTCR_OFFSET);
        if(configReg & 0x4)
        {
            configReg = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_DSCR_OFFSET);
            int pageSize = (configReg & 0xfff0) >> 4;
            wordCount = pageSize >> 2;
            remainder = qspiPtr->remainingBytes & 0x3;
            
            for(cnt = 0; cnt < wordCount; cnt++)
            {
                if(qspiPtr->remainingBytes >= 4)
                {
                    *(u32*)(qspiPtr->config.dataBaseAddress) = *(u32*)(qspiPtr->dataBufferPtr);
                    (qspiPtr->dataBufferPtr)+=4;
                    (qspiPtr->remainingBytes)-=4; 
                }
                else
                {
                    u32 value = 0xffffffff;
                    
                    if(remainder == 0)
                        break;
                    
                    while(remainder > 0)
                    {
                        value = value << 8;
                        value = value | *(u8*)((qspiPtr->dataBufferPtr) + remainder - 1);
                        (qspiPtr->remainingBytes)--; 
                        remainder--;
                    }
                    *(u32*)(qspiPtr->config.dataBaseAddress) = value;
                    break;
                }
            }
        }
        
        qspiPtr->statusHandler(qspiPtr->statusRef, 
                               intrStatus,
                               qspiPtr->remainingBytes);              
    }  
    
    /* stig mode transfer complete*/
    if (intrStatus & QSPIPS_INTR_STIG_COMPLETE_MASK)
    {
        qspiPtr->isBusy = FALSE;
        qspiPtr->statusHandler(qspiPtr->statusRef, 
                               intrStatus,
                               qspiPtr->remainingBytes);          
    }
    /* indirect mode transfer complete*/
    if (intrStatus & QSPIPS_INTR_INDIRECT_COMPLETE_MASK)
    {
        /*read complete*/
        configReg = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_IRTCR_OFFSET);
        if(configReg & 0x20)
        {
            FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IRTCR_OFFSET, 0x20); 
        }
        /*write complete*/
        configReg = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_IWTCR_OFFSET);
        if(configReg & 0x20)
        {
            FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IWTCR_OFFSET, 0x20); 
        }
        
        qspiPtr->isBusy = FALSE;
        qspiPtr->statusHandler(qspiPtr->statusRef, 
                               intrStatus,
                               qspiPtr->remainingBytes);          
    } 
}

static FQspiPs_Dma qspi_dma_default = {
   /*
   .burstType = 5,
    .singleType = 2,
    .txIf = 6,
    .rxIf = 7,
    .io =  0, 
    */
	5, /* burstType */
	2, /* singleType */
	6, /* txIf */
	7, /* rxIf */
	0  /* io */
};

static FQspiPs_Ops qspi_ops = {
 /*
 	.Erase = FQspiPs_Erase,
    .Write = FQspiPs_Write,
    .Read = FQspiPs_Read,
    .Reset = FQspiPs_ResetFlash,
    .EnableQuad = FQspiPs_EnableQuad,
    .Lock = FQspiPs_Lock,
    .Unlock = FQspiPs_Unlock,
    */
	FQspiPs_Erase,      /* Erase*/
	FQspiPs_Write,      /* Write*/
	FQspiPs_Read,       /* Read*/
	FQspiPs_EnableQuad, /* EnableQuad */
	FQspiPs_Lock,       /* Lock*/
	FQspiPs_Unlock,     /* Unlock*/
	FQspiPs_ResetFlash  /* Reset */
};

/*****************************************************************************
* This function initializes controller struct
*
* @param
*       - DeviceId contains the ID of the device
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Initialize(FQspiPs_T* qspiPtr, u16 deviceId)
{
    int status;
    FQspiPs_Config_T* cfgPtr;
    
    FMSH_ASSERT(qspiPtr != NULL);
    
    cfgPtr = FQspiPs_LookupConfig(deviceId);
    if (cfgPtr == NULL) 
    {
        return FMSH_FAILURE;
    }
    
    status = FQspiPs_CfgInitialize(qspiPtr, cfgPtr);  
    if(status != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;
    }
    
    /*qspiPtr->type = "NAND";*/

    status = FQspiPs_InitHw(qspiPtr, NULL);
    if (status != FMSH_SUCCESS) 
    {
        return FMSH_FAILURE;
    }
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function initializes controller hardware
*
* @param
*       - capsPtr point to FQspiPs_Caps
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_InitHw(FQspiPs_T* qspiPtr, FQspiPs_Caps* capsPtr)
{
    int status = FMSH_SUCCESS;
    FQspiPs_Caps* caps;
    
    FMSH_ASSERT(qspiPtr != NULL);
    
    /* Check whether there is another transfer in progress. Not thread-safe*/
    if(qspiPtr->isBusy == TRUE) 
    {
        return QSPI_BUSY;
    }
    else
    {
        qspiPtr->version = 102;
        
        /* reset hardware*/
        FQspiPs_Reset(qspiPtr); 
        
        /*increase operation speed*/
        FQspiPs_SetBaudRate(qspiPtr, QSPIPS_BAUD(8));
        
        /*get maker & device size*/
        status = FQspiPs_GetFlashInfo(qspiPtr);
        if(status != FMSH_SUCCESS)
            return QSPI_DETECT_FAIL;
        
        /*config qspi caps*/
        if(capsPtr == NULL)
        {
            if(strcmp(qspiPtr->type, "NOR") == 0)
                memcpy((void*)&(qspiPtr->caps),
                       (void*)&qspi_nor_caps_default,
                       (unsigned int)sizeof(FQspiPs_Caps));
            else if(strcmp(qspiPtr->type, "NAND") == 0)
                memcpy((void*)&(qspiPtr->caps),
                       (void*)&qspi_nand_caps_default,
                       (unsigned int)sizeof(FQspiPs_Caps));
        }
        else
        {
            memcpy((void*)&(qspiPtr->caps),
                   (void*)capsPtr,
                   (unsigned int)sizeof(FQspiPs_Caps));
        }
        
        caps = &(qspiPtr->caps);
        
        /*fill other members*/
        qspiPtr->dma = &qspi_dma_default;
        qspiPtr->ops = &qspi_ops;
        
        status = FQspiPs_ResetFlash(qspiPtr);
        if(status != FMSH_SUCCESS)
            return QSPI_INIT_FAIL;
        
        /*set transfer mode*/
        status = FQspiPs_SetFlashMode(qspiPtr, caps->rdMode); 
        if(status != FMSH_SUCCESS)
            return QSPI_INIT_FAIL;
        
        /*set baud rate & data capture delay*/
        FQspiPs_Calibration(qspiPtr, caps->baudRate);
        
        status = FQspiPs_Initr(qspiPtr);
        if(status != FMSH_SUCCESS)
            return QSPI_INIT_FAIL;
    }
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function calibrate read delay capture
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
static int FQspiPs_Calibration(FQspiPs_T* qspiPtr, u32 baudRate)
{
    int i;
    u32 id = 0, id_temp = 0;
    int range_lo = -1, range_hi = -1;
    
    FQspiPs_Disable(qspiPtr);
    FQspiPs_SetBaudRate(qspiPtr, QSPIPS_BAUD(8));
    FQspiPs_SetCaptureDelay(qspiPtr, 0);
    FQspiPs_Enable(qspiPtr);
    id = FQspiPs_ReadId(qspiPtr, NULL);
    
    FQspiPs_SetBaudRate(qspiPtr, QSPIPS_BAUD(baudRate));
    for(i=0; i<16; i++)
    {
        FQspiPs_Disable(qspiPtr);
        FQspiPs_SetCaptureDelay(qspiPtr, i);
        FQspiPs_Enable(qspiPtr);
        id_temp = FQspiPs_ReadId(qspiPtr, NULL);
        /* search for lowest value of delay */
        if((range_lo == -1) && (id == id_temp))
        {
            range_lo = i;
            continue;
        }
        /* search for highest value of delay */
        if((range_lo != -1) && (id != id_temp))
        {
            range_hi = i-1;
            break;
        }
        range_hi = i;
    }
    
    if(range_lo == -1)
    {
        TRACE_OUT(DEBUG_OUT, "QSPI Calibration failed\n");
        return FMSH_FAILURE;
    }
    
    FQspiPs_Disable(qspiPtr);
    FQspiPs_SetCaptureDelay(qspiPtr, (range_hi + range_lo) / 2);
    FQspiPs_Enable(qspiPtr);
    /*TRACE_OUT(DEBUG_OUT, "QSPI Read data capture delay calibrate to %i (%i - %i)\n", */
    /*          (range_hi + range_lo) / 2, range_lo, range_hi);*/
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function write flash using direct mode
*
* @param
*       - offset is a value where data write 
*       - byteCount is a number of bytes to write
*       - sendBuffer is a point to write data
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_SendBytes(FQspiPs_T* qspiPtr, u32 offset, u32 byteCount, u8 *sendBuffer)
{    
    FMSH_ASSERT(qspiPtr != NULL);
    FMSH_ASSERT(byteCount != 0);
    FMSH_ASSERT(sendBuffer != NULL);
        
    u32 address = qspiPtr->config.dataBaseAddress + offset;
    /* Write to Flash  */
    memcpy((void*)address,
           (void*)sendBuffer,
           (size_t)byteCount); 
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function read flash using direct mode
*
* @param
*       - offset is a value where data read 
*       - byteCount is a number of bytes to read
*       - recvBuffer is a point to read data
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_RecvBytes(FQspiPs_T* qspiPtr, u32 offset, u32 byteCount, u8 *recvBuffer)
{
    u32 address;
    
    FMSH_ASSERT(qspiPtr != NULL);
    FMSH_ASSERT(byteCount != 0);
    FMSH_ASSERT(recvBuffer != NULL);
        
    /* Read from flash */
    address = qspiPtr->config.dataBaseAddress + offset;
    memcpy((void*)recvBuffer,
           (void*)address,
           (size_t)byteCount); 
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function write flash using indirect mode
*
* @param
*       - offset is a value where data write 
*       - byteCount is a number of bytes to write
*       - sendBuffer is a point to write data
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_FastSendBytes(FQspiPs_T* qspiPtr, u32 offset, u32 byteCount, u8* sendBuffer)
{
    u32 intrMask = 0;
    
    FMSH_ASSERT(qspiPtr != NULL);
    FMSH_ASSERT(sendBuffer != NULL);
    
    /* Check whether there is another transfer in progress. Not thread-safe*/
    if(qspiPtr->isBusy == TRUE)
    {
        return QSPI_BUSY;
    }
    else
    {
        qspiPtr->remainingBytes = byteCount;
        qspiPtr->dataBufferPtr = sendBuffer;
        
        /* set flash address where write data (address in flash) */
        FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IWTSAR_OFFSET, offset);  
        /* set number of bytes to be transferred*/
        FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IWTNBR_OFFSET, qspiPtr->remainingBytes);  
        /* set controller trig adress where write data (adress in controller)*/
        FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IAATR_OFFSET, qspiPtr->config.dataBaseAddress);    
        /* set Trigger Address Range (2^n)*/
        FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_ITARR_OFFSET, 0xf);      
        /* set WaterMark Register (between 10~250 if page_size is 256 bytes)*/
        FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IWTWR_OFFSET, qspiPtr->caps.txEmptyLvl);   
        FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IRTWR_OFFSET, 0x0); 
        
        if(qspiPtr->caps.hasIntr == 1)
        {
            if(qspiPtr->caps.hasDma == 0)
            {
                /*clear all interrupts*/
                FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_ISR_OFFSET, QSPIPS_INTR_ALL);
                /*enalbe interrupts*/
                intrMask = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_IMR_OFFSET);  
                intrMask |= QSPIPS_INTR_INDIRECT_COMPLETE_MASK | QSPIPS_INTR_REQ_REJECT_MASK | 
                    QSPIPS_INTR_SRAM_READ_FULL_MASK | QSPIPS_INTR_WATERMARK_LEVEL_BREACHED_MASK;
                FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IMR_OFFSET, intrMask);
            }
            else
            {
                /*clear all interrupts*/
                FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_ISR_OFFSET, QSPIPS_INTR_ALL);
                /*enalbe interrupts*/
                intrMask = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_IMR_OFFSET);  
                intrMask |= QSPIPS_INTR_INDIRECT_COMPLETE_MASK | QSPIPS_INTR_REQ_REJECT_MASK;
                intrMask &= ~(QSPIPS_INTR_SRAM_READ_FULL_MASK | QSPIPS_INTR_WATERMARK_LEVEL_BREACHED_MASK);
                FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IMR_OFFSET, intrMask);
            }
        }
        else
        {
            /*dsiable interrupts*/
            intrMask = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_IMR_OFFSET);  
            intrMask &= ~(QSPIPS_INTR_INDIRECT_COMPLETE_MASK | QSPIPS_INTR_REQ_REJECT_MASK | 
                          QSPIPS_INTR_SRAM_READ_FULL_MASK | QSPIPS_INTR_WATERMARK_LEVEL_BREACHED_MASK);
            FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IMR_OFFSET, intrMask); 
        }
        
        if(qspiPtr->caps.hasDma == 1)
        {
            /*Set DPCR*/
            u32 NumBurstType = qspiPtr->dma->burstType << 8;
            u32 NumSingleType = qspiPtr->dma->singleType;
            FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_DPCR_OFFSET, NumBurstType | NumSingleType); 
            FQspiPs_EnableDMA(qspiPtr);
        }
        else
        {
            FQspiPs_DisableDMA(qspiPtr);
        }
        
        /*Set the busy flag, cleared when transfer is done*/
        qspiPtr->isBusy = TRUE;
        
        /* trigger Indirect Write access*/
        FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IWTCR_OFFSET, 0x1);
        
        /*without interrupt & dma, using poll*/
        if(qspiPtr->caps.hasIntr == 0 && qspiPtr->caps.hasDma == 0)
        {
            u32 configReg;
            int wordCount, remainder;
            int cnt;
            
            /*write progress*/
            do{
                configReg = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_IWTCR_OFFSET);
                if(configReg & 0x4)
                {
                    wordCount = QSPIFLASH_PAGE_SIZE << 2;
                    remainder = qspiPtr->remainingBytes & 0x3;
                    
                    for(cnt = 0; cnt < wordCount; cnt++)
                    {
                        if(qspiPtr->remainingBytes >= 4)
                        {
                            *(u32*)(qspiPtr->config.dataBaseAddress) = *(u32*)(qspiPtr->dataBufferPtr);
                            (qspiPtr->dataBufferPtr)+=4;
                            (qspiPtr->remainingBytes)-=4; 
                        }
                        else
                        {
                            u32 value = 0xffffffff;
                            
                            if(remainder == 0)
                                break;
                            
                            while(remainder > 0)
                            {
                                value = value << 8;
                                value = value | *(u8*)((qspiPtr->dataBufferPtr) + remainder - 1);
                                (qspiPtr->remainingBytes)--; 
                                remainder--;
                            }
                            *(u32*)(qspiPtr->config.dataBaseAddress) = value;
                            break;
                        }
                    }
                }  
            }while(configReg & 0x4);
                   
            configReg = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_IWTCR_OFFSET);
            if(configReg & 0x20)
            {
                FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IWTCR_OFFSET, 0x20); 
                qspiPtr->isBusy = FALSE;
            }
        }
    }
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function read flash using indirect mode
*
* @param
*       - offset is a value where data read 
*       - byteCount is a number of bytes to read
*       - recvBuffer is a point to read data
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_FastRecvBytes(FQspiPs_T* qspiPtr, u32 offset, u32 byteCount, u8* recvBuffer)
{    
    u32 intrMask = 0;
    
    FMSH_ASSERT(qspiPtr != NULL);
    FMSH_ASSERT(recvBuffer != NULL);
    
    /* Check whether there is another transfer in progress. Not thread-safe*/
    if(qspiPtr->isBusy == TRUE)
    {
        return QSPI_BUSY;
    }
    else
    {
        qspiPtr->remainingBytes = byteCount;
        qspiPtr->dataBufferPtr = recvBuffer;
        
        /* set flash address where read data (address in flash) */
        FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IRTSAR_OFFSET, offset);  
        /* set number of bytes to be transferred */
        FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IRTNBR_OFFSET, qspiPtr->remainingBytes);  
        /* set controller trig adress where read data (adress in controller) */
        FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IAATR_OFFSET, qspiPtr->config.dataBaseAddress);    
        /* set Trigger Address Range (2^15) */
        FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_ITARR_OFFSET, 0xf);  
        /* set WaterMark Register */
        FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IWTWR_OFFSET, 0x0); 
        FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IRTWR_OFFSET, qspiPtr->caps.rxFullLvl);  
        
        if(qspiPtr->caps.hasIntr == 1)
        {
            if(qspiPtr->caps.hasDma == 0)
            {
                /*clear all interrupts*/
                FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_ISR_OFFSET, QSPIPS_INTR_ALL);
                /*enalbe interrupts*/
                intrMask = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_IMR_OFFSET);  
                intrMask |= QSPIPS_INTR_INDIRECT_COMPLETE_MASK | QSPIPS_INTR_REQ_REJECT_MASK |
                    QSPIPS_INTR_SRAM_READ_FULL_MASK | QSPIPS_INTR_WATERMARK_LEVEL_BREACHED_MASK;
                FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IMR_OFFSET, intrMask);
            }
            else
            {
                /*clear all interrupts*/
                FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_ISR_OFFSET, QSPIPS_INTR_ALL);
                /*enalbe interrupts*/
                intrMask = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_IMR_OFFSET);  
                intrMask |= QSPIPS_INTR_INDIRECT_COMPLETE_MASK | QSPIPS_INTR_REQ_REJECT_MASK;
                intrMask &= ~(QSPIPS_INTR_SRAM_READ_FULL_MASK | QSPIPS_INTR_WATERMARK_LEVEL_BREACHED_MASK);
                FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IMR_OFFSET, intrMask);
            }
        }
        else
        {
            /*dsiable interrupts*/
            intrMask = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_IMR_OFFSET);  
            intrMask &= ~(QSPIPS_INTR_INDIRECT_COMPLETE_MASK | QSPIPS_INTR_REQ_REJECT_MASK | 
                          QSPIPS_INTR_SRAM_READ_FULL_MASK | QSPIPS_INTR_WATERMARK_LEVEL_BREACHED_MASK);
            FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IMR_OFFSET, intrMask); 
        }
        
        if(qspiPtr->caps.hasDma == 1)
        {
            /*Set DPCR*/
            u32 NumBurstType = qspiPtr->dma->burstType << 8;
            u32 NumSingleType = qspiPtr->dma->singleType;
            FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_DPCR_OFFSET, NumBurstType | NumSingleType); 
            FQspiPs_EnableDMA(qspiPtr);
        }
        else
        {
            FQspiPs_DisableDMA(qspiPtr);
        }
        
        /*Set the busy flag, cleared when transfer is done*/
        qspiPtr->isBusy = TRUE;
        
        /* trigger Indirect Write access */
        FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IRTCR_OFFSET, 0x1);  
        
        /*without interrupt & dma, using poll*/
        if(qspiPtr->caps.hasIntr == 0 && qspiPtr->caps.hasDma == 0)
        {
            u32 configReg;
            int wordCount, remainder;
            int cnt;
            
            do{
                /*read progress*/
                configReg = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_IRTCR_OFFSET);
                if(configReg & 0x4)
                {
                    /* Read  Watermark */
                    wordCount = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_SFLR_OFFSET);
                    remainder = qspiPtr->remainingBytes & 0x3;
                    
                    /* Read data from SRAM */
                    for(cnt = 0; cnt < wordCount; cnt++)
                    {
                        if(qspiPtr->remainingBytes >= 4)
                        {
                            *(u32*)(qspiPtr->dataBufferPtr) = *(u32*)(qspiPtr->config.dataBaseAddress);
                            (qspiPtr->dataBufferPtr)+=4;
                            (qspiPtr->remainingBytes)-=4;  
                        }
                        else
                        {                    
                            if(remainder == 0)
                                break;
                            
                            u32 value = *(u32*)(qspiPtr->config.dataBaseAddress);
                            while(remainder > 0)
                            {
                                *(u8*)(qspiPtr->dataBufferPtr) = value & 0xff;
                                value = value >>8;
                                (qspiPtr->dataBufferPtr)++;
                                (qspiPtr->remainingBytes)--; 
                                remainder--;
                            }
                            
                            break;
                        }
                    }
                }
            }while(configReg & 0x4);
            
            configReg = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_IRTCR_OFFSET);
            if(configReg & 0x20)
            {
                FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_IRTCR_OFFSET, 0x20); 
                qspiPtr->isBusy = FALSE;
            } 
        }
    }
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function executes READ ID.
*
* @param
*
* @return
*		- ID value.
*
* @note		
*
******************************************************************************/
u32 FQspiPs_ReadId(FQspiPs_T* qspi, void* id)
{
    u32* id1 = (u32*)id;
    u32 id_l, id_h;
	
    /*Send RDID Command (4 bytes)*/
    FQspiPs_CmdExecute(qspi, 0x9FF00001);
	
    /*Get ID*/
    id_l = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_FCRDRL_OFFSET);
    id_h = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_FCRDRU_OFFSET);
	
   if (id != NULL)
    {
        *id1 = id_l;
        *(id1+1) = id_h;
    }
   
    return id_l;
}

/*****************************************************************************
* This function read and chcek flash id
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
int FQspiPs_GetFlashInfo(FQspiPs_T* qspiPtr)
{  
    int status;
    u8 id[8];
    
    FMSH_ASSERT(qspiPtr != NULL);
    
    /*Read ID in Auto mode.*/
    FQspiPs_ReadId(qspiPtr, (void*)id);
    
    if(strcmp(qspiPtr->type, "NAND") == 0)
        status = FQspiPs_Nand_GetFlashInfo(qspiPtr, id);
    else if(strcmp(qspiPtr->type, "NOR") == 0)
        status = FQspiPs_Nor_GetFlashInfo(qspiPtr, id);

    return status;
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
int FQspiPs_SetFlashMode(FQspiPs_T* qspiPtr, u8 mode)
{
    FMSH_ASSERT(qspiPtr != NULL);
    
    if(strcmp(qspiPtr->type, "NOR") == 0) 
        FQspiPs_Nor_SetFlashMode(qspiPtr, mode);
    else if(strcmp(qspiPtr->type, "NAND") == 0) 
        FQspiPs_Nand_SetFlashMode(qspiPtr, mode);
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function executes Flash Reset.
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
int FQspiPs_ResetFlash(FQspiPs_T* qspiPtr)
{
    int status;
    
    FMSH_ASSERT(qspiPtr != NULL);
    
    if(strcmp(qspiPtr->type, "NOR") == 0) 
        status =FQspiPs_Nor_Reset(qspiPtr);
    else if(strcmp(qspiPtr->type, "NAND") == 0)
        status = FQspiPs_Nand_Reset(qspiPtr);
    
    return status;
}

/*****************************************************************************
* This function executes Enable Quad.
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
int FQspiPs_EnableQuad(FQspiPs_T* qspiPtr)
{
    int status;
    
    FMSH_ASSERT(qspiPtr != NULL);
    
    if(strcmp(qspiPtr->type, "NOR") == 0) 
        status = FQspiPs_Nor_EnableQuad(qspiPtr);
    else if(strcmp(qspiPtr->type, "NAND") == 0)
        status = FQspiPs_Nand_EnableQuad(qspiPtr);
    
    return status;
}

/*****************************************************************************
* This function executes Lock Flash.
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
int FQspiPs_Lock(FQspiPs_T* qspiPtr)
{
    int status;
    
    FMSH_ASSERT(qspiPtr != NULL);
    
    if(strcmp(qspiPtr->type, "NOR") == 0) 
        status = FQspiPs_Nor_Lock(qspiPtr);
    else if(strcmp(qspiPtr->type, "NAND") == 0)
        status = FQspiPs_Nand_Lock(qspiPtr);
    
    return status;
}

/*****************************************************************************
* This function executes Unlock Flash.
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
int FQspiPs_Unlock(FQspiPs_T* qspiPtr)
{
    int status;
    
    FMSH_ASSERT(qspiPtr != NULL);
    
    if(strcmp(qspiPtr->type, "NOR") == 0) 
        status = FQspiPs_Nor_Unlock(qspiPtr);
    else if(strcmp(qspiPtr->type, "NAND") == 0)
        status = FQspiPs_Nand_Unlock(qspiPtr);
    
    return status;
}

/*****************************************************************************
* This function executes Flash ERASE.
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
int FQspiPs_Erase(FQspiPs_T* qspiPtr, u64 address, u32 byteCount, u32 blockSize)
{
    int status;
    
    FMSH_ASSERT(qspiPtr != NULL);
    
    if(strcmp(qspiPtr->type, "NOR") == 0) 
        status = FQspiPs_Nor_Erase(qspiPtr, address, byteCount, blockSize);
    else if(strcmp(qspiPtr->type, "NAND") == 0)
        status = FQspiPs_Nand_Erase(qspiPtr, address, byteCount, blockSize);
    
    return status;
}

/*****************************************************************************
* This function executes Flash Write
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
int FQspiPs_Write(FQspiPs_T* qspiPtr, u64 address, u32 byteCount, u8 *sendBuffer)
{
    int status;
    
    FMSH_ASSERT(qspiPtr != NULL);
    
    if(strcmp(qspiPtr->type, "NOR") == 0) 
        status = FQspiPs_Nor_Write(qspiPtr, address, byteCount, sendBuffer);
    else if(strcmp(qspiPtr->type, "NAND") == 0)
        status = FQspiPs_Nand_Write(qspiPtr, address, byteCount, sendBuffer);
    
    return status;
}

/*****************************************************************************
* This function executes Flash Read
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
int FQspiPs_Read(FQspiPs_T* qspiPtr, u64 address, u32 byteCount, u8 *recvBuffer)
{
    int status;
    
    FMSH_ASSERT(qspiPtr != NULL);
    
    if(strcmp(qspiPtr->type, "NOR") == 0) 
        status = FQspiPs_Nor_Read(qspiPtr, address, byteCount, recvBuffer);
    else if(strcmp(qspiPtr->type, "NAND") == 0)
        status = FQspiPs_Nand_Read(qspiPtr, address, byteCount, recvBuffer);
    
    return status;
}

/*****************************************************************************
* initialize device after controller ready
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
static int FQspiPs_Initr(FQspiPs_T* qspiPtr)
{
    int status;
    
    FMSH_ASSERT(qspiPtr != NULL);
    
    if(strcmp(qspiPtr->type, "NOR") == 0) 
        status = FQspiPs_Nor_Initr(qspiPtr);
    else if(strcmp(qspiPtr->type, "NAND") == 0)
        status = FQspiPs_Nand_Initr(qspiPtr);
    
    return status;
}
