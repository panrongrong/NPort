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
* @file fmsh_qspips_nand.h
* @addtogroup qspips_v1_1
* @{
*
* This header file contains the identifiers and basic spi nand driver
* functions (or macros) that can be used to access the device.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who Date     Changes
* ----- --- -------- -----------------------------------------------
* 1.10  hzq 11/26/20 
* 		     First release
*
* </pre>
*
******************************************************************************/

#ifndef _FMSH_QSPIPS_NAND_H_	/* prevent circular inclusions */ 
#define _FMSH_QSPIPS_NAND_H_	/* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif
    
/**********************************Include File*********************************/

/**********************************Constant Definition**************************/
    
#define FQSPIPS_NAND_BBT_SCAN_2ND_PAGE		        (0x1)	/**< Scan the second page for bad block information */

#define FQSPIPS_NAND_BB_PATTERN_OFFSET	            (0)	    /**< Bad block pattern offset in a large page */
#define FQSPIPS_NAND_BB_PATTERN_LENGTH	            (2)	    /**< Bad block pattern length */
#define FQSPIPS_NAND_BB_PATTERN_GOOD			    (0xff)	/**< Bad block pattern to search in a page */  
#define FQSPIPS_NAND_BB_PATTERN_BAD			        (0x00)	/**< Bad block pattern to search in a page */  

/*
 * Block definitions for RAM based Bad Block Table (BBT)
 */
#define FQSPIPS_NAND_BLOCK_GOOD			            (0x3)	/**< Block is good */
#define FQSPIPS_NAND_BLOCK_BAD			            (0x2)	/**< Block is bad */
#define FQSPIPS_NAND_BLOCK_RESERVED                 (0x1)
#define FQSPIPS_NAND_BLOCK_FACTORY_BAD              (0x0)

#define FQSPIPS_NAND_BBT_BLOCK_SHIFT		        (2)	    /**< 1 byte represent 4 block in BBT */
#define FQSPIPS_NAND_BLOCK_TYPE_MASK		        (0x03)	/**< Block type mask */
#define FQSPIPS_NAND_BLOCK_SHIFT_MASK		        (0x06)	/**< Block shift mask for a Bad Block Table entry byte */ 
     
#define FQSPIPS_NAND_MAXPAGESIZE                    (4096)     
#define FQSPIPS_NAND_MAXSPARESIZE                   (128) 
#define FQSPIPS_NAND_MAXNUMBLOCKS                   (2048)
#define FQSPIPS_NAND_NUM_BBT_CHECK_BLOCKS           (512) 

#define FQSPIPS_PARAM_PAGE_SIZE                     (256)

#define FQSPIPS_NAND_S1_CMP                         (0x1 << 1)
#define FQSPIPS_NAND_S1_TB                          (0x1 << 2)      
#define FQSPIPS_NAND_S1_BP0                         (0x1 << 3)
#define FQSPIPS_NAND_S1_BP1                         (0x1 << 4)
#define FQSPIPS_NAND_S1_BP2                         (0x1 << 5)
#define FQSPIPS_NAND_S1_BRWD                        (0x1 << 7)

#define FQSPIPS_NAND_S2_QE                          (0x1)
#define FQSPIPS_NAND_S2_ECC_EN                      (0x1 << 4)
#define FQSPIPS_NAND_S2_OTP_EN                      (0x1 << 6)
#define FQSPIPS_NAND_S2_OTP_PRT                     (0x1 << 7)

#define FQSPIPS_NAND_S3_OIP                         (0x1)
#define FQSPIPS_NAND_S3_WEL                         (0x1 << 1)  
#define FQSPIPS_NAND_S3_EFAIL                       (0x1 << 2)     
#define FQSPIPS_NAND_S3_PFAIL                       (0x1 << 3) 
#define FQSPIPS_NAND_S3_ECCS0                       (0x1 << 4) 
#define FQSPIPS_NAND_S3_ECCS1                       (0x1 << 5) 
#define FQSPIPS_NAND_ECC_STATUS                     (FQSPIPS_NAND_S3_ECCS0 | FQSPIPS_NAND_S3_ECCS1)
     
/**********************************Type Definition******************************/  
typedef struct {
	u32 options;		    /**< Options to search the bad block pattern */
	u32 offset;		        /**< Offset to search for specified pattern */
	u32 length;		        /**< Number of bytes to check the pattern */
	u8 pattern[2];		    /**< Pattern format to search for */
} FQspiPs_Nand_BbPattern;

typedef struct{
    int version; 
    char* name;
    int id_len;
    u8 maker;
    u32 pageSize;
    u32 spareSize;
    u32 blockSize;
    u64 deviceSize; 
    u32 numPagesPerBlock;
    u32 numBlocksPerTarget;
    
    FQspiPs_Nand_BbPattern      bbPattern;	    /**< Bad block pattern to search */
    u8 bbInfo[FQSPIPS_NAND_MAXNUMBLOCKS >> 2];

} FQspiPs_Nand_Device;    


/**
 * Parameter page structure of ONFI 1.0 specification.
 * Enhanced this sturcture to include ONFI 2.3 information for EZ NAND support.
 */
#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
typedef struct {
	/*
	 * Revision information and features block
	 */
	u8 Signature[4];	/**< Parameter page signature */
	u16 Revision;		/**< Revision Number */
	u16 Features;		/**< Features supported */
	u16 OptionalCmds;	/**< Optional commands supported */
	u8 Reserved0[22];	/**< ONFI 2.3: Reserved */
	/*
	 * Manufacturer information block
	 */
	u8 DeviceManufacturer[12];	/**< Device manufacturer */
	u8 DeviceModel[20];		/**< Device model */
	u8 JedecManufacturerId;		/**< JEDEC Manufacturer ID */
	u8 DateCode[2];			/**< Date code */
	u8 Reserved1[13];		/**< Reserved */
	/*
	 * Memory organization block
	*/
	u32 BytesPerPage;		/**< Number of data bytes per page */
	u16 SpareBytesPerPage;		/**< Number of spare bytes per page */
    u8 Reserved2[6];
	u32 PagesPerBlock;		/**< Number of pages per block */
	u32 BlocksPerLun;		/**< Number of blocks per logical unit
					  (LUN) */
	u8 NumLuns;			/**< Number of LUN's */
	u8 AddrCycles;			/**< Number of address cycles */
	u8 BitsPerCell;			/**< Number of bits per cell */
	u16 MaxBadBlocksPerLun;		/**< Bad blocks maximum per LUN */
	u16 BlockEndurance;		/**< Block endurance */
	u8 GuaranteedValidBlock;	/**< Guaranteed valid blocks at
					  beginning of target */
	u16 BlockEnduranceGvb;		/**< Block endurance for guaranteed
					  valid block */
	u8 ProgramsPerPage;		/**< Number of programs per page */
	u8 Reserved3;		/**< Partial programming attributes */
	u8 EccBits;			/**< Number of bits ECC
					  correctability */
	u8 InterleavedAddrBits;		/**< Number of interleaved address
					  bits */
	u8 InterleavedOperation;	/**< Interleaved operation
					  attributes */
	u8 Reserved4[13];		/**< Reserved */
	/*
	 * Electrical parameters block
	*/
	u8 IOPinCapacitance;		/**< I/O pin capacitance */
	u8 Reserved5[4];
	u16 TProg;			/**< Maximum page program time */
	u16 TBers;			/**< Maximum block erase time */
	u16 TR;				/**< Maximum page read time */
    u8 Reserved6[25];
	/*
	 * Vendor block
	*/
	u16 VendorRevisionNum;		/**< Vendor specific revision
                                number */
	u8 VendorSpecific[88];		/**< Vendor specific */
	u16 Crc;                    /**< Integrity CRC */
#ifdef __ICCARM__
} FQspiPs_Nand_ParamPage;
#pragma pack(pop)
#else
}__attribute__((packed)) FQspiPs_Nand_ParamPage;
#endif

/**********************************Macro (inline function) Definition***********/

/**********************************Variable Definition**************************/
extern FQspiPs_Nand_Device qspi_nand_dev;
extern FQspiPs_Caps qspi_nand_caps_default;    

/**********************************Function Prototype***************************/
int FQspiPs_Nand_GetFlashInfo(FQspiPs_T* qspiPtr, u8* id);
int FQspiPs_Nand_SetFlashMode(FQspiPs_T* qspiPtr, u8 mode);

int FQspiPs_Nand_Erase(FQspiPs_T* qspiPtr, u64 offset, u32 byteCount, 
                       u32 blockSize);
int FQspiPs_Nand_Write(FQspiPs_T* qspiPtr, u64 offset, u32 byteCount, 
                       u8* data);
int FQspiPs_Nand_Read(FQspiPs_T* qspiPtr, u64 offset, u32 byteCount, 
                      u8* data);
int FQspiPs_Nand_WriteSpareBytes(FQspiPs_T* qspiPtr, u32 block, u32 page, 
                                 u8* data);
int FQspiPs_Nand_ReadSpareBytes(FQspiPs_T* qspiPtr, u32 block, u32 page, 
                                u8* data);

int FQspiPs_Nand_SkipErase(FQspiPs_T* qspiPtr, u64 offset, u32 byteCount, 
                           u32 blockSize);
int FQspiPs_Nand_SkipWrite(FQspiPs_T* qspiPtr, u64 offset, u32 byteCount, 
                           u8* data);
int FQspiPs_Nand_SkipRead(FQspiPs_T* qspiPtr, u64 offset, u32 byteCount, 
                          u8* data);

int FQspiPs_Nand_ProgramPage(FQspiPs_T* qspiPtr, u32 block, u32 page, 
                             u16 offset, u8* data);
int FQspiPs_Nand_ReadPage(FQspiPs_T* qspiPtr, u32 block, u32 page, 
                          u16 offset, u8* data);
int FQspiPs_Nand_EraseBlock(FQspiPs_T* qspiPtr, u32 block);

int FQspiPs_Nand_ReadUniqueID(FQspiPs_T* qspiPtr, u8* data);
int FQspiPs_Nand_ReadParaPage(FQspiPs_T* qspiPtr, u8* data);
int FQspiPs_Nand_OtpPage(FQspiPs_T* qspiPtr, u8 page, u8* data);
                          
int FQspiPs_Nand_Reset(FQspiPs_T* qspiPtr);
u8 FQspiPs_Nand_GetFeature(FQspiPs_T* qspiPtr, u8 feature);
int FQspiPs_Nand_SetFeature(FQspiPs_T* qspiPtr, u8 feature, u8 value);
int FQspiPs_Nand_Lock(FQspiPs_T* qspiPtr);
int FQspiPs_Nand_Unlock(FQspiPs_T* qspiPtr);
int FQspiPs_Nand_EnableQuad(FQspiPs_T* qspiPtr);
int FQspiPs_Nand_DisableQuad(FQspiPs_T* qspiPtr);
int FQspiPs_Nand_EnableECC(FQspiPs_T* qspiPtr);
int FQspiPs_Nand_DisableECC(FQspiPs_T* qspiPtr);
int FQspiPs_Nand_EnableOtp(FQspiPs_T* qspiPtr);
int FQspiPs_Nand_DisableOtp(FQspiPs_T* qspiPtr);

int FQspiPs_Nand_WREN(FQspiPs_T* qspiPtr);
int FQspiPs_Nand_WRDI(FQspiPs_T* qspiPtr);  
int FQspiPs_Nand_WaitReady(FQspiPs_T* qspiPtr, u8* status);

int FQspiPs_Nand_MarkBlockBad(FQspiPs_T* qspiPtr, u32 block);
int FQspiPs_Nand_IsBlockBad(FQspiPs_T* qspiPtr, u32 block);

int FQspiPs_Nand_Initr(FQspiPs_T* qspiPtr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* prevent circular inclusions */;