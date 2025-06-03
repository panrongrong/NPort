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
* @file fmsh_qspips_sw.h
* @addtogroup qspips_v1_1
* @{
*
* This header file used to contains the identifiers and user used driver
* functions (or macros) that can be used to access the device.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who Date     Changes
* ----- --- -------- -----------------------------------------------
* 1.00  hzq 7/1/19 
* 		     First release
* 1.10  hzq 11/26/20 
* 		     This file has been deprecated and used for compatibility only 
*            which is not recommended to use in the new program.
*
* </pre>
*
******************************************************************************/

#ifndef _FMSH_QSPIPS_SW_H_	/* prevent circular inclusions */
#define _FMSH_QSPIPS_SW_H_	/* by using protection macros */ 

#ifdef __cplusplus
extern "C" {
#endif
    
/**********************************Include File*********************************/

/**********************************Constant Definition**************************/  
    
/**********************************Type Definition******************************/

/**********************************Macro (inline function) Definition***********/
#define FQspiFlash_ReadId(qspiPtr)    \
                    FQspiPs_ReadId(qspiPtr, id);     
    
#define FQspiPs_EnterXIP(qspiPtr, cmd)  \
                    FQspiPs_Nor_EnterXIP(qspiPtr, cmd)        

#define FQspiPs_UnlockFlash(qspiPtr)    \
                    FQspiPs_Nor_Unlock(qspiPtr)
                        
#define FQspiPs_SetFlashSegment(qspiPtr, highAddr)  \
                    FQspiPs_Nor_SetSegment(qspiPtr, highAddr)    
                        
#define FQspiPs_EraseSectors(qspiPtr, offset, byteCount, blockSize) \
                    FQspiPs_Nor_Erase(qspiPtr, offset, byteCount, blockSize)            

#define FQspiPs_EraseChip(qspiPtr) \
                    FQspiPs_Nor_ChipErase(qspiPtr, 0xC7)
                        
#define FQspiFlash_GetStatus1(qspiPtr)  \
                    FQspiPs_Nor_GetStatus1(qspiPtr)

#define FQspiFlash_GetReg16(qspiPtr, cmd)   \
                    FQspiPs_Nor_GetReg16(qspiPtr, cmd)
                        
#define FQspiFlash_GetReg8(qspiPtr, cmd)    \
                    FQspiPs_Nor_GetReg8(qspiPtr, cmd)
                        
#define FQspiFlash_SetReg16(qspiPtr, cmd, high_value, low_value)    \
                    FQspiPs_Nor_SetReg16(qspiPtr, cmd, high_value, low_value)    

#define  FQspiFlash_SetReg8(qspiPtr, cmd, value)    \
                    FQspiPs_Nor_SetReg8(qspiPtr, cmd, value)    
                        
#define FQspiFlash_WaitForWIP(qspiPtr)  \
                    FQspiPs_Nor_WaitForWIP(qspiPtr, 1500)
                        
#define FQspiFlash_WREN(qspiPtr)  \
                    FQspiPs_Nor_WREN(qspiPtr)                        

#define FQspiFlash_WRDI(qspiPtr)  \
                    FQspiPs_Nor_WRDI(qspiPtr)
                        
#define FQspiFlash_SectorErase(qspiPtr, start_offset, cmd)  \
                    FQspiPs_Nor_SectorErase(qspiPtr, start_offset, cmd)
                        
#define FQspiFlash_ChipErase(qspiPtr, cmd)  \
                    FQspiPs_Nor_ChipErase(qspiPtr, cmd)
                                              

/**********************************Variable Definition**************************/

/**********************************Function Prototype***************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* prevent circular inclusions */*