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
* @file fmsh_qspips_hw.c
* @addtogroup qspips_v1_1
* @{
*
* Contains implements the low level interface functions of the FQspiPs driver.
* See fmsh_qspips_hw.h for a detailed description of the device and driver.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who Date     Changes
* ----- --- -------- -----------------------------------------------
* 1.00  hzq 7/1/19 
* 		     First release
* 1.10  hzq 11/26/20 
* 		     FQspiPs_SetPoll API has been increased.
* 		     FQspiPs_DisablePoll API has been increased.
*            FQspiPs_CmdExecute API has been modified to increase 
*            two step STIG mode trig.
*
* </pre>
*
******************************************************************************/

#include "fmsh_qspips_lib.h" 

/*****************************************************************************
* This function enables/disables qspi controller
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_Enable(FQspiPs_T* qspi)
{
    uint32_t configReg;
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET);
    configReg |= QSPIPS_ENABLE_MASK;
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET, configReg);
}

void FQspiPs_Disable(FQspiPs_T* qspi)
{
    uint32_t configReg;
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET);
    configReg &= ~QSPIPS_ENABLE_MASK;
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET, configReg);
}

/*****************************************************************************
* This function enables/disables qspi direct controller
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_EnableDAC(FQspiPs_T* qspi)
{
    uint32_t configReg;
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET);
    configReg |= QSPIPS_DAC_MASK;
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET, configReg);
}

void FQspiPs_DisableDAC(FQspiPs_T* qspi)
{
    uint32_t configReg;
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET);
    configReg &= ~QSPIPS_DAC_MASK;
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET, configReg);
}

/*****************************************************************************
* This function enables/disables qspi controller legacy function
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_EnableLegacy(FQspiPs_T* qspi)
{
    uint32_t configReg;
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET);
    configReg |= QSPIPS_LEGACYIP_MASK;
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET, configReg);
}

void FQspiPs_DisableLegacy(FQspiPs_T* qspi)
{
    uint32_t configReg;
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET);
    configReg &= ~QSPIPS_LEGACYIP_MASK;
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET, configReg);
}

/*****************************************************************************
* This function enables/disables qspi controller dma function
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_EnableDMA(FQspiPs_T* qspi)
{
    uint32_t configReg;
    
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET);
    configReg |= QSPIPS_DMA_MASK;
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET, configReg);
}

void FQspiPs_DisableDMA(FQspiPs_T* qspi)
{
    uint32_t configReg;
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET);
    configReg &= ~QSPIPS_DMA_MASK;
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET, configReg);
}

/*****************************************************************************
* This function enables/disables qspi controller remap function
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_EnableRemap(FQspiPs_T* qspi)
{
    uint32_t configReg;
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET);
    configReg |= QSPIPS_REMAP_MASK;
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET, configReg);
}

void FQspiPs_DisableRemap(FQspiPs_T* qspi)
{
    uint32_t configReg;
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET);
    configReg &= ~QSPIPS_REMAP_MASK;
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET, configReg);
}

/*****************************************************************************
* This function enables/disables qspi controller xip function
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_EnableXip(FQspiPs_T* qspi)
{
    uint32_t configReg;
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET);
    /* enable XIP next read operation */
    configReg |= QSPIPS_XIPN_MASK;
    /* enable XIP immediately */
    /*configReg |= QSPIPS_XIPI_MASK;*/
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET, configReg);
}

void FQspiPs_DisableXip(FQspiPs_T* qspi)
{
    uint32_t configReg;
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET);
    configReg &= ~(QSPIPS_XIPN_MASK | QSPIPS_XIPI_MASK);
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET, configReg);
}

/*****************************************************************************
* This function enables/disables qspi controller sw protect function
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_EnableProtect(FQspiPs_T* qspi, int inv)
{
    uint32_t configReg;
    /* drive hardware write protect pin */
    /*configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET);*/
    /*configReg |= QSPIPS_WPPIN_MASK;*/
    /*FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET, Reg);*/
    /* software write protection */
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_WPR_OFFSET);
    if(inv)
    {
        configReg |= QSPIPS_WP_ENABLE_MASK | QSPIPS_WP_INVERSION_MASK;
    }
    else
    {
        configReg |= QSPIPS_WP_ENABLE_MASK;
        configReg &= ~QSPIPS_WP_INVERSION_MASK;
    }
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_WPR_OFFSET, configReg);    
}

void FQspiPs_DisableProtect(FQspiPs_T* qspi)
{
    uint32_t configReg;
    /* drive hardware write protect pin */
    /*configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET);*/
    /*configReg &= ~QSPIPS_WPPIN_MASK;*/
    /*FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET, Reg);   */
    /* software write protection */
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_WPR_OFFSET);
    configReg &= ~QSPIPS_WP_ENABLE_MASK;
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_WPR_OFFSET, configReg);
}

/*****************************************************************************
* This function sets qspi controller clock format.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_SetClockFormat(FQspiPs_T* qspi, uint32_t clockFormat)
{
    uint32_t configReg;
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET);
    configReg &= ~QSPIPS_CLOCKFORMAT_MASK; 
    configReg |= (clockFormat << QSPIPS_CLOCKFORMAT_SHIFT) & QSPIPS_CLOCKFORMAT_MASK;
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET, configReg);
}

/*****************************************************************************
* This function sets qspi controller baud rate.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_SetBaudRate(FQspiPs_T* qspi, uint32_t baudRate)
{
    uint32_t configReg;
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET);
    configReg &= ~QSPIPS_BAUD_MASK;
    configReg |= (baudRate << QSPIPS_BAUD_SHIFT) & QSPIPS_BAUD_MASK;
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET, configReg);
}

/*****************************************************************************
* This function sets qspi controller address bytes number.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_SetAddrBytesNum(FQspiPs_T* qspi, uint32_t addrSize)
{
    uint32_t configReg;
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_DSCR_OFFSET);
    configReg &= ~QSPIPS_DS_ADDRSIZE_MASK;
    configReg |= (addrSize & QSPIPS_DS_ADDRSIZE_MASK);
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_DSCR_OFFSET, configReg);
}

/*****************************************************************************
* This function sets qspi controller page size.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_SetPageSize(FQspiPs_T* qspi, uint32_t pageSize)
{
    uint32_t configReg;
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_DSCR_OFFSET);
    configReg &= ~QSPIPS_DS_PAGESIZE_MASK; 
    configReg |= (pageSize << QSPIPS_DS_PAGESIZE_SHIFT) & QSPIPS_DS_PAGESIZE_MASK;
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_DSCR_OFFSET, configReg);
}

/*****************************************************************************
* This function sets qspi controller protected block size.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_SetBlockSize(FQspiPs_T* qspi, uint32_t blockSize)
{
    uint32_t configReg;
    /* Set Num of bytes per Block(2^n) protected */
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_DSCR_OFFSET);
    configReg &= ~QSPIPS_DS_BLOCKSIZE_MASK;
    configReg |= (blockSize << QSPIPS_DS_BLOCKSIZE_SHIFT) & QSPIPS_DS_BLOCKSIZE_MASK;
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_DSCR_OFFSET, configReg);
}
/*****************************************************************************
* This function sets qspi controller read capture delay cycles.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_SetCaptureDelay(FQspiPs_T* qspi, uint32_t cycles)
{
	uint32_t configReg;
	
	configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_RDCR_OFFSET);
	configReg &= ~QSPIPS_RC_CAPTURE_DELAY_MASK;
	configReg |= (cycles << QSPIPS_RC_CAPTURE_DELAY_SHIFT) & QSPIPS_RC_CAPTURE_DELAY_MASK;
	FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_RDCR_OFFSET, configReg);
}
/*****************************************************************************
* This function sets qspi controller remap offset.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_SetRemap(FQspiPs_T* qspi, uint32_t offset)
{
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_RAR_OFFSET, offset); 
}

/*****************************************************************************
* This function sets qspi controller protected low block.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_SetLowBlock(FQspiPs_T* qspi, uint32_t lowBlock)
{    
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_LWPR_OFFSET, lowBlock);
}

/*****************************************************************************
* This function sets qspi controller protected high block.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_SetHighBlock(FQspiPs_T* qspi, uint32_t highBlock)
{   
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_UWPR_OFFSET, highBlock);
}

/*****************************************************************************
* This function sets qspi controller tx empty level.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_SetTxNotFullLvl(FQspiPs_T* qspi, uint32_t threshold)
{
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_TTR_OFFSET, threshold);
}

/*****************************************************************************
* This function sets qspi controller rx full level.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_SetRxNotEmptyLvl(FQspiPs_T* qspi, uint32_t threshold)
{
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_RTR_OFFSET, threshold);
}

/*****************************************************************************
* This function sets qspi controller mode bits.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_SetModeBits(FQspiPs_T* qspi, u8 modeBits)
{
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_MBCR_OFFSET, modeBits);
}

/*****************************************************************************
* This function enables/disables interrupt.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_EnableIntr(FQspiPs_T* qspi, uint32_t mask)
{
    uint32_t configReg;
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_IMR_OFFSET);
    configReg |= mask;
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_IMR_OFFSET, configReg);
}

void FQspiPs_DisableIntr(FQspiPs_T* qspi, uint32_t mask)
{
    uint32_t configReg;
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_IMR_OFFSET);
    configReg &= ~mask;
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_IMR_OFFSET, configReg);
}

/*****************************************************************************
* This function clears interrupt status.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_ClearIntr(FQspiPs_T* qspi, uint32_t mask)
{
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_ISR_OFFSET, mask);    
}

/*****************************************************************************
* This function gets enabled interrupts.
*
* @param
*
* @return
*       - enabled interrupts   
*
* @note		
*
******************************************************************************/
uint32_t FQspiPs_IntrEnabled(FQspiPs_T* qspi)
{
    return FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_IMR_OFFSET);
}

/*****************************************************************************
* This function sets/gets qspi controller delays.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_SetDelays(FQspiPs_T* qspi, u8 csda, u8 csdads, u8 cseot, u8 cssot)
{
    u32 configReg;
    configReg = ((u32)csda << 24) | ((u32)csdads << 16) |
                ((u32)cseot << 8) | ((u32)cssot);
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_DDR_OFFSET, configReg);
}

void FQspiPs_GetDelays(FQspiPs_T* qspi, u8* csdaPtr, u8* csdadsPtr, u8* cseotPtr, u8* cssotPtr)
{
    u32 configReg;
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_DDR_OFFSET);
    *csdaPtr = (configReg >> 24) & 0xff;
    *csdadsPtr = (configReg >> 16) & 0xff;
    *cseotPtr = (configReg >> 8) & 0xff;
    *cssotPtr = (configReg) & 0xff;    
}

/*****************************************************************************
* This function sets/disable auto poll.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_SetPoll(FQspiPs_T* qspi, u8 opCode, u8 pollBit, int polarity)
{
    uint32_t configReg;
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_WCCR_OFFSET);
    configReg &= ~(0xffff);
    configReg = configReg | opCode | ((pollBit & 0x7) <<8) | ((polarity & 0x1) << 13);
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_WCCR_OFFSET, configReg);       
}

void FQspiPs_DisablePoll(FQspiPs_T* qspi)
{
    uint32_t configReg;
    configReg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_WCCR_OFFSET);
    configReg |= (0x1<<14);
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_WCCR_OFFSET, configReg);    
}

/*****************************************************************************
* This function executes qspi commands in stig mode.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
int FQspiPs_CmdExecute(FQspiPs_T* qspi, uint32_t cmd)
{
    int ret;
    
    cmd &= ~(u32)0x1;
    
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_FCCR_OFFSET, cmd);
    ret = FQspiPs_WaitIdle(qspi);
    if(ret){
        return FMSH_FAILURE;
    }
    
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_FCCR_OFFSET, cmd | 0x1);
    
    ret = FQspiPs_WaitForBit(qspi, QSPIPS_FCCR_OFFSET, QSPIPS_REG_FCCR_INPROGRESS_MASK, 0);
    if(ret){
        return FMSH_FAILURE;
    }
    
    ret = FQspiPs_WaitIdle(qspi);
    if(ret){
        return FMSH_FAILURE;
    }
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function waits for qspi controller idle.
*
* @param
*
* @return
*		- FMSH_SUCCESS if idle.
*		- FMSH_FAILURE if overtime.
*
* @note		
*
******************************************************************************/
int FQspiPs_WaitIdle(FQspiPs_T* qspi)
{
    uint32_t reg;
    uint32_t timeout = 0;
    
    while(1){
        reg = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_CFG_OFFSET); 
        if(reg & 0x80000000){
            return FMSH_SUCCESS;
        }   
        delay_1us();
        timeout++;
        if(timeout > 10){
            return FMSH_FAILURE;
        }
    }
}

/*****************************************************************************
* This function waits for certain register bit.
*
* @param
*
* @return
*		- FMSH_SUCCESS if right bit .
*		- FMSH_FAILURE if overtime.
*
* @note		
*
******************************************************************************/
int FQspiPs_WaitForBit(FQspiPs_T* qspi, uint32_t regOffset, uint32_t mask, u8 pollBit)
{
    uint32_t reg;
    uint32_t timeout = 0;
    
    while(1){
        reg = FMSH_ReadReg(qspi->config.baseAddress, regOffset); 
        if(pollBit == 0){  
            reg = ~reg;
        }
        reg &= mask;
        if(reg == mask ){
            return FMSH_SUCCESS;
        }   
        delay_1us();
        timeout++;
        if(timeout > 10){
            return FMSH_FAILURE;
        }
    }
}
