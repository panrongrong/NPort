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

#include <vxWorks.h>
#include <intLib.h>
#include <logLib.h>
#include <muxLib.h>
#include <semLib.h>
#include <sysLib.h>
#include <vxBusLib.h>
#include <wdLib.h>
#include <string.h>

#include "../../common/fmsh_ps_parameters.h"

#include "../../slcr/fmsh_slcr.h"
#include "fmsh_spips_lib.h" 


/*****************************************************************************/
/**
*
* @file fmsh_spips.c
* @addtogroup spips_v1_1
* @{
*
* Contains implements the interface functions of the FSpiPs driver.
* See fmsh_spips.h for a detailed description of the device and driver.
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
* 		     The FSpiPs_InterruptHandler API has been modified to increase  
* 		     data transfer operation.
* 		     The spi_dma_default has been increased which is a struct of
* 		     dma related parameters.
* 		     The spi_caps_default has been increased which is a struct of 
* 		     spi user configuration.
* 		     The FSpiPs_Initialize API has been increased to initialize 
* 		     FSpiPs with default configuration.
* 		     The FSpiPs_InitHw API has been increased to initialize 
* 		     FSpiPs with user defined configuration.
* 		     The FSpiPs_Transfer API has been increased to transfer data.
* 		     The FSpiPs_PolledTransfer API has been used to transfer data 
*            using poll. 		     
*
* </pre>
*
******************************************************************************/

#include <string.h>
#include "../../common/fmsh_ps_parameters.h"/*#include "fmsh_ps_parameters.h"*/
#include "../../slcr/fmsh_slcr.h" /*#include "fmsh_slcr.h"*/
#include "fmsh_spips_lib.h" 

static void StubStatusHandler(void *callBackRef, u32 statusEvent,
                              u32 byteCount);

/*****************************************************************************
* This function initializes a specific FSpiPs_T device/instance. This function
* must be called prior to using the device to read or write any data.
*
* @param	spi is a pointer to the FSpiPs_T instance.
* @param	configPtr points to the FSpiPs_T device configuration structure.
*
* @return
*		- FMSH_SUCCESS if successful.
*		- FMSH_FAILURE if fail.
*
* @note		The user needs to first call the FSpiPs_LookupConfig() API
*		which returns the Configuration structure pointer which is
*		passed as a parameter to the FSpiPs_CfgInitialize() API.
*
******************************************************************************/
int FSpiPs_CfgInitialize(FSpiPs_T* spi, FSpiPs_Config_T* configPtr)
{
    FMSH_ASSERT(spi != NULL);
    FMSH_ASSERT(configPtr != NULL);
    
    /* Set default value*/
    spi->config.deviceId = configPtr->deviceId;
    spi->config.baseAddress = configPtr->baseAddress;
    
    spi->flag = 0;
    spi->options = 0;
    spi->isBusy = FALSE;
    spi->isEnable = FALSE;
    spi->slaveSelect = 0;
    
    spi->dma = NULL;
    
    spi->totalBytes = 0;
    spi->remainingBytes = 0;
    spi->requestedBytes = 0;
    spi->sendBufferPtr = NULL;
    spi->recvBufferPtr = NULL;
    
    spi->statusHandler = StubStatusHandler;
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function resets spi device registers to default value.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FSpiPs_Reset(FSpiPs_T* spi)
{  
    if(spi->config.deviceId == FPS_SPI0_DEVICE_ID)
    {
        FSlcrPs_ipSetRst(SLCR_SPI0_CTRL, SPI0_APB_RST);
        FSlcrPs_ipSetRst(SLCR_SPI0_CTRL, SPI0_REF_RST);
        FSlcrPs_ipReleaseRst(SLCR_SPI0_CTRL, SPI0_APB_RST);
        FSlcrPs_ipReleaseRst(SLCR_SPI0_CTRL, SPI0_REF_RST);
    }
    else if(spi->config.deviceId == FPS_SPI1_DEVICE_ID)
    {
        FSlcrPs_ipSetRst(SLCR_SPI1_CTRL, SPI1_APB_RST);
        FSlcrPs_ipSetRst(SLCR_SPI1_CTRL, SPI1_REF_RST);
        FSlcrPs_ipReleaseRst(SLCR_SPI1_CTRL, SPI1_APB_RST);
        FSlcrPs_ipReleaseRst(SLCR_SPI1_CTRL, SPI1_REF_RST);
    }
}

/*****************************************************************************
* This function tests if spi device exists.
*
* @param
*
* @return
*		- FMSH_SUCCESS if spi device exists.
*		- FMSH_FAILURE if spi device not exists.
*
* @note		
*
******************************************************************************/
int FSpiPs_SelfTest(FSpiPs_T* spi)
{   
    u32 value;
    FSpiPs_SetTxEmptyLvl(spi, 0xa);
    value = FSpiPs_GetTxLevel(spi);
    if(value != 0xa)
        return FMSH_FAILURE;
    FSpiPs_SetTxEmptyLvl(spi, 0x0);
    return FMSH_SUCCESS;   
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
void FSpiPs_SetStatusHandler(FSpiPs_T* spi, void* callBackRef,
                             FSpiPs_StatusHandler funcPtr)
{
    FMSH_ASSERT(spi != NULL);
    FMSH_ASSERT(funcPtr != NULL);
    
    spi->statusHandler = funcPtr;
    spi->statusRef = callBackRef;
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
static void StubStatusHandler(void *callBackRef, u32 statusEvent,
                              u32 byteCount)
{
    (void) callBackRef;
    (void) statusEvent;
    (void) byteCount;
}

/*****************************************************************************
* The interrupt handler for SPI interrupts. This function must be connected
* by the user to an interrupt source. This function does not save and restore
* the processor context such that the user must provide this processing.
*
* The interrupts that are handled are:
*
* - Multi Master Fault. This interrupt is generated if both device set as master
*   and try to transfer with the same slave. The driver aborts this transfer.
*   The upper layer software is informed of the error.
*
* - Data Receive Register (FIFO) Overrun. This interrupt is generated when the
*   SPI device attempts to write a received byte to an already full DRR/FIFO.
*   A full DRR/FIFO usually means software is not emptying the data in a timely
*   manner.  No action is taken by the driver other than to inform the upper
*   layer software of the error.
*
* - Data Receive Register (FIFO) Underrun. This interrupt is generated when the
*   SPI device attempts to read a received byte from an empty DRR/FIFO.
*   A empty DRR/FIFO usually means software is not fill the data in a timely
*   manner.  No action is taken by the driver other than to inform the upper
*   layer software of the error.
*
* - Data Transmit Register (FIFO) Overrun. This interrupt is generated when
*   the SPI device attempts to write data to an already full DTR/FIFO.  
*   An full DTR/FIFO usually means that software is not giving the
*   device data in a timely manner. No action is taken by the driver other than
*   to inform the upper layer software of the error.
*
* - Data Transmit Register (FIFO) Empty. This interrupt is generated when the
*   transmit register or FIFO is empty. The driver uses this interrupt during a
*   transmission to continually send/receive data until there is no more data
*   to send/receive.
*
* - Data Receive Register (FIFO) Full. This interrupt is generated when the
*   receive register or FIFO is full. The driver uses this interrupt during a
*   transmission, used as slave, to continually send/receive data until 
*   there is no more data to send/receive.
*
* @param	InstancePtr is a pointer to the FSpiPs_T instance to be worked on.
*
* @return	None.
*
* @note
*
* The slave select register is being set to deselect the slave when a transfer
* is complete.  This is being done regardless of whether it is a slave or a
* master since the hardware does not drive the slave select as a slave.
*
******************************************************************************/
void FSpiPs_InterruptHandler(void* instancePtr)
{
    FSpiPs_T* spiPtr = (FSpiPs_T*)instancePtr;
    FSpiPs_Caps* caps;
    /*u32 intrMask;*/
    u32 intrStatus;
    int cnt;
    /*fmql16550Pollprintf("In FSpiPs_InterruptHandler\n");*/
    FMSH_ASSERT(instancePtr != NULL);
    
    if(spiPtr->isBusy == FALSE)
        return;
    
    
    /* Get & Clear interrupt status */
    
    intrStatus =  FMSH_ReadReg(spiPtr->config.baseAddress, SPIPS_ISR_OFFSET);
    /*intrMask = FMSH_ReadReg(spiPtr->config.baseAddress, SPIPS_IMR_OFFSET);*/
    /*FMSH_WriteReg(spiPtr->config.baseAddress, SPIPS_IMR_OFFSET, 0);*/
    /*logMsg("intrStatus = 0X%X\n",intrStatus,2,3,4,5,6);*/
    
    /*mask all spi interrupt*/
  /*  FSpiPs_DisableIntr(spiPtr, 0x3f);*/
    
    /* Multi-Master Fault */
    if (intrStatus & SPIPS_INTR_MSTIS_MASK)
    {
        FSpiPs_SetEnable(spiPtr, 0);
        spiPtr->isBusy = FALSE;
        spiPtr->statusHandler(spiPtr->statusRef, 
                              SPIPS_INTR_MSTIS_MASK, 
                              0);
    }
    
    /* Check for overflow and underflow errors */
    if (intrStatus & SPIPS_INTR_RXOIS_MASK) 
    {
        FSpiPs_SetEnable(spiPtr, 0);
        spiPtr->isBusy = FALSE;
        spiPtr->statusHandler(spiPtr->statusRef,
                              SPIPS_INTR_RXOIS_MASK, 
                              0);
    }
    
    if (intrStatus & SPIPS_INTR_RXUIS_MASK) 
    {
        FSpiPs_SetEnable(spiPtr, 0);
        spiPtr->isBusy = FALSE;
        spiPtr->statusHandler(spiPtr->statusRef,
                              SPIPS_INTR_RXUIS_MASK, 
                              0);
    }
    
    if (intrStatus & SPIPS_INTR_TXOIS_MASK) 
    {
        FSpiPs_SetEnable(spiPtr, 0);
        spiPtr->isBusy = FALSE;
        spiPtr->statusHandler(spiPtr->statusRef,
                              SPIPS_INTR_TXOIS_MASK, 
                              0);
    }
    
    /* Tx Empty */
    if (intrStatus & SPIPS_INTR_TXEIS_MASK) 
    {
        if((spiPtr->remainingBytes != 0) || (spiPtr->requestedBytes != 0))
        {  
            if((spiPtr->options & SPI_OPT_USRHANDLED) == 0)
            {
                caps = &(spiPtr->caps);
                
                cnt = FMSH_ReadReg(spiPtr->config.baseAddress, SPIPS_RXFLR_OFFSET);
                /*logMsg("cnt = 0x%x\n",cnt,2,3,4,5,6);*/
                while(cnt > 0 && spiPtr->requestedBytes != 0)
                {
                    if(caps->frameSize == 8)
                    {
                        *(u8*)(spiPtr->recvBufferPtr) = (u8)FSpiPs_Recv(spiPtr);
                    }
                    else if(caps->frameSize == 16)
                    {
                        *(u16*)(spiPtr->recvBufferPtr) = (u16)FSpiPs_Recv(spiPtr);
                    }
                    else if(caps->frameSize == 32)
                    {
                        *(u32*)(spiPtr->recvBufferPtr) = (u32)FSpiPs_Recv(spiPtr);
                    }
                    
                    spiPtr->recvBufferPtr += caps->frameSize >> 3;
                    spiPtr->requestedBytes -= caps->frameSize >> 3;
                    cnt--;
                }
                cnt = SPIPS_FIFO_DEPTH - caps->txEmptyLvl;
                while(cnt > 0 && spiPtr->remainingBytes != 0)
                {
                    if(caps->frameSize == 8)
                    {
                        FSpiPs_Send(spiPtr, *(u8*)spiPtr->sendBufferPtr);
                    }
                    else if(caps->frameSize == 16)
                    {
                        FSpiPs_Send(spiPtr, *(u16*)spiPtr->sendBufferPtr);
                    }
                    else if(caps->frameSize == 32)
                    {
                        FSpiPs_Send(spiPtr, *(u32*)spiPtr->sendBufferPtr);
                    }
                    spiPtr->sendBufferPtr += caps->frameSize >> 3;
                    spiPtr->remainingBytes -= caps->frameSize >> 3;
                    cnt--;
                }   
            }
            
            /* Get & Clear interrupt status */

            FMSH_ReadReg(spiPtr->config.baseAddress, SPIPS_ICR_OFFSET);    
            
            /*upper layer has something else to do*/
            spiPtr->statusHandler(spiPtr->statusRef,
                                  SPIPS_INTR_TXEIS_MASK,
                                  spiPtr->remainingBytes);   
            
        }
        else if(!(FSpiPs_GetStatus(spiPtr) & SPIPS_SR_BUSY))
        {    
            /* Transfer complete, diable interrupt & spi*/
            FMSH_WriteReg(spiPtr->config.baseAddress, SPIPS_IMR_OFFSET, 0);
            
            FSpiPs_SetEnable(spiPtr, 0);
            
            spiPtr->isBusy = FALSE;
            
            /* inform upper layer*/
            spiPtr->statusHandler(spiPtr->statusRef,
                                  SPIPS_TRANSFER_DONE,
                                  spiPtr->totalBytes);
        }
        
    }
    
    /* Rx FIFO Full */
    if (intrStatus & SPIPS_INTR_RXFIS_MASK)
    {   
        if((spiPtr->options & SPI_OPT_USRHANDLED) == 0)
        {
            caps = &(spiPtr->caps);
            while((FSpiPs_GetStatus(spiPtr) & SPIPS_SR_RFNE) && spiPtr->requestedBytes != 0)
            {
                if(caps->frameSize == 8)
                {
                    *(u8*)(spiPtr->recvBufferPtr) = (u8)FSpiPs_Recv(spiPtr);
                }
                else if(caps->frameSize == 16)
                {
                    *(u16*)(spiPtr->recvBufferPtr) = (u16)FSpiPs_Recv(spiPtr);
                }
                else if(caps->frameSize == 32)
                {
                    *(u32*)(spiPtr->recvBufferPtr) = (u32)FSpiPs_Recv(spiPtr);
                }
                
                spiPtr->recvBufferPtr += caps->frameSize >> 3;
                spiPtr->requestedBytes -= caps->frameSize >> 3;
                cnt--;
            }
        }
        /*upper layer has something else to do*/
        spiPtr->statusHandler(spiPtr->statusRef,
                              SPIPS_INTR_RXFIS_MASK,
                              spiPtr->requestedBytes);   
        
        
        if((spiPtr->remainingBytes == 0) && (spiPtr->requestedBytes == 0))
        {          
            /* Transfer complete, diable interrupt & spi*/
            FMSH_WriteReg(spiPtr->config.baseAddress, SPIPS_IMR_OFFSET, 0);
            
            FSpiPs_SetEnable(spiPtr, 0);
            
            spiPtr->isBusy = FALSE;
            
            /* inform upper layer*/
            spiPtr->statusHandler(spiPtr->statusRef,
                                  SPIPS_TRANSFER_DONE,
                                  spiPtr->totalBytes);
            
        }
    }
    
    /*unmask all spi interrupt*/
   /* FSpiPs_DisableIntr(spiPtr, 0);*/
}

static FSpiPs_Caps spi_caps_default = {
    1,                /*.isMaster*/
    0,                /*.loop*/
    0,                /*.hasIntr*/
    0,                /*.hasDma*/
    10,               /*.txEmptyLvl*/
    0,                /*.rxFullLvl*/
    SPIPS_TRANSFER_STATE,    /*.tsfMode*/
    0,         /*.cpol*/
    0,         /*.cpha*/
    8,         /*.frameSize*/
    256,       /*.frameLen*/
    10,        /*.baudRate*/
    0,         /*.sampleDelay*/
};

static FSpiPs_Dma spi_dma_default = {
    0,     /*.type =  */
    2,     /*.txIf =  */
    3,     /*.rxIf =  */
    SPIPS_DR_OFFSET,     /*.io =   */ 
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
int FSpiPs_Initialize(FSpiPs_T* spiPtr, u16 deviceId)
{
    FSpiPs_Config_T* cfgPtr;
    
    FMSH_ASSERT(spiPtr != NULL);
    /*get config info table from parameter*/
    cfgPtr = FSpiPs_LookupConfig(deviceId);
    if (cfgPtr == NULL) 
    {
        return FMSH_FAILURE;
    }
    /*initialize controller struct*/
    return FSpiPs_CfgInitialize(spiPtr, cfgPtr);  
}

/*****************************************************************************
* This function initializes controller hardware
*
* @param
*       - capsPtr point to FSpiPs_Caps
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FSpiPs_HwInit(FSpiPs_T* spiPtr, FSpiPs_Caps* capsPtr)
{
    int status = FMSH_SUCCESS;
    u32 configReg;
    FSpiPs_Caps* caps;
    
    FMSH_ASSERT(spiPtr != NULL);
    
    /* Check whether there is another transfer in progress. Not thread-safe*/
    if(spiPtr->isBusy == TRUE) 
    {
        status = SPI_BUSY;
    }
    else
    {
        /*config spi caps*/
        if(capsPtr == NULL)
            memcpy((void*)&(spiPtr->caps),
                   (void*)&spi_caps_default,
                   (unsigned int)sizeof(FSpiPs_Caps));
        else
            memcpy((void*)&(spiPtr->caps),
                   (void*)capsPtr,
                   (unsigned int)sizeof(FSpiPs_Caps));
        
        spiPtr->dma = &spi_dma_default;
        
        caps = &(spiPtr->caps);
        
        /*reset hardware*/
        FSpiPs_Reset(spiPtr);
        
        /*disable spi*/
        FMSH_WriteReg(spiPtr->config.baseAddress, SPIPS_SSIENR_OFFSET, 0x0);
        
        /*config spi as master or slave*/
        if(caps->isMaster)
            FMSH_WriteReg(spiPtr->config.baseAddress, SPIPS_MSTR_OFFSET, 0x1);
        else
            FMSH_WriteReg(spiPtr->config.baseAddress, SPIPS_MSTR_OFFSET, 0x0);
        
        configReg = FMSH_ReadReg(spiPtr->config.baseAddress, SPIPS_CTRLR0_OFFSET);
        configReg &= ~SPIPS_CTRL0_MASK;
        /*config spi frame size_32(4~32)*/
        if(caps->frameSize<4 || caps->frameSize > 32)
            status = SPI_INIT_FAIL;
        else
            configReg |= (caps->frameSize-1) << SPIPS_CTRL0_DFS32_SHIFT;
        /*config spi transfer mode(0~3)  */
        if(caps->tsfMode > 3)
            status = SPI_INIT_FAIL;
        else
            configReg |= caps->tsfMode << SPIPS_CTRL0_TMOD_SHIFT;
        /*config spi cpol   */
        if(caps->cpol)
            configReg |= 0x1 << SPIPS_CTRL0_SCPOL_SHIFT;
        /*config spi cpha */
        if(caps->cpha)
            configReg |= 0x1 << SPIPS_CTRL0_SCPH_SHIFT;
        /*config loop(none or internal or slcr)*/
        if(caps->loop == 1)
            configReg |= 0x1 << SPIPS_CTRL0_SRL_SHIFT;
        
        FMSH_WriteReg(spiPtr->config.baseAddress, SPIPS_CTRLR0_OFFSET, configReg); 
        
        /*config baudrate & frame length*/
        if(caps->isMaster)
        {
            FMSH_WriteReg(spiPtr->config.baseAddress, SPIPS_BAUDR_OFFSET, caps->baudRate);  
            FMSH_WriteReg(spiPtr->config.baseAddress, SPIPS_CTRLR1_OFFSET, caps->frameLen - 1);
        }
        /*config rx sample delay*/
        FMSH_WriteReg(spiPtr->config.baseAddress, SPIPS_RX_SAMPLE_OFFSET, caps->sampleDelay);  
        
        /* Config IMR*/
        FMSH_WriteReg(spiPtr->config.baseAddress, SPIPS_IMR_OFFSET, 0);
        
        /* Config Tx/Rx Threshold*/
        if(caps->hasIntr)
        {
            FMSH_WriteReg(spiPtr->config.baseAddress, SPIPS_TXFTLR_OFFSET, caps->txEmptyLvl);
            FMSH_WriteReg(spiPtr->config.baseAddress, SPIPS_RXFTLR_OFFSET, caps->rxFullLvl);
        }
        
        if(caps->hasDma)
        {
            FMSH_WriteReg(spiPtr->config.baseAddress, SPIPS_DMATDLR_OFFSET, caps->txEmptyLvl);
            FMSH_WriteReg(spiPtr->config.baseAddress, SPIPS_DMARDLR_OFFSET, caps->rxFullLvl);
        }
        
        /*config slave selsct*/
        if(caps->isMaster)
            FMSH_WriteReg(spiPtr->config.baseAddress, SPIPS_SER_OFFSET, spiPtr->slaveSelect); 
    }
    
    if(status != FMSH_SUCCESS)
        return FMSH_FAILURE;
    else
        return FMSH_SUCCESS;
}

/*****************************************************************************
* This function transfers data
*
* @param
*       - sendBuffer is a point to write data
*       - recvBuffer is a point to read data
*       - byteCount is a number of bytes to transfer
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FSpiPs_Transfer(FSpiPs_T* spiPtr, void* sendBuffer, void* recvBuffer, u32 byteCount)
{
    int status = FMSH_SUCCESS;
    FSpiPs_Caps* caps;
    u32 intrMask = 0;
    
    FMSH_ASSERT(spiPtr != NULL);
    
    /* Check whether there is another transfer in progress. Not thread-safe*/
    if(spiPtr->isBusy == TRUE)
    {
        status = SPI_BUSY;  
    }
    else
    {        
        /*Get spi caps*/
        caps = &(spiPtr->caps);
        
        switch(caps->tsfMode)
        {
        case SPIPS_TRANSFER_STATE:
            if(sendBuffer == NULL || recvBuffer == NULL)
                return FMSH_FAILURE;
            spiPtr->remainingBytes = byteCount;   
            spiPtr->requestedBytes = byteCount;   
            intrMask |= SPIPS_INTR_TXEIS_MASK;
            break;
        case SPIPS_TRANSMIT_ONLY_STATE:
            if(sendBuffer == NULL)
                return FMSH_FAILURE;
            spiPtr->requestedBytes = 0;
            spiPtr->remainingBytes = byteCount;
            intrMask |= SPIPS_INTR_TXEIS_MASK;
            break;
        case SPIPS_RECEIVE_ONLY_STATE:
            if(recvBuffer == NULL)
                return FMSH_FAILURE;
            spiPtr->requestedBytes = byteCount;
            spiPtr->remainingBytes = 0;
            FSpiPs_SetDFNum(spiPtr, byteCount);
            intrMask |= SPIPS_INTR_RXFIS_MASK;
            break;
        case SPIPS_EEPROM_STATE:
            if(sendBuffer == NULL || recvBuffer == NULL)
                return FMSH_FAILURE;
            spiPtr->requestedBytes = byteCount;
            spiPtr->remainingBytes = 3;
            FSpiPs_SetDFNum(spiPtr, byteCount);
            intrMask |= SPIPS_INTR_RXFIS_MASK;
            break;
        default:
            status = FMSH_FAILURE;
            break;
        }
        
        spiPtr->totalBytes = byteCount;
        if(sendBuffer != NULL)
            spiPtr->sendBufferPtr = (u8*)sendBuffer;
        if(recvBuffer != NULL)
            spiPtr->recvBufferPtr = (u8*)recvBuffer;
        
        /*clear all interrupts*/
        FSpiPs_DisableIntr(spiPtr, 0x3f);
        FSpiPs_ClearIntrStatus(spiPtr);
        
        /*config interrupts*/
        if(caps->hasIntr)
        {
            intrMask |= SPIPS_INTR_RXOIS_MASK | SPIPS_INTR_RXUIS_MASK 
                | SPIPS_INTR_TXOIS_MASK;
            if(caps->isMaster)
                intrMask |= SPIPS_INTR_MSTIS_MASK;
            FSpiPs_EnableIntr(spiPtr, intrMask);
        }
        
        /*Set the busy flag, cleared when transfer is done*/
        spiPtr->isBusy = TRUE; 
        
        /*config slave select*/
        if(caps->isMaster)
            FSpiPs_SetSlave(spiPtr, spiPtr->slaveSelect);
        
        /*enable spi to start transfer*/
        FSpiPs_SetEnable(spiPtr, 1);
        
        if(caps->hasDma)
        {
            if(sendBuffer != NULL)
                FSpiPs_EnableDMATx(spiPtr);
            if(recvBuffer != NULL)
                FSpiPs_EnableDMARx(spiPtr);
        }
        
        if(caps->tsfMode == SPIPS_RECEIVE_ONLY_STATE)
            /* Write one dummy data word to Tx FIFO */
            FSpiPs_Send(spiPtr, 0xffffffff);
    }
    
    if(status != FMSH_SUCCESS)
        return FMSH_FAILURE;
    else
        return FMSH_SUCCESS;
}

/*****************************************************************************
* This function transfers data with polled
*
* @param
*       - sendBuffer is a point to write data
*       - recvBuffer is a point to read data
*       - byteCount is a number of bytes to transfer
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FSpiPs_PolledTransfer(FSpiPs_T* spiPtr, void* sendBuffer, void* recvBuffer, u32 byteCount)
{
    int status = FMSH_SUCCESS;
    FSpiPs_Caps* caps;
    int cnt;
    u32 txLvl;
    u32 txEmptyLvl;
    
    FMSH_ASSERT(spiPtr != NULL);
    
    /* Check whether there is another transfer in progress. Not thread-safe*/
    if(spiPtr->isBusy == TRUE)
    {
        status = SPI_BUSY;  
    }
    else
    {    
        /*Get spi caps*/
        caps = &(spiPtr->caps);
        
        switch(caps->tsfMode)
        {
        case SPIPS_TRANSFER_STATE:
            if(sendBuffer == NULL || recvBuffer == NULL)
                return FMSH_FAILURE;
            spiPtr->remainingBytes = byteCount;   
            spiPtr->requestedBytes = byteCount;   
            break;
        case SPIPS_TRANSMIT_ONLY_STATE:
            if(sendBuffer == NULL)
                return FMSH_FAILURE;
            spiPtr->requestedBytes = 0;
            spiPtr->remainingBytes = byteCount;
            break;
        case SPIPS_RECEIVE_ONLY_STATE:
            if(recvBuffer == NULL)
                return FMSH_FAILURE;
            spiPtr->requestedBytes = byteCount;
            spiPtr->remainingBytes = 0;
            FSpiPs_SetDFNum(spiPtr, byteCount);
            /* Write one dummy data word to Tx FIFO */
            FSpiPs_Send(spiPtr, 0xffffffff);
            break;
        case SPIPS_EEPROM_STATE:
            if(sendBuffer == NULL || recvBuffer == NULL)
                return FMSH_FAILURE;
            spiPtr->requestedBytes = byteCount;
            spiPtr->remainingBytes = 3;
            FSpiPs_SetDFNum(spiPtr, byteCount);
            break;
        default:
            status = FMSH_FAILURE;
            break;
        }
        
        spiPtr->totalBytes = byteCount;
        if(sendBuffer != NULL)
            spiPtr->sendBufferPtr = (u8*)sendBuffer;
        if(recvBuffer != NULL)
            spiPtr->recvBufferPtr = (u8*)recvBuffer;
        
        /*config slave select*/
        FSpiPs_SetSlave(spiPtr, spiPtr->slaveSelect);
        
        /*disable interrupt*/
        FSpiPs_DisableIntr(spiPtr, SPIPS_INTR_ALL);
        
        /*Set the busy flag, cleared when transfer is done*/
        spiPtr->isBusy = TRUE; 
        
        /*enable spi*/
        FSpiPs_SetEnable(spiPtr, 1);
        
        /*polling tx fifo level until transfer complete*/
        txEmptyLvl = caps->txEmptyLvl;
        while(spiPtr->remainingBytes !=0 || spiPtr->requestedBytes != 0) 
        {
            txLvl = FMSH_ReadReg(spiPtr->config.baseAddress, SPIPS_TXFLR_OFFSET);
            if(txLvl <= txEmptyLvl)
            {
                cnt = FMSH_ReadReg(spiPtr->config.baseAddress, SPIPS_RXFLR_OFFSET);
                while(cnt > 0 && spiPtr->requestedBytes != 0)
                {
                    if(caps->frameSize == 8)
                    {
                        *(u8*)(spiPtr->recvBufferPtr) = (u8)FSpiPs_Recv(spiPtr);
                    }
                    else if(caps->frameSize == 16)
                    {
                        *(u16*)(spiPtr->recvBufferPtr) = (u16)FSpiPs_Recv(spiPtr);
                    }
                    else if(caps->frameSize == 32)
                    {
                        *(u32*)(spiPtr->recvBufferPtr) = (u32)FSpiPs_Recv(spiPtr);
                    }
                    
                    spiPtr->recvBufferPtr += caps->frameSize >> 3;
                    spiPtr->requestedBytes -= caps->frameSize >> 3;
                    cnt--;
                }
                cnt = SPIPS_FIFO_DEPTH - txEmptyLvl;
                while(cnt > 0 && spiPtr->remainingBytes != 0)
                {
                    if(caps->frameSize == 8)
                    {
                        FSpiPs_Send(spiPtr, *(u8*)spiPtr->sendBufferPtr);
                    }
                    else if(caps->frameSize == 16)
                    {
                        FSpiPs_Send(spiPtr, *(u16*)spiPtr->sendBufferPtr);
                    }
                    else if(caps->frameSize == 32)
                    {
                        FSpiPs_Send(spiPtr, *(u32*)spiPtr->sendBufferPtr);
                    }
                    spiPtr->sendBufferPtr += caps->frameSize >> 3;
                    spiPtr->remainingBytes -= caps->frameSize >> 3;
                    cnt--;
                }           
            }
        }
        
        /*disable spi*/
        while(FSpiPs_GetStatus(spiPtr) & SPIPS_SR_BUSY);
        
        FSpiPs_SetEnable(spiPtr, 0);
        
        /*Clear the busy flag*/
        spiPtr->isBusy = FALSE;
        
    }
    
    if(status != FMSH_SUCCESS)
        return FMSH_FAILURE;
    else
        return FMSH_SUCCESS;
}
#if 0
#if defined(FPAR_GPIOPS_0_DEVICE_ID) && defined(FPAR_GPIOPS_1_DEVICE_ID)

#include "fmsh_gpio_lib.h"

static struct FSpiPs_MioArray {
    int ready;
    struct FSpiPs_Mio {
        int mioNo;
        int selected;
    } mio[21]; 
} mioArray = 
{
    /*ready*/
    0,
    /*mio*/
    {
        /*spio:0*/
        {18, 0},
        {19, 0},
        {20, 0},
        /*spio:1*/
        {30, 0},
        {31, 0},
        {32, 0},
        /*spio:2*/
        {42, 0},
        {43, 0},
        {44, 0},
        /*spi1:0*/
        {13, 0},
        {14, 0},
        {15, 0},
        /*spi1:1*/
        {25, 0},
        {26, 0},
        {27, 0},
        /*spi1:2*/
        {37, 0},
        {38, 0},
        {39, 0},
        /*spi1:2*/
        {49, 0},
        {50, 0},
        {51, 0}, 
    },
};

/*****************************************************************************
* This function scan mio configuration to find the mio that is configured
* as spi0/1, and mark the mio as 'selected'. This function run only once.
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*
* @note		
*
******************************************************************************/
static int FSpiPs_MioInit()
{
    int i;
    u32 slcrOff, slcrValue;
    
    if(mioArray.ready == 1)
        return FMSH_SUCCESS;
    
    for(i = 0; i < 21; i++)
    {
        slcrOff = 0x700 + (mioArray.mio[i].mioNo << 2);
        slcrValue = FMSH_ReadReg(SLCR_REG_BASE, slcrOff);
        /* check if this mio is selected as spi */
        if((slcrValue & 0xE0) == 0xA0)
            mioArray.mio[i].selected = 1;
        else
            mioArray.mio[i].selected = 0;
    }
    
    mioArray.ready = 1;
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function config mio to gpio if enabled, otherwise config mio to spi
*
* @param
*       - mio is the number of mio pin
*       - enable is a signal if mio configured as gpio or spi
*
* @return
*
* @note		
*
******************************************************************************/
static void FSpiPs_SetMioGpio(int mio, int enable)
{
    u32 slcrOff, slcrValue;
    
    slcrOff = 0x700 + (mio << 2);
    slcrValue = FMSH_ReadReg(SLCR_REG_BASE, slcrOff);
    slcrValue &= 0xffffff00;
    /* set cs mio to gpio */
    if(enable)
        FMSH_WriteReg(SLCR_REG_BASE, slcrOff, slcrValue);
    else
        FMSH_WriteReg(SLCR_REG_BASE, slcrOff, slcrValue | 0xa0);    
}

/*****************************************************************************
* This function enable/disable manual chip select.
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*
* @note		
*
******************************************************************************/
int FSpiPs_ManualChipSelect(FSpiPs_T* spiPtr, int enable)
{
    int i;
    
    FSpiPs_MioInit();
    
    FMSH_WriteReg(SLCR_REG_BASE, 0x008, 0xDF0D767BU);
    if(spiPtr->config.deviceId == FPAR_SDPS_0_DEVICE_ID)
    {
        for(i = 0; i < 9; i++)
        {
            if(mioArray.mio[i].selected == 1)
            {
                FSpiPs_SetMioGpio(mioArray.mio[i].mioNo, enable);
            } 
        }  
    }
    else if(spiPtr->config.deviceId == FPAR_SDPS_1_DEVICE_ID)
    {
        for(i = 10; i < 21; i++)
        {
            if(mioArray.mio[i].selected == 1)
            {
                FSpiPs_SetMioGpio(mioArray.mio[i].mioNo, enable);
            } 
        }  
    }
    FMSH_WriteReg(SLCR_REG_BASE, 0x004, 0xDF0D767BU);
    
    return FMSH_SUCCESS;
}


/*****************************************************************************
* This function set cs value, can only be used for mio gpio controled cs. 
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FSpiPs_SetChipSelect(FSpiPs_T* spiPtr, u8 cs, u8 set)
{
    int i;
    FGpioPs_T gpioDevA, gpioDevB;
    FGpioPs_Config *pGpioCfg;
    u32 gpioA = 0, gpioB = 0;
    u32 maskA = 0, maskB = 0;
    u32 outputA = 0, outputB = 0;
    u32 dirA, dirB;
    
    FMSH_ASSERT((cs > 0) && (cs < 4));
    
    if(spiPtr->config.deviceId == FPAR_SDPS_0_DEVICE_ID)
    {
        for(i = 0; i < 9; i++)
        {     
            if(mioArray.mio[i].selected == 1)
            {
                if(mioArray.mio[i].mioNo < 32)
                {
                    maskA |= 0x1 << (mioArray.mio[i].mioNo);
                    if((i % 3) == (cs - 1))
                    {
                        outputA |= 0x1 << (mioArray.mio[i].mioNo);
                    }
                }
                else
                {
                    maskB |= 0x1 << (mioArray.mio[i].mioNo - 32);
                    if((i % 3) == (cs - 1))
                    {
                        outputB |= 0x1 << (mioArray.mio[i].mioNo - 32);
                    }
                }
            }
        }
    }
    else if(spiPtr->config.deviceId == FPAR_SDPS_1_DEVICE_ID)
    {
        for(i = 10; i < 21; i++)
        {     
            if(mioArray.mio[i].selected == 1)
            {
                if(mioArray.mio[i].mioNo < 32)
                {
                    maskA |= 0x1 << (mioArray.mio[i].mioNo);
                }
                else
                {
                    maskB |= 0x1 << (mioArray.mio[i].mioNo - 32);
                }
            }
        }        
    }
       
    /* initialize gpio */
    if(maskA != 0)  
    {
        pGpioCfg = FGpioPs_LookupConfig(FPAR_GPIOPS_0_DEVICE_ID);
        FGpioPs_init(&gpioDevA, pGpioCfg);
        dirA = FGpioPs_getDirection(&gpioDevA);
        FGpioPs_setDirection(&gpioDevA, dirA | maskA);
        gpioA = FGpioPs_readData(&gpioDevA);
        if(set == 1)
        {
            gpioA |= outputA;
        }
        else
        {
            gpioA &= ~outputA;
        }
        FGpioPs_writeData(&gpioDevA, gpioA);
    }
    if(maskB != 0)
    {
        pGpioCfg = FGpioPs_LookupConfig(FPAR_GPIOPS_1_DEVICE_ID);
        FGpioPs_init(&gpioDevB, pGpioCfg);  
        dirB = FGpioPs_getDirection(&gpioDevB);
        FGpioPs_setDirection(&gpioDevB, dirB | maskB);
        gpioB = FGpioPs_readData(&gpioDevB);
        if(set == 1)
        {
            gpioB |= outputB;
        }
        else
        {
            gpioB &= ~outputB;
        }        
        FGpioPs_writeData(&gpioDevB, gpioB);
    }    
}

#endif /* FPAR_GPIOPS_0_DEVICE_ID && FPAR_GPIOPS_1_DEVICE_ID */
#endif
