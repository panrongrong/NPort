/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */

#include "../../../common/fmsh_ps_parameters.h"

/* Definitions of physical drive number for each drive */
#define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
#define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */
/*--------------------------------------------------------------------------

	Public Functions

---------------------------------------------------------------------------*/

/*
 * Global variables
 */
static DSTATUS Stat;	/* Disk status */

static UINT32 BaseAddress;
static UINT32 CardDetect;
static UINT32 WriteProtect;

/*static enum SDMMC_TransMode TransMode;*/
static int TransMode;


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
	
	/*
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
*/
	s = 0;

Label:
	Stat = s;

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
	int Status;
	UINT32 timeout_cnt = SDMMC_LOOP_TIMEOUT;

	/*
	 * Check if card is in the socket
	 */
	s = disk_status(pdrv);
	if ((s & STA_NODISK) != 0U) 
	{
		return s;
	}

	#if 0
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
	#endif

	/* Initialize the host controller */
#if 1
	/*Status = vxInit_Sdmmc_2(pdrv, 0);  // 0 - MODE_XFER_FIFO*/
	Status = vxInit_Sdmmc(pdrv, 0);  /* 0 - MODE_XFER_FIFO*/
	if (Status != FMSH_SUCCESS) 
	{
		s |= STA_NOINIT;
		return s;
	}
	else
	{
		printf("vxInit_Sdmmc_2() ok! \n");
	}
#endif

	/*
	 * Disk is initialized.
	 * Store the same in Stat.
	 */
	s &= (~STA_NOINIT);

	Stat = s;

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
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DSTATUS s;
	int Status;
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

#if 1	
		vxT_SDMMC * pSdmmc = NULL;		
		switch (pdrv)
		{
		case SDMMC_CTRL_0:
			pSdmmc = g_pSdmmc0; 
			break;
		
		case SDMMC_CTRL_1:
			pSdmmc = g_pSdmmc1; 
			break;
		}			
		Status  = vxSdmmc_RdSector_Poll_fatFS(pSdmmc, (UINT8 *)(&buff[0]), LocSector, len);
		if (Status != FMSH_SUCCESS) 
		{
			return RES_ERROR;
		}	
	
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
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DSTATUS s;
	int Status;
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

	#if 1	
		vxT_SDMMC * pSdmmc = NULL;		
		switch (pdrv)
		{
		case SDMMC_CTRL_0:
			pSdmmc = g_pSdmmc0; 
			break;
		
		case SDMMC_CTRL_1:
			pSdmmc = g_pSdmmc1; 
			break;
		}			
		Status  = vxSdmmc_WrSector_Poll_fatFS(pSdmmc, (UINT8 *)(&buff[0]), LocSector, len);
		if (Status != FMSH_SUCCESS) 
		{
			return RES_ERROR;
		}	
	#endif


	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
/*
待完善...
*/
DRESULT disk_ioctl 
(
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
#if 0  /* origin*/
	DRESULT res;
	int result;

	switch (pdrv) {
	case DEV_RAM :

		/* Process of the command for the RAM drive*/

		return res;

	case DEV_MMC :

		/* Process of the command for the MMC/SD card*/

		return res;

	case DEV_USB :

		/* Process of the command the USB drive*/

		return res;
	}

	return RES_PARERR;
#else

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


