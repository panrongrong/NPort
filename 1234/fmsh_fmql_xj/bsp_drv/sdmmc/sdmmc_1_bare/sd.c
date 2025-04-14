/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  sd.c
*
* This file contains
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 0.01   yl  12/20/2018  First Release
*</pre>
******************************************************************************/

/***************************** Include Files *********************************/
#include "../../common/fmsh_ps_parameters.h"
#include "../../common/fmsh_common.h"

#include "./fatFS/ff.h"
#include "sd.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/

FIL fil;		/* File object */
FATFS fatfs;
static char buffer[32];
static char *boot_file = buffer;
static char *write_file = buffer;
u32 FlashReadBaseAddress;

/************************** Function Prototypes ******************************/

/*****************************************************************************
*
* @description
* This function copies a string to another, the source string must be null-
* terminated.
*
* @param 	
*           Dest is starting address for the destination string.
*           Src is starting address for the source string.
*
* @return	
*           Starting address for the destination string.
*
* @note     NA.
*
*****************************************************************************/
char *strcpy_rom(char *Dest, const char *Src)
{
	unsigned i;
	for (i=0; Src[i] != '\0'; ++i)
		Dest[i] = Src[i];
	Dest[i] = '\0';
	return Dest;
}

/*****************************************************************************
*
* @description
* This function initializes the controller for the SD FLASH interface.
*
* @param	
*           filename of the file that is to be used.
*
* @return
*		    FMSH_SUCCESS if the controller initializes correctly.
*		    FMSH_FAILURE if the controller initializes failed.
*
* @note		NA.
*
*****************************************************************************/
u32 InitSD(const char *filename)
{
	FRESULT rc;
	TCHAR *path = "0:/"; /* Logical drive number is 0 */
	
	/* Register volume work area, initialize device */
	rc = f_mount(&fatfs, path, 0);
	TRACE_OUT(DEBUG_OUT,"SD: rc= %.8x\n\r", rc);

	if (rc != FR_OK) {
		return FMSH_FAILURE;
	}

	strcpy_rom(buffer, filename);
	boot_file = (char *)buffer;
	FlashReadBaseAddress = FPS_SDMMC0_BASEADDR;

	rc = f_open(&fil, boot_file, FA_READ);
	if (rc) {
		TRACE_OUT(DEBUG_OUT,"SD: Unable to open file %s: %d\r\n", boot_file, rc);
		return FMSH_FAILURE;
	}

	return FMSH_SUCCESS;
}

/*****************************************************************************
*
* @description
* This function provides the SD FLASH interface for the Simplified header
* functionality.
*
* @param	
*           SourceAddress is address in FLASH data space.
*           DestinationAddress is address in OCM data space.
*           LengthBytes is the number of bytes to move.
*
* @return
*		    FMSH_SUCCESS if the read completes correctly.
*		    FMSH_FAILURE if the read completes failed.
*
* @note		NA.
*
*****************************************************************************/
u32 SDAccess( u32 SourceAddress, u32 DestinationAddress, u32 LengthBytes)
{
	FRESULT rc;	 /* Result code */
	UINT br;

	rc = f_lseek(&fil, SourceAddress);
	if (rc) {
		TRACE_OUT(DEBUG_OUT,"SD: Unable to seek to %lx\r\n", SourceAddress);
		return FMSH_FAILURE;
	}

	rc = f_read(&fil, (void*)DestinationAddress, LengthBytes, &br);

	if (rc) {
		TRACE_OUT(DEBUG_OUT,"*** ERROR: f_read returned %d\r\n", rc);
	}

	return FMSH_SUCCESS;
} /* End of SDAccess */


/*****************************************************************************
*
* @description
* This function closes the file object
*
* @param	
*           NA.
*
* @return	
*           NA.
*
* @note		NA.
*
*****************************************************************************/
void ReleaseSD(void)
{
	f_close(&fil);
}

/*****************************************************************************
*
* @description
* This function provides theSD Card interface for the Simplified header
* functionality of Write.
*
* @param	
*           SourceAddress is address inDDR data space.
*           LengthBytes is the number of bytes to move.
*           filename is the name of the Write file in SD card.
*
* @return
*		    FMSH_SUCCESS if the write completes correctly.
*		    FMSH_FAILURE if the write completes failed.
*
* Note(s) : NA.

*****************************************************************************/
u32 SDWriteAccess(u32 SourceAddress, u32 LengthBytes, const char *filename)
{
	FRESULT rc; /* Result code*/
	UINT bw;
	TCHAR *path = "0:/"; /* Logical drive number is 0 */
	
	/* Register volume work area, initialize device */
	rc = f_mount(&fatfs, path, 0);
	TRACE_OUT(DEBUG_OUT,"SD: rc= %.8x\n\r", rc);
	if (rc != FR_OK) {
		return FMSH_FAILURE;
	}

	strcpy_rom(buffer, filename);
	write_file = (char *)buffer;
	rc = f_open(&fil, write_file, FA_CREATE_ALWAYS|FA_WRITE);
	if (rc) {
		TRACE_OUT(DEBUG_OUT, "SD: Unable to Write File.\r\n");
		return FMSH_FAILURE;
	}

	rc= f_lseek(&fil, fil.obj.objsize);
	if (rc) {
		TRACE_OUT(DEBUG_OUT, "Shift Pointer To the End of File Failed.\r\n");
		return FMSH_FAILURE;
	}

	rc= f_write(&fil,(void*)SourceAddress, LengthBytes, &bw);
	if(rc){
		TRACE_OUT(DEBUG_OUT, "Write File To SD Card Failed.\r\n");
		return FMSH_FAILURE;
	}

	rc= f_lseek(&fil, fil.obj.objsize);
	if(rc){
		return FMSH_FAILURE;
	}

	rc= f_close(&fil);
	if(rc){
		return FMSH_FAILURE;
	}

	return FMSH_SUCCESS;
}


