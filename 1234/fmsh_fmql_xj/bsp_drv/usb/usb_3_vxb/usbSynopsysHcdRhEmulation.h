/* usbSynopsysHcdRhEmulation.h - root hub emulation entries for Synopsys HCD */

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
01a,04aug09,m_y  written.
*/

/*
DESCRIPTION

This contains the data structures and prototypes of
functions of the root hub emulation module used by
the Synopsys USB Host Controller Driver.

*/
#ifndef __INCSynopsysHcdRHEmulationh
#define __INCSynopsysHcdRHEmulationh

#ifdef    __cplusplus
extern "C" {
#endif

/* defines */

/* Class specific values */

#define USB_SYNOPSYSHCD_RH_C_HUB_LOCAL_POWER       (0x00)
#define USB_SYNOPSYSHCD_RH_C_HUB_OVER_CURRENT      (0x01)

#define USB_SYNOPSYSHCD_RH_PORT_CONNECTION         (0x00)
#define USB_SYNOPSYSHCD_RH_PORT_ENABLE             (0x01)
#define USB_SYNOPSYSHCD_RH_PORT_SUSPEND            (0x02)
#define USB_SYNOPSYSHCD_RH_PORT_OVER_CURRENT       (0x03)
#define USB_SYNOPSYSHCD_RH_PORT_RESET              (0x04)
#define USB_SYNOPSYSHCD_RH_PORT_POWER              (0x08)
#define USB_SYNOPSYSHCD_RH_PORT_LOW_SPEED          (0x09)
#define USB_SYNOPSYSHCD_RH_PORT_HIGH_SPEED         (0x0A)
#define USB_SYNOPSYSHCD_RH_C_PORT_CONNECTION       (0x10)
#define USB_SYNOPSYSHCD_RH_C_PORT_ENABLE           (0x11)
#define USB_SYNOPSYSHCD_RH_C_PORT_SUSPEND          (0x12)
#define USB_SYNOPSYSHCD_RH_C_PORT_OVER_CURRENT     (0x13)
#define USB_SYNOPSYSHCD_RH_C_PORT_RESET            (0x14)

#define USB_SYNOPSYSHCD_RH_PORT_CHANGE_CONNECTION       (0x0)
#define USB_SYNOPSYSHCD_RH_PORT_CHANGE_ENABLE           (0x1)
#define USB_SYNOPSYSHCD_RH_PORT_CHANGE_SUSPEND          (0x2)
#define USB_SYNOPSYSHCD_RH_PORT_CHANGE_OVER_CURRENT     (0x3)
#define USB_SYNOPSYSHCD_RH_PORT_CHANGE_RESET            (0x4)
#define USB_SYNOPSYSHCD_RH_PORT_TEST                    (0x15)
#define USB_SYNOPSYSHCD_RH_PORT_INDICATOR               (0x16)


#define USB_SYNOPSYSHCD_RH_STANDARD_REQUEST         (0x00)
#define USB_SYNOPSYSHCD_RH_CLASS_SPECIFIC_REQUEST   (0x20)
#define USB_SYNOPSYSHCD_RH_REQUEST_TYPE             (0x60)
#define USB_SYNOPSYSHCD_RH_RECIPIENT_MASK           (0x1F)

#define USB_SYNOPSYSHCD_RH_GET_CONFIG_SIZE          (0x01)
#define USB_SYNOPSYSHCD_RH_DESCRIPTOR_BITPOSITION   (0x08)
#define USB_SYNOPSYSHCD_RH_DEVICE_DESC_SIZE         (0x12)
#define USB_SYNOPSYSHCD_RH_CONFIG_DESC_SIZE         (0x19)
#define USB_SYNOPSYSHCD_RH_GET_STATUS_SIZE          (0x02)

/* USB 2.0 spec */

#define USB_SYNOPSYSHCD_PORT_CONNECTION	        (0x01 << 0)
#define USB_SYNOPSYSHCD_PORT_ENABLE		        (0x01 << 1)
#define USB_SYNOPSYSHCD_PORT_SUSPEND		    (0x01 << 2)
#define USB_SYNOPSYSHCD_PORT_OVER_CURRENT	    (0x01 << 3)
#define USB_SYNOPSYSHCD_PORT_RESET		        (0x01 << 4)
#define USB_SYNOPSYSHCD_PORT_POWER		        (0x01 << 8)
#define USB_SYNOPSYSHCD_PORT_LOWSPEED		    (0x01 << 9)
#define USB_SYNOPSYSHCD_PORT_HIGHSPEED		    (0x01 << 10)
#define USB_SYNOPSYSHCD_PORT_C_CONNECTION	    (0x01 << 16)
#define USB_SYNOPSYSHCD_PORT_C_ENABLE		    (0x01 << 17)
#define USB_SYNOPSYSHCD_PORT_C_SUSPEND		    (0x01 << 18)
#define USB_SYNOPSYSHCD_PORT_C_OVER_CURRENT	    (0x01 << 19)
#define USB_SYNOPSYSHCD_PORT_C_RESET		    (0x01 << 20)
#define USB_SYNOPSYSHCD_PORT_TEST               (0x01 << 21)
#define USB_SYNOPSYSHCD_PORT_INDICATOR          (0x01 << 22)

/* Add it for Synopsys HCD */

#define USB_SYNOPSYSHCD_RH_MASK_VALUE               (0x02)

#define USB_SYNOPSYSHCD_RH_PORT_SUSPEND_CHANGE  (USB_SYNOPSYSHCD_PORT_C_SUSPEND)
#define USB_SYNOPSYSHCD_RH_PORT_RESET_CHANGE    (USB_SYNOPSYSHCD_PORT_C_RESET)
#define USB_SYNOPSYSHCD_RH_PORT_SUSPEND_MASK    (USB_SYNOPSYSHCD_PORT_SUSPEND)

IMPORT USBHST_STATUS usbSynopsysHcdRhCreatePipe
    (
    pUSB_SYNOPSYSHCD_DATA pHCDData,
    UINT8                 uDeviceAddress,
    UINT8                 uDeviceSpeed,
    UCHAR *               pEndpointDescriptor,
    ULONG *               puPipeHandle
    );

IMPORT USBHST_STATUS usbSynopsysHcdRHDeletePipe
    (
    pUSB_SYNOPSYSHCD_DATA pHCDData,
    ULONG                 uPipeHandle
    );

IMPORT USBHST_STATUS usbSynopsysHcdRHSubmitURB
    (
    pUSB_SYNOPSYSHCD_DATA pHCDData,
    ULONG                 uPipeHandle,
    pUSBHST_URB           pURB
    );

IMPORT USBHST_STATUS usbSynopsysHcdRHCancelURB
    (
    pUSB_SYNOPSYSHCD_DATA pHCDData,
    ULONG                 uPipeHandle,
    pUSBHST_URB           pURB
    );

#ifdef    __cplusplus
}
#endif

#endif /* __INCSynopsysHcdRHEmulationh */

/* End of file */
