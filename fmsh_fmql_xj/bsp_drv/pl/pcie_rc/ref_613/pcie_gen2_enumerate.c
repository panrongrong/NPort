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
* Opsero Electronic Design Inc. 2017
*
* This application is a modified version of the example provided in the 
* Xilinx SDK 2017.1 at this location:
*
* \Xilinx\SDK\2017.1\data\embeddedsw\XilinxProcessorIPLib\drivers\axipcie_v3_1\examples\xaxipcie_rc_enumerate_example.c
* 
* The code demonstrates how to:
*	- Initialize a AXI Memory Mapped to PCIe IP core OR
*     AXI Bridge for PCIe Gen3 IP core built as a root complex
*   - Determine link speed and width once a link is established
*	- Enumerate PCIe end points in the system
*
*
*****************************************************************************/

/***************************** Include Files ********************************/

#include "./axipcie_v3_1/xparameters.h"	/* Defines for XPAR constants */
#include "./axipcie_v3_1/xaxipcie.h"		/* XAxiPcie level 1 interface */

#include "stdio.h"

/*#include "xil_printf.h"*/
/* jc*/
#define xil_printf  printf

/************************** Constant Definitions ****************************/


#define AXIPCIE_DEVICE_ID 	XPAR_AXIPCIE_0_DEVICE_ID


/*
 * Command register offsets
 */
#define PCIE_CFG_CMD_IO_EN      0x00000001 /* I/O access enable */
#define PCIE_CFG_CMD_MEM_EN     0x00000002 /* Memory access enable */
#define PCIE_CFG_CMD_BUSM_EN    0x00000004 /* Bus master enable */
#define PCIE_CFG_CMD_PARITY     0x00000040 /* parity errors response */
#define PCIE_CFG_CMD_SERR_EN    0x00000100 /* SERR report enable */

/*
 * PCIe Configuration registers offsets
 */

#define PCIE_CFG_ID_REG			    0x0000 /* Vendor ID/Device ID offset */
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

#define PCIE_CFG_BAR_1_REG		0x0005 /* PCIe Base Addr 1 : add by jc */

#define PCIE_CFG_FUN_NOT_IMP_MASK	0xFFFF
#define PCIE_CFG_HEADER_TYPE_MASK	0x00EF0000
#define PCIE_CFG_MUL_FUN_DEV_MASK	0x00800000


#define PCIE_CFG_MAX_NUM_OF_BUS		256  /* 256 -> 8 -> 256 */
#define PCIE_CFG_MAX_NUM_OF_DEV		32  /* jc: 1->32*/
#define PCIE_CFG_MAX_NUM_OF_FUN		8

#define PCIE_CFG_PRIM_SEC_BUS		0x00070100

#define PCIE_CFG_HEADER_O_TYPE		0x0000

#define PCIE_CFG_BAR_0_ADDR		0x00001111

/*
* Macros for reading link speed and width from the core
*/

#define	 XAxiPcie_IsGen3(InstancePtr) 	\
	(XAxiPcie_ReadReg((InstancePtr)->Config.BaseAddress, 	\
	XAXIPCIE_PHYSC_OFFSET) & 0x00001000) ? 1 : 0

#define	 XAxiPcie_IsGen2(InstancePtr) 	\
	(XAxiPcie_ReadReg((InstancePtr)->Config.BaseAddress, 	\
	XAXIPCIE_PHYSC_OFFSET) & 0x00000001) ? 1 : 0

#define	 XAxiPcie_LinkWidth(InstancePtr) 	\
	((XAxiPcie_ReadReg((InstancePtr)->Config.BaseAddress, 	\
	XAXIPCIE_PHYSC_OFFSET) & XAXIPCIE_PHYSC_LINK_WIDTH_MASK) >> 1)



/**************************** Type Definitions ******************************/


/***************** Macros (Inline Functions) Definitions ********************/


/************************** Function Prototypes *****************************/

int PcieInitRootComplex(XAxiPcie *AxiPciePtr, UINT16 DeviceId);
void PCIeEnumerateFabric(XAxiPcie *AxiPciePtr);

/* jc*/
void PCIeEnumerateFabric_2(XAxiPcie *AxiPciePtr, int bus, int dev, int fun);

static void __attribute__ ((noinline)) UtilDelay(unsigned int Seconds);

/************************** Variable Definitions ****************************/

/* Allocate PCIe Root Complex IP Instance */
XAxiPcie AxiPcieInstance;
XAxiPcie_BarAddr BarAddr;  /* jc*/

/****************************************************************************/
/**
* This function is the entry point for PCIe Root Complex Enumeration Example
*
* @param 	None
*
* @return 	- XST_SUCCESS if successful
*		- XST_FAILURE if unsuccessful.
*
* @note 	None.
*
*****************************************************************************/
int pl_pcie_rc_init(void)
{
	int Status;

	/* Allow time for link-up*/
	/*UtilDelay(1);*/
	
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

	xil_printf("=============================\r\n");
	xil_printf("PCIe Gen2 Enumeration Example\r\n");
	xil_printf("=============================\r\n");

	taskDelay(10);
	
	/* Initialize Root Complex */
	Status = PcieInitRootComplex(&AxiPcieInstance, AXIPCIE_DEVICE_ID);

	if (Status != XST_SUCCESS) 
	{
		xil_printf("Failed to initialize AXI PCIe Root port\r\n");
		return XST_FAILURE;
	}

	/* 
	Scan PCIe Fabric 
	*/
#if 0  /* origin*/
	PCIeEnumerateFabric(&AxiPcieInstance);

#else  /* jc: for scan the 2nd pcie-bridge*/
	PCIeEnumerateFabric_2(&AxiPcieInstance, 0, 0, 0);
#endif
	
	
	xil_printf("==>pcie_link_speed: Gen-%d \n", get_pcie_link_speed(&AxiPcieInstance));
	xil_printf("==>pcie_link_width: X-%d \n", get_pcie_link_width(&AxiPcieInstance));

	return XST_SUCCESS;
}

void test_pl_pcie_rc(void)
{
	pl_pcie_rc_init();
}


int pl_pcie_rc_init_2(void)
{
	int Status;

	/* Allow time for link-up*/
	/*UtilDelay(1);*/
	
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

	xil_printf("=============================\r\n");
	xil_printf("PCIe Gen2 Enumeration Example\r\n");
	xil_printf("=============================\r\n");

	taskDelay(10);
	
	/* Initialize Root Complex */
	Status = PcieInitRootComplex(&AxiPcieInstance, AXIPCIE_DEVICE_ID);

	if (Status != XST_SUCCESS) 
	{
		xil_printf("Failed to initialize AXI PCIe Root port\r\n");
		return XST_FAILURE;
	}

	/* 
	Scan PCIe Fabric 
	*/
#if 0  /* origin*/
	PCIeEnumerateFabric(&AxiPcieInstance);

#else  /* jc: for scan the 2nd pcie-bridge*/
	/*PCIeEnumerateFabric_2(&AxiPcieInstance, 0, 0, 0);*/
#endif
	
	
	/*xil_printf("==>pcie_link_speed: Gen-%d \n", get_pcie_link_speed(&AxiPcieInstance));*/
	/*xil_printf("==>pcie_link_width: X-%d \n", get_pcie_link_width(&AxiPcieInstance));*/

	return XST_SUCCESS;
}

/****************************************************************************/
/**
* This function returns the negotiated PCIe link speed once link-up is achieved
*
* @param	AxiPciePtr is a pointer to an instance of XAxiPcie data
*		structure represents a root complex IP.
* @return	- 1 if Gen1
*           - 2 if Gen2
*           - 3 if Gen3
*		- 0 if unsuccessful.
*
* @note 	None.
*
*
******************************************************************************/

int get_pcie_link_speed(XAxiPcie *AxiPciePtr)
{
	int is_gen2;
	int is_gen3;
	
	is_gen2 = XAxiPcie_IsGen2(AxiPciePtr);
	is_gen3 = XAxiPcie_IsGen3(AxiPciePtr);
	
	if ((is_gen2 == 0) && (is_gen3 == 1))
		return(3);
	
	if ((is_gen2 == 1) && (is_gen3 == 0))
		return(2);
	
	if ((is_gen2 == 0) && (is_gen3 == 0))
		return(1);
	
	return(0);
}

/****************************************************************************/
/**
* This function returns the negotiated PCIe link width once link-up is achieved
*
* @param	AxiPciePtr is a pointer to an instance of XAxiPcie data
*		structure represents a root complex IP.
* @return	- link width (1,2,4 or 8)
*
* @note 	None.
*
*
******************************************************************************/

int get_pcie_link_width(XAxiPcie *AxiPciePtr)
{
	int i;
	int link_width;
	int result;
	
	link_width = XAxiPcie_LinkWidth(AxiPciePtr);
	
	result = 1;
	
	for (i = 0; i < link_width; i++)
	{	
		result = result * 2;
	}
	
	return(result);
}


/****************************************************************************/
/**
* This function initializes a AXI PCIe IP built as a root complex
*
* @param	AxiPciePtr is a pointer to an instance of XAxiPcie data
*		structure represents a root complex IP.
* @param 	DeviceId is AXI PCIe IP unique ID
*
* @return	- XST_SUCCESS if successful.
*		- XST_FAILURE if unsuccessful.
*
* @note 	None.
*
*
******************************************************************************/
int PcieInitRootComplex(XAxiPcie *AxiPciePtr, UINT16 DeviceId)
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
	printf("XAxiPcie_LookupConfig end \n");
	
	Status = XAxiPcie_CfgInitialize(AxiPciePtr, ConfigPtr, ConfigPtr->BaseAddress);
	printf("XAxiPcie_CfgInitialize end \n");

	if (Status != XST_SUCCESS) 
	{
		xil_printf("Failed to initialize PCIe Root Complex"
							"IP Instance\r\n");
		return XST_FAILURE;
	}

	if(!AxiPciePtr->Config.IncludeRootComplex) 
	{
		xil_printf("Failed to initialize...AXI PCIE is configured"
							" as endpoint\r\n");
		return XST_FAILURE;
	}
	

	/*A2P_CSR_WRITE_4 (pDev, A2P_PCIE_CFG_COMMAND, 0x00000004);*/
	/*val = A2P_CSR_READ_4 (pDev, A2P_PCIE_CFG_COMMAND);*/
	UINT32 val, data;
	
	/*XAxiPcie_WriteLocalConfigSpace(AxiPciePtr, PCIE_CFG_CMD_STATUS_REG, 0x00000004);*/
	XAxiPcie_WriteReg((AxiPciePtr->Config.BaseAddress), (PCIE_CFG_CMD_STATUS_REG*4), 0x00000004);
	/*val = XAxiPcie_ReadLocalConfigSpace(AxiPciePtr, PCIE_CFG_CMD_STATUS_REG, &data);*/
	val = XAxiPcie_ReadReg((AxiPciePtr->Config.BaseAddress), (PCIE_CFG_CMD_STATUS_REG*4));
	

/* See what interrupts are currently enabled */
	XAxiPcie_GetEnabledInterrupts(AxiPciePtr, &InterruptMask);
	xil_printf("Interrupts currently enabled are %8X\r\n", InterruptMask);

	/* Make sure all interrupts disabled. */
	XAxiPcie_DisableInterrupts(AxiPciePtr, XAXIPCIE_IM_ENABLE_ALL_MASK);


	/* See what interrupts are currently pending */
	XAxiPcie_GetPendingInterrupts(AxiPciePtr, &InterruptMask);
	xil_printf("Interrupts currently pending are %8X\r\n", InterruptMask);

	/* Just if there is any pending interrupt then clear it.*/
	XAxiPcie_ClearPendingInterrupts(AxiPciePtr,	XAXIPCIE_ID_CLEAR_ALL_MASK);

	/*
	 * Read enabled interrupts and pending interrupts
	 * to verify the previous two operations and also
	 * to test those two API functions
	 */
	XAxiPcie_GetEnabledInterrupts(AxiPciePtr, &InterruptMask);
	xil_printf("Interrupts currently enabled are %8X\r\n", InterruptMask);

	XAxiPcie_GetPendingInterrupts(AxiPciePtr, &InterruptMask);
	xil_printf("Interrupts currently pending are %8X\r\n", InterruptMask);


    /* 
    Enable the bridge 
	*/
/*#define A2P_ROOT_PORT_BRIDGE_ENABLE     (0x00000001)*/
	val = XAxiPcie_ReadReg((AxiPciePtr->Config.BaseAddress), XAXIPCIE_RPSC_OFFSET);
    val |= 0x00000001;
	XAxiPcie_WriteReg((AxiPciePtr->Config.BaseAddress), XAXIPCIE_RPSC_OFFSET, val);

    /* 
    Update the address translation register 
	*/
    /*A2P_CSR_WRITE_4 (pDev, AXIBAR2PCIEBAR_0L, pDrvCtrl->memIo32Addr);*/
	/*XAxiPcie_WriteReg((AxiPciePtr->Config.BaseAddress), XAXIPCIE_AXIBAR2PCIBAR_0L_OFFSET, 0x60000000);*/
	XAxiPcie_WriteReg((AxiPciePtr->Config.BaseAddress), XAXIPCIE_AXIBAR2PCIBAR_0L_OFFSET, XPAR_AXI_PCIE_0_AXIBAR2PCIEBAR_0);


#if 1
	XAxiPcie_GetLocalBusBar2PcieBar(AxiPciePtr, 0, &BarAddr);
	xil_printf("BarAddr------- 0x%08X(high) 0x%08X(low) -------- \r\n", BarAddr.UpperAddr,BarAddr.LowerAddr);
	XAxiPcie_SetLocalBusBar2PcieBar(AxiPciePtr, 0, &BarAddr);

	/* 
	Make sure link is up. 
	*/
	Status = XAxiPcie_IsLinkUp(AxiPciePtr);
	if (Status != TRUE ) 
	{
		xil_printf("Link:\r\n  - LINK NOT UP!\r\n");
		return XST_FAILURE;
	}
	printf("XAxiPcie_IsLinkUp end \n");
#endif	

	xil_printf("Link:\r\n  - LINK UP, Gen%d - X%d lanes \n\n",
		get_pcie_link_speed(AxiPciePtr),get_pcie_link_width(AxiPciePtr));


	/*
	 * Read back requester ID.
	 */
	XAxiPcie_GetRequesterId(AxiPciePtr, &BusNumber,	&DeviceNumber, &FunNumber, &PortNumber);

	xil_printf("Requester ID:\r\n");
	xil_printf("  - Bus Number     : %02X\r\n"
			   "  - Device Number  : %02X\r\n"
	 		   "  - Function Number: %02X\r\n"
	 	       "  - Port Number    : %02X\r\n",
	 		BusNumber, DeviceNumber, FunNumber, PortNumber);


	/* Set up the PCIe header of this Root Complex */
	XAxiPcie_ReadLocalConfigSpace(AxiPciePtr, PCIE_CFG_CMD_STATUS_REG, &HeaderData);

	HeaderData |= (PCIE_CFG_CMD_BUSM_EN | PCIE_CFG_CMD_MEM_EN |
				PCIE_CFG_CMD_IO_EN | PCIE_CFG_CMD_PARITY |
							PCIE_CFG_CMD_SERR_EN);

	XAxiPcie_WriteLocalConfigSpace(AxiPciePtr, PCIE_CFG_CMD_STATUS_REG, HeaderData);

	/*
	 * Read back local config reg.
	 * to verify the write.
	 */

	xil_printf("PCIe Local Config Space:\r\n");

	XAxiPcie_ReadLocalConfigSpace(AxiPciePtr, PCIE_CFG_CMD_STATUS_REG, &HeaderData);

	xil_printf("  - 0x%08X at register CommandStatus\r\n", HeaderData);

	/*
	 * Set up Bus number
	 */
	HeaderData = PCIE_CFG_PRIM_SEC_BUS;
	XAxiPcie_WriteLocalConfigSpace(AxiPciePtr, PCIE_CFG_PRI_SEC_BUS_REG, HeaderData);
	/*
	 * Read back local config reg.
	 * to verify the write.
	 */
	XAxiPcie_ReadLocalConfigSpace(AxiPciePtr, PCIE_CFG_PRI_SEC_BUS_REG, &HeaderData);
	xil_printf("  - 0x%08X at register Prim Sec. Bus\r\n", HeaderData);

	/* Now it is ready to function */
	xil_printf("Root Complex IP Instance has been successfully"
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
void PCIeEnumerateFabric(XAxiPcie *AxiPciePtr)
{
	u32 ConfigData;
	u32 PCIeHeaderType;
	u32 PCIeMultiFun;
	u32 PCIeBusNum;
	u32 PCIeDevNum;
	u32 PCIeFunNum;
	UINT16 PCIeVendorID;
	
	UINT16 PCIeDevID;  /* jc*/
	
	u32 RegVal;
	u32 tmp32 = 0;

	xil_printf("Start Enumeration of PCIe Fabric on This System\r\n");


	/* 
	Scan PCIe Fabric 
	*/
	for (PCIeBusNum = 0; PCIeBusNum < PCIE_CFG_MAX_NUM_OF_BUS;	PCIeBusNum++) 
	{
		for (PCIeDevNum = 0; PCIeDevNum < PCIE_CFG_MAX_NUM_OF_DEV;	PCIeDevNum++) 
		{
			for (PCIeFunNum = 0; PCIeFunNum < PCIE_CFG_MAX_NUM_OF_FUN;	PCIeFunNum++) 
			{
				/* Vendor ID */
				XAxiPcie_ReadRemoteConfigSpace(
					AxiPciePtr,PCIeBusNum,
					PCIeDevNum, PCIeFunNum,
					PCIE_CFG_ID_REG, &ConfigData);

				PCIeVendorID = (UINT16) (ConfigData & 0xFFFF);
				PCIeDevID = (UINT16) ((ConfigData & 0xFFFF0000) >> 16);  /* jc*/
				
				if (PCIeVendorID ==	PCIE_CFG_FUN_NOT_IMP_MASK) 
				{
					if (PCIeFunNum == 0)
					{	/*
						 * We don't need to look
						 * any further on this device.
						 */
						break;
					}
				}
				else 
				{
					xil_printf("PCIeBus %02X:\r\n"
						"  - PCIeDev: %02X\r\n"
						"  - PCIeFunc: %02X\r\n",
						PCIeBusNum, PCIeDevNum,
								PCIeFunNum);

					xil_printf("  - Vendor ID: %04X, Dev ID: %04X \r\n",
								PCIeVendorID, PCIeDevID);

					/* Header Type */
					XAxiPcie_ReadRemoteConfigSpace(
						AxiPciePtr, PCIeBusNum,
						PCIeDevNum, PCIeFunNum,
						PCIE_CFG_CAH_LAT_HD_REG,
						&ConfigData);

					PCIeHeaderType = ConfigData & PCIE_CFG_HEADER_TYPE_MASK;
					PCIeMultiFun   = ConfigData & PCIE_CFG_MUL_FUN_DEV_MASK;

					if (PCIeHeaderType == PCIE_CFG_HEADER_O_TYPE) 
					{
						/* This is an End Point */
						xil_printf("  - End Point\r\n");

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
							PCIE_CFG_CMD_MEM_EN | PCIE_CFG_CMD_IO_EN/*jc*/);

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
					#if 1
						ConfigData =
						(PCIE_CFG_BAR_0_ADDR |
							PCIeBusNum |
							PCIeDevNum |
							PCIeFunNum);
					#else	/* for test*/
						 ConfigData=0x80000000;
					#endif

						XAxiPcie_WriteRemoteConfigSpace
						(AxiPciePtr,
						PCIeBusNum, PCIeDevNum,
						PCIeFunNum, PCIE_CFG_BAR_0_REG,
						ConfigData);

						/**/
						/* jc: it's very important for ep_bar0_address*/
						/**/
						/* auto_write ep_bar0-reg_address by pl_pcie_rc_scan()*/
						/**/
					#if 0  /* 0-scan&auto setup, 1-fixed setup at here*/
						XAxiPcie_WriteRemoteConfigSpace
						(AxiPciePtr,
						PCIeBusNum, PCIeDevNum,
						PCIeFunNum, PCIE_CFG_BAR_0_REG,
						XPAR_AXI_PCIE_0_AXIBAR_0);
					#endif

						xil_printf("  - End Point has been"
							" enabled\r\n");
						
				#if 1
					int reg = 0;
					for (reg=0; reg<16; reg++)
					{
						/* jc*/
						XAxiPcie_ReadRemoteConfigSpace(
							AxiPciePtr, PCIeBusNum,
							PCIeDevNum, PCIeFunNum,
							reg,
							&ConfigData);
						xil_printf("    -- PCIeHeader Reg_%d: 0x%08X \r\n", reg, ConfigData);
					}
						
						XAxiPcie_ReadRemoteConfigSpace(
							AxiPciePtr, PCIeBusNum,
							PCIeDevNum, PCIeFunNum,
							0x4,
							&ConfigData);  /* 0x04* = 0x10-offset*/
						xil_printf("  - BAR(04)_0x10: 0x%08X \r\n", ConfigData);
					/*	*/
				#endif	
					}
					else 
					{
						/* This is a bridge */
						xil_printf("  - Bridge\r\n");	
						
					#if 0 /* jc: 0-scan&auto setup, 1-fixed setup at here*/
						/* write 0x20- 0x60006000*/
						tmp32 = (XPAR_AXI_PCIE_0_AXIBAR_0 & 0xFFFF0000) | ((XPAR_AXI_PCIE_0_AXIBAR_0 & 0xFFFF0000) >> 16);
						/*
						XAxiPcie_WriteRemoteConfigSpace
							(AxiPciePtr,
							PCIeBusNum, PCIeDevNum,
							PCIeFunNum, 0x08,
							tmp32);
						*/
						XAxiPcie_WriteLocalConfigSpace(&AxiPcieInstance, 0x08, tmp32);
					#endif
						
					#if 1
                        int reg2 = 0;
                        for (reg2=0; reg2<16; reg2++)
                        {
                            /* jc*/
                            XAxiPcie_ReadRemoteConfigSpace(
                                    AxiPciePtr, PCIeBusNum,
                                    PCIeDevNum, PCIeFunNum,
                                    reg2,
                                    &ConfigData);
                            xil_printf("    -- PCIeHeader Reg_%d: 0x%08X \r\n", reg2, ConfigData);
                        }
                                
                        XAxiPcie_ReadRemoteConfigSpace(
                                AxiPciePtr, PCIeBusNum,
                                PCIeDevNum, PCIeFunNum,
                                4/*0x10*/,
                                &ConfigData);
                        xil_printf("  - BAR 0x10: 0x%08X \r\n", ConfigData);
                        /*	*/
					#endif		
					}
				}

				if ((!PCIeFunNum) && (!PCIeMultiFun))
				{
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

	xil_printf("End of Enumeration of PCIe Fabric on This system\r\n");

	/* Bridge enable */
	XAxiPcie_GetRootPortStatusCtrl(AxiPciePtr, &RegVal);
	RegVal |= XAXIPCIE_RPSC_BRIDGE_ENABLE_MASK;
	XAxiPcie_SetRootPortStatusCtrl(AxiPciePtr, RegVal);

	return;
}

void PCIeEnumerateFabric_2(XAxiPcie *AxiPciePtr, int bus_no, int dev_no, int fun_no)
{
	u32 ConfigData;
	u32 PCIeHeaderType;
	
	u32 PCIeMultiFun;	
	u32 PCIeBusNum, PCIeDevNum, PCIeFunNum;
	
	UINT16 PCIeVendorID;	
	UINT16 PCIeDevID;  /* jc*/

	/*/////////////////////////////////////////////////*/
	u32 ConfigData_2;
	u32 PCIeHeaderType_2;
	
	u32 PCIeMultiFun_2;	
	u32 PCIeBusNum_2, PCIeDevNum_2, PCIeFunNum_2;
	
	UINT16 PCIeVendorID_2;	
	UINT16 PCIeDevID_2;  /* jc*/
	/*/////////////////////////////////////////////////*/
	
	u32 RegVal;
	u32 tmp32 = 0;

	xil_printf("Start Enumeration of PCIe Fabric on This System: bus-%d,dev-%d,fun-%d \r\n", \
		        bus_no, dev_no, fun_no);


	/* 
	Scan PCIe Fabric 
	*/
	for (PCIeBusNum = bus_no; PCIeBusNum < PCIE_CFG_MAX_NUM_OF_BUS;	PCIeBusNum++) 
	{
		for (PCIeDevNum = dev_no; PCIeDevNum < PCIE_CFG_MAX_NUM_OF_DEV;	PCIeDevNum++) 
		/*for (PCIeDevNum = dev_no; PCIeDevNum < 0x10; PCIeDevNum++) */
		{
			/* jc: test for 613*/
			/*
			if ((PCIeDevNum == 0) || ((PCIeDevNum == 0xF) && (PCIeBusNum == 2)))
			{
				// going on
			}
			else
			{
				continue;
			}	
			*/
			
			for (PCIeFunNum = fun_no; PCIeFunNum < PCIE_CFG_MAX_NUM_OF_FUN;	PCIeFunNum++) 
			{
			
				//printf("init pcie-%d-%d-%d ... \n", PCIeBusNum, PCIeDevNum, PCIeFunNum);
				
				/* Vendor ID */
				XAxiPcie_ReadRemoteConfigSpace(
					AxiPciePtr,PCIeBusNum,
					PCIeDevNum, PCIeFunNum,
					PCIE_CFG_ID_REG, &ConfigData);
				
				if (ConfigData != 0xFFFFFFFF)
				{
					printf("init pcie-%d-%d-%d ... \n", PCIeBusNum, PCIeDevNum, PCIeFunNum);
					printf("read pcie-%d-%d-%d : reg-0=0x%08X \n\n", PCIeBusNum, PCIeDevNum, PCIeFunNum, ConfigData);
				}

				PCIeVendorID = (UINT16) (ConfigData & 0xFFFF);
				PCIeDevID = (UINT16) ((ConfigData & 0xFFFF0000) >> 16);  /* jc*/
				
				if (PCIeVendorID ==	PCIE_CFG_FUN_NOT_IMP_MASK) 
				{
					if (PCIeFunNum == 0)
					{	/*
						 * We don't need to look
						 * any further on this device.
						 */
						break;
					}
				}
				else 
				{
					xil_printf("PCIeBus %02X:\r\n"
								"  - PCIeDev: %02X\r\n"
								"  - PCIeFunc: %02X\r\n",
								PCIeBusNum, PCIeDevNum,
								PCIeFunNum);

					xil_printf("  - Vendor ID: %04X, Dev ID: %04X \r\n",
								PCIeVendorID, PCIeDevID);

					/* Header Type */
					XAxiPcie_ReadRemoteConfigSpace(
						AxiPciePtr, PCIeBusNum,
						PCIeDevNum, PCIeFunNum,
						PCIE_CFG_CAH_LAT_HD_REG,
						&ConfigData);

					PCIeHeaderType = ConfigData & PCIE_CFG_HEADER_TYPE_MASK;
					PCIeMultiFun   = ConfigData & PCIE_CFG_MUL_FUN_DEV_MASK;

					if (PCIeHeaderType == PCIE_CFG_HEADER_O_TYPE)   /* endpoint*/
					{
						/* This is an End Point */
						xil_printf("  - End Point\r\n");

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
							PCIE_CFG_CMD_MEM_EN | PCIE_CFG_CMD_IO_EN/*jc*/);			

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
					#if 1
						ConfigData =
						(PCIE_CFG_BAR_0_ADDR |
							PCIeBusNum |
							PCIeDevNum |
							PCIeFunNum);
					#else	/* for test*/
						 ConfigData=0x80000000;
					#endif

						XAxiPcie_WriteRemoteConfigSpace
						(AxiPciePtr,
						PCIeBusNum, PCIeDevNum,
						PCIeFunNum, PCIE_CFG_BAR_0_REG,
						ConfigData);

						/**/
						/* jc: it's very important for ep_bar0_address*/
						/**/
						/* auto_write ep_bar0-reg_address by pl_pcie_rc_scan()*/
						/**/
					#if 1  /* 0-scan&auto setup, 1-fixed setup at here*/
						XAxiPcie_WriteRemoteConfigSpace
						(AxiPciePtr,
						PCIeBusNum, PCIeDevNum,
						PCIeFunNum, PCIE_CFG_BAR_0_REG,
						XPAR_AXI_PCIE_0_AXIBAR_0);    /* bar0: 0x6000_0000*/
					
						XAxiPcie_WriteRemoteConfigSpace
						(AxiPciePtr,
						PCIeBusNum, PCIeDevNum,
						PCIeFunNum, PCIE_CFG_BAR_1_REG,
						(XPAR_AXI_PCIE_0_AXIBAR_0+0x00010000));  /*  bar1: 0x6001_0000					*/
					#endif

						xil_printf("  - End Point has been"
							" enabled\r\n");
						
				#if 1 
					int reg = 0;
					for (reg=0; reg<16; reg++)
					{
						/* jc*/
						XAxiPcie_ReadRemoteConfigSpace(
							AxiPciePtr, PCIeBusNum,
							PCIeDevNum, PCIeFunNum,
							reg,
							&ConfigData);
						xil_printf("    -- PCIeHeader Reg_%d: 0x%08X \r\n", reg, ConfigData);
					}
						
						XAxiPcie_ReadRemoteConfigSpace(
							AxiPciePtr, PCIeBusNum,
							PCIeDevNum, PCIeFunNum,
							0x4,
							&ConfigData);  /* 0x04* = 0x10-offset*/
						xil_printf("  - BAR(04)_0x10: 0x%08X \r\n", ConfigData);
					/*	*/
				#endif	
					}
					else  /* bridge*/
					{
						/* This is a bridge */
						xil_printf("  - Bridge:(%d-%d-%d) \r\n", PCIeBusNum, PCIeDevNum, PCIeFunNum);	
						
					
						/* read*/
						XAxiPcie_ReadRemoteConfigSpace(AxiPciePtr,
							PCIeBusNum, PCIeDevNum, PCIeFunNum,
							PCIE_CFG_CMD_STATUS_REG, &ConfigData); 					
						printf("XAxiPcie_ReadRemoteConfigSpace:(%d-%d-%d)=> (1):0x%08X \r\n", PCIeBusNum, PCIeDevNum, PCIeFunNum, ConfigData);

						ConfigData |= (PCIE_CFG_CMD_BUSM_EN | PCIE_CFG_CMD_MEM_EN | PCIE_CFG_CMD_IO_EN);	
						ConfigData &= ~PCI_CMD_INTX_DISABLE;	
						
						XAxiPcie_WriteRemoteConfigSpace (AxiPciePtr,
							PCIeBusNum, PCIeDevNum, PCIeFunNum,
							PCIE_CFG_CMD_STATUS_REG, ConfigData);	
							
						/* read*/
						XAxiPcie_ReadRemoteConfigSpace(AxiPciePtr,
							PCIeBusNum, PCIeDevNum, PCIeFunNum,
							PCIE_CFG_CMD_STATUS_REG, &ConfigData); 					
						printf("XAxiPcie_ReadRemoteConfigSpace:(%d-%d-%d)=> (1):0x%08X \r\n", PCIeBusNum, PCIeDevNum, PCIeFunNum, ConfigData);
						
						ConfigData |= (0xFFFF << 16);
						
						XAxiPcie_WriteRemoteConfigSpace (AxiPciePtr,
							PCIeBusNum, PCIeDevNum, PCIeFunNum,
							PCIE_CFG_CMD_STATUS_REG, ConfigData);							
						
					#if 1 /* jc: 0-scan&auto setup, 1-fixed setup at here*/
						if (PCIeBusNum == 0)  /* root*/
						{
							/* write 0x20- 0x60006000 - brige*/
							tmp32 = (XPAR_AXI_PCIE_0_AXIBAR_0 & 0xFFFF0000) | ((XPAR_AXI_PCIE_0_AXIBAR_0 & 0xFFFF0000) >> 16);
							/*
							XAxiPcie_WriteRemoteConfigSpace
								(AxiPciePtr,
								PCIeBusNum, PCIeDevNum,
								PCIeFunNum, 0x08,
								tmp32);
							*/
							XAxiPcie_WriteLocalConfigSpace(&AxiPcieInstance, (0x20/4), tmp32);
						}
					#endif

					
					#if 0
                        int reg2 = 0;
                        for (reg2=0; reg2<16; reg2++)
                        {
                            /* jc*/
                            XAxiPcie_ReadRemoteConfigSpace(
                                    AxiPciePtr, PCIeBusNum,
                                    PCIeDevNum, PCIeFunNum,
                                    reg2,
                                    &ConfigData);
                            xil_printf("    -- Bridge-PCIeHeader Reg_%d: 0x%08X \r\n", reg2, ConfigData);
                        }
                                
                        XAxiPcie_ReadRemoteConfigSpace(
                                AxiPciePtr, PCIeBusNum,
                                PCIeDevNum, PCIeFunNum,
                                4/*0x10*/,
                                &ConfigData);
                        xil_printf("  - Bridge-BAR 0x10: 0x%08X \r\n", ConfigData);
                        /*	*/
					#endif	
					
						/**/
						/* recall the functions*/
						/**/
						/* PCIeBusNum += 1;*/
						/* PCIeEnumerateFabric_2(AxiPciePtr, PCIeBusNum, PCIeDevNum, PCIeFunNum);*/
						/**/
						/*/////////////////////////////////////////////////////////////////////////////////////////////////////*/

					}/* for (PCIeFunNum) */
				}/* for (PCIeDevNum) */

				if ((!PCIeFunNum) && (!PCIeMultiFun))
				{
					/*
					 * If it is function 0 and it is not a
					 * multi function device, we don't need
					 * to look any further on this devie
					 */
					break;
				}				
			}  /* Functions in one device */
		}  /* Devices on the same bus */

		/**/
		/* recall the functions*/
		/**/
		/*PCIeBusNum += 1;*/
		/*PCIeEnumerateFabric_2(AxiPciePtr, PCIeBusNum, PCIeDevNum, PCIeFunNum);		*/
	}  /* Buses in the same system */

	xil_printf("End of Enumeration of PCIe Fabric on This system\r\n");

	/* Bridge enable */
	XAxiPcie_GetRootPortStatusCtrl(AxiPciePtr, &RegVal);
	RegVal |= XAXIPCIE_RPSC_BRIDGE_ENABLE_MASK;
	XAxiPcie_SetRootPortStatusCtrl(AxiPciePtr, RegVal);

	return;
}


/* for 613*/
void pl_pcie_rc_set_bar1_addr(void)
{
	u32 PCIeBusNum, PCIeDevNum, PCIeFunNum;	
	XAxiPcie * AxiPciePtr = (XAxiPcie *)(&AxiPcieInstance);

	PCIeBusNum = 2; 
	PCIeDevNum = 15; 
	PCIeFunNum = 0;

	/**/
	/* ref_613 afdx pcei-ep*/
	/**/
	/* 0x6000_0000: reg_space 512 bytes*/
	/* 0x6001_0000: ram_space 5k bytes*/
	/*	*/
	XAxiPcie_WriteRemoteConfigSpace(AxiPciePtr,
									PCIeBusNum, PCIeDevNum,
									PCIeFunNum, PCIE_CFG_BAR_1_REG,
									(XPAR_AXI_PCIE_0_AXIBAR_0 + 0x00010000));  /*  bar1: 0x6001_0000					*/

	return;
}

static void __attribute__ ((noinline)) UtilDelay(unsigned int Seconds)
{
#if defined (__MICROBLAZE__) || defined(__PPC__)
	static int WarningFlag = 0;

	/* If MB caches are disabled or do not exist, this delay loop could
	 * take minutes instead of seconds (e.g., 30x longer).  Print a warning
	 * message for the user (once).  If only MB had a built-in timer!
	 */
	if (((mfmsr() & 0x20) == 0) && (!WarningFlag)) {
		WarningFlag = 1;
	}

#define ITERS_PER_SEC   (XPAR_CPU_CORE_CLOCK_FREQ_HZ / 6)
    asm volatile ("\n"
			"1:               \n\t"
			"addik r7, r0, %0 \n\t"
			"2:               \n\t"
			"addik r7, r7, -1 \n\t"
			"bneid  r7, 2b    \n\t"
			"or  r0, r0, r0   \n\t"
			"bneid %1, 1b     \n\t"
			"addik %1, %1, -1 \n\t"
			:: "i"(ITERS_PER_SEC), "d" (Seconds));
#else
    sleep(Seconds);
#endif
}


#if 1

int XAxiPcie_ReadRemote_CfgSpace2(UINT8 PCIeBusNum, UINT8 PCIeDevNum, UINT8 PCIeFunNum, UINT16 reg_idx, UINT32 *DataPtr)
{	
	XAxiPcie* AxiPciePtr  = (XAxiPcie*)(&AxiPcieInstance);
	
    /* 
     * check whether it is a valid access.
     * 1. every root port has only one slot.
     *    so, if (bus ==0 && devNum > 0), just return.
     * 2. there is only one device on the bus directly attached to RC's.
     *    so, if (bus ==1 && devNum > 0), just return.
     */
    if ((PCIeBusNum <= 1) && (PCIeDevNum != 0))
    {
        return (-1);  /* error*/
    }
	else
	{
		XAxiPcie_ReadRemoteConfigSpace(
			AxiPciePtr,
			PCIeBusNum, PCIeDevNum,
			PCIeFunNum,
		    reg_idx,
			DataPtr);

		return 0;  /* ok*/
	}
}


int XAxiPcie_WriteRemote_CfgSpace2(UINT8 PCIeBusNum, UINT8 PCIeDevNum, UINT8 PCIeFunNum, UINT16 reg_idx, UINT32 Data)
{
	XAxiPcie* AxiPciePtr  = (XAxiPcie*)(&AxiPcieInstance);
	
    /* 
     * check whether it is a valid access.
     * 1. every root port has only one slot.
     *    so, if (bus ==0 && devNum > 0), just return.
     * 2. there is only one device on the bus directly attached to RC's.
     *    so, if (bus ==1 && devNum > 0), just return.
     */
    if ((PCIeBusNum <= 1) && (PCIeDevNum != 0))
    {
        return (-1);  /* error*/
    }
	else
	{
	
	XAxiPcie_WriteRemoteConfigSpace
		(AxiPciePtr,
		PCIeBusNum, PCIeDevNum,
		PCIeFunNum,
	    reg_idx,
		Data);
	
		return 0;  /* ok*/
	}
}

#endif



#if 1  /* for test*/
typedef struct pciHeaderDevice_2
{
    short	vendorId;	/* vendor ID */
    short	deviceId;	/* device ID */
    short	command;	/* command register */
    short	status;		/* status register */
    char	revisionId;	/* revision ID */
    char	classCode;	/* class code */
    char	subClass;	/* sub class code */
    char	progIf;		/* programming interface */
    char	cacheLine;	/* cache line */
    char	latency;	/* latency time */
    char	headerType;	/* header type */
    char	bist;		/* BIST */
    int		base0;		/* base address 0 */
    int		base1;		/* base address 1 */
    int		base2;		/* base address 2 */
    int		base3;		/* base address 3 */
    int		base4;		/* base address 4 */
    int		base5;		/* base address 5 */
    int		cis;		/* cardBus CIS pointer */
    short	subVendorId;	/* sub system vendor ID */
    short	subSystemId;	/* sub system ID */
    int		romBase;	/* expansion ROM base address */
    int		reserved0;	/* reserved */
    int		reserved1;	/* reserved */
    char	intLine;	/* interrupt line */
    char	intPin;		/* interrupt pin */
    char	minGrant;	/* min Grant */
    char	maxLatency;	/* max Latency */
} PCI_HEADER_DEVICE_2;


void pciDheaderPrint  (    PCI_HEADER_DEVICE_2 * pD)
{
    printf ("vendor ID =                   0x%.4x\n", (UINT16)pD->vendorId);
    printf ("device ID =                   0x%.4x\n", (UINT16)pD->deviceId);
    printf ("command register =            0x%.4x\n", (UINT16)pD->command);
    printf ("status register =             0x%.4x\n", (UINT16)pD->status);	
    printf ("revision ID =                 0x%.2x\n", (UINT8)pD->revisionId);
    printf ("class code =                  0x%.2x\n", (UINT8)pD->classCode);	
    printf ("sub class code =              0x%.2x\n", (UINT8)pD->subClass);
    printf ("programming interface =       0x%.2x\n", (UINT8)pD->progIf);	
    printf ("cache line =                  0x%.2x\n", (UINT8)pD->cacheLine);
    printf ("latency time =                0x%.2x\n", (UINT8)pD->latency);
    printf ("header type =                 0x%.2x\n", (UINT8)pD->headerType);
    printf ("BIST =                        0x%.2x\n", (UINT8)pD->bist);	
    printf ("base address 0 =              0x%.8x\n", pD->base0);	
    printf ("base address 1 =              0x%.8x\n", pD->base1);	
    printf ("base address 2 =              0x%.8x\n", pD->base2);	
    printf ("base address 3 =              0x%.8x\n", pD->base3);	
    printf ("base address 4 =              0x%.8x\n", pD->base4);	
    printf ("base address 5 =              0x%.8x\n", pD->base5);	
    printf ("cardBus CIS pointer =         0x%.8x\n", pD->cis);	
    printf ("sub system vendor ID =        0x%.4x\n", (UINT16)pD->subVendorId);
    printf ("sub system ID =               0x%.4x\n", (UINT16)pD->subSystemId);
    printf ("expansion ROM base address =  0x%.8x\n", pD->romBase);
    printf ("interrupt line =              0x%.2x\n", (UINT8)pD->intLine);
    printf ("interrupt pin =               0x%.2x\n", (UINT8)pD->intPin);	
    printf ("min Grant =                   0x%.2x\n", (UINT8)pD->minGrant);
    printf ("max Latency =                 0x%.2x\n", (UINT8)pD->maxLatency);

	return;
}

STATUS pciHeaderShow
    (
    int	busNo,		/* bus number */
    int	deviceNo,	/* device number */
    int	funcNo		/* function number */
    )
{
    PCI_HEADER_DEVICE_2 headerDevice;
    /*PCI_HEADER_BRIDGE headerBridge;*/
    
    PCI_HEADER_DEVICE_2 * pD = &headerDevice;
	
    int configRegTmp = 0;
	
    /*PCI_HEADER_BRIDGE *pB = &headerBridge;*/
    /*memset(&headerDevice,0,sizeof(PCI_HEADER_DEVICE_2));*/
    /*pciDheaderPrint (pD);*/
    /*if (pciLibInitStatus != OK)			/* sanity check */
    /*    return (ERROR);*/

    /*pciConfigInByte (busNo, deviceNo, funcNo, PCI_CFG_HEADER_TYPE, */
	/*	     &pD->headerType);*/

    /*if (pD->headerType & 0x01)		 PCI-to-PCI bridge */
	{
 
	}
    /* else					/* PCI device */
	{
		XAxiPcie_ReadRemote_CfgSpace2(busNo, deviceNo, funcNo,	PCIE_CFG_ID_REG, &configRegTmp);
		
		pD->vendorId = configRegTmp;
		pD->deviceId = configRegTmp>>16;
		XAxiPcie_ReadRemote_CfgSpace2 (busNo, deviceNo, funcNo, PCI_CFG_COMMAND, &configRegTmp);
		
		pD->command = configRegTmp;
		pD->status = configRegTmp>>16;
		XAxiPcie_ReadRemote_CfgSpace2 (busNo, deviceNo, funcNo, 2, &configRegTmp);
		
		pD->revisionId= configRegTmp;
		pD->progIf= configRegTmp>>8;
		pD->subClass= configRegTmp>>16;
		pD->cacheLine= configRegTmp>>24;
		XAxiPcie_ReadRemote_CfgSpace2 (busNo, deviceNo, funcNo, PCIE_CFG_CAH_LAT_HD_REG, &configRegTmp);
		
		pD->cacheLine = configRegTmp;
		pD->latency = configRegTmp>>8;
		pD->headerType = configRegTmp>>16;
		pD->bist = configRegTmp>>24;
		XAxiPcie_ReadRemote_CfgSpace2(busNo, deviceNo, funcNo, PCIE_CFG_BAR_0_REG, &pD->base0);
		
		XAxiPcie_ReadRemote_CfgSpace2(busNo, deviceNo, funcNo, 5, &pD->base1);
		XAxiPcie_ReadRemote_CfgSpace2(busNo, deviceNo, funcNo, 6, &pD->base2);
		XAxiPcie_ReadRemote_CfgSpace2(busNo, deviceNo, funcNo, 7, &pD->base3);
		XAxiPcie_ReadRemote_CfgSpace2(busNo, deviceNo, funcNo, 8, &pD->base4);
		XAxiPcie_ReadRemote_CfgSpace2(busNo, deviceNo, funcNo, 9, &pD->base5);
		
		XAxiPcie_ReadRemote_CfgSpace2(busNo, deviceNo, funcNo, PCI_CFG_CIS>>2, &pD->cis);
		
		XAxiPcie_ReadRemote_CfgSpace2(busNo, deviceNo, funcNo, PCI_CFG_SUB_VENDER_ID>>2, &configRegTmp);
		
		pD->subVendorId = configRegTmp;
		pD->subSystemId = configRegTmp>>16;
		XAxiPcie_ReadRemote_CfgSpace2(busNo, deviceNo, funcNo, PCI_CFG_DEV_INT_LINE>>2, &configRegTmp);
		
		pD->intLine = configRegTmp;
		pD->intPin = configRegTmp>>8;
		pD->minGrant = configRegTmp>>16;
		pD->maxLatency = configRegTmp>>24;

		/**/
		/* print pcie info*/
		/**/
        pciDheaderPrint (pD);
	}

    return (OK);
}

#endif



