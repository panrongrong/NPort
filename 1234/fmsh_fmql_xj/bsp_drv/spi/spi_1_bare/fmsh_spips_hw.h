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
* @file fmsh_spips_hw.h
* @addtogroup spips_v1_1
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
* 		     Add defination of SPIPS_CTRL0_MASK.
* 		     Add defination of SPIPS_CTRL0_SCPOL_SHIFT.
* 		     Add defination of SPIPS_CTRL0_SLVOE_SHIFT.
* 		     Add defination of SPIPS_CTRL0_SRL_SHIFT.
*
* </pre>
*
******************************************************************************/

#ifndef _FMSH_SPIPS_HW_H_	/* prevent circular inclusions */
#define _FMSH_SPIPS_HW_H_	/* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif
    
/***************************** Include Files *********************************/ 

/************************** Constant Definitions *****************************/ 

#define SPIPS_MSTR_OFFSET           (0x100)
#define SPIPS_CTRLR0_OFFSET         (0x00)
#define SPIPS_CTRLR1_OFFSET         (0x04)
#define SPIPS_SSIENR_OFFSET         (0x08)
#define SPIPS_MVCR_OFFSET           (0x0C)
#define SPIPS_SER_OFFSET            (0x10)
#define SPIPS_BAUDR_OFFSET          (0x14)
#define SPIPS_TXFTLR_OFFSET         (0x18)
#define SPIPS_RXFTLR_OFFSET         (0x1C)
#define SPIPS_TXFLR_OFFSET          (0x20)
#define SPIPS_RXFLR_OFFSET          (0x24)
#define SPIPS_SR_OFFSET             (0x28)
#define SPIPS_IMR_OFFSET            (0x2C)
#define SPIPS_ISR_OFFSET            (0x30)
#define SPIPS_RISR_OFFSET           (0x34)
#define SPIPS_TXOICR_OFFSET         (0x38)
#define SPIPS_RXOICR_OFFSET         (0x3C)
#define SPIPS_RXUICR_OFFSET         (0x40)
#define SPIPS_MSTICR_OFFSET         (0x44)
#define SPIPS_ICR_OFFSET            (0x48)
#define SPIPS_DMACR_OFFSET          (0x4C)
#define SPIPS_DMATDLR_OFFSET        (0x50)
#define SPIPS_DMARDLR_OFFSET        (0x54)
#define SPIPS_IDR_OFFSET            (0x58)
#define SPIPS_VERSION_OFFSET        (0x5C)
#define SPIPS_DR_OFFSET             (0x60)
#define SPIPS_RX_SAMPLE_OFFSET      (0xf0)
#define SPIPS_SCTRLR0_OFFSET        (0xf4)
#define SPIPS_RSVD1_OFFSET          (0xf8)
#define SPIPS_RSVD2_OFFSET          (0xfc)

/***************************************************************************
* CTRL0
*
****************************************************************************/
#define SPIPS_CTRL0_MASK            (0x1f0fc0)

#define SPIPS_CTRL0_SCPH_MASK       (0x1 << 6)
#define SPIPS_CTRL0_SCPOL_MASK      (0x1 << 7)   

#define SPIPS_CTRL0_TMOD_MASK       (0x3 << 8)    
#define SPIPS_CTRL0_SLVOE_MASK      (0x1 << 10)   
#define SPIPS_CTRL0_SRL_MASK        (0x1 << 11)
#define SPIPS_CTRL0_DFS32_MASK      (0x1f << 16)

#define SPIPS_CTRL0_SCPH_SHIFT      (6)   
#define SPIPS_CTRL0_SCPOL_SHIFT     (7)   
#define SPIPS_CTRL0_TMOD_SHIFT      (8)
#define SPIPS_CTRL0_SLVOE_SHIFT     (10) 
#define SPIPS_CTRL0_SRL_SHIFT       (11)
#define SPIPS_CTRL0_DFS32_SHIFT     (16)

/* clock phase */
#define SPI_CKPHA           0x01
/* clock polarity */
#define SPI_CKPOL           0x02
/* clk_mode */
#undef SPI_MODE_0
#undef SPI_MODE_1
#undef SPI_MODE_2
#undef SPI_MODE_3
#define SPI_MODE_0          (0 | 0)
#define SPI_MODE_1          (0 | SPI_CKPHA)
#define SPI_MODE_2          (SPI_CKPOL | 0)
#define SPI_MODE_3          (SPI_CKPOL | SPI_CKPHA)
/*
*/


#define SPIPS_TRANSFER_STATE        (0x0)   /* tx & rx */
#define SPIPS_TRANSMIT_ONLY_STATE   (0x1)   /* tx only */
#define SPIPS_RECEIVE_ONLY_STATE    (0x2)   /* rx only */
#define SPIPS_EEPROM_STATE          (0x3)   /* eeprom */

/**************************************************************************
* Status & Interrupt
*
***************************************************************************/
#define SPIPS_SR_DCOL               (0x40)  /* bit6 DCOL R Data Collision Error */
#define SPIPS_SR_TXE                (0x20)  /* bit5 TXE  R Transmission Error */
#define SPIPS_SR_RFF                (0x10)  /* bit4 RFF  R Receive FIFO Full */
#define SPIPS_SR_RFNE               (0x08)  /* bit3 RFNE R Receive FIFO Not Empty. */
#define SPIPS_SR_TFE                (0x04)  /* bit2 TFE  R Transmit FIFO Empty */
#define SPIPS_SR_TFNF               (0x02)  /* bit1 TFNF R Transmit FIFO Not Full */
#define SPIPS_SR_BUSY               (0x01)  /* bit0 BUSY R SSI Busy Flag */

#define SPIPS_INTR_MSTIS_MASK       (0x20)   /*  Multi-Master Contention Interrupt (ssi_mst_intr)  */

#define SPIPS_INTR_RXFIS_MASK       (0x10)   /* Receive FIFO Full Interrupt (ssi_rxf_intr) */
#define SPIPS_INTR_RXOIS_MASK       (0x08)   /* Receive FIFO Overflow Interrupt (ssi_rxo_intr) */
#define SPIPS_INTR_RXUIS_MASK       (0x04)   /* Receive FIFO Underflow Interrupt (ssi_rxu_intr) */
/* */
#define SPIPS_INTR_TXOIS_MASK       (0x02)   /* Transmit FIFO Overflow Interrupt (ssi_txo_intr) */
#define SPIPS_INTR_TXEIS_MASK       (0x01)   /* Transmit FIFO Empty Interrupt (ssi_txe_intr) */
/* */
#define SPIPS_INTR_ALL              (0x3f)   /* Combined Interrupt Request (ssi_intr) */

#define SPIPS_FIFO_DEPTH            (0x20)

/**************************** Type Definitions *******************************/   
    
/**********************************Variable Definition**************************/

/**********************************Function Prototype***************************/
    
void FSpiPs_SetMst(FSpiPs_T *spi, u8 setMaster);
void FSpiPs_SetEnable(FSpiPs_T* spi, u8 setEnable);
int FSpiPs_SetSckMode(FSpiPs_T* spi, u32 sckMode);
int FSpiPs_SetTMod(FSpiPs_T* spi, u32 tmod);
int FSpiPs_SetSlvOut(FSpiPs_T* spi, u8 enable);
int FSpiPs_SetLoopBack(FSpiPs_T* spi, u8 enable);
int FSpiPs_SetDFS32(FSpiPs_T* spi, u32 dfs32);
int FSpiPs_SetDFNum(FSpiPs_T* spi, u32 dfNum);
int FSpiPs_SetSlave(FSpiPs_T* spi, u32 slaveNo);
int FSpiPs_SetSckDv(FSpiPs_T* spi, u32 sckdv);
int FSpiPs_SetTxEmptyLvl(FSpiPs_T* spi, u8 tlvl);
u32 FSpiPs_GetTxLevel(FSpiPs_T* spi);
int FSpiPs_SetRxFullLvl(FSpiPs_T* spi, u8 tlvl);
u32 FSpiPs_GetRxLevel(FSpiPs_T* spi);
void FSpiPs_EnableIntr( FSpiPs_T* spi, u32 mask);
void FSpiPs_DisableIntr( FSpiPs_T* spi, u32 mask);
void FSpiPs_ClearIntrStatus(FSpiPs_T* spi);
void FSpiPs_SetDMATLvl(FSpiPs_T* spi, u32 tlvl);
void FSpiPs_SetDMARLvl(FSpiPs_T* spi, u32 tlvl);
void FSpiPs_EnableDMATx(FSpiPs_T* spi);
void FSpiPs_EnableDMARx(FSpiPs_T* spi);
void FSpiPs_DisableDMA(FSpiPs_T* spi);
u32 FSpiPs_GetStatus(FSpiPs_T* spi);
u32 FSpiPs_Recv(FSpiPs_T* spi );
void FSpiPs_Send(FSpiPs_T* spi, u32 data );
     
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* prevent circular inclusions */

