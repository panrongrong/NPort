/* vxbFmshSpi.h - Freescale AM335x SPI hardware defintions */

/*
 * Copyright (c) 2012 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01a,28nov12,mpc  created
*/

#ifndef __INC_vxbFmNfc_h__
#define __INC_vxbFmNfc_h__

#ifdef __cplusplus
extern "C"
{
#endif

/* defines */

#define FM_NFC_NAME	   "fmnfc"

#define  VX_NFC_S_CFG_BASE   (0xE0042000)

#define  NFC_CTRL_0   (0)

#define  OK_FLAG_0     (0)
#define  OK_FLAG_1     (1)

#define  VX_NFC_S_CFG_BASE              (0xE0042000)

#define  NFC_CTRL_0                     (0)

#define FPS_NFC_S_FLASHWIDTH            (16)
#define FPS_NFC_NUM_INSTANCES           (1)   

#define NFC_AHB_RST_MASK                (0x01)
#define NFC_REF_RST_MASK                (0x02)

/***********************
 * Registers
 ***********/
#define NFCPS_COMMAND_OFFSET              0x00
#define NFCPS_CONTROL_OFFSET              0x04
#define NFCPS_STATUS_OFFSET               0x08
#define NFCPS_STATUS_MASK_OFFSET          0x0C
#define NFCPS_INT_MASK_OFFSET             0x10
#define NFCPS_INT_STATUS_OFFSET           0x14
#define NFCPS_ECC_CTRL_OFFSET             0x18
#define NFCPS_ECC_OFFSET_OFFSET           0x1C
#define NFCPS_ECC_STAT_OFFSET             0x20
#define NFCPS_ADDR0_COL_OFFSET            0x24
#define NFCPS_ADDR0_ROW_OFFSET            0x28
#define NFCPS_ADDR1_COL_OFFSET            0x2C
#define NFCPS_ADDR1_ROW_OFFSET            0x30
#define NFCPS_PROTECT_OFFSET              0x34
#define NFCPS_FIFO_DATA_OFFSET            0x38
#define NFCPS_DATA_REG_OFFSET             0x3C
#define NFCPS_DATA_REG_SIZE_OFFSET        0x40
#define NFCPS_DEV0_PTR_OFFSET             0x44
#define NFCPS_DEV1_PTR_OFFSET             0x48
#define NFCPS_DMA_ADDR_L_OFFSET           0x64
#define NFCPS_DMA_CNT_OFFSET              0x6C
#define NFCPS_DMA_CTRL_OFFSET             0x70
#define NFCPS_BBM_CTRL_OFFSET             0x74
#define NFCPS_MEM_CTRL_OFFSET             0x80
#define NFCPS_DATA_SIZE_OFFSET            0x84
#define NFCPS_TIMINGS_ASYN_OFFSET         0x88
#define NFCPS_TIME_SEQ_0_OFFSET           0x90
#define NFCPS_TIME_SEQ_1_OFFSET           0x94
#define NFCPS_TIME_GEN_SEQ_0_OFFSET       0x98
#define NFCPS_TIME_GEN_SEQ_1_OFFSET       0x9C
#define NFCPS_TIME_GEN_SEQ_2_OFFSET       0xA0
#define NFCPS_FIFO_INIT_OFFSET            0xB0
#define NFCPS_FIFO_STATE_OFFSET           0xB4
#define NFCPS_GEN_SEQ_CTRL_OFFSET         0xB8
#define NFCPS_MLUN_OFFSET                 0xBC
#define NFCPS_DEV0_SIZE_OFFSET            0xC0
#define NFCPS_DEV1_SIZE_OFFSET            0xC4
#define NFCPS_SS_CCNT0_OFFSET             0xE0
#define NFCPS_SS_CCNT1_OFFSET             0xE4
#define NFCPS_SS_SCNT_OFFSET              0xE8
#define NFCPS_SS_ADDR_DEV_CTRL_OFFSET     0xEC
#define NFCPS_SS_CMD0_OFFSET              0xF0
#define NFCPS_SS_CMD1_OFFSET              0xF4
#define NFCPS_SS_CMD2_OFFSET              0xF8
#define NFCPS_SS_CMD3_OFFSET              0xFC
#define NFCPS_SS_ADDR_OFFSET              0x100
#define NFCPS_SS_MSEL_OFFSET              0x104
#define NFCPS_SS_REQ_OFFSET               0x108
#define NFCPS_SS_BRK_OFFSET               0x10C
#define NFCPS_DMA_TLVL_OFFSET             0x114
#define NFCPS_CMD_MARK_OFFSET             0x124
#define NFCPS_LUN_STATUS_0_OFFSET         0x128
#define NFCPS_TIME_GEN_SEQ_3_OFFSET       0x134
#define NFCPS_INTERNAL_STATUS_OFFSET      0x148
#define NFCPS_BBM_OFFSET_OFFSET           0x14C
#define NFCPS_PARAM_REG_OFFSET            0x150

/*************************
 * Interrupt Mask
 ************/
#define NFCPS_INTR_ECC_INT1_MASK            (0x1 << 25)
#define NFCPS_INTR_ECC_INT0_MASK            (0x1 << 24)
#define NFCPS_INTR_STAT_ERR_INT1_MASK       (0x1 << 17)
#define NFCPS_INTR_STAT_ERR_INT0_MASK       (0x1 << 16)
#define NFCPS_INTR_MEM1_RDY_INT_MASK        (0x1 << 9)
#define NFCPS_INTR_MEM0_RDY_INT_MASK        (0x1 << 8)
#define NFCPS_INTR_PG_SZ_ERR_INT_MASK       (0x1 << 6)
#define NFCPS_INTR_SS_READY_INT_MASK        (0x1 << 5)
#define NFCPS_INTR_TRANS_ERR_MASK           (0x1 << 4)
#define NFCPS_INTR_DMA_INT_MASK             (0x1 << 3)
#define NFCPS_INTR_DATA_REG_MASK            (0x1 << 2)
#define NFCPS_INTR_CMD_END_INT_MASK         (0x1 << 1)
#define NFCPS_INTR_PORT_INT_MASK            (0x1 << 0)
     
#define NFCPS_INTR_ALL                      (0xffffffff)

/*********************************
 * CONTROL 
 *****************/
#define NFCPS_AUTO_READ_STAT_EN_MASK        (0x1 << 23)
#define NFCPS_MLUN_EN_MASK                  (0x1 << 22)
#define NFCPS_SMALL_BLOCK_EN_MASK           (0x1 << 21)
#define NFCPS_ADDR1_AUTO_INCR_MASK          (0x1 << 17)
#define NFCPS_ADDR0_AUTO_INCR_MASK          (0x1 << 16)
#define NFCPS_PROT_EN_MASK                  (0x1 << 14)
#define NFCPS_BBM_EN_MASK                   (0x1 << 13)
#define NFCPS_IOWIDTH16_MASK                (0x1 << 12)
#define NFCPS_BLOCK_SIZE_MASK               (0x3 << 6)
#define NFCPS_ECC_EN_MASK                   (0x1 << 5)
#define NFCPS_INT_EN_MASK                   (0x1 << 4)
#define NFCPS_RETRY_EN_MASK                 (0x1 << 3)
#define NFCPS_ECC_BLOCK_SIZE_MASK           (0x3 << 1)
#define NFCPS_READ_STATUS_EN_MASK           (0x1 << 0)
     
#define NFCPS_USER_CTRL_MASK                (NFCPS_AUTO_READ_STAT_EN_MASK |\
                                                NFCPS_MLUN_EN_MASK |\
                                                NFCPS_SMALL_BLOCK_EN_MASK |\
                                                NFCPS_ADDR1_AUTO_INCR_MASK |\
                                                NFCPS_ADDR0_AUTO_INCR_MASK |\
                                                NFCPS_PROT_EN_MASK |\
                                                NFCPS_BBM_EN_MASK |\
                                                NFCPS_ECC_EN_MASK |\
                                                NFCPS_INT_EN_MASK |\
                                                NFCPS_RETRY_EN_MASK |\
                                                NFCPS_READ_STATUS_EN_MASK )
#define NFCPS_ECCBLOCKSIZE(bytes)           (bytes)
#define NFCPS_BLOCKSIZE(pages)              (pages)

#define NFCPS_ECC_BLOCK_SIZE_SHIFT           (1)
#define NFCPS_BLOCK_SIZE_SHIFT               (6)

/*****************************
 * CTRL & STATUS
 **************/
#define NFCPS_CTRL_MEM0WP_MASK              (0x1 << 8)
#define NFCPS_CTRL_MEMCE_MASK               (0x1) 
     
#define NFCPS_CTRL_MEM0WP_SHIFT             (8)

/****************************     
 * PROTECT
 ************/    
#define NFCPS_PROT_UP_MASK                  (0xffff << 16) 
#define NFCPS_PROT_DOWN_MASK                (0xffff)   

#define NFCPS_PROT_UP_SHIFT                 (16)      
 
/****************************
 * ECC CTRL
 ****************/     
#define NFCPS_ECC_SEL_MASK                  (0x3 << 16)
#define NFCPS_ECC_THRESHOLD_MASK            (0x3f << 8)
#define NFCPS_ECC_CAP_MASK                  (0x7)

#define NFCPS_ECC_SEL_SHIFT                 (16)
#define NFCPS_ECC_THRESHOLD_SHIFT           (8)

#define NFCPS_ECC_ERROR_STATE               (0x0)
#define NFCPS_ECC_UNC_STATE                 (0x1)
#define NFCPS_ECC_OVER_STATE                (0x2)
#define NFCPS_ECC_CAP2_STATE                (0x0)
#define NFCPS_ECC_CAP4_STATE                (0x1)
#define NFCPS_ECC_CAP8_STATE                (0x2)
#define NFCPS_ECC_CAP16_STATE               (0x3)
#define NFCPS_ECC_CAP24_STATE               (0x4)
#define NFCPS_ECC_CAP32_STATE               (0x5)

/*****************************
 * ECC STATUS
 **************/
#define NFCPS_ECC_OVER_MASK                 (0x1 << 16)
#define NFCPS_ECC_UNC_MASK                  (0x1 << 8)
#define NFCPS_ECC_ERROR_MASK                (0x1)

/******************************
 * FIFO_INIT
 ***********/
#define NFCPS_FIFO_INIT_MASK                (0x1)

/*****************************
 * FIFO STAT
 *************/ 
#define NFCPS_STAT_DFIFO_W_EMPTY_MASK       (0x1 << 7)
#define NFCPS_STAT_DFIFO_R_FULL_MASK        (0x1 << 6)
#define NFCPS_STAT_CFIFO_ACCPT_W_MASK       (0x1 < <5)
#define NFCPS_STAT_CFIFO_ACCPT_R_MASK       (0x1 << 4)
#define NFCPS_STAT_CFIFO_FULL_MASK          (0x1 << 3)
#define NFCPS_STAT_CFIFO_EMPTY_MASK         (0x1 << 2)
#define NFCPS_STAT_DFIFO_W_FULL_MASK        (0x1 << 1)
#define NFCPS_STAT_DFIFO_R_EMPTY_MASK       (0x1 << 0)

#define NFCPS_FIFO_DEPTH                    (2048)
/******************************
 * DATA REG SIZE
 ***********/
#define NFCPS_DATAREG_SIZE_MASK             (0x3)   

#define NFCPS_DATA_1B_STATE                 (0x0) 
#define NFCPS_DATA_2B_STATE                 (0x1)
#define NFCPS_DATA_3B_STATE                 (0x2)
#define NFCPS_DATA_4B_STATE                 (0x3)

/******************************
 * DATA SIZE
 ***********/
#define NFCPS_DATA_SIZE_MASK                (0x3fff) 

/*****************************
 * DMA CTRL
 *************/
#define NFCPS_DMA_READY_MASK                (0x1)
#define NFCPS_DMA_ERR_MASK                  (0x1 << 1)
#define NFCPS_DMA_BURSTTYPE_MASK            (0x7 << 2)
#define NFCPS_DMA_MODE_MASK                 (0x1 << 5)    
#define NFCPS_DMA_START_MASK                (0x1 << 7)    
     
#define NFCPS_DMA_START_SHIFT               (7)     
#define NFCPS_DMA_MODE_SHIFT                (5)  
#define NFCPS_DMA_BURSTTYPE_SHIFT           (2)  
#define NFCPS_DMA_ERR_SHIFT                 (1)  
     
#define NFCPS_DMA_SFR_STATE                 (0x0)  
#define NFCPS_DMA_SG_STATE                  (0x1) 
     
#define NFCPS_DMA_BURSTTYPE0_STATE          (0x0) 
#define NFCPS_DMA_BURSTTYPE1_STATE          (0x1)
#define NFCPS_DMA_BURSTTYPE2_STATE          (0x2)
#define NFCPS_DMA_BURSTTYPE3_STATE          (0x3)
#define NFCPS_DMA_BURSTTYPE4_STATE          (0x4)
#define NFCPS_DMA_BURSTTYPE5_STATE          (0x5)

/*****************************
 * STATUS
 *************/    
#define NFCPS_STAT_CMD_ID_MASK               (0xff << 16)
#define NFCPS_STAT_DATA_REG_RDY_MASK         (0x1 << 10)
#define NFCPS_STAT_DATA_SIZE_ERROR_MASK      (0x1 << 9)
#define NFCPS_STAT_CTRL_RDY_MASK             (0x1 << 8)  
#define NFCPS_STAT_MEM0_RDY_MASK             (0x1)
     
#define NFCPS_RST_CTRL_MASK                 (0x3)

#define NFCPS_ECC_NONE_VX    (0) 		/**< No ECC */
#define NFCPS_ECC_ONDIE_VX   (1)		/**< On-Die ECC */
#define NFCPS_ECC_HW_VX      (2)		/**< Hardware controller ECC */
#define NFCPS_ECC_SW_VX      (3)		/**< Hardware controller ECC */


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
#define FPS_NFC_SAMPLE_DELAY          (0)   /* 0 cycles*/
#define FPS_NFC_TIMING_TWHR           (12)  /* 60ns*/
#define FPS_NFC_TIMING_TRHW           (20)  /* 100ns*/
#define FPS_NFC_TIMING_TADL           (14)  /* 70ns*/
#define FPS_NFC_TIMING_TCCS           (2)   /* not defined*/
#define FPS_NFC_TIMING_TWW            (20)  /* 100ns*/
#define FPS_NFC_TIMING_TRR            (4)   /* 20ns*/
#define FPS_NFC_TIMING_TWB            (20)  /* (max)100ns*/
#define FPS_NFC_TIMING_TRWH           (2)   /* 10ns*/
#define FPS_NFC_TIMING_TRWP           (4)   /* 12ns*/

#if 1
/** Maximum number of blocks in a device */
#define NAND_MAX_BLOCKS                      (2048) /*2048 */

/** Maximum number of pages in one block */
#define NAND_MAX_PAGES_1BLK                  (64) /*256 */

/**Maximum size of the data area of one page, in bytes. */
#define NAND_MAX_PAGE_DATASIZE               (2048) /*4096 */

/** Maximum size of the spare area of one page, in bytes. */
#define NAND_MAX_PAGE_SPARESIZE              (64) /*128*/ /*256 */

/** Maximum number of ecc bytes stored in the spare for one single page. */
#define NAND_MAX_SPARE_ECCBYTES              (48) /*24 */

#define NAND_MAX_BADBLKS                     (104)  

#define NFCPS_ECC_HW_OFFSET                  (0x20)

#endif


/* typedefs */

/**
 * Parameter page structure of ONFI 1.0 specification.
 * Enhanced this sturcture to include ONFI 2.3 information for EZ NAND support.
 */
typedef struct 
{
	/*
	 * Revision information and features block
	 */
	UINT8 Signature[4];	/**< Parameter page signature */
	
	UINT16 Revision;		/**< Revision Number */
	UINT16 Features;		/**< Features supported */
	UINT16 OptionalCmds;	/**< Optional commands supported */
	
	UINT8 Reserved0[2];	    /**< ONFI 2.3: Reserved */
	UINT16 ExtParamPageLen;	/**< ONFI 2.3: extended parameter page	length */
	UINT8 NumOfParamPages;	    /**< ONFI 2.3: No of parameter pages */
	UINT8 Reserved1[17];	    /**< Reserved */
	
	/*
	 * Manufacturer information block
	 */
	UINT8 DeviceManufacturer[12];	/**< Device manufacturer */
	UINT8 DeviceModel[20];		    /**< Device model */
	UINT8 JedecManufacturerId;		/**< JEDEC Manufacturer ID */
	UINT8 DateCode[2];			    /**< Date code */
	UINT8 Reserved2[13];		    /**< Reserved */
	
	/*
	 * Memory organization block
	*/
	UINT32 BytesPerPage;		    /**< Number of data bytes per page */
	UINT16 SpareBytesPerPage;		/**< Number of spare bytes per page */
	UINT32 BytesPerPartialPage;	    /**< Number of data bytes per partial page */
	UINT16 SpareBytesPerPartialPage;	/**< Number of spare bytes per partial page */
	
	UINT32 PagesPerBlock;		/**< Number of pages per block */
	UINT32 BlocksPerLun;		/**< Number of blocks per logical unit(LUN) */
	
	UINT8 NumLuns;			    /**< Number of LUN's */
	UINT8 AddrCycles;			/**< Number of address cycles */
	UINT8 BitsPerCell;			/**< Number of bits per cell */
	
	UINT16 MaxBadBlocksPerLun;		/**< Bad blocks maximum per LUN */
	UINT16 BlockEndurance;		    /**< Block endurance */
	
	UINT8 GuaranteedValidBlock;	    /**< Guaranteed valid blocks at beginning of target */
	UINT16 BlockEnduranceGvb;		/**< Block endurance for guaranteed valid block */
	
	UINT8 ProgramsPerPage;		/**< Number of programs per page */
	UINT8 PartialProgAttr;		/**< Partial programming attributes */
	
	UINT8 EccBits;			    /**< Number of bits ECC correctability */
	
	UINT8 InterleavedAddrBits;		/**< Number of interleaved address bits */
	UINT8 InterleavedOperation;	    /**< Interleaved operationattributes */
	
	UINT8 EzNandSupport;		/**< ONFI 2.3: EZ NAND supportparameters */
	UINT8 Reserved3[12];		/**< Reserved */
	
	/*
	 * Electrical parameters block
	*/
	UINT8 IOPinCapacitance;		/**< I/O pin capacitance */
	UINT16 TimingMode;			/**< Timing mode support */
	UINT16 PagecacheTimingMode;	/**< Program cache timing mode */
	
	UINT16 TProg;			/**< Maximum page program time */
	UINT16 TBers;			/**< Maximum block erase time */
	UINT16 TR;				/**< Maximum page read time */
	UINT16 TCcs;			/**< Maximum change column setup time */
	
 	UINT16 SynTimingMode;		/**< ONFI 2.3: Source synchronous timing mode support */
	UINT8 SynFeatures;			/**< ONFI 2.3: Source synchronous features */
	UINT16 ClkInputPinCap;		/**< ONFI 2.3: CLK input pin capacitance */
	UINT16 IOPinCap;			/**< ONFI 2.3: I/O pin capacitance */
	UINT16 InputPinCap;		/**< ONFI 2.3: Input pin capacitance typical */
	UINT8 InputPinCapMax;		/**< ONFI 2.3: Input pin capacitance maximum */
	UINT8 DrvStrength;			/**< ONFI 2.3: Driver strength support */
	
	UINT16 TMr;			/**< ONFI 2.3: Maximum multi-plane read time */
	UINT16 TAdl;			/**< ONFI 2.3: Program page register clear enhancement value */
	UINT16 TEr;			/**< ONFI 2.3: Typical page read time for EZ NAND */
	
	UINT8 Reserved4[6];		/**< Reserved */
	
	/*
	 * Vendor block
	*/
	UINT16 VendorRevisionNum;		/**< Vendor specific revision number */
	UINT8 VendorSpecific[88];		/**< Vendor specific */
	
	UINT16 Crc;     /**< Integrity CRC */	
} vxbT_PARAMPAGE_FLASH;


typedef struct
{
	UINT32 offset_block;		/**< Block offset where BBT resides */
	UINT32 offset_sig;			/**< Signature offset in Spare area */
	UINT32 offset_ver;			/**< Offset of BBT version */
	
	UINT32 len_sig; 			/**< Length of the signature */
	UINT32 max_block;			/**< Max blocks to search for BBT */
	
	char signature[4];			/**< BBT signature */
	UINT8 version;				/**< BBT version */
	
	UINT32 valid;				/**< BBT descriptor is valid or not */
	
} vxbT_BBT_DESC; 	 /* Bad block table descriptor */


typedef struct 
{
	/** Identifier for the device. */
	UINT32 Manufacture;
	UINT32 DevId;
	
	/** Special options for the NandFlash. */
	UINT32 Options;
	
	/* Width of IO */
	UINT32 IoWidth;
	
	/** Size of the data area of a page, in bytes. */
	UINT32 PageSize;
	
	/** Size of the spare area of a page, in bytes. */
	UINT32 SpareSize;
	
	/** Number of pages per block */
	UINT32 PagePerBlock;
	
	/** Size of one block in kilobytes. */
	UINT32 BlockSizeKB;  /* KB size*/
	
	/** Number of blocks per lun */
	UINT32 BlockPerLun;
	
	/** Size of the device in MB. */
	UINT32 DevSizeMB;  /* MB size*/
	
	/** Number of pages in total device */
	UINT32 PageNum;
	
	/** Number of blocks in total device */
	UINT32 BlockNum;
	
	/** LUN Numbers in total device*/
	UINT32 LunNum;
	
	/** Address cycles */
	UINT32 RowAddrCycle;
	UINT32 ColAddrCycle;
	
} vxbT_NAND_INFO;


typedef struct 
{
	UINT32 options; 		/**< Options to search the bad block pattern */
	UINT32 offset;			/**< Offset to search for specified pattern */
	UINT32 length;			/**< Number of bytes to check the pattern */
	UINT8 pattern[2];		/**< Pattern format to search for */
	
} vxbT_BB_PATTERN;



/* structure holding the instance specific details */

typedef struct fm_nfc_drv_ctrl
{
    VXB_DEVICE_ID    pDev;
    void *           regBase;
    void *           regHandle;
	
    UINT32           clkFrequency;	
    BOOL             polling;
    BOOL             dmaMode;
    BOOL             initDone;	
	int              initPhase;

	int ctrl_x;

	UINT16 devId;				/**< Unique ID of device */
	UINT32 cfgBaseAddr;		    /**< APB Base address of the device */
	UINT32 sysClk;				/**< Input clock frequency */
	UINT32 busWidth;			/**< Input clock frequency */
	
	/*===================================*/

    UINT32 eccMode;                 /**< ECC Mode */
	 
	vxbT_NAND_INFO   NAND;              /*  Part geometry */
	
	UINT32 res[16];                    /* for data error */
	
	vxbT_BBT_DESC    BBT_DESC;           /* Bad block table descriptor */
	vxbT_BBT_DESC    BBT_DESC_MIRROR;    /* Mirror BBT descriptor */

	vxbT_BB_PATTERN  BB_PATTERN;         /* Bad block pattern */
	
    UINT8   info_BB[NAND_MAX_BLOCKS >> 2];
    UINT32  map_BB[NAND_MAX_BADBLKS];                           /**< Nandflash Bad Block Info */
	/*===================================*/

	UINT32 devIP_ver;
    UINT32 devIP_type;      /* identify peripheral types:  */

	int status;

    SEM_ID           semSync;
    SEM_ID           muxSem;	
} FM_NFC_DRV_CTRL;

#if 1
UINT32 vxFlashCmdIdx_to_NfcRegVal(UINT32 flash_cmd_idx);
void vxNfcCtrl_Wr_CfgReg32(VXB_DEVICE_ID pDev, UINT32 offset, UINT32 value);
UINT32 vxNfcCtrl_Rd_CfgReg32(VXB_DEVICE_ID pDev, UINT32 offset);
void vxNfcCtrl_Wr_Fifo(VXB_DEVICE_ID pDev, UINT32 data);
UINT32 vxNfcCtrl_Rd_Fifo(VXB_DEVICE_ID pDev);
void vxNfcCtrl_Clr_Fifo(VXB_DEVICE_ID pDev);
void vxNfcCtrl_Wr_CtrlReg(VXB_DEVICE_ID pDev, UINT32 ctrl_val);
UINT32 vxNfcCtrl_Rd_CtrlReg(VXB_DEVICE_ID pDev);
void vxNfcCtrl_Wr_IrqMask(VXB_DEVICE_ID pDev, UINT32 mask);
UINT32 vxNfcCtrl_Rd_IrqMask(VXB_DEVICE_ID pDev);
void vxNfcCtrl_Clr_IrqStatus(VXB_DEVICE_ID pDev);
UINT32 vxNfcCtrl_Rd_IrqStatus(VXB_DEVICE_ID pDev);
void vxNfcCtrl_Wr_StatusMask(VXB_DEVICE_ID pDev, UINT32 mask);
void vxNfcCtrl_Set_Timing(VXB_DEVICE_ID pDev);
int vxNfcCtrl_Get_FlashInfo(VXB_DEVICE_ID pDev);
int vxNfcCtrl_Wait_ExeOk(VXB_DEVICE_ID pDev, UINT32 offset, UINT32 regMask, UINT8 ok_flag);
int vxNfcCtrl_Set_IoWidth(VXB_DEVICE_ID pDev, UINT32 width);
int vxNfcCtrl_Set_BlockSize(VXB_DEVICE_ID pDev, UINT32 size);
int vxNfcCtrl_Set_EccBlockSize(VXB_DEVICE_ID pDev, UINT32 size);
void vxNfcCtrl_En_Ecc(VXB_DEVICE_ID pDev);
void vxNfcCtrl_Dis_Ecc(VXB_DEVICE_ID pDev);
void vxNfcCtrl_Wr_EccOffset(VXB_DEVICE_ID pDev, UINT16 offset);
UINT16 vxNfcCtrl_Rd_EccOffset(VXB_DEVICE_ID pDev);
int vxNfcCtrl_Set_EccCtrl(VXB_DEVICE_ID pDev, UINT32 sel, UINT32 threshold, UINT32 cap);
int vxNfcCtrl_Set_EccMode(VXB_DEVICE_ID pDev, UINT32 eccMode);
void vxNfcCtrl_Init_ReMap(VXB_DEVICE_ID pDev);
void vxNfcCtrl_Set_Dev0PTR(VXB_DEVICE_ID pDev, UINT32 address);
void vxNfcCtrl_Set_Dev0Size(VXB_DEVICE_ID pDev, UINT32 size);

int vxNfcFlash_Rst_NandChip(VXB_DEVICE_ID pDev);
UINT32 vxNfcFlash_Get_OnfiId(VXB_DEVICE_ID pDev);
int vxNfcFlash_Get_ChipId(VXB_DEVICE_ID pDev, UINT32* flashID);
int vxNfcFlash_Get_ParamPage(VXB_DEVICE_ID pDev, void* ParamPage);
int vxNfcFlash_Chk_FlashId(VXB_DEVICE_ID pDev);
UINT32 vxNfcFlash_Get_Feature(VXB_DEVICE_ID pDev, UINT8 feature_in);
int vxNfcFlash_Set_Feature(VXB_DEVICE_ID pDev, UINT8 feature, UINT32 Value);
int vxNfcFlash_Read_Page(VXB_DEVICE_ID pDev, UINT16 block, UINT16 page, UINT16 offset, void* pDataBuf, void* pSpareBuf);
int vxNfcFlash_Write_Page(VXB_DEVICE_ID pDev, UINT16 block, UINT16 page, UINT16 offset, void* pDataBuf, void* pSpareBuf);
int vxNfcFlash_Read_MultiPage(VXB_DEVICE_ID pDev, UINT64 srcAddr, UINT32 byteCnt, UINT8* pDstBuf, UINT8* pUsrSpare);
int vxNfcFlash_Write_MultiPage(VXB_DEVICE_ID pDev, UINT64 dstAddr, UINT32 byteCnt, UINT8 *pSrcBuf, UINT8 *pUsrSpare);
int vxNfcFlash_Erase_Block(VXB_DEVICE_ID pDev, UINT32 block_idx);

void vxNfcSlcr_Rst_NfcCtrl(VXB_DEVICE_ID pDev);
void vxNfcSlcr_Set_NfcMIO(void);

#endif


#ifdef __cplusplus
}
#endif

#endif /* __INC_vxbFmNfc_h__ */


