/* ------------------------------------------------------------ */
/*				Include File Definitions						*/
/* ------------------------------------------------------------ */

#include "display_demo.h"
#include "./hdmi_app/display_ctrl/display_ctrl.h"
#include <stdio.h>
#include "math.h"
#include <ctype.h>
#include <stdlib.h>
#include "./include/xil_types.h"
#include "./include/xil_cache.h"
#include "./include/xparameters.h"
#include "pic_800_600.h"

/* jc*/
/*#include "i2c/PS_i2c.h"*/
/*#include "xgpio.h"*/
/*#include "sleep.h"*/

/*
 * XPAR redefines
 */
#define DYNCLK_BASEADDR XPAR_AXI_DYNCLK_0_BASEADDR
#define VGA_VDMA_ID XPAR_AXIVDMA_0_DEVICE_ID
#define DISP_VTC_ID XPAR_VTC_0_DEVICE_ID
#define VID_VTC_IRPT_ID XPS_FPGA3_INT_ID
#define VID_GPIO_IRPT_ID XPS_FPGA4_INT_ID
#define SCU_TIMER_ID XPAR_SCUTIMER_DEVICE_ID
#define UART_BASEADDR XPAR_PS7_UART_1_BASEADDR


/* ------------------------------------------------------------ */
/*				Global Variables								*/
/* ------------------------------------------------------------ */

/*
 * Display Driver structs
 */
DisplayCtrl dispCtrl;
XAxiVdma vdma;

/*
 * Framebuffers for video data
 */
u8 frameBuf[DISPLAY_NUM_FRAMES][DEMO_MAX_FRAME] __attribute__ ((aligned(64)));
u8 *pFrames[DISPLAY_NUM_FRAMES]; /*array of pointers to the frame buffers*/


XIicPs IicInstance;

extern XGpio GpioOutput;


extern void i2c_slave_sii9134_init(void);
extern void gpio_hdmi_sii9134_rst(void);

void Xil_DCacheFlushRange(void * Addr, u32 Len)
{
	cacheFlush(DATA_CACHE, (void *)(Addr), (Len));
}

/* ------------------------------------------------------------ */
/*				Procedure Definitions							*/
/* ------------------------------------------------------------ */




void DemoPrintTest(u8 *frame, u32 width, u32 height, u32 stride, int pattern)
{
	u32 xcoi, ycoi;
	u32 xcoi2;
	u32 iPixelAddr = 0;
	u8 wRed, wBlue, wGreen;
	u32 xInt, yInt;
	u32 pic_number=0;


	switch (pattern)
	{
	case DEMO_PATTERN_0:
		for(ycoi = 0; ycoi < 600; ycoi++)
		{
			for(xcoi = 0; xcoi < (800 * BYTES_PIXEL); xcoi+=BYTES_PIXEL)
			{
				frame[xcoi + iPixelAddr + 0] = gImage_pic_800_600[pic_number++];
				frame[xcoi + iPixelAddr + 1] = gImage_pic_800_600[pic_number++];
				frame[xcoi + iPixelAddr + 2] = gImage_pic_800_600[pic_number++];
			}
			iPixelAddr += stride;
		}
		/*
		 * Flush the framebuffer memory range to ensure changes are written to the
		 * actual memory, and therefore accessible by the VDMA.
		 */
		Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
		break;
		
		
	case DEMO_PATTERN_1:         /* Grid: black&white*/
		for(ycoi = 0; ycoi < height; ycoi++)
		{
			for(xcoi = 0; xcoi < (width * BYTES_PIXEL); xcoi+=BYTES_PIXEL)
			{
				if (((xcoi/BYTES_PIXEL)&0x20)^(ycoi&0x20)) 
				{
					wRed = 255;
					wGreen = 255;
					wBlue = 255;
				}
				else
				{
					wRed = 0;
					wGreen = 0;
					wBlue = 0;
				}


				frame[xcoi + iPixelAddr + 0] = wBlue;
				frame[xcoi + iPixelAddr + 1] = wGreen;
				frame[xcoi + iPixelAddr + 2] = wRed;
			}
			iPixelAddr += stride;
		}
		/*
		 * Flush the framebuffer memory range to ensure changes are written to the
		 * actual memory, and therefore accessible by the VDMA.
		 */
		Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
		break;
		
	case DEMO_PATTERN_2: /* 8 intervals: black&white bar*/
		for(ycoi = 0; ycoi < height; ycoi++)
		{
			for(xcoi = 0; xcoi < (width * BYTES_PIXEL); xcoi+=BYTES_PIXEL)
			{

				frame[xcoi + iPixelAddr + 0] = xcoi/BYTES_PIXEL;
				frame[xcoi + iPixelAddr + 1] = xcoi/BYTES_PIXEL;
				frame[xcoi + iPixelAddr + 2] = xcoi/BYTES_PIXEL;
			}
			iPixelAddr += stride;
		}
		/*
		 * Flush the framebuffer memory range to ensure changes are written to the
		 * actual memory, and therefore accessible by the VDMA.
		 */
		Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
		break;
		
	case DEMO_PATTERN_5:         /* Grid: colors	*/
		xInt = width*BYTES_PIXEL / 8; /*each with width/8 pixels	*/
		yInt = 0;
		for(ycoi = 0; ycoi < height; ycoi++)
		{
			for(xcoi2 = 0; xcoi2 < (width * BYTES_PIXEL); xcoi2+=BYTES_PIXEL)
			{
				if (((xcoi2/BYTES_PIXEL)&0x20)^(ycoi&0x20)) 
				{
					xcoi = (xcoi2);
					
					if (xcoi < xInt) {                                   /*White color*/
						wRed = 255;
						wGreen = 255;
						wBlue = 255;
					}
					else if ((xcoi >= xInt) && (xcoi < xInt*2))	{         /*YELLOW color*/
						wRed = 255;
						wGreen = 255;
						wBlue = 0;
					}
					else if ((xcoi >= xInt*2) && (xcoi < xInt*3)){        /*CYAN color*/
						wRed = 0;
						wGreen = 255;
						wBlue = 255;
					}
					else if ((xcoi >= xInt*3) && (xcoi < xInt*4)){        /*GREEN color*/
						wRed = 0;
						wGreen = 255;
						wBlue = 0;
					}
					else if ((xcoi >= xInt*4) && (xcoi < xInt*5)){        /*MAGENTA color*/
						wRed = 255;
						wGreen = 0;
						wBlue = 255;
					}
					else if ((xcoi >= xInt*5) && (xcoi < xInt*6)){        /*RED color*/
						wRed = 255;
						wGreen = 0;
						wBlue = 0;
					}
					else if ((xcoi >= xInt*6) && (xcoi < xInt*7)){        /*BLUE color*/
						wRed = 0;
						wGreen = 0;
						wBlue = 255;
					}
					else {                                                /*BLACK color*/
						wRed = 0;
						wGreen = 0;
						wBlue = 0;
					}
				}

				frame[xcoi+iPixelAddr + 0] = wBlue;
				frame[xcoi+iPixelAddr + 1] = wGreen;
				frame[xcoi+iPixelAddr + 2] = wRed;
			}
			
			iPixelAddr += stride;
		}
		/*
		 * Flush the framebuffer memory range to ensure changes are written to the
		 * actual memory, and therefore accessible by the VDMA.
		 */
		Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
		break;
		
	case DEMO_PATTERN_3:  /* 8 intervals color bar*/

		xInt = width*BYTES_PIXEL / 8; /*each with width/8 pixels*/

		for(ycoi = 0; ycoi < height; ycoi++)
		{

			/*
			 * Just draw white in the last partial interval (when width is not divisible by 7)
			 */

			for(xcoi = 0; xcoi < (width*BYTES_PIXEL); xcoi+=BYTES_PIXEL)
			{

				if (xcoi < xInt) {                                   /*White color*/
					wRed = 255;
					wGreen = 255;
					wBlue = 255;
				}

				else if ((xcoi >= xInt) && (xcoi < xInt*2)){         /*YELLOW color*/
					wRed = 255;
					wGreen = 255;
					wBlue = 0;
				}
				else if ((xcoi >= xInt*2) && (xcoi < xInt*3)){        /*CYAN color*/
					wRed = 0;
					wGreen = 255;
					wBlue = 255;
				}
				else if ((xcoi >= xInt*3) && (xcoi < xInt*4)){        /*GREEN color*/
					wRed = 0;
					wGreen = 255;
					wBlue = 0;
				}
				else if ((xcoi >= xInt*4) && (xcoi < xInt*5)){        /*MAGENTA color*/
					wRed = 255;
					wGreen = 0;
					wBlue = 255;
				}
				else if ((xcoi >= xInt*5) && (xcoi < xInt*6)){        /*RED color*/
					wRed = 255;
					wGreen = 0;
					wBlue = 0;
				}
				else if ((xcoi >= xInt*6) && (xcoi < xInt*7)){        /*BLUE color*/
					wRed = 0;
					wGreen = 0;
					wBlue = 255;
				}
				else {                                                /*BLACK color*/
					wRed = 0;
					wGreen = 0;
					wBlue = 0;
				}

				frame[xcoi+iPixelAddr + 0] = wBlue;
				frame[xcoi+iPixelAddr + 1] = wGreen;
				frame[xcoi+iPixelAddr + 2] = wRed;
				/*
				 * This pattern is printed one vertical line at a time, so the address must be incremented
				 * by the stride instead of just 1.
				 */
			}
			iPixelAddr += stride;

		}
		/*
		 * Flush the framebuffer memory range to ensure changes are written to the
		 * actual memory, and therefore accessible by the VDMA.
		 */
		Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
		break;
		
	default :
		printf("Error: invalid pattern passed to DemoPrintTest");
	}
}

void DemoPrintTest2(u8 *frame, u32 width, u32 height, u32 stride, int pattern, UINT8* pBuf)
{
	u32 xcoi, ycoi;
	u32 iPixelAddr = 0;
	u8 wRed, wBlue, wGreen;
	u32 xInt;
	u32 pic_number=0;

	switch (pattern)
	{		
	case DEMO_PATTERN_4:
		for(ycoi = 0; ycoi < 600; ycoi++)
		{
			for(xcoi = 0; xcoi < (800 * BYTES_PIXEL); xcoi+=BYTES_PIXEL)
			{
				frame[xcoi + iPixelAddr + 0] = pBuf[pic_number++];
				frame[xcoi + iPixelAddr + 1] = pBuf[pic_number++];
				frame[xcoi + iPixelAddr + 2] = pBuf[pic_number++];
			}
			
			iPixelAddr += stride;
		}
		/*
		 * Flush the framebuffer memory range to ensure changes are written to the
		 * actual memory, and therefore accessible by the VDMA.
		 */
		Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
		break;
	}

	return;
}

int test_hdmi(void)
{

	int Status;
	XAxiVdma_Config *vdmaConfig;
	int i;
	int cnt = 0;
	
	/*
	 * Initialize an array of pointers to the 3 frame buffers
	 */
	for (i = 0; i < DISPLAY_NUM_FRAMES; i++)
	{
		pFrames[i] = frameBuf[i];
	}


	/**/
	/* reset 9134*/
	/**/
	gpio_hdmi_sii9134_rst();
	

	/**/
	/* enable sii9134 by i2c*/
	/**/
	i2c_slave_sii9134_init();


	/*
	 * Initialize VDMA driver
	 */
	vdmaConfig = XAxiVdma_LookupConfig(VGA_VDMA_ID);
	if (!vdmaConfig)
	{
		printf("No video DMA found for ID %d\r\n", VGA_VDMA_ID);

	}
	Status = XAxiVdma_CfgInitialize(&vdma, vdmaConfig, vdmaConfig->BaseAddress);
	if (Status != XST_SUCCESS)
	{
		printf("VDMA Configuration Initialization failed %d\r\n", Status);

	}

	/*
	 * Initialize the Display controller and start it
	 */
	Status = DisplayInitialize(&dispCtrl, &vdma, DISP_VTC_ID, DYNCLK_BASEADDR, pFrames, DEMO_STRIDE);
	if (Status != XST_SUCCESS)
	{
		printf("Display Ctrl initialization failed during demo initialization%d\r\n", Status);

	}
	Status = DisplayStart(&dispCtrl);
	if (Status != XST_SUCCESS)
	{
		printf("Couldn't start display during demo initialization%d\r\n", Status);

	}
	
	printf("hdmi show pic ... ... \n\n");	
		
	cnt = 0;
	
	while (1)
	{
		DemoPrintTest(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride, DEMO_PATTERN_0);
		taskDelay(180);
		
		DemoPrintTest(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride, DEMO_PATTERN_1);
		taskDelay(180);
		
		DemoPrintTest(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride, DEMO_PATTERN_2);
		taskDelay(180);
		
		DemoPrintTest(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride, DEMO_PATTERN_3);
		taskDelay(180);

		/* end of test*/
		cnt++;
		if (cnt > 10)
		{
			break;
		}
	}
	
	return 0;
}

/**/
/* test bmp file with tool: Image2Lcd.exe*/
/**/
int test_hdmi_bmp(char* pic_file)
{

	int Status;
	XAxiVdma_Config *vdmaConfig;
	int i;	
	int cnt = 0;
	
	UINT8 fileName[64] = {0};
	int fd = 0;
	UINT8 * pBuf8 = NULL;
	int ret = 0;

	/*
	 * Initialize an array of pointers to the 3 frame buffers
	 */
	for (i = 0; i < DISPLAY_NUM_FRAMES; i++)
	{
		pFrames[i] = frameBuf[i];
	}


	/**/
	/* reset 9134*/
	/**/
	gpio_hdmi_sii9134_rst();
	

	/**/
	/* enable sii9134 by i2c*/
	/**/
	i2c_slave_sii9134_init();


	/*
	 * Initialize VDMA driver
	 */
	vdmaConfig = XAxiVdma_LookupConfig(VGA_VDMA_ID);
	if (!vdmaConfig)
	{
		printf("No video DMA found for ID %d\r\n", VGA_VDMA_ID);

	}
	Status = XAxiVdma_CfgInitialize(&vdma, vdmaConfig, vdmaConfig->BaseAddress);
	if (Status != XST_SUCCESS)
	{
		printf("VDMA Configuration Initialization failed %d\r\n", Status);

	}

	/*
	 * Initialize the Display controller and start it
	 */
	Status = DisplayInitialize(&dispCtrl, &vdma, DISP_VTC_ID, DYNCLK_BASEADDR, pFrames, DEMO_STRIDE);
	if (Status != XST_SUCCESS)
	{
		printf("Display Ctrl initialization failed during demo initialization%d\r\n", Status);

	}
	Status = DisplayStart(&dispCtrl);
	if (Status != XST_SUCCESS)
	{
		printf("Couldn't start display during demo initialization%d\r\n", Status);

	}

	/**/
	/* open a new pic*/
	/**/
	pBuf8 = (UINT8*)malloc(0x800000); /* 8M*/
	
	sprintf((char*)(&fileName[0]), "%s", pic_file);	
	printf("\nPS start open pic_file, pls waiting ... ... \n\n");	
	
	fd = fopen((char*)(&fileName[0]), "r");  /* read*/
	if (fd > 0)
	{
		ret = fread(pBuf8, 1, 0x800000, fd);
	}	

	printf("hdmi show pic ... ... \n\n");	
	
	cnt = 0;
	
	while (1)
	{
		DemoPrintTest(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride, DEMO_PATTERN_0);
		taskDelay(180);
		
		DemoPrintTest(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride, DEMO_PATTERN_1);
		taskDelay(180);
		
		DemoPrintTest(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride, DEMO_PATTERN_2);
		taskDelay(180);
		
		DemoPrintTest(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride, DEMO_PATTERN_3);
		taskDelay(180);
		
		DemoPrintTest(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride, DEMO_PATTERN_5);
		taskDelay(180);

		/**/
		/* for new pic_file*/
		/**/
		DemoPrintTest2(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride, DEMO_PATTERN_4, pBuf8);
		taskDelay(180);

		
		/* end of test*/
		cnt++;
		if (cnt > 10)
		{
			DemoPrintTest(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride, DEMO_PATTERN_0);
			break;
		}
	}

	free(pBuf8);
	
	return 0;
}


extern void jpg_to_bmp_transfer(UINT8 * pSrc8, UINT8 * pDst8);
extern void jpg_to_bmp_transfer2(UINT8 * pSrc8, UINT8 * pDst8);

/**/
/* test jpg file without header: color is ok*/
/**/
int test_hdmi_jpg(char* pic_file)
{

	int Status;
	XAxiVdma_Config *vdmaConfig;
	int i;	
	int cnt = 0;
	
	UINT8 fileName[64] = {0};
	int fd = 0;
	UINT8 * pBuf8 = NULL;
	UINT8 * pBuf8_2 = NULL;
	UINT8 * pBuf8_3 = NULL;
	int ret = 0;

	/*
	 * Initialize an array of pointers to the 3 frame buffers
	 */
	for (i = 0; i < DISPLAY_NUM_FRAMES; i++)
	{
		pFrames[i] = frameBuf[i];
	}


	/**/
	/* reset 9134*/
	/**/
	gpio_hdmi_sii9134_rst();
	

	/**/
	/* enable sii9134 by i2c*/
	/**/
	i2c_slave_sii9134_init();


	/*
	 * Initialize VDMA driver
	 */
	vdmaConfig = XAxiVdma_LookupConfig(VGA_VDMA_ID);
	if (!vdmaConfig)
	{
		printf("No video DMA found for ID %d\r\n", VGA_VDMA_ID);

	}
	Status = XAxiVdma_CfgInitialize(&vdma, vdmaConfig, vdmaConfig->BaseAddress);
	if (Status != XST_SUCCESS)
	{
		printf("VDMA Configuration Initialization failed %d\r\n", Status);

	}

	/*
	 * Initialize the Display controller and start it
	 */
	Status = DisplayInitialize(&dispCtrl, &vdma, DISP_VTC_ID, DYNCLK_BASEADDR, pFrames, DEMO_STRIDE);
	if (Status != XST_SUCCESS)
	{
		printf("Display Ctrl initialization failed during demo initialization%d\r\n", Status);

	}
	Status = DisplayStart(&dispCtrl);
	if (Status != XST_SUCCESS)
	{
		printf("Couldn't start display during demo initialization%d\r\n", Status);

	}

	/**/
	/* open a new pic*/
	/**/
	pBuf8 = (UINT8*)malloc(0x800000); /* 8M*/
	pBuf8_2 = (UINT8*)malloc(0x800000); /* 8M*/
	
	sprintf((char*)(&fileName[0]), "%s", pic_file);	
	printf("\nPS start open pic_file, pls waiting ... ... \n\n");	
	
	fd = fopen((char*)(&fileName[0]), "r");  /* read*/
	if (fd > 0)
	{
		ret = fread(pBuf8, 1, 0x800000, fd);
	}	

	printf("jpg to bmp transfer ... \n\n");
	jpg_to_bmp_transfer2(pBuf8, pBuf8_2);

	pBuf8_3 = pBuf8_2 + 8;  /* 0X10,0X18,0X03,0X20,0X02,0X58,0X00,0X39, */

	printf("hdmi show pic ... ... \n\n");	
	
	cnt = 0;
	
	while (1)
	{
		DemoPrintTest(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride, DEMO_PATTERN_0);
		taskDelay(180);		

		/**/
		/* for new pic_file*/
		/**/
		DemoPrintTest2(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride, DEMO_PATTERN_4, pBuf8_3);
		taskDelay(180);

		
		/* end of test*/
		cnt++;
		if (cnt > 10)
		{
			DemoPrintTest(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride, DEMO_PATTERN_0);
			break;
		}
	}

	free(pBuf8);
	free(pBuf8_2);
	
	return 0;
}

/**/
/* test jpg file with header: color is some bug*/
/**/
int test_hdmi4(char* pic_file)
{

	int Status;
	XAxiVdma_Config *vdmaConfig;
	int i;	
	int cnt = 0;
	
	UINT8 fileName[64] = {0};
	int fd = 0;
	UINT8 * pBuf8 = NULL;
	UINT8 * pBuf8_2 = NULL;
	UINT8 * pBuf8_3 = NULL;
	int ret = 0;

	/*
	 * Initialize an array of pointers to the 3 frame buffers
	 */
	for (i = 0; i < DISPLAY_NUM_FRAMES; i++)
	{
		pFrames[i] = frameBuf[i];
	}


	/**/
	/* reset 9134*/
	/**/
	gpio_hdmi_sii9134_rst();
	

	/**/
	/* enable sii9134 by i2c*/
	/**/
	i2c_slave_sii9134_init();


	/*
	 * Initialize VDMA driver
	 */
	vdmaConfig = XAxiVdma_LookupConfig(VGA_VDMA_ID);
	if (!vdmaConfig)
	{
		printf("No video DMA found for ID %d\r\n", VGA_VDMA_ID);

	}
	Status = XAxiVdma_CfgInitialize(&vdma, vdmaConfig, vdmaConfig->BaseAddress);
	if (Status != XST_SUCCESS)
	{
		printf("VDMA Configuration Initialization failed %d\r\n", Status);

	}

	/*
	 * Initialize the Display controller and start it
	 */
	Status = DisplayInitialize(&dispCtrl, &vdma, DISP_VTC_ID, DYNCLK_BASEADDR, pFrames, DEMO_STRIDE);
	if (Status != XST_SUCCESS)
	{
		printf("Display Ctrl initialization failed during demo initialization%d\r\n", Status);

	}
	Status = DisplayStart(&dispCtrl);
	if (Status != XST_SUCCESS)
	{
		printf("Couldn't start display during demo initialization%d\r\n", Status);

	}

	/**/
	/* open a new pic*/
	/**/
	pBuf8 = (UINT8*)malloc(0x800000); /* 8M*/
	pBuf8_2 = (UINT8*)malloc(0x800000); /* 8M*/
	
	sprintf((char*)(&fileName[0]), "%s", pic_file);	
	printf("\nPS start open pic_file, pls waiting ... ... \n\n");	
	
	fd = fopen((char*)(&fileName[0]), "r");  /* read*/
	if (fd > 0)
	{
		ret = fread(pBuf8, 1, 0x800000, fd);
	}	

	printf("jpg to bmp transfer ... \n\n");
	jpg_to_bmp_transfer(pBuf8, pBuf8_2);
	
	pBuf8_3 = pBuf8_2 + 8;

	printf("hdmi show pic ... ... \n\n");	
	
	cnt = 0;
	
	while (1)
	{
		DemoPrintTest(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride, DEMO_PATTERN_0);
		taskDelay(180);		

		/**/
		/* for new pic_file*/
		/**/
		DemoPrintTest2(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride, DEMO_PATTERN_4, pBuf8_3);
		taskDelay(180);

		
		/* end of test*/
		cnt++;
		if (cnt > 10)
		{
			DemoPrintTest(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride, DEMO_PATTERN_0);
			break;
		}
	}

	free(pBuf8);
	free(pBuf8_2);
	
	return 0;
}

