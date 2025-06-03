/* vxbArmGenIntCtlr.c - ARM generic interrupt controller driver */

/*
* Copyright (c) 2008-2015 Wind River Systems, Inc.
*
* The right to copy, distribute, modify or otherwise make use
* of this software may be licensed only pursuant to the terms
* of an applicable Wind River license agreement.
*/

/*
modification history
--------------------
27feb15,y_f  fix rerouting an inactive interrupt in vxbArmGicIntReroute()
             (VXW6-84106)
20nov14,c_l  Fix system hung issue for preempt model. (VXW6-83802)
17jul14,c_l  Set different priority between the SPI and IPI. (VXW6-83150)
25mar14,c_l  add support to interrupt disconnection. (VXW6-80764)
01t,08aug13,c_l  Add necessary header file for max performance vsb.
                 (WIND00429875)
01s,15jul13,c_l  Add interrupt priority/sensitivity support. (WIND00420705)
01r,07aug12,j_b  add distributor and CPU interface register offset support
                 (CQ:WIND00367952)
01q,31jul12,cwl  Fix interrupt reroute issue. (WIND00362493)
01p,15apr12,clx  support PPI interrupt type.
01o,10jan12,ggm  fixed vxbArmGicIpiDisconnect function (WIND00255968).
01n,31jan11,to   WIND00251990: restore backward compatibility for ILP32
01m,16dec10,jdw  Fix to use correct API for intDisable() (CQ: WIND00246472)
01l,28sep10,rab  mods for guest os
01j,20jul10,fao  add be8 support.
01i,05jul10,my_  Fix ipiId calculation in vxbArmGicIpiEnable/Disable
                 (WIND00221526)
01h,09jun10,wqi  Corrected return value of vxbArmGicIpiConnect. (WIND00210508)
01g,17may10,cww  Updated vxIpiCntrlInit to avoid LP64 padding
01f,27oct09,z_l  update this driver.
                 use standard vxbus driver mode. i.e, input-pin and so on.
                 add SMP support, including interrupt reroute, priority set.
                 clean up codes.
01e,17sep09,j_b  rename potentially conflicting vxIpiCtrlInit struct
                 (WIND00179338)
01d,01Apr09,j_b  update VxBus version to VXB_VER_4_0_0;
                 rename _WRS_VX_SMP to _WRS_CONFIG_SMP;
                 fix numArmGicIntLines calculation
01c,11nov08,j_b  define armGicBase locally
01b,04dec08,mdo  Clear pending ints before enabling
01a,21oct08,j_b  written based on vxbEbGenIntrCtl.c, rev 01e
*/

/*
DESCRIPTION

This module implements the interrupt controller driver for the ARM Generic
Interrupt Controller (GIC), such as that implemented in the ARM11 MPCore where
it is referred to as the MPCore Distributed Interrupt Controller.  This
controller consists of an interrupt distributor and a CPU interface.  For a
multicore CPU, there is a CPU interface for each core.  Each core accesses its
own CPU interface at the same address, with the bus control system (i.e., SCU)
identifying the CPU by its transaction ID.

This driver supports the ARM Generic Interrupt Controller architecture that is
implemented in the ARM11 MPCore processor, which preceded and is a subset of
that described in the ARM Generic Interrupt Controller Architecture
Specification.
For support of the interrupt control architecture used by the ARM11 MPCore core
tile with the ARM Emulation Baseboard (EB), use vxbEbGenIntrCtl.c.

The interrupt controller supports interrupts running at different
priority levels. S/W prioritization of interrupts are disabled in this
release. However, prioritization can be performed by allowing all
interrupt levels to pass from the distributor to the CPU by setting
the CPU priority mask to GIC_INT_ALL_ENABLED. Prioritization will
occur at the distributor by adjusting the priority of each individual
IRQ via the distributor priority registers. For this release, we use 0
as the priority of all interrupts by default.

Both single core and multiple cores are supported by this driver.

EXTERNAL INTERFACE

The driver provides the standard vxbus external interface,
vxbArmGenIntCtlrRegister(). This function registers the driver with the
vxbus subsystem, and instances will be created as needed. Since GIC is a
processor local bus device, each device instance must be specified in the
hwconf.c file in a BSP.

Driver-specific resources are specified below (in the driver source file and in
the README file).

the explanation of some abbreviations

  CPC - Cross Processor Call
  IPI - Inter Processor Interrupt
  GIC - General Interrupt Controller
  SGI - Software Generated Interrupt
  PPI - Private Peripheral Interrupt
  SPI - Shared Peripheral Interrupt

Optinally, the interrupt priority and sensitivity can be set as below in hwconf:

\cs
LOCAL const struct intrCtlrPriority armGICPriority[] = {
    { INT_PIN_TIMER,       INT_PRI_TIMER_0},
    { INT_PIN_UART,        INT_PRI_UART_1 },
    { INT_PIN_DMA,         INT_PRI_SDMA   },
    { INT_PIN_END,         INT_PRI_FEC    },
};
\ce

\cs
LOCAL const struct intrCtlrTrigger armGICTrigger[] = {
    { INT_PIN_TIMER,       VXB_INTR_TRIG_FALLING_EDGE },
    { INT_PIN_UART,        VXB_INTR_TRIG_RISING_EDGE },
    { INT_PIN_DMA,         VXB_INTR_TRIG_ACTIVE_LOW },
    { INT_PIN_END,         VXB_INTR_TRIG_ACTIVE_HIGH },
};
\ce

An example hwconf entry is shown below:

\cs
LOCAL const struct hcfResource armGICResources[] = {
    { VXB_REG_BASE,        HCF_RES_INT,  {(void *)PBXA9_GIC1_BASE } },
    { "distOffset",        HCF_RES_INT,  {(void *)gicDistOffset } },
    { "cpuOffset",         HCF_RES_INT,  {(void *)gicCpuOffset } },
    { "input",             HCF_RES_ADDR, {(void *)&gicInputs[0] } },
    { "intMode",           HCF_RES_INT,  {(void *)INT_MODE } },
    { "maxIntLvl",         HCF_RES_INT,  {(void *)SYS_INT_LEVELS_MAX } },
    { "inputTableSize",    HCF_RES_INT,  {(void *)NELEMENTS(gicInputs) } },
    { "cpuRoute",          HCF_RES_ADDR, {(void *)&gicCpu[0] } },
    { "cpuRouteTableSize", HCF_RES_INT,  {(void *)NELEMENTS(gicCpu) } }
#ifdef _WRS_CONFIG_SMP
   ,{ "maxCpuNum",         HCF_RES_INT, { (void *)VX_SMP_NUM_CPUS} }
#endif
};
\ce

INCLUDE FILES:
intLib.h vxIpiLib.h vxbArmGenIntCtlr.h
*/

#include <vxWorks.h>
#include <vsbConfig.h>
#include <intLib.h>
#include <arch/arm/intArmLib.h>
#include <arch/arm/excArmLib.h>
#include <arch/arm/vxAtomicArchLib.h>
#include <iv.h>

#include <sysLib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memLib.h>

#ifdef  _WRS_CONFIG_SV_INSTRUMENTATION
#   include <private/eventP.h>
#endif  /* _WRS_CONFIG_SV_INSTRUMENTATION */

#include <hwif/util/hwMemLib.h>
#include <vxBusLib.h>
#include <hwif/vxbus/vxBus.h>
#include <hwif/vxbus/vxbPlbLib.h>
#include <hwif/vxbus/vxbIntrCtlr.h>
#include <../src/hwif/intCtlr/vxbIntCtlrLib.h>
#include <hwif/vxbus/hwConf.h>
#include "../h/vxbus/vxbAccess.h"

/*
#include <hwif/intCtlr/vxbArmGenIntCtlr.h>
*/
#include "vxbArmGenIntCtlr.h"

#ifdef _WRS_CONFIG_SMP
#   include <vxIpiLib.h>
#   include <private/cpcLibP.h>
#endif

/* define  */

#ifdef ARMBE8
#    define SWAP32 vxbSwap32
#else
#    define SWAP32
#endif /* ARMBE8 */

/* vxBus Driver Name */

#define GIC_NAME    "armGicDev"

/* Connect the interrupt handler to it's exception vector */

#define INTR_EXC_ID (EXC_OFF_IRQ) /* exception id, for external interrupt */

#define GIC_IPI_PRIORITY 0
#define GIC_PRIORITY_LEVEL_STEP 0x10
#define GIC_SPI_PRIORITY_DEFAULT 0x10101010

#ifndef EXC_CONNECT_INTR_RTN
#   define EXC_CONNECT_INTR_RTN(rtn) \
    excIntConnect ((VOIDFUNCPTR *)INTR_EXC_ID,rtn)
#endif

/* This sets the CPU interrupt enable on */

#ifndef CPU_INTERRUPT_ENABLE
#   define CPU_INTERRUPT_ENABLE intCpuUnlock(0)
#endif

/* This resets the CPU interrupt enable */

#ifndef CPU_INTERRUPT_DISABLE
#   define CPU_INTERRUPT_DISABLE intCpuLock()
#endif

#define GIC_ISR(pEnt,inputPin,func)                          \
        {                                                    \
        struct vxbIntCtlrPin * pPin =                        \
           vxbIntCtlrPinEntryGet(pEnt,inputPin);             \
        func = pPin->isr;                                    \
        }

#define GIC_DESTCPU(pEnt,inputPin,destCpu)                   \
        {                                                    \
        struct vxbIntCtlrPin * pPin =                        \
           vxbIntCtlrPinEntryGet(pEnt,inputPin);             \
        destCpu = pPin->pinCpu;                              \
        }

/* structure holding Generic Interupt Controller details */

typedef struct armGicDrvCtrl
    {
    VXB_DEVICE_ID        pInst;         /* instance pointer */
    BOOL                 initialized;
    struct intCtlrHwConf isrHandle;
    int                  intMode;       /* type of interrupt handling*/
    void *               gicBase;       /* gic base address */
    INT32                gicDistOffset; /* distributor register base offset */
    INT32                gicCpuOffset;  /* CPU interface register base offset */
    UINT32               gicLvlCurrent;
    void *               regBase;
    void *               handle;
    INT32                gicLvlNum;     /* the maximum interrupt level number */
    INT32                gicCpuNum;     /* the maximum CPU number in system */
    } ARM_GIC_DRV_CTRL;

/* forward declarations */

IMPORT STATUS (*_func_intConnectRtn) (VOIDFUNCPTR *, VOIDFUNCPTR, int);
IMPORT STATUS (*_func_intDisconnectRtn) (VOIDFUNCPTR *, VOIDFUNCPTR, int);

LOCAL STATUS vxbArmGicDevInit (VXB_DEVICE_ID pInst);
#ifdef _WRS_CONFIG_SMP
STATUS vxbArmGicLvlVecChk (VXB_DEVICE_ID pInst, int*, int*, int*);
STATUS vxbArmGicLvlVecAck (VXB_DEVICE_ID pInst, int, int, int);
#else
STATUS vxbArmGicLvlVecChk (VXB_DEVICE_ID pInst, int*, int*);
STATUS vxbArmGicLvlVecAck (VXB_DEVICE_ID pInst, int, int);
#endif
int vxbArmGicLvlChg (VXB_DEVICE_ID  pInst, int);
LOCAL STATUS vxbArmGicIntDevInit (int cpuNum);
#ifdef INCLUDE_SHOW_ROUTINES
void vxbArmGicDataShow (VXB_DEVICE_ID  pInst, int *dummy);
#endif /* INCLUDE_SHOW_ROUTINES */

LOCAL void vxbArmGicCtlInit(VXB_DEVICE_ID pInst);
LOCAL void vxbArmGicCtlInit2(VXB_DEVICE_ID pInst);
LOCAL STATUS sysArmGicConnect
    (
    VOIDFUNCPTR *  vector,     /* interrupt vector to attach to     */
    VOIDFUNCPTR    routine,    /* routine to be called              */
    int            parameter   /* parameter to be passed to routine */
    );
LOCAL STATUS sysArmGicDisconnect
    (
    VOIDFUNCPTR *  vector,     /* interrupt vector to detach from   */
    VOIDFUNCPTR    routine,    /* routine to be disconnected        */
    int            parameter   /* parameter to be matched           */
    );
LOCAL STATUS sysArmGicISREnable
    (
    struct intCtlrHwConf *	pEntries,
    int				inputPin
    );
LOCAL STATUS sysArmGicEnable
    (
    int            vector
    );
LOCAL STATUS sysArmGicISRDisable
    (
    struct intCtlrHwConf *	pEntries,
    int			            inputPin
    );
LOCAL STATUS sysArmGicDisable
    (
    int vector
    );
LOCAL STATUS vxbArmGicConnect
    (
    VXB_DEVICE_ID           pIntCtlr,             /* int Ctlr device struct  */
    VXB_DEVICE_ID           pDev,                 /* device being connnected */
    int                     index,                /* device index            */
    void                    (*pIsr)(void * pArg), /* isr being connected     */
    void *                  pArg,                 /* isr's arg               */
    int *                   pInputPin             /* input pin device is on  */
    );
LOCAL STATUS vxbArmGicDisconnect
    (
    VXB_DEVICE_ID           pIntCtlr,             /* int Ctlr device struct  */
    VXB_DEVICE_ID           pDev,                 /* device being connnected */
    int                     index,                /* device index            */
    VOIDFUNCPTR             pIsr,                 /* isr being connected     */
    void *                  pArg                  /* isr's arg               */
    );
LOCAL STATUS vxbArmGicEnable
    (
    VXB_DEVICE_ID           pIntCtlr,             /* int Ctlr device struct  */
    VXB_DEVICE_ID           pDev,                 /* device being connnected */
    int                     index,                /* device index            */
    VOIDFUNCPTR             pIsr,                 /* isr being connected     */
    void *                  pArg                  /* isr's arg               */
    );
LOCAL STATUS vxbArmGicDisable
    (
    VXB_DEVICE_ID           pIntCtlr,             /* int Ctlr device struct  */
    VXB_DEVICE_ID           pDev,                 /* device being connnected */
    int                     index,                /* device index            */
    VOIDFUNCPTR             pIsr,                 /* isr being connected     */
    void *                  pArg                  /* isr's arg               */
    );
LOCAL STATUS vxbArmGicLvlEnable(VXB_DEVICE_ID pInst, int level);
LOCAL STATUS vxbArmGicLvlDisable(VXB_DEVICE_ID pInst, int level);
LOCAL int vxbArmGicHwLvlChg(int level);

#ifdef _WRS_CONFIG_SMP
LOCAL STATUS vxbArmGicHwEnable(int inputPin);
LOCAL STATUS vxbArmGicHwDisable(int inputPin);
/*void sysArmGicDevInit(void);*/
STATUS sysArmGicDevInit(void);  /* jc */
LOCAL STATUS vxbArmGicIpiGen(VXB_DEVICE_ID pCtlr, INT32 ipiId,
                                    cpuset_t cpus);
LOCAL STATUS vxbArmGicIpiConnect(VXB_DEVICE_ID pCtlr, INT32 ipiId, \
                                   IPI_HANDLER_FUNC ipiHandler, void * ipiArg);
LOCAL STATUS vxbArmGicIpiDisconnect(VXB_DEVICE_ID pCtlr, INT32 ipiId, \
                                   IPI_HANDLER_FUNC ipiHandler, void * ipiArg);
LOCAL STATUS vxbArmGicIpiEnable(VXB_DEVICE_ID pCtlr, INT32 ipiId);
LOCAL STATUS vxbArmGicIpiDisable(VXB_DEVICE_ID pCtlr, INT32 ipiId);
LOCAL INT32 vxbArmGicIpiPrioGet(VXB_DEVICE_ID pCtlr, INT32 ipiId);
LOCAL STATUS vxbArmGicIpiPrioSet(VXB_DEVICE_ID pCtlr, INT32 ipiId, \
                                        INT32 prio);
LOCAL VXIPI_CTRL_INIT * vxbArmGicIpiCtlGet(VXB_DEVICE_ID pInst, \
                                                  void * pArg);
LOCAL STATUS vxbArmGicIntReroute(VXB_DEVICE_ID pDev, int index, \
                                         cpuset_t destCpu);
LOCAL STATUS vxbArmGicCpuReroute(VXB_DEVICE_ID pDev,void * destCpu);

METHOD_DECL(vxbArmGicIntCtlInit);
DEVMETHOD_DEF(vxbArmGicIntCtlInit, "Init ARM general interrupt controller");
#endif /* _WRS_CONFIG_SMP */

/* GIC base address */

LOCAL UINT32  armGicBase = 0;

/*
 * Distributor and CPU interface register base offsets,
 * with backward-compatible default values for ARM11 MPCore and Cortex-A9 cores.
 */

LOCAL UINT32  armGicDistOffset = 0x1000;
LOCAL UINT32  armGicCpuOffset  = 0x100;

/* armGicLinesNum is used to reduce interrupt process time */

LOCAL UINT32 armGicLinesNum = 0;

/* GIC priority levels */

LOCAL UINT32 armGicPriorityLvlMax = 0;

LOCAL VXB_DEVICE_ID vxbGicId;  /* GIC device ID */
LOCAL ARM_GIC_DRV_CTRL * pVxbArmGicDrvCtrl = NULL;

LOCAL struct drvBusFuncs vxbArmGicCtlFuncs =
    {
    vxbArmGicCtlInit,       /* devInstanceInit */
    vxbArmGicCtlInit2,      /* devInstanceInit2 */
    NULL                    /* devConnect */
    };

/* vxBus vxbArmGicCtl driver registration data structure */

LOCAL struct vxbDevRegInfo armGicCtlRegistration =
    {
    NULL,                         /* pNext */
    VXB_DEVID_DEVICE,             /* devID */
    VXB_BUSID_PLB,                /* busID = PLB */
    VXB_VER_4_0_0,                /* vxbVersion */
    GIC_NAME,                     /* drvName */
    &vxbArmGicCtlFuncs,           /* pDrvBusFuncs */
    NULL,                         /* pMethods */
    NULL,                         /* devProbe */
    NULL                          /* pParamDefaults */
    };

LOCAL device_method_t gicIntCtlr_methods[] =
    {
    DEVMETHOD(vxbIntCtlrConnect,    vxbArmGicConnect),
    DEVMETHOD(vxbIntCtlrDisconnect, vxbArmGicDisconnect),
    DEVMETHOD(vxbIntCtlrEnable,     vxbArmGicEnable),
    DEVMETHOD(vxbIntCtlrDisable,    vxbArmGicDisable),

#ifdef _WRS_CONFIG_SMP
    DEVMETHOD(vxIpiControlGet,      vxbArmGicIpiCtlGet),
    DEVMETHOD(vxbIntCtlrIntReroute, vxbArmGicIntReroute),
    DEVMETHOD(vxbIntCtlrCpuReroute, vxbArmGicCpuReroute),
#endif /* _WRS_CONFIG_SMP */

#ifdef GIC_INTCTLR_DEBUG_ON
    DEVMETHOD(busDevShow,           vxbArmGicShow),
#endif /* GIC_INTCTLR_DEBUG_ON */

    { 0, 0}
    };

#ifdef _WRS_CONFIG_SMP
IMPORT UINT32 vxCpuIndexGet(void);

/*
* This structure is initialized with the control functions for the IPI
* interface. This set of functions allow the CPC layer to manipulate IPI
* interrupts.
*/

LOCAL VXIPI_CTRL_INIT_DECL (vxArmGicIpiCtrlInit,
    {NULL},                   /* ipiList */
    0,                        /* pCpus */
    vxbArmGicIpiGen,          /* ipiEmitFunc */
    vxbArmGicIpiConnect,      /* ipiConnectFunc */
    vxbArmGicIpiEnable,       /* ipiEnableFunc */
    vxbArmGicIpiDisable,      /* ipiDisableFunc */
    vxbArmGicIpiDisconnect,   /* ipiDisconnFunc */
    vxbArmGicIpiPrioGet,      /* ipiPrioGetFunc */
    vxbArmGicIpiPrioSet,      /* ipiPrioSetFunc */
    ARM_GIC_IPI_COUNT,        /* ipiCount */
    NULL                      /* pCtlr */
    );
#endif /* _WRS_CONFIG_SMP */


/*******************************************************************************
*
* vxbArmGicNonPreempISR - non-pre-emptive interrupt exception handler
*
* This handler does not provide pre-emptive interrupts. If a high
* priority interrupt occurs while a low priority interrupt is being
* handled, the high priority interrupt must wait for the low priority
* interrupt handler to finish.  As soon as the low-priority handler is
* done, the high priority handler will be invoked.  This model has less
* exception handling overhead of the fully pre-emptive model, but has a
* greater worst case latency for high priority interrupts.
*
* RETURNS: N/A
*
* ERROR: N/A
*/

LOCAL void vxbArmGicNonPreempISR ()
    {
    int level;
    int vector;
#ifdef _WRS_CONFIG_SV_INSTRUMENTATION
    int loopCnt = -1;
#endif
#ifdef _WRS_CONFIG_SMP
    int srcCpuId = 0;
#endif

#ifdef _WRS_CONFIG_SMP
    if (vxbArmGicLvlVecChk (pVxbArmGicDrvCtrl->pInst, &level, &vector,
                            &srcCpuId) == ERROR)
#else
    if (vxbArmGicLvlVecChk (pVxbArmGicDrvCtrl->pInst, &level, &vector) == ERROR)
#endif
        {
        return;
        }

    do
        {

        /* Loop until no more interrupts are found */

#ifdef _WRS_CONFIG_SV_INSTRUMENTATION

        /*
         * In the ARM architecture, exceptions cannot be locked out
         * with intCpuLock() which makes a two-stage logging approach (i.e.
         * timestamp saved in intEnt and then used here) dangerous...it
         * can lead to out-of sequence events in the event log, thus
         * confusing the parser. So we just use a single stage logging
         * here.
         */

        WV_EVT_INT_ENT (vector)
        loopCnt++;

#endif  /* _WRS_CONFIG_SV_INSTRUMENTATION */


        VXB_INTCTLR_ISR_CALL (&(pVxbArmGicDrvCtrl->isrHandle), vector)

        /* acknowledge the interrupt and restore interrupt level */

#ifdef _WRS_CONFIG_SMP
        vxbArmGicLvlVecAck (pVxbArmGicDrvCtrl->pInst, level, vector, srcCpuId);
#else
        vxbArmGicLvlVecAck (pVxbArmGicDrvCtrl->pInst, level, vector);
#endif
        }
#ifdef _WRS_CONFIG_SMP
        while (vxbArmGicLvlVecChk (pVxbArmGicDrvCtrl->pInst, &level, &vector,
                                   &srcCpuId) != ERROR);
#else
        while (vxbArmGicLvlVecChk (pVxbArmGicDrvCtrl->pInst, &level, &vector)
                                   != ERROR);
#endif

#ifdef _WRS_CONFIG_SV_INSTRUMENTATION
    while (loopCnt-- > 0)
        EVT_CTX_0(EVENT_INT_EXIT);
#endif

    }


/*******************************************************************************
*
* vxbArmGicPreempISR - pre-emptive interrupt exception handler
*
* This handler is fully pre-emptive. In this model, high priority
* interrupts are enabled during the processing of low-priority
* interrupts. Should a high priority interrupt occur, the low-priority
* handler is interrupted and the high priority handler takes over.
*
* \NOMANUAL
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxbArmGicPreempISR()
    {
    int vector, level;
#ifdef _WRS_CONFIG_SMP
    int srcCpuId = 0;
#endif

#ifdef _WRS_CONFIG_SMP
    if (vxbArmGicLvlVecChk (pVxbArmGicDrvCtrl->pInst, &level, &vector,
                            &srcCpuId) == ERROR)
#else
    if (vxbArmGicLvlVecChk (pVxbArmGicDrvCtrl->pInst, &level, &vector) == ERROR)
#endif
        {
        return;
        }

#ifdef _WRS_CONFIG_SV_INSTRUMENTATION

    /*
     * In the ARM architecture, exceptions cannot be locked out with intCpuLock()
     * which makes a two-stage logging approach (i.e. timestamp saved in intEnt
     * and then used here) dangerous...it can lead to out-of sequence events
     * in the event log, thus confusing the parser. So we just use a single
     * stage logging here
     */

    WV_EVT_INT_ENT(vector)
#endif  /* _WRS_CONFIG_SV_INSTRUMENTATION */

    CPU_INTERRUPT_ENABLE;

    VXB_INTCTLR_ISR_CALL (&(pVxbArmGicDrvCtrl->isrHandle), vector)

    CPU_INTERRUPT_DISABLE;

    /* acknowledge the interrupt and restore interrupt level */

#ifdef _WRS_CONFIG_SMP
    vxbArmGicLvlVecAck (pVxbArmGicDrvCtrl->pInst, level, vector, srcCpuId);
#else
    vxbArmGicLvlVecAck (pVxbArmGicDrvCtrl->pInst, level, vector);
#endif /* _WRS_CONFIG_SMP */

    }

/*******************************************************************************
*
* vxbArmGenIntCtlrRegister - register GIC driver
*
* This routine registers the GIC driver with vxbus as a child
* of the PLB bus type.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void vxbArmGenIntCtlrRegister(void)
   {
   vxbDevRegister ((struct vxbDevRegInfo *)&armGicCtlRegistration);
   }

/*******************************************************************************
*
* vxbArmGenIntCtlrInit - initialize the device
*
* This is the vxbArmGenIntrClt initialization routine.  It retrieves and stores
* device driver control data, connects the device driver specific routines into
* the architecture level hooks and initializes the interrupt controller.
* If the BSP needs to create a wrapper routine around any of the architecture
* level routines, it should install the pointer to the wrapper routine after
* calling this routine.
*
* NOTE:
*
* This routine is called early during system initialization (sysHwInit), and
* *MUST NOT* make calls to OS facilities such as memory allocation
* and I/O. It is called from the vxBus routine hardWareInterFaceInit();
*
* RETURNS: N/A
*
* ERRNO
*/

LOCAL void vxbArmGicCtlInit
    (
    VXB_DEVICE_ID pInst
    )
    {
    ARM_GIC_DRV_CTRL * pDrvCtrl;
    HCF_DEVICE * pHcf;

    /* get the HCF device from the instance ID */

    pHcf = hcfDeviceGet (pInst);

    /* if pHcf is NULL, no device is present in hwconf.c */

    if (pHcf == NULL)
        return;

    /* allocate memory for the data */

    pDrvCtrl = (ARM_GIC_DRV_CTRL *)hwMemAlloc (sizeof(ARM_GIC_DRV_CTRL));

    if (pDrvCtrl == NULL)
        return;

    pDrvCtrl->pInst = pInst;

    vxbGicId = pInst;

    /* update the driver control data pointer */

    pInst->pDrvCtrl = pDrvCtrl;

    pVxbArmGicDrvCtrl = pDrvCtrl;

    /* get connectivity info from hwconf */

   intCtlrHwConfGet (pInst, pHcf, &(pDrvCtrl->isrHandle));

   /*
    * resourceDesc {
    * The intMode resource specifies the interrupt mode. Interrupts can be in
    * either preemptive or non-preemptive mode, and the associated macro
    * definition is INT_PREEMPT_MODEL or INT_NON_PREEMPT_MODEL.
    *
    * The maxIntLvl resource specifies the number of interrupt level.
    *
    * The maxCpuNum resource specifies the number of CPU. It is 1 for uni-core
    * system. It may be 2, 3 or 4 for multi-core system.
    }
    */

    if (devResourceGet (pHcf, "intMode", HCF_RES_INT,
                        (void *)&pDrvCtrl->intMode) != OK)
        {
#ifndef _VXBUS_BASIC_HWMEMLIB
        hwMemFree((char *)pDrvCtrl);
#endif
        return;
        }

    pDrvCtrl->gicBase = pInst->pRegBase[0];
    armGicBase = (UINT32)pDrvCtrl->gicBase;

    vxbRegMap(pInst, 0, &pDrvCtrl->handle);

    /*
     * Get the distributor and CPU interface register base addresses,
     * using the default (for backward-compatibility with ARM11 MPCore and
     * Cortex-A9) if they're not supplied by the BSP.
     */

    if (devResourceGet (pHcf, "distOffset", HCF_RES_INT,
                        (void *)&pDrvCtrl->gicDistOffset) != OK)
        {

        /* not found, so use default */

        pDrvCtrl->gicDistOffset = armGicDistOffset;
        }
    else
        armGicDistOffset = pDrvCtrl->gicDistOffset;


    if (devResourceGet (pHcf, "cpuOffset", HCF_RES_INT,
                        (void *)&pDrvCtrl->gicCpuOffset) != OK)
        {

        /* not found, so use default */

        pDrvCtrl->gicCpuOffset = armGicCpuOffset;
        }
    else
        armGicCpuOffset = pDrvCtrl->gicCpuOffset;

    /* get the maximum interrupt level number */

    if (devResourceGet (pHcf, "maxIntLvl", HCF_RES_INT,
                        (void *)&pDrvCtrl->gicLvlNum) != OK)
        {
#ifndef _VXBUS_BASIC_HWMEMLIB
        hwMemFree((char *)pDrvCtrl);
#endif
        return;
        }

    armGicLinesNum = pDrvCtrl->gicLvlNum;
    if (armGicLinesNum > GIC_INT_MAX_NUM)
        {
#ifndef _VXBUS_BASIC_HWMEMLIB
	hwMemFree((char *)pDrvCtrl);
#endif
	return;
	}

#ifdef _WRS_CONFIG_SMP

    /* get the maximum CPU numbers */

    if (devResourceGet (pHcf, "maxCpuNum", HCF_RES_INT,
                        (void *)&pDrvCtrl->gicCpuNum) != OK)
        {
#ifndef _VXBUS_BASIC_HWMEMLIB
        hwMemFree((char *)pDrvCtrl);
#endif
        return;
        }
#else
    pDrvCtrl->gicCpuNum = 1;
#endif /* _WRS_CONFIG_SMP */

    pInst->pMethods = &gicIntCtlr_methods[0];

    /* Install three pointers for legacy type intEnable/intDisable/intLevelSet */

    sysIntLvlEnableRtn  = (FUNCPTR)sysArmGicEnable;
    sysIntLvlDisableRtn = (FUNCPTR)sysArmGicDisable;
    sysIntLvlChgRtn     = (FUNCPTR)vxbArmGicHwLvlChg;

    if (_func_intConnectRtn == NULL)
        _func_intConnectRtn = sysArmGicConnect;

    if (_func_intDisconnectRtn == NULL)
        _func_intDisconnectRtn = sysArmGicDisconnect;

    /* initialize the GIC */

    vxbArmGicDevInit (pInst);

    /* assign ISR to hook (defined in excArchLib.c) */

    if (pDrvCtrl->intMode & INT_PREEMPT_MODEL)
        EXC_CONNECT_INTR_RTN (vxbArmGicPreempISR);
    else
        EXC_CONNECT_INTR_RTN (vxbArmGicNonPreempISR);
    }

/*******************************************************************************
*
* vxbArmGenIntCtlrInit2 - second level initialization of the ARM GIC
*
* This is the second level ARM GIC initialization routine.  Nothing needs to
* be done during this level of initialization.
*
* RETURNS: N/A
*
* ERRNO
*/

LOCAL void vxbArmGicCtlInit2
    (
    VXB_DEVICE_ID pInst
    )
    {
    }

/*******************************************************************************
*
* vxbArmGicDevInit - initialize the interrupt controller
*
* This routine initializes the interrupt controller device, disabling all
* interrupt sources. It connects the device driver specific routines
* into the architecture level hooks. If the BSP needs to create a wrapper
* routine around any of the architecture level routines, it should install the
* pointer to the wrapper routine after calling this routine.
*
* RETURNS: OK or ERROR if parameter is invalid.
*
* ERRNO: N/A
*/

LOCAL STATUS vxbArmGicDevInit
    (
    VXB_DEVICE_ID pInst
    )
    {

    ARM_GIC_DRV_CTRL * pDrvCtrl;

    /* if parameters are invalid, return ERROR */

    if ((pInst == NULL) || (pInst->pDrvCtrl == NULL))
        return (ERROR);

    /* retrieve the data */

    pDrvCtrl = (ARM_GIC_DRV_CTRL *)(pInst->pDrvCtrl);

    pDrvCtrl->gicLvlCurrent = GIC_INT_ALL_ENABLED;

    vxbArmGicIntDevInit(0);

    vxbArmGicLvlChg (pInst, GIC_INT_ALL_ENABLED); /* enable all levels */

    return OK;
    }

/*******************************************************************************
*
* vxbArmGicIntDevInit - initialize the interrupt controller
*
* This routine initializes the interrupt controller device, disabling all
* interrupt sources. It connects the device driver specific routines into the
* architecture level hooks.  If the BSP needs to create a wrapper routine
* around any of the architecture level routines, it should install the pointer
* to the wrapper routine after calling this routine.
*
* Note that for multicore processor, some registers are banked for each process.
* This routine uses CPU0 to initialize the common resources and banked
* registers are processed by every processor.
*
* RETURNS: always OK
*
* ERRNO: N/A
*/

LOCAL STATUS vxbArmGicIntDevInit
    (
    int cpuNum
    )
    {
    int                       i;
    UINT32                    aGicPrio = 0;
    UINT32                    aGicCfg = 0;
    STATUS                      stat;
    HCF_DEVICE                * pHcf;
    struct intrCtlrPriority   * pPrioTable;
    struct intrCtlrTrigger    * pTrigTable = NULL;
    int                         tableSize;
    void *                      pValue;

#ifndef _WRS_CONFIG_WRHV_GUEST

    pHcf = hcfDeviceGet (vxbGicId);
    if (pHcf == NULL)
        {
        return (ERROR);
        }

    /* disable distributor */

    if (cpuNum == 0) /* CPU0 */
        {
        *GIC_Control = 0;
        }

    /* disable this processor's CPU interface */

    *GIC_CPU_Control = 0;

    /* disable all PPI interrupts */

    *GIC_IntEnClr(0) = SWAP32 (ALL_PPI_INT_MASK);

    /* Get the max priority */

    *GIC_Prio(0) = SWAP32 (0xff);
    armGicPriorityLvlMax = SWAP32 (*GIC_Prio(0));
    *GIC_Prio(0) = 0;

    /* clear all pending PPI and SGI interrupts in the distributor */

    *GIC_IntPendClr(0) = SWAP32 (ALL_PPI_INT_MASK | ALL_SGI_INT_MASK);

    /* set default priority for all PPI and SGI interrupts to level 0(highest) */

    for (i = 0; i < SPI_START_INT_NUM; i += PRIOS_PER_WORD)
        *GIC_Prio(i) = GIC_IPI_PRIORITY;

    if (cpuNum == 0) /* CPU0 */
        {

        /*
         * Disable all SPI interrupts
         * Clear all pending SPI interrupts in the distributor
         */

        for(i = SPI_START_INT_NUM; i < GIC_INT_MAX_NUM; i += BITS_PER_WORD)
            {
            *GIC_IntEnClr(i) = SWAP32 (0xffffffff);
            *GIC_IntPendClr(i) = SWAP32 (0xffffffff);
            }

        /*
         * set default priority for all SPI interrupts to level 0 and direct all
         * interrupts to go to CPU 0
         */

        for (i = SPI_START_INT_NUM; i < armGicLinesNum; i += PRIOS_PER_WORD)
            {
            *GIC_Prio(i) = GIC_SPI_PRIORITY_DEFAULT;
            *GIC_CPUTarg(i) = SWAP32 ( GIC_CPU_DIR_DEFAULT );
            }

        /* setting whether 1-N/N-N and Level/Edge triggered */

        for (i = SPI_START_INT_NUM; i < armGicLinesNum; i += CONFIGS_PER_WORD)
            *GIC_Config(i) = SWAP32 ( GIC_INT_ONEMINUS_HIGH ); /* 1-N, Level */

        /*
         * get trigger table from hwconf
         *
         * resourceDesc {
         * The trigger resource specifies a pointer to a
         * intrCtlrTrigger structure which configures triggering
         * for particular interrupt vectors. }
         */

        stat = devResourceGet(pHcf, "trigger", HCF_RES_ADDR, &pValue);
        pTrigTable = (struct intrCtlrTrigger *)pValue;
        if (stat == OK)
            {
            stat = devResourceGet(pHcf, "triggerTableSize", HCF_RES_INT,
                                  (void *) &tableSize);
            if (stat == OK)
                {
                for ( i = 0 ; i < tableSize ; i++ )
                    {
                    if (pTrigTable->inputPin > armGicLinesNum)
                        continue;

                    /*
                     * The GIC does not permit use of the Interrupt
                     * Configuration Register to program the trigger
                     * mode of PPIs.
                     */

                    if (pTrigTable->inputPin >= SPI_START_INT_NUM)
                        {
                        aGicCfg = SWAP32 (*GIC_Config(pTrigTable->inputPin));

                        /*
                         * Trigger configure type:
                         *
                         * VXB_INTR_TRIG_FALLING_EDGE ==  edge triggered,
                         *                                falling edge
                         * VXB_INTR_TRIG_RISING_EDGE  ==  edge triggered,
                         *                                rising edge
                         * VXB_INTR_TRIG_ACTIVE_LOW   ==  level sensitive,
                         *                                active low
                         * VXB_INTR_TRIG_ACTIVE_HIGH  ==  level sensitive,
                         *                                active high
                         */

                        if (pTrigTable->trigger & VXB_INTR_TRIG_LEVEL)
                            {
                            aGicCfg &=
                               ~(BIT (((pTrigTable->inputPin % CONFIGS_PER_WORD)
                               * GIC_INT_TRIGGER_SHIFT + 1)));
                            }
                        else if (pTrigTable->trigger & VXB_INTR_TRIG_EDGE)
                            {
                            aGicCfg |=
                                 BIT (((pTrigTable->inputPin % CONFIGS_PER_WORD)
                                 * GIC_INT_TRIGGER_SHIFT + 1));
                            }
                        *GIC_Config(pTrigTable->inputPin) = SWAP32 (aGicCfg);
                        pTrigTable++;
                        }
                    }
                }
            }
        }

    /*
     * get priority table from hwconf
     *
     * resourceDesc {
     * The priority resource specifies a pointer to a
     * intrCtlrPriority structure which assigns priority
     * levels to particular interrupt vectors. }
     */

    stat = devResourceGet(pHcf, "priority", HCF_RES_ADDR, &pValue);
    pPrioTable = (struct intrCtlrPriority *)pValue;
    if (stat == OK)
        {
        stat = devResourceGet(pHcf, "priorityTableSize", HCF_RES_INT,
                              (void *) &tableSize);
        if (stat == OK)
            {
            for ( i = 0 ; i < tableSize ; i++ )
                {

                /* use CPU0 to set the common SPI priority */

                if ((pPrioTable->inputPin >= SPI_START_INT_NUM) &&
                    (cpuNum != 0))
                    continue;

                aGicPrio = SWAP32 (*GIC_Prio(pPrioTable->inputPin));

                /*
                 * Priority range:
                 *
                 * 0x0  == GIC_INT_HIGHEST_PRIORITY, the highest priority
                 * 0xFF == GIC_INT_LOWEST_PRIORITY, the lowest priority
                 */

                if (pPrioTable->priority > armGicPriorityLvlMax)
                    {
                    aGicPrio |=
                        ((armGicPriorityLvlMax & GIC_INT_PRIORITY_MASK)
                        << ((pPrioTable->inputPin % PRIOS_PER_WORD)
                        * GIC_INT_PRIORITY_SHIFT));
                    }
                else if ((pPrioTable->inputPin >= SPI_START_INT_NUM) &&
                         (pTrigTable->inputPin <= armGicLinesNum) &&
                         (pPrioTable->priority < GIC_PRIORITY_LEVEL_STEP))
                    {
                    aGicPrio |=
                        ((GIC_PRIORITY_LEVEL_STEP & GIC_INT_PRIORITY_MASK)
                        << ((pPrioTable->inputPin % PRIOS_PER_WORD)
                        * GIC_INT_PRIORITY_SHIFT));
                    }
                else
                    {
                    aGicPrio |=
                        ((pPrioTable->priority & GIC_INT_PRIORITY_MASK)
                        << ((pPrioTable->inputPin % PRIOS_PER_WORD)
                        * GIC_INT_PRIORITY_SHIFT));
                    }
                *GIC_Prio(pPrioTable->inputPin) = SWAP32 (aGicPrio);
                pPrioTable++;
                }
            }
        }

    /* enable all interrupt priorities */

    *GIC_CPU_PriMask = SWAP32 (GIC_INT_ALL_ENABLED);

    /* split group priority and subpriority */

    if (pVxbArmGicDrvCtrl != NULL)
        {
        if (pVxbArmGicDrvCtrl->intMode & INT_PREEMPT_MODEL)
            {
            *GIC_CPU_BinPoint = 0;
            }
        else
            {
            *GIC_CPU_BinPoint = SWAP32 (GIC_CPU_BINP_DEFAULT);
            }
        }

    /* enable this processor's CPU interface */

    *GIC_CPU_Control = SWAP32 (GIC_CONTROL_ENABLE);

    if (cpuNum == 0)
        {

        /* enable distributor */

        *GIC_Control = SWAP32(GIC_CONTROL_ENABLE);
        }
#else /* _WRS_CONFIG_WRHV_GUEST */

    /* RAB - for guest only enable distributor */

    *GIC_Control = GIC_CONTROL_ENABLE;

#endif /* _WRS_CONFIG_WRHV_GUEST */

    return OK;
    }

/*******************************************************************************
*
* sysArmGicConnect - interrupt instance connect handler
*
* This function implements the connection of interrupt handler. It's used to
* support intConnect(...).
*
* NOTE:
*
* This routine is called somewhere by legacy driver.
*
* RETURNS: OK if operation succeed else ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS sysArmGicConnect
    (
    VOIDFUNCPTR *  vector,     /* interrupt vector to attach to */
    VOIDFUNCPTR    routine,    /* routine to be called */
    int            parameter   /* parameter to be passed to routine */
    )
    {
    ARM_GIC_DRV_CTRL * pDrvCtrl = vxbGicId->pDrvCtrl;

    if ((int)(vector) >= pDrvCtrl->gicLvlNum)
        return(ERROR);

    return(intCtlrISRAdd(&pDrvCtrl->isrHandle, (int)vector, routine,
                         (void *)parameter));
    }

/*******************************************************************************
*
* vxbArmGicConnect - VxBus instance connect handler
*
* This function implements the VxBus InstConnect handler for an GIC instance.
*
* RETURNS: OK if operation succeed else ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbArmGicConnect
    (
    VXB_DEVICE_ID       pIntCtlr,
    VXB_DEVICE_ID       pDev,
    int                 index,
    void                (*pIsr)(void * pArg),
    void *              pArg,
    int *               pInputPin
    )
    {
    ARM_GIC_DRV_CTRL * pDrvCtrl = pIntCtlr->pDrvCtrl;
    int     inputPin;

    /* get interrupt input pin */

    inputPin = intCtlrPinFind (pDev, index, pIntCtlr, &pDrvCtrl->isrHandle);

    if (inputPin == ERROR)
        return (ERROR);

    *pInputPin = inputPin;

    /* assign the ISR and arg to the specified cpu input pin */

    if (intCtlrISRAdd(&pDrvCtrl->isrHandle,
                  inputPin, pIsr, pArg) != OK)
        return (ERROR);

    return (OK);
    }

/*******************************************************************************
*
* sysArmGicDisconnect - disconnect a C routine from an interrupt
*
* This routine disconnects a specified C routine from a specified
* interrupt vector. It's used to support intDisconnect(...).
*
* This routine is called somewhere by legacy driver.
*
* RETURNS: OK, or ERROR if <vector> is out of range.
*
* ERRNO: N/A
*/

LOCAL STATUS sysArmGicDisconnect
    (
    VOIDFUNCPTR *  vector,     /* interrupt vector to detach from */
    VOIDFUNCPTR    routine,    /* routine to be disconnected      */
    int            parameter   /* parameter to be matched         */
    )
    {
    ARM_GIC_DRV_CTRL * pDrvCtrl = vxbGicId->pDrvCtrl;

    if ((int)(vector) >= pDrvCtrl->gicLvlNum)
        return(ERROR);

    return(intCtlrISRRemove(&pDrvCtrl->isrHandle, (int)vector, routine,
                            (void *)parameter));
    }

/*******************************************************************************
*
* vxbArmGicDisconnect - disconnect device interrupt
*
* This routine disconnects the supplied routine and arg from the interrupt
* input found with intCtlrPinFind.
*
* \NOMANUAL
*
* RETURNS: OK if operation succeed else ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbArmGicDisconnect
    (
    VXB_DEVICE_ID       pIntCtlr,
    VXB_DEVICE_ID       pDev,
    int                 index,
    VOIDFUNCPTR         pIsr,
    void *              pArg
    )
    {
    ARM_GIC_DRV_CTRL * pDrvCtrl = pIntCtlr->pDrvCtrl;
    int inputPin;

    /* get interrupt input pin */

    inputPin = intCtlrPinFind (pDev, index, pIntCtlr, &pDrvCtrl->isrHandle);
    if (inputPin == ERROR)
        return (ERROR);

    /* remove the ISR and arg from the specified cpu input pin */

    if (intCtlrISRRemove(&pDrvCtrl->isrHandle, inputPin, pIsr, pArg) != OK)
        return (ERROR);

    return (OK);
    }

/*******************************************************************************
*
* sysArmGicISREnable - enable interrupt handler
*
* This routine enables interrupt for the configured input pin
*
* \NOMANUAL
*
* RETURNS: OK if operation succeed else ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS sysArmGicISREnable
    (
    struct intCtlrHwConf *	pEntries,
    int			            inputPin
    )
    {
    struct intCtlrISRChainEntry * pChain;
    UINT32                        flagValue;
    VOIDFUNCPTR                   func;

    /* make sure top-level enabled */

    flagValue = intCtlrTableFlagsGet(pEntries, inputPin);
    flagValue |= VXB_INTCTLR_FLG_ENABLE;
    intCtlrTableFlagsSet(pEntries, inputPin, flagValue);

    func = intCtlrTableIsrGet(pEntries, inputPin);
    if ( func == intCtlrChainISR )
        {
        pChain = (struct intCtlrISRChainEntry *)
        intCtlrTableArgGet(pEntries, inputPin);
        while (pChain != NULL)
            {
            pChain->flags |= VXB_INTCTLR_FLG_ENABLE;
            pChain = pChain->pNext;
            }
        }

    return(OK);
    }

/*******************************************************************************
*
* sysArmGicEnable - enable an interrupt handler from the vector table
*
* This routine enables interrupt handler from the vector table. It's used
* to support intEnable(...).
*
* This routine is called somewhere by legacy driver.
*
* \NOMANUAL
*
* RETURNS: OK if operation succeed else ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS sysArmGicEnable
    (
    int vector
    )
    {
    ARM_GIC_DRV_CTRL *pDrvCtrl = vxbGicId->pDrvCtrl;

    if ((int)(vector) >= pDrvCtrl->gicLvlNum)
        return(ERROR);

    if (sysArmGicISREnable(&(pDrvCtrl->isrHandle), vector) != OK)
        return (ERROR);

    vxbArmGicLvlEnable(pDrvCtrl->pInst, vector);
    return (OK);
    }

/*******************************************************************************
*
* vxbArmGicEnable - enable device interrupt
*
* This routine enables interrupt for the configured input pin
*
* \NOMANUAL
*
* RETURNS: OK if operation succeed else ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbArmGicEnable
    (
    VXB_DEVICE_ID       pIntCtlr,
    VXB_DEVICE_ID       pDev,
    int                 index,
    VOIDFUNCPTR         pIsr,
    void *              pArg
    )
    {
    ARM_GIC_DRV_CTRL * pDrvCtrl = pIntCtlr->pDrvCtrl;
    int inputPin;

    /* get interrupt input pin */

    inputPin = intCtlrPinFind (pDev, index, pIntCtlr, &pDrvCtrl->isrHandle);
    if (inputPin == ERROR)
        return (ERROR);

    /* enable ISR */

    if (intCtlrISREnable(&pDrvCtrl->isrHandle, inputPin, pIsr, pArg) != OK)
        return (ERROR);

    vxbArmGicLvlEnable(pDrvCtrl->pInst, inputPin);

    return (OK);
    }

/*******************************************************************************
*
* sysArmGicISRDisable - disable interrupt handler
*
* This routine disables interrupt for the configured input pin
*
* \NOMANUAL
*
* RETURNS: OK if operation succeed else ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS sysArmGicISRDisable
    (
    struct intCtlrHwConf *  pEntries,
    int                     inputPin
    )
    {
    struct intCtlrISRChainEntry * pChain;
    UINT32                        flagValue;
    VOIDFUNCPTR                   func;

    /* make sure top-level disabled */

    flagValue = intCtlrTableFlagsGet(pEntries, inputPin);
    flagValue &= ~VXB_INTCTLR_FLG_ENABLE;
    intCtlrTableFlagsSet(pEntries, inputPin, flagValue);

    func = intCtlrTableIsrGet(pEntries, inputPin);
    if ( func == intCtlrChainISR )
        {
        pChain = (struct intCtlrISRChainEntry *)
        intCtlrTableArgGet(pEntries, inputPin);
        while (pChain != NULL)
            {
            pChain->flags &= ~VXB_INTCTLR_FLG_ENABLE;
            pChain = pChain->pNext;
            }
        }

    return(OK);
    }

/*******************************************************************************
*
* sysArmGicDisable - disable an interrupt handler from the vector table
*
* This routine disables interrupt handler from the vector table. It's used
* to support intDisable(...).
*
* This routine is called somewhere by legacy driver.
*
* \NOMANUAL
*
* RETURNS: OK if operation succeed else ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS sysArmGicDisable
    (
    int vector
    )
    {
    ARM_GIC_DRV_CTRL *pDrvCtrl = vxbGicId->pDrvCtrl;

    if ((int)(vector) >= pDrvCtrl->gicLvlNum)
        return(ERROR);

    if (sysArmGicISRDisable(&(pDrvCtrl->isrHandle), vector) != OK)
        return (ERROR);

    vxbArmGicLvlDisable(pDrvCtrl->pInst, vector);
    return (OK);
    }

/*******************************************************************************
*
* vxbArmGicDisable - disable device interrupt
*
* This routine disables interrupt for the configured input pin
*
* \NOMANUAL
*
* RETURNS: OK if operation succeed else ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbArmGicDisable
    (
    VXB_DEVICE_ID       pIntCtlr,
    VXB_DEVICE_ID       pDev,
    int                 index,
    VOIDFUNCPTR         pIsr,
    void *              pArg
    )
    {
    ARM_GIC_DRV_CTRL * pDrvCtrl = pIntCtlr->pDrvCtrl;
    BOOL allDisabled;
    int inputPin;

    /* get interrupt input pin */

    inputPin = intCtlrPinFind (pDev, index, pIntCtlr, &pDrvCtrl->isrHandle);
    if (inputPin == ERROR)
        return (ERROR);

    /* disable ISR */

    allDisabled = intCtlrISRDisable(&pDrvCtrl->isrHandle, inputPin, pIsr, pArg);

    if (allDisabled)
        vxbArmGicLvlDisable(pDrvCtrl->pInst, inputPin);

    return (OK);
    }

/*******************************************************************************
*
* vxbArmGicLvlVecChk - check for and return any pending interrupts
*
* This routine interrogates the hardware to determine the highest priority
* interrupt pending. It returns the vector associated with that interrupt,
* and also the level of that interrupt.
*
* This routine must be called with CPU interrupts disabled.
*
* The return value ERROR indicates that no pending interrupt was found and
* that the level and vector values were not returned.
*
* RETURNS: OK or ERROR if no interrupt is pending.
*
* ERRNO: N/A
*/

STATUS  vxbArmGicLvlVecChk
    (
    VXB_DEVICE_ID pInst,
    int * pLevel,
    int * pVector
#ifdef _WRS_CONFIG_SMP
    , int * pSrcCpuId
#endif
    )
    {
    UINT32 currentLevel, levelBak;

    /* read pending interrupt register and mask undefined bits */

    levelBak = SWAP32(*GIC_CPU_IntAck);
    currentLevel = levelBak & GIC_INT_SPURIOUS;

    /*
     * If no interrupt is pending, register will have a value of 1023,
     * return ERROR
     */

    if (currentLevel == GIC_INT_SPURIOUS)
        {
        return ERROR;
        }

#ifdef _WRS_CONFIG_SMP

    /* check if interrupt is IPI */

    if(currentLevel < SGI_INT_MAX)
        {
        /*
         * Level for IPI is defined and processed by software.
         * Hardware always use 0 - SGI_INT_MAX as the IPI interrupt level.
         * When we find that this is a SGI interrupt, we add armGicLinesNum to
         * the current SGI level (0 - SGI_INT_MAX), in other words, the SGI
         * level is redefined by software to declare that this is a specific
         * interrupt, then we need save the source CPU ID to ack this interrupt.
         */

        *pSrcCpuId = levelBak & GIC_SGI_SRC_CPU_ID_MASK;
        *pLevel = (currentLevel + armGicLinesNum);
        *pVector = IVEC_TO_INUM(currentLevel + armGicLinesNum);
        }
    else
#endif /* _WRS_CONFIG_SMP */
        {
        *pLevel = currentLevel;

        /* fetch, or compute the interrupt vector number */

        *pVector = IVEC_TO_INUM(currentLevel);
        }

    return OK;
    }

/*******************************************************************************
*
* vxbArmGicLvlVecAck - acknowledge the current interrupt
*
* This routine acknowledges the current interrupt cycle. The level and vector
* values are those generated during the vxbArmGicLvlVecChk() routine for this
* interrupt cycle.
*
* RETURNS: OK or ERROR if level is invalid.
*
* ERRNO
*/

STATUS  vxbArmGicLvlVecAck
    (
    VXB_DEVICE_ID pInst,
    int level,    /* old interrupt level to be restored */
    int vector    /* current interrupt vector, if needed */
#ifdef _WRS_CONFIG_SMP
    , int srcCpuId
#endif /* _WRS_CONFIG_SMP */
    )
    {
    UINT32 maxIntLines;

#ifdef _WRS_CONFIG_SMP
    maxIntLines = armGicLinesNum + ARM_GIC_IPI_COUNT;
#else
    maxIntLines = armGicLinesNum;
#endif /* _WRS_CONFIG_SMP */

    /* Validity check for level. */

    if (level < 0 || level >= maxIntLines ||
        (level >= SGI_INT_MAX && level < PPI_START_INT_NUM))
        return ERROR;

    /*
     * Ack the interrupt. It's implemented on the CPU interface to the
     * interrupt distributor.
     */

#ifdef _WRS_CONFIG_SMP
    if (level >= armGicLinesNum)
        {
        /*
         * SGI is used to implement the IPI. The source CPU ID must be carried
         * to acknowledge the SGI interrupts.
         */

        level -= armGicLinesNum;
        level |= srcCpuId;
        }
#endif /* _WRS_CONFIG_SMP */

    *GIC_CPU_EOInt = SWAP32( level );

    return OK;
    }

/*******************************************************************************
*
* vxbArmGicLvlChg - change the interrupt level value
*
* This routine sets the interrupt priority mask.
* All levels up to the specified level are disabled.
* All levels above the specified level will be enabled.
* This routine must be called with interrupts disabled.
*
* RETURNS: previous interrupt level or ERROR if level is invalid.
*
* ERRNO: N/A
*/

int  vxbArmGicLvlChg
    (
    VXB_DEVICE_ID pInst,
    int level    /* new interrupt level */
    )
    {
    int oldLevel;
    ARM_GIC_DRV_CTRL * pDrvCtrl;

    /* if parameters are invalid, return ERROR */

    if ((pInst == NULL) || (pInst->pDrvCtrl == NULL))
        return (ERROR);

    /* retrieve the data */

    pDrvCtrl = (ARM_GIC_DRV_CTRL *)(pInst->pDrvCtrl);

    oldLevel = pDrvCtrl->gicLvlCurrent;

    if (level <= GIC_IPI_PRIORITY)
        level = oldLevel;
    if (level > armGicPriorityLvlMax)
        level = armGicPriorityLvlMax;

    /* change current interrupt level */

    pDrvCtrl->gicLvlCurrent = level;

    *GIC_CPU_PriMask = SWAP32 (level);

    VX_SYNC_BARRIER();

    return oldLevel;
    }

/*******************************************************************************
*
* vxbArmGicLvlEnable - enable a single interrupt level
*
* This routine enables a specific interrupt level at the interrupt distributor.
* The enabled level will be allowed to generate an interrupt to the configured
* CPU(s). Without being enabled, the interrupt is blocked.
*
* RETURNS: OK or ERROR if level is invalid.
*
* ERRNO: N/A
*/

LOCAL STATUS  vxbArmGicLvlEnable
    (
    VXB_DEVICE_ID pInst,
    int level  /* level to be enabled */
    )
    {
    int key;

    /*
     * For SGI interrupt, we always return OK, since SGI interrupt can not be
     * disabled.
     */

    if ((level >= 0) && (level < SGI_INT_MAX))
        return OK;

    /* Validity check for level. */

    if (level < PPI_START_INT_NUM || level >= armGicLinesNum )
        return ERROR;

    key = intCpuLock ();              /* LOCK INTERRUPTS */

    *GIC_IntEnable(level) = SWAP32( BIT(level)); /* enable interrupt */

    intCpuUnlock (key);               /* UNLOCK INTERRUPTS */

    return OK;
    }

/*******************************************************************************
*
* vxbArmGicLvlDisable - disable a single interrupt level
*
* This routine disables a specific interrupt level at the interrupt distributor.
* The disabled level is prevented from generating an interrupts.
*
* RETURNS: OK or ERROR if level is invalid.
*
* ERRNO: N/A
*/

LOCAL STATUS vxbArmGicLvlDisable
    (
    VXB_DEVICE_ID pInst,
    int level  /* level to be disabled */
    )
    {
    int key;

    /* Validity check for level. */

    if (level < PPI_START_INT_NUM || level >= armGicLinesNum)
        return ERROR;

    key = intCpuLock ();

    *GIC_IntEnClr(level) = SWAP32( BIT(level));

    intCpuUnlock (key);

    return OK;
    }

/******************************************************************************
*
* vxbArmGicHwLvlChg - change the interrupt level value
*
* This routine sets the current interrupt level to the specified level.
*
* \NOMANUAL
*
* RETURNS: Previous interrupt level.
*
* ERRNO: N/A
*/

LOCAL int vxbArmGicHwLvlChg
    (
    int level
    )
    {
    return vxbArmGicLvlChg(pVxbArmGicDrvCtrl->pInst,level);
    }

#ifdef INCLUDE_SHOW_ROUTINES

/*******************************************************************************
*
* vxbArmGicDataShow - show data acquired by vxBus
*
* This routine shows data acquired by vxBus.
*
* RETURNS: N/A
*/

void vxbArmGicDataShow
    (
    VXB_DEVICE_ID  pInst,
    int *dummy
    )
    {
    ARM_GIC_DRV_CTRL * pDrvCtrl;

    /* check if parameters are invalid */

    if ((pInst == NULL) || (pInst->pDrvCtrl == NULL))
        return;

    /* retrieve the data */

    pDrvCtrl = (ARM_GIC_DRV_CTRL *)(pInst->pDrvCtrl);

    printf ("\nGIC Configuration Data acquired by vxBus\n\n");

    printf ("\n  ARM GIC Info :                       \n");

    printf ("  pInst              = 0x%08x\n", pInst);

    printf ("  GicBase              = 0x%08x\n",
                    pDrvCtrl->gicBase);
    printf ("  GicLvlCurrent              = 0x%08x\n",
                    pDrvCtrl->gicLvlCurrent);
    printf ("  gicDistOffset              = 0x%08x\n",
                    pDrvCtrl->gicDistOffset);
    printf ("  gicCpuOffset               = 0x%08x\n",
                    pDrvCtrl->gicCpuOffset);
    }

#endif /* INCLUDE_SHOW_ROUTINES */


#ifdef _WRS_CONFIG_SMP

/*******************************************************************************
*
* vxbArmGicHwEnable - enable hardware interrupt
*
* This routine enables the interrupt for the configured input pin
*
* \NOMANUAL
*
* RETURNS: OK if operation succeed else ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbArmGicHwEnable
    (
    int inputPin
    )
    {
    return vxbArmGicLvlEnable(pVxbArmGicDrvCtrl->pInst, inputPin);
    }

/******************************************************************************
*
* vxbArmGicHwDisable - disable device interrupt
*
* This routine disables the interrupt the configured input pin
*
* \NOMANUAL
*
* RETURNS: OK if operation succeed else ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbArmGicHwDisable
    (
    int inputPin
    )
    {
    return vxbArmGicLvlDisable(pVxbArmGicDrvCtrl->pInst, inputPin);
    }

/*******************************************************************************
*
* sysArmGicDevInit - initialize interrupt controller.
*
* This routine initializes interrupt controller. It may be called by non-CPU0
* routines in BSP, or somewhere else.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
/* void sysArmGicDevInit (void) */ /* origin */
STATUS sysArmGicDevInit (void)  /* jc */
{
    int cpuNum = vxCpuIndexGet();

    (void)vxbArmGicIntDevInit(cpuNum);

	return OK;
}

/*******************************************************************************
*
* vxbArmGicIntReroute - reroute interrupt to specified CPU
*
* This routine reroutes device interrupt to requested CPU. Note that the cpu is
* specified in a cpuset_t type, and this would allow for multiple cpus to be
* bundled with the interrupt. Multicore processor can ensure that only one CPU
* will be triggered if the interrupt occurs.
*
* RETURNS: OK if operation succeed else ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbArmGicIntReroute
    (
    VXB_DEVICE_ID       pDev,
    int                 index,
    cpuset_t            destCpu
    )
    {
    struct intCtlrHwConf * isrHandle;
    BOOL flag;
    int targCpu = 0;
    int bitCnt = 0;
    int i;
    int inputPin;
    int tmpData;
    UINT32 oldIntCfg;

    isrHandle = &(((ARM_GIC_DRV_CTRL *)(vxbGicId->pDrvCtrl))->isrHandle);

    /* convert cpuset_t cpu number to numeric cpu number */

    for (i = 0; i < ((ARM_GIC_DRV_CTRL *)(vxbGicId->pDrvCtrl))->gicCpuNum; i++)
        {
        if (destCpu & (1 << i))
            {
            bitCnt++;
            targCpu = i;
            }
        }

    /* make sure destCpu is a proper cpuset_t value */

    if (bitCnt != 1)
        return (ERROR);

    /* find the device input pin number */

    inputPin = intCtlrPinFind (pDev, index, vxbGicId, isrHandle);
    if (inputPin == ERROR)
        return (ERROR);

    /* make sure pin is allocated */

    VXB_INTCTLR_PINENTRY_ALLOCATED(isrHandle, inputPin, flag);

    if (!flag)
        return (ERROR);

    oldIntCfg = SWAP32(*GIC_IntEnable(inputPin));

    /* disable interrupt source */

    vxbArmGicLvlDisable(pDev, inputPin);

    /* clear the current configuration */

    tmpData = 0xff << ((inputPin % TARGETS_PER_WORD) * 8);
    tmpData = SWAP32( *GIC_CPUTarg(inputPin) ) & ~tmpData;

    /* set route bit */

    tmpData |= ((UINT32) (0x1 << (UINT32)targCpu) << \
               ((inputPin % TARGETS_PER_WORD) * 8));

    *GIC_CPUTarg(inputPin) = SWAP32( tmpData );

    if ((oldIntCfg & SWAP32 (BIT (inputPin))) != 0)
        {
        /* enable interrupt source */

        vxbArmGicLvlEnable(pDev, inputPin);
        }

    return OK;
    }

/*******************************************************************************
*
* vxbArmGicCpuReroute - reroute interrupts to specified CPU
*
* This routine reroutes interrupts that are configured in hwconf.c for a CPU
* other than the default CPU to that CPU.
*
* RETURNS: OK if operation succeed else ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbArmGicCpuReroute
    (
    VXB_DEVICE_ID       pDev,
    void *              destCpu
    )
    {
    ARM_GIC_DRV_CTRL * pDrvCtrl = pDev->pDrvCtrl;
    struct intCtlrHwConf *isrHandle = &(pDrvCtrl->isrHandle);
    int i;
    int configCpu, tmpData;
    BOOL flag;
    void (*func)();

    if ((int)destCpu >= pDrvCtrl->gicCpuNum)
        return (ERROR);

    /* move interrupts to requested cpu */

    for (i = 0; i < armGicLinesNum; i++)
        {
        /* verify this is an allocated pin so NULL references are avoided */

        VXB_INTCTLR_PINENTRY_ALLOCATED(isrHandle, i, flag);

        if (flag)
            {
            /* only move interrupts that are configured with an ISR */

            GIC_ISR(isrHandle, i, func);

            if (func == NULL || func == intCtlrStrayISR)
               continue;

            /* move interrupts that are configured for the requested CPU */

            GIC_DESTCPU(isrHandle, i, configCpu);

            if (configCpu == (int)destCpu)
                {
                /* disable interrupt source */

                vxbArmGicLvlDisable(pDev, i);

                /* clear the current configuration */

                tmpData = 0xff << ((i % TARGETS_PER_WORD) * 8);
                tmpData = SWAP32( *GIC_CPUTarg(i) ) & ~tmpData;

                /* set route bit */

                tmpData |= ((UINT32) (0x1 << (UINT32)destCpu) << \
                           ((i % TARGETS_PER_WORD) * 8));

                *GIC_CPUTarg(i) = SWAP32( tmpData );

                /* enable interrupt source */

                vxbArmGicLvlEnable(pDev, i);

                }
            }
        }

    return (OK);
    }

/*******************************************************************************
*
* vxbArmGicIpiCtlGet - retrieve IPI control structure
*
* This routine retrieves IPI control structure.
*
* RETURNS: pointer of IPI control structure
*
* ERRNO: N/A
*/

LOCAL VXIPI_CTRL_INIT * vxbArmGicIpiCtlGet
    (
    VXB_DEVICE_ID pInst,
    void * pArg
    )
    {
    vxArmGicIpiCtrlInit.pCpus = sysCpuAvailableGet();
    vxArmGicIpiCtrlInit.pCtlr = pInst;

    return(&vxArmGicIpiCtrlInit);
    }

/*******************************************************************************
*
* vxbArmGicIpiGen - generate Inter Processor Interrupt
*
* This routine generates Inter Processor Interrupt(IPI) through
* Software Generated Interrupt(SGI)
*
* RETURNS: OK or ERROR if parameter is invalid.
*
* ERRNO: N/A
*/

LOCAL STATUS vxbArmGicIpiGen
    (
    VXB_DEVICE_ID pCtlr,
    INT32 ipiId,
    cpuset_t cpus
    )
    {
    if (ipiId < 0 || ipiId >= ARM_GIC_IPI_COUNT || cpus == 0)
        return (ERROR);

    *GIC_SWInterrupt = SWAP32(cpus << 16 | ipiId);

    return(OK);
    }

/*******************************************************************************
*
* vxbArmGicIpiConnect - connect ISR to IPI
*
* This routine connects ISR to Inter Processor Interrupt(IPI)
*
* RETURNS: return value of intConnect()
*
* ERRNO: N/A
*/

LOCAL STATUS vxbArmGicIpiConnect
    (
    VXB_DEVICE_ID pCtlr,
    INT32 ipiId,
    IPI_HANDLER_FUNC ipiHandler,
    void * ipiArg
    )
    {
    INT32 level;

    level = ipiId + armGicLinesNum;

    return (intCtlrISRAdd(&pVxbArmGicDrvCtrl->isrHandle,
                          level, ipiHandler, ipiArg));
    }

/*******************************************************************************
*
* vxbArmGicIpiDisconnect - disconnect ISR from IPI
*
* This routine disconnects ISR from Inter Processor Interrupt(IPI)
*
* RETURNS: OK if operation succeed else ERROR
*
* ERRNO:
*/

LOCAL STATUS vxbArmGicIpiDisconnect
    (
    VXB_DEVICE_ID pCtlr,
    INT32 ipiId,
    IPI_HANDLER_FUNC ipiHandler,
    void * ipiArg
    )
    {
    INT32 level;

    level = ipiId + armGicLinesNum;

    return (intCtlrISRRemove(&pVxbArmGicDrvCtrl->isrHandle,
                             level, ipiHandler, ipiArg));

    }

/*******************************************************************************
*
* vxbArmGicIpiEnable - enable specified IPI
*
* This routine enables specified IPI
*
* RETURNS: return value of intEnable()
*
* ERRNO: N/A
*/

LOCAL STATUS vxbArmGicIpiEnable
    (
    VXB_DEVICE_ID pCtlr,
    INT32 ipiId
    )
    {
    return (vxbArmGicHwEnable(ipiId));
    }

/*******************************************************************************
*
* vxbArmGicIpiDisable - disable specified IPI
*
* This routine disables specified IPI
*
* RETURNS: return value of intDisable()
*
* ERRNO: N/A
*/

LOCAL STATUS vxbArmGicIpiDisable
    (
    VXB_DEVICE_ID pCtlr,
    INT32 ipiId
    )
    {
    return (vxbArmGicHwDisable(ipiId));
    }

/*******************************************************************************
*
* vxbArmGicIpiPrioGet - retrieve IPI priority
*
* This routine gets the current priority of specified IPI.
*
* NOTE
* priority configuration is currently unsupported, and we always use 0 as the
* priority of all IPIs.
*
* RETURNS: always 0
*
* ERRNO: N/A
*/

LOCAL INT32 vxbArmGicIpiPrioGet
    (
    VXB_DEVICE_ID pCtlr,
    INT32 ipiId
    )
    {
    return (0);
    }

/*******************************************************************************
*
* vxbArmGicIpiPrioSet - set IPI priority
*
* This routine sets the priority of specified IPI.
*
* NOTE
* priority configuration is currently unsupported, and we always use 0 as the
* priority of all IPIs.
*
* RETURNS: always OK
*
* ERRNO: N/A
*/

LOCAL STATUS vxbArmGicIpiPrioSet
    (
    VXB_DEVICE_ID pCtlr,
    INT32 ipiId,
    INT32 prio
    )
    {
    return (OK);
    }
#endif /* _WRS_CONFIG_SMP */

