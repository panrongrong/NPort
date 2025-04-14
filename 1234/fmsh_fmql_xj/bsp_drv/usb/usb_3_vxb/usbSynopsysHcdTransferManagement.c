/* usbSynopsysHcdTransferManagement.c - interfaces registered with USBD */

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
01u,23dec14,wyy  remove unused VX_SYNC_BARRIER and add protection for setting SOF
                 register (VXW6-83137)
01t,09sep14,wyy  unlink request before callback and add VX_SYNC_BARRIER 
                 (VXW6-83137)
01s,09aug13,ljg  adjust pHCDData->RequestSynchEventID (WIND00429223)
01r,25jul13,ljg  add dmaChannelMutex to protect uIdleDmaChannelMap (WIND00427761)
01q,10Jul13,wyy  Make usbd layer to uniformly handle ClearTTBuffer request
                 (WIND00424927)
01p,04jul13,ljg  Continue to release resource when get NULL request (WIND00423618)
01o,03may13,wyy  Remove compiler warning (WIND00356717)
01n,21apr13,wyy  Cut the relationship among busIndex of HCDData, unitNumber of
                 vxBus Device, and HC count (such as g_EHCDControllerCount or
                 g_UhcdHostControllerCount) to dynamically release or announce
                 a HCD driver (WIND00362065)
01m,02sep11,m_y  Using lstLib replace the old list structure
01l,23aug11,m_y  Modify usbSynopsysHcdDeletePipe (WIND00289879)
01k,05aug11,m_y  Modify the usbSynopsysHcdCancelURB, usbSynopsysHcdDeletePipe and
                 usbSynopsysHcdModifyDefaultPipe process (WIND00290062)
01j,06jan11,m_y  Implement routine usbSynopsysHcdIsRequestPending (WIND00247982)
01i,16sep10,m_y  Replace the OS_ASSERT by if judgment (WIND00232860)
01h,08sep10,m_y  modify to remove build warning
01g,08jul10,m_y  modify log message
01f,05feb10,m_y  modify the usbSynopsysHcdCancelURB and usbSynopsysHcdDeletePipe
                 entries to make sure the resources can be released in "CHHLTD"
                 interrupt (WIND00198693)
01e,03nov09,m_y  modify the usbSynopsysHcdCancelURB and usbSynopsysHcdDeletePipe
                 entries to make the plug out usb device safely release
                 resources.
01d,24sep09,m_y  modify the usbSynopsysHcdGetFrameNumber entry to make
                 ISO transfer successfully.
01c,22sep09,m_y  add schedule list to schuedule all the submiteed URB.
01b,10sep09,m_y  remove warning message.
01a,01sep09,s_z  written.
*/

/*
DESCRIPTION

This module defines the interfaces which are registered with the USBD during
Synopsys USB Host Controller Driver initialization.

INCLUDE FILES: usb/usbOsal.h, usb/usbHst.h, usb/usbHcdInstr.h, usb/usb.h,
               usb/usbd.h, usbSynopsysHcdDataStructures.h,
               usbSynopsysHcdInterfaces.h, usbSynopsysHcdRegisterInfo.h,
               usbSynopsysHcdHardwareAccess.h, usbSynopsysHcdRhEmulation.h,
               usbSynopsysHcdUtil.h,
*/

/* includes */

#include <usb/usbOsal.h>
#include <usb/usbHst.h>
#include <usb/usbHcdInstr.h>
#include <usb/usb.h>
#include <usb/usbd.h>

/*
#include <usbSynopsysHcdDataStructures.h>
#include <usbSynopsysHcdInterfaces.h>
#include <usbSynopsysHcdRegisterInfo.h>
#include <usbSynopsysHcdHardwareAccess.h>
#include <usbSynopsysHcdRhEmulation.h>
#include <usbSynopsysHcdUtil.h>
*/
#include "usbSynopsysHcdEventHandler.h"
#include "usbSynopsysHcdHardwareAccess.h"
#include "usbSynopsysHcdDataStructures.h"
#include "usbSynopsysHcdInterfaces.h"
#include "usbSynopsysHcdRegisterInfo.h"
#include "usbSynopsysHcdRhEmulation.h"
#include "usbSynopsysHcdUtil.h"

/* globals */

/* To hold the array of pointers to the HCD maintained data structures */

IMPORT pUSB_SYNOPSYSHCD_DATA *g_pSynopsysHCDData;

/* Number of host controllers present in the system */

IMPORT UINT32  g_SynopsysHCDControllerCount;

IMPORT STATUS usbShdrcHsDmaChannelRelease
    (
    pUSB_SYNOPSYSHCD_DATA pHCDData,
    int                  uChannel
    );

/*******************************************************************************
*
* usbSynopsysHcdGetFrameNumber - get the current frame number of the bus
*
* This function is used to get the current frame number of the host controller.
* <uBusIndex> specifies the host controller bus index.
* <puFrameNumber> is a pointer to a variable to hold the current frame number.
*
* RETURNS: USBHST_STATUS of the get result.
*
* ERRNO: N/A
*
* \NOMANUAL
*/

USBHST_STATUS usbSynopsysHcdGetFrameNumber
    (
    UINT8   uBusIndex,    /* Index of the host controller */
    UINT16 *puFrameNumber /* Pointer to the variable to hold the frame number */
    )
    {

    /* To hold the pointer to the data structure */

    pUSB_SYNOPSYSHCD_DATA pHCDData = NULL;

    /* To hold the status of HFNUM register */

    UINT32 uUsbHfnumReg = 0;

    /* Check the validity of the parameters */

    if ((USB_MAX_SYNOPSYSHCI_COUNT <= uBusIndex) ||
        (NULL == puFrameNumber))
        {
        USB_SHCD_ERR("Invalid parameters\n",
                     0, 0, 0, 0, 0, 0);

        return USBHST_INVALID_PARAMETER;
        }

    /* Extract the global data structure */

    pHCDData = g_pSynopsysHCDData[uBusIndex];

    /* Assert if the global pointer is not valid */

    if (pHCDData == NULL)
        {
        USB_SHCD_ERR("Invalid pHCDData\n", 0, 0, 0, 0, 0, 0);

        return USBHST_INVALID_PARAMETER;
        }

    /* Read the frame register contents and copy it to the OUT parameter */

    uUsbHfnumReg = USB_SYNOPSYSHCD_READ32_REG (pHCDData, USB_SYNOPSYSHCD_HFNUM);

    /*
     * Until the USB device plug in and be enabled, the frame number woun't
     * change this will make the init fail
     * "uMicroFrameNumChangeFlag" as a flag to indicate whether the frame number
     * can be changed by hardware
     * "uMicroFrameNumStep" as a step to make the frame number to be increased
     */

    if (pHCDData->uMicroFrameNumChangeFlag == 0)
        {
        *puFrameNumber = (UINT16)(((uUsbHfnumReg &
                                  USB_SYNOPSYSHCD_HFNUM_FRNUM_MASK)) +
                                  pHCDData->uMicroFrameNumStep) %
                                  USB_SYNOPSYSHCD_FRAME_NUM_MAX;
        pHCDData->uMicroFrameNumStep += 4;
        }
    else
        {
        *puFrameNumber = (UINT16)((uUsbHfnumReg &
                                  USB_SYNOPSYSHCD_HFNUM_FRNUM_MASK)) %
                                  USB_SYNOPSYSHCD_FRAME_NUM_MAX;
        }

	return USBHST_SUCCESS;
    }

/*******************************************************************************
*
* usbSynopsysHcdSetBitRate - modify the frame width
*
* This function is used to modify the frame width of the host controller.
* <uBusIndex> specifies the host controller bus index. <bIncrement> is a flag
* to specify whether the frame number should be incremented or decremented.
* If TRUE, the frame number will be incremented, otherwise, decremented.
* <puCurrentFrameWidth> is a pointer to hold the current frame width(after
* modification)
*
* RETURNS: USBHST_FAILURE - returned since the functionality is not
* supported.
*
* ERRNO: N/A
*
* \NOMANUAL
*/

USBHST_STATUS usbSynopsysHcdSetBitRate
    (
    UINT8   uBusIndex,          /* Index of the host controller       */
    BOOL    bIncrement,         /* Flag for increment or decrement    */
    UINT32 *puCurrentFrameWidth /* Pointer to the current frame width */
    )
    {

    /* As this functionality is not supported, always return a failure. */

    return USBHST_FAILURE;
    }

/*******************************************************************************
*
* usbSynopsysHcdIsBandwidthAvailable - check the bandwidth availability
*
* This function is used to check whether there is enough
* bandwidth to support the new configuration or an alternate interface setting
* of an interface.
* <uBusIndex> specifies the host controller bus index.
* <uDeviceAddress> specifies the device address. <uDeviceSpeed> is the speed
* of the device which needs to be modified. <pCurrentDescriptor> is the pointer
* to the current configuration or interface descriptor. If the pNewDescriptor
* corresponds to a USB configuration descriptor, this parameter is ignored
* (i.e. this parameter can be NULL). <pNewDescriptor> is the pointer to the new
* configuration or interface descriptor.
*
* RETURNS: USBHST_STATUS of the check result.
*
* ERRNO: N/A
*
* \NOMANUAL
*/

USBHST_STATUS usbSynopsysHcdIsBandwidthAvailable
    (
    UINT8   uBusIndex,            /* Host controller bus index */
    UINT8   uDeviceAddress,       /* Handle to the device addr */
    UINT8   uDeviceSpeed,         /* Speed of the device in default state */
    UCHAR * pCurrentDescriptor,   /* Ptr to current configuration */
    UCHAR * pNewDescriptor        /* Ptr to new configuration */
    )
    {

    /* Pointer to the HCD specific data structure */

    pUSB_SYNOPSYSHCD_DATA  pHCDData = NULL;


    /* Check the validity of the parameters */

    if (USB_MAX_SYNOPSYSHCI_COUNT <= uBusIndex ||
        0 == uDeviceAddress ||
        NULL == pNewDescriptor)
        {
        USB_SHCD_ERR("Parameters are not valid\n", 0, 0, 0, 0, 0, 0);
        return USBHST_INVALID_PARAMETER;
        }

    /* Extract the global data structure */

    pHCDData = g_pSynopsysHCDData[uBusIndex];

    /* Judge if the global pointer is not valid */

    if (pHCDData == NULL)
        {
        USB_SHCD_ERR("pHCDData are not valid\n", 0, 0, 0, 0, 0, 0);
        return USBHST_INVALID_PARAMETER;
        }


    /* Return success */

    return USBHST_SUCCESS;
    }

/*******************************************************************************
*
* usbSynopsysHcdCreatePipe - create a pipe specific to an endpoint
*
* This function creates a pipe specific to an endpoint.
*
* RETURNS: USBHST_STATUS of the create result.
*
* ERRNO: N/A
*
* \NOMANUAL
*/

USBHST_STATUS usbSynopsysHcdCreatePipe
    (
    UINT8   uBusIndex,          /* Host controller index      */
    UINT8   uDeviceAddress,     /* USB device address         */
    UINT8   uDeviceSpeed,       /* USB device speed           */
    UCHAR  *pEndpointDescriptor,/* Endpoint descriptor        */
    UINT16  uHighSpeedHubInfo,  /* High speed hub information */
    ULONG  *puPipeHandle        /* Pointer to the pipe handle */
    )
    {

    /* To hold the status of the request */

    USBHST_STATUS Status = USBHST_FAILURE;

    /* Pointer to the Endpoint Descriptor */

    pUSBHST_ENDPOINT_DESCRIPTOR pEndpointDesc = NULL;

    /* To hold the pointer to the SynopsysHCD maintained pipe data structure */

    pUSB_SYNOPSYSHCD_PIPE pHCDPipe = NULL;

    /* Pointer to the HCD specific data structure */

    pUSB_SYNOPSYSHCD_DATA  pHCDData = NULL;

    /* To hold the endpoint wMaxPacketSize in CPU endian format */

    UINT16 wMaxPacketSizeCpu = 0;

    /* To hold the maximum packet size for the endpoint */

    UINT32 uMaxPacketSize = 0;

    /* Check the validity of the parameters */

    if ((USB_MAX_SYNOPSYSHCI_COUNT <= uBusIndex) ||
        (USB_SYNOPSYSHCD_MAX_DEVICE_ADDRESS < uDeviceAddress) ||
        (NULL == pEndpointDescriptor) ||
        (NULL == puPipeHandle))
        {
        USB_SHCD_ERR("parameters are not valid\n", 0, 0, 0, 0, 0, 0);

        return USBHST_INVALID_PARAMETER;
        }

    /* Extract the global data structure */

    pHCDData = g_pSynopsysHCDData[uBusIndex];

    /* Judge if the global pointer is not valid */

    if (pHCDData == NULL)
        {
        USB_SHCD_ERR("pHCDData are not valid\n", 0, 0, 0, 0, 0, 0);
        return USBHST_INVALID_PARAMETER;
        }

    /* Check if the request is for the Root hub and route it */

    if (uDeviceAddress == pHCDData->RHData.uDeviceAddress)
        {
        Status = usbSynopsysHcdRhCreatePipe(pHCDData,
                                            uDeviceAddress,
                                            uDeviceSpeed,
                                            pEndpointDescriptor,
                                            puPipeHandle);

        return Status;
        }

    /* Retrieve the endpoint descriptor */

    pEndpointDesc = (pUSBHST_ENDPOINT_DESCRIPTOR)pEndpointDescriptor;

    /* Swap the maximum packet size to get it in CPU endian format */

    wMaxPacketSizeCpu = OS_UINT16_LE_TO_CPU(pEndpointDesc->wMaxPacketSize);

    /* Check the validity of the endpoint type */

    if ((USBHST_ISOCHRONOUS_TRANSFER !=
        (pEndpointDesc->bmAttributes & USB_SYNOPSYSHCD_ENDPOINT_TYPE_MASK)) &&
        (USBHST_INTERRUPT_TRANSFER !=
        (pEndpointDesc->bmAttributes & USB_SYNOPSYSHCD_ENDPOINT_TYPE_MASK)) &&
        (USBHST_CONTROL_TRANSFER !=
        (pEndpointDesc->bmAttributes & USB_SYNOPSYSHCD_ENDPOINT_TYPE_MASK)) &&
        (USBHST_BULK_TRANSFER !=
        (pEndpointDesc->bmAttributes & USB_SYNOPSYSHCD_ENDPOINT_TYPE_MASK)))
        {
        USB_SHCD_ERR("endpoint types is not valid\n", 0, 0, 0, 0, 0, 0);

        return USBHST_INVALID_PARAMETER;
        }

    /* Check if the endpoint type is isochronous or interrupt */

    if ((USBHST_ISOCHRONOUS_TRANSFER ==
        (pEndpointDesc->bmAttributes & USB_SYNOPSYSHCD_ENDPOINT_TYPE_MASK)) ||
        (USBHST_INTERRUPT_TRANSFER ==
        (pEndpointDesc->bmAttributes & USB_SYNOPSYSHCD_ENDPOINT_TYPE_MASK)))
        {
        /*
         * If the device speed is high speed, the payload is
         * equal the maxpacketsize * number of packets that can
         * be sent in a microframe.
         * The bits 0 to 10 give the maximum packet size. The bits 11 and 12
         * give the (number of packets in a microframe - 1).
         */

        if (USBHST_HIGH_SPEED == uDeviceSpeed)
            {
            uMaxPacketSize =
            ((wMaxPacketSizeCpu) &
             USB_SYNOPSYSHCD_ENDPOINT_MAX_PACKET_SIZE_MASK) *
            ((((wMaxPacketSizeCpu) &
               USB_SYNOPSYSHCD_ENDPOINT_NUMBER_OF_TRANSACTIONS_MASK) >> 11) + 1);
            }

        /*
         * If it is low or full speed, the maximum packet size is the same
         * as that retrieved from the endpoint descriptor
         */

        else
            {
            uMaxPacketSize = wMaxPacketSizeCpu;
            }

        }

    /* Allocate memory for the USB_EHCD_PIPE data structure */

    pHCDPipe = usbSynopsysHcdNewPipe();

    /* Check if memory allocation is successful */

    if (NULL == pHCDPipe)
        {
        USB_SHCD_ERR("Memory not allocated for SynopsysHCD pipe\n",
                     0, 0, 0, 0, 0, 0);

        return USBHST_MEMORY_NOT_ALLOCATED;
        }

    /* Update the pointer to the pipe handle */

    *puPipeHandle = (UINT32)pHCDPipe;

    /* Update the endpoint speed */

    pHCDPipe->uSpeed = uDeviceSpeed;

    /* Update the device address which holds the endpoint */

    pHCDPipe->uDeviceAddress = uDeviceAddress;

    /* Update the type of endpoint to be created */

    pHCDPipe->uEndpointType =
    (pEndpointDesc->bmAttributes) & USB_SYNOPSYSHCD_ENDPOINT_TYPE_MASK;

    /* Update the direction of the endpoint */

    pHCDPipe->uEndpointDir =
        (pEndpointDesc->bEndpointAddress & USB_SYNOPSYSHCD_DIR_IN) ? 1 : 0;

    /* Update Endpoint address */

    pHCDPipe->uEndpointAddress = pEndpointDesc->bEndpointAddress;

    pHCDPipe->bInterval = pEndpointDesc->bInterval;

    /*
     * For periodic transfer, bInterval means interval for polling endpoint
     * for data transfers. For non-periodic transfer, bInterval specify the
     * maximum NAK rate of the endpoint. We use this value to decide when
     * to schedule the request on this pipe, so we must set the pipe's bInterval
     * to 0 if the pipe is non-periodic and it's bInterval is bigger than 0.
     */

    if (!(USB_SYNOPSYSHCD_IS_PERIODIC(pHCDPipe)) && (pHCDPipe->bInterval > 0))
        pHCDPipe->bInterval = 0;

    /*
     * Store the maximum packet size - for high speed includes the number
     * of transactions in a microframe also
     */

    pHCDPipe->uMaximumPacketSize = wMaxPacketSizeCpu;
    pHCDPipe->uPidToggle = 0;

    /* Update the high speed hub information */

    pHCDPipe->uHubInfo = uHighSpeedHubInfo;

    /* Add the new pipe into pipe list */

    OS_WAIT_FOR_EVENT(pHCDData->RequestSynchEventID, WAIT_FOREVER);
    lstAdd(&pHCDData->pipeList, &pHCDPipe->pipeNode);
    OS_RELEASE_EVENT(pHCDData->RequestSynchEventID);

    return USBHST_SUCCESS;
    }

/*******************************************************************************
*
* usbSynopsysHcdDeletePipe - delete a pipe created already
*
* This function is used to delete a pipe specific to an endpoint.
*
* RETURNS:  USBHST_STATUS of the delete result.
*
* ERRNO: N/A
*
* \NOMANUAL
*/

USBHST_STATUS usbSynopsysHcdDeletePipe
    (
    UINT8   uBusIndex,   /* Index of the host controller     */
    ULONG   uPipeHandle  /* Handle of the pipe to be deleted */
    )
    {
    /* To hold the pointer to the data structure */

    pUSB_SYNOPSYSHCD_DATA pHCDData = NULL;

    /* Pointer to the HCD maintained pipe */

    pUSB_SYNOPSYSHCD_PIPE pHCDPipe = NULL;

    /* To hold the request information */

    pUSB_SYNOPSYSHCD_REQUEST_INFO  pRequestInfo = NULL;
    NODE *                         pNode = NULL;
    /* To hold the status of the request */

    USBHST_STATUS Status = USBHST_FAILURE;

    /* To hold the status of HCCHAR register */

    UINT32  uUsbHccharReg = 0x0;
    UINT32  uTimeout = 0x0;
    UINT8   uChannel = 0x0;

    /* Pointer to save pURB */

    pUSBHST_URB pURB = NULL;


    /* Check the validity of the parameters */

    if ((USB_MAX_SYNOPSYSHCI_COUNT <= uBusIndex) ||
        (0 == uPipeHandle) )
        {
        USB_SHCD_ERR("parameters are not valid\n",
                     0, 0, 0, 0, 0, 0);
        return USBHST_INVALID_PARAMETER;
        }

    /* Extract the global data structure */

    pHCDData = g_pSynopsysHCDData[uBusIndex];

    /* Judge if the global pointer is not valid */

    if (NULL == pHCDData)
        {
        USB_SHCD_ERR("pHCDData is Null\n",
                     0, 0, 0, 0, 0, 0);
        return USBHST_FAILURE;
        }

    /* Judge if the hardware is ok to be accessed */

    if (0 == pHCDData->initDone)
        {
        USB_SHCD_ERR("Hardware is down\n",
                     0, 0, 0, 0, 0, 0);
        return USBHST_FAILURE;
        }

    OS_WAIT_FOR_EVENT(pHCDData->RequestSynchEventID, WAIT_FOREVER);
	
    /* Extract the USB_SYNOPSYS_DWC_PIPE data structure */

    pHCDPipe = (pUSB_SYNOPSYSHCD_PIPE)uPipeHandle;

    /* Check if the request is for the Root hub and route it */

    if ((pHCDData->RHData.pInterruptPipe == pHCDPipe) ||
        (pHCDData->RHData.pControlPipe == pHCDPipe))
        {
        Status = usbSynopsysHcdRHDeletePipe(pHCDData,
                                           uPipeHandle);
        
        OS_RELEASE_EVENT(pHCDData->RequestSynchEventID);

        return Status;
        }

    /*
     * To make this channel transfer, we may halt some other channels.
     * We must let other channels continue transfer before we delete the pipe.
     */
     
    if (pHCDData->uPlatformType != USB_SYNOPSYSHCI_PLATFORM_ALTERA_SOC_GEN5)
        {
        usbSynopsysHcdUnHaltChannel(pHCDData, pHCDPipe);
        }

    /*
     * How to delete a pipe
     * 1: Update the pipe's flag
     * 2: Remove the pipe from the pipe list
     * 3: Call all the URB's callback routine to notify the upper
     * 4: Check if the headrequest is under transfer or not
     * 5: If not, release all the request on the pipe
     * 6: Or else, set HW to disable the channel and wait for the interrupt
     *    to reclaim the request and pipe
     */

    if (pHCDPipe == pHCDData->pDefaultPipe)
        {
        /*
         * We don't need to modify the default pipe to NULL.
         * As the flag: USB_SYNOPSYSHCD_PIPE_FLAG_MODIFY_DEGAULT_PIPE
         * will make the pHCDPipe reuse again.
         */

        pHCDPipe->uPipeFlag |= USB_SYNOPSYSHCD_PIPE_FLAG_MODIFY_DEFAULT_PIPE;
        }
    else
        {

        if (ERROR != lstFind(&pHCDData->pipeList, &pHCDPipe->pipeNode))
            {
            lstDelete(&pHCDData->pipeList, &pHCDPipe->pipeNode);
            }

        /* Mark the pipe as deleted */

        pHCDPipe->uPipeFlag |= USB_SYNOPSYSHCD_PIPE_FLAG_DELETE;
        }

    /* Call urb's callback to notify upper all the request is cancelled */

    pNode = lstFirst(&pHCDPipe->reqList);
    while (pNode != NULL)
        {
        pRequestInfo = LIST_NODE_TO_USB_SYNOPSYSHCD_REQUEST_INFO(pNode);
        if (pRequestInfo != NULL)
            {
            pURB = pRequestInfo->pUrb;

            pRequestInfo->pUrb = NULL;

            usbSynopsysHcdUrbComplete(pURB, USBHST_TRANSFER_CANCELLED);
            }
        pNode = lstNext(pNode);
        }

    /*
     * Check if the request is still in transfer, disable the pipe
     * wait for the usbSynopsysHcdCompleteProcess to deal with
     * the request list
     */

    /* Read HCCHAR register */

    uChannel = pHCDPipe->uDmaChannel;
    uUsbHccharReg =
         USB_SYNOPSYSHCD_READ32_REG(pHCDData,
                                    USB_SYNOPSYSHCD_HCCHAR(uChannel));
    pRequestInfo = usbSynopsysHcdFirstReqGet(pHCDPipe);
    if(pRequestInfo == NULL)
        {
        USB_SHCD_WARN("Can't get first request 0x%x, 0x%x, %d.\n",
                      pHCDPipe, pHCDData, lstCount(&pHCDPipe->reqList),0,0,0);
        }

    if ((pHCDPipe->uPipeFlag &
        USB_SYNOPSYSHCD_PIPE_FLAG_SCHEDULED) && (pRequestInfo != NULL) &&
        (pRequestInfo->uTransferFlag &
        USB_SYNOPSYSHCD_REQUEST_STILL_TRANSFERING) &&
        (uUsbHccharReg & USB_SYNOPSYSHCD_HCCHAR_CHENA))
        {
        /*
         * NOTE: The following code is for hardware workaround.
         *
         * The datasheet says:
         *
         * The application can disable any channel by setting
         * USBC_HCCHARn[CHDIS,CHENA] both to 1. This enables the USBC to flush
         * the posted requests (if any) and generates a channel-halted interrupt.
         * The application must wait for the USBC_HCINTn[CHHLTD] interrupt before
         * reallocating the channel for other transactions. The USBC does not
         * interrupt the transaction that has been already started on USB.
         *
         * But if we only set the USBC_HCCHARn[CHDIS,CHENA] both to 1. There will
         * be no "CHHLTD" interrupt occur when the device is already pluged out.
         * So the request and the pipe will nerver be deleted, this channel will
         * be nerver re-used.
         *
         * If we re-init this channel, we will get the CHHLTD interrupt as expect.
         * So we copy the init channel code here to workaround this situation.
         *
         * This function will be called frequently when plug in/out the device.
         * In current test, this workarout doesn't raise any side effect. If any,
         * this should be pay attention to.
         */

        uUsbHccharReg = USB_SYNOPSYSHCD_READ32_REG(pHCDData,
                         USB_SYNOPSYSHCD_HCCHAR(uChannel));
        uUsbHccharReg &= ~USB_SYNOPSYSHCD_HCCHAR_CHENA;
        uUsbHccharReg |= USB_SYNOPSYSHCD_HCCHAR_CHDIS;
        uUsbHccharReg &= ~USB_SYNOPSYSHCD_HCCHAR_EPDIR;
        USB_SYNOPSYSHCD_WRITE32_REG(pHCDData,
                                    USB_SYNOPSYSHCD_HCCHAR(uChannel),
                                    uUsbHccharReg);
        uUsbHccharReg = USB_SYNOPSYSHCD_READ32_REG(pHCDData,
                                                   USB_SYNOPSYSHCD_HCCHAR(uChannel));
        uUsbHccharReg |= USB_SYNOPSYSHCD_HCCHAR_CHENA;
        uUsbHccharReg |= USB_SYNOPSYSHCD_HCCHAR_CHDIS;
        uUsbHccharReg &= ~USB_SYNOPSYSHCD_HCCHAR_EPDIR;
        USB_SYNOPSYSHCD_WRITE32_REG(pHCDData,
                                    USB_SYNOPSYSHCD_HCCHAR(uChannel),
                                    uUsbHccharReg);
        /* workround end */

        /* Wait until the channel to be halted or timeout */

        uUsbHccharReg = USB_SYNOPSYSHCD_READ32_REG(pHCDData,
                                                   USB_SYNOPSYSHCD_HCCHAR(uChannel));

        while ((0 != (uUsbHccharReg & USB_SYNOPSYSHCD_HCCHAR_CHENA)) &&
              (uTimeout < 10))
            {
            uTimeout++;
            OS_DELAY_MS(1);

            uUsbHccharReg = USB_SYNOPSYSHCD_READ32_REG(pHCDData,
                                                       USB_SYNOPSYSHCD_HCCHAR(uChannel));
            }

        /*
         * Now, the channel already disabled or timeout
         * If disabled, means the hardware already doesn't use the request any
         * more, we can release the resource.
         * If timeout<sometimes> the hardware may be something wrong.
         * This routine is called when device plugged out, the hardware somtimes
         * doesn't work correctly without device. So we can release the resource.
         */
        }

    /* Release all the request on the pipe */

    usbSynopsysHcdCleanPipe(pHCDData, pHCDPipe);

    /* Release the channel and pipe resource */

    if (pHCDData->pPipeInChannel[pHCDPipe->uDmaChannel] == pHCDPipe)
        {
        pHCDData->pPipeInChannel[pHCDPipe->uDmaChannel] = NULL;

        (void) usbShdrcHsDmaChannelRelease(pHCDData, pHCDPipe->uDmaChannel);
        }

    /*
     * Mark the Default pipe can be used now.
     * But for other pipe,we should release the resource
     */

    if (pHCDPipe == pHCDData->pDefaultPipe)
        {
        pHCDPipe->uPipeFlag = USB_SYNOPSYSHCD_PIPE_FLAG_OPEN;
        }
    else
        {
        usbSynopsysHcdDestroyPipe(pHCDPipe);
        }

    /* Release the event */

    OS_RELEASE_EVENT(pHCDData->RequestSynchEventID);
    return USBHST_SUCCESS;
    }

/*******************************************************************************
*
* usbSynopsysHcdSubmitURB - submit a request to a pipe.
*
* This function is used to submit a request to the pipe. <uBusIndex> specifies
* the host controller bus index. <uPipeHandle> holds the pipe handle. <pURB>
* is the pointer to the URB holding the request details.
*
* RETURNS: USBHST_STATUS of the submit result.
*
* ERRNO: N/A
*
* \NOMANUAL
*/

USBHST_STATUS usbSynopsysHcdSubmitURB
    (
    UINT8        uBusIndex,   /* Index of the host controller */
    ULONG        uPipeHandle, /* Pipe handle */
    pUSBHST_URB  pURB         /* Pointer to the URB */
    )
    {
    /* To hold the pointer to the data structure */

    pUSB_SYNOPSYSHCD_DATA pHCDData = NULL;

    /* Pointer to the HCD maintained pipe */

    pUSB_SYNOPSYSHCD_PIPE pHCDPipe = NULL;

    /* To hold the request information */

    pUSB_SYNOPSYSHCD_REQUEST_INFO  pRequestInfo = NULL;

    /* To hold the status of the request */

    USBHST_STATUS Status = USBHST_FAILURE;

    UINT32                  uUsbGinmsk = 0;

    /* Check the validity of the parameters */

    if ((USB_MAX_SYNOPSYSHCI_COUNT <= uBusIndex) ||
        (0 == uPipeHandle) ||
        (NULL == pURB))
        {
        USB_SHCD_ERR("parameters are not valid\n", 0, 0, 0, 0, 0, 0);
        return USBHST_INVALID_PARAMETER;
        }

    /* Extract the global data structure */

    pHCDData = g_pSynopsysHCDData[uBusIndex];

    /* Judge if the global pointer is not valid */

    if (pHCDData == NULL)
        {
        USB_SHCD_ERR("pHCDData are not valid\n", 0, 0, 0, 0, 0, 0);
        return USBHST_INVALID_PARAMETER;
        }

    /* Exclusively access the list */

    OS_WAIT_FOR_EVENT(pHCDData->RequestSynchEventID, OS_WAIT_INFINITE);

    /* Extract the USB_SYNOPSYS_DWC_PIPE data structure */

    pHCDPipe = (pUSB_SYNOPSYSHCD_PIPE)uPipeHandle;

    if (pHCDPipe->uPipeFlag & USB_SYNOPSYSHCD_PIPE_FLAG_DELETE)
        {
        USB_SHCD_ERR("ERROR, the pipe will be deleted\n",
                     0, 0, 0, 0, 0, 0);

        /* Release the exclusive access */

        OS_RELEASE_EVENT(pHCDData->RequestSynchEventID);

        return USBHST_INVALID_PARAMETER;
        }

    /* Release the exclusive access */

    OS_RELEASE_EVENT(pHCDData->RequestSynchEventID);

    /* Check if the request is for the Root hub and route it */

    if ((pHCDData->RHData.pInterruptPipe == pHCDPipe) ||
          (pHCDData->RHData.pControlPipe == pHCDPipe) ||
          ((pHCDData->pDefaultPipe == pHCDPipe) &&
           (0 == pHCDData->RHData.uDeviceAddress)))
        {
        Status = usbSynopsysHcdRHSubmitURB(pHCDData,
                                           uPipeHandle,
                                           pURB);


        return Status;
        }

    /*
     * This dirver is driven by SOF interrupt. If there is no SOF interrupt,
     * all requests cann't be scheduled so no request should be added to the
     * schedule list.
     * This may happen when dealing with the device detachment.
     */

    if (pHCDData->uMicroFrameNumChangeFlag == 0)
        {
        USB_SHCD_ERR("ERROR, there is no SOF interrupt for schedule the URB\n",
                     0, 0, 0, 0, 0, 0);
        return USBHST_FAILURE;
        }

    /*
     * It is of the assumption that the USBD always gives a valid pipe handle.
     * This also includes not giving a submit URB request while the pipe
     * is being deleted.
     */

    /* Exclusively access the list */

    OS_WAIT_FOR_EVENT(pHCDData->RequestSynchEventID, OS_WAIT_INFINITE);

    /* Allocate the request information structure */

    pRequestInfo = usbSynopsysHcdCreateReq(pHCDPipe);

    /* Check if memory allocation is successful */

    if (NULL == pRequestInfo)
        {
        USB_SHCD_ERR("Memory not allocated for the request info\n",
                     0, 0, 0, 0, 0, 0);

        /* Release the event */

        OS_RELEASE_EVENT(pHCDData->RequestSynchEventID);

        return USBHST_MEMORY_NOT_ALLOCATED;
        }

    /* Copy the URB pointer */

    pRequestInfo->pUrb = pURB;

    /* Store the USB_SYNOPSYSHCD_PIPE pointer */

    pRequestInfo->pHCDPipe = pHCDPipe;

    /* Store the pUSB_SYNOPSYSHCD_DATA pointer */

    pRequestInfo->pHCDData = pHCDData;

    /* Store the URB's total packet count */

    pRequestInfo->uUrbTotalPacketCount = pURB->uNumberOfPackets;

    pURB->pHcdSpecific = (void *)pRequestInfo;

    if (pHCDData->pDefaultPipe == pHCDPipe)
        {
        pRequestInfo->uStage = USB_SYNOPSYSHCD_REQUEST_STAGE_SETUP;
        }

    if (pHCDPipe->uEndpointType == USB_ATTR_CONTROL)
        {
        pRequestInfo->uStage = USB_SYNOPSYSHCD_REQUEST_STAGE_SETUP;
        }
    pRequestInfo->uXferSize = pRequestInfo->pUrb->uTransferLength;

    /* Read micro frame number */

    (void) usbSynopsysHcdGetFrameNumber(pHCDData->uBusIndex,
                                 &pHCDPipe->uMicroFrameNumberLast);
    pHCDPipe->uMicroFrameNumberNext =
        (pHCDPipe->uMicroFrameNumberLast + pHCDPipe->bInterval) %
                                 USB_SYNOPSYSHCD_FRAME_NUM_MAX;

    /* Store the Full Frame number */

    pHCDPipe->uFullFrameNumberLast =
        USB_SYNOPSYSHCD_GET_FULL_FRAME_NUMBER(pHCDPipe->uMicroFrameNumberLast);
    pHCDPipe->uFullFrameNumberNext =
        (pHCDPipe->uFullFrameNumberLast + pHCDPipe->bInterval)%
                                USB_SYNOPSYSHCD_FULL_FRAME_NUM_MAX;

    /*
     * Add request into Sending list. We depend on the SOF interrupt to
     * Schedule the transfer. Now we only add it to Sending List, dont't
     * Start the transfer.
     */

    usbSynopsysHcdAddRequestIntoSendList(pHCDData,
                                         pHCDPipe,
                                         pRequestInfo);

 	/* OS_RELEASE_EVENT(pHCDData->RequestSynchEventID); */ /* move to back*/
    /*
     * The SOF interrupt will mask itself, see usbSynopsysHcdInterruptHandler
     * So when a new request is coming,if the SOF is disable we should
     * reopen it
     */

    uUsbGinmsk = USB_SYNOPSYSHCD_READ32_REG(pHCDData,
                                           USB_SYNOPSYSHCD_GINTMSK);
    if (0 == (uUsbGinmsk & USB_SYNOPSYSHCD_GINTSTS_SOF))
        {
        uUsbGinmsk |= (USB_SYNOPSYSHCD_GINTSTS_SOF);
        USB_SYNOPSYSHCD_WRITE32_REG(pHCDData,
                                    USB_SYNOPSYSHCD_GINTMSK,
                                    uUsbGinmsk);
        }  
	OS_RELEASE_EVENT(pHCDData->RequestSynchEventID);  /* move to here*/

    return USBHST_SUCCESS;
    }

/*******************************************************************************
*
* usbSynopsysHcdCancelURB - cancel a request to a pipe
*
* This function is used to cancel a request to the pipe. <uBusIndex> specifies
* the host controller bus index. <uPipeHandle> holds the pipe handle. <pURB>
* pointer to the URB holding the request details.
*
* RETURNS: USBHST_STATUS of the cancel result.
*
* ERRNO: N/A
*
* \NOMANUAL
*/

USBHST_STATUS usbSynopsysHcdCancelURB
    (
    UINT8       uBusIndex,   /* Index of the host controller */
    ULONG       uPipeHandle, /* Pipe handle */
    pUSBHST_URB pURB         /* Pointer to the URB */
    )
    {
    /* To hold the pointer to the data structure */

    pUSB_SYNOPSYSHCD_DATA pHCDData = NULL;

    /* Pointer to the HCD maintained pipe */

    pUSB_SYNOPSYSHCD_PIPE pHCDPipe = NULL;

    /* To hold the request information */

    pUSB_SYNOPSYSHCD_REQUEST_INFO  pRequest = NULL;

    /* To hold the status of HCCHAR register */

    UINT32 uUsbHccharReg = 0;

    /* To hold the status of the request */

    USBHST_STATUS Status = USBHST_FAILURE;

    /* Check the validity of the parameters */

    if ((USB_MAX_SYNOPSYSHCI_COUNT <= uBusIndex) ||
        (0 == uPipeHandle) ||
        (NULL == pURB))
        {
        USB_SHCD_ERR("parameters are not valid\n",
                     0, 0, 0, 0, 0, 0);
        return USBHST_INVALID_PARAMETER;
        }

    /* Extract the global data structure */

    pHCDData = g_pSynopsysHCDData[uBusIndex];

    /* Judge if the global pointer is not valid */

    if (NULL == pHCDData)
        {
        USB_SHCD_ERR("pHCDData is Null\n",
                      0, 0, 0, 0, 0, 0);
        return USBHST_FAILURE;
        }

    /* Judge if the hardware is ok to be accessed */

    if (0 == pHCDData->initDone)
        {
        USB_SHCD_ERR("Hardware is down\n",
                     0, 0, 0, 0, 0, 0);
        return USBHST_FAILURE;
        }

    USB_SHCD_WARN("usbSynopsysHcdCancelURB entered.\n", 0, 0, 0, 0, 0, 0);

    OS_WAIT_FOR_EVENT(pHCDData->RequestSynchEventID, WAIT_FOREVER);

    /* Extract the USB_SYNOPSYS_DWC_PIPE data structure */

    pHCDPipe = (pUSB_SYNOPSYSHCD_PIPE)uPipeHandle;

    /* Check if the request is for the Root hub and route it */

    if (pHCDData->RHData.pInterruptPipe == pHCDPipe)
        {
        Status = usbSynopsysHcdRHCancelURB(pHCDData,
                                           uPipeHandle,
                                           pURB);

        /* Release the exclusive access */

        OS_RELEASE_EVENT(pHCDData->RequestSynchEventID);

        return Status;
        }
    /*
     * How to cancel a URB
     * 1: Get the request related to the urb, check if the urb already
     *    cancelled.
     * 2: If not, set URB's status as cancel and call callback to notify
     *    upper layer.
     * 3: Check if the request is still under transfer
     * 4: If not, release the request
     * 5: Or else, set hardware to disable the channel wait complete process
     *    to reclaim the resource
     */

    pRequest = pURB->pHcdSpecific;
    if ((pRequest == NULL) || (pRequest->pUrb != pURB))
        {
        USB_SHCD_WARN("This URB already canceled\n", 1, 2, 3, 4, 5, 6);

        OS_RELEASE_EVENT(pHCDData->RequestSynchEventID);

        return USBHST_INVALID_REQUEST;
        }

    /* Judge the request is still under transfer or not */

    /* Read HCCHAR register */

    uUsbHccharReg = USB_SYNOPSYSHCD_READ32_REG(pHCDData,
                    USB_SYNOPSYSHCD_HCCHAR(pHCDPipe->uDmaChannel));

    if ((pRequest->uTransferFlag &
         USB_SYNOPSYSHCD_REQUEST_STILL_TRANSFERING)&&
        (uUsbHccharReg & USB_SYNOPSYSHCD_HCCHAR_CHENA))
        {
        /* Release the channel and pipe resource */

        usbSynopysHcdDeleteRequestFromScheduleList(pHCDData,
                                                   pHCDPipe,
                                                   pRequest);

        pRequest->uTransferFlag &= ~USB_SYNOPSYSHCD_REQUEST_STILL_TRANSFERING;

        /* Set CHENA,CHDIS both to "1" to halt channel to disable the transfer */

        uUsbHccharReg |= USB_SYNOPSYSHCD_HCCHAR_CHENA;
        uUsbHccharReg |= USB_SYNOPSYSHCD_HCCHAR_CHDIS;
        USB_SYNOPSYSHCD_WRITE32_REG(pHCDData,
                                    USB_SYNOPSYSHCD_HCCHAR(pHCDPipe->uDmaChannel),
                                    uUsbHccharReg);

        /* Unlink request from pipe reqList */

        usbSynopsysHcdUnlinkReq(pHCDPipe, pRequest);

        /* Set request's transfer flag as canceled */

        pRequest->uTransferFlag = USB_SYNOPSYSHCD_REQUEST_TO_BE_CANCELED;

        /* Prevent to call urb's callback routine twice */

        pRequest->pUrb = NULL;

        /* Set urb's status as cancelled, call callback function to notify upper */

        usbSynopsysHcdUrbComplete(pURB, USBHST_TRANSFER_CANCELLED);

        usbSynopsysHcdDeleteReq(pHCDPipe, pRequest);

        pHCDPipe->uPipeFlag &= ~USB_SYNOPSYSHCD_PIPE_FLAG_SCHEDULED;

        (void) usbShdrcHsDmaChannelRelease(pHCDData, pHCDPipe->uDmaChannel);


        /*
         * Wait "CHHLTD" interrupt to free the request's resource
         * It will be done at "usbSynopsysHcdChannelInterruptHandler"
         */

        OS_RELEASE_EVENT(pHCDData->RequestSynchEventID);

        return USBHST_SUCCESS;
        }

    /* Unlink request from pipe reqList */

    usbSynopsysHcdUnlinkReq(pHCDPipe, pRequest);

    /* Prevent to call urb's callback routine twice */

    pRequest->pUrb = NULL;

    /* Set urb's status as cancelled, call callback function to notify upper */

    usbSynopsysHcdUrbComplete(pURB, USBHST_TRANSFER_CANCELLED);

    /* Remove the requestinfo from the schedule list */

    usbSynopysHcdDeleteRequestFromScheduleList(pHCDData,
                                               pHCDPipe,
                                               pRequest);

    /* Release the channel and pipe resource */

    pRequest->uTransferFlag &= ~USB_SYNOPSYSHCD_REQUEST_STILL_TRANSFERING;

    usbSynopsysHcdDeleteReq(pHCDPipe, pRequest);

    pHCDPipe->uPipeFlag &= ~USB_SYNOPSYSHCD_PIPE_FLAG_SCHEDULED;

    if (pHCDData->pPipeInChannel[pHCDPipe->uDmaChannel] == pHCDPipe)
        {
        pHCDData->pPipeInChannel[pHCDPipe->uDmaChannel] = NULL;

        (void) usbShdrcHsDmaChannelRelease(pHCDData, pHCDPipe->uDmaChannel);
        }

    OS_RELEASE_EVENT(pHCDData->RequestSynchEventID);

    return USBHST_SUCCESS;
    }

/*******************************************************************************
*
* usbSynopsysHcdModifyDefaultPipe - modify the default pipe characteristics
*
* This function is used to modify the properties (device speed and maximum
* packet size) of the default pipe (address 0, endpoint 0). <uBusIndex>
* specifies the host controller bus index. <uDefaultPipeHandle> holds the
* pipe handle of the default pipe. <uDeviceSpeed> is the speed of the
* device which needs to be modified. <uMaxPacketSize> is the maximum packet
* size of the default pipe which needs to be modified. <uHighSpeedHubInfo>
* specifies the nearest high speed hub and the port number information. This
* information will be used to handle a split transfer to the full / low speed
* device. The high byte will hold the high speed hub address. The low byte
* will hold the port number to which the USB 1.1 device is connected.
*
* RETURNS: USBHST_STATUS of the modify result.
*
* ERRNO: N/A
*
* \NOMANUAL
*/

USBHST_STATUS usbSynopsysHcdModifyDefaultPipe
    (
    UINT8   uBusIndex,          /* Host controller bus index               */
    ULONG   uDefaultPipeHandle, /* Handle to the default pipe              */
    UINT8   uDeviceSpeed,       /* Speed of the device in default state    */
    UINT8   uMaxPacketSize,     /* Maximum packet size of the default pipe */
    UINT16  uHighSpeedHubInfo   /* High speed hub info for USB 1.1 device  */
    )
    {

    /* To hold the pointer to the pipe data structure */

    pUSB_SYNOPSYSHCD_PIPE pHCDPipe = NULL;

    /* Pointer to the HCD specific data structure */

    pUSB_SYNOPSYSHCD_DATA  pHCDData = NULL;

    /* To hold the request information */

    pUSB_SYNOPSYSHCD_REQUEST_INFO  pRequestInfo = NULL;

    /* To hold the status of HCCHAR register */

    UINT32  uUsbHccharReg = 0x0;
    NODE *  pNode = NULL;

    /* Pointer to save pURB */

    pUSBHST_URB pURB = NULL;

    /* Check the validity of the parameters */
    /* NOTE: the speed need translate to the SynopsysHcd speed */

    if ((USB_MAX_SYNOPSYSHCI_COUNT <= uBusIndex) ||
        (0 == uDefaultPipeHandle) ||
        (USBHST_HIGH_SPEED != uDeviceSpeed &&
         USBHST_FULL_SPEED != uDeviceSpeed &&
         USBHST_LOW_SPEED != uDeviceSpeed))
        {
        USB_SHCD_ERR("parameters not valid\n", 0, 0, 0, 0, 0, 0);
        return USBHST_INVALID_PARAMETER;
        }

    /* Extract the global data structure */

    pHCDData = g_pSynopsysHCDData[uBusIndex];

    /* Judge if the global pointer is not valid */

    if (pHCDData == NULL)
        {
        USB_SHCD_ERR("pHCDData are not valid\n", 0, 0, 0, 0, 0, 0);
        return USBHST_INVALID_PARAMETER;
        }

    /* Check if this request is for the Root hub */

    if (0 == pHCDData->RHData.uDeviceAddress)
        {
        return USBHST_SUCCESS;
        }

    /* Exclusively access the list */

    OS_WAIT_FOR_EVENT(pHCDData->RequestSynchEventID, WAIT_FOREVER);

    /* Extract the default pipe handle */

    pHCDPipe = (pUSB_SYNOPSYSHCD_PIPE)uDefaultPipeHandle;

    /* Call urb's callback to notify upper all the request is cancelled */

    pNode = lstFirst(&pHCDPipe->reqList);
    while (pNode != NULL)
        {
        pRequestInfo = LIST_NODE_TO_USB_SYNOPSYSHCD_REQUEST_INFO(pNode);
        if (pRequestInfo != NULL)
            {
            pURB = pRequestInfo->pUrb;

            pRequestInfo->pUrb = NULL;

            usbSynopsysHcdUrbComplete(pURB, USBHST_TRANSFER_CANCELLED);
            }
        pNode = lstNext(pNode);
        }

    if ((pHCDPipe->uSpeed != uDeviceSpeed) ||
        (pHCDPipe->uMaximumPacketSize != uMaxPacketSize) ||
        (pHCDPipe->uHubInfo != uHighSpeedHubInfo))
        {
        /* We check the first request flags */

        pRequestInfo = usbSynopsysHcdFirstReqGet(pHCDPipe);

        /* Check the pRequestInfo exist or not */

        if (pRequestInfo != NULL)
            {
            if ((pHCDPipe->uPipeFlag &
                 USB_SYNOPSYSHCD_PIPE_FLAG_SCHEDULED)    &&
                (pRequestInfo->uTransferFlag &
                 USB_SYNOPSYSHCD_REQUEST_STILL_TRANSFERING))
                {
                uUsbHccharReg = USB_SYNOPSYSHCD_READ32_REG(pHCDData,
                                  USB_SYNOPSYSHCD_HCCHAR(pHCDPipe->uDmaChannel));

                if (uUsbHccharReg & USB_SYNOPSYSHCD_HCCHAR_CHENA)
                    {
                    /* Set CHDIS to disable the transfer */

                    uUsbHccharReg |= USB_SYNOPSYSHCD_HCCHAR_CHDIS;
                    USB_SYNOPSYSHCD_WRITE32_REG (pHCDData,
                                      USB_SYNOPSYSHCD_HCCHAR(pHCDPipe->uDmaChannel),
                                      uUsbHccharReg);
                    }

                pRequestInfo->uTransferFlag =
                                            USB_SYNOPSYSHCD_REQUEST_TO_BE_CANCELED;

                /* The follow is wait for the interrupt to delete the request*/

                pHCDPipe->uPipeFlag = USB_SYNOPSYSHCD_PIPE_FLAG_MODIFY_DEFAULT_PIPE;

                }
            else
                {
                /* Release all the request on the pipe */

                usbSynopsysHcdCleanPipe(pHCDData, pHCDPipe);

                /* Modify pipe flag as can use it again */

                pHCDPipe->uPipeFlag = USB_SYNOPSYSHCD_PIPE_FLAG_OPEN;
                }
            }
        }

    /* Set default pipe's attribute */

    pHCDPipe->uMaximumPacketSize = uMaxPacketSize;
    pHCDPipe->uSpeed             = uDeviceSpeed;
    pHCDPipe->uHubInfo           = uHighSpeedHubInfo;
    pHCDPipe->bInterval          = 0;
    pHCDPipe->uEndpointAddress   = 0;
    pHCDPipe->uEndpointType      = USB_ATTR_CONTROL;

    /* Get current micro frame number */

    (void) usbSynopsysHcdGetFrameNumber(pHCDData->uBusIndex,
                                 &pHCDPipe->uMicroFrameNumberLast);
    pHCDPipe->uMicroFrameNumberNext = (pHCDPipe->uMicroFrameNumberLast ) %
                                       USB_SYNOPSYSHCD_FRAME_NUM_MAX;


    /* Release the exclusive list access */

    OS_RELEASE_EVENT(pHCDData->RequestSynchEventID);

    return USBHST_SUCCESS;
    }

/*******************************************************************************
*
* usbSynopsysHcdIsRequestPending - check if a request is pending for a pipe
*
* This function is used to check whether any request is pending on a pipe.
* <uBusIndex> Specifies the host controller bus index. <uPipeHandle> holds the
* pipe handle.
*
* RETURNS: USBHST_STATUS of the check result.
*
* ERRNO: N/A
*
* \NOMANUAL
*/

USBHST_STATUS usbSynopsysHcdIsRequestPending
    (
    UINT8    uBusIndex,   /* Index of the host controller */
    ULONG    uPipeHandle  /* Pipe handle */
    )
    {
    /* To hold the pointer to the pipe data structure */

    pUSB_SYNOPSYSHCD_PIPE pHCDPipe = NULL;

    /* Pointer to the HCD specific data structure */

    pUSB_SYNOPSYSHCD_DATA  pHCDData = NULL;

    USB_SHCD_DBG("usbSynopsysHcdIsRequestPending - Entry\n",
                 0, 0, 0, 0, 0, 0);

    /* Check the validity of the parameters */

    if ((USB_MAX_SYNOPSYSHCI_COUNT <= uBusIndex) ||
        (0 == uPipeHandle))
        {
        USB_SHCD_ERR("usbSynopsysHcdIsRequestPending - parameters are not valid\n",
                     0, 0, 0, 0, 0, 0);

        return USBHST_INVALID_PARAMETER;
        }

    /* Extract the global data structure */

    pHCDData = g_pSynopsysHCDData[uBusIndex];

    /* Judge if the global pointer is not valid */

    if (pHCDData == NULL)
        {
        USB_SHCD_ERR("pHCDData are not valid\n", 0, 0, 0, 0, 0, 0);
        return USBHST_INVALID_PARAMETER;
        }

    pHCDPipe = (pUSB_SYNOPSYSHCD_PIPE)uPipeHandle;

    /* Exclusively access the list */

    OS_WAIT_FOR_EVENT(pHCDData->RequestSynchEventID, WAIT_FOREVER);

    /* Check if any request is pending for the endpoint */

    if (NULL == usbSynopsysHcdFirstReqGet(pHCDPipe))
        {
        OS_RELEASE_EVENT(pHCDData->RequestSynchEventID);
        return USBHST_FAILURE;
        }

    /* Return success indicating that a request is pending for the pipe */

    OS_RELEASE_EVENT(pHCDData->RequestSynchEventID);
    return USBHST_SUCCESS;
    }


/* End of file */
