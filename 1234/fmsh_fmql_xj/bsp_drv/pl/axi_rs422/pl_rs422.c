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


extern void vx_FGicPs_Enable(UINT32 Int_Id);

SEM_ID g_rs422RcvSem[RS422_NUM];


#if 1
/*********************gyro******************************
**************gyro	FUNCTION**************************
*********************gyro******************************/

void pl_IRQ_Callback_GYRO( int irqNo)
{	
	unsigned int dat;
	int i;	
	
	intDisable(irqNo);		
	
	for (i=0; i<SYN_FRM_LEN_GYRO; i++)
	{	
		g_Databuf_GYRO[i] = *(unsigned int *)UART_GYRO_READ_DATA_REG;
	}
	
	intEnable(irqNo);
	semGive(g_rs422RcvSem[UART_CHN_NUM_GYRO]);
	
	logMsg("GYRO!\n",1,2,3,4,5,6);
	return;
}

void pl_Irq_Init_GYRO(void)
{
	intConnect(INUM_TO_IVEC(SYN_IRQ_GYRO), pl_IRQ_Callback_GYRO, SYN_IRQ_GYRO);
	intEnable(SYN_IRQ_GYRO);	
	
	g_rs422RcvSem[UART_CHN_NUM_GYRO] = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
	return;
}

int ps_WaitRead_GYRO(unsigned char * uart_buf)
{
	if (*(UINT32 *)0x40000014 != 0x11)
	{
		*(UINT32 *)0x40000014 = 0x11;
	}
	
	semTake(g_rs422RcvSem[UART_CHN_NUM_GYRO], WAIT_FOREVER);
	memcpy(uart_buf, g_Databuf_GYRO, SYN_FRM_LEN_GYRO);
	
	return SYN_FRM_LEN_GYRO;
}

void task_psRead_GYRO(void)
{
	unsigned char *uartbuffer = (unsigned char *)0x20000000;
	unsigned int cnt = 0;
	
	while (1)
	{
		ps_WaitRead_GYRO(uartbuffer);
		
		uartbuffer += 0x10;
		
		cnt++;
		if (cnt > 0x1000)
		{
			cnt = 0;
			printf("gyro:0x1000 times\n");
			uartbuffer = (unsigned char *)0x20000000;
		}
	}
}

void test_pl_422_1(void)
{	
	pl_Irq_Init_GYRO();
	taskSpawn ("testUart_Gyro",105, 0, 20000,(FUNCPTR)task_psRead_GYRO,0,0,0,0,0,0,0,0,0,0);
	return;
}

#endif


#if 1

/*********************DLL******************************
**************DLL	FUNCTION**************************
*********************DLL******************************/

void pl_IRQ_Callback_DLL( int irqNo)
{	
	unsigned int dat;
	int i;	
	
	intDisable(irqNo);		
	
	/*
	for (i=0; i<SYN_FRM_LEN_DLL; i++)
	{	
		g_Databuf_DLL[i] = *(unsigned int *)UART_DLL_READ_DATA_REG;
	}
	*/
	logMsg("DLL!\n", 1,2,3,4,5,6);
	
	intEnable(irqNo);	
	/*semGive(g_rs422RcvSem[UART_CHN_NUM_DLL]);*/
	
	return;
}

void pl_Irq_Init_DLL(void)
{
	intConnect(INUM_TO_IVEC(SYN_IRQ_DLL), pl_IRQ_Callback_DLL, SYN_IRQ_DLL);
	intEnable(SYN_IRQ_DLL);		
	
	g_rs422RcvSem[UART_CHN_NUM_DLL] = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
	return;
}

int ps_WaitRead_DLL(unsigned char * uart_buf)
{
	if (*(UINT32 *)0x40000014 != 0x11)
	{
		*(UINT32 *)0x40000014 = 0x11;
	}
	
	semTake(g_rs422RcvSem[UART_CHN_NUM_DLL], WAIT_FOREVER);
	memcpy(uart_buf, g_Databuf_DLL, SYN_FRM_LEN_DLL);
	
	return SYN_FRM_LEN_DLL;
}

void task_psRead_DLL(void)
{
	unsigned char *uartbuffer = (unsigned char *)0x22000000;
	unsigned int cnt = 0;
	while(1)
		{
		ps_WaitRead_DLL(uartbuffer);
		uartbuffer += 0x10;
		cnt++;
		if(cnt>0x1000)
			{
			cnt = 0;
			printf("DLL:0x1000 times\n");
			uartbuffer = (unsigned char *)0x22000000;
			}
		}
}

void test_pl_422_5(void)
{	
	pl_Irq_Init_DLL();
	taskSpawn ("testUart_DLL",105, 0, 20000,(FUNCPTR)task_psRead_DLL,0,0,0,0,0,0,0,0,0,0);
	return;
}

#endif

#if 1

/*********************FLY******************************
**************FLY	FUNCTION**************************
*********************FLY******************************/

int g_pl_2_ps_irq_cnt = 0;

void pl_IRQ_Callback_FLY( int irqNo)
{	
	unsigned int dat;
	int i;	
	
	intDisable(irqNo);	
	
	for (i=0; i<SYN_FRM_LEN_FLY; i++)
	{	
		/*g_Databuf_FLY[i] = i;  // *unsigned int *UART_FLY_READ_DATA_REG;*/
	}
	
	intEnable(irqNo);
	/*semGive(g_rs422RcvSem[UART_CHN_NUM_FLY]);*/
	logMsg("pl-irq:%d,FLY(%d)! \n", irqNo, g_pl_2_ps_irq_cnt,3,4,5,6);
	g_pl_2_ps_irq_cnt++;
		
	return;
}

void pl_IRQ_Callback_FLY_2( int irqNo)
{	
	unsigned int dat;
	int i;	
	
	intDisable(irqNo);	
	
	for (i=0; i<SYN_FRM_LEN_FLY; i++)
	{	
		/*g_Databuf_FLY[i] = i;  // *unsigned int *UART_FLY_READ_DATA_REG;*/
	}
	
	intEnable(irqNo);
	/*semGive(g_rs422RcvSem[UART_CHN_NUM_FLY]);*/
	
	logMsg("pl-irq:%d,FLY2(%d)! \n", irqNo, g_pl_2_ps_irq_cnt,3,4,5,6);
	g_pl_2_ps_irq_cnt++;
	return;
}


void pl_Irq_Init_FLY(void)
{
	intConnect(INUM_TO_IVEC(SYN_IRQ_FLY),pl_IRQ_Callback_FLY,SYN_IRQ_FLY);
	intEnable(SYN_IRQ_FLY);		

	vx_FGicPs_Enable(SYN_IRQ_FLY);
	
	g_rs422RcvSem[UART_CHN_NUM_FLY] = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
	return;
}

void pl_Irq_Init_FLY_2(void)
{
	/*intConnect(INUM_TO_IVEC(SYN_IRQ_FLY),pl_IRQ_Callback_FLY,SYN_IRQ_FLY);*/
	/*intEnable(SYN_IRQ_FLY);	*/
	
	/*
	intConnect(INUM_TO_IVEC(INT_VEC_CAN_INT1), CAN1_interrupt_hanlder, NULL);
	intEnable(INT_VEC_CAN_INT1);
	*/
	intConnect(INUM_TO_IVEC(INT_VEC_IRQF2P_0), pl_IRQ_Callback_FLY, 57);
	intEnable(INT_VEC_IRQF2P_0);
	vx_FGicPs_Enable(57);

	taskDelay(10);
	
	intConnect(INUM_TO_IVEC(INT_VEC_IRQF2P_4), pl_IRQ_Callback_FLY_2, 61);
	intEnable(INT_VEC_IRQF2P_4);
	vx_FGicPs_Enable(61);
	
	/*g_rs422RcvSem[UART_CHN_NUM_FLY] = semBCreate(SEM_Q_FIFO,SEM_EMPTY);*/
	return;
}

int ps_WaitRead_FLY(unsigned char * uart_buf)
{
	if (*(UINT32 *)0x40000014 != 0x11)
	{
		*(UINT32 *)0x40000014 = 0x11;
	}
	
	semTake(g_rs422RcvSem[UART_CHN_NUM_FLY], WAIT_FOREVER);
	memcpy(uart_buf, g_Databuf_FLY, SYN_FRM_LEN_FLY);
	
	return SYN_FRM_LEN_FLY;
}

void task_psRead_FLY(void)
{
	unsigned char *uartbuffer = (unsigned char *)0x23000000;
	unsigned int cnt = 0;
	while(1)
		{
		ps_WaitRead_FLY(uartbuffer);
		uartbuffer += 0x80;
		cnt++;
		if(cnt>0x1000)
			{
			cnt = 0;
			printf("FLY:0x1000 times\n");
			uartbuffer = (unsigned char *)0x23000000;
			}
		}
}

void test_pl_422_4(void)
{	
	pl_Irq_Init_FLY();
	taskSpawn ("testUart_FLY",105, 0, 20000,(FUNCPTR)task_psRead_FLY,0,0,0,0,0,0,0,0,0,0);
	return;
}


#endif


#if 1

/*********************TARCK******************************
**************TARCK	FUNCTION**************************
*********************TARCK******************************/

void pl_IRQ_Callback_TARCK( int irqNo)
{	
	unsigned int dat;
	int i;	
	
	intDisable(irqNo);	
	
	for (i=0; i<SYN_FRM_LEN_TARCK; i++)
	{	
		g_Databuf_TRACK[i] = *(unsigned int *)UART_TARCK_READ_DATA_REG;
	}
	
	intEnable(irqNo);
	semGive(g_rs422RcvSem[UART_CHN_NUM_TARCK]);
	
	logMsg("TARCK!\n", 1,2,3,4,5,6);
	return;
}

void pl_Irq_Init_TARCK(void)
{
	intConnect(INUM_TO_IVEC(SYN_IRQ_TARCK), pl_IRQ_Callback_TARCK, SYN_IRQ_TARCK);
	intEnable(SYN_IRQ_TARCK);		
	
	g_rs422RcvSem[UART_CHN_NUM_TARCK] = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
	return;
}

int ps_WaitRead_TARCK(unsigned char * uart_buf)
{
	if (*(UINT32 *)0x40000014 != 0x11)
	{
		*(UINT32 *)0x40000014 = 0x11;
	}
	
	semTake(g_rs422RcvSem[UART_CHN_NUM_TARCK], WAIT_FOREVER);
	memcpy(uart_buf, g_Databuf_TRACK, SYN_FRM_LEN_TARCK);
	
	return SYN_FRM_LEN_TARCK;	
}

void task_psRead_TARCK(void)
{
	unsigned char *uartbuffer = (unsigned char *)0x24000000;
	unsigned int cnt = 0;
	
	while (1)
	{
		ps_WaitRead_TARCK(uartbuffer);
		uartbuffer += 0x40;
		
		cnt++;
		if(cnt > 0x1000)
		{
			cnt = 0;
			printf("TARCK:0x1000 times\n");
			uartbuffer = (unsigned char *)0x24000000;
		}
	}
}

void test_pl_422_3(void)
{	
	pl_Irq_Init_TARCK();
	taskSpawn ("testUart_TARCK",105, 0, 20000,(FUNCPTR)task_psRead_TARCK,0,0,0,0,0,0,0,0,0,0);
	return;
}

#endif


#if 1  /* for demo the 1-way mode*/

/*********************TV******************************
**************TV	FUNCTION**************************
*********************TV******************************/

void pl_IRQ_Callback_TV( int irqNo)
{	
	unsigned int dat;
	int i;	
	
	intDisable(irqNo);		
	
	for (i=0; i<SYN_FRM_LEN_TV; i++)
	{	
		g_Databuf_TV[i] = (UINT8)(*(unsigned int *)UART_TV_READ_DATA_REG & 0xFF);
	}
	
	intEnable(irqNo);	
	semGive(g_rs422RcvSem[UART_CHN_NUM_TV]);
	
	/*logMsg("TV!\n",1,2,3,4,5,6);*/
	return;
}

void pl_Irq_Init_TV(void)
{
	intConnect(INUM_TO_IVEC(SYN_IRQ_TV), pl_IRQ_Callback_TV, SYN_IRQ_TV);
	intEnable(SYN_IRQ_TV);	
	
	g_rs422RcvSem[UART_CHN_NUM_TV] = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
	
	return;
}

int ps_WaitRead_TV(unsigned char * uart_buf)
{
	if(*(UINT32 *)0x40000014 != 0x11)
	{
		*(UINT32 *)0x40000014 = 0x11;
	}
	
	semTake(g_rs422RcvSem[UART_CHN_NUM_TV], WAIT_FOREVER);
	memcpy(uart_buf, g_Databuf_TV, SYN_FRM_LEN_TV);
	
	return SYN_FRM_LEN_TV;
}

void task_psRead_TV(void)
{
	unsigned char *uartbuffer = (unsigned char *)0x21000000;
	unsigned int cnt = 0;
	int i = 0;
	
	while (1)
	{
		ps_WaitRead_TV(uartbuffer);

	#if 0  /* for test*/
		uartbuffer += 0x20;
		cnt++;		
		if (cnt>0x1000)
		{
			cnt = 0;
			
			printf("TV:0x1000 times\n");
			uartbuffer = (unsigned char *)0x21000000;
		}
	#endif
		
	#if 1 /* for test*/
		printf("\n<--tv rx_frm: ");
		for (i=0; i<SYN_FRM_LEN_TV; i++)
		{
			printf("%02X ", g_Databuf_TV[i]);
		}		
		printf("\n\n");
	#endif
	}

	return;
}

int g_test_pl422_tv = 0;
void test_pl422_tx_TV(void)
{
	int i = 0;
	UINT8 tx_buf[30] = {0};
	UINT8* pBuf8 = (UINT8*)UART_WRITE_DATA_REG;

	/* init*/
	tx_buf[0] = 0x80;
	for (i=1; i<SYN_FRM_LEN_TV; i++)
	{	
		tx_buf[i] = i + g_test_pl422_tv;
	}
	g_test_pl422_tv++;

	/* snd*/
	for (i=0; i<SYN_FRM_LEN_TV; i++)
	{
		*pBuf8 = tx_buf[i];
	}
	
	/* printf	*/
	printf("\n-->tv tx_frm: ");
	for (i=0; i<SYN_FRM_LEN_TV; i++)
	{
		printf("%02X ", tx_buf[i]);
	}		
	printf("\n\n");

	taskDelay(1);
	
	return;
}

void test_pl_422_2(void)
{	
	pl_Irq_Init_TV();
	
	taskSpawn ("testUart_TV",105, 0, 20000,(FUNCPTR)task_psRead_TV,0,0,0,0,0,0,0,0,0,0);
	return;
}

/*
-> testvx_update_bit3 "/sd0:1/pl_rs422_1way.bit"

-> test_pl_422_2

-> test_pl422_tx_TV
-->tv tx_frm: 80 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 
<--tv rx_frm: 80 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 
*/

#endif



/********************test demo************************************/
void testRs422ALL(void)
{
	test_pl_422_1();
	test_pl_422_2();
	test_pl_422_3();
	test_pl_422_4();
	test_pl_422_5();	

	return;
}

#if 1

#define UART_PL_DATA_REG 	0x40000000

void test_pl422_rx_poll(void)
{
	unsigned char dat[30];
	unsigned char cnt;
	int len = 29;
	
	*(UINT32 *)0x40000014 = 0x11;
	
	taskDelay(10);
	
	/*while (1)*/
	{
		/*if (*UINT32 *UART_PL_STATUS_REG == 1)*/
		{
			for (cnt = 0; cnt<len; cnt++)
			{	
				dat[cnt] = *(UINT32 *)UART_PL_DATA_REG;
			}

			
			printf("tv rx_frm: ");
			
			for (cnt = 0; cnt<len; cnt++)
			{	
				printf("%02X ", dat[cnt]);	
			}			
			printf("\n");
			
			*(UINT32 *)UART_PL_STATUS_REG = 0;	
		}
		/*
		else
		{	
			taskDelay(1);
		}
		*/
	}

	return;
}

#endif

/* test for ref_613*/
#if 1

SEM_ID g_rcvSem_613;

int g_irq_cnt = 0;

void pl_IRQ_Callback_613( int irqNo)
{	
	unsigned int dat;
	int i;	
	
	intDisable(irqNo);		
	
	g_irq_cnt++;
	
	intEnable(irqNo);
	
	semGive(g_rcvSem_613);
	
	logMsg("irq_613(%d) - %d! \n", irqNo, g_irq_cnt,3,4,5,6);
	return;
}

void pl_Irq_Init_613(void)
{
	int irq_no = 60;

#if 1
	UINT16 pl_reg = 0x60001000;
	int time = 1000000000 / 20;  /* 20ns/cnt*/

	/* clear timer*/
	*(UINT16*)pl_reg = 0;
	*(UINT16*)pl_reg = 1;

	/* set timer_step*/
	*(UINT16*)(pl_reg+4) = 1;

	/* set timer_val*/
	*(UINT16*)(pl_reg+6) = time & 0xFFFF;
	*(UINT16*)(pl_reg+8) = (time & 0xFFFF0000) >> 16;

	/* start timer*/
	*(UINT16*)(pl_reg+2) = 1;
#endif
	
	
	intConnect(irq_no, pl_IRQ_Callback_613, irq_no);
	intEnable(irq_no);	
	
	g_rcvSem_613 = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
	
	return;
}

void task_pl_irq_613(void)
{
	unsigned char *uartbuffer = (unsigned char *)0x21000000;
	unsigned int cnt = 0;
	int i = 0;
	
	while (1)
	{
		semTake(g_rcvSem_613, WAIT_FOREVER);
		
		printf("pl_irq_cnt: %d \n ", g_irq_cnt);
	}

	return;
}

void test_pl_irq_613(void)
{	
	pl_Irq_Init_613();
	
	taskSpawn ("testirq_613",105, 0, 20000,(FUNCPTR)task_pl_irq_613, 0,0,0,0,0,0,0,0,0,0);
	return;
}

#endif

