#include <vxWorks.h>
#include <intLib.h>
#include <logLib.h>
#include <muxLib.h>
#include <semLib.h>
#include <sysLib.h>
#include <vxBusLib.h>
#include <wdLib.h>
#include <string.h>

#include "fmsh_qspips_lib.h"

/*****************************************************************************
* This function executes WREN.
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiFlash_WREN(FQspiPs_T* qspi) 
{
    int error = 0;
    u8 status;
    u32 timeout = 0; 
    /* Send WREN(0x06) Command */
    error = FQspiPs_CmdExecute(qspi, 0x06000001);    
    if(error){
        return FMSH_FAILURE;
    }
    while(1){     
        /* poll status.wel value */
        status = FQspiFlash_GetStatus1(qspi);
        if(status & WEL_MASK){
            return FMSH_SUCCESS;
        }
        delay_1ms();
        timeout++;
        if(timeout > 750){
            return FMSH_FAILURE;
        }
    }
}

/*****************************************************************************
* This function executes WRDI.
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiFlash_WRDI(FQspiPs_T* qspi)
{ 
    int error = 0;
    u8 status;
    u32 timeout = 0;
    /* Send WREN(0x04) Command */
    error = FQspiPs_CmdExecute(qspi, 0x04000001);
    if(error){
        return FMSH_FAILURE;
    }
    while(1){
        /* poll status.wel value    */
        status = FQspiFlash_GetStatus1(qspi);
        status = ~status;
        if(status & WEL_MASK){
            return FMSH_SUCCESS;
        }
        delay_1ms();
        timeout++;
        if(timeout > 750){
            return FMSH_FAILURE;
        }
    }
}

/*****************************************************************************
* This function executes READ STATUS1.
*
* @param
*
* @return
*		- Status value.
*
* @note		
*
******************************************************************************/
u8 FQspiFlash_GetStatus1(FQspiPs_T* qspi)
{
    /* Send Read Status1 Register command(0x05) to device */
    FQspiPs_CmdExecute(qspi, 0x05800001);
    
    return FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_FCRDRL_OFFSET) & 0xff;
} 

/*****************************************************************************
* This function executes READ 16 bits register.
*
* @param
*
* @return
*		- Register value.
*
* @note		
*
******************************************************************************/
u16 FQspiFlash_GetReg16(FQspiPs_T *qspi, u8 cmd)
{
    u32 reg;
    u16 value;
    
    reg = 0x00900001 | (cmd << 24);
    /* Read Register command from device (2B) */
    FQspiPs_CmdExecute(qspi, reg);    
    value = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_FCRDRL_OFFSET) & 0xffff;
    return value;
} 

/*****************************************************************************
* This function executes READ 8 bits register.
*
* @param
*
* @return
*		- Register value.
*
* @note		
*
******************************************************************************/
u8 FQspiFlash_GetReg8(FQspiPs_T *qspi, u8 cmd)
{
    u32 reg;
    u8 value;
    
    reg = 0x00800001 | (cmd << 24);
    /* Read Register command from device (1B) */
    FQspiPs_CmdExecute(qspi, reg);    
    value = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_FCRDRL_OFFSET) & 0xff;
    return value;
} 

/*****************************************************************************
* This function executes WRITE 16 bits register.
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiFlash_SetReg16(FQspiPs_T* qspi, u8 cmd, u8 high_value, u8 low_value)
{
    int error;
    u32 reg;
    
    reg = 0x00009001 | (cmd << 24);
    /* Set value to be sent*/
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_FCWDRL_OFFSET, ((high_value << 8) | low_value));  
	
    error = FQspiFlash_WREN(qspi);
    if(error){
        return FMSH_FAILURE;
    }
    /*Send command to device*/
    FQspiPs_CmdExecute(qspi, reg);
	
    error = FQspiFlash_WaitForWIP(qspi);
    if(error){
        return FMSH_FAILURE;
    }
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function executes WRITE 8 bits register.
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiFlash_SetReg8(FQspiPs_T* qspi, u8 cmd, u8 value)
{
    int error;
    u32 reg;
    
    reg = 0x00008001 | (cmd << 24);
    /* Set value to be sent */
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_FCWDRL_OFFSET, (u32)value);
    
    error = FQspiFlash_WREN(qspi);
    if(error){
        return 1;
    }
    
    /* Send command to device */
    FQspiPs_CmdExecute(qspi, reg);
    
    error = FQspiFlash_WaitForWIP(qspi);
    if(error){
        return FMSH_FAILURE;
    }
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function executes READ ID.
*
* @param
*
* @return
*		- ID value.
*
* @note		
*
******************************************************************************/
#if 0
u32 FQspiFlash_ReadId(FQspiPs_T* qspi)
{
    /* Send RDID Command (4 bytes) */
    FQspiPs_CmdExecute(qspi, 0x9FD00001);
    /*Get ID */
    return FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_FCRDRL_OFFSET);
}
#else
u32 FQspiFlash_ReadId(FQspiPs_T* qspi)
{
	u32 tmp32 = 0;
	u32 tmp32_2 = 0;
	
    /* Send RDID Command (4 bytes) */
    FQspiPs_CmdExecute(qspi, 0x9FD00001);
	
    /* Get ID */
    tmp32 = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_FCRDRL_OFFSET);
	
	tmp32_2 = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_FCRDRU_OFFSET);
	printf("flashID: 0x%08X-%08X \n", tmp32, tmp32_2);
	
	return tmp32;
}
#endif
/*****************************************************************************
* This function executes READ STATUS1 and wait for WIP.
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if overtime.
*
* @note		
*
******************************************************************************/
int FQspiFlash_WaitForWIP(FQspiPs_T* qspi)
{ 
    u8 status;
    u32 timeout = 0;
    
    /* Poll Status Register1 */
    while(1){
        status = FQspiFlash_GetStatus1(qspi);
        status = ~status;
        status &= BUSY_MASK;
        if(status == BUSY_MASK){
            return FMSH_SUCCESS;
        }    
        delay_1ms();
        timeout++;
        if(timeout > 1500){
            return FMSH_FAILURE;
        }
    }
}

/*****************************************************************************
* This function executes SECTOR ERASE.
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiFlash_SectorErase(FQspiPs_T* qspi, u32 start_offset, u8 cmd)
{
    int error;
    u32 reg;
    
    /*Set Flash Command Address */
    reg = qspi->config.dataBaseAddress + start_offset;
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_FCAR_OFFSET, reg);
    
    error = FQspiFlash_WREN(qspi);
    if(error){
        return FMSH_FAILURE;
    }
    
    /*Send Erase Command */
    reg = 0x000A0001 | (cmd << 24);
    error = FQspiPs_CmdExecute(qspi, reg);
    if(error){
        return FMSH_FAILURE;
    }
	
    /* Poll Status Register1 */
    u8 status;
    u32 timeout = 0;
	
    while (1)
	{
        status = FQspiFlash_GetStatus1(qspi);
        status = ~status;
        status &= BUSY_MASK;
		
        if(status == BUSY_MASK)
		{
            return FMSH_SUCCESS;
        }    
		
        delay_1ms();
        timeout++;
        if(timeout > 5000)
		{
            return FMSH_FAILURE;
        }
    }
}

/*****************************************************************************
* This function executes CHIP ERASE.
*
* @param
*
* @return
*		- FMSH_SUCCESS if success.
*		- FMSH_FAILURE if failure.
*
* @note		
*
******************************************************************************/
int FQspiFlash_ChipErase(FQspiPs_T* qspi, u8 cmd)
{
    int error;
    u32 reg;
    
    error = FQspiFlash_WREN(qspi);
    if(error){
        return FMSH_FAILURE;
    }
    
    /*Send Erase Command */
    reg = 0x00000001 | (cmd << 24);
    error = FQspiPs_CmdExecute(qspi, reg);
    if(error){
        return FMSH_FAILURE;
    }
	
    /* Poll Status Register1 */
    u8 status;
    u32 timeout = 0;
    while(1)
	{
        status = FQspiFlash_GetStatus1(qspi);
        status = ~status;
        status &= BUSY_MASK;
		
        if(status == BUSY_MASK)
		{
            return FMSH_SUCCESS;
        }    
		
        delay_1ms();
        timeout++;
        if(timeout > 600000){
            return FMSH_FAILURE;
        }
    }
}
