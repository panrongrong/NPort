/* vxSdmmc.c - fmsh 7020/7045 sdmmc driver */

/*
 * Copyright (c) 2013 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */
 
/*
modification history
--------------------
01a, 22Nov19, jc  written.
*/

/*
DESCRIPTION
	no vxbus struct
*/

/* includes */

#include <vxWorks.h>
#include <stdio.h>
#include <semLib.h>
#include <sysLib.h>
#include <taskLib.h>
#include <vxBusLib.h>
#include <cacheLib.h>
#include <string.h>
#include <vxbTimerLib.h>

#include "vxbSdmmc2.h"

#ifdef ENABLE_FATFS_SDMMC
#include "./vxbFatFS2/ff.h"
#endif

/*
defines 
*/
#if 1
#define SDMMC_DBG
#ifdef SDMMC_DBG

#define VX_DBG(string, a, b, c, d, e, f)                    \
        if (_func_logMsg != NULL)                           \
           printf(string, a, b, c, d, e, f)         /* (* _func_logMsg)(string, a, b, c, d, e, f) */
#else
#define VX_DBG(string, a, b, c, d, e, f)
#endif

/* error info log */
#define VX_DBG2(string, a, b, c, d, e, f) printf(string, a, b, c, d, e, f)
#endif


#undef  FMQL_SDMMC_0_BASE
#undef  FMQL_SDMMC_1_BASE
#define  FMQL_SDMMC_0_BASE     (VX_SDMMC_0_BASE)
#define  FMQL_SDMMC_1_BASE     (VX_SDMMC_1_BASE)



#if 1

static const UINT32 vxb_tran_exp[] = 
{
    100000,         1000000,                10000000,       100000000,
    0,              0,              0,              0
};

static const UINT8 vxb_tran_mant[] =
{
    0,      10,     12,     13,     15,     20,     25,     30,
    35,     40,     45,     50,     55,     60,     70,     80,
};

static const UINT32 vxb_taac_exp[] = 
{
	1,	10,	100,	1000,	10000,	100000,	1000000, 10000000,
};

static const UINT32 vxb_taac_mant[] = 
{
	0,	10,	12,	13,	15,	20,	26,	30,
	35,	40,	45,	52,	55,	60,	70,	80,
};

#endif

/* 
VxBus methods 
*/
LOCAL void vxbSdmmcInstInit (VXB_DEVICE_ID pDev);
LOCAL void vxbSdmmcInstInit2 (VXB_DEVICE_ID pDev);
LOCAL void vxbSdmmcInstConnect (VXB_DEVICE_ID pDev);
LOCAL void vxbSdmmcIsr(VXB_DEVICE_ID pDev);

void vxbSdmmcCtrl_WrReg32(VXB_DEVICE_ID pDev, UINT32 offset, UINT32 value);
UINT32 vxbSdmmcCtrl_RdReg32(VXB_DEVICE_ID pDev, UINT32 offset);


#if 1
/* locals */

LOCAL struct drvBusFuncs vxbSdmmcVxbFuncs = 
{
    vxbSdmmcInstInit,      /* devInstanceInit */
    vxbSdmmcInstInit2,     /* devInstanceInit2 */
    vxbSdmmcInstConnect    /* devConnect */
};

/*
LOCAL device_method_t vxbSdmmcDeviceMethods[] = 
{
    DEVMETHOD (vxbSdmmcControlGet, vxbSdmmcCtrlGet),
    DEVMETHOD (busDevShow,       vxbSdmmcShow),
    DEVMETHOD (vxbDrvUnlink,     vxbSdmmcInstUnlink),
    DEVMETHOD_END
};
*/

LOCAL struct vxbPlbRegister vxbSdmmcDevRegistration = 
{
    {
    NULL,                   /* pNext */
    VXB_DEVID_DEVICE,       /* devID */
    VXB_BUSID_PLB,          /* busID = PLB */
    VXB_VER_4_0_0,          /* vxbVersion */
    
    FM_SDMMC_NAME,            /* drvName : fmcan */
    
    &vxbSdmmcVxbFuncs,         /* pDrvBusFuncs */
    NULL /*vxbSdmmcDeviceMethods*/,     /* pMethods */
    NULL,                   /* devProbe  */
    NULL,                   /* pParamDefaults */
    },
};


/*****************************************************************************
*
* vxbSdmmcRegister - register with the VxBus subsystem
*
* This routine registers the SPI driver with VxBus Systems.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void vxbSdmmcRegister (void)
{
    /*vxbDevRegister ((struct vxbDevRegInfo * &vxbSdmmcDevRegistration);*/
}

void vxbSdmmcRegister2 (void)
{
    vxbDevRegister ((struct vxbDevRegInfo *) &vxbSdmmcDevRegistration);
}

/*****************************************************************************
*
* vxbSdmmcInstInit - initialize fmsh SPI controller
*
* This function implements the VxBus instInit handler for a SPI controller
* device instance.
*
* Initialize the device by the following:
*
* - retrieve the resource from the hwconf
* - per-device init
* - announce SPI Bus and create device instance
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxbSdmmcInstInit
    (
    VXB_DEVICE_ID pDev
    )
{
    FM_SDMMC_DRV_CTRL2 * pDrvCtrl;
    struct hcfDevice * pHcf;
    FUNCPTR clkFunc = NULL;
    int i;

    /* check for valid parameter */	
    /*VXB_ASSERT_NONNULL_V (pDev);*/
    if (pDev == NULL)
        return;

    /* 
    create controller driver context structure for core 
	*/
    pDrvCtrl = (FM_SDMMC_DRV_CTRL2 *) malloc (sizeof (FM_SDMMC_DRV_CTRL2));
    if (pDrvCtrl == NULL)
    {
        return;
    }
	
    bzero ((char *)pDrvCtrl, sizeof(FM_SDMMC_DRV_CTRL2));

    /*
    save instance ID 
	*/
    pDev->pDrvCtrl = pDrvCtrl;
    pDrvCtrl->pDev = pDev;
	
#if 0
    for (i = 0; i < VXB_MAXBARS; i++)
    {
        if (pDev->regBaseFlags[i] == VXB_REG_MEM)
        {
            break;
        }
    }

    if (i == VXB_MAXBARS)
    {
    #ifndef  _VXBUS_BASIC_HWMEMLIB
        hwMemFree ((char *)pDrvCtrl);
    #endif /* _VXBUS_BASIC_HWMEMLIB */

        pDev->pDrvCtrl = NULL;
        return;
    }
    pDrvCtrl->regBase = pDev->pRegBase[i];
#else
    pDrvCtrl->regBase = pDev->pRegBase[0];
	printf("<SdMmc>pDev->unitNumber:(%d); pDrvCtrl->regBase:0x%08X \n", pDev->unitNumber, pDrvCtrl->regBase);
#endif

	i = 0;  /* jc*/
    vxbRegMap (pDev, i, &pDrvCtrl->regHandle);
    pDrvCtrl->regHandle = (void *)((ULONG)pDrvCtrl->regHandle);

    pHcf = (struct hcfDevice *) hcfDeviceGet (pDev);
    if (pHcf == NULL)
    {
    #ifndef  _VXBUS_BASIC_HWMEMLIB
        hwMemFree ((char *)pDrvCtrl);
    #endif /* _VXBUS_BASIC_HWMEMLIB */

        pDev->pDrvCtrl = NULL;
        return;
    }

    /* retrieve the integer value for clock frequency */

    /*
     * resourceDesc {
     * The clkFreq resource specifies the
     * frequency of the external oscillator
     * connected to the device for baud
     * rate determination.  When specified
     * as an integer, it represents the
     * frequency, in Hz, of the external
     * oscillator. }
     */
    if (devResourceGet (pHcf, "clkFreq", HCF_RES_INT,
                        (void *) &pDrvCtrl->clkFrequency) != OK)
    {        
        pDrvCtrl->clkFrequency = FM_SDMMC_CLK_DIVIDED_VAL;
    }

    /*
     * resourceDesc {
     * The polling resource specifies whether the driver uses polling mode
     * or not. If this property is not explicitly specified, the driver
     * uses interrupt by default. }
     */
    if (devResourceGet (pHcf, "polling", HCF_RES_INT,
                        (void *) &pDrvCtrl->polling) != OK)
    {
        pDrvCtrl->polling = TRUE;
    }

    /*
     * resourceDesc {
     * The dmaMode resource specifies whether the driver uses DMA mode
     * or not. If this property is not explicitly specified, the driver
     * does not use DMA by default. }
     */
    if (devResourceGet (pHcf, "dmaMode", HCF_RES_INT,
                        (void *) &pDrvCtrl->dmaMode) != OK)
    {
        pDrvCtrl->dmaMode = FALSE;
    }
						
    /*
     * resourceDesc {
     * The dmaMode resource specifies whether the driver uses DMA mode
     * or not. If this property is not explicitly specified, the driver
     * does not use DMA by default. }
     */
    if (devResourceGet (pHcf, "flags", HCF_RES_INT,
                        (void *) &pDrvCtrl->flags) != OK)
    {
        pDrvCtrl->flags = (FM_SDMMC_FLAGS_CARD_PRESENT | FM_SDMMC_FLAGS_CARD_WRITABLE);
    }

    pDrvCtrl->initPhase = 1;
	return;
}

/*******************************************************************************
*
* vxbSdmmcCtrlInit -  SPI controller initialization
*
* This routine performs the SPI controller initialization.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxbSdmmcCtrl_Init (VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;

	vxbT_SDMMC_CTRL* pCtrl = (vxbT_SDMMC_CTRL*)(&pDrvCtrl->CTRL);
	vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CARD*)(&pDrvCtrl->CARD);
	vxbT_SDMMC_CMD* pCmd   = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);;
	
    UINT32 value;
    UINT32 us = 0;
	int ctrl_x = 0;

    /* check for valid parameter */
   /* VXB_ASSERT_NONNULL_V (pDev);*/
	int ret = 0;

	UINT32 tmp32 = 0;
	int timeout = SDMMC_POLL_TIMEOUT;
	int dma_msize = 0;
	int rx_wmark = 0, tx_wmark = 0;

	UINT8 cmd6_RdBuf[64] = { 0U };
	
	pDrvCtrl->CTRL.ctrl_x = pDev->unitNumber;
	ctrl_x = pDrvCtrl->CTRL.ctrl_x;

#if 1
	/**/
	/* vxbSdmmc_ctrl select*/
	/**/
	switch (ctrl_x)
	{
	case SDMMC_CTRL_0:
		pCtrl->baseAddr = FMQL_SDMMC_0_BASE;	/* Base address of the device */
		break;
		
	case SDMMC_CTRL_1:
		pCtrl->baseAddr = FMQL_SDMMC_1_BASE;	/* Base address of the device */
		break;
	}	
#endif


	/**/
	/* config init*/
	/**/
	pCtrl->ctrl_x = ctrl_x;

	pCtrl->sysClkHz = PS_SDIO_CLK_FREQ_HZ;		/* 100M Hz Input to vxbSdmmc_ctrl clock frequency: sysclk of bus */
	pCtrl->isCardDetect = 1;	/* has Card Detect ? */
	pCtrl->isWrProtect = 1; 	/* has Write Protect ? */

	pCtrl->devIP_ver = 0x1000;	  /* 1.0.0.0 */
	pCtrl->devIP_type = FMSH_DEVIP_APB_SDMMC;	   /* identify peripheral types: FMSH_DEVIP_APB_SDMMC */

	pCtrl->xferMode = MODE_XFER_FIFO;  /* default*/


	/*///////////////////////////////////*/
	/**/
	/* vxbSdmmc_ctrl init*/
	/**/
	/*///////////////////////////////////*/
	/**/
	/* reset ctrl*/
	/*	*/
#if 1
	tmp32 = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_CTRL) | CONTROLLER_RESET;
	vxbSdmmcCtrl_WrReg32(pDev, SDMMC_CTRL, tmp32);

	timeout = SDMMC_POLL_TIMEOUT;
	do
	{
		delay_1us();
		tmp32 = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_CTRL) & CONTROLLER_RESET;
		
		timeout--;
		if (timeout <= 0)
		{
			VX_DBG2("sdmmc ctrl_%d reset ctrl fail, exit! \n", ctrl_x, 2,3,4,5,6);
			return (-FMSH_ETIME);
		}
	} while (tmp32);

	/**/
	/* reset fifo*/
	/**/
	tmp32 = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_CTRL) | FIFO_RESET;
	vxbSdmmcCtrl_WrReg32(pDev, SDMMC_CTRL, tmp32);

	timeout = SDMMC_POLL_TIMEOUT;
	do
	{
		delay_1us();
		tmp32 = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_CTRL) & FIFO_RESET;
		
		timeout--;
		if (timeout <= 0)
		{
			VX_DBG2("sdmmc ctrl_%d reset fifo fail, exit! \n", ctrl_x, 2,3,4,5,6);
			return (-FMSH_ETIME);
		}
	} while (tmp32);
#else
	/**/
	/* reset ctrl, fifo, dma*/
	/**/
	tmp32 = vxbSdmmcCtrl_RdReg32(SDMMC_CTRL) | (CONTROLLER_RESET | FIFO_RESET | DMA_RESET);
	vxbSdmmcCtrl_WrReg32(SDMMC_CTRL, tmp32);

	timeout = SDMMC_POLL_TIMEOUT;
	do
	{
		delay_1us();
		tmp32 = vxbSdmmcCtrl_RdReg32(SDMMC_CTRL) & (CONTROLLER_RESET | FIFO_RESET | DMA_RESET);
		
		timeout--;
		if (timeout <= 0)
		{
			VX_DBG2("sdmmc ctrl_%d reset ctrl_fifo_dma fail, exit! \n", ctrl_x, 2,3,4,5,6);
			return (-FMSH_ETIME);
		}
	} while (tmp32);
#endif

	/*
	RX_WMark = (FIFO_DEPTH / 2) - 1
	TX_WMark =	FIFO_DEPTH / 2
	*/
	/**/
	/* set dma & fifo*/
	/*	*/
	dma_msize = 2; /* 010 - 8*/
	tmp32 = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_FIFOTH) | (dma_msize << DW_DMA_MULTI_TRAN_SIZE_OFFSET);
	vxbSdmmcCtrl_WrReg32(pDev, SDMMC_FIFOTH, tmp32);

	/* MSize = 8，RX_WMark = 7, TX_WMark = 8*/
	tmp32 = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_FIFOTH) & (~(RX_WMARK_MASK << RX_WMARK_OFFSET));
	rx_wmark = 7;
	tmp32 |= (rx_wmark << RX_WMARK_OFFSET);
	vxbSdmmcCtrl_WrReg32(pDev, SDMMC_FIFOTH, tmp32);
	/**/
	tmp32 = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_FIFOTH) & (~(TX_WMARK_MASK << TX_WMARK_OFFSET));
	tx_wmark = 8;
	tmp32 |= (tx_wmark << TX_WMARK_OFFSET);
	vxbSdmmcCtrl_WrReg32(pDev, SDMMC_FIFOTH, tmp32);

	/**/
	/* clear irq*/
	/*	*/
	vxbSdmmcCtrl_WrReg32(pDev, SDMMC_RINTSTS, 0xFFFFFFFF);

	/* enable irq*/
	tmp32 = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_CTRL) | INT_ENABLE;
	vxbSdmmcCtrl_WrReg32(pDev, SDMMC_CTRL, tmp32);

	/* mask interrupt: enable CD*/
	tmp32 = INT_MASK_CD;			  /* 0-屏蔽中断; 1-使能中断 */
	vxbSdmmcCtrl_WrReg32(pDev, SDMMC_INTMASK, tmp32);

	/**/
	/* dma_mode ?*/
	/**/
	if (pCtrl->xferMode == MODE_XFER_DMA)
	{		
		/* enble DMA*/
		tmp32 = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_CTRL) | DMA_ENABLE;
		vxbSdmmcCtrl_WrReg32(pDev, SDMMC_CTRL, tmp32);

		/**/
		/* init dma*/
		/* FSdmmcPs_initDmac(pDev);*/
	}

	/**/
	/* clear int*/
	/**/
	vxbSdmmcCtrl_WrReg32(pDev, SDMMC_RINTSTS, vxbSdmmcCtrl_RdReg32(pDev, SDMMC_RINTSTS));

#if 1  /* card detect*/
	if (pCtrl->isCardDetect == 1)
	{	
		tmp32 = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_CDETECT) & CARD_DETECT;
		if (tmp32 != CARD_INSERT)
		{
			VX_DBG2("sdmmc ctrl_%d: No card insert, exit! \n", ctrl_x, 2,3,4,5,6);
			return ERROR;
		}
		else
		{
			VX_DBG("sdmmc ctrl_%d: Card insert! \n", ctrl_x, 2,3,4,5,6);
		}
	}

	pCard->status = 0;
	pCard->rca = 0;
#endif

	/**/
	/* powerup*/
	/**/
#if 1
	vxbSdmmcCtrl_WrReg32(pDev, SDMMC_PWREN, 0xFFFF);
	delay_us(100);
#endif

	/**/
	/* setclock-400K*/
	/**/
	ret = vxbSdmmcSet_Clk_DataRate(pDev, SDMMC_CLK_400_KHZ);
	if (ret < 0)
	{
		VX_DBG2("sdmmc ctrl_%d: vxbSdmmcSet_Clk_DataRate-400K fail, exit(%d)! \n", ctrl_x, ret,3,4,5,6);
		return ret;
	}
	else
	{
		VX_DBG("sdmmc ctrl_%d: vxbSdmmcSet_Clk_DataRate-400K OK! \n", ctrl_x, 2,3,4,5,6);
	}

	delay_us(10);

#if 1	
	/* 
	go IDLE state 
	*/
	ret = vxbSdmmc_cmd0(pDev);
	if (ret < 0)
	{
		VX_DBG2("sdmmc ctrl_%d: vxbSdmmc_cmd0 fail, exit(%d)! \n", ctrl_x, ret,3,4,5,6);
		return ret;
	}
	pCard->card_ver = SDMMC_CARD_VER_1_0;

	delay_us(100);

	/*
	check whether ver2.0
	*/
	ret = vxbSdmmc_cmd8_sd(pDev);
	if (ret < 0)
	{
		pCard->card_ver = SDMMC_CARD_VER_1_0;
		VX_DBG2("sdmmc ctrl_%d: vxbSdmmc_cmd8_sd no response(%d)! \n", ctrl_x, ret,3,4,5,6);
	}
	else
	{
		VX_DBG("sdmmc ctrl_%d: vxbSdmmc_cmd8_get response! \n", ctrl_x, 2,3,4,5,6);

		if((pCmd->rsp_buf[0] & 0xFF) == (SD_CMD8_VOL_PATTERN & 0xFF))
		{
			pCard->card_ver = SDMMC_CARD_VER_2_0;
			VX_DBG("sdmmc ctrl_%d: card ver2.0 0x%X \n", ctrl_x, pCmd->rsp_buf[0], 3,4,5,6);
		}
		else 
		{
			pCard->card_ver = SDMMC_CARD_VER_1_0;
			VX_DBG("sdmmc ctrl_%d: card ver1.0 0x%X \n", ctrl_x, pCmd->rsp_buf[0], 3,4,5,6);
		}
	}


	/* 
	 check whether it's a MMC or a SD card. 
	*/

	/* APP_CMD */

	VX_DBG("sdmmc ctrl_%d:vxbSdmmc_cmd55->g_pCard->rca(0x%X) \n", ctrl_x, pCard->rca,3,4,5,6);

	ret = vxbSdmmc_cmd55(pDev); 
	if (ret < 0)
	{
		if (ret == (-FMSH_ENODEV))
		{
			return ret;
		}
		
		VX_DBG("sdmmc ctrl_%d: MultiMedia Card Identification: \n", ctrl_x, 2,3,4,5,6);
		
		tmp32 = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_CDETECT) & CARD_DETECT;		
		if (tmp32 == CARD_INSERT)
		{	 
			pCard->card_type = MMC;
		}
		else
		{	 
			pCard->card_type = EMMC;
		}		
	}
	else
	{
		pCard->card_type = SD;
		VX_DBG("sdmmc ctrl_%d: SD Memory Card Identification: \n", ctrl_x,2,3,4,5,6);
		
	}

	/**/
	/* enumerate the sdmmc*/
	/**/
	switch (pCard->card_type)
	{
	case MMC:
	case EMMC:
		ret = vxbSdmmc_Enumerate_MMC(pDev);
		if (ret < 0)
		{
			VX_DBG2("sdmmc ctrl_%d: vxbSdmmc_Enumerate_MMC fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
			return ret;
		}
		break;

	case SD:
		ret = vxbSdmmc_Enumerate_SD(pDev);
		if (ret < 0)
		{
			VX_DBG2("sdmmc ctrl_%d: vxbSdmmc_Enumerate_SD fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
			return ret;
		}
		break;
	}

#endif

#if 1
	/**/
	/* switch to TRAN mode to Select the current SD/MMC*/
	/* SELECT CARD & set card state from Stand-by to Transfer*/
	/**/
	ret = vxbSdmmc_cmd7(pDev);
	if (ret < 0)
	{
		VX_DBG2("sdmmc ctrl_%d: vxbSdmmc_cmd7 fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
		return ret;
	}
	if (pCmd->rsp_buf[0] & (R1_CC_ERR | R1_ERR))
	{
		VX_DBG2("sdmmc ctrl_%d: CMD7 response fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
		return (-FMSH_EIO);
	}

	/* 
	select card bus width 
	*/
	if (pCard->card_type == SD)
	{
		ret = vxbSdmmcCard_Set_BusWidth(pDev, SD_BUS_WIDTH_4);
		if (ret < 0)
		{
			VX_DBG2("sdmmc ctrl_%d: vxbSdmmcCard_Set_BusWidth fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
			return ret;
		}
	}
	else  /* mmc / eMMC*/
	{
		ret = vxbSdmmcCard_Set_BusWidth_MMC(pDev, SD_BUS_WIDTH_4);
		if (ret < 0)
		{
			VX_DBG2("sdmmc ctrl_%d: vxbSdmmcCard_Set_BusWidth_MMC fail,exit(%d)! \n", ctrl_x, ret, 3,4,5,6);
			return ret;
		}

		delay_ms(10);
		
		ret = vxbSdmmcCard_Get_ExtCsd_MMC(pDev);
		if (ret < 0)
		{
			VX_DBG2("sdmmc ctrl_%d: Get MMC Ext_CSD info failed! \n", ctrl_x,2,3,4,5,6);
			return ret;
		}		
	}


	/* 
	Set high clock rate for the normal data transfer 
	*/
	ret = vxbSdmmcSet_Clk_DataRate(pDev, pCard->info_csd.max_dtr);
	if (ret < 0)
	{
		VX_DBG2("sdmmc ctrl_%d: Switch the clock to %dHz for data transfer fail! \n", ctrl_x, pCard->info_csd.max_dtr, 3,4,5,6);
		return ret;
	}
	else
	{
		VX_DBG("sdmmc ctrl_%d: Switch the clock to %dHz for data transfer OK! \n", ctrl_x, pCard->info_csd.max_dtr, 3,4,5,6);
	}
#endif

#if 1
	if (pCard->card_type == SD)
	{
		/* get sd card SCR info*/
		ret = vxbSdmmcCard_Get_Info_SCR(pDev);
		if (ret < 0)
		{
			VX_DBG2("SD Get SD's SCR info error! \n", 1,2,3,4,5,6);
			return ret;
		}

		if (pCard->info_scr.sd_spec != 0)
		{
			ret = sdmmcCar_Get_BusSpeed(pDev, cmd6_RdBuf);
			if (ret < 0)
			{
				return ret;
			}

			if ((cmd6_RdBuf[13] & HIGH_SPEED_SUPPORT) != 0)
			{
				ret = vxbSdmmcChg_BusSpeed(pDev);
				if (ret < 0)
				{
					VX_DBG2("Failed to change SD to high speed mode! \n", 1,2,3,4,5,6);
					return ret;
				}
				else
				{ 
					VX_DBG2("Change SD to high speed mode succeed! \n", 1,2,3,4,5,6);
				}
			}
		}
	}
	else if ((pCard->card_type == MMC) || (pCard->card_type == EMMC))
	{
		if ((pCard->ext_csd[MMC_EXT_CSD_CARD_TYPE] & EXT_CSD_DEVICE_TYPE_HIGH_SPEED) != 0)
		{
			ret = vxbSdmmcChg_BusSpeed_MMC(pDev);
			if (ret < 0)
			{
				VX_DBG2("Failed to change MMC to high speed mode! \n", 1,2,3,4,5,6);
				return ret;
			}
			else
			{
				VX_DBG2("Change MMC to high speed mode succeed! \n", 1,2,3,4,5,6);
			}
		}
		else
		{
			VX_DBG2("MMC doesn't support high speed mode! \n", 1,2,3,4,5,6);
		}
	}
#endif

	if (pCard->highCapacity == 0)
	{	 
		vxbSdmmcCard_Set_BlockSize(pDev, SDMMC_BLOCK_SIZE);
		VX_DBG("sdmmc ctrl_%d: vxbSdmmcCard_Set_BlockSize(%d) OK! \n", ctrl_x, SDMMC_BLOCK_SIZE, 3,4,5,6);
	}
	
    pDrvCtrl->initDone = TRUE;
	return;
}

void vxbSdmmcCtrl_Init_fatFS (VXB_DEVICE_ID pDev)
{
	vxbSdmmcCtrl_Init(pDev);
	return;
}

/*******************************************************************************
*
* vxbSdmmcInstInit2 - second level initialization routine of SPI controller
*
* This routine performs the second level initialization of the SPI controller.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxbSdmmcInstInit2
    (
    VXB_DEVICE_ID pDev
    )
{
    FM_SDMMC_DRV_CTRL2 * pDrvCtrl;

    /* check for valid parameter */
    /*VXB_ASSERT_NONNULL_V (pDev);*/

    pDrvCtrl = (FM_SDMMC_DRV_CTRL2 *) pDev->pDrvCtrl;

    /* 
    The semSync semaphore is used to synchronize the SPI transfer. 
	*/
    pDrvCtrl->semSync = semBCreate (SEM_Q_PRIORITY, SEM_FULL);
    if (pDrvCtrl->semSync == NULL)
    {
        VX_DBG2 (  "semBCreate failed for semSync\n",
                 0, 0, 0, 0, 0, 0);
        return;
    }

    /* 
    The muxSem semaphore is used to mutex accessing the controller. 
    */
    pDrvCtrl->muxSem = semMCreate (SEM_Q_PRIORITY);
    if (pDrvCtrl->muxSem == NULL)
    {
        VX_DBG2 (  "semMCreate failed for muxSem\n",
                 0, 0, 0, 0, 0, 0);

        (void) semDelete (pDrvCtrl->semSync);
        pDrvCtrl->semSync = NULL;

        return;
    }
	
    /* 
    sdmmc controller init 
    */
    (void) vxbSdmmcCtrl_Init(pDev);

    pDrvCtrl->initPhase = 2;
}

/*******************************************************************************
*
* vxbSdmmcInstConnect - third level initialization
*
* This routine performs the third level initialization of the SPI controller
* driver.
*
* RETURNS: N/A
*
* ERRNO : N/A
*/

LOCAL void vxbSdmmcInstConnect
    (
    VXB_DEVICE_ID       pDev
    )
{
    FM_SDMMC_DRV_CTRL2 * pDrvCtrl;

    /* check for valid parameter */
    /*VXB_ASSERT_NONNULL_V (pDev);*/

    pDrvCtrl = (FM_SDMMC_DRV_CTRL2 *) pDev->pDrvCtrl;

    /* 
    connect and enable interrupt for non-poll mode 
	*/
    if (!pDrvCtrl->polling)
    {
        if (vxbIntConnect (pDev, 0, vxbSdmmcIsr, pDev) != OK)
        {
            VX_DBG2("vxbIntConnect return ERROR \n",
                    1, 2, 3, 4, 5, 6);
        }
        (void) vxbIntEnable (pDev, 0, vxbSdmmcIsr, pDev);
    }

    pDrvCtrl->initPhase = 3;
}

/*****************************************************************************
*
* vxbSdmmcInstUnlink - VxBus unlink handler
*
* This function shuts down a SPI controller instance in response to an
* an unlink event from VxBus. This may occur if our VxBus instance has
* been terminated, or if the SPI driver has been unloaded.
*
* RETURNS: OK if device was successfully destroyed, otherwise ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbSdmmcInstUnlink
    (
    VXB_DEVICE_ID pDev,
    void *        unused
    )
{
    /* check if the pDev pointer is valid */
    /*VXB_ASSERT (pDev != NULL, ERROR)*/

    FM_SDMMC_DRV_CTRL2 * pDrvCtrl = (FM_SDMMC_DRV_CTRL2 *) pDev->pDrvCtrl;

    /*
     * The semaphore and interrupt resource are released here . The 
     * semaphore was created at phase 2 and interrupt was installed
     * at phase 3.
     */
    if (pDrvCtrl->initPhase >= 2)
    {
        (void)semTake (pDrvCtrl->muxSem, WAIT_FOREVER);

        /* disconnect and disable interrupt */

        if (!pDrvCtrl->polling && pDrvCtrl->initPhase == 3)
        {
            (void) vxbIntDisable (pDev, 0, vxbSdmmcIsr, pDev);

            if (vxbIntDisconnect (pDev, 0, vxbSdmmcIsr, pDev) != OK)
            {
                VX_DBG2("vxbIntDisconnect return ERROR \n",
                        1, 2, 3, 4, 5, 6);
            }
        }

        (void) semDelete (pDrvCtrl->muxSem);
        pDrvCtrl->muxSem = NULL;

        (void) semDelete (pDrvCtrl->semSync);
        pDrvCtrl->semSync = NULL;
    }

    if (pDrvCtrl->dmaMode)
    {
        /*vxbSdmmcDmaChanFree (pDev);*/
    }

    pDrvCtrl->initPhase = 0;
    pDev->pDrvCtrl = NULL;

    return (OK);
}


/*******************************************************************************
*
* vxbSdmmcIsr - interrupt service routine
*
* This routine handles interrupts of SPI.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxbSdmmcIsr
    (
    VXB_DEVICE_ID pDev
    )
{
    FM_SDMMC_DRV_CTRL2 * pDrvCtrl;

    /* Check if the pDev pointer is valid */
    VXB_ASSERT (pDev != NULL, ERROR)

    pDrvCtrl = (FM_SDMMC_DRV_CTRL2 *)pDev->pDrvCtrl;

    /* disable all SPI interrupts */
/*
	CSR_WRITE_4 (pDev, MCCAN_IRQENABLE, 0);

    VX_DBG2 (CAN_DBG_ISR, "vxbSdmmcIsr: intSts 0x%x\n", 
             CSR_READ_4 (pDev, MCCAN_IRQSTATUS), 2, 3, 4, 5, 6);
*/
    semGive (pDrvCtrl->semSync);

	return;
}

#endif


/*
functions
*/
static int VXB_UNSTUFF_BITS(u32 *rsp_buf, unsigned int start, unsigned int size)
{
	const unsigned int __mask = (1 << (size)) - 1;
	const int __off = /*3 - */((start) / 32);	 
	const int __shft = (start) & 31;	 
	unsigned int __res;  
	__res = rsp_buf[__off] >> __shft; 
	if ((size) + __shft >= 32)	 
		__res |= rsp_buf[__off+1] << (32 - __shft);	
	return (__res & __mask);
}


#if 1

void vxbSdmmcCtrl_WrReg32(VXB_DEVICE_ID pDev, UINT32 offset, UINT32 value)
{
	FM_SDMMC_DRV_CTRL2 * pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	UINT32 tmp32 = pDrvCtrl->regBase;
	
	FMQL_WRITE_32((tmp32 + offset), value);
	return;
}

UINT32 vxbSdmmcCtrl_RdReg32(VXB_DEVICE_ID pDev, UINT32 offset)
{
	FM_SDMMC_DRV_CTRL2 * pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	UINT32 tmp32 = pDrvCtrl->regBase;
	
	return FMQL_READ_32(tmp32 + offset);
}

/*****************************************************************************
*
* @description
* This function is used to check if a command has been accepted by the card.
*
* @param
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
static int vxbSdmmcCtrl_wait_StartCmd_Ok(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2 * pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;

    int timeout = SDMMC_POLL_TIMEOUT;
    UINT32 tmp32 = 0;
    
    do
	{
        delay_1us();
		
		tmp32 = vxbSdmmcCtrl_RdReg32(pDev, SDMMC_CMD_VAL) & START_CMD;
		
        timeout--;
        if (timeout <= 0)
        {
            return (-FMSH_ETIME);
        }
    } while (tmp32);

    return 0;
}

#endif


#if 1

int vxbSdmmcCard_decode_CSD(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CARD*)(&pDrvCtrl->CARD);
	
	UINT32 e, m;
	UINT32 * pResp;	
	
	pResp = pCard->raw_csd;

	if (pCard->card_type == SD)
	{
		pCard->info_csd.csd_struct = VXB_UNSTUFF_BITS(pResp, 126, 2);

		m = VXB_UNSTUFF_BITS(pResp, 115, 4);
		e = VXB_UNSTUFF_BITS(pResp, 112, 3);		
		pCard->info_csd.taac_ns	 = (vxb_taac_exp[e] * vxb_taac_mant[m] + 9) / 10;
		pCard->info_csd.taac_clks	 = VXB_UNSTUFF_BITS(pResp, 104, 8) * 100;
		
		m = VXB_UNSTUFF_BITS(pResp, 99, 4);
		e = VXB_UNSTUFF_BITS(pResp, 96, 3);
		pCard->info_csd.max_dtr	  = vxb_tran_exp[e] * vxb_tran_mant[m]/10;
		pCard->info_csd.cmdclass	  = VXB_UNSTUFF_BITS(pResp, 84, 12);
		
		if (pCard->highCapacity == 0)
		{
			e = VXB_UNSTUFF_BITS(pResp, 47, 3);
	/*		m = VXB_UNSTUFF_BITS(pResp, 62, 12);*/
			m = VXB_UNSTUFF_BITS(pResp, 62, 2) | (VXB_UNSTUFF_BITS(pResp, 64, 10) << 2);
			pCard->info_csd.blockNR	  = (1 + m) << (e + 2);
		}
		else if(pCard->highCapacity == 1)
		{
			e = VXB_UNSTUFF_BITS(pResp, 48, 22);
			pCard->info_csd.blockNR = (e + 1)<<10;
		}
		
		pCard->info_csd.read_blkbits = VXB_UNSTUFF_BITS(pResp, 80, 4);
		pCard->info_csd.read_partial = VXB_UNSTUFF_BITS(pResp, 79, 1);
		pCard->info_csd.write_misalign = VXB_UNSTUFF_BITS(pResp, 78, 1);
		pCard->info_csd.read_misalign = VXB_UNSTUFF_BITS(pResp, 77, 1);
		pCard->info_csd.r2w_factor = VXB_UNSTUFF_BITS(pResp, 26, 3);
		pCard->info_csd.write_blkbits = VXB_UNSTUFF_BITS(pResp, 22, 4);
		pCard->info_csd.write_partial = VXB_UNSTUFF_BITS(pResp, 21, 1);
		
	}
	else  /* mmc / eMMC */
	{
		pCard->info_csd.csd_struct = VXB_UNSTUFF_BITS(pResp, 126, 2);
		if (pCard->info_csd.csd_struct == 0)
		{
			return -FMSH_EINVAL;
		}

		pCard->info_csd.mmca_vsn	 = VXB_UNSTUFF_BITS(pResp, 122, 4);
		
		m = VXB_UNSTUFF_BITS(pResp, 115, 4);
		e = VXB_UNSTUFF_BITS(pResp, 112, 3);
		pCard->info_csd.taac_ns	 = (vxb_taac_exp[e] * vxb_taac_mant[m] + 9) / 10;
		pCard->info_csd.taac_clks	 = VXB_UNSTUFF_BITS(pResp, 104, 8) * 100;

		m = VXB_UNSTUFF_BITS(pResp, 99, 4);
		e = VXB_UNSTUFF_BITS(pResp, 96, 3);
		pCard->info_csd.max_dtr	  = vxb_tran_exp[e] * vxb_tran_mant[m]/10;		
		pCard->info_csd.cmdclass	  = VXB_UNSTUFF_BITS(pResp, 84, 12);

		e = VXB_UNSTUFF_BITS(pResp, 47, 3);
		m = VXB_UNSTUFF_BITS(pResp, 62, 2) | (VXB_UNSTUFF_BITS(pResp, 64, 10) << 2);
		pCard->info_csd.blockNR	  = (1 + m) << (e + 2);
		
		pCard->info_csd.read_blkbits = VXB_UNSTUFF_BITS(pResp, 80, 4);
		pCard->info_csd.read_partial = VXB_UNSTUFF_BITS(pResp, 79, 1);
		pCard->info_csd.write_misalign = VXB_UNSTUFF_BITS(pResp, 78, 1);
		pCard->info_csd.read_misalign = VXB_UNSTUFF_BITS(pResp, 77, 1);
		pCard->info_csd.r2w_factor = VXB_UNSTUFF_BITS(pResp, 26, 3);
		pCard->info_csd.write_blkbits = VXB_UNSTUFF_BITS(pResp, 22, 4);
		pCard->info_csd.write_partial = VXB_UNSTUFF_BITS(pResp, 21, 1);
	}
	
	return 0;
}


int vxbSdmmcCard_decode_CSD_MMC(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CARD*)(&pDrvCtrl->CARD);
		
	UINT32 e, m;
	UINT32 * pResp;	
	
	pResp = pCard->raw_csd;
	
	/* mmc / eMMC */
	{
		pCard->info_csd.csd_struct = VXB_UNSTUFF_BITS(pResp, 126, 2);
		if (pCard->info_csd.csd_struct == 0)
		{
			return -FMSH_EINVAL;
		}

		pCard->info_csd.mmca_vsn	 = VXB_UNSTUFF_BITS(pResp, 122, 4);
		
		m = VXB_UNSTUFF_BITS(pResp, 115, 4);
		e = VXB_UNSTUFF_BITS(pResp, 112, 3);
		pCard->info_csd.taac_ns	 = (vxb_taac_exp[e] * vxb_taac_mant[m] + 9) / 10;
		pCard->info_csd.taac_clks	 = VXB_UNSTUFF_BITS(pResp, 104, 8) * 100;

		m = VXB_UNSTUFF_BITS(pResp, 99, 4);
		e = VXB_UNSTUFF_BITS(pResp, 96, 3);
		pCard->info_csd.max_dtr	  = vxb_tran_exp[e] * vxb_tran_mant[m]/10;		
		pCard->info_csd.cmdclass	  = VXB_UNSTUFF_BITS(pResp, 84, 12);

		e = VXB_UNSTUFF_BITS(pResp, 47, 3);
		m = VXB_UNSTUFF_BITS(pResp, 62, 2) | (VXB_UNSTUFF_BITS(pResp, 64, 10) << 2);
		pCard->info_csd.blockNR	  = (1 + m) << (e + 2);
		
		pCard->info_csd.read_blkbits = VXB_UNSTUFF_BITS(pResp, 80, 4);
		pCard->info_csd.read_partial = VXB_UNSTUFF_BITS(pResp, 79, 1);
		pCard->info_csd.write_misalign = VXB_UNSTUFF_BITS(pResp, 78, 1);
		pCard->info_csd.read_misalign  = VXB_UNSTUFF_BITS(pResp, 77, 1);
		pCard->info_csd.r2w_factor    = VXB_UNSTUFF_BITS(pResp, 26, 3);
		pCard->info_csd.write_blkbits = VXB_UNSTUFF_BITS(pResp, 22, 4);
		pCard->info_csd.write_partial = VXB_UNSTUFF_BITS(pResp, 21, 1);
	}
	
	return 0;
}

int vxbSdmmcCard_decode_CID(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CARD*)(&pDrvCtrl->CARD);
	
	UINT32 * pResp;
	
	pResp = pCard->raw_cid;
	
	if (pCard->card_type == SD)
	{
		/*
		 * SD doesn't currently have a version field so we will
		 * have to assume we can parse this.
		 */
		pCard->info_cid.manfid		    = VXB_UNSTUFF_BITS(pResp, 120, 8);
		pCard->info_cid.oemid			= VXB_UNSTUFF_BITS(pResp, 104, 16);
		pCard->info_cid.prod_name[0]		= VXB_UNSTUFF_BITS(pResp, 96, 8);
		pCard->info_cid.prod_name[1]		= VXB_UNSTUFF_BITS(pResp, 88, 8);
		pCard->info_cid.prod_name[2]		= VXB_UNSTUFF_BITS(pResp, 80, 8);
		pCard->info_cid.prod_name[3]		= VXB_UNSTUFF_BITS(pResp, 72, 8);
		pCard->info_cid.prod_name[4]		= VXB_UNSTUFF_BITS(pResp, 64, 8);
		pCard->info_cid.hwrev			= VXB_UNSTUFF_BITS(pResp, 60, 4);
		pCard->info_cid.fwrev			= VXB_UNSTUFF_BITS(pResp, 56, 4);
		pCard->info_cid.serial		    = VXB_UNSTUFF_BITS(pResp, 24, 32);
		pCard->info_cid.year			= VXB_UNSTUFF_BITS(pResp, 12, 8);
		pCard->info_cid.month			= VXB_UNSTUFF_BITS(pResp, 8, 4);

		pCard->info_cid.year += 2000; /* SD cards year offset */
	}
	else   /* mmc / eMMC */
	{
		/*
		 * The selection of the format here is based upon published
		 * specs from sandisk and from what people have reported.
		 */
		switch (pCard->info_csd.mmca_vsn)
		{
			case CSD_SPEC_VER_0: /* MMC v1.0 - v1.2 */
			case CSD_SPEC_VER_1: /* MMC v1.4 */
				pCard->info_cid.manfid	= VXB_UNSTUFF_BITS(pResp, 104, 24);
				pCard->info_cid.prod_name[0]	= VXB_UNSTUFF_BITS(pResp, 96, 8);
				pCard->info_cid.prod_name[1]	= VXB_UNSTUFF_BITS(pResp, 88, 8);
				pCard->info_cid.prod_name[2]	= VXB_UNSTUFF_BITS(pResp, 80, 8);
				pCard->info_cid.prod_name[3]	= VXB_UNSTUFF_BITS(pResp, 72, 8);
				pCard->info_cid.prod_name[4]	= VXB_UNSTUFF_BITS(pResp, 64, 8);
				pCard->info_cid.prod_name[5]	= VXB_UNSTUFF_BITS(pResp, 56, 8);
				pCard->info_cid.prod_name[6]	= VXB_UNSTUFF_BITS(pResp, 48, 8);
				pCard->info_cid.hwrev		= VXB_UNSTUFF_BITS(pResp, 44, 4);
				pCard->info_cid.fwrev		= VXB_UNSTUFF_BITS(pResp, 40, 4);
				pCard->info_cid.serial	    = VXB_UNSTUFF_BITS(pResp, 16, 24);
				pCard->info_cid.month		= VXB_UNSTUFF_BITS(pResp, 12, 4);
				pCard->info_cid.year		= VXB_UNSTUFF_BITS(pResp, 8, 4) + 1997;
				break;

			case CSD_SPEC_VER_2: /* MMC v2.0 - v2.2 */
			case CSD_SPEC_VER_3: /* MMC v3.1 - v3.3 */
			case CSD_SPEC_VER_4: /* MMC v4 */
				pCard->info_cid.manfid	    = VXB_UNSTUFF_BITS(pResp, 120, 8);
				pCard->info_cid.oemid		= VXB_UNSTUFF_BITS(pResp, 104, 16);
				pCard->info_cid.prod_name[0]	= VXB_UNSTUFF_BITS(pResp, 96, 8);
				pCard->info_cid.prod_name[1]	= VXB_UNSTUFF_BITS(pResp, 88, 8);
				pCard->info_cid.prod_name[2]	= VXB_UNSTUFF_BITS(pResp, 80, 8);
				pCard->info_cid.prod_name[3]	= VXB_UNSTUFF_BITS(pResp, 72, 8);
				pCard->info_cid.prod_name[4]	= VXB_UNSTUFF_BITS(pResp, 64, 8);
				pCard->info_cid.prod_name[5]	= VXB_UNSTUFF_BITS(pResp, 56, 8);
				pCard->info_cid.serial	    = VXB_UNSTUFF_BITS(pResp, 16, 32);
				pCard->info_cid.month		= VXB_UNSTUFF_BITS(pResp, 12, 4);
				pCard->info_cid.year		= VXB_UNSTUFF_BITS(pResp, 8, 4) + 1997;
				break;

			default:
				return -FMSH_EINVAL;
		}
	}
	
	return 0;
}

int vxbSdmmcCard_decode_CID_MMC(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CARD*)(&pDrvCtrl->CARD);
	
	UINT32 * pResp;
	
	pResp = pCard->raw_cid;
	
	/* mmc / eMMC */
	{
		/*
		 * The selection of the format here is based upon published
		 * specs from sandisk and from what people have reported.
		 */
		switch (pCard->info_csd.mmca_vsn)
		{
			case CSD_SPEC_VER_0: /* MMC v1.0 - v1.2 */
			case CSD_SPEC_VER_1: /* MMC v1.4 */
				pCard->info_cid.manfid	= VXB_UNSTUFF_BITS(pResp, 104, 24);
				pCard->info_cid.prod_name[0]	= VXB_UNSTUFF_BITS(pResp, 96, 8);
				pCard->info_cid.prod_name[1]	= VXB_UNSTUFF_BITS(pResp, 88, 8);
				pCard->info_cid.prod_name[2]	= VXB_UNSTUFF_BITS(pResp, 80, 8);
				pCard->info_cid.prod_name[3]	= VXB_UNSTUFF_BITS(pResp, 72, 8);
				pCard->info_cid.prod_name[4]	= VXB_UNSTUFF_BITS(pResp, 64, 8);
				pCard->info_cid.prod_name[5]	= VXB_UNSTUFF_BITS(pResp, 56, 8);
				pCard->info_cid.prod_name[6]	= VXB_UNSTUFF_BITS(pResp, 48, 8);
				pCard->info_cid.hwrev		= VXB_UNSTUFF_BITS(pResp, 44, 4);
				pCard->info_cid.fwrev		= VXB_UNSTUFF_BITS(pResp, 40, 4);
				pCard->info_cid.serial	= VXB_UNSTUFF_BITS(pResp, 16, 24);
				pCard->info_cid.month		= VXB_UNSTUFF_BITS(pResp, 12, 4);
				pCard->info_cid.year		= VXB_UNSTUFF_BITS(pResp, 8, 4) + 1997;
				break;

			case CSD_SPEC_VER_2: /* MMC v2.0 - v2.2 */
			case CSD_SPEC_VER_3: /* MMC v3.1 - v3.3 */
			case CSD_SPEC_VER_4: /* MMC v4 */
				pCard->info_cid.manfid	= VXB_UNSTUFF_BITS(pResp, 120, 8);
				pCard->info_cid.oemid		= VXB_UNSTUFF_BITS(pResp, 104, 16);
				pCard->info_cid.prod_name[0]	= VXB_UNSTUFF_BITS(pResp, 96, 8);
				pCard->info_cid.prod_name[1]	= VXB_UNSTUFF_BITS(pResp, 88, 8);
				pCard->info_cid.prod_name[2]	= VXB_UNSTUFF_BITS(pResp, 80, 8);
				pCard->info_cid.prod_name[3]	= VXB_UNSTUFF_BITS(pResp, 72, 8);
				pCard->info_cid.prod_name[4]	= VXB_UNSTUFF_BITS(pResp, 64, 8);
				pCard->info_cid.prod_name[5]	= VXB_UNSTUFF_BITS(pResp, 56, 8);
				pCard->info_cid.serial	= VXB_UNSTUFF_BITS(pResp, 16, 32);
				pCard->info_cid.month		= VXB_UNSTUFF_BITS(pResp, 12, 4);
				pCard->info_cid.year		= VXB_UNSTUFF_BITS(pResp, 8, 4) + 1997;
				break;

			default:
				return -FMSH_EINVAL;
		}
	}
	
	return 0;
}

UINT32 vxbSdmmcCard_Get_CardSize_KB(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CARD*)(&pDrvCtrl->CARD);
	
    if (pCard->capacity == 0xFFFFFFFF)
    {
        return pCard->info_csd.blockNR / 2;
    }
    
    return pCard->capacity / 1024;
}

/*****************************************************************************
*
* @description
* This function sets the bus width to the card.
*
* @param
*           pDev is the pointer to a sd card device.
*           bus_width is bus width.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int vxbSdmmcCard_Set_BusWidth(VXB_DEVICE_ID pDev, UINT32 bus_width)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CARD*)(&pDrvCtrl->CARD);
	
    int ret;
    int retry = 5;
    
    if (pCard->card_type == SD)
    {
        while (retry)
        {
            ret = vxbSdmmc_sdACMD6(pDev, bus_width);
            if (ret < 0)
            {
                if (ret == (-FMSH_ENODEV))
                {
					VX_DBG2("SD ACMD6 failed! \n", 1,2,3,4,5,6);
                    return ret;
                }
                
                retry--;				
				VX_DBG("retry to SD set buswidth \n", 1,2,3,4,5,6);
            }
            else
            {
                break;
            }
        }
		
        if (ret < 0)
        {
            VX_DBG2("ACMD6 failed! \n", 1,2,3,4,5,6);
            return ret;
        }
    }
    else
    {
        if (pCard->info_csd.mmca_vsn >= CSD_SPEC_VER_4)
        {
            while (retry)
            {
                ret = vxbSdmmc_cmd6_mmc(pDev, MMC_4_BIT_BUS_ARG);
                if (ret < 0)
                {
                    if (ret == (-FMSH_ENODEV))
                    {
                    
						VX_DBG2("MMC ACMD6 failed! \n", 1,2,3,4,5,6);
                        return ret;
                    }
                    
                    retry--;					
					VX_DBG("retry to MMC set buswidth \n", 1,2,3,4,5,6);
                }
                else
                {
                    break;
                }
            }
			
            if (ret < 0)
            {
				VX_DBG2("ACMD6_mmc failed! \n", 1,2,3,4,5,6);
                return ret;
            }
        }
    }
	
    switch (bus_width)
    {       
    case SD_BUS_WIDTH_4:
        vxbSdmmcCtrl_WrReg32(pDev, SDMMC_CTYPE, 0x1);
    	break;  
	
    case SD_BUS_WIDTH_1:
        vxbSdmmcCtrl_WrReg32(pDev, SDMMC_CTYPE, 0);
    	break;  
	
    default:
        vxbSdmmcCtrl_WrReg32(pDev, SDMMC_CTYPE, 0);
    	break;
    }   

    return ret;
}

int vxbSdmmcCard_Set_BusWidth_MMC(VXB_DEVICE_ID pDev, UINT32 bus_width)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CARD*)(&pDrvCtrl->CARD);
	
    int ret;
    int retry = 5;
	
    if (pCard->info_csd.mmca_vsn >= CSD_SPEC_VER_4)
    {
        while (retry)
        {
            ret = vxbSdmmc_cmd6_mmc(pDev, MMC_4_BIT_BUS_ARG);
            if (ret < 0)
            {
                if (ret == (-FMSH_ENODEV))
                {
                
					VX_DBG2("MMC ACMD6 failed! \n", 1,2,3,4,5,6);
                    return ret;
                }
                
                retry--;					
				VX_DBG("retry to MMC set buswidth \n", 1,2,3,4,5,6);
            }
            else
            {
                break;
            }
        }
		
        if (ret < 0)
        {
			VX_DBG2("ACMD6_mmc fail! \n", 1,2,3,4,5,6);
            return ret;
        }
		else
		{
			VX_DBG2("ACMD6_mmc ok! \n", 1,2,3,4,5,6);
		}
    }
	
    /*pCard->setBuswidth(pDev, bus_width);*/
    switch (bus_width)
    {       
    case SD_BUS_WIDTH_4:
        vxbSdmmcCtrl_WrReg32(pDev, SDMMC_CTYPE, 0x1);
    	break;  
	
    case SD_BUS_WIDTH_1:
        vxbSdmmcCtrl_WrReg32(pDev, SDMMC_CTYPE, 0);
    	break;  
	
    default:
        vxbSdmmcCtrl_WrReg32(pDev, SDMMC_CTYPE, 0);
    	break;
    }   

    return ret;
}


int vxbSdmmcCard_Get_Info_CSD(VXB_DEVICE_ID pDev) 
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CARD*)(&pDrvCtrl->CARD);

	UINT32 * pResp;
	
	pResp = pCard->raw_csd;
	
	/* read/write block length */
	pCard->read_blk_len	= 1 << pCard->info_csd.read_blkbits;
	pCard->write_blk_len = 1 << pCard->info_csd.write_blkbits;
	
	printf("vxbSdmmcCard_Get_Info_CSD->pInstance->read_blk_len:%d \n", pInstance->read_blk_len);
	printf("vxbSdmmcCard_Get_Info_CSD->pInstance->write_blk_len:%d \n", pInstance->write_blk_len);

	if (pCard->read_blk_len != pCard->write_blk_len)
	{
		return (-FMSH_EINVAL);
	}

	/* partial block read/write I/O support */
	if (pCard->info_csd.read_partial) 
		pCard->status |= MMC_READ_PART;
	
	if (pCard->info_csd.write_partial) 
		pCard->status |= MMC_WRITE_PART;

	/* calculate total card size in bytes */
    if (pCard->highCapacity == 1)
		pCard->capacity = 0xFFFFFFFF; /* SDHC/SDXC may larger than 32bits*/
	else
		pCard->capacity = pCard->info_csd.blockNR * pCard->read_blk_len;

	return 0;
}

/*****************************************************************************
*
* @description
* This function sets the block length to the card.
*
* @param
*           pDev is the pointer to a sd card device.
*           len is block length in bytes.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int vxbSdmmcCard_Set_BlockSize(VXB_DEVICE_ID pDev, int block_size)
{
    int ret = 0;
    int retry = 100;

    while (retry)
    {
        ret = vxbSdmmc_cmd16(pDev, block_size);
        if(ret < 0)
        {
            delay_us(10);
            retry--;
        }
        else
        {
            break;
        }
    }
    
    if (ret < 0)
    {    
    	VX_DBG2("Cmd 16 error, set block_size(%d) fail! \n", block_size, 2,3,4,5,6);
    }
	
    return ret;
}

/*****************************************************************************
*
* @description
* This function gets bus speed supported by card.
*
* @param
*           pDev is the pointer to a sd card device.
*           pBuf is buffer to store function group support data 
*                   returned by card.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int sdmmcCar_Get_BusSpeed(VXB_DEVICE_ID pDev, UINT8 * pBuf)
{
    int ret;
    UINT32 arg;

    arg = SD_SWITCH_CMD_HS_GET;
    ret = vxbSdmmc_cmd6_sd(pDev, arg, pBuf);

    return ret;
}


int vxbSdmmcCard_Get_Info_SCR(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CARD*)(&pDrvCtrl->CARD);
	
    int ret;
	int i = 0;
    UINT32 scr_struct;

    memset(pCard->raw_scr, 0, (8*sizeof(UINT8)));
	
    ret = vxbSdmmc_sdACMD51(pDev, pCard->raw_scr);
    if (ret < 0)
    {
        return ret;
    }

    VX_DBG("scr is: 0x", 1,2,3,4,5,6);
	for (i=0; i<=7; i++)
	{
	    VX_DBG("%02X_", pCard->raw_scr[i], 2,3,4,5,6);
	}
    VX_DBG(" \n", 1,2,3,4,5,6);

    scr_struct = (pCard->raw_scr[0] >> 4) & 0xF;
    if (scr_struct != 0)
    {
        return -FMSH_EINVAL;
    }
	
    pCard->info_scr.sd_spec   = pCard->raw_scr[0] & 0xF;
    pCard->info_scr.bus_width = pCard->raw_scr[1] & 0xF;
    pCard->info_scr.sd_spec3 = (pCard->raw_scr[2] >> 7) & 0x1;
    
    return ret;
}

/*****************************************************************************
*
* @description
* This function sends command 8 to get the EXT_CSD register
* as a block of data, only used for MMC card 4.0.
*
* @param
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int vxbSdmmcCard_Get_ExtCsd_MMC(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CARD*)(&pDrvCtrl->CARD);
	
    int ret;
	int retry = 5;
	
    if (pCard->info_csd.mmca_vsn >= CSD_SPEC_VER_4)
    {
        /* get EXT CSD info */
        retry = 5;
        
        while (retry)
        {
            memset(pCard->ext_csd, 0, (512*sizeof(UINT8)));
			
            ret = vxbSdmmc_cmd8_mmc(pDev, pCard->ext_csd);
            if (ret < 0)
            {
                retry--;
                if (retry <= 0)
                {    
                	return ret;
                }
            }
            break;
        }
    }

    return 0;
}

#endif



#if 1

int vxbSdmmcSet_Clk_DataRate(VXB_DEVICE_ID pDev, UINT32 rate)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;
	vxbT_SDMMC_CTRL* pCtrl = (vxbT_SDMMC_CTRL*)(&pDrvCtrl->CTRL);
	
	int ret = 0;
	UINT32 tmp32 = 0;
	
	int clk_div = 0, div_cnt = 0, sd_clk = 0;
	int timeout = 0;
	int ctrl_x = 0;
	
	/*rate = SDMMC_CLK_400_KHZ;*/
	ctrl_x = pCtrl->ctrl_x;	
	sd_clk = pCtrl->sysClkHz;   /* 100M Hz*/

	if (rate >= sd_clk)
	{
		clk_div = 0;
	}
	else
	{
		if (rate != 0)
		{
			/*clk_div = (sd_clk / (2 *rate) + 1);*/
			for (div_cnt=1; div_cnt<=SDMMC_MAX_DIV_CNT; div_cnt++)
			{
				if ((sd_clk / (2 * div_cnt) <= rate))
				{
					clk_div = div_cnt;
					break;
				}
			}
		}
	}

	/**/
	/* disable clock */
	/**/
	vxbSdmmcCtrl_WrReg32(pDev, SDMMC_CLKENA, 0);
	tmp32 = START_CMD + UPDATE_CLOCK_REG_ONLY + WAIT_PRVDATA_COMPLETE;
	vxbSdmmcCtrl_WrReg32(pDev, SDMMC_CMD_VAL, tmp32);
	ret = vxbSdmmcCtrl_wait_StartCmd_Ok(pDev);
	if (ret < 0)
	{
		VX_DBG2("sdmmc ctrl_%d: disable SDMMC_CLKENA fail, exit! \n", ctrl_x, 2,3,4,5,6);
		return (-FMSH_ETIME);
	}
	
	/**/
	/* set clock */
	/**/
	vxbSdmmcCtrl_WrReg32(pDev, SDMMC_CLKSRC, 0);
	vxbSdmmcCtrl_WrReg32(pDev, SDMMC_CLKDIV, clk_div);
	tmp32 = START_CMD + UPDATE_CLOCK_REG_ONLY + WAIT_PRVDATA_COMPLETE;
	vxbSdmmcCtrl_WrReg32(pDev, SDMMC_CMD_VAL, tmp32);	
	ret = vxbSdmmcCtrl_wait_StartCmd_Ok(pDev);
	if (ret < 0)
	{
		VX_DBG2("sdmmc ctrl_%d: set SDMMC_CLKSRC fail, exit! \n", ctrl_x, 2,3,4,5,6);
		return (-FMSH_ETIME);
	}

	/**/
	/* enable clock */
	/**/
	vxbSdmmcCtrl_WrReg32(pDev, SDMMC_CLKENA, 0xFFFF);
	tmp32 = START_CMD + UPDATE_CLOCK_REG_ONLY + WAIT_PRVDATA_COMPLETE;
	vxbSdmmcCtrl_WrReg32(pDev, SDMMC_CMD_VAL, tmp32);
	ret = vxbSdmmcCtrl_wait_StartCmd_Ok(pDev);
	if (ret < 0)
	{
		VX_DBG2("sdmmc ctrl_%d: enable SDMMC_CLKENA fail, exit! \n", ctrl_x, 2,3,4,5,6);
		return (-FMSH_ETIME);
	}
	
    delay_us(10);

	return 0;
}












/*****************************************************************************
*
* @description
* This function to set high speed in card and host.
* Changes clock in host accordingly.
*
* @param
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int vxbSdmmcChg_BusSpeed(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;

	vxbT_SDMMC_CTRL* pCtrl = (vxbT_SDMMC_CTRL*)(&pDrvCtrl->CTRL);
	vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CARD*)(&pDrvCtrl->CARD);
		
    int ret;
    UINT32 arg;	
    UINT8 readBuf[64];
    
    if (pCard->card_type == SD)
    {
        arg = SD_SWITCH_CMD_HS_SET;
        ret = vxbSdmmc_cmd6_sd(pDev, arg, readBuf);
        if (ret < 0)
        {
            return ret;
        }
        
		ret = vxbSdmmcSet_Clk_DataRate(pDev, SDMMC_CLK_25_MHZ);  /* SDMMC_CLK_50_MHZ */
        if (ret < 0)
        {
            return ret;
        }
    }
    else if ((pCard->card_type == MMC) || (pCard->card_type == EMMC))
    {
        arg = MMC_HIGH_SPEED_ARG;
        ret = vxbSdmmc_cmd6_mmc(pDev, arg);
        if (ret < 0)
        {
            return ret;
        }
        
		ret = vxbSdmmcSet_Clk_DataRate(pDev, SDMMC_CLK_25_MHZ); /* SDMMC_CLK_52_MHZ */
        if (ret < 0)
        {
            return ret;
        }
    }

    return ret;
}


int vxbSdmmcChg_BusSpeed_MMC(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;

	vxbT_SDMMC_CTRL* pCtrl = (vxbT_SDMMC_CTRL*)(&pDrvCtrl->CTRL);
	vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CARD*)(&pDrvCtrl->CARD);
		
    int ret;
    UINT32 arg;	
    UINT8 readBuf[64];
    
    arg = MMC_HIGH_SPEED_ARG;
    ret = vxbSdmmc_cmd6_mmc(pDev, arg);
	
    if (ret < 0)
    {
        return ret;
    }
    
	ret = vxbSdmmcSet_Clk_DataRate(pDev, SDMMC_CLK_25_MHZ); /* SDMMC_CLK_52_MHZ */
    if (ret < 0)
    {
        return ret;
    }

    return ret;
}


#endif


#define __ENUM__

#if 1
/*****************************************************************************
*
* @description
* This function runs the MMC card and emmc enumeration sequence.
* This function runs after the initialization and identification procedure.
* It gets all necessary information from the card.
*
* @param
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int vxbSdmmc_Enumerate_MMC(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;

	vxbT_SDMMC_CTRL* pCtrl = (vxbT_SDMMC_CTRL*)(&pDrvCtrl->CTRL);
	vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CARD*)(&pDrvCtrl->CARD);
	vxbT_SDMMC_CMD* pCmd   = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);;
	
    int ret;
    int retry = 10000;
	
    
    pCard->highCapacity = 0;

    /* 
    SEND_OP_COND 
	*/
    ret = vxbSdmmc_cmd1(pDev);
    if(ret == -FMSH_ENODEV)
    {
        return ret;
    }
    delay_ms(1);
	
    pCard->ocr = pCmd->rsp_buf[0];
	
    while (!(pCard->ocr & MMC_CARD_BUSY))
    {
        ret = vxbSdmmc_cmd1(pDev);
        if(ret == -FMSH_ENODEV)
        {
            return ret;
        }
		
        pCard->ocr = pCmd->rsp_buf[0];
		
        retry--;
        if (retry == 0)
        {
            ret = -FMSH_ETIME;
            return ret;
        }
		
        delay_ms(1);
    }
	
	VX_DBG("sdmmc ctrl_%d: CMD1 response success, Card is unbusy, retry[%d]! \n", pCtrl->ctrl_x, retry, 3,4,5,6);
	VX_DBG("--ctrl(%d)cmd1-- \n\n", pCtrl->ctrl_x, 2,3,4,5,6);
	
    /* 
    ALL_SEND_CID 
	*/
    ret = vxbSdmmc_cmd2(pDev);
    if (ret < 0) 
    {
        return ret;
    }
	VX_DBG("--ctrl(%d)cmd2-- \n\n", pCtrl->ctrl_x, 2,3,4,5,6);
	
    memcpy(pCard->raw_cid, pCmd->rsp_buf, (4*sizeof(UINT32)));

    /* 
    SET_RELATIVE_ADDR, Stand-by State 
	*/
    /*pInstance->rca = 2; /* slot id begins at 0 */
    
    ret = vxbSdmmc_cmd3(pDev);
    if (ret < 0)
    {
        return ret;
    }
	VX_DBG("mmc cmd3->g_pCard->rca: 0x%X \n", pCard->rca, 2,3,4,5,6);
	VX_DBG("--ctrl(%d)cmd3-- \n\n", pCtrl->ctrl_x, 2,3,4,5,6);
	
    if (!(pCmd->rsp_buf[0] & STATE_IDENT))
    {
		VX_DBG2("sdmmc ctrl_%d: CMD3 response failed\! \n", pCtrl->ctrl_x, 2,3,4,5,6);
        return -FMSH_ENODEV;
    }

    /* 
    SEND_CSD, Stand-by State 
	*/
    ret = vxbSdmmc_cmd9(pDev);
    if (ret < 0)
    {
        return ret;
    }
	
    VX_DBG("csd[0] is 0x%08x\r\n", pCmd->rsp_buf[0], 2,3,4,5,6);
    VX_DBG("csd[1] is 0x%08x\r\n", pCmd->rsp_buf[1], 2,3,4,5,6);
    VX_DBG("csd[2] is 0x%08x\r\n", pCmd->rsp_buf[2], 2,3,4,5,6);
    VX_DBG("csd[3] is 0x%08x\r\n", pCmd->rsp_buf[3], 2,3,4,5,6);
	
    VX_DBG("g_pCard->rca:0x%08X \n", pCard->rca, 2,3,4,5,6);
	VX_DBG("--ctrl(%d)cmd9-- \n\n", pCtrl->ctrl_x, 2,3,4,5,6);
	
    memcpy(pCard->raw_csd, pCmd->rsp_buf, (4*sizeof(UINT32)));
    
    ret = vxbSdmmcCard_decode_CSD_MMC(pDev);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: unrecognised CSD structure version! \n", pCtrl->ctrl_x, 2,3,4,5,6);
        return ret;
    }
    
    ret = vxbSdmmcCard_decode_CID_MMC(pDev);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: card has unknown MMCA version! \n", pCtrl->ctrl_x, 2,3,4,5,6);
    }
    
    ret = vxbSdmmcCard_Get_Info_CSD(pDev);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: read_blk_len and write_blk_len are not equal! \n", pCtrl->ctrl_x, 2,3,4,5,6);
    }
	
	VX_DBG("sdmmc ctrl_%d: -I- MMC Card OK, size: %d MB! \n", \
	       pCtrl->ctrl_x, vxbSdmmcCard_Get_CardSize_KB(pDev)/1024, 3,4,5,6);

    return 0;
}


/*****************************************************************************
*
* @description
* This function runs the SD card enumeration sequence.
* This function runs after the initialization and identification procedure.
* It gets all necessary information from the card.
*
* @param
*           pDev is the pointer to a sd card device.
*
* @return
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note     NA.
*
*****************************************************************************/
int vxbSdmmc_Enumerate_SD(VXB_DEVICE_ID pDev)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;

	vxbT_SDMMC_CTRL* pCtrl = (vxbT_SDMMC_CTRL*)(&pDrvCtrl->CTRL);
	vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CARD*)(&pDrvCtrl->CARD);
	vxbT_SDMMC_CMD* pCmd   = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);;
	
    int ret;
    int retry = 20000;	
    
    ret = vxbSdmmc_appCmd41(pDev);
    if(ret < 0)
    {
        VX_DBG2("ACMD41 failed: %d\r\n", ret, 2,3,4,5,6);
        return ret;
    }
    
    pCard->ocr = pCmd->rsp_buf[0];
	
    while (!(pCard->ocr & MMC_CARD_BUSY))
    {
        ret = vxbSdmmc_sdACMD41(pDev);
        if(ret < 0)
        {
            VX_DBG2("ACMD41 failed: %d\r\n", ret, 2,3,4,5,6);
            return ret;
        }
		
        pCard->ocr = pCmd->rsp_buf[0];
		
        retry--;
        if (retry == 0)
        {
            ret = (-FMSH_ETIME);
            return ret;
        }
    }
	
    VX_DBG("ACMD41 response success, Card is unbusy, retry[%d]! \n", retry, 2,3,4,5,6);
    
    if(pCard->ocr & 0x40000000)
    {
        VX_DBG("high capacity sd card \n", 1,2,3,4,5,6);
        pCard->highCapacity = 1;
    }
    else
    {
        VX_DBG("normal capacity sd card \n", 1,2,3,4,5,6);
        pCard->highCapacity = 0;
    }
	
    /* 
    ALL_SEND_CID 
	*/
    ret = vxbSdmmc_cmd2(pDev);
    if (ret < 0)
    {
		VX_DBG2("vxbSdmmc_cmd2 failed: %d \n", ret, 2,3,4,5,6);
        return ret;
    }
		
    VX_DBG("cid[0] is 0x%08x\r\n", pCmd->rsp_buf[0], 2,3,4,5,6);
    VX_DBG("cid[1] is 0x%08x\r\n", pCmd->rsp_buf[1], 2,3,4,5,6);
    VX_DBG("cid[2] is 0x%08x\r\n", pCmd->rsp_buf[2], 2,3,4,5,6);
    VX_DBG("cid[3] is 0x%08x\r\n", pCmd->rsp_buf[3], 2,3,4,5,6);
	
    memcpy(pCard->raw_cid, pCmd->rsp_buf, (4*sizeof(UINT32)));
    
    /*
    SET_RELATIVE_ADDR, Stand-by State 
    */
    pCard->rca = 1 ; /* slot id begins at 0 */
	
    ret = vxbSdmmc_cmd3(pDev);
    if (ret < 0)
    {
		VX_DBG2("vxbSdmmc_cmd3 failed: %d \n", ret, 2,3,4,5,6);
        return ret;
    }
    pCard->rca = pCmd->rsp_buf[0] >> 16;
	
	VX_DBG("sd cmd3->g_pCard->rca: 0x%X \n", pCard->rca, 2,3,4,5,6);
	
    if (!(pCmd->rsp_buf[0] & STATE_IDENT))
    {
		VX_DBG2("vxbSdmmc_cmd3-STATE_IDENT: 0x%08X\r\n", pCmd->rsp_buf[0], 2,3,4,5,6);
        return (-FMSH_ENODEV);
    }
	
    /* 
    SEND_CSD, Stand-by State 
	*/
    ret = vxbSdmmc_cmd9(pDev);
    if (ret < 0)
    {
		VX_DBG2("vxbSdmmc_cmd9 failed: %d \n", ret, 2,3,4,5,6);
        return ret;
    }
	
    VX_DBG("\ncsd[0] is 0x%08x\r\n", pCmd->rsp_buf[0], 2,3,4,5,6);
    VX_DBG("csd[1] is 0x%08x\r\n", pCmd->rsp_buf[1], 2,3,4,5,6);
    VX_DBG("csd[2] is 0x%08x\r\n", pCmd->rsp_buf[2], 2,3,4,5,6);
    VX_DBG("csd[3] is 0x%08x\r\n", pCmd->rsp_buf[3], 2,3,4,5,6);
	
    memcpy(pCard->raw_csd, pCmd->rsp_buf, (4*sizeof(UINT32)));
	
    ret = vxbSdmmcCard_decode_CSD(pDev);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: unrecognised CSD structure version! \n", pCtrl->ctrl_x, 2,3,4,5,6);
        return ret;
    }
    
    ret = vxbSdmmcCard_decode_CID(pDev);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: card has unknown MMCA version! \n", pCtrl->ctrl_x, 2,3,4,5,6);
    }
    
    ret = vxbSdmmcCard_Get_Info_CSD(pDev);
    if (ret < 0)
    {
		VX_DBG2("sdmmc ctrl_%d: read_blk_len and write_blk_len are not equal! \n", pCtrl->ctrl_x, 2,3,4,5,6);
		return ret;
    }

	
	VX_DBG("sdmmc ctrl_%d: -I- SD Card OK, size: %d MB! \n", \
	       pCtrl->ctrl_x, vxbSdmmcCard_Get_CardSize_KB(pDev)/1024, 3,4,5,6);
	
    return 0;    
}





/*
UINT32 addr_offset: sector_idx
*/
int vxbSdmmc_WrSector_Poll(VXB_DEVICE_ID pDev, UINT8 * pBuf, UINT32 addr_offset, UINT32 len)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;

	vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CARD*)(&pDrvCtrl->CARD);
	vxbT_SDMMC_CMD* pCmd   = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);;
		
    int ret;

    if (((pCard->card_type == MMC)  || (pCard->card_type == EMMC)) \
		 && (len > 0x200))
    {
        ret = vxbSdmmc_cmd23(pDev, len);
        if (ret)
        {
            VX_DBG2("send cmd23 error! \n", 1,2,3,4,5,6);
            return ret;
        }
    }
    
    /* write*/
    pCmd->cmd_idx = len > 512 ? CMD_WRITE_MULTIPLE_BLOCK : CMD_WRITE_BLOCK;

    if(pCard->highCapacity == 0)  
        pCmd->cmd_arg = addr_offset << 9; /* block size: 512Bytes*/
    else
        pCmd->cmd_arg = addr_offset;      /* unit: block number  //hc, xc*/
	
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_WDATA;
	
    pCmd->txRxBuf = pBuf;
    pCmd->data_len = len;
    pCmd->block_size = 512;
	
	ret = vxbSdmmc_sndCmd(pDev, pCmd);
    if (ret < 0)
    {
		VX_DBG2("send CMD%d fail! \n", pCmd->cmd_idx,2,3,4,5,6);
        return ret;
    }	
	
	/* 
	check card-state. if card-state != StandBy, return BUSY 
	*/
    do
    {
        ret = vxbSdmmc_cmd13(pDev);
    } while (pCmd->rsp_buf[0] != 0x900);

    return len;
}


/*
UINT32 addr_offset: sector_idx
*/
int vxbSdmmc_RdSector_Poll(VXB_DEVICE_ID pDev, UINT8 * pBuf, UINT32 addr_offset, UINT32 len)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;

	vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CARD*)(&pDrvCtrl->CARD);
	vxbT_SDMMC_CMD* pCmd   = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);;
		
    int ret;

    if (((pCard->card_type == MMC)  || (pCard->card_type == EMMC)) \
		 && (len > 0x200))
    {
        ret = vxbSdmmc_cmd23(pDev, len);
        if (ret)
        {
            VX_DBG2("send cmd23 error! \n", 1,2,3,4,5,6);
            return ret;
        }
    }

    /* read*/
    pCmd->cmd_idx = len > 512 ? CMD_READ_MULTIPLE_BLOCK : CMD_READ_SINGLE_BLOCK;
	
    if(pCard->highCapacity == 0)  
        pCmd->cmd_arg = addr_offset << 9; /* block size: 512Bytes*/
    else
        pCmd->cmd_arg = addr_offset;      /* unit: block number  //hc, xc*/
        
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_RDATA;
	
    pCmd->txRxBuf = pBuf;
    pCmd->data_len = len;
    pCmd->block_size = 512;
	
	ret = vxbSdmmc_sndCmd(pDev, pCmd);
    if (ret < 0)
    {
		VX_DBG2("send CMD%d fail! \n", pCmd->cmd_idx,2,3,4,5,6);
        return ret;
    }
	
	/* 
	check card-state. if card-state != StandBy, return BUSY 
	*/
    do
    {
        ret = vxbSdmmc_cmd13(pDev);
    } while (pCmd->rsp_buf[0] != 0x900);

    return len;
}



/*
UINT32 addr_offset: sector_idx
*/
int vxbSdmmc_WrSector_Poll2(VXB_DEVICE_ID pDev, UINT8 * pBuf, UINT32 addr_offset, UINT32 len)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;

	vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CARD*)(&pDrvCtrl->CARD);
	vxbT_SDMMC_CMD* pCmd   = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);;
		
    int ret = OK;

    if (((pCard->card_type == MMC)  || (pCard->card_type == EMMC)) \
		 && (len > 0x200))
    {
        ret = vxbSdmmc_cmd23(pDev, len);
        if (ret)
        {
            VX_DBG2("send cmd23 error! \n", 1,2,3,4,5,6);
            return ret;
        }
    }
    
    /* write*/
    pCmd->cmd_idx = len > 512 ? CMD_WRITE_MULTIPLE_BLOCK : CMD_WRITE_BLOCK;

    if(pCard->highCapacity == 0)  
        pCmd->cmd_arg = addr_offset << 9; /* block size: 512Bytes*/
    else
        pCmd->cmd_arg = addr_offset;      /* unit: block number  //hc, xc*/
	
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_WDATA;
	
    pCmd->txRxBuf = pBuf;
    pCmd->data_len = len;
    pCmd->block_size = 512;
	
	ret = vxbSdmmc_sndCmd(pDev, pCmd);
    if (ret < 0)
    {
		VX_DBG2("send CMD%d fail! \n", pCmd->cmd_idx,2,3,4,5,6);
        return ret;
    }	
	
	/* 
	check card-state. if card-state != StandBy, return BUSY 
	*/
    do
    {
        ret = vxbSdmmc_cmd13(pDev);
    } while (pCmd->rsp_buf[0] != 0x900);

    return ret;
}


/*
UINT32 addr_offset: sector_idx
*/
int vxbSdmmc_RdSector_Poll2(VXB_DEVICE_ID pDev, UINT8 * pBuf, UINT32 addr_offset, UINT32 len)
{
	FM_SDMMC_DRV_CTRL2* pDrvCtrl = (FM_SDMMC_DRV_CTRL2*)pDev->pDrvCtrl;

	vxbT_SDMMC_CARD* pCard = (vxbT_SDMMC_CARD*)(&pDrvCtrl->CARD);
	vxbT_SDMMC_CMD* pCmd   = (vxbT_SDMMC_CMD*)(&pDrvCtrl->CMD);;
	
    int ret = OK;

    if (((pCard->card_type == MMC)  || (pCard->card_type == EMMC)) \
		 && (len > 0x200))
    {
        ret = vxbSdmmc_cmd23(pDev, len);
        if (ret)
        {
            VX_DBG2("send cmd23 error! \n", 1,2,3,4,5,6);
            return ret;
        }
    }

    /* read*/
    pCmd->cmd_idx = len > 512 ? CMD_READ_MULTIPLE_BLOCK : CMD_READ_SINGLE_BLOCK;
	
    if(pCard->highCapacity == 0)  
        pCmd->cmd_arg = addr_offset << 9; /* block size: 512Bytes*/
    else
        pCmd->cmd_arg = addr_offset;      /* unit: block number  //hc, xc*/
        
    pCmd->rsp_type = CMD_RESP_TYPE_R1;
    pCmd->rsp_flag = CMD_RESP_FLAG_RDATA;
	
    pCmd->txRxBuf = pBuf;
    pCmd->data_len = len;
    pCmd->block_size = 512;
	
	ret = vxbSdmmc_sndCmd(pDev, pCmd);
    if (ret < 0)
    {
		VX_DBG2("send CMD%d fail! \n", pCmd->cmd_idx,2,3,4,5,6);
        return ret;
    }
	
	/* 
	check card-state. if card-state != StandBy, return BUSY 
	*/
    do
    {
        ret = vxbSdmmc_cmd13(pDev);
    } while (pCmd->rsp_buf[0] != 0x900);

    return ret;
}

#endif


/*******************************************************************************
test code & sample 
*/
#if 1
int g_test_sd2 = 0;


STATUS test_rd_sector_vx0(UINT32 blkStart)
{
    VXB_DEVICE_ID       pDev;
    FM_SDMMC_DRV_CTRL2 * pDrvCtrl;
	
    UINT32              i;
	UINT8 buf8[1024] = {0};
	
	int unit = 0;
	
    pDev = vxbInstByNameFind (FM_SDMMC_NAME, unit);
    if (pDev == NULL)
    {
        printf ("Can not find this sd unit(%d)!  \n", unit);
        return ERROR;
    }
	else
	{
		/*printf ("vxbInstByNameFind(%s): 0x%X \n", FM_SDMMC_NAME, pDev);*/
	}
	
    pDrvCtrl= (FM_SDMMC_DRV_CTRL2 *) pDev->pDrvCtrl;
	
	printf("InstInit2->regBase: 0x%08X \n", pDrvCtrl->regBase);

    if (vxbSdmmc_RdSector_Poll2 (pDev, buf8, blkStart, 512) == ERROR)
    {
        printf ("Read error, sector(%d)!!! \n", blkStart);
        return ERROR;
    }
	else
	{
		printf("\n--------sd(%d)_read(%d)-----------\n", unit, blkStart);
		
		for (i=0; i<SDMMC_BLOCK_SIZE; i++)
		{
			printf("%02X", buf8[i]);
			
			if ((i+1)%4 == 0)
			{
				printf(" ");
			}
			
			if ((i+1)%16 == 0)
			{
				printf(" \n");
			}
		}
		
		printf("------------------- \n");
	}
	
    return OK;
}	


STATUS test_wr_sector_vx0(UINT32 blkStart)
{
    VXB_DEVICE_ID       pDev;
    FM_SDMMC_DRV_CTRL2 * pDrvCtrl;

    UINT32              i;
	UINT8 buf8[1024] = {0};
	
	int unit = 0;
	
    pDev = vxbInstByNameFind (FM_SDMMC_NAME, unit);
    if (pDev == NULL)
    {
        printf ("Can not find this sd unit(%d)!  \n", unit);
        return ERROR;
    }
	else
	{
		/*printf ("vxbInstByNameFind(%s): 0x%X \n", FM_SDMMC_NAME, pDev);*/
	}
	
    pDrvCtrl= (FM_SDMMC_DRV_CTRL2 *) pDev->pDrvCtrl;
	
	/*printf("InstInit2->regBase: 0x%08X \n", pDrvCtrl->regBase);*/

	/*////////////////////////////////*/
	for (i=0; i<SDMMC_BLOCK_SIZE; i++)
	{
		buf8[i] = i + g_test_sd2;
	}
	g_test_sd2++;
	
	buf8[4] = blkStart;
	buf8[5] = blkStart;
	buf8[6] = blkStart;
	buf8[7] = blkStart;

    if (vxbSdmmc_WrSector_Poll2(pDev, buf8, blkStart, 512) == ERROR)
    {
        printf ("Write error, sector(%d)!!! \n", blkStart);
        return ERROR;
    }
	else
	{
		printf("\n--------sd(%d)_write(%d)-----------\n", unit, blkStart);
		
		for (i=0; i<SDMMC_BLOCK_SIZE; i++)
		{
			printf("%02X", buf8[i]);
			
			if ((i+1)%4 == 0)
			{
				printf(" ");
			}
			
			if ((i+1)%16 == 0)
			{
				printf(" \n");
			}
		}
		
		printf("------------------- \n");
	}
	
    return OK;
}
#endif


