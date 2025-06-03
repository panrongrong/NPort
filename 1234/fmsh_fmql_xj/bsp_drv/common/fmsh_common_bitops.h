/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file  fmsh_common_bitops.h
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
#ifndef _FMSH_COMMON_BITOPS_H_
#define _FMSH_COMMON_BITOPS_H_

#ifdef __cplusplus
extern "C" {    /* allow C++ to use these headers */
#endif

/***************************** Include Files *********************************/

/************************** Constant Definitions *****************************/

/* Constant definitions for various bits of a 32-bit word. */
#define FMSH_BIT0     0x00000001
#define FMSH_BIT1     0x00000002
#define FMSH_BIT2     0x00000004
#define FMSH_BIT3     0x00000008
#define FMSH_BIT4     0x00000010
#define FMSH_BIT5     0x00000020
#define FMSH_BIT6     0x00000040
#define FMSH_BIT7     0x00000080
#define FMSH_BIT8     0x00000100
#define FMSH_BIT9     0x00000200
#define FMSH_BIT10    0x00000400
#define FMSH_BIT11    0x00000800
#define FMSH_BIT12    0x00001000
#define FMSH_BIT13    0x00002000
#define FMSH_BIT14    0x00004000
#define FMSH_BIT15    0x00008000
#define FMSH_BIT16    0x00010000
#define FMSH_BIT17    0x00020000
#define FMSH_BIT18    0x00040000
#define FMSH_BIT19    0x00080000
#define FMSH_BIT20    0x00100000
#define FMSH_BIT21    0x00200000
#define FMSH_BIT22    0x00400000
#define FMSH_BIT23    0x00800000
#define FMSH_BIT24    0x01000000
#define FMSH_BIT25    0x02000000
#define FMSH_BIT26    0x04000000
#define FMSH_BIT27    0x08000000
#define FMSH_BIT28    0x10000000
#define FMSH_BIT29    0x20000000
#define FMSH_BIT30    0x40000000
#define FMSH_BIT31    0x80000000
#define FMSH_BITS_ALL 0xFFFFFFFF

/**************************** Type Definitions *******************************/

/*****************************************************************************
* DESCRIPTION
*  Returns the width of the specified bit-field.
* ARGUMENTS
*  __bfws      a width/shift pair       
*
*****************************************************************************/
#define FMSH_BIT_WIDTH(__bfws)    ((UINT32) (bfw## __bfws))

/*****************************************************************************
* DESCRIPTION
*  Returns the offset of the specified bit-field.
* ARGUMENTS
*  __bfws      a width/shift pair
*
*****************************************************************************/
#define FMSH_BIT_OFFSET(__bfws)   ((UINT32) (bfo## __bfws))

/*****************************************************************************
* DESCRIPTION
*  Returns a mask with the bits to be addressed set and all others cleared.
* ARGUMENTS
*  __bfws      a width/shift pair
*
*****************************************************************************/
#define FMSH_BIT_MASK(__bfws) ((UINT32) (((bfw## __bfws) == 32) ?  \
        0xFFFFFFFF : ((1U << (bfw## __bfws)) - 1)) << (bfo## __bfws))

/*****************************************************************************
* DESCRIPTION
*  Clear the specified bits.
* ARGUMENTS
*  __datum     the word of data to be modified
*  __bfws      a width/shift pair
*
*****************************************************************************/
#define FMSH_BIT_CLEAR(__datum, __bfws)                               \
    ((__datum) = ((UINT32) (__datum) & ~FMSH_BIT_MASK(__bfws)))

/*****************************************************************************
* DESCRIPTION
*  Returns the relevant bits masked from the data word, still at their
*  original offset.
* ARGUMENTS
*  __datum     the word of data to be accessed
*  __bfws      a width/shift pair
*
*****************************************************************************/
#define FMSH_BIT_GET_UNSHIFTED(__datum, __bfws)                       \
    ((UINT32) ((__datum) & FMSH_BIT_MASK(__bfws)))

/*****************************************************************************
* DESCRIPTION
*  Returns the relevant bits masked from the data word shifted to bit
*  zero (i.e. access the specifed bits from a word of data as an
*  integer value).
* ARGUMENTS
*  __datum     the word of data to be accessed
*  __bfws      a width/shift pair
*
*****************************************************************************/
#define FMSH_BIT_GET(__datum, __bfws)                                 \
    ((UINT32) (((__datum) & FMSH_BIT_MASK(__bfws)) >>               \
                 (bfo## __bfws)))

/*****************************************************************************
* DESCRIPTION
*  Place the specified value into the specified bits of a word of data
*  (first the data is read, and the non-specified bits are re-written).
* ARGUMENTS
*  __datum     the word of data to be accessed
*  __bfws      a width/shift pair       
*  __val       the data value to be shifted into the specified bits
*
*****************************************************************************/
#define FMSH_BIT_SET(__datum, __bfws, __val)                          \
    ((__datum) = ((UINT32) (__datum) & ~FMSH_BIT_MASK(__bfws)) |    \
            ((__val << (bfo## __bfws)) & FMSH_BIT_MASK(__bfws)))

/*****************************************************************************
* DESCRIPTION
*  Place the specified value into the specified bits of a word of data
*  without reading first - for sensitive interrupt type registers
* ARGUMENTS
*  __datum     the word of data to be accessed
*  __bfws      a width/shift pair       
*  __val       the data value to be shifted into the specified bits
*
*****************************************************************************/
#define FMSH_BIT_SET_NOREAD(__datum, __bfws, __val)                   \
    ((UINT32) ((__datum) = (((__val) << (bfo## __bfws)) &        \
                              FMSH_BIT_MASK(__bfws))))

/*****************************************************************************
* DESCRIPTION
 *  Shift the specified value into the desired bits.
 * ARGUMENTS
 *  __bfws      a width/shift pair       
 *  __val       the data value to be shifted into the specified bits
*
*****************************************************************************/
#define FMSH_BIT_BUILD(__bfws, __val)                                 \
    ((UINT32) (((__val) << (bfo## __bfws)) & FMSH_BIT_MASK(__bfws)))

#ifdef __cplusplus
}
#endif

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

u32 mask_generate(u32 bit_no);

#endif /* #ifndef _FMSH_COMMON_BITOPS_H_ */

