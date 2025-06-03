/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_axidma.c
*
* This file contains all private & pbulic function
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 0.01   jc  03/15/2021  First Release
*</pre>
******************************************************************************/

/***************************** Include Files *********************************/
#include <stdlib.h>
/*#include "../../common/fmsh_ps_parameters.h"*/

/*#include "fmsh_axidma.h"*/

/*
prototype def
*/
void init_sys_intr(void);
void Axidma_Intr_Enable(XAxiDma *AxiDmaPtr);
void pl_hp_init(void);


#define AXIDMA_DEVICE_ID	XPAR_AXIDMA_0_DEVICE_ID
/*#define INT_VEC_IRQF2P_0            IVEC_TO_INUM(57)*/
/*#define INT_VEC_IRQF2P_1            IVEC_TO_INUM(58)*/
#define TX_INTR_ID			XPAR_FABRIC_AXI_DMA_0_MM2S_INTROUT_INTR - 4
#define RX_INTR_ID			XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR - 4

#define MEM_BASE_ADDR		0x20000000 /* 0x01000000*/
#define TX_BUFFER_BASE		(MEM_BASE_ADDR + 0x00100000)
#define RX_BUFFER_BASE		(MEM_BASE_ADDR + 0x00300000)
#define RX_BUFFER_HIGH		(MEM_BASE_ADDR + 0x004FFFFF)

/* Timeout loop counter for reset*/
#define RESET_TIMEOUT_COUNTER	10000

/* test start value*/
#define TEST_START_VALUE	0x0

/*Buffer and Buffer Descriptor related constant definition*/
#define MAX_PKT_LEN			1024

/* transfer times*/
#define NUMBER_OF_TRANSFERS	10000


UINT32* TxBufferPtr = (u32*)TX_BUFFER_BASE;
UINT32* RxBufferPtr = (u32*)RX_BUFFER_BASE;

volatile int TxDone;
volatile int RxDone;
volatile int Error;

static	XAxiDma axidma;

#if 1  /* for compile*/

u32 Xil_AssertStatus;

void Xil_Assert(const char8 *File, s32 Line)
{
	/* do nothing*/
}

void Xil_DCacheFlushRange(void * Addr, u32 Len)
{
	cacheFlush(DATA_CACHE, (void *)(Addr), (Len));
}
		
void Xil_DCacheInvalidateRange(void * Addr, u32 Len) 
{
	cacheInvalidate(DATA_CACHE, (void *)(Addr), (Len));
}

#endif


int test_axidma(void)
{
    /*init_platform();*/
	init_sys_intr();
	
	Axidma_Test();

    return 0;
}

/*****************************************************************************
 * This function initialize dma, gic.
 *
 * @note		None.
 ******************************************************************************/
void init_sys_intr(void)
{
	/*Init_Fgic(&fgic_inst);												// initial DMA interrupt system*/
	Axidma_Init(&axidma, AXIDMA_DEVICE_ID);								/* initial interrupt system*/

	/* hp open*/
	pl_hp_init();
		
	/*Setup_Intr_Exception(&fgic_inst);*/
	/*Axidma_Setup_Intr(&fgic_inst, &axidma, TX_INTR_ID, RX_INTR_ID);		// setup dma interrpt system*/
	Axidma_Setup_Intr_2(&axidma, TX_INTR_ID, RX_INTR_ID);		/* setup dma interrpt system*/
	
	Axidma_Intr_Enable(&axidma);
}


/*****************************************************************************
 * This function takes axidma test.
 * It firstly sends memory data to slave device,
 * and then reads data back.
 *
 * @return
 *		- FMSH_SUCCESS if validation is successful
 *		- FMSH_FAILURE if validation is failure.
 *
 * @note		None.
 ******************************************************************************/
int Axidma_Test()
{
	int i = 0, Index = 0, success;
	UINT32 Value = 0, start_val = 0;
	
	int	Status;
	int Length = MAX_PKT_LEN/4;
	TxDone	= 0;
	RxDone	= 0;
	Error	= 0;

	printf("\r\n-------DMA Test-------\r\n");
	printf("PKT_LEN = %d\r\n", MAX_PKT_LEN);

	for(i = 0; i < NUMBER_OF_TRANSFERS; i ++)
	{
		Value = TEST_START_VALUE + i;
		start_val = Value;
		
		for(Index = 0; Index < Length; Index ++)
		{
			TxBufferPtr[Index] = Value;
			Value = Value + 1;
		}

		/* Flush the SrcBuffer before the DMA transfer, in case the Data Cache is enabled */

		/**/
		/* flush cache, it's must be for ddr flush new data*/
		/**/
		(void)cacheInvalidate (DATA_CACHE, TxBufferPtr, MAX_PKT_LEN); 

		Status = XAxiDma_SimpleTransfer(&axidma, (u32)RxBufferPtr, MAX_PKT_LEN, XAXIDMA_DEVICE_TO_DMA);
		if (Status != FMSH_SUCCESS) {
			return FMSH_FAILURE;
		}

		Status = XAxiDma_SimpleTransfer(&axidma, (u32)TxBufferPtr, MAX_PKT_LEN, XAXIDMA_DMA_TO_DEVICE);
		if (Status != FMSH_SUCCESS) {
			return FMSH_FAILURE;
		}

		/* Wait TX done and RX done */
		while (!TxDone || !RxDone) 
		{
		    __asm__("NOP"); /* just delay*/
			__asm__("NOP"); 
			__asm__("NOP");
		}

		/**/
		/* flush cache, it's must be for ddr flush new data*/
		/**/
		(void)cacheInvalidate (DATA_CACHE, RxBufferPtr, MAX_PKT_LEN); 

		success++;
		
		TxDone = 0;
		RxDone = 0;
                
		printf("test(%d)=> start_data: 0x%X->(0x%X) - ok \r\n", success, start_val, *(RxBufferPtr+255));
                
                
		if (Error) {
			printf("Failed test transmit%s done");
			goto Done;
		}
		
		/* Test finished, check data */
		Status = Axidma_CheckData(Length, (TEST_START_VALUE + i));
		if (Status != FMSH_SUCCESS) {
			printf("Data check failed\r\n");
			goto Done;
		}

	}
	printf("AXI DMA interrupt example test passed\r\n");
	printf("success=%d\r\n", success);

	
	/* Disable TX and RX Ring interrupts and return success */
	/* Axidma_DisableIntrSystem(&fgic_inst, TX_INTR_ID, RX_INTR_ID);*/
	
Done:
	printf("--- Exiting Test --- \r\n");
	return FMSH_SUCCESS;
}


/*****************************************************************************
 * This function checks data buffer after the DMA transfer is finished.
 *
 * We use the static tx/rx buffers.
 *
 * @param	Length is the length to check
 * @param	StartValue is the starting value of the first byte
 *
 * @return
 *		- FMSH_SUCCESS if validation is successful
 *		- FMSH_FAILURE if validation is failure.
 *
 * @note		None.
 ******************************************************************************/
 int Axidma_CheckData(int Length, u32 StartValue)
{
	u32*	RxPacket;
	u32*	TxPacket;
	int		idx = 0;
	u32		Value;
	

	RxPacket = (u32*) RX_BUFFER_BASE;
	TxPacket = (u32*) TX_BUFFER_BASE;
	/*Value	 = StartValue;*/

	/* Invalidate the DestBuffer before receiving the data, in case the
	 * Data Cache is enabled */
	for(idx = 0; idx < Length; idx++)
	{
		if (RxPacket[idx] != TxPacket[idx])
		{
			printf("Data Check is Error %d: 0x%X / 0x%X\r\n", idx, RxPacket[idx], TxPacket[idx]);
			return FMSH_FAILURE;
		}
		/*Value = Value + 1;*/
	}

	return FMSH_SUCCESS;
}


#if 1

/*****************************************************************************
 * This is the DMA device init function.
 *
 * @param	AxiDmaPtr is a pointer to the instance of the DMA engine.
 * @param	DeviceId is the DMA device ID.
 *
 * @return
 *		- FMSH_SUCCESS if successful
 *		- FMSH_FAILURE if not succesful
 *
 * @note	None.
 *
 ******************************************************************************/
int Axidma_Init(XAxiDma* AxiDmaPtr, u32 DeviceId)
{
	int Status;
	XAxiDma_Config *Config = NULL;

	Config = XAxiDma_LookupConfig(DeviceId);
	if (!Config) {
		printf("No config found for %d\r\n", DeviceId);
		return FMSH_FAILURE;
	}

	/* Initialize DMA engine */
	Status = XAxiDma_CfgInitialize(AxiDmaPtr, Config);
	if (Status != FMSH_SUCCESS) {
		printf("Initialization failed %d\r\n", Status);
		return FMSH_FAILURE;
	}

	if(XAxiDma_HasSg(AxiDmaPtr)){
		printf("Device configured as SG mode\r\n");
		return FMSH_FAILURE;
	}
	
	return FMSH_SUCCESS;
}


/*****************************************************************************
 * This function setups the interrupt system so interrupts can occur for the
 * DMA, it assumes INTC component exists in the hardware system.
 *
 * @param	InstancePtr is a pointer to the instance of the INTC.
 * @param	AxiDmaPtr is a pointer to the instance of the DMA engine.
 * @param	TxIntrId is the TX channel Interrupt ID.
 * @param	RxIntrId is the RX channel Interrupt ID.
 *
 * @return
 *		- FMSH_SUCCESS if successful
 *		- FMSH_FAILURE if not succesful
 *
 * @note		None.
 *
 ******************************************************************************/
#if 0
int Axidma_Setup_Intr(FGicPs* InstancePtr, XAxiDma* AxiDmaPtr, u32 TxIntrId, u32 RxIntrId)
{
	int Status;
	
	FGicPs_SetPriorityTriggerType(InstancePtr, TxIntrId, 0xA0, 0x3);
	FGicPs_SetPriorityTriggerType(InstancePtr, RxIntrId, 0xA0, 0x3);
	
	/*
	 * Connect the device driver handler that will be called when an
	 * interrupt for the device occurs, the handler defined above performs
	 * the specific interrupt processing for the device.
	 */
	Status = FGicPs_Connect(InstancePtr, TxIntrId, (FMSH_InterruptHandler)Axidma_TxIntrHandler, AxiDmaPtr);
	if (Status != FMSH_SUCCESS) {
		return Status;
	}

	Status = FGicPs_Connect(InstancePtr, RxIntrId, (FMSH_InterruptHandler)Axidma_RxIntrHandler, AxiDmaPtr);
	if (Status != FMSH_SUCCESS) {
		return Status;
	}

	FGicPs_Enable(InstancePtr, TxIntrId);
	FGicPs_Enable(InstancePtr, RxIntrId);
	return FMSH_SUCCESS;
}
#endif




/*****************************************************************************
 * This is the DMA TX Interrupt handler function.
 *
 * It gets the interrupt status from the hardware, acknowledges it, and if any
 * error happens, it resets the hardware. Otherwise, if a completion interrupt
 * is present, then sets the TxDone.flag
 *
 * @param	Callback is a pointer to TX channel of the DMA engine.
 *
 * @return	None.
 *
 * @note	None.
 *
 ******************************************************************************/
static void Axidma_TxIntrHandler(void* Callback)
{
	u32 IrqStatus;
	int TimeOut;
	XAxiDma* AxiDmaInst = (XAxiDma*) Callback;

	/* Read pending interrupts */
	IrqStatus = XAxiDma_IntrGetIrq(AxiDmaInst, XAXIDMA_DMA_TO_DEVICE);

	/* Acknowledge pending interrupts */
	XAxiDma_IntrAckIrq(AxiDmaInst, IrqStatus, XAXIDMA_DMA_TO_DEVICE);

	/* If no interrupt is asserted, we do nothing */
	if (!(IrqStatus & XAXIDMA_IRQ_ALL_MASK)) {
		return;
	}

	/*
	 * If error interrupt is asserted, raise error flag, reset the
	 * hardware to recover from the error, and return with no further
	 * processing.
	 */
	if((IrqStatus & XAXIDMA_IRQ_ERROR_MASK))
	{
		Error = 1;
		
		/* Reset should never fail for transmit channel */
		XAxiDma_Reset(AxiDmaInst);
		TimeOut = RESET_TIMEOUT_COUNTER;
		while(TimeOut) {
			if(XAxiDma_ResetIsDone(AxiDmaInst))
				break;
			TimeOut -= 1;
		}

		return;
	}

	/* If Completion interrupt is asserted, then set the TxDone flag */
	if((IrqStatus & XAXIDMA_IRQ_IOC_MASK))
		TxDone = 1;

	/*logMsg("Axidma_TxIntrHandler: 0x%X \n", Callback, 2,3,4,5,6);*/
	return;
}


/*****************************************************************************
 * This is the DMA RX interrupt handler function
 *
 * It gets the interrupt status from the hardware, acknowledges it, and if any
 * error happens, it resets the hardware. Otherwise, if a completion interrupt
 * is present, then it sets the RxDone flag.
 *
 * @param	Callback is a pointer to RX channel of the DMA engine.
 *
 * @return	None.
 *
 * @note		None.
 ******************************************************************************/
static void Axidma_RxIntrHandler(void* Callback)
{
	u32 IrqStatus;
	int TimeOut;
	XAxiDma* AxiDmaInst = (XAxiDma*)Callback;

	/* Read pending interrupts */
	IrqStatus = XAxiDma_IntrGetIrq(AxiDmaInst, XAXIDMA_DEVICE_TO_DMA);

	/* Acknowledge pending interrupts */
	XAxiDma_IntrAckIrq(AxiDmaInst, IrqStatus, XAXIDMA_DEVICE_TO_DMA);

	/* If no interrupt is asserted, we do not do anything */
	if (!(IrqStatus & XAXIDMA_IRQ_ALL_MASK)) {
		return;
	}

	/*
	 * If error interrupt is asserted, raise error flag, reset the
	 * hardware to recover from the error, and return with no further
	 * processing.
	 */
	if ((IrqStatus & XAXIDMA_IRQ_ERROR_MASK)) {

		Error = 1;

		/* Reset could fail and hang
		 * NEED a way to handle this or do not call it??
		 */
		XAxiDma_Reset(AxiDmaInst);
		TimeOut = RESET_TIMEOUT_COUNTER;
		while (TimeOut) {
			if(XAxiDma_ResetIsDone(AxiDmaInst)) {
				break;
			}

			TimeOut -= 1;
		}

		return;
	}

	/* If completion interrupt is asserted, then set RxDone flag */
	if ((IrqStatus & XAXIDMA_IRQ_IOC_MASK))
		RxDone = 1;
	
	/*logMsg("Axidma_RxIntrHandler: 0x%X \n", Callback, 2,3,4,5,6);*/
	return;
}


/*****************************************************************************
 * This function enable the interrupts for DMA engine.
 *
 * @param	AxiDmaPtr is a pointer to the instance of the DMA engine
 *
 * @return	None.
 *
 ******************************************************************************/
void Axidma_Intr_Enable(XAxiDma *AxiDmaPtr)
{
	/* Disable all interrupts before setup */
	XAxiDma_IntrDisable(AxiDmaPtr, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
	XAxiDma_IntrDisable(AxiDmaPtr, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);

	/* Enable all interrupts */
	XAxiDma_IntrEnable(AxiDmaPtr, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
	XAxiDma_IntrEnable(AxiDmaPtr, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
}


/*****************************************************************************
 * This function disables the interrupts for DMA engine.
 *
 * @param	InstancePtr is the pointer to the INTC component instance
 * @param	TxIntrId is interrupt ID associated w/ DMA TX channel
 * @param	RxIntrId is interrupt ID associated w/ DMA RX channel
 *
 * @return	None.
 *
 * @note	None.
 ******************************************************************************/
/*void Axidma_DisableIntrSystem(FGicPs* InstancePtr, u32 TxIntrId, u32 RxIntrId)
{
	//FGicPs_Disconnect(InstancePtr, TxIntrId);
	//FGicPs_Disconnect(InstancePtr, RxIntrId);
}
*/\
int Axidma_Setup_Intr_2(XAxiDma* AxiDmaPtr, u32 TxIntrId, u32 RxIntrId)
{
	int Status;
	
	/*FGicPs_SetPriorityTriggerType(InstancePtr, TxIntrId, 0xA0, 0x3);*/
	/*FGicPs_SetPriorityTriggerType(InstancePtr, RxIntrId, 0xA0, 0x3);*/
	
	/*
	 * Connect the device driver handler that will be called when an
	 * interrupt for the device occurs, the handler defined above performs
	 * the specific interrupt processing for the device.
	 */
	/*
	Status = FGicPs_Connect(InstancePtr, TxIntrId, (FMSH_InterruptHandler)Axidma_TxIntrHandler, AxiDmaPtr);
	if (Status != FMSH_SUCCESS) {
		return Status;
	}

	Status = FGicPs_Connect(InstancePtr, RxIntrId, (FMSH_InterruptHandler)Axidma_RxIntrHandler, AxiDmaPtr);
	if (Status != FMSH_SUCCESS) {
		return Status;
	}

	FGicPs_Enable(InstancePtr, TxIntrId);
	FGicPs_Enable(InstancePtr, RxIntrId);
	*/
	intConnect(INUM_TO_IVEC(TxIntrId), Axidma_TxIntrHandler, AxiDmaPtr);
	intEnable(TxIntrId);		
	
	intConnect(INUM_TO_IVEC(RxIntrId), Axidma_RxIntrHandler, AxiDmaPtr);
	intEnable(RxIntrId);
	
	return FMSH_SUCCESS;
}

#endif

#if 1   /* hp open & enable*/

UINT32 hpBase = 0;

void pl_hp_init(void)
{
	UINT32 virtAddr;
	int hp_chn = 0;
	
	/* reset hp*/
	slcr_write(0x344, 0x0F);
	slcr_write(0x344, 0x00);
	
	/* reopen lvl_shift*/
	slcr_write(0x838, 0x0F);

	/* ddr hp clk enable*/
	slcr_write(0x218, 0x07);

	/**/
	/* reopen hp channel*/
	/**/
	/*
	virtAddr = pmapGlobalMap (0xE0029000, 0x1000,
							  (MMU_ATTR_SUP_RW | MMU_ATTR_CACHE_OFF | MMU_ATTR_CACHE_GUARDED));
	if (virtAddr == PMAP_FAILED)
	{
		printf ("HP channel init fail! \n");
		return;
	}
	*/
	hpBase = (UINT32)0xE0029000;
	
	hp_chn = 0;
	*((UINT32 *)((hpBase + 0x490) + 0xB0 * hp_chn)) = 0x01;
	hp_chn = 1;
	*((UINT32 *)((hpBase + 0x490) + 0xB0 * hp_chn)) = 0x01;
	hp_chn = 2;
	*((UINT32 *)((hpBase + 0x490) + 0xB0 * hp_chn)) = 0x01;
	hp_chn = 3;
	*((UINT32 *)((hpBase + 0x490) + 0xB0 * hp_chn)) = 0x01;

	return;
}

#endif



