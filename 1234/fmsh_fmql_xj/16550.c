#include <vxWorks.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <fcntl.h>
#include <tickLib.h>

#define AXI_UART_BASE(n)         (0x41200000 + 0x2000 * n) /* ����ַ */
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
#define LCR_SBRK                  0x40  // BREAK �źſ���λ
#define LSR_TX_READY              0x01  /* Data Ready */
#define LSR_TX_BUFFER_EMPTY       0x20  /* Transmit  reg empty */
// ���� XON/XOFF �����ַ���ASCII ֵ��
#define XON_CHAR  0x11  // XON �ַ���DC1��
#define XOFF_CHAR 0x13  // XOFF �ַ���DC3��
#define LSR_THRE_MASK  0x20    // ���ͱ��ּĴ���Ϊ�ձ�־λ

#define CHUNK_SIZE 1024  // ÿ�η��͵����ݿ��С
#define TIMEOUT_MS 1000  // ��ʱʱ�䣨���룩
 
#define CHOOSEONE_BAUD 115200  
typedef struct usart_params
{
	unsigned int  baud_rate;
	unsigned char data_bit;
	unsigned char stop_bit;
	unsigned char parity;
	unsigned char mark;
	unsigned char space;
	unsigned char usart_mcr_dtr;
	unsigned char usart_mcr_rts;
	unsigned char usart_crtscts;
	unsigned char IX_on;
	unsigned char IX_off; //XonXoff
}usart_params_t;

typedef struct {
	usart_params_t config;  // ��������
    MSG_Q_ID msg_queue;           // ��Ϣ����ָ�루ƽ̨���ʵ�֣�

    int sock_cmd;               // Socket�ļ�������
    int sock_data;              // Socket�ļ�������

    uint16_t sock_cmd_port;       // Socket�˿ں�
    uint16_t sock_data_port;       // Socket�˿ں�

    char *tx_buffer;          // ���ͻ�����ָ��
    char *rx_buffer;          // ���ջ�����ָ��

	uint8_t enable; //ʹ��
} UART_Config_Params;

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
//	printf("buffer[(*len)++]:%d\r\n",*len);
	return 0;
}

int axi16550Send(unsigned int channel, uint8_t *buffer, uint32_t len) {
	int i = 0;
//	printf("axi16550Send!\r\n");
	if (buffer == NULL || len < 0) {
		//printf("buffer null!\r\n");
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
//		printf("userAxiCfgRead!\r\n");
	}
	return 0;
}

void axi165502CInit(unsigned int channel, usart_params_t params)
{
	unsigned int div;
	unsigned short dlm, dll;
	unsigned char reg, lcr = 0;

	// 1. ���� LCR ֵ
	switch(params.data_bit) {
	case 5: lcr |= 0x00; break;
	case 6: lcr |= 0x01; break;
	case 7: lcr |= 0x02; break;
	case 8: lcr |= 0x03; break;
	}
	if (params.stop_bit == 2) lcr |= 0x04;
	if (params.parity != 0) {
		lcr |= 0x08; // PEN=1��������żУ��
		if (params.parity == 2) lcr |= 0x10; // EPS=1��ѡ��żУ��
	}

	// 2. ���ò�����
	div = AXI_16550_CLK / 16 / params.baud_rate;
	dlm = (div >> 8) & 0xFF;
	dll = div & 0xFF;

	// 3. д�� LCR �Ĵ��������� DLAB ���ʳ����Ĵ�����
	reg = userAxiCfgRead(channel, AXI_16550_LCR);
	userAxiCfgWrite(channel, AXI_16550_LCR, reg | 0x80);
	userAxiCfgWrite(channel, AXI_16550_DLM, dlm);
	userAxiCfgWrite(channel, AXI_16550_DLL, dll);
	userAxiCfgWrite(channel, AXI_16550_LCR, reg); // �ָ� DLAB=0

	// 4. д�� LCR �Ĵ�������������λ��ֹͣλ����żУ�飩
	userAxiCfgWrite(channel, AXI_16550_LCR, lcr);

	// 5. �����Ĵ������ã�ʾ����
//	userAxiCfgWrite(channel, AXI_16550_FCR, 0xCF);
	userAxiCfgWrite(channel, AXI_16550_FCR, 0x87); 
	userAxiCfgWrite(channel, AXI_16550_FCR, 0x81);
	userAxiCfgWrite(channel, AXI_16550_MCR, 0x00);
	userAxiCfgWrite(channel, AXI_16550_IER, 0x00);
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

// ���� START BREAK �ź�
void axi16550SendStartBreak(unsigned int channel) {
	unsigned char lcr = userAxiCfgRead(channel, AXI_16550_LCR);
	// ���� SBRK λ������ BREAK
	lcr |= LCR_SBRK;
	userAxiCfgWrite(channel, AXI_16550_LCR, lcr);

	// ����һ��ʱ�䣨�������������ʱ��
	taskDelay(10);  // ���� taskDelay ��λΪϵͳʱ�ӵδ���ʱ 10 ���δ�
}

// ���� STOP BREAK �ź�
void axi16550SendStopBreak(unsigned int channel) {
	unsigned char lcr = userAxiCfgRead(channel, AXI_16550_LCR);
	// ��� SBRK λ��ֹͣ BREAK
	lcr &= ~LCR_SBRK;
	userAxiCfgWrite(channel, AXI_16550_LCR, lcr);
}

// ���� XON/XOFF �ַ�����
void send_xon_xoff_char(uint8_t channel, uint8_t is_xon) {
	uint8_t control_char = is_xon ? XON_CHAR : XOFF_CHAR;
	// �ȴ����ͻ�����Ϊ��
	while (!(userAxiCfgRead(channel, AXI_16550_LSR) & LSR_THRE_MASK));
	// д�뷢�ͱ��ּĴ���
	userAxiCfgWrite(channel, AXI_16550_THR, control_char);
}

//void axi16550Init(unsigned int channel,unsigned int baud)
//{
//    unsigned int div;
//    unsigned short dlm,dll;
//    unsigned char reg;
//    div = AXI_16550_CLK/16/baud;
//    dlm = (div>>8)&0xFF;
//    dll = div&0xFF;
//    reg = userAxiCfgRead(channel,AXI_16550_LCR);
//    userAxiCfgWrite(channel,AXI_16550_LCR,reg|0x80);
//    userAxiCfgWrite(channel,AXI_16550_DLM,dlm);/*dlm*/
//    userAxiCfgWrite(channel,AXI_16550_DLL,dll);/*dll*/
//    userAxiCfgWrite(channel,AXI_16550_LCR,reg);
//    userAxiCfgWrite(channel,AXI_16550_LCR,0x03);
//    userAxiCfgWrite(channel,AXI_16550_FCR,0xCF);
////    userAxiCfgWrite(channel,AXI_16550_FCR,0x81);
//    userAxiCfgWrite(channel,AXI_16550_MCR,0x00);/*0x00  normal -> 0x10 loopback */
//    userAxiCfgWrite(channel,AXI_16550_IER,0x00);
////    sysAxiWriteLong(0x58800000+BRAM_KZ,0<<channel);/* recv enable */
//}

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

    userAxiCfgWrite(channel, AXI_16550_FCR, 0xCF);
//    userAxiCfgWrite(channel, AXI_16550_FCR, 0x81);
    userAxiCfgWrite(channel, AXI_16550_MCR, 0x00); /* 0x00  normal -> 0x10 loopback */
    userAxiCfgWrite(channel, AXI_16550_IER, 0x00);

}
