/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file  fmsh_common_delay.h
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
#ifndef _FMSH_COMMON_DELAY_H_
#define _FMSH_COMMON_DELAY_H_

#ifdef __cplusplus
extern "C" {    /* allow C++ to use these headers */
#endif

/***************************** Include Files *********************************/

/************************** Constant Definitions *****************************/

#define GTC_CLK_FREQ    15/*25M */

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

void global_timer_enable();
u64 get_current_time()   ;
void delay_ms(double time_ms);
void delay_us(u32 time_us);
void delay_1ms();
void delay_1us();

#endif /* #ifndef _FMSH_COMMON_DELAY_H_ */
