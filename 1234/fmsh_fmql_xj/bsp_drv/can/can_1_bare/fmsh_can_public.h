/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_can_public.h
*
* This file contains public constant & function define
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

#ifndef _FMSH_CAN_PUBLIC_H_ /* prevent circular inclusions */
#define _FMSH_CAN_PUBLIC_H_ /* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/    

#include "fmsh_can_common.h"
        
/************************** Constant Definitions *****************************/        

/**************************** Type Definitions *******************************/

/**
 * DESCRIPTION
 *  can filter mode
 */
enum filter_mode
{
    single_filter = 0x01,
    dual_filter = 0x00,
};

/**
 * DESCRIPTION
 *  can frame type
 */
enum frame_type
{
    data_frame = 0x00,
    remote_frame = 0x01,
};

/**
 * DESCRIPTION
 *  This is a generic data type used for 1-bit wide bitfields which have
 *  a "set/clear" property.  This is used when modifying registers
 *  within a peripheral's memory map.
 */
enum FCanPs_state
{
   CAN_set = 1,
   CAN_clear = 0,
   CAN_err = -1,
};

/***************** Macros (Inline Functions) Definitions *********************/  
        
/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/
FCanPs_Config *FCanPs_LookupConfig(u16 DeviceId);

u8 FCanPs_init(FCanPs_T *dev, FCanPs_Config *cfg);

u8 FCanPs_setBaudRate(FCanPs_T *dev, u32 baud);

u8 FCanPs_setStdACR(FCanPs_T *dev, u32 id, u8 ptr, u8 byte1, u8 byte2, enum filter_mode mode);
u8 FCanPs_setEffACR(FCanPs_T *dev, u32 id, u8 ptr, u8 byte1, u8 byte2, enum filter_mode mode);
u8 FCanPs_setAMR(FCanPs_T *dev,u32 AMR0,u32 AMR1,u32 AMR2,u32 AMR3);

u8 FCanPs_setSleepMode(FCanPs_T *dev, enum FCanPs_state state);
u8 FCanPs_setListenOnlyMode(FCanPs_T *dev, enum FCanPs_state state);
u8 FCanPs_setResetMode(FCanPs_T *dev, enum FCanPs_state state);
u8 FCanPs_setSelfTestMode(FCanPs_T *dev, enum FCanPs_state state);
u8 FCanPs_setReceiveFilterMode(FCanPs_T *dev, enum filter_mode mode);

u32 FCanPs_getReceiveBufferStatus(FCanPs_T *dev);
u32 FCanPs_getDataOverrunStatus(FCanPs_T *dev);
u32 FCanPs_getTransmitBufferStatus(FCanPs_T *dev);
u32 FCanPs_getTransmissionCompleteStatus(FCanPs_T *dev);
u32 FCanPs_getReceiveStatus(FCanPs_T *dev);
u32 FCanPs_getTtansmitStatus(FCanPs_T *dev);
u32 FCanPs_getErrorStatus(FCanPs_T *dev);
u32 FCanPs_getBusStatus(FCanPs_T *dev);

u8 FCanPs_transmissionRequest(FCanPs_T *dev);
u8 FCanPs_abortTransmission(FCanPs_T *dev);
u8 FCanPs_releaseReceiveBuffer(FCanPs_T *dev);
u8 FCanPs_clearDataOverrun(FCanPs_T *dev);
u8 FCanPs_selfReceptionRequest(FCanPs_T *dev);

u8 FCanPs_setReceiveInterrupt(FCanPs_T *dev, enum FCanPs_state state);
u8 FCanPs_setTransmitInterrupt(FCanPs_T *dev, enum FCanPs_state state);
u8 FCanPs_setErrorWarningInterrupt(FCanPs_T *dev, enum FCanPs_state state);
u8 FCanPs_setDataOverrunInterrupt(FCanPs_T *dev, enum FCanPs_state state);
u8 FCanPs_setWakeUpInterrupt(FCanPs_T *dev, enum FCanPs_state state);
u8 FCanPs_setErrorPassiveInterrupt(FCanPs_T *dev, enum FCanPs_state state);
u8 FCanPs_setArbitrationLostInterrupt(FCanPs_T *dev, enum FCanPs_state state);
u8 FCanPs_setBusErrorInterrupt(FCanPs_T *dev, enum FCanPs_state state);

u8 FCanPs_standardFrameTransmit(FCanPs_T *dev, u32 id,u8 *sbuf,u8 len,enum frame_type state);
u8 FCanPs_extendedFrameTransmit(FCanPs_T *dev, u32 id,u8 *sbuf,u8 len,enum frame_type state);
u8 FCanPs_frameReceive(FCanPs_T *dev, u8 *rbuf);

u32 FCanPs_getInterrupt(FCanPs_T *dev);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* end of protection macro */C