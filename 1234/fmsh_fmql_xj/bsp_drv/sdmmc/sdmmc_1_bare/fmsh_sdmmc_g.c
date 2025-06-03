/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_sdmmc_g.c
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
* 0.01   yl  08/27/2019  First Release
*</pre>
******************************************************************************/

#include "../../common/fmsh_ps_parameters.h"

#include "fmsh_sdmmc.h"


/*
* The configuration table for devices
*/

FSdmmcPs_Config FSdmmcPs_CfgTbl[] =
{
    {
		FPAR_SDPS_0_DEVICE_ID,
		FPAR_SDPS_0_BASEADDR,
		FPAR_SDPS_0_SDIO_CLK_FREQ_HZ,
		FPAR_SDPS_0_HAS_CD,
		FPAR_SDPS_0_HAS_WP
    },
    
    {
		FPAR_SDPS_1_DEVICE_ID,
		FPAR_SDPS_1_BASEADDR,
		FPAR_SDPS_1_SDIO_CLK_FREQ_HZ,
		FPAR_SDPS_1_HAS_CD,
		FPAR_SDPS_1_HAS_WP
    }
};


