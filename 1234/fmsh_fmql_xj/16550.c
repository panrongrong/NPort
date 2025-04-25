#include <vxWorks.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <fcntl.h>
#include <tickLib.h>
#include "pack2.h"

#define AXI_UART_BASE(n)         (0x41200000 + 0x2000 * n) /* 基地址 */
#define AXI_16550_INT            (84)
#define AXI_16550_CLK            (29491200)
#define AXI_16550_CLK1           (32000000)
#define AXI_16550_RBR            (0x1000)
#define AXI_16550_THR            (0x1000)
#define AXI_16550_IER            (0x1004)
#define AXI_16550_IIR            (0x1008)
#define AXI_16550_FCR            (0x1008)
#define AXI_16550_LCR            (0x100C)
#define AXI_16550_MCR            (0x1010)
#define AXI_16550_LSR            (0x1014)
#define AXI_16550_MSR            (0x1018)
#define AXI_16550_SCR            (0x101C)
#define AXI_16550_DLL            (0x1000)
#define AXI_16550_DLM            (0x1004)
#define BRAM_KZ                  (0x00000004)
#define LCR_SBRK                  0x40  // BREAK 信号控制位
#define LSR_TX_READY              0x01  /* Data Ready */
#define LSR_TX_BUFFER_EMPTY       0x20  /* Transmit  reg empty */
// 定义 XON/XOFF 控制字符（ASCII 值）
#define XON_CHAR  0x11  // XON 字符（DC1）
#define XOFF_CHAR 0x13  // XOFF 字符（DC3）
#define LSR_THRE_MASK  0x20    // 发送保持寄存器为空标志位


int32_t sysAxiReadLong(ULONG address) {
	return *(volatile int32_t*)address;
}

void sysAxiWriteLong(ULONG address, int32_t data) {
	*(volatile int32_t*)address = data;
}

void userAxiCfgWrite(unsigned int channel, unsigned int offset, unsigned int data) {
	sysAxiWriteLong(AXI_UART_BASE(channel) + offset, data);
}

unsigned int userAxiCfgRead(unsigned int channel, unsigned int offset) {
	unsigned int data = 0;
	data = sysAxiReadLong(AXI_UART_BASE(channel) + offset);
	return data;
}

int axi16550Recv(unsigned int channel, uint8_t *buffer, uint32_t *len) {
	*len = 0;
	/* Read data until there is no more data available */
	while (userAxiCfgRead(channel, AXI_16550_LSR) & LSR_TX_READY) {
		/* Read the data from the UART */
		buffer[(*len)++] = userAxiCfgRead(channel, AXI_16550_RBR);
		
	}
	if (buffer == NULL || *len == 0)
		return -1;
	return 0;
}

int axi16550_TxReady(unsigned int channel)
{
	if((userAxiCfgRead(channel, AXI_16550_LSR) & LSR_TX_BUFFER_EMPTY) == 0)
		return 0;
	else 
		return 1;
}

int axi16550Send(unsigned int channel, uint8_t *buffer, uint32_t len) {
	int i = 0;
	if (buffer == NULL || len < 0) {
		return -1;
	}
	for (i = 0; i < len; i++) {
		/* Wait for the transmitter holding register to be empty */
		if ((userAxiCfgRead(channel, AXI_16550_LSR) & LSR_TX_BUFFER_EMPTY) == 1)
		{
			printf("TBE!\r\n");
		}
		while ((userAxiCfgRead(channel, AXI_16550_LSR) & LSR_TX_BUFFER_EMPTY) == 0);
		userAxiCfgWrite(channel, AXI_16550_THR, buffer[i]);
	}
	return 0;
}


void axi16550BaudInit(unsigned int channel, unsigned int baud) {
	unsigned int div;
	unsigned short dlm, dll;
	unsigned char reg;
	div = AXI_16550_CLK / 16 / baud;
	dlm = (div >> 8) & 0xFF;
	dll = div & 0xFF;
	reg = userAxiCfgRead(channel, AXI_16550_LCR);
	userAxiCfgWrite(channel, AXI_16550_LCR, reg | 0x80);
	userAxiCfgWrite(channel, AXI_16550_DLM, dlm);
	userAxiCfgWrite(channel, AXI_16550_DLL, dll);
	userAxiCfgWrite(channel, AXI_16550_LCR, reg);
}

// 发送 START BREAK 信号
void axi16550SendStartBreak(unsigned int channel) {
	unsigned char lcr = userAxiCfgRead(channel, AXI_16550_LCR);
	// 设置 SBRK 位，启动 BREAK
	lcr |= LCR_SBRK;
	userAxiCfgWrite(channel, AXI_16550_LCR, lcr);

	// 保持一段时间（根据需求调整延时）
	taskDelay(10);  // 假设 taskDelay 单位为系统时钟滴答，延时 10 个滴答
}

// 发送 STOP BREAK 信号
void axi16550SendStopBreak(unsigned int channel) {
	unsigned char lcr = userAxiCfgRead(channel, AXI_16550_LCR);
	// 清除 SBRK 位，停止 BREAK
	lcr &= ~LCR_SBRK;
	userAxiCfgWrite(channel, AXI_16550_LCR, lcr);
}

// 发送 XON/XOFF 字符函数
void send_xon_xoff_char(uint8_t channel, uint8_t is_xon) {
	uint8_t control_char = is_xon ? XON_CHAR : XOFF_CHAR;
	// 等待发送缓冲区为空
	while (!(userAxiCfgRead(channel, AXI_16550_LSR) & LSR_THRE_MASK));
	// 写入发送保持寄存器
	userAxiCfgWrite(channel, AXI_16550_THR, control_char);
}

void axi16550Init(unsigned int channel, unsigned int baud)
{
    unsigned int div;
    unsigned short dlm, dll;
    unsigned char reg;
    div = AXI_16550_CLK / 16 / baud;
    dlm = (div >> 8) & 0xFF;
    dll = div & 0xFF;
    reg = userAxiCfgRead(channel, AXI_16550_LCR);
    userAxiCfgWrite(channel, AXI_16550_LCR, reg | 0x80);
    userAxiCfgWrite(channel, AXI_16550_DLM, dlm); /* dlm */
    userAxiCfgWrite(channel, AXI_16550_DLL, dll); /* dll */
    userAxiCfgWrite(channel, AXI_16550_LCR, reg);
    userAxiCfgWrite(channel, AXI_16550_LCR, 0x03);
    userAxiCfgWrite(channel, AXI_16550_FCR, 0x87);
    userAxiCfgWrite(channel, AXI_16550_FCR, 0x81);
    userAxiCfgWrite(channel, AXI_16550_MCR, 0x00); /* 0x00  normal -> 0x10 loopback */
    userAxiCfgWrite(channel, AXI_16550_IER, 0x00);

}

void axi165502CInit(UART_Config_Params *uart_instance,int channel)
{
	unsigned int div;
	unsigned short dlm, dll;
	unsigned char reg, lcr = 0;

	switch(uart_instance->config.data_bit) {
	case 5: lcr |= 0x00; break;
	case 6: lcr |= 0x01; break;
	case 7: lcr |= 0x02; break;
	case 8: lcr |= 0x03; break;
	}
	if (uart_instance->config.stop_bit == 2) lcr |= 0x04;
	if (uart_instance->config.parity != 0) {
		lcr |= 0x08; 
		if (uart_instance->config.parity == 2) lcr |= 0x10;
	}


	div = AXI_16550_CLK / 16 /uart_instance->config.baud_rate;
	dlm = (div >> 8) & 0xFF;
	dll = div & 0xFF;


	reg = userAxiCfgRead(channel, AXI_16550_LCR);
	userAxiCfgWrite(channel, AXI_16550_LCR, reg | 0x80);
	userAxiCfgWrite(channel, AXI_16550_DLM, dlm);
	userAxiCfgWrite(channel, AXI_16550_DLL, dll);
	userAxiCfgWrite(channel, AXI_16550_LCR, reg); 

	userAxiCfgWrite(channel, AXI_16550_LCR, lcr);

	userAxiCfgWrite(channel, AXI_16550_FCR, 0x87); 
	userAxiCfgWrite(channel, AXI_16550_FCR, 0x81);
	userAxiCfgWrite(channel, AXI_16550_MCR, 0x00);
	userAxiCfgWrite(channel, AXI_16550_IER, 0x00);
} 


void uart_task(unsigned int channel)
{
	uint8_t recv_buf[100];
	uint8_t send_buf[100];
	unsigned int i =0;
	unsigned int j =0;
	unsigned int cnt =0;
	unsigned int len=0,res=0;
	axi16550Init(channel, 921600);
	while(1)
	{
//		for(i=0;i<24;i++)
//		{
			res = axi16550Recv(channel,recv_buf,&len);
			if(res != -1)
			{
				cnt = len+cnt;
				printf("uart485 channel:%d,recv cnt:%d\r\n",channel,cnt);
				for(j=0;j<len;j++)
				{
					 printf("0x%02X  ",recv_buf[j]);
				}
				printf("\r\n");
			}
//		}
		len=0;
		memset(send_buf,0,100);
		memset(recv_buf,0,100);
		taskDelay(10);
	}
}
