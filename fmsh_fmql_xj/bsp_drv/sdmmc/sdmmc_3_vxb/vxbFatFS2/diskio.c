/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
#include "ff.h"

/* jc*/
#include "./../../../common/fmsh_ps_parameters.h"
#include "./../vxbSdmmc2.h"


#ifdef FILE_SYSTEM_INTERFACE_SD
/*#include "fmsh_sdmmc_lib.h"		/* SD device driver */
#endif

/*--------------------------------------------------------------------------

	Public Functions

---------------------------------------------------------------------------*/

/*
 * Global variables
 */
static DSTATUS Stat;	/* Disk status */

#ifdef FILE_SYSTEM_INTERFACE_SD

/*static FSdmmcPs_T s_SDMMC[BOARD_NUM_SDMMC];*/
/*static FSdmmcPs_Instance_T s_SDMMC_instance;*/
/*static FDmaPs_T s_Dmac;*/

static UINT32 BaseAddress;
static UINT32 CardDetect;
static UINT32 WriteProtect;
/*static enum SDMMC_TransMode TransMode;*/
static int TransMode;

#endif

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

/*****************************************************************************/
/**
*
* Gets the status of the disk.
* In case of SD, it checks whether card is present or not.
*
* @param	pdrv - Drive number
*
* @return
*		0		Status ok
*		STA_NOINIT	Drive not initialized
*		STA_NODISK	No medium in the drive
*		STA_PROTECT	Write protected
*
* @note		In case Card detect signal is not connected,
*		this function will not be able to check if card is present.
*
******************************************************************************/
DSTATUS disk_status 
(
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS s = Stat;
	UINT32 StatusReg;

#ifdef FILE_SYSTEM_INTERFACE_SD

#if 0
	if ((UINT32)s_SDMMC[pdrv].config.BaseAddress == (UINT32)0) 
	{
		if (pdrv == 1)
		{
			BaseAddress = FPAR_SDPS_1_BASEADDR;
		}
		else
		{
			BaseAddress = FPAR_SDPS_0_BASEADDR;
		}

		CardDetect = FPAR_SDPS_0_HAS_CD;
		WriteProtect = FPAR_SDPS_0_HAS_WP;
		TransMode = sdmmc_trans_mode_normal;
	}

	StatusReg = FMSH_ReadReg(BaseAddress, SDMMC_CDETECT) & CARD_DETECT;
	if (CardDetect) 
	{
		if (StatusReg == 1U) 
		{
			s = STA_NODISK | STA_NOINIT;
			goto Label;
		}
	}
	s &= ~STA_NODISK;
	
	StatusReg = FMSH_ReadReg(BaseAddress, SDMMC_WRTPRT) & WRITE_PROTECT;
	if (WriteProtect) 
	{
		if (StatusReg == 1U)
		{
			s |= STA_PROTECT;
			goto Label;
		}
	}
	s &= ~STA_PROTECT;

Label:
		Stat = s;
#endif

#if 1
	if (pdrv == 1)
	{
		BaseAddress = FPAR_SDPS_1_BASEADDR;
	}
	else
	{
		BaseAddress = FPAR_SDPS_0_BASEADDR;
	}

	CardDetect = FPAR_SDPS_0_HAS_CD;
	WriteProtect = FPAR_SDPS_0_HAS_WP;
	TransMode = MODE_XFER_FIFO;  /* sdmmc_trans_mode_normal;*/
	
	StatusReg = FMSH_ReadReg(BaseAddress, SDMMC_CDETECT) & CARD_DETECT;
	if (CardDetect) 
	{
		if (StatusReg == 1U) 
		{
			s = STA_NODISK | STA_NOINIT;
			goto Label;
		}
	}
	s &= ~STA_NODISK;
	
	StatusReg = FMSH_ReadReg(BaseAddress, SDMMC_WRTPRT) & WRITE_PROTECT;
	if (WriteProtect) 
	{
		if (StatusReg == 1U)
		{
			s |= STA_PROTECT;
			goto Label;
		}
	}
	s &= ~STA_PROTECT;

Label:
	Stat = s;
#endif

#endif

	return s;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/
/*****************************************************************************/
/**
*
* Initializes the drive.
* In case of SD, it initializes the host controller and the card.
* This function also selects additional settings such as bus width,
* speed and block size.
*
* @param	pdrv - Drive number
*
* @return	s - which contains an OR of the following information
*		STA_NODISK	Disk is not present
*		STA_NOINIT	Drive not initialized
*		STA_PROTECT	Drive is write protected
*		0 or only STA_PROTECT both indicate successful initialization.
*
* @note
*
******************************************************************************/
DSTATUS disk_initialize 
(
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS s;
	s32 Status;
	UINT32 timeout_cnt = SDMMC_LOOP_TIMEOUT;

#ifdef FILE_SYSTEM_INTERFACE_SD

	/*
	 * Check if card is in the socket
	 */
	s = disk_status(pdrv);
	if ((s & STA_NODISK) != 0U) 
	{
		return s;
	}

	if (CardDetect) 
	{
		/*
		 * Card detection check
		 * If the HC detects the No Card State, power will be cleared
		 */
		while (FMSH_ReadReg(BaseAddress, SDMMC_CDETECT) & CARD_DETECT)
		{
			timeout_cnt--;
			
			delay_1us();
			
			if (timeout_cnt == 0)
			{
				s |= STA_NOINIT;
				return s;
			}
		}
	}

	/* Initialize the host controller */
#if 0    
    FSdmmcPs_Config *pSdCfg;
    
    /* Initialize the SD Driver */
    pSdCfg = FSdmmcPs_LookupConfig((u16)pdrv);
    if (pSdCfg == NULL) {
		s |= STA_NOINIT;
		return s;
    }
	FSdmmcPs_initDev(&s_SDMMC[pdrv], &s_SDMMC_instance, pSdCfg);
    
	Stat = STA_NOINIT;
	
	Status = FSdmmcPs_cfgInitialize(&s_SDMMC[pdrv], &s_Dmac, TransMode);
	if (Status != FMSH_SUCCESS) 
	{
		s |= STA_NOINIT;
		return s;
	}

	Status = FSdmmcPs_initCard(&s_SDMMC[pdrv]);
	if (Status != FMSH_SUCCESS) 
	{
		s |= STA_NOINIT;
		return s;
	}
#endif

#if 0
    VXB_DEVICE_ID  pDev;
	int unit = pdrv;	
    pDev = vxbInstByNameFind (FM_SDMMC_NAME, unit);
    if (pDev == NULL)
    {
        printf ("Can not find this sd unit(%d)!  \n", unit);
		s |= STA_NOINIT;
		return s;
    }
	/*extern void vxbSdmmcCtrl_Init_fatFS (VXB_DEVICE_ID pDev);*/
	Status = vxbSdmmcCtrl_Init_fatFS(pDev);
	if (Status != FMSH_SUCCESS) 
	{
		s |= STA_NOINIT;
		return s;
	}
	else
	{
		printf("vxbSdmmcCtrl_Init() ok! \n");
	}
#endif

	/*
	 * Disk is initialized.
	 * Store the same in Stat.
	 */
	s &= (~STA_NOINIT);

	Stat = s;
#endif

	return s;
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
/*****************************************************************************/
/**
*
* Reads the drive
* In case of SD, it reads the SD card using ADMA2 in polled mode.
*
* @param	pdrv - Drive number
* @param	*buff - Pointer to the data buffer to store read data
* @param	sector - Start sector number
* @param	count - Sector count
*
* @return
*		RES_OK		Read successful
*		STA_NOINIT	Drive not initialized
*		RES_ERROR	Read not successful
*
* @note
*
******************************************************************************/
DRESULT disk_read 
(
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
#ifdef FILE_SYSTEM_INTERFACE_SD

	DSTATUS s;
	s32 Status;
	UINT32 len;
	DWORD LocSector = sector;

	s = disk_status(pdrv);

	if ((s & STA_NOINIT) != 0U) 
	{
		return RES_NOTRDY;
	}
	
	if (count == 0U) 
	{
		return RES_PARERR;
	}

	len = count * SDMMC_BLOCK_SIZE;

	#if 0
		Status  = FSdmmcPs_readFun(&s_SDMMC[pdrv], LocSector, (char *)buff, len, TransMode);
		if (Status != FMSH_SUCCESS) 
		{
			return RES_ERROR;
		}
	#endif

	#if 1		
		VXB_DEVICE_ID		pDev;
		int unit = pdrv;		
		pDev = vxbInstByNameFind (FM_SDMMC_NAME, unit);
		if (pDev == NULL)
		{
			printf ("Can not find this sd unit(%d)!  \n", unit);
			return RES_ERROR;
		}		
		Status = vxbSdmmc_RdSector_Poll2 (pDev, (UINT8 *)(&buff[0]), LocSector, len);		
		if (Status == ERROR) 
		{
			return RES_ERROR;
		}	
	#endif
	
#endif

    return RES_OK;
}

/*****************************************************************************/
/**
*
* Writes the drive
* In case of SD, it writes to the SD card using ADMA2 in polled mode.
*
* @param	pdrv - Drive number
* @param	*buff - Pointer to the data to be written
* @param	sector - Sector address
* @param	count - Sector count
*
* @return
*		RES_OK		Read successful
*		STA_NOINIT	Drive not initialized
*		RES_ERROR	Read not successful
*
* @note
*
******************************************************************************/
DRESULT disk_write 
(
	BYTE pdrv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	UINT count			/* Number of sectors to write (1..128) */
)
{
	DSTATUS s;
	s32 Status;
	UINT32 len;
	DWORD LocSector = sector;

#ifdef FILE_SYSTEM_INTERFACE_SD

	s = disk_status(pdrv);

	if ((s & STA_NOINIT) != 0U) 
	{
		return RES_NOTRDY;
	}
	
	if (count == 0U) 
	{
		return RES_PARERR;
	}

	len = count * SDMMC_BLOCK_SIZE;	

	#if 0
		Status = FSdmmcPs_writeFun(&s_SDMMC[pdrv], LocSector, (char *)buff, len, TransMode);
		if (Status != FMSH_SUCCESS) 
		{
			return RES_ERROR;
		}
	#endif

	#if 1	
		VXB_DEVICE_ID		pDev;
		int unit = pdrv;
		pDev = vxbInstByNameFind (FM_SDMMC_NAME, unit);
		if (pDev == NULL)
		{
			printf ("Can not find this sd unit(%d)!  \n", unit);
			return RES_ERROR;
		}		
		/*int vxbSdmmc_WrSector_Poll2(VXB_DEVICE_ID pDev, UINT8 *pBuf, UINT32 addr_offset, UINT32 len)*/
		Status = vxbSdmmc_WrSector_Poll2 (pDev, (UINT8 *)(&buff[0]), LocSector, len);		
		if (Status == ERROR) 
		{
			return RES_ERROR;
		}	
	#endif	
#endif

	return RES_OK;
}


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions						*/
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl 
(
	BYTE pdrv,				/* Physical drive number (0) */
	BYTE cmd,				/* Control code */
	void *buff				/* Buffer to send/receive control data */
)
{
#ifdef FILE_SYSTEM_INTERFACE_SD

	DRESULT res;
	void *LocBuff = buff;
	
	if ((disk_status(pdrv) & STA_NOINIT) != 0U) /* Check if card is in the socket */
	{	
		return RES_NOTRDY;
	}

	res = RES_ERROR;
	switch (cmd) 
	{
		case (BYTE)CTRL_SYNC :	/* Make sure that no pending write process */
			res = RES_OK;
			break;

		case (BYTE)GET_SECTOR_COUNT : /* Get number of sectors on the disk (DWORD) */
			res = RES_ERROR;
			break;

		case (BYTE)GET_BLOCK_SIZE :	/* Get erase block size in unit of sector (DWORD) */
			(*((DWORD *)((void *)LocBuff))) = ((DWORD)128);
			res = RES_OK;
			break;

		default:
			res = RES_PARERR;
			break;
	}

	return res;
#else

	return 0;
#endif
}

/******************************************************************************/
/**
*
* This function is User Provided Timer Function for FatFs module
*
* @param	None
*
* @return	DWORD
*
* @note		None
*
****************************************************************************/

DWORD get_fattime (void)
{
	return	((DWORD)(2010U - 1980U) << 25U)	/* Fixed to Jan. 1, 2010 */
		| ((DWORD)1 << 21)
		| ((DWORD)1 << 16)
		| ((DWORD)0 << 11)
		| ((DWORD)0 << 5)
		| ((DWORD)0 >> 1);
}


