#include "lwip_demo.h"
#include "ethernet_interface.h"
#include "fmsh_gmac_hw.h"
#include "fmsh_gmac_lib.h"
#include "fmsh_mdio.h"
#include "fmsh_gic.h"
#include "fmsh_gic_hw.h"
#include "fmsh_gpio_public.h"
#include "fmsh_ps_parameters.h"
#include <stdio.h>
#include "cache.h"
#include "lwip/udp.h"
/************************** Constant Definitions *****************************/
#define ENET_MAC_ADDRESS 0x5a,0x01,0x02,0x03,0x04,0x05

#define DEBUG_OUT_EHT FMQL_LWIP_DEBUG

/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 'n'

#define GMAC_RDES_NUM 			512		/* number of Rx DES */
/* for normal test 9212-Byte-length frame, BUFFERSIZE * UNITSIZE * 2 >= 9212 */
#define GMAC_TDES_NUM 			512		/* number of Tx DES */

#define GMAC_PACKET_BUFFER_SIZE       3000    /* RxPacketBuffer size */
#define GMAC_TXPACKET_BUFFER_SIZE       1600    /* TxPacketBuffer size */

#define SGI_ID          0U
#define CPU_ID          1U
/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/
#define TRACE_OUT(flag, ...)      \
do {                           \
    if(flag) {                     \
        printf(__VA_ARGS__);   \
    }                          \
} while(0)

/************************** Function Prototypes ******************************/
u8 FGmacPs_IrqHandler(FGmacPs_Instance_T *pGmac);
void *FGmac_Ps_RcvPollEFrame(FGmacPs_Instance_T *pGmac,u32 *pRcvSize);
u32 g_rcv_cnt=0;
int tx_done=0;
/************************** Variable Definitions *****************************/
extern struct netif Gmac_LwIP_if;

static FGmacPs_Instance_T 	    s_GMAC_Instance={
    .mac_address={ENET_MAC_ADDRESS},
    .csr_clk=5,
};
FGmacPs_Instance_T *s_GMAC_inst_lwip_p=&s_GMAC_Instance;

static FGmacPs_LinkStatus_T	    s_GMAC_LinkStatus;
static FGmacPs_PhyConfig_T	    s_GMAC_PhyCfg={
    .phy_device=PHY_88E1116R,
    .auto_detect_ad_en=1,
    .mdio_address=0,
    .auto_nag_en=1,
};
/**
* Helper struct to hold private data used to operate your ethernet interface.
* Keeping the ethernet address of the MAC in this struct is not necessary
* as it is already kept in the struct netif.
* But this is only an example, anyway...
*/
struct ethernetif {
	struct eth_addr *ethaddr;
	/* Add whatever per-interface state that is needed here. */
};

/* place in UnCached section */
FGmacPs_RxDescriptor_T GMAC0_RxDs[GMAC_RDES_NUM]@ "GMAC_DES";
FGmacPs_TxDescriptor_T GMAC0_TxDs[GMAC_TDES_NUM]@ "GMAC_DES";
/**************************  Function ***********************************/
#if (RCV_POLL==0)
u32  gicTestFlag = GIC_FAILURE;
void SGI0_hanlder (void *InstancePtr);
u32  FGicPs_SelfTest(FGicPs *InstancePtr)
{
    u32 Status;
    Status =  FGicPs_SetupInterruptSystem(InstancePtr);
    if(Status!=GIC_SUCCESS)
    {
        return GIC_FAILURE ;
    }

    Status = FGicPs_Connect(InstancePtr,SGI_ID,
                            (FMSH_InterruptHandler)SGI0_hanlder,InstancePtr );
    if (Status != GIC_SUCCESS) {
        return GIC_FAILURE;
    }

    FMSH_ExceptionRegisterHandler(FMSH_EXCEPTION_ID_IRQ_INT,
                                  (FMSH_ExceptionHandler)FGicPs_InterruptHandler_IRQ,
                                  InstancePtr);

    FGicPs_Enable(InstancePtr, SGI_ID);
    FGicPs_SoftwareIntr(InstancePtr,SGI_ID,CPU_ID);
    return gicTestFlag;
}


void SGI0_hanlder (void *InstancePtr)
{
    gicTestFlag = GIC_SUCCESS;
}

void gmac_interrupt_handler(void)
{
    FGmacPs_IrqHandler(s_GMAC_inst_lwip_p);
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

	/* what caused the interrupt?*/
	reg_val_32b = FMSH_IN32_32(pDma->GDMA_SR);
	if((reg_val_32b & gdma_irq_aie) != 0)
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
		if((reg_val_32b&gdma_irq_rx) != 0)   /* receive interrupt */
		{
			userCallback = pGmac->rxCallback;
			callbackArg  = gdma_irq_rx;
			clearIrqMask = gdma_irq_rx;
		}
		else if((reg_val_32b&gdma_irq_early_rx) != 0)   /* Early receive interrupt */
		{
			userCallback = pGmac->listener;
			callbackArg  = gdma_irq_early_rx;
			clearIrqMask = gdma_irq_early_rx;
		}
		else if((reg_val_32b&gdma_irq_tx) != 0)   /* Transmit interrupt */
		{
			userCallback = pGmac->txCallback;
			callbackArg  = gdma_irq_tx;
			clearIrqMask = gdma_irq_tx;
		}
		else if((reg_val_32b&gdma_irq_tx_unbuffer) != 0)   /* Transmit buffer unavailable */
		{
			userCallback = pGmac->listener;
			callbackArg  = gdma_irq_tx_unbuffer;
			clearIrqMask = gdma_irq_tx_unbuffer;
		}
	}
	else if((reg_val_32b & gdma_irq_gli) != 0) 				/* GMAC Line Interface Interrupt */
	{
		userCallback = pGmac->listener;
		callbackArg  = gdma_irq_gli;
		clearIrqMask = gdma_irq_none;							/* GLI has been cleared after listener function */
	}
	else
	{
		TRACE_OUT(DEBUG_OUT_EHT, "\r\n");
		TRACE_OUT(DEBUG_OUT_EHT, "!!! Unknown Interrupt !!!\r\n");
		rtval = GMAC_RETURN_CODE_ERR;
		return rtval;
	}

	/* -----call the user listener function------------- */
	if(userCallback != NULL)
		userCallback(pGmac,callbackArg);

	/*--------clear the serviced interrupt-------------- */
	if(clearIrqMask != 0)
	{
		FGmac_Ps_ClearIrq(pGmac,clearIrqMask);
		reg_val_32b = FMSH_IN32_32(pDma->GDMA_SR);
		/* clear aie summary */
		if((reg_val_32b&gdma_irq_all_ai)==0)
		{
			FMSH_OUT32_32(gdma_irq_aie,pDma->GDMA_SR);
		}
		/* clear nie summary */
		if((reg_val_32b&gdma_irq_all_ni)==0)
		{
			FMSH_OUT32_32(gdma_irq_nie,pDma->GDMA_SR);
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
		TRACE_OUT(DEBUG_OUT_EHT, "> Irq:Tx process stopped \r\n");
		break;
	case gdma_irq_tx_unbuffer:
		TRACE_OUT(DEBUG_OUT_EHT, "> Irq:Tx Buffer Unavailable \r\n");
		break;
	case gdma_irq_tx_jabber_timeout:
		TRACE_OUT(DEBUG_OUT_EHT, "> Irq:Tx jabber timeout \r\n");
		break;
	case gdma_irq_rx_overflow:
		TRACE_OUT(DEBUG_OUT_EHT, "> Irq:Rx FIFO overflow \r\n");
		break;
	case gdma_irq_tx_underflow:
		TRACE_OUT(DEBUG_OUT_EHT, "> Irq:Tx underflow \r\n");
		break;
	case gdma_irq_rx_unbuffer:
		TRACE_OUT(DEBUG_OUT_EHT, "> Irq:Rx buffer unavailable \r\n");
		TRACE_OUT(DEBUG_OUT_EHT, "Reset this Rx DES  \r\n");
		reg=FMSH_IN32_32(pDmaPortMap->GDMA_CRXDES);
		FGmac_Ps_ResetCurRxDES(s_GMAC_inst_lwip_p,(FGmacPs_RxDescriptor_T *)reg);
		break;
	case gdma_irq_rx_stop:
		TRACE_OUT(DEBUG_OUT_EHT, "> Irq:Rx process stopped \r\n");
		break;
	case gdma_irq_rx_wd_timeout:
		TRACE_OUT(DEBUG_OUT_EHT, "> Irq:Rx watchdog timeout \r\n");
		break;
	case gdma_irq_early_tx:
		TRACE_OUT(DEBUG_OUT_EHT, "> Irq:Early Tx interrupt \r\n");
		break;
	case gdma_irq_fatal_bus:
		TRACE_OUT(DEBUG_OUT_EHT, "> Irq:Fatal bus error \r\n");
		break;
	case gdma_irq_early_rx:
		TRACE_OUT(DEBUG_OUT_EHT, "> Irq:Early Rx interrupt \r\n");
		break;
	case gdma_irq_gli:  /* not finished */
		TRACE_OUT(DEBUG_OUT_EHT, "> Irq:GMAC Line Interface Interrupt \r\n");
		reg=FMSH_IN32_32(pGmacPortMap->GMAC_ISR);
		if((reg&GMAC_ISR_RGSMIIIS) != 0)
		{
			TRACE_OUT(DEBUG_OUT_EHT, "RGMII or SMII Interrupt, Link status change \r\n");
			FGmac_Ps_GetLinkStatus(pGmac);  /* GLI will be cleared when read these bits */
		}
		break;
	default:
		TRACE_OUT(DEBUG_OUT_EHT, "> unexpected argument: 0x%x \r\n",ecode);
		break;
	}
}

void FGmacPs_GmacRxCallback(FGmacPs_Instance_T *pGmac,int32_t ecode)
{
    TRACE_OUT(DEBUG_OUT_EHT, "rx int\r\n");
    g_rcv_cnt++;
    TRACE_OUT(DEBUG_OUT_EHT, "rx cnt:%d\r\n",g_rcv_cnt);
    /*    /* close irq */
    /*    FGmac_Ps_SetupIntr(pGmac, gdma_irq_none);*/
    /*    ethernetif_input(&Gmac_LwIP_if);*/
    /*     poll */
    /*    while(ethernetif_input(&Gmac_LwIP_if)!= ERR_OK);*/
    /*    /* open irq */
    /*    FGmac_Ps_SetupIntr(pGmac, gdma_irq_all);*/

}

void FGmacPs_GmacTxCallback(FGmacPs_Instance_T *pGmac,int32_t ecode)
{
    TRACE_OUT(DEBUG_OUT_EHT, "tx int\r\n");
    tx_done++;
}
#endif
/**
* In this function, the hardware should be initialized.
* Called from ethernetif_init().
*
* @param netif the already initialized lwip network interface structure
*        for this ethernetif
*/
static void low_level_init(struct netif *netif)
{
    u8 GMacAddress[6] = {ENET_MAC_ADDRESS};
	/* set MAC hardware address length */
	netif->hwaddr_len = ETHARP_HWADDR_LEN;

	/* set MAC hardware address */
	netif->hwaddr[0] = GMacAddress[0];
	netif->hwaddr[1] = GMacAddress[1];
	netif->hwaddr[2] = GMacAddress[2];
	netif->hwaddr[3] = GMacAddress[3];
	netif->hwaddr[4] = GMacAddress[4];
	netif->hwaddr[5] = GMacAddress[5];

	/* maximum transfer unit */
	netif->mtu = 1500;

	/* device capabilities */
	/* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

#if LWIP_IPV6 && LWIP_IPV6_MLD
	/*
	* For hardware/netifs that implement MAC filtering.
	* All-nodes link-local is handled by default, so we must let the hardware know
	* to allow multicast packets in.
	* Should set mld_mac_filter previously. */
	if (netif->mld_mac_filter != NULL) {
		ip6_addr_t ip6_allnodes_ll;
		ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
		netif->mld_mac_filter(netif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
	}
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

	/* Do whatever else is needed to initialize interface. */
#if (0==RCV_POLL)
    g_rcv_cnt=0;
    /* gic */
    u32 Status =  FGicPs_SelfTest(&IntcInstance);
	if(Status!=GIC_SUCCESS)
		TRACE_OUT(FMQL_LWIP_DEBUG, " GIC Setup Failed!\r\n");
	else
		TRACE_OUT(FMQL_LWIP_DEBUG, " GIC Setup pass!\r\n");
#endif
    /*  phy hw reset  */
    FGpioPs_T g_GPIOB;
    FGpioPs_Config *pGpioCfg;
    pGpioCfg = FGpioPs_LookupConfig(FPAR_GPIOPS_1_DEVICE_ID);
    FGpioPs_init(&g_GPIOB, pGpioCfg);

    FGpioPs_setDirection(&g_GPIOB, 0xffffffff);  /*1-out  0-in*/
    FGpioPs_writeData(&g_GPIOB, 0xffffffff);
    FGpioPs_writeData(&g_GPIOB, 0);
    delay_ms(200);
    FGpioPs_writeData(&g_GPIOB, 0xffffffff);

	/* ----------Initial Device -------------------------- */
    u8* GMAC0_PacketBuffer;
    u8* GMAC0_RxBuffer;
    u8* GMAC0_TxBuffer;

    /* malloc mem */
    GMAC0_PacketBuffer=(u8 *)malloc(GMAC_PACKET_BUFFER_SIZE);
    if(GMAC0_PacketBuffer==NULL){
        TRACE_OUT(FMQL_LWIP_DEBUG,"gmac lib malloc fail\r\n");
        return ;
    }

    GMAC0_RxBuffer=(u8 *)malloc(2*GMAC_RDES_NUM*GMAC_RBUFFER_UNIT_SIZE);
    if(GMAC0_RxBuffer==NULL){
        TRACE_OUT(FMQL_LWIP_DEBUG,"gmac lib malloc fail\r\n");
        return ;
    }

    GMAC0_TxBuffer=(u8*)malloc(2*GMAC_TDES_NUM*GMAC_TBUFFER_UNIT_SIZE);
    if(GMAC0_TxBuffer==NULL){
        TRACE_OUT(FMQL_LWIP_DEBUG,"gmac lib malloc fail\r\n");
        return ;
    }

    FGmacPs_Config_T *pGmacCfg;
    pGmacCfg = FGmacPs_LookupConfig(LWIP_GMAC_ID);
    FGmac_Ps_StructInit(s_GMAC_inst_lwip_p,
                        &s_GMAC_LinkStatus,
                        pGmacCfg,
                        &s_GMAC_PhyCfg,
                        GMAC_RDES_NUM, GMAC_TDES_NUM,
                        GMAC_RBUFFER_UNIT_SIZE, GMAC_TBUFFER_UNIT_SIZE,
                        GMAC0_PacketBuffer, GMAC_PACKET_BUFFER_SIZE);
    FGmac_Ps_DeviceReset(s_GMAC_inst_lwip_p);
    FGmac_Ps_DmaInit(s_GMAC_inst_lwip_p,
                     GMAC0_RxDs,
                     GMAC0_RxBuffer,
                     GMAC0_TxDs,
                     GMAC0_TxBuffer);
    FGmac_Ps_SetupIntr(s_GMAC_inst_lwip_p, gdma_irq_none);
#if (0==RCV_POLL)
    FGmac_Ps_SetupIntr(s_GMAC_inst_lwip_p, gdma_irq_tx|gdma_irq_rx|gdma_irq_nie);

    FGmac_Ps_SetListener(s_GMAC_inst_lwip_p,(FMSH_callback)FGmacPs_GmacListener);
	FGmac_Ps_SetTxCallback(s_GMAC_inst_lwip_p,(FMSH_callback)FGmacPs_GmacTxCallback);
	FGmac_Ps_SetRxCallback(s_GMAC_inst_lwip_p,(FMSH_callback)FGmacPs_GmacRxCallback);

    FGicPs_SetupInterruptSystem(&IntcInstance);
    FGicPs_Connect(&IntcInstance,GMAC0_INT_ID,(FMSH_InterruptHandler)gmac_interrupt_handler,0);
    FMSH_ExceptionRegisterHandler(FMSH_EXCEPTION_ID_FIQ_INT,(FMSH_ExceptionHandler)FGicPs_InterruptHandler_FIQ,&IntcInstance);
    FGicPs_Enable(&IntcInstance,GMAC0_INT_ID);
#endif
    FGmac_Ps_EnTxOsf(s_GMAC_inst_lwip_p, 1);
	FGmac_Ps_MacInit(s_GMAC_inst_lwip_p);

    /* 9031 */
    /*
    u16 s_tx_clk_skew=0x1f;
    u16 s_rx_clk_skew=0x1f;
    u16 s_tx_data_skew=0x0;
    u16 s_rx_data_skew=0x0;

    mic9031RNX_clk_pad_skew(s_GMAC_inst_lwip_p,s_tx_clk_skew,s_rx_clk_skew);
    mic9031RNX_ctrl_pad_skew(s_GMAC_inst_lwip_p,s_rx_data_skew,s_tx_data_skew);
    mic9031RNX_tx_data_pad_skew(s_GMAC_inst_lwip_p,s_tx_data_skew,s_tx_data_skew,s_tx_data_skew,s_tx_data_skew); //only 9031 RNX
    mic9031RNX_rx_data_pad_skew(s_GMAC_inst_lwip_p,s_rx_data_skew,s_rx_data_skew,s_rx_data_skew,s_rx_data_skew); //only 9031 RNX
    */
}

/**
* This function should do the actual transmission of the packet. The packet is
* contained in the pbuf that is passed to the function. This pbuf
* might be chained.
*
* @param netif the lwip network interface structure for this ethernetif
* @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
* @return ERR_OK if the packet could be sent
*         an err_t value if the packet couldn't be sent
*
* @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
*       strange results. You might consider waiting for space in the DMA queue
*       to become available since the stack doesn't retry to send a packet
*       dropped because of memory failure (except for the TCP timers).
*/
err_t low_level_output(struct netif *netif, struct pbuf *p)
{
#if PSOC_CACHE_ENABLE
    u32 range_start, range_end;
#endif
#if ETH_PAD_SIZE
	pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif
    FGmacPs_TxDescriptor_T *pTxDES_tmp;   /* tmp pDES */
    u16 TxDES_Idx;				/* tmp Idx of TxDES */
    u32 TDes_num=s_GMAC_inst_lwip_p->wTxListSize;
    u32 TxDesBufSize=s_GMAC_inst_lwip_p->TxDesBufSize;
    /* initial */
    TxDES_Idx=s_GMAC_inst_lwip_p->wTxHead;
    pTxDES_tmp = &s_GMAC_inst_lwip_p->pTxD[0];
    u32 size = p->tot_len;

    pTxDES_tmp[TxDES_Idx].TDES0.val=0;
    pTxDES_tmp[TxDES_Idx].TDES1.val=0;
    pbuf_copy_partial(p, (u32*)(pTxDES_tmp[TxDES_Idx].BufferAdd1), size, 0);

    if(TxDES_Idx>=(TDes_num-1)) {
        pTxDES_tmp[TxDES_Idx].TDES1.val |= GMAC_TDES1_TER;
    }

    /* setup buffer size */
    if(size<=TxDesBufSize)
    {
        pTxDES_tmp[TxDES_Idx].TDES1.val |= size;
    }else{
        pTxDES_tmp[TxDES_Idx].TDES1.val |= TxDesBufSize;
        pTxDES_tmp[TxDES_Idx].TDES1.val |= (size-TxDesBufSize)<<11;
    }
#if PSOC_CACHE_ENABLE
    range_start = pTxDES_tmp[TxDES_Idx].BufferAdd1 & 0xffffffc0;
    range_end = ((pTxDES_tmp[TxDES_Idx].BufferAdd1 + TxDesBufSize*2) & 0xffffffc0) + (1<<6);
	
#if 0  /* bare-code*/
	 flush_dcache_range(range_start, range_end);
#else  /* vx-code*/
	/**/
	/* flush cache, it's must be for ddr flush new data*/
	/**/
	(void)cacheInvalidate (DATA_CACHE, FPS_AHB_SRAM_BASEADDR, 0x20000);   /* 118K ddr flush*/
#endif
#endif

    pTxDES_tmp[TxDES_Idx].TDES1.val |= GMAC_TDES1_LS|GMAC_TDES1_IC;
    pTxDES_tmp[TxDES_Idx].TDES1.val |= GMAC_TDES1_FS;
    pTxDES_tmp[TxDES_Idx].TDES0.val |= GMAC_TDES0_OWN;

    TxDES_Idx++;
    if(TxDES_Idx>=TDes_num)
        TxDES_Idx=0;
    /* update head */
    s_GMAC_inst_lwip_p->wTxHead=TxDES_Idx;

    /* start to transmit */
    gmac_DmaTxPollDemand(s_GMAC_inst_lwip_p);
    tx_done--;

#if ETH_PAD_SIZE
	pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif
	LINK_STATS_INC(link.xmit);
	return ERR_OK;
}

/**
* Should allocate a pbuf and transfer the bytes of the incoming
* packet from the interface into the pbuf.
*
* @param netif the lwip network interface structure for this ethernetif
* @return a pbuf filled with the received packet (including MAC header)
*         NULL on memory error
*/
struct pbuf * low_level_input(struct netif *netif)
{
	struct pbuf *p, *q;
	u32_t len;
    u8 * rcv_packet=NULL;
	/* Obtain the size of the packet and put it into the "len"
	variable. */
/*    u8 status;*/
/*	status=FGmac_Ps_RcvPoll(s_GMAC_inst_lwip_p,&len);*/
/*    if(status==GMAC_RETURN_CODE_RX_NULL)*/
/*        return NULL;*/
/*    rcv_packet=s_GMAC_inst_lwip_p->pFrmBuffer;*/

    rcv_packet=(u8 *)FGmac_Ps_RcvPollEFrame(s_GMAC_inst_lwip_p,&len);
	if(rcv_packet==NULL)
		return NULL;

#if ETH_PAD_SIZE
	len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif
	/* We allocate a pbuf chain of pbufs from the pool. */
	p = pbuf_alloc(PBUF_RAW, len, PBUF_RAM);

	if (p != NULL) {

#if ETH_PAD_SIZE
		pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

		/* We iterate over the pbuf chain until we have read the entire
		* packet into the pbuf. */
		for(q = p; q != NULL; q = q->next) {
			/* Read enough bytes to fill this pbuf in the chain. The
			* available data in the pbuf is given by the q->len
			* variable.
			* This does not necessarily have to be a memcpy, you can also preallocate
			* pbufs for a DMA-enabled MAC and after receiving truncate it to the
			* actually received size. In this case, ensure the tot_len member of the
			* pbuf is the sum of the chained pbuf len members.
			*/
			/*read data into(q->payload, q->len);*/
			memcpy(q->payload,rcv_packet,q->len);
			rcv_packet+=q->len;
		}
#if ETH_PAD_SIZE
		pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

		LINK_STATS_INC(link.recv);
	} else {
		/*drop packet();*/
		/*LINK_STATS_INC(link.memerr);*/
		/*LINK_STATS_INC(link.drop);*/
	}

	return p;
}

/**
* This function should be called when a packet is ready to be read
* from the interface. It uses the function low_level_input() that
* should handle the actual reception of bytes from the network
* interface. Then the type of the received packet is determined and
* the appropriate input function is called.
*
* @param netif the lwip network interface structure for this ethernetif
*/
err_t ethernetif_input_poll(struct netif *netif)
{
    err_t err;
    struct pbuf *p=NULL;

    /* move received packet into a new pbuf */
    p = low_level_input(netif);

    /* no packet could be read, silently ignore this */
    if (p == NULL)
        return ERR_MEM;
    /* entry point to the LwIP stack */
    err = netif->input(p, netif);
    if (err != ERR_OK)
    {
        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
        pbuf_free(p);
    }

    return err;
}

err_t ethernetif_input(struct netif *netif)
{
    err_t err;
    struct pbuf *p=NULL;
    /* close irq */
    FGicPs_Disable(&IntcInstance,GMAC0_INT_ID);
    u32 i;
    for(i=0;i<GMAC_POLL_MAX;i++){
        p = low_level_input(netif);
        if(p == NULL)
            break;
        err = netif->input(p, netif);
        if (err != ERR_OK) {
            LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
            pbuf_free(p);
        }
    }
    /* open irq */
    g_rcv_cnt--;
    FGicPs_Enable(&IntcInstance,GMAC0_INT_ID);
    return ERR_OK;
}


/**
* Should be called at the beginning of the program to set up the
* network interface. It calls the function low_level_init() to do the
* actual setup of the hardware.
*
* This function should be passed as a parameter to netif_add().
*
* @param netif the lwip network interface structure for this ethernetif
* @return ERR_OK if the loopif is initialized
*         ERR_MEM if private data couldn't be allocated
*         any other err_t on error
*/
err_t ethernetif_init(struct netif *netif)
{
	struct ethernetif *ethernetif;

	LWIP_ASSERT("netif != NULL", (netif != NULL));

	ethernetif = mem_malloc(sizeof(struct ethernetif));
	if (ethernetif == NULL) {
		LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_init: out of memory\n"));
		return ERR_MEM;
	}

#if LWIP_NETIF_HOSTNAME
	/* Initialize interface hostname */
	netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

	/*
	* Initialize the snmp variables and counters inside the struct netif.
	* The last argument should be replaced with your link speed, in units
	* of bits per second.
	*/
	netif->state = ethernetif;
	netif->name[0] = IFNAME0;
	netif->name[1] = IFNAME1;
	/* We directly use etharp_output() here to save a function call.
	* You can instead declare your own function an call etharp_output()
	* from it if you have to do some checks before sending (e.g. if link
	* is available...) */
	netif->output = etharp_output;
#if LWIP_IPV6
	netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
	netif->linkoutput = low_level_output;

	ethernetif->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);

	/* initialize the hardware */
	low_level_init(netif);

#if 0 /*debug no copy*/
    pbuf_p=&pbuf_s;
    pbuf_p->next=NULL;
    pbuf_p->flags=0;
    pbuf_p->ref=1;
    pbuf_p->type=PBUF_POOL;
#endif
	return ERR_OK;
}
