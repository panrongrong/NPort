/* vxUart.c - fmsh 7020/7045 pUart driver */

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
01a, 28Oct20, jc  written.
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

#include "vxUart.h"

/*
defines 
*/
#if 1

#undef VX_DBG
#undef VX_DBG2

#define UART_DBG

#ifdef UART_DBG

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
uart_0 : default console
*/
static vxT_UART_CTRL vxUart_Ctrl_0 = {0};

static vxT_UART vx_pUart_0 = {0};
vxT_UART * g_pUart0 = (vxT_UART *)(&vx_pUart_0.uart_x);

/* 
uart_1 : 
*/
static vxT_UART_CTRL vxUart_Ctrl_1 = {0};

static vxT_UART vx_pUart_1 = {0};
vxT_UART * g_pUart1 = (vxT_UART *)(&vx_pUart_1.uart_x);


#if 1

void uartCtrl_Wr_CfgReg32(vxT_UART* pUart, UINT32 offset, UINT32 value)
{
    UINT32 tmp32 = pUart->pUartCtrl->cfgBaseAddr;
    FMQL_WRITE_32((tmp32 + offset), value);
    return;
}

UINT32 uartCtrl_Rd_CfgReg32(vxT_UART* pUart, UINT32 offset)
{
    UINT32 tmp32 = pUart->pUartCtrl->cfgBaseAddr;
    return FMQL_READ_32(tmp32 + offset);
}

#endif


#if 1

/****************************************************************************/
/**
*
*  This function initializes a uart. It disables all interrupts and
*  resets the Tx and Rx FIFOs.  It also initializes the driver's
*  internal data structures.
*
* @param   pUart is uart handle. 
*
* @return  
*  0           -- if successful
*  -FMSH_ENOSYS  -- hardware parameters for the device could not be
*              automatically determined
*
* @note    
*  If the FMSH_apb_uart is independently reset, it must be re-initialized
*  by calling this function, in order to keep the device and driver in
*  sync.
*
****************************************************************************/
/*
int uartCtrl_init(vxT_UART * pUart)
{
    int retval;

    // attempt to determine hardare parameters
    retval = uartCtrl_autoCompParams(pUart);

    // disable all uart interrupts
    uartCtrl_Disable_Irq(pUart, Uart_irq_all);

    // reset device FIFOs
    uartCtrl_Rst_TxFifo(pUart);
    uartCtrl_Rst_RxFifo(pUart);

    // initialize private variables
    uartCtrl_Reset(pUart);

    return retval;
}
*/
/****************************************************************************/
/**
*
*  This function returns whether the UART is busy (transmitting and/or
*  receiving) or not.  If the UART busy bit is unsupported, an error
*  code is returned.  
*
* @param   pUart is uart handle. 
*
* @return  
*  TRUE        -- UART is busy
*  FALSE       -- UART is not busy
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - usr/busy
*
****************************************************************************/
int uartCtrl_Is_Busy(vxT_UART * pUart)
{
    int retval;
    UINT32 reg;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_usr);
    if (FMSH_BIT_GET(reg, UART_USR_BUSY) == UART_set)
        retval = TRUE;
    else
        retval = FALSE;

    return retval;
}

/****************************************************************************/
/**
*
*  This function performs a hardware reset on a FMSH_apb_uart device.
*
* @param   pUart is uart handle. 
*
* @return  
*  0           -- if successful
*  -FMSH_ENOSYS  -- function not supported
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - srr/ur
*  After resetting the uart, the device driver should be re-initialized
*  by calling uartCtrl_init().
*
****************************************************************************/
int uartCtrl_Reset(vxT_UART * pUart)
{
    int retval;
    UINT32 reg;

    reg = retval = 0;
    
    FMSH_BIT_SET(reg, UART_SRR_UR, UART_set);
    
    /*uartCtrl_Wr_CfgReg32(pUart, reg, UART_REG_srr);*/
    uartCtrl_Wr_CfgReg32(pUart, UART_REG_srr, reg);

    return retval;
}

/****************************************************************************/
/**
*
*  This funciton resets the transmitter FIFO.
*
* @param   pUart is uart handle. 
*
* @return  None.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - fcr/xfifor
*   - srr/xfr
*
****************************************************************************/
void uartCtrl_Rst_TxFifo(vxT_UART * pUart)
{
    UINT32 reg;

    /* The FIFO reset bits are self-clearing.*/

    reg = 0;
    FMSH_BIT_SET(reg, UART_SRR_XFR, UART_set); /* bfoUART_SRR_XFR*/
    
    uartCtrl_Wr_CfgReg32(pUart, UART_REG_srr, reg);
    
    return;
}

/****************************************************************************/
/**
*
*  This function resets the receiver FIFO.
*
* @param   pUart is uart handle. 
*
* @return  None.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - fcr/rfifor
*   - srr/rfr
*
****************************************************************************/
void uartCtrl_Rst_RxFifo(vxT_UART * pUart)
{
    UINT32 reg;

    reg = 0;
    FMSH_BIT_SET(reg, UART_SRR_RFR, UART_set); /*bfoUART_SRR_RFR*/
    
    uartCtrl_Wr_CfgReg32(pUart, UART_REG_srr, reg);
    
    return;
}

/****************************************************************************/
/**
*
*  Sets the divisor value used when dividing down sclk, which
*  determines the baud rate.  See the FMSH_apb_uart databook for more
*  information.
*
* @param   pUart is uart handle. 
* @param   divisor -- value to set clock divisor.
*
* @return 
*  0           -- if successful
*  -FMSH_EBUSY   -- uart is busy
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - lcr/dlab
*   - dll/all bits
*   - dlh/all bits
*  A delay is required before using the UART after setting the baud
*  rate.  The minimum delay needed is 8 clock cycles of the slowest
*  FMSH_apb_uart clock.
*
****************************************************************************/
int uartCtrl_Set_ClockDivisor(vxT_UART * pUart, UINT16             divisor, UINT16 fdivisor)
{
    int retval;
    UINT32 reg;
    UINT8 dll, dlh, dlf;

    /*the divisor latch access bit (DLAB) of the line*/
    /* control register (LCR) cannot be written while the UART is busy.*/
    /* This does not affect version 2.*/
    if (uartCtrl_Is_Busy(pUart) == TRUE)
    {    
        retval = -FMSH_EBUSY;
    }
    else
    {
        retval = 0;
        
        /* set DLAB to access DLL and DLH registers        */
        reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_lcr);
        
        FMSH_BIT_SET(reg, UART_LCR_DLAB, UART_set); /* bfoUART_LCR_DLAB*/
        
        uartCtrl_Wr_CfgReg32(pUart, UART_REG_lcr, reg);
        
        dll = divisor & 0x00ff;
        dlh = (divisor & 0xff00) >> 8;
        
        dlf = fdivisor;
        
        uartCtrl_Wr_CfgReg32(pUart, UART_REG_rbr_thr_dll, dll);
        uartCtrl_Wr_CfgReg32(pUart, UART_REG_ier_dlh, dlh);        
        uartCtrl_Wr_CfgReg32(pUart, UART_REG_dlf, dlf);
        
        /* clear DLAB*/
        FMSH_BIT_CLEAR(reg, UART_LCR_DLAB);
        
        uartCtrl_Wr_CfgReg32(pUart, UART_REG_lcr, reg);
    }

    return retval;
}

/****************************************************************************/
/**
*
*  This function returns the clock divisor value.  This is the value
*  used to divide down sclk, thereby determining the baud rate.  See
*  the FMSH_apb_uart databook for more information.
*
* @param   pUart is uart handle. 
*
* @return The clock divisor value.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - lcr/dlab
*   - dll/all bits
*   - dlh/all bits
*
*  This function should never be called when the FMSH_apb_uart is busy as
*  to do so can result in data loss.  If the value of the divisor is
*  required while a transfer is in progress, it should be saved to a
*  local variable before the transfer begins.
*
****************************************************************************/
UINT16 uartCtrl_Get_ClockDivisor(vxT_UART * pUart)
{
    UINT8 dll, dlh;
    UINT16 retval;
    UINT32 reg;

    /* set DLAB to access DLL and DLH registers*/
    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_lcr);
    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_lcr);
    
    FMSH_BIT_SET(reg, UART_LCR_DLAB, UART_set);
    uartCtrl_Wr_CfgReg32(pUart, UART_REG_lcr, reg);
    
    dll = uartCtrl_Rd_CfgReg32(pUart, UART_REG_rbr_thr_dll);
    dlh = uartCtrl_Rd_CfgReg32(pUart, UART_REG_ier_dlh);
    
    /* clear DLAB*/
    FMSH_BIT_CLEAR(reg, UART_LCR_DLAB);
    
    uartCtrl_Wr_CfgReg32(pUart, UART_REG_lcr, reg);

    retval = (dlh << 8) | dll;

    return retval;
}

/****************************************************************************/
/**
*
* This function set buad rate of uart device
*
* @param   pUart is a pointer to the uart instance.   
* @param   baud_rate is the buad rate   
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
****************************************************************************/
UINT8 uartCtrl_Set_BaudRate(vxT_UART * pUart, UINT32 baudRate)
{
    UINT16 retval;
    
    UINT32 divisor;
    float divisor_f;
    float tmp;

    UINT32 tmp32 = 0;
    
    retval = 0;
    
    tmp = 16 * baudRate;    
    tmp = (float)(pUart->pUartCtrl->input_clock / tmp); 
    
    divisor = (UINT32)tmp; 
    divisor_f = tmp - divisor;  
    divisor_f = divisor_f * 16;  
    
    if (((UINT32)(divisor_f * 10) % 10) < 5)  /* 四舍五入*/
    {
        tmp32 = (UINT32)(divisor_f * 10) / 10;
    }
    else
    {
        tmp32 = (UINT32)(divisor_f * 10 + 9) / 10;
    }    
    
    /*uartCtrl_Set_ClockDivisor(pUart, divisor, (UINT16)round(divisor_f));*/
    uartCtrl_Set_ClockDivisor(pUart, divisor, (tmp32));
    
    return retval;
}

/****************************************************************************/
/**
*
*  This function is used to set the parity and the number of data and
*  stop bits.  The uartCtrl_line_control defintions are used to specify
*  this mode.  The line control settings should not be changed when the
*  UART is busy.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*  mode        -- line control settings
*
* @return
*  0           -- if successful
*  -FMSH_EBUSY   -- UART is busy
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - lcr/cls
*   - lcr/stop
*   - lcr/pen
*   - lcr/eps
*
****************************************************************************/
int uartCtrl_Set_LineControl(vxT_UART * pUart, int setting)
{
    int retval;
    UINT8 reg;

    /* Note: it does not matter if the UART is busy when setting the*/
    /* line control register on FMSH_apb_uart v2.00.  For uart v2.00,*/
    /* the isBusy function returns -FMSH_ENOSYS (not supported).  This is*/
    /* necessary, however, for UART v3.00.*/
    if (uartCtrl_Is_Busy(pUart) == TRUE)
    {    
        retval = -FMSH_EBUSY;
    }
    else 
    {
        retval = 0;
        
        reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_lcr);
        
        /* avoid bus write is possible*/
        if (FMSH_BIT_GET(reg, UART_LCR_LINE) != setting) 
        {
            FMSH_BIT_SET(reg, UART_LCR_LINE, setting);
            
            uartCtrl_Wr_CfgReg32(pUart, UART_REG_lcr, reg);
        }
    }

    return retval;
}

/****************************************************************************/
/**
*
*  Returns the line control settings.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  The current line control settings.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - lcr/cls
*   - lcr/stop
*   - lcr/pen
*   - lcr/eps
*
****************************************************************************/
int uartCtrl_Get_LineControl(vxT_UART             * pUart)
{
    UINT32 reg;
    int retval;

    /* get the value of the LCR, masking irrelevant bits*/
    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_lcr);
    
    retval =  FMSH_BIT_GET(reg, UART_LCR_LINE);

    return retval;
}

/****************************************************************************/
/**
*
* Sets the number of bits per character (5/6/7/8).
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*  cls         -- number of data bits per character
*
* @return None.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - lcr/cls
*
****************************************************************************/
void uartCtrl_Set_DataBits(vxT_UART * pUart, int           cls)
{
    UINT32 reg;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_lcr);
    
    if (FMSH_BIT_GET(reg, UART_LCR_CLS) != cls) 
    {
        FMSH_BIT_SET(reg, UART_LCR_CLS, cls);
        
        uartCtrl_Wr_CfgReg32(pUart, UART_REG_lcr, reg);
    }

    return;
}

/****************************************************************************/
/**
*
* Returns the number of bits per character setting for data transfers.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return 
*  The current number of data bits setting.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - lcr/cls
*
****************************************************************************/
int uartCtrl_Get_DataBits(vxT_UART * pUart)
{
    UINT32 reg;
    int retval;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_lcr);
    
    retval =  FMSH_BIT_GET(reg, UART_LCR_CLS);

    return retval;
}

/****************************************************************************/
/**
*
*  Sets the number of stop bits (1/1.5/2).
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*  stop        -- number of stop bits
*
* @return None.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - lcr/stop
*
****************************************************************************/
void uartCtrl_Set_StopBits(vxT_UART * pUart, int stop)
{
    UINT32 reg;

    /*reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_lcr);    */
    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_lcr);
    
    if(FMSH_BIT_GET(reg, UART_LCR_STOP) != stop)
    {
        FMSH_BIT_SET(reg, UART_LCR_STOP, stop);
        
        uartCtrl_Wr_CfgReg32(pUart, UART_REG_lcr, reg);
    }

    return;
}

/****************************************************************************/
/**
*
*  Returns the number of stop bits setting for data transfers.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  The current number of stop bits setting.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - lcr/stop
*
****************************************************************************/
int uartCtrl_Get_StopBits(vxT_UART * pUart)
{
    UINT32 reg;
    int retval;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_lcr);
    
    retval = FMSH_BIT_GET(reg, UART_LCR_STOP);

    return retval;
}

/****************************************************************************/
/**
*
*  Sets the parity mode (none/odd/even).
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*  parity      -- parity to set
*
* @return None��
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - lcr/pen
*   - lcr/eps
*
****************************************************************************/
void uartCtrl_Set_Parity(vxT_UART * pUart, int parity)
{
    UINT32 reg;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_lcr);
    
    if (FMSH_BIT_GET(reg, UART_LCR_PARITY) != parity) 
    {
        FMSH_BIT_SET(reg, UART_LCR_PARITY, parity);
        
        uartCtrl_Wr_CfgReg32(pUart, UART_REG_lcr, reg);
    }

    return;
}

void uartCtrl_Set_Stick(vxT_UART * pUart, UINT8 stick)
{
    UINT32 reg;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_lcr);
    
    if (FMSH_BIT_GET(reg, UART_LCR_STICK_PARITY) != stick) 
    {
        FMSH_BIT_SET(reg, UART_LCR_STICK_PARITY, stick);
        
        uartCtrl_Wr_CfgReg32(pUart, UART_REG_lcr, reg);
    }
    
    return;
}

/****************************************************************************/
/**
*
*  Returns the parity setting for data transfers.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  The current parity setting.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - lcr/pen
*   - lcr/eps
*
****************************************************************************/
int uartCtrl_Get_Parity(vxT_UART * pUart)
{
    UINT32 reg;
    int retval;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_lcr);
    retval = FMSH_BIT_GET(reg, UART_LCR_PARITY);

    return retval;
}

/****************************************************************************/
/**
*
*  This function enables receive and transmit FIFOs, if they are
*  available.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  0           -- if successful
*  -FMSH_ENOSYS  -- function not supported
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - fcr/fifoe
*   - sfe/fe
*
****************************************************************************/
int uartCtrl_Enable_Fifos(vxT_UART * pUart)
{
    int retval;
    UINT32 reg;

    /* if FIFOs are available*/
    retval = 0;
    
    /* The status of the FIFOs is cached here as reading the IIR*/
    /* register directly with uartCtrl_Is_FifosEnabled() can*/
    /* inadvertently clear any pending THRE interrupt.*/
    pUart->pUartCtrl->tChn.fifos_enabled = TRUE;
    
    /* update stored FCR value*/
    FMSH_BIT_SET(pUart->pUartCtrl->tChn.value_in_fcr, UART_FCR_FIFO_ENABLE, UART_set);
    
    /* if shadow registers are available*/
    reg = 0;
    FMSH_BIT_SET(reg, UART_SFE_FE, UART_set); /*bfoUART_SFE_FE*/
    uartCtrl_Wr_CfgReg32(pUart, UART_REG_sfe, reg);

    return retval;
}

/****************************************************************************/
/**
*
* This function disables receive and transmit FIFOs.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  0           -- if successful
*  -FMSH_ENOSYS  -- function not supported
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - fcr/fifoe
*   - sfe/fe
*
****************************************************************************/
int uartCtrl_Disable_Fifos(vxT_UART * pUart)
{
    int retval;
    UINT32 reg;

    retval = 0;
    
    /* The status of the FIFOs is cached here as reading the IIR*/
    /* register directly with uartCtrl_Is_FifosEnabled() can*/
    /* inadvertently clear any pending THRE interrupt.*/
    pUart->pUartCtrl->tChn.fifos_enabled = FALSE;
    
    /* update stored FCR value*/
    FMSH_BIT_CLEAR(pUart->pUartCtrl->tChn.value_in_fcr, UART_FCR_FIFO_ENABLE);
    
    /* if shadow registers are available*/
    reg = 0;
    FMSH_BIT_SET(reg, UART_SFE_FE, UART_clear);
    uartCtrl_Wr_CfgReg32(pUart, UART_REG_sfe, reg);

    return retval;
}

/****************************************************************************/
/**
*
* Returns whether the FIFOs and enabled or not.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  TRUE        -- FIFOs are enabled
*  FALSE       -- FIFOs are disabled or not available
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - iir/fifose
*   - sfe/fe
*  warning��
*  This function can indirectly clear a THRE interrupt.  When the
*  SHADOW registers are not available, it reads the IIR register in
*  order to determine if FIFOs are enabled or not.  Reading the IIR
*  register clears any pending THRE interrupt.
*
****************************************************************************/
int uartCtrl_Is_FifosEnabled(vxT_UART * pUart)
{
    int retval;
    UINT32 reg;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_sfe);
    
    if (FMSH_BIT_GET(reg, UART_SFE_FE) == UART_clear)
        retval = FALSE;
    else
        retval = TRUE;

    return retval;
}

/****************************************************************************/
/**
*
* Returns whether the transmitter FIFO is full or not.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  TRUE        -- Tx FIFO is full
*  FALSE       -- Tx FIFO is not full
*  -FMSH_ENOSYS  -- function not supported
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*  This function is affected by the FIFO_STAT hardware parameter.
*   - usr/tfnf
*
****************************************************************************/
int uartCtrl_Is_TxFifoFull(vxT_UART * pUart)
{
    int retval;
    UINT32 reg;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_usr);
    
    if (FMSH_BIT_GET(reg, UART_USR_TFNF) == UART_clear)
        retval = TRUE;
    else
        retval = FALSE;

    return retval;
}

/****************************************************************************/
/**
*
* Returns whether the transmitter FIFO is empty or not.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  TRUE        -- Tx FIFO is empty
*  FALSE       -- Tx FIFO is not empty
*  -FMSH_ENOSYS  -- function not supported
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - usr/tfe
*
*  This function is affected by the FIFO_STAT hardware parameter.
*
****************************************************************************/
int uartCtrl_Is_TxFifoEmpty(vxT_UART * pUart)
{
    int retval;
    UINT32 reg;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_usr);
    
    if (FMSH_BIT_GET(reg, UART_USR_TFE) == 1)
        retval = TRUE;
    else
        retval = FALSE;

    return retval;
}

/****************************************************************************/
/**
*
* Returns whether the receiver FIFO is full or not.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  TRUE        -- Rx FIFO is full
*  FALSE       -- Rx FIFO is not full
*  -FMSH_ENOSYS  -- function not supported
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - usr/rff
*
*  This function is affected by the FIFO_STAT hardware parameter.
*
****************************************************************************/
int uartCtrl_Is_RxFifoFull(vxT_UART * pUart)
{
    int retval;
    UINT32 reg;    

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_usr);
    
    if (FMSH_BIT_GET(reg, UART_USR_RFF) == 1)
        retval = TRUE;
    else
        retval = FALSE;

    return retval;
}

/****************************************************************************/
/**
*
* This function returns whether the receiver FIFO is empty or not.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  TRUE        -- Rx FIFO is empty
*  FALSE       -- Rx FIFO is not empty
*  -FMSH_ENOSYS  -- function not supported
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - usr/rfne
*
*  This function is affected by the FIFO_STAT hardware parameter.
*
****************************************************************************/
int uartCtrl_Is_RxFifoEmpty(vxT_UART * pUart)
{
    int retval;
    UINT32 reg;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_usr);
    
    /* RFNE bit clear == Rx FIFO empty*/
    if (FMSH_BIT_GET(reg, UART_USR_RFNE) == UART_clear)
        retval = TRUE;
    else
        retval = FALSE;

    return retval;
}

/****************************************************************************/
/**
*
*  This function returns the number of characters currently present in
*  the Tx FIFO.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  -FMSH_ENOSYS  -- function not supported
*  Otherwise number of characters currently in the Tx FIFO is returned.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - tfl/all bits
*
*  This function is affected by the FIFO_STAT hardware parameter.
*
****************************************************************************/
int uartCtrl_Get_TxFifoLevel(vxT_UART * pUart)
{
    int retval;
    UINT32 reg;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_tfl);
    retval = FMSH_BIT_GET(reg, UART_TFL_LEVEL);

    return retval;
}

/****************************************************************************/
/**
*
*  This function returns the number of characters currently present in
*  the Rx FIFO.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  -FMSH_ENOSYS  -- function not supported
*  Otherwise number of characters currently in the Rx FIFO is returned.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - rfl/all bits
*
*  This function is affected by the FIFO_STAT hardware parameter.
*
****************************************************************************/
int uartCtrl_Get_RxFifoLevel(vxT_UART * pUart)
{
    int retval;
    UINT32 reg;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_rfl);
    retval = FMSH_BIT_GET(reg, UART_RFL_LEVEL);

    return retval;
}

/****************************************************************************/
/**
*
* Returns how many bytes deep the transmitter and receiver FIFOs are.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  FIFO depth in bytes (64)
*
* @note    
*  This function is affected by the FIFO_MODE hardware parameter.
*
****************************************************************************/
unsigned uartCtrl_Get_FifoDepth(vxT_UART * pUart)
{
    unsigned retval;

    retval = pUart->pUartCtrl->tParam.fifo_depth;

    return retval;
}

/****************************************************************************/
/**
*
*  This function enables Programmable Threshold (THRE) Interrupt Mode
*  (PTIME).  This mode enables triggering of interrupts for different
*  levels of the Tx/Rx FIFOs.  Enabling PTIME also changes the
*  functionality of the lsr/thre bit (LSR[5]) to indicate that the Tx
*  FIFO is full.  See the FMSH_apb_uart databook for more information.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  0           -- if successful
*  -FMSH_ENOSYS  -- function not supported
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - ier/ptime
*
*  This function is affected by the THRE_MODE hardware parameter.
*  warning��
*  FIFOs must also be enabled in order to use the Programmable THRE
*  Interrupt Mode.
*
****************************************************************************/
int uartCtrl_Enable_Ptime(vxT_UART * pUart)
{
    int retval;
    UINT32 reg;

    retval = 0;
    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_ier_dlh);
    
    /* avoid bus write if possible*/
    if (FMSH_BIT_GET(reg, UART_IER_PTIME) != UART_set)
    {
        FMSH_BIT_SET(reg, UART_IER_PTIME, UART_set);
        
        /* save IER value*/
        pUart->pUartCtrl->tChn.ier_save = reg;
        
        /* enable PTIME*/
        uartCtrl_Wr_CfgReg32(pUart, UART_REG_ier_dlh, reg);
    }

    return retval;
}

/****************************************************************************/
/**
*
*  This function disables Programmable Threshold (THRE) Interrupt Mode
*  (PTIME).  When PTIME is disabled, the functionality of the lsr/thre
*  bit (LSR[5]) is normal, indicating that the Tx FIFO/THR is empty.
*  See the FMSH_apb_uart databook for more information.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  0           -- if successful
*  -FMSH_ENOSYS  -- function not supported
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - ier/ptime
*
*  This function is affected by the THRE_MODE hardware parameter.
*
****************************************************************************/
int uartCtrl_Disable_Ptime(vxT_UART * pUart)
{
    int retval;
    UINT32 reg;

    retval = 0;
    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_ier_dlh);
    
    /* avoid bus write if possible*/
    if (FMSH_BIT_GET(reg, UART_IER_PTIME) != UART_clear) 
    {
        FMSH_BIT_SET(reg, UART_IER_PTIME, UART_clear);
        
        /* save IER value*/
        pUart->pUartCtrl->tChn.ier_save = reg;
        
        /* disable PTIME*/
        uartCtrl_Wr_CfgReg32(pUart, UART_REG_ier_dlh, reg);
    }

    return retval;
}

/****************************************************************************/
/**
*
*  This function returns whether Programmable Threshold (THRE)
*  Interrupt Mode (PTIME) is enabled or not.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  TRUE        -- PTIME is enabled
*  FALSE       -- PTIME is disabled
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - ier/ptime
*
****************************************************************************/
int uartCtrl_Is_PtimeEnabled(vxT_UART * pUart)
{
    int retval;
    UINT32 reg;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_ier_dlh);
    
    if (FMSH_BIT_GET(reg, UART_IER_PTIME) == UART_set)
        retval = TRUE;
    else
        retval = FALSE;

    return retval;
}

/****************************************************************************/
/**
*
*  Sets the break control bit to 'state'.  When enabled, it causes a
*  break signal to be generated, by holding the sout line low, until
*  the break bit is subsequently cleared (with this function).
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*  state       -- Set or Clear
*
* @return None.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - lcr/break
*   - sbcr/bcr
*
*  This function is affected by the SHADOW hardware parameter.
*
****************************************************************************/
void uartCtrl_Set_Break(vxT_UART * pUart, int state)
{
    UINT32 reg;
    
    reg = 0;
    FMSH_BIT_SET(reg, UART_SBCR_BCR, state);
    
    uartCtrl_Wr_CfgReg32(pUart, UART_REG_sbcr, state);

    return;
}

/****************************************************************************/
/**
*
* Returns the state of the break control bit.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  The current break bit state.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - lcr/break
*   - sbcr/bcr
*
*  This function is affected by the SHADOW hardware parameter.
*
****************************************************************************/
int uartCtrl_Get_Break(vxT_UART * pUart)
{
    UINT32 reg;
    int retval;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_sbcr);
    retval = (int) FMSH_BIT_GET(reg, UART_SBCR_BCR);

    return retval;
}

/****************************************************************************/
/**
*
*  This function is used to Set specific modem lines.  The lines
*  argument comprises of one or more bitwise OR'ed uartCtrl_modem_line
*  enumerated values.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*  lines       -- modem line(s) to Set
*
* @return None.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - mcr/dtr
*   - mcr/rts
*   - mcr/out1
*   - mcr/out2
*
****************************************************************************/
void uartCtrl_Set_ModemLine(vxT_UART * pUart,            int lines)
{ 
    UINT32 reg;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_mcr);
    reg |= lines;
    
    uartCtrl_Wr_CfgReg32(pUart, UART_REG_mcr, reg);

    return;
}

/****************************************************************************/
/**
*
*  This function is used to Clear specific modem lines.  The lines
*  argument comprises of one or more bitwise OR'ed uartCtrl_modem_line
*  enumerated values.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
 *  lines       -- modem line(s) to Clear
*
* @return None.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - mcr/dtr
*   - mcr/rts
*   - mcr/out1
*   - mcr/out2
*
****************************************************************************/
void uartCtrl_clearModemLine(vxT_UART * pUart,             int lines)
{
    UINT32 reg;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_mcr);
    reg &= ~lines;
    
    uartCtrl_Wr_CfgReg32(pUart, UART_REG_mcr, reg);

    return;
}

/****************************************************************************/
/**
*
*  This function returns the state of the modem control lines.  The
*  uartCtrl_modem_line enumerated values are used with this function's
*  return value to determine the current state of the modem lines.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  The current value on the modem line control settings.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - mcr/dtr
*   - mcr/rts
*   - mcr/out1
*   - mcr/out2
*
****************************************************************************/
int uartCtrl_Get_ModemLine(vxT_UART            * pUart)
{
    UINT32 reg;
    int retval;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_mcr);
    retval = (int) (reg & Uart_modem_line_all);

    return retval;
}

/****************************************************************************/
/**
*
*  Enables loopback mode.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  The current value on the modem line control settings.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - mcr/loopback
*
****************************************************************************/
void uartCtrl_Enable_Loopback(vxT_UART * pUart)
{
    UINT32 reg;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_mcr);
    
    if (FMSH_BIT_GET(reg, UART_MCR_LOOPBACK) != UART_set)
    {
        FMSH_BIT_SET(reg, UART_MCR_LOOPBACK, UART_set);
        uartCtrl_Wr_CfgReg32(pUart, UART_REG_mcr, reg);
    }

    return;
}

/****************************************************************************/
/**
*
*  Disables loopback mode.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  None.
*
* @note    
 *  Accesses the following FMSH_apb_uart register(s)/bit field(s):
 *   - mcr/loopback
*
****************************************************************************/
void uartCtrl_Disable_Loopback(vxT_UART * pUart)
{
    UINT32 reg;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_mcr);
    
    if (FMSH_BIT_GET(reg, UART_MCR_LOOPBACK) != UART_clear)
    {
        FMSH_BIT_SET(reg, UART_MCR_LOOPBACK, UART_clear);
        
        uartCtrl_Wr_CfgReg32(pUart, UART_REG_mcr, reg);
    }

    return;
}

/****************************************************************************/
/**
*
*  Returns whether loopback mode is enabled or not.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  TRUE        -- loopback mode is enabled
*  FALSE       -- loopback mode is disabled
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - mcr/loopback
*
****************************************************************************/
int uartCtrl_Is_LoopbackEnabled(vxT_UART * pUart)
{
    int retval;
    UINT32 reg;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_mcr);
    retval = (int) FMSH_BIT_GET(reg, UART_MCR_LOOPBACK);

    return retval;
}

/****************************************************************************/
/**
*
*  Enables Automatic Flow Control mode.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  0           -- if successful
*  -FMSH_ENOSYS  -- function not supported
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - mcr/afce
*
*  This function is affected by the AFCE_MODE hardware parameter.
*
****************************************************************************/
int uartCtrl_Enable_Afc(vxT_UART * pUart)
{
    int retval;
    UINT32 reg;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_mcr);
    FMSH_BIT_SET(reg, UART_MCR_AFCE, UART_set);
    
    uartCtrl_Wr_CfgReg32(pUart, UART_REG_mcr, reg);
    retval = 0;

    return retval;
}

/****************************************************************************/
/**
*
*  Disables Automatic Flow Control mode.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  0           -- if successful
*  -FMSH_ENOSYS  -- function not supported
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - mcr/afce
*
*  This function is affected by the AFCE_MODE hardware parameter.
*
****************************************************************************/
int uartCtrl_Disable_Afc(vxT_UART * pUart)
{
    int retval;
    UINT32 reg;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_mcr);
    FMSH_BIT_CLEAR(reg, UART_MCR_AFCE);
    
    uartCtrl_Wr_CfgReg32(pUart, UART_REG_mcr, reg);
    retval = 0;

    return retval;
}

/****************************************************************************/
/**
*
*  Returns whether Automatic Flow Control mode is enabled or not.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  TRUE        -- Automatic Flow Control is enabled
*  FALSE       -- Automatic Flow Control is disabled
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - mcr/afce
*
*  This function is affected by the AFCE_MODE hardware parameter.
*
****************************************************************************/
int uartCtrl_Is_AfcEnabled(vxT_UART * pUart)
{
    int retval;
    UINT32 reg;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_mcr);
    retval = FMSH_BIT_GET(reg, UART_MCR_AFCE);

    return retval;
}

/****************************************************************************/
/**
*
*  This function returns the current line status register value.  This
*  value is used in conjunction with the uartCtrl_line_status enumerated
*  values to determine the current line status.  See the FMSH_apb_uart
*  databook for more information about the line status register.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
* The current line status register value.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - lsr/all bits
*
*  When using the Interrupt API and the FIFO status registers are not
*  available, the line status register is read to determine when the
*  Tx/Rx FIFOs are full/empty.  As this can inadvertently reset an
*  error bit, any active error bits are temporarily saved.  These bits
*  are OR'ed with the return value of this function when it is called.
*
*  This is to ensure that no errors are missed.  Also note that this
*  means that reported errors are not necessarily associated with the
*  character at the top of the Rx FIFO, when using the Interrupt API.
*
****************************************************************************/
int uartCtrl_Get_LineStatus(vxT_UART * pUart)
{
    UINT8 reg;
    int retval;

    /* read line status register*/
    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_lsr);
    
    /* include saved line status errors*/
    retval = (int) (reg | pUart->pUartCtrl->tChn.lsr_save);
    
    /* reset saved errors*/
    pUart->pUartCtrl->tChn.lsr_save = 0;

    return retval;
}

/****************************************************************************/
/**
*
*  This function returns the current modem status register value.  This
*  value is used in conjunction with the uartCtrl_modem_status
*  enumerated values to determine the current modem status.  See the
*  FMSH_apb_uart databook for more information about the modem status
*  register.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  The current modem status register value.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - msr/all bits
*
****************************************************************************/
int uartCtrl_Get_ModemStatus(vxT_UART * pUart)
{
    UINT8 reg;
    int retval;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_msr);
    retval = (int) reg;

    return retval;
}

/****************************************************************************/
/**
*
*  Sets the value of the scratchpad register.  This register has no
*  functional use and is available to a programmer to use at their own
*  discretion.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*  byte        -- value to set
*
* @return
*  None.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - scr/all bits
*
****************************************************************************/
void uartCtrl_Set_Scratchpad(vxT_UART * pUart, UINT8 value)
{
    uartCtrl_Wr_CfgReg32(pUart, UART_REG_scr, value);
    return;
}

/****************************************************************************/
/**
*
*  Returns the value of the scratchpad register.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  The current scratchpad register value.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - scr/all bits
*
****************************************************************************/
UINT8 uartCtrl_Get_Scratchpad(vxT_UART * pUart)
{
    UINT8 retval;

    retval = uartCtrl_Rd_CfgReg32(pUart, UART_REG_scr);

    return retval;
}

/****************************************************************************/
/**
*
*  Reads a single character from the receiver FIFO and returns it.
*  This function does not check if there is data in the Rx FIFO
*  beforehand, which is a user responsibility.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  The character read from the Rx FIFO.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - rbr/all bits
*
****************************************************************************/
UINT8 uartCtrl_read(vxT_UART * pUart)
{
    UINT8 retval;

    retval = uartCtrl_Rd_CfgReg32(pUart, UART_REG_rbr_thr_dll);

    return retval;
}

/****************************************************************************/
/**
*
*  Writes a single character to the transmitter FIFO.  This function
*  does not check if there is space in the Tx FIFO beforehand, which is
*  a user responsibility.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*  character   -- character to write
*
* @return
*  None.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - thr/all bits
*
****************************************************************************/
void uartCtrl_write(vxT_UART * pUart, UINT8 character)
{
    uartCtrl_Wr_CfgReg32(pUart, UART_REG_rbr_thr_dll, character);
    return;
}

/****************************************************************************/
/**
*
*  This function reads characters from the Rx FIFO, using burst
*  transactions on the AHB bus.  This function does not check if there
*  is enough valid data in the Rx FIFO beforehand, which is a user
*  responsibility.  The length argument should never exceed the FIFO
*  depth.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*  buffer      -- buffer to which data is stored
*  length      -- number of characters to read
*
* @return
*  0           -- if successful
*  -FMSH_ENOSYS  -- function not supported
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - srbr/all bits
*
*  This function is affected by the SIR_MODE hardware parameter.
*
*  The ANSI C memcpy() function is used to read from the shadow receive
*  buffer registers in bursts of four characters.  When compiled using
*  the ARM compiler, this function is replaced with efficient LDM/STM
*  assembler instructions, thus improving AHB bus usage.
*
****************************************************************************/
int uartCtrl_burstRead(vxT_UART * pUart, UINT8 *buffer, unsigned length)
{
    int i, index, retval;
    UINT32 tmpbuf[4];
    
    UINT32 tmp32 = pUart->pUartCtrl->cfgBaseAddr;

    retval = i = 0;
    
    /* if there are four or more bytes to be read*/
    while (((int) length - (i + 4)) >= 4) 
    {
        /* burst read from the shadow receive buffer registers*/
        /*memcpy(tmpbuf, &(UART_REG_srbr_sthr[0]), sizeof(UINT32) *4);*/
        memcpy(tmpbuf, (UINT32*)(tmp32+UART_REG_srbr_sthr_BASE), (sizeof(UINT32) * 4));
        
        /* copy 4 bytes from temporary word buffer to user buffer*/
        for (index = 0; index < 4; i++, index++)
        {    
            buffer[i] = tmpbuf[index];
        }
    }
    
    /* read any remaining characters*/
    while (i < length) 
    {    
        buffer[i++] = (UINT8) uartCtrl_Rd_CfgReg32(pUart, UART_REG_rbr_thr_dll);
    }

    return retval;
}

/****************************************************************************/
/**
*
*  This function writes a characters to the Tx FIFO, using burst
*  transfers on the AHB bus.  This function does not check if there is
*  sufficient spave available in the Tx FIFO beforehand, which is a
*  user responsibility.  The length argument should never exceed the
*  FIFO depth.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*  buffer      -- buffer from which to send data
*  length      -- number of characters to write
*
* @return
*  0           -- if successful
*  -FMSH_ENOSYS  -- function not supported
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - sthr/all bits
*
*  This function is affected by the SIR_MODE hardware parameter.
*  The ANSI C memcpy() function to write to the shadow transmit hold
*  registers in bursts of four characters.  When compiled using the ARM
*  compiler, this function is replaced with efficient LDM/STM assembler
*  instructions, thus improving AHB bus usage.
*
****************************************************************************/
int uartCtrl_burstWrite(vxT_UART * pUart, UINT8 *buffer, unsigned length)
{
    int i, index, retval;
    UINT32 tmpbuf[4];
    
    UINT32 tmp32 = pUart->pUartCtrl->cfgBaseAddr;

    retval = i = 0;
    
    while (((int) length - i) >= 4)
    {
        /* copy 4 bytes from user buffer to temporary word buffer*/
        for (index = 0; index < 4; i++, index++)
        {
            tmpbuf[index] = buffer[i];
        }
        
        /* burst write to the shadow transmit hold registers*/
        /*memcpy(&(UART_REG_srbr_sthr[0]), tmpbuf, (sizeof(UINT32) *4));*/
        memcpy((UINT32*)(tmp32+UART_REG_srbr_sthr_BASE), tmpbuf, (sizeof(UINT32) * 4));
    }
    
    /* write any remaining characters*/
    while (i < length) 
    {
        uartCtrl_Wr_CfgReg32(pUart, UART_REG_rbr_thr_dll, buffer[i++]);
    }

    return retval;
}

/****************************************************************************/
/**
*
*  Enables specified interrupt(s).
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*  interrupts  -- interrupt(s) to enable
*
* @return
* None.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - ier/all bits
*
****************************************************************************/
void uartCtrl_Enable_Irq(vxT_UART * pUart, int
        interrupts)
{
    UINT32 reg;

    /* get current interrupt enable settings*/
    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_ier_dlh);
    
    /* set interrupts to be enabled*/
    reg |= interrupts;
    
    /* update copy of IER value (used when avoiding shared data issues)*/
    pUart->pUartCtrl->tChn.ier_save = reg;
    
    /* write new IER value*/
    uartCtrl_Wr_CfgReg32(pUart, UART_REG_ier_dlh, reg);

    return;
}

/****************************************************************************/
/**
*
*  Disables specified interrupt(s).
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*  interrupts  -- interrupt(s) to disable
*
* @return
* None.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - ier/all bits
*
****************************************************************************/
void uartCtrl_Disable_Irq(vxT_UART * pUart, int           interrupts)
{
    UINT32 reg;

    /* get current interrupt enable settings*/
    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_ier_dlh);
    
    /* mask interrupts to be disabled*/
    reg = (UINT32) reg & ~interrupts;
    
    /* update copy of IER value (used when avoiding shared data issues)*/
    pUart->pUartCtrl->tChn.ier_save = reg;
    
    /* write new IER value*/
    uartCtrl_Wr_CfgReg32(pUart, UART_REG_ier_dlh, reg);

    return;
}

/****************************************************************************/
/**
*
*  Returns whether the specified interrupt is enabled or not.  Only one
*  interrupt may be specifed per invocation of this function.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*  interrupt   -- interrupt to check
*
* @return
*  TRUE        -- interrupt is enabled
*  FALSE       -- interrupt is disabled
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - ier/all bits
*
****************************************************************************/
int uartCtrl_Is_IrqEnabled(vxT_UART * pUart, int            interrupt)
{
    int retval;
    UINT32 reg;

    /* get current interrupt enable settings*/
    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_ier_dlh);
    
    /* specified interrupt enabled?*/
    if ((reg & interrupt) != 0)
        retval = TRUE;
    else
        retval = FALSE;

    return retval;
}

/****************************************************************************/
/**
*
*  Returns the current interrupt mask.  For each bitfield, a value of
*  '0' indicates that an interrupt is masked while a value of '1'
*  indicates that an interrupt is enabled.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  TRUE        -- interrupt is enabled
*  FALSE       -- interrupt is disabled
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - ier/all bits
*
****************************************************************************/
UINT8 uartCtrl_Get_IrqMask(vxT_UART * pUart)
{
    UINT8 retval;

    /* get current interrupt enable settings*/
    retval = uartCtrl_Rd_CfgReg32(pUart, UART_REG_ier_dlh);

    return retval;
}

/****************************************************************************/
/**
*
*  Returns the event identification number of the highest priority
*  interrupt that is active.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  The current highest priority active interrupt.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - iir/iid
*  warnings
*  This function can indirectly clear a THRE interrupt.  Reading the
*  IIR register clears any pending THRE interrupt.
*
****************************************************************************/
int uartCtrl_Get_ActiveIrq(vxT_UART * pUart)
{
    UINT32 reg;
    int retval;

    /* get current interrupt ID*/
    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_iir_fcr);
    retval = (int) FMSH_BIT_GET(reg, UART_IIR_INTERRUPT_ID);

    return retval;
}

/****************************************************************************/
/**
*
*  Sets the trigger level of the transmitter FIFO empty interrupt.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*  trigger       -- level at which to set trigger
*
* @return
*  None.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - fcr/tet
*   - stet/trigger
*
****************************************************************************/
void uartCtrl_Set_TxTrigger(vxT_UART * pUart, int trigger)
{
    UINT32 reg;

    /* save user-specified Tx trigger*/
    pUart->pUartCtrl->tChn.txTrigger = trigger;

    /* update stored FCR value*/
    FMSH_BIT_SET(pUart->pUartCtrl->tChn.value_in_fcr, UART_FCR_TX_EMPTY_TRIGGER, trigger);
    
    /* set transmitter empty trigger*/
    reg = 0;
    FMSH_BIT_SET(reg, UART_STET_TRIGGER, trigger);
    uartCtrl_Wr_CfgReg32(pUart, UART_REG_stet, reg);
    
    return;
}
        
/****************************************************************************/
/**
*
*  Gets the trigger level of the transmitter FIFO empty interrupt.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*
* @return
*  transmitter trigger level
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - fcr/tet
*   - stet/trigger
*
****************************************************************************/
int uartCtrl_Get_TxTrigger(vxT_UART          * pUart)
{
    UINT32 reg;
    int retval;

    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_stet);
    retval = (int) FMSH_BIT_GET(reg, UART_STET_TRIGGER);
    
    return retval;
}

/****************************************************************************/
/**
*
*  Sets the trigger level for the receiver FIFO full interrupt.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*  trigger       -- level at which to set trigger
*
* @return
*  None
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - fcr/rt
*   - srt/trigger
*
****************************************************************************/
  void uartCtrl_Set_RxTrigger(vxT_UART * pUart, int trigger)
{
    UINT32 reg;

    /* update stored FCR value*/
    FMSH_BIT_SET(pUart->pUartCtrl->tChn.value_in_fcr, UART_FCR_RCVR_TRIGGER, trigger);
    
    /* set receiver trigger*/
    reg = 0;
    FMSH_BIT_SET(reg, UART_SRT_TRIGGER, trigger);
    uartCtrl_Wr_CfgReg32(pUart, UART_REG_srt, reg);

    return;
}

/****************************************************************************/
/**
*
*  Gets the trigger level of the receiver FIFO full interrupt.
*
* @param 
*  pUart         -- FMSH_apb_uart handle
*  level       -- level at which to set trigger
*
* @return
*  The receiver empty trigger level.
*
* @note    
*  Accesses the following FMSH_apb_uart register(s)/bit field(s):
*   - fcr/rt
*   - srt/trigger
*
****************************************************************************/
int uartCtrl_Get_RxTrigger(vxT_UART         * pUart)
{
    UINT32 reg;
    int retval;

    /* get receiver trigger*/
    reg = uartCtrl_Rd_CfgReg32(pUart, UART_REG_srt);
    retval = (int) FMSH_BIT_GET(reg, UART_SRT_TRIGGER);

    return retval;
}

#if 0

/******************************************************************************
*
*  This function is used to transmit date.
*
* @param 
*  pUart      -- FMSH_apb_uart handle
*  buffer   -- 
*
* @return
*  0        -- 
*  -FMSH_EBUSY     -- 
*
* @note    
*
******************************************************************************/
int uartCtrl_transmit(vxT_UART * pUart, void *buffer, unsigned
        length, FMSH_callback userFunction)
{
    int retval;
    unsigned numChars;
    UINT8 *tmp;
    uartCtrl_Param_T *param;
    uartCtrl_Portmap_T *portmap;
    uartCtrl_Instance_T *instance;
    
    /*FMSH_REQUIREMENTS(pUart);*/
    FMSH_REQUIRE(buffer != NULL);
    FMSH_REQUIRE(length != 0);

    param = &(pUart->comp_param);
    portmap = (uartCtrl_Portmap_T*) pUart->base_address;
    instance = &(pUart->instance);

    /* disable UART interrupts*/
    UART_ENTER_CRITICAL_SECTION();
    
    if((instance->state == Uart_state_tx)
            || (instance->state == Uart_state_tx_rx))
        retval = -FMSH_EBUSY;
    else {
        retval = 0;

        /* update state*/
        switch(instance->state) {
            case Uart_state_rx:
                instance->state = Uart_state_tx_rx;
                break;
            case Uart_state_idle:
                instance->state = Uart_state_tx;
                break;
            default:
                FMSH_ASSERT(FALSE);
                break;
        }

        /* reset number of characters sent in current/last transfer*/
        instance->txLength = length;
        instance->txRemain = length;
        instance->txCallback = userFunction;
        instance->txHold = 0;
        instance->txIdx = 0;

        /* check for non word-aligned buffer as UART[_X]_FIFO_WRITE()*/
        /* works efficiently on word reads from txHold*/
        tmp = (UINT8 *) buffer;
        while(((unsigned) tmp) & 0x3) {
            instance->txHold |= ((*tmp++ & 0xff) << (instance->txIdx *
                        8));
            instance->txIdx++;
        }
        instance->txBuffer = (UINT32 *) tmp;

        if(instance->fifos_enabled == FALSE) {
            UART_FIFO_WRITE();
        }
        else if(uartCtrl_Is_TxFifoEmpty(pUart) == TRUE) {
            numChars = param->fifo_depth;
            UART_X_FIFO_WRITE(numChars);
        }
        else if(param->fifo_stat == TRUE) {
            numChars = param->fifo_depth - uartCtrl_Get_TxFifoLevel(pUart);
            UART_X_FIFO_WRITE(numChars);
        }
        else {
            UART_FIFO_WRITE();
        }
        
        /* make sure THR/Tx FIFO empty interrupt is enabled*/
        instance->ier_save |= Uart_irq_etbei;
    }

    /* restore UART interrupts*/
    UART_EXIT_CRITICAL_SECTION();

    return retval;
}

/******************************************************************************
*
*  This function is used to receive date.
*
* @param 
*  pUart      -- FMSH_apb_uart handle
*  buffer   -- save address
*  length   -- number of perparing to receive 
*  userFunction   -- callback function
*
* @return
*  0        -- 
*  -FMSH_EBUSY     -- 
*
* @note    
*
******************************************************************************/
int uartCtrl_receive(vxT_UART * pUart, void *buffer, unsigned length, FMSH_callback userFunction)
{
    int retval;
    unsigned numChars;
    uartCtrl_Param_T *param;
    uartCtrl_Portmap_T *portmap;
    uartCtrl_Instance_T *instance;
    FMSH_callback userCallback;

    FMSH_REQUIRE(buffer != NULL);
    FMSH_REQUIRE(length != 0);

    param = &(pUart->comp_param);
    portmap = (uartCtrl_Portmap_T *) pUart->base_address;
    instance = &(pUart->instance);

    /* disable UART interrupts*/
    UART_ENTER_CRITICAL_SECTION();

    if((instance->state == Uart_state_rx)
            || (instance->state == Uart_state_tx_rx))
        retval = -FMSH_EBUSY;
    else {
        retval = 0;

    switch(instance->state) 
    {
            case Uart_state_tx:
            case Uart_state_tx_rx_req:
                instance->state = Uart_state_tx_rx;
                break;
            case Uart_state_idle:
            case Uart_state_rx_req:
                instance->state = Uart_state_rx;
                break;
            default:
                FMSH_ASSERT(FALSE);
                break;
     }

        /* check for buffer alignment*/
        if(((unsigned) buffer & 0x3) == 0)
            instance->rxAlign = TRUE;
        else
            instance->rxAlign = FALSE;

        /* update Rx buffer*/
        instance->rxBuffer = buffer;
        /* reset number of characters received for current transfer*/
        instance->rxLength = length;
        instance->rxRemain = length;
        instance->rxCallback = userFunction;
        instance->rxHold = 0;
        instance->rxIdx = 4;

        /* if the Rx buffer is 32-bit word-aligned*/
        if(instance->rxAlign == TRUE) {
            if(instance->fifos_enabled == FALSE) {
                UART_FIFO_READ();
            }
            else if(param->fifo_stat == TRUE) {
                numChars = uartCtrl_Get_RxFifoLevel(pUart);
                UART_X_FIFO_READ(numChars);
            }
            else {
                UART_FIFO_READ();
            }
        }

        /* if we've already filled the user Rx buffer*/
        if(instance->rxRemain == 0) {
            /* flush any byte in the hold register*/
            uartCtrl_flushRxHold(pUart);
            /* update state*/
            switch(instance->state) {
                case Uart_state_tx_rx:
                    instance->state = Uart_state_tx;
                    break;
                case Uart_state_rx:
                    instance->state = Uart_state_idle;
                    break;
                default:
                    FMSH_ASSERT(FALSE);
                    break;
            }
            userCallback = instance->rxCallback;
            instance->rxBuffer = NULL;
            instance->rxCallback = NULL;
            /* call callback functoin*/
            if(userCallback != NULL)
                (userCallback)(pUart, instance->rxLength);
        }
        else  {
            /* ensure receive is underway*/
            instance->ier_save |= Uart_irq_erbfi;
        }
    }

    /* restore UART interrupts*/
    UART_EXIT_CRITICAL_SECTION();

    return retval;
}

/**********************************************************************/
/***                    PRIVATE FUNCTIONS                           ***/
/**********************************************************************/

/****************************************************************************/
/**
*
*  Initializes the uart instance structure members.
*
* @param 
*  pUart      -- FMSH_apb_uart handle
*
* @return
* None
*
* @note    
*  Interrupt *must* be disabled before calling this function to avoid
*  shared data issues.
*
****************************************************************************/
void uartCtrl_Rst_Instance(vxT_UART * pUart)
{

    UART_COMMON_ASSERT(pUart);

    /* initialize instance variables*/
    pUart->pUartCtrl->tChn.state = Uart_state_idle;
    pUart->pUartCtrl->tChn.value_in_fcr = 0;
    pUart->pUartCtrl->tChn.lsr_save = 0;
    pUart->pUartCtrl->tChn.ier_save = 0;
    pUart->pUartCtrl->tChn.txCallback = NULL;
    pUart->pUartCtrl->tChn.rxCallback = NULL;
    pUart->pUartCtrl->tChn.callbackMode = Uart_buffer_full;
    pUart->pUartCtrl->tChn.listener = NULL;
    pUart->pUartCtrl->tChn.txBuffer = NULL;
    pUart->pUartCtrl->tChn.txHold = 0;
    pUart->pUartCtrl->tChn.txIdx = 0;
    pUart->pUartCtrl->tChn.txLength = 0;
    pUart->pUartCtrl->tChn.txRemain = 0;
    pUart->pUartCtrl->tChn.rxBuffer = NULL;
    pUart->pUartCtrl->tChn.rxHold = 0;
    pUart->pUartCtrl->tChn.rxIdx = 0;
    pUart->pUartCtrl->tChn.rxLength = 0;
    pUart->pUartCtrl->tChn.rxRemain = 0;
    pUart->pUartCtrl->tChn.rxAlign = FALSE;
    pUart->pUartCtrl->tChn.fifos_enabled = uartCtrl_Is_FifosEnabled(pUart);
    pUart->pUartCtrl->tChn.txTrigger = uartCtrl_Get_TxTrigger(pUart);
}

/****************************************************************************/
/**
*
*  Flushes any characters present in the Rx FIFO to the user Rx buffer.
*
* @param 
*  pUart      -- FMSH_apb_uart handle
*
* @return
*  0           -- if successful
*  -EPERM      -- no Rx transfer is currently in progress
*
* @note    
*  Interrupt *must* be disabled before calling this function to avoid
*  shared data issues.
*
****************************************************************************/
int uartCtrl_flushRxFifo(vxT_UART * pUart)
{
    int retval;
    unsigned numChars;

    /* if a user Rx buffer is available*/
    if (pUart->pUartCtrl->tChn.rxBuffer != NULL) 
    {
        retval = 0;

        if(pUart->pUartCtrl->tChn.fifos_enabled == FALSE)
        {
            UART_FIFO_READ();
        }
        else if(pUart->pUartCtrl->tParam.fifo_stat == TRUE) 
        {
            numChars = uartCtrl_Get_RxFifoLevel(pUart);
            UART_X_FIFO_READ(numChars);
        }
        else
        {
            UART_FIFO_READ();
        }

        /* flush the Rx 'hold' variable*/
        uartCtrl_flushRxHold(pUart);
    }
    else
        retval = -FMSH_EPERM;     /* permission denied (no Rx buffer set)*/

    return retval;
}

/****************************************************************************/
/**
*
*  Flushes any characters present in the pUart->pUartCtrl->tChn.rxHold variable to
*  the user Rx buffer.
*
* @param 
*  pUart      -- FMSH_apb_uart handle
*
* @return
* None
*
* @note    
*  Interrupt *must* be disabled before calling this function to avoid
*  shared data issues.
*
****************************************************************************/
void uartCtrl_flushRxHold(vxT_UART * pUart)
{
    int i;
    UINT8 *tmp;
    UINT32 rxMask, c;

    UART_COMMON_ASSERT(pUart);

    FMSH_ASSERT(pUart->pUartCtrl->tChn.rxIdx <= 4);
    /* if there is data in the rxHold variable*/
    if(pUart->pUartCtrl->tChn.rxIdx != 4)
    {
        /* Flush read buffer of data if last read in*/
        /* UART_[X_]FIFO_READ was not 4 complete bytes*/
        /* if there is greater than 4 bytes remaining in the Rx buffer*/
        if(pUart->pUartCtrl->tChn.rxRemain >= 4) 
        {
            /* safe to perform 32-bit writes*/
            rxMask = ((UINT32) (0xffffffff) >> (8 *
                    pUart->pUartCtrl->tChn.rxIdx));
            c = rxMask & (pUart->pUartCtrl->tChn.rxHold >> (8 *
                    pUart->pUartCtrl->tChn.rxIdx));
            * pUart->pUartCtrl->tChn.rxBuffer = (* pUart->pUartCtrl->tChn.rxBuffer & ~rxMask) | c;
        }
        else 
        {
            /* Need to write each byte individually to avoid writing*/
            /* past the end of the Rx buffer.*/
            /* tmp = next free location in Rx buffer*/
            tmp = (UINT8 *) pUart->pUartCtrl->tChn.rxBuffer;
            /* shift rxHold so that lsB contains valid data*/
            c = pUart->pUartCtrl->tChn.rxHold >> (8 * pUart->pUartCtrl->tChn.rxIdx);
            /* write out valid character to Rx buffer*/
            for(i = (4 - pUart->pUartCtrl->tChn.rxIdx); i > 0; i--) 
            {
                    *tmp++ = (UINT8) (c & 0xff);
                    c >>= 8;
            }
        }
    }
}

#endif


/****************************************************************************/
/**
*
*  This function attempts to automatically discover the component
*  hardware parameters, if possible.  If this is not supported, an
*  error code is returned.
*
* @param 
*  pUart      -- FMSH_apb_uart handle
*
* @return
*  0           if successful
*  -ENOSYS     -- auto-configuration is not supported
*
* @note    
*  This function is affected by the ADD_ENCODED_PARAMS hardware
*  parameter.
*  Interrupt *must* be disabled before calling this function to avoid
*  shared data issues.
*
****************************************************************************/
int uartCtrl_autoCompParams(vxT_UART * pUart)
{
    int retval;
    UINT32 comp_param;

    /* assume component parameters register is not available*/
    retval = -FMSH_ENOSYS;
    
    /* only version 3.00 and greater support component identification registers*/
    /*if (uartCtrl_Rd_CfgReg32(pUart, UART_REG_comp_type) == 0x44570110) */
    if (uartCtrl_Rd_CfgReg32(pUart, UART_REG_comp_type) == 0x44570110) 
    {
        pUart->pUartCtrl->devIP_ver = uartCtrl_Rd_CfgReg32(pUart, UART_REG_comp_version);
        pUart->pUartCtrl->devIP_ver = uartCtrl_Rd_CfgReg32(pUart, UART_REG_comp_type);
        
        /*comp_param = uartCtrl_Rd_CfgReg32(pUart, UART_REG_comp_param_1);*/
        comp_param = uartCtrl_Rd_CfgReg32(pUart, UART_REG_comp_param_1);
        
        if (comp_param != 0x0) 
        {
            retval = 0;
            pUart->pUartCtrl->tParam.afce_mode   = FMSH_BIT_GET(comp_param, UART_PARAM_AFCE_MODE); 
            
            pUart->pUartCtrl->tParam.fifo_access = FMSH_BIT_GET(comp_param, UART_PARAM_FIFO_ACCESS);
            pUart->pUartCtrl->tParam.fifo_depth  = FMSH_BIT_GET(comp_param, UART_PARAM_FIFO_MODE);
            
            pUart->pUartCtrl->tParam.fifo_depth *= 16;
            
            pUart->pUartCtrl->tParam.fifo_stat   = FMSH_BIT_GET(comp_param,  UART_PARAM_FIFO_STAT);
            pUart->pUartCtrl->tParam.new_feat    = FMSH_BIT_GET(comp_param,  UART_PARAM_NEW_FEAT);
            pUart->pUartCtrl->tParam.shadow      = FMSH_BIT_GET(comp_param,  UART_PARAM_SHADOW);
            pUart->pUartCtrl->tParam.sir_lp_mode = FMSH_BIT_GET(comp_param,  UART_PARAM_SIR_LP_MODE);
            pUart->pUartCtrl->tParam.sir_mode    = FMSH_BIT_GET(comp_param,  UART_PARAM_SIR_MODE);
            pUart->pUartCtrl->tParam.thre_mode   = FMSH_BIT_GET(comp_param,  UART_PARAM_THRE_MODE);
            pUart->pUartCtrl->tParam.dma_extra   = FMSH_BIT_GET(comp_param,  UART_PARAM_DMA_EXTRA);
        }
    }

    return retval;
}

#endif





#if 1

void uartSlcr_Enable_UartClk(void)
{
    return;
}

#endif



#if 1


int vxInit_Uart(int ctrl_x)
{
    vxT_UART * pUart = NULL;
    vxT_UART_CTRL * pUartCtrl = NULL;
    
    int ret = 0;    
    UINT32 tmp32 = 0;

    
#if 1 
    /*
    init the pUart structure
    */
    switch (ctrl_x)
    {
    case UART_CTRL_0:
        pUart = g_pUart0;    
        break;
        
    case UART_CTRL_1:
        pUart = g_pUart1;
        break;
    }
    
    if (pUart->init_flag == 1)
    {
        return 0;  /* init already and return*/
    }

    /**/
    /* spi_ctrl select*/
    /**/
    switch (ctrl_x)
    {
    case UART_CTRL_0:
        bzero(pUart, sizeof(vxT_UART));
        
        pUart->pUartCtrl = (vxT_UART_CTRL *)(&vxUart_Ctrl_0.ctrl_x);
        bzero(pUart->pUartCtrl, sizeof(vxT_UART_CTRL));

        pUart->uart_x = UART_CTRL_0;
        pUart->pUartCtrl->devId = UART_CTRL_0;
        pUart->pUartCtrl->cfgBaseAddr = VX_UART_0_CFG_BASE;
        pUart->pUartCtrl->input_clock = FPAR_UARTPS_0_UART_CLK_FREQ_HZ;
        break;
        
    case UART_CTRL_1:
        bzero(pUart, sizeof(vxT_UART));
    
        pUart->pUartCtrl = (vxT_UART_CTRL *)(&vxUart_Ctrl_1.ctrl_x);
        bzero(pUart->pUartCtrl, sizeof(vxT_UART_CTRL));
        
        pUart->uart_x = UART_CTRL_1;
        pUart->pUartCtrl->devId = UART_CTRL_1;
        pUart->pUartCtrl->cfgBaseAddr = VX_UART_1_CFG_BASE;
        pUart->pUartCtrl->input_clock = FPAR_UARTPS_1_UART_CLK_FREQ_HZ;
        break;
    }
    
    pUartCtrl = pUart->pUartCtrl;    
    
    pUartCtrl->ctrl_x = pUartCtrl->devId;
#endif    
    
    /* attempt to determine hardare parameters*/
    uartCtrl_autoCompParams(pUart);
    
    /* disable all uart interrupts*/
    uartCtrl_Disable_Irq(pUart, Uart_irq_all);
    
    /* reset device FIFOs*/
    uartCtrl_Rst_TxFifo(pUart);
    uartCtrl_Rst_RxFifo(pUart);
    
    uartCtrl_Set_BaudRate(pUart, 115200);

    /**/
    uartCtrl_Set_DataBits(pUart, 8);
    uartCtrl_Set_Parity(pUart, 1);
    uartCtrl_Set_StopBits(pUart, 1);

    /*line settings*/
    uartCtrl_Set_LineControl(pUart, Uart_line_8n1);
        
    uartCtrl_Enable_Fifos(pUart);
    
    pUart->init_flag = 1;  /* init ok*/
    
    return;
}

int uartCtrl_Tx_Poll(vxT_UART* pUart, UINT8* pData, int len)
{
    int timeout = 0x10000;
    int i = 0;
    
    for (i=0; i<len; i++) 
    {
        uartCtrl_write(pUart, pData[i]);   /* ABCDE FGHIJ KLMNO P*/
        
        /*while (TRUE != uartCtrl_Is_TxFifoEmpty(pUart));*/
    }

    if (timeout <= 0)
    {
        return ERROR;
    }
    else
    {
        return OK;
    }
}

int uartCtrl_Rx_Poll(vxT_UART* pUart, UINT8 * pRxBuf)
{
    return 0;
}


int uartCtrl_SLCR_Loop(UINT8 * tx_frm, UINT8 * rx_buf)
{
    vxT_UART* pUart0 = g_pUart0;
    vxT_UART* pUart1 = g_pUart1;    

    return;
}


#endif


#if 1

int g_test_uart1 = 0;


/* 
*/
void test_uart1_snd(void)
{
    vxT_UART* pUart1 = g_pUart1;    
    
    int i = 0;
    
    UINT8 tx_buf[16] = {0};
    UINT8 rx_buf[16] = {0};

    /**/
    /* init uart_1*/
    /**/
    vxInit_Uart(UART_CTRL_1);
        
    for (i=0; i<8; i++)
    {        
        tx_buf[i] = g_test_uart1 + i;
    }
    g_test_uart1++;

    uartCtrl_Tx_Poll(pUart1, (UINT8*)(&tx_buf[0]), 8);
    
    uartCtrl_Tx_Poll(pUart1, "\r\n", 2);  /* \r */
    
    return;
}

#endif




