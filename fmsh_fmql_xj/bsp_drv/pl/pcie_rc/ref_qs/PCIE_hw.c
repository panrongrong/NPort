#include "PCIE_hw.h"
#include "stdio.h"
/*#include "pciConfigLib.h"*/
XAxiPcie_BarAddr BarAddr;

void PCIEWriteReg(unsigned int offset, unsigned int data)
{
	*(unsigned int *)(XPAR_AXI_PCIE_0_BASEADDR + offset) = data;
}

unsigned int PCIEReadReg(unsigned int offset)
{
	unsigned int data;
	data = *(unsigned int *)(XPAR_AXI_PCIE_0_BASEADDR + offset);
	return data;
}

void XAxiPcie_GetEnabledInterrupts(uint32_t *EnabledMaskPtr)
{

	*EnabledMaskPtr = PCIEReadReg(XAXIPCIE_IM_OFFSET);

}

void XAxiPcie_DisableInterrupts(uint32_t DisableMask)
{
	uint32_t Data = 0;

	Data = PCIEReadReg(XAXIPCIE_IM_OFFSET);

	PCIEWriteReg(XAXIPCIE_IM_OFFSET, (Data & (~DisableMask)));

}

void XAxiPcie_GetPendingInterrupts( uint32_t *PendingMaskPtr)
{
	*PendingMaskPtr = PCIEReadReg(XAXIPCIE_ID_OFFSET);

}

void XAxiPcie_ClearPendingInterrupts(uint32_t ClearMask)
{
	PCIEWriteReg( XAXIPCIE_ID_OFFSET, PCIEReadReg(XAXIPCIE_ID_OFFSET) & (ClearMask));
}


void XAxiPcie_GetRequesterId(uint8_t *BusNumPtr, uint8_t *DevNumPtr, uint8_t *FunNumPtr, uint8_t *PortNumPtr)
{
	uint32_t Data = 0;

	Data = PCIEReadReg(XAXIPCIE_BL_OFFSET);

	*BusNumPtr = (uint8_t)((Data & XAXIPCIE_BL_BUS_MASK) >>
							XAXIPCIE_BL_BUS_SHIFT);

	*DevNumPtr = (uint8_t)((Data & XAXIPCIE_BL_DEV_MASK) >>
							XAXIPCIE_BL_DEV_SHIFT);

	*FunNumPtr = (uint8_t)(Data & XAXIPCIE_BL_FUNC_MASK);


	*PortNumPtr = (uint8_t)((Data & XAXIPCIE_BL_PORT_MASK) >>
						XAXIPCIE_BL_PORT_SHIFT);

}

void XAxiPcie_ReadLocalConfigSpace(u16 Offset, uint32_t *DataPtr)
{
	*DataPtr = PCIEReadReg((XAXIPCIE_PCIE_CORE_OFFSET + ((uint32_t) (Offset * 4))));

}

void XAxiPcie_WriteLocalConfigSpace(u16 Offset,uint32_t Data)
{
	PCIEWriteReg((XAXIPCIE_PCIE_CORE_OFFSET + ((uint32_t) (Offset * 4))), Data);

}

void XAxiPcie_GetLocalBusBar2PcieBar(uint8_t BarNumber, XAxiPcie_BarAddr *BarAddrPtr)
{
	BarAddrPtr->LowerAddr =
			PCIEReadReg((XAXIPCIE_AXIBAR2PCIBAR_0L_OFFSET + (BarNumber * (sizeof(uint32_t) * 2))));

	BarAddrPtr->UpperAddr =
			PCIEReadReg((XAXIPCIE_AXIBAR2PCIBAR_0U_OFFSET + (BarNumber * (sizeof(uint32_t) * 2))));

}

void XAxiPcie_SetLocalBusBar2PcieBar(uint8_t BarNumber, XAxiPcie_BarAddr *BarAddrPtr)
{

	PCIEWriteReg((XAXIPCIE_AXIBAR2PCIBAR_0L_OFFSET + (BarNumber * (sizeof(uint32_t) * 2))), (BarAddrPtr->LowerAddr));

	PCIEWriteReg((XAXIPCIE_AXIBAR2PCIBAR_0U_OFFSET + (BarNumber * (sizeof(uint32_t) * 2))), (BarAddrPtr->UpperAddr));

}

int PcieInitRootComplex()
{
	int Status;
	uint32_t HeaderData;
	uint32_t InterruptMask;
	uint8_t  BusNumber;
	uint8_t  DeviceNumber;
	uint8_t  FunNumber;
	uint8_t  PortNumber;

	
	/* See what interrupts are currently enabled */
	XAxiPcie_GetEnabledInterrupts(&InterruptMask);
	xil_printf("Interrupts currently enabled are %8X\r\n", InterruptMask);

	/* Make sure all interrupts disabled. */
	XAxiPcie_DisableInterrupts(XAXIPCIE_IM_ENABLE_ALL_MASK);


	/* See what interrupts are currently pending */
	XAxiPcie_GetPendingInterrupts(&InterruptMask);
	xil_printf("Interrupts currently pending are %8X\r\n", InterruptMask);

	/* Just if there is any pending interrupt then clear it.*/
	XAxiPcie_ClearPendingInterrupts(XAXIPCIE_ID_CLEAR_ALL_MASK);

	/*
	 * Read enabled interrupts and pending interrupts
	 * to verify the previous two operations and also
	 * to test those two API functions
	 */

	XAxiPcie_GetEnabledInterrupts(&InterruptMask);
	xil_printf("Interrupts currently enabled are %8X\r\n", InterruptMask);

	XAxiPcie_GetPendingInterrupts(&InterruptMask);
	xil_printf("Interrupts currently pending are %8X\r\n", InterruptMask);

	/*
	 * The following two calls have no effect on the behavior
	 * of this program. It shows you how to use those two API calls
	 */

	XAxiPcie_GetLocalBusBar2PcieBar(0, &BarAddr);
	xil_printf("BarAddr--------------- 0x%8X 0x%8X\r\n", BarAddr.UpperAddr,BarAddr.LowerAddr);
	XAxiPcie_SetLocalBusBar2PcieBar(0, &BarAddr);


	/* Make sure link is up. */
	Status = XAxiPcie_IsLinkUp();
	if (Status != TRUE ) {
		printf("Link is not up\r\n");
		return ERROR;
	}

	printf("Link is up\r\n");

	/*
	 * Read back requester ID.
	 */

	XAxiPcie_GetRequesterId(&BusNumber,
				&DeviceNumber, &FunNumber, &PortNumber);

	printf("Bus Number is %02X\r\n"
			"Device Number is %02X\r\n"
	 			"Function Number is %02X\r\n"
	 				"Port Number is %02X\r\n",
	 		BusNumber, DeviceNumber, FunNumber, PortNumber);


	/* Set up the PCIe header of this Root Complex */
	XAxiPcie_ReadLocalConfigSpace(PCIE_CFG_CMD_STATUS_REG, &HeaderData);

	HeaderData |= (PCIE_CFG_CMD_BUSM_EN | PCIE_CFG_CMD_MEM_EN |
				PCIE_CFG_CMD_IO_EN | PCIE_CFG_CMD_PARITY |
							PCIE_CFG_CMD_SERR_EN);

	XAxiPcie_WriteLocalConfigSpace(PCIE_CFG_CMD_STATUS_REG, HeaderData);

	/*
	 * Read back local config reg.
	 * to verify the write.
	 */

	XAxiPcie_ReadLocalConfigSpace(PCIE_CFG_CMD_STATUS_REG, &HeaderData);

	xil_printf("PCIe Local Config Space is %8X at register"
					" CommandStatus\r\n", HeaderData);

	/*
	 * Set up Bus number
	 */

	HeaderData = PCIE_CFG_PRIM_SEC_BUS;

	XAxiPcie_WriteLocalConfigSpace(PCIE_CFG_PRI_SEC_BUS_REG, HeaderData);

	/*
	 * Read back local config reg.
	 * to verify the write.
	 */

	XAxiPcie_ReadLocalConfigSpace(PCIE_CFG_PRI_SEC_BUS_REG, &HeaderData);

	xil_printf("PCIe Local Config Space is %8X at register "
					"Prim Sec. Bus\r\n", HeaderData);

	/* Now it is ready to function */

	xil_printf("Root Complex IP Instance has been successfully"
							" initialized\r\n");

	return OK;
}

uint32_t XAxiPcie_ComposeExternalConfigAddress(uint8_t Bus, uint8_t Device, uint8_t Function,
								 u16 Offset)
{
	uint32_t Location = 0;

	Location |= ((((uint32_t)Bus) << XAXIPCIE_ECAM_BUS_SHIFT) &
						XAXIPCIE_ECAM_BUS_MASK);

	Location |= ((((uint32_t)Device) << XAXIPCIE_ECAM_DEV_SHIFT) &
						XAXIPCIE_ECAM_DEV_MASK);

	Location |= ((((uint32_t)Function) << XAXIPCIE_ECAM_FUN_SHIFT) &
						XAXIPCIE_ECAM_FUN_MASK);

	Location |= ((((uint32_t)Offset) << XAXIPCIE_ECAM_REG_SHIFT) &
						XAXIPCIE_ECAM_REG_MASK);

	Location &= XAXIPCIE_ECAM_MASK;

	return Location;
}

void XAxiPcie_ReadRemoteConfigSpace(uint8_t Bus, uint8_t Device, uint8_t Function, u16 Offset, uint32_t *DataPtr)
{
	uint32_t Location = 0;
	uint32_t Data;

/*	if (((Bus == 0) && !((Device == 0) && (Function == 0))) ||
		(Bus > InstancePtr->MaxNumOfBuses)) {
		*DataPtr = 0xFFFFFFFF;
		return;
	}*/

	/* Compose function configuration space location */
	Location = XAxiPcie_ComposeExternalConfigAddress (Bus, Device,
							Function, Offset);

	while(XAxiPcie_IsEcamBusy());

	/* Read data from that location */
	Data = PCIEReadReg(Location);
	*DataPtr = Data;

}


void XAxiPcie_WriteRemoteConfigSpace(uint8_t Bus, uint8_t Device, uint8_t Function, u16 Offset, uint32_t Data)
{
	uint32_t Location = 0;
	uint32_t TestWrite = 0;
	uint8_t Count = 3;

/*	if ((Bus == 0) || (Bus > InstancePtr->MaxNumOfBuses)) {
		return;
	}*/

	/* Compose function configuration space location */
	Location = XAxiPcie_ComposeExternalConfigAddress (Bus, Device,
							Function, Offset);
	while(XAxiPcie_IsEcamBusy());


	/* Write data to that location */
	PCIEWriteReg(Location , Data);


	/* Read data from that location to verify write */
	while (Count) {

		TestWrite =
				PCIEReadReg(Location);

		if (TestWrite == Data) {
			break;
		}

		Count--;
	}
}

void XAxiPcie_GetRootPortStatusCtrl(uint32_t *StatusPtr)
{

	*StatusPtr = PCIEReadReg(XAXIPCIE_RPSC_OFFSET);
}


void XAxiPcie_SetRootPortStatusCtrl(uint32_t StatusData)
{

	PCIEWriteReg(XAXIPCIE_RPSC_OFFSET,StatusData & XAXIPCIE_RPSC_MASK);

}

void PCIeEnumerateFabric()
{

	uint32_t ConfigData;
	uint32_t PCIeHeaderType;
	uint32_t PCIeMultiFun;
	uint32_t PCIeBusNum;
	uint32_t PCIeDevNum;
	uint32_t PCIeFunNum;
	u16 PCIeVendorID;
	uint32_t RegVal;

	xil_printf("Start Enumeration of PCIe Fabric on This System\r\n");

	/* Scan PCIe Fabric */

	for (PCIeBusNum = 0; PCIeBusNum < PCIE_CFG_MAX_NUM_OF_BUS; PCIeBusNum++) 
	{
		for (PCIeDevNum = 0; PCIeDevNum < PCIE_CFG_MAX_NUM_OF_DEV; PCIeDevNum++) 
		{
			for (PCIeFunNum = 0; PCIeFunNum < PCIE_CFG_MAX_NUM_OF_FUN; PCIeFunNum++) 
			{
				/*xil_printf("PCIeBus is %02X  PCIeDev is %02X PCIeFunc is %02X\n",PCIeBusNum, PCIeDevNum,PCIeFunNum);*/	
				/* Vendor ID */
				XAxiPcie_ReadRemoteConfigSpace(
					PCIeBusNum,
					PCIeDevNum, PCIeFunNum,
					PCIE_CFG_ID_REG, &ConfigData);

				PCIeVendorID = (u16) (ConfigData >> 16);

				if (PCIeVendorID ==
						PCIE_CFG_FUN_NOT_IMP_MASK) {
					if (PCIeFunNum == 0)
					/*
					 * We don't need to look
					 * any further on this device.
					 */
					break;
				}
				else {
					xil_printf("PCIeBus is %02X\r\n"
						"PCIeDev is %02X\r\n"
						"PCIeFunc is %02X\r\n",
						PCIeBusNum, PCIeDevNum,
								PCIeFunNum);

					xil_printf("Vendor ID is %04X \r\n",
								PCIeVendorID);

					/* Header Type */
					XAxiPcie_ReadRemoteConfigSpace(
						PCIeBusNum,
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
						xil_printf("This is an "
							"End Point\r\n");

						/*
						 * Initialize this end point
						 * and return.
						 */

						XAxiPcie_ReadRemoteConfigSpace(
							PCIeBusNum, PCIeDevNum,
							PCIeFunNum,
						PCIE_CFG_CMD_STATUS_REG,
								&ConfigData);

						ConfigData |=
						(PCIE_CFG_CMD_BUSM_EN |
							PCIE_CFG_CMD_MEM_EN);


						XAxiPcie_WriteRemoteConfigSpace
							(PCIeBusNum, PCIeDevNum,
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
						(PCIeBusNum, PCIeDevNum,
						PCIeFunNum, PCIE_CFG_BAR_0_REG,
						ConfigData);

						xil_printf("End Point has been"
							" enabled\r\n");

					}
					else {
						/* This is a bridge */
						xil_printf("This is a "
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

	xil_printf("End of Enumeration of PCIe Fabric on This system\r\n");

	/* Bridge enable */
	XAxiPcie_GetRootPortStatusCtrl(&RegVal);
	RegVal |= XAXIPCIE_RPSC_BRIDGE_ENABLE_MASK;
	XAxiPcie_SetRootPortStatusCtrl(RegVal);

	return;
}


#if 1
LOCAL void pciDheaderPrint
    (
    PCI_HEADER_DEVICE * pD
    )
    {
    printf ("vendor ID =                   0x%.4x\n", (ushort_t)pD->vendorId);
    printf ("device ID =                   0x%.4x\n", (ushort_t)pD->deviceId);
    printf ("command register =            0x%.4x\n", (ushort_t)pD->command);
    printf ("status register =             0x%.4x\n", (ushort_t)pD->status);	
    printf ("revision ID =                 0x%.2x\n", (uchar_t)pD->revisionId);
    printf ("class code =                  0x%.2x\n", (uchar_t)pD->classCode);	
    printf ("sub class code =              0x%.2x\n", (uchar_t)pD->subClass);
    printf ("programming interface =       0x%.2x\n", (uchar_t)pD->progIf);	
    printf ("cache line =                  0x%.2x\n", (uchar_t)pD->cacheLine);
    printf ("latency time =                0x%.2x\n", (uchar_t)pD->latency);
    printf ("header type =                 0x%.2x\n", (uchar_t)pD->headerType);
    printf ("BIST =                        0x%.2x\n", (uchar_t)pD->bist);	
    printf ("base address 0 =              0x%.8x\n", pD->base0);	
    printf ("base address 1 =              0x%.8x\n", pD->base1);	
    printf ("base address 2 =              0x%.8x\n", pD->base2);	
    printf ("base address 3 =              0x%.8x\n", pD->base3);	
    printf ("base address 4 =              0x%.8x\n", pD->base4);	
    printf ("base address 5 =              0x%.8x\n", pD->base5);	
    printf ("cardBus CIS pointer =         0x%.8x\n", pD->cis);	
    printf ("sub system vendor ID =        0x%.4x\n", (ushort_t)pD->subVendorId);
    printf ("sub system ID =               0x%.4x\n", (ushort_t)pD->subSystemId);
    printf ("expansion ROM base address =  0x%.8x\n", pD->romBase);
    printf ("interrupt line =              0x%.2x\n", (uchar_t)pD->intLine);
    printf ("interrupt pin =               0x%.2x\n", (uchar_t)pD->intPin);	
    printf ("min Grant =                   0x%.2x\n", (uchar_t)pD->minGrant);
    printf ("max Latency =                 0x%.2x\n", (uchar_t)pD->maxLatency);
    }

STATUS pciHeaderShow
    (
    int	busNo,		/* bus number */
    int	deviceNo,	/* device number */
    int	funcNo		/* function number */
    )
    {
    PCI_HEADER_DEVICE headerDevice;
    /*PCI_HEADER_BRIDGE headerBridge;*/
    PCI_HEADER_DEVICE * pD = &headerDevice;
    int configRegTmp=0;
    /*PCI_HEADER_BRIDGE *pB = &headerBridge;*/
    /*memset(&headerDevice,0,sizeof(PCI_HEADER_DEVICE));*/
    /*pciDheaderPrint (pD);*/
    /*if (pciLibInitStatus != OK)			/* sanity check */
    /*    return (ERROR);*/

    /*pciConfigInByte (busNo, deviceNo, funcNo, PCI_CFG_HEADER_TYPE, */
	/*	     &pD->headerType);*/

    /*if (pD->headerType & 0x01)		 PCI-to-PCI bridge */
	{
 
	}
    /*else					/* PCI device */
	{
       /* pciConfigInWord (busNo, deviceNo, funcNo, PCI_CFG_VENDOR_ID, 
			 &pD->vendorId);
        pciConfigInWord (busNo, deviceNo, funcNo, PCI_CFG_DEVICE_ID, 
			 &pD->deviceId);
        pciConfigInWord (busNo, deviceNo, funcNo, PCI_CFG_COMMAND, 
			 &pD->command);
        pciConfigInWord (busNo, deviceNo, funcNo, PCI_CFG_STATUS, 
			 &pD->status);
        pciConfigInByte (busNo, deviceNo, funcNo, PCI_CFG_REVISION, 
			 &pD->revisionId);
        pciConfigInByte (busNo, deviceNo, funcNo, PCI_CFG_PROGRAMMING_IF, 
			 &pD->progIf);
        pciConfigInByte (busNo, deviceNo, funcNo, PCI_CFG_SUBCLASS, 
			 &pD->subClass);
        pciConfigInByte (busNo, deviceNo, funcNo, PCI_CFG_CLASS, 
			 &pD->classCode);
        pciConfigInByte (busNo, deviceNo, funcNo, PCI_CFG_CACHE_LINE_SIZE, 
			 &pD->cacheLine);
        pciConfigInByte (busNo, deviceNo, funcNo, PCI_CFG_LATENCY_TIMER, 
			 &pD->latency);
        pciConfigInByte (busNo, deviceNo, funcNo, PCI_CFG_HEADER_TYPE, 
			 &pD->headerType);
        pciConfigInByte (busNo, deviceNo, funcNo, PCI_CFG_BIST, 
			 &pD->bist);
        pciConfigInLong (busNo, deviceNo, funcNo, PCI_CFG_BASE_ADDRESS_0, 
			 &pD->base0);
        pciConfigInLong (busNo, deviceNo, funcNo, PCI_CFG_BASE_ADDRESS_1, 
			 &pD->base1);
        pciConfigInLong (busNo, deviceNo, funcNo, PCI_CFG_BASE_ADDRESS_2, 
			 &pD->base2);
        pciConfigInLong (busNo, deviceNo, funcNo, PCI_CFG_BASE_ADDRESS_3, 
			 &pD->base3);
        pciConfigInLong (busNo, deviceNo, funcNo, PCI_CFG_BASE_ADDRESS_4, 
			 &pD->base4);
        pciConfigInLong (busNo, deviceNo, funcNo, PCI_CFG_BASE_ADDRESS_5, 
			 &pD->base5);
        pciConfigInLong (busNo, deviceNo, funcNo, PCI_CFG_CIS, 
			 &pD->cis);
        pciConfigInWord (busNo, deviceNo, funcNo, PCI_CFG_SUB_VENDER_ID, 
			 &pD->subVendorId);
        pciConfigInWord (busNo, deviceNo, funcNo, PCI_CFG_SUB_SYSTEM_ID, 
			 &pD->subSystemId);
        pciConfigInLong (busNo, deviceNo, funcNo, PCI_CFG_EXPANSION_ROM, 
			 &pD->romBase);
        pciConfigInByte (busNo, deviceNo, funcNo, PCI_CFG_DEV_INT_LINE, 
			 &pD->intLine);
        pciConfigInByte (busNo, deviceNo, funcNo, PCI_CFG_DEV_INT_PIN, 
			 &pD->intPin);
        pciConfigInByte (busNo, deviceNo, funcNo, PCI_CFG_MIN_GRANT, 
			 &pD->minGrant);
        pciConfigInByte (busNo, deviceNo, funcNo, PCI_CFG_MAX_LATENCY, 
			 &pD->maxLatency);*/
		XAxiPcie_ReadRemoteConfigSpace(busNo,deviceNo, funcNo,	PCIE_CFG_ID_REG, &configRegTmp);
		pD->vendorId = configRegTmp;
		pD->deviceId = configRegTmp>>16;
		XAxiPcie_ReadRemoteConfigSpace (busNo, deviceNo, funcNo, PCI_CFG_COMMAND, &configRegTmp);
		pD->command = configRegTmp;
		pD->status = configRegTmp>>16;
		XAxiPcie_ReadRemoteConfigSpace (busNo, deviceNo, funcNo, 2, &configRegTmp);
		pD->revisionId= configRegTmp;
		pD->progIf= configRegTmp>>8;
		pD->subClass= configRegTmp>>16;
		pD->cacheLine= configRegTmp>>24;
		XAxiPcie_ReadRemoteConfigSpace (busNo, deviceNo, funcNo, PCIE_CFG_CAH_LAT_HD_REG, &configRegTmp);
		pD->cacheLine = configRegTmp;
		pD->latency = configRegTmp>>8;
		pD->headerType = configRegTmp>>16;
		pD->bist = configRegTmp>>24;
		XAxiPcie_ReadRemoteConfigSpace(
				busNo,
				deviceNo, funcNo,
				PCIE_CFG_BAR_0_REG, &pD->base0);
		XAxiPcie_ReadRemoteConfigSpace(
				busNo,
				deviceNo, funcNo,
				5, &pD->base1);
		XAxiPcie_ReadRemoteConfigSpace(
				busNo,
				deviceNo, funcNo,
				6, &pD->base2);
		XAxiPcie_ReadRemoteConfigSpace(
				busNo,
				deviceNo, funcNo,
				7, &pD->base3);
		XAxiPcie_ReadRemoteConfigSpace(
				busNo,
				deviceNo, funcNo,
				8, &pD->base4);
		XAxiPcie_ReadRemoteConfigSpace(
				busNo,
				deviceNo, funcNo,
				9, &pD->base5);
		XAxiPcie_ReadRemoteConfigSpace(
				busNo,
				deviceNo, funcNo,
				PCI_CFG_CIS>>2, &pD->cis);
		XAxiPcie_ReadRemoteConfigSpace(
						busNo,
						deviceNo, funcNo,
						PCI_CFG_SUB_VENDER_ID>>2, &configRegTmp);
		pD->subVendorId = configRegTmp;
		pD->subSystemId = configRegTmp>>16;
		XAxiPcie_ReadRemoteConfigSpace(
						busNo,
						deviceNo, funcNo,
						PCI_CFG_DEV_INT_LINE>>2, &configRegTmp);
		pD->intLine = configRegTmp;
		pD->intPin = configRegTmp>>8;
		pD->minGrant = configRegTmp>>16;
		pD->maxLatency = configRegTmp>>24;
        pciDheaderPrint (pD);
	}

    return (OK);
    }

#endif

	