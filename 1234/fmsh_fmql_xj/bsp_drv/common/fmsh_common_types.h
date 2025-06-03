/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file  fmsh_common_types.h
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
#ifndef _FMSH_COMMON_TYPES_H_
#define _FMSH_COMMON_TYPES_H_

/***************************** Include Files *********************************/

#include <stdint.h>
#include <stddef.h>

/************************** Constant Definitions *****************************/

#ifndef TRUE
#define TRUE		1U
#endif

#ifndef FALSE
#define FALSE		0U
#endif

#ifndef NULL
#define NULL		0U
#endif

#define COMPONENT_IS_READY     0x11111111U  /**< component has been initialized */
#define COMPONENT_IS_STARTED   0x22222222U  /**< component has been started */

/**************************** Type Definitions *******************************/


#ifndef __KERNEL__
typedef uint8_t u8;    /* usigned 8-bit  integer   */
typedef uint16_t u16; /* usigned 16-bit  integer   */
typedef UINT32 u32; /* usigned 32-bit  integer   */
typedef uint64_t u64; /* usigned 64-bit  integer   */
typedef int8_t s8;     /* signed 8-bit  integer   */
typedef int16_t s16;  /* signed 16-bit  integer   */
typedef int32_t s32;  /* signed 32-bit  integer   */
typedef int64_t s64;  /* signed 64-bit  integer   */
typedef float f32;	   /* 32-bit floating point */
typedef double f64;   /* 64-bit double precision FP */
typedef char char8; 
typedef int sint32;

#if !defined(LONG) || !defined(ULONG)
typedef long LONG;
typedef unsigned long ULONG;
#endif

#ifndef __STDBOOL_H__
/* typedef unsigned char BOOL; */
#endif

#define ULONG64_HI_MASK	0xFFFFFFFF00000000U
#define ULONG64_LO_MASK	~ULONG64_HI_MASK

#else
#include <linux/types.h>
#endif

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

#endif /* #ifndef _FMSH_COMMON_TYPES_H_ */
