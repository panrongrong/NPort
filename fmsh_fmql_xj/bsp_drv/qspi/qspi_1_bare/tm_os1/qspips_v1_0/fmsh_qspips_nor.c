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
* @file fmsh_qspips_nor.c
* @addtogroup qspips_v1_1
* @{
*
* Contains implements the spi nor flash interface functions of 
* the FQspiPs driver.
* See fmsh_qspips_nor.h for a detailed description of the device and driver.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who Date     Changes
* ----- --- -------- -----------------------------------------------
* 1.10  hzq 11/26/20 
* 		     First release
* 1.11  hzq 01/27/21
*            The FQspiPs_Nor_GetBankReg API has been added to help  
* 		     for flash reset if bank address is not 0.
* </pre>
*
******************************************************************************/

#include <string.h>
#include "fmsh_qspips_lib.h"

FQspiPs_Nor_Device qspi_nor_dev;

FQspiPs_Caps qspi_nor_caps_default = {
    /*
    .hasIntr = 1,
    .hasDma = 0,
    .hasXIP = 0,
    .txEmptyLvl = 32,
    .rxFullLvl = 480,
    .rdMode = QOR_CMD,
    .baudRate = 4, 
    .addressBytes = 3,
    */
    1,
    0,
    0,
    32,
    480,
    QOR_CMD,
    4, /* baudRate */
    0, /* sampleDelay */
    3  /* addressBytes */
};

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
int FQspiPs_Nor_Initr(FQspiPs_T* qspiPtr)
{
    return  FMSH_SUCCESS;
}

/*****************************************************************************
* This function chcek flash id & fill nor device struct
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
int FQspiPs_Nor_GetFlashInfo(FQspiPs_T* qspiPtr, u8* id)
{
    FQspiPs_Nor_Device* dev;
    
    qspiPtr->dev = &qspi_nor_dev;
    dev = (FQspiPs_Nor_Device*)qspiPtr->dev;
    
    /*flash make*/
    switch(id[0])
    {
    case SPI_MICRON_ID: 
    case SPI_SPANSION_ID: 
    case SPI_WINBOND_ID: 
    case SPI_MACRONIX_ID: 
    case SPI_ISSI_ID: 
    case SPI_FMSH_ID: 
    case SPI_GD_ID: 
        qspiPtr->maker = dev->maker = id[0];
        break;
    default: 
        qspiPtr->maker = dev->maker = SPI_UNKNOWN_ID; 
        break;
    }
    
    /*device size*/
    switch(id[2])
    {
    case QSPIFLASH_SIZE_ID_256K: 
        dev->deviceSize = qspiPtr->devSize = 0x1<<15; 
        break;
    case QSPIFLASH_SIZE_ID_512K: 
        dev->deviceSize = qspiPtr->devSize = 0x1<<16; 
        break;
    case QSPIFLASH_SIZE_ID_1M: 
        dev->deviceSize = qspiPtr->devSize = 0x1<<17; 
        break;
    case QSPIFLASH_SIZE_ID_2M: 
        dev->deviceSize = qspiPtr->devSize = 0x1<<18; 
        break;
    case QSPIFLASH_SIZE_ID_4M: 
        dev->deviceSize = qspiPtr->devSize = 0x1<<19; 
        break;
    case QSPIFLASH_SIZE_ID_8M: 
        dev->deviceSize = qspiPtr->devSize = 0x1<<20; 
        break;
    case QSPIFLASH_SIZE_ID_16M: 
        dev->deviceSize = qspiPtr->devSize = 0x1<<21; 
        break;
    case QSPIFLASH_SIZE_ID_32M: 
        dev->deviceSize = qspiPtr->devSize = 0x1<<22; 
        break;
    case QSPIFLASH_SIZE_ID_64M:
        dev->deviceSize = qspiPtr->devSize = 0x1<<23; 
        break;
    case QSPIFLASH_SIZE_ID_128M: 
        dev->deviceSize = qspiPtr->devSize = 0x1<<24; 
        break;
    case QSPIFLASH_SIZE_ID_256M: 
        dev->deviceSize = qspiPtr->devSize = 0x1<<25; 
        break;
    case QSPIFLASH_SIZE_ID_512M:
    case QSPIFLASH_SIZE_ID_512M_TYPE2: 
        dev->deviceSize = qspiPtr->devSize = 0x1<<26; 
        break;
    case QSPIFLASH_SIZE_ID_1G: 
    case QSPIFLASH_SIZE_ID_1G_TYPE2: 
        dev->deviceSize = qspiPtr->devSize = 0x1<<27; 
        break;
    default: 
        dev->deviceSize = qspiPtr->devSize = SPI_UNKNOWN_SIZE;
        break;
    }
    
    switch(id[0])
    {
    case SPI_MICRON_ID: 
        dev->blockSize = qspiPtr->sectorSize = 0x1 << 16;
        dev->pageSize = 0x1 << 8;
        
        if(qspiPtr->devSize >= QSPIFLASH_SIZE_256M)
        {
            FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_WCCR_OFFSET, 0x022770);
            qspiPtr->flag |= QSPI_FSR;
        }
        break;
    case SPI_SPANSION_ID:
        if(id[4] == 0x0)
        { /*256KB*/
            dev->blockSize = qspiPtr->sectorSize = 0x1 << 18;
        }
        else
        { /*64KB*/
            dev->blockSize = qspiPtr->sectorSize = 0x1 << 16;
        }
        dev->pageSize = 0x1 << 8;
        break;
    case SPI_WINBOND_ID: 
        dev->blockSize = qspiPtr->sectorSize = 0x1 << 16;
        dev->pageSize = 0x1 << 8;
        break;
    case SPI_MACRONIX_ID: 
        dev->blockSize = qspiPtr->sectorSize = 0x1 << 16;
        dev->pageSize = 0x1 << 8;
        break;
    case SPI_ISSI_ID: 
        dev->blockSize = qspiPtr->sectorSize = 0x1 << 16;
        dev->pageSize = 0x1 << 8;
        break;
    case SPI_FMSH_ID: 
        dev->blockSize = qspiPtr->sectorSize = 0x1 << 16;
        dev->pageSize = 0x1 << 8;
        break;
    case SPI_GD_ID: 
        dev->blockSize = qspiPtr->sectorSize = 0x1 << 16;
        dev->pageSize = 0x1 << 8;
        break;
    default: 
        dev->blockSize = qspiPtr->sectorSize = 0x1 << 16;
        dev->pageSize = 0x1 << 8;
        break;
    }

    return FMSH_SUCCESS;
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
int FQspiPs_Nor_SetFlashMode(FQspiPs_T* qspiPtr, u8 mode)
{
    u32 configReg;
    
    FMSH_ASSERT(qspiPtr != NULL);
    
    if(qspiPtr->caps.addressBytes == 3)
    {
    	switch(mode)
    	{
    	case READ_CMD: configReg = 0x00000003; break;
    	case FAST_READ_CMD: configReg = 0x0800000B; break;
    	case DOR_CMD: configReg = 0x0801003B; break;
    	case QOR_CMD: configReg = 0x0802006B; break;
    	case DIOR_CMD:   
        	if(qspiPtr->maker == SPI_MICRON_ID)
            	configReg = 0x041110BB;
        	else
            	configReg = 0x001110BB;   
        	break;
    	case QIOR_CMD:
        	if(qspiPtr->maker == SPI_MICRON_ID)
           	 configReg = 0x081220EB;
        	else
            	configReg = 0x041220EB;   
        	break;
    	default: configReg = 0x0800000B; break;
		}
        /*set program */
        FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_DWIR_OFFSET, 0x00000002);
        /*FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_DWIR_OFFSET, 0x00020032);*/
    }
    else if(qspiPtr->caps.addressBytes == 4)
    {
    	switch(mode)
    	{
    	case READ_CMD: configReg = 0x00000013; break;
    	case FAST_READ_CMD: configReg = 0x0800000C; break;
    	case DOR_CMD: configReg = 0x0801003C; break;
    	case QOR_CMD: configReg = 0x0802006C; break;
    	case DIOR_CMD:   
        	if(qspiPtr->maker == SPI_MICRON_ID)
            	configReg = 0x041110BC;
        	else
            	configReg = 0x001110BC;   
        	break;
    	case QIOR_CMD:
        	if(qspiPtr->maker == SPI_MICRON_ID)
           	 configReg = 0x081220EC;
        	else
            	configReg = 0x041220EC;   
        	break;
    	default: configReg = 0x0800000C; break;
		}
        /*set program */
        FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_DWIR_OFFSET, 0x00000012);
    }   
    else
    {
        return FMSH_FAILURE;
    }

    FQspiPs_SetAddrBytesNum(qspiPtr, qspiPtr->caps.addressBytes - 1); 
    FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_DRIR_OFFSET, configReg);

    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function erase flash
*
* @param
*       - offset is a value where sector erase start from
*       - byteCount is a number of bytes to sector erase
*       - blockSize is a value of flash sector bytes
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nor_Erase(FQspiPs_T* qspiPtr, u32 offset, u32 byteCount, u32 blockSize)
{
    int status = FMSH_SUCCESS;
    u32 sectorOffset;
    
    FMSH_ASSERT(qspiPtr != NULL);
    
    if(qspiPtr->isBusy == TRUE)
    {
        return QSPI_BUSY;
    }
    else
    {
        u32 sectorEnd = offset + byteCount;
        if(byteCount == 0)
        {
            return QSPI_PARAM_ERR; 
        }
        
        qspiPtr->isBusy = TRUE;
        
        if((blockSize != 0) && (blockSize != qspiPtr->sectorSize))
        {
            qspiPtr->sectorSize = blockSize;
        }
        /* calculate sector start address to be erased */
        sectorOffset = offset & ~(qspiPtr->sectorSize-1);
        /* Erase Flash with required sectors */
        while(sectorOffset < sectorEnd)
        {
            status = FQspiPs_Nor_SectorErase(qspiPtr, sectorOffset, 0);
            if(status != FMSH_SUCCESS)
            {
                break;
            }
            sectorOffset += qspiPtr->sectorSize;
        }
        
        qspiPtr->isBusy = FALSE;
    }
    
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
int FQspiPs_Nor_Write(FQspiPs_T* qspiPtr, u32 offset, u32 byteCount, u8 *sendBuffer)
{
    int status;
    u32 size, pageSize;
    u32 reg1, reg2;
    u32 address;
    FQspiPs_Nor_Device* dev;
    
    FMSH_ASSERT(qspiPtr != NULL);
    FMSH_ASSERT(byteCount != 0);
    FMSH_ASSERT(sendBuffer != NULL);
    
    /*diasble wel & auto_poll*/
    reg1 = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_WCCR_OFFSET);
    FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_WCCR_OFFSET, reg1 | 0x4000);
    reg2 = FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_DWIR_OFFSET);
    FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_DWIR_OFFSET, reg2 | 0x100);    
    
    dev = (FQspiPs_Nor_Device*)qspiPtr->dev;
    pageSize = dev->pageSize;
    
    while(byteCount > 0)
    {
        status = FQspiPs_Nor_WREN(qspiPtr) ;
        if(status)
            break;;
        
        if((offset & (pageSize - 1)) != 0)
            size = pageSize - (offset % pageSize);
        else
            size = pageSize;
        
        if(byteCount < size)
            size = byteCount;
        
        address = qspiPtr->config.dataBaseAddress + offset;
        /* Write to Flash  */
        memcpy((void*)address,
               (void*)sendBuffer,
               (size_t)size); 
        
        status = FQspiPs_Nor_WaitForReady(qspiPtr, 1500);
        if(status)
            break;
        
        byteCount -= size;
        offset += size;
        sendBuffer += size;
    }
    
    FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_WCCR_OFFSET, reg1);
    FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_DWIR_OFFSET, reg2);  
    
    return status;
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
int FQspiPs_Nor_Read(FQspiPs_T* qspiPtr, u32 offset, u32 byteCount, u8 *recvBuffer)
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
* This function reset flash
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
int FQspiPs_Nor_Reset(FQspiPs_T* qspiPtr)
{
    int err = 0;
    
    FMSH_ASSERT(qspiPtr != NULL);
    
    if(qspiPtr->maker == SPI_SPANSION_ID)
    {
        err = FQspiPs_CmdExecute(qspiPtr, 0xFF000001); 
        if(err != FMSH_SUCCESS) 
        {
            return FMSH_FAILURE;
        }
        err = FQspiPs_CmdExecute(qspiPtr, 0xF0000001); 
        if(err != FMSH_SUCCESS) 
        {
            return FMSH_FAILURE;
        }
    }
    else
    {
        err = FQspiPs_CmdExecute(qspiPtr, 0x66000001); 
        if(err != FMSH_SUCCESS) 
        {
            return FMSH_FAILURE;
        }
        err = FQspiPs_CmdExecute(qspiPtr, 0x99000001); 
        if(err != FMSH_SUCCESS) 
        {
            return FMSH_FAILURE;
        }
    }
    
    delay_ms(1);
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function check bank regiser
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
u8 FQspiPs_Nor_GetBankReg(FQspiPs_T* qspiPtr)
{
    u8 reg = 0;
    
    FMSH_ASSERT(qspiPtr != NULL);
    
    if(qspiPtr->devSize <= QSPIFLASH_SIZE_128M)
        return 0;
    
    if(qspiPtr->maker == SPI_SPANSION_ID)
    {
        reg = FQspiPs_Nor_GetReg8(qspiPtr, 0x16); 
    }
    else
    {
        reg = FQspiPs_Nor_GetReg8(qspiPtr, 0xC8);
    }
    
    return reg;
}

/*****************************************************************************
* This function enable flash quad io
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
int FQspiPs_Nor_EnableQuad(FQspiPs_T* qspi)
{
    int err;
    
    FMSH_ASSERT(qspi != NULL);

    /* config qspi flash registers (quad_en)*/
    if(qspi->maker == SPI_SPANSION_ID)
    {
        u8 status1 = FQspiPs_Nor_GetReg8(qspi, 0x05);
        u8 cfg = FQspiPs_Nor_GetReg8(qspi, 0x35);
        if((cfg & SPANSION_QUAD_EN_MASK) == 0)
        {
            cfg |= (u8)SPANSION_QUAD_EN_MASK;
            err = FQspiPs_Nor_SetReg16(qspi, 0x01, cfg, status1);/*0x01*/
            if(err)
            {
                return FMSH_FAILURE;
            }  
        }	
    }
    
    else if(qspi->maker == SPI_MICRON_ID)
    {
        /*no need to operte*/
    }
    
    else if(qspi->maker == SPI_MACRONIX_ID)
    {
        u8 status = FQspiPs_Nor_GetReg8(qspi, 0x05);
        if((status & MACRONIX_QUAD_EN_MASK) == 0)
        {
            status |= (u8)MACRONIX_QUAD_EN_MASK;
            err = FQspiPs_Nor_SetReg8(qspi, 0x01, status);
            if(err)
            {
                return FMSH_FAILURE;
            }  
        }  
    }
    
    else if(qspi->maker == SPI_ISSI_ID)
    {   
        u8 status = FQspiPs_Nor_GetReg8(qspi, 0x05); 
        if((status & ISSI_QUAD_EN_MASK) == 0)
        {
            status |= (u8)ISSI_QUAD_EN_MASK;
            err = FQspiPs_Nor_SetReg8(qspi, 0x01, status);
            if(err)
            {
                return FMSH_FAILURE;
            } 
        }    
    }
    
    else if(qspi->maker == SPI_WINBOND_ID)
    {
        u8 status1 = FQspiPs_Nor_GetReg8(qspi, 0x05);
        u8 status2 = FQspiPs_Nor_GetReg8(qspi, 0x35);   
        if((status2 & WINBOND_QUAD_EN_MASK) == 0)
        {
            status2 |= (u8)WINBOND_QUAD_EN_MASK;
            err = FQspiPs_Nor_SetReg16(qspi, 0x01, status2, status1);
            if(err)
            {
                return FMSH_FAILURE;
            }  
        }
    }
    
    else if(qspi->maker == SPI_FMSH_ID)
    {
        u8 status2 = FQspiPs_Nor_GetReg8(qspi, 0x35);
        if((status2 & FMSH_QUAD_EN_MASK) == 0)
        {
            status2 |= (u8)FMSH_QUAD_EN_MASK;
            err = FQspiPs_Nor_SetReg8(qspi, 0x31, status2);
            if(err)
            {
                return FMSH_FAILURE;
            }
        } 
    }
 
    else if(qspi->maker == SPI_GD_ID)
    {
        u8 status2 = FQspiPs_Nor_GetReg8(qspi, 0x35);
        if((status2 & GD_QUAD_EN_MASK) == 0)
        {
            status2 |= (u8)GD_QUAD_EN_MASK;
            err = FQspiPs_Nor_SetReg8(qspi, 0x31, status2);
            if(err)
            {
                return FMSH_FAILURE;
            }
        } 
    }
    
    else{
        /*do nothing*/
    }
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function lock flash protect
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
int FQspiPs_Nor_Lock(FQspiPs_T* qspi)
{
    int err = 0;
    
    FMSH_ASSERT(qspi != NULL);
       
    if(qspi->maker == SPI_SPANSION_ID)
    {
        u8 cfg = FQspiPs_Nor_GetReg8(qspi, 0x35);
        u8 status1 = FQspiPs_Nor_GetReg8(qspi, 0x05);
        status1 |= 0x1c;
        err = FQspiPs_Nor_SetReg16(qspi, 0x01, cfg, status1);    
    }
    if(qspi->maker == SPI_MICRON_ID)
    {
        u8 status1 = FQspiPs_Nor_GetReg8(qspi, 0x05);
        status1 |= 0x5c;
        err = FQspiPs_Nor_SetReg8(qspi, 0x01, status1);    
    }
    else if(qspi->maker == SPI_ISSI_ID ||
            qspi->maker == SPI_MACRONIX_ID)
    {
        u8 status1 = FQspiPs_Nor_GetReg8(qspi, 0x05);
        status1 &= 0x40;
        status1 |= 0x3c;
        err = FQspiPs_Nor_SetReg8(qspi, 0x01, status1);   
    }
    else
    {
        u8 status1 = FQspiPs_Nor_GetReg8(qspi, 0x05);
        status1 |= 0x1c;
        err = FQspiPs_Nor_SetReg8(qspi, 0x01, status1);  
    }

    if(err != FMSH_SUCCESS) 
    {
        return FMSH_FAILURE;
    }
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function unlock flash protect
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
int FQspiPs_Nor_Unlock(FQspiPs_T* qspi)
{
    int err = 0;
    
    FMSH_ASSERT(qspi != NULL);
       
    if(qspi->maker == SPI_SPANSION_ID)
    {
        u8 cfg = FQspiPs_Nor_GetReg8(qspi, 0x35);
        err = FQspiPs_Nor_SetReg16(qspi, 0x01, cfg, 0x00);    
    }
    else if(qspi->maker == SPI_ISSI_ID ||
            qspi->maker == SPI_MACRONIX_ID)
    {
        u8 status = FQspiPs_Nor_GetReg8(qspi, 0x05);
        status &= 0x40;
        err = FQspiPs_Nor_SetReg8(qspi, 0x01, status);   
    }
    else
    {
        err = FQspiPs_Nor_SetReg8(qspi, 0x01, 0x00);  
    }

    if(err != FMSH_SUCCESS) 
    {
        return FMSH_FAILURE;
    }
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function set flash segment
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
int FQspiPs_Nor_SetSegment(FQspiPs_T* qspi, u8 highAddr)
{
    int err;
    u8 extAddr, extAddrRb;
    
    FMSH_ASSERT(qspi != NULL);
    
    if((qspi->maker == SPI_SPANSION_ID) ||
       (qspi->maker == SPI_ISSI_ID))
    {        
        /* bit7 is extadd id 0 to enable 3B address */
        extAddr = highAddr & 0x7F;
        err = FQspiPs_Nor_SetReg8(qspi, 0x17, extAddr); 
        if(err != 0)
        {
            return FMSH_FAILURE;
        }
        
        /* readback & verify */
        extAddrRb = FQspiPs_Nor_GetReg8(qspi, 0x16); 
        if(extAddrRb != extAddr)
        {
            return FMSH_FAILURE;
        }       
    }
    
    else if((qspi->maker == SPI_MICRON_ID) ||
            (qspi->maker == SPI_MACRONIX_ID) ||
            (qspi->maker == SPI_WINBOND_ID) || 
            (qspi->maker == SPI_FMSH_ID) ||
            (qspi->maker == SPI_GD_ID))
    {        
        extAddr = highAddr;
        
        err = FQspiPs_Nor_SetReg8(qspi, 0xC5, extAddr);
        if(err != 0)
        {
            return FMSH_FAILURE;
        }
        
        /* readback & verify */
        extAddrRb = FQspiPs_Nor_GetReg8(qspi, 0xC8);
        if(extAddrRb != extAddr)
        {
            return FMSH_FAILURE;
        }   
    }

    else
    {
        return FMSH_FAILURE;
    }
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function enter flash into 4B mode
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
int FQspiPs_Nor_Enter4B(FQspiPs_T* qspiPtr)
{
    int error;
    u32 reg;

    error = FQspiPs_Nor_WREN(qspiPtr);
    if(error){
        return FMSH_FAILURE;
    }
    
    /*Send Enter 4B Command*/
    reg = 0xB7000000;
    error = FQspiPs_CmdExecute(qspiPtr, reg);
    if(error){
        return FMSH_FAILURE;
    }
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function exit flash from 4B mode
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
int FQspiPs_Nor_Exit4B(FQspiPs_T* qspiPtr)
{
    int error;
    u32 reg;

    error = FQspiPs_Nor_WREN(qspiPtr);
    if(error){
        return FMSH_FAILURE;
    }
    
    /*Send Enter 4B Command*/
    reg = 0xE9000000;
    error = FQspiPs_CmdExecute(qspiPtr, reg);
    if(error){
        return FMSH_FAILURE;
    }
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function enter flash into XIP mode
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
int FQspiPs_Nor_EnterXIP(FQspiPs_T* qspi, u8 cmd)
{
    u32 DRIR;
    
    FMSH_ASSERT(qspi != NULL);
    
    FQspiPs_DisableDAC(qspi);
    
    if((qspi->maker == SPI_SPANSION_ID) ||
       (qspi->maker == SPI_ISSI_ID) || 
       (qspi->maker == SPI_MACRONIX_ID)) 
    {
        FQspiPs_SetModeBits(qspi, 0xA5);
        if(cmd == DIOR_CMD)
        {
            DRIR = 0x001110BB;    
        }
        else if(cmd == QIOR_CMD)
        {
            DRIR = 0x041220EB;
        }
    }
    
    else if((qspi->maker == SPI_WINBOND_ID) ||
            (qspi->maker == SPI_FMSH_ID) || 
            (qspi->maker == SPI_GD_ID)) 
    {
        FQspiPs_SetModeBits(qspi, 0x20);
        if(cmd == DIOR_CMD)
        {
            DRIR = 0x001110BB;    
        }
        else if(cmd == QIOR_CMD)
        {
            DRIR = 0x041220EB;
        }
    }
    
    else if(qspi->maker == SPI_MICRON_ID)
    {
        FQspiPs_SetModeBits(qspi, 0x00);     
        if(cmd == DIOR_CMD)
        {
            DRIR = 0x041110BB;    
        }
        else if(cmd == QIOR_CMD)
        {
            DRIR = 0x081220EB;
        }
        u8 volatile_cfg = FQspiPs_Nor_GetReg8(qspi, MICRON_RDVCR_CMD);
        FQspiPs_Nor_SetReg8(qspi, MICRON_WRVCR_CMD, volatile_cfg & 0xf3);
    } 
    
    else
    {
        /*
        FQspiPs_SetModeBits(qspi, 0x20);
        if(cmd == DIOR_CMD)
        {
            DRIR = 0x001110BB;    
        }
        else if(cmd == QIOR_CMD)
        {
            DRIR = 0x041220EB;
        }*/
        FQspiPs_EnableDAC(qspi);
        return FMSH_SUCCESS;
    }
    
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_DRIR_OFFSET, DRIR);
    
    FQspiPs_EnableXip(qspi);
    FQspiPs_EnableDAC(qspi);
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function executes SECTOR ERASE.
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
int FQspiPs_Nor_SectorErase(FQspiPs_T* qspiPtr, u32 start_offset, u8 cmd)
{
    int error;
    u32 reg;
    
    /*Set Flash Command Address*/
    reg = start_offset;
    FMSH_WriteReg(qspiPtr->config.baseAddress, QSPIPS_FCAR_OFFSET, reg);
    
    error = FQspiPs_Nor_WREN(qspiPtr);
    if(error){
        return FMSH_FAILURE;
    }
    
    /*Send Erase Command*/
    if(qspiPtr->caps.addressBytes == 3)
        reg = 0xD80A0000;
    else if(qspiPtr->caps.addressBytes == 4)
        reg = 0xDC0B0000;
    error = FQspiPs_CmdExecute(qspiPtr, reg);
    if(error){
        return FMSH_FAILURE;
    }
    
    error = FQspiPs_Nor_WaitForReady(qspiPtr, 5000);
    if(error)
        return FMSH_FAILURE;
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function executes CHIP ERASE.
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
int FQspiPs_Nor_ChipErase(FQspiPs_T* qspiPtr, u8 cmd)
{
    int error;
    u32 reg;
    
    error = FQspiPs_Nor_WREN(qspiPtr);
    if(error){
        return FMSH_FAILURE;
    }
    
    /*Send Erase Command*/
    reg = ((u32)cmd << 24);
    error = FQspiPs_CmdExecute(qspiPtr, reg);
    if(error){
        return FMSH_FAILURE;
    }
    
    error = FQspiPs_Nor_WaitForReady(qspiPtr, 600000);
    if(error)
        return FMSH_FAILURE;

    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function executes READ STATUS1.
*
* @param
*
* @return
*		- Status value.
*
* @note		
*
******************************************************************************/
u8 FQspiPs_Nor_GetStatus1(FQspiPs_T* qspiPtr)
{
    /*Send Read Status1 Register command(0x05) to device*/
    FQspiPs_CmdExecute(qspiPtr, 0x05800000);
    return FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_FCRDRL_OFFSET) & 0xff;
} 

u8 FQspiPs_Nor_GetFlagStatus(FQspiPs_T* qspiPtr)
{
    /*Send Read Flag Status Register command(0x70) to device*/
    FQspiPs_CmdExecute(qspiPtr, 0x70800000);
    return FMSH_ReadReg(qspiPtr->config.baseAddress, QSPIPS_FCRDRL_OFFSET) & 0xff;
} 

void FQspiPs_Nor_ClearFlagStatus(FQspiPs_T* qspiPtr)
{
    /*Send Read Flag Status Register command(0x70) to device*/
    FQspiPs_CmdExecute(qspiPtr, 0x50000000);
} 

/*****************************************************************************
* This function executes READ 16 bits register.
*
* @param
*
* @return
*		- Register value.
*
* @note		
*
******************************************************************************/
u16 FQspiPs_Nor_GetReg16(FQspiPs_T *qspi, u8 cmd)
{
    u32 reg;
    u16 value;
    
    reg = 0x00900000 | (cmd << 24);
    /*Read Register command from device (2B)*/
    FQspiPs_CmdExecute(qspi, reg);    
    value = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_FCRDRL_OFFSET) & 0xffff;
    return value;
} 

/*****************************************************************************
* This function executes READ 8 bits register.
*
* @param
*
* @return
*		- Register value.
*
* @note		
*
******************************************************************************/
u8 FQspiPs_Nor_GetReg8(FQspiPs_T *qspi, u8 cmd)
{
    u32 reg;
    u8 value;
    
    reg = 0x00800000 | (cmd << 24);
    /*Read Register command from device (1B)*/
    FQspiPs_CmdExecute(qspi, reg);    
    value = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_FCRDRL_OFFSET) & 0xff;
    return value;
} 

/*****************************************************************************
* This function executes WRITE 16 bits register.
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
int FQspiPs_Nor_SetReg16(FQspiPs_T* qspi, u8 cmd, u8 high_value, u8 low_value)
{
    int error;
    u32 reg;
    
    reg = 0x00009000 | (cmd << 24);
    /* Set value to be sent*/
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_FCWDRL_OFFSET, ((high_value << 8) | low_value));  
    error = FQspiPs_Nor_WREN(qspi);
    if(error){
        return FMSH_FAILURE;
    }
    /*Send command to device*/
    FQspiPs_CmdExecute(qspi, reg);
    if(error){
        return FMSH_FAILURE;
    }
    
    error = FQspiPs_Nor_WaitForReady(qspi, 1500);
    if(error){
        return FMSH_FAILURE;
    }
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function executes WRITE 8 bits register.
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
int FQspiPs_Nor_SetReg8(FQspiPs_T* qspi, u8 cmd, u8 value)
{
    int error;
    u32 reg;
    
    reg = 0x00008000 | (cmd << 24);
    /* Set value to be sent*/
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_FCWDRL_OFFSET, (u32)value);
    error = FQspiPs_Nor_WREN(qspi);
    if(error){
        return FMSH_FAILURE;
    }
    
    /*Send command to device*/
    FQspiPs_CmdExecute(qspi, reg);
    
    error = FQspiPs_Nor_WaitForReady(qspi, 1500);
    if(error){
        return FMSH_FAILURE;
    }
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function executes READ STATUS1 and wait for WIP.
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if overtime.
*
* @note		
*
******************************************************************************/
int FQspiPs_Nor_WaitForWIP(FQspiPs_T* qspiPtr, u32 expire)
{ 
    u8 status;
    u32 timeout = 0;
    
    /* Poll Status Register1*/
    while(1){
        status = FQspiPs_Nor_GetStatus1(qspiPtr);
        status = ~status;
        status &= BUSY_MASK;
        if(status == BUSY_MASK){
            return FMSH_SUCCESS;
        }    
        delay_1ms();
        timeout++;
        if(timeout > expire){
            return FMSH_FAILURE;
        }
    }
}

int FQspiPs_Nor_WaitForFSR(FQspiPs_T* qspiPtr, u32 expire)
{ 
    u8 status;
    int det = 2;
    u32 timeout = 0;
    
    while(1){
        status = FQspiPs_Nor_GetFlagStatus(qspiPtr);
        if((status & 0x80)== 0x80){
            det--;
            if(det == 0)
                return FMSH_SUCCESS;
        }    
        delay_1ms();
        timeout++;
        if(timeout > expire){
            return FMSH_FAILURE;
        }
    }
}

int FQspiPs_Nor_WaitForReady(FQspiPs_T* qspiPtr, u32 expire)
{ 
    u8 status;
    
    /* Poll Status Register1*/
    status = FQspiPs_Nor_WaitForWIP(qspiPtr, expire);
    if(status)
        return FMSH_FAILURE;
    
    /* Poll Flag Status Register*/
    if(qspiPtr->flag & QSPI_FSR)
    {
        status = FQspiPs_Nor_WaitForFSR(qspiPtr, expire);
        if(status)
            return FMSH_FAILURE;
        
        status = FQspiPs_Nor_GetFlagStatus(qspiPtr);
        if(status & 0x32)
        {
            FQspiPs_Nor_ClearFlagStatus(qspiPtr);
            return FMSH_FAILURE;
        }
    }
    
    return FMSH_SUCCESS;
}
/*****************************************************************************
* This function executes WREN.
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

int FQspiPs_Nor_WREN(FQspiPs_T* qspiPtr) 
{
    int status = 0;
    int timeout = 0;
	
    /* Send WREN(0x06) Command*/
    status = FQspiPs_CmdExecute(qspiPtr, 0x06000000);    
    if(status){
        return FMSH_FAILURE;
    }

    while(1){
        status = FQspiPs_Nor_GetStatus1(qspiPtr);
        if((status & 0x2) == 0x2){
            return FMSH_SUCCESS;
        }    
        delay_1ms();
        timeout++;
        if(timeout > 1500){
            return FMSH_FAILURE;
        }
    }
}

/*****************************************************************************
* This function executes WRDI.
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
int FQspiPs_Nor_WRDI(FQspiPs_T* qspiPtr)
{ 
    int status = 0;
    int timeout = 0;
    /* Send WREN(0x04) Command*/
    status = FQspiPs_CmdExecute(qspiPtr, 0x04000000);
    if(status){
        return FMSH_FAILURE;
    }
    
    while(1){
        status = FQspiPs_Nor_GetStatus1(qspiPtr);
        if((status & 0x2) == 0x0){
            return FMSH_SUCCESS;
        }    
        delay_1ms();
        timeout++;
        if(timeout > 1500){
            return FMSH_FAILURE;
        }
    }
}
