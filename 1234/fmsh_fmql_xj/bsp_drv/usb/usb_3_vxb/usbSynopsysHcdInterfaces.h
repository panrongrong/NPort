/* usbSynopsysHcdInterfaces.h - interfaces registered with USBD */

/*
 * Copyright (c) 2009-2010 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify, or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01b,08sep10,m_y  modify to remove build warning
01a,20jul09,m_y  written.
*/

/*
DESCRIPTION

This contains the interfaces which are registered with
the USBD.

*/

#ifndef __INCSynopsysHcdInterfacesh
#define __INCSynopsysHcdInterfacesh

#ifdef	__cplusplus
extern "C" {
#endif

IMPORT USBHST_STATUS  usbSynopsysHcdCreatePipe
    (
    UINT8     uBusIndex,
    UINT8     uDeviceAddress,
    UINT8     uDeviceSpeed,
    UCHAR   * pEndpointDescriptor,
    UINT16    uHighSpeedHubInfo,
    ULONG   * puPipeHandle
    );

IMPORT USBHST_STATUS  usbSynopsysHcdDeletePipe
    (
    UINT8   uBusIndex,
    ULONG   uPipeHandle
    );

IMPORT USBHST_STATUS  usbSynopsysHcdModifyDefaultPipe
    (
    UINT8   uBusIndex,
    ULONG   uDefaultPipeHandle,
    UINT8   uDeviceSpeed,
    UINT8   uMaxPacketSize,
    UINT16  uHighSpeedHubInfo
    );

IMPORT USBHST_STATUS  usbSynopsysHcdIsBandwidthAvailable
    (
    UINT8    uBusIndex,
    UINT8    uDeviceAddress,
    UINT8    uDeviceSpeed,
    UCHAR  * pCurrentDescriptor,
    UCHAR  * pNewDescriptor
    );

IMPORT USBHST_STATUS usbSynopsysHcdSubmitURB
    (
    UINT8       uBusIndex,
    ULONG       uPipeHandle,
    pUSBHST_URB pURB
    );

IMPORT USBHST_STATUS usbSynopsysHcdCancelURB
    (
    UINT8       uBusIndex,
    ULONG       uPipeHandle,
    pUSBHST_URB pURB
    );

IMPORT USBHST_STATUS  usbSynopsysHcdIsRequestPending
    (
    UINT8    uBusIndex,
    ULONG    uPipeHandle
    );

IMPORT USBHST_STATUS usbSynopsysHcdGetFrameNumber
    (
    UINT8     uBusIndex,
    UINT16  * puFrameNumber
    );

IMPORT USBHST_STATUS usbSynopsysHcdSetBitRate
    (
    UINT8     uBusIndex,
    BOOL      bIncrement,
    UINT32  * puCurrentFrameWidth
    );


IMPORT USBHST_STATUS   usbSynopsysHcdResetTTRequestComplete
    (
    UINT8         uRelativeBusIndex,
    VOID *        pContext,
    USBHST_STATUS nStatus
    );

IMPORT USBHST_STATUS   usbSynopsysHcdClearTTRequestComplete
    (
    UINT8         uRelativeBusIndex,
    VOID *        pContext,
    USBHST_STATUS nStatus
    );

#ifdef	__cplusplus
}
#endif

#endif /* __INCSynopsysHcdInterfacesh */

/* End of file */
