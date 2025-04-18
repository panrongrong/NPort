/******************************************************************************
*
* Copyright (C) 2015 - 2019 Xilinx, Inc.  All rights reserved.
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
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/
/*****************************************************************************/
/**
*
* @file xaxidma_selftest.c
* @addtogroup axidma_v9_9
* @{
*
* Contains diagnostic/self-test functions for the XAxiDma component.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who  Date     Changes
* ----- ---- -------- -----------------------------------------------
* 8.1 	adk  29/01/15 First release
* </pre>
*
******************************************************************************/

/***************************** Include Files *********************************/

#include "../include/xil_io.h"
#include "xaxidma.h"


/************************** Constant Definitions *****************************/
#define XAXIDMA_RESET_TIMEOUT   500

/**************************** Type Definitions *******************************/


/***************** Macros (Inline Functions) Definitions *********************/


/************************** Function Prototypes ******************************/


/************************** Variable Definitions *****************************/


/*****************************************************************************/
/**
*
* Runs a self-test on the driver/device. This test perform a
* reset of the DMA device and checks the device is coming out of reset or not
*
* @param	InstancePtr is a pointer to the XAxiDma instance.
*
* @return
* 		- XST_SUCCESS if self-test was successful
*		- XST_FAILURE if the device is not coming out of reset.
*
* @note
*     None.
*
******************************************************************************/
int XAxiDma_Selftest(XAxiDma * InstancePtr)
{
	int TimeOut;

	Xil_AssertNonvoid(InstancePtr != NULL);

	XAxiDma_Reset(InstancePtr);

	/* At the initialization time, hardware should finish reset quickly
	 */
	TimeOut = XAXIDMA_RESET_TIMEOUT;

	while (TimeOut) {

		if(XAxiDma_ResetIsDone(InstancePtr)) {
			break;
		}

		TimeOut -= 1;

	}

	if (!TimeOut)
		return XST_FAILURE;

	return XST_SUCCESS;
}
/** @} */
