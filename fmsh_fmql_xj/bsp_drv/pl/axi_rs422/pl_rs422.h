/*
demo pl rs422, tx_loop_rx in fpga
*/

/*
modification history
--------------------
01a, 12Apri21, jc  written.
*/

#ifndef __INC_VX_PL422_H__
#define __INC_VX_PL422_H__



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**********************
*aix-gp rs422 
*5 channels
***********************/

#define UART_PL_STATUS_REG 	0x40000018
#define RS422_NUM 5

/*************read regs******************/
#define UART_TV_READ_DATA_REG 		0x40000000

#define UART_DLL_READ_DATA_REG 		0x40000004
#define UART_TARCK_READ_DATA_REG 	0x40000008
#define UART_GYRO_READ_DATA_REG 	0x4000000c
#define UART_FLY_READ_DATA_REG 		0x40000010


/*************write regs******************/
#define UART_WRITE_DATA_REG 	0x40000020

/*****************channel num***************/

#define UART_CHN_NUM_TV 		(0)
#define UART_CHN_NUM_DLL 		(1)
#define UART_CHN_NUM_TARCK 		(2)
#define UART_CHN_NUM_GYRO 		(3)
#define UART_CHN_NUM_FLY 		(4)

/*****************************************/
#define SYN_FRM_LEN_GYRO        (15)
#define SYN_FRM_LEN_TV          (29)
#define SYN_FRM_LEN_DLL         (16)
#define SYN_FRM_LEN_FLY         (72)
#define SYN_FRM_LEN_TARCK       (28)


/***********write regs*************/
#define SYN_WR_FRM_LEN_TV 	    (20)
#define SYN_WR_FRM_LEN_DLL 	    (32)
#define SYN_WR_FRM_LEN_TARCK	(48)


/***************buffer***********************/
unsigned char volatile g_Databuf_GYRO[SYN_FRM_LEN_GYRO] 	= {0}; 
unsigned char volatile g_Databuf_TV[SYN_FRM_LEN_TV] 		= {0}; 
unsigned char volatile g_Databuf_DLL[SYN_FRM_LEN_DLL] 	    = {0}; 
unsigned char volatile g_Databuf_FLY[SYN_FRM_LEN_FLY] 	    = {0}; 
unsigned char volatile g_Databuf_TRACK[SYN_FRM_LEN_TARCK]	= {0}; 


/*****************************irq num***************************/

#define SYN_IRQ_GYRO 	 (57)    /*0*/
#define SYN_IRQ_TV 		 (58)	 /*1*/
#define SYN_IRQ_DLL 	 (59)	 /*2*/
#define SYN_IRQ_FLY 	 (61)	 /*4*/
#define SYN_IRQ_TARCK 	 (60)	 /*3*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_VX_PL422_H__ */


