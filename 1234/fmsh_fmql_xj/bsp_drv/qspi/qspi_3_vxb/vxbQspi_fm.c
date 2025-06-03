/* vxbQspi_fm.c - FMSH QSPI Controller Driver */

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
01a,12Dec19,jc  created.
*/

/*
DESCRIPTION

The FMSH QSPI is a high-speed serial input/output port that allows
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
routine is the vxQspiRegister(), which registers the driver with
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
LOCAL struct vxbSpiDevInfo spiDevTbl[] = {
    { "spiFlash_w25q64",       0, 8, 24000000,  SPI_MODE_0},
};

LOCAL UINT32 txEvt = {16, 18};
LOCAL UINT32 rxEvt = {17, 19};

LOCAL struct hcfResource fmshSpiResources[] =  {
    { "regBase",       HCF_RES_INT,   { (void *)(AM335X_SPI0_BASE) } },
    { "clkFreq",       HCF_RES_INT,   { (void *)(48000000)  } },
    { "polling",       HCF_RES_INT,   { (void *)(FALSE) } },
    { "dmaMode",       HCF_RES_INT,   { (void *)(TRUE) } },
    { "spiDev",        HCF_RES_ADDR,  { (void *)&spiDevTbl} },
    { "spiDevNum",     HCF_RES_INT,   { (void *)NELEMENTS(spiDevTbl)}}
    { "edmaTEvt",      HCF_RES_ADDR,  { (void *)&txEvt} },
    { "edmaREvt",      HCF_RES_ADDR,  { (void *)&rxEvt} },
};
#   define fmshSpiNum  NELEMENTS(fmshSpiResources)
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

#include "vxbQspiFlash_fm.h"
#include "vxbQspi_fm.h"

/* debug macro */
#undef  QSPI_DBG_ON
#define  QSPI_DBG_ON

#ifdef  QSPI_DBG_ON

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

LOCAL UINT32 spiDbgMask = SPI_DBG_ALL;
IMPORT FUNCPTR _func_logMsg;
/* vx_origin*/
#define QSPI_DBG(mask, string, X1, X2, X3, X4, X5, X6)       \
           printf(string, (int)X1, (int)X2, (int)X3, (int)X4, (int)X5, (int)X6)
/* jc           */
#define QSPI_DBG1(string, X1, X2, X3, X4, X5, X6)       \
           printf(string, (int)X1, (int)X2, (int)X3, (int)X4, (int)X5, (int)X6)
#else
#define QSPI_DBG(mask, string, X1, X2, X3, X4, X5, X6)
#endif  /* QSPI_DBG_ON */

#define QSPI_DBG2(string, X1, X2, X3, X4, X5, X6)       \
           printf(string, (int)X1, (int)X2, (int)X3, (int)X4, (int)X5, (int)X6)

extern void sysUsDelay(int);

/* 
VxBus methods 
*/
LOCAL void vxQspiInstInit (VXB_DEVICE_ID pDev);
LOCAL void vxQspiInstInit2 (VXB_DEVICE_ID pDev);
LOCAL void vxQspiInstConnect (VXB_DEVICE_ID pDev);
LOCAL STATUS vxQspiInstUnlink (VXB_DEVICE_ID pDev, void * unused);


/* 
forward declarations 
*/
#if 1
LOCAL void vxQspiCtrl_Init (VXB_DEVICE_ID pDev);
LOCAL void vxQspiIsr (VXB_DEVICE_ID pDev);
LOCAL STATUS vxQspiWaitBitSet (VXB_DEVICE_ID pDev,UINT32 regOffset,UINT32 regBit);
/**/
void vxQspiCtrl_Wr_CfgReg32(VXB_DEVICE_ID pDev, UINT32 offset, UINT32 value);
UINT32 vxQspiCtrl_Rd_CfgReg32(VXB_DEVICE_ID pDev, UINT32 offset);
void vxQspiCtrl_Reset(int ctrl_x);
int vxQspiCtrl_Wait_BusIdle(VXB_DEVICE_ID pDev);
int vxQspiCtrl_Wait_ExeOk(VXB_DEVICE_ID pDev, UINT32 offset, UINT32 regMask, UINT8 ok_flag);
int vxQspiCtrl_CmdExecute(VXB_DEVICE_ID pDev, UINT32 ctrl_cmd);
void vxQspiCtrl_Set_BaudRate(VXB_DEVICE_ID pDev, int baud_rate);
/**/
UINT8 vxQspiFlash_Get_Reg8(VXB_DEVICE_ID pDev, UINT8 flash_cmd);
int vxQspiFlash_Set_Reg8(VXB_DEVICE_ID pDev, UINT8 flash_cmd, UINT8 value);
UINT16 vxQspiFlash_Get_Reg16(VXB_DEVICE_ID pDev, UINT8 flash_cmd);
int vxQspiFlash_Set_Reg16(VXB_DEVICE_ID pDev, UINT8 flash_cmd, UINT8 high_val8, UINT8 low_val8);
int vxQspiFlash_WREN_Cmd(VXB_DEVICE_ID pDev);
int vxQspiFlash_Wait_WIP_Ok(VXB_DEVICE_ID pDev, int time_out);
UINT8 vxQspiFlash_Get_RegStatus1(VXB_DEVICE_ID pDev) ;
UINT32 vxQspiFlash_ReadId_2(VXB_DEVICE_ID pDev, UINT32 * buf);
int vxQspiFlash_ParseInfo_2(VXB_DEVICE_ID pDev, UINT32 * flashId);
int vxQspiFlash_SetMode(VXB_DEVICE_ID pDev, UINT8 flash_cmd);

int vxQspiCtrl_SetFlashMode(VXB_DEVICE_ID pDev, UINT8 flash_cmd, int bytes_mode);
void vxQspiCtrl_Set_CaptureDelay(VXB_DEVICE_ID pDev, int cycles);

#endif

extern void sysUsDelay(int);
extern void sysMsDelay(int);

#if 1
static vxbT_QSPI_CTRL_PARAMS gQspi_Ctrl_Params = {0};
static vxbT_QSPI_DMA_PARAMS  gQspi_Dma_Params =  {0};


/*
ADDR_3_BYTES_MODE:
	1. flash chip size:  <= 16M 
	2. direct_mode
	3. >16M, set segment
	
ADDR_4_BYTES_MODE:
	1. > 16M 
	2. not set segment
*/
void vxQspiCtrl_params_init(void)
{
	/**/
	/* qspi ctrl params*/
	/**/
    gQspi_Ctrl_Params.hasIntr       =  1;
    gQspi_Ctrl_Params.hasDma        =  0;
    gQspi_Ctrl_Params.hasXIP        =  0;    
    gQspi_Ctrl_Params.txEmptyLvl    =  32;
    gQspi_Ctrl_Params.rxFullLvl     =  480;   
    gQspi_Ctrl_Params.rdMode        =  QOR_CMD;    
	
    gQspi_Ctrl_Params.baudRate      =  4; 
	
    gQspi_Ctrl_Params.sampleDelay   =  2;

	/**/
	/* address mode init*/
	/**/
    /*gQspi_Ctrl_Params.addrBytesMode =  ADDR_4_BYTES_MODE;      // default: 4_bytes_address mode*/
    gQspi_Ctrl_Params.addrBytesMode =  ADDR_3_BYTES_MODE;      /* for test*/
	if (QSPI_DIRECT_MODE)
	{
    	gQspi_Ctrl_Params.addrBytesMode =  ADDR_3_BYTES_MODE;  /* only 3_bytes_address mode in direct_mode		*/
	}

	/**/
	/* dma params*/
	/**/
    gQspi_Dma_Params.burstType  = 5;
    gQspi_Dma_Params.singleType = 2;
    gQspi_Dma_Params.txIf       = 6;
    gQspi_Dma_Params.rxIf       = 7;
    gQspi_Dma_Params.io         = 0;

	return;
}

#endif


/* locals */
#if 1

LOCAL struct drvBusFuncs vxQspiVxbFuncs = 
{
    vxQspiInstInit,      /* devInstanceInit */
    vxQspiInstInit2,     /* devInstanceInit2 */
    vxQspiInstConnect    /* devConnect */
};


LOCAL struct vxbPlbRegister vxQspiDevRegistration = 
{
    {
    NULL,                   /* pNext */
    VXB_DEVID_DEVICE,       /* devID */
    VXB_BUSID_PLB,          /* busID = PLB */
    VXB_VER_4_0_0,          /* vxbVersion */
    
    FM_QSPI_NAME,          /* drvName : fmspi */
    
    &vxQspiVxbFuncs,       /* pDrvBusFuncs */
    NULL,                  /* pMethods */
    NULL,                   /* devProbe  */
    NULL,                   /* pParamDefaults */
    },
};

/*****************************************************************************
*
* vxQspiRegister - register with the VxBus subsystem
*
* This routine registers the SPI driver with VxBus Systems.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void vxQspiRegister (void)
{
    vxbDevRegister ((struct vxbDevRegInfo *) &vxQspiDevRegistration);
}

void vxQspiRegister2 (void)
{
    vxbDevRegister ((struct vxbDevRegInfo *) &vxQspiDevRegistration);
}

/*****************************************************************************
*
* vxQspiInstInit - initialize fmsh SPI controller
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

LOCAL void vxQspiInstInit
    (
    VXB_DEVICE_ID pDev
    )
{
    FM_QSPI_DRV_CTRL * pDrvCtrl;
    struct hcfDevice * pHcf;
    FUNCPTR clkFunc = NULL;
    int i;

    /* 
    check for valid parameter 
	*/
   /* VXB_ASSERT_NONNULL_V (pDev);*/
    if (pDev == NULL)
        return;

    /* 
    create controller driver context structure for core 
	*/
    /*pDrvCtrl = (FM_QSPI_DRV_CTRL * hwMemAlloc (sizeof (FM_QSPI_DRV_CTRL));*/
    pDrvCtrl = (FM_QSPI_DRV_CTRL *) malloc (sizeof (FM_QSPI_DRV_CTRL));
    if (pDrvCtrl == NULL)
    {
        return;
    }
	
    bzero ((char *)pDrvCtrl, sizeof(FM_QSPI_DRV_CTRL));

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
	printf("pDev->unitNumber:(%d); pDrvCtrl->regBase:0x%08X \n", pDev->unitNumber, pDrvCtrl->regBase);

    pDrvCtrl->fifoBase = pDev->pRegBase[1];
	printf("<Qspi>pDev->unitNumber:(%d); pDrvCtrl->fifoBase:0x%08X \n", pDev->unitNumber, pDrvCtrl->fifoBase);
#endif

	i = 0;
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

    if (devResourceGet (pHcf, "fifoBase", HCF_RES_INT,
                        (void *) &pDrvCtrl->fifoBase) != OK)
    {
    	switch (pDev->unitNumber)
		{
		case 0:
			pDrvCtrl->fifoBase = QSPI0_D_BASE_ADRS;
			break;
		case 1:
			pDrvCtrl->fifoBase = QSPI1_D_BASE_ADRS;
			break;
		}
		printf("!!!fifoBase get fail!!! \n");
    }
	printf("2: pDev->unitNumber:(%d); pDrvCtrl->fifoBase:0x%08X \n", pDev->unitNumber, pDrvCtrl->fifoBase);
						
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
        pDrvCtrl->clkFrequency = 100000000;
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
    the first phase of the initialization 
	*/
    pDrvCtrl->initPhase = 1;						
	pDrvCtrl->init_flag = QSPI_CTRL_INIT_OK;

	return;
}



#define ______INIT_FUN______
/*******************************************************************************
*
* vxQspiCtrl_Init -  QSPI controller initialization
*
* This routine performs the SPI controller initialization.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxQspiCtrl_Init
    (
    VXB_DEVICE_ID pDev
    )
{
	FM_QSPI_DRV_CTRL* pDrvCtrl;

	int ret = 0;
	UINT32 flash_ID[2] = {0};
	int ctrl_x = 0;
    int mode = 0, bdr = 0, sampleDelay = 0;

    /* check for valid parameter */
   /* VXB_ASSERT_NONNULL_V (pDev);*/

    pDrvCtrl = (FM_QSPI_DRV_CTRL *) pDev->pDrvCtrl;
	QSPI_DBG1("\n\n  ... vxQspiCtrl_Init(%d):0x%08X ... \n\n\n", ctrl_x, pDrvCtrl, 3,4,5,6);

	/*
	init the pQspi ctrl default params
	*/
	vxQspiCtrl_params_init();
	

    pDrvCtrl->CTRL.cfgBaseAddr = (UINT32 *)pDrvCtrl->regBase;
    pDrvCtrl->CTRL.fifoBaseAddr = (UINT32 *)pDrvCtrl->fifoBase;
	pDrvCtrl->CTRL.ctrl_x = pDev->unitNumber;
		
	ctrl_x = pDrvCtrl->CTRL.ctrl_x;
	

	/*
	reset ctrl
	*/
	vxQspiCtrl_Reset(pDrvCtrl->CTRL.ctrl_x);
	QSPI_DBG1("vxQspiCtrl_Reset(%d) end! \n", ctrl_x, 2,3,4,5,6);

	
	/* set low baud rate*/
	bdr = 8;
	vxQspiCtrl_Set_BaudRate(pDev, QSPIPS_BAUD(bdr));
	
	/*
	read flash ID
	*/
	vxQspiFlash_ReadId_2(pDev, (UINT32 *)(&flash_ID[0]));
	/* 
	Vendor and Device ID : S25FL256S: 0x4D190201_30528001,  Spansion 
	                       N25Q512A : 0x1020BA20_64230000,  MICRON
	                       W25Q256FV: 0x001940EF_00000000,  Winbond
	*/
	QSPI_DBG1("flash ID1_2: 0x%08X_%08X \n", flash_ID[0],flash_ID[1], 3,4,5,6);
	
	/*
	parse flash info: devSize, sectSize
	*/
	ret = vxQspiFlash_ParseInfo_2(pDev, (UINT32 *)(&flash_ID[0]));
    if (ret == ERROR)
    {
		QSPI_DBG2("ctrl_%d: vxQspiFlash_ParseInfo fail! \n", ctrl_x, 2,3,4,5,6);
        return ERROR;
    }
	else
	{
		QSPI_DBG1("ctrl_%d: vxQspiFlash_ParseInfo: ", ctrl_x, 2,3,4,5,6);
		QSPI_DBG1("devSize:%d MB, sectSize:%d KB \n", \
			      (pDrvCtrl->FLASH.devSize/0x100000), (pDrvCtrl->FLASH.sectSize/1024), 3,4,5,6);
	}
	
	/**/
	/* reset flash device*/
	/**/
	vxQspiFlash_Reset_Flash(pDev);

	/**/
	/* set 4bytes_mode or 3bytes_mode*/
	/**/
	pDrvCtrl->CTRL.addr_bytes_mode = gQspi_Ctrl_Params.addrBytesMode;	
	
	/**/
	/* FAST_READ_CMD: 1-line_cmd*/
	/* QOR_CMD: 4-line_cmd*/
	/**/
	ret = vxQspiCtrl_SetFlashMode(pDev, QOR_CMD, pDrvCtrl->CTRL.addr_bytes_mode);
    if (ret == ERROR)
    {
		QSPI_DBG2("ctrl_%d: vxQspiFlash_SetMode fail! \n", ctrl_x, 2,3,4,5,6);
        return ERROR;
    }

	QSPI_DBG1("addr_bytes_mode:%d \n", pDrvCtrl->CTRL.addr_bytes_mode, 2,3,4,5,6);
	
	
	/*
	bit22~19:	R/W 主机模式下波特率分频(2到32)
	--------
	SPI波特率 = (主机 reference clock) / BD
	BD= 4'h0  /2
		4'h1  /4
		4'h2  /6 
		…
		4'hf  /32
	*/	
	/* 2->100M; */
	/* 8->25M; */
	/* 4->50M	*/
	/* 200MHz/bdr = x MHz*/
	bdr = gQspi_Ctrl_Params.baudRate;   /* 4*/
	vxQspiCtrl_Set_BaudRate(pDev, QSPIPS_BAUD(bdr));
	
	/**/
	/*  .sampleDelay = 2,*/
	/**/
	sampleDelay = gQspi_Ctrl_Params.sampleDelay;  /* 2*/
	vxQspiCtrl_Set_CaptureDelay(pDev, sampleDelay);	

	/**/
	/* unlock flash protect*/
	/*	*/
    ret = vxQspiFlash_Get_RegStatus1(pDev);
    if (ret & 0x1c)
    {    
		vxQspiFlash_Unlock_Flash(pDev);
    }

	/**/
	/* set flash quad-line mode*/
	/**/
	if (vxQspiFlash_Get_QuadMode(pDev) == FLASH_1_LINE_MODE)
	{
		printf("flash interface: 1-line mode! \n");
		
		if (FMSH_SUCCESS == vxQspiFlash_Set_QuadMode(pDev))
		{
			printf("flash interface: set 4-line mode ok! \n");
		}
		/*
		*/
	}
	else
	{
		printf("flash interface: 4-line mode! \n");
	}

	/* for Micron: 256M bit flash, and 4_bytes_addr mode*/
	if (pDrvCtrl->FLASH.flagStatus & QSPI_FSR)
	{
		if (pDrvCtrl->CTRL.addr_bytes_mode == ADDR_4_BYTES_MODE)
		{
			vxQspiFlash_Enter_4BAddr(pDev);
		}
		else  /* 3_bytes_addr mode*/
		{
			vxQspiFlash_Exit_4BAddr(pDev);
		}
	}
	
    pDrvCtrl->initDone = TRUE;	
	pDrvCtrl->init_flag = QSPI_CTRL_INIT_OK;
	
	return;
}

#define ______INIT_END______

/*******************************************************************************
*
* vxQspiInstInit2 - second level initialization routine of SPI controller
*
* This routine performs the second level initialization of the SPI controller.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxQspiInstInit2
    (
    VXB_DEVICE_ID pDev
    )
{
    FM_QSPI_DRV_CTRL * pDrvCtrl;

    /* check for valid parameter */
    /*VXB_ASSERT_NONNULL_V (pDev);*/

    pDrvCtrl = (FM_QSPI_DRV_CTRL *) pDev->pDrvCtrl;

    /* 
    The semSync semaphore is used to synchronize the QSPI transfer. 
	*/
    pDrvCtrl->semSync = semBCreate (SEM_Q_PRIORITY, SEM_FULL);
    if (pDrvCtrl->semSync == NULL)
    {
        QSPI_DBG (SPI_DBG_ERR, "semBCreate failed for semSync\n",
                 0, 0, 0, 0, 0, 0);
        return;
    }

    /* The muxSem semaphore is used to mutex accessing the controller. */
    /*pDrvCtrl->muxSem = semMCreate (SEM_Q_PRIORITY);*/
    pDrvCtrl->muxSem = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE);
    if (pDrvCtrl->muxSem == NULL)
    {
        QSPI_DBG (SPI_DBG_ERR, "semMCreate failed for muxSem\n",
                 0, 0, 0, 0, 0, 0);

        (void) semDelete (pDrvCtrl->semSync);
        pDrvCtrl->semSync = NULL;

        return;
    }
	
    /* 
    QSPI controller init 
	*/
    (void) vxQspiCtrl_Init(pDev);

    pDrvCtrl->initPhase = 2;
	
	return;
}

/*******************************************************************************
*
* vxQspiInstConnect - third level initialization
*
* This routine performs the third level initialization of the SPI controller
* driver.
*
* RETURNS: N/A
*
* ERRNO : N/A
*/

LOCAL void vxQspiInstConnect
    (
    VXB_DEVICE_ID       pDev
    )
{
    FM_QSPI_DRV_CTRL * pDrvCtrl;

    /* check for valid parameter */
    /*VXB_ASSERT_NONNULL_V (pDev);*/

    pDrvCtrl = (FM_QSPI_DRV_CTRL *) pDev->pDrvCtrl;

    /* connect and enable interrupt for non-poll mode */
    if (!pDrvCtrl->polling)
        {
        if (vxbIntConnect (pDev, 0, vxQspiIsr, pDev) != OK)
            {
            QSPI_DBG(SPI_DBG_ERR, "vxbIntConnect return ERROR \n",
                    1, 2, 3, 4, 5, 6);
            }
        (void) vxbIntEnable (pDev, 0, vxQspiIsr, pDev);
        }

    pDrvCtrl->initPhase = 3;

	return;
}

/*****************************************************************************
*
* vxQspiInstUnlink - VxBus unlink handler
*
* This function shuts down a SPI controller instance in response to an
* an unlink event from VxBus. This may occur if our VxBus instance has
* been terminated, or if the SPI driver has been unloaded.
*
* RETURNS: OK if device was successfully destroyed, otherwise ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxQspiInstUnlink
    (
    VXB_DEVICE_ID pDev,
    void *        unused
    )
{
    /* check if the pDev pointer is valid */
   /* VXB_ASSERT (pDev != NULL, ERROR)*/

    FM_QSPI_DRV_CTRL * pDrvCtrl = (FM_QSPI_DRV_CTRL *) pDev->pDrvCtrl;

    /*
     * The semaphore and interrupt resource are released here . The 
     * semaphore was created at phase 2 and interrupt was installed
     * at phase 3.
     */
    if (pDrvCtrl->initPhase >= 2)
    {
        (void)semTake (pDrvCtrl->muxSem, WAIT_FOREVER);

        /* 
        disconnect and disable interrupt
        */
        if (!pDrvCtrl->polling && pDrvCtrl->initPhase == 3)
        {
            (void) vxbIntDisable (pDev, 0, vxQspiIsr, pDev);

            if (vxbIntDisconnect (pDev, 0, vxQspiIsr, pDev) != OK)
            {
                QSPI_DBG(SPI_DBG_ERR, "vxbIntDisconnect return ERROR \n",
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
       /* fmshSpiDmaChanFree (pDev);*/
    }

    pDrvCtrl->initPhase = 0;

    pDev->pDrvCtrl = NULL;

    return (OK);
}



    



/*******************************************************************************
*
* vxQspiIsr - interrupt service routine
*
* This routine handles interrupts of SPI.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxQspiIsr
    (
    VXB_DEVICE_ID pDev
    )
{
    FM_QSPI_DRV_CTRL * pDrvCtrl;

    /* Check if the pDev pointer is valid */
    /*VXB_ASSERT (pDev != NULL, ERROR)*/

    pDrvCtrl = (FM_QSPI_DRV_CTRL *)pDev->pDrvCtrl;

    /* disable all QSPI interrupts */

    semGive (pDrvCtrl->semSync);

	return;
}

#endif



#if 1

#define __CTRL__

void vxQspiCtrl_Wr_CfgReg32(VXB_DEVICE_ID pDev, UINT32 offset, UINT32 value)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = NULL;
	UINT32 tmp32 = 0;
	
	pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	tmp32 = pDrvCtrl->CTRL.cfgBaseAddr;	
	
	FMQL_WRITE_32((tmp32 + offset), value);
	return;
}

UINT32 vxQspiCtrl_Rd_CfgReg32(VXB_DEVICE_ID pDev, UINT32 offset)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = NULL;
	UINT32 tmp32 = 0;
	
	pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	tmp32 = pDrvCtrl->CTRL.cfgBaseAddr;	
	
	return FMQL_READ_32(tmp32 + offset);
}

void vxQspiCtrl_Reset(int ctrl_x)
{    
	UINT32 tmp32 = 0;
	
    switch (ctrl_x)
    {
    case QSPI_CTRL_0:
        tmp32 = slcr_read(SLCR_QSPI0_CTRL) | (QSPI_APB_RST_MASK | QSPI_AHB_RST_MASK | QSPI_REF_RST_MASK);
		slcr_write(SLCR_QSPI0_CTRL, tmp32);
		sysUsDelay(10);	
        tmp32 = slcr_read(SLCR_QSPI0_CTRL) & (~(QSPI_APB_RST_MASK | QSPI_AHB_RST_MASK | QSPI_REF_RST_MASK));
		slcr_write(SLCR_QSPI0_CTRL, tmp32);
		break;
		
    case QSPI_CTRL_1:
        tmp32 = slcr_read(SLCR_QSPI1_CTRL) | (QSPI_APB_RST_MASK | QSPI_AHB_RST_MASK | QSPI_REF_RST_MASK);
		slcr_write(SLCR_QSPI1_CTRL, tmp32);
		sysUsDelay(10);	
        tmp32 = slcr_read(SLCR_QSPI1_CTRL) & (~(QSPI_APB_RST_MASK | QSPI_AHB_RST_MASK | QSPI_REF_RST_MASK));
		slcr_write(SLCR_QSPI1_CTRL, tmp32);
		break;
    }
	
	return;
}

int vxQspiCtrl_Wait_ExeOk(VXB_DEVICE_ID pDev, UINT32 offset, UINT32 regMask, UINT8 ok_flag)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    UINT32 tmp32 = 0;;
    int timeout = 100;
    
    do
	{
        sysUsDelay(1);
		
        tmp32 = vxQspiCtrl_Rd_CfgReg32(pDev, offset) & regMask; 
		switch (ok_flag)
		{
		case OK_FLAG_1:
			if (tmp32 == regMask)
			{
				/*printf("OK_FLAG_1:timout:%d \n", timeout);*/
				return OK;
			} 
			break;
			
		case OK_FLAG_0:
			if (tmp32 == 0x00)
			{
				/*printf("OK_FLAG_0:timout:%d \n", timeout);*/
				return OK;
			}			
			break;
		}
		
        timeout--;
    } while (timeout > 0);

	return ERROR;
}

int vxQspiCtrl_Wait_BusIdle(VXB_DEVICE_ID pDev)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;

    UINT32 tmp32 = 0;
    /*int timeout = 100;*/
    int timeout = 500;
    
    do
	{
        sysUsDelay(1);
		
		/*
		offset: 0x00 
		------------
		bit31: (RO) Serial Interface and QSPI pipeline is IDLE
		*/
        tmp32 = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_CFG_OFFSET) & 0x80000000; 
		
        if (tmp32 == 0x80000000)
		{
            return OK;
        }   
		
        timeout--;
    } while (timeout > 0);
		
	return ERROR;
}

int vxQspiCtrl_CmdExecute(VXB_DEVICE_ID pDev, UINT32 ctrl_cmd)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    int ret;
	UINT32 cmd = 0;
	
    /* 
    Read configuration Register command(0x35) to device (1B) 
	*/
	/*
	Flash Command Control Register:
	
	Offset: 0x90	
	-------------
	Bit     R/W Description Reset
	31:24	R/W 命令操作符:	命令操作符应该在触发前设置完成。	8'h00
	23	    R/W 读数据使能: 	如果bit[31:24]需要从器件设备上读取数据	1'h0
	22:20	R/W 读数据个数: 	最多可以读取8byte数据(0 对应1byte… 7对应8byte)
				如果Flash Command Control Register bit[2]使能，该字段无效。
				读取数据大小依据Flash Command Control Memory Register 的bit[15:8]	3'h0
	19	    R/W 命令地址使能: 	当bit[31:24]需要传输地址时，置“1” 1'h0
	18	    R/W Mode 位使能。: 	该位被置“1”，mode位按照Mode Bit Configuration Register，并在地址被传输后被传送出去	1'h0
	17:16	R/W 地址自己数，设置需要传输的字节位宽
				2'b00	1byte
				2'b01	2byte
				2'b10	3byte
				2'b11	4byte	2'h00
	15		    写数据使能:  	当bit[31:24]需要写数据时，该位置“1”	1'h0
	14:12		写入数据字节数:  	最多可以写8byte数据(0 对应1byte… 7对应8byte)	3'h0
	11:7		Dummy时钟数 :	当bit[31:24]需要Dummy时钟，设置位置	5'h00
	6:3 	RO	Reserved	4'h0
	2		R/W STIG存储器bank使能 1'h0
	1		RO	STIG命令正在执行	1'h0
	0		WO	执行该命令 n/a
	*/	
	cmd = ctrl_cmd;

	/**/
	/* set val to cmd_ctrl_reg with no-trig, at first*/
	/*	*/
    cmd &= ~(UINT32)0x01;
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_FCCR_OFFSET, cmd);
	
	/*
	wait pipeline idle status
	*/
    ret = vxQspiCtrl_Wait_BusIdle(pDev);
    if (ret == ERROR)
	{
        return ERROR;
    }
	
	/**/
	/* trig cmd_ctrl_reg*/
	/*	*/
    cmd = cmd | 0x01;  /* bit0 WO	执行该命令 n/a*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_FCCR_OFFSET, cmd);
	
	/*
	wait cmd execute ok
	*/
    ret = vxQspiCtrl_Wait_ExeOk(pDev, QSPIPS_FCCR_OFFSET, QSPIPS_REG_FCCR_INPROGRESS_MASK, OK_FLAG_0);
    if (ret == ERROR)
	{
        return ERROR;
    }
    
	/*
	wait pipeline idle status
	*/
    ret = vxQspiCtrl_Wait_BusIdle(pDev);
    if (ret == ERROR)
	{
        return ERROR;
    }
    
    return OK;
}


void vxQspiCtrl_Set_BaudRate(VXB_DEVICE_ID pDev, int baud_rate)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    UINT32 cfgReg = 0;
	
    cfgReg = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_CFG_OFFSET);
	
    cfgReg &= ~QSPIPS_BAUD_MASK;
    cfgReg |= (baud_rate << QSPIPS_BAUD_SHIFT) & QSPIPS_BAUD_MASK;

	/*
	bit22~19:	R/W 主机模式下波特率分频(2到32)
	--------
	SPI波特率 = (主机 reference clock) / BD
	BD= 4'h0  /2
		4'h1  /4
		4'h2  /6 
		…
		4'hf  /32
	*/	
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_CFG_OFFSET, cfgReg);

	return;
}

int vxQspiCtrl_Clr_RcvFifo(VXB_DEVICE_ID pDev)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
	int dataCnt = 0;	
	UINT8 * pBuf8 = (UINT8 *)pDrvCtrl->CTRL.fifoBaseAddr;
	
	/*	Read  Watermark */
	dataCnt = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_SFLR_OFFSET) * 4;
	if (dataCnt > 0)
	{		
		/* 	Read data from SRAM 	*/
		memcpy((UINT8*)pDrvCtrl->CTRL.rcvBuf, pBuf8, dataCnt);
	}

	return OK;
}

#if 1

/*****************************************************************************
* This function sets pDev controller address bytes number.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void vxQspiCtrl_Set_BytesMode(VXB_DEVICE_ID pDev, int addrSize)
{
    UINT32 cfgReg = 0;

	/*
	offset:0x14 
	-----------
		bit3~0: R/W Number of address bytes
				A value of 0 = 1 byte etc.
	*/
    cfgReg = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_DSCR_OFFSET);
	
    cfgReg &= ~QSPIPS_DS_ADDRSIZE_MASK;
    cfgReg |= (addrSize & QSPIPS_DS_ADDRSIZE_MASK);
	
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_DSCR_OFFSET, cfgReg);

	return;
}

/*
set flash_cmd mode
 & addr 4bytes mode
*/
int vxQspiCtrl_SetFlashMode(VXB_DEVICE_ID pDev, UINT8 flash_cmd, int bytes_mode)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
    UINT32 cfgReg = 0;
    
    if (bytes_mode == ADDR_3_BYTES_MODE)   /* 3 bytes_mode*/
    {
    	switch (flash_cmd)
    	{
    	case READ_CMD: 
			cfgReg = 0x00000003; 
			break;
    	case FAST_READ_CMD: 
			cfgReg = 0x0800000B; 
			break;
    	case DOR_CMD: 
			cfgReg = 0x0801003B; 
			break;
    	case QOR_CMD: 
			cfgReg = 0x0802006B; 
			break;
		
    	case DIOR_CMD:   
        	if (pDrvCtrl->FLASH.maker == SPI_MICRON_ID)
            {	
            	cfgReg = 0x041110BB;
			}
        	else
            {	
            	cfgReg = 0x001110BB;   
			}
        	break;
			
    	case QIOR_CMD:
        	if (pDrvCtrl->FLASH.maker == SPI_MICRON_ID)
           	{	
           		cfgReg = 0x081220EB;
			}
        	else
            {	
            	cfgReg = 0x041220EB;  
			} 
        	break;
			
    	default: 
			cfgReg = 0x0800000B; 
			break;
		}
		
        /* set program */
        vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_DWIR_OFFSET, 0x00000002);
		
    }
    else if (bytes_mode == ADDR_4_BYTES_MODE)  /* 4 bytes_mode*/
    {
    	switch (flash_cmd)
    	{
    	case READ_CMD: 
			cfgReg = 0x00000013; 		
			break;
    	case FAST_READ_CMD: 
			cfgReg = 0x0800000C; 		
			break;
    	case DOR_CMD: 
			cfgReg = 0x0801003C; 
			break;
    	case QOR_CMD: 
			cfgReg = 0x0802006C; 
			break;
		
    	case DIOR_CMD:   
        	if (pDrvCtrl->FLASH.maker == SPI_MICRON_ID)
            {	
            	cfgReg = 0x041110BC;
			}
        	else
            {	
            	cfgReg = 0x001110BC;   
			}
        	break;
			
    	case QIOR_CMD:
        	if (pDrvCtrl->FLASH.maker == SPI_MICRON_ID)
           	{	
           		cfgReg = 0x081220EC;
			}
        	else
            {	
            	cfgReg = 0x041220EC;  
			} 
        	break;
			
    	default: 
			cfgReg = 0x0800000C; 
			break;
		}
		
        /*set program */
        vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_DWIR_OFFSET, 0x00000012);
    }   
    else
    {
        return ERROR;
    }

    vxQspiCtrl_Set_BytesMode(pDev, (bytes_mode - 1)); 
	
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_DRIR_OFFSET, cfgReg);
    
    return OK;
}


/*****************************************************************************
* This function sets pDev controller read capture delay cycles.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void vxQspiCtrl_Set_CaptureDelay(VXB_DEVICE_ID pDev, int cycles)
{
	UINT32 cfgReg = 0;
	/*
	offset-0x10 bit4~1: R/W 
	        Delay the read data capturing logic 
	        by the programmable number of ref_clk cycles
	*/
	cfgReg = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_RDCR_OFFSET);
	
	cfgReg &= ~QSPIPS_RC_CAPTURE_DELAY_MASK;
	cfgReg |= (cycles << QSPIPS_RC_CAPTURE_DELAY_SHIFT) & QSPIPS_RC_CAPTURE_DELAY_MASK;
	
	vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_RDCR_OFFSET, cfgReg);

	return;
}


void vxQspiCtrl_Set_3B_AddrMode(VXB_DEVICE_ID pDev)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	int ret = 0;
	
    pDrvCtrl->CTRL.addr_bytes_mode = ADDR_3_BYTES_MODE;
	
	ret = vxQspiCtrl_SetFlashMode(pDev, QOR_CMD, pDrvCtrl->CTRL.addr_bytes_mode);
    if (ret == ERROR)
    {
		QSPI_DBG2("ctrl_%d: vxQspiFlash_SetMode fail! \n", pDrvCtrl->CTRL.ctrl_x, 2,3,4,5,6);
        return ERROR;
    }

	QSPI_DBG2("addr_bytes_mode:%d \n", pDrvCtrl->CTRL.addr_bytes_mode, 2,3,4,5,6);
	return;
}

void vxQspiCtrl_Set_4B_AddrMode(VXB_DEVICE_ID pDev)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	int ret = 0;
	
    pDrvCtrl->CTRL.addr_bytes_mode = ADDR_4_BYTES_MODE;
	
	ret = vxQspiCtrl_SetFlashMode(pDev, QOR_CMD, pDrvCtrl->CTRL.addr_bytes_mode);
    if (ret == ERROR)
    {
		QSPI_DBG2("ctrl_%d: vxQspiFlash_SetMode fail! \n",pDrvCtrl->CTRL.ctrl_x, 2,3,4,5,6);
        return ERROR;
    }

	QSPI_DBG2("addr_bytes_mode:%d \n", pDrvCtrl->CTRL.addr_bytes_mode, 2,3,4,5,6);
	return;
}

void test_set_3b_addrmode(void)
{
	VXB_DEVICE_ID pDev;
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, 0);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", 0);
        return ERROR;
    }
	
	vxQspiCtrl_Set_3B_AddrMode(pDev);
	return;
}

void test_set_4b_addrmode(void)
{
	VXB_DEVICE_ID pDev;
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, 0);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", 0);
        return ERROR;
    }
	
	vxQspiCtrl_Set_4B_AddrMode(pDev);
	return;
}

#endif

#endif


#if 1

#define __FLASH__

UINT8 vxQspiFlash_Get_Reg8(VXB_DEVICE_ID pDev, UINT8 flash_cmd)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    UINT32 ctrl_cmd = 0;
    UINT8 value = 0;
    
    /*ctrl_cmd = 0x00800001 | (flash_cmd << 24);*/
    ctrl_cmd = 0x00800000 | ((UINT32)flash_cmd << 24);
	
    /* 
    Read Register command from device (1B) 
	*/	
    vxQspiCtrl_CmdExecute(pDev, ctrl_cmd);   
	
    value = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_FCRDRL_OFFSET) & 0xFF;
    return value;
} 

int vxQspiFlash_Set_Reg8(VXB_DEVICE_ID pDev, UINT8 flash_cmd, UINT8 value)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    UINT32 ctrl_cmd = 0;
    int ret = 0;
    
    /*ctrl_cmd = 0x00008001 | (flash_cmd << 24);*/
    ctrl_cmd = 0x00008000 | ((UINT32)flash_cmd << 24);
	
    /* 
    Set value to be sent 
	*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_FCWDRL_OFFSET, (UINT32)value);
    
    ret = vxQspiFlash_WREN_Cmd(pDev);
    if (ret == ERROR)
	{
        return ERROR;
    }
    
    /* 
    Send command to device 
	*/
    ret = vxQspiCtrl_CmdExecute(pDev, ctrl_cmd);
    if (ret == ERROR)
	{
        return ERROR;
    }
    
    /*ret = vxQspiFlash_Wait_WIP_Ok(pDev);	*/
	ret = vxQspiFlash_Wait_FlashReady(pDev, 1500);
    if (ret == ERROR)
	{
        return ERROR;
    }
    
    return OK;
}


UINT16 vxQspiFlash_Get_Reg16(VXB_DEVICE_ID pDev, UINT8 flash_cmd)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    int ret;
    UINT32 ctrl_cmd = 0;
    UINT16 tmp16 = 0;
    
    /*ctrl_cmd = 0x00900001 | (flash_cmd << 24);*/
    ctrl_cmd = 0x00900000 | ((UINT32)flash_cmd << 24);
	
    /* 
    Read Register command from device (2B)
    */
    ret = vxQspiCtrl_CmdExecute(pDev, ctrl_cmd);  
    if (ret == ERROR)
	{
        return ERROR;
    }
	
    tmp16 = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_FCRDRL_OFFSET) & 0xFFFF;
    return tmp16;
}

int vxQspiFlash_Set_Reg16(VXB_DEVICE_ID pDev, UINT8 flash_cmd, UINT8 high_val8, UINT8 low_val8)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    int ret;
    UINT32 ctrl_cmd = 0;
    
    /*ctrl_cmd = 0x00009001 | (flash_cmd << 24);*/
    ctrl_cmd = 0x00009000 | ((UINT32)flash_cmd << 24);
	
    /* Set value to be sent*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_FCWDRL_OFFSET, ((high_val8 << 8) | low_val8));  
	
    ret = vxQspiFlash_WREN_Cmd(pDev);
    if (ret == ERROR)
	{
        return ERROR;
    }
	
    /* Send command to device*/
    ret = vxQspiCtrl_CmdExecute(pDev, ctrl_cmd);
    if (ret == ERROR)
	{
        return ERROR;
    }
	
   /* ret = vxQspiFlash_Wait_WIP_Ok(pDev); */
    ret = vxQspiFlash_Wait_WIP_Ok(pDev, 1500); 
    if (ret == ERROR)
	{
        return ERROR;
    }
    
    return OK;
}

UINT8 vxQspiFlash_Get_RegStatus1(VXB_DEVICE_ID pDev)
{
    int ret = 0;
	
    /* 
    Send Read Status1 Register command(0x05) to device 
	*/
    /*ret = vxQspiCtrl_CmdExecute(pDev, 0x05800001);*/
    ret = vxQspiCtrl_CmdExecute(pDev, 0x05800000);
    /*
    if (ret == ERROR)
	{
        return ERROR;
    }
	*/
    
    return vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_FCRDRL_OFFSET) & 0xFF;
} 

UINT8 vxQspiFlash_Get_FlagStatus(VXB_DEVICE_ID pDev)
{
	UINT8 ret = 0;
	
    /*Send Read Flag Status Register command(0x70) to device*/
    vxQspiCtrl_CmdExecute(pDev, 0x70800000);
	
    ret = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_FCRDRL_OFFSET) & 0xFF;
	return ret;
} 

void vxQspiFlash_Clr_FlagStatus(VXB_DEVICE_ID pDev)
{
    vxQspiCtrl_CmdExecute(pDev, 0x50000000);
	return;
} 

int vxQspiFlash_WREN_Cmd(VXB_DEVICE_ID pDev) 
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    int ret = 0;
    UINT8 status;
    UINT32 timeout = 1500; 
	
    /* 
    Send WREN(0x06) Command 
	*/
    /*ret = vxQspiCtrl_CmdExecute(pDev, 0x06000001);    	*/
    ret = vxQspiCtrl_CmdExecute(pDev, 0x06000000);    	
    if (ret == ERROR)
	{
        return ERROR;
    }
	
   	do
	{     
        /*sysMsDelay(1);*/
        vxbMsDelay(1);
		
        /* 
        poll status.wel value 
		*/
        status = vxQspiFlash_Get_RegStatus1(pDev);
        if ((status & WEL_MASK) == WEL_MASK)
		{
            return OK;
        }
		
        timeout--;
    } while (timeout > 0);
		
	return ERROR;
}

/*
flash cmd:
----------
SR1: 
	bit0 WIP Write in Progress
*/
int vxQspiFlash_Wait_WIP_Ok(VXB_DEVICE_ID pDev, int time_out) 
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
	UINT8 tmp8;
    int timeout = time_out;
    
    /* 
    Poll Status Register1 
	*/
   	do
	{
        /*sysMsDelay(1);*/
        vxbMsDelay(1);
		
        tmp8 = vxQspiFlash_Get_RegStatus1(pDev) & BUSY_MASK;	
        tmp8 = ~tmp8;
        tmp8 &= BUSY_MASK;
        if(tmp8 == BUSY_MASK) /* 1-Device Busy, 0-Ready Device is in standby */
		{
            return OK;
        }   
		
        timeout--;
    } while (timeout > 0);
    
	return ERROR;
}

int vxQspiFlash_Wait_FSR_Ok(VXB_DEVICE_ID pDev, int time_out)
{ 
    UINT8 status;
    int det = 2;
    int cnt = 0;
    
    while (1)
	{
        status = vxQspiFlash_Get_FlagStatus(pDev);
        if ((status & 0x80)== 0x80)
		{
            det--;
            if (det == 0)
            {    
            	return OK;
            }
        }    
		
        vxbMsDelay(1);
		
        cnt++;
        if (cnt > time_out)
		{
            return ERROR;
        }
    }
}

int vxQspiFlash_Wait_FlashReady(VXB_DEVICE_ID pDev, int time_out)
{ 
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
    int status;
    
    /* Poll Status Register1*/
    status = vxQspiFlash_Wait_WIP_Ok(pDev, time_out);
    if (status == ERROR)
    {    
    	return ERROR;
    }

	/**/
    /* Poll Flag Status Register, only for such as: MT25Q512 */
    /**/
    if (pDrvCtrl->FLASH.flagStatus & QSPI_FSR)
    {
        status = vxQspiFlash_Wait_FSR_Ok(pDev, time_out);
        if (status == ERROR)
		{	 
			return ERROR;
		}
        
        status = vxQspiFlash_Get_FlagStatus(pDev);
        if (status & 0x32)
        {
            vxQspiFlash_Clr_FlagStatus(pDev);
            return ERROR;
        }
    }
    
    return OK;
}

UINT32 vxQspiFlash_ReadId(VXB_DEVICE_ID pDev)
{
	UINT32  tmp32 = 0;
	
    /* 
    Send RDID Command (4 bytes) 
	*/
    vxQspiCtrl_CmdExecute(pDev, 0x9FD00001);
	
    /*Get ID */
    tmp32 = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_FCRDRL_OFFSET);

	return tmp32;
}

void test_qspiflash_rd_id(void)
{
	VXB_DEVICE_ID pDev;
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, 0);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", 0);
        return ERROR;
    }
	
	printf("qpsiflash_ID: 0x%08X \n", vxQspiFlash_ReadId(pDev));
	
	return;
}

UINT32 vxQspiFlash_ReadId_2(VXB_DEVICE_ID pDev, UINT32 * buf)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;

	UINT32  tmp32 = 0;	
	UINT32  tmp32_2 = 0;
	
    /* 
    Send RDID Command (4 bytes) 
	*/
    /*vxQspiCtrl_CmdExecute(pDev, 0x9FD00001);	*/
    vxQspiCtrl_CmdExecute(pDev, 0x9FF00000);  /* more bytes*/
    
    /*
    Get ID 
    */
    tmp32 = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_FCRDRL_OFFSET);	
    tmp32_2 = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_FCRDRU_OFFSET);

	buf[0] = tmp32;
	buf[1] = tmp32_2;
	
	return tmp32;
}

#if 1
/*
all id data_area data bytes:
*/
void vxQspiFlash_ReadId_3(VXB_DEVICE_ID pDev, UINT8* buf)
{
	UINT32 cfgReg = 0;
	UINT32 tmp32 = 0;

	/* read the setup val*/
	tmp32 = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_DRIR_OFFSET);
	
	/*
	from byte_idx_3 to byte_idx_last
	*/
	cfgReg = 0x9F;
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_DRIR_OFFSET, cfgReg);

	/* read data_area of id_area*/
	vxQspiFlash_Read_Page(pDev, 0, buf);

	/* replace the val*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_DRIR_OFFSET, tmp32);
	
	return;
}

void test_qspiflash_rd_ids(void)
{	
	UINT32 tmp32 = 0;
	UINT8 pBuf8[0x100] = {0};
	int i = 0, j = 0;
	
	VXB_DEVICE_ID pDev;
	FM_QSPI_DRV_CTRL* pDrvCtrl;
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, 0);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", 0);
        return ERROR;
    }
	pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
	/*
	from byte_idx_3 to byte_idx_last
	*/
	vxQspiFlash_ReadId_3(pDev, (UINT8*)(&pBuf8[4]));
	
    /* 
    Get ID: first 4bytes data 
	*/
    tmp32 = vxQspiFlash_ReadId(pDev);
	/*printf("tmp32: 0x%08X \n", tmp32);*/
	
	if (pDrvCtrl->CTRL.addr_bytes_mode == ADDR_3_BYTES_MODE)
	{
		/* 
		first 3bytes data 
		*/
		pBuf8[0] = (UINT8)(tmp32  & 0x000000FF);	
		pBuf8[1] = (UINT8)((tmp32 & 0x0000FF00) >> 8);
		pBuf8[2] = (UINT8)((tmp32 & 0x00FF0000) >> 16);
		/*
		from byte_idx_3 to byte_idx_last
		*/
		for (i=4; i<0x100; i++)
		{
			pBuf8[i-1] = pBuf8[i];
		}
	}
	else
	{
		/* 
		first 4bytes data 
		*/
		pBuf8[0] = (UINT8)(tmp32  & 0x000000FF);	
		pBuf8[1] = (UINT8)((tmp32 & 0x0000FF00) >> 8);
		pBuf8[2] = (UINT8)((tmp32 & 0x00FF0000) >> 16);
		pBuf8[3] = (UINT8)((tmp32 & 0xFF000000) >> 24);
	}

	
#if 1
	for (j=0; j<0x100; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
#endif

	return;
}

/*
-> vxInit_Qspi 0
value = 0 = 0x0
-> 
-> test_qspiflash_rd_ids
0102194D 01805230 81FFFFFF FFFFFFFF  
51525902 00400053 46510027 36000006  
08081002 02030319 02010800 021F0010  
00FD0100 01FFFFFF FFFFFFFF FFFFFFFF  
50524931 33210201 00080001 03000007  
01414C54 32300010 53323546 4C323536  
5341473F 3F495230 8001F084 0885288A  
6475287A 6488040A 0100018C 06960123  
00230090 56060E46 4303130B 0C3B3C6B  
6CBBBCEB EC320300 00000000 00000004  
00020150 00FFFF00 08000800 08040002  
045A01FF FF000800 08000804 01020468  
02FFFF00 08000800 08040202 058502FF  
FF0008FF FFFFFFFF FFFFFF94 01109801  
009A2A05 0846430D 0EBDBEED EE320304  
01020201 03420004 02020401 06420104  
*/
#endif

int vxQspiFlash_ParseInfo_2(VXB_DEVICE_ID pDev, UINT32 * flashId)
{  
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
    UINT8 maker = 0, devSize = 0, bigSectMode = 0;
	int ret = OK;
	
    maker = flashId[0] & 0xFF;
    devSize = (flashId[0] >> 16) & 0xFF;
	
    bigSectMode = flashId[1] & 0xFF;  /* judge: 256K or 64K*/
	
    /* 
    Deduce flash make   
	*/
	switch (maker)
	{
	case SPI_MICRON_ID:
	case SPI_SPANSION_ID:
	case SPI_WINBOND_ID:
	case SPI_MACRONIX_ID:
	case SPI_ISSI_ID:
		
	case SPI_FMSH_ID:		
	case SPI_GD_ID:
		pDrvCtrl->FLASH.maker = maker;
		break;
	
	default:
        pDrvCtrl->FLASH.maker = SPI_WRONG_ID;
        ret = ERROR;
		break;
	}
    
    /* 
    Deduce flash Size 
	*/
    switch (devSize)
    {
   	case QSPIFLASH_SIZE_ID_256K: 
        pDrvCtrl->FLASH.devSize = QSPIFLASH_SIZE_256K;
		break;
    case QSPIFLASH_SIZE_ID_512K: 
        pDrvCtrl->FLASH.devSize = QSPIFLASH_SIZE_512K;
		break;
    case QSPIFLASH_SIZE_ID_1M:
        pDrvCtrl->FLASH.devSize = QSPIFLASH_SIZE_1M;
		break;
    case QSPIFLASH_SIZE_ID_2M: 
        pDrvCtrl->FLASH.devSize = QSPIFLASH_SIZE_2M;
		break;
    case QSPIFLASH_SIZE_ID_4M: 
        pDrvCtrl->FLASH.devSize = QSPIFLASH_SIZE_4M;
		break;
    case QSPIFLASH_SIZE_ID_8M:
        pDrvCtrl->FLASH.devSize = QSPIFLASH_SIZE_8M;
		break;
    case QSPIFLASH_SIZE_ID_16M:
        pDrvCtrl->FLASH.devSize = QSPIFLASH_SIZE_16M;
		break;
    case QSPIFLASH_SIZE_ID_32M:
        pDrvCtrl->FLASH.devSize = QSPIFLASH_SIZE_32M;
		break;
    case QSPIFLASH_SIZE_ID_64M: 
        pDrvCtrl->FLASH.devSize = QSPIFLASH_SIZE_64M;
		break;
    case QSPIFLASH_SIZE_ID_128M:
        pDrvCtrl->FLASH.devSize = QSPIFLASH_SIZE_128M;
		break;
    case QSPIFLASH_SIZE_ID_256M: 
        pDrvCtrl->FLASH.devSize = QSPIFLASH_SIZE_256M;
		break;

    case QSPIFLASH_SIZE_ID_512M:
    case MACRONIX_QSPIFLASH_SIZE_ID_512M: /* ISSI_QSPIFLASH_SIZE_ID_512M = MACRONIX_QSPIFLASH_SIZE_ID_512M */
        pDrvCtrl->FLASH.devSize = QSPIFLASH_SIZE_512M;
		break;

    case QSPIFLASH_SIZE_ID_1G:
    case MACRONIX_QSPIFLASH_SIZE_ID_1G: 
        pDrvCtrl->FLASH.devSize = QSPIFLASH_SIZE_1G;
		break;

	default:
        pDrvCtrl->FLASH.devSize = SPI_WRONG_ID;
        ret = ERROR;
		break;
    }

	/*
	00h (Uniform 256-kB sectors)
	01h (4-kB parameter sectors with uniform 64-kB sectors)
	*/
	switch (maker)
    {
    case SPI_MICRON_ID: 
        pDrvCtrl->FLASH.sectSize = QSPIFLASH_SECTOR_64K; /* 0x1 << 16;  // 64K bytes*/
        pDrvCtrl->FLASH.pageSize = QSPIFLASH_PAGE_256;  /* 0x1 << 8;   // 256 bytes*/
        
        if (pDrvCtrl->FLASH.devSize >= QSPIFLASH_SIZE_512M)
        {
			vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_WCCR_OFFSET, 0x022770);
			pDrvCtrl->FLASH.flagStatus |= QSPI_FSR;  /* qspi-flash flag_status */
			
			printf("Micron: N25Q512A \n");
        }
        break;
		
    case SPI_SPANSION_ID:		
		switch (bigSectMode)
		{
		case 0x00:
			pDrvCtrl->FLASH.sectSize = QSPIFLASH_SECTOR_256K;
			break;
			
		case 0x01:
			pDrvCtrl->FLASH.sectSize = QSPIFLASH_SECTOR_64K;
			break;
		}        
        pDrvCtrl->FLASH.pageSize = QSPIFLASH_PAGE_256;
        break;
		
    case SPI_WINBOND_ID: 
        pDrvCtrl->FLASH.sectSize = QSPIFLASH_SECTOR_64K;
        pDrvCtrl->FLASH.pageSize = QSPIFLASH_PAGE_256;
        break;
		
    case SPI_MACRONIX_ID: 
        pDrvCtrl->FLASH.sectSize = QSPIFLASH_SECTOR_64K;
        pDrvCtrl->FLASH.pageSize = QSPIFLASH_PAGE_256;
        break;
		
    case SPI_ISSI_ID: 
        pDrvCtrl->FLASH.sectSize = QSPIFLASH_SECTOR_64K;
        pDrvCtrl->FLASH.pageSize = QSPIFLASH_PAGE_256;
        break;
		
    case SPI_FMSH_ID: 
        pDrvCtrl->FLASH.sectSize = QSPIFLASH_SECTOR_64K;
        pDrvCtrl->FLASH.pageSize = QSPIFLASH_PAGE_256;
        break;
		
    case SPI_GD_ID: 
        pDrvCtrl->FLASH.sectSize = QSPIFLASH_SECTOR_64K;
        pDrvCtrl->FLASH.pageSize = QSPIFLASH_PAGE_256;
        break;
		
    default: 
        pDrvCtrl->FLASH.sectSize = QSPIFLASH_SECTOR_64K;
        pDrvCtrl->FLASH.pageSize = QSPIFLASH_PAGE_256;
        break;
    }
	
	pDrvCtrl->FLASH.flash_ID1 = flashId[0];
	pDrvCtrl->FLASH.flash_ID2 = flashId[1];
	
    return ret;
}


/***************
* pDev flash reset
*
***************/
int vxQspiFlash_Reset_Flash(VXB_DEVICE_ID pDev)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
    int ret = 0;

    if (pDrvCtrl->FLASH.maker == SPI_SPANSION_ID)
    {
		/* MBR Mode Bit Reset FF*/
        ret = vxQspiCtrl_CmdExecute(pDev, 0xFF000001); 
        if (ret != OK) 
        {
            return ERROR;
        }
		
		/* RESET Software Reset F0*/
        ret = vxQspiCtrl_CmdExecute(pDev, 0xF0000001); 
        if (ret != OK) 
        {
            return ERROR;
        }
    }
	else  /* all other flash type*/
	{
        ret = vxQspiCtrl_CmdExecute(pDev, 0x66000001); 
        if (ret != OK) 
        {
            return ERROR;
        }
		
        ret = vxQspiCtrl_CmdExecute(pDev, 0x99000001); 
        if (ret != OK) 
        {
            return ERROR;
        }
	}
    
    vxbMsDelay(1);
	
    ret = vxQspiFlash_Wait_FlashReady(pDev, 1500);
    if(ret != OK) 
    {
        return ERROR;
    }
	
	pDrvCtrl->FLASH.seg_flag = 0;
	
    return OK;
}


/*******************************
* pDev flash disable protect
*
*******************************/
int vxQspiFlash_Unlock_Flash(VXB_DEVICE_ID pDev)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
    int err = OK;
    UINT8 status = 0, cfg = 0;

	if (pDrvCtrl->FLASH.unlock == 1)
	{
		return OK;
	}
	
    switch (pDrvCtrl->FLASH.maker)
	{
	case SPI_SPANSION_ID:
        cfg = vxQspiFlash_Get_Reg8(pDev, 0x35);     /* SPANSION_RDCR_CMD*/
        err = vxQspiFlash_Set_Reg16(pDev, 0x01, cfg, 0x00);    
		break;
		
	case SPI_ISSI_ID:
	case SPI_MACRONIX_ID:
        status = vxQspiFlash_Get_Reg8(pDev, 0x05);  /* RDSR1_CMD*/
        status &= 0x40;
        err = vxQspiFlash_Set_Reg8(pDev, 0x01, status);   
		break;

	default:
        err = vxQspiFlash_Set_Reg8(pDev, 0x01, 0x00);  
		break;		
	}

    if (err == ERROR) 
    {
        return ERROR;
    }
	
	pDrvCtrl->FLASH.unlock = 1;  /* unlock ok	*/
    return OK;
}

/*
flash is single-line mode, in normal
------------
4-line mode: FLASH_4_LINE_MODE
1-line mode: FLASH_1_LINE_MODE
*/
int vxQspiFlash_Get_QuadMode(VXB_DEVICE_ID pDev)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	int err = 0, ret = 0;
	UINT8 status = 0;
	UINT8 status1 = 0, status2 = 0;
	UINT8 cfg = 0; 

    switch (pDrvCtrl->FLASH.maker)
	{
		case SPI_SPANSION_ID:
	    {
	        status = vxQspiFlash_Get_Reg8(pDev, RDSR1_CMD); /*0x05*/
	        cfg = vxQspiFlash_Get_Reg8(pDev, SPANSION_RDCR_CMD); /*0x35*/
	        
	        if ((cfg & SPANSION_QUAD_EN_MASK) == 0)
	        {
				ret = FLASH_1_LINE_MODE;
			}
			else
			{
				ret = FLASH_4_LINE_MODE;
			}
	    }
		break;
		
    case SPI_MICRON_ID:
	    {
			/*no need to operate			*/
			ret = FLASH_4_LINE_MODE;  /* default: 4-line mode*/
	    }    
		break;
	
    case SPI_MACRONIX_ID:
	    {
	        status = vxQspiFlash_Get_Reg8(pDev, RDSR1_CMD); /*0x05*/
	        if ((status & MACRONIX_QUAD_EN_MASK) == 0)
	        {
				ret = FLASH_1_LINE_MODE;
			}
			else
			{
				ret = FLASH_4_LINE_MODE;
			}
	    }
		break;
    
    case SPI_ISSI_ID:
	    {   
	        status = vxQspiFlash_Get_Reg8(pDev, RDSR1_CMD); /*0x05   */
	        if ((status & ISSI_QUAD_EN_MASK) == 0)
	        {
				ret = FLASH_1_LINE_MODE;
			}
			else
			{
				ret = FLASH_4_LINE_MODE;
			}
	    }
		break;
    
    case SPI_WINBOND_ID:
	    {
	        status1 = vxQspiFlash_Get_Reg8(pDev, RDSR1_CMD); /*0x05*/
	        status2 = vxQspiFlash_Get_Reg8(pDev, WINBOND_RDSR2_CMD); /*0x35     */
	        if ((status2 & WINBOND_QUAD_EN_MASK) == 0)
	        {
				ret = FLASH_1_LINE_MODE;
			}
			else
			{
				ret = FLASH_4_LINE_MODE;
			}
	    }
		break;
    
    case SPI_FMSH_ID:
	    {
	        status2 = vxQspiFlash_Get_Reg8(pDev, FMSH_RDSR2_CMD); /*0x35*/
	        if ((status2 & FMSH_QUAD_EN_MASK) == 0)
	        {
				ret = FLASH_1_LINE_MODE;
			}
			else
			{
				ret = FLASH_4_LINE_MODE;
			}
	    }
		break;
 
    case SPI_GD_ID:
	    {
	        status2 = vxQspiFlash_Get_Reg8(pDev, GD_RDSR2_CMD); /*0x35*/
	        if ((status2 & GD_QUAD_EN_MASK) == 0)
	        {
				ret = FLASH_1_LINE_MODE;
			}
			else
			{
				ret = FLASH_4_LINE_MODE;
			}
	    }
		break;
	
    default:
		ret = 0;
		break;
    }

	return ret;
}


/*
flash is single-line mode, in normal
------------
4-line mode: FLASH_4_LINE_MODE
1-line mode: FLASH_1_LINE_MODE
*/
int vxQspiFlash_Set_QuadMode(VXB_DEVICE_ID pDev)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	int err = 0, ret = OK;
	UINT8 status = 0;
	UINT8 status1 = 0, status2 = 0;
	UINT8 cfg = 0; 

    switch (pDrvCtrl->FLASH.maker)
	{
		case SPI_SPANSION_ID:
	    {
	        status = vxQspiFlash_Get_Reg8(pDev, RDSR1_CMD); /*0x05*/
	        cfg = vxQspiFlash_Get_Reg8(pDev, SPANSION_RDCR_CMD); /*0x35*/
	        
	        if ((cfg & SPANSION_QUAD_EN_MASK) == 0)
	        {
	            cfg |= (UINT8)SPANSION_QUAD_EN_MASK;
	            err = vxQspiFlash_Set_Reg16(pDev, WRR_CMD, cfg, status);  /*0x01*/
	            if (err)
	            {
	                ret = ERROR;
	            }  
	        }	
	    }
		break;
		
    case SPI_MICRON_ID:
	    {
			/*no need to operate, default 4-line mode*/
	    }    
		break;
	
    case SPI_MACRONIX_ID:
	    {
	        status = vxQspiFlash_Get_Reg8(pDev, RDSR1_CMD); /*0x05*/
	        if ((status & MACRONIX_QUAD_EN_MASK) == 0)
	        {
	            status |= (UINT8)MACRONIX_QUAD_EN_MASK;
	            err = vxQspiFlash_Set_Reg8(pDev, WRR_CMD, status);/*0x01*/
	            if (err)
	            {
	                ret = ERROR;
	            }  
	        }  
	    }
		break;
    
    case SPI_ISSI_ID:
	    {   
	        status = vxQspiFlash_Get_Reg8(pDev, RDSR1_CMD); /*0x05   */
	        if ((status & ISSI_QUAD_EN_MASK) == 0)
	        {
	            status |= (UINT8)ISSI_QUAD_EN_MASK;
	            err = vxQspiFlash_Set_Reg8(pDev, WRR_CMD, status); /*0x01*/
	            if(err)
	            {
	                ret = ERROR;
	            } 
	        }    
	    }
		break;
    
    case SPI_WINBOND_ID:
	    {
	        status1 = vxQspiFlash_Get_Reg8(pDev, RDSR1_CMD); /*0x05*/
	        status2 = vxQspiFlash_Get_Reg8(pDev, WINBOND_RDSR2_CMD); /*0x35     */
	        if ((status2 & WINBOND_QUAD_EN_MASK) == 0)
	        {
	            status2 |= (UINT8)WINBOND_QUAD_EN_MASK;
	            err = vxQspiFlash_Set_Reg16(pDev, WRR_CMD, status2, status1);/*0x01*/
	            if (err)
	            {
	                ret = ERROR;
	            }  
	        }
	    }
		break;
    
    case SPI_FMSH_ID:
	    {
	        status2 = vxQspiFlash_Get_Reg8(pDev, FMSH_RDSR2_CMD); /*0x35*/
	        if ((status2 & FMSH_QUAD_EN_MASK) == 0)
	        {
	            status2 |= (UINT8)FMSH_QUAD_EN_MASK;
	            err = vxQspiFlash_Set_Reg8(pDev, FMSH_WRSR2_CMD, status2);/*0x31*/
	            if (err)
	            {
	                ret = ERROR;
	            }
	        } 
	    }
		break;
 
    case SPI_GD_ID:
	    {
	        status2 = vxQspiFlash_Get_Reg8(pDev, GD_RDSR2_CMD); /*0x35*/
	        if ((status2 & GD_QUAD_EN_MASK) == 0)
	        {
	            status2 |= (UINT8)GD_QUAD_EN_MASK;
	            err = vxQspiFlash_Set_Reg8(pDev, GD_WRSR2_CMD, status2);/*0x31*/
	            if (err)
	            {
	                ret = ERROR;
	            }
	        } 
	    }
		break;
	
    default:
		ret = ERROR;
		break;
    }

	return ret;
}

#endif

#if 1

#define __3_bytes_addr_mode__

/*
3_bytes_address mode:
*/

/*
while 3_bytes_address mode:
---------------------------
1. max one segment: 16M bytes
2. two or four segment for one_flash_chip
3. offset > 16M bytes, must be remap
4. highAddr: 0x00 - 0 ~ 16M
             0x01 - 16 ~ 32M
             0x02 - 32 ~ 48M
             0x03 - 48 ~ 64M
*/
int vxQspiFlash_Set_Segment(VXB_DEVICE_ID pDev, UINT8 highAddr)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
    int err;
    UINT8 extAddr = 0, extAddrRb = 0;    
    
    switch (pDrvCtrl->FLASH.maker)
    {
    case SPI_SPANSION_ID:
    case SPI_ISSI_ID:
	    {        
	        /* bit7 is extadd id 0 to enable 3B address */
	        extAddr = highAddr & 0x7F;
	        /*err = vxQspiFlash_Set_Reg8(pDev, 0x17, extAddr); */
	        err = vxQspiFlash_Set_Reg8(pDev, 0x17, extAddr); 
	        if (err != 0)
	        {
	        	/*printf("vxQspiFlash_Set_Segment->vxQspiFlash_Set_Reg8 fail! \n");*/
	            return ERROR;
	        } 
			else
			{
				/*printf("vxQspiFlash_Set_Segment->vxQspiFlash_Set_Reg8 (0x%X-%d) ok! \n", highAddr, extAddr);*/
			}
	        /* readback & verify */
	        /*extAddrRb = vxQspiFlash_Get_Reg8(pDev, 0x16); */
	        extAddrRb = vxQspiFlash_Get_Reg8(pDev, 0x16); 
	        if(extAddrRb != extAddr)
	        {
	        	/*printf("vxQspiFlash_Set_Segment->vxQspiFlash_Get_Reg8 fail: 0x%X-0x%X(r) \n", extAddr, extAddrRb);*/
	            return ERROR;
	        }
			else
			{
				/*printf("vxQspiFlash_Set_Segment->vxQspiFlash_Get_Reg8: 0x%X-0x%X(r) ok \n", extAddr, extAddrRb);*/
			}
	    }
		break;
    
    case  SPI_MICRON_ID:
    case  SPI_MACRONIX_ID:
    case  SPI_WINBOND_ID: 
    case  SPI_FMSH_ID:
    case  SPI_GD_ID:
	    {        
	        extAddr = highAddr;	        
	        err = vxQspiFlash_Set_Reg8(pDev, 0xC5, extAddr);
	        if (err != 0)
	        {
	        	/*printf("2-vxQspiFlash_Set_Segment->vxQspiFlash_Set_Reg8 fail! \n");*/
	            return ERROR;
	        }	        
	        /* readback & verify */
	        extAddrRb = vxQspiFlash_Get_Reg8(pDev, 0xC8);
	        if(extAddrRb != extAddr)
	        {
	        	/*printf("2-vxQspiFlash_Set_Segment->vxQspiFlash_Get_Reg8 fail: 0x%X-0x%X(r) \n", extAddr, extAddrRb);*/
	            return ERROR;
	        }   
	    }
		break;

    default:
	    {
	        return ERROR;
	    }
		break;
    }
    
    return OK;
}

UINT8 vxQspiFlash_Get_Segment(VXB_DEVICE_ID pDev)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
    UINT32 ret;
    UINT8 status;
	UINT8 cfg;
	
    switch (pDrvCtrl->FLASH.maker)
    {
    case SPI_SPANSION_ID:
    case SPI_ISSI_ID:
        ret = vxQspiFlash_Get_Reg8(pDev, 0x16); 
		break;
    
    case  SPI_MICRON_ID:
    case  SPI_MACRONIX_ID:
    case  SPI_WINBOND_ID: 
    case  SPI_FMSH_ID:
    case  SPI_GD_ID:
        ret = vxQspiFlash_Get_Reg8(pDev, 0xC8);
		break;

    default:
	    ret = 0;
		break;
    }
    
    return ret;
}

int vxQspiFlash_Set_Seg_0_16M(VXB_DEVICE_ID pDev)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
	if (pDrvCtrl->FLASH.seg_flag == SEG_FLAG_0_16M)
	{
		/*return OK;*/
	}	
	pDrvCtrl->FLASH.seg_flag = SEG_FLAG_0_16M;
	
	return vxQspiFlash_Set_Segment(pDev, SEG_3B_ADDR_0_16M);
}

int vxQspiFlash_Set_Seg_16_32M(VXB_DEVICE_ID pDev)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
	if (pDrvCtrl->FLASH.seg_flag == SEG_FLAG_16_32M)
	{
		/*return OK;*/
	}	
	pDrvCtrl->FLASH.seg_flag = SEG_FLAG_16_32M;
	
	return vxQspiFlash_Set_Segment(pDev, SEG_3B_ADDR_16_32M);
}

int vxQspiFlash_Set_Seg_32_48M(VXB_DEVICE_ID pDev)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
	if (pDrvCtrl->FLASH.seg_flag == SEG_FLAG_32_48M)
	{
		return OK;
	}	
	pDrvCtrl->FLASH.seg_flag = SEG_FLAG_32_48M;
	
	return vxQspiFlash_Set_Segment(pDev, SEG_3B_ADDR_32_48M);
}

int vxQspiFlash_Set_Seg_48_64M(VXB_DEVICE_ID pDev)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
	if (pDrvCtrl->FLASH.seg_flag == SEG_3B_ADDR_48_64M)
	{
		return OK;
	}	
	pDrvCtrl->FLASH.seg_flag = SEG_3B_ADDR_48_64M;
	
	return vxQspiFlash_Set_Segment(pDev, SEG_3B_ADDR_48_64M);
}

void test_get_seg(void)
{
	UINT8 ret = 0;
	UINT32 id[2] = {0};
	
	VXB_DEVICE_ID pDev;
	FM_QSPI_DRV_CTRL* pDrvCtrl;
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, 0);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", 0);
        return ERROR;
    }
	pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
	ret = vxQspiFlash_Get_Segment(pDev);
    switch (pDrvCtrl->FLASH.maker)
    {
    case SPI_SPANSION_ID:
		printf("SPI_SPANSION_ID: ");
		break;
		
    case SPI_ISSI_ID:
		printf("SPI_ISSI_ID: ");
		break;
    
    case  SPI_MICRON_ID:
		printf("SPI_MICRON_ID: ");
		break;
	
    case  SPI_MACRONIX_ID:
		printf("SPI_MACRONIX_ID: ");
		break;
	
    case  SPI_WINBOND_ID: 
		printf("SPI_WINBOND_ID: ");
		break;
	
    case  SPI_FMSH_ID:
		printf("SPI_FMSH_ID: ");
		break;
	
    case  SPI_GD_ID:
		printf("SPI_GD_ID: ");
		break;

    default:
		printf("Unknow_ID!!! \n");
		break;
    }

	/* read id*/
	vxQspiFlash_ReadId_2(pDev, id);
	printf("0x%08X-%08X ", id[0], id[1]);
	
	printf("=0x%08X seg-(%d) \n\n", ret, ret&0x03);

	return;
}

void test_set_seg(int seg_idx)
{
	VXB_DEVICE_ID pDev;
	FM_QSPI_DRV_CTRL* pDrvCtrl;
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, 0);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", 0);
        return ERROR;
    }
	pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;

	switch (seg_idx)
	{
	case 0:
		vxQspiFlash_Set_Seg_0_16M(pDev);
		break;
	case 1:
		vxQspiFlash_Set_Seg_16_32M(pDev);
		break;
	case 2:
		vxQspiFlash_Set_Seg_32_48M(pDev);
		break;
	case 3:
		vxQspiFlash_Set_Seg_48_64M(pDev);
		break;
	}

	return;
}

#if 1
/*
in 3_bytes_addr mode:
1. logic idx: 0,1,2,3 ... (max-1)  [ max  = devSize/sectSize ]
2. phy_idx  : 0,1,2,3 ... (seg-1)  [ seg = 16M/sectSize      ]
3. logic -> phy:
   -------------
   logic: 0,1,2 ... (seg-1), | seg   ... (2*seg-1), | (2*seg) ... (3*seg-1), | (3*seg) ... (4*seg-1)
          -----------------  | -------------------  | ---------------------  | ---------------------
   phy  : 0,1,2 ... (seg-1), | 0,1,2 ...   (seg-1), | 0,1,2   ...   (seg-1), | 0,1,2   ...   (seg-1)
*/
int vxQspiFlash_SectIdx_LogtoPhy(VXB_DEVICE_ID pDev, int log_idx)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	int ret = 0;
	int seg = 0, phy_idx = 0;

	switch (pDrvCtrl->CTRL.addr_bytes_mode)
	{
	case ADDR_4_BYTES_MODE:		
		{
			phy_idx = log_idx;
		}
		break;
	
	case ADDR_3_BYTES_MODE:
		{
			/**/
			/* 16M / sectSize*/
			/**/
			seg = 0x1000000 / pDrvCtrl->FLASH.sectSize;  /* 16M / 64K = 256 sects*/
			/*printf("seg_num: %d (indirect_16M/64K) => ", seg);*/
			
			if ((0 <= log_idx) && (log_idx < seg))                /* first 16M area */
			{
				phy_idx = log_idx;
				ret = vxQspiFlash_Set_Seg_0_16M(pDev);
				
				/*printf("sect_log->phy: 0-seg (ret:%d) \n", ret);*/
			}
			else if ((seg <= log_idx) && (log_idx < (2*seg)))    /* second 16M area */
			{
				phy_idx = log_idx - 1*seg;
				ret = vxQspiFlash_Set_Seg_16_32M(pDev);
				
				/*printf("bank_map=>sect_log->phy: 1-seg (ret:%d) \n", ret);*/
			}
			else if (((2*seg) <= log_idx) && (log_idx < (3*seg))) /* third 16M area */
			{
				phy_idx = log_idx - 2*seg;
				ret = vxQspiFlash_Set_Seg_32_48M(pDev);
				
				/*printf("sect_log->phy: 2-seg (ret:%d) \n", ret);*/
			}
			else if (((3*seg) <= log_idx) && (log_idx < (4*seg))) /* fourth 16M area */
			{
				phy_idx = log_idx - 3*seg;
				ret = vxQspiFlash_Set_Seg_48_64M(pDev);
				
				/*printf("sect_log->phy: 3-seg (ret:%d) \n", ret);*/
			}
		}
		break;

	default:			
		phy_idx = 0;  /* error*/
		break;
	}
	
	return phy_idx;
}

int vxQspiFlash_SectIdx_LogtoPhy_Only(VXB_DEVICE_ID pDev, int log_idx)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	int ret = 0;
	int seg = 0, phy_idx = 0;

	switch (pDrvCtrl->CTRL.addr_bytes_mode)
	{
	case ADDR_4_BYTES_MODE:		
		{
			phy_idx = log_idx;
		}
		break;
	
	case ADDR_3_BYTES_MODE:
		{
			/**/
			/* 16M / sectSize*/
			/**/
			seg = 0x1000000 / pDrvCtrl->FLASH.sectSize;  /* 16M / 64K = 256 sects*/
			
			if ((0 <= log_idx) && (log_idx < seg))                /* first 16M area */
			{
				phy_idx = log_idx;
			}
			else if ((seg <= log_idx) && (log_idx < (2*seg)))    /* second 16M area */
			{
				phy_idx = log_idx - 1*seg;
			}
			else if (((2*seg) <= log_idx) && (log_idx < (3*seg))) /* third 16M area */
			{
				phy_idx = log_idx - 2*seg;
			}
			else if (((3*seg) <= log_idx) && (log_idx < (4*seg))) /* fourth 16M area */
			{
				phy_idx = log_idx - 3*seg;
			}
		}
		break;

	default:			
		phy_idx = 0;  /* error*/
		break;
	}
	
	return phy_idx;
}

int vxQspiFlash_Offset_LogtoPhy(VXB_DEVICE_ID pDev, int offset_addr)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	int ret = 0;
	UINT32 seg = 0, phy_addr = 0;

	switch (pDrvCtrl->CTRL.addr_bytes_mode)
	{
	case ADDR_4_BYTES_MODE:		
		{
			phy_addr = offset_addr;
		}
		break;
	
	case ADDR_3_BYTES_MODE:
		{
			/**/
			/* 16M / sectSize*/
			/**/
			/*seg = 0x1000000 / pDrvCtrl->FLASH.sectSize;  // 16M / 64K = 256 sects*/
			seg = 0x1000000;  /* 16M / 64K = 256 sects*/
			/*printf("seg_num: 0x%X (direct_16M) => ", seg);*/
			
			if ((0 <= offset_addr) && (offset_addr < seg))                 /* first 16M area */
			{
				phy_addr = offset_addr;
				ret = vxQspiFlash_Set_Seg_0_16M(pDev);
				
				/*printf("sect_log->phy: 0-seg (ret:%d) \n", ret);*/
			}
			else if ((seg <= offset_addr) && (offset_addr < (2*seg)))      /* second 16M area */
			{
				phy_addr = offset_addr - 1*seg;
				ret = vxQspiFlash_Set_Seg_16_32M(pDev);
				
				/*printf("sect_log->phy: 1-seg (ret:%d) \n", ret);*/
			}
			else if (((2*seg) <= offset_addr) && (offset_addr < (3*seg)))   /* third 16M area */
			{
				phy_addr = offset_addr - 2*seg;
				ret = vxQspiFlash_Set_Seg_32_48M(pDev);
				
				/*printf("sect_log->phy: 2-seg (ret:%d) \n", ret);*/
			}
			else if (((3*seg) <= offset_addr) && (offset_addr < (4*seg)))    /* fourth 16M area */
			{
				phy_addr = offset_addr - 3*seg;
				ret = vxQspiFlash_Set_Seg_48_64M(pDev);
				
				/*printf("sect_log->phy: 3-seg (ret:%d) \n", ret);*/
			}
		}
		break;

	default:			
		phy_addr = 0;  /* error*/
		break;
	}
	
	return phy_addr;
}

int vxQspiFlash_PageIdx_LogtoPhy(VXB_DEVICE_ID pDev, int page_idx_log)
{

}

#endif

#define __4_bytes_addr_mode__
/*
4_bytes_address mode:
*/

/*
for Micron FlashChip: N25Q512A
*/
int vxQspiFlash_Enter_4BAddr(VXB_DEVICE_ID pDev)
{
    int error;
    UINT32 reg;

    error = vxQspiFlash_WREN_Cmd(pDev);
    if (error == ERROR)
	{
        return ERROR;
    }
    
    /*Send Enter 4B Command*/
    reg = (0xB7 << 24);
    error = vxQspiCtrl_CmdExecute(pDev, reg);
    if (error == ERROR)
	{
        return ERROR;
    }
	
    return OK;
}

int vxQspiFlash_Exit_4BAddr(VXB_DEVICE_ID pDev)
{
    int error;
    UINT32 reg;

    error = vxQspiFlash_WREN_Cmd(pDev);
    if (error == ERROR)
	{
        return ERROR;
    }
    
    /*Send Enter 4B Command*/
    reg = (0xE9 << 24);
    error = vxQspiCtrl_CmdExecute(pDev, reg);
    if (error == ERROR)
	{
        return ERROR;
    }
	
    return OK;
}

#endif

#if 1

#define __Direct__InDirect__Mode__

/*
erase the whole flash chip
*/
/* need more test*/
int vxQspiFlash_Erase_Chip(VXB_DEVICE_ID pDev)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
    int error;
    UINT32 reg;
	
	UINT8 cmd = 0;
    
    error = vxQspiFlash_WREN_Cmd(pDev);
    if (error == ERROR)
	{
        return ERROR;
    }
    
    /*Send Erase Command*/
    cmd = 0xC4; /* 0x60 C7*/
    reg = 0xC40A0000;/*((UINT32)cmd << 24);*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_FCAR_OFFSET, 0);
	
    error = vxQspiCtrl_CmdExecute(pDev, reg);
    if (error == ERROR)
	{
        return ERROR;
    }
    
    error = vxQspiFlash_Wait_FlashReady(pDev, 6000000);
    if (error == ERROR)
	{
		return ERROR;
	}

    return OK;
}

/*****************************************************************************
* This function executes SECTOR ERASE.
*
* @param
*       - sect_idx: sector or block idxL: 0,1,2,3....(max-1)
* @return
*		- OK if success.
*		- ERROR if failure.
*
* @note:	
*     default - 64K sector	
*
******************************************************************************/

int vxQspiFlash_Erase_Sect(VXB_DEVICE_ID pDev, int sect_idx)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;

    int ret = OK;
    UINT32 sect_offset = 0, reg = 0;	
	int phy_idx = 0;
	
    QSPI_SEM_TAKE(pDrvCtrl->muxSem);

	phy_idx = vxQspiFlash_SectIdx_LogtoPhy(pDev, sect_idx);
	
    /* 
    calculate sector start address to be erased 
	*/
    sect_offset = (phy_idx * pDrvCtrl->FLASH.sectSize);
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_FCAR_OFFSET, sect_offset);
    
    ret = vxQspiFlash_WREN_Cmd(pDev);
    if (ret == ERROR)
	{
        goto ERASE_END;
    }

	/* Erase Operations */
	/*
	#define SUBSECTOR_ERASE_CMD                  0x20
	#define SUBSECTOR_ERASE_4_BYTE_ADDR_CMD      0x21

	#define SECTOR_ERASE_CMD                     0xD8
	#define SECTOR_ERASE_4_BYTE_ADDR_CMD         0xDC

	#define BULK_ERASE_CMD                       0xC7
	*/
	
    /* Send Erase Command*/
    if (pDrvCtrl->CTRL.addr_bytes_mode == ADDR_4_BYTES_MODE)
    {    
    	reg = 0xDC0B0000; /* 64k*/
    }
	else
    {    
    	reg = 0xD80A0000;  /* 64k*/
    }	
	
    ret = vxQspiCtrl_CmdExecute(pDev, reg);
    if (ret == ERROR)
	{
        goto ERASE_END;
    }
    
    ret = vxQspiFlash_Wait_FlashReady(pDev, 5000);
    if (ret == ERROR)
	{
        goto ERASE_END;
    }

ERASE_END:	
    QSPI_SEM_GIVE(pDrvCtrl->muxSem);
    
    return ret;
}



#endif

#if 1  /* InDirect mode*/

#define __InDirect_Fifo_Mode__

/*****************************************************************************
* This function read flash using indirect mode
*
* @param
*       - offset is a value where data read 
*       - byteCount is a number of bytes to read
*       - recvBuffer is a point to read data
*
* @return
*		- OK if success.
*		- ERROR if failure.
*
* @note		
*
******************************************************************************/
int vxQspiCtrl_FastRcv_Setup(VXB_DEVICE_ID pDev, UINT32 flash_offset, UINT32 byteCnt, UINT8* rcvBuf)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
		
    UINT32 cfgReg = 0;
    UINT32 rxFullLvl = 480;    
    
    /*
    if (pDrvCtrl->CTRL.isBusy == TRUE)
    {
        return ERROR;
    }	
    pDrvCtrl->CTRL.isBusy = TRUE;
	*/
    QSPI_SEM_TAKE(pDrvCtrl->muxSem);
    
    pDrvCtrl->CTRL.requestBytes = byteCnt;
    pDrvCtrl->CTRL.requestBytes = byteCnt;
    pDrvCtrl->CTRL.remainBytes = byteCnt;
    pDrvCtrl->CTRL.rcvBuf = rcvBuf;
    
    /* 
    set flash address where read data (address in flash) 
	*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IRTSAR_OFFSET, flash_offset);  
	
    /* 
    set number of bytes to be transferred 
	*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IRTNBR_OFFSET, pDrvCtrl->CTRL.requestBytes);  
	
    /* 
    set controller trig adress where read data (adress in controller) 
	*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IAATR_OFFSET, pDrvCtrl->CTRL.fifoBaseAddr);    
	
    /* 
    set Trigger Address Range (2^15) 
	*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_ITARR_OFFSET, 0x0F);  
	
    /* 
    set WaterMark Register 
	*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IWTWR_OFFSET, 0x00); 	  /* tx*/
	rxFullLvl = gQspi_Ctrl_Params.rxFullLvl;
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IRTWR_OFFSET, rxFullLvl);  /* rx*/
	
    /* 
    trigger Indirect Write access
    */
    cfgReg = 0x01;
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IRTCR_OFFSET, cfgReg);  
	
    /*pDrvCtrl->CTRL.isBusy = FALSE;  /* jc     */
    QSPI_SEM_GIVE(pDrvCtrl->muxSem);
	
    return OK;
}


int vxQspiCtrl_FastSnd_Setup(VXB_DEVICE_ID pDev, UINT32 flash_offset, UINT32 byteCnt, UINT8* sndBuf)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
		
    UINT32 cfgReg = 0;
    UINT32 txEmptyLvl = 32;    
    
    /*
    if (pDrvCtrl->CTRL.isBusy == TRUE)
    {
        return ERROR;
    }	
    pDrvCtrl->CTRL.isBusy = TRUE;
	*/	
    QSPI_SEM_TAKE(pDrvCtrl->muxSem);
    
    pDrvCtrl->CTRL.requestBytes = byteCnt;
    pDrvCtrl->CTRL.remainBytes = byteCnt;
    pDrvCtrl->CTRL.sndBuf = sndBuf;
    
    /* 
    set flash address where write data (address in flash) 
	*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IWTSAR_OFFSET, flash_offset);  
	
    /* 
    set number of bytes to be transferred 
	*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IWTNBR_OFFSET, pDrvCtrl->CTRL.requestBytes);  
	
    /* 
    set controller trig adress where write data (adress in controller) 
	*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IAATR_OFFSET, pDrvCtrl->CTRL.fifoBaseAddr);    
	
    /* 
    set Trigger Address Range (2^15) 
	*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_ITARR_OFFSET, 0xf);      
	
    /* 
    set WaterMark Register (between 10~250 if page_size is 256 bytes) 
	*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IRTWR_OFFSET, 0x00);   /* rx*/
	txEmptyLvl = gQspi_Ctrl_Params.txEmptyLvl;
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IWTWR_OFFSET, txEmptyLvl);   /* tx*/
	
    /* 
    trigger Indirect Write access 
	*/
    cfgReg = 0x01;
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IWTCR_OFFSET, cfgReg);
    
    /*pDrvCtrl->CTRL.isBusy = FALSE;  /* jc 	*/
    QSPI_SEM_GIVE(pDrvCtrl->muxSem);
	
    return OK;
}

int vxQspiFlash_Rx_Data(VXB_DEVICE_ID pDev, UINT32 offset, UINT32 byteCnt, UINT8* rcvBuf)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
	int ret = 0;
	UINT32 wdCnt = 0, remainder = 0;
	UINT8 * pBuf8 = NULL;
	UINT32 tmp32 = 0;
	int idx = 0;
	int timeout = 0, j = 0;
	UINT32 cfgReg = 0;
		
	/**/
	/* clear Rcv Fifo*/
	/**/
	vxQspiCtrl_Clr_RcvFifo(pDev);

	/*
	setup fastRcv
	*/
	vxQspiCtrl_FastRcv_Setup(pDev, offset, byteCnt, rcvBuf);

	do
	{
		/*read progress*/
		cfgReg = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_IRTCR_OFFSET);
		
		if (cfgReg & 0x4)
		{
			/* Read  Watermark */
			wdCnt = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_SFLR_OFFSET);
			remainder = pDrvCtrl->CTRL.requestBytes & 0x3;
			
			/* Read data from SRAM */
			for (idx=0; idx<wdCnt; idx++)
			{
				if (pDrvCtrl->CTRL.requestBytes >= 4)
				{
					*(UINT32*)(pDrvCtrl->CTRL.rcvBuf) = *(UINT32*)(pDrvCtrl->CTRL.fifoBaseAddr);
					
					pDrvCtrl->CTRL.rcvBuf += 4;
					pDrvCtrl->CTRL.requestBytes -= 4;  
				}
				else
				{					 
					if (remainder == 0)
					{	
						break;
					}
					
					tmp32 = *(UINT32*)(pDrvCtrl->CTRL.fifoBaseAddr);
					while (remainder > 0)
					{
						*(UINT8*)(pDrvCtrl->CTRL.rcvBuf) = tmp32 & 0xff;
						tmp32 = tmp32 >> 8;
						
						pDrvCtrl->CTRL.rcvBuf++;						
						pDrvCtrl->CTRL.remainBytes--; 
						remainder--;
					}
					
					break;  /* for()*/
				}
			}/* for ()*/
		}/* if (cfgReg & 0x4)`*/
	} while (cfgReg & 0x4);


	timeout = QSPI_TIMEOUT;
	do 
	{
		ret = vxQspiCtrl_Wait_ExeOk(pDev, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK, OK_FLAG_1);
		if (ret == ERROR)
		{
			sysUsDelay(10);		
		}
		else
		{
			break;
		}
		
		timeout--;
		if (timeout <= 0)
		{
			byteCnt = 0;
		}
	} while (timeout > 0);

	/* 
	clear QSPI_IND_RD_XFER_DONE bit 
	*/
	vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK);

	return byteCnt;
}

int vxQspiFlash_Tx_Data(VXB_DEVICE_ID pDev, UINT32 offset, UINT32 byteCnt, UINT8* sndBuf)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
		
	int ret = 0;
	UINT32 wdCnt = 0, remainder = 0;
	UINT32 tmp32 = 0;
	int idx = 0;
	int timeout = 0, j = 0;	
	UINT32 cfgReg = 0;	
	
	/**/
	/* UnLock flash */
	/**/
	vxQspiFlash_Unlock_Flash(pDev);

	/*
	setup fastSnd
	*/
	vxQspiCtrl_FastSnd_Setup(pDev, offset, byteCnt, sndBuf);

	/* 
	copy data from sram to flash 
	*/	
	do
	{
		cfgReg = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_IWTCR_OFFSET);
		
		if (cfgReg & 0x4)
		{
			/*wdCnt = QSPIFLASH_PAGE_256 << 2;*/
			wdCnt = QSPIFLASH_PAGE_256 >> 2;
			remainder = pDrvCtrl->CTRL.remainBytes & 0x3;
			
			for (idx=0; idx<wdCnt; idx++)
			{
				if (pDrvCtrl->CTRL.remainBytes >= 4)
				{
					*((UINT32*)(pDrvCtrl->CTRL.fifoBaseAddr)) = *((UINT32*)(pDrvCtrl->CTRL.sndBuf));
					
					pDrvCtrl->CTRL.sndBuf += 4;
					pDrvCtrl->CTRL.remainBytes -= 4; 
				}
				else
				{
					tmp32 = 0xffffffff;
					
					if (remainder == 0)
					{	
						break; /* for()*/
					}
					
					while (remainder > 0)
					{
						tmp32 = tmp32 << 8;
						tmp32 = tmp32 | *((UINT8*)(pDrvCtrl->CTRL.sndBuf + remainder - 1));
						
						pDrvCtrl->CTRL.remainBytes -= 1; 
						remainder -= 1;
					}
					
					*((UINT32*)(pDrvCtrl->CTRL.fifoBaseAddr)) = tmp32;				
					break;  /* for()*/
				}
			}/* for ()*/
		}/* if (cfgReg & 0x4)*/
	} while (cfgReg & 0x4);

	timeout = QSPI_TIMEOUT;
	do 
	{
		ret = vxQspiCtrl_Wait_ExeOk(pDev, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK, OK_FLAG_1);
		if (ret == ERROR)
		{
			sysUsDelay(10);		
		}
		else
		{
			break;
		}
		
		timeout--;
		if (timeout <= 0)
		{
			byteCnt = 0;
		}
	} while (timeout > 0);

	/* 
	clear QSPI_IND_WR_XFER_DONE bit 
	*/
	vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK);

	return byteCnt;
}


#define __qspi_flash_api__

/*
sect_idx: 0,1,2,3 ... ...
*/
int vxQspiFlash_Read_Sect(VXB_DEVICE_ID pDev, int sect_idx, UINT8* rcvBuf8)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
	UINT32 offset = 0;
	UINT32 byteCnt = 0;	
	int phy_idx = 0;	

	phy_idx = vxQspiFlash_SectIdx_LogtoPhy(pDev, sect_idx);
	
	offset = phy_idx * pDrvCtrl->FLASH.sectSize;
	byteCnt = pDrvCtrl->FLASH.sectSize;	
	
	return vxQspiFlash_Rx_Data(pDev, offset, byteCnt, rcvBuf8);
}

/*
sect_idx: 0,1,2,3 ... ...
*/
int vxQspiFlash_Write_Sect(VXB_DEVICE_ID pDev, int sect_idx, UINT8 * sndBuf8)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;

	UINT32 offset = 0;
	UINT32 byteCnt = 0;
	int phy_idx = 0;	

	phy_idx = vxQspiFlash_SectIdx_LogtoPhy(pDev, sect_idx);
	
	offset = phy_idx * pDrvCtrl->FLASH.sectSize;
	byteCnt = pDrvCtrl->FLASH.sectSize;	
	
	return vxQspiFlash_Tx_Data(pDev, offset, byteCnt, sndBuf8);
}

/*
page_idx: 0,1,2,3 ... ...
*/
int vxQspiFlash_Read_Page(VXB_DEVICE_ID pDev, int page_idx, UINT8* rcvBuf8)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
	UINT32 offset = 0;
	UINT32 byteCnt = 0;
	
	offset = page_idx * pDrvCtrl->FLASH.pageSize;
	byteCnt = pDrvCtrl->FLASH.pageSize;	
	
	return vxQspiFlash_Rx_Data(pDev, offset, byteCnt, rcvBuf8);
	
#if 0	

	/**/
	/* clear Rcv Fifo*/
	/**/
	vxQspiCtrl_Clr_RcvFifo(pDev);

	/*
	setup fastRcv
	*/
	vxQspiCtrl_FastRcv_Setup(pDev, offset, byteCnt, rcvBuf8);
	
	pBuf8 = (UINT8 *)pDrvCtrl->CTRL.fifoBaseAddr;
	idx = 0;	
	do 
	{
		/* 	Read  Watermark */
		dataCnt = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_SFLR_OFFSET) * 4;		
		if (dataCnt == 0)
		{
			delay_1us();
		}
		else
		{		
			/* 
			Read data from SRAM 
			*/
			memcpy((UINT8*)pDrvCtrl->CTRL.rcvBuf, (UINT8*)(&pBuf8[idx]), dataCnt);
			idx += dataCnt;
			
			pDrvCtrl->CTRL.rcvBuf += dataCnt;
			pDrvCtrl->CTRL.remainBytes -= dataCnt;
		}
	} while (pDrvCtrl->CTRL.remainBytes > 0);
	
	timeout = QSPI_TIMEOUT;
	do 
	{
		ret = vxQspiCtrl_Wait_ExeOk(pDev, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK, OK_FLAG_1);
		if (ret == ERROR)
		{
			sysUsDelay(10);		
		}
		else
		{
			break;
		}
		
		timeout--;
		if (timeout <= 0)
		{
			byteCnt = 0;
		}
	} while (timeout > 0);

	/* 
	clear QSPI_IND_RD_XFER_DONE bit 
	*/
	vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK);

	return byteCnt;
#endif
}

/*
page_idx: 0,1,2,3 ... ...
*/
int vxQspiFlash_Write_Page(VXB_DEVICE_ID pDev, int page_idx, UINT8* sndBuf8)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
		
	UINT32 offset = 0;
	UINT32 byteCnt = 0;
	
	offset = page_idx * pDrvCtrl->FLASH.pageSize;
	byteCnt = pDrvCtrl->FLASH.pageSize;	
	
	return vxQspiFlash_Tx_Data(pDev, offset, byteCnt, sndBuf8);
}


int vxQspiFlash_RcvBytes_Indirect(VXB_DEVICE_ID pDev, UINT32 addr, int len, UINT8* rcvBuf8)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
	int ret = 0;
	UINT32 offset = 0;
	UINT32 byteCnt = 0;
	
	int phy_idx = 0;	
	int log_idx = 0;

	int start_bank = 0, end_bank = 0;
	UINT32 addr1 = 0, addr2 = 0;
	int len1 = 0, len2 = 0;

	if (pDrvCtrl->CTRL.addr_bytes_mode ==  ADDR_4_BYTES_MODE)
	{
		offset = addr;
		byteCnt = len;	
		
		QSPI_SEM_TAKE(pDrvCtrl->muxSem);
		ret = vxQspiFlash_Rx_Data(pDev, offset, byteCnt, rcvBuf8);		
		QSPI_SEM_GIVE(pDrvCtrl->muxSem);
		
		if (ret > 0)
		{
			return OK;
		}
		else
		{
			return ERROR;
		}
	}
	else
	{		
		start_bank = addr / SIZE_16M;  /* 16M / bank*/
		end_bank = (addr + len) / SIZE_16M;  /* 16M / bank*/

		/*if (start_bank == end_bank)*/
		{
			/*log_idx = addr / pDrvCtrl->FLASH.sectSize;*/
			/*phy_idx = vxQspiFlash_SectIdx_LogtoPhy(pDev, log_idx);				*/
			/*offset = (phy_idx *pDrvCtrl->FLASH.sectSize) + (addr % pDrvCtrl->FLASH.sectSize);			*/
			offset = vxQspiFlash_Offset_LogtoPhy(pDev, addr);
			
			byteCnt = len;	

			/*printf("rd sect_idx=>logic:%d->phy:%d \n", log_idx, phy_idx);			*/
			/*printf("wr sect_idx=>logic:0x%X->phy:0x%X(len-%d) \n", addr, offset, len);*/
			
		    QSPI_SEM_TAKE(pDrvCtrl->muxSem);
			ret = vxQspiFlash_Rx_Data(pDev, offset, byteCnt, rcvBuf8);		
		    QSPI_SEM_GIVE(pDrvCtrl->muxSem);
		}
/*
		else
		{	
			// len1-area
			addr1 = addr, 
			len1 = ((start_bank + 1) * SIZE_16M) - addr; 
			
			log_idx = addr1 / pDrvCtrl->FLASH.sectSize;
			phy_idx = vxQspiFlash_SectIdx_LogtoPhy(pDev, log_idx);			
			offset = (phy_idx * pDrvCtrl->FLASH.sectSize) + (addr1 % pDrvCtrl->FLASH.sectSize);
			byteCnt = len1;
			
		    QSPI_SEM_TAKE(pDrvCtrl->muxSem);			
			ret = vxQspiFlash_Rx_Data(pDev, offset, byteCnt, rcvBuf8);			
		    QSPI_SEM_GIVE(pDrvCtrl->muxSem);

			// len2-area
			addr2 = (start_bank + 1) * SIZE_16M;
			len2 = (addr + len) - ((start_bank + 1) * SIZE_16M);
			
			log_idx = addr2 / pDrvCtrl->FLASH.sectSize;
			phy_idx = vxQspiFlash_SectIdx_LogtoPhy(pDev, log_idx);			
			offset = (phy_idx * pDrvCtrl->FLASH.sectSize) + (addr2 % pDrvCtrl->FLASH.sectSize);
			byteCnt = len2;
			
			QSPI_SEM_TAKE(pDrvCtrl->muxSem);
			rcvBuf8 += len1;
			ret = vxQspiFlash_Rx_Data(pDev, offset, byteCnt, rcvBuf8);			
			QSPI_SEM_GIVE(pDrvCtrl->muxSem);
		}
*/
		if (ret > 0)
		{
			return OK;
		}
		else
		{
			return ERROR;
		}
	}	

}

int vxQspiFlash_SndBytes_Indirect(VXB_DEVICE_ID pDev, UINT32 addr, int len, UINT8* sndBuf8)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	int ret = 0;
	UINT32 offset = 0;
	UINT32 byteCnt = 0;
	
	int phy_idx = 0;	
	int log_idx = 0;
	int start_bank = 0, end_bank = 0;
	UINT32 addr1 = 0, addr2 = 0;
	int len1 = 0, len2 = 0;
	
	if (pDrvCtrl->CTRL.addr_bytes_mode ==  ADDR_4_BYTES_MODE)
	{
		offset = addr;
		byteCnt = len;	
		
		QSPI_SEM_TAKE(pDrvCtrl->muxSem);	
		ret = vxQspiFlash_Tx_Data(pDev, offset, byteCnt, sndBuf8);		
		QSPI_SEM_GIVE(pDrvCtrl->muxSem);
		
		if (ret > 0)
		{
			return OK;
		}
		else
		{
			return ERROR;
		}
	}
	else
	{
		start_bank = addr / SIZE_16M;  /* 16M / bank*/
		end_bank = (addr + len) / SIZE_16M;  /* 16M / bank*/

		/*if (start_bank == end_bank)*/
		{
			/*log_idx = addr / pDrvCtrl->FLASH.sectSize;*/
			/*phy_idx = vxQspiFlash_SectIdx_LogtoPhy(pDev, log_idx);				*/
			/*offset = (phy_idx *pDrvCtrl->FLASH.sectSize) + (addr % pDrvCtrl->FLASH.sectSize);*/
			offset = vxQspiFlash_Offset_LogtoPhy(pDev, addr);
			
			byteCnt = len;	
			
			/*printf("wr sect_idx=>logic:%d->phy:%d(len-%d) \n", log_idx, phy_idx, len);*/
			/*printf("wr sect_idx=>logic:0x%X->phy:0x%X(len-%d) \n", addr, offset, len);*/
			
		    QSPI_SEM_TAKE(pDrvCtrl->muxSem);	
			ret = vxQspiFlash_Tx_Data(pDev, offset, byteCnt, sndBuf8);		
		    QSPI_SEM_GIVE(pDrvCtrl->muxSem);
		}
/*
		else
		{	
			addr1 = addr, 
			len1 = ((start_bank + 1) * SIZE_16M) - addr; 
			
			log_idx = addr1 / pDrvCtrl->FLASH.sectSize;
			phy_idx = vxQspiFlash_SectIdx_LogtoPhy(pDev, log_idx);			
			offset = (phy_idx * pDrvCtrl->FLASH.sectSize) + (addr1 % pDrvCtrl->FLASH.sectSize);
			byteCnt = len1;
			
		    QSPI_SEM_TAKE(pDrvCtrl->muxSem);			
			ret = vxQspiFlash_Tx_Data(pDev, offset, byteCnt, sndBuf8);			
		    QSPI_SEM_GIVE(pDrvCtrl->muxSem);

			addr2 = (start_bank + 1) * SIZE_16M;
			len2 = (addr + len) - ((start_bank + 1) * SIZE_16M);
			
			log_idx = addr2 / pDrvCtrl->FLASH.sectSize;
			phy_idx = vxQspiFlash_SectIdx_LogtoPhy(pDev, log_idx);			
			offset = (phy_idx * pDrvCtrl->FLASH.sectSize) + (addr2 % pDrvCtrl->FLASH.sectSize);
			byteCnt = len2;
			
			QSPI_SEM_TAKE(pDrvCtrl->muxSem);
			sndBuf8 += len1;
			ret = vxQspiFlash_Tx_Data(pDev, offset, byteCnt, sndBuf8);			
			QSPI_SEM_GIVE(pDrvCtrl->muxSem);
		}
*/
		
		if (ret > 0)
		{
			return OK;
		}
		else
		{
			return ERROR;
		}
	}
}


#define __only_qspi_0__
int vxQspiFlash_ReadBytes_InDirect0(UINT32 addr, int len, UINT8 * rcvBuf8)
{
	VXB_DEVICE_ID pDev;
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, 0);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", 0);
        return ERROR;
    }
	
	if (ERROR == vxQspiFlash_RcvBytes_Indirect(pDev, addr, len, rcvBuf8))
	{
		return ERROR;
	}
	else
	{
		return OK;	
	}
}

int vxQspiFlash_WriteBytes_InDirect0(UINT32 addr, int len, UINT8 * sndBuf8)
{
	VXB_DEVICE_ID pDev;
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, 0);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", 0);
        return ERROR;
    }
	
	if (ERROR == vxQspiFlash_SndBytes_Indirect(pDev, addr, len, sndBuf8))
	{
		return ERROR;
	}
	else
	{
		return OK;	
	}
}

int vxQspiFlash_UnLock_Flash_Ctrl0(void)
{
	VXB_DEVICE_ID pDev;
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, 0);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", 0);
        return ERROR;
    }
	
	return vxQspiFlash_Unlock_Flash(pDev);
}

#endif


#if 1   /* direct mode*/

#define __Direct_Sram_Mode__

/*****************************************************************************
* This function write flash using direct mode
*
* @param
*       - offset is a value where data write 
*       - byteCount is a number of bytes to write
*       - sendBuffer is a point to write data
*
* @return
*		- OK if success.
*		- ERROR if failure.
*
* @note		
*
******************************************************************************/
int vxQspiFlash_SndBytes_Direct_2(VXB_DEVICE_ID pDev, UINT32 offset_log, UINT32 byteCnt, UINT8* sndBuf)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
    int status;
    int size, pageSize;
    UINT32 reg1, reg2;
    UINT32 address;
    UINT32 offset;
	
	/* logic_addr to phy_addr*/
	offset = vxQspiFlash_Offset_LogtoPhy(pDev, offset_log);
	
	/**/
	/* Wait semphore*/
	/**/
    QSPI_SEM_TAKE(pDrvCtrl->muxSem);

	/**/
    /* disable wel & auto_poll*/
    /**/
    reg1 = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_WCCR_OFFSET);
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_WCCR_OFFSET, reg1 | 0x4000);
	
    reg2 = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_DWIR_OFFSET);
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_DWIR_OFFSET, reg2 | 0x100);    
    
    pageSize = (0x1 << pDrvCtrl->FLASH.pageSize);
    
    while (byteCnt > 0)
    {
        status = vxQspiFlash_WREN_Cmd(pDev) ;
        if (status)
        {   
        	break;
        }
        
        if ((offset & (pDrvCtrl->FLASH.pageSize - 1)) != 0)
            size = pageSize - (offset % pageSize);
        else
            size = pageSize;
        
        if (byteCnt < size)
            size = byteCnt;
        
        address = pDrvCtrl->CTRL.fifoBaseAddr + offset;
		
        /* 
        Write to Flash  
		*/
        memcpy((void*)address, (void*)sndBuf, (size_t)size); 
        
        status = vxQspiFlash_Wait_FlashReady(pDev, 1500);
        if (status)
		{	
			break;
		}
        
        byteCnt -= size;
        offset += size;
        sndBuf += size;
    }
    
	/**/
    /* enable wel & auto_poll*/
    /**/
    reg1 = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_WCCR_OFFSET);
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_WCCR_OFFSET, reg1);
	
    reg2 = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_DWIR_OFFSET);
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_DWIR_OFFSET, reg2);  

	/**/
	/* Give semphore*/
	/**/
    QSPI_SEM_GIVE(pDrvCtrl->muxSem);
    
    return status;
    
}

int vxQspiFlash_SndBytes_Direct(VXB_DEVICE_ID pDev, UINT32 offset_log, UINT32 byteCnt, UINT8 *sndBuf)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	UINT32  remain;
	UINT32  num;
	UINT8* src;
	UINT8* dst;
	int timeout = 0;
	int ret = 0, err = 0;
	
    UINT32 offset;
	
	/* logic_addr to phy_addr*/
	offset = vxQspiFlash_Offset_LogtoPhy(pDev, offset_log);
	
    QSPI_SEM_TAKE(pDrvCtrl->muxSem);

	/*src = ppBuf[0];*/
	/*dst = (UINT8 *pDrvCtrl->memBase + uAddr;*/
	/*remain = uBufLen;*/
	src = sndBuf;
	dst = (UINT8 *)(pDrvCtrl->CTRL.fifoBaseAddr + offset);
	remain = byteCnt;

	while (remain > 0)
	{
		if (remain > pDrvCtrl->FLASH.pageSize)
		{
			num = (UINT32)pDrvCtrl->FLASH.pageSize;
		}
		else
		{
			num = (UINT32)remain;
		}

		memcpy((void *)dst, (void *)src, (size_t)num);

		/* 
		Wait for write operation done 
		*/
		/*err = vxQspiFlash_Wait_WIP_Ok(pDev);*/
		err = vxQspiFlash_Wait_WIP_Ok(pDev, 1500);
		if (err == ERROR)
		{
			QSPI_SEM_GIVE(pDrvCtrl->muxSem);
			return ERROR;
		}

		src += num;
		dst += num;
		remain -= num;
	}

    QSPI_SEM_GIVE(pDrvCtrl->muxSem);
    
    return OK;
}

/*****************************************************************************
* This function read flash using direct mode
*
* @param
*       - offset is a value where data read 
*       - byteCount is a number of bytes to read
*       - recvBuffer is a point to read data
*
* @return
*		- OK if success.
*		- ERROR if failure.
*
* @note		
*
******************************************************************************/
int vxQspiFlash_RcvBytes_Direct(VXB_DEVICE_ID pDev, UINT32 offset_log, UINT32 byteCnt, UINT8 *rcvBuf)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
    UINT32 address;    
    UINT32 offset;    
    	
    if (byteCnt == 0)
    {
        return ERROR; 
    }    

	/* logic_addr to phy_addr*/
	offset = vxQspiFlash_Offset_LogtoPhy(pDev, offset_log);
	
    if (pDrvCtrl->FLASH.devSize < (offset + byteCnt))
    {
        return ERROR; 
    }	
	
    QSPI_SEM_TAKE(pDrvCtrl->muxSem);
    
    /* Read from flash */
    address = pDrvCtrl->CTRL.fifoBaseAddr + offset;
    memcpy((UINT8*)rcvBuf, (UINT8*)address, (size_t)byteCnt); 

 
    QSPI_SEM_GIVE(pDrvCtrl->muxSem);
    
    return OK;
}

#define __only_qspi_0__

/*
qpsi-flash control_0
*/
int vxQspiFlash_ReadBytes_Direct0(int addr, int len, UINT8 * rcvBuf8)
{
	VXB_DEVICE_ID pDev;
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, 0);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", 0);
        return ERROR;
    }
	
	if (ERROR == vxQspiFlash_RcvBytes_Direct(pDev, addr, len, rcvBuf8))
	{
		return ERROR;
	}
	else
	{
		return OK;	
	}
}

/*
qpsi-flash control_0
*/
int vxQspiFlash_WriteBytes_Direct0(int addr, int len, UINT8 * sndBuf8)
{
	VXB_DEVICE_ID pDev;
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, 0);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", 0);
        return ERROR;
    }
	
	if (ERROR == vxQspiFlash_SndBytes_Direct(pDev, addr, len, sndBuf8))
	{
		return ERROR;
	}
	else
	{
		return OK;	
	}
}

#endif

#if 1

#define __tffs_norflash__

/*******************************************************************************
*
* qspiFlashInit1 - initialize Altera Soc Gen 5 controller.
*
* This function initializes Altera Soc Gen 5 controller stage 1.
*
* RETURNS: N/A.
*
* ERRNO
*/
void qspiFlashInit1 (void)
{
    /* reset pDev controller and pDev flash chip */

    /*qspiFlashReset ();*/
    /*qspiFlashResetChip ();*/
    
#if 1	
	int i = 0;
	UINT32 tmp32 = slcr_read(SLCR_QSPI0_CTRL);

	/* set 1*/
	tmp32 |= (QSPI_APB_RST | QSPI_AHB_RST | QSPI_REF_RST);
	slcr_write(SLCR_QSPI0_CTRL, tmp32);

	for (i=0; i<100; i++);  /* for delay*/
	
	/* set 0*/
	tmp32 &= ~(QSPI_APB_RST | QSPI_AHB_RST | QSPI_REF_RST);
	slcr_write(SLCR_QSPI0_CTRL, tmp32);
#endif

#if 0
	/*
	reg[00] = 0x80080081 
	reg[04] = 0x0802006B 
	reg[08] = 0x00000002 
	reg[0C] = 0x00000000 
	reg[10] = 0x00000001 
	reg[14] = 0x00000002 
	reg[18] = 0x00000080 
	reg[1C] = 0xE8000000 
	*/
	UINT32 base = 0xE0000000;
	*((UINT32*)(base+0x00)) = 0x80080081;
	*((UINT32*)(base+0x04)) = 0x0802006B; 
	*((UINT32*)(base+0x08)) = 0x00000002; 
	*((UINT32*)(base+0x0C)) = 0x00000000; 
	*((UINT32*)(base+0x10)) = 0x00000001; 
	*((UINT32*)(base+0x14)) = 0x00000002; 
	*((UINT32*)(base+0x18)) = 0x00000080; 
	*((UINT32*)(base+0x1C)) = 0xE8000000; 
#endif

    return;
}

/*******************************************************************************
*
* qspiFlashInit2 - initialize Altera Soc Gen 5 controller.
*
* This function initializes Altera Soc Gen 5 controller stage 2.
*
* RETURNS: N/A.
*
* ERRNO
*/

 void qspiFlashInit2 (void)
{
    /*
    int i = 0;

    for (i = 0; i < QSPI_MAX_DEVICE; i++)
        QspiDev [i].sem = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE |
                                      SEM_INVERSION_SAFE);
	*/	
	vxQspiRegister2();
	
	return;
}


/*******************************************************************************
*
* qspiFlashSectorErase - erase the specified flash sector
*
* This routine erases the specified sector, all bits in sector are
* set to 1.
*
* RETURNS: OK, or ERROR if timeout or parameter is invalid.
*
* ERRNO: N/A.
*/

STATUS tffs_qspiFlashSectorErase
    (
    UINT32 addr
    )
{
	int ret = 0;
	int sect_idx = 0;
	
	VXB_DEVICE_ID pDev;
	FM_QSPI_DRV_CTRL* pDrvCtrl;
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, 0);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", 0);
        return ERROR;
    }
	pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
	sect_idx = addr / pDrvCtrl->FLASH.sectSize;
	ret = vxQspiFlash_Erase_Sect(pDev, sect_idx);
	
	return ret;
}
	
STATUS tffs_qspiFlashSectorErase_Idx (UINT32 log_idx)
{
	int ret = 0, phy_idx = 0;
	
	VXB_DEVICE_ID pDev;
	FM_QSPI_DRV_CTRL* pDrvCtrl;
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, 0);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", 0);
        return ERROR;
    }
	pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
	/*phy_idx = vxQspiFlash_SectIdx_LogtoPhy(pDev, log_idx);*/
	
	ret = vxQspiFlash_Erase_Sect(pDev, log_idx);
	return ret;
}

/*******************************************************************************
*
* qspiFlashRead - read pDev flash device
*
* This routine fills the read buffer supplied in the parameters.
*
* SEE ALSO: qspiFlashPageWrite()
*
* RETURNS: OK, or ERROR if timeout or parameter is invalid.
*
* ERRNO: N/A
*/

STATUS tffs_qspiFlashRead
    (
    UINT32         offset,         /* The address to read */
    char *      readBuffer,     /* A pointer to a location to read the data */
    int         byteLen         /* The size to read */
    )
{
#if QSPI_DIRECT_MODE
	return vxQspiFlash_ReadBytes_Direct0(offset, byteLen, readBuffer);
#else
	return vxQspiFlash_ReadBytes_InDirect0(offset, byteLen, readBuffer);	
#endif	
}

/*******************************************************************************
*
* qspiFlashPageWrite - write pDev flash device
*
* This routine fills the write buffer supplied in the parameters to flash.
* The byte length must not exceed than one page.
*
* SEE ALSO: qspiFlashRead()
*
* RETURNS: OK, or ERROR if timeout or parameter is invalid.
*
* ERRNO: N/A
*/

STATUS tffs_qspiFlashPageWrite
    (
    UINT32         addr,           /* byte offset into flash memory */
    char *      buf,            /* buffer */
    int         byteLen         /* size of bytes */
    )
{	
#if QSPI_DIRECT_MODE
	return vxQspiFlash_WriteBytes_Direct0(addr, byteLen, buf);
#else
	return vxQspiFlash_WriteBytes_InDirect0(addr, byteLen, buf); /* InDirect*/
#endif
}

#endif

#if 0

int vxQspiCtrl_FastRcv_Setup(VXB_DEVICE_ID pDev, UINT32 flash_offset, UINT32 byteCnt, UINT8* rcvBuf)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
		
    UINT32 cfgReg = 0;
    UINT32 waterMark = 480;    
    
	QSPI_SEM_TAKE(pDrvCtrl->semSync);
    
    pDrvCtrl->CTRL.requestBytes = byteCnt;
    pDrvCtrl->CTRL.remainBytes = byteCnt;
    pDrvCtrl->CTRL.rcvBuf = rcvBuf;
    
    /* 
    set flash address where read data (address in flash) 
	*/
    /*vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IRTSAR_OFFSET, (pDrvCtrl->CTRL.fifoBaseAddr + flash_offset));  */
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IRTSAR_OFFSET, (flash_offset));  
	
    /* 
    set number of bytes to be transferred 
	*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IRTNBR_OFFSET, pDrvCtrl->CTRL.requestBytes);  
	
    /* 
    set controller trig adress where read data (adress in controller) 
	*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IAATR_OFFSET, pDrvCtrl->CTRL.fifoBaseAddr);    
	
    /* 
    set Trigger Address Range (2^15) 
	*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_ITARR_OFFSET, 0x0F);  
	
    /* 
    set WaterMark Register 
	*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IWTWR_OFFSET, 0x00); 
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IRTWR_OFFSET, waterMark);  
	
    /* 
    trigger Indirect Write access
    */
    cfgReg = 0x01;
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IRTCR_OFFSET, cfgReg);  
	
	QSPI_SEM_GIVE(pDrvCtrl->semSync);
	
    return OK;
}


int vxQspiCtrl_FastSnd_Setup(VXB_DEVICE_ID pDev, UINT32 flash_offset, UINT32 byteCnt, UINT8* sndBuf)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
		
    UINT32 cfgReg = 0;
    UINT32 waterMark = 32;    
    
	QSPI_SEM_TAKE(pDrvCtrl->semSync);
    
    pDrvCtrl->CTRL.requestBytes = byteCnt;
    pDrvCtrl->CTRL.remainBytes = byteCnt;
    pDrvCtrl->CTRL.sndBuf = sndBuf;
    
    /* 
    set flash address where write data (address in flash) 
	*/
    /*vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IWTSAR_OFFSET, (pDrvCtrl->CTRL.fifoBaseAddr + flash_offset)); 	*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IWTSAR_OFFSET, (flash_offset));  
	
    /* 
    set number of bytes to be transferred 
	*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IWTNBR_OFFSET, pDrvCtrl->CTRL.requestBytes);  
	
    /* 
    set controller trig adress where write data (adress in controller) 
	*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IAATR_OFFSET, pDrvCtrl->CTRL.fifoBaseAddr);    
	
    /* 
    set Trigger Address Range (2^15) 
	*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_ITARR_OFFSET, 0xf);      
	
    /* 
    set WaterMark Register (between 10~250 if page_size is 256 bytes) 
	*/
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IWTWR_OFFSET, waterMark);   
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IRTWR_OFFSET, 0x00);   
	
    /* 
    trigger Indirect Write access 
	*/
    cfgReg = 0x01;
    vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IWTCR_OFFSET, cfgReg);
    
	QSPI_SEM_GIVE(pDrvCtrl->semSync);
	
    return OK;
}


int vxQspiFlash_Read_Sect(VXB_DEVICE_ID pDev, int sect_idx, UINT8* rcvBuf8)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
	int ret = 0;
	UINT32 offset = 0;
	UINT32 byteCnt = 0, dataCnt = 0;
	UINT8 * pBuf8 = NULL;
	int idx = 0;
	int timeout = 0, j = 0;
	
	offset = sect_idx * pDrvCtrl->FLASH.sectSize;
	byteCnt = pDrvCtrl->FLASH.sectSize;	
	
	/**/
	/* clear Rcv Fifo*/
	/**/
	vxQspiCtrl_Clr_RcvFifo(pDev);

	/*
	setup fastRcv
	*/
	vxQspiCtrl_FastRcv_Setup(pDev, offset, byteCnt, rcvBuf8);
	
	pBuf8 = (UINT8 *)pDrvCtrl->CTRL.fifoBaseAddr;
	idx = 0;	
	do 
	{
		/* 	Read  Watermark */
		dataCnt = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_SFLR_OFFSET) * 4;		
		if (dataCnt == 0)
		{
			sysUsDelay(1);
		}
		else
		{		
			/* 
			Read data from SRAM 
			*/
			memcpy((UINT8*)pDrvCtrl->CTRL.rcvBuf, (UINT8*)(&pBuf8[idx]), dataCnt);
			idx += dataCnt;
			
			pDrvCtrl->CTRL.rcvBuf += dataCnt;
			pDrvCtrl->CTRL.remainBytes -= dataCnt;
		}
	} while (pDrvCtrl->CTRL.remainBytes > 0);
		
printf("4: vxQspiCtrl_Rd_CfgReg32, memcpy \n");
	
	timeout = QSPI_TIMEOUT;
	do 
	{
		ret = vxQspiCtrl_Wait_ExeOk(pDev, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK, OK_FLAG_1);
		if (ret == ERROR)
		{
			sysUsDelay(10);		
		}
		else
		{
			break;
		}
		
		timeout--;
		if (timeout <= 0)
		{
			byteCnt = 0;
		}
	} while (timeout > 0);
		
	printf("5: vxQspiCtrl_Wait_ExeOk:timeout(%d),ret=%d \n", timeout, ret);

	/* 
	clear QSPI_IND_RD_XFER_DONE bit 
	*/
	vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK);

	return byteCnt;
}

int vxQspiFlash_Write_Sect(VXB_DEVICE_ID pDev, int sect_idx, UINT8* sndBuf8)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
		
	int ret = 0;
	UINT32 offset = 0;
	UINT32 byteCnt = 0, dataCnt = 0;
	UINT8 * pBuf8 = NULL;
	int idx = 0;
	int timeout = 0, j = 0;
	
	offset = sect_idx * pDrvCtrl->FLASH.sectSize;
	byteCnt = pDrvCtrl->FLASH.sectSize;	

	/*
	unlock flash
	*/
	vxQspiFlash_UnLock_Flash(pDev);

	/*
	setup fastSnd
	*/
	vxQspiCtrl_FastSnd_Setup(pDev, offset, byteCnt, sndBuf8);


	/* copy data from sram to flash */	
	pBuf8 = (UINT8*)pDrvCtrl->CTRL.fifoBaseAddr;
	idx = 0;
	do 
	{
		dataCnt = QSPIFLASH_PAGE_256;		
		if (pDrvCtrl->CTRL.remainBytes < QSPIFLASH_PAGE_256)
		{
			dataCnt = pDrvCtrl->CTRL.remainBytes;
		}
		
		memcpy((UINT8*)(&pBuf8[idx]), (const UINT8*)pDrvCtrl->CTRL.sndBuf, dataCnt);
		idx += dataCnt;
		
		pDrvCtrl->CTRL.sndBuf += dataCnt;
		pDrvCtrl->CTRL.remainBytes -= dataCnt;		
		
	} while (pDrvCtrl->CTRL.remainBytes > 0);

	timeout = QSPI_TIMEOUT;
	do 
	{
		ret = vxQspiCtrl_Wait_ExeOk(pDev, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK, OK_FLAG_1);
		if (ret == ERROR)
		{
			sysUsDelay(10);		
		}
		else
		{
			break;
		}
		
		timeout--;
		if (timeout <= 0)
		{
			byteCnt = 0;
		}
	} while (timeout > 0);
	
	/* 
	clear QSPI_IND_WR_XFER_DONE bit 
	*/
	vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK);

	return byteCnt;
}

int vxQspiFlash_Erase_Sector(VXB_DEVICE_ID pDev, int sect_idx)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	int ctrl_x = 0;
	
    int ret = 0;
    UINT32 sect_offset = 0;
    UINT32 sectEnd = 0;

	QSPI_SEM_TAKE(pDrvCtrl->semSync);

	ctrl_x = pDrvCtrl->CTRL.ctrl_x;    
	
	/**/
	/* UnLock flash */
	/**/
	vxQspiFlash_UnLock_Flash(pDev);
	    
    /* 
    calculate sector start address to be erased 
	*/
    sect_offset = (sect_idx * pDrvCtrl->FLASH.sectSize)  & (~(pDrvCtrl->FLASH.sectSize-1));
	
    /* 
    Erase Flash with required sectors 
	*/
    ret = vxQspiFlash_SectErase_CmdExe(pDev, sect_offset, SE_CMD);
    if(ret == ERROR)
    {
		QSPI_DBG2("ctrl_%d: vxQspiFlash_Erase_Sector(%d) fail: cmdExe! \n", ctrl_x, sect_idx, 3,4,5,6);
        goto err;
    }
	
	QSPI_SEM_GIVE(pDrvCtrl->semSync);

    return OK;

err:	
	QSPI_SEM_GIVE(pDrvCtrl->semSync);
    return ERROR;	
}

int vxQspiFlash_Erase_Sector2(VXB_DEVICE_ID pDev, int sect_addr)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	int ctrl_x = 0;
	
    int ret = 0;
    UINT32 sect_offset = 0;
    UINT32 sectEnd = 0;

	QSPI_SEM_TAKE(pDrvCtrl->semSync);

	ctrl_x = pDrvCtrl->CTRL.ctrl_x;    
	
	/**/
	/* UnLock flash */
	/**/
	vxQspiFlash_UnLock_Flash(pDev);
	    
    /* 
    calculate sector start address to be erased 
	*/
    sect_offset = sect_addr;
	
    /* 
    Erase Flash with required sectors 
	*/
    ret = vxQspiFlash_SectErase_CmdExe(pDev, sect_offset, SE_CMD);
    if(ret == ERROR)
    {
		QSPI_DBG2("ctrl_%d: vxQspiFlash_Erase_Sector(0x%X) fail: cmdExe! \n", ctrl_x, sect_offset, 3,4,5,6);
        goto err;
    }
	
	QSPI_SEM_GIVE(pDrvCtrl->semSync);

    return OK;

err:	
	QSPI_SEM_GIVE(pDrvCtrl->semSync);
    return ERROR;	
}

int vxQspiFlash_Read_Page(VXB_DEVICE_ID pDev, int page_idx, UINT8* rcvBuf8)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
	int ret = 0;
	UINT32 offset = 0;
	UINT32 byteCnt = 0, dataCnt = 0;
	UINT8 * pBuf8 = NULL;
	int idx = 0;
	int timeout = 0, j = 0;
	
	offset = page_idx * pDrvCtrl->FLASH.pageSize;
	byteCnt = pDrvCtrl->FLASH.pageSize;	
	
	/**/
	/* clear Rcv Fifo*/
	/**/
	vxQspiCtrl_Clr_RcvFifo(pDev);

	/*
	setup fastRcv
	*/
	vxQspiCtrl_FastRcv_Setup(pDev, offset, byteCnt, rcvBuf8);
	
	pBuf8 = (UINT8 *)pDrvCtrl->CTRL.fifoBaseAddr;
	idx = 0;	
	do 
	{
		/* 	Read  Watermark */
		dataCnt = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_SFLR_OFFSET) * 4;		
		if (dataCnt == 0)
		{
			sysUsDelay(1);
		}
		else
		{		
			/* 
			Read data from SRAM 
			*/
			printf("pDrvCtrl->CTRL.rcvBuf:0x%X \n", pDrvCtrl->CTRL.rcvBuf);
			printf("pBuf8[0x%X]:0x%X \n", idx, &pBuf8[idx]);
			
			memcpy((UINT8*)pDrvCtrl->CTRL.rcvBuf, (UINT8*)(&pBuf8[idx]), dataCnt);
			idx += dataCnt;
			
			pDrvCtrl->CTRL.rcvBuf += dataCnt;
			pDrvCtrl->CTRL.remainBytes -= dataCnt;
		}
	} while (pDrvCtrl->CTRL.remainBytes > 0);
	
	timeout = QSPI_TIMEOUT;
	do 
	{
		ret = vxQspiCtrl_Wait_ExeOk(pDev, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK, OK_FLAG_1);
		if (ret == ERROR)
		{
			sysUsDelay(10);		
		}
		else
		{
			break;
		}
		
		timeout--;
		if (timeout <= 0)
		{
			byteCnt = 0;
		}
	} while (timeout > 0);

	/* 
	clear QSPI_IND_RD_XFER_DONE bit 
	*/
	vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK);

	return byteCnt;
}


int vxQspiFlash_Write_Page(VXB_DEVICE_ID pDev, int page_idx, UINT8* sndBuf8)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
		
	int ret = 0;
	UINT32 offset = 0;
	UINT32 byteCnt = 0, dataCnt = 0;
	UINT8 * pBuf8 = NULL;
	int idx = 0;
	int timeout = 0, j = 0;
	
	offset = page_idx * pDrvCtrl->FLASH.pageSize;
	byteCnt = pDrvCtrl->FLASH.pageSize;	

	/*
	unlock flash
	*/
	vxQspiFlash_UnLock_Flash(pDev);

	/*
	setup fastSnd
	*/
	vxQspiCtrl_FastSnd_Setup(pDev, offset, byteCnt, sndBuf8);


	/* copy data from sram to flash */	
	pBuf8 = (UINT8*)pDrvCtrl->CTRL.fifoBaseAddr;
	idx = 0;
	do 
	{
		dataCnt = QSPIFLASH_PAGE_256;		
		if (pDrvCtrl->CTRL.remainBytes < QSPIFLASH_PAGE_256)
		{
			dataCnt = pDrvCtrl->CTRL.remainBytes;
		}
		
		memcpy((UINT8*)(&pBuf8[idx]), (const UINT8*)pDrvCtrl->CTRL.sndBuf, dataCnt);
		idx += dataCnt;
		
		pDrvCtrl->CTRL.sndBuf += dataCnt;
		pDrvCtrl->CTRL.remainBytes -= dataCnt;		
		
	} while (pDrvCtrl->CTRL.remainBytes > 0);

	timeout = QSPI_TIMEOUT;
	do 
	{
		ret = vxQspiCtrl_Wait_ExeOk(pDev, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK, OK_FLAG_1);
		if (ret == ERROR)
		{
			sysUsDelay(10);		
		}
		else
		{
			break;
		}
		
		timeout--;
		if (timeout <= 0)
		{
			byteCnt = 0;
			/*goto ERR;*/
		}
	} while (timeout > 0);
	
	/* 
	clear QSPI_IND_WR_XFER_DONE bit 
	*/
	vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK);

	return byteCnt;
}

int vxQspiFlash_Read_Bytes(VXB_DEVICE_ID pDev, int addr_offset, int len_bytes, UINT8* rcvBuf8)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
	
	int ret = 0;
	UINT32 offset = 0;
	UINT32 byteCnt = 0, dataCnt = 0;
	UINT8 * pBuf8 = NULL;
	int idx = 0;
	int timeout = 0, j = 0;
	
	/*offset = page_idx *pDrvCtrl->FLASH.pageSize;*/
	/*byteCnt = pDrvCtrl->FLASH.pageSize;	*/
	offset = addr_offset;
	byteCnt = len_bytes;	
	
	/**/
	/* clear Rcv Fifo*/
	/**/
	vxQspiCtrl_Clr_RcvFifo(pDev);

	/*
	setup fastRcv
	*/
	vxQspiCtrl_FastRcv_Setup(pDev, offset, byteCnt, rcvBuf8);
	
	pBuf8 = (UINT8 *)pDrvCtrl->CTRL.fifoBaseAddr;
	idx = 0;	
	do 
	{
		/* 	Read  Watermark */
		dataCnt = vxQspiCtrl_Rd_CfgReg32(pDev, QSPIPS_SFLR_OFFSET) * 4;		
		if (dataCnt == 0)
		{
			sysUsDelay(1);
		}
		else
		{		
			/* 
			Read data from SRAM 
			*/
			/*printf("pDrvCtrl->CTRL.rcvBuf:0x%X \n", pDrvCtrl->CTRL.rcvBuf);*/
			/*printf("pBuf8[0x%X]:0x%X \n", idx, &pBuf8[idx]);*/
			
			memcpy((UINT8*)pDrvCtrl->CTRL.rcvBuf, (UINT8*)(&pBuf8[idx]), dataCnt);
			idx += dataCnt;
			
			pDrvCtrl->CTRL.rcvBuf += dataCnt;
			pDrvCtrl->CTRL.remainBytes -= dataCnt;
		}
	} while (pDrvCtrl->CTRL.remainBytes > 0);
	
	timeout = QSPI_TIMEOUT;
	do 
	{
		ret = vxQspiCtrl_Wait_ExeOk(pDev, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK, OK_FLAG_1);
		if (ret == ERROR)
		{
			sysUsDelay(10);		
		}
		else
		{
			break;
		}
		
		timeout--;
		if (timeout <= 0)
		{
			byteCnt = 0;
		}
	} while (timeout > 0);

	/* 
	clear QSPI_IND_RD_XFER_DONE bit 
	*/
	vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IRTCR_OFFSET, QSPIPS_IRTCR_COMPLETE_MASK);

	return byteCnt;
}

int vxQspiFlash_Write_Bytes(VXB_DEVICE_ID pDev, int addr_offset, int len_bytes, UINT8* sndBuf8)
{
	FM_QSPI_DRV_CTRL* pDrvCtrl = (FM_QSPI_DRV_CTRL*)pDev->pDrvCtrl;
		
	int ret = 0;
	UINT32 offset = 0;
	UINT32 byteCnt = 0, dataCnt = 0;
	UINT8 * pBuf8 = NULL;
	int idx = 0;
	int timeout = 0, j = 0;
	
	/*offset = page_idx *pDrvCtrl->FLASH.pageSize;*/
	/*byteCnt = pDrvCtrl->FLASH.pageSize;	*/
	offset = addr_offset;
	byteCnt = len_bytes;	

	/*
	unlock flash
	*/
	vxQspiFlash_UnLock_Flash(pDev);

	/*
	setup fastSnd
	*/
	vxQspiCtrl_FastSnd_Setup(pDev, offset, byteCnt, sndBuf8);


	/* copy data from sram to flash */	
	pBuf8 = (UINT8*)pDrvCtrl->CTRL.fifoBaseAddr;
	idx = 0;
	do 
	{
		dataCnt = QSPIFLASH_PAGE_256;		
		if (pDrvCtrl->CTRL.remainBytes < QSPIFLASH_PAGE_256)
		{
			dataCnt = pDrvCtrl->CTRL.remainBytes;
		}
		
		memcpy((UINT8*)(&pBuf8[idx]), (const UINT8*)pDrvCtrl->CTRL.sndBuf, dataCnt);
		idx += dataCnt;
		
		pDrvCtrl->CTRL.sndBuf += dataCnt;
		pDrvCtrl->CTRL.remainBytes -= dataCnt;		
		
	} while (pDrvCtrl->CTRL.remainBytes > 0);

	timeout = QSPI_TIMEOUT;
	do 
	{
		ret = vxQspiCtrl_Wait_ExeOk(pDev, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK, OK_FLAG_1);
		if (ret == ERROR)
		{
			sysUsDelay(10);		
		}
		else
		{
			break;
		}
		
		timeout--;
		if (timeout <= 0)
		{
			byteCnt = 0;
			/*goto ERR;*/
		}
	} while (timeout > 0);
	
	/* 
	clear QSPI_IND_WR_XFER_DONE bit 
	*/
	vxQspiCtrl_Wr_CfgReg32(pDev, QSPIPS_IWTCR_OFFSET, QSPIPS_IWTCR_COMPLETE_MASK);

	return byteCnt;
}

#endif


#if 0

/*******************************************************************************
*
* qspiFlashInit1 - initialize Altera Soc Gen 5 controller.
*
* This function initializes Altera Soc Gen 5 controller stage 1.
*
* RETURNS: N/A.
*
* ERRNO
*/
void qspiFlashInit1 (void)
{
    /* reset qspi controller and qspi flash chip */

    /*qspiFlashReset ();*/
    /*qspiFlashResetChip ();*/
    
#if 1	
	UINT32 tmp32 = slcr_read(SLCR_QSPI0_CTRL);
	/* set 1*/
	tmp32 |= (QSPI_APB_RST | QSPI_AHB_RST | QSPI_REF_RST);
	slcr_write(SLCR_QSPI0_CTRL, tmp32);
	
	/* set 0*/
	tmp32 &= ~(QSPI_APB_RST | QSPI_AHB_RST | QSPI_REF_RST);
	slcr_write(SLCR_QSPI0_CTRL, tmp32);
#endif

#if 0
	/*
	reg[00] = 0x80080081 
	reg[04] = 0x0802006B 
	reg[08] = 0x00000002 
	reg[0C] = 0x00000000 
	reg[10] = 0x00000001 
	reg[14] = 0x00000002 
	reg[18] = 0x00000080 
	reg[1C] = 0xE8000000 
	*/
	UINT32 base = 0xE0000000;
	*((UINT32*)(base+0x00)) = 0x80080081;
	*((UINT32*)(base+0x04)) = 0x0802006B; 
	*((UINT32*)(base+0x08)) = 0x00000002; 
	*((UINT32*)(base+0x0C)) = 0x00000000; 
	*((UINT32*)(base+0x10)) = 0x00000001; 
	*((UINT32*)(base+0x14)) = 0x00000002; 
	*((UINT32*)(base+0x18)) = 0x00000080; 
	*((UINT32*)(base+0x1C)) = 0xE8000000; 
#endif

    return;
}

/*******************************************************************************
*
* qspiFlashInit2 - initialize Altera Soc Gen 5 controller.
*
* This function initializes Altera Soc Gen 5 controller stage 2.
*
* RETURNS: N/A.
*
* ERRNO
*/

 void qspiFlashInit2 (void)
{
    /*
    int i = 0;

    for (i = 0; i < QSPI_MAX_DEVICE; i++)
        QspiDev [i].sem = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE |
                                      SEM_INVERSION_SAFE);
	*/
	return;
}

/*******************************************************************************
*
* qspiFlashDirectReadMap - Map for qspi flash for direct read access
*
* This routine provides mapping for qspi flash for direct read access.
*
* RETURNS: OK, or ERROR if timeout or parameter is invalid.
*
* ERRNO: N/A
*/

STATUS qspiFlashDirectReadMap
    (
    int         offset,         /* The address to read */
    int         byteLen         /* The size to read */
    )
{
    int i = 0;
    int timeout = 0;
    UINT32 data = 0;
    UINT32 bytesRemain = 0;
    UINT32 status = 0;

   /* UINT32 *spiDataAddr = (UINT32 * QSPI_CTRLR_DATA_REGISTER_ADDR;*/
   /* MAP_flash_cmd_ctrl_reg_t flashcmdRegVal;*/

    if ((offset < 0) || (byteLen < 0) || ( byteLen > QSPI_DIRECT_MAP_SIZE ) ||
        ((offset + byteLen) > SPI_FLASH_MEM_SIZE))
        {
        printf("qspiFlashDirectReadMap: invalid parameters\n");
        return (ERROR);
        }

    /*QSPI_SEM_TAKE (QspiDev [0].sem);*/

    /* set read transfer start address */
    /*QSPI_REG_WRITE_32 (0, QSPI_REMAP_ADDR, offset);*/
    /* #define QSPI_REMAP_ADDR  0x24*/
	FMQL_WRITE_32((0xE0000000 + 0x24), offset);

    /* enable memory map mode */

    /* set memory map address */
   /* QSPI_SEM_GIVE (QspiDev [0].sem);*/

    return OK;
}


int g_Qspi_Init_vxb = 0;
 
/*******************************************************************************
*
* qspiFlashSectorErase - erase the specified flash sector
*
* This routine erases the specified sector, all bits in sector are
* set to 1.
*
* RETURNS: OK, or ERROR if timeout or parameter is invalid.
*
* ERRNO: N/A.
*/

STATUS tffs_qspiFlashSectorErase
    (
    int addr
    )
{
    VXB_DEVICE_ID pDev;
    FM_QSPI_DRV_CTRL * pDrvCtrl;
	int ret = 0;
	
	int unit = 0;     /* ctrl_0 */
	int sect_idx = 0;
	
	if (g_Qspi_Init_vxb == 0)
	{
		vxQspiRegister2();
		g_Qspi_Init_vxb = 1;
	}
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, unit);
    if (pDev == NULL)
    {
        printf ("Cannot find pDev unit(%d)->erase:addr(0x%X)!  \n\n", unit, addr);
        return ERROR;
    }	
	pDrvCtrl= (FM_QSPI_DRV_CTRL *)pDev->pDrvCtrl;

#if 0	
	sect_idx = addr / pDrvCtrl->FLASH.sectSize;
	ret = vxQspiFlash_Erase_Sector(pDev, sect_idx);
#else
	ret = vxQspiFlash_Erase_Sector2(pDev, addr);
#endif

	return ret;
}

STATUS tffs_qspiFlashSectorErase_Idx (UINT32 log_idx)
{
    VXB_DEVICE_ID pDev;
    FM_QSPI_DRV_CTRL * pDrvCtrl;
	int ret = 0;
	
	int unit = 0;     /* ctrl_0 */
	int sect_idx = 0;
	
	if (g_Qspi_Init_vxb == 0)
	{
		vxQspiRegister2();
		g_Qspi_Init_vxb = 1;
	}
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, unit);
    if (pDev == NULL)
    {
        printf ("Cannot find pDev unit(%d)!  \n\n", unit);
        return ERROR;
    }	
	pDrvCtrl= (FM_QSPI_DRV_CTRL *)pDev->pDrvCtrl;
	
	/*phy_idx = vxQspiFlash_SectIdx_LogtoPhy(pDev, log_idx);*/
	
	sect_idx = log_idx;
	ret = vxQspiFlash_Erase_Sector(pDev, sect_idx);
	
	return ret;
}

/*******************************************************************************
*
* qspiFlashRead - read qspi flash device
*
* This routine fills the read buffer supplied in the parameters.
*
* SEE ALSO: qspiFlashPageWrite()
*
* RETURNS: OK, or ERROR if timeout or parameter is invalid.
*
* ERRNO: N/A
*/

STATUS tffs_qspiFlashRead
    (
    int         offset,         /* The address to read */
    char *      readBuffer,     /* A pointer to a location to read the data */
    int         byteLen         /* The size to read */
    )
{
    VXB_DEVICE_ID pDev;
    FM_QSPI_DRV_CTRL * pDrvCtrl;
	int ret = 0;
	
	int unit = 0;     /* ctrl_0 */

	int i = 0;
	int page_idx = 0, page_num = 0, pageSize = 0;
	UINT8* rcvBuf8 = NULL;
	
	if (g_Qspi_Init_vxb == 0)
	{
		vxQspiRegister2();
		g_Qspi_Init_vxb = 1;
	}
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, unit);
    if (pDev == NULL)
    {
        printf ("Cannot find pDev unit(%d)->read:addr(0x%X)len(%d)!  \n\n", unit, offset, byteLen);
        return ERROR;
    }
	pDrvCtrl= (FM_QSPI_DRV_CTRL *)pDev->pDrvCtrl;

#if 0	
	pageSize = pDrvCtrl->FLASH.pageSize;
	if ((byteLen % pageSize) == 0)
	{
		page_num = byteLen / pageSize;
	}
	else
	{
		page_num = (byteLen / pageSize) + 1;
	}

	for (i=0; i<page_num; i++)
	{
		page_idx = offset / pageSize + i;
		rcvBuf8 = readBuffer + i*pageSize;
		
		printf("rdPage->rcvBuf8:0x%X, page_num:%d, offset:0x%X, pageSize:%d \n", \
			    rcvBuf8, page_num, offset, pageSize);
		
		ret = vxQspiFlash_Read_Page(pDev, page_idx, rcvBuf8);
		if (ret == ERROR)
		{
			return ERROR;
		}
	}
	
#else

	ret = vxQspiFlash_Read_Bytes(pDev, offset, byteLen, readBuffer);
	if (ret == ERROR)
	{
		return ERROR;
	}
#endif

	return OK;
}

/*******************************************************************************
*
* qspiFlashPageWrite - write qspi flash device
*
* This routine fills the write buffer supplied in the parameters to flash.
* The byte length must not exceed than one page.
*
* SEE ALSO: qspiFlashRead()
*
* RETURNS: OK, or ERROR if timeout or parameter is invalid.
*
* ERRNO: N/A
*/

STATUS tffs_qspiFlashPageWrite
    (
    int         addr,           /* byte offset into flash memory */
    char *      buf,            /* buffer */
    int         byteLen         /* size of bytes */
    )
{
    VXB_DEVICE_ID pDev;
    FM_QSPI_DRV_CTRL * pDrvCtrl;
	int ret = 0;
	
	int unit = 0;     /* ctrl_0 */
	int i = 0;
	int page_idx = 0, page_num = 0, pageSize = 0;
	UINT8* sndBuf8 = NULL;
	
	if (g_Qspi_Init_vxb == 0)
	{
		vxQspiRegister2();
		g_Qspi_Init_vxb = 1;
	}
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, unit);
    if (pDev == NULL)
    {
        printf ("Cannot find pDev unit(%d)->pageWrite:addr(0x%X)len(%d)!  \n\n", unit, addr, byteLen);
        return ERROR;
    }
	pDrvCtrl= (FM_QSPI_DRV_CTRL *)pDev->pDrvCtrl;

#if 0	
	pageSize = pDrvCtrl->FLASH.pageSize;
	if ((byteLen % pageSize) == 0)
	{
		page_num = byteLen / pageSize;
	}
	else
	{
		page_num = (byteLen / pageSize) + 1;
	}
	
	for (i=0; i<page_num; i++)
	{
		page_idx = addr / pageSize + i;
		sndBuf8 = buf + i*pageSize;
		
		ret = vxQspiFlash_Write_Page(pDev, page_idx, sndBuf8);
		if (ret == ERROR)
		{
			return ERROR;
		}
	}
#else
	ret = vxQspiFlash_Write_Bytes(pDev, addr, byteLen, buf);
	if (ret == ERROR)
	{
		return ERROR;
	}

#endif

	return OK;
}


#endif



#if 1
UINT8 RcvBuf3[QSPIFLASH_SECTOR_64K+16] = {0};
UINT8 SndBuf3[QSPIFLASH_SECTOR_64K+16] = {0};

STATUS test_qspi_rd_sect_vx(int ctrl_x, int sect_idx)
{
    VXB_DEVICE_ID       pDev;
    FM_QSPI_DRV_CTRL * pDrvCtrl;

	int ret = 0;
    int i;
	int unit = 0;
	
	UINT8 * pBuf8 = (UINT8 *)(&RcvBuf3[0]);
	int j = 0, byteCnt = 0;
	
	switch (ctrl_x)
	{
	case QSPI_CTRL_0:
		unit = 0;
		break;
	
	case QSPI_CTRL_1:
		unit = 1;
		break;		
	}
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, unit);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", unit);
        return ERROR;
    }
	else
	{
		pDrvCtrl= (FM_QSPI_DRV_CTRL *)pDev->pDrvCtrl;
		printf ("\nvxbInstByNameFind(%s): 0x%X \n\n", FM_QSPI_NAME, pDev);
	}	

	
	byteCnt = pDrvCtrl->FLASH.sectSize;	
	bzero((UINT8 *)(&RcvBuf3[0]), byteCnt);
	
	ret = vxQspiFlash_Read_Sect(pDev, sect_idx, pBuf8);
	if (ret == ERROR)
	{
		printf("----pDev(%d)-read sector(%d):fail!----\n", ctrl_x, sect_idx);	
		return;
	}	

	printf("----pDev(%d)-read(%d):%dKB----\n", pDrvCtrl->CTRL.ctrl_x, sect_idx, (byteCnt/1024));	
	
	for (j=0; j<512; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}

	printf(" ... ... ... ... \n\n");
	
	for (j=(byteCnt-512); j<byteCnt; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf("------------\n");

	printf("\n--compare read & write -- \n");
	for (i=0; i<byteCnt; i++)
	{	
		if (RcvBuf3[i] != SndBuf3[i])
		{
			printf("read & write different at(%d): 0x%02X(R) != 0x%02X(W) \n", i, RcvBuf3[i], SndBuf3[i]);
			return OK;
		}
	}
	printf("QspiFlash: read & write same at sector(%d)! \n", sect_idx);
	printf("------------\n\n");
	
	return OK;
}


int g_test_qspi3 = 0;
void test_qspi_wr_sect_vx(int ctrl_x, int sect_idx)
{
    VXB_DEVICE_ID      pDev;
    FM_QSPI_DRV_CTRL * pDrvCtrl;

	int ret = 0;
    int i;
	int unit = 0;
	
	UINT8 * pBuf8 = (UINT8 *)(&SndBuf3[0]);
	int j = 0, byteCnt = 0;
	
	switch (ctrl_x)
	{
	case QSPI_CTRL_0:
		unit = 0;
		break;
	
	case QSPI_CTRL_1:
		unit = 1;
		break;		
	}
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, unit);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", unit);
        return ERROR;
    }
	else
	{
		pDrvCtrl= (FM_QSPI_DRV_CTRL *)pDev->pDrvCtrl;
		printf ("\nvxbInstByNameFind(%s): 0x%X \n\n", FM_QSPI_NAME, pDev);
	}	

	
	byteCnt = pDrvCtrl->FLASH.sectSize;
	
	for (i=0; i<byteCnt; i++)
	{
		pBuf8[i] = g_test_qspi3 + i;
	}
	g_test_qspi3++;
	
	ret = vxQspiFlash_Write_Sect(pDev, sect_idx, pBuf8);
	if (ret == ERROR)
	{
		printf("----pDev(%d)-write sector(%d):fail!----\n", ctrl_x, sect_idx);	
		return;
	}	
	
	for (j=0; j<512; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}

	printf(" ... ... ... ... \n\n");
	
	for (j=(byteCnt-512); j<byteCnt; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	printf("------------\n");

	return;
}

void test_qspi_erase_sect_vx(int ctrl_x, int sect_idx)
{
    VXB_DEVICE_ID      pDev;
    FM_QSPI_DRV_CTRL * pDrvCtrl;
	
	int ret = 0;
	int unit = 0;
	
	switch (ctrl_x)
	{
	case QSPI_CTRL_0:
		unit = 0;
		break;
	
	case QSPI_CTRL_1:
		unit = 1;
		break;		
	}
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, unit);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", unit);
        return ERROR;
    }
	else
	{
		pDrvCtrl= (FM_QSPI_DRV_CTRL *)pDev->pDrvCtrl;
		printf ("\nvxbInstByNameFind(%s): 0x%X \n\n", FM_QSPI_NAME, pDev);
	}
	
	ret = vxQspiFlash_Erase_Sect(pDev, sect_idx);
    if (ret == ERROR)
    {
    	printf("\nctrl_%d: qspiflash erase sect: %d fail! \n\n", ctrl_x, sect_idx);
    }
	else
	{
		printf("\nctrl_%d: qspiflash erase sect: %d ok! \n\n", ctrl_x, sect_idx);
	}
	
	return;
}

STATUS test_qspi_rd_page_vx(int ctrl_x, int page_idx)
{
    VXB_DEVICE_ID       pDev;
    FM_QSPI_DRV_CTRL * pDrvCtrl;

	int ret = 0;
    int i;
	int unit = 0;
	
	UINT8 * pBuf8 = (UINT8 *)(&RcvBuf3[0]);
	int j = 0, byteCnt = 0;
	
	switch (ctrl_x)
	{
	case QSPI_CTRL_0:
		unit = 0;
		break;
	
	case QSPI_CTRL_1:
		unit = 1;
		break;		
	}
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, unit);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", unit);
        return ERROR;
    }
	else
	{
		pDrvCtrl= (FM_QSPI_DRV_CTRL *)pDev->pDrvCtrl;
		printf ("\nvxbInstByNameFind(%s): 0x%X \n\n", FM_QSPI_NAME, pDev);
	}	

	
	byteCnt = pDrvCtrl->FLASH.pageSize;	
	bzero((UINT8 *)(&RcvBuf3[0]), byteCnt);
	
	ret = vxQspiFlash_Read_Page(pDev, page_idx, pBuf8);
	if (ret == ERROR)
	{
		printf("----pDev(%d)-read page(%d):fail!----\n", ctrl_x, page_idx);	
		return;
	}	

	printf("----pDev(%d)-read page(%d):%d Bytes----\n", pDrvCtrl->CTRL.ctrl_x, page_idx, (byteCnt));	
	
	for (j=0; j<byteCnt; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}
	
	printf("------------\n");

	printf("\n--compare read & write -- \n");
	for (i=0; i<byteCnt; i++)
	{	
		if (RcvBuf3[i] != SndBuf3[i])
		{
			printf("read & write different at(%d): 0x%02X(R) != 0x%02X(W) \n", i, RcvBuf3[i], SndBuf3[i]);
			return OK;
		}
	}
	printf("QspiFlash: read & write same at page(%d)! \n", page_idx);
	printf("------------\n\n");
	
	return OK;
}

void test_qspi_wr_page_vx(int ctrl_x, int page_idx)
{
    VXB_DEVICE_ID      pDev;
    FM_QSPI_DRV_CTRL * pDrvCtrl;

	int ret = 0;
    int i;
	int unit = 0;
	
	UINT8 * pBuf8 = (UINT8 *)(&SndBuf3[0]);
	int j = 0, byteCnt = 0;
	
	switch (ctrl_x)
	{
	case QSPI_CTRL_0:
		unit = 0;
		break;
	
	case QSPI_CTRL_1:
		unit = 1;
		break;		
	}
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, unit);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", unit);
        return ERROR;
    }
	else
	{
		pDrvCtrl= (FM_QSPI_DRV_CTRL *)pDev->pDrvCtrl;
		printf ("\nvxbInstByNameFind(%s): 0x%X \n\n", FM_QSPI_NAME, pDev);
	}	

	
	byteCnt = pDrvCtrl->FLASH.pageSize;
	
	for (i=0; i<byteCnt; i++)
	{
		pBuf8[i] = g_test_qspi3 + i;
	}
	g_test_qspi3++;
	
	ret = vxQspiFlash_Write_Page(pDev, page_idx, pBuf8);
	if (ret == ERROR)
	{
		printf("----pDev(%d)-write page(%d):fail!----\n", ctrl_x, page_idx);	
		return;
	}	
	
	for (j=0; j<byteCnt; j++)
	{
		printf("%02X", pBuf8[j]);
		
		if ((j+1)%4 == 0)
		{
			printf(" ");
		}
		
		if ((j+1)%16 == 0)
		{
			printf(" \n");
		}
		
		if ((j+1)%256 == 0)
		{
			printf(" \n");
		}
	}

	printf("------------\n");

	return;
}

#endif




#if 1
void testvxb_qspi_erase_sect2(int sect_idx)
{
    VXB_DEVICE_ID      pDev;
    FM_QSPI_DRV_CTRL * pDrvCtrl;
	
	int ret = 0;
	int unit = 0;
	int ctrl_x = 0;
	
	switch (ctrl_x)
	{
	case QSPI_CTRL_0:
		unit = 0;
		break;
	
	case QSPI_CTRL_1:
		unit = 1;
		break;		
	}
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, unit);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", unit);
        return ERROR;
    }
	else
	{
		pDrvCtrl= (FM_QSPI_DRV_CTRL *)pDev->pDrvCtrl;
		/*printf ("\nvxbInstByNameFind(%s): 0x%X \n\n", FM_QSPI_NAME, pDev);*/
	}
	
	ret = vxQspiFlash_Erase_Sect(pDev, sect_idx);
    if (ret == ERROR)
    {
    	printf("ctrl_%d: qspiflash erase sect: %d fail! \n", ctrl_x, sect_idx);
    }
	else
	{
		printf("ctrl_%d: qspiflash erase sect: %d ok! \n", ctrl_x, sect_idx);
	}
	
	return;
}

void testvxb_qspi_rd_sect2(int sect_idx)
{
    VXB_DEVICE_ID       pDev;
    FM_QSPI_DRV_CTRL * pDrvCtrl;

	int ret = 0;
    int i;
	int unit = 0;
	
	UINT8 * pBuf8 = (UINT8 *)(&RcvBuf3[0]);
	int j = 0, byteCnt = 0;
	
	int ctrl_x = QSPI_CTRL_0;
	
	switch (ctrl_x)
	{
	case QSPI_CTRL_0:
		unit = 0;
		break;
	
	case QSPI_CTRL_1:
		unit = 1;
		break;		
	}
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, unit);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", unit);
        return ERROR;
    }
	else
	{
		pDrvCtrl= (FM_QSPI_DRV_CTRL *)pDev->pDrvCtrl;
		/*printf ("\nvxbInstByNameFind(%s): 0x%X \n\n", FM_QSPI_NAME, pDev);*/
	}	
	
	byteCnt = pDrvCtrl->FLASH.sectSize;	
	bzero((UINT8 *)(&RcvBuf3[0]), byteCnt);

	ret = vxQspiFlash_Read_Sect(pDev, sect_idx, pBuf8);
	if (ret == ERROR)
	{
		printf("--qpsi read sect(%d) fail!-- \n", sect_idx);
	}
	else
	{
		printf("qpsi read sect(%d) ok! \n", sect_idx);
	}

	return;
}

void testvxb_qspi_wr_sect2(int sect_idx)
{
    VXB_DEVICE_ID      pDev;
    FM_QSPI_DRV_CTRL * pDrvCtrl;

	int ret = 0;
    int i;
	int unit = 0;
	
	UINT8 * pBuf8 = (UINT8 *)(&SndBuf3[0]);
	int j = 0, byteCnt = 0;
	
	int ctrl_x = QSPI_CTRL_0;
	
	switch (ctrl_x)
	{
	case QSPI_CTRL_0:
		unit = 0;
		break;
	
	case QSPI_CTRL_1:
		unit = 1;
		break;		
	}
	
    pDev = vxbInstByNameFind (FM_QSPI_NAME, unit);
    if (pDev == NULL)
    {
        printf ("\nCan not find this pDev unit(%d)!  \n\n", unit);
        return ERROR;
    }
	else
	{
		pDrvCtrl= (FM_QSPI_DRV_CTRL *)pDev->pDrvCtrl;
		/*printf ("\nvxbInstByNameFind(%s): 0x%X \n\n", FM_QSPI_NAME, pDev);*/
	}	

	
	byteCnt = pDrvCtrl->FLASH.sectSize;
	
	for (i=0; i<byteCnt; i++)
	{
		pBuf8[i] = g_test_qspi3 + i;
	}
	g_test_qspi3++;
	
	ret = vxQspiFlash_Write_Sect(pDev, sect_idx, pBuf8);
	if (ret == ERROR)
	{
		printf("--qpsi write sect(%d) fail!-- \n", sect_idx);
	}
	else
	{
		printf("qpsi write sect(%d) ok! \n", sect_idx);
	}
	
	return;
}

/*#define MODE_ONLY_ERASE_RD*/

void testvxb_qspi_rdwr_sects(int from_sect, int to_sect)
{
	int idx = 0, j = 0;
	int sect_diff = 0;

	for (idx = from_sect; idx <= to_sect; idx++)
	{
		testvxb_qspi_erase_sect2(idx);
		taskDelay(10);

	#ifndef MODE_ONLY_ERASE_RD
		testvxb_qspi_wr_sect2(idx);
		taskDelay(10);
	#endif
		
		testvxb_qspi_rd_sect2(idx);
		taskDelay(10);

		sect_diff = 0;
		for (j=0; j<QSPIFLASH_SECTOR_64K; j++)
		{
		#ifndef MODE_ONLY_ERASE_RD
			if (RcvBuf3[j] != SndBuf3[j])
		#else
			if (RcvBuf3[j] != 0xFF)
		#endif
			{
				sect_diff = 1;
				break;
			}
		}

		if (sect_diff == 1)
		{
			printf("------sect(%d) erase & wr & rd err:[%d][0x%X-0x%X]!------- \n\n", \
				    idx, j, RcvBuf3[j], SndBuf3[j]);
		}
		else
		{
		
		#ifndef MODE_ONLY_ERASE_RD
			printf("====sect(%d) erase & wr & rd OK!==== \n\n", idx);
		#else
			printf("====sect(%d) erase & ... & rd OK!==== \n\n", idx);  /* only erase & read*/
		#endif
		}
	}

	printf("\n\n testvxb sectors:[%d -> %d] finish! \n\n\n", from_sect, to_sect);
	
	return;
}

void test_rd_ctrl_regs(void)
{
	UINT32* pReg = NULL;
	UINT32 base = 0xE0000000;
	int i = 0;

	for (i=0; i<0xb4; i+=4)
	{
		if ((i == 0x48) || (i == 0x4C)
			|| (i == 0x5C) 
			|| (i == 0x84) || (i == 0x88)
			|| (i == 0x98) || (i == 0x9C))
		{
			printf("reg[%02X] = -------- \n", i);
		}
		else
		{
			pReg = (UINT32*)(base + i);
			printf("reg[%02X] = 0x%08X \n", i, *pReg);
		}
	}
	
	pReg = (UINT32*)(base + 0xFC);
	printf("reg[%02X] = 0x%08X \n", i, *pReg);

	return;
}

#endif

