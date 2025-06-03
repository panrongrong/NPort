/* vxbFmshSdhcCtrl.c - SDHC host controller common driver library ref: vxbSdhcCtrl.c*/

/*
 * Copyright (c) 2012 - 2013, 2015-2018 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01m,03sep18,d_l  update SDHC_DBG function name (VXW6-86922)
01l,12jul18,j_x  masking member busWidth of structure SD_HOST_SPEC for binary 
                 compatible (VXW6-86766)
01k,19sep17,d_l  add 10bit clock mode support. (VXW6-86304)
01j,14jun17,ffu  clear warnings (VXW6-85661)
01i,01mar17,d_l  add bus-width selection. (VXW6-86099)
                 fix forward null issue.
01h,26oct16,y_y  reduece data timeout for commands with busy signal. (VXW6-85760)
01g,25jan16,myt  add support for direct bio for SD/eMMC (VXW6-85000)
01f,26oct16,myt  add support of reliable write for eMMC (VXW6-84383)
01e,07jul15,j_x  add vxbDmaBufMapSync before and after DMA operation (VXW6-84648)
01d,06jan15,myt  fix the error of DMA address update (VXW6-83697)
01c,20nov13,e_d  remove APIs wrongly published. (WIND00444661)
01b,29oct13,e_d  fix prevent issue. (WIND00440964)
01a,28aug12,e_d  written.
*/

/*
DESCRIPTION

This is the vxbus compliant Secure Digital Host Controller (SDHC)
driver library which implements the functionality specified in
"SD Specifications Part A2 SD Host Controller Simplified Specification
Version 2.00"

The SDHC provides an interface between the host controller and
SD/SDIO target device.

This file is not one vxbus driver's file. And it implements some routines as
stand SDHC controller. But for a lot of SDHC controllers, they have a little
different with stand SDHC controller. If write one new SDHC controller driver,
we only write some routines that different with stand SDHC. Others we can call
stand routine in this file.

SEE ALSO: vxBus, xbd
\tb "SD Specifications Part A1 Physical Layer Simplified Specification Version 2.00"
\tb "SD Specifications Part A2 SD Host Controller Simplified Specification Version 2.00"
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
#include <hwif/vxbus/vxBus.h>
#include <hwif/vxbus/hwConf.h>
#include <hwif/util/vxbParamSys.h>

#include <hwif/vxbus/vxbPciLib.h>
#include <hwif/vxbus/hwConf.h>
#include <drv/pci/pciConfigLib.h>
#include <drv/pci/pciIntLib.h>

#include <hwif/vxbus/vxbSdLib.h>
#include <../src/hwif/h/vxbus/vxbAccess.h>
#include <../src/hwif/h/vxbus/vxbPciBus.h>

#include "vxbFmshSdhcCtrl.h"


/**/
/* jc: define for normal_speed; undef for rd&wr high_speed*/
/**/
#define  SDMMC_NORMAL_SPEED


/* defines */
#define ALT_DW_MSHC_FS_UNCACHE_BUFFER

#define SDHC_DBG_ON
#ifdef  SDHC_DBG_ON

#ifdef  LOCAL
#undef  LOCAL
#define LOCAL
#endif

#define SDHC_DBG_IRQ            0x00000001
#define SDHC_DBG_RW             0x00000002
#define SDHC_DBG_XBD            0x00000004
#define SDHC_DBG_ERR            0x00000008
#define  SDHC_DBG_CMD            0x00000010
#define SDHC_DBG_ALL            0xffffffff
#define SDHC_DBG_OFF            0x00000000

UINT32 sdhcDbgMask = SDHC_DBG_ERR;

IMPORT FUNCPTR _func_logMsg;

#define SDHC_DBG(mask, string, a, b, c, d, e, f)        \
    if ((sdhcDbgMask & mask) || (mask == SDHC_DBG_ALL))  \
        if (_func_logMsg != NULL) \
           (* _func_logMsg)(string, a, b, c, d, e, f)
#else
#define SDHC_DBG(mask, string, a, b, c, d, e, f)
#endif  /* SDHC_DBG_ON */

/*
 * File system use cacheDmaMalloc to alloc buffer (file bio.c, bio_alloc() routine),
 * so there is no need for driver to use cacheFlush and cacheInvalidate to keep buffer's
 * coherence. When define SDHC_FS_UNCACHE_BUFFER in this driver, we will ignore the
 * the sync operaton of DMA buffer.
 */

/*#define SDHC_FS_UNCACHE_BUFFER*/
#define ALT_DW_MSHC_FS_UNCACHE_BUFFER

/* register low level access routines */



#ifdef ARMBE8
#    define SWAP32 vxbSwap32
#else
#    define SWAP32 
#endif /* ARMBE8 */
/* externs */

IMPORT UCHAR erfLibInitialized;
IMPORT void vxbUsDelay (int);
IMPORT void vxbMsDelay (int);

BOOL mmcHrfsInclude = FALSE; 


LOCAL STATUS fmshSdhcCtrl_PollIntsts( VXB_DEVICE_ID  pDev,UINT32 mask);
LOCAL void fmshSdhcCtrl_InstInit( VXB_DEVICE_ID pInst);
LOCAL void fmshSdhcCtrl_InstInit2( VXB_DEVICE_ID pInst);
LOCAL STATUS fmshSdhcCtrl_InstConnect( VXB_DEVICE_ID pInst);
STATUS fmshSdhcCtrl_SpecInfoGet(VXB_DEVICE_ID pDev,void ** pHostSpec, VXB_SD_CMDISSUE_FUNC * pCmdIssue);

void fmshSdhcCtrl_BusWidthSetup(VXB_DEVICE_ID pDev,UINT32 width);
BOOL fmshSdhcCtrl_CardWpCheck(VXB_DEVICE_ID pDev);
BOOL fmshSdhcCtrl_CardInsertSts(VXB_DEVICE_ID pDev);
STATUS fmshSdhcCtrl_CmdPrepare(VXB_DEVICE_ID pDev,SD_CMD * pSdCmd);
LOCAL STATUS fmshSdhcCtrl_Isr(VXB_DEVICE_ID pDev);
STATUS fmshSdhcCtrl_Init(VXB_DEVICE_ID pInst);
STATUS fmshSdhc_VddSetup(VXB_DEVICE_ID pDev, UINT32 vdd);
LOCAL void fmshSdhcCtrl_ReadFifo(VXB_DEVICE_ID pDev);
LOCAL void fmshSdhcCtrl_WriteFifo(VXB_DEVICE_ID pDev);

LOCAL DRIVER_INITIALIZATION fmshSdhcFuncs =
    {
    		fmshSdhcCtrl_InstInit,        /* devInstanceInit */
    		fmshSdhcCtrl_InstInit2,       /* devInstanceInit2 */
    		fmshSdhcCtrl_InstConnect      /* devConnect */
    };

LOCAL device_method_t vxbFmshSdhcCtrl_methods[] =
    {
    DEVMETHOD (vxbSdSpecInfoGet, fmshSdhcCtrl_SpecInfoGet),
    DEVMETHOD_END
    };

LOCAL DRIVER_REGISTRATION fmshSdhcPlbRegistration =
    {
    NULL,
    VXB_DEVID_DEVICE,                                   /* devID */
    VXB_BUSID_PLB,                                      /* busID = PLB */
    VXB_VER_5_0_0,                                      /* vxbVersion */
    FMSH_SDHC_NAME,                                     /* drvName */
    &fmshSdhcFuncs,                                     /* pDrvBusFuncs */
    vxbFmshSdhcCtrl_methods,                            /* pMethods */
    NULL,                                               /* devProbe */
    NULL                                                /* pParamDefaults */
    };

IMPORT UCHAR erfLibInitialized;

#if 1

void fmshSdhcRegister (void)
    {
    (void)vxbDevRegister (&fmshSdhcPlbRegistration);
    }

/*******************************************************************************
*
* sdhcInstInit - first level initialization routine of sdhc device
*
* This routine performs the first level initialization of the sdhc device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL void fmshSdhcCtrl_InstInit
    (
    VXB_DEVICE_ID pInst
    )
{
    /* get the next available unit number */
	logMsg("fmshSdhcCtrl_InstInit\n",1,2,3,4,5,6);
    if (pInst->busID == VXB_BUSID_PCI)
        vxbNextUnitGet (pInst);

	return;
}

LOCAL void fmshSdhcCtrl_InstInit2
    (
    VXB_DEVICE_ID pInst
    )
    {
    FMSH_SDHC_DRV_CTRL * pDrvCtrl;
    struct hcfDevice * pHcf;
    FUNCPTR clkFunc = NULL;
    UINT16 i;
    int flag;
    bus_size_t alignment;

    pDrvCtrl = (FMSH_SDHC_DRV_CTRL *)malloc (sizeof(FMSH_SDHC_DRV_CTRL));
    if (pDrvCtrl == NULL)
        return;

    bzero ((char *)pDrvCtrl, sizeof(FMSH_SDHC_DRV_CTRL));

    pDrvCtrl->sdHostCtrl.pDev = pInst;
    pDrvCtrl->sdHostCtrl.attached = FALSE;
    pDrvCtrl->sdHostCtrl.isInsert = FALSE;
    pInst->pDrvCtrl = pDrvCtrl;

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
    vxbRegMap (pInst, i, &pDrvCtrl->regHandle);

    pHcf = (struct hcfDevice *)hcfDeviceGet (pInst);

    if (pHcf != NULL)
        {

        /*
         * resourceDesc {
         * The clkFreq resource specifies clock
         * frequency of SDHC base clock. }
         */

        /* Need not check return status at here */

        (void)devResourceGet (pHcf, "clkFreqSetup", HCF_RES_ADDR, (void *)&clkFunc);
        if (clkFunc)
        {
			/*logMsg("clkFunc = %x\n",&clkFunc,2,3,4,5,6);*/
            pDrvCtrl->sdHostCtrl.curClkFreq = (*clkFunc) ();
        }

        /*
         * resourceDesc {
         * The dmaMode resource specifies the DMA
         * mode of eSDHC. If this property is not
         * explicitly specified, the driver uses
         * SDMA by default. }
         */

        /* Need not check return status at here */

        (void)devResourceGet (pHcf, "dmaMode", HCF_RES_INT,
                             (void *)&(pDrvCtrl->sdHostCtrl.dmaMode));

        /*
         * resourceDesc {
         * The polling resource specifies whether
         * the driver uses polling mode or not.
         * If this property is not explicitly
         * specified, the driver uses interrupt
         * by default. }
         */

        /* Need not check return status at here */

        (void)devResourceGet (pHcf, "polling", HCF_RES_INT,
                             (void *)&(pDrvCtrl->sdHostCtrl.polling));

        /*
         * resourceDesc {
         * The flags resource specifies various
         * controll flags of the host controller. }
         */

        /* Need not check return status at here */

        (void)devResourceGet (pHcf, "flags", HCF_RES_INT,
                             (void *)&(pDrvCtrl->flags));
      /*  logMsg("pDrvCtrl->flags = %x\n",pDrvCtrl->flags,2,3,4,5,6);*/
        
        
        
        
        
        /*
         * resourceDesc {
         * The directBio resource specifies if the direct bio mode 
         * is supported by the driver. }
         */
        (void) devResourceGet (pHcf, "clkFreqSetup", HCF_RES_ADDR,
                                     (void *)&pDrvCtrl->clkFreqSetup);
        
        
   
            
#ifdef ALT_DW_MSHC_FS_UNCACHE_BUFFER
    pDrvCtrl->flags |= ALT_MSHC_FLAGS_SNOOP_ENABLED;
#endif /* ALT_DW_MSHC_FS_UNCACHE_BUFFER */            

#ifdef SD_BUSWIDTH
        /* 
         * All soc and eMMC devices support 4 bit width. And some of them,
         * such as Zynq 7k SOC, only supports 4 bit width.  Therefore
         * if BSP doesn't give us a bus-width, set it to 4 bit width 
         */

        /*
         * resourceDesc {
         * This resource specifies bit width used by eMMC devices. Host controller 
         * selects maximum bus-width (4bit or 8bit) it supported for eMMC device. 
         * This parameter doesn't influence SD cards. }
         */

        if (devResourceGet (pHcf, "bus-width", HCF_RES_INT,
                             (void *)
                             &(pDrvCtrl->sdHostCtrl.sdHostSpec.busWidth)) != OK)
            {
            pDrvCtrl->sdHostCtrl.sdHostSpec.busWidth = 4;
            }
 #endif
          }

    /* initialize SDIO configuration library */
    pDrvCtrl->pIntInfo = (struct vxbSdioInt *)malloc(sizeof(struct vxbSdioInt));
    if(pDrvCtrl->pIntInfo == NULL)
        return;

    vxbSdioIntLibInit(pDrvCtrl->pIntInfo);
	
#if 0
    if (pDrvCtrl->flags & SDHC_FIFO_ENDIANESS_REVERSE)
       {

        /*
         * If endianess of internal buffer data port is differernt
         * from CPU's endiness, we swap the handle.
         */

        pDrvCtrl->pioHandle = (void *)
                               (VXB_HANDLE_SWAP ((ULONG)pDrvCtrl->regHandle));
        }
#endif

    pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdBusWidthSetup = fmshSdhcCtrl_BusWidthSetup;
    pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdCardWpCheck   = fmshSdhcCtrl_CardWpCheck;
    pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdCardInsertSts = fmshSdhcCtrl_CardInsertSts;
    pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdCmdPrepare    = fmshSdhcCtrl_CmdPrepare;
    pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdHostCtrlInit  = fmshSdhcCtrl_Init;
    pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdClkFreqSetup  = fmshSdhc_ClkFreqSetup;
    pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdIsr           = fmshSdhcCtrl_Isr;
	
    pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdVddSetup      = NULL;
    /*pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdVddSetup      = fmshSdhc_VddSetup;*/
	
    pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdResumeSet     = NULL;
    
    /* which line is the point ? */
    pDrvCtrl->sdHostCtrl.sdHostSpec.vxbSdBusWidthSetup = pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdBusWidthSetup;
    pDrvCtrl->sdHostCtrl.sdHostSpec.vxbSdCardWpCheck   = pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdCardWpCheck;
	
    pDrvCtrl->sdHostCtrl.sdHostSpec.vxbSdClkFreqSetup  = pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdClkFreqSetup;
	
    pDrvCtrl->sdHostCtrl.sdHostSpec.vxbSdResumeSet     = pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdResumeSet;
    pDrvCtrl->sdHostCtrl.sdHostSpec.vxbSdVddSetup      =  pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdVddSetup;
    pDrvCtrl->sdHostCtrl.sdHostSpec.capbility          = pDrvCtrl->sdHostCtrl.capbility;
    pDrvCtrl->sdHostCtrl.sdHostSpec.directBio          = pDrvCtrl->sdHostCtrl.directBio;

    /* Need not check return status at here */
    (void)vxbBusAnnounce (pInst, VXB_BUSID_SD);
    }
/*******************************************************************************
*
* fmshSdhcCtrl_Monitor -  SDHC insert status monitor
*
* This function will check zynq SDHC insert/remove status. If target device
* insert status is TRUE, the function will run sdioBusAnnounceDevices to add
* target device to vxbus system. If FLASE, will run vxbDevRemovalAnnounce to
* remove the target device from vxbus system.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
extern void sdBusAnnounceDevices_2	(VXB_DEVICE_ID pDev,void * pCookie);

LOCAL void fmshSdhcCtrl_Monitor
    (
    VXB_DEVICE_ID pDev
    )
    {
    STATUS rc;
    int i = 0;
    VXB_DEVICE_ID pDevList;
    SD_HOST_CTRL * pSdHostCtrl;

    pSdHostCtrl = (SD_HOST_CTRL *)(pDev->pDrvCtrl);
    if (pSdHostCtrl == NULL)
        return;

    while (erfLibInitialized == FALSE)
        taskDelay (sysClkRateGet ());

    while (1)
    {    
        rc = pSdHostCtrl->sdHostOps.vxbSdCardInsertSts(pDev);
		/* jc*/
		/*printf("\n ---- vxbSdCardInsertSts: rc=%d, sd_%d ---- \n", rc, pDev->unitNumber);*/
        if (rc)
        {
            if (pSdHostCtrl->attached == TRUE)
            {
                printf("shoulb not be happen\n");
                continue;
            }
			
            if (pSdHostCtrl->sdHostOps.vxbSdVddSetup != NULL)
                pSdHostCtrl->sdHostOps.vxbSdVddSetup (pDev, pSdHostCtrl->vdd);
			
            if (pSdHostCtrl->sdHostOps.vxbSdClkFreqSetup != NULL)
                pSdHostCtrl->sdHostOps.vxbSdClkFreqSetup(pDev,SDMMC_CLK_FREQ_400KHZ);
			
            if (pSdHostCtrl->sdHostOps.vxbSdBusWidthSetup != NULL)
                pSdHostCtrl->sdHostOps.vxbSdBusWidthSetup (pDev, SDMMC_BUS_WIDTH_1BIT);

            /* Need not check return status at here */
			printf("\n ==== sdBusAnnounceDevices: rc=%d, sd_ctrl_[%d] ==== \n", rc, pDev->unitNumber);

		#if 1  /* usr_define lib*/
            (void)sdBusAnnounceDevices_2(pDev, NULL);  /* usr-define interface, for ruineng?*/
            
        #else  /* vx_sys_lib*/
			(void)sdBusAnnounceDevices(pDev, NULL);  /* sd/mmc verify_board*/
		#endif
		
            pSdHostCtrl->attached = TRUE;

			/* jc*/
			/*pSdHostCtrl->sdHostOps.vxbSdBusWidthSetup (pDev, SDMMC_BUS_WIDTH_4BIT);*/
			/*pSdHostCtrl->sdHostOps.vxbSdClkFreqSetup(pDev, SDMMC_CLK_50_MHZ);*/
        }
        else
        {
            if (pSdHostCtrl->attached == FALSE)
                continue;
			
            pSdHostCtrl->attached = FALSE;
        
            pDevList = pDev->u.pSubordinateBus->instList;
            for(i = 0; i < MAX_TARGET_DEV; i++)
                {
                if (pDevList != NULL)
                    {

                    /* Need not check return status at here */

                    (void)vxbDevRemovalAnnounce(pDevList);
                    pDevList = pDevList->pNext;
                    }
                else
                    break;
                }
            pSdHostCtrl->attached = FALSE;
        }
		
	#ifdef SDMMC_NORMAL_SPEED	
        taskDelay (2 * sysClkRateGet());
    #else
		taskDelay (2*10);  /* jc*/
	#endif
    }

	return;
}

/*******************************************************************************
*
* sdhcInstConnect - third level initialization routine of sdhc device
*
* This routine performs the third level initialization of the sdhc device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS fmshSdhcCtrl_InstConnect
    (
    		VXB_DEVICE_ID pInst
    
    )
    {
    STATUS rc;
    UINT32 val;
    
    FMSH_SDHC_DRV_CTRL * pDrvCtrl =(FMSH_SDHC_DRV_CTRL *)pInst->pDrvCtrl;
    SD_HOST_CTRL * pSdHostCtrl;
    TASK_ID monTaskId; 
    if (pDrvCtrl == NULL)
        return ERROR;
    else
        pSdHostCtrl = (SD_HOST_CTRL *) pDrvCtrl;

    /*
     * The devChange semaphore is used by the interrupt service routine
     * to inform the card monitor task that a state change has occurred.
     */

    pSdHostCtrl->devChange = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
    if (pSdHostCtrl->devChange == NULL)
        {
        SDHC_DBG (SDHC_DBG_ERR, "semBCreate failed for devChange\n",
                      0, 0, 0, 0, 0, 0);
        return ERROR;
        }

    pSdHostCtrl->cmdDone = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
    if (pSdHostCtrl->cmdDone == NULL)
        {
        SDHC_DBG (SDHC_DBG_ERR, "semBCreate failed for cmdDone\n",
                      0, 0, 0, 0, 0, 0);
        goto err;
        }

    pSdHostCtrl->dataDone = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
    if (pSdHostCtrl->dataDone == NULL)
        {
        SDHC_DBG (SDHC_DBG_ERR, "semBCreate failed for dataDone\n",
                      0, 0, 0, 0, 0, 0);
        goto err;
        }

    pSdHostCtrl->pioReady = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
    if (pSdHostCtrl->pioReady == NULL)
        {
        SDHC_DBG (SDHC_DBG_ERR, "semBCreate failed for pioReady\n",
                      0, 0, 0, 0, 0, 0);
        goto err;
        }

    pSdHostCtrl->hostDevSem = semMCreate(SEM_Q_PRIORITY |
                                         SEM_DELETE_SAFE |
                                         SEM_INVERSION_SAFE);
    if (pSdHostCtrl->hostDevSem == NULL)
        {
        SDHC_DBG (SDHC_DBG_ERR, "semBCreate failed for pioReady\n",
                      0, 0, 0, 0, 0, 0);
        goto err;
        }

    /* per-device init */

    if (pSdHostCtrl->sdHostOps.vxbSdHostCtrlInit != NULL)
        {
        rc = pSdHostCtrl->sdHostOps.vxbSdHostCtrlInit(pSdHostCtrl->pDev);
        if (rc == ERROR)
            goto err;
        }
    
    /*syx*/
    monTaskId = taskSpawn ("sdBusMonitor", 100, 0,
                               8192, (FUNCPTR)fmshSdhcCtrl_Monitor,
                               (_Vx_usr_arg_t)pInst,
                               0, 0, 0, 0, 0, 0, 0, 0, 0);
    
	/* setup the interrupt mask */ 

	pDrvCtrl->intMask = ALT_MSHC_INT_PIO_DEFAULTS;

	CSR_WRITE_4 (pInst, ALT_MSHC_INTMASK, pDrvCtrl->intMask);

	if (!pDrvCtrl->sdHostCtrl.polling)  /* irq_mode*/
	{
		/* connect and enable interrupt */
		/*logMsg("intconnect#######################\n",1,2,3,4,5,6);*/
		vxbIntConnect (pInst, 0, fmshSdhcCtrl_Isr, pInst);
		vxbIntEnable (pInst, 0, fmshSdhcCtrl_Isr, pInst);

#if 0  /* ref vx_irq_call*/
		ret = intConnect (INUM_TO_IVEC (pGpioBank->vec),  sysGpioIsr, (int)pGpioBank);
		if (ret != OK)
			return ret;
		
		ret = intEnable (pGpioBank->vec);
		if (ret != OK)
			return ret;
#endif

		/* enable interrupts */
		CSR_SETBIT_4 (pInst, ALT_MSHC_CTRL, ALT_MSHC_CTRL_INT_ENABLE);

		/* card always present, notify monitor task */
		if ((pDrvCtrl->flags & ALT_MSHC_FLAGS_CARD_PRESENT) != 0)
		{
			/* printf("semGive (pSdHostCtrl->devChange);\n");*/
			semGive (pSdHostCtrl->devChange);
		}
	}

    return OK;

err:
    if (pSdHostCtrl->dataDone != NULL)
        {
        if (semDelete(pSdHostCtrl->dataDone) != OK)
            SDHC_DBG (SDHC_DBG_ERR, "semDelete failed for dateDone\n",
                      0, 0, 0, 0, 0, 0);
        }

    if (pSdHostCtrl->cmdDone != NULL)
        {
        if (semDelete(pSdHostCtrl->cmdDone) != OK)
            SDHC_DBG (SDHC_DBG_ERR, "semDelete failed for cmdDone\n",
                      0, 0, 0, 0, 0, 0);
        }
    if (pSdHostCtrl->devChange != NULL)
        {
        if (semDelete(pSdHostCtrl->devChange) != OK)
            SDHC_DBG (SDHC_DBG_ERR, "semDelete failed for devChange\n",
                      0, 0, 0, 0, 0, 0);
        }
    if (pSdHostCtrl->pioReady != NULL)
        {
        if (semDelete(pSdHostCtrl->pioReady) != OK)
            SDHC_DBG (SDHC_DBG_ERR, "semDelete failed for devChange\n",
                      0, 0, 0, 0, 0, 0);
        }
    if (pSdHostCtrl->hostDevSem != NULL)
        {
        if (semDelete(pSdHostCtrl->hostDevSem) != OK)
            SDHC_DBG (SDHC_DBG_ERR, "semDelete failed for devChange\n",
                      0, 0, 0, 0, 0, 0);
        }
    return ERROR;
}

#endif

#if 1
/*******************************************************************************
*
* sdhcCtrlInit - eSDHC per device specific initialization
*
* This routine performs per device specific initialization of eSDHC.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS fmshSdhcCtrl_Init
    (
    VXB_DEVICE_ID pInst
    )
{
    FMSH_SDHC_DRV_CTRL * pDrvCtrl = (FMSH_SDHC_DRV_CTRL *)pInst->pDrvCtrl;
    UINT32 hostCap;

    /* reset eSDHC controller */

    if (pDrvCtrl == NULL)
        return ERROR;
    CSR_WRITE_4 (pInst, ALT_MSHC_CTRL, (ALT_MSHC_CTRL_RESET | 
                 ALT_MSHC_CTRL_FIFO_RESET | ALT_MSHC_CTRL_DMA_RESET));

    while ((CSR_READ_4 (pInst, ALT_MSHC_CTRL) & 
            (ALT_MSHC_CTRL_RESET | ALT_MSHC_CTRL_FIFO_RESET | 
            ALT_MSHC_CTRL_DMA_RESET)) != 0) ;
	
    /* clear interrupts */
    CSR_WRITE_4(pInst, ALT_MSHC_RINTSTS, 0xFFFFFFFF);
    CSR_WRITE_4(pInst, ALT_MSHC_INTMASK, 0);

    CSR_WRITE_4(pInst, ALT_MSHC_TMOUT, ALT_MSHC_DEFAULT_TIMEOUT);
    CSR_WRITE_4(pInst,  ALT_MSHC_FIFOTH, ALT_MSHC_DEFAULT_FIFO_THRESH);
    
    /*CSR_WRITE_4(pInst,  ALT_MSHC_CLKENA, 0);*/
    /*CSR_WRITE_4(pInst,  ALT_MSHC_CLKSRC, 0);*/

    /* setup data transfer width to 1-bit */
    if (pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdBusWidthSetup != NULL)
        pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdBusWidthSetup(pInst, SDMMC_BUS_WIDTH_1BIT);
    
    /* voltage supported */  
    /*pDrvCtrl->sdHostCtrl.capbility |= (OCR_VDD_VOL_32_33 | OCR_VDD_VOL_33_34);  // 3.3V sd/emmc*/
    pDrvCtrl->sdHostCtrl.capbility |= (OCR_VDD_VOL_32_33 | OCR_VDD_VOL_33_34 | OCD_VDD_VOL_LOW);  /* 1.8V  sd/emmc*/

    /* high capacity SD card is supported */
    pDrvCtrl->sdHostCtrl.capbility |= OCR_CARD_CAP_STS;

    /* determine high speed SD card supported */
    pDrvCtrl->sdHostCtrl.sdHostSpec.capbility = pDrvCtrl->sdHostCtrl.capbility;

    pDrvCtrl->highSpeed = TRUE;
    /*pDrvCtrl->highSpeed = FALSE;  // for ruineng*/

    return OK;
}

/*******************************************************************************
*
* sdhcIsr - interrupt service routine
*
* This routine handles interrupts of eSDHC.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS fmshSdhcCtrl_Isr
    (
    VXB_DEVICE_ID pDev
    )
{
	FMSH_SDHC_DRV_CTRL * pDrvCtrl = (FMSH_SDHC_DRV_CTRL *)pDev->pDrvCtrl;

	UINT32              mintSts;
	UINT32              cmdErr = 0;
	UINT32              idSts = 0;
	BOOL                needRead  = FALSE;
	BOOL                needWrite = FALSE;

	mintSts = CSR_READ_4 (pDev, ALT_MSHC_RINTSTS);
	/*   logMsg ("MINTSTS=0x%x  cmd:%d \n", mintSts, pDrvCtrl->pSdCmdLast.cmdIdx, 3, 4, 5, 6);*/

	if ((mintSts == 0) && (idSts == 0))
	   return;

	if ((mintSts & ALT_MSHC_INT_RTO) != 0) 
	{
	  SDHC_DBG (SDHC_DBG_IRQ, "CMD%d command timeout\n",
	       		   pDrvCtrl->pSdCmdLast.cmdIdx, 2, 3, 4, 5, 6);
	  cmdErr |= SDMMC_CMD_ERR_TIMEOUT;
	  pDrvCtrl->pSdCmdLast.cmdErr |= SDMMC_CMD_ERR_TIMEOUT;                 
	          
	}
	
	if ((mintSts & ALT_MSHC_INT_CMD_ERROR) != 0) 
	{
	   SDHC_DBG (SDHC_DBG_IRQ, "CMD%d command error\n",
			   pDrvCtrl->pSdCmdLast.cmdIdx, 2, 3, 4, 5, 6);
	   cmdErr |= SDMMC_CMD_ERR_PROTOCOL;
	   pDrvCtrl->pSdCmdLast.cmdErr |= SDMMC_CMD_ERR_PROTOCOL;
	}

	if ((mintSts & ALT_MSHC_INT_DATA_ERROR) != 0) 
	   {
	   SDHC_DBG (SDHC_DBG_IRQ, "CMD%d data error %x\n",
			   pDrvCtrl->pSdCmdLast.cmdIdx, mintSts, 3, 4, 5, 6);
	   cmdErr |= SDMMC_DATA_ERR_PROTOCOL;
	   pDrvCtrl->pSdCmdLast.cmdErr |= SDMMC_CMD_ERR_PROTOCOL;
	   }

	if ((mintSts & ALT_MSHC_INT_CD) != 0) 
	   {
	   SDHC_DBG (SDHC_DBG_IRQ, "Card hot plug\n",
	                    1, 2, 3, 4, 5, 6);
	   semGive (pDrvCtrl->sdHostCtrl.devChange);
	   }
	
	if( pDrvCtrl->sdHostCtrl.dmaMode == SDHC_DMA_MODE_PIO)
	/* if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_PIO)*/
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
	       if (pDrvCtrl->pSdCmdLast.cmdData.isRead)
	           needRead = TRUE;
	       else
	           needWrite = TRUE;
	       }
	   }

	if ((mintSts & ALT_MSHC_INT_DTO) != 0)
	   {
	   /* read remaining data for DTO */
	   if (pDrvCtrl->sdHostCtrl.dmaMode == SDHC_DMA_MODE_PIO)
	       {
	       if (pDrvCtrl->pSdCmdLast.cmdData.isRead)
	           {
	           needRead = TRUE;
	           }
	       
	       /* data done for PIO read and write */
	       /*logMsg("mintSts:%x  ALT_MSHC_INT_DTO\n",mintSts,2,3,4,5,6);*/
	       
	       semGive (pDrvCtrl->sdHostCtrl.dataDone);
	       }
	   }

	if ((mintSts & ALT_MSHC_INT_CMD) != 0)
	   {
	   SDHC_DBG (SDHC_DBG_IRQ, "CMD%d command complete count=%d\n",
			   pDrvCtrl->pSdCmdLast.cmdIdx, pDrvCtrl->count++, 0, 0, 0, 0);
	  
	     semGive (pDrvCtrl->sdHostCtrl.cmdDone);
	   }

	/* level sensitive, handle fifo before clear interrupts */
	if (needRead)
	   {
	   fmshSdhcCtrl_ReadFifo(pDev);
	  
	   }
	if (needWrite)
	   {
	   fmshSdhcCtrl_WriteFifo(pDev);
	   }
	CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, mintSts);

#if 0 /*unsupport DMA interrupts */
	if (pDrvCtrl->rwMode == ALT_DW_MSHC_RW_MODE_DMA)
	   {
	   if ((idSts & (ALT_MSHC_IDMA_INT_TI | ALT_MSHC_IDMA_INT_RI | 
	                ALT_MSHC_IDMA_INT_NI)) != 0)
	       {
	       SDHC_DBG (SDHC_DBG_IRQ, 
	                        "CMD%d DMA interrupt status: 0x%x\n",
	                        host->cmdIssued.cmdIdx, idSts, 3, 4, 5, 6);

	       /* data done for dma read and write */

	       semGive (pDrvCtrl->sdHostCtrl.dataDone);
	       }

	   CSR_WRITE_4 (pDev, ALT_MSHC_IDSTS, idSts);
	   }
#endif

	return (OK);
}
	
#endif

#if 1  /* sdhc or sdmmc_ctrl*/
/*******************************************************************************
*
* sdhcCmdPrepare - prepare the command to be sent
*
* This routine prepares the command to be sent.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/
LOCAL STATUS fmshSdhcCtrl_SendCmd
    (
    VXB_DEVICE_ID       pDev, 
    UINT32              cmd, 
    UINT32              arg
    )
    {
    UINT32              i;
   /* printf("fmshSdhcCtrl_SendCmd:cmd=0x%x, arg=0x%x send  0x%x\n",cmd, arg, CSR_READ_4 (pDev, ALT_MSHC_CMD));*/
    CSR_WRITE_4 (pDev, ALT_MSHC_CMDARG, arg);
    CSR_WRITE_4 (pDev, ALT_MSHC_CMD, (UINT32)ALT_MSHC_CMD_START | cmd);
	
#ifdef SDMMC_NORMAL_SPEED 
    for (i = 0; i < ALT_MSHC_MAX_RETRIES; i++) 
    {
        /*if ((CSR_READ_4 (pDev, ALT_MSHC_CMD) & (UINT32)ALT_MSHC_CMD_START) == 0)*/
		if ((CSR_READ_4 (pDev, ALT_MSHC_STATUS) & (UINT32)ALT_MSHC_CMD_DATA_EXP) == 0)  /* bit9==0: sd/mmc not busy*/
        {
            return OK;
        }
        vxbUsDelay(10);
    }
	
#else  /* for speed*/

    for (i = 0; i < ALT_MSHC_MAX_RETRIES*10; i++) 
    {
        /*if ((CSR_READ_4 (pDev, ALT_MSHC_CMD) & (UINT32)ALT_MSHC_CMD_START) == 0)*/
		if ((CSR_READ_4 (pDev, ALT_MSHC_STATUS) & (UINT32)ALT_MSHC_STATUS_CARD_DATA_BUSY) == 0)
        {
            return OK;
        }
        /*vxbUsDelay(10);*/
    }

#endif

    SDHC_DBG (SDHC_DBG_ERR, 
                     "cmd=0x%x, arg=0x%x send timeout 0x%x\n",
                     cmd, arg, CSR_READ_4 (pDev, ALT_MSHC_CMD), 4, 5, 6);
    return ERROR;
    }

LOCAL STATUS fmshSdhcCtrl_ResetFifo
    (
    VXB_DEVICE_ID pDev
    )
    {
    UINT32 i;
    
    CSR_SETBIT_4 (pDev, ALT_MSHC_CTRL, ALT_MSHC_CTRL_FIFO_RESET);
	
#ifdef SDMMC_NORMAL_SPEED   
    for (i = 0; i < ALT_MSHC_MAX_RETRIES; i++)
    {
        if ((CSR_READ_4 (pDev, ALT_MSHC_CTRL) & ALT_MSHC_CTRL_FIFO_RESET) == 0) 
        {
            return OK;
        }
		
        vxbUsDelay(10);
    }
	
#else  /* for speed*/

    for (i = 0; i < ALT_MSHC_MAX_RETRIES*10; i++)
    {
        if ((CSR_READ_4 (pDev, ALT_MSHC_CTRL) & ALT_MSHC_CTRL_FIFO_RESET) == 0) 
        {
            return OK;
        }
		
        /*vxbUsDelay(10);*/
    }
#endif

    SDHC_DBG (SDHC_DBG_ERR, "FIFO reset time out\n",
                     1, 2, 3, 4, 5, 6);
    return ERROR;
    }

STATUS fmshSdhcCtrl_CmdPrepare
    (
    VXB_DEVICE_ID pDev,
    SD_CMD * pSdCmd
    )
{
	SD_HOST_CTRL * pSdHostCtrl;
	FMSH_SDHC_DRV_CTRL * pDrvCtrl = (FMSH_SDHC_DRV_CTRL *)pDev->pDrvCtrl;
	UINT32 xferTyp = 0;
	UINT32 i = 0;

	STATUS              status;

	UINT32              dataLen;
	UINT32              count = 0;
	UINT32              cmdSend = 0;
	UINT32 *            buf32;

	if (pDrvCtrl == NULL)
	   return ERROR;
	else
	   pSdHostCtrl = (SD_HOST_CTRL *)pDrvCtrl;

	if (pSdCmd == NULL)
	   {
	   SDHC_DBG (SDHC_DBG_ERR, 
	                    "fmshSdhcCtrl_CmdPrepare: cmd is NULL\n",
	                    1, 2, 3, 4, 5, 6);
	   return ERROR;
	   }

	/* jc: sd/mmc busy detect */
/*
	for (i = 0; i < ALT_MSHC_MAX_RETRIES; i++) 
    {
		if ((CSR_READ_4 (pDev, ALT_MSHC_STATUS) & (UINT32)ALT_MSHC_STATUS_CARD_DATA_BUSY) == 0)  // bit9==0: sd/mmc not busy
        {
            break;
        }
        vxbUsDelay(10);
    }
*/

	cmdSend = ALT_MSHC_CMD_INDX (pSdCmd->cmdIdx);

	if (cmdSend == SDMMC_CMD_STOP_TRANSMISSION)
	   cmdSend |= ALT_MSHC_CMD_STOP;
	else
	   cmdSend |= ALT_MSHC_CMD_PRV_DAT_WAIT;

	if (pSdCmd->rspType != SDMMC_CMD_RSP_NONE) 
	   {
	   cmdSend |= ALT_MSHC_CMD_RESP_EXP;
	   if (pSdCmd->rspType & SDMMC_CMD_RSP_LEN136)
	       cmdSend |= ALT_MSHC_CMD_RESP_LONG;
	   }

	if (pSdCmd->rspType & SDMMC_CMD_RSP_CRC)
	   cmdSend |= ALT_MSHC_CMD_RESP_CRC;

	if (pSdCmd->cmdIdx == SDMMC_CMD_GO_IDLE_STATE)
	   {
	   cmdSend |= ALT_MSHC_CMD_INIT;
	   }

	/* always use hold reg as drvsel is not zero */
	cmdSend |= ALT_MSHC_CMD_USE_HOLD_REG; 

	if (pSdCmd->hasData) 
	   {
	   status = fmshSdhcCtrl_ResetFifo (pDev);
	   if (status == ERROR)
	       return ERROR;

	   dataLen = pSdCmd->cmdData.blkNum *pSdCmd->cmdData.blkSize;
	   buf32   = (UINT32 *)pSdCmd->cmdData.buffer;
	   if ((buf32 == NULL) || (dataLen == 0))
	       {
	       SDHC_DBG (SDHC_DBG_ERR, 
	                        "fmshSdhcCtrl_CmdPrepare: wrong cmd data\n",
	                        1, 2, 3, 4, 5, 6);
	       return ERROR;
	       }
	  
	   
	   CSR_WRITE_4 (pDev, ALT_MSHC_BYTCNT, dataLen);
	   CSR_WRITE_4 (pDev, ALT_MSHC_BLKSIZ, pSdCmd->cmdData.blkSize);
	   
	   pDrvCtrl->remainBytes = dataLen;
	   /*printf("*emain = %d*n",pDrvCtrl->remainBytes);*/
	   cmdSend |= ALT_MSHC_CMD_DATA_EXP;

	   if (pSdCmd->cmdData.blkNum > 1)
	       {
	       cmdSend |= ALT_MSHC_CMD_SEND_AUTO_STOP;
	       }
	   if (!pSdCmd->cmdData.isRead) 
	       cmdSend |= ALT_MSHC_CMD_DATA_WR;

	   if (0/*pDrvCtrl->sdHostCtrl.dmaMode == ALT_DW_MSHC_RW_MODE_DMA*/)
	   {
	       /*fmshSdhcCtrl_IdmaStart (pDev);   dma unsupport*/
	   }
	   else
	   {
	       /* PIO mode */		 
		     pDrvCtrl->pioDesc.bufOri = pSdCmd->cmdData.buffer;
		     pDrvCtrl->pioDesc.bufCur = pSdCmd->cmdData.buffer;
		     pDrvCtrl->pioDesc.blkSize = pSdCmd->cmdData.blkSize;
		     /*syx  need blkNum!!!!!!!!		         */
	       
	       if (!pSdCmd->cmdData.isRead) 
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
	               CSR_WRITE_4 (pDev, ALT_MSHC_DATA, SWAP32(*buf32));
	               buf32++;
	               }

	           pDrvCtrl->remainBytes -= count * ALT_MSHC_FIFO_STEPS;
	           }
	       }
	}

	pDrvCtrl->pSdCmdLast = *pSdCmd;

	/*   printf("prepare:pSdCmd->cmdid=%d cmdSend=%x",pSdCmd->cmdIdx,cmdSend);*/
	return (fmshSdhcCtrl_SendCmd (pDev, cmdSend, pSdCmd->cmdArg));
}

/*******************************************************************************
*
* sdhcCmdIssue - issue the command
*
* This routine issues the command.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS fmshSdhcCtrl_CmdIssue
    (
    VXB_DEVICE_ID pDev,
    SD_CMD * pSdCmd
    )
{
	STATUS              rc;
	UINT32              mintsts;
	UINT32              idsts;
	UINT32              i;
	FMSH_SDHC_DRV_CTRL * pDrvCtrl = (FMSH_SDHC_DRV_CTRL *)(pDev->pDrvCtrl);
	SD_HOST_CTRL * pSdHostCtrl;
	UINT32 pollTimeOut = 0;
	UINT32 blkIndex;

	if (pDrvCtrl == NULL)
	   return ERROR;
	else
	   pSdHostCtrl = (SD_HOST_CTRL *) pDrvCtrl;

	if (semTake (pSdHostCtrl->hostDevSem, WAIT_FOREVER) == ERROR)
	   return ERROR;

	pDrvCtrl->count = 0;
	rc = fmshSdhcCtrl_CmdPrepare (pDev,pSdCmd);

	if (rc == ERROR)
	{
		/* printf("fmshSdhcCtrl_CmdPrepare error\n");*/
		semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
		return ERROR;
	}

	if (!pDrvCtrl->sdHostCtrl.polling)  /* irq_mode*/
	{
		rc = semTake (pDrvCtrl->sdHostCtrl.cmdDone,
		         (sysClkRateGet() * ALT_DW_MSHC_CMD_WAIT_IN_SECS));

		*pSdCmd = pDrvCtrl->pSdCmdLast;
		
		if (rc == ERROR)
		{
			SDHC_DBG (SDHC_DBG_ERR, 
			                "fmshSdhcCtrl_CmdIssue: CMD%d timeout\n",
			                pSdCmd->cmdIdx, 2, 3, 4, 5, 6);
			
			pSdCmd->cmdErr |= SDMMC_CMD_ERR_TIMEOUT;
			
			semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
			return ERROR;
		}
		else
		{
			/* printf("pSdCmd->cmdErr = %x\n",pSdCmd->cmdErr);*/
			if (pSdCmd->cmdErr != 0)
			{
				SDHC_DBG (SDHC_DBG_ERR, 
				                "fmshSdhcCtrl_CmdIssue: CMD%d error \n",
				                pSdCmd->cmdIdx, 2, 3, 4, 5, 6);
				
				semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
				return ERROR;
			}
		}
	}
	else  /* polling*/
	{
		/* poll and check status */
		for (i = 0; i < ALT_MSHC_MAX_RETRIES; i++) 
		{
			mintsts = CSR_READ_4 (pDev,  ALT_MSHC_MINTSTS);
			SDHC_DBG (SDHC_DBG_CMD, 
			            "fmshSdhcCtrl_CmdIssue: mintsts 0x%x\n",
			            mintsts, 2, 3, 4, 5, 6);

			if ((mintsts & ALT_MSHC_INT_CMD) != 0)
			{
				CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, mintsts);
				break;
			}

			if ((mintsts & ALT_MSHC_INT_RTO) != 0) 
			{
				SDHC_DBG (SDHC_DBG_ERR, 
				                "fmshSdhcCtrl_CmdIssue: "
				                "response time out\n",
				                1, 2, 3, 4, 5, 6);
				CSR_WRITE_4(pDev, ALT_MSHC_RINTSTS, mintsts);
				
				pSdCmd->cmdErr |= SDMMC_CMD_ERR_TIMEOUT;
				
				semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
				return ERROR;
			}

			if (((mintsts & ALT_MSHC_INT_RCRC) != 0) && 
			    ((pSdCmd->rspType & SDMMC_CMD_RSP_CRC) != 0))
			{
				SDHC_DBG (SDHC_DBG_ERR, 
				                "fmshSdhcCtrl_CmdIssue: "
				                "response CRC error\n",
				                1, 2, 3, 4, 5, 6);
				CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, mintsts);
				
				pSdCmd->cmdErr |= SDMMC_CMD_ERR_PROTOCOL;
				
				semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
				return ERROR;
			}

			if ((mintsts & ALT_MSHC_INT_RE) != 0)
			{
				SDHC_DBG (SDHC_DBG_ERR, 
				                "fmshSdhcCtrl_CmdIssue: response error\n",
				                1, 2, 3, 4, 5, 6);
				CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, mintsts);
				
				pSdCmd->cmdErr |= SDMMC_CMD_ERR_PROTOCOL;
				
				semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
				return ERROR;
			}

			vxbUsDelay(10);
		}

		if (i >= ALT_MSHC_MAX_RETRIES)
		{
			SDHC_DBG (SDHC_DBG_ERR, 
			                "fmshSdhcCtrl_CmdIssue: CMD%d timeout\n",
			                pSdCmd->cmdIdx, 2, 3, 4, 5, 6);
			
			pSdCmd->cmdErr |= SDMMC_CMD_ERR_TIMEOUT;
			
			semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
			return ERROR;
		}
	}

	if (pSdCmd->rspType != SDMMC_CMD_RSP_NONE)
	{
		if ((pSdCmd->rspType & SDMMC_CMD_RSP_LEN136) != 0)
		{
			UINT32 cmdRsp[4];

			cmdRsp[0] = CSR_READ_4 (pDev, ALT_MSHC_RESP0);
			cmdRsp[1] = CSR_READ_4 (pDev, ALT_MSHC_RESP1);
			cmdRsp[2] = CSR_READ_4 (pDev, ALT_MSHC_RESP2);
			cmdRsp[3] = CSR_READ_4 (pDev, ALT_MSHC_RESP3);

			pSdCmd->cmdRsp[0] = be32toh (cmdRsp[3]);
			pSdCmd->cmdRsp[1] = be32toh (cmdRsp[2]);
			pSdCmd->cmdRsp[2] = be32toh (cmdRsp[1]);
			pSdCmd->cmdRsp[3] = be32toh (cmdRsp[0]);
		}
		else
		{
			pSdCmd->cmdRsp[0] = CSR_READ_4 (pDev, ALT_MSHC_RESP0);
			pSdCmd->cmdRsp[1] = 0;
			pSdCmd->cmdRsp[2] = 0;
			pSdCmd->cmdRsp[3] = 0;
			/*  if(pSdCmd->cmdIdx==41)*/
			/*    printf("pSdCmd->cmdRsp[0]=%x\n",pSdCmd->cmdRsp[0]);*/
		}
	}

	if (pSdCmd->hasData)
	{
		if (!pDrvCtrl->sdHostCtrl.polling)  /* irq_mode*/
		{
			/* interrupt mode */
			rc = semTake (pDrvCtrl->sdHostCtrl.dataDone, 
			         (sysClkRateGet() * ALT_DW_MSHC_CMD_WAIT_IN_SECS));
			
			/* printf("#dataDone#\n");*/
			if (rc == ERROR)
			{
				SDHC_DBG (SDHC_DBG_ERR, 
				                "fmshSdhcCtrl_CmdIssue: data time out\n",
				                1, 2, 3, 4, 5, 6);
				
				pSdCmd->dataErr |= SDMMC_DATA_ERR_TIMEOUT;
				
				semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
				return ERROR;
			}
			else
			{
				if (pSdCmd->dataErr != 0)
				{
					SDHC_DBG (SDHC_DBG_ERR, 
					            "fmshSdhcCtrl_CmdIssue: data error\n",
					            1, 2, 0, 0, 0, 0);
					
					semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
					return ERROR;
				}
			}
		}
		else  /* polling*/
		{
			/* poll mode */
			if ( pDrvCtrl->sdHostCtrl.dmaMode == SDHC_DMA_MODE_PIO)
			{
				/* pio */

				if (pSdCmd->cmdData.isRead)
				{
					rc = fmshSdhcCtrl_PioRead (pDev);
					if (rc == ERROR)
					{
						semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
						return ERROR;
					}
				}
				else
				{
					rc = fmshSdhcCtrl_PioWrite (pDev);
					if (rc == ERROR)
					{
						semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
						return ERROR;
					}

					rc = fmshSdhcCtrl_PollIntsts (pDev, ALT_MSHC_INT_DTO);
					if (rc == ERROR)
					{
						SDHC_DBG (SDHC_DBG_RW, 
						            "PIO write waiting data over failed\n",
						            1, 2, 3, 4, 5, 6);
						
						semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
						return ERROR;
					}
				}
			}
			else
			{
				/* dma */
				for (i = 0; i < ALT_MSHC_MAX_RETRIES; i++) 
				{
					idsts = CSR_READ_4 (pDev,  ALT_MSHC_IDSTS);
					SDHC_DBG (SDHC_DBG_CMD, 
					            "fmshSdhcCtrl_CmdIssue: idsts 0x%x\n",
					            idsts, 2, 3, 4, 5, 6);

					if ((idsts & (ALT_MSHC_IDMA_INT_TI | ALT_MSHC_IDMA_INT_RI |
					         ALT_MSHC_IDMA_INT_NI)) != 0)
					{
						CSR_WRITE_4 (pDev, ALT_MSHC_IDSTS, idsts);
						break;
					}

					if ((idsts & ALT_MSHC_IDMA_INT_AI) != 0) 
					{
						SDHC_DBG (SDHC_DBG_ERR, 
						                "fmshSdhcCtrl_CmdIssue: "
						                "CMD%d dma error\n",
						                pDrvCtrl->pSdCmdLast.cmdIdx, 2, 3, 4, 5, 6);
						
						CSR_WRITE_4 (pDev, ALT_MSHC_IDSTS, idsts);
						
						pSdCmd->cmdErr |= SDMMC_DATA_ERR_PROTOCOL;
						break;
					}

					vxbMsDelay (1);
				}

				if (i >= ALT_MSHC_MAX_RETRIES)
				{
					SDHC_DBG (SDHC_DBG_ERR, 
					            "fmshSdhcCtrl_CmdIssue: "
					            "CMD%d dma timeout\n",
					            pDrvCtrl->pSdCmdLast.cmdIdx, 2, 3, 4, 5, 6);
					pSdCmd->cmdErr |= SDMMC_DATA_ERR_TIMEOUT;
				}
			}
		}

	}

	semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
	return (rc);
}

/*******************************************************************************
*
* sdhcBusWidthSetup - setup the bus width
*
* This routine setup the bus width.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void fmshSdhcCtrl_BusWidthSetup
    (
    VXB_DEVICE_ID pDev,
    UINT32       width
    )
{
	UINT32       buswidth;

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
	   buswidth = ALT_MSHC_CTYPE_1BIT;
	   break;
	}
	/*
	31:16 card_width 0 One bit per card indicates if card is 8-bit:
		0 – Non 8-bit mode
		1 – 8-bit mode
		Bit[31] corresponds to card[15]; bit[16] corresponds to card[0].
		
	15:0 card_width 0 One bit per card indicates if card is 1-bit or 4-bit:
		0 – 1-bit mode
		1 – 4-bit mode
		Bit[15] corresponds to card[15], bit[0] corresponds to card[0].
		Only NUM_CARDS*2 number of bits are implemented.
	*/ 
	CSR_WRITE_4 (pDev, ALT_MSHC_CTYPE, buswidth);
	return;
}

int fmshSdhcCtrl_Get_BusWidth    (VXB_DEVICE_ID pDev)
{
	UINT32       buswidth;
	
	/*
	31:16 card_width 0 One bit per card indicates if card is 8-bit:
		0 – Non 8-bit mode
		1 – 8-bit mode
		Bit[31] corresponds to card[15]; bit[16] corresponds to card[0].
		
	15:0 card_width 0 One bit per card indicates if card is 1-bit or 4-bit:
		0 – 1-bit mode
		1 – 4-bit mode
		Bit[15] corresponds to card[15], bit[0] corresponds to card[0].
		Only NUM_CARDS*2 number of bits are implemented.
	*/ 
	buswidth = CSR_READ_4 (pDev, ALT_MSHC_CTYPE);

	if (buswidth & 0x10000)
	{
		return 8; /* 8 bit mode*/
	}
	else if (buswidth & 0x01)
	{
		return 4; /* 4 bit mode*/
	}
	else if ((buswidth & 0x01) == 0x00)
	{
		return 1; /* 1 bit mode*/
	}
	else
	{
		return 0; /* unknow bit mode*/
	}
}


/*******************************************************************************
*
* sdhcCardWpCheck - check if card is write protected
*
* This routine checks if card is write protected.
*
* RETURNS: TRUE if card is write protected or FALSE
*
* ERRNO: N/A
*
* \NOMANUAL
*/

BOOL fmshSdhcCtrl_CardInsertSts
    (
    VXB_DEVICE_ID pDev
    )
    {
    UINT32 prsStat;
    SD_HOST_CTRL * pSdHostCtrl;
    pSdHostCtrl = (SD_HOST_CTRL *)pDev->pDrvCtrl;
    FMSH_SDHC_DRV_CTRL * pDrvCtrl = (FMSH_SDHC_DRV_CTRL *)(pDev->pDrvCtrl);

    BOOL                present = FALSE;
    
    if (pSdHostCtrl == NULL)
        return FALSE;

    if (!pSdHostCtrl->polling)  /* irq*/
    {
    	/*logMsg( "pDrvCtrl->flags=%x\n",pDrvCtrl->flags,2,3,4,5,6);*/
        /* Need not check return status */

        (void)semTake(pSdHostCtrl->devChange, WAIT_FOREVER);
        /*logMsg( "semTake(pSdHostCtrl->devChange, WAIT_FOREVER);\n",1,2,3,4,5,6);*/
        
#if 1  /* for sd / emmc-verify_board*/
        present = (CSR_READ_4 (pDev, ALT_MSHC_CDETECT) & 1) == 0;
#else /* for emmc-big ruineng*/
		present = TRUE;
#endif
		
        if (present)
            pSdHostCtrl->isInsert = TRUE;
        else
            pSdHostCtrl->isInsert = FALSE;
       /* logMsg( "pDrvCtrl->isInsert=%d\n",pSdHostCtrl->isInsert,2,3,4,5,6);*/
       
    }
    else  /* poll mode*/
    {
        while (1)
            {
            taskDelay (10 * sysClkRateGet());
           /* prsStat = CSR_READ_4 (pDev, SDHC_PRSSTAT);*/
           
          #if 1  /* for sd / emmc-verify_board*/
            present = (CSR_READ_4 (pDev, ALT_MSHC_CDETECT) & 1) == 0;
		  #else /* for emmc-big ruineng*/
            present = TRUE;
		  #endif
		   
            if (present)
                {
                if (pSdHostCtrl->isInsert != TRUE)
                    {
                    pSdHostCtrl->isInsert = TRUE;
                    break;
                    }
                }
            else
                {
                if (pSdHostCtrl->isInsert != FALSE)
                    {
                    pSdHostCtrl->isInsert = FALSE;
                    break;
                    }
                }
            }
        }

    return (pSdHostCtrl->isInsert);
    }

/*******************************************************************************
*
* sdhcCardWpCheck - check if card is write protected
*
* This routine checks if card is write protected.
*
* RETURNS: TRUE if card is write protected or FALSE
*
* ERRNO: N/A
*
* \NOMANUAL
*/

BOOL fmshSdhcCtrl_CardWpCheck
    (
    VXB_DEVICE_ID pDev
    )
    {
    BOOL wp = FALSE;
    FMSH_SDHC_DRV_CTRL * pDrvCtrl = (FMSH_SDHC_DRV_CTRL *)pDev->pDrvCtrl;

   
#if 1  /* for sd / emmc-verify_board*/
        if ((pDrvCtrl->flags & ALT_MSHC_FLAGS_CARD_WRITABLE) != 0)
            {
            wp = FALSE;
            }
        else
            {
            wp = (CSR_READ_4 (pDev, ALT_MSHC_WRTPRT) & 1) != 0;
            }
        
 #else  /* for emmc-big ruineng*/
	wp = FALSE; 
 #endif

    return  (wp);
    }

/*******************************************************************************
*
* sdhcPioRead - read data from SD/MMC card using PIO
*
* This routine reads data from SD/MMC card using PIO.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/
LOCAL void fmshSdhcCtrl_ReadFifo
    (
    VXB_DEVICE_ID       pDev
    )
{    
	FMSH_SDHC_DRV_CTRL * pDrvCtrl = (FMSH_SDHC_DRV_CTRL *)pDev->pDrvCtrl;
	UINT32 *buf = pDrvCtrl->pioDesc.bufCur;
   /* UINT32 *           buf = (UINT32 *host->cmdIssued.cmdData.buffer;*/
    UINT32              i;
    UINT32              bytesRead;
    UINT32              mshcStatus;
    UINT32              fifoLen;

    bytesRead = (pDrvCtrl->pSdCmdLast.cmdData.blkSize)*(pDrvCtrl->pSdCmdLast.cmdData.blkNum) \
		        - pDrvCtrl->remainBytes;
	
    buf += bytesRead / ALT_MSHC_FIFO_STEPS;
    
    mshcStatus = CSR_READ_4 (pDev, ALT_MSHC_STATUS);
    fifoLen = ALT_MSHC_STATUS_FIFO_COUNT (mshcStatus);

    for (i = 0; i < fifoLen; i++)
    {
        *buf = SWAP32(CSR_READ_4(pDev, ALT_MSHC_DATA));
        buf++;
    }

    pDrvCtrl->remainBytes -= fifoLen * ALT_MSHC_FIFO_STEPS;
    SDHC_DBG (SDHC_DBG_RW, "PIO read %d bytes, remain %d bytes  %x #\n", 
                     fifoLen * 4, pDrvCtrl->remainBytes, mshcStatus>>17, 4, 5, 6);

	return;
}


LOCAL STATUS fmshSdhcCtrl_PollIntsts
    (
    VXB_DEVICE_ID       pDev,
    UINT32              mask
    )
{
    UINT32              intsts;
    UINT32              i;

#ifdef SDMMC_NORMAL_SPEED    /* speed slow a little*/
    for (i = 0; i < ALT_MSHC_MAX_RETRIES; i++)
    {
        intsts = CSR_READ_4 (pDev, ALT_MSHC_RINTSTS);

        if ((intsts & ALT_MSHC_INT_ERROR) != 0)
        {
            CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, intsts);
            SDHC_DBG (SDHC_DBG_ERR, 
                             "Poll MINTSTS failed with 0x%x\n", 
                             intsts, 2, 3, 4, 5, 6);
            return ERROR;
        }

        if ((intsts & mask) != 0)
        {
            CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, (intsts & mask));
            return OK;
        }

        vxbUsDelay (100);
    }
	
#else /* for speed*/

    for (i = 0; i < ALT_MSHC_MAX_RETRIES*10; i++)
    {
        intsts = CSR_READ_4 (pDev, ALT_MSHC_RINTSTS);

        if ((intsts & ALT_MSHC_INT_ERROR) != 0)
        {
            CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, intsts);
            SDHC_DBG (SDHC_DBG_ERR, 
                             "Poll MINTSTS failed with 0x%x\n", 
                             intsts, 2, 3, 4, 5, 6);
            return ERROR;
        }

        if ((intsts & mask) != 0)
        {
            CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, (intsts & mask));
            return OK;
        }

        /*vxbUsDelay (100);*/
    }
#endif

    CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, intsts);
    SDHC_DBG (SDHC_DBG_RW, "Poll MINTSTS time out with 0x%x\n", 
                     intsts, 2, 3, 4, 5, 6);
    return ERROR;
}


STATUS fmshSdhcCtrl_PioRead
    (
    VXB_DEVICE_ID   pDev
    )
{
    FMSH_SDHC_DRV_CTRL * pDrvCtrl = (FMSH_SDHC_DRV_CTRL *)pDev->pDrvCtrl;
    UINT32 blkSize;
    char * buffer;
    UINT32 data = 0;
    UINT32 i;
    STATUS              status;
    if (pDrvCtrl == NULL)
        return ERROR;

    blkSize = pDrvCtrl->pioDesc.blkSize;
    buffer = pDrvCtrl->pioDesc.bufCur;
	
    /* logMsg("fmshSdhcCtrl_PioRead:blkSize = %d buffer=%x pDrvCtrl->remainBytes=%d\n ",blkSize,buffer,pDrvCtrl->remainBytes,4,5,6);*/
    while (pDrvCtrl->remainBytes != 0) 
	{
		status = fmshSdhcCtrl_PollIntsts (pDev,
		                                ALT_MSHC_INT_RXDR | 
		                                ALT_MSHC_INT_DTO  | 
		                                ALT_MSHC_INT_HTO);
		if (status == ERROR)
		{
			SDHC_DBG (SDHC_DBG_ERR, 
			                "PIO read error, remain %d bytes\n", 
			                pDrvCtrl->remainBytes, 2, 3, 4, 5, 6);
			return ERROR;
		}

		fmshSdhcCtrl_ReadFifo (pDev);
		
	#ifdef SDMMC_NORMAL_SPEED	
		vxbUsDelay (100);
	#else	
		vxbUsDelay (10);  /* for speed*/
	#endif
	}

    return OK;
}

/*******************************************************************************
*
* sdhcPioWrite - write data to SD/MMC card using PIO
*
* This routine writes data to SD/MMC card using PIO.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/
LOCAL void fmshSdhcCtrl_WriteFifo
    (
    VXB_DEVICE_ID       pDev
    )
{
	FMSH_SDHC_DRV_CTRL * pDrvCtrl = (FMSH_SDHC_DRV_CTRL *)pDev->pDrvCtrl;
	UINT32* buf = pDrvCtrl->pioDesc.bufCur;   
   
    UINT32              i;
    UINT32              mshcStatus;
    UINT32              bytesWritten;
    UINT32              fifoSlots;
    UINT32              writeLen;    

    bytesWritten =(pDrvCtrl->pSdCmdLast.cmdData.blkSize) * (pDrvCtrl->pSdCmdLast.cmdData.blkNum) - pDrvCtrl->remainBytes;
    buf += bytesWritten / ALT_MSHC_FIFO_STEPS;
 
    mshcStatus =  CSR_READ_4 (pDev, ALT_MSHC_STATUS);
    fifoSlots = ALT_MSHC_STATUS_FIFO_COUNT (mshcStatus);
    
   /* logMsg("#remain=%d slot=%d d=%d i=%x#\n",pDrvCtrl->remainBytes,fifoSlots,CSR_READ_4 (pDev, ALT_MSHC_BYTCNT),CSR_READ_4 (pDev, ALT_MSHC_RINTSTS),5,6);*/
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
        CSR_WRITE_4 (pDev, ALT_MSHC_DATA, SWAP32(*buf));
        buf++;
    }

    pDrvCtrl->remainBytes -= writeLen * ALT_MSHC_FIFO_STEPS;
    SDHC_DBG (SDHC_DBG_RW, "PIO write %d bytes, remain %d bytes\n", 
                     writeLen * 4, pDrvCtrl->remainBytes, 3, 4, 5, 6);
	
	return;
}

STATUS fmshSdhcCtrl_PioWrite
    (
    VXB_DEVICE_ID   pDev
    )
{
    FMSH_SDHC_DRV_CTRL * pDrvCtrl = (FMSH_SDHC_DRV_CTRL *)pDev->pDrvCtrl;
    UINT32 blkSize;
    char * buffer;
    UINT32 data;
    UINT32 i;
    STATUS              status;
	
    if (pDrvCtrl == NULL)
        return ERROR;

    blkSize = pDrvCtrl->pioDesc.blkSize;
    buffer = pDrvCtrl->pioDesc.bufCur;
       
	while (pDrvCtrl->remainBytes != 0) 
	{
		status = fmshSdhcCtrl_PollIntsts (pDev, 
		                                ALT_MSHC_INT_TXDR | 
		                                ALT_MSHC_INT_HTO);

		if (status == ERROR)
		{
			SDHC_DBG (SDHC_DBG_ERR, 
			                "PIO write error, remain %d bytes\n", 
			                pDrvCtrl->remainBytes, 2, 3, 4, 5, 6);
			return ERROR;
		}

		fmshSdhcCtrl_WriteFifo (pDev);

	#ifdef SDMMC_NORMAL_SPEED
		vxbUsDelay (100);
	#else
		vxbUsDelay (10);  /* for speed*/
	#endif
	}
	
	return OK;
}
	
/*finished above*/
/*******************************************************************************
*
* sdhcCmdIssuePoll - issue the command using polling mode
*
* This routine issues the command using polling mode.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS fmshSdhcCtrl_CmdIssuePoll
    (
    VXB_DEVICE_ID   pDev,
    SD_CMD * pSdCmd
    )
{
	STATUS              rc;
	UINT32              mintsts;
	UINT32              idsts;
	UINT32              i;
	FMSH_SDHC_DRV_CTRL * pDrvCtrl = (FMSH_SDHC_DRV_CTRL *)(pDev->pDrvCtrl);
	SD_HOST_CTRL * pSdHostCtrl;
	UINT32 pollTimeOut = 0;

	UINT32 blkIndex;

	if (pDrvCtrl == NULL)
		return ERROR;
	else
		pSdHostCtrl = (SD_HOST_CTRL *) pDrvCtrl;

	if (semTake (pSdHostCtrl->hostDevSem, WAIT_FOREVER) == ERROR)
		return ERROR;
	
	rc = fmshSdhcCtrl_CmdPrepare (pDev, pSdCmd);
	if (rc == ERROR)
	{
		semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
		return ERROR;
	}

	/* poll and check status */	  
	for (i = 0; i < ALT_MSHC_MAX_RETRIES; i++) 
	{
		mintsts = CSR_READ_4 (pDev,  ALT_MSHC_MINTSTS);
		SDHC_DBG (SDHC_DBG_ERR, "fmshSdhcCtrl_CmdIssue: mintsts 0x%x\n",mintsts, 2, 3, 4, 5, 6);

		/*  printf("mintsts=%x\n",mintsts);*/
		if ((mintsts & ALT_MSHC_INT_CMD) != 0)
		{
		    CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, mintsts);
		    break;
		}
		
		if ((mintsts & ALT_MSHC_INT_RTO) != 0) 
		{
			SDHC_DBG (SDHC_DBG_ERR, 
			              "fmshSdhcCtrl_CmdIssue: "
			              "response time out mintsts=%x\n",
			              mintsts, 2, 3, 4, 5, 6);
			CSR_WRITE_4(pDev, ALT_MSHC_RINTSTS, mintsts);
			
			pSdCmd->cmdErr |= SDMMC_CMD_ERR_TIMEOUT;
			
			semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
			return ERROR;
		}

		if (((mintsts & ALT_MSHC_INT_RCRC) != 0) && 
		     ((pSdCmd->rspType & SDMMC_CMD_RSP_CRC) != 0))
		 {
			 SDHC_DBG (SDHC_DBG_ERR, 
			                  "fmshSdhcCtrl_CmdIssue: "
			                  "response CRC error\n",
			                  1, 2, 3, 4, 5, 6);
			 CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, mintsts);
			 
			 pSdCmd->cmdErr |= SDMMC_CMD_ERR_PROTOCOL;
			 
			 semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
			 return ERROR;
		}

		if ((mintsts & ALT_MSHC_INT_RE) != 0)
		{
			SDHC_DBG (SDHC_DBG_ERR, 
			              "fmshSdhcCtrl_CmdIssue: response error\n",
			              1, 2, 3, 4, 5, 6);
			CSR_WRITE_4 (pDev, ALT_MSHC_RINTSTS, mintsts);
			
			pSdCmd->cmdErr |= SDMMC_CMD_ERR_PROTOCOL;
			
			semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
			return ERROR;
		}
		
		vxbUsDelay(10);
	}

	if (i >= ALT_MSHC_MAX_RETRIES)
	{
		SDHC_DBG (SDHC_DBG_ERR, 
		              "fmshSdhcCtrl_CmdIssue: CMD%d timeout\n",
		              pSdCmd->cmdIdx, 2, 3, 4, 5, 6);
		
		pSdCmd->cmdErr |= SDMMC_CMD_ERR_TIMEOUT;
		
		semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
		return ERROR;
	}

	if (pSdCmd->rspType != SDMMC_CMD_RSP_NONE)
	{
		if ((pSdCmd->rspType & SDMMC_CMD_RSP_LEN136) != 0)
		{
			UINT32 cmdRsp[4];

			cmdRsp[0] = CSR_READ_4 (pDev, ALT_MSHC_RESP0);
			cmdRsp[1] = CSR_READ_4 (pDev, ALT_MSHC_RESP1);
			cmdRsp[2] = CSR_READ_4 (pDev, ALT_MSHC_RESP2);
			cmdRsp[3] = CSR_READ_4 (pDev, ALT_MSHC_RESP3);

			pSdCmd->cmdRsp[0] = be32toh (cmdRsp[3]);
			pSdCmd->cmdRsp[1] = be32toh (cmdRsp[2]);
			pSdCmd->cmdRsp[2] = be32toh (cmdRsp[1]);
			pSdCmd->cmdRsp[3] = be32toh (cmdRsp[0]);
		}
		else
		{
			pSdCmd->cmdRsp[0] = CSR_READ_4 (pDev, ALT_MSHC_RESP0);
			pSdCmd->cmdRsp[1] = 0;
			pSdCmd->cmdRsp[2] = 0;
			pSdCmd->cmdRsp[3] = 0;
		}
	 }

	if (pSdCmd->hasData)
	{
		if (0)  /* irq_mode*/
		{
			/* interrupt mode */
			rc = semTake (pDrvCtrl->sdHostCtrl.dataDone, 
			(sysClkRateGet() * ALT_DW_MSHC_CMD_WAIT_IN_SECS));
			
			if (rc == ERROR)
			{
				SDHC_DBG (SDHC_DBG_ERR, 
				      "fmshSdhcCtrl_CmdIssue: data time out\n",
				      1, 2, 3, 4, 5, 6);
				
				pSdCmd->dataErr |= SDMMC_DATA_ERR_TIMEOUT;
				
				semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
				return ERROR;
			}
			else
			{
				if (pSdCmd->dataErr != 0)
				{
					SDHC_DBG (SDHC_DBG_ERR, 
					          "fmshSdhcCtrl_CmdIssue: data error\n",
					          1, 2, 0, 0, 0, 0);
					
					semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
					return ERROR;
				}
			}
		}
		else  /* polling*/
		{
			/* poll mode */
			if (pDrvCtrl->sdHostCtrl.dmaMode == SDHC_DMA_MODE_PIO)
			{
				/* pio */
				if (pSdCmd->cmdData.isRead)
				{
					rc = fmshSdhcCtrl_PioRead (pDev);
					if (rc == ERROR)
					{
						semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
						return ERROR;
					}
				}
				else
				{
					rc = fmshSdhcCtrl_PioWrite (pDev);
					if (rc == ERROR)
					{
						semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
						return ERROR;
					}

					rc = fmshSdhcCtrl_PollIntsts (pDev, ALT_MSHC_INT_DTO);
					if (rc == ERROR)
					{
						SDHC_DBG (SDHC_DBG_RW, 
						              "PIO write waiting data over failed\n",
						              1, 2, 3, 4, 5, 6);
						semGive (pDrvCtrl->sdHostCtrl.hostDevSem);
						return ERROR;
					}
				}
			}
			else
			{
				/* dma */
				for (i = 0; i < ALT_MSHC_MAX_RETRIES; i++) 
				{
					idsts = CSR_READ_4 (pDev,  ALT_MSHC_IDSTS);
					SDHC_DBG (SDHC_DBG_CMD, 
					          "fmshSdhcCtrl_CmdIssue: idsts 0x%x\n",
					          idsts, 2, 3, 4, 5, 6);

					if ((idsts & (ALT_MSHC_IDMA_INT_TI | ALT_MSHC_IDMA_INT_RI |
					       ALT_MSHC_IDMA_INT_NI)) != 0)
					{
						CSR_WRITE_4 (pDev, ALT_MSHC_IDSTS, idsts);
						break;
					}

					if ((idsts & ALT_MSHC_IDMA_INT_AI) != 0) 
					{
						SDHC_DBG (SDHC_DBG_ERR, 
						              "fmshSdhcCtrl_CmdIssue: "
						              "CMD%d dma error\n",
						              pSdCmd->cmdIdx, 2, 3, 4, 5, 6);
						CSR_WRITE_4 (pDev, ALT_MSHC_IDSTS, idsts);
						pSdCmd->cmdErr |= SDMMC_DATA_ERR_PROTOCOL;
						break;
					}

					vxbMsDelay (1);
				}

				if (i >= ALT_MSHC_MAX_RETRIES)
				{
					SDHC_DBG (SDHC_DBG_ERR, 
					          "fmshSdhcCtrl_CmdIssue: "
					          "CMD%d dma timeout\n",
					          pSdCmd->cmdIdx, 2, 3, 4, 5, 6);
					pSdCmd->cmdErr |= SDMMC_DATA_ERR_TIMEOUT;
				}
			}
		}
	}
	
	semGive (pDrvCtrl->sdHostCtrl.hostDevSem);

	return OK;
}

#endif

/*******************************************************************************
* sdhcInterruptInfo - Returns the SDHC interrupt info ptr from the bus
* controller.
*
* \NOMANUAL
*
* RETURNS: OK, or ERROR if this library is not initialized
*
* ERRNO
*
* \NOMANUAL
*/

/*STATUS sdhcInterruptInfo*/
STATUS fmshSdhcCtrl_InterruptInfo
    (
    VXB_DEVICE_ID pInst,
    UINT32 * pArgs
    )
    {
    FMSH_SDHC_DRV_CTRL * pDrvCtrl = (FMSH_SDHC_DRV_CTRL *)pInst->pDrvCtrl;

    if (pDrvCtrl == NULL)
        return ERROR;
    *(UINT32 *)pArgs = (UINT32)((ULONG)pDrvCtrl->pIntInfo);

    return(OK);
    }

/*******************************************************************************
*
* sdhcDevControl - device control routine
*
* This routine handles manipulation of downstream devices, such as
* interrupt management.
*
* RETURNS: OK, or ERROR
*
* ERRNO
*
* \NOMANUAL
*/

STATUS fmshSdhcCtrl_DevControl
    (
    VXB_DEVICE_ID    devID,
    pVXB_DEVCTL_HDR  pBusDevControl
    )
    {
    VXB_DEVICE_ID busCtrlID;
    FMSH_SDHC_DRV_CTRL *pDrvCtrl;
    STATUS status = OK;
    VOIDFUNCPTR * vector = NULL;
    struct plbIntrEntry * pIntr;
    pVXB_ACCESS_INTERRUPT_INFO pAccessIntInfo;
    UINT32 * pVector;

    if ((devID ==  NULL) || (pBusDevControl == NULL))
        return (ERROR);

    busCtrlID = vxbDevParent(devID);
    if (busCtrlID == NULL)
        return (ERROR);

    pDrvCtrl = ((FMSH_SDHC_DRV_CTRL *)busCtrlID->pDrvCtrl);
    if (pDrvCtrl == NULL)
        return (ERROR);

    pIntr = busCtrlID->pIntrInfo;
    if (pIntr == NULL)
        return (ERROR);

    pAccessIntInfo = (pVXB_ACCESS_INTERRUPT_INFO)pBusDevControl;
    if (busCtrlID->busID == VXB_BUSID_PLB)
        {
        pVXB_ACCESS_INT_VECTOR_GET pIntVector;
        pIntVector = (pVXB_ACCESS_INT_VECTOR_GET)pAccessIntInfo;
        vector = pIntr->intVecList[pAccessIntInfo->intIndex];
        }
    if (busCtrlID->busID == VXB_BUSID_PCI)
        {
        struct pciIntrEntry * pInterruptInfo;
        pInterruptInfo = (struct pciIntrEntry *)(busCtrlID->pIntrInfo);
        vector = (VOIDFUNCPTR*)pInterruptInfo->intVecInfo[pAccessIntInfo->intIndex].intVector;
        }
    if (pAccessIntInfo->intIndex >= pIntr->numVectors)
        return (ERROR);

    switch ( pBusDevControl->vxbAccessId)
        {
        case VXB_ACCESS_INT_CONNECT:
            {

            /* connect the interrupt */

            status = vxbSdioIntConnect(devID,
                                       vector,
                                       (VOIDFUNCPTR)pAccessIntInfo->pISR,
                                       (_Vx_usr_arg_t)pAccessIntInfo->pParameter
                                       );

            return(status);
            }
        default:
            return ERROR;
        }
    }

STATUS fmshSdhc_CmdIssue
    (
    VXB_DEVICE_ID pDev,
    SD_CMD * pSdCmd
    )
    {
    SD_HOST_CTRL * pSdHostCtrl;
    STATUS rc;

    pSdHostCtrl = (SD_HOST_CTRL *)(pDev->pDrvCtrl);
    if (pSdHostCtrl == NULL)
        return ERROR;

    if (pSdHostCtrl->polling)
        rc = fmshSdhcCtrl_CmdIssuePoll(pDev, pSdCmd);
    else
        rc = fmshSdhcCtrl_CmdIssue(pDev, pSdCmd);

    return (rc);
    }

STATUS fmshSdhcCtrl_SpecInfoGet
    (
    VXB_DEVICE_ID pDev,
    void ** pHostSpec,
    VXB_SD_CMDISSUE_FUNC * pCmdIssue
    )
    {
    SD_HOST_CTRL * pSdHostCtrl;

    pSdHostCtrl = (SD_HOST_CTRL *)pDev->pDrvCtrl;
    if (pSdHostCtrl == NULL)
        return ERROR;

    *pHostSpec = (void *)(&(pSdHostCtrl->sdHostSpec));
    *pCmdIssue = fmshSdhc_CmdIssue;
    return (OK);
    }


#if 1
/*******************************************************************************
*
* sdhcInstInit2 - second level initialization routine of sdhc device
*
* This routine performs the second level initialization of the sdhc device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/
IMPORT void sysMshcClkFreqSetup(UINT32 clk);
void fmshSdhc_ClkFreqSetup(VXB_DEVICE_ID pDev, UINT32 clk)
{
	/* jc*/
	if (pDev->unitNumber == 0)  /* sd_ctrl_0*/
	{
		clk_switch_sdmmc_0();
	}
	else /* if (pDev->unitNumber == 1)  // sd_ctrl_1*/
	{
		clk_switch_sdmmc_1();
	}

	sysMshcClkFreqSetup(clk);

	return;
}

void test_readintMMC(void)
{
	printf("# %x  %x#\n",*(volatile UINT32 *)(0xE0043000+0x44),(*(volatile UINT32 *)(0xE0043000+ALT_MSHC_STATUS))>>17);
}

int fmshSdhc_Get_Bitmode(int ctrl_x)
{
    VXB_DEVICE_ID       pDev;
    FMSH_SDHC_DRV_CTRL * pDrvCtrl;
	
    pDev = vxbInstByNameFind (FMSH_SDHC_NAME, ctrl_x);
    if (pDev == NULL)
    {
        printf ("Can not find sd/emmc at sd_ctrl_%d ! \n", ctrl_x);
        return ERROR;
    }
	
	return fmshSdhcCtrl_Get_BusWidth(pDev);
}

#define SDHC_PROCTL             0x028

STATUS fmshSdhc_VddSetup(VXB_DEVICE_ID pDev, UINT32 vdd)
{
    volatile UINT32 val;

    /* The Cavium Networks CNS3xxx only suport 3.3V, so ignore other values */

#if 1  /* 3.3v sd/emmc*/
    vdd = PROCTL_SDBUS_VOLT_3P3;
#else  /* 1.8v sd/emmc*/
    vdd = PROCTL_SDBUS_VOLT_1P8;
#endif

    val = CSR_READ_4(pDev, SDHC_PROCTL);
    val |= (vdd << PROCTL_VOLT_SEL_SHIFT) | PROCTL_SD_PWR_EN;

    CSR_WRITE_4(pDev, SDHC_PROCTL, val);

    vxbMsDelay(150);
    return OK;
}

#endif


#if 1

/*
sd_ctrl_1: sd_card
*/
int file_idx_dosfs = 0;
/*void test_sd_speed_dosfs(void)*/
void test_sd_speed_dosfs(char* file_name)
{
	float speed = 0.0;
	UINT32 all_size = 0;
	int tick_start = 0, tick_end = 0;
	int fd = 0;
	char file_name[64] = {0};
	int file_size = 0;

	UINT8* pBuf8 = (UINT8*)malloc(0x1000000);  /* 16M*/


	/**/
	/* read*/
	/**/
	tick_start = tickGet();
	/*sprintf(file_name, "%s", "/sd0:0/zImage");*/
	sprintf(file_name, "/sd0:0/%s", file_name);
	
	fd = open(file_name, O_RDWR, 0);
	if (fd == -1)
	{
		logMsg("error: open file failed \r\n", 0,0,0,0,0,0);
		goto ERR;
	}

	all_size = read(fd, pBuf8, 0x1000000);	
	tick_end = tickGet();	
	
	speed = (float)(all_size * 1.0) / ((float)(tick_end - tick_start)*1.0 / sysClkRateGet());	
	logMsg("read %d bytes data of file(%s) \r\n", all_size, file_name,0,0,0,0);

	printf("test sd_dosfs read_speed: %.2f bytes/s \n", speed);
	close(fd);

	printf("size-%d, end-%d, start-%d (ClkRate-%d) \n", all_size, tick_end, tick_start, sysClkRateGet());

	/**/
	/* write*/
	/*	*/
	tick_start = tickGet();
	
	sprintf(file_name, "/sd0:0/test_%d", file_idx_dosfs);
	file_idx_dosfs++;
	
    /*
    fd = open(file_name, O_RDWR, 0);
    if (fd == -1)
	{
		logMsg("error: create file(%s) failed \r\n", file_name,0,0,0,0,0);
        goto ERR;
    }
	*/
    fd = creat(file_name, O_RDWR); 
    if (fd == -1)
	{
		logMsg("error: creat file(%s) failed\r\n", file_name, 0,0,0,0,0);
        goto ERR;
    }
	
	file_size = 0x800000; /* 8M	*/
    all_size = write(fd, pBuf8, file_size); 
	
    if (all_size != file_size)
	{
		logMsg("error: write %d bytes data\r\n", all_size,0,0,0,0,0);
        close(fd);
        goto ERR;
    }
	tick_end = tickGet();	
	
	speed = (float)(all_size * 1.0) / ((float)(tick_end - tick_start)*1.0 / sysClkRateGet());
	logMsg("write %d bytes data of file(%s) \r\n", all_size, file_name, 0,0,0,0);
	printf("data: %d_bit mode, speed:%d Hz \n", fmshSdhc_Get_Bitmode(1), sysMshcClkFreq_Get(1));   /* sd_ctrl_1: sd_card*/

	printf("test sd_dosfs write_speed: %.2f bytes/s \n", speed);
	close(fd);

ERR:	
	free(pBuf8);	
	return;
}

/*
sd_ctrl_0: emmc
*/
void test_mmc_speed_dosfs(void)
{
	float speed = 0.0;
	UINT32 all_size = 0;
	int tick_start = 0, tick_end = 0;
	int fd = 0;
	char file_name[64] = {0};	
	int file_size = 0x1000000;  /* max: 16M*/
	
	UINT8* pBuf8 = (UINT8*)malloc(file_size);

	tick_start = tickGet();

	sprintf(file_name, "%s", "/mmc0:0/zImage");
	
	/* read*/
	fd = open(file_name, O_RDWR, 0);
	if(fd == -1)
	{
		logMsg("error: open file(%s) failed \r\n", file_name, 0,0,0,0,0);
		goto ERR;
	}
	
	all_size = read(fd, pBuf8, file_size);	
	tick_end = tickGet();	
	
	speed = (float)(all_size * 1.0) / ((float)(tick_end - tick_start)*1.0 / sysClkRateGet());
	logMsg("read %d bytes data of file(%s) \r\n", all_size, file_name, 0,0,0,0);

	printf("test mmc_dosfs read speed: %.2f bytes/s \n", speed);
	close(fd);
	
	/**/
	/* write*/
	/*	*/
	tick_start = tickGet();
	
	sprintf(file_name, "/mmc0:0/test_%d", file_idx_dosfs);
	file_idx_dosfs++;
	
	/*
	fd = open(file_name, O_RDWR, 0);
	if (fd == -1)
	{
		logMsg("error: create file(%s) failed \r\n", file_name,0,0,0,0,0);
		goto ERR;
	}
	*/
	fd = creat(file_name, O_RDWR); 
	if (fd == -1)
	{
		logMsg("error: creat file(%s) failed\r\n", file_name, 0,0,0,0,0);
		goto ERR;
	}
	
	file_size = 0x800000; /* 8M */
	all_size = write(fd, pBuf8, file_size); 
	
	if (all_size != file_size)
	{
		logMsg("error: write %d bytes data\r\n", all_size,0,0,0,0,0);
		close(fd);
		goto ERR;
	}
	tick_end = tickGet();	
	
	speed = (float)(all_size * 1.0) / ((float)(tick_end - tick_start)*1.0 / sysClkRateGet());
	logMsg("write %d bytes data of file(%s) \r\n", all_size, file_name, 0,0,0,0);

	printf("data: %d_bit mode, speed:%d Hz \n", fmshSdhc_Get_Bitmode(0), sysMshcClkFreq_Get(0));   /* sd_ctrl_0: sd_card*/

	printf("test mmc_dosfs write_speed: %.2f bytes/s \n", speed);
	close(fd);

ERR:	
	free(pBuf8);	
	return;
}

/*
7045_verfiy_board:
==================

-> test_mmc_speed
0x15841b0 (tShell0): read 6009968 bytes data of file(/mmc0:0/zImage) 
test mmc_dosfs read speed: 32781644.00 bytes/s 
0x15841b0 (tShell0): write 8388608 bytes data of file(/mmc0:0/test_3) 
data: 4_bit mode, speed:50000000 Hz 
test mmc_dosfs write_speed: 7626007.50 bytes/s 
value = 0 = 0x0
-> 
-> 
-> 
-> 
-> test_sd_speed
0x15841b0 (tShell0): read 6009968 bytes data of file(/sd0:0/zImage) 
test sd_dosfs read_speed: 30049840.00 bytes/s 
0x15841b0 (tShell0): write 8388608 bytes data of file(/sd0:0/test_4) 
data: 4_bit mode, speed:50000000 Hz 
test sd_dosfs write_speed: 14380471.00 bytes/s 
value = 0 = 0x0
-> 
*/

void test_mmc_wr_file(char * filename)
{
	float speed = 0.0;
	UINT32 all_size = 0;
	int tick_start = 0, tick_end = 0;
	int fd = 0;
	char file_name[64] = {0};	
	int file_size = 0x1000000;  /* max: 16M*/
	
	UINT8* pBuf8 = (UINT8*)malloc(file_size);
	int i = 0;
	
	for (i=0; i<file_size; i++)
	{
		pBuf8[i] = i;
	}
	
	/**/
	/* write*/
	/*	*/
	tick_start = tickGet();
	
	sprintf(file_name, "/mmc0:0/%s", filename);
	
	/*
	fd = open(file_name, O_RDWR, 0);
	if (fd == -1)
	{
		logMsg("error: create file(%s) failed \r\n", file_name,0,0,0,0,0);
		goto ERR;
	}
	*/
	fd = creat(file_name, O_RDWR); 
	if (fd == -1)
	{
		logMsg("error: creat file(%s) failed\r\n", file_name, 0,0,0,0,0);
		goto ERR;
	}
	
	file_size = 0x800000; /* 8M 	*/
	all_size = write(fd, pBuf8, file_size); 
	
	if (all_size != file_size)
	{
		logMsg("error: write %d bytes data\r\n", all_size,0,0,0,0,0);
		close(fd);
		goto ERR;
	}
	tick_end = tickGet();	
	
	taskDelay(10);
	speed = (float)(all_size * 1.0) / ((float)(tick_end - tick_start)*1.0 / sysClkRateGet());
	logMsg("write %d bytes data of file(%s) \r\n", all_size, file_name, 0,0,0,0);

	logMsg("data: %d_bit mode, speed:%d Hz \n", fmshSdhc_Get_Bitmode(0), sysMshcClkFreq_Get(0), 0,0,0,0);   /* sd_ctrl_0: sd_card*/

	taskDelay(10);
	printf("test vx69_mmc_dosfs write_speed: %.2f bytes/s \n", speed, 0,0,0,0,0);
	close(fd);

ERR:	
	free(pBuf8);	
	return;
}

void test_mmc_rd_file(char* filename)
{
	float speed = 0.0;
	UINT32 all_size = 0;
	int tick_start = 0, tick_end = 0;
	int fd = 0, i = 0;
	char file_name[64] = {0};	
	int file_size = 0x1000000;  /* max: 16M*/
	
	UINT8* pBuf8 = (UINT8*)malloc(file_size);

	tick_start = tickGet();

	sprintf(file_name, "/mmc0:0/%s", filename);
	
	/* read*/
	fd = open(file_name, O_RDWR, 0);
	if(fd == -1)
	{
		logMsg("error: open file(%s) failed \r\n", file_name, 0,0,0,0,0);
		goto ERR;
	}
	
	all_size = read(fd, pBuf8, file_size);	
	tick_end = tickGet();
	
	for (i=0; i<64; i++)
	{
		printf("%02X", pBuf8[i]);
		
		if ((i+1)%8 == 0)
		{
			printf(" ");
		}
		
		if ((i+1)%32 == 0)
		{
			printf("\n");
		}
	}
	printf("\n---------\n");
	for (i=(all_size-64); i<all_size; i++)
	{
		printf("%02X", pBuf8[i]);
		
		if ((i+1)%8 == 0)
		{
			printf(" ");
		}
		
		if ((i+1)%32 == 0)
		{
			printf("\n");
		}
	}

	
	taskDelay(10);
	speed = (float)(all_size * 1.0) / ((float)(tick_end - tick_start)*1.0 / sysClkRateGet());
	logMsg("read %d bytes data of file(%s) \r\n", all_size, file_name, 3,4,5,6);

	taskDelay(10);		
	logMsg("data: %d_bit mode, speed:%d Hz \n", fmshSdhc_Get_Bitmode(0), sysMshcClkFreq_Get(0), 3,4,5,6);   /* sd_ctrl_0: sd_card*/

	taskDelay(10);		
	printf("test vx69_mmc_dosfs read_speed: %.2f bytes/s \n", speed, 2,3,4,5,6);
	close(fd);

ERR:	
	free(pBuf8);	
	return;
}

/*
7045_verfiy_board:
==================
-> devs
drv name                
  0 /null               
  1 /tyCo/0             
  8 host:               
  9 /vio                
  3 /mmc0:0             
value = 25 = 0x19
-> 
-> ll  "/mmc0:0"

Listing Directory /mmc0:0:
-rwxrwxrwx  1 0       0              8388608 Jan  1  1980 test1.txt 
-rwxrwxrwx  1 0       0              8388608 Jan  1  1980 test2.txt 
-rwxrwxrwx  1 0       0              8388608 Jan  1  1980 test3.txt 
-rwxrwxrwx  1 0       0              8388608 Jan  1  1980 test4.txt 
-rwxrwxrwx  1 0       0              8388608 Jan  1  1980 test5.txt 
value = 0 = 0x0
-> 
-> test_mmc_wr_file "test5.txt"
0x1ef5c18 (tShell0): write 8388608 bytes data of file(/mmc0:0/test5.txt) 
0x1ef5c18 (tShell0): data: 4_bit mode, speed:50000000 Hz 
test vx69_mmc_dosfs write_speed: 1958429.88 bytes/s 
value = 0 = 0x0
-> 
-> ll  "/mmc0:0"

Listing Directory /mmc0:0:
-rwxrwxrwx  1 0       0              8388608 Jan  1  1980 test1.txt 
-rwxrwxrwx  1 0       0              8388608 Jan  1  1980 test2.txt 
-rwxrwxrwx  1 0       0              8388608 Jan  1  1980 test3.txt 
-rwxrwxrwx  1 0       0              8388608 Jan  1  1980 test4.txt 
-rwxrwxrwx  1 0       0              8388608 Jan  1  1980 test5.txt 
value = 0 = 0x0
-> 
-> test_mmc_wr_file "test5.txt"
0x1ef5c18 (tShell0): write 8388608 bytes data of file(/mmc0:0/test5.txt) 
0x1ef5c18 (tShell0): data: 4_bit mode, speed:50000000 Hz 
test mmc_dosfs write_speed: 1958429.88 bytes/s 
value = 0 = 0x0
-> 
-> test_mmc_rd_file "test5.txt"
0001020304050607 08090A0B0C0D0E0F 1011121314151617 18191A1B1C1D1E1F 
2021222324252627 28292A2B2C2D2E2F 3031323334353637 38393A3B3C3D3E3F 

---------
C0C1C2C3C4C5C6C7 C8C9CACBCCCDCECF D0D1D2D3D4D5D6D7 D8D9DADBDCDDDEDF 
E0E1E2E3E4E5E6E7 E8E9EAEBECEDEEEF F0F1F2F3F4F5F6F7 F8F9FAFBFCFDFEFF 
0x1ef5c18 (tShell0): read 8388608 bytes data of file(/mmc0:0/test5.txt) 
0x1ef5c18 (tShell0): data: 4_bit mode, speed:50000000 Hz 
test vx69_mmc_dosfs read_speed: 15252015.00 bytes/s 
value = 0 = 0x0
-> 
*/


/*
7045_demo_board or 7045_verify_board:
*/
void test_sd_wr_file(char * filename)
{
	float speed = 0.0;
	UINT32 all_size = 0;
	int tick_start = 0, tick_end = 0;
	int fd = 0;
	char file_name[64] = {0};	
	int file_size = 0x1000000;  /* max: 16M*/
	
	UINT8* pBuf8 = (UINT8*)malloc(file_size);
	int i = 0;
	
	for (i=0; i<file_size; i++)
	{
		pBuf8[i] = i;
	}
	
	/**/
	/* write*/
	/*	*/
	tick_start = tickGet();
	
	sprintf(file_name, "/sd0:1/%s", filename);
	
	/*
	fd = open(file_name, O_RDWR, 0);
	if (fd == -1)
	{
		logMsg("error: create file(%s) failed \r\n", file_name,0,0,0,0,0);
		goto ERR;
	}
	*/
	fd = creat(file_name, O_RDWR); 
	if (fd == -1)
	{
		logMsg("error: creat file(%s) failed\r\n", file_name, 0,0,0,0,0);
		goto ERR;
	}
	
	file_size = 0x800000; /* 8M 	*/
	all_size = write(fd, pBuf8, file_size); 
	
	if (all_size != file_size)
	{
		logMsg("error: write %d bytes data\r\n", all_size,0,0,0,0,0);
		close(fd);
		goto ERR;
	}
	tick_end = tickGet();	
	
	taskDelay(10);
	speed = (float)(all_size * 1.0) / ((float)(tick_end - tick_start)*1.0 / sysClkRateGet());
	logMsg("write %d bytes data of file(%s) \r\n", all_size, file_name, 0,0,0,0);

	logMsg("data: %d_bit mode, speed:%d Hz \n", fmshSdhc_Get_Bitmode(0), sysMshcClkFreq_Get(0), 0,0,0,0);   /* sd_ctrl_0: sd_card*/

	taskDelay(10);
	printf("test vx69_sd_dosfs write_speed: %.2f bytes/s \n", speed, 0,0,0,0,0);
	close(fd);

ERR:	
	free(pBuf8);	
	return;
}

void test_sd_rd_file(char* filename)
{
	float speed = 0.0;
	UINT32 all_size = 0;
	int tick_start = 0, tick_end = 0;
	int fd = 0, i = 0;
	char file_name[64] = {0};	
	int file_size = 0x1000000;  /* max: 16M*/
	
	UINT8* pBuf8 = (UINT8*)malloc(file_size);

	tick_start = tickGet();

	sprintf(file_name, "/sd0:1/%s", filename);
	
	/* read*/
	fd = open(file_name, O_RDWR, 0);
	if(fd == -1)
	{
		logMsg("error: open file(%s) failed \r\n", file_name, 0,0,0,0,0);
		goto ERR;
	}
	
	all_size = read(fd, pBuf8, file_size);	
	tick_end = tickGet();
	
	for (i=0; i<64; i++)
	{
		printf("%02X", pBuf8[i]);
		
		if ((i+1)%8 == 0)
		{
			printf(" ");
		}
		
		if ((i+1)%32 == 0)
		{
			printf("\n");
		}
	}
	printf("\n---------\n");
	for (i=(all_size-64); i<all_size; i++)
	{
		printf("%02X", pBuf8[i]);
		
		if ((i+1)%8 == 0)
		{
			printf(" ");
		}
		
		if ((i+1)%32 == 0)
		{
			printf("\n");
		}
	}

	
	taskDelay(10);
	speed = (float)(all_size * 1.0) / ((float)(tick_end - tick_start)*1.0 / sysClkRateGet());
	logMsg("read %d bytes data of file(%s) \r\n", all_size, file_name, 3,4,5,6);

	taskDelay(10);		
	logMsg("data: %d_bit mode, speed:%d Hz \n", fmshSdhc_Get_Bitmode(0), sysMshcClkFreq_Get(0), 3,4,5,6);   /* sd_ctrl_0: sd_card*/

	taskDelay(10);		
	printf("test vx69_sd_dosfs read_speed: %.2f bytes/s \n", speed, 2,3,4,5,6);
	close(fd);

ERR:	
	free(pBuf8);	
	return;
}

/*
7045_demo_board:
================

uboot:
------
fmsh> mmc info

Device: dwmmc@e0043000
Manufacturer ID: 3
OEM: 5344
Name: SD04G 
Bus Speed: 50000000
Mode : SD High Speed (50MHz)
Rd Block Len: 512
SD version 3.0
High Capacity: Yes
Capacity: 3.7 GiB
Bus Width: 4-bit
Erase Group Size: 512 Bytes
fmsh>  

vx6.9:
======
-> devs
drv name                
  0 /null               
  1 /tyCo/0             
  8 host:               
  9 /vio                
  3 /sd0:1              
  3 /sd0:2              
value = 25 = 0x19
-> test_sd_wr_file "test_sd_2.txt"
0x19fd0d8 (tShell0): write 8388608 bytes data of file(/sd0:1/test_sd_2.txt) 
0x19fd0d8 (tShell0): data: 4_bit mode, speed:49999999 Hz 
test vx69_sd_dosfs write_speed: 1935832.62 bytes/s 
value = 0 = 0x0
-> test_sd_rd_file "test_sd_2.txt"
0001020304050607 08090A0B0C0D0E0F 1011121314151617 18191A1B1C1D1E1F 
2021222324252627 28292A2B2C2D2E2F 3031323334353637 38393A3B3C3D3E3F 

---------
C0C1C2C3C4C5C6C7 C8C9CACBCCCDCECF D0D1D2D3D4D5D6D7 D8D9DADBDCDDDEDF 
E0E1E2E3E4E5E6E7 E8E9EAEBECEDEEEF F0F1F2F3F4F5F6F7 F8F9FAFBFCFDFEFF 
0x19fd0d8 (tShell0): read 8388608 bytes data of file(/sd0:1/test_sd_2.txt) 
0x19fd0d8 (tShell0): data: 4_bit mode, speed:49999999 Hz 
test vx69_sd_dosfs read_speed: 11983726.00 bytes/s 
value = 0 = 0x0
-> 
*/
#endif

