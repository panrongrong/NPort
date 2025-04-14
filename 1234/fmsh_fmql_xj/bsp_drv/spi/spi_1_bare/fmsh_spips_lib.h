/******************************************************************************
*
* Copyright (C) FMSH, Corp.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* FMSH BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the FMSH shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from FMSH.
*
******************************************************************************/
/*****************************************************************************/
/**
*
* @file fmsh_spips_lib.h
* @addtogroup spips_v1_1
* @{
*
* This header file contains all other header files.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who Date     Changes
* ----- --- -------- -----------------------------------------------
* 1.00  hzq 7/1/19 
* 		     First release
* 1.10  hzq 11/26/20 
* 		     Add fmsh_spips_eeprom.h and fmsh_qspips_nand.h.
*
* </pre>
*
******************************************************************************/

#ifndef _FMSH_SPIPS_LIB_H_	/* prevent circular inclusions */
#define _FMSH_SPIPS_LIB_H_	/* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif
    
/**********************************Include File*********************************/
    
#include "../../common/fmsh_common.h"

#include "fmsh_spips.h"
#include "fmsh_spips_hw.h"
#include "fmsh_spips_sw.h"

/**********************************Constant Definition**************************/
    
/**********************************Type Definition******************************/

/**********************************Macro (inline function) Definition***********/
    
/**********************************Variable Definition**************************/

/**********************************Function Prototype***************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* prevent circular inclusions */
