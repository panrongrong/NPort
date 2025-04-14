/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_can_common.h
*
* This file contains common type define
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 0.01   wfb  11/23/2018  First Release
*</pre>
******************************************************************************/

#ifndef _FMSH_CAN_COMMON_H_		/* prevent circular inclusions */
#define _FMSH_CAN_COMMON_H_		/* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif
 
/***************************** Include Files *********************************/
    
#include "../../common/fmsh_common.h"
    
/************************** Constant Definitions *****************************/
/**
 * This typedef contains configuration information for the device.
 */
typedef struct {
	u16 DeviceId;	 /**< Unique ID  of device */
	u32 BaseAddress; /**< Base address of device (IPIF) */
	u32 InputClockHz;/**< Input clock frequency */
} FCanPs_Config;

/**************************** Type Definitions *******************************/
 
/**
 * CAN bit-timing parameters
 *
 * For further information, please read chapter "8 BIT TIMING
 * REQUIREMENTS" of the "Bosch CAN Specification version 2.0"
 * at http://www.semiconductors.bosch.de/pdf/can2spec.pdf.
 */
struct can_bittiming {
	u32 bitrate;		/* Bit-rate in bits/second */
	u32 sample_point;	/* Sample point in one-tenth of a percent */
	u32 tq;		/* Time quanta (TQ) in nanoseconds */
	u32 prop_seg;		/* Propagation segment in TQs */
	u32 phase_seg1;	/* Phase buffer segment 1 in TQs */
	u32 phase_seg2;	/* Phase buffer segment 2 in TQs */
	u32 sjw;		/* Synchronisation jump width in TQs */
	u32 brp;		/* Bit-rate prescaler */
};

/**
 * CAN harware-dependent bit-timing constant
 *
 * Used for calculating and checking bit-timing parameters
 */
struct can_bittiming_const {
	char name[16];		/* Name of the CAN controller hardware */
	u32 tseg1_min;	/* Time segement 1 = prop_seg + phase_seg1 */
	u32 tseg1_max;
	u32 tseg2_min;	/* Time segement 2 = phase_seg2 */
	u32 tseg2_max;
	u32 sjw_max;		/* Synchronisation jump width */
	u32 brp_min;		/* Bit-rate prescaler */
	u32 brp_max;
	u32 brp_inc;
};

/**
 * DESCRIPTION
 *  This is the primary structure used when dealing with all devices.
 *  It serves as a hardware abstraction layer for driver code and also
 *  allows this code to support more than one device of the same type
 *  simultaneously.  This structure needs to be initialized with
 *  meaningful values before a pointer to it is passed to a driver
 *  initialization function.
 * PARAMETERS
 *  baseAddress     physical base address of device
 *  instance        device private data structure pointer
 *  compVersion     device version identification number
 *  compType        device identification number
 */
typedef struct FCanPs 
{
    void *base_address;
    u32 input_clock;
    struct can_bittiming bt;
    struct can_bittiming_const btc;
    u32 comp_version;
    u32 comp_type;
    u32 id;
}FCanPs_T;

/***************** Macros (Inline Functions) Definitions *********************/  
    

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* end of protection macro */
