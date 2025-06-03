/* st16552Sio.c - ST 16C552 DUART tty driver */

/* Copyright 1984-1999 Wind River Systems, Inc. */


/*
modification history
--------------------
01h,30jun99,jpd  improved interrupt efficiency with FIFOs (SPR #27952).
01g,23jul98,cdp  added ARM big-endian support.
01f,04dec97,jpd  updated to latest standards. Fix max/min baud setting.
01e,03oct97,jpd  Expanded comments/documentation.
01d,04apr97,jpd  Added multiplexed interrupt handler.
01c,18feb97,jpd  Tidied comments/documentation.
01b,03dec96,jpd  Added case IIR_TIMEOUT in st16550Int routine as per change
		 to version 01f of ns16550Sio.c
01a,18jul96,jpd  Created from ns1550Sio.c version 01e, by adding setting of
		 INT bit in MCR in InitChannel().
*/

/*
DESCRIPTION
This is the device driver for the Startech ST16C552 DUART, similar, but
not quite identical to the National Semiconductor 16550 UART.

The chip is a dual universal asynchronous receiver/transmitter with 16
byte transmit and receive FIFOs and a programmable baud-rate
generator. Full modem control capability is included and control over
the four interrupts that can be generated: Tx, Rx, Line status, and
modem status.  Only the Rx and Tx interrupts are used by this driver.
The FIFOs are enabled for both Tx and Rx by this driver.

Only asynchronous serial operation is supported by the UART which
supports 5 to 8 bit bit word lengths with or without parity and with
one or two stop bits. The only serial word format supported by the
driver is 8 data bits, 1 stop bit, no parity,  The default baud rate is
determined by the BSP by filling in the ST16552_CHAN structure before
calling st16552DevInit().

The exact baud rates supported by this driver will depend on the
crystal fitted (and consequently the input clock to the baud-rate
generator), but in general, baud rates from about 50 to about 115200
are possible.

.SH DATA STRUCTURES
An ST16552_CHAN data structure is used to describe the two channels of
the chip and, if necessary, an ST16552_MUX structure is used to
describe the multiplexing of the interrupts for the two channels of the
DUART.  These structures are described in h/drv/sio/st16552Sio.h.

.SH CALLBACKS
Servicing a "transmitter ready" interrupt involves making a callback to
a higher level library in order to get a character to transmit.  By
default, this driver installs dummy callback routines which do nothing.
A higher layer library that wants to use this driver (e.g. ttyDrv)
will install its own callback routine using the SIO_INSTALL_CALLBACK
ioctl command.  Likewise, a receiver interrupt handler makes a callback
to pass the character to the higher layer library.

.SH MODES
This driver supports both polled and interrupt modes.

.SH USAGE
The driver is typically only called by the BSP. The directly callable
routines in this module are st16552DevInit(), st16552Int(),
st16552IntRd(), st16552IntWr(), and st16552MuxInt.

The BSP's sysHwInit() routine typically calls sysSerialHwInit(), which
initialises all the hardware-specific values in the ST16552_CHAN
structure before calling st16552DevInit() which resets the device and
installs the driver function pointers. After this the UART will be
enabled and ready to generate interrupts, but those interrupts will be
disabled in the interrupt controller.

The following example shows the first parts of the initialisation:

.CS
#include "drv/sio/st16552Sio.h"

static ST16552_CHAN st16552Chan[N_16552_CHANNELS];

void sysSerialHwInit (void)
    {
    int i;

    for (i = 0; i < N_16552_CHANNELS; i++)
	{
	st16552Chan[i].regDelta = devParas[i].regSpace;
	st16552Chan[i].regs = devParas[i].baseAdrs;
	st16552Chan[i].baudRate = CONSOLE_BAUD_RATE;
	st16552Chan[i].xtal = UART_XTAL_FREQ;
	st16552Chan[i].level = devParas[i].intLevel;

	/@
	 * Initialise driver functions, getTxChar, putRcvChar and
	 * channelMode and init UART.
	 @/

	st16552DevInit(&st16552Chan[i]);
	}
    }
.CE

The BSP's sysHwInit2() routine typically calls sysSerialHwInit2(),
which connects the chips interrupts via intConnect() (either the single
interrupt `st16552Int', the three interrupts `st16552IntWr',
`st16552IntRd', and `st16552IntEx', or the multiplexed interrupt
handler `st16552MuxInt' which will cope with both channels of a DUART
producing the same interrupt). It then enables those interrupts in the
interrupt controller as shown in the following example:

.CS
void sysSerialHwInit2 (void)
    {
    /@ Connect the multiplexed interrupt handler @/

    (void) intConnect (INUM_TO_IVEC(devParas[0].vector),
			st16552MuxInt, (int) &st16552Mux);
    intEnable (devParas[0].intLevel);
    }
.CE

.SH BSP
By convention all the BSP-specific serial initialisation is performed
in a file called sysSerial.c, which is #include'ed by sysLib.c.
sysSerial.c implements at least four functions, sysSerialHwInit()
sysSerialHwInit2(), sysSerialChanGet(), and sysSerialReset(). The first
two have been described above, the others work as follows:

sysSerialChanGet is called by usrRoot to get the serial channel
descriptor associated with a serial channel number. The routine takes a
single parameter which is a channel number ranging between zero and
NUM_TTY. It returns a pointer to the corresponding channel descriptor,
SIO_CHAN *, which is just the address of the ST16552_CHAN strucure.

sysSerialReset is called from sysToMonitor() and should reset the
serial devices to an inactive state (prevent them from generating any
interrupts).

.SH INCLUDE FILES:
drv/sio/st16552Sio.h sioLib.h

SEE ALSO:
.I "Startech ST16C552 Data Sheet"
*/

#include "vxWorks.h"
#include "errnoLib.h"
#include "errno.h"
//#include "intLib.h"
#include "sioLib.h"
#include "logLib.h"
#include "stdio.h"
#include "stdlib.h"
#include "../PL_Reg.h"

#include "uartDriver.h"


//#include "common/debug.h"



#undef DBG_LEVEL
#define DBG_LEVEL -1

//串口设备名
char *Rs485DevName[2]={
	"/Rs485/0",
	"/Rs485/1",
};


/* 中断调试打印 */
#if 0
#define UART_INT_DEBUG 
#endif


#define RS485_TX_RX_SET		(0x8000)

#define UART_INT_INDEX 0
#define FPGA_IRQ_REG_DELTA			4

#define FPGA_INT_CORE_VER			(0x00*FPGA_IRQ_REG_DELTA)
#define FPGA_INT_TEST_REG			(0x01*FPGA_IRQ_REG_DELTA)
#define FPGA_INT_INT_EN				(0x11*FPGA_IRQ_REG_DELTA)
#define FPGA_INT_INT_ISR			(0x12*FPGA_IRQ_REG_DELTA)
#define FPGA_INT_INT_CLR			(0x13*FPGA_IRQ_REG_DELTA)
#define FPGA_INT_INT_MODE			(0x14*FPGA_IRQ_REG_DELTA)
#define FPGA_INT_INT_FORCE			(0x16*FPGA_IRQ_REG_DELTA)
#define FPGA_INT_EDGE_L				(0x17*FPGA_IRQ_REG_DELTA)
#define FPGA_INT_EDGE_H				(0x18*FPGA_IRQ_REG_DELTA)
#define FPGA_INT_INT_CNT_L			(0x19*FPGA_IRQ_REG_DELTA)
#define FPGA_INT_INT_CNT_H			(0x1a*FPGA_IRQ_REG_DELTA)
#define FPGA_INT_INT_SRC			(0x1b*FPGA_IRQ_REG_DELTA)


uart_DEV uartDev[MAX_UART_NUM];


#if 0
#define MYUART_REG_READ(pChan, reg, result) result =sysInWord((UINT16)pChan->regs + (reg*pChan->regDelta))
#define MYUART_REG_WRITE(pChan, reg, data) sysOutWord((UINT16)pChan->regs + (reg*pChan->regDelta), data)

#define MY_REG_WRITE(reg,data)		sysOutWord((UINT16)(reg),data)
#define MY_REG_READ(reg,result)		result =sysInWord((UINT16)(reg))
#else

#define FPGA_DELTA		(4)

#define MYUART_REG_READ(pChan, offset, result)	(result) = *(volatile UINT32*)((UINT32)pChan->regs + (offset)*FPGA_DELTA)
#define MYUART_REG_WRITE(pChan, offset, value)	*(volatile UINT32 *)((UINT32)pChan->regs + (offset)*FPGA_DELTA) = (value)
#define MY_REG_WRITE(reg, data)		*(volatile UINT32 *)((UINT32)(reg)) = (data)		
#define MY_REG_READ(reg, result)		(result) = *(volatile UINT32 *)((UINT32)(reg))	
#endif


#ifdef _WRS_CONFIG_SMP
static int intLock(int intLev)
{
	SPIN_LOCK_ISR_TAKE(intLev);
	return 0;
}

static void intUnlock(int intLev)
{
	SPIN_LOCK_ISR_GIVE (intLev);
	return 0;
}

#endif




/* locals */
static BOOL multiplexed = FALSE;	/* whether int from mux-ed handler */
static char intStatus;

/* function prototypes */
static STATUS st16552DummyCallback ();
static void st16552InitChannel (UART_CHAN *pChan);
static STATUS st16552Ioctl (SIO_CHAN * pSioChan, int request, int arg);
static int st16552TxStartup (SIO_CHAN * pSioChan);
static int st16552CallbackInstall(SIO_CHAN * pSioChan, int callbackType,STATUS (*callback)(), void * callbackArg);
static int st16552PollInput (SIO_CHAN * pSioChan, char *);
static int st16552PollOutput (SIO_CHAN * pSioChan, char);


/* driver functions */
static SIO_DRV_FUNCS st16552SioDrvFuncs =
{
	(int (*)())st16552Ioctl,
	st16552TxStartup,
	st16552CallbackInstall,
	st16552PollInput,
	st16552PollOutput
};



/*******************************************************************************
*
* st16552DummyCallback - dummy callback routine.
*
* RETURNS: ERROR, always.
*/

static STATUS st16552DummyCallback (void)
    {
    return ERROR;
    }

/*******************************************************************************
*
* st16552DevInit - initialise an ST16552 channel
*
* This routine initialises some SIO_CHAN function pointers and then resets
* the chip in a quiescent state.  Before this routine is called, the BSP
* must already have initialised all the device addresses, etc. in the
* ST16552_CHAN structure.
*
* RETURNS: N/A
*/

static void st16552DevInit
    (
    UART_CHAN *	pChan
    )
    {

    int oldlevel = intLock(&pChan->spinLock);

    /* initialise the driver function pointers in the SIO_CHAN */

    pChan->sio.pDrvFuncs = &st16552SioDrvFuncs;

    /* set the non BSP-specific constants */

    pChan->getTxChar = st16552DummyCallback;
    pChan->putRcvChar = st16552DummyCallback;

    pChan->channelMode = SIO_MODE_INT; /* 中断模式:SIO_MODE_INT */ /* 轮询模式:SIO_MODE_POLL */  

    /* reset the chip */

    st16552InitChannel(pChan);
   
    intUnlock(&pChan->spinLock);
	
    }


/*******************************************************************************
*
* st16552InitChannel - initialise UART
*
* This routine performs hardware initialisation of the UART channel.
*
* RETURNS: N/A
*/

static void st16552InitChannel
    (
    UART_CHAN *	pChan	/* ptr to ST16552 struct describing channel */
    )
    {
    /* Configure Port -  Set 8 bits, 1 stop bit, no parity. */

    /* keep soft copy */
	/* Restore Line Control Register */

	//关闭 DLAB
    MYUART_REG_WRITE(pChan, LCR, (pChan->lcr&0x7F));

	//8个数据位    ，1个停止位，无校验位
    pChan->lcr = (UINT8)(CHAR_LEN_8 | ONE_STOP | PARITY_NONE);

    /* Set Line Control Register */


	//写寄存器
    MYUART_REG_WRITE(pChan,LCR, pChan->lcr);
	
#if 1

    /* Reset/Enable the FIFOs */

	//RxCLEAR/TxCLEAR 复位发送和接收 FIFO
	//FIFO_ENABLE   FIFO 使能
	

    MYUART_REG_WRITE(pChan, FCR, RxCLEAR | TxCLEAR | FIFO_ENABLE);

	
    /* Enable access to the divisor latches by setting DLAB in LCR. */
	//使能DLAB 从而可以设置波特率了
    MYUART_REG_WRITE(pChan, LCR, LCR_DLAB | pChan->lcr);

//OK

    /* Set divisor latches to set baud rate */
	//设置波特率
    MYUART_REG_WRITE(pChan, DLL,  pChan->xtal/(8*pChan->baudRate));
    MYUART_REG_WRITE(pChan, DLM, (pChan->xtal/(8*pChan->baudRate)) >> 8);


	/* Restore Line Control Register */
	//关闭 DLAB
    MYUART_REG_WRITE(pChan, LCR, pChan->lcr);
    /*
     * This appears to be different from the NS 16550, which defines Bit 3 to
     * be a general purpose output. The ST 16552 datasheet defines it to set
     * the INT output pin from tri-state to normal active mode. If this is not
     * done, then no interrupts are generated, at least on the ARM PID7T
     * board.
     */

    /* Set INT output pin to normal/active operating mode and assert DTR MCR_INT | MCR_DTR |*/
	pChan->mcr = MCR_INT;   //MCR_RTS|
	
   	MYUART_REG_WRITE(pChan,MCR,pChan->mcr);

    /* Make a copy of Interrupt Enable Register */

    pChan->ier = (UINT8)(RxFIFO_BIT);


    /* Disable interrupts */

    MYUART_REG_WRITE(pChan, IER, 0);			//中断禁止
#endif
    }

/*******************************************************************************
*
* st16552Ioctl - special device control
*
* This routine handles the IOCTL messages from the user.
*
* RETURNS: OK on success, EIO on device error, ENOSYS on unsupported
* request.
*/

static STATUS st16552Ioctl
    (
    SIO_CHAN *		pSioChan, /* ptr to SIO_CHAN describing this channel */
    int			request,  /* request code */
    int			arg	  /* some argument */
    )
{
    int 		oldlevel=0; /* current interrupt level mask */
    STATUS		status;	  /* status to return */
    UART_CHAN *	pChan = (UART_CHAN *)pSioChan;
    UINT32		brd=0;	  /* baud rate divisor */
	char temp = 0;
	UINT8 regV;

    status = OK;	/* preset to return OK */

    switch (request)
	{
	//波特率设置
	case SIO_BAUD_SET:
	    /*
	     * Set the baud rate. Return EIO for an invalid baud rate, or
	     * OK on success.
	     *
	     * baudrate divisor must be non-zero and must fit in a 16-bit
	     * register.
	     */
			   /* Enable access to the divisor latches by setting DLAB in LCR. */
	//	//使能DLAB 从而可以设置波特率了
	//    MYUART_REG_WRITE(pChan, LCR, LCR_DLAB | pChan->lcr);
	//
	////OK
	//
	//    /* Set divisor latches to set baud rate */
	//	//设置波特率
	//    MYUART_REG_WRITE(pChan, DLL,  pChan->xtal/(8*pChan->baudRate));
	//    MYUART_REG_WRITE(pChan, DLM, (pChan->xtal/(8*pChan->baudRate)) >> 8);
	//
	//
	//	/* Restore Line Control Register */
	//	//关闭 DLAB
	//    MYUART_REG_WRITE(pChan, LCR, pChan->lcr);



	    brd = pChan->xtal/(8*arg);	/* calculate baudrate divisor */

	    if ((brd < 1) || (brd > 0xFFFF))
    	{
	    	status = EIO;		/* baud rate out of range */
	    	break;
    	}

	    /* disable interrupts during chip access */

		//printf("pChan->lcr = 0x%x\n",pChan->lcr);
		
	    oldlevel = intLock (&pChan->spinLock);

	    /* Enable access to the divisor latches by setting DLAB in LCR. */

	    MYUART_REG_WRITE(pChan, LCR, LCR_DLAB | pChan->lcr);


	    /* Set divisor latches. */

	    MYUART_REG_WRITE(pChan, DLL, brd);
	    MYUART_REG_WRITE(pChan, DLM, brd >> 8);

	    /* Restore Line Control Register */

	    MYUART_REG_WRITE(pChan, LCR, pChan->lcr);

	    pChan->baudRate = arg;

	    intUnlock (&pChan->spinLock);

	    break;

	case RS485_TX_RX_SET:	/* LCR[5] RS485 TXRX 使能 0,RX !0,TX */
	{ 
		//PDEBUG(1, "ioctl RS485  arg=%d \n", arg);
		if (arg) 
		{ /* TX */
			MYUART_REG_READ(pChan, LCR, regV);
			regV |= (1<<5);
			MYUART_REG_WRITE(pChan, LCR, regV);
			pChan->lcr |= (1<<5);
		} 
		else 
		{
			MYUART_REG_READ(pChan, LCR, regV);
			regV &= (~(1<<5));
			MYUART_REG_WRITE(pChan, LCR, regV);

			pChan->lcr &= (~(1<<5));
		}
		break;
	}

	//波特率获取
	case SIO_BAUD_GET:
	    *(int *)arg = pChan->baudRate;
	    break;
#if 0
	//模式设置
	case SIO_MODE_SET:
	    if ((arg != SIO_MODE_POLL) && (arg != SIO_MODE_INT))
		{
		status = EIO;
		break;
		}

	    oldlevel = intLock ();

	    if (arg == SIO_MODE_INT)
		{
		/* Enable appropriate interrupts */

		MYUART_REG_WRITE(pChan,
				IER, pChan->ier | RxFIFO_BIT | TxFIFO_BIT);
		}
	    else
		{
		/* Disable the interrupts */

		MYUART_REG_WRITE(pChan, IER, 0);
		}

	    pChan->channelMode = arg;

	    intUnlock(oldlevel);
	    break;


	case SIO_MODE_GET:
	    *(int *)arg = pChan->channelMode;
	    break;
#endif

	case SIO_AVAIL_MODES_GET:
	{
	    *(int *)arg = SIO_MODE_INT|SIO_MODE_POLL;
	    break;
	}
/*	case SIO_LOOP_SET:
	{
		if(0==arg)
		{
			pChan->mcr &= (~MCR_LOOP);
		}
		else
		{
			pChan->mcr = pChan->mcr|MCR_LOOP;
		}
		MYUART_REG_WRITE(pChan, MCR, pChan->mcr);
		break;
	}
*/

	case SIO_HW_OPTS_SET:
	{
		MYUART_REG_READ(pChan, LCR, pChan->lcr);
		
		switch(arg&(PARODD|PARENB))
		{
			case PARODD|PARENB:    //奇校验
			{
				pChan->lcr &= (~LCR_SP);
				pChan->lcr |= LCR_PEN;		
				pChan->lcr &= (~LCR_EPS);				
				break;
			}
			
			case PARENB:			//偶校验
			{
				pChan->lcr &= (~LCR_SP);
				pChan->lcr |= LCR_PEN;
				pChan->lcr |= LCR_EPS;
				break;
			}

			default:				//无校验
			{
				pChan->lcr &= (~LCR_SP);
				pChan->lcr &= (~LCR_PEN);
				pChan->lcr &= (~LCR_EPS);
				break;
			}

		}
		pChan->lcr &= 0xFC;
		pChan->lcr |= (arg>>2)&0x03;
		MYUART_REG_WRITE(pChan, LCR, pChan->lcr);
		/*
		sysUsDelay(1000);
		MYUART_REG_READ(pChan, LCR, regV);
		printf("lcr = 0x%x  0x3无校验 0xb奇校验 0x1b偶校验\n", regV);
		*/
		break;
	}
	case SIO_HW_OPTS_GET:
	{ 
		temp = (pChan->lcr&0x03)<<2;
		if(((pChan->lcr&LCR_PEN)>0)&&((pChan->lcr&LCR_EPS)>0))
		{
			temp|= PARENB;
		}
		else if((pChan->lcr&LCR_PEN)>0)
		{
			temp|= (PARENB|PARODD);
		}
		*((int*)arg) = temp;
		break;
	}

	default:
	    status = ENOSYS;
	}

    return status;

}

/*******************************************************************************
*
* st16552IntWr - handle a transmitter interrupt
*
* This routine handles write interrupts from the UART.
*
* RETURNS: N/A
*/
static void st16552IntWr
    (
    UART_CHAN *	pChan	/* ptr to struct describing channel */
    )
    {
    char outChar;
    BOOL gotOne;
    UINT32 status;
	UINT32 test;

    do
    {
		/* get a character to send, if available */
		gotOne = (*pChan->getTxChar) (pChan->getTxArg, &outChar) != ERROR;
		
		if (gotOne)				/* there was one, so send it */
		{
			MYUART_REG_WRITE(pChan, THR, outChar);	/* write char to THR */
			#ifdef UART_INT_DEBUG
			logMsg("myUART:send: %d\n",outChar,0,0,0,0,0);
			#endif
		}
		
		MYUART_REG_READ(pChan, LSR, status);	/* read status */
		#ifdef UART_INT_DEBUG
		logMsg("myUART:pChan(0x%x)---status = 0x%x\n",pChan->regs,status,0,0,0,0);
		#endif
		
   	} while (gotOne && ((status & LSR_THRE) != 0));
		
#ifdef UART_INT_DEBUG
       logMsg("myUART:Over pChanLSR(0x%x)---status = 0x%x\n",pChan->regs,status,0,0,0,0);
#endif
    
       MYUART_REG_READ(pChan, IER, test);	

#ifdef UART_INT_DEBUG	   
	logMsg("myUART:Over pChanIER(0x%x)---status = 0x%x\n",pChan->regs,test,0,0,0,0);	
#endif	

	if (!gotOne)
	{
		#ifdef UART_INT_DEBUG
		/* no more chars to send, disable Tx interrupt */
		logMsg("myUART:Disable = 0x%x\n",gotOne,0,0,0,0,0);
		#endif
		
		pChan->ier &= (~TxFIFO_BIT);			/* update copy */
		MYUART_REG_WRITE(pChan, IER, pChan->ier);	/* disable Tx int */
	}

    return;
    }

/*****************************************************************************
*
* st16552IntRd - handle a receiver interrupt
*
* This routine handles read interrupts from the UART.
*
* RETURNS: N/A
*/
static void st16552IntRd
    (
    UART_CHAN *	pChan	/* ptr to struct describing channel */
    )
    {
    char inchar;
    UINT32 status;

    do
	{
	/* read character from Receive Holding Reg. */

	MYUART_REG_READ(pChan, RBR, inchar);
	#ifdef UART_INT_DEBUG
	/* send it on to upper level, via installed callback */
	logMsg(">>>>>>myUart REC:%d\n",inchar,0,0,0,0,0);
	#endif	
	(*pChan->putRcvChar) (pChan->putRcvArg, inchar);

	MYUART_REG_READ(pChan, LSR, status);	/* read status, more chars? */
	}
    while ((status & LSR_DR) != 0);

    return;
    }

/**********************************************************************
*
* st16552IntEx - miscellaneous interrupt processing
*
* This routine handles miscellaneous interrupts on the UART.
*
* RETURNS: N/A
*/

static void st16552IntEx
    (
    UART_CHAN *	pChan	/* ptr to struct describing channel */
    )
    {

    /* Nothing for now... */
    }

/******************************************************************************
*
* st16552Int - interrupt level processing
*
* This routine handles interrupts from the UART.
*
* RETURNS: N/A
*/

static void st16552Int
    (
    UART_CHAN *	pChan	/* ptr to struct describing channel */
    )
    {
    /*
     * If this routine has been called from the multiplexed interrupt
     * handler, then we have already read the Interrupt Status Register,
     * and must not read it again, else this routine has been installled
     * directly using intConnect() and we must now read the Interrupt
     * Status Register (or Interrupt Identification Register).
     */

	char intStatus;
	char test;
	unsigned short fpgaRegSta,fpgaRegEn;

	int intKey;
	intKey = intLock(&pChan->spinLock);

     if (!multiplexed)
	 {
	 	MYUART_REG_READ(pChan, IIR, intStatus);
	 	intStatus &= 0x0F;
	#ifdef UART_INT_DEBUG
	 	logMsg("myUart Start IIR(0x%x):%d\n",pChan->regs,intStatus,0,0,0,0);
	#endif
	 }

	/* 获取逻辑中断复用模块中断状态 */
	#if 1 /* 一般情况下,我们的项目没有使用中断复用模块 */
	MY_REG_READ(pChan->fpgaIntBase+FPGA_INT_INT_ISR,fpgaRegSta);

	
	if(!(fpgaRegSta & pChan->intBit))
	{
			#ifdef UART_INT_DEBUG
		logMsg("fpga int module is not interrupt(0x%x & 0x%x)\n",fpgaRegSta, pChan->intBit,0,0,0,0);
			#endif
		intUnlock(&pChan->spinLock);
		return;
	}
	else
	{
		#ifdef UART_INT_DEBUG
		logMsg("fpga int module is interrupt(0x%x & 0x%x)\n",fpgaRegSta,pChan->intBit,0,0,0,0);
			#endif
	}
	#else
	if (intStatus & 0x0001) {
		#ifdef UART_INT_DEBUG
		logMsg("logic int module is not interrupt( 0x%x )\n", pChan->intBit,0,0,0,0,0);
		#endif
		return;
	}
	else
		#ifdef UART_INT_DEBUG
		logMsg("logic int module is interrupt( 0x%x )\n",pChan->intBit,0,0,0,0,0);
		#endif
	#endif

	/* hp noted: 在A4项目上验证,需要关闭中断复用才能收到其它通道中断 */
#if 1 /*不需要关闭中断,只清除状态就可以*/
	/*关闭逻辑中断复用*/
	MY_REG_READ(pChan->fpgaIntBase+FPGA_INT_INT_EN,fpgaRegEn);
	fpgaRegEn = fpgaRegEn & (~(pChan->intBit));
	MY_REG_WRITE(pChan->fpgaIntBase+FPGA_INT_INT_EN,fpgaRegEn);
#endif
	
    /*
     * This UART chip always produces level active interrupts, and the IIR
     * only indicates the highest priority interrupt.
     * In the case that receive and transmit interrupts happened at
     * the same time, we must clear both interrupt pending to prevent
     * edge-triggered interrupt(output from interrupt controller) from locking
     * up. One way doing it is to disable all the interrupts at the beginning
     * of the ISR and enable at the end.
     */
    MYUART_REG_WRITE(pChan, IER, 0);			/* disable interrupts */

    switch (intStatus)
	{
	case IIR_RLS:
	    /*
	     * overrun, parity error and break interrupt:
	     *
	     * read LSR to reset interrupt
	     */

	    //MYUART_REG_READ(pChan, LSR, intStatus);
	    break;

	case IIR_RDA:
	    /* received data available: FALL THROUGH to timeout */

	case IIR_TIMEOUT:
	    /*
	     * receiver FIFO interrupt. In some cases, IIR_RDA
	     * will not be indicated in IIR register when there
	     * is more than one char. in FIFO.
	     */
		//logMsg("myUart IIR:%d\n",intStatus,0,0,0,0,0);
	    st16552IntRd (pChan);	/* at least one RxChar available */
	    break;

	case IIR_THRE:
		#ifdef UART_INT_DEBUG
		logMsg("myUart IIR:%d\n",intStatus,0,0,0,0,0);
		#endif
	    st16552IntWr (pChan);	/* can transmit at least one char */
	    break;

	case IIR_MSTAT:
		//MYUART_REG_READ(pChan, MSR, intStatus);
		
		break;
	default:
	    break;
	}

	/*清除逻辑中断复用*/
	fpgaRegSta = fpgaRegSta & pChan->intBit;
	MY_REG_WRITE(pChan->fpgaIntBase+FPGA_INT_INT_CLR,fpgaRegSta);
	
	MYUART_REG_WRITE(pChan, IER, pChan->ier); /* 开中断 */

	#ifdef UART_INT_DEBUG
	MYUART_REG_READ(pChan, IER, test);	
	logMsg("myUART: OVER !!!IER(0x%x)--status = 0x%x\n",(pChan->regs+IER),test,0,0,0,0);
	#endif

	#if 1
	/*开启逻辑中断复用*/
	fpgaRegEn = fpgaRegEn | pChan->intBit;
	MY_REG_WRITE(pChan->fpgaIntBase+FPGA_INT_INT_EN,fpgaRegEn);
	#endif

	intUnlock(&pChan->spinLock);
}

/******************************************************************************
*
* st16552MuxInt - multiplexed interrupt level processing
*
* This routine handles multiplexed interrupts from the DUART. It assumes that
* channels 0 and 1 are connected so that they produce the same interrupt.
*
* RETURNS: N/A
*/

static void st16552MuxInt
    (
    ST16552_MUX *	pMux	/* ptr to struct describing multiplexed chans */
    )
    {
    UART_CHAN *	pChan;

    /* get pointer to structure for channel to examine first */

    pChan = &(pMux->pChan[pMux->nextChan]);


    /*
     * Step on the next channel to examine: use round-robin for which to
     * examine first.
     */

    if (pMux->nextChan == 0)
	pMux->nextChan = 1;
    else
	pMux->nextChan = 0;

    /*
     * Let the st16552Int() routine know it is called from here, not direct
     * from intConnect().
     */

    multiplexed = TRUE;


    /* check Interrupt Status Register for this channel */

    MYUART_REG_READ(pChan, IIR, intStatus);
    intStatus &= 0x0F;


    if ((intStatus & 0x01) == 0)
	{
	/* Call int handler if int active */

	st16552Int (pChan);
	}
    else
	{
	/* step on again */

	pChan = &pMux->pChan[pMux->nextChan];

	if (pMux->nextChan == 0)
	    pMux->nextChan = 1;
	else
	    pMux->nextChan = 0;


	/* get interrupt status for next channel */

	MYUART_REG_READ(pChan, IIR, intStatus);
	intStatus &= 0x0F;


	/* and call interrupt handler if active */

	if ((intStatus & 0x01) == 0)
	    st16552Int (pChan);
	}

    multiplexed = FALSE;

    return;
    }

/*******************************************************************************
*
* st16552TxStartup - transmitter startup routine
*
* Call interrupt level character output routine and enable interrupt!
*
* RETURNS: OK on success, ENOSYS if the device is polled-only, or
* EIO on hardware error.
*/

static int st16552TxStartup
    (
    SIO_CHAN *		pSioChan  /* ptr to SIO_CHAN describing this channel */
    )
{
    UART_CHAN *	pChan = (UART_CHAN *)pSioChan;

    if (pChan->channelMode == SIO_MODE_INT) 
    {
		
		pChan->ier |= TxFIFO_BIT;
			
		MYUART_REG_WRITE(pChan, IER, pChan->ier);
		
#ifdef UART_INT_DEBUG		
		logMsg("st16552TxStartup OK pChan->ier=0x%x \n", pChan->ier, 2,3,4,5,6);
#endif		
		return OK;
	}
    else
		return ENOSYS;
}

/******************************************************************************
*
* st16552PollOutput - output a character in polled mode.
*
* RETURNS: OK if a character arrived, EIO on device error, EAGAIN
* if the output buffer is full, ENOSYS if the device is interrupt-only.
*/

static int st16552PollOutput
    (
    SIO_CHAN *		pSioChan, /* ptr to SIO_CHAN describing this channel */
    char		outChar	  /* char to be output */
    )
    {
    UART_CHAN *	pChan = (UART_CHAN *)pSioChan;
    char pollStatus;

    MYUART_REG_READ(pChan, LSR, pollStatus);

    /* is the transmitter ready to accept a character? */

    if ((pollStatus & LSR_THRE) == 0x00)
	return EAGAIN;

	
    /* write out the character */

    MYUART_REG_WRITE(pChan, THR, outChar);	/* transmit character */

    return OK;
    }

/******************************************************************************
*
* st16552PollInput - poll the device for input.
*
* RETURNS: OK if a character arrived, EIO on device error, EAGAIN
* if the input buffer is empty, ENOSYS if the device is interrupt-only.
*/

static int st16552PollInput
    (
    SIO_CHAN *		pSioChan, /* ptr to SIO_CHAN describing this channel */
    char *		thisChar  /* ptr to where to return character */
    )
    {
    UART_CHAN *	pChan = (UART_CHAN *)pSioChan;
    char pollStatus;

    MYUART_REG_READ(pChan, LSR, pollStatus);

    if ((pollStatus & LSR_DR) == 0x00)
	return EAGAIN;


    /* got a character */

    MYUART_REG_READ(pChan, RBR, *thisChar);

    return OK;

    }

/******************************************************************************
*
* st16552CallbackInstall - install ISR callbacks to get/put chars.
*
* This routine installs interrupt callbacks for transmitting characters
* and receiving characters.
*
* RETURNS: OK on success, or ENOSYS for an unsupported callback type.
*/

static int st16552CallbackInstall
    (
    SIO_CHAN *	pSioChan,	/* ptr to SIO_CHAN describing this channel */
    int		callbackType,	/* type of callback */
    STATUS	(*callback)(),	/* callback */
    void *	callbackArg	/* parameter to callback */

    )
{
	UART_CHAN * pChan = (UART_CHAN *)pSioChan;

	switch (callbackType)
	{
		case SIO_CALLBACK_GET_TX_CHAR:
		    pChan->getTxChar	= callback;
		    pChan->getTxArg	= callbackArg;
		    return OK;

		case SIO_CALLBACK_PUT_RCV_CHAR:
		    pChan->putRcvChar	= callback;
		    pChan->putRcvArg	= callbackArg;
		    return OK;

		default:
		    return ENOSYS;
	}
}




LOCAL UART_CHAN uartBk;
int sysUartRd(UINT16 reg)
{
	UART_CHAN* uart = &uartBk;
	unsigned short regV;
	MYUART_REG_READ(uart, reg, regV);
	return regV;
}

int sysUartWr(UINT16 reg, UINT16 val)
{
	UART_CHAN* uart = &uartBk;
	unsigned short regV;
	MYUART_REG_WRITE(uart, reg, val);
	return 0;
}


LOCAL STATUS serialInit(UART_CHAN* uart, UINT32 udBaseAddr, char* DevName, unsigned int intBit)
{
    int i=0;
    int oldlevel=0;
    char bResult = 0;
    char* NameBuffer[32];
	unsigned short regV;
    INTERRUPT_TRIG_T intMode;
    
    intMode = INTR_TRIGGER_HIGH_LEVEL;
		
	sprintf(NameBuffer, "%s", DevName);
	
	uart->regDelta = TLC16C554_DELTA_REG;
	uart->regs = udBaseAddr;
	uart->baudRate = CONSOLE_BAUD_RATE;
	uart->xtal = UART_XTAL_FREQ;
	
	spinLockIsrInit (&(uart->spinLock), 0);

	//RS485 中断编号
	uart->level = RS485_IRQ_NUM;		
	uart->fpgaIntBase = FPGA_IRQ_MULTI_BASE; /* UART[0:8]RS422使用中断0 */	
	uart->intBit = intBit;
	
	st16552DevInit(uart);
	
#if 1
	/* 关闭中断 */
	intDisable(uart->level);	
    oldlevel = intLock(&uart->spinLock);//这个地方的用法不知道是否有问题
	if (intConnect((VOIDFUNCPTR *)uart->level, (VOIDFUNCPTR)st16552Int, (int)uart)==ERROR)
	{
		printf("intConnect ERROR!\n");	
		return -1;
	}
	intUnlock(&uart->spinLock);

	(void)ttyDevCreate (NameBuffer, uart, UART_BUFFER_SIZE, UART_BUFFER_SIZE);

	MYUART_REG_WRITE(uart, IER, RxFIFO_BIT/*|IER_ETHREI*/);			//打开接收中断


	/* 配置逻辑中断复用模块 */
	#if 1
	MY_REG_READ(uart->fpgaIntBase+FPGA_INT_INT_EN, regV);
	regV = regV |intBit;
	MY_REG_WRITE(uart->fpgaIntBase+FPGA_INT_INT_EN, regV);/*开中断*/

	MY_REG_READ(uart->fpgaIntBase+FPGA_INT_INT_MODE, regV);
	regV = regV & (~intBit);
	MY_REG_WRITE(uart->fpgaIntBase+FPGA_INT_INT_MODE, regV);/*中断模式--电平中断*/
	#endif
	
	/* 打开逻辑还回功能 */
//	MYUART_REG_WRITE(uart, FPGA_LOOP, 0x1); 

//	plIntModeSet(uart->level,INTR_TRIGGER_HIGH_LEVEL);/*高电平触发*/
	/* 开中断 */
	intEnable (uart->level);	
#endif

	return OK;
}


int uartIntSim(unsigned int chn)
{
	UART_CHAN * pUart=(UART_CHAN *)&uartDev[0];
	unsigned short regV;
	unsigned int i = 0;



	MY_REG_READ(pUart->fpgaIntBase+FPGA_INT_INT_FORCE,regV);
	regV = regV |pUart->intBit;
	MY_REG_WRITE(pUart->fpgaIntBase+FPGA_INT_INT_FORCE,regV);
	taskDelay(100);
	printf("\n\rUART[%d] force int[0x%x]\n\r", chn,pUart->intBit);
}

int IrqIntSim(unsigned int chn)
{
	UART_CHAN * pUart=(UART_CHAN *)&uartDev[0];
	unsigned short regV;
	unsigned int i = 0;

	MY_REG_WRITE(pUart->fpgaIntBase+FPGA_INT_INT_MODE, 0);/*中断模式--电平中断*/
	
	MY_REG_WRITE(pUart->fpgaIntBase+FPGA_INT_INT_EN, 0xFF);


	MY_REG_READ(pUart->fpgaIntBase+FPGA_INT_INT_FORCE,regV);
	regV = regV |BIT(chn);
	MY_REG_WRITE(pUart->fpgaIntBase+FPGA_INT_INT_FORCE,regV);
	taskDelay(100);
	printf("\n\rUART[%d] force int[0x%x]\n\r", chn, BIT(chn));

}


/*
* 初始化串口设备 在系统启动时调用
*/
//Charlotte tilbury stoned rose
STATUS uartDevInit()
{
	int i;
	int i;
	
	uartDev[0].RS485 = 1;
	serialInit(&uartDev[0], RS485_0_BASE_ADDR, "/Rs485/0", BIT(0));

	uartDev[1].RS485 = 1;
	serialInit(&uartDev[1], RS485_1_BASE_ADDR, "/Rs485/1", BIT(1));
	return OK;
}


/******************************************************************************************/
#if 1
#define HUPCL	0x10
#define STOPB	0x20
#define PARENB	0x40
#define PARODD	0x80

#define CS5		0x0 /* 5bits */
#define CS6		0x4 /* 6bits */
#define CS7		0x8 /* 7bits */
#define CS8		0xc /* 8bits */

STATUS rdwrUart(char *devName, char *wrBuf, char *rdBuf, int length, UINT32 bRate, UINT32 odd,int flag)//flag 0->tx 1->rx
{
	int fd = -1;
	int rtn = -1;
	int ix = 0;
	int cnt = 0;

	if (0 > (fd = open(devName, 0x202, 666)))
	{
		printf("open device failed \n");
		return ERROR;
	}

	/* 波特率设置 */
	ioctl(fd, SIO_BAUD_SET, bRate); 
	/* 奇偶校验设置 */
	ioctl(fd, SIO_HW_OPTS_SET, odd); 
 if(!flag)
 {
	rtn = write(fd, wrBuf, length);
	sysUsDelay(1000);
	if ( rtn != length)
		printf("write data err. rtn=0x%x \n", rtn);
	else
		printf("write data ok! rtn=0x%x\n", rtn);

	sysDelay(10);
 }
 else
 {
	 /* RS485读之前发送读使能 */
	 do {
		 rtn = -1;
		 rtn = read(fd, rdBuf, length);
		 if (rtn <= 0)
		 {
			 printf("read data err. rtn=0x%x \n", rtn);
			 return ERROR;
		 }
		 else
			 printf("read ok. rtn=0x%x \n", rtn);
		 
		 cnt += rtn;
		 
		 for (ix=0; ix<rtn; ix++)
			 printf(" rdBuf[%x]=0x%x \n", ix, rdBuf[ix]);
		 
	 } while (cnt < length);
	 
	 printf("cnt=0x%x \n", cnt);

 }
	close(fd);
 
	return OK;
}


STATUS rdwrUart485(char *devName, char *wrBuf, char *rdBuf, int length, int chn)
{
	int fd = -1;
	int rtn = -1;
	int ix = 0;
	int cnt = 0;

	if (0 > (fd = open(devName, 0x202, 666))) {
		printf("open device failed \n");
		return ERROR;
	}

#define RS485_TXRX_SET (0x8000)
#define RS485_TX (1)
#define RS485_RX (0)

	/* RS485 0通道使能发送 */
	if (chn == 0) {
		
		ioctl(fd, RS485_TXRX_SET, RS485_TX);
		rtn = write(fd, wrBuf, length);
		sysUsDelay(1000);
		if ( rtn != length)
			printf("write data err. rtn=0x%x \n", rtn);
		else
			printf("write data ok! rtn=0x%x\n", rtn);
		
	/* RS485 1~7 通道 发送读使能 */
	} else {

		ioctl(fd, RS485_TXRX_SET, RS485_RX);

		do {
			rtn = -1;
			rtn = read(fd, rdBuf, length);
			if (rtn <= 0)
			{
				printf("read data err. rtn=0x%x \n", rtn);
				return ERROR;
			}
			else
				printf("read ok. rtn=0x%x \n", rtn);
		
			cnt += rtn;
		
			for (ix=0; ix<rtn; ix++)
				printf(" rdBuf[%x]=0x%x \n", ix, rdBuf[ix]);
		
		} while (cnt < length);

		printf("cnt=0x%x \n", cnt);
	}

	close(fd);

	return OK;
}




/* 
* 串口测试代码 
*	chn是串口通道
*/
STATUS uartTest(int chn, UINT32 bRate, UINT32 bOdd,int flag)//flag:0->tx 1->rx
{
#define WR_SIZE	 512
#define RD_SIZE	 512

	UINT32 rate;
	UINT32 odd;

	/* 波特率是需要实现常用的几种即可 */
	if ((bRate!=9600)&&(bRate!=230400)&&(bRate!=921600))
		rate = 115200;
	else
		rate = bRate;

	/* 1奇校验 2偶校验 0无校验 */
	if (bOdd == 0)
		odd = CS8;
	else if (bOdd == 1)
		odd = CS8|PARODD|PARENB;
	else if (bOdd == 2)
		odd = CS8|PARENB;

	char wrBuf[WR_SIZE];
	char rdBuf[RD_SIZE];
	int ix = chn;
	char testData = chn + 0x80;
	char testLng = 0x10;
	
	memset(rdBuf, 0xff, WR_SIZE);
	memset(wrBuf, testData, RD_SIZE);

	printf("\n");
	if(!flag)
	printf(" write data: %x  length:%x \n", testData, testLng);
	else
	printf(" rx  length:%x \n",  testLng);	
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	rdwrUart(Rs485DevName[chn], wrBuf, rdBuf, testLng, rate, odd,flag);
	printf("---------------------------------------------------------\n");
	printf("\n");

	return OK;
}


/**********************************lvvl************************************/
STATUS rwUart485(char *devName, char *wrBuf, char *rdBuf, int length,int flag)
{

	int fd = -1;
	int rtn = -1;
	int ix = 0;
	int cnt = 0;

	if (0 > (fd = open(devName, 0x202, 666))) {
		printf("open device failed \n");
		return ERROR;
	}

#define RS485_TXRX_SET (0x8000)
#define RS485_TX (1)
#define RS485_RX (0)

	/* RS485 0通道使能发送 */
	if (flag == 0) {
		
		ioctl(fd, RS485_TXRX_SET, RS485_TX);
		rtn = write(fd, wrBuf, length);
		sysUsDelay(1000);
		if ( rtn != length)
			printf("write data err. rtn=0x%x \n", rtn);
		else
			printf("write data ok! rtn=0x%x\n", rtn);
		
	/* RS485 1~7 通道 发送读使能 */
	} else {

		ioctl(fd, RS485_TXRX_SET, RS485_RX);

		do {
			rtn = -1;
			rtn = read(fd, rdBuf, length);
			if (rtn <= 0)
			{
				printf("read data err. rtn=0x%x \n", rtn);
				return ERROR;
			}
			else
				printf("read ok. rtn=0x%x \n", rtn);
		
			cnt += rtn;
		
			for (ix=0; ix<rtn; ix++)
				printf(" rdBuf[%x]=0x%x \n", ix, rdBuf[ix]);
		
		} while (cnt < length);

		printf("cnt=0x%x \n", cnt);
	}

	close(fd);

	return OK;

}
int rs485TxRx(char data,int len,int flag)//flag=0 ->tx flag=1->rx
{
	#define WR_SIZE	 512
	#define RD_SIZE	 512
		char wrBuf[WR_SIZE];
		char rdBuf[RD_SIZE];
		char testData =data;
		char testLng = len;
		
		memset(rdBuf, 0xff, WR_SIZE);
		memset(wrBuf, testData, RD_SIZE);
	
		printf("\n");
		printf(" RS485测试.\n");
		printf(" write data: %x  length:%x \n", testData, testLng);
		printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

		/* RS485_0输出 */
		rwUart485("/Rs485/0", wrBuf, rdBuf, testLng, flag);

}

#endif


#if 1
/* defines */
#define DEV_RS485_0      	 "/Rs485/0"
#define DEV_RS485_1 	 	 "/Rs485/1"

#define SIO_BAUD_SET			0x1003
#define SIO_HW_OPTS_SET			0x1005

#define DATA_COUNT     			1024

#define	O_RDONLY	0		/* +1 == FREAD */
#define	O_WRONLY	1		/* +1 == FWRITE */
#define	O_RDWR		2		/* +1 == FREAD|FWRITE */
#define STOPB		0x20	/* send two stop bits (else one) */
#define PARENB		0x40	
#define PARODD		0x80	/* 奇校验:PARODD|PARENB; 偶校验:PARENB */
#define CS5			0x0	/* 5 bits */
#define CS6			0x4	/* 6 bits */
#define CS7			0x8	/* 7 bits */
#define CS8			0xc	/* 8 bits */

#define RS485_TXRX_SET	(0x8000)
#define RS485_TX 		(1)
#define RS485_RX 		(0)

/* globals */ 
static int g_nUartBaud[] = {9600, 115200, 921600};

/* 自环测试  */
static int testRS(char *W_DEV, char *R_DEV)
{
	int write_fd, read_fd,rtn=0,cnt=0;
	unsigned char *writeBuff =(char *)malloc(DATA_COUNT*2);
	unsigned char *readBuff  =(char *)malloc(DATA_COUNT*2);
	unsigned int i=0, readcount=0,ix=0;
	
	memset(writeBuff, 0xA5, DATA_COUNT);
	memset(readBuff, 0x00, DATA_COUNT);
	
	printf("...测试通路[%s, %s] \n", W_DEV, R_DEV);
	write_fd = open(W_DEV, 0, 0);
	read_fd = open(R_DEV, 0, 0);	
	
	if(write_fd==ERROR || read_fd==ERROR)
	{
		printf("...打开失败 \n");
		goto __ERROR__;
	}
	printf("...通路打开完成 write_fd=%d, read_fd=%d \n", write_fd, read_fd);
	
	printf("...设置输入输出状态  \n");
	ioctl(write_fd, RS485_TXRX_SET, RS485_TX);
	ioctl(read_fd, RS485_TXRX_SET, RS485_RX);
	taskDelay(1);

	for(i=0; i<3; i++)	
	{
		printf("...设置通信波特率[%d]\n", g_nUartBaud[i]);
		ioctl(write_fd, SIO_BAUD_SET, g_nUartBaud[i]);
		ioctl(read_fd, SIO_BAUD_SET, g_nUartBaud[i]);
		taskDelay(1000);
		printf("...向[%s]写数据 \n", W_DEV);
		write(write_fd, writeBuff, DATA_COUNT);
		taskDelay(100);
		printf("...从[%s]读数据 \n", R_DEV);
		readcount = 0;
		rtn=0;
		while(1)
		{
			
			rtn=read(read_fd, &readBuff[readcount], DATA_COUNT);
			if(rtn<=0)
			{
				printf("rtn:%x\n",rtn);
				break;
			}
			readcount += rtn;
			taskDelay(1);
			if(readcount == DATA_COUNT)
			{
				printf("readcount:%x\n",readcount);
				break;
			}
		}
//			cnt=0;
//			do {
//			rtn = -1;
//			rtn = read(read_fd, readBuff, DATA_COUNT);
//			if (rtn <= 0)
//			{
//				printf("read data err. rtn=0x%x \n", rtn);
//				return ERROR;
//			}
//			else
//				printf("read ok. rtn=0x%x \n", rtn);
//		
//			cnt += rtn;
//		
////			for (i=0; i<rtn; i++)
////				printf(" rdBuf[%x]=0x%x \n", i, readBuff[i]);
//		} while (cnt < DATA_COUNT);
			
//		taskDelay(100);
		printf("readcnt:%d\n",cnt);
		for (ix = 0; ix < DATA_COUNT; ++ix)
		{
			if(ix%16==0)
				printf("\n");
			printf("%x ",readBuff[ix]);
		}
		if(memcmp(readBuff, writeBuff, DATA_COUNT))
		{
			printf("...读写数据不一致, 测试失败 \n");
			goto __ERROR__;
		}
		printf("...通路[%s, %s]在波特率[%d]下0x%x个数据读写完成 \n", W_DEV, R_DEV, g_nUartBaud[i], DATA_COUNT);
//		taskDelay(100);
	}
	
	close(write_fd);
	close(read_fd);
	free(writeBuff);
	free(readBuff);
	taskDelay(1);	
	return OK;
	
__ERROR__:
	if(read_fd)		close(read_fd);
	if(write_fd)	close(write_fd);
	if(writeBuff)	free(writeBuff);
	if(readBuff)	free(readBuff);
	taskDelay(1);
	return ERROR;
}

STATUS testRS485(void)
{
	int retErr=0;
	
	if(testRS(DEV_RS485_0, DEV_RS485_1) == ERROR) 	retErr++;
	if(testRS(DEV_RS485_1, DEV_RS485_0) == ERROR) 	retErr++;
	
	return retErr==0? OK: ERROR;
}

int testRs4220tx(void)
{
	int ret=0;
	unsigned char *wbuff =(char *)malloc(DATA_COUNT);
	unsigned char *rbuff =(char *)malloc(DATA_COUNT);
	memset(wbuff, 0xCD, DATA_COUNT);
	memset(rbuff, 0x00, DATA_COUNT);
	ret=memset(wbuff, 0xCD, DATA_COUNT);
	wbuff[DATA_COUNT-1]='\n';
	write(1, wbuff, DATA_COUNT);
	taskDelay(1000);
	free(wbuff);
	free(rbuff);
}
int testRs4221tx(void)
{
	int ret=0,fd=0;
	unsigned char *wbuff =(char *)malloc(DATA_COUNT);
	memset(wbuff, 0xCD, DATA_COUNT);
	ret=memset(wbuff, 0xCD, DATA_COUNT);
	wbuff[DATA_COUNT-1]='\n';
	fd = open("/tyCo/1", 0, 0);
	if(fd<=0)
		printf("open err\n");
	printf("...通路打开完成 fd=%d \n", fd);
	write(fd, wbuff, DATA_COUNT);
	taskDelay(1000);
	free(wbuff);
	close(fd);
}
int testRs4221rx(void)
{
	int ret=0,fd=0,ix=0;
	unsigned char *rbuff =(char *)malloc(DATA_COUNT);
	memset(rbuff, 0x00, DATA_COUNT);
	fd = open("/tyCo/1", 0, 0);
	if(fd<=0)
		printf("open err\n");
	printf("...通路打开完成 fd=%d \n", fd);
	ret=read(fd, rbuff, DATA_COUNT);
	printf("readcnt:%d\n",ret);
	for (ix = 0; ix < DATA_COUNT; ++ix)
	{
		if(ix%16==0)
			printf("\n");
		printf("%x ",rbuff[ix]);
	}
	taskDelay(1000);
	free(rbuff);
	close(fd);
}

STATUS testRS422(void)
{
	int fd=0, readcount=0;
	unsigned char *wbuff =(char *)malloc(DATA_COUNT);
	unsigned char *rbuff =(char *)malloc(DATA_COUNT);
	memset(wbuff, 0xCD, DATA_COUNT);
	memset(rbuff, 0x00, DATA_COUNT);
	
	fd = open("/tyCo/1", 0, 0);
	if(fd<=0)
		printf("open err\n");
	printf("...通路打开完成 fd=%d \n", fd);
	
	printf("...设置通信波特率[%d]\n", 115200);
	ioctl(fd, SIO_BAUD_SET, 115200);
	
	printf("...发送数据  \n");
	write(fd, wbuff, 100);
	taskDelay(10);
	
	printf("...接收数据 \n");
	read(fd, rbuff, 100);
	taskDelay(10);
	
	if(memcmp(rbuff, wbuff, 100))
	{
		printf("...读写数据不一致, 测试失败 \n");
		free(wbuff);
		free(rbuff);
		close(fd);
		return ERROR;
	}
	printf("...数据读写完成  \n");
	
	free(wbuff);
	free(rbuff);
	close(fd);
	return OK;
}
#endif

