/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_dmac.c
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
* 0.01   yl  12/20/2018  First Release
*</pre>
******************************************************************************/

/***************************** Include Files *********************************/
#include <stdlib.h>

#include "../../common/fmsh_common.h"
#include "../../common/fmsh_ps_parameters.h"
/*#include "fmsh_ps_parameters.h"*/
/*#include "fmsh_common.h"            // common definitions for all devices*/

#include "fmsh_dmac_lib.h"          /* DMA Controller header*/

#include "fmsh_dmac.h"          /* DMA Controller header*/

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

/*****************************************************************************
*
* @description
* This function is initialize the DMA device.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           pInstance is the pointer to instance structure.
*           pParam is the pointer to parameter structure.
*
* @return   
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FDmaPs_initDev(FDmaPs_T *pDmac,
	                    FDmaPs_Instance_T *pInstance,
	                    FDmaPs_Param_T *pParam,
	                    FDmaPs_Config *pDmaCfg)
{
	/* initial device*/
	pDmac->config.DeviceId = pDmaCfg->DeviceId;
	pDmac->config.BaseAddress = pDmaCfg->BaseAddress;
	pDmac->instance = pInstance;
	pDmac->comp_param = pParam;
	pDmac->comp_version = 0x3230322A;
	pDmac->comp_type = FMSH_ahb_dmac;
	FMSH_INIT_LIST_HEAD(&pDmac->list);
}

/*****************************************************************************
*
* @description
* Reset the DMA controller by SLCR registers.
*
* @param	
*           NA.
*
* @return   
*           NA.
*
* @note		
*           NA.
*
*****************************************************************************/
void FDmaPs_resetController(void)
{
	u32 i;
	
	/* unlock SLCR */
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x008, 0xDF0D767B);
	/* set rst_ctrl */
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x328, 1);

	for(i = 0; i< 50; i++);
	
	/* clear rst_ctrl */
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x328, 0);
	/* relock SLCR */
	FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x004, 0xDF0D767B);
}

/*****************************************************************************
*
* @description
* This function is used to initialize the DMA controller. All
* interrupts are cleared and disabled; DMA channels are disabled; and
* the device instance structure is reset.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_init(FDmaPs_T *pDmac)
{
    int errorCode;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    /* Remove from test mode*/
    FDmaPs_setTestMode(pDmac, FMSH_clear);
    
    /* Reset the DMA instance structure*/
    FDmaPs_resetInstance(pDmac);
    
    /* Disable the DMA controller*/
    errorCode = FDmaPs_disable(pDmac);
    if (errorCode == 0)
	{
        /* Disable all DMA channels*/
        errorCode = FDmaPs_disableChannel(pDmac, Dmac_all_channels);
    }
    if (errorCode == 0)
	{
        /* Disable all channel interrupts*/
        errorCode = FDmaPs_disableChannelIrq(pDmac, Dmac_all_channels);
    }
    if (errorCode == 0)
	{
        /* Mask all channel interrupts*/
        errorCode = FDmaPs_maskIrq(pDmac, Dmac_all_channels, Dmac_irq_all);
    }
    if (errorCode == 0)
	{
        /* Clear any pending interrupts*/
         errorCode = FDmaPs_clearIrq(pDmac, Dmac_all_channels, Dmac_irq_all);
    }

    return errorCode;
}

/*****************************************************************************
*
* @description
* This function will enable the DMA controller.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*
* @return   
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FDmaPs_enable(FDmaPs_T *pDmac)
{
    DMAC_COMMON_REQUIREMENTS(pDmac);
	
	FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_DMA_CFG_REG_L_OFFSET, 0x1);
}

/*****************************************************************************
*
* @description
* This function will disable the dma controller.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_disable(FDmaPs_T *pDmac)
{
    int errorCode;
    u32 reg;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    errorCode = 0;

    /* Check first to see if DMA is already disabled*/
	reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_DMA_CFG_REG_L_OFFSET);
	reg &= DMAC_DMACFGREG_L_DMA_EN;

    if (reg != 0x0)
	{
        /*reg = 0x0;*/
		FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_DMA_CFG_REG_L_OFFSET, 0x0);

        /* Ensure that the DMA was disabled*/
        /* May not disable due to split response on one*/
        /* of the DMA channels*/
        
		if (FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_DMA_CFG_REG_L_OFFSET))
		{
			errorCode = -FMSH_EBUSY;
		}
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns when the DMA controller is enabled.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*
* @return   
*           TRUE if the DMA controller is enabled.
*           FALSE if the DMA controller is not enabled.
*
* @note     NA.
*
*****************************************************************************/
BOOL FDmaPs_isEnabled(FDmaPs_T *pDmac)
{
    u32 reg;
    BOOL retval;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    /* read the DMA_CFG_REG*/
	reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_DMA_CFG_REG_L_OFFSET);
	retval = (BOOL)(reg & DMAC_DMACFGREG_L_DMA_EN);
    FMSH_ASSERT((retval == TRUE) || (retval == FALSE));
    return retval;
}

/*****************************************************************************
*
* @description
* This function enables the specified DMA channel(s).
* Multiple DMA channels can be specified for the FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_enableChannel(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    int errorCode;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* The FDmaPs_channelNumber enum is declared such that*/
        /* the enumerated value maps exactly to the value that*/
        /* needs to be written into the ChEnReg for enabling.*/
        reg = ch_num;
		FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CH_EN_REG_L_OFFSET, reg);
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function disables the specified DMA channel(s).
* Multiple DMA channels can be specified for the FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_disableChannel(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    int errorCode;
    u32 reg;
    u32 enabled_ch;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errorCode = FDmaPs_checkChannelRange(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Check first if the channel was already disabled*/
		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CH_EN_REG_L_OFFSET);
        enabled_ch = reg & ch_num;

        if (enabled_ch != 0x0)
		{
            reg = ch_num & (DMAC_MAX_CH_MASK << DMAC_MAX_CHANNELS);
			FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CH_EN_REG_L_OFFSET, reg);

            /* Ensure that the channel(s) was disabled.*/
            /* Channel may not disable due to a split response.*/
			if (FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CH_EN_REG_L_OFFSET) & ch_num)
			{
				errorCode = -FMSH_EBUSY;
			}
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns whether the specified DMA channel is enabled.
* Only ONE DMA channel can be specified for the FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           TRUE if the channel is enabled.
*           FALSE if the channel is not enabled.
*
* @note     NA.
*
*****************************************************************************/
BOOL FDmaPs_isChannelEnabled(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    u8 ch_index;
    u32 reg;
    BOOL retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

	reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CH_EN_REG_L_OFFSET);
	retval = (BOOL)((reg & DMAC_CHENREG_L_CH_EN(ch_index)) >> ch_index);
    FMSH_ASSERT((retval == TRUE) || (retval == FALSE));
    return retval;
}

/*****************************************************************************
*
* @description
* This function returns the lower byte of the channel enable register(ChEnReg).
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*
* @return   
*           Contents of the lower byte of the ChEnReg.
*
* @note     NA.
*
*****************************************************************************/
u8 FDmaPs_getChannelEnableReg(FDmaPs_T *pDmac)
{
    u32 reg;
    u8 retval;

    DMAC_COMMON_REQUIREMENTS(pDmac);
	
	reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CH_EN_REG_L_OFFSET);
	retval = (u8)(reg & DMAC_CHENREG_L_CH_EN_ALL);
    return retval;
}

/*****************************************************************************
*
* @description
* This function enables interrupts for the selected channel(s).
* Multiple DMA channels can be specified for the FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_enableChannelIrq(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    int x;
    int errorCode;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if(ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    if (errorCode == 0)
	{

        /* Loop through each channel in turn and enable*/
        /* the channel Irq for the selected channels.*/
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(x));
				if ((reg & DMAC_CTL_L_INT_EN) != 0x1)
				{
					reg |= DMAC_CTL_L_INT_EN;
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(x), reg);
				}
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function disables interrupts for the selected channel(s).
* Multiple DMA channels can be specified for the FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_disableChannelIrq(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    int x;
    int errorCode;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Loop through each channel in turn and disable*/
        /* the channel Irq for the selected channels.*/
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(x));
				
				if ((reg & DMAC_CTL_L_INT_EN) != 0x0)
				{
					reg &= ~DMAC_CTL_L_INT_EN;					
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(x), reg);
				}
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns whether interrupts are enabled for the 
* specified DMA channel. Only ONE DMA channel can be specified for
* the FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           TRUE if channel interrupts are enabled.
*           FALSE if channel interrupts are not enabled.
*
* @note     NA.
*
*****************************************************************************/
BOOL FDmaPs_isChannelIrqEnabled(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    u8 ch_index;
    BOOL retval;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

	reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(ch_index));
	retval = (BOOL)(reg & DMAC_CTL_L_INT_EN);
    FMSH_ASSERT((retval == TRUE) || (retval == FALSE));
    return retval;
}

/*****************************************************************************
*
* @description
* This function returns a DMA channel number (enumerated) that is
* disabled. The function starts at channel 0 and increments up
* through the channels until a free channel is found.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*
* @return   
*           DMA channel number, as an enumerated type.
*
* @note     NA.
*
*****************************************************************************/
enum FDmaPs_channelNumber FDmaPs_getFreeChannel(
        FDmaPs_T *pDmac)
{
    int x,y;
    u32 reg;
    FDmaPs_Param_T *param;
    enum FDmaPs_channelNumber retval;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    retval = Dmac_no_channel;

    /* read the channel enable register*/
	reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CH_EN_REG_L_OFFSET);
	reg &= DMAC_CHENREG_L_CH_EN_ALL;

    /* Check each channel in turn until we find one*/
    /* that is NOT enabled.  Loop checks channels*/
    /* starting at channel 0 and works up incrementally.*/
    for (x = 0; x < param->num_channels; x++)
	{
        y = (1 << x);
        if (!(reg & y))
		{
            retval = (enum FDmaPs_channelNumber)((y << DMAC_MAX_CHANNELS) | y);
            break;
        }
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function suspends transfers on the specified channel(s).
* Multiple DMA channels can be specified for the FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_suspendChannel(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    int x;
    int errorCode;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errorCode = FDmaPs_checkChannelRange(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Loop through each of the selected channels and*/
        /* read the current CFGx register, set the suspend*/
        /* bit and write the register back out again.*/
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(x));
				if ((reg & DMAC_CFG_L_CH_SUSP) == 0x0)
				{
					reg |= DMAC_CFG_L_CH_SUSP;
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(x), reg);
				}
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function resumes (remove suspend) on the specified channel(s).
* Multiple DMA channels can be specified for the FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_resumeChannel(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    int x;
    int errorCode;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errorCode = FDmaPs_checkChannelRange(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Loop through each of the selected channels and*/
        /* read the current CFGx register, clear the suspend*/
        /* bit and write the register back out again.*/
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(x));
				if((reg & DMAC_CFG_L_CH_SUSP) != 0x0)
				{
					reg &= ~DMAC_CFG_L_CH_SUSP;
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(x), reg);
				}
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns whether the specified channel is suspended.
* Only ONE DMA channel can be specified for the FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           TRUE if the channel is suspended.
*           FALSE if the channel is not suspended.
*
* @note     NA.
*
*****************************************************************************/
BOOL FDmaPs_isChannelSuspended(
             FDmaPs_T *pDmac,
             enum FDmaPs_channelNumber ch_num)
{
    u8 ch_index;
    BOOL retval;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

	reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(ch_index));
	retval = (BOOL)((reg & DMAC_CFG_L_CH_SUSP) >> 8);
    FMSH_ASSERT((retval == TRUE) || (retval == FALSE));
    return retval;
}

/*****************************************************************************
*
* @description
* This function clears the specified interrupt(s) on the specified channel(s).
* Multiple DMA channels can be specified for the FDmaPs_channelNumber argument.
* Multiple interrupt types can be specified for the FDmaPs_irq argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           ch_irq Enumerated interrupt type.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_clearIrq(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_irq ch_irq)
{
    int x;
    int errorCode;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errorCode = FDmaPs_checkChannelRange(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Loop through and clear the selected channel Irq*/
        /* for the targeted channels.*/
        reg = ch_num & DMAC_MAX_CH_MASK;
        for (x = 0; x < DMAC_MAX_INTERRUPTS; x++)
		{
            if (ch_irq & (1 << x))
			{
                switch (x)
				{
                    case 0 : FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_CLEAR_TFR_L_OFFSET, reg);
                             break;
                    case 1 : FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_CLEAR_BLOCK_L_OFFSET, reg);
                             break;
                    case 2 : FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_CLEAR_SRCTRAN_L_OFFSET, reg);
                             break;
                    case 3 : FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_CLEAR_DSTTRAN_L_OFFSET, reg);
                             break;
                    case 4 : FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_CLEAR_ERR_L_OFFSET, reg);
                             break;
                }
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function masks the specified interrupt(s) on the specified channel(s).
* Multiple DMA channels can be specified for the FDmaPs_channelNumber argument.
* Multiple interrupt types can be specified for the FDmaPs_irq argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           ch_irq Enumerated interrupt type.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_maskIrq(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_irq ch_irq)
{
    int x;
    int errorCode;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errorCode = FDmaPs_checkChannelRange(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Loop through and mask the selected channel Irq*/
        /* for the targeted channels.*/
        reg = ch_num & (DMAC_MAX_CH_MASK << DMAC_MAX_CHANNELS);
        for (x = 0; x < DMAC_MAX_INTERRUPTS; x++)
		{
            if (ch_irq & (1 << x))
			{
                switch (x)
				{
                    case 0 : FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_MASK_TFR_L_OFFSET, reg);
                             break;
                    case 1 : FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_MASK_BLOCK_L_OFFSET, reg);
                             break;
                    case 2 : FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_MASK_SRCTRAN_L_OFFSET, reg);
                             break;
                    case 3 : FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_MASK_DSTTRAN_L_OFFSET, reg);
                             break;
                    case 4 : FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_MASK_ERR_L_OFFSET, reg);
                             break;
                }
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function unmasks the specified interrupt(s) on the specified channel(s).
* Multiple DMA channels can be specified for the FDmaPs_channelNumber argument.
* Multiple interrupt types can be specified for the FDmaPs_irq argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           ch_irq Enumerated interrupt type.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_unmaskIrq(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_irq ch_irq)
{
    int x;
    int errorCode;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errorCode = FDmaPs_checkChannelRange(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Loop through and unmask the selected channel Irq*/
        /* for the targeted channels.*/
        reg = ch_num;
        for (x = 0; x < DMAC_MAX_INTERRUPTS; x++)
		{
            if (ch_irq & (1 << x))
			{
                switch (x)
				{
                    case 0 : FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_MASK_TFR_L_OFFSET, reg);
                             break;
                    case 1 : FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_MASK_BLOCK_L_OFFSET, reg);
                             break;
                    case 2 : FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_MASK_SRCTRAN_L_OFFSET, reg);
                             break;
                    case 3 : FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_MASK_DSTTRAN_L_OFFSET, reg);
                             break;
                    case 4 : FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_MASK_ERR_L_OFFSET, reg);
                             break;
                }
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns whether the specified interrupt on the
* specified channel is masked.
* Only 1 DMA channel can be specified for the FDmaPs_channelNumber argument.
* Only 1 interrupt type can be specified for the FDmaPs_irq argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           ch_irq Enumerated interrupt type.
*
* @return   
*           TRUE if the interrupt is masked.
*           FALSE if the interrupt is not masked.
*
* @note     NA.
*
*****************************************************************************/
BOOL FDmaPs_isIrqMasked(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_irq ch_irq)
{
    u32 reg;
    u8 ch_index;
    BOOL retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* Allow only ONE interrupt type to be specified*/
    FMSH_ASSERT(ch_irq == Dmac_irq_tfr      ||
            ch_irq == Dmac_irq_block   ||
            ch_irq == Dmac_irq_srctran ||
            ch_irq == Dmac_irq_dsttran ||
            ch_irq == Dmac_irq_err     );

    switch (ch_irq)
	{
        case Dmac_irq_tfr:
			reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_MASK_TFR_L_OFFSET);
            break;
        case Dmac_irq_block:
			reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_MASK_BLOCK_L_OFFSET);
            break;
        case Dmac_irq_srctran:
			reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_MASK_SRCTRAN_L_OFFSET);
            break;
        case Dmac_irq_dsttran:
			reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_MASK_DSTTRAN_L_OFFSET);
            break;
        case Dmac_irq_err:
			reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_MASK_ERR_L_OFFSET);
            break;
    }

	retval = (BOOL)((reg & DMAC_INT_MASK_L(ch_index)) >> ch_index);
    FMSH_ASSERT((retval == TRUE) || (retval == FALSE));

    /* Masked will read 0 : Unmasked will read 1*/
    if(retval == FALSE) {
        retval = TRUE;
    }
    else {
        retval = FALSE;
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function returns whether the specified raw interrupt on the
* specified channel is active.
* Only 1 DMA channel can be specified for the FDmaPs_channelNumber argument.
* Only 1 interrupt type can be specified for the FDmaPs_irq argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           ch_irq Enumerated interrupt type.
*
* @return   
*           TRUE if the interrupt is active.
*           FALSE if the interrupt is not active.
*
* @note     NA.
*
*****************************************************************************/
BOOL FDmaPs_isRawIrqActive(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_irq ch_irq)
{
    u32 reg;
    u8 ch_index;
    BOOL retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* Allow only ONE interrupt type to be specified*/
    FMSH_ASSERT(ch_irq == Dmac_irq_tfr      ||
            ch_irq == Dmac_irq_block   ||
            ch_irq == Dmac_irq_srctran ||
            ch_irq == Dmac_irq_dsttran ||
            ch_irq == Dmac_irq_err     );

    switch (ch_irq)
	{
        case Dmac_irq_tfr     : 
			reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_RAW_TFR_L_OFFSET);
			break;
        case Dmac_irq_block   : 
			reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_RAW_BLOCK_L_OFFSET);
            break;
        case Dmac_irq_srctran : 
			reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_RAW_SRCTRAN_L_OFFSET);
            break;
        case Dmac_irq_dsttran : 
			reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_RAW_DSTTRAN_L_OFFSET);
            break;
        case Dmac_irq_err     : 
			reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_RAW_ERR_L_OFFSET);
            break;
    }

	retval = (BOOL)((reg & DMAC_INT_RAW_STAT_CLR(ch_index)) >> ch_index);
    FMSH_ASSERT((retval == TRUE) || (retval == FALSE));
    return retval;
}

/*****************************************************************************
*
* @description
* This function returns whether the specified interrupt on the
* specified channel is active after masking.
* All DMA channels OR only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument.
* Only 1 interrupt type can be specified for the FDmaPs_irq argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           ch_irq Enumerated interrupt type.
*
* @return   
*           TRUE if the interrupt is active.
*           FALSE if the interrupt is not active.
*
* @note     NA.
*
*****************************************************************************/
BOOL FDmaPs_isIrqActive(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_irq ch_irq)
{
    u8 ch_index;
    BOOL retval;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Allow only ONE interrupt type to be specified*/
    FMSH_ASSERT(ch_irq == Dmac_irq_tfr      ||
            ch_irq == Dmac_irq_block   ||
            ch_irq == Dmac_irq_srctran ||
            ch_irq == Dmac_irq_dsttran ||
            ch_irq == Dmac_irq_err     );

    if (ch_num == Dmac_all_channels)
	{
		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_STATUS_INT_L_OFFSET);
        retval = (BOOL) reg & ch_irq;
    }
	else
	{
        ch_index = FDmaPs_getChannelIndex(ch_num);

        /* Allow only ONE channel to be specified*/
        FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

        /* Check that the specified channel is in range*/
        FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

        switch (ch_irq)
		{
	        case Dmac_irq_tfr     : 
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_STATUS_TFR_L_OFFSET);
				break;
	        case Dmac_irq_block   : 
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_STATUS_BLOCK_L_OFFSET);
	            break;
	        case Dmac_irq_srctran : 
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_STATUS_SRCTRAN_L_OFFSET);
	            break;
	        case Dmac_irq_dsttran : 
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_STATUS_DSTTRAN_L_OFFSET);
	            break;
	        case Dmac_irq_err     : 
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_STATUS_ERR_L_OFFSET);
	            break;
        }

		retval = (BOOL)((reg & DMAC_INT_RAW_STAT_CLR(ch_index)) >> ch_index);
    }
    FMSH_ASSERT((retval == TRUE) || (retval == FALSE));
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets configuration parameters in the DMAC's
* channel registers on the specified DMA channel.
* Only 1 DMA channel can be specified for the FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           ch Configuration structure handle
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setChannelConfig(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        FDmaPs_ChannelConfig_T *ch)
{
    int errorCode;
    u8 ch_index;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* check for out of range values*/
        if (ch->llp_lms > param->num_master_int-1)
		{
            errorCode = -FMSH_EINVAL;
        }
		
        if (ch->ctl_sms > param->num_master_int-1 ||
           ch->ctl_dms > param->num_master_int-1)
        {
            errorCode = -FMSH_EINVAL;
        }
		   
        if (param->ch_fc[ch_index] == DMAC_DMA_FC_ONLY &&
           (ch->ctl_tt_fc == Dmac_prf2mem_prf    ||
            ch->ctl_tt_fc == Dmac_prf2prf_srcprf ||
            ch->ctl_tt_fc == Dmac_mem2prf_prf    ||
            ch->ctl_tt_fc == Dmac_prf2prf_dstprf))
        {
            errorCode = -FMSH_EINVAL;
        }
			
        if (param->ch_fc[ch_index] == DMAC_SRC_FC_ONLY &&
           (ch->ctl_tt_fc != Dmac_prf2mem_prf &&
            ch->ctl_tt_fc != Dmac_prf2prf_srcprf))
        {
            errorCode = -FMSH_EINVAL;
        }
			
        if (param->ch_fc[ch_index] == DMAC_DST_FC_ONLY &&
           (ch->ctl_tt_fc != Dmac_mem2prf_prf &&
            ch->ctl_tt_fc != Dmac_prf2prf_dstprf))
        {
            errorCode = -FMSH_EINVAL;
        }
			
        if ((1 << (ch->ctl_src_msize + 1)) >
           param->ch_max_mult_size[ch_index] ||
           (1 << (ch->ctl_dst_msize + 1)) >
           param->ch_max_mult_size[ch_index])
        {
            errorCode = -FMSH_EINVAL;
        }
		   
        if (ch->cfg_dst_per >= param->num_hs_int ||
           ch->cfg_src_per >= param->num_hs_int)
        {
            errorCode = -FMSH_EINVAL;
        }
		   
        if (ch->cfg_ch_prior > param->num_channels - 1)
		{
            errorCode = -FMSH_EINVAL;
        }
    }

    if (errorCode == 0x0)
	{
        /* Set the control register*/
		reg = 
        (ch->ctl_int_en) |
        (ch->ctl_dst_tr_width << 1) |
        (ch->ctl_src_tr_width << 4) |
        (ch->ctl_dinc << 7) |
        (ch->ctl_sinc << 9) |
        (ch->ctl_dst_msize << 11) |
        (ch->ctl_src_msize << 14) |
        (ch->ctl_src_gather_en << 17) |
        (ch->ctl_dst_scatter_en << 18)|
        (ch->ctl_tt_fc << 20) |
        (ch->ctl_dms << 23) |
        (ch->ctl_sms << 25) |
        (ch->ctl_llp_dst_en << 27) |
        (ch->ctl_llp_src_en << 28);
		FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(ch_index), reg);

        reg = ch->ctl_block_ts | (ch->ctl_done << 12);
		FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_H_OFFSET(ch_index), reg);

        /* Set the config register*/
        reg =
        (ch->cfg_ch_prior << 5) |
        (ch->cfg_hs_sel_dst << 10) |
        (ch->cfg_hs_sel_src << 11) |
        (ch->cfg_lock_ch_l << 12) |
        (ch->cfg_lock_b_l << 14) |
        (ch->cfg_lock_ch << 16) |
        (ch->cfg_lock_b << 17) |
        (ch->cfg_dst_hs_pol << 18) |
        (ch->cfg_src_hs_pol << 19) |
        (ch->cfg_max_abrst << 20) |
        (ch->cfg_reload_src << 30) |
        (ch->cfg_reload_dst << 31);
		FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(ch_index), reg);

        reg =
        (ch->cfg_fcmode) |
        (ch->cfg_fifo_mode << 1) |
        (ch->cfg_protctl << 2) |
        (ch->cfg_ds_upd_en << 5) |
        (ch->cfg_ss_upd_en << 6) |
        (ch->cfg_src_per << 7) |
        (ch->cfg_dst_per << 11);
		FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_H_OFFSET(ch_index), reg);
      
        /* set the SAR/DAR registers*/
		FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_SAR_L_OFFSET(ch_index), ch->sar);
		FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_DAR_L_OFFSET(ch_index), ch->dar);

        /* set the LLP register*/
        if (param->ch_hc_llp[ch_index] == 0x0)
		{
            reg = (ch->llp_lms & 0x3) | (ch->llp_loc << 2);
			FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_LLP_L_OFFSET(ch_index), reg);
        }

        /* set the SGR register*/
        if (param->ch_src_gat_en[ch_index] == 0x1)
		{
            reg = (ch->sgr_sgi & 0xFFFFF) | (ch->sgr_sgc << 20);
			FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_SGR_L_OFFSET(ch_index), reg);
        }

        /* set the DSR register*/
        if (param->ch_dst_sca_en[ch_index] == 0x1)
		{
			reg = (ch->dsr_dsi & 0xFFFFF)|(ch->dsr_dsc << 20);
			FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_DSR_L_OFFSET(ch_index), reg);
        }

        /* set the SSTAT/DSTAT SSTATAR/DSTATAR registers*/
        if (param->ch_stat_src[ch_index] == 0x1)
		{
			FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_SSTAT_L_OFFSET(ch_index), ch->sstat);
			FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_SSTATAR_L_OFFSET(ch_index), ch->sstatar);
        }
		
        if (param->ch_stat_dst[ch_index] == 0x1)
		{
			FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_DSTAT_L_OFFSET(ch_index), ch->dstat);
			FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_DSTATAR_L_OFFSET(ch_index), ch->dstatar);
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function gets configuration parameters in the DMAC's
* channel registers for the specified DMA channel.
* Only 1 DMA channel can be specified for the FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           ch Configuration structure handle
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_getChannelConfig(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        FDmaPs_ChannelConfig_T *ch)
{
    int errorCode;
    u8 ch_index;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    errorCode = 0;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Check for valid channel number - can only specify one channel */
    if (DMAC_CH_MASK & ch_num || ch_num == Dmac_no_channel ||
       ch_index == DMAC_MAX_CHANNELS)
    {
        errorCode = -FMSH_ECHRNG;
    }

    if (errorCode == 0)
	{
        /* read in the control register*/
		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(ch_index));
		
        ch->ctl_llp_src_en = (enum FMSH_state)((reg & DMAC_CTL_L_LLP_SRC_EN) >> 28);
        ch->ctl_llp_dst_en = (enum FMSH_state)((reg & DMAC_CTL_L_LLP_DST_EN) >> 27);
		
        ch->ctl_sms = (enum FDmaPs_masterNumber)((reg & DMAC_CTL_L_SMS) >> 25);
        ch->ctl_dms = (enum FDmaPs_masterNumber)((reg & DMAC_CTL_L_DMS) >> 23);
		
        ch->ctl_src_msize = (enum FDmaPs_burstTransLength)((reg & DMAC_CTL_L_SRC_MSIZE) >> 14);
        ch->ctl_dst_msize = (enum FDmaPs_burstTransLength)((reg & DMAC_CTL_L_DEST_MSIZE) >> 11);
		
        ch->ctl_sinc = (enum FDmaPs_addressIncrement)((reg & DMAC_CTL_L_SINC) >> 9);
        ch->ctl_dinc = (enum FDmaPs_addressIncrement)((reg & DMAC_CTL_L_DINC) >> 7);
		
        ch->ctl_src_tr_width = (enum FDmaPs_transferWidth)((reg & DMAC_CTL_L_SRC_TR_WIDTH) >> 4);
        ch->ctl_dst_tr_width = (enum FDmaPs_transferWidth)((reg & DMAC_CTL_L_DST_TR_WIDTH) >> 1);
		
        ch->ctl_tt_fc = (enum FDmaPs_transferFlow)((reg & DMAC_CTL_L_TT_FC) >> 20);
        ch->ctl_dst_scatter_en = (enum FMSH_state)((reg & DMAC_CTL_L_DST_SCATTER_EN) >> 18);
        ch->ctl_src_gather_en = (enum FMSH_state)((reg & DMAC_CTL_L_SRC_GATHER_EN) >> 17);
		
        ch->ctl_int_en = (enum FMSH_state)(reg & DMAC_CTL_L_INT_EN);

		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_H_OFFSET(ch_index));
        ch->ctl_done = (enum FMSH_state)((reg & DMAC_CTL_H_DONE) >> 12);
        ch->ctl_block_ts = reg & DMAC_CTL_H_BLOCK_TS;

        /* read in the config register*/
		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(ch_index));
        ch->cfg_ch_prior = (enum FDmaPs_channelPriority)((reg & DMAC_CFG_L_CH_PRIOR) >> 5);
        ch->cfg_lock_ch_l = (enum FDmaPs_lockLevel)((reg & DMAC_CFG_L_LOCK_CH_L) >> 12);
        ch->cfg_lock_b_l = (enum FDmaPs_lockLevel)((reg & DMAC_CFG_L_LOCK_B_L) >> 14);
        ch->cfg_lock_ch = (enum FMSH_state)((reg & DMAC_CFG_L_LOCK_CH) >> 16);
        ch->cfg_lock_b = (enum FMSH_state)((reg & DMAC_CFG_L_LOCK_B) >> 17);
		
		ch->cfg_max_abrst = (reg & DMAC_CFG_L_MAX_ABRST) >> 20;
        
        ch->cfg_hs_sel_dst = (enum FDmaPs_swHwHsSelect)((reg & DMAC_CFG_L_HS_SEL_DST) >> 10);
        ch->cfg_hs_sel_src = (enum FDmaPs_swHwHsSelect)((reg & DMAC_CFG_L_HS_SEL_SRC) >> 11);
		
        ch->cfg_dst_hs_pol = (enum FDmaPs_polarityLevel)((reg & DMAC_CFG_L_DST_HS_POL) >> 18);
        ch->cfg_src_hs_pol = (enum FDmaPs_polarityLevel)((reg & DMAC_CFG_L_SRC_HS_POL) >> 19);
		
        ch->cfg_reload_dst = (enum FMSH_state)((reg & DMAC_CFG_L_RELOAD_DST) >> 31);
        ch->cfg_reload_src = (enum FMSH_state)((reg & DMAC_CFG_L_RELOAD_SRC) >> 30);

		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_H_OFFSET(ch_index));
        ch->cfg_ds_upd_en = (enum FMSH_state)((reg & DMAC_CFG_H_DS_UPD_EN) >> 5);
        ch->cfg_ss_upd_en = (enum FMSH_state)((reg & DMAC_CFG_H_SS_UPD_EN) >> 6);
		
        ch->cfg_src_per = (enum FDmaPs_hsInterface)((reg & DMAC_CFG_H_SRC_PER) >> 7);
        ch->cfg_dst_per = (enum FDmaPs_hsInterface)((reg & DMAC_CFG_H_DEST_PER) >> 11);
		
        ch->cfg_fcmode = (enum FDmaPs_flowCtlMode)(reg & DMAC_CFG_H_FCMODE);
        ch->cfg_fifo_mode = (enum FDmaPs_fifoMode)((reg & DMAC_CFG_H_FIFO_MODE) >> 1);
        ch->cfg_protctl = (enum FDmaPs_protLevel)((reg & DMAC_CFG_H_PROTCTL) >> 2);

        /* read the SAR/DAR registers*/
        ch->sar = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_SAR_L_OFFSET(ch_index));
        ch->dar = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_DAR_L_OFFSET(ch_index));

        /* read the LLP register*/
        if (param->ch_hc_llp[ch_index] == 0x0)
		{
			reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_LLP_L_OFFSET(ch_index));
            ch->llp_lms = (enum FDmaPs_masterNumber)(reg & DMAC_LLP_L_LMS);
            ch->llp_loc = (reg & DMAC_LLP_L_LOC) >> 2;
        }
		else
		{
            ch->llp_lms = (enum FDmaPs_masterNumber) 0x0;
            ch->llp_loc = 0x0;
        }

        /* read the SSTAT/DSTAT SSTATAR/DSTATAR registers*/
        if (param->ch_stat_src[ch_index] == 0x1)
		{
            ch->sstat = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_SSTAT_L_OFFSET(ch_index));
            ch->sstatar = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_SSTATAR_L_OFFSET(ch_index));
        }
		else
		{
            ch->sstat = 0x0;
            ch->sstatar = 0x0;
        }

        if (param->ch_stat_dst[ch_index] == 0x1)
		{
            ch->dstat = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_DSTAT_L_OFFSET(ch_index));
            ch->dstatar = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_DSTATAR_L_OFFSET(ch_index));
        }
		else
		{
            ch->dstat = 0x0;
            ch->dstatar = 0x0;
        }

        /* read the SGR register*/
        if (param->ch_src_gat_en[ch_index] == 0x1)
		{
            reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_SGR_L_OFFSET(ch_index));
            ch->sgr_sgc = (reg & DMAC_SGR_L_SGC) >> 20;
            ch->sgr_sgi = reg & DMAC_SGR_L_SGI;
        }
		else
		{
            ch->sgr_sgc = 0x0;
            ch->sgr_sgi = 0x0;
        }

        /* read the DSR register*/
        if (param->ch_dst_sca_en[ch_index] == 0x1)
		{
            reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_DSR_L_OFFSET(ch_index));
            ch->dsr_dsc = (reg & DMAC_DSR_L_DSC) >> 20;
            ch->dsr_dsi = reg & DMAC_DSR_L_DSI;
        }
		else
		{
            ch->dsr_dsc = 0x0;
            ch->dsr_dsi = 0x0;
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function sets up the specified DMA channel(s) for the specified
* transfer type. The FDmaPs_transferType enumerated type describes
* all of the transfer types supported by the DMA controller.
* Multiple DMA channels can be specified for the FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           trans_type Enumerated DMA transfer type.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setTransferType(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_transferType  transfer)
{
    int errorCode;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    if (errorCode == 0)
	{
        switch (transfer)
		{
            case Dmac_transfer_row1 :
                errorCode = FDmaPs_setListPointerAddress(pDmac, ch_num, 0x0);
                if (errorCode == 0)
				{
                    errorCode = FDmaPs_setLlpEnable(pDmac, ch_num, Dmac_src_dst, FMSH_clear);
                }
                if (errorCode == 0)
				{
                    errorCode = FDmaPs_setReload(pDmac, ch_num, Dmac_src_dst, FMSH_clear);
                }
                break;
            case  Dmac_transfer_row2 :
                errorCode = FDmaPs_setListPointerAddress(pDmac, ch_num, 0x0);
                if (errorCode == 0)
				{
                    errorCode = FDmaPs_setLlpEnable(pDmac, ch_num, Dmac_src_dst, FMSH_clear);
                }
                if (errorCode == 0)
				{
                    errorCode = FDmaPs_setReload(pDmac, ch_num, Dmac_src, FMSH_clear);
                }
                if (errorCode == 0)
				{
                    errorCode = FDmaPs_setReload(pDmac, ch_num, Dmac_dst, FMSH_set);
                }
                break;
            case  Dmac_transfer_row3 :
                errorCode = FDmaPs_setListPointerAddress(pDmac, ch_num, 0x0);
                if (errorCode == 0)
				{
                    errorCode = FDmaPs_setLlpEnable(pDmac, ch_num, Dmac_src_dst, FMSH_clear);
                }
                if (errorCode == 0)
				{
                    errorCode = FDmaPs_setReload(pDmac, ch_num, Dmac_src, FMSH_set);
                }
                if (errorCode == 0)
				{
                    errorCode = FDmaPs_setReload(pDmac, ch_num, Dmac_dst, FMSH_clear);
                }
                break;
            case  Dmac_transfer_row4 :
                errorCode = FDmaPs_setListPointerAddress(pDmac, ch_num, 0x0);
                if (errorCode == 0)
				{
                    errorCode = FDmaPs_setLlpEnable(pDmac, ch_num, Dmac_src_dst, FMSH_clear);
                }
                if (errorCode == 0)
				{
                    errorCode = FDmaPs_setReload(pDmac, ch_num, Dmac_src_dst, FMSH_set);
                }
                break;
            case  Dmac_transfer_row5 :
                errorCode = FDmaPs_setLlpEnable(pDmac,ch_num, Dmac_src_dst,FMSH_clear);
                if (errorCode == 0)
				{
                    errorCode = FDmaPs_setReload(pDmac,ch_num, Dmac_src_dst,FMSH_clear);
                }
                break;
            case  Dmac_transfer_row6 :
                errorCode = FDmaPs_setLlpEnable(pDmac,ch_num, Dmac_src,FMSH_clear);
                if (errorCode == 0)
				{
                	errorCode = FDmaPs_setLlpEnable(pDmac,ch_num, Dmac_dst,FMSH_set);
                }
                if (errorCode == 0)
				{
                    errorCode = FDmaPs_setReload(pDmac,ch_num, Dmac_src_dst,FMSH_clear);
                }
                break;
            case  Dmac_transfer_row7 :
                errorCode = FDmaPs_setLlpEnable(pDmac,ch_num, Dmac_src,FMSH_clear);
                if (errorCode == 0)
				{
                    errorCode = FDmaPs_setLlpEnable(pDmac,ch_num, Dmac_dst,FMSH_set);
                }
                if (errorCode == 0)
				{
                    errorCode = FDmaPs_setReload(pDmac,ch_num, Dmac_src,FMSH_set);
                }
                if (errorCode == 0)
				{
                    errorCode = FDmaPs_setReload(pDmac,ch_num, Dmac_dst,FMSH_clear);
                }
                break;
            case  Dmac_transfer_row8 :
                errorCode = FDmaPs_setLlpEnable(pDmac,ch_num, Dmac_src,FMSH_set);
                if(errorCode == 0) {
                    errorCode = FDmaPs_setLlpEnable(pDmac,ch_num, Dmac_dst,FMSH_clear);
                }
                if(errorCode == 0) {
                    errorCode = FDmaPs_setReload(pDmac,ch_num, Dmac_src_dst,FMSH_clear);
                }
                break;
            case  Dmac_transfer_row9 :
                errorCode = FDmaPs_setLlpEnable(pDmac,ch_num, Dmac_src,FMSH_set);
                if (errorCode == 0)
				{
                    errorCode = FDmaPs_setLlpEnable(pDmac,ch_num, Dmac_dst,FMSH_clear);
                }
                if (errorCode == 0)
				{
                    errorCode = FDmaPs_setReload(pDmac,ch_num, Dmac_src,FMSH_clear);
                }
                if (errorCode == 0)
				{
                    errorCode = FDmaPs_setReload(pDmac,ch_num, Dmac_dst,FMSH_set);
                }
                break;
            case  Dmac_transfer_row10 :
                errorCode = FDmaPs_setLlpEnable(pDmac,ch_num, Dmac_src_dst,FMSH_set);
                if (errorCode == 0)
				{
                    errorCode = FDmaPs_setReload(pDmac,ch_num, Dmac_src_dst,FMSH_clear);
                }
                break;
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the DMA transfer type for the specified DMA channel.
* The FDmaPs_transferType enumerated type describes all of the transfer
* types supported by the DMA controller.
* Only 1 DMA channel can be specified for the FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           Enumerated DMA transfer type.
*
* @note     NA.
*
*****************************************************************************/
enum FDmaPs_transferType FDmaPs_getTransferType(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    u8 ch_index;
    u32 llp_reg, ctl_reg, cfg_reg;
    u8 row;
    enum FDmaPs_transferType retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    row     = 0x0;
    llp_reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_LLP_L_OFFSET(ch_index));
    ctl_reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(ch_index));
    cfg_reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(ch_index));

	if (cfg_reg & DMAC_CFG_L_RELOAD_DST)
	{
        row |= 0x01;
    }
    if (ctl_reg & DMAC_CTL_L_LLP_DST_EN)
	{
        row |= 0x02;
    }
    if (cfg_reg & DMAC_CFG_L_RELOAD_SRC)
	{
        row |= 0x04;
    }
    if (ctl_reg & DMAC_CTL_L_LLP_SRC_EN)
	{
        row |= 0x08;
    }
    if (llp_reg & DMAC_LLP_L_LOC)
	{
        row |= 0x10;
    }

    switch (row)
	{
        case 0x00 : retval = Dmac_transfer_row1 ;  break;
        case 0x01 : retval = Dmac_transfer_row2 ;  break;
        case 0x04 : retval = Dmac_transfer_row3 ;  break;
        case 0x05 : retval = Dmac_transfer_row4 ;  break;
        case 0x10 : retval = Dmac_transfer_row5 ;  break;
        case 0x12 : retval = Dmac_transfer_row6 ;  break;
        case 0x16 : retval = Dmac_transfer_row7 ;  break;
        case 0x18 : retval = Dmac_transfer_row8 ;  break;
        case 0x19 : retval = Dmac_transfer_row9 ;  break;
        case 0x1a : retval = Dmac_transfer_row10;  break;
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function returns whether the block transfer of the selected
* channel has completed.
* Only 1 DMA channel can be specified for the FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           TRUE if the block transfer is done.
*           FALSE if the block transfer is not done.
*
* @note     NA.
*
*****************************************************************************/
BOOL FDmaPs_isBlockTransDone(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    u8 ch_index;
    BOOL retval;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* For Linked List transfers the memory location of the linked*/
    /* list item has to be read : LLI.done*/
    if ((FDmaPs_getTransferType(pDmac, ch_num) == Dmac_transfer_row5) ||
        (FDmaPs_getTransferType(pDmac, ch_num) == Dmac_transfer_row6) ||
        (FDmaPs_getTransferType(pDmac, ch_num) == Dmac_transfer_row7) ||
        (FDmaPs_getTransferType(pDmac, ch_num) == Dmac_transfer_row8) ||
        (FDmaPs_getTransferType(pDmac, ch_num) == Dmac_transfer_row9) ||
        (FDmaPs_getTransferType(pDmac, ch_num) == Dmac_transfer_row10))
    {
        FMSH_ASSERT(FALSE);
    }

    /* For Non Linked list transfers we read the memory mapped DMA*/
    /* register*/
    else
	{
		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_H_OFFSET(ch_index));
		retval = (BOOL)((reg & DMAC_CTL_H_DONE) >> 12);
        FMSH_ASSERT((retval == TRUE) || (retval == FALSE));
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function returns whether the FIFO is empty on the specified channel.
* Only 1 DMA channel can be specified for the FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           TRUE if channel FIFO is empty.
*           FALSE if channel FIFO is not empty.
*
* @note     NA.
*
*****************************************************************************/
BOOL FDmaPs_isFifoEmpty(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    u8 ch_index;
    BOOL retval;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

	reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(ch_index));
	retval = (BOOL)((reg & DMAC_CFG_L_FIFO_EMPTY) >> 9);
    FMSH_ASSERT((retval == TRUE) || (retval == FALSE));
    return retval;
}

/*****************************************************************************
*
* @description
* This function enables/disables test mode in the DMAC.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           state Enumerated Enabled/Disabled state.
*
* @return   
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FDmaPs_setTestMode(      FDmaPs_T *pDmac, enum FMSH_state state)
{
    DMAC_COMMON_REQUIREMENTS(pDmac);
	
	FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_DMA_TEST_REG_L_OFFSET, state);
}

/*****************************************************************************
*
* @description
* This function returns whether test mode is enabled or disabled
* in the DMA controller.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*
* @return   
*           Enumerated Enabled/Disabled state.
*
* @note     NA.
*
*****************************************************************************/
enum FMSH_state FDmaPs_getTestMode(
        FDmaPs_T *pDmac)
{
    u32 reg;
    enum FMSH_state retval;

    DMAC_COMMON_REQUIREMENTS(pDmac);

	reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_DMA_TEST_REG_L_OFFSET);
	retval = (enum FMSH_state)(reg & DMAC_DMATESTREG_L_TEST_SLV_IF);
    return retval;
}

/*****************************************************************************
*
* @description
* This function is used to activate/de-activate the source and 
* destination software request registers.
* Three registers exist for software requests on the source and destination. 
* These are: Request, Single Request, Last Request. 
* Use the FDmaPs_softwareReq enum to select which of the three
* registers is accessed.
* Use the FDmaPs_srcDstSelect enum to select either the source or
* destination register.
* Multiple DMA channels can be specified for the FDmaPs_channelNumber argument.
* Both source and destination can be specified for 
* the FDmaPs_srcDstSelect argument. 
* Only 1 request register can be specified for the FDmaPs_softwareReq argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*           sw_req Enumerated request register select.
*           state Enumerated enabled/disabled state.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setSoftwareRequest(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel,
        enum FDmaPs_softwareReq   sw_req,
        enum FMSH_state state)
{
    int x, errorCode;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    errorCode = 0;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errorCode = FDmaPs_checkChannelRange(pDmac, ch_num);
    
    /* channel must be enabled for write to happen !!*/
    if (errorCode == 0)
	{
		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CH_EN_REG_L_OFFSET);
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                if (!(reg & (1 << x)))
				{
                    errorCode = -FMSH_EPERM;
                    break;
                }
            }
        }
    }
    /*if(errorCode == 0) {*/
        /* Check for valid channel number*/
    /*    errorCode = FDmaPs_checkChannelRange(pDmac, ch_num);*/
    /*}*/
    if (errorCode == 0)
	{
        if (state)
		{
            reg = ch_num;
        }
		else
		{
            reg = ch_num & (DMAC_MAX_CH_MASK << DMAC_MAX_CHANNELS);
        }

        if (sd_sel == Dmac_src || sd_sel == Dmac_src_dst)
		{
            if (sw_req == Dmac_request)
			{
				FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_SWHS_REQ_SRC_REG_L_OFFSET, reg);
            }
			else if (sw_req == Dmac_single_request)
			{
				FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_SWHS_SGL_REQ_SRC_REG_L_OFFSET, reg);
            } 
			else if (sw_req == Dmac_last_request)
			{
				FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_SWHS_LST_SRC_REG_L_OFFSET, reg);
            }
			else
			{
                errorCode = -FMSH_EINVAL;
            }
        }
        if (sd_sel == Dmac_dst || sd_sel == Dmac_src_dst)
		{
            if (sw_req == Dmac_request)
			{
				FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_SWHS_REQ_DST_REG_L_OFFSET, reg);
            }
			else if (sw_req == Dmac_single_request)
			{
				FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_SWHS_SGL_REQ_DST_REG_L_OFFSET, reg);
            }
			else if (sw_req == Dmac_last_request)
			{
				FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_SWHS_LST_DST_REG_L_OFFSET, reg);
            }
			else
			{
                errorCode = -FMSH_EINVAL;
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function is used to return the activate/in-activate status 
* of the source and destination software request registers.
* Three registers exist for software requests on the source and destination. 
* These are: Request, Single Request, Last Request. 
* Use the FDmaPs_softwareReq enum to select which of the three
* registers is accessed.
* Use the FDmaPs_srcDstSelect enum to select either the source or
* destination register.
* Only 1 DMA channel can be specified for the FDmaPs_channelNumber argument.
* Only 1, source or destination, can be specified for the 
* FDmaPs_srcDstSelect argument. 
* Only 1 request register can be specified for the FDmaPs_softwareReq argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*           sw_req Enumerated request register select.
*
* @return   
*           Enumerated enabled/disabled state.
*
* @note     NA.
*
*****************************************************************************/
enum FMSH_state FDmaPs_getSoftwareRequest(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel,
        enum FDmaPs_softwareReq   sw_req)
{
    u8 ch_index;
    u32 reg;
    enum FMSH_state retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* Allow only source OR destination*/
    FMSH_ASSERT(sd_sel == Dmac_src || sd_sel == Dmac_dst);

    if (sd_sel == Dmac_src)
	{
        if (sw_req == Dmac_request)
		{
			reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_SWHS_REQ_SRC_REG_L_OFFSET);
        } else if (sw_req == Dmac_single_request)
		{
			reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_SWHS_SGL_REQ_SRC_REG_L_OFFSET);
        } else if (sw_req == Dmac_last_request)
		{
			reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_SWHS_LST_SRC_REG_L_OFFSET);
        }
    }
	else
	{
        if (sw_req == Dmac_request)
		{
			reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_SWHS_REQ_DST_REG_L_OFFSET);
        }
		else if (sw_req == Dmac_single_request)
		{
			reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_SWHS_SGL_REQ_DST_REG_L_OFFSET);
        }
		else if (sw_req == Dmac_last_request)
		{
			reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_SWHS_LST_DST_REG_L_OFFSET);
        }
    }
	retval = (enum FMSH_state)((reg & DMAC_SW_HANDSHAKE_L(ch_index)) >> ch_index);
    return retval; 
}

/*****************************************************************************
*
* @description
* This function sets the address on the specified source or/and
* destination register of the specified channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument. Both source and destination can
* be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*           address 32-bit address value.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setAddress(
        FDmaPs_T *pDmac,
        enum   FDmaPs_channelNumber ch_num,
        enum   FDmaPs_srcDstSelect sd_sel,
        u32 address)
{
    int errorCode;
    int x;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                if (sd_sel == Dmac_src || sd_sel == Dmac_src_dst)
				{
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_SAR_L_OFFSET(x), address);
                }
                if (sd_sel == Dmac_dst || sd_sel == Dmac_src_dst)
				{
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_DAR_L_OFFSET(x), address);
                }
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the address on the specified source or
*  destination register of the specified channel.
*  Only 1 DMA channel can be specified for the
*  FDmaPs_channelNumber argument. Only 1, source or destination,
*  can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*
* @return   
*           32-bit contents of source/destination address register of the
*           specified DMA channel.
*
* @note     NA.
*
*****************************************************************************/
u32 FDmaPs_getAddress(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel)
{
    u8 ch_index;
    u32 retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* Allow only source OR destination*/
    FMSH_ASSERT(sd_sel == Dmac_src || sd_sel == Dmac_dst);

    if (sd_sel == Dmac_src)
	{
		retval = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_SAR_L_OFFSET(ch_index));
    }
	else
	{
		retval = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_DAR_L_OFFSET(ch_index));
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the block size of a transfer on the specified channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           block_size Size of the transfers block.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setBlockTransSize(
        FDmaPs_T *pDmac,
        enum   FDmaPs_channelNumber ch_num,
        u16 block_size)
{
    int errorCode;
    int x;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_H_OFFSET(x));
				if ((reg & DMAC_CTL_H_BLOCK_TS) != block_size)
				{
					reg = (reg & ~DMAC_CTL_H_BLOCK_TS) | block_size;
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_H_OFFSET(x), reg);
				}
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the block size of a transfer on the specified channel.
* Only ONE DMA channel can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           16-bit value of the transfer block size.
*
* @note     NA.
*
*****************************************************************************/
u16 FDmaPs_getBlockTransSize(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    u8 ch_index;
    u16 retval;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

	reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_H_OFFSET(ch_index));
	retval = reg & DMAC_CTL_H_BLOCK_TS;
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the specified source and/or destination master
* select interface on the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* the FDmaPs_channelNumber argument. Both source and destination
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*           mst_num Enumerated master interface number.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setMstSelect(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel,
        enum FDmaPs_masterNumber  mst_num)
{
    int errorCode;
    int x;
    BOOL write_en;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Check is the specified master interface is in range*/
        if (mst_num >= param->num_master_int)
		{
            errorCode = -FMSH_EINVAL;
        }
		else
		{
            /* Check if the field is hard-coded*/
            for (x = 0; x < param->num_channels; x++)
			{
                if (ch_num & (1 << x))
				{
                    if (sd_sel == Dmac_src || Dmac_src_dst)
					{
                        if (param->ch_sms[x] != 0x4)
						{
                            errorCode = -FMSH_ENOSYS;
                            break;
                        }
                    }
                    if (sd_sel == Dmac_dst || Dmac_src_dst)
					{
                        if (param->ch_dms[x] != 0x4)
						{
                            errorCode = -FMSH_ENOSYS;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                write_en = FALSE;
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(x));
                if (sd_sel == Dmac_src || sd_sel == Dmac_src_dst)
				{
					if (((reg & DMAC_CTL_L_SMS) >> 25) != mst_num)
					{
						reg = (reg & ~DMAC_CTL_L_SMS) | (mst_num << 25);
						write_en = TRUE;
					}
                }
                if (sd_sel == Dmac_dst || sd_sel == Dmac_src_dst)
				{
					if (((reg & DMAC_CTL_L_DMS) >> 23) != mst_num)
					{
						reg = (reg & ~DMAC_CTL_L_DMS) | (mst_num << 23);
						write_en = TRUE;
					}
                }
                if (write_en)
				{
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(x), reg);
                }
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the specified source or destination master
* select interface on the specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument. Only 1, source or destination,
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*
* @return   
*           Enumerated master interface number.
*
* @note     NA.
*
*****************************************************************************/
enum FDmaPs_masterNumber FDmaPs_getMstSelect(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel)
{
    u8 ch_index;
    u32 reg;
    enum FDmaPs_masterNumber retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* Allow only source OR destination*/
    FMSH_ASSERT(sd_sel == Dmac_src || sd_sel == Dmac_dst);

	reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(ch_index));
    if (sd_sel == Dmac_src)
	{
		retval = (enum FDmaPs_masterNumber)((reg & DMAC_CTL_L_SMS) >> 25);
    }
	else
	{
		retval = (enum FDmaPs_masterNumber)((reg & DMAC_CTL_L_DMS) >> 23);
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the transfer device type and flow control
* (TT_FC) for the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           tt_fc Enumerated transfer device type and flow control.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setMemPeriphFlowCtl(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_transferFlow  tt_fc)
{
    int x, errorCode;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }
    
    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Check for hard-coded values*/
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                if (param->ch_fc[x] == DMAC_DMA_FC_ONLY &&
                   (tt_fc == Dmac_prf2mem_prf    ||
                    tt_fc == Dmac_prf2prf_srcprf ||
                    tt_fc == Dmac_mem2prf_prf    ||
                    tt_fc == Dmac_prf2prf_dstprf))
                {
                    errorCode = -FMSH_ENOSYS;
                    break;
                }
                if (param->ch_fc[x] == DMAC_SRC_FC_ONLY &&
                   (tt_fc != Dmac_prf2mem_prf &&
                    tt_fc != Dmac_prf2prf_srcprf))
                {
                    errorCode = -FMSH_ENOSYS;
                    break;
                }
                if (param->ch_fc[x] == DMAC_DST_FC_ONLY &&
                   (tt_fc != Dmac_mem2prf_prf &&
                    tt_fc != Dmac_prf2prf_dstprf))
                {
                    errorCode = -FMSH_ENOSYS;
                    break;
                }
            }
        }
    }
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(x));
				if (((reg & DMAC_CTL_L_TT_FC) >> 20) != tt_fc)
				{
					reg = (reg & ~DMAC_CTL_L_TT_FC) | (tt_fc << 20);
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(x), reg);
				}
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns whether scatter mode is enabled or disabled
* on the specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           Enumerated Enabled/Disabled state.
*
* @note     NA.
*
*****************************************************************************/
enum FDmaPs_transferFlow FDmaPs_getMemPeriphFlowCtl(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    u8 ch_index;
    u32 reg;
    enum FDmaPs_transferFlow retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);
    
    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

	reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(ch_index));
	retval = (enum FDmaPs_transferFlow)((reg & DMAC_CTL_L_TT_FC) >> 20);
    return retval;
}

/*****************************************************************************
*
* @description
* This function enables or disables the destination scatter mode
* on the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           state Enumerated Enable/Disable state.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setScatterEnable(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FMSH_state state)
{
    int errorCode;
    int x;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Check for hard-coded values*/
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                if (param->ch_dst_sca_en[x] == 0x0)
				{
                    errorCode = -FMSH_ENOSYS;
                    break;
                }
            }
        }
    }
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(x));
				if (((reg & DMAC_CTL_L_DST_SCATTER_EN) >> 18) != state)
				{
					reg = (reg & ~DMAC_CTL_L_DST_SCATTER_EN) | (state << 18);
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(x), reg);
				}
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns whether scatter mode is enabled or disabled
* on the specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           Enumerated Enabled/Disabled state.
*
* @note     NA.
*
*****************************************************************************/
enum FMSH_state FDmaPs_getScatterEnable(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    u8 ch_index;
    u32 reg;
    enum FMSH_state retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

	reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(ch_index));
	retval = (enum FMSH_state)((reg & DMAC_CTL_L_DST_SCATTER_EN) >> 18);
    return retval;
}

/*****************************************************************************
*
* @description
* This function enables or disables the source gather mode on the
* specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           state Enumerated Enable/Disable state.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setGatherEnable(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FMSH_state state)
{
    int errorCode;
    int x;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Check for hard-coded values*/
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                if (param->ch_src_gat_en[x] == 0x0)
				{
                    errorCode = -FMSH_ENOSYS;
                    break;
                }
            }
        }
        
    }
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(x));
				if (((reg & DMAC_CTL_L_SRC_GATHER_EN) >> 17) != state)
				{
					reg = (reg & ~DMAC_CTL_L_SRC_GATHER_EN) | (state << 17);
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(x), reg);
				}
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns whether gather mode is enabled or disabled
* on the specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           Enumerated Enabled/Disabled state.
*
* @note     NA.
*
*****************************************************************************/
enum FMSH_state FDmaPs_getGatherEnable(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    u8 ch_index;
    u32 reg;
    enum FMSH_state retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

	reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(ch_index));
	retval = (enum FMSH_state)((reg & DMAC_CTL_L_SRC_GATHER_EN) >> 17);
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the specified source and/or destination
* burst size on the specified DMA channel(s). 
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument. Both source and destination
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*           length Enumerated burst size.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setBurstTransLength(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel,
        enum FDmaPs_burstTransLength length)
{
    int errorCode;
    int x;
    BOOL write_en;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Check if the specified value is in range*/
        for (x = 0; x < param->num_channels; x++)
        {
            if (ch_num & (1 << x))
			{
                if ((1 << (length + 1)) > param->ch_max_mult_size[x])
				{
                    errorCode = -FMSH_EINVAL;
                    break;
                }
            }
        }
    }
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                write_en = FALSE;
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(x));
                if (sd_sel == Dmac_src || sd_sel == Dmac_src_dst)
				{
					if (((reg & DMAC_CTL_L_SRC_MSIZE) >> 14) != length)
					{
						reg = (reg & ~DMAC_CTL_L_SRC_MSIZE) | (length << 14);
						write_en = TRUE;
					}
                }
                if (sd_sel == Dmac_dst || sd_sel == Dmac_src_dst)
				{
					if(((reg & DMAC_CTL_L_DEST_MSIZE) >> 11) != length)
					{
						reg = (reg & ~DMAC_CTL_L_DEST_MSIZE) | (length << 11);
						write_en = TRUE;
					}
                }
                if (write_en)
				{
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(x), reg);
                }
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the specified source or destination
* burst size on the specified DMA channel. 
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument. Only 1, source or destination,
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*
* @return   
*           Enumerated burst size.
*
* @note     NA.
*
*****************************************************************************/
enum FDmaPs_burstTransLength FDmaPs_getBurstTransLength(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel)
{
    u8 ch_index;
    u32 reg;
    enum FDmaPs_burstTransLength retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* Allow only source OR destination*/
    FMSH_ASSERT(sd_sel == Dmac_src || sd_sel == Dmac_dst);

    reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(ch_index));
    if (sd_sel == Dmac_src)
	{
        retval = (enum FDmaPs_burstTransLength)((reg & DMAC_CTL_L_SRC_MSIZE) >> 14);
    }
	else
	{
        retval = (enum FDmaPs_burstTransLength)((reg & DMAC_CTL_L_DEST_MSIZE) >> 11);
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the address increment type on the specified
* source and/or destination on the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument. Both source and destination
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*           addr_inc Enumerated increment type.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setAddressInc(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel,
        enum FDmaPs_addressIncrement addr_inc)
{
    int errorCode;
    int x;
    BOOL write_en;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                write_en = FALSE;
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(x));
                if (sd_sel == Dmac_src || sd_sel == Dmac_src_dst)
				{
					if (((reg & DMAC_CTL_L_SINC) >> 9) != addr_inc)
					{
						reg = (reg & ~DMAC_CTL_L_SINC) | (addr_inc << 9);
                        write_en = TRUE;
                    }
                }
                if (sd_sel == Dmac_dst || sd_sel == Dmac_src_dst)
				{
					if (((reg & DMAC_CTL_L_DINC) >> 7) != addr_inc)
					{
						reg = (reg & ~DMAC_CTL_L_DINC) | (addr_inc << 7);
                        write_en = TRUE;
                    }
                }
                if (write_en)
				{
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(x), reg);
                }
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the address increment type on the specified
* source or destination on the specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument. Only 1, source or destination,
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*
* @return   
*           Enumerated address increment type.
*
* @note     NA.
*
*****************************************************************************/
enum FDmaPs_addressIncrement FDmaPs_getAddressInc(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel)
{
    u8 ch_index;
    u32 reg;
    enum FDmaPs_addressIncrement retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* Allow only source OR destination*/
    FMSH_ASSERT(sd_sel == Dmac_src || sd_sel == Dmac_dst);

    reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(ch_index));
    if (sd_sel == Dmac_src)
	{
        if (((reg & DMAC_CTL_L_SINC) >> 9) == 0x3)
		{
            retval = Dmac_addr_nochange;
        }
		else
		{
            retval = (enum FDmaPs_addressIncrement)((reg & DMAC_CTL_L_SINC) >> 9);
        }
    }
	else
	{
        if (((reg & DMAC_CTL_L_DINC) >> 7) == 0x3)
		{
            retval = Dmac_addr_nochange;
        }
		else
		{
            retval = (enum FDmaPs_addressIncrement)((reg & DMAC_CTL_L_DINC) >> 7);
        }
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the specified source and/or destination
* transfer width on the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument. Both source and destination
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*           width Enumerated transfer width.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setTransWidth(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel,
        enum FDmaPs_transferWidth width)
{
    int errorCode;
    int x;
    BOOL write_en;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Check if the field is hard-coded*/
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                if (sd_sel == Dmac_src || Dmac_src_dst)
				{
                    if (param->ch_stw[x] != 0x0)
					{
                        errorCode = -FMSH_ENOSYS;
                        break;
                    }
                }
                if (sd_sel == Dmac_dst || Dmac_src_dst)
				{
                    if (param->ch_dtw[x] != 0x0)
					{
                        errorCode = -FMSH_ENOSYS;
                        break;
                    }
                }
            }
        }
    }
    if(errorCode == 0) {
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                write_en = FALSE;
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(x));
                if (sd_sel == Dmac_src || sd_sel == Dmac_src_dst)
				{
					if (((reg & DMAC_CTL_L_SRC_TR_WIDTH) >> 4) != width)
					{
						reg = (reg & ~DMAC_CTL_L_SRC_TR_WIDTH) | (width << 4);
                        write_en = TRUE;
                    }
                }
                if (sd_sel == Dmac_dst || sd_sel == Dmac_src_dst)
				{
					if (((reg & DMAC_CTL_L_DST_TR_WIDTH) >> 1) != width)
					{
						reg = (reg & ~DMAC_CTL_L_DST_TR_WIDTH) | (width << 1);
                        write_en = TRUE;
                    }
                }
                if (write_en)
				{
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(x), reg);
                }
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the specified source or destination
* transfer width on the specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument. Only 1, source or destination,
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*
* @return   
*           Enumerated transfer width.
*
* @note     NA.
*
*****************************************************************************/
enum FDmaPs_transferWidth FDmaPs_getTransWidth(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel)
{
    u8 ch_index;
    u32 reg;
    enum FDmaPs_transferWidth retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* Allow only source OR destination*/
    FMSH_ASSERT(sd_sel == Dmac_src || sd_sel == Dmac_dst);

    reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(ch_index));
    if (sd_sel == Dmac_src) 
	{
		retval = (enum FDmaPs_transferWidth)((reg & DMAC_CTL_L_SRC_TR_WIDTH) >> 4);
    }
	else
	{
		retval = (enum FDmaPs_transferWidth)((reg & DMAC_CTL_L_DST_TR_WIDTH) >>1);
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the handshaking interface on the specified
* source or destination on the specified DMA channel.
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument. Both source and destination
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*           hs_inter Enumerated handshaking interface number.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setHsInterface(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel,
        enum FDmaPs_hsInterface   hs_inter)
{
    int errorCode;
    int x;
    BOOL write_en;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errorCode = FDmaPs_checkChannelRange(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Check if the specified value is in range*/
        if (hs_inter >= param->num_hs_int)
		{
            errorCode = -FMSH_EINVAL;
        }
    }
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                write_en = FALSE;
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_H_OFFSET(x));
                if (sd_sel == Dmac_src || sd_sel == Dmac_src_dst)
				{
					if (((reg & DMAC_CFG_H_SRC_PER) >> 7) != hs_inter)
					{
						reg = (reg & ~DMAC_CFG_H_SRC_PER) | (hs_inter << 7);
                        write_en = TRUE;
                    }
                }
                if (sd_sel == Dmac_dst || sd_sel == Dmac_src_dst)
				{
					if (((reg & DMAC_CFG_H_DEST_PER) >> 11) != hs_inter)
					{
						reg = (reg & ~DMAC_CFG_H_DEST_PER) | (hs_inter << 11);
                        write_en = TRUE;
                    }
                }
                if (write_en)
				{
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_H_OFFSET(x), reg);
                }
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the handshaking interface on the specified
* source or destination on the specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument. Only 1, source or destination,
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*
* @return   
*           Enumerated handshaking interface number.
*
* @note     NA.
*
*****************************************************************************/
enum FDmaPs_hsInterface FDmaPs_getHsInterface(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel)
{
    u8 ch_index;
    u32 reg;
    enum FDmaPs_hsInterface retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* Allow only source OR destination*/
    FMSH_ASSERT(sd_sel == Dmac_src || sd_sel == Dmac_dst);

    reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_H_OFFSET(ch_index));
    if(sd_sel == Dmac_src) 
	{
		retval = (enum FDmaPs_hsInterface)((reg & DMAC_CFG_H_SRC_PER) >> 7);
    } else
	{
		retval = (enum FDmaPs_hsInterface)((reg & DMAC_CFG_H_DEST_PER) >> 11);
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function enables/disables the specified source and/or
* destination status update feature on the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument. Both source and destination
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*           state Enumerated Enable/Disable state.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setStatUpdate(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel,
        enum FMSH_state state)
{
    int errorCode;
    int x;
    BOOL write_en;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errorCode = FDmaPs_checkChannelRange(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Check for hard-coded values*/
        for (x = 0; x < param->num_channels; x++)
		{
            if(ch_num & (1 << x))
			{
                if (sd_sel == Dmac_src || sd_sel == Dmac_src_dst)
				{
                    if (param->ch_stat_src[x] == 0x0)
					{
                        errorCode = -FMSH_ENOSYS;
                        break;
                    }
                }
                if (sd_sel == Dmac_dst || sd_sel == Dmac_src_dst)
				{
                    if (param->ch_stat_dst[x] == 0x0)
					{
                        errorCode = -FMSH_ENOSYS;
                        break;
                    }
                }
            }
        }
    }
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                write_en = FALSE;
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_H_OFFSET(x));
                if (sd_sel == Dmac_src || sd_sel == Dmac_src_dst)
				{
					if (((reg & DMAC_CFG_H_SS_UPD_EN) >> 6) != state)
					{
						reg = (reg & ~DMAC_CFG_H_SS_UPD_EN) | (state << 6);
                        write_en = TRUE;
                    }
                }
                if (sd_sel == Dmac_dst || sd_sel == Dmac_src_dst)
				{
					if (((reg & DMAC_CFG_H_DS_UPD_EN) >> 5) != state)
					{
						reg = (reg & ~DMAC_CFG_H_DS_UPD_EN) | (state << 5);
                        write_en = TRUE;
                    }
                }
                if (write_en)
				{
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_H_OFFSET(x), reg);
                }
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns whether the status update feature is
* enabled or disabled for the specified source or destination
* on the specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument. Only 1, source or destination,
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*
* @return   
*           Enumerated Enabled/Disabled state.
*
* @note     NA.
*
*****************************************************************************/
enum FMSH_state FDmaPs_getStatUpdate(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel)
{
    u8 ch_index;
    u32 reg;
    enum FMSH_state retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* Allow only source OR destination*/
    FMSH_ASSERT(sd_sel == Dmac_src || sd_sel == Dmac_dst);

    reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_H_OFFSET(ch_index));
    if(sd_sel == Dmac_src) 
	{
		retval = (enum FMSH_state)((reg & DMAC_CFG_H_SS_UPD_EN) >> 6);
    } else
	{
		retval = (enum FMSH_state)((reg & DMAC_CFG_H_DS_UPD_EN) >> 5);
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the prot level for the AMBA bus
* on the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           prot_lvl Enumerated prot level.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setProtCtl(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_protLevel prot_lvl)
{
    int errorCode;
    int x;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if(ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errorCode = FDmaPs_checkChannelRange(pDmac, ch_num);
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_H_OFFSET(x));
				if (((reg & DMAC_CFG_H_PROTCTL) >> 2) != prot_lvl)
				{
					reg = (reg & ~DMAC_CFG_H_PROTCTL) | (prot_lvl << 2);
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_H_OFFSET(x), reg);
				}
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the prot level for the AMBA bus on the
* specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           Enumerated protection level.
*
* @note     NA.
*
*****************************************************************************/
enum FDmaPs_protLevel FDmaPs_getProtCtl(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    u8 ch_index;
    u32 reg;
    enum FDmaPs_protLevel retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

	reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_H_OFFSET(ch_index));
	retval = (enum FDmaPs_protLevel)((reg & DMAC_CFG_H_PROTCTL) >> 2);
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the FIFO mode on the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*           fifo_mode Enumerated fifo mode for the DMA.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setFifoMode(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_fifoMode fifo_mode)
{
    int errorCode;
    int x;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errorCode = FDmaPs_checkChannelRange(pDmac, ch_num);
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_H_OFFSET(x));
				if (((reg & DMAC_CFG_H_FIFO_MODE) >> 1) != fifo_mode)
				{
					reg = (reg & ~DMAC_CFG_H_FIFO_MODE) | (fifo_mode << 1);
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_H_OFFSET(x), reg);
				}
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the FIFO mode on the specified DMA channel(s)
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           Enumerated FIFO mode for the DMA.
*
* @note     NA.
*
*****************************************************************************/
enum FDmaPs_fifoMode FDmaPs_getFifoMode(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    u8 ch_index;
    u32 reg;
    enum FDmaPs_fifoMode retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

	reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_H_OFFSET(ch_index));
	retval = (enum FDmaPs_fifoMode)((reg & DMAC_CFG_H_FIFO_MODE) >> 1);
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the flow control mode on the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*           fc_mode Enumerated flow control mode.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setFlowCtlMode(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_flowCtlMode  fc_mode)
{
    int errorCode;
    int x;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if(ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errorCode = FDmaPs_checkChannelRange(pDmac, ch_num);
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_H_OFFSET(x));
				if ((reg & DMAC_CFG_H_FCMODE) != fc_mode)
				{
					reg = (reg & ~DMAC_CFG_H_FCMODE) | fc_mode;
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_H_OFFSET(x), reg);
				}
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the flow control mode on the specified DMA channel.
* Only 1 DMA channel can be specified for the FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           Enumerated flow control mode.
*
* @note     NA.
*
*****************************************************************************/
enum  FDmaPs_flowCtlMode FDmaPs_getFlowCtlMode(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    u8 ch_index;
    u32 reg;
    enum FDmaPs_flowCtlMode retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

	reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_H_OFFSET(ch_index));
	retval = (enum FDmaPs_flowCtlMode)(reg & DMAC_CFG_H_FCMODE);
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the maximum amba burst length on the specified
* DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           burst_length AMBA burst length value.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setMaxAmbaBurstLength(
        FDmaPs_T *pDmac,
        enum   FDmaPs_channelNumber ch_num,
        u16 burst_length )
{
    int errorCode;
    int x;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if(ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }
    
    /* Check for valid channel number*/
    errorCode = FDmaPs_checkChannelRange(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Check for hard-coded value*/
        if (param->mabrst == 0x0)
		{
            errorCode = -FMSH_ENOSYS;
        }
    }
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(x));
				if (((reg & DMAC_CFG_L_MAX_ABRST) >> 20) != burst_length)
				{
					reg = (reg & ~DMAC_CFG_L_MAX_ABRST) | (burst_length << 20);
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(x), reg);
				}
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the maximum amba burst length 
* on the specified DMA channel.
* Only ONE DMA channel can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           AMBA burst length value.
*
* @note     NA.
*
*****************************************************************************/
u16 FDmaPs_getMaxAmbaBurstLength(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    u8 ch_index;
    u32 reg;
    u16 retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

	reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(ch_index));
	retval = (reg & DMAC_CFG_L_MAX_ABRST) >> 20;
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the handshaking interface polarity on the
* specified source and/or destination on the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument. Both source and destination
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*           pol_level Enumerated polarity level.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setHsPolarity(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel,
        enum FDmaPs_polarityLevel pol_level)
{
    int errorCode;
    int x;
    BOOL write_en;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errorCode = FDmaPs_checkChannelRange(pDmac, ch_num);
    if (errorCode == 0) {
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                write_en = FALSE;
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(x));
                if (sd_sel == Dmac_src || sd_sel == Dmac_src_dst)
				{
					if (((reg & DMAC_CFG_L_SRC_HS_POL) >> 19) != pol_level)
					{
						reg = (reg & ~DMAC_CFG_L_SRC_HS_POL) | (pol_level << 19);
                        write_en = TRUE;
                    }
                }
                if (sd_sel == Dmac_dst || sd_sel == Dmac_src_dst)
				{
					if (((reg & DMAC_CFG_L_DST_HS_POL) >> 18) != pol_level)
					{
						reg = (reg & ~DMAC_CFG_L_DST_HS_POL) | (pol_level << 18);
                        write_en = TRUE;
                    }
                }
                if (write_en)
				{
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(x), reg);
                }
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the handshaking interface polarity on the
* specified source or destination on the specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument. Only 1, source or destination,
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*
* @return   
*           Enumerated polarity level.
*
* @note     NA.
*
*****************************************************************************/
enum FDmaPs_polarityLevel FDmaPs_getHsPolarity(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel)
{
    u8 ch_index;
    u32 reg;
    enum FDmaPs_polarityLevel retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* Allow only source OR destination*/
    FMSH_ASSERT(sd_sel == Dmac_src || sd_sel == Dmac_dst);

    reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(ch_index));
    if(sd_sel == Dmac_src) 
	{
		retval = (enum FDmaPs_polarityLevel)((reg & DMAC_CFG_L_SRC_HS_POL) >> 19);
    } else
	{
		retval = (enum FDmaPs_polarityLevel)((reg & DMAC_CFG_L_DST_HS_POL) >> 18);
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the lock level for the specified bus and/or
* channel on the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           bus_ch Enumerated channel or bus lock select.
*           lock_l Enumerated level for the lock feature.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setLockLevel(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_lockBusCh    bus_ch,
        enum FDmaPs_lockLevel     lock_l)
{
    int errorCode;
    int x;
    BOOL write_en;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if(ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errorCode = FDmaPs_checkChannelRange(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Check for hard-coded values*/
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                if (param->ch_lock_en[x] == 0x0)
				{
                    errorCode = -FMSH_ENOSYS;
                    break;
                }
            }
        }
    }
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                write_en = FALSE;
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(x));
                if (bus_ch == Dmac_lock_bus || bus_ch == Dmac_lock_bus_ch)
				{
					if (((reg & DMAC_CFG_L_LOCK_B_L) >> 14) != lock_l)
					{
						reg = (reg & ~DMAC_CFG_L_LOCK_B_L) | (lock_l << 14);
                        write_en = TRUE;
                    }
                }
                if (bus_ch == Dmac_lock_channel || bus_ch == Dmac_lock_bus_ch)
				{
					if (((reg & DMAC_CFG_L_LOCK_CH_L) >> 12) != lock_l)
					{
						reg = (reg & ~DMAC_CFG_L_LOCK_CH_L) | (lock_l << 12);
                        write_en = TRUE;
                    }
                }
                if (write_en)
				{
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(x), reg);
                }
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the lock level for the specified bus or
* channel on the specified DMA channel.  Only 1 DMA channel can be
* specified for the FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           bus_ch Enumerated channel or bus lock select.
*
* @return   
*           Enumerated level for the lock feature.
*
* @note     NA.
*
*****************************************************************************/
enum  FDmaPs_lockLevel FDmaPs_getLockLevel(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_lockBusCh bus_ch)
{
    u8 ch_index;
    u32 reg;
    enum FDmaPs_lockLevel retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* Allow only bus OR channel*/
    FMSH_ASSERT(bus_ch == Dmac_lock_bus || bus_ch == Dmac_lock_channel);

    reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(ch_index));
    if (bus_ch == Dmac_lock_bus) 
	{
		retval = (enum FDmaPs_lockLevel)((reg & DMAC_CFG_L_LOCK_B_L) >> 14);
    }
	else
	{
		retval = (enum FDmaPs_lockLevel)((reg & DMAC_CFG_L_LOCK_CH_L) >> 12);
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function enables/disables the lock feature on the specified
* bus and/or channel on the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           bus_ch Enumerated channel or bus lock select.
*           state Enumerated Enable/Disable state.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setLockEnable(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_lockBusCh    bus_ch,
        enum FMSH_state state)
{
    int errorCode;
    int x;
    BOOL write_en;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);
    
    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errorCode = FDmaPs_checkChannelRange(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Check for hard-coded values*/
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                if (param->ch_lock_en[x] == 0x0)
				{
                    errorCode = -FMSH_ENOSYS;
                    break;
                }
            }
        }
    }
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                write_en = FALSE;
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(x));
                if (bus_ch == Dmac_lock_bus || bus_ch == Dmac_lock_bus_ch)
                {
					if (((reg & DMAC_CFG_L_LOCK_B) >> 17) != state)
					{
						reg = (reg & ~DMAC_CFG_L_LOCK_B) | (state << 17);
                        write_en = TRUE;
                    }
                }
                if (bus_ch == Dmac_lock_channel || bus_ch == Dmac_lock_bus_ch)
                {
					if (((reg & DMAC_CFG_L_LOCK_CH) >> 16) != state)
					{
						reg = (reg & ~DMAC_CFG_L_LOCK_CH) | (state << 16);
                        write_en = TRUE;
                    }
                }
                if (write_en)
				{
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(x), reg);
                }
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the enabled or disabled the lock status 
* on the specified bus or channel on the specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           bus_ch Enumerated channel or bus lock select.
*
* @return   
*           Enumerated Enabled/Disabled state.
*
* @note     NA.
*
*****************************************************************************/
enum  FMSH_state FDmaPs_getLockEnable(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_lockBusCh bus_ch)
{
    u8 ch_index;
    u32 reg;
    enum FMSH_state retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);
    
    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* Allow only bus OR channel*/
    FMSH_ASSERT(bus_ch == Dmac_lock_bus || bus_ch == Dmac_lock_channel);

    reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(ch_index));
    if (bus_ch == Dmac_lock_bus) 
	{
		retval = (enum FMSH_state)((reg & DMAC_CFG_L_LOCK_B) >> 17);
    }
	else
	{
		retval = (enum FMSH_state)((reg & DMAC_CFG_L_LOCK_CH) >> 16);
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the handshaking mode from hardware to software
* on the specified source and/or destination on the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument. Both source and destination
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*           hs_hwsw_sel Enumerated software/hardware handshaking select.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setHandshakingMode(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber  ch_num,
        enum FDmaPs_srcDstSelect  sd_sel,
        enum FDmaPs_swHwHsSelect hs_hwsw_sel)
{
    int errorCode;
    int x;
    BOOL write_en;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errorCode = FDmaPs_checkChannelRange(pDmac, ch_num);
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                write_en = FALSE;
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(x));
                if (sd_sel == Dmac_src || sd_sel == Dmac_src_dst)
				{
					if (((reg & DMAC_CFG_L_HS_SEL_SRC) >> 11) != hs_hwsw_sel)
					{
						reg = (reg & ~DMAC_CFG_L_HS_SEL_SRC) | (hs_hwsw_sel << 11);
                        write_en = TRUE;
                    }
                }
                if (sd_sel == Dmac_dst || sd_sel == Dmac_src_dst)
				{
					if (((reg & DMAC_CFG_L_HS_SEL_DST) >> 10) != hs_hwsw_sel)
					{
						reg = (reg & ~DMAC_CFG_L_HS_SEL_DST) | (hs_hwsw_sel << 10);
                        write_en = TRUE;
                    }
                }
                if (write_en)
				{
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(x), reg);
                }
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the handshaking mode hardware or software
* on the specified source or destination on the specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument. Only 1, source or destination,
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*
* @return   
*           Enumerated software/hardware handshaking select.
*
* @note     NA.
*
*****************************************************************************/
enum FDmaPs_swHwHsSelect FDmaPs_getHandshakingMode(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel)
{
    u8 ch_index;
    u32 reg;
    enum FDmaPs_swHwHsSelect retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);
    
    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));
    
    /* Allow only source OR destination*/
    FMSH_ASSERT(sd_sel == Dmac_src || sd_sel == Dmac_dst);

    reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(ch_index));
    if (sd_sel == Dmac_src) 
	{
		retval = (enum FDmaPs_swHwHsSelect)((reg & DMAC_CFG_L_HS_SEL_SRC) >> 11);
    }
	else
	{
		retval = (enum FDmaPs_swHwHsSelect)((reg & DMAC_CFG_L_HS_SEL_DST) >> 10);
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the priority level on the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           ch_priority Enumerated priority level.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setChannelPriority(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber   ch_num,
        enum FDmaPs_channelPriority ch_priority)
{
    int errorCode;
    int x;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errorCode = FDmaPs_checkChannelRange(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* check the value is in range*/
        if (ch_priority > param->num_channels - 1)
		{
            errorCode = -FMSH_EINVAL;
        }
    }
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(x));
				if (((reg & DMAC_CFG_L_CH_PRIOR) >> 5) != ch_priority)
				{
					reg = (reg & ~DMAC_CFG_L_CH_PRIOR) | (ch_priority << 5);
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(x), reg);
				}
            }
        }

        /* Set the channel priority order*/
        FDmaPs_setChannelPriorityOrder(pDmac);
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the priority level on the specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           Enumerated channel priority level.
*
* @note     NA.
*
*****************************************************************************/
enum FDmaPs_channelPriority FDmaPs_getChannelPriority(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    u8 ch_index;
    u32 reg;
    enum FDmaPs_channelPriority retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

	reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(ch_index));
	retval = (enum FDmaPs_channelPriority)((reg & DMAC_CFG_L_CH_PRIOR) >> 5);
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the list master select interface on the specified
* DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           mst_num Enumerated master interface number.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setListMstSelect(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_masterNumber  mst_num)
{
    int errorCode;
    int x;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    errorCode = 0;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Check if the register is present*/
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                if (param->ch_hc_llp[x] == 0x1)
				{
                    errorCode = -FMSH_ENOSYS;
                }
            }
        }
    }

    if (errorCode == 0)
	{
        /* Check is the specified master interface is in range*/
        if (mst_num >= param->num_master_int)
		{
            errorCode = -FMSH_EINVAL;
        }
		else
		{
            /* Check for hard-coded values*/
            for (x = 0; x < param->num_channels; x++)
			{
                if (ch_num & (1 << x))
				{
                    if (param->ch_lms[x] != 0x4)
					{
                        errorCode = -FMSH_ENOSYS;
                        break;
                    }
                }
            }
        }
    }
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_LLP_L_OFFSET(x));
				if ((reg & DMAC_LLP_L_LMS) != mst_num)
				{
					reg = (reg & ~DMAC_LLP_L_LMS) | mst_num;
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_LLP_L_OFFSET(x), reg);
				}
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the list master select interface on the
* specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           Enumerated master interface number.
*
* @note     NA.
*
*****************************************************************************/
enum FDmaPs_masterNumber FDmaPs_getListMstSelect(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    u8 ch_index;
    u32 reg;
    enum FDmaPs_masterNumber retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

	reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_LLP_L_OFFSET(ch_index));
	retval = (enum FDmaPs_masterNumber)(reg & DMAC_LLP_L_LMS);
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the address for the first linked list item
* in the system memory for the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           address Linked list item address.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setListPointerAddress(
        FDmaPs_T *pDmac,
        enum   FDmaPs_channelNumber ch_num,
        u32 address)
{
    int errorCode;
    int x;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    errorCode = 0;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    /* Check if the register is present*/
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                if (param->ch_hc_llp[x] == 0x1)
				{
                    errorCode = -FMSH_ENOSYS;
                }
            }
        }
    }

    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_LLP_L_OFFSET(x));
				if (((reg & DMAC_LLP_L_LOC) >> 2) != address)
				{
					reg = (reg & ~DMAC_LLP_L_LOC) | (address << 2);
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_LLP_L_OFFSET(x), reg);
				}
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the address for the first linked list item
* in the system memory for the specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           Linked list item address.
*
* @note     NA.
*
*****************************************************************************/
u32 FDmaPs_getListPointerAddress(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    u8 ch_index;
    u32 reg;
    u32 retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    if(param->ch_hc_llp[ch_index] != 0x1)
	{
		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_LLP_L_OFFSET(ch_index));
    }
	else
	{
        reg = 0;
    }
	retval = (reg & DMAC_LLP_L_LOC) >> 2;
    return retval;
}

/*****************************************************************************
*
* @description
* This function enables or disables the block chaining on the 
* specified source and/or destination on the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument. Both source and destination
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*           state Enumerated enable/disable state.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setLlpEnable(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel,
        enum FMSH_state state)
{
    int x, errorCode;
    BOOL write_en;
    u32 reg;
    FDmaPs_Param_T *param;
    
    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
	
    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Check for hard-coded values*/
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                if (param->ch_multi_blk_en[x] == 0x0 || param->ch_hc_llp[x] == 0x1)
				{
                    errorCode = -FMSH_ENOSYS;
                    break;
                }
            }
        }
    }
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                write_en = FALSE;
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(x));
                if (sd_sel == Dmac_src || sd_sel == Dmac_src_dst)
				{
					if(((reg & DMAC_CTL_L_LLP_SRC_EN) >> 28) != state)
					{
						reg = (reg & ~DMAC_CTL_L_LLP_SRC_EN) | (state << 28);
                        write_en = TRUE;
                    }
                }
                if (sd_sel == Dmac_dst || sd_sel == Dmac_src_dst)
				{
					if(((reg & DMAC_CTL_L_LLP_DST_EN) >> 27) != state)
					{
						reg = (reg & ~DMAC_CTL_L_LLP_DST_EN) | (state << 27);
                        write_en = TRUE;
                    }
                }
                /* only do the write if the value being written*/
                /* differs from the current register value*/
                if (write_en == 1)
				{
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(x), reg);
                }
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns whether block chaining is enabled or disabled
* on the specified source or destination on the specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument. Only 1, source or destination,
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*
* @return   
*           Enumerated Enabled/Disabled state.
*
* @note     NA.
*
*****************************************************************************/
enum FMSH_state FDmaPs_getLlpEnable(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel)
{
    u8 ch_index;
    u32 reg;
    enum FMSH_state retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* Allow only source OR destination*/
    FMSH_ASSERT(sd_sel == Dmac_src || sd_sel == Dmac_dst);

    reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CTL_L_OFFSET(ch_index));
    if (sd_sel == Dmac_src) 
	{
		retval = (enum FMSH_state)((reg & DMAC_CTL_L_LLP_SRC_EN) >> 28);
    }
	else
	{
		retval = (enum FMSH_state)((reg & DMAC_CTL_L_LLP_DST_EN) >> 27);
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function enables or disables the reload feature on the
* specified source and/or destination on the specified DMA channel(s).
* Multiple DMA channels can be specified for the ch_num argument. Both
* source and destination can be specified for the sd_sel argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*           state Enumerated enable/disable state.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setReload(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel,
        enum FMSH_state state)
{
    int x, errorCode;
    BOOL write_en;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number*/
    errorCode = FDmaPs_checkChannelRange(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Check for hard-coded values*/
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                if (param->ch_multi_blk_en[x] == 0x0)
				{
                    errorCode = -FMSH_ENOSYS;
                    break;
                }
                if (param->ch_multi_blk_type[x] != 0x0)
				{
                    if (sd_sel == Dmac_src || sd_sel == Dmac_src_dst)
					{
                        if (state == FMSH_set &&
                            (param->ch_multi_blk_type[x] != Dmac_reload_cont &&
                             param->ch_multi_blk_type[x] != Dmac_reload_llp))
                        {
                            errorCode = -FMSH_ENOSYS;
                            break;
                        }
                        if (state == FMSH_clear &&
                            (param->ch_multi_blk_type[x] == Dmac_reload_cont ||
                             param->ch_multi_blk_type[x] == Dmac_reload_llp))
                        {
                            errorCode = -FMSH_ENOSYS;
                            break;
                        }
                    }
                    if (sd_sel == Dmac_dst || sd_sel == Dmac_src_dst)
					{
                        if (state == FMSH_set &&
                            (param->ch_multi_blk_type[x] != Dmac_cont_reload &&
                             param->ch_multi_blk_type[x] != Dmac_reload_reload &&
                             param->ch_multi_blk_type[x] != Dmac_llp_reload))
                        {
                            errorCode = -FMSH_ENOSYS;
                            break;
                        }
                        if (state == FMSH_clear &&
                            (param->ch_multi_blk_type[x] == Dmac_cont_reload ||
                             param->ch_multi_blk_type[x] == Dmac_reload_reload ||
                             param->ch_multi_blk_type[x] == Dmac_llp_reload))
                        {
                            errorCode = -FMSH_ENOSYS;
                            break;
                        }
                    }
                }
            }
        }
    }
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                write_en = FALSE;
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(x));
                if (sd_sel == Dmac_src || sd_sel == Dmac_src_dst)
				{
					if (((reg & DMAC_CFG_L_RELOAD_SRC) >> 30) != state)
					{
						reg = (reg & ~DMAC_CFG_L_RELOAD_SRC) | (state << 30);
                        write_en = TRUE;
                    }
                }
                if (sd_sel == Dmac_dst || sd_sel == Dmac_src_dst)
				{
					if (((reg & DMAC_CFG_L_RELOAD_DST) >> 31) != state)
					{
						reg = (reg & ~DMAC_CFG_L_RELOAD_DST) | (state << 31);
                        write_en = TRUE;
                    }
                }
                /* only do the write if the value being written*/
                /* differs from the current register value*/
                if (write_en == 1)
				{
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(x), reg);
                }
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns whether the reload feature is enabled or
* disabled on the specified source or destination on the specified
* DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument. Only 1, source or destination,
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*
* @return   
*           Enumerated Enabled/Disabled state.
*
* @note     NA.
*
*****************************************************************************/
enum FMSH_state FDmaPs_getReload(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel)
{
    u8 ch_index;
    u32 reg;
    enum FMSH_state retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* Allow only source OR destination*/
    FMSH_ASSERT(sd_sel == Dmac_src || sd_sel == Dmac_dst);

    reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(ch_index));
    if (sd_sel == Dmac_src) 
	{
		retval = (enum FMSH_state)((reg & DMAC_CFG_L_RELOAD_SRC) >> 30);
    }
	else
	{
		retval = (enum FMSH_state)((reg & DMAC_CFG_L_RELOAD_DST) >> 31);
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the status registers on the specified source
* and/or destination on the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument. Both source and destination
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*           value 32-bit status value.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setStatus(
        FDmaPs_T *pDmac,
        enum   FDmaPs_channelNumber ch_num,
        enum   FDmaPs_srcDstSelect sd_sel,
        u32 value)
{
    int errorCode;
    int x;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    if (errorCode == 0)
	{            
        /* Check if the register exists*/
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                if (sd_sel == Dmac_src || sd_sel == Dmac_src_dst)
				{
                    if (param->ch_stat_src[x] == 0x0)
					{
                        errorCode = -FMSH_ENOSYS;
                        break;
                    }
                }
                if (sd_sel == Dmac_dst || sd_sel == Dmac_src_dst)
				{
                    if (param->ch_stat_dst[x] == 0x0)
					{
                        errorCode = -FMSH_ENOSYS;
                        break;
                    }
                }
            }
        }
    }
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                if (sd_sel == Dmac_src || sd_sel == Dmac_src_dst)
				{
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_SSTAT_L_OFFSET(x), value);
                }
                if (sd_sel == Dmac_dst || sd_sel == Dmac_src_dst)
				{
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_DSTAT_L_OFFSET(x), value);
                }
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the status registers on the specified 
* source or destination on the specified DMA channel.
* Only ONE DMA channel can be specified for the
* FDmaPs_channelNumber argument. Only ONE, source or destination,
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*
* @return   
*           32-bit status value.
*
* @note     NA.
*
*****************************************************************************/
u32 FDmaPs_getStatus(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel)
{
    u8 ch_index;
    u32 retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* Allow only source OR destination*/
    FMSH_ASSERT(sd_sel == Dmac_src || sd_sel == Dmac_dst);

    retval = 0;
    if (sd_sel == Dmac_src)
	{
        if (param->ch_stat_src[ch_index] != 0)
		{
			retval = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_SSTAT_L_OFFSET(ch_index));
        }
    }
	else
	{
        if (param->ch_stat_dst[ch_index] != 0)
		{
			retval = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_DSTAT_L_OFFSET(ch_index));
        }
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the status address registers on the specified
* source and/or destination on the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument. Both source and destination
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*           address 32-bit address from where status is fetched.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setStatusAddress(
        FDmaPs_T *pDmac,
        enum   FDmaPs_channelNumber ch_num,
        enum   FDmaPs_srcDstSelect sd_sel,
        u32 address)
{
    int x;
    int errorCode;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Check if the register exists*/
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                if (sd_sel == Dmac_src || sd_sel == Dmac_src_dst)
				{
                    if (param->ch_stat_src[x] == 0x0)
					{
                        errorCode = -FMSH_ENOSYS;
                        break;
                    }
                }
                if (sd_sel == Dmac_dst || sd_sel == Dmac_src_dst)
				{
                    if (param->ch_stat_dst[x] == 0x0)
					{
                        errorCode = -FMSH_ENOSYS;
                        break;
                    }
                }
            }
        }
    }
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                if (sd_sel == Dmac_src || sd_sel == Dmac_src_dst)
				{
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_SSTATAR_L_OFFSET(x), address);
                }
                if (sd_sel == Dmac_dst || sd_sel == Dmac_src_dst)
				{
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_DSTATAR_L_OFFSET(x), address);
                }
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the status address register on the
* specified source or destination on the specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument. Only 1, source or destination,
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*
* @return   
*           32-bit address from where status is fetched.
*
* @note     NA.
*
*****************************************************************************/
u32 FDmaPs_getStatusAddress(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel)
{
    u8 ch_index;
    u32 retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* Allow only source OR destination*/
    FMSH_ASSERT(sd_sel == Dmac_src || sd_sel == Dmac_dst);

    retval = 0;
    if (sd_sel == Dmac_src)
	{
        if(param->ch_stat_src[ch_index] != 0)
		{
			retval = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_SSTATAR_L_OFFSET(ch_index));
        }
    }
	else
	{
        if(param->ch_stat_dst[ch_index] != 0)
		{
			retval = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_DSTATAR_L_OFFSET(ch_index));
        }
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the specified gather interval or count
* on the specified DMA channel(s).
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           cnt_int Enumerated count/interval select.
*           value Count or interval value.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setGatherParam(
        FDmaPs_T *pDmac,
        enum   FDmaPs_channelNumber ch_num,
        enum   FDmaPs_scatterGatherParam cnt_int,
        u32 value)
{
    int errorCode;
    int x;
    BOOL write_en;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
	
    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Check if the register exists*/
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                if (param->ch_src_gat_en[x] == 0x0)
				{
                    errorCode = -FMSH_ENOSYS;
                    break;
                }
            }
        }
    }
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                write_en = FALSE;
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_SGR_L_OFFSET(x));
                if (cnt_int == Dmac_sg_count)
				{
					if (((reg & DMAC_SGR_L_SGC) >> 20) != value)
					{
						reg = (reg & ~DMAC_SGR_L_SGC) | (value << 20);
                        write_en = TRUE;
                    }
                }
                if (cnt_int == Dmac_sg_interval)
				{
					if((reg & DMAC_SGR_L_SGI) != value)
					{
						reg = (reg & ~DMAC_SGR_L_SGI) | value;
                        write_en = TRUE;
                    }
                }
                if (write_en)
				{
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_SGR_L_OFFSET(x), reg);
                }
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the specified gather interval or count
* on the specified DMA channel. Only 1 DMA channel can be specified
* for the FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           cnt_int Enumerated count/interval select.
*
* @return   
*           Count or interval value.
*
* @note     NA.
*
*****************************************************************************/
u32 FDmaPs_getGatherParam(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_scatterGatherParam cnt_int)
{
    u8 ch_index;
    u32 reg;
    u32 retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    if (param->ch_src_gat_en[ch_index] != 0x0)
	{
		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_SGR_L_OFFSET(ch_index));
    }
	else
	{
        reg = 0x0;
    }

    if(cnt_int == Dmac_sg_count)
	{
		retval = (reg & DMAC_SGR_L_SGC) >> 20;
    }
	else
	{
		retval = reg & DMAC_SGR_L_SGI;
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function sets the specified scatter interval or count on the
* specified DMA channel.
* Multiple DMA channels can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           cnt_int Enumerated count/interval select.
*           value Count or interval value.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_setScatterParam(
        FDmaPs_T *pDmac,
        enum   FDmaPs_channelNumber ch_num,
        enum   FDmaPs_scatterGatherParam cnt_int,
        u32 value)
{
    int errorCode;
    int x;
    BOOL write_en;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    /* Limit all channels to the number of channels on this*/
    /* configuration of the DMA controller.*/
    if (ch_num == Dmac_all_channels)
	{
        ch_num &= DMAC_CH_ALL_MASK;
    }

    /* Check for valid channel number and not busy*/
    errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    if (errorCode == 0)
	{
        /* Check if the register exists*/
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                if (param->ch_dst_sca_en[x] == 0x0)
				{
                    errorCode = -FMSH_ENOSYS;
                    break;
                }
            }
        }
    }
    if (errorCode == 0)
	{
        for (x = 0; x < param->num_channels; x++)
		{
            if (ch_num & (1 << x))
			{
                write_en = FALSE;
				reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_DSR_L_OFFSET(x));
                if (cnt_int == Dmac_sg_count)
				{
					if (((reg & DMAC_DSR_L_DSC) >> 20) != value)
					{
						reg = (reg & ~DMAC_DSR_L_DSC) | (value << 20);
                        write_en = TRUE;
                    }
                }
                if (cnt_int == Dmac_sg_interval)
				{
					if ((reg & DMAC_DSR_L_DSI) != value)
					{
						reg = (reg & ~DMAC_DSR_L_DSI) | value;
                        write_en = TRUE;
                    }
                }
                if (write_en)
				{
					FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_CHX_DSR_L_OFFSET(x), reg);
                }
            }
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function returns the specified scatter interval or count
* on the specified DMA channel.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           ch_int Enumerated count/interval select.
*
* @return   
*           Count or interval value.
*
* @note     NA.
*
*****************************************************************************/
u32  FDmaPs_getScatterParam(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_scatterGatherParam cnt_int)
{
    u8 ch_index;
    u32 reg;
    u32 retval;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    if (param->ch_dst_sca_en[ch_index] != 0x0)
	{
		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_DSR_L_OFFSET(ch_index));
    }
	else
	{
        reg = 0;
    }

    if (cnt_int == Dmac_sg_count)
	{
		retval = (reg & DMAC_DSR_L_DSC) >> 20;
    }
	else
	{
		retval = reg & DMAC_DSR_L_DSI;
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function returns the channel index from the specified channel
* enumerated type.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           ch_num Enumerated DMA channel number.
*
* @return   
*           The DMA channel index.
*
* @note     NA.
*
*****************************************************************************/
unsigned FDmaPs_getChannelIndex(enum FDmaPs_channelNumber ch_num)
{
    unsigned ch_enum=1;
    unsigned ch_index=0;

    ch_num &= DMAC_MAX_CH_MASK;
	
    while (ch_index < DMAC_MAX_CHANNELS)
	{
        if (ch_enum == ch_num)
		{
			break;
		}
		
        ch_enum *= 2; 
		ch_index++;
    }
    return ch_index;
}  

/*****************************************************************************
*
* @description
* This function returns the number of channels that the DMA controller
* is configured to have. This function returns the value on the
* DMAH_NUM_CHANNELS hardware parameter for the specified DMA
* controller device.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*
* @return   
*           Number of channels that the DMA controller was 
*           configured to have.
*
* @note     NA.
*
*****************************************************************************/
u8 FDmaPs_getNumChannels(FDmaPs_T *pDmac)
{
    FDmaPs_Param_T *param;
    
    param = pDmac->comp_param;

    return param->num_channels;
}

/*****************************************************************************
*
* @description
* This function returns the FIFO depth of the specified DMA channel
* that the DMA controller is configured to have. This function
* returns the value on the DMAH_CHx_FIFO_DEPTH hardware
* parameter for the specified DMA controller device.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           Depth of the FIFO for the specified DMA channel.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_getChannelFifoDepth(
       FDmaPs_T *pDmac,
       enum FDmaPs_channelNumber ch_num)
{
    u8 ch_index;
    FDmaPs_Param_T *param;
    
    param = pDmac->comp_param;

    ch_index = FDmaPs_getChannelIndex(ch_num);

    return param->ch_fifo_depth[ch_index];
}

/*****************************************************************************
*
* @description
* This function creates a Linked List Item or appends a current linked
* list with a new item. The FDmaPs_ChannelConfig_T structure handle
* contains the values for the FDmaPs_lli_item structure members.
*
* @param    
*           lhead Handle to a dw_list_head structure.
*           lli_item Handle to a FDmaPs_lli_item structure.
*           config Handle to a FDmaPs_ChannelConfig_T structure.
*
* @return   
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FDmaPs_addLliItem(
        FMSH_listHead *lhead,
        FDmaPs_LliItem_T *lli_item,
        FDmaPs_ChannelConfig_T *config)
{
    FDmaPs_LliItem_T *prev_entry;

    FMSH_listAddTail(&lli_item->list, lhead);

    /* set the LLP of the previous lli_item to the current lli_item*/
    /* address*/
    prev_entry = FMSH_LIST_ENTRY(lli_item->list.prev, FDmaPs_LliItem_T, list);
    prev_entry->llp = (u32) lli_item;
    
    lli_item->sar = config->sar;
    lli_item->dar = config->dar;
    lli_item->llp = NULL; /* set next time this function is called*/
    lli_item->ctl_l =  
    (config->ctl_int_en) |
    (config->ctl_dst_tr_width << 1)  |
    (config->ctl_src_tr_width << 4)  |
    (config->ctl_dinc << 7)          |
    (config->ctl_sinc << 9)          |
    (config->ctl_dst_msize << 11)     |
    (config->ctl_src_msize << 14)     |
    (config->ctl_src_gather_en << 17) |
    (config->ctl_dst_scatter_en << 18)|
    (config->ctl_tt_fc << 20)         |
    (config->ctl_dms << 23)           |
    (config->ctl_sms << 25)           |
    (config->ctl_llp_dst_en << 27)    |
    (config->ctl_llp_src_en << 28);

    lli_item->ctl_h = (config->ctl_block_ts) | (0 << 12);

    lli_item->sstat = 0;
    lli_item->dstat = 0;
}

/*****************************************************************************
*
* @description
* This function identifies the current highest priority active
* interrupt, if any, and forwards it to a user-specified listener
* function for processing.  This allows a user absolute control over
* how each DMAC interrupt is processed.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*
* @return   
*           FMSH_SUCCESS an interrupt was processed.
*           FMSH_FAILURE no interrupt was processed.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_userIrqHandler(FDmaPs_T *pDmac)
{
    int i, ch_index, retval;
    int callbackArg;
    u32 reg, irq_active, mask;
    enum FDmaPs_irq clearIrqMask;
    FDmaPs_Param_T *param;
    FDmaPs_Instance_T *instance;
    FMSH_callback userCallback;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    instance = pDmac->instance;

    /* Assume an interrupt will be processed.  The return value will be*/
    /* set to FALSE if an active interrupt is not found.*/
    retval = FMSH_SUCCESS;
    userCallback = NULL;

    /* read the interrupt status register*/
	irq_active = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_STATUS_INT_L_OFFSET);

    /* ERR INTERRUPT*/
    if (irq_active & DMAC_STATUSINT_L_ERR)
	{
		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_STATUS_ERR_L_OFFSET);

        /* Loop through the channels until we find*/
        /* active interrupt. We start at the highest priority*/
        /* channel and work down to the lowest priority.*/
        for (i = 0; i < param->num_channels; i++)
		{
            mask = 1 << instance->ch_order[i];
            if (reg & mask)
			{
                ch_index = instance->ch_order[i];
                break;
            }
        }

        /* Setup interrupt(s) to clear and call to listener function.*/
        userCallback = instance->ch[ch_index].userListener;
        callbackArg = Dmac_irq_err;
        clearIrqMask = Dmac_irq_err;
    }
    /* TFR INTERRUPT*/
    else if (irq_active & DMAC_STATUSINT_L_TFR)
	{
		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_STATUS_TFR_L_OFFSET);
        
        /* Loop through the channels until we find*/
        /* active interrupt. We start at the highest priority*/
        /* channel and work down to the lowest priority.*/
        for (i = 0; i < param->num_channels; i++)
		{
            mask = 1 << instance->ch_order[i];
            if (reg & mask)
			{
                ch_index = instance->ch_order[i];
                break;
            }
        }

        /* Setup interrupt(s) to clear and call to listener function.*/
        userCallback = instance->ch[ch_index].userCallback;
        callbackArg = Dmac_irq_tfr;
        clearIrqMask = (enum FDmaPs_irq)(Dmac_irq_block | Dmac_irq_srctran | Dmac_irq_dsttran);
    }
    /* BLOCK INTERRUPT*/
    else if (irq_active & DMAC_STATUSINT_L_BLOCK)
	{
		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_STATUS_BLOCK_L_OFFSET);

        /* Loop through the channels until we find*/
        /* active interrupt. We start at the highest priority*/
        /* channel and work down to the lowest priority.*/
        for (i = 0; i < param->num_channels; i++)
		{
            mask = 1 << instance->ch_order[i];
            if (reg & mask)
			{
                ch_index = instance->ch_order[i];
                break;
            }
        }

        /* Setup interrupt(s) to clear and call to listener function.*/
        userCallback = instance->ch[ch_index].userListener;
        callbackArg = Dmac_irq_block;
        clearIrqMask = (enum FDmaPs_irq)(Dmac_irq_block | Dmac_irq_srctran | Dmac_irq_dsttran);
    }
    /* SRCTRAN INTERRUPT*/
    else if (irq_active & DMAC_STATUSINT_L_SRCTRAN)
	{
		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_STATUS_SRCTRAN_L_OFFSET);

        /* Loop through the channels until we find*/
        /* active interrupt. We start at the highest priority*/
        /* channel and work down to the lowest priority.*/
        for (i = 0; i < param->num_channels; i++)
		{
            mask = 1 << instance->ch_order[i];
            if (reg & mask)
			{
                ch_index = instance->ch_order[i];
                break;
            }
        }

        /* Setup interrupt(s) to clear and call to listener function.*/
        userCallback = instance->ch[ch_index].userListener;
        callbackArg = Dmac_irq_srctran;
        clearIrqMask = Dmac_irq_srctran;
    }
    /* DSTTRAN INTERRUPT*/
    else if(irq_active & DMAC_STATUSINT_L_DSTTRAN)
	{
		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_STATUS_DSTTRAN_L_OFFSET);

        /* Loop through the channels until we find*/
        /* active interrupt. We start at the highest priority*/
        /* channel and work down to the lowest priority.*/
        for (i = 0; i < param->num_channels; i++)
		{
            mask = 1 << instance->ch_order[i];
            if (reg & mask)
			{
                ch_index = instance->ch_order[i];
                break;
            }
        }

        /* Setup interrupt(s) to clear and call to listener function.*/
        userCallback = instance->ch[ch_index].userListener;
        callbackArg = Dmac_irq_dsttran;
        clearIrqMask = Dmac_irq_dsttran;
    }
    else
	{
        /* no active interrupt was found*/
        retval = FMSH_FAILURE;
    }

    /* FMSH_ASSERT that a listener function exists for the active channel*/
    FMSH_ASSERT(instance->ch[ch_index].userListener != NULL);

    /* call the listener function*/
    if(userCallback != NULL)
    {
        userCallback(pDmac, callbackArg);
    }

    /* clear interrupt(s)*/
    if(clearIrqMask != 0)
    {
        FDmaPs_clearIrq(pDmac, (enum FDmaPs_channelNumber)DMAC_CH_NUM(ch_index), clearIrqMask);
    }

    return retval;
}

/*****************************************************************************
*
* @description
* This function handles and processes any DMA controller interrupts.
* It works in conjuntion with the Interrupt API and user listener
* functions to manage interrupt-driven DMA transfers.  Before using
* this function, the user must set up a listener function using
* FDmaPs_setListener() for the relevant channel(s). When fully using
* the Interrupt API, this function should be called whenever a
* dmac interrupt occurs.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*
* @return   
*           FMSH_SUCCESS an interrupt was processed.
*           FMSH_FAILURE no interrupt was processed.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_irqHandler(FDmaPs_T *pDmac)
{
    int i, ch_index, retval;
    u32 reg, irq_active, mask;
    int callbackArg;
    FDmaPs_Param_T *param;
    FDmaPs_Instance_T *instance;
    FMSH_callback userCallback;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    instance = pDmac->instance;

    /* Assume an interrupt will be processed.  The return value will be*/
    /* set to FALSE if an active interrupt is not found.*/
    retval = FMSH_SUCCESS;

    /* read the interrupt status register*/
	irq_active = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_STATUS_INT_L_OFFSET);

    /* ERR INTERRUPT*/
    if (irq_active & DMAC_STATUSINT_L_ERR)
	{
		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_STATUS_ERR_L_OFFSET);

        /* find the active interrupt and start at the highest priority*/
        /* channel and work down to the lowest priority.*/
        for (i = 0; i < param->num_channels; i++)
		{
            mask = 1 << instance->ch_order[i];
            if (reg & mask)
			{
                ch_index = instance->ch_order[i];
                break;
            }
        }

        /* run the listener function*/
        FMSH_ASSERT(instance->ch[ch_index].userListener != NULL);
        userCallback = instance->ch[ch_index].userListener;
        callbackArg = Dmac_irq_err;
        userCallback(pDmac, callbackArg);

        /* clear the interrupt*/
		FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_CLEAR_ERR_L_OFFSET, mask);
    }
    /* TFR INTERRUPT*/
    else if (irq_active & DMAC_STATUSINT_L_TFR)
	{
		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_STATUS_TFR_L_OFFSET);
        
        /* find the active interrupt and start at the highest priority*/
        /* channel and work down to the lowest priority.*/
        for (i = 0; i < param->num_channels; i++)
		{
            mask = 1 << instance->ch_order[i];
            if (reg & mask)
			{
                ch_index = instance->ch_order[i];
                break;
            }
        }

        /* increment the block count*/
        instance->ch[ch_index].block_cnt++;

        /* run the callback function*/
        if (instance->ch[ch_index].userCallback != NULL)
		{
            userCallback = instance->ch[ch_index].userCallback;
            callbackArg = instance->ch[ch_index].block_cnt;
            userCallback(pDmac, callbackArg);
        }

        /* reset the source and destination states*/
        instance->ch[ch_index].src_state = Dmac_idle;
        instance->ch[ch_index].dst_state = Dmac_idle;

        /* reset the byte count*/
        instance->ch[ch_index].src_byte_cnt = 0;
        instance->ch[ch_index].dst_byte_cnt = 0;

		/* Disable all channel interrupts*/
		FDmaPs_disableChannelIrq(pDmac, instance->ch[ch_index].ch_num);

        /* clear any pending block/srcTran/dstTran interrupts*/
		FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_CLEAR_BLOCK_L_OFFSET, mask);
		FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_CLEAR_SRCTRAN_L_OFFSET, mask);
		FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_CLEAR_DSTTRAN_L_OFFSET, mask);

		/* Mask all channel interrupts*/
        FDmaPs_maskIrq(pDmac, instance->ch[ch_index].ch_num, Dmac_irq_all);
    }
    /* BLOCK INTERRUPT*/
    else if (irq_active & DMAC_STATUSINT_L_BLOCK)
	{
		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_STATUS_BLOCK_L_OFFSET);

        /* find the active interrupt and start at the highest priority*/
        /* channel and work down to the lowest priority.*/
        for (i = 0; i < param->num_channels; i++)
		{
            mask = 1 << instance->ch_order[i];
            if (reg & mask)
			{
                ch_index = instance->ch_order[i];
                break;
            }
        }

        /* increment the block count*/
        instance->ch[ch_index].block_cnt++;

        /* If the next block is the last block we must*/
        /* clear the reload bit in the CFG register for*/
        /* certain transfer types.*/
        if (instance->ch[ch_index].block_cnt == instance->ch[ch_index].total_blocks-1)
		{
            switch (instance->ch[ch_index].trans_type)
			{
                case Dmac_transfer_row2 :
                case Dmac_transfer_row9 :
                    FDmaPs_setReload(pDmac, instance->ch[ch_index].ch_num, Dmac_dst, FMSH_clear);
                    break;
                case Dmac_transfer_row3 :
                case Dmac_transfer_row7 :
                    FDmaPs_setReload(pDmac, instance->ch[ch_index].ch_num, Dmac_src, FMSH_clear);
                    break;
                case Dmac_transfer_row4 :
                    FDmaPs_setReload(pDmac, instance->ch[ch_index].ch_num, Dmac_src_dst, FMSH_clear);
                    break;
                default :
                    break;
            }
		}

        /* run the listener function*/
        FMSH_ASSERT(instance->ch[ch_index].userListener != NULL);
        userCallback = instance->ch[ch_index].userListener;
        callbackArg = Dmac_irq_block;
        userCallback(pDmac, callbackArg);

        /* reset the source and destination states*/
        instance->ch[ch_index].src_state = Dmac_burst_region;
        instance->ch[ch_index].dst_state = Dmac_burst_region;

        /* reset the byte count*/
        instance->ch[ch_index].src_byte_cnt = 0;
        instance->ch[ch_index].dst_byte_cnt = 0;

        /* clear any pending srcTran/dstTran interrupts*/
		FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_CLEAR_SRCTRAN_L_OFFSET, mask);
		FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_CLEAR_DSTTRAN_L_OFFSET, mask);
        
		/* clear the block interrupt*/
		FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_CLEAR_BLOCK_L_OFFSET, mask);
    }
    /* SRCTRAN INTERRUPT*/
    else if (irq_active & DMAC_STATUSINT_L_SRCTRAN)
	{
		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_STATUS_SRCTRAN_L_OFFSET);

        /* find the active interrupt and start at the highest priority*/
        /* channel and work down to the lowest priority.*/
        for (i = 0; i < param->num_channels; i++)
		{
            mask = 1 << instance->ch_order[i];
            if (reg & mask)
			{
                ch_index = instance->ch_order[i];
                break;
            }
        }

        /* increment the bytes received count*/
        if (instance->ch[ch_index].src_state == Dmac_single_region)
		{
            instance->ch[ch_index].src_byte_cnt += instance->ch[ch_index].src_single_inc;
        }
        if (instance->ch[ch_index].src_state == Dmac_burst_region)
		{
            instance->ch[ch_index].src_byte_cnt += instance->ch[ch_index].src_burst_inc;
        }

        /* run the listener function*/
        FMSH_ASSERT(instance->ch[ch_index].userListener != NULL);
        userCallback = instance->ch[ch_index].userListener;
        callbackArg = Dmac_irq_srctran;
        userCallback(pDmac, callbackArg);

        /* clear the interrupt*/
		FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_CLEAR_SRCTRAN_L_OFFSET, mask);
    }
    /* DSTTRAN INTERRUPT*/
    else if (irq_active & DMAC_STATUSINT_L_DSTTRAN)
	{
		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_INT_STATUS_DSTTRAN_L_OFFSET);

        /* find the active interrupt and start at the highest priority*/
        /* channel and work down to the lowest priority.*/
        for (i = 0; i < param->num_channels; i++)
		{
            mask = 1 << instance->ch_order[i];
            if (reg & mask)
			{
                ch_index = instance->ch_order[i];
                break;
            }
        }

        /* increment the bytes received count*/
        if (instance->ch[ch_index].dst_state == Dmac_single_region)
		{
        	instance->ch[ch_index].dst_byte_cnt += instance->ch[ch_index].dst_single_inc;
        }
        if (instance->ch[ch_index].dst_state == Dmac_burst_region)
		{
        	instance->ch[ch_index].dst_byte_cnt += instance->ch[ch_index].dst_burst_inc;
        }

        /* run the listener function*/
        FMSH_ASSERT(instance->ch[ch_index].userListener != NULL);
        userCallback = instance->ch[ch_index].userListener;
        callbackArg = Dmac_irq_dsttran;
        userCallback(pDmac, callbackArg);

        /* clear the interrupt*/
		FMSH_WriteReg(pDmac->config.BaseAddress, DMAC_INT_CLEAR_DSTTRAN_L_OFFSET, mask);
    }
    else
	{
        /* If we've reached this point, either the enabling and*/
        /* disabling of DW_ahb_dmac interrupts is not being handled*/
        /* properly or this function is being called unnecessarily.*/
        retval = FMSH_FAILURE;
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function is used to start an interrupt-driven transfer on a
* DMA channel.  Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument.
*
* The function enables DMA channel interrupts and stores
* information needed by the IRQ Handler to control the transfer.
* The DMA channel is also enabled to begin the DMA transfer. The
* following channel interrupts are enabled and unmasked by this function:
*
*    IntTfr - transfer complete interrupt
*    IntBlock - block transfer complete interrupt
*    IntErr - error response on the AMBA AHB bus
*
* If software handshaking is used on the source and the source
* device is a peripheral, the following interrupt is unmasked. If
* the transfer set up does not match that described and the user
* wants to use this interrupt, the user should unmask the
* interrupt using the FDmaPs_unmaskIrq() function prior to calling
* this function.
*  
*    IntSrcTran - source burst/single tranfer completed
*
* If software handshaking is used on the destination and the
* destination device is a peripheral, the following interrupt
* is unmasked. If the transfer setup does not match that described
* and the user wants to use this interrupt, the user should
* unmask the interrupt using the FDmaPs_unmaskIrq() function prior
* to calling this function.
*
*    IntDstTran - destination burst/single tranfer completed
*  
* All channel interrupts are masked and disabled on completion of
* the DMA transfer.
*
* If the number of blocks that make up the DMA transfer is not known,
* the user should enter 0 for the num_blocks argument. The user's
* listener function is called by the FDmaPs_irqHandler() function
* each time a block interrupt occurs. The user can use the
* FDmaPs_getBlockCount() API function to fetch the number of blocks
* completed by the DMA Controller from within the listener function.
* When the total number of blocks is known, the user should call the
* FDmaPs_nextBlockIsLast() function also from within the Listener function.
* The listener function has two arguments, the DMAC device handle
* and the interrupt type (FDmaPs_irq).
*  
* At the end of the DMA transfer, the FDmaPs_irqHandler() calls
* the user's callback function if the user has specified one. The
* callback function has two arguments: the DMAC device handle and
* the number of blocks transferred by the DMA Controller.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           num_blocks Number of blocks in the DMA transfer.
*           cb_func User callback function (can be NULL) - called by ISR.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_startTransfer(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        int    num_blocks,
        FMSH_callback cb_func)
{
    int errorCode;
    u8 ch_index;
    FDmaPs_Instance_T *instance;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    instance = pDmac->instance;

    /* Update the channel instance*/
    ch_index = FDmaPs_getChannelIndex(ch_num);
	
    /* Allow only ONE channel to be specified*/
    if (ch_index == DMAC_MAX_CHANNELS)
	{
        errorCode = -FMSH_ECHRNG;
    }
	else
	{
        /* Check for valid channel number and not busy*/
        errorCode = FDmaPs_checkChannelBusy(pDmac, ch_num);
    }

    if (errorCode == 0)
	{
        /* Disable the channels interrupts*/
        errorCode = FDmaPs_disableChannelIrq(pDmac, ch_num);
    }

    if (errorCode == 0)
	{
		/* set the call back function, the number of blocks*/
		/* in the transfer and the source and destination states.*/
        instance->ch[ch_index].userCallback = cb_func;
        instance->ch[ch_index].total_blocks = num_blocks;
        instance->ch[ch_index].src_state = Dmac_burst_region;
        instance->ch[ch_index].dst_state = Dmac_burst_region;

		/* store the transfer type.*/
		instance->ch[ch_index].trans_type = FDmaPs_getTransferType(pDmac, ch_num);
	
		/* initiatize the block / byte count variables*/
        instance->ch[ch_index].block_cnt = 0;
        instance->ch[ch_index].src_byte_cnt = 0;
        instance->ch[ch_index].dst_byte_cnt = 0;

		/* set the increment value for the source when in*/
		/* the single transaction region.*/
        instance->ch[ch_index].src_single_inc = (POW2(FDmaPs_getTransWidth(pDmac, ch_num, Dmac_src) + 3) / 8);
	
		/* set the increment value for the source when NOT in*/
		/* the single transaction region.*/
        instance->ch[ch_index].src_burst_inc  = (instance->ch[ch_index].src_single_inc * 
        										POW2(FDmaPs_getBurstTransLength(pDmac, ch_num, Dmac_src) + 1));
	
		/* set the increment value for the destination when in*/
		/* the single transaction region.*/
        instance->ch[ch_index].dst_single_inc = (POW2(FDmaPs_getTransWidth(pDmac, ch_num, Dmac_dst) + 3) / 8);
	
		/* set the increment value for the destination when NOT in*/
		/* the single transaction region.*/
        instance->ch[ch_index].dst_burst_inc  = (instance->ch[ch_index].dst_single_inc *
												POW2(FDmaPs_getBurstTransLength(pDmac, ch_num, Dmac_dst) + 1));
	
        /* always want to unmask the tfr, block and err interrupts*/
        FDmaPs_unmaskIrq(pDmac, ch_num, Dmac_irq_tfr);
        FDmaPs_unmaskIrq(pDmac, ch_num, Dmac_irq_block);
        FDmaPs_unmaskIrq(pDmac, ch_num, Dmac_irq_err);

		instance->ch[ch_index].tt_fc = FDmaPs_getMemPeriphFlowCtl(pDmac, ch_num);

		/* Unmask the srctran interrupt if the the source is using*/
		/* software handshaking and the source device is a peripheral*/
		if (FDmaPs_getHandshakingMode(pDmac, ch_num, Dmac_src) == Dmac_hs_software) 
	    {
        	if (instance->ch[ch_index].tt_fc == Dmac_prf2mem_dma    ||
                instance->ch[ch_index].tt_fc == Dmac_prf2mem_prf    ||
                instance->ch[ch_index].tt_fc == Dmac_prf2prf_dma ||
                instance->ch[ch_index].tt_fc == Dmac_prf2prf_srcprf ||
                instance->ch[ch_index].tt_fc == Dmac_prf2prf_dstprf)
        	{
                FDmaPs_unmaskIrq(pDmac, ch_num, Dmac_irq_srctran);
	    	}
        }
	    
		/* Unmask the dsttran interrupt if the the destination is*/
		/* using software handshaking and the destination device is*/
		/* a perihperal*/
		if (FDmaPs_getHandshakingMode(pDmac, ch_num, Dmac_dst) == Dmac_hs_software)
		{
            if (instance->ch[ch_index].tt_fc == Dmac_mem2prf_dma    ||
                instance->ch[ch_index].tt_fc == Dmac_mem2prf_prf    ||
                instance->ch[ch_index].tt_fc == Dmac_prf2prf_dma ||
                instance->ch[ch_index].tt_fc == Dmac_prf2prf_srcprf ||
                instance->ch[ch_index].tt_fc == Dmac_prf2prf_dstprf)
            {
                FDmaPs_unmaskIrq(pDmac, ch_num, Dmac_irq_dsttran);
	    	}
		}
 
        /* Enable the channel interrupts for the type of transfer*/
        errorCode = FDmaPs_enableChannelIrq(pDmac, ch_num);

		if (errorCode == 0)
		{
            /* Enable the DMA channel*/
            errorCode = FDmaPs_enableChannel(pDmac, ch_num);
		}
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function is part of the interrupt-driven interface for the
* DMA controller driver. This function writes to the source
* software request registers on the DMA controller.
*
* This function is ONLY useful when the source device is a
* peripheral (non-memory) AND that source device is interfacing to
* the DMA controller via software handshaking. Under all other
* transfer conditions, this function should NOT be used.
*
* This function should ideally be called inside an ISR for the
* source peripheral device to indicate that it is ready for a DMA transfer.
*
* If the source peripheral is not the flow control device, the
* single and last arguments are ignored and should be left at 'FALSE'.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_index DMA channel index (0 to DMAC_MAX_CHANNELS).
*
*           The following arguments are only useful when the source
*           peripheral is the flow control device:
*  
*           single when 'TRUE' requests a single transfer.
*                  when 'FALSE' requests a burst transfer.
*           last when 'TRUE' the next single/burst transfer is the
*                last in the current block.
*                when 'FALSE' the next single/burst transfer is NOT
*                the last in the current block.
*
* @return   
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FDmaPs_sourceReady(
        FDmaPs_T *pDmac,
        unsigned ch_index,
        BOOL single, BOOL last)
{
    enum FDmaPs_channelNumber ch_num;
    FDmaPs_Instance_T *instance;
    FDmaPs_Param_T *param;

    param = pDmac->comp_param;
    instance = pDmac->instance;

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index < DMAC_MAX_CHANNELS);

    /* convert channel index to channel number*/
    ch_num = (enum FDmaPs_channelNumber)DMAC_CH_NUM(ch_index);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* Source peripheral is flow controller*/
    if (instance->ch[ch_index].tt_fc == Dmac_prf2mem_prf    ||
        instance->ch[ch_index].tt_fc == Dmac_prf2prf_srcprf)
    {
        if (single)
		{
            instance->ch[ch_index].src_state = Dmac_single_region;
            FDmaPs_setSoftwareRequest(pDmac, ch_num, Dmac_src, Dmac_single_request, FMSH_set);
        }
		else
		{
            instance->ch[ch_index].src_state = Dmac_burst_region;
            FDmaPs_setSoftwareRequest(pDmac, ch_num, Dmac_src, Dmac_single_request, FMSH_clear);
        }

        if (last)
		{
            FDmaPs_setSoftwareRequest(pDmac, ch_num, Dmac_src, Dmac_last_request, FMSH_set);
        }
		else
		{
            FDmaPs_setSoftwareRequest(pDmac, ch_num, Dmac_src, Dmac_last_request, FMSH_clear);
        }

        FDmaPs_setSoftwareRequest(pDmac, ch_num, Dmac_src, Dmac_request, FMSH_set);
    }
    else	/* Source peripheral is not the flow controller*/
	{
        if (instance->ch[ch_index].src_state == Dmac_single_region)
		{
            FDmaPs_setSoftwareRequest(pDmac, ch_num, Dmac_src, Dmac_single_request, FMSH_set);
        }

        if (instance->ch[ch_index].src_state == Dmac_burst_region)
		{
            FDmaPs_setSoftwareRequest(pDmac, ch_num, Dmac_src, Dmac_request, FMSH_set);
            FDmaPs_setSoftwareRequest(pDmac, ch_num, Dmac_src, Dmac_single_request, FMSH_set);
        }
    }

}

/*****************************************************************************
*
* @description
* This function is part of the interrupt-driven interface for the
* DMA controller driver. This function writes to the destination
* software request registers on the DMA controller.
*
* This function is ONLY useful when the destination device is a
* peripheral (non-memory) AND that destination device is
* interfacing to the DMA controller via software handshaking.
* Under all other transfer conditions, this function should NOT be used.
*
* This function should ideally be called inside an ISR for the
* destination peripheral device to indicate that it is ready for a
* DMA transfer.
*
* If the destination peripheral is not the flow control device, the
* single and last arguments are ignored and should be left at 'FALSE'.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_index DMA channel index (0 to DMAC_MAX_CHANNELS).
*
*           The following arguments are only useful when the destination
*           peripheral is the flow control device:
*  
*           single when 'TRUE' requests a single transfer.
*                  when 'FALSE' requests a burst transfer.
*           last when 'TRUE' the next single/burst transfer is the
*                last in the current block.
*                when 'FALSE' the next single/burst transfer is NOT
*                the last in the current block.
*
* @return   
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FDmaPs_destinationReady(
        FDmaPs_T *pDmac,
        unsigned ch_index,
        BOOL single, BOOL last)
{
    enum FDmaPs_channelNumber ch_num;
    FDmaPs_Instance_T *instance;
    FDmaPs_Param_T *param;

    param = pDmac->comp_param;
    instance = pDmac->instance;

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index < DMAC_MAX_CHANNELS);

    /* convert channel index to channel number*/
    ch_num = (enum FDmaPs_channelNumber)DMAC_CH_NUM(ch_index);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* Destination peripheral is flow controller*/
    if (instance->ch[ch_index].tt_fc == Dmac_mem2prf_prf    ||
        instance->ch[ch_index].tt_fc == Dmac_prf2prf_dstprf)
    {
        if (single)
		{
            instance->ch[ch_index].dst_state = Dmac_single_region;
            FDmaPs_setSoftwareRequest(pDmac, ch_num, Dmac_dst, Dmac_single_request, FMSH_set);
        }
		else
		{
            instance->ch[ch_index].dst_state = Dmac_burst_region;
            FDmaPs_setSoftwareRequest(pDmac, ch_num, Dmac_dst, Dmac_single_request, FMSH_clear);
        }

        if (last)
		{
            FDmaPs_setSoftwareRequest(pDmac, ch_num, Dmac_dst, Dmac_last_request, FMSH_set);
        }
		else
		{
            FDmaPs_setSoftwareRequest(pDmac, ch_num, Dmac_dst, Dmac_last_request, FMSH_clear);
        }

        FDmaPs_setSoftwareRequest(pDmac, ch_num, Dmac_dst, Dmac_request, FMSH_set);
    }
    else 	/* Destination peripheral is not the flow controller*/
	{
        if (instance->ch[ch_index].dst_state == Dmac_single_region)
		{
            FDmaPs_setSoftwareRequest(pDmac, ch_num, Dmac_dst, Dmac_single_request, FMSH_set);
        }

        if (instance->ch[ch_index].dst_state == Dmac_burst_region)
		{
            FDmaPs_setSoftwareRequest(pDmac, ch_num, Dmac_dst, Dmac_request, FMSH_set);
            FDmaPs_setSoftwareRequest(pDmac, ch_num, Dmac_dst, Dmac_single_request, FMSH_set);
        }
    }

}

/*****************************************************************************
*
* @description
* This function is part of the interrupt-driven interface for the
* DMA controller driver. The function is used to instruct the
* driver that subsequent transfers in a block are to be completed
* using single transfers.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument.
*  
* This function is ONLY useful when either the source or
* destination device is a peripheral (non-memory) AND that source or
* destination device is interfacing to the DMA controller via
* software handshaking. Under all other transfer conditions this
* function should NOT be used.
*  
* This function is only be needed if the source or destination
* transfer can enter a single transaction region.
*
* If the source or destination enters a single transaction region,
* the user has the choice of completing the block transaction using
* a single transfer (in which case this function should be called) 
* or completing the block transaction using a burst transfer, and
* allowing the DMA controller to early terminate.
* Care should be taken here to set the threshold levels in the
* peripheral device to match the requested transfer (single/burst).
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*
* @return   
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FDmaPs_setSingleRegion(
        FDmaPs_T *pDmac, 
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel)
{
    u8 ch_index;
    FDmaPs_Instance_T *instance;
    FDmaPs_Param_T *param;

    param = pDmac->comp_param;
    instance = pDmac->instance;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    if (sd_sel == Dmac_src || sd_sel == Dmac_src_dst)
	{
        instance->ch[ch_index].src_state = Dmac_single_region;
    }

    if (sd_sel == Dmac_dst || sd_sel == Dmac_src_dst)
	{
        instance->ch[ch_index].dst_state = Dmac_single_region;
    }
}

/*****************************************************************************
*
* @description
* This function is part of the interrupt-driven interface in
* the DMAC driver. This function is only needed for the
* special case where the number of blocks that make up the DMAC
* transfer is not known when the transfer is initiated. If this
* is the case, the user can monitor the block count in the listener
* function and call this function when the last block of the DMAC
* transfer is known. Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FDmaPs_nextBlockIsLast(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    u8 ch_index;
    FDmaPs_Instance_T *instance;
    FDmaPs_Param_T *param;

    param = pDmac->comp_param;
    instance = pDmac->instance;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    instance->ch[ch_index].total_blocks = instance->ch[ch_index].block_cnt + 1;
}

/*****************************************************************************
*
* @description
* This function is used to set up a listener function for the
* interrupt handler of the DMAC driver. The listener function is
* responsible for handling all interrupts that are not handled
* by the Driver Kit interrupt handler. A listener function need to be
* setup for each channel that is being used.
*
* Only 1 DMA channel can be specified for the ch_num argument. There
* is no need to clear any interrupts in the listener as this is
* handled automatically by the Driver Kit interrupt handlers. Note
* that when using the FDmaPs_irqHandler() interrupt handler, the
* Dmac_irq_tfr interrupt is never passed to the listener function.
* Instead, an optional user-provided callback function is called.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           userFunction function pointer to user listener function.
*
* @return   
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FDmaPs_setListener(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        FMSH_callback userFunction)
{
    u8 ch_index;
    FDmaPs_Instance_T *instance;
    FDmaPs_Param_T *param;

    FMSH_ASSERT(userFunction != NULL);

    param = pDmac->comp_param;
    instance = pDmac->instance;

    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    instance->ch[ch_index].userListener = userFunction;
}

/*****************************************************************************
*
* @description
* This function returns the number of blocks that a DMA
* channel has completed transferring. This function should only be
* used for interrupt driven transfers.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           Number of blocks completed by the DMA controller.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_getBlockCount(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    int retval;
    u8 ch_index;
    FDmaPs_Instance_T *instance;
    FDmaPs_Param_T *param;

    param = pDmac->comp_param;
    instance = pDmac->instance;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    retval = instance->ch[ch_index].block_cnt;

    return retval;
}

/*****************************************************************************
*
* @description
* This function returns the number of bytes within a DMA block
* that have been transferred by the DMA controller on the specified
* source or destination. This function should only be used for
* interrupt driven transfers, where the SRCTRAN or DSTTRAN
* interrupts are enabled and unmasked.
* Only 1 DMA channel can be specified for the
* FDmaPs_channelNumber argument. Only 1, source or destination,
* can be specified for the FDmaPs_srcDstSelect argument.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*           sd_sel Enumerated source/destination select.
*
* @return   
*           Number of bytes completed on source or destination.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_getBlockByteCount(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num,
        enum FDmaPs_srcDstSelect sd_sel)
{
    int retval;
    u8 ch_index;
    FDmaPs_Instance_T *instance;
    FDmaPs_Param_T *param;

    param = pDmac->comp_param;
    instance = pDmac->instance;
    ch_index = FDmaPs_getChannelIndex(ch_num);

    /* Allow only ONE channel to be specified*/
    FMSH_ASSERT(ch_index != DMAC_MAX_CHANNELS);

    /* Check that the specified channel is in range*/
    FMSH_ASSERT(!(DMAC_CH_MASK & ch_num));

    /* Allow only source OR destination*/
    FMSH_ASSERT(sd_sel == Dmac_src || sd_sel == Dmac_dst);

    if (sd_sel == Dmac_src)
	{
    	retval = instance->ch[ch_index].src_byte_cnt;
	}
    else
	{
    	retval = instance->ch[ch_index].dst_byte_cnt;
	}
	
    return retval;
}

/*****************************************************************************
*
* @description
* This functions resets the FDmaPs_Instance_T structure.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*
* @return   
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FDmaPs_resetInstance(FDmaPs_T *pDmac)
{
    int i;
    FDmaPs_Param_T *param;
    FDmaPs_Instance_T *instance;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    instance = pDmac->instance;

    for (i = 0; i < param->num_channels; i++)
	{
        switch (i)
		{
            case 0: instance->ch[i].ch_num = Dmac_channel0; 
					break;
            case 1: instance->ch[i].ch_num = Dmac_channel1; 
					break;
            case 2: instance->ch[i].ch_num = Dmac_channel2; 
					break;
            case 3: instance->ch[i].ch_num = Dmac_channel3; 
					break;
            case 4: instance->ch[i].ch_num = Dmac_channel4; 
					break;
            case 5: instance->ch[i].ch_num = Dmac_channel5; 
					break;
            case 6: instance->ch[i].ch_num = Dmac_channel6; 
					break;
            case 7: instance->ch[i].ch_num = Dmac_channel7; 
					break;
        }

        instance->ch[i].src_state      = Dmac_idle;
        instance->ch[i].dst_state      = Dmac_idle;
        instance->ch[i].block_cnt      = 0;
        instance->ch[i].total_blocks   = 0;
        instance->ch[i].src_byte_cnt   = 0;
        instance->ch[i].dst_byte_cnt   = 0;
        instance->ch[i].src_single_inc = 0;
        instance->ch[i].src_burst_inc  = 0;
        instance->ch[i].dst_single_inc = 0;
        instance->ch[i].dst_burst_inc  = 0;
        instance->ch[i].trans_type     = Dmac_transfer_row1;
        instance->ch[i].tt_fc          = Dmac_mem2mem_dma;
        instance->ch[i].userCallback  = NULL;
        instance->ch[i].userListener  = NULL;
    }

    /* Set the channel priority order*/
    FDmaPs_setChannelPriorityOrder(pDmac);

}

/*****************************************************************************
*
* @description
* This function attempts to automatically discover the hardware
* component parameters.
* This is usually controlled by the ADD_ENCODED_PARAMS coreConsultant
* parameter.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_autoCompParams(FDmaPs_T *pDmac)
{
    int retval = FMSH_SUCCESS;
    u32 reg, tmp;
	u32 baseAddr;
	u32 compVersion;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
	baseAddr = pDmac->config.BaseAddress;

    /* Only version 2.02a and greater support hardware parameter*/
    /* registers.*/
    compVersion = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_ID_REG_H_OFFSET);
	
    if ((FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_ID_REG_L_OFFSET) == FMSH_ahb_dmac) &&
        (compVersion >= 0x3230322A))
	{
        pDmac->comp_version = compVersion;

		reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_1_H_OFFSET);
		
		param->num_master_int = ((reg & DMAC_PARAM_NUM_MASTER_INT) >> 11) + 0x1;
		param->num_channels = ((reg & DMAC_PARAM_NUM_CHANNELS) >> 8) + 0x1;
		param->num_hs_int = (reg & DMAC_PARAM_NUM_HS_INT) >> 23;
		param->intr_io = (reg & DMAC_PARAM_INTR_IO) >> 1;
		param->mabrst = (reg & DMAC_PARAM_MABRST) >> 3;
		param->big_endian = (reg & DMAC_PARAM_BIG_ENDIAN);
		
        tmp = (reg & DMAC_PARAM_S_HDATA_WIDTH) >> 13;
        switch (tmp)
		{
            case 0 : param->s_hdata_width =  32; break;
            case 1 : param->s_hdata_width =  64; break;
            case 2 : param->s_hdata_width = 128; break;
            case 3 : param->s_hdata_width = 256; break;
        }
		
        tmp = (reg & DMAC_PARAM_M1_HDATA_WIDTH) >> 15;
        switch (tmp)
		{
            case 0 : param->m_hdata_width[0] =  32; break;
            case 1 : param->m_hdata_width[0] =  64; break;
            case 2 : param->m_hdata_width[0] = 128; break;
            case 3 : param->m_hdata_width[0] = 256; break;
        }
		
        tmp = (reg & DMAC_PARAM_M2_HDATA_WIDTH) >> 17;
        switch (tmp)
		{
            case 0 : param->m_hdata_width[1] =  32; break;
            case 1 : param->m_hdata_width[1] =  64; break;
            case 2 : param->m_hdata_width[1] = 128; break;
            case 3 : param->m_hdata_width[1] = 256; break;
        }
		
        tmp = (reg & DMAC_PARAM_M3_HDATA_WIDTH) >> 19;
        switch (tmp)
		{
            case 0 : param->m_hdata_width[2] =  32; break;
            case 1 : param->m_hdata_width[2] =  64; break;
            case 2 : param->m_hdata_width[2] = 128; break;
            case 3 : param->m_hdata_width[2] = 256; break;
        }
		
        tmp = (reg & DMAC_PARAM_M4_HDATA_WIDTH) >> 21;
        switch (tmp)
		{
            case 0 : param->m_hdata_width[3] =  32; break;
            case 1 : param->m_hdata_width[3] =  64; break;
            case 2 : param->m_hdata_width[3] = 128; break;
            case 3 : param->m_hdata_width[3] = 256; break;
        }
		
        param->encoded_params = (reg & DMAC_PARAM_ADD_ENCODED_PARAMS) >> 28;
        param->static_endian_select = (reg & DMAC_PARAM_STATIC_ENDIAN_SELECT) >> 29;

        /* Channel 0 */
        reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_1_L_OFFSET);
		tmp = reg & DMAC_PARAM_CH0_MAX_BLK_SIZE;
        switch (tmp)
		{
            case 0  : param->ch_max_blk_size[0] =    3; break;
            case 1  : param->ch_max_blk_size[0] =    7; break;
            case 2  : param->ch_max_blk_size[0] =   15; break;
            case 3  : param->ch_max_blk_size[0] =   31; break;
            case 4  : param->ch_max_blk_size[0] =   63; break;
            case 5  : param->ch_max_blk_size[0] =  127; break;
            case 6  : param->ch_max_blk_size[0] =  255; break;
            case 7  : param->ch_max_blk_size[0] =  511; break;
            case 8  : param->ch_max_blk_size[0] = 1023; break;
            case 9  : param->ch_max_blk_size[0] = 2047; break;
            case 10 : param->ch_max_blk_size[0] = 4095; break;
        }
		
		reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_2_H_OFFSET);
		param->ch_multi_blk_type[0] = (reg & DMAC_PARAM_CH0_MULTI_BLK_TYPE);
		
		reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_2_L_OFFSET);
		tmp = reg & DMAC_PARAM_CHX_DTW;
        switch (tmp)
		{
            case 0 : param->ch_dtw[0] =   0; break;
            case 1 : param->ch_dtw[0] =   8; break;
            case 2 : param->ch_dtw[0] =  16; break;
            case 3 : param->ch_dtw[0] =  32; break;
            case 4 : param->ch_dtw[0] =  64; break;
            case 5 : param->ch_dtw[0] = 128; break;
            case 6 : param->ch_dtw[0] = 256; break;
        }
		
		tmp = (reg & DMAC_PARAM_CHX_STW) >> 3;
        switch (tmp)
		{
            case 0 : param->ch_stw[0] =   0; break;
            case 1 : param->ch_stw[0] =   8; break;
            case 2 : param->ch_stw[0] =  16; break;
            case 3 : param->ch_stw[0] =  32; break;
            case 4 : param->ch_stw[0] =  64; break;
            case 5 : param->ch_stw[0] = 128; break;
            case 6 : param->ch_stw[0] = 256; break;
        }
		
		param->ch_stat_dst[0] = (reg & DMAC_PARAM_CHX_STAT_DST) >> 6;
		param->ch_stat_src[0] = (reg & DMAC_PARAM_CHX_STAT_SRC) >> 7;
		param->ch_dst_sca_en[0] = (reg & DMAC_PARAM_CHX_DST_SCA_EN) >> 8;
		param->ch_src_gat_en[0] = (reg & DMAC_PARAM_CHX_SRC_GAT_EN) >> 9;
		param->ch_lock_en[0] = (reg & DMAC_PARAM_CHX_LOCK_EN) >> 10;
		param->ch_multi_blk_en[0] = (reg & DMAC_PARAM_CHX_MULTI_BLK_EN) >> 11;
		param->ch_ctl_wb_en[0] = (reg & DMAC_PARAM_CHX_CTL_WB_EN) >> 12;
		param->ch_hc_llp[0] = (reg & DMAC_PARAM_CHX_HC_LLP) >> 13;
		param->ch_fc[0] = (reg & DMAC_PARAM_CHX_FC) >> 14;
		
		tmp = (reg & DMAC_PARAM_CHX_MAX_MULT_SIZE) >> 16;
        switch (tmp)
		{
            case 0 : param->ch_max_mult_size[0] =   4; break;
            case 1 : param->ch_max_mult_size[0] =   8; break;
            case 2 : param->ch_max_mult_size[0] =  16; break;
            case 3 : param->ch_max_mult_size[0] =  32; break;
            case 4 : param->ch_max_mult_size[0] =  64; break;
            case 5 : param->ch_max_mult_size[0] = 128; break;
            case 6 : param->ch_max_mult_size[0] = 256; break;
        }
		
		param->ch_dms[0] = (reg & DMAC_PARAM_CHX_DMS) >> 19;
		param->ch_lms[0] = (reg & DMAC_PARAM_CHX_LMS) >> 22;
		param->ch_sms[0] = (reg & DMAC_PARAM_CHX_SMS) >> 25;
		
		tmp = (reg & DMAC_PARAM_CHX_FIFO_DEPTH) >> 28;
        switch(tmp) {
            case 0 : param->ch_fifo_depth[0] =   4; break;
            case 1 : param->ch_fifo_depth[0] =   8; break;
            case 2 : param->ch_fifo_depth[0] =  16; break;
            case 3 : param->ch_fifo_depth[0] =  32; break;
            case 4 : param->ch_fifo_depth[0] =  64; break;
            case 5 : param->ch_fifo_depth[0] = 128; break;
            case 6 : param->ch_fifo_depth[0] = 256; break;
        }
		
        /* Channel 1 */
        if (param->num_channels >= 2)
		{
        	reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_1_L_OFFSET); 
			tmp = (reg & DMAC_PARAM_CH1_MAX_BLK_SIZE) >> 4;
            switch (tmp)
			{
                case 0  : param->ch_max_blk_size[1] =    3; break;
                case 1  : param->ch_max_blk_size[1] =    7; break;
                case 2  : param->ch_max_blk_size[1] =   15; break;
                case 3  : param->ch_max_blk_size[1] =   31; break;
                case 4  : param->ch_max_blk_size[1] =   63; break;
                case 5  : param->ch_max_blk_size[1] =  127; break;
                case 6  : param->ch_max_blk_size[1] =  255; break;
                case 7  : param->ch_max_blk_size[1] =  511; break;
                case 8  : param->ch_max_blk_size[1] = 1023; break;
                case 9  : param->ch_max_blk_size[1] = 2047; break;
                case 10 : param->ch_max_blk_size[1] = 4095; break;
            }
			reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_2_H_OFFSET);
			param->ch_multi_blk_type[1] = (reg & DMAC_PARAM_CH1_MULTI_BLK_TYPE) >> 4;
			
			reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_3_H_OFFSET);
			tmp = reg & DMAC_PARAM_CHX_DTW;
            switch (tmp)
			{
                case 0 : param->ch_dtw[1] =   0; break;
                case 1 : param->ch_dtw[1] =   8; break;
                case 2 : param->ch_dtw[1] =  16; break;
                case 3 : param->ch_dtw[1] =  32; break;
                case 4 : param->ch_dtw[1] =  64; break;
                case 5 : param->ch_dtw[1] = 128; break;
                case 6 : param->ch_dtw[1] = 256; break;
            }
			tmp = (reg & DMAC_PARAM_CHX_STW) >> 3;
            switch (tmp)
			{
                case 0 : param->ch_stw[1] =   0; break;
                case 1 : param->ch_stw[1] =   8; break;
                case 2 : param->ch_stw[1] =  16; break;
                case 3 : param->ch_stw[1] =  32; break;
                case 4 : param->ch_stw[1] =  64; break;
                case 5 : param->ch_stw[1] = 128; break;
                case 6 : param->ch_stw[1] = 256; break;
            }
			
			param->ch_stat_dst[1] = (reg & DMAC_PARAM_CHX_STAT_DST) >> 6;
			param->ch_stat_src[1] = (reg & DMAC_PARAM_CHX_STAT_SRC) >> 7;
			param->ch_dst_sca_en[1] = (reg & DMAC_PARAM_CHX_DST_SCA_EN) >> 8;
			param->ch_src_gat_en[1] = (reg & DMAC_PARAM_CHX_SRC_GAT_EN) >> 9;
			param->ch_lock_en[1] = (reg & DMAC_PARAM_CHX_LOCK_EN) >> 10;
			param->ch_multi_blk_en[1] = (reg & DMAC_PARAM_CHX_MULTI_BLK_EN) >> 11;
			param->ch_ctl_wb_en[1] = (reg & DMAC_PARAM_CHX_CTL_WB_EN) >> 12;
			param->ch_hc_llp[1] = (reg & DMAC_PARAM_CHX_HC_LLP) >> 13;
			param->ch_fc[1] = (reg & DMAC_PARAM_CHX_FC) >> 14;
			
			tmp = (reg & DMAC_PARAM_CHX_MAX_MULT_SIZE) >> 16;
            switch (tmp)
			{
                case 0 : param->ch_max_mult_size[1] =   4; break;
                case 1 : param->ch_max_mult_size[1] =   8; break;
                case 2 : param->ch_max_mult_size[1] =  16; break;
                case 3 : param->ch_max_mult_size[1] =  32; break;
                case 4 : param->ch_max_mult_size[1] =  64; break;
                case 5 : param->ch_max_mult_size[1] = 128; break;
                case 6 : param->ch_max_mult_size[1] = 256; break;
            }
			
			param->ch_dms[1] = (reg & DMAC_PARAM_CHX_DMS) >> 19;
			param->ch_lms[1] = (reg & DMAC_PARAM_CHX_LMS) >> 22;
			param->ch_sms[1] = (reg & DMAC_PARAM_CHX_SMS) >> 25;
			
			tmp = (reg & DMAC_PARAM_CHX_FIFO_DEPTH) >> 28;
            switch (tmp)
			{
	            case 0 : param->ch_fifo_depth[1] =   4; break;
	            case 1 : param->ch_fifo_depth[1] =   8; break;
	            case 2 : param->ch_fifo_depth[1] =  16; break;
	            case 3 : param->ch_fifo_depth[1] =  32; break;
	            case 4 : param->ch_fifo_depth[1] =  64; break;
	            case 5 : param->ch_fifo_depth[1] = 128; break;
	            case 6 : param->ch_fifo_depth[1] = 256; break;
            }
        }

        /* Channel 2 */
        if(param->num_channels >= 3)
		{
        	reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_1_L_OFFSET);
			tmp = (reg & DMAC_PARAM_CH2_MAX_BLK_SIZE) >> 8;
            switch (tmp)
			{
                case 0  : param->ch_max_blk_size[2] =    3; break;
                case 1  : param->ch_max_blk_size[2] =    7; break;
                case 2  : param->ch_max_blk_size[2] =   15; break;
                case 3  : param->ch_max_blk_size[2] =   31; break;
                case 4  : param->ch_max_blk_size[2] =   63; break;
                case 5  : param->ch_max_blk_size[2] =  127; break;
                case 6  : param->ch_max_blk_size[2] =  255; break;
                case 7  : param->ch_max_blk_size[2] =  511; break;
                case 8  : param->ch_max_blk_size[2] = 1023; break;
                case 9  : param->ch_max_blk_size[2] = 2047; break;
                case 10 : param->ch_max_blk_size[2] = 4095; break;
            }
			reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_2_H_OFFSET);
			param->ch_multi_blk_type[2] = (reg & DMAC_PARAM_CH2_MULTI_BLK_TYPE) >> 8;
			
			reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_3_L_OFFSET);
			tmp = reg & DMAC_PARAM_CHX_DTW;
            switch (tmp)
			{
                case 0 : param->ch_dtw[2] =   0; break;
                case 1 : param->ch_dtw[2] =   8; break;
                case 2 : param->ch_dtw[2] =  16; break;
                case 3 : param->ch_dtw[2] =  32; break;
                case 4 : param->ch_dtw[2] =  64; break;
                case 5 : param->ch_dtw[2] = 128; break;
                case 6 : param->ch_dtw[2] = 256; break;
            }
			tmp = (reg & DMAC_PARAM_CHX_STW) >> 3;
            switch (tmp)
			{
                case 0 : param->ch_stw[2] =   0; break;
                case 1 : param->ch_stw[2] =   8; break;
                case 2 : param->ch_stw[2] =  16; break;
                case 3 : param->ch_stw[2] =  32; break;
                case 4 : param->ch_stw[2] =  64; break;
                case 5 : param->ch_stw[2] = 128; break;
                case 6 : param->ch_stw[2] = 256; break;
            }
			
			param->ch_stat_dst[2] = (reg & DMAC_PARAM_CHX_STAT_DST) >> 6;
			param->ch_stat_src[2] = (reg & DMAC_PARAM_CHX_STAT_SRC) >> 7;
			param->ch_dst_sca_en[2] = (reg & DMAC_PARAM_CHX_DST_SCA_EN) >> 8;
			param->ch_src_gat_en[2] = (reg & DMAC_PARAM_CHX_SRC_GAT_EN) >>9;
			param->ch_lock_en[2] = (reg & DMAC_PARAM_CHX_LOCK_EN) >> 10;
			param->ch_multi_blk_en[2] = (reg & DMAC_PARAM_CHX_MULTI_BLK_EN) >> 11;
			param->ch_ctl_wb_en[2] = (reg & DMAC_PARAM_CHX_CTL_WB_EN) >> 12;
			param->ch_hc_llp[2] = (reg & DMAC_PARAM_CHX_HC_LLP) >> 13;
			param->ch_fc[2] = (reg & DMAC_PARAM_CHX_FC) >> 14;
			
			tmp = (reg & DMAC_PARAM_CHX_MAX_MULT_SIZE) >> 16;
            switch (tmp)
			{
                case 0 : param->ch_max_mult_size[2] =   4; break;
                case 1 : param->ch_max_mult_size[2] =   8; break;
                case 2 : param->ch_max_mult_size[2] =  16; break;
                case 3 : param->ch_max_mult_size[2] =  32; break;
                case 4 : param->ch_max_mult_size[2] =  64; break;
                case 5 : param->ch_max_mult_size[2] = 128; break;
                case 6 : param->ch_max_mult_size[2] = 256; break;
            }
			
			param->ch_dms[2] = (reg & DMAC_PARAM_CHX_DMS) >> 19;
			param->ch_lms[2] = (reg & DMAC_PARAM_CHX_LMS) >> 22;
			param->ch_sms[2] = (reg & DMAC_PARAM_CHX_SMS) >> 25;
			
			tmp = (reg & DMAC_PARAM_CHX_FIFO_DEPTH) >> 28;
            switch (tmp)
			{
	            case 0 : param->ch_fifo_depth[2] =   4; break;
	            case 1 : param->ch_fifo_depth[2] =   8; break;
	            case 2 : param->ch_fifo_depth[2] =  16; break;
	            case 3 : param->ch_fifo_depth[2] =  32; break;
	            case 4 : param->ch_fifo_depth[2] =  64; break;
	            case 5 : param->ch_fifo_depth[2] = 128; break;
	            case 6 : param->ch_fifo_depth[2] = 256; break;
            }
        }

        /* Channel 3 */
        if(param->num_channels >= 4) {
        	reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_1_L_OFFSET);
			tmp = (reg & DMAC_PARAM_CH3_MAX_BLK_SIZE) >> 12;
            switch (tmp)
			{
                case 0  : param->ch_max_blk_size[3] =    3; break;
                case 1  : param->ch_max_blk_size[3] =    7; break;
                case 2  : param->ch_max_blk_size[3] =   15; break;
                case 3  : param->ch_max_blk_size[3] =   31; break;
                case 4  : param->ch_max_blk_size[3] =   63; break;
                case 5  : param->ch_max_blk_size[3] =  127; break;
                case 6  : param->ch_max_blk_size[3] =  255; break;
                case 7  : param->ch_max_blk_size[3] =  511; break;
                case 8  : param->ch_max_blk_size[3] = 1023; break;
                case 9  : param->ch_max_blk_size[3] = 2047; break;
                case 10 : param->ch_max_blk_size[3] = 4095; break;
            }
            reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_2_H_OFFSET);
			param->ch_multi_blk_type[3] = (reg & DMAC_PARAM_CH3_MULTI_BLK_TYPE) >> 12;
			
			reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_4_H_OFFSET);
			tmp = reg & DMAC_PARAM_CHX_DTW;
            switch (tmp)
			{
                case 0 : param->ch_dtw[3] =   0; break;
                case 1 : param->ch_dtw[3] =   8; break;
                case 2 : param->ch_dtw[3] =  16; break;
                case 3 : param->ch_dtw[3] =  32; break;
                case 4 : param->ch_dtw[3] =  64; break;
                case 5 : param->ch_dtw[3] = 128; break;
                case 6 : param->ch_dtw[3] = 256; break;
            }
			tmp = (reg & DMAC_PARAM_CHX_STW) >> 3;
            switch (tmp)
			{
                case 0 : param->ch_stw[3] =   0; break;
                case 1 : param->ch_stw[3] =   8; break;
                case 2 : param->ch_stw[3] =  16; break;
                case 3 : param->ch_stw[3] =  32; break;
                case 4 : param->ch_stw[3] =  64; break;
                case 5 : param->ch_stw[3] = 128; break;
                case 6 : param->ch_stw[3] = 256; break;
            }
			param->ch_stat_dst[3] = (reg & DMAC_PARAM_CHX_STAT_DST) >> 6;
			param->ch_stat_src[3] = (reg & DMAC_PARAM_CHX_STAT_SRC) >> 7;
			param->ch_dst_sca_en[3] = (reg & DMAC_PARAM_CHX_DST_SCA_EN) >> 8;
			param->ch_src_gat_en[3] = (reg & DMAC_PARAM_CHX_SRC_GAT_EN) >> 9;
			param->ch_lock_en[3] = (reg & DMAC_PARAM_CHX_LOCK_EN) >> 10;
			param->ch_multi_blk_en[3] = (reg & DMAC_PARAM_CHX_MULTI_BLK_EN) >> 11;
			param->ch_ctl_wb_en[3] = (reg & DMAC_PARAM_CHX_CTL_WB_EN) >> 12;
			param->ch_hc_llp[3] = (reg & DMAC_PARAM_CHX_HC_LLP) >> 13;
			param->ch_fc[3] = (reg & DMAC_PARAM_CHX_FC) >> 14;
			
			tmp = (reg & DMAC_PARAM_CHX_MAX_MULT_SIZE) >> 16;
            switch (tmp)
			{
                case 0 : param->ch_max_mult_size[3] =   4; break;
                case 1 : param->ch_max_mult_size[3] =   8; break;
                case 2 : param->ch_max_mult_size[3] =  16; break;
                case 3 : param->ch_max_mult_size[3] =  32; break;
                case 4 : param->ch_max_mult_size[3] =  64; break;
                case 5 : param->ch_max_mult_size[3] = 128; break;
                case 6 : param->ch_max_mult_size[3] = 256; break;
            }
			
			param->ch_dms[3] = (reg & DMAC_PARAM_CHX_DMS) >> 19;
			param->ch_lms[3] = (reg & DMAC_PARAM_CHX_LMS) >> 22;
			param->ch_sms[3] = (reg & DMAC_PARAM_CHX_SMS) >> 25;
			
			tmp = (reg & DMAC_PARAM_CHX_FIFO_DEPTH) >> 28;
            switch (tmp)
			{
	            case 0 : param->ch_fifo_depth[3] =   4; break;
	            case 1 : param->ch_fifo_depth[3] =   8; break;
	            case 2 : param->ch_fifo_depth[3] =  16; break;
	            case 3 : param->ch_fifo_depth[3] =  32; break;
	            case 4 : param->ch_fifo_depth[3] =  64; break;
	            case 5 : param->ch_fifo_depth[3] = 128; break;
	            case 6 : param->ch_fifo_depth[3] = 256; break;
            }
        }

        /* Channel 4 */
        if(param->num_channels >= 5) 
		{
        	reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_1_L_OFFSET);
			tmp = (reg & DMAC_PARAM_CH4_MAX_BLK_SIZE) >> 16;
            switch (tmp)
			{
                case 0  : param->ch_max_blk_size[4] =    3; break;
                case 1  : param->ch_max_blk_size[4] =    7; break;
                case 2  : param->ch_max_blk_size[4] =   15; break;
                case 3  : param->ch_max_blk_size[4] =   31; break;
                case 4  : param->ch_max_blk_size[4] =   63; break;
                case 5  : param->ch_max_blk_size[4] =  127; break;
                case 6  : param->ch_max_blk_size[4] =  255; break;
                case 7  : param->ch_max_blk_size[4] =  511; break;
                case 8  : param->ch_max_blk_size[4] = 1023; break;
                case 9  : param->ch_max_blk_size[4] = 2047; break;
                case 10 : param->ch_max_blk_size[4] = 4095; break;
            }
			reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_2_H_OFFSET);
			param->ch_multi_blk_type[4] = (reg & DMAC_PARAM_CH4_MULTI_BLK_TYPE) >> 16;
			
			reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_4_L_OFFSET);
			tmp = reg & DMAC_PARAM_CHX_DTW;
            switch (tmp)
			{
                case 0 : param->ch_dtw[4] =   0; break;
                case 1 : param->ch_dtw[4] =   8; break;
                case 2 : param->ch_dtw[4] =  16; break;
                case 3 : param->ch_dtw[4] =  32; break;
                case 4 : param->ch_dtw[4] =  64; break;
                case 5 : param->ch_dtw[4] = 128; break;
                case 6 : param->ch_dtw[4] = 256; break;
            }
			tmp = (reg & DMAC_PARAM_CHX_STW) >> 3;
            switch (tmp)
			{
                case 0 : param->ch_stw[4] =   0; break;
                case 1 : param->ch_stw[4] =   8; break;
                case 2 : param->ch_stw[4] =  16; break;
                case 3 : param->ch_stw[4] =  32; break;
                case 4 : param->ch_stw[4] =  64; break;
                case 5 : param->ch_stw[4] = 128; break;
                case 6 : param->ch_stw[4] = 256; break;
            }
			
			param->ch_stat_dst[4] = (reg & DMAC_PARAM_CHX_STAT_DST) >> 6;
			param->ch_stat_src[4] = (reg & DMAC_PARAM_CHX_STAT_SRC) >> 7;
			param->ch_dst_sca_en[4] = (reg & DMAC_PARAM_CHX_DST_SCA_EN) >> 8;
			param->ch_src_gat_en[4] = (reg & DMAC_PARAM_CHX_SRC_GAT_EN) >> 9;
			param->ch_lock_en[4] = (reg & DMAC_PARAM_CHX_LOCK_EN) >> 10;
			param->ch_multi_blk_en[4] = (reg & DMAC_PARAM_CHX_MULTI_BLK_EN) >> 11;
			param->ch_ctl_wb_en[4] = (reg & DMAC_PARAM_CHX_CTL_WB_EN) >> 12;
			param->ch_hc_llp[4] = (reg & DMAC_PARAM_CHX_HC_LLP) >> 13;
			param->ch_fc[4] = (reg & DMAC_PARAM_CHX_FC) >> 14;
			
			tmp = (reg & DMAC_PARAM_CHX_MAX_MULT_SIZE) >> 16;
            switch (tmp)
			{
                case 0 : param->ch_max_mult_size[4] =   4; break;
                case 1 : param->ch_max_mult_size[4] =   8; break;
                case 2 : param->ch_max_mult_size[4] =  16; break;
                case 3 : param->ch_max_mult_size[4] =  32; break;
                case 4 : param->ch_max_mult_size[4] =  64; break;
                case 5 : param->ch_max_mult_size[4] = 128; break;
                case 6 : param->ch_max_mult_size[4] = 256; break;
            }
			
			param->ch_dms[4] = (reg & DMAC_PARAM_CHX_DMS) >> 19;
			param->ch_lms[4] = (reg & DMAC_PARAM_CHX_LMS) >> 22;
			param->ch_sms[4] = (reg & DMAC_PARAM_CHX_SMS) >> 25;
			
			tmp = (reg & DMAC_PARAM_CHX_FIFO_DEPTH) >> 28;
            switch (tmp)
			{
	            case 0 : param->ch_fifo_depth[4] =   4; break;
	            case 1 : param->ch_fifo_depth[4] =   8; break;
	            case 2 : param->ch_fifo_depth[4] =  16; break;
	            case 3 : param->ch_fifo_depth[4] =  32; break;
	            case 4 : param->ch_fifo_depth[4] =  64; break;
	            case 5 : param->ch_fifo_depth[4] = 128; break;
	            case 6 : param->ch_fifo_depth[4] = 256; break;
            }
        }

        /* Channel 5 */
        if(param->num_channels >= 6) 
		{
			reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_1_L_OFFSET);
			tmp = (reg & DMAC_PARAM_CH5_MAX_BLK_SIZE) >> 20;
            switch (tmp)
			{
                case 0  : param->ch_max_blk_size[5] =    3; break;
                case 1  : param->ch_max_blk_size[5] =    7; break;
                case 2  : param->ch_max_blk_size[5] =   15; break;
                case 3  : param->ch_max_blk_size[5] =   31; break;
                case 4  : param->ch_max_blk_size[5] =   63; break;
                case 5  : param->ch_max_blk_size[5] =  127; break;
                case 6  : param->ch_max_blk_size[5] =  255; break;
                case 7  : param->ch_max_blk_size[5] =  511; break;
                case 8  : param->ch_max_blk_size[5] = 1023; break;
                case 9  : param->ch_max_blk_size[5] = 2047; break;
                case 10 : param->ch_max_blk_size[5] = 4095; break;
            }
			
			reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_2_H_OFFSET);
			param->ch_multi_blk_type[5] = (reg & DMAC_PARAM_CH5_MULTI_BLK_TYPE) >> 20;
			
			reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_5_H_OFFSET);
			tmp = reg & DMAC_PARAM_CHX_DTW;
            switch (tmp)
			{
                case 0 : param->ch_dtw[5] =   0; break;
                case 1 : param->ch_dtw[5] =   8; break;
                case 2 : param->ch_dtw[5] =  16; break;
                case 3 : param->ch_dtw[5] =  32; break;
                case 4 : param->ch_dtw[5] =  64; break;
                case 5 : param->ch_dtw[5] = 128; break;
                case 6 : param->ch_dtw[5] = 256; break;
            }
			tmp = (reg & DMAC_PARAM_CHX_STW) >> 3;
            switch (tmp)
			{
                case 0 : param->ch_stw[5] =   0; break;
                case 1 : param->ch_stw[5] =   8; break;
                case 2 : param->ch_stw[5] =  16; break;
                case 3 : param->ch_stw[5] =  32; break;
                case 4 : param->ch_stw[5] =  64; break;
                case 5 : param->ch_stw[5] = 128; break;
                case 6 : param->ch_stw[5] = 256; break;
            }
            
			param->ch_stat_dst[5] = (reg & DMAC_PARAM_CHX_STAT_DST) >> 6;
			param->ch_stat_src[5] = (reg & DMAC_PARAM_CHX_STAT_SRC) >> 7;
			param->ch_dst_sca_en[5] = (reg & DMAC_PARAM_CHX_DST_SCA_EN) >> 8;
			param->ch_src_gat_en[5] = (reg & DMAC_PARAM_CHX_SRC_GAT_EN) >> 9;
			param->ch_lock_en[5] = (reg & DMAC_PARAM_CHX_LOCK_EN) >> 10;
			param->ch_multi_blk_en[5] = (reg & DMAC_PARAM_CHX_MULTI_BLK_EN) >> 11;
			param->ch_ctl_wb_en[5] = (reg & DMAC_PARAM_CHX_CTL_WB_EN) >> 12;
			param->ch_hc_llp[5] = (reg & DMAC_PARAM_CHX_HC_LLP) >> 13;
			param->ch_fc[5] = (reg & DMAC_PARAM_CHX_FC) >> 14;
			
			tmp = (reg & DMAC_PARAM_CHX_MAX_MULT_SIZE) >> 16;
            switch (tmp)
			{
                case 0 : param->ch_max_mult_size[5] =   4; break;
                case 1 : param->ch_max_mult_size[5] =   8; break;
                case 2 : param->ch_max_mult_size[5] =  16; break;
                case 3 : param->ch_max_mult_size[5] =  32; break;
                case 4 : param->ch_max_mult_size[5] =  64; break;
                case 5 : param->ch_max_mult_size[5] = 128; break;
                case 6 : param->ch_max_mult_size[5] = 256; break;
            }
			
			param->ch_dms[5] = (reg & DMAC_PARAM_CHX_DMS) >> 19;
			param->ch_lms[5] = (reg & DMAC_PARAM_CHX_LMS) >> 22;
			param->ch_sms[5] = (reg & DMAC_PARAM_CHX_SMS) >> 25;
			
			tmp = (reg & DMAC_PARAM_CHX_FIFO_DEPTH) >> 28;
            switch (tmp)
			{
	            case 0 : param->ch_fifo_depth[5] =   4; break;
	            case 1 : param->ch_fifo_depth[5] =   8; break;
	            case 2 : param->ch_fifo_depth[5] =  16; break;
	            case 3 : param->ch_fifo_depth[5] =  32; break;
	            case 4 : param->ch_fifo_depth[5] =  64; break;
	            case 5 : param->ch_fifo_depth[5] = 128; break;
	            case 6 : param->ch_fifo_depth[5] = 256; break;
            }
        }

        /* Channel 6 */
        if(param->num_channels >= 7) 
		{
			reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_1_L_OFFSET);
			tmp = (reg & DMAC_PARAM_CH6_MAX_BLK_SIZE) >> 24;
            switch (tmp)
			{
                case 0  : param->ch_max_blk_size[6] =    3; break;
                case 1  : param->ch_max_blk_size[6] =    7; break;
                case 2  : param->ch_max_blk_size[6] =   15; break;
                case 3  : param->ch_max_blk_size[6] =   31; break;
                case 4  : param->ch_max_blk_size[6] =   63; break;
                case 5  : param->ch_max_blk_size[6] =  127; break;
                case 6  : param->ch_max_blk_size[6] =  255; break;
                case 7  : param->ch_max_blk_size[6] =  511; break;
                case 8  : param->ch_max_blk_size[6] = 1023; break;
                case 9  : param->ch_max_blk_size[6] = 2047; break;
                case 10 : param->ch_max_blk_size[6] = 4095; break;
            }
			
			reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_2_H_OFFSET);
			param->ch_multi_blk_type[6] = (reg & DMAC_PARAM_CH6_MULTI_BLK_TYPE) >> 24;
			
			reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_5_L_OFFSET);
			tmp = reg & DMAC_PARAM_CHX_DTW;
            switch (tmp)
			{
                case 0 : param->ch_dtw[6] =   0; break;
                case 1 : param->ch_dtw[6] =   8; break;
                case 2 : param->ch_dtw[6] =  16; break;
                case 3 : param->ch_dtw[6] =  32; break;
                case 4 : param->ch_dtw[6] =  64; break;
                case 5 : param->ch_dtw[6] = 128; break;
                case 6 : param->ch_dtw[6] = 256; break;
            }
			tmp = (reg & DMAC_PARAM_CHX_STW) >> 3;
            switch (tmp)
			{
                case 0 : param->ch_stw[6] =   0; break;
                case 1 : param->ch_stw[6] =   8; break;
                case 2 : param->ch_stw[6] =  16; break;
                case 3 : param->ch_stw[6] =  32; break;
                case 4 : param->ch_stw[6] =  64; break;
                case 5 : param->ch_stw[6] = 128; break;
                case 6 : param->ch_stw[6] = 256; break;
            }
            
			param->ch_stat_dst[6] = (reg & DMAC_PARAM_CHX_STAT_DST) >> 6;
			param->ch_stat_src[6] = (reg & DMAC_PARAM_CHX_STAT_SRC) >> 7;
			param->ch_dst_sca_en[6] = (reg & DMAC_PARAM_CHX_DST_SCA_EN) >> 8;
			param->ch_src_gat_en[6] = (reg & DMAC_PARAM_CHX_SRC_GAT_EN) >> 9;
			param->ch_lock_en[6] = (reg & DMAC_PARAM_CHX_LOCK_EN) >> 10;
			param->ch_multi_blk_en[6] = (reg & DMAC_PARAM_CHX_MULTI_BLK_EN) >> 11;
			param->ch_ctl_wb_en[6] = (reg & DMAC_PARAM_CHX_CTL_WB_EN) >> 12;
			param->ch_hc_llp[6] = (reg & DMAC_PARAM_CHX_HC_LLP) >> 13;
			param->ch_fc[6] = (reg & DMAC_PARAM_CHX_FC) >> 14;
			
			tmp = (reg & DMAC_PARAM_CHX_MAX_MULT_SIZE) >> 16;
            switch (tmp)
			{
                case 0 : param->ch_max_mult_size[6] =   4; break;
                case 1 : param->ch_max_mult_size[6] =   8; break;
                case 2 : param->ch_max_mult_size[6] =  16; break;
                case 3 : param->ch_max_mult_size[6] =  32; break;
                case 4 : param->ch_max_mult_size[6] =  64; break;
                case 5 : param->ch_max_mult_size[6] = 128; break;
                case 6 : param->ch_max_mult_size[6] = 256; break;
            }
			
			param->ch_dms[6] = (reg & DMAC_PARAM_CHX_DMS) >> 19;
			param->ch_lms[6] = (reg & DMAC_PARAM_CHX_LMS) >> 22;
			param->ch_sms[6] = (reg & DMAC_PARAM_CHX_SMS) >> 25;
			
			tmp = (reg & DMAC_PARAM_CHX_FIFO_DEPTH) >> 28;
            switch (tmp)
			{
	            case 0 : param->ch_fifo_depth[6] =   4; break;
	            case 1 : param->ch_fifo_depth[6] =   8; break;
	            case 2 : param->ch_fifo_depth[6] =  16; break;
	            case 3 : param->ch_fifo_depth[6] =  32; break;
	            case 4 : param->ch_fifo_depth[6] =  64; break;
	            case 5 : param->ch_fifo_depth[6] = 128; break;
	            case 6 : param->ch_fifo_depth[6] = 256; break;
            }
        }

        /* Channel 7 */
        if(param->num_channels == 8) 
		{
			reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_1_L_OFFSET);
			tmp = (reg & DMAC_PARAM_CH7_MAX_BLK_SIZE) >> 28;
            switch (tmp)
			{
                case 0  : param->ch_max_blk_size[7] =    3; break;
                case 1  : param->ch_max_blk_size[7] =    7; break;
                case 2  : param->ch_max_blk_size[7] =   15; break;
                case 3  : param->ch_max_blk_size[7] =   31; break;
                case 4  : param->ch_max_blk_size[7] =   63; break;
                case 5  : param->ch_max_blk_size[7] =  127; break;
                case 6  : param->ch_max_blk_size[7] =  255; break;
                case 7  : param->ch_max_blk_size[7] =  511; break;
                case 8  : param->ch_max_blk_size[7] = 1023; break;
                case 9  : param->ch_max_blk_size[7] = 2047; break;
                case 10 : param->ch_max_blk_size[7] = 4095; break;
            }
			reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_2_H_OFFSET);
			param->ch_multi_blk_type[7] = (reg & DMAC_PARAM_CH7_MULTI_BLK_TYPE) >> 28;
			
			reg = FMSH_ReadReg(baseAddr, DMAC_DMA_COMP_PARAMS_6_H_OFFSET);
			tmp = reg & DMAC_PARAM_CHX_DTW;
            switch (tmp)
			{
                case 0 : param->ch_dtw[7] =   0; break;
                case 1 : param->ch_dtw[7] =   8; break;
                case 2 : param->ch_dtw[7] =  16; break;
                case 3 : param->ch_dtw[7] =  32; break;
                case 4 : param->ch_dtw[7] =  64; break;
                case 5 : param->ch_dtw[7] = 128; break;
                case 6 : param->ch_dtw[7] = 256; break;
            }
			tmp = (reg & DMAC_PARAM_CHX_STW) >> 3;
            switch (tmp)
			{
                case 0 : param->ch_stw[7] =   0; break;
                case 1 : param->ch_stw[7] =   8; break;
                case 2 : param->ch_stw[7] =  16; break;
                case 3 : param->ch_stw[7] =  32; break;
                case 4 : param->ch_stw[7] =  64; break;
                case 5 : param->ch_stw[7] = 128; break;
                case 6 : param->ch_stw[7] = 256; break;
            }
			
			param->ch_stat_dst[7] = (reg & DMAC_PARAM_CHX_STAT_DST) >> 6;
			param->ch_stat_src[7] = (reg & DMAC_PARAM_CHX_STAT_SRC) >> 7;
			param->ch_dst_sca_en[7] = (reg & DMAC_PARAM_CHX_DST_SCA_EN) >> 8;
			param->ch_src_gat_en[7] = (reg & DMAC_PARAM_CHX_SRC_GAT_EN) >> 9;
			param->ch_lock_en[7] = (reg & DMAC_PARAM_CHX_LOCK_EN) >> 10;
			param->ch_multi_blk_en[7] = (reg & DMAC_PARAM_CHX_MULTI_BLK_EN) >> 11;
			param->ch_ctl_wb_en[7] = (reg & DMAC_PARAM_CHX_CTL_WB_EN) >> 12;
			param->ch_hc_llp[7] = (reg & DMAC_PARAM_CHX_HC_LLP) >> 13;
			param->ch_fc[7] = (reg & DMAC_PARAM_CHX_FC) >> 14;
			
			tmp = (reg & DMAC_PARAM_CHX_MAX_MULT_SIZE) >> 16;
            switch (tmp)
			{
                case 0 : param->ch_max_mult_size[7] =   4; break;
                case 1 : param->ch_max_mult_size[7] =   8; break;
                case 2 : param->ch_max_mult_size[7] =  16; break;
                case 3 : param->ch_max_mult_size[7] =  32; break;
                case 4 : param->ch_max_mult_size[7] =  64; break;
                case 5 : param->ch_max_mult_size[7] = 128; break;
                case 6 : param->ch_max_mult_size[7] = 256; break;
            }
			
			param->ch_dms[7] = (reg & DMAC_PARAM_CHX_DMS) >> 19;
			param->ch_lms[7] = (reg & DMAC_PARAM_CHX_LMS) >> 22;
			param->ch_sms[7] = (reg & DMAC_PARAM_CHX_SMS) >> 25;
			
			tmp = (reg & DMAC_PARAM_CHX_FIFO_DEPTH) >> 28;
            switch (tmp)
			{
	            case 0 : param->ch_fifo_depth[7] =   4; break;
	            case 1 : param->ch_fifo_depth[7] =   8; break;
	            case 2 : param->ch_fifo_depth[7] =  16; break;
	            case 3 : param->ch_fifo_depth[7] =  32; break;
	            case 4 : param->ch_fifo_depth[7] =  64; break;
	            case 5 : param->ch_fifo_depth[7] = 128; break;
	            case 6 : param->ch_fifo_depth[7] = 256; break;
            }
        }
    }
    else
	{
        retval = -FMSH_ENOSYS;
    }
    return retval;
}

/*****************************************************************************
*
* @description
* This function checks if the specified DMA channel is in range.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_checkChannelRange(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    int errorCode = 0;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;

    if (DMAC_CH_MASK & ch_num || ch_num == Dmac_no_channel)
	{
        errorCode = -FMSH_ECHRNG;
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function checks if the specified DMA channel is Busy (enabled)
* or not. Also checks if the specified channel is in range.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*           ch_num Enumerated DMA channel number.
*
* @return   
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int FDmaPs_checkChannelBusy(
        FDmaPs_T *pDmac,
        enum FDmaPs_channelNumber ch_num)
{
    int errorCode;
    u32 reg;
    FDmaPs_Param_T *param;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    errorCode = 0;

    if (DMAC_CH_MASK & ch_num || ch_num == Dmac_no_channel)
	{
        errorCode = -FMSH_ECHRNG;
    }
	else
	{
		reg = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CH_EN_REG_L_OFFSET);
        if (reg & ch_num)
		{
            errorCode = -FMSH_EBUSY;
        }
    }
    return errorCode;
}

/*****************************************************************************
*
* @description
* This function places each channel number into an ordered array
* based on the priority level setting for each channel.
*
* @param    
*           pDmac is the pointer to the DMA controller device.
*
* @return   
*           NA.
*
* @note     NA.
*
*****************************************************************************/
void FDmaPs_setChannelPriorityOrder(FDmaPs_T *pDmac)
{
    int i, ch_num, order, ch_priority;
    int priority[8];

    FDmaPs_Param_T *param;
    FDmaPs_Instance_T *instance;

    DMAC_COMMON_REQUIREMENTS(pDmac);

    param = pDmac->comp_param;
    instance = pDmac->instance;

    /* Initialise the channel order and priority*/
    for (i = 0; i < DMAC_MAX_CHANNELS; i++)
	{
        priority[i] = 0;
        instance->ch_order[i] = i;
    }

    /* loop through each of the channels configured on the DMAC*/
    for (ch_num = 0; ch_num < param->num_channels; ch_num++)
	{

        /* read the priority of the current channel*/
		ch_priority = FMSH_ReadReg(pDmac->config.BaseAddress, DMAC_CHX_CFG_L_OFFSET(ch_num));
		ch_priority = (ch_priority & DMAC_CFG_L_CH_PRIOR) >> 5;

        /* Check the channel order array to see what position*/
        /* this channel comes in the priority list.*/
        for (i = 0; i <= param->num_channels; i++)
		{
            if (A_MAXEQ_B(ch_priority, priority[i]) || i == param->num_channels)
			{
                order = i;
                break;
            }
        }

        /* Now we know the order for this channel insert in the correct*/
        /* array position and shift current priority ordering to suite.*/
        for (i = ch_num; i > order; i--)
		{
            priority[i] = priority[i-1];
            instance->ch_order[i] = instance->ch_order[i-1];
        }
        priority[order] = ch_priority;
        instance->ch_order[order] = ch_num;
    }
}


