/* vxNorflash.h - vxNorflash driver header */

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

#ifndef __INC_VX_NORFLASH_H__
#define __INC_VX_NORFLASH_H__



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CFI_MAX_ERASE_REGION    4

/** Common flash interface query command. */
#define CFI_QUERY_COMMAND     0x98
#define CFI_QUERY_ADDRESS     0x55
#define CFI_QUERY_OFFSET      0x10

/** Vendor command set control interface ID code . */
#define CMD_SET_NULL          0x0000
#define CMD_SET_INTEL_EXT     0x0001 
#define CMD_SET_AMD           0x0002
#define CMD_SET_INTEL         0x0003
#define CMD_SET_AMD_EXT       0x0004
#define CMD_SET_MISUBISHI     0x0100
#define CMD_SET_MISUBISHI_EXT 0x0101
#define CMD_SET_SST           0x0102


/** Indicates the maximum region for norflash device. */
#define NORFLASH_MAXNUMRIGONS 4

/** Indicates the NorFlash uses an 8-bit address bus. */
#define FLASH_CHIP_WIDTH_8BITS  0x01

/** Indicates the NorFlash uses an 16-bit address bus. */
#define FLASH_CHIP_WIDTH_16BITS 0x02

/** Indicates the NorFlash uses an 32-bit address bus. */
#define FLASH_CHIP_WIDTH_32BITS 0x04

/** Indicates the NorFlash uses an 64-bit address bus. */
#define FLASH_CHIP_WIDTH_64BITS 0x08


#if 1
/** The function completed successfully. */
#define NorCommon_ERROR_NONE               0

/** Can not detect common flash infterface.*/
#define NorCommon_ERROR_UNKNOWNMODEL       1

/** A read operation cannot be carried out.*/
#define NorCommon_ERROR_CANNOTREAD         2

/** A write operation cannot be carried out.*/
#define NorCommon_ERROR_CANNOTWRITE        3

/** A erase operation cannot be carried out.*/
#define NorCommon_ERROR_CANNOTERASE        4

/** A locked operation cannot be carried out.*/
#define NorCommon_ERROR_PROTECT            5

/** An operation is time out.*/
#define NorCommon_ERROR_TIMEOUT            6
#endif

/*****************************************************************************
* DESCRIPTION
*  This is a Norflash CFI query system interface information.
*
*****************************************************************************/
typedef struct _t_NorFlashCfiQueryInfo_ 
{
    UINT8 reserved;
	
    /** Query Unique String "QRY". */
    UINT8 queryUniqueString[3];
	
    /** Primary vendor command set and control interface ID .*/
    UINT16 primaryCode;
	
    /** Address for primary extended query table.*/
    UINT16 primaryAddr;
	
    /** Alternate vendor command set and control interface ID .*/
    UINT16 alternateCode;
	
    /** Address for alternate extended query table.*/
    UINT16 alternateAddr;
	
    /** Vcc logic supply minimum write/erase voltage.*/
    UINT8 minVcc;
	
    /** Vcc logic supply maximum write/erase voltage.*/
    UINT8 maxVcc;
	
    /** Vpp logic supply minimum write/erase voltage.*/
    UINT8 minVpp;
	
    /** Vpp logic supply maximum write/erase voltage.*/
    UINT8 maxVpp;
	
    /** Timeout per single write (2<<n) in microsecond.*/
    UINT8 minTimeOutWrite;
	
    /** Timeout for minimum-size buffer write (2<<n) in microsecond.*/
    UINT8 minTimeOutBuffer;
	
    /** Timeout for block erase (2<<n) in microsecond.*/
    UINT8 minTimeOutBlockErase;
	
    /** Timeout for chip erase (2<<n) in microsecond.*/
    UINT8 minTimeOutChipErase;
	
    /** Maximum timeout per write (2<<n) in microsecond.*/
    UINT8 maxTimeOutWrite;
	
    /** Maximum timeout for buffer write (2<<n) in microsecond.*/
    UINT8 maxTimeOutBuffer;
	
    /** Maximum timeout for block erase (2<<n) in microsecond.*/
    UINT8 maxTimeOutBlockErase;
	
    /** Maximum timeout for chip erase (2<<n) in microsecond.*/
    UINT8 maxTimeOutChipErase;
}vxT_NORFLASHCFI_QUERYINFO;


/*****************************************************************************
* DESCRIPTION
*  This is a Norflash CFI Erase block Region information.
*
*****************************************************************************/
typedef struct _t_EraseRegionInfo_ 
{
    /** Number of erase blocks within the region.*/
    UINT16 Y;
	
    /** Size within the region.*/
    UINT16 Z;
	
} vxT_EraseRegionInfo;


/*****************************************************************************
* DESCRIPTION
*  This is a Norflash CFI critical details of device geometry.
*
*****************************************************************************/
typedef struct _t_NorFlashCfiDeviceGeometry_ 
{
    UINT8 reserved1;
	
    /** Size of Device (2<<n) in number of bytes.*/
    UINT8 devSize;
	
    /** Flash device interface description.*/
    UINT16 devInterface;
	
    /** Maximum number of bytes in multi-byte write (2<<n).*/
    UINT16 numMultiWrite;
	
    /** Number of erase block regions.*/
    UINT8 numEraseRegion;
    UINT8 reserved2;
	
    /** Erase block Region information. */
    vxT_EraseRegionInfo eraseRegionInfo[16];
	
}vxT_NORFlASHCFI_DEVGEOMETRY;


/*****************************************************************************
* DESCRIPTION
*  This is a Norflash Common Flash Interface information.
*
*****************************************************************************/
typedef struct _t_norflash_cfi_
{
    /** CFI query system interface information. */
    vxT_NORFLASHCFI_QUERYINFO  norFlashCfiQueryInfo;
	
    /** CFI critical details of device geometry. */
    vxT_NORFlASHCFI_DEVGEOMETRY  norFlashCfiDevGeometry;
} vxT_NorFlashCFI;


/*
norflash
*/
typedef struct _t_norflash_
{
    /* amd/intel cmd identify*/
	UINT32 cmdSet;
	
	/* Base address */
	UINT32 baseAddr;
	
	/* device ID */
	UINT16 manufacture_id;
	UINT16 device_id1;
	
	/* Address bus using giving by CFI detection.
	    It can not retrieve info directly from  the NorFlashCFI, it depend on hardware connection. */
	UINT8 devChipWidth;
	
	/* Indicate the decive CFI is compatible */
	UINT8 cfiCompatible;
	
	/* Norflash Common Flash Interface information. */
	vxT_NorFlashCFI cfiDescription;
} vxT_NORFLASH;


	

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_VX_NORFLASH_H__ */

