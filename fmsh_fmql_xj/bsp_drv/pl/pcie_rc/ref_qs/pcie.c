/*
 * main.c
 *
 *  Created on: 2020年5月5日
 *      Author: lenovo
 */
#include "vxWorks.h"
#include "stdio.h"
/******************************************************************************
*
* Copyright (C) 2011 - 2014 Xilinx, Inc.  All rights reserved.
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

/***************************** Include Files ********************************/

#include "PCIE_hw.h"	/* Defines for XPAR constants */

/*#define  INLUCDE_AFDX_ENABLE 1*/
/************************** Constant Definitions ****************************/

/*
 * AXICDMA Transfer Parameters. These have to be defined properly based
 * on the HW system.
 */
#define AXICDMA_SRC_ADDR	0x48000000	/* Source Address */
#define AXICDMA_DEST_ADDR	0xD0000000	/* Destination Address */
#define AXICDMA_LENGTH		0x400		/* Length */


/*
 * Command register offsets
 */
#define PCIE_CFG_CMD_IO_EN		0x00000001 /* I/O access enable */
#define PCIE_CFG_CMD_MEM_EN		0x00000002 /* Memory access enable */
#define PCIE_CFG_CMD_BUSM_EN	0x00000004 /* Bus master enable */
#define PCIE_CFG_CMD_PARITY		0x00000040 /* parity errors response */
#define PCIE_CFG_CMD_SERR_EN	0x00000100 /* SERR report enable */


/*
 * PCIe Configuration registers offsets
 */
#define PCIE_CFG_ID_REG				0x0000 /* Vendor ID/Device ID offset */
#define PCIE_CFG_CMD_STATUS_REG		0x0001 /*
											* Command/Status Register
											* Offset
											*/
#define PCIE_CFG_PRI_SEC_BUS_REG	0x0006 /*
											* Primary/Sec.Bus Register
											* Offset
											*/

#define PCIE_CFG_CAH_LAT_HD_REG		0x0003 /*
											* Cache Line/Latency Timer/
											* Header Type/
											* BIST Register Offset
											*/
#define PCIE_CFG_BAR_0_REG		    0x0004 /* PCIe Base Addr 0 */


#define PCIE_CFG_FUN_NOT_IMP_MASK	0xFFFF
#define PCIE_CFG_HEADER_TYPE_MASK	0x00EF0000
#define PCIE_CFG_MUL_FUN_DEV_MASK	0x00800000



#define PCIE_CFG_MAX_NUM_OF_DEV		1
#define PCIE_CFG_MAX_NUM_OF_FUN		8

#define PCIE_CFG_PRIM_SEC_BUS		0xFFFF0100

#define PCIE_CFG_HEADER_O_TYPE		0x0000


/**************************** Type Definitions ******************************/


/***************** Macros (Inline Functions) Definitions ********************/


/************************** Function Prototypes *****************************/

int PcieInitRootComplex();
void PCIeEnumerateFabric();
int DmaDataTransfer(u16 CdmaID);

int afdxComPortSndMsg(unsigned int portId,char * buff,unsigned int length);

/************************** Variable Definitions ****************************/

/* Allocate PCIe Root Complex IP Instance */

/*XAxiPcie_BarAddr BarAddr;*/

/* Allocate AXI CDMA IP Instance */
/*XAxiCdma CdmaInstance;*/
char afdxSendBuff[1024];
void afdxSendTest(int portId,int sendLength,int cnt)
{
	int sendResult = 0;
	int i;
	
	for(i=0;i<cnt;i++)
	{
#if INLUCDE_AFDX_ENABLE
		sendResult = afdxComPortSndMsg(portId,afdxSendBuff,sendLength);
#endif
		xil_printf("afdxComPortSndMsg port=%d,length=%d:%d\n",portId,sendLength,sendResult);
		taskDelay(100);
	}
}

#if 1  /* jc for test*/
void PCIE_AFDX_Reset_Write(int reg, int val)
{
	return;
}

void PCIE_AFDX_Read(int reg, UINT32* pVal)
{
	return;
}

void AFDX_Init(int addr)
{
	return;
}
#endif


/****************************************************************************/
/**
* This function is the entry point for PCIe Root Complex Enumeration Example
*
* @param 	None
*
* @return
*		- XST_SUCCESS if successful
*		- XST_FAILURE if unsuccessful.
*
* @note 	None.
*
*****************************************************************************/
void PCIE_AFDX_Init(uint32_t *returncode)
{
	int Status;	
	uint32_t temreg=0;
	
	
	PCIE_AFDX_Read(0, &temreg);  /* 判断IP核状态   */
	
	logMsg("temreg 0x%x\n",temreg,1,2,3,4,5);
	
	if (temreg != 1)
	{
		printf("PCIE AFDX IP ERROR!!\n");
		*returncode = 0x01;
	}
	else
	{
		/* Initialize Root Complex */
		Status = PcieInitRootComplex();
		
		if (Status != OK) 
		{			
			*returncode = PCIE_ERROR;			
			return ;
		}
		
		/* Scan PCIe Fabric */
		PCIeEnumerateFabric();
		taskDelay(1000);
		
		#if INLUCDE_AFDX_ENABLE
			AFDX_Init(XPAR_AXI_PCIE_AFDX_ADDR);
		#endif

		*returncode = PCIE_NO_ERROR;
		printf("PCIE AFDX Init Successfully!!\n");
	}

	return;
}


/*int8_t afdxCfgPath[64]="/tffs0/AFDX_CFG.bin";*/
void PCIE_AFDX_Init1(int8_t *afdxCfgPath, uint32_t *returncode)
{
	int Status;

	/* Initialize Root Complex */
	Status = PcieInitRootComplex();
	if (Status != OK) 
	{
		*returncode = PCIE_ERROR;
		return ;
	}
	
	/* Scan PCIe Fabric */
	PCIeEnumerateFabric();
	taskDelay(1000);

#if INLUCDE_AFDX_ENABLE	
	AFDX_Init(XPAR_AXI_PCIE_AFDX_ADDR,afdxCfgPath);	
#endif

	*returncode = PCIE_NO_ERROR;

	printf("PCIE AFDX Init1 Successfully!!\n");
	return;
}


#if 1  /* jc*/

void test_pcie_rc_init(void)
{
	int Status;

	/* Initialize Root Complex */
	Status = PcieInitRootComplex();
	if (Status != OK) 
	{
		printf("PcieInitRootComplex() fail! \n");
		return ;
	}
	
	/* Scan PCIe Fabric */
	PCIeEnumerateFabric();

	taskDelay(1000);
	printf("PCIE test_pcie_rc_init Successfully!!\n");
	return;
}


#endif
void PCIE_ZK_Reset(uint32_t *returncode)
{
	PCIE_AFDX_Reset_Write(1,0);  /* afdx子卡复位  */
	taskDelay(200);
	
	PCIE_AFDX_Reset_Write(1,1);
	*returncode = PCIE_NO_ERROR;
	return;
}

void PCIE_AFDX_Reset(uint32_t *returncode)
{

	PCIE_AFDX_Reset_Write(0,0);  /* ip复位  */
	taskDelay(200);
	
	PCIE_AFDX_Reset_Write(0,1);	
	taskDelay(1000);
	
	PCIE_ZK_Reset(&returncode);
	*returncode = PCIE_NO_ERROR;
	return;
}

/*****************************************************************************/
/**
* This function transfers data from Source Address to Destination Address
* using the AXI CDMA.
* User has to specify the Source Address, Destination Address and Transfer
* Length in AXICDMA_SRC_ADDR, AXICDMA_DEST_ADDR and AXICDMA_LENGTH defines
* respectively.
*
* @param	DeviceId is device ID of the XAxiCdma Device.
*
* @return
*		- XST_SUCCESS if successful
*		- XST_FAILURE.if unsuccessful.
*
* @note		If the hardware system is not built correctly this function
*		may never return to the caller.
*
******************************************************************************/
#if 0
int DmaDataTransfer (u16 DeviceID)
{
	int Status;
	volatile int Error;
	XAxiCdma_Config *ConfigPtr;

	Error = 0;

	/*
	 * Make sure we have a valid addresses for Src and Dst.
	 */
	if (AXICDMA_SRC_ADDR == 0) {
		return XST_FAILURE;
	}

	if (AXICDMA_DEST_ADDR == 0) {
		return XST_FAILURE;
	}

	/*
	 * Initialize the AXI CDMA IP.
	 */
	ConfigPtr = XAxiCdma_LookupConfig(DeviceID);
	if (ConfigPtr == NULL) {
		return XST_FAILURE;
	}

	Status = XAxiCdma_CfgInitialize(&CdmaInstance,
				ConfigPtr, ConfigPtr->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Reset the AXI CDMA device.
	 */
	XAxiCdma_Reset(&CdmaInstance);

	/*
	 * Disable AXI CDMA Interrupts
	 */
	XAxiCdma_IntrDisable(&CdmaInstance, XAXICDMA_XR_IRQ_ALL_MASK);

	/*
	 * Start Transferring Data from source to destination in polled mode
	 */
	XAxiCdma_SimpleTransfer (&CdmaInstance, AXICDMA_SRC_ADDR,
				AXICDMA_DEST_ADDR, AXICDMA_LENGTH, 0, 0);

	/*
	 * Poll Status register waiting for either Completion or Error
	 */
	while (XAxiCdma_IsBusy(&CdmaInstance));

	Error = XAxiCdma_GetError(&CdmaInstance);

	if (Error != 0x0) {

		xil_printf("AXI CDMA Transfer Error =  %8.8x\r\n");
		return XST_FAILURE;
	}

	xil_printf("AXI CDMA Transfer is Complete\r\n");


	return XST_SUCCESS;
}
#endif

