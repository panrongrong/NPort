/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_can_sint.c
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
#include "../../common/fmsh_ps_parameters.h"

#include "fmsh_can_lib.h"


/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/
extern FCanPs_Config FCanPs_ConfigTable[];

/************************** Function Prototypes ******************************/

/****************************************************************************
*
*  This function look up the CAN by device id. 
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
FCanPs_Config *FCanPs_LookupConfig(u16 DeviceId)
{
    FCanPs_Config *CfgPtr=NULL;
    
    u32 Index;
    
    for(Index=0;Index<FPAR_CANPS_NUM_INSTANCES;Index++)
    {
        if(FCanPs_ConfigTable[Index].DeviceId==DeviceId)
        {
            CfgPtr=&FCanPs_ConfigTable[Index];
            break;
        }
    }
    
    return (FCanPs_Config *)CfgPtr;
}
