/* vxSpi.h - vxSpi driver header */

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
01a, 20Dec19, jc  written.
*/

#ifndef __INC_VX_SPI_H__
#define __INC_VX_SPI_H__



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define  VX_SPI_0_CFG_BASE   (0xE0001000)
#define  VX_SPI_1_CFG_BASE   (0xE0021000)

#define  SPI_CTRL_0   (0)
#define  SPI_CTRL_1   (1)

#define  MODE_SPI_MASTER   (0)
#define  MODE_SPI_SLAVE    (1)

#define SPI_CKPHA           0x01  /* clock phase */
#define SPI_CKPOL           0x02  /* clock polarity */

#define SPI_MODE_0          (0 | 0)
#define SPI_MODE_1          (0 | SPI_CKPHA)
#define SPI_MODE_2          (SPI_CKPOL | 0)
#define SPI_MODE_3	(SPI_CKPOL | SPI_CKPHA)


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
#define SPIPS_CTRL0_SCPH_MASK       (0x1 << 6)
#define SPIPS_CTRL0_SCPOL_MASK      (0x1 << 7)   

#define SPIPS_CTRL0_TMOD_MASK       (0x3 << 8)    
#define SPIPS_CTRL0_SLVOE_MASK      (0x1 << 10)   
#define SPIPS_CTRL0_SRL_MASK        (0x1 << 11)
#define SPIPS_CTRL0_DFS32_MASK      (0x1f << 16)

#define SPIPS_CTRL0_SCPH_SHIFT      (6)     
#define SPIPS_CTRL0_TMOD_SHIFT      (8)
#define SPIPS_CTRL0_DFS32_SHIFT     (16)


#define SPI0_APB_RST_MASK         (0x01)  /*(0x0)*/
#define SPI1_APB_RST_MASK         (0x02)  /*(0x1)*/
#define SPI0_REF_RST_MASK         (0x04)  /*(0x2)*/
#define SPI1_REF_RST_MASK         (0x08)  /*(0x3)*/


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

#define FPS_SPI_MODE_MASTER         (1)
#define FPS_SPI_MODE_SLAVE          (0)

#define FPS_SPI0_DEVICE_ID          (0)
#define FPS_SPI0_CLK_FREQ_HZ        (40000000)
#define FPS_SPI1_DEVICE_ID          (1)
#define FPS_SPI1_CLK_FREQ_HZ        (40000000)
#define FPS_SPI_NUM_INSTANCES       (2)

#define SPIPS_TRANSFER_DONE         (0x80)


/*
spi ctrl
*/
typedef struct _t_spi_ctrl_
{
	int  ctrl_x;  /* 0, 1 */

	int mode_mst_slv;   /* 0-master, 1-slave */
	
	UINT16 devId;				/**< Unique ID of device */
	UINT32 cfgBaseAddr;		/**< APB Base address of the device */
	UINT32 sysClk;				/**< Input clock frequency */
	
    UINT32 flag;
	
    BOOL isEnable;   
    BOOL isBusy;
    BOOL isMaster;                  /**< Master/Slave */  
	
    UINT8 frmSize;
    
    UINT32 totalBytes;
	
    UINT32 remainBytes;
    UINT32 requestBytes;
	
    UINT8* sndBuf;	            /**< Buffer to send (state) */
    UINT8* rcvBuf;	            /**< Buffer to receive (state) */

	int status;
} vxT_SPI_CTRL;


typedef struct _t_spi_
{
	int spi_x;
	
	vxT_SPI_CTRL * pSpiCtrl;
	
} vxT_SPI;
	

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_VX_SPI_H__ */

