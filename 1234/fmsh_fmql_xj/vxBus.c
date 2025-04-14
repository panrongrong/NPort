/* vxBus.c - vxBus subsystem source file */

/*
 * Copyright (c) 2005-2013 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
05b,18sep13,e_d  add SD bus support.
05a,05sep13,xms  fix NULL_RETURNS error. (WIND00414265)
04z,18apr13,y_y  Fix build error when enable small footprint profile.
                 (WIND00413207, WIND00390400)
04y,15mar13,j_z  add vxbIntAlloc/vxbIntFree to support MSI/MSI-X.(WIND00333514)
04x,05mar13,wyt  Fix coverity error. (WIND00401412)
04w,12nov12,w_x  remove vxbBusRemovalAnnounce() (WIND00387497)
04v,14sep12,l_z  Zero the pName when remove device. (WIND00191819)
04u,13sep12,l_z  Release lock when vxbRegUnmapAll failed. (WIND00339448)
04t,14aug12,l_z  Return NULL when pIntrInfo is NULL in vxbIntVectorGet. 
                 (WIND00366488)
04s,04aug12,w_x  add VXB_BUSID_USB_HOST (WIND00372413)
04r,19jul12,w_x  add vxbBusRemovalAnnounce() API (WIND00338041)
04q,25jul12,sye  Fixed small footprint for hwMemFree(). (WIND00364456)
04p,13apr12,sye  Remove zero padding for debug message. (WIND00339224)
04o,03feb12,l_z  Add I2C bus support
04n,02sep11,sye  Fix vxbDevIterate() access to removed list nodes.(WIND00272826)
04m,11aug11,rbc  WIND00238999 - Fix more build errors
04l,10aug11,rbc  WIND00238999 - Fix build error for vxbPwrDeviceAnnounceFunc
04k,28jul11,rbc  WIND00238999 - Add VSB _WRS_CONFIG_PWR_MGMT
04j,01jun11,rec  WIND00255358 add device power management
04i,18feb11,bsk  fix sp errors and inconsistent comments for doc gen
04h,08sep10,ghs  Add USB MHCD support.
04g,24aug10,y_c  fixed compile error in fuc vxbDevFromBusRemove.(WIND00229625)
04f,09feb10,pch  small footprint
04e,29apr10,h_k  cleaned up a compiler warning in SMP LP64.
04d,04feb10,d_c  Update API references to clarify CPU index type. See uHLD 1053.
04c,09dec09,h_k  added unmap the BARs for device disconnection.
04b,13oct09,yjw  Fix vxbDevFromBusRemove(), vxbDevConnectHelper(), and
		 vxbDevInitRun() miss mutual exclusion. (WIND00182148)
04a,08sep09,rgo  hwMemPool can increase in vxbDevStructAlloc, only when
04b,17jun10,h_k  reverted the pre-kernel initialization support in LP64.
04a,07jun10,h_k  added pre-kernel initialization support in LP64.
                 return ERROR when pPlbBus is not initialized in vxbDevIterate.
                 HWMEM_ALLOC_FAIL_DEBUG defined.(WIND145720)
03z,23jul09,my_  fixed incompatibility issue between vxbDeviceAnnounce() and
		 vxbDevFromBusRemove() (WIND00172204)
03y,23jul09,my_  modified vxbDevRemovalAnnounce() to check return value,
                 corrected return value of vxbDevFromBusRemove() (WIND00172190)
03x,r2jul09,fao  corrected using of vxbLockTake in vxbBusAnnounce.(WIND00170674)
03w,12jun09,x_s  added support for small footprint.
03v,24feb09,h_k  added missing vxbLockDelete() for bus information.
03u,19feb09,h_k  added vxbBusPresentVerify().
03t,04feb09,h_k  updated vxbDeviceDriverRelease() and vxbDrvDisconnect()
                 to remove subordinate bus. (CQ:152849)
03s,28may09,h_k  enabled RW semaphore protections.
03r,05may09,cww  Updated atomic operators
03q,24apr09,h_k  fixed print format.
03p,23feb09,h_k  updated for LP64 support.
                 disabled semRW temporary for LP64.
03o,26nov08,h_k  fixed memory leak for RW semaphore in vxbDevStructFree().
                 (CQ:145722)
                 removed unused pBusDevExpiredList.
03n,23oct08,rgo  WIND00088084 Documentation for vxbDevConnectHelper is
                 incorrect.
03m,24sep08,h_k  fixed VxBus tests failures. (CQ:132074)
                 cleaned up compiler warnings in GNU.
03l,18sep08,wap  Correct handling of device removal (WIND00134137)
03k,03sep08,jpb  Renamed VSB header file
03j,21aug08,sru  allow BSP to exclude devices from VxBus at runtime
03i,30jun08,tor  library build
03h,18jun08,jpb  Renamed _WRS_VX_SMP to _WRS_CONFIG_SMP.  Added include path
                 for kernel configurations options set in vsb.
03g,17jun08,h_k  removed pAccess.
                 removed redundant zero clear after hwMemAlloc().
03f,08may08,pmr  allow fall-through to vxbDevControl method in vxbIntEnable
03e,01may08,tor  update version
03d,21apr08,tor  optimization
03c,23apr08,h_k  reverted satnity test for pDev in vxbDevMethodGet().
03b,07dec07,tor  optimization
03a,25sep07,tor  VxBus version checks
02z,20sep07,h_k  removed error messages.
		 cleaned up compiler warnings in GNU.
02y,30aug07,tor  WIND00101663 VMR routines
02x,01may07,dlk  Simplify vxbInstInsert() and vxbInstRemove().
                 Support for SPIN_LOCK_TRACE.
02w,25jul07,tor  vxbLock self-init and uses semRW
02v,24jul07,jrp  Moved vxbDevConnect and vxbDevInit to vxbUsrCmdLine.c
02u,15jul07,pdg  added string for VXB_BUSID_MF bus type
02t,09jul07,dtr  Export interrupt controller mgt struct pkus add some comments.
02s,04jun07,tor  add VxBus locks
02r,11may07,tor  temp remove lock on vxbDevIterate
02q,10may07,tor  remove undef LOCAL
02p,04may07,wap  Convert to new register access API
02o,29apr07,cjj  modified vxbDeviceDriverRelease() to not use
		 VXB_METHOD_DRIVER_UNLINK.  SMP sandbox merge.
02n,27apr07,slk  add interrupt and cpu route functions
02m,24apr07,tor  check for null function pointers
02l,17apr07,sup  USB Bus type names added for display in vxBusShow
02k,03apr07,pdg  critical section locks added
02j,29mar07,tor  clean up unlink method
02i,15mar07,ebh  manually merge in FRZ4 version with changes from 12feb07
02h,15mar07,tor  Modified interrupt support for IntrCtlr drivers
02g,05mar07,dtr  Add extra bus method.
02f,23feb07,pdg  added new method definition for timer drivers
02e,12feb07,bwa  added vxbDeviceDriverRelease() and vxbDrvRescan()
02d,18jan07,d_c  Corrected VXB_DEBUG_MSG vxbNewDriverInit2
                 Remove explicit #undef to VXBUS_DEBUG_ON
02c,09nov06,tor  add interrupt controller support
02b,08jan07,sup  added new method definition for timer drivers
02a,14dec06,wap  Correct vxbInstRemove logic
01z,30nov06,wap  Add list handling convenience routines, insure insertions
                 into lists are tail-first instead of head-first
01y,17oct06,pdg  replaced VXB_METHOD_DRIVER_UNLINK with an address and
                 updated method string definitions.
01x,19sep06,pdg  defined strings for busConfiguration access methods
01w,26jul06,pdg  corrected vxbResourceSearch ()
01v,22jul06,wap  Avoid initializing devices more than once
01u,02jul06,bwa  added virtual bus type.
01t,29jun06,wap  add method support
01s,27Jun06,tor  add PLB to vxbDevIterate()
01r,22jun06,wap  Silence compiler warning
01q,20jun06,wap  Add vxbNextUnitGet()
01p,15mar06,pdg  Fix for SPR #118459 (VxBus bus controllers and dynamically
                 loaded drivers)
01o,07feb06,pdg  Fix for SPR #117282 (handling devices and buses without a
                 parent)
01n,19jan06,pdg  added parameter system support
01m,02jan06,wap  Add MII_Bus type, fix SPR #115878
01l,05dec05,pdg  Updated vxbSubDevAction () for having the function called for
		 orphans also
01k,28nov05,pdg  Fix for SPR #112638(Unit number API added)
01j,17nov05,pdg  Fix for the error in vxbDriverUnregister ()(i.e If there are
		 already registered drivers in the list and if
		 vxbDriverUnregister () is called without calling
		 vxbDevRegister (), it returns OK)
01i,14nov05,mdo  Add routine - vxbSubDevAction
01h,29sep05,mdo  Fix gnu warnings
01g,20sep05,pdg  Fixed vxbus errors identified during testing(SPR #112193)
01f,29aug05,pdg  Enhancement functions added
01e,19aug05,mdo  Fix return value in busDeviceMethodRun
01d,18aug05,mdo  remove freeBsd_compatibility code
01c,10aug05,mdo  Phase in new access method
01b,18jul05,mdo  Fix per coding standards
		 Fix compiler warnings
01a,17jan05,tor  written
*/

/*
DESCRIPTION
This library contains the support routines for the
vxBus subsystem.

INCLUDE FILES: vxBus.h vxbPlbLib.h
*/

/* includes */

#include <vxWorks.h>
#include <vsbConfig.h>
#include <ctype.h>
#include <vxAtomicLib.h>
#include <hwif/vxbus/vxBus.h>
#include <hwif/util/hwMemLib.h>
#include <hwif/vxbus/vxbPlbLib.h>
#include "../h/vxbus/vxbAccess.h"

#include <private/spinLockLibP.h>
#ifdef _VXBUS_BASIC_RWSEM
#include <intLib.h>
#endif /* _VXBUS_BASIC_RWSEM */
#include <private/memPartLibP.h>	/* for KMEM_ALLOC */

METHOD_DECL(sysBspDevFilter);
METHOD_DECL(vxbDrvUnlink);
METHOD_DECL(vxbIntCtlrConnect);
METHOD_DECL(vxbIntCtlrCpuReroute);
METHOD_DECL(vxbIntCtlrDisconnect);
METHOD_DECL(vxbIntCtlrAck);
METHOD_DECL(vxbIntCtlrEnable);
METHOD_DECL(vxbIntCtlrDisable);
METHOD_DECL(vxbIntCtlrDisconnect);
METHOD_DECL(vxbIntCtlrIntReroute);
METHOD_DECL(vxbIntCtlrAlloc);
METHOD_DECL(vxbIntCtlrFree);

#include <logLib.h>

#include <stdio.h>
#include <string.h>
#define VXBUS_DEBUG_ON


#if (defined _WRS_CONFIG_PWR_MGMT) && (defined _WRS_ARCH_HAS_DEV_PWR_MGMT)
#include <hwif/pwr/pwrDeviceLib.h>
#endif /* (_WRS_CONFIG_PWR_MGMT) && (_WRS_ARCH_HAS_DEV_PWR_MGMT) */

#ifdef VXBUS_DEBUG_ON

int usrBusDebugLevel = 100;

#undef VXB_DEBUG_MSG
#define VXB_DEBUG_MSG(lvl,msg,a,b,c,d,e,f) if ( usrBusDebugLevel >= lvl ) printf(msg,a,b,c,d,e,f)

#else /* VXBUS_DEBUG_ON */

#undef VXB_DEBUG_MSG
#define VXB_DEBUG_MSG(lvl,msg,a,b,c,d,e,f)

#endif /* VXBUS_DEBUG_ON */

/* defines */

#define IA32_LOCK_REMOVAL

#define VXB_ENDIAN_FLAG(pDev)   0
#define VXB_ENDIAN_MASK     0x0f

/* typedefs */

/* structure used by vxbIntReroute() to pass info to vxbISRCpuReroute() */
struct vxbIntrRerouteInfo
    {
    VXB_DEVICE_ID       pInst;
    int                 interruptIndex;
    cpuset_t            destCpu;
    };

struct vxbMethodRunInfo
    {
    VXB_METHOD_ID	method;
    void *      pArg;
    };

#ifndef	_WRS_CONFIG_VXBUS_BASIC
/* structure used in vxbDriverUnregister to retrieve the status */

struct vxbDrvDisconnectData
    {
    struct vxbDevRegInfo *  pDriver;
    STATUS          status;
    };
#endif	/* _WRS_CONFIG_VXBUS_BASIC */

/* Structure used by vxbNextUnitGet() */

typedef struct vxb_unit
    {
    UINT32 vxbCheckUnit;
    BOOL vxbUnitTaken;
    VXB_DEVICE_ID vxbDev;
    } VXB_UNIT;

/* PLB device control method function pointer type */

typedef STATUS (*PLB_DEV_CTRL_FUNCPTR)
    (
    VXB_DEVICE_ID       pDev,
    pVXB_DEVCTL_HDR     pBusDevControl
    );

/* externs */

IMPORT STATUS vxbDrvVerCheck
    (
    struct vxbDevRegInfo * pDevInfo /* per-bus recognition info */
    );

/* Hook for user post vxbDevInit routine */
extern VOIDFUNCPTR _func_vxbUserHookDevInit;

/* device drivers which have registered */
struct vxbDevRegInfo * pDriverListHead = NULL;

/* busTypes which have registered */
struct vxbBusTypeInfo * pBusListHead = NULL;

/* lock to protect updations of the drivers and bus types list */
VXB_LOCK_DECL(vxbGlobalListsLock);

/* head of list of actual busses present on the system */
struct vxbBusPresent * pBusHead = NULL;

/* lock to protect updations of the buses list */
VXB_LOCK_DECL(vxbBusListLock);

#ifdef VXB_PERFORM_SANITY_CHECKS
/* head of list of devices which do not reside on any bus */
struct vxbDev	*	pLostDevHead = NULL;

/* head of the list of buses which do not have a parent */
struct vxbBusPresent *	pLostBusHead = NULL;

/* lock to protect updations of the lost devices and buses list */
VXB_LOCK_DECL(vxbLostDevListLock);
#endif /* VXB_PERFORM_SANITY_CHECKS */

struct vxbBusPresent * pPlbBus = NULL;
struct vxbDev * pPlbDev = NULL;

/* lock to protect updations of the free and expired devices list */

VXB_LOCK_DECL(vxbDevStructListLock);

/* locals */

LOCAL int vxbInitPhase = -1;

LOCAL struct vxbDev * pBusDevFreeList = NULL;

/* forward declarations */

LOCAL void vxbInstInsert(VXB_DEVICE_ID * pHead, VXB_DEVICE_ID pDev);

#ifndef	_WRS_CONFIG_VXBUS_BASIC
LOCAL STATUS vxbInstRemove(VXB_DEVICE_ID * pHead, VXB_DEVICE_ID pDev);
LOCAL STATUS vxbSubordinateBusRemove (VXB_DEVICE_ID pDev);
#endif	/* _WRS_CONFIG_VXBUS_BASIC */

#ifdef SPIN_LOCK_TRACE
LOCAL void vxbSpinLockTakeStub(spinlockIsr_t * lock, char * file, int line);
LOCAL void vxbSpinLockStub(spinlockIsr_t * lock);
SPIN_LOCK_ISR_TAKE_FUNC pVxbSpinLockTake = vxbSpinLockTakeStub;
#else	/* SPIN_LOCK_TRACE */
LOCAL void vxbSpinLockStub(spinlockIsr_t * lock);
SPIN_LOCK_ISR_TAKE_FUNC pVxbSpinLockTake = vxbSpinLockStub;
#endif /* SPIN_LOCK_TRACE */

void (*pVxbSpinLockGive)(spinlockIsr_t * lock) = vxbSpinLockStub;

/*********************************************************************
*
* vxbLibInit - initialize vxBus library
*
* This routine initializes the vxBus library.
*
* RETURNS: OK, always
*
* ERRNO: N/A
*/

STATUS vxbLibInit (void)
    {
    vxbInitPhase = 0;
    return(OK);
    }

/*********************************************************************
*
* vxbInit - initialize vxBus
*
* This routine initializes the vxBus subsystem.
*
* RETURNS: OK, always
*
* ERRNO: N/A
*/

STATUS vxbInit (void)
    {
    vxbInitPhase = 1;

    plbInit1(pPlbDev);
    return(OK);
    }

/*********************************************************************
*
* vxbInstInsert - add an instance to an instance list
*
* This routine adds a new instance to a device list. This is a
* helper routine used to insure that the instance is always added
* to the tail of the list instead of the head. Head-first insertions
* result in confusing instance enumeration.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxbInstInsert
    (
    VXB_DEVICE_ID * pHead,
    VXB_DEVICE_ID pDev
    )
    {
    while (*pHead != NULL)
	pHead = &(*pHead)->pNext;

    *pHead = pDev;

    pDev->pNext = NULL;

    return;
    }

#ifndef	_WRS_CONFIG_VXBUS_BASIC
/*********************************************************************
*
* vxbInstRemove - remove an instance from an instance list
*
* This is a helper routine that removes an instance from a list.
*
* RETURNS: OK if instance was successfully removed, ERROR if
* instance was not found.
*
* ERRNO: N/A
*/

LOCAL STATUS vxbInstRemove
    (
    VXB_DEVICE_ID * pHead,
    VXB_DEVICE_ID pDev
    )
    {
    while (*pHead != NULL)
	{
	if (*pHead == pDev)
	    {
	    *pHead = pDev->pNext;
	    pDev->pNext = NULL;
	    return OK;
	    }
	pHead = &(*pHead)->pNext;
	}

    return ERROR;
    }
#endif	/* _WRS_CONFIG_VXBUS_BASIC */

/*********************************************************************
*
* vxbDevInit2Helper - second-pass initialization of devices
*
* This routine is the second stage of the vxBus subsystem
* initialization.
*
* RETURNS: OK, always
*
* ERRNO: N/A
*/

LOCAL STATUS vxbDevInit2Helper
    (
    struct vxbDev * pInst,
    void * pArg
    )
    {
    if ( pInst->pDriver == NULL )
        return(OK);
    if ( pInst->pDriver->pDrvBusFuncs == NULL )
        return(ERROR);
    if (pInst->flags & VXB_INST_INIT2_DONE)
        return (ERROR);

    if ( pInst->pDriver->pDrvBusFuncs->devInstanceInit2 == NULL )
        return(OK);

    (*pInst->pDriver->pDrvBusFuncs->devInstanceInit2)(pInst);

    pInst->flags |= VXB_INST_INIT2_DONE;

    return(OK);
    }

/*********************************************************************
*
* vxbDevConnectHelper - third-pass initialization of devices
*
* This routine executes the third stage of device initialization.
*
* RETURNS: OK, or ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbDevConnectHelper
    (
    struct vxbDev * pInst,  /* device information */
    void * pArg
    )
    {
    STATUS status = OK;

    /* take device lock */
    vxbLockTake (&pInst->devLock, VXB_LOCK_WRITER);

    if (pInst->pDriver == NULL)
        goto vxbDevConnectHelperExit;

    if (pInst->pDriver->pDrvBusFuncs == NULL)
        {
        status = ERROR;
        goto vxbDevConnectHelperExit;
        }

    if (pInst->flags & VXB_INST_CONNECT_DONE)
        {
        status = ERROR;
        goto vxbDevConnectHelperExit;
        }

    if (pInst->pDriver->pDrvBusFuncs->devInstanceConnect == NULL)
        goto vxbDevConnectHelperExit;

    (*pInst->pDriver->pDrvBusFuncs->devInstanceConnect)(pInst);

    pInst->flags |= VXB_INST_CONNECT_DONE;

vxbDevConnectHelperExit:

    /* release device lock */
    vxbLockGive (&pInst->devLock, VXB_LOCK_WRITER);

    return (status);
    }


/*********************************************************************
*
* vxbDevInitInternal - second-pass initialization of devices
*
* This routine executes the second stage of device initialization.
* Note:  This is called from the vxbUsrCmdLine.c in command line builds
*
* RETURNS: OK, always
*
* ERRNO: N/A
*/

STATUS vxbDevInitInternal (void)
    {
    vxbInitPhase = 2;

    /*
     * spinlocks can be used starting from the 2nd stage of
     * OS initialization. Update the function pointers with
     * the actual spinlock functions
     */

    pVxbSpinLockTake = spinLockIsrTake;
    pVxbSpinLockGive = spinLockIsrGive;

    /*
     * Starting with init phase 2, VxBus locks can be initialized
     * for normal operation.
     */

    vxbLockInit(&vxbGlobalListsLock);
    vxbLockInit(&vxbBusListLock);
#ifdef VXB_PERFORM_SANITY_CHECKS
    vxbLockInit(&vxbLostDevListLock);
#endif /* VXB_PERFORM_SANITY_CHECKS */
    vxbLockInit(&vxbDevStructListLock);

    /* execute init phase 2 for all devices */

    vxbDevIterate(vxbDevInit2Helper, NULL, VXB_ITERATE_INSTANCES);

    return(OK);
    }

/*********************************************************************
*
* vxbDevConnectInternal - third-pass initialization of devices
* Note:  This is called from the vxbUsrCmdLine.c in command line builds
*
* This routine executes the third stage of device initialization.
*
* RETURNS: OK, always
*
* ERRNO: N/A
*/

STATUS vxbDevConnectInternal (void)
    {
    vxbInitPhase = 3;
    vxbDevIterate(vxbDevConnectHelper, NULL, VXB_ITERATE_INSTANCES);

    if (_func_vxbUserHookDevInit != NULL)
	(*_func_vxbUserHookDevInit)();

    return(OK);
    }

/*********************************************************************
*
* vxbLibError - handle error conditions
*
* This is a generic routine to handle error conditions.
*
* RETURNS: ERROR, always
*
* ERRNO: N/A
*/

STATUS vxbLibError
    (
    FUNCPTR pAddr,  /* calling routine */
    char *  pMsg    /* error message */
    )
    {
    if ( _func_logMsg != NULL )
        _func_logMsg ("vxbLibError: %s @ %p\n",
                      (_Vx_usr_arg_t)pMsg, (_Vx_usr_arg_t)pAddr, 3L, 4L, 5L, 6L);
    return(ERROR);
    }

/*********************************************************************
*
* vxbDevInitRun - device-specific initialization
*
* This routine calls the appropriate driver-specified
* initialization routines, depending on what initialization
* phase has been completed in the system.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxbDevInitRun
    (
    VXB_DEVICE_ID devID,
    struct vxbDevRegInfo * pDrv
    )
    {
    /* take device lock */
    vxbLockTake (&devID->devLock, VXB_LOCK_WRITER);

    /* first pass */

    if (!(devID->flags & VXB_INST_INIT_DONE))
        {
        if ( pDrv->pDrvBusFuncs->devInstanceInit != NULL )
	    (*(pDrv->pDrvBusFuncs->devInstanceInit))(devID);
        devID->flags |= VXB_INST_INIT_DONE;
        }

    /* second pass */

    if (vxbInitPhase >= 2 && !(devID->flags & VXB_INST_INIT2_DONE))
        {
        if ( pDrv->pDrvBusFuncs->devInstanceInit2 != NULL )
	    (*(pDrv->pDrvBusFuncs->devInstanceInit2))(devID);
        devID->flags |= VXB_INST_INIT2_DONE;
        }

    /* third pass */

    if (vxbInitPhase >= 3 && !(devID->flags & VXB_INST_CONNECT_DONE))
        {
        if ( pDrv->pDrvBusFuncs->devInstanceConnect != NULL )
	    (*(pDrv->pDrvBusFuncs->devInstanceConnect))(devID);
        devID->flags |= VXB_INST_CONNECT_DONE;

#if (defined _WRS_CONFIG_PWR_MGMT) && (defined _WRS_ARCH_HAS_DEV_PWR_MGMT)
        /* if device power management is supported */
        
        if (vxbPwrDeviceAnnounceFunc != NULL)
            {
            FUNCPTR pMethod = NULL;

            /*
             * Get the power management method.
             */

            pMethod = vxbDevMethodGet (devID,
                                       DEVMETHOD_CALL(pwrStateSet));

            if (pMethod != NULL)  /* If the device supports power management */
                {
                
                /* register the device to the device power manager */

                devID->pwrDeviceId = vxbPwrDeviceAnnounceFunc (devID, NULL);
                }
            }
#endif /* (_WRS_CONFIG_PWR_MGMT) && (_WRS_ARCH_HAS_DEV_PWR_MGMT) */
        }

    /* release device lock */
    vxbLockGive (&devID->devLock, VXB_LOCK_WRITER);
    }

/*********************************************************************
*
* vxbNewDriver - register a device driver with the vxBus subsystem
*
* This routine checks a known orphan device to see if it
* can be claimed by the new driver.
*
* This duplicates some of the functionality of vxbDeviceAnnounce().
*
* RETURNS: OK, or ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbNewDriver
    (
    struct vxbDev * pDev,    /* Device Information */
    void * pArg
    )
    {
    struct vxbDevRegInfo *  pDriver = (struct vxbDevRegInfo *)pArg;
    struct vxbBusTypeInfo *    pBusEntry;
    struct vxbBusPresent *     pBus;
    BOOL            drvFound;
    struct vxbDev *     pOrphanList;
    struct vxbDev *     pOrphanNext;


    VXB_DEBUG_MSG(10, "vxbNewDriver(%p,...)\n", pDev, 2, 3, 4, 5, 6);

    /* check busID */

    if ( pDriver->busID != pDev->busID )
        return(ERROR);

    if ( pDev->pParentBus == NULL )
        return(ERROR);

    /* Should never happen */

    VXB_ASSERT((pDev->flags & VXB_INST_INIT_DONE) == 0, ERROR)

    VXB_DEBUG_MSG(12, "    vxbNewDriver(%p,...) found busID match\n",
                  pDev, 2, 3, 4, 5, 6);

    /* get bus type of current device */

    pBusEntry = pDev->pParentBus->pBusType;

    /* check bus-specific match routine */

    drvFound = (*(pBusEntry->vxbDevMatch))(pDriver, pDev);
    if ( drvFound == FALSE )
        return(ERROR);

    VXB_DEBUG_MSG(12, "    vxbNewDriver(%p,...) found bus match\n",
                  pDev, 2, 3, 4, 5, 6);

    /* check driver-supplied probe routine */

    if ( pDriver->devProbe == NULL )
        {
        drvFound = TRUE;
        }
    else
        {
        drvFound = (*(pDriver->devProbe))(pDev);
        if ( drvFound == FALSE )
            return(ERROR);
        }

    VXB_DEBUG_MSG(10, "vxbNewDriver(%p) orphan found\n",
                  pDev, 2, 3, 4, 5, 6);

    /* attach driver */

    pDev->pDriver = pDriver;

    /*
     * Adjust name, if pDev->pName isn't NULL, the name was assigned when the
     * device was created by bus controller driver, just keep it. If the name
     * is not assigned when the deivce was created, set the name to driver name.
     */

    if (pDev->pName == NULL)
    pDev->pName = &pDriver->drvName[0];

    /* get parent bus */

    pBus = (struct vxbBusPresent *)pDev->pParentBus;

    /* acquire the lock */
    vxbLockTake(&pBus->listLock, VXB_LOCK_WRITER);

    /* remove this instance from orphan list */

    pOrphanList = pBus->devList;
    if ( pOrphanList == pDev )
        {
        pBus->devList = pDev->pNext;
        pDev->pNext = NULL;
        }
    else
        {
        while ( pOrphanList->pNext != NULL )
            {
            if ( pOrphanList->pNext == pDev )
                {

                /* update links to remove from list */

                pOrphanNext = pDev->pNext;
                pDev->pNext = NULL;
                pOrphanList->pNext = pOrphanNext;

		break;
                }
            else
                {
                pOrphanList = pOrphanList->pNext;
                }
            }
        }

    /* save as instance */

    vxbInstInsert (&pBus->instList, pDev);

    /* release the lock */
    vxbLockGive(&pBus->listLock, VXB_LOCK_WRITER);

    /* perform initialization */

    vxbDevInitRun(pDev, pDriver);

    return(OK);
    }

/*********************************************************************
*
* vxbDevRegister - register a device driver
*
* This routine registers a device driver with the vxBus subsystem.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS vxbDevRegister
    (
    struct vxbDevRegInfo * pDevInfo /* per-bus recognition info */
    )
    {
    struct vxbDevRegInfo * pListEnd;
    struct vxbDevRegInfo * pCurrent;

    VXB_ASSERT(pDevInfo!=NULL, ERROR)

    if ( vxbDrvVerCheck(pDevInfo) != OK )
        return(ERROR);

    /*
     * Walk the list of new registration functions
     * to find the last entry.  This allows drivers
     * to register multiple entries with a single
     * call.
     */

    pListEnd = pDevInfo;
    while ( pListEnd->pNext != NULL )
        pListEnd = pListEnd->pNext;

    /* add new list to driver head */


    VXB_DEBUG_MSG(2, "vxbDevRegister() registering new driver @ %p : %s \n",
                  pDevInfo, &pDevInfo->drvName[0], 3, 4, 5, 6);

    /* acquire the lock */

    vxbLockTake(&vxbGlobalListsLock, VXB_LOCK_WRITER);

    pListEnd->pNext = pDriverListHead;
    pDriverListHead = pDevInfo;

    /* release the lock */

    vxbLockGive(&vxbGlobalListsLock, VXB_LOCK_WRITER);

    /* advance pListEnd to point to first driver *AFTER* the current one */

    pListEnd = pListEnd->pNext;

    if ( vxbInitPhase < 1 )
        return OK;

    VXB_DEBUG_MSG(2, "vxbDevRegister(): pDevInfo @ %p, pListEnd @ %p\n",
                  pDevInfo, pListEnd, 3, 4, 5, 6);

    /* Check existing devices, to see if this driver matches */

    pCurrent = pDevInfo;
    while ( pCurrent != pListEnd )
        {
        VXB_DEBUG_MSG(2, "vxbDevRegister() checking for orphans on %s\n",
                      vxbBusTypeString(pCurrent->busID), 2, 3, 4, 5, 6);

        vxbDevIterate(vxbNewDriver, pCurrent, VXB_ITERATE_ORPHANS);
        pCurrent = pCurrent->pNext;
        }

    return OK;

    }

/******************************************************************
*
* vxbDrvRescan - rescan all orphans to match against driver
*
* This routine re-scans all orphan device, checking to see whether
* they match the specified driver.
*
* RETURNS: OK, or ERROR
*
* ERRNO: N/A
*/

BOOL vxbDrvRescan
    (
    struct vxbDevRegInfo * pDriver
    )
    {
    vxbDevIterate(vxbNewDriver, pDriver, VXB_ITERATE_ORPHANS);
    return TRUE;
    }

#ifndef	_WRS_CONFIG_VXBUS_BASIC
/*********************************************************************
*
* vxbDrvDisconnect - dissociate a driver from a device
*
* This routine dissociates the specified driver from an instance.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbDrvDisconnect
    (
    struct vxbDev * pDev,    /* Device Information */
    void * pArg
    )
    {
    struct vxbDrvDisconnectData *   pDrvDisconnect =
                                    (struct vxbDrvDisconnectData *)pArg;

    VXB_ASSERT(pDrvDisconnect!=NULL, ERROR)

    /* check if this is an instance of the specified driver */

    if ( pDev->pDriver != pDrvDisconnect->pDriver)
        {
        return(OK);
        }

    /* turn the instance into an orphan */

    if (vxbDeviceDriverRelease (pDev) != OK)
	{
	pDrvDisconnect->status = ERROR;
	return (ERROR);
	}

    return(OK);
    }

/*********************************************************************
*
* vxbDeviceDriverRelease - turn an instance into an orphan
*
* This routine dissociates the specified device from its driver and
* removes a subordinate bus if the device is a bus controller device.
*
* RETURNS: OK, or ERROR if the device fails to unlink or remove the
* subordinate bus when it is a bus controller device.
*
* ERRNO: N/A
*/

STATUS vxbDeviceDriverRelease
    (
    struct vxbDev *pDev
    )
    {
    struct vxbBusPresent * pBus;
    FUNCPTR pMethod;
    STATUS status = OK;

    VXB_ASSERT(pDev->pDriver!=NULL, ERROR)

    /* retrieve the method for unlink id */

    pMethod = vxbDevMethodGet(pDev, DEVMETHOD_CALL(vxbDrvUnlink));

    /* if there is no such method, it is an ERROR */

    if (pMethod != NULL)
        {
        /* execute the method */

        if (((*pMethod)(pDev, NULL)) != OK)
            {
            return ERROR;
            }

        /* deallocate the memory allocated for pInst->pParams */

        if (pDev->pParams != NULL)
            {
#ifndef _VXBUS_BASIC_HWMEMLIB                
            hwMemFree((char *)(pDev->pParams));
#endif /* !_VXBUS_BASIC_HWMEMLIB */            
            pDev->pParams = NULL;
            }
        }
    else
        {
        return ERROR;
        }

    /* if bus controller with subordinate devices, remove those */

    if (vxbSubordinateBusRemove (pDev) != OK)
	return (ERROR);

#ifdef	_WRS_CONFIG_LP64
    /* unmap the BARs */

    if (vxbRegUnmapAll (pDev) != OK)
	return (ERROR);
#endif	/* _WRS_CONFIG_LP64 */

    /* Device no longer initialized */

    pDev->flags &=
	~(VXB_INST_INIT_DONE | VXB_INST_INIT2_DONE | VXB_INST_CONNECT_DONE);

    /* get parent bus and instance list head */

    pBus = (struct vxbBusPresent *)pDev->pParentBus;

    /* acquire the lock */

    vxbLockTake (&pBus->listLock, VXB_LOCK_WRITER);

    /* remove from instance list */

    status = vxbInstRemove (&pBus->instList, pDev);

    /* add to head of orphan list */

    vxbInstInsert (&pBus->devList, pDev);

    /* release the lock */

    vxbLockGive(&pBus->listLock, VXB_LOCK_WRITER);

    /* zero out device pDrvCtrl and pDriver pointers */

    pDev->pDrvCtrl = NULL;

    /* if the pName point to drvName, zero it */

    if (pDev->pDriver != NULL &&
        pDev->pName == &pDev->pDriver->drvName[0])
        pDev->pName = NULL;

    pDev->pDriver = NULL;

    return status;
    }
#endif	/* _WRS_CONFIG_VXBUS_BASIC */


/******************************************************************************
*
* vxbBusPresentVerify - verify bus is present
*
* This routine verifies the bus is in the buses list.
*
* RETURNS: OK if the bus is found in the buses list, otherwise ERROR.
*/

LOCAL STATUS vxbBusPresentVerify
    (
    struct vxbBusPresent * pBus
    )
    {
    struct vxbBusPresent *	pHead;
    STATUS			stat = ERROR;

    /* acquire the lock */

    vxbLockTake (&vxbBusListLock, VXB_LOCK_READER);

    /* search the bus from the chains start from the pPlbBus */

    pHead = pPlbBus;

    while (pHead != NULL)
	{
	if (pHead == pBus)
	    {
	    /* bus found in the registered bus list */

	    stat = OK;
	    break;
	    }

	pHead = pHead->pNext;
	}

#ifdef	VXB_PERFORM_SANITY_CHECKS
    if (stat != OK)
	{
	/* search the bus from the chains start from the pLostBusHead */

	pHead = pLostBusHead;

	while (pHead != NULL)
	    {
	    if (pHead == pBus)
		{
		/* bus found in the lost bus list */

		stat = OK;
		break;
		}
	    }
	}
#endif	/* VXB_PERFORM_SANITY_CHECKS */

    /* release the lock */

    vxbLockGive (&vxbBusListLock, VXB_LOCK_READER);

    return (stat);
    }

#ifndef	_WRS_CONFIG_VXBUS_BASIC
/*******************************************************************************
*
* vxbSubordinateBusRemove - remove subordinate bus
*
* If the device instance is a bus controller with a subordinate bus, this
* routine removes the subordinate bus.
*
* RETURNS: OK if the subordinate bus and all the devices on the bus are removed
* successfully or the device instance is not with a subordinate bus, otherwise 
* ERROR.
*
*/

LOCAL STATUS vxbSubordinateBusRemove
    (
    VXB_DEVICE_ID pDev
    )
    {
    struct vxbBusPresent *	pBus;
    volatile VXB_DEVICE_ID	subordinateDev;
    STATUS			stat = OK;

    /* if bus controller with subordinate devices, remove those */

    pBus = pDev->u.pSubordinateBus;

    if (pBus != NULL)
	{
	struct vxbBusPresent * pParentBus;

	/*
	 * Verify the pDev->u.pSubordinateBus indicates the pSubordinateBus
	 * or pDevPrivate.
	 */

	if (vxbBusPresentVerify (pBus) != OK)
	    return (OK);

	/* release subordinate instances */

	subordinateDev = pBus->devList;

	/* if there are devices connected to this bus, remove the devices */

	while (subordinateDev != NULL)
	    {
	    if (vxbDevRemovalAnnounce (subordinateDev) != OK)
		return ERROR;

	    subordinateDev = pBus->devList;
	    }

	/* if there are instances connected to this bus, remove them */

	subordinateDev = pBus->instList;

	while (subordinateDev != NULL)
	    {
	    if (vxbDevRemovalAnnounce (subordinateDev) != OK)
		return ERROR;

	    subordinateDev = pBus->instList;
	    }

	/* Remove the bus from the list of busses */

	pParentBus = pBus->pCtlr->pParentBus;

	if (pParentBus != NULL)	/* should always be true */
	    {
	    void * pHead;

	    /* acquire the lock */

	    vxbLockTake (&vxbBusListLock, VXB_LOCK_WRITER);

	    /* remove from list */

	   pHead = (void *) &pParentBus->pNext;

	   stat = vxbInstRemove ((VXB_DEVICE_ID *)pHead, (VXB_DEVICE_ID)pBus);

	   /* release the lock */

	   vxbLockGive (&vxbBusListLock, VXB_LOCK_WRITER);
	   }
	else
	   {
#ifdef	VXB_PERFORM_SANITY_CHECKS
	    /* acquire the lock */

	     vxbLockTake (&vxbBusListLock, VXB_LOCK_WRITER);

	    if (pLostBusHead != NULL)
		{
		void * pHead = (void *) &pLostBusHead;

		/* remove from list */

		(void)vxbInstRemove ((VXB_DEVICE_ID *)pHead, (VXB_DEVICE_ID)pBus);
		}

	    /* release the lock */

	    vxbLockGive (&vxbBusListLock, VXB_LOCK_WRITER);
#endif	/* VXB_PERFORM_SANITY_CHECKS */

	   stat = ERROR;
	   }

	/* terminate the subordinate bus */

	pDev->u.pSubordinateBus = NULL;

	/* delete the lock for the bus information */

	if (vxbLockDelete (&pBus->listLock) != OK)
	    {
	    /* restore the subordinate bus */

	    pDev->u.pSubordinateBus = pBus;

	    return (ERROR);
	    }
#ifndef _VXBUS_BASIC_HWMEMLIB
	/* free up the memory allocated */

	hwMemFree ((char *) pBus);
#endif /* !_VXBUS_BASIC_HWMEMLIB */
	}

    return (stat);
    }

/*********************************************************************
*
* vxbDriverUnregister - remove a device driver from the bus subsystem
*
* This routine is the final stage of driver removal.  The driver
* must disconnect itself from the OS, free resources, and then finally
* call this routine.  If the vxbDevRegInfo structure was dynamically
* allocated by the driver, the driver is responsible for freeing it.
*
* This routine dissociates the driver from each device to which it
* has been connected, and moves each such device onto the orphan
* list of the bus it resides on.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS vxbDriverUnregister
    (
    struct vxbDevRegInfo * pDriver
    )
    {
    struct vxbDevRegInfo * pDrvList = NULL;

    struct vxbDrvDisconnectData driverData;

    VXB_ASSERT(pDriver!=NULL, ERROR)

    driverData.pDriver = pDriver;
    driverData.status = OK;

    /* call instance disconnect routine */

    vxbDevIterate(vxbDrvDisconnect, &driverData, VXB_ITERATE_INSTANCES);

    /* if there is an ERROR in instances removal, return ERROR */

    if (driverData.status != OK)
        return ERROR;

    /* acquire the lock */

    vxbLockTake(&vxbGlobalListsLock, VXB_LOCK_WRITER);

    /* remove remaining references to driver */

    pDrvList = pDriverListHead;

    if ( pDrvList == pDriver )
        {
        pDriverListHead = pDrvList->pNext;
        pDrvList->pNext = NULL;
        }
    else
        {
        while ( pDrvList != NULL && pDrvList->pNext != NULL )
            {
            if ( pDrvList->pNext == pDriver )
                {
                pDrvList->pNext = pDriver->pNext;
                pDriver->pNext = NULL;

                /* release the lock */

                vxbLockGive(&vxbGlobalListsLock, VXB_LOCK_WRITER);

                return OK;
                }

            pDrvList = pDrvList->pNext;
            }
        if ((pDrvList == NULL) || (pDrvList->pNext == NULL))
            {
            vxbLockGive(&vxbGlobalListsLock, VXB_LOCK_WRITER);
            return ERROR;
            }
        }

    /* release the lock */

    vxbLockGive(&vxbGlobalListsLock, VXB_LOCK_WRITER);

    return OK;
    }
#endif	/* _WRS_CONFIG_VXBUS_BASIC */

/*********************************************************************
*
* vxbBusTypeRegister - register a bus type
*
* This routine registers a bus type with the vxBus subsystem.
*
* RETURNS: OK if successfully registered, else ERROR
*
* ERRNO: N/A
*/

STATUS vxbBusTypeRegister
    (
    struct vxbBusTypeInfo * pBusType
    )
    {
    struct vxbBusTypeInfo *pTempPtr;

    VXB_ASSERT(pBusType!=NULL, ERROR)

    /* acquire the lock */

    vxbLockTake(&vxbGlobalListsLock, VXB_LOCK_WRITER);

    pTempPtr = pBusListHead;

    /* check if there is a duplicate entry for this bus type */

    if (pBusListHead != NULL)
        {

        while (pTempPtr != NULL)
            {
            if (pTempPtr->busID == pBusType->busID)
                {
                vxbLockGive(&vxbGlobalListsLock, VXB_LOCK_WRITER);
                return ERROR;
                }

            pTempPtr = pTempPtr->pNext;
            }
        }

    /* update the list element */

    pBusType->pNext = pBusListHead;
    pBusListHead = pBusType;

    /* release the lock */

    vxbLockGive(&vxbGlobalListsLock, VXB_LOCK_WRITER);

    return OK;
    }

#ifndef	_WRS_CONFIG_VXBUS_BASIC

/*********************************************************************
*
* vxbBusTypeUnregister - unregister a bus type
*
* This routine unregisters a bus type with the vxBus subsystem.
*
* RETURNS: OK is success, else ERROR
*
* ERRNO: N/A
*/

STATUS vxbBusTypeUnregister
    (
    struct vxbBusTypeInfo * pBusType
    )
    {
    STATUS	status = OK;
    void * pBusListHeadDevId;
    struct vxbBusPresent * pBusPres;

    VXB_ASSERT(pBusType!=NULL, ERROR);
    VXB_ASSERT(pBusListHead!=NULL, ERROR);

    /* acquire the lock */

    vxbLockTake(&vxbBusListLock, VXB_LOCK_WRITER);

    /*
     * if there is a bus of this type present, the bus type
     * should not be unregistered
     */

    pBusPres = pPlbBus;

    while (pBusPres != NULL)
        {
        if (pBusPres->pBusType == pBusType)
            {
            vxbLockGive(&vxbBusListLock, VXB_LOCK_WRITER);
            return ERROR;
            }

        pBusPres = pBusPres->pNext;
        }

    pBusListHeadDevId = (void *) &pBusListHead;

    if (vxbInstRemove ((VXB_DEVICE_ID *)pBusListHeadDevId,
                       (VXB_DEVICE_ID)pBusType) != OK)
        status = ERROR;

    /* release the lock */

    vxbLockGive(&vxbBusListLock, VXB_LOCK_WRITER);

    return status;
    }
#endif	/* _WRS_CONFIG_VXBUS_BASIC */

/*********************************************************************
*
* vxbDeviceAnnounce - announce device discovery to bus subsystem
*
* This routine goes through existing bus types and drivers,
* attempting to match the device which has been discovered
* with a driver.  If a driver is found, then an instance is created,
* and the instance is added to the bus on which the device resides.
*
* If no driver is found for the device, then the device is added
* to a list of unattached devices.  At some future time, a driver
* matching this device may be added to the system.
*
* The pDev->pParentBus must be populated before calling this
* function. Otherwise, this device shows up in the
* lost Devices list(under "Lost devices in vxBus system") when
* vxBusShow () is called
*
* RETURNS: OK, or FALSE
*
* ERRNO: N/A
*/

STATUS vxbDeviceAnnounce
    (
    struct vxbDev * pDev  /* device information */
    )
    {
    struct vxbBusTypeInfo * pBusEntry;
    struct vxbBusTypeInfo * busMatch = NULL;
    BOOL         drvFound = FALSE;
    struct vxbDevRegInfo *  pDrv;
    struct vxbBusPresent * pBus;
    FUNCPTR pMethod;

    if ( pPlbDev == NULL && pDev->busID == VXB_BUSID_LOCAL )
        {
        pPlbDev = pDev;
        return(OK);
        }

    VXB_DEBUG_MSG(1, "vxbDeviceAnnounce(%p: %s)\n", pDev,
                  pDev->pName, 3, 4, 5, 6);

    /* 
     * BSPs may want to exclude hardware from VxBus at runtime, rather 
     * than by making modifications to hwconf.c on an image-by-image 
     * basis.  If they want to, they can associate the sysBspDevFilter 
     * with the PLB device prior to starting VxBus.  If VxBus finds
     * this method, it passes the 'vxbDev' structure to the filter.
     * The filter returns OK if the hardware should be used by VxBus,
     * and returns ERROR otherwise.
     */

    if (pPlbDev != NULL)
	{
	pMethod = vxbDevMethodGet(pPlbDev, DEVMETHOD_CALL(sysBspDevFilter));
	if (pMethod != NULL)
	    {
	    if ((*pMethod)(pDev) != OK)
		{
		VXB_DEBUG_MSG(1,
			      "vxbDeviceAnnounce(%p(%s)) excluded by BSP\n",
			      pDev,
			      pDev->pName, 3, 4, 5, 6);

		return ERROR;
		}
	    }  
	}

    if (pDev->pParentBus == NULL)
        {
#ifdef VXB_PERFORM_SANITY_CHECKS
        vxbLockTake(&vxbLostDevListLock, VXB_LOCK_WRITER);

        vxbInstInsert (&pLostDevHead, pDev);

        vxbLockGive(&vxbLostDevListLock, VXB_LOCK_WRITER);

#endif /* VXB_PERFORM_SANITY_CHECKS */

        return ERROR;
        }

    /* acquire global lock as reader */

    vxbLockTake(&vxbGlobalListsLock, VXB_LOCK_READER);

    for ( pBusEntry = pBusListHead ; pBusEntry != NULL ;
          pBusEntry = pBusEntry->pNext )
        {

        /* check for matching bus type */

        if ( pBusEntry->busID != pDev->busID )
            continue;

        for ( pDrv = pDriverListHead ; pDrv != NULL ;
             pDrv = pDrv->pNext )
            {

            VXB_DEBUG_MSG(1, "vxbDeviceAnnounce(): checking %p (%s) "
                             "against %s\n",
                          pDev, pDev->pName,
                          &pDrv->drvName[0], 4, 5, 6);

            if ( pDrv->busID != pDev->busID )
                {
                VXB_DEBUG_MSG(1, "vxbDeviceAnnounce(): "
                                 "%s@%p failed type check\n",
                              pDev->pName, pDev, 3, 4, 5, 6);
                continue;
                }

            /* check bus-specific match routine */

            drvFound = (*(pBusEntry->vxbDevMatch))(pDrv, pDev);

            if ( ! drvFound )
                {
                VXB_DEBUG_MSG(1, "vxbDeviceAnnounce(): "
                                 "%s@%p failed bus match\n",
                              pDev->pName, pDev, 3, 4, 5, 6);
                continue;
                }

            busMatch = pBusEntry;

            /* check driver-supplied probe routine */

            if ( pDrv->devProbe == NULL )
                {
                VXB_DEBUG_MSG(1, "vxbDeviceAnnounce(): "
                                 "no driver probe available\n",
                              1, 2, 3, 4, 5, 6);
                drvFound = TRUE;
                }
            else
                {
                VXB_DEBUG_MSG(1, "vxbDeviceAnnounce(): "
                                 "calling driver probe\n",
                              1, 2, 3, 4, 5, 6);

                drvFound = (*(pDrv->devProbe))(pDev);
                if ( drvFound == FALSE )
                    {
                    VXB_DEBUG_MSG(1, "vxbDeviceAnnounce(): "
                                     "driver probe failed\n",
                                  1, 2, 3, 4, 5, 6);
                    continue;
                    }
                }

            VXB_DEBUG_MSG(1, "vxbDeviceAnnounce(): "
                             "found match, driver @ %p\n",
                          pDrv, 2, 3, 4, 5, 6);

            /* attach driver registration info */

            pDev->pDriver = pDrv;

            /* 
             * Adjust name, if pDev->pName isn't NULL, the name was assigned
             * when the device was created by bus controller driver, just keep
             * it. If the name is not assigned when the deivce was created, 
             * set the name to driver name.
             */

            if (pDev->pName == NULL)
                pDev->pName = &pDrv->drvName[0];

            /* get parent bus */

            pBus = (struct vxbBusPresent *)pDev->pParentBus;

            /* acquire the lock */

            vxbLockTake(&pBus->listLock, VXB_LOCK_WRITER);

            /* add this instance to bus device list */

            vxbInstInsert (&pBus->instList, pDev);

            /* release the lock */

            vxbLockGive(&pBus->listLock, VXB_LOCK_WRITER);

            /* perform initialization */

            vxbDevInitRun(pDev, pDrv);

            break;
            }
        }

    /* release global lock */

    vxbLockGive(&vxbGlobalListsLock, VXB_LOCK_READER);

    if ( drvFound == FALSE )
        {
        /* get parent bus */

        pBus = (struct vxbBusPresent *)pDev->pParentBus;

        /* insure pDriver is initialized */

        pDev->pDriver = NULL;

        /* acquire the lock */

        vxbLockTake(&pBus->listLock, VXB_LOCK_WRITER);

        /* keep track of unattached device */

        vxbInstInsert (&pBus->devList, pDev);

        /* release the lock */

        vxbLockGive(&pBus->listLock, VXB_LOCK_WRITER);
        }

	VXB_DEBUG_MSG(1, "\n", 1, 2, 3, 4, 5, 6);
    return OK;
}

/*********************************************************************
*
* vxbDevOnlyMethodGet - find entry point of method
*
* This routine finds a device-specific entry point for specified
* method.  Methods advertised from the driver's registration
* structure are not found.
*
* RETURNS: a pointer to the entry point routine for accessing the
* specified functionality of the specified device, or NULL if no such
* functionality is available.
*
* ERRNO: N/A
*/

LOCAL FUNCPTR vxbDevOnlyMethodGet
    (
    struct vxbDev * pDev,    /* Device information */
    VXB_METHOD_ID   method   /* Specified method */
    )
    {
    struct vxbDeviceMethod * pMethods;

    pMethods = pDev->pMethods;
    if ( pMethods != NULL )
        {
        while ( pMethods->devMethodId != 0 && pMethods->handler != NULL )
            {
            if ( pMethods->devMethodId == method )
                return(pMethods->handler);
            pMethods++;
            }
        }

    return(NULL);
    }

#ifndef	_WRS_CONFIG_VXBUS_BASIC
/*********************************************************************
*
* vxbDevFromBusRemove - remove device from bus
*
* This routine removes the device from the bus to which it is
* connected.  The caller must provide the parent.  This routine 
* does not handle bus controller devices.  If any bus controller
* devices with subordinate devices is passed to this routine, the
* subordinate devices will be lost.
*
* If the driver provides the {vxbDrvUnlink}() method, then it 
* is called before any further action. 
*
* RETURNS: OK, or ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbDevFromBusRemove
    (
    struct vxbDev *     devID,          /* device to be removed */
    struct vxbDev *     parentID,       /* parent bus controller */
    BOOL                remove          /* remove entirely? */
    )
    {
    struct vxbBusPresent * pBus; 
    FUNCPTR             removalRtn;
    STATUS              status = OK;

#ifdef	VXBUS_DEBUG_ON
    if (parentID != NULL)
	{
	VXB_DEBUG_MSG(10,
		      "vxbDevFromBusRemove(%s%d, %s%d, %s), flags = 0x%x\n",
		      devID->pName, devID->unitNumber,
		      parentID->pName, parentID->unitNumber,
		      remove ? "TRUE" : "FALSE",devID->flags);
	}
    else
	{
	VXB_DEBUG_MSG(10,
                      "vxbDevFromBusRemove(%s%d, %p, %s), flags = 0x%x\n",
		      devID->pName, devID->unitNumber, parentID,
		      remove ? "TRUE" : "FALSE", devID->flags, 6);
	}
#endif	/* VXBUS_DEBUG_ON */
    
    /* take device lock */
    vxbLockTake (&devID->devLock, VXB_LOCK_WRITER);
    
    if ( ( devID->flags & VXB_REMOVAL_MASK) == 0 )
        {

        /* If the driver provides a removal routine, call it */

        removalRtn = vxbDevMethodGet(devID, DEVMETHOD_CALL(vxbDrvUnlink));
        VXB_DEBUG_MSG(100, "vxbDevFromBusRemove(): (%s%d) removalRtn @ %p\n",
                      devID->pName, devID->unitNumber, removalRtn, 4, 5, 6);

        if (removalRtn != NULL)
            {
            /* execute the method */

            if (((*removalRtn)(devID, NULL)) != OK)
                {
                VXB_DEBUG_MSG(10, "vxbDevFromBusRemove(): (%s%d) (*%p)(%p, 0) "
                                  "returned ERROR\n",
                              devID->pName, devID->unitNumber,
                              removalRtn, devID, 5, 6);

                status = ERROR;
                goto vxbDevFromBusRemoveExit;
                }
            }

        if ( devID->pDriver != NULL )
            {
#ifdef	_WRS_CONFIG_LP64
	    /* unmap the BARs */

	    if (vxbRegUnmapAll (devID) != OK)
                {
                VXB_DEBUG_MSG(10, "vxbDevFromBusRemove(): (%s%d) "
                                  "vxbRegUnmapAll failed \n",
                              devID->pName, devID->unitNumber,
                              3, 4, 5, 6);

                status = ERROR;
                goto vxbDevFromBusRemoveExit;
                }

#endif	/* _WRS_CONFIG_LP64 */

            /* clean up driver-specific parts of device structure */

            devID->flags &= ~( VXB_INST_INIT_DONE | VXB_INST_INIT2_DONE |
                               VXB_INST_CONNECT_DONE );
            devID->flags |= VXB_REMOVAL_INSTANCE;

            /* if the pName point to drvName, zero it */

            if (devID->pName == &devID->pDriver->drvName[0])
                devID->pName = NULL;

            devID->pDriver = NULL;
            devID->pMethods = NULL;
            devID->pDrvCtrl = NULL;

            if (devID->pParams != NULL)
#ifndef _VXBUS_BASIC_HWMEMLIB                
                hwMemFree((char *)(devID->pParams));
#endif /* !_VXBUS_BASIC_HWMEMLIB */
            devID->pParams = NULL;
            }
        else
            {
            devID->flags |= VXB_REMOVAL_ORPHAN;
            }
        }

    if ( ! remove )
        {
		status = OK;
        goto vxbDevFromBusRemoveExit;
        }

    /* if the parent bus is NULL, remove the device from lost device list */

    if (parentID == NULL)
        {
#ifdef	VXB_PERFORM_SANITY_CHECKS
        vxbLockTake(&vxbLostDevListLock, VXB_LOCK_WRITER);

        if (vxbInstRemove (&pLostDevHead, devID) != OK)
            {
            VXB_DEBUG_MSG(10, "vxbDevFromBusRemove(): vxbInstRemove "
                              "(&pLostDevHead, devID) returned ERROR\n",
                          1, 2, 3, 4, 5, 6);
            }

        vxbLockGive(&vxbLostDevListLock, VXB_LOCK_WRITER);
        status = ERROR;
        goto vxbDevFromBusRemoveExit; 
#else
        status = ERROR;
        goto vxbDevFromBusRemoveExit;
#endif  /* VXB_PERFORM_SANITY_CHECKS */
        }

    pBus = parentID->u.pSubordinateBus;

    /* remove the device from the appropriate list */

    vxbLockTake(&pBus->listLock, VXB_LOCK_WRITER);

    if ( ( devID->flags & VXB_REMOVAL_MASK ) == VXB_REMOVAL_INSTANCE )
        {
        if (vxbInstRemove (&pBus->instList, devID) != OK)
            {
            VXB_DEBUG_MSG(10, "vxbDevFromBusRemove(): vxbInstRemove "
                              "(&pBus->instList, devID) returned ERROR\n",
                          1, 2, 3, 4, 5, 6);
            status = ERROR;
            }
        }
    else
        {
        if (vxbInstRemove (&pBus->devList, devID) != OK)
            {
            VXB_DEBUG_MSG(10, "vxbDevFromBusRemove(): vxbInstRemove "
                              "(&pBus->devList, devID) returned ERROR\n",
                          1, 2, 3, 4, 5, 6);
            status = ERROR;
            }
        }

    vxbLockGive(&pBus->listLock, VXB_LOCK_WRITER);
	
vxbDevFromBusRemoveExit:
	
    /* release device lock */

    vxbLockGive (&devID->devLock, VXB_LOCK_WRITER);
	
    return(status);
    }

/*********************************************************************
*
* vxbDevRemovalAnnounce - announce device removal to bus subsystem
*
* This routine removes the device from the bus subsystem.
* If there is an instance associated with this device, then the
* driver's method for unlink is called and device
* is removed from the instances list. If the
* driver  does not implement this method, then this function returns
* an ERROR.
* If the device is present in the orphans list, it is
* removed from the orphans list.
*
* RETURNS: OK, or ERROR
*
* ERRNO: N/A
*/

STATUS vxbDevRemovalAnnounce
    (
    VXB_DEVICE_ID devID
    )
    {
    VXB_DEVICE_ID parent;
    STATUS                      stat;

    /* find parent bus controller */

    parent = vxbDevParent(devID);

    VXB_DEBUG_MSG(10, "parent(%p) = %p\n", devID, parent, 3, 4, 5, 6);

    /* call remove method for device */

    stat = vxbDevFromBusRemove(devID, parent, FALSE);
    if ( stat != OK )
        {
        VXB_DEBUG_MSG(1, "vxbDevFromBusRemove(%p, %p, FALSE) returned %d\n",
                      devID, parent, (int)stat, 4, 5, 6);

        return(ERROR);
        }

    /* if bus controller with subordinate devices, remove those */

    stat = vxbSubordinateBusRemove (devID);
    if (stat != OK)
        {
        VXB_DEBUG_MSG(1, "vxbSubordinateBusRemove(%p) returned %d\n",
                      devID, (int)stat, 3, 4, 5, 6);

        return (ERROR);
        }

    /* unlink this device from it's parent */

    stat = vxbDevFromBusRemove (devID, parent, TRUE);
    if (stat != OK)
        {
        VXB_DEBUG_MSG(1, "vxbDevFromBusRemove(%p, %p, TRUE) returned %d\n",
                      devID, parent, (int)stat, 4, 5, 6);

        return (ERROR);
        }

    /* return this device to the pool */

    vxbDevStructFree(devID);

    return (OK);
    }
#endif	/* _WRS_CONFIG_VXBUS_BASIC */

/*********************************************************************
*
* vxbBusAnnounce - announce bus discovery to bus subsystem
*
* This routine is called by bus controller drivers, whenever a
* new bus is found.
*
* The pBusDev->pParentBus must be populated before calling this
* function. Otherwise, the bus shows up as a lost bus when
* vxBusShow () is called
*
* RETURNS: OK, or ERROR
*
* ERRNO: N/A
*/

STATUS vxbBusAnnounce
    (
    struct vxbDev *     pBusDev,    /* bus controller */
    UINT32              busID       /* bus type */
    )
    {
    struct vxbBusPresent * pBusEnt;
    struct vxbBusPresent * pParent;
    struct vxbBusTypeInfo * pType;
    struct vxbDev * pTempDev;

    VXB_ASSERT(pBusDev!=NULL, ERROR)

    /* retrieve the parent device information */

    pParent = pBusDev->pParentBus;

    /*
     * if the parent device is not NULL, locate the
     * device in the parent's instance or orphan's list
     */

    if (pParent != NULL)
        {

        /* acquire the lock */

        vxbLockTake(&pParent->listLock, VXB_LOCK_WRITER);

        /* retrieve the head of the instances or the orphans list */

        if (pBusDev->pDriver != NULL)
            pTempDev = pParent->instList;
        else
            pTempDev = pParent->devList;

        /* verify whether the device has been announced already */

        while (pTempDev != NULL)
            {
            if (pTempDev == pBusDev)
                break;

            pTempDev = pTempDev->pNext;
            }

        /* release the lock */

        vxbLockGive(&pParent->listLock, VXB_LOCK_WRITER);

        if (pTempDev == NULL)
            return ERROR;
        }

    /* allocate bus entry */

    pBusEnt = (struct vxbBusPresent *)hwMemAlloc(sizeof(*pBusEnt));
    if ( pBusEnt == NULL )
        return(ERROR);

    if ( pPlbBus == NULL && busID == VXB_BUSID_LOCAL )
        {
        pPlbBus = pBusEnt;
        pPlbDev->pParentBus = NULL;
        }

    /*
     * initialize the lock used for protecting updations
     * to the instances and orphans lists
     */

    if ( vxbInitPhase > 1 )
        vxbLockInit(&pBusEnt->listLock);

    /* find bus type record */

    pBusEnt->pBusType = NULL;

    /* acquire the lock */

    vxbLockTake(&vxbGlobalListsLock, VXB_LOCK_READER);

    for ( pType = pBusListHead ; pType != NULL ; pType = pType->pNext )
        if ( pType->busID == busID )
            {
            pBusEnt->pBusType = pType;
            break;
            }

    /* release the lock */

    vxbLockGive(&vxbGlobalListsLock, VXB_LOCK_READER);

    /* if the bus type is not located, return error */

    if (pType == NULL)
        {
        if (pPlbBus == pBusEnt)
            pPlbBus = NULL;

#ifndef _VXBUS_BASIC_HWMEMLIB
        hwMemFree((char *)pBusEnt);

#endif /* !_VXBUS_BASIC_HWMEMLIB */   
     
        return ERROR;
        }

    /* fill in controller record */

    pBusEnt->pCtlr = pBusDev;

    /* initialize devices on this bus */

    pBusEnt->instList = NULL;   /* devices w/ driver */
    pBusEnt->devList = NULL;    /* devices w/o driver */

    /* find parent */

    pParent = pBusEnt->pCtlr->pParentBus;

    if ( pParent != NULL )
        {
	void * pParentDevId = (void *) &pParent;

        /* acquire the lock */

        vxbLockTake(&vxbBusListLock, VXB_LOCK_WRITER);

        /* link into list */

        vxbInstInsert ((VXB_DEVICE_ID *)pParentDevId, (VXB_DEVICE_ID)pBusEnt);

        /* release the lock */

        vxbLockGive(&vxbBusListLock, VXB_LOCK_WRITER);
        }

    /* update busInfo field of pBusDev */

    pBusDev->u.pSubordinateBus = pBusEnt;

    /*
     * if the parent bus is not available and if the bus device is not
     * a PLB bus device, then update the lost buses list and return
     * ERROR.
     */

    if ((pParent == NULL) && (pPlbDev != pBusDev))
        {

#ifdef	VXB_PERFORM_SANITY_CHECKS

        /* acquire the lock */

        vxbLockTake(&vxbLostDevListLock, VXB_LOCK_WRITER);

        pBusEnt->pNext = pLostBusHead;
        pLostBusHead = pBusEnt;

        /* release the lock */

        vxbLockGive(&vxbLostDevListLock, VXB_LOCK_WRITER);

#endif	/* VXB_PERFORM_SANITY_CHECKS */
        return ERROR;
        }

    return(OK);
    }

/*********************************************************************
*
* vxbDevParent - find parent device
*
* This routine finds the parent device of the specified device.
*
* RETURNS: pointer to parent device, or NULL
*
* ERRNO: N/A
*/

struct vxbDev * vxbDevParent
    (
    struct vxbDev * pDev    /* Device Information */
    )
    {
    struct vxbBusPresent * pBusPres;
    struct vxbDev * pParentDev;

    if (pDev == NULL)
	return (NULL);

    pBusPres = (struct vxbBusPresent *)pDev->pParentBus;

    if ( pBusPres == NULL )
        return(NULL);

    pParentDev = pBusPres->pCtlr;

    return(pParentDev);
    }

/*********************************************************************
*
* vxbDevPath - trace from device to nexus
*
* This routine traces the specified device to the PLB bus.
*
* RETURNS: OK, or ERROR
*
* ERRNO: N/A
*/

STATUS vxbDevPath
    (
    struct vxbDev * pDev,    /* device */
    BOOL (*func)(struct vxbDev * pDev, void * pArg), /* func @ each ctlr */
    void *      pArg        /* 2nd arg to func */
    )
    {
    struct vxbDev * pParentDev;
    BOOL        retVal;

    VXB_ASSERT(pDev!=NULL, ERROR)
    VXB_ASSERT(func!=NULL, ERROR)

    retVal = (*func)(pDev, pArg);
    if ( retVal )
        return(OK);

    pParentDev = vxbDevParent(pDev);

    if ( pParentDev == NULL )
        return(OK);

    return(vxbDevPath(pParentDev, func, pArg));
    }

/*********************************************************************
*
* vxbDevMethodGet - find entry point of method
*
* This routine finds an entry point for specified method.
*
* RETURNS: a pointer to the entry point routine for accessing the
* specified functionality of the specified device, or NULL if no such
* functionality is available.
*
* ERRNO: N/A
*/

FUNCPTR vxbDevMethodGet
    (
    struct vxbDev * pDev,    /* Device information */
    VXB_METHOD_ID   method   /* Specified method */
    )
    {
    struct vxbDeviceMethod * pMethods;
    FUNCPTR methodRoutine;

    VXB_ASSERT(method!=0, NULL)

    /*
     * sanity check must be done
     *
     * Note: Some APIs in vxbDmaBufLib call this function
     *       without testing the parent's Dev ID and may
     *       pass a NULL pointer as the device
     *       information pointer.
     */

    if ( pDev == NULL )
        return(NULL);


    /* first check instance-specific methods */

    methodRoutine = vxbDevOnlyMethodGet(pDev, method);
    if ( methodRoutine != NULL )
        return(methodRoutine);

    /* next check driver-specific methods */

    /* driver registration info present? */

    if ( pDev->pDriver == NULL )
        return(NULL);

    /* get methods list */

    pMethods = pDev->pDriver->pMethods;
    if ( pMethods == NULL )
        return(NULL);

    while ( pMethods->devMethodId != 0 && pMethods->handler != NULL )
        {
        if ( pMethods->devMethodId == method )
            return(pMethods->handler);
        pMethods++;
        }

    /* method not available.  Return NULL */

    return(NULL);

    }


/*********************************************************************
*
* vxbDevIterate - perform specified action for each device
*
* This routine walks through VxBus device list to perform the specified
* <func> with argument <pArg>. Which VxBus device list to walk through
* is determinted by <flags>.
*
* The return value of <func> is not checked internally in this routine, 
* which means even it meets EEROR, it still goes on to perform the <func> 
* for the remaining devices on that list.
*
* <flags> can be either of the following and cannot be ORed.
* \is
* \i "VXB_ITERATE_INSTANCES  (1)"
* walk through device instances list.
* \i "VXB_ITERATE_ORPHANS  (2)"
* walk through orphan device list.
* \ie
*
* RETURNS: OK, or ERROR when pPlbBus is NULL
*
* ERRNO: N/A
*/

STATUS vxbDevIterate
    (
    FUNCPTR func,       /* function to call */
    void *  pArg,       /* 2nd argument to func */
    UINT32  flags       /* flags to determine what to do */
    )
    {
    struct vxbBusPresent * pBusPres;
    struct vxbDev * pInst, * pNext;

    VXB_ASSERT(func!=NULL, ERROR)

    /* if PLB bus is not initialized, return ERROR */

    if (pPlbBus == NULL)
	return (ERROR);

    pBusPres = pPlbBus;

    /* start with PLB controller */

    (*func)(pBusPres->pCtlr, pArg);

    while ( pBusPres != NULL )
        {

        if ( ( flags & VXB_ITERATE_INSTANCES ) == VXB_ITERATE_INSTANCES )
            {

#ifndef IA32_LOCK_REMOVAL

            /* acquire the lock */

            vxbLockTake(&pBusPres->listLock, VXB_LOCK_WRITER);

#endif /* IA32_LOCK_REMOVAL */

            pInst = pBusPres->instList;

#ifndef IA32_LOCK_REMOVAL

            /* release the lock */

            vxbLockGive(&pBusPres->listLock, VXB_LOCK_WRITER);

#endif /* IA32_LOCK_REMOVAL */

            for ( ; pInst != NULL ; pInst = pNext )
                {
#ifndef IA32_LOCK_REMOVAL

                /* acquire the lock */

                vxbLockTake(&pBusPres->listLock, VXB_LOCK_WRITER);

#endif /* IA32_LOCK_REMOVAL */

                pNext = pInst->pNext;

#ifndef IA32_LOCK_REMOVAL

                /* 
                 * release the lock to prevent recursive take 
                 * of the same lock
                 */

                vxbLockGive(&pBusPres->listLock, VXB_LOCK_WRITER);
#endif /* IA32_LOCK_REMOVAL */

                (*func)(pInst, pArg);
                }
            }

        if ( ( flags & VXB_ITERATE_ORPHANS ) == VXB_ITERATE_ORPHANS )
            {

#ifndef IA32_LOCK_REMOVAL

            /* acquire the lock */

            vxbLockTake(&pBusPres->listLock, VXB_LOCK_WRITER);

#endif /* IA32_LOCK_REMOVAL */

            pInst = pBusPres->devList;

#ifndef IA32_LOCK_REMOVAL

            /* release the lock */

            vxbLockGive(&pBusPres->listLock, VXB_LOCK_WRITER);

#endif /* IA32_LOCK_REMOVAL */

            for ( ; pInst != NULL ; pInst = pNext )
                {
#ifndef IA32_LOCK_REMOVAL

                /* acquire the lock */

                vxbLockTake(&pBusPres->listLock, VXB_LOCK_WRITER);

#endif /* IA32_LOCK_REMOVAL */

                pNext = pInst->pNext;

#ifndef IA32_LOCK_REMOVAL

                /* 
                 * release lock to prevent recursive take of 
                 * the same lock 
                 */

                vxbLockGive(&pBusPres->listLock, VXB_LOCK_WRITER);

#endif /* IA32_LOCK_REMOVAL */

                (*func)(pInst, pArg);
                }
            }

#ifndef IA32_LOCK_REMOVAL

        /* acquire the lock to protect the buses list */

        vxbLockTake(&vxbBusListLock, VXB_LOCK_WRITER);

#endif /* IA32_LOCK_REMOVAL */

        pBusPres = pBusPres->pNext;

#ifndef IA32_LOCK_REMOVAL

        /* release the lock */

        vxbLockGive(&vxbBusListLock, VXB_LOCK_WRITER);

#endif /* IA32_LOCK_REMOVAL */

        }

    return(OK);
    }

/*********************************************************************
*
* vxbDeviceMethodRun - run method on device
*
* This routine runs a method on a device if it provides specified
* method.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS vxbDeviceMethodRun
    (
    struct vxbDev * pDev,    /* Device Information */
    void *      pArg         /* Parameter to method */
    )
    {
    struct vxbMethodRunInfo * pBMRI = (struct vxbMethodRunInfo *)pArg;
    FUNCPTR     handler;

    VXB_ASSERT(pArg!=NULL, ERROR)

    /* find handler */

    handler = vxbDevMethodGet(pDev, pBMRI->method);

    /* if no method handler, vxbIterate() should continue */

    if ( handler == NULL )
        return(ERROR);

    /* call method handler */

    return ((*handler)(pDev, pBMRI->pArg));

    }

/*********************************************************************
*
* vxbDevMethodRun - run method on devices
*
* This routine runs the specified method for all instances which
* provide a specified method.
*
* RETURNS: OK, always
*
* ERRNO: N/A
*/

STATUS vxbDevMethodRun
    (
    VXB_METHOD_ID	method,	/* Method to run */
    void * pArg      /* Argument to routine */
    )
    {
    struct vxbMethodRunInfo m;

    m.method = method;
    m.pArg = pArg;

    vxbDevIterate(vxbDeviceMethodRun, &m, VXB_ITERATE_INSTANCES);

    return(OK);
    }

/*********************************************************************
*
* vxbSubDevAction - perform an action on all devs on bus controller
*
* This routine performs the specified action (that is, calls
* the specified function) for each device directly connected
* to the bus downstream from the specified bus controller.
*
* RETURNS: OK, or ERROR
*
* ERRNO: N/A
*/

STATUS vxbSubDevAction
    (
    struct vxbDev *		pBusCtlr,
    VXB_SUBDEV_ACTION_FUNC	actionFunc,
    char *			pArg,
    UINT32			flags
    )
    {
    struct vxbBusPresent *      pBus;
    VXB_DEVICE_ID               pDevOnBus;
    VXB_DEVICE_ID               pNextDevOnBus;
    STATUS                      status;
    STATUS                      retStat = OK;

    VXB_ASSERT(pBusCtlr!=NULL, ERROR)
    VXB_ASSERT(actionFunc!=NULL, ERROR)

    pBus = pBusCtlr->u.pSubordinateBus;

    VXB_ASSERT(pBus!=NULL, ERROR)

    /* iterate through all the instances connected to the bus */

    if ( ( flags & VXB_ITERATE_INSTANCES ) == VXB_ITERATE_INSTANCES )
        {

        /* acquire the lock */

        vxbLockTake(&pBus->listLock, VXB_LOCK_WRITER);

        pDevOnBus = pBus->instList;

        /* release the lock */

        vxbLockGive(&pBus->listLock, VXB_LOCK_WRITER);

        while ( pDevOnBus != NULL )
            {

            /* acquire the lock */

            vxbLockTake(&pBus->listLock, VXB_LOCK_WRITER);

            pNextDevOnBus = pDevOnBus->pNext;

            /* release the lock */

            vxbLockGive(&pBus->listLock, VXB_LOCK_WRITER);

            /* call the function for the instance */

            status = (*actionFunc)(pDevOnBus, pBusCtlr, pArg);

            /*
             * if the function call returns ERROR, the
             * return status of vxbSubDevAction () will be ERROR.
             */

            if ( status != OK )
                retStat = ERROR;

            /* 
             * move on to the next instance in the list 
             * of instances 
             */

            pDevOnBus = pNextDevOnBus;

            }
        }

    /* iterate through all the orphans connected to the bus */

    if ( ( flags & VXB_ITERATE_ORPHANS ) == VXB_ITERATE_ORPHANS )
        {

        /* acquire the lock */

        vxbLockTake(&pBus->listLock, VXB_LOCK_WRITER);

        pDevOnBus = pBus->devList;

        /* release the lock */

        vxbLockGive(&pBus->listLock, VXB_LOCK_WRITER);

        while ( pDevOnBus != NULL )
            {

            /* acquire the lock */

            vxbLockTake(&pBus->listLock, VXB_LOCK_WRITER);

            pNextDevOnBus = pDevOnBus->pNext;

            /* release the lock */

            vxbLockGive(&pBus->listLock, VXB_LOCK_WRITER);

            /* call the function for the orphan */

            status = (*actionFunc)(pDevOnBus, pBusCtlr, pArg);

            /*
             * if the function call returns ERROR, the
             * return status of vxbSubDevAction () will be ERROR
             */

            if ( status != OK )
                retStat = ERROR;

            /* move on to the next orphan in the list of orphans */

            pDevOnBus = pNextDevOnBus;

            }
        }

    return(retStat);
    }

/*********************************************************************
*
* vxbResourceSearch - find and allocate a vxBus resource
*
* This routine searches for a VxBus resource to allocate. Resources
* are identified by a driver method. The driver method, when called,
* must allocate and initialize the specified resource, fill in the
* appropriate fields in the structure pointed to by the pArg variable,
* and return OK.
*
* RETURNS: OK, or ERROR if the resource could not be allocated
*
* ERRNO: N/A
*/

LOCAL STATUS vxbResourceSearch
    (
    struct vxbDev *     instance,
    struct vxbDev *     pParentDev,
    VXB_METHOD_ID       method,
    char *           pArg
    )
    {
    struct vxbDev *     pDevOnBus;
    struct vxbBusPresent *  pBus;
    STATUS          (*methodHandler)(VXB_DEVICE_ID, VXB_DEVICE_ID, char *);
    STATUS          status;

    VXB_ASSERT(instance!=NULL, ERROR)

    if (pParentDev == NULL)
	return (ERROR);

    /* iterate through devices directly connected to the parent bus */

    pBus = pParentDev->u.pSubordinateBus;

    /* acquire the lock */

    vxbLockTake(&pBus->listLock, VXB_LOCK_WRITER);

    pDevOnBus = pBus->instList;

    /* release the lock */

    vxbLockGive(&pBus->listLock, VXB_LOCK_WRITER);

    while ( pDevOnBus != NULL )
        {
        methodHandler = vxbDevMethodGet(pDevOnBus, method);
        if ( methodHandler != NULL )
            {
            status = (*methodHandler)(pDevOnBus, instance, pArg);
            if ( status == OK )
                return(OK);
            }

        /* acquire the lock */

        vxbLockTake(&pBus->listLock, VXB_LOCK_WRITER);

        pDevOnBus = pDevOnBus->pNext;

        /* release the lock */

        vxbLockGive(&pBus->listLock, VXB_LOCK_WRITER);
        }

    pParentDev = vxbDevParent(pParentDev);

    /* if no resource available on this bus, check upstream bus */

    return(vxbResourceSearch(instance, pParentDev, method, pArg));
    }

/*********************************************************************
*
* vxbResourceFind - find and allocate a vxBus resource
*
* This routine searches for a vxBus resource to allocate. Resources
* are identified by a driver method. The driver method, when called,
* must allocate and initialize the specified resource, fill in the
* appropriate fields in the structure pointed to by the pArg variable,
* and return OK.
*
* The flags are currently ignored.  The flags field is present
* for a future enhancement to allow different behaviors:
*
* 1) Check only PLB
* 2) Check only bus on which device resides
* 3) Check all devices if none found on direct path
*
* RETURNS: OK, or ERROR if the resource could not be allocated
*
* ERRNO: N/A
*/

STATUS vxbResourceFind
    (
    struct vxbDev * instance,
    VXB_METHOD_ID   method,
    char * pArg,
    UINT flags
    )
    {
    struct vxbDev *     pParentDev;

    /* find parent bus controller instance */

    pParentDev = vxbDevParent(instance);

    /* if no parent, terminate processing with ERROR */

    if ( pParentDev == NULL )
        return(ERROR);

    return(vxbResourceSearch(instance, pParentDev, method, pArg));
    }

/*********************************************************************
*
* noDev - optional driver functionality not present
*
* This routine indicates that there is no optional device driver
* functionality present.
*
* RETURNS: FALSE, always
*
* ERRNO: N/A
*/

BOOL noDev(void)
    {
    return(FALSE);
    }

/*********************************************************************
*
* vxbBusTypeString - retrieve bus type string
*
* This routine returns the bus type string based on the bus type
* integer value
*
* RETURNS: pointer to char
*
* ERRNO: N/A
*/

char * vxbBusTypeString
    (
    int busType
    )
    {
    static char * lcl = "PLB_Bus";
    static char * vme = "VME_Bus";
    static char * pci = "PCI_Bus";
    static char * pcix = "PCI-X_Bus";
    static char * pcixp = "PCI-Express_Bus";
    static char * i2c = "I2C_Bus";
    static char * htrans = "HyperTransport_Bus";
    static char * rio = "RapidIO_Bus";
    static char * mii = "MII_Bus";
    static char * spi = "SPI_Bus";
    static char * virtual = "Virtual_Bus";
    static char * unkn = "UnknownType_Bus";
    static char * usbEhci = "USB-EHCI_Bus";
    static char * usbOhci = "USB-OHCI_Bus";
    static char * usbUhci = "USB-UHCI_Bus";
    static char * usbSynopsysHci = "USB-SYNOPSYSHCI_Bus";
    static char * usbMhci = "USB-MHCI_Bus";
    static char * usbHost  = "USB-Host_Bus";
    static char * usbHub  = "USB-HUB_Bus";
    static char * pMf = "MF_Bus";
    static char * sdMmc = "SDMMC_BUS";

    switch (busType)
        {
        case VXB_BUSID_LOCAL:
            return(lcl);
        case VXB_BUSID_VME:
            return(vme);
        case VXB_BUSID_PCI:
            return(pci);
        case VXB_BUSID_PCIX:
            return(pcix);
        case VXB_BUSID_PCIEXPRESS:
            return(pcixp);
        case VXB_BUSID_HYPERTRANSPORT:
            return(htrans);
        case VXB_BUSID_I2C:
            return(i2c);
        case VXB_BUSID_SPI:
            return(spi);
        case VXB_BUSID_RAPIDIO:
            return(rio);
        case VXB_BUSID_MII:
            return(mii);
        case VXB_BUSID_VIRTUAL:
            return(virtual);
        case VXB_BUSID_USB_HOST_EHCI:
            return(usbEhci);
        case VXB_BUSID_USB_HOST_OHCI:
            return(usbOhci);
        case VXB_BUSID_USB_HOST_UHCI:
            return(usbUhci);
        case VXB_BUSID_USB_HOST_SYNOPSYSHCI:
            return(usbSynopsysHci);
        case VXB_BUSID_USB_HOST_MHCI:
            return(usbMhci);
        case VXB_BUSID_USB_HOST:
            return usbHost;
        case VXB_BUSID_USB_HUB:
            return(usbHub);
        case VXB_BUSID_MF:
            return(pMf);
        case VXB_BUSID_SD:
            return(sdMmc);
        default:
            return(unkn);
        }
    }

#ifdef VXB_LEGACY_ACCESS

/*********************************************************************
*
* vxbAccessMethodGet - find specific method for accessing device
*
* This routine finds the specified method for accessing the
* specified device.
*
* RETURNS: pointer to method, or NULL
*
* ERRNO: N/A
*/

FUNCPTR vxbAccessMethodGet
    (
    struct vxbDev * pDev,    /* Device Information */
    VXB_METHOD_ID   accessType   /* Access method to find */
    )
    {
    FUNCPTR     func = NULL;

    VXB_ASSERT(pDev->pAccess!=NULL, NULL)

    if ( accessType < FIRST_CUSTOM_BUS_ACCESS_METHOD )
    {
    switch (accessType)
        {
        case VXB_ACCESS_PROBE:
            func = pDev->pAccess->vxbRegisterProbe;
        break;
        case VXB_ACCESS_REG_READ8:
            func = pDev->pAccess->registerRead8;
        break;
        case VXB_ACCESS_REG_READ16:
            func = pDev->pAccess->registerRead16;
        break;
        case VXB_ACCESS_REG_READ32:
            func = pDev->pAccess->registerRead32;
        break;
        case VXB_ACCESS_REG_READ64:
            func = pDev->pAccess->registerRead64;
        break;
        case VXB_ACCESS_REG_WRITE8:
            func = pDev->pAccess->registerWrite8;
        break;
        case VXB_ACCESS_REG_WRITE16:
            func = pDev->pAccess->registerWrite16;
        break;
        case VXB_ACCESS_REG_WRITE32:
            func = pDev->pAccess->registerWrite32;
        break;
        case VXB_ACCESS_REG_WRITE64:
            func = pDev->pAccess->registerWrite64;
        break;
        case VXB_ACCESS_VOLREG_WRITE:
            func = pDev->pAccess->volRegWrite;
        break;
        case VXB_ACCESS_VOLREG_READ:
            func = pDev->pAccess->volRegRead;
        break;
        case VXB_ACCESS_INT_CONNECT:
        case VXB_ACCESS_INT_DISCONN:
        case VXB_ACCESS_INT_VEC_GET:
        case VXB_ACCESS_INT_ENABLE:
        case VXB_ACCESS_INT_DISABLE:
        case VXB_ACCESS_INT_ACKNOWLEDGE:
            func = pDev->pAccess->vxbDevControl;
        break;

        case VXB_ACCESS_CFG_READ:
            func = pDev->pAccess->busCfgRead;
        break;
        case VXB_ACCESS_CFG_WRITE:
            func = pDev->pAccess->busCfgWrite;
        break;
        default:
            return(NULL);
        }

    if ( func != NULL )
        return(func);
    }

    return NULL;

    }

/*********************************************************************
*
* writeConvertData - convert data to required byte-order
*
* This routine converts the byte order of the specified buffer from
* the byte-order specified in the flags variable (which contains the
* current byte-order of the data) to the order
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void writeConvertData
    (
    struct vxbDev * pDev,
    void *      pDataBuf,
    UINT32      transactionSize,
    UINT32 *        pFlags
    )
    {
    }

/*********************************************************************
*
* vxbVolRegWrite - volatile register writes
*
* This routine performs volatile register writes of the size
* specified in transactionSize.
*
* RETURNS: OK, or ERROR
*
* ERRNO: N/A
*/

STATUS vxbVolRegWrite
    (
    struct vxbDev * pDev,            /* Device Information */
    UINT32          regBaseIndex,    /* which pRegBase to use */
    UINT32          byteOffset,      /* offset, in bytes, of register */
    UINT32          transactionSize, /* transaction size, in bytes */
    void *          pDataBuf,        /* buffer to read-from/write-to */
    UINT32 *        pFlags           /* flags */
    )
    {
    STATUS retVal;
    FUNCPTR readFunc;
    FUNCPTR writeFunc;

    switch ( transactionSize )
        {
        case 1:
            readFunc = pDev->pAccess->registerRead8;
            writeFunc = pDev->pAccess->registerWrite8;
            break;
        case 2:
            readFunc = pDev->pAccess->registerRead16;
            writeFunc = pDev->pAccess->registerWrite16;
            break;
        case 4:
            readFunc = pDev->pAccess->registerRead32;
            writeFunc = pDev->pAccess->registerWrite32;
            break;
        case 8:
            readFunc = pDev->pAccess->registerRead64;
            writeFunc = pDev->pAccess->registerWrite64;
            break;
        default:
            return(ERROR);
        }

    if ( ( *pFlags & VXB_ENDIAN_MASK ) != VXB_ENDIAN_FLAG(pDev) )
        writeConvertData(pDev, pDataBuf, transactionSize, pFlags);

    retVal = (*writeFunc)(regBaseIndex, byteOffset,
                          pDataBuf, pFlags);

    if ( retVal != OK )
        return(retVal);

    retVal = (*readFunc)(regBaseIndex, byteOffset,
                         pDataBuf, pFlags);

    if ( retVal != OK )
        return(retVal);

    if ( ( *pFlags & VXB_ENDIAN_MASK ) != VXB_ENDIAN_FLAG(pDev) )
        writeConvertData(pDev, pDataBuf, transactionSize, pFlags);

    return(retVal);
    }
#endif /* VXB_LEGACY_ACCESS */

/*********************************************************************
*
* vxbIntCtlrMatch - match a device with connected intCtlr
*
*  This routine finds the intCltr to which the specified device
*  is attached.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbIntCtlrMatch
    (
    struct vxbDev * pInst,
    void * pArg
    )
    {
    struct vxbintCtlrMgmt * pInfo = (struct vxbintCtlrMgmt *)pArg;
    FUNCPTR   func;
    STATUS    stat;
    struct plbIntrEntry * pIntrEntry;
    int			inputPin;
    struct pIntCtlrTable * pICTab;
    struct plbIntCtlrTable * pICTab2;
    int		x;
    int		y;
    struct plbIntCtlrEntry * pEnt;
    BOOL pICTabAllocFlag = FALSE;

    if ( pInfo->found )
        return(OK);

    func = vxbDevMethodGet(pInst, DEVMETHOD_CALL(vxbIntCtlrConnect));
    if ( func == NULL )
        return(OK);

    VXB_DEBUG_MSG(1, "vxbIntCtlrMatch(): intCtlr %p\n", pInst, 2, 3, 4, 5, 6);

    /* call method to determine whether valid connect and to actually connect */

    stat = (*func)(pInst, pInfo->pDev, pInfo->index, pInfo->pIsr, 
                   pInfo->pArg, &inputPin);
    if ( stat != OK )
        return (ERROR);

    VXB_DEBUG_MSG(10, "vxbIntCtlrMatch(): intCtlr %p inPin %d matches\n",
        pInst, inputPin, 3, 4, 5, 6);

    pInfo->found = TRUE;

    /*
     * Update the device with the information to point to the 
     * correct interrupt source of the interrupt table based on 
     * the index passed from caller 
     */

    pIntrEntry = pInfo->pDev->pIntrInfo;
    pICTab = pIntrEntry->pIntCtlrTable;

    if ( pICTab == NULL )
        {
        pICTab = (struct pIntCtlrTable *)
                 hwMemAlloc(sizeof(struct pIntCtlrTable));

        if ( pICTab != NULL )
            pICTabAllocFlag = TRUE;
        else
            return (ERROR);

        pIntrEntry->pIntCtlrTable = pICTab;
        }

    x = pInfo->index / VXB_INTR_ENTSZ;
    y = pInfo->index % VXB_INTR_ENTSZ;

    pICTab2 = pICTab->pList[x];
    if ( pICTab2 == NULL )
        {
        pICTab2 = (struct plbIntCtlrTable *)
                  hwMemAlloc(sizeof(struct plbIntCtlrTable));

        if ( pICTab2 == NULL )
            {
            if (pICTabAllocFlag == TRUE)
                {
#ifndef _VXBUS_BASIC_HWMEMLIB
                hwMemFree ((char *)pICTab);
#endif /* !_VXBUS_BASIC_HWMEMLIB */
                }

            return (ERROR);
            }

        pICTab->pList[x] = pICTab2;
        }

    /* 
     * Update entry in device interrupt output table to point to 
     * interrupt controller input pin.
     */

    pEnt = &(pICTab2->intCtlr[y]);
    pEnt->intCtlr = pInst;
    pEnt->intCtlrPin = inputPin;

    VXB_DEBUG_MSG(10, "vxbIntCtlrMatch(): saved record @ %p\n",
    		  pEnt, 2, 3, 4, 5, 6);

    return(OK);
    }

/*********************************************************************
*
* vxbDevControl - device control routine
*
* This routine finds the method to control devices from the parent bus
* and handles manipulation of downstream devices, such as interrupt
* management.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbDevControl
    (
    VXB_DEVICE_ID	pDev,
    pVXB_DEVCTL_HDR	pBusDevControl
    )
    {
    PLB_DEV_CTRL_FUNCPTR	func;
#ifdef  VXB_LEGACY_ACCESS

    VXB_ASSERT(pDev->pAccess!=NULL && pDev->pAccess->vxbDevControl!=NULL,
               ERROR)

    func = (PLB_DEV_CTRL_FUNCPTR) pDev->pAccess->vxbDevControl;
#else	/* VXB_LEGACY_ACCESS */
    VXB_DEVICE_ID		pParent;

    func = NULL;

    /* find busCtlrDevControl method set in the closest upstream parent bus */

    for (pParent = vxbDevParent (pDev);
	   pParent != NULL;
	   pParent = vxbDevParent (pParent))
	{
	func = vxbDevMethodGet (pParent, DEVMETHOD_CALL(busCtlrDevCtlr));

	if (func != NULL)
	    break;
	}

    /*
     * If no parent bus sets busCtlrDevControl method,
     * execute plbDevControl().
     */

    if (func == NULL)
	func = (PLB_DEV_CTRL_FUNCPTR) pPlbDevControl;
#endif  /* VXB_LEGACY_ACCESS */

    /* manipulate downstream devices */

    return ((*func) (pDev, pBusDevControl));
    }

/*********************************************************************
*
* vxbIntAllocMatch - match a device with vxbIntCtlrAlloc
*
* This routine finds the vxbIntCtlrAlloc to which the specified device
* is attached.
*
* RETURNS: ERROR/OK
*
* ERRNO: N/A
*/

LOCAL STATUS vxbIntAllocMatch
    (
    struct vxbDev * pInst,
    void * pArg
    )
    {
    struct vxbIntPropertyMgmt * pInfo = (struct vxbIntPropertyMgmt *)pArg;
    FUNCPTR   func;

    if ( pInfo->found )
        return ERROR;

    func = vxbDevMethodGet(pInst, DEVMETHOD_CALL(vxbIntCtlrAlloc));
    if (func == NULL)
        return ERROR;

    /* call actual allocate method */

    pInfo->retValue = (*func)(pInfo->pDev, pInfo->type, pInfo->count);

    if (pInfo->retValue > 0)
        pInfo->found = TRUE;
    return OK;
    }

/*********************************************************************
*
* vxbIntAlloc - allocate count of interrupt resource for device
*
* This routine allocate number of MSI interrupt resources for vxBus device.
* Interrupt controller implement the actual allocate function and fill device's
* interrupt information.
*
* RETURNS: OK/ERROR or actual allocated interrupt number.
*
* ERRNO: N/A
*/

int vxbIntAlloc
    (
    struct vxbDev * pDev,       /* Device Information */
    UINT32          type,       /* interrupt type VXB_INT_PCI_MSI/VXB_INT_PCI_MSIX*/
    UINT32          count       /* number of interrupt to request */
    )
    {
    struct vxbIntPropertyMgmt intProperty;
    VXB_INTR_ENTRY * pIntInfo;
    VXB_INTR_ENTRY   saveIntInfo;
    void           * tempPar;
    int              i;
    STATUS           (*pMethod) (VXB_DEVICE_ID, UINT32, UINT32);

    VXB_ASSERT(pDev != NULL, ERROR)

    pMethod = vxbDevMethodGet (pDev,
        DEVMETHOD_CALL(vxbDevIntCapabCheck));

    if(pMethod == NULL)
        pMethod = vxbDevMethodGet (vxbDevParent (pDev),
            DEVMETHOD_CALL(vxbDevIntCapabCheck));

    if (pMethod != NULL)
        {
        if (pMethod (pDev, VXB_INT_TYPE(type), count) != OK)
            {
            VXB_DEBUG_MSG(1, "vxbDevIntCapabCheck ERROR %s%d\n",
                pDev->pName,pDev->unitNumber,3,4,5,6);
            return ERROR;
            }
        }

    pIntInfo = (VXB_INTR_ENTRY *)pDev->pIntrInfo;

    /* save previous interrupt information */

    bcopy ((char *) pIntInfo, (char *)&saveIntInfo, sizeof (VXB_INTR_ENTRY));

    tempPar = hwMemAlloc (sizeof(VOIDFUNCPTR) * count +
        sizeof(VXB_INTR_ENTRY));

    if ( tempPar == NULL)
        return ERROR;

    pIntInfo->intVecList = tempPar;
    pIntInfo->intrFlag = type;

    for (i = 0; i < count; i ++)
        pIntInfo->intVecList[i] = (void *)INVALD_INDEX;

    intProperty.found = FALSE;
    intProperty.pDev  = pDev;
    intProperty.type  = type;
    intProperty.count = count;

    /* find a match and call the allocate routine */

    vxbDevIterate (vxbIntAllocMatch, (void *)&intProperty, VXB_ITERATE_INSTANCES);

    if (intProperty.retValue <= 0)
        {
        bcopy ((char *)&saveIntInfo, (char *) pIntInfo, sizeof (VXB_INTR_ENTRY));
        return ERROR;
        }

    pIntInfo->numVectors = intProperty.retValue;

    bcopy ((char *) &saveIntInfo, (char *)((ULONG)pIntInfo->intVecList +
        sizeof(VOIDFUNCPTR)* pIntInfo->numVectors), sizeof (VXB_INTR_ENTRY));

    return intProperty.retValue;
    }

/*********************************************************************
*
* vxbIntFreeMatch - match a device with vxbIntCtlrFree
*
* This routine finds the vxbIntCtlrFree to which the specified device
* is attached.
*
* RETURNS: OK/ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS vxbIntFreeMatch
    (
    struct vxbDev * pInst,
    void * pArg
    )
    {
    struct vxbIntPropertyMgmt * pInfo = (struct vxbIntPropertyMgmt *)pArg;
    FUNCPTR   func;

    if (pInfo->found)
        return ERROR;

    func = vxbDevMethodGet(pInst, DEVMETHOD_CALL(vxbIntCtlrFree));
    if (func == NULL)
        return ERROR;

    /* call actual allocate method */

    (*func)(pInfo->pDev);

    return OK;
    }

/*********************************************************************
*
* vxbIntFree - free device's interrupt resource
*
* This routine free device's all allocated interrupt, recover the previous
* configured interrupt information.
*
* RETURNS:  N/A
*
* ERRNO: N/A
*/

void vxbIntFree
    (
    struct vxbDev * pDev
    )
    {
    struct vxbIntPropertyMgmt intProperty;
    VXB_INTR_ENTRY      *     pIntInfo;
    VXB_INTR_ENTRY            saveIntInfo;
    UINT32                    i;
    FUNCPTR                   pMethod;

    VXB_ASSERT(pDev != NULL, ERROR)

    intProperty.found = FALSE;
    intProperty.pDev  = pDev;

    pIntInfo = (VXB_INTR_ENTRY *)pDev->pIntrInfo;
    if (pIntInfo->intrFlag == 0)
        return ;

    bcopy ((char *)((ULONG)pIntInfo->intVecList +
        sizeof(VOIDFUNCPTR)* pIntInfo->numVectors),
        (char *) &saveIntInfo, sizeof (VXB_INTR_ENTRY));

    /* find a match and call the free routine */

    vxbDevIterate (vxbIntFreeMatch, (void *)&intProperty, VXB_ITERATE_INSTANCES);

#ifndef _VXBUS_BASIC_HWMEMLIB
    hwMemFree ((char *)pIntInfo->intVecList);
#endif /* _VXBUS_BASIC_HWMEMLIB */

    /* recover interrupt information */

    bcopy ((char *) &saveIntInfo, (char *)pIntInfo, sizeof (VXB_INTR_ENTRY));
    }

/*********************************************************************
*
* vxbIntConnect - connect device's ISR
*
* This routine connects the given interrupt service routine to
* the interrupt signal for the specified device.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS vxbIntConnect
    (
    struct vxbDev * pDev,       /* Device Information */
    int             index,      /* index of interrupt vector */
    VOIDFUNCPTR     pIsr,       /* ISR */
    void *          pArg        /* parameter */
    )
    {
    VXB_ACCESS_INTERRUPT_INFO   accessIntrInfo;
    struct vxbintCtlrMgmt info;

    VXB_ASSERT(pDev!=NULL, ERROR)

    VXB_DEBUG_MSG(1, "vxbIntConnect(%p,%d,%p,%p)\n",
                  pDev, index, pIsr, pArg, 5, 6);

    info.found = FALSE;
    info.pIntCtlr = NULL;
    info.inputPin = 0;
    info.pDev = pDev;
    info.index = index;
    info.pIsr = pIsr;
    info.pArg = pArg;

    /* Find a match and call the int connect routine */

    vxbDevIterate(vxbIntCtlrMatch, &info, VXB_ITERATE_INSTANCES);

    if ( info.found == TRUE )
        return(OK);

    /*
     * If no match drop through to device control function for specific
     * int connect info.
     */

    accessIntrInfo.header.vxbAccessId = VXB_ACCESS_INT_CONNECT;
    accessIntrInfo.intIndex = index;
    accessIntrInfo.pISR =  pIsr;
    accessIntrInfo.pParameter = pArg;

    return (vxbDevControl (pDev, (pVXB_DEVCTL_HDR)&accessIntrInfo));
    }

/*********************************************************************
*
* vxbIntDisconnect - disconnect device's interrupt
*
* This routine disconnects the specified ISR from the interrupt source.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS vxbIntDisconnect
    (
    struct vxbDev * pDev,       /* Device Information */
    int         index,          /* index of interrupt vector */
    VOIDFUNCPTR     pIsr,       /* ISR */
    void *      pArg            /* parameter */
    )
    {
    VXB_ACCESS_INTERRUPT_INFO   accessIntrInfo;
    struct plbIntrEntry * pIntrEntry;
    VXB_DEVICE_ID       pIntCtlr;
    FUNCPTR   func;
    BOOL      stat;
    int				x;
    int				y;

    VXB_ASSERT(pDev!=NULL, ERROR)

    pIntrEntry = pDev->pIntrInfo;
    if ( pIntrEntry == NULL )
        goto skipIntCtlr;

    x = index / VXB_INTR_ENTSZ;
    y = index % VXB_INTR_ENTSZ;

    if ( pIntrEntry->pIntCtlrTable == NULL )
        goto skipIntCtlr;
    if ( pIntrEntry->pIntCtlrTable->pList[x] == NULL )
        goto skipIntCtlr;
    if ( pIntrEntry->pIntCtlrTable->pList[x]->intCtlr[y].intCtlr == NULL )
        goto skipIntCtlr;

    pIntCtlr = pIntrEntry->pIntCtlrTable->pList[x]->intCtlr[y].intCtlr;

    func = vxbDevMethodGet(pIntCtlr, DEVMETHOD_CALL(vxbIntCtlrDisconnect));
    if ( func != NULL )
        {

        /* use interrupt controller mechanism */

        stat = (*func)(pIntCtlr, pDev, index, pIsr, pArg);
        if ( stat != OK )
          return(ERROR);

        return(OK);
        }

skipIntCtlr:

    accessIntrInfo.header.vxbAccessId = VXB_ACCESS_INT_DISCONN;
    accessIntrInfo.intIndex = index;
    accessIntrInfo.pISR =  pIsr;
    accessIntrInfo.pParameter = pArg;

    return (vxbDevControl (pDev, (pVXB_DEVCTL_HDR)&accessIntrInfo));
    }

/*********************************************************************
*
* vxbIntAcknowledge - acknowledge device's interrupt
*
* This routine acknowledges and clears the specified interrupt on
* any interrupt controller intervening between the processor and
* the device.  It does not affect the interrupt source or the
* processor.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS vxbIntAcknowledge
    (
    struct vxbDev * pDev,       /* Device Information */
    int             index,      /* index of interrupt vector */
    VOIDFUNCPTR     pIsr,       /* ISR */
    void *          pArg        /* parameter */
    )
    {
    VXB_ACCESS_INTERRUPT_INFO   accessIntrInfo;
    struct plbIntrEntry * pIntrEntry;
    VXB_DEVICE_ID       pIntCtlr;
    FUNCPTR   func;
    BOOL      stat;
    int				x;
    int				y;

    VXB_ASSERT(pDev!=NULL, ERROR)

    pIntrEntry = pDev->pIntrInfo;
    if ( pIntrEntry == NULL )
        goto skipIntCtlr;

    x = index / VXB_INTR_ENTSZ;
    y = index % VXB_INTR_ENTSZ;

    if ( pIntrEntry->pIntCtlrTable == NULL )
        goto skipIntCtlr;
    if ( pIntrEntry->pIntCtlrTable->pList[x] == NULL )
        goto skipIntCtlr;
    if ( pIntrEntry->pIntCtlrTable->pList[x]->intCtlr[y].intCtlr == NULL )
        goto skipIntCtlr;

    pIntCtlr = pIntrEntry->pIntCtlrTable->pList[x]->intCtlr[y].intCtlr;

    func = vxbDevMethodGet(pIntCtlr, DEVMETHOD_CALL(vxbIntCtlrAck));
    if ( func != NULL )
        {

        /* use interrupt controller mechanism */

        stat = (*func)(pIntCtlr, pDev, index, pIsr, pArg);
        if ( stat != OK )
          return(ERROR);

        return(OK);
        }

skipIntCtlr:

    accessIntrInfo.header.vxbAccessId = VXB_ACCESS_INT_ACKNOWLEDGE;
    accessIntrInfo.intIndex = index;
    accessIntrInfo.pISR =  pIsr;
    accessIntrInfo.pParameter = pArg;

    return (vxbDevControl (pDev, (pVXB_DEVCTL_HDR)&accessIntrInfo));
    }

/*********************************************************************
*
* vxbIntEnable - enable device's interrupt
*
* This routine enables the specified interrupt on any interrupt
* controller intervening between the processor and the device.
* It affects neither the interrupt source nor the processor.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS vxbIntEnable
    (
    struct vxbDev *	pDev,  /* Device Information */
    int             index, /* index of interrupt vector */
    VOIDFUNCPTR     pIsr,  /* ISR */
    void *          pArg   /* parameter */
    )
    {
    VXB_ACCESS_INTERRUPT_INFO   accessIntrInfo;
    struct plbIntrEntry *	pIntrEntry;
    VXB_DEVICE_ID       	pIntCtlr;
    FUNCPTR   			func;
    BOOL      			stat;
    int				x;
    int				y;

    VXB_ASSERT(pDev!=NULL, ERROR)

    pIntrEntry = pDev->pIntrInfo;
    if ( pIntrEntry == NULL )
        goto skipIntCtlr;

    x = index / VXB_INTR_ENTSZ;
    y = index % VXB_INTR_ENTSZ;

    if ( pIntrEntry->pIntCtlrTable == NULL )
        goto skipIntCtlr;
    if ( pIntrEntry->pIntCtlrTable->pList[x] == NULL )
        goto skipIntCtlr;
    if ( pIntrEntry->pIntCtlrTable->pList[x]->intCtlr[y].intCtlr == NULL )
        goto skipIntCtlr;

    pIntCtlr = pIntrEntry->pIntCtlrTable->pList[x]->intCtlr[y].intCtlr;

    func = vxbDevMethodGet(pIntCtlr, DEVMETHOD_CALL(vxbIntCtlrEnable));
    if ( func != NULL )
        {

        /* use interrupt controller mechanism */

        stat = (*func)(pIntCtlr, pDev, index, pIsr, pArg);
        
        if (stat == OK)
            return OK;
        }

skipIntCtlr:

    /*
     * default to mechanism provided by bus controller
     * through access functions
     */

    accessIntrInfo.header.vxbAccessId = VXB_ACCESS_INT_ENABLE;
    accessIntrInfo.intIndex = index;
    accessIntrInfo.pISR =  pIsr;
    accessIntrInfo.pParameter = pArg;

    return (vxbDevControl (pDev, (pVXB_DEVCTL_HDR)&accessIntrInfo));
    }

/*********************************************************************
*
* vxbIntDisable - disable device's interrupt
*
* This routine disables the specified interrupt on the lowest-level
* interrupt controller between the processor and the device.  It
* does not affect the interrupt source nor the processor.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS vxbIntDisable
    (
    struct vxbDev * pDev,      /* Device Information */
    int            index,      /* index of interrupt vector */
    VOIDFUNCPTR     pIsr,      /* ISR */
    void *          pArg       /* parameter */
    )
    {
    VXB_ACCESS_INTERRUPT_INFO   accessIntrInfo;
    struct plbIntrEntry * pIntrEntry;
    VXB_DEVICE_ID       pIntCtlr;
    FUNCPTR   func;
    BOOL      stat;
    int				x;
    int				y;

    VXB_ASSERT(pDev!=NULL, ERROR)

    pIntrEntry = pDev->pIntrInfo;
    if ( pIntrEntry == NULL )
        goto skipIntCtlr;

    x = index / VXB_INTR_ENTSZ;
    y = index % VXB_INTR_ENTSZ;

    if ( pIntrEntry->pIntCtlrTable == NULL )
        goto skipIntCtlr;
    if ( pIntrEntry->pIntCtlrTable->pList[x] == NULL )
        goto skipIntCtlr;
    if ( pIntrEntry->pIntCtlrTable->pList[x]->intCtlr[y].intCtlr == NULL )
        goto skipIntCtlr;

    pIntCtlr = pIntrEntry->pIntCtlrTable->pList[x]->intCtlr[y].intCtlr;

    func = vxbDevMethodGet(pIntCtlr, DEVMETHOD_CALL(vxbIntCtlrDisable));
    if ( func != NULL )
        {

        /* use interrupt controller mechanism */

        stat = (*func)(pIntCtlr, pDev, index, pIsr, pArg);
        if ( stat != OK )
          return(ERROR);

        return(OK);
        }

skipIntCtlr:

    accessIntrInfo.header.vxbAccessId = VXB_ACCESS_INT_DISABLE;
    accessIntrInfo.intIndex = index;
    accessIntrInfo.pISR =  pIsr;
    accessIntrInfo.pParameter = pArg;

    return (vxbDevControl (pDev, (pVXB_DEVCTL_HDR)&accessIntrInfo));
    }

/*********************************************************************
*
* vxbIntVectorGet - get device's interrupt vector
*
* This routine returns the vector for the specified interrupt on
* the specified device.  The return value of this routine can be
* passed as the vector to intConnect() and other system-level
* interrupt support routines.
*
* RETURNS: vector, or NULL
*
* ERRNO: N/A
*/

VOIDFUNCPTR * vxbIntVectorGet
    (
    struct vxbDev * pDev,       /* Device Information */
    int             index       /* index of interrupt vector */
    )
    {
    VXB_ACCESS_INT_VECTOR_GET accessVectorGet;
    struct plbIntrEntry *     pIntrEntry;
    VXB_DEVICE_ID             pIntCtlr;
    FUNCPTR                   func;
    int                       x;
    int                       y;

    VXB_ASSERT(pDev!=NULL, NULL)

    pIntrEntry = pDev->pIntrInfo;
    if ( pIntrEntry == NULL )
        return NULL;

    x = index / VXB_INTR_ENTSZ;
    y = index % VXB_INTR_ENTSZ;

    if ( pIntrEntry->pIntCtlrTable == NULL )
        goto skipIntCtlr;
    if ( pIntrEntry->pIntCtlrTable->pList[x] == NULL )
        goto skipIntCtlr;
    if ( pIntrEntry->pIntCtlrTable->pList[x]->intCtlr[y].intCtlr == NULL )
        goto skipIntCtlr;

    pIntCtlr = pIntrEntry->pIntCtlrTable->pList[x]->intCtlr[y].intCtlr;

    func = vxbDevMethodGet(pIntCtlr, DEVMETHOD_CALL(vxbIntCtlrDisconnect));
    if ( func == NULL )
        goto skipIntCtlr;

    /*
     * If this device is connected through VxBus interrupt
     * controller, then the entire concept of a vector
     * is undefined.  Just return NULL.
     */

    return(NULL);

skipIntCtlr:

    accessVectorGet.header.vxbAccessId = VXB_ACCESS_INT_VEC_GET;
    accessVectorGet.intIndex = index;
    accessVectorGet.pIntVector = NULL;

    vxbDevControl (pDev, (pVXB_DEVCTL_HDR)&accessVectorGet);

    return (accessVectorGet.pIntVector);
    }

#ifdef _WRS_CONFIG_SMP /* interrupt routing is SMP only */

/*********************************************************************
*
* vxbIntToCpuRoute - reroute all interrupts for a specified CPU
*
* This routine calls all interrupt controllers that support interrupt
* routing functions (method vxbIntCpuReroute declared by the controller)
* passing this function the specified CPU for the reroute operation.
*
* RETURNS: OK always
*
* ERRNO: N/A
*/

STATUS vxbIntToCpuRoute
    (
    unsigned int destCpu /* logical index of destination CPU for reroute */
    )
    {
    vxbDevMethodRun (DEVMETHOD_CALL(vxbIntCtlrCpuReroute),
        (void *)((ULONG)destCpu));

    return (OK);
    }

/*******************************************************************
*
* vxbIntRerouteSet - forward interrupt cpu/route to int ctlr
*
* This routine forwards the route request to the interrupt 
* controllers which publish a {vxbIntCtlrIntReroute}() method
*
* RETURNS: OK always
*
* ERRNO: N/A
*/

LOCAL STATUS vxbIntRerouteSet
    (
    VXB_DEVICE_ID pDev,
    void * pArg
    )
    {
    struct vxbIntrRerouteInfo *routeInfo;
    FUNCPTR pMethod;

    routeInfo = (struct vxbIntrRerouteInfo *)pArg;
    pMethod = vxbDevMethodGet(pDev, DEVMETHOD_CALL(vxbIntCtlrIntReroute));

    if (pMethod != NULL)
        (*pMethod)(routeInfo->pInst, routeInfo->interruptIndex,
                   routeInfo->destCpu);

    return (OK);
    }

/*******************************************************************
*
* vxbIntReroute - route specified interrupt to destination CPU
*
* This routine routes the specified interrupt to the specified CPU.
*
* RETURNS: OK always
*
* ERRNO: N/A
*/

STATUS vxbIntReroute
    (
    VXB_DEVICE_ID pRequestor,
    int           interruptIndex,
    cpuset_t      destCpu
    )
    {
    struct vxbIntrRerouteInfo routeInfo;

    routeInfo.pInst = pRequestor;
    routeInfo.interruptIndex = interruptIndex;
    routeInfo.destCpu = destCpu;
    vxbDevIterate ((FUNCPTR)vxbIntRerouteSet, &routeInfo,
                   VXB_ITERATE_INSTANCES);

    return (OK);
    }

#endif /* _WRS_CONFIG_SMP */

/*********************************************************************
*
* vxbDevStructAlloc - allocate VXB_DEVICE structure
*
* This routine allocates the VXB_DEVICE structure.
*
* RETURNS: pointer to structure, or NULL
*
* ERRNO: N/A
*/

struct vxbDev * vxbDevStructAlloc
    (
    int flags        /* Flags */
    )
    {
    struct vxbDev * pRet;
    UINT32      savedVersion;

    /* acquire the lock */

    vxbLockTake(&vxbDevStructListLock, VXB_LOCK_WRITER);

    /* reuse available storage, if possible */

    if ( pBusDevFreeList != NULL )
        {

        /* fetch the first pointer */

        pRet = pBusDevFreeList;

        /* increment the head of the list */

        pBusDevFreeList = pRet->pNext;

        /* release the lock */

        vxbLockGive(&vxbDevStructListLock, VXB_LOCK_WRITER);

        /* save the version counter */

        savedVersion = pRet->versionCounter;

        /* clear out the vxbDev structure */

        bzero((void *)pRet, sizeof(struct vxbDev));

        /* initialize the lock */

        if ( vxbInitPhase > 1 )
            vxbLockInit(&pRet->devLock);

        /* restore the versionCounter */

        pRet->versionCounter = savedVersion;

        return(pRet);
        }

    /* release the lock */

    vxbLockGive(&vxbDevStructListLock, VXB_LOCK_WRITER);

    /* allocate new storage, if required */

    pRet = (struct vxbDev *)hwMemAlloc(sizeof(struct vxbDev));

#ifdef HWMEM_ALLOC_FAIL_DEBUG
    if ( pRet == NULL )
        {

        /*
         * If the caller allows us to wait forever for space,
         * that means that we can replenish the hwMemPool
         * from the system memory pool, if we're in an appropriate
         * stage of initialization.  We do so in increments
         * of 16KBytes.
         */

        if ( ( flags == WAIT_FOREVER ) && ( vxbInitPhase >= 2 ) )
            {
            char * pMem;

            /*
             * allocate 16 KBytes from system pool and add it
             * to hwMem pool
             */

            pMem = (char *)KMEM_ALLOC(16 * 1024);
            hwMemPoolCreate(pMem, 16 * 1024);

            /* try to allocate the structure again */

            pRet = (struct vxbDev *)hwMemAlloc(sizeof(struct vxbDev));
            }
        }
#endif	/* HWMEM_ALLOC_FAIL_DEBUG */

    /* if we got a structure, zero it, and init versionCounter */

    if ( pRet != NULL )
        {

        /* initialize the lock */

        if ( vxbInitPhase > 1 )
            vxbLockInit(&pRet->devLock);

        /*
         * versionCounter field should be initialized by 
         * hwMemAlloc() pRet->versionCounter = 0;
         */

        }

    return(pRet);
    }

#ifndef	_WRS_CONFIG_VXBUS_BASIC

/*********************************************************************
*
* vxbDevStructFree - free VXB_DEVICE structure
*
* This routine frees the previously allocated VXB_DEVICE structure.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void vxbDevStructFree
    (
    struct vxbDev * pDev    /* Device Information */
    )
    {

    /* delete the lock for the device information */

    if (vxbLockDelete (&pDev->devLock) == OK)
	{

	/*
	 * increment version counter on free,
	 * and save the structure to the free list
	 *
	 * XXX: What is the version counter used for?
	 */

	pDev->versionCounter++;

	/* acquire the lock for the Device Structure List */

	vxbLockTake (&vxbDevStructListLock, VXB_LOCK_WRITER);

	pDev->pNext = pBusDevFreeList;
	pBusDevFreeList = pDev;

	/* release the lock for the Device Structure List */

	vxbLockGive (&vxbDevStructListLock, VXB_LOCK_WRITER);
	}
    }
#endif	/* _WRS_CONFIG_VXBUS_BASIC */

/*********************************************************************
*
* vxbInstUnitSet - set the unit number
*
* This routine sets the unit number for the device.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS	vxbInstUnitSet
    (
    VXB_DEVICE_ID	pDev,
    UINT32		unitNumber
    )
    {
    VXB_ASSERT(pDev!=NULL, ERROR)

    /* set the unit number */

    pDev->unitNumber = unitNumber;

    return OK;
    }

/*********************************************************************
*
* vxbInstUnitGet - get the unit number
*
* This routine gets the unit number for the device.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS	vxbInstUnitGet
    (
    VXB_DEVICE_ID	pDev,
    UINT32 *		pUnitNumber
    )
    {
    VXB_ASSERT(pDev!=NULL, ERROR)

    /* set the unit number */

    *pUnitNumber = pDev->unitNumber;

    return OK;
    }

/*********************************************************************
*
* vxbUnitCheck - test is a device unit number is in use
*
* This is a helper routine for vxbNextUnitGet() below. It tests
* to see if a device is associated with a given driver, and if
* so, it checks its unit number.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void vxbUnitCheck
    (
    VXB_DEVICE_ID pDev,
    void * pArg
    )
    {
    VXB_UNIT * pUnit;

    VXB_ASSERT_V(pDev!=NULL)
    VXB_ASSERT_V(pDev->pDriver!=NULL)

    pUnit = pArg;

    if (
        (pUnit->vxbUnitTaken) ||
    	(strcmp(pDev->pDriver->drvName, pUnit->vxbDev->pDriver->drvName) != 0)
       )
        return;

    if (pDev->unitNumber == pUnit->vxbCheckUnit)
        pUnit->vxbUnitTaken = TRUE;

    return;
    }

/*********************************************************************
*
* vxbNextUnitGet - get the next available unit number for a driver
*
* This routine searches the instances associated with a given driver
* and finds the next available free unit number for a new device.
* The search starts at unit zero and works up until an unused unit
* number is found.
*
* This is a convenience function for drivers where simply using a
* monotonically increasing unit number would be unsuitable, such as
* for plugable devices where instances are constantly being created
* and destroyed.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*/

STATUS vxbNextUnitGet
    (
    VXB_DEVICE_ID pDev
    )
    {
    UINT i;
    VXB_UNIT u;

    i = 0;

    /*
     * Temporarily set the unit number to a bogus value
     * instead of the default of 0, so we don't accidentally
     * match this device as unit 0.
     */

    vxbInstUnitSet (pDev, 0xFFFFFFFF);
    u.vxbDev = pDev;

    FOREVER
        {
        u.vxbCheckUnit = i;
        u.vxbUnitTaken = FALSE;
        vxbDevIterate ((FUNCPTR)vxbUnitCheck, &u, VXB_ITERATE_INSTANCES);
        if (u.vxbUnitTaken == FALSE)
            break;
        i++;
        }

    vxbInstUnitSet (pDev, i);

    return (OK);
    }

/*********************************************************************
*
* vxbSpinLockStub - stub for spinlock functions
*
* Spinlocks cannot be used before the 2nd stage of OS initialization.
* This function does nothing and is used in place of spinLockIsrTake
* and spinLockIsrGive before the 2nd stage of OS initialization.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxbSpinLockStub
    (
    spinlockIsr_t * lock
    )
    {
    /* do nothing here */
    }

#ifdef SPIN_LOCK_TRACE

/*********************************************************************
*
* vxbSpinLockTakeStub - stub for spinlock functions
*
* Spinlocks cannot be used before the 2nd stage of OS initialization.
* This function does nothing and is used in place of spinLockIsrTake
* and spinLockIsrGive before the 2nd stage of OS initialization.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxbSpinLockTakeStub
    (
    spinlockIsr_t * lock,
    char * file,
    int line
    )
    {
    /* do nothing here */
    }
#endif	/* SPIN_LOCK_TRACE */

/*********************************************************************
*
* vxbLockTake - take a VxBus lock
*
* This routine takes a VxBus lock depending on if VXBUS_BASIC_RWSEM 
* is defined.
* 
* 1) If VXBUS_BASIC_RWSEM is defined, it locks with a counter and 
*    disables interrupts of the local CPU. 
*
* 2) If VXBUS_BASIC_RWSEM is not defined, the behaviour depends on 
*    the current VxBus initialization phase:
* 
* 2.1) Before VxBus second phase, it locks with readerCount;
*
* 2.2) After VxBus second phase, it will initialize the lock as a 
*      semaphore if not already, and then take the semaphore.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* SEE ALSO: vxbLockInit(), vxbLockTake(), vxbLockDelete()
*/

STATUS vxbLockTake
    (
    VXB_LOCK	lockId,
    BOOL	reader
    )
    {
#ifdef _VXBUS_BASIC_RWSEM
    if (lockId->counter == 0)
        {
        lockId->counter = 1;
        lockId->key = intCpuLock();
        }
    return (OK);
#else /* _VXBUS_BASIC_RWSEM */
    STATUS	retVal;

    if ( ( lockId->initDone != TRUE ) && ( vxbInitPhase >= 2 ) )
        vxbLockInit(lockId);

    if ( lockId->initDone != TRUE )
        {
	if ( reader )
	    {
	    if ( lockId->readerCount == 0 )
		{
		lockId->readerCount = 1;
		return(OK);
		}
	    }
	else
	    {
	    if ( lockId->readerCount == 0 )
		{
		lockId->readerCount = -1;
		return(OK);
		}
	    }
	return(ERROR);
	}

    if ( reader )
        {
	retVal = semRTake(lockId->semID, WAIT_FOREVER);
	}
    else
        {
	retVal = semWTake(lockId->semID, WAIT_FOREVER);
	}

    return(retVal);
#endif /* _VXBUS_BASIC_RWSEM */    
    }

/*********************************************************************
*
* vxbLockGive - release a VxBus lock
*
* This routine releases a VxBus lock depending on if VXBUS_BASIC_RWSEM
* is defined.
* 
* 1) If VXBUS_BASIC_RWSEM is defined, it unlocks with a counter and 
*    restores interrupts of the local CPU. 
*
* 2) If VXBUS_BASIC_RWSEM is not defined, the behaviour depends on 
*    the current VxBus initialization phase:
* 
* 2.1) Before VxBus second phase, it unlocks with readerCount.
*
* 2.2) After VxBus second phase, it will initialize the lock as a 
*      semaphore if not already, and then give the semaphore.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* SEE ALSO: vxbLockInit(), vxbLockTake(), vxbLockDelete()
*/

STATUS vxbLockGive
    (
    VXB_LOCK	lockId,
    BOOL	reader
    )
    {
#ifdef _VXBUS_BASIC_RWSEM
    if (lockId->counter == 1)
        {
        lockId->counter = 0;            
        intCpuUnlock (lockId->key);
        }
    return (OK);
#else  /* _VXBUS_BASIC_RWSEM */      
    STATUS retVal;

    if ( lockId->initDone != TRUE )
        {
	if ( reader )
	    {
	    if ( lockId->readerCount > 0 )
		{
		lockId->readerCount--;

		if ( (lockId->readerCount == 0) && (vxbInitPhase >= 2) )
		    vxbLockInit(lockId);

		return(OK);
		}
	    }
	else
	    {
	    if ( lockId->readerCount == -1 )
		{
		lockId->readerCount = 0;

		if ( vxbInitPhase >= 2 )
		    vxbLockInit(lockId);

		return(OK);
		}
	    }
	return(ERROR);
	}

    retVal = semGive(lockId->semID);

    return(retVal);
#endif /* _VXBUS_BASIC_RWSEM */    
    }

/*********************************************************************
*
* vxbLockInit - initialize a VxBus lock
*
* This routine initializes a VxBus lock depending on if 
* VXBUS_BASIC_RWSEM is defined.
*
* 1) If VXBUS_BASIC_RWSEM is defined, return OK directly. 
*
* 2) If VXBUS_BASIC_RWSEM is not defined, and the readerCount has not
*     been taken by vxbLockTake, then create it as a semaphore.
*
* NOTE:
*
* The content of <lockId> should be zero'ed before it is used the 
* first time.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* SEE ALSO: vxbLockTake(), vxbLockGive(), vxbLockDelete()
*/

STATUS vxbLockInit
    (
    VXB_LOCK	lockId
    )
    {
#ifdef _VXBUS_BASIC_RWSEM        
    return (OK);
#else /* _VXBUS_BASIC_RWSEM */

    if (vxbInitPhase < 2 ||
        lockId->readerCount != 0 )
        return(ERROR);

    /* check if init done or in-progress */
    if (vxAtomic32Set(&(lockId->initDone), TRUE) == TRUE)
	{
	while (*(volatile SEM_ID *) &(lockId->semID) == NULL)
	    /* wait: do nothing */;

	return(OK);
	}

    lockId->semID = semRWCreate(SEM_Q_FIFO, 256);

    if ( lockId->semID == NULL )
        {
	return(ERROR);
	}
    return(OK);
#endif /* _VXBUS_BASIC_RWSEM */      
    }

/*********************************************************************
*
* vxbLockDelete - delete a VxBus lock
*
* This routine deletes a VxBus lock depending on if VXBUS_BASIC_RWSEM
* is defined.
* 
* 1) If VXBUS_BASIC_RWSEM is defined, return OK directly. 
*
* 2) If VXBUS_BASIC_RWSEM is not defined, and the semaphore has been 
*    created, delete it.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* SEE ALSO:  vxbLockInit() vxbLockTake() vxbLockGive()
*/

STATUS vxbLockDelete
    (
    VXB_LOCK	lockId
    )
    {
#ifdef _VXBUS_BASIC_RWSEM           
    return (OK);
#else /* _VXBUS_BASIC_RWSEM */
    if (lockId->readerCount != 0)
	return (ERROR);

    /* check if init is done */

    if (lockId->initDone != TRUE)
	return (OK);

    return (semDelete (lockId->semID));
#endif /* _VXBUS_BASIC_RWSEM */
    }
