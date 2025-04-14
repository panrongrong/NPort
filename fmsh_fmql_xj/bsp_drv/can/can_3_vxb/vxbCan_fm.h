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

#ifndef __INC_vxbFmCan_h__
#define __INC_vxbFmCan_h__

#ifdef __cplusplus
extern "C"
{
#endif

/* defines */

#define FM_CAN_NAME	   "fmcan"

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

#define  SINGLE_FILTER   (0x01)  /* single_filter = 0x01,*/
#define  DUAL_FILTER     (0x00)  /* dual_filter = 0x00,*/

#define  DATA_FRM        (0x00)  /* data_frame = 0x00,*/
#define  REMOTE_FRM      (0x01)  /* remote_frame = 0x01,*/


/*
can ctrl reg
*/
#define CAN_REG_MODE    (0x00)    /* volatile u32 mode;  //0x00*/
#define CAN_REG_CMR     (0x04)    /* volatile u32 cmr;   //0x04   */
#define CAN_REG_SR      (0x08)    /* volatile u32 sr;    //0x08*/
#define CAN_REG_IR      (0x0C)    /* volatile u32 ir;    //0x0c*/
#define CAN_REG_IER     (0x10)    /* volatile u32 ier;   //0x10   */
#define CAN_REG_RSV1    (0x14)    /* volatile u32 rsv1;  //0x14    */
#define CAN_REG_BTR0    (0x18)    /* volatile u32 btr0;  //0x18*/
#define CAN_REG_BTR1    (0x1C)    /* volatile u32 btr1;  // 0x1c  */
#define CAN_REG_OCR     (0x20)    /* volatile u32 ocr;   //0x20*/
#define CAN_REG_RSV2    (0x24)    /* volatile u32 rsv2;  //0x24*/
#define CAN_REG_RSV3    (0x28)    /* volatile u32 rsv3;  //0x28   */
#define CAN_REG_ALC     (0x2C)    /* volatile u32 alc;   //0x2c    */
#define CAN_REG_ECC     (0x30)    /* volatile u32 ecc;   //0x30*/
#define CAN_REG_EWLR    (0x34)    /* volatile u32 ewlr;  //0x34   */
#define CAN_REG_RXERR   (0x38)    /* volatile u32 rxerr; //0x38*/
#define CAN_REG_TXERR   (0x3C)    /* volatile u32 txerr; //0x3c*/

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
#define CAN_REG_ACR0_TXRXBUF    (0x40)    /* volatile u32 acr0_txrxbuf;  //0x40 */
#define CAN_REG_ACR1_TXRXBUF    (0x44)    /* volatile u32 acr1_txrxbuf;  //0x44 */
#define CAN_REG_ACR2_TXRXBUF    (0x48)    /* volatile u32 acr2_txrxbuf;  //0x48*/
#define CAN_REG_ACR3_TXRXBUF    (0x4C)    /* volatile u32 acr3_txrxbuf;  //0x4c*/
#define CAN_REG_AMR0_TXRXBUF    (0x50)    /* volatile u32 amr0_txrxbuf;  //0x50*/
#define CAN_REG_AMR1_TXRXBUF    (0x54)    /* volatile u32 amr1_txrxbuf;  //0x54*/
#define CAN_REG_AMR2_TXRXBUF    (0x58)    /* volatile u32 amr2_txrxbuf;  //0x58   */
#define CAN_REG_AMR3_TXRXBUF    (0x5C)    /* volatile u32 amr3_txrxbuf;  //0x5c  */
#define CAN_REG_TXRXBUF         (0x60)    /* volatile u32 txrxbuf[5];    //0x60~0x70 */

#define CAN_REG_RMC     (0x74)    /* volatile u32 rmc;  // 0x74*/
#define CAN_REG_RBSA    (0x78)    /* volatile u32 rbsa; //0x78*/
#define CAN_REG_CDR     (0x7C)    /* volatile u32 cdr;  //0x7c  */

#define CAN_REG_RXFIFO    (0x080)    /* volatile u32 rxfifo[0x40];  //080h-17Ch  Receive FIFO	 Read only	Read/Write*/
#define CAN_REG_TXFIFO    (0x180)    /* volatile u32 txfifo[0x11];  //180h-1C0h  Transmit Buffer Read only	Read only*/
#define CAN_REG_RSV4      (0x1C4)    /* volatile u32 rsv4[0x13];    //     */

/* mode */
#define MODE_RST    (0x01)
#define MODE_RUN    (0x00)

#define CAN_CALC_MAX_ERROR   (50)       /* in one-tenth of a percent */
#define CAN_CALC_SYNC_SEG    (1)

#define UINT_MAX	               (~0U)

#define MAX(a,b)                   (((a)>(b))?(a):(b))
#define MIN(a,b)                   (((a)<(b))?(a):(b))

#define clamp(a, min_val, max_val)  MIN(MAX((a), (min_val)), (max_val))

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

/* typedefs */

/* structure holding the instance specific details */

typedef struct fm_can_drv_ctrl
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

    SEM_ID           semSync;
    SEM_ID           muxSem;	
} FM_CAN_DRV_CTRL;


#if 1
LOCAL void vxCanInstInit (VXB_DEVICE_ID pDev);
LOCAL void vxCanInstInit2 (VXB_DEVICE_ID pDev);
LOCAL void vxCanInstConnect (VXB_DEVICE_ID pDev);
LOCAL void vxCanShow (VXB_DEVICE_ID, int);
LOCAL STATUS vxCanInstUnlink (VXB_DEVICE_ID pDev, void * unused);

/* forward declarations */
LOCAL void vxCanCtrl_Init (VXB_DEVICE_ID pDev);
LOCAL void vxCanIsr (VXB_DEVICE_ID pDev);


UINT32 vxDo_div2(UINT64 n, UINT32 base);
void vxCanCtrl_Wr_CfgReg32(VXB_DEVICE_ID pDev, UINT32 offset, UINT32 value);
UINT32 vxCanCtrl_Rd_CfgReg32(VXB_DEVICE_ID pDev, UINT32 offset);
int vxCanCtrl_Update_SamplePoint(VXB_DEVICE_ID pDev,			   UINT32 sample_point_nominal, 
											UINT32 tseg, UINT32 *tseg1_ptr, UINT32 *tseg2_ptr, UINT32 *pSamplePointErr);
int vxCanCtrl_Calc_BusTiming(VXB_DEVICE_ID pDev);
int vxCanCtrl_Set_RunMode(VXB_DEVICE_ID pDev, UINT32 mode_state);
int vxCanCtrl_Set_BusTiming(VXB_DEVICE_ID pDev);
int vxCanCtrl_Set_BaudRate(VXB_DEVICE_ID pDev, int baud);
int vxCanCtrl_Set_StdACR(VXB_DEVICE_ID pDev, UINT32 id, UINT8 ptr, UINT8 byte1, UINT8 byte2, int filter_mode);
int vxCanCtrl_Set_AMR(VXB_DEVICE_ID pDev, UINT32 AMR0, UINT32 AMR1, UINT32 AMR2, UINT32 AMR3);
UINT32 vxCanCtrl_Get_TxBufStatus(VXB_DEVICE_ID pDev);
int vxCanCtrl_Setup_StdFrmTx(VXB_DEVICE_ID pDev, UINT32 id, UINT8* pBbuf, int len, int frm_type);
int vxCanCtrl_Set_TxRequest(VXB_DEVICE_ID pDev);
UINT32 vxCanCtrl_Get_TxOkStatus(VXB_DEVICE_ID pDev);
int vxCanCtrl_Get_RxFrmBuf(VXB_DEVICE_ID pDev, UINT8 * rxBuf);
int vxCanCtrl_ReleaseRxBuf(VXB_DEVICE_ID pDev);

void vxCanSlcr_Enable_CanClk(void);
void vxCanSlcr_Set_CanMIO(void);
void vxCanSlcr_Set_CanLoop(int loop_en);

#endif


#ifdef __cplusplus
}
#endif

#endif /* __INC_vxbFmCan_h__ */


