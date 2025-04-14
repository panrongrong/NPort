/* usbSynopsysHcdInitExit.c - Synopsys HCD initialization routine */

/*
 * Copyright (c) 2009-2011, 2013, 2014 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify, or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01v,03dec14,wyy  remove compiler warnings
01u,03sep13,wyy  remove compiler warnings
01t,09aug13,ljg  adjust pHCDData->RequestSynchEventID (WIND00429223)
01s,02aug13,wyy  Remove compiler warnings
01r,25jul13,ljg  add dmaChannelMutex to protect uIdleDmaChannelMap (WIND00427761)
01q,10jul13,ljg  add alt_soc_gen5 support
01p,10Jul13,wyy  Make usbd layer to uniformly handle ClearTTBuffer request
                 (WIND00424927)
01o,08jul13,ljg  Fix coverity issue (WIND00425410)
01n,03may13,wyy  Remove compiler warning (WIND00356717)
01m,21apr13,wyy  Cut the relationship among busIndex of HCDData, unitNumber of
                 vxBus Device, and HC count (such as g_EHCDControllerCount or
                 g_UhcdHostControllerCount) to dynamically release or announce
                 a HCD driver (WIND00362065)
01l,31jan13,ghs  Check the return value (WIND00360509)
01k,13dec11,m_y  Modify according to code check result (WIND00319317)
01j,02sep11,m_y  Using lstLib replace the old list structure add support for
                 polling mode (WIND00298534)
01i,14dec10,ghs  Change the usage of the reboot hook APIs to specific
                 rebootHookAdd/rebootHookDelete (WIND00240804)
01h,16sep10,m_y  Update the prototype for SYNOPSYSHCI init/exit routine
                 (WIND00232860)
01g,13sep10,ghs  Delete reboot hook when host controller init and exit
01f,02sep10,ghs  Use OS_THREAD_FAILURE to check taskSpawn failure (WIND00229830)
01e,08jul10,m_y  modify log message
01d,05feb10,m_y  modify usbSynopsysHcdDisableHC by turning off the port power
                 replace the closure of the USB PHY to avoid exception at reboot
                 (WIND00198543).
01c,22sep09,m_y  add schedule list to schedule the submited request.
01b,11aug09,s_z  add transfer thread and message queue to handle the
                 sequence message.
01a,20jul09,m_y  written.
*/

/*
DESCRIPTION

This file contains the initialization and uninitialization routines
provided by the Synopsys USB Host Controller Driver.

INCLUDE FILES: vxWorks.h, string.h, rebootLib.h, spinLockLib.h
               hwif/util/vxbParamSys.h, hwif/vxbus/vxBus.h,
               hwif/vxbus/hwConf.h, hwif/vxbus/vxbPciLib.h,
               usb/usbOsal.h, usb/usbHst.h, usb/usbHcdInstr.h,
               usbSynopsysHcdDataStructures.h,
               usbSynopsysHcdEventHandler.h,
               usbSynopsysHcdInterfaces.h,
               usbSynopsysHcdHardwareAccess.h,
               usbSynopsysHcdRegisterInfo.h,
               usbSynopsysHcdUtil.h
               hookLib.h
*/

/* includes */

#include <vxWorks.h>
#include <string.h>
#include <rebootLib.h>
#include <spinLockLib.h>
#include <hwif/util/vxbParamSys.h>
#include <hwif/vxbus/vxBus.h>
#include <hwif/vxbus/hwConf.h>
#include <hwif/vxbus/vxbPciLib.h>
#include <usb/usbOsal.h>
#include <usb/usbHst.h>
#include <usb/usbHcdInstr.h>
#include <hookLib.h>

/*
#include <usbSynopsysHcdDataStructures.h>
#include <usbSynopsysHcdEventHandler.h>
#include <usbSynopsysHcdInterfaces.h>
#include <usbSynopsysHcdHardwareAccess.h>
#include <usbSynopsysHcdRegisterInfo.h>
#include <usbSynopsysHcdUtil.h>
*/
#include "usbSynopsysHcdEventHandler.h"
#include "usbSynopsysHcdHardwareAccess.h"
#include "usbSynopsysHcdDataStructures.h"
#include "usbSynopsysHcdInterfaces.h"
#include "usbSynopsysHcdRegisterInfo.h"
#include "usbSynopsysHcdRhEmulation.h"
#include "usbSynopsysHcdUtil.h"

/* defines */
#define USB_SYNOPSYSHCD_RH_DOWN_PORTS          1

/* Name to register vxBus*/
#define USB_SHCI_HUB_NAME "vxbUsbSynopsysHciHub"
#define USB_SHCI_PLB_NAME "vxbPlbUsbSynopsysHci"
#define USB_SHCI_PCI_NAME "vxbPciUsbSynopsysHci"


#define USB_SHCD_ERR(fmt, arg1, arg2, arg3, arg4, arg5, arg6)   printf(fmt, arg1, arg2, arg3, arg4, arg5, arg6);


/* forward declarations */

IMPORT VOID usbVxbRootHubAdd (VXB_DEVICE_ID pDev);  /* function to add root hub*/

IMPORT STATUS usbVxbRootHubRemove (VXB_DEVICE_ID pDev); /* function to remove
                                                         * root hub
                                                         */

IMPORT VOID usbSynopsysHcdFlushRequest (pUSB_SYNOPSYSHCD_DATA pHCDData);

/* locals */

LOCAL STATUS vxbUsbSynopsysHciUnRegister (VOID);    /* function to deregister the
                                                     * Synopsys USB host controller
                                                     * driver with vxBus
                                                     */

LOCAL BOOL usbVxbHcdSynopsysHciDeviceProbe(VXB_DEVICE_ID pDev);


LOCAL VOID usbHcdSynopsysHciPlbInit1 (VXB_DEVICE_ID pDev); /* function to do the
                                                            * BSP specific
                                                            * initialization incase
                                                            * of PLB based Controller
                                                            */
LOCAL VOID usbHcdSynopsysHciPlbInit2 (VXB_DEVICE_ID pDev);

LOCAL VOID usbVxbNullFunction (VXB_DEVICE_ID pDev);

LOCAL VOID usbHcdSynopsysHciDeviceConnect (VXB_DEVICE_ID pDev);/* function to
                                                                * intialize the
                                                                * Synopsys USB
                                                                * Host controller
                                                                * device
                                                                */

LOCAL STATUS usbHcdSynopsysHciDeviceRemove(VXB_DEVICE_ID pDev); /* funtion to
                                                                 * uninitialize the
                                                                 * Synopsys USB
                                                                 * Host controller
                                                                 * device
                                                                 */

LOCAL STATUS usbSynopsysHcdHostBusInit(pUSB_SYNOPSYSHCD_DATA   * ppHCDData,
                                       VXB_DEVICE_ID    pDeviceInfo,
                                       UINT32           uBusIndex);

LOCAL VOID  usbSynopsysHcdHostBusUnInit(pUSB_SYNOPSYSHCD_DATA pHCDData);

LOCAL STATUS usbSynopsysHcdHostDataInit(pUSB_SYNOPSYSHCD_DATA pHCDData);

LOCAL VOID usbSynopsysHcdHostDataUnInit(pUSB_SYNOPSYSHCD_DATA pHCDData);

/* globals */

void vxbUsbSynopsysHciRegister (void);  /* function to register the Synopsys
                                         * USB host controller driver with vxBus
                                         */

int usbSynopsysHcdDisableHC(int startType);     /* function to be attached as
                                                 * reboot hook
                                                 */


/* To hold the array of pointers to the HCD maintained data structures */

pUSB_SYNOPSYSHCD_DATA * g_pSynopsysHCDData = NULL;

/* To hold the handle returned by the USBD during HC driver registration */

UINT32  g_SynopsysHCDHandle = 0;

/* Number of host controllers present in the system */

UINT32  g_SynopsysHCDControllerCount = 0;

/* Event used for synchronizing the access of the free lists */

OS_EVENT_ID g_SynopsysHcdListAccessEvent = NULL;

#ifdef USB_SHCD_POLLING_MODE
OS_THREAD_ID    ShcdPollingThread[USB_MAX_SYNOPSYSHCI_COUNT] ;
#endif


/*
 * Array of Spinlocks for the Synopsys USB host Controllers. The array will be
 * allocated in the usbShcdInit once the maxShciCount is known.
 */

spinlockIsr_t spinLockIsrSynopsysHcd[USB_MAX_SYNOPSYSHCI_COUNT];

/* locals */

LOCAL PCI_DEVVEND       usbVxbPcidevVendId[1] =
    {
    {0xffff,             /* device ID */
    0xffff}              /* vendor ID */
    };


LOCAL DRIVER_METHOD     usbVxbHcdSynopsysHciHCMethods[2] =
    {
    DEVMETHOD(vxbDrvUnlink, usbHcdSynopsysHciDeviceRemove),
    { 0, NULL }
    };

LOCAL struct drvBusFuncs usbVxbHcdSynopsysHciPlbDriverFuncs =
    {
    usbHcdSynopsysHciPlbInit1,                          /* init 1 */
    usbHcdSynopsysHciPlbInit2,                          /* init 2 */
    usbHcdSynopsysHciDeviceConnect                      /* device connect */
    };

LOCAL struct drvBusFuncs usbVxbHcdSynopsysHciPciDriverFuncs =
    {
    usbVxbNullFunction,                           /* init 1 */
    usbVxbNullFunction,                           /* init 2 */
    usbVxbNullFunction                            /* device connect */
    };

/* Initialization structure for Synopsys USB host Controller Root Hub */

LOCAL struct drvBusFuncs usbVxbRootHubDriverFuncs =
    {
    usbVxbNullFunction,         /* init 1 */
    usbVxbNullFunction,         /* init 2 */
    usbVxbRootHubAdd            /* device connect */
    };

/* Method structure for Synopsys USB host Controller Root Hub */

LOCAL struct vxbDeviceMethod usbVxbRootHubMethods[2] =
    {
    DEVMETHOD(vxbDrvUnlink, usbVxbRootHubRemove),
    DEVMETHOD_END
    };

/* DRIVER_REGISTRATION for Synopsys USB host Controller Root hub */

LOCAL DRIVER_REGISTRATION usbVxbHcdSynopsysHciHub =
    {
    NULL,                          /* pNext */
    VXB_DEVID_BUSCTRL,             /* hub driver is bus
                                    * controller
                                    */
    VXB_BUSID_USB_HOST_SYNOPSYSHCI,/* parent bus ID */
    USB_VXB_VERSIONID,             /* version */
    USB_SHCI_HUB_NAME,             /* driver name */
    &usbVxbRootHubDriverFuncs,     /* struct drvBusFuncs * */
    &usbVxbRootHubMethods[0],      /* struct vxbDeviceMethod */
    NULL,                          /* probe routine */
    NULL                           /* vxbParams */
    };

/* Default PLB Synopsys USB host Controller parameters */
LOCAL VXB_PARAMETERS usbVxbPlbHcdSynopsysHciDevParamDefaults[] =
    {
       {"regBigEndian", VXB_PARAM_INT32, {(void *)TRUE}},  /* origin*/
      /* {"regBigEndian", VXB_PARAM_INT32, {(void *FALSE}},  // jc */
       
       {"descBigEndian", VXB_PARAM_INT32, {(void *)FALSE}},
       {NULL, VXB_PARAM_END_OF_LIST, {NULL}}
    };

LOCAL DRIVER_REGISTRATION usbVxbPlbHcdSynopsysHciDevRegistration =
    {
    NULL,                                   /* register next driver */
    VXB_DEVID_BUSCTRL,                      /* bus controller */
    VXB_BUSID_PLB,                          /* bus id - PLB Bus Type */
    USB_VXB_VERSIONID,                      /* vxBus version Id */
    USB_SHCI_PLB_NAME,                      /* drv name */
    &usbVxbHcdSynopsysHciPlbDriverFuncs,    /* pDrvBusFuncs */
    &usbVxbHcdSynopsysHciHCMethods[0],      /* pMethods */
    NULL,                                   /* probe routine */
    usbVxbPlbHcdSynopsysHciDevParamDefaults /* vxbParams */
    };

LOCAL VXB_PARAMETERS usbVxbPciHcdSynopsysHciDevParamDefaults[] =
    {
       {NULL, VXB_PARAM_END_OF_LIST, {NULL}}
    };

LOCAL PCI_DRIVER_REGISTRATION   usbVxbPciHcdSynopsysHciDevRegistration =
    {
        {
        NULL,                                   /* register next driver */
        VXB_DEVID_BUSCTRL,                      /* bus controller */
        VXB_BUSID_PCI,                          /* bus id - PCI Bus Type */
        USB_VXB_VERSIONID,                      /* vxBus version Id */
        USB_SHCI_PCI_NAME,                      /* drv name */
        &usbVxbHcdSynopsysHciPciDriverFuncs,    /* pDrvBusFuncs */
        &usbVxbHcdSynopsysHciHCMethods [0],     /* pMethods */
        usbVxbHcdSynopsysHciDeviceProbe,        /* probe routine */
        usbVxbPciHcdSynopsysHciDevParamDefaults /* vxbParams */
        },
        NELEMENTS(usbVxbPcidevVendId),          /* idListLen */
        & usbVxbPcidevVendId [0]                /* idList */
    };

/*******************************************************************************
*
* usbSynopsysHcdInstantiate - instantiate the Synopsys USB HCD
*
* This routine instantiates the Synopsys USB Host Controller Driver and allows
* the Synopsys USB Host Controller driver to be included with the vxWorks image
* and not be registered with vxBus. Synopsys USB Host Controller devices will
* remain orphan devices until the usbSynopsysHcdInit() routine is called. This
* supports the INCLUDE_SYNOPSYSHCI behaviour of previous vxWorks releases.
*
* The routine itself does nothing.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void usbSynopsysHcdInstantiate (void)
    {
    return;
    }

/*******************************************************************************
*
* usbSynopsysHcdInit - initialize the Synopsys USB Host Controller Driver
*
* This routine intializes the Synopsys USB Host Controller data structures.
* This routine is executed prior to vxBus device connect to initialize
* data structures expected by the device initialization.
*
* The USBD must be initialized prior to calling this routine.
* In this routine the book-keeping variables for the Synopsys USB Host
* Controller Driver are initialized.
*
* RETURNS: OK, or ERROR if the initialization fails
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS usbSynopsysHcdInit (void)
    {
    USBHST_STATUS               status = USBHST_FAILURE;/* To hold the status */
    USBHST_HC_DRIVER            g_pSYNOPSYSHCDriverInfo;/* Structure to hold
                                                         * Synopsys HCI driver
                                                         * informations
                                                         */

	printf("\n....usbSynopsysHcdInit ... \n\n");  /* jc*/
														 
    /*
     * Check whether the globals are initialized - This can happen if this
     * routine is called more than once.
     */

    if (g_SynopsysHCDHandle != 0)
        {
        USB_SHCD_ERR("SYNOPSYSHCD already initialized\n", 0, 0, 0, 0, 0, 0);

        return ERROR;
        }

    if (g_pSynopsysHCDData != NULL)
        {
        USB_SHCD_ERR("SYNOPSYSHCD already initialized\n", 0, 0, 0, 0, 0, 0);

        return ERROR;
        }

    /* Initialize the global array */

    g_pSynopsysHCDData = (pUSB_SYNOPSYSHCD_DATA *)OS_MALLOC
                         (sizeof(pUSB_SYNOPSYSHCD_DATA) *
                          USB_MAX_SYNOPSYSHCI_COUNT);


    /* Check if memory allocation is successful */

    if (NULL == g_pSynopsysHCDData)
        {
        USB_SHCD_ERR("location failed for g_pSynopsysHCDData\n", 0, 0, 0, 0, 0, 0);

        return ERROR;
        }

    /* Reset the global array */

    OS_MEMSET (g_pSynopsysHCDData, 0,
               sizeof(pUSB_SYNOPSYSHCD_DATA) * USB_MAX_SYNOPSYSHCI_COUNT);

    /* Create the event used for synchronising the free list accesses */

    g_SynopsysHcdListAccessEvent = OS_CREATE_EVENT(OS_EVENT_SIGNALED);

    /* Check if the event is created successfully */

    if (NULL == g_SynopsysHcdListAccessEvent)
        {
        USB_SHCD_ERR("Error in creating the list event\n", 0, 0, 0, 0, 0, 0);

        /* Free the global array */

        OS_FREE (g_pSynopsysHCDData);

        g_pSynopsysHCDData = NULL;

        return ERROR;
        }

    /* Hook the routine which needs to be called on a reboot */

    if (ERROR == rebootHookAdd(usbSynopsysHcdDisableHC))
        {
        USB_SHCD_ERR("Error in hooking usbSynopsysHcdDisableHC\n",
                     0, 0, 0, 0, 0, 0);

        /* Destroy the event */

        OS_DESTROY_EVENT(g_SynopsysHcdListAccessEvent);
        g_SynopsysHcdListAccessEvent = NULL;

        /* Free the global array */

        OS_FREE (g_pSynopsysHCDData);

        g_pSynopsysHCDData = NULL;

        return ERROR;
        }

    /* Initialize the members of the data structure */

    OS_MEMSET(&g_pSYNOPSYSHCDriverInfo, 0, sizeof(USBHST_HC_DRIVER));

    /* Populate the members of the HC Driver data structure - start */
    /* Function to retrieve the frame number */

    g_pSYNOPSYSHCDriverInfo.getFrameNumber = usbSynopsysHcdGetFrameNumber;

    /* Function to change the frame interval */

    g_pSYNOPSYSHCDriverInfo.setBitRate = usbSynopsysHcdSetBitRate;

    /* Function to check whether bandwidth is available */

    g_pSYNOPSYSHCDriverInfo.isBandwidthAvailable = usbSynopsysHcdIsBandwidthAvailable;

    /* Function to create a pipe */

    g_pSYNOPSYSHCDriverInfo.createPipe = usbSynopsysHcdCreatePipe;

    /* Function to modify the default pipe */

    g_pSYNOPSYSHCDriverInfo.modifyDefaultPipe = usbSynopsysHcdModifyDefaultPipe;

    /* Function to delete the pipe */

    g_pSYNOPSYSHCDriverInfo.deletePipe = usbSynopsysHcdDeletePipe;

    /* Function to check if the request is pending */

    g_pSYNOPSYSHCDriverInfo.isRequestPending = usbSynopsysHcdIsRequestPending;

    /* Function to submit an URB */

    g_pSYNOPSYSHCDriverInfo.submitURB = usbSynopsysHcdSubmitURB;

    /* Function to cancel an URB */

    g_pSYNOPSYSHCDriverInfo.cancelURB = usbSynopsysHcdCancelURB;

    /* Function to submit a clear tt request complete */

    g_pSYNOPSYSHCDriverInfo.clearTTRequestComplete = NULL;

    /* Function to submit a reset tt request complete */

    g_pSYNOPSYSHCDriverInfo.resetTTRequestComplete = NULL;

    /* Populate the members of the HC Driver data structure - End */

    /*
     * Register the HCD with the USBD. We also pass the bus id in this function
     * This is to register Synopsys HCI driver with vxBus as a bus type.
     * After the registration is done we get a handle "g_SynopsysHCDHandle".
     * This handle is used for all subsequent communication of Synopsys HCI
     * driver with USBD.
     */

    status = usbHstHCDRegister(&g_pSYNOPSYSHCDriverInfo,
                               &g_SynopsysHCDHandle,
                               NULL,
                               VXB_BUSID_USB_HOST_SYNOPSYSHCI
                               );

    /* Check whether the registration is successful */

    if (USBHST_SUCCESS != status)
        {
        USB_SHCD_ERR("Error in registering the HCD \n", 0, 0, 0, 0, 0, 0);

        /* Destroy the event */

        OS_DESTROY_EVENT(g_SynopsysHcdListAccessEvent);

        g_SynopsysHcdListAccessEvent = NULL;

        /* Free the global array */

        OS_FREE (g_pSynopsysHCDData);

        g_pSynopsysHCDData = NULL;

        (void)rebootHookDelete(usbSynopsysHcdDisableHC);

        return ERROR;
        }

    return OK;
    }

/*******************************************************************************
*
* usbSynopsysHcdExit - uninitialize the Synopsys HCI host controller
*
* This routine uninitializes the Synopsys USB Host Controller Driver and
* detaches it from the usbd interface layer.
*
* RETURNS: OK, or ERROR if there is an error during HCD uninitialization.
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS usbSynopsysHcdExit(void)
    {
    USBHST_STATUS Status = USBHST_FAILURE;
    pUSB_SYNOPSYSHCD_DATA      pHCDData = NULL;
    UINT8                      index = 0;

    /* Flush request to terminate all the transfer */

    for (index = 0; index < USB_MAX_SYNOPSYSHCI_COUNT; index++)
        {

        /* Extract the pointer from the global array */

        pHCDData = g_pSynopsysHCDData[index];

        /* Check if the pointer is valid */

        if (NULL == pHCDData)
            break;

        /* Flush the request */

        usbSynopsysHcdFlushRequest(pHCDData);
        }

    /* Call the function to unregister with vxBus */

    if (ERROR == vxbUsbSynopsysHciUnRegister())
        {
        USB_SHCD_ERR("Error to Deregister SYNOPSYSHCI Driver with vxBus\n",
                     0, 0, 0, 0, 0, 0);

        return ERROR;
        }

    /* Deregister the HCD from USBD */

    Status = usbHstHCDDeregister(g_SynopsysHCDHandle);

    /* Check if HCD is deregistered successfully */

    if (USBHST_SUCCESS != Status)
        {
        USB_SHCD_ERR("Failure in deregistering the HCD\n",
                     0, 0, 0, 0, 0, 0);

        return ERROR;
        }
    if (ERROR == rebootHookDelete(usbSynopsysHcdDisableHC))
        return ERROR;


    g_SynopsysHCDHandle = 0;

    /* Destroy the event used for synchronisation of the free list */

    OS_DESTROY_EVENT(g_SynopsysHcdListAccessEvent);

    g_SynopsysHcdListAccessEvent = NULL;

    /* Free the memory allocated for the global data structure */

    OS_FREE(g_pSynopsysHCDData);

    g_pSynopsysHCDData = NULL;

    return OK;
    }

/*******************************************************************************
*
* vxbUsbSynopsysHciRegister - register the Synopsys HCD with vxBus
*
* This routine registers the Synopsys USB host controller Driver and Synopsys
* USB host controller Root-hub driver with vxBus. Note that this can be called
* early in the initialization sequence.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void vxbUsbSynopsysHciRegister (void)
{
#if 1 /* jc*/

    /* Register the Synopsys HCI driver for PCI bus as underlying bus */
    if (vxbDevRegister ((DRIVER_REGISTRATION *)
            &usbVxbPciHcdSynopsysHciDevRegistration)
            == ERROR)
        {
        USB_SHCD_ERR("Error  registering SYNOPSYSHCI Driver over PCI Bus\n",
                     0, 0, 0, 0, 0, 0);

        return;
        }

    /* Register the Synopsys HCI driver for PLB bus as underlying bus */

    if (vxbDevRegister ((DRIVER_REGISTRATION *)
        &usbVxbPlbHcdSynopsysHciDevRegistration)
        == ERROR)
        {
        USB_SHCD_ERR("Error registering SYNOPSYSHCI Driver over PLB Bus\n",
                     0, 0, 0, 0, 0, 0);

        return;
        }

    /* Register the Synopsys USB host Controller driver for root hub */

    if (vxbDevRegister ((DRIVER_REGISTRATION *)
        &usbVxbHcdSynopsysHciHub)
        == ERROR)
        {
        USB_SHCD_ERR("Error registering SYNOPSYSHCI root hub\n",
                     0, 0, 0, 0, 0, 0);

        return;
        }
#ifdef DEBUG_ANNOUNCE_USB
    USB_SHCD_INFO("vxbUsbSynopsysHciRegister - SYNOPSYSHCI "
                  "Driver successfully registered with vxBus\n",
                  0, 0, 0, 0, 0, 0);
#else
    printf("vxbUsbSynopsysHciRegister - SYNOPSYSHCI \nDriver successfully registered with vxBus\n");

#endif

#endif
    return;
}

void vxbUsbSynopsysHciRegister2 (void)
{
#if 1 /* jc*/

    /* Register the Synopsys HCI driver for PCI bus as underlying bus */
    if (vxbDevRegister ((DRIVER_REGISTRATION *)
            &usbVxbPciHcdSynopsysHciDevRegistration)
            == ERROR)
    {
        USB_SHCD_ERR("Error  registering SYNOPSYSHCI Driver over PCI Bus\n",
                     0, 0, 0, 0, 0, 0);

        return;
    }

    /* Register the Synopsys HCI driver for PLB bus as underlying bus */
    if (vxbDevRegister ((DRIVER_REGISTRATION *)
        &usbVxbPlbHcdSynopsysHciDevRegistration)
        == ERROR)
    {
        USB_SHCD_ERR("Error registering SYNOPSYSHCI Driver over PLB Bus\n",
                     0, 0, 0, 0, 0, 0);

        return;
    }

    /* Register the Synopsys USB host Controller driver for root hub */
    if (vxbDevRegister ((DRIVER_REGISTRATION *)
        &usbVxbHcdSynopsysHciHub)
        == ERROR)
    {
        USB_SHCD_ERR("Error registering SYNOPSYSHCI root hub\n",
                     0, 0, 0, 0, 0, 0);

        return;
    }
		
#ifdef DEBUG_ANNOUNCE_USB
    USB_SHCD_INFO("vxbUsbSynopsysHciRegister - SYNOPSYSHCI "
                  "Driver successfully registered with vxBus\n",
                  0, 0, 0, 0, 0, 0);
#endif

#endif
    return;
}

/*******************************************************************************
*
* vxbUsbSynopsysHciUnRegister - deregister Synopsys HCD driver with vxBus
*
* This routine deregisters the Synopsys USB host controller Driver with vxBus
* Module. The routine first de-registers the Synopsys USB host controller Root
* hub. This is followed by deregistration of Synopsys USB host controller for
* PCI and PLB bus types
*
* RETURNS: OK, or ERROR if not able to deregister with vxBus
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS vxbUsbSynopsysHciUnRegister (VOID)
    {

    /*
     * Unregister the Synopsys USB host Controller root hub
     * as bus controller driver
     */

    if (vxbDriverUnregister (&usbVxbHcdSynopsysHciHub) == ERROR)
        {
        USB_SHCD_ERR("Error de-registering SYNOPSYSHCI Root Hub with vxBus\n",
                     0, 0, 0, 0, 0, 0);

        return ERROR;
        }

    /*
     * Unregister the Synopsys USB host Controller driver for
     * PCI bus as underlying bus
     */

    if (vxbDriverUnregister ((DRIVER_REGISTRATION *)
        &usbVxbPciHcdSynopsysHciDevRegistration)
        == ERROR)
        {
        USB_SHCD_ERR("Error de-registering SYNOPSYSHCI Driver over PCI Bus\n",
                     0, 0, 0, 0, 0, 0);

        return ERROR;
        }

    /*
     * Unregister the Synopsys USB host Controller driver for
     * PLB bus as underlying bus
     */

    if (vxbDriverUnregister ((DRIVER_REGISTRATION *)
        &usbVxbPlbHcdSynopsysHciDevRegistration)
        == ERROR)
        {
        USB_SHCD_ERR("Error de-registering SYNOPSYSHCI Driver over PLB Bus\n",
                     0, 0, 0, 0, 0, 0);
        return ERROR;
        }
    return OK;
    }

/*******************************************************************************
*
* usbVxbHcdSynopsysHciDeviceProbe - judge whether mathcing device is Synopsys HCD
*
* This routine determines whether the matching device is an Synopsys USB host
* Controller or not. As the Synopsys USB host controller is not a PCI device
* this routine will always return FALSE.
*
* RETURNS: FALSE as Synopsys HCI is not a PCI device
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL BOOL usbVxbHcdSynopsysHciDeviceProbe
    (
    VXB_DEVICE_ID       pDev
    )
    {
    return FALSE;
    }

/*******************************************************************************
*
* usbSynopsysHcdDisableHC - called on a reboot to disable the host controller
*
* This routine is called on a warm reboot to disable the host controller by
* the BSP.
*
* RETURNS: 0, always
*
* ERRNO: N/A
*
* \NOMANUAL
*/

int usbSynopsysHcdDisableHC
    (
    int startType
    )
    {

    /* Pointer to the HCD data structure */

    pUSB_SYNOPSYSHCD_DATA      pHCDData = NULL;
    UINT8                      index = 0;

    if ((0 == g_SynopsysHCDControllerCount) ||
        (NULL == g_SynopsysHcdListAccessEvent) ||
        (NULL == g_pSynopsysHCDData))
        {
        return 0;
        }

    /* This loop releases the resources for all the host controllers present */

    for (index = 0; index < USB_MAX_SYNOPSYSHCI_COUNT; index++)
        {
        /* Extract the pointer from the global array */

        pHCDData = g_pSynopsysHCDData[index];

        /* Check if the pointer is valid */

        if (NULL == pHCDData)
            continue;

        /* Disable all interrupts */

        USB_SYNOPSYSHCD_WRITE32_REG(pHCDData,
                                    USB_SYNOPSYSHCD_GINTMSK,
                                    USB_SYNOPSYSHCD_INTERRUPT_MASK);

        /* Turn off the Vbus power */

        USB_SYNOPSYSHCD_CLEARBITS32_REG(pHCDData,
                                      USB_SYNOPSYSHCD_HPRT,
                                      USB_SYNOPSYSHCD_HPRT_PRTPWR);

        /* Call the function to unregister the interrupt line */

        usbSynopsysHcdHostBusUnInit (pHCDData);

        }

    return 0;
    }

/*******************************************************************************
*
* usbHcdSynopsysHciPlbInit1 - BSP specific initializaion
*
* For many PLB based controllers, different BSP level initialization is
* requried. This function does the BSP specific initializaiton incase it is
* required
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL VOID usbHcdSynopsysHciPlbInit1
    (
    VXB_DEVICE_ID pDev /* struct vxbDev */
    )
    {
    /* This is the first init sequence for Synopsys HCD */

    HCF_DEVICE    * pHcf = NULL;
    void          (* pPlbInit1) (void) = NULL;

    /* Get the HCF device from the instance id */

    pHcf = hcfDeviceGet (pDev);

    /* Get the PHY init function from BSP */

    if (NULL == pHcf || OK != devResourceGet (pHcf, "phyInit", HCF_RES_ADDR,(void *)&(pPlbInit1)))
       {
       USB_SHCD_ERR("devResourceGet phyInit fail\n", 1, 2, 3, 4, 5, 6);
       }

    if (NULL != pPlbInit1)
        {
        (*pPlbInit1)();
        }

    return;
    }

/*******************************************************************************
*
* usbHcdSynopsysHciPlbInit2 - second init sequence if need
*
* This routine just for extend if the second init sequence is neeed,now it
* is doing nothing.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL VOID usbHcdSynopsysHciPlbInit2
    (
    VXB_DEVICE_ID pDev /* struct vxbDev */
    )
    {
    return;
    }

/*******************************************************************************
*
* LOCAL VOID usbVxbNullFunction - dummy routine
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL VOID usbVxbNullFunction
    (
    VXB_DEVICE_ID       pDev /* struct vxbDev */
    )
    {
    /* This is a dummy routine which simply returns */

    return ;
    }

/*******************************************************************************
*
* usbHcdSynopsysHciDeviceConnect - initialize the Synopsys HCD
*
* This function intializes the Synopsys HCD and activates it to handle all
* USB operations. The function is called by vxBus on a successful
* driver - device match with VXB_DEVICE_ID as parameter. This structure has all
* information about the device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL VOID usbHcdSynopsysHciDeviceConnect
    (
    VXB_DEVICE_ID               pDev            /* struct vxbDev */
    )
    {
    STATUS                      busInitStatus = FALSE;
    STATUS                      dataInitStatus = FALSE;
    USBHST_STATUS               status = USBHST_FAILURE;
    pUSB_SYNOPSYSHCD_DATA       pHCDData = NULL;
    UINT32                      index;

    /* Validate paramters */

    if (NULL == pDev)
        {
        USB_SHCD_ERR("Invalid Parameters\n", 0, 0, 0, 0, 0, 0);

        status = USBHST_INVALID_PARAMETER;
        goto ERROR_HANDLER;
        }

    /*
     * Determine whether the usage counter has exceeded maximum number of
     * host controllers
     */

    OS_WAIT_FOR_EVENT (g_SynopsysHcdListAccessEvent, WAIT_FOREVER);

    if (USB_MAX_SYNOPSYSHCI_COUNT == g_SynopsysHCDControllerCount)
        {
        USB_SHCD_ERR("Out of resources \n", 0, 0, 0, 0, 0, 0);

        status = USBHST_INSUFFICIENT_RESOURCE;
        OS_RELEASE_EVENT (g_SynopsysHcdListAccessEvent);
        goto ERROR_HANDLER;
        }

    /* Update the unit number with g_SynopsysHCDControllerCount */
    for (index = 0; index < USB_MAX_SYNOPSYSHCI_COUNT; index ++)
        {
        if (g_pSynopsysHCDData[index]== NULL)
            break;
        }

    if (index == USB_MAX_SYNOPSYSHCI_COUNT)
        {
        USB_SHCD_ERR("g_pSynopsysHCDData is full\n",
            0, 0, 0, 0, 0, 0);

        status = USBHST_INSUFFICIENT_RESOURCE;

        OS_RELEASE_EVENT (g_SynopsysHcdListAccessEvent);

        goto ERROR_HANDLER;
        }

    /* If this devcie is a PCI device, unitNumber shall be assigned */
    if (0 == strncmp(pDev->pName, USB_SHCI_PCI_NAME, sizeof(USB_SHCI_PCI_NAME)))
        (void) vxbNextUnitGet(pDev);


    /* Initialize the ISR Spinlock */
    SPIN_LOCK_ISR_INIT(&spinLockIsrSynopsysHcd[index], 0);

    OS_RELEASE_EVENT (g_SynopsysHcdListAccessEvent);


    /*
     * Call the bus specific initialization function to initialize the
     * host controllers in the system
     */

    busInitStatus = usbSynopsysHcdHostBusInit(&pHCDData, pDev, index);
    if (OK != busInitStatus)
        {
        USB_SHCD_ERR("usbSynopsysHcdHostBusInit failed\n", 0, 0, 0, 0, 0, 0);

        /* Reset the global array */

        g_pSynopsysHCDData[index] = NULL;
        status = USBHST_FAILURE;
        goto ERROR_HANDLER;
        }

    /* Initialize the Host Controller data structure */

    dataInitStatus = usbSynopsysHcdHostDataInit(pHCDData);
    if (OK != dataInitStatus)
        {
        USB_SHCD_ERR("usbSynopsysHcdHostDataInit failed\n", 0, 0, 0, 0, 0, 0);

        /* Call the uninitialization function of the HC bus */

        usbSynopsysHcdHostBusUnInit(pHCDData);
        status = USBHST_FAILURE;
        goto ERROR_HANDLER;
        }

    /*
     * After we call vxbIntEnable, we will enter the ISR,
     * So we must make sure the device is OK to handlle the interrupt
     * But usually, the root hub isn't OK now.
     * So we'd better disable the interrupt before vxbIntEnable is called.
     * After the root hub is configured, the interrupt will be enabled
     */
    USB_SYNOPSYSHCD_WRITE32_REG(pHCDData,
                                USB_SYNOPSYSHCD_GINTMSK,
                                USB_SYNOPSYSHCD_INTERRUPT_MASK);
#ifndef USB_SHCD_POLLING_MODE

    /* Enable the interrupts */
    if (vxbIntEnable (pDev, 0, usbSynopsysHcdISR, pHCDData) == ERROR)
        {
        USB_SHCD_ERR("Error enabling intrrupts\n",
                     0, 0, 0, 0, 0, 0);

        /* Call the uninitialization function of the HC bus */

        usbSynopsysHcdHostBusUnInit(pHCDData);

        /* Call the uninitialization function of the HCD data */

        usbSynopsysHcdHostDataUnInit(pHCDData);

        status = USBHST_FAILURE;
        goto ERROR_HANDLER;
        }
#endif


    /* Take the mutex */

    OS_WAIT_FOR_EVENT (g_SynopsysHcdListAccessEvent, WAIT_FOREVER);

    /* Increase the global counter */

    g_SynopsysHCDControllerCount++;

    pHCDData->isrMagic = USB_SYNOPSYSHCD_MAGIC_ALIVE;

    /* Release the mutex */

    OS_RELEASE_EVENT (g_SynopsysHcdListAccessEvent);

    /* Register the bus with the USBD */

    status = usbHstBusRegister (g_SynopsysHCDHandle,
                                USBHST_HIGH_SPEED,
                                (UINT32)pHCDData->pDefaultPipe,
                                pDev);

    /* Check if the bus is registered successfully */
    if (USBHST_SUCCESS != status )
    {
        USB_SHCD_ERR("Error in registering the bus\n",
                     0, 0, 0, 0, 0, 0);

        /* Take the mutex */

        OS_WAIT_FOR_EVENT (g_SynopsysHcdListAccessEvent, WAIT_FOREVER);

        /* Decrease the global counter */

        g_SynopsysHCDControllerCount--;

        /* Reset the array element */

        g_pSynopsysHCDData[index] = NULL;

        /* Release the mutex */

        OS_RELEASE_EVENT (g_SynopsysHcdListAccessEvent);

#ifndef USB_SHCD_POLLING_MODE
        /* Disable intrrupts */
        (void) vxbIntDisable (pDev, 0, usbSynopsysHcdISR, pHCDData);
#endif
        /* Call the uninitialization function of the HC bus */
        usbSynopsysHcdHostBusUnInit(pHCDData);

        /* Call the uninitialization function of the HCD data */
        usbSynopsysHcdHostDataUnInit(pHCDData);

        status = USBHST_FAILURE;

        goto ERROR_HANDLER;
    }

    return;

ERROR_HANDLER:
    (void) ossStatus (status);
    return;
}

/*******************************************************************************
*
* usbHcdSynopsysHciDeviceRemove - remove the Synopsys HCD
*
* This function uninitializes the USB host controller device. The function
* is registered with vxBus and called when the driver is deregistered with
* vxBus. The function will have VXB_DEVICE_ID as its parameter. This structure
* will consists of all the information about the device. The function will
* subsequently de-register the bus from USBD.
*
* RETURNS: OK, or ERROR if not able to remove the Synopsys HCD
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS usbHcdSynopsysHciDeviceRemove
    (
    VXB_DEVICE_ID               pDev            /* struct vxbDev */
    )
    {
    pUSB_SYNOPSYSHCD_DATA pHCDData = NULL;  /* Pointer to the
                                             * USB_SYNOPSYSHCD_DATA
                                             */
    USBHST_STATUS  status;  /* To hold the status of the function */
    HCF_DEVICE    * pHcf = NULL;
    void          (*pPhyUnInit) (void) = NULL;
    UINT32        uBusIndex;


    /* Validate the paramters */

    if (NULL == pDev)
        return ERROR;

    OS_WAIT_FOR_EVENT (g_SynopsysHcdListAccessEvent, WAIT_FOREVER);

    /* Extract the pointer from the global array */

    pHCDData = pDev->pDrvCtrl;

    /* Check if the pointer is valid */

    if (NULL == pHCDData)
        {
        USB_SHCD_ERR("pHCDData is not valid\n",
                     0, 0, 0, 0, 0, 0);
        OS_RELEASE_EVENT (g_SynopsysHcdListAccessEvent);
        return ERROR;
        }

    uBusIndex = pHCDData->uBusIndex;

    /* Shut Down the Hardware */

    /* Disable USB Port */

    USB_SYNOPSYSHCD_SETBITS32_REG(pHCDData,
                                  USB_SYNOPSYSHCD_HPRT,
                                  USB_SYNOPSYSHCD_HPRT_PRTENA);
    /* Close USB PHY */

    pHcf = hcfDeviceGet (pDev);

    if (NULL == pHcf || OK != devResourceGet (pHcf, "phyUninit", HCF_RES_ADDR, (void *)&(pPhyUnInit)))
        {
        USB_SHCD_ERR("devResourceGet phyUninit fail\n",
                    1, 2, 3, 4, 5, 6);
        }

    if (NULL != pPhyUnInit)
        {
        (*pPhyUnInit)();
        }

    /* Set init flag to indicate the hardware is donw */

    pHCDData->initDone = 0;


    /*
     * Call the function to perform the host bus specific uninitialization
     * This function will un-hook the ISR from the interrupt line
     */

    usbSynopsysHcdHostBusUnInit(pHCDData);

    /* We can not respond to interrupts */

    pHCDData->isrMagic = USB_SYNOPSYSHCD_MAGIC_DEAD;

    /* Call the function to deregister the bus */

    status = usbHstBusDeregister(g_SynopsysHCDHandle,
                                 uBusIndex,
                                 (UINT32)pHCDData->pDefaultPipe);

    /* Check if the bus is deregistered successfully */

    if (USBHST_SUCCESS != status)
        {
        USB_SHCD_ERR("Failure in deregistering the bus\n",
                     0, 0, 0, 0, 0, 0);
        OS_RELEASE_EVENT (g_SynopsysHcdListAccessEvent);
        return ERROR;
        }

    /* Call the function to uninitialize the SYNOPSYSHCD data structure */

    usbSynopsysHcdHostDataUnInit(pHCDData);

    /* Release the memory allocated for the HCD data structure */

    OS_FREE(pHCDData);
    pHCDData = NULL;

    g_pSynopsysHCDData[uBusIndex] = NULL;

    /* Decrease the global counter by 1 */

    g_SynopsysHCDControllerCount--;

    OS_RELEASE_EVENT (g_SynopsysHcdListAccessEvent);

    return OK;
    }

/*******************************************************************************
*
* usbSynopsysHcdHostBusInit - perform the host bus specific initialization
*
* This function performs the host bus specific initialization of the
* Synopsys HCD.
*
* RETURNS: OK, or ERROR if the entry is faild
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS usbSynopsysHcdHostBusInit
    (
    pUSB_SYNOPSYSHCD_DATA       * ppHCDData,   /* struct usb_synopsyshcd_data */
    VXB_DEVICE_ID                 pDeviceInfo, /* struct vxbDev */
    UINT32                        uBusIndex    /* index of pUSB_SYNOPSYSHCD_DATA */
    )
    {
    HCF_DEVICE                * pHcf = NULL;
    VXB_INST_PARAM_VALUE        paramVal; /* To hold the parameter */
    UINT64                      baseReg64 = 0;
    UINT32                      baseReg32 = 0;
    UINT32                      resourceVal = 0;

    if ((NULL == ppHCDData) || (NULL == pDeviceInfo))
        {
        USB_SHCD_ERR("parameters are not valid\n",
                     0, 0, 0, 0, 0, 0);
        return ERROR;
        }

    /* get the HCF device from the instance id */

    pHcf = hcfDeviceGet (pDeviceInfo);

    if (pHcf == NULL)
        {
        USB_SHCD_ERR("hcfDeviceGet returned NULL\n",
                     0, 0, 0, 0, 0, 0);
        return ERROR;
        }

    /* Allocate memory for the USB_SYNOPSYSHCD_DATA structure */

    *ppHCDData = (pUSB_SYNOPSYSHCD_DATA)OS_MALLOC(sizeof(USB_SYNOPSYSHCD_DATA));

    /* Check if memory allocation is successful */

    if (NULL == *ppHCDData)
        {
        USB_SHCD_ERR("memory not allocated for HCD_DATA structure\n",
                     0, 0, 0, 0, 0, 0);

        return ERROR;
        }

    /* Initialize all the elements of the structure */

    OS_MEMSET(*ppHCDData, 0, sizeof(USB_SYNOPSYSHCD_DATA));

    /* Populate pUSB_SYNOPSYSHCD_DATA :: pDev */

    (*ppHCDData)->pDev = pDeviceInfo;

    (*ppHCDData)->pDev->pDrvCtrl = (pVOID) *ppHCDData;

    (*ppHCDData)->uBusIndex =  uBusIndex;

    switch (pDeviceInfo->busID)
        {
        case VXB_BUSID_PLB:
            /* Get addressmode and init the regbase */

            if (ERROR != devResourceGet (pHcf, "addressMode", HCF_RES_INT,
                            (void *)(&resourceVal)))
                {
                (*ppHCDData)->addressMode64 = resourceVal;
                }

            if (1 == (*ppHCDData)->addressMode64)
            {
                if (ERROR != devResourceGet (pHcf, "regBaseHigh", HCF_RES_INT,
                              (void *)(&resourceVal)))
                    {
                    baseReg64 = resourceVal;
                    }

                if (ERROR != devResourceGet (pHcf, "regBaseLow", HCF_RES_INT,
                              (void *)(&baseReg32)))
                    {
                    baseReg64 = (baseReg64 << 32) | baseReg32;
                    }

                (*ppHCDData)->regBase = (ULONG)baseReg64;

                /* Get register access entries */

                /* 64-bit register read/write uint64 entries */
                if (ERROR != devResourceGet (pHcf, "regRead64", HCF_RES_ADDR,
                                (void *)&((*ppHCDData)->pRegRead64Uint64Handle)))
                    {
                    USB_SHCD_VDBG ("hcfDeviceGet returns pRegRead64Uint64Handle %p\n",
                                   (*ppHCDData)->pRegRead64Uint64Handle, 2, 3, 4, 5 ,6);
                    }
								
                if (ERROR != devResourceGet (pHcf, "regWrite64", HCF_RES_ADDR,
                                (void *)&((*ppHCDData)->pRegWrite64Uint64Handle)))
                    {
                    USB_SHCD_VDBG ("hcfDeviceGet returns pRegWrite64Uint64Handle %p\n",
                                   (*ppHCDData)->pRegWrite64Uint64Handle, 2, 3, 4, 5 ,6);
                    }

            }
            else
            {
                /*
                 * For 32-bit system extention, BSP needs to pass the 32 bit base
                 * address in regBaseLow.
                 * However, we still set the 32-bit regbase to 64-bit regBase to make
                 * 64-bit safe.
                 */
                if (ERROR != devResourceGet (pHcf, "regBaseLow", HCF_RES_INT,
                                             (void *)(&baseReg32)))
                {
                    (*ppHCDData)->regBase = baseReg32;
                }
            }


            /* 
            Get platformType 
			*/
            if (ERROR != devResourceGet (pHcf, "platformType", HCF_RES_INT,
                                         (void *)(&resourceVal)))
            {
                (*ppHCDData)->uPlatformType = (UINT32)resourceVal;
            }

            /* Register read/write 32-bit entries */
            if (ERROR != devResourceGet (pHcf, "regRead32", HCF_RES_ADDR,
                                         (void *)&((*ppHCDData)->pRegRead32Handle)))
                {
                USB_SHCD_VDBG ("hcfDeviceGet returns regRead32 %p\n",
                               (*ppHCDData)->pRegRead32Handle, 2, 3, 4, 5 ,6);
                }

            if (ERROR != devResourceGet (pHcf, "regWrite32", HCF_RES_ADDR,
                                         (void *)&((*ppHCDData)->pRegWrite32Handle)))
                {
                USB_SHCD_VDBG ("hcfDeviceGet returns regRead32 %p\n",
                               (*ppHCDData)->pRegWrite32Handle, 2, 3, 4, 5 ,6);
                }

            /* Get root hub port count */
            if (ERROR != devResourceGet (pHcf, "rootHubNumPorts", HCF_RES_INT,
                           (void *)(&resourceVal)))
                {
                (*ppHCDData)->usbNumPorts = resourceVal;
                }

            /* Get host channel count */
            if (ERROR != devResourceGet (pHcf, "hostNumDmaChannels", HCF_RES_INT,
                           (void *)(&resourceVal)))
                {
                (*ppHCDData)->hostNumDmaChannels = resourceVal;
                }

            /*
             * Populate the function pointers for Cpu to Physical Memory and
             * vice versa conversions
             */
            if (ERROR != devResourceGet (pHcf, "cpuToBus", HCF_RES_ADDR,
                                         (void *)&((*ppHCDData)->pCpuToBus)))
                {
                USB_SHCD_VDBG("hcfDeviceGet returns cpuToBus %p\n",
                              (*ppHCDData)->pCpuToBus, 2, 3, 4, 5 ,6);
                }

            if (ERROR != devResourceGet (pHcf, "busToCpu", HCF_RES_ADDR,
                                         (void *)&((*ppHCDData)->pBusToCpu)))
                {
                USB_SHCD_VDBG("hcfDeviceGet returns busToCpu %p\n",
                              (*ppHCDData)->pBusToCpu, 2, 3, 4, 5 ,6);
                }

            /*
             * USB is little endian, if CPU is big endian, we need this swap!
             * This is to swap data such as the PortStatus and PortStatusChange
             * to/from CPU endian to set or clear any bit; And when reporting
             * to the USBD (by URB callback), swap into Little Endian so that
             * it conforms to USB requirement.
             */

#if (_BYTE_ORDER == _BIG_ENDIAN)
            (*ppHCDData)->pUsbSwap = vxbSwap32;
#endif

            if (OK == vxbInstParamByNameGet(pDeviceInfo,
                                            "regBigEndian",
                                            VXB_PARAM_INT32,
                                            &paramVal))
                {
#if (_BYTE_ORDER == _BIG_ENDIAN)
                /* CPU BE, but REG LE */

                if (paramVal.int32Val == FALSE)
                    {
                    (*ppHCDData)->pRegSwap = vxbSwap32;
                    }
#else
               /* CPU LE, but REG BE */

               if (paramVal.int32Val == TRUE)
                    {
                    (*ppHCDData)->pRegSwap = vxbSwap32;
                    }
#endif
                }
            else /* vxbInstParamByNameGet == ERROR */
                {
                USB_SHCD_WARN("Get regBigEndian in BSP failed, which may casue"\
                              " undefined behavior\n",
                              0, 0, 0, 0, 0, 0);
                }

            if (OK == vxbInstParamByNameGet(pDeviceInfo,
                                            "descBigEndian",
                                            VXB_PARAM_INT32,
                                            &paramVal))
                {
#if (_BYTE_ORDER == _BIG_ENDIAN)
                /* CPU BE, but DESC LE */

                if (paramVal.int32Val == FALSE)
                    {
                    (*ppHCDData)->pDescSwap = vxbSwap32;
                    }
#else
                /* CPU LE, but DESC BE */

                if (paramVal.int32Val == TRUE)
                    {
                    (*ppHCDData)->pDescSwap = vxbSwap32;
                    }
#endif
                }
            else /* vxbInstParamByNameGet == ERROR*/
                {
                USB_SHCD_WARN("Get descBigEndian in BSP failed, which may cause"\
                              " undefined behavior\n",
                              0, 0, 0, 0, 0, 0);
                }

            /* Update the vxBus read-write handle */

            if ((*ppHCDData)->pRegSwap != NULL)
                {
                (*ppHCDData)->pRegAccessHandle = (void *)
                 VXB_HANDLE_SWAP ((UINT32)((*ppHCDData)->pRegAccessHandle));
                }
            break;
        case VXB_BUSID_PCI:
            /* Currently no PCI support is needed, fall through! */		
        default:
            (*ppHCDData)->pDev->pDrvCtrl = NULL;
            OS_FREE(*ppHCDData);
            return ERROR;
        }

    /* Initialize the hardware */
    if ((*ppHCDData)->initDone == 0)
    {
        /* Initialize the usb Core */
        usbSynopsysHCDCoreInit(*ppHCDData);

        /* Initialize the usb host side function */
        usbSynopsysHCDHostInit(*ppHCDData);
        (*ppHCDData)->initDone = 1;
    }

    /* Initialize the global array with the element */
    g_pSynopsysHCDData[(*ppHCDData)->uBusIndex] = *ppHCDData;

#ifndef USB_SHCD_POLLING_MODE
    /* Register the interrupt handler for the IRQ */
    if (ERROR == (vxbIntConnect (pDeviceInfo,
                                 0,
                                 usbSynopsysHcdISR,
                                 (VOID *)(*ppHCDData))))
        {
        USB_SHCD_ERR("Error hooking the ISR\n", 0, 0, 0, 0, 0, 0);

        (*ppHCDData)->pDev->pDrvCtrl = NULL;
        OS_FREE(*ppHCDData);
        return ERROR;
        }
#else

    ShcdPollingThread[uBusIndex] = OS_CREATE_THREAD("PollingISR",
                           USB_SHCD_POLLING_THREAD_PRIORITY,
                           usbShcdPollingISR,
                           (VOID *)(*ppHCDData));

     if (ShcdPollingThread[uBusIndex] == OS_THREAD_FAILURE)
         {
         USB_SHCD_ERR(
             "Error creating the polling task\n",
             0, 0, 0, 0, 0, 0);
         (*ppHCDData)->pDev->pDrvCtrl = NULL;
         OS_FREE(*ppHCDData);
         return ERROR;
         }
#endif
    return OK;
    }

/*******************************************************************************
*
* usbSynopsysHcdHostBusUnInit - perform the host bus specific uninitialization
*
* This routine performs the host bus specific uninitialization.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL VOID usbSynopsysHcdHostBusUnInit
    (
    pUSB_SYNOPSYSHCD_DATA pHCDData /* Pointer to HCD block */
    )
    {

    /* Check the validity of the parameter */

    if (NULL == pHCDData)
        {
        USB_SHCD_ERR("invalid parameter\n",
                     0, 0, 0, 0, 0, 0);
        return;
        }
#ifndef USB_SHCD_POLLING_MODE

    /* Disable the interrupts */

    if (vxbIntDisable (pHCDData->pDev, 0, usbSynopsysHcdISR, (VOID *)(pHCDData))
                  == ERROR)
        {
        USB_SHCD_ERR("failure in deregistering the bus\n",
                     0, 0, 0, 0, 0, 0);
        return;
        }
#else
    if ((ShcdPollingThread[pHCDData->uBusIndex] != OS_THREAD_FAILURE) &&
        (ShcdPollingThread[pHCDData->uBusIndex] != NULL))
        {

        OS_DESTROY_THREAD (ShcdPollingThread[pHCDData->uBusIndex]);

        ShcdPollingThread[pHCDData->uBusIndex] = OS_THREAD_FAILURE;
        }
#endif
    return;
    }

/*******************************************************************************
*
* usbSynopsysHcdHostDataInit - initialize USB_SYNOPSYSHCD_DATA structure.
*
* This routine initializes the USB_SYNOPSYSHCD_DATA structure.
*
* RETURNS: OK, or ERROR if USB_SYNOPSYSHCD_DATA initialization is unsuccessful.
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS usbSynopsysHcdHostDataInit
    (
    pUSB_SYNOPSYSHCD_DATA            pHCDData /* Pointer to HCD block */
    )
    {
    UCHAR ThreadName[20]; /* To hold the name of the interrupt thread */
    UCHAR TransferThreadName[20]; /* To hold the name of the transfer thread */
    UINT32 uHubStatusChangeSize = 0; /* To hold hub status change data size */

    /* Check the validity of the parameter */

    if (NULL == pHCDData)
        {
        USB_SHCD_ERR("Parameter is not valid\n", 0, 0, 0, 0, 0, 0);

        return ERROR;
        }

    /* Init RootHub Data */

    pHCDData->RHData.uNumPorts = pHCDData->usbNumPorts;

    /* Check if the number of ports is valid */

    if (0 == pHCDData->RHData.uNumPorts)
        {
        USB_SHCD_ERR("uNumPorts is 0, reject!\n", 0, 0, 0, 0, 0, 0);
        return ERROR;
        }

    /* Allocate memory for the Root hub port status */

    pHCDData->RHData.pPortStatus = (UCHAR *)
                                    OS_MALLOC(pHCDData->RHData.uNumPorts
                                    * USB_SYNOPSYSHCD_HUBSTATUS_SIZE);

    /* Check if memory allocation is successful */

    if (NULL == pHCDData->RHData.pPortStatus)
        {
        USB_SHCD_ERR("memory not allocated for port status\n",
                     0, 0, 0, 0, 0, 0);

        usbSynopsysHcdHostDataUnInit(pHCDData);
        return ERROR;
        }

    /* Initialize the fields of the port status register */

    OS_MEMSET(pHCDData->RHData.pPortStatus, 0,
              pHCDData->RHData.uNumPorts *
              USB_SYNOPSYSHCD_HUBSTATUS_SIZE);

    /*
     * The interrupt transfer data is of the following format
     * D0 - Holds the status change information of the hub
     * D1 - Holds the status change information of the Port 1
     *
     * Dn - holds the status change information of the Port n
     * So if the number of downstream ports is N, the size of interrupt
     * transfer data would be N + 1 bits.
     */

    uHubStatusChangeSize = (pHCDData->RHData.uNumPorts + 1)/ 8;
    if (0 != ((pHCDData->RHData.uNumPorts + 1) % 8))
        {
        uHubStatusChangeSize = uHubStatusChangeSize + 1;
        }

    /* Allocate memory for the interrupt transfer data */

    pHCDData->RHData.pHubInterruptData = OS_MALLOC(uHubStatusChangeSize);

    /* Check if memory allocation is successful */

    if (NULL == pHCDData->RHData.pHubInterruptData)
        {
        USB_SHCD_ERR("memory not allocated for interrupt transfer data\n",
                     0, 0, 0, 0, 0, 0);

        usbSynopsysHcdHostDataUnInit(pHCDData);

        return ERROR;
        }

    /* Initialize the interrupt data */

    OS_MEMSET(pHCDData->RHData.pHubInterruptData, 0, uHubStatusChangeSize);

    /* Copy the size of the interrupt data */

    pHCDData->RHData.uSizeInterruptData = uHubStatusChangeSize;

    /* Create the default pipe */

    pHCDData->pDefaultPipe = usbSynopsysHcdNewPipe();

    /* Check if default pipe is created successfully */

    if (NULL == pHCDData->pDefaultPipe)
        {
        USB_SHCD_ERR("memory not allocated for the default pipe\n",
                     0, 0, 0, 0, 0, 0);

        usbSynopsysHcdHostDataUnInit(pHCDData);

        return ERROR;
        }

    /* Create the event which is used for signalling the thread on interrupt */

    pHCDData->interruptEvent = OS_CREATE_EVENT(OS_EVENT_NON_SIGNALED);

    /* Check if the event is created successfully */

    if (NULL == pHCDData->interruptEvent)
        {
        USB_SHCD_ERR("signal event not created\n",
                     0, 0, 0, 0, 0, 0);

        usbSynopsysHcdHostDataUnInit(pHCDData);
        return ERROR;
        }

    /*
     * Create message queue which is used for transfer
     * message to transfer thread
     */

    pHCDData->transferThreadMsgQ = msgQCreate(USB_SYNOPSYSHCD_MSGQ_COUNT,
                                              sizeof(USB_SYNOPSYSHCD_TRANSFER_TASK_INFO),
                                              MSG_Q_FIFO);

    if (NULL == pHCDData->transferThreadMsgQ)
        {
        USB_SHCD_ERR("message queue not created\n",
                     0, 0, 0, 0, 0, 0);

        usbSynopsysHcdHostDataUnInit(pHCDData);

        return ERROR;
        }

    /* Create the event used for synchronisation of the requests */

    pHCDData->RequestSynchEventID = semMCreate(SEM_Q_PRIORITY     |
                                               SEM_INVERSION_SAFE |
                                               SEM_DELETE_SAFE);

    /* Check if the event is created successfully */

    if (NULL == pHCDData->RequestSynchEventID)
        {
        USB_SHCD_ERR("synch event not created\n",
                     0, 0, 0, 0, 0, 0);

        usbSynopsysHcdHostDataUnInit(pHCDData);

        return ERROR;
        }

    /* Init the pipe list */

    lstInit(&(pHCDData->pipeList));

    /* Init the schedule list for transfer data */

    lstInit(&(pHCDData->periodicReqList));

    lstInit(&(pHCDData->periodicReqReadyList));

    lstInit(&(pHCDData->nonPeriodicReqList));

    lstInit(&(pHCDData->nonPeriodicReqReadyList));

    /* Init the IdleDmaChannelMap, a bit of 1 means idle */

    pHCDData->uIdleDmaChannelMap = 0xFFFFFFFF;

    /* Assign an unique name for the interrupt handler thread */

    (void) snprintf((char*)ThreadName, 20, "SynoISR%d", pHCDData->uBusIndex);

    pHCDData->dmaChannelMutex = semMCreate(SEM_Q_PRIORITY |
                                           SEM_INVERSION_SAFE |
                                           SEM_DELETE_SAFE);

    if (pHCDData->dmaChannelMutex == NULL)
        {
        USB_SHCD_ERR("usbSynopsysHcdHostDataInit(): "
                     "creat dmaChannelMutex failed\n", 1, 2, 3, 4, 5, 6);
        return ERROR;
        }

    /* Create the interrupt handler thread for handling the interrupts */

    pHCDData->intHandlerThread = OS_CREATE_THREAD((char *)ThreadName,
                                                  USB_SYNOPSYSHCD_INT_THREAD_PRIORITY,
                                                  usbSynopsysHcdInterruptHandler,
                                                  pHCDData);

    /* Check whether the thread creation is successful */

    if (OS_THREAD_FAILURE == pHCDData->intHandlerThread)
        {
        USB_SHCD_ERR("interrupt handler thread is not created\n",
                     0, 0, 0, 0, 0, 0);

        usbSynopsysHcdHostDataUnInit(pHCDData);

        return ERROR;
        }


    /* Assign an unique name for the transfer handler thread */

    snprintf((char*)TransferThreadName, 20, "SynoTran%d", pHCDData->uBusIndex);

    /*
     * Create the transfer handler thread for handling the message sended by
     * the interrupt thread
     */

    pHCDData->transferThread = OS_CREATE_THREAD((char *)TransferThreadName,
                                                 USB_SYNOPSYSHCD_INT_THREAD_PRIORITY,
                                                 usbSynopsysHcdTransferHandler,
                                                 pHCDData);

    /* Check whether the thread creation is successful */

    if (OS_THREAD_FAILURE == pHCDData->transferThread)
        {
        USB_SHCD_ERR("transfer handler thread is not created\n",
                     0, 0, 0, 0, 0, 0);

        usbSynopsysHcdHostDataUnInit(pHCDData);

        return ERROR;
        }

    return OK;
    }

/*******************************************************************************
*
* usbSynopsysHcdHostDataUnInit - uninitialize the USB_SYNOPSYSHCD_DATA structure
*
* This routine uninitializes the USB_SYNOPSYSHCD_DATA structure.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL VOID usbSynopsysHcdHostDataUnInit
    (
    pUSB_SYNOPSYSHCD_DATA pHCDData /* Pointer to HCD block */
    )
    {
    if (NULL == pHCDData)
        {
        USB_SHCD_ERR("invalid parameter\n", 0, 0, 0, 0, 0, 0);
        return;
        }

    /* Destroy the thread created for handling the interrupts */

    if (OS_THREAD_FAILURE != pHCDData->intHandlerThread
        && 0 != pHCDData->intHandlerThread)
        {
        OS_DESTROY_THREAD(pHCDData->intHandlerThread);
        pHCDData->intHandlerThread = OS_THREAD_FAILURE;
        }

    /* Destroy the signalling event */

    if (NULL != pHCDData->interruptEvent)
        {
        OS_DESTROY_EVENT(pHCDData->interruptEvent);
        pHCDData->interruptEvent = NULL;
        }

    if (NULL != pHCDData->dmaChannelMutex)
        {
        OS_DESTROY_EVENT(pHCDData->dmaChannelMutex);
        pHCDData->dmaChannelMutex = NULL;
        }

    /* Destroy the thread created for handling the message */

    if (0 != pHCDData->transferThread
        && OS_THREAD_FAILURE != pHCDData->transferThread)
        {
        OS_DESTROY_THREAD(pHCDData->transferThread);
        pHCDData->transferThread = OS_THREAD_FAILURE;
        }

    /* Destroy the MsgQ */

    if (pHCDData->transferThreadMsgQ)
        {
        (void) msgQDelete (pHCDData->transferThreadMsgQ);
        pHCDData->transferThreadMsgQ = NULL;
        }

    /* Destroy the synchronization event */

    if (NULL != pHCDData->RequestSynchEventID)
        {
        if (OK == OS_WAIT_FOR_EVENT(pHCDData->RequestSynchEventID, WAIT_FOREVER))
            {
            OS_DESTROY_EVENT(pHCDData->RequestSynchEventID);
            pHCDData->RequestSynchEventID = NULL;
            }
        else
            {
            USB_SHCD_ERR("Take pHCDData->RequestSynchEventID failed. \n",
                         1, 2, 3, 4, 5, 6);
            }
        }

    /* Free memory allocated for the default pipe */

    if (NULL != pHCDData->pDefaultPipe)
        {
        usbSynopsysHcdDestroyPipe(pHCDData->pDefaultPipe);
        pHCDData->pDefaultPipe = NULL;
        }

    /* Free memory allocated for the interrupt data */

    if (NULL != pHCDData->RHData.pHubInterruptData)
        {
        OS_FREE(pHCDData->RHData.pHubInterruptData);
        pHCDData->RHData.pHubInterruptData = NULL;
        }

    /* Free memory allocated for the port status */

    if (NULL != pHCDData->RHData.pPortStatus)
        {
        OS_FREE(pHCDData->RHData.pPortStatus);
        pHCDData->RHData.pPortStatus = NULL;
        }
    return;
    }

/* End of file */
