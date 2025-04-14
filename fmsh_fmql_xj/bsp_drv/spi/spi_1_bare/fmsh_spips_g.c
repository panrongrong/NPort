#include "../../common/fmsh_ps_parameters.h"/*#include "fmsh_ps_parameters.h"*/
#include "fmsh_spips_lib.h"

/******************************************************************************
* This table contains configuration information for each SPI
* device in the system.
******************************************************************************/
FSpiPs_Config_T FSpiPs_ConfigTable[] =
{
    {
        FPS_SPI0_DEVICE_ID,
        FPS_SPI0_BASEADDR
    },
    {
        FPS_SPI1_DEVICE_ID,
        FPS_SPI1_BASEADDR  
    } 
}; 
