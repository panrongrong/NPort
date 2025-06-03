/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file  fmsh_gmac_example_simple.c
*
* a simple example : rcv then send
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 0.01   Hansen Yang  12/24/2018  First Release
*</pre>
******************************************************************************/

/***************************** Include Files *********************************/
#include <stdlib.h>
/*#include "fmsh_gic.h"*/

#include "./gmacps_v1_0/fmsh_gmac_lib.h"
#include "./gmacps_v1_0/phy/fmsh_mdio.h"

#include "../../common/fmsh_common.h"
/*#include "fmsh_gpio_public.h"*/


#define TRACE_OUT2(flag, string, X1, X2, X3, X4, X5, X6)      \
	do {						   \
		if(flag) {					   \
		logMsg(string, (int)X1, (int)X2, (int)X3, (int)X4, (int)X5, (int)X6);   \
		}						   \
	} while(0)


/************************** Constant Definitions *****************************/
#define ENET_MAC_ADDRESS  0x5a,0x01,0x02,0x03,0x04,0x05
u8 u8_ENET_MAC_ADDRESS[6] = {0x5a,0x01,0x02,0x03,0x04,0x05};

#define GMAC_TESTFRAME_HEAD 0x5A,0x01,0x02,0x03,0x04,0x05,0x5A,0x06,0x07,0x08,0x09,0x0A,0x88,0x00

#define GMAC_TEST_FRAME_DATA_LENGTH  300 /* 0-1500 */
#define GMAC_DEBUG_OUT  (1)

#define GMAC_VERIFY_ID              (FPAR_GMACPS_0_DEVICE_ID)


#define GMAC_RDES_NUM 			(5)		/* number of Rx DES */
										/* for normal test 9212-Byte-length frame, BUFFERSIZE * UNITSIZE * 2 >= 9212 */

#define GMAC_TDES_NUM 			(5)		/* number of Tx DES */

#define GMAC_PACKET_BUFFER_SIZE       (3000)    /* RxPacketBuffer size */


/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/
extern u32 mvl88e1116r_pcs_lpbk(FGmacPs_Instance_T *pGmac, u8 enable,u8 speed);
u8 FGmacPs_IrqHandler(FGmacPs_Instance_T *p_dev);
/************************** Variable Definitions *****************************/
static FGmacPs_Instance_T s_GMAC_Instance =
{
	0
	/*
    .mac_address = {ENET_MAC_ADDRESS},
    .csr_clk=5,
    */
};
	
static FGmacPs_Instance_T       *g_pGMAC_Instance = &s_GMAC_Instance;

static FGmacPs_LinkStatus_T	    s_GMAC_LinkStatus;

static FGmacPs_PhyConfig_T s_GMAC_PhyCfg =
{ 
	0
	/*
    .phy_device = PHY_88E1116R,
    .auto_detect_ad_en = 1,
    .mdio_address = 0,
    .auto_nag_en = 1,
    */
};

static u8 s_tx_done_flag=0;
static u8 s_rcv_flag=0;
static u32 s_status_reg=0;
static u8 s_rcv_frame_cnt=0;

/* place in UnCached section */
/*FGmacPs_RxDescriptor_T GMAC0_RxDs[GMAC_RDES_NUM]@ "GMAC_DES";*/
/*FGmacPs_TxDescriptor_T GMAC0_TxDs[GMAC_TDES_NUM]@ "GMAC_DES";*/
/*FGmacPs_RxDescriptor_T GMAC0_RxDs[GMAC_RDES_NUM];*/
/*FGmacPs_TxDescriptor_T GMAC0_TxDs[GMAC_TDES_NUM];*/


/************************** Function *****************************************/
void preInit_s_GMAC_Instance(void)
{
	int i = 0;
	
	for (i=0; i<6; i++)
	{
		s_GMAC_Instance.mac_address[i] = u8_ENET_MAC_ADDRESS[i];
	}
	
    s_GMAC_Instance.csr_clk = 5;

	return;
}

void preInit_s_GMAC_PhyCfg(void)
{
    s_GMAC_PhyCfg.phy_device = PHY_88E1116R;
    s_GMAC_PhyCfg.auto_detect_ad_en = 1;
	
    /*s_GMAC_PhyCfg.mdio_address = 0;*/
    s_GMAC_PhyCfg.mdio_address = 0xF;
	
    s_GMAC_PhyCfg.auto_nag_en = 1;	

	return;
}


/*  interrupt Func */
void gmac_interrupt_handler(void)
{
    FGmacPs_IrqHandler(g_pGMAC_Instance);
}

u8 FGmacPs_IrqHandler(FGmacPs_Instance_T *pGmac)
{
	u8 rtval;
	FGmacPs_DmaPortMap_T *pDma;
	FMSH_callback   userCallback;
	u32 reg_val_32b=0;
	u32 callbackArg;
	FGmacPs_DmaIrq_T clearIrqMask;

	/*------variables init------------------------------*/
	pDma = (FGmacPs_DmaPortMap_T *)((u32)pGmac->base_address + GMAC_DMA_OFFSET);

	userCallback = NULL;
	callbackArg = 0;
	clearIrqMask = gdma_irq_none;
	rtval = GMAC_RETURN_CODE_OK;

	/* 
	what caused the interrupt?
	*/
	reg_val_32b = FMSH_IN32_32(pDma->GDMA_SR);
    s_status_reg = s_status_reg | reg_val_32b;
	
	if ((reg_val_32b & gdma_irq_aie) != 0)
	{
		userCallback = pGmac->listener;
		if((reg_val_32b&gdma_irq_rx_wd_timeout) != 0)   /* Receive watchdog timeout */
		{
			callbackArg  = gdma_irq_rx_wd_timeout;
			clearIrqMask = gdma_irq_rx_wd_timeout;
		}
		else if((reg_val_32b&gdma_irq_tx_jabber_timeout) != 0)    /* Tansmit jabber timeout */
		{
			callbackArg  = gdma_irq_tx_jabber_timeout;
			clearIrqMask = gdma_irq_tx_jabber_timeout;
		}
		else if((reg_val_32b&gdma_irq_rx_overflow) != 0)   /* Receive FIFO overflow */
		{
			callbackArg  = gdma_irq_rx_overflow;
			clearIrqMask = gdma_irq_rx_overflow;
		}
		else if((reg_val_32b&gdma_irq_rx_unbuffer) != 0)   /* Receive buffer unavailable */
		{
			callbackArg  = gdma_irq_rx_unbuffer;
			clearIrqMask = gdma_irq_rx_unbuffer;
		}
		else if((reg_val_32b&gdma_irq_rx_stop) != 0)   /* Receive Process stopped */
		{
			callbackArg  = gdma_irq_rx_stop;
			clearIrqMask = gdma_irq_rx_stop;
		}
		else if((reg_val_32b&gdma_irq_tx_stop) != 0)   /* Transmit Process stopped */
		{
			callbackArg  = gdma_irq_tx_stop;
			clearIrqMask = gdma_irq_tx_stop;
		}
		else if((reg_val_32b&gdma_irq_early_tx) != 0)   /* Early Transmit interrupt */
		{
			callbackArg  = gdma_irq_early_tx;
			clearIrqMask = gdma_irq_early_tx;
		}
		else if((reg_val_32b&gdma_irq_tx_underflow) != 0)   /* Transmit underflow */
		{
			callbackArg  = gdma_irq_tx_underflow;
			clearIrqMask = gdma_irq_tx_underflow;
		}
		else if((reg_val_32b&gdma_irq_fatal_bus) != 0)   /* Fatal bus error */
		{
			callbackArg  = gdma_irq_fatal_bus;
			clearIrqMask = gdma_irq_fatal_bus;
		}
	}
	else if((reg_val_32b & gdma_irq_nie) != 0)
	{
		if ((reg_val_32b & gdma_irq_rx) != 0)                /* receive interrupt */
		{
			userCallback = pGmac->rxCallback;
			callbackArg  = gdma_irq_rx;
			clearIrqMask = gdma_irq_rx;
			
			logMsg("rx irq callbk! \n", 1,2,3,4,5,6);
		}
		else if ((reg_val_32b & gdma_irq_early_rx) != 0)   /* Early receive interrupt */
		{
			userCallback = pGmac->listener;
			callbackArg  = gdma_irq_early_rx;
			clearIrqMask = gdma_irq_early_rx;
			
			logMsg("listener irq callbk! \n", 1,2,3,4,5,6);
		}
		else if ((reg_val_32b & gdma_irq_tx) != 0)         /* Transmit interrupt */
		{
			userCallback = pGmac->txCallback;
			callbackArg  = gdma_irq_tx;
			clearIrqMask = gdma_irq_tx;
			
			logMsg("tx irq callbk! \n", 1,2,3,4,5,6);
		}
		else if ((reg_val_32b & gdma_irq_tx_unbuffer) != 0)   /* Transmit buffer unavailable */
		{
			userCallback = pGmac->listener;
			callbackArg  = gdma_irq_tx_unbuffer;
			clearIrqMask = gdma_irq_tx_unbuffer;
		}
	}
	else if ((reg_val_32b & gdma_irq_gli) != 0) 				/* GMAC Line Interface Interrupt */
	{
		userCallback = pGmac->listener;
		callbackArg  = gdma_irq_gli;
		clearIrqMask = gdma_irq_none;							/* GLI has been cleared after listener function */
	}
	else
	{
		TRACE_OUT2(GMAC_DEBUG_OUT, "\r\n", 1,2,3,4,5,6);
		TRACE_OUT2(GMAC_DEBUG_OUT, "!!! Unknown Interrupt !!!\r\n", 1,2,3,4,5,6);
		rtval = GMAC_RETURN_CODE_ERR;
		return rtval;
	}

	/* -----call the user listener function------------- */
	if (userCallback != NULL)
	{	
		userCallback(pGmac,callbackArg);
	}

	/*--------clear the serviced interrupt-------------- */
	if (clearIrqMask != 0)
	{
		FGmac_Ps_ClearIrq(pGmac,clearIrqMask);
		
		reg_val_32b = FMSH_IN32_32(pDma->GDMA_SR);
		
		/* 
		clear aie summary
		*/
		if ((reg_val_32b & gdma_irq_all_ai) == 0)
		{
			FMSH_OUT32_32(gdma_irq_aie, pDma->GDMA_SR);
		}
		
		/* clear nie summary */
		if ((reg_val_32b & gdma_irq_all_ni) == 0)
		{
			FMSH_OUT32_32(gdma_irq_nie, pDma->GDMA_SR);
		}
	}
	
	reg_val_32b = FMSH_IN32_32(pDma->GDMA_SR);
	
	return rtval;
}

void FGmacPs_GmacListener(FGmacPs_Instance_T *pGmac,int32_t ecode)
{
	FGmacPs_MacPortMap_T *pGmacPortMap;
	pGmacPortMap = pGmac->base_address;
    FGmacPs_DmaPortMap_T *pDmaPortMap;
	
    pDmaPortMap = (FGmacPs_DmaPortMap_T*)((u32)pGmac->base_address + GMAC_DMA_OFFSET);
	u32 reg;
	
	switch(ecode)
	{
	case gdma_irq_tx_stop:
		TRACE_OUT2(GMAC_DEBUG_OUT, "> Irq:Tx process stopped \r\n", 1,2,3,4,5,6);
		break;
	
	case gdma_irq_tx_unbuffer:
		TRACE_OUT2(GMAC_DEBUG_OUT, "> Irq:Tx Buffer Unavailable \r\n", 1,2,3,4,5,6);
		break;
	
	case gdma_irq_tx_jabber_timeout:
		TRACE_OUT2(GMAC_DEBUG_OUT, "> Irq:Tx jabber timeout \r\n", 1,2,3,4,5,6);
		break;
	
	case gdma_irq_rx_overflow:
		TRACE_OUT2(GMAC_DEBUG_OUT, "> Irq:Rx FIFO overflow \r\n", 1,2,3,4,5,6);
		break;
	
	case gdma_irq_tx_underflow:
		TRACE_OUT2(GMAC_DEBUG_OUT, "> Irq:Tx underflow \r\n", 1,2,3,4,5,6);
		break;
	
	case gdma_irq_rx_unbuffer:
		TRACE_OUT2(GMAC_DEBUG_OUT, "> Irq:Rx buffer unavailable \r\n", 1,2,3,4,5,6);
		TRACE_OUT2(GMAC_DEBUG_OUT, "Reset this Rx DES  \r\n", 1,2,3,4,5,6);
		reg=FMSH_IN32_32(pDmaPortMap->GDMA_CRXDES);
		FGmac_Ps_ResetCurRxDES(pGmac,(FGmacPs_RxDescriptor_T *)reg);
		break;
		
	case gdma_irq_rx_stop:
		TRACE_OUT2(GMAC_DEBUG_OUT, "> Irq:Rx process stopped \r\n", 1,2,3,4,5,6);
		break;
	
	case gdma_irq_rx_wd_timeout:
		TRACE_OUT2(GMAC_DEBUG_OUT, "> Irq:Rx watchdog timeout \r\n", 1,2,3,4,5,6);
		break;
	
	case gdma_irq_early_tx:
		TRACE_OUT2(GMAC_DEBUG_OUT, "> Irq:Early Tx interrupt \r\n", 1,2,3,4,5,6);
		break;
	
	case gdma_irq_fatal_bus:
		TRACE_OUT2(GMAC_DEBUG_OUT, "> Irq:Fatal bus error \r\n", 1,2,3,4,5,6);
		break;
	
	case gdma_irq_early_rx:
		TRACE_OUT2(GMAC_DEBUG_OUT, "> Irq:Early Rx interrupt \r\n", 1,2,3,4,5,6);
		break;
	
	case gdma_irq_gli:  /* not finished */
		TRACE_OUT2(GMAC_DEBUG_OUT, "> Irq:GMAC Line Interface Interrupt \r\n", 1,2,3,4,5,6);
		reg=FMSH_IN32_32(pGmacPortMap->GMAC_ISR);
		if((reg&GMAC_ISR_RGSMIIIS) != 0)
		{
			TRACE_OUT2(GMAC_DEBUG_OUT, "RGMII or SMII Interrupt, Link status change \r\n", 1,2,3,4,5,6);
			FGmac_Ps_GetLinkStatus(pGmac);  /* GLI will be cleared when read these bits */
		}
		break;
		
	default:
		TRACE_OUT2(GMAC_DEBUG_OUT, "> unexpected argument: 0x%x \r\n", ecode, 2,3,4,5,6);
		break;
	}

	return;
}

void FGmacPs_GmacRxCallback(FGmacPs_Instance_T *pGmac, int32_t ecode)
{
	uint32_t frmSize;
	u8      *pBuffer;
	u8 j =0;
	u8 isBroadcast = 0;
	u32 poll_result;
	pBuffer  = pGmac->pFrmBuffer;

	TRACE_OUT2(GMAC_DEBUG_OUT, "< Irq: Recevie interrupt \r\n", 1,2,3,4,5,6);
	
	poll_result = FGmac_Ps_RcvPoll(pGmac,&frmSize);
	pGmac->RxFrameSize = frmSize;
	
	s_rcv_flag = 1;
	s_rcv_frame_cnt++;

	if (poll_result == GMAC_RETURN_CODE_OK)
	{
		TRACE_OUT2(GMAC_DEBUG_OUT, "A Frame is recevied successfully,size is %d !\r\n", frmSize, 2,3,4,5,6);
		
#if 0 /* (GMAC_DEBUG_OUT)  // jc*/
        for(u32 tmp_i=0;tmp_i<frmSize;tmp_i++)
        {    
        	TRACE_OUT2(GMAC_DEBUG_OUT, "%x ",pBuffer[tmp_i]);
        }
#endif
	}
	else if(poll_result == GMAC_RETURN_CODE_RX_NULL)
	{
		TRACE_OUT2(GMAC_DEBUG_OUT, "Rx Frame Error: No data..\r\n", 1,2,3,4,5,6);
		return;
	}
	else if(poll_result == GMAC_RETURN_CODE_SIZE_TOO_SMALL)
	{
		TRACE_OUT2(GMAC_DEBUG_OUT, "Rx Frame Error: Buffer size is too small..\r\n", 1,2,3,4,5,6);
		return;
	}

	/*** Detect which cast: unicast,multicast,broadcast ***/
	while (pBuffer[j] == 0xFF)
	{
		j++;
		if(j== 6)
		{
			isBroadcast = 1;
			
			break;
		}
	}
	
	if (isBroadcast)
	{
		TRACE_OUT2(GMAC_DEBUG_OUT, "Received a broad-cast frame.\r\n", 1,2,3,4,5,6);
	}
	else if ((pBuffer[0]&0x01) == 0x01)
	{
		TRACE_OUT2(GMAC_DEBUG_OUT, "Received a multi-cast frame.\r\n", 1,2,3,4,5,6);
	}
	else 
	{
		TRACE_OUT2(GMAC_DEBUG_OUT, "Received a uni-cast frame.\r\n", 1,2,3,4,5,6);
	}

	return;
}

void FGmacPs_GmacTxCallback(FGmacPs_Instance_T *pGmac,int32_t ecode)
{
	TRACE_OUT2(GMAC_DEBUG_OUT, "> Irq: Transmit interrupt \r\n", 1,2,3,4,5,6);
	TRACE_OUT2(GMAC_DEBUG_OUT, "Sending frame is done!\r\n", 1,2,3,4,5,6);
	
	s_tx_done_flag = 1;

	return;
}

void FGmacPs_88e1116rHwRst()
{
    /* 
    88e1116r HW reset 
	*/
    FGpioPs_T gpioDevB;
    FGpioPs_Config *pGpioCfg;
	
    pGpioCfg = FGpioPs_LookupConfig(FPAR_GPIOPS_1_DEVICE_ID);
    FGpioPs_init(&gpioDevB, pGpioCfg);
	
    FGpioPs_setDirection(&gpioDevB, 0xffffffff);  /*1-out  0-in*/
    FGpioPs_writeData(&gpioDevB, 0xffffffff);
	
    FGpioPs_writeData(&gpioDevB, 0);
	
    /*delay_ms(200);*/
    vxbMsDelay(200);
	
    FGpioPs_writeData(&gpioDevB, 0xffffffff);

	return;
}


#define GMAC0_INT_ID 		51U
#define GMAC1_INT_ID		72U 

#define FGicPs_ENABLE_SET_OFFSET	0x00000100U  /* Enable Set Register */
#define FPAR_SCUGIC_DIST_BASEADDR	(FPS_GIC_BASEADDR + 0x00001000U)

void FGicPs_Enable2(u32 Int_Id)
{
	u32 Mask;
	u32 addr = 0;

	/*
	 * Assert the arguments
	 */
	/*
	FMSH_ASSERT(InstancePtr != NULL);
	FMSH_ASSERT(Int_Id < FGicPs_MAX_NUM_INTR_INPUTS);
    FMSH_ASSERT(InstancePtr->IsReady == COMPONENT_IS_READY);
	*/	
	
	/*
	 * The Int_Id is used to create the appropriate mask for the
	 * desired bit position. Int_Id currently limited to 0 - 31
	 */
	Mask = 0x00000001U << (Int_Id % 32U);

	/*
	 * Enable the selected interrupt source by setting the
	 * corresponding bit in the Enable Set register.
	 */
	/*FGicPs_DistWriteReg(InstancePtr, (u32)FGicPs_ENABLE_SET_OFFSET + ((Int_Id / 32U) *4U), Mask);*/
	addr = FPAR_SCUGIC_DIST_BASEADDR + (u32)FGicPs_ENABLE_SET_OFFSET + ((Int_Id / 32U) * 4U), 
	*(u32*)addr = Mask;

	return;
}

int g_first_gmac_test = 0;
/****************************************************************************/
/**
*
* gmac0 simple example, 88e1116r PCS loopback.
* Tx send a frame, phy echo, Rx rcv and check the frame.
*
* @param    None.
*
* @return   success: 0, fail: 1
*
****************************************************************************/
u8 test_gmac_loop_phy(void)
{
	if (g_first_gmac_test == 0)
	{			
		/**/
		/* pre-init for parameters*/
		/**/
		preInit_s_GMAC_Instance();
		preInit_s_GMAC_PhyCfg();

		/**/
		/* reset phy: 88e1116r*/
		/**/
	    FGmacPs_88e1116rHwRst();

	    u8* GMAC0_PacketBuffer;
		
	    FGmacPs_RxDescriptor_T * GMAC0_RxDs;	
	    u8* GMAC0_RxBuffer;
		
	    FGmacPs_TxDescriptor_T * GMAC0_TxDs;	
	    u8* GMAC0_TxBuffer;

	    /*
	    malloc mem 
		*/
	    /*GMAC0_PacketBuffer = (u8 *malloc(GMAC_PACKET_BUFFER_SIZE);*/
	    GMAC0_PacketBuffer = (u8 *)0x20000000;
	    if (GMAC0_PacketBuffer == NULL)
		{
	        TRACE_OUT2(GMAC_DEBUG_OUT,"gmac lib malloc fail\r\n", 1,2,3,4,5,6);
	        return GMAC_RETURN_CODE_ERR;
	    }

		/**/
		/* dma rx_descriptor*/
		/**/
	    /*GMAC0_RxDs = (FGmacPs_RxDescriptor_T *malloc(sizeof(FGmacPs_RxDescriptor_T)*MAC_RDES_NUM);*/
	    /*GMAC0_RxDs = (FGmacPs_RxDescriptor_T *0x20100000;*/
	    GMAC0_RxDs = (FGmacPs_RxDescriptor_T *)FPS_AHB_SRAM_BASEADDR;
	    if (GMAC0_RxDs == NULL)
		{
	        TRACE_OUT2(GMAC_DEBUG_OUT,"gmac lib malloc fail\r\n", 1,2,3,4,5,6);
	        return GMAC_RETURN_CODE_ERR;
	    }

		/**/
		/* dma rx_buf*/
		/**/
	    /*GMAC0_RxBuffer = (u8 *malloc(2*MAC_RDES_NUM*MAC_RBUFFER_UNIT_SIZE);*/
	    GMAC0_RxBuffer = (u8 *)0x20200000;
	    if (GMAC0_RxBuffer == NULL)
		{
	        TRACE_OUT2(GMAC_DEBUG_OUT,"gmac lib malloc fail\r\n", 1,2,3,4,5,6);
	        return GMAC_RETURN_CODE_ERR;
	    }

		/**/
		/* dma tx_descriptor*/
		/**/
	    GMAC0_TxDs = (FGmacPs_TxDescriptor_T *)malloc(sizeof(FGmacPs_TxDescriptor_T)*GMAC_RDES_NUM);
	    /*GMAC0_TxDs = (FGmacPs_TxDescriptor_T *0x20300000;*/
	    GMAC0_TxDs = (FGmacPs_TxDescriptor_T *)(FPS_AHB_SRAM_BASEADDR + 0x10000);
	    if (GMAC0_TxDs==NULL)
		{
	        TRACE_OUT2(GMAC_DEBUG_OUT,"gmac lib malloc fail\r\n", 1,2,3,4,5,6);
	        return GMAC_RETURN_CODE_ERR;
	    }

		/**/
		/* dma tx_buf*/
		/**/
	    /*GMAC0_TxBuffer = (u8*malloc(2*MAC_TDES_NUM*MAC_TBUFFER_UNIT_SIZE);*/
	    GMAC0_TxBuffer = (u8*)0x20400000;
	    if (GMAC0_TxBuffer==NULL)
		{
	        TRACE_OUT2(GMAC_DEBUG_OUT,"gmac lib malloc fail\r\n", 1,2,3,4,5,6);
	        return GMAC_RETURN_CODE_ERR;
	    }


	    /* 
	    setup gmac 
		*/
	    FGmacPs_Config_T *pGmacCfg;
	    pGmacCfg = FGmacPs_LookupConfig(FPAR_GMACPS_0_DEVICE_ID);
		
	    FGmac_Ps_StructInit(g_pGMAC_Instance,
	                        &s_GMAC_LinkStatus,
	                        pGmacCfg,
	                        &s_GMAC_PhyCfg,
	                        GMAC_RDES_NUM, GMAC_TDES_NUM,
	                        GMAC_RBUFFER_UNIT_SIZE, GMAC_TBUFFER_UNIT_SIZE,
	                        GMAC0_PacketBuffer, GMAC_PACKET_BUFFER_SIZE);
		
	    FGmac_Ps_DeviceReset(g_pGMAC_Instance);
		
	    FGmac_Ps_DmaInit(g_pGMAC_Instance,
	                     GMAC0_RxDs,
	                     GMAC0_RxBuffer,
	                     GMAC0_TxDs,
	                     GMAC0_TxBuffer);
		
		/**/
		/* flush cache, it's must be for ddr flush new data*/
		/**/
		(void)cacheInvalidate (DATA_CACHE, 0x20000000, 0x1000000);   /* 16M ddr flush*/
		
		FGmac_Ps_MacInit(g_pGMAC_Instance);
		
		FGmac_Ps_SetListener(g_pGMAC_Instance,(FMSH_callback)FGmacPs_GmacListener);
		FGmac_Ps_SetTxCallback(g_pGMAC_Instance,(FMSH_callback)FGmacPs_GmacTxCallback);
		FGmac_Ps_SetRxCallback(g_pGMAC_Instance,(FMSH_callback)FGmacPs_GmacRxCallback);

	    /*
	    gic: setup irq
	    */
#if 0  /* bare-code in iar*/
	    FGicPs_SetupInterruptSystem(&IntcInstance);
	    FGicPs_Connect(&IntcInstance,GMAC0_INT_ID,(FMSH_InterruptHandler)gmac_interrupt_handler,0);
	    FMSH_ExceptionRegisterHandler(FMSH_EXCEPTION_ID_FIQ_INT,(FMSH_ExceptionHandler)FGicPs_InterruptHandler_FIQ,&IntcInstance);
	    FGicPs_Enable(&IntcInstance,GMAC0_INT_ID);
		
#else  /* in vx*/

		/*intConnect(INUM_TO_IVEC(INT_VEC_EMAC0), gmac_interrupt_handler, NULL);*/
		/*intEnable(INT_VEC_EMAC0);	*/
		intConnect(INUM_TO_IVEC(51), gmac_interrupt_handler, NULL);
		intEnable(51);	
		
	    FGicPs_Enable2(GMAC0_INT_ID);	
#endif
		

	    /* 
	    enable phy PCS loopback 
		*/
	    FGmacPs_Speed LinkSpeed = FPAR_GMACPS_0_SPEED;
	    if (LinkSpeed == speed_10)
	    {
	        mvl88e1116r_pcs_lpbk(g_pGMAC_Instance, 1, 0);
	    }
		else if (LinkSpeed == speed_100)
	    {
	        mvl88e1116r_pcs_lpbk(g_pGMAC_Instance, 1, 1);
	    }
		else if (LinkSpeed == speed_1000)
	    {
	        mvl88e1116r_pcs_lpbk(g_pGMAC_Instance, 1, 2);
	    } 

		/*g_first_gmac_test = 1;*/
	}

    /* 
    setup tx frame 
	*/
    u8 mac_frame[GMAC_TEST_FRAME_DATA_LENGTH + 14] = {GMAC_TESTFRAME_HEAD};
	int i = 0;
	
    for (i=0; i<GMAC_TEST_FRAME_DATA_LENGTH; i++)
    {
        mac_frame[14+i] = (u8)i + (g_first_gmac_test);
    }	
	g_first_gmac_test++;
	
	/**/
	/* flush cache, it's must be for ddr flush new data*/
	/**/
	(void)cacheInvalidate (DATA_CACHE, 0x20000000, 0x1000000);   /* 16M ddr flush*/
	
    /* 
    send one frame 
	*/
    FGmac_Ps_Send(g_pGMAC_Instance, mac_frame, GMAC_TEST_FRAME_DATA_LENGTH+14, 0, 0);

    /* 
    wait for tx interrupt 
	*/
    u32 timeout = 0;
    while (0 == s_tx_done_flag)  /* waiting tx ok irq flag*/
    {
        /*if (timeout > 100)*/
        if (timeout > 60)
        {
            printf("GMAC TX interrupt time out.\n");
            return -1;
        }		
        timeout++;
		
        /*delay_ms(1);*/
        taskDelay(1);
    }
    s_tx_done_flag=0;


    /* 
    wait for rx interrupt 
	*/
    timeout = 0;
    while (0 == s_rcv_flag)  /* waiting rx data irq flag*/
    {
        /*if (timeout>100)*/
        if (timeout > 120)
        {
            printf("GMAC RX interrupt time out.\n");
            return -1;
        }		
        timeout++;
		
        /*delay_ms(1);*/
		taskDelay(1);
    }
    s_rcv_flag = 0;

	
	/**/
	/* flush cache, it's must be for ddr flush new data*/
	/**/
	(void)cacheInvalidate (DATA_CACHE, 0x20000000, 0x1000000);   /* 16M ddr flush*/
	
    /* 
    check data 
	*/
	u8 check_result = 0;
    for (i=0; i<GMAC_TEST_FRAME_DATA_LENGTH+14; i++)
	{
        if (*((u8*)(g_pGMAC_Instance->pFrmBuffer+i)) != mac_frame[i])
		{
            check_result = 1;
            break;
        }
    }

	if (check_result == 1)
	{
		printf("\n");
		printf("gmac tx&rx raw data by phy_loop: \n\n");

		/**/
		/* frame-header*/
		/**/
		for (i=0; i<(14); i++)
		{
			printf("%02X", mac_frame[i]);

			/* " "*/
			if (((i+1) % 4) == 0)
			{
				printf(" ");
			}
		}
		printf("\n");

		/**/
		/* frame-data*/
		/**/
		for (i=14; i<(GMAC_TEST_FRAME_DATA_LENGTH+14); i++)
		{
			printf("%02X", mac_frame[i]);

			/* " "*/
			if ((((i-14)+1) % 4) == 0)
			{
				printf(" ");
			}
			
			/* "\n"*/
			if ((((i-14)+1) % 64) == 0)
			{
				printf("\n");
			}
		}
		
		printf("\n----data_end----\n\n");		
	}
	else
	{
		printf("\n gmac tx&rx raw data by phy_loop check fail!!! \n\n");
	}
	
    printf("gmac check result:%d\r\n", check_result);
	
    return check_result;
}

/*
rj45: 
	1-tx_+, 2-tx_-; 3-rx_+, 6-rx_-; 
loop: 
	1-tx_+ <==> 3-rx_+; 2-tx_- <==> 6-rx_-	
	4 <==> 7; 6 <==> 8
*/
u8 test_gmac_loop_line(void)
{
	if (g_first_gmac_test == 0)
	{			
		/**/
		/* pre-init for parameters*/
		/**/
		preInit_s_GMAC_Instance();
		preInit_s_GMAC_PhyCfg();

		/**/
		/* reset phy: 88e1116r*/
		/**/
	    FGmacPs_88e1116rHwRst();

	    u8* GMAC0_PacketBuffer;
		
	    FGmacPs_RxDescriptor_T * GMAC0_RxDs;	
	    u8* GMAC0_RxBuffer;
		
	    FGmacPs_TxDescriptor_T * GMAC0_TxDs;	
	    u8* GMAC0_TxBuffer;

	    /*
	    malloc mem 
		*/
	    /*GMAC0_PacketBuffer = (u8 *malloc(GMAC_PACKET_BUFFER_SIZE);*/
	    GMAC0_PacketBuffer = (u8 *)0x20000000;
	    if (GMAC0_PacketBuffer == NULL)
		{
	        TRACE_OUT2(GMAC_DEBUG_OUT,"gmac lib malloc fail\r\n", 1,2,3,4,5,6);
	        return GMAC_RETURN_CODE_ERR;
	    }

		/**/
		/* dma rx_descriptor*/
		/**/
	    /*GMAC0_RxDs = (FGmacPs_RxDescriptor_T *malloc(sizeof(FGmacPs_RxDescriptor_T)*MAC_RDES_NUM);*/
	    /*GMAC0_RxDs = (FGmacPs_RxDescriptor_T *0x20100000;*/
	    GMAC0_RxDs = (FGmacPs_RxDescriptor_T *)FPS_AHB_SRAM_BASEADDR;
	    if (GMAC0_RxDs == NULL)
		{
	        TRACE_OUT2(GMAC_DEBUG_OUT,"gmac lib malloc fail\r\n", 1,2,3,4,5,6);
	        return GMAC_RETURN_CODE_ERR;
	    }

		/**/
		/* dma rx_buf*/
		/**/
	    /*GMAC0_RxBuffer = (u8 *malloc(2*MAC_RDES_NUM*MAC_RBUFFER_UNIT_SIZE);*/
	    GMAC0_RxBuffer = (u8 *)0x20200000;
	    if (GMAC0_RxBuffer == NULL)
		{
	        TRACE_OUT2(GMAC_DEBUG_OUT,"gmac lib malloc fail\r\n", 1,2,3,4,5,6);
	        return GMAC_RETURN_CODE_ERR;
	    }

		/**/
		/* dma tx_descriptor*/
		/**/
	    GMAC0_TxDs = (FGmacPs_TxDescriptor_T *)malloc(sizeof(FGmacPs_TxDescriptor_T)*GMAC_RDES_NUM);
	    /*GMAC0_TxDs = (FGmacPs_TxDescriptor_T *0x20300000;*/
	    GMAC0_TxDs = (FGmacPs_TxDescriptor_T *)(FPS_AHB_SRAM_BASEADDR + 0x10000);
	    if (GMAC0_TxDs==NULL)
		{
	        TRACE_OUT2(GMAC_DEBUG_OUT,"gmac lib malloc fail\r\n", 1,2,3,4,5,6);
	        return GMAC_RETURN_CODE_ERR;
	    }

		/**/
		/* dma tx_buf*/
		/**/
	    /*GMAC0_TxBuffer = (u8*malloc(2*MAC_TDES_NUM*MAC_TBUFFER_UNIT_SIZE);*/
	    GMAC0_TxBuffer = (u8*)0x20400000;
	    if (GMAC0_TxBuffer==NULL)
		{
	        TRACE_OUT2(GMAC_DEBUG_OUT,"gmac lib malloc fail\r\n", 1,2,3,4,5,6);
	        return GMAC_RETURN_CODE_ERR;
	    }


	    /* 
	    setup gmac 
		*/
	    FGmacPs_Config_T *pGmacCfg;
	    pGmacCfg = FGmacPs_LookupConfig(FPAR_GMACPS_0_DEVICE_ID);
		
	    FGmac_Ps_StructInit(g_pGMAC_Instance,
	                        &s_GMAC_LinkStatus,
	                        pGmacCfg,
	                        &s_GMAC_PhyCfg,
	                        GMAC_RDES_NUM, GMAC_TDES_NUM,
	                        GMAC_RBUFFER_UNIT_SIZE, GMAC_TBUFFER_UNIT_SIZE,
	                        GMAC0_PacketBuffer, GMAC_PACKET_BUFFER_SIZE);
		
	    FGmac_Ps_DeviceReset(g_pGMAC_Instance);
		
	    FGmac_Ps_DmaInit(g_pGMAC_Instance,
	                     GMAC0_RxDs,
	                     GMAC0_RxBuffer,
	                     GMAC0_TxDs,
	                     GMAC0_TxBuffer);
		
		/**/
		/* flush cache, it's must be for ddr flush new data*/
		/**/
		(void)cacheInvalidate (DATA_CACHE, 0x20000000, 0x1000000);   /* 16M ddr flush*/
		
		FGmac_Ps_MacInit(g_pGMAC_Instance);
		
		FGmac_Ps_SetListener(g_pGMAC_Instance,(FMSH_callback)FGmacPs_GmacListener);
		FGmac_Ps_SetTxCallback(g_pGMAC_Instance,(FMSH_callback)FGmacPs_GmacTxCallback);
		FGmac_Ps_SetRxCallback(g_pGMAC_Instance,(FMSH_callback)FGmacPs_GmacRxCallback);

	    /*
	    gic: setup irq
	    */
#if 0  /* bare-code in iar*/
	    FGicPs_SetupInterruptSystem(&IntcInstance);
	    FGicPs_Connect(&IntcInstance,GMAC0_INT_ID,(FMSH_InterruptHandler)gmac_interrupt_handler,0);
	    FMSH_ExceptionRegisterHandler(FMSH_EXCEPTION_ID_FIQ_INT,(FMSH_ExceptionHandler)FGicPs_InterruptHandler_FIQ,&IntcInstance);
	    FGicPs_Enable(&IntcInstance,GMAC0_INT_ID);
		
#else  /* in vx*/

		/*intConnect(INUM_TO_IVEC(INT_VEC_EMAC0), gmac_interrupt_handler, NULL);*/
		/*intEnable(INT_VEC_EMAC0);	*/
		intConnect(INUM_TO_IVEC(51), gmac_interrupt_handler, NULL);
		intEnable(51);	
		
	    FGicPs_Enable2(GMAC0_INT_ID);	
#endif

		/**/
		/* ext line loop*/
		/**/
		mvl88e1116r_line_extloop(g_pGMAC_Instance, 1);
				
		/*g_first_gmac_test = 1;*/
	}

    /* 
    setup tx frame 
	*/
    u8 mac_frame[GMAC_TEST_FRAME_DATA_LENGTH + 14] = {GMAC_TESTFRAME_HEAD};
	int i = 0;
	
    for (i=0; i<GMAC_TEST_FRAME_DATA_LENGTH; i++)
    {
        mac_frame[14+i] = (u8)i + (g_first_gmac_test);
    }	
	g_first_gmac_test++;
	
	/**/
	/* flush cache, it's must be for ddr flush new data*/
	/**/
	(void)cacheInvalidate (DATA_CACHE, 0x20000000, 0x1000000);   /* 16M ddr flush*/
	
    /* 
    send one frame 
	*/
    FGmac_Ps_Send(g_pGMAC_Instance, mac_frame, GMAC_TEST_FRAME_DATA_LENGTH+14, 0, 0);

    /* 
    wait for tx interrupt 
	*/
    u32 timeout = 0;
    while (0 == s_tx_done_flag)  /* waiting tx ok irq flag*/
    {
        /*if (timeout > 100)*/
        if (timeout > 60)
        {
            printf("GMAC TX interrupt time out.\n");
            return -1;
        }		
        timeout++;
		
        /*delay_ms(1);*/
        taskDelay(1);
    }
    s_tx_done_flag=0;


    /* 
    wait for rx interrupt 
	*/
    timeout = 0;
    while (0 == s_rcv_flag)  /* waiting rx data irq flag*/
    {
        /*if (timeout>100)*/
        if (timeout > 120)
        {
            printf("GMAC RX interrupt time out.\n");
            return -1;
        }		
        timeout++;
		
        /*delay_ms(1);*/
		taskDelay(1);
    }
    s_rcv_flag = 0;

	
	/**/
	/* flush cache, it's must be for ddr flush new data*/
	/**/
	(void)cacheInvalidate (DATA_CACHE, 0x20000000, 0x1000000);   /* 16M ddr flush*/
	
    /* 
    check data 
	*/
	u8 check_result = 0;
    for (i=0; i<GMAC_TEST_FRAME_DATA_LENGTH+14; i++)
	{
        if (*((u8*)(g_pGMAC_Instance->pFrmBuffer+i)) != mac_frame[i])
		{
            check_result = 1;
            break;
        }
    }

	if (check_result == 1)
	{
		printf("\n");
		printf("gmac tx&rx raw data by ext-line_loop: \n\n");

		/**/
		/* frame-header*/
		/**/
		for (i=0; i<(14); i++)
		{
			printf("%02X", mac_frame[i]);

			/* " "*/
			if (((i+1) % 4) == 0)
			{
				printf(" ");
			}
		}
		printf("\n");

		/**/
		/* frame-data*/
		/**/
		for (i=14; i<(GMAC_TEST_FRAME_DATA_LENGTH+14); i++)
		{
			printf("%02X", mac_frame[i]);

			/* " "*/
			if ((((i-14)+1) % 4) == 0)
			{
				printf(" ");
			}
			
			/* "\n"*/
			if ((((i-14)+1) % 64) == 0)
			{
				printf("\n");
			}
		}
		
		printf("\n----data_end----\n\n");		
	}
	else
	{
		printf("\n gmac tx&rx raw data by ext-line_loop check fail!!! \n\n");
	}
	
    printf("gmac check result:%d\r\n", check_result);
	
    return check_result;
}

