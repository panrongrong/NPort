/* norflash_intel.h - norflash_intel driver header */

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

#ifndef __INC_VX_NORFLASH_INTEL_H__
#define __INC_VX_NORFLASH_INTEL_H__



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Command for vendor command set CMD_SET_INTEL. Device commands are written 
	to the Command User Interface (CUI) to control all flash memory device operations. 
*/
#define INTEL_CMD_IDIN             0x0090
#define INTEL_CMD_BLOCK_ERASE_1    0x0020
#define INTEL_CMD_BLOCK_ERASE_2    0x00D0
#define INTEL_CMD_READ_STATUS      0x0070
#define INTEL_CMD_CLEAR_STATUS     0x0050
#define INTEL_CMD_BLOCK_LOCKSTART  0x0060
#define INTEL_CMD_BLOCK_LOCK       0x0001
#define INTEL_CMD_BLOCK_UNLOCK     0x00D0
#define INTEL_CMD_BLOCK_LOCKDOWN   0x002F
#define INTEL_CMD_PROGRAM_WORD     0x0010
#define INTEL_CMD_RESET            0x00FF
	
	
/** 
Intel norflash status resgister 
*/
#define INTEL_STATUS_DWS    0x80
#define INTEL_STATUS_ESS    0x40
#define INTEL_STATUS_ES     0x20
#define INTEL_STATUS_PS     0x10
#define INTEL_STATUS_VPPS   0x08
#define INTEL_STATUS_PSS    0x04
#define INTEL_STATUS_BLS    0x02
#define INTEL_STATUS_BWS    0x01
	
/** 
Intel norflash device Identifier infomation address offset. 
*/
#define INTEL_MANU_ID       0x00
#define INTEL_DEVIDE_ID     0x01
#define INTEL_LOCKSTATUS    0x02
	
/*/ Intel norflash device lock status.*/
#define INTEL_LOCKSTATUS_LOCKED         0x01
#define INTEL_LOCKSTATUS_LOCKDOWNED     0x02
	
extern void norFlashINTEL_sndCmd8(UINT32 BaseAddr, UINT32 Offset, UINT32 Cmd);
extern void norFlashINTEL_Reset_Cmd(vxT_NORFLASH* pNorflash, UINT32 address);
extern UINT32 norFlashINTEL_Read_Identify(	  vxT_NORFLASH* pNorflash,	   UINT32 offset);
extern UINT8 norFlashINTEL_Read_Status(vxT_NORFLASH* pNorflash, UINT32 address);
extern void norFlashINTEL_ClearStatus(vxT_NORFLASH* pNorflash);
extern void norFlashINTEL_unLock_Sector(vxT_NORFLASH* pNorflash, UINT32 address);
extern UINT32 norFlashINTEL_Get_BlkLockStatus(vxT_NORFLASH* pNorflash, UINT32 address);
extern UINT8 norFlashINTEL_Program(    vxT_NORFLASH* pNorflash, UINT32 address, UINT32 data);
extern void norFlashINTEL_Reset(vxT_NORFLASH* pNorflash, UINT32 address);
extern UINT32 norFlashINTEL_ReadDeviceID(vxT_NORFLASH* pNorflash);
extern UINT8 norFlashINTEL_EraseSector(vxT_NORFLASH* pNorflash,  UINT32 address);
extern UINT8 norFlashINTEL_EraseChip(vxT_NORFLASH* pNorflash);
extern UINT8 norFlashINTEL_Write_Data(vxT_NORFLASH* pNorflash, UINT32 address, UINT8 *buffer, UINT32 size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_VX_NORFLASH_INTEL_H__ */

