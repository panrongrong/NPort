#ifndef __FPAG_UART_H_
#define __FPAG_UART_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Copyright (c) 1990,1991 Intel Corporation
 *
 * Intel hereby grants you permission to copy, modify, and
 * distribute this software and its documentation.  Intel grants
 * this permission provided that the above copyright notice
 * appears in all copies and that both the copyright notice and
 * this permission notice appear in supporting documentation.  In
 * addition, Intel grants this permission provided that you
 * prominently mark as not part of the original any modifications
 * made to this software or documentation, and that the name of
 * Intel Corporation not be used in advertising or publicity
 * pertaining to distribution of the software or the documentation
 * without specific, written prior permission.
 *
 * Intel Corporation does not warrant, guarantee or make any
 * representations regarding the use of, or the results of the use
 * of, the software and documentation in terms of correctness,
 * accuracy, reliability, currentness, or otherwise; and you rely
 * on the software, documentation and results solely at your own risk.
 */


/* REGISTER DESCRIPTION OF STARTECH 16552 DUART */

#ifndef _ASMLANGUAGE

#include "vxWorks.h"
#include "sioLib.h"
#include "spinLockLib.h"

#define HD_BP2020_A4 /* for A4 */

#define BIT(i)		(1<<(i))


#ifdef HD_BP2020_A4
#define MAX_UART_NUM		(2)	/* uart0~8 RS422  uart9~16 RS485 */

#define SYS_RS422 0
#define SYS_RS485 1

#else
#define MAX_UART_NUM		(8)
#endif

typedef enum{
    INTR_TRIGGER_HIGH_LEVEL = 0x1,   
    INTR_TRIGGER_RAISE_EDGE = 0x3  
}INTERRUPT_TRIG_T;


#define RS485_IRQ_NUM    57
/***************************************************************************************/

/* Register offsets from base address */

#define RBR	0x00	/* Receive Holding Register (R/O) */
#define THR	0x00	/* Transmit Holding Register (W/O)*/
#define DLL	0x00	/* Divisor Latch Low */
#define IER	0x01	/* Interrupt Enable Register */
#define DLM	0x01	/* Divisor Latch Middle */
#define IIR	0x02	/* Interupt identification Register (R/O) */
#define FCR	0x02	/* FIFO Control register (W/O) */
#define LCR	0x03	/* Line Control Register */
#define MCR	0x04	/* Modem Control Register */
#define LSR	0x05	/* Line Status register */
#define MSR	0x06	/* Modem Status Register */
#define SCR	0x07	/* Scratchpad Register */

#define RXFIFO_WRDS_L	0x06	/* Recv Fifo Count Low Register */
#define RXFIFO_WRDS_H	0x07	/* Recv Fifo Count Low Register */


#define CPLD_VER 0x40   //CPLD 逻辑版本
#define CPLD_TEST 0x41   //CPLD 测试寄存器
#define CPLD_LOOP 0x42   //CPLD 环回测试寄存器
#define CPLD_CHECK_ERR_CNT 0x43   //CPLD 校验错误计数寄存器
#define CPLD_STOP_ERR_CNT 0x44   //CPLD 停止位错误计数寄存器
#define CPLD_RCV_CNT_L 0x45   //CPLD 收数计数寄存器低位
#define CPLD_RCV_CNT_H 0x46   //CPLD 收数计数寄存器高位
#define CPLD_SND_CNT_L 0x47   //CPLD 发数计数寄存器低位
#define CPLD_SND_CNT_H 0x48   //CPLD 发数计数寄存器高位

#define FPGA_VER 0x40   //FPGA 逻辑版本
#define FPGA_TEST 0x41   //FPGA 测试寄存器
#define FPGA_LOOP 0x42   //FPGA 环回测试寄存器
#define FPGA_CHECK_ERR_CNT 0x43   //FPGA 校验错误计数寄存器
#define FPGA_STOP_ERR_CNT 0x44   //FPGA 停止位错误计数寄存器
#define FPGA_RCV_CNT_L 0x45   //FPGA 收数计数寄存器低位
#define FPGA_RCV_CNT_H 0x46   //FPGA 收数计数寄存器高位
#define FPGA_SND_CNT_L 0x47   //FPGA 发数计数寄存器低位
#define FPGA_SND_CNT_H 0x48   //FPGA 发数计数寄存器高位


#define BAUD_LO(baud)  ((XTAL/(16*baud)) & 0xFF)
#define BAUD_HI(baud)  (((XTAL/(16*baud)) & 0xFF00) >> 8)

/* Line Control Register values */

#define CHAR_LEN_5	0x00
#define CHAR_LEN_6	0x01
#define CHAR_LEN_7	0x02
#define CHAR_LEN_8	0x03
#define LCR_STB		0x04	/* Stop bit control */
#define ONE_STOP	0x00	/* One stop bit! */
#define LCR_PEN		0x08	/* Parity Enable */
#define PARITY_NONE	0x00
#define LCR_EPS		0x10	/* Even Parity Select */
#define LCR_SP		0x20	/* Force Parity */
#define LCR_SBRK	0x40	/* Start Break */
#define LCR_DLAB	0x80	/* Divisor Latch Access Bit */
#define DLAB		LCR_DLAB

/* Line Status Register */

#define LSR_DR		0x01	/* Data Ready */
#define RxCHAR_AVAIL	LSR_DR
#define LSR_OE		0x02	/* Overrun Error */
#define LSR_PE		0x04	/* Parity Error */
#define LSR_FE		0x08	/* Framing Error */
#define LSR_BI		0x10	/* Received Break Signal */
#define LSR_THRE	0x20	/* Transmit Holding Register Empty */
#define LSR_TEMT	0x40	/* THR and FIFO empty */
#define LSR_FERR	0x80	/* Parity, Framing error or break in FIFO */

/* Interrupt Identification Register */

#define IIR_IP		0x01		/* Interrupt Pending */
#define IIR_ID		0x0E		/* Interrupt source mask */
#define IIR_RLS		0x06		/* Rx Line Status Int */
#define Rx_INT		IIR_RLS
#define IIR_RDA		0x04		/* Rx Data Available */
#define RxFIFO_INT	IIR_RDA
#define IIR_THRE	0x02		/* THR Empty */
#define TxFIFO_INT	IIR_THRE
#define IIR_MSTAT	0x00		/* Modem Status Register Int */
#define IIR_TIMEOUT	0x0C		/* Rx Data Timeout */

/* Interrupt Enable Register */

#define IER_ERDAI	0x01		/* Enable Rx Data Available Int */
#define RxFIFO_BIT	IER_ERDAI
#define IER_ETHREI	0x02		/* Enable THR Empty Int */
#define TxFIFO_BIT	IER_ETHREI
#define IER_ELSI	0x04		/* Enable Line Status Int */
#define Rx_BIT		IER_ELSI
#define IER_EMSI	0x08		/* Enable Modem Status Int */

/* Modem Control Register */

#define MCR_DTR		0x01		/* state of DTR output */
#define DTR			MCR_DTR
#define MCR_RTS		0x02		/* state of RTS output */
#define MCR_OUT1	0x04		/* UNUSED in ST16552 */
#define MCR_INT		0x08		/* Int Mode */
#define MCR_LOOP	0x10		/* Enable Loopback mode */

/* Modem Status Register */

#define MSR_DCTS	0x01		/* change in CTS */
#define MSR_DDSR	0x02		/* change in DSR */
#define MSR_TERI	0x04		/* change in RI */
#define MSR_DDCD	0x08		/* change in DCD */
#define MSR_CTS		0x10		/* state of CTS input */
#define MSR_DSR		0x20		/* state of DSR input */
#define MSR_RI		0x40		/* state of RI input */
#define MSR_DCD		0x80		/* state of DCD input */

/* FIFO Control Register */

#define FCR_EN			0x01		/* FIFO Enable */
#define FIFO_ENABLE		FCR_EN
#define FCR_RXCLR		0x02		/* Rx FIFO Clear */
#define RxCLEAR			FCR_RXCLR
#define FCR_TXCLR		0x04		/* Tx FIFO Clear */
#define TxCLEAR			FCR_TXCLR
#define FCR_DMA			0x08		/* FIFO Mode Control */
#define FCR_RXTRIG_L	0x40		/* FIFO Trigger level Low */
#define FCR_RXTRIG_H	0x80		/* FIFO Trigger level High */

typedef struct 		/* ST16552_CHAN */
{
    /* must be first */
    SIO_CHAN	sio;			/* standard SIO_CHAN element */
	
    /* callbacks */
    STATUS	(*getTxChar) ();	/* installed Tx callback routine */
    STATUS	(*putRcvChar) ();	/* installed Rx callback routine */
    void *	getTxArg;			/* argument to Tx callback routine */
    void *	putRcvArg;			/* argument to Rx callback routine */

    UINT8 *	regs;				/* ST16552 registers */
    UINT8	level;				/* Interrupt level for this device */
    UINT8	ier;				/* copy of IER */
    UINT8	lcr;				/* copy of LCR */
    UINT8   mcr;				/* copy of MCR */

    UINT32	channelMode;		/* such as INT, POLL modes */
    UINT32	regDelta;			/* register address spacing */
    int		baudRate;			/* the current baud rate */
    UINT32	xtal;				/* UART clock frequency */


	 spinlockIsr_t  spinLock;  //SMP 要使用自旋锁
	
    /*为逻辑的中断复用增加的功能*/
   	UINT32 fpgaIntBase;	/*中断复用模块寄存器基地址*/
   	UINT32 intBit;	/*中断用哪一位bit控制*/

	UINT32 RS485; /* 标识当前串口是RS485 */
}UART_CHAN, uart_DEV;

uart_DEV uartDev[MAX_UART_NUM];

/* structure used as parameter to multiplexed interrupt handler */
typedef struct st16552_mux		/* ST16552_MUX */
 	{
    int			nextChan;	/* next channel to examine on int */
    UART_CHAN *	pChan;		/* array of ST16552_CHAN structs */
    } ST16552_MUX;

/***************************************************************************************/

#define IVEC_TO_INUM(intVec)    	(intVec)
#define INUM_TO_IVEC(intNum)    	(intNum+UART_EXT_LEVEL)


#define TLC16C554_DELTA_REG   		4

#define CONSOLE_BAUD_RATE			115200			/* console baud rate */
#define UART_XTAL_FREQ          	14745600    	//串口时钟频率14.7456M
#define UART_EXT_LEVEL				12				//powerpc 有12个外部中断 ，所以是从0-11 为外部中断编号
#define UART_INTER_LEVEL       		26				//中断等级,中断编号

#define UART_BUFFER_SIZE     		2048



const char* DEV_NAME[] = {
	"/Rs485/0","/Rs485/1"
};

/* 中断相关寄存器 */
#define EPIC_VEC_EXT_IRQ0       		0
#define INUM_IRQ0  EPIC_VEC_EXT_IRQ0
#define INUM_IRQ1 (EPIC_VEC_EXT_IRQ0 + 1)
#define INUM_IRQ2 (EPIC_VEC_EXT_IRQ0 + 2)
#define INUM_IRQ3 (EPIC_VEC_EXT_IRQ0 + 3)
#define INUM_IRQ4 (EPIC_VEC_EXT_IRQ0 + 4)
#define INUM_IRQ5 (EPIC_VEC_EXT_IRQ0 + 5)
#define INUM_IRQ6 (EPIC_VEC_EXT_IRQ0 + 6)
#define INUM_IRQ7 (EPIC_VEC_EXT_IRQ0 + 7)
#define INUM_IRQ8 (EPIC_VEC_EXT_IRQ0 + 8)
#define INUM_IRQ9 (EPIC_VEC_EXT_IRQ0 + 9)
#define INUM_IRQ10 (EPIC_VEC_EXT_IRQ0 + 10)
#define INUM_IRQ11 (EPIC_VEC_EXT_IRQ0 + 11)

#if 0
/* 中断触发方式设置 */
typedef enum{
    INTR_TRIGGER_LOW_LEVEL = 1, /**< 低电平触发 */
    INTR_TRIGGER_HIGH_LEVEL,    /**< 高电平触发 */
    INTR_TRIGGER_FALL_EDGE,     /**< 下降沿触发 */
    INTR_TRIGGER_RAISE_EDGE     /**< 上升沿触发 */
}INTERRUPT_TRIG_T;
#endif

#define	Int_en					0x22//0x11	中断使能
#define	Int_isr					0x24//0x12  中断状态
#define	Int_clr					0x26//0x13  中断清除
#define Int_mode				0x28//0x14	中断方式 


extern UART_CHAN my16C554[MAX_UART_NUM];


/* function declarations */

//extern void st16552DevInit (UART_CHAN *);
extern STATUS hdIntListAdd	(VOIDFUNCPTR *vector, VOIDFUNCPTR routine,int parameter);
extern STATUS intModeSet(int intNum,unsigned int intMode);

STATUS mySysSerial(UART_CHAN* pmy16C554,UINT32 udBaseAddr,char* DevName,unsigned int intBit);


#endif	/* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif


#endif /* __FPAG_UART_H_ */

