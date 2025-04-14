/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file  fmsh_gmac_lib.c
*
* gmac driver
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
#include <string.h>
#include <stdlib.h>

#include "../../../common/fmsh_ps_parameters.h"

#include "fmsh_gmac_lib.h"
#include "fmsh_gmac_hw.h"
#include "./phy/fmsh_mdio.h"

/*#include "cache.h"*/

/************************** Constant Definitions *****************************/
#define GMAC_LIB_DEBUG            0
/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/
/* Increment head or tail */
#define GMAC_GCIRC_INC(headortail,size) do{\
headortail++;             \
    if(headortail >= size) {  \
        headortail = 0;       \
    }						\
        }while(0)


/************************** Function Prototypes ******************************/

/************************** Variable Definitions *****************************/

/************************** Local Function ***********************************/

/************************** Extern Function **********************************/

/****************************************************************************/
/**
*
* Initialize Rx Descriptor
*
* @param  FGmacPs_Instance_T * pGmac
*
* @return   GMAC_RETURN_CODE_OK
*
* @note    None.
*
****************************************************************************/
u8 FGmac_Ps_InitRxDes(FGmacPs_Instance_T * pGmac)
{
    FGmacPs_DmaPortMap_T  * pDmaPortMap;
    FGmacPs_RxDescriptor_T * pRxD;
	
    u8 *pRxBuffer;
    u16 RxListSize;
    u32 RxDesBufSize=pGmac->RxDesBufSize;

	int i = 0;

    pDmaPortMap = (FGmacPs_DmaPortMap_T*)((u32)pGmac->base_address + GMAC_DMA_OFFSET);
    pRxBuffer = pGmac->pRxBuffer;
    pRxD      = pGmac->pRxD;
    RxListSize    = pGmac->wRxListSize;
	
    /* Disable Rx */
    FGmac_Ps_SetRxState(pGmac,GMAC_STATE_STOP);
	
    /* Initialize Rx Descriptor List */
    pGmac->wRxI = 0;
	
    for (i=0; i < RxListSize; i++)
    {
        pRxD[i].RDES0.val = (u32)(GMAC_RDES0_OWN);
        pRxD[i].RDES1.val = (u32)((RxDesBufSize)|((RxDesBufSize) << 11));	/* indicate buffer size in bytes */
        pRxD[i].BufferAdd1 = (u32)(&(pRxBuffer[2*i*(RxDesBufSize)]));
        pRxD[i].BufferAdd2 = (u32)(&(pRxBuffer[(2*i+1)*(RxDesBufSize)]));
    }
    pRxD[RxListSize-1].RDES1.val |= GMAC_RDES1_RER;				/* indicate the final descriptor */
	
    /* DMA Reg3 Points to the start of the Rx Descriptor List  */
    FMSH_OUT32_32((u32)pRxD,pDmaPortMap->GDMA_RDLA);

    /* Enable DMA Rx */
    FGmac_Ps_SetRxState(pGmac,GMAC_STATE_RUN);
	
    return GMAC_RETURN_CODE_OK;
}

/****************************************************************************/
/**
*
* Reset current Rx Descriptor
*
* @param    FGmacPs_RxDescriptor_T *pRxD
* @param    FGmacPs_Instance_T * pGmac
*
* @return   void
*
* @note    None.
*
****************************************************************************/
void FGmac_Ps_ResetCurRxDES(FGmacPs_Instance_T * pGmac,FGmacPs_RxDescriptor_T *pRxD)
{
    pRxD->RDES0.val = (u32)(GMAC_RDES0_OWN);
    if(pRxD->RDES1.val&GMAC_RDES1_RER!=0) {
        pRxD->RDES1.val = (u32)GMAC_RDES1_RER;
    }
    pRxD->RDES1.val |= (u32)((pGmac->RxDesBufSize)|((pGmac->RxDesBufSize) << 11));
}

/****************************************************************************/
/**
*
* Initialize Tx Descriptor
*
* @param	FGmacPs_Instance_T * pGmac
*
* @return   GMAC_RETURN_CODE_OK
*
* @note    None.
*
****************************************************************************/
u8 FGmac_Ps_InitTxDes(FGmacPs_Instance_T * pGmac)
{
    u8 *pTxBuffer;
    FGmacPs_TxDescriptor_T *pTxD;
    u16 TxSize;
    u32 Index;
    FGmacPs_DmaPortMap_T  *pDmaPortMap;
    pDmaPortMap      = (FGmacPs_DmaPortMap_T*)((u32)pGmac->base_address + GMAC_DMA_OFFSET);

    pTxBuffer = pGmac->pTxBuffer;
    pTxD      = pGmac->pTxD;
    TxSize    = pGmac->wTxListSize;
    u32 TxDesBufSize=pGmac->TxDesBufSize;
	
    /* Disable Tx */
    FGmac_Ps_SetTxState(pGmac,GMAC_STATE_STOP);
	
    /* Initialize Tx Descriptor List */
    pGmac->wRxI = 0;
    for(Index = 0;Index < TxSize;Index++)
    {
        pTxD[Index].TDES0.val = 0; /*(u32)(GMAC_TDES0_OWN);*/
        pTxD[Index].TDES1.val = (u32)((TxDesBufSize)|((TxDesBufSize) << 11));
        pTxD[Index].BufferAdd1 = (u32)(&(pTxBuffer[2*Index*(TxDesBufSize)]));
        pTxD[Index].BufferAdd2 = (u32)(&(pTxBuffer[(2*Index+1)*(TxDesBufSize)]));
    }
    pTxD[TxSize-1].TDES1.val |= GMAC_TDES1_TER;				/* indicate the final descriptor */

    /* DMA Reg4 Points to the start of the Tx Descriptor List  */
    FMSH_OUT32_32((u32)pTxD,pDmaPortMap->GDMA_TDLA);

    /* Enable Tx */
    FGmac_Ps_SetTxState(pGmac,GMAC_STATE_RUN);
    return GMAC_RETURN_CODE_OK;
}

/****************************************************************************/
/**
*
* initialize dma of gmac
*
* @param    FGmacPs_Instance_T *pGmac
* @param    FGmacPs_RxDescriptor_T * pRxDs,
* @param    u8* pRxBuffer,
* @param    FGmacPs_TxDescriptor_T * pTxDs,
* @param    u8* pTxBuffer
*
* @return   GMAC_RETURN_CODE_OK or GMAC_RETURN_CODE_TIME_OUT
*
* @note     ref DataSheet
*
****************************************************************************/
u8 FGmac_Ps_DmaInit(FGmacPs_Instance_T *pGmac,
                    FGmacPs_RxDescriptor_T * pRxDs,
                    u8* pRxBuffer,
                    FGmacPs_TxDescriptor_T * pTxDs,
                    u8* pTxBuffer)
{
    FGmacPs_DmaPortMap_T *pDmaPortMap;
    FGmacPs_MacPortMap_T *pGmacPortMap;
    u32 reg_val_32b;
    u32 time_out_cnt=0;
    pDmaPortMap = (FGmacPs_DmaPortMap_T*)((u32)pGmac->base_address + GMAC_DMA_OFFSET);
    pGmacPortMap = pGmac->base_address;

    /*----------------------------
    * step 1 : software reset, useless
    ------------------------------*/
    /*FMSH_OUT32_32(GDMA_BMR_SWR,pDmaPortMap->GDMA_BMR);*/

    /* ----------------------------
    * step 2 : wait for completing reset , useless
    ------------------------------*/
    /*do{reg_val_32b = FMSH_IN32_32(pDmaPortMap->GDMA_BMR);}*/
    /*while((reg_val_32b&GDMA_BMR_SWR) != 0);*/

    /* ----------------------------
    * step 3 :
    * confirm that all previously initiated
    * or ongoing AHB or AXI transactions are complete.
    ------------------------------*/
    time_out_cnt=0;
    do
    {
        delay_1us();
        time_out_cnt++;
        if(time_out_cnt>GMAC_TIME_OUT_VALUE_1)
        {
            return GMAC_RETURN_CODE_TIME_OUT;
        }
        reg_val_32b = FMSH_IN32_32(pDmaPortMap->GDMA_ASR);
    }while((reg_val_32b&(GDMA_AXWHSTS|GDMA_AXIRDSTS)) != 0);

    /*----------------------------
    * step 4 : Config DMA Bus Mode
    ------------------------------*/
    reg_val_32b=FMSH_IN32_32(pDmaPortMap->GDMA_BMR);
#if (GMAC_BUS_MODE_AALB==1)
    SET_BIT(reg_val_32b,GDMA_BMR_AAL);			/* Address-align */
#endif
#if (GMAC_BUS_MODE_FIXED_BURST==1)
    SET_BIT(reg_val_32b,GDMA_BMR_FB);           /* Fix Burst */
#endif
    FMSH_OUT32_32(reg_val_32b,pDmaPortMap->GDMA_BMR);

    /*----------------------------
    * step 5 : Program the AXI Interface options Reg10
    * If fixed burst length is enabled,
    * then select the maximum burst length possible on the AXI bus
    ------------------------------*/
#if (GMAC_BUS_MODE_FIXED_BURST==1)
    reg_val_32b=FMSH_IN32_32(pDmaPortMap->GDMA_ABM);
    SET_BIT(reg_val_32b,0xFE);  /* clear BLEN */
    SET_BIT(reg_val_32b,GMAC_AXI_BURST_LEHGTH);
    FMSH_OUT32_32(reg_val_32b,pDmaPortMap->GDMA_ABM);
#endif

    /*----------------------------
    * step 6.7.8 : Init Rx & Tx DES
    ------------------------------*/
    pGmac->pRxBuffer = pRxBuffer;
    pGmac->pRxD = pRxDs;
    pGmac->pTxBuffer = pTxBuffer;
    pGmac->pTxD = pTxDs;

    FGmac_Ps_InitRxDes(pGmac);
    FGmac_Ps_InitTxDes(pGmac);

    /*----------------------------
    * step 9 : Config DMA Operation mode Reg6
    ------------------------------*/
    reg_val_32b=FMSH_IN32_32(pDmaPortMap->GDMA_OMR);
    SET_BIT(reg_val_32b,GDMA_OMR_RSF);			/* Receive Store and Forward? */
    SET_BIT(reg_val_32b,GDMA_OMR_TSF);			/* Transmit Store and Forward? */
	
#if (GMAC_OP_MODE_FW_ERR_FRAME==1)
    SET_BIT(reg_val_32b,GDMA_OMR_FEF);			/* Forward Error Frame  */
#endif

#if (GMAC_OP_MODE_FW_UDSZ_GOOD_FRAME==1)
    SET_BIT(reg_val_32b,GDMA_OMR_FUF);			/* forward undersized good frames */
#endif

    /*SET_BIT(reg_val_32b,GDMA_OMR_DFF);			/* disable flush Rx frame*/
#if (GMAC_OP_MODE_HW_FLOW_CTRL==1)
    SET_BIT(reg_val_32b,GDMA_OMR_EFC);
    SET_BIT(reg_val_32b,GDMA_OMR_RFA_2K);
    SET_BIT(reg_val_32b,GDMA_OMR_RFD_3K);
#endif

    FMSH_OUT32_32(reg_val_32b,pDmaPortMap->GDMA_OMR);

    /*----------------------------
    * step 10 : Clear DMA Interrupt requests
    * 10.1 : clear all MAC interrupt status(MAC reg14) by reading corresponding regs
    * 10.2 : write 1'b1 to DAM reg5 bit[16:0] to clear DMA status
    ------------------------------*/
    reg_val_32b= FMSH_IN32_32 (pGmacPortMap->GMAC_GPIO);
    reg_val_32b= FMSH_IN32_32 (pGmacPortMap->GMAC_LCS);
    /* read MAC reg458 if timestamp enable */
    reg_val_32b= FMSH_IN32_32 (pGmacPortMap->GMAC_AN_STS);
    reg_val_32b= FMSH_IN32_32 (pGmacPortMap->GMAC_MII_STS);
    FMSH_OUT32_32(gdma_irq_all,pDmaPortMap->GDMA_SR);

    /*----------------------------
    * step 11 : Enable DMA interrupt
    ------------------------------*/
    FGmac_Ps_SetupIntr(pGmac, gdma_irq_all);

    /*----------------------------
    * step 12 : Start Rx & Tx DMA
    ------------------------------*/
    reg_val_32b = FMSH_IN32_32(pDmaPortMap->GDMA_OMR);
    SET_BIT(reg_val_32b,GDMA_OMR_SR|GDMA_OMR_ST);
    FMSH_OUT32_32(reg_val_32b,pDmaPortMap->GDMA_OMR);

    return GMAC_RETURN_CODE_OK;
}

/****************************************************************************/
/**
*
* initialize dma of gmac, for fast loop.
* Tx Rx des point to the same buffer
*
* @param    FGmacPs_Instance_T *pGmac
* @param    FGmacPs_RxDescriptor_T * pRxDs,
* @param    u8* pRxBuffer,
* @param    FGmacPs_TxDescriptor_T * pTxDs,
* @param    u8* pTxBuffer
*
* @return   GMAC_RETURN_CODE_OK or GMAC_RETURN_CODE_TIME_OUT
*
* @note     ref DataSheet
*
****************************************************************************/
u8 FGmac_Ps_DmaInit_for_loop_pro(FGmacPs_Instance_T *pGmac,
                                 FGmacPs_RxDescriptor_T * pRxDs,
                                 u8* pRxBuffer,
                                 FGmacPs_TxDescriptor_T * pTxDs,
                                 u8* pTxBuffer)
{
    FGmacPs_DmaPortMap_T *pDmaPortMap;
    FGmacPs_MacPortMap_T *pGmacPortMap;
    u32 reg_val_32b;
    u32 time_out_cnt=0;
    pDmaPortMap = (FGmacPs_DmaPortMap_T*)((u32)pGmac->base_address + GMAC_DMA_OFFSET);
    pGmacPortMap = pGmac->base_address;

    /*----------------------------
    * step 1 : software reset, useless
    ------------------------------*/
    /*FMSH_OUT32_32(GDMA_BMR_SWR,pDmaPortMap->GDMA_BMR);*/

    /* ----------------------------
    * step 2 : wait for completing reset , useless
    ------------------------------*/
    /*do{reg_val_32b = FMSH_IN32_32(pDmaPortMap->GDMA_BMR);}*/
    /*while((reg_val_32b&GDMA_BMR_SWR) != 0);*/

    /* ----------------------------
    * step 3 :
    * confirm that all previously initiated
    * or ongoing AHB or AXI transactions are complete.
    ------------------------------*/
    time_out_cnt=0;
    do
    {
        delay_1us();
        time_out_cnt++;
        if(time_out_cnt>GMAC_TIME_OUT_VALUE_1)
        {
            return GMAC_RETURN_CODE_TIME_OUT;
        }
        reg_val_32b = FMSH_IN32_32(pDmaPortMap->GDMA_ASR);
    }while((reg_val_32b&(GDMA_AXWHSTS|GDMA_AXIRDSTS)) != 0);

    /*----------------------------
    * step 4 : Config DMA Bus Mode
    ------------------------------*/
    reg_val_32b=FMSH_IN32_32(pDmaPortMap->GDMA_BMR);
#if (GMAC_BUS_MODE_AALB==1)
    SET_BIT(reg_val_32b,GDMA_BMR_AAL);			/* Address-align */
#endif
#if (GMAC_BUS_MODE_FIXED_BURST==1)
    SET_BIT(reg_val_32b,GDMA_BMR_FB);           /* Fix Burst */
#endif
    FMSH_OUT32_32(reg_val_32b,pDmaPortMap->GDMA_BMR);

    /*----------------------------
    * step 5 : Program the AXI Interface options Reg10
    * If fixed burst length is enabled,
    * then select the maximum burst length possible on the AXI bus
    ------------------------------*/
#if (GMAC_BUS_MODE_FIXED_BURST==1)
    reg_val_32b=FMSH_IN32_32(pDmaPortMap->GDMA_ABM);
    SET_BIT(reg_val_32b,0xFE);  /* clear BLEN */
    SET_BIT(reg_val_32b,GMAC_AXI_BURST_LEHGTH);
    FMSH_OUT32_32(reg_val_32b,pDmaPortMap->GDMA_ABM);
#endif

    /*----------------------------
    * step 6.7.8 : Init Rx & Tx DES
    ------------------------------*/
    pGmac->pRxBuffer = pRxBuffer;
    pGmac->pRxD = pRxDs;
    pGmac->pTxBuffer = pTxBuffer;
    pGmac->pTxD = pTxDs;

    FGmac_Ps_InitRxDes(pGmac);
    FGmac_Ps_InitTxDes(pGmac);

    /*----------------------------
    * step 9 : Config DMA Operation mode Reg6
    ------------------------------*/
    reg_val_32b=FMSH_IN32_32(pDmaPortMap->GDMA_OMR);
    SET_BIT(reg_val_32b,GDMA_OMR_RSF);			/* Receive Store and Forward? */
    SET_BIT(reg_val_32b,GDMA_OMR_TSF);			/* Transmit Store and Forward? */
#if (GMAC_OP_MODE_FW_ERR_FRAME==1)
    SET_BIT(reg_val_32b,GDMA_OMR_FEF);			/* Forward Error Frame  */
#endif
#if (GMAC_OP_MODE_FW_UDSZ_GOOD_FRAME==1)
    SET_BIT(reg_val_32b,GDMA_OMR_FUF);			/* forward undersized good frames */
#endif
    /*SET_BIT(reg_val_32b,GDMA_OMR_DFF);			/* disable flush Rx frame*/
#if (GMAC_OP_MODE_HW_FLOW_CTRL==1)
    SET_BIT(reg_val_32b,GDMA_OMR_EFC);
    SET_BIT(reg_val_32b,GDMA_OMR_RFA_2K);
    SET_BIT(reg_val_32b,GDMA_OMR_RFD_3K);
#endif
    FMSH_OUT32_32(reg_val_32b,pDmaPortMap->GDMA_OMR);

    /*----------------------------
    * step 10 : Clear DMA Interrupt requests
    * 10.1 : clear all MAC interrupt status(MAC reg14) by reading corresponding regs
    * 10.2 : write 1'b1 to DAM reg5 bit[16:0] to clear DMA status
    ------------------------------*/
    reg_val_32b= FMSH_IN32_32 (pGmacPortMap->GMAC_GPIO);
    reg_val_32b= FMSH_IN32_32 (pGmacPortMap->GMAC_LCS);
    /* read MAC reg458 if timestamp enable */
    reg_val_32b= FMSH_IN32_32 (pGmacPortMap->GMAC_AN_STS);
    reg_val_32b= FMSH_IN32_32 (pGmacPortMap->GMAC_MII_STS);
    FMSH_OUT32_32(gdma_irq_all,pDmaPortMap->GDMA_SR);

    /*----------------------------
    * step 11 : Enable DMA interrupt
    ------------------------------*/
    FGmac_Ps_SetupIntr(pGmac, gdma_irq_all);

    /*----------------------------
    * step 12 : Start Rx & Tx DMA
    ------------------------------*/
    reg_val_32b = FMSH_IN32_32(pDmaPortMap->GDMA_OMR);
    SET_BIT(reg_val_32b,GDMA_OMR_SR|GDMA_OMR_ST);
    FMSH_OUT32_32(reg_val_32b,pDmaPortMap->GDMA_OMR);

    return GMAC_RETURN_CODE_OK;
}

/****************************************************************************/
/**
*
* Initialize Gmac
*
* @param    FGmacPs_Instance_T * pGmac
* @param    u8 * pMacAddr
*
* @return   GMAC_RETURN_CODE_OK, GMAC_RETURN_CODE_ERR, GMAC_RETURN_CODE_TIME_OUT
*
* @note     ref DataSheet
*
****************************************************************************/
u8 FGmac_Ps_MacInit(FGmacPs_Instance_T * pGmac)
{
    FGmacPs_MacPortMap_T *pGmac_PortMap;
    pGmac_PortMap = pGmac->base_address;
    FGmacPs_PhyConfig_T *pPhyConfig = pGmac->phy_cfg;
    FGmacPs_Config_T *pGmacCfg = pGmac->gmac_cfg;

    u32 reg_val_32b;
	
    /*----------------------------
    * step 1.2 : (only for TBI,SGMII & RTBI PHY interface)
    ------------------------------*/

    /*----------------------------
    * step 3 : program to control PHY
    ------------------------------*/
    pPhyConfig->phy_op_init(pGmac);
    pPhyConfig->phy_op_cfg(pGmac);
    pPhyConfig->phy_op_reset(pGmac);
	
    /*----------------------------
    * step 5 : set MAC address
    ------------------------------*/
    FGmac_Ps_SetupMacAddr(pGmac,0,pGmac->mac_address,1,0,0);    	/* set Mac address0 */

    /*----------------------------
    * step 6 : hash address(not use)
    ------------------------------*/

    /*----------------------------
    * step 7 : address filter
    ------------------------------*/
    reg_val_32b=FMSH_IN32_32(pGmac_PortMap->GMAC_MFF);
	
#if (GMAC_RCV_ALL==1)
    SET_BIT(reg_val_32b,GMAC_MFF_RA);		/* receive all */
#endif

#if (GMAC_RCV_ALL_MULTICAST==1)
    SET_BIT(reg_val_32b,GMAC_MFF_PM);      /* pass all multi */
#endif

#if (GMAC_DIS_BROADCAST==1)
    SET_BIT(reg_val_32b,GMAC_MFF_DBF);		/* disable broadcast frame */
#endif

#if (GMAC_SRC_ADDR_FILT==1)
    SET_BIT(reg_val_32b,GMAC_MFF_SAF);
#else
    RESET_BIT(reg_val_32b,GMAC_MFF_SAF);
#endif

    FMSH_OUT32_32(reg_val_32b,pGmac_PortMap->GMAC_MFF);

    /*----------------------------
    * step 8 : Configure FLow Control
    ------------------------------*/
    FGmac_Ps_FlwCtrlSetup(pGmac,0,0,0,0,1,0,0);

    /*----------------------------
    * step 9 : Interrupt Mask bit
    ------------------------------*/

    /*----------------------------
    * step 9.5 : get status
    ------------------------------*/
    FGmac_Ps_GetHwFeature(pGmac);
    FGmac_Ps_GetLinkStatus(pGmac);

    /*----------------------------
    * step 10 : MAC Configuration
    ------------------------------*/
    /*FMSH_gmacMDIO_get_phy_status(pGmac);*/
    /*FGmac_Ps_SetupSpeed(pGmac,pGmac->phy_status->speed);*/
    if (pGmacCfg->Speed==speed_1000)
        FGmac_Ps_SetupSpeed(pGmac,2); /*1:100M, 2:1000M*/
    else if (pGmacCfg->Speed==speed_100)
        FGmac_Ps_SetupSpeed(pGmac,1);
    else if (pGmacCfg->Speed==speed_10)
        FGmac_Ps_SetupSpeed(pGmac,0);

#if GMAC_CFG_PAD_CRC_STRIPPING
    FGmac_Ps_PadCrcStrip(pGmac,1);       				/* enable pad CRC stripping*/
#endif

#if GMAC_CFG_CRC_STRIPPING_FOR_TYPE
    FGmac_Ps_TypeCrcStrip(pGmac,1);       				/* enable CRC stripping for type frame */
#endif

#if GMAC_CFG_LOOPBACK
    FGmac_Ps_LoopbackMode(pGmac,1);           				/* enable Loopback mode*/
#endif

#if GMAC_CFG_2K_FRAME_EN
    FGmac_Ps_Enable2k(pGmac,1);      				/* 2k frame enable */
#endif

#if GMAC_CFG_JUMBO_EN
    FGmac_Ps_EnableJumbo(pGmac,1);      				/* Jumbo enable*/
#endif

    /*----------------------------
    * step 11 : Other Configuration
    ------------------------------*/
    /*FGmac_Ps_SetupRxWatchDog(pGmac,GMAC_WATCHDOG_TIMEOUT,1);		/* Setting Rx frame watchdog timeout*/

    /*----------------------------
    * step 12 : Enable GMAC Tansmitter & Receiver
    * if GMAC_NORMAL_TEST, enable TxRx when test
    ------------------------------*/
    gmac_enable_rcv(pGmac,1);
    gmac_enable_tsv(pGmac,1);
    return GMAC_RETURN_CODE_OK;
}

/****************************************************************************/
/**
*
* Poll Rcv packet
*
* @param    FGmacPs_Instance_T *pGmac
* @param    u32 *pRcvSize (as a return value)
*
* @return   GMAC_RETURN_CODE_RX_NULL, GMAC_RETURN_CODE_SIZE_TOO_SMALL,
*            GMAC_RETURN_CODE_OK,
*
* @note
*
****************************************************************************/
u8 FGmac_Ps_RcvPoll(FGmacPs_Instance_T *pGmac,u32 *pRcvSize)
{
    FGmacPs_RxDescriptor_T *pRxTd;

    u16 CopyLength;
    u32 FrameSizeDone = 0;
    u32 tmpIdx;
    u8  isFrame = 0;

    u8 *pFrmBuffer;
    u32 FrmBufferSize;
#if PSOC_CACHE_ENABLE
	u32 range_start, range_end;
#endif

    /* variables init */
    tmpIdx    = pGmac->wRxI;
    pRxTd     = &pGmac->pRxD[tmpIdx];
    pFrmBuffer    = pGmac->pFrmBuffer;
    FrmBufferSize = pGmac->FrmBufferSize;
	
#if PSOC_CACHE_ENABLE
    range_start = (u32)(pRxTd->BufferAdd1) & 0xffffffc0; /* 64 byte aligned */
    range_end = (((u32)(pRxTd->BufferAdd2) + GMAC_RBUFFER_UNIT_SIZE) & 0xffffffc0) + (1<<6);

	#if 0  /* bare-code*/
    	 invalidate_dcache_range(range_start, range_end);  
    #else  /* vx-code*/
		/**/
		/* flush cache, it's must be for ddr flush new data*/
		/**/
		(void)cacheInvalidate (DATA_CACHE, FPS_AHB_SRAM_BASEADDR, 0x20000);   /* 118K ddr flush*/
	#endif
#endif

    u32 RxDesBufSize=pGmac->RxDesBufSize;
    *pRcvSize = 0;

    while ((pRxTd->RDES0.val & GMAC_RDES0_OWN) == 0) /* ownership = 0, ownered by HOST */
	{  	
        if ((pRxTd->RDES0.val & GMAC_RDES0_FS) == GMAC_RDES0_FS) /* First Descriptor */
		{ 
            /* Skip previous fragment */
            while(tmpIdx != pGmac->wRxI) 
			{
                pRxTd = &pGmac->pRxD[pGmac->wRxI];
                pRxTd->RDES0.val |= GMAC_RDES0_OWN;       /* ownered by DMA */
                GMAC_GCIRC_INC(pGmac->wRxI,pGmac->wRxListSize);
            }
			
            FrameSizeDone = 0;
            /* Start to gather buffers in a frame */
            isFrame = 1;
        }

        /* Increase tmpIdx */
        GMAC_GCIRC_INC(tmpIdx,pGmac->wRxListSize);

        /* Copy data in the frame buffer */
        if (isFrame) 
		{
            if (tmpIdx == pGmac->wRxI) 
			{
                do
				{
                    pRxTd = &pGmac->pRxD[pGmac->wRxI];
                    pRxTd->RDES0.val |= GMAC_RDES0_OWN;
					
                    GMAC_GCIRC_INC(pGmac->wRxI, pGmac->wRxListSize);
                } while (tmpIdx != pGmac->wRxI);
				
                return GMAC_RETURN_CODE_RX_NULL;
            }
			
            CopyLength = (RxDesBufSize);     /*Transfer data size each time*/
            
            if ((FrameSizeDone + CopyLength) >= FrmBufferSize) 
			{
                CopyLength = FrmBufferSize - FrameSizeDone;
                memcpy(pFrmBuffer,(void*)(pRxTd->BufferAdd1),CopyLength);  /*cp data: BufferAdd1 -> pFrmBuffer*/
                
                pFrmBuffer += CopyLength;     /*Increase pointer*/
                FrameSizeDone += CopyLength;  /*record tranfered data size*/
            } 
			else if ((FrameSizeDone + 2*CopyLength) >= FrmBufferSize)
			{
                memcpy(pFrmBuffer,(void*)(pRxTd->BufferAdd1),CopyLength);  /*cp data: BufferAdd1 -> pFrmBuffer*/
                
                pFrmBuffer += CopyLength;     /*Increase pointer*/
                FrameSizeDone += CopyLength;  /*record tranfered data size*/

                CopyLength = FrmBufferSize - FrameSizeDone;
                memcpy(pFrmBuffer,(void*)(pRxTd->BufferAdd2),CopyLength);  /*cp data: BufferAdd2 -> pFrmBuffer*/
                
                pFrmBuffer += CopyLength;     /*Increase pointer*/
                FrameSizeDone += CopyLength;  /*record tranfered data size*/
            } 
			else
			{
                memcpy(pFrmBuffer,(void*)(pRxTd->BufferAdd1),CopyLength);  /*cp data: BufferAdd1 -> pFrmBuffer*/
                
                pFrmBuffer += CopyLength;     /*Increase pointer*/
                FrameSizeDone += CopyLength;  /*record tranfered data size*/

                memcpy(pFrmBuffer,(void*)(pRxTd->BufferAdd2),CopyLength);  /*cp data: BufferAdd2 -> pFrmBuffer*/
                
                pFrmBuffer += CopyLength;     /*Increase pointer*/
                FrameSizeDone += CopyLength;  /*record tranfered data size*/
            }
			
            /* EOF has been received, return the data */
            if ((pRxTd->RDES0.val & GMAC_RDES0_LS) == GMAC_RDES0_LS) 
			{
                /*Frame size from the GMAC*/
                *pRcvSize = (pRxTd->RDES0.val & GMAC_RDES0_FL)>>16;
				
                /* Application frame buffer is too small all data have not been copied */
                if (FrmBufferSize < *pRcvSize) 
				{
                    return GMAC_RETURN_CODE_SIZE_TOO_SMALL;
                }
				
                pGmac->rx_last_des0=pRxTd->RDES0.val;
				
                /* All data have been copied in buffer , release TD */
                while ((pGmac->wRxI) != tmpIdx) 
				{
                    pRxTd = &pGmac->pRxD[pGmac->wRxI];
                    pRxTd->RDES0.val = (u32)(GMAC_RDES0_OWN);
                    GMAC_GCIRC_INC(pGmac->wRxI, pGmac->wRxListSize);
                }
				
                return GMAC_RETURN_CODE_OK;
            }
        } 
		else 
		{  /* isFrame = 0; SOF has not been detected */
            pRxTd->RDES0.val |= GMAC_RDES0_OWN;            /* ownered by DMA */
            pGmac->wRxI = tmpIdx;
        }
		
        /* Process the next buffer */
        pRxTd = &pGmac->pRxD[tmpIdx];
    }
	
    return GMAC_RETURN_CODE_RX_NULL;
}
/****************************************************************************/
/**
*
* Poll Rcv packet for ethernet frame(<1522 Byte)
* No copy , return pointer of frame
* one Des contains one frame
*
* @param    FGmacPs_Instance_T *pGmac
* @param    u32 *pRcvSize (as a return value)
*
* @return    pointer to Frame or NULL
*
* @note
*
****************************************************************************/
void *FGmac_Ps_RcvPollEFrame(FGmacPs_Instance_T *pGmac,u32 *pRcvSize)
{
    FGmacPs_RxDescriptor_T *pRxTd;

    u32 tmpIdx ;
    u8  isFrame = 0;
#if PSOC_CACHE_ENABLE
    u32 range_start, range_end;
#endif
    /* variables init */
    tmpIdx    = pGmac->wRxI;
    pRxTd     = &pGmac->pRxD[tmpIdx];

    *pRcvSize = 0;
	
#if PSOC_CACHE_ENABLE
    range_start = (u32)(pRxTd->BufferAdd1) & 0xffffffc0; /* 64 byte aligned */
    range_end = (((u32)(pRxTd->BufferAdd2) + GMAC_RBUFFER_UNIT_SIZE) & 0xffffffc0) + (1<<6);
	
#if 0  /* bare-code*/
		invalidate_dcache_range(range_start, range_end);
#else  /* vx-code*/
		/**/
		/* flush cache, it's must be for ddr flush new data*/
		/**/
		(void)cacheInvalidate (DATA_CACHE, FPS_AHB_SRAM_BASEADDR, 0x20000);   /* 118K ddr flush*/
#endif
#endif

    while ((pRxTd->RDES0.val & GMAC_RDES0_OWN) == 0)    /* ownered by HOST */
    {
        if((pRxTd->RDES0.val & GMAC_RDES0_FS) == GMAC_RDES0_FS)  /* First Descriptor */
        {
            /* Skip previous fragment */
            while(tmpIdx != pGmac->wRxI) {
                pRxTd = &pGmac->pRxD[pGmac->wRxI];
                pRxTd->RDES0.val |= GMAC_RDES0_OWN;            /* ownered by DMA */
                GMAC_GCIRC_INC(pGmac->wRxI,pGmac->wRxListSize);
            }
            /* Start to gather buffers in a frame */
            isFrame = 1;
        }

        /* Increase tmpIdx */
        GMAC_GCIRC_INC(tmpIdx,pGmac->wRxListSize);

        /* Copy data in the frame buffer */
        if(isFrame) {
            if(tmpIdx == pGmac->wRxI) {
                do{
                    pRxTd = &pGmac->pRxD[pGmac->wRxI];
                    pRxTd->RDES0.val |= GMAC_RDES0_OWN;
                    GMAC_GCIRC_INC(pGmac->wRxI, pGmac->wRxListSize);
                }while(tmpIdx != pGmac->wRxI);
                return NULL;
            }
            /* EOF has been received, return the data */
            if((pRxTd->RDES0.val & GMAC_RDES0_LS) == GMAC_RDES0_LS) {
                /*Frame size from the GMAC*/
                void * tmp_p=NULL;
                *pRcvSize = (pRxTd->RDES0.val & GMAC_RDES0_FL)>>16;
                pGmac->rx_last_des0=pRxTd->RDES0.val;
                /* All data have been copied in buffer , release TD */
                tmp_p=(void*)pRxTd->BufferAdd1;
                while((pGmac->wRxI) != tmpIdx) {
                    pRxTd = &pGmac->pRxD[pGmac->wRxI];
                    pRxTd->RDES0.val = (u32)(GMAC_RDES0_OWN);
                    GMAC_GCIRC_INC(pGmac->wRxI, pGmac->wRxListSize);
                }
                return tmp_p;
            }
        } else {
            /* isFrame = 0; SOF has not been detected */
            pRxTd->RDES0.val |= GMAC_RDES0_OWN;            /* ownered by DMA */
            pGmac->wRxI = tmpIdx;
        }
        /* Process the next buffer */
        pRxTd = &pGmac->pRxD[tmpIdx];
#if PSOC_CACHE_ENABLE
        range_start = (u32)(pRxTd->BufferAdd1) & 0xffffffc0; /* 64 byte aligned */
        range_end = (((u32)(pRxTd->BufferAdd2) + GMAC_RBUFFER_UNIT_SIZE) & 0xffffffc0) + (1<<6);
		
#if 0  /* bare-code*/
			invalidate_dcache_range(range_start, range_end);
#else  /* vx-code*/
			/**/
			/* flush cache, it's must be for ddr flush new data*/
			/**/
			(void)cacheInvalidate (DATA_CACHE, FPS_AHB_SRAM_BASEADDR, 0x20000);   /* 118K ddr flush*/
#endif
#endif
    }
    return NULL;
}

/****************************************************************************/
/**
*
* send a frame
*
* @param    FGmacPs_Instance_T *pGmac
* @param    u8 *pBuffer (point to frame to be sent)
* @param    u32 size (frame size to be sent)
* @param    u8 DisCRC (disable CRC auto-add)
* @param    u8 DisPAD (disable PAD auto-add)
*
* @return   GMAC_RETURN_CODE_OK
*
* @note    None.
*
****************************************************************************/
u8 FGmac_Ps_Send(FGmacPs_Instance_T *pGmac,u8 *pBuffer,u32 size,u8 DisCRC,u8 DisPAD)
{
    FGmacPs_DmaPortMap_T* pDmaPortMap;
    pDmaPortMap = (FGmacPs_DmaPortMap_T*)((u32)(pGmac->base_address)+GMAC_DMA_OFFSET);
    u32 reg_val_32b;

    reg_val_32b = FMSH_IN32_32(pDmaPortMap->GDMA_SR);  /* if stop, then start Tx */

	if ((reg_val_32b & GDMA_SR_TS) == 0) 
	{
        FGmac_Ps_SetTxState(pGmac,GMAC_STATE_RUN);
    }
	
#if GMAC_TX_COPY_MEM
    FGmac_Ps_PreSendCopy(pGmac,pBuffer,size,DisCRC,DisPAD);
#else
    FGmac_Ps_PreSendNoCopy(pGmac,pBuffer,size,DisCRC,DisPAD);
#endif

	/**/
	/* flush cache, it's must be for ddr flush new data*/
	/**/
	(void)cacheInvalidate (DATA_CACHE, 0x20000000, 0x1000000);   /* 16M ddr flush*/
	
    /* start to transmit */
    /*debug_gpio_low();*/
    gmac_DmaTxPollDemand(pGmac);


    /*debug_gpio_high();*/
    return GMAC_RETURN_CODE_OK;
}

/****************************************************************************/
/**
*
*  prepare to send a frame, let DES pt point to the frame; setup DES; but don't send
*
* @param    FGmacPs_Instance_T *pGmac
* @param    u8 *pBuffer (point to frame to be sent)
* @param    u32 size (frame size to be sent)
* @param    u8 DisCRC (disable CRC auto-add)
* @param    u8 DisPAD (disable PAD auto-add)
*
* @return   GMAC_RETURN_CODE_ERR, GMAC_RETURN_CODE_OK
*
* @note    TxDES use 11bit address for each buffer, so each Buffer is less than 2KB-1.
* we split giant frame in pieces of TxDesBufSize Byte
* We should set OWN BIT of 1st DES after all oprations have been done
*
****************************************************************************/
u8 FGmac_Ps_PreSendNoCopy(FGmacPs_Instance_T *pGmac,u8 *pFrame,u32 size,u8 DisCRC,u8 DisPAD)
{
    FGmacPs_TxDescriptor_T *pTxDES_tmp;   /* tmp pDES */
    u16 TxDES_Idx;				/* tmp Idx of TxDES */
    u8 TxDES_num_used;		/* number of DES used for this frame */
	
    u32 TDes_num=pGmac->wTxListSize;
    u32 TxDesBufSize=pGmac->TxDesBufSize;
	
	int i = 0;
	
#if PSOC_CACHE_ENABLE
    u32 range_start, range_end;
#endif


    /* initial */
    TxDES_Idx=pGmac->wTxHead;
    pTxDES_tmp = &pGmac->pTxD[0];
    TxDES_num_used=(size-1)/(TxDesBufSize*2)+1; /* whatif size == TxDesBufSize*N ? so use (size -1) */
    /* judge frame size */
    if(TxDES_num_used>(2*TDes_num)) {
        return GMAC_RETURN_CODE_ERR;
    }

    for (i=0; i<TxDES_num_used; i++) 
	{
        /* clear DES */
        pTxDES_tmp[TxDES_Idx].TDES0.val=0;
        pTxDES_tmp[TxDES_Idx].TDES1.val=0;
        /* setup own bit, except 1st DES */
        if(i!=0) {
            pTxDES_tmp[TxDES_Idx].TDES0.val |= GMAC_TDES0_OWN;
        }
        /* is last DES in list? */
        if(TxDES_Idx>=(TDes_num-1)) {
            pTxDES_tmp[TxDES_Idx].TDES1.val |= GMAC_TDES1_TER;
        }
        /* setup address 1 */
        pTxDES_tmp[TxDES_Idx].BufferAdd1 = (u32)(&pFrame[TxDesBufSize*i*2]);
        /* setup address2, no matter used or not*/
        pTxDES_tmp[TxDES_Idx].BufferAdd2 = (u32)(&pFrame[TxDesBufSize*i*2])+TxDesBufSize;
        /* setup buffer size */
        if(size<=(TxDesBufSize*i*2+TxDesBufSize)) {
            pTxDES_tmp[TxDES_Idx].TDES1.val |= GMAC_TDES1_TBS1&(size-TxDesBufSize*i*2);
            /* buffer size 2 =0 */
        }
        else if(size<=(TxDesBufSize*i*2+TxDesBufSize*2)) {
            pTxDES_tmp[TxDES_Idx].TDES1.val |= TxDesBufSize;
            pTxDES_tmp[TxDES_Idx].TDES1.val |= (size-TxDesBufSize*i*2-TxDesBufSize)<<11;
        } else {
            pTxDES_tmp[TxDES_Idx].TDES1.val |= TxDesBufSize;
            pTxDES_tmp[TxDES_Idx].TDES1.val |= (TxDesBufSize<<11);
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
        TxDES_Idx++;
        if(TxDES_Idx>=TDes_num)
            TxDES_Idx=0;
    }
    /* setup tail DES */
    pGmac->wTxTail=(pGmac->wTxHead+TxDES_num_used-1)%TDes_num;
    pTxDES_tmp[pGmac->wTxTail].TDES1.val |= GMAC_TDES1_LS|GMAC_TDES1_IC;
    pGmac->tx_last_des=(FGmacPs_TxDescriptor_T *)(&(pTxDES_tmp[pGmac->wTxTail]));
    /* setup head DES */
    pTxDES_tmp[pGmac->wTxHead].TDES1.val |= GMAC_TDES1_FS;
    pTxDES_tmp[pGmac->wTxHead].TDES0.val |= GMAC_TDES0_OWN;
    if(DisCRC) {
        pTxDES_tmp[pGmac->wTxHead].TDES1.val |= GMAC_TDES1_DC;
    } if(DisPAD) {
        pTxDES_tmp[pGmac->wTxHead].TDES1.val |= GMAC_TDES1_DP;
    }
	
    /* update head */
    pGmac->wTxHead=TxDES_Idx;
	
    /* after all, wTxHead point to the 1st DES of next frame,
    wTxTail still point to last DES, we can use wTxTail to get status after send */
	
    return GMAC_RETURN_CODE_OK;
}

/****************************************************************************/
/**
*
* prepare to send a frame, copy frame to buffer; setup DES; but don't send
*
* @param    FGmacPs_Instance_T *pGmac
* @param    u8 *pBuffer (point to frame to be sent)
* @param    u32 size (frame size to be sent)
* @param    u8 DisCRC (disable CRC auto-add)
* @param    u8 DisPAD (disable PAD auto-add)
*
* @return   GMAC_RETURN_CODE_ERR, GMAC_RETURN_CODE_OK
*
* @note    TxDES use 11bit address for each buffer, so each Buffer is less than 2KB-1.
* we split giant frame in pieces of TxDesBufSize Byte
* We should set OWN BIT of 1st DES after all oprations have been done
*
****************************************************************************/
u8 FGmac_Ps_PreSendCopy(FGmacPs_Instance_T *pGmac,u8 *pFrame,u32 size,u8 DisCRC,u8 DisPAD)
{
    FGmacPs_TxDescriptor_T *pTxDES_tmp; /* tmp pDES */
    u16 TxDES_Idx;						/* tmp Idx of TxDES */
    u8 TxDES_num_used;					/* number of DES used for this frame */
    u32 TDes_num=pGmac->wTxListSize;
    u32 TxDesBufSize=pGmac->TxDesBufSize;

	int i = 0;
	
#if PSOC_CACHE_ENABLE
    u32 range_start, range_end;
#endif

    /* initial */
    TxDES_Idx=pGmac->wTxHead;
    pTxDES_tmp = &pGmac->pTxD[0];
    TxDES_num_used=(size-1)/(TxDesBufSize*2)+1; /* whatif size == TxDesBufSize*N ? so use (size -1) */

	/* judge frame size */
    if (TxDES_num_used>(2*TDes_num)) 
	{
        return GMAC_RETURN_CODE_ERR;
    }

    for (i=0; i<TxDES_num_used; i++)
	{
        /* clear DES */
        pTxDES_tmp[TxDES_Idx].TDES0.val=0;
        pTxDES_tmp[TxDES_Idx].TDES1.val=0;
		
        /* setup own bit, except 1st DES */
        if(i!=0) 
		{
            pTxDES_tmp[TxDES_Idx].TDES0.val |= GMAC_TDES0_OWN;
        }
		
        /* is last DES in list? */
        if(TxDES_Idx>=(TDes_num-1))
		{
            pTxDES_tmp[TxDES_Idx].TDES1.val |= GMAC_TDES1_TER;
        }
		
        /* copy & setup buffer size */
        if(size<=(TxDesBufSize*i*2+TxDesBufSize)) 
		{
            pTxDES_tmp[TxDES_Idx].TDES1.val |= GMAC_TDES1_TBS1&(size-TxDesBufSize*i*2);
            memcpy((void *)pTxDES_tmp[TxDES_Idx].BufferAdd1, \
                pFrame+TxDesBufSize*i*2, \
                    size-TxDesBufSize*i*2);
            /* buffer size 2 =0 */
        } 
		else if(size<=(TxDesBufSize*i*2+TxDesBufSize*2)) 
		{
            pTxDES_tmp[TxDES_Idx].TDES1.val |= TxDesBufSize;
            memcpy((void *)pTxDES_tmp[TxDES_Idx].BufferAdd1, \
                pFrame+TxDesBufSize*i*2, \
                    TxDesBufSize);
            pTxDES_tmp[TxDES_Idx].TDES1.val |= (size-TxDesBufSize*i*2-TxDesBufSize)<<11;
            memcpy((void *)pTxDES_tmp[TxDES_Idx].BufferAdd2, \
                pFrame+TxDesBufSize*i*2+TxDesBufSize, \
                    size-TxDesBufSize*i*2-TxDesBufSize);
        } 
		else 
		{
            pTxDES_tmp[TxDES_Idx].TDES1.val |= TxDesBufSize;
            memcpy((void *)pTxDES_tmp[TxDES_Idx].BufferAdd1, \
                pFrame+TxDesBufSize*i*2, \
                    TxDesBufSize);
            pTxDES_tmp[TxDES_Idx].TDES1.val |= (TxDesBufSize<<11);
            memcpy((void *)pTxDES_tmp[TxDES_Idx].BufferAdd2, \
                pFrame+TxDesBufSize*i*2+TxDesBufSize, \
                    TxDesBufSize);
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
        TxDES_Idx++;
        if(TxDES_Idx>=TDes_num)  {
            TxDES_Idx=0;
        }
    }
	
    /* setup tail DES */
    pGmac->wTxTail=(pGmac->wTxHead+TxDES_num_used-1)%TDes_num;
    pTxDES_tmp[pGmac->wTxTail].TDES1.val |= GMAC_TDES1_LS|GMAC_TDES1_IC;
    pGmac->tx_last_des=(FGmacPs_TxDescriptor_T *)(&(pTxDES_tmp[pGmac->wTxTail]));
	
    /* setup head DES */
    pTxDES_tmp[pGmac->wTxHead].TDES1.val |= GMAC_TDES1_FS;
    pTxDES_tmp[pGmac->wTxHead].TDES0.val |= GMAC_TDES0_OWN;
	
    if(DisCRC) 
	{
        pTxDES_tmp[pGmac->wTxHead].TDES1.val |= GMAC_TDES1_DC;
    }
	
    if(DisPAD) 
	{
        pTxDES_tmp[pGmac->wTxHead].TDES1.val |= GMAC_TDES1_DP;
    }
	
    /* update head */
    pGmac->wTxHead=TxDES_Idx;
	
    return GMAC_RETURN_CODE_OK;
}

/****************************************************************************/
/**
*
* setup interrupt listerner function
*
* @param    FGmacPs_Instance_T *pGmac
* @param    FMSH_callback userFunction
*
* @return   void
*
* @note
*
****************************************************************************/
void FGmac_Ps_SetListener(FGmacPs_Instance_T *pGmac, FMSH_callback userFunction)
{
    /* check para */
    FMSH_REQUIRE(userFunction != NULL);
    /* setup func */
    pGmac->listener = userFunction;
}

/****************************************************************************/
/**
*
* setup Rx interrupt callback function
*
* @param    FGmacPs_Instance_T *pGmac
* @param    FMSH_callback userFunction
*
* @return   void
*
* @note
*
****************************************************************************/
void FGmac_Ps_SetRxCallback(FGmacPs_Instance_T *pGmac, FMSH_callback userFunction)
{
    /* check para */
    FMSH_REQUIRE(userFunction != NULL);
    /* setup func */
    pGmac->rxCallback = userFunction;
}

/****************************************************************************/
/**
*
* setup Tx interrupt callback function
*
* @param    FGmacPs_Instance_T *pGmac
* @param    FMSH_callback userFunction
*
* @return   void
*
* @note
*
****************************************************************************/
void FGmac_Ps_SetTxCallback(FGmacPs_Instance_T *pGmac, FMSH_callback userFunction)
{
    /* check para */
    FMSH_REQUIRE(userFunction != NULL);
    /* setup func */
    pGmac->txCallback = userFunction;
}

/****************************************************************************/
/**
*
* setup Tx state : run or stop
*
* @param    FGmacPs_Instance_T *pGmac
* @param    u8 state : GMAC_STATE_STOP or GMAC_STATE_RUN
*
* @return   GMAC_RETURN_CODE_OK or GMAC_RETURN_CODE_PARAM_ERR
*
* @note
*
****************************************************************************/
u8 FGmac_Ps_SetTxState(FGmacPs_Instance_T *pGmac,u8 state)
{
    FGmacPs_MacPortMap_T    *pGmacPortMap;
    u32 reg_val_32b=0;
    pGmacPortMap = pGmac->base_address;
    if(state==GMAC_STATE_STOP) { /* stop */
        /* disable tx DMA */
        gmac_dma_enable_tsv(pGmac,0);
        /* wait for previous frame trams done */
        do{
            reg_val_32b=FMSH_IN32_32(pGmacPortMap->GMAC_DBR);
        }while((reg_val_32b&(GMAC_DBR_TPESTS|GMAC_DBR_TFCSTS|GMAC_DBR_TXPAUSED|GMAC_DBR_TRCSTS|GMAC_DBR_TWCSTS|GMAC_DBR_TXFSTS|GMAC_DBR_TXSTSFSTS))!=0);
        /* disable tx MAC */
        gmac_enable_tsv(pGmac,0);
        /* make sure tx FIFO empty ??? */
    } else if(state==GMAC_STATE_RUN){  /* start */
        /* start tx DMA */
        gmac_dma_enable_tsv(pGmac,1);
        /* start tx MAC */
        gmac_enable_tsv(pGmac,1);
    } else {
        return GMAC_RETURN_CODE_PARAM_ERR;
    }
    return GMAC_RETURN_CODE_OK;
}

/****************************************************************************/
/**
*
* setup Rx state : run or stop
*
* @param    FGmacPs_Instance_T *pGmac
* @param    u8 state : GMAC_STATE_STOP or GMAC_STATE_RUN
*
* @return   GMAC_RETURN_CODE_OK
*
* @note
*
****************************************************************************/
u8 FGmac_Ps_SetRxState(FGmacPs_Instance_T *pGmac,u8 state)
{
    FGmacPs_MacPortMap_T    *pGmacPortMap;
    u32 reg_val_32b=0;
    pGmacPortMap = pGmac->base_address;
    if(state==GMAC_STATE_STOP)/*stop*/
    {
        /* disable rx MAC */
        gmac_enable_rcv(pGmac,0);
        /* wait for previous frame trams done */
        do
        {
            reg_val_32b=FMSH_IN32_32(pGmacPortMap->GMAC_DBR);
        }while((reg_val_32b&(GMAC_DBR_RPESTS|GMAC_DBR_RFCFCSTS_WR|GMAC_DBR_RFCFCSTS_RD|GMAC_DBR_RWCSTS|GMAC_DBR_RRCSTS|GMAC_DBR_RXFSTS))!=0);
        /* disable rx DMA */
        gmac_dma_enable_rcv(pGmac,0);
        /* make sure rx FIFO empty ???*/
    }
    else if(state==GMAC_STATE_RUN)/*start*/
    {
        /* start rx DMA */
        gmac_dma_enable_rcv(pGmac,1);
        /* start rx MAC */
        gmac_enable_rcv(pGmac,1);
    }
    return GMAC_RETURN_CODE_OK;
}

/****************************************************************************/
/**
*
* read mac reg54 (SGMII/RGMII/SMII Status Register), store in
* FGmacPs_Instance_T->gmac_link_status
*
* @param    FGmacPs_Instance_T *pGmac
*
* @return   GMAC_RETURN_CODE_OK
*
* @note
*
****************************************************************************/
u8 FGmac_Ps_GetLinkStatus(FGmacPs_Instance_T *pGmac)
{
    FGmacPs_DmaPortMap_T *pGmacDmaPortMap;
    pGmacDmaPortMap = (FGmacPs_DmaPortMap_T*)((u32)(pGmac->base_address)+GMAC_DMA_OFFSET);
    FGmacPs_MacPortMap_T *pGmacPortMap = pGmac->base_address;
    FGmacPs_LinkStatus_T *pGmacLinkSts=pGmac->gmac_link_status;

    u32 reg_val_32b=0;
    u8 interface_type;
    reg_val_32b=FMSH_IN32_32(pGmacDmaPortMap->GDMA_HWFT);
    interface_type=((reg_val_32b&GDMA_HWFT_ACTPHYIF)>>28);

    switch(interface_type)
    {
    case PHY_ITF_GMIIMII:	/* not finished */
        break;
		
    case PHY_ITF_RGMII:
        reg_val_32b =FMSH_IN32_32(pGmacPortMap->GMAC_MII_STS);
        pGmacLinkSts->smidrxs        = (reg_val_32b & GMAC_MII_STS_SMIDRXS) >> 16;
        pGmacLinkSts->fals_car_dect  = (reg_val_32b & GMAC_MII_STS_FALSCARDET) >> 5;
        pGmacLinkSts->jabber_timeout = (reg_val_32b & GMAC_MII_STS_JABTO) >> 4;
        pGmacLinkSts->link_status    = (reg_val_32b & GMAC_MII_STS_LNKSTS) >> 3;
        pGmacLinkSts->link_speed     = (reg_val_32b & GMAC_MII_STS_LNKSPEED) >> 1;
        pGmacLinkSts->link_mod       =  reg_val_32b & GMAC_MII_STS_LNKMOD;
        break;
		
    case PHY_ITF_SGMII:	/* not finished */
        break;
    case PHY_ITF_TBI:	/* not finished */
        break;
    case PHY_ITF_RMII:	/* not finished */
        break;
    case PHY_ITF_RTBI:	/* not finished */
        break;
    case PHY_ITF_SMII:	/* not finished */
        break;
    case PHY_ITF_RevMII:	/* not finished */
        break;
    }
    return GMAC_RETURN_CODE_OK;
}

/****************************************************************************/
/**
*
* get hardware feature of core configuration
*
* @param    FGmacPs_Instance_T *pGmac
*
* @return   GMAC_RETURN_CODE_OK
*
* @note
*
****************************************************************************/
u8 FGmac_Ps_GetHwFeature(FGmacPs_Instance_T *pGmac)
{
    /*   FGmacPs_DmaPortMap_T *pGmacDmaPortMap;


    u32 reg_val_32b=0;
    pGmacDmaPortMap = (FGmacPs_DmaPortMap_T *)((u32)pGmac->base_address + GMAC_DMA_OFFSET);

    reg_val_32b=FMSH_IN32_32(pGmacDmaPortMap->GDMA_HWFT);
    pGmac->gmac_cfg->interface=((reg_val_32b&GDMA_HWFT_ACTPHYIF)>>28);
    */
    return GMAC_RETURN_CODE_OK;
}

/****************************************************************************/
/**
*
* Write 1bit to status register to clear interrupts
*
* @param    FGmacPs_Instance_T *pGmac
* @param    FGmacPs_DmaIrq_T interrupts
*
* @return   void
*
* @note
*
****************************************************************************/
void FGmac_Ps_ClearIrq(FGmacPs_Instance_T *pGmac, FGmacPs_DmaIrq_T interrupts)
{
    FGmacPs_DmaPortMap_T *pGmacDmaPortMap;

    pGmacDmaPortMap = (FGmacPs_DmaPortMap_T *)((u32)pGmac->base_address+GMAC_DMA_OFFSET);

    if(interrupts == gdma_irq_all)
    {
        FMSH_OUT32_32(gdma_irq_all,pGmacDmaPortMap->GDMA_SR);
    }
    else
    {
        if((interrupts & gdma_irq_tx) != 0)
        {
            FMSH_OUT32_32(gdma_irq_tx,pGmacDmaPortMap->GDMA_SR);
        }
        else if((interrupts & gdma_irq_tx_unbuffer) != 0)
        {
            FMSH_OUT32_32(gdma_irq_tx_unbuffer,pGmacDmaPortMap->GDMA_SR);
        }
        else if((interrupts & gdma_irq_rx) != 0)
        {
            FMSH_OUT32_32(gdma_irq_rx,pGmacDmaPortMap->GDMA_SR);
        }
        else if((interrupts & gdma_irq_early_rx) != 0)
        {
            FMSH_OUT32_32(gdma_irq_early_rx,pGmacDmaPortMap->GDMA_SR);
        }
        else if((interrupts & gdma_irq_rx_unbuffer) != 0)
        {
            FMSH_OUT32_32(gdma_irq_rx_unbuffer,pGmacDmaPortMap->GDMA_SR);
        }
        else if((interrupts & gdma_irq_rx_stop) != 0)
        {
            FMSH_OUT32_32(gdma_irq_rx_stop,pGmacDmaPortMap->GDMA_SR);
        }
        else if((interrupts & gdma_irq_rx_overflow) != 0)
        {
            FMSH_OUT32_32(gdma_irq_rx_overflow,pGmacDmaPortMap->GDMA_SR);
        }
        else if((interrupts & gdma_irq_rx_wd_timeout) != 0)
        {
            FMSH_OUT32_32(gdma_irq_rx_wd_timeout,pGmacDmaPortMap->GDMA_SR);
        }
        else if((interrupts & gdma_irq_tx_stop) != 0)
        {
            FMSH_OUT32_32(gdma_irq_tx_stop,pGmacDmaPortMap->GDMA_SR);
        }
        else if((interrupts & gdma_irq_tx_jabber_timeout) != 0)
        {
            FMSH_OUT32_32(gdma_irq_tx_jabber_timeout,pGmacDmaPortMap->GDMA_SR);
        }
        else if((interrupts & gdma_irq_tx_underflow) != 0)
        {
            FMSH_OUT32_32(gdma_irq_tx_underflow,pGmacDmaPortMap->GDMA_SR);
        }
        else if((interrupts & gdma_irq_early_tx) != 0)
        {
            FMSH_OUT32_32(gdma_irq_early_tx,pGmacDmaPortMap->GDMA_SR);
        }
        else if((interrupts & gdma_irq_fatal_bus) != 0)
        {
            FMSH_OUT32_32(gdma_irq_fatal_bus,pGmacDmaPortMap->GDMA_SR);
        }
    }
}

/****************************************************************************/
/**
*
* reset gmac instance struct
*
* @param    FGmacPs_Instance_T * pGmac
* @param    FGmacPs_LinkStatus_T * gmac_link_status,
* @param    FGmacPs_Config_T * gmac_cfg,
* @param    FGmacPs_PhyConfig_T * phy_cfg,
* @param    u32 RDes_num,
* @param    u32 TDes_num,
* @param    u32 RxDesBufSize,
* @param    u32 TxDesBufSize,
* @param    u8* pPacketBuffer,
* @param    u32 FrmBufferSize
*
* @return   GMAC_RETURN_CODE_OK
*
* @note
*
****************************************************************************/
u8 FGmac_Ps_StructInit(FGmacPs_Instance_T *pGmac,
                       FGmacPs_LinkStatus_T * gmac_link_status,
                       FGmacPs_Config_T * gmac_cfg,
                       FGmacPs_PhyConfig_T * phy_cfg,
                       u32 RDes_num, u32 TDes_num,
                       u32 RxDesBufSize, u32 TxDesBufSize,
                       u8* pPacketBuffer, u32 FrmBufferSize)
{
    pGmac -> index = gmac_cfg->DeviceId;
    pGmac -> gmac_cfg = gmac_cfg;

    pGmac->wRxListSize = RDes_num;
    pGmac->wTxListSize = TDes_num;

    pGmac->RxDesBufSize = RxDesBufSize;
    pGmac->TxDesBufSize = TxDesBufSize;

    pGmac -> pFrmBuffer = pPacketBuffer;
    pGmac -> FrmBufferSize = FrmBufferSize;

    /*gmac_cfg->speed=gmac_ip_cfg->Speed;*/
    /*gmac_cfg->interface=gmac_ip_cfg->InterFaceType;*/
    phy_cfg->speed=gmac_cfg->Speed;
    phy_cfg->interface=gmac_cfg->InterFaceType;

    pGmac-> gmac_link_status=gmac_link_status;
    /*pGmac-> gmac_cfg=gmac_cfg;*/
    pGmac-> phy_cfg=phy_cfg;

    pGmac -> base_address = (void*)(gmac_cfg->BaseAddress);
    pGmac -> wTxHead=0;
    pGmac -> wTxTail=0;

    if (PHY_88E1116R == phy_cfg->phy_device)
    {
        phy_cfg->phy_op_init     = mvl88e1116r_init;
        phy_cfg->phy_op_reset    = mvl88e1116r_reset;
        phy_cfg->phy_op_cfg      = mvl88e1116r_cfg;
        phy_cfg->phy_op_reg_dump = mvl88e1116r_reg_dump;
    }
	else if (PHY_KSZ9031RNX == phy_cfg->phy_device)
    {
        /*
        phy_cfg->phy_op_init     = mic9031RNX_init;
        phy_cfg->phy_op_reset    = mic9031RNX_reset;
        phy_cfg->phy_op_cfg      = mic9031RNX_cfg;
        phy_cfg->phy_op_reg_dump = mic9031RNX_reg_dump;
		*/
    }
	else if (PHY_88E1111 == phy_cfg->phy_device)
    {
        /*
        phy_cfg->phy_op_init     = mvl88e1111_init;
        phy_cfg->phy_op_reset    = mvl88e1111_reset;
        phy_cfg->phy_op_cfg      = mvl88e1111_cfg;
        phy_cfg->phy_op_reg_dump = mvl88e1111_reg_dump;
		*/
    }
	else if (PHY_GENERIC == phy_cfg->phy_device)
	{
        phy_cfg->phy_op_init     = fmsh_mdio_init;
        phy_cfg->phy_op_reset    = fmsh_mdio_reset;
        phy_cfg->phy_op_cfg      = fmsh_mdio_init;
        phy_cfg->phy_op_reg_dump = (u8 (*)(FGmacPs_Instance_T * pGmac))fmsh_mdio_reg_dump;
    }
	
    return GMAC_RETURN_CODE_OK;
}

/****************************************************************************/
/**
*
* gmac device reset
*
* @param    FGmacPs_Instance_T *pGmac
*
* @return   GMAC_RETURN_CODE_OK
*
* @note     dma reg0 bit0 is useless, use SLCR reg to reset
*
****************************************************************************/
u8 FGmac_Ps_DeviceReset(FGmacPs_Instance_T *pGmac)
{
    FGmacPs_Config_T *pGmacCfg = pGmac->gmac_cfg;
	
    if(pGmac->index==FPAR_GMACPS_0_DEVICE_ID)
    {
        gmac0_bus_rst(pGmacCfg->InterFaceType);
    }
    else if(pGmac->index==FPAR_GMACPS_1_DEVICE_ID)
    {
        gmac1_bus_rst(pGmacCfg->InterFaceType);
    }
    else
    {
        return GMAC_RETURN_CODE_ERR;
    }
    return GMAC_RETURN_CODE_OK;
}


