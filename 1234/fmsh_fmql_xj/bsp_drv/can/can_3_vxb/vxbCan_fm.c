/* vxCan_fm.c - FMSH CAN Controller Driver */

/*
 * Copyright (c) 2012, 2013 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01a,23Dec19,jc  created.
*/

/*
DESCRIPTION

The FMSH SPI is a high-speed serial input/output port that allows
a serial bit stream of programmed length (4 to 32 bits) to be shifted into and
out of the device at a programmed bit-transfer rate. The SPI is normally used
for communication between the device and external peripherals. Typical
applications include interface to external I/O or peripheral expansion via
devices such as shift registers, display drivers, SPI EPROMS, SPI Flash and
analog-to-digital converters.

This module implements a driver for the SPI controller present on many
TI ARM processors. The controller is capable of acting as a master
mode. This driver supports working on polling, interrupt or DMA mode,
meanwhile, the driver can dynamically switch clock frequency, duplex mode and
chipSelect according to the device requirement. The McSPI controller has
a built-in 64 bytes buffer in order to unload DMA or interrupt handler and
improve data throughput. The driver unconditionally supports the FIFO.

EXTERNAL INTERFACE
The driver provides a vxBus external interface. The only exported
routine is the vxCanRegister(), which registers the driver with
VxBus. Since the SPIs are processor local bus devices, each device
instance must be specified in the hwconf.c file in a BSP. The hwconf entry
must specify the following parameters:

\is
\i <regBase>
Specifies the base address where the controller's CSR registers are mapped
into the host's address space. All register offsets are computed relative
to this address.
\ie

\is
\i <clkFreq>
Specifies the clock frequency of the controller.
\ie

\is
\i <polling>
Specifies driver mode, TRUE for polling mode, FALSE for the interrupt mode, if
this is not supplied, the interrupt mode will be used.
\ie

\is
\i <dmaMode>
Specifies driver use DMA or not, TRUE for DMA mode, FALSE for non-DMA mode, if
this is not supplied, the DMA mode will not be used. If dmaMode is TRUE, the
driver DRV_TI_EDMA3 should be included. If the driver DRV_TI_EDMA3 is not
included, the SPI driver will work in the mode that specified by
<polling>.
\ie

\is
\i <spiDev>
Specifies the pointer of the supported device table(spiDevTbl[]) on this SPI
controller. It should be implemented, otherwise means no spi device connect on
this controller.
\ie

\is
\i <spiDevNum>
Specifies the spi device table size, how many spi device connected on the bus.
\ie

\is
\i <chanCfgHook>
Specifies the channel configuration hook routine. If special configuration
for a channel must be set, user should define a routine.
\ie

\is
\i <edmaTEvt>
Specifies the EDMA event number for SPI transmit. If <dmaMode> is FALSE, the
<edmaTEvt> can be ignored.
\ie

\is
\i <edmaREvt>
Specifies the EDMA event number for SPI receive. If <dmaMode> is FALSE, the
<edmaREvt> can be ignored.
\ie

An example hwconf entry is shown below:

\cs
LOCAL struct vxCanDevInfo spiDevTbl[] = {
    { "spiFlash_w25q64",       0, 8, 24000000,  CAN_MODE_0},
};

LOCAL UINT32 txEvt = {16, 18};
LOCAL UINT32 rxEvt = {17, 19};

LOCAL struct hcfResource vxCanResources[] =  {
    { "regBase",       HCF_RES_INT,   { (void *)(AM335X_SPI0_BASE) } },
    { "clkFreq",       HCF_RES_INT,   { (void *)(48000000)  } },
    { "polling",       HCF_RES_INT,   { (void *)(FALSE) } },
    { "dmaMode",       HCF_RES_INT,   { (void *)(TRUE) } },
    { "spiDev",        HCF_RES_ADDR,  { (void *)&spiDevTbl} },
    { "spiDevNum",     HCF_RES_INT,   { (void *)NELEMENTS(spiDevTbl)}}
    { "edmaTEvt",      HCF_RES_ADDR,  { (void *)&txEvt} },
    { "edmaREvt",      HCF_RES_ADDR,  { (void *)&rxEvt} },
};
#   define vxCanNum  NELEMENTS(vxCanResources)
\ce

To add the driver to the vxWorks image, add the following component to the
kernel configuration:

\cs
vxprj component add DRV_TI_AM335X_SPI
\ce

or define the following macro in config.h. 

\cs
#define DRV_TI_AM335X_SPI
\ce

SEE ALSO: vxBus, spiBus
\tb "AM335x ARM Cortex-A8 Microprocessors (MPUs) Technical Reference Manual"
*/

/* includes */

#include <vxWorks.h>
#include <vsbConfig.h>
#include <intLib.h>

#include <stdlib.h>
#include <string.h>
#include <semLib.h>
#include <taskLib.h>
#include <stdio.h>
#include <vxbTimerLib.h>
#include <cacheLib.h>
#include <hwif/util/hwMemLib.h>
#include <hwif/util/vxbParamSys.h>
#include <hwif/vxbus/vxBus.h>
#include <hwif/vxbus/vxbPlbLib.h>
#include <../src/hwif/h/vxbus/vxbAccess.h>

/*#include <../src/hwif/h/spi/vxCan_fm.h>*/
#include "vxbCan_fm.h"

/* debug macro */
#undef  CAN_DBG_ON
#define  CAN_DBG_ON

#ifdef  CAN_DBG_ON

/* turning local symbols into global symbols */
#ifdef  LOCAL
#undef  LOCAL
#endif
#define LOCAL

#define CAN_DBG_OFF            0x00000000
#define CAN_DBG_ISR            0x00000001
#define CAN_DBG_RW             0x00000002
#define CAN_DBG_ERR            0x00000004
#define CAN_DBG_RTN            0x00000008
#define CAN_DBG_ALL            0xffffffff

IMPORT FUNCPTR _func_logMsg;

#define CAN_DBG(mask, string, X1, X2, X3, X4, X5, X6)       \
        if (_func_logMsg != NULL) \
           (* _func_logMsg)(string, (int)X1, (int)X2, (int)X3, (int)X4, \
                (int)X5, (int)X6)
                
#define CAN_DBG1(string, X1, X2, X3, X4, X5, X6)  printf(string, (int)X1, (int)X2, (int)X3, (int)X4, (int)X5, (int)X6)
	
#else
#define CAN_DBG(mask, string, X1, X2, X3, X4, X5, X6)
#endif  /* CAN_DBG_ON */

#define CAN_DBG2(string, X1, X2, X3, X4, X5, X6)  printf(string, (int)X1, (int)X2, (int)X3, (int)X4, (int)X5, (int)X6)



/* VxBus methods */


#if 1
/* locals */

LOCAL struct drvBusFuncs vxCanVxbFuncs = 
{
    vxCanInstInit,      /* devInstanceInit */
    vxCanInstInit2,     /* devInstanceInit2 */
    vxCanInstConnect    /* devConnect */
};

/*
LOCAL device_method_t vxCanDeviceMethods[] = 
{
    DEVMETHOD (vxCanControlGet, vxCanCtrlGet),
    DEVMETHOD (busDevShow,       vxCanShow),
    DEVMETHOD (vxbDrvUnlink,     vxCanInstUnlink),
    DEVMETHOD_END
};
*/

LOCAL struct vxbPlbRegister vxCanDevRegistration = 
{
    {
    NULL,                   /* pNext */
    VXB_DEVID_DEVICE,       /* devID */
    VXB_BUSID_PLB,          /* busID = PLB */
    VXB_VER_4_0_0,          /* vxbVersion */
    
    FM_CAN_NAME,            /* drvName : fmcan */
    
    &vxCanVxbFuncs,         /* pDrvBusFuncs */
    NULL /*vxCanDeviceMethods*/,     /* pMethods */
    NULL,                   /* devProbe  */
    NULL,                   /* pParamDefaults */
    },
};


/*****************************************************************************
*
* vxCanRegister - register with the VxBus subsystem
*
* This routine registers the SPI driver with VxBus Systems.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void vxCanRegister (void)
{
    /*vxbDevRegister ((struct vxbDevRegInfo * &vxCanDevRegistration);*/
}

void vxCanRegister2 (void)
{
    vxbDevRegister ((struct vxbDevRegInfo *) &vxCanDevRegistration);
}

/*****************************************************************************
*
* vxCanInstInit - initialize fmsh SPI controller
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

LOCAL void vxCanInstInit
    (
    VXB_DEVICE_ID pDev
    )
{
    FM_CAN_DRV_CTRL * pDrvCtrl;
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
    pDrvCtrl = (FM_CAN_DRV_CTRL *) malloc (sizeof (FM_CAN_DRV_CTRL));
    if (pDrvCtrl == NULL)
    {
        return;
    }
	
    bzero ((char *)pDrvCtrl, sizeof(FM_CAN_DRV_CTRL));

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
	printf("<Can>pDev->unitNumber:(%d); pDrvCtrl->regBase:0x%08X \n", \
		    pDev->unitNumber, pDrvCtrl->regBase);
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
                        (void *) &pDrvCtrl->sysClk) != OK)
    {        
        pDrvCtrl->sysClk = FPAR_CANPS_0_CAN_CLK_FREQ_HZ;  /* 41,666,666 Hz */
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

    pDrvCtrl->initPhase = 1;
	return;
}

/*******************************************************************************
*
* vxCanCtrlInit -  SPI controller initialization
*
* This routine performs the SPI controller initialization.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxCanCtrl_Init (VXB_DEVICE_ID pDev)
{
    FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL*)pDev->pDrvCtrl;
	
    UINT32 value;
    UINT32 us = 0;
	int ctrl_x = 0;

    /* check for valid parameter */
   /* VXB_ASSERT_NONNULL_V (pDev);*/

    pDrvCtrl->cfgBaseAddr = (UINT32*)pDrvCtrl->regBase;
	
	pDrvCtrl->ctrl_x = pDev->unitNumber;	
	pDrvCtrl->devId = pDrvCtrl->ctrl_x;
	ctrl_x = pDrvCtrl->ctrl_x;

	/**/
	/* enable can_clk of slcr*/
	/**/
	vxCanSlcr_Enable_CanClk();
	
	/**/
	/* setup can_MIO of slcr*/
	/**/
	vxCanSlcr_Set_CanMIO();
	
	sprintf((char*)(&pDrvCtrl->BUS_TIME_C.name[0]), "%s", "bus_timing_c");
	pDrvCtrl->BUS_TIME_C.tseg1_min = 1;
	pDrvCtrl->BUS_TIME_C.tseg1_max = 16;
	pDrvCtrl->BUS_TIME_C.tseg2_min = 1;
	pDrvCtrl->BUS_TIME_C.tseg2_max = 8;
	pDrvCtrl->BUS_TIME_C.sjw_max = 4;
	pDrvCtrl->BUS_TIME_C.brp_min = 1;
	pDrvCtrl->BUS_TIME_C.brp_max = 64;
	pDrvCtrl->BUS_TIME_C.brp_inc = 1;
	
	vxCanCtrl_Set_BaudRate(pDev, CAN_BUAD_1MHZ);

	vxCanCtrl_Set_StdACR(pDev, 0x55, 1, 0x66, 0x66, SINGLE_FILTER);

	vxCanCtrl_Set_AMR(pDev, 0xff, 0xff, 0xff, 0xff);
	
    pDrvCtrl->initDone = TRUE;
	return;
}

/*******************************************************************************
*
* vxCanInstInit2 - second level initialization routine of SPI controller
*
* This routine performs the second level initialization of the SPI controller.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxCanInstInit2
    (
    VXB_DEVICE_ID pDev
    )
{
    FM_CAN_DRV_CTRL * pDrvCtrl;

    /* check for valid parameter */
    /*VXB_ASSERT_NONNULL_V (pDev);*/

    pDrvCtrl = (FM_CAN_DRV_CTRL*)pDev->pDrvCtrl;

    /* 
    The semSync semaphore is used to synchronize the SPI transfer. 
	*/
    pDrvCtrl->semSync = semBCreate (SEM_Q_PRIORITY, SEM_FULL);
    if (pDrvCtrl->semSync == NULL)
    {
        CAN_DBG (CAN_DBG_ERR, "semBCreate failed for semSync\n",
                 0, 0, 0, 0, 0, 0);
        return;
    }

    /* 
    The muxSem semaphore is used to mutex accessing the controller. 
    */
    pDrvCtrl->muxSem = semMCreate (SEM_Q_PRIORITY);
    if (pDrvCtrl->muxSem == NULL)
    {
        CAN_DBG (CAN_DBG_ERR, "semMCreate failed for muxSem\n",
                 0, 0, 0, 0, 0, 0);

        (void) semDelete (pDrvCtrl->semSync);
        pDrvCtrl->semSync = NULL;

        return;
    }
	
    /* 
    SPI controller init 
    */
    vxCanCtrl_Init(pDev);

    pDrvCtrl->initPhase = 2;
}

/*******************************************************************************
*
* vxCanInstConnect - third level initialization
*
* This routine performs the third level initialization of the SPI controller
* driver.
*
* RETURNS: N/A
*
* ERRNO : N/A
*/

LOCAL void vxCanInstConnect
    (
    VXB_DEVICE_ID       pDev
    )
{
    FM_CAN_DRV_CTRL * pDrvCtrl;

    /* check for valid parameter */
    /*VXB_ASSERT_NONNULL_V (pDev);*/

    pDrvCtrl = (FM_CAN_DRV_CTRL *) pDev->pDrvCtrl;

    /* 
    connect and enable interrupt for non-poll mode 
	*/
    if (!pDrvCtrl->polling)
    {
        if (vxbIntConnect (pDev, 0, vxCanIsr, pDev) != OK)
        {
            CAN_DBG(CAN_DBG_ERR, "vxbIntConnect return ERROR \n",
                    1, 2, 3, 4, 5, 6);
        }
        (void) vxbIntEnable (pDev, 0, vxCanIsr, pDev);
    }

    pDrvCtrl->initPhase = 3;
}

/*****************************************************************************
*
* vxCanInstUnlink - VxBus unlink handler
*
* This function shuts down a SPI controller instance in response to an
* an unlink event from VxBus. This may occur if our VxBus instance has
* been terminated, or if the SPI driver has been unloaded.
*
* RETURNS: OK if device was successfully destroyed, otherwise ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxCanInstUnlink
    (
    VXB_DEVICE_ID pDev,
    void *        unused
    )
{
    /* check if the pDev pointer is valid */
    /*VXB_ASSERT (pDev != NULL, ERROR)*/

    FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL *) pDev->pDrvCtrl;

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
            (void) vxbIntDisable (pDev, 0, vxCanIsr, pDev);

            if (vxbIntDisconnect (pDev, 0, vxCanIsr, pDev) != OK)
            {
                CAN_DBG(CAN_DBG_ERR, "vxbIntDisconnect return ERROR \n",
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
        /*vxCanDmaChanFree (pDev);*/
    }

    pDrvCtrl->initPhase = 0;
    pDev->pDrvCtrl = NULL;

    return (OK);
}


/*******************************************************************************
*
* vxCanIsr - interrupt service routine
*
* This routine handles interrupts of SPI.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxCanIsr
    (
    VXB_DEVICE_ID pDev
    )
{
    FM_CAN_DRV_CTRL * pDrvCtrl;

    /* Check if the pDev pointer is valid */
    VXB_ASSERT (pDev != NULL, ERROR)

    pDrvCtrl = (FM_CAN_DRV_CTRL *)pDev->pDrvCtrl;

    /* disable all SPI interrupts */
/*
	CSR_WRITE_4 (pDev, MCCAN_IRQENABLE, 0);

    CAN_DBG (CAN_DBG_ISR, "vxCanIsr: intSts 0x%x\n", 
             CSR_READ_4 (pDev, MCCAN_IRQSTATUS), 2, 3, 4, 5, 6);
*/
    semGive (pDrvCtrl->semSync);

	return;
}


#endif

#if 1

/*UINT32 vxDo_div2(UINT32 n, UINT32 base) */
UINT32 vxDo_div2(UINT64 n, UINT32 base) 
{					
	UINT32 __base = (base);				
	UINT32 __rem;	
	
	__rem = ((uint64_t)(n)) % __base;			
	(n) = ((uint64_t)(n)) / __base;				
	
    return __rem;							
}

void vxCanCtrl_Wr_CfgReg32(VXB_DEVICE_ID pDev, UINT32 offset, UINT32 value)
{
	FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL*)pDev->pDrvCtrl;
	UINT32 tmp32 = pDrvCtrl->cfgBaseAddr;
	
	FMQL_WRITE_32((tmp32 + offset), value);
	return;
}

UINT32 vxCanCtrl_Rd_CfgReg32(VXB_DEVICE_ID pDev, UINT32 offset)
{
	FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL*)pDev->pDrvCtrl;	
	UINT32 tmp32 = pDrvCtrl->cfgBaseAddr;
	
	return FMQL_READ_32(tmp32 + offset);
}

int vxCanCtrl_Update_SamplePoint(VXB_DEVICE_ID pDev,   			   UINT32 sample_point_nominal, 
                                            UINT32 tseg, UINT32 *tseg1_ptr, UINT32 *tseg2_ptr, UINT32 *pSamplePointErr)
{
	FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL*)pDev->pDrvCtrl;	
	
	UINT32 sample_point_error, best_SamplePointErr = UINT_MAX;
	UINT32 sample_point, best_sample_point = 0;
	UINT32 tseg1, tseg2;
	
	int i;

	for (i=0; i<=1; i++) 
    {
		tseg2 = tseg + CAN_CALC_SYNC_SEG - (sample_point_nominal * (tseg + CAN_CALC_SYNC_SEG)) / 1000 - i;
		tseg2 = clamp(tseg2, pDrvCtrl->BUS_TIME_C.tseg2_min, pDrvCtrl->BUS_TIME_C.tseg2_max);
		tseg1 = tseg - tseg2;
		if (tseg1 > pDrvCtrl->BUS_TIME_C.tseg1_max) 
        {
			tseg1 = pDrvCtrl->BUS_TIME_C.tseg1_max;
			tseg2 = tseg - tseg1;
		}

		sample_point = 1000 * (tseg + CAN_CALC_SYNC_SEG - tseg2) / (tseg + CAN_CALC_SYNC_SEG);
		sample_point_error = abs(sample_point_nominal - sample_point);

		if ((sample_point <= sample_point_nominal) && (sample_point_error < best_SamplePointErr)) 
        {
			best_sample_point = sample_point;
			best_SamplePointErr = sample_point_error;
			*tseg1_ptr = tseg1;
			*tseg2_ptr = tseg2;
		}
	}

	if (pSamplePointErr)
		*pSamplePointErr = best_SamplePointErr;

	return best_sample_point;
}

int vxCanCtrl_Calc_BusTiming(VXB_DEVICE_ID pDev)
{
	FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL*)pDev->pDrvCtrl;	
	
    UINT32 bitrate;			                    /* current bitrate */
	UINT32 bitrate_error;		                /* difference between current and nominal value */
	UINT32 best_bitrate_error = UINT_MAX;
	
	UINT32 sample_point_error;	                /* difference between current and nominal value */
	UINT32 best_SamplePointErr = UINT_MAX;
	UINT32 sample_point_nominal;	            /* nominal sample point */
	
	UINT32 best_tseg = 0;		                /* current best value for tseg */
	UINT32 best_brp = 0;		                /* current best value for brp */
	
	UINT32 brp, tsegall, tseg, tseg1 = 0, tseg2 = 0;
	UINT64 v64;
    
    /*struct can_bittiming *t;*/
    /*const struct can_bittiming_const *tc;*/
        
   	/* 
   	Use CiA recommended sample points 
	*/
	if (pDrvCtrl->BUS_TIME.sample_point)
    {
		sample_point_nominal = pDrvCtrl->BUS_TIME.sample_point;
	} 
    else 
    {
		if (pDrvCtrl->BUS_TIME.bitrate > 800000)
			sample_point_nominal = 750;
		else if (pDrvCtrl->BUS_TIME.bitrate > 500000)
			sample_point_nominal = 800;
		else
			sample_point_nominal = 875;
	}

	/* tseg even = round down, odd = round up */
	for (tseg = (pDrvCtrl->BUS_TIME_C.tseg1_max + pDrvCtrl->BUS_TIME_C.tseg2_max) * 2 + 1;
	     tseg >= (pDrvCtrl->BUS_TIME_C.tseg1_min + pDrvCtrl->BUS_TIME_C.tseg2_min) * 2; tseg--) 
    {
		tsegall = CAN_CALC_SYNC_SEG + tseg / 2;

		/* Compute all possible tseg choices (tseg=tseg1+tseg2) */
		brp = pDrvCtrl->sysClk / (tsegall * pDrvCtrl->BUS_TIME.bitrate) + tseg % 2;

		/* choose brp step which is possible in system */
		brp = (brp / pDrvCtrl->BUS_TIME_C.brp_inc) * pDrvCtrl->BUS_TIME_C.brp_inc;
		if ((brp < pDrvCtrl->BUS_TIME_C.brp_min) || (brp > pDrvCtrl->BUS_TIME_C.brp_max))
			continue;

		bitrate = pDrvCtrl->sysClk / (brp * tsegall);
		bitrate_error = abs(pDrvCtrl->BUS_TIME.bitrate - bitrate);

		/* 
		tseg brp biterror 
		*/
		if (bitrate_error > best_bitrate_error)
			continue;

		/* 
		reset sample point error if we have a better bitrate 
		*/
		if (bitrate_error < best_bitrate_error)
			best_SamplePointErr = UINT_MAX;

		vxCanCtrl_Update_SamplePoint(pDev, sample_point_nominal, tseg / 2, &tseg1, &tseg2, &sample_point_error);
		if (sample_point_error > best_SamplePointErr)
			continue;

		best_SamplePointErr = sample_point_error;
		best_bitrate_error = bitrate_error;
		best_tseg = tseg / 2;
		best_brp = brp;

		if (bitrate_error == 0 && sample_point_error == 0)
			break;
	}
    
	/* real sample point */
	pDrvCtrl->BUS_TIME.sample_point = vxCanCtrl_Update_SamplePoint(pDev, sample_point_nominal, best_tseg,
					  &tseg1, &tseg2, NULL);

	v64 = (u64)best_brp * 1000 * 1000 * 1000;
	
	vxDo_div2(v64, pDrvCtrl->sysClk);
	
	pDrvCtrl->BUS_TIME.tq = (UINT32)v64;
	pDrvCtrl->BUS_TIME.prop_seg = tseg1 / 2;
	pDrvCtrl->BUS_TIME.phase_seg1 = tseg1 - pDrvCtrl->BUS_TIME.prop_seg;
	pDrvCtrl->BUS_TIME.phase_seg2 = tseg2;

	/* 
	check for sjw user settings 
	*/
	if (!pDrvCtrl->BUS_TIME.sjw || !pDrvCtrl->BUS_TIME_C.sjw_max)
    {
		pDrvCtrl->BUS_TIME.sjw = 1;
	}
    else
    {
		/* 
		pDrvCtrl->BUS_TIME.sjw is at least 1 -> sanitize upper bound to sjw_max 
		*/
		if (pDrvCtrl->BUS_TIME.sjw > pDrvCtrl->BUS_TIME_C.sjw_max)
			pDrvCtrl->BUS_TIME.sjw = pDrvCtrl->BUS_TIME_C.sjw_max;
		
		/* 
		pDrvCtrl->BUS_TIME.sjw must not be higher than tseg2 
		*/
		if (tseg2 < pDrvCtrl->BUS_TIME.sjw)
			pDrvCtrl->BUS_TIME.sjw = tseg2;
	}

	pDrvCtrl->BUS_TIME.brp = best_brp;

	/* 
	real bitrate 
	*/
	pDrvCtrl->BUS_TIME.bitrate = pDrvCtrl->sysClk / (pDrvCtrl->BUS_TIME.brp * (CAN_CALC_SYNC_SEG + tseg1 + tseg2));

	return OK;
    
}

int vxCanCtrl_Set_RunMode(VXB_DEVICE_ID pDev, UINT32 run_mode)
{
	FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL*)pDev->pDrvCtrl;
	UINT32 tmp32 = 0;

	tmp32 = vxCanCtrl_Rd_CfgReg32(pDev, CAN_REG_MODE);
	
	if (run_mode == MODE_RST)
	{
		tmp32 |= (UINT32)(MODE_RST);   /* bit0 -> 1 */
	}
	else
	{
		tmp32 &= (UINT32)(~MODE_RST);  /* bit0 -> 0 */
	}
	
	vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_MODE, tmp32);
	
    return OK;
}

int vxCanCtrl_Set_BusTiming(VXB_DEVICE_ID pDev)
{
	FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL*)pDev->pDrvCtrl;	
	
    UINT32 btr0 = 0, btr1 = 0;
    UINT32 tmp32 = 0;
    
   	btr0 = ((pDrvCtrl->BUS_TIME.brp - 1) & 0x3f) | (((pDrvCtrl->BUS_TIME.sjw - 1) & 0x3) << 6);
	btr1 = ((pDrvCtrl->BUS_TIME.prop_seg + pDrvCtrl->BUS_TIME.phase_seg1 - 1) & 0xf) |
		    (((pDrvCtrl->BUS_TIME.phase_seg2 - 1) & 0x7) << 4); 

	tmp32 = vxCanCtrl_Rd_CfgReg32(pDev, CAN_REG_BTR0);
	if (tmp32 != btr0)
	{
		vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_BTR0, btr0);
	}
	
	tmp32 = vxCanCtrl_Rd_CfgReg32(pDev, CAN_REG_BTR1);
	if (tmp32 != btr1)
	{
		vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_BTR1, btr1);
	}
	
    return OK;
}


int vxCanCtrl_Set_BaudRate(VXB_DEVICE_ID pDev, int baud)
{
	FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL*)pDev->pDrvCtrl;	
	
    pDrvCtrl->BUS_TIME.bitrate = baud;

    vxCanCtrl_Calc_BusTiming(pDev);

	vxCanCtrl_Set_RunMode(pDev, MODE_RST);
	
    vxCanCtrl_Set_BusTiming(pDev);
	
	vxCanCtrl_Set_RunMode(pDev, MODE_RUN);
    
    return OK;
}

/*****************************************************************************
*
* @description
* This function set standard frame Acceptance Code Registers
*
* @param    dev is a pointer to the instance of can device.
* @param    id:11 bits
* @param    ptr: 0/1 0 -- remote frame, 1 -- data frame
* @param    byte1: 
* @param    byte2:
* @param    mode: 
*                 single_filter
*                 dual_filter
*
* @return   0 if successful, otherwise 1.
*
* @note     Only read/write in reset mode.
*
******************************************************************************/
int vxCanCtrl_Set_StdACR(VXB_DEVICE_ID pDev, UINT32 id, UINT8 ptr, UINT8 byte1, UINT8 byte2, int filter_mode)
{
	FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL*)pDev->pDrvCtrl;	
	
    if (ptr > 0x1)
        return ERROR;

    if (filter_mode == SINGLE_FILTER)
    {
		vxCanCtrl_Set_RunMode(pDev, MODE_RST);	
		
		vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_ACR0_TXRXBUF, (id >> 3));		
		vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_ACR1_TXRXBUF, (((id & 0x07) << 5)|(ptr << 4)));		
		vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_ACR2_TXRXBUF, byte1);
		vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_ACR3_TXRXBUF, byte2);		
		
		vxCanCtrl_Set_RunMode(pDev, MODE_RUN);
    }
    else if (filter_mode == DUAL_FILTER) 
    {
		vxCanCtrl_Set_RunMode(pDev, MODE_RST);
		
		vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_ACR0_TXRXBUF, (id >> 3));
		vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_ACR1_TXRXBUF, (((id & 0x07) << 5)|(ptr << 4)|(byte1 >> 4)));
		vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_ACR2_TXRXBUF, (id >> 3));
		vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_ACR3_TXRXBUF, ((id & 0x07) << 5)|(ptr << 4)|(byte1 & 0x0f));
		
		vxCanCtrl_Set_RunMode(pDev, MODE_RUN);		
    }
    else
    {    
    	return ERROR;
    }

    return OK;   
}

/*****************************************************************************
*
* @description
* This function set Acceptance Mask Registers
*
* @param    dev is a pointer to the instance of can device.
* @param    AMR0~3 
*           0��s in AMR0 �C 3 identify the bits at the corresponding positions in ACR0 �C 3 
*           which must be matched in the message identifier, ��1��s identify the corresponding bits
*           as ��don��t care��.
*
* @return   0 if successful, otherwise 1.
*
* @note     Only read/write in reset mode.
*
******************************************************************************/
int vxCanCtrl_Set_AMR(VXB_DEVICE_ID pDev, UINT32 AMR0, UINT32 AMR1, UINT32 AMR2, UINT32 AMR3)
{
	FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL*)pDev->pDrvCtrl;	
	
	vxCanCtrl_Set_RunMode(pDev, MODE_RST);
	
	vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_AMR0_TXRXBUF, AMR0);
	vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_AMR1_TXRXBUF, AMR1);
	vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_AMR2_TXRXBUF, AMR2);
	vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_AMR3_TXRXBUF, AMR3);
	
	vxCanCtrl_Set_RunMode(pDev, MODE_RUN);		
    return OK;
}

/*****************************************************************************
*
* @description
* This function returns the status of Transmit Buffer Status.
*
* @param    dev is a pointer to the instance of can device.
*
* @return   status: 0/1
*
* @note     None.
*
******************************************************************************/
UINT32 vxCanCtrl_Get_TxBufStatus(VXB_DEVICE_ID pDev)
{
	FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL*)pDev->pDrvCtrl;	
	
    UINT32 tmp32 = 0;
	
	/*
	SR.2	TBS	发送缓存区状态	
			1	发送缓存器释放，CPU可以向发送缓存区写消息。
			0	发送缓存区锁定，CPU无法访问发送缓存区，因为消息正在等待传输或正在传输。
	*/
	tmp32 = vxCanCtrl_Rd_CfgReg32(pDev, CAN_REG_SR) & 0x04;

    return tmp32;
}

/*****************************************************************************
*
* @description
* This function sends standard frame format message
*
* @param    dev is a pointer to the instance of can device.
* @param    id is 11 bits
* @param    sbuf is buffer of message data
* @param    len is length of message
* @param    state is a enum frame_type:data_frame or remote_frame
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
int vxCanCtrl_Setup_StdFrmTx(VXB_DEVICE_ID pDev, UINT32 id, UINT8* pBbuf, int len, int frm_type)
{
	FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL*)pDev->pDrvCtrl;	
	
    int timeout = 10000;
	int i = 0;

    while ((vxCanCtrl_Get_TxBufStatus(pDev) == 0) && (timeout--));  /* wait tx buffer empty*/

	vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_ACR0_TXRXBUF, ((frm_type << 6) | len));
	vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_ACR1_TXRXBUF, (((id << 5) & 0xFF00) >> 8));
	vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_ACR2_TXRXBUF, (((id << 5) | (frm_type << 4)) & 0x00FF));
	
    /* data frame*/
    if (frm_type == DATA_FRM) 
    {
		i = 0;
		vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_ACR3_TXRXBUF, pBbuf[i++]);
		
		vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_AMR0_TXRXBUF, pBbuf[i++]);
		vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_AMR1_TXRXBUF, pBbuf[i++]);
		vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_AMR2_TXRXBUF, pBbuf[i++]);
		vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_AMR3_TXRXBUF, pBbuf[i++]);
		
		vxCanCtrl_Wr_CfgReg32(pDev, (CAN_REG_TXRXBUF+0x04), pBbuf[i++]);
		vxCanCtrl_Wr_CfgReg32(pDev, (CAN_REG_TXRXBUF+0x08), pBbuf[i++]);
		vxCanCtrl_Wr_CfgReg32(pDev, (CAN_REG_TXRXBUF+0x0C), pBbuf[i++]);		
    }
    
    return OK;
}


/*****************************************************************************
*
* @description
* This function set Transmission Request
*
* @param    dev is a pointer to the instance of can device.
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
int vxCanCtrl_Set_TxRequest(VXB_DEVICE_ID pDev)
{
	FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL*)pDev->pDrvCtrl;	
	
    UINT32 tmp32 = 0;
    
	/*
	CMR.0	TR	传输请求	1：发送消息。
	*/
	tmp32 = vxCanCtrl_Rd_CfgReg32(pDev, CAN_REG_CMR) | 0x01;
	vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_CMR, tmp32); 

    return OK;
}

/*****************************************************************************
*
* @description
* This function returns the status of Transmission Complete Status.
*
* @param    dev is a pointer to the instance of can device.
*
* @return   status: 0/1
*
* @note     None.
*
******************************************************************************/
UINT32 vxCanCtrl_Get_TxOkStatus(VXB_DEVICE_ID pDev)
{
	FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL*)pDev->pDrvCtrl;	
	
    UINT32 tmp32 = 0;
	
	/*
	SR.3	TCS 传输完成状态	
			1	最后请求的传输已成功完成。
			0	最后请求的传输尚未完成。
	*/
	tmp32 = vxCanCtrl_Rd_CfgReg32(pDev, CAN_REG_SR) & 0x08;

    return tmp32;
}


/*****************************************************************************
*
* @description
* This function receives message, read the receive buffer data.
*
* @param    dev is a pointer to the instance of can device.
* @param    rbuf is buffer of message data to readback into
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
int vxCanCtrl_Get_RxFrmBuf(VXB_DEVICE_ID pDev, UINT8 * rxBuf)
{
	FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL*)pDev->pDrvCtrl;	
	
	int i = 0;
	
	rxBuf[i++] = (UINT8)vxCanCtrl_Rd_CfgReg32(pDev, CAN_REG_ACR0_TXRXBUF);
	rxBuf[i++] = (UINT8)vxCanCtrl_Rd_CfgReg32(pDev, CAN_REG_ACR1_TXRXBUF);
	rxBuf[i++] = (UINT8)vxCanCtrl_Rd_CfgReg32(pDev, CAN_REG_ACR2_TXRXBUF);
	rxBuf[i++] = (UINT8)vxCanCtrl_Rd_CfgReg32(pDev, CAN_REG_ACR3_TXRXBUF);
	
	rxBuf[i++] = (UINT8)vxCanCtrl_Rd_CfgReg32(pDev, CAN_REG_AMR0_TXRXBUF);
	rxBuf[i++] = (UINT8)vxCanCtrl_Rd_CfgReg32(pDev, CAN_REG_AMR1_TXRXBUF);
	rxBuf[i++] = (UINT8)vxCanCtrl_Rd_CfgReg32(pDev, CAN_REG_AMR2_TXRXBUF);
	rxBuf[i++] = (UINT8)vxCanCtrl_Rd_CfgReg32(pDev, CAN_REG_AMR3_TXRXBUF);
	
	rxBuf[i++] = (UINT8)vxCanCtrl_Rd_CfgReg32(pDev, (CAN_REG_TXRXBUF+0x00));
	rxBuf[i++] = (UINT8)vxCanCtrl_Rd_CfgReg32(pDev, (CAN_REG_TXRXBUF+0x04));
	rxBuf[i++] = (UINT8)vxCanCtrl_Rd_CfgReg32(pDev, (CAN_REG_TXRXBUF+0x08));
	rxBuf[i++] = (UINT8)vxCanCtrl_Rd_CfgReg32(pDev, (CAN_REG_TXRXBUF+0x0C));
	rxBuf[i++] = (UINT8)vxCanCtrl_Rd_CfgReg32(pDev, (CAN_REG_TXRXBUF+0x10));

    return i;
}

/*****************************************************************************
*
* @description
* This function set Release Receive Buffer
*
* @param    dev is a pointer to the instance of can device.
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
int vxCanCtrl_ReleaseRxBuf(VXB_DEVICE_ID pDev)
{
	FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL*)pDev->pDrvCtrl;	
	
    UINT32 tmp32 = 0;
	
	/*
	CMR.2	RRB	释放接收缓存区	1：释放接收缓存区 。
	*/
	tmp32 = vxCanCtrl_Rd_CfgReg32(pDev, CAN_REG_CMR) | 0x04;
	vxCanCtrl_Wr_CfgReg32(pDev, CAN_REG_CMR, tmp32);
		
    return OK;
}

#endif


#if 1

void vxCanSlcr_Enable_CanClk(void)
{
	slcr_write(0x304, 0x3); /* CAN1/0_APB_CLKACT: 1-enable    */
	return;
}

void vxCanSlcr_Set_CanMIO(void)
{
	slcr_write(0x768, 0x00001621U);  /* MIO[26] g_CAN0 RX         */
	slcr_write(0x76C, 0x00001620U);  /* MIO[27] g_CAN0 TX    */
	    
	slcr_write(0x7C0, 0x00001620U);  /* MIO[48] g_CAN1 TX         */
	slcr_write(0x7C4, 0x00001621U);  /* MIO[49] g_CAN1 RX  */
	
	return;
}

/****************************************************************************/
/**
*
*  This function loop CAN0's outputs to CAN1's inputs,and CAN1's outputs to
*  CAN0's inputs
*
* @param
*  loop_en -- 0:connect CAN inputs according to MIO mapping;1:set the loop
*
* @return
*  Null
*
* @note
*  Null
*
****************************************************************************/
void vxCanSlcr_Set_CanLoop(int loop_en)
{
	UINT32 tmp32 = 0;	
	
	if (loop_en == 0)
	{
	    tmp32 = slcr_read(SLCR_MIO_LOOPBACK);
		tmp32 &= ~0x04;
		slcr_write(SLCR_MIO_LOOPBACK, tmp32);
	}
	else if (loop_en == 1)
	{
		tmp32 = slcr_read(SLCR_MIO_LOOPBACK);
		tmp32 |= 0x04;
		slcr_write(SLCR_MIO_LOOPBACK, tmp32);
    }

	return;
}

#endif


#if 1

int vxCanCtrl_SndFrm_Poll(VXB_DEVICE_ID pDev, UINT32 id, UINT8* pData, int len)
{
	FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL*)pDev->pDrvCtrl;
	
	int timeout = 0x10000;
	
	vxCanCtrl_Setup_StdFrmTx(pDev, id, pData, len, DATA_FRM);
	
	vxCanCtrl_Set_TxRequest(pDev);

	/*
	SR.3	TCS 传输完成状态	1	最后请求的传输已成功完成。
				0	最后请求的传输尚未完成。
	*/
	while ((vxCanCtrl_Get_TxOkStatus(pDev) == 0) && (timeout--));

	if (timeout <= 0)
	{
		return ERROR;
	}
	else
	{
		return OK;
	}
}

int vxCanCtrl_RcvFrm_Poll(VXB_DEVICE_ID pDev, UINT8 * pRxBuf)
{
	FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL*)pDev->pDrvCtrl;
	
	int ret = 0;
	
	ret = vxCanCtrl_Get_RxFrmBuf(pDev, pRxBuf);
	
	vxCanCtrl_ReleaseRxBuf(pDev);
	        
	return ret;
}

int vxCanCtrl_SLCR_Loop(VXB_DEVICE_ID pDev0, VXB_DEVICE_ID pDev1, UINT8 * tx_frm, UINT8 * rx_buf)
{
	FM_CAN_DRV_CTRL * pDrvCtrl0 = (FM_CAN_DRV_CTRL*)pDev0->pDrvCtrl;
	FM_CAN_DRV_CTRL * pDrvCtrl1 = (FM_CAN_DRV_CTRL*)pDev1->pDrvCtrl;
	
    int error = 0;

	int id = 0x55;	
	int len = 8;
    
    TRACE_OUT(DEBUG_OUT, "\n -vxb-can- SLCR_Loop Test \n");
	
    /* 
    enable SLCR loop 
	*/
    vxCanSlcr_Set_CanLoop(TRUE);
    

#if 1 
	vxCanCtrl_SndFrm_Poll(pDev0, id, tx_frm, len);	/* spi1-slave, spi0-master */
	printf("<slcr_loop>can_%d: snd \n", pDrvCtrl0->ctrl_x);
		
	sysMsDelay(10);	
	printf("<------>\n");	
	
	vxCanCtrl_RcvFrm_Poll(pDev1, rx_buf); 
	printf("<slcr_loop>can_%d: rcv \n\n", pDrvCtrl1->ctrl_x);
#endif
	
    /* 
    disable slcr loop 
	*/
    vxCanSlcr_Set_CanLoop(FALSE);

	return;
}



int vxCanCtrl_X_Loop(VXB_DEVICE_ID pDev0, VXB_DEVICE_ID pDev1, UINT32 loop_tx_data)
{

}

#endif

#if 0

/*******************************************************************************
*
* vxCanCtrlGet - get the SPI controller struct
*
* This routine returns the SPI controller struct pointer (VXB_CAN_BUS_CTRL *)
* to caller (SPI Lib) by vxCanControlGet method. Currently, this struct
* only contain the spiTransfer routine(eg: vxCanTransfer) for SPI Lib used,
* other parameters can be easily added in this struct.
*
* RETURNS: the pointer of SPI controller struct
*
* ERRNO: N/A
*/

LOCAL VXB_CAN_BUS_CTRL * vxCanCtrlGet
    (
    VXB_DEVICE_ID pDev
    )
{
    FM_CAN_DRV_CTRL * pDrvCtrl;

    /* check if the pDev pointer is valid */
    VXB_ASSERT (pDev != NULL, NULL)

    pDrvCtrl = (FM_CAN_DRV_CTRL *) pDev->pDrvCtrl;

    CAN_DBG(CAN_DBG_RTN, "vxCanCtrlGet(0x08%x) called\n",
            (_Vx_usr_arg_t)pDev, 2, 3, 4, 5, 6);

    return (&(pDrvCtrl->vxCanCtrl));
}

/*******************************************************************************
*
* vxCanChanCfg - configure the SPI channel
*
* This routine configures the SPI channel register (MCCAN_CH(i)CONF) which
* controls SPI master operation with its parameters.
*
* RETURNS: OK if device was successfully configured, otherwise ERROR
*
* ERRNO: N/A
*/

 LOCAL STATUS vxCanChanCfg
    (
    VXB_DEVICE_ID  pDev,     /* controller pDev */
    CAN_HARDWARE * pSpiDev   /* device info */
    )
{
    UINT32  regValue;
    UINT32  frqDiv;
    FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL *) pDev->pDrvCtrl;

    while (CAN_REG_READ_32(pDrvCtrl, SPIx_SR) & SR_BUSY);

    /* Diable SPI controller */
    CAN_REG_WRITE_32(pDrvCtrl, SPIx_SSIENR, 0);

    CAN_REG_WRITE_32(pDrvCtrl, SPIx_SER, spiDevCtlr->csNum);

    /* Configure SPI controller chip select */
    regValue = CAN_REG_READ_32(pDrvCtrl, SPIx_CTRLR0);
    regValue &= ~(TMOD_MASK | CAN_MODE_MASK | CAN_FRF_MASK | DFS_MASK);
    regValue |= spiDevCtlr->spiDev.ctrlr0;
    CAN_REG_WRITE_32(pDrvCtrl, SPIx_CTRLR0, regValue);

    /* Configure SPI clock frequency */
    frqDiv = pDrvCtrl->apbClkFreqGet() / spiDevCtlr->devFrq;
    if (frqDiv == 0)
        frqDiv = 1;
    CAN_REG_WRITE_32(pDrvCtrl, SPIx_BAUDR, frqDiv);
}

/*******************************************************************************
*
* vxCanWaitBitSet - wait a register's specific bit is set
*
* The routine waits a register's specific bit is set
*
* RETURNS: OK or ERROR if timeout
*
* ERRNO: N/A
*/

LOCAL STATUS vxCanWaitBitSet
    (
    VXB_DEVICE_ID  pDev,       /* controller pDev */
    UINT32         regOffset,  /* Register  */
    UINT32         regBit      /* which bit */
    )
    {
    STATUS sts = ERROR;
    UINT32 ns = 0;

    do
        {
        if ((CSR_READ_4 (pDev, regOffset) & regBit) != 0)
            {
            sts = OK;
            break;
            }
        vxbUsDelay (1);
        if  (ns ++ > 5 * CAN_TIMEOUT_US)
            {
            break;
            }
        }while (TRUE);

    return sts;
    }

/*******************************************************************************
*
* vxCanChanShutdown - shutdown one SPI channel
*
* The routine performs shutdown operation of a specific SPI channel
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

 LOCAL void vxCanChanShutdown
    (
    VXB_DEVICE_ID  pDev,       /* controller pDev */
    UINT32         chan        /* SPI channel */
    )
    {
    UINT32 spiConf;
    
    /* Disable SPI */

    CSR_CLRBIT_4 (pDev, MCCAN_CHCTRL(chan), ~MCCAN_CH0CTRL_EN);

    spiConf = CSR_READ_4 (pDev, MCCAN_CHCONF(chan));

    /* disable Tx/Rx FIFO */

    spiConf &= ~(MCCAN_CH0CONF_FFER | MCCAN_CH0CONF_FFEW);

    /* Transmit-Receive mode */

    spiConf &= ~MCCAN_CHOCONF_TRM_MASK;

    /* disable Tx/Rx DMA */

    spiConf &= ~(MCCAN_CH0CONF_DMAR | MCCAN_CH0CONF_DMAW);

    /* deactive cs */

    spiConf &= ~MCCAN_CH0CONF_FORCE;

    CSR_WRITE_4 (pDev, MCCAN_CHCONF(chan), spiConf);

    /* disable all SPI interrupts */

    CSR_WRITE_4 (pDev, MCCAN_IRQENABLE, 0);

    /* clear previous pending IRQ status */

    CSR_WRITE_4 (pDev, MCCAN_IRQSTATUS,
                 CSR_READ_4 (pDev, MCCAN_IRQENABLE));
    }


/*******************************************************************************
*
* vxCanPollTrans - SPI transfer routine in poll mode
*
* The routine performs the transmission in poll mode.
*
* RETURNS: OK
*
* ERRNO: N/A
*/

LOCAL STATUS vxCanPollTrans
    (
    VXB_DEVICE_ID  pDev,     /* controller pDev */
    CAN_HARDWARE * pSpiDev,  /* device info */
    CAN_TRANSFER * pPkg      /* transfer data info */
    )
    {
    FM_CAN_DRV_CTRL * pDrvCtrl;
    UINT32 channel;
    UINT32 spiConf;
    BOOL   isFullDuplex;
    UINT32 len;
    UINT32 intSts;
    UINT32 alignSize;
    UINT32 data;
    int    i;
    STATUS sts = OK;
    UINT32 skipRxCnt;

    /* Check if the pDev pointer is valid */

    VXB_ASSERT (pDev != NULL, ERROR)

    pDrvCtrl = (FM_CAN_DRV_CTRL *) pDev->pDrvCtrl;

    channel  = pSpiDev->devInfo->chipSelect;

    CSR_WRITE_4 (pDev, MCCAN_IRQENABLE, 0);

     if (pSpiDev->devInfo->bitWidth <= 8)       /* 4 to 8 bits */
        {
        alignSize = sizeof(char);
        }
    else if (pSpiDev->devInfo->bitWidth <= 16) /* 9 to 16 bits */
        {
        alignSize = sizeof(UINT16);
        }
    else    /* 17 to 32 bits */
        {
        alignSize = sizeof(UINT32);
        }

    spiConf = CSR_READ_4 (pDev, MCCAN_CHCONF(channel));

    spiConf |= (MCCAN_CH0CONF_FFEW | MCCAN_CH0CONF_FFER);
    spiConf &= ~(MCCAN_CH0CONF_DMAR | MCCAN_CH0CONF_DMAW);

    /* Transmit-Receive mode */

    spiConf &= ~(MCCAN_TRM_TX | MCCAN_TRM_RX);

    isFullDuplex = IS_CAN_FULL_DUPLEX(pSpiDev->devInfo->mode);
    skipRxCnt = isFullDuplex ? 0 : pPkg->txLen;
    len = pDrvCtrl->txLeft > pDrvCtrl->rxLeft ? pDrvCtrl->txLeft : 
                                                pDrvCtrl->rxLeft;

    /* set the transfer levels register */

    if (len > CAN_FIFO_LEN)
        {
        CSR_WRITE_4 (pDev, MCCAN_XFERLEVEL,
                     (CAN_FIFO_LEN - 1) << MCCAN_XFERLEVEL_AFL_SHIFT);
        }
    else
        {
        CSR_WRITE_4 (pDev, MCCAN_XFERLEVEL,
                    (len - 1) << MCCAN_XFERLEVEL_AFL_SHIFT);
        }

    spiConf |= MCCAN_CH0CONF_FORCE;        /* forece cs */

    CSR_WRITE_4 (pDev, MCCAN_CHCONF(channel), spiConf);

    /* Enable SPI */

    CSR_SETBIT_4 (pDev, MCCAN_CHCTRL(channel), MCCAN_CH0CTRL_EN);

    /* Clear the pending interrupts */

    CSR_WRITE_4 (pDev, MCCAN_IRQSTATUS, CSR_READ_4 (pDev, MCCAN_IRQSTATUS));

    for (i = 0; i < CAN_FIFO_LEN && i < len; i += alignSize)
        {
        if (pDrvCtrl->txLeft > 0)
            {
            if (alignSize == sizeof(char))
                {
                data = pDrvCtrl->txBuf[pDrvCtrl->txLen - pDrvCtrl->txLeft];
                }
            else if (alignSize == sizeof(UINT16))
                {
                data = ((UINT16*)pDrvCtrl->txBuf)[pDrvCtrl->txLen -
                                                  pDrvCtrl->txLeft];
                }
            else
                {
                data = ((UINT32*)pDrvCtrl->txBuf)[pDrvCtrl->txLen -
                                                  pDrvCtrl->txLeft];
                }
    
            CSR_WRITE_4 (pDev, MCCAN_TX(channel), data);
    
            CAN_DBG (CAN_DBG_RW, "Tx: 0x%x\n",
                            data, 2, 3, 4, 5, 6);

            pDrvCtrl->txLeft -= alignSize;
            }
        else
            {
            CSR_WRITE_4 (pDev, MCCAN_TX(channel), 0);  /* write dummy */
            }
        }

    do
    {
  
    /* Read SPI Interrupt Status Register */

    intSts = CSR_READ_4 (pDev, MCCAN_IRQSTATUS);

    CSR_WRITE_4 (pDev, MCCAN_IRQSTATUS, intSts);

    /* Handle Rx */

    if (IS_RX_INT (channel, intSts))
        {

        /* Rx */

        for (i = 0; i < CAN_FIFO_LEN && i < len; i += alignSize)
            {
            if (skipRxCnt > 0)
                {
                CSR_READ_4 (pDev, MCCAN_RX(channel));
                skipRxCnt -= alignSize;
                }
            else if (pDrvCtrl->rxLeft > 0)
                {
                data =  CSR_READ_4 (pDev, MCCAN_RX(channel));
                if (alignSize == sizeof(char))
                    {
                    pDrvCtrl->rxBuf[pDrvCtrl->rxLen - pDrvCtrl->rxLeft]
                              = (UINT8)data;
                    }
                else if (alignSize == sizeof(UINT16))
                    {
                    ((UINT16*)pDrvCtrl->rxBuf)[pDrvCtrl->rxLen - pDrvCtrl->rxLeft]
                              = (UINT16)data;
                    }
                else
                    {
                    ((UINT32*)pDrvCtrl->rxBuf)[pDrvCtrl->rxLen - pDrvCtrl->rxLeft]
                              = data;
                    }
    
                CAN_DBG (CAN_DBG_RW, "Rx: 0x%x\n", data, 2, 3, 4, 5, 6);
                pDrvCtrl->rxLeft -= alignSize;
                }
            else
                {
                CSR_READ_4 (pDev, MCCAN_RX(channel));  /* read dummy */
                }
            }

        len = pDrvCtrl->txLeft > pDrvCtrl->rxLeft ? 
                                            pDrvCtrl->txLeft : pDrvCtrl->rxLeft;

        if (len > CAN_FIFO_LEN)
            {
            CSR_WRITE_4 (pDev, MCCAN_XFERLEVEL,
                         (CAN_FIFO_LEN - 1) << MCCAN_XFERLEVEL_AFL_SHIFT);
            }
        else
            {
            CSR_WRITE_4 (pDev, MCCAN_XFERLEVEL,
                         (len - 1) << MCCAN_XFERLEVEL_AFL_SHIFT);
            }

        /* Tx */

        for (i = 0; i < CAN_FIFO_LEN && i < len; i += alignSize)
            {
            if (pDrvCtrl->txLeft > 0)
                {
                if (alignSize == sizeof(char))
                    {
                    data = pDrvCtrl->txBuf[pDrvCtrl->txLen - pDrvCtrl->txLeft];
                    }
                else if (alignSize == sizeof(UINT16))
                    {
                    data = ((UINT16*)pDrvCtrl->txBuf)[pDrvCtrl->txLen -
                                                      pDrvCtrl->txLeft];
                    }
                else
                    {
                    data = ((UINT32*)pDrvCtrl->txBuf)[pDrvCtrl->txLen -
                                                      pDrvCtrl->txLeft];
                    }
        
                CSR_WRITE_4 (pDev, MCCAN_TX(channel), data);
        
                CAN_DBG (CAN_DBG_RW, "Tx: 0x%x\n",
                                data, 2, 3, 4, 5, 6);
    
                pDrvCtrl->txLeft -= alignSize;;
                }
            else
                {
                CSR_WRITE_4 (pDev, MCCAN_TX(channel), 0);  /* write dummy */
                }
            }

        if (skipRxCnt ==0 && (pDrvCtrl->txLeft == 0) && (pDrvCtrl->rxLeft == 0))
            {

            sts = vxCanWaitBitSet (pDev, MCCAN_CHSTAT(channel), 
                                       MCCAN_CH0STAT_TXFFE);
            break;
            }
        }

    }while (TRUE);

    vxCanChanShutdown (pDev, channel);

    return sts;
    }


/*******************************************************************************
*
* vxCanIntTrans - start SPI transfer in interrupt mode
*
* The routine starts a transmission in interrupt mode
*
* RETURNS: OK
*
* ERRNO: N/A
*/

LOCAL STATUS vxCanIntTrans
    (
    VXB_DEVICE_ID  pDev,     /* controller pDev */
    CAN_HARDWARE * pSpiDev,  /* device info */
    CAN_TRANSFER * pPkg      /* transfer data info */
    )
    {
    FM_CAN_DRV_CTRL * pDrvCtrl;
    UINT32 channel;
    UINT32 spiConf;
    BOOL   isFullDuplex;
    UINT32 len;
    UINT32 intSts;
    UINT32 alignSize;
    UINT32 data;
    int    i;
    UINT32 timeoutTick;
    STATUS sts = OK;
    UINT32 skipRxCnt;

    /* Check if the pDev pointer is valid */

    VXB_ASSERT (pDev != NULL, ERROR)

    pDrvCtrl = (FM_CAN_DRV_CTRL *) pDev->pDrvCtrl;

    channel  = pSpiDev->devInfo->chipSelect;

    CSR_WRITE_4 (pDev, MCCAN_IRQENABLE, 0);

     if (pSpiDev->devInfo->bitWidth <= 8)       /* 4 to 8 bits */
        {
        alignSize = sizeof(char);
        }
    else if (pSpiDev->devInfo->bitWidth <= 16) /* 9 to 16 bits */
        {
        alignSize = sizeof(UINT16);
        }
    else    /* 17 to 32 bits */
        {
        alignSize = sizeof(UINT32);
        }

    spiConf = CSR_READ_4 (pDev, MCCAN_CHCONF(channel));

    spiConf |= (MCCAN_CH0CONF_FFEW | MCCAN_CH0CONF_FFER);
    spiConf &= ~(MCCAN_CH0CONF_DMAR | MCCAN_CH0CONF_DMAW);

    /* Transmit-Receive mode */

    spiConf &= ~(MCCAN_TRM_TX | MCCAN_TRM_RX);

    isFullDuplex = IS_CAN_FULL_DUPLEX(pSpiDev->devInfo->mode);
    skipRxCnt = isFullDuplex ? 0 : pPkg->txLen;
    len = pDrvCtrl->txLeft > pDrvCtrl->rxLeft ? pDrvCtrl->txLeft : 
                                                               pDrvCtrl->rxLeft;
    if (len > CAN_FIFO_LEN)
        {
        CSR_WRITE_4 (pDev, MCCAN_XFERLEVEL,
                     (CAN_FIFO_LEN - 1) << MCCAN_XFERLEVEL_AFL_SHIFT);
        }
    else
        {
        CSR_WRITE_4 (pDev, MCCAN_XFERLEVEL,
                     (len - 1) << MCCAN_XFERLEVEL_AFL_SHIFT);
        }

    spiConf |= MCCAN_CH0CONF_FORCE;        /* forece cs */

    CSR_WRITE_4 (pDev, MCCAN_CHCONF(channel), spiConf);

    /* Enable SPI */

    CSR_SETBIT_4 (pDev, MCCAN_CHCTRL(channel), MCCAN_CH0CTRL_EN);

    /* Clear the pending interrupts */

    CSR_WRITE_4 (pDev, MCCAN_IRQSTATUS, CSR_READ_4 (pDev, MCCAN_IRQSTATUS));

    /* enable Rx interrupt only. Tx in Rx interrupt */

    CSR_SETBIT_4 (pDev, MCCAN_IRQENABLE, MCCAN_IRQENABLE_RX(channel));

    for (i = 0; i < CAN_FIFO_LEN && i < len; i += alignSize)
        {
        if (pDrvCtrl->txLeft > 0)
            {
            if (alignSize == sizeof(char))
                {
                data = pDrvCtrl->txBuf[pDrvCtrl->txLen - pDrvCtrl->txLeft];
                }
            else if (alignSize == sizeof(UINT16))
                {
                data = ((UINT16*)pDrvCtrl->txBuf)[pDrvCtrl->txLen -
                                                  pDrvCtrl->txLeft];
                }
            else
                {
                data = ((UINT32*)pDrvCtrl->txBuf)[pDrvCtrl->txLen -
                                                  pDrvCtrl->txLeft];
                }
    
            CSR_WRITE_4 (pDev, MCCAN_TX(channel), data);
    
            CAN_DBG (CAN_DBG_RW, "Tx: 0x%x\n", data, 2, 3, 4, 5, 6);

            pDrvCtrl->txLeft -= alignSize;
            }
        else
            {
            CSR_WRITE_4 (pDev, MCCAN_TX(channel), 0);  /* write dummy */
            }
        }
    do
    {
    timeoutTick = (vxbSysClkRateGet () * 8 * CAN_FIFO_LEN) / 
                  pDrvCtrl->curWorkingFrq;
    timeoutTick = 5 * timeoutTick + 10;

    (void)semTake (pDrvCtrl->semSync, timeoutTick);

    /* Read SPI Interrupt Status Register */

    intSts = CSR_READ_4 (pDev, MCCAN_IRQSTATUS);

    CAN_DBG (CAN_DBG_RW, "vxCanIsr: intSts 0x%x\n", intSts, 2, 3, 4, 5, 6);

    if (intSts == 0)  /* fake interrupt or timeout */
        {
        CAN_DBG (CAN_DBG_ERR, "vxCanIsr: fake interrupt or timeout\n", 
                 1, 2, 3, 4, 5, 6);

        sts = ERROR;
        goto intDone;
        }

    CSR_WRITE_4 (pDev, MCCAN_IRQSTATUS, intSts);

    /* Handle Rx */

    if (IS_RX_INT (channel, intSts))
        {

        /* Rx */

        for (i = 0; i < CAN_FIFO_LEN && i < len; i += alignSize)
            {
            if (skipRxCnt > 0)
                {
                CSR_READ_4 (pDev, MCCAN_RX(channel));
                skipRxCnt -= alignSize;
                }
            else if (pDrvCtrl->rxLeft > 0)
                {
                data =  CSR_READ_4 (pDev, MCCAN_RX(channel));
                if (alignSize == sizeof(char))
                    {
                    pDrvCtrl->rxBuf[pDrvCtrl->rxLen - pDrvCtrl->rxLeft]
                              = (UINT8)data;
                    }
                else if (alignSize == sizeof(UINT16))
                    {
                    ((UINT16*)pDrvCtrl->rxBuf)[pDrvCtrl->rxLen - pDrvCtrl->rxLeft]
                              = (UINT16)data;
                    }
                else
                    {
                    ((UINT32*)pDrvCtrl->rxBuf)[pDrvCtrl->rxLen - pDrvCtrl->rxLeft]
                              = data;
                    }
    
                CAN_DBG (CAN_DBG_RW, "Rx: 0x%x\n", data, 2, 3, 4, 5, 6);
                pDrvCtrl->rxLeft -= alignSize;
                }
            else
                {
                CSR_READ_4 (pDev, MCCAN_RX(channel));  /* read dummy */
                }
            }

        len = pDrvCtrl->txLeft > pDrvCtrl->rxLeft ? pDrvCtrl->txLeft : 
                                                               pDrvCtrl->rxLeft;

        if (len > CAN_FIFO_LEN)
            {
            CSR_WRITE_4 (pDev, MCCAN_XFERLEVEL,
                         (CAN_FIFO_LEN - 1) << MCCAN_XFERLEVEL_AFL_SHIFT);
            }
        else
            {
            CSR_WRITE_4 (pDev, MCCAN_XFERLEVEL,
                         (len - 1) << MCCAN_XFERLEVEL_AFL_SHIFT);
            }

        /* Tx */

        for (i = 0; i < CAN_FIFO_LEN && i < len; i += alignSize)
            {
            if (pDrvCtrl->txLeft > 0)
                {
                if (alignSize == sizeof(char))
                    {
                    data = pDrvCtrl->txBuf[pDrvCtrl->txLen - pDrvCtrl->txLeft];
                    }
                else if (alignSize == sizeof(UINT16))
                    {
                    data = ((UINT16*)pDrvCtrl->txBuf)[pDrvCtrl->txLen -
                                                      pDrvCtrl->txLeft];
                    }
                else
                    {
                    data = ((UINT32*)pDrvCtrl->txBuf)[pDrvCtrl->txLen -
                                                      pDrvCtrl->txLeft];
                    }
        
                CSR_WRITE_4 (pDev, MCCAN_TX(channel), data);
        
                CAN_DBG (CAN_DBG_RW, "Tx: 0x%x\n",
                                data, 2, 3, 4, 5, 6);
    
                pDrvCtrl->txLeft -= alignSize;;
                }
            else
                {
                CSR_WRITE_4 (pDev, MCCAN_TX(channel), 0);  /* write dummy */
                }
            }

        if ((skipRxCnt == 0) && (pDrvCtrl->txLeft == 0) && 
            (pDrvCtrl->rxLeft == 0))
            {
            sts = vxCanWaitBitSet (pDev, MCCAN_CHSTAT(channel), 
                                       MCCAN_CH0STAT_TXFFE);
            break;
            }

        /* enable Rx interrupt only. Tx in Rx interrupt */

        CSR_SETBIT_4 (pDev, MCCAN_IRQENABLE, MCCAN_IRQENABLE_RX(channel));
        }

    }while (TRUE);

intDone:

    vxCanChanShutdown (pDev, channel);

    return sts;
    }

/*******************************************************************************
*
* vxCanDmaParaPrep - prepare DMA parameter
*
* The routine prepares DMA parameter
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxCanDmaParaPrep
    (
    TI_EDMA3_PARAM  * pDmaPara,  /* EDMA parameter */
    UINT8 *           pSrcBuf,   /* src buffer */
    UINT8 *           pDstBuf,   /* dst buffer */
    UINT32            len,       /* data len */
    UINT32            alignSize, /* align size */
    BOOL              isSrcFix,  /* the src is fixed or not */
    BOOL              isDstFix   /* the dst is fixed or not */
    )
    {
    pDmaPara->acnt = alignSize;
    len = len / alignSize;
    pDmaPara->bcnt = len & (DMA_BCNT_MAX -1);
    pDmaPara->ccnt = (len + DMA_BCNT_MAX - 1) >> DMA_BCNT_MAX_SHIFT;
    if (pDmaPara->bcnt == 0)
        {
        pDmaPara->bcnt = DMA_BCNT_MAX;
        }
    pDmaPara->dst = (UINT32)pDstBuf;
    pDmaPara->src = (UINT32)pSrcBuf;
    pDmaPara->bcntrld = DMA_BCNT_MAX;
    if (isSrcFix && isDstFix)
        {
        pDmaPara->srcbidx = 0;
        pDmaPara->srccidx = 0;
        pDmaPara->dstbidx = 0;
        pDmaPara->dstcidx = 0;
        }
    else if (isDstFix)
        {
        pDmaPara->srcbidx = pDmaPara->acnt;
        pDmaPara->srccidx = pDmaPara->acnt;
        pDmaPara->dstbidx = 0;
        pDmaPara->dstcidx = 0;
        }
    else if (isSrcFix)
        {
        pDmaPara->srcbidx = 0;
        pDmaPara->srccidx = 0;
        pDmaPara->dstbidx = pDmaPara->acnt;
        pDmaPara->dstcidx = pDmaPara->acnt;
        }
    else
        {
        CAN_DBG (CAN_DBG_ERR, "vxCanDmaParaPrep: invalid parameter \n", 
                 1, 2, 3, 4, 5, 6);
        }
    }

/*******************************************************************************
*
* vxCanDmaTrans - start SPI transfer in DMA mode
*
* The routine starts a transmission in DMA mode
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxCanDmaTrans
    (
    VXB_DEVICE_ID  pDev,     /* controller pDev */
    CAN_HARDWARE * pSpiDev,  /* device info */
    CAN_TRANSFER * pPkg       /* transfer data info */
    )
    {
    FM_CAN_DRV_CTRL * pDrvCtrl;
    UINT32 channel;
    UINT32 spiConf;
    UINT8 * pData;
    UINT32 len;
    UINT32 alignSize;
    UINT32 timeoutTick;
    STATUS sts = OK;
    UINT32 maxTranLen;
    UINT32 dummyData;
    
    /* Check if the pDev pointer is valid */

    VXB_ASSERT (pDev != NULL, ERROR)

    pDrvCtrl = (FM_CAN_DRV_CTRL *) pDev->pDrvCtrl;
    channel  = pSpiDev->devInfo->chipSelect;

    if (pSpiDev->devInfo->bitWidth <= 8)       /* 4 to 8 bits */
        {
        alignSize = sizeof(char);
        maxTranLen = DMA_MAX_LEN;
        }
    else if (pSpiDev->devInfo->bitWidth <= 16) /* 9 to 16 bits */
        {
        alignSize = sizeof(UINT16);
        maxTranLen = DMA_MAX_LEN * alignSize;
        }
    else    /* 17 to 32 bits */
        {
        alignSize = sizeof(UINT32);
        maxTranLen = (UINT32)(-1) & ~0x03;
        }

    spiConf = CSR_READ_4 (pDev, MCCAN_CHCONF(channel));
    spiConf &= ~(MCCAN_TRM_TX | MCCAN_TRM_RX);
    spiConf &= ~(MCCAN_CH0CONF_DMAR | MCCAN_CH0CONF_DMAW);
    spiConf &= ~(MCCAN_CH0CONF_FFEW | MCCAN_CH0CONF_FFER);
    spiConf |= MCCAN_CH0CONF_FORCE;        /* force cs */
    CSR_WRITE_4 (pDev, MCCAN_CHCONF(channel), spiConf);

    /* Enable SPI */

    CSR_SETBIT_4 (pDev, MCCAN_CHCTRL(channel), MCCAN_CH0CTRL_EN);

    /* Clear the pending interrupts */

    CSR_WRITE_4 (pDev, MCCAN_IRQSTATUS, CSR_READ_4 (pDev, MCCAN_IRQSTATUS));

    if (pDrvCtrl->txLen > 0)
        {
        CACHE_USER_FLUSH (pDrvCtrl->txBuf, pDrvCtrl->txLen);
        }
    if (pDrvCtrl->rxLen > 0)
        {
        CACHE_USER_FLUSH (pDrvCtrl->rxBuf, pDrvCtrl->rxLen);
        }

    while(pDrvCtrl->txLeft > 0 || pDrvCtrl->rxLeft > 0)
        {

        /* Tx */

        if (pDrvCtrl->txLeft != 0)
            {
            len = pDrvCtrl->txLeft > maxTranLen ? maxTranLen : pDrvCtrl->txLeft;
            pData = pPkg->txBuf + (pDrvCtrl->txLen - pDrvCtrl->txLeft);

            vxCanDmaParaPrep (pDrvCtrl->pTxChanParam[channel], pData, 
                        (UINT8*)((UINT32)pDrvCtrl->regBase + MCCAN_TX(channel)),
                        len, alignSize, FALSE, TRUE);
            vxbDmaWrite (pDrvCtrl->dmaTxRes[channel],
                         0, 0, 0, 0, 0,
                         (pVXB_DMA_COMPLETE_FN)vxCanDmaCallback,
                         0);

            /* Rx dummy */

            pData = (UINT8*)&dummyData;

            vxCanDmaParaPrep (pDrvCtrl->pRxChanParam[channel],
                        (UINT8*)((UINT32)pDrvCtrl->regBase + MCCAN_RX(channel)),
                        pData, len, alignSize, TRUE, TRUE);
            vxbDmaRead (pDrvCtrl->dmaRxRes[channel],
                        0, 0, 0, 0, 0,
                        (pVXB_DMA_COMPLETE_FN)vxCanDmaCallback,
                        (void *)pDrvCtrl->semSync);

            timeoutTick = (vxbSysClkRateGet () * 8 * len) / 
                      pDrvCtrl->curWorkingFrq;
            timeoutTick = 5 * timeoutTick + 100;

            /* enable DMA request */

            CSR_SETBIT_4 (pDev, MCCAN_CHCONF(channel), 
                          (MCCAN_CH0CONF_DMAR | MCCAN_CH0CONF_DMAW));

            if (semTake (pDrvCtrl->semSync, timeoutTick) != OK)
                {
                sts = ERROR;
                goto dmaDone;
                }

            /* disable DMA request */

            CSR_CLRBIT_4 (pDev, MCCAN_CHCONF(channel), 
                          ~(MCCAN_CH0CONF_DMAR | MCCAN_CH0CONF_DMAW));
            
            pDrvCtrl->txLeft -= len;
           
            }
        else if (pDrvCtrl->rxLeft != 0)
            {
            /* Tx dummy */

            len = pDrvCtrl->rxLeft > maxTranLen ? maxTranLen : pDrvCtrl->rxLeft;
            pData = (UINT8*)&dummyData;
            vxCanDmaParaPrep (pDrvCtrl->pTxChanParam[channel], pData, 
                        (UINT8*)((UINT32)pDrvCtrl->regBase + MCCAN_TX(channel)),
                        len, alignSize, TRUE, TRUE);
            vxbDmaWrite (pDrvCtrl->dmaTxRes[channel],
                         0, 0, 0, 0, 0,
                         (pVXB_DMA_COMPLETE_FN)vxCanDmaCallback,
                         (void *)pDrvCtrl->semSync);

            /* Rx */

            pData = pPkg->rxBuf + (pDrvCtrl->rxLen - pDrvCtrl->rxLeft);;

            vxCanDmaParaPrep (pDrvCtrl->pRxChanParam[channel],
                        (UINT8*)((UINT32)pDrvCtrl->regBase + MCCAN_RX(channel)),
                        pData, len, alignSize, TRUE, FALSE);

            vxbDmaRead (pDrvCtrl->dmaRxRes[channel],
                        0, 0, 0, 0, 0,
                        (pVXB_DMA_COMPLETE_FN)vxCanDmaCallback,
                        (void *)pDrvCtrl->semSync);

            /* enable DMA request */

            CSR_SETBIT_4 (pDev, MCCAN_CHCONF(channel), 
                          (MCCAN_CH0CONF_DMAR | MCCAN_CH0CONF_DMAW));

            timeoutTick = (vxbSysClkRateGet () * 8 * len) / 
                          pDrvCtrl->curWorkingFrq;
            timeoutTick = 5 * timeoutTick + 100;

            if (semTake (pDrvCtrl->semSync, timeoutTick) != OK)
                {
                sts = ERROR;
                goto dmaDone;
                }
            CACHE_USER_INVALIDATE (pData, len);

            /* disable DMA request */

            CSR_CLRBIT_4 (pDev, MCCAN_CHCONF(channel), 
                          ~(MCCAN_CH0CONF_DMAR | MCCAN_CH0CONF_DMAW));

            pDrvCtrl->rxLeft -= len;
            }
        }

dmaDone:

    vxCanChanShutdown (pDev, channel);

    CAN_DBG (CAN_DBG_RTN, "vxCanDmaTrans: done Tx/Rx \n", 1, 2, 3, 4, 5, 6);

    return sts;
    }

/*******************************************************************************
*
* vxCanDmaTransFullDuplex - start SPI transfer in DMA mode for full duplex
*
* The routine starts a transmission in DMA mode for full duplex device driver
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxCanDmaTransFullDuplex
    (
    VXB_DEVICE_ID  pDev,     /* controller pDev */
    CAN_HARDWARE * pSpiDev,  /* device info */
    CAN_TRANSFER * pPkg       /* transfer data info */
    )
    {
    FM_CAN_DRV_CTRL * pDrvCtrl;
    UINT32 channel;
    UINT32 spiConf;
    UINT8 * pData;
    UINT32 len;
    UINT32 alignSize;
    UINT32 timeoutTick;
    STATUS sts = OK;
    UINT32 maxTranLen;
    UINT32 dummyData;
    UINT32 minLen;

    /* Check if the pDev pointer is valid */

    VXB_ASSERT (pDev != NULL, ERROR)

    pDrvCtrl = (FM_CAN_DRV_CTRL *) pDev->pDrvCtrl;
    if (pDrvCtrl->txLen == 0 || pDrvCtrl->rxLen == 0)
        {
        return vxCanDmaTrans (pDev, pSpiDev, pPkg);
        }

    channel  = pSpiDev->devInfo->chipSelect;

    if (pSpiDev->devInfo->bitWidth <= 8)       /* 4 to 8 bits */
        {
        alignSize = sizeof(char);
        maxTranLen = DMA_MAX_LEN;
        }
    else if (pSpiDev->devInfo->bitWidth <= 16) /* 9 to 16 bits */
        {
        alignSize = sizeof(UINT16);
        maxTranLen = DMA_MAX_LEN * alignSize;
        }
    else    /* 17 to 32 bits */
        {
        alignSize = sizeof(UINT32);
        maxTranLen = (UINT32)(-1) & ~0x03;
        }

    spiConf = CSR_READ_4 (pDev, MCCAN_CHCONF(channel));
    spiConf &= ~(MCCAN_TRM_TX | MCCAN_TRM_RX);
    spiConf &= ~(MCCAN_CH0CONF_DMAR | MCCAN_CH0CONF_DMAW);
    spiConf &= ~(MCCAN_CH0CONF_FFEW | MCCAN_CH0CONF_FFER);
    spiConf |= MCCAN_CH0CONF_FORCE;        /* force cs */
    CSR_WRITE_4 (pDev, MCCAN_CHCONF(channel), spiConf);

    /* Enable SPI */

    CSR_SETBIT_4 (pDev, MCCAN_CHCTRL(channel), MCCAN_CH0CTRL_EN);

    /* Clear the pending interrupts */

    CSR_WRITE_4 (pDev, MCCAN_IRQSTATUS, CSR_READ_4 (pDev, MCCAN_IRQSTATUS));

    CACHE_USER_FLUSH (pDrvCtrl->txBuf, pDrvCtrl->txLen);
    CACHE_USER_FLUSH (pDrvCtrl->rxBuf, pDrvCtrl->rxLen);
    minLen = pDrvCtrl->txLeft > pDrvCtrl->rxLeft ? pDrvCtrl->rxLeft :
                                                               pDrvCtrl->txLeft;
    while(minLen > 0)
        {
        len = minLen > maxTranLen ? maxTranLen : minLen;

        /* Tx */

        pData = pPkg->txBuf + (pDrvCtrl->txLen - pDrvCtrl->txLeft);
        vxCanDmaParaPrep (pDrvCtrl->pTxChanParam[channel], pData, 
                        (UINT8*)((UINT32)pDrvCtrl->regBase + MCCAN_TX(channel)),
                        len, alignSize, FALSE, TRUE);
        vxbDmaWrite (pDrvCtrl->dmaTxRes[channel],
                     0, 0, 0, 0, 0,
                     (pVXB_DMA_COMPLETE_FN)vxCanDmaCallback,
                     0);

        /* Rx */

        pData = pPkg->rxBuf + (pDrvCtrl->rxLen - pDrvCtrl->rxLeft);;
        vxCanDmaParaPrep (pDrvCtrl->pRxChanParam[channel],
                        (UINT8*)((UINT32)pDrvCtrl->regBase + MCCAN_RX(channel)),
                        pData, len, alignSize, TRUE, FALSE);

        vxbDmaRead (pDrvCtrl->dmaRxRes[channel],
                    0, 0, 0, 0, 0,
                    (pVXB_DMA_COMPLETE_FN)vxCanDmaCallback,
                    (void *)pDrvCtrl->semSync);
    
        /* enable DMA request */
    
        CSR_SETBIT_4 (pDev, MCCAN_CHCONF(channel), 
                      (MCCAN_CH0CONF_DMAR | MCCAN_CH0CONF_DMAW));
    
        timeoutTick = (vxbSysClkRateGet () * 8 * len) / 
                      pDrvCtrl->curWorkingFrq;
        timeoutTick = 5 * timeoutTick + 100;
    
        if (semTake (pDrvCtrl->semSync, timeoutTick) != OK)
            {
            sts = ERROR;
            goto dmafullDone;
            }
        CACHE_USER_INVALIDATE (pData, len);
    
        /* disable DMA request */
    
        CSR_CLRBIT_4 (pDev, MCCAN_CHCONF(channel), 
                      ~(MCCAN_CH0CONF_DMAR | MCCAN_CH0CONF_DMAW));
    
        pDrvCtrl->rxLeft -= len;
        
        pDrvCtrl->txLeft -= len;
        minLen -= len;
        }

    if (pDrvCtrl->txLeft ==0 && pDrvCtrl->rxLeft == 0)
        {
        goto dmafullDone;
        }

    while(pDrvCtrl->txLeft > 0 || pDrvCtrl->rxLeft > 0)
        {
        if (pDrvCtrl->txLeft != 0)
            {
            len = pDrvCtrl->txLeft > maxTranLen ? maxTranLen : pDrvCtrl->txLeft;
            pData = pPkg->txBuf + (pDrvCtrl->txLen - pDrvCtrl->txLeft);

            vxCanDmaParaPrep (pDrvCtrl->pTxChanParam[channel], pData, 
                        (UINT8*)((UINT32)pDrvCtrl->regBase + MCCAN_TX(channel)),
                        len, alignSize, FALSE, TRUE);
            vxbDmaWrite (pDrvCtrl->dmaTxRes[channel],
                         0, 0, 0, 0, 0,
                         (pVXB_DMA_COMPLETE_FN)vxCanDmaCallback,
                         0);

            /* Rx dummy */

            pData = (UINT8*)&dummyData;

            vxCanDmaParaPrep (pDrvCtrl->pRxChanParam[channel],
                        (UINT8*)((UINT32)pDrvCtrl->regBase + MCCAN_RX(channel)),
                        pData, len, alignSize, TRUE, TRUE);
            vxbDmaRead (pDrvCtrl->dmaRxRes[channel],
                        0, 0, 0, 0, 0,
                        (pVXB_DMA_COMPLETE_FN)vxCanDmaCallback,
                        (void *)pDrvCtrl->semSync);

            timeoutTick = (vxbSysClkRateGet () * 8 * len) / 
                      pDrvCtrl->curWorkingFrq;
            timeoutTick = 5 * timeoutTick + 100;

            /* enable DMA request */

            CSR_SETBIT_4 (pDev, MCCAN_CHCONF(channel), 
                          (MCCAN_CH0CONF_DMAR | MCCAN_CH0CONF_DMAW));

            if (semTake (pDrvCtrl->semSync, timeoutTick) != OK)
                {
                sts = ERROR;
                goto dmafullDone;
                }

            /* disable DMA request */

            CSR_CLRBIT_4 (pDev, MCCAN_CHCONF(channel), 
                          ~(MCCAN_CH0CONF_DMAR | MCCAN_CH0CONF_DMAW));
            
            pDrvCtrl->txLeft -= len;
           
            }
        else if (pDrvCtrl->rxLeft != 0)
            {
            /* Tx dummy */

            len = pDrvCtrl->rxLeft > maxTranLen ? maxTranLen : pDrvCtrl->rxLeft;
            pData = (UINT8*)&dummyData;
            vxCanDmaParaPrep (pDrvCtrl->pTxChanParam[channel], pData, 
                        (UINT8*)((UINT32)pDrvCtrl->regBase + MCCAN_TX(channel)),
                        len, alignSize, TRUE, TRUE);
            vxbDmaWrite (pDrvCtrl->dmaTxRes[channel],
                         0, 0, 0, 0, 0,
                         (pVXB_DMA_COMPLETE_FN)vxCanDmaCallback,
                         (void *)pDrvCtrl->semSync);

            /* Rx */

            pData = pPkg->rxBuf + (pDrvCtrl->rxLen - pDrvCtrl->rxLeft);;

            vxCanDmaParaPrep (pDrvCtrl->pRxChanParam[channel],
                        (UINT8*)((UINT32)pDrvCtrl->regBase + MCCAN_RX(channel)),
                        pData, len, alignSize, TRUE, FALSE);

            vxbDmaRead (pDrvCtrl->dmaRxRes[channel],
                        0, 0, 0, 0, 0,
                        (pVXB_DMA_COMPLETE_FN)vxCanDmaCallback,
                        (void *)pDrvCtrl->semSync);

            /* enable DMA request */

            CSR_SETBIT_4 (pDev, MCCAN_CHCONF(channel), 
                          (MCCAN_CH0CONF_DMAR | MCCAN_CH0CONF_DMAW));

            timeoutTick = (vxbSysClkRateGet () * 8 * len) / 
                      pDrvCtrl->curWorkingFrq;
            timeoutTick = 5 * timeoutTick + 100;

            if (semTake (pDrvCtrl->semSync, timeoutTick) != OK)
                {
                sts = ERROR;
                goto dmafullDone;
                }
            CACHE_USER_INVALIDATE (pData, len);

            /* disable DMA request */

            CSR_CLRBIT_4 (pDev, MCCAN_CHCONF(channel), 
                          ~(MCCAN_CH0CONF_DMAR | MCCAN_CH0CONF_DMAW));

            pDrvCtrl->rxLeft -= len;
            }
        }

dmafullDone:

    vxCanChanShutdown (pDev, channel);

    CAN_DBG (CAN_DBG_RTN, "vxCanDmaTrans: done Tx/Rx \n", 1, 2, 3, 4, 5, 6);

    return OK;
    }

/*******************************************************************************
*
* vxCanTransfer - SPI transfer routine
*
* This routine is used to perform one transmission. It is the interface which 
* can be called by SPI device driver to send and receive data via the SPI 
* controller.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxCanTransfer
    (
    VXB_DEVICE_ID  pDev,     /* controller pDev */
    CAN_HARDWARE * pSpiDev,  /* device info */
    CAN_TRANSFER * pPkg       /* transfer data info */
    )
    {
    STATUS sts = OK;
    FM_CAN_DRV_CTRL * pDrvCtrl;
    UINT32 channel;
    UINT32 alignSize;

    /* Check if the pointers are valid */

    if (pDev == NULL || pSpiDev == NULL || pPkg == NULL ||
	pSpiDev->devInfo == NULL)
        {
        CAN_DBG (CAN_DBG_ERR, "vxCanTransfer NULL pointer \n",
                 1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    pDrvCtrl = (FM_CAN_DRV_CTRL *) pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        {
        CAN_DBG (CAN_DBG_ERR, "vxCanTransfer pDrvCtrl is NULL \n",
                 1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    CAN_DBG (CAN_DBG_RTN, "vxCanTransfer  txLen[%d] rxLen[%d] \n",
             pPkg->txLen, pPkg->rxLen, 3, 4, 5, 6);

    if (pPkg->txLen == 0 && pPkg->rxLen == 0)
        {
        CAN_DBG (CAN_DBG_ERR, "vxCanTransfer tx/rx are 0 \n",
                 1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    if ((pPkg->txLen != 0 && pPkg->txBuf == NULL ) ||
        (pPkg->rxLen != 0 && pPkg->rxBuf == NULL))
        {
        CAN_DBG (CAN_DBG_ERR,
                 "vxCanTransfer invalid parameters[%x %x %x %x] \n",
                  pPkg->txBuf, pPkg->txLen, pPkg->rxLen, pPkg->rxBuf, 5, 6);

        return ERROR;
        }

    if (pSpiDev->devInfo->bitWidth <= 8)       /* 4 to 8 bits */
        {
        alignSize = sizeof(char);
        }
    else if (pSpiDev->devInfo->bitWidth <= 16) /* 9 to 16 bits */
        {
        alignSize = sizeof(UINT16);
        }
    else    /* 17 to 32 bits */
        {
        alignSize = sizeof(UINT32);
        }

    /* check to see if the address is aligned with SPI bit width */

    if ((pPkg->txLen != 0 && (((UINT32)pPkg->txBuf & (alignSize - 1)) != 0 
          || (pPkg->txLen & (alignSize - 1)) != 0) ) ||
        (pPkg->rxLen != 0 && (((UINT32)pPkg->rxBuf & (alignSize - 1)) != 0
          || (pPkg->rxLen & (alignSize - 1)) != 0)))
        {
        CAN_DBG (CAN_DBG_ERR,
                 "vxCanTransfer address or len is not aligned:"
                 "[tx:%x-%x rx:%x-%x] with %d \n",
                 pPkg->txBuf, pPkg->txLen, pPkg->rxBuf,pPkg->rxLen, 
                 alignSize, 6);

        return ERROR;
        }

    channel = pSpiDev->devInfo->chipSelect;
    if (channel >= CAN_MAX_CS_NUM)
        {
        CAN_DBG (CAN_DBG_ERR,
                 "vxCanTransfer invalid channel[%x] \n",
                  channel, 2, 3, 4, 5, 6);

        return ERROR;
        }

    /*
     * The controller can not be used by multichannel at the same time.
     * If the initPhase < 2, there is no multi-task context. So, the
     * semTake is not needed.
     */

    if (pDrvCtrl->initPhase >= 2)
        {
        (void)semTake (pDrvCtrl->muxSem, WAIT_FOREVER);
        }

    
    if (vxCanChanCfg(pDev, pSpiDev) != OK)
        {
        CAN_DBG (CAN_DBG_ERR, "vxCanChanCfg failed.\n",
                 1, 2, 3, 4, 5, 6);
    
        if (pDrvCtrl->initPhase >= 2)
            {
            semGive (pDrvCtrl->muxSem);
            }
    
        return ERROR;
        }

    pDrvCtrl->txBuf  = pPkg->txBuf;
    pDrvCtrl->txLen  = pPkg->txLen;
    pDrvCtrl->txLeft = pPkg->txLen;
    pDrvCtrl->rxBuf  = pPkg->rxBuf;
    pDrvCtrl->rxLen  = pPkg->rxLen;
    pDrvCtrl->rxLeft = pPkg->rxLen;

    /*
     *  The poll mode will be used unconditionally if the interrupt is not
     *  installed (before phase 3)
     */

    if (pDrvCtrl->polling || pDrvCtrl->initPhase < 3)
        {
        sts = vxCanPollTrans (pDev, pSpiDev, pPkg);
        }
    else
        {
        if (pDrvCtrl->dmaMode && vxCanDmaChanAlloc (pDev, channel) == OK)
            {
            if (IS_CAN_FULL_DUPLEX(pSpiDev->devInfo->mode))
                {
                sts = vxCanDmaTransFullDuplex(pDev, pSpiDev, pPkg);
                }
            else
                {
                sts = vxCanDmaTrans (pDev, pSpiDev, pPkg);
                }
            }
        else
            {
            sts = vxCanIntTrans (pDev, pSpiDev, pPkg);
            }
        }

    if (pPkg->usDelay > 0)
        {
        vxbUsDelay(pPkg->usDelay);
        }

    if (pDrvCtrl->initPhase >= 2)
        {
        semGive (pDrvCtrl->muxSem);
        }

    return sts;
    }


/*******************************************************************************
*
* vxCanDmaCallback - callback routine when DMA transfer is done 
*
* This routine is called when DMA transfer is done. The routine runs
* in interrupt context
*
* RETURNS: N/A 
*
* ERRNO: N/A
*/

LOCAL void vxCanDmaCallback
    (
    SEM_ID      semId
    )
    {
    CAN_DBG(CAN_DBG_ISR, "DMA channel callback function\n", 1, 2, 3, 4, 5, 6);

    if (semId != 0)
        {
        semGive (semId);
        }
    }

/*******************************************************************************
*
* vxCanDmaChanAlloc - allocate DMA resource 
*
* This routine allocates DMA resource 
*
* RETURNS: N/A 
*
* ERRNO: N/A
*/

LOCAL STATUS vxCanDmaChanAlloc
    (
    VXB_DEVICE_ID pDev,
    UINT32        channel
    )
    {
    VXB_DEVICE_ID pInst;
    FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL *)pDev->pDrvCtrl;

    if (pDrvCtrl->dmaTxRes[channel] != NULL &&
        pDrvCtrl->dmaRxRes[channel] != NULL)
        {
        CAN_DBG (CAN_DBG_RTN,
                 "dma channels already allocated\n",
                 1, 2, 3, 4, 5, 6);
        return OK;
        }

    /* find EDMA3 controller instance */

    pInst = vxbInstByNameFind ("tiedma3", 0);
    if (pInst == NULL)
        {
        CAN_DBG(CAN_DBG_ERR,
                    "Can not find DMA controller instance\n",
                    1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    pDrvCtrl->pTxChanParam[channel] = (TI_EDMA3_PARAM *)malloc (
                                      sizeof (TI_EDMA3_PARAM));
    if (pDrvCtrl->pTxChanParam[channel] == NULL)
        {
        goto DmaChanAllocfailed;
        }

    bzero ((char *)pDrvCtrl->pTxChanParam[channel], sizeof (TI_EDMA3_PARAM));

    pDrvCtrl->pTxChanParam[channel]->opt     = TI_EDMA3_PRIV_SUP |
                                               TI_EDMA3_SYNCDIM_A;

    pDrvCtrl->pTxChanParam[channel]->link    = TI_EDMA3_LINK_END;

    memset (&pDrvCtrl->txInfo[channel], 0, sizeof (TI_EDMA3_CHAN_ALLOC_INFO));
    memset (&pDrvCtrl->rxInfo[channel], 0, sizeof (TI_EDMA3_CHAN_ALLOC_INFO));

    pDrvCtrl->txInfo[channel].requestedChan = pDrvCtrl->edmaTEvt[channel];;
    pDrvCtrl->txInfo[channel].autoTrigger = TRUE;
    pDrvCtrl->txInfo[channel].usrProvideParam = TRUE;
    pDrvCtrl->txInfo[channel].data = pDrvCtrl->pTxChanParam[channel];

    /* allocate TX channel */

    pDrvCtrl->dmaTxRes[channel] = vxbDmaChanAlloc (pInst, 1, 0,
                                                   &pDrvCtrl->txInfo[channel]);
    if (pDrvCtrl->dmaTxRes[channel] == NULL)
        {
        CAN_DBG (CAN_DBG_ERR,
                    "Can not allocate tx dma resource\n",
                    0, 0, 0, 0, 0, 0);
        goto DmaChanAllocfailed;
        }

    pDrvCtrl->pRxChanParam[channel] = (TI_EDMA3_PARAM *)malloc (
                                      sizeof (TI_EDMA3_PARAM));
    if (pDrvCtrl->pRxChanParam[channel] == NULL)
        {
        goto DmaChanAllocfailed;
        }
    bzero ((char *)pDrvCtrl->pRxChanParam[channel], sizeof (TI_EDMA3_PARAM));

    pDrvCtrl->pRxChanParam[channel]->opt     = TI_EDMA3_PRIV_SUP |
                                               TI_EDMA3_TCINTEN |
                                               TI_EDMA3_SYNCDIM_A;

    pDrvCtrl->pRxChanParam[channel]->link    = TI_EDMA3_LINK_END;

    pDrvCtrl->rxInfo[channel].requestedChan = pDrvCtrl->edmaREvt[channel];
    pDrvCtrl->rxInfo[channel].autoTrigger = TRUE;
    pDrvCtrl->rxInfo[channel].usrProvideParam = TRUE;
    pDrvCtrl->rxInfo[channel].data = pDrvCtrl->pRxChanParam[channel];

    /* allocate RX channel */

    pDrvCtrl->dmaRxRes[channel] = vxbDmaChanAlloc (pInst, 1, 0,
                                                   &pDrvCtrl->rxInfo[channel]);
    if (pDrvCtrl->dmaRxRes[channel] == NULL)
        {
        CAN_DBG (CAN_DBG_ERR,
                    "Can not allocate rx dma resource\n",
                    0, 0, 0, 0, 0, 0);
        goto DmaChanAllocfailed;
        }

    return OK;

DmaChanAllocfailed:

    if (pDrvCtrl->dmaTxRes[channel])
        {
        vxbDmaChanFree (pDrvCtrl->dmaTxRes[channel]);
        pDrvCtrl->dmaTxRes[channel] = NULL;
        }

    if (pDrvCtrl->dmaRxRes[channel])
        {
        vxbDmaChanFree (pDrvCtrl->dmaRxRes[channel]);
        pDrvCtrl->dmaRxRes[channel] = NULL;
        }

    return ERROR;
    }

/*******************************************************************************
*
* vxCanDmaChanFree - free DMA resource 
*
* This routine frees DMA resource 
*
* RETURNS: N/A 
*
* ERRNO: N/A
*/

LOCAL void vxCanDmaChanFree
    (
    VXB_DEVICE_ID pDev
    )
    {   
    UINT32 i;
    FM_CAN_DRV_CTRL * pDrvCtrl = (FM_CAN_DRV_CTRL *)pDev->pDrvCtrl;

    for (i = 0; i < CAN_MAX_CS_NUM; i++)
        {
        if (pDrvCtrl->dmaTxRes[i] != NULL)
            {
            vxbDmaChanFree(pDrvCtrl->dmaTxRes[i]);
            pDrvCtrl->dmaTxRes[i] = NULL;
            }

        if (pDrvCtrl->pTxChanParam[i] != NULL)
            {
            free (pDrvCtrl->pTxChanParam[i]);
            pDrvCtrl->pTxChanParam[i] = NULL;
            }

        if (pDrvCtrl->dmaRxRes[i] != NULL)
            {
            vxbDmaChanFree(pDrvCtrl->dmaRxRes[i]);
            pDrvCtrl->dmaRxRes[i] = NULL;
            }

        if (pDrvCtrl->pRxChanParam[i] != NULL)
            {
            free (pDrvCtrl->pRxChanParam[i]);
            pDrvCtrl->pRxChanParam[i] = NULL;
            }
        }
    }

/*****************************************************************************
*
* vxCanShow - show the controller info
*
* This function shows the SPI controller's info.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxCanShow
    (
    VXB_DEVICE_ID pDev,
    int verbose
    )
    {
    FM_CAN_DRV_CTRL * pDrvCtrl;

    /* check for valid parameter */

    VXB_ASSERT_NONNULL_V (pDev);

    pDrvCtrl = (FM_CAN_DRV_CTRL *) pDev->pDrvCtrl;

    printf ("        %s unit %d on %s @ %p with busInfo %p\n",
            pDev->pName,
            pDev->unitNumber,
            vxbBusTypeString (pDev->busID),
            pDev,
            pDev->u.pSubordinateBus);

    if (verbose > 0)
        {
        printf ("            BAR0 @ 0x%08x (memory mapped)\n",
                                                   pDev->pRegBase[0]);
        printf ("            pDrvCtrl @ 0x%08x\n", pDev->pDrvCtrl);
        printf ("            clkFrequency:  %dMhz\n",
                             pDrvCtrl->clkFrequency / 1000000);
        printf ("            current working frequency:  %dhz\n",
                             pDrvCtrl->curWorkingFrq);
        printf ("            interrupt   :  %s\n",
                             pDrvCtrl->polling ? "FALSE" : "TRUE");
        printf ("            dma   :  %s\n",
                             pDrvCtrl->dmaMode ? "TRUE" : "FALSE");
        printf ("            init  :  %s\n", 
                             pDrvCtrl->initDone ? "SUCCESS" : "FAIL");
        }
}
#endif

#if 1

int g_test_can3 = 0;


/*
test SLCR_Loop
*/

int gRegister2Drv_Can = 0;

STATUS test_can_loop2_vx(void)
{
    VXB_DEVICE_ID       pDev0;
    FM_CAN_DRV_CTRL * pDrvCtrl0;
	
    VXB_DEVICE_ID       pDev1;
    FM_CAN_DRV_CTRL * pDrvCtrl1;
	
	int unit = 0;	
	int i = 0;
	
	UINT8 tx_buf[16] = {0};
	UINT8 rx_buf[16] = {0};

	if (gRegister2Drv_Can == 0)
	{
		vxCanRegister2();
		gRegister2Drv_Can = 1;
	}
	
	/*==================================================*/
	unit = 0;
	pDev0 = vxbInstByNameFind (FM_CAN_NAME, unit);
	if (pDev0 == NULL)
	{
		printf ("\nCan not find this spi unit(%d)!	\n\n", unit);
		return ERROR;
	}
	else
	{
		pDrvCtrl0 = (FM_CAN_DRV_CTRL *)pDev0->pDrvCtrl;
		printf ("vxbInstByNameFind(%s)_%d: 0x%X \n", FM_CAN_NAME, unit, pDev0);
	}
	/*==================================================*/

	/*==================================================*/
	unit = 1;
	pDev1 = vxbInstByNameFind (FM_CAN_NAME, unit);
	if (pDev1 == NULL)
	{
		printf ("\nCan not find this spi unit(%d)!	\n\n", unit);
		return ERROR;
	}
	else
	{
		pDrvCtrl1 = (FM_CAN_DRV_CTRL *)pDev1->pDrvCtrl;
		printf ("vxbInstByNameFind(%s)_%d: 0x%X \n\n", FM_CAN_NAME, unit, pDev1);
	}
	/*==================================================*/
	
	
	for (i=0; i<8; i++)
	{        
		tx_buf[i] = g_test_can3 + i;
	}
	g_test_can3++;
	
	printf("vx-can0_tx-src_data: \n");
    for(i = 0; i < 11; i++)
    {
       printf("%02X ", tx_buf[i]);
    }
	
	vxCanCtrl_SLCR_Loop(pDev0, pDev1, tx_buf, rx_buf);
	
    for(i=0; i<11; i++)
    {
		tx_buf[i] = vxCanCtrl_Rd_CfgReg32(pDev0, (CAN_REG_TXFIFO + 4 * i));
    }
	
	printf("\nvx-can0_tx: \n");
    for(i = 0; i < 11; i++)
    {
       printf("%02X ", tx_buf[i]);
    }
	printf("\n");
	
	printf("vx-can1_rx: \n");
    for(i = 0; i < 11; i++)
    {
       printf("%02X ", rx_buf[i]);
    }
	printf("\n\n");
	

	return OK;
}


/*
test X_Loop
*/
STATUS test_can_loop3_vx(UINT32 tx_data)
{
}

void test_rd_can_regs(void)
{
	UINT32* pReg = NULL;
	UINT32 base = 0;
	int i = 0;
	
    VXB_DEVICE_ID       pDev0;
    FM_CAN_DRV_CTRL * pDrvCtrl0;
	
    VXB_DEVICE_ID       pDev1;
    FM_CAN_DRV_CTRL * pDrvCtrl1;
	
	int unit = 0;

	/*////////////////////////////////////////////////////////////////////////////////*/
	unit = 0;
    pDev0 = vxbInstByNameFind (FM_CAN_NAME, unit);
    /*pDev0 = vxbDevAcquireByName (FM_CAN_NAME, unit);*/
    if (pDev0 == NULL)
    {
        printf ("\n Can not find this qspi unit(%d)!  \n\n", unit);
        return ERROR;
    }
	else
	{
		pDrvCtrl0 = (FM_CAN_DRV_CTRL*)(pDev0->pDrvCtrl);
		printf ("\n vxbInstByNameFind(%s)_%d: 0x%X \n\n", FM_CAN_NAME, unit, pDev0);
	}	

	printf("regBase: 0x%X \n", pDrvCtrl0->regBase);
	
	base = pDrvCtrl0->regBase;

	for (i=0; i<0xb4; i+=4)
	{
		pReg = (UINT32*)(base + i);
		printf("reg[%02X] = 0x%08X \n", i, *pReg);
	}	
	/*////////////////////////////////////////////////////////////////////////////////*/


	
	/*////////////////////////////////////////////////////////////////////////////////*/
	unit = 1;
    pDev1 = vxbInstByNameFind (FM_CAN_NAME, unit);
    if (pDev1 == NULL)
    {
        printf ("\n Can not find this qspi unit(%d)!  \n\n", unit);
        return ERROR;
    }
	else
	{
		pDrvCtrl1 = (FM_CAN_DRV_CTRL*)(pDev1->pDrvCtrl);
		printf ("\n vxbInstByNameFind(%s)_%d: 0x%X \n\n", FM_CAN_NAME, unit, pDev1);
	}	

	printf("regBase: 0x%X \n", pDrvCtrl1->regBase);
	
	base = pDrvCtrl1->regBase;

	for (i=0; i<0xb4; i+=4)
	{
		pReg = (UINT32*)(base + i);
		printf("reg[%02X] = 0x%08X \n", i, *pReg);
	}	
	/*////////////////////////////////////////////////////////////////////////////////*/

	return;
}

#endif


