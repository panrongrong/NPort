/* vxbAltSocGen5Timer.c - Altera SOC Gen 5 timer driver */

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
01b,16apr13,swu  Moved to hwif/timer folder and code clean.
01a,02may12,jrs  Adopted for use by the Altera SoC BSP, from ARM Versatile
                 Express A9x4 BSP.
*/

/*
DESCRIPTION

This is the vxbus compliant timer driver which implements the functionality
specific to the Altera Timer device.

One 32-bit timers provide pulses to the interrupt controller.

A device specific instance data structure (ALTERA_TIMER_DRV_CTRL), is
maintained within this driver, which holds pointer to device instance, number of
timers in the instance and timer functionality specific data structure
(ALTERA_TIMER_DATA).

A pointer to the allocated instance structure is stored in 'pInst->pDrvCtrl'
during vxbAltSocGen5TimerInstInit().

The driver implements all the vxbus driver specific initialization routines 
like vxbAltSocGen5TimerInstInit(), vxbAltSocGen5TimerInstInit2() and
vxbAltSocGen5TimerInstConnect().

vxbAltSocGen5TimerInstInit() allocates memory for timer instance data structure 
and fills the timer functionality specific parameters retrieved from hwconf.c.

vxbAltSocGen5TimerInstInit2() attempts to connect the driver to the vxbus 
interrupt mechanism using vxbIntConnect().

A method for methodId VXB_METHOD_TIMER_FUNC_GET is implemented in this driver
and is used by the Timer Abstraction Layer/application to retrieve the
characteristics of the timer device. A pointer to 'struct vxbTimerFunctionality'
is allocated by the Timer Abstraction Layer/application and pointer to this
structure is given as the parameter to this method.  The timer driver populates
this data structure with the features of the timer device.

EXTERNAL INTERFACE

The driver provides the standard vxbus external interface alteraTimerRegister().
This function registers the driver with the vxbus subsystem, and instances will
be created as needed.  Since alteraTimer is a processor local bus device, each
device instance must be specified in the hwconf.c file in a BSP.

Driver-specific resources are specified below (in the driver source file and in
the README file).

An example hwconf entry is shown below:

\cs
struct hcfResource alteraTimerDev0Resources[] = {
{ "regBase",    HCF_RES_INT, {(void *)ALTERA_TIMER0_BASE} },
{ "irq",        HCF_RES_INT, {(void *)INT_VEC_L4_TIMER0} },
{ "irqLevel",   HCF_RES_INT, {(void *)INT_LVL_L4_TIMER0} },
{ "minClkRate", HCF_RES_INT, {(void *)SYS_CLK_RATE_MIN } },
{ "maxClkRate", HCF_RES_INT, {(void *)SYS_CLK_RATE_MAX} },
{ "clkFreq",    HCF_RES_INT,   { (void *)HHP_SP_TIMER_CLK } }
};
\ce

Please refer following for more information on the timers controlled 
by this module,

    Synopsys DesignWare DW_apb_timers Databook
    Altera's internal HPS timer documentation

*/
 
/* includes */

#include <vxWorks.h>
#include <string.h>
#include <vxbTimerLib.h>
#include <logLib.h>
#include <hwif/util/hwMemLib.h>
#include <hwif/vxbus/hwConf.h>

/* defines */

#define ALTERA_TIMER_NAME         "altTimer"

#define ALTERA_TIMER_REG_WIDTH    16
#define ALTERA_TIMER_MAX_COUNTL   0xffff
#define ALTERA_TIMER_MAX_COUNTH   0xffff
#define ALTERA_TIMER_MAX_COUNT    (ALTERA_TIMER_MAX_COUNTL | \
                                  (UINT32)(ALTERA_TIMER_MAX_COUNTH << \
                                   ALTERA_TIMER_REG_WIDTH))

/* timer register offsets */

#define TIMER_STATUS            0x10 /* status of timer */
#define TIMER_CONTROL           0x08 /* controller register */
#define TIMER_LOAD_CNT          0x00 /* load count */ 
#define TIMER_CURR_VAL          0x04 /* current value */
#define TIMER_EOI               0x0C /* End of Interrupt */

/* Timer Control Register bit defination */

#define CTRL_EN_DISABLE         (0)
#define CTRL_EN_ENABLE          (1<<0)

#define CTRL_MODE_FREERUN       (0)
#define CTRL_MODE_PERIODIC      (1<<1)

#define CTRL_INT_NOT_MASK       (0)
#define CTRL_INT_MASKED         (1<<2)

/* Timer Status Register Bit defination */

#define TIMER_STATUS_INT        0x0     /* Interrupt Status */

/* We will not let the timer counter be set to anything lower that this value */

#define LOW_TIMER_THRESHOLD     2000     
#define DEFAULT_TICKS_PER_SECOND        60

/* typedefs */

/* altera timer data */

typedef struct alteraTimerData
    {
    VXB_DEVICE_ID                       pInst;
    UINT32                              timerVal;
    int                                 enabled;
    struct vxbTimerFunctionality        timerFunc;
    void                                (*pIsrFunc)(int);
    int                                 arg;
    spinlockIsr_t                       spinLock;
    } ALTERA_TIMER_DATA;

/* altera timer driver control */

typedef struct alteraTimerDrvCtrl
    {
    VXB_DEVICE_ID                       pInst;
    UINT32                              clkFreq;
    ALTERA_TIMER_DATA *                 pTimerData;
    void *                              regBase;
    void *                              handle;
    } ALTERA_TIMER_DRV_CTRL;

/* timer register low level access routines */

#define TIMER_BAR(p)    ((ALTERA_TIMER_DRV_CTRL *)(p)->pDrvCtrl)->regBase
#define TIMER_HANDLE(p) ((ALTERA_TIMER_DRV_CTRL *)(p)->pDrvCtrl)->handle

#ifdef ARMBE8
#    define SWAP32 vxbSwap32
#else
#    define SWAP32 
#endif /* ARMBE8 */

#undef CSR_READ_2
#define CSR_READ_2(pDev, addr)          \
        SWAP32 (vxbRead32(TIMER_HANDLE(pDev),   \
                  (UINT32 *)((char *)TIMER_BAR(pDev) + addr)))

#undef CSR_WRITE_2
#define CSR_WRITE_2(pDev, addr, data)   \
        vxbWrite32(TIMER_HANDLE(pDev),  \
                   (UINT32 *)((char *)TIMER_BAR(pDev) + addr), SWAP32 (data))

#undef CSR_SETBIT_2          
#define CSR_SETBIT_2(pDev, addr, data)  \
        CSR_WRITE_2(pDev, addr, CSR_READ_2(pDev, addr) | (data))

#undef CSR_CLRBIT_2
#define CSR_CLRBIT_2(pDev, addr, data)  \
        CSR_WRITE_2(pDev, addr, CSR_READ_2(pDev, addr) & ~(data))

/* forward declarations */

LOCAL void vxbAltSocGen5TimerInstInit(VXB_DEVICE_ID);
LOCAL void vxbAltSocGen5TimerInstInit2(VXB_DEVICE_ID);
LOCAL void vxbAltSocGen5TimerInstConnect(VXB_DEVICE_ID);
LOCAL void vxbAltSocGen5TimerDevInit(VXB_DEVICE_ID);
LOCAL STATUS vxbAltSocGen5TimerFuncGet (VXB_DEVICE_ID, 
                                        struct vxbTimerFunctionality **, int);
LOCAL STATUS vxbAltSocGen5TimerAllocate (VXB_DEVICE_ID pInst, UINT32 flags,
                                         void ** pCookie, UINT32 timerNo);
LOCAL STATUS vxbAltSocGen5TimerRelease (VXB_DEVICE_ID pInst, void * pCookie);
LOCAL STATUS vxbAltSocGen5TimerRolloverGet (void * pCookie, UINT32 * count);
LOCAL STATUS vxbAltSocGen5TimerCountGet (void * pCookie, UINT32 * count);
LOCAL STATUS vxbAltSocGen5TimerDisable (void * pCookie);
LOCAL STATUS vxbAltSocGen5TimerEnable (void * pCookie, UINT32 maxTimerCount);
LOCAL STATUS vxbAltSocGen5TimerISRSet (void * pCookie, void(*pFunc)(int), 
                                       int arg);
LOCAL void vxbAltSocGen5TimerIsr(VXB_DEVICE_ID);

LOCAL struct drvBusFuncs alteraTimerFuncs =
    {
    vxbAltSocGen5TimerInstInit,        /* devInstanceInit */
    vxbAltSocGen5TimerInstInit2,       /* devInstanceInit2 */
    vxbAltSocGen5TimerInstConnect      /* devConnect */
    };

LOCAL device_method_t alteraTimerMethods[] =
    {
    DEVMETHOD (vxbTimerFuncGet, vxbAltSocGen5TimerFuncGet),
    DEVMETHOD_END
    };

struct vxbDevRegInfo alteraTimerDevRegistration =
    {
    NULL,               /* pNext */
    VXB_DEVID_DEVICE,   /* devID */
    VXB_BUSID_PLB,      /* busID = PLB */
    VXB_VER_4_0_0,      /* vxbVersion */
    ALTERA_TIMER_NAME,  /* drvName */
    &alteraTimerFuncs,  /* pDrvBusFuncs */
    NULL,               /* pMethods */
    NULL                /* devProbe */
    };

/*******************************************************************************
*
* vxbAltSocGen5TimerRegister - register Altera timer driver
*
* This routine registers the Altera timer driver with the vxbus subsystem.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void vxbAltSocGen5TimerRegister (void)
    {
    vxbDevRegister (&alteraTimerDevRegistration);
    }

/*******************************************************************************
*
* vxbAltSocGen5TimerInstInit - first level initialization routine of timer device
*
* This routine performs the first level initialization of the timer device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxbAltSocGen5TimerInstInit
    (
    VXB_DEVICE_ID pInst
    )
    {
    ALTERA_TIMER_DRV_CTRL * pDrvCtrl = NULL;
    ALTERA_TIMER_DATA *pTimerData = NULL;

    struct vxbTimerFunctionality * pTimerFunc = NULL;
    HCF_DEVICE * pHcf = NULL;
    /* UINT32 numTimers = 0; */
	
    FUNCPTR clkFunc = NULL; /* jc*/
    
    pDrvCtrl = (ALTERA_TIMER_DRV_CTRL *) hwMemAlloc (
                sizeof (ALTERA_TIMER_DRV_CTRL));
    
    if (pDrvCtrl == NULL)
        return;
    
    /* 
     * Set up pointers so that the drv ctrl structure points to the inst
     * and the inst points to the drv ctrl structure
     */
    
    pDrvCtrl->pInst = pInst;
    pInst->pDrvCtrl = pDrvCtrl;

    /* get a pointer to the HCF for this device */
	
    pHcf = hcfDeviceGet (pInst);
    if (pHcf == NULL)
        {
#ifndef _VXBUS_BASIC_HWMEMLIB        
        hwMemFree ((char *) pDrvCtrl);
#endif /* _VXBUS_BASIC_HWMEMLIB */
        return;
        }
      
    /*
     * resourceDesc {
     * The clkFreq resource specifies the sytem
     * clock frequency retrieving routine address. }
     */
    /*
	*/

    /* get the clock frequency */   
#if 0
    if (devResourceGet(pHcf, "clkFreq", HCF_RES_INT, 
                       (void *) &pDrvCtrl->clkFreq) != OK)
        {
#ifndef _VXBUS_BASIC_HWMEMLIB         
        hwMemFree ((char *) pDrvCtrl);
#endif /* _VXBUS_BASIC_HWMEMLIB */        
        return;
        }
#else
	(void)devResourceGet (pHcf, "clkFreq", HCF_RES_ADDR, (void *)&clkFunc);
	if (clkFunc)
	{
		/*logMsg("clkFunc = %x\n",&clkFunc,2,3,4,5,6);*/
		pDrvCtrl->clkFreq = (*clkFunc)();
	}
	else
	{
		pDrvCtrl->clkFreq = FMQL_TTC_CLK;
	}
#endif

    pDrvCtrl->regBase = pInst->pRegBase[0];   /* store the base address */
    vxbRegMap(pInst, 0, &pDrvCtrl->handle);   /* map the window */

    pTimerData = (ALTERA_TIMER_DATA *) hwMemAlloc (sizeof (ALTERA_TIMER_DATA));
    if (pTimerData == NULL)
        {
#ifndef _VXBUS_BASIC_HWMEMLIB          
        hwMemFree ((char *) pDrvCtrl);
#endif /* _VXBUS_BASIC_HWMEMLIB */         
        return;
        }
    
    pDrvCtrl->pTimerData = pTimerData;

    /* make sure the data value we were just allocated is initialized to 0 */
    memset ((void *) pTimerData, 0, sizeof (ALTERA_TIMER_DATA));
    
    /* set up the timer data */
    pTimerData = &pDrvCtrl->pTimerData [0];

    pTimerData->pInst = pInst;

    /* set up the timer function */
    pTimerFunc = &pTimerData->timerFunc;

    /* copy and null terminate the name of the timer */
    strncpy (pTimerFunc->timerName, ALTERA_TIMER_NAME, MAX_DRV_NAME_LEN);
    pTimerFunc->timerName [MAX_DRV_NAME_LEN] = '\0';
  
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
        hwMemFree ((char *) pDrvCtrl);
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
        hwMemFree ((char *) pDrvCtrl);
#endif /* _VXBUS_BASIC_HWMEMLIB */        
        return;
        }
    pTimerFunc->clkFrequency = pDrvCtrl->clkFreq;

    pTimerFunc->features = VXB_TIMER_CAN_INTERRUPT
            | VXB_TIMER_INTERMEDIATE_COUNT | VXB_TIMER_SIZE_32
            | VXB_TIMER_AUTO_RELOAD;

    /* calculate the maximum possible timer rollover period */    
    pTimerFunc->rolloverPeriod = ALTERA_TIMER_MAX_COUNT / pDrvCtrl->clkFreq;

    /* update default system ticks per second */    
    pTimerFunc->ticksPerSecond = DEFAULT_TICKS_PER_SECOND;

	
    pTimerFunc->timerAllocate = vxbAltSocGen5TimerAllocate;
    pTimerFunc->timerRelease = vxbAltSocGen5TimerRelease;
	
    pTimerFunc->timerRolloverGet = vxbAltSocGen5TimerRolloverGet;
    pTimerFunc->timerCountGet = vxbAltSocGen5TimerCountGet;
	
    pTimerFunc->timerDisable = vxbAltSocGen5TimerDisable;
    pTimerFunc->timerEnable = vxbAltSocGen5TimerEnable;
	
    pTimerFunc->timerISRSet = vxbAltSocGen5TimerISRSet;
	
    pTimerData->timerVal = ALTERA_TIMER_MAX_COUNT;

    /* initialize the spinlock */
    SPIN_LOCK_ISR_INIT (&pTimerData->spinLock, 0);

    /* publish methods */
    pInst->pMethods = alteraTimerMethods;
    
    /* per-device init */    
    vxbAltSocGen5TimerDevInit (pInst);
    }

/*******************************************************************************
*
* vxbAltSocGen5TimerInstInit2 - second level initialization routine of timer device
*
* This routine performs the second level initialization of the timer device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxbAltSocGen5TimerInstInit2
    (
    VXB_DEVICE_ID pDev          /* The device */
    )
{
    /* connect the ISR for the timer */    
    if (vxbIntConnect (pDev, 0, vxbAltSocGen5TimerIsr, pDev) != OK)
        pDev->pDrvCtrl = NULL;
}

/*******************************************************************************
*
* vxbAltSocGen5TimerInstConnect - third level initialization routine of timer device
*
* This routine performs the third level initialization of the timer device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxbAltSocGen5TimerInstConnect
    (
    VXB_DEVICE_ID pInst    /* The device */
    )
    {
    /* Nothing to do at this level of initialization */
    
    return;
    }

/*******************************************************************************
*
* vxbAltSocGen5TimerDevInit - Altera timer per device specific initialization
*
* This routine performs per device specific initialization of the timer device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxbAltSocGen5TimerDevInit
    (
    VXB_DEVICE_ID pInst   /* The device */
    )
    {
   
    if (pInst == NULL)
        return;
    
    /* 
     * set the timer control register to disable the timer, periodic mode,
     * interrupts masked, pulse width modulator disabled
     */
    
    CSR_WRITE_2(pInst, TIMER_CONTROL, CTRL_EN_DISABLE | CTRL_MODE_PERIODIC |
                CTRL_INT_MASKED);
        
    /* set the value of the timer's count down value to zero */
    
    CSR_WRITE_2(pInst, TIMER_LOAD_CNT, 0);
    
    /* read the timer's end-of-interrupt register, clearing any interrupt */
    
    CSR_READ_2(pInst, TIMER_EOI);
    }

/*******************************************************************************
*
* vxbAltSocGen5TimerFuncGet - method to retrieve the timer functionality
*
* This routine is the driver method used
*
* RETURNS: OK or ERROR if functionality is not retrieved
*
* ERRNO: N/A
*/

LOCAL STATUS vxbAltSocGen5TimerFuncGet
    (
    VXB_DEVICE_ID pInst,                         /* The device */
    struct vxbTimerFunctionality ** ppTimerFunc, /* An output, A pointer to a timer function */
    int timerNo                                  /* The timer number */
    )
    {
    ALTERA_TIMER_DRV_CTRL * pDrvCtrl = NULL;
    ALTERA_TIMER_DATA * pTimerData = NULL;
    
    /* validate the parameters */

    if ((pInst == NULL) || (ppTimerFunc == NULL) || (timerNo != 0))
        return ERROR;
    
    pDrvCtrl = (ALTERA_TIMER_DRV_CTRL *) pInst->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return ERROR;

    pTimerData = &pDrvCtrl->pTimerData[timerNo];

    /* 
     * set the passed in address of the timer function pointer to the value 
     * for this timer 
     */
    
    *ppTimerFunc = &(pTimerData->timerFunc);
    
    return OK;
    }

/*******************************************************************************
*
* vxbAltSocGen5TimerAllocate - allocate resources for a timer
*
* This routine allocates resources for a timer by the Timer Abstraction Layer.
*
* RETURNS: OK or ERROR if timer allocation fails
*
* ERRNO: N/A
*/

LOCAL STATUS vxbAltSocGen5TimerAllocate
    (
    VXB_DEVICE_ID pInst,   /* The device */
    UINT32 flags,          /* Flags for the timer, currently unused */
    void ** pCookie,       /* An output, a handle to a cookie containing timer information */
    UINT32 timerNo         /* The timer number */
    )
    {
    ALTERA_TIMER_DRV_CTRL * pDrvCtrl;
    ALTERA_TIMER_DATA * pTimerData;

    /* validate the parameters */

    if ((pInst == NULL) || (pCookie == NULL) || (timerNo != 0))
        return ERROR;
    
    pDrvCtrl = (ALTERA_TIMER_DRV_CTRL *) pInst->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return ERROR;
    
    pTimerData = &pDrvCtrl->pTimerData[timerNo];

    /* take the spinlock for the exclusive access to the timer resources */
    SPIN_LOCK_ISR_TAKE (&pTimerData->spinLock);
    
    /* check to see if the timer is already allocated */
    if (pTimerData->timerFunc.allocated)
        {
        
        /* release the spinlock */
        SPIN_LOCK_ISR_GIVE (&pTimerData->spinLock);
        return ERROR;
        }
    
    /* store the timer information in the pCookie */    
    *pCookie = pTimerData;

    /* set the timer allocated flag */
    pTimerData->timerFunc.allocated = TRUE;
    
    /* release the spinlock */
    SPIN_LOCK_ISR_GIVE (&pTimerData->spinLock);
    
    return OK;
    }

/*******************************************************************************
*
* vxbAltSocGen5TimerRelease - release the timer resource
*
* This routine releases the resources allocated for a timer device.
*
* RETURNS: OK or ERROR if the parameter is invalid
*
* ERRNO: N/A
*/

LOCAL STATUS vxbAltSocGen5TimerRelease
    (
    VXB_DEVICE_ID pInst,   /* The device */
    void * pCookie         /* A pointer to timer information for this timer */
    )
    {
    ALTERA_TIMER_DRV_CTRL * pDrvCtrl;
    ALTERA_TIMER_DATA * pTimerData;

    /* validate the parameters */
    if ((pInst == NULL) || (pCookie == NULL))
        return ERROR;

    pDrvCtrl = (ALTERA_TIMER_DRV_CTRL *) pInst->pDrvCtrl;
    pTimerData = (ALTERA_TIMER_DATA *) pCookie;

    /* validate pInst and check to see if the timer is allocated */
    if ((pDrvCtrl->pInst != pInst) || (!pTimerData->timerFunc.allocated))
        return ERROR;
    
    /* take the spinlock for the exclusive access to the timer resources */
    SPIN_LOCK_ISR_TAKE (&pTimerData->spinLock);
    
    /* disable the timer */    
    if (vxbAltSocGen5TimerDisable(pCookie) != OK)
        {
        /* release the spinlock */
        SPIN_LOCK_ISR_GIVE (&pTimerData->spinLock);
        return ERROR;
        }
        
    pTimerData->pIsrFunc = NULL;
    pTimerData->arg = 0;

    /* reset the timer allocated flag */
    pTimerData->timerFunc.allocated = FALSE;
    
    /* release the spinlock */
    SPIN_LOCK_ISR_GIVE (&pTimerData->spinLock);
    
    return OK;
    }

/*******************************************************************************
*
* vxbAltSocGen5TimerRolloverGet - retrieve the maximum value of the counter
*
* This routine retrieves the maximum value of the counter.
*
* RETURNS: OK
*
* ERRNO: N/A
*/

LOCAL STATUS vxbAltSocGen5TimerRolloverGet
    (
    void * pCookie,   /* Information about this timer */
    UINT32 * count    /* An output, the current rollover value for this timer */
    )
    {
    ALTERA_TIMER_DATA * pTimerData;

    if ((pCookie == NULL) || (count == NULL))
        return ERROR;
    
    pTimerData = (ALTERA_TIMER_DATA *) pCookie;

    /* store the counter value */
    *count = pTimerData->timerVal;

    return OK;
    }

/*******************************************************************************
*
* vxbAltSocGen5TimerCountGet - retrieve the current value of the counter
*
* This routine retrieves the current value of the counter.
*
* RETURNS: OK or ERROR if the parameter is invalid
*
* ERRNO: N/A
*/

LOCAL STATUS vxbAltSocGen5TimerCountGet
    (
    void * pCookie,   /* Information about this timer */
    UINT32 * count    /* An output, the current count for this timer */
    )
    {
    ALTERA_TIMER_DATA * pTimerData;
    UINT32 snap;

    if ((pCookie == NULL) || (count == NULL))
        return ERROR;
    
    pTimerData = (ALTERA_TIMER_DATA *) pCookie;

    /* take the spinlock for the exclusive access to the timer resources */
    SPIN_LOCK_ISR_TAKE (&pTimerData->spinLock);
    
    /* read the current value of the timer */    
    snap = CSR_READ_2(pTimerData->pInst,TIMER_CURR_VAL);
    
    /* 
     * Set the result to the max value of the timer minus the current value,
     * which will be the number of decrements of the counter that have been 
     * made since the previous "load" of the counter
     */    
    *count = pTimerData->timerVal - snap;
    
    /* release the spinlock */
    SPIN_LOCK_ISR_GIVE (&pTimerData->spinLock);
    
    return OK;
    }

/*******************************************************************************
*
* vxbAltSocGen5TimerDisable - disable the timer
*
* This routine disables the timer.
*
* RETURNS: OK
*
* ERRNO: N/A
*/

LOCAL STATUS vxbAltSocGen5TimerDisable
    (
    void * pCookie   /* Information about this timer */
    )
    {
    ALTERA_TIMER_DATA * pTimerData;

    if (pCookie == NULL)
        return ERROR;
    
    pTimerData = (ALTERA_TIMER_DATA *) pCookie;

    /* clear the enable bit to disable the timer */
    
    CSR_CLRBIT_2(pTimerData->pInst, TIMER_CONTROL, CTRL_EN_ENABLE);

    /* mask interrupts by setting the interrupt mask bit of the timer control register */
    
    CSR_SETBIT_2(pTimerData->pInst, TIMER_CONTROL, CTRL_INT_MASKED);

    /* clear interrupt status bits of timer by reading the EOI register */
    
    CSR_READ_2(pTimerData->pInst, TIMER_EOI);

    /* 
     * Read the timer status register to get the interrupt status, this is
     * probably not needed but we do it to ensure that we have zeroed out the 
     * interrupt status 
     */
    
    CSR_READ_2(pTimerData->pInst, TIMER_STATUS);

    /* set the enabled flag to false in the local timer data structure */
    
    pTimerData->enabled = FALSE;
    
    return OK;
    }

/*******************************************************************************
*
* vxbAltSocGen5TimerEnable - enable the timer
*
* This routine updates the maximum count value and enables the timer.
*
* RETURNS: OK or ERROR if timer is not enabled
*
* ERRNO: N/A
*/

LOCAL STATUS vxbAltSocGen5TimerEnable
    (
    void * pCookie,        /* Information about this timer */
    UINT32 maxTimerCount   /* The desired maximum timer count for this timer */
    )
    {
    VXB_DEVICE_ID       pDev;
    ALTERA_TIMER_DATA * pTimerData;
    struct vxbTimerFunctionality * pTF;
    
    /* validate the parameters */    
    if ((pCookie == NULL) || (maxTimerCount == 0))
        return ERROR;

    pTimerData = (ALTERA_TIMER_DATA *) pCookie;    
    pDev = pTimerData->pInst;    
    pTF = &pTimerData->timerFunc;

    /* take the spinlock for the exclusive access to the timer resources */

    SPIN_LOCK_ISR_TAKE (&pTimerData->spinLock);
    
    /* if the timer is already running, stop it before making adjustments */
    if (pTimerData->enabled)
        {
        if (vxbAltSocGen5TimerDisable (pCookie) != OK)
            {
            /* release the spinlock */
            SPIN_LOCK_ISR_GIVE (&pTimerData->spinLock);
            return ERROR;
            }
        }

    /* read the Timer EOI register to clear the timer interrupt status */    
    CSR_READ_2(pTimerData->pInst, TIMER_EOI);
    
    /* read the Timer status register, probably not needed but just in case */    
    CSR_READ_2(pTimerData->pInst, TIMER_STATUS);

    /* 
     * We need to give the timer ISR some time to run, complete, and allow the OS
     * to do someting before we interrupt again.  We have a safeguard against setting
     * the timer count too low.  This number was arbitrarily chosen based on experiementation,
     * but it might need to be raised if the ISR callback function takes too long.
     * We think we may have a problem (at least in VT) where VT dies if the timer counts
     * down to zero before the previous timer ISR has a chance to complete.
     */    
    if (maxTimerCount < LOW_TIMER_THRESHOLD)
        {
        maxTimerCount = LOW_TIMER_THRESHOLD;
        }

    /* save the clock speed */    
    pTimerData->timerVal = maxTimerCount;

    /* update the actual ticks per second */    
    pTF->ticksPerSecond = pTF->clkFrequency / maxTimerCount;
    
    /* update ​rollover​Period */    
    pTF->rolloverPeriod = pTimerData->timerVal / (pTF->clkFrequency);
    
    /* write the timer load count with the initial count down value */    
    CSR_WRITE_2(pTimerData->pInst, TIMER_LOAD_CNT, maxTimerCount);

    /* start the timer and enable interrupt */    
    CSR_WRITE_2(pTimerData->pInst, TIMER_CONTROL, CTRL_EN_ENABLE | 
                CTRL_MODE_PERIODIC | CTRL_INT_NOT_MASK);

    if (vxbIntEnable (pDev, 0, vxbAltSocGen5TimerIsr, pDev) != OK)
        {
        /* release the spinlock */

        SPIN_LOCK_ISR_GIVE (&pTimerData->spinLock);
        return ERROR;
        }
    
    /* set the enabled flag */    
    pTimerData->enabled = TRUE;

    /* release the spinlock */
    SPIN_LOCK_ISR_GIVE (&pTimerData->spinLock);
    
    return OK;
}

/*******************************************************************************
*
* vxbAltSocGen5TimerISRSet - set a function to be called on the timer interrupt
*
* This routine sets a function to be called on the timer interrupt.
*
* RETURNS: OK
*
* ERRNO: N/A
*/

LOCAL STATUS vxbAltSocGen5TimerISRSet
    (
    void *      pCookie,        /* Information about this timer */
    void        (*pFunc)(int),  /* A function to be called upon timer interrupt */
    int         arg             /* An argument to be provided to that function upon interrupt */
    )
    {
    ALTERA_TIMER_DATA * pTimerData;
    
    if ((pCookie == NULL) || (pFunc == NULL))
        return ERROR;
    
    pTimerData = (ALTERA_TIMER_DATA *) pCookie;
    
    /* take the spinlock for the exclusive access to the timer resources */
    SPIN_LOCK_ISR_TAKE (&pTimerData->spinLock);
    
    /* store the interrupt routine and argument information */    
    pTimerData->pIsrFunc = pFunc;
    pTimerData->arg = arg;
    
    /* release the spinlock */
    SPIN_LOCK_ISR_GIVE (&pTimerData->spinLock);
    
    return OK;
    }

/*******************************************************************************
*
* vxbAltSocGen5TimerIsr - Alter interal timer interrupt handler
*
* This routine handles the timer interrupt.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxbAltSocGen5TimerIsr
    (
    VXB_DEVICE_ID        pDev   /* Information about the driver */
    )
{
    ALTERA_TIMER_DATA * pTimerData = NULL;
    ALTERA_TIMER_DRV_CTRL * pDrvCtrl = NULL;
    
    if (pDev == NULL)
        return;
    
    pDrvCtrl = pDev->pDrvCtrl;
    
    pTimerData = (ALTERA_TIMER_DATA *)&pDrvCtrl->pTimerData[0];
  
    /* clear the event that created the interrupt */
    
    CSR_READ_2(pTimerData->pInst, TIMER_EOI);
    
    /* call the ISR hander if one is registered */
    
    if (pTimerData->enabled && pTimerData->pIsrFunc != NULL)
    {
        (*(pTimerData->pIsrFunc))(pTimerData->arg);
    }
}


