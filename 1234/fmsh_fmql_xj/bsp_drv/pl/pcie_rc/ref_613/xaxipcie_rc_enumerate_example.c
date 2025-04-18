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
/****************************************************************************/
/**
* @file xaxipcie_rc_enumerate_example.c
*
* This file contains a design example for using AXI PCIe IP and its driver.
* This is an example to show the usage of driver APIs when AXI PCIe IP is
* configured as a Root Port.  The AXI PCIe can be configured as a Root Port
* only on the 7 Series Xilinx FPGA families.
*
* The example initialises the AXI PCIe IP and shows how to enumerate the PCIe
* system.
*
* @note
*
* This example should be used only when AXI PCIe IP is configured as
* root complex.
*
* This code will illustrate how the XAxiPcie IP and its standalone driver can
* be used to:
*	- Initialize a AXI PCIe IP core built as a root complex
*	- Enumerate PCIe end points in the system
*
* Please note that this example enumerates and initializes PCIe end points
* only. It does not shows how to deal with PCIe switches (and its virtual
* P2P bridges)
*
* We tried to use as much of the driver's API calls as possible to show the
* reader how each call could be used and that probably made the example not
* the shortest way of doing the tasks shown as they could be done.
*
*<pre>
* MODIFICATION HISTORY:
*
* Ver   Who  Date     Changes
* ----- ---- -------- -----------------------------------------------
* 2.00a nm   10/19/11 Initial version of AXI PCIe root complex example
* 2.02a nm   08/19/12 Removed the calls to XAxiPcie_GetLocalBusBar2PcieBar
*		      and XAxiPcie_SetLocalBusBar2PcieBar as they are not
*		      needed for enumeration
* 3.1   ms   01/23/17 Added printf statement in main function to
*                     ensure that "Successfully ran" and "Failed" strings
*                     are available in all examples. This is a fix for
*                     CR-965028.
*       ms   04/05/17 Added tabspace for return statements in functions
*                     for proper documentation while generating doxygen.
*
*</pre>
*****************************************************************************/

/***************************** Include Files ********************************/

#include "./axipcie_v3_1/xparameters.h"	/* Defines for XPAR constants */
#include "./axipcie_v3_1/xaxipcie.h"		/* XAxiPcie level 1 interface */

#include "stdio.h"

/*#include "printf.h" edit by YJ*/
/*#include "printf.h"*/

/************************** Constant Definitions ****************************/


#define AXIPCIE_DEVICE_ID 	XPAR_AXIPCIE_0_DEVICE_ID


/*
 * Command register offsets
 */
#define PCIE_CFG_CMD_IO_EN	0x00000001 /* I/O access enable */
#define PCIE_CFG_CMD_MEM_EN	0x00000002 /* Memory access enable */
#define PCIE_CFG_CMD_BUSM_EN	0x00000004 /* Bus master enable */
#define PCIE_CFG_CMD_PARITY	0x00000040 /* parity errors response */
#define PCIE_CFG_CMD_SERR_EN	0x00000100 /* SERR report enable */

/*
 * PCIe Configuration registers offsets
 */

#define PCIE_CFG_ID_REG			0x0000 /* Vendor ID/Device ID offset */
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
#define PCIE_CFG_BAR_0_REG		0x0004 /* PCIe Base Addr 0 */

#define PCIE_CFG_FUN_NOT_IMP_MASK	0xFFFF
#define PCIE_CFG_HEADER_TYPE_MASK	0x00EF0000
#define PCIE_CFG_MUL_FUN_DEV_MASK	0x00800000


#define PCIE_CFG_MAX_NUM_OF_BUS		256
#define PCIE_CFG_MAX_NUM_OF_DEV		1
#define PCIE_CFG_MAX_NUM_OF_FUN		8

#define PCIE_CFG_PRIM_SEC_BUS		0x00070100

#define PCIE_CFG_HEADER_O_TYPE		0x0000

#define PCIE_CFG_BAR_0_ADDR		0x00001111

/**************************** Type Definitions ******************************/


/***************** Macros (Inline Functions) Definitions ********************/


/************************** Function Prototypes *****************************/

int PcieInitRootComplex2(XAxiPcie *AxiPciePtr, u16 DeviceId);
void PCIeEnumerateFabric2(XAxiPcie *AxiPciePtr);


/************************** Variable Definitions ****************************/

/* Allocate PCIe Root Complex IP Instance */
XAxiPcie AxiPcieInstance2;

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
/*int main(void)*/
void test_pl_pcie_rc2(void)
{
	int Status;
	
	/* init_platform();	*/
	extern UINT32 slcr_read(UINT32);
	extern void slcr_write(UINT32, UINT32);
	if (0xF != slcr_read(0x838))
	{
		slcr_write(0x838, 0xF);
	}	
	if (0xF != slcr_read(0x83C))
	{
		slcr_write(0x83C, 0xF);
	}

    printf("Hello World\n\r");
	
	/* Initialize Root Complex */
	Status = PcieInitRootComplex2(&AxiPcieInstance2, AXIPCIE_DEVICE_ID);

	if (Status != XST_SUCCESS) 
	{
		printf("Axipcie rc enumerate Example Failed\r\n");
		return XST_FAILURE;
	}

	/* Scan PCIe Fabric */
	PCIeEnumerateFabric2(&AxiPcieInstance2);

	printf("Successfully ran Axipcie rc enumerate Example\r\n");
	return XST_SUCCESS;
}


void test_pcie_rc_613(void)
{
	extern int pl_pcie_rc_init(void);
	pl_pcie_rc_init();
}


/****************************************************************************/
/**
* This function initializes a AXI PCIe IP built as a root complex
*
* @param	AxiPciePtr is a pointer to an instance of XAxiPcie data
*		structure represents a root complex IP.
* @param 	DeviceId is AXI PCIe IP unique ID
*
* @return
*		- XST_SUCCESS if successful.
*		- XST_FAILURE if unsuccessful.
*
* @note 	None.
*
*
******************************************************************************/
int PcieInitRootComplex2(XAxiPcie *AxiPciePtr, u16 DeviceId)
{
	int Status;
	u32 HeaderData;
	u32 InterruptMask;
	u8  BusNumber;
	u8  DeviceNumber;
	u8  FunNumber;
	u8  PortNumber;

	XAxiPcie_Config *ConfigPtr;

	ConfigPtr = XAxiPcie_LookupConfig(DeviceId);

	Status = XAxiPcie_CfgInitialize(AxiPciePtr, ConfigPtr,
						ConfigPtr->BaseAddress);

	if (Status != XST_SUCCESS) {
		printf("Failed to initialize PCIe Root Complex"
							"IP Instance\r\n");
		return XST_FAILURE;
	}

	if(!AxiPciePtr->Config.IncludeRootComplex) {
		printf("Failed to initialize...AXI PCIE is configured"
							" as endpoint\r\n");
		return XST_FAILURE;
	}


	/* See what interrupts are currently enabled */
	XAxiPcie_GetEnabledInterrupts(AxiPciePtr, &InterruptMask);
	printf("Interrupts currently enabled are %8X\r\n", InterruptMask);

	/* Make sure all interrupts disabled. */
	XAxiPcie_DisableInterrupts(AxiPciePtr, XAXIPCIE_IM_ENABLE_ALL_MASK);


	/* See what interrupts are currently pending */
	XAxiPcie_GetPendingInterrupts(AxiPciePtr, &InterruptMask);
	printf("Interrupts currently pending are %8X\r\n", InterruptMask);

	/* Just if there is any pending interrupt then clear it.*/
	XAxiPcie_ClearPendingInterrupts(AxiPciePtr,
						XAXIPCIE_ID_CLEAR_ALL_MASK);

	/*
	 * Read enabled interrupts and pending interrupts
	 * to verify the previous two operations and also
	 * to test those two API functions
	 */

	XAxiPcie_GetEnabledInterrupts(AxiPciePtr, &InterruptMask);
	printf("Interrupts currently enabled are %8X\r\n", InterruptMask);

	XAxiPcie_GetPendingInterrupts(AxiPciePtr, &InterruptMask);
	printf("Interrupts currently pending are %8X\r\n", InterruptMask);

	/* Make sure link is up. */
	Status = XAxiPcie_IsLinkUp(AxiPciePtr);
	if (Status != TRUE )
	{
		printf("PCIE Link is not up\r\n");
		return XST_FAILURE;
	}

	printf("PCIE Link is up\r\n");

	/*
	 * Read back requester ID.
	 */
	XAxiPcie_GetRequesterId(AxiPciePtr, &BusNumber,
				&DeviceNumber, &FunNumber, &PortNumber);

	printf("Bus Number is %02X\r\n"
			"Device Number is %02X\r\n"
	 			"Function Number is %02X\r\n"
	 				"Port Number is %02X\r\n",
	 		BusNumber, DeviceNumber, FunNumber, PortNumber);


	/* Set up the PCIe header of this Root Complex */
	XAxiPcie_ReadLocalConfigSpace(AxiPciePtr,
					PCIE_CFG_CMD_STATUS_REG, &HeaderData);

	HeaderData |= (PCIE_CFG_CMD_BUSM_EN | PCIE_CFG_CMD_MEM_EN |
				PCIE_CFG_CMD_IO_EN | PCIE_CFG_CMD_PARITY |
							PCIE_CFG_CMD_SERR_EN);

	XAxiPcie_WriteLocalConfigSpace(AxiPciePtr,
					PCIE_CFG_CMD_STATUS_REG, HeaderData);

	/*
	 * Read back local config reg.
	 * to verify the write.
	 */

	XAxiPcie_ReadLocalConfigSpace(AxiPciePtr,
					PCIE_CFG_CMD_STATUS_REG, &HeaderData);

	printf("PCIe Local Config Space is %8X at register"
					" CommandStatus\r\n", HeaderData);

	/*
	 * Set up Bus number
	 */

	HeaderData = PCIE_CFG_PRIM_SEC_BUS;

	XAxiPcie_WriteLocalConfigSpace(AxiPciePtr,
					PCIE_CFG_PRI_SEC_BUS_REG, HeaderData);

	/*
	 * Read back local config reg.
	 * to verify the write.
	 */
	XAxiPcie_ReadLocalConfigSpace(AxiPciePtr,
					PCIE_CFG_PRI_SEC_BUS_REG, &HeaderData);

	printf("PCIe Local Config Space is %8X at register "
					"Prim Sec. Bus\r\n", HeaderData);

	/* Now it is ready to function */

	printf("Root Complex IP Instance has been successfully"
							" initialized\r\n");

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
* This function enumerates its PCIe system and figures out the nature of each
* component there like end points,bridges,...
*
* @param 	AxiPciePtr is a pointer to an instance of XAxiPcie
*		data structure represents a root complex IP.
*
* @return 	None.
*
* @note 	None.
*
******************************************************************************/
void PCIeEnumerateFabric2(XAxiPcie *AxiPciePtr)
{

	u32 ConfigData;
	u32 PCIeHeaderType;
	u32 PCIeMultiFun;
	u32 PCIeBusNum;
	u32 PCIeDevNum;
	u32 PCIeFunNum;
	u16 PCIeVendorID;
	u32 RegVal;
	u16 PCIeDevID;

	printf("Start Enumeration of PCIe Fabric on This System\r\n");

	/* Scan PCIe Fabric */

	for (PCIeBusNum = 0; PCIeBusNum < PCIE_CFG_MAX_NUM_OF_BUS; PCIeBusNum++) 
	{
		for (PCIeDevNum = 0; PCIeDevNum < PCIE_CFG_MAX_NUM_OF_DEV; PCIeDevNum++) 
		{
			for (PCIeFunNum = 0; PCIeFunNum < PCIE_CFG_MAX_NUM_OF_FUN; PCIeFunNum++) 
			{
				/* Vendor ID */
				XAxiPcie_ReadRemoteConfigSpace(
					AxiPciePtr,PCIeBusNum,
					PCIeDevNum, PCIeFunNum,
					PCIE_CFG_ID_REG, &ConfigData);

				PCIeVendorID = (u16) (ConfigData & 0xFFFF);
				PCIeDevID = (u16) ((ConfigData & 0xFFFF0000) >> 16);

				if (PCIeVendorID ==	PCIE_CFG_FUN_NOT_IMP_MASK) 
				{
					if (PCIeFunNum == 0)
						/*
						 * We don't need to look
						 * any further on this device.
						 */
						break;
				}
				else 
				{
					printf("PCIeBus is %02X\r\n"
						"PCIeDev is %02X\r\n"
						"PCIeFunc is %02X\r\n",
						PCIeBusNum, PCIeDevNum,
								PCIeFunNum);

					printf("Vendor ID is %04X \r\n",
								PCIeVendorID);
					printf("Dev ID is %04X \r\n",
								PCIeDevID);

					/* Header Type */
					XAxiPcie_ReadRemoteConfigSpace(
						AxiPciePtr, PCIeBusNum,
						PCIeDevNum, PCIeFunNum,
						PCIE_CFG_CAH_LAT_HD_REG,
						&ConfigData);

					PCIeHeaderType = ConfigData &
						PCIE_CFG_HEADER_TYPE_MASK;

					PCIeMultiFun = ConfigData &
						PCIE_CFG_MUL_FUN_DEV_MASK;

					if (PCIeHeaderType ==
						PCIE_CFG_HEADER_O_TYPE) {
						/* This is an End Point */
						printf("This is an "
							"End Point\r\n");

						/*
						 * Initialize this end point
						 * and return.
						 */

						XAxiPcie_ReadRemoteConfigSpace(
							AxiPciePtr,
							PCIeBusNum, PCIeDevNum,
							PCIeFunNum,
						PCIE_CFG_CMD_STATUS_REG,
								&ConfigData);

						ConfigData |=
						(PCIE_CFG_CMD_BUSM_EN |
							PCIE_CFG_CMD_MEM_EN);

						XAxiPcie_WriteRemoteConfigSpace
							(AxiPciePtr,
							PCIeBusNum, PCIeDevNum,
							PCIeFunNum,
						PCIE_CFG_CMD_STATUS_REG,
								ConfigData);

						/*
						 * Write Address to
						 * PCIe BAR0
						 */
						ConfigData =
						(PCIE_CFG_BAR_0_ADDR |
							PCIeBusNum |
							PCIeDevNum |
							PCIeFunNum);

						XAxiPcie_WriteRemoteConfigSpace
						(AxiPciePtr,
						PCIeBusNum, PCIeDevNum,
						PCIeFunNum, PCIE_CFG_BAR_0_REG,
						ConfigData);

						printf("End Point has been"
							" enabled\r\n");

							/* jc*/
					XAxiPcie_ReadRemoteConfigSpace(
						AxiPciePtr,
						PCIeBusNum, PCIeDevNum,
						PCIeFunNum,
						PCIE_CFG_BAR_0_REG,
							&ConfigData);
					printf("Bar Addr0: 0x%08X \r\n", ConfigData);

					}
					else {
						/* This is a bridge */
						printf("This is a "
								"Bridge\r\n");
					}
				}

				if ((!PCIeFunNum) && (!PCIeMultiFun)) {
					/*
					 * If it is function 0 and it is not a
					 * multi function device, we don't need
					 * to look any further on this devie
					 */
					break;
				}
			}  /* Functions in one device */
		}  /* Devices on the same bus */
	}  /* Buses in the same system */

	printf("End of Enumeration of PCIe Fabric on This system\r\n");

	/* Bridge enable */
	XAxiPcie_GetRootPortStatusCtrl(AxiPciePtr, &RegVal);
	RegVal |= XAXIPCIE_RPSC_BRIDGE_ENABLE_MASK;
	XAxiPcie_SetRootPortStatusCtrl(AxiPciePtr, RegVal);

	return;
}

