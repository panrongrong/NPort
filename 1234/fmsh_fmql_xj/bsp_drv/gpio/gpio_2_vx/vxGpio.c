/* vxGpio.c - fmsh 7020/7045 pGpio driver */

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
01a, 20May26, jc  written.
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

#include "vxGpio.h"

/*
defines 
*/
#if 1

#undef VX_DBG
#undef VX_DBG2

#define GPIO_DBG

#ifdef GPIO_DBG

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
GPIO
*/
static vxT_GPIO_CTRL vxGpio_Ctrl = {0};

static vxT_GPIO vx_pGpio = {0};
vxT_GPIO * g_pGpio = (vxT_GPIO *)(&vx_pGpio.gpio_x);


#if 1

void gpioCtrl_Wr_CfgReg32(vxT_GPIO* pGpio, UINT32 offset, UINT32 value)
{
	UINT32 tmp32 = pGpio->pGpioCtrl->cfgBaseAddr;	
	int bank_off = 0;
	
	switch (pGpio->gpio_bank_x)
	{
	case GPIO_BANK_A:
		bank_off = pGpio->pGpioCtrl->offset_A;
		break;		
	case GPIO_BANK_B:
		bank_off = pGpio->pGpioCtrl->offset_B;
		break;		
	case GPIO_BANK_C:
		bank_off = pGpio->pGpioCtrl->offset_C;
		break;		
	case GPIO_BANK_D:
		bank_off = pGpio->pGpioCtrl->offset_D;
		break;		
	}
	tmp32 += bank_off;	
	
	FMQL_WRITE_32((tmp32 + offset), value);
	
	return;
}

UINT32 gpioCtrl_Rd_CfgReg32(vxT_GPIO* pGpio, UINT32 offset)
{
	UINT32 tmp32 = pGpio->pGpioCtrl->cfgBaseAddr;	
	int bank_off = 0;	
	
	switch (pGpio->gpio_bank_x)
	{
	case GPIO_BANK_A:
		bank_off = pGpio->pGpioCtrl->offset_A;
		break;		
	case GPIO_BANK_B:
		bank_off = pGpio->pGpioCtrl->offset_B;
		break;		
	case GPIO_BANK_C:
		bank_off = pGpio->pGpioCtrl->offset_C;
		break;		
	case GPIO_BANK_D:
		bank_off = pGpio->pGpioCtrl->offset_D;
		break;		
	}
	tmp32 += bank_off;	
	
	return FMQL_READ_32(tmp32 + offset);
}


/******************************************************************************
*
*
* This function write data to GPIO port
*
* @param    dev is gpio device handle.
* @param    data is the data of data write to GPIO port
*
* @return   0 if successful, otherwise 1.
*
* @note    None.
*
******************************************************************************/
UINT8 gpioCtrl_Write_Data(vxT_GPIO* pGpio, UINT32 data)
{
    gpioCtrl_Wr_CfgReg32(pGpio, GPIO_REG_SWPORT_DR, data);

    return 0;
}

/******************************************************************************
*
*
*  This function returns the data register value.
*
* @param    dev is gpio device handle.
*
* @return   return gpio prot data.
*
* @note    None.
*
******************************************************************************/
UINT32 gpioCtrl_Read_Data(vxT_GPIO* pGpio)
{
    UINT32 retval;
	
    retval = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_SWPORT_DR);

    return retval;
}

/******************************************************************************
*
*
*  This function returns the value of external port data register.
*
* @param    dev is gpio device handle.
*
* @return    data of extern PORT data
*
* @note    None.
*
******************************************************************************/
UINT32 gpioCtrl_Get_ExtPort(vxT_GPIO* pGpio)
{
    UINT32 retval;
	
    retval = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_EXT_PORT);

    return retval;
}

/******************************************************************************
*
*
*  This function sets the specified bits of a port data register to the
*  value specified.  Multiple bits can be specified in the function arguments,
*  using the bitwise 'OR' operator.  The allowable values that a port bit can
*  be set to are 0 and 1
*
* @param    dev is gpio device handle.
* @param    value is value to set(Gpio_low/Gpio_high).
* @param    bits -- port bits to change.
*
* @return   0 if successful, otherwise 1.
*
* @note    None.
*
******************************************************************************/
UINT8 gpioCtrl_Set_PortBit(vxT_GPIO* pGpio, int gpio_val, UINT32 port_bits)
{
    UINT32 data_in;
    UINT32 data_out;
    UINT32 retval = 0;

    data_in = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_SWPORT_DR);

    switch (gpio_val)
    {
        case GPIO_LOW:
            data_out = data_in & (~port_bits);
            break;
		
        case GPIO_HIGH:
            data_out = data_in | port_bits;
            break;
		
        default:
            retval = 1;
            break;
    }

    /* avoid bus write if possible*/
    if ((data_in != data_out) && (retval == 0))
    {
        gpioCtrl_Wr_CfgReg32(pGpio, GPIO_REG_SWPORT_DR, data_out);
    }

    return retval;
}

/******************************************************************************
*
*
*  This function returns the value of a port data register bit.  Only
*  one bit may be specified per invocation of this function.
*
* @param    dev is gpio device handle.
* @param    bit -- bit to return the value of
*
* @return
*  Gpio_high     -- if the specified port bit is set (0x1)
*  Gpio_low      -- if the specified port bit is clear (0x0)
*
* @note    None.
*
******************************************************************************/
int gpioCtrl_Get_PortBit_Chk(vxT_GPIO* pGpio, int bit)
{
    UINT32 reg;
    int retval = 0;

    reg = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_SWPORT_DR);

    retval = ((reg & (0x1 << bit)) >> bit);

    return retval;
}

/******************************************************************************
*
*
* This function set GpioX direction
*
* @param    dev is gpio device handle.
*           bitX = 0  --  input;
*			bitX = 1  --   output.
*
* @return   0 if successful, otherwise 1.
*
* @note    None.
*
******************************************************************************/
int gpioCtrl_Set_PortDirection(vxT_GPIO* pGpio, UINT32 dir_data)
{
    /*GPIO_OUT32P(data, portmap->swport_ddr);*/
	gpioCtrl_Wr_CfgReg32(pGpio, GPIO_REG_SWPORT_DDR, dir_data);

    return 0;
}

/******************************************************************************
*
*
* This function get GpioX direction
*
* @param    dev is gpio device handle.
*           bitX = 0  --  input; 
*			bitX = 1  --  output.
*
* @return   0 if successful, otherwise 1.
*
* @note    None.
*
******************************************************************************/
int gpioCtrl_Get_PortDirection(vxT_GPIO* pGpio)
{
    UINT32 retval;

    /*retval = GPIO_IN32P(portmap->swport_ddr);*/
    retval = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_SWPORT_DDR);

    return retval;
}

/******************************************************************************
*
*
*  This function sets the specified bits of a port data direction
*  register to the value specified.  Multiple bits of a single port can
*  be specified in the function arguments, using the bitwise 'OR'
*  operator.  The allowable values that a port data direction bit can
*  be set to are Gpio_input and Gpio_output.
*
* @param    dev is GpioX device handle.
* @param    bits -- port bits to change.
* @param    direction is value to set(Gpio_input/Gpio_output).
*
* @return   0 if successful, otherwise 1.
*
* @note    None.
*
******************************************************************************/
int gpioCtrl_Set_BitDirection(vxT_GPIO* pGpio, UINT32 bits, int direction)
{
    UINT32 data_in;
    UINT32 data_out;
    UINT32 retval = 0;

    /*data_in = GPIO_IN32P(portmap->swport_ddr);*/
    data_in = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_SWPORT_DDR);

    switch (direction)
    {
        case GPIO_DIR_IN:
            data_out = data_in & (~bits);
            break;
		
        case GPIO_DIR_OUT:
            data_out = data_in | bits;
            break;
		
        default:
            retval = 1;
            break;
    }

    /* avoid bus write if possible*/
    if ((data_in != data_out) && (retval == 0))
    {   
    	/*GPIO_OUT32P(data_out, portmap->swport_ddr);*/
    	gpioCtrl_Wr_CfgReg32(pGpio, GPIO_REG_SWPORT_DDR, data_out);
    }

    return retval;
}

/******************************************************************************
*
*
*  This function returns the value of a port data direction bit.  Only
*  one bit may be specified per invocation of this function.
*
* @param    dev is GpioX device handle.
* @param    bit -- port bit to check
*
* @return
*  Gpio_input          -- if the port bit is an input
*  Gpio_output         -- if the port bit is an output
*  Gpio_no_direction   -- if the specified port bit is not available
*
* @note    None.
*
******************************************************************************/
int gpioCtrl_Get_BitDirection(vxT_GPIO* pGpio, int bit)
{
    UINT32 reg;
    int retval;

    retval = -1;

    /* reg = GPIO_IN32P(portmap->swport_ddr);*/
	reg = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_SWPORT_DDR);	
    retval = ((reg & (0x1 << bit)) >> bit);

    return retval;
}

/******************************************************************************
*
*
* This function enables interrupts for the specified bit(s) of port
* Multiple bits can be specified in the function arguments, using the
* bitwise 'OR' operator.
*
* @param    dev is gpio device handle.
* @param    interrupts  -- interrupt bit(s) to enable
*
* @return   0 if successful, otherwise 1.
*
* @note    None.
*
******************************************************************************/
int gpioCtrl_Enable_Irq(vxT_GPIO* pGpio, UINT32 interrupts)
{
    UINT32 retval;
    UINT32 data_in;
    UINT32 data_out;

    retval = 0;
	
    /*data_in = GPIO_IN32P(portmap->inten);*/
    data_in = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_INT_EN);	
	
    data_out = data_in | interrupts;

    /* avoid bus write if possible*/
    if (data_in != data_out)
    {    
    	/*GPIO_OUT32P(data_out, portmap->inten);*/
		gpioCtrl_Wr_CfgReg32(pGpio, GPIO_REG_INT_EN, data_out);
    }

    return retval;
}

/******************************************************************************
*
*
*  This function disables interrupts for the specified bit(s) of port
*  .  Multiple bits can be specified in the function arguments, using
*  the bitwise 'OR' operator.
*
* @param    dev is gpio device handle.
* @param    interrupts  -- interrupt bit(s) to enable
*
* @return   0 if successful, otherwise 1.
*
* @note    None.
*
******************************************************************************/
int gpioCtrl_Disable_Irq(vxT_GPIO* pGpio, UINT32 interrupts)
{
    UINT32 retval;
    UINT32 data_in;
    UINT32 data_out;

    retval = 0;
	
    /*data_in = GPIO_IN32P(portmap->inten);	*/
    data_in = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_INT_EN);	
	
    data_out = data_in & ~interrupts;

    /* avoid bus write if possible*/
    if (data_in != data_out)
    {    
    	/*GPIO_OUT32P(data_out, portmap->inten);*/
		gpioCtrl_Wr_CfgReg32(pGpio, GPIO_REG_INT_EN, data_out);
    }

    return retval;
}

/******************************************************************************
*
*
*  This function returns whether interrupts are enabled for a
*  particular bit of port or not.  Only one bit may be specified per
*  invocation of this function.
*
* @param    dev is gpio device handle.
* @param    interrupt   -- bit of port to check
*
* @return
*  TRUE        -- if interrupts are enabled for the specified bit
*  FALSE       -- if interrupts are disabled for the specified bit
*
* @note    None.
*
******************************************************************************/
BOOL gpioCtrl_isEnable_Irq(vxT_GPIO* pGpio, int interrupt)
{
    BOOL retval;
    UINT32 reg;

    retval = FALSE;
	
    /*reg = GPIO_IN32P(portmap->inten);*/
    reg = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_INT_EN);
	
    if ((reg & (0x1 << interrupt)) != 0x0)
    {    
    	retval = TRUE;
    }

    return retval;
}

/******************************************************************************
*
*
*  This function returns the value of the interrupt enable register.
*
* @param    dev is gpio device handle.
*
* @return
*     The value of the interrupt enable register.
*
* @note    None.
*
******************************************************************************/
UINT32 gpioCtrl_Get_Irq(vxT_GPIO* pGpio)
{
    UINT32 retval;

    /*retval = GPIO_IN32P(portmap->inten);*/
    retval = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_INT_EN);

    return retval;
}

/******************************************************************************
*
*
*  This function masks interrupts for the specified bit(s) of port.
*  Multiple bits can be specified in the function arguments, using the
*  bitwise 'OR' operator.
*
* @param    dev is gpio device handle.
* @param    interrupts  -- interrupt bit(s) to mask
*
* @return   0 if successful, otherwise 1.
*
* @note    None.
*
******************************************************************************/
UINT8 gpioCtrl_Mask_Irq(vxT_GPIO* pGpio, UINT32 interrupts)
{
    UINT32 retval;
    UINT32 data_in;
    UINT32 data_out;

    retval = 0;
	
    /*data_in = GPIO_IN32P(portmap->intmask);    */
    data_in = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_INT_MASK);
	
    data_out = data_in | interrupts;

    /* avoid bus write if possible*/
    if (data_in != data_out)
    {    
    	/*GPIO_OUT32P(data_out, portmap->intmask);*/
    	gpioCtrl_Wr_CfgReg32(pGpio, GPIO_REG_INT_MASK, data_out);
    }

    return retval;
}

/******************************************************************************
*
*
*  This function unmasks interrupts for the specified bit(s) of port.
*  Multiple bits can be specified in the function arguments, using the
*  bitwise 'OR' operator.
*
* @param    dev is gpio device handle.
* @param    interrupts  -- interrupt bit(s) to unmask
*
* @return   0 if successful, otherwise 1.
*
* @note    None.
*
******************************************************************************/
UINT8 gpioCtrl_unMask_Irq(vxT_GPIO* pGpio, UINT32 interrupts)
{
    UINT32 retval;
    UINT32 data_in;
    UINT32 data_out;

    retval = 0;
	
    /*data_in = GPIO_IN32P(portmap->intmask);*/
    data_in = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_INT_MASK);
	
    data_out = data_in & (~interrupts);

    /* avoid bus write if possible*/
    if (data_in != data_out)
    {    
    	/*GPIO_OUT32P(data_out, portmap->intmask);*/
		gpioCtrl_Wr_CfgReg32(pGpio, GPIO_REG_INT_MASK, data_out);
    }

    return retval;
}

/******************************************************************************
*
*
*  This function returns whether interrupts are masked for a
*  particular bit of port or not.  Only one bit may be specified per
*  invocation of this function.
*
* @param    dev is gpio device handle.
* @param    interrupt is bit of port A to check
*
* @return
*  TRUE        -- if interrupts are masked for the specified bit
*  FALSE       -- if interrupts are not masked for the specified bit
*
* @note    None.
*
******************************************************************************/
BOOL gpioCtrl_isMask_Irq(vxT_GPIO* pGpio, int interrupt)
{
    BOOL retval;
    UINT32 reg;

    retval = FALSE;
	
    /* reg = GPIO_IN32P(portmap->intmask);	*/
    reg = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_INT_MASK);
	
    if ((reg & (0x1 << interrupt)) != 0x0)
    {    
    	retval = TRUE;
    }

    return retval;
}

/******************************************************************************
*
*
*  This function returns the value of the interrupt mask register.
*
* @param    dev is gpio device handle.
*
* @return
*  The interrupt mask register value.
*
* @note    None.
*
******************************************************************************/
UINT32 gpioCtrl_Get_IrqMask(vxT_GPIO* pGpio)
{
    UINT32 retval;

    /*retval = GPIO_IN32P(portmap->intmask);*/
    retval = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_INT_MASK);

    return retval;
}

/******************************************************************************
*
*
*  This function sets the interrupt type for the specified bit(s) of
*  port.  There are two types of interrupts available,
*  level-sensitive and edge-sensitive.  The polarity of these interrupt
*  types is set using dw_gpio_setIrqPolarity().  Multiple bits can be
*  specified in the function arguments, using the bitwise 'OR'
*  operator.
*
* @param    dev is gpio device handle.
* @param    type -- interrupt type to set
* @param    interrupts  -- interrupt bit(s) to set
*
* @return   0 if successful, otherwise 1.
*
* @note    None.
*
******************************************************************************/
UINT32 gpioCtrl_Set_IrqType(vxT_GPIO* pGpio, int type, UINT32 interrupts)
{
    int retval;
    UINT32 data_in;
    UINT32 data_out;

    retval = 0;
    /*data_in = GPIO_IN32P(portmap->inttype_level);*/
    data_in = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_INT_TYPELEVEL);

    switch (type)
    {
    case GPIO_IRQ_TYPE_LEVEL:
        data_out = data_in & (~interrupts);
        break;
    case GPIO_IRQ_TYPE_EDGE:
        data_out = data_in | interrupts;
        break;
    default:
        retval = 1;
        break;
    }

    /* avoid bus write if possible*/
    if ((data_in != data_out) && (retval == 0))
    {   
    	/*GPIO_OUT32P(data_out, portmap->inttype_level);*/
		gpioCtrl_Wr_CfgReg32(pGpio, GPIO_REG_INT_TYPELEVEL, data_out);
    }

    return retval;
}

/******************************************************************************
*
*
*  This function returns the interrupt type for a specified but of
*  port .  Only one bit may be specified per invocation of this
*  function.
*
* @param    dev is gpio device handle.
* @param    interrupt  -- bit of port to check
*
* @return
*  Gpio_level_sensitive    -- if the interrupt bit is level-sensitive
*  Gpio_edge_sensitive     -- if the interrupt bit is edge-sensitive
*  Gpio_no_type            -- if interrupts are not supported or the
*                             specified bit is not available
*
* @note    None.
*
******************************************************************************/
int gpioCtrl_Get_IrqType(vxT_GPIO* pGpio, int interrupt)
{
    int retval;
    UINT32 reg;

    /*reg = GPIO_IN32P(portmap->inttype_level);*/
    reg = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_INT_TYPELEVEL);
	
    retval = ((reg & (0x1 << interrupt)) >> interrupt);

    return retval;
}

/******************************************************************************
*
*
*  This function sets the interrupt polarity for the specified bit(s)
*  of port.  The polarity can be either active-low or active-high.
*  For edge-sensitive interrupts, active-low corresponds to a
*  falling-edge interrupt while active-high corresponds to a
*  rising-edge interrupt.  The interrupt type is set using
*  gpioCtrl_setIrqType().  Multiple bits can be specified in the
*  function arguments, using the bitwise 'OR' operator.
*
* @param  dev is gpio device handle.
* @param  polarity    -- interrupt polarity to set
* @param  interrupts  -- interrupt bit(s) to set
*
* @return   0 if successful, otherwise 1.
*
* @note    None.
*
******************************************************************************/
UINT32 gpioCtrl_Set_IrqPolar(vxT_GPIO* pGpio, int polarity, UINT32 interrupts)
{
    UINT32 retval;
    UINT32 data_in;
    UINT32 data_out;

    retval = 0;
	
    /*data_in = GPIO_IN32P(portmap->int_polarity);*/
    data_in = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_INT_POLAR);

    switch (polarity)
    {
    case GPIO_ACT_LOW_FALL_EDGE:
        data_out = data_in & (~interrupts);
        break;
    case GPIO_ACT_HIGH_RISE_EDGE:
        data_out = data_in | interrupts;
        break;
    default:
        retval = 1;
        break;
    }

    /* avoid bus write if possible*/
    if ((data_in != data_out) && (retval == 0))
    {   
    	/*GPIO_OUT32P(data_out, portmap->int_polarity);*/
    	gpioCtrl_Wr_CfgReg32(pGpio, GPIO_REG_INT_POLAR, data_out);
    }

    return retval;
}

/******************************************************************************
*
*
*  This function returns the interrupt polarity of a specified bit of
*  . Only one bit may be specified per invocation of this function.
*
* @param  dev is gpio device handle.
* @param  polarity    -- interrupt polarity to set
* @param  interrupts  -- bit of port to check
*
* @return
* Gpio_active_low     -- if interrupt bit is active-low/falling-edge
* Gpio_active_high    -- if interrupt bit is active-high/rising-edge
* Gpio_no_polarity    -- if interrupts are not supported

* @note    None.
*
******************************************************************************/
 int gpioCtrl_Get_IrqPolar(vxT_GPIO* pGpio, int interrupt)
{
    int retval;
    UINT32 reg;

    /*reg = GPIO_IN32P(portmap->int_polarity);	*/
    reg = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_INT_POLAR);
	
    retval = ((reg & (0x1 << interrupt)) >> interrupt);

    return retval;
}

/******************************************************************************
*
*
*  This function returns the value of the interrupt status register.
*
* @param    dev is gpio device handle.
*
* @return   The interrupt status register value.
*
* @note    None.
*
******************************************************************************/
UINT32 gpioCtrl_Get_ActiveIrq(vxT_GPIO* pGpio)
{
    UINT32 retval;

    /* retval = GPIO_IN32P(portmap->intstatus);*/
    retval = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_INT_STATUS);

    return retval;
}

/******************************************************************************
*
*
*  This function is used to clear edge-sensitive interrupts of port.
*  Multiple bits can be specified in the function arguments, using the
*  bitwise 'OR' operator.
*
* @param    dev is gpio device handle.
* @param    interrupts is interrupt bit(s) to clear
*
* @return   0 if successful, otherwise 1.
*
* @note    None.
*
******************************************************************************/
UINT32 gpioCtrl_Clear_Irq(vxT_GPIO* pGpio, UINT32 interrupts)
{
    UINT32 retval;

    retval = 0;
    /*GPIO_OUT32P(interrupts, portmap->port_eoi);*/
	gpioCtrl_Wr_CfgReg32(pGpio, GPIO_REG_PORT_EOI, interrupts);

    return retval;
}

/******************************************************************************
*
*
*  This function enables debounce logic for the specified bit(s) of
*  port.  When enabled, a signal must be valid for two periods of the
*  external debounce clock before it is internally processed.
*  Multiple bits can be specified in the function arguments,
*  using the bitwise 'OR' operator.
*
* @param    dev is gpio device handle.
* @param    interrupts  -- interrupt bit(s) to set
*
* @return   0 if successful, otherwise 1.
*
* @note    None.
*
******************************************************************************/
UINT32 gpioCtrl_Enable_Debounce(vxT_GPIO* pGpio, UINT32 interrupts)
{
    UINT32 retval;
    UINT32 data_in;
    UINT32 data_out;

    retval = 0;
	
    /*data_in = GPIO_IN32P(portmap->debounce);	*/
    retval = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_DEBOUNCE);
	
    data_out = data_in | interrupts;
	
    /* avoid bus write if possible*/
    if (data_in != data_out)
    {    
    	/*GPIO_OUT32P(interrupts, portmap->debounce);*/
    	gpioCtrl_Wr_CfgReg32(pGpio, GPIO_REG_DEBOUNCE, data_out);
    }

    return retval;
}

/******************************************************************************
*
*
*  This function disables debounce logic for the specified bit(s) of
*  port A.  Multiple bits can be specified in the function arguments,
*  using the bitwise 'OR' operator.
*
* @param    dev is gpio device handle.
* @param    interrupts  -- interrupt bit(s) to set
*
* @return   0 if successful, otherwise 1.
*
* @note    None.
*
******************************************************************************/
UINT32 gpioCtrl_Disable_Debounce(vxT_GPIO* pGpio, UINT32 bits)
{
    UINT32 retval;
    UINT32 data_in;
    UINT32 data_out;

    retval = 0;
	
    /*data_in = GPIO_IN32P(portmap->debounce);	*/
    data_in = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_DEBOUNCE);
	
    data_out = data_in & (~bits);
	
    /* avoid bus write if possible*/
    if (data_in != data_out)
    {    
    	/*GPIO_OUT32P(data_out, portmap->debounce);*/
		gpioCtrl_Wr_CfgReg32(pGpio, GPIO_REG_DEBOUNCE, data_out);
    }

    return retval;
}

/******************************************************************************
*
*
*  This function returns whether debounce is enabled for the specified
*  it or port or not.  Only one bit may be specified per invocation
*  of this function.
*
* @param    dev is gpio device handle.
* @param    interrupt   -- bit of port to check
*
* @return
*  TRUE        -- if debounce is enabled for interrupt bit
*  FALSE       -- if debounce is disabled for interrupt bit
*
* @note    None.
*
******************************************************************************/
BOOL gpioCtrl_isEnable_Debounce(vxT_GPIO* pGpio, int bits)
{
    BOOL retval;
    UINT32 reg;

    retval = FALSE;
	
    /*reg = GPIO_IN32P(portmap->debounce);	*/
    reg = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_DEBOUNCE);
	
    if ((reg & (0x1 << bits)) != 0x0)
    {    
    	retval = TRUE;
    }

    return retval;
}

/******************************************************************************
*
*
*  This function enables interrupt synchronization.  When enabled, all
*  level-sensitive interrupts are synchronized to pclk.
*
* @param    dev is gpio device handle.
*
* @return   0 if successful, otherwise 1.
*
* @note    None.
*
*****************************************************************************/
UINT32 gpioCtrl_Enable_Sync(vxT_GPIO* pGpio)
{
    UINT32 retval;

    retval = 0;
	
    /* FMSH_BIT_SET(reg, GPIO_LS_SYNC_SYNCLEVEL, 0x1);	*/
    /* GPIO_OUT32P(reg, portmap->ls_sync); */
	gpioCtrl_Wr_CfgReg32(pGpio, GPIO_REG_LS_SYNC, 0x01);

    return retval;
}

/******************************************************************************
*
*
*  This function disables synchronization for level-sensitive
*  interrupts.
*
* @param    dev is gpio device handle.
*
* @return   0 if successful, otherwise 1.
*
* @note    None.
*
*****************************************************************************/
UINT32 gpioCtrl_Disable_Sync(vxT_GPIO* pGpio)
{
    UINT32 retval;

    retval = 0;
    /*FMSH_BIT_SET(reg, GPIO_LS_SYNC_SYNCLEVEL, 0x0);*/
    /*GPIO_OUT32P(reg, portmap->ls_sync);*/
	gpioCtrl_Wr_CfgReg32(pGpio, GPIO_REG_LS_SYNC, 0x00);

    return retval;
}

/******************************************************************************
*
*
*  This function returns whether synchronization is enabled for
*  level-sensitive interrupts or not.
*
* @param    dev is gpio device handle.
*
* @return
*  TRUE        -- if interrupt synchronization is enabled
*  FALSE       -- if interrupt synchronization is not enabled
*
* @note    None.
*
*****************************************************************************/
BOOL gpioCtrl_isSync(vxT_GPIO* pGpio)
{
    BOOL retval = FALSE;
	UINT32 reg = 0;

	reg = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_LS_SYNC);

    /*if (GPIO_IN32P(portmap->ls_sync) != 0x0)*/
	if (reg != 0x0)
    {    
    	retval = TRUE;
	}

    return retval;
}

/******************************************************************************
*
*
* This function enable both edge interrupt, when this function is called
* , gpio level and polarity is no sense.
*
* @param    dev is gpio device handle.
* @param    interrupts  -- interrupt bit(s) to set
*
* @return   0 if successful, otherwise 1.
*
* @note    None.
*
*****************************************************************************/
UINT8 gpioCtrl_Enable_BothEdgeIrq(vxT_GPIO* pGpio, UINT32 interrupts)
{
    UINT32 retval;
    UINT32 data_in;
    UINT32 data_out;

    retval = 0;
	
    /*data_in = GPIO_IN32P(portmap->int_bothedge);    */
	data_in = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_INT_BOTHEDGE);
	
    data_out = data_in | interrupts;

    /* avoid bus write if possible*/
    if (data_in != data_out)
    {    
    	/*GPIO_OUT32P(data_out, portmap->int_bothedge);*/
		gpioCtrl_Wr_CfgReg32(pGpio, GPIO_REG_INT_BOTHEDGE, data_out);
    }

    return retval;
}

/******************************************************************************
*
*
* This function disable both edge interrupt, when this function is called
* , interrupt occurs depending on gpio level and polarity .
*
* @param    dev is gpio device handle.
* @param    interrupts  -- interrupt bit(s) to set
*
* @return   0 if successful, otherwise 1.
*
* @note    None.
*
*****************************************************************************/
UINT8 gpioCtrl_Disable_BothEdgeIrq(vxT_GPIO* pGpio, UINT32 interrupts)
{
    UINT32 retval;
    UINT32 data_in;
    UINT32 data_out;

    retval = 0;
	
    /*data_in = GPIO_IN32P(portmap->int_bothedge);*/
	data_in = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_INT_BOTHEDGE);
	
    data_out = data_in & (~interrupts);

    /* avoid bus write if possible*/
    if (data_in != data_out)
    {    
    	/* GPIO_OUT32P(data_out, portmap->int_bothedge);*/
    	gpioCtrl_Wr_CfgReg32(pGpio, GPIO_REG_INT_BOTHEDGE, data_out);
    }

    return retval;
}

/******************************************************************************
*
*
* This function returns value of both edge interrupt reg.
*
* @param    dev is gpio device handle.
*
* @return   value of both edge interrupt reg.
*
* @note    None.
*
*****************************************************************************/
UINT32 gpioCtrl_Get_BothEdgeIrq(vxT_GPIO* pGpio)
{
    UINT32 data_in;
	
    /*data_in = GPIO_IN32P(portmap->int_bothedge);*/
	data_in = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_INT_BOTHEDGE);

    return data_in;
}

/******************************************************************************
*
*
* This function returns gpio version ID code.
*
* @param    dev is gpio device handle.
*
* @return  version ID code.
*
* @note    None.
*
*****************************************************************************/
UINT32 gpioCtrl_Get_VerIdCode(vxT_GPIO* pGpio)
{
    UINT32 data;

    /*data = GPIO_IN32P(portmap->ver_id_code);*/
    data = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_VER_IDCODE);

    return data;
}

/******************************************************************************
*
*
* This function returns gpio ID code.
*
* @param    dev is gpio device handle.
*
* @return   ID code.
*
* @note    None.
*
*****************************************************************************/
UINT32 gpioCtrl_Get_IdCode(vxT_GPIO* pGpio)
{
    UINT32 data = 0;

    /*data = GPIO_IN32P(portmap->id_code);*/
    data = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_ID_CODE);

    return data;
}

/*******************************************************************************
*
* sysGpioSetIrqTrigger - set gpio interrupt trigger mode
*
* This routine sets gpio interrupt trigger mode
*
* RETURNS: N/A
*
* ERRNO: N/A
*
*/

int gpioCtrl_Set_IrqTrigger    (    vxT_GPIO* pGpio,     UINT32 bits, UINT32 trigger)
{
    UINT32      intEdgeLevel = 0;
    UINT32      intRiseFall = 0;

    intEdgeLevel = gpioCtrl_Rd_CfgReg32(pGpio, INTTYPELEVEL_OFFSET);
    intRiseFall  = gpioCtrl_Rd_CfgReg32(pGpio, INTPOLARITY_OFFSET);

    switch (trigger)
    {
    case GPIO_IRQ_LOW_LEVEL_SENSITIVE :
    case GPIO_IRQ_SENSITIVE_NONE :
        intEdgeLevel &= ~bits;
        intRiseFall  &= ~bits;
        break;
		
    case GPIO_IRQ_HIGH_LEVEL_SENSITIVE :
        intEdgeLevel &= ~bits;
        intRiseFall  |= bits;
        break;
		
    case GPIO_IRQ_RISING_EDGE_SENSITIVE :
        intEdgeLevel |= bits;
        intRiseFall  |= bits;
        break;
		
    case GPIO_IRQ_FALLING_EDGE_SENSITIVE :
        intEdgeLevel |= bits;
        intRiseFall  &= ~bits;
        break;
		
    default:
        return;
    }

    gpioCtrl_Wr_CfgReg32(pGpio, INTTYPELEVEL_OFFSET, intEdgeLevel);
    gpioCtrl_Wr_CfgReg32(pGpio, INTPOLARITY_OFFSET, intRiseFall);

	return 0;
}

#endif

#if 1

/*
*/
void gpioSlcr_Mio_Init(void)
{
	slcr_write(0x32C, 0x00C1);
	return;
}


#endif

#if 1

int vxInit_Gpio(void)
{
	vxT_GPIO * pGpio = NULL;
	vxT_GPIO_CTRL * pGpioCtrl = NULL;
	
	int ret = 0;	
	UINT32 tmp32 = 0;

	/*
	init the pGpio structure
	*/
	pGpio = g_pGpio;
	
	if (pGpio->init_flag == 1)
	{
		return 0;  /* already init*/
	}
	
	/**/
	/* spi_ctrl select*/
	/**/
	bzero(pGpio, sizeof(vxT_GPIO));	
	
	pGpio->pGpioCtrl = (vxT_GPIO_CTRL *)(&vxGpio_Ctrl.ctrl_x);	
	bzero(pGpio->pGpioCtrl, sizeof(vxT_GPIO_CTRL));

	pGpio->pGpioCtrl->devId = 0;
	pGpio->pGpioCtrl->cfgBaseAddr = VX_GPIO_CFG_BASE;	
	
	pGpio->pGpioCtrl->offset_A = VX_GPIO_OFFSET_A;
	pGpio->pGpioCtrl->offset_B = VX_GPIO_OFFSET_B;
	pGpio->pGpioCtrl->offset_C = VX_GPIO_OFFSET_C;
	pGpio->pGpioCtrl->offset_D = VX_GPIO_OFFSET_D;
		
	/* gpio ctrl_0;*/
	pGpio->gpio_x = 0;             
	pGpio->pGpioCtrl->ctrl_x = 0;  
	
	pGpioCtrl->devIP_ver  = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_COMP_VER);
	pGpioCtrl->devIP_type = gpioCtrl_Rd_CfgReg32(pGpio, GPIO_REG_COMP_TYPE);

	/* default*/
	pGpio->gpio_bank_x = GPIO_BANK_A;	
	pGpio->gpio_pin_x = 0;	
	
	pGpio->init_flag = 1;  /* 1-init_ok, 0-no_init1*/
	return;
}

int gpio_set_output(int pin_x)
{	
	vxT_GPIO * pGpio = g_pGpio;
	int bit = 0, bits = 0;
	
	/* init*/
	vxInit_Gpio();

	pGpio->gpio_pin_x = pin_x;
	pGpio->gpio_bank_x = GPIO_PIN_TO_BANK(pin_x);

	bit = GPIO_PIN_TO_BIT(pin_x);
	bits = 1 << bit;

	gpioCtrl_Set_BitDirection(pGpio, bits, GPIO_DIR_OUT);
	return 0;
}

int gpio_set_input(int pin_x)
{
	vxT_GPIO * pGpio = g_pGpio;
	int bit = 0, bits = 0;
	
	/* init*/
	vxInit_Gpio();

	pGpio->gpio_pin_x = pin_x;
	pGpio->gpio_bank_x = GPIO_PIN_TO_BANK(pin_x);

	bit = GPIO_PIN_TO_BIT(pin_x);
	bits = 1 << bit;
	
	gpioCtrl_Set_BitDirection(pGpio, bits, GPIO_DIR_IN);
	return 0;
}


int gpio_set_output_val(int pin_x, int val)
{	
	vxT_GPIO * pGpio = g_pGpio;
	int bit = 0, bits = 0;
	
	/* init*/
	vxInit_Gpio();

	pGpio->gpio_pin_x = pin_x;
	pGpio->gpio_bank_x = GPIO_PIN_TO_BANK(pin_x);
	
	bit = GPIO_PIN_TO_BIT(pin_x);
	bits = 1 << bit;
	
	gpioCtrl_Set_PortBit(pGpio, val, bits);
	return 0;
}

void gpio_set_HIGH(int pin_x)
{	
	gpio_set_output_val(pin_x, GPIO_HIGH);
	return;
}

void gpio_set_LOW(int pin_x)
{	
	gpio_set_output_val(pin_x, GPIO_LOW);
	return;
}

int gpio_get_input_val(int pin_x)
{	
	vxT_GPIO * pGpio = g_pGpio;
	int bit = 0, bits = 0;
	int val = 0;
	
	/* init*/
	vxInit_Gpio();

	pGpio->gpio_pin_x = pin_x;
	pGpio->gpio_bank_x = GPIO_PIN_TO_BANK(pin_x);
	
	bit = GPIO_PIN_TO_BIT(pin_x);
	bits = 1 << bit;
	
	val = gpioCtrl_Get_ExtPort(pGpio);
	
	if ((val & (1 << bit)) == bits)
	{
		return GPIO_HIGH;
	}
	else
	{
		return GPIO_LOW;
	}
}


/*
bankA: 0   ~ 31  (32)
bankB: 32  ~ 53  (22)
bankC: 54  ~ 85  (32)
bankD: 86  ~ 117 (32)
*/
int gpio_get_pins_by_bank(int bank_x)
{
	int pins = 0;
	
	if (bank_x == GPIO_BANK_B)
	{
		pins = 22;
	}
	else
	{
		pins = 32;
	}

	return pins;
}

#endif

/**/
/* Demo_led test*/
/**/
#if 1

#if 1  /* new demo_board 201907 */
#define FMQL_DEMO_GPIO_LED1           88 /* D11 */
#define FMQL_DEMO_GPIO_LED2           92 /* D15 */
#define FMQL_DEMO_GPIO_LED3           86 /* D9 */
#define FMQL_DEMO_GPIO_LED4           89 /* D12 */

#else /* old demo board 201905 */

#define FMQL_DEMO_GPIO_LED1           54 /* D11 */
#define FMQL_DEMO_GPIO_LED2           55 /* D15 */
#define FMQL_DEMO_GPIO_LED3           56 /* D9 */
#define FMQL_DEMO_GPIO_LED4           57 /* D12 */
#endif

void test_gpio_led(void)
{
	int i = 0;
	
    gpio_set_output (FMQL_DEMO_GPIO_LED1);
    gpio_set_output (FMQL_DEMO_GPIO_LED2);
    gpio_set_output (FMQL_DEMO_GPIO_LED3);
    gpio_set_output (FMQL_DEMO_GPIO_LED4);

    printf ("Turn on&off Demo_leds for 5 times! \n");
    taskDelay (20);

    for (i = 0; i < 5; i++)
    {
        gpio_set_LOW (FMQL_DEMO_GPIO_LED1);
        taskDelay (40);
        gpio_set_LOW (FMQL_DEMO_GPIO_LED2);
        taskDelay (40);
        gpio_set_LOW (FMQL_DEMO_GPIO_LED3);
        taskDelay (40);
        gpio_set_LOW (FMQL_DEMO_GPIO_LED4);
        taskDelay (40);

        gpio_set_HIGH (FMQL_DEMO_GPIO_LED1);
        taskDelay (40);
        gpio_set_HIGH (FMQL_DEMO_GPIO_LED2);
        taskDelay (40);
        gpio_set_HIGH (FMQL_DEMO_GPIO_LED3);
        taskDelay (40);
        gpio_set_HIGH (FMQL_DEMO_GPIO_LED4);
        taskDelay (40);
    }
	
    printf ("test_gpio_led ok and finished! \n");	
	return;
}


#define PHY_88E1111_RESET_MIO_PIN     (46)   /* ref_613*/

void test_phy_reset(void)
{
	gpio_set_output (PHY_88E1111_RESET_MIO_PIN);       
	
	gpio_set_HIGH (PHY_88E1111_RESET_MIO_PIN);
	
	gpio_set_LOW (PHY_88E1111_RESET_MIO_PIN);
    taskDelay (1);
    gpio_set_HIGH (PHY_88E1111_RESET_MIO_PIN);
    
    return;
}

#endif


/**/
/* Demo_button test*/
/**/
#if 1
#define FMQL_DEMO_GPIO_S_1             54
#define FMQL_DEMO_GPIO_S_2             55
#define FMQL_DEMO_GPIO_S_3             56
#define FMQL_DEMO_GPIO_S_4             57
#define FMQL_DEMO_GPIO_S_5             58

/* demo_board: pin -> key */
int gpio_get_key_by_pin(int pin_x)
{
	int key_x = 0;

	switch (pin_x)
	{
	case FMQL_DEMO_GPIO_S_1:
		key_x = 1;
		break;
	case FMQL_DEMO_GPIO_S_2:
		key_x = 2;
		break;
	case FMQL_DEMO_GPIO_S_3:
		key_x = 3;
		break;
	case FMQL_DEMO_GPIO_S_4:
		key_x = 4;
		break;
	case FMQL_DEMO_GPIO_S_5:
		key_x = 5;
		break;
	}

	return key_x;
}

void gpio_irq_init(int pin_x)
{
	vxT_GPIO * pGpio = g_pGpio;

	/* init*/
	vxInit_Gpio();
	
	pGpio->gpio_pin_x = pin_x;
	pGpio->gpio_bank_x = GPIO_PIN_TO_BANK(pin_x);
	
	/* init registers */
	gpioCtrl_Wr_CfgReg32 (pGpio, INTEN_OFFSET, 0);
	
	gpioCtrl_Wr_CfgReg32 (pGpio, INTMASK_OFFSET, GPIO_INTERRUPT_MASK);
	
	gpioCtrl_Wr_CfgReg32 (pGpio, INTTYPELEVEL_OFFSET, 0);
	
	gpioCtrl_Wr_CfgReg32 (pGpio, INTPOLARITY_OFFSET, 0);
	
	gpioCtrl_Wr_CfgReg32 (pGpio, INTSTATUS_OFFSET, 0);
	
	gpioCtrl_Wr_CfgReg32 (pGpio, DEBOUNCE_OFFSET, 0);

	return;
}

void gpio_poll_init(int pin_x)
{
	vxT_GPIO * pGpio = g_pGpio;

	/* init*/
	vxInit_Gpio();
	
	pGpio->gpio_pin_x = pin_x;
	pGpio->gpio_bank_x = GPIO_PIN_TO_BANK(pin_x);
	
	/* init registers */
	gpioCtrl_Wr_CfgReg32 (pGpio, INTEN_OFFSET, 0);
	
	gpioCtrl_Wr_CfgReg32 (pGpio, INTMASK_OFFSET, GPIO_INTERRUPT_MASK);
	
	gpioCtrl_Wr_CfgReg32 (pGpio, INTTYPELEVEL_OFFSET, 0);
	
	gpioCtrl_Wr_CfgReg32 (pGpio, INTPOLARITY_OFFSET, 0);
	
	gpioCtrl_Wr_CfgReg32 (pGpio, INTSTATUS_OFFSET, 0);
	
	gpioCtrl_Wr_CfgReg32 (pGpio, DEBOUNCE_OFFSET, 0);

	return;
}

STATUS gpio_Irq_Enable(UINT32         pin_x, int trigger)
{
	vxT_GPIO * pGpio = g_pGpio;
	
	UINT32 bit = 0, bits = 0;
    UINT32 tmp = 0;

	/* init*/
	vxInit_Gpio();
	
	pGpio->gpio_pin_x = pin_x;
	pGpio->gpio_bank_x = GPIO_PIN_TO_BANK(pin_x);

    bit = GPIO_PIN_TO_BIT(pin_x);
	bits = 1 << bit;	

    /* 
    sysGpioIntConnect() should be called first 
	*/
    gpioCtrl_Set_IrqTrigger(pGpio, bits, trigger);
	

    tmp = gpioCtrl_Rd_CfgReg32 (pGpio, DEBOUNCE_OFFSET);
    tmp |= bits;
    gpioCtrl_Wr_CfgReg32 (pGpio, DEBOUNCE_OFFSET, tmp);
	
    tmp = gpioCtrl_Rd_CfgReg32 (pGpio, INTMASK_OFFSET);
    tmp &= ~bits;
    gpioCtrl_Wr_CfgReg32 (pGpio, INTMASK_OFFSET, tmp);
	
    tmp = gpioCtrl_Rd_CfgReg32 (pGpio, INTEN_OFFSET);
    tmp |= bits;
    gpioCtrl_Wr_CfgReg32 (pGpio, INTEN_OFFSET, tmp);

    return OK;
}

STATUS gpio_Poll_Enable(UINT32         pin_x, int trigger)
{
	vxT_GPIO * pGpio = g_pGpio;
	
	UINT32 bit = 0, bits = 0;
    UINT32 tmp = 0;

	/* init*/
	vxInit_Gpio();
	
	pGpio->gpio_pin_x = pin_x;
	pGpio->gpio_bank_x = GPIO_PIN_TO_BANK(pin_x);

    bit = GPIO_PIN_TO_BIT(pin_x);
	bits = 1 << bit;	

    /* 
    sysGpioIntConnect() should be called first 
	*/
    /*gpioCtrl_Set_IrqTrigger(pGpio, bits, trigger);	*/

    tmp = gpioCtrl_Rd_CfgReg32 (pGpio, DEBOUNCE_OFFSET);
    tmp |= bits;
    gpioCtrl_Wr_CfgReg32 (pGpio, DEBOUNCE_OFFSET, tmp);
	
/*	
    tmp = gpioCtrl_Rd_CfgReg32 (pGpio, INTMASK_OFFSET);
    tmp &= ~bits;
    gpioCtrl_Wr_CfgReg32 (pGpio, INTMASK_OFFSET, tmp);
	
    tmp = gpioCtrl_Rd_CfgReg32 (pGpio, INTEN_OFFSET);
    tmp |= bits;
    gpioCtrl_Wr_CfgReg32 (pGpio, INTEN_OFFSET, tmp);
*/
    return OK;
}


int gpio_irq_attach[4] = {0};  /* a,b,c,d: 4 irq_vec*/

void gpioIsr_callbk(int arg)
{
	vxT_GPIO* pGpio = g_pGpio;
    
    int bank_x, pin_x, key_x;
    UINT32      tmp;
    UINT32      i;
    UINT32      mask = 0;

    bank_x = (int)arg;	
	pGpio->gpio_bank_x = bank_x;

    tmp = gpioCtrl_Rd_CfgReg32 (pGpio, INTSTATUS_OFFSET);
    if (tmp == 0)
    {
        return;
    }

    /* save current mask status */
    mask = gpioCtrl_Rd_CfgReg32 (pGpio, INTMASK_OFFSET);

    /* mask all gpio pin in this bank */
    gpioCtrl_Wr_CfgReg32 (pGpio, INTMASK_OFFSET, GPIO_INTERRUPT_MASK);

    for (i=0; i<gpio_get_pins_by_bank(bank_x); i++)
    {
        if ((tmp & (1 << i)) != 0)
        {
			gpioCtrl_Wr_CfgReg32 (pGpio, EOI_OFFSET, (1 << i));
			
			pin_x = GPIO_BANK_BIT_TO_PIN(pGpio->gpio_bank_x, i);
			break;
       }
   }

    /* restore interrupt mask */
    gpioCtrl_Wr_CfgReg32 (pGpio, INTMASK_OFFSET, mask);

	key_x = gpio_get_key_by_pin(pin_x);

	logMsg("Button Pin:%d->(S_%d) Pushed \n", pin_x, key_x, 3, 4, 5, 6);
	return;	
}

void gpio_attach_irq(int pin_x)
{
	vxT_GPIO * pGpio = g_pGpio;
	int irq_no = 0;
	int ret = 0;

	/* init*/
	vxInit_Gpio();
	
	pGpio->gpio_pin_x = pin_x;
	pGpio->gpio_bank_x = GPIO_PIN_TO_BANK(pin_x);

	if (gpio_irq_attach[pGpio->gpio_bank_x] == 1)
	{
		return;
	}
	
	irq_no = GPIO_PIN_TO_IRQ_NO(pin_x);
    ret = intConnect (INUM_TO_IVEC(irq_no),  gpioIsr_callbk, (int)pGpio->gpio_bank_x);
    ret = intEnable (irq_no);
	
	gpio_irq_attach[pGpio->gpio_bank_x] = 1;
	return;
}


/*
 irq mode
*/
void test_gpio_button(void)
{
	int ret = 0;
	int irq_no = 0;

	/**/
	/* init irq*/
	/**/
	gpio_irq_init(FMQL_DEMO_GPIO_S_1);
	gpio_irq_init(FMQL_DEMO_GPIO_S_2);
	gpio_irq_init(FMQL_DEMO_GPIO_S_3);
	gpio_irq_init(FMQL_DEMO_GPIO_S_4);
	gpio_irq_init(FMQL_DEMO_GPIO_S_5);
	
	/**/
	/* init input*/
	/**/
    gpio_set_input (FMQL_DEMO_GPIO_S_1);
    gpio_set_input (FMQL_DEMO_GPIO_S_2);
    gpio_set_input (FMQL_DEMO_GPIO_S_3);
    gpio_set_input (FMQL_DEMO_GPIO_S_4);
    gpio_set_input (FMQL_DEMO_GPIO_S_5);

	/**/
	/* connect irq*/
	/**/
	gpio_attach_irq(FMQL_DEMO_GPIO_S_1);
	gpio_attach_irq(FMQL_DEMO_GPIO_S_2);
	gpio_attach_irq(FMQL_DEMO_GPIO_S_3);
	gpio_attach_irq(FMQL_DEMO_GPIO_S_4);
	gpio_attach_irq(FMQL_DEMO_GPIO_S_5);
	
	/**/
	/* enable gpio irq*/
	/**/
	gpio_Irq_Enable(FMQL_DEMO_GPIO_S_1, GPIO_IRQ_FALLING_EDGE_SENSITIVE);
	gpio_Irq_Enable(FMQL_DEMO_GPIO_S_2, GPIO_IRQ_FALLING_EDGE_SENSITIVE);
	gpio_Irq_Enable(FMQL_DEMO_GPIO_S_3, GPIO_IRQ_FALLING_EDGE_SENSITIVE);
	gpio_Irq_Enable(FMQL_DEMO_GPIO_S_4, GPIO_IRQ_FALLING_EDGE_SENSITIVE);
	gpio_Irq_Enable(FMQL_DEMO_GPIO_S_5, GPIO_IRQ_FALLING_EDGE_SENSITIVE);

	return;
}



/*
 poll mode
*/
int g_gpio_poll_init_flag = 0;

void test_gpio_button_poll(void)
{
	int ret = 0;
	int i = 0;
	int val = 0;

	if (g_gpio_poll_init_flag == 0)
	{
		/**/
		/* init poll*/
		/**/
		gpio_poll_init(FMQL_DEMO_GPIO_S_1);
		gpio_poll_init(FMQL_DEMO_GPIO_S_2);
		gpio_poll_init(FMQL_DEMO_GPIO_S_3);
		gpio_poll_init(FMQL_DEMO_GPIO_S_4);
		gpio_poll_init(FMQL_DEMO_GPIO_S_5);

		/**/
		/* init input*/
		/**/
		gpio_set_input (FMQL_DEMO_GPIO_S_1);
		gpio_set_input (FMQL_DEMO_GPIO_S_2);
		gpio_set_input (FMQL_DEMO_GPIO_S_3);
		gpio_set_input (FMQL_DEMO_GPIO_S_4);
		gpio_set_input (FMQL_DEMO_GPIO_S_5);

		/**/
		/* connect irq*/
		/**/
		/*
		gpio_attach_irq(FMQL_DEMO_GPIO_S_1);
		gpio_attach_irq(FMQL_DEMO_GPIO_S_2);
		gpio_attach_irq(FMQL_DEMO_GPIO_S_3);
		gpio_attach_irq(FMQL_DEMO_GPIO_S_4);
		gpio_attach_irq(FMQL_DEMO_GPIO_S_5);
		*/	

		/**/
		/* enable gpio poll*/
		/**/
		gpio_Poll_Enable(FMQL_DEMO_GPIO_S_1, GPIO_IRQ_LOW_LEVEL_SENSITIVE);
		gpio_Poll_Enable(FMQL_DEMO_GPIO_S_2, GPIO_IRQ_LOW_LEVEL_SENSITIVE);
		gpio_Poll_Enable(FMQL_DEMO_GPIO_S_3, GPIO_IRQ_LOW_LEVEL_SENSITIVE);
		gpio_Poll_Enable(FMQL_DEMO_GPIO_S_4, GPIO_IRQ_LOW_LEVEL_SENSITIVE);
		gpio_Poll_Enable(FMQL_DEMO_GPIO_S_5, GPIO_IRQ_LOW_LEVEL_SENSITIVE);

		g_gpio_poll_init_flag = 1;
	}

	
	for (i=0; i<10; i++)
	{
		val = gpio_get_input_val(FMQL_DEMO_GPIO_S_1);
		printf("GPIO_S_1: %d \n", val);
		
		val = gpio_get_input_val(FMQL_DEMO_GPIO_S_2);
		printf("GPIO_S_2: %d \n", val);
		
		val = gpio_get_input_val(FMQL_DEMO_GPIO_S_3);
		printf("GPIO_S_3: %d \n", val);
		
		val = gpio_get_input_val(FMQL_DEMO_GPIO_S_4);
		printf("GPIO_S_4: %d \n", val);
		
		val = gpio_get_input_val(FMQL_DEMO_GPIO_S_5);
		printf("GPIO_S_5: %d \n", val);

		printf("\n");
		taskDelay(2*sysClkRateGet());  /* delay 1s*/
	}
	
	return;
}

#endif


#if 1  /* test_note_&_log*/

/*
test cmd & procedure:
---------------------

1. call the function: void testvx_update_bit3(char* bit_file)
->testvx_update_bit3 "/sd0:1/pl_led_key.bit"

2. first:
-> test_gpio_led
   then: observe the led flash

3. first: 
-> test_gpio_button
   then: press the any button of SW5,6,7,8,19 of Demo_board
*/


/*
press the switch key and look up the log:
-----------------------------------------

-> test_gpio_button_poll

GPIO_S_1: 1 
GPIO_S_2: 1 
GPIO_S_3: 1 
GPIO_S_4: 1 
GPIO_S_5: 1 

GPIO_S_1: 1 
GPIO_S_2: 0 
GPIO_S_3: 1 
GPIO_S_4: 1 
GPIO_S_5: 1 

GPIO_S_1: 0 
GPIO_S_2: 1 
GPIO_S_3: 1 
GPIO_S_4: 1 
GPIO_S_5: 1 

GPIO_S_1: 1 
GPIO_S_2: 1 
GPIO_S_3: 1 
GPIO_S_4: 1 
GPIO_S_5: 0 

GPIO_S_1: 1 
GPIO_S_2: 1 
GPIO_S_3: 1 
GPIO_S_4: 0 
GPIO_S_5: 1 

GPIO_S_1: 1 
GPIO_S_2: 1 
GPIO_S_3: 0 
GPIO_S_4: 1 
GPIO_S_5: 1 
*/

#endif


