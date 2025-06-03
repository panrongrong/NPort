#include <vxWorks.h>
#include <intLib.h>
#include <logLib.h>
#include <muxLib.h>
#include <semLib.h>
#include <sysLib.h>
#include <vxBusLib.h>
#include <wdLib.h>
#include <string.h>

/* #include <string.h> */
#include "fmsh_qspips_lib.h"

/**************************
* qspi struct Initialize
*
***************/
int FQspiPs_Initialize(FQspiPs_T* qspi, u16 deviceId)
{
    int err = 0;
    
    FMSH_ASSERT(qspi != NULL);
    
    FQspiPs_Config_T* cfgPtr;
    cfgPtr = FQspiPs_LookupConfig(deviceId);
    if (cfgPtr == NULL) 
    {
        return FMSH_FAILURE;
    }
    err = FQspiPs_CfgInitialize(qspi, cfgPtr);  
    if(err != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;
    }
    
    /* reset controller */
    FQspiPs_Reset(qspi); 

	
    /* get maker & device size */
    err = FQspiPs_GetFlashInfo(qspi);
    if(err != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;
    } 	  
    /*set transfer mode */
    err = FQspiPs_SetFlashMode(qspi, FAST_READ_CMD); 
    if(err != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;
    } 	
	
    /* set baud rate at 4 */
    FQspiPs_SetBaudRate(qspi, QSPIPS_BAUD(4));
	/* FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_RDCR_OFFSET, 0x5); */
    
    return FMSH_SUCCESS;
}

int FQspiPs_Initialize_1(FQspiPs_T* qspi, u16 deviceId)
{
    int err = 0;
    
    FMSH_ASSERT(qspi != NULL);
    
    FQspiPs_Config_T* cfgPtr;
    cfgPtr = FQspiPs_LookupConfig(deviceId);
    if (cfgPtr == NULL) 
    {
        return FMSH_FAILURE;
    }
    err = FQspiPs_CfgInitialize(qspi, cfgPtr);  
    if(err != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;
    }
    
    /* reset controller */
   /* FQspiPs_Reset(qspi);  */

    
    return FMSH_SUCCESS;
}

int FQspiPs_Initialize_2(FQspiPs_T* qspi, u16 deviceId)
{
    int err = 0;
    	
    /* 
    get maker & device size 
	*/
    err = FQspiPs_GetFlashInfo(qspi);
    if(err != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;
    } 	  
	
#if 0
	/*
    set transfer mode 
    */
    err = FQspiPs_SetFlashMode(qspi, FAST_READ_CMD);
#else	
	/*
	* set transfer mode 
	*/
	err = FQspiPs_SetFlashMode(qspi, QOR_CMD); 
#endif

    if(err != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;
    } 	
	
    /* 
    set baud rate at 4 
    SPI baud rate = (master reference clock) / BD => BD/2 - 1
    BD = 2^n + 2: 0 - 2; 1 - 4; 2 - 6 ... 
    */
    FQspiPs_SetBaudRate(qspi, QSPIPS_BAUD(4));
    /* FQspiPs_SetBaudRate(qspi, QSPIPS_BAUD(16)); */
    
	/* FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_RDCR_OFFSET, 0x5); */
    
    return FMSH_SUCCESS;
}
/*******************************
* check flash info
*
***************/
int FQspiPs_GetFlashInfo(FQspiPs_T* qspi)
{  
    u32 flashId;
    u8 maker, devSize;
	int ret = FMSH_SUCCESS;
    
    FMSH_ASSERT(qspi != NULL);
    
    /* Read ID in Auto mode. */
    flashId = FQspiFlash_ReadId(qspi);

	
	
    maker = flashId & 0xff;
    devSize = (flashId >> 16) & 0xff;
    
    /* Deduce flash make   */
	switch (maker)
	{
	case SPI_MICRON_ID:
	case SPI_SPANSION_ID:
	case SPI_WINBOND_ID:
	case SPI_MACRONIX_ID:
	case SPI_ISSI_ID:
	case SPI_FMSH_ID:
		qspi->maker = maker;
		break;
	
	default:
        qspi->maker = SPI_WRONG_ID;
        ret = FMSH_FAILURE;
		break;
	}
    
    /* Deduce flash Size */
    switch (devSize)
    {
   	case QSPIFLASH_SIZE_ID_256K: 
        qspi->devSize = QSPIFLASH_SIZE_256K;
		break;
    case QSPIFLASH_SIZE_ID_512K: 
        qspi->devSize = QSPIFLASH_SIZE_512K;
		break;
    case QSPIFLASH_SIZE_ID_1M:
        qspi->devSize = QSPIFLASH_SIZE_1M;
		break;
    case QSPIFLASH_SIZE_ID_2M: 
        qspi->devSize = QSPIFLASH_SIZE_2M;
		break;
    case QSPIFLASH_SIZE_ID_4M: 
        qspi->devSize = QSPIFLASH_SIZE_4M;
		break;
    case QSPIFLASH_SIZE_ID_8M:
        qspi->devSize = QSPIFLASH_SIZE_8M;
		break;
    case QSPIFLASH_SIZE_ID_16M:
        qspi->devSize = QSPIFLASH_SIZE_16M;
		break;
    case QSPIFLASH_SIZE_ID_32M:
        qspi->devSize = QSPIFLASH_SIZE_32M;
		break;
    case QSPIFLASH_SIZE_ID_64M: 
        qspi->devSize = QSPIFLASH_SIZE_64M;
		break;
    case QSPIFLASH_SIZE_ID_128M:
        qspi->devSize = QSPIFLASH_SIZE_128M;
		break;
    case QSPIFLASH_SIZE_ID_256M: 
        qspi->devSize = QSPIFLASH_SIZE_256M;
		break;

    case QSPIFLASH_SIZE_ID_512M:
    case MACRONIX_QSPIFLASH_SIZE_ID_512M: /* ISSI_QSPIFLASH_SIZE_ID_512M */
        qspi->devSize = QSPIFLASH_SIZE_512M;
		break;

    case QSPIFLASH_SIZE_ID_1G:
    case MACRONIX_QSPIFLASH_SIZE_ID_1G: 
        qspi->devSize = QSPIFLASH_SIZE_1G;
		break;

	default:
        qspi->devSize = SPI_WRONG_ID;
        ret = FMSH_FAILURE;
		break;
    }
    
    return ret;
}

/********************************
* set flash(x1, x2, x4) mode and timing
*
****************/
int FQspiPs_SetFlashMode(FQspiPs_T* qspi, u8 cmd)
{
    int err;
    u32 DRIR;
    u8 ioWidth, status;
	int ret = FMSH_SUCCESS;	
    u8 has_changed = 0, status1, status2, cfg;
	u8 volatile_cfg, enhanced_cfg, extended_addr;
	u8 read, bank_address;
	
    FMSH_ASSERT(qspi != NULL);

	switch (cmd)
	{
    case READ_CMD:
        ioWidth = 0x1;
        DRIR = 0x00000003;
		break;
		
    case FAST_READ_CMD:
        ioWidth = 0x1;
        DRIR = 0x0800000B;
		break;
		
    case DOR_CMD:
        ioWidth = 0x2;
        DRIR = 0x0801003B;
		break;
		
    case QOR_CMD:  /* now working */ 
        ioWidth = 0x4;
        DRIR = 0x0802006B;
		break;
		
    case DIOR_CMD:
        ioWidth = 0x2;
        if(qspi->maker == SPI_MICRON_ID)
        {
            DRIR = 0x080110BB;
        }
        else
        {
            DRIR = 0x040110BB;   
        }
		break;
		
    case QIOR_CMD:
        ioWidth = 0x4;
        if(qspi->maker == SPI_MICRON_ID)
        {
            DRIR = 0x0A0220EB;
        }
        else
        {
            DRIR = 0x041220EB;   
        }
		break;
	
    default:
        ret = FMSH_FAILURE;
		break;
	}
    
    /* config qspi flash registers (dummy & quad_en) */
    switch (qspi->maker)
    {    	
    case SPI_SPANSION_ID:
        has_changed = 0;        
        status1 = FQspiFlash_GetReg8(qspi, RDSR1_CMD); /*0x05 */
        cfg = FQspiFlash_GetReg8(qspi, SPANSION_RDCR_CMD); /*0x35 */
        /* check LC bits for dummy parameter */
        if((cfg & SPANSION_LC_MASK) != 0x00)
        {
            cfg = (cfg & ~(u8)SPANSION_LC_MASK) | 0x00;
            has_changed = 1;
        }        
        /* check quad_en bit */
        if(ioWidth == 0x4)
        {
            if((cfg & SPANSION_QUAD_EN_MASK) == 0x00)
            { /* if not quad_en    */
                cfg |= (u8)SPANSION_QUAD_EN_MASK;
                has_changed = 1;
            }
        }
        else
        {
            if(cfg & SPANSION_QUAD_EN_MASK)
            { /* if quad_en    */
                cfg &= ~(u8)SPANSION_QUAD_EN_MASK;
                has_changed = 1;
            }
        }        
        if(has_changed)
        {
            err = FQspiFlash_SetReg16(qspi, WRR_CMD, cfg, status1);
            if(err)
            {
                ret = FMSH_FAILURE;
            }
        }
		break;
    
    case SPI_MICRON_ID:
        has_changed = 0;        
        cfg = FQspiFlash_GetReg16(qspi, MICRON_RDNVCR_CMD); /*0xB5 */
        /* check nv cfg, including dummy cycle... */
        if(cfg != 0xFF)
        {
            volatile_cfg = 0xfb; /*assure dummy cycle is default */
            enhanced_cfg = 0xff; /*assure extended-spi mode */
            extended_addr = 0x00; /*assure lowest 128Mb */
            has_changed = 1;
        }
        if (has_changed)
        {
            err = FQspiFlash_SetReg8(qspi, MICRON_WRVCR_CMD, volatile_cfg);
            if(err)
            {
                ret = FMSH_FAILURE;
            }
            err = FQspiFlash_SetReg8(qspi, MICRON_WRECR_CMD, enhanced_cfg);
            if(err)
            {
                ret = FMSH_FAILURE;
            }
            err = FQspiFlash_SetReg8(qspi, MICRON_WREAR_CMD, extended_addr);
            if(err)
            {
                ret = FMSH_FAILURE;
            }
        }
		break;
    
    case SPI_WINBOND_ID:
        has_changed = 0;
        status1 = FQspiFlash_GetReg8(qspi, RDSR1_CMD); /*0x05 */
        status2 = FQspiFlash_GetReg8(qspi, WINBOND_RDSR2_CMD); /*0x35      */
        /* check quad_en bit, quad mode 1, others 0; */
        if (ioWidth == 0x4)
        {
            if((status2 & WINBOND_QUAD_EN_MASK) == 0x00)
            {
                status2 |= (u8)WINBOND_QUAD_EN_MASK;
                has_changed = 1;
            }
        }
        else
        {
            if((status2 & WINBOND_QUAD_EN_MASK) != 0x00)
            {
                status2 &= ~(u8)WINBOND_QUAD_EN_MASK;
                has_changed = 1;
            }
        }
        if (has_changed)
        {
            err = FQspiFlash_SetReg16(qspi, WRR_CMD, status2, status1);
            if(err)
            {
                ret = FMSH_FAILURE;
            }  
        }
		break;
    
    case SPI_MACRONIX_ID:
        has_changed = 0;
        status = FQspiFlash_GetReg8(qspi, RDSR1_CMD); /*0x05 */
        cfg = FQspiFlash_GetReg8(qspi, MACRONIX_RDCR_CMD); /*0x15 */
        /*check dummy  */
        if((cfg & MACRONIX_DC_MASK) != 0x00)
        {
            cfg = (cfg & ~(u8)MACRONIX_DC_MASK) | 0x00;
            has_changed = 1;
        }
        /* check quad_en */
        if(ioWidth == 0x4)
        {
            if((status & MACRONIX_QUAD_EN_MASK) == 0x00)
            {
                status |= (u8)MACRONIX_QUAD_EN_MASK;
                has_changed = 1;
            }
        }
        else
        {
            if((status & MACRONIX_QUAD_EN_MASK) != 0x00)
            {
                status &= ~(u8)MACRONIX_QUAD_EN_MASK;
                has_changed = 1;
            }
        }
        if(has_changed)
        {
            err = FQspiFlash_SetReg16(qspi, WRR_CMD, cfg, status);
            if(err)
            {
                ret = FMSH_FAILURE;
            }   
        } 
		break;
    
    case SPI_ISSI_ID:
        has_changed = 0;
        status = FQspiFlash_GetReg8(qspi, 0x05); /* RDSR1_CMD */
        read = FQspiFlash_GetReg8(qspi, 0x61); /* ISSI_RDRP_CMD */
        bank_address = FQspiFlash_GetReg8(qspi, 0x16); /* ISSI_RDBR_CMD */
        /* check dummy cycle */
        if ((read & ISSI_DUMMY_MASK) != 0x00)
        {
            read = (read & ~(u8)ISSI_DUMMY_MASK) | 0x00;
            has_changed |= 0x1;
        }
        /* check quad_en */
        if (ioWidth == 0x4)
		{
            if((status & ISSI_QUAD_EN_MASK) == 0x00)
            {
                status |= (u8)ISSI_QUAD_EN_MASK;
                has_changed |= 0x2;
            }
        }
        else
		{
            if((status & ISSI_QUAD_EN_MASK) != 0x00)
            {
                status &= ~(u8)ISSI_QUAD_EN_MASK;
                has_changed |= 0x2;
            }
        }
        /* extended bank address */
        if (bank_address != 0x00)
        {
            bank_address = 0x00;
            has_changed |= 0x4;
        }
        if (has_changed)
		{
            if(has_changed & 0x1)
            {
                err = FQspiFlash_SetReg8(qspi, 0xC0, read); /* ISSI_SPRV_CMD */
                if(err)
                {
                    return FMSH_FAILURE;
                } 
            }
            if(has_changed & 0x2)
            {
                err = FQspiFlash_SetReg8(qspi, 0x01, status); /* WRR_CMD */
                if(err){
                    return FMSH_FAILURE;
                } 
            }
            if(has_changed & 0x4)
            {
                err = FQspiFlash_SetReg8(qspi, 0x17, bank_address); /*ISSI_WRBRV_CMD */
                if(err)
                {
                    ret = FMSH_FAILURE;
                } 
            }
        }
		break;
    
    case SPI_FMSH_ID:
        has_changed = 0;        
        status1 = FQspiFlash_GetReg8(qspi, RDSR1_CMD); /*0x05 */
        status2 = FQspiFlash_GetReg8(qspi, FMSH_RDSR2_CMD); /*0x35 */
        /* check quad_en bit, quad mode 1, others 0; */
        if (ioWidth == 0x4)
		{
            if((status2 & FMSH_QUAD_EN_MASK) == 0x00)
            {
                status2 |= (u8)FMSH_QUAD_EN_MASK;
                has_changed = 1;
            }
        }
        else
        {
            if((status2 & FMSH_QUAD_EN_MASK) != 0x00)
            {
                status2 &= ~(u8)FMSH_QUAD_EN_MASK;
                has_changed = 1;
            }
        }        
        if(has_changed)
        {
            err = FQspiFlash_SetReg16(qspi, WRR_CMD, status2, status1);
            if(err)
            {
                ret = FMSH_FAILURE;
            }  
        }
		break;
    
    default:
        ret = FMSH_FAILURE;
		break;
    }
    
    FQspiPs_WaitIdle(qspi);   
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_DRIR_OFFSET, DRIR);
    
    return ret;
}


/****************************
* qspi flash reset
*
***************/
int FQspiPs_ResetFlash(FQspiPs_T* qspi)
{
    int err = 0;
    
    FMSH_ASSERT(qspi != NULL);
    
    if(qspi->maker == SPI_SPANSION_ID)
    {
        err = FQspiPs_CmdExecute(qspi, 0xFF000001); 
        if(err != FMSH_SUCCESS) 
        {
            return FMSH_FAILURE;
        }
    }
    else if(qspi->maker == SPI_MICRON_ID || 
            qspi->maker == SPI_MACRONIX_ID ||
                qspi->maker == SPI_ISSI_ID ||
                    qspi->maker == SPI_WINBOND_ID ||        
                        qspi->maker == SPI_FMSH_ID     )
    {
        
        err = FQspiPs_CmdExecute(qspi, 0x66000001); 
        if(err != FMSH_SUCCESS) 
        {
            return FMSH_FAILURE;
        }
        err = FQspiPs_CmdExecute(qspi, 0x99000001); 
        if(err != FMSH_SUCCESS) 
        {
            return FMSH_FAILURE;
        }
    }
    else
    {
        return FMSH_FAILURE; 
    }
    
    err = FQspiFlash_WaitForWIP(qspi);  
    if(err != FMSH_SUCCESS) 
    {
        return FMSH_FAILURE;
    }
    return FMSH_SUCCESS;
}

/*******************************
* qspi flash disable protect
*
**********************/
int FQspiPs_UnlockFlash(FQspiPs_T* qspi)
{
    int err = 0;
    
    FMSH_ASSERT(qspi != NULL);
    
    if (qspi->maker == SPI_SPANSION_ID)
	{
        u32 config = FQspiFlash_GetReg8(qspi, SPANSION_RDCR_CMD);
        FQspiFlash_SetReg16(qspi, WRR_CMD, config, 0x00);
    }
    else if (qspi->maker == SPI_ISSI_ID)
	{
        u8 status = FQspiFlash_GetReg8(qspi, 0x05); /* RDSR1_CMD */
        status &= 0x40;
        FQspiFlash_SetReg8(qspi, 0x01, status);   
    }
    else if (qspi->maker == SPI_FMSH_ID)
	{
        u32 config = FQspiFlash_GetReg8(qspi, FMSH_RDSR2_CMD);
        FQspiFlash_SetReg16(qspi, WRR_CMD, config, 0x00);  
    }
    else if (qspi->maker == SPI_MICRON_ID || 
             qspi->maker == SPI_MACRONIX_ID ||
             qspi->maker == SPI_WINBOND_ID)
    {
    	/* nothing*/
    }
    else
	{
        return FMSH_FAILURE; 
    }
    
    err = FQspiFlash_WaitForWIP(qspi);  
    if (err != FMSH_SUCCESS) 
	{
        return FMSH_FAILURE;
    }
	
    return FMSH_SUCCESS;
}

/******************************
* remap high address data to 16M AHB area
*
******************/
int FQspiPs_SetFlashSegment(FQspiPs_T* qspi, u8 highAddr)
{
    int err;
    u8 extAddr, extAddrRb;
    
    FMSH_ASSERT(qspi != NULL);
    
    if(qspi->maker == SPI_SPANSION_ID)
    {
        /* check if 3B address mode, return err if yes*/
        u8 bankReg = FQspiFlash_GetReg8(qspi, SPANSION_BRWR_CMD); 
        if((bankReg & SPANSION_EXTADDR_MASK) == SPANSION_EXTADDR_MASK)
        {
            return FMSH_FAILURE;
        }
        
        /* bit7 is extadd id 0 to enable 3B address */
        extAddr = highAddr & 0x7F;
        err = FQspiFlash_SetReg8(qspi, SPANSION_BRWR_CMD, extAddr); 
        if(err != 0)
        {
            return FMSH_FAILURE;
        }
        
        /* readback & verify */
        extAddrRb = FQspiFlash_GetReg8(qspi, SPANSION_BRRD_CMD); 
        if(extAddrRb != extAddr)
        {
            return FMSH_FAILURE;
        }       
    }
    else if(qspi->maker == SPI_MICRON_ID)
    {
        /* check if 3B address mode, return err if yes*/
        u16 nvCfg = FQspiFlash_GetReg16(qspi, MICRON_RDNVCR_CMD);
        if((nvCfg & MICRON_EXTADDR_MASK) != MICRON_EXTADDR_MASK)
        {           
            return FMSH_FAILURE;
        }
        
        extAddr = highAddr;
        err = FQspiFlash_SetReg8(qspi, MICRON_WREAR_CMD, extAddr); /* 0xC5 */
        if(err != 0)
        {
            return FMSH_FAILURE;
        }
        
        /* readback & verify */
        extAddrRb = FQspiFlash_GetReg8(qspi, MICRON_RDEAR_CMD); /*0xC8 */
        if(extAddrRb != extAddr)
        {
            return FMSH_FAILURE;
        }
    }
    else if(qspi->maker == SPI_WINBOND_ID)
    {
        /* check if 3B address mode, return err if yes*/
        u8 status3 = FQspiFlash_GetReg8(qspi, WINBOND_RDSR3_CMD);
        if((status3 & WINBOND_ADS_MASK) == WINBOND_ADS_MASK)
        {           
            return FMSH_FAILURE;
        }
        
        extAddr = highAddr;
        
        err = FQspiFlash_SetReg8(qspi, WINBOND_WREAR_CMD, extAddr); /* 0xC5 */
        if(err != 0)
        {
            return FMSH_FAILURE;
        }
        
        /* readback & verify */
        extAddrRb = FQspiFlash_GetReg8(qspi, WINBOND_RDEAR_CMD); /*0xC8 */
        if(extAddrRb != extAddr)
        {
            return FMSH_FAILURE;
        }   
    }
    else if(qspi->maker == SPI_MACRONIX_ID)
    {
        /* check if 3B address mode */
        u8 cfg = FQspiFlash_GetReg8(qspi, MACRONIX_RDCR_CMD);
        if((cfg & MACRONIX_4B_MASK) == MACRONIX_4B_MASK){
            return FMSH_FAILURE;            
        }
        
        extAddr = highAddr;
        err = FQspiFlash_SetReg8(qspi, MACRONIX_WREAR_CMD, extAddr); /* 0xC5 */
        if(err != 0)
        {
            return FMSH_FAILURE;
        }
        
        /* readback & verify */
        extAddrRb = FQspiFlash_GetReg8(qspi, MACRONIX_RDEAR_CMD); /*0xC8 */
        if(extAddrRb != extAddr)
        {
            return FMSH_FAILURE;
        } 
    }
    else if(qspi->maker == SPI_ISSI_ID)
    {
        /* check if 3B address mode, return err if yes*/
        u8 bankReg = FQspiFlash_GetReg8(qspi, ISSI_RDBR_CMD); 
        if((bankReg & ISSI_EXTADDR_MASK) == ISSI_EXTADDR_MASK)
        {
            return FMSH_FAILURE;
        }
        
        /* bit7 is extadd id 0 to enable 3B address */
        extAddr = highAddr & 0x7F;
        err = FQspiFlash_SetReg8(qspi, ISSI_WRBRV_CMD, extAddr); /* 0x17 or C5 */
        if(err != 0)
        {
            return FMSH_FAILURE;
        }
        
        /* readback & verify */
        extAddrRb = FQspiFlash_GetReg8(qspi, ISSI_RDBR_CMD); /*0x16 or C8 */
        if(extAddrRb != extAddr)
        {
            return FMSH_FAILURE;
        } 
    }
    else if(qspi->maker == SPI_FMSH_ID)
    {
    }
    else
    {
        return FMSH_FAILURE;
    }
    
    return FMSH_SUCCESS;
}

/******************************
* erase all data on chip
*
*****************/
int FQspiPs_EraseChip(FQspiPs_T* qspi)
{
    int err;
    
    FMSH_ASSERT(qspi != NULL);
    
    err = FQspiFlash_ChipErase(qspi, CE_CMD);
    if(err != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;
    }
    
    return FMSH_SUCCESS;
}

int FQspiPs_EraseSectors(FQspiPs_T* qspi, u32 offset, u32 byteCount, u32 sectorSize)
{
    int err;
    u32 sectorOffset;
    u32 sectorEnd = offset + byteCount;
    
    FMSH_ASSERT(qspi != NULL);
    
    qspi->sectorSize = sectorSize;
    
    if(byteCount == 0)
    {
        return FMSH_FAILURE; 
    }
    if(qspi->devSize < sectorEnd)
    {
        return FMSH_FAILURE; 
    }   
    if(qspi->isBusy == TRUE)
    {
        return FMSH_FAILURE;
    }
    qspi->isBusy = TRUE;
    
    /* calculate sector start address to be erased */
    sectorOffset = offset & ~(qspi->sectorSize-1);
	
    /* Erase Flash with required sectors */
    while(sectorOffset < sectorEnd)
    {
        err = FQspiFlash_SectorErase(qspi, sectorOffset, SE_CMD);
        if(err != 0)
        {
            return FMSH_FAILURE;
        }
        sectorOffset += qspi->sectorSize;
    }
    
    qspi->isBusy = FALSE;
    
    return FMSH_SUCCESS;
}

/*
direct_mem_copy mode
*/
int FQspiPs_SendBytes(FQspiPs_T* qspi, u32 offset, u32 byteCount, u8 *sendBuffer)
{
    int err;
    u32 address;
    
    FMSH_ASSERT(qspi != NULL);
    FMSH_ASSERT(sendBuffer != NULL);
    
    if(byteCount == 0)
    {
        return FMSH_FAILURE; 
    }
    if(qspi->devSize < (offset + byteCount))
    {
        return FMSH_FAILURE; 
    }
    if(qspi->isBusy == TRUE)
    {
        return FMSH_FAILURE;
    }
    qspi->isBusy = TRUE;
    
    /* Write to Flash  */
    address = qspi->config.dataBaseAddress + offset;
    memcpy((void*)address,
           (void*)sendBuffer,
           (size_t)byteCount); 
    
    /* Wait for write operation done */
    err = FQspiFlash_WaitForWIP(qspi);
    if(err)
    {
        return FMSH_FAILURE;
    }
    
    qspi->isBusy = FALSE;
    
    return FMSH_SUCCESS;
}

/*
direct_mem_copy mode
*/
int FQspiPs_RecvBytes(FQspiPs_T* qspi, u32 offset, u32 byteCount, u8 *recvBuffer)
{
    u32 address;
    
    FMSH_ASSERT(qspi != NULL);
    FMSH_ASSERT(recvBuffer != NULL);
    
    if(byteCount == 0)
    {
        return FMSH_FAILURE; 
    }    
    if(qspi->devSize < (offset + byteCount))
    {
        return FMSH_FAILURE; 
    }
    if(qspi->isBusy == TRUE)
    {
        return FMSH_FAILURE;
    }
    qspi->isBusy = TRUE;
    
    /* Read from flash */
    address = qspi->config.dataBaseAddress + offset;
    memcpy((void*)recvBuffer,
           (void*)address,
           (size_t)byteCount); 
    
    qspi->isBusy = FALSE;
    
    return FMSH_SUCCESS;
}


int FQspiPs_FastSendBytes(FQspiPs_T* qspi, u32 offset, u32 byteCount, u8* sendBuffer)
{
    u32 configReg;
    u32 waterMark = 32;
    
    FMSH_ASSERT(qspi != NULL);
    FMSH_ASSERT(sendBuffer != NULL);
    
    if(qspi->isBusy == TRUE)
    {
        return FMSH_FAILURE;
    }
    qspi->isBusy = TRUE;
    
    qspi->requestedBytes = byteCount;
    qspi->remainingBytes = byteCount;
    qspi->sendBufferPtr = sendBuffer;
    
    /* set flash address where write data (address in flash) */
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_IWTSAR_OFFSET, qspi->config.dataBaseAddress + offset);  
	
    /* set number of bytes to be transferred */
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_IWTNBR_OFFSET, qspi->requestedBytes);  
	
    /* set controller trig adress where write data (adress in controller) */
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_IAATR_OFFSET, qspi->config.dataBaseAddress);    
	
    /* set Trigger Address Range (2^15) */
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_ITARR_OFFSET, 0xf);      
	
    /* set WaterMark Register (between 10~250 if page_size is 256 bytes) */
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_IWTWR_OFFSET, waterMark);   
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_IRTWR_OFFSET, 0x0);   
	
    /* trigger Indirect Write access */
    configReg = 0x1;
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_IWTCR_OFFSET, configReg);
    
    qspi->isBusy = FALSE;  /* jc */
	
    return FMSH_SUCCESS;
}


int FQspiPs_FastRecvBytes(FQspiPs_T* qspi, u32 offset, u32 byteCount, u8* recvBuffer)
{
    u32 configReg;
    u32 waterMark = 480;
    
    FMSH_ASSERT(qspi != NULL);
    FMSH_ASSERT(recvBuffer != NULL);
    
    if(qspi->isBusy == TRUE)
    {
        return FMSH_FAILURE;
    }
    qspi->isBusy = TRUE;
    
    qspi->requestedBytes = byteCount;
    qspi->remainingBytes = byteCount;
    qspi->recvBufferPtr = recvBuffer;
    
    /* set flash address where read data (address in flash) */
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_IRTSAR_OFFSET, qspi->config.dataBaseAddress + offset);  
	
    /* set number of bytes to be transferred */
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_IRTNBR_OFFSET, qspi->requestedBytes);  
	
    /* set controller trig adress where read data (adress in controller) */
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_IAATR_OFFSET, qspi->config.dataBaseAddress);    
	
    /* set Trigger Address Range (2^15) */
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_ITARR_OFFSET, 0xf);  
	
    /* set WaterMark Register */
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_IWTWR_OFFSET, 0x0); 
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_IRTWR_OFFSET, waterMark);  
	
    /* trigger Indirect Write access */
    configReg = 0x1;
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_IRTCR_OFFSET, configReg);  
	
    qspi->isBusy = FALSE;  /* jc */
	
    return FMSH_SUCCESS;
}


int FQspiPs_EnterXIP(FQspiPs_T* qspi, u8 cmd)
{
    u32 DRIR;
    u8 ioWidth;
    
    FMSH_ASSERT(qspi != NULL);
    
    if(cmd == DIOR_CMD)
    {
        ioWidth = 2;
    }
    else if(cmd == QIOR_CMD)
    {
        ioWidth = 4;
    }
    else
    {
        return 1;
    }
    
    FQspiPs_DisableDAC(qspi);
    
    if((qspi->maker == SPI_SPANSION_ID) ||
       (qspi->maker == SPI_ISSI_ID)) 
    {
        FQspiPs_SetModeBits(qspi, 0xA0);
        if(ioWidth == 2)
        {
            DRIR = 0x001110BB;    
        }
        else if(ioWidth == 4)
        {
            DRIR = 0x041220EB;
        }
    }
    else if((qspi->maker == SPI_WINBOND_ID) ||
            (qspi->maker == SPI_FMSH_ID)) 
    {
        FQspiPs_SetModeBits(qspi, 0x20);
        if(ioWidth == 2)
        {
            DRIR = 0x001110BB;    
        }
        else if(ioWidth == 4)
        {
            DRIR = 0x041220EB;
        }
    }
    
    else if(qspi->maker == SPI_MACRONIX_ID)
    {
        FQspiPs_SetModeBits(qspi, 0xA5);
        if(ioWidth == 2)
        {
            return FMSH_FAILURE;   
        }
        else if(ioWidth == 4)
        {
            DRIR = 0x041220EB;
        }
    }
    else if(qspi->maker == SPI_MICRON_ID)
    {
        u8 volatile_cfg = FQspiFlash_GetReg8(qspi, MICRON_RDVCR_CMD);
		
        /*config vcr to enable xip*/
        FQspiFlash_SetReg8(qspi, MICRON_WRVCR_CMD, volatile_cfg & 0xf3);
        FQspiPs_SetModeBits(qspi, 0x00);
        if(ioWidth == 2)
        {
            DRIR = 0x041110BB;    
        }
        else if(ioWidth == 4)
        {
            DRIR = 0x081220EB;/*0x0012006B;//*/
        }
    } 
    else
    {
        return FMSH_FAILURE;
    }
    
    FQspiPs_WaitIdle(qspi);   
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_DRIR_OFFSET, DRIR);
    
    FQspiPs_EnableXip(qspi);
    FQspiPs_EnableDAC(qspi);
    
    return FMSH_SUCCESS;
}


#if 1  // all add by jc 

#if 1

#define QSPI_TIMEOUT    50000

extern void sysUsDelay(int);
void qspiFlashDelay2(UINT32 delay)
{
    sysUsDelay (delay);
}

/* jc */
static FQspiPs_T g_qspi_0 = {0};
FQspiPs_T * qspi_0 = (FQspiPs_T *)(&g_qspi_0);

int SpiFlash_IndirectRead(FQspiPs_T* qspi, u32 offset, u32 byteCount, u8* recvBuffer)
{
    u32 dataCnt = 0;
	int timeout = 0;
	u8 * pBuf8 = NULL;
	int idx = 0;
	
	/*FQspiPs_SetFlashMode(qspi, QIOR_CMD); */

	/* 0x10 : bit1 = disable the adapted loopback clock circuit */
	/* FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_RDCR_OFFSET, 0x1); */
	
	FQspiPs_FastRecvBytes(qspi, offset, byteCount, recvBuffer);

	/* copy data from sram to ddr */
	pBuf8 = (u8*)qspi->config.dataBaseAddress;
	idx = 0;
	do 
	{
		/* Read  Watermark */
	    /* dataCnt = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_SFLR_OFFSET) *4;*/
	    dataCnt = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_IRTNBR_OFFSET);	
		
		if (dataCnt == 0)
		{
			delay_1us();
		}
		else
		{		
		    /* Read data from SRAM */
		    memcpy((u8*)qspi->recvBufferPtr,
		           (u8*)(&pBuf8[idx]) /*(const u8*)qspi->config.dataBaseAddress*/,
		           (size_t)dataCnt);
			idx += dataCnt;
			
		    qspi->recvBufferPtr += dataCnt;
		    qspi->remainingBytes -= dataCnt;
		}

		/* clear the sram_full flag */
		/*
		if (FMSH_SUCCESS == FQspiPs_WaitForBit(qspi, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK, 1))
		{
			FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK);
		}*/	
	} while (qspi->remainingBytes > 0);

	while ((FMSH_FAILURE == FQspiPs_WaitForBit(qspi, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK, 1)) \
		    && (++timeout < QSPI_TIMEOUT))
	{
	   qspiFlashDelay2 (10);
	}
			
    if (timeout == QSPI_TIMEOUT)
        return FMSH_FAILURE;
	/*
	FQspiPs_WaitForBit(qspi, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK, 1);
	*/

    /* clear QSPI_IND_RD_XFER_DONE bit */
	FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK);

	return byteCount;
}

int SpiFlash_IndirectWrite(FQspiPs_T* qspi, u32 offset, u32 byteCount, u8* sendBuffer)
{
	u32 dataCnt = 0;
	int timeout = 0;
	
	u8 * pBuf8 = NULL;
	int idx = 0;
	
	/*FQspiPs_SetFlashMode(qspi, QIOR_CMD);*/
	
	FQspiPs_FastSendBytes(qspi, offset, byteCount, sendBuffer);

	/* copy data from sram to flash */	
	pBuf8 = (u8*)qspi->config.dataBaseAddress;
	idx = 0;
	do 
	{
		byteCount = QSPIFLASH_PAGE_256;		
		if(qspi->remainingBytes < QSPIFLASH_PAGE_256)
		{
			byteCount = qspi->remainingBytes;
		}
		
		/*memcpy((void*)qspi->config.dataBaseAddress,
			   (const void*)qspi->sendBufferPtr,
			   (size_t)byteCount);*/
		memcpy((u8*)(&pBuf8[idx]) /*(char*)qspi->config.dataBaseAddress*/,
			   (const char*)qspi->sendBufferPtr,
			   (size_t)byteCount);
		idx += byteCount;
		
		qspi->sendBufferPtr += byteCount;
		qspi->remainingBytes -= byteCount;		
	} while (qspi->remainingBytes > 0);

	while ((FMSH_FAILURE == FQspiPs_WaitForBit(qspi, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK, 1)) \
		    && (++timeout < QSPI_TIMEOUT))
	{
	   qspiFlashDelay2 (10);
	}
			
    if (timeout == QSPI_TIMEOUT)
        return FMSH_FAILURE;
	
	/*
	FQspiPs_WaitForBit(qspi, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK, 1);
	*/	
	FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK);

	return FMSH_SUCCESS;
}

void qspi_show(UINT32 offset)
{
	UINT32 tmp32 = 0, addr = 0;

	addr = 0xE0000000 + offset;
	tmp32 = *(UINT32 *)addr;

	printf("qspi[0x%08X] = 0x%08X \n", addr, tmp32);
	return;
}

extern UINT32 clk_qspi_get(void);
UINT32 qspi_baud_get(void)
{
	UINT32 tmp32_0 = 0, tmp32_1 = 0; 
	int BD = 0;
	UINT32 qspi_baud = 0;

	tmp32_0 = clk_qspi_get();
	
    /* 
    set baud rate at 4 
    -----
    0x00 : bit22~19 R/W Master mode baud rate divisor (2 to 32)
    SPI baud rate = (master reference clock) / BD => BD/2 - 1
    BD = 2^n + 2: 0 - 2; 1 - 4; 2 - 6 ... 
    */
    tmp32_1 = FMSH_ReadReg(qspi_0->config.baseAddress, QSPIPS_CFG_OFFSET); 
    BD = (2 << ((tmp32_1 & QSPIPS_BAUD_MASK) >> QSPIPS_BAUD_SHIFT)) + 2;
	
	qspi_baud = tmp32_0 / BD;

	return qspi_baud;
}

void qspi_baud_show(void)
{
	printf("qspi ref_clk : %d \n", clk_qspi_get());
	printf("qspi baudrate: %d \n", qspi_baud_get());
	return;
}

int FQspiPs_UnlockFlash_Ctrl0(void)
{
	return FQspiPs_UnlockFlash(qspi_0);
}

#endif


#if 1
static FQspiPs_T gQspiFlash0_2 = {0};
FQspiPs_T * g_pQspi0_2 = (FQspiPs_T *)(&gQspiFlash0_2);

void test_qspi_init()
{
    int mode = 0, bdr = 0;
	FQspiPs_T * pQspi = g_pQspi0_2;
	
	FQspiPs_Initialize(pQspi, FPS_QSPI0_DEVICE_ID);
	
    pQspi->sectorSize = QSPIFLASH_SECTOR_64K;
    FQspiPs_UnlockFlash(pQspi);
	
	mode = 2;
	bdr = 2;
	TRACE_OUT(DEBUG_OUT, "x4 transfer, baud rate = %d \r\n", bdr);
	FQspiPs_SetFlashMode(pQspi, QOR_CMD); 
	
	if (bdr == 2)
	{
		FMSH_WriteReg(pQspi->config.baseAddress, QSPIPS_RDCR_OFFSET, 0x9);
	}
	else if (bdr == 4)
	{
		FMSH_WriteReg(pQspi->config.baseAddress, QSPIPS_RDCR_OFFSET, 0x5);
	}
	else
	{
		FMSH_WriteReg(pQspi->config.baseAddress, QSPIPS_RDCR_OFFSET, 0x1);
	}
	
	FQspiPs_SetBaudRate(pQspi, QSPIPS_BAUD(bdr));	

	return;
}

void test_rd_qspi_sect(int sect_idx, int len)
{
	int qspi_x = 0;
	FQspiPs_T * pQspi = g_pQspi0_2;
	
	u32 offset = 0;
	u32 byteCnt = 0;
	
 	u8 RcvBuf[2048] = {0};
	
	u32 dataCnt = 0;
	u8 * pBuf8 = NULL;
	int idx = 0;
	int timeout = 0, j;
	
	offset = sect_idx * pQspi->sectorSize;
	if (len <= 0)
	{
		byteCnt = 256;
	}
	else
	{
		byteCnt = len;
	}
	
	
	FQspiPs_FastRecvBytes(pQspi, offset, byteCnt, RcvBuf);
	
	pBuf8 = (u8*)pQspi->config.dataBaseAddress;
	idx = 0;
	do 
	{
		/* 
		Read  Watermark 
		*/
		/*dataCnt = FMSH_ReadReg(pQspi->config.baseAddress, QSPIPS_IRTNBR_OFFSET); 		*/
		dataCnt = FMSH_ReadReg(pQspi->config.baseAddress, QSPIPS_SFLR_OFFSET)*4;
		
		if (dataCnt == 0)
		{
			delay_1us();
		}
		else
		{		
			/* 
			Read data from SRAM 
			*/
			memcpy((u8*)pQspi->recvBufferPtr, (u8*)(&pBuf8[idx]), (size_t)dataCnt);
			idx += dataCnt;
			
			pQspi->recvBufferPtr += dataCnt;
			pQspi->remainingBytes -= dataCnt;
		}
	} while (pQspi->remainingBytes > 0);
	
	while ((FMSH_FAILURE == FQspiPs_WaitForBit(pQspi, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK, 1)) \
			&& (++timeout < QSPI_TIMEOUT))
	{
	   qspiFlashDelay2 (10);
	}
			
	if (timeout == QSPI_TIMEOUT)
		return FMSH_FAILURE;

	/* 
	clear QSPI_IND_RD_XFER_DONE bit 
	*/
	FMSH_WriteReg(pQspi->config.baseAddress, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK);

#if 1
	printf("----qspi(%d)-read(%d)----\n", qspi_x, sect_idx);	
	
	for (j=0; j<byteCnt; j++)
	{
		printf("%02X", RcvBuf[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf("------------\n");
#endif


	return;
}

void test_erase_qspi_sect(int sect_idx)
{
	FQspiPs_T * pQspi = g_pQspi0_2;
	u32 offset = 0;
	u32 byteCnt = 0;
	

	offset = sect_idx * pQspi->sectorSize;
	byteCnt = pQspi->sectorSize;
	
	FQspiPs_EraseSectors(pQspi, offset, byteCnt, pQspi->sectorSize);	
	
	return;
}


int g_test_qspi = 0;

void test_wr_qspi_sect(int sect_idx, int len)
{
	int qspi_x = 0;
	FQspiPs_T * pQspi = g_pQspi0_2;
 	u8 SndBuf[2048] = {0};

	u32 dataCnt = 0;
	int timeout = 0;
	u32 byteCnt = 0;
	u32 offset = 0;
	
	u8 * pBuf8 = NULL;
	int idx = 0;
	int i = 0, j = 0;

#if 1	
	for (i=0; i<2048; i++)
	{
		SndBuf[i] = g_test_qspi + i;
	}
	g_test_qspi++;
#endif

	offset = sect_idx * pQspi->sectorSize;
	if (len <= 0)
	{
		byteCnt = 256;
	}
	else
	{
		byteCnt = len;
	}

	FQspiPs_FastSendBytes(pQspi, offset, byteCnt, SndBuf);

	/* copy data from sram to flash */	
	pBuf8 = (u8*)pQspi->config.dataBaseAddress;
	idx = 0;
	do 
	{
		dataCnt = QSPIFLASH_PAGE_256;		
		if(pQspi->remainingBytes < QSPIFLASH_PAGE_256)
		{
			dataCnt = pQspi->remainingBytes;
		}
		
		memcpy((u8*)(&pBuf8[idx]), (const char*)pQspi->sendBufferPtr, (size_t)dataCnt);
		idx += dataCnt;
		
		pQspi->sendBufferPtr += dataCnt;
		pQspi->remainingBytes -= dataCnt;		
	} while (pQspi->remainingBytes > 0);

	while ((FMSH_FAILURE == FQspiPs_WaitForBit(pQspi, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK, 1)) \
		    && (++timeout < QSPI_TIMEOUT))
	{
	   qspiFlashDelay2 (10);
	}
			
    if (timeout == QSPI_TIMEOUT)
        return FMSH_FAILURE;
	
	FMSH_WriteReg(pQspi->config.baseAddress, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK);

#if 1
	printf("----qspi(%d)-write(%d)----\n", qspi_x, sect_idx);	
	
	for (j=0; j<byteCnt; j++)
	{
		printf("%02X", SndBuf[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf("------------\n");
#endif

	return;
}

#if 1
u8 RcvBuf[QSPIFLASH_SECTOR_64K] = {0};
void test_rd_qspi_sect2(int sect_idx)
{
	int qspi_x = 0;
	FQspiPs_T * pQspi = g_pQspi0_2;
	
	u32 offset = 0;
	u32 byteCnt = 0;
	
	
	u32 dataCnt = 0;
	u8 * pBuf8 = NULL;
	int idx = 0;
	int timeout = 0, j;
	
	offset = sect_idx * pQspi->sectorSize;
	byteCnt = QSPIFLASH_SECTOR_64K;
	
	
	FQspiPs_FastRecvBytes(pQspi, offset, byteCnt, RcvBuf);
	
	pBuf8 = (u8*)pQspi->config.dataBaseAddress;
	idx = 0;
	do 
	{
		/* Read  Watermark */
		dataCnt = FMSH_ReadReg(pQspi->config.baseAddress, QSPIPS_IRTNBR_OFFSET); 
		
		if (dataCnt == 0)
		{
			delay_1us();
		}
		else
		{		
			/* Read data from SRAM */
			memcpy((u8*)pQspi->recvBufferPtr, (u8*)(&pBuf8[idx]), (size_t)dataCnt);
			idx += dataCnt;
			
			pQspi->recvBufferPtr += dataCnt;
			pQspi->remainingBytes -= dataCnt;
		}
	} while (pQspi->remainingBytes > 0);
	
	while ((FMSH_FAILURE == FQspiPs_WaitForBit(pQspi, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK, 1)) \
			&& (++timeout < QSPI_TIMEOUT))
	{
	   qspiFlashDelay2 (10);
	}
			
	if (timeout == QSPI_TIMEOUT)
		return FMSH_FAILURE;

	/* 
	clear QSPI_IND_RD_XFER_DONE bit 
	*/
	FMSH_WriteReg(pQspi->config.baseAddress, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK);

#if 1
	printf("----qspi(%d)-read(%d)----\n", qspi_x, sect_idx);	
	
	for (j=0; j<512; j++)
	{
		printf("%02X", RcvBuf[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}

	printf(" ... ... ... ... \n\n");
	
	for (j=(byteCnt-512); j<byteCnt; j++)
	{
		printf("%02X", RcvBuf[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf("------------\n");
#endif

	return;
}


u8 SndBuf[QSPIFLASH_SECTOR_64K] = {0};
void test_wr_qspi_sect2(int sect_idx)
{
	int qspi_x = 0;
	FQspiPs_T * pQspi = g_pQspi0_2;

	u32 dataCnt = 0;
	int timeout = 0;
	u32 byteCnt = 0;
	u32 offset = 0;
	
	u8 * pBuf8 = NULL;
	int idx = 0;
	int i = 0, j = 0;

	offset = sect_idx * pQspi->sectorSize;
	byteCnt = QSPIFLASH_SECTOR_64K;
	
#if 1	
	for (i=0; i<byteCnt; i++)
	{
		SndBuf[i] = g_test_qspi + i;
	}
	g_test_qspi++;
#endif


	FQspiPs_FastSendBytes(pQspi, offset, byteCnt, SndBuf);

	/* copy data from sram to flash */	
	pBuf8 = (u8*)pQspi->config.dataBaseAddress;
	idx = 0;
	do 
	{
		dataCnt = QSPIFLASH_PAGE_256;		
		if(pQspi->remainingBytes < QSPIFLASH_PAGE_256)
		{
			dataCnt = pQspi->remainingBytes;
		}
		
		memcpy((u8*)(&pBuf8[idx]), (const char*)pQspi->sendBufferPtr, (size_t)dataCnt);
		idx += dataCnt;
		
		pQspi->sendBufferPtr += dataCnt;
		pQspi->remainingBytes -= dataCnt;		
	} while (pQspi->remainingBytes > 0);

	while ((FMSH_FAILURE == FQspiPs_WaitForBit(pQspi, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK, 1)) \
		    && (++timeout < QSPI_TIMEOUT))
	{
	   qspiFlashDelay2 (10);
	}
			
    if (timeout == QSPI_TIMEOUT)
        return FMSH_FAILURE;
	
	FMSH_WriteReg(pQspi->config.baseAddress, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK);

#if 1
	printf("----qspi(%d)-write(%d)----\n", qspi_x, sect_idx);	
	
	for (j=0; j<512; j++)
	{
		printf("%02X", SndBuf[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	
	printf(" ... ... ... ... \n\n");
	
	for (j=(byteCnt-512); j<byteCnt; j++)
	{
		printf("%02X", SndBuf[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	
	printf("------------\n");
#endif

	return;
}

#endif


#if 0
int qspi_Rd_Page(int page_idx, UINT8 * pBuf)
{
	offset = sect_idx * pQspi->sectorSize;
	byteCnt = QSPIFLASH_SECTOR_256K;

	FQspiPs_FastSendBytes(pQspi, offset, byteCnt, SndBuf);

	/* copy data from sram to flash */	
	pBuf8 = (u8*)pQspi->config.dataBaseAddress;
	idx = 0;
	do 
	{
		dataCnt = QSPIFLASH_PAGE_256;		
		if(pQspi->remainingBytes < QSPIFLASH_PAGE_256)
		{
			dataCnt = pQspi->remainingBytes;
		}
		
		memcpy((u8*)(&pBuf8[idx]), (const char*)pQspi->sendBufferPtr, (size_t)dataCnt);
		idx += dataCnt;
		
		pQspi->sendBufferPtr += dataCnt;
		pQspi->remainingBytes -= dataCnt;		
	} while (pQspi->remainingBytes > 0);

	while ((FMSH_FAILURE == FQspiPs_WaitForBit(pQspi, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK, 1)) \
		    && (++timeout < QSPI_TIMEOUT))
	{
	   qspiFlashDelay2 (10);
	}
			
    if (timeout == QSPI_TIMEOUT)
        return FMSH_FAILURE;
	
	FMSH_WriteReg(pQspi->config.baseAddress, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK);


}

#endif



#endif


#if 1

void test_qspi_erase_sect3(int sect_idx)
{
	FQspiPs_T * pQspi = g_pQspi0_2;
	u32 offset = 0;
	u32 byteCnt = 0;
	int ret = 0;
	

	offset = sect_idx * pQspi->sectorSize;
	byteCnt = pQspi->sectorSize;
	
	ret = FQspiPs_EraseSectors(pQspi, offset, byteCnt, pQspi->sectorSize);	
    if (ret == FMSH_FAILURE)
    {
    	printf("qspiflash erase sect: %d fail! \n", sect_idx);
    }
	else
	{
		printf("qspiflash erase sect: %d ok! \n", sect_idx);
	}
	
	return;
}

void test_qspi_wr_sect3(int sect_idx)
{
	int qspi_x = 0;
	FQspiPs_T * pQspi = g_pQspi0_2;

	u32 dataCnt = 0;
	int timeout = 0;
	u32 byteCnt = 0;
	u32 offset = 0;
	
	u8 * pBuf8 = NULL;
	int idx = 0;
	int i = 0, j = 0;

	offset = sect_idx * pQspi->sectorSize;
	byteCnt = QSPIFLASH_SECTOR_64K;
	
#if 1	
	for (i=0; i<byteCnt; i++)
	{
		SndBuf[i] = g_test_qspi + i;
	}
	g_test_qspi++;
#endif

	FQspiPs_FastSendBytes(pQspi, offset, byteCnt, SndBuf);

	/* copy data from sram to flash */	
	pBuf8 = (u8*)pQspi->config.dataBaseAddress;
	idx = 0;
	do 
	{
		dataCnt = QSPIFLASH_PAGE_256;		
		if(pQspi->remainingBytes < QSPIFLASH_PAGE_256)
		{
			dataCnt = pQspi->remainingBytes;
		}
		
		memcpy((u8*)(&pBuf8[idx]), (const char*)pQspi->sendBufferPtr, (size_t)dataCnt);
		idx += dataCnt;
		
		pQspi->sendBufferPtr += dataCnt;
		pQspi->remainingBytes -= dataCnt;		
	} while (pQspi->remainingBytes > 0);

	while ((FMSH_FAILURE == FQspiPs_WaitForBit(pQspi, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK, 1)) \
		    && (++timeout < QSPI_TIMEOUT))
	{
	   qspiFlashDelay2 (10);
	}			
	
	FMSH_WriteReg(pQspi->config.baseAddress, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK);
			
    if (timeout >= QSPI_TIMEOUT)
    {    
		printf("--qspi write sect(%d) timeout!!!-- \n", sect_idx);
    	return;
    }
	else
	{
		printf("qspi write sect(%d) end! \n", sect_idx);
	}
	
	return;
}

void test_qspi_rd_sect3(int sect_idx)
{
	int qspi_x = 0;
	FQspiPs_T * pQspi = g_pQspi0_2;
	
	u32 offset = 0;
	u32 byteCnt = 0;
	
	
	u32 dataCnt = 0;
	u8 * pBuf8 = NULL;
	int idx = 0;
	int timeout = 0, j;
	
	offset = sect_idx * pQspi->sectorSize;
	byteCnt = QSPIFLASH_SECTOR_64K;
	
	for (j=0; j<byteCnt; j++)
	{
		RcvBuf[j] = 0x0;
	}
	
	FQspiPs_FastRecvBytes(pQspi, offset, byteCnt, RcvBuf);
	
	pBuf8 = (u8*)pQspi->config.dataBaseAddress;
	idx = 0;
	do 
	{
		/* Read  Watermark */
		dataCnt = FMSH_ReadReg(pQspi->config.baseAddress, QSPIPS_IRTNBR_OFFSET); 
		
		if (dataCnt == 0)
		{
			delay_1us();
		}
		else
		{		
			/* Read data from SRAM */
			memcpy((u8*)pQspi->recvBufferPtr, (u8*)(&pBuf8[idx]), (size_t)dataCnt);
			idx += dataCnt;
			
			pQspi->recvBufferPtr += dataCnt;
			pQspi->remainingBytes -= dataCnt;
		}
	} while (pQspi->remainingBytes > 0);
	
	while ((FMSH_FAILURE == FQspiPs_WaitForBit(pQspi, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK, 1)) \
			&& (++timeout < QSPI_TIMEOUT))
	{
	   qspiFlashDelay2 (10);
	}
			
	if (timeout == QSPI_TIMEOUT)
		return FMSH_FAILURE;

	/* 
	clear QSPI_IND_RD_XFER_DONE bit 
	*/
	FMSH_WriteReg(pQspi->config.baseAddress, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK);

	printf("qspi read sect(%d) end! \n", sect_idx);
	
	return;
}

void test_qspi_rdwr_sects(int from_sect, int to_sect)
{
	FQspiPs_T * pQspi = g_pQspi0_2;
	
	int idx = 0, j = 0;
	int sect_diff = 0;

	for (idx = from_sect; idx <= to_sect; idx++)
	{
		test_qspi_erase_sect3(idx);
		taskDelay(20);
		
		test_qspi_wr_sect3(idx);
		taskDelay(20);
		
		test_qspi_rd_sect3(idx);
		taskDelay(20);

		sect_diff = 0;
		for (j=0; j<pQspi->sectorSize; j++)
		{
			if (RcvBuf[j] != SndBuf[j])
			{
				sect_diff = 1;
				break;
			}
		}

		if (sect_diff == 1)
		{
			printf("------sect(%d) erase & wr & rd err:[%d][0x%X-0x%X]!------- \n\n", \
				    idx, j, RcvBuf[j], SndBuf[j]);
		}
		else
		{
			printf("sect(%d) erase & wr & rd OK! \n\n", idx, RcvBuf[j], SndBuf[j]);
		}
	}

	printf("\n\n test meta_drv sectors:[%d -> %d] finish! \n\n\n", from_sect, to_sect);
	
	return;
}

#endif

#endif

