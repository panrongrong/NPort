/* vxbNs16550Sio.h - National Semiconductor 16552 DUART header file */

/*
 * Copyright (c) 1995, 1997, 2000, 2004-2009, 2014 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
10sep14,d_l add checkLSRBeforeReadData for DM 37X. (VXW6-83413)
01p,28aug09,h_k  added BAR size.
01o,28feb08,l_g  added ierInit field.
01n,13aug07,h_k  added handle field.
01m,23may07,h_k  removed externs.
01l,09apr07,h_k  added TX FIFO support.
                 added irq field in NS16550VXB_CHAN.
01k,21mar07,sru  ISR_DEFER_QUEUE changed to ISR_DEFER_QUEUE_ID
01j,02feb07,wap  defect 83778, serial initialization
01j,03jan07,h_k  updated for SMP.
01i,28Jul06,tor  modified for compatability with non-Vxbus version
01h,24may06,dtr  Add in divisor func ptr to channel struct.
01g,12sep05,mdo  SPR#111986 - vxBus interrupt mode fix.
01f,07sep04,mdo  Documentation fixes for apigen
01e,11may04,rec  remove manual errors
01d,12apr00,dra  Add include of sioLib.h
01c,08may97,db   added elements options and mcr to NS16550_CHAN structure to
         implement hardware options & modem control(SPRs #7570, #7082).
01b,01nov95,myz  removed #if CPU=I960XX ... in NS16550_CHAN structure
01a,24oct95,myz  written from ns16552.h.
*/

#ifndef __INCvxbNs16550Sioh
#define __INCvxbNs16550Sioh

#ifdef __cplusplus
extern "C" {
#endif

/*
 *********************************************************************
 *      Copyright (c) 1990,1991 Intel Corporation
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
 *
 *\NOMANUAL
 **********************************************************************
*/

/*
 ******************************************************************************
 *
 * REGISTER DESCRIPTION OF NATIONAL 16552 DUART
 *
 * $Id: ns16552.h,v 2.1 1993/06/07 15:07:59 wise active $
 *
 *\NOMANUAL
 *******************************************************************************
*/

#ifndef _ASMLANGUAGE

#include <sioLib.h>
#include <spinLockLib.h>
#include <isrDeferLib.h>

/* Register offsets from base address */

#define RBR     0x00    /* receiver buffer register */
#define THR     0x00    /* transmit holding register */
#define DLL     0x00    /* divisor latch */
#define IER     0x01    /* interrupt enable register */
#define DLM     0x01    /* divisor latch(MS) */
#define IIR     0x02    /* interrupt identification register */
#define FCR     0x02    /* FIFO control register */
#define LCR     0x03    /* line control register */
#define MCR     0x04    /* modem control register */
#define LSR     0x05    /* line status register */
#define MSR     0x06    /* modem status register */
#define SCR     0x07    /* scratch register */

#define NS16550_SIO_BAR_SIZE	(SCR + 1)

#define BAUD_LO(baud)  ((XTAL/(16*baud)) & 0xff)
#define BAUD_HI(baud)  (((XTAL/(16*baud)) & 0xff00) >> 8)

/* Line Control Register */

#define CHAR_LEN_5  0x00    /* 5bits data size */
#define CHAR_LEN_6  0x01    /* 6bits data size */
#define CHAR_LEN_7  0x02    /* 7bits data size */
#define CHAR_LEN_8  0x03    /* 8bits data size */
#define LCR_STB     0x04    /* 2 stop bits */
#define ONE_STOP    0x00    /* one stop bit */
#define LCR_PEN     0x08    /* parity enable */
#define PARITY_NONE 0x00    /* parity disable */
#define LCR_EPS     0x10    /* even parity select */
#define LCR_SP      0x20    /* stick parity select */
#define LCR_SBRK    0x40    /* break control bit */
#define LCR_DLAB    0x80    /* divisor latch access enable */
#define DLAB        LCR_DLAB

/* Line Status Register */

#define LSR_DR      0x01    /* data ready */
#define RxCHAR_AVAIL    LSR_DR  /* data ready */
#define LSR_OE      0x02    /* overrun error */
#define LSR_PE      0x04    /* parity error */
#define LSR_FE      0x08    /* framing error */
#define LSR_BI      0x10    /* break interrupt */
#define LSR_THRE    0x20    /* transmit holding register empty */
#define LSR_TEMT    0x40    /* transmitter empty */
#define LSR_FERR    0x80    /* in fifo mode, set when PE,FE or BI error */

/* Interrupt Identification Register */

#define IIR_IP      0x01
#define IIR_ID      0x0e
#define IIR_RLS     0x06    /* received line status */
#define Rx_INT      IIR_RLS /* received line status */
#define IIR_RDA     0x04    /* received data available */
#define RxFIFO_INT  IIR_RDA /* received data available */
#define IIR_THRE    0x02    /* transmit holding register empty */
#define TxFIFO_INT  IIR_THRE 
#define IIR_MSTAT   0x00    /* modem status */
#define IIR_TIMEOUT 0x0c    /* char receive timeout */

/* Interrupt Enable Register */

#define IER_ERDAI   0x01    /* received data avail. & timeout int */
#define RxFIFO_BIT  IER_ERDAI
#define IER_ETHREI  0x02    /* transmitter holding register empty int */
#define TxFIFO_BIT  IER_ETHREI
#define IER_ELSI    0x04    /* receiver line status int enable */
#define Rx_BIT      IER_ELSI
#define IER_EMSI    0x08    /* modem status int enable */

/* Modem Control Register */

#define MCR_DTR     0x01    /* dtr output */
#define DTR     MCR_DTR
#define MCR_RTS     0x02    /* rts output */
#define MCR_OUT1    0x04    /* output #1 */
#define MCR_OUT2    0x08    /* output #2 */
#define MCR_LOOP    0x10    /* loopback enable */

/* Modem Status Register */

#define MSR_DCTS    0x01    /* cts change */
#define MSR_DDSR    0x02    /* dsr change */
#define MSR_TERI    0x04    /* ring indicator change */
#define MSR_DDCD    0x08    /* data carrier indicator change */
#define MSR_CTS     0x10    /* complement of cts */
#define MSR_DSR     0x20    /* complement of dsr */
#define MSR_RI      0x40    /* complement of ring signal */
#define MSR_DCD     0x80    /* complement of dcd */

/* FIFO Control Register */

#define FCR_EN      0x01    /* enable xmit and rcvr */
#define FIFO_ENABLE FCR_EN
#define FCR_RXCLR   0x02    /* clears rcvr fifo */
#define RxCLEAR     FCR_RXCLR
#define FCR_TXCLR   0x04    /* clears xmit fifo */
#define TxCLEAR     FCR_TXCLR
#define FCR_DMA     0x08    /* dma */
#define FCR_RXTRIG_L    0x40    /* rcvr fifo trigger lvl low */
#define FCR_RXTRIG_H    0x80    /* rcvr fifo trigger lvl high */

typedef  struct ns16550vxbChan     /* NS16550VXB_CHAN * */
    {
    /* always goes first */

    SIO_DRV_FUNCS *     pDrvFuncs;      /* driver functions */

    /* callbacks */

    STATUS      (*getTxChar) (); /* pointer to xmitr function */
    STATUS      (*putRcvChar) (); /* pointer to rcvr function */
    void *      getTxArg;
    void *      putRcvArg;

    UINT8   *regs;      /* NS16550 registers */
    UINT8   level;      /* 8259a interrupt level for this device */
    UINT8   ier;        /* copy of ier */
    UINT8   lcr;        /* copy of lcr, not used by ns16552 driver */
    UINT8   mcr;        /* copy of modem control register */
    UINT8   ierInit;    /* initial IER register value */
    UINT16  fifoSize;   /* FIFO depth */

    UINT16      channelMode;    /* such as INT, POLL modes */
    UINT16      regDelta;   /* register address spacing */
    UINT16      irq;        /* IRQ level */
    int         baudRate;
    UINT32      xtal;       /* UART clock frequency     */
    UINT32  options;    /* hardware setup options */

    int     regIndex;   /* bus subsystem: reg base index */
    VXB_DEVICE_ID pDev;     /* bus subsystem: device ID */
    void *  handle;

    int     channelNo;  /* channel number */
    FUNCPTR pDivisorFunc; /* allow BSP to calulate divisor */
    spinlockIsr_t spinlockIsr;  /* ISR-callable spinlock */
    BOOL    txDefer;    /* TX interrupt is currently deferred */
    ISR_DEFER_QUEUE_ID queueId;
    ISR_DEFER_JOB isrDefRd;
    ISR_DEFER_JOB isrDefWr;

    /*
     * Each device may have multiple channels.  To handle this,
     * we keep a pointer to a pChan in pDev->pDrvCtrl, and maintain
     * a linked list of channels per controller.
     */
    struct ns16550vxbChan * pNext; /* next channel for this device */

    UINT32	initLevel;
    UINT8       ierValue;       /* Value for IER register */

    /*
     * DM 37X board can not read RBR directly, it must check LSR first.
     * If data ready bit is set, it can read RBR, otherwise it will
     * produce a data exception. The below item will mark if we need 
     * check LSR_DR bit before read RBR. Its default value should be
     * FALSE.
     */

    BOOL checkLSRBeforeReadData;
    
    } NS16550VXB_CHAN;

/* 16550 initialization structure for plb based vxBus devices */

typedef struct ns16550vxbChanParams
    {
    ULONG   vector;         /* Actual architectural vector */
    ULONG   baseAdrs;       /* Virt address of register set */
    ULONG   regSpace;       /* Increment between registers - usually 1 */
    ULONG   intLevel;       /* interrupt level */
    } VX_BUS_NS16550VXB_CHAN_PARAS;

METHOD_DECL(sioEnable);

#endif  /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif
 
#endif /* __INCvxbNs16550Sioh */
