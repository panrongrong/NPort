/*
demo pl rs422, tx_loop_rx in fpga
*/

#include <vxWorks.h>
#include <sysLib.h>
#include <string.h>
#include <intLib.h>
#include <taskLib.h>
#include <vxLib.h>
#include <muxLib.h>
#include <cacheLib.h>

#include "pl_rs422.h"

typedef	unsigned char  	u8,  __u8, unchar;
/*
define the Macro for enabel or diable function of the 422 chn 
*/
#if 1

/*
IN - 1: 
	rx - 15bytes, 38400bps, odd_chksum, 200ms
	tx - 12bytes, 38400bps, odd_chksum, 200ms
*/
#define RS422_CHN_1


/*
Di-Mian-Jian-Ce(鍦伴潰妫�祴) - 1: 
	rx - no
	tx - 35bytes, 115200bps, odd_chksum, 10ms
*/
#define RS422_CHN_2


/*
LPTU - 3: 
	rx - 29bytes, 38400bps, odd_chksum, 15ms
	tx - no
*/
#define RS422_CHN_3
/*
RPTU - 4: 
	rx - 29bytes, 38400bps, odd_chksum, 15ms
	tx - no
*/
#define RS422_CHN_4


/*
two kinds of functions:
---------------------
ji-li-qi(婵�姳鍣� - 5: 
	rx - 80bytes, 57600bps, odd_chksum, 25ms
	tx - no
or
---------------------
juo-cha(缃楀樊) -5
	rx - 28bytes, 57600bps, odd_chksum, 200ms
	tx - 28bytes, 57600bps, odd_chksum, 200ms
*/
#define RS422_CHN_5


#define RS422_CHN_6  /* no-use*/

/*
LMFD - 7: 
	rx -  6bytes, 38400bps, odd_chksum, event_type
	tx - 28bytes, 38400bps, odd_chksum, 40ms
*/
#define RS422_CHN_7
/*
RMFD - 8: 
	rx -  6bytes, 38400bps, odd_chksum, event_type
	tx - 28bytes, 38400bps, odd_chksum, 40ms
*/
#define RS422_CHN_8


/*
MMP1 - 9: 
	rx - 1 / 10 bytes, 38400bps, even_chksum, event_type
	tx - 1 / 14 bytes, 38400bps, even_chksum, event_type
*/
#define RS422_CHN_9
/*
MMP2 - 10: 
	rx - 1 / 10 bytes, 38400bps, even_chksum, event_type
	tx - 1 / 14 bytes, 38400bps, even_chksum, event_type
*/
#define RS422_CHN_10


/*
IDMP - 11: 
	rx - no
	tx - 77 bytes, 57600bps, odd_chksum, 20ms
*/
#define RS422_CHN_11
/*
IDMP2 - 12: 
	rx - no
	tx - 77 bytes, 57600bps, odd_chksum, 20ms
*/
#define RS422_CHN_12

/*
tuoluo-x,y,z: 13,14,15
//
header(1) + temperatur(1) + tuoluo(4) + status(1) + chksum(1) => 8 bytes
 B1 - x   --> chn_13
 C2 - y   --> chn_14
 D3 - z   --> chn_15
*/
#define RS422_CHN_13   /* tuoluo -  x*/
#define RS422_CHN_14   /* tuoluo -  y*/
#define RS422_CHN_15   /* tuoluo -  z*/


/*
GPS(鍗槦鏉� - 16: 
	rx -    50 bytes, 115200bps, odd_chksum, 100ms
	tx - 48/11 bytes, 115200bps, odd_chksum, 100ms
*/
#define RS232_CHN_16

/*
GPS(鍗槦鏉� - 17: beifen(澶囦唤) 
*/
#define RS232_CHN_17

#endif

#if 1

/***********************************
PL reg definition for RS422 or RS232
chn_1  ~ 3  - rs422_chip_1
chn_4  ~ 8  - rs422_chip_2
chn_9  ~ 12 - rs422_chip_3
chn_13 ~ 15 - rs422_chip_4

chn_16 ~ 17 - rs232
***********************************/
#define RS422_INIT_ENABLE_CHIP_1  (0x3b)  /* chip_1 => chn1,2,3,4*/
#define RS422_INIT_ENABLE_CHIP_2  (0x3c)  /* chip_2 => chn5,6,7,8*/
#define RS422_INIT_ENABLE_CHIP_3  (0x3d)  /* chip_3 => chn9,10,11,12*/
#define RS422_INIT_ENABLE_CHIP_4  (0x3e)  /* chip_4 => chn13,14,15,*/
/*----------------------------------------// rs232 --> chn16, 17 */

#define TX_BUF_x(chn)         (0x08 + 3*(chn-1))
#define TX_EN_x(chn)          (0x09 + 3*(chn-1))
#define RX_EN_x(chn)          (0x0a + 3*(chn-1))
#define RX_FLAG_x(chn)        (0x40 + 2*(chn-1))
#define RX_BUF_x(chn)         (0x41 + 2*(chn-1))


/* chn-6*/
#define PL_REG_TX_BUF_CHN_6    (TX_BUF_x(6))  /* 0x08 + 3*chn-1)*/
#define PL_REG_TX_EN_CHN_6     (TX_EN_x(6))      /* 0x09 + 3*chn-1)*/
#define PL_REG_RX_EN_CHN_6     (RX_EN_x(6))      /* 0x0a + 3*chn-1)*/

#define PL_REG_RX_FLAG_CHN_6   (RX_FLAG_x(6))      /* 0x40 + 2*chn-1)*/
#define PL_REG_RX_BUF_CHN_6    (RX_BUF_x(6))      /* 0x41 + 2*chn-1)*/

#define GP02PL_COMM_BASEADDR     0x40000000   /* pl gp_0 base_addr*/


/* func declar*/
void tuoluoInit(void);

void ps2pl_write(int pl_reg, unsigned int setValues)
{
	unsigned int *p;
	p = (unsigned int *)(GP02PL_COMM_BASEADDR + (pl_reg * 4));
	*p = setValues;
};

unsigned int ps2pl_read(int pl_reg)
{
	volatile unsigned int p;
	p = *((volatile unsigned int *)(GP02PL_COMM_BASEADDR + (pl_reg * 4)));
	return p;
};

void pl_uart_init(int chn)
{
	int baud = 0;
	
	/*baud = 50000000 / 115200 + 1; // (50m/115200)+1=435;*/
	/*ps2pl_write(0x05, 435);//serial_baudrate    (50m/115200)+1=435*/
	/*ps2pl_write(0x05, baud);//serial_baudrate    (50m/115200)+1=435*/
	
	/*ps2pl_write(0x06, 1);  //serial_stop_bit     2'd1:??|??|?2'd2:??|?¨￠?|?*/
	/*ps2pl_write(0x07, 0);  //serial_parity       2'b00:??§μ?;2'b01:?§μ?;2'b10:?§μ? */
	
	if(chn == 1) {
	  ps2pl_write(0x05,435);/*serial_baudrate    			(50m/115200)+1=435*/
	  ps2pl_write(0x06,1);  /*serial_stop_bit     			2'd1:停止位为一位;2'd2:停止位为两位*/
	  ps2pl_write(0x07,0);  /*serial_parity      			2'b00:没有校验;2'b01:奇校验;2'b10:偶校验*/
	}else {
	  ps2pl_write(0x40+3*(chn-2),435);/*serial_baudrate    (50m/115200)+1=435*/
	  ps2pl_write(0x41+3*(chn-2),1);  /*serial_stop_bit     2'd1:停止位为一位;2'd2:停止位为两位*/
	  ps2pl_write(0x42+3*(chn-2),0);  /*serial_parity       2'b00:没有校验;2'b01:奇校验;2'b10:偶校验*/
	} 

  	return;
}
void ps_write_uart_old(unsigned long offsetaddress,unsigned long offsetaddress1,unsigned long setValues)
{
  ps2pl_write(offsetaddress,setValues);
  ps2pl_write(offsetaddress1,1);
  ps2pl_write(offsetaddress1,0);
}

/*u8 ps_read_uart(unsigned long offsetaddress,unsigned long offsetaddress1) {*/
/*  u8 read_uart_data;*/
/*  ps2pl_write(offsetaddress,1);*/
/*  ps2pl_write(offsetaddress,0);*/
/*  read_uart_data=ps2pl_read(offsetaddress1);*/
/*  return read_uart_data;*/
/*}*/

void ps_write_uart(int pl_reg_rx, int pl_reg_rx_en, unsigned int setValues)
{
	ps2pl_write(pl_reg_rx, setValues);

	ps2pl_write(pl_reg_rx_en, 1);
	ps2pl_write(pl_reg_rx_en, 0);
	return;
}

/*unsigned int ps_read_uart(unsigned long cfgaddress,unsigned long rxaddress)*/
unsigned int ps_read_uart(int pl_reg_rx_en, int pl_reg_rx)
{
  unsigned int rx_data;
  
  ps2pl_write(pl_reg_rx_en,1);
  ps2pl_write(pl_reg_rx_en,0);
  
  rx_data = ps2pl_read(pl_reg_rx);
  return rx_data;
}

#endif



/*
PL rs422 channel - 13,14,15: tuoluo-x,y,z
				         ad: jiaji-1,2,3
*/
#define RS422_CHN_TUOLUO_JIAJI

#ifdef RS422_CHN_TUOLUO_JIAJI

/*
//
// chn-13: tuoluo-x
//
#define PL_REG_TX_BUF_CHN_13    (TX_BUF_x(13))     // 0x08 + 3*(chn-1)
#define PL_REG_TX_EN_CHN_13     (TX_EN_x(13))      // 0x09 + 3*(chn-1)
#define PL_REG_RX_EN_CHN_13     (RX_EN_x(13))      // 0x0a + 3*(chn-1)
#define PL_REG_RX_FLAG_CHN_13   (RX_FLAG_x(13))    // 0x40 + 2*(chn-1)
#define PL_REG_RX_BUF_CHN_13    (RX_BUF_x(13))      // 0x41 + 2*(chn-1)


//
// chn-14: tuoluo-y
//
#define PL_REG_TX_BUF_CHN_14    (TX_BUF_x(14))     // 0x08 + 3*(chn-1)
#define PL_REG_TX_EN_CHN_14     (TX_EN_x(14))      // 0x09 + 3*(chn-1)
#define PL_REG_RX_EN_CHN_14     (RX_EN_x(14))      // 0x0a + 3*(chn-1)
#define PL_REG_RX_FLAG_CHN_14   (RX_FLAG_x(14))    // 0x40 + 2*(chn-1)
#define PL_REG_RX_BUF_CHN_14    (RX_BUF_x(14))      // 0x41 + 2*(chn-1)

//
// chn-15: tuoluo-z
//
#define PL_REG_TX_BUF_CHN_15    (TX_BUF_x(15))     // 0x08 + 3*(chn-1)
#define PL_REG_TX_EN_CHN_15     (TX_EN_x(15))      // 0x09 + 3*(chn-1)
#define PL_REG_RX_EN_CHN_15     (RX_EN_x(15))      // 0x0a + 3*(chn-1)
#define PL_REG_RX_FLAG_CHN_15   (RX_FLAG_x(15))    // 0x40 + 2*(chn-1)
#define PL_REG_RX_BUF_CHN_15    (RX_BUF_x(15))      // 0x41 + 2*(chn-1)
*/

/**/
/* pl to ps irq for tuoluo and jiaji*/
/**/
#define PL2PS_IRQ_TUOLUO    (57)

/**/
/* jiaji-1,2,3*/
/**/
#define PL_REG_RX_BUF_JIAJI    (0xb0)

UINT32 gRx_buf_Jiaji = 0;

/* 
header(1) + temperatur(1) + tuoluo(4) + status(1) + chksum(1) => 8 bytes
 B1 - x   --> chn_13
 C2 - y   --> chn_14
 D3 - z   --> chn_15
*/
#define RX_SIZE_TUOLUO    (8)

UINT32 gRx_buf_TLx[RX_SIZE_TUOLUO] = {0};
UINT32 gRx_buf_TLy[RX_SIZE_TUOLUO] = {0};
UINT32 gRx_buf_TLz[RX_SIZE_TUOLUO] = {0};

UINT32 tuoluo_rx_bufx[1024] = {0};
UINT32 tuoluo_rx_bufy[1024] = {0};
UINT32 tuoluo_rx_bufz[1024] = {0};
UINT32 jiaji_rx_bufx[1024] = {0};
UINT32 g_irq_ps_cnt = 0;	
UINT32 g_irq_pl_cnt = 0;

SEM_ID gRcvSem_Tuoluo_Jiaji;

void irq_callbk_tuoluo_jiaji(void)
{	
	intDisable(PL2PS_IRQ_TUOLUO);		
	semGive(gRcvSem_Tuoluo_Jiaji);	
	intEnable(PL2PS_IRQ_TUOLUO);	
	
	/*logMsg("irq_Tuoluo_Jiaji-%d! \n", PL2PS_IRQ_TUOLUO, 2,3,4,5,6);	*/
	return;
}

void init_irq_tuoluo_jiaji(void)
{
	int irq_no = PL2PS_IRQ_TUOLUO;
	
	intConnect(INUM_TO_IVEC(irq_no), irq_callbk_tuoluo_jiaji, irq_no);
	intEnable(irq_no);	
	
	gRcvSem_Tuoluo_Jiaji = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
	return;
}

void taskfun_tuoluo_jiaji(void)
{
	int i = 0,j = 0,k = 0,l = 0,m = 0;
	UINT32 data = 0;
	
	while (1)
	{
		semTake(gRcvSem_Tuoluo_Jiaji, WAIT_FOREVER);
		
		/*taskDelay(10);*/
		/*for (i=0; i<RX_SIZE_TUOLUO; i++)*/
		{
/*			gRx_buf_TLx[i] = ps2pl_read(PL_REG_RX_FLAG_CHN_13);									//0x4a empty flag*/
/*			gRx_buf_TLy[i] = ps2pl_read(PL_REG_RX_FLAG_CHN_14); 								//0x4a empty flag*/
/*			gRx_buf_TLz[i] = ps2pl_read(PL_REG_RX_FLAG_CHN_15); 								//0x4a empty flag*/
			tuoluo_rx_bufx[j++%1024] = ps2pl_read(0x30);
			tuoluo_rx_bufx[j++%1024] = ps2pl_read(0x31);
			
			tuoluo_rx_bufy[k++%1024] = ps2pl_read(0x32);
			tuoluo_rx_bufy[k++%1024] = ps2pl_read(0x33);
			
			tuoluo_rx_bufz[l++%1024] = ps2pl_read(0x34);
			tuoluo_rx_bufz[l++%1024] = ps2pl_read(0x35);

			/* pl cnt: read*/
			g_irq_pl_cnt = ps2pl_read(0x36);

			/* ps cnt: add*/
			g_irq_ps_cnt++;			
		}
		
		jiaji_rx_bufx[m++%1024] = ps2pl_read(PL_REG_RX_BUF_JIAJI);
		
		/* for log //*/
	#if 0
		for (i=0; i<RX_SIZE_TUOLUO; i++)
		{
			printf("0x%08X \n", gRx_buf_TLx[i]);
		}
		printf("\n");
		
		for (i=0; i<RX_SIZE_TUOLUO; i++)
		{
			printf("0x%08X \n", gRx_buf_TLy[i]);
		}
		printf("\n");
		
		for (i=0; i<RX_SIZE_TUOLUO; i++)
		{
			printf("0x%08X \n", gRx_buf_TLz[i]);
		}
		printf("\n");
	#endif

#if 0	/*uart6 rx*/
		while(1) {
			data=ps2pl_read(0x4a);									/*0x4a empty flag*/
			if(data==1) {
			  printf("uart6 is empty\r\n");
			  break;
			}else {
			  /*uart_data=ps_read_uart(0x19,0x4b);					//0x19  rd tog addr ;0x4b rd addr*/
			  printf("uart6 data =0x%x\r\n",ps_read_uart(0x19,0x4b)); 
			}
		}
		taskDelay(30);
		printf("\n");
#endif
	}
}


void start_tuoluo_jiaji(void)
{
	init_irq_tuoluo_jiaji();
	tuoluoInit();

	taskSpawn ("tsk_tuoluo", 105, 0, 20000, (FUNCPTR)taskfun_tuoluo_jiaji, 0,0,0,0,0,0,0,0,0,0);
	return;
}


/*unsigned int app_read_tuoluo_xyz(unsigned int *pDstBuf)*/
/*{*/
/*//	if (irq_en)*/
/*	semTake(gRcvSem_Tuoluo_Jiaji, WAIT_FOREVER);*/
/*	gRx_buf_TLx[i] = ps2pl_read(PL_REG_RX_FLAG_CHN_13);		*/
/*}*/

void run_init(void)
{
  /*shang dian fu wei*/
  ps2pl_write(0x00,1);
  ps2pl_write(0x00,0); 
  
  /*13 14 15 dakai xinpian*/
  ps2pl_write(0x3b,0x00);
  ps2pl_write(0x3c,0x00);
  ps2pl_write(0x3d,0x00);
  ps2pl_write(0x3e,0x00);
  /*delay_ms(1);*/
  taskDelay(1);
  /*qidong zhongduan 1:on 0:off*/
  ps2pl_write(0x73,0x01);  
}


void tuoluoInit(void) {
	/*pl_uart_init(13);*/
	/*pl_uart_init(14);*/
	/*pl_uart_init(15);*/
	/*13	ou jiaoyan*/
	ps2pl_write(0x70,0x0b1); 
	/*14*/
	ps2pl_write(0x71,0x1c2); 
	/*15*/
	ps2pl_write(0x72,0x1d3);  
	
	run_init();
}
void showXyzBuf(void) 
{
	int i = 0;
	int size = 64;
	
	printf("tuoluo x ----bufAddr 0x%08x\n",tuoluo_rx_bufx);
	for(i = 0;i < size;i++)
	{
		printf("%08x ",tuoluo_rx_bufx[i]);
		if(((i+1)%16) == 0) {
			printf("\n");
		}
	}
	printf("tuoluo y ----bufAddr 0x%08x\n",tuoluo_rx_bufy);
	for(i = 0;i < size;i++)
	{
		printf("%08x ",tuoluo_rx_bufy[i]);
		if(((i+1)%16) == 0) {
			printf("\n");
		}
	}
	printf("tuoluo z ----bufAddr 0x%08x\n",tuoluo_rx_bufz);
	for(i = 0;i < size;i++)
	{
		printf("%08x ",tuoluo_rx_bufz[i]);
		if(((i+1)%16) == 0) {
			printf("\n");
		}
	}
	printf("jiaji   ----bufAddr 0x%08x\n",jiaji_rx_bufx);
	for(i = 0;i < size;i++)
	{
		printf("%08x ",jiaji_rx_bufx[i]);
		if(((i+1)%16) == 0) {
			printf("\n");
		}
	}

	printf("irq_ack_cnt: (pl-ps) 0x%08X - 0x%08X  \n\n", g_irq_pl_cnt, g_irq_ps_cnt);
	
	return;
}


#if 1
/*
usr-app interface_functions:
*/
static int gTuoluo_Rx_status = 0;
int UartRecvData(int chn, unsigned char * dataBuf, unsigned int dataLen)
{
	int i = 0;
	unsigned int* pBuf32 = (unsigned int*)dataBuf;

	switch (chn)
	{
	/* tuoluo*/
	case 13:
	case 14:
	case 15:
		/**/
		/* wait semphore forever*/
		/**/
		switch (gTuoluo_Rx_status)
		{
		case 0:
			semTake(gRcvSem_Tuoluo_Jiaji, WAIT_FOREVER);
			gTuoluo_Rx_status = 0xF0;  
			break;
		default:
			gTuoluo_Rx_status += 1;  /* 0xF1, 0xF2*/
			if (gTuoluo_Rx_status >= 0xF2)
			{
				gTuoluo_Rx_status = 0;
			}
			break;
		}
		/**/
		/* read rx_data for one of chn-13,14,15*/
		/**/
		switch (chn)
		{
		case 13: /* tuoluo-x*/
			pBuf32[0] = ps2pl_read(0x30);
			pBuf32[1] = ps2pl_read(0x31);
			pBuf32[2] = ps2pl_read(0x32);
			break;
		case 14: /* tuoluo-y*/
			pBuf32[0] = ps2pl_read(0x33);
			pBuf32[1] = ps2pl_read(0x34);
			pBuf32[2] = ps2pl_read(0x35);
			break;
		case 15: /* tuoluo-z*/
			pBuf32[0] = ps2pl_read(0x36);
			pBuf32[1] = ps2pl_read(0x37);
			pBuf32[2] = ps2pl_read(0x38);
			break;
		}
		break;
	/*
	all other channel:chn_1~12,16~17
	*/
	default:
		for (i=0; i<(dataLen/4); i++)
		{
			pBuf32[i] = ps2pl_read(RX_BUF_x(chn));
		}
		break;	
	}
}

#endif 

#endif  /* #ifdef RS422_CHN_TUOLUO_JIAJI */


#if 0
#define printf_dbg  printf
#else
#define printf_dbg(...)
#endif

#define PL_RS422_IRQ_POLL_MODE

/*
irq&poll mode: chn1~12, 16~17
*/
#ifdef PL_RS422_IRQ_POLL_MODE

#if 1

void pl_uart_init_2(void)
{
	/*uart1 */
	ps2pl_write(0x05,1303);/*serial_baudrate    (50m/115200)+1=435*/
	ps2pl_write(0x06,1);  /*serial_stop_bit     2'd1:??位??位;2'd2:??位?谩?位*/
	ps2pl_write(0x07,0);  /*serial_parity       2'b00:??校?;2'b01:?校?;2'b10:?校? */

	/*2*/
	ps2pl_write(0x40,435);/*serial_baudrate    (50m/115200)+1=435*/
	ps2pl_write(0x41,1);  /*serial_stop_bit     2'd1:??位??位;2'd2:??位?谩?位*/
	ps2pl_write(0x42,0);  /*serial_parity       2'b00:??校?;2'b01:?校?;2'b10:?校? */

	/*3*/
	ps2pl_write(0x43,1303);/*serial_baudrate    (50m/115200)+1=435*/
	ps2pl_write(0x44,1);  /*serial_stop_bit     2'd1:??位??位;2'd2:??位?谩?位*/
	ps2pl_write(0x45,0);  /*serial_parity       2'b00:??校?;2'b01:?校?;2'b10:?校? */

	/*4*/
	ps2pl_write(0x46,1303);/*serial_baudrate    (50m/115200)+1=435*/
	ps2pl_write(0x47,1);  /*serial_stop_bit     2'd1:??位??位;2'd2:??位?谩?位*/
	ps2pl_write(0x48,0);  /*serial_parity       2'b00:??校?;2'b01:?校?;2'b10:?校? */

	/*5*/
	ps2pl_write(0x49,435);/*serial_baudrate    (50m/115200)+1=435*/
	ps2pl_write(0x4a,1);  /*serial_stop_bit     2'd1:??位??位;2'd2:??位?谩?位*/
	ps2pl_write(0x4b,0);  /*serial_parity       2'b00:??校?;2'b01:?校?;2'b10:?校? */

	/*6*/
	ps2pl_write(0x4c,435);/*serial_baudrate    (50m/115200)+1=435*/
	ps2pl_write(0x4d,1);  /*serial_stop_bit     2'd1:??位??位;2'd2:??位?谩?位*/
	ps2pl_write(0x4e,0);  /*serial_parity       2'b00:??校?;2'b01:?校?;2'b10:?校? */

	/*7*/
	ps2pl_write(0x4f,1303);/*serial_baudrate    (50m/115200)+1=435*/
	ps2pl_write(0x50,1);  /*serial_stop_bit     2'd1:??位??位;2'd2:??位?谩?位*/
	ps2pl_write(0x51,0);  /*serial_parity       2'b00:??校?;2'b01:?校?;2'b10:?校? */

	/*8*/
	ps2pl_write(0x52,1303);/*serial_baudrate    (50m/115200)+1=435*/
	ps2pl_write(0x53,1);  /*serial_stop_bit     2'd1:??位??位;2'd2:??位?谩?位*/
	ps2pl_write(0x54,0);  /*serial_parity       2'b00:??校?;2'b01:?校?;2'b10:?校? */

	/*9*/
	ps2pl_write(0x55,1303);/*serial_baudrate    (50m/115200)+1=435*/
	ps2pl_write(0x56,1);  /*serial_stop_bit     2'd1:??位??位;2'd2:??位?谩?位*/
	ps2pl_write(0x57,0);  /*serial_parity       2'b00:??校?;2'b01:?校?;2'b10:?校? */

	/*10*/
	ps2pl_write(0x58,1303);/*serial_baudrate    (50m/115200)+1=435*/
	ps2pl_write(0x59,1);  /*serial_stop_bit     2'd1:??位??位;2'd2:??位?谩?位*/
	ps2pl_write(0x5a,0);  /*serial_parity       2'b00:??校?;2'b01:?校?;2'b10:?校? */

	/*11  */
	ps2pl_write(0x5b,869);/*serial_baudrate    (50m/115200)+1=435*/
	ps2pl_write(0x5c,1);  /*serial_stop_bit     2'd1:??位??位;2'd2:??位?谩?位*/
	ps2pl_write(0x5d,0);  /*serial_parity       2'b00:??校?;2'b01:?校?;2'b10:?校? */

	/*12*/
	ps2pl_write(0x5e,435);/*serial_baudrate    (50m/115200)+1=435*/
	ps2pl_write(0x5f,1);  /*serial_stop_bit     2'd1:??位??位;2'd2:??位?谩?位*/
	ps2pl_write(0x60,0);  /*serial_parity       2'b00:??校?;2'b01:?校?;2'b10:?校? */

	/*16*/
	ps2pl_write(0x6a,2605);/*serial_baudrate    (50m/115200)+1=435*/
	ps2pl_write(0x6b,1);  /*serial_stop_bit     2'd1:??位??位;2'd2:??位?谩?位*/
	ps2pl_write(0x6c,0);  /*serial_parity       2'b00:??校?;2'b01:?校?;2'b10:?校? */

	/*17*/
	ps2pl_write(0x6d,2605);/*serial_baudrate    (50m/115200)+1=435*/
	ps2pl_write(0x6e,1);  /*serial_stop_bit     2'd1:??位??位;2'd2:??位?谩?位*/
	ps2pl_write(0x6f,0);  /*serial_parity       2'b00:??校?;2'b01:?校?;2'b10:?校? */

	return;
}

void tuoluo_init(void)     /*ou jiao yan*/
{
	/*13: tuoluo-x*/
	ps2pl_write(0x70,0x0b1); 

	/*14: tuoluo-y*/
	ps2pl_write(0x71,0x1c2); 

	/*15: tuoluo-z*/
	ps2pl_write(0x72,0x1d3);  

	return;
}

void qidong_init(void)
{
	/*shang dian fu wei*/
	ps2pl_write(0x00,1);
	ps2pl_write(0x00,0); 

	/* rs422-phy_chip reset-1,2,3,4*/
	ps2pl_write(0x3b,0x00);
	ps2pl_write(0x3c,0x00);
	ps2pl_write(0x3d,0x00);
	ps2pl_write(0x3e,0x00);

	/*delay_ms(1);*/
	taskDelay(10);
	ps2pl_write(0x73,0x01);  

	return;
}

u8 UartTXBuffer[256];
u8 UartRXBuffer[256];

u8 gRxData[18][128] = {0};

#define UART_SEND_NUM  64

void pl_init_all_chn(void)
{
	/*pl chuan kou chu shi hua*/
	pl_uart_init_2();
	tuoluo_init();
	qidong_init();

	int uart_cnt = 0;
	
	/**/
	for (uart_cnt = 0; uart_cnt < UART_SEND_NUM; uart_cnt++)
	{
		UartRXBuffer[uart_cnt] = uart_cnt;
	}
/*	
	ps2pl_write(0x3b,0x00);
	ps2pl_write(0x3c,0x00);
	ps2pl_write(0x3d,0x00);
	ps2pl_write(0x3e,0x00);	
	delay_ms(1000);
*/
	return;
}


SEM_ID gRcvSem_Other_Chn;

#define PL2PS_IRQ_OTHER_CHN   (58)

void irq_callbk_other_chn(void)
{	
	intDisable(PL2PS_IRQ_OTHER_CHN);		
	semGive(gRcvSem_Other_Chn);	
	intEnable(PL2PS_IRQ_OTHER_CHN);	
	
	/*logMsg("irq_other_chn-%d! \n", PL2PS_IRQ_OTHER_CHN, 2,3,4,5,6);	*/
	return;
}

void init_irq_other_chn(void)
{
	int irq_no = PL2PS_IRQ_OTHER_CHN;
	
	intConnect(INUM_TO_IVEC(irq_no), irq_callbk_other_chn, irq_no);
	intEnable(irq_no);	
	
	gRcvSem_Other_Chn = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
	return;
}

void taskfun_other_chn(void)
{
	int i = 0,j = 0,k = 0,l = 0,m = 0;
	UINT32 irq_flag = 0, data_flag = 0, rx_data = 0;
	int chn = 0;
	
	while (1)
	{
		semTake(gRcvSem_Other_Chn, WAIT_FOREVER);

		/**/
		/* pl2ps irq flag: chn1~12,16~17, irq_No: 58*/
		/*               : chn13~15,      irq_No: 57, tuoluo-x,y,z*/
		/**/
		irq_flag = ps2pl_read(0x37);
		/*printf_dbg("irq_flag=0x%08X \r\n", irq_flag);*/
		
		/* rs422_1*/
		if ((irq_flag >> 0) & 0x01)
		{ 
			i = 0;
			while (1)
			{
				data_flag = ps2pl_read(0x40);
				if (data_flag == 1)
				{
					printf_dbg("uart1 is empty \r\n");
					break;
				}
				else
				{
					rx_data = ps_read_uart(0x0a, 0x41);
					printf_dbg("uart1 data = 0x%X \r\n", rx_data); 
					
					gRxData[chn][i++] = rx_data;					
				}
			}
		} 
		
		/* rs422_2*/
		if ((irq_flag >> 1) & 0x01) 
		{
			i = 0;
			while (1)
			{
				data_flag = ps2pl_read(0x42);
				if (data_flag == 1)
				{
					printf_dbg("uart1 is empty \r\n");
					break;
				}
				else
				{
					rx_data = ps_read_uart(0x0D, 0x43);
					printf_dbg("uart2 data = 0x%X \r\n", rx_data); 

					gRxData[chn][i++] = rx_data;
				}
			}
		} 
		
		/* rs422_3*/
		if ((irq_flag >> 2) & 0x01) 
		{
			i = 0;
			while (1)
			{
				data_flag = ps2pl_read(0x44);
				if (data_flag == 1)
				{
					printf_dbg("uart1 is empty \r\n");
					break;
				}
				else
				{
					rx_data = ps_read_uart(0x010, 0x45);
					printf_dbg("uart3 data = 0x%X \r\n", rx_data); 

					gRxData[chn][i++] = rx_data;
				}
			}
		} 
		
		/* rs422_4*/
		if ((irq_flag >> 3) & 0x01) 
		{
			i = 0;
			while (1)
			{
				data_flag = ps2pl_read(0x46);
				if (data_flag == 1)
				{
					printf_dbg("uart1 is empty \r\n");
					break;
				}
				else
				{
					rx_data = ps_read_uart(0x13, 0x47);
					printf_dbg("uart4 data = 0x%X \r\n", rx_data); 

					gRxData[chn][i++] = rx_data;
				}
			}
		} 
		
		/* rs422_5*/
		if ((irq_flag >> 4) & 0x01)
		{ 
			i = 0;
			while (1)
			{
				data_flag = ps2pl_read(0x48);
				if (data_flag == 1)
				{
					printf_dbg("uart1 is empty \r\n");
					break;
				}
				else
				{
					rx_data = ps_read_uart(0x16, 0x49);
					printf_dbg("uart5 data = 0x%X \r\n", rx_data); 

					gRxData[chn][i++] = rx_data;
				}
			}
		} 
		
		/* rs422_6*/
		if ((irq_flag >> 5) & 0x01) 
		{ 
			i = 0;
			while (1)
			{
				data_flag = ps2pl_read(0x4a);
				if (data_flag == 1)
				{
					printf_dbg("uart1 is empty \r\n");
					break;
				}
				else
				{
					rx_data = ps_read_uart(0x19, 0x4b);
					printf_dbg("uart6 data = 0x%X \r\n", rx_data); 
					
					gRxData[chn][i++] = rx_data;
				}
			}
		} 
		
		/* rs422_7*/
		if ((irq_flag >> 6) & 0x01) 
		{ 
			i = 0;
			while (1)
			{
				data_flag = ps2pl_read(0x4c);
				if (data_flag == 1)
				{
					printf_dbg("uart7 is empty \r\n");
					break;
				}
				else
				{
					rx_data = ps_read_uart(0x1c, 0x4d);
					printf_dbg("uart7 data = 0x%X \r\n", rx_data); 
					
					gRxData[chn][i++] = rx_data;
				}
			}
		} 
		
		/* rs422_8*/
		if ((irq_flag >> 7) & 0x01)
		{
			i = 0;
			while (1)
			{
				data_flag = ps2pl_read(0x4e);
				if (data_flag == 1)
				{
					printf_dbg("uart8 is empty \r\n");
					break;
				}
				else
				{
					rx_data = ps_read_uart(0x1f, 0x4f);
					printf_dbg("uart8 data = 0x%X \r\n", rx_data); 
					
					gRxData[chn][i++] = rx_data;
				}
			}
		} 
		
		/* rs422_9*/
		if ((irq_flag >> 8) & 0x01)
		{ 
			i = 0;
			while (1)
			{
				data_flag = ps2pl_read(0x50);
				if (data_flag == 1)
				{
					printf_dbg("uart9 is empty \r\n");
					break;
				}
				else
				{
					rx_data = ps_read_uart(0x22, 0x51);
					printf_dbg("uart9 data = 0x%X \r\n", rx_data); 
					
					gRxData[chn][i++] = rx_data;
				}
			}
		} 
		
		/* rs422_10*/
		if ((irq_flag >> 9) & 0x01) 
		{
			i = 0;
			while (1)
			{
				data_flag = ps2pl_read(0x52);
				if (data_flag == 1)
				{
					printf_dbg("uart10 is empty \r\n");
					break;
				}
				else
				{
					rx_data = ps_read_uart(0x25, 0x53);
					printf_dbg("uart10 data = 0x%X \r\n", rx_data); 
					
					gRxData[chn][i++] = rx_data;
				}
			}
		} 
		
		/* rs422_11*/
		if ((irq_flag >>10) & 0x01) 
		{ 
			i = 0;
			while (1)
			{
				data_flag = ps2pl_read(0x54);
				if (data_flag == 1)
				{
					printf_dbg("uart11 is empty \r\n");
					break;
				}
				else
				{
					rx_data = ps_read_uart(0x28, 0x55);
					printf_dbg("uart11 data = 0x%X \r\n", rx_data); 
					
					gRxData[chn][i++] = rx_data;
				}
			}
		} 
		
		/* rs422_12*/
		if ((irq_flag >>11) & 0x01)
		{
			i = 0;
			while (1)
			{
				data_flag = ps2pl_read(0x56);
				if (data_flag == 1)
				{
					printf_dbg("uart12 is empty \r\n");
					break;
				}
				else
				{
					rx_data = ps_read_uart(0x2b, 0x57);
					printf_dbg("uart12 data = 0x%X \r\n", rx_data); 
					
					gRxData[chn][i++] = rx_data;
				}
			}
		} 

		/* tuoluo-x,y,z*/
		/* rs422_13*/
		/* rs422_14*/
		/* rs422_15*/
		
		/* rs232_16*/
		if ((irq_flag >>15) & 0x01) 
		{ 
			i = 0;
			while (1)
			{
				data_flag = ps2pl_read(0x5e);
				if (data_flag == 1)
				{
					printf_dbg("uart16 is empty \r\n");
					break;
				}
				else
				{
					rx_data = ps_read_uart(0x37, 0x5f);
					printf_dbg("uart16 data = 0x%X \r\n", rx_data); 
					
					gRxData[chn][i++] = rx_data;
				}
			}
		} 
		
		/* rs232_17*/
		if ((irq_flag >>16) & 0x01) 
		{ 
			i = 0;
			while (1)
			{
				data_flag = ps2pl_read(0x60);
				if (data_flag == 1)
				{
					printf_dbg("uart17 is empty \r\n");
					break;
				}
				else
				{
					rx_data = ps_read_uart(0x3a, 0x61);
					printf_dbg("uart17 data = 0x%X \r\n", rx_data); 
					
					gRxData[chn][i++] = rx_data;
				}
			}
		} 		
	}
}


void start_other_chn(void)
{
	pl_init_all_chn();
	init_irq_other_chn();

	taskSpawn ("tsk_othr", 105, 0, 20000, (FUNCPTR)taskfun_other_chn, 0,0,0,0,0,0,0,0,0,0);
	return;

}

void start_other(void)
{
	start_other_chn();
}

void test_snd_chn_x(int chn)
{
	int uart_cnt = 0;
	
	switch (chn)
	{
		/*uart1 tx*/
	case 1:
		for(uart_cnt = 0; uart_cnt < UART_SEND_NUM; uart_cnt++)
		{
		  ps_write_uart(0x08,0x09,UartRXBuffer[uart_cnt]);
		}
		break;

		/*uart2 tx*/
	case 2:
		for(uart_cnt = 0; uart_cnt < UART_SEND_NUM; uart_cnt++)
		{
		  ps_write_uart(0x0b,0x0c,UartRXBuffer[uart_cnt]);

		}
		break;

	/*uart3 tx*/
	case 3:
		for(uart_cnt = 0; uart_cnt < UART_SEND_NUM; uart_cnt++)
		{
		  ps_write_uart(0x0e,0x0f,UartRXBuffer[uart_cnt]);

		} 
		break;

	/*uart4 tx*/
	case 4:
		for(uart_cnt = 0; uart_cnt < UART_SEND_NUM; uart_cnt++)
		{
		  ps_write_uart(0x11,0x12,UartRXBuffer[uart_cnt]);
		}
		break;

	/*uart5 tx*/
	case 5:
		for(uart_cnt = 0; uart_cnt < UART_SEND_NUM; uart_cnt++)
		{
		  ps_write_uart(0x14,0x15,UartRXBuffer[uart_cnt]);

		}
		break;

	/*uart6 tx*/
	case 6:
		for(uart_cnt = 0; uart_cnt < UART_SEND_NUM; uart_cnt++)
		{
		  ps_write_uart(0x17,0x18,UartRXBuffer[uart_cnt]);

		}
		break;

	/*uart7 tx*/
	case 7:
		for(uart_cnt = 0; uart_cnt < UART_SEND_NUM; uart_cnt++)
		{
		  ps_write_uart(0x1a,0x1b,UartRXBuffer[uart_cnt]);

		}
		break;

	/*uart8 tx*/
	case 8:
		for(uart_cnt = 0; uart_cnt < UART_SEND_NUM; uart_cnt++)
		{
		  ps_write_uart(0x1d,0x1e,UartRXBuffer[uart_cnt]);
		}
		break;

	/*uart9 tx*/
	case 9:
		for(uart_cnt = 0; uart_cnt < UART_SEND_NUM; uart_cnt++)
		{
		  ps_write_uart(0x20,0x21,UartRXBuffer[uart_cnt]);
		}
		break;
		
	/*uart10 tx*/
	case 10:
		for(uart_cnt = 0; uart_cnt < UART_SEND_NUM; uart_cnt++)
		{
		  ps_write_uart(0x23,0x24,UartRXBuffer[uart_cnt]);
		}
		break;

	/*uart11 tx*/
	case 11:
		for(uart_cnt = 0; uart_cnt < UART_SEND_NUM; uart_cnt++)
		{
		  ps_write_uart(0x26,0x27,UartRXBuffer[uart_cnt]);
		}
		break;

	/*uart12 tx*/
	case 12:
		for(uart_cnt = 0; uart_cnt < UART_SEND_NUM; uart_cnt++)
		{
		  ps_write_uart(0x29,0x2a,UartRXBuffer[uart_cnt]);
		}
		break;

		/*tuoluo-x,y,z*/
		/*uart13 tx*/
		/*uart14 tx*/
		/*uart15 tx*/
	case 13:
	case 14:
	case 15:
		break;

		/*uart116 tx*/
	case 16:
		for(uart_cnt = 0; uart_cnt < UART_SEND_NUM; uart_cnt++)
		{
		  ps_write_uart(0x35,0x36,UartRXBuffer[uart_cnt]);
		}
		break;

		/*uart17 tx*/
	case 17:
		for(uart_cnt = 0; uart_cnt < UART_SEND_NUM; uart_cnt++)
		{
		  ps_write_uart(0x38,0x39,UartRXBuffer[uart_cnt]);
		}
		break;
	}

	switch (chn)
	{
	case 1:
		ps2pl_write(0x74,0x10);/*1 */
		break;
	case 2:
		ps2pl_write(0x75,0x10);/*2 */
		break;
	case 3:
		ps2pl_write(0x76,0x10);/*3 */
		break;
	case 4:
		ps2pl_write(0x77,0x10);/*4 */
		break;
	case 5:
		ps2pl_write(0x78,0x10);/*5 */
		break;
	case 6:
		ps2pl_write(0x79,0x10);/*6    */
		break;
	case 7:
		ps2pl_write(0x7a,0x10);/*7 */
		break;
	case 8:
		ps2pl_write(0x7b,0x10);/*8 */
		break;
	case 9:
		ps2pl_write(0x7c,0x10);/*9 */
		break;
	case 10:
		ps2pl_write(0x7d,0x10);/*10 */
		break;
	case 11:
		ps2pl_write(0x7e,0x10);/*11 */
		break;
	case 12:
		ps2pl_write(0x7f,0x10);/*12 */
		break;
	
	/*tuoluo-x,y,z*/
	/*uart13 tx*/
	/*uart14 tx*/
	/*uart15 tx*/
	case 13:
	case 14:
	case 15:
		break;
	
	case 16:
		ps2pl_write(0x80,0x10);/*16 */
		break;
	case 17:
		ps2pl_write(0x81,0x10);/*17  */
		break;
	}

	return;
}

void test_show_rx_data(int chn)
{
	int j = 0;
	
	for (j=0; j<128; j++)
	{
		printf("%02X ", gRxData[chn][j]);

		if (((j + 1) % 16) == 0)
		{
			printf("\n");
		}
	}
	printf("\n");

	return;
}

#endif




void read_pl_rs422_chn(void)
{
	/*
	IN - 1: 
		rx - 15bytes, 38400bps, odd_chksum, 200ms
		tx - 12bytes, 38400bps, odd_chksum, 200ms
	*/
#ifdef RS422_CHN_1
	
#endif

	/*
	Di-Mian-Jian-Ce(鍦伴潰妫�祴) - 1: 
		rx - no
		tx - 35bytes, 115200bps, odd_chksum, 10ms
	*/
#ifdef RS422_CHN_2
#endif
	
	
	/*
	LPTU - 3: 
		rx - 29bytes, 38400bps, odd_chksum, 15ms
		tx - no
	*/
#ifdef RS422_CHN_3
#endif
	/*
	RPTU - 4: 
		rx - 29bytes, 38400bps, odd_chksum, 15ms
		tx - no
	*/
#ifdef RS422_CHN_4
#endif
	
	
	/*
	two kinds of functions:
	---------------------
	ji-li-qi(婵�姳鍣� - 5: 
		rx - 80bytes, 57600bps, odd_chksum, 25ms
		tx - no
	or
	---------------------
	juo-cha(缃楀樊) -5
		rx - 28bytes, 57600bps, odd_chksum, 200ms
		tx - 28bytes, 57600bps, odd_chksum, 200ms
	*/
#ifdef RS422_CHN_5
#endif
	
	
#ifdef RS422_CHN_6  /* no-use*/
#endif
	
	/*
	LMFD - 7: 
		rx -  6bytes, 38400bps, odd_chksum, event_type
		tx - 28bytes, 38400bps, odd_chksum, 40ms
	*/
#ifdef RS422_CHN_7
#endif
	/*
	RMFD - 8: 
		rx -  6bytes, 38400bps, odd_chksum, event_type
		tx - 28bytes, 38400bps, odd_chksum, 40ms
	*/
#ifdef RS422_CHN_8
#endif
	
	
	/*
	MMP1 - 9: 
		rx - 1 / 10 bytes, 38400bps, even_chksum, event_type
		tx - 1 / 14 bytes, 38400bps, even_chksum, event_type
	*/
#ifdef RS422_CHN_9
#endif
	/*
	MMP2 - 10: 
		rx - 1 / 10 bytes, 38400bps, even_chksum, event_type
		tx - 1 / 14 bytes, 38400bps, even_chksum, event_type
	*/
#ifdef RS422_CHN_10
#endif
	
	
	/*
	IDMP1 - 11: 
		rx - no
		tx - 77 bytes, 57600bps, odd_chksum, 20ms
	*/
#ifdef RS422_CHN_11
#endif


	/*
	IDMP2 - 12: 
		rx - no
		tx - 77 bytes, 57600bps, odd_chksum, 20ms
	*/
#ifdef RS422_CHN_12
#endif
	
	/*
	tuoluo-x,y,z: 13,14,15
	//
	header(1) + temperatur(1) + tuoluo(4) + status(1) + chksum(1) => 8 bytes
	 B1 - x   --> chn_13
	 C2 - y   --> chn_14
	 D3 - z   --> chn_15
	*/
#if defined(RS422_CHN_13) && defined(RS422_CHN_14) && defined(RS422_CHN_15) /* tuoluo -  x*/
#endif
	
	
	/*
	GPS(鍗槦鏉� - 16: 
		rx -	50 bytes, 115200bps, odd_chksum, 100ms
		tx - 48/11 bytes, 115200bps, odd_chksum, 100ms
	*/
#ifdef RS232_CHN_16
#endif
	
	/*
	GPS(鍗槦鏉� - 17: beifen(澶囦唤) 
	*/
#ifdef RS232_CHN_17
#endif

}

#endif

