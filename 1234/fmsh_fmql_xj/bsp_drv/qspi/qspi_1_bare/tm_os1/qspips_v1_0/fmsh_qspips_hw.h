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
* @file fmsh_qspips_hw.h
* @addtogroup qspips_v1_1
* @{
*
* This header file contains the identifiers and basic HW access driver
* functions (or macros) that can be used to access the device.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who Date     Changes
* ----- --- -------- -----------------------------------------------
* 1.00  hzq 7/1/19 
* 		     First release
* 1.10  hzq 11/26/20 
* 		     Add prototypes of API, FQspiPs_SetPoll and FQspiPs_DisablePoll.
*
* </pre>
*
******************************************************************************/

#ifndef _FMSH_QSPIPS_HW_H_	/* prevent circular inclusions */ 
#define _FMSH_QSPIPS_HW_H_	/* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/
    
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

/***************** Macros (Inline Functions) Definitions *********************/  
        
/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

void FQspiPs_Enable(FQspiPs_T* qspi);
void FQspiPs_Disable(FQspiPs_T* qspi);
void FQspiPs_EnableDAC(FQspiPs_T* qspi);
void FQspiPs_DisableDAC(FQspiPs_T* qspi);
void FQspiPs_EnableLegacy(FQspiPs_T* qspi);
void FQspiPs_DisableLegacy(FQspiPs_T* qspi);
void FQspiPs_EnableDMA(FQspiPs_T* qspi);
void FQspiPs_DisableDMA(FQspiPs_T* qspi);
void FQspiPs_EnableRemap(FQspiPs_T* qspi);
void FQspiPs_DisableRemap(FQspiPs_T* qspi);
void FQspiPs_EnableXip(FQspiPs_T* qspi);
void FQspiPs_DisableXip(FQspiPs_T* qspi);
void FQspiPs_EnableProtect(FQspiPs_T* qspi, int inv);
void FQspiPs_DisableProtect(FQspiPs_T* qspi);
void FQspiPs_SetClockFormat(FQspiPs_T* qspi, uint32_t clockFormat);
void FQspiPs_SetBaudRate(FQspiPs_T* qspi, uint32_t baudRate);
void FQspiPs_SetAddrBytesNum(FQspiPs_T* qspi, uint32_t addrSize);
void FQspiPs_SetPageSize(FQspiPs_T* qspi, uint32_t pageSize);
void FQspiPs_SetBlockSize(FQspiPs_T* qspi, uint32_t blockSize);
void FQspiPs_SetCaptureDelay(FQspiPs_T* qspi, uint32_t cycles);
void FQspiPs_SetRemap(FQspiPs_T* qspi, uint32_t offset);
void FQspiPs_SetLowBlock(FQspiPs_T* qspi, uint32_t lowBlock);
void FQspiPs_SetHighBlock(FQspiPs_T* qspi, uint32_t highBlock);
void FQspiPs_SetTxNotFullLvl(FQspiPs_T* qspi, uint32_t threshold);
void FQspiPs_SetRxNotEmptyLvl(FQspiPs_T* qspi, uint32_t threshold);
void FQspiPs_SetModeBits(FQspiPs_T* qspi, u8 modeBits);
void FQspiPs_EnableIntr(FQspiPs_T* qspi, uint32_t mask);
void FQspiPs_DisableIntr(FQspiPs_T* qspi, uint32_t mask);
void FQspiPs_ClearIntr(FQspiPs_T* qspi, uint32_t mask);
uint32_t FQspiPs_IntrEnabled(FQspiPs_T* qspi);
void FQspiPs_SetDelays(FQspiPs_T* qspi, u8 csda, u8 csdads, u8 cseot, u8 cssot);
void FQspiPs_GetDelays(FQspiPs_T* qspi, u8* csdaPtr, u8* csdadsPtr, u8* cseotPtr, u8* cssotPtr);
void FQspiPs_SetPoll(FQspiPs_T* qspi, u8 opCode, u8 pollBit, int polarity);
void FQspiPs_DisablePoll(FQspiPs_T* qspi);
int FQspiPs_CmdExecute(FQspiPs_T* qspi, uint32_t cmd);
int FQspiPs_WaitIdle(FQspiPs_T* qspi);
int FQspiPs_WaitForBit(FQspiPs_T* qspi, uint32_t regOffset, uint32_t mask, u8 pollBit);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* prevent circular inclusions */c