/* norflash_amd.h - norflash_amd driver header */

/*
 * Copyright (c) 2013 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01a, 09Jun20, jc  written.
*/

#ifndef __INC_VX_NORFLASH_AMD_H__
#define __INC_VX_NORFLASH_AMD_H__



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** 
Command for vendor command set CMD_SET_AMD. 
*/
#define AMD_CMD_RESET         0x00F0 /* Reset or read mode */
#define AMD_CMD_CFI           0x0098 /* CFI command */
#define AMD_CMD_AUTO_SELECT   0x0090 /* Auto select command */
#define AMD_CMD_UNLOCK_1      0x00AA /* Unlock cycle 1 */
#define AMD_CMD_UNLOCK_2      0x0055 /* Unlock cycle 2 */
#define AMD_CMD_ERASE_SETUP   0x0080 /* Setup erase */
#define AMD_CMD_ERASE_RESUME  0x0030 /* Resume erase */
#define AMD_CMD_ERASE_CHIP    0x0010 /* Chip erase command */
#define AMD_CMD_ERASE_SECTOR  0x0030 /* Sector erase command */
#define AMD_CMD_PROGRAM       0x00A0 /* Program command */
#define AMD_CMD_UNLOCK_BYPASS 0x0020 /* Unlock bypass command */
	
/**
Command offset for vendor command set CMD_SET_AMD 
*/
#define AMD_OFFSET_UNLOCK_1   0x0555
#define AMD_OFFSET_UNLOCK_2   0x02AA

/** 
Query command address.
*/
#define FLASH_ADDRESS_CFI     0x0055
	
/** 
AMD norflash device Identifier infomation address offset. 
*/
#define AMD_MANU_ID           0x00
#define AMD_DEVIDE_ID         0x01
	
/** 
Data polling mask for vendor command set CMD_SET_AMD 
*/
#define AMD_POLLING_DQ7       0x80
#define AMD_POLLING_DQ6       0x60
#define AMD_POLLING_DQ5       0x20
#define AMD_POLLING_DQ3       0x08
	
#define DQ1_MASK   (0x02)  /* DQ1 mask for all interleave devices */
#define DQ2_MASK   (0x04)  /* DQ2 mask for all interleave devices */
#define DQ3_MASK   (0x08)  /* DQ3 mask for all interleave devices */
#define DQ5_MASK   (0x20)  /* DQ5 mask for all interleave devices */
#define DQ6_MASK   (0x40)  /* DQ6 mask for all interleave devices */
/*#define DQ6_TGL_DQ1_MASK (dq6_toggles >> 5)	/* Mask for DQ1 when device DQ6 toggling */
/*#define DQ6_TGL_DQ5_MASK (dq6_toggles >> 1)	 Mask for DQ5 when device DQ6 toggling */
	

extern void norFlashAMD_Reset_Cmd(vxT_NORFLASH* pNorflash);
extern UINT32 norFlashAMD_ReadIdentification(vxT_NORFLASH* pNorflash);
extern UINT8 norFlashAMD_PollByToggle(vxT_NORFLASH* pNorflash, UINT32 offset);
extern UINT8 norFlashAMD_Program(vxT_NORFLASH* pNorflash, UINT32 address, UINT32 data);
extern void norFlashAMD_Reset(vxT_NORFLASH* pNorflash);
extern UINT32 norFlashAMD_Read_DevID(vxT_NORFLASH* pNorflash);
extern UINT8 norFlashAMD_EraseSector(vxT_NORFLASH* pNorflash, UINT32 address);
extern UINT8 norFlashAMD_EraseChip(vxT_NORFLASH* pNorflash);
extern UINT8 norFlashAMD_Write_Data(vxT_NORFLASH* pNorflash, UINT32 address, UINT8 *buffer, UINT32 size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_VX_NORFLASH_AMD_H__ */

