/* vxNfc_fm.c - FMSH NFC Controller Driver */

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
routine is the vxNfcRegister(), which registers the driver with
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
LOCAL struct vxNfcDevInfo spiDevTbl[] = {
    { "spiFlash_w25q64",       0, 8, 24000000,  NFC_MODE_0},
};

LOCAL UINT32 txEvt = {16, 18};
LOCAL UINT32 rxEvt = {17, 19};

LOCAL struct hcfResource vxNfcResources[] =  {
    { "regBase",       HCF_RES_INT,   { (void *)(AM335X_SPI0_BASE) } },
    { "clkFreq",       HCF_RES_INT,   { (void *)(48000000)  } },
    { "polling",       HCF_RES_INT,   { (void *)(FALSE) } },
    { "dmaMode",       HCF_RES_INT,   { (void *)(TRUE) } },
    { "spiDev",        HCF_RES_ADDR,  { (void *)&spiDevTbl} },
    { "spiDevNum",     HCF_RES_INT,   { (void *)NELEMENTS(spiDevTbl)}}
    { "edmaTEvt",      HCF_RES_ADDR,  { (void *)&txEvt} },
    { "edmaREvt",      HCF_RES_ADDR,  { (void *)&rxEvt} },
};
#   define vxNfcNum  NELEMENTS(vxNfcResources)
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

#include "vxbNandFlash_fm.h"
#include "vxbNfc_fm.h"

/* debug macro */
#undef  NFC_DBG_ON
#define  NFC_DBG_ON

#ifdef  NFC_DBG_ON

/* turning local symbols into global symbols */
#ifdef  LOCAL
#undef  LOCAL
#endif
#define LOCAL

#define NFC_DBG_OFF            0x00000000
#define NFC_DBG_ISR            0x00000001
#define NFC_DBG_RW             0x00000002
#define NFC_DBG_ERR            0x00000004
#define NFC_DBG_RTN            0x00000008
#define NFC_DBG_ALL            0xffffffff

IMPORT FUNCPTR _func_logMsg;

#define NFC_DBG(mask, string, X1, X2, X3, X4, X5, X6)       \
        if (_func_logMsg != NULL) \
           (* _func_logMsg)(string, (int)X1, (int)X2, (int)X3, (int)X4, \
                (int)X5, (int)X6)
                
#define NFC_DBG1(string, X1, X2, X3, X4, X5, X6)  printf(string, (int)X1, (int)X2, (int)X3, (int)X4, (int)X5, (int)X6)
	
#else
#define NFC_DBG(mask, string, X1, X2, X3, X4, X5, X6)
#endif  /* NFC_DBG_ON */

#define NFC_DBG2(string, X1, X2, X3, X4, X5, X6)  printf(string, (int)X1, (int)X2, (int)X3, (int)X4, (int)X5, (int)X6)



/* VxBus methods */
#if 1
LOCAL void vxNfcInstInit (VXB_DEVICE_ID pDev);
LOCAL void vxNfcInstInit2 (VXB_DEVICE_ID pDev);
LOCAL void vxNfcInstConnect (VXB_DEVICE_ID pDev);
LOCAL void vxNfcShow (VXB_DEVICE_ID, int);
LOCAL STATUS vxNfcInstUnlink (VXB_DEVICE_ID pDev, void * unused);

/* forward declarations */
LOCAL void vxNfcCtrl_Init (VXB_DEVICE_ID pDev);
LOCAL void vxNfcIsr (VXB_DEVICE_ID pDev);
#endif

extern int vxNfcSlcr_Set_NfcMIO_2(void);
extern UINT32 vxFlashCmdIdx_to_NfcRegVal(UINT32 flash_cmd_idx);


#if 1
/* locals */

LOCAL struct drvBusFuncs vxNfcVxbFuncs = 
{
    vxNfcInstInit,      /* devInstanceInit */
    vxNfcInstInit2,     /* devInstanceInit2 */
    vxNfcInstConnect    /* devConnect */
};

/*
LOCAL device_method_t vxNfcDeviceMethods[] = 
{
    DEVMETHOD (vxNfcControlGet, vxNfcCtrlGet),
    DEVMETHOD (busDevShow,       vxNfcShow),
    DEVMETHOD (vxbDrvUnlink,     vxNfcInstUnlink),
    DEVMETHOD_END
};
*/

LOCAL struct vxbPlbRegister vxNfcDevRegistration = 
{
    {
    NULL,                   /* pNext */
    VXB_DEVID_DEVICE,       /* devID */
    VXB_BUSID_PLB,          /* busID = PLB */
    VXB_VER_4_0_0,          /* vxbVersion */
    
    FM_NFC_NAME,            /* drvName : fmcan */
    
    &vxNfcVxbFuncs,         /* pDrvBusFuncs */
    NULL /*vxNfcDeviceMethods*/,     /* pMethods */
    NULL,                   /* devProbe  */
    NULL,                   /* pParamDefaults */
    },
};


/*****************************************************************************
*
* vxNfcRegister - register with the VxBus subsystem
*
* This routine registers the SPI driver with VxBus Systems.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void vxNfcRegister (void)
{
    /*vxbDevRegister ((struct vxbDevRegInfo * &vxNfcDevRegistration);*/
}

void vxNfcRegister2 (void)
{
    vxbDevRegister ((struct vxbDevRegInfo *) &vxNfcDevRegistration);
}

/*****************************************************************************
*
* vxNfcInstInit - initialize fmsh SPI controller
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

LOCAL void vxNfcInstInit
    (
    VXB_DEVICE_ID pDev
    )
{
    FM_NFC_DRV_CTRL * pDrvCtrl;
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
    pDrvCtrl = (FM_NFC_DRV_CTRL *) malloc (sizeof (FM_NFC_DRV_CTRL));
    if (pDrvCtrl == NULL)
    {
        return;
    }
	
    bzero ((char *)pDrvCtrl, sizeof(FM_NFC_DRV_CTRL));

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
	printf("<Can>pDev->unitNumber:(%d); pDrvCtrl->regBase:0x%08X \n", pDev->unitNumber, pDrvCtrl->regBase);
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
        pDrvCtrl->sysClk = 100000000;  /* 100M Hz */
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

    if (devResourceGet (pHcf, "busWidth", HCF_RES_INT,
                        (void *) &pDrvCtrl->busWidth) != OK)
    {
        pDrvCtrl->busWidth = 16;
    }

    pDrvCtrl->initPhase = 1;
	return;
}

/*******************************************************************************
*
* vxNfcCtrlInit -  SPI controller initialization
*
* This routine performs the SPI controller initialization.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxNfcCtrl_Init (VXB_DEVICE_ID pDev)
{
    FM_NFC_DRV_CTRL * pDrvCtrl;
	
    UINT32 value;
    UINT32 us = 0;
	int ctrl_x = 0;

	UINT32 ctrlReg_val = 0;
	int ret = 0;

    /* check for valid parameter */
   /* VXB_ASSERT_NONNULL_V (pDev);*/

    pDrvCtrl = (FM_NFC_DRV_CTRL *) pDev->pDrvCtrl;	
	pDrvCtrl->ctrl_x = pDev->unitNumber;
	
    pDrvCtrl->cfgBaseAddr = (UINT32 *)pDrvCtrl->regBase;
	
	ctrl_x = pDrvCtrl->ctrl_x;
	pDrvCtrl->devId = ctrl_x;
	
	/*==================================================*/
#if 1
	/**/
	/* setup nfc_MIO of slcr*/
	/**/
	#if 1  /* for some board: MIO not init in fsbl or uboot*/
	/*nfcSlcr_Set_NfcMIO();*/
	/*vxNfcSlcr_Set_NfcMIO_2();*/
	#endif


	vxNfcSlcr_Rst_NfcCtrl(pDev);

	pDrvCtrl->eccMode = NFCPS_ECC_NONE_VX;
	
	vxNfcCtrl_Wr_StatusMask(pDev, 0x0140);
	
	vxNfcCtrl_Set_Timing(pDev);

	vxNfcCtrl_Wr_StatusMask(pDev, 0x0140);

	vxNfcCtrl_Get_FlashInfo(pDev);
	
	ret = vxNfcFlash_Chk_FlashId(pDev);
	if (ret == FMSH_FAILURE)
	{
		return FMSH_FAILURE;
	}
	
	/* 
	set ctrl 
	*/
	ctrlReg_val = NFCPS_AUTO_READ_STAT_EN_MASK | NFCPS_READ_STATUS_EN_MASK;
	vxNfcCtrl_Wr_CtrlReg(pDev, (ctrlReg_val & NFCPS_USER_CTRL_MASK));
	
	/* 
	set iowidth 
	*/
	ret = vxNfcCtrl_Set_IoWidth(pDev, pDrvCtrl->busWidth);
	if (ret == FMSH_FAILURE)
	{
		return FMSH_FAILURE;
	}
	
	/* 
	set blocksize 
	*/
	ret = vxNfcCtrl_Set_BlockSize(pDev, (pDrvCtrl->NAND.PagePerBlock));
	if (ret == FMSH_FAILURE)
	{
		return FMSH_FAILURE;
	}

	/* 
	set ecc_blocksize 
	*/
	ret = vxNfcCtrl_Set_EccBlockSize(pDev, (pDrvCtrl->NAND.PageSize >> 2)); 	
	if (ret == FMSH_FAILURE)
	{
		return FMSH_FAILURE;
	}

	/* 
	set interrupt 
	*/
	ctrlReg_val = vxNfcCtrl_Rd_CtrlReg(pDev);    
	if (ctrlReg_val & NFCPS_INT_EN_MASK)
	{
	  vxNfcCtrl_Wr_IrqMask(pDev, (~NFCPS_INTR_ALL));
	}
	
	/* 
	set ecc 
	*/
	if (ctrlReg_val & NFCPS_ECC_EN_MASK)
	{		 
		if(pDrvCtrl->eccMode != NFCPS_ECC_HW_VX)
		{
			vxNfcCtrl_Dis_Ecc(pDev);
		}
	}
	
	/* 
	set ecc mdoe
	*/
	vxNfcCtrl_Set_EccMode(pDev, pDrvCtrl->eccMode);

	/* 
	set bbm 
	*/
	vxNand_Init_BadBlkTable_Desc(pDev);
	ret = vxNand_Scan_BadBlkTable(pDev);
	if (ret == FMSH_FAILURE)
	{
		return FMSH_FAILURE;
	}
	
	if (ctrlReg_val & NFCPS_BBM_EN_MASK)
	{
		vxNfcCtrl_Set_Dev0PTR(pDev, (UINT32)pDrvCtrl->map_BB);
		vxNfcCtrl_Set_Dev0Size(pDev, (NAND_MAX_BADBLKS >> 3));
		vxNfcCtrl_Init_ReMap(pDev); 
	}

#endif
	/*==================================================*/

    pDrvCtrl->initDone = TRUE;
	return;
}

/*******************************************************************************
*
* vxNfcInstInit2 - second level initialization routine of SPI controller
*
* This routine performs the second level initialization of the SPI controller.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxNfcInstInit2
    (
    VXB_DEVICE_ID pDev
    )
{
    FM_NFC_DRV_CTRL * pDrvCtrl;

    /* check for valid parameter */
    /*VXB_ASSERT_NONNULL_V (pDev);*/

    pDrvCtrl = (FM_NFC_DRV_CTRL *) pDev->pDrvCtrl;

    /* 
    The semSync semaphore is used to synchronize the SPI transfer. 
	*/
    pDrvCtrl->semSync = semBCreate (SEM_Q_PRIORITY, SEM_FULL);
    if (pDrvCtrl->semSync == NULL)
    {
        NFC_DBG (NFC_DBG_ERR, "semBCreate failed for semSync\n",
                 0, 0, 0, 0, 0, 0);
        return;
    }

    /* 
    The muxSem semaphore is used to mutex accessing the controller. 
    */
    pDrvCtrl->muxSem = semMCreate (SEM_Q_PRIORITY);
    if (pDrvCtrl->muxSem == NULL)
    {
        NFC_DBG (NFC_DBG_ERR, "semMCreate failed for muxSem\n",
                 0, 0, 0, 0, 0, 0);

        (void) semDelete (pDrvCtrl->semSync);
        pDrvCtrl->semSync = NULL;

        return;
    }
	
    /* 
    SPI controller init 
    */
    (void) vxNfcCtrl_Init(pDev);

    pDrvCtrl->initPhase = 2;
}

/*******************************************************************************
*
* vxNfcInstConnect - third level initialization
*
* This routine performs the third level initialization of the SPI controller
* driver.
*
* RETURNS: N/A
*
* ERRNO : N/A
*/

LOCAL void vxNfcInstConnect
    (
    VXB_DEVICE_ID       pDev
    )
{
    FM_NFC_DRV_CTRL * pDrvCtrl;

    /* check for valid parameter */
    /*VXB_ASSERT_NONNULL_V (pDev);*/

    pDrvCtrl = (FM_NFC_DRV_CTRL *) pDev->pDrvCtrl;

    /* 
    connect and enable interrupt for non-poll mode 
	*/
    if (!pDrvCtrl->polling)
    {
        if (vxbIntConnect (pDev, 0, vxNfcIsr, pDev) != OK)
        {
            NFC_DBG(NFC_DBG_ERR, "vxbIntConnect return ERROR \n",
                    1, 2, 3, 4, 5, 6);
        }
        (void) vxbIntEnable (pDev, 0, vxNfcIsr, pDev);
    }

    pDrvCtrl->initPhase = 3;
}

/*****************************************************************************
*
* vxNfcInstUnlink - VxBus unlink handler
*
* This function shuts down a SPI controller instance in response to an
* an unlink event from VxBus. This may occur if our VxBus instance has
* been terminated, or if the SPI driver has been unloaded.
*
* RETURNS: OK if device was successfully destroyed, otherwise ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxNfcInstUnlink
    (
    VXB_DEVICE_ID pDev,
    void *        unused
    )
{
    /* check if the pDev pointer is valid */
    /*VXB_ASSERT (pDev != NULL, ERROR)*/

    FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL *) pDev->pDrvCtrl;

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
            (void) vxbIntDisable (pDev, 0, vxNfcIsr, pDev);

            if (vxbIntDisconnect (pDev, 0, vxNfcIsr, pDev) != OK)
            {
                NFC_DBG(NFC_DBG_ERR, "vxbIntDisconnect return ERROR \n",
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
        /*vxNfcDmaChanFree (pDev);*/
    }

    pDrvCtrl->initPhase = 0;
    pDev->pDrvCtrl = NULL;

    return (OK);
}


/*******************************************************************************
*
* vxNfcIsr - interrupt service routine
*
* This routine handles interrupts of SPI.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxNfcIsr
    (
    VXB_DEVICE_ID pDev
    )
{
    FM_NFC_DRV_CTRL * pDrvCtrl;

    /* Check if the pDev pointer is valid */
    VXB_ASSERT (pDev != NULL, ERROR)

    pDrvCtrl = (FM_NFC_DRV_CTRL *)pDev->pDrvCtrl;

    /* disable all SPI interrupts */
/*
	CSR_WRITE_4 (pDev, MCNFC_IRQENABLE, 0);

    NFC_DBG (NFC_DBG_ISR, "vxNfcIsr: intSts 0x%x\n", 
             CSR_READ_4 (pDev, MCNFC_IRQSTATUS), 2, 3, 4, 5, 6);
*/
    semGive (pDrvCtrl->semSync);

	return;
}

#endif


#if 1


void vxNfcCtrl_Wr_CfgReg32(VXB_DEVICE_ID pDev, UINT32 offset, UINT32 value)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;
	UINT32 tmp32 = pDrvCtrl->cfgBaseAddr;
	
	FMQL_WRITE_32((tmp32 + offset), value);
	return;
}

UINT32 vxNfcCtrl_Rd_CfgReg32(VXB_DEVICE_ID pDev, UINT32 offset)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;	
	UINT32 tmp32 = pDrvCtrl->cfgBaseAddr;
	
	return FMQL_READ_32(tmp32 + offset);
}

/***********************************
 * FIFO
 ******************/
void vxNfcCtrl_Wr_Fifo(VXB_DEVICE_ID pDev, UINT32 data)
{
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_FIFO_DATA_OFFSET, data);
	return;
}

UINT32 vxNfcCtrl_Rd_Fifo(VXB_DEVICE_ID pDev)
{
    return vxNfcCtrl_Rd_CfgReg32(pDev, NFCPS_FIFO_DATA_OFFSET);
}

void vxNfcCtrl_Clr_Fifo(VXB_DEVICE_ID pDev)
{
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_FIFO_INIT_OFFSET, NFCPS_FIFO_INIT_MASK);
	return;
}

/***************************
* CTRL
*************/
void vxNfcCtrl_Wr_CtrlReg(VXB_DEVICE_ID pDev, UINT32 ctrl_val)
{
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_CONTROL_OFFSET, ctrl_val);
	return;
}

UINT32 vxNfcCtrl_Rd_CtrlReg(VXB_DEVICE_ID pDev)
{
    return vxNfcCtrl_Rd_CfgReg32(pDev, NFCPS_CONTROL_OFFSET);
}

/***********************************
 * INT_MASK
 *****************/
void vxNfcCtrl_Wr_IrqMask(VXB_DEVICE_ID pDev, UINT32 mask)
{ 
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_INT_MASK_OFFSET, mask);
	return;
}

UINT32 vxNfcCtrl_Rd_IrqMask(VXB_DEVICE_ID pDev)
{ 
    return vxNfcCtrl_Rd_CfgReg32(pDev, NFCPS_INT_MASK_OFFSET);
}

/***********************************
 * INT_STATUS
 *****************/
void vxNfcCtrl_Clr_IrqStatus(VXB_DEVICE_ID pDev)
{
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_INT_STATUS_OFFSET, (~NFCPS_INTR_ALL));
	return;
}

UINT32 vxNfcCtrl_Rd_IrqStatus(VXB_DEVICE_ID pDev)
{  
    return vxNfcCtrl_Rd_CfgReg32(pDev, NFCPS_INT_STATUS_OFFSET);
}


/**********************
 * STATUS_MASK
 *********************/
void vxNfcCtrl_Wr_StatusMask(VXB_DEVICE_ID pDev, UINT32 mask)
{  
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_STATUS_MASK_OFFSET, mask);
	return;
}

/*****************
 * TIMING
 ****************/
void vxNfcCtrl_Set_Timing(VXB_DEVICE_ID pDev)
{
    UINT32 cfgReg = 0;
	
    cfgReg = ((FPS_NFC_TIMING_TWHR & 0x3f) << 24) | 
                ((FPS_NFC_TIMING_TRHW & 0x3f) << 16) | 
                ((FPS_NFC_TIMING_TADL & 0x3f) << 8) | 
                ((FPS_NFC_TIMING_TCCS & 0x3f) << 0);
	
   vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_TIME_SEQ_0_OFFSET, cfgReg);
    
    cfgReg = ((FPS_NFC_TIMING_TWW & 0x3f) << 16) | 
                ((FPS_NFC_TIMING_TRR & 0x3f) << 8) | 
                ((FPS_NFC_TIMING_TWB & 0x3f) << 0);
	
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_TIME_SEQ_1_OFFSET, cfgReg);
    
    cfgReg = ((FPS_NFC_TIMING_TRWH & 0xf) << 4) | 
                ((FPS_NFC_TIMING_TRWP & 0xf));
	
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_TIMINGS_ASYN_OFFSET, cfgReg);
	
	return;
}



int vxNfcCtrl_Get_FlashInfo(VXB_DEVICE_ID pDev)
{ 
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;

    int ret;
    UINT32 first_word, second_word;
    UINT8 Id1, Id2, Id3, Id4, Id5;
	
	UINT32 flashID[2]= {0};	
	
	vxbT_PARAMPAGE_FLASH ParamPage = {0};

	/**/
    /* Reset Nandflash chip*/
    /**/
    ret = vxNfcFlash_Rst_NandChip(pDev);
    if (ret == ERROR)
	{
        return ERROR;
    }

	/**/
    /* read id*/
    /**/
    ret = vxNfcFlash_Get_ChipId(pDev, (UINT32*)(&flashID[0]));
    if (ret == ERROR)
	{
        return ERROR;
    }
	
    /* calculate ID*/
    Id1 = flashID[0] & 0xff;
    Id2 = (flashID[0] >> 8)  & 0xff;
    Id3 = (flashID[0] >> 16) & 0xff;
    Id4 = (flashID[0] >> 24) & 0xff;
	
    Id5 = flashID[1] & 0xff;

	/**/
    /* Set NandflashModel*/
    /**/
    pDrvCtrl->NAND.Manufacture = Id1;
    pDrvCtrl->NAND.DevId = Id2;
    pDrvCtrl->NAND.Options = 0;
	
    /* Fetch from the extended ID4
    * ID4 D5  D4 BlockSizeKB || D1  D0  PageSize
    *     0   0   64K      || 0   0   1K
    *     0   1   128K     || 0   1   2K
    *     1   0   256K     || 1   0   4K
    *     1   1   512K     || 1   1   8k 
    */
    
    /* Page Size*/
    switch (Id4 & 0x03) 
	{
    case 0x00: 
		pDrvCtrl->NAND.PageSize = 1024; 
		break;
    case 0x01: 
		pDrvCtrl->NAND.PageSize = 2048; 
		break;
    case 0x02: 
		pDrvCtrl->NAND.PageSize = 4096; 
		break;
    case 0x03: 
		pDrvCtrl->NAND.PageSize = 8192; 
		break;
    }
	
    /* Spare Size*/
    switch(Id4 & 0x04)
	{
    case 0x00: 
		pDrvCtrl->NAND.SpareSize = 32; 
		break;
    case 0x04: 
		pDrvCtrl->NAND.SpareSize = 64; 
		break;
    }
	
    /* Block Size*/
    switch (Id4 & 0x30) 
	{
    case 0x00: 
		pDrvCtrl->NAND.BlockSizeKB = 64;  
		break;
    case 0x10: 
		pDrvCtrl->NAND.BlockSizeKB = 128; 
		break;
    case 0x20: 
		pDrvCtrl->NAND.BlockSizeKB = 256; 
		break;
    case 0x30: 
		pDrvCtrl->NAND.BlockSizeKB = 512; 
		break;
    }
	
    /* IO width*/
    switch (Id4 & 0x40)
	{
    case 0x00: 
		pDrvCtrl->NAND.IoWidth = 8;  
		break;
    case 0x40: 
		pDrvCtrl->NAND.IoWidth = 16; 
		break;
    }
	
    /* calculate other parameters*/
    pDrvCtrl->NAND.PagePerBlock = (UINT16)(((UINT32)pDrvCtrl->NAND.BlockSizeKB << 10) / pDrvCtrl->NAND.PageSize);
    pDrvCtrl->NAND.BlockPerLun = NAND_MAX_BLOCKS;
    pDrvCtrl->NAND.LunNum = 0x1;
	
    pDrvCtrl->NAND.BlockNum =  pDrvCtrl->NAND.BlockPerLun * pDrvCtrl->NAND.LunNum;
    pDrvCtrl->NAND.PageNum = pDrvCtrl->NAND.PagePerBlock * pDrvCtrl->NAND.BlockPerLun * pDrvCtrl->NAND.LunNum;
    pDrvCtrl->NAND.DevSizeMB = (UINT16)((pDrvCtrl->NAND.BlockSizeKB * pDrvCtrl->NAND.BlockPerLun * pDrvCtrl->NAND.LunNum) >> 10);

	pDrvCtrl->NAND.RowAddrCycle = 3;
    pDrvCtrl->NAND.ColAddrCycle = 2;


	/**/
    /* Get Nandflash Id(check if support ONFI)*/
    /**/
    /*
    if (vxNfcFlash_Get_OnfiId(pDev) ==  NANDFLASH_ONFI_ID)
	{
		//
        // device support onfi then read parameter page(256 bytes)
        //
        ret = vxNfcFlash_Get_ParamPage(pDev, &ParamPage);
        if (ret == ERROR)
		{
            return ERROR;
        }
		
        // Initialize parameter
        pDrvCtrl->NAND.Manufacture = ParamPage.JedecManufacturerId;    
        pDrvCtrl->NAND.PageSize = ParamPage.BytesPerPage;
        pDrvCtrl->NAND.SpareSize = ParamPage.SpareBytesPerPage;
        pDrvCtrl->NAND.PagePerBlock = ParamPage.PagesPerBlock;
        pDrvCtrl->NAND.BlockSizeKB = (ParamPage.PagesPerBlock * ParamPage.BytesPerPage)>>10;
        pDrvCtrl->NAND.BlockPerLun = ParamPage.BlocksPerLun;
        pDrvCtrl->NAND.DevSizeMB = (ParamPage.NumLuns * ParamPage.BlocksPerLun * pDrvCtrl->NAND.BlockSizeKB)>>10;
        pDrvCtrl->NAND.PageNum = ParamPage.PagesPerBlock * ParamPage.BlocksPerLun * ParamPage.NumLuns;
        pDrvCtrl->NAND.BlockNum =  ParamPage.BlocksPerLun * ParamPage.NumLuns;
        pDrvCtrl->NAND.LunNum = ParamPage.NumLuns;
        
        pDrvCtrl->NAND.RowAddrCycle = ParamPage.AddrCycles & 0xf;
        pDrvCtrl->NAND.ColAddrCycle = (ParamPage.AddrCycles >> 4) & 0xf;
    }
	*/
	
    pDrvCtrl->busWidth = pDrvCtrl->NAND.IoWidth;
    
    return OK;
}

extern void sysUsDelay(int);

int vxNfcCtrl_Wait_ExeOk(VXB_DEVICE_ID pDev, UINT32 offset, UINT32 regMask, UINT8 ok_flag)
{
    UINT32 tmp32 = 0;
    int timeout = 4500;
    
    do
	{
        sysUsDelay(1);
		
        tmp32 = vxNfcCtrl_Rd_CfgReg32(pDev, offset) & regMask; 
		switch (ok_flag)
		{
		case OK_FLAG_1:
			if (tmp32 == regMask)
			{
				return OK;
			} 
			break;
			
		case OK_FLAG_0:
			if (tmp32 == 0x00)
			{
				return OK;
			}			
			break;
		}
		
        timeout--;
    } while (timeout > 0);

	return ERROR;
}


int vxNfcCtrl_Set_IoWidth(VXB_DEVICE_ID pDev, UINT32 width)
{
    UINT32 cfgReg = 0;
       
    /* 
    validity check 
	*/
    if ((width != 8) && (width != 16))
    {
        return ERROR;
    }
    
    cfgReg = vxNfcCtrl_Rd_CfgReg32(pDev, NFCPS_CONTROL_OFFSET);
    if (width == 8)
    {
        cfgReg &= ~NFCPS_IOWIDTH16_MASK;
    }
    else if(width == 16)
    {
        cfgReg |= NFCPS_IOWIDTH16_MASK;
    }
	
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_CONTROL_OFFSET, cfgReg);    
    
    return OK;
}


int vxNfcCtrl_Set_BlockSize(VXB_DEVICE_ID pDev, UINT32 size)
{
    UINT32 cfgReg = 0, sizeCfg = 0;
    
    /* validity check */
    if (size == 32)
    {
        sizeCfg = 0;
    }
    else if (size == 64)
    {
        sizeCfg = 1;
    }
    else if (size == 128)
    {
        sizeCfg = 2;
    }
    else if (size == 256)
    {
        sizeCfg = 3;
    }
    else
    {
        return ERROR;
    } 
    
    cfgReg = vxNfcCtrl_Rd_CfgReg32(pDev, NFCPS_CONTROL_OFFSET);
	
    cfgReg &= ~NFCPS_BLOCK_SIZE_MASK;
    cfgReg |= (sizeCfg << NFCPS_BLOCK_SIZE_SHIFT) & NFCPS_BLOCK_SIZE_MASK;
	
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_CONTROL_OFFSET, cfgReg);
    
    return OK;
}


int vxNfcCtrl_Set_EccBlockSize(VXB_DEVICE_ID pDev, UINT32 size)
{
    UINT32 cfgReg = 0, sizeCfg = 0;
    
    /* validity check */
    if(size == 256)
    {
        sizeCfg = 0;
    }
    else if(size == 512)
    {
        sizeCfg = 1;
    }
    else if(size == 1024)
    {
        sizeCfg = 2;
    }
    else if(size == 2048)
    {
        sizeCfg = 3;
    }
    else
    {
        return ERROR;
    }
    
    cfgReg = vxNfcCtrl_Rd_CfgReg32(pDev, NFCPS_CONTROL_OFFSET);
	
    cfgReg &= ~NFCPS_ECC_BLOCK_SIZE_MASK;
    cfgReg |= (sizeCfg << NFCPS_ECC_BLOCK_SIZE_SHIFT) & NFCPS_ECC_BLOCK_SIZE_MASK;
	
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_CONTROL_OFFSET, cfgReg);
    
    return OK;
}

void vxNfcCtrl_En_Ecc(VXB_DEVICE_ID pDev)
{
    UINT32 cfgReg = 0;
    
    cfgReg = vxNfcCtrl_Rd_CfgReg32(pDev, NFCPS_CONTROL_OFFSET);
    cfgReg |= NFCPS_ECC_EN_MASK;
	
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_CONTROL_OFFSET, cfgReg);    
    return;
}

void vxNfcCtrl_Dis_Ecc(VXB_DEVICE_ID pDev)
{
    UINT32 cfgReg = 0;
    
    cfgReg = vxNfcCtrl_Rd_CfgReg32(pDev, NFCPS_CONTROL_OFFSET);
    cfgReg &= ~NFCPS_ECC_EN_MASK;
	
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_CONTROL_OFFSET, cfgReg);    
    return;
}

/***********************************
 * ECC_STAT
 * 
 *
 ****************/
void vxNfcCtrl_Wr_EccOffset(VXB_DEVICE_ID pDev, UINT16 offset)
{
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ECC_OFFSET_OFFSET, offset);
    return;
}

UINT16 vxNfcCtrl_Rd_EccOffset(VXB_DEVICE_ID pDev)
{
    return vxNfcCtrl_Rd_CfgReg32(pDev, NFCPS_ECC_OFFSET_OFFSET);
}


/*************************************
 * ECC_CTRL
 * all configuration parameters required by the ECC
 * IntrSrc-correctable ret/uncorrectable ret/acceptable errors level overflow
 * Errlvl-number of errors that is acceptable for the host system
 * EccCap-2/4/8/16/24/32
 ************************/
int vxNfcCtrl_Set_EccCtrl(VXB_DEVICE_ID pDev, UINT32 sel, UINT32 threshold, UINT32 cap)
{
    UINT32 cfgReg = 0;
    
    /* validity check */
    if ((sel != NFCPS_ECC_ERROR_STATE)    \
		&& (sel != NFCPS_ECC_UNC_STATE)   \
		&& (sel != NFCPS_ECC_OVER_STATE))
    {
        return ERROR;
    }
	   
    if (threshold > 0x3f)
    {
        return ERROR;
    }
	
    if ((cap != NFCPS_ECC_CAP2_STATE) && (cap != NFCPS_ECC_CAP4_STATE)   &&  \
        (cap != NFCPS_ECC_CAP8_STATE) && (cap != NFCPS_ECC_CAP16_STATE)  &&  \
        (cap != NFCPS_ECC_CAP24_STATE) && (cap != NFCPS_ECC_CAP32_STATE))
    {
        return ERROR;
    }
    
    cfgReg = (sel << NFCPS_ECC_SEL_SHIFT) | (threshold << NFCPS_ECC_THRESHOLD_SHIFT) | cap;  	   
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ECC_CTRL_OFFSET, cfgReg);
    
    return OK;
}


int vxNfcCtrl_Set_EccMode(VXB_DEVICE_ID pDev, UINT32 eccMode)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;
	
	int ret = 0;
	UINT32 feature;
	UINT32 offset;

	vxNfcCtrl_Dis_Ecc(pDev);

	switch (eccMode)
	{
	case NFCPS_ECC_NONE_VX:
		break;

	case NFCPS_ECC_ONDIE_VX:
		feature = vxNfcFlash_Get_Feature(pDev, 0x90);
		if ((feature & 0x08) != 0x08)
		{ 
			ret = vxNfcFlash_Set_Feature(pDev, 0x90, 0x08);
			if (ret == ERROR)
			{
				return ERROR;
			}
		}
		break;

	case NFCPS_ECC_HW_VX:
		vxNfcCtrl_En_Ecc(pDev);
		
		offset = SLCR_REG_BASE + vxNandInfo_Get_PageSize(pDev) + NFCPS_ECC_HW_OFFSET;
	
		if (pDrvCtrl->NAND.IoWidth == 8)
		{
			vxNfcCtrl_Wr_EccOffset(pDev, offset);
		} 
		else if(pDrvCtrl->NAND.IoWidth == 16)
		{
			vxNfcCtrl_Wr_EccOffset(pDev, (offset >> 1));
		}
		else
		{
			return ERROR;
		}
		
		ret = vxNfcCtrl_Set_EccCtrl(pDev, NFCPS_ECC_UNC_STATE, 2, NFCPS_ECC_CAP2_STATE);   
		if (ret == ERROR)
		{
		  return ERROR;
		}
		break;
		
	case NFCPS_ECC_SW_VX:
		/* not implemented */
		break;
		
	default: 
		break;
	}

	return OK;
}


/******************************
 * BBM
 *****************/

/*
RMP_INIT	[0]	remapping初始标志。 如果设置此标志，强制BBM模块在软件更新后重新读取remapping表。该标志由软件置1并由硬件清零。
*/
void vxNfcCtrl_Init_ReMap(VXB_DEVICE_ID pDev)
{
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_BBM_CTRL_OFFSET, 0x1);
	return;
}

/*
PTR_ADDR	[31: 0]	remapping表指针。 该字段包含系统内存中的重映射表的地址。
*/
void vxNfcCtrl_Set_Dev0PTR(VXB_DEVICE_ID pDev, UINT32 address)
{
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_DEV0_PTR_OFFSET, address);
	return;
}

void vxNfcCtrl_Set_Dev0Size(VXB_DEVICE_ID pDev, UINT32 size)
{
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_DEV0_SIZE_OFFSET, size);
	return;
}

UINT32 vxNfcCtrl_Get_HwProt(VXB_DEVICE_ID pDev)
{ 
    UINT32 cfgReg = 0;
	
    cfgReg = vxNfcCtrl_Rd_CfgReg32(pDev, NFCPS_MEM_CTRL_OFFSET);
	
    cfgReg &= NFCPS_CTRL_MEM0WP_MASK;
    cfgReg >>= NFCPS_CTRL_MEM0WP_SHIFT;
	
    return cfgReg;   
}

#endif

#if 1
int vxNfcFlash_Rst_NandChip(VXB_DEVICE_ID pDev)
{
    int ret = 0;
    UINT32 reg_val = 0;
    
    /* 
    Set reg_val (Reset) 
	*/ 
    reg_val = vxFlashCmdIdx_to_NfcRegVal(NANDFLASH_RESET);
	
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_COMMAND_OFFSET, reg_val);
	
    /* 
    Wait for Ready 
	*/
    ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);

	if (ret == ERROR)
    {
        return ERROR;
    }
	else
	{
	    return OK;
	}    
}


UINT32 vxNfcFlash_Get_OnfiId(VXB_DEVICE_ID pDev)
{
    int ret = 0;
    UINT32 reg_val;
    UINT32 onfi_id;
    
    /* Set Address*/
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ADDR0_COL_OFFSET, 0x20);
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ADDR0_ROW_OFFSET, 0);
	
    /* 0x40: Set Data Reg Size (4 Bytes)*/
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_DATA_REG_SIZE_OFFSET, 0x3); /* bit[1~0]: 11 - all four bytes valid */
	
    /* Set reg_val (Data Reg & Read ONFI ID)*/
    /*
    bit[7]: DATA_SEL
		0 – the FIFO module selected
		1 – the DATA register selected
	*/
    reg_val = vxFlashCmdIdx_to_NfcRegVal(NANDFLASH_READ_ID);
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_COMMAND_OFFSET, (reg_val | (0x1<<7)));  /* use data reg*/
    
    /* Wait For Ready*/
    /*
	MEM0_ST [0] 设备0状态标志：
			1 - 设备ready
			0 - 设备busy
	*/
    ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);
    if (ret == ERROR)
    {
        return ERROR;
    }
	
    /* Wait For Data Reg Avaliable*/
    /*
	DATA_REG_ST [10]	DATA_REG：只有通过读取DATA_REG寄存器中的数据才能复位该标志。
			1 - DATA_REG中的数据可用
			0 - DATA_REG中的数据不可用
	*/
    ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_STATUS_OFFSET, NFCPS_STAT_DATA_REG_RDY_MASK, OK_FLAG_1);
    if (ret == ERROR)
    {
        return ERROR;
    }
	
    /* Get Status*/
    onfi_id = vxNfcCtrl_Rd_CfgReg32(pDev, NFCPS_DATA_REG_OFFSET);
    
    return onfi_id;
}


int vxNfcFlash_Get_ChipId(VXB_DEVICE_ID pDev, UINT32* flashID)
{
    int ret = 0;
    UINT32 reg_val = 0;
    
    /* Set Address*/
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ADDR0_COL_OFFSET, 0);
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ADDR0_ROW_OFFSET, 0);
	
    /* 0x84: Set Data Size*/
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_DATA_SIZE_OFFSET, 5);
	
    /* Set reg_val ( FIFO & SIU & RDID)*/
    reg_val = vxFlashCmdIdx_to_NfcRegVal(NANDFLASH_READ_ID);
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_COMMAND_OFFSET, reg_val);
	
    /* Wait For FIFO Empty*/
    /*
	CF_EMPTY	[2]	命令FIFO 空标志。
					该位指示实际的命令FIFO状态。它不能用于检查下一次传输是否会被接受。
	*/
    ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_FIFO_STATE_OFFSET, NFCPS_STAT_CFIFO_EMPTY_MASK, OK_FLAG_1);
    if (ret == ERROR)
    {
        return ERROR;
    }
	
    /* Wait For Status Ready*/
    /*
	MEM0_ST [0] 设备0状态标志：
		1 - 设备ready
		0 - 设备busy
	*/
    ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);
    if (ret == ERROR)
    {
        return ERROR;
    }
	
    /* Wait for FIFO DFR not empty*/
    /*
	DF_R_EMPTY	[0]	FIFO空 状态位。
					该位表示FIFO中没有数据可用。该标志对读取方向有效。
	*/
    ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_FIFO_STATE_OFFSET, NFCPS_STAT_DFIFO_R_EMPTY_MASK, OK_FLAG_0);
    if (ret == ERROR)
    {
        return ERROR;
    }
	
    /* Get Id*/
    flashID[0] = vxNfcCtrl_Rd_Fifo(pDev); 
    flashID[1] = vxNfcCtrl_Rd_Fifo(pDev); 
    
    return OK;
}


int vxNfcFlash_Get_ParamPage(VXB_DEVICE_ID pDev, void* ParamPage)
{
    int ret = 0;
    UINT32 reg_val;
    int ByteCount;
    
    UINT32 * pBuf32 = (UINT32 *)ParamPage;
    
    /* Set Address*/
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ADDR0_COL_OFFSET, 0);
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ADDR0_ROW_OFFSET, 0);
	
    /* Set Data Size*/
    ByteCount = 256;
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_DATA_SIZE_OFFSET, ByteCount);

	/**/
    /* Set reg_val ( FIFO & SIU & RD PARA PAGE)*/
    /**/
    reg_val = vxFlashCmdIdx_to_NfcRegVal(NANDFLASH_READ_PARAMETER_PAGE);
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_COMMAND_OFFSET, reg_val);
	
	
    /* Wait For FIFO Empty*/
    /*
	CF_EMPTY	[2]	命令FIFO 空标志。
					该位指示实际的命令FIFO状态。它不能用于检查下一次传输是否会被接受。
	*/
    ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_FIFO_STATE_OFFSET, NFCPS_STAT_CFIFO_EMPTY_MASK, OK_FLAG_1);
    if (ret == ERROR)
    {
        return ERROR;
    }
	
    /* Wait For Status Ready*/
    /*
	MEM0_ST [0] 设备0状态标志：
		1 - 设备ready
		0 - 设备busy
	*/
    ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);
    if (ret == ERROR)
    {
        return ERROR;
    }	
	
    /* Wait for FIFO DFR not empty*/
    /*
	DF_R_EMPTY	[0]	FIFO空 状态位。
					该位表示FIFO中没有数据可用。该标志对读取方向有效。
	*/
    ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_FIFO_STATE_OFFSET, NFCPS_STAT_DFIFO_R_EMPTY_MASK, OK_FLAG_0);
    if (ret == ERROR)
    {
        return ERROR;
    }

	/**/
    /* Get ParamPage*/
    /**/
    while (ByteCount > 0)
    {
        *pBuf32 = vxNfcCtrl_Rd_Fifo(pDev); 
        pBuf32++;
		
        ByteCount -= 4;
    }
    
    return OK;
}

int vxNfcFlash_Chk_FlashId(VXB_DEVICE_ID pDev)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;

	UINT8 maker = pDrvCtrl->NAND.Manufacture;
	UINT8 ioWidth = pDrvCtrl->NAND.IoWidth;

	/* 
	validity check 
	*/
	if ((maker != NAND_MICRON_ID)      \
		&& (maker != NAND_SPANSION_ID) \
		&& (maker != NAND_FMSH_ID))
	{
		return ERROR;
	} 
	  
	if ((ioWidth != 8) && (ioWidth != 16)) 
	{
		return ERROR;
	}

	return OK;
}


UINT32 vxNfcFlash_Get_Feature(VXB_DEVICE_ID pDev, UINT8 feature_in)
{
    int ret = 0;
    UINT32 reg_val = 0;
    UINT32 feature;
	
    /* Set Address*/
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ADDR0_COL_OFFSET, feature_in);
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ADDR0_ROW_OFFSET, 0);
	
    /* Set Data Reg Size (4 Bytes)*/
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_DATA_REG_SIZE_OFFSET, 0x3);
	
    /* Set Command (Data Reg & Get Feature)*/
    reg_val = vxFlashCmdIdx_to_NfcRegVal(NANDFLASH_GET_FEATURE);
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_COMMAND_OFFSET, (reg_val | (0x1 << 7)));
	
    /* Wait For Ready*/
    /*
	MEM0_ST [0] 设备0状态标志：
			1 - 设备ready
			0 - 设备busy
	*/
    ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);
    if (ret == ERROR)
    {
        return ERROR;
    }
		
    /* Wait For Data Reg Avaliable*/
    /*
	DATA_REG_ST [10]	DATA_REG：只有通过读取DATA_REG寄存器中的数据才能复位该标志。
			1 - DATA_REG中的数据可用
			0 - DATA_REG中的数据不可用
	*/
    ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_STATUS_OFFSET, NFCPS_STAT_DATA_REG_RDY_MASK, OK_FLAG_1);
    if (ret == ERROR)
    {
        return ERROR;
    }
	
    /* Get Status*/
    feature =  vxNfcCtrl_Rd_CfgReg32(pDev, NFCPS_DATA_REG_OFFSET);
	
    return feature;
}


int vxNfcFlash_Set_Feature(VXB_DEVICE_ID pDev, UINT8 feature, UINT32 Value)
{
    int ret = 0;
    UINT32 reg_val = 0;
    
    /* Set Address*/
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ADDR0_COL_OFFSET, feature);  
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ADDR0_ROW_OFFSET, 0);
	
    /* Set Data Size*/
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_DATA_SIZE_OFFSET, 4);
	
    /* Set FIFO*/
    vxNfcCtrl_Clr_Fifo(pDev);
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_FIFO_DATA_OFFSET, Value);
	
    /* Set Command (Program)*/
    reg_val = vxFlashCmdIdx_to_NfcRegVal(NANDFLASH_SET_FEATURE);
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_COMMAND_OFFSET, reg_val);
	
    /* Wait For CFIFO Empty*/
    /*
    DF_W_EMPTY	[7]	FIFO空状态位。
    				该位表示FIFO中没有数据可用。该标志对写入方向有效。
	CF_EMPTY	[2]	命令FIFO 空标志。
					该位指示实际的命令FIFO状态。它不能用于检查下一次传输是否会被接受。
	*/
    ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_FIFO_STATE_OFFSET, \
                             (NFCPS_STAT_DFIFO_W_EMPTY_MASK | NFCPS_STAT_CFIFO_EMPTY_MASK), OK_FLAG_1);
    if (ret == ERROR)
    {
        return ERROR;
    }
		
    /* Wait For Ready*/
    /*
	MEM0_ST [0] 设备0状态标志：
			1 - 设备ready
			0 - 设备busy
	*/
    ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);
    if (ret == ERROR)
    {
        return ERROR;
    }
    
    return OK;
}

#endif

#if 1
/**
* vxNfcFlash_Read_Page: read a page
* Nandflash_ReadSpare: read pSpareBuf area
*/
int vxNfcFlash_Read_Page(VXB_DEVICE_ID pDev, UINT16 block, UINT16 page, UINT16 offset, void* pDataBuf, void* pSpareBuf)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;

    int ret;
	
    UINT32 ctrl;
    UINT32 reg_val;
    UINT32 colAddr, rowAddr;
    UINT32 byteCnt, cnt; 
    UINT32 *data32, *spare32;    

	/* 
	page data_area 
	*/
    if (pDataBuf)
    {
        data32 = (UINT32*)pDataBuf; 
		
        /* ByteCount is page size */                       
        byteCnt= pDrvCtrl->NAND.PageSize;
		
        /* Set Address */
        if (pDrvCtrl->NAND.IoWidth == 8)
        {
            colAddr = offset;
        }
        else if (pDrvCtrl->NAND.IoWidth == 16)
        {
            colAddr = offset >> 1;
        }
        else
        {
            return ERROR;
        }
		
        rowAddr = (block << 6) | page;
		
        vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ADDR0_COL_OFFSET, colAddr);
        vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ADDR0_ROW_OFFSET, rowAddr);
		
        /* Set Data Size to be transfered */
        vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_DATA_SIZE_OFFSET, byteCnt);
		
        vxNfcCtrl_Clr_Fifo(pDev);
		
        /* Set Command (Read) */
        reg_val = vxFlashCmdIdx_to_NfcRegVal(NANDFLASH_READ_PAGE);
        vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_COMMAND_OFFSET, reg_val);
		
        /* Wait For CFIFO Empty */
        ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_FIFO_STATE_OFFSET, NFCPS_STAT_CFIFO_EMPTY_MASK, OK_FLAG_1);
        if (ret == ERROR)
        {
            return ERROR;
        }
		
        /* Wait For Ready */
        ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);
        if (ret == ERROR)
        {
            return ERROR;
        }
		
        /* Read pDataBuf from fifo if dfifo is not empty*/
        while(byteCnt > 0)
        {
            ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_FIFO_STATE_OFFSET, NFCPS_STAT_DFIFO_R_EMPTY_MASK, OK_FLAG_0);
			if (ret == ERROR)
			{
				return ERROR;
			}
			
            *data32 = vxNfcCtrl_Rd_CfgReg32(pDev, NFCPS_FIFO_DATA_OFFSET); 
            data32++;
			
            byteCnt -= 4;  
        }  
    }
	
	/* 
	page spare_area 
	*/
    if (pSpareBuf)
    {
        spare32 = (UINT32*)pSpareBuf;
		
        ctrl = vxNfcCtrl_Rd_CtrlReg(pDev);
		
        /* Disable Ecc */
        if ((ctrl & NFCPS_ECC_EN_MASK) == 1)
        {
            vxNfcCtrl_Wr_CtrlReg(pDev, ctrl & ~NFCPS_ECC_EN_MASK);
        }
		
        /* Set Address */
        if (pDrvCtrl->NAND.IoWidth == 8)
        {
            colAddr = pDrvCtrl->NAND.PageSize;
        }
        else if (pDrvCtrl->NAND.IoWidth == 16)
        {
            colAddr = pDrvCtrl->NAND.PageSize >> 1;
        }
        else
        {
            return ERROR;
        }
		
        rowAddr = (block << 6) | page;
		
        vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ADDR0_COL_OFFSET, colAddr);
        vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ADDR0_ROW_OFFSET, rowAddr);
		
        /* Set Data Size */
        cnt = pDrvCtrl->NAND.SpareSize;
        vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_DATA_SIZE_OFFSET, cnt);
        vxNfcCtrl_Clr_Fifo(pDev);
		
        /* Set Command (Read) */
        reg_val = vxFlashCmdIdx_to_NfcRegVal(NANDFLASH_READ_PAGE);
        vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_COMMAND_OFFSET, reg_val);
		
        /* Wait For FIFO Empty */
        ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_FIFO_STATE_OFFSET, NFCPS_STAT_CFIFO_EMPTY_MASK, OK_FLAG_1);
        if (ret == ERROR)
        {
            return ERROR;
        }
		
        /* Wait For Ready */
        ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);
        if (ret == ERROR)
        {
            return ERROR;
        }
		
        /* Read pDataBuf from fifo */
        while(cnt > 0)
        {
            ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_FIFO_STATE_OFFSET, NFCPS_STAT_DFIFO_R_EMPTY_MASK, OK_FLAG_0);
            if (ret == ERROR)
            {
                return ERROR;
            }  
			
            *spare32 = vxNfcCtrl_Rd_CfgReg32(pDev, NFCPS_FIFO_DATA_OFFSET); 
            spare32++;
			
            cnt -= 4;  
        } 
		
        /* Re-Enable Ecc */
        if ((ctrl & NFCPS_ECC_EN_MASK) == 1)
        {
            vxNfcCtrl_Wr_CtrlReg(pDev, ctrl);
        }
    }
    
    return OK;
}

/**
* Nandflash_WritePage: write a page
*/
int vxNfcFlash_Write_Page(VXB_DEVICE_ID pDev, UINT16 block, UINT16 page, UINT16 offset, void* pDataBuf, void* pSpareBuf)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;

    int ret;
    UINT32 ctrl;
    UINT32 reg_val;
    UINT32 colAddr, rowAddr;
    UINT32 byteCnt, cnt;
    UINT32* data32, *spare32;      
    
    if (pDataBuf)
    {
        data32 = (UINT32*)pDataBuf;
		
        /* ByteCount is page size */
        byteCnt = pDrvCtrl->NAND.PageSize;
		
        while (byteCnt > 0)
        {
            /* Set Address */
            if (pDrvCtrl->NAND.IoWidth == 8)
            {
                colAddr = offset;
            }
            else if (pDrvCtrl->NAND.IoWidth == 16)
            {
                colAddr = offset >> 1;
            }
            else
            {
                return ERROR;
            }
			
            rowAddr = (block << 6) | page;
			
            vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ADDR0_COL_OFFSET, colAddr);
            vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ADDR0_ROW_OFFSET, rowAddr);
			
            /* Set Data Size to be transfered */
            if(byteCnt > NFCPS_FIFO_DEPTH)
            {
                /* page size is larger than fifo depth (2048 Bytes) */
                cnt = NFCPS_FIFO_DEPTH;
            }
            else
            {
                cnt = byteCnt;
            }
			
            offset += cnt;
            byteCnt -= cnt;
			
            vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_DATA_SIZE_OFFSET, cnt);
			
            /* Set FIFO */
            vxNfcCtrl_Clr_Fifo(pDev);
			
            while (cnt > 0)
            {
                vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_FIFO_DATA_OFFSET, *data32);
                data32++;
				
                cnt -= 4;
            }
			
            /* Set Command (Program) */
            reg_val = vxFlashCmdIdx_to_NfcRegVal(NANDFLASH_PROGRAM_PAGE);
            vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_COMMAND_OFFSET, reg_val);
			
            /* Wait for FIFO_STATE.DF_EMPTY */
            ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_FIFO_STATE_OFFSET,  \
                                     (NFCPS_STAT_DFIFO_W_EMPTY_MASK | NFCPS_STAT_CFIFO_EMPTY_MASK), OK_FLAG_1);
            if (ret == ERROR)
            {
                return ERROR;
            }
			
            /* Wait For Ready */
            ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);
            if (ret == ERROR)
            {
                return ERROR;
            }
        }
    }  
	
    if (pSpareBuf)
    {
        spare32 = (UINT32*)pSpareBuf;
		
        ctrl = vxNfcCtrl_Rd_CtrlReg(pDev);
		
        /* Disable Ecc */
        if ((ctrl & NFCPS_ECC_EN_MASK) == 1)
        {
            vxNfcCtrl_Wr_CtrlReg(pDev, ctrl & ~NFCPS_ECC_EN_MASK);
        }
		
        /* Set Address */
        if (pDrvCtrl->NAND.IoWidth == 8)
        {
            colAddr = pDrvCtrl->NAND.PageSize;
        }
        else if (pDrvCtrl->NAND.IoWidth == 16)
        {
            colAddr = pDrvCtrl->NAND.PageSize >> 1;
        }
		
        rowAddr = (block << 6) | page;
		
        vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ADDR0_COL_OFFSET, colAddr);
        vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ADDR0_ROW_OFFSET, rowAddr);
		
        /* Set Data Size */
        cnt = pDrvCtrl->NAND.SpareSize;
        vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_DATA_SIZE_OFFSET, cnt);
		
        /* Set FIFO */
        vxNfcCtrl_Clr_Fifo(pDev);
		
        while (cnt > 0)
        {
            vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_FIFO_DATA_OFFSET, *spare32);
            spare32++;
			
            cnt -= 4;
        }
		
        /* Set Command (Program) */
        reg_val = vxFlashCmdIdx_to_NfcRegVal(NANDFLASH_PROGRAM_PAGE);
        vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_COMMAND_OFFSET, reg_val);
		
        /* Wait for FIFO_STATE.DF_EMPTY */
        ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_FIFO_STATE_OFFSET, \
                                 (NFCPS_STAT_DFIFO_W_EMPTY_MASK | NFCPS_STAT_CFIFO_EMPTY_MASK), OK_FLAG_1);
        if (ret == ERROR)
        {
            return ERROR;
        }
		
        /* Wait For Ready */
        ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);
        if (ret == ERROR)
        {
            return ERROR;
        }
		
        /* Re-Enable Ecc */
        if((ctrl & NFCPS_ECC_EN_MASK) == 1)
        {
            vxNfcCtrl_Wr_CtrlReg(pDev, ctrl);
        }
    }
    
    return OK; 
}

int vxNfcFlash_Read_MultiPage(VXB_DEVICE_ID pDev, UINT64 srcAddr, UINT32 byteCnt_in, UINT8* pDstBuf, UINT8* pUsrSpare)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;

	int ret;
	UINT64 devSize;
	UINT16 block, page, offset;
	UINT32 length;

	UINT8 pDataBuf[NAND_MAX_PAGE_DATASIZE];
	UINT8 pSpareBuf[NAND_MAX_PAGE_SPARESIZE];
	UINT32 byteCnt = 0;
	
	byteCnt = byteCnt_in;


	/* validity check */
	devSize = vxNandInfo_Get_DevSize_Bytes(pDev);
	
	if (byteCnt == 0)
	{
		return ERROR;
	}

	if ((srcAddr + byteCnt) > devSize)
	{
		return ERROR;
	}

	/* remaining pDataBuf to be read */
	while (byteCnt > 0)
	{
		/* calculate address */
		vxNandInfo_Parse_FlashAddr(pDev, srcAddr, byteCnt, &block, &page, &offset);

		/* read entire page from nandflash */
		ret = vxNfcFlash_Read_Page(pDev, block, page, 0, pDataBuf, pSpareBuf);
		if (ret == ERROR)
		{
		  return ERROR;
		}

		/* maxium length of bytes needed */
		length = pDrvCtrl->NAND.PageSize - offset;

		/* check has more pDataBuf to read */
		if (byteCnt > length)
		{
			memcpy(pDstBuf, &pDataBuf[offset], length);
			pDstBuf += length;
			srcAddr += length;
			byteCnt -= length;
		} 
		else
		{
			memcpy(pDstBuf, &pDataBuf[offset], byteCnt);
			byteCnt = 0;
		}
	}

	if (pUsrSpare)
	{
		memcpy(pUsrSpare, &pSpareBuf[0], pDrvCtrl->NAND.SpareSize);
	}

	return OK;
}


int vxNfcFlash_Write_MultiPage(VXB_DEVICE_ID pDev, UINT64 dstAddr, UINT32 byteCnt_in, UINT8 *pSrcBuf, UINT8 *pUsrSpare)
{
	FM_NFC_DRV_CTRL * pDrvCtrl = (FM_NFC_DRV_CTRL*)pDev->pDrvCtrl;

	int ret;

	UINT64 devSize;
	UINT16 block, page, offset;
	UINT32 length;

	UINT8 pDataBuf[NAND_MAX_PAGE_DATASIZE];
	UINT8 pSpareBuf[NAND_MAX_PAGE_SPARESIZE];
	UINT32 byteCnt = 0;
	
	byteCnt = byteCnt_in;

	/* validity check */
	devSize = vxNandInfo_Get_DevSize_Bytes(pDev);
	/*devSize = (UINT64)pDrvCtrl->NAND.DevSizeMB << 20;*/
	
	if (byteCnt == 0)
	{
		return ERROR;
	}
	
	if ((dstAddr + byteCnt) > devSize)
	{
		return ERROR;
	}
	
	/* 
	check write protect status 
	*/
	if (vxNfcCtrl_Get_HwProt(pDev) == 0)
	{
		return ERROR;
	}

	/* copy pSpareBuf buffer */
	if (pUsrSpare)
	{
		memcpy(&pSpareBuf[0], pUsrSpare, pDrvCtrl->NAND.SpareSize);
	} 
	else
	{
		memset(&pSpareBuf[0], 0xff, NAND_MAX_PAGE_SPARESIZE);
	}

	while (byteCnt > 0)
	{
		/* calculate address */
		vxNandInfo_Parse_FlashAddr(pDev, dstAddr, byteCnt, &block, &page, &offset);
		
		/* length of bytes to be send to nandflash */
		length = pDrvCtrl->NAND.PageSize - offset;
		if (byteCnt < length)
		{
		  length = byteCnt;
		}
		
		/* init & fill buffer */
		memset(&pDataBuf[0], 0xff, NAND_MAX_PAGE_DATASIZE);
		memcpy(&pDataBuf[offset], pSrcBuf, length);
		
		pSrcBuf += length;
		
		/* write to nandflash */
		ret = vxNfcFlash_Write_Page(pDev, block, page, 0, pDataBuf, pSpareBuf);
		if (ret == ERROR)
		{
		  return ERROR;
		}
		
		/* check has more pDataBuf in buffer to write */
		if (byteCnt > length)
		{
			dstAddr += length;
			byteCnt -= length;
		} 
		else
		{
			byteCnt = 0;
		}
	}

	return OK;
}


/*******************************
* Erase Blcok
* @block 
***********/
int vxNfcFlash_Erase_Block(VXB_DEVICE_ID pDev, UINT32 block_idx) 
{
    int ret = 0;
    UINT32 reg_val;
    
    /* Set Address */
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ADDR0_COL_OFFSET, 0);
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_ADDR0_ROW_OFFSET, (block_idx << 6));
	
    /* Set Command(seq14) */
    reg_val = vxFlashCmdIdx_to_NfcRegVal(NANDFLASH_ERASE_BLOCK);
    vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_COMMAND_OFFSET, reg_val);
	
    /* Wait for Flash Ready */
    ret = vxNfcCtrl_Wait_ExeOk(pDev, NFCPS_STATUS_OFFSET, NFCPS_STAT_MEM0_RDY_MASK, OK_FLAG_1);
    if (ret == ERROR)
    {
        return ERROR;
    }
    
    return OK;
}

#endif

#if 1

void vxNfcSlcr_Rst_NfcCtrl(VXB_DEVICE_ID pDev)
{  
	UINT32 tmp32 = 0;

	tmp32 = slcr_read(SLCR_NFC_CTRL);
	tmp32 |= (NFC_AHB_RST_MASK);
	slcr_write(SLCR_NFC_CTRL, tmp32);
	
	tmp32 = slcr_read(SLCR_NFC_CTRL);
	tmp32 |= (NFC_REF_RST_MASK);
	slcr_write(SLCR_NFC_CTRL, tmp32);

	/*delay_1ms();*/
	
	tmp32 = slcr_read(SLCR_NFC_CTRL);
	tmp32 &= ~(NFC_AHB_RST_MASK);
	slcr_write(SLCR_NFC_CTRL, tmp32);
	
	tmp32 = slcr_read(SLCR_NFC_CTRL);
	tmp32 &= ~(NFC_REF_RST_MASK);
	slcr_write(SLCR_NFC_CTRL, tmp32);
	
    /* 
    clear RETRY_EN bit
	*/
	vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_CONTROL_OFFSET, 0x0);
	
    /* 
    disable protect
	*/
	vxNfcCtrl_Wr_CfgReg32(pDev, NFCPS_MEM_CTRL_OFFSET, 0x0100);

	return;
}

#endif


#if 1

int g_init_flag_nfc3 = 0;

void nfc_id3(void)
{
	int ret = 0;
	
    UINT32 first_word, second_word;
    UINT8 Id1, Id2, Id3, Id4, Id5;
	
	UINT32 flashId[2] = {0};
	
	vxbT_NAND_INFO nand;
		
    VXB_DEVICE_ID     pDev;
    FM_NFC_DRV_CTRL * pDrvCtrl;	
	int unit = 0;	
	
	/*==================================================*/
	unit = 0;
	pDev = vxbInstByNameFind (FM_NFC_NAME, unit);
	if (pDev == NULL)
	{
		printf ("\nCan not find this spi unit(%d)!	\n\n", unit);
		return ERROR;
	}
	else
	{
		pDrvCtrl = (FM_NFC_DRV_CTRL *)pDev->pDrvCtrl;
		printf ("\nvxbInstByNameFind(%s): 0x%X \n\n", FM_NFC_NAME, pDev);
	}
	/*==================================================*/

	
	/* read id*/
    ret = vxNfcFlash_Get_ChipId(pDev, (UINT32*)(&flashId[0]));
    if (ret == ERROR)
	{
        return ERROR;
    }
	
	first_word  = flashId[0];
	second_word = flashId[1];

	printf("<vx> nand_id(MT29F1G16A 0x5580B12C-00000004): 0x%08X-%08X \n", first_word, second_word);
	
    /* calculate ID*/
    Id1 = first_word & 0xff;
    Id2 = (first_word >> 8) & 0xff;
    Id3 = (first_word >> 16) & 0xff;
    Id4 = (first_word >> 24) & 0xff;
    Id5 = second_word & 0xff;
    
    /*Set NandflashModel*/
    nand.Manufacture = Id1;
    nand.DevId = Id2;
    nand.Options = 0;

	printf("Manufacture: 0x%X \n", nand.Manufacture);
	printf("DeviceId: 0x%X \n\n", nand.DevId);
	
    /* Fetch from the extended ID4
    * ID4 D5  D4 BlockSize || D1  D0  PageSize
    *     0   0   64K      || 0   0   1K
    *     0   1   128K     || 0   1   2K
    *     1   0   256K     || 1   0   4K
    *     1   1   512K     || 1   1   8k */
    
    /* Page Size*/
    switch(Id4 & 0x03) 
	{
    case 0x00: 
		nand.PageSize = 1024; 
		break;
    case 0x01: 
		nand.PageSize = 2048; 
		break;
    case 0x02: 
		nand.PageSize = 4096; 
		break;
    case 0x03: 
		nand.PageSize = 8192; 
		break;
    }
	printf("PageSize: %d \n", nand.PageSize);
	
    /* Spare Size*/
    switch(Id4 & 0x04)
	{
    case 0x00: 
		nand.SpareSize = 32; 
		break;
    case 0x04: 
		nand.SpareSize = 64; 
		break;
    }
	printf("SpareSize: %d \n", nand.SpareSize);
	
    /* Block Size*/
    switch(Id4 & 0x30) 
	{
    case 0x00: 
		nand.BlockSizeKB = 64;  
		break;
    case 0x10: 
		nand.BlockSizeKB = 128; 
		break;
    case 0x20: 
		nand.BlockSizeKB = 256; 
		break;
    case 0x30: 
		nand.BlockSizeKB = 512; 
		break;
    }
	printf("BlockSizeKB: %d KB \n", nand.BlockSizeKB);
	
    /* IO width*/
    switch(Id4 & 0x40)
	{
    case 0x00: 
		nand.IoWidth = 8;  
		break;
    case 0x40: 
		nand.IoWidth = 16; 
		break;
    }
	printf("IoWidth: %d bit \n\n", nand.IoWidth);
	
    /* calculate other parameters*/
    nand.BlockPerLun = NAND_MAX_BLOCKS;
    nand.LunNum = 0x1;
    nand.PagePerBlock = (u16)(((u32)nand.BlockSizeKB << 10) / nand.PageSize);
    nand.BlockNum =  nand.BlockPerLun * nand.LunNum;
    nand.PageNum = nand.PagePerBlock * nand.BlockPerLun * nand.LunNum;
    nand.DevSizeMB = (u16)((nand.BlockSizeKB * nand.BlockPerLun * nand.LunNum) >> 10);
	
	printf("PagePerBlock: %d \n", nand.PagePerBlock);
	printf("BlockNum: %d \n", nand.BlockNum);
	printf("PageNum: %d \n", nand.PageNum);
	printf("DeviceSizeInMegaBytes: %d MB \n", nand.DevSizeMB);

	printf("\n");
	return;
}

void nfc_onfi_id3(void)
{
	UINT32 onfi_id = 0;	
	
    VXB_DEVICE_ID     pDev;
    FM_NFC_DRV_CTRL * pDrvCtrl;	
	int unit = 0;	
	
	/*==================================================*/
	unit = 0;
	
	pDev = vxbInstByNameFind (FM_NFC_NAME, unit);
	if (pDev == NULL)
	{
		printf ("\nCan not find this spi unit(%d)!	\n\n", unit);
		return ERROR;
	}
	else
	{
		pDrvCtrl = (FM_NFC_DRV_CTRL *)pDev->pDrvCtrl;
		printf ("\nvxbInstByNameFind(%s): 0x%X \n\n", FM_NFC_NAME, pDev);
	}
	/*==================================================*/

	onfi_id = vxNfcFlash_Get_OnfiId(pDev);

	printf("<vx>onfi_id: 0x%08X \n", onfi_id);
	return;
}



int g_test_nfc3 = 0;

UINT8 Nfc_SndBuf3[2056] = {0};
UINT8 Nfc_RcvBuf3[2056] = {0};

void test_nfc_rd_page_vx(int page_idx)
{
	int i = 0;
	
	int page_size = 0;
	UINT64 flashAddr =0;	

    VXB_DEVICE_ID     pDev;
    FM_NFC_DRV_CTRL * pDrvCtrl;	
	int unit = 0;	
	
	/*==================================================*/
	unit = 0;
	
	pDev = vxbInstByNameFind (FM_NFC_NAME, unit);
	if (pDev == NULL)
	{
		printf ("\nCan not find this spi unit(%d)!	\n\n", unit);
		return ERROR;
	}
	else
	{
		pDrvCtrl = (FM_NFC_DRV_CTRL *)pDev->pDrvCtrl;
		printf ("\nvxbInstByNameFind(%s): 0x%X \n\n", FM_NFC_NAME, pDev);
	}
	/*==================================================*/

	
	page_size = vxNandInfo_Get_PageSize(pDev);
	flashAddr = (page_idx * page_size);
	
	printf("----vx devSize(%d)MB---- \n\n", vxNandInfo_Get_DevSize_MB(pDev));
	
    /* read data*/
    if (OK != vxNfcFlash_Read_MultiPage(pDev, flashAddr, page_size, Nfc_RcvBuf3, 0))
	{
		printf("vxNfcFlash_Read_MultiPage fail->flashAddr: 0x%X \n", flashAddr);
		return;
	}

	printf("----vx page(%d), size(%d)---- \n", page_idx, page_size);
	for (i=0; i<256; i++)
	{
		printf("%02X", Nfc_RcvBuf3[i]);
		if ((i+1)%4 == 0)
		{
			printf(" ");
		}
		if ((i+1)%16 == 0)
		{
			printf("\n");
		}
	}
	
	printf(" ... ... ... \n");
	
	for (i=(page_size-256); i<page_size; i++)
	{
		printf("%02X", Nfc_RcvBuf3[i]);
		if ((i+1)%4 == 0)
		{
			printf(" ");
		}
		if ((i+1)%16 == 0)
		{
			printf("\n");
		}
	}	
	printf("------------vx------------ \n\n");

	return;
}

void test_nfc_erase_blk_vx(int block_idx)
{
    VXB_DEVICE_ID     pDev;
    FM_NFC_DRV_CTRL * pDrvCtrl;	
	int unit = 0;	
	
	/*==================================================*/
	unit = 0;
	
	pDev = vxbInstByNameFind (FM_NFC_NAME, unit);
	if (pDev == NULL)
	{
		printf ("\nCan not find this spi unit(%d)!	\n\n", unit);
		return ERROR;
	}
	else
	{
		pDrvCtrl = (FM_NFC_DRV_CTRL *)pDev->pDrvCtrl;
		printf ("\nvxbInstByNameFind(%s): 0x%X \n\n", FM_NFC_NAME, pDev);
	}
	/*==================================================*/

	/* erase*/
	return vxNfcFlash_Erase_Block(pDev, block_idx); 
}


void test_nfc_wr_page_vx(int page_idx)
{
	int i = 0;
	
	int page_size = 0, page = 0;
	UINT64 flashAddr =0;
	
    VXB_DEVICE_ID     pDev;
    FM_NFC_DRV_CTRL * pDrvCtrl;	
	int unit = 0;	
	
	/*==================================================*/
	unit = 0;
	
	pDev = vxbInstByNameFind (FM_NFC_NAME, unit);
	if (pDev == NULL)
	{
		printf ("\nCan not find this spi unit(%d)!	\n\n", unit);
		return ERROR;
	}
	else
	{
		pDrvCtrl = (FM_NFC_DRV_CTRL *)pDev->pDrvCtrl;
		printf ("\nvxbInstByNameFind(%s): 0x%X \n\n", FM_NFC_NAME, pDev);
	}
	/*==================================================*/
	
	page_size = vxNandInfo_Get_PageSize(pDev);	
	flashAddr = (page_idx * page_size);
	page = page_size;
	
    for (i=0; i<page_size; i++)
    {
        Nfc_SndBuf3[i] = g_test_nfc3 + i; 
        Nfc_RcvBuf3[i] = 0;
    }
	g_test_nfc3++;
	
    if (OK != vxNfcFlash_Write_MultiPage(pDev, flashAddr, page_size, Nfc_SndBuf3, 0))
	{
		printf("vxNfcFlash_Write_MultiPage Fail! page(%d)->flashAddr:0x%X, pageSize:0x%X \n\n", page_idx, flashAddr, page_size);
		return;
	}
	else
	{
		printf("vxNfcFlash_Write_MultiPage OK! page(%d)->flashAddr:0x%X, pageSize:%d \n\n", page_idx, flashAddr, page);
	}
	
	return;
}

#endif

