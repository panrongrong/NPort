#include "fmsh_ps_parameters.h"
#include "fmsh_qspips_lib.h"

/******************************************************************************
* This table contains configuration information for each QSPI
* device in the system.
******************************************************************************/
FQspiPs_Config_T FQspiPs_ConfigTable[] =
{
    {
        FPS_QSPI0_DEVICE_ID,
        FPS_QSPI0_D_BASEADDR,
        FPS_QSPI0_BASEADDR,
    },
    {
        FPS_QSPI1_DEVICE_ID,
        FPS_QSPI1_D_BASEADDR,
        FPS_QSPI1_BASEADDR,
    }
};