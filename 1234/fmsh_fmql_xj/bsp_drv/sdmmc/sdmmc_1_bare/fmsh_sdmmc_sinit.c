/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_sdmmc_sint.c
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

/***************************** Include Files *********************************/
#include "../../common/fmsh_ps_parameters.h"

#include "fmsh_sdmmc.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/
extern FSdmmcPs_Config FSdmmcPs_CfgTbl[];

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
FSdmmcPs_Config *FSdmmcPs_LookupConfig(u16 DeviceId)
{
    FSdmmcPs_Config *CfgPtr = NULL;

    int i;

    for (i = 0; i < FPAR_SDPS_NUM_INSTANCES; i++)
    {
        if (FSdmmcPs_CfgTbl[i].DeviceId == DeviceId)
        {
            CfgPtr = &FSdmmcPs_CfgTbl[i];
            break;
        }
    }

    return CfgPtr;
}

