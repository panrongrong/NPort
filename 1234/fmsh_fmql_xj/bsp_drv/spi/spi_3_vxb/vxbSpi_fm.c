/* vxSpi_fm.c - FMSH SPI Controller Driver */

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
routine is the vxSpiRegister(), which registers the driver with
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
LOCAL struct vxSpiDevInfo spiDevTbl[] = {
    { "spiFlash_w25q64",       0, 8, 24000000,  SPI_MODE_0},
};

LOCAL UINT32 txEvt = {16, 18};
LOCAL UINT32 rxEvt = {17, 19};

LOCAL struct hcfResource vxSpiResources[] =  {
    { "regBase",       HCF_RES_INT,   { (void *)(AM335X_SPI0_BASE) } },
    { "clkFreq",       HCF_RES_INT,   { (void *)(48000000)  } },
    { "polling",       HCF_RES_INT,   { (void *)(FALSE) } },
    { "dmaMode",       HCF_RES_INT,   { (void *)(TRUE) } },
    { "spiDev",        HCF_RES_ADDR,  { (void *)&spiDevTbl} },
    { "spiDevNum",     HCF_RES_INT,   { (void *)NELEMENTS(spiDevTbl)}}
    { "edmaTEvt",      HCF_RES_ADDR,  { (void *)&txEvt} },
    { "edmaREvt",      HCF_RES_ADDR,  { (void *)&rxEvt} },
};
#   define vxSpiNum  NELEMENTS(vxSpiResources)
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

/*#include <../src/hwif/h/spi/vxSpi_fm.h>*/
#include "vxbSpi_fm.h"

/* debug macro */
#undef  SPI_DBG_ON
#define  SPI_DBG_ON

#ifdef  SPI_DBG_ON

/* turning local symbols into global symbols */
#ifdef  LOCAL
#undef  LOCAL
#endif
#define LOCAL

#define SPI_DBG_OFF            0x00000000
#define SPI_DBG_ISR            0x00000001
#define SPI_DBG_RW             0x00000002
#define SPI_DBG_ERR            0x00000004
#define SPI_DBG_RTN            0x00000008
#define SPI_DBG_ALL            0xffffffff

IMPORT FUNCPTR _func_logMsg;

#define SPI_DBG(mask, string, X1, X2, X3, X4, X5, X6)       \
        if (_func_logMsg != NULL) \
           (* _func_logMsg)(string, (int)X1, (int)X2, (int)X3, (int)X4, \
                (int)X5, (int)X6)
                
#define SPI_DBG1(string, X1, X2, X3, X4, X5, X6)  printf(string, (int)X1, (int)X2, (int)X3, (int)X4, (int)X5, (int)X6)
	
#else
#define SPI_DBG(mask, string, X1, X2, X3, X4, X5, X6)
#endif  /* SPI_DBG_ON */

#define SPI_DBG2(string, X1, X2, X3, X4, X5, X6)  printf(string, (int)X1, (int)X2, (int)X3, (int)X4, (int)X5, (int)X6)

extern void sysUsDelay(int);

#define sysRead32(baseAddr)		    *((volatile unsigned int *)(baseAddr))
#define sysWrite32(baseAddr, data)	*((volatile unsigned int *)(baseAddr)) = data

#define SPI_REG_READ_32(pDrv, addr)     sysRead32 (pDrv->regBase + addr)
#define SPI_REG_WRITE_32(pDrv, addr, data)  sysWrite32 (pDrv->regBase + addr, data)



/* VxBus methods */
#if 1
LOCAL void vxSpiInstInit (VXB_DEVICE_ID pDev);
LOCAL void vxSpiInstInit2 (VXB_DEVICE_ID pDev);
LOCAL void vxSpiInstConnect (VXB_DEVICE_ID pDev);
LOCAL void vxSpiShow (VXB_DEVICE_ID, int);
LOCAL STATUS vxSpiInstUnlink (VXB_DEVICE_ID pDev, void * unused);

/* forward declarations */
LOCAL void vxSpiCtrl_Init (VXB_DEVICE_ID pDev);
LOCAL void vxSpiIsr (VXB_DEVICE_ID pDev);

void vxSpiSlcr_Set_SpiClk(void);
void vxSpiSlcr_Set_SpiLoop(UINT32 loop_en);
int vxSpiCtrl_Init_Master(VXB_DEVICE_ID pDev);
int vxSpiCtrl_Init_Slave(VXB_DEVICE_ID pDev);
void vxSpiCtrl_Reset(VXB_DEVICE_ID pDev, int ctrl_x);

extern void sysMsDelay(int);
#endif


#if 1
/* locals */

LOCAL struct drvBusFuncs vxSpiVxbFuncs = 
{
    vxSpiInstInit,      /* devInstanceInit */
    vxSpiInstInit2,     /* devInstanceInit2 */
    vxSpiInstConnect    /* devConnect */
};

/*
LOCAL device_method_t vxSpiDeviceMethods[] = 
{
    DEVMETHOD (vxSpiControlGet, vxSpiCtrlGet),
    DEVMETHOD (busDevShow,       vxSpiShow),
    DEVMETHOD (vxbDrvUnlink,     vxSpiInstUnlink),
    DEVMETHOD_END
};
*/

LOCAL struct vxbPlbRegister vxSpiDevRegistration = 
{
    {
    NULL,                   /* pNext */
    VXB_DEVID_DEVICE,       /* devID */
    VXB_BUSID_PLB,          /* busID = PLB */
    VXB_VER_4_0_0,          /* vxbVersion */
    
    FM_SPI_NAME,            /* drvName : fmSpi */
    
    &vxSpiVxbFuncs,         /* pDrvBusFuncs */
    NULL /*vxSpiDeviceMethods*/,     /* pMethods */
    NULL,                   /* devProbe  */
    NULL,                   /* pParamDefaults */
    },
};


/*****************************************************************************
*
* vxSpiRegister - register with the VxBus subsystem
*
* This routine registers the SPI driver with VxBus Systems.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void vxSpiRegister (void)
{
    /*vxbDevRegister ((struct vxbDevRegInfo * &vxSpiDevRegistration);*/
}

void vxSpiRegister2 (void)
{
    vxbDevRegister ((struct vxbDevRegInfo *) &vxSpiDevRegistration);
}

/*****************************************************************************
*
* vxSpiInstInit - initialize fmsh SPI controller
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

LOCAL void vxSpiInstInit
    (
    VXB_DEVICE_ID pDev
    )
{
    FM_SPI_DRV_CTRL * pDrvCtrl;
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
    /*pDrvCtrl = (FM_SPI_DRV_CTRL * hwMemAlloc (sizeof (FM_SPI_DRV_CTRL));*/
    pDrvCtrl = (FM_SPI_DRV_CTRL *) malloc (sizeof (FM_SPI_DRV_CTRL));
    if (pDrvCtrl == NULL)
    {
        return;
    }
	
    bzero ((char *)pDrvCtrl, sizeof(FM_SPI_DRV_CTRL));

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
	printf("<Spi>pDev->unitNumber:(%d); pDrvCtrl->regBase:0x%08X \n", \
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
                        (void *) &pDrvCtrl->clkFrequency) != OK)
    {        
        pDrvCtrl->clkFrequency = 100000000;  /* 100M Hz */
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

    if (devResourceGet (pHcf, "isSlave", HCF_RES_INT,
                        (void *) &pDrvCtrl->isSlave) != OK)
    {
        pDrvCtrl->isSlave = FALSE;  /* default: master*/
    }

    /* install the transfer routine for SPI Lib */
    /*pDrvCtrl->vxSpiCtrl.spiTransfer = (void *vxSpiTransfer;*/

    /* announce that there's a SPI bus */
    /*(void) vxbBusAnnounce (pDev, VXB_BUSID_SPI);*/

    /* notify the bus subsystem of all devices on SPI */
    /*(void) spiBusAnnounceDevices (pDev);*/

    /* the first phase of the initialization */

    pDrvCtrl->initPhase = 1;
    }

/*******************************************************************************
*
* vxSpiCtrlInit -  SPI controller initialization
*
* This routine performs the SPI controller initialization.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxSpiCtrl_Init (VXB_DEVICE_ID pDev)
{
    FM_SPI_DRV_CTRL * pDrvCtrl;
	
    UINT32 value;
    UINT32 us = 0;
	int ctrl_x = 0;

    /* check for valid parameter */
   /* VXB_ASSERT_NONNULL_V (pDev);*/

    pDrvCtrl = (FM_SPI_DRV_CTRL *) pDev->pDrvCtrl;	
	pDrvCtrl->CTRL.ctrl_x = pDev->unitNumber;
	
    pDrvCtrl->CTRL.cfgBaseAddr = (UINT32*)pDrvCtrl->regBase;

	if (pDrvCtrl->isSlave == TRUE)
	{
		pDrvCtrl->CTRL.mode_mst_slv = MODE_SPI_SLAVE;
		pDrvCtrl->CTRL.isMaster = FALSE;	
	}
	else
	{
		pDrvCtrl->CTRL.mode_mst_slv = MODE_SPI_MASTER;	
		pDrvCtrl->CTRL.isMaster = TRUE; 
	}
	
	ctrl_x = pDrvCtrl->CTRL.ctrl_x;
	pDrvCtrl->CTRL.devId = ctrl_x;

	vxSpiSlcr_Set_SpiClk();

    switch (pDrvCtrl->CTRL.mode_mst_slv)
	{
	case MODE_SPI_MASTER:
		vxSpiCtrl_Init_Master(pDev);
		break;
		
	case MODE_SPI_SLAVE:
		vxSpiCtrl_Init_Slave(pDev);
		break;	
	}
	
    pDrvCtrl->initDone = TRUE;
}

/*******************************************************************************
*
* vxSpiInstInit2 - second level initialization routine of SPI controller
*
* This routine performs the second level initialization of the SPI controller.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxSpiInstInit2
    (
    VXB_DEVICE_ID pDev
    )
{
    FM_SPI_DRV_CTRL * pDrvCtrl;

    /* check for valid parameter */
    /*VXB_ASSERT_NONNULL_V (pDev);*/

    pDrvCtrl = (FM_SPI_DRV_CTRL *) pDev->pDrvCtrl;

    /* 
    The semSync semaphore is used to synchronize the SPI transfer. 
	*/
    pDrvCtrl->semSync = semBCreate (SEM_Q_PRIORITY, SEM_FULL);
    if (pDrvCtrl->semSync == NULL)
    {
        SPI_DBG (SPI_DBG_ERR, "semBCreate failed for semSync\n",
                 0, 0, 0, 0, 0, 0);
        return;
    }

    /* 
    The muxSem semaphore is used to mutex accessing the controller. 
    */
    pDrvCtrl->muxSem = semMCreate (SEM_Q_PRIORITY);
    if (pDrvCtrl->muxSem == NULL)
    {
        SPI_DBG (SPI_DBG_ERR, "semMCreate failed for muxSem\n",
                 0, 0, 0, 0, 0, 0);

        (void) semDelete (pDrvCtrl->semSync);
        pDrvCtrl->semSync = NULL;

        return;
    }
	
    /* 
    SPI controller init 
    */
    (void) vxSpiCtrl_Init(pDev);

    pDrvCtrl->initPhase = 2;
}

/*******************************************************************************
*
* vxSpiInstConnect - third level initialization
*
* This routine performs the third level initialization of the SPI controller
* driver.
*
* RETURNS: N/A
*
* ERRNO : N/A
*/

LOCAL void vxSpiInstConnect
    (
    VXB_DEVICE_ID       pDev
    )
{
    FM_SPI_DRV_CTRL * pDrvCtrl;

    /* check for valid parameter */
    /*VXB_ASSERT_NONNULL_V (pDev);*/

    pDrvCtrl = (FM_SPI_DRV_CTRL *) pDev->pDrvCtrl;

    /* 
    connect and enable interrupt for non-poll mode 
	*/
    if (!pDrvCtrl->polling)
    {
        if (vxbIntConnect (pDev, 0, vxSpiIsr, pDev) != OK)
        {
            SPI_DBG(SPI_DBG_ERR, "vxbIntConnect return ERROR \n",
                    1, 2, 3, 4, 5, 6);
        }
        (void) vxbIntEnable (pDev, 0, vxSpiIsr, pDev);
    }

    pDrvCtrl->initPhase = 3;
}

/*****************************************************************************
*
* vxSpiInstUnlink - VxBus unlink handler
*
* This function shuts down a SPI controller instance in response to an
* an unlink event from VxBus. This may occur if our VxBus instance has
* been terminated, or if the SPI driver has been unloaded.
*
* RETURNS: OK if device was successfully destroyed, otherwise ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxSpiInstUnlink
    (
    VXB_DEVICE_ID pDev,
    void *        unused
    )
{
    /* check if the pDev pointer is valid */
    /*VXB_ASSERT (pDev != NULL, ERROR)*/

    FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL *) pDev->pDrvCtrl;

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
            (void) vxbIntDisable (pDev, 0, vxSpiIsr, pDev);

            if (vxbIntDisconnect (pDev, 0, vxSpiIsr, pDev) != OK)
            {
                SPI_DBG(SPI_DBG_ERR, "vxbIntDisconnect return ERROR \n",
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
        /*vxSpiDmaChanFree (pDev);*/
    }

    pDrvCtrl->initPhase = 0;
    pDev->pDrvCtrl = NULL;

    return (OK);
}


/*******************************************************************************
*
* vxSpiIsr - interrupt service routine
*
* This routine handles interrupts of SPI.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxSpiIsr
    (
    VXB_DEVICE_ID pDev
    )
{
    FM_SPI_DRV_CTRL * pDrvCtrl;

    /* Check if the pDev pointer is valid */
    VXB_ASSERT (pDev != NULL, ERROR)

    pDrvCtrl = (FM_SPI_DRV_CTRL *)pDev->pDrvCtrl;

    /* disable all SPI interrupts */
/*
	CSR_WRITE_4 (pDev, MCSPI_IRQENABLE, 0);

    SPI_DBG (SPI_DBG_ISR, "vxSpiIsr: intSts 0x%x\n", 
             CSR_READ_4 (pDev, MCSPI_IRQSTATUS), 2, 3, 4, 5, 6);
*/
    semGive (pDrvCtrl->semSync);

	return;
}


#endif

#if 1

void vxSpiCtrl_Wr_CfgReg32(VXB_DEVICE_ID pDev, UINT32 offset, UINT32 value)
{
	FM_SPI_DRV_CTRL* pDrvCtrl = NULL;
	UINT32 tmp32 = 0;
	
	pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	tmp32 = pDrvCtrl->CTRL.cfgBaseAddr;	
	
	FMQL_WRITE_32((tmp32 + offset), value);
	return;
}

UINT32 vxSpiCtrl_Rd_CfgReg32(VXB_DEVICE_ID pDev, UINT32 offset)
{
	FM_SPI_DRV_CTRL* pDrvCtrl = NULL;
	UINT32 tmp32 = 0;
	
	pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	tmp32 = pDrvCtrl->CTRL.cfgBaseAddr;	
	
	return FMQL_READ_32(tmp32 + offset);
}

void vxSpiCtrl_Reset(VXB_DEVICE_ID pDev, int ctrl_x)
{  
	FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	
	UINT32 tmp32 = 0;
	
    switch (ctrl_x)
    {
    case SPI_CTRL_0:
        tmp32 = slcr_read(SLCR_SPI0_CTRL) | (SPI0_APB_RST_MASK | SPI0_REF_RST_MASK);
		slcr_write(SLCR_SPI0_CTRL, tmp32);
		delay_ms(1);	
        tmp32 = slcr_read(SLCR_SPI0_CTRL) & (~(SPI0_APB_RST_MASK | SPI0_REF_RST_MASK));
		slcr_write(SLCR_SPI0_CTRL, tmp32);
		break;
		
    case SPI_CTRL_1:
        tmp32 = slcr_read(SLCR_SPI1_CTRL) | (SPI1_APB_RST_MASK | SPI1_REF_RST_MASK);
		slcr_write(SLCR_SPI1_CTRL, tmp32);
		delay_ms(1);	
        tmp32 = slcr_read(SLCR_SPI1_CTRL) & (~(SPI1_APB_RST_MASK | SPI1_REF_RST_MASK));
		slcr_write(SLCR_SPI1_CTRL, tmp32);
		break;
    }
	
	return;
}


void vxSpiCtrl_Set_Master(VXB_DEVICE_ID pDev)
{
	FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    vxSpiCtrl_Wr_CfgReg32(pDev, SPIPS_MSTR_OFFSET, 0x1);
    pDrvCtrl->CTRL.isMaster = TRUE;
	
	return;
}

int vxSpiCtrl_Set_Baud_Mst(VXB_DEVICE_ID pDev, UINT32 sck_dv)
{
	FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    if (pDrvCtrl->CTRL.isEnable == TRUE || pDrvCtrl->CTRL.isMaster == FALSE)
    {
        return ERROR;
    }
	
    /*
	Fsclk_out = Fssi_clk/SCKDV
	*/
    vxSpiCtrl_Wr_CfgReg32(pDev, SPIPS_BAUDR_OFFSET, sck_dv);  
    
    return OK;
}

void vxSpiCtrl_Set_Slave(VXB_DEVICE_ID pDev)
{
	FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    vxSpiCtrl_Wr_CfgReg32(pDev, SPIPS_MSTR_OFFSET, 0x0);
    pDrvCtrl->CTRL.isMaster = FALSE;
	
	return;
}

/*
仅当SSI配置从机操作时才有效；该位会禁止/使能ssi_oe_n信号，当SLV_OE=1时，ssi_oe_n信号不会被激活，
所以由ssi_oe_n信号控制的tri-state buffer 在txd输出上永远呈现高阻。
当主设备以广播方式传输时，如果用户不希望从机回应，可以采用该配置。
	0：Slave txd is enabled
	1：Slave txd is disabled
	复位值：0x0
*/
int vxSpiCtrl_Set_enSlvTxd(VXB_DEVICE_ID pDev, BOOL enable)
{
	FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    UINT32 cfgReg = 0;
    
    if (pDrvCtrl->CTRL.isEnable == TRUE)
    {
        return ERROR;
    }  
	
    cfgReg = vxSpiCtrl_Rd_CfgReg32(pDev, SPIPS_CTRLR0_OFFSET);
    if (enable) 
    {
        cfgReg &= ~SPIPS_CTRL0_SLVOE_MASK;  /* 0：Slave txd is enabled */
    }
    else 
    {
        cfgReg |= SPIPS_CTRL0_SLVOE_MASK;   /* 1：Slave txd is disabled */
    }
	
    vxSpiCtrl_Wr_CfgReg32(pDev, SPIPS_CTRLR0_OFFSET, cfgReg);  
    
    return OK;
}

int vxSpiCtrl_Set_XfrMode(VXB_DEVICE_ID pDev, UINT32 xfr_mode)
{
	FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    UINT32 cfgReg = 0;
    
    if (pDrvCtrl->CTRL.isEnable == TRUE)
    {
        return ERROR;
    }   
	
    if (xfr_mode > 3)
    {
        return ERROR;
    }
    
    cfgReg = vxSpiCtrl_Rd_CfgReg32(pDev, SPIPS_CTRLR0_OFFSET);
	
    cfgReg &= ~SPIPS_CTRL0_TMOD_MASK;
    cfgReg |= (xfr_mode << SPIPS_CTRL0_TMOD_SHIFT);
	
    vxSpiCtrl_Wr_CfgReg32(pDev, SPIPS_CTRLR0_OFFSET, cfgReg); 
    
    return OK;
}

int vxSpiCtrl_Set_SckMode(VXB_DEVICE_ID pDev, UINT32 sck_mode)
{
	FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    UINT32 cfgReg = 0;
    
    if(pDrvCtrl->CTRL.isEnable == TRUE)
    {
        return ERROR;
    }
	
    if (sck_mode > 3)
    {
        return ERROR;
    }
	
    cfgReg = vxSpiCtrl_Rd_CfgReg32(pDev, SPIPS_CTRLR0_OFFSET);
	
    cfgReg &= ~(SPIPS_CTRL0_SCPH_MASK | SPIPS_CTRL0_SCPOL_MASK);
    cfgReg |= (sck_mode << SPIPS_CTRL0_SCPH_SHIFT);
	
    vxSpiCtrl_Wr_CfgReg32(pDev, SPIPS_CTRLR0_OFFSET, cfgReg); 
    
    return OK;
}

/*
用来选择传输所用数据帧的长度
bit20~16:	DFS_32, DFS_32字段数值+1为设置的数据帧长度

*/
int vxSpiCtrl_Set_FrmLen(VXB_DEVICE_ID pDev, UINT32 dfs32)
{
	FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    UINT32 cfgReg = 0;
    
    if (pDrvCtrl->CTRL.isEnable == TRUE)
    {
        return ERROR;
    }
	
    if ((dfs32 < 4) || (dfs32 > 0x20))
    {
        return ERROR;
    }
    
    cfgReg = vxSpiCtrl_Rd_CfgReg32(pDev, SPIPS_CTRLR0_OFFSET);
	
    cfgReg &= ~SPIPS_CTRL0_DFS32_MASK;
    cfgReg |= ((dfs32 - 1) << SPIPS_CTRL0_DFS32_SHIFT);
	
    vxSpiCtrl_Wr_CfgReg32(pDev, SPIPS_CTRLR0_OFFSET, cfgReg); 
    
    pDrvCtrl->CTRL.frmSize = dfs32;
    
    return OK;
}

/*
bit11	SRL:
	0：正常模式操作
	1：测试模式操作, loopback
*/
int vxSpiCtrl_Set_LoopBack(VXB_DEVICE_ID pDev, BOOL enable)
{
	FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    UINT32 cfgReg = 0;
    
    if (pDrvCtrl->CTRL.isEnable == TRUE)
    {
        return ERROR;
    }    
	
	/* CTRLR0 bit11: SRL  */
    cfgReg = vxSpiCtrl_Rd_CfgReg32(pDev, SPIPS_CTRLR0_OFFSET);
	
    if (enable)  
    {
        cfgReg |= SPIPS_CTRL0_SRL_MASK;  
    }
    else  
    {
        cfgReg &= ~SPIPS_CTRL0_SRL_MASK;
    }
	
    vxSpiCtrl_Wr_CfgReg32(pDev, SPIPS_CTRLR0_OFFSET, cfgReg); 
    
    return OK;
}


/*
接收数据帧的数量
*/
int vxSpiCtrl_Set_RcvFrmNum(VXB_DEVICE_ID pDev, UINT32 dfNum)
{
	FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    if (pDrvCtrl->CTRL.isEnable == TRUE || pDrvCtrl->CTRL.isMaster == FALSE)
    {
        return ERROR;
    }
	
    vxSpiCtrl_Wr_CfgReg32(pDev, SPIPS_CTRLR1_OFFSET, (dfNum-1));
    
    return OK;
}


/*
发送FIFO阈值；当TX FIFO中数据个数小于或等于阈值时，触发发送FIFO空中断。
*/
int vxSpiCtrl_Set_TxFiFoEmptyLvl(VXB_DEVICE_ID pDev, UINT8 tx_lvl)
{
	FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    if (pDrvCtrl->CTRL.isEnable == TRUE)
    {
        return ERROR;
    }    
	
    vxSpiCtrl_Wr_CfgReg32(pDev, SPIPS_TXFTLR_OFFSET, tx_lvl);
    
    return OK;
}

int vxSpiCtrl_Set_RxFifoFullLvl(VXB_DEVICE_ID pDev, UINT8 rx_lvl)
{
	FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    if (pDrvCtrl->CTRL.isEnable == TRUE)
    {
        return ERROR;
    }  
	
    vxSpiCtrl_Wr_CfgReg32(pDev, SPIPS_RXFTLR_OFFSET, rx_lvl);
    
    return OK;
}


/*
DMA发送数据水位值，控制着发送逻辑何时产生DMA请求；
当TX FIFO中数据小于或等于水位值,且TDMAE=1时，产生dma_tx_req；
*/
void vxSpiCtrl_Set_DMATxLvl(VXB_DEVICE_ID pDev, UINT32 tx_lvl)
{
    vxSpiCtrl_Wr_CfgReg32(pDev, SPIPS_DMATDLR_OFFSET, tx_lvl);
	return;
}

void vxSpiCtrl_Set_DMARxLvl(VXB_DEVICE_ID pDev, UINT32 rx_lvl)
{
    vxSpiCtrl_Wr_CfgReg32(pDev, SPIPS_DMARDLR_OFFSET, rx_lvl);
	return;
}

/*
IRQ Mask Register
*/
void vxSpiCtrl_DisIRQ(VXB_DEVICE_ID pDev, UINT32 mask)
{
    UINT32 cfgReg = 0;
    
    cfgReg = vxSpiCtrl_Rd_CfgReg32(pDev, SPIPS_IMR_OFFSET);
    cfgReg &= ~mask;
	
    vxSpiCtrl_Wr_CfgReg32(pDev, SPIPS_IMR_OFFSET, cfgReg);
	return;
}

/*

从机选择使能标志，该寄存器中每一位都对应着一个从机。
该寄存器仅在SSI主机模块中有效。该寄存器用来选择与SSI模块连接的外设从机
*/
int vxSpiCtrl_Set_PeerSlave(VXB_DEVICE_ID pDev, UINT32 ctrl_x)
{    
	FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    if (pDrvCtrl->CTRL.isMaster == FALSE)
    {
        return ERROR;
    }
	
    if (ctrl_x == 0)
    {
        vxSpiCtrl_Wr_CfgReg32(pDev, SPIPS_SER_OFFSET, 0x0); 
    }
    else
    {
        vxSpiCtrl_Wr_CfgReg32(pDev, SPIPS_SER_OFFSET, (0x1 << (ctrl_x - 1))); 
    }
    
    return OK;
}

void vxSpiCtrl_Disable(VXB_DEVICE_ID pDev)
{
	FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    vxSpiCtrl_Wr_CfgReg32(pDev, SPIPS_SSIENR_OFFSET, 0x0);	
    pDrvCtrl->CTRL.isEnable = FALSE;
	return;
}

void vxSpiCtrl_Enable(VXB_DEVICE_ID pDev)
{
	FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    vxSpiCtrl_Wr_CfgReg32(pDev, SPIPS_SSIENR_OFFSET, 0x1);
    pDrvCtrl->CTRL.isEnable = TRUE;
    
    return;
}

#endif

#if 1

void vxSpiSlcr_Set_SpiClk(void)
{
	slcr_write(IO_PLL_CLKOUT3_DIVISOR, 0xF);  /* 15=> 30M *00 / 15 = 200M ref_clk_spi*/
	slcr_write(SPI_CLK_CTRL, 0xF);
	return;
}

/****************************************************************************/
/**
*
*  This function loop SPI0's outputs to SPI1's inputs,and SPI1's outputs to
*  SPI0's inputs
*
* @param
*  loop_en -- 0:connect SPI inputs according to MIO mapping;1:set the loop
*
* @return
*  Null
*
* @note
*  Null
*
****************************************************************************/
void vxSpiSlcr_Set_SpiLoop(UINT32 loop_en)
{
	UINT32 tmp32 = 0;
	
	if (loop_en == 0)
	{
	    /*FSlcrPs_setBitTo0(FPS_SLCR_BASEADDR, SLCR_MIO_LOOPBACK,0);*/
	    tmp32 = slcr_read(SLCR_MIO_LOOPBACK);
		tmp32 &= ~0x01;
		slcr_write(SLCR_MIO_LOOPBACK, tmp32);
		
	}
	else if (loop_en == 1)
	{
	    /*FSlcrPs_setBitTo1(FPS_SLCR_BASEADDR, SLCR_MIO_LOOPBACK,0);*/
		tmp32 = slcr_read(SLCR_MIO_LOOPBACK);
		tmp32 |= 0x01;
		slcr_write(SLCR_MIO_LOOPBACK, tmp32);
    }

	return;
}

#endif

#if 1
int vxSpiCtrl_Init_Master(VXB_DEVICE_ID pDev)
{
	FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    int ret = 0;
        
    /* 
    Check whether there is another transfer in progress. Not thread-safe 
	*/
    if (pDrvCtrl->CTRL.isBusy == TRUE) 
    {
        return ERROR;
    }
	else
	{
		pDrvCtrl->CTRL.isBusy = TRUE;
	}
    
    /* 
    Disable device 
	*/
    vxSpiCtrl_Disable(pDev);
	
    /* 
    Select device as Master 
	*/
    vxSpiCtrl_Set_Master(pDev);
	
    /* 
    CTRL (TMode, CkMode, BaudRate, DFSize, DFNum, isLoopBack) 
	*/
    ret |= vxSpiCtrl_Set_XfrMode(pDev, SPIPS_TRANSFER_STATE);
    ret |= vxSpiCtrl_Set_SckMode(pDev, SPI_MODE_3);          /* 3 =>  #define SPI_MODE_3  (SPI_CKPOL | SPI_CKPHA) */

    ret |= vxSpiCtrl_Set_Baud_Mst(pDev, 200);    /* Fsclk_out = Fssi_clk/SCKDV => 40_000_000 / 200 = 200K bps */

#if 0  /* 1 byte/frm*/
    ret |= vxSpiCtrl_Set_FrmLen(pDev, 8);       /* frame_len: 8bit-1bytes */
#else  /* 4 bytes/frm*/
    ret |= vxSpiCtrl_Set_FrmLen(pDev, 32);       /* frame_len: 32bit-4bytes */
#endif
	
    ret |= vxSpiCtrl_Set_LoopBack(pDev, FALSE);  /* normal work mode, no loop */
	
    ret |= vxSpiCtrl_Set_RcvFrmNum(pDev, 128);   /* rcv frame num */ 
    
    /* 
    Config Tx/Rx Threshold 
	*/
    ret |= vxSpiCtrl_Set_TxFiFoEmptyLvl(pDev, 8); /* ->24 */
    ret |= vxSpiCtrl_Set_RxFifoFullLvl(pDev, 0);  /* -> 8 */
	
    vxSpiCtrl_Set_DMATxLvl(pDev, 8);
    vxSpiCtrl_Set_DMARxLvl(pDev, 24);
    
    /* 
    Config IMR : disable all IRQ
	*/
    vxSpiCtrl_DisIRQ(pDev, SPIPS_INTR_ALL);
    
    /*
    SlaveSelect 
	*/
    ret |= vxSpiCtrl_Set_PeerSlave(pDev, SPI_CTRL_1);  /*  ???  */
    if (ret == ERROR)
    {
        return ERROR;
    }
	
    /* 
    Enable pDev
    */
    vxSpiCtrl_Enable(pDev);
    
	pDrvCtrl->CTRL.isBusy = FALSE;
	
    return OK;
}


int vxSpiCtrl_Init_Slave(VXB_DEVICE_ID pDev)
{
	FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    int ret = 0;

    /* 
    Check whether there is another transfer in progress. Not thread-safe 
	*/
    if (pDrvCtrl->CTRL.isBusy == TRUE) 
    {
        return ERROR;
    }
	else
	{
		pDrvCtrl->CTRL.isBusy = TRUE;
	}
	
    /* 
    Disable device 
	*/
    vxSpiCtrl_Disable(pDev);

	/* 
    Select device as Slave 
	*/
    vxSpiCtrl_Set_Slave(pDev);
	
    /* Config CTRLR0 (TMode, CkMode, DFSize, IsSlaveOut, isLoopBack) */
    ret |= vxSpiCtrl_Set_XfrMode(pDev, SPIPS_TRANSFER_STATE);
    ret |= vxSpiCtrl_Set_SckMode(pDev, SPI_MODE_3);          /* 3 =>  #define SPI_MODE_3  (SPI_CKPOL | SPI_CKPHA) */

#if 0  /* 1 byte/frm*/
		ret |= vxSpiCtrl_Set_FrmLen(pDev, 8); 	  /* frame_len: 8bit-1bytes */
#else  /* 4 bytes/frm*/
		ret |= vxSpiCtrl_Set_FrmLen(pDev, 32);	   /* frame_len: 32bit-4bytes */
#endif

    ret |= vxSpiCtrl_Set_LoopBack(pDev, FALSE);  /* normal work mode, no loop */

    ret |= vxSpiCtrl_Set_enSlvTxd(pDev, TRUE);    

    /* 
    Config Tx/Rx Threshold 
	*/
    ret |= vxSpiCtrl_Set_TxFiFoEmptyLvl(pDev, 8); /* ->24 */
    ret |= vxSpiCtrl_Set_RxFifoFullLvl(pDev, 0);  /* -> 8 */
	
    vxSpiCtrl_Set_DMATxLvl(pDev, 8);
    vxSpiCtrl_Set_DMARxLvl(pDev, 24);

	
    /* 
    Config IMR : disable IRQ
	*/
    vxSpiCtrl_DisIRQ(pDev, SPIPS_INTR_ALL);
        
    if (ret == ERROR)
    {
        return ERROR;
    }	
	
    /* 
    Enable pDev
    */
    vxSpiCtrl_Enable(pDev);
	
	pDrvCtrl->CTRL.isBusy = FALSE;

    return OK;
}


int vxSpiCtrl_SndFifo_Poll(VXB_DEVICE_ID pDev, UINT32 tx_data)
{
	FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    UINT32 count = 0;
    UINT8 status;
    
    status = vxSpiCtrl_Rd_CfgReg32(pDev, SPIPS_SR_OFFSET); 
	
    while ((status & SPIPS_SR_TFNF) == 0)   /* loop if TX fifo full */
    {
        sysUsDelay(1);
		
        count++;
        if(count > 10000)
        {
            break;
        }
        status = vxSpiCtrl_Rd_CfgReg32(pDev, SPIPS_SR_OFFSET); 
    }

    vxSpiCtrl_Wr_CfgReg32(pDev, SPIPS_DR_OFFSET, tx_data);  
	
	return FMSH_SUCCESS;
}

UINT32 vxSpiCtrl_RcvFifo_Poll(VXB_DEVICE_ID pDev)
{
	FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    UINT32 count = 0;
    UINT8 status;
    
    status = vxSpiCtrl_Rd_CfgReg32(pDev, SPIPS_SR_OFFSET); 
    while ((status & SPIPS_SR_RFNE) == 0)  /* loop if RX fifo empty */
    {
        sysUsDelay(1);
		
        count++;
        if(count > 10000)
        {
            break;
        }        
        status = vxSpiCtrl_Rd_CfgReg32(pDev, SPIPS_SR_OFFSET); 
    }      

    return vxSpiCtrl_Rd_CfgReg32(pDev, SPIPS_DR_OFFSET); 
}

/*
3.5.2.1. CTRLR0:
	bit11	SRL	R/W	环回逻辑，仅用于测试。
	内部激活时，将发送移位寄存器的输出连接至接收移位寄存器输入，可用于串行主从机
		0：正常模式操作
		1：测试模式操作
		当SSI配置为从机操作且工作在测试模式时，ss_in_n和ssi_clk信号由外部提供复位值：0x0
*/

int vxSpiCtrl_IP_Loop(VXB_DEVICE_ID pDev, UINT32 loop_tx_data)
{
	FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    int error = 0;
    UINT32 data;
    
    SPI_DBG1("\n -vxb- Self IP_Loop Test: \n", 1,2,3,4,5,6);
    
    vxSpiCtrl_Reset(pDev, pDrvCtrl->CTRL.ctrl_x);
	
    vxSpiCtrl_Init_Master(pDev);

    vxSpiCtrl_Disable(pDev);
    vxSpiCtrl_Set_LoopBack(pDev, TRUE);	
    vxSpiCtrl_Enable(pDev); 
    
    /* slave sends first, then master sends */
    vxSpiCtrl_SndFifo_Poll(pDev, loop_tx_data); /*0xAABBCCDD */
	SPI_DBG1("<ip_loop>spi_%d: snd: 0x%X \n", pDrvCtrl->CTRL.ctrl_x, loop_tx_data, 3,4,5,6);
	
    sysMsDelay(100);
	
    /*data = FMSH_ReadReg(pDev->config.baseAddress, SPIPS_DR_OFFSET);*/
    data = vxSpiCtrl_RcvFifo_Poll(pDev);	
    if (data != loop_tx_data) 
    {    
    	error = 0x1;
		SPI_DBG1("Err! <ip_loop>spi_%d: 0x%X(rx) != 0x%X(tx) \n", \
			     pDrvCtrl->CTRL.ctrl_x, data, loop_tx_data, 4,5,6);
    }
	else
	{
		SPI_DBG1("Ok! <ip_loop>spi_%d: 0x%X(rx) == 0x%X(tx) \n", \
			     pDrvCtrl->CTRL.ctrl_x, data, loop_tx_data, 4,5,6);
	}
	    
    /* 
    disable loop 
	*/
    vxSpiCtrl_Disable(pDev);
    vxSpiCtrl_Set_LoopBack(pDev, FALSE);
    vxSpiCtrl_Enable(pDev); 
    
    if (error)
    {
        SPI_DBG1("-vxb- IP_loop test Fail! \n\n", 1,2,3,4,5,6);
        return ERROR;
    }
    else
    {
        SPI_DBG1("-vxb- IP_loop test Pass! \n\n", 1,2,3,4,5,6);
        return OK;
    }

	return OK;
}

/*
MIO_LOOPBACK	0x804	reserved	
--------------------------------
                       31:4	rw	0x0	reserved
                       
		I2C0_LOOP_I2C1	3	rw	0x0	I2C Loopback Control.
						0 = Connect I2C inputs according to MIO mapping.
						1 = Loop I2C 0 outputs to I2C 1 inputs, and I2C 1 outputs
						
		CAN0_LOOP_CAN1	2	rw	0x0	CAN Loopback Control.
						0 = Connect CAN inputs according to MIO mapping.
						1 = Loop CAN 0 Tx to CAN 1 Rx, and CAN 1 Tx to CAN 0 Rx.
						
		UA0_LOOP_UA1	1	rw	0x0	UART Loopback Control.
						0 = Connect UART inputs according to MIO mapping.
						1 = Loop UART 0 outputs to UART 1 inputs, and UART 1 outputs to UART 0 inputs.
							RXD/TXD cross-connected. RTS/CTS cross-connected.
							DSR, DTR, DCD and RI not used.
						
		SPI0_LOOP_SPI1	0	rw	0x0	SPI Loopback Control.
						0 = Connect SPI inputs according to MIO mapping.
						1 = Loop SPI 0 outputs to SPI 1 inputs, and SPI 1 outputs to SPI 0 inputs.
							The other SPI core will appear on the LS Slave Select.
----------------------------------------------------------------------------------
MIO swap:
spi_0(master) --> spi_1(slave)
spi_1         --> spi_0
*/
int vxSpiCtrl_SLCR_Loop(VXB_DEVICE_ID pDev0, VXB_DEVICE_ID pDev1, UINT32 loop_tx_data)
{
	FM_SPI_DRV_CTRL * pDrvCtrl0 = (FM_SPI_DRV_CTRL*)pDev0->pDrvCtrl;
	FM_SPI_DRV_CTRL * pDrvCtrl1 = (FM_SPI_DRV_CTRL*)pDev1->pDrvCtrl;
	
    int error = 0;
    UINT32 rx_data0, rx_data1;
    
    SPI_DBG1("\n -vxb- SLCR_Loop Test \n", 1,2,3,4,5,6);
    
    /* 
    enable SLCR loop 
	*/
    vxSpiSlcr_Set_SpiLoop(TRUE);

    /* 
    slave sends first, then master sends 
	*/
#if 1 
	vxSpiCtrl_SndFifo_Poll(pDev1, loop_tx_data);	/* spi1-slave, spi0-master */
	printf("<slcr_loop>spi_%d: snd-0x%X (to mst) \n", pDrvCtrl1->CTRL.ctrl_x, loop_tx_data);
		
	sysMsDelay(10);	
	printf("<------>\n");	
	
	rx_data0 = vxSpiCtrl_RcvFifo_Poll(pDev0); 
	printf("<slcr_loop>spi_%d: rcv-0x%X (from slv) \n\n", pDrvCtrl0->CTRL.ctrl_x, rx_data0);
#endif

	printf("=========== \n");	

#if 1
    vxSpiCtrl_SndFifo_Poll(pDev0, loop_tx_data);  /* spi1-slave, spi0-master */
	printf("<slcr_loop>spi_%d: snd-0x%X (to slv) \n", pDrvCtrl0->CTRL.ctrl_x, loop_tx_data);
	
    sysMsDelay(10);	
	printf("<------>\n");
	
    rx_data1 = vxSpiCtrl_RcvFifo_Poll(pDev1); 
	printf("<slcr_loop>spi_%d: rcv-0x%X (from mst) \n\n", pDrvCtrl1->CTRL.ctrl_x, rx_data1);
#endif
	
    /* 
    disable slcr loop 
	*/
    vxSpiSlcr_Set_SpiLoop(FALSE);

	return;
}


int vxSpiCtrl_X_Loop(VXB_DEVICE_ID pDev0, VXB_DEVICE_ID pDev1, UINT32 loop_tx_data)
{
	FM_SPI_DRV_CTRL * pDrvCtrl0 = (FM_SPI_DRV_CTRL*)pDev0->pDrvCtrl;
	FM_SPI_DRV_CTRL * pDrvCtrl1 = (FM_SPI_DRV_CTRL*)pDev1->pDrvCtrl;
	
    int error = 0;
    UINT32 rx_data0, rx_data1;
    
    SPI_DBG1("\n -vxb- X_Loop Test \n", 1,2,3,4,5,6);
    
    /* 
    slave sends first, then master sends 
	*/
#if 1 
	vxSpiCtrl_SndFifo_Poll(pDev1, loop_tx_data);	/* spi1-slave, spi0-master */
	printf("<slcr_loop>spi_%d: snd-0x%X (to mst) \n", pDrvCtrl1->CTRL.ctrl_x, loop_tx_data);
		
	sysMsDelay(10);	
	printf("<------>\n");	
	
	rx_data0 = vxSpiCtrl_RcvFifo_Poll(pDev0); 
	printf("<slcr_loop>spi_%d: rcv-0x%X (from slv) \n\n", pDrvCtrl0->CTRL.ctrl_x, rx_data0);
#endif

#if 1
    vxSpiCtrl_SndFifo_Poll(pDev0, loop_tx_data);  /* spi1-slave, spi0-master */
	printf("<slcr_loop>spi_%d: snd-0x%X (to slv) \n", pDrvCtrl0->CTRL.ctrl_x, loop_tx_data);
	
    sysMsDelay(10);	
	printf("<------>\n");
	
    rx_data1 = vxSpiCtrl_RcvFifo_Poll(pDev1); 
	printf("<slcr_loop>spi_%d: rcv-0x%X (from mst) \n\n", pDrvCtrl1->CTRL.ctrl_x, rx_data1);
#endif
	
	return;
}

#endif

#if 0

/*******************************************************************************
*
* vxSpiCtrlGet - get the SPI controller struct
*
* This routine returns the SPI controller struct pointer (VXB_SPI_BUS_CTRL *)
* to caller (SPI Lib) by vxSpiControlGet method. Currently, this struct
* only contain the spiTransfer routine(eg: vxSpiTransfer) for SPI Lib used,
* other parameters can be easily added in this struct.
*
* RETURNS: the pointer of SPI controller struct
*
* ERRNO: N/A
*/

LOCAL VXB_SPI_BUS_CTRL * vxSpiCtrlGet
    (
    VXB_DEVICE_ID pDev
    )
{
    FM_SPI_DRV_CTRL * pDrvCtrl;

    /* check if the pDev pointer is valid */
    VXB_ASSERT (pDev != NULL, NULL)

    pDrvCtrl = (FM_SPI_DRV_CTRL *) pDev->pDrvCtrl;

    SPI_DBG(SPI_DBG_RTN, "vxSpiCtrlGet(0x08%x) called\n",
            (_Vx_usr_arg_t)pDev, 2, 3, 4, 5, 6);

    return (&(pDrvCtrl->vxSpiCtrl));
}

/*******************************************************************************
*
* vxSpiChanCfg - configure the SPI channel
*
* This routine configures the SPI channel register (MCSPI_CH(i)CONF) which
* controls SPI master operation with its parameters.
*
* RETURNS: OK if device was successfully configured, otherwise ERROR
*
* ERRNO: N/A
*/

 LOCAL STATUS vxSpiChanCfg
    (
    VXB_DEVICE_ID  pDev,     /* controller pDev */
    SPI_HARDWARE * pSpiDev   /* device info */
    )
{
    UINT32  regValue;
    UINT32  frqDiv;
    FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL *) pDev->pDrvCtrl;

    while (SPI_REG_READ_32(pDrvCtrl, SPIx_SR) & SR_BUSY);

    /* Diable SPI controller */
    SPI_REG_WRITE_32(pDrvCtrl, SPIx_SSIENR, 0);

    SPI_REG_WRITE_32(pDrvCtrl, SPIx_SER, spiDevCtlr->csNum);

    /* Configure SPI controller chip select */
    regValue = SPI_REG_READ_32(pDrvCtrl, SPIx_CTRLR0);
    regValue &= ~(TMOD_MASK | SPI_MODE_MASK | SPI_FRF_MASK | DFS_MASK);
    regValue |= spiDevCtlr->spiDev.ctrlr0;
    SPI_REG_WRITE_32(pDrvCtrl, SPIx_CTRLR0, regValue);

    /* Configure SPI clock frequency */
    frqDiv = pDrvCtrl->apbClkFreqGet() / spiDevCtlr->devFrq;
    if (frqDiv == 0)
        frqDiv = 1;
    SPI_REG_WRITE_32(pDrvCtrl, SPIx_BAUDR, frqDiv);
}

/*******************************************************************************
*
* vxSpiWaitBitSet - wait a register's specific bit is set
*
* The routine waits a register's specific bit is set
*
* RETURNS: OK or ERROR if timeout
*
* ERRNO: N/A
*/

LOCAL STATUS vxSpiWaitBitSet
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
        if  (ns ++ > 5 * SPI_TIMEOUT_US)
            {
            break;
            }
        }while (TRUE);

    return sts;
    }

/*******************************************************************************
*
* vxSpiChanShutdown - shutdown one SPI channel
*
* The routine performs shutdown operation of a specific SPI channel
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

 LOCAL void vxSpiChanShutdown
    (
    VXB_DEVICE_ID  pDev,       /* controller pDev */
    UINT32         chan        /* SPI channel */
    )
    {
    UINT32 spiConf;
    
    /* Disable SPI */

    CSR_CLRBIT_4 (pDev, MCSPI_CHCTRL(chan), ~MCSPI_CH0CTRL_EN);

    spiConf = CSR_READ_4 (pDev, MCSPI_CHCONF(chan));

    /* disable Tx/Rx FIFO */

    spiConf &= ~(MCSPI_CH0CONF_FFER | MCSPI_CH0CONF_FFEW);

    /* Transmit-Receive mode */

    spiConf &= ~MCSPI_CHOCONF_TRM_MASK;

    /* disable Tx/Rx DMA */

    spiConf &= ~(MCSPI_CH0CONF_DMAR | MCSPI_CH0CONF_DMAW);

    /* deactive cs */

    spiConf &= ~MCSPI_CH0CONF_FORCE;

    CSR_WRITE_4 (pDev, MCSPI_CHCONF(chan), spiConf);

    /* disable all SPI interrupts */

    CSR_WRITE_4 (pDev, MCSPI_IRQENABLE, 0);

    /* clear previous pending IRQ status */

    CSR_WRITE_4 (pDev, MCSPI_IRQSTATUS,
                 CSR_READ_4 (pDev, MCSPI_IRQENABLE));
    }


/*******************************************************************************
*
* vxSpiPollTrans - SPI transfer routine in poll mode
*
* The routine performs the transmission in poll mode.
*
* RETURNS: OK
*
* ERRNO: N/A
*/

LOCAL STATUS vxSpiPollTrans
    (
    VXB_DEVICE_ID  pDev,     /* controller pDev */
    SPI_HARDWARE * pSpiDev,  /* device info */
    SPI_TRANSFER * pPkg      /* transfer data info */
    )
    {
    FM_SPI_DRV_CTRL * pDrvCtrl;
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

    pDrvCtrl = (FM_SPI_DRV_CTRL *) pDev->pDrvCtrl;

    channel  = pSpiDev->devInfo->chipSelect;

    CSR_WRITE_4 (pDev, MCSPI_IRQENABLE, 0);

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

    spiConf = CSR_READ_4 (pDev, MCSPI_CHCONF(channel));

    spiConf |= (MCSPI_CH0CONF_FFEW | MCSPI_CH0CONF_FFER);
    spiConf &= ~(MCSPI_CH0CONF_DMAR | MCSPI_CH0CONF_DMAW);

    /* Transmit-Receive mode */

    spiConf &= ~(MCSPI_TRM_TX | MCSPI_TRM_RX);

    isFullDuplex = IS_SPI_FULL_DUPLEX(pSpiDev->devInfo->mode);
    skipRxCnt = isFullDuplex ? 0 : pPkg->txLen;
    len = pDrvCtrl->txLeft > pDrvCtrl->rxLeft ? pDrvCtrl->txLeft : 
                                                pDrvCtrl->rxLeft;

    /* set the transfer levels register */

    if (len > SPI_FIFO_LEN)
        {
        CSR_WRITE_4 (pDev, MCSPI_XFERLEVEL,
                     (SPI_FIFO_LEN - 1) << MCSPI_XFERLEVEL_AFL_SHIFT);
        }
    else
        {
        CSR_WRITE_4 (pDev, MCSPI_XFERLEVEL,
                    (len - 1) << MCSPI_XFERLEVEL_AFL_SHIFT);
        }

    spiConf |= MCSPI_CH0CONF_FORCE;        /* forece cs */

    CSR_WRITE_4 (pDev, MCSPI_CHCONF(channel), spiConf);

    /* Enable SPI */

    CSR_SETBIT_4 (pDev, MCSPI_CHCTRL(channel), MCSPI_CH0CTRL_EN);

    /* Clear the pending interrupts */

    CSR_WRITE_4 (pDev, MCSPI_IRQSTATUS, CSR_READ_4 (pDev, MCSPI_IRQSTATUS));

    for (i = 0; i < SPI_FIFO_LEN && i < len; i += alignSize)
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
    
            CSR_WRITE_4 (pDev, MCSPI_TX(channel), data);
    
            SPI_DBG (SPI_DBG_RW, "Tx: 0x%x\n",
                            data, 2, 3, 4, 5, 6);

            pDrvCtrl->txLeft -= alignSize;
            }
        else
            {
            CSR_WRITE_4 (pDev, MCSPI_TX(channel), 0);  /* write dummy */
            }
        }

    do
    {
  
    /* Read SPI Interrupt Status Register */

    intSts = CSR_READ_4 (pDev, MCSPI_IRQSTATUS);

    CSR_WRITE_4 (pDev, MCSPI_IRQSTATUS, intSts);

    /* Handle Rx */

    if (IS_RX_INT (channel, intSts))
        {

        /* Rx */

        for (i = 0; i < SPI_FIFO_LEN && i < len; i += alignSize)
            {
            if (skipRxCnt > 0)
                {
                CSR_READ_4 (pDev, MCSPI_RX(channel));
                skipRxCnt -= alignSize;
                }
            else if (pDrvCtrl->rxLeft > 0)
                {
                data =  CSR_READ_4 (pDev, MCSPI_RX(channel));
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
    
                SPI_DBG (SPI_DBG_RW, "Rx: 0x%x\n", data, 2, 3, 4, 5, 6);
                pDrvCtrl->rxLeft -= alignSize;
                }
            else
                {
                CSR_READ_4 (pDev, MCSPI_RX(channel));  /* read dummy */
                }
            }

        len = pDrvCtrl->txLeft > pDrvCtrl->rxLeft ? 
                                            pDrvCtrl->txLeft : pDrvCtrl->rxLeft;

        if (len > SPI_FIFO_LEN)
            {
            CSR_WRITE_4 (pDev, MCSPI_XFERLEVEL,
                         (SPI_FIFO_LEN - 1) << MCSPI_XFERLEVEL_AFL_SHIFT);
            }
        else
            {
            CSR_WRITE_4 (pDev, MCSPI_XFERLEVEL,
                         (len - 1) << MCSPI_XFERLEVEL_AFL_SHIFT);
            }

        /* Tx */

        for (i = 0; i < SPI_FIFO_LEN && i < len; i += alignSize)
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
        
                CSR_WRITE_4 (pDev, MCSPI_TX(channel), data);
        
                SPI_DBG (SPI_DBG_RW, "Tx: 0x%x\n",
                                data, 2, 3, 4, 5, 6);
    
                pDrvCtrl->txLeft -= alignSize;;
                }
            else
                {
                CSR_WRITE_4 (pDev, MCSPI_TX(channel), 0);  /* write dummy */
                }
            }

        if (skipRxCnt ==0 && (pDrvCtrl->txLeft == 0) && (pDrvCtrl->rxLeft == 0))
            {

            sts = vxSpiWaitBitSet (pDev, MCSPI_CHSTAT(channel), 
                                       MCSPI_CH0STAT_TXFFE);
            break;
            }
        }

    }while (TRUE);

    vxSpiChanShutdown (pDev, channel);

    return sts;
    }


/*******************************************************************************
*
* vxSpiIntTrans - start SPI transfer in interrupt mode
*
* The routine starts a transmission in interrupt mode
*
* RETURNS: OK
*
* ERRNO: N/A
*/

LOCAL STATUS vxSpiIntTrans
    (
    VXB_DEVICE_ID  pDev,     /* controller pDev */
    SPI_HARDWARE * pSpiDev,  /* device info */
    SPI_TRANSFER * pPkg      /* transfer data info */
    )
    {
    FM_SPI_DRV_CTRL * pDrvCtrl;
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

    pDrvCtrl = (FM_SPI_DRV_CTRL *) pDev->pDrvCtrl;

    channel  = pSpiDev->devInfo->chipSelect;

    CSR_WRITE_4 (pDev, MCSPI_IRQENABLE, 0);

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

    spiConf = CSR_READ_4 (pDev, MCSPI_CHCONF(channel));

    spiConf |= (MCSPI_CH0CONF_FFEW | MCSPI_CH0CONF_FFER);
    spiConf &= ~(MCSPI_CH0CONF_DMAR | MCSPI_CH0CONF_DMAW);

    /* Transmit-Receive mode */

    spiConf &= ~(MCSPI_TRM_TX | MCSPI_TRM_RX);

    isFullDuplex = IS_SPI_FULL_DUPLEX(pSpiDev->devInfo->mode);
    skipRxCnt = isFullDuplex ? 0 : pPkg->txLen;
    len = pDrvCtrl->txLeft > pDrvCtrl->rxLeft ? pDrvCtrl->txLeft : 
                                                               pDrvCtrl->rxLeft;
    if (len > SPI_FIFO_LEN)
        {
        CSR_WRITE_4 (pDev, MCSPI_XFERLEVEL,
                     (SPI_FIFO_LEN - 1) << MCSPI_XFERLEVEL_AFL_SHIFT);
        }
    else
        {
        CSR_WRITE_4 (pDev, MCSPI_XFERLEVEL,
                     (len - 1) << MCSPI_XFERLEVEL_AFL_SHIFT);
        }

    spiConf |= MCSPI_CH0CONF_FORCE;        /* forece cs */

    CSR_WRITE_4 (pDev, MCSPI_CHCONF(channel), spiConf);

    /* Enable SPI */

    CSR_SETBIT_4 (pDev, MCSPI_CHCTRL(channel), MCSPI_CH0CTRL_EN);

    /* Clear the pending interrupts */

    CSR_WRITE_4 (pDev, MCSPI_IRQSTATUS, CSR_READ_4 (pDev, MCSPI_IRQSTATUS));

    /* enable Rx interrupt only. Tx in Rx interrupt */

    CSR_SETBIT_4 (pDev, MCSPI_IRQENABLE, MCSPI_IRQENABLE_RX(channel));

    for (i = 0; i < SPI_FIFO_LEN && i < len; i += alignSize)
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
    
            CSR_WRITE_4 (pDev, MCSPI_TX(channel), data);
    
            SPI_DBG (SPI_DBG_RW, "Tx: 0x%x\n", data, 2, 3, 4, 5, 6);

            pDrvCtrl->txLeft -= alignSize;
            }
        else
            {
            CSR_WRITE_4 (pDev, MCSPI_TX(channel), 0);  /* write dummy */
            }
        }
    do
    {
    timeoutTick = (vxbSysClkRateGet () * 8 * SPI_FIFO_LEN) / 
                  pDrvCtrl->curWorkingFrq;
    timeoutTick = 5 * timeoutTick + 10;

    (void)semTake (pDrvCtrl->semSync, timeoutTick);

    /* Read SPI Interrupt Status Register */

    intSts = CSR_READ_4 (pDev, MCSPI_IRQSTATUS);

    SPI_DBG (SPI_DBG_RW, "vxSpiIsr: intSts 0x%x\n", intSts, 2, 3, 4, 5, 6);

    if (intSts == 0)  /* fake interrupt or timeout */
        {
        SPI_DBG (SPI_DBG_ERR, "vxSpiIsr: fake interrupt or timeout\n", 
                 1, 2, 3, 4, 5, 6);

        sts = ERROR;
        goto intDone;
        }

    CSR_WRITE_4 (pDev, MCSPI_IRQSTATUS, intSts);

    /* Handle Rx */

    if (IS_RX_INT (channel, intSts))
        {

        /* Rx */

        for (i = 0; i < SPI_FIFO_LEN && i < len; i += alignSize)
            {
            if (skipRxCnt > 0)
                {
                CSR_READ_4 (pDev, MCSPI_RX(channel));
                skipRxCnt -= alignSize;
                }
            else if (pDrvCtrl->rxLeft > 0)
                {
                data =  CSR_READ_4 (pDev, MCSPI_RX(channel));
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
    
                SPI_DBG (SPI_DBG_RW, "Rx: 0x%x\n", data, 2, 3, 4, 5, 6);
                pDrvCtrl->rxLeft -= alignSize;
                }
            else
                {
                CSR_READ_4 (pDev, MCSPI_RX(channel));  /* read dummy */
                }
            }

        len = pDrvCtrl->txLeft > pDrvCtrl->rxLeft ? pDrvCtrl->txLeft : 
                                                               pDrvCtrl->rxLeft;

        if (len > SPI_FIFO_LEN)
            {
            CSR_WRITE_4 (pDev, MCSPI_XFERLEVEL,
                         (SPI_FIFO_LEN - 1) << MCSPI_XFERLEVEL_AFL_SHIFT);
            }
        else
            {
            CSR_WRITE_4 (pDev, MCSPI_XFERLEVEL,
                         (len - 1) << MCSPI_XFERLEVEL_AFL_SHIFT);
            }

        /* Tx */

        for (i = 0; i < SPI_FIFO_LEN && i < len; i += alignSize)
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
        
                CSR_WRITE_4 (pDev, MCSPI_TX(channel), data);
        
                SPI_DBG (SPI_DBG_RW, "Tx: 0x%x\n",
                                data, 2, 3, 4, 5, 6);
    
                pDrvCtrl->txLeft -= alignSize;;
                }
            else
                {
                CSR_WRITE_4 (pDev, MCSPI_TX(channel), 0);  /* write dummy */
                }
            }

        if ((skipRxCnt == 0) && (pDrvCtrl->txLeft == 0) && 
            (pDrvCtrl->rxLeft == 0))
            {
            sts = vxSpiWaitBitSet (pDev, MCSPI_CHSTAT(channel), 
                                       MCSPI_CH0STAT_TXFFE);
            break;
            }

        /* enable Rx interrupt only. Tx in Rx interrupt */

        CSR_SETBIT_4 (pDev, MCSPI_IRQENABLE, MCSPI_IRQENABLE_RX(channel));
        }

    }while (TRUE);

intDone:

    vxSpiChanShutdown (pDev, channel);

    return sts;
    }

/*******************************************************************************
*
* vxSpiDmaParaPrep - prepare DMA parameter
*
* The routine prepares DMA parameter
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxSpiDmaParaPrep
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
        SPI_DBG (SPI_DBG_ERR, "vxSpiDmaParaPrep: invalid parameter \n", 
                 1, 2, 3, 4, 5, 6);
        }
    }

/*******************************************************************************
*
* vxSpiDmaTrans - start SPI transfer in DMA mode
*
* The routine starts a transmission in DMA mode
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxSpiDmaTrans
    (
    VXB_DEVICE_ID  pDev,     /* controller pDev */
    SPI_HARDWARE * pSpiDev,  /* device info */
    SPI_TRANSFER * pPkg       /* transfer data info */
    )
    {
    FM_SPI_DRV_CTRL * pDrvCtrl;
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

    pDrvCtrl = (FM_SPI_DRV_CTRL *) pDev->pDrvCtrl;
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

    spiConf = CSR_READ_4 (pDev, MCSPI_CHCONF(channel));
    spiConf &= ~(MCSPI_TRM_TX | MCSPI_TRM_RX);
    spiConf &= ~(MCSPI_CH0CONF_DMAR | MCSPI_CH0CONF_DMAW);
    spiConf &= ~(MCSPI_CH0CONF_FFEW | MCSPI_CH0CONF_FFER);
    spiConf |= MCSPI_CH0CONF_FORCE;        /* force cs */
    CSR_WRITE_4 (pDev, MCSPI_CHCONF(channel), spiConf);

    /* Enable SPI */

    CSR_SETBIT_4 (pDev, MCSPI_CHCTRL(channel), MCSPI_CH0CTRL_EN);

    /* Clear the pending interrupts */

    CSR_WRITE_4 (pDev, MCSPI_IRQSTATUS, CSR_READ_4 (pDev, MCSPI_IRQSTATUS));

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

            vxSpiDmaParaPrep (pDrvCtrl->pTxChanParam[channel], pData, 
                        (UINT8*)((UINT32)pDrvCtrl->regBase + MCSPI_TX(channel)),
                        len, alignSize, FALSE, TRUE);
            vxbDmaWrite (pDrvCtrl->dmaTxRes[channel],
                         0, 0, 0, 0, 0,
                         (pVXB_DMA_COMPLETE_FN)vxSpiDmaCallback,
                         0);

            /* Rx dummy */

            pData = (UINT8*)&dummyData;

            vxSpiDmaParaPrep (pDrvCtrl->pRxChanParam[channel],
                        (UINT8*)((UINT32)pDrvCtrl->regBase + MCSPI_RX(channel)),
                        pData, len, alignSize, TRUE, TRUE);
            vxbDmaRead (pDrvCtrl->dmaRxRes[channel],
                        0, 0, 0, 0, 0,
                        (pVXB_DMA_COMPLETE_FN)vxSpiDmaCallback,
                        (void *)pDrvCtrl->semSync);

            timeoutTick = (vxbSysClkRateGet () * 8 * len) / 
                      pDrvCtrl->curWorkingFrq;
            timeoutTick = 5 * timeoutTick + 100;

            /* enable DMA request */

            CSR_SETBIT_4 (pDev, MCSPI_CHCONF(channel), 
                          (MCSPI_CH0CONF_DMAR | MCSPI_CH0CONF_DMAW));

            if (semTake (pDrvCtrl->semSync, timeoutTick) != OK)
                {
                sts = ERROR;
                goto dmaDone;
                }

            /* disable DMA request */

            CSR_CLRBIT_4 (pDev, MCSPI_CHCONF(channel), 
                          ~(MCSPI_CH0CONF_DMAR | MCSPI_CH0CONF_DMAW));
            
            pDrvCtrl->txLeft -= len;
           
            }
        else if (pDrvCtrl->rxLeft != 0)
            {
            /* Tx dummy */

            len = pDrvCtrl->rxLeft > maxTranLen ? maxTranLen : pDrvCtrl->rxLeft;
            pData = (UINT8*)&dummyData;
            vxSpiDmaParaPrep (pDrvCtrl->pTxChanParam[channel], pData, 
                        (UINT8*)((UINT32)pDrvCtrl->regBase + MCSPI_TX(channel)),
                        len, alignSize, TRUE, TRUE);
            vxbDmaWrite (pDrvCtrl->dmaTxRes[channel],
                         0, 0, 0, 0, 0,
                         (pVXB_DMA_COMPLETE_FN)vxSpiDmaCallback,
                         (void *)pDrvCtrl->semSync);

            /* Rx */

            pData = pPkg->rxBuf + (pDrvCtrl->rxLen - pDrvCtrl->rxLeft);;

            vxSpiDmaParaPrep (pDrvCtrl->pRxChanParam[channel],
                        (UINT8*)((UINT32)pDrvCtrl->regBase + MCSPI_RX(channel)),
                        pData, len, alignSize, TRUE, FALSE);

            vxbDmaRead (pDrvCtrl->dmaRxRes[channel],
                        0, 0, 0, 0, 0,
                        (pVXB_DMA_COMPLETE_FN)vxSpiDmaCallback,
                        (void *)pDrvCtrl->semSync);

            /* enable DMA request */

            CSR_SETBIT_4 (pDev, MCSPI_CHCONF(channel), 
                          (MCSPI_CH0CONF_DMAR | MCSPI_CH0CONF_DMAW));

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

            CSR_CLRBIT_4 (pDev, MCSPI_CHCONF(channel), 
                          ~(MCSPI_CH0CONF_DMAR | MCSPI_CH0CONF_DMAW));

            pDrvCtrl->rxLeft -= len;
            }
        }

dmaDone:

    vxSpiChanShutdown (pDev, channel);

    SPI_DBG (SPI_DBG_RTN, "vxSpiDmaTrans: done Tx/Rx \n", 1, 2, 3, 4, 5, 6);

    return sts;
    }

/*******************************************************************************
*
* vxSpiDmaTransFullDuplex - start SPI transfer in DMA mode for full duplex
*
* The routine starts a transmission in DMA mode for full duplex device driver
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxSpiDmaTransFullDuplex
    (
    VXB_DEVICE_ID  pDev,     /* controller pDev */
    SPI_HARDWARE * pSpiDev,  /* device info */
    SPI_TRANSFER * pPkg       /* transfer data info */
    )
    {
    FM_SPI_DRV_CTRL * pDrvCtrl;
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

    pDrvCtrl = (FM_SPI_DRV_CTRL *) pDev->pDrvCtrl;
    if (pDrvCtrl->txLen == 0 || pDrvCtrl->rxLen == 0)
        {
        return vxSpiDmaTrans (pDev, pSpiDev, pPkg);
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

    spiConf = CSR_READ_4 (pDev, MCSPI_CHCONF(channel));
    spiConf &= ~(MCSPI_TRM_TX | MCSPI_TRM_RX);
    spiConf &= ~(MCSPI_CH0CONF_DMAR | MCSPI_CH0CONF_DMAW);
    spiConf &= ~(MCSPI_CH0CONF_FFEW | MCSPI_CH0CONF_FFER);
    spiConf |= MCSPI_CH0CONF_FORCE;        /* force cs */
    CSR_WRITE_4 (pDev, MCSPI_CHCONF(channel), spiConf);

    /* Enable SPI */

    CSR_SETBIT_4 (pDev, MCSPI_CHCTRL(channel), MCSPI_CH0CTRL_EN);

    /* Clear the pending interrupts */

    CSR_WRITE_4 (pDev, MCSPI_IRQSTATUS, CSR_READ_4 (pDev, MCSPI_IRQSTATUS));

    CACHE_USER_FLUSH (pDrvCtrl->txBuf, pDrvCtrl->txLen);
    CACHE_USER_FLUSH (pDrvCtrl->rxBuf, pDrvCtrl->rxLen);
    minLen = pDrvCtrl->txLeft > pDrvCtrl->rxLeft ? pDrvCtrl->rxLeft :
                                                               pDrvCtrl->txLeft;
    while(minLen > 0)
        {
        len = minLen > maxTranLen ? maxTranLen : minLen;

        /* Tx */

        pData = pPkg->txBuf + (pDrvCtrl->txLen - pDrvCtrl->txLeft);
        vxSpiDmaParaPrep (pDrvCtrl->pTxChanParam[channel], pData, 
                        (UINT8*)((UINT32)pDrvCtrl->regBase + MCSPI_TX(channel)),
                        len, alignSize, FALSE, TRUE);
        vxbDmaWrite (pDrvCtrl->dmaTxRes[channel],
                     0, 0, 0, 0, 0,
                     (pVXB_DMA_COMPLETE_FN)vxSpiDmaCallback,
                     0);

        /* Rx */

        pData = pPkg->rxBuf + (pDrvCtrl->rxLen - pDrvCtrl->rxLeft);;
        vxSpiDmaParaPrep (pDrvCtrl->pRxChanParam[channel],
                        (UINT8*)((UINT32)pDrvCtrl->regBase + MCSPI_RX(channel)),
                        pData, len, alignSize, TRUE, FALSE);

        vxbDmaRead (pDrvCtrl->dmaRxRes[channel],
                    0, 0, 0, 0, 0,
                    (pVXB_DMA_COMPLETE_FN)vxSpiDmaCallback,
                    (void *)pDrvCtrl->semSync);
    
        /* enable DMA request */
    
        CSR_SETBIT_4 (pDev, MCSPI_CHCONF(channel), 
                      (MCSPI_CH0CONF_DMAR | MCSPI_CH0CONF_DMAW));
    
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
    
        CSR_CLRBIT_4 (pDev, MCSPI_CHCONF(channel), 
                      ~(MCSPI_CH0CONF_DMAR | MCSPI_CH0CONF_DMAW));
    
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

            vxSpiDmaParaPrep (pDrvCtrl->pTxChanParam[channel], pData, 
                        (UINT8*)((UINT32)pDrvCtrl->regBase + MCSPI_TX(channel)),
                        len, alignSize, FALSE, TRUE);
            vxbDmaWrite (pDrvCtrl->dmaTxRes[channel],
                         0, 0, 0, 0, 0,
                         (pVXB_DMA_COMPLETE_FN)vxSpiDmaCallback,
                         0);

            /* Rx dummy */

            pData = (UINT8*)&dummyData;

            vxSpiDmaParaPrep (pDrvCtrl->pRxChanParam[channel],
                        (UINT8*)((UINT32)pDrvCtrl->regBase + MCSPI_RX(channel)),
                        pData, len, alignSize, TRUE, TRUE);
            vxbDmaRead (pDrvCtrl->dmaRxRes[channel],
                        0, 0, 0, 0, 0,
                        (pVXB_DMA_COMPLETE_FN)vxSpiDmaCallback,
                        (void *)pDrvCtrl->semSync);

            timeoutTick = (vxbSysClkRateGet () * 8 * len) / 
                      pDrvCtrl->curWorkingFrq;
            timeoutTick = 5 * timeoutTick + 100;

            /* enable DMA request */

            CSR_SETBIT_4 (pDev, MCSPI_CHCONF(channel), 
                          (MCSPI_CH0CONF_DMAR | MCSPI_CH0CONF_DMAW));

            if (semTake (pDrvCtrl->semSync, timeoutTick) != OK)
                {
                sts = ERROR;
                goto dmafullDone;
                }

            /* disable DMA request */

            CSR_CLRBIT_4 (pDev, MCSPI_CHCONF(channel), 
                          ~(MCSPI_CH0CONF_DMAR | MCSPI_CH0CONF_DMAW));
            
            pDrvCtrl->txLeft -= len;
           
            }
        else if (pDrvCtrl->rxLeft != 0)
            {
            /* Tx dummy */

            len = pDrvCtrl->rxLeft > maxTranLen ? maxTranLen : pDrvCtrl->rxLeft;
            pData = (UINT8*)&dummyData;
            vxSpiDmaParaPrep (pDrvCtrl->pTxChanParam[channel], pData, 
                        (UINT8*)((UINT32)pDrvCtrl->regBase + MCSPI_TX(channel)),
                        len, alignSize, TRUE, TRUE);
            vxbDmaWrite (pDrvCtrl->dmaTxRes[channel],
                         0, 0, 0, 0, 0,
                         (pVXB_DMA_COMPLETE_FN)vxSpiDmaCallback,
                         (void *)pDrvCtrl->semSync);

            /* Rx */

            pData = pPkg->rxBuf + (pDrvCtrl->rxLen - pDrvCtrl->rxLeft);;

            vxSpiDmaParaPrep (pDrvCtrl->pRxChanParam[channel],
                        (UINT8*)((UINT32)pDrvCtrl->regBase + MCSPI_RX(channel)),
                        pData, len, alignSize, TRUE, FALSE);

            vxbDmaRead (pDrvCtrl->dmaRxRes[channel],
                        0, 0, 0, 0, 0,
                        (pVXB_DMA_COMPLETE_FN)vxSpiDmaCallback,
                        (void *)pDrvCtrl->semSync);

            /* enable DMA request */

            CSR_SETBIT_4 (pDev, MCSPI_CHCONF(channel), 
                          (MCSPI_CH0CONF_DMAR | MCSPI_CH0CONF_DMAW));

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

            CSR_CLRBIT_4 (pDev, MCSPI_CHCONF(channel), 
                          ~(MCSPI_CH0CONF_DMAR | MCSPI_CH0CONF_DMAW));

            pDrvCtrl->rxLeft -= len;
            }
        }

dmafullDone:

    vxSpiChanShutdown (pDev, channel);

    SPI_DBG (SPI_DBG_RTN, "vxSpiDmaTrans: done Tx/Rx \n", 1, 2, 3, 4, 5, 6);

    return OK;
    }

/*******************************************************************************
*
* vxSpiTransfer - SPI transfer routine
*
* This routine is used to perform one transmission. It is the interface which 
* can be called by SPI device driver to send and receive data via the SPI 
* controller.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxSpiTransfer
    (
    VXB_DEVICE_ID  pDev,     /* controller pDev */
    SPI_HARDWARE * pSpiDev,  /* device info */
    SPI_TRANSFER * pPkg       /* transfer data info */
    )
    {
    STATUS sts = OK;
    FM_SPI_DRV_CTRL * pDrvCtrl;
    UINT32 channel;
    UINT32 alignSize;

    /* Check if the pointers are valid */

    if (pDev == NULL || pSpiDev == NULL || pPkg == NULL ||
	pSpiDev->devInfo == NULL)
        {
        SPI_DBG (SPI_DBG_ERR, "vxSpiTransfer NULL pointer \n",
                 1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    pDrvCtrl = (FM_SPI_DRV_CTRL *) pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        {
        SPI_DBG (SPI_DBG_ERR, "vxSpiTransfer pDrvCtrl is NULL \n",
                 1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    SPI_DBG (SPI_DBG_RTN, "vxSpiTransfer  txLen[%d] rxLen[%d] \n",
             pPkg->txLen, pPkg->rxLen, 3, 4, 5, 6);

    if (pPkg->txLen == 0 && pPkg->rxLen == 0)
        {
        SPI_DBG (SPI_DBG_ERR, "vxSpiTransfer tx/rx are 0 \n",
                 1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    if ((pPkg->txLen != 0 && pPkg->txBuf == NULL ) ||
        (pPkg->rxLen != 0 && pPkg->rxBuf == NULL))
        {
        SPI_DBG (SPI_DBG_ERR,
                 "vxSpiTransfer invalid parameters[%x %x %x %x] \n",
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
        SPI_DBG (SPI_DBG_ERR,
                 "vxSpiTransfer address or len is not aligned:"
                 "[tx:%x-%x rx:%x-%x] with %d \n",
                 pPkg->txBuf, pPkg->txLen, pPkg->rxBuf,pPkg->rxLen, 
                 alignSize, 6);

        return ERROR;
        }

    channel = pSpiDev->devInfo->chipSelect;
    if (channel >= SPI_MAX_CS_NUM)
        {
        SPI_DBG (SPI_DBG_ERR,
                 "vxSpiTransfer invalid channel[%x] \n",
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

    
    if (vxSpiChanCfg(pDev, pSpiDev) != OK)
        {
        SPI_DBG (SPI_DBG_ERR, "vxSpiChanCfg failed.\n",
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
        sts = vxSpiPollTrans (pDev, pSpiDev, pPkg);
        }
    else
        {
        if (pDrvCtrl->dmaMode && vxSpiDmaChanAlloc (pDev, channel) == OK)
            {
            if (IS_SPI_FULL_DUPLEX(pSpiDev->devInfo->mode))
                {
                sts = vxSpiDmaTransFullDuplex(pDev, pSpiDev, pPkg);
                }
            else
                {
                sts = vxSpiDmaTrans (pDev, pSpiDev, pPkg);
                }
            }
        else
            {
            sts = vxSpiIntTrans (pDev, pSpiDev, pPkg);
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
* vxSpiDmaCallback - callback routine when DMA transfer is done 
*
* This routine is called when DMA transfer is done. The routine runs
* in interrupt context
*
* RETURNS: N/A 
*
* ERRNO: N/A
*/

LOCAL void vxSpiDmaCallback
    (
    SEM_ID      semId
    )
    {
    SPI_DBG(SPI_DBG_ISR, "DMA channel callback function\n", 1, 2, 3, 4, 5, 6);

    if (semId != 0)
        {
        semGive (semId);
        }
    }

/*******************************************************************************
*
* vxSpiDmaChanAlloc - allocate DMA resource 
*
* This routine allocates DMA resource 
*
* RETURNS: N/A 
*
* ERRNO: N/A
*/

LOCAL STATUS vxSpiDmaChanAlloc
    (
    VXB_DEVICE_ID pDev,
    UINT32        channel
    )
    {
    VXB_DEVICE_ID pInst;
    FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL *)pDev->pDrvCtrl;

    if (pDrvCtrl->dmaTxRes[channel] != NULL &&
        pDrvCtrl->dmaRxRes[channel] != NULL)
        {
        SPI_DBG (SPI_DBG_RTN,
                 "dma channels already allocated\n",
                 1, 2, 3, 4, 5, 6);
        return OK;
        }

    /* find EDMA3 controller instance */

    pInst = vxbInstByNameFind ("tiedma3", 0);
    if (pInst == NULL)
        {
        SPI_DBG(SPI_DBG_ERR,
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
        SPI_DBG (SPI_DBG_ERR,
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
        SPI_DBG (SPI_DBG_ERR,
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
* vxSpiDmaChanFree - free DMA resource 
*
* This routine frees DMA resource 
*
* RETURNS: N/A 
*
* ERRNO: N/A
*/

LOCAL void vxSpiDmaChanFree
    (
    VXB_DEVICE_ID pDev
    )
    {   
    UINT32 i;
    FM_SPI_DRV_CTRL * pDrvCtrl = (FM_SPI_DRV_CTRL *)pDev->pDrvCtrl;

    for (i = 0; i < SPI_MAX_CS_NUM; i++)
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
* vxSpiShow - show the controller info
*
* This function shows the SPI controller's info.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxSpiShow
    (
    VXB_DEVICE_ID pDev,
    int verbose
    )
    {
    FM_SPI_DRV_CTRL * pDrvCtrl;

    /* check for valid parameter */

    VXB_ASSERT_NONNULL_V (pDev);

    pDrvCtrl = (FM_SPI_DRV_CTRL *) pDev->pDrvCtrl;

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

int g_test_spi3 = 0;

/*
test IP_Loop
*/

STATUS test_spi_loop1_vx(int ctrl_x, UINT32 tx_data)
{
    VXB_DEVICE_ID       pDev;
    FM_SPI_DRV_CTRL * pDrvCtrl;
	
	int unit = 0;	
	UINT32 tx_loop_data = 0;
	
	switch (ctrl_x)
	{
	case SPI_CTRL_0:
		unit = 0;
		break;
	
	case SPI_CTRL_1:
		unit = 1;
		break;		
	}
	
    pDev = vxbInstByNameFind (FM_SPI_NAME, unit);
    if (pDev == NULL)
    {
        printf ("\nCan not find this spi unit(%d)!  \n\n", unit);
        return ERROR;
    }
	else
	{
		pDrvCtrl= (FM_SPI_DRV_CTRL *)pDev->pDrvCtrl;
		printf ("\nvxbInstByNameFind(%s): 0x%X \n\n", FM_SPI_NAME, pDev);
	}
	
	if (tx_data == 0)
	{
		tx_loop_data = g_test_spi3;
	}
	else
	{
		tx_loop_data = tx_data + g_test_spi3;
	}
	
	g_test_spi3++;
	
	switch (ctrl_x)
	{
	case SPI_CTRL_0:
		vxSpiCtrl_IP_Loop(pDev, tx_loop_data);
		break;
	
	case SPI_CTRL_1:
		vxSpiCtrl_IP_Loop(pDev, tx_loop_data);
		break;
	}

	return OK;
}

/*
test SLCR_Loop
*/

STATUS test_spi_loop2_vx(UINT32 tx_data)
{
    VXB_DEVICE_ID       pDev0;
    FM_SPI_DRV_CTRL * pDrvCtrl0;
	
    VXB_DEVICE_ID       pDev1;
    FM_SPI_DRV_CTRL * pDrvCtrl1;
	
	int unit = 0;	
	UINT32 tx_loop_data = 0;
	
	/*==================================================*/
	unit = 0;
	pDev0 = vxbInstByNameFind (FM_SPI_NAME, unit);
	if (pDev0 == NULL)
	{
		printf ("\nCan not find this spi unit(%d)!	\n\n", unit);
		return ERROR;
	}
	else
	{
		pDrvCtrl0 = (FM_SPI_DRV_CTRL *)pDev0->pDrvCtrl;
		/*printf ("\nvxbInstByNameFind(%s): 0x%X \n\n", FM_SPI_NAME, pDev0);*/
	}
	/*==================================================*/

	/*==================================================*/
	unit = 1;
	pDev1 = vxbInstByNameFind (FM_SPI_NAME, unit);
	if (pDev1 == NULL)
	{
		printf ("\nCan not find this spi unit(%d)!	\n\n", unit);
		return ERROR;
	}
	else
	{
		pDrvCtrl1 = (FM_SPI_DRV_CTRL *)pDev1->pDrvCtrl;
		/*printf ("\nvxbInstByNameFind(%s): 0x%X \n\n", FM_SPI_NAME, pDev1);*/
	}
	/*==================================================*/
	
	
	if (tx_data == 0)
	{
		tx_loop_data = g_test_spi3;
	}
	else
	{
		tx_loop_data = tx_data + g_test_spi3;
	}
	g_test_spi3++;		
	
	vxSpiCtrl_SLCR_Loop(pDev0, pDev1,tx_loop_data);

	return OK;
}


/*
test X_Loop
*/
STATUS test_spi_loop3_vx(UINT32 tx_data)
{
    VXB_DEVICE_ID       pDev0;
    FM_SPI_DRV_CTRL * pDrvCtrl0;
	
    VXB_DEVICE_ID       pDev1;
    FM_SPI_DRV_CTRL * pDrvCtrl1;
	
	int unit = 0;	
	UINT32 tx_loop_data = 0;
	
	/*==================================================*/
	unit = 0;
	pDev0 = vxbInstByNameFind (FM_SPI_NAME, unit);
	if (pDev0 == NULL)
	{
		printf ("\nCan not find this spi unit(%d)!	\n\n", unit);
		return ERROR;
	}
	else
	{
		pDrvCtrl0 = (FM_SPI_DRV_CTRL *)pDev0->pDrvCtrl;
		/*printf ("\nvxbInstByNameFind(%s): 0x%X \n\n", FM_SPI_NAME, pDev0);*/
	}
	/*==================================================*/

	/*==================================================*/
	unit = 1;
	pDev1 = vxbInstByNameFind (FM_SPI_NAME, unit);
	if (pDev1 == NULL)
	{
		printf ("\nCan not find this spi unit(%d)!	\n\n", unit);
		return ERROR;
	}
	else
	{
		pDrvCtrl1 = (FM_SPI_DRV_CTRL *)pDev1->pDrvCtrl;
		/*printf ("\nvxbInstByNameFind(%s): 0x%X \n\n", FM_SPI_NAME, pDev1);*/
	}
	/*==================================================*/
	
	
	if (tx_data == 0)
	{
		tx_loop_data = g_test_spi3;
		g_test_spi3++;		
	}
	else
	{
		tx_loop_data = tx_data;
	}
	
	vxSpiCtrl_X_Loop(pDev0, pDev1,tx_loop_data);

	return OK;
}

#endif


