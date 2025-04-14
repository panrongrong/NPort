/* vxbFdtArmv7GenTimer.c - ARMV7 Generic Timer driver for VxBus */

/*
 * Copyright (c) 2014-2016 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
10Jun19,jc   merge from vx7.0 to vx6.9

03mar16,dee  add vxbIntLib.h to resolve implicit reference
02feb16,sye  added support for LS2085A (US73102)
20may15,g_x  remove frequency limit for Keystone (US59534)
09oct14,yya  Created. (US47371)
*/

/*
DESCRIPTION

This module implements vxBus compliant timer driver for the ARMV7 Generic Timer.
The number of timers provided by an implementation of the Generic Timer depends
on whether the implementation includes the Security Extensions and the
Virtualization Extensions, Example, if Virtualization Extensions implemented,
the implementation provides:
    . A Non-secure PL1 physical timer.
    . A Secure PL1 physical timer.
    . A Non-secure PL2 physical timer.
    . A virtual timer.
By default, this driver uses the virtual timer, and if it is not available,
the physical timer will be used.

To add the driver to the vxWorks image, add the following component to the
kernel configuration.

\cs
vxprj component add DRV_ARMV7_GEN_TIMER
\ce

The generic timer device should be bound to a device tree node which requires
below properties:

\cs
compatible:     Specify the programming model for the device.
                It should be set to "arm,armv7-gen-timer" and is used
                by vxbus GEN2 for device driver selection.

clock-frequency: Optional, specify the clock frequency of the timers's clock 
                source in HZ if not getting from CNTFRQ.

double-read:    Optional, specify whether to read the timer's CNT registers
                twice. Some chips may have bug that it's necessary to read the
                CNT registers twice any time it is read. If it's not specified,
                this property is 0 by default which means no need to read twice.

interrupts:     Specify interrupt vectors of the interrupts that are generated
                by this device.

interrupt-parent: This property is available to define an interrupt
                parent. if it is missing from a device, it's interrupt parent 
                is assumed to be its device tree parent.
\ce

Below is an example:

\cs
        timer0
            {
            compatible = "arm,armv7-gen-timer";
            interrupts = <29 1 4        /@ secure physical timer PPI @/
                          30 1 4        /@ non-secure physical timer PPI @/
                          27 1 4        /@ virtual timer PPI @/
                          26 1 4>;      /@ hypervisor timer PPI @/
            interrupt-parent = <&intc>;
            };
\ce

INCLUDE FILES: vxBus.h vxbTimerLib.h string.h vxbFdtLib.h
*/

/* includes */

#include <vxWorks.h>
#include <string.h>
#include <intLib.h>
#include <vxCpuLib.h>
#include <vsbConfig.h>

#if 0  /* change by jc 2019-06-10 */
#include <hwif/vxBus.h>
#include <subsys/int/vxbIntLib.h>
#include <subsys/timer/vxbTimerLib.h>
#include <subsys/clk/vxbClkLib.h>
#include <hwif/buslib/vxbFdtLib.h>
#else
#include <vxbTimerLib.h>
#include <hwif/util/hwMemLib.h>
#include <hwif/vxbus/hwConf.h>
#include <string.h>
#include <../src/hwif/h/vxbus/vxbAccess.h>
#endif

#ifdef _WRS_CONFIG_SMP
#include <cpuset.h>
#include <taskLib.h>
#include <private/cpcLibP.h>
#endif /* _WRS_CONFIG_SMP */

/* defines */

#ifdef ARMBE8
#define SWAP32 vxbSwap32
#else
#define SWAP32 
#endif /* ARMBE8 */

/* debug macro */
#if 0  /* change by jc 2019-06-10 */
#undef  GEN_TIMER_DBG_ON
#else
#define  GEN_TIMER_DBG_ON
#endif

#ifdef  GEN_TIMER_DBG_ON

/* turning local symbols into global symbols */
#ifdef  LOCAL
#undef  LOCAL
#define LOCAL
#endif

#include <private/kwriteLibP.h>    /* _func_kprintf */
#define GEN_TIMER_DBG_OFF          0x00000000
#define GEN_TIMER_DBG_ISR          0x00000001
#define GEN_TIMER_DBG_ERR          0x00000002
#define GEN_TIMER_DBG_INFO         0x00000004
#define GEN_TIMER_DBG_ALL          0xffffffff

LOCAL UINT32 armv7GenTimerDbgMask = GEN_TIMER_DBG_ALL;

#define GEN_TIMER_DBG(mask, ...)                        \
    do                                                  \
        {                                               \
        if ((armv7GenTimerDbgMask & (mask)) ||          \
            ((mask) == GEN_TIMER_DBG_ALL))              \
            {                                           \
            if (_func_kprintf != NULL)                  \
                {                                       \
                (* _func_kprintf)(__VA_ARGS__);         \
                }                                       \
            }                                           \
        }                                               \
    while ((FALSE))
#else
#define GEN_TIMER_DBG(...)
#endif  /* GEN_TIMER_DBG_ON */

#undef TIMERFUNC_TO_TIMERDATA
#define TIMERFUNC_TO_TIMERDATA(pTimerFunc)                      \
                (ARMV7_GEN_TIMER_DRV_CTRL *)((ULONG)(pTimerFunc) -  \
                OFFSET (ARMV7_GEN_TIMER_DRV_CTRL, timerFunc))
                

#define ARMV7_GEN_TIMER_NAME                    "gtcTimer"  /* armv7GenTimer*/

#define ARM_GEN_TIMER_MAX_COUNT                 0xffffffff
#define ARM_GEN_TIMER_MAX_COUNT64               0xffffffffffffffffLLU
#define ARM_GEN_TIMER_DEFAULT_TPS               60

#define ARM_GEN_TIMER_DEFAULT_MIN_FREQ          10
#define ARM_GEN_TIMER_DEFAULT_MAX_FREQ          5000

#define ARM_GEN_TIMER_MAX_CLK_FREQ              50000000
#define ARM_GEN_TIMER_MIN_CLK_FREQ              1000000

#define ARMV7_GEN_TIMER_MAX_COUNT               (ARM_GEN_TIMER_MAX_COUNT)  /* 0x7fff_ffff */
#define ARMV7_GEN_TIMER_DEFAULT_TPS             60

#define ARMV7_GEN_TIMER_DEFAULT_MIN_FREQ        10
#define ARMV7_GEN_TIMER_DEFAULT_MAX_FREQ        5000

#define ARMV7_GEN_TIMER_MAX_CLK_FREQ            50000000
#define ARMV7_GEN_TIMER_MIN_CLK_FREQ            1000000

#define GEN_TIMER_REG_CTL                       0
#define GEN_TIMER_REG_TVAL                      1

#define GEN_TIMER_CTRL_ENABLE                   (1 << 0)
#define GEN_TIMER_CTRL_IMASK                    (1 << 1)
#define GEN_TIMER_CTRL_ISTATUS                  (1 << 2)

#define GEN_TIMER_SECURE_PHY_PPI                (0)
#define GEN_TIMER_NON_SECURE_PHY_PPI            (1)
#define GEN_TIMER_VIRTURE_PPI                   (2)
#define GEN_TIMER_HYP_PPI                       (3)
#define GEN_TIMER_MAX_PPI                       (4)


#define DEFAULT_TICKS_PER_SECOND                (60)


/* typedefs */

/* structure to store the timer information */

/* altera timer data */
typedef struct armv7GenTimerData
{
    VXB_DEVICE_ID                       pInst;
	
    UINT32                              timerVal;
    int                                 enabled;
	
    struct vxbTimerFunctionality        timerFunc;
    void                                (*pIsrFunc)(int);
	
    int                                 arg;
    spinlockIsr_t                       spinLock;
	
    UINT64              maxCount;
    UINT64              startCount;
    UINT64              cmpCount;
} ARMV7_GEN_TIMER_DATA;

/* altera timer driver control */

typedef struct armv7GenTimerDrvCtrl
{
    VXB_DEVICE_ID                       pInst;
	
    UINT32                              clkFreq;
	
    ARMV7_GEN_TIMER_DATA *                 pTimerData;
	
    void *                              regBase;
    void *                              handle;
	
    UINT32              flags;
    BOOL                isEnabled;	
	
    BOOL                doubleRead;
    BOOL                isAutoReload;
} ARMV7_GEN_TIMER_DRV_CTRL;


/* forward declarations */
LOCAL void armv7GenTimerInstInit(VXB_DEVICE_ID);
LOCAL void armv7GenTimerInstInit2(VXB_DEVICE_ID);
LOCAL void armv7GenTimerInstConnect(VXB_DEVICE_ID);

LOCAL void armv7GenTimerDevInit(VXB_DEVICE_ID);
LOCAL STATUS armv7GenTimerFuncGet (VXB_DEVICE_ID, 
                                        struct vxbTimerFunctionality **, int);

LOCAL STATUS armv7GenTimerAllocate (VXB_DEVICE_ID pInst, UINT32 flags,
                                         void ** pCookie, UINT32 timerNo);

LOCAL STATUS armv7GenTimerRelease (VXB_DEVICE_ID pInst, void * pCookie);

LOCAL STATUS armv7GenTimerRolloverGet (void * pCookie, UINT32 * count);
LOCAL STATUS armv7GenTimerCountGet (void * pCookie, UINT32 * count);
LOCAL STATUS armv7GenTimerRolloverGet64 (void * pCookie, UINT64 * count);
LOCAL STATUS armv7GenTimerCountGet64 (void * pCookie, UINT64 * count);

LOCAL STATUS armv7GenTimerDisable (void * pCookie);

LOCAL STATUS armv7GenTimerEnable (void * pCookie, UINT32 maxTimerCount);
LOCAL STATUS armv7GenTimerEnable64 (void * pCookie, UINT64 maxTimerCount);

LOCAL STATUS armv7GenTimerISRSet (void * pCookie, void(*pFunc)(int), int arg);

LOCAL void armv7GenTimerIsr(VXB_DEVICE_ID);

/* 
 * The inline assembly to access generic timer on c14 follows diab's inline
 * assembly syntax, so diab compiler must be used.
 */

#if 1 /* copy from vx7 by jc 2019-06-11 */

__asm volatile UINT32 __inline__GetCntFreq (void)
{
! "r0"
    mrc p15, 0, r0, c14, c0, 0
}

__asm volatile UINT64 __inline__GetVirtTimerCnt (void)
{
! "r0", "r1"
    .word 0xec510f1e /* mrrc p15, 1, r0, r1, c14 */
}

__asm volatile UINT64 __inline__GetVirtTimerCmp (void)
{
! "r0", "r1"
    .word 0xec510f3e /* mrrc p15, 3, r0, r1, c14 */
}

__asm volatile UINT32 __inline__GetVirtTimerValue (void)
{
! "r0"
    mrc p15, 0, r0, c14, c3, 0
}

__asm volatile UINT32 __inline__GetVirtTimerCtrl (void)
{
! "r0"
    mrc p15, 0, r0, c14, c3, 1
}

__asm volatile void __inline__SetVirtTimerCmp (UINT32 val_low, UINT32 val_hi)
{
% reg val_low, val_hi
! "r0", "r1"
    mov r0, val_low  /* the compiler needs the GPRs of mcrr to be consecutive */
    mov r1, val_hi   /* otherwise it reports error */
    .word 0xec410f3e /* mcrr p15, 3, r0, r1, c14 */
}

__asm volatile void __inline__SetVirtTimerValue (UINT32 val)
{
% reg val
! "r0"
    mcr p15, 0, val, c14, c3, 0
}

__asm volatile void __inline__SetVirtTimerCtrl (UINT32 val)
{
% reg val
! "r0"
    mcr p15, 0, val, c14, c3, 1
}

__asm volatile UINT64 __inline__GetPhyTimerCnt (void)
{
! "r0", "r1"
    .word 0xec510f0e /* mrrc p15, 0, r0, r1, c14 */
}

__asm volatile UINT64 __inline__GetPhyTimerCmp (void)
{
! "r0", "r1"
    .word 0xec510f2e /* mrrc p15, 2, r0, r1, c14 */
}

__asm volatile UINT32 __inline__GetPhyTimerValue (void)
{
! "r0"
    mrc p15, 0, r0, c14, c2, 0
}

__asm volatile UINT32 __inline__GetPhyTimerCtrl (void)
{
! "r0"
    mrc p15, 0, r0, c14, c2, 1
}

__asm volatile void __inline__SetPhyTimerCmp (UINT32 val_low, UINT32 val_hi)
{
% reg val_low, val_hi
! "r0", "r1"
    mov r0, val_low  /* the compiler needs the GPRs of mcrr to be consecutive */
    mov r1, val_hi   /* otherwise it reports error */
    .word 0xec410f2e /* mcrr p15, 2, r0, r1, c14 */
}

__asm volatile void __inline__SetPhyTimerValue (UINT32 val)
{
% reg val
! "r0"
    mcr p15, 0, val, c14, c2, 0
}

__asm volatile void __inline__SetPhyTimerCtrl (UINT32 val)
{
% reg val
! "r0"
    mcr p15, 0, val, c14, c2, 1
}

#endif



/*******************************************************************************
*
* armv7GenTimerWriteReg - write the timer register
*
* This routine writes value to the generic timer register.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

static void armv7GenTimerWriteReg
    (
    ARMV7_GEN_TIMER_DRV_CTRL *  pDrvCtrl,
    UINT32                  reg,
    UINT32                  val
    )
{
    if (reg == GEN_TIMER_REG_CTL)
    {
        __inline__SetPhyTimerCtrl (val);
    }
    else
    {
        __inline__SetPhyTimerValue (val);
    }

    WRS_ASM ("ISB");
}

/*******************************************************************************
*
* armv7GenTimerReadReg - read the timer register
*
* This routine reads the generic timer register value.
*
* RETURNS: timer register value.
*
* ERRNO: N/A
*/

static UINT32 armv7GenTimerReadReg
    (
    ARMV7_GEN_TIMER_DRV_CTRL *  pDrvCtrl,
    UINT32                  reg
    )
{
    volatile UINT32         val1 = 0;
    volatile UINT32         val2 = 0;

    if (reg == GEN_TIMER_REG_CTL)
    {
        val1 = __inline__GetPhyTimerCtrl ();
    }
    else    /* timer value */
	{
        val1 = __inline__GetPhyTimerValue ();

		#if 0  /* change by jc 2019-06-11 */
        /*
         * Workaround for LS2085A chip errata: A-008585
         * Read until two consecutive reads return the same value
         * for CNTP_TVAL, CNTHP_TVAL and CNTV_TVAL.
         */

        if (pDrvCtrl->doubleRead)
        {
            while (val1 != val2)
            {
                val2 = val1;
                if (pDrvCtrl->isVirtual)
                {
                    val1 = __inline__GetVirtTimerValue ();
                }
                else
                {
                    val1 = __inline__GetPhyTimerValue ();
                }
           }
        }
		#endif		
    }
	
    return val1;
}

/*******************************************************************************
*
* armv7GenTimerGetTimerCnt - get the generic timer count
*
* This routine gets the generic timer counter register value.
*
* RETURNS: timer counter.
*
* ERRNO: N/A
*/

static UINT64 armv7GenTimerGetTimerCnt(ARMV7_GEN_TIMER_DRV_CTRL * pDrvCtrl)
{
    volatile UINT64         val1 = 0;
    volatile UINT64         val2 = 0;

    WRS_ASM ("ISB");

    val1 = __inline__GetPhyTimerCnt ();
	
#if 1  /* change by jc 2019-06-11 */
    /*
     * Workaround for LS2085A chip errata: A-008585
     * Read until two consecutive reads return the same value
     * for CNTPCT and CNTVCT.
     */
    if (pDrvCtrl->doubleRead)
    {
        while (val1 != val2)
        {
            val2 = val1;
            val1 = __inline__GetPhyTimerCnt ();
        }
    }
#endif

    return val1;
}

/*******************************************************************************
*
* armGenTimerSetTimerCmp - set value to generic timer compare register
*
* This function sets value to the generic timer compare register.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
static void armv7GenTimerSetTimerCmp
    (
    ARMV7_GEN_TIMER_DRV_CTRL *    pDrvCtrl,
    UINT64                  val
    )
{
    UINT32 val_low;
    UINT32 val_hi;

    val_low = (UINT32)val;
    val_hi  = (UINT32)(val >> 32);
	
	__inline__SetPhyTimerCmp (val_low, val_hi);
	/*__inline__SetPhyTimerCmp (val);*/
}


#if 1

/* 
structure to store the driver functions for vxBus 
*/
LOCAL struct drvBusFuncs armv7GenTimerDrvFuncs = 
{
    armv7GenTimerInstInit,      /* devInstanceInit */
    armv7GenTimerInstInit2,     /* devInstanceInit2 */
    armv7GenTimerInstConnect    /* devConnect */
};


/* 
instance methods 
*/
LOCAL device_method_t armv7GenTimerDrv_methods[] = 
{
    DEVMETHOD(vxbTimerFuncGet, armv7GenTimerFuncGet),
    DEVMETHOD_END
};


/* 
structure used for registration with vxbus 
*/
LOCAL struct vxbDevRegInfo armv7GenTimerDrvRegistration =
{
    NULL,                       /* pNext */
    VXB_DEVID_DEVICE,           /* devID */
    VXB_BUSID_PLB,              /* busID = PLB */
    VXB_VER_4_0_0,              /* vxbVersion */
    
    ARMV7_GEN_TIMER_NAME,       /* drvName: armv7GenTimer */
    
    &armv7GenTimerDrvFuncs,     /* pDrvBusFuncs */
    NULL,                       /* pMethods */
    NULL,                       /* devProbe */
    NULL                        /* pParamDefaults */
};


/*******************************************************************************
*
* armv7GenTimerDrvRegister - register armv7Gen timer driver
*
* This routine registers the armv7Gen timer driver with the vxBus subsystem.
*
* RETURNS: N/A
*
* ERRNO: none
*/
void armv7GenTimerDrvRegister (void)
{
    /* 
    call the vxBus routine to register the timer driver 
	*/
    /*vxbDevRegister (&armv7GenTimerDrvRegistration);*/
	
	/* logMsg("armv7GenTimerDrvRegister \n", 1,2,3,4,5,6); */
	printf("armv7GenTimerDrvRegister \n");
}


void armv7GenTimerDrvRegister2 (void)
{
    /* 
    call the vxBus routine to register the timer driver 
	*/
    vxbDevRegister (&armv7GenTimerDrvRegistration);
	
	/* logMsg("armv7GenTimerDrvRegister \n", 1,2,3,4,5,6); */
	printf("armv7GenTimerDrvRegister2 \n");
}

/*******************************************************************************
*
* armv7GenTimerInstInit - first level initialization routine of timer device
*
* This is the function called to perform the first level initialization of
* the timer device.
*
* NOTE:
*
* This routine is called early during system initialization, and
* *MUST NOT* make calls to OS facilities such as memory allocation
* and I/O.  It may, however, call hwMemAlloc().
*
* RETURNS: N/A
*
* ERRNO
*/
LOCAL void armv7GenTimerInstInit
    (
    VXB_DEVICE_ID pInst
    )
{
    ARMV7_GEN_TIMER_DRV_CTRL * pDrvCtrl = NULL;	
    ARMV7_GEN_TIMER_DATA *pTimerData = NULL;
	
    struct vxbTimerFunctionality * pTimerFunc = NULL;	
    HCF_DEVICE * pHcf = NULL;

    /* allocate the memory for the timer structure */
    pDrvCtrl = (ARMV7_GEN_TIMER_DRV_CTRL *)hwMemAlloc(sizeof (ARMV7_GEN_TIMER_DRV_CTRL));
    if (pDrvCtrl == NULL)
        return;

    /* store the vxBus device pointer in the timer driver structure */
    pDrvCtrl->pInst = pInst;
    /* also store the timer driver pointer in the vxBus device structure */
    pInst->pDrvCtrl = pDrvCtrl;

	/* 
	get the HCF device from the hwconf 
	*/
	pHcf = hcfDeviceGet (pInst);
	if (pHcf == NULL)
	{
#ifndef _VXBUS_BASIC_HWMEMLIB
		hwMemFree ((char *)pDrvCtrl);
#endif /* _VXBUS_BASIC_HWMEMLIB */
		return;
	}
	
	pDrvCtrl->regBase = pInst->pRegBase[0];
    vxbRegMap (pInst, 0, &pDrvCtrl->handle);
	
	pTimerData = (ARMV7_GEN_TIMER_DATA *) hwMemAlloc (sizeof (ARMV7_GEN_TIMER_DATA));
	if (pTimerData == NULL)
	{
#ifndef _VXBUS_BASIC_HWMEMLIB          
		hwMemFree ((char *) pDrvCtrl);
#endif /* _VXBUS_BASIC_HWMEMLIB */         
		return;
	}
	
	pDrvCtrl->pTimerData = pTimerData;
	
    /* make sure the data value we were just allocated is initialized to 0 */
    memset ((void *) pTimerData, 0, sizeof (ARMV7_GEN_TIMER_DATA));
    
    /* set up the timer data */
    pTimerData = &pDrvCtrl->pTimerData[0];
    pTimerData->pInst = pInst;

    /* set up the timer function */
    pTimerFunc = &pTimerData->timerFunc;

	
    /*
     * resourceDesc {
     * The clkFreq resource specifies the sytem
     * clock frequency. }
     */
    pDrvCtrl->doubleRead = FALSE;
    (void) devResourceGet (pHcf, "double-read", HCF_RES_INT,
                           (void *)&pDrvCtrl->doubleRead);
	
    /* locate the timer functionality data structure */
    pTimerFunc->minFrequency = ARM_GEN_TIMER_DEFAULT_MIN_FREQ;
    pTimerFunc->maxFrequency = ARM_GEN_TIMER_DEFAULT_MAX_FREQ;

    pTimerFunc->clkFrequency = __inline__GetCntFreq();  /* 16666666 ???*/

    GEN_TIMER_DBG (GEN_TIMER_DBG_INFO,
                   "Clock Frequency: %d\n", pTimerFunc->clkFrequency);
    if (pTimerFunc->clkFrequency < ARM_GEN_TIMER_MIN_CLK_FREQ)
    {
        GEN_TIMER_DBG (GEN_TIMER_DBG_ERR,
                       "clkFrequency wrong for ARM generic timer\n");
        goto errOut;
    }

    /* store the feature provided by the timer */
    pTimerFunc->features =  VXB_TIMER_CAN_INTERRUPT |
                            VXB_TIMER_INTERMEDIATE_COUNT |
                            VXB_TIMER_SIZE_32 |
                            VXB_TIMER_SIZE_64 |
                            VXB_TIMER_AUTO_RELOAD;

    /* set default system ticks per second */    
    pTimerFunc->ticksPerSecond = DEFAULT_TICKS_PER_SECOND;
	
    /* 
    calculate the maximum possible timer rollover period 
	*/
    pTimerData->maxCount = ARM_GEN_TIMER_MAX_COUNT;
    pTimerFunc->rolloverPeriod = (UINT32)(pTimerData->maxCount / 
                                          (UINT64)pTimerFunc->clkFrequency);
	
    /* copy and null terminate the name of the timer */
    strncpy (pTimerFunc->timerName, ARMV7_GEN_TIMER_NAME, MAX_DRV_NAME_LEN);
    pTimerFunc->timerName [MAX_DRV_NAME_LEN] = '\0';
    /*pTimerFunc->timerNo = 0;*/

    /* 
    populate the function pointers 
	*/
    pTimerFunc->timerAllocate = armv7GenTimerAllocate;
    pTimerFunc->timerRelease = armv7GenTimerRelease;
	
    pTimerFunc->timerRolloverGet = armv7GenTimerRolloverGet;
    pTimerFunc->timerRolloverGet64 = armv7GenTimerRolloverGet64;
	
    pTimerFunc->timerCountGet = armv7GenTimerCountGet;	
    pTimerFunc->timerCountGet64 = armv7GenTimerCountGet64;
	
    pTimerFunc->timerDisable = armv7GenTimerDisable;	
	
    pTimerFunc->timerEnable = armv7GenTimerEnable;	
    pTimerFunc->timerEnable64 = armv7GenTimerEnable64;
	
    pTimerFunc->timerISRSet = armv7GenTimerISRSet;
	
    pTimerData->timerVal = ARMV7_GEN_TIMER_MAX_COUNT;

    /* 
    initialize the spinlock 
	*/
    SPIN_LOCK_ISR_INIT (&pTimerData->spinLock, 0);

    /* 
    publish methods
    */
    pInst->pMethods = armv7GenTimerDrv_methods;
	
    /* per-device init */    
    armv7GenTimerDevInit (pInst);

	return;
	
errOut:	
	hwMemFree (pTimerData);
	hwMemFree (pDrvCtrl);
	return;
}

	
/*******************************************************************************
*
* armv7GenTimerInstInit2 - second level initialization routine of timer device
*
* This routine performs the second level initialization of the timer device.
*
* This routine is called later during system initialization.  OS features
* such as memory allocation are available at this time.
*
* RETURNS: N/A
*
* ERRNO
*/
LOCAL void armv7GenTimerInstInit2
    (
    VXB_DEVICE_ID pInst
    )
{
    /* 
    connect the ISR to the timer interrupt 
	*/
    (void) vxbIntConnect (pInst, 0, armv7GenTimerIsr, (void *) pInst);
	
	/* 
	enable the timer interrupt 
	*/	
	/*(void) vxbIntEnable (pInst, 0, armv7GenTimerIsr, (void *pInst);*/

	/* 
	Enable system_Counter 
	*/
	*(UINT32 *)GTIMER_ADDR_BASE = 0x03;
	
	return;
}

/*******************************************************************************
*
* armv7GenTimerInstConnect - third level initialization routine of timer device
*
* This is the function called to perform the third level initialization of
* the timer device.
*
* RETURNS: N/A
*
* ERRNO
*/

LOCAL void armv7GenTimerInstConnect
    (
    VXB_DEVICE_ID	pInst
    )
    {
    /* nothing is done here */

    return;
    }

#endif

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

LOCAL void armv7GenTimerDevInit
    (
    VXB_DEVICE_ID pInst   /* The device */
    )
{   
    if (pInst == NULL)
        return;    
}
	
/*******************************************************************************
*
* armMpCoreTimerFuncGet - method to retrieve the timer functionality
*
* This function is the driver method used to retrieve the timer functionality.
*
* RETURNS: OK or ERROR if functionality is not retrieved.
*
* ERRNO
*/

LOCAL STATUS armv7GenTimerFuncGet
    (
    VXB_DEVICE_ID                   pInst,
    struct vxbTimerFunctionality ** ppTimerFunc,
    int                             timerNo
    )
{
    ARMV7_GEN_TIMER_DRV_CTRL* pDrvCtrl = NULL;
    ARMV7_GEN_TIMER_DATA * pTimerData = NULL;

    /* validate the parameters */
    if ((pInst == NULL) || (ppTimerFunc == NULL) || (timerNo != 0))
        return ERROR;

    pDrvCtrl = (ARMV7_GEN_TIMER_DRV_CTRL *) pInst->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return ERROR;

    pTimerData = &pDrvCtrl->pTimerData[timerNo];

    /* store the address of the timer functionality data structure */
    *ppTimerFunc = &pTimerData->timerFunc;

    return OK;
}


/*******************************************************************************
*
* armv7GenTimerAllocate - allocate resources for a timer
*
* This is the function called to allocate a timer for usage by the
* Timer Abstraction Layer.
*
* RETURNS: OK or ERROR if timer allocation failed.
*
* ERRNO: N/A
*/

LOCAL STATUS armv7GenTimerAllocate
    (
    VXB_DEVICE_ID pInst,   /* The device */
    UINT32 flags,          /* Flags for the timer, currently unused */
    void ** pCookie,       /* An output, a handle to a cookie containing timer information */
    UINT32 timerNo         /* The timer number */
    )
{
/*    struct vxbTimerFunctionality *pTimerFunc;*/

    ARMV7_GEN_TIMER_DRV_CTRL * pDrvCtrl;
    ARMV7_GEN_TIMER_DATA * pTimerData;

    /* validate the parameters */
    if ((pInst == NULL) || (pCookie == NULL) || (timerNo != 0))
        return ERROR;
    
    pDrvCtrl = (ARMV7_GEN_TIMER_DRV_CTRL *) pInst->pDrvCtrl;
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
* armv7GenTimerRelease - release the timer resource
*
* This is the function called to release a timer device.
*
* RETURNS: OK or ERROR if parameter is not valid.
*
* ERRNO: N/A
*/

LOCAL STATUS armv7GenTimerRelease
    (
    VXB_DEVICE_ID pInst,   /* The device */
    void * pCookie         /* A pointer to timer information for this timer */
    )
{
    ARMV7_GEN_TIMER_DRV_CTRL * pDrvCtrl;	
    ARMV7_GEN_TIMER_DATA * pTimerData;
	
 /*   struct vxbTimerFunctionality *pTimerFunc;*/

    /* validate the parameters */
    if ((pInst == NULL) || (pCookie == NULL))
        return ERROR;

    pDrvCtrl = (ARMV7_GEN_TIMER_DRV_CTRL *) pInst->pDrvCtrl;
    pTimerData = (ARMV7_GEN_TIMER_DATA *) pCookie;

    /* validate pInst and check to see if the timer is allocated */
    if ((pDrvCtrl->pInst != pInst) || (!pTimerData->timerFunc.allocated))
        return ERROR;

    SPIN_LOCK_ISR_TAKE (&pTimerData->spinLock);

    /* disable the timer */    
    if (armv7GenTimerDisable(pCookie) != OK)
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
* armv7GenTimerRolloverGet - retrieve the maximum value of the counter
*
* This is the function called to retrieve the maximum value of the counter.
* The maximum value is returned in 'pCount' parameter.
*
* RETURNS: OK or ERROR if the parameter is invalid.
*
* ERRNO: N/A
*/

LOCAL STATUS armv7GenTimerRolloverGet
    (
    void * pCookie,   /* Information about this timer */
    UINT32 * count    /* An output, the current rollover value for this timer */
    )
{
    ARMV7_GEN_TIMER_DATA * pTimerData;

    if ((pCookie == NULL) || (count == NULL))
        return ERROR;
    
    pTimerData = (ARMV7_GEN_TIMER_DATA *) pCookie;

    /* store the counter value */
    *count = pTimerData->timerVal;

    return OK;
}
	
/*******************************************************************************
*
* armGenTimerRolloverGet64 - retrieve the maximum value of the 64-bit counter
*
* This is the function called to retrieve the maximum value of the 64-bit 
* counter. The maximum value is returned in 'pCount' parameter.
*
* RETURNS: OK or ERROR if the parameter is invalid.
*
* ERRNO: N/A
*/

LOCAL STATUS armv7GenTimerRolloverGet64
    (
    void *      pCookie,
    UINT64 *    pCount
    )
{
    if (pCount == NULL)
        return ERROR;

    /* free run counter, always get the MAX count for timestamp */
    *pCount = ARM_GEN_TIMER_MAX_COUNT64;

    GEN_TIMER_DBG (GEN_TIMER_DBG_INFO,
                   "armGenTimerRolloverGet: %u\n", ARM_GEN_TIMER_MAX_COUNT);
    return OK;
}


/*******************************************************************************
*
* armv7GenTimerCountGet - retrieve the current value of the counter
* armMpCoreTimerRolloverGet - retrieve the maximum value of the counter

*
* This function is used to retrieve the current value of the counter.
* The current value is returned in 'pCount' parameter.
*
* RETURNS: OK or ERROR if the parameter is invalid.
*
* ERRNO: N/A
*/

LOCAL STATUS armv7GenTimerCountGet
    (
    void *      pCookie,
    UINT32 *    pCount
    )
{
	ARMV7_GEN_TIMER_DRV_CTRL * pDrvCtrl;
	ARMV7_GEN_TIMER_DATA * pTimerData;
    UINT32 snap;
    UINT64      cnt64;
    UINT32      cnt32;

    if ((pCookie == NULL) || (pCount == NULL))
        return ERROR;
    
    pTimerData = (ARMV7_GEN_TIMER_DATA *) pCookie;
	pDrvCtrl = (ARMV7_GEN_TIMER_DRV_CTRL*)pTimerData->pInst->pDrvCtrl;

    /* take the spinlock for the exclusive access to the timer resources */
    SPIN_LOCK_ISR_TAKE (&pTimerData->spinLock);
    
    /* read the current value of the timer */    
    cnt64 = armv7GenTimerGetTimerCnt (pDrvCtrl);
	
    cnt64 -= pTimerData->startCount;
    cnt32 = (UINT32)(cnt64 % ARMV7_GEN_TIMER_MAX_COUNT);
	
    *pCount = cnt32;        
    
    /* release the spinlock */
    SPIN_LOCK_ISR_GIVE (&pTimerData->spinLock);
    
    return OK;	
}

/*******************************************************************************
*
* armGenTimerCountGet64 - retrieve the current value of the 64-bit counter
*
* This function is used to retrieve the current value of the 64-bit counter.
* The current value is returned in 'pCount' parameter.
*
* RETURNS: OK or ERROR if the parameter is invalid.
*
* ERRNO: N/A
*/

LOCAL STATUS armv7GenTimerCountGet64
    (
    void *      pCookie,
    UINT64 *    pCount
    )
{
    UINT64                          cnt64;
	
    ARMV7_GEN_TIMER_DRV_CTRL * pDrvCtrl;
    ARMV7_GEN_TIMER_DATA *            pTimerData;
    struct vxbTimerFunctionality *  pTimerFunc;

    GEN_TIMER_DBG (GEN_TIMER_DBG_INFO, "armGenTimerISRSet\n");

    if ((pCookie == NULL) || (pCount == NULL))
        return ERROR;

    pTimerData = (struct vxbTimerFunctionality *)pCookie;
	pDrvCtrl = (ARMV7_GEN_TIMER_DRV_CTRL*)pTimerData->pInst->pDrvCtrl;

    /*
     * The timer counter starts when enabled and with a period of
     * ARM_GEN_TIMER_MAX_COUNT64.
     */

    cnt64 = armv7GenTimerGetTimerCnt (pDrvCtrl);
    cnt64 -= pTimerData->startCount;
    *pCount = cnt64;

    GEN_TIMER_DBG (GEN_TIMER_DBG_INFO,
                   "armGenTimerCountGet: cnt64=%u\n", cnt64);
    return OK;
    }

/*******************************************************************************
*
* armv7GenTimerDisableInternal - disable the timer without spinlock
*
* This routine stops the timer and disables interrupt generation for the
* requested hardware timer without spinlock.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
*/
LOCAL void armv7GenTimerDisableInternal
    (
    ARMV7_GEN_TIMER_DRV_CTRL * pDrvCtrl
    )
{
    if (!pDrvCtrl->isEnabled)
        return;

    /* disable the timer */    
    armv7GenTimerWriteReg (pDrvCtrl, GEN_TIMER_REG_CTL, GEN_TIMER_CTRL_IMASK);
    
    /* disable the timer interrupt */
#if 0
    if (pDrvCtrl->isVirtual)
    {
        (void)vxbIntDisable (pDrvCtrl->pInst, pDrvCtrl->intRes[GEN_TIMER_VIRTURE_PPI]);
    }
    else
    {
        if (pDrvCtrl->secPhyAvail)
        {
            (void)vxbIntDisable (pDrvCtrl->pInst, pDrvCtrl->intRes[GEN_TIMER_SECURE_PHY_PPI]);
        }
		
        if (pDrvCtrl->nonSecPhyAvail)
        {
            (void)vxbIntDisable (pDrvCtrl->pInst, pDrvCtrl->intRes[GEN_TIMER_NON_SECURE_PHY_PPI]);
        }
    }
#else
	(void)vxbIntDisable (pDrvCtrl->pInst, 0, armv7GenTimerIsr, 0);	
#endif

    pDrvCtrl->pTimerData->maxCount = ARMV7_GEN_TIMER_MAX_COUNT;
    pDrvCtrl->isEnabled = FALSE;

	return;
}

/*******************************************************************************
*
* armv7GenTimerDisable - disable the timer interrupt
*
* This function is called to disable the timer interrupt.
*
* RETURNS: OK or ERROR if timer is not disabled
*
* ERRNO
*/

LOCAL STATUS armv7GenTimerDisable
    (
    void *      pCookie
    )
{
    ARMV7_GEN_TIMER_DRV_CTRL *  pDrvCtrl;
    ARMV7_GEN_TIMER_DATA * pTimerData;

    if (pCookie == NULL)
        return ERROR;
	
    pTimerData = (ARMV7_GEN_TIMER_DATA *) pCookie;
    pDrvCtrl = (ARMV7_GEN_TIMER_DRV_CTRL *)pTimerData->pInst->pDrvCtrl;

    SPIN_LOCK_ISR_TAKE (&pTimerData->spinLock);
	
    armv7GenTimerDisableInternal (pDrvCtrl);
	
    SPIN_LOCK_ISR_GIVE (&pTimerData->spinLock);

    return OK;
}

/*******************************************************************************
*
* armv7GenTimerEnable - enable the timer interrupt
*
* This function enables the timer interrupt.
*
* RETURNS: OK or ERROR if timer is not enabled
*
* ERRNO
*/

LOCAL STATUS armv7GenTimerEnable
    (
    void *      pCookie,
    UINT32      maxTimerCount
    )
{
    VXB_DEVICE_ID       pInst;
    ARMV7_GEN_TIMER_DRV_CTRL * pDrvCtrl;	
    ARMV7_GEN_TIMER_DATA * pTimerData;
    struct vxbTimerFunctionality * pTimerFunc;
	
    /* validate the parameters */    
    if ((pCookie == NULL) || (maxTimerCount == 0))
        return ERROR;

    if (maxTimerCount > ARMV7_GEN_TIMER_MAX_COUNT)
    {
        return ERROR;
    }	

    /* retrieve the pDrvCtrl */    
    pTimerData = (ARMV7_GEN_TIMER_DRV_CTRL *) pCookie;    
    pInst = pTimerData->pInst;    
    pTimerFunc = &pTimerData->timerFunc;
	
	pDrvCtrl = (ARMV7_GEN_TIMER_DRV_CTRL*)pInst->pDrvCtrl;

    SPIN_LOCK_ISR_TAKE (&pTimerData->spinLock);

    /* if the timer is already running, stop it before making adjustments */    
    if (pDrvCtrl->isEnabled)
    {
        (void)armv7GenTimerDisableInternal (pDrvCtrl);
    }	
	
	/* enable the timer interrupt */	
	(void) vxbIntEnable (pInst, 0, armv7GenTimerIsr, (void *)pInst);

    pTimerData->maxCount  = maxTimerCount;

    /* record timestamp start count */
    pTimerData->startCount = armv7GenTimerGetTimerCnt(pDrvCtrl);

    /* recalculate ticksPerSecond */
   /* pTimerFunc->ticksPerSecond = pTimerFunc->clkFrequency / pTimerData->maxCount;*/
    pTimerFunc->ticksPerSecond = (UINT32)((UINT64)pTimerFunc->clkFrequency / 
                                          pTimerData->maxCount);

    /* update the timer value register with maxTimerCount */
    armv7GenTimerWriteReg (pDrvCtrl, GEN_TIMER_REG_TVAL, maxTimerCount);

    /* set up the timer control register */
    armv7GenTimerWriteReg (pDrvCtrl, GEN_TIMER_REG_CTL, GEN_TIMER_CTRL_ENABLE);
	
    pDrvCtrl->isEnabled = TRUE;
    SPIN_LOCK_ISR_GIVE (&pTimerData->spinLock);

    GEN_TIMER_DBG (GEN_TIMER_DBG_INFO, "armv7GenTimerEnable OK\n");
    return OK;
}

LOCAL STATUS armv7GenTimerEnable64
    (
    void *  pCookie,
    UINT64  maxTimerCount
    )
{
    return armv7GenTimerEnable (pCookie, maxTimerCount);
}

/*******************************************************************************
*
* armv7GenTimerISRSet - set a function to be called on the timer interrupt
*
* This function is called to set a function which can be called whenever
* the timer interrupt occurs.
*
* RETURNS: OK or ERROR if the parameter is invalid.
*
* ERRNO: N/A
*/

LOCAL STATUS armv7GenTimerISRSet
    (
    void *      pCookie,
    void        (*pFunc)(int),
    int         arg
    )
{
    ARMV7_GEN_TIMER_DATA * pTimerData;

    if ((pCookie == NULL) || (pFunc == NULL))
        return ERROR;
    
    pTimerData = (ARMV7_GEN_TIMER_DATA *) pCookie;

    /* take the spinlock to update pIsrFunc and arg atomically */
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
* armv7GenTimerIsr - ISR for the ARM Generic Timer
*
* This routine handles the ARM Generic Timer interrupt. 
*
* RETURNS : N/A
*/

LOCAL void armv7GenTimerIsr
    (
    VXB_DEVICE_ID  pInst 
    )
{
    ARMV7_GEN_TIMER_DRV_CTRL * pDrvCtrl = NULL;
    ARMV7_GEN_TIMER_DATA * pTimerData = NULL;
    
    UINT32 ctl;	
    UINT64  cntVal;
	
    if (pInst == NULL)
        return;
    
    pDrvCtrl = pInst->pDrvCtrl;    
    pTimerData = (ARMV7_GEN_TIMER_DATA *)&pDrvCtrl->pTimerData[0];

	ctl = armv7GenTimerReadReg (pDrvCtrl, GEN_TIMER_REG_CTL);
	if ((ctl & GEN_TIMER_CTRL_ISTATUS) == 0)
		return;
	
#if 1
	/*
	 * In a very extreme situation, e.g., in a very heavy load system with high
	 * system clock tick frequency, the timer interrupt may be deferred to one
	 * or more ticks, only increasing one time of maxCount to the Compare
	 * Register would lead to the compare value behind the counter, so the match
	 * interrupt will not occur until the counter rollovers to the compare 
	 * value. 
	 * The following do...while() statement can avoid such situation.
	 */	
	if (pDrvCtrl->isAutoReload)
	{
		do
		{
			pTimerData->cmpCount += pTimerData->maxCount;
			
			armv7GenTimerSetTimerCmp (pDrvCtrl, pTimerData->cmpCount);
			
			cntVal = armv7GenTimerGetTimerCnt (pDrvCtrl);
			
		} while (cntVal > pTimerData->cmpCount);
	}
#endif	
    
    /* call the ISR hander if one is registered */    
    if (pTimerData->enabled && pTimerData->pIsrFunc != NULL)
    {
        (*(pTimerData->pIsrFunc))(pTimerData->arg);
    }

	return;
}


#if 1
void test_gtc(void)
{
	VXB_DEVICE_ID		pDev;
	ARMV7_GEN_TIMER_DRV_CTRL* pDrvCtrl = NULL;

	int unit = 0;	
	int i = 0;

	UINT32 gtc_cnt = 0;

	/*==================================================*/
	unit = 0;
	pDev = vxbInstByNameFind (ARMV7_GEN_TIMER_NAME, unit);
	if (pDev == NULL)
	{
		printf ("\n GTC not find this spi unit(%d)!	\n\n", unit);
		return ERROR;
	}
	else
	{
		pDrvCtrl = (ARMV7_GEN_TIMER_DRV_CTRL *)pDev->pDrvCtrl;
		printf ("GTC: vxbInstByNameFind(%s)_%d: 0x%X \n", ARMV7_GEN_TIMER_NAME, unit, pDev);
	}
	/*==================================================*/

	armv7GenTimerCountGet(pDrvCtrl->pTimerData, &gtc_cnt);

	printf("gtc_cnt: %d (0x%X) \n", gtc_cnt, gtc_cnt);

	return;
}

#endif

