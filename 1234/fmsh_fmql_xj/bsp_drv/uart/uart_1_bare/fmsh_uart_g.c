/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file  fmsh_uart_lib.c
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
* 0.01   wfb  11/23/2018  First Release
*</pre>
******************************************************************************/

/***************************** Include Files *********************************/
#include "../../common/ps_init.h"
#include "../../common/fmsh_ps_parameters.h"

#include "fmsh_uart_lib.h"
/*#include "fmsh_ps_parameters.h"*/

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/
FUartPs_Config FUartPs_ConfigTable[] =
{
	{
		FPAR_UARTPS_0_DEVICE_ID,
		FPAR_UARTPS_0_BASEADDR,
		FPAR_UARTPS_0_UART_CLK_FREQ_HZ,
		FPAP_UARTPS_0_HAS_MODEM
	},
        {
		FPAR_UARTPS_1_DEVICE_ID,
		FPAR_UARTPS_1_BASEADDR,
		FPAR_UARTPS_1_UART_CLK_FREQ_HZ,
		FPAP_UARTPS_1_HAS_MODEM
	}
};
/************************** Function Prototypes ******************************/
