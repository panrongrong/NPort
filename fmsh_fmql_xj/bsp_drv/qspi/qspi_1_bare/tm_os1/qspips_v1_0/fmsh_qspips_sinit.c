#include "fmsh_ps_parameters.h"
#include "fmsh_qspips_lib.h"

extern FQspiPs_Config_T FQspiPs_ConfigTable[];

/*****************************************************************************
* This function looks up the device configuration based on the unique device ID.
* The table s_ConfigTable contains the configuration info for each device
* in the system.
*
* @param	
*       - DeviceId contains the ID of the device for which the
*		device configuration pointer is to be returned.
*
* @return
*		- A pointer to the configuration found.
*		- NULL if the specified device ID was not found.
*
* @note		None.
*
******************************************************************************/
FQspiPs_Config_T *FQspiPs_LookupConfig(u16 deviceId)
{
    int index;
    FQspiPs_Config_T *cfgPtr = NULL;
    
    for (index = 0; index < FPAR_QSPIPS_NUM_INSTANCES; index++) 
    {
        if (FQspiPs_ConfigTable[index].deviceId == deviceId) 
        {
            cfgPtr = &FQspiPs_ConfigTable[index];
            break;
        }
    }
    return cfgPtr;
}