/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_gmac_sint.c
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
* 0.01   yhs  08/30/2019  First Release
*</pre>
******************************************************************************/

/***************************** Include Files *********************************/
#include "fmsh_gmac_lib.h"
#include "../../../common/fmsh_ps_parameters.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/
extern FGmacPs_Config_T FGmacPs_ConfigTable[];

/************************** Function Prototypes ******************************/

/****************************************************************************/
/**
*
* Looks up the device configuration based on the unique device ID. The table
* contains the configuration info for each device in the system.
*
* @param DeviceId contains the ID of the device
*
* @return
*
* A pointer to the configuration structure or NULL if the specified device
* is not in the system.
*
* @note
*
* None.
*
******************************************************************************/
FGmacPs_Config_T *FGmacPs_LookupConfig(u16 DeviceId)
{
    FGmacPs_Config_T *CfgPtr = NULL;

    int i;

    for (i = 0; i < FPAR_GMACPS_NUM_INSTANCES; i++)
    {
        if (FGmacPs_ConfigTable[i].DeviceId == DeviceId)
        {
            CfgPtr = &FGmacPs_ConfigTable[i];
            break;
        }
    }

    return CfgPtr;
}

