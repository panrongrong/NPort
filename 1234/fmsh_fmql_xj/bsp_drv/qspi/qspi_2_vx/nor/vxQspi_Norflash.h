/* vxQspi_Norflash.h - vx_qspi driver header */

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
01a, 16Dec19, jc  written.
*/

#ifndef __INC_VX_QSPI_NORFLASH_H__
#define __INC_VX_QSPI_NORFLASH_H__



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define FSBL_MAP_HIGH_BANK

/************************** Constant Definitions *****************************/

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

/*****************************************************************************
* Interrupt Status Register
*
******************************************************************************/ 

#define QSPIPS_INTR_MODE_FAIL_MASK	                (0x00000001) /**< Mode fail */
#define QSPIPS_INTR_TX_UNDERFLOW_MASK	            (0x00000002) /**<  */
#define QSPIPS_INTR_INDIRECT_COMPLETE_MASK	        (0x00000004) /**<  */
#define QSPIPS_INTR_REQ_REJECT_MASK	                (0x00000008) /**<  */
#define QSPIPS_INTR_WRITE_PROTECT_MASK	            (0x00000010) /**<  */
#define QSPIPS_INTR_ILLEGAL_AHB_ACCESS_MASK	        (0x00000020) /**<  */
#define QSPIPS_INTR_WATERMARK_LEVEL_BREACHED_MASK	(0x00000040) /**<  */
#define QSPIPS_INTR_RX_OVERFLOW_MASK	            (0x00000080) /**<  */
#define QSPIPS_INTR_TX_NOT_FULL_MASK	            (0x00000100) /**<  */
#define QSPIPS_INTR_TX_FULL_MASK	                (0x00000200) /**<  */
#define QSPIPS_INTR_RX_NOT_EMPTY_MASK	            (0x00000400) /**<  */
#define QSPIPS_INTR_RX_FULL_MASK		            (0x00000800) /**<  */
#define QSPIPS_INTR_SRAM_READ_FULL_MASK	            (0x00001000) /**<  */
#define QSPIPS_INTR_POLL_EXPIRED_MASK	            (0x00002000) /**<  */
#define QSPIPS_INTR_STIG_COMPLETE_MASK	            (0x00004000) /**<  */
#define QSPIPS_INTR_ALL	                            (0x00007fff)    
                              
/*****************************************************************************
* Write Protection Register
*
******************************************************************************/ 

#define QSPIPS_WP_ENABLE_MASK                       (0x00000002)
#define QSPIPS_WP_INVERSION_MASK                    (0x00000001)

/*****************************************************************************
* Flash Read Control Register
*
******************************************************************************/

#define QSPIPS_REG_FCCR_INPROGRESS_MASK	            (0x2)

/*****************************************************************************
* SLCR Register
* Bit Masks of above SLCR Registers .
*
******************************************************************************/

#define QSPIPS_RST_CTRL_MASK                        (0x7) /**< Quad SPI Software Reset Control */


/* indirect_read */
#define QSPIPS_IRTCR_COMPLETE_MASK	    (0x20)      /* 0x60 : bit5 R/W Indirect Completion Status (status) */
#define QSPIPS_IRTCR_COMPLETE 	        (1)
#define QSPIPS_IRTCR_SRAM_FULL_MASK	    (0x08)      /* 0x60 : bit3 R/W SRAM full */

/* indirect_write */
#define QSPIPS_IWTCR_COMPLETE_MASK      (0x20)      /* 0x70 : bit5 R/W Indirect Completion Status (status) */
#define QSPIPS_IWTCR_COMPLETE 	        (1)

/****************************
 * qspi flash manufacturer code
 ************/
#define SPI_SPANSION_ID                     (0x01)
#define SPI_MICRON_ID                       (0x20)
#define SPI_WINBOND_ID                      (0xEF)
#define SPI_MACRONIX_ID                     (0xC2)
#define SPI_ISSI_ID                         (0x9D)
#define SPI_FMSH_ID                         (0xA1)
#define SPI_GD_ID                           (0xC8)
#define SPI_UNKNOWN_ID                      (0xFF)  

/*****************************
 * qspi flash size(bits) code
 ************/
#define QSPIFLASH_SIZE_ID_256K              (0x09)
#define QSPIFLASH_SIZE_ID_512K              (0x10)
#define QSPIFLASH_SIZE_ID_1M                (0x11)
#define QSPIFLASH_SIZE_ID_2M                (0x12)
#define QSPIFLASH_SIZE_ID_4M                (0x13)
#define QSPIFLASH_SIZE_ID_8M                (0x14)
#define QSPIFLASH_SIZE_ID_16M               (0x15)
#define QSPIFLASH_SIZE_ID_32M               (0x16)
#define QSPIFLASH_SIZE_ID_64M               (0x17)
#define QSPIFLASH_SIZE_ID_128M              (0x18)
#define QSPIFLASH_SIZE_ID_256M              (0x19)

#define QSPIFLASH_SIZE_ID_512M              (0x20)
#define MACRONIX_QSPIFLASH_SIZE_ID_512M     (0x1A)
#define ISSI_QSPIFLASH_SIZE_ID_512M         (0x1A)
#define QSPIFLASH_SIZE_ID_1G                (0x21)
#define MACRONIX_QSPIFLASH_SIZE_ID_1G       (0x1B)

#define SPI_WRONG_ID                        (0xFF)

/*************************
 * qspi flash size(bytes)
 **************/
#define QSPIFLASH_PAGE_256                 (0x00000100)

#define QSPIFLASH_SECTOR_4K                 (0x00001000)
#define QSPIFLASH_SECTOR_64K                (0x00010000)
#define QSPIFLASH_SECTOR_256K               (0x00040000)    

#define QSPIFLASH_SIZE_256K                 (0x00008000)
#define QSPIFLASH_SIZE_512K                 (0x00010000)    
#define QSPIFLASH_SIZE_1M                   (0x00020000)
#define QSPIFLASH_SIZE_2M                   (0x00040000)
#define QSPIFLASH_SIZE_4M                   (0x00080000)    
#define QSPIFLASH_SIZE_8M                   (0x00100000)
#define QSPIFLASH_SIZE_16M                  (0x00200000)
#define QSPIFLASH_SIZE_32M                  (0x00400000)
#define QSPIFLASH_SIZE_64M                  (0x00800000)    
#define QSPIFLASH_SIZE_128M                 (0x01000000)
#define QSPIFLASH_SIZE_256M                 (0x02000000)
#define QSPIFLASH_SIZE_512M                 (0x04000000)
#define QSPIFLASH_SIZE_1G                   (0x08000000)
#define QSPIFLASH_SIZE_2G                   (0x10000000)

/**************************
 * qspi flash command set (general)
 *************/
#define RDID_CMD                            (0x9F)
#define WREN_CMD                            (0x06)
#define WRDI_CMD                            (0x04)
#define RDSR1_CMD                           (0x05)
#define WRR_CMD                             (0x01)
#define READ_CMD                            (0x03)
#define FAST_READ_CMD                       (0x0B)
#define DOR_CMD                             (0x3B)
#define QOR_CMD                             (0x6B)
#define DIOR_CMD                            (0xBB)
#define QIOR_CMD                            (0xEB)    
#define SE_CMD                              (0xD8)
#define CE_CMD                              (0xC7)
#define PP_CMD                              (0x02)

#define SPANSION_RDSR2_CMD                  (0x07) /*< Read Status Rregister-2 */
#define SPANSION_RDCR_CMD                   (0x35) /*< Read Configuration Rregister*/
#define SPANSION_BRRD_CMD                   (0x16) /*< Read Bank Rregister*/
#define SPANSION_BRWR_CMD                   (0x17) /*< Write Bank Rregister*/
   
#define MICRON_RDNVCR_CMD                   (0xB5) /*< Read Nonvolatile Configuration Rregister*/
#define MICRON_RDVCR_CMD                    (0x85) /*< Read Volatile Configuration Rregister*/
#define MICRON_RDECR_CMD                    (0x65) /*< Read Enhanced Volatile Configuration Rregister*/
#define MICRON_RDEAR_CMD                    (0xC8) /*< Read Enhanced Address Rregister*/
#define MICRON_WRNVCR_CMD                   (0xB1) /*< Write Nonvolatile Configuration Rregister*/
#define MICRON_WRVCR_CMD                    (0x81) /*< Write Volatile Configuration Rregister*/
#define MICRON_WRECR_CMD                    (0x61) /*< Write Enhanced Volatile Configuration Rregister*/
#define MICRON_WREAR_CMD                    (0xC5) /*< Write Enhanced Address Rregister*/

#define WINBOND_RDSR2_CMD                   (0x35) /*< Read Status Rregister-2 */
#define WINBOND_RDSR3_CMD                   (0x15) /*< Read Status Rregister-3 */
#define WINBOND_RDEAR_CMD                   (0xC8) /*< Read Extended Address Rregister */
#define WINBOND_WRSR2_CMD                   (0x31) /*< Write Status Rregister-2 */
#define WINBOND_WRSR3_CMD                   (0x11) /*< Write Status Rregister-3 */
#define WINBOND_WREAR_CMD                   (0xC5) /*< Write Extended Address Rregister */
         
#define MACRONIX_RDCR_CMD                   (0x15) /*< Read Configuration Rregister*/
#define MACRONIX_RDEAR_CMD                  (0xC8) /*< Read Extended Address Rregister */
#define MACRONIX_WREAR_CMD                  (0xC5) /*< Write Extended Address Rregister */
#define MACRONIX_EN4B_CMD                   (0xB7) /*< Enter 4B Mode */
#define MACRONIX_EX4B_CMD                   (0xE9) /*< Exit 4B Mode */
        
#define ISSI_RDPAR_CMD                      (0x61) /*< Read Read Parameter(Volatile) Rregister*/
#define ISSI_RDWRP_CMD                      (0x81) /*< Read Extended Read Parameter(Volatile) Rregister*/
#define ISSI_RDBR_CMD                       (0x16) /*< Read Bank Address Rregister*/
#define ISSI_WRBRV_CMD                      (0x17) /*< Write Bank Address(Volatile) Rregister*/
#define ISSI_WRBRNV_CMD                     (0x18) /*< Write Bank Address(Non-Volatile) Rregister*/        
#define ISSI_SPRNV_CMD                      (0x65) /*< Set Read Parameter(Non-Volatile) Rregister*/
#define ISSI_SPRV_CMD                       (0xC0) /*< Set Read Parameter(Volatile) Rregister*/   
#define ISSI_SEPRNV_CMD                     (0x85) /*< Set Extended Read Parameter(Non-Volatile) Rregister*/
#define ISSI_SEPRV_CMD                      (0x83) /*< Set Extended Read Parameter(Volatile) Rregister*/   
#define ISSI_EN4B_CMD                       (0xB7) /*< Enter 4B Mode */
#define ISSI_EX4B_CMD                       (0x29) /*< Exit 4B Mode */ 
         
#define FMSH_RDSR2_CMD                      (0x35) /*< Read Status Rregister-2 */
#define GD_RDSR2_CMD                        (0x35) /*< Read Status Rregister-2 */

/*************************** 
 * qspi flash register bits
 *****************/   
#define SRWD_MASK                           (0x80) /*< ignore WRR command when WP# is low */   
#define BP_MASK                             (0x1C) /*< block protection */
#define WEL_MASK                            (0x02)
#define BUSY_MASK                           (0x01)    

#define SPANSION_LC_MASK                    (0xC0)
#define SPANSION_QUAD_EN_MASK               (0x02)
#define SPANSION_EXTADDR_MASK               (0x80)     
#define SPANSION_BA24_MASK                  (0x01)

#define MICRON_NV_DUMMY_MASK                (0xF000) /*< dummy cycle numbers */
#define MICRON_SEG_SEL_MASK                 (0x0002) /*< select 128Mb segment at upper(0) or lower(1) */
#define MICRON_EXTADDR_MASK                 (0x0001) /*< select 4B addr(0) or 3B addr(1) */         
#define MICRON_V_DUMMY_MASK                 (0xF0) /*< dummy cycle numbers */

#define WINBOND_QUAD_EN_MASK                (0x02) /*< status2 bit 2 */
#define WINBOND_ADP_MASK                    (0x02)
#define WINBOND_ADS_MASK                    (0x01)
         
#define MACRONIX_QUAD_EN_MASK               (0x40) /*< status bit 6 */
#define MACRONIX_DC_MASK                    (0xC0) /*< cfg bits 6~7 */
#define MACRONIX_4B_MASK                    (0x20) /*< cfg bits 5 */
     
#define ISSI_QUAD_EN_MASK                   (0x40) /*< status bit 6 */
#define ISSI_DUMMY_MASK                     (0x78) /*< read parameter bits 3~6 */
#define ISSI_EXTADDR_MASK                   (0x80)

#define FMSH_RDSR2_CMD                      (0x35) /*< Read Status Rregister-2 */
#define GD_RDSR2_CMD                        (0x35) /*< Read Status Rregister-2 */

#define FMSH_QUAD_EN_MASK                   (0x02) /*< status2 bit 2 */
#define FMSH_WRSR2_CMD                      (0x31) /*< Write Status Rregister-2 */

#define GD_QUAD_EN_MASK                     (0x02) /*< status2 bit 2 */
#define GD_WRSR2_CMD                        (0x31) /*< Write Status Rregister-2 */


/***************************
 * qspi flash delay parameters
 *****************/
#define QSPIFLASH_CSDA                      (0x00)
#define QSPIFLASH_CSDADS                    (0x00) 
#define QSPIFLASH_CSEOT                     (0x00)
#define QSPIFLASH_CSSOT                     (0x00)

/***************************
 * qspiflash mode bits
 *******************/
#define SPANSION_MODE_BIT                   (0xA0)
#define MICRON_MODE_BIT                     (0x00)
#define WINBOND_MODE_BIT                    (0x20)
#define MACRONIX_MODE_BIT                   (0xA5)
#define ISSI_MODE_BIT                       (0xA0)
#define FMSH_MODE_BIT                       (0x20)

/*#define QSPI_TIMEOUT    50000*/
#define QSPI_TIMEOUT    50000*2

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
				   semGive (sem);	   \
				} while (0)
#else
#define QSPI_SEM_TAKE(sem) 
#define QSPI_SEM_GIVE(sem) 
#endif

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
	
} vxT_QSPI_CTRL_PARAMS;

typedef struct
{
    UINT8  burstType;
    UINT8  singleType;
    UINT8  txIf;
    UINT8  rxIf;
    UINT32 io; 
} vxT_QSPI_DMA_PARAMS;



/*
qspi ctrl
*/
typedef struct _t_qspi_ctrl_
{
	int  ctrl_x;  /* 0, 1 */	
	
	UINT16 devId;	       /* Unique ID  of device: 0, 1 */
	UINT32 fifoBaseAddr;   /* Data Base address of the device */
	UINT32 cfgBaseAddr;    /* Config Base address of the device */	

	int addr_bytes_mode;   /* 4-4bytes_mode, 3-3bytes_mode */
	int dma_mode;          /* 1-dma, 0-fifo(default) */
	
	int isBusy; 	 /* 1-busy, 0-no busy */
	
	UINT8 * sndBuf;				  /**< Buffer to send (state) */
	UINT8 * rcvBuf ;			  /**< Buffer to receive (state) */
	
	int requestBytes; 			  /**< Number of bytes to transfer (state) */
	int remainBytes; 			  /**< Number of bytes left to transfer(state) */

} vxT_QSPI_CTRL;

/*
qspi device: flash
*/
typedef struct _t_qspi_flash_
{
	int status;  /* 1-ok */

	UINT32 flash_ID1;
	UINT32 flash_ID2;
	
	UINT32 flagStatus;  /* flag -> flagStatus*/
	
	UINT8  maker;					  /**< Flash maker */
	int    unlock;                    /* lock&protect or unlock*/
	
	UINT32 devSize;					  /**< Flash device size in bytes */
	UINT32 sectSize; 				  /**< Flash sector size in bytes */ 
	UINT32 pageSize; 				  /**< Flash page size in bytes */ 	

	int seg_flag;
	
} vxT_QSPI_FLASH;



typedef struct _t_qspi_
{
	int qspi_x;
	
	vxT_QSPI_CTRL * pQspiCtrl;
	
	vxT_QSPI_FLASH * pQspiFlash;
	
    SEM_ID      sem;
	
	int init_flag;
} vxT_QSPI;
	

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_VX_QSPI_NORFLASH_H__ */

