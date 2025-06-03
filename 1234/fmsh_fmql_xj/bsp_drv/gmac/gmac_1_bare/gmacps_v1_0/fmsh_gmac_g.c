/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_gmac_g.c
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 0.01   yhs  08/30/2019  First Release
*</pre>
******************************************************************************/

#include "fmsh_gmac_lib.h"
#include "../../../common/fmsh_ps_parameters.h"

/*
* The configuration table for devices
*/

FGmacPs_Config_T FGmacPs_ConfigTable[] =
{
    {
        FPAR_GMACPS_0_DEVICE_ID,
        FPAR_GMACPS_0_BASEADDR,
        FPAR_GMACPS_0_SPEED,      /* default: 1G */
        FPAR_GMACPS_0_INTERFACE   /* default: rgmii */
    },
    {
        FPAR_GMACPS_1_DEVICE_ID,
        FPAR_GMACPS_1_BASEADDR,
        FPAR_GMACPS_1_SPEED,
        FPAR_GMACPS_1_INTERFACE
    }
};
