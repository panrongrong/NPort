/* vxi2c.c - fmsh 7020/7045 pI2c driver */

/*
 * Copyright (c) 2013 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */
 
/*
modification history
--------------------
01a, 04Jun20, jc  written.
*/

/*
DESCRIPTION
	no vxbus struct
*/

/* includes */

#include <vxWorks.h>
#include <stdio.h>
#include <semLib.h>
#include <sysLib.h>
#include <taskLib.h>
#include <vxBusLib.h>
#include <cacheLib.h>
#include <string.h>
#include <vxbTimerLib.h>

#include "vxi2c.h"

/*
defines 
*/
#if 1

#undef VX_DBG
#undef VX_DBG2

#define I2C_DBG

#ifdef I2C_DBG

#define VX_DBG(string, a, b, c, d, e, f)                    \
        if (_func_logMsg != NULL)                           \
           printf(string, a, b, c, d, e, f)  /* (* _func_logMsg)(string, a, b, c, d, e, f) */
#else
#define VX_DBG(string, a, b, c, d, e, f)
#endif

/* error info log */
#define VX_DBG2(string, a, b, c, d, e, f) printf(string, a, b, c, d, e, f)
#endif



/* 
i2c_0 : master
*/
static vxT_I2C_CTRL vxI2c_Ctrl_0 = {0};

static vxT_I2C vx_pI2c_0 = {0};
vxT_I2C * g_pI2c0 = (vxT_I2C *)(&vx_pI2c_0.i2c_x);

/* 
i2c_1 : slave
*/
static vxT_I2C_CTRL vxI2c_Ctrl_1 = {0};

static vxT_I2C vx_pI2c_1 = {0};
vxT_I2C * g_pI2c1 = (vxT_I2C *)(&vx_pI2c_1.i2c_x);


#if 1

void i2cCtrl_Wr_CfgReg32(vxT_I2C* pI2c, UINT32 offset, UINT32 value)
{
	UINT32 tmp32 = pI2c->pI2cCtrl->cfgBaseAddr;
	FMQL_WRITE_32((tmp32 + offset), value);
	return;
}

UINT32 i2cCtrl_Rd_CfgReg32(vxT_I2C* pI2c, UINT32 offset)
{
	UINT32 tmp32 = pI2c->pI2cCtrl->cfgBaseAddr;
	return FMQL_READ_32(tmp32 + offset);
}



/*****************************************************************************
*
* @description
*  This function enables the I2C.
*
* @param   pI2c is i2c device handle. 
*
* @return  
*  none
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_enable/enable
* SEE ALSO
*  i2cCtrl_disable(), i2cCtrl_isEnable()
* SOURCE
*
****************************************************************************/
void i2cCtrl_Enable(vxT_I2C* pI2c)
{
    UINT32 reg;

    reg = 0;
	
    /*FMSH_BIT_SET(reg, I2C_ENABLE_ENABLE, 0x1);*/
	reg |= 0x01;
	
    /*I2C_OUTP(reg, portmap->enable);*/
    i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_ENABLE, reg);

	return;
}

/*****************************************************************************
*
* @description
*  This functions disables the I2C, if it is not busy (determined by
*  the activity interrupt bit).  The I2C should not be disabled during
*  interrupt-driven transfers as the resulting driver behavior is
*  undefined.
*
* @param   pI2c is i2c device handle. 
*
* @return  
*  0           -- if successful
*  -FMSH_EBUSY   -- if the I2C is busy
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_enable/enable
* SEE ALSO
*  i2cCtrl_disable(), i2cCtrl_isEnable()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Disable(vxT_I2C* pI2c)
{
    int retval;
    UINT32 reg;

    if (i2cCtrl_isBusy(pI2c) == 0) 
	{
        reg = 0;
        /*FMSH_BIT_SET(reg, I2C_ENABLE_ENABLE, 0);*/
        /*I2C_OUTP(reg, portmap->enable);		*/
	    i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_ENABLE, reg);
		
		retval = 0; 	
    }
    else
    {    
    	retval = -FMSH_EBUSY;
    }

    return retval;
}


/*****************************************************************************
*
* @description
*   This function returns whether the I2C is enabled or not.
*
* @param   pI2c is i2c device handle. 
*
* @return  
*  true        -- the I2C is enabled
*  false       -- the I2C is disabled
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_enable/enable
* SEE ALSO
*  i2cCtrl_disable(), i2cCtrl_isEnable()
* SOURCE
*
****************************************************************************/
int i2cCtrl_isEnable(vxT_I2C* pI2c)
{
    int retval;
    UINT32 reg;

    /*reg = I2C_INP(portmap->enable);	*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_ENABLE);
	
    /*retval = (bool) FMSH_BIT_GET(reg, I2C_ENABLE_ENABLE);*/
    retval = reg & 0x01;

    return retval;
}

/*****************************************************************************
*
* @description
*   This function returns whether the I2C is busy (transmitting
*   or receiving) or not.
*
* @param   pI2c is i2c device handle. 
*
* @return  
*  true        -- the I2C device is busy
*  false       -- the I2C device is not busy
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_status/activity
* SEE ALSO
* SOURCE
*
****************************************************************************/
int i2cCtrl_isBusy(vxT_I2C* pI2c)
{
    int retval;
    UINT32 reg;

    /*reg = I2C_INP(portmap->status);*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_STATUS);
    
    /*if (FMSH_BIT_GET(reg, I2C_STATUS_ACTIVITY) == 0x0)*/
    if ((reg & I2C_STATUS_ACTIVITY) == 0x0)
    {    
    	retval = 0;
	}
    else
    {    
    	retval = 1;
	}

    return retval;
}

/*****************************************************************************
*
* @description
*   This function sets the speed mode used for I2C transfers.
*
* @param   
* pI2c is i2c device handle.
* mode is the speed mode to set 
*
* @return  
*  0           -- if successful
*  -FMSH_EPERM   -- if the I2C is enabled
*  -FMSH_ENOSYS  -- if the specified speed is not supported
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_con/speed
* The I2C must be disabled in order to change the speed mode.
*  This function is affected by the MAX_SPEED_MODE hardware parameter.
*  It is not possible to set the speed higher than the maximum speed
*  mode.
* SEE ALSO
*  i2cCtrl_getSpeedMode(), FIicPs_SpeedMode_T
* SOURCE
*
****************************************************************************/
int i2cCtrl_Set_SpeedMode(vxT_I2C* pI2c, int mode)
{
    int retval;
    UINT32 reg;
	
    /*
    param = (FIicPs_Param_T*) pI2c->comp_param;

    if (mode > param->max_speed_mode)
        retval = -FMSH_ENOSYS;
    else 
	*/	
	/*if (i2cCtrl_isEnable(pI2c) == false) */
	if (i2cCtrl_isEnable(pI2c) == 0) 
	{
        /*reg = I2C_INP(portmap->con);*/
		reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CON);
		
        /* avoid bus write if possible*/
        /*if (FMSH_BIT_GET(reg, I2C_CON_SPEED) != mode) */
        /*
        bit2~1:	SPEED
				1：standard mode
				2：fast mode or fast plus mode
		*/
        if (((reg & 0x06) >> 1) != mode) 
		{
            /*FMSH_BIT_SET(reg, I2C_CON_SPEED, mode);*/
            /*I2C_OUTP(reg, portmap->con);*/
        	reg &= (~0x6);
            reg |= (mode << 1);
            i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_CON, reg);
        }
		
		retval = 0;
    }
    else
    {    
    	retval = -FMSH_EPERM;
    }

    return retval;
}
/*****************************************************************************
*
* @description
*   This function returns the speed mode currently in use by the I2C.
*
* @param   
* pI2c is i2c device handle.
*
* @return  
*  The current I2C speed mode.
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_con/speed
* SEE ALSO
*  i2cCtrl_setSpeedMode(), FIicPs_SpeedMode_T
* SOURCE
*
****************************************************************************/
int i2cCtrl_Get_SpeedMode(vxT_I2C* pI2c)
{
    UINT32 reg;
	int retval = 0;

   /* reg = I2C_INP(portmap->con);*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CON);
   
    /*retval = (FIicPs_SpeedMode_T) FMSH_BIT_GET(reg, I2C_CON_SPEED);*/
    /*
	bit2~1: SPEED
			1：standard mode(100 kbps)
			2：fast mode(400 kbps) or fast plus mode(3400 kbps)
	*/
	retval = (reg & 0x06) >> 1;

    return retval;
}
/*****************************************************************************
*
* @description
*   This function sets the pI2c addressing mode (7-bit or 10-bit).
*
* @param   
* pI2c is i2c device handle.
*  mode is the addressing mode to set.
*
* @return  
*  0           -- if successful
*  -FMSH_EPERM   -- if the I2C is enabled
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_con/ic_10bitaddr_master
* 
*  The I2C must be disabled in order to change the pI2c addressing
*  mode.
* SEE ALSO
*  i2cCtrl_getMasterAddressMode(), enum i2cCtrl_address_mode
* SOURCE
*
****************************************************************************/
int i2cCtrl_Master_Set_AddrMode(vxT_I2C* pI2c, int addr_mode)
{
    int retval;
    UINT32 reg;

    if (i2cCtrl_isEnable(pI2c) == 0) 
	{
        retval = 0;
		
        /*reg = I2C_INP(portmap->tar);*/
		reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_TAR);
		
    	/*  
    	if(FMSH_BIT_GET(reg, I2C_CON_10BITADDR_MASTER) != mode) 
    	{
            FMSH_BIT_SET(reg, I2C_CON_10BITADDR_MASTER, mode);
            I2C_OUTP(reg, portmap->con);
        }*/
        /*if (FMSH_BIT_GET(reg, I2C_TAR_10BITADDR_MASTER) != mode) */
        /*
		bit12:	IC_10BITMASTER	R/W 主机时，决定寻址方式
				0 - 7bit寻址
				1 - 10bit寻址,  复位值：1
		*/
        if (((reg & I2C_TAR_ADR_10BIT) >> 12) != addr_mode)
		{
            /*FMSH_BIT_SET(reg, I2C_TAR_10BITADDR_MASTER, mode);			*/
            /*I2C_OUTP(reg, portmap->tar);            */
            if (addr_mode == I2C_10BIT_ADDR)
            {
            	reg |= I2C_TAR_ADR_10BIT;
            }
			else
			{
				reg &= ~I2C_TAR_ADR_10BIT;
			}
			
			i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_TAR, reg);
        }
    }
    else
    {    
    	retval = -FMSH_EPERM;
    }

    return retval;
}

/*****************************************************************************
*
* @description
*   This function returns the current pI2c addressing mode (7-bit or
*  10-bit).
*
* @param   
*  pI2c is i2c device handle.
*  
*
* @return  
*  The current pI2c addresing mode.
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_con/ic_10bitaddr_master
* SEE ALSO
*  i2cCtrl_setSlaveAddressMode(), enum i2cCtrl_address_mode
* SOURCE
*
****************************************************************************/
int i2cCtrl_Master_Get_AddrMode(vxT_I2C  * pI2c)
{
    UINT32 reg;
    int retval;

	/* reg = I2C_INP(portmap->con);*/
	/* retval = (FIicPs_AddressMode_T) FMSH_BIT_GET(reg, I2C_CON_10BITADDR_MASTER);   */
	/*
	bit4:   IC_10BITADDR_MASTER_rd_only   R   表示主机所用的寻址方式
		   0 - 7bit 寻址
		   1 - 10bit寻址,   复位值：0x1
	*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CON);
	retval = (reg & I2C_CON_MASTER_ADR_10BIT ) >> 4;

    return retval;
}

/*****************************************************************************
*
* @description
*   This function sets the I2C slave addressing mode (7-bit or 10-bit).
*
* @param   
*  pI2c is i2c device handle.
*   mode is the addressing mode to set.
*
* @return  
*  0           -- if successful
*  -FMSH_EPERM   -- if the I2C is enabled
*
* @note    
*   Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_con/ic_10bitaddr_slave
* 
*  The I2C must be disabled in order to change the slave addressing
*  mode.
* SEE ALSO
*  i2cCtrl_getSlaveAddressMode(), enum i2cCtrl_address_mode
* SOURCE
*
****************************************************************************/
int i2cCtrl_Slave_Set_AddrMode(vxT_I2C* pI2c, int addr_mode)
{
    int retval;
    UINT32 reg;

    if (i2cCtrl_isEnable(pI2c) == 0) 
	{
        /* reg = I2C_INP(portmap->con);*/
		reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CON);
		
        /* avoid bus write if possible*/
        /*if (FMSH_BIT_GET(reg, I2C_CON_10BITADDR_SLAVE) != mode) */
        /*
		bit3	IC_10BITADDR_SLAVE	R/W    作为从机工作时，控制着I2C响应的寻址方式
				0 - 7bit寻址，10bit寻址的数据传输将会被忽略
				1 - 10bit寻址，7bit寻址的数据传输将会被忽略, 				复位值：0x1
		*/
        if (((reg & I2C_CON_SLAVE_ADR_10BIT) >> 3) != addr_mode) 
		{
            /* FMSH_BIT_SET(reg, I2C_CON_10BITADDR_SLAVE, mode);*/
            /* I2C_OUTP(reg, portmap->con);*/
            if (addr_mode == I2C_10BIT_ADDR)
        	{
            	reg &= ~I2C_CON_SLAVE_ADR_7BIT;
	            reg |= I2C_CON_SLAVE_ADR_10BIT;
        	}
			else
			{
				reg &= ~I2C_CON_SLAVE_ADR_10BIT;
				reg |= I2C_CON_SLAVE_ADR_7BIT;
			}
			
			i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_CON, reg);            
        }
		
		retval = 0;
    }
    else
    {    
    	retval = -FMSH_EPERM;
    }

    return retval;
}
/*****************************************************************************
*
* @description
*   This function returns the current slave addressing mode (7-bit or
*  10-bit).
*
* @param   
*  pI2c is i2c device handle.
*  
*
* @return  
*  The current slave addressing mode.
*
* @note    
*  AAccesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_con/ic_10bitaddr_slave
* SEE ALSO
*  i2cCtrl_setSlaveAddressMode(), enum i2cCtrl_address_mode
* SOURCE
*
****************************************************************************/
int i2cCtrl_Slave_Get_AddrMode(vxT_I2C* pI2c)
{
    UINT32 reg;
    int retval;

    /*
    reg = I2C_INP(portmap->con);
    retval = (FIicPs_AddressMode_T) FMSH_BIT_GET(reg, I2C_CON_10BITADDR_SLAVE);
	*/	
	/*
	bit3	IC_10BITADDR_SLAVE	R/W    作为从机工作时，控制着I2C响应的寻址方式
			0 - 7bit寻址，10bit寻址的数据传输将会被忽略
			1 - 10bit寻址，7bit寻址的数据传输将会被忽略, 				复位值：0x1
	*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CON);
	retval = (reg & I2C_CON_SLAVE_ADR_10BIT ) >> 3;

    return retval;
}

/*****************************************************************************
*
* @description
*   This function enables the I2C slave.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*   0           -- if successful
*  -FMSH_EPERM   -- if the I2C is enabled
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_con/ic_slave_disable
* 
*  The I2C must be disabled in order to enable the slave.
* SEE ALSO
*  i2cCtrl_disableSlave(), i2cCtrl_isSlaveEnabled()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Slave_Enable(vxT_I2C* pI2c)
{
    int retval;
    UINT32 reg;

    if (i2cCtrl_isEnable(pI2c) == 0) 
	{		
        /*reg = I2C_INP(portmap->con);*/
        reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CON);
		
        /* avoid bus write if possible*/
        /*if (FMSH_BIT_GET(reg, I2C_CON_SLAVE_DISABLE) != 0x0)*/
        /*
		bit6:	IC_SLAVE_DISABLE	R/W 控制着I2C从机是否禁止
				1 - I2C从机被禁止
				0 - I2C从机使能,    	复位值：0x1		
		*/
        if ((reg & I2C_CON_SLAVE_DISABLE) != 0x0)
        {
           /* FMSH_BIT_SET(reg, I2C_CON_SLAVE_DISABLE, 0x0);*/
           /* I2C_OUTP(reg, portmap->con);*/
           reg &= ~I2C_CON_SLAVE_DISABLE;
		   i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_CON, reg);
        }
		
		retval = 0;
    }
    else
    {    
    	retval = -FMSH_EPERM;
    }

    return retval;
}
/*****************************************************************************
*
* @description
*   This function disables the I2C slave.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*   0           -- if successful
*  -FMSH_EPERM   -- if the I2C is enabled
*
* @note    
*   Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_con/ic_slave_disable
* 
*  The I2C must be disabled in order to disable the slave.
* SEE ALSO
*  i2cCtrl_enableSlave(), i2cCtrl_isSlaveEnabled()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Slave_Disable(vxT_I2C* pI2c)
{
    int retval;
    UINT32 reg;

    if (i2cCtrl_isEnable(pI2c) == 0) 
	{		
        /*reg = I2C_INP(portmap->con);*/
        reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CON);
		
        /* avoid bus write if possible*/
        /*if (FMSH_BIT_GET(reg, I2C_CON_SLAVE_DISABLE) != 0x1) */
        
        /*
		bit6:	IC_SLAVE_DISABLE	R/W 控制着I2C从机是否禁止
				1 - I2C从机被禁止
				0 - I2C从机使能,    	复位值：0x1		
		*/
        if (((reg & I2C_CON_SLAVE_DISABLE) >> 6)  != 0x1)         
		{
            /*FMSH_BIT_SET(reg, I2C_CON_SLAVE_DISABLE, 0x1);*/
            /*I2C_OUTP(reg, portmap->con);*/
            reg |= I2C_CON_SLAVE_DISABLE;
			i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_CON, reg);			
        }
		
		retval = 0;
    }
    else
    {    
    	retval = -FMSH_EPERM;
    }

    return retval;
}
/*****************************************************************************
*
* @description
*   This function returns whether the I2C slave is enabled or not.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  true        -- slave is enabled
*  false       -- slave is disabled
*
* @note    
*   Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_con/ic_slave_disable
* SEE ALSO
*  i2cCtrl_enableSlave(), i2cCtrl_disableSlave()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Slave_isEnable(vxT_I2C* pI2c)
{
    int retval;
    UINT32 reg;

    /*reg = I2C_INP(portmap->con);*/
    reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CON);
	
    /*if (FMSH_BIT_GET(reg, I2C_CON_SLAVE_DISABLE) == 0x0)*/
	/*
	bit6:	IC_SLAVE_DISABLE	R/W 控制着I2C从机是否禁止
			1 - I2C从机被禁止
			0 - I2C从机使能,		复位值：0x1 	
	*/
	if (((reg & I2C_CON_SLAVE_DISABLE) >> 6) == 0x0)		  
    {    
    	retval = 1;
	}
    else
    {    
    	retval = 0;
    }

    return retval;
}

/*****************************************************************************
*
* @description
*   This function enables the I2C pI2c.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  0           -- if successful
*  -FMSH_EPERM   -- if the I2C is enabled
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_con/master_mode
* 
*  The I2C must be disabled in order to enable the pI2c.
* SEE ALSO
*  i2cCtrl_disableMaster(), i2cCtrl_isMasterEnabled()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Master_Enable(vxT_I2C* pI2c)
{
    int retval;
    UINT32 reg;

    if (i2cCtrl_isEnable(pI2c) == 0) 
	{
        /*reg = I2C_INP(portmap->con);*/
		reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CON);
		
        /* avoid bus write if possible*/
        /*if (FMSH_BIT_GET(reg, I2C_CON_MASTER_MODE) != 0x1)*/
        /*
		bit0:	MASTER_MODE R/W I2C主机是否使能
				0 - 禁止
				1 - 使能,   	复位值：0x1
		*/
        if ((reg & I2C_CON_ME) != 0x1) 
		{
            /*FMSH_BIT_SET(reg, I2C_CON_MASTER_MODE, 0x1);*/
            /*I2C_OUTP(reg, portmap->con);*/
            reg |= 0x01;
			i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_CON, reg);
        }
		
		retval = 0;
    }
    else
    {    
    	retval = -FMSH_EPERM;
    }

    return retval;
}

/*****************************************************************************
*
* @description
*   This function disables the I2C pI2c.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  0           -- if successful
*  -FMSH_EPERM   -- if the I2C is enabled
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_con/master_mode
* 
*  The I2C must be disabled in order to disable the pI2c.
* SEE ALSO
*  i2cCtrl_enableMaster(), i2cCtrl_isMasterEnabled()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Master_Disable(vxT_I2C* pI2c)
{
    int retval;
    UINT32 reg;

    if (i2cCtrl_isEnable(pI2c) == 0)
	{		
        /*reg = I2C_INP(portmap->con);		*/
		reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CON);
		
        /* avoid bus write if possible*/
        /*if (FMSH_BIT_GET(reg, I2C_CON_MASTER_MODE) != 0x0) */
        /*
		bit0:	MASTER_MODE R/W I2C主机是否使能
				0 - 禁止
				1 - 使能,   	复位值：0x1
		*/
        if ((reg & I2C_CON_ME) != 0x0) 
		{
            /*FMSH_BIT_SET(reg, I2C_CON_MASTER_MODE, 0x0);*/
            /*I2C_OUTP(reg, portmap->con);            */
			reg &= ~0x01;
			i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_CON, reg);
        }
		
		retval = 0;
    }
    else
    {    
    	retval = -FMSH_EPERM;
    }

    return retval;
}


/*****************************************************************************
*
* @description
*   This function returns whether the I2C pI2c is enabled or not.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  true        -- pI2c is enabled
*  false       -- pI2c is disabled
*
* @note    
*   Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_con/master_mode
* SEE ALSO
*  i2cCtrl_enableMaster(), i2cCtrl_disableMaster()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Master_isEnable(vxT_I2C* pI2c)
{
    UINT32 reg;
    int retval;

    /*reg = I2C_INP(portmap->con);	*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CON);
	
    /* if(FMSH_BIT_GET(reg, I2C_CON_MASTER_MODE) == 0x1)*/
	/*
	bit0:	MASTER_MODE R/W I2C主机是否使能
			0 - 禁止
			1 - 使能, 	复位值：0x1
	*/
	if ((reg & I2C_CON_ME) == 0x1) 
    {    
    	retval = 1;
	}
    else
    {    
    	retval = 0;
	}

    return retval;
}

/*****************************************************************************
*
* @description
*   This function enables the use of restart conditions.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  0           -- if successful
*  -FMSH_EPERM   -- if the I2C is enabled
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_con/ic_restart_en
*
*  The I2C must be disabled in order to enable restart conditions.
* SEE ALSO
*  i2cCtrl_disableRestart()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Restart_Enable(vxT_I2C* pI2c)
{
    int retval;
    UINT32 reg;

    if (i2cCtrl_isEnable(pI2c) == 0) 
	{
        /*reg = I2C_INP(portmap->con);*/
		reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CON);
		
        /* avoid bus write if possible*/
        /*if(FMSH_BIT_GET(reg, I2C_CON_RESTART_EN) != 0x1) */
        /*
		bit5:	IC_RESTART_EN	R/W 当作为主机工作时，控制是否发送Restart条件
				1 - 使能
				0 - 禁止, 复位值：0x1
		*/
        if (((reg & I2C_CON_RESTART_EN) >> 5) != 0x1) 
		{
            /* FMSH_BIT_SET(reg, I2C_CON_RESTART_EN, 0x1);*/
            /* I2C_OUTP(reg, portmap->con);*/
            reg |= I2C_CON_RESTART_EN;
			i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_CON, reg);
        }
		
		retval = 0;
    }
    else
    {    
    	retval = -FMSH_EPERM;
    }

    return retval;
}

/*****************************************************************************
*
* @description
*   This function disables the use of restart conditions.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  0           -- if successful
*  -FMSH_EPERM   -- if the I2C is enabled
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_con/ic_restart_en
*
*  The I2C must be disabled in order to disable restart conditions.
* SEE ALSO
*  i2cCtrl_enableRestart()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Restart_Disable(vxT_I2C* pI2c)
{
    int retval;
    UINT32 reg;

    if (i2cCtrl_isEnable(pI2c) == 0) 
	{		
        /*reg = I2C_INP(portmap->con);*/
		reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CON);
		
        /* avoid bus write if possible*/
        /*if (FMSH_BIT_GET(reg, I2C_CON_RESTART_EN) != 0x0)         */
        /*
		bit5:	IC_RESTART_EN	R/W 当作为主机工作时，控制是否发送Restart条件
				1 - 使能
				0 - 禁止, 复位值：0x1
		*/
        if (((reg & I2C_CON_RESTART_EN) >> 5) != 0x0) 
		{
            /*FMSH_BIT_SET(reg, I2C_CON_RESTART_EN, 0x0);*/
            /*I2C_OUTP(reg, portmap->con);            */
			reg &= ~I2C_CON_RESTART_EN;
			i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_CON, reg);          
        }
		
		retval = 0;
    }
    else
    {    
    	retval = -FMSH_EPERM;
    }

    return retval;
}

/*****************************************************************************
*
* @description
*   This function returns whether restart conditions are currently in
*  use or not by the I2C.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  true        -- restart conditions are enabled
*  false       -- restart conditions are disabled
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_con/ic_restart_en
* SEE ALSO
*  i2cCtrl_enableRestart(), i2cCtrl_disableRestart()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Restart_isEnable(vxT_I2C* pI2c)
{
    UINT32 reg;
    int retval;

    /*reg = I2C_INP(portmap->con);	*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CON);
	
    /*if (FMSH_BIT_GET(reg, I2C_CON_RESTART_EN) == 0x1)    */
	/*
	bit5:	IC_RESTART_EN	R/W 当作为主机工作时，控制是否发送Restart条件
			1 - 使能
			0 - 禁止, 复位值：0x1
	*/
	if (((reg & I2C_CON_RESTART_EN) >> 5) == 0x1)
    {    
    	retval = 1;
	}
    else
    {    
    	retval = 0;
    }

    return retval;
}


/*****************************************************************************
*
* @description
*   This function sets the target address used by the I2C pI2c.  When
*  not issuing a general call or using a start byte, this is the
*  address the pI2c uses when performing transfers over the I2C bus.
*
* @param   
*  pI2c is i2c device handle. 
*  address is target address to set
*
* @return  
*  0           -- if successful
*  -FMSH_EPERM   -- if the I2C is enabled
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_tar/ic_tar
* 
*  The I2C must be disabled in order to set the target address.  Only
*  the 10 least significant bits of the address are relevant.
* SEE ALSO
*  i2cCtrl_getTargetAddress()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Set_TargetAddr(vxT_I2C* pI2c, UINT16 address)
{
    int retval;
    UINT32 reg;

    if (i2cCtrl_isEnable(pI2c) == 0) 
	{
        /*reg = I2C_INP(portmap->tar);		*/
		reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_TAR);
		
        /* avoid bus write if possible*/
        /*if(FMSH_BIT_GET(reg, I2C_TAR_ADDR) != address) */
        /*
		bit9~0: IC_TAR	R/W 主机传输时所用的目标从机地址，
		                    当处于广播地址模式时，字段被忽略。当处于开始字节模式时，CPU需要写入该字段一次。
						复位值：0x055
		*/
        if ((reg & 0x3FF) != address) 
		{
            /*FMSH_BIT_SET(reg, I2C_TAR_ADDR, address);*/
            /*I2C_OUTP(reg, portmap->tar);*/
            reg &= ~ 0x3FF;
            reg |= (address & 0x3FF);
			i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_TAR, reg);
        }		
		
		retval = 0;
    }
    else
    {    
    	retval = -FMSH_EPERM;
    }

    return retval;
}

/*****************************************************************************
*
* @description
*  This function returns the current target address in use by the I2C
*  pI2c.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  The current target address.
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_tar/ic_tar
* SEE ALSO
*  i2cCtrl_setTargetAddress()
* SOURCE
*
****************************************************************************/
UINT16 i2cCtrl_Get_TargetAddr(vxT_I2C* pI2c)
{
    UINT32 reg;
    UINT16 retval;

    /*reg = I2C_INP(portmap->tar);	*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_TAR);
	
    /*retval = FMSH_BIT_GET(reg, I2C_TAR_ADDR);*/
    retval = reg & 0x3FF;

    return retval;
}

/*****************************************************************************
*
* @description
*  This function sets the slave address to which the I2C slave
*  responds, when enabled.
*
* @param   
*  pI2c is i2c device handle. 
*  address is slave address to set
*
* @return  
*  0           -- if successful
*  -FMSH_EPERM   -- if the I2C is enabled
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_sar/ic_sar
* 
*  The I2C must be disabled in order to set the target address.  Only
*  the 10 least significant bits of the address are relevant.
* SEE ALSO
*  i2cCtrl_getSlaveAddress()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Set_SlaveAddr(vxT_I2C* pI2c, UINT16 address)
{
    int retval;
    UINT32 reg;

    if (i2cCtrl_isEnable(pI2c) == 0) 
	{
        /*FMSH_BIT_SET(reg, I2C_SAR_ADDR, address);*/
        /*I2C_OUTP(reg, portmap->sar);*/
        /*
		bit9~0: IC_SAR	R/W I2C工作在从机模式下，保存I2C从机地址，
						仅当I2C模块被禁止时可以写入该寄存器
						复位值：0x055
		*/
		reg = address & 0x3FF;		
		i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_SAR, reg);
		
		retval = 0;
    }
    else
    {    
    	retval = -FMSH_EPERM;
    }

    return retval;
}

/*****************************************************************************
*
* @description
*  This function returns the current address in use by the I2C slave.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  The current I2C slave address.
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_sar/ic_sar
* SEE ALSO
*  i2cCtrl_setSlaveAddress()
* SOURCE
*
****************************************************************************/
UINT16 i2cCtrl_Get_SlaveAddr(vxT_I2C* pI2c)
{
    UINT32 reg;
    UINT16 retval;

    /*reg = I2C_INP(portmap->sar);*/
    /*retval = FMSH_BIT_GET(reg, I2C_SAR_ADDR);*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_SAR);
	retval = reg & 0x3FF;		

    return retval;
}

/*****************************************************************************
*
* @description
*  This function sets the pI2c transmit mode.  That is, whether to
*  use a start byte, general call, or the programmed target address.
*
* @param   
*  pI2c is i2c device handle. 
*  mode is transfer mode to set
*
* @return  
*  0           -- if successful
*  -FMSH_EPERM   -- if the I2C is enabled
*
* @note    
* Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_tar/special
*   - ic_tar/gc_or_start
*
*  The I2C must be disabled in order to set the pI2c transmit mode.
* SEE ALSO
*  i2cCtrl_getTxMode(), FIicPs_TxMode_T
* SOURCE
*
****************************************************************************/
int i2cCtrl_Set_TxMode(vxT_I2C* pI2c, int tx_mode)
{
    int retval;
    UINT32 reg;

    if (i2cCtrl_isEnable(pI2c) == 0)
	{
        /*reg = I2C_INP(portmap->tar);*/
		reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_TAR);
		
        /* avoid bus write if possible*/
        /*if (FMSH_BIT_GET(reg, I2C_TAR_TX_MODE) != mode) */
        /*
		bit11:	SPECIAL R/W 决定是否执行器件ID，广播地址，开始字节
				0：忽略bit10，正常使用IC_TAR
				1：执行bit13或bit10中的命令, 		复位值：0x0
		bit10:	GC_OR_START R/W 如果bit11设置为1，bit13设置为0，该位决定主机是否执行广播地址或开始字节
				0：广播地址，其后面只能执行写操作，读操作会导致TX_ABRT中断，I2C保持广播地址模式。直到bit11被清0.
				1：开始字节传输,  		复位值：0x0
		*/
        if (((reg & 0x0C00) >> 10) != tx_mode) 
		{
            /*FMSH_BIT_SET(reg, I2C_TAR_TX_MODE, mode);*/
            /*I2C_OUTP(reg, portmap->tar);*/
        	reg &= ~0x0C00;
			reg |= (tx_mode << 10);
			i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_TAR, reg);            
        }
		
		retval = 0;
    }
    else
    {    
    	retval = -FMSH_EPERM;
    }

    return retval;
}

/*****************************************************************************
*
* @description
*  This function returns the current transmit mode in use by an I2C
*  pI2c.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  The current pI2c transmit mode.
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_tar/special
*   - ic_tar/gc_or_start
* SEE ALSO
*  i2cCtrl_setTxMode(), FIicPs_TxMode_T
* SOURCE
*
****************************************************************************/
int i2cCtrl_Get_TxMode(vxT_I2C* pI2c)
{
    UINT32 reg;
    int retval;

    /*reg = I2C_INP(portmap->tar);*/
    /*retval = (FIicPs_TxMode_T) FMSH_BIT_GET(reg, I2C_TAR_TX_MODE);*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_TAR);
	retval = (reg & 0xC00) >> 10;

    return retval;
}

/*****************************************************************************
*
* @description
*  This function sets the pI2c code, used during high-speed mode
*  transfers.
*
* @param   
*  pI2c is i2c device handle. 
*  code is pI2c code to set
*
* @return  
*  0           -- if successful
*  -FMSH_EPERM   -- if the I2C is enabled
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_hs_maddr/ic_hs_mar
*
*  The I2C must be disabled in order to set the high-speed mode pI2c
*  code.
* SEE ALSO
*  i2cCtrl_getMasterCode()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Set_MasterCode(vxT_I2C* pI2c, UINT8 code)
{
    int retval;
    UINT32 reg;

    if (i2cCtrl_isEnable(pI2c) == 0) 
	{
        reg = 0;
        /*FMSH_BIT_SET(reg, I2C_HS_MADDR_HS_MAR, code);*/
        /*I2C_OUTP(reg, portmap->hs_maddr);*/
        /*
		bit2~0: IC_HS_MAR R/W This bit field holds the value 
		                      of the I2C HS mode pI2c code
		*/
        reg = (code & 0x07);
		i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_HS_MADDR, reg); 

		retval = 0;
    }
    else
    {    
    	retval = -FMSH_EPERM;
    }

    return retval;
}

/*****************************************************************************
*
* @description
*  Initializes an I2C peripheral.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  The current high-speed mode pI2c code.
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_hs_maddr/ic_hs_mar
* SEE ALSO
*  i2cCtrl_setMasterCode()
* SOURCE
*
****************************************************************************/
UINT8 i2cCtrl_Get_MasterCode(vxT_I2C* pI2c)
{
    UINT32 reg;
    UINT8 retval;

    /*reg = I2C_INP(portmap->hs_maddr);*/
    reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_HS_MADDR);
	
    /*retval = (UINT8) FMSH_BIT_GET(reg, I2C_HS_MADDR_HS_MAR);*/
	retval = (UINT8)(reg & 0x07);

    return retval;
}

/*****************************************************************************
*
* @description
*  This function set the scl count value for a particular speed mode
*  (standard, fast, high) and clock phase (low, high).
*
* @param   
*  pI2c is i2c device handle. 
*  mode is speed mode of count value to set
*  phase is scl phase of count value to set
*  value is count value to set
*
* @return  
*  0           -- if successful
*  -FMSH_EPERM   -- if the I2C is enabled
*  -FMSH_ENOSYS  -- if the scl count registers are hardcoded
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_ss_scl_lcnt/ic_ss_scl_lcnt
*   - ic_ss_scl_hcnt/ic_ss_scl_hcnt
*   - ic_fs_scl_lcnt/ic_fs_scl_lcnt
*   - ic_fs_scl_hcnt/ic_fs_scl_hcnt
*   - ic_hs_scl_lcnt/ic_hs_scl_lcnt
*   - ic_hs_scl_hcnt/ic_hs_scl_hcnt
*
*  The I2C must be disabled in order to set any of the scl count
*  values.  The minimum programmable value for any of these registers
*  is 6.
*  This function is affected by the HC_COUNT_VALUES hardware parameter.
* SEE ALSO
*  i2cCtrl_getSclCount(), FIicPs_SpeedMode_T, FIicPs_SclPhase_T
* SOURCE
*
****************************************************************************/
int i2cCtrl_Set_SclCount(vxT_I2C* pI2c, int speed_mode, int scl_phase, UINT16 count)
{
    int retval;
    UINT32 reg;

	if (i2cCtrl_isEnable(pI2c) == 0) 
	{
        reg = 0;
		
        /*FMSH_BIT_SET(reg, I2C_SCL_COUNT, count);*/
        /*
		bit15~0: IC_HS_SCL_LCNT  High Speed I2C Clock SCL Low Count Register
		*/
        reg = count & 0xFFFF;
		
        switch (speed_mode)
		{
		case I2_SPEED_HIGH: /* (mode == I2c_speed_high) */
			{
	            if (scl_phase == I2C_SCL_LOW) /* I2c_scl_low*/
	            {    
	            	/*I2C_OUTP(reg, portmap->hs_scl_lcnt);*/
	            	i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_HS_SCL_LCNT, reg);
	            }
	            else
	            {    
	            	/*I2C_OUTP(reg, portmap->hs_scl_hcnt);*/
					i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_HS_SCL_HCNT, reg);
	            }
	        }
			break;
		
        case I2_SPEED_FAST: /* else if(mode == I2c_speed_fast) */
			{
	            if (scl_phase == I2C_SCL_LOW) /* I2c_scl_low*/
	            {    
	            	/*I2C_OUTP(reg, portmap->hs_scl_lcnt);*/
	            	i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_HS_SCL_LCNT, reg);
	            }
	            else
	            {    
	            	/*I2C_OUTP(reg, portmap->hs_scl_hcnt);*/
					i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_HS_SCL_HCNT, reg);
	            }
	        }
			break;
		
        case I2_SPEED_STANDARD: /* else if(mode == I2c_speed_standard) */
			{
	            if (scl_phase == I2C_SCL_LOW) /* I2c_scl_low*/
	            {    
	            	/*I2C_OUTP(reg, portmap->hs_scl_lcnt);*/
	            	i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_HS_SCL_LCNT, reg);
	            }
	            else
	            {    
	            	/*I2C_OUTP(reg, portmap->hs_scl_hcnt);*/
					i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_HS_SCL_HCNT, reg);
	            }
	        }
			break;
        }

		retval = 0;
    }
    else
    {    
    	retval = -FMSH_EPERM;
    }

    return retval;
}

/*****************************************************************************
*
* @description
*  This function returns the current scl count value for all speed
*  modes (standard, fast, high) and phases (low, high).
*
* @param   
*  pI2c is i2c device handle. 
*  mode is speed mode to get count value of
*  phase is scl phase to get count value of
* 
* @return  
*  The current specified scl count value.
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_ss_scl_lcnt/ic_ss_scl_lcnt
*   - ic_ss_scl_hcnt/ic_ss_scl_hcnt
*   - ic_fs_scl_lcnt/ic_fs_scl_lcnt
*   - ic_fs_scl_hcnt/ic_fs_scl_hcnt
*   - ic_hs_scl_lcnt/ic_hs_scl_lcnt
*   - ic_hs_scl_hcnt/ic_hs_scl_hcnt
*
*  This function returns 0x0000 for any non-existent scl count
*  registers.
* SEE ALSO
*  i2cCtrl_setSclCount(), FIicPs_SpeedMode_T, FIicPs_SclPhase_T
* SOURCE
*
****************************************************************************/
UINT16 i2cCtrl_Get_SclCount(vxT_I2C* pI2c, int speed_mode, int scl_phase)
{
    UINT32 reg;
    UINT16 retval;

    /*
    if (mode == I2c_speed_high) {
        if(phase == I2c_scl_low)
            reg = I2C_INP(portmap->hs_scl_lcnt);
        else
            reg = I2C_INP(portmap->hs_scl_hcnt);
    }
    else if(mode == I2c_speed_fast) {
        if(phase == I2c_scl_low)
            reg = I2C_INP(portmap->fs_scl_lcnt);
        else
            reg = I2C_INP(portmap->fs_scl_hcnt);
    }
    else if(mode == I2c_speed_standard) {
        if(phase == I2c_scl_low)
            reg = I2C_INP(portmap->ss_scl_lcnt);
        else
            reg = I2C_INP(portmap->ss_scl_hcnt);
    }
	*/	
	switch (speed_mode)
	{
	case I2_SPEED_HIGH: /* (mode == I2c_speed_high) */
		{
			if (scl_phase == I2C_SCL_LOW) /* I2c_scl_low*/
			{	 
				reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_HS_SCL_LCNT);
			}
			else
			{	 
				reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_HS_SCL_HCNT);
			}
		}
		break;
	
	case I2_SPEED_FAST: /* else if(mode == I2c_speed_fast) */
		{
			if (scl_phase == I2C_SCL_LOW) /* I2c_scl_low*/
			{	 
				reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_HS_SCL_LCNT);
			}
			else
			{	 
				reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_HS_SCL_HCNT);
			}
		}
		break;
	
	case I2_SPEED_STANDARD: /* else if(mode == I2c_speed_standard) */
		{
			if (scl_phase == I2C_SCL_LOW) /* I2c_scl_low*/
			{	 
				reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_HS_SCL_LCNT);
			}
			else
			{	 
				reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_HS_SCL_HCNT);
			}
		}
		break;
	}

    /*retval = (UINT16) FMSH_BIT_GET(reg, I2C_SCL_COUNT);*/
    retval = reg & 0xFFFF;

    return retval;
}



/*****************************************************************************
*
* @description
*  This function reads a single byte from the I2C receive FIFO.
*
* @param   
*  pI2c is i2c device handle. 
* 
* @return  
*  The character read from the I2C FIFO
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_data_cmd/dat
*
*  This function also does not check whether there is valid data or not
*  in the FIFO beforehand.  As such, it can cause an receive underflow
*  error if used improperly.
* SEE ALSO
*  i2cCtrl_Write_Fifo(), i2cCtrl_issueRead(), i2cCtrl_masterReceive(),
*  i2cCtrl_slaveReceive()
* SOURCE
*
****************************************************************************/
UINT8 i2cCtrl_Read_Fifo(vxT_I2C* pI2c)
{
    UINT8 retval;
    UINT32 reg;

    /* read a byte from the DATA_CMD register*/
    /*reg = I2C_IN8P(portmap->data_cmd);*/
    reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_DATA_CMD);
	
    retval = (UINT8)(reg & 0xFF);

    return retval;
}

/*****************************************************************************
*
* @description
*  This function writes a single byte to the I2C transmit FIFO.
*
* @param   
*  pI2c is i2c device handle. 
*  datum  is byte to write to FIFO
*
* @return  
*  none
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_data_cmd/dat
*
*  This function does not check whether the I2C transmit FIFO is full
*  or not beforehand.  As such, it can cause a transmit overflow error
*  if used improperly.
* SEE ALSO
*  i2cCtrl_read(), i2cCtrl_issueRead(), i2cCtrl_masterTransmit(),
*  i2cCtrl_slaveTransmit()
* SOURCE
*
****************************************************************************/
void i2cCtrl_Write_Fifo(vxT_I2C* pI2c, UINT8 char_data)
{
    /* write a byte to the DATA_CMD register*/
    /*I2C_OUTP(character, portmap->data_cmd);	*/
    i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_DATA_CMD, char_data);
	return;
}
                                                     
/*****************************************************************************
*
* @description
*  This function writes a read command to the I2C transmit FIFO.  This
*  is used during pI2c-receiver/slave-transmitter transfers and is
*  typically followed by a read from the pI2c receive FIFO after the
*  slave responds with data.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  none
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_data_cmd/cmd
* 
*  This function does not check whether the I2C FIFO is full or not
*  before writing to it.  As such, it can result in a transmit FIFO
*  overflow if used improperly.
* SEE ALSO
*  i2cCtrl_read(), i2cCtrl_Write_Fifo(), i2cCtrl_masterReceive()
* SOURCE
*
****************************************************************************/
void i2cCtrl_Xfer_READ(vxT_I2C* pI2c)
{
    /*I2C_OUTP(0x100, portmap->data_cmd);     */
    i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_DATA_CMD, 0x100);
	return;
}

/*****************************************************************************
*
* @description
*  This function returns the current value of the I2C transmit abort
*  status register.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  The current transmit abort status.
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_tx_abrt_source/all bits
* 
*  The transmit abort status register is cleared by the I2C upon
*  reading.  Note that it is possible for more than one bit of this
*  register to be active simultaneously and this should be dealt with
*  properly by any function operating on this return value.
* SEE ALSO
*  FIicPs_TxAbort_T
* SOURCE
*
****************************************************************************/
int i2cCtrl_Get_TxAbortSrc(vxT_I2C* pI2c)
{
    UINT32 reg;
    int retval;  /* FIicPs_TxAbort_T*/

    /* reg = I2C_INP(portmap->tx_abrt_source);*/
    /* retval = (FIicPs_TxAbort_T) FMSH_BIT_GET(reg, I2C_TX_ABRT_SRC_ALL);*/
    reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_TX_ABRT_SOURCE);
	retval = reg & 0xFFFF;

    return retval;
}

/*****************************************************************************
*
* @description
*  Returns how many bytes deep the  I2C transmit FIFO is.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  FIFO depth in bytes (from 2 to 256)
*
* @note    
*  This function is affected by the TX_BUFFER_DEPTH hardware parameter.
* SEE ALSO
*  i2cCtrl_getRxFifoDepth(), i2cCtrl_getTxFifoLevel()
* SOURCE
*
****************************************************************************/
UINT16 i2cCtrl_Get_TxFifoDepth(vxT_I2C* pI2c)
{
    UINT32 reg;
    UINT16 retval;

    /*param = (FIicPs_Param_T* pI2c->comp_param;*/
    /*retval = param->tx_buffer_depth;*/
    /*
    0xf4: component parameters 1
			bit8~15: rx_buf_depth
			bit16~23: tx_buf_depth
	*/
    reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_COMP_PARAM_1);
    retval = ((reg & 0x00FF0000) >> 16) + 1;

    return retval;
}

/*****************************************************************************
*
* @description
*  Returns how many bytes deep the I2C transmit FIFO is.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  FIFO depth in bytes (from 2 to 256)
*
* @note    
*  This function is affected by the RX_BUFFER_DEPTH hardware parameter.
* SEE ALSO
*  i2cCtrl_getTxFifoDepth(), i2cCtrl_getRxFifoLevel()
* SOURCE
*
****************************************************************************/
UINT16 i2cCtrl_Get_RxFifoDepth(vxT_I2C* pI2c)
{
    UINT32 reg;
    UINT16 retval;

    /*retval = param->rx_buffer_depth;*/
    /*
    0xf4: component parameters 1
			bit23~16: tx_buf_depth
			bit15~8:  rx_buf_depth
	*/
    reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_COMP_PARAM_1);
    retval = ((reg & 0x0000FF00) >> 8) + 1;

    return retval;
}

/*****************************************************************************
*
* @description
*  Returns whether the transmitter FIFO is full or not.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  true        -- the transmit FIFO is full
*  false       -- the transmit FIFO is not full
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_status/tfnf
* SEE ALSO
*  i2cCtrl_isTxFifoEmpty()
* SOURCE
*
****************************************************************************/
int i2cCtrl_isFull_TxFifo(vxT_I2C* pI2c)
{
    UINT32 reg;
    int retval;

    /*reg = I2C_INP(portmap->status);    */
    reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_STATUS);
	
    /*if (FMSH_BIT_GET(reg, I2C_STATUS_TFNF) == 0)*/
    /*
	bit1:	TFNF	R	发送FIFO非满标志，
	                    当FIFO存在数据时，该位置1,。当FIFO满时，该位清0,	复位值：0x1
	*/
	if ((reg & I2C_STATUS_TFNF) == 0)
    {    
    	retval = 1; /* true*/
	}
    else
    {    
    	retval = 0; /* false*/
    }

    return retval;
}

/*****************************************************************************
*
* @description
*  Returns whether the transmitter FIFO is empty or not.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  true        -- the transmit FIFO is full
*  false       -- the transmit FIFO is not full
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_status/tfe
* SEE ALSO
*  i2cCtrl_isTxFifoFull()
* SOURCE
*
****************************************************************************/
int i2cCtrl_isEmpty_TxFifo(vxT_I2C* pI2c)
{
    int retval;
    UINT32 reg;

    /*
    reg = I2C_INP(portmap->status);
    if(FMSH_BIT_GET(reg, I2C_STATUS_TFE) == 1)
        retval = true;
    else
        retval = false;
	*/
	
    reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_STATUS);
    /*
	bit2:	TFE R	发送FIFO空标志，
	        当发送FIFO为空时，该位置1.当发送FIFO存在数据时，该位清0,	复位值：0x1	
	*/
	if ((reg & I2C_STATUS_TFE) == 1)
    {    
    	retval = 1; /* true*/
	}
    else
    {    
    	retval = 0; /* false*/
    }

    return retval;
}

/*****************************************************************************
*
* @description
*  This function returns whether the receive FIFO is full or not.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  true        -- the receive FIFO is full
*  false       -- the receive FIFO is not full
*
* @note    
*   Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_status/rff
* SEE ALSO
*  i2cCtrl_isRxFifoEmpty()
* SOURCE
*
****************************************************************************/
int i2cCtrl_isFull_RxFifo(vxT_I2C* pI2c)
{
    UINT32 reg;
    int retval;

   /*
   reg = I2C_INP(portmap->status);
    if(FMSH_BIT_GET(reg, I2C_STATUS_RFF) == 1)
        retval = true;
    else
        retval = false;
	*/
    reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_STATUS);
    /*
	bit4:	RFF R	接收FIFO已满，
	        该位置1.当接收FIFO有余量的时候，清除。	复位值：0x0	
	*/
	if ((reg & I2C_STATUS_RFF) == 1)
    {    
    	retval = 1; /* true*/
	}
    else
    {    
    	retval = 0; /* false*/
    }

    return retval;
}

/*****************************************************************************
*
* @description
*  This function returns whether the receive FIFO is empty or not.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  true        -- the receive FIFO is empty
*  false       -- the receive FIFO is not empty
*
* @note    
*   Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_status/rfne
* SEE ALSO
*  i2cCtrl_isRxFifoFull()
* SOURCE
*
****************************************************************************/
int i2cCtrl_isEmpty_RxFifo(vxT_I2C* pI2c)
{
    UINT32 reg;
    int retval;

    /*
    reg = I2C_INP(portmap->status);
    if(FMSH_BIT_GET(reg, I2C_STATUS_RFNE) == 0)
        retval = true;
    else
        retval = false;
	*/
	
    reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_STATUS);
    /*
	bit3:	RFNE	R	接收FIFO非空标志，
	        当接收FIFO存在数据时，该位置1.接收FIFO为空时，该位清0,	复位值：0x0
	*/
	if ((reg & I2C_STATUS_RFNE) == 0)
    {    
    	retval = 1; /* true*/
	}
    else
    {    
    	retval = 0; /* false*/
    }

    return retval;
}

/*****************************************************************************
*
* @description
*  This function returns the number of valid data entries currently
*  present in the transmit FIFO.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  Number of valid data entries in the transmit FIFO.
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_txflr/txflr
* SEE ALSO
*  i2cCtrl_getRxFifoLevel(), i2cCtrl_isTxFifoFull(),
*  i2cCtrl_isTxFifoEmpty(), i2cCtrl_setTxThreshold(),
*  i2cCtrl_getTxThreshold()
* SOURCE
*
****************************************************************************/
UINT16 i2cCtrl_Get_TxFifoLevel(vxT_I2C* pI2c)
{
    UINT32 reg;
    UINT16 retval;

    /*reg = I2C_INP(portmap->txflr);*/
    /*retval = (UINT16) FMSH_BIT_GET(reg, I2C_TXFLR_TXFL);*/
    reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_TXFLR);
	retval = reg & 0x03FF;

    return retval;
}

/*****************************************************************************
*
* @description
*   This function returns the number of valid data entries currently
*  present in the receiver FIFO.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  Number of valid data entries in the receive FIFO.
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_rxflr/rxflr
* SEE ALSO
*  i2cCtrl_getTxFifoLevel(), i2cCtrl_isRxFifoFull(),
*  i2cCtrl_isRxFifoEmpty(), i2cCtrl_setRxThreshold(),
*  i2cCtrl_getRxThreshold()
* SOURCE
*
****************************************************************************/
UINT16 i2cCtrl_Get_RxFifoLevel(vxT_I2C* pI2c)
{
    UINT32 reg;
    UINT16 retval;
	
   /* reg = I2C_INP(portmap->rxflr);*/
   /* retval = (UINT16) FMSH_BIT_GET(reg, I2C_RXFLR_RXFL);*/
    reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_RXFLR);
	retval = reg & 0x03FF;

    return retval;
}

/*****************************************************************************
*
* @description
*  This function sets the threshold level for the transmit FIFO.  When
*  the number of data entries in the transmit FIFO is at or below this
*  level, the tx_empty interrupt is triggered.  If an interrupt-driven
*  transfer is already in progress, the transmit threshold level is not
*  updated until the end of the transfer.
*
* @param   
*  pI2c is i2c device handle. 
*  level is level at which to set threshold
*
* @return  
*  0           -- if successful
*  -FMSH_EINVAL  -- if the level specified is greater than the transmit
*                 FIFO depth; the threshold is set to the transmit FIFO
*                 depth.
*  -FMSH_EBUSY   -- if an interrupt-driven transfer is currently in
*                 progress; the requested level will be written to the
*                 transmit threshold register when the current transfer
*                 completes.
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_tx_tl/tx_tl
*
*  The driver keeps a copy of the last transmit threshold specified by
*  the user as it manipulates the transmit threshold level at the end
*  of interrupt-driven transmit transfers.  This copy is used to
*  restore the transmit threshold upon completion of a transmit
*  transfer.
* SEE ALSO
*  i2cCtrl_getTxThreshold(), i2cCtrl_setRxThreshold(),
*  i2cCtrl_getRxThreshold(), i2cCtrl_getTxFifoLevel()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Set_TxThreshold(vxT_I2C* pI2c, UINT8 level)
{
    int retval;
    UINT32 reg;
	

    /* We need to be careful here not to overwrite the tx threshold if*/
    /* the interrupt handler has altered it to trigger on the last byte*/
    /* of the current transfer (in order to call the user callback*/
    /* function at the appropriate time).  When the driver returns to*/
    /* the idle state, it will update the tx threshold with the*/
    /* user-specified value.*/
    
   /* if(level > param->tx_buffer_depth)*/
	if(level > i2cCtrl_Get_TxFifoDepth(pI2c))
    {    
    	retval = -FMSH_EINVAL;
	}
    else 
	{
        /* store user tx threshold value*/
        /*instance->txThreshold = level;*/
        pI2c->pI2cCtrl->txThreshold = level;

        /*if (instance->state == I2c_state_idle) */
        if (pI2c->pI2cCtrl->status == 0x00) /* I2c_state_idle = 0,*/
		{
            reg = 0;
            /*FMSH_BIT_SET(reg, I2C_TX_TL_TX_TL, level);*/
            /*I2C_OUTP(reg, portmap->tx_tl)*/
            /*
			bit7~0: TX_TL	R/W TX FIFO阈值,  	复位值：0x0
			*/
            reg |= level & 0x00FF;
			i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_TX_TL, reg);

			retval = 0;;
        }
        else
        {    
        	retval = -FMSH_EBUSY;
        }
    }

    return retval;
}

/*****************************************************************************
*
* @description
*  This function returns the current threshold level for the transmit
*  FIFO.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  The transmit FIFO threshold level.
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_tx_tl/tx_tl
* SEE ALSO
*  i2cCtrl_setTxThreshold(), i2cCtrl_setRxThreshold(),
*  i2cCtrl_getRxThreshold(), i2cCtrl_getTxFifoLevel()
* SOURCE
*
****************************************************************************/
UINT8 i2cCtrl_Get_TxThreshold(vxT_I2C* pI2c)
{
    UINT32 reg;
    UINT8 retval;

    /*reg = I2C_INP(portmap->tx_tl);*/
    /* retval = FMSH_BIT_GET(reg, I2C_TX_TL_TX_TL);*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_TX_TL);
	retval = reg & 0xFF;

    return retval;
}

/*****************************************************************************
*
* @description
*  This function sets the threshold level for the receive FIFO.  When
*  the number of data entries in the receive FIFO is at or above this
*  level, the rx_full interrupt is triggered.  If an interrupt-driven
*  transfer is already in progress, the receive threshold level is not
*  updated until the end of the transfer.
*
* @param   
*  pI2c is i2c device handle. 
*  level is level at which to set threshold
*
* @return  
*  0           -- if successful
*  -FMSH_EINVAL  -- if the level specified is greater than the receive
*                 FIFO depth, the threshold is set to the receive FIFO
*                 depth.
*  -FMSH_EBUSY   -- if an interrupt-driven transfer is currently in
*                 progress, the requested level is written to the
*                 receive threshold register when the current transfer
*                 completes.
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_rx_tl/rx_tl
*
*  The driver keeps a copy of the last receive threshold specified by
*  the user as it manipulates the receive threshold level at the end of
*  interrupt-driven receive transfers.  This copy is used to restore
*  the receive threshold upon completion of an receive transfer.
* WARNINGS
*  When this function is called, if the following slave receive
*  transfer is less than the threshold set, the rx_full interrupt is
*  not immediately triggered.
* SEE ALSO
*  i2cCtrl_getRxThreshold(), i2cCtrl_setTxThreshold(),
*  i2cCtrl_getTxThreshold(), i2cCtrl_terminate(), i2cCtrl_getRxFifoLevel()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Set_RxThreshold(vxT_I2C* pI2c, UINT8 level)
{
    int retval;
    UINT32 reg;

    /* We need to be careful here not to overwrite the rx threshold if*/
    /* the interrupt handler has altered it to trigger on the last byte*/
    /* of the current transfer (in order to call the user callback*/
    /* function at the appropriate time).  When the driver returns to*/
    /* the idle state, it will update the rx threshold with the*/
    /* user-specified value.*/
    
    /*if (level > param->rx_buffer_depth)*/
    if (level > i2cCtrl_Get_RxFifoDepth(pI2c))
    {    
    	retval = -FMSH_EINVAL;
    }
    else 
	{
        /* store user rx threshold value*/
        /*instance->rxThreshold = level;*/
        pI2c->pI2cCtrl->rxThreshold = level;

        /*if (instance->state == I2c_state_idle) */
        if (pI2c->pI2cCtrl->status == 0x00) /* I2c_state_idle = 0,*/
		{
            reg = 0;
            /*FMSH_BIT_SET(reg, I2C_RX_TL_RX_TL, level);*/
            /*I2C_OUTP(reg, portmap->rx_tl);*/
            reg |= level & 0x00FF;
			i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_RX_TL, reg);

			retval = 0;
        }
        else
        {    
        	retval = -FMSH_EBUSY;
        }
    }

    return retval;
}

/*****************************************************************************
*
* @description
*  This function returns the current threshold level for the receive
*  FIFO.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  The receive FIFO threshold level.
*
* @note    
*   Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_rx_tl/rx_tl
* 
*  It is possible that the returned value is not equal to what the user
*  has previously set the receive threshold to be.  This is because the
*  driver manipulates the threshold value in order to complete receive
*  transfers.  The previous user-specified threshold is restored upon
*  completion of each transfer.
* SEE ALSO
*  i2cCtrl_setRxThreshold(), i2cCtrl_setTxThreshold(),
*  i2cCtrl_getTxThreshold(), i2cCtrl_getRxFifoLevel()
* SOURCE
*
****************************************************************************/
UINT8 i2cCtrl_Get_RxThreshold(vxT_I2C* pI2c)
{
    UINT32 reg;
    UINT8 retval;

    /*reg = I2C_INP(portmap->rx_tl);*/
    /*retval = FMSH_BIT_GET(reg, I2C_RX_TL_RX_TL);*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_RX_TL);
	retval = reg & 0xFF;

    return retval;
}

#if 0
/*****************************************************************************
*
* @description
*  This function initiates an interrupt-driven pI2c back-to-back
*  transfer.  To do this, the I2C must first be properly configured,
*  enabled and a transmit buffer must be setup which contains the
*  sequential reads and writes to perform.  An associated receive
*  buffer of suitable size must also be specified when issuing the
*  transfer.  As data is received, it is written to the receive buffer.
*  The callback function is called (if it is not NULL) when the final
*  byte is received and there is no more data to send.
*
*  A transfer may be stopped at any time by calling i2cCtrl_terminate(),
*  which returns the number of bytes that are sent before the transfer
*  is interrupted.  A terminated transfer's callback function is never
*  called.
*
* @param   
*  pI2c is i2c device handle. 
*  txBuffer is buffer from which to send data
*  txLength is length of transmit buffer/number of bytes to send
*  rxBuffer is buffer to write received data to
*  rxLength is length of receive buffer/number of bytes to receive
*  callback is function to call when transfer is complete
*
* @return  
*  0           -- if successful
*  -FMSH_EBUSY   -- if the I2C is busy (transfer already in progress)
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_intr_mask/all bits
*   - ic_rx_tl/rx_tl
*
*  This function enables the tx_empty and tx_abrt interrupts.
*  The FMSH_callback function pointer typedef is defined in the common
*  header files.
*  The transmit buffer must be 16 bits wide as a read command is 9 bits
*  long (0x100).  Restart conditions must be enabled in order to
*  perform back-to-back transfers.
*
*  This function is part of the Interrupt API and should not be called
*  when using the I2C in a poll-driven manner.
*
*  This function cannot be used when using an interrupt handler other
*  than i2cCtrl_irqHandler().
* SEE ALSO
*  i2cCtrl_masterTransmit(), i2cCtrl_masterReceive(),
*  i2cCtrl_slaveTransmit(), i2cCtrl_slaveBulkTransmit(),
*  i2cCtrl_slaveReceive(), i2cCtrl_terminate()
* SOURCE
*
****************************************************************************/
int i2cCtrl_masterBack2Back(vxT_I2C* pI2c, UINT16*txBuffer,
        unsigned txLength, UINT8*rxBuffer, unsigned rxLength,
        FMSH_callback userFunction)
{
    int retval;
    UINT8*tmp;
    UINT32 reg;
    FIicPs_PortMap_T*portmap;
    FIicPs_Instance_T*instance;

    I2C_COMMON_REQUIREMENTS(pI2c);
    FMSH_REQUIRE(txBuffer != NULL);
    FMSH_REQUIRE(txLength != 0);
    FMSH_REQUIRE(rxBuffer != NULL);
    FMSH_REQUIRE(rxLength != 0);
    FMSH_REQUIRE(rxLength <= txLength);

    instance = (FIicPs_Instance_T*) pI2c->instance;
    portmap = (FIicPs_PortMap_T*) pI2c->base_address;

    FMSH_REQUIRE(instance->listener != NULL);

    /* Critical section of code.  Shared data needs to be protected.*/
    /* This macro disables FMSH_apb_i2c interrupts.*/
    I2C_ENTER_CRITICAL_SECTION();

    if(instance->state == I2c_state_idle) {
        retval = 0;
        /* pI2c back2back transfer mode*/
        instance->state = I2c_state_back2back;
        instance->rxBuffer = rxBuffer;
        instance->rxLength = rxLength;
        instance->rxRemain = rxLength;
        instance->rxCallback = userFunction;
        instance->rxHold = 0;
        instance->rxIdx = 4;
        instance->b2bBuffer = txBuffer;
        instance->txLength = txLength;
        instance->txRemain = txLength;
        instance->txCallback = userFunction;
        instance->txHold = 0;
        instance->txIdx = 0;

        /* check if rx buffer is word-aligned*/
        if(((unsigned) rxBuffer & 0x3) == 0)
            instance->rxAlign = true;
        else
            instance->rxAlign = false;

        /* support non-word aligned 16-bit buffers*/
        tmp = (UINT8*) txBuffer;
        while(((((unsigned) tmp) & 0x3) != 0x0) && ((instance->txLength
                        - instance->txIdx) > 0)) {
            instance->txHold |= ((*tmp++ & 0xff) << (instance->txIdx*
                        8));
            instance->txIdx++;
        }
        instance->txBuffer = tmp;

        /* set rx fifo threshold if necessary*/
        if(rxLength <= instance->rxThreshold) {
            reg = 0;
            FMSH_BIT_SET(reg, I2C_RX_TL_RX_TL, rxLength - 1);
            I2C_OUTP(reg, portmap->rx_tl);
        }

        /* ensure transfer is underway*/
        instance->intr_mask_save |= (I2c_irq_tx_empty | I2c_irq_tx_abrt
                | I2c_irq_rx_full);
    }
    else
        retval = -FMSH_EBUSY;

    /* End of critical section of code. This macros restores FMSH_apb_i2c*/
    /* interrupts.*/
    I2C_EXIT_CRITICAL_SECTION();

    return retval;
}

/*****************************************************************************
*
* @description
*  This function initiates an interrupt-driven pI2c transmit
*  transfer.  To do this, the I2C must first be properly configured and
*  enabled.  This function configures a pI2c transmit transfer and
*  enables the transmit interrupt to keep the transmit FIFO filled.
*  Upon completion, the callback function is called (if it is not
*  NULL).
*
*  A transfer may be stopped at any time by calling i2cCtrl_terminate(),
*  which returns the number of bytes that are sent before the transfer
*  is interrupted.  A terminated transfer's callback function is never
*  called.
*
* @param   
*  pI2c is i2c device handle. 
*  buffer is buffer from which to send data
*  length is length of transmit buffer/number of bytes to send
*  callback is function to call when transfer is complete
*
* @return  
*  0           -- if successful
*  -FMSH_EBUSY   -- if the I2C is busy (transfer already in progress)
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_intr_mask/all bits
* 
*  This function enables the tx_empty and tx_abrt interrupts.
*  The FMSH_callback function pointer typedef is defined in the common
*  header files.
*
*  This function is part of the Interrupt API and should not be called
*  when using the I2C in a poll-driven manner.
*
*  This function cannot be used when using an interrupt handler other
*  than i2cCtrl_irqHandler().
* SEE ALSO
*  i2cCtrl_masterBack2Back(), i2cCtrl_masterReceive(),
*  i2cCtrl_slaveTransmit(), i2cCtrl_slaveBulkTransmit(),
*  i2cCtrl_slaveReceive(), i2cCtrl_terminate()
* SOURCE
*
****************************************************************************/
int i2cCtrl_masterTransmit(vxT_I2C* pI2c, UINT8*buffer,
        unsigned length, FMSH_callback userFunction)
{
    int retval;
    UINT8*tmp;
    FIicPs_PortMap_T*portmap;
    FIicPs_Instance_T*instance;

    I2C_COMMON_REQUIREMENTS(pI2c);
    FMSH_REQUIRE(buffer != NULL);
    FMSH_REQUIRE(length != 0);

    instance = (FIicPs_Instance_T*) pI2c->instance;
    portmap = (FIicPs_PortMap_T*) pI2c->base_address;

    FMSH_REQUIRE(instance->listener != NULL);

    /* Critical section of code.  Shared data needs to be protected.*/
    /* This macro disables FMSH_apb_i2c interrupts.*/
    I2C_ENTER_CRITICAL_SECTION();

    if (instance->state == I2c_state_idle) 
	{
        /* pI2c-transmitter*/
        retval = 0;

        instance->state = I2c_state_master_tx;
        instance->txCallback = userFunction;
        instance->txLength = length;
        instance->txRemain = length;
        instance->txHold = 0;
        instance->txIdx = 0;
		
        /* check for non word-aligned buffer as I2C_FIFO_WRITE() works*/
        /* efficiently on words reads from instance->txHold.*/
        tmp = buffer;
		
        while (((((unsigned) tmp) & 0x3) != 0x0) && ((instance->txLength
                        - instance->txIdx) > 0)) 
        {
            instance->txHold |= ((*tmp++ & 0xff) << (instance->txIdx*
                        8));
            instance->txIdx++;
        }
						
        instance->txBuffer = tmp;

        /* ensure transfer is underway*/
        instance->intr_mask_save |= (I2c_irq_tx_empty | I2c_irq_tx_abrt);
    }
    else
    {    
    	retval = -FMSH_EBUSY;
    }

    /* End of critical section of code. This macros restores FMSH_apb_i2c*/
    /* interrupts.*/
    I2C_EXIT_CRITICAL_SECTION();

    return retval;
}

/*****************************************************************************
*
* @description
*  This function initiates an interrupt-driven slave transmit transfer.
*  To do this, the I2C must first be properly configured, enabled and
*  must also receive a read request (I2c_irq_rd_req) from an I2C
*  pI2c.  This function fills the transmit FIFO and, if there is more
*  data to send, sets up and enables the transmit interrupts to keep
*  the FIFO filled.  Upon completion, the callback function is called
*  (if it is not NULL).
*
*  A transfer may be stopped at any time by calling i2cCtrl_terminate(),
*  which returns the number of bytes that were sent before the transfer
*  was interrupted.  A terminated transfer's callback function is never
*  called.
*
* @param   
*  pI2c is i2c device handle. 
*  buffer is buffer from which to send data
*  length is length of transmit buffer/number of bytes to send
*  callback is function to call when transfer is complete
*
* @return  
*  0           -- if successful
*  -FMSH_EBUSY   -- if the I2C is busy (transfer already in progress)
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_intr_mask/all bits
* 
*  This function enables the tx_empty and tx_abrt interrupts.
*  This function may only be called from the user listener function
*  after a read request has been received.
*  The FMSH_callback function pointer typedef is defined in the common
*  header files.
*
*  This function is part of the Interrupt API and should not be called
*  when using the I2C in a poll-driven manner.
*
*  This function cannot be used when using an interrupt handler other
*  than i2cCtrl_irqHandler().
* SEE ALSO
*  i2cCtrl_masterBack2Back(), i2cCtrl_masterTransmit(),
*  i2cCtrl_slaveBulkTransmit(), i2cCtrl_masterReceive(),
*  i2cCtrl_slaveReceive(), i2cCtrl_terminate()
* SOURCE
*
****************************************************************************/
int i2cCtrl_slaveTransmit(vxT_I2C* pI2c, UINT8*buffer,
        unsigned length, FMSH_callback userFunction)
{
    UINT8*tmp;
    int retval;
    FIicPs_PortMap_T*portmap;
    FIicPs_Instance_T*instance;

    I2C_COMMON_REQUIREMENTS(pI2c);
    FMSH_REQUIRE(buffer != NULL);
    

    FMSH_REQUIRE(length != 0);
    
    portmap = (FIicPs_PortMap_T*) pI2c->base_address;
    instance = (FIicPs_Instance_T*) pI2c->instance;

    FMSH_REQUIRE(instance->listener != NULL);

    /* Critical section of code.  Shared data needs to be protected.*/
    /* This macro disables FMSH_apb_i2c interrupts.*/
    I2C_ENTER_CRITICAL_SECTION();

    if((instance->state == I2c_state_rd_req) || (instance->state ==
                I2c_state_slave_rx_rd_req)) {
        /* slave-transmitter*/
        retval = 0;

        instance->txCallback = userFunction;
        instance->txLength = length;
        instance->txRemain = length;
        instance->txHold = 0;
        instance->txIdx = 0;
        /* check for non word-aligned buffer as I2C_FIFO_WRITE() works*/
        /* efficiently on words reads from instance->txHold.*/
        tmp = (UINT8*) buffer;
        while(((((unsigned) tmp) & 0x3) != 0x0) && ((instance->txLength
                        - instance->txIdx) > 0)) {
            instance->txHold |= ((*tmp++ & 0xff) << (instance->txIdx*
                        8));
            instance->txIdx++;
        }
        /* buffer is now word-aligned*/
        instance->txBuffer = tmp;
        /* write only one byte of data to the slave tx fifo*/
        I2C_FIFO_WRITE(1);
     
        switch(instance->state) {
            case I2c_state_rd_req:
                instance->state = I2c_state_slave_tx;
                break;
            case I2c_state_slave_rx_rd_req:
                instance->state = I2c_state_slave_tx_rx;
                break;
            default:
                FMSH_ASSERT(false);
                break;
        }
        /* Note: tx_empty is not enabled here as rd_req is the signal*/
        /* used to write the next byte of data to the tx fifo.*/
    }
    else
        retval = -FMSH_EPROTO;

    /* End of critical section of code. This macros restores FMSH_apb_i2c*/
    /* interrupts.*/
    I2C_EXIT_CRITICAL_SECTION();

    return retval;
}

/*****************************************************************************
*
* @description
*  This function initiates an interrupt-driven slave transmit transfer.
*  To do this, the I2C must first be properly configured, enabled and
*  must also receive a read request (I2c_irq_rd_req) from an I2C
*  pI2c.  This function fills the transmit FIFO and, if there is more
*  data to send, sets up and enables the transmit interrupts to keep
*  the FIFO filled.  Upon completion, the callback function is called
*  (if it is not NULL).
*
*  A transfer may be stopped at any time by calling i2cCtrl_terminate(),
*  which returns the number of bytes that were sent before the transfer
*  was interrupted.  A terminated transfer's callback function is never
*  called.
*
* @param   
*  pI2c is i2c device handle. 
*  buffer is buffer from which to send data
*  length is length of transmit buffer/number of bytes to send
*  callback is function to call when transfer is complete
*
* @return  
*  0           -- if successful
*  -FMSH_EPROTO  -- if a read request was not received
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_intr_mask/all bits
* 
*  This function enables the tx_empty and tx_abrt interrupts.
*  This function may only be called from the user listener function
*  after a read request has been received.
*  The FMSH_callback function pointer typedef is defined in the common
*  header files.
*
*  This function is part of the Interrupt API and should not be called
*  when using the I2C in a poll-driven manner.
*
*  This function cannot be used when using an interrupt handler other
*  than i2cCtrl_irqHandler().
* SEE ALSO
*  i2cCtrl_masterBack2Back(), i2cCtrl_masterTransmit(),
*  i2cCtrl_slaveTransmit(), i2cCtrl_masterReceive(),
*  i2cCtrl_slaveReceive(), i2cCtrl_terminate()
* SOURCE
*
****************************************************************************/
int i2cCtrl_slaveBulkTransmit(vxT_I2C* pI2c, UINT8*buffer,
        unsigned length, FMSH_callback userFunction)
{
    UINT8*tmp;
    int retval, maxBytes;
    FIicPs_Param_T*param;
    FIicPs_PortMap_T*portmap;
    FIicPs_Instance_T*instance;

    I2C_COMMON_REQUIREMENTS(pI2c);
    FMSH_REQUIRE(buffer != NULL);
    FMSH_REQUIRE(length != 0);

    param = (FIicPs_Param_T*) pI2c->comp_param;
    portmap = (FIicPs_PortMap_T*) pI2c->base_address;
    instance = (FIicPs_Instance_T*) pI2c->instance;

    FMSH_REQUIRE(instance->listener != NULL);

    /* Critical section of code.  Shared data needs to be protected.*/
    /* This macro disables FMSH_apb_i2c interrupts.*/
    I2C_ENTER_CRITICAL_SECTION();

    if((instance->state == I2c_state_rd_req) || (instance->state ==
                I2c_state_slave_rx_rd_req)) {
        /* slave-transmitter*/
        retval = 0;

        instance->txCallback = userFunction;
        instance->txLength = length;
        instance->txRemain = length;
        instance->txHold = 0;
        instance->txIdx = 0;
        /* check for non word-aligned buffer as I2C_FIFO_WRITE() works*/
        /* efficiently on words reads from instance->txHold.*/
        tmp = (UINT8*) buffer;
        while(((((unsigned) tmp) & 0x3) != 0x0) && ((instance->txLength
                        - instance->txIdx) > 0)) {
            instance->txHold |= ((*tmp++ & 0xff) << (instance->txIdx*
                        8));
            instance->txIdx++;
        }
        /* buffer is now word-aligned*/
        instance->txBuffer = tmp;
        /* maximum available space in the tx fifo*/
        maxBytes = param->tx_buffer_depth - i2cCtrl_getTxFifoLevel(pI2c);
        I2C_FIFO_WRITE(maxBytes);
        switch(instance->state) {
            case I2c_state_rd_req:
                instance->state = I2c_state_slave_bulk_tx;
                break;
            case I2c_state_slave_rx_rd_req:
                instance->state = I2c_state_slave_bulk_tx_rx;
                break;
            default:
                FMSH_ASSERT(false);
                break;
        }
        /* ensure transfer is underway*/
        instance->intr_mask_save |= (I2c_irq_tx_empty |
                I2c_irq_tx_abrt);
    }
    else
        retval = -FMSH_EPROTO;

    /* End of critical section of code. This macros restores FMSH_apb_i2c*/
    /* interrupts.*/
    I2C_EXIT_CRITICAL_SECTION();

    return retval;
}

/*****************************************************************************
*
* @description
*  This function initiates an interrupt-driven pI2c receive transfer.
*  To do this, the I2C must first be properly configured and enabled.
*  This function sets up the transmit FIFO to be loaded with read
*  commands.  In parallel, this function sets up and enables the
*  receive interrupt to fill the buffer from the receive FIFO (the same
*  number of times as writes to the transmit FIFO).  Upon completion,
*  the callback function is called (if it is not NULL).
*
*  A transfer may be stopped at any time by calling i2cCtrl_terminate(),
*  which returns the number of bytes that were received before the
*  transfer was interrupted.  A terminated transfer's callback function
*  is never called.
*
* @param   
*  pI2c is i2c device handle. 
*  buffer is buffer from which to send data
*  length is length of transmit buffer/number of bytes to send
*  callback is function to call when transfer is complete
*
* @return  
*  0           -- if successful
*  -FMSH_EBUSY   -- if the I2C is busy (transfer already in progress)
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_intr_mask/all bits
*   - ic_rx_tl/rx_tl
* 
*  This function enables the tx_empty, tx_abrt & rx_full interrupts.
*  The FMSH_callback function pointer typedef is defined in the common
*  header files.
*
*  This function is part of the Interrupt API and should not be called
*  when using the I2C in a poll-driven manner.
*
*  This function cannot be used when using an interrupt handler other
*  than i2cCtrl_irqHandler().
* SEE ALSO
*  i2cCtrl_masterBack2Back(), i2cCtrl_masterTransmit(),
*  i2cCtrl_slaveTransmit(), i2cCtrl_slaveBulkTransmit(),
*  i2cCtrl_slaveReceive(), i2cCtrl_terminate()
* SOURCE
*
****************************************************************************/
int i2cCtrl_masterReceive(vxT_I2C* pI2c, UINT8* buffer,
        unsigned length, FMSH_callback userFunction)
{
    int retval;
    UINT32 reg;
    FIicPs_PortMap_T*portmap;
    FIicPs_Instance_T*instance;

    I2C_COMMON_REQUIREMENTS(pI2c);
    FMSH_REQUIRE(buffer != NULL);
    FMSH_REQUIRE(length != 0);

    instance = (FIicPs_Instance_T*) pI2c->instance;
    portmap = (FIicPs_PortMap_T*) pI2c->base_address;

    FMSH_REQUIRE(instance->listener != NULL);

    /* Critical section of code.  Shared data needs to be protected.*/
    /* This macro disables FMSH_apb_i2c interrupts.*/
    I2C_ENTER_CRITICAL_SECTION();

    if (instance->state == I2c_state_idle) 
	{
        retval = 0;
        instance->state = I2c_state_master_rx;
        /* check if rx buffer is not word-aligned*/
        if(((unsigned) buffer & 0x3) == 0)
            instance->rxAlign = true;
        else
            instance->rxAlign = false;

        /* set rx fifo threshold if necessary*/
        if (length <= instance->rxThreshold) 
		{
            reg = 0;
            FMSH_BIT_SET(reg, I2C_RX_TL_RX_TL, length - 1);
            I2C_OUTP(reg, portmap->rx_tl);
        }

        /* set transfer variables*/
        instance->rxBuffer = buffer;
        instance->rxLength = length;
        instance->rxRemain = length;
        instance->txRemain = length;
        instance->rxCallback = userFunction;
        instance->rxIdx = 4;
        /* restore interrupts and ensure pI2c-receive is underway*/
        instance->intr_mask_save |= (I2c_irq_rx_full | I2c_irq_tx_empty
                | I2c_irq_tx_abrt);
    }
    else
        retval = -FMSH_EBUSY;

    /* End of critical section of code. This macros restores FMSH_apb_i2c*/
    /* interrupts.*/
    I2C_EXIT_CRITICAL_SECTION();

    return retval;
}
                                       
/*****************************************************************************
*
* @description
*  This function initiates an interrupt-driven slave receive transfer.
*  To do this, the I2C must first be properly configured and enabled.
*  This function sets up and enables the receive interrupt to fill the
*  buffer from the receive FIFO.  Upon completion, the callback
*  function is called (if it is not NULL).
*
*  A transfer may be stopped at any time by calling i2cCtrl_terminate(),
*  which returns the number of bytes that were received before the
*  transfer was interrupted.  A terminated transfer's callback function
*  is never called.
*
* @param   
*  pI2c is i2c device handle. 
*  buffer is buffer to write received data to
*  length is length of buffer/max number of bytes to receive
*  callback is function to call when transfer is complete
*
* @return  
*  0           -- if successful
*  -FMSH_EBUSY   -- if the I2C is busy (transfer already in progress)
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_intr_mask/all bits
*   - ic_rx_tl/rx_tl
*
*  This function enables the rx_full interrupt.
*  The FMSH_callback function pointer typedef is defined in the common
*  header files.
*
*  This function is part of the Interrupt API and should not be called
*  when using the I2C in a poll-driven manner.
*
*  This function cannot be used when using an interrupt handler other
*  than i2cCtrl_irqHandler().
* SEE ALSO
*  i2cCtrl_masterBack2Back(), i2cCtrl_masterTransmit(),
*  i2cCtrl_masterReceive(), i2cCtrl_slaveTransmit(),
*  i2cCtrl_slaveBulkTransmit(), i2cCtrl_terminate()
* SOURCE
*
****************************************************************************/
int i2cCtrl_slaveReceive(vxT_I2C* pI2c, UINT8*buffer, unsigned
        length, FMSH_callback userFunction)
{
    int retval;
    UINT32 reg;
    FIicPs_PortMap_T*portmap;
    FIicPs_Instance_T*instance;

    I2C_COMMON_REQUIREMENTS(pI2c);
    FMSH_REQUIRE(buffer != NULL);
    FMSH_REQUIRE(length != 0);

    instance = (FIicPs_Instance_T*) pI2c->instance;
    portmap = (FIicPs_PortMap_T*) pI2c->base_address;

    FMSH_REQUIRE(instance->listener != NULL);

    /* Critical section of code.  Shared data needs to be protected.*/
    /* This macro disables FMSH_apb_i2c interrupts.*/
    I2C_ENTER_CRITICAL_SECTION();

    if((instance->state == I2c_state_rx_req)
            || (instance->state == I2c_state_idle)
            || (instance->state == I2c_state_slave_tx)
            || (instance->state == I2c_state_slave_bulk_tx)
            || (instance->state == I2c_state_slave_tx_rx_req)
            || (instance->state == I2c_state_slave_bulk_tx_rx_req)
            || (instance->state == I2c_state_master_tx_gen_call)) {
        retval = 0;
        /* in case the state was idle*/
        switch(instance->state) 
		{
            case I2c_state_idle:
            case I2c_state_rx_req:
                instance->state = I2c_state_slave_rx;
                break;
            case I2c_state_slave_tx:
            case I2c_state_slave_tx_rx_req:
                instance->state = I2c_state_slave_tx_rx;
                break;
            case I2c_state_slave_bulk_tx:
            case I2c_state_slave_bulk_tx_rx_req:
                instance->state = I2c_state_slave_bulk_tx_rx;
                break;
            case I2c_state_master_tx_gen_call:
                instance->state = I2c_state_master_tx_slave_rx;
                break;
            default:
                FMSH_ASSERT(false);
                break;
        }
		
        if(((unsigned) buffer & 0x3) == 0)
            instance->rxAlign = true;
        else
            instance->rxAlign = false;

        /* set rx fifo threshold if necessary*/
        if (length <= instance->rxThreshold) 
		{
            reg = 0;
            FMSH_BIT_SET(reg, I2C_RX_TL_RX_TL, length - 1);
            I2C_OUTP(reg, portmap->rx_tl);
        }

        /* set transfer variables*/
        instance->rxBuffer = buffer;
        instance->rxLength = length;
        instance->rxRemain = length;
        instance->rxCallback = userFunction;
        instance->rxIdx = 4;

        /* ensure receive is underway*/
        instance->intr_mask_save |= I2c_irq_rx_full;
    }
    else
        retval = -FMSH_EBUSY;

    /* End of critical section of code. This macros restores FMSH_apb_i2c*/
    /* interrupts.*/
    I2C_EXIT_CRITICAL_SECTION();

    return retval;
}



/*****************************************************************************
*
* @description
*  This function terminates the current I2C interrupt-driven transfer
*  in progress, if any.  This function must be called to end an
*  unfinished interrupt-driven transfer as driver instability would
*  ensue otherwise.
*  Any data received after calling this function is treated as a new
*  transfer by the driver.  Therefore, it would be prudent to wait
*  until the next detected stop condition when receiving data in order
*  to avoid a misalignment between the device and driver.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  The number of bytes sent/received during the interrupted transfer,
*  if any.
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_intr_mask/all bits
*   - ic_rxflr/rxflr
*   - ic_data_cmd/dat
*   - ic_rx_tl/rx_tl
* 
*  This function is part of the interrupt-driven interface and should
*  not be called when using the I2C in a poll-driven manner.
*  This function disables the tx_empty and enables the rx_full
*  interrupts.
*  This function restores the receive FIFO threshold to the previously
*  user-specified value.
*
*  This function is part of the Interrupt API and should not be called
*  when using the I2C in a poll-driven manner.
*
*  This function cannot be used when using an interrupt handler other
*  than i2cCtrl_irqHandler().
* SEE ALSO
*  i2cCtrl_masterBack2Back(), i2cCtrl_masterTransmit(),
*  i2cCtrl_masterReceive(), i2cCtrl_slaveTransmit(),
*  i2cCtrl_slaveBulkTransmit(), i2cCtrl_slaveReceive()
* SOURCE
*
****************************************************************************/
int i2cCtrl_terminate(vxT_I2C* pI2c)
{
    UINT32 reg;
    int retval, maxBytes;
    FIicPs_PortMap_T*portmap;
    FIicPs_Instance_T*instance;

    I2C_COMMON_REQUIREMENTS(pI2c);

    instance = (FIicPs_Instance_T*) pI2c->instance;
    portmap = (FIicPs_PortMap_T*) pI2c->base_address;

    FMSH_REQUIRE(instance->listener != NULL);

    /* Critical section of code.  Shared data needs to be protected.*/
    /* This macro disables FMSH_apb_i2c interrupts.*/
    I2C_ENTER_CRITICAL_SECTION();

    /* disable tx interrupt*/
    if((instance->state == I2c_state_master_tx)
            || (instance->state == I2c_state_back2back)
            || (instance->state == I2c_state_slave_tx)
            || (instance->state == I2c_state_slave_tx_rx)
            || (instance->state == I2c_state_slave_bulk_tx)
            || (instance->state == I2c_state_slave_bulk_tx_rx)
            || (instance->state == I2c_state_master_tx_slave_rx)) 
    {
        /* ensure tx empty is not re-enabled when interrupts are*/
        /* restored*/
        instance->intr_mask_save &= ~I2c_irq_tx_empty;
    }

    /* flush rx fifo if necessary*/
    if((instance->state == I2c_state_master_rx)
            || (instance->state == I2c_state_slave_rx)
            || (instance->state == I2c_state_slave_tx_rx)
            || (instance->state == I2c_state_slave_bulk_tx_rx)
            || (instance->state == I2c_state_master_tx_slave_rx)
            || (instance->state == I2c_state_back2back)) 
    {
        maxBytes = i2cCtrl_getRxFifoLevel(pI2c);
		
        I2C_FIFO_READ(maxBytes);
        i2cCtrl_flushRxHold(pI2c);
		
        /* number of bytes that were received during the last transfer*/
        retval = instance->rxLength - instance->rxRemain;
    }
    else 
	{
        /* number of bytes that were sent during the last transfer*/
        retval = instance->txLength - instance->txRemain;
    }
	
    /* sanity check .. retval should never be less than zero*/
    FMSH_ASSERT(retval >= 0);

    /* terminate current transfer*/
    instance->state = I2c_state_idle;
    instance->txCallback = NULL;
    instance->txBuffer = NULL;
    instance->rxCallback = NULL;
    instance->rxBuffer = NULL;

    /* restore user-specified tx/rx fifo threshold*/
    reg = 0;
    FMSH_BIT_SET(reg, I2C_TX_TL_TX_TL, instance->txThreshold);
    I2C_OUTP(reg, portmap->tx_tl);
	
    reg = 0;
    FMSH_BIT_SET(reg, I2C_RX_TL_RX_TL, instance->rxThreshold);
    I2C_OUTP(reg, portmap->rx_tl);
    
    /* End of critical section of code. This macros restores FMSH_apb_i2c*/
    /* interrupts.*/
    I2C_EXIT_CRITICAL_SECTION();

    /* sanity check*/
    FMSH_ASSERT(i2cCtrl_isIrqMasked(pI2c, I2c_irq_tx_empty) == true);

    return retval;
}

#endif

/*****************************************************************************
*
* @description
*  Unmasks specified I2C interrupt(s).
*
* @param   
*  pI2c is i2c device handle. 
*  interrupts is interrupt(s) to enable
*
* @return  
*  none
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_intr_mask/all bits
* SEE ALSO
*  i2cCtrl_maskIrq(), i2cCtrl_clearIrq(), i2cCtrl_isIrqMasked(),
*  i2cCtrl_Irq_T
* SOURCE
*
****************************************************************************/
void i2cCtrl_unMask_Irq(vxT_I2C* pI2c, int interrupts)
{
    UINT32 reg;

    /*reg = I2C_INP(portmap->intr_mask);	*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_INTR_MASK);
	
    /* avoid bus write if irq already enabled*/
    if ((interrupts & reg) != interrupts) 
	{
        reg |= interrupts;
		
        /* save current value of interrupt mask register*/
        /*instance->intr_mask_save = reg;*/
        pI2c->pI2cCtrl->intr_mask_save = reg;
		
        /*I2C_OUTP(reg, portmap->intr_mask);*/
        i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_INTR_MASK, reg);
    }

	return;
}

/*****************************************************************************
*
* @description
*  Masks specified I2C interrupt(s).
*
* @param   
*  pI2c is i2c device handle. 
*  interrupts is interrupt(s) to disable
*
* @return  
*  none
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_intr_mask/all bits
* SEE ALSO
*  i2cCtrl_unmaskIrq(), i2cCtrl_clearIrq(), i2cCtrl_isIrqMasked(),
*  FIicPs_Irq_T
* SOURCE
*
****************************************************************************/
void i2cCtrl_Mask_Irq(vxT_I2C* pI2c, int         interrupts)
{
    UINT32 reg;

    /*reg = I2C_INP(portmap->intr_mask);*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_INTR_MASK);
	
    /* avoid bus write if interrupt(s) already disabled*/
    if ((interrupts & reg) != 0)
	{
        reg &= ~interrupts;
		
        /* save current value of interrupt mask register*/
        /*instance->intr_mask_save = reg;*/
        /*I2C_OUTP(reg, portmap->intr_mask);*/
		pI2c->pI2cCtrl->intr_mask_save = reg;
		i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_INTR_MASK, reg);		
    }

	return;
}

/*****************************************************************************
*
* @description
*  Clears specified I2C interrupt(s).  Only the following interrupts
*  can be cleared in this fashion: rx_under, rx_over, tx_over, rd_req,
*  tx_abrt, rx_done, activity, stop_det, start_det, gen_call.  Although
*  they can be specified, the tx_empty and rd_req interrupts cannot be
*  cleared using this function.
*
* @param   
*  pI2c is i2c device handle. 
*  interrupts is interrupt(s) to clear
*
* @return  
*  none
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_clr_intr/clr_intr
*   - ic_clr_rx_under/clr_rx_under
*   - ic_clr_rx_over/clr_rx_over
*   - ic_clr_tx_over/clr_tx_over
*   - ic_clr_rd_req/clr_rd_req
*   - ic_clr_tx_abrt/clr_tx_abrt
*   - ic_clr_rx_done/clr_rx_done
*   - ic_clr_activity/clr_activity
*   - ic_clr_stop_det/clr_stop_det
*   - ic_clr_start_det/clr_start_det
*   - ic_clr_gen_call/clr_gen_call
* SEE ALSO
*  i2cCtrl_unmaskIrq(), i2cCtrl_maskIrq(), i2cCtrl_isIrqMasked(),
*  FIicPs_Irq_T
* SOURCE
*
****************************************************************************/
void i2cCtrl_Clear_Irq(vxT_I2C* pI2c, int interrupts)
{
    UINT32 reg;

    if (interrupts == I2C_IRQ_ALL) /* I2c_irq_all*/
    {    
    	/*reg = I2C_INP(portmap->clr_intr);*/
    	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CLR_INTR);
    }
    else 
	{
        if ((interrupts & I2C_IRQ_RX_UNDER) != 0) /*I2c_irq_rx_under*/
        {    
        	/*reg = I2C_INP(portmap->clr_rx_under);*/
	    	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CLR_RX_UNDER);
        }
		
        if ((interrupts & I2C_IRQ_RX_OVER) != 0) /*I2c_irq_rx_over*/
        {    
        	/*reg = I2C_INP(portmap->clr_rx_over);*/
			reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CLR_RX_OVER);
        }
		
        if ((interrupts & I2C_IRQ_TX_OVER) != 0) /* I2c_irq_tx_over*/
        {    
        	/*reg = I2C_INP(portmap->clr_tx_over);*/
			reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CLR_TX_OVER);
        }
		
        if ((interrupts & I2C_IRQ_RD_REQ) != 0) /* I2c_irq_rd_req*/
        {    
        	/*reg = I2C_INP(portmap->clr_rd_req);*/
			reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CLR_RD_REQ);
        }
		
        if ((interrupts & I2C_IRQ_TX_ABRT) != 0) /* I2c_irq_tx_abrt*/
        {    
        	/*reg = I2C_INP(portmap->clr_tx_abrt);*/
			reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CLR_TX_ABRT);
        }
		
        if ((interrupts & I2C_IRQ_RX_DONE) != 0) /* I2c_irq_rx_done*/
        {    
        	/*reg = I2C_INP(portmap->clr_rx_done);*/
			reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CLR_RX_DONE);
        }
		
        if ((interrupts & I2C_IRQ_ACTIVITY) != 0)  /* I2c_irq_activity*/
        {    
        	/*reg = I2C_INP(portmap->clr_activity);*/
			reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CLR_ACTIVITY);
        }
		
        if ((interrupts & I2C_IRQ_STOP_DET) != 0) /* I2c_irq_stop_det*/
        {    
        	/*reg = I2C_INP(portmap->clr_stop_det);*/
			reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CLR_STOP_DET);
        }
		
        if ((interrupts & I2C_IRQ_START_DET) != 0)  /* I2c_irq_start_det*/
        {    
        	/*reg = I2C_INP(portmap->clr_start_det);*/
			reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CLR_START_DET);
        }
		
        if ((interrupts & I2C_IRQ_RESTART_DET) != 0) /*I2c_irq_restart_det*/
        {    
        	/*reg = I2C_INP(portmap->clr_restart_det);*/
			reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CLR_RESTART_DET);
        }
		
        if ((interrupts & I2C_IRQ_GEN_CALL) != 0)  /* I2c_irq_gen_call*/
        {   
        	/*reg = I2C_INP(portmap->clr_gen_call);*/
			reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CLR_GEN_CALL);
        }
		
        if ((interrupts & I2C_SCL_STUCK_AT_LOW) != 0) /* I2c_scl_stuck_at_low*/
        {    
        	/* reg = I2C_INP(portmap->clr_scl_stuck_det);*/
			reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CLR_SCL_STUCK_DET);
        }
    }

	return;
}

/*****************************************************************************
*
* @description
*  Returns whether the specified I2C interrupt is masked or not.  Only
*  one interrupt can be specified at a time.
*
* @param   
*  pI2c is i2c device handle. 
*  interrupts is interrupt to check
*
* @return  
*  true        -- interrupt is enabled
*  false       -- interrupt is disabled
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_intr_mask/all bits
* SEE ALSO
*  i2cCtrl_unmaskIrq(), i2cCtrl_maskIrq(), i2cCtrl_clearIrq(),
*  i2cCtrl_getIrqMask(), FIicPs_Irq_T
* SOURCE
*
****************************************************************************/
int i2cCtrl_isMask_Irq(vxT_I2C* pI2c, int          interrupt)
{
    int retval;
    UINT32 reg;

    /*reg = I2C_INP(portmap->intr_mask);*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_INTR_MASK);
    
    reg &= interrupt;

    if (reg == 0)
    {    
    	retval = 1;  /* true;*/
	}
    else
    {    
    	retval = 0; /* false;*/
	}

    return retval;
}

/*****************************************************************************
*
* @description
*  Returns the current interrupt mask.  For each bitfield, a value of
*  '0' indicates that an interrupt is masked while a value of '1'
*  indicates that an interrupt is enabled.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  The DW_apb_i2c interrupt mask.
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_intr_mask/all bits
* SEE ALSO
*  i2cCtrl_unmaskIrq(), i2cCtrl_maskIrq(), i2cCtrl_clearIrq(),
*  i2cCtrl_isIrqMasked(), FIicPs_Irq_T
* SOURCE
*
****************************************************************************/
UINT32 i2cCtrl_Get_IrqMask(vxT_I2C* pI2c)
{
    UINT32 retval;

    /*retval = I2C_INP(portmap->intr_mask);*/
	retval = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_INTR_MASK);

    return retval;
}

/*****************************************************************************
*
* @description
*  Returns whether an I2C interrupt is active or not, after the masking
*  stage.
*
* @param   
*  pI2c is i2c device handle. 
*  interrupt is interrupt to check
*
* @return  
*  true        -- irq is active
*  false       -- irq is inactive
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_intr_stat/all bits
* SEE ALSO
*  i2cCtrl_isRawIrqActive(), i2cCtrl_unmaskIrq(), i2cCtrl_maskIrq(),
*  i2cCtrl_isIrqMasked(), i2cCtrl_clearIrq(), i2cCtrl_Irq_T
* SOURCE
*
****************************************************************************/
int i2cCtrl_isActive_Irq(vxT_I2C* pI2c, int           interrupt)
{
    int retval;
    UINT32 reg;

    /*reg = I2C_INP(portmap->intr_stat);    */
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_INTR_STAT);
	
    reg &= interrupt;

    if (reg == 0)
    {    
    	retval = 0;  /* false;*/
	}
    else
    {    
    	retval = 1;  /* true;*/
	}

    return retval;
}

/*****************************************************************************
*
* @description
*  Returns whether an I2C raw interrupt is active or not, regardless of
*  masking.
*
* @param   
*  pI2c is i2c device handle. 
*  interrupt is interrupt to check
*
* @return  
*  true        -- irq is active
*  false       -- irq is inactive
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_raw_intr_stat/all bits
* SEE ALSO
*  i2cCtrl_isIrqActive(), i2cCtrl_unmaskIrq(), i2cCtrl_maskIrq(),
*  i2cCtrl_isIrqMasked(), i2cCtrl_clearIrq(), FIicPs_Irq_T
* SOURCE
*
****************************************************************************/
int i2cCtrl_isActive_RawIrq(vxT_I2C* pI2c, int            interrupt)
{
    int retval;
    UINT32 reg;

    /*reg = I2C_INP(portmap->raw_intr_stat);	*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_RAW_INTR_STAT);
	
    reg &= interrupt;

    if (reg == 0)
    {    
    	retval = 0;  /* false;*/
	}
    else
    {    
    	retval = 1;  /* true;*/
	}

    return retval;
}

/*****************************************************************************
*
* @description
*  This function is used to set the DMA mode for transmit transfers.
*  Possible options are none (disabled), software or hardware
*  handshaking.  For software handshaking, a transmit notifier function
*  (notifies the DMA that the I2C is ready to accept more data) must
*  first be set via the i2cCtrl_setNotifier_destinationReady() function.
*  The transmitter empty interrupt is masked for hardware handshaking
*  and unmasked (and managed) for software handshaking or when the DMA
*  mode is set to none.
*
* @param   
*  pI2c is i2c device handle. 
*  mode is DMA mode to set (none, hw or sw handshaking).
*
* @return  
*  0           -- if successful
*  -FMSH_ENOSYS  -- if device does not have a DMA interface
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_dma_cr/tdmae
*
*  This function is affected by the HAS_DMA hardware parameter.
*
*  This function is part of the Interrupt API and should not be called
*  when using the I2C in a poll-driven manner.
*
*  This function cannot be used when using an interrupt handler other
*  than i2cCtrl_irqHandler().
* SEE ALSO
*  i2cCtrl_getDmaTxMode(), i2cCtrl_getDmaTxLevel(),
*  i2cCtrl_setNotifier_destinationReady(), i2cCtrl_setDmaRxMode()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Set_DmaTxMode(vxT_I2C* pI2c, int mode)
{
    int retval;
    UINT32 reg;
	
    /*if (param->has_dma == true) */
    if (pI2c->pI2cCtrl->has_dma == 1) 
	{
        retval = 0;
		
        /*if (instance->dmaTx.mode != mode) */
        if (pI2c->pI2cCtrl->dmaTx_mode != mode) 
		{
            /*reg = I2C_INP(portmap->dma_cr);            */
			reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_DMA_CR);
			
            /*instance->dmaTx.mode = mode;*/
            pI2c->pI2cCtrl->dmaTx_mode = mode;
			
			/* FMSH_dma_hw_handshake mode = hw, enable the transmit FIFO DMA channel.*/
			/*
			bit1:	TDMAE	R/W 发送DMA使能，使能TX FIFO DMA通道。
					0：禁用
					1：使能	复位值：0x0
			*/
            if (mode == I2C_DMA_HW_HANDSHAKE)  
            {    
            	/*FMSH_BIT_SET(reg, I2C_DMA_CR_TDMAE, 0x1);*/
            	reg |= 0x02;
			}
            else
            {    
            	/*FMSH_BIT_SET(reg, I2C_DMA_CR_TDMAE, 0x0);*/
				reg &= (~0x02);
			}
			
            /*I2C_OUTP(reg, portmap->dma_cr);*/
            i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_DMA_CR, reg);
        }
    }
    else
    {    
    	retval = -FMSH_ENOSYS;
    }

    return retval;
}

/*****************************************************************************
*
* @description
*  This function returns the current DMA mode for I2C transmit
*  transfers.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  The current DMA transmit mode.
*
* @note    
*  This function is part of the Interrupt API and should not be called
*  when using the I2C in a poll-driven manner.
*
*  TThis function cannot be used when using an interrupt handler other
*  than i2cCtrl_irqHandler().
* SEE ALSO
*  i2cCtrl_setDmaTxMode(), i2cCtrl_getDmaRxMode()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Get_DmaTxMode(vxT_I2C* pI2c)
{
    int retval;

    /*retval = instance->dmaTx.mode;*/
    retval = pI2c->pI2cCtrl->dmaTx_mode;

    return retval;
}

/*****************************************************************************
*
* @description
*  This function is used to set the DMA mode for receive transfers.
*  Possible options are none (disabled), software or hardware
*  handshaking.  For software handshaking, a receive notifier function
*  (notifies the DMA that the I2C is ready to accept more data) must
*  first be setup via the i2cCtrl_setNotifier_sourceReady() function.
*  The receiver full interrupt is masked for hardware handshaking and
*  unmasked for software handshaking or when the DMA mode is set to
*  none.
*
* @param   
*  pI2c is i2c device handle. 
*  mode is DMA mode to set (none, hw or sw handshaking)
*
* @return  
*  0           -- if successful
*  -FMSH_ENOSYS  -- if device does not have a DMA interface
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_dma_cr/tdmae
*
*  This function is affected by the HAS_DMA hardware parameter.
*
*  This function is part of the Interrupt API and should not be called
*  when using the I2C in a poll-driven manner.
*
*  This function cannot be used when using an interrupt handler other
*  than i2cCtrl_irqHandler().
* SEE ALSO
*  i2cCtrl_getDmaRxMode(), i2cCtrl_getDmaTxLevel(), dw_dma_mode,
*  i2cCtrl_setNotifier_sourceReady(), i2cCtrl_setDmaRxMode()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Set_DmaRxMode(vxT_I2C* pI2c, int mode)
{
    int retval;
    UINT32 reg;

    /*if (param->has_dma == true) */
    if (pI2c->pI2cCtrl->has_dma == 1) 
	{
        retval = 0;
		
        /*if (instance->dmaRx.mode != mode) */
        if (pI2c->pI2cCtrl->dmaRx_mode != mode) 
		{
            /*reg = I2C_INP(portmap->dma_cr);*/
			reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_DMA_CR);
			
            /*instance->dmaRx.mode = mode;*/
			pI2c->pI2cCtrl->dmaRx_mode= mode;
			
            if (mode == I2C_DMA_HW_HANDSHAKE) /* FMSH_dma_hw_handshake*/
			{
                /*FMSH_BIT_SET(reg, I2C_DMA_CR_RDMAE, 0x1);	*/
                reg |= 0x02;
				
                /* mask rx full interrupt*/
                /*i2cCtrl_maskIrq(pI2c, I2c_irq_rx_full);*/
                i2cCtrl_Mask_Irq(pI2c, I2C_IRQ_RX_FULL);
            }
            else 
			{
                /*FMSH_BIT_SET(reg, I2C_DMA_CR_RDMAE, 0x0);*/
                reg |= (~0x02);
				
                /* unmask rx full interrupt*/
                /*i2cCtrl_unmaskIrq(pI2c, I2c_irq_rx_full);*/
				i2cCtrl_unMask_Irq(pI2c, I2C_IRQ_RX_FULL);                
            }
			
            /*I2C_OUTP(reg, portmap->dma_cr);*/
            i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_DMA_CR, reg); 
        }
    }
    else
    {    
    	retval = -FMSH_ENOSYS;
    }

    return retval;
}

/*****************************************************************************
*
* @description
*  This function returns the current DMA mode for I2C transmit
*  transfers.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  The current DMA transmit mode.
*
* @note    
*  This function is part of the Interrupt API and should not be called
*  when using the I2C in a poll-driven manner.
*
*  This function cannot be used when using an interrupt handler other
*  than i2cCtrl_irqHandler().
* SEE ALSO
*  i2cCtrl_setDmaTxMode(), i2cCtrl_getDmaRxMode()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Get_DmaRxMode(vxT_I2C* pI2c)
{
    int retval;

    /*retval = instance->dmaRx.mode;*/
    retval = pI2c->pI2cCtrl->dmaRx_mode;

    return retval;
}

/*****************************************************************************
*
* @description
*  This function sets the threshold level at which new data is
*  requested from the DMA.  This is used for DMA hardware handshaking
*  mode only.
*
* @param   
*  pI2c is i2c device handle. 
*  level is DMA request threshold level.
*
* @return  
*  0           -- if successful
*  -FMSH_ENOSYS  -- if device does not have a DMA interface
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_dma_tdlr/dmatdl
*
*  This function is affected by the HAS_DMA hardware parameter.
* SEE ALSO
*  i2cCtrl_getDmaTxLevel(), i2cCtrl_setDmaTxMode()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Set_DmaTxLevel(vxT_I2C* pI2c, UINT8 level)
{
    int retval;
    UINT32 reg;

    if (pI2c->pI2cCtrl->has_dma == 1) 
	{
        reg = 0;
		
        /*FMSH_BIT_SET(reg, I2C_DMA_TDLR_DMATDL, level);*/
        reg |= level;
		
        /*I2C_OUTP(reg, portmap->dma_tdlr);*/
		i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_DMA_TDLR, reg);

		retval = 0;
    }
    else
    {    
    	retval = -FMSH_ENOSYS;
    }

    return retval;
}

/*****************************************************************************
*
* @description
*  This functions gets the current DMA transmit data threshold level.
*  This is the FIFO level at which the DMA is requested to send more
*  data from the I2C.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  The current DMA transmit data level threshold.
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_dma_tdlr/dmatdl
* SEE ALSO
*  i2cCtrl_setDmaTxLevel(), i2cCtrl_setDmaTxMode()
* SOURCE
*
****************************************************************************/
UINT8 i2cCtrl_Get_DmaTxLevel(vxT_I2C* pI2c)
{
    UINT32 reg;
    UINT32 retval;

    /* reg = I2C_INP(portmap->dma_tdlr);*/
    /* retval = FMSH_BIT_GET(reg, I2C_DMA_TDLR_DMATDL);*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_DMA_TDLR);
	retval = reg & 0xFF;

    return retval;
}

/*****************************************************************************
*
* @description
*  This function sets the threshold level at which the DMA is requested
*  to receive data from the I2C.  This is used for DMA hardware
*  handshaking mode only.
*
* @param   
*  pI2c is i2c device handle. 
*  level is DMA request threshold level
*
* @return  
*  0           -- if successful
*  -FMSH_ENOSYS  -- if device does not have a DMA interface
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_dma_rdlr/dmardl
*
*  This function is affected by the HAS_DMA hardware parameter.
* SEE ALSO
*  i2cCtrl_getDmaRxLevel(), i2cCtrl_setDmaRxMode()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Set_DmaRxLevel(vxT_I2C* pI2c, UINT8 level)
{
    int retval;
    UINT32 reg;

    if (pI2c->pI2cCtrl->has_dma == 1) 
	{
        reg = 0;
		
        /*FMSH_BIT_SET(reg, I2C_DMA_RDLR_DMARDL, level);*/
        /*I2C_OUTP(reg, portmap->dma_rdlr);*/
        reg |= level;
		
		i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_DMA_RDLR, reg);
		
		retval = 0;
    }
    else
    {    
    	retval = -FMSH_ENOSYS;
    }

    return retval;
}

/*****************************************************************************
*
* @description
*  This functions gets the current DMA receive data threshold level.
*  This is the FIFO level at which the DMA is requested to receive from
*  the I2C.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  The current DMA receive data level threshold.
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_dma_rdlr/dmardl
* SEE ALSO
*  i2cCtrl_setDmaRxLevel(), i2cCtrl_setDmaRxMode()
* SOURCE
*
****************************************************************************/
UINT8 i2cCtrl_Get_DmaRxLevel(vxT_I2C* pI2c)
{
    UINT32 reg;
    UINT8 retval;

    /*reg = I2C_INP(portmap->dma_rdlr);*/
    /*retval = FMSH_BIT_GET(reg, I2C_DMA_RDLR_DMARDL);*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_DMA_RDLR);
	retval = (UINT8)(reg & 0xFF);

    return retval;
}

#if 0
/*****************************************************************************
*
* @description
*  This function sets the user DMA transmit notifier function.  This
*  function is required when the DMA transmit mode is software
*  handshaking.  The I2C driver calls this function at a predefined
*  threshold to request the DMA to send more data to the I2C.
*
* @param   
*  pI2c is i2c device handle. 
*  funcptr  is called to request more data from the DMA
*  dmac  is associated DW_ahb_dmac device handle
*  channel is channel number used for the transfer
*
* @return  
*  0           -- if successful
*  -FMSH_ENOSYS  -- if device does not have a DMA interface
*
* @note    
*  This function is affected by the HAS_DMA hardware parameter.
*
*  This function is part of the Interrupt API and should not be called
*  when using the I2C in a poll-driven manner.
*
*  This function cannot be used when using an interrupt handler other
*  than i2cCtrl_irqHandler().
* SEE ALSO
*  i2cCtrl_setNotifier_sourceReady(), i2cCtrl_setDmaTxMode(),
*  i2cCtrl_setTxThreshold()
* SOURCE
*
****************************************************************************/
int i2cCtrl_setNotifier_destinationReady(vxT_I2C* pI2c,
        FMSH_dmaNotifierFunc funcptr, FDmaPs_T*dmac, unsigned
        channel)
{
    int retval;
    FIicPs_Param_T*param;
    FIicPs_Instance_T*instance;

    I2C_COMMON_REQUIREMENTS(pI2c);
    FMSH_REQUIRE(funcptr != NULL);
    FMSH_REQUIRE(dmac != NULL);
    FMSH_REQUIRE(dmac->comp_type == FMSH_ahb_dmac);

    param = (FIicPs_Param_T*) pI2c->comp_param;
    instance = (FIicPs_Instance_T*) pI2c->instance;

    if(param->has_dma == true) {
        retval = 0;
        instance->dmaTx.notifier = funcptr;
        instance->dmaTx.dmac = dmac;
        instance->dmaTx.channel = channel;
    }
    else
        retval = -FMSH_ENOSYS;

    return retval;
}

/*****************************************************************************
*
* @description
*  This function sets the user DMA receive notifier function.  This
*  function is required when the DMA receive mode is software
*  handshaking.  The I2C driver calls this function at a predefined
*  threshold to inform the DMA that data is ready to be read from the
*  I2C.
*
* @param   
*  pI2c is i2c device handle. 
*  funcptr  is called to inform the DMA to fetch more data
*  dmac is associated DMA device handle
*  channel is channel number used for the transfer
*
* @return  
*  0           -- if successful
*  -FMSH_ENOSYS  -- if device does not have a DMA interface
*
* @note    
*  This function is affected by the HAS_DMA hardware parameter.
*
*  This function is part of the Interrupt API and should not be called
*  when using the I2C in a poll-driven manner.
*
*  This function cannot be used when using an interrupt handler other
*  than i2cCtrl_irqHandler().
* SEE ALSO
*  i2cCtrl_setNotifier_destinationReady(), i2cCtrl_setDmaRxMode(),
*  i2cCtrl_setRxThreshold()
* SOURCE
*
****************************************************************************/
int i2cCtrl_setNotifier_sourceReady(vxT_I2C* pI2c,
        FMSH_dmaNotifierFunc funcptr, FDmaPs_T*dmac, unsigned
        channel)
{
    int retval;
    FIicPs_Param_T*param;
    FIicPs_Instance_T*instance;

    I2C_COMMON_REQUIREMENTS(pI2c);
    FMSH_REQUIRE(funcptr != NULL);
    FMSH_REQUIRE(dmac != NULL);
    FMSH_REQUIRE(dmac->comp_type == FMSH_ahb_dmac);

    param = (FIicPs_Param_T*) pI2c->comp_param;
    instance = (FIicPs_Instance_T*) pI2c->instance;

    if(param->has_dma == true) {
        retval = 0;
        instance->dmaRx.notifier = funcptr;
        instance->dmaRx.dmac = dmac;
        instance->dmaRx.channel = channel;
    }
    else
        retval = -FMSH_ENOSYS;

    return retval;
}

/*****************************************************************************
*
* @description
*  This function handles and processes I2C interrupts.  It works in
*  conjunction with the Interrupt API and a user listener function
*  to manage interrupt-driven transfers.  When fully using the
*  Interrupt API, this function should be called whenever a DW_apb_i2c
*  interrupt occurs.  There is an alternate interrupt handler
*  available, i2cCtrl_userIrqHandler(), but this cannot be used in
*  conjunction with the other Interrupt API functions.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  true        -- an interrupt was processed
*  false       -- no interrupt was processed
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_intr_mask/all bits
*   - ic_intr_stat/all bits
*   - ic_clr_rx_under/clr_rx_under
*   - ic_clr_rx_over/clr_rx_over
*   - ic_clr_tx_over/clr_tx_over
*   - ic_clr_rd_req/clr_rd_req
*   - ic_clr_tx_abrt/clr_tx_abrt
*   - ic_clr_rx_done/clr_rx_done
*   - ic_clr_activity/clr_activity
*   - ic_clr_stop_det/clr_stop_det
*   - ic_clr_start_det/clr_start_det
*   - ic_clr_gen_call/clr_gen_call
*   - ic_status/rfne
*   - ic_rxflr/rxflr
*   - ic_data_cmd/cmd
*   - ic_data_cmd/dat
*   - ic_rx_tl/rx_tl
*   - ic_txflr/txflr
*
*  This function is part of the Interrupt API and should not be called
*  when using the I2C in a poll-driven manner.
* WARNINGS
*  The user listener function is run in interrupt context and, as such,
*  care must be taken to ensure that any data shared between it and
*  normal code is adequately protected from corruption.  Depending on
*  the target platform, i2cnlocks, mutexes or semaphores may be used to
*  achieve this.  The other Interrupt API functions disable I2C
*  interrupts before entering critical sections of code to avoid any
*  shared data issues.
* SEE ALSO
*  i2cCtrl_masterTransmit(), i2cCtrl_masterReceive(),
*  i2cCtrl_masterBack2Back(),  i2cCtrl_slaveTransmit(),
*  i2cCtrl_slaveReceive(), i2cCtrl_terminate(),
*  i2cCtrl_slaveBulkTransmit()
* SOURCE
*
****************************************************************************/
int i2cCtrl_irqHandler(vxT_I2C* pI2c)
{
    int retval;
    UINT8*tmp;
    UINT32 reg;
    int i, maxBytes;
    int32_t callbackArg;
    FMSH_callback userCallback;
    FIicPs_Irq_T clearIrqMask;
    FIicPs_Param_T*param;
    FIicPs_PortMap_T*portmap;
    FIicPs_Instance_T*instance;

    I2C_COMMON_REQUIREMENTS(pI2c);

    param = (FIicPs_Param_T*) pI2c->comp_param;
    portmap = (FIicPs_PortMap_T*) pI2c->base_address;
    instance = (FIicPs_Instance_T*) pI2c->instance;

    userCallback = NULL;
    callbackArg = 0;
    clearIrqMask = I2c_irq_none;
    /* Assume an interrupt will be processed.  This will be set to false*/
    /* if no active interrupt is found.*/
    retval = true;

    /* what caused the interrupt?*/
    reg = I2C_INP(portmap->intr_stat);

    /* if an error has occurred*/
    if((reg & (I2c_irq_tx_abrt | I2c_irq_rx_over | I2c_irq_rx_under |
                    I2c_irq_tx_over)) != 0) {
        instance->state = I2c_state_error;
        userCallback = instance->listener;
        i2cCtrl_maskIrq(pI2c, I2c_irq_tx_empty);
        /* if a tx transfer was aborted*/
        if((reg & I2c_irq_tx_abrt) != 0) {
            callbackArg = I2c_irq_tx_abrt;
            clearIrqMask = I2c_irq_tx_abrt;
        }
        /* rx fifo overflow*/
        else if((reg & I2c_irq_rx_over) != 0) {
            callbackArg = I2c_irq_rx_over;
            clearIrqMask = I2c_irq_rx_over;
        }
        /* rx fifo underflow*/
        else if((reg & I2c_irq_rx_under) != 0) {
            callbackArg = I2c_irq_rx_under;
            clearIrqMask = I2c_irq_rx_under;
        }
        /* tx fifo overflow*/
        else if((reg & I2c_irq_tx_over) != 0) {
            callbackArg = I2c_irq_tx_over;
            clearIrqMask = I2c_irq_tx_over;
        }
    }
    /* a general call was detected*/
    else if((reg & I2c_irq_gen_call) != 0) {
        userCallback = instance->listener;
        callbackArg = I2c_irq_gen_call;
        clearIrqMask = I2c_irq_gen_call;
        /* update state -- awaiting user to start a slave rx transfer*/
        switch(instance->state) {
            case I2c_state_idle:
                instance->state = I2c_state_rx_req;
                break;
            case I2c_state_master_tx:
                instance->state = I2c_state_master_tx_gen_call;
                break;
            case I2c_state_slave_rx:
                /* leave state unchanged; user already has an rx buffer*/
                /* set up to receive data.*/
                break;
            default:
                /* should never reach this clause*/
                FMSH_ASSERT(false);
                break;
        }
    }
    /* rx fifo level at or above threshold*/
    else if((reg & I2c_irq_rx_full) != 0) {
        /* The rx full interrupt should not be unmasked when a DMA*/
        /* interface with hardware handshaking is being used.*/
        FMSH_REQUIRE(instance->dmaRx.mode != FMSH_dma_hw_handshake);
        if(instance->dmaRx.mode == FMSH_dma_sw_handshake) {
            /* The user must have previously set an rx notifier via*/
            /* i2cCtrl_setDmaRxNotifier.*/
            FMSH_REQUIRE(instance->dmaRx.notifier != NULL);
            /* Disable the Rx full interrupt .. this is re-enabled*/
            /* after the DMA has finished the current transfer (via a*/
            /* callback set in the DMA driver by the user).*/
            i2cCtrl_maskIrq(pI2c, I2c_irq_rx_full);
            /* Notify the DMA that the Rx FIFO has data to be read.*/
            /* This function and its arguments are set by the user via*/
            /* the i2cCtrl_setNotifier_sourceReady() function.*/
            (instance->dmaRx.notifier)(instance->dmaRx.dmac,
                                       instance->dmaRx.channel,
                                       false, false);
        }
        else {
            if((instance->state == I2c_state_idle) ||
               (instance->state == I2c_state_slave_tx) ||
               (instance->state == I2c_state_slave_bulk_tx)) {
                /* sanity check: rxBuffer should be NULL in these states*/
                FMSH_ASSERT(instance->rxBuffer == NULL);
                /* inform the user listener function of the event*/
                userCallback = instance->listener;
                callbackArg = I2c_irq_rx_full;
                switch(instance->state) {
                    case I2c_state_idle:
                        instance->state = I2c_state_rx_req;
                        break;
                    case I2c_state_slave_tx:
                        instance->state = I2c_state_slave_tx_rx_req;
                        break;
                    case I2c_state_slave_bulk_tx:
                        instance->state =
                            I2c_state_slave_bulk_tx_rx_req;
                        break;
                }
            }
            else {
                FMSH_ASSERT(instance->rxBuffer != NULL);
                /* does the rx buffer need to be word-aligned?*/
                if(instance->rxAlign == false) {
                    /* align buffer:*/
                    tmp = (UINT8*) instance->rxBuffer;
                    /* repeat until either the buffer is aligned, there*/
                    /* is no more space in the rx buffer or there is no*/
                    /* more data to read from the rx fifo*/
                    while((((unsigned) tmp) & 0x3) &&
                            (instance->rxRemain > 0) &&
                            (i2cCtrl_isRxFifoEmpty(pI2c) == false)) {
                       *tmp++ = I2C_IN16P(portmap->data_cmd);
                        instance->rxRemain--;
                    }
                    instance->rxBuffer = tmp;
                    if(((unsigned) tmp & 0x3) == 0)
                        instance->rxAlign = true;
                }       /* instance->rxAlign == false*/
                /* This code is only executed when the rx buffer is*/
                /* word-aligned as I2C_FIFO_READ works efficiently with*/
                /* a word-aligned buffer.*/
                if(instance->rxAlign == true) {
                    maxBytes = i2cCtrl_getRxFifoLevel(pI2c);
                    I2C_FIFO_READ(maxBytes);
                }       /* instance->rxAlign == true*/
                /* if the rx buffer is full*/
                if(instance->rxRemain == 0) {
                    /* Prepare to call the user callback function to*/
                    /* notify it that the current transfer has finished.*/
                    /* For an rx or back-to-back transfer, the number of*/
                    /* bytes received is passed as an argument to the*/
                    /* listener function.*/
                    userCallback = instance->rxCallback;
                    callbackArg = instance->rxLength;
                    /* Flush the instance->rxHold regsiter to the rx*/
                    /* buffer.*/
                    i2cCtrl_flushRxHold(pI2c);
                    /* transfer complete*/
                    instance->rxBuffer = NULL;
                    instance->rxCallback = NULL;
                    /* restore rx threshold to user-specified value*/
                    I2C_OUTP(instance->rxThreshold, portmap->rx_tl);
                    /* update state*/
                    switch(instance->state) {
                        case I2c_state_master_rx:
                            /* End of pI2c-receiver transfer.  Ensure*/
                            /* that the tx empty interrupt is disabled.*/
                            i2cCtrl_maskIrq(pI2c, I2c_irq_tx_empty);
                        case I2c_state_slave_rx:
                            /* return to idle state*/
                            instance->state = I2c_state_idle;
                            break;
                        case I2c_state_back2back:
                            /* Back-to-back transfer is complete if*/
                            /* there is no more data to send.  Else, the*/
                            /* callback function is not called until all*/
                            /* bytes have been transmitted.  Note that*/
                            /* txCallback is cleared when all bytes have*/
                            /* been sent and/or received.*/
                            if(instance->txCallback == NULL) {
                                instance->state = I2c_state_idle;
                                FMSH_ASSERT(i2cCtrl_isIrqMasked(pI2c,
                                        I2c_irq_tx_empty) == true);
                            }
                            else
                                userCallback = NULL;
                            break;
                        case I2c_state_slave_tx_rx:
                            instance->state = I2c_state_slave_tx;
                            break;
                        case I2c_state_slave_bulk_tx_rx:
                            instance->state = I2c_state_slave_bulk_tx;
                            break;
                        case I2c_state_master_tx_slave_rx:
                            instance->state = I2c_state_master_tx;
                            break;
                        default:
                            /* this clause should never be reached*/
                            FMSH_ASSERT(false);
                            break;
                    }
                }       /* remain == 0*/
                else if(instance->rxRemain < (instance->rxThreshold +
                            1)) {
                    reg = 0;
                    FMSH_BIT_SET(reg, I2C_RX_TL_RX_TL, (instance->rxRemain
                                - 1));
                    I2C_OUTP(reg, portmap->rx_tl);
                }
            }       /* instance->rxBuffer != NULL*/
        }
    }       /* (reg & I2c_irq_rx_full) != 0*/
    /* read-request transfer completed (tx fifo may still contain data)*/
    else if((reg & I2c_irq_rx_done) != 0) {
        clearIrqMask = I2c_irq_rx_done;
        switch(instance->state) {
            case I2c_state_slave_tx:
                /* return to idle state if tx transfer finished*/
                if(instance->txRemain == 0) {
                    instance->state = I2c_state_idle;
                    FMSH_ASSERT(i2cCtrl_isIrqMasked(pI2c,
                            I2c_irq_tx_empty) == true);
                    callbackArg = 0;
                    /* call user tx callback function*/
                    userCallback = instance->txCallback;
                    /* clear tx buffer and callback function pointers*/
                    instance->txBuffer = NULL;
                    instance->txCallback = NULL;
                }
                break;
            case I2c_state_slave_tx_rx:
                /* return to slave-rx state if tx transfer finished*/
                if(instance->txRemain == 0) {
                    instance->state = I2c_state_slave_rx;
                    callbackArg = 0;
                    /* call user tx callback function*/
                    userCallback = instance->txCallback;
                    /* clear tx buffer and callback function pointers*/
                    instance->txBuffer = NULL;
                    instance->txCallback = NULL;
                }
                break;
            case I2c_state_slave_bulk_tx:
                /* mask tx empty interrupt*/
                i2cCtrl_maskIrq(pI2c, I2c_irq_tx_empty);
                /* return to idle state*/
                instance->state = I2c_state_idle;
                /* call user tx callback function*/
                userCallback = instance->txCallback;
                /* number of bytes left unsent*/
                callbackArg = instance->txRemain +
                    i2cCtrl_getTxFifoLevel(pI2c);
                /* clear tx buffer and callback function pointers*/
                instance->txBuffer = NULL;
                instance->txCallback = NULL;
                break;
            case I2c_state_slave_bulk_tx_rx:
                /* mask tx empty interrupt*/
                i2cCtrl_maskIrq(pI2c, I2c_irq_tx_empty);
                /* return to slave rx state*/
                instance->state = I2c_state_slave_rx;
                /* call user tx callback function*/
                userCallback = instance->txCallback;
                /* number of bytes left unsent*/
                callbackArg = instance->txRemain +
                    i2cCtrl_getTxFifoLevel(pI2c);
                /* clear tx buffer and callback function pointers*/
                instance->txBuffer = NULL;
                instance->txCallback = NULL;
                break;
            default:
                /* should not get rx_done in any other driver state*/
                FMSH_ASSERT(false);
                break;
        }
    }
    /* read request received*/
    else if((reg & I2c_irq_rd_req) != 0) {
        switch(instance->state) {
            case I2c_state_idle:
                clearIrqMask = I2c_irq_rd_req;
                instance->state = I2c_state_rd_req;
                userCallback = instance->listener;
                callbackArg = I2c_irq_rd_req;
                break;
            case I2c_state_slave_rx:
                clearIrqMask = I2c_irq_rd_req;
                instance->state = I2c_state_slave_rx_rd_req;
                userCallback = instance->listener;
                callbackArg = I2c_irq_rd_req;
                break;
            case I2c_state_slave_tx:
            case I2c_state_slave_tx_rx:
                clearIrqMask = I2c_irq_rd_req;
                /* remain in the current state and write the next byte*/
                /* from the tx buffer to the tx fifo*/
                I2C_FIFO_WRITE(1);
                break;
            case I2c_state_slave_bulk_tx_rx:
                /* A read request has occurred because, even though we*/
                /* are performing a slave bulk transfer, the system did*/
                /* not keep the tx FIFO from emptying.  This interrupt*/
                /* is therefore treated the same as a tx empty*/
                /* interrupt.*/
                maxBytes = MIN(param->tx_buffer_depth -
                    i2cCtrl_getTxFifoLevel(pI2c), instance->txRemain);
                /* buffer should be word-aligned (done by*/
                /* i2cCtrl_slaveBulkTransmit)*/
                if(instance->txRemain > 0) {
                    clearIrqMask = I2c_irq_rd_req;
                    I2C_FIFO_WRITE(maxBytes);
                }
                else {
                    /* tx buffer has all been sent in bulk mode yet the*/
                    /* pI2c is still requesting more data.  We need to*/
                    /* call the tx callback function first and then pass*/
                    /* read request to the user listener.*/
                    /* update state*/
                    instance->state = I2c_state_slave_rx;
                    /* mask tx empty interrupt*/
                    i2cCtrl_maskIrq(pI2c, I2c_irq_tx_empty);
                    /* call user callback function with no bytes left to*/
                    /* send*/
                    userCallback = instance->txCallback;
                    callbackArg = 0;
                    instance->txBuffer = NULL;
                    instance->txCallback = NULL;
                }
                break;
            case I2c_state_slave_bulk_tx:
                /* A read request has occurred because, even though we*/
                /* are performing a slave bulk transfer, the system did*/
                /* not keep the tx FIFO from emptying.  This interrupt*/
                /* is therefore treated the same as a tx empty*/
                /* interrupt.*/
                maxBytes = MIN(param->tx_buffer_depth -
                    i2cCtrl_getTxFifoLevel(pI2c), instance->txRemain);
                /* buffer should be word-aligned (done by*/
                /* i2cCtrl_slaveBulkTransmit)*/
                if(instance->txRemain > 0) {
                    clearIrqMask = I2c_irq_rd_req;
                    I2C_FIFO_WRITE(maxBytes);
                }
                else {
                    /* tx buffer has all been sent in bulk mode yet the*/
                    /* pI2c is still requesting more data.  We need to*/
                    /* call the tx callback function first and then pass*/
                    /* read request to the user listener.*/
                    /* update state*/
                    instance->state = I2c_state_idle;
                    /* mask tx empty interrupt*/
                    i2cCtrl_maskIrq(pI2c, I2c_irq_tx_empty);
                    FMSH_ASSERT(i2cCtrl_isIrqMasked(pI2c,
                            I2c_irq_tx_empty) == true);
                    /* call user callback function with no bytes left to*/
                    /* send*/
                    userCallback = instance->txCallback;
                    callbackArg = 0;
                    instance->txBuffer = NULL;
                    instance->txCallback = NULL;
                }
                break;
            default:
                /* should not get rd_req in any other driver state*/
                FMSH_ASSERT(false);
                break;
        }
    }
    /* tx fifo level at or below threshold*/
    else if((reg & I2c_irq_tx_empty) != 0) {
        /* The tx empty interrupt should never be unmasked when we are*/
        /* using DMA with hardware handshaking.*/
        FMSH_REQUIRE(instance->dmaTx.mode != FMSH_dma_hw_handshake);
        if(instance->dmaTx.mode == FMSH_dma_sw_handshake) {
            /* The user must have previously set a tx notifier.*/
            FMSH_REQUIRE(instance->dmaTx.notifier != NULL);
            /* Disable the tx empty interrupt .. this is re-enabled*/
            /* after the DMA has finished the current transfer (via a*/
            /* callback set by the user).*/
            i2cCtrl_maskIrq(pI2c, I2c_irq_tx_empty);
            /* Notify the dma that the tx fifo is ready to receive mode*/
            /* data.  This function and its arguments are set by the*/
            /* user via the i2cCtrl_setNotifier_destinationReady()*/
            /* function.*/
            (instance->dmaTx.notifier)(instance->dmaTx.dmac,
                                       instance->dmaTx.channel,
                                       false, false);
        }
        else if(instance->txRemain == 0) {
            /* default: call callback function with zero as argument (no*/
            /* bytes left to send)*/
            /* tx callback function*/
            userCallback = instance->txCallback;
            /* number of bytes left to transmit*/
            callbackArg = 0;
            i2cCtrl_maskIrq(pI2c, I2c_irq_tx_empty);
            /* restore user-specfied tx threshold value*/
            I2C_OUTP(instance->txThreshold, portmap->tx_tl);
            /* update driver state*/
            switch(instance->state) {
                case I2c_state_master_tx:
                    /* return to idle state at end of tx transfer*/
                    instance->state = I2c_state_idle;
                    FMSH_ASSERT(i2cCtrl_isIrqMasked(pI2c,
                            I2c_irq_tx_empty) == true);
                    /* transfer complete*/
                    instance->txBuffer = NULL;
                    instance->txCallback = NULL;
                    break;
                case I2c_state_master_tx_slave_rx:
                    /* return to slave-rx state if slave rx transfer*/
                    /* is still in progress*/
                    instance->state = I2c_state_slave_rx;
                    /* transfer complete*/
                    instance->txBuffer = NULL;
                    instance->txCallback = NULL;
                    break;
                case I2c_state_master_rx:
                    /* Reset tx buffer and callback function pointers*/
                    instance->txBuffer = NULL;
                    instance->txCallback = NULL;
                    /* For a pI2c-rx transfer, the callback is not*/
                    /* called until the last byte has been received.*/
                    userCallback = NULL;
                    callbackArg = NULL;
                    break;
                case I2c_state_slave_bulk_tx:
                case I2c_state_slave_bulk_tx_rx:
                    /* For slave bulk transfers, the callback is not*/
                    /* called until rx_done has been received.*/
                    userCallback = NULL;
                    callbackArg = NULL;
                    break;
                case I2c_state_back2back:
                    if(instance->rxCallback == NULL) {
                        /* If there is no more data to receive either,*/
                        /* return to the idle state and call callback*/
                        /* with the number of bytes received.*/
                        instance->state = I2c_state_idle;
                        FMSH_ASSERT(i2cCtrl_isIrqMasked(pI2c,
                            I2c_irq_tx_empty) == true);
                        callbackArg = instance->rxLength;
                    }
                    else {
                        /* Otherwise, if there is still data to receive,*/
                        /* do not call the user callback function.*/
                        userCallback = NULL;
                        callbackArg = NULL;
                    }
                    /* Reset tx buffer and callback function pointers*/
                    instance->txBuffer = NULL;
                    instance->txCallback = NULL;
                    break;
                default:
                    /* we shouldn't get a tx_empty interrupt in any*/
                    /* other driver state.*/
                    FMSH_ASSERT(false);
                    break;
            }
        }
        else {
            switch(instance->state) {
                case I2c_state_master_tx:
                case I2c_state_slave_bulk_tx:
                case I2c_state_slave_bulk_tx_rx:
                case I2c_state_master_tx_slave_rx:
                    /* slave-transmitter or pI2c-transmitter*/
                    maxBytes = param->tx_buffer_depth -
                        i2cCtrl_getTxFifoLevel(pI2c);
                    /* buffer should already be word-aligned*/
                    I2C_FIFO_WRITE(maxBytes);
                    break;
                case I2c_state_master_rx:
                    maxBytes = MIN((param->tx_buffer_depth -
                        i2cCtrl_getTxFifoLevel(pI2c)),
                            instance->txRemain);
                    for(i = 0; i < maxBytes; i++)
                    { 
                         I2C_OUTP(0x100, portmap->data_cmd);
                    }
                    
                    instance->txRemain -= maxBytes;
                    break;
                case I2c_state_back2back:
                    maxBytes = MIN((param->tx_buffer_depth -
                                i2cCtrl_getTxFifoLevel(pI2c)),
                            instance->txRemain);
                    I2C_FIFO_WRITE16(maxBytes);
                    instance->txRemain -= maxBytes;
                    break;
                default:
                    /* we shouldn't get a tx_empty interrupt in any*/
                    /* other driver state.*/
                    FMSH_ASSERT(false);
                    break;
            }
            /* If the tx buffer is empty, set the tx threshold to no*/
            /* bytes in fifo.  This is to ensure the the tx callback*/
            /* function, if any, is only called when the current*/
            /* transfer has been completed by the FMSH_apb_i2c device.*/
            if(instance->txRemain == 0)
                I2C_OUTP(0x0, portmap->tx_tl);
        }
    }
    /* start condition detected*/
    else if((reg & I2c_irq_start_det) != 0) {
        userCallback = instance->listener;
        callbackArg = I2c_irq_start_det;
        clearIrqMask = I2c_irq_start_det;
    }
    /* stop condition detected*/
    else if((reg & I2c_irq_stop_det) != 0) {
        userCallback = instance->listener;
        callbackArg = I2c_irq_stop_det;
        clearIrqMask = I2c_irq_stop_det;
    }
    /* i2c bus activity*/
    else if((reg & I2c_irq_activity) != 0) {
        userCallback = instance->listener;
        callbackArg = I2c_irq_activity;
        clearIrqMask = I2c_irq_activity;
    }
	/*i2c bus scl stuck at low*/
	else if((reg & I2c_scl_stuck_at_low) != 0){
        userCallback = instance->listener;
        callbackArg = I2c_scl_stuck_at_low;
        clearIrqMask = I2c_scl_stuck_at_low;
    }
	
    else {
        /* If we've reached this point, either the enabling and*/
        /* disabling of I2C interrupts is not being handled properly or*/
        /* this function is being called unnecessarily.*/
        retval = false;
    }

    /* call the user listener function, if it has been set*/
    if(userCallback != NULL)
        userCallback(pI2c, callbackArg);

    /* If the driver is still in one of these states, the user listener*/
    /* function has not correctly handled a device event/interrupt.*/
    FMSH_REQUIRE(instance->state != I2c_state_rx_req);
    FMSH_REQUIRE(instance->state != I2c_state_rd_req);
    FMSH_REQUIRE(instance->state != I2c_state_slave_tx_rx_req);
    FMSH_REQUIRE(instance->state != I2c_state_slave_rx_rd_req);
    FMSH_REQUIRE(instance->state != I2c_state_master_tx_gen_call);
    FMSH_REQUIRE(instance->state != I2c_state_error);

    /* clear the serviced interrupt*/
    if(clearIrqMask != 0)
        i2cCtrl_clearIrq(pI2c, clearIrqMask);

    return retval;
}

/*****************************************************************************
*
* @description
*   This function identifies the current highest priority active
*  interrupt, if any, and forwards it to a user-provided listener
*  function for processing.  This allows a user absolute control over
*  how each I2C interrupt is processed.
*
*  None of the other Interrupt API functions can be used with this
*  interrupt handler.  This is because they are symbiotic with the
*  i2cCtrl_irqHandler() interrupt handler.  All Command and Status API
*  functions, however, can be used within the user listener function.
*  This is in contrast to i2cCtrl_irqHandler(), where i2cCtrl_read(),
*  i2cCtrl_Write_Fifo() and i2cCtrl_issueRead() cannot be used within the user
*  listener function.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  true        -- an interrupt was processed
*  false       -- no interrupt was processed
*
* @note    
*  Accesses the following DW_apb_i2c register(s)/bit field(s):
*   - ic_intr_stat/all bits
*   - ic_clr_rx_under/clr_rx_under
*   - ic_clr_rx_over/clr_rx_over
*   - ic_clr_tx_over/clr_tx_over
*   - ic_clr_rd_req/clr_rd_req
*   - ic_clr_tx_abrt/clr_tx_abrt
*   - ic_clr_rx_done/clr_rx_done
*   - ic_clr_activity/clr_activity
*   - ic_clr_stop_det/clr_stop_det
*   - ic_clr_start_det/clr_start_det
*   - ic_clr_gen_call/clr_gen_call
*
*  This function is part of the Interrupt API and should not be called
*  when using the I2C in a poll-driven manner.
* WARNINGS
*  The user listener function is run in interrupt context and, as such,
*  care must be taken to ensure that any data shared between it and
*  normal code is adequately protected from corruption.  Depending on
*  the target platform, i2cnlocks, mutexes or semaphores may be used to
*  achieve this.
* SEE ALSO
*  i2cCtrl_setListener()
* SOURCE
*
****************************************************************************/
int i2cCtrl_userIrqHandler(vxT_I2C* pI2c)
{
    bool retval;
    UINT32 reg;
    int32_t callbackArg;
    UINT32 fifo_level,i;
    UINT32 tmp=0;
    FMSH_callback userCallback;
    FIicPs_Irq_T clearIrqMask;
    FIicPs_PortMap_T*portmap;

    I2C_COMMON_REQUIREMENTS(pI2c);
   
    portmap = (FIicPs_PortMap_T*) pI2c->base_address;

    /* Assume an interrupt will be processed.  userCallback will be set*/
    /* to NULL and retval to false if no active interrupt is found.*/
    retval = true;
   /* userCallback = instance->listener;*/
    userCallback = NULL;
    callbackArg = 0;
    clearIrqMask = I2c_irq_none;

    /* what caused the interrupt?*/
    reg = I2C_INP(portmap->intr_stat);
    /*reg = I2C_INP(portmap->raw_intr_stat);*/
    /* if a tx transfer was aborted*/
    if((reg & I2c_irq_tx_abrt) != 0) {
        callbackArg = I2c_irq_tx_abrt;
        clearIrqMask = I2c_irq_tx_abrt;
       
        VX_DBG(DEBUG_OUT, "*** Irq: TX_ABRT...\n");
    }
    /* rx fifo overflow*/
    else if((reg & I2c_irq_rx_over) != 0) {
        callbackArg = I2c_irq_rx_over;
        clearIrqMask = I2c_irq_rx_over;
  
        VX_DBG(DEBUG_OUT, "*** Irq: RX_OVER...\n");
    }
    /* rx fifo underflow*/
    else if((reg & I2c_irq_rx_under) != 0) {
        callbackArg = I2c_irq_rx_under;
        clearIrqMask = I2c_irq_rx_under;
      
        VX_DBG(DEBUG_OUT, "*** Irq: RX_UNDER...\n");
    }
    /* tx fifo overflow*/
    else if((reg & I2c_irq_tx_over) != 0) {
        callbackArg = I2c_irq_tx_over;
        clearIrqMask = I2c_irq_tx_over;
       
        VX_DBG(DEBUG_OUT, "*** Irq: TX_OVER...\n");
    }
    /* a general call was detected*/
    else if((reg & I2c_irq_gen_call) != 0) {
        callbackArg = I2c_irq_gen_call;
        clearIrqMask = I2c_irq_gen_call;
    
        VX_DBG(DEBUG_OUT, "*** Irq: GEN_CALL...\n");
    }
    /* rx fifo level at or above threshold*/
    else if((reg & I2c_irq_rx_full) != 0) {
        callbackArg = I2c_irq_rx_full;
       
        /*read fifo*/
        fifo_level = i2cCtrl_getRxFifoLevel(pI2c);
        for(i=0;i<fifo_level;i++)
        {
	     tmp=I2C_INP(portmap->data_cmd);
        }
        
        VX_DBG(DEBUG_OUT, "*** Irq: RX_FULL...\n");
    }
    /* read-request transfer completed (Tx FIFO may still contain data)*/
    else if((reg & I2c_irq_rx_done) != 0) {
        callbackArg = I2c_irq_rx_done;
        clearIrqMask = I2c_irq_rx_done;
     
        VX_DBG(DEBUG_OUT, "*** Irq: RX_DONE...\n");
    }
    /* read request received*/
    else if((reg & I2c_irq_rd_req) != 0) {
        callbackArg = I2c_irq_rd_req;
        clearIrqMask = I2c_irq_rd_req;
     /*   i2cCtrl_Write_Fifo(pI2c,0x33);*/
 
       /* I2C_OUTP(0X00,portmap->data_cmd);*/
        VX_DBG(DEBUG_OUT, "*** Irq: RD_REQ...\n");
        
        for(i=0;i<16;i++)
        {
	    I2C_OUTP(i,portmap->data_cmd);
        }
        
    }
    /* tx fifo level at or below threshold*/
    else if((reg & I2c_irq_tx_empty) != 0) {
        callbackArg = I2c_irq_tx_empty;
        i2cCtrl_maskIrq(pI2c,I2c_irq_tx_empty);  /*mask tx_empty*/
       

        VX_DBG(DEBUG_OUT, "*** Irq: TX_EMPTY...\n");
    }
    /* start condition detected*/
    else if((reg & I2c_irq_start_det) != 0) {
        callbackArg = I2c_irq_start_det;
        clearIrqMask = I2c_irq_start_det;
       
        VX_DBG(DEBUG_OUT, "*** Irq: START_DET...\n");
    }
    /* restart condition detected*/
    else if((reg & I2c_irq_restart_det) != 0) {
        callbackArg = I2c_irq_restart_det;
        clearIrqMask = I2c_irq_restart_det;
     
        VX_DBG(DEBUG_OUT, "*** Irq: RESTART_DET...\n");
    }
    /* stop condition detected*/
    else if((reg & I2c_irq_stop_det) != 0) {
        callbackArg = I2c_irq_stop_det;
        clearIrqMask = I2c_irq_stop_det;
       
        VX_DBG(DEBUG_OUT, "*** Irq: STOP_DET...\n");
    }
    /* i2c bus activity*/
    else if((reg & I2c_irq_activity) != 0) {
        callbackArg = I2c_irq_activity;
        clearIrqMask = I2c_irq_activity;
     
        VX_DBG(DEBUG_OUT, "*** Irq: ACTIVITY...\n");
    }
      else if((reg & I2c_irq_mst_on_hold) != 0) {
        callbackArg = I2c_irq_mst_on_hold;
        clearIrqMask = I2c_irq_mst_on_hold;

        VX_DBG(DEBUG_OUT, "*** Irq: MST_ON_HOLD...\n");
        i2cCtrl_maskIrq(pI2c,I2c_irq_mst_on_hold); 
    }
     else if((reg & I2c_scl_stuck_at_low) != 0) {
        callbackArg = I2c_scl_stuck_at_low;
        clearIrqMask = I2c_scl_stuck_at_low;
       
        VX_DBG(DEBUG_OUT, "*** Irq: SCL_STUCK_AT_LOW...\n");
    }
    else {
        /* no active interrupt was found*/
        retval = false;
        userCallback = NULL;
    }

    /* call the user listener function, if there was an active interrupt*/
    if(userCallback != NULL)
        userCallback(pI2c, callbackArg);

    /* clear any serviced interrupt*/
    if(clearIrqMask != 0)
        i2cCtrl_clearIrq(pI2c, clearIrqMask);

    return retval;
}

void i2cCtrl_Reset_RxInstance(vxT_I2C* pI2c)
{
  /*int retval;*/
  FIicPs_PortMap_T*portmap;
  FIicPs_Instance_T*instance;
  
  instance = (FIicPs_Instance_T*) pI2c->instance;
  portmap = (FIicPs_PortMap_T*) pI2c->base_address;
  
  I2C_ENTER_CRITICAL_SECTION();
  
  instance->state          = I2c_state_idle;
  instance->intr_mask_save = I2c_irq_rd_req;
  
  I2C_EXIT_CRITICAL_SECTION();
}

#endif

void i2cCtrl_Set_SDAHold(vxT_I2C* pI2c, UINT16 rxHold, UINT16 txHold)
{
     UINT32 reg;
     
     reg = rxHold;
     reg <<= 16;
     reg += txHold;
	 
     /*I2C_OUTP(reg, portmap->reserved1);*/
	i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_RESERVE1, reg);
	return;
}

void i2cCtrl_Set_DeviceID(vxT_I2C* pI2c)
{
	UINT32 reg;

	/*reg = I2C_INP(portmap->tar);*/
	/*reg |= (1<<13);*/
	/*I2C_OUTP(reg, portmap->tar);*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_TAR);
	reg |= (1 << 13);
	i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_TAR, reg);

	return;	 
}

void i2cCtrl_Clr_DeviceID(vxT_I2C* pI2c)
{
     UINT32 reg;
     
     /*reg=I2C_INP(portmap->tar);	 */
     /*if(reg&(1<<13)!=0)*/
     /*  reg&=~(1<<13);*/
     /* I2C_OUTP(reg, portmap->tar);*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_TAR);
	if (reg & (1 << 13) != 0)
	{	
		reg &= ~(1 << 13);
	}
	
	i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_TAR, reg);
	return;
}

/*void FI2cPs_issueSTOP(FI2cPs_T *ev,uint8_t character)*/
void i2cCtrl_Xfer_STOP(vxT_I2C* pI2c, UINT8 character)
{
  UINT16 wSend;  
  wSend = (0x0200) | (UINT16)character;
  
  /*I2C_OUTP(wSend,portmap->data_cmd);*/
	i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_DATA_CMD, wSend);
	return;
}

void i2cCtrl_Xfer_STOPWrite(vxT_I2C* pI2c, UINT8 character)
{
	UINT16 wSend;

	wSend = (0x200) | (UINT16)character;

	/*I2C_OUTP(wSend,portmap->data_cmd);*/
	i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_DATA_CMD, wSend);
	return;
}

void i2cCtrl_Xfer_STOPRead(vxT_I2C* pI2c, UINT8 character)
{
	UINT16 wSend;

	wSend = (0x300) | (UINT16)character;

	/*I2C_OUTP(wSend,portmap->data_cmd);*/
	i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_DATA_CMD, wSend);
	return;
}

void i2cCtrl_Config(vxT_I2C* pI2c, UINT32 data)
{
	/*I2C_OUTP(data,portmap->con);*/
	i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_CON, data);
	return;
}

void i2cCtrl_Set_AckGenCall(vxT_I2C* pI2c)
{  
	/*I2C_OUTP(0x01,portmap->ack_gen_call);*/
	i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_ACK_GEN_CALL, 0x01);
	return;
}

void i2cCtrl_Xfer_RESTART(vxT_I2C* pI2c, UINT8 character)
{
	UINT16 wSend;
	wSend = (0x400) | (UINT16)character;

	/*I2C_OUTP(wSend,portmap->data_cmd);*/
	i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_DATA_CMD, wSend);
	return;
}

UINT32 i2cCtrl_Clr_TxAbrt(vxT_I2C* pI2c)
{
	UINT32 reg;

	/* Read IC_CLR_TX_ABRT to clear TX_ABRT*/
	/*reg = I2C_INP(portmap->clr_tx_abrt); */
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CLR_TX_ABRT);
	return reg;
}

int i2cCtrl_Set_SCLTimeout(vxT_I2C* pI2c, UINT32 timeout)
{
    int retval;

	if (i2cCtrl_isEnable(pI2c) == 0) 
	{
        /*I2C_OUTP(timeout, portmap->scl_low_timeout);*/
		i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_SCL_LOW_TIMEOUT, timeout);
        
		retval = 0;
    }
    else
    {    
    	retval = -FMSH_EPERM;
    }

	return retval;
}

int i2cCtrl_Set_SDATimeout(vxT_I2C* pI2c, UINT32 timeout)
{
    int retval;

	if (i2cCtrl_isEnable(pI2c) == 0) 
	{
        /*I2C_OUTP(timeout, portmap->sda_low_timeout);*/
		i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_SDA_LOW_TIMEOUT, timeout);
		
		retval = 0;
    }
    else
    {    
    	retval = -FMSH_EPERM;
    }

	return retval;
}

int i2cCtrl_Enable_BusClear(vxT_I2C* pI2c)
{
    int retval;
    UINT32 reg;

    if (i2cCtrl_isEnable(pI2c) == 0) 
	{
        /*reg = I2C_INP(portmap->con);*/
        reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_CON);
		
        /* avoid bus write if possible*/
        /*if (FMSH_BIT_GET(reg, I2C_CON_BUS_CLEAR) != 0x1) */
        /*
		bit11	BUS_CLEAR_FEATURE_CTRL	R/W 主机模式下：
				1：总线清除使能
				0：总线清除禁止,		复位值：0x0		从机模式无效
		*/
        if (((reg & 0x0800) >> 11) != 0x1) 
		{
            /*FMSH_BIT_SET(reg, I2C_CON_BUS_CLEAR, 0x1);*/
            /* I2C_OUTP(reg, portmap->con);*/
            reg |= 0x0800;
			i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_CON, reg);
        }
		
		retval = 0;
    }
    else
    {    
    	retval = -FMSH_EPERM;
    }

    return retval;
}

void i2cCtrl_Master_Abort(vxT_I2C* pI2c)
{
    UINT32 reg;

    reg = 0;
	
    /*FMSH_BIT_SET(reg, I2C_ENABLE_ABORT, 0x1);*/
    /*I2C_OUTP(reg, portmap->enable);*/
	/*
	bit1:	ABORT 置位时，控制器中止传输
	*/
	reg |= 0x02;
	i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_ENABLE, reg);

	return;
}

void i2cCtrl_Enable_SdaStuckRecovery(vxT_I2C* pI2c)
{
    UINT32 reg;

    /*reg = I2C_INP(portmap->enable);*/
    /*reg |=(1<<3);*/
    /*I2C_OUTP(reg, portmap->enable);*/
    
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_ENABLE);
	reg |= (1 << 3);
	i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_ENABLE, reg);
	
	return;
}

int i2cCtrl_isNot_SdaStuckRecovery(vxT_I2C* pI2c)
{
    UINT32 reg;
    int retval;

    /*reg = I2C_INP(portmap->status);*/
	reg = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_STATUS);
	/*
	bit11:	SDA_STUCK_RECOVERED R	
	         该位表示SDA仍保持为低，没有从恢复机制中恢复。	复位值：0x0
	*/
    if (reg & (1 << 11) != 0)
    {    
    	retval = 1;  /* true;*/
	}
    else
    {    
    	retval = 0;  /* false;*/
	}

    return retval;
}


UINT32 i2cCtrl_Get_RawIntrStat(vxT_I2C* pI2c)
{
    UINT32 retval;

    /*retval = I2C_INP(portmap->raw_intr_stat);*/
	retval = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_RAW_INTR_STAT);

    return retval;
}

void i2cCtrl_Setup_Clock(vxT_I2C* pI2c, unsigned ic_clk)
{
    UINT16 ss_scl_high, ss_scl_low;
    UINT16 fs_scl_high, fs_scl_low;
    UINT16 hs_scl_high, hs_scl_low;
 
    /* ic_clk is the clock speed (in MHz) that is being supplied to the*/
    /* DW_apb_i2c device.  The correct clock count values are determined*/
    /* by using this inconjunction with the minimum high and low signal*/
    /* hold times as per the I2C bus specification.*/
    ss_scl_high = ((UINT16)(((SS_MIN_SCL_HIGH * ic_clk) / 1000) + 1));
    ss_scl_low  = ((UINT16)(((SS_MIN_SCL_LOW * ic_clk) / 1000) + 1));
	
    fs_scl_high = ((UINT16)(((FS_MIN_SCL_HIGH * ic_clk) / 1000) + 1));
    fs_scl_low  = ((UINT16)(((FS_MIN_SCL_LOW * ic_clk) / 1000) + 1));
	
    hs_scl_high = ((UINT16)(((HS_MIN_SCL_HIGH_100PF * ic_clk) / 1000) + 1));
    hs_scl_low  = ((UINT16)(((HS_MIN_SCL_LOW_100PF * ic_clk) / 1000) + 1));

    i2cCtrl_Set_SclCount(pI2c, I2_SPEED_STANDARD, I2C_SCL_HIGH, ss_scl_high);
    i2cCtrl_Set_SclCount(pI2c, I2_SPEED_STANDARD, I2C_SCL_LOW,  ss_scl_low);
	
    i2cCtrl_Set_SclCount(pI2c, I2_SPEED_FAST, I2C_SCL_HIGH, fs_scl_high);
    i2cCtrl_Set_SclCount(pI2c, I2_SPEED_FAST, I2C_SCL_LOW,  fs_scl_low);
	
    i2cCtrl_Set_SclCount(pI2c, I2_SPEED_HIGH, I2C_SCL_HIGH, hs_scl_high);
    i2cCtrl_Set_SclCount(pI2c, I2_SPEED_HIGH, I2C_SCL_LOW,  hs_scl_low);

	return;
}


/**********************************************************************/
/*                    PRIVATE FUNCTIONS                               */
/**********************************************************************/

/***
* The following functions are all private and as such are not part of
* the driver's public API.
***/

#if 0
/*****************************************************************************
*
* @description
*   This functions virtually flushes any data in the hold variable to
*  the buffer (both in the i2c_Instance_T structure).  The 'hold'
*  variable normally stores up to four data bytes before they are
*  written to memory (i.e. the user buffer) to optimize bus performace.
*  Flushing the
*  (instance->) hold variable only makes sense when the i2c is in
*  either pI2c-receiver or slave-receiver mode.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  0           if successful
*  -EPERM      if the i2c is not in a receive mode (pI2c-rx/slave-rx)
*
* @note    
* This function comprises part of the interrupt-driven interface and
*  normally should never need to be called directly.  The
*  FMSH_i2c_terminate function always calls FMSH_i2c_flushRxHold before
*  terminating a transfer.
* SEE ALSO
*  FMSH_i2c_terminate()
* SOURCE
*
****************************************************************************/
int i2cCtrl_Flush_RxHold(vxT_I2C* pI2c)
{
    int i, retval;
    UINT8*tmp;
    UINT32 c, mask;
    UINT32*buf;
    FIicPs_Instance_T*instance;

    I2C_COMMON_REQUIREMENTS(pI2c);

    instance = (FIicPs_Instance_T*) pI2c->instance;

    FMSH_REQUIRE(instance->rxBuffer != NULL);

    /* sanity check .. idx should never be greater than four*/
    FMSH_ASSERT(instance->rxIdx <= 4);
	
    retval = 0;
	
    if (instance->rxIdx != 4) 
	{
        /* need to handle the case where there is less*/
        /* than four bytes remaining in the rx buffer*/
        if (instance->rxRemain >= 4) 
		{
            buf = (UINT32*) instance->rxBuffer;
            mask = ((UINT32) (0xffffffff) >> (8 * instance->rxIdx));
            c = mask & (instance->rxHold >> (8 * instance->rxIdx));
           *buf = (*buf & ~mask) | c;
        }
        else 
		{
            /* tmp = next free location in rx buffer*/
            tmp = (UINT8*) instance->rxBuffer;
			
            /* shift hold so that the least*/
            /* significant byte contains valid data*/
            c = instance->rxHold >> (8* instance->rxIdx);
			
            /* write out valid character to rx buffer*/
            for(i = (4 - instance->rxIdx); i > 0; i--) 
			{
               *tmp++ = (UINT8) (c & 0xff);
                c >>= 8;
            }
        }       /* instance->rxRemain <= 4*/
    }       /* instance->rxIdx != 4*/

    return retval;
}

#endif

/*****************************************************************************
*
* @description
*   This function attempts to automatically discover the hardware
*  component parameters, if this supported by the i2c in question.
*  This is usually controlled by the ADD_ENCODED_PARAMS coreConsultant
*  parameter.
*
* @param   
*  pI2c is i2c device handle. 
*
* @return  
*  0           if successful
*  -ENOSYS     function not supported
*
* @note    
* Accesses the following FMSH_apb_i2c register/bitfield(s):
*   - ic_comp_type
*   - ic_comp_version
*   - ic_comp_param_1/all bits
* This function does not allocate any memory.  An instance of
*  FIicPs_Param_T must already be allocated and properly referenced from
*  the relevant compParam i2cCtrl_T  structure member.
* SEE ALSO
*  FMSH_i2c_init()
* SOURCE
*
****************************************************************************/
int i2cCtrl_autoCompParams(vxT_I2C* pI2c)
{
    int retval;
	UINT32 devIP_type;
	UINT32 comp_param_1;

    /*pI2c->comp_version = I2C_INP(portmap->comp_version);*/
	pI2c->pI2cCtrl->devIP_ver = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_COMP_VERSION);

	/*
    // only version 1.03 and greater support identification registers
    */
	devIP_type = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_COMP_TYPE); 
	comp_param_1 = i2cCtrl_Rd_CfgReg32(pI2c, I2C_REG_COMP_PARAM_1); 
	
	/*
	bit23~16	TX_BUFFER_DEPTH R	发送FIFO深度
	
	bit15~8	RX_BUFFER_DEPTH R	接收FIFO深度
	
	bit7	ADD_ENCODED_PARAMS	R	
			1：软件可以通过该寄存器读取参数的数值
			0：不允许软件读取参数值
		
	bit6	HAS_DMA R	是否具有DMA接口
			1：有
			0：无
		
	bit5	INTR_IO R	中断类型
			1：组合		
			0：独立
		
	bit4	HC_COUNT_VALUES R	
			1:硬编码
			0：没有硬编码
		
	bit3~2 MAX_SPEED_MODE	R	
			0x1：standard
			0x2：fAST
			Else：Reserved
		
	bit1~0 APB_DATA_WIDTH	R	APB总线位宽
			0x1：8bit
			0x2：16bit
			0x3：32bit
			0x4：Reserved
	*/
    if ((devIP_type == FMSH_apb_i2c) && (((comp_param_1 & 0x0080) >> 7) == 1))
    {
        retval = 0;
		
        pI2c->pI2cCtrl->hc_count_values = (comp_param_1 & 0x10) >> 4;
		
        pI2c->pI2cCtrl->has_dma = (comp_param_1 & 0x40) >> 6;
		
        pI2c->pI2cCtrl->max_speed_mode = (comp_param_1 & 0x0C) >> 2;
		
        pI2c->pI2cCtrl->rx_buffer_depth = (comp_param_1 & 0x0000FF00) >> 8;		
        pI2c->pI2cCtrl->rx_buffer_depth++;
		
        pI2c->pI2cCtrl->tx_buffer_depth = (comp_param_1 & 0x00FF0000) >> 16;		
        pI2c->pI2cCtrl->tx_buffer_depth++;
    }
    else
    {    
    	retval = -FMSH_ENOSYS;
    }

    return retval;
}


/*void I2C_master_init(FI2cPs_T *ev)*/
void i2cCtrl_Master_Init(vxT_I2C* pI2c)
{
    /*disable the dev I2C device*/
    /*FI2cPs_disable(dev);*/
    i2cCtrl_Disable(pI2c);
	
    /* 
    Config 
    */  
    /*FMSH_OUT32_32(0x1, pPortmap->fs_spklen);  //offset = 0xA0 IC_FS_SPKLEN*/
    /*FMSH_OUT32_32(0x5,pPortmap->reserved1);*/
    i2cCtrl_Wr_CfgReg32(pI2c, I2C_REG_FS_SPKLEN, 0x01);
    
    /* Set up the clock count register.  
     The argument I2C1_CLOCK is specified as the I2C dev input clock.
     */  
    /*FI2cPs_clock_setup(dev, (dev->input_clock)/1000000);*/
    i2cCtrl_Setup_Clock(pI2c, (pI2c->pI2cCtrl->sysClk)/1000000);
    
    /* set the speed mode to standard*/
    /*FI2cPs_setSpeedMode(dev, I2c_speed_standard); // or I2c_speed_high*/
    i2cCtrl_Set_SpeedMode(pI2c, I2c_speed_standard);
    
    /* use 10-bit addressing*/
    /*FI2cPs_setMasterAddressMode(dev, I2c_7bit_address);*/
    /*FI2cPs_setSlaveAddressMode(dev, I2c_7bit_address);*/
    i2cCtrl_Master_Set_AddrMode(pI2c, I2c_7bit_address);
    i2cCtrl_Slave_Set_AddrMode(pI2c, I2c_7bit_address);
    
    /* enable restart conditions*/
    /*FI2cPs_enableRestart(dev);*/
    i2cCtrl_Restart_Enable(pI2c);
    
    /* enable master FSM*/
    /*FI2cPs_enableMaster(dev);*/
	i2cCtrl_Master_Enable(pI2c);
		
    /* Use the start byte protocol with the target address when*/
    /* initiating transfer.*/
    /* FI2cPs_setTxMode(dev, I2c_tx_gen_call);*/
    /*  FI2cPs_setTxMode(dev, I2c_tx_start_byte);*/
    /**/
    /*FI2cPs_setTxMode(dev, I2c_tx_target);*/
    i2cCtrl_Set_TxMode(pI2c, I2c_tx_target);
    
    /* set target address to the I2C slave address*/
    /*FI2cPs_setTargetAddress(dev, SLAVE_ADDRESS);*/
    i2cCtrl_Set_TargetAddr(pI2c, SLAVE_ADDRESS);
	
    /*Set the user listener function*/
    /*FI2cPs_setListener(dev, I2C0_Master_Listener);   // polling*/

    /* clear Irq */
    /*FI2cPs_clearIrq(dev,I2c_irq_all);*/
    i2cCtrl_Clear_Irq(pI2c, I2c_irq_all);
    
    /*enable the dev I2C device*/
    /* FI2cPs_enable(dev);*/

	return;
}

/*void I2C_slave_init(FI2cPs_T *ev)*/
void i2cCtrl_Slave_Init(vxT_I2C* pI2c)
{
    /*disable the dev I2C device*/
    /*FI2cPs_disable(dev);  */
    i2cCtrl_Disable(pI2c);
 
    /* 
    Config  
	*/
    /*FMSH_OUT32_32(0x1D7, pPortmap->reserved3[12]);  //offset = C4    */
    i2cCtrl_Wr_CfgReg32(pI2c, 0xC4, 0x1D7);
	
    /* Set up the clock count register.  The argument I2C1_CLOCK is
     specified as the I2C dev input clock.*/
    /*FI2cPs_clock_setup(dev, (dev->input_clock)/1000000);*/
    i2cCtrl_Setup_Clock(pI2c, (pI2c->pI2cCtrl->sysClk)/1000000);
    
    /* set the speed mode to standard*/
    /*FI2cPs_setSpeedMode(dev, I2c_speed_standard);*/
    i2cCtrl_Set_SpeedMode(pI2c, I2c_speed_standard);
    
    /* use 7-bit addressing*/
    /*FI2cPs_setMasterAddressMode(dev, I2c_7bit_address);*/
    /*FI2cPs_setSlaveAddressMode(dev, I2c_7bit_address);*/
    i2cCtrl_Master_Set_AddrMode(pI2c, I2c_7bit_address);
    i2cCtrl_Slave_Set_AddrMode(pI2c, I2c_7bit_address);
   
    /* enable restart conditions*/
    /*FI2cPs_enableRestart(dev);*/
    i2cCtrl_Restart_Enable(pI2c);
    
    /* enable slave FSM*/
    /*FI2cPs_enableSlave(dev);*/
    /*FI2cPs_disableMaster(dev);*/
	i2cCtrl_Slave_Enable(pI2c);
	i2cCtrl_Master_Disable(pI2c);
    
    /* set target address to the I2C slave address*/
    /*FI2cPs_setSlaveAddress(dev, SLAVE_ADDRESS);*/
    i2cCtrl_Set_SlaveAddr(pI2c, SLAVE_ADDRESS);
    
    /*Set the user listener function*/
    /*FI2cPs_setListener(dev, I2C0_Slave_Listener);  // polling*/
    
    /* clear Irq */
    /* FI2cPs_clearIrq(dev,I2c_irq_all);*/

	return;    
}


#endif

#if 1

int i2cSlcr_Rst_i2c(void)
{
    FMSH_WriteReg(0xE0026000, 0x008, 0xDF0D767BU);/*unlock*/
    FMSH_WriteReg(0xE0026000,0x318,0x3);/**/
    delay_ms(100);
    FMSH_WriteReg(0xE0026000,0x318,0x0);/**/
    return 0;
}



void i2cSlcr_Set_I2cMIO(void)
{
	
	return;
}

/****************************************************************************/
/**
*
*  This function loop I2C0's outputs to I2C1's inputs,and I2C1's outputs to
*  I2C0's inputs
*
* @param
*  loop_en -- 0:connect I2C inputs according to MIO mapping;1:set the loop
*
* @return
*  Null
*
* @note
*  Null
*
****************************************************************************/
void i2cSlcr_Set_I2cLoop(int loop_en)
{
	UINT32 tmp32 = 0;	
	
	/*
	I2C0_LOOP_I2C1	3	rw	0x0 I2C Loopback Control.
					0 = Connect I2C inputs according to MIO mapping.
					1 = Loop I2C 0 outputs to I2C 1 inputs, and I2C 1 outputs
					to I2C 0 inputs.
					
	CAN0_LOOP_CAN1	2	rw	0x0 CAN Loopback Control.
					0 = Connect CAN inputs according to MIO mapping.
					1 = Loop CAN 0 Tx to CAN 1 Rx, and CAN 1 Tx to CAN 0 Rx.
					
	UA0_LOOP_UA1	1	rw	0x0 UART Loopback Control.
					0 = Connect UART inputs according to MIO mapping.
					1 = Loop UART 0 outputs to UART 1 inputs, and UART 1 outputs to UART 0 inputs.
					RXD/TXD cross-connected. RTS/CTS cross-connected.
					DSR, DTR, DCD and RI not used.
					
	SPI0_LOOP_SPI1	0	rw	0x0 SPI Loopback Control.
					0 = Connect SPI inputs according to MIO mapping.
					1 = Loop SPI 0 outputs to SPI 1 inputs, and SPI 1 outputs to SPI 0 inputs.
					The other SPI core will appear on the LS Slave Select.
	*/
	
	if (loop_en == 0)
	{
	    tmp32 = slcr_read(SLCR_MIO_LOOPBACK);
		tmp32 &= ~0x08;
		slcr_write(SLCR_MIO_LOOPBACK, tmp32);
	}
	else if (loop_en == 1)
	{
		tmp32 = slcr_read(SLCR_MIO_LOOPBACK);
		tmp32 |= 0x08;
		slcr_write(SLCR_MIO_LOOPBACK, tmp32);
    }

	return;
}

#endif

#if 1


int vxInit_I2c(int ctrl_x)
{
	vxT_I2C * pI2c = NULL;
	vxT_I2C_CTRL * pI2cCtrl = NULL;
	
	int ret = 0;	
	UINT32 tmp32 = 0;

	
#if 1 
	switch (ctrl_x)
	{
	case I2C_CTRL_0:
		pI2c = g_pI2c0;	
		break;
		
	case I2C_CTRL_1:
		pI2c = g_pI2c1;
		break;
	}

/*	if (pI2c->init_flag == 1)
	{
		return;
	}*/
	
	/**/
	/* enable can_clk of slcr*/
	/**/
	/*i2cSlcr_Enable_I2cClk();*/
	
	/**/
	/* setup can_MIO of slcr*/
	/**/
	i2cSlcr_Set_I2cMIO();


	/*
	init the pI2c structure
	*/
	/**/
	/* i2c_ctrl select*/
	/**/
	switch (ctrl_x)
	{
	case I2C_CTRL_0:
		pI2c = g_pI2c0;	
		bzero(pI2c, sizeof(vxT_I2C));
		
		pI2c->pI2cCtrl = (vxT_I2C_CTRL *)(&vxI2c_Ctrl_0.ctrl_x);
		bzero(pI2c->pI2cCtrl, sizeof(vxT_I2C_CTRL));

		pI2c->i2c_x = I2C_CTRL_0;
		pI2c->pI2cCtrl->devId = I2C_CTRL_0;
		pI2c->pI2cCtrl->cfgBaseAddr = VX_I2C_0_CFG_BASE;
		pI2c->pI2cCtrl->sysClk = FPAR_I2CPS_0_I2C_CLK_FREQ_HZ;
		break;
		
	case I2C_CTRL_1:
		pI2c = g_pI2c1;
		bzero(pI2c, sizeof(vxT_I2C));
	
		pI2c->pI2cCtrl = (vxT_I2C_CTRL *)(&vxI2c_Ctrl_1.ctrl_x);
		bzero(pI2c->pI2cCtrl, sizeof(vxT_I2C_CTRL));
		
		pI2c->i2c_x = I2C_CTRL_1;
		pI2c->pI2cCtrl->devId = I2C_CTRL_1;
		pI2c->pI2cCtrl->cfgBaseAddr = VX_I2C_1_CFG_BASE;
		pI2c->pI2cCtrl->sysClk = FPAR_I2CPS_1_I2C_CLK_FREQ_HZ;
		break;
	}
	
	pI2cCtrl = pI2c->pI2cCtrl;		
	pI2cCtrl->ctrl_x = pI2cCtrl->devId;

	/**/
    /* disable device*/
    /**/
    /*retval = FI2cPs_disable(dev);*/
    ret = i2cCtrl_Disable(pI2c);
	
    /* if device is not busy (i.e. it is now disabled)*/
    if (ret == 0) 
    {
        /* disable all interrupts*/
        /*FI2cPs_maskIrq(dev, I2c_irq_all);*/
		i2cCtrl_Mask_Irq(pI2c, I2c_irq_all);
			
        /*FI2cPs_clearIrq(dev, I2c_irq_all);*/
        i2cCtrl_Clear_Irq(pI2c, I2c_irq_all);

        /* reset instance variables*/
        /*FI2cPs_resetInstance(dev);*/

		/**/
        /* autoConfigure component parameters if possible*/
        /**/
        /*retval = FI2cPs_autoCompParams(dev);*/
		ret = i2cCtrl_autoCompParams(pI2c);
    }

#endif	
	
	pI2c->init_flag = 1;   /* i2c init ok		*/
	return;
}


/****************************************
**  EEPROM Operations
*****************************************/
void i2c_EEPROM_Write(vxT_I2C* pI2c,         UINT16 iaddress, UINT8 byte)
{
    UINT8 bHaddr, bLaddr;
    
    bHaddr = (UINT8)(iaddress >> 8);
    bLaddr = (UINT8)iaddress;
   
    /* write internal address */
    i2cCtrl_Write_Fifo(pI2c, bHaddr);
    i2cCtrl_Write_Fifo(pI2c, bLaddr);
    
    i2cCtrl_Xfer_STOPWrite(pI2c, byte);

	return;  
}

UINT8 i2c_EEPROM_Read(vxT_I2C* pI2c,         UINT16 iaddress)
{
    UINT8 bHaddr, bLaddr;
    UINT8 bRtval = 0;
    /*UINT16 fifo_level=0;*/
    
    bHaddr = (UINT8)(iaddress >> 8);
    bLaddr = (UINT8)iaddress;
    
    /*dummy write*/
    i2cCtrl_Write_Fifo(pI2c, bHaddr);
    i2cCtrl_Write_Fifo(pI2c, bLaddr);
	
    /*Issue read*/
    i2cCtrl_Xfer_READ(pI2c);
     
    while (i2cCtrl_isEmpty_RxFifo(pI2c) == 1);

    bRtval = i2cCtrl_Read_Fifo(pI2c);
	
    return bRtval;       
}

int i2cCtrl_SLCR_Loop(void)
{
	vxT_I2C* pI2c0 = g_pI2c0;
	vxT_I2C* pI2c1 = g_pI2c1;
	
    int error = 0;

	int id = 0x55;	
	int len = 8;

	int ret = FMSH_SUCCESS;

	
    printf("\r\n-I- SLCR_Loop Test\r\n");
	
    /* 
    enable SLCR loop 
	*/
    i2cSlcr_Set_I2cLoop(1);	

	/**/
	/* reset i2c ctrl*/
	/*	*/
	i2cSlcr_Rst_i2c();
		
    /* 
    i2c0 master, i2c1 slaver 
	*/
    /*if (pI2c0->pI2cCtrl->status != 1)*/
	{
		/* Initial I2C0 */
		/*I2C0_device_init(pI2c0_dev,pI2c0_Instance,&I2c0_Param);*/
		/*I2C_master_init(pI2c0_dev);    //In default config the device as master*/
	    vxInit_I2c(I2C_CTRL_0);
		i2cCtrl_Master_Init(pI2c0);
		
		pI2c0->pI2cCtrl->status = 1;
	}	


	/*if (pI2c1->pI2cCtrl->status != 1)*/
    {
		/* Initial I2C1 */
		/*I2C1_device_init(pI2c1_dev,pI2c1_Instance,&I2c1_Param);*/
		/*I2C_slave_init(pI2c1_dev);	  //In default config the device as slave*/
    	vxInit_I2c(I2C_CTRL_1);
		i2cCtrl_Slave_Init(pI2c1);
		
		pI2c1->pI2cCtrl->status = 1;
    }	
	
	/*
	Disable irq 
	*/
	i2cCtrl_Mask_Irq(pI2c0, I2c_irq_all);	 
	i2cCtrl_Mask_Irq(pI2c1, I2c_irq_all);	 
	
	/* 
	Enable device
	*/
	i2cCtrl_Enable(pI2c0);
	i2cCtrl_Enable(pI2c1);

	UINT8 i = 0, data = 0, base = 0x55;
	int timeout = 0x10000;

	printf("Master sending 8 bytes...\n");
	
	for (i=0; i<8; i++)
	{	
		/*FI2cPs_write(pI2c0_dev,  base+i);*/
		i2cCtrl_Write_Fifo(pI2c0, (base+i));
	}

	printf("Slave received bytes: ");
	
	for (i=0; i<8; i++)
	{
		timeout = 0x20000;
		
		/*while ((FI2cPs_isRxFifoEmpty(pI2c1_dev) == true)&&(timeout--));	*/
		while ((i2cCtrl_isEmpty_RxFifo(pI2c1) == 1) && (timeout--));	
		
		if (timeout == 0)
		{
			printf("\n*** ERROR: received timeout ***\n");
			
			ret = FMSH_FAILURE;
			break;
		}
		
		/*data = FI2cPs_read(pI2c1_dev);*/
		data = i2cCtrl_Read_Fifo(pI2c1);
		
		if (data != (base+i))
		{
		   printf("\n*** ERROR: expected 0x%X, received 0x%X ***\n", (base+i), data);  
		   ret = FMSH_FAILURE;	
		}
		else		  
		{	
			printf("%02X ", data);
		}
	}
	
	if (ret != FMSH_SUCCESS)
	{  
		goto END;
	}

	/*delay_ms(10);*/
	taskDelay(10);


	printf("\nMaster issuing 7 read requests...\n");
	
	for (i = 0; i < 8; i++)
	{  
		/*FI2cPs_issueRead(pI2c0_dev);*/
		i2cCtrl_Xfer_READ(pI2c0);
	}

	/*delay_ms(10);*/
	taskDelay(10);

	printf("Write to slave Tx FIFO...\n");
	
	for (i=0; i<8; i++) 
	{
		/*FI2cPs_write(pI2c1_dev, base+i);*/
		i2cCtrl_Write_Fifo(pI2c1, (base+i));
		
		/* clear the read request interrupt*/
		/*FI2cPs_clearIrq(pI2c1_dev, I2c_irq_rd_req);*/
		i2cCtrl_Clear_Irq(pI2c1, I2c_irq_rd_req);
	}

	printf("Read from master Rx FIFO...\n");
	printf("Master received bytes: ");
	
	for (i=0; i<8; i++)
	{
		timeout = 0x20000;
		
		/*while ((FI2cPs_isRxFifoEmpty(pI2c0_dev) == true)&&(timeout--));		*/
		while ((i2cCtrl_isEmpty_RxFifo(pI2c0) == 1) && (timeout--));	
		
		if (timeout == 0)
		{
			printf("\n*** ERROR: received timeout ***\n");
			ret = FMSH_FAILURE;
			break;
		}
		
		/*data = FI2cPs_read(pI2c0_dev);*/
		data = i2cCtrl_Read_Fifo(pI2c0);
		
		if (data != (base+i))
		{
			printf("\n*** ERROR: expected 0x%X, received 0x%X ***\n ", (base+i), data);
			ret = FMSH_FAILURE;
		}
		else
		{	
			printf("%02X ", data);
		}
	}

	printf("\n The Poll mode test is done... \n");
		 
END:	
	/*FI2cPs_issueSTOP(pI2c0_dev, 0x88);*/
    /*FI2cPs_masterAbort(pI2c0_dev);*/
	i2cCtrl_Xfer_STOPWrite(pI2c0, 0x88);
	i2cCtrl_Master_Abort(pI2c0);

	
	return 0;
}

#endif


#if 1


/**----------------------------------
test eeprom
-----------------------------------**/
UINT8 test_i2c_eeprom(void)
{  
	vxT_I2C* pI2c = g_pI2c0;

	int i, j = 0;
	
	UINT8 tmpBuffer[100]={0};

	/* Initial I2C */

	/* Set up the clock count register.  The argument I2C1_CLOCK is
	 specified as the I2C pI2c input clock.*/
	i2cCtrl_Setup_Clock(pI2c, 80);
	
	/* set the speed mode to standard*/
	i2cCtrl_Set_SpeedMode(pI2c, I2_SPEED_STANDARD);  /* I2c_speed_standard*/

	/* use 7-bit addressing*/
	i2cCtrl_Master_Set_AddrMode(pI2c, I2C_7BIT_ADDR);  /* I2c_7bit_address*/
	i2cCtrl_Slave_Set_AddrMode(pI2c, I2C_7BIT_ADDR);

	/* enable restart conditions*/
	i2cCtrl_Restart_Enable(pI2c);

	/* enable pI2c FSM*/
	i2cCtrl_Master_Enable(pI2c);
	i2cCtrl_Slave_Disable(pI2c);

	/**/
	/* Use the start byte protocol with the target address when*/
	/* initiating transfer.*/
	/**/
	/* i2cCtrl_setTxMode(pI2c, I2c_tx_gen_call);*/
	/* i2cCtrl_setTxMode(pI2c, I2c_tx_start_byte);*/
	/**/
	i2cCtrl_Set_TxMode(pI2c, I2C_TX_TARGET);  /* I2c_tx_target*/

	/* set target address to the I2C slave address*/
	/*if (strcmp(pI2c->name, "I2C0") == 0)*/
	{  
		i2cCtrl_Set_TargetAddr(pI2c, 0x50);
	}
	/*
	else if(strcmp(pI2c->name, "I2C1")==0)
	{  
		i2cCtrl_setTargetAddress(pI2c, 0x52); 
	}
	*/

	/*print_init_info(pI2c);*/
	    
	/*enable the pI2c I2C device*/
	i2cCtrl_Enable(pI2c); 
	   
	 for (i=0;i<100;i++)
	 {
	     /*Write*/
	     if(i%10==0)
	       VX_DBG("\nWrite Data:", 1,2,3,4,5,6);
		 
	     VX_DBG(" %d", 0x55+i ,2,3,4,5,6);
		 
	     i2c_EEPROM_Write(pI2c, i, 0x55+i); 
		 
	     delay_ms(20);
		 
	     /*Read  */
	     tmpBuffer[i] = i2c_EEPROM_Read(pI2c,i);
		 
	     if(tmpBuffer[i]!=0x55+i)
	       break;
		 
	     delay_ms(10);
	 }
	 
	 for (j=0; j<i; j++)
	 {
	    if(j%10==0)
	      VX_DBG("\nRead Data:", 1,2,3,4,5,6);
	    
	    VX_DBG(" %d", tmpBuffer[j], 2,3,4,5,6);
	 }
	 
	 i2cCtrl_Master_Abort(pI2c);
	 
	 if (i == 100)
	 {
	     VX_DBG("\nTEST EEPROM Passed.\n", 1,2,3,4,5,6);
	     return 0;
	 }
	 else
	 {
	     VX_DBG("\nTest EEPROM failed.\n", 1,2,3,4,5,6);
	     return 1;
	 }
	 
		return;
}


void test_i2c_9134(void)
{  
	vxT_I2C* pI2c = g_pI2c0;
	int i, j = 0;
	
	int i2c_dev_addr_1 = 0x72;
	int i2c_dev_addr_2 = 0x7A;
	int ret1 = 0, ret2 = 0;


	/* 
	Initial I2C 
	*/
	vxInit_I2c(I2C_CTRL_0);

	/* 
	Set up the clock count register.  The argument I2C1_CLOCK is
	 specified as the I2C pI2c input clock.
	 */
	/*i2cCtrl_Setup_Clock(pI2c, 80);	*/
    i2cCtrl_Setup_Clock(pI2c, (pI2c->pI2cCtrl->sysClk)/1000000);
	
	/* set the speed mode to standard*/
	i2cCtrl_Set_SpeedMode(pI2c, I2_SPEED_STANDARD);  /* I2c_speed_standard*/

	/* use 7-bit addressing*/
	i2cCtrl_Master_Set_AddrMode(pI2c, I2C_7BIT_ADDR);  /* I2c_7bit_address*/
	i2cCtrl_Slave_Set_AddrMode(pI2c, I2C_7BIT_ADDR);

	/* enable restart conditions*/
	i2cCtrl_Restart_Enable(pI2c);

	/* enable pI2c FSM*/
	i2cCtrl_Master_Enable(pI2c);
	i2cCtrl_Slave_Disable(pI2c);

	/**/
	/* Use the start byte protocol with the target address when*/
	/* initiating transfer.*/
	/**/
	/* i2cCtrl_setTxMode(pI2c, I2c_tx_gen_call);*/
	/* i2cCtrl_setTxMode(pI2c, I2c_tx_start_byte);*/
	/**/
	i2cCtrl_Set_TxMode(pI2c, I2C_TX_TARGET);  /* I2c_tx_target*/


	/* set target address to the I2C slave address*/
	/*if (strcmp(pI2c->name, "I2C0") == 0)*/
	{  
		/*i2cCtrl_Set_TargetAddr(pI2c, 0x50);*/
		i2cCtrl_Set_TargetAddr(pI2c, i2c_dev_addr_1);
	}
	
	/*enable the pI2c I2C device*/
	i2cCtrl_Enable(pI2c); 
	
	/*i2c_reg8_write(&IicInstance, 0x72>>1, 0x08, 0x35);	*/
	i2c_dev_addr_1 = 0x72 >> 1;
	/*ret1 = i2c_EEPROM_Read(pI2c, 0x08);*/
	i2c_EEPROM_Write(pI2c, 0x08, 0x35);
	ret2 = i2c_EEPROM_Read(pI2c, 0x08);
	printf("i2c_addr(0x%X)=0x%X-0x%X \n", i2c_dev_addr_1, ret1, ret2);
	
	/*i2c_reg8_write(&IicInstance, 0x7a>>1, 0x2f, 0x00);*/
	i2c_dev_addr_2 = 0x7A >> 1;
	i2cCtrl_Set_TargetAddr(pI2c, i2c_dev_addr_2);
	
	/*ret1 = i2c_EEPROM_Read(pI2c, 0x2f);*/
	i2c_EEPROM_Write(pI2c, 0x2f, 0x00);
	ret2 = i2c_EEPROM_Read(pI2c, 0x2f);
	printf("i2c_addr(0x%X)=0x%X-0x%X \n", i2c_dev_addr_2, ret1, ret2);

	return; 
}


u8 i2cEeprom_init(int i2c_ctrl_x, int i2c_dev_addr)
{
	vxT_I2C * pI2c = NULL;
	
	switch (i2c_ctrl_x)
	{
	case I2C_CTRL_0:
		pI2c = g_pI2c0;	
		break;
		
	case I2C_CTRL_1:
		pI2c = g_pI2c1;
		break;
	}
	
    /* 
     Set up the clock count register.  The argument I2C1_CLOCK is
     specified as the I2C dev input clock.
     */
    /*FI2cPs_ClockSetup(dev, I2C_CLOCK);*/
   /* FI2cPs_ClockSetup(dev, (dev->input_clock)/1000000);*/
    i2cCtrl_Setup_Clock(pI2c, (pI2c->pI2cCtrl->sysClk)/1000000);
	
    /* set the speed mode to standard*/
   /* FI2cPs_setSpeedMode(dev, I2c_speed_standard);*/
	i2cCtrl_Set_SpeedMode(pI2c, I2_SPEED_STANDARD);  /* I2c_speed_standard*/
	
    /* use 7-bit addressing*/
    /*FI2cPs_setMasterAddressMode(dev, I2c_7bit_address);*/
    /*FI2cPs_setSlaveAddressMode(dev, I2c_7bit_address);*/
	i2cCtrl_Master_Set_AddrMode(pI2c, I2C_7BIT_ADDR);  /* I2c_7bit_address*/
	i2cCtrl_Slave_Set_AddrMode(pI2c, I2C_7BIT_ADDR);
	
    /* enable restart conditions*/
   /* FI2cPs_enableRestart(dev);*/
	i2cCtrl_Restart_Enable(pI2c);
	
    /* enable master FSM*/
    /*FI2cPs_enableMaster(dev);*/
    /*FI2cPs_disableSlave(dev);*/
	i2cCtrl_Master_Enable(pI2c);
	i2cCtrl_Slave_Disable(pI2c);
    
    /* Use the start byte protocol with the target address when*/
    /* initiating transfer.*/
    /*FI2cPs_setTxMode(dev, I2c_tx_target);*/
	i2cCtrl_Set_TxMode(pI2c, I2C_TX_TARGET);  /* I2c_tx_target*/
    
    /* set target address to the I2C slave address*/
    /*FI2cPs_setTargetAddress(dev, EEPROM_ADDR);*/
   /* FI2cPs_setTargetAddress(dev, i2c_dev_addr);*/
	i2cCtrl_Set_TargetAddr(pI2c, i2c_dev_addr);
        
    /*enable the dev I2C device*/
    /*FI2cPs_enable(dev); */
	i2cCtrl_Enable(pI2c); 
    
    return 0;
}

/* 
i2c SLCR loop: i2c0_tx  -> i2c1_rx
*/

void test_i2c_loop2(void)
{
	i2cCtrl_SLCR_Loop();
}


int test_i2c_slave(void)
{
	UINT8 i=0, reg = 0;

	/*FI2cPs_T *pI2c0_dev = &gI2c0_dev;	*/
	/*FIicPs_Instance_T *I2c1_Instance = &gI2c0_Instance;*/
	/*FIicPs_Param_T I2c0_Param;*/
	vxT_I2C* pI2c = g_pI2c0;
	
	int i2c_dev_addr_1 = 0x72 >> 1;  /* 0x39*/
	int i2c_dev_addr_2 = 0x7A >> 1;  /* 0x3d*/
	

	/*FI2c0Ps_DeviceInit(pI2c0_dev,pI2c1_Instance, &I2c0_Param);*/
	vxInit_I2c(I2C_CTRL_0);

	/**/
	i2cEeprom_init(I2C_CTRL_0, i2c_dev_addr_1);  

	/*FI2cPs_enableRestart(pI2c0_dev);*/
	i2cCtrl_Restart_Enable(pI2c);

	/*FI2cPs_write(pI2c0_dev, 0x2);   */
    i2cCtrl_Write_Fifo(pI2c, 0x2);
	
	/*FI2cPs_issueRead(pI2c0_dev);  */
    i2cCtrl_Xfer_READ(pI2c);
	
	/*while (FI2cPs_isRxFifoEmpty(pI2c0_dev) == true);   */
	while (i2cCtrl_isEmpty_RxFifo(pI2c) == 1);  
	
	/*reg = FI2cPs_read(pI2c0_dev);*/
	reg = i2cCtrl_Read_Fifo(pI2c);

	printf("reg:0x%X \n", reg);

/*	FI2cPs_write(pI2c0_dev,0x3);   
	FI2cPs_issueRead(pI2c0_dev);   
	while(FI2cPs_isRxFifoEmpty(pI2c0_dev) == true);   
	reg = FI2cPs_read(pI2c0_dev);

	printf("reg:0x%X \n", reg);
*/

	/*FI2cPs_masterAbort(pI2c0_dev);*/

	return 0;      
}



#endif



