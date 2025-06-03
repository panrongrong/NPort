/* norflash_cfi.h - norflash_cfi driver header */

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

#ifndef __INC_VX_NORFLASH_CFI_H__
#define __INC_VX_NORFLASH_CFI_H__



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define NOR_WRITE_COMMAND8(commandAddress, command)              \
		{*(volatile UINT8 *)(commandAddress) = (UINT8)command;}
		
#define NOR_WRITE_DATA8(dataAddress, data)                   \
		{(*((volatile UINT8 *) dataAddress)) = (UINT8)data;}
		
#define NOR_READ_DATA8(dataAddress)          \
		(*((volatile UINT8 *) dataAddress))

	
extern void norFlashCFI_Dump_Cfg(vxT_NORFLASH* pNorflash);
extern UINT32 norFlashCFI_Get_DevNumOfBlks(vxT_NORFLASH* pNorflash);
extern UINT32 norFlashCFI_Get_DevMinBlkSize(vxT_NORFLASH* pNorflash);
extern UINT32 norFlashCFI_Get_DevMaxBlkSize(vxT_NORFLASH* pNorflash);
extern UINT32 norFlashCFI_Get_DevBlkSize(vxT_NORFLASH* pNorflash, UINT32 sector);
extern UINT16 norFlashCFI_Get_DevSectInRegion(vxT_NORFLASH* pNorflash, UINT32 memoryOffset);
extern UINT32 norFlashCFI_Get_DevSectAddr(vxT_NORFLASH* pNorflash, UINT32 sector);
extern UINT32 norFlashCFI_Get_ByteAddr(vxT_NORFLASH* pNorflash, UINT32 offset);
extern UINT32 norFlashCFI_Get_ByteAddrInChip(vxT_NORFLASH* pNorflash, UINT32 offset);
extern UINT32 norFlashCFI_Get_AddrInChip(vxT_NORFLASH* pNorflash, UINT32 offset);
extern UINT8 norFlashCFI_Get_DataBusWidth( vxT_NORFLASH* pNorflash);
extern unsigned long  norFlashCFI_Get_DevSizeInBytes(vxT_NORFLASH* pNorflash);
extern UINT8 norFlashCFI_Detect(vxT_NORFLASH* pNorflash,  UINT8 hardwareBusWidth);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_VX_NORFLASH_CFI_H__ */

