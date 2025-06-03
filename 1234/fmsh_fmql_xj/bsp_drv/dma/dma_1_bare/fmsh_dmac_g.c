/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_dmac_g.c
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
#include <stdlib.h>

#include "../../common/fmsh_ps_parameters.h"
/*#include "fmsh_ps_parameters.h"*/

#include "fmsh_dmac.h"

/*
* The configuration table for devices
*/

FDmaPs_Config FDmaPs_ConfigTable[] =
{
    {
        FPAR_DMAPS_DEVICE_ID,
        FPAR_DMAPS_BASEADDR
    }
};


