/* vxbAltSocGen5Wdt.c -  VxBus WDT driver for Altera SoC Gen 5 */

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
This module contains the BSP support routines for the Altera SoC Watch Dog 
Timer. The WDT can only be used as period timer.
*/

/* includes */

#include <vxWorks.h>
#include <vxbTimerLib.h>
#include <string.h>

#include <hwif/util/hwMemLib.h>
#include <hwif/vxbus/vxBus.h>
#include <hwif/vxbus/vxbPlbLib.h>
#include <hwif/vxbus/hwConf.h>
#include <spinLockLib.h>
#include <vxBusLib.h>

/* defines */

/* #undef WDT_DBG_ON */
#define WDT_DBG_ON

#ifdef  WDT_DBG_ON
#define WDT_DBG_IRQ         0x00000001
#define WDT_DBG_RW          0x00000002
#define WDT_DBG_ERR         0x00000004
#define WDT_DBG_ALL         0xffffffff
#define WDT_DBG_OFF         0x00000000
LOCAL UINT32 wdtDbgMask = WDT_DBG_ALL;
IMPORT FUNCPTR _func_logMsg;

#define WDT_DBG(mask, string, a, b, c, d, e, f)         \
    if ((wdtDbgMask & mask) || (mask == WDT_DBG_ALL))   \
        if (_func_logMsg != NULL)                       \
            (* _func_logMsg)(string, a, b, c, d, e, f)
#else
#define WDT_DBG(mask, string, a, b, c, d, e, f)
#endif  /* WDT_DBG_ON */

#define DEFAULT_WDT_TICKS_PER_SECOND    60
#define WDT_PERIOD_MAX                  (15)
#define WDT_PERIOD(x)                   (0x1 << (16 + x))
#define WDT_TIMER_MAX_COUNT             (0x1 << 31)

#define ALTERA_WDT_DRIVER_NAME          "altWdt"

#undef TIMER_BAR
#define TIMER_BAR(p)    ((char *)((p)->pRegBase[0]))

#undef TIMER_HANDLE
#define TIMER_HANDLE(p) (((ALTERA_WDT_TIMER *)(p)->pDrvCtrl)->handle)

#ifdef ARMBE8
#    define SWAP32 vxbSwap32
#else
#    define SWAP32 
#endif /* ARMBE8 */

#undef CSR_READ_2
#define CSR_READ_2(pDev, addr)          \
        SWAP32(vxbRead32(TIMER_HANDLE(pDev),	\
                  (UINT32 *)((char *)TIMER_BAR(pDev) + addr)))

#undef CSR_READ_1
#define CSR_READ_1(pDev, addr)          \
        vxbRead8(TIMER_HANDLE(pDev),    \
                 (UINT8 *)((char *)TIMER_BAR(pDev) + addr))

#undef CSR_WRITE_2
#define CSR_WRITE_2(pDev, addr, data)   \
        vxbWrite32(TIMER_HANDLE(pDev),  \
                   (UINT32 *)((char *)TIMER_BAR(pDev) + addr), SWAP32(data))

#undef CSR_WRITE_1
#define CSR_WRITE_1(pDev, addr, data)   \
        vxbWrite8(TIMER_HANDLE(pDev),   \
                   (UINT8 *)((char *)TIMER_BAR(pDev) + addr), data)

/* 
Peripheral Module Reset Register 
*/
#if 0
#define  RESET_PERIPH                   0x14
#define  RESET_PERIPH_L4WD0             (1 << 6)
#define  RESET_PERIPH_L4WD1             (1 << 7)
#else
/*
WDT_RST_CTRL	0x330	
---------------------
	reserved	31:2	rw	0x0	Reserved. 写无效，读为0
	WDT_RST	1	rw	0x0	"WDT REF 时钟复位：
					0：不复位
					1：复位"
	WDT_APB_RST	0	rw	0x0	"WDT APB 时钟复位：
					0：不复位
					1：复位"
*/
#define  RESET_PERIPH                   0x330
#define  RESET_PERIPH_L4WD0             (0x3 << 0)
/* #define  RESET_PERIPH_L4WD1             (1 << 7) */
#endif


/* Watchdog Register */

#define  WDT_CR_OFFSET             0x00
#define  WDT_TORR_OFFSET           0x04
#define  WDT_CCVR_OFFSET           0x08
#define  WDT_CRR_OFFSET            0x0C
#define  WDT_STAT_OFFSET           0x10
#define  WDT_EOI_OFFSET            0x14
#define  WDT_COMP_PARAMS_5_OFFSET  0xE4
#define  WDT_COMP_PARAMS_4_OFFSET  0xE8
#define  WDT_COMP_PARAMS_3_OFFSET  0xEC
#define  WDT_COMP_PARAMS_2_OFFSET  0xF0
#define  WDT_COMP_PARAMS_1_OFFSET  0xF4
#define  WDT_COMP_VERSION_OFFSET   0xF8
#define  WDT_COMP_TYPE_OFFSET      0xFC

#define COMP_VERSION               0x3130332a 

#define WDT_ENABLE_MASK 0x01
#define WDT_ENABLE_IRQRST 0x02

#define WDT_PET_WATCHDOG_CRR_VALUE 0x76

/* typedefs */

/* structure to store the timer information */

typedef struct alteraWdtTimer
    {
    VXB_DEVICE_ID                   pDev;
    struct vxbTimerFunctionality    timerFunc;
    void                            (*pIsrFunc)(int);
    int                             arg;
    spinlockIsr_t                   spinLock;
    UINT32                          timerVal;
    UINT8                           control;
    UINT16                          clockTimerRollOver;
    BOOL                            isEnabled;
    void *                          handle;
    UINT32                          resetMgrBase;
    UINT8                           WdtChanNo;
    } ALTERA_WDT_TIMER;

/* forward declarations */

LOCAL void vxbAltSocGen5WdtInstInit(VXB_DEVICE_ID);
LOCAL void vxbAltSocGen5WdtInstInit2(VXB_DEVICE_ID);
LOCAL void vxbAltSocGen5WdtInstConnect(VXB_DEVICE_ID);
    
LOCAL STATUS vxbAltSocGen5WdtAllocate
    (
    VXB_DEVICE_ID       pInst,
    UINT32	        flags,
    void **	        pCookie,
    UINT32	        timerNo
    );

LOCAL STATUS vxbAltSocGen5WdtRelease
    (
    VXB_DEVICE_ID	pInst,
    void *	        pCookie
    );

LOCAL STATUS vxbAltSocGen5WdtRolloverGet
    (
    void *	pCookie,
    UINT32 *	count
    );
    
LOCAL STATUS vxbAltSocGen5WdtCountGet
    (
    void *	pCookie,
    UINT32 *	count
    );
    
LOCAL STATUS vxbAltSocGen5WdtISRSet
    (
    void *	pCookie,
    void	(*pFunc)(int),
    int		arg
    );
    
LOCAL STATUS vxbAltSocGen5WdtEnable
    (
    void *	pCookie,
    UINT32	maxTimerCount
    );

LOCAL STATUS vxbAltSocGen5WdtDisable
    (
    void *	pCookie
    );

LOCAL STATUS vxbAltSocGen5WdtIsr
    (
    VXB_DEVICE_ID        pDev   /* Information about the driver */
    );

LOCAL struct drvBusFuncs AlteraWdtDrvFuncs =
    {
    vxbAltSocGen5WdtInstInit,          /* devInstanceInit */
    vxbAltSocGen5WdtInstInit2,         /* devInstanceInit2 */
    vxbAltSocGen5WdtInstConnect        /* devConnect */
    };

LOCAL device_method_t AlteraWdtDrv_methods[] =
{    
   DEVMETHOD_END 
};

LOCAL struct vxbDevRegInfo AlteraWdtDrvRegistration =
    {
    NULL,                       /* pNext */
    VXB_DEVID_DEVICE,           /* devID */
    VXB_BUSID_PLB,              /* busID = PLB */
    VXB_VER_4_0_0,              /* busVer */
    ALTERA_WDT_DRIVER_NAME,     /* drvName */
    &AlteraWdtDrvFuncs,         /* pDrvBusFuncs */
    AlteraWdtDrv_methods,       /* pMethods */
    NULL                        /* devProbe */
    };

/******************************************************************************
 *
 * vxbAltSocGen5WdtDrvRegister - register Altera Wdt driver
 *
 * This routine registers the Altera Wdt driver with the vxBus subsystem.
 *
 * RETURNS: N/A
 *
 * ERRNO: N/A
 */

void vxbAltSocGen5WdtDrvRegister (void)
{  
    vxbDevRegister(&AlteraWdtDrvRegistration);
}

/* for test */
void vxbAltSocGen5WdtDrvRegister2 (void)
{  
   /* vxbDevRegister(&AlteraWdtDrvRegistration); */
 }

/*******************************************************************************
 *
 * vxbAltSocGen5WdtInstInit - first level initialization routine of Wdt device
 *
 * This routine performs the first level initialization of the Wdt device.
 *
 * RETURNS: N/A
 *
 * ERRNO: N/A
 */

LOCAL void vxbAltSocGen5WdtInstInit
    (
    VXB_DEVICE_ID pDev
    )
    {
    const struct hcfDevice * pHcf = NULL;
    
    ALTERA_WDT_TIMER *pTimer;
    
    struct vxbTimerFunctionality * pTimerFunc;
    
    /* allocate the memory for the timer structure */
    pTimer = (ALTERA_WDT_TIMER *) hwMemAlloc (sizeof (ALTERA_WDT_TIMER));
    
    /* check if memory allocation is successful */

    if (pTimer == NULL)
        return;

    pTimer->pDev = pDev;
    pDev->pDrvCtrl = pTimer;
    
    /* locate the timer functionality data structure */
    pTimerFunc = &(pTimer->timerFunc);
        
    pHcf = hcfDeviceGet (pDev);
    if (pHcf == NULL)
        {
#ifndef _VXBUS_BASIC_HWMEMLIB
        hwMemFree ((char *) pTimer);
#endif /* _VXBUS_BASIC_HWMEMLIB */
        return;
        }

    /*
     * resourceDesc {
     * The clkFreq resource specifies the sytem
     * clock frequency retrieving routine address. }
     */

    /* get the clock frequency */     
    if (devResourceGet(pHcf, "clkFreq", HCF_RES_INT, 
                       (void *) &pTimerFunc->clkFrequency) != OK)
        {
#ifndef _VXBUS_BASIC_HWMEMLIB
        hwMemFree ((char *) pTimer);
#endif /* _VXBUS_BASIC_HWMEMLIB */
        return;
        }

    /*
     * resourceDesc {
     * The resetMgrBase resource specifies the
     * base address of reset manager
     */
    if (devResourceGet (pHcf, "resetMgrBase", HCF_RES_ADDR,
                        (void *)&pTimer->resetMgrBase) != OK)
        {
#ifndef _VXBUS_BASIC_HWMEMLIB
        hwMemFree ((char *) pTimer);
#endif /* _VXBUS_BASIC_HWMEMLIB */
        return;
        }
    
    /*
     * resourceDesc {
     * The HCF_RES_INT resource specifies the
     * watchdog timer channel number
     */
    if (devResourceGet (pHcf, "WdtChanNo", HCF_RES_INT,
                        (void *)&pTimer->WdtChanNo) != OK)
        {
#ifndef _VXBUS_BASIC_HWMEMLIB
        hwMemFree ((char *) pTimer);
#endif /* _VXBUS_BASIC_HWMEMLIB */
        return;
        }
    
    vxbRegMap(pDev, 0, &pTimer->handle);   /* map the window */
    
    /*
     * resourceDesc {
     * The maxClkRate resource specifies the
     * maximum clock rate (in ticks/sec) of
     * timer }
     */

    if (devResourceGet (pHcf, "maxClkRate", HCF_RES_INT,
                    (void *)&pTimerFunc->maxFrequency) != OK)
        {
#ifndef _VXBUS_BASIC_HWMEMLIB
        hwMemFree ((char *) pTimer);
#endif /* _VXBUS_BASIC_HWMEMLIB */
        return;
        }

    /*
     * resourceDesc {
     * The minClkRate resource specifies the
     * minimum clock rate (in ticks/sec) of
     * timer }
     */

    if (devResourceGet (pHcf, "minClkRate", HCF_RES_INT,
                    (void *)&pTimerFunc->minFrequency) != OK)
        {
#ifndef _VXBUS_BASIC_HWMEMLIB
        hwMemFree ((char *) pTimer);
#endif /* _VXBUS_BASIC_HWMEMLIB */
        return;
        }

    /* copy and null terminate the name of the timer */    
    strncpy (pTimerFunc->timerName, ALTERA_WDT_DRIVER_NAME, MAX_DRV_NAME_LEN);
    pTimerFunc->timerName [MAX_DRV_NAME_LEN] = '\0';

    pTimerFunc->features =  VXB_TIMER_CAN_INTERRUPT | 
                            VXB_TIMER_CANNOT_SUPPORT_ALL_FREQS;
    
    /* set the default ticks per second */

    pTimerFunc->ticksPerSecond = DEFAULT_WDT_TICKS_PER_SECOND;
    
    pTimerFunc->timerAllocate = vxbAltSocGen5WdtAllocate;
    pTimerFunc->timerRelease = vxbAltSocGen5WdtRelease;
    pTimerFunc->timerRolloverGet = vxbAltSocGen5WdtRolloverGet;
    pTimerFunc->timerCountGet = vxbAltSocGen5WdtCountGet;
    pTimerFunc->timerDisable = vxbAltSocGen5WdtDisable;
    pTimerFunc->timerEnable = vxbAltSocGen5WdtEnable;
    pTimerFunc->timerISRSet = vxbAltSocGen5WdtISRSet;
 
    /* initialize the spinlock */
    SPIN_LOCK_ISR_INIT (&pTimer->spinLock, 0);
    
    /* publish methods */
    pDev->pMethods = AlteraWdtDrv_methods;
    
    }

/*******************************************************************************
 *
 * vxbAltSocGen5WdtInstInit2 - second level initialization routine of Wdt modules
 *
 * This routine performs the second level initialization of the Wdt modules.
 *
 * This routine is called later during system initialization.  OS features
 * such as memory allocation are available at this time.
 *
 * RETURNS: N/A
 *
 * ERRNO: N/A
 *
 * \NOMANUAL
 *
 */

LOCAL void vxbAltSocGen5WdtInstInit2
    (
    VXB_DEVICE_ID pDev
    )
    {
    /* connect the ISR for the timer */
    
    if (vxbIntConnect (pDev, 0, vxbAltSocGen5WdtIsr, pDev) != OK)
        pDev->pDrvCtrl = NULL;
    
    }

/*******************************************************************************
 *
 * vxbAltSocGen5WdtInstConnect - third level initialization routine of Wdt device
 *
 * This routine performs the third level initialization of the Wdt device.
 *
 * RETURNS: N/A
 *
 * ERRNO: N/A
 */

LOCAL void vxbAltSocGen5WdtInstConnect 
    (
    VXB_DEVICE_ID pDev
    )
    {
    return;
    }

/*******************************************************************************
 *
 * vxbAltSocGen5WdtAllocate - allocate resources for a timer
 *
 * This routine allocates resources for a timer by the Timer Abstraction Layer.
 *
 * RETURNS: OK or ERROR if timer allocation fails
 *
 * ERRNO: N/A
 */

LOCAL STATUS vxbAltSocGen5WdtAllocate
    (
    VXB_DEVICE_ID	pInst,
    UINT32	        flags,
    void **	        pCookie,
    UINT32	        timerNo
    )
    {
    ALTERA_WDT_TIMER * pDrvCtrl = NULL;

    /* validate the parameters */

    if ((pInst == NULL) || (pCookie == NULL) || (timerNo != 0))
        return ERROR;

    pDrvCtrl = (ALTERA_WDT_TIMER *) pInst->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return ERROR;

    /* take the spinlock for the exclusive access to the timer resources */

    SPIN_LOCK_ISR_TAKE (&pDrvCtrl->spinLock);

    /* check to see if the timer is already allocated */

    if (pDrvCtrl->timerFunc.allocated)
        {
        /* release the spinlock */

        SPIN_LOCK_ISR_GIVE (&pDrvCtrl->spinLock);
        return ERROR;
        }

    /* store the timer information in the pCookie */
    
    *pCookie = pDrvCtrl;

    /* set the timer allocated flag */

    pDrvCtrl->timerFunc.allocated = TRUE;

    /* release the spinlock */

    SPIN_LOCK_ISR_GIVE (&pDrvCtrl->spinLock);
    
    return OK;
    }
    
/*******************************************************************************
 *
 * vxbAltSocGen5WdtRelease - release the timer resource
 *
 * This routine releases the resources allocated for a timer device.
 *
 * RETURNS: OK or ERROR if the parameter is invalid
 *
 * ERRNO: N/A
 */

LOCAL STATUS vxbAltSocGen5WdtRelease
    (
    VXB_DEVICE_ID	pInst,
    void *	        pCookie
    )
    {
    ALTERA_WDT_TIMER * pDrvCtrl = NULL;
    
    /* validate the parameters */

    if ((pInst == NULL) || (pCookie == NULL))
        return ERROR;

    pDrvCtrl = (ALTERA_WDT_TIMER *) pInst->pDrvCtrl;

    /* validate pInst and check to see if the timer is allocated */

    if ((pDrvCtrl->pDev != pInst) || (!pDrvCtrl->timerFunc.allocated))
        return ERROR;
    
    /* take the spinlock for the exclusive access to the timer resources */

    SPIN_LOCK_ISR_TAKE (&pDrvCtrl->spinLock);
    
    /* disable the timer */
    
    if (vxbAltSocGen5WdtDisable (pCookie) != OK)
        {
        /* release the spinlock */

        SPIN_LOCK_ISR_GIVE (&pDrvCtrl->spinLock);
        return ERROR;
        }

    pDrvCtrl->pIsrFunc = NULL;
    pDrvCtrl->arg = 0;

    /* reset the timer allocated flag */

    pDrvCtrl->timerFunc.allocated = FALSE;
    
    /* release the spinlock */

    SPIN_LOCK_ISR_GIVE (&pDrvCtrl->spinLock);
    
    return OK;
    }

/*******************************************************************************
*
* vxbAltSocGen5WdtRolloverGet - retrieve the maximum value of the counter
*
* This routine retrieves the maximum value of the counter.
*
* RETURNS: OK
*
* ERRNO: N/A
*/
   
LOCAL STATUS vxbAltSocGen5WdtRolloverGet
    (
    void *	pCookie,
    UINT32 *	count
    )
    {
    ALTERA_WDT_TIMER * pDrvCtrl = NULL;
    
    if ((pCookie == NULL) || (count == NULL))
        return ERROR;
    
    pDrvCtrl = (ALTERA_WDT_TIMER *) pCookie;
    
    /* take the spinlock for the exclusive access to the timer resources */

    SPIN_LOCK_ISR_TAKE (&pDrvCtrl->spinLock);
    
    /* store the counter value */
    
    *count = pDrvCtrl->timerVal;
    
    /* release the spinlock */

    SPIN_LOCK_ISR_GIVE (&pDrvCtrl->spinLock);
    
    return OK;
    }

/*******************************************************************************
*
* vxbAltSocGen5WdtCountGet - retrieve the current value of the counter
*
* This routine retrieves the current value of the counter.
*
* RETURNS: OK or ERROR if the parameter is invalid
*
* ERRNO: N/A
*/

LOCAL STATUS vxbAltSocGen5WdtCountGet
    (
    void *	pCookie,
    UINT32 *	count
    )
    {
    ALTERA_WDT_TIMER * pDrvCtrl = NULL;
    VXB_DEVICE_ID       pDev;
    UINT32 snap;
    
    if ((pCookie == NULL) || (count == NULL))
        return ERROR;
    
    pDrvCtrl = (ALTERA_WDT_TIMER *) pCookie;
    pDev = pDrvCtrl->pDev ;
    
    /* take the spinlock for the exclusive access to the timer resources */

    SPIN_LOCK_ISR_TAKE (&pDrvCtrl->spinLock);
    
    /* read the current value of the timer */
    
    snap = CSR_READ_2(pDev, WDT_CCVR_OFFSET);
    
    /* 
     * Set the result to the max value of the timer minus the current value,
     * which will be the number of decrements of the counter that have been 
     * made since the previous "load" of the counter
     */
    
    *count = pDrvCtrl->timerVal - snap;
    
    /* release the spinlock */

    SPIN_LOCK_ISR_GIVE (&pDrvCtrl->spinLock);
    
    return OK;
    }

/*******************************************************************************
*
* vxbAltSocGen5WdtISRSet - set a function to be called on the timer interrupt
*
* This routine sets a function to be called on the timer interrupt.
*
* RETURNS: OK
*
* ERRNO: N/A
*/

LOCAL STATUS vxbAltSocGen5WdtISRSet
    (
    void *	pCookie,
    void	(*pFunc)(int),
    int		arg
    )
    {
    ALTERA_WDT_TIMER * pDrvCtrl = NULL;
    
    if ((pCookie == NULL) || (pFunc == NULL))
        return ERROR;
    
    pDrvCtrl = (ALTERA_WDT_TIMER *) pCookie;
    
    /* take the spinlock for the exclusive access to the timer resources */

    SPIN_LOCK_ISR_TAKE (&pDrvCtrl->spinLock);
    
    /* store the interrupt routine and argument information */
    
    pDrvCtrl->pIsrFunc = pFunc;
    pDrvCtrl->arg = arg;
    
    /* release the spinlock */

    SPIN_LOCK_ISR_GIVE (&pDrvCtrl->spinLock);
    
    return OK;
    }

/*******************************************************************************
*
* vxbAltSocGen5WdtEnable - enable the timer
*
* This routine updates the maximum count value and enables the timer.
*
* RETURNS: OK or ERROR if timer is not enabled
*
* ERRNO: N/A
*/

LOCAL STATUS vxbAltSocGen5WdtEnable
    (
    void *	pCookie,
    UINT32	maxTimerCount
    )
{
    VXB_DEVICE_ID       pDev;
    ALTERA_WDT_TIMER * pDrvCtrl = NULL;
    struct vxbTimerFunctionality * pTF;
    
    UINT32 resetMgrBase = 0;
    UINT32 val;
    int    ix;
    int    period = 0;
    unsigned long long    tempWdtCount;
    
    /* validate the parameters */    
    if ((pCookie == NULL) || (maxTimerCount == 0) || 
        (maxTimerCount > (UINT32)WDT_TIMER_MAX_COUNT ))
        return ERROR;
    
    pDrvCtrl = (ALTERA_WDT_TIMER *) pCookie;
    pDev = pDrvCtrl->pDev ;
    pTF = &pDrvCtrl->timerFunc;
     
    /* take the spinlock for the exclusive access to the timer resources */
    SPIN_LOCK_ISR_TAKE (&pDrvCtrl->spinLock);
    
    if (pDrvCtrl->isEnabled)
    {
        if (vxbAltSocGen5WdtDisable (pCookie) != OK)
        {
            /* release the spinlock */
            SPIN_LOCK_ISR_GIVE (&pDrvCtrl->spinLock);
            return ERROR;
        }
    }

    /* release WDT from reset */    
    resetMgrBase = pDrvCtrl->resetMgrBase + RESET_PERIPH;
    val = *(volatile unsigned int *) resetMgrBase;
    
    if (pDrvCtrl->WdtChanNo == 0)
        val &= ~RESET_PERIPH_L4WD0;
    /*else */
    /*    val &= ~RESET_PERIPH_L4WD1; */
	
    *(volatile unsigned int *)(resetMgrBase) = val;
    
    tempWdtCount = maxTimerCount; 
            
    /* caculate the best period */    
    for (ix = 0; ix < WDT_PERIOD_MAX; ix++)
        {
        if (tempWdtCount > (WDT_PERIOD(ix)))
            {
            if (tempWdtCount <= (WDT_PERIOD(ix) + WDT_PERIOD(ix + 1))/2)
                period = ix;
            else
                period = ix + 1;
            }
        else
            break;
        }

    WDT_DBG (WDT_DBG_RW, "\nInput maxTimerCount %d, output %d, val %d", 
             maxTimerCount, period, WDT_PERIOD(period), 0, 0, 0);

    /* save the clock speed */    
    pDrvCtrl->timerVal = WDT_PERIOD(period);
    
    /* update ​rollover​Period */    
    pTF->rolloverPeriod = pDrvCtrl->timerVal / (pTF->clkFrequency);
    
    /* set timeout value */    
    CSR_WRITE_1(pDev, WDT_TORR_OFFSET, period | (period << 0x4));
    
    val = CSR_READ_1(pDev, WDT_CR_OFFSET);    
    val |= WDT_ENABLE_MASK | WDT_ENABLE_IRQRST;    
    CSR_WRITE_1(pDev, WDT_CR_OFFSET, val);

    if (vxbIntEnable (pDev, 0, vxbAltSocGen5WdtIsr, (void *) pDev) != OK)
    {
        /* release the spinlock */
        SPIN_LOCK_ISR_GIVE (&pDrvCtrl->spinLock);            
        return ERROR;
    }
    
    /* set the enabled flag */    
    pDrvCtrl->isEnabled = TRUE;
    
    /* release the spinlock */
    SPIN_LOCK_ISR_GIVE (&pDrvCtrl->spinLock);
    
    return OK;
}


/*******************************************************************************
*
* vxbAltSocGen5WdtDisable - disable the timer
*
* This routine disables the timer.
*
* RETURNS: OK
*
* ERRNO: N/A
*/
LOCAL STATUS vxbAltSocGen5WdtDisable
    (
    void *	pCookie
    )
    {
    ALTERA_WDT_TIMER * pDrvCtrl = NULL;
    UINT32 resetMgrBase = 0;
    VXB_DEVICE_ID       pDev;
    UINT32 val;
    
    if (pCookie == NULL)
        return ERROR;
    
    pDrvCtrl = (ALTERA_WDT_TIMER *) pCookie;
    
    pDev = pDrvCtrl->pDev;
    
    /* take the spinlock for the exclusive access to the timer resources */

    SPIN_LOCK_ISR_TAKE (&pDrvCtrl->spinLock);
    
    /* reset WDT */    
    resetMgrBase = pDrvCtrl->resetMgrBase + RESET_PERIPH;

    if (pDrvCtrl->WdtChanNo == 0)
        *(volatile unsigned int *)(resetMgrBase) = RESET_PERIPH_L4WD0;
    /*else */
    /*    *(volatile unsigned int *)(resetMgrBase) = RESET_PERIPH_L4WD1; */
    
    /* release the spinlock */

    SPIN_LOCK_ISR_GIVE (&pDrvCtrl->spinLock);

    return OK;
    }

/*******************************************************************************
*
* vxbAltSocGen5WdtIsr - Alter interal timer interrupt handler
*
* This routine handles the timer interrupt.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL STATUS vxbAltSocGen5WdtIsr
    (
    VXB_DEVICE_ID        pDev   /* Information about the driver */
    )
 {
    ALTERA_WDT_TIMER * pDrvCtrl = NULL;
    
    if (pDev == NULL)
        return ERROR;
    
    pDrvCtrl = pDev->pDrvCtrl;

    /* take the spinlock for the exclusive access to the timer resources */
    SPIN_LOCK_ISR_TAKE (&pDrvCtrl->spinLock);
    
    if (CSR_READ_1(pDev, WDT_STAT_OFFSET))
    {        
        /* ack interrupt */        
        (void) CSR_READ_1(pDev, WDT_EOI_OFFSET);

        /* write 0x76 to Counter Restart Register */        
        CSR_WRITE_1(pDev, WDT_CRR_OFFSET, WDT_PET_WATCHDOG_CRR_VALUE);
    }
    
    /* release the spinlock */
    SPIN_LOCK_ISR_GIVE (&pDrvCtrl->spinLock);

    /* call the ISR hander if one is registered */    
    if (pDrvCtrl->isEnabled && pDrvCtrl->pIsrFunc != NULL)
    {
        (*(pDrvCtrl->pIsrFunc))(pDrvCtrl->arg);
    }
    
    return OK;
}

#if 1

FUNCPTR wdtCntGet  = NULL;

int wdt_time_get(void)
{
/*
	VXB_DEVICE_ID       pDev;
    ALTERA_WDT_TIMER * pDrvCtrl;
    struct vxbTimerFunctionality * pTimerFunc;
	
	UINT32 count = 0;	
	int unit = 0;
	
	pDev = vxbInstByNameFind ("altWdt", unit);
	if (NULL == pDev)
	{
		printf("wdt_time_get: __LINE__=%d error \n", __LINE__);
		return ERROR;
	}
	else
	{
		printf("wdt_time_get: pDev=0x%08X \n", pDev);
	}
*/	
	VXB_DEVICE_ID pDev;
	ALTERA_WDT_TIMER * pCtrl;
    struct vxbTimerFunctionality * pTimerFunc;
	UINT32 count = 0;	

	/*
	pDev = vxbInstByNameFind("altWdt", 0);
	
	pCtrl = pDev->pDrvCtrl;	
	pTimerFunc = &pCtrl->timerFunc;
	*/
	pTimerFunc = (struct vxbTimerFunctionality *)vxbInstByNameFind("altWdt", 0);
	
	vxbAltSocGen5WdtEnable(pTimerFunc, 0xFFFFFFFF);
	
	vxbAltSocGen5WdtCountGet(pTimerFunc, &count);
	
	return count;
}

void test_wdt(int delay)
{
	int tmp = 0, tmp2 = 0 ;
	
	tmp = wdt_time_get();
	taskDelay(delay);
	tmp2 = wdt_time_get();
	
	printf("t1: %d -> ", tmp);
	printf("t2: %d (%d) \n\n", tmp2, (tmp2-tmp));

	return;
}

#endif

