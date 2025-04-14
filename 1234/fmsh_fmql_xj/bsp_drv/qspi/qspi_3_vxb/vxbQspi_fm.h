/* vxbQspi_fm.h - Freescale AM335x SPI hardware defintions */

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

#ifndef __INCvxbQspi_fm_h__
#define __INCvxbQspi_fm_h__

#ifdef __cplusplus
extern "C"
{
#endif

/* defines */

#define FM_QSPI_NAME	"fmqspi"

#define  OK_FLAG_0     (0)
#define  OK_FLAG_1     (1)

#define  QSPI_CTRL_0   (0)
#define  QSPI_CTRL_1   (1)

#define QSPI_APB_RST_MASK         (0x01)
#define QSPI_AHB_RST_MASK         (0x02)
#define QSPI_REF_RST_MASK         (0x04)

/*
qspi ctrl register definitions
*/
#define QSPIPS_CFG_OFFSET 	    (0x00)	    /**< QSPI Configuration Register */
#define QSPIPS_DRIR_OFFSET	    (0x04)	    /**< Device Read Instruction Register */
#define QSPIPS_DWIR_OFFSET	    (0X08)	    /**< Device Write Instruction Register */
#define QSPIPS_DDR_OFFSET 	    (0x0C)	    /**< QSPI Device Delay Register */
#define QSPIPS_RDCR_OFFSET	    (0x10)	    /**< Read Data Capture Register */
#define QSPIPS_DSCR_OFFSET	    (0x14)	    /**< Device Size Register */
#define QSPIPS_SPCR_OFFSET	    (0x18)	    /**< SRAM Partition Register */
#define QSPIPS_IAATR_OFFSET	    (0x1C)	    /**< Indirect AHB Address Trigger Register */
#define QSPIPS_DPCR_OFFSET	    (0x20)	    /**< DMA Peripheral Register */
#define QSPIPS_RAR_OFFSET	    (0x24)	    /**< Remap Address Register */
#define QSPIPS_MBCR_OFFSET	    (0x28)	    /**< Mode Bit Register */
#define QSPIPS_SFLR_OFFSET	    (0x2C)      /**< SRAM Fill Level Register */
#define QSPIPS_TTR_OFFSET	    (0x30)      /**< TX Threshold Register */
#define QSPIPS_RTR_OFFSET	    (0x34)      /**< RX Threshold Register */
#define QSPIPS_WCCR_OFFSET      (0x38)      /**< Write Completion Control Register */
#define QSPIPS_PER_OFFSET       (0x3C)      /**< Polling Expiration Register */
#define QSPIPS_ISR_OFFSET	    (0x40)      /**< Interrupt Status Register */
#define QSPIPS_IMR_OFFSET	    (0x44)      /**< Interrupt MASK Register */
#define QSPIPS_LWPR_OFFSET	    (0x50)      /**< Lower Write Protection Register */
#define QSPIPS_UWPR_OFFSET      (0x54)      /**< Upper Write Protection Register */
#define QSPIPS_WPR_OFFSET	    (0x58)      /**< Write Protection Register */
#define QSPIPS_IRTCR_OFFSET	    (0x60)      /**< Indirect Read Transfer Register */
#define QSPIPS_IRTWR_OFFSET	    (0x64)      /**< Indirect Read Transfer Watermark Register */
#define QSPIPS_IRTSAR_OFFSET	(0x68)      /**< Indirect Read Transfer Start Address */
#define QSPIPS_IRTNBR_OFFSET	(0x6C)      /**< Indirect Read Transfer Number Bytes  */
#define QSPIPS_IWTCR_OFFSET	    (0x70)      /**< Indirect Write Transfer Register */
#define QSPIPS_IWTWR_OFFSET	    (0x74)      /**< Indirect Write Transfer Watermark Register */
#define QSPIPS_IWTSAR_OFFSET	(0x78)      /**< Indirect Write Transfer Start Address Register */
#define QSPIPS_IWTNBR_OFFSET	(0x7C)      /**< Indirect Read Transfer Number Bytes Register */
#define QSPIPS_ITARR_OFFSET	    (0x80)      /**< Indirect Trigger Address Range Register */
#define QSPIPS_FCCMR_OFFSET	    (0x8C)      /**< Flash Command Control Memory Register */
#define QSPIPS_FCCR_OFFSET	    (0x90)      /**< Flash Command Control Register */
#define QSPIPS_FCAR_OFFSET	    (0x94)      /**< Flash Command Address Register */
#define QSPIPS_FCRDRL_OFFSET	(0xA0)      /**< Flash Command Read Data Register (Lower) */
#define QSPIPS_FCRDRU_OFFSET    (0xA4)      /**< Flash Command Read Data Register (Upper) */
#define QSPIPS_FCWDRL_OFFSET	(0xA8)      /**< Flash Command Write Data Register (Lower) */
#define QSPIPS_FCWDRU_OFFSET	(0xAC)      /**< Flash Command Write Data Register (Upper) */
#define QSPIPS_PFSR_OFFSET	    (0xB0)      /**< Polling Flash Status Register */
#define QSPIPS_MIR_OFFSET	    (0xFC)      /**< Module ID Register */


/*
bit definition of all registers
*/
#define QSPIPS_REG_FCCR_INPROGRESS_MASK	            (0x2)

/* indirect_read */
#define QSPIPS_IRTCR_COMPLETE_MASK	    (0x20)      /* 0x60 : bit5 R/W Indirect Completion Status (status) */
#define QSPIPS_IRTCR_COMPLETE 	        (1)
#define QSPIPS_IRTCR_SRAM_FULL_MASK	    (0x08)      /* 0x60 : bit3 R/W SRAM full */

/* indirect_write */
#define QSPIPS_IWTCR_COMPLETE_MASK      (0x20)      /* 0x70 : bit5 R/W Indirect Completion Status (status) */
#define QSPIPS_IWTCR_COMPLETE 	        (1)

#define QSPIPS_ENABLE_MASK              (0x1)
#define QSPIPS_CLOCKFORMAT_MASK         (0x3 << 1)    
#define QSPIPS_DAC_MASK                 (0x1 << 7)      
#define QSPIPS_LEGACYIP_MASK            (0x1 << 8) 
#define QSPIPS_WPPIN_MASK               (0x1 << 14)     
#define QSPIPS_DMA_MASK                 (0x1 << 15)
#define QSPIPS_REMAP_MASK               (0x1 << 16)
#define QSPIPS_XIPN_MASK                (0x1 << 17)
#define QSPIPS_XIPI_MASK                (0x1 << 18)   
#define QSPIPS_BAUD_MASK                (0xf << 19)    
#define QSPIPS_DTR_MASK                 (0x1 << 24)  
#define QSPIPS_IDLE_MASK                (0x1 << 31)

#define QSPIPS_CLOCKFORMAT_SHIFT        (1)                                         
#define QSPIPS_BAUD_SHIFT               (19)     
#define QSPIPS_BAUD(baud)               ((baud>>1)-1)

/*#define QSPI_TIMEOUT    50000*/
#define QSPI_TIMEOUT    50000*2

#define QSPI_DIRECT_MAP_SIZE                    (0x01000000)   /* 16M*/

#define  SPI_FLASH_PAGE_SIZE     0x100          /* 256 bytes/page */
#define  SPI_FLASH_MEM_SIZE      0x2000000      /* 32M */
#define  SPI_FLASH_SECTOR_SIZE   0x10000        /* 64K */

/***************************************************************************** 
* Configuration Register
* This register contains various control bits that
* affect the operation of the QSPI device. Read/Write.
*
******************************************************************************/
#define QSPIPS_ENABLE_MASK              (0x1)
#define QSPIPS_CLOCKFORMAT_MASK         (0x3 << 1)    
#define QSPIPS_DAC_MASK                 (0x1 << 7)      
#define QSPIPS_LEGACYIP_MASK            (0x1 << 8) 
#define QSPIPS_WPPIN_MASK               (0x1 << 14)     
#define QSPIPS_DMA_MASK                 (0x1 << 15)
#define QSPIPS_REMAP_MASK               (0x1 << 16)
#define QSPIPS_XIPN_MASK                (0x1 << 17)
#define QSPIPS_XIPI_MASK                (0x1 << 18)   
#define QSPIPS_BAUD_MASK                (0xf << 19)    
#define QSPIPS_DTR_MASK                 (0x1 << 24)  
#define QSPIPS_IDLE_MASK                (0x1 << 31)

#define QSPIPS_CLOCKFORMAT_SHIFT        (1)                                         
#define QSPIPS_BAUD_SHIFT               (19)     
#define QSPIPS_BAUD(baud)               ((baud>>1)-1)
     
#define QSPIPS_CFG_RST_STATE            (0x80780081)       
#define QSPIPS_CFG_RESET_MASK_SET       (QSPIPS_ENABLE_MASK | QSPIPS_DAC_MASK)
#define QSPIPS_CFG_RESET_MASK_CLR       (QSPIPS_CLOCKMODE_MASK | \
                                            QSPIPS_LEGACYIP_MASK | \
                                            QSPIPS_DMA_MASK | \
                                            QSPIPS_REMAP_MASK | \
                                            QSPIPS_XIPI_MASK | \
                                            QSPIPS_XIPN_MASK | \
                                            QSPIPS_DTR_MASK)
                                    
/*****************************************************************************
* Device Read Instruction Register
*
******************************************************************************/ 

#define QSPIPS_DR_RST_STATE                         (0x00000003)
#define QSPIPS_DR_FASTREAD_STATE                    (0x0800000B)
#define QSPIPS_DR_DUAL_OUTPUT_STATE                 (0x0801003B)                                   
#define QSPIPS_DR_QUAD_OUTPUT_STATE                 (0x0802006B)                                 
     
/*****************************************************************************
* Read Data Capture Register
*
******************************************************************************/ 

#define QSPIPS_RC_CAPTURE_DELAY_MASK                (0xf << 1)
#define QSPIPS_RC_CAPTURE_DELAY_SHIFT               (1)
	                                
/*****************************************************************************
* Device Size Configuration Register
*
******************************************************************************/ 

#define QSPIPS_DS_ADDRSIZE_MASK                     (0xf)                                    
#define QSPIPS_DS_PAGESIZE_MASK                     (0xfff << 4)
#define QSPIPS_DS_BLOCKSIZE_MASK                    (0x1f << 16)

#define QSPIPS_DS_PAGESIZE_SHIFT                    (4)                                    
#define QSPIPS_DS_BLOCKSIZE_SHIFT                   (16)
                                    
#define QSPIPS_DS_RST_STATE                         (0x00001002)     	                               


/*
flash addr-space:
-----------------
1. 3-bytes_addr_mode or 4-bytes_addr_mode
2. flash size <=16M bytes must be 3-bytes_addr_mode
3. size > 16M, maybe 4-bytes_addr_mode and indirect mode
4. sram or xip mode: must be 3-bytes_addr_mode
*/
#define ADDR_3_BYTES_MODE     (3)
#define ADDR_4_BYTES_MODE     (4)
		
/*
flash read&write interface: single/quad line mode
*/
#define FLASH_4_LINE_MODE     (4)
#define FLASH_1_LINE_MODE     (1)
		
		
#define  VX_QSPI_0_CFG_BASE   (0xE0000000)
#define  VX_QSPI_1_CFG_BASE   (0xE0020000)
		
#define  VX_QSPI_0_FIFO_BASE   (0xE8000000)
#define  VX_QSPI_1_FIFO_BASE   (0xE9000000)
		
#define  OK_FLAG_0     (0)
#define  OK_FLAG_1     (1)
		
#define  QSPI_CTRL_0   (0)
#define  QSPI_CTRL_1   (1)
		
#define QSPIFLASH_PAGE_256                 (0x00000100)
		
#define QSPIFLASH_SECTOR_64K                (0x00010000)
#define QSPIFLASH_SECTOR_256K               (0x00040000)    
		
#define QSPIFLASH_SIZE_16M                  (0x00200000)
#define QSPIFLASH_SIZE_32M                  (0x00400000)
		
#define QSPI_APB_RST_MASK         (0x01)
#define QSPI_AHB_RST_MASK         (0x02)
#define QSPI_REF_RST_MASK         (0x04)
		
#define  QSPI_CTRL_INIT_OK        (1)
#define  QSPI_CTRL_INIT_ERR       (0)
		
#define  QSPI_FSR                  (0x1 << 1)    /* QSPI_FLASH_FLAG_STATUS*/
		
#define  SIZE_16M                  (0x1000000)   /* 16M*/
		
		
/*
4. highAddr: 0x00 - 0 ~ 16M
			 0x01 - 16 ~ 32M
			 0x02 - 32 ~ 48M
			 0x03 - 48 ~ 64M
*/
#define SEG_3B_ADDR_0_16M    (0x00)
#define SEG_3B_ADDR_16_32M   (0x01)
#define SEG_3B_ADDR_32_48M   (0x02)
#define SEG_3B_ADDR_48_64M   (0x03)
		
#define SEG_FLAG_0_16M    (0x10)
#define SEG_FLAG_16_32M   (0x11)
#define SEG_FLAG_32_48M   (0x12)
#define SEG_FLAG_48_64M   (0x13)


#if 0
#define QSPI_SEM_TAKE(sem)      \
        do { \
           if (sem != NULL)    \
           semTake (sem, WAIT_FOREVER);    \
        } while (0)

#define QSPI_SEM_GIVE(sem)      \
        do { \
           if (sem != NULL)    \
           semGive (sem);      \
        } while (0)
#else
#define QSPI_SEM_TAKE(sem) 
#define QSPI_SEM_GIVE(sem) 
#endif

/* typedefs */
		   
typedef struct 
{
    UINT8  hasIntr;
    UINT8  hasDma;
    UINT8  hasXIP;
	
    UINT32 txEmptyLvl;
    UINT32 rxFullLvl;
	
    UINT8  rdMode;  
    UINT32 baudRate; 
    UINT8  sampleDelay;
    UINT8  addrBytesMode;
	
} vxbT_QSPI_CTRL_PARAMS;


typedef struct
{
    UINT8  burstType;
    UINT8  singleType;
    UINT8  txIf;
    UINT8  rxIf;
    UINT32 io; 
} vxbT_QSPI_DMA_PARAMS;


/* structure holding the instance specific details */

typedef struct fm_qspi_drv_ctrl
{
    VXB_DEVICE_ID    pDev;
	
    void *           regBase;
    void *           fifoBase;  /* jc	*/
    void *           regHandle;
	
    UINT32           clkFrequency;	
    BOOL             polling;
    BOOL             dmaMode;
    BOOL             initDone;	
	int              initPhase;

	struct 
	{	
		int ctrl_x;
		
		UINT32 cfgBaseAddr;    /* Config Base address of the device */	
		UINT32 fifoBaseAddr;   /* Data Base address of the device */
		
		int addr_bytes_mode;   /* 4-4bytes_mode, 3-3bytes_mode */
		int dma_mode;		   /* 1-dma, 0-fifo(default) */
		
		int isBusy; 	 /* 1-busy, 0-no busy */
		
		UINT8 * sndBuf;				  /**< Buffer to send (state) */
		UINT8 * rcvBuf ;			  /**< Buffer to receive (state) */
		
		int requestBytes; 			  /**< Number of bytes to transfer (state) */
		int remainBytes; 			  /**< Number of bytes left to transfer(state) */
	} CTRL;

	struct 
	{	
		int status;  /* 1-ok */
		
		UINT32 flash_ID1;
		UINT32 flash_ID2;
		
		UINT32 flagStatus;	/* flag -> flagStatus*/
		
		UINT8  maker;					  /**< Flash maker */
		int    unlock;					  /* lock&protect or unlock*/
		
		UINT32 devSize;					  /**< Flash device size in bytes */
		UINT32 sectSize; 				  /**< Flash sector size in bytes */ 
		UINT32 pageSize;				  /**< Flash page size in bytes */ 
		
		int seg_flag;
	} FLASH;
	
    SEM_ID           semSync;
    SEM_ID           muxSem;	
	
	int init_flag;
} FM_QSPI_DRV_CTRL;

#ifdef __cplusplus
}
#endif

#endif /* __INCvxbQspi_fm_h__ */

