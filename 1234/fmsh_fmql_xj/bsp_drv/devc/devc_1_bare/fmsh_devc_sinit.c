/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_devc_sint.c
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
* 0.01   lq  08/27/2019  First Release
*</pre>
******************************************************************************/

/***************************** Include Files *********************************/
#include <stdlib.h>

#include "../../common/ps_init.h"
#include "../../common/fmsh_ps_parameters.h"

#include "fmsh_devc_lib.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/
extern FDevcPs_Config FDevcPs_ConfigTable[];

/************************** Function Prototypes ******************************/

/****************************************************************************
*
*  This function look up the devc by device id. 
*
* @param   DeviceId is device id. 
*
* @return  
* - point to the corresponding device
* - NULL invalid device id
*
* @note    none
*
****************************************************************************/
FDevcPs_Config *FDevcPs_LookupConfig(u16 DeviceId)
{
    FDevcPs_Config *CfgPtr=NULL;
    
    u32 Index;
    
    for(Index=0;Index<FPAR_DEVCPS_NUM_INSTANCES;Index++)
    {
        if(FDevcPs_ConfigTable[Index].DeviceId==DeviceId)
        {
            CfgPtr=&FDevcPs_ConfigTable[Index];
            break;
        }
    }
    
    return (FDevcPs_Config *)CfgPtr;
}
