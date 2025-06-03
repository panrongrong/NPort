/* vxCan.h - vxCan driver header */

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

#ifndef __INC_VX_CAN_H__
#define __INC_VX_CAN_H__



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define  VX_CAN_0_CFG_BASE   (0xE0005000)
#define  VX_CAN_1_CFG_BASE   (0xE0006000)

#define  CAN_CTRL_0   (0)
#define  CAN_CTRL_1   (1)

#define  MODE_CAN_MASTER   (0)
#define  MODE_CAN_SLAVE    (1)

#define CAN_BUAD_1MHZ   1000000
#define CAN_BUAD_500KHZ  500000
#define CAN_BUAD_250KHZ  250000
#define CAN_BUAD_100KHZ  100000
#define CAN_BUAD_50KHZ    50000
#define CAN_BUAD_25KHZ    25000

#define  SINGLE_FILTER   (0x01)  /* single_filter = 0x01, */
#define  DUAL_FILTER     (0x00)  /* dual_filter = 0x00, */

#define  DATA_FRM        (0x00)  /* data_frame = 0x00, */
#define  REMOTE_FRM      (0x01)  /* remote_frame = 0x01, */

#define OK      (0)
#define ERROR   (-1)

/*
can ctrl reg
*/
#define CAN_REG_MODE    (0x00)    /* volatile u32 mode;  0x00 */
#define CAN_REG_CMR     (0x04)    /* volatile u32 cmr;   0x04    */
#define CAN_REG_SR      (0x08)    /* volatile u32 sr;    0x08 */
#define CAN_REG_IR      (0x0C)    /* volatile u32 ir;    0x0c */
#define CAN_REG_IER     (0x10)    /* volatile u32 ier;   0x10    */
#define CAN_REG_RSV1    (0x14)    /* volatile u32 rsv1;  0x14     */
#define CAN_REG_BTR0    (0x18)    /* volatile u32 btr0;  0x18 */
#define CAN_REG_BTR1    (0x1C)    /* volatile u32 btr1;   0x1c   */
#define CAN_REG_OCR     (0x20)    /* volatile u32 ocr;   0x20 */
#define CAN_REG_RSV2    (0x24)    /* volatile u32 rsv2;  0x24 */
#define CAN_REG_RSV3    (0x28)    /* volatile u32 rsv3;  0x28    */
#define CAN_REG_ALC     (0x2C)    /* volatile u32 alc;   0x2c     */
#define CAN_REG_ECC     (0x30)    /* volatile u32 ecc;   0x30 */
#define CAN_REG_EWLR    (0x34)    /* volatile u32 ewlr;  0x34    */
#define CAN_REG_RXERR   (0x38)    /* volatile u32 rxerr; 0x38 */
#define CAN_REG_TXERR   (0x3C)    /* volatile u32 txerr; 0x3c */

/*
Transmit Buffer	
40h	       Transmit Frame Information	Write	see below	Read back from 60h
44h – 70h  Transmit Data Information	Write		Read back from 61h – 6Ch

Receive Window
40h	 	   Receive Frame Information Read			
44h – 70h  Receive Data Information Read	

40h – 4Ch	ACR0–3	Acceptance Code Registers 0 – 3     Read/Write	
50h – 5Ch	AMR0–3	Acceptance Mask Registers 0 – 3		Read/Write
*/
#define CAN_REG_ACR0_TXRXBUF    (0x40)    /* volatile u32 acr0_txrxbuf;  0x40  */
#define CAN_REG_ACR1_TXRXBUF    (0x44)    /* volatile u32 acr1_txrxbuf;  0x44  */
#define CAN_REG_ACR2_TXRXBUF    (0x48)    /* volatile u32 acr2_txrxbuf;  0x48 */
#define CAN_REG_ACR3_TXRXBUF    (0x4C)    /* volatile u32 acr3_txrxbuf;  0x4c */
#define CAN_REG_AMR0_TXRXBUF    (0x50)    /* volatile u32 amr0_txrxbuf;  0x50 */
#define CAN_REG_AMR1_TXRXBUF    (0x54)    /* volatile u32 amr1_txrxbuf;  0x54 */
#define CAN_REG_AMR2_TXRXBUF    (0x58)    /* volatile u32 amr2_txrxbuf;  0x58    */
#define CAN_REG_AMR3_TXRXBUF    (0x5C)    /* volatile u32 amr3_txrxbuf;  0x5c   */
#define CAN_REG_TXRXBUF         (0x60)    /* volatile u32 txrxbuf[5];    0x60~0x70  */

#define CAN_REG_RMC     (0x74)    /* volatile u32 rmc;   0x74 */
#define CAN_REG_RBSA    (0x78)    /* volatile u32 rbsa; 0x78 */
#define CAN_REG_CDR     (0x7C)    /* volatile u32 cdr;  0x7c   */

#define CAN_REG_RXFIFO    (0x080)    /* volatile u32 rxfifo[0x40];  080h-17Ch  Receive FIFO	 Read only	Read/Write */
#define CAN_REG_TXFIFO    (0x180)    /* volatile u32 txfifo[0x11];  180h-1C0h  Transmit Buffer Read only	Read only */
#define CAN_REG_RSV4      (0x1C4)    /* volatile u32 rsv4[0x13];          */

/* mode  */
#define MODE_RST    (0x01)
#define MODE_RUN    (0x00)


/**   
 * DESCRIPTION
 *	Used in conjunction with fmsh_common_bitops.h to access register
 *	bitfields.	They are defined as bit offset/mask pairs for each gpio
 *	register bitfield.
 * NOTES
 *	bfo is the offset of the bitfield with respect to LSB;
 *	bfw is the width of the bitfield
 */
	/* cdr register   */
#define bfoCAN_CDR_CDR               0   
#define bfwCAN_CDR_CDR               3

#define bfoCAN_CDR_ClockOff          3  
#define bfwCAN_CDR_ClockOff          1  

	/* ocr register   */
#define bfoCAN_OCR_OCMODE            0   
#define bfwCAN_OCR_OCMODE            2
	
	/* mode register   */
#define bfoCAN_MODE_RM           0   
#define bfwCAN_MODE_RM           1

#define bfoCAN_MODE_LOM           1   
#define bfwCAN_MODE_LOM           1    

#define bfoCAN_MODE_STM           2   
#define bfwCAN_MODE_STM           1    

#define bfoCAN_MODE_AFM           3   
#define bfwCAN_MODE_AFM           1    

#define bfoCAN_MODE_SM           4   
#define bfwCAN_MODE_SM           1 
	
	/* sr register	 */
#define bfoCAN_SR_RBS           0   
#define bfwCAN_SR_RBS           1

#define bfoCAN_SR_DOS           1   
#define bfwCAN_SR_DOS           1

#define bfoCAN_SR_TBS           2   
#define bfwCAN_SR_TBS           1

#define bfoCAN_SR_TCS           3   
#define bfwCAN_SR_TCS           1    

#define bfoCAN_SR_RS           4   
#define bfwCAN_SR_RS           1    

#define bfoCAN_SR_TS           5   
#define bfwCAN_SR_TS           1    

#define bfoCAN_SR_ES           6   
#define bfwCAN_SR_ES           1  

#define bfoCAN_SR_BS           7   
#define bfwCAN_SR_BS           1  

	/* cmr register   */
#define bfoCAN_CMR_TR           0   
#define bfwCAN_CMR_TR           1

#define bfoCAN_CMR_AT           1   
#define bfwCAN_CMR_AT           1

#define bfoCAN_CMR_RRB           2   
#define bfwCAN_CMR_RRB           1    

#define bfoCAN_CMR_CDO           3   
#define bfwCAN_CMR_CDO           1        

#define bfoCAN_CMR_SRR           4   
#define bfwCAN_CMR_SRR           1   

	/* ier register   */
#define bfoCAN_IER_RIE           0   
#define bfwCAN_IER_RIE           1     

#define bfoCAN_IER_TIE           1   
#define bfwCAN_IER_TIE           1     

#define bfoCAN_IER_EIE           2   
#define bfwCAN_IER_EIE           1     

#define bfoCAN_IER_DOIE           3   
#define bfwCAN_IER_DOIE           1      

#define bfoCAN_IER_WUIE           4  
#define bfwCAN_IER_WUIE           1    

#define bfoCAN_IER_EPIE           5   
#define bfwCAN_IER_EPIE           1     

#define bfoCAN_IER_ALIE           6   
#define bfwCAN_IER_ALIE           1    

#define bfoCAN_IER_BEIE           7   
#define bfwCAN_IER_BEIE           1  

	/* ir register	 */
#define bfoCAN_IR_RI           0   
#define bfwCAN_IR_RI           1     

#define bfoCAN_IR_TI           1   
#define bfwCAN_IR_TI           1    

#define bfoCAN_IR_EI           2   
#define bfwCAN_IR_EI           1    

#define bfoCAN_IR_DOI           3   
#define bfwCAN_IR_DOI           1      

#define bfoCAN_IR_WUI           4  
#define bfwCAN_IR_WUI           1     

#define bfoCAN_IR_EPI           5   
#define bfwCAN_IR_EPI           1     

#define bfoCAN_IR_ALI           6   
#define bfwCAN_IR_ALI           1     

#define bfoCAN_IR_BEI           7   
#define bfwCAN_IR_BEI           1   

	/* BRT0 register   */
#define bfoCAN_BRT0_BRP          0   
#define bfwCAN_BRT0_BRP          6     

	/* BRT1 register   */
#define bfoCAN_BRT1_SEG1          0   
#define bfwCAN_BRT1_SEG1          4    

#define bfoCAN_BRT1_SEG2          4   
#define bfwCAN_BRT1_SEG2          3 


	/* Definitions for driver CANPS */
#define FPAR_CANPS_NUM_INSTANCES 2
	
	/* Definitions for peripheral PS_CAN_0 */
#define FPAR_CANPS_0_DEVICE_ID 0
#define FPAR_CANPS_0_BASEADDR FPS_CAN0_BASEADDR
#define FPAR_CANPS_0_CAN_CLK_FREQ_HZ CAN_FREQ
	
	/* Definitions for peripheral PS_CAN_1 */
#define FPAR_CANPS_1_DEVICE_ID 1
#define FPAR_CANPS_1_BASEADDR FPS_CAN1_BASEADDR
#define FPAR_CANPS_1_CAN_CLK_FREQ_HZ CAN_FREQ

#define CAN_CALC_MAX_ERROR   (50)       /* in one-tenth of a percent */
#define CAN_CALC_SYNC_SEG    (1)

#define UINT_MAX	               (~0U)

#define MAX(a,b)                   (((a)>(b))?(a):(b))
#define MIN(a,b)                   (((a)<(b))?(a):(b))

#define clamp(a, min_val, max_val)  MIN(MAX((a), (min_val)), (max_val))


/*
spi ctrl
*/
typedef struct _t_can_ctrl_
{
	int  ctrl_x;  /* 0, 1 */

	int mode_mst_slv;   /* 0-master, 1-slave */
	
	UINT16 devId;				/**< Unique ID of device */
	UINT32 cfgBaseAddr;		    /**< APB Base address of the device */
	UINT32 sysClk;				/**< Input clock frequency */
	
	/**
	 * CAN bit-timing parameters
	 *
	 * For further information, please read chapter "8 BIT TIMING
	 * REQUIREMENTS" of the "Bosch CAN Specification version 2.0"
	 * at http://www.semiconductors.bosch.de/pdf/can2spec.pdf.
	 */
	struct 
	{
		UINT32 bitrate;		     /* Bit-rate in bits/second */
		UINT32 sample_point;	 /* Sample point in one-tenth of a percent */
		UINT32 tq; 	             /* Time quanta (TQ) in nanoseconds */
		UINT32 prop_seg;		 /* Propagation segment in TQs */
		UINT32 phase_seg1;       /* Phase buffer segment 1 in TQs */
		UINT32 phase_seg2;       /* Phase buffer segment 2 in TQs */
		UINT32 sjw;		         /* Synchronisation jump width in TQs */
		UINT32 brp;		         /* Bit-rate prescaler */
	} BUS_TIME;

	
	/**
	 * CAN harware-dependent bit-timing constant
	 *
	 * Used for calculating and checking bit-timing parameters
	 */
	struct
	{
		char name[16];		    /* Name of the CAN controller hardware */
		
		UINT32 tseg1_min;	    /* Time segement 1 = prop_seg + phase_seg1 */
		UINT32 tseg1_max;
		UINT32 tseg2_min;	    /* Time segement 2 = phase_seg2 */
		UINT32 tseg2_max;
		UINT32 sjw_max;		    /* Synchronisation jump width */
		UINT32 brp_min;		    /* Bit-rate prescaler */
		UINT32 brp_max;
		UINT32 brp_inc;
	} BUS_TIME_C;   

	UINT32 devIP_ver;
    UINT32 devIP_type;      /* identify peripheral types:  */

	int status;
} vxT_CAN_CTRL;


typedef struct _t_can_
{
	int can_x;
	
	vxT_CAN_CTRL * pCanCtrl;
	
} vxT_CAN;
	

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_VX_CAN_H__ */

