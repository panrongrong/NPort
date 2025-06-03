#ifndef _FMSH_NFCPS_H_   /* prevent circular inclusions */
#define _FMSH_NFCPS_H_

#include "fmsh_nfcps_model.h"

#ifdef __cplusplus
extern "C" {
#endif
    
/**********************************Include File*********************************/

/**********************************Constant Definition**************************/

#ifndef FPAR_NANDPS_0_DEVICE_ID         
#define FPS_NFC_S_DEVICE_ID             (0) 
#else
#define FPS_NFC_S_DEVICE_ID             FPAR_NANDPS_0_DEVICE_ID     
#endif
    
#ifndef FPS_NFC_S_BASEADDR
#define FPS_NFC_S_BASEADDR              (0xE0042000)   
#endif
    
#define FPS_NFC_S_FLASHWIDTH            (16) /* 16->8*/
#define FPS_NFC_NUM_INSTANCES           (1)   


#if 0
#define FPS_NFC_SAMPLE_DELAY          0  /* 0 cycles*/
#define FPS_NFC_TIMING_TWHR           12 /* 60ns*/
#define FPS_NFC_TIMING_TRHW           20 /* 100ns*/
#define FPS_NFC_TIMING_TADL           14 /* 70ns*/
#define FPS_NFC_TIMING_TCCS           0  /* not defined*/
#define FPS_NFC_TIMING_TWW            20 /* 100ns*/
#define FPS_NFC_TIMING_TRR            4  /* 20ns*/
#define FPS_NFC_TIMING_TWB            20 /* (max)100ns*/
#define FPS_NFC_TIMING_TRWH           2  /* 10ns*/
#define FPS_NFC_TIMING_TRWP           3  /* 12ns*/

#else
#define FPS_NFC_SAMPLE_DELAY          0 /* 0 cycles*/
#define FPS_NFC_TIMING_TWHR           12/* 60ns*/
#define FPS_NFC_TIMING_TRHW           20 /* 100ns*/
#define FPS_NFC_TIMING_TADL           14 /* 70ns*/
#define FPS_NFC_TIMING_TCCS           2 /* not defined*/
#define FPS_NFC_TIMING_TWW            20 /* 100ns*/
#define FPS_NFC_TIMING_TRR            4 /* 20ns*/
#define FPS_NFC_TIMING_TWB            20 /* (max)100ns*/
#define FPS_NFC_TIMING_TRWH           2 /* 10ns*/
#define FPS_NFC_TIMING_TRWP           4 /* 12ns*/
#endif

/* T_WHR = 16*/
/* T_RHW = 20*/
/* T_ADL = 14*/
/* T_CCS = 2*/
/* T_WW = 20*/
/* T_RR = 4*/
/* T_WB = 20*/
/* T_DLY = 0*/
/* T_RWH = 1*/
/* T_RWP = 2*/

#if 0
/** Maximum number of blocks in a device */
#define NAND_MAX_BLOCKS                        2048 /*2048 */

/** Maximum number of pages in one block */
#define NAND_MAX_PAGES_1BLK                     64 /*256 */

/** Maximum size of the data area of one page, in bytes. */
#define NAND_MAX_PAGE_DATASIZE                  2048 /*4096 */

/** Maximum size of the spare area of one page, in bytes. */
#define NAND_MAX_PAGE_SPARESIZE                 64 /*128*/ /*256 */

/** Maximum number of ecc bytes stored in the spare for one single page. */
#define NAND_MAX_SPARE_ECCBYTES                  48 /*24 */
#else

/** Maximum number of blocks in a device */
#define NAND_MAX_BLOCKS                       2048

/** Maximum number of pages in one block */
#define NAND_MAX_PAGES_1BLK                    256

/** Maximum size of the data area of one page, in bytes. */
#define NAND_MAX_PAGE_DATASIZE                 4096

/** Maximum size of the spare area of one page, in bytes. */
#define NAND_MAX_PAGE_SPARESIZE                 256

/** Maximum number of ecc bytes stored in the spare for one single page. */
#define NAND_MAX_SPARE_ECCBYTES                 48
#endif

#define NAND_MAX_BADBLKS                   104  

#define NFCPS_ECC_HW_OFFSET             0x20
/**********************************Type Definition******************************/
    
typedef void (*Nfc_StatusHandler) (void *callBackRef, u32 statusEvent,
                                   u32 byteCount);

/******************************
* This enum contains ECC Mode
**************/
typedef enum {
	NFCPS_ECC_NONE,	        /**< No ECC */
	NFCPS_ECC_ONDIE,	    /**< On-Die ECC */
    NFCPS_ECC_HW,	        /**< Hardware controller ECC */
    NFCPS_ECC_SW	        /**< Hardware controller ECC */
} FNfcPs_EccMode_E;

/******************************
* Bad block table descriptor
**************/
typedef struct{
    u32 blockOffset;	    /**< Block offset where BBT resides */
    u32 sigOffset;		    /**< Signature offset in Spare area */
    u32 verOffset;		    /**< Offset of BBT version */
    u32 sigLength;		    /**< Length of the signature */
    u32 maxBlocks;		    /**< Max blocks to search for BBT */
    char signature[4];	    /**< BBT signature */
    u8 version;		        /**< BBT version */
    u32 valid;		        /**< BBT descriptor is valid or not */
} FNfcPs_BbtDesc_T;

/****************************
 * Bad block pattern
 ****************/
typedef struct {
	u32 options;		    /**< Options to search the bad block pattern */
	u32 offset;		        /**< Offset to search for specified pattern */
	u32 length;		        /**< Number of bytes to check the pattern */
	u8 pattern[2];		    /**< Pattern format to search for */
} FNfcPs_BbPattern_T;

/*****************************
 * This typedef contains configuration information for the flash device.
 ****************/
typedef struct {
    u16 deviceId;		    /**< Unique ID  of device */
    u32 baseAddress;        /**< AHB Base address of the device */
    u32 flashWidth;
} FNfcPs_Config_T;

typedef struct FNfcPs_Tag{
    FNfcPs_Config_T config;	                /**< Configuration structure */   
    u32 flag;
    FNfcPs_Model_T model;                   /**< Part geometry */
    
    FNfcPs_EccMode_E eccMode;                 /**< ECC Mode */
    u8 eccCalc[NAND_MAX_PAGE_SPARESIZE];	/**< Buffer for calculated ECC */
    u8 eccCode[NAND_MAX_PAGE_SPARESIZE];	/**< Buffer for stored ECC */
    
    FNfcPs_BbtDesc_T bbtDesc;                 /**< Bad block table descriptor */
    FNfcPs_BbtDesc_T bbtMirrorDesc;	        /**< Mirror BBT descriptor */
    FNfcPs_BbPattern_T bbPattern;	            /**< Bad block pattern to search */
    u8 bbInfo[NAND_MAX_BLOCKS >> 2];
    u32 bbMap[NAND_MAX_BADBLKS];                         /**< Nandflash Bad Block Info */
    
    u8 dataBuffer[NAND_MAX_PAGE_DATASIZE+NAND_MAX_PAGE_SPARESIZE];  /**< Buffer to send (state) */
    u8* spareBufferPtr;	 /**< Buffer to receive (state) */
    
    int (*Erase) (struct FNfcPs_Tag* nfc, u64 destAddr, u32 blockCount);
    int (*ReadPage) (struct FNfcPs_Tag* nfc, u64 srcAddr, u32 byteCount, u8* destPtr); /**< Read Page routine */
	int (*WritePage) (struct FNfcPs_Tag* nfc, u64 destAddr, u32 byteCount, u8* srcPtr); /**< Write Page routine */
    
    Nfc_StatusHandler statusHandler;
    void* statusRef;
} FNfcPs_T;

/**********************************Macro (inline function) Definition***********/

/**********************************Variable Definition**************************/

/**********************************Function Prototype***************************/
FNfcPs_Config_T* FNfcPs_LookupConfig(u16 deviceId);
int FNfcPs_CfgInitialize(FNfcPs_T* nfc, FNfcPs_Config_T* configPtr);
void FNfcPs_Reset(FNfcPs_T* nfc);
int FNfcPs_SelfTest(FNfcPs_T* nfc);
void FNfcPs_SetStatusHandler(FNfcPs_T* nfc, void* callBackRef, Nfc_StatusHandler funcPtr);
void FNfcPs_InterruptHandler(void *instancePtr);
u32 FNfcPs_FindInstruction(FNfcPs_T* nfc, u32 command);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* prevent circular inclusions */
