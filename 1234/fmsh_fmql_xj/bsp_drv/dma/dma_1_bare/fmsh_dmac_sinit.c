/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_dmac_sint.c
*
* This file contains 
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 0.01   yl  08/27/2019  First Release
*</pre>
******************************************************************************/

/***************************** Include Files *********************************/
#include <stdlib.h>

#include "../../common/fmsh_ps_parameters.h"
/*#include "fmsh_ps_parameters.h"*/

#include "fmsh_dmac.h"          /* DMA Controller header*/

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/
extern FDmaPs_Config FDmaPs_ConfigTable[];

/************************** Function Prototypes ******************************/

/****************************************************************************/
/**
*
* Looks up the device configuration based on the unique device ID. The table
* contains the configuration info for each device in the system.
*
* @param DeviceId contains the ID of the device
*
* @return
*
* A pointer to the configuration structure or NULL if the specified device
* is not in the system.
*
* @note
*
* None.
*
******************************************************************************/
FDmaPs_Config *FDmaPs_LookupConfig(u16 DeviceId)
{
    FDmaPs_Config *CfgPtr = NULL;

    int i;

    for (i = 0; i < FPAR_DMAPS_NUM_INSTANCES; i++)
    {
        if (FDmaPs_ConfigTable[i].DeviceId == DeviceId)
        {
            CfgPtr = &FDmaPs_ConfigTable[i];
            break;
        }
    }

    return CfgPtr;
}



#if 1

/* Source memory location*/
#define SRC_MEMORY_BASE          (FPS_AHB_SRAM_BASEADDR)

/* Destination memory location*/
#define DST_MEMORY_BASE          (FPS_AHB_SRAM_BASEADDR + 0x10000)

FDmaPs_T g_DMA_dmac;
FDmaPs_Param_T g_DMA_param;
FDmaPs_Instance_T g_DMA_instance;

static volatile BOOL s_DMA_tfrFlag;
static volatile BOOL s_DMA_errFlag;
static volatile BOOL s_DMA_srcTranFlag;
static volatile BOOL s_DMA_dstTranFlag;

#define DISABLE_INT        (1)

/************************** Function Prototypes ******************************/

void FDmaPs_IRQ (void *InstancePtr)
{
	FDmaPs_irqHandler((FDmaPs_T *)InstancePtr);
}

/*********************************************************************/
void userCallback(void *pDev, int eCode)
{
    /* -------*/
    /* This function is called by the IRQ handler.*/
    /* Here we are going to set a global flag to inform the*/
    /* application that the DMA transfer has completed.*/
    /* -------*/
    s_DMA_tfrFlag = TRUE;
}

/*********************************************************************/
void userListener(void *pDev, int eCode)
{
    /* Check the source of the interrupt*/
    if (eCode == Dmac_irq_err)
	{
        /* Just inform the application that this interrupt has occured.*/
        s_DMA_errFlag = TRUE;
    }

    if (eCode == Dmac_irq_srctran)
	{
        /* Just inform the application that this interrupt has occured.*/
        s_DMA_srcTranFlag = TRUE;
    }

    if (eCode == Dmac_irq_dsttran)
	{
        /* Just inform the application that this interrupt has occured.*/
        s_DMA_dstTranFlag = TRUE;
    }

    if (eCode == Dmac_irq_block)
	{
        /* We fully expect this interrupt to fire each time the DMA*/
        /* controller has completed the transfer of a block of data.*/
        /* For this example we are not concerned that a block has*/
        /* completed, only that the transfer completes. So we do*/
        /* nothing here.*/
    }
}

/*********************************************************************/
s32 FDmaPs_funcVerify(FDmaPs_T *pDmac)
{
    int i, j, chIndex, errorCode;
    u32 blockSize, numBlocks, dataLen;
    u32 dst_data, exp_data, *dst_addr, *src_addr;
    enum FDmaPs_channelNumber chNum;
    enum FDmaPs_transferType transType;
    FDmaPs_ChannelConfig_T ch_config;  /* Channel configuration struct.*/

    /* Initialise the DMA controller*/
    FDmaPs_init(pDmac);

    /* Transfer characteristics*/
    chNum = Dmac_channel0; /* Select a DMA channel*/
    transType = Dmac_transfer_row1; /* Select a transfer type*/
    blockSize = 2000; /* Select the size of a block*/
    numBlocks = 1; /* Select the number of blocks*/
    dataLen = blockSize * numBlocks;

    /* Initialise the memory regions on both the source and the*/
    /* destination memories.*/
	src_addr = (u32 *)SRC_MEMORY_BASE;  /* 0xE1FE0000 ahb_sram*/
	{
		for(i = 0; i < dataLen; i++)
	    {
	    	*src_addr = i + 1;
			src_addr++;
	    }
	}
	
	dst_addr = (u32 *)DST_MEMORY_BASE;  /* 0xE1FE0000+0x10000 ahb_sram*/
	{
		memset(dst_addr, 0, dataLen * sizeof(u32));
	}

    /* Get the channel index from the enumerated type*/
    chIndex = FDmaPs_getChannelIndex(chNum);

    /* START : Channel configuration*/
    /* -------*/
    errorCode = FDmaPs_setTransferType(pDmac, chNum, transType);
    if (errorCode != 0)
	{
        TRACE_OUT(DEBUG_OUT, "ERROR: Failed to set the transfer type row %d\r\n", transType);
    }
    
    /* Enable the interrupts on Channel x*/
    errorCode = FDmaPs_enableChannelIrq(pDmac, chNum);
    if (errorCode != 0)
	{
        TRACE_OUT(DEBUG_OUT, "ERROR: Failed to enable channel %d interrupts\r\n", chIndex);
    }

    /* Initialise the channel configuration structure.*/
    errorCode = FDmaPs_getChannelConfig(pDmac, chNum, &ch_config);
    if (errorCode != 0)
	{
        TRACE_OUT(DEBUG_OUT, "ERROR: Failed to initialise configuration ");
        TRACE_OUT(DEBUG_OUT, "structure from the DMA registers on channel %d\r\n", chIndex);
    }

    /* Change the configuration structure members to initialise the*/
    /* DMA channel for the chosen transfer.*/
    /* Set the Source and destination addresses*/
    ch_config.sar = SRC_MEMORY_BASE;
    ch_config.dar = DST_MEMORY_BASE;
	
    /* Set the source and destination transfer width*/
    ch_config.ctl_src_tr_width = Dmac_trans_width_32;
    ch_config.ctl_dst_tr_width = Dmac_trans_width_32;
	
    /* Set the Address increment type for the source and destination*/
    ch_config.ctl_sinc = Dmac_addr_increment;
    ch_config.ctl_dinc = Dmac_addr_increment;
	
    /* Set the source and destination burst transaction length*/
    ch_config.ctl_src_msize = Dmac_msize_32;
    ch_config.ctl_dst_msize = Dmac_msize_32;
	
	/* set scatter/gather enable and parameters*/
	ch_config.ctl_dst_scatter_en = FMSH_clear;
	ch_config.ctl_src_gather_en = FMSH_clear;
	
    /* Set the block size for the DMA transfer*/
    /* Block size is the number of words of size Dmac_trans_width*/
    ch_config.ctl_block_ts = blockSize;
	
    /* Set the transfer device type and flow controller*/
    ch_config.ctl_tt_fc = Dmac_mem2mem_dma;

    /* Write the new configuration setting into the DMA Controller device.*/
    errorCode = FDmaPs_setChannelConfig(pDmac, chNum, &ch_config);
    if (errorCode != 0)
	{
        TRACE_OUT(DEBUG_OUT, "ERROR: Failed to write configuration structure ");
        TRACE_OUT(DEBUG_OUT, "into the DMA controller registers on channel %d\r\n", chIndex);
    }
    /* -------*/
    /* END : Channel configuration*/

    /* display the transfer information.*/
    TRACE_OUT(DEBUG_OUT, "/ -----\r\n");
    TRACE_OUT(DEBUG_OUT, "  Channel number       %d\r\n", chIndex);
    TRACE_OUT(DEBUG_OUT, "  Transfer type        Row %d\r\n", transType);
    TRACE_OUT(DEBUG_OUT, "  Block Size           %u\r\n", blockSize);
    TRACE_OUT(DEBUG_OUT, "  Number of blocks     %u\r\n", numBlocks);
    TRACE_OUT(DEBUG_OUT, "  Source address       %x\r\n", ch_config.sar);
    TRACE_OUT(DEBUG_OUT, "  Destination address  %x\r\n", ch_config.dar);
    TRACE_OUT(DEBUG_OUT, "/ -----\r\n");

#if DISABLE_INT	
	/* do nothing*/
#else
    /* Before beginning an interrupt driven transfer the user must*/
    /* register a listener function in the DMA driver. */
    /* -------*/
    FDmaPs_setListener(pDmac, chNum, userListener);
#endif

    /* Initialise global flags to FALSE*/
    s_DMA_tfrFlag = FALSE;
    s_DMA_errFlag = FALSE;
    s_DMA_srcTranFlag = FALSE;
    s_DMA_dstTranFlag = FALSE;

    /* Enable the DMA controller and begin the interrupt driven DMA transfer.*/
    FDmaPs_enable(pDmac);
	
    errorCode = FDmaPs_startTransfer(pDmac, chNum, numBlocks, userCallback);
    if (errorCode != 0)
	{
        TRACE_OUT(DEBUG_OUT, "ERROR: Failed to begin the interrupt transfer\r\n");
    }

    /* Now we monitor the transfers progress and wait for completion*/
    /* -------*/
#if DISABLE_INT
	u32 tmp;
	tmp = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_RAW_TFR_L_OFFSET);
	while ((tmp & 0x1) == 0)	/* transfer is in progress*/
#else
	while (s_DMA_tfrFlag == FALSE)	/* transfer is in progress*/
#endif
	{
        /* Heartbeat*/
        TRACE_OUT(DEBUG_OUT, ".");

        /* Just checking here that no error condition interrupts*/
        /* fire during the data transfer.*/

        if(s_DMA_errFlag == TRUE)
		{
            /* Wasn't expecting this !!*/
            TRACE_OUT(DEBUG_OUT, "\nERROR: Recieved an err interrupt\r\n");
        }

        if (s_DMA_srcTranFlag == TRUE)
		{
            /* This interrupt should not be unmasked when*/
            /* a memory device is on the source !!*/
            TRACE_OUT(DEBUG_OUT, "\nERROR: Received an srcTran interrupt\r\n");
        }

        if (s_DMA_dstTranFlag == TRUE)
		{
            /* This interrupt should not be unmasked when*/
            /* a memory device is on the destination !!*/
            TRACE_OUT(DEBUG_OUT, "\nERROR: Received an dstTran interrupt\r\n");
        }
		
#if DISABLE_INT
		tmp = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_RAW_TFR_L_OFFSET);
#endif

    }


	TRACE_OUT(DEBUG_OUT, "\r\n");

    /* -------*/
    /* Disable the DMA after transfer is complete.*/
    /* -------*/
    FDmaPs_disable(pDmac);
	
    src_addr = (u32 *) ch_config.sar;
    dst_addr = (u32 *) ch_config.dar;
    for (j = 0; j < blockSize; j++)
	{
        exp_data = *(u32 *) src_addr++;
        dst_data = *(volatile u32 *) dst_addr++;
        if (exp_data != dst_data)
		{
        	TRACE_OUT(DEBUG_OUT, "dst_addr = %x : dst_data = %x : exp_data = %x\r\n",
				      dst_addr-1, dst_data, exp_data);
			return FMSH_FAILURE;
        }
    }

    return FMSH_SUCCESS;
}


s32 dmac_test_example(void)
{
    s32 status = FMSH_FAILURE;
	
    FDmaPs_Config *pDmaCfg;
	
    FDmaPs_T *pDmac = &g_DMA_dmac;
	
    FDmaPs_Instance_T *pInstance = &g_DMA_instance;
    FDmaPs_Param_T *pParam = &g_DMA_param;

    memset(pInstance, 0, sizeof(FDmaPs_Instance_T));
    memset(pParam, 0, sizeof(FDmaPs_Param_T));

    /* Initialize the DMA Driver */
    pDmaCfg = FDmaPs_LookupConfig(FPAR_DMAPS_DEVICE_ID);
    if (pDmaCfg == NULL) {
        return FMSH_FAILURE;
    }
	
    FDmaPs_initDev(pDmac, pInstance, pParam, pDmaCfg);
	
#if DISABLE_INT	
	/* do nothing*/
#else
    status = FGicPs_registerInt(&IntcInstance, DMA_INT_ID,
              (FMSH_InterruptHandler)FDmaPs_IRQ, pDmac);
    if (status != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;
    }
#endif

	status = FDmaPs_autoCompParams(pDmac);
    if (status != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;
    }
    
	TRACE_OUT(DEBUG_OUT, "Begin to run DMA test example.\r\n");
	TRACE_OUT(DEBUG_OUT, "/ -------\r\n");
    
	status = FDmaPs_funcVerify(pDmac);
	if (status != FMSH_SUCCESS)
	{
		TRACE_OUT(DEBUG_OUT, "DMA test example FAILED.\r\n");
	}
	else
	{
		TRACE_OUT(DEBUG_OUT, "DMA test example PASSED.\r\n");
	}

	return status;
}

s32 test_dmac_init(void)
{
    s32 status = FMSH_FAILURE;
    FDmaPs_Config *pDmaCfg;
    FDmaPs_T *pDmac = &g_DMA_dmac;
    FDmaPs_Instance_T *pInstance = &g_DMA_instance;
    FDmaPs_Param_T *pParam = &g_DMA_param;

    memset(pInstance, 0, sizeof(FDmaPs_Instance_T));
    memset(pParam, 0, sizeof(FDmaPs_Param_T));

    /* Initialize the DMA Driver */
    pDmaCfg = FDmaPs_LookupConfig(FPAR_DMAPS_DEVICE_ID);
    if (pDmaCfg == NULL) {
        return FMSH_FAILURE;
    }
	
    FDmaPs_initDev(pDmac, pInstance, pParam, pDmaCfg);
	
#if DISABLE_INT	
	/* do nothing*/
#else
    status = FGicPs_registerInt(&IntcInstance, DMA_INT_ID,
              (FMSH_InterruptHandler)FDmaPs_IRQ, pDmac);
    if (status != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;
    }
#endif

	status = FDmaPs_autoCompParams(pDmac);
    if (status != FMSH_SUCCESS)
    {
        return FMSH_FAILURE;
    } 

#ifdef DRV_FM_SDMMC	
	g_SDMMC_dataParam.trans_mode = sdmmc_trans_mode_dw_dma;
#endif

/*	
	TRACE_OUT(DEBUG_OUT, "Begin to run DMA test example.\r\n");
	TRACE_OUT(DEBUG_OUT, "/ -------\r\n");
    
	status = FDmaPs_funcVerify(pDmac);
	if (status != FMSH_SUCCESS)
	{
		TRACE_OUT(DEBUG_OUT, "DMA test example FAILED.\r\n");
	}
	else
	{
		TRACE_OUT(DEBUG_OUT, "DMA test example PASSED.\r\n");
	}
*/
	return status;
}

#endif

