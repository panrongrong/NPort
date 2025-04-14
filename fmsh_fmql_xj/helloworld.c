/******************************************************************************
 *
 * Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Use of the Software is limited solely to applications:
 * (a) running on a Xilinx device, or
 * (b) that interact with a Xilinx device through a bus or interconnect.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Xilinx shall not be used
 * in advertising or otherwise to promote the sale, use or other dealings in
 * this Software without prior written authorization from Xilinx.
 *
 ******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */
#include <vxWorks.h>
#include <stdio.h>
#include <oled.h>
#include "ioLib.h"
#include "bmp.h"
#include <inetLib.h>
#include <taskLib.h>
#include <stdlib.h>
#include <sysLib.h>
#include <errno.h>
#include <string.h>
#include "in.h"
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include "oled.h"


#define BUFFER_SIZE 1024

uint32_t uart_flag = 1;
// 定义 uint8_t 类型
void oled_display_task(void);

// OLED 显示任务函数
void oled_display_task(void) {
	uint8_t t = ' ';
	uint8_t test[100];
	uint8_t ip0 = 0;
	uint8_t ip1 = 0;
	uint8_t ip2 = 0;
	uint8_t ip3 = 0;

	OLED_Init();  // 初始化 OLED
	while(1)
	{
		OLED_Refresh();
		OLED_ShowChinese(0,0,0,32,1);//w
		OLED_ShowChinese(32,0,1,32,1);//q
		OLED_ShowChinese(64,0,2,32,1);//s
		OLED_ShowChinese(96,0,3,32,1);//z
		OLED_ShowString(25, 40, "2013/05/01", 16, 1);
		//OLED_Refresh();
		sysDelay();
	}
	taskDelay(sysClkRateGet() * 5);

	printf("oled_display_task\r\n");
	while (1) {

		//		if (uart_flag == 0) {
		//			OLED_ShowString(0, 0, "IP:", 8, 1);
		//			OLED_ShowString(0, 20, "GATEWAY:", 8, 1);
		//			OLED_ShowString(0, 40, "MASK:", 8, 1);
		//		} else {
		//			ip0 = 192;
		//			ip1 = 168;
		//			ip2 = 5;
		//			ip3 = 1;
		//			sprintf((char *)test, "IP:%d.%d.%d.%d", ip0, ip1, ip2, ip3);
		//			OLED_ShowString(0, 0, (char *)test, 8, 1);
		//			OLED_ShowString(0, 20, "GATEWAY:", 8, 1);
		//			OLED_ShowString(0, 40, "MASK:", 8, 1);
		//		}
		//		OLED_Refresh();
		//		OLED_ShowChinese(0,0,0,32,1);//w
		//		OLED_ShowChinese(32,0,1,32,1);//q
		//		OLED_ShowChinese(64,0,2,32,1);//s
		//		OLED_ShowChinese(96,0,3,32,1);//z
		//		OLED_ShowString(25, 40, "2013/05/01", 16, 1);
		OLED_Refresh();
		//taskDelay(sysClkRateGet() / 2);
		//		taskDelay(sysClkRateGet() / 2);  // 延时 500ms
		//taskDelay(sysClkRateGet() * 5);  // 延时 5s
		//OLED_Clear();
		OLED_ScrollDisplay(4,4,1);
		taskDelay(sysClkRateGet() / 2);
#if 0
		// 上电后一直显示
		OLED_ShowChinese(0, 0, 0, 32, 1);  // w
		OLED_ShowChinese(32, 0, 1, 32, 1);  // q
		OLED_ShowChinese(64, 0, 2, 32, 1);  // s
		OLED_ShowChinese(96, 0, 3, 32, 1);  // z
		OLED_ShowString(25, 40, "2013/05/01", 16, 1);
		OLED_Refresh();
		taskDelay(sysClkRateGet() * 5);  // 延时 5s
		OLED_Clear();
		OLED_ScrollDisplay(4, 4, 1);
#endif

#if 0
		OLED_ShowPicture(0, 0, 128, 64, BMP1, 1);
		OLED_Refresh();
		taskDelay(sysClkRateGet() / 20);  // 延时 50ms
		OLED_Clear();
		OLED_ShowChinese(0, 0, 0, 16, 1);  // 中
		OLED_ShowChinese(18, 0, 1, 16, 1);  // 景
		OLED_ShowChinese(36, 0, 2, 16, 1);  // 园
		OLED_ShowChinese(54, 0, 3, 16, 1);  // 电
		OLED_ShowChinese(72, 0, 4, 16, 1);  // 子
		OLED_ShowChinese(90, 0, 5, 16, 1);  // 技
		OLED_ShowChinese(108, 0, 6, 16, 1);  // 术
		OLED_ShowString(8, 16, "ZHONGJINGYUAN", 16, 1);
		OLED_ShowString(20, 32, "2014/05/01", 16, 1);
		OLED_ShowString(0, 48, "ASCII:", 16, 1);
		OLED_ShowString(63, 48, "CODE:", 16, 1);
		OLED_ShowChar(48, 48, t, 16, 1);  // 显示 ASCII 字符
		t++;
		if (t > '~') t = ' ';
		OLED_ShowNum(103, 48, t, 3, 16, 1);
		OLED_Refresh();
		taskDelay(sysClkRateGet() / 20);  // 延时 50ms
		OLED_Clear();
		OLED_ShowChinese(0, 0, 0, 16, 1);  // 16*16 中
		OLED_ShowChinese(16, 0, 0, 24, 1);  // 24*24 中
		OLED_ShowChinese(24, 20, 0, 32, 1);  // 32*32 中
		OLED_ShowChinese(64, 0, 0, 64, 1);  // 64*64 中
		OLED_Refresh();
		taskDelay(sysClkRateGet() / 20);  // 延时 50ms
		OLED_Clear();
		OLED_ShowString(0, 0, "ABC", 8, 1);  // 6*8 “ABC”
		OLED_ShowString(0, 8, "ABC", 12, 1);  // 6*12 “ABC”
		OLED_ShowString(0, 20, "ABC", 16, 1);  // 8*16 “ABC”
		OLED_ShowString(0, 36, "ABC", 24, 1);  // 12*24 “ABC”
		OLED_Refresh();
		taskDelay(sysClkRateGet() / 20);  // 延时 50ms
		OLED_ScrollDisplay(11, 4, 1);
#endif
	}
}


// OLED 显示任务函数
//void oled_display_task(void) {
//	uint8_t t = ' ';
//	   printf("Hello World\n\r");
//	 	OLED_Init();				//��ʼ��OLED
//		while(1)
//		{
//			OLED_ShowPicture(0,0,128,64,BMP1,1);
//			OLED_Refresh();
//			//delay_ms(500);
//			taskDelay(sysClkRateGet() * 5);  // 延时 5s
////			usleep(5000);
//			OLED_Clear();
//			OLED_ShowChinese(0,0,0,16,1);//��
//			OLED_ShowChinese(18,0,1,16,1);//��
//			OLED_ShowChinese(36,0,2,16,1);//԰
//			OLED_ShowChinese(54,0,3,16,1);//��
//			OLED_ShowChinese(72,0,4,16,1);//��
//			OLED_ShowChinese(90,0,5,16,1);//��
//			OLED_ShowChinese(108,0,6,16,1);//��
//			OLED_ShowString(8,16,"ZHONGJINGYUAN",16,1);
//			OLED_ShowString(20,32,"2014/05/01",16,1);
//			OLED_ShowString(0,48,"ASCII:",16,1);
//			OLED_ShowString(63,48,"CODE:",16,1);
//			OLED_ShowChar(48,48,t,16,1);//��ʾASCII�ַ�
//			t++;
//			if(t>'~')t=' ';
//			OLED_ShowNum(103,48,t,3,16,1);
//			OLED_Refresh();
//			//delay_ms(500);
////			usleep(5000);
//			taskDelay(sysClkRateGet() * 5);  // 延时 5s
//			OLED_Clear();
//			OLED_ShowChinese(0,0,0,16,1);  //16*16 ��
//		  OLED_ShowChinese(16,0,0,24,1); //24*24 ��
//			OLED_ShowChinese(24,20,0,32,1);//32*32 ��
//		  OLED_ShowChinese(64,0,0,64,1); //64*64 ��
//			OLED_Refresh();
//		  //delay_ms(500);
////			usleep(5000);
//			taskDelay(sysClkRateGet() * 5);  // 延时 5s
//	  	OLED_Clear();
//			OLED_ShowString(0,0,"ABC",8,1);//6*8 ��ABC��
//			OLED_ShowString(0,8,"ABC",12,1);//6*12 ��ABC��
//		  OLED_ShowString(0,20,"ABC",16,1);//8*16 ��ABC��
//			OLED_ShowString(0,36,"ABC",24,1);//12*24 ��ABC��
//		  OLED_Refresh();
//			//delay_ms(500);
////		  	usleep(5000);
//		  taskDelay(sysClkRateGet() * 5);  // 延时 5s
//			OLED_ScrollDisplay(11,4,1);
//		}
//}

// 主函数，用于创建任务
void  startOLEDtask() {

	printf("startOLEDtask\r\n");
	// 创建 OLED 显示任务
	taskSpawn("oled_display_task", 100, 0, 0x2000, (FUNCPTR)oled_display_task, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

}

void SCL_task(void) {

	OLED_SCL_Set();
	taskDelay(500);
	OLED_SCL_Clr();		
	taskDelay(500);
}
void  startSCLtask() {

	printf("startOLEDtask\r\n");
	// 创建 OLED 显示任务
	taskSpawn("SCL_task", 100, 0, 0x2000, (FUNCPTR)SCL_task, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

}

void SDA_task(void) {

	OLED_SDA_Set();
	taskDelay(500);
	OLED_SDA_Clr();		
	taskDelay(500);
}
void  startSDAtask() {

	// 创建 OLED 显示任务
	taskSpawn("SDA_task", 100, 0, 0x2000, (FUNCPTR)SDA_task, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

}

void RES_task(void) {

	OLED_RES_Set();
	taskDelay(500);
	OLED_RES_Clr();		
	taskDelay(500);
}
void  startREStask() {

	// 创建 OLED 显示任务
	taskSpawn("RES_task", 100, 0, 0x2000, (FUNCPTR)RES_task, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

}

void DC_task(void) {

	OLED_DC_Set();
	taskDelay(500);
	OLED_DC_Clr();		
	taskDelay(500);
}
void  startDCtask() {

	// 创建 OLED 显示任务
	taskSpawn("DC_task", 100, 0, 0x2000, (FUNCPTR)DC_task, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

}

void CS_task(void) {

	OLED_CS_Set();
	taskDelay(500);
	OLED_CS_Clr();		
	taskDelay(500);
}
void  startCStask() {

	// 创建 OLED 显示任务
	taskSpawn("CS_task", 100, 0, 0x2000, (FUNCPTR)CS_task, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

}

void oled0_task(void) {
	//	for(int i = 0;i < 10;i++)
	//	{
	OLED0_Set();
	taskDelay(50);
	OLED1_Set();
	taskDelay(50);
	OLED2_Set();
	taskDelay(50);
	OLED3_Set();
	taskDelay(50);
	OLED4_Set();
	taskDelay(50);
	OLED5_Set();
	taskDelay(50);
	OLED6_Set();
	taskDelay(50);
	OLED7_Set();
	taskDelay(50);
	OLED8_Set();
	taskDelay(50);
	OLED9_Set();
	taskDelay(50);
	OLED10_Set();
	taskDelay(50);
	OLED11_Set();
	taskDelay(50);
	OLED12_Set();
	taskDelay(50);
	OLED13_Set();
	taskDelay(50);
	OLED14_Set();
	taskDelay(50);
	OLED15_Set();
	taskDelay(50);
	//	OLED0_Clr();		
	//	taskDelay(3);
	//	}
}
void  startoled0task() {

	// 创建 OLED 显示任务
	taskSpawn("oled0_task", 100, 0, 0x2000, (FUNCPTR)oled0_task, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

}
void oledTX0_task(void) {
	//	for(int i = 0;i < 10;i++)
	//	{
	OLEDTX0_Set();
	taskDelay(50);
	OLEDTX1_Set();
	taskDelay(50);
	OLEDTX2_Set();
	taskDelay(50);
	OLEDTX3_Set();
	taskDelay(50);
	OLEDTX4_Set();
	taskDelay(50);
	OLEDTX5_Set();
	taskDelay(50);
	OLEDTX6_Set();
	taskDelay(50);
	OLEDTX7_Set();
	taskDelay(50);
	OLEDTX8_Set();
	taskDelay(50);
	OLEDTX9_Set();
	taskDelay(50);
	OLEDTX10_Set();
	taskDelay(50);
	OLEDTX11_Set();
	taskDelay(50);
	OLEDTX12_Set();
	taskDelay(50);
	OLEDTX13_Set();
	taskDelay(50);
	OLEDTX14_Set();
	taskDelay(50);
	OLEDTX15_Set();
	taskDelay(50);
	//	OLED0_Clr();		
	//	taskDelay(3);
	//	}
}
void  startoledTXtask() {

	// 创建 OLED 显示任务
	taskSpawn("oledTX0_task", 100, 0, 0x2000, (FUNCPTR)oledTX0_task, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

}
void oledRX0_task(void) {
	//	for(int i = 0;i < 10;i++)
	//	{
	OLEDRX0_Set();
	taskDelay(50);
	OLEDRX1_Set();
	taskDelay(50);
	OLEDRX2_Set();
	taskDelay(50);
	OLEDRX3_Set();
	taskDelay(50);
	OLEDRX4_Set();
	taskDelay(50);
	OLEDRX5_Set();
	taskDelay(50);
	OLEDRX6_Set();
	taskDelay(50);
	OLEDRX7_Set();
	taskDelay(50);
	OLEDRX8_Set();
	taskDelay(50);
	OLEDRX9_Set();
	taskDelay(50);
	OLEDRX10_Set();
	taskDelay(50);
	OLEDRX11_Set();
	taskDelay(50);
	OLEDRX12_Set();
	taskDelay(50);
	OLEDRX13_Set();
	taskDelay(50);
	OLEDRX14_Set();
	taskDelay(50);
	OLEDRX15_Set();
	taskDelay(50);
	//	OLED0_Clr();		
	//	taskDelay(3);
	//	}
}
void  startoledRXtask() {

	// 创建 OLED 显示任务
	taskSpawn("oledRX0_task", 100, 0, 0x2000, (FUNCPTR)oledRX0_task, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

}
//void oled_display()
//{
//	OLED_Init();
//	while(1)
//	{
//		OLED_ShowChinese(0,0,0,32,1);//w
//		OLED_ShowChinese(32,0,1,32,1);//q
//		OLED_ShowChinese(64,0,2,32,1);//s
//		OLED_ShowChinese(96,0,3,32,1);//z
//		OLED_ShowString(25,40,"2013/05/01",16,1);
//		OLED_Refresh();
//		//usleep(5000000);
//		taskDelay(sysClkRateGet() * 5);
//		OLED_Clear();
//		OLED_ScrollDisplay(4,4,1);		
//	}
//}



void oled_display()
{
	OLED_Init();
	while(1)
	{
		OLED_ShowString(0, 0, "gem0:192.168.46.77", 12, 1);  // 显示字符串
		OLED_ShowString(0, 20, "gem1:192.168.46.77", 12, 1);  // 显示字符串
		//		OLED_ShowString(25,40,"2013/05/01",16,1);
		OLED_Refresh();
		//usleep(5000000);
		taskDelay(5000);
		OLED_Clear();
		//		OLED_ScrollDisplay(13,4,1);	

	}
}
void  startoleddisplaytask() {

	// 创建 OLED 显示任务
	taskSpawn("oled_displaytask", 100, 0, 0x2000, (FUNCPTR)oled_display, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

}
