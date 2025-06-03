/* usbSynopsysHcdEventHandler.h - interrupt routines for Synopsys HCD */

/*
 * Copyright (c) 2009, 2011 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify, or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------                 
01b,05sep11,m_y  add support for polling mode (WIND00298534)  
01a,11aug09,s_z  written
*/

/*
DESCRIPTION

This file contains interrupt routines which handle the Synopsys USB
host controller interrupts.

*/

#ifndef __INCSynopsysHcdEventHandlerh
#define __INCSynopsysHcdEventHandlerh

/*
#include <usbSynopsysHcdDataStructures.h>
*/
#include "usbSynopsysHcdDataStructures.h"
#ifdef	__cplusplus
extern "C" {
#endif



IMPORT void usbSynopsysHcdISR
    (
    pUSB_SYNOPSYSHCD_DATA pSYNOPSYSHCDData
    );
IMPORT void usbSynopsysHcdInterruptHandler
    (
    pUSB_SYNOPSYSHCD_DATA pSYNOPSYSHCDData
    );
IMPORT VOID usbSynopsysHcdProcessScheduleHandler
    (
    pUSB_SYNOPSYSHCD_DATA pHCDData
    );
IMPORT VOID usbSynopsysHcdTransferHandler
    (
    pUSB_SYNOPSYSHCD_DATA pHCDData
    );
IMPORT void usbShcdPollingISR
    (
    pUSB_SYNOPSYSHCD_DATA pHCDData
    );
#ifdef	__cplusplus
}
#endif

#endif /* __INCSynopsysHcdEventHandlerh */

/* End of file */
