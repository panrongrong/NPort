/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file  fmsh_i2c_g.c
*
* This file contains global variable which use to config.
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

#include "fmsh_can_lib.h"     

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/
FCanPs_Config FCanPs_ConfigTable[] =
{
	{
		FPAR_CANPS_0_DEVICE_ID,
		FPAR_CANPS_0_BASEADDR,
		FPAR_CANPS_0_CAN_CLK_FREQ_HZ
	},
        {
		FPAR_CANPS_1_DEVICE_ID,
		FPAR_CANPS_1_BASEADDR,
		FPAR_CANPS_1_CAN_CLK_FREQ_HZ
	}
};
/************************** Function Prototypes ******************************/
