/* vxbAltSocGen5DwMshc_fm.c - Altera SoC DesignWare MSHC driver */

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
01a, 15Aug19, jc  written.
*/

/*
DESCRIPTION

This is the vxbus compliant Altera SoC DesignWare Mobile Storage 
Host Controller driver which implements the functionality specified in its
manual.

The driver provides an interface between the host controller and the 
SD/MMC memory cards. The driver implements all the vxbus driver specific 
initialization routines like altSocGen5DwMshc_InstInit(), 
altSocGen5DwMshc_InstInit2() and altSocGen5DwMshc_InstConnect().

EXTERNAL INTERFACE

The driver provides the standard vxbus external interface 
altSocGen5DwMshc_StorageRegister(). This function registers the driver with 
the vxbus subsystem, and instances will be created as needed. As the MSHC 
device is on platform local bus, each device instance must be specified in 
the hwconf.c file in a BSP. 

The hwconf entry must specify the following parameters:

\is

\i <regBase>
Specifies the base address where the MSHC registers are mapped into the host's
address space. All register offsets are computed relative to this address.

\i <clkFreq>
Specifies the clock source frequency of the module. The clock source
frequency is platform dependent.

\i <rwMode>
Specifies the read and write mode of the module. Both DMA and PIO mode are 
supported now. If this property is not explicitly specified, the driver uses 
DMA mode by default.

\i <polling>
Specifies whether the driver uses polling mode or not. If this property is
not explicitly specified, the driver uses interrupt mode by default.

\i <flags>
Specifies various features of the host controller.
Currently, the following flags are supported:

\is

\i ALT_MSHC_FLAGS_CARD_PRESENT
Specify whether the SD card is present always. This is used for no
hardware card detection scenario, thus the driver assumes the card is always
plugged in and does not support hot plug out.

\i ALT_MSHC_FLAGS_CARD_WRITABLE
Specify whether the SD card is writable always. This is used for no
hardware card write protection scenario.

\i ALT_MSHC_FLAGS_SNOOP_ENABLED
Specify whether the host controller has hardware snoop features. Cache 
operations are not performd when this flag is set.

\ie

\i <clkFreqSetup>
Specifies board related clock frequency setup method. Some boards may need to 
change board specific settings when SD clock frequency is changed. 
The prototype of this method is :

\cs
    void (* clkFreqSetup) (UINT32 clk)
\ce

\ie

An example hwconf entry is shown below:

\cs
struct hcfResource alteraMshcResources[] = {
    { "regBase",        HCF_RES_INT,    { (void *)ALT_SDMMC_BASE } },
    { "clkFreq",        HCF_RES_INT,    { (void *)ALT_SDMMC_CLK_DIVIDED_VAL} },
    { "rwMode",         HCF_RES_INT,    { (void *)ALT_DW_MSHC_RW_MODE_DMA } },
    { "polling",        HCF_RES_INT,    { (void *)FALSE } },
    { "clkFreqSetup",   HCF_RES_ADDR,   { (void *)sysMshcClkFreqSetup } },
    { "flags",          HCF_RES_INT,    { (void *)(ALT_MSHC_FLAGS_CARD_PRESENT |
                                                   ALT_MSHC_FLAGS_CARD_WRITABLE)
                                                  } },
};
\ce

To add this driver to the vxWorks image, add the following component to the
kernel configuration, or define the following macro in config.h.

\cs
vxprj component add DRV_STORAGE_ALT_SOC_GEN5_DW_MSHC
\ce

\cs
#define DRV_STORAGE_ALT_SOC_GEN5_DW_MSHC
\ce

SEE ALSO: vxBus, xbd
\tb "Cyclone V Device Handbook Volume 3: Hard Processor System Technical Reference Manual"
\tb "SD Specifications Part 1 Physical Layer Simplified Specification Version 3.01"
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
#include <hwif/vxbus/vxBus.h>
#include <hwif/vxbus/hwConf.h>
#include <hwif/util/vxbParamSys.h>
#include <../src/hwif/h/vxbus/vxbAccess.h>
#include <../src/hwif/h/storage/vxbSdMmcXbd.h>

/* #include <../src/hwif/h/storage/vxbSdMmcLib.h> */
/* #include <../src/hwif/h/storage/vxbAltSocGen5DwMshc.h> */

#include "vxbSdMmcLib_fm.h"
#include "vxbAltSocGen5DwMshc_fm.h"

#include "vx69_sdmmc.h"

/*
defines 
*/
#undef ALT_DW_MSHC_DBG_ON

#ifdef  ALT_DW_MSHC_DBG_ON

#ifdef  LOCAL
#undef  LOCAL
#define LOCAL
#endif /* LOCAL */

#define ALT_DW_MSHC_DBG_IRQ            0x00000001
#define ALT_DW_MSHC_DBG_RW             0x00000002
#define ALT_DW_MSHC_DBG_XBD            0x00000004
#define ALT_DW_MSHC_DBG_ERR            0x00000008
#define ALT_DW_MSHC_DBG_INIT           0x00000010
#define ALT_DW_MSHC_DBG_CMD            0x00000020
#define ALT_DW_MSHC_DBG_ALL            0xffffffff
#define ALT_DW_MSHC_DBG_OFF            0x00000000

/* LOCAL UINT32 altSocGen5DwMshc_DbgMask = ALT_DW_MSHC_DBG_ERR; */
LOCAL UINT32 altSocGen5DwMshc_DbgMask = ALT_DW_MSHC_DBG_ALL;

IMPORT FUNCPTR _func_logMsg;

#define ALT_DW_MSHC_DBG(mask, string, a, b, c, d, e, f)                    \
    if ((altSocGen5DwMshc_DbgMask & mask) || (mask == ALT_DW_MSHC_DBG_ALL)) \
        if (_func_logMsg != NULL)                                          \
           printf(string, a, b, c, d, e, f) /*(* _func_logMsg)(string, a, b, c, d, e, f)*/
		
#else /* !ALT_DW_MSHC_DBG_ON */
#define ALT_DW_MSHC_DBG(mask, string, a, b, c, d, e, f)
#endif  /* ALT_DW_MSHC_DBG_ON */

/*
 * File system use cacheDmaMalloc to alloc buffer 
 * (file bio.c, bio_alloc() routine), so there is no need for driver to 
 * use cacheFlush and cacheInvalidate to keep buffer's coherence. 
 * When define ALT_DW_MSHC_FS_UNCACHE_BUFFER in this driver, we will ignore the
 * the sync operaton of DMA buffer.
 */
#define ALT_DW_MSHC_FS_UNCACHE_BUFFER

/* register low level access routines */
#define ALT_DW_MSHC_BAR(p)     ((ALT_MSHC_DRV_CTRL *)(p)->pDrvCtrl)->regBase
#define ALT_DW_MSHC_HANDLE(p)  ((ALT_MSHC_DRV_CTRL *)(p)->pDrvCtrl)->regHandle

#ifdef ARMBE8
#    define SWAP32 vxbSwap32
#else
#    define SWAP32 
#endif /* ARMBE8 */

#undef CSR_READ_4
#define CSR_READ_4(pDev, addr)                  \
        SWAP32(vxbRead32(ALT_DW_MSHC_HANDLE(pDev),      \
                  (UINT32 *)((char *)ALT_DW_MSHC_BAR(pDev) + addr)))

#undef CSR_WRITE_4
#define CSR_WRITE_4(pDev, addr, data)           \
        vxbWrite32(ALT_DW_MSHC_HANDLE(pDev),    \
                   (UINT32 *)((char *)ALT_DW_MSHC_BAR(pDev) + addr), SWAP32(data))

#undef CSR_SETBIT_4
#define CSR_SETBIT_4(pDev, offset, val)         \
        CSR_WRITE_4(pDev, offset, CSR_READ_4(pDev, offset) | (val))

#undef CSR_CLRBIT_4
#define CSR_CLRBIT_4(pDev, offset, val)         \
        CSR_WRITE_4(pDev, offset, CSR_READ_4(pDev, offset) & ~(val))

/* forward declarations */
#if 1
LOCAL void altSocGen5DwMshc_InstInit (VXB_DEVICE_ID pInst);
LOCAL void altSocGen5DwMshc_InstInit2 (VXB_DEVICE_ID pInst);
LOCAL void altSocGen5DwMshc_InstConnect (VXB_DEVICE_ID pInst);
LOCAL void altSocGen5DwMshc_DevInit (VXB_DEVICE_ID pInst);
LOCAL void altSocGen5DwMshc_Isr (VXB_DEVICE_ID pDev);
LOCAL void altSocGen5DwMshc_CardMonTask (VXB_DEVICE_ID);
LOCAL STATUS altSocGen5DwMshc_CmdIssue (SDMMC_HOST * host);
LOCAL void altSocGen5DwMshc_BusWidthSetup (SDMMC_HOST * host, UINT32 width);
LOCAL BOOL altSocGen5DwMshc_CardWpCheck (SDMMC_HOST * host);
LOCAL BOOL altSocGen5DwMshc_CardDetect (VXB_DEVICE_ID pDev);
LOCAL STATUS altSocGen5DwMshc_BlkRead (SDMMC_CARD * card, sector_t blkNo, UINT32 numBlks, void * pBuf);
LOCAL STATUS altSocGen5DwMshc_BlkWrite (SDMMC_CARD * card, sector_t blkNo, UINT32 numBlks, void * pBuf);
LOCAL STATUS altSocGen5DwMshc_PioRead (VXB_DEVICE_ID pDev);
LOCAL STATUS altSocGen5DwMshc_PioWrite (VXB_DEVICE_ID pDev);
LOCAL void altSocGen5DwMshc_ClkFreqSetup (SDMMC_HOST * host, UINT32 clk);
LOCAL void altSocGen5DwMshc_VddSetup (SDMMC_HOST * host, UINT32 vdd);
LOCAL void altSocGen5DwMshc_WriteFifo (VXB_DEVICE_ID pDev);
LOCAL void altSocGen5DwMshc_ReadFifo (VXB_DEVICE_ID pDev);
LOCAL STATUS altSocGen5DwMshc_PollIntsts (VXB_DEVICE_ID pDev, UINT32 mask);

extern void sdmmc_HostShow(SDMMC_HOST * host);
extern void sdmmc_CardShow(    SDMMC_CARD * card);
#endif

/* locals */

LOCAL DRIVER_INITIALIZATION altSocGen5DwMshc_Funcs =
    {
    altSocGen5DwMshc_InstInit,        /* devInstanceInit */
    altSocGen5DwMshc_InstInit2,       /* devInstanceInit2 */
    altSocGen5DwMshc_InstConnect      /* devConnect */
    };

LOCAL DRIVER_REGISTRATION altSocGen5DwMshc_PlbRegistration =
{
    NULL,                            /* pNext */
    VXB_DEVID_DEVICE,                /* devID */
    VXB_BUSID_PLB,                   /* busID = PLB */
    VXB_VER_4_0_0,                   /* vxbVersion */
    ALT_DW_MSHC_NAME,                /* drvName */
    &altSocGen5DwMshc_Funcs,          /* pDrvBusFuncs */
    NULL,                            /* pMethods */
    NULL,                            /* devProbe */
    NULL                             /* pParamDefaults */
};

/* externs */

IMPORT UCHAR erfLibInitialized;

/*******************************************************************************
*
* altSocGen5DwMshc_Register - register this driver
*
* This routine registers this driver with the vxbus subsystem.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void altSocGen5DwMshc_Register (void)
{
	/* extern int g_sdmmc_ctrl;  
	 g_sdmmc_ctrl = 1;	// sdmmc ctrl_1
     vxbDevRegister (&altSocGen5DwMshc_PlbRegistration); */
   
}

void altSocGen5DwMshc_Register2 (void)
{
	/*extern int g_sdmmc_ctrl;*/
	/*g_sdmmc_ctrl = 1;  // sdmmc ctrl_1*/
	
    vxbDevRegister (&altSocGen5DwMshc_PlbRegistration);
}


/*******************************************************************************
*
* altSocGen5DwMshc_InstInit - first level initialization routine of the device
*
* This routine performs the first level initialization of the device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshc_InstInit
    (
    VXB_DEVICE_ID pInst
    )
    {
    }

/*******************************************************************************
*
* altSocGen5DwMshc_InstInit2 - second level initialization routine of the device
*
* This routine performs the second level initialization of the the device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshc_InstInit2
    (
    VXB_DEVICE_ID       pInst
    )
{
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    struct hcfDevice *  pHcf;
	
    UINT16              i;
    STATUS              status;

    if (pInst == NULL)
        return;

    pDrvCtrl = (ALT_MSHC_DRV_CTRL *)malloc (sizeof(ALT_MSHC_DRV_CTRL));
    if (pDrvCtrl == NULL)
        return;

    bzero ((char *)pDrvCtrl, sizeof(ALT_MSHC_DRV_CTRL));

    pDrvCtrl->pDev = pInst;
    pInst->pDrvCtrl = pDrvCtrl;
	
#if 0
    for (i = 0; i < VXB_MAXBARS; i++)
        {
        if (pInst->regBaseFlags[i] == VXB_REG_MEM)
            break;
        }

    if (i == VXB_MAXBARS)
        {
        free (pDrvCtrl);
        return;
        }
    pDrvCtrl->regBase = pInst->pRegBase[i];
#else
    pDrvCtrl->regBase = pInst->pRegBase[0];

	printf("pInst->unitNumber:(%d); pDrvCtrl->regBase:0x%08X \n", pInst->unitNumber, pDrvCtrl->regBase);

	extern int g_sdmmc_ctrl;
	if (pInst->unitNumber == 1)
	{	
		g_sdmmc_ctrl = 1;  /* sdmmc ctrl_1*/
	}
	else
	{
		g_sdmmc_ctrl = 0;  /* sdmmc ctrl_0*/
	}
#endif
	
    vxbRegMap (pInst, i, &pDrvCtrl->regHandle);

    pHcf = (struct hcfDevice *)hcfDeviceGet (pInst);
    if (pHcf != NULL)
    {   
        /*
         * resourceDesc {
         * The clkFreq resource specifies clock
         * frequency for base clock. }
         */
        status = devResourceGet (pHcf, "clkFreq", HCF_RES_INT,
                                 (void *)&pDrvCtrl->clkFreq);
        if (status == ERROR)
            goto errOut;
        
        /*
         * resourceDesc {
         * The rwMode resource specifies the read and write
         * mode. If this property is not
         * explicitly specified, the driver uses
         * DMA by default. }
         */
        status = devResourceGet (pHcf, "rwMode", HCF_RES_INT,
                                 (void *)&pDrvCtrl->rwMode);
        if (status == ERROR)
            pDrvCtrl->rwMode = ALT_DW_MSHC_RW_MODE_DEFAULT;

        /*
         * resourceDesc {
         * The polling resource specifies whether
         * the driver uses polling mode or not.
         * If this property is not explicitly
         * specified, the driver uses interrupt mode
         * by default. }
         */
        status = devResourceGet (pHcf, "polling", HCF_RES_INT,
                                 (void *)&pDrvCtrl->polling);
        if (status == ERROR)
            pDrvCtrl->polling = FALSE;

        /*
         * resourceDesc {
         * The flags resource specifies various 
         * controll flags of the host controller. }
         */
        pDrvCtrl->flags = 0;
        (void) devResourceGet (pHcf, "flags", HCF_RES_INT,
                               (void *)&pDrvCtrl->flags);

        /*
         * resourceDesc {
         * The boardTuning resource specifies the method of 
         * board specific clock tuning 
         */
        (void) devResourceGet (pHcf, "clkFreqSetup", HCF_RES_ADDR,
                               (void *)&pDrvCtrl->clkFreqSetup);
    }

    /*
     * The devChange semaphore is used by the interrupt service routine
     * to inform the card monitor task that a state change has occurred.
     * If the board has special mechanism to notify state change, BSP should 
     * implement it, such as GPIO interrupt.
     */

    pDrvCtrl->devChange = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
    if (pDrvCtrl->devChange == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "semBCreate failed for devChange\n",
                         1, 2, 3, 4, 5, 6);
        goto errOut;
        }

    pDrvCtrl->cmdDone = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
    if (pDrvCtrl->cmdDone == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "semBCreate failed for cmdDone\n",
                         1, 2, 3, 4, 5, 6);
        goto errOut;
        }

    pDrvCtrl->dataDone = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
    if (pDrvCtrl->dataDone == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "semBCreate failed for dataDone\n",
                         1, 2, 3, 4, 5, 6);
        goto errOut;
        }

#ifdef ALT_DW_MSHC_FS_UNCACHE_BUFFER
    pDrvCtrl->flags |= ALT_MSHC_FLAGS_SNOOP_ENABLED;
#endif /* ALT_DW_MSHC_FS_UNCACHE_BUFFER */

    /* success */

    return;

errOut:
    if ((pDrvCtrl->dataDone != NULL) && (semDelete(pDrvCtrl->dataDone) != OK))
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, "semDelete failed for dateDone\n",
                         1, 2, 3, 4, 5, 6);

    if ((pDrvCtrl->cmdDone != NULL) && (semDelete(pDrvCtrl->cmdDone) != OK))
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, "semDelete failed for cmdDone\n",
                         1, 2, 3, 4, 5, 6);
    if ((pDrvCtrl->devChange != NULL) && (semDelete(pDrvCtrl->devChange) != OK))
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "semDelete failed for devChange\n",
                         1, 2, 3, 4, 5, 6);
    free (pDrvCtrl);

    return;
    }

/*******************************************************************************
*
* altSocGen5DwMshc_InstConnect - third level initialization routine of the device
*
* This routine performs the third level initialization of the the device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshc_InstConnect
    (
    VXB_DEVICE_ID pInst
    )
{
    ALT_MSHC_DRV_CTRL * pDrvCtrl;

    if (pInst == NULL)
        return;

    pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pInst->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return;
	
    /* 
    per-device init 
	*/
    altSocGen5DwMshc_DevInit (pInst);
	

    /* 
    create a monitor task that handles card state change 
	*/
    taskSpawn (ALT_DW_MSHC_CARD_MON_TASK_NAME, ALT_DW_MSHC_CARD_MON_TASK_PRI, 0,
               ALT_DW_MSHC_CARD_MON_TASK_STACK, 
               (FUNCPTR)altSocGen5DwMshc_CardMonTask, 
               (_Vx_usr_arg_t)pInst,
               0, 0, 0, 0, 0, 0, 0, 0, 0);

	/**/
	/* dma mod*/
	/**/
    if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_DMA)
    {
        pDrvCtrl->idmaDesc = (ALT_MSHC_IDMA_DESC *)cacheDmaMalloc 
                             (sizeof (ALT_MSHC_IDMA_DESC) * ALT_MSHC_IDMA_DESC_NUM);    
        if (pDrvCtrl->idmaDesc == NULL)
        {
            /* use pio instead */
            pDrvCtrl->rwMode = ALT_DW_MSHC_RW_MODE_PIO;
			
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_INIT, 
                             "Failed to alloc dma descriptors, use PIO\n", 
                             1, 2, 3, 4, 5, 6);
        }
		
        bzero ((char *)pDrvCtrl->idmaDesc, (sizeof(ALT_MSHC_IDMA_DESC) * ALT_MSHC_IDMA_DESC_NUM));
    }

    /*
    setup the interrupt mask 
	*/ 
    if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_DMA)
    {
        pDrvCtrl->intMask = ALT_MSHC_INT_DMA_DEFAULTS;
    }
    else
    {    
    	pDrvCtrl->intMask = ALT_MSHC_INT_PIO_DEFAULTS;
    }
	
	/**/
	/* clear int_status mask bit*/
	/**/
    CSR_WRITE_4 (pInst, ALT_MSHC_INTMASK, pDrvCtrl->intMask);

	/**/
	/* irq mode*/
	/**/
    if (!pDrvCtrl->polling)
    {
        /*
        connect and enable interrupt 
		*/
        vxbIntConnect (pInst, 0, altSocGen5DwMshc_Isr, pInst);
        vxbIntEnable (pInst, 0, altSocGen5DwMshc_Isr, pInst);
		
		ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_INIT, 
						 "vxbIntConnect(): altSocGen5DwMshc_Isr \n", 1, 2, 3, 4, 5, 6);

        /* enable interrupts */
        CSR_SETBIT_4 (pInst, ALT_MSHC_CTRL, ALT_MSHC_CTRL_INT_ENABLE);

        /* 
        card always present, notify monitor task 
		*/
        if ((pDrvCtrl->flags & ALT_MSHC_FLAGS_CARD_PRESENT) != 0)
        {    
        	semGive (pDrvCtrl->devChange);
        }
    }

    return;
}

/*******************************************************************************
*
* altSocGen5DwMshc_DevInit - per device specific initialization of ALT MSHC
*
* This routine performs per device specific initialization of ALT MSHC.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshc_DevInit
    (
    VXB_DEVICE_ID pInst
    )
{
    ALT_MSHC_DRV_CTRL *  pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pInst->pDrvCtrl;
    ALT_MSHC_IDMA_DESC * pIdmaDesc = pDrvCtrl->idmaDesc;
    void *               physAddr;
	
	UINT32 tmp32 = 0;

    /* 
    reset controller: ctrl, fifo, dma
	*/
#if 1
    CSR_WRITE_4 (pInst, ALT_MSHC_CTRL, (ALT_MSHC_CTRL_RESET | 
                 ALT_MSHC_CTRL_FIFO_RESET | ALT_MSHC_CTRL_DMA_RESET));

    while ((CSR_READ_4 (pInst, ALT_MSHC_CTRL) & 
            (ALT_MSHC_CTRL_RESET | ALT_MSHC_CTRL_FIFO_RESET | 
            ALT_MSHC_CTRL_DMA_RESET)) != 0);
#else

	tmp32 = CSR_READ_4 (pInst, ALT_MSHC_CTRL);
    CSR_WRITE_4 (pInst, ALT_MSHC_CTRL, (tmp32 | ALT_MSHC_CTRL_RESET));
    while ((CSR_READ_4 (pInst, ALT_MSHC_CTRL) & ALT_MSHC_CTRL_RESET) != 0);
	
	tmp32 = CSR_READ_4 (pInst, ALT_MSHC_CTRL);
    CSR_WRITE_4 (pInst, ALT_MSHC_CTRL, (tmp32 | ALT_MSHC_CTRL_FIFO_RESET));
    while ((CSR_READ_4 (pInst, ALT_MSHC_CTRL) & ALT_MSHC_CTRL_FIFO_RESET) != 0);
#endif

	/**/
	/* dma mode*/
	/**/
    if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_DMA)
    {
        /* 
        dual buffer mode 
		*/
        pIdmaDesc->des0 = SWAP32(ALT_MSHC_IDMA_DES0_FD);
        pIdmaDesc[ALT_MSHC_IDMA_DESC_NUM - 1].des0 =  SWAP32(ALT_MSHC_IDMA_DES0_ER);
		
        physAddr = CACHE_DMA_VIRT_TO_PHYS (pIdmaDesc);
        pIdmaDesc[ALT_MSHC_IDMA_DESC_NUM - 1].des3 =  SWAP32(VXB_ADDR_LOW32 (physAddr)); 

        CSR_WRITE_4 (pInst, ALT_MSHC_BMOD, 4 * ALT_MSHC_BMOD_DSL);
        CSR_WRITE_4 (pInst, ALT_MSHC_BMOD, ALT_MSHC_BMOD_SWR);

        CSR_WRITE_4 (pInst, ALT_MSHC_IDINTEN_90H, (ALT_MSHC_IDMA_INT_RI | 
                     ALT_MSHC_IDMA_INT_TI | ALT_MSHC_IDMA_INT_NI));

        CSR_WRITE_4 (pInst, ALT_MSHC_DBADDR_88H, VXB_ADDR_LOW32 (physAddr));
    }

    /* 
    clear interrupts 
	*/
    CSR_WRITE_4(pInst, ALT_MSHC_RINTSTS, 0xFFFFFFFF); /* Writes 1 clears status bit */

	/**/
	/* enable irq*/
	/**/
	tmp32 = CSR_READ_4 (pInst, ALT_MSHC_CTRL) | 0x00000010;   /* INT_ENABLE */
    CSR_WRITE_4 (pInst, ALT_MSHC_CTRL, tmp32);
	
    /*CSR_WRITE_4(pInst, ALT_MSHC_INTMASK, 0); /* Value:0 masks interrupt; value:1 enables interrupt.*/
    CSR_WRITE_4(pInst, ALT_MSHC_INTMASK, 0x01); /* SDMMC_INTMASK: enable CD */

    CSR_WRITE_4(pInst, ALT_MSHC_TMOUT, ALT_MSHC_DEFAULT_TIMEOUT);

	/**/
	/* MSize = 8，RX_WMark = 7 TX_WMark = 8*/
	/**/
    CSR_WRITE_4(pInst,  ALT_MSHC_FIFOTH, ALT_MSHC_DEFAULT_FIFO_THRESH);

    /* 
    disable clock 
	*/
    /*CSR_WRITE_4(pInst,  ALT_MSHC_CLKENA, 0);*/
    /*CSR_WRITE_4(pInst,  ALT_MSHC_CLKSRC, 0);*/
	
	/**/
    /* clear int*/
	/*	*/
	tmp32 = CSR_READ_4 (pInst, ALT_MSHC_RINTSTS) ;
    CSR_WRITE_4(pInst, ALT_MSHC_RINTSTS, tmp32);

    /* fill in SDMMC_HOST structure */
    pDrvCtrl->host.pDev = pInst;

    pDrvCtrl->host.ops.cmdIssue        = (FUNCPTR)altSocGen5DwMshc_CmdIssue;
	
    pDrvCtrl->host.ops.vddSetup        = (FUNCPTR)altSocGen5DwMshc_VddSetup;
	
    /*pDrvCtrl->host.ops.clkFreqSetup    = (FUNCPTR)altSocGen5DwMshc_ClkFreqSetup;*/
   /* extern void sysMshcClkFreqSetup(UINT32 clk);*/
    /*pDrvCtrl->host.ops.clkFreqSetup    = (FUNCPTR)sysMshcClkFreqSetup;*/
	
    pDrvCtrl->host.ops.busWidthSetup   = (FUNCPTR)altSocGen5DwMshc_BusWidthSetup;
    pDrvCtrl->host.ops.cardWpCheck     = (FUNCPTR)altSocGen5DwMshc_CardWpCheck;

    /* voltage supported */
    /*pDrvCtrl->host.capbility |= (OCR_VDD_VOL_32_33 | OCR_VDD_VOL_33_34);*/
    /*pDrvCtrl->host.capbility |= 0x00FF0000;*/
	pDrvCtrl->host.capbility = MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195;;

    /* high capacity SD card is supported */
    pDrvCtrl->host.capbility |= OCR_CARD_CAP_STS;

    /* high speed SD card supported */
    pDrvCtrl->host.highSpeed = TRUE;

    pDrvCtrl->card.host = &pDrvCtrl->host;

    /* fill in SDMMC_XBD_DEV structure */
    pDrvCtrl->card.xbdDev.blkRd = (FUNCPTR)altSocGen5DwMshc_BlkRead;
    pDrvCtrl->card.xbdDev.blkWt = (FUNCPTR)altSocGen5DwMshc_BlkWrite;

    pDrvCtrl->card.xbdDev.card  = &pDrvCtrl->card;
	
    pDrvCtrl->card.idx          = pInst->unitNumber;

	return;
}

/*******************************************************************************
*
* altSocGen5DwMshc_Isr - interrupt service routine
*
* This routine handles interrupts of the ALT MSHC controller.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshc_Isr
    (
    VXB_DEVICE_ID pDev
    )
{
    ALT_MSHC_DRV_CTRL * pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    SDMMC_HOST *        host = &pDrvCtrl->host;
    UINT32              mintSts;
    UINT32              idSts = 0;
    BOOL                needRead  = FALSE;
    BOOL                needWrite = FALSE;

    mintSts = CSR_READ_4 (pDev, ALT_MSHC_MINTSTS);
    ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_IRQ, 
                     "MINTSTS=0x%x\n", mintSts, 2, 3, 4, 5, 6);

    if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_DMA)
	{
        idSts = CSR_READ_4 (pDev, ALT_MSHC_IDSTS_8CH);
    }
	
    if ((mintSts == 0) && (idSts == 0))
        return;

    if ((mintSts & ALT_MSHC_INT_CMD_ERROR) != 0) 
    {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_IRQ, "CMD%d command error\n",
                         host->cmdIssued.cmdIdx, 2, 3, 4, 5, 6);
        host->cmdIssued.cmdErr |= SDMMC_CMD_ERR_PROTOCOL;
    }

    if ((mintSts & ALT_MSHC_INT_DATA_ERROR) != 0) 
    {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_IRQ, "CMD%d data error\n",
                         host->cmdIssued.cmdIdx, 2, 3, 4, 5, 6);
        host->cmdIssued.cmdErr |= SDMMC_DATA_ERR_PROTOCOL;
    }

    if ((mintSts & ALT_MSHC_INT_CD) != 0) 
    {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_IRQ, "Card hot plug\n",
                         1, 2, 3, 4, 5, 6);
        semGive (pDrvCtrl->devChange);
    }

    if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_PIO)
    {
        if ((mintSts & ALT_MSHC_INT_RXDR) != 0)
        {
            needRead = TRUE;
        }

        if ((mintSts & ALT_MSHC_INT_TXDR) != 0)
        {
            needWrite = TRUE;
        }
		
        if ((mintSts & ALT_MSHC_INT_HTO) != 0)
        {
            if (host->cmdIssued.cmdData.isRead)
                needRead = TRUE;
            else
                needWrite = TRUE;
        }
    }

    if ((mintSts & ALT_MSHC_INT_DTO) != 0)
    {
        /* read remaining data for DTO */
        if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_PIO)
        {
            if (host->cmdIssued.cmdData.isRead)
            {
                needRead = TRUE;
            }
            
            /* data done for PIO read and write */            
            semGive (pDrvCtrl->dataDone);
        }
    }

    if (((mintSts & ALT_MSHC_INT_CMD_DONE) != 0) || (host->cmdIssued.cmdErr != 0))
    {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_IRQ, "CMD%d command complete \n",
                         host->cmdIssued.cmdIdx, 0, 0, 0, 0, 0);
        semGive (pDrvCtrl->cmdDone);
    }

    /* level sensitive, handle fifo before clear interrupts */
    if (needRead)
    {
        altSocGen5DwMshc_ReadFifo (pDev);
    }
    if (needWrite)
    {
        altSocGen5DwMshc_WriteFifo (pDev);
    }

	/**/
	/* clear the int_status bit*/
	/**/
    CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, mintSts);
	

    /* DMA interrupts */
    if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_DMA)
    {
        if ((idSts & (ALT_MSHC_IDMA_INT_TI | ALT_MSHC_IDMA_INT_RI | 
                     ALT_MSHC_IDMA_INT_NI)) != 0)
        {
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_IRQ, 
                             "CMD%d DMA interrupt status: 0x%x\n",
                             host->cmdIssued.cmdIdx, idSts, 3, 4, 5, 6);

            /* data done for dma read and write */
            semGive (pDrvCtrl->dataDone);
        }

        CSR_WRITE_4 (pDev, ALT_MSHC_IDSTS_8CH, idSts);
    }
}

/*******************************************************************************
*
* altSocGen5DwMshc_CardMonTask - card status monitor task
*
* This routine is the task loop to handle card insertion and removal.
*
* RETURN: N/A
*
* ERRNO: N/A
*/

#define __sdMmcIdentify_here__

LOCAL void altSocGen5DwMshc_CardMonTask
    (
    VXB_DEVICE_ID pDev
    )
    {
    ALT_MSHC_DRV_CTRL * pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    device_t            dev;
    STATUS              rc;
    BOOL                inserted = FALSE;
	

/*
	while (erfLibInitialized == FALSE)
    {    
    	taskDelay (sysClkRateGet ());
    }
*/

	taskDelay (1);

    FOREVER
    {
        if (!pDrvCtrl->polling)
        {
            /* no need to check return value as the semaphore ID is correct */
            (void) semTake (pDrvCtrl->devChange, WAIT_FOREVER);
        }

        inserted = altSocGen5DwMshc_CardDetect (pDev);
 
        if (inserted && (pDrvCtrl->cardIns == FALSE))
        {
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_XBD, "Card inserted\n",
                             1, 2, 3, 4, 5, 6);
		#if 0
            rc = sdmmc_Identify (&pDrvCtrl->card);  /* only sd*/
		#else
            /*rc = sdmmc_Identify_2 (&pDrvCtrl->card);  // sd / mmc*/
            rc = sdmmc_Identify_3 (&pDrvCtrl->card);  /* sd / mmc*/
		#endif
            if (rc == ERROR)
            {
                ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_XBD, 
                                 "SD/MMC identification process failed\n",
                                 1, 2, 3, 4, 5, 6);
                continue;
            }

            /* already attached, do not create new xbd */
            if (pDrvCtrl->card.attached == TRUE)
            {
                ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_XBD, 
                                 "This should never happen unless hardware "
                                 "state is unstable\n",
                                 1, 2, 3, 4, 5, 6);
                continue;
            }
            pDrvCtrl->card.attached = TRUE;

            /*
            dev = sdMmcXbdDevCreate (&pDrvCtrl->card, (char *)NULL);
            if (dev == NULLDEV)
            {
                ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                                 "sdMmcXbdXbdCreate() failed\n",
                                 1, 2, 3, 4, 5, 6);
                pDrvCtrl->card.attached = FALSE;
                continue;
            }
			*/

            pDrvCtrl->cardIns = TRUE;
        }

        if (!inserted && (pDrvCtrl->cardIns == TRUE))
        {
            pDrvCtrl->card.attached = FALSE;
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_XBD, "Card removed\n",
                             1, 2, 3, 4, 5, 6);

           /* sdMmcXbdDevDelete (&pDrvCtrl->card);*/
            pDrvCtrl->cardIns = FALSE;
        }

        if (pDrvCtrl->polling)
        {    
        	taskDelay (ALT_DW_MSHC_CARDMON_DELAY_SECS * sysClkRateGet ());
        }
    }  /* while (1)*/
}



/*******************************************************************************
*
* altSocGen5DwMshc_ResetFifo - reset the internal fifo buffer
*
* This routine resets the internal fifo buffer.
*
* RETURNS: OK, or ERROR if fifo is not reset until time out.
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwMshc_ResetFifo(VXB_DEVICE_ID pDev)
{	
	int i = 0;
	
    CSR_SETBIT_4 (pDev, ALT_MSHC_CTRL, ALT_MSHC_CTRL_FIFO_RESET);
    
    for (i = 0; i < ALT_MSHC_MAX_RETRIES; i++)
    {
        if ((CSR_READ_4 (pDev, ALT_MSHC_CTRL) & ALT_MSHC_CTRL_FIFO_RESET) == 0) 
        {
            return OK;
        }
		
        vxbUsDelay(10);
    }

    ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, "FIFO reset time out\n",
                     1, 2, 3, 4, 5, 6);
    return ERROR;
}



/*******************************************************************************
*
* altSocGen5DwMshc_IdmaStart - start the internal DMA controller of MSHC
*
* This routine prepares DMA descriptors and starts the internal DMA transfer.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshc_IdmaStart
    (
    VXB_DEVICE_ID pDev
    )
    {    
    ALT_MSHC_DRV_CTRL * pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    SDMMC_HOST *        host = &pDrvCtrl->host;
    UINT32              bufAddr = (UINT32)host->cmdIssued.cmdData.buffer;
    ALT_MSHC_IDMA_DESC * pIdmaDesc = pDrvCtrl->idmaDesc;
    UINT32              round;
    UINT32              remain;
    void *              physAddr;
    UINT32              i;
    UINT32              dataLen;

    pIdmaDesc->des0 = SWAP32(ALT_MSHC_IDMA_DES0_FD);

    /* dual buffer mode */
    dataLen = host->cmdIssued.cmdData.blkNum * host->cmdIssued.cmdData.blkSize;
    round   = dataLen / (ALT_MSHC_IDMA_BUFFER_SIZE * 2);
    remain  = dataLen % (ALT_MSHC_IDMA_BUFFER_SIZE * 2);

    for (i = 0; i < round; i ++)
        {
        pIdmaDesc->des0 |= SWAP32(ALT_MSHC_IDMA_DES0_OWN | 
		                  ALT_MSHC_IDMA_DES0_DIC);
        pIdmaDesc->des1 = SWAP32(ALT_MSHC_IDMA_DES1 (ALT_MSHC_IDMA_BUFFER_SIZE,
                                 ALT_MSHC_IDMA_BUFFER_SIZE));

        physAddr = CACHE_DMA_VIRT_TO_PHYS (bufAddr);
        bufAddr += ALT_MSHC_IDMA_BUFFER_SIZE;
        pIdmaDesc->des2 = SWAP32(VXB_ADDR_LOW32 (physAddr));

        physAddr = CACHE_DMA_VIRT_TO_PHYS (bufAddr);
        bufAddr += ALT_MSHC_IDMA_BUFFER_SIZE;
        pIdmaDesc->des3 = SWAP32(VXB_ADDR_LOW32 (physAddr));

        pIdmaDesc ++;
        }

    if (remain != 0)
        {
        pIdmaDesc->des0 |= SWAP32(ALT_MSHC_IDMA_DES0_OWN | ALT_MSHC_IDMA_DES0_LD);
        pIdmaDesc->des0 &= SWAP32(~ALT_MSHC_IDMA_DES0_DIC);

        physAddr = CACHE_DMA_VIRT_TO_PHYS (bufAddr);
        pIdmaDesc->des2 = SWAP32(VXB_ADDR_LOW32 (physAddr));
        if (remain > ALT_MSHC_IDMA_BUFFER_SIZE)
            {
            bufAddr += ALT_MSHC_IDMA_BUFFER_SIZE;
            pIdmaDesc->des1 = SWAP32(ALT_MSHC_IDMA_DES1 (ALT_MSHC_IDMA_BUFFER_SIZE,
                                                  (remain - 
                                                   ALT_MSHC_IDMA_BUFFER_SIZE)));
            physAddr = CACHE_DMA_VIRT_TO_PHYS (bufAddr);
            pIdmaDesc->des3 = SWAP32(VXB_ADDR_LOW32 (physAddr));
            }
        else
            {
            pIdmaDesc->des1 = SWAP32(ALT_MSHC_IDMA_DES1 (remain, 0));
            pIdmaDesc->des3 = 0;
            }
        }
    else
        {
        pIdmaDesc --;
        pIdmaDesc->des0 |= SWAP32(ALT_MSHC_IDMA_DES0_OWN | 
                                  ALT_MSHC_IDMA_DES0_LD);
        pIdmaDesc->des0 &= SWAP32(~ALT_MSHC_IDMA_DES0_DIC);
        }

    CSR_SETBIT_4 (pDev, ALT_MSHC_CTRL, ALT_MSHC_CTRL_DMA_ENABLE |
                  ALT_MSHC_CTRL_USE_IDMAC);
    CSR_SETBIT_4 (pDev, ALT_MSHC_BMOD, ALT_MSHC_BMOD_FB | ALT_MSHC_BMOD_DE);
	
    CSR_WRITE_4 (pDev, ALT_MSHC_PLDMND_84H, 1);
    
    }

/*******************************************************************************
*
* altSocGen5DwMshc_IdmaStop - stop the internal DMA controller of MSHC
*
* This routine resets DMA descriptors and stop the internal DMA transfer.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshc_IdmaStop
    (
    VXB_DEVICE_ID pDev
    )
    {
    ALT_MSHC_DRV_CTRL *  pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    ALT_MSHC_IDMA_DESC * idmaDesc;
    UINT32               tmp;

    idmaDesc = pDrvCtrl->idmaDesc;

    tmp = CSR_READ_4 (pDev, ALT_MSHC_RINTSTS);
    bzero ((char *)pDrvCtrl->idmaDesc, 
           sizeof (ALT_MSHC_IDMA_DESC) * ALT_MSHC_IDMA_DESC_NUM);

    tmp = CSR_READ_4 (pDev, ALT_MSHC_CTRL);
    tmp &= ~(ALT_MSHC_CTRL_DMA_ENABLE | ALT_MSHC_CTRL_USE_IDMAC);
    tmp |= ALT_MSHC_CTRL_DMA_RESET;
    CSR_WRITE_4 (pDev, ALT_MSHC_CTRL, tmp);
    
    while ((CSR_READ_4 (pDev, ALT_MSHC_CTRL) & (ALT_MSHC_CTRL_DMA_RESET)) != 0)
        ;

    CSR_CLRBIT_4 (pDev, ALT_MSHC_BMOD, ALT_MSHC_BMOD_FB | ALT_MSHC_BMOD_DE);
}

/*******************************************************************************
*
* altSocGen5DwMshc_CardDetect - check if card is present
*
* This routine checks if card is plugged in.
*
* RETURNS: FALSE, or TRUE if card is plugged in.
*
* ERRNO: N/A
*/

LOCAL BOOL altSocGen5DwMshc_CardDetect
    (
    VXB_DEVICE_ID pDev
    )
{
	UINT32 tmp32 = 0; /* jc */
	
    ALT_MSHC_DRV_CTRL * pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    BOOL                present = FALSE;

   /*
   	if ((pDrvCtrl->flags & ALT_MSHC_FLAGS_CARD_PRESENT) != 0)
    {    
    	present = TRUE;
    }
    else
	*/
    {
    	tmp32 = CSR_READ_4 (pDev, ALT_MSHC_CDETECT);
		/*printf("CardDetect: 0x%08X \n", tmp32);*/
    	present = ((tmp32 & 1) == 0);
    }
	
	if (present == TRUE)
	{
		/*pDrvCtrl->flags &= ALT_MSHC_FLAGS_CARD_PRESENT;*/
		pDrvCtrl->flags |= ALT_MSHC_FLAGS_CARD_PRESENT;
	}

    return present;
}


/*******************************************************************************
*
* altSocGen5DwMshc_CmdPrepare - prepare the command to be sent
*
* This routine prepares the command to be sent.
*
* RETURNS: OK, or ERROR if wrong parameters or command is not accepted.
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwMshc_CmdPrepare
    (
    SDMMC_HOST * host
    )
{
    VXB_DEVICE_ID       pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    SDMMC_CMD *         cmd;
    STATUS              status;
    UINT32              i;
    UINT32              dataLen;
    UINT32              count = 0;
    UINT32              cmdSend = 0;
    UINT32 *            buf32;

    if (host == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshc_CmdPrepare: host is NULL\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    pDev = host->pDev;
    if (pDev == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshc_CmdPrepare: pDev is NULL\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshc_CmdPrepare: pDrvCtrl is NULL\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    cmd = &host->cmdIssued;
    if (cmd == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshc_CmdPrepare: cmd is NULL\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    cmdSend = ALT_MSHC_CMD_INDX (cmd->cmdIdx);

    if (cmdSend == SDMMC_CMD12_STOP_TRANSMISSION)
    {    
    	cmdSend |= ALT_MSHC_CMD_STOP;
    }
    else
    {    
    	cmdSend |= ALT_MSHC_CMD_PRV_DAT_WAIT;
    }

    if (cmd->rspType != SDMMC_CMD_RSP_NONE) 
    {
        cmdSend |= ALT_MSHC_CMD_RESP_EXP;		
        if (cmd->rspType & SDMMC_CMD_RSP_LEN136)
        {    
        	cmdSend |= ALT_MSHC_CMD_RESP_LONG;
        }
    }

    if (cmd->rspType & SDMMC_CMD_RSP_CRC)
    {    
    	cmdSend |= ALT_MSHC_CMD_RESP_CRC;
    }

    if (cmd->cmdIdx == SDMMC_CMD0_GO_IDLE_STATE)
    {
        cmdSend |= ALT_MSHC_CMD_INIT;
    }

    /* always use hold reg as drvsel is not zero */    
    cmdSend |= ALT_MSHC_CMD_USE_HOLD_REG; 

    if (cmd->hasData) 
    {
        status = altSocGen5DwMshc_ResetFifo (pDev);
        if (status == ERROR)
            return ERROR;

        dataLen = cmd->cmdData.blkNum *cmd->cmdData.blkSize;
        buf32   = (UINT32 *)cmd->cmdData.buffer;
        if ((buf32 == NULL) || (dataLen == 0))
            {
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                             "altSocGen5DwMshc_CmdPrepare: wrong cmd data\n",
                             1, 2, 3, 4, 5, 6);
            return ERROR;
            }

        CSR_WRITE_4 (pDev, ALT_MSHC_BYTCNT, dataLen);
        CSR_WRITE_4 (pDev, ALT_MSHC_BLKSIZ, cmd->cmdData.blkSize);
		
        pDrvCtrl->remainBytes = dataLen;

        cmdSend |= ALT_MSHC_CMD_DATA_EXP;

        if (cmd->cmdData.blkNum > 1)
        {
            cmdSend |= ALT_MSHC_CMD_SEND_AUTO_STOP;
        }
		
        if (!cmd->cmdData.isRead) 
        {    
        	cmdSend |= ALT_MSHC_CMD_DATA_WR;
        }

		/* dma mode*/
        if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_DMA)
        {
            altSocGen5DwMshc_IdmaStart (pDev);
        }
        else /* fifo mode*/
        {
            /* PIO mode */            
            if (!cmd->cmdData.isRead)   /* fifo-write mode */
	        {
	            /* always multiple of ALT_MSHC_FIFO_STEPS */                
	            if ((dataLen / ALT_MSHC_FIFO_STEPS) >= ALT_MSHC_DEFAULT_FIFO_LEN) 
	            {
	                count = ALT_MSHC_DEFAULT_FIFO_LEN;
	            }
	            else
	            {
	                count = dataLen / ALT_MSHC_FIFO_STEPS;
	            }

	            for (i = 0; i < count; i++) 
	            {
	                CSR_WRITE_4 (pDev, ALT_MSHC_DATA_FIFO, SWAP32(*buf32));
	                buf32++;
	            }

	            pDrvCtrl->remainBytes -= count * ALT_MSHC_FIFO_STEPS;
	        }
        }
    }

    return (altSocGen5DwMshc_SendCmd (pDev, cmdSend, cmd->cmdArg));
}


/*******************************************************************************
*
* altSocGen5DwMshc_SendCmd - send the command to the controller
*
* This routine sends the command to the controller and waits until it is 
* accepted.
*
* RETURNS: OK, or ERROR if command is not accepted by controller until time out.
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwMshc_SendCmd
    (
    VXB_DEVICE_ID       pDev, 
    UINT32              cmd, 
    UINT32              arg
    )
{
    UINT32              i;
    
    CSR_WRITE_4 (pDev, ALT_MSHC_CMDARG, arg);
    CSR_WRITE_4 (pDev, ALT_MSHC_CMD, (UINT32)ALT_MSHC_CMD_START | cmd);
 
    for (i=0; i<ALT_MSHC_MAX_RETRIES; i++) 
    {
	   vxbUsDelay(10);
	   
       /* if ((CSR_READ_4 (pDev, ALT_MSHC_CMD) & (UINT32)ALT_MSHC_CMD_START) == 0)*/
        if ((CSR_READ_4 (pDev, ALT_MSHC_RINTSTS) & (UINT32)ALT_MSHC_INT_CMD_DONE) == ALT_MSHC_INT_CMD_DONE)
        {
        	/**/
        	/* clear the CMD_DONE*/
        	/**/
			/*CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, ALT_MSHC_INT_CMD_DONE);*/
			
			ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ALL, 
							 "ok-SendCmd: cmd=0x%x, arg=0x%x ALT_MSHC_CMD:0x%08X (%d)\n",
							 cmd, arg, CSR_READ_4 (pDev, ALT_MSHC_RINTSTS), i, 5, 6);
            return OK;
        }
    }

    /*
    ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                     "cmd=0x%x, arg=0x%x send timeout 0x%x\n",
                     cmd, arg, CSR_READ_4 (pDev, ALT_MSHC_CMD), 4, 5, 6);
	*/
	
	/*CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, 0xFFFFFFFF);*/
	
    ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                     "err-SendCmd: cmd=0x%x, arg=0x%x send timeout-ALT_MSHC_RINTSTS:0x%08X (%d) \n",
                     cmd, arg, CSR_READ_4 (pDev, ALT_MSHC_RINTSTS), i, 5, 6);
	
    return ERROR;
}


/*******************************************************************************
*
* altSocGen5DwMshc_CmdIssue - issue the command
*
* This routine issues the command and checks the command status.
*
* RETURNS: OK, or ERROR if the command can not be complete successfully.
*
* ERRNO: N/A
*/

#if 0
STATUS altSocGen5DwMshc_CmdIssue(SDMMC_HOST * host)
{
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    SDMMC_CMD *         cmd;
    VXB_DEVICE_ID       pDev;
    STATUS              rc;
    UINT32              mintsts;
    UINT32              idsts;
    UINT32              i;

    rc = altSocGen5DwMshc_CmdPrepare (host);

    if (rc == ERROR)
        return ERROR;

    pDrvCtrl = (ALT_MSHC_DRV_CTRL *)host->pDev->pDrvCtrl;
    cmd      = &host->cmdIssued;
    pDev     = host->pDev;

    if (!pDrvCtrl->polling)  /* irq mode*/
    {
        rc = semTake (pDrvCtrl->cmdDone, (sysClkRateGet() * ALT_DW_MSHC_CMD_WAIT_IN_SECS));		
        if (rc == ERROR)
        {
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                             "altSocGen5DwMshc_CmdIssue: CMD%d timeout\n",
                             cmd->cmdIdx, 2, 3, 4, 5, 6);
            cmd->cmdErr |= SDMMC_CMD_ERR_TIMEOUT;
            return ERROR;
        }
        else
        {
            if (cmd->cmdErr != 0)
            {
                ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                                 "altSocGen5DwMshc_CmdIssue: CMD%d error\n",
                                 cmd->cmdIdx, 2, 3, 4, 5, 6);
                return ERROR;
            }
        }
    }
    else  /* poll mode*/
    {
        /* poll and check status */         
        for (i = 0; i < ALT_MSHC_MAX_RETRIES; i++) 
        {
            /*mintsts = CSR_READ_4 (pDev,  ALT_MSHC_MINTSTS);*/
            mintsts = CSR_READ_4 (pDev,  ALT_MSHC_RINTSTS);
			
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_CMD, 
                             "CmdIssue->altSocGen5DwMshc_CmdIssue: mintsts 0x%08X (%d) \n",
                             mintsts, i, 3, 4, 5, 6);

			/**/
			/* clear the int_status: CMD_DONE bit*/
			/**/
            if ((mintsts & ALT_MSHC_INT_CMD_DONE) != 0)  /* cmd done*/
            {
                CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, mintsts);
                break;
            }

            if ((mintsts & ALT_MSHC_INT_RTO) != 0) 
            {
                ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                                 "altSocGen5DwMshc_CmdIssue: "
                                 "response time out\n",
                                 1, 2, 3, 4, 5, 6);
				
                CSR_WRITE_4(pDev, ALT_MSHC_RINTSTS, mintsts);
                cmd->cmdErr |= SDMMC_CMD_ERR_TIMEOUT;
                return ERROR;
            }

            if (((mintsts & ALT_MSHC_INT_RCRC) != 0) && 
                ((cmd->rspType & SDMMC_CMD_RSP_CRC) != 0))
            {
                ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                                 "altSocGen5DwMshc_CmdIssue: "
                                 "response CRC error\n",
                                 1, 2, 3, 4, 5, 6);
				
                CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, mintsts);
                cmd->cmdErr |= SDMMC_CMD_ERR_PROTOCOL;
                return ERROR;
            }

            if ((mintsts & ALT_MSHC_INT_RE) != 0)
            {
                ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                                 "altSocGen5DwMshc_CmdIssue: response error\n",
                                 1, 2, 3, 4, 5, 6);
				
                CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, mintsts);
                cmd->cmdErr |= SDMMC_CMD_ERR_PROTOCOL;
                return ERROR;
            }

            vxbUsDelay(10);
        }

        if (i >= ALT_MSHC_MAX_RETRIES)
        {
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                             "altSocGen5DwMshc_CmdIssue: CMD%d timeout\n",
                             cmd->cmdIdx, 2, 3, 4, 5, 6);
			
            cmd->cmdErr |= SDMMC_CMD_ERR_TIMEOUT;
            return ERROR;
        }
    }

    if (cmd->rspType != SDMMC_CMD_RSP_NONE)
        {
        if ((cmd->rspType & SDMMC_CMD_RSP_LEN136) != 0)
            {
            UINT32 cmdRsp[4];

            cmdRsp[0] = CSR_READ_4 (pDev, ALT_MSHC_RESP0);
            cmdRsp[1] = CSR_READ_4 (pDev, ALT_MSHC_RESP1);
            cmdRsp[2] = CSR_READ_4 (pDev, ALT_MSHC_RESP2);
            cmdRsp[3] = CSR_READ_4 (pDev, ALT_MSHC_RESP3);

            cmd->cmdRsp[0] = be32toh (cmdRsp[3]);
            cmd->cmdRsp[1] = be32toh (cmdRsp[2]);
            cmd->cmdRsp[2] = be32toh (cmdRsp[1]);
            cmd->cmdRsp[3] = be32toh (cmdRsp[0]);
            }
        else
            {
            cmd->cmdRsp[0] = CSR_READ_4 (pDev, ALT_MSHC_RESP0);
            cmd->cmdRsp[1] = 0;
            cmd->cmdRsp[2] = 0;
            cmd->cmdRsp[3] = 0;
            }
        }

    if (cmd->hasData)
    {
        if (!pDrvCtrl->polling)
            {
            /* interrupt mode */
            rc = semTake (pDrvCtrl->dataDone, (sysClkRateGet() * ALT_DW_MSHC_CMD_WAIT_IN_SECS));
            if (rc == ERROR)
                {
                ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                                 "altSocGen5DwMshc_CmdIssue: data time out\n",
                                 1, 2, 3, 4, 5, 6);
                cmd->dataErr |= SDMMC_DATA_ERR_TIMEOUT;
                return ERROR;
                }
            else
                {
                if (cmd->dataErr != 0)
                    {
                    ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                                     "altSocGen5DwMshc_CmdIssue: data error\n",
                                     1, 2, 0, 0, 0, 0);
                    return ERROR;
                    }
                }
        }
        else
        {
            /* poll mode */
            if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_PIO)
            {
                /* pio */
                if (cmd->cmdData.isRead)  /* pio_read*/
                {
                    rc = altSocGen5DwMshc_PioRead (pDev);
                    if (rc == ERROR)
                        {
                        return ERROR;
                        }
                }
                else  /* pio_write*/
                {
                    rc = altSocGen5DwMshc_PioWrite (pDev);
                    if (rc == ERROR)
                    {
                        return ERROR;
                    }

                    rc = altSocGen5DwMshc_PollIntsts (pDev, ALT_MSHC_INT_DTO);
                    if (rc == ERROR)
                    {
                        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_RW, 
                                         "PIO write waiting data over failed\n",
                                         1, 2, 3, 4, 5, 6);
                        return ERROR;
                    }
                }
            }
            else
            {
                /* dma */
                for (i = 0; i < ALT_MSHC_MAX_RETRIES; i++) 
                {
                    idsts = CSR_READ_4 (pDev,  ALT_MSHC_IDSTS_8CH);
                    ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_CMD, 
                                     "altSocGen5DwMshc_CmdIssue: idsts 0x%x\n",
                                     idsts, 2, 3, 4, 5, 6);
            
                    if ((idsts & (ALT_MSHC_IDMA_INT_TI | ALT_MSHC_IDMA_INT_RI |
                                  ALT_MSHC_IDMA_INT_NI)) != 0)
                    {
                        CSR_WRITE_4 (pDev, ALT_MSHC_IDSTS_8CH, idsts);
                        break;
                    }

                    if ((idsts & ALT_MSHC_IDMA_INT_AI) != 0) 
                    {
                        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                                         "altSocGen5DwMshc_CmdIssue: "
                                         "CMD%d dma error\n",
                                         cmd->cmdIdx, 2, 3, 4, 5, 6);
                        CSR_WRITE_4 (pDev, ALT_MSHC_IDSTS_8CH, idsts);
                        cmd->cmdErr |= SDMMC_DATA_ERR_PROTOCOL;
                        break;
                    }

                    vxbMsDelay (1);
                }

                if (i >= ALT_MSHC_MAX_RETRIES)
                {
                    ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                                     "altSocGen5DwMshc_CmdIssue: "
                                     "CMD%d dma timeout\n",
                                     cmd->cmdIdx, 2, 3, 4, 5, 6);
                    cmd->cmdErr |= SDMMC_DATA_ERR_TIMEOUT;
                }
            }
        }

        /* stop internal dma */        
        if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_DMA)
        {
            altSocGen5DwMshc_IdmaStop (pDev);
            if (cmd->cmdErr != 0)
                return ERROR;
        }
    }

    return OK;
}

#else

int sdmmc_txData_poll2(VXB_DEVICE_ID          pDev, UINT8 *pBuf, int tx_len)
{
    UINT32 tmp32;
    int timeout = 0;
	int cnt = 0;
    
    /*timeout = 0x100000;*/
    timeout = 0x200000;
    while (cnt < tx_len)
    {
        if (!(CSR_READ_4 (pDev, ALT_MSHC_STATUS) & FIFO_FULL))
        {
            CSR_WRITE_4 (pDev, ALT_MSHC_DATA_FIFO, *(UINT32 *)(pBuf + cnt));
            cnt += 4;
        }

        timeout--;
        if (timeout <= 0)
        {
            return (ERROR);
        }
    }
	
	/**/
	/* poll */
	/**/
    timeout = SDMMC_POLL_TIMEOUT;
    do 
	{
        delay_1us();
        tmp32 = CSR_READ_4 (pDev, ALT_MSHC_RINTSTS);
		
        timeout--;
        if (timeout <= 0)
        {
            return (ERROR);
        }
    } while (!(tmp32 & RAW_INT_STATUS_DTO)); /* bit3 - 数据传输（DTO）*/
    
    return cnt;
}


int sdmmc_rxData_poll2(VXB_DEVICE_ID          pDev, UINT8 *pBuf, int rx_len) 
{
    int timeout = 0;
    UINT32 tmp32 = 0;
    int cnt = 0;
    
   /* timeout = 0x100000;*/
    timeout = 0x200000;
    while (cnt < rx_len)
    {
        timeout--;
		
        if (!(CSR_READ_4 (pDev, ALT_MSHC_STATUS) & FIFO_EMPTY))
        {
            *(UINT32 *)(pBuf + cnt) = CSR_READ_4 (pDev, ALT_MSHC_DATA_FIFO);
            cnt += 4;
        }

        if (timeout <= 0)
        {
            return (ERROR);
        }
    }
    
    return cnt;
}


UINT32 sdmmc_CmdIdx_2_CmdVal(UINT32 cardType, UINT8 cmd_idx)
{
    UINT32 cmd_data = 0;
	
    switch (cmd_idx)
    {
        /*No response commands*/
        case CMD_GO_IDLE_STATE:
            cmd_data = (START_CMD + SEND_INITIALIZATION);
        	break;
		
        case CMD_SET_DSR:
        case CMD_GO_INACTIVE_STATE:
            cmd_data = cmd_idx;
        	break;
		
         /*Long response commands*/
        case CMD_ALL_SEND_CID:
        case CMD_SEND_CSD:
        case CMD_SEND_CID:
            cmd_data = (RESPONSE_EXPECT
                      + RESPONSE_LENGTH_LONG 
                      + cmd_idx);
        	break;
		
        /*Short response commands*/
        case CMD_GO_IRQ_STATE:
        case CMD_SEND_OP_COND:
        case CMD_SET_RELATIVE_ADDR:
        case CMD_SET_BLOCKLEN:
        case CMD_SET_BLOCK_COUNT:
        case CMD_SET_WRITE_PROT:
        case CMD_CLR_WRITE_PROT:
        case CMD_ERASE_GROUP_START:
        case CMD_ERASE_GROUP_END:
        case CMD_GEN_CMD:
        case ACMD_OP_COND:
        case CMD_ACMD_SET_BUS_WIDTH:
        case CMD_SELECT_CARD:
        case CMD_SEND_STATUS:
        case CMD_APP_CMD:
        /*case ACMD_SEND_SCR:*/
        case CMD_SEND_IF_COND:
            cmd_data = (RESPONSE_EXPECT
                      + cmd_idx);
        	break;
		
        /*Stop/abort command*/
        case CMD_STOP_TRANSMISSION:
            cmd_data = (RESPONSE_EXPECT
                      + STOP_ABORT_CMD
                      + cmd_idx);
        	break;
		
        /*Have data command write*/
        case CMD_WRITE_BLOCK:
            cmd_data = (WAIT_PRVDATA_COMPLETE
                      + NREADWRITE
                      + DATA_EXPECTED
                      + CHECK_RESPONSE_CRC
                      + RESPONSE_EXPECT
                      + cmd_idx) & ~SEND_AUTO_STOP;
        	break;
		
        case CMD_WRITE_MULTIPLE_BLOCK:
            if (cardType == SD)
            {    cmd_data = (SEND_AUTO_STOP
                          + WAIT_PRVDATA_COMPLETE
                          + NREADWRITE
                          + DATA_EXPECTED
                          + CHECK_RESPONSE_CRC
                          + RESPONSE_EXPECT
                          + cmd_idx);
            }
            else
            {
                cmd_data = (WAIT_PRVDATA_COMPLETE
                          + NREADWRITE
                          + DATA_EXPECTED
                          + CHECK_RESPONSE_CRC
                          + RESPONSE_EXPECT
                          + cmd_idx) & ~SEND_AUTO_STOP;
            }
        	break;
		
        /*Have data command read*/
        case CMD_READ_MULTIPLE_BLOCK:
            if (cardType == SD)
            {    cmd_data = (SEND_AUTO_STOP
                          + WAIT_PRVDATA_COMPLETE
                          + DATA_EXPECTED
                          + CHECK_RESPONSE_CRC
                          + RESPONSE_EXPECT
                          + cmd_idx);
            }
            else
            {
                cmd_data = (WAIT_PRVDATA_COMPLETE
                          + DATA_EXPECTED
                          + CHECK_RESPONSE_CRC
                          + RESPONSE_EXPECT
                          + cmd_idx) & ~SEND_AUTO_STOP;
            }
        	break;
		
        case ACMD_SEND_SCR:			
        case CMD_READ_SINGLE_BLOCK:
            cmd_data = (WAIT_PRVDATA_COMPLETE
                      + DATA_EXPECTED
                      + CHECK_RESPONSE_CRC
                      + RESPONSE_EXPECT
                      + cmd_idx);
        	break;
		
        case CMD_SWITCH_FUNC:
        case MMC_SEND_EXT_CSD:
            cmd_data = (WAIT_PRVDATA_COMPLETE
                      + DATA_EXPECTED
                      + CHECK_RESPONSE_CRC
                      + RESPONSE_EXPECT
                      + (cmd_idx >> 4));
        	break;
		
        default:
            printf("unknown command %d.\r\n", cmd_idx, 2,3,4,5,6);
            cmd_data = 0;
        	break;
    }

	if (cmd_data != 0)
	{
	    cmd_data |= START_CMD;
	}
	
    return cmd_data;
}

STATUS altSocGen5DwMshc_CmdIssue(SDMMC_HOST * host)
{
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    SDMMC_CMD *         cmd;
    VXB_DEVICE_ID       pDev;
    STATUS              rc;
    UINT32              mintsts;
    UINT32              idsts;
    UINT32              i;
	
    UINT32  tmp32 = 0;
    UINT32  cmdVal = 0, cardType = 0;
	int  timeout = 0, ret = 0;

    pDrvCtrl = (ALT_MSHC_DRV_CTRL *)host->pDev->pDrvCtrl;
    cmd      = &host->cmdIssued;
    pDev     = host->pDev;

	tmp32 = CSR_READ_4 (pDev, ALT_MSHC_CDETECT) & 0x00000001;   /* CARD_DETECT */
	if (tmp32 != 0x00)  /* CARD_INSERT */
	{
		printf("CardDetect: 0x%08X fail! \n", tmp32);
		return ERROR;
	}
	
    if (cmd->rspFlag & (CMD_RESP_FLAG_RDATA | CMD_RESP_FLAG_WDATA))
    {
		CSR_WRITE_4 (pDev, ALT_MSHC_BYTCNT, cmd->cmdData.dataLen);
		CSR_WRITE_4 (pDev, ALT_MSHC_BLKSIZ, cmd->cmdData.blkSize);
    }
	
	tmp32 = CSR_READ_4 (pDev, ALT_MSHC_RINTSTS);
    CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, tmp32);

	/**/
	/* response_timeout: 0xFF*/
	/**/
    CSR_WRITE_4 (pDev, ALT_MSHC_TMOUT, 0xFFFFFFFF);
	
	if (pDrvCtrl->card.isMmc == FALSE)
	{	
		cardType = SD;
	}
	else
	{	
		cardType= EMMC;
	}
	cmdVal = sdmmc_CmdIdx_2_CmdVal(cardType, cmd->cmdIdx);
    /* 
    Send the command (CRC calculated by host). 
	*/
    CSR_WRITE_4 (pDev, ALT_MSHC_CMDARG, cmd->cmdArg);
    CSR_WRITE_4 (pDev, ALT_MSHC_CMD, cmdVal);

	/**/
	/* polling cmd finish*/
	/**/
    timeout = SDMMC_POLL_TIMEOUT * 2;
    do
    {
        sysUsDelay(1);
		
    	tmp32 = CSR_READ_4 (pDev, ALT_MSHC_RINTSTS) & RAW_INT_STATUS_CMD_DONE; /* bit2 - 命令完成（CD） */
			
        timeout--;
        if (timeout <= 0)
        {
            printf("fail: poll CMD[%d] done time_out! \n", cmd->cmdIdx, 2,3,4,5,6);
            return (ERROR);
        }
    } while (!tmp32);

    /* */
    /*clear CMDD int */
	/**/
    CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, RAW_INT_STATUS_CMD_DONE);

   
    if (cmd->rspType != SDMMC_CMD_RSP_NONE)
    {
		tmp32 = CSR_READ_4 (pDev, ALT_MSHC_RINTSTS);
		       
        if (tmp32 & (RAW_INT_STATUS_RTO | RAW_INT_STATUS_DRTO))  /* timeout */
        {
			CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, tmp32);
            /*printf("fail: snd CMD[%d] timeout-int_status:0x%08X \n", cmd->cmdIdx, tmp32, 3,4,5,6);*/
			
           return (ERROR);
        }        
        else if (!(cmd->rspFlag & CMD_RESP_FLAG_NOCRC) && (tmp32 & (RAW_INT_STATUS_RCRC | RAW_INT_STATUS_DCRC)))
        {
			CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, tmp32);
            /*printf("fail: snd CMD[%d] CRC error-int_status:0x%08X \n", cmd->cmdIdx, tmp32, 3,4,5,6);*/
			
            return (ERROR);
        }	
    }

	if (cmd->rspFlag & CMD_RESP_FLAG_DATALINE) 
    {
        ret = sdmmc_rxData_poll2(pDev, (UINT8 *)cmd->cmdData.buffer, cmd->cmdData.dataLen);
    } 
    else 
    {
        if (gSdmmc_Resp_Len_Type2[cmd->rspType] == CMD_RES_LEN_LONG) 
        {
            cmd->cmdRsp[0] = CSR_READ_4 (pDev, ALT_MSHC_RESP0);
            cmd->cmdRsp[1] = CSR_READ_4 (pDev, ALT_MSHC_RESP1);
            cmd->cmdRsp[2] = CSR_READ_4 (pDev, ALT_MSHC_RESP2);
            cmd->cmdRsp[3] = CSR_READ_4 (pDev, ALT_MSHC_RESP3);
        }
        else
        {
            cmd->cmdRsp[0] = CSR_READ_4 (pDev, ALT_MSHC_RESP0);
            cmd->cmdRsp[1] = 0;
            cmd->cmdRsp[2] = 0;
            cmd->cmdRsp[3] = 0;
        }
        
        if (cmd->rspFlag & CMD_RESP_FLAG_RDATA) 
        {
            ret = sdmmc_rxData_poll2(pDev, (UINT8 *)cmd->cmdData.buffer, cmd->cmdData.dataLen);
        }
		else if (cmd->rspFlag & CMD_RESP_FLAG_WDATA) 
        {
            ret = sdmmc_txData_poll2(pDev, (UINT8 *)cmd->cmdData.buffer, cmd->cmdData.dataLen);
        }
    }
     

    return ret;
}

#endif



#if 1

#if 0
/*******************************************************************************
*
* altSocGen5DwMshc_BlkRead - read block from SD/MMC card
*
* This routine reads block from SD/MMC card.
*
* RETURNS: OK, or ERROR if read failed.
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwMshc_BlkRead
    (
    SDMMC_CARD *        card,
    sector_t            blkNo,
    UINT32              numBlks,
    void *              pBuf
    )
    {
    VXB_DEVICE_ID       pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    STATUS              rc;
    UINT32              ix, round, remain;
    void *              bufAddr;
    sector_t            blkAddr;

    if ((card == NULL) || (numBlks == 0) || (pBuf == NULL))
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshc_BlkRead: parameter wrong\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    pDev = card->host->pDev;
    if (pDev == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshc_BlkRead: parameter wrong\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshc_BlkRead: parameter wrong\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    /* split the transfer */
    round = numBlks / ALT_MSHC_MAX_RW_SECTORS;
    remain = numBlks % ALT_MSHC_MAX_RW_SECTORS;

    for (ix = 0; ix < round; ix ++)
        {
        blkAddr = blkNo + ix * ALT_MSHC_MAX_RW_SECTORS;
        bufAddr = (void *)((ULONG)pBuf + ix * ALT_MSHC_MAX_RW_SECTORS *
                           SDMMC_BLOCK_SIZE); 

        /*rc = sdMmcBlkRead (card, blkAddr, ALT_MSHC_MAX_RW_SECTORS, bufAddr);*/
        rc = sdmmc_BlkRead (card, blkAddr, ALT_MSHC_MAX_RW_SECTORS, bufAddr);
        if (rc == ERROR)
            return ERROR;

        /*sdMmcStatusWaitReadyForData (card);*/
        sdmmc_StatusWaitReadyForData (card);

        /* if hardware does not support snoop, we invalidate cache */
        if ((pDrvCtrl->flags & ALT_MSHC_FLAGS_SNOOP_ENABLED) == 0)
            CACHE_USER_INVALIDATE (bufAddr,
                                  (ALT_MSHC_MAX_RW_SECTORS * SDMMC_BLOCK_SIZE));
        }

     if (remain != 0)
         {
         blkAddr = blkNo + round * ALT_MSHC_MAX_RW_SECTORS;
         bufAddr = (void *)((ULONG)pBuf + round * ALT_MSHC_MAX_RW_SECTORS *
                            SDMMC_BLOCK_SIZE);

         /*rc = sdMmcBlkRead (card, blkAddr, remain, bufAddr);*/
         rc = sdmmc_BlkRead (card, blkAddr, remain, bufAddr);
         if (rc == ERROR)
             return ERROR;

         /*sdMmcStatusWaitReadyForData (card);*/
         sdmmc_StatusWaitReadyForData (card);

         /* if hardware does not support snoop, we invalidate cache */
         if ((pDrvCtrl->flags & ALT_MSHC_FLAGS_SNOOP_ENABLED) == 0)
             CACHE_USER_INVALIDATE (bufAddr, (remain * SDMMC_BLOCK_SIZE));
         }

    return OK;
    }

/*******************************************************************************
*
* altSocGen5DwMshc_BlkWrite - write block to SD/MMC card
*
* This routine writes block to SD/MMC card.
*
* RETURNS: OK, or ERROR if write failed.
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwMshc_BlkWrite
    (
    SDMMC_CARD *        card,
    sector_t            blkNo,
    UINT32              numBlks,
    void *              pBuf
    )
    {
    VXB_DEVICE_ID       pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    STATUS              rc;
    UINT32              ix, round, remain;
    sector_t            blkAddr;
    void *              bufAddr;

    if ((card == NULL) || (numBlks == 0) || (pBuf == NULL))
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshc_BlkRead: parameter wrong\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    pDev = card->host->pDev;
    if (pDev == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshc_BlkRead: parameter wrong\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
        {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshc_BlkRead: parameter wrong\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
        }
        
    /* split the transfer */

    round = numBlks / ALT_MSHC_MAX_RW_SECTORS;
    remain = numBlks % ALT_MSHC_MAX_RW_SECTORS;

    for (ix = 0; ix < round; ix ++)
        {
        bufAddr = (void *)((ULONG)pBuf + ix * ALT_MSHC_MAX_RW_SECTORS *
                           SDMMC_BLOCK_SIZE);
        blkAddr = blkNo + ix * ALT_MSHC_MAX_RW_SECTORS;

        /* if hardware does not support snoop, we flush cache */

        if ((pDrvCtrl->flags & ALT_MSHC_FLAGS_SNOOP_ENABLED) == 0)
            CACHE_USER_FLUSH (bufAddr,
                              (ALT_MSHC_MAX_RW_SECTORS * SDMMC_BLOCK_SIZE));
        
        rc = sdMmcBlkWrite (card, blkAddr, ALT_MSHC_MAX_RW_SECTORS, bufAddr);
        if (rc == ERROR )
            return ERROR;

        sdMmcStatusWaitReadyForData (card);
        }

    if (remain != 0)
        {
        bufAddr = (void *)((ULONG)pBuf + round * ALT_MSHC_MAX_RW_SECTORS * 
                           SDMMC_BLOCK_SIZE);
        blkAddr = blkNo + round * ALT_MSHC_MAX_RW_SECTORS;

        if ((pDrvCtrl->flags & ALT_MSHC_FLAGS_SNOOP_ENABLED) == 0)
            CACHE_USER_FLUSH (bufAddr, (remain * SDMMC_BLOCK_SIZE));
        
        rc = sdMmcBlkWrite (card, blkAddr, remain, bufAddr);
        if (rc == ERROR )
            return ERROR;

        sdMmcStatusWaitReadyForData(card);
        }

    return OK;
    }

#else

extern STATUS sdmmc_StatusWaitReadyForData(SDMMC_CARD * card);

LOCAL STATUS altSocGen5DwMshc_BlkRead (SDMMC_CARD * card, sector_t blkNo, UINT32 numBlks,void * pBuf)
{
    VXB_DEVICE_ID       pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    STATUS              rc;
    UINT32              ix, round, remain;
    void *              bufAddr;
    sector_t            blkAddr;

    if ((card == NULL) || (numBlks == 0) || (pBuf == NULL))
    {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshc_BlkRead: parameter wrong\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
    }

    pDev = card->host->pDev;
    if (pDev == NULL)
    {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshc_BlkRead: parameter wrong\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
    }

    pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
    {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshc_BlkRead: parameter wrong\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
    }

    /* split the transfer */
    round = numBlks / ALT_MSHC_MAX_RW_SECTORS;
    remain = numBlks % ALT_MSHC_MAX_RW_SECTORS;

    for (ix = 0; ix < round; ix ++)
    {
        blkAddr = blkNo + ix * ALT_MSHC_MAX_RW_SECTORS;
        bufAddr = (void *)((ULONG)pBuf + ix * ALT_MSHC_MAX_RW_SECTORS *
                           SDMMC_BLOCK_SIZE); 

        rc = sdmmc_BlkRead (card, blkAddr, ALT_MSHC_MAX_RW_SECTORS, bufAddr);
        if (rc == ERROR)
            return ERROR;

        sdmmc_StatusWaitReadyForData (card);

        /* if hardware does not support snoop, we invalidate cache */
        if ((pDrvCtrl->flags & ALT_MSHC_FLAGS_SNOOP_ENABLED) == 0)
            CACHE_USER_INVALIDATE (bufAddr,
                                  (ALT_MSHC_MAX_RW_SECTORS * SDMMC_BLOCK_SIZE));
    }

     if (remain != 0)
     {
         blkAddr = blkNo + round * ALT_MSHC_MAX_RW_SECTORS;
         bufAddr = (void *)((ULONG)pBuf + round * ALT_MSHC_MAX_RW_SECTORS *
                            SDMMC_BLOCK_SIZE);

         rc = sdmmc_BlkRead (card, blkAddr, remain, bufAddr);
         if (rc == ERROR)
             return ERROR;

         sdmmc_StatusWaitReadyForData (card);

         /* if hardware does not support snoop, we invalidate cache */
         if ((pDrvCtrl->flags & ALT_MSHC_FLAGS_SNOOP_ENABLED) == 0)
             CACHE_USER_INVALIDATE (bufAddr, (remain * SDMMC_BLOCK_SIZE));
     }

    return OK;
}

STATUS altSocGen5DwMshc_BlkRead_Poll (SDMMC_CARD * card, sector_t blkNo, UINT32 numBlks,void * buf)
{
    SDMMC_CMD *cmd = &card->host->cmdIssued;
    UINT32 idx, arg;
	int rc = 0;
	
	UINT32 cmdArg = 0, rspFlag = 0;	
	rspFlag = CMD_RESP_FLAG_RDATA;

    if (numBlks > 1)
        idx = SDMMC_CMD18_READ_MULTI_BLOCK;
    else
        idx = SDMMC_CMD17_READ_SINGLE_BLOCK;

    if (card->info.highCapacity)
        arg = (UINT32)blkNo;
    else
        arg = (UINT32)blkNo * SDMMC_BLOCK_SIZE;

    SDMMC_CMD_FORM (cmd, idx, arg, SDMMC_CMD_RSP_R1, TRUE, rspFlag);
    SDMMC_DATA_FORM (cmd, buf, numBlks, SDMMC_BLOCK_SIZE, TRUE, SDMMC_BLOCK_SIZE);

    rc = sdmmc_Host_CmdIssue (card->host);	
	if (rc == ERROR)
	{
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, "altSocGen5DwMshc_BlkRead_Poll: error! \n", 1,2,3,4,5,6);
		return ERROR;
	}
	
	rc = sdmmc_StatusWaitReadyForData (card);
	if (rc == ERROR)
	{
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, "sdmmc_StatusWaitReadyForData: error! \n", 1,2,3,4,5,6);
		return ERROR;
	}

	return OK;
}



LOCAL STATUS altSocGen5DwMshc_BlkWrite(SDMMC_CARD *        card, sector_t blkNo, UINT32 numBlks, void * pBuf)
{
    VXB_DEVICE_ID       pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    STATUS              rc;
    UINT32              ix, round, remain;
    sector_t            blkAddr;
    void *              bufAddr;

    if ((card == NULL) || (numBlks == 0) || (pBuf == NULL))
    {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshc_BlkRead: parameter wrong\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
    }

    pDev = card->host->pDev;
    if (pDev == NULL)
    {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshc_BlkRead: parameter wrong\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
    }

    pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    if (pDrvCtrl == NULL)
    {
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                         "altSocGen5DwMshc_BlkRead: parameter wrong\n",
                         1, 2, 3, 4, 5, 6);
        return ERROR;
    }
        
    /* split the transfer */

    round = numBlks / ALT_MSHC_MAX_RW_SECTORS;
    remain = numBlks % ALT_MSHC_MAX_RW_SECTORS;

    for (ix = 0; ix < round; ix ++)
    {
        bufAddr = (void *)((ULONG)pBuf + ix * ALT_MSHC_MAX_RW_SECTORS *
                           SDMMC_BLOCK_SIZE);
        blkAddr = blkNo + ix * ALT_MSHC_MAX_RW_SECTORS;

        /* if hardware does not support snoop, we flush cache */

        if ((pDrvCtrl->flags & ALT_MSHC_FLAGS_SNOOP_ENABLED) == 0)
            CACHE_USER_FLUSH (bufAddr,
                              (ALT_MSHC_MAX_RW_SECTORS * SDMMC_BLOCK_SIZE));
        
        rc = sdmmc_BlkWrite (card, blkAddr, ALT_MSHC_MAX_RW_SECTORS, bufAddr);
        if (rc == ERROR )
            return ERROR;

        sdmmc_StatusWaitReadyForData (card);
    }

    if (remain != 0)
    {
        bufAddr = (void *)((ULONG)pBuf + round * ALT_MSHC_MAX_RW_SECTORS * 
                           SDMMC_BLOCK_SIZE);
        blkAddr = blkNo + round * ALT_MSHC_MAX_RW_SECTORS;

        if ((pDrvCtrl->flags & ALT_MSHC_FLAGS_SNOOP_ENABLED) == 0)
            CACHE_USER_FLUSH (bufAddr, (remain * SDMMC_BLOCK_SIZE));
        
        rc = sdmmc_BlkWrite (card, blkAddr, remain, bufAddr);
        if (rc == ERROR )
            return ERROR;

        sdmmc_StatusWaitReadyForData(card);
    }

    return OK;
}

STATUS altSocGen5DwMshc_BlkWrite_Poll(SDMMC_CARD *        card, sector_t blkNo, UINT32        numBlks, void * buf)
{
	SDMMC_CMD *cmd = &card->host->cmdIssued;
	UINT32 idx, arg;
	int rc = 0;

	UINT32 cmdArg = 0, rspFlag = 0, dataLen = 0;	
	rspFlag = CMD_RESP_FLAG_WDATA;

	if (numBlks > 1)
		idx = SDMMC_CMD25_WRITE_MULTI_BLOCK;
	else
		idx = SDMMC_CMD24_WRITE_SINGLE_BLOCK;

	if (card->info.highCapacity)
	{	
		arg = (UINT32)blkNo;
	}
	else
	{	
		arg = (UINT32)blkNo * SDMMC_BLOCK_SIZE;
	}

	SDMMC_CMD_FORM (cmd, idx, arg, SDMMC_CMD_RSP_R1, TRUE, rspFlag);
	dataLen = numBlks * SDMMC_BLOCK_SIZE;
	SDMMC_DATA_FORM (cmd, buf, numBlks, SDMMC_BLOCK_SIZE, TRUE, dataLen);

	rc = sdmmc_Host_CmdIssue (card->host);	
	if (rc == ERROR)
	{
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, "altSocGen5DwMshc_BlkRead_Poll: error! \n", 1,2,3,4,5,6);
		return ERROR;
	}
	
	rc = sdmmc_StatusWaitReadyForData (card);
	if (rc == ERROR)
	{
        ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, "sdmmc_StatusWaitReadyForData: error! \n", 1,2,3,4,5,6);
		return ERROR;
	}

	return OK;
}


#endif



/*******************************************************************************
*
* altSocGen5DwMshc_PollIntsts - poll the interrupt status register
*
* This routine polls the status of the interrupt status register.
*
* RETURNS: OK, or ERROR if there's wrong status.
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwMshc_PollIntsts
    (
    VXB_DEVICE_ID       pDev,
    UINT32              mask
    )
{
    UINT32              intsts;
    UINT32              i;
	int j, k, m;
    
    for (i = 0; i < ALT_MSHC_MAX_RETRIES; i++)
    {
        /*intsts = CSR_READ_4 (pDev, ALT_MSHC_MINTSTS);  // sd-0 write sector ok, sd-1 fail*/
        intsts = CSR_READ_4 (pDev, ALT_MSHC_RINTSTS);

        if ((intsts & ALT_MSHC_INT_ERROR) != 0)
        {
            CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, intsts);  /* clear the mask_bit*/
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_RW, 
                             "Poll MINTSTS failed with:0x%08X (%d) \n", 
                             intsts, i, 3, 4, 5, 6);
            return ERROR;
        }

        if ((intsts & mask) != 0)
        {
            CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, (intsts & mask));  /* clear the mask_bit*/
            return OK;
        }

       vxbUsDelay (100);
       /*
       for (j=0; j<1000; j++)
       {
       		for (k=0; k<1000; k++)
       		{
       			m = k;
       		}
       }
	   */
    }

    CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, intsts);  /* clear the mask_bit*/
    ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_RW, "Poll MINTSTS time out with:0x%08X \n", 
                     intsts, 2, 3, 4, 5, 6);
    return ERROR;
}

/*******************************************************************************
*
* altSocGen5DwMshc_ReadFifo - PIO read the internal FIFO buffer
*
* This routine reads the internal FIFO buffer using PIO method.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshc_ReadFifo
    (
    VXB_DEVICE_ID       pDev
    )
{    
    ALT_MSHC_DRV_CTRL * pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    SDMMC_HOST *        host = &pDrvCtrl->host;
    UINT32 *            buf = (UINT32 *)host->cmdIssued.cmdData.buffer;
    UINT32              i;
    UINT32              bytesRead;
    UINT32              mshcStatus;
    UINT32              fifoLen;

    bytesRead = host->cmdIssued.cmdData.blkNum * host->cmdIssued.cmdData.blkSize
                - pDrvCtrl->remainBytes;
	
    buf += bytesRead / ALT_MSHC_FIFO_STEPS;
    
    mshcStatus = CSR_READ_4 (pDev, ALT_MSHC_STATUS);
    fifoLen = ALT_MSHC_STATUS_FIFO_COUNT (mshcStatus);

    for (i = 0; i < fifoLen; i++)
    {
        *buf = SWAP32(CSR_READ_4(pDev, ALT_MSHC_DATA_FIFO));
        buf++;
    }

    pDrvCtrl->remainBytes -= fifoLen * ALT_MSHC_FIFO_STEPS;

#if 0
    ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_RW, "PIO Fifo read %d bytes, remain %d bytes,mshcStatus:0x%08X \n", 
                     fifoLen * 4, pDrvCtrl->remainBytes, mshcStatus, 4, 5, 6);
#endif

	return;
}

/*******************************************************************************
*
* altSocGen5DwMshc_WriteFifo - PIO write the internal FIFO buffer
*
* This routine writes the internal FIFO buffer using PIO method.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshc_WriteFifo
    (
    VXB_DEVICE_ID       pDev
    )
    {
    ALT_MSHC_DRV_CTRL * pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    SDMMC_HOST *        host = &pDrvCtrl->host;
    UINT32 *            buf = (UINT32 *)host->cmdIssued.cmdData.buffer;
    UINT32              i;
    UINT32              mshcStatus;
    UINT32              bytesWritten;
    UINT32              fifoSlots;
    UINT32              writeLen;    

    bytesWritten = host->cmdIssued.cmdData.blkNum * 
                   host->cmdIssued.cmdData.blkSize - pDrvCtrl->remainBytes;
    buf += bytesWritten / ALT_MSHC_FIFO_STEPS;
 
    mshcStatus =  CSR_READ_4 (pDev, ALT_MSHC_STATUS);
    fifoSlots = ALT_MSHC_STATUS_FIFO_COUNT (mshcStatus);
    fifoSlots = ALT_MSHC_DEFAULT_FIFO_LEN - fifoSlots;
    
    if (pDrvCtrl->remainBytes > (fifoSlots * ALT_MSHC_FIFO_STEPS))
        {
        writeLen  = fifoSlots;
        } 
    else 
        {
        writeLen = pDrvCtrl->remainBytes / ALT_MSHC_FIFO_STEPS;
        }

    for (i = 0; i < writeLen; i++)
        {
        CSR_WRITE_4 (pDev, ALT_MSHC_DATA_FIFO, SWAP32(*buf));
        buf++;
        }

    pDrvCtrl->remainBytes -= writeLen * ALT_MSHC_FIFO_STEPS;
    ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_RW, "PIO Fifo write %d bytes, remain %d bytes\n", 
                     writeLen * 4, pDrvCtrl->remainBytes, 3, 4, 5, 6);
    }

/*******************************************************************************
*
* altSocGen5DwMshc_PioRead - read data from SD/MMC card using PIO
*
* This routine reads data from SD/MMC card using PIO.
*
* RETURNS: OK, or ERROR if read failed.
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwMshc_PioRead
    (
    VXB_DEVICE_ID       pDev
    )
{    
    ALT_MSHC_DRV_CTRL * pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    STATUS              status;

    while (pDrvCtrl->remainBytes != 0) 
    {
#if 0
    	if (!pDrvCtrl->polling) /* irq_mode */
    	{
	        status = altSocGen5DwMshc_PollIntsts (pDev,
	                                             ALT_MSHC_INT_RXDR | 
	                                             ALT_MSHC_INT_DTO  | 
	                                             ALT_MSHC_INT_HTO);
	        if (status == ERROR)
	        {
	            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
	                             "PIO read error, remain %d bytes\n", 
	                             pDrvCtrl->remainBytes, 2, 3, 4, 5, 6);
	            return ERROR;
	        }
    	}
		else /* poll_mode */
		{
	        status = altSocGen5DwMshc_PollIntsts (pDev,
	                                             ALT_MSHC_INT_RXDR | 
	                                             ALT_MSHC_INT_DTO  | 
	                                             ALT_MSHC_INT_HTO);
	        if (status == ERROR)
	        {
	            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
	                             "PIO read error, remain %d bytes\n", 
	                             pDrvCtrl->remainBytes, 2, 3, 4, 5, 6);
	            return ERROR;
	        }		
		}
#endif

        altSocGen5DwMshc_ReadFifo (pDev);
        vxbUsDelay (100);
    }

    return OK;
}

/*******************************************************************************
*
* altSocGen5DwMshc_PioWrite - write data to SD/MMC card using PIO
*
* This routine writes data to SD/MMC card using PIO.
*
* RETURNS: OK, or ERROR if write failed
*
* ERRNO: N/A
*/

LOCAL STATUS altSocGen5DwMshc_PioWrite
    (
    VXB_DEVICE_ID       pDev
    )
{
    ALT_MSHC_DRV_CTRL * pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    STATUS              status;
    
    while (pDrvCtrl->remainBytes != 0) 
    {
     /*   
        status = altSocGen5DwMshc_PollIntsts (pDev, 
                                             ALT_MSHC_INT_TXDR | 
                                             ALT_MSHC_INT_HTO);
        if (status == ERROR)
        {
            ALT_DW_MSHC_DBG (ALT_DW_MSHC_DBG_ERR, 
                             "PIO write error, remain %d bytes\n", 
                             pDrvCtrl->remainBytes, 2, 3, 4, 5, 6);
            return ERROR;
        }
		*/

        altSocGen5DwMshc_WriteFifo (pDev);
        vxbUsDelay (100);
    }

    return OK;
}

#endif





#if 1



/*******************************************************************************
*
* altSocGen5DwMshc_VddSetup - setup the SD bus voltage level and power it up
*
* This routine sets up the SD bus voltage and powers it up.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshc_VddSetup
    (
    SDMMC_HOST *        host,
    UINT32              vdd
    )
{
   /* CSR_WRITE_4 (host->pDev, ALT_MSHC_PWREN, 1);*/
    CSR_WRITE_4 (host->pDev, ALT_MSHC_PWREN, 0xFFFF);
    vxbMsDelay(100);
}

/*******************************************************************************
*
* altSocGen5DwMshc_ClkFreqSetup - setup the clock frequency
*
* This routine sets up the clock frequency.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshc_ClkFreqSetup
    (
    SDMMC_HOST *        host,
    UINT32              clk
    )
{
    VXB_DEVICE_ID       pDev = host->pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    UINT32              clkFreq = pDrvCtrl->clkFreq;
    UINT32              div;
    static UINT32       freqSaved = 0;

    if (clk >= clkFreq)
    {
        div = 0;
        clk = clkFreq;
    }
    else if (clkFreq % (clk * 2))
    {
        div = ((clkFreq / clk) >> 1) + 1;
    }
    else
    {
        div = (clkFreq  / clk) >> 1;
    }

    if (freqSaved == clk)
        return;
    else
        freqSaved = clk;

    CSR_WRITE_4 (pDev, ALT_MSHC_CLKENA, 0);
    CSR_WRITE_4 (pDev, ALT_MSHC_CLKSRC, 0);
     
    altSocGen5DwMshc_SendCmd (pDev, ALT_MSHC_CMD_UPD_CLK |
                            ALT_MSHC_CMD_PRV_DAT_WAIT, 0);

    if (pDrvCtrl->clkFreqSetup != NULL)
        {
        pDrvCtrl->clkFreqSetup (clk);
        }

    CSR_WRITE_4 (pDev, ALT_MSHC_CLKDIV, div);
    CSR_WRITE_4 (pDev, ALT_MSHC_CLKENA, ALT_MSHC_CLKENA_CCLK_ENABLE);

    altSocGen5DwMshc_SendCmd (pDev, ALT_MSHC_CMD_UPD_CLK |
                             ALT_MSHC_CMD_PRV_DAT_WAIT, 0);

}


/*******************************************************************************
*
* altSocGen5DwMshc_BusWidthSetup - setup the bus width
*
* This routine sets up the bus width.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void altSocGen5DwMshc_BusWidthSetup
    (
    SDMMC_HOST * host,
    UINT32       width
    )
{
    UINT32       buswidth;

    buswidth = ALT_MSHC_CTYPE_1BIT;
    
    switch (width) 
    {
    case SDMMC_BUS_WIDTH_1BIT:
        buswidth = ALT_MSHC_CTYPE_1BIT;
        break;
    case SDMMC_BUS_WIDTH_4BIT:
        buswidth = ALT_MSHC_CTYPE_4BIT;
        break;
    case SDMMC_BUS_WIDTH_8BIT:
        buswidth = ALT_MSHC_CTYPE_8BIT;
        break;
    default:
        /* should not be there but use default */
        break;
    }
            
    CSR_WRITE_4 (host->pDev, ALT_MSHC_CTYPE, buswidth);
}


/*******************************************************************************
*
* altSocGen5DwMshc_CardWpCheck - check if card is write protected
*
* This routine checks if card is write protected.
*
* RETURNS: FALSE or TRUE if card is write protected.
*
* ERRNO: N/A
*/

LOCAL BOOL altSocGen5DwMshc_CardWpCheck
    (
    SDMMC_HOST * host
    )
{
    VXB_DEVICE_ID       pDev = host->pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;
    BOOL                wp = FALSE;

    if ((pDrvCtrl->flags & ALT_MSHC_FLAGS_CARD_WRITABLE) != 0)
        {
        wp = FALSE;
        }
    else
        {
        wp = (CSR_READ_4 (pDev, ALT_MSHC_WRTPRT) & 1) != 0;
        }

    return wp;
}

#endif

/*******************************************************************************
*
* altSocGen5DwMshc_Show - print MSHC controller and device related information
*
* This routine prints MSHC controller and device related information.
*
* RETURNS: OK, or ERROR if parameter is wrong.
*
* ERRNO: N/A
*/

STATUS altSocGen5DwMshc_Show
    (
    int                 unit
    )
{
    VXB_DEVICE_ID       pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl;

    pDev = vxbInstByNameFind (ALT_DW_MSHC_NAME, unit);
    if (pDev == NULL)
        return ERROR;

    pDrvCtrl = (ALT_MSHC_DRV_CTRL *)pDev->pDrvCtrl;

    printf ("[%s]: regBase @ %p, rwMode (%s) polling (%s) flags (0x%x)\n",
            ALT_DW_MSHC_NAME,
            pDrvCtrl->regBase, 
            (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_DMA) ? "DMA" : "PIO",
            (pDrvCtrl->polling) ? "TRUE" : "FALSE",
            pDrvCtrl->flags);

    sdmmc_HostShow (&pDrvCtrl->host);
	
    if (pDrvCtrl->cardIns)
        sdmmc_CardShow (&pDrvCtrl->card);
    else
        printf ("\nNo SD/MMC card inserted\n");

    return OK;
}



/* use timestamp to test performance, must add INCLUDE_TIMESTAMP component */

#if 1
/* #ifdef ALT_DW_MSHC_DBG_ON */

#include <tickLib.h>

/*******************************************************************************
*
* altSocGen5DwMshc_Test - test SD card raw read and write
*
* This routine tests SD card raw block read and write.
*
* RETURNS: OK, or ERROR if parameter is wrong.
*
* ERRNO: N/A
*
* \NOMANUAL
*
*/

STATUS altSocGen5DwMshc_Test
    (
    int                 unit,
    UINT32              blkStart,
    UINT32              blkNum
    )
{
    VXB_DEVICE_ID       pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    SDMMC_CARD *        card;
    UINT32              i, j, k;
    UINT8 *             pBuf;
    UINT32              timeStart, timeEnd;
    UINT32              tickStart, tickEnd, tickDiff, timeDiff;
    float               speed;

    pDev = vxbInstByNameFind (ALT_DW_MSHC_NAME, unit);
    if (pDev == NULL)
    {
        printf ("Can not find this sd unit %d.\n", unit);
        return ERROR;
    }
	else
	{
		printf ("vxbInstByNameFind(%s): 0x%X \n", ALT_DW_MSHC_NAME, pDev);
	}
    pDrvCtrl= (ALT_MSHC_DRV_CTRL *) pDev->pDrvCtrl;
    card = &pDrvCtrl->card;

	printf ("(blkStart+blkNum): %d \n", (blkStart + blkNum));
	printf ("card->info.blkNum: %d \n", card->info.blkNum);
	
    if ((blkStart + blkNum) > card->info.blkNum)
    {
        printf ("Test block exceeds the sd card capacity.\n");
        return ERROR;
    }

    pBuf = (UINT8 *)cacheDmaMalloc (blkNum * SDMMC_BLOCK_SIZE);
    if (pBuf == NULL)
        return ERROR;

    printf ("Test buffer addr = 0x%08x.\n", pBuf);

    for (i = 0; i < blkNum * SDMMC_BLOCK_SIZE; i++)
    {
        pBuf[i] = i%256;
    }

    vxbTimestampEnable();

#if 0 /* write	 */
    timeStart = vxbTimestamp();
    tickStart = tickGet();

    if (altSocGen5DwMshc_BlkWrite (card, blkStart, blkNum, pBuf) == ERROR)
        {
        printf ("Write error!\n");
        cacheDmaFree (pBuf);
        return ERROR;
        }

    timeEnd = vxbTimestamp();
    tickEnd = tickGet();
    tickDiff = tickEnd - tickStart;

    if (tickDiff > 1)
        speed = (double)blkNum / 2 / 
                       ((double)tickDiff / (double)sysClkRateGet());
    else
        {
        if (tickDiff == 0)
            timeDiff = timeEnd - timeStart;
        else
            timeDiff = timeEnd + (vxbTimestampPeriod ()  - timeStart);
        speed =  (double)blkNum / 2 / 
                        ((double)timeDiff / (double)vxbTimestampFreq());
        }

    printf ("Write finished, write speed = %f KB/s.\n", speed);
#endif

#if 1 /* read */
    bzero ((char *)pBuf, blkNum * SDMMC_BLOCK_SIZE);

    timeStart = vxbTimestamp();
    tickStart = tickGet();

    if (altSocGen5DwMshc_BlkRead (card, blkStart, blkNum, pBuf) == ERROR)
    {
        printf ("Read error!\n");
        cacheDmaFree (pBuf);
        return ERROR;
    }
	else
	{
		for (i=0; i<SDMMC_BLOCK_SIZE; i++)
		{
			printf("%02X", pBuf[i]);
			
			if ((i+1)%4 == 0)
			{
				printf(" ");
			}
			
			if ((i+1)%16 == 0)
			{
				printf(" \n");
			}
		}
	}


    timeEnd = vxbTimestamp();
    tickEnd = tickGet();
    tickDiff = tickEnd - tickStart;

    if (tickDiff > 1)
        speed = (double)blkNum / 2 / 
                       ((double)tickDiff / (double)sysClkRateGet());
    else
    {
        if (tickDiff == 0)
            timeDiff = timeEnd - timeStart;
        else
            timeDiff = timeEnd + (vxbTimestampPeriod ()  - timeStart);
		
        speed = (double) blkNum / 2 / 
                        ((double)timeDiff / (double)vxbTimestampFreq());
    }

    printf ("Read finished, read speed = %f KB/s.\n", speed);
#endif

#if 0
    for (i = 0; i < blkNum * SDMMC_BLOCK_SIZE; i++)
    {
        if (pBuf[i] != i%256)
        {
            k = i / SDMMC_BLOCK_SIZE;
            printf ("r/w ERROR at %d  0x%08x %d \n", i, &pBuf[i], pBuf[i]);
            for (j = 0; j < SDMMC_BLOCK_SIZE; j++)
            {
                if (j%16 == 0)
                    printf ("\r\n");
                printf ("%x  ", pBuf[k * SDMMC_BLOCK_SIZE + j]);
            }

            printf ("\r\n");
            cacheDmaFree (pBuf);
            return ERROR;
        }
    }
#endif

    printf ("Test ok.\n");
    cacheDmaFree (pBuf);
	
    return OK;
}


#if 1

int g_test_sd2 = 0;

UINT8 g_txBuf8[512] = {0};
UINT8 g_rxBuf8[512] = {0};

STATUS test_rd_sector_vx0(UINT32 blkStart)
{
    VXB_DEVICE_ID       pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    SDMMC_CARD *        card;
    UINT32              i;
	int same_flag = 0;
	
	/*UINT8 buf8[1024] = {0};*/
	UINT8* buf8 = (UINT8*)(&g_rxBuf8[0]);	
	int unit = 0;
	
    pDev = vxbInstByNameFind (ALT_DW_MSHC_NAME, unit);
    if (pDev == NULL)
    {
        printf ("Can not find this sd unit(%d)!  \n", unit);
        return ERROR;
    }
	else
	{
		/*printf ("vxbInstByNameFind(%s): 0x%X \n", ALT_DW_MSHC_NAME, pDev);*/
	}
	
    pDrvCtrl= (ALT_MSHC_DRV_CTRL *) pDev->pDrvCtrl;
    card = &pDrvCtrl->card;
	
	printf("InstInit2->regBase: 0x%08X \n", pDrvCtrl->regBase);

	memset(buf8, 0, 512);

    /*if (altSocGen5DwMshc_BlkRead (card, blkStart, 1, buf8) == ERROR)*/
    if (altSocGen5DwMshc_BlkRead_Poll (card, blkStart, 1, buf8) == ERROR)
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

#if 1
	same_flag = 1;
	for (i=0; i<SDMMC_BLOCK_SIZE; i++)
	{
		if (g_rxBuf8[i] != g_txBuf8[i])
		{
			printf("write & read not same: 0x%02X(r) != 0x%02X(t) \n", g_rxBuf8[i], g_txBuf8[i]);
			same_flag = 0;
			break;
		}
	}
	if (same_flag == 0)
	{
		printf("== read&write sector(%d) is not same ??? == \n\n", blkStart);
	}
	else
	{
		printf("== read&write sector(%d) is same !!! == \n\n", blkStart);
	}
#endif

    return OK;
}	



STATUS test_wr_sector_vx0(UINT32 blkStart)
{
    VXB_DEVICE_ID       pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    SDMMC_CARD *        card;
    UINT32              i;
	
	/*UINT8 buf8[1024] = {0};*/
	UINT8* buf8 = (UINT8*)(&g_txBuf8[0]);
	int byteCnt = 0;
	
	int unit = 0;
	
    pDev = vxbInstByNameFind (ALT_DW_MSHC_NAME, unit);
    if (pDev == NULL)
    {
        printf ("Can not find this sd unit(%d)!  \n", unit);
        return ERROR;
    }
	else
	{
		/*printf ("vxbInstByNameFind(%s): 0x%X \n", ALT_DW_MSHC_NAME, pDev);*/
	}
	
    pDrvCtrl= (ALT_MSHC_DRV_CTRL *) pDev->pDrvCtrl;
    card = &pDrvCtrl->card;
	
	/*printf("InstInit2->regBase: 0x%08X \n", pDrvCtrl->regBase);*/

	/*////////////////////////////////*/
	for (i=0; i<SDMMC_BLOCK_SIZE; i++)
	{
		buf8[i] = i + g_test_sd2;
	}
	g_test_sd2++;

	
	/* head*/
	*((UINT32*)(&buf8[0])) = 0xabcd1234;
	*((UINT32*)(&buf8[4])) = blkStart;

	/* tail*/
	byteCnt = 512;
	*((UINT32*)(&buf8[byteCnt-8])) = blkStart;
	*((UINT32*)(&buf8[byteCnt-4])) = 0xcdef6789;
	

    /*if (altSocGen5DwMshc_BlkWrite(card, blkStart, 1, buf8) == ERROR)*/
    if (altSocGen5DwMshc_BlkWrite_Poll(card, blkStart, 1, buf8) == ERROR)
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

#if 1

void test_sd_rd_sect2(int sect_idx)
{
    VXB_DEVICE_ID       pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    SDMMC_CARD *        card;
    UINT32              i;
	
	UINT8* buf8 = (UINT8*)(&g_rxBuf8[0]);	
	int unit = 0;
	
    pDev = vxbInstByNameFind (ALT_DW_MSHC_NAME, unit);
    if (pDev == NULL)
    {
        printf ("Can not find this sd unit(%d)!  \n", unit);
        return;
    }
	
    pDrvCtrl= (ALT_MSHC_DRV_CTRL *) pDev->pDrvCtrl;
    card = &pDrvCtrl->card;	

	memset(buf8, 0, 512);
    if (altSocGen5DwMshc_BlkRead_Poll (card, sect_idx, 1, buf8) == ERROR)
    {
		printf("---emmc read sect(%d) fail!--- \n", sect_idx);
        return;
    }
	else
	{
		printf("--emmc(%d)_read(%d)--\n", unit, sect_idx);		
	}

	return;
}

void test_sd_wr_sect2(int sect_idx)
{
    VXB_DEVICE_ID       pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    SDMMC_CARD *        card;
    UINT32              i;
	
	/*UINT8 buf8[1024] = {0};*/
	UINT8* buf8 = (UINT8*)(&g_txBuf8[0]);
	int byteCnt = 0;
	
	int unit = 0;
	
    pDev = vxbInstByNameFind (ALT_DW_MSHC_NAME, unit);
    if (pDev == NULL)
    {
        printf ("Can not find this sd unit(%d)!  \n", unit);
        return ERROR;
    }
	else
	{
		/*printf ("vxbInstByNameFind(%s): 0x%X \n", ALT_DW_MSHC_NAME, pDev);*/
	}
	
    pDrvCtrl= (ALT_MSHC_DRV_CTRL *) pDev->pDrvCtrl;
    card = &pDrvCtrl->card;
	
	/*printf("InstInit2->regBase: 0x%08X \n", pDrvCtrl->regBase);*/

	/*////////////////////////////////*/
	for (i=0; i<SDMMC_BLOCK_SIZE; i++)
	{
		buf8[i] = i + g_test_sd2;
	}
	g_test_sd2++;

	
	/* head*/
	*((UINT32*)(&buf8[0])) = 0xabcd1234;
	*((UINT32*)(&buf8[4])) = sect_idx;

	/* tail*/
	byteCnt = 512;
	*((UINT32*)(&buf8[byteCnt-8])) = sect_idx;
	*((UINT32*)(&buf8[byteCnt-4])) = 0xcdef6789;

    if (altSocGen5DwMshc_BlkWrite_Poll(card, sect_idx, 1, buf8) == ERROR)
    {
		printf("\n---emmc write sect(%d) fail!--- \n", sect_idx);
        return;
    }
	else
	{
		printf("\n-emmc write sect(%d) ok!- \n", sect_idx);
	}
	
	return;
}

void test_sd_rdwr_sects(int from_sect, int to_sect)
{
	int idx = 0, j = 0;
	int sect_diff = 0;

	for (idx = from_sect; idx <= to_sect; idx++)
	{
		test_sd_wr_sect2(idx);
		taskDelay(1);
		
		test_sd_rd_sect2(idx);
		taskDelay(1);

		sect_diff = 0;
		for (j=0; j<512; j++)
		{
			if (g_rxBuf8[j] != g_txBuf8[j])
			{
				sect_diff = 1;
				break;
			}
		}

		if (sect_diff == 1)
		{
			printf("------sect(%d) wr & rd err:[%d][0x%X-0x%X]!------- \n\n", \
				    idx, j, g_rxBuf8[j], g_txBuf8[j]);
		}
		else
		{
			printf("====sect(%d) wr & rd OK!==== \n\n", idx);
		}
	}

	printf("\n\n test emmc sectors:[%d -> %d] finish! \n\n\n", from_sect, to_sect);
	
	return;
}

#endif


#if 1
int g_test_sd3 = 0;

STATUS test_rd_sector_vx1(UINT32 blkStart)
{
    VXB_DEVICE_ID       pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    SDMMC_CARD *        card;
    UINT32              i;
	UINT8 buf8[1024] = {0};
	
	int unit = 1;
	
    pDev = vxbInstByNameFind (ALT_DW_MSHC_NAME, unit);
    if (pDev == NULL)
    {
        printf ("Can not find this sd unit(%d)!  \n", unit);
        return ERROR;
    }
	else
	{
		/*printf ("vxbInstByNameFind(%s): 0x%X \n", ALT_DW_MSHC_NAME, pDev);*/
	}

	
    pDrvCtrl= (ALT_MSHC_DRV_CTRL *) pDev->pDrvCtrl;
    card = &pDrvCtrl->card;

	
	printf("InstInit2->regBase: 0x%08X \n", pDrvCtrl->regBase);

    /*if (altSocGen5DwMshc_BlkRead (card, blkStart, 1, buf8) == ERROR)*/
    if (altSocGen5DwMshc_BlkRead_Poll (card, blkStart, 1, buf8) == ERROR)
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


STATUS test_wr_sector_vx1(UINT32 blkStart)
{
    VXB_DEVICE_ID       pDev;
    ALT_MSHC_DRV_CTRL * pDrvCtrl;
    SDMMC_CARD *        card;
    UINT32              i;
	UINT8 buf8[1024] = {0};
	
	int unit = 1;
	
    pDev = vxbInstByNameFind (ALT_DW_MSHC_NAME, unit);
    if (pDev == NULL)
    {
        printf ("Can not find this sd unit(%d)!  \n", unit);
        return ERROR;
    }
	else
	{
		/*printf ("vxbInstByNameFind(%s): 0x%X \n", ALT_DW_MSHC_NAME, pDev);*/
	}
	
    pDrvCtrl= (ALT_MSHC_DRV_CTRL *) pDev->pDrvCtrl;
    card = &pDrvCtrl->card;
	
	/*printf("InstInit2->regBase: 0x%08X \n", pDrvCtrl->regBase);*/

	/*////////////////////////////////*/
	for (i=0; i<SDMMC_BLOCK_SIZE; i++)
	{
		buf8[i] = i + g_test_sd3;
	}
	g_test_sd3++;

   /* if (altSocGen5DwMshc_BlkWrite(card, blkStart, 1, buf8) == ERROR)*/
    if (altSocGen5DwMshc_BlkWrite_Poll(card, blkStart, 1, buf8) == ERROR)
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

#endif /* ALT_DW_MSHC_DBG_ON */

