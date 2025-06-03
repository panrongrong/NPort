/* usbSynopsysHcdUtil.h - utility Functions for Synopsys HCD */

/*
 * Copyright (c) 2009-2011, 2014 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify, or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01f,09sep14,wyy  unlink request before callback (VXW6-83137)
01e,02sep11,m_y  using lstLib replace the old list structure
01d,23aug11,m_y  add routines usbSynopsysHcdHaltChannel and 
                 usbSynopsysHcdUnHaltChannel (WIND00289879)
01c,05aug11,m_y  add routine usbSynopsysHcdUrbComplete and 
                 usbSynopsysHcdCleanPipe (WIND00290062)
01b,17mar10,m_y  modify the prototype of usbSynopsysGetTransferLength
                 (WIND00202672).
01a,10nov09,m_y  written.
*/

/*
DESCRIPTION

This contains the prototypes of the utility functions
which are used by the Synopsys USB host controller Driver.

*/

#ifndef __INCSynopsysHcdUtilh
#define __INCSynopsysHcdUtilh

#ifdef	__cplusplus
extern "C" {
#endif

#include <ffsLib.h>

/* This macro saves the current transfer toggle */

#define USB_SYNOPSYSHCD_SAVE_DATA_TOGGLE(pHCDData,pHCDPipe,uChannel)          \
    {                                                                         \
    if (0 == (USB_SYNOPSYSHCD_HCTSIZ_PID &                                    \
        USB_SYNOPSYSHCD_READ32_REG(pHCDData,USB_SYNOPSYSHCD_HCTSIZ(uChannel)) )) \
        pHCDPipe->uPidToggle = 0;                                             \
    else                                                                      \
        pHCDPipe->uPidToggle = 1;                                             \
    }                                                                         \

/* This macro gets current transfer toggle */

#define USB_SYNOPSYSHCD_GET_DATA_TOGGLE(pHCDPipe)                             \
        ((pHCDPipe->uPidToggle) ? 2 : 0)

/* This macro returns the full frame number according the micro frame number */

#define USB_SYNOPSYSHCD_GET_FULL_FRAME_NUMBER(uMicroFrameNum)                  \
        ((uMicroFrameNum >> 3) % USB_SYNOPSYSHCD_FULL_FRAME_NUM_MAX)

/* This macro judges the pipe is periodic or not */

#define USB_SYNOPSYSHCD_IS_PERIODIC(pHCDPipe)                                 \
        ((pHCDPipe->uEndpointType == USB_ATTR_INTERRUPT) ||                   \
         (pHCDPipe->uEndpointType == USB_ATTR_ISOCH))

/* This macro judges the pipe need split or not */

#define USB_SYNOPSYSHCD_SHOULD_SPLIT(pHCDData,pHCDPipe)                       \
    (((pHCDPipe->uSpeed != USBHST_HIGH_SPEED) &&                              \
      (USB_SYNOPSYSHCD_HPRT_PRTSPD_HIGH ==                                    \
       (USB_SYNOPSYSHCD_READ32_REG(pHCDData,USB_SYNOPSYSHCD_HPRT) &           \
        USB_SYNOPSYSHCD_HPRT_PRTSPD))) ? TRUE : FALSE)                           \

/* Get the first idle dma channel from the channel map */

#define USB_SYNOPSYSHCD_GET_FIRST_IDLE_DMACHANNEL(uIdleDmaChannelMap)         \
    (ffsLsb(uIdleDmaChannelMap)-1)

/* This macro indicates to find the first set bit */

#define USB_SYNOPSYSHCD_FIND_FIRST_SET_BIT(uValue)                            \
    (ffsLsb(uValue)-1)

#define USB_SYNOPSYSHCD_PIPE_HALT_VALUE    (10)

UINT32 usbSynopsysGetTransferLength
    (
    pUSB_SYNOPSYSHCD_REQUEST_INFO pRequestInfo,
    UINT32                        uUsbHctSizeReg,
    UINT32                        uUsbHccharReg
    );
VOID usbSynopysHcdDeleteRequestFromScheduleList
    (
    pUSB_SYNOPSYSHCD_DATA         pHCDData,
    pUSB_SYNOPSYSHCD_PIPE         pHCDPipe,
    pUSB_SYNOPSYSHCD_REQUEST_INFO pRequestInfo
    );
VOID usbSynopysHcdDeleteRequestFromReadyList
    (
    pUSB_SYNOPSYSHCD_DATA         pHCDData,
    pUSB_SYNOPSYSHCD_PIPE         pHCDPipe,
    pUSB_SYNOPSYSHCD_REQUEST_INFO pRequestInfo
    );
VOID usbSynopsysHcdAddRequestIntoSendList
    (
    pUSB_SYNOPSYSHCD_DATA         pHCDData,
    pUSB_SYNOPSYSHCD_PIPE         pHCDPipe,
    pUSB_SYNOPSYSHCD_REQUEST_INFO pRequestInfo
    );
STATUS usbSynopsysHcdMoveRequetToReadyList
    (
    pUSB_SYNOPSYSHCD_DATA         pHCDData,
    pUSB_SYNOPSYSHCD_PIPE         pHCDPipe,
    pUSB_SYNOPSYSHCD_REQUEST_INFO pRequestInfo
    );
STATUS usbSynopsysHcdMoveRequetToSendList
    (
    pUSB_SYNOPSYSHCD_DATA         pHCDData,
    pUSB_SYNOPSYSHCD_PIPE         pHCDPipe,
    pUSB_SYNOPSYSHCD_REQUEST_INFO pRequestInfo
    );
void usbSynopsysHcdUrbComplete
    (
    pUSBHST_URB            pUrb,
    int                    status
    );
void usbSynopsysHcdCleanPipe
    (
    pUSB_SYNOPSYSHCD_DATA         pHCDData,    
    pUSB_SYNOPSYSHCD_PIPE         pHCDPipe
    );
void usbSynopsysHcdHaltChannel
    (
    pUSB_SYNOPSYSHCD_DATA         pHCDData,   
    pUSB_SYNOPSYSHCD_PIPE         pHCDPipe    
    );
void usbSynopsysHcdUnHaltChannel
    (
    pUSB_SYNOPSYSHCD_DATA         pHCDData,   
    pUSB_SYNOPSYSHCD_PIPE         pHCDPipe    
    );
pUSB_SYNOPSYSHCD_REQUEST_INFO usbSynopsysHcdCreateReq
    (
    pUSB_SYNOPSYSHCD_PIPE pHCDPipe
    );
void usbSynopsysHcdUnlinkReq
    (
    pUSB_SYNOPSYSHCD_PIPE          pHCDPipe,
    pUSB_SYNOPSYSHCD_REQUEST_INFO  pRequest
    );
void usbSynopsysHcdDeleteReq
    (
    pUSB_SYNOPSYSHCD_PIPE          pHCDPipe,
    pUSB_SYNOPSYSHCD_REQUEST_INFO  pRequest
    );
pUSB_SYNOPSYSHCD_REQUEST_INFO usbSynopsysHcdFirstReqGet
    (
    pUSB_SYNOPSYSHCD_PIPE          pHCDPipe
    );
pUSB_SYNOPSYSHCD_PIPE usbSynopsysHcdNewPipe(void);
void usbSynopsysHcdDestroyPipe
    (
    pUSB_SYNOPSYSHCD_PIPE pHCDPipe
    );

#ifdef	__cplusplus
}
#endif

#endif /* __INCSynopsysHcdUtilh*/

/* End of file */
