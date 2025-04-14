/* vxI2c.h - vxI2c driver header */

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
01a, 04Jun20, jc  written.
*/

#ifndef __INC_VX_I2C_H__
#define __INC_VX_I2C_H__



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*#define I2C_0_BASE_ADRS         (0xE0002000)*/
/*#define I2C_1_BASE_ADRS         (0xE0022000)*/
#define  VX_I2C_0_CFG_BASE        (0xE0002000)
#define  VX_I2C_1_CFG_BASE        (0xE0022000)



#define  I2C_CTRL_0   (0)
#define  I2C_CTRL_1   (1)

#define  MODE_I2C_MASTER   (0)
#define  MODE_I2C_SLAVE    (1)


/*
i2c ctrl reg
*/
#define I2C_REG_CON                  (0x00)      /* con;               // control register          (0x00)*/
#define I2C_REG_TAR                  (0x04)      /* tar;               // target address            (0x04)*/
#define I2C_REG_SAR                  (0x08)      /* sar;               // slave address             (0x08)*/
#define I2C_REG_HS_MADDR             (0x0c)      /* hs_maddr;          // high speed master code    (0x0c)*/
#define I2C_REG_DATA_CMD             (0x10)      /* data_cmd;          // tx/rx data/command buffer (0x10)*/
#define I2C_REG_SS_SCL_HCNT          (0x14)      /* ss_scl_hcnt;       // standard SCL high count   (0x14)*/
#define I2C_REG_SS_SCL_LCNT          (0x18)      /* ss_scl_lcnt;       // standard SCL low count    (0x18)*/

#define I2C_REG_FS_SCL_HCNT          (0x1c)      /* fs_scl_hcnt;       // full speed SCL high count (0x1c)*/
#define I2C_REG_FS_SCL_LCNT          (0x20)      /* fs_scl_lcnt;       // full speed SCL low count  (0x20)*/
#define I2C_REG_HS_SCL_HCNT          (0x24)      /* hs_scl_hcnt;       // high speed SCL high count (0x24)*/
#define I2C_REG_HS_SCL_LCNT          (0x28)      /* hs_scl_lcnt;       // high speed SCL low count  (0x28)*/
#define I2C_REG_INTR_STAT            (0x2c)      /* intr_stat;         // irq status                (0x2c)*/
#define I2C_REG_INTR_MASK            (0x30)      /* intr_mask;         // irq mask                  (0x30)*/
#define I2C_REG_RAW_INTR_STAT        (0x34)      /* raw_intr_stat;     // raw irq status            (0x34)*/
#define I2C_REG_RX_TL                (0x38)      /* rx_tl;             // Rx fifo threshold         (0x38)*/
#define I2C_REG_TX_TL                (0x3c)      /* tx_tl;             // Tx fifo threshold         (0x3c)*/
#define I2C_REG_CLR_INTR             (0x40)      /* clr_intr;          // clear all interrupts      (0x40)*/
#define I2C_REG_CLR_RX_UNDER         (0x44)      /* clr_rx_under;      // clear RX_UNDER irq        (0x44)*/
#define I2C_REG_CLR_RX_OVER          (0x48)      /* clr_rx_over;       // clear RX_OVER irq         (0x48)*/
#define I2C_REG_CLR_TX_OVER          (0x4c)      /* clr_tx_over;       // clear TX_OVER irq         (0x4c)*/
#define I2C_REG_CLR_RD_REQ           (0x50)      /* clr_rd_req;        // clear RD_REQ irq          (0x50)*/
#define I2C_REG_CLR_TX_ABRT          (0x54)      /* clr_tx_abrt;       // clear TX_ABRT irq         (0x54)*/
#define I2C_REG_CLR_RX_DONE          (0x58)      /* clr_rx_done;       // clear RX_DONE irq         (0x58)*/
#define I2C_REG_CLR_ACTIVITY         (0x5c)      /* clr_activity;      // clear ACTIVITY irq        (0x5c)*/
#define I2C_REG_CLR_STOP_DET         (0x60)      /* clr_stop_det;      // clear STOP_DET irq        (0x60)*/
#define I2C_REG_CLR_START_DET        (0x64)      /* clr_start_det;     // clear START_DET irq       (0x64)*/
#define I2C_REG_CLR_GEN_CALL         (0x68)      /* clr_gen_call;      // clear GEN_CALL irq        (0x68)*/

#define I2C_REG_ENABLE               (0x6c)      /* enable;            // i2c enable register       (0x6c)*/
#define I2C_REG_STATUS               (0x70)      /* status;            // i2c status register       (0x70)*/
#define I2C_REG_TXFLR                (0x74)      /* txflr;             // Tx fifo level register    (0x74)*/
#define I2C_REG_RXFLR                (0x78)      /* rxflr;             // Rx fifo level regsiter    (0x78)*/
#define I2C_REG_RESERVE1             (0x7C)      /* reserved		 (0x7c)*/

#define I2C_REG_TX_ABRT_SOURCE       (0x80)      /* tx_abrt_source;    // Tx abort status register  (0x80)*/

#define I2C_REG_DMA_CR               (0x88)      /* dma_cr;            // dma control register      (0x88)*/
#define I2C_REG_DMA_TDLR             (0x8c)      /* dma_tdlr;          // dma transmit data level   (0x8c)*/
#define I2C_REG_DMA_RDLR             (0x90)      /* dma_rdlr;          // dma receive data level    (0x90)*/
#define I2C_REG_SDA_SETUP            (0x94)      /* sda_setup;         // I2c SDA setup register    (0x94)*/
#define I2C_REG_ACK_GEN_CALL         (0x98)      /* ack_gen_call;      // Ack general call register (0x98)*/
#define I2C_REG_ENALBE_STATUS        (0x9c)      /* enable_status;     // Enable status register           (0x9c)*/
#define I2C_REG_FS_SPKLEN            (0xa0)      /* fs_spklen;         // SS & FS spike suppression limit  (0xa0)*/
#define I2C_REG_HS_SPKLEN            (0xa4)      /* hs_spklen;         // HS spike suppression limit       (0xa4)*/
#define I2C_REG_CLR_RESTART_DET      (0xa8)      /* clr_restart_det;   // clear RESTART_DET irq            (0xa8)*/
#define I2C_REG_SCL_LOW_TIMEOUT      (0xac)      /* scl_low_timeout;   // scl low timeout                  (0xac)*/
#define I2C_REG_SDA_LOW_TIMEOUT      (0xb0)      /* sda_low_timeout;   // sda low timeout                  (0xb0)*/
#define I2C_REG_CLR_SCL_STUCK_DET    (0xb4)      /* clr_scl_stuck_det; // clr SCL_STUCK_DET irq            (0xb4)*/
#define I2C_REG_DEVICE_ID            (0xb8)      /* device_id;         // Device ID                        (0xb8)*/

#define I2C_REG_COMP_PARAM_1         (0xf4)      /* comp_param_1;      // component parameters 1                 (0xf4)*/
#define I2C_REG_COMP_VERSION         (0xf8)      /* comp_version;      // component version                      (0xf8)*/
#define I2C_REG_COMP_TYPE            (0xfc)      /* comp_type;         // component type                         (0xfc)*/

/* speed mode */
#define I2_SPEED_STANDARD  (0x01)  /* I2c_speed_standard = 0x1,	// standard speed (100 kbps)*/
#define I2_SPEED_FAST      (0x02)  /* I2c_speed_fast = 0x2,		// fast speed (400 kbps)*/
#define I2_SPEED_HIGH      (0x03)  /* I2c_speed_high = 0x3		// high speed (3400 kbps)*/

/* address mode */
#define I2C_7BIT_ADDR    (0x00)  /* I2c_7bit_address = 0x0, 	*/
                                 /* 7-bit address mode.	Only the 7 LSBs*/
							     /* of the slave and/or target address are relevant.*/
							     
#define I2C_10BIT_ADDR   (0x01)   /* I2c_10bit_address = 0x1 	*/
                                  /* 10-bit address mode.  The 10 LSBs of*/
							      /* the slave and/or target address are relevant.*/

/*
 * These minimum high and low times are in nanoseconds.  They represent
 * the minimum amount of time a bus signal must remain either high or
 * low to be interpreted as a logical high or low as per the I2C bus
 * protocol.  These values are used in conjunction with an I2C input
 * clock frequency to determine the correct values to be written to the
 * clock count registers.
 */
#define SS_MIN_SCL_HIGH         (4000)
#define SS_MIN_SCL_LOW          (4700)
#define FS_MIN_SCL_HIGH         (600)
#define FS_MIN_SCL_LOW          (1300)
#define HS_MIN_SCL_HIGH_100PF   (60)
#define HS_MIN_SCL_LOW_100PF    (120)


/* 
I2C_STATUS
*/    
#define I2C_STATUS_ACTIVITY             (0x1 << 0)
#define I2C_STATUS_TFNF	                (0x1 << 1)
#define I2C_STATUS_TFE                  (0x1 << 2)
#define I2C_STATUS_RFNE                 (0x1 << 3)
#define I2C_STATUS_RFF                  (0x1 << 4)
#define I2C_STATUS_MST_ACTIVITY		    (0x1 << 5)
#define I2C_STATUS_SLV_ACTIVITY	     	(0x1 << 6)

/* 
I2C_CON
*/    
#define I2C_CON_ME                      (0x1 << 0)
#define I2C_CON_MS_SS     	            (0x1 << 1)
#define I2C_CON_MS_FS                   (0x2 << 1)
#define I2C_CON_SLAVE_ADR_7BIT          (0x0 << 3)
#define I2C_CON_SLAVE_ADR_10BIT         (0x1 << 3)    
#define I2C_CON_MASTER_ADR_7BIT         (0x0 << 4)
#define I2C_CON_MASTER_ADR_10BIT        (0x1 << 4)  
#define I2C_CON_RESTART_EN              (0x1 << 5)  
#define I2C_CON_SLAVE_DISABLE           (0x1 << 6)   

/* 
I2C_TAR
*/    
#define I2C_TAR_STARTBYTE               (0x1 << 10)
#define I2C_TAR_SPECIAL_STARTBYTE       (0x1 << 11)
#define I2C_TAR_ADR_7BIT                (0x0)
#define I2C_TAR_ADR_10BIT               (0x1 << 12)

#define I2C_TX_TARGET      (0x00)  /* I2c_tx_target = 0x0,		// normal transfer using target address*/
#define I2C_TX_GEN_CALL    (0x02)  /* I2c_tx_gen_call = 0x2,		// issue a general call*/
#define I2C_TX_START_BYTE  (0x03)  /* I2c_tx_start_byte = 0x3 	// issue a start byte I2C command*/

/* 
SCL
*/
#define I2C_SCL_LOW   (0x0)  /* I2c_scl_low = 0x0,			// SCL clock count low phase*/
#define I2C_SCL_HIGH  (0x1)  /* I2c_scl_high = 0x1			// SCL clock count high phase*/

/*
tx abort status or source
*/
#define I2C_ABRT_7B_ADDR_NOACK    (0x0001)  /* I2c_abrt_7b_addr_noack = 0x0001,*/
#define I2C_ABRT_10B_ADDR1_NOACK  (0x0002)  /* I2c_abrt_10addr1_noack = 0x0002,*/
#define I2C_ABRT_10B_ADDR2_NOACK  (0x0004)  /* I2c_abrt_10addr2_noack = 0x0004,*/
#define I2C_ABRT_TXDATA_NOACK     (0x0008)  /* I2c_abrt_txdata_noack = 0x0008,*/
#define I2C_ABRT_GCALL_NOACK      (0x0010)  /* I2c_abrt_gcall_noack = 0x0010,*/
#define I2C_ABRT_GCALL_READ       (0x0020)  /* I2c_abrt_gcall_read = 0x0020,*/
#define I2C_ABRT_HS_ACKDET        (0x0040)  /* I2c_abrt_hs_ackdet = 0x0040,*/
#define I2C_ABRT_SBYTE_ACKDET     (0x0080)  /* I2c_abrt_sbyte_ackdet = 0x0080,*/
#define I2C_ABRT_HS_NORSTRT       (0x0100)  /* I2c_abrt_hs_norstrt = 0x0100,*/
#define I2C_ABRT_SBYTE_NORSTRT    (0x0200)  /* I2c_abrt_sbyte_norstrt = 0x0200,*/
#define I2C_ABRT_10B_RD_NORSTRT   (0x0400)  /* I2c_abrt_10b_rd_norstrt = 0x0400,*/
#define I2C_ABRT_MASTER_DIS       (0x0800)  /* I2c_abrt_master_dis = 0x0800,*/
#define I2C_ABRT_ARB_LOST         (0x1000)  /* I2c_abrt_arb_lost = 0x1000,*/
#define I2C_ABRT_SLVFLUSH_TXFIFO  (0x2000)  /* I2c_abrt_slvflush_txfifo = 0x2000,*/
#define I2C_ABRT_SLV_ARBLOST      (0x5000)  /* I2c_abrt_slv_arblost = 0x5000,*/
#define I2C_ABRT_SLVRD_INTX       (0x8000)  /* I2c_abrt_slvrd_intx = 0x8000*/

/*
I2C_IRQ
*/
#define I2C_IRQ_NONE           (0x0000)  /* I2c_irq_none = 0x000,		// Specifies no interrupt*/
#define I2C_IRQ_RX_UNDER       (0x0001)  /* I2c_irq_rx_under = 0x001,	// Set if the processor attempts to read*/
#define I2C_IRQ_RX_OVER        (0x0002)  /* I2c_irq_rx_over = 0x002,	// Set if the receive FIFO was*/
#define I2C_IRQ_RX_FULL        (0x0004)  /* I2c_irq_rx_full = 0x004,	// Set when the transmit FIFO reaches or*/
#define I2C_IRQ_TX_OVER        (0x0008)  /* I2c_irq_tx_over = 0x008,	// Set during transmit if the transmit*/
#define I2C_IRQ_TX_EMPTY       (0x0010)  /* I2c_irq_tx_empty = 0x010,	// Set when the transmit FIFO is at or*/
#define I2C_IRQ_RD_REQ         (0x0020)  /* I2c_irq_rd_req = 0x020, 	// Set when the I2C is acting as a slave*/
#define I2C_IRQ_TX_ABRT        (0x0040)  /* I2c_irq_tx_abrt = 0x040,	// In general, this is set when the I2C*/
#define I2C_IRQ_RX_DONE        (0x0080)  /* I2c_irq_rx_done = 0x080,	// When the I2C is acting as a*/
#define I2C_IRQ_ACTIVITY       (0x0100)  /* I2c_irq_activity = 0x100,	// This is set whenever the I2C is busy*/
#define I2C_IRQ_STOP_DET       (0x0200)  /* I2c_irq_stop_det = 0x200,	// Indicates whether a stop condition*/
#define I2C_IRQ_START_DET      (0x0400)  /* I2c_irq_start_det = 0x400,	// Indicates whether a start condition*/
#define I2C_IRQ_GEN_CALL       (0x0800)  /* I2c_irq_gen_call = 0x800,	// Indicates that a general call request*/
#define I2C_IRQ_RESTART_DET    (0x1000)  /* I2c_irq_restart_det = 0x1000,*/
#define I2C_IRQ_MST_ON_HOLD    (0x2000)  /* I2c_irq_mst_on_hold = 0x2000,*/
#define I2C_SCL_STUCK_AT_LOW   (0x4000)  /* I2c_scl_stuck_at_low = 0x4000, //							 */
#define I2C_IRQ_ALL            (0x0FFF)  /* I2c_irq_all = 0xfff 		// Specifies all I2C interrupts.  This*/

/*
I2C DMA mode
*/
#define I2C_DMA_NONE            (0x00)  /* FMSH_dma_none,			  // DMA is not being used*/
#define I2C_DMA_SW_HANDSHAKE    (0x01)  /* FMSH_dma_sw_handshake,	  // DMA using software handshaking*/
#define I2C_DMA_HW_HANDSHAKE    (0x02)  /* FMSH_dma_hw_handshake	  // DMA using hardware handshaking*/


/*
i2c ctrl
*/
typedef struct _t_i2c_ctrl_
{
	int  ctrl_x;  /* 0, 1 */

	int mode_mst_slv;   /* 0-master, 1-slave */
	
	UINT16 devId;				/**< Unique ID of device */
	UINT32 cfgBaseAddr;		    /**< APB Base address of the device */
	UINT32 sysClk;				/**< Input clock frequency */
	
	int txThreshold;
	int rxThreshold;
	
	UINT32 intr_mask_save;
	
    int hc_count_values;       /* hardcoded scl count values?*/
    int has_dma;               /* i2c has a dma interface?    */
    UINT16 rx_buffer_depth;   /* Rx fifo depth*/
    UINT16 tx_buffer_depth;   /* Tx fifo depth*/
    int max_speed_mode;  /* standard, fast or high*/

	int dmaTx_mode;
	int dmaRx_mode;

	UINT32 devIP_ver;
    UINT32 devIP_type;      /* identify peripheral types:  */

	int status;
} vxT_I2C_CTRL;


typedef struct _t_i2c_
{
	int i2c_x;
	
	vxT_I2C_CTRL * pI2cCtrl;

	int init_flag;
} vxT_I2C;
	

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_VX_I2C_H__ */

