/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file  fmsh_common_dev.h
*
* This file contains
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 0.01   yl  12/20/2018  First Release
*</pre>
******************************************************************************/
#ifndef _FMSH_COMMON_DEV_H_
#define _FMSH_COMMON_DEV_H_

#ifdef __cplusplus
extern "C" {    /* allow C++ to use these headers */
#endif

/***************************** Include Files *********************************/

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/*****************************************************************************
* DESCRIPTION
*  This is a generic data type used for 1-bit wide bitfields which have
*  a "set/clear" property.  This is used when modifying registers
*  within a peripheral's memory map.       
*
*****************************************************************************/
enum FMSH_state
{
    FMSH_err = -1,
    FMSH_clear = 0,
    FMSH_set = 1
};

/*****************************************************************************
* DESCRIPTION
*  This data type is used to identify peripheral types.  These values
*  are typically encoded in a peripheral's portmap and are also used in
*  the FMSH_device structure.
*
*****************************************************************************/
enum FMSH_compType {
    FMSH_dev_none     = 0x00000000,
    FMSH_ahb_smc      = 0x44572110,
    FMSH_ahb_dmac     = 0x44571110,
    FMSH_ahb_ictl     = 0x44571120,
    FMSH_ahb_pci      = 0x44571130,
    FMSH_ahb_usb      = 0x44571140,
    FMSH_ahb_sdmmc    = 0x44571150,
    FMSH_apb_gpio     = 0x44570160,
    FMSH_apb_i2c      = 0x44570140,
    FMSH_apb_ictl     = 0x44570180,
    FMSH_apb_rap      = 0x44570190,
    FMSH_apb_rtc      = 0x44570130,
    FMSH_apb_ssi      = 0x44570150,
    FMSH_apb_timers   = 0x44570170,
    FMSH_apb_uart     = 0x44570110,
    FMSH_apb_wdt      = 0x44570120,
};

/****s* include.types/FMSH_device
 * NAME
 *  FMSH_device -- low-level device handle structure
 * SYNOPSIS
 *  This is the handle used to identify and manipulate all DesignWare
 *  peripherals.
 * DESCRIPTION
 *  This is the primary structure used when dealing with all devices.
 *  It serves as a harFMSHare abstraction layer for driver code and also
 *  allows this code to support more than one device of the same type
 *  simultaneously.  This structure needs to be initialized with
 *  meaningful values before a pointer to it is passed to a driver
 *  initialization function.
 * PARAMETERS
 *  name            name of device
 *  dataWidth       bus data width of the device
 *  baseAddress     physical base address of device
 *  instance        device private data structure pointer
 *  os              unused pointer for associating with an OS structure
 *  compParam       pointer to structure containing device's
 *                  coreConsultant configuration parameters structure
 *  compVersion     device version identification number
 *  compType        device identification number
 ***/
/*****************************************************************************
* DESCRIPTION
*  This is the handle used to identify and manipulate all DesignWare
*  peripherals. 
*  This is the primary structure used when dealing with all devices.
*  It serves as a harFMSHare abstraction layer for driver code and also
*  allows this code to support more than one device of the same type
*  simultaneously.  This structure needs to be initialized with
*  meaningful values before a pointer to it is passed to a driver
*  initialization function.
*
* @param
*  name            name of device
*  dataWidth       bus data width of the device
*  baseAddress     physical base address of device
*  instance        device private data structure pointer
*  os              unused pointer for associating with an OS structure
*  compParam       pointer to structure containing device's
*                  coreConsultant configuration parameters structure
*  compVersion     device version identification number
*  compType        device identification number      
*
*****************************************************************************/
typedef struct fmsh_device_s {
    const char *name;
    unsigned data_width;
    void *base_address;
    void *instance;
    void *os;
    void *comp_param;
    UINT32 comp_version;
    enum FMSH_compType comp_type;
    FMSH_listHead list;
}FMSH_device;

/*****************************************************************************
* DESCRIPTION
*  This is a generic data type used for handling callback functions
*  with each driver.
*
* @param    
*           pDev pointer to device handle.
*           eCode event/error code.
*
* @note
*  The usage of the eCode argument is typically negative for an error
*  code and positive for an event code
*
*****************************************************************************/
typedef void (*FMSH_callback)(void *pDev, int32_t eCode);
	
/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _FMSH_COMMON_DEV_H_ */

