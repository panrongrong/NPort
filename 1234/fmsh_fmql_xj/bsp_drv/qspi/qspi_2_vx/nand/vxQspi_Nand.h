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

#ifndef __INC_VX_QSPI_NAND_H__
#define __INC_VX_QSPI_NAND_H__



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


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
/*#define QSPIFLASH_PAGE_256                 (0x00000100)  // 256 : nor flash*/
#define QSPIFLASH_PAGE_256                 (0x00000800)   /* 2048 : nand flash*/

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

#define FMSH_QUAD_EN_MASK                   (0x02) /*< status2 bit 2 */
#define GD_QUAD_EN_MASK                     (0x02) /*< status2 bit 2 */

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
nand flash special
*/
#define PAGE_NUM (2048*64)

#define PAGE_SIZE 2048
#define BLOCK_NUM 2048

#define BIT(nr)			(1UL << (nr))
#define EINVAL 1

/* Operation timeout value */
#define CQSPI_TIMEOUT_MS			    500
#define CQSPI_READ_TIMEOUT_MS			10

/* Instruction type */
#define CQSPI_INST_TYPE_SINGLE			0
#define CQSPI_INST_TYPE_DUAL			1
#define CQSPI_INST_TYPE_QUAD			2

#define CQSPI_DUMMY_CLKS_PER_BYTE		8
#define CQSPI_DUMMY_BYTES_MAX			4
#define CQSPI_DUMMY_CLKS_MAX			31

#define CQSPI_STIG_DATA_LEN_MAX			8
#define CQSPI_STIG_ADDR_LEN_MAX			3

/* Register map */
#define CQSPI_REG_CONFIG			0x00
	#define CQSPI_REG_CONFIG_ENABLE_MASK		BIT(0)
	#define CQSPI_REG_CONFIG_DECODE_MASK		BIT(9)
	#define CQSPI_REG_CONFIG_CHIPSELECT_LSB		10
	#define CQSPI_REG_CONFIG_DMA_MASK		    BIT(15)
	#define CQSPI_REG_CONFIG_BAUD_LSB		    19
	#define CQSPI_REG_CONFIG_IDLE_LSB		    31
	#define CQSPI_REG_CONFIG_CHIPSELECT_MASK	0xF
	#define CQSPI_REG_CONFIG_BAUD_MASK		    0xF

#define CQSPI_REG_RD_INSTR			0x04
	#define CQSPI_REG_RD_INSTR_OPCODE_LSB		0
	#define CQSPI_REG_RD_INSTR_TYPE_INSTR_LSB	8
	#define CQSPI_REG_RD_INSTR_TYPE_ADDR_LSB	12
	#define CQSPI_REG_RD_INSTR_TYPE_DATA_LSB	16
	#define CQSPI_REG_RD_INSTR_MODE_EN_LSB		20
	#define CQSPI_REG_RD_INSTR_DUMMY_LSB		24
	#define CQSPI_REG_RD_INSTR_TYPE_INSTR_MASK	0x3
	#define CQSPI_REG_RD_INSTR_TYPE_ADDR_MASK	0x3
	#define CQSPI_REG_RD_INSTR_TYPE_DATA_MASK	0x3
	#define CQSPI_REG_RD_INSTR_DUMMY_MASK		0x1F

#define CQSPI_REG_WR_INSTR			0x08
	#define CQSPI_REG_WR_INSTR_OPCODE_LSB		0
	#define CQSPI_REG_WR_INSTR_TYPE_ADDR_LSB	12
	#define CQSPI_REG_WR_INSTR_TYPE_DATA_LSB	16

#define CQSPI_REG_DELAY				0x0C
	#define CQSPI_REG_DELAY_TSLCH_LSB		0
	#define CQSPI_REG_DELAY_TCHSH_LSB		8
	#define CQSPI_REG_DELAY_TSD2D_LSB		16
	#define CQSPI_REG_DELAY_TSHSL_LSB		24
	#define CQSPI_REG_DELAY_TSLCH_MASK		0xFF
	#define CQSPI_REG_DELAY_TCHSH_MASK		0xFF
	#define CQSPI_REG_DELAY_TSD2D_MASK		0xFF
	#define CQSPI_REG_DELAY_TSHSL_MASK		0xFF

#define CQSPI_REG_READCAPTURE			0x10
	#define CQSPI_REG_READCAPTURE_BYPASS_LSB	0
	#define CQSPI_REG_READCAPTURE_DELAY_LSB		1
	#define CQSPI_REG_READCAPTURE_DELAY_MASK	0xF

#define CQSPI_REG_SIZE				0x14
	#define CQSPI_REG_SIZE_ADDRESS_LSB		0
	#define CQSPI_REG_SIZE_PAGE_LSB			4
	#define CQSPI_REG_SIZE_BLOCK_LSB		16
	#define CQSPI_REG_SIZE_ADDRESS_MASK		0xF
	#define CQSPI_REG_SIZE_PAGE_MASK		0xFFF
	#define CQSPI_REG_SIZE_BLOCK_MASK		0x1F

#define CQSPI_REG_SRAMPARTITION			0x18
#define CQSPI_REG_INDIRECTTRIGGER		0x1C

#define CQSPI_REG_DMA				0x20
	#define CQSPI_REG_DMA_SINGLE_LSB		0
	#define CQSPI_REG_DMA_BURST_LSB			8
	#define CQSPI_REG_DMA_SINGLE_MASK		0xFF
	#define CQSPI_REG_DMA_BURST_MASK		0xFF

#define CQSPI_REG_REMAP				0x24
#define CQSPI_REG_MODE_BIT			0x28

#define CQSPI_REG_SDRAMLEVEL			0x2C
	#define CQSPI_REG_SDRAMLEVEL_RD_LSB		0
	#define CQSPI_REG_SDRAMLEVEL_WR_LSB		16
	#define CQSPI_REG_SDRAMLEVEL_RD_MASK		0xFFFF
	#define CQSPI_REG_SDRAMLEVEL_WR_MASK		0xFFFF

#define CQSPI_REG_WRCPLTCTRL			0x38   /*Write Completion Control Register*/
	#define CQSPI_REG_WRCPLTCTRL_DIS_POLL_MASK		BIT(14)


#define CQSPI_REG_IRQSTATUS			    0x40
#define CQSPI_REG_IRQMASK			    0x44

#define CQSPI_REG_INDIRECTRD			0x60
	#define CQSPI_REG_INDIRECTRD_START_MASK		BIT(0)
	#define CQSPI_REG_INDIRECTRD_CANCEL_MASK	BIT(1)
	#define CQSPI_REG_INDIRECTRD_DONE_MASK		BIT(5)

#define CQSPI_REG_INDIRECTRDWATERMARK		0x64
#define CQSPI_REG_INDIRECTRDSTARTADDR		0x68
#define CQSPI_REG_INDIRECTRDBYTES		    0x6C

#define CQSPI_REG_CMDCTRL			0x90
	#define CQSPI_REG_CMDCTRL_EXECUTE_MASK		BIT(0)
	#define CQSPI_REG_CMDCTRL_INPROGRESS_MASK	BIT(1)
	#define CQSPI_REG_CMDCTRL_DUM_CYCLE_LSB		7
	#define CQSPI_REG_CMDCTRL_WR_BYTES_LSB		12
	#define CQSPI_REG_CMDCTRL_WR_EN_LSB		    15
	#define CQSPI_REG_CMDCTRL_ADD_BYTES_LSB		16
	#define CQSPI_REG_CMDCTRL_ADDR_EN_LSB		19
	#define CQSPI_REG_CMDCTRL_RD_BYTES_LSB		20
	#define CQSPI_REG_CMDCTRL_RD_EN_LSB		    23
	#define CQSPI_REG_CMDCTRL_OPCODE_LSB		24
	#define CQSPI_REG_CMDCTRL_WR_BYTES_MASK		0x7
	#define CQSPI_REG_CMDCTRL_ADD_BYTES_MASK	0x3
	#define CQSPI_REG_CMDCTRL_RD_BYTES_MASK		0x7

#define CQSPI_REG_INDIRECTWR			0x70
	#define CQSPI_REG_INDIRECTWR_START_MASK		BIT(0)
	#define CQSPI_REG_INDIRECTWR_CANCEL_MASK	BIT(1)
	#define CQSPI_REG_INDIRECTWR_DONE_MASK		BIT(5)

#define CQSPI_REG_INDIRECTWRWATERMARK		0x74
#define CQSPI_REG_INDIRECTWRSTARTADDR		0x78
#define CQSPI_REG_INDIRECTWRBYTES	        0x7C

#define CQSPI_REG_CMDADDRESS			   0x94
#define CQSPI_REG_CMDREADDATALOWER		   0xA0
#define CQSPI_REG_CMDREADDATAUPPER		   0xA4
#define CQSPI_REG_CMDWRITEDATALOWER		   0xA8
#define CQSPI_REG_CMDWRITEDATAUPPER	       0xAC


/* 
Interrupt status bits 
*/
#define CQSPI_REG_IRQ_MODE_ERR			BIT(0)
#define CQSPI_REG_IRQ_UNDERFLOW			BIT(1)
#define CQSPI_REG_IRQ_IND_COMP			BIT(2)
#define CQSPI_REG_IRQ_IND_RD_REJECT		BIT(3)
#define CQSPI_REG_IRQ_WR_PROTECTED_ERR	BIT(4)
#define CQSPI_REG_IRQ_ILLEGAL_AHB_ERR	BIT(5)
#define CQSPI_REG_IRQ_WATERMARK			BIT(6)
#define CQSPI_REG_IRQ_IND_SRAM_FULL		BIT(12)

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

/*#define QSPIFLASH_PAGE_256               (0x00000100)    // nor flash: 256*/
#define QSPIFLASH_PAGE_256                 (0x00000800)    /* nand flash: 2048*/

#define QSPIFLASH_SECTOR_64K                (0x00010000)
#define QSPIFLASH_SECTOR_256K               (0x00040000)    

#define QSPIFLASH_SIZE_16M                  (0x00200000)
#define QSPIFLASH_SIZE_32M                  (0x00400000)

#define QSPI_APB_RST_MASK         (0x01)
#define QSPI_AHB_RST_MASK         (0x02)
#define QSPI_REF_RST_MASK         (0x04)

#define  QSPI_CTRL_INIT_OK          (1)
#define  QSPI_CTRL_INIT_ERR         (0)

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
	
	int isBusy; 	 /* 1-busy, 0-no busy */
	
	UINT8 * sndBuf;				  /**< Buffer to send (state) */
	UINT8 * rcvBuf ;			  /**< Buffer to receive (state) */
	
	int requestBytes; 			  /**< Number of bytes to transfer (state) */
	int remainBytes; 			  /**< Number of bytes left to transfer(state) */

} vxT_QSPI_CTRL;

/*
qspi device: flash
*/
typedef struct _t_qspi_nand_
{
	int status;  /* 1-ok */

	UINT32 flash_ID1;
	UINT32 flash_ID2;
	
	UINT32 flag;
	
	UINT8  maker;					  /**< Flash maker */
	
	UINT32 devSize;		  /**< Flash device size in bytes: 2plane * 1024blk * 64pages */
	UINT32 blkSize; 	  /**< Flash block size in bytes: 64pages/blk */ 
	
	UINT32 pageSize; 	  /**< Flash page size in bytes: 2048  */ 	
	UINT32 spareSize; 	  /**< Flash spare size in bytes: 64 */ 	
	
	int pages_1_blk;      /* pages per block: 64 pages/blk */
} vxT_QSPI_NAND;



typedef struct _t_qspi_
{
	int qspi_x;
	
	vxT_QSPI_CTRL * pQspiCtrl;
	
	vxT_QSPI_NAND * pQspiNand;
	
    SEM_ID      sem;
	
	int init_flag;
} vxT_QSPI;
	

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_VX_QSPI_NAND_H__ */

