/******************************************************************************
*
* Copyright (C) 2010 - 2019 Xilinx, Inc.  All rights reserved.
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
 * @file xaxidma_bd.c
* @addtogroup axidma_v9_9
* @{
 *
 * Buffer descriptor (BD) management API implementation.
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   Who  Date     Changes
 * ----- ---- -------- -------------------------------------------------------
 * 1.00a jz   05/18/10 First release
 * 2.00a jz   08/10/10 Second release, added in xaxidma_g.c, xaxidma_sinit.c,
 *                     updated tcl file, added xaxidma_porting_guide.h
 * 3.00a jz   11/22/10 Support IP core parameters change
 * 6.00a srt  01/24/12 Added support for Multi-Channel DMA.
 *		       - Changed APIs
 *			* XAxiDma_BdSetLength(XAxiDma_Bd *BdPtr,
 *					u32 LenBytes, u32 LengthMask)
 *       		* XAxiDma_BdGetActualLength(BdPtr, LengthMask)
 *			* XAxiDma_BdGetLength(BdPtr, LengthMask)
 * 8.0   srt  01/29/14 Added support for Micro DMA Mode:
 *		       - New API
 *			 XAxiDma_BdSetBufAddrMicroMode(XAxiDma_Bd*, u32)
 * 9.8   mus  11/05/18 Support 64 bit DMA addresses for Microblaze-X
 *
 * </pre>
 *
 *****************************************************************************/

#include "xaxidma_bd.h"

/************************** Function Prototypes ******************************/

/*****************************************************************************/
/**
 * Set the length field for the given BD.
 *
 * Length has to be non-zero and less than LengthMask.
 *
 * For TX channels, the value passed in should be the number of bytes to
 * transmit from the TX buffer associated with the given BD.
 *
 * For RX channels, the value passed in should be the size of the RX buffer
 * associated with the given BD in bytes. This is to notify the RX channel
 * the capability of the RX buffer to avoid buffer overflow.
 *
 * The actual receive length can be equal or smaller than the specified length.
 * The actual transfer length will be updated by the hardware in the
 * XAXIDMA_BD_STS_OFFSET word in the BD.
 *
 * @param	BdPtr is the BD to operate on.
 * @param	LenBytes is the requested transfer length
 * @param	LengthMask is the maximum transfer length
 *
 * @returns
 *		- XST_SUCCESS for success
 *		- XST_INVALID_PARAM for invalid BD length
 *
 * @note	This function can be used only when DMA is in SG mode
 *
 *****************************************************************************/
int XAxiDma_BdSetLength(XAxiDma_Bd *BdPtr, u32 LenBytes, u32 LengthMask)
{
	if (LenBytes <= 0 || (LenBytes > LengthMask)) {

		xdbg_printf(XDBG_DEBUG_ERROR, "invalid length %d\n",
		    (int)LenBytes);

		return XST_INVALID_PARAM;
	}

	XAxiDma_BdWrite((BdPtr), XAXIDMA_BD_CTRL_LEN_OFFSET,
		((XAxiDma_BdRead((BdPtr), XAXIDMA_BD_CTRL_LEN_OFFSET) & \
		~LengthMask)) | LenBytes);

	return XST_SUCCESS;
}
/*****************************************************************************/
/**
 * Set the BD's buffer address.
 *
 * @param	BdPtr is the BD to operate on
 * @param	Addr is the address to set
 *
 * @return
 *		- XST_SUCCESS if buffer address set successfully
 *		- XST_INVALID_PARAM if hardware has no DRE and address is not
 *		aligned
 *
 * @note	This function can be used only when DMA is in SG mode
 *
 *****************************************************************************/
u32 XAxiDma_BdSetBufAddr(XAxiDma_Bd* BdPtr, UINTPTR Addr)
{
	u32 HasDRE;
	u8 WordLen;

	HasDRE = XAxiDma_BdRead(BdPtr, XAXIDMA_BD_HAS_DRE_OFFSET);
	WordLen = HasDRE & XAXIDMA_BD_WORDLEN_MASK;

	if (Addr & (WordLen - 1)) {
		if ((HasDRE & XAXIDMA_BD_HAS_DRE_MASK) == 0) {
			printf("Error set buf addr %x with %x and %x,"
			" %x\r\n",Addr, HasDRE, (WordLen - 1),
			Addr & (WordLen - 1));

			return XST_INVALID_PARAM;
		}
	}

#if defined(__aarch64__) || defined(__arch64__)
	XAxiDma_BdWrite64(BdPtr, XAXIDMA_BD_BUFA_OFFSET, Addr);
#else
	XAxiDma_BdWrite(BdPtr, XAXIDMA_BD_BUFA_OFFSET, Addr);
#endif

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
 * Set the BD's buffer address when configured for Micro Mode.  The buffer
 * address should be 4K aligned.
 *
 * @param	BdPtr is the BD to operate on
 * @param	Addr is the address to set
 *
 * @return
 *		- XST_SUCCESS if buffer address set successfully
 *		- XST_INVALID_PARAM if hardware has no DRE and address is not
 *		aligned
 *
 * @note	This function can be used only when DMA is in SG mode
 *
 *****************************************************************************/
u32 XAxiDma_BdSetBufAddrMicroMode(XAxiDma_Bd* BdPtr, UINTPTR Addr)
{
	if (Addr & XAXIDMA_MICROMODE_MIN_BUF_ALIGN) {
			printf("Error set buf addr %x and %x,"
			" %x\r\n", Addr, XAXIDMA_MICROMODE_MIN_BUF_ALIGN,
			Addr & XAXIDMA_MICROMODE_MIN_BUF_ALIGN);

			return XST_INVALID_PARAM;
	}

#if defined(__aarch64__) || defined(__arch64__)
	XAxiDma_BdWrite64(BdPtr, XAXIDMA_BD_BUFA_OFFSET, Addr);
#else
	XAxiDma_BdWrite(BdPtr, XAXIDMA_BD_BUFA_OFFSET, Addr);
#endif

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
 * Set the APP word at the specified APP word offset for a BD.
 *
 * @param	BdPtr is the BD to operate on.
 * @param	Offset is the offset inside the APP word, it is valid from
 *		0 to 4
 * @param	Word is the value to set
 *
 * @returns
 *		- XST_SUCCESS for success
 *		- XST_INVALID_PARAM under following error conditions:
 *		1) StsCntrlStrm is not built in hardware
 *		2) Offset is not in valid range
 *
 * @note
 *		If the hardware build has C_SG_USE_STSAPP_LENGTH set to 1,
 *		then the last APP word, XAXIDMA_LAST_APPWORD, must have
 *		non-zero value when AND with 0x7FFFFF. Not doing so will cause
 *		the hardware to stall.
 *		This function can be used only when DMA is in SG mode
 *
 *****************************************************************************/
int XAxiDma_BdSetAppWord(XAxiDma_Bd* BdPtr, int Offset, u32 Word)
{
	if (XAxiDma_BdRead(BdPtr, XAXIDMA_BD_HAS_STSCNTRL_OFFSET) == 0) {

		xdbg_printf(XDBG_DEBUG_ERROR, "BdRingSetAppWord: no sts cntrl"
			"stream in hardware build, cannot set app word\r\n");

		return XST_INVALID_PARAM;
	}

	if ((Offset < 0) || (Offset > XAXIDMA_LAST_APPWORD)) {

		xdbg_printf(XDBG_DEBUG_ERROR, "BdRingSetAppWord: invalid"
			"offset %d",Offset);

		return XST_INVALID_PARAM;
	}

	XAxiDma_BdWrite(BdPtr, XAXIDMA_BD_USR0_OFFSET + Offset * 4, Word);

	return XST_SUCCESS;
}
/*****************************************************************************/
/**
 * Get the APP word at the specified APP word offset for a BD.
 *
 * @param	BdPtr is the BD to operate on.
 * @param	Offset is the offset inside the APP word, it is valid from
 *		0 to 4
 * @param	Valid is to tell the caller whether parameters are valid
 *
 * @returns
 *		The APP word. Passed in parameter Valid holds 0 for failure,
 *		and 1 for success.
 *
 * @note	This function can be used only when DMA is in SG mode
 *
 *****************************************************************************/
u32 XAxiDma_BdGetAppWord(XAxiDma_Bd* BdPtr, int Offset, int *Valid)
{
	*Valid = 0;

	if (XAxiDma_BdRead(BdPtr, XAXIDMA_BD_HAS_STSCNTRL_OFFSET) == 0) {

		xdbg_printf(XDBG_DEBUG_ERROR, "BdRingGetAppWord: no sts cntrl "
			"stream in hardware build, no app word available\r\n");

		return (u32)0;
	}

	if((Offset < 0) || (Offset > XAXIDMA_LAST_APPWORD)) {

		xdbg_printf(XDBG_DEBUG_ERROR, "BdRingGetAppWord: invalid"
			" offset %d", Offset);

		return (u32)0;
	}

	*Valid = 1;

	return XAxiDma_BdRead(BdPtr, XAXIDMA_BD_USR0_OFFSET + Offset * 4);
}

/*****************************************************************************/
/**
 * Set the control bits for a BD.
 *
 * @param	BdPtr is the BD to operate on.
 * @param	Data is the bit value to set
 *
 * @return	None
 *
 * @note	This function can be used only when DMA is in SG mode
 *
 *****************************************************************************/
void XAxiDma_BdSetCtrl(XAxiDma_Bd* BdPtr, u32 Data)
{
	u32 RegValue = XAxiDma_BdRead(BdPtr, XAXIDMA_BD_CTRL_LEN_OFFSET);

	RegValue &= ~XAXIDMA_BD_CTRL_ALL_MASK;

	RegValue |= (Data & XAXIDMA_BD_CTRL_ALL_MASK);

	XAxiDma_BdWrite((BdPtr), XAXIDMA_BD_CTRL_LEN_OFFSET, RegValue);

	return;
}
/*****************************************************************************/
/**
 * Dump the fields of a BD.
 *
 * @param	BdPtr is the BD to operate on.
 *
 * @return	None
 *
 * @note	This function can be used only when DMA is in SG mode
 *
 *****************************************************************************/
void XAxiDma_DumpBd(XAxiDma_Bd* BdPtr)
{

	printf("Dump BD %x:\r\n", (UINTPTR)BdPtr);
	printf("\tNext Bd Ptr: %x\r\n",
	    (unsigned int)XAxiDma_BdRead(BdPtr, XAXIDMA_BD_NDESC_OFFSET));
	printf("\tBuff addr: %x\r\n",
	    (unsigned int)XAxiDma_BdRead(BdPtr, XAXIDMA_BD_BUFA_OFFSET));
	printf("\tMCDMA Fields: %x\r\n",
	    (unsigned int)XAxiDma_BdRead(BdPtr, XAXIDMA_BD_MCCTL_OFFSET));
	printf("\tVSIZE_STRIDE: %x\r\n",
	    (unsigned int)XAxiDma_BdRead(BdPtr,
					XAXIDMA_BD_STRIDE_VSIZE_OFFSET));
	printf("\tContrl len: %x\r\n",
	    (unsigned int)XAxiDma_BdRead(BdPtr, XAXIDMA_BD_CTRL_LEN_OFFSET));
	printf("\tStatus: %x\r\n",
	    (unsigned int)XAxiDma_BdRead(BdPtr, XAXIDMA_BD_STS_OFFSET));

	printf("\tAPP 0: %x\r\n",
	    (unsigned int)XAxiDma_BdRead(BdPtr, XAXIDMA_BD_USR0_OFFSET));
	printf("\tAPP 1: %x\r\n",
	    (unsigned int)XAxiDma_BdRead(BdPtr, XAXIDMA_BD_USR1_OFFSET));
	printf("\tAPP 2: %x\r\n",
	    (unsigned int)XAxiDma_BdRead(BdPtr, XAXIDMA_BD_USR2_OFFSET));
	printf("\tAPP 3: %x\r\n",
	    (unsigned int)XAxiDma_BdRead(BdPtr, XAXIDMA_BD_USR3_OFFSET));
	printf("\tAPP 4: %x\r\n",
	    (unsigned int)XAxiDma_BdRead(BdPtr, XAXIDMA_BD_USR4_OFFSET));

	printf("\tSW ID: %x\r\n",
	    (unsigned int)XAxiDma_BdRead(BdPtr, XAXIDMA_BD_ID_OFFSET));
	printf("\tStsCtrl: %x\r\n",
	    (unsigned int)XAxiDma_BdRead(BdPtr,
	           XAXIDMA_BD_HAS_STSCNTRL_OFFSET));
	printf("\tDRE: %x\r\n",
	    (unsigned int)XAxiDma_BdRead(BdPtr, XAXIDMA_BD_HAS_DRE_OFFSET));

	printf("\r\n");
}
/** @} */
