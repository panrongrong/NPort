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
* @file fmsh_spips_hw.c
* @addtogroup spips_v1_1
* @{
*
* Contains implements the low level interface functions of the FSpiPs driver.
* See fmsh_spips_hw.h for a detailed description of the device and driver.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who Date     Changes
* ----- --- -------- -----------------------------------------------
* 1.00  hzq 7/1/19 
* 		     First release
* 1.10  hzq 11/26/20 
* 		     The FSpiPs_SetMst API has been increased to replace 
* 		     FSpiPs_Mst and FSpiPs_Slv API functions.
* 		     The FSpiPs_SetEnable API has been increased to replace
* 		     FSpiPs_Enable and FSpiPs_Disable API functions.
* 		     The FSpiPs_SetSckMode API has been modified to update 
* 		     user configuration information.
* 		     The FSpiPs_SetTMode API has been modified to update 
* 		     user configuration information.
* 		     The FSpiPs_SetLoopback API has been modified to update 
* 		     user configuration information.
* 		     The FSpiPs_SetDFS32 API has been modified to update 
* 		     user configuration information.
* 		     The FSpiPs_SetDFNum API has been modified to update 
* 		     user configuration information.
* 		     The FSpiPs_SetSckDv API has been modified to update 
* 		     user configuration information.
*
* </pre>
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

#include "fmsh_spips_lib.h" 

/*****************************************************************************
* This function sets spi device work in master/slave
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FSpiPs_SetMst(FSpiPs_T *spi, u8 setMaster)
{
    if(setMaster)
        spi->caps.isMaster = 1;
    else
        spi->caps.isMaster = 0;

    FMSH_WriteReg(spi->config.baseAddress, SPIPS_MSTR_OFFSET, spi->caps.isMaster);
}

/*****************************************************************************
* This function enables/disables spi device
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FSpiPs_SetEnable(FSpiPs_T* spi, u8 setEnable)
{
    if(setEnable)
        spi->isEnable = 1;
    else
        spi->isEnable = 0;

    FMSH_WriteReg(spi->config.baseAddress, SPIPS_SSIENR_OFFSET, spi->isEnable);
}

/*****************************************************************************
* This function sets spi device CPOL/CPHA with 4 clock modes. It can only be set 
* when device is disabled.
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
int FSpiPs_SetSckMode(FSpiPs_T* spi, u32 sckMode)
{
    u32 configReg;
    
    if(spi->isEnable == 1 || sckMode > 3)
    {
        return FMSH_FAILURE;
    }
    
    spi->caps.cpha = sckMode & 0x1;
    spi->caps.cpol = (sckMode >> 1) & 0x1;
    
    configReg = FMSH_ReadReg(spi->config.baseAddress, SPIPS_CTRLR0_OFFSET);
    configReg &= ~(SPIPS_CTRL0_SCPH_MASK | SPIPS_CTRL0_SCPOL_MASK);
    configReg |= (sckMode << SPIPS_CTRL0_SCPH_SHIFT);
    FMSH_WriteReg(spi->config.baseAddress, SPIPS_CTRLR0_OFFSET, configReg); 
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function sets spi device transfer modes. It can only be set 
* when device is disabled. tmod 0 is transfer, tmod 1 is transmit only
* tmod 2 is receive only, tmod 3 is eeprom mode.
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
int FSpiPs_SetTMod(FSpiPs_T* spi, u32 tmod)
{
    u32 configReg;
    
    if(spi->isEnable == 1 || tmod > 3)
    {
        return FMSH_FAILURE;
    }    
    
    spi->caps.tsfMode = tmod;
    
    configReg = FMSH_ReadReg(spi->config.baseAddress, SPIPS_CTRLR0_OFFSET);
    configReg &= ~SPIPS_CTRL0_TMOD_MASK;
    configReg |= (tmod << SPIPS_CTRL0_TMOD_SHIFT);
    FMSH_WriteReg(spi->config.baseAddress, SPIPS_CTRLR0_OFFSET, configReg); 
    
    return FMSH_SUCCESS;
}

int FSpiPs_SetSlvOut(FSpiPs_T* spi, u8 enable)
{
    u32 configReg;
    
    if(spi->isEnable == 1)
    {
        return FMSH_FAILURE;
    }    
    
    configReg = FMSH_ReadReg(spi->config.baseAddress, SPIPS_CTRLR0_OFFSET);
    if(enable) 
        configReg &= ~SPIPS_CTRL0_SLVOE_MASK;
    else 
        configReg |= SPIPS_CTRL0_SLVOE_MASK;
    FMSH_WriteReg(spi->config.baseAddress, SPIPS_CTRLR0_OFFSET, configReg);  
    
    return FMSH_SUCCESS;
}

int FSpiPs_SetLoopBack(FSpiPs_T* spi, u8 enable)
{
    u32 configReg;
    
    if(spi->isEnable == 1)
    {
        return FMSH_FAILURE;
    }    
		/* CTRLR0 bit11: SRL - 环回逻辑，仅用于测试 */
    spi->caps.loop = enable;

    configReg = FMSH_ReadReg(spi->config.baseAddress, SPIPS_CTRLR0_OFFSET);
    if(enable)  
        configReg |= SPIPS_CTRL0_SRL_MASK;
    else  
        configReg &= ~SPIPS_CTRL0_SRL_MASK;
    FMSH_WriteReg(spi->config.baseAddress, SPIPS_CTRLR0_OFFSET, configReg); 
    
    return FMSH_SUCCESS;
}

/*
用来选择传输所用数据帧的长度
*/
int FSpiPs_SetDFS32(FSpiPs_T* spi, u32 dfs32)
{
    u32 configReg;
    
    if(spi->isEnable == 1 || dfs32<4 || dfs32 > 0x20)
    {
        return FMSH_FAILURE;
    }
    
    spi->caps.frameSize = dfs32;
        
    configReg = FMSH_ReadReg(spi->config.baseAddress, SPIPS_CTRLR0_OFFSET);
    configReg &= ~SPIPS_CTRL0_DFS32_MASK;
    configReg |= ((dfs32-1) << SPIPS_CTRL0_DFS32_SHIFT);
    FMSH_WriteReg(spi->config.baseAddress, SPIPS_CTRLR0_OFFSET, configReg); 
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function sets receivedata frame number. 
* it can only be set when device is disabled and device works in master
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
int FSpiPs_SetDFNum(FSpiPs_T* spi, u32 dfNum)
{
    if(spi->isEnable == TRUE || spi->caps.isMaster == FALSE)
    {
        return FMSH_FAILURE;
    }    
    
    spi->caps.frameLen = dfNum;
    
    FMSH_WriteReg(spi->config.baseAddress, SPIPS_CTRLR1_OFFSET, dfNum-1);
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function sets slave select. it can only be set when device works in master
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
int FSpiPs_SetSlave(FSpiPs_T* spi, u32 slaveNo)
{    
	DEBUG_LOG("in %s slaveNo = 0x%x\n",__FUNCTION__,slaveNo);
    if(spi->caps.isMaster == FALSE)
    {
        return FMSH_FAILURE;
    }
    
    if(slaveNo == 0)
    {
        FMSH_WriteReg(spi->config.baseAddress, SPIPS_SER_OFFSET, 0x0); 
    }
    else
    {
        FMSH_WriteReg(spi->config.baseAddress, SPIPS_SER_OFFSET, 0x1 << (slaveNo - 1)); 
    }
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function sets baud rate. it can only be set when device is disabled and device 
* works in master
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
int FSpiPs_SetSckDv(FSpiPs_T* spi, u32 sckdv)
{
    if(spi->isEnable == TRUE || spi->caps.isMaster == FALSE)
    {
        return FMSH_FAILURE;
    }    
    
    spi->caps.baudRate = sckdv;
    
    FMSH_WriteReg(spi->config.baseAddress, SPIPS_BAUDR_OFFSET, sckdv);  
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function sets fifo level. it can only be set when device is disabled
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
int FSpiPs_SetTxEmptyLvl(FSpiPs_T* spi, u8 tlvl)
{
    if(spi->isEnable == TRUE)
    {
        return FMSH_FAILURE;
    }    
    FMSH_WriteReg(spi->config.baseAddress, SPIPS_TXFTLR_OFFSET, tlvl);
    
    return FMSH_SUCCESS;
}

u32 FSpiPs_GetTxLevel(FSpiPs_T* spi)
{
    return FMSH_ReadReg(spi->config.baseAddress, SPIPS_TXFLR_OFFSET);
}

int FSpiPs_SetRxFullLvl(FSpiPs_T* spi, u8 tlvl)
{
    if(spi->isEnable == TRUE)
    {
        return FMSH_FAILURE;
    }    
    FMSH_WriteReg(spi->config.baseAddress, SPIPS_RXFTLR_OFFSET, tlvl);
    
    return FMSH_SUCCESS;
}

u32 FSpiPs_GetRxLevel(FSpiPs_T* spi)
{
    return FMSH_ReadReg(spi->config.baseAddress, SPIPS_RXFLR_OFFSET);
}

/*****************************************************************************
* This function enables/disables interrupt mask
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FSpiPs_EnableIntr( FSpiPs_T* spi, u32 mask)
{    
    u32 configReg;
    
    configReg = FMSH_ReadReg(spi->config.baseAddress, SPIPS_IMR_OFFSET);
    configReg |= mask;
    FMSH_WriteReg(spi->config.baseAddress, SPIPS_IMR_OFFSET, configReg);
}

void FSpiPs_DisableIntr( FSpiPs_T* spi, u32 mask)
{
    u32 configReg;
    
    configReg = FMSH_ReadReg(spi->config.baseAddress, SPIPS_IMR_OFFSET);
    configReg &= ~mask;
    FMSH_WriteReg(spi->config.baseAddress, SPIPS_IMR_OFFSET, configReg);
}

void FSpiPs_ClearIntrStatus(FSpiPs_T* spi)
{    
    FMSH_ReadReg(spi->config.baseAddress, SPIPS_ICR_OFFSET);
}

/*****************************************************************************
* This function sets DMA trigger level
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FSpiPs_SetDMATLvl(FSpiPs_T* spi, u32 tlvl)
{
    FMSH_WriteReg(spi->config.baseAddress, SPIPS_DMATDLR_OFFSET, tlvl);
}

void FSpiPs_SetDMARLvl(FSpiPs_T* spi, u32 tlvl)
{
    FMSH_WriteReg(spi->config.baseAddress, SPIPS_DMARDLR_OFFSET, tlvl);
}

void FSpiPs_EnableDMATx(FSpiPs_T* spi)
{
    FMSH_WriteReg(spi->config.baseAddress, SPIPS_DMACR_OFFSET, 0x2);
}

void FSpiPs_EnableDMARx(FSpiPs_T* spi)
{
    FMSH_WriteReg(spi->config.baseAddress, SPIPS_DMACR_OFFSET, 0x1);
}

void FSpiPs_DisableDMA(FSpiPs_T* spi)
{    
    FMSH_WriteReg(spi->config.baseAddress, SPIPS_DMACR_OFFSET, 0);
}

/*****************************************************************************
* This function gets status register value.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
u32 FSpiPs_GetStatus(FSpiPs_T* spi)
{  
    return FMSH_ReadReg(spi->config.baseAddress, SPIPS_SR_OFFSET);
}

/*****************************************************************************
* This function receives data from register(FIFO) if it is not empty. exit if 
* timeover
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
u32 FSpiPs_Recv(FSpiPs_T* spi )
{
    u32 count = 0;
    u8 status;
    
    status = FMSH_ReadReg(spi->config.baseAddress, SPIPS_SR_OFFSET); 
    while((status & SPIPS_SR_RFNE) == 0)  /* loop if RX fifo empty */
    {
        delay_us(1);
        count++;
        if(count > 10000)
        {
            break;
        }        
        status = FMSH_ReadReg(spi->config.baseAddress, SPIPS_SR_OFFSET); 
    }      

    return FMSH_ReadReg(spi->config.baseAddress, SPIPS_DR_OFFSET); 
}

/*****************************************************************************
* This function sends data to register(FIFO) if it is not full. exit if 
* timeover
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FSpiPs_Send(FSpiPs_T* spi, u32 Data )
{
    u32 count = 0;
    u8 status;
    
    status = FMSH_ReadReg(spi->config.baseAddress, SPIPS_SR_OFFSET); 
    while((status & SPIPS_SR_TFNF) == 0)   /* loop if TX fifo full */
    {
        delay_us(1);
        count++;
        if(count > 10000)
        {
            break;
        }
        status = FMSH_ReadReg(spi->config.baseAddress, SPIPS_SR_OFFSET); 
    }

    FMSH_WriteReg(spi->config.baseAddress, SPIPS_DR_OFFSET, Data);  
}
