/* sysGpio.c - support routines for FMSH FMQL GPIO */

/*
 * Copyright (c) 2013 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify, or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01a,25apr13,swu  Written.
01a,20aug19,l_l   modify it for FMQL Soc.
*/

/*
DESCRIPTION

This library provides support routines for FMSH FMQL GPIO.
refer to the Altera Soc Gen 5 GPIO.

A GPIO is a software controlled digital signal which represents a bit
connected to a particular pin. Logically, GPIOs are identified by unsigned
integers from 0 to the max GPIO number of a specified board supported.
GPIOs are commonly grouped in banks and each bank have several GPIO
pins together. On Altera Soc Gen 5 board, there are 3 banks of GPIOs,
and each bank have 29 GPIO pins which makes a total number of 87 GPIOs.
The board only used GPIO0 - GPIO70 as I/O pins and 14 as input-only pins.

\h Features

This library provides generic GPIO features includes:

- Allocation and free

A GPIO must be allocated before use. Allocation means the GPIO is used as a
specified function and can not be used by other functions until it is freed.
A terminated string describing the GPIO usage can be provided by the caller
to track its function.

- Direction selection and value getting/setting

A GPIO can be used as input or output. Input is readable and output is writable,
and the gpio value can be level high (GPIO_VALUE_HIGH) or level low
(GPIO_VALUE_LOW). The GPIO direction must be selected before further operations.
Example: value getting and setting, interrupt connection, etc.

- Interrupt handling

When a GPIO is selected as input, it can also be used as an IRQ signal. All
GPIOs within a bank share the interrupt of the bank and check the IRQ status of
the bank can identify which one triggered it. A generic ISR is connected to the
GPIO bank interrupt if any GPIO within the bank want to use the interrupt
function. A user ISR can be chained to the generic ISR and be called by it when
the interrupt source is identified.

Before interrupt can be used, the GPIO interrupt trigger mode must be set. The
supported modes are: GPIO_IRQ_LOW_LEVEL_SENSITIVE, GPIO_IRQ_HIGH_LEVEL_SENSITIVE
, GPIO_IRQ_RISING_EDGE_SENSITIVE and GPIO_IRQ_FALLING_EDGE_SENSITIVE.
The interrupt must be connected then enabled and vice versa, it must be disabled
then disconnected.

- GPIO debounce

The GPIO has inner glitch issues, if no debounce handling, the GPIO value may
not be right for input handling. This library enabled the debounce feature by
default if the GPIO is used as interrupt.

\h Calling sequences

Typical calling sequences include:

    /@ used as input @/

    sysGpioAlloc (...)
    sysGpioSelectInput (...)
    sysGpioGetValue (...)

    sysGpioFree (...)

    /@ used as input and interrupt @/

    sysGpioAlloc (...)
    sysGpioSelectInput (...)
    sysGpioIntConnect (...)
    sysGpioIntEnable (...)
    sysGpioGetValue (...)

    sysGpioIntDisable (...)
    sysGpioIntDisconnect (...)
    sysGpioFree (...)

    /@ used as output @/

    sysGpioAlloc (...)
    sysGpioSelectOutput (...)
    sysGpioSetValue (...)

    sysGpioFree (...)

\h Notes

Normally, the GPIO pin is multiplexed, the user of this library should ensure
the right multiplexing.

To add this library to the vxWorks image, add the following component to the
kernel configuration, or define the following macro in config.h.

\cs
vxprj component add INCLUDE_FMSH_FMQL_GPIO
\ce

\cs
#define INCLUDE_FMSH_FMQL_GPIO
\ce

INCLUDE FILES: sysGpio.h
*/

/* includes */

#include <vxWorks.h>
#include <spinLockLib.h>
#include <intLib.h>
#include <stdio.h>
#include <iosLib.h>

#include "./../../../fmsh_fmql.h"
#include "sysGpio.h"

/* definitions */
#define GPIO_DEBUG 1
#define GPIO_DEBUG_OUT(flag, ...)      \
	do {						   \
		if(flag) {					   \
		printf(__VA_ARGS__);   \
		}						   \
	} while(0)

#undef ARMA9CTX_REGISTER_READ
#undef ARMA9CTX_REGISTER_WRITE
#define ARMA9CTX_REGISTER_READ(reg)   (*(volatile UINT32 *)(reg))
#define ARMA9CTX_REGISTER_WRITE(reg, data) *(volatile UINT32 *)(reg) = (data)

#define SYS_GPIO_WRITE32                ARMA9CTX_REGISTER_WRITE
#define SYS_GPIO_READ32                 ARMA9CTX_REGISTER_READ

/*
Bank A - gpio[ 0:31] -- MIO  gpio number 32
Bank B - gpio[32:53] -- MIO  gpio number 22

Bank C - gpio[54:85] --  EMIO  gpio number 32
Bank D - gpio[86:117] -- EMIO  gpio number 32

GPIO0	4KB	0xE000_3000 ~ 0xE000_3FFF
---------------
Bank_x	offset:
---------------
Bank A  0x000
Bank B  0x100
Bank C  0x200
Bank D  0x400

IRQ:
49	SPI	GPIO0	
68	SPI	GPIO1		
69	SPI	GPIO2		
70	SPI	GPIO3		
*/
#define ALT_GPIO_BANK_NUM               (4)

#define ALT_GPIO_BANK_WIDTH              (32)  /* the max value of gpio bank width */
#define GPIO_USER_DATA_WIDTH             (32)

#define ALT_GPIO0_BANK_WIDTH             (32)
#define ALT_GPIO1_BANK_WIDTH             (22)
#define ALT_GPIO2_BANK_WIDTH             (32)
#define ALT_GPIO3_BANK_WIDTH             (32)

/* register offsets */

#define SWPORTA_DR_OFFSET               (0x00)
#define SWPORTA_DDR_OFFSET              (0x04)
#define INTEN_OFFSET                    (0x30)
#define INTMASK_OFFSET                  (0x34)
#define INTTYPELEVEL_OFFSET             (0x38)
#define INTPOLARITY_OFFSET              (0x3C)
#define INTSTATUS_OFFSET                (0x40)
#define RAWINTSTATUS_OFFSET             (0x44)
#define DEBOUNCE_OFFSET                 (0x48)
#define EOI_OFFSET                      (0x4C)
#define EXTPORTA_OFFSET                 (0x50)
#define LSSYNC_OFFSET                   (0x60)
#define IDCODE_OFFSET                   (0x64)
#define COMPVERSION_OFFSET              (0x6C)
#define CONFIGREG1_OFFSET               (0x70)
#define CONFIGREG2_OFFSET               (0x74)

#define GPIO_INTERRUPT_MASK             (0x1FFFFFFF)

/* typdefs */

struct gpioIsrData
    {
    FUNCPTR func;
    void *  arg;
    };

typedef struct gpioBank
    {
    UINT32              base;
    spinlockIsr_t       lock;
    atomicVal_t         pinUsed;
    atomicVal_t         isrInstalled;
    UINT32              vec;
    UINT32              ngpio; /* gpio number */
    UINT16              trigger[ALT_GPIO_BANK_WIDTH];
    char *              usage[GPIO_USER_DATA_WIDTH];
    struct gpioIsrData  isr[GPIO_USER_DATA_WIDTH];
    } GPIO_BANK;

LOCAL GPIO_BANK  gpioBank[ALT_GPIO_BANK_NUM];

/*
(1) Bank A 控制MIO 引脚[31:0];
(2) Bank B 控制MIO引脚[53:32];         // 54 GPIO
(3) Bank C 控制EMIO引脚信号[31:0]；
(4) Bank D 控制EMIO引脚信号[63:32]；

bankA: 0   ~ 31  (32)
bankB: 32  ~ 53  (22)
bankC: 54  ~ 85  (32)
bankD: 86  ~ 117 (32)
*/
#define SYS_GPIO_TO_BANK(gpio)          ((gpio < 32) ? 0 : \
		                                 (gpio < 54) ? 1 : \
		                                 (gpio < 86) ? 2 : \
		                                 (gpio < 118) ? 3 : 0)
		                                 
#define SYS_GPIO_TO_PIN(gpio)           ((gpio < 32) ? ((gpio)) : \
                                         (gpio < 54) ? ((gpio) - 32) : \
                                         (gpio < 86) ? ((gpio) - 54) : \
                                         (gpio < 118) ? ((gpio) - 86) : 0)
                                         
#define SYS_BANK_TO_GPIO(bank, n)       ((bank == 0) ? (n) : \
		                                 (bank == 1) ? ((n) + 32) : \
		                                 (bank == 2) ? ((n) + 54) : \
		                                 (bank == 3) ? ((n) + 86) : 0)
		                                 
#define SYS_GPIO_BIT(gpio)              (1 << SYS_GPIO_TO_PIN(gpio))

#define SYS_GPIO_NUM                    (118)
#define SYS_GPIO_MAX_OUTPUT             (SYS_GPIO_NUM) /* GPIO0 - GPIO117 are I/O pins */
                
/* forward declarations */

_WRS_INLINE void sysGpioSetDir (GPIO_BANK * pGpioBank, UINT32 bit,
                                BOOL isInput);

LOCAL void sysGpioSetIrqTrigger (GPIO_BANK * pGpioBank, UINT32 bit,
                                 UINT32 trigger);
LOCAL GPIO_BANK * sysGpioCheckValid (UINT32 gpio);
LOCAL void sysGpioIsr (void * arg);
LOCAL STATUS sysGpioSysIrqConnect (GPIO_BANK * pGpioBank, UINT32 bit);
LOCAL void sysGpioSysIrqDisconnect (GPIO_BANK * pGpioBank, UINT32 bit);

/*******************************************************************************
*
* sysGpioIsValid - check a logical gpio number can be allocated
*
* This routine checks a logical gpio number can be allocated or not.
*
* RETURNS: TRUE, or FALSE if the logical gpio number is out of range
* or the gpio is already allocated by sysGpioAlloc().
*
* ERRNO: N/A
*/

BOOL sysGpioIsValid
    (
    UINT32 gpio
    )
    {
    GPIO_BANK * pGpioBank;
    UINT32      bit;

    if (gpio < SYS_GPIO_NUM)
        {
        bit = SYS_GPIO_BIT (gpio);
        pGpioBank = &gpioBank[SYS_GPIO_TO_BANK (gpio)];

        if ((vxAtomicGet (&pGpioBank->pinUsed) & bit) == 0)
            {
            return TRUE;
            }
        }

    return FALSE;
    }

/*******************************************************************************
*
* sysGpioAlloc - allocate gpio pin for a specific function
*
* This routine allocates a gpio pin for a specific function. Before calling any
* other routines provided by this gpio library, the user should call this
* routine to get a dedicated gpio pin for its usage. The input parameter <gpio>
* specifies which logical gpio number is going to be used and <usage> is a null
* terminated string describing the usage, this information will be used in show
* function (this parameter could be NULL). If this function returns ERROR, the
* other routines in this library should not be called.
*
* RETURNS: OK, or ERROR if the input parameter <gpio> is out of range or the
* gpio is already allocated.
*
* ERRNO: N/A
*
*/

STATUS sysGpioAlloc
    (
    UINT32       gpio,
    const char * usage
    )
    {
    GPIO_BANK * pGpioBank;
    UINT32      bit;

    if (!sysGpioIsValid (gpio))
        return ERROR;

    bit = SYS_GPIO_BIT (gpio);
    pGpioBank = &gpioBank[SYS_GPIO_TO_BANK (gpio)];
    pGpioBank->usage[SYS_GPIO_TO_PIN (gpio)] = (char *)usage;
    vxAtomicOr (&pGpioBank->pinUsed, bit);

    return OK;
    }

/*******************************************************************************
*
* sysGpioSelectInput - set gpio pin as input
*
* This routine sets gpio pin as input.
*
* RETURNS: OK, or ERROR if the input parameter is out of range.
*
* ERRNO: N/A
*
*/

STATUS sysGpioSelectInput
    (
    UINT32 gpio
    )
    {
    GPIO_BANK  * pGpioBank;
    UINT32       bit;

    pGpioBank = sysGpioCheckValid (gpio);
    if (pGpioBank == NULL)
        return ERROR;

    bit = SYS_GPIO_BIT (gpio);

    SPIN_LOCK_ISR_TAKE (&pGpioBank->lock);
    sysGpioSetDir (pGpioBank, bit, TRUE);
    SPIN_LOCK_ISR_GIVE (&pGpioBank->lock);

    return OK;
    }

/*******************************************************************************
*
* sysGpioSetValue - set output value for a gpio pin
*
* This routine sets output value for a gpio pin. The pin should be selected as
* output function by sysGpioSelectOutput().
*
* RETURNS: OK, or ERROR if the input parameter is out of range or the gpio pin
* is not selected as output.
*
* ERRNO: N/A
*
*/

STATUS sysGpioSetValue
    (
    UINT32 gpio,
    UINT32 val         /* GPIO_VALUE_LOW or GPIO_VALUE_HIGH */
    )
    {
    GPIO_BANK  * pGpioBank;
    UINT32       bit;
    UINT32       tmp;

    pGpioBank = sysGpioCheckValid (gpio);
    if (pGpioBank == NULL)
        return ERROR;

    /* check it's output */

    SPIN_LOCK_ISR_TAKE (&pGpioBank->lock);
    tmp = SYS_GPIO_READ32 (pGpioBank->base + SWPORTA_DDR_OFFSET);
    SPIN_LOCK_ISR_GIVE (&pGpioBank->lock);

    bit = SYS_GPIO_BIT (gpio);
    if ((tmp & bit) == 0)
        return ERROR;

    SPIN_LOCK_ISR_TAKE (&pGpioBank->lock);

    tmp = SYS_GPIO_READ32 (pGpioBank->base + SWPORTA_DR_OFFSET);
    if (val == GPIO_VALUE_LOW)
        {
        tmp &= ~bit;
        }
    else
        {
        tmp |= bit;
        }
    SYS_GPIO_WRITE32 (pGpioBank->base + SWPORTA_DR_OFFSET, tmp);

    SPIN_LOCK_ISR_GIVE (&pGpioBank->lock);

    return OK;
    }

/*******************************************************************************
*
* sysGpioGetValue - get value for a gpio pin
*
* This routine gets value for a gpio pin.
*
* RETURNS: gpio pin value, or GPIO_VALUE_INVALID if the input parameter is out
* of range.
*
* ERRNO: N/A
*
*/

UINT32 sysGpioGetValue
    (
    UINT32 gpio
    )
    {
    GPIO_BANK  * pGpioBank;
    UINT32       bit;
    UINT32       val;

    pGpioBank = sysGpioCheckValid (gpio);
    if (pGpioBank == NULL)
        return (GPIO_VALUE_INVALID);

    bit = SYS_GPIO_BIT (gpio);

    SPIN_LOCK_ISR_TAKE (&pGpioBank->lock);
    val = ((SYS_GPIO_READ32 (pGpioBank->base + EXTPORTA_OFFSET) & bit) != 0)
            ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW;
    SPIN_LOCK_ISR_GIVE (&pGpioBank->lock);

    return val;
    }

/*******************************************************************************
*
* sysGpioSelectOutput - set gpio pin as output
*
* This routine sets gpio pin as output.
*
* RETURNS: OK, or ERROR if the input parameter is out of range.
*
* ERRNO: N/A
*
*/

STATUS sysGpioSelectOutput
    (
    UINT32 gpio
    )
    {
    GPIO_BANK  * pGpioBank;
    UINT32       bit;

    /* check gpio can be output */

    if (gpio >= SYS_GPIO_MAX_OUTPUT)
        return ERROR;

    pGpioBank = sysGpioCheckValid (gpio);
    if (pGpioBank == NULL)
        return ERROR;

    bit = SYS_GPIO_BIT (gpio);

    SPIN_LOCK_ISR_TAKE (&pGpioBank->lock);
    sysGpioSetDir (pGpioBank, bit, FALSE);
    SPIN_LOCK_ISR_GIVE (&pGpioBank->lock);

    return OK;
    }

/*******************************************************************************
*
* sysGpioIntConnect - connect a user gpio interrupt handler
*
* This routine connects a user gpio interrupt handler.
*
* RETURNS: OK, or ERROR if the input parameters are out of range or the top
* level gpio ISR connection failed.
*
* ERRNO: N/A
*
*/

STATUS sysGpioIntConnect
    (
    UINT32      gpio,
    UINT32      triggerMode,
    FUNCPTR     func,
    void      * arg
    )
    {
    GPIO_BANK * pGpioBank;
    UINT32      pin;
    UINT32      bit;
    STATUS      ret;

    if (func == NULL)
        return ERROR;

    switch (triggerMode)
        {
        case GPIO_IRQ_LOW_LEVEL_SENSITIVE       :
        case GPIO_IRQ_HIGH_LEVEL_SENSITIVE      :
        case GPIO_IRQ_RISING_EDGE_SENSITIVE     :
        case GPIO_IRQ_FALLING_EDGE_SENSITIVE    :
            break;
        default:
            return ERROR;
        }

    pGpioBank = sysGpioCheckValid (gpio);
    if (pGpioBank == NULL)
        {
        return ERROR;
        }

    pin = SYS_GPIO_TO_PIN (gpio);
    bit = SYS_GPIO_BIT (gpio);

    SPIN_LOCK_ISR_TAKE (&pGpioBank->lock);

    if (pGpioBank->isr[pin].func == NULL)
        {
        pGpioBank->isr[pin].func = func;
        pGpioBank->isr[pin].arg  = arg;
        ret = OK;
        }
    else
        {
        ret = ERROR;
        }

    SPIN_LOCK_ISR_GIVE (&pGpioBank->lock);

    if (ret == ERROR)
        return ret;

    ret = sysGpioSysIrqConnect (pGpioBank, bit);
    if (ret == ERROR)
        return ret;

    SPIN_LOCK_ISR_TAKE (&pGpioBank->lock);

    pGpioBank->trigger[pin] = triggerMode;

    SPIN_LOCK_ISR_GIVE (&pGpioBank->lock);

    return OK;
    }

/*******************************************************************************
*
* sysGpioIntEnable - enable a gpio interrupt
*
* This routine enables a gpio pin interrupt.
*
* RETURNS: OK, or ERROR if the input parameter is out of range.
*
* ERRNO: N/A
*
*/

STATUS sysGpioIntEnable
    (
    UINT32      gpio
    )
    {
    GPIO_BANK * pGpioBank;
    UINT32      bit;
    UINT32      pin;
    UINT32      tmp;

    pGpioBank = sysGpioCheckValid (gpio);
    if (pGpioBank == NULL)
        {
        return ERROR;
        }

    bit = SYS_GPIO_BIT (gpio);
    pin = SYS_GPIO_TO_PIN (gpio);

    /* sysGpioIntConnect() should be called first */

    if (vxAtomicGet (&pGpioBank->isrInstalled) == 0)
        {
        return ERROR;
        }

    SPIN_LOCK_ISR_TAKE (&pGpioBank->lock);

    sysGpioSetIrqTrigger (pGpioBank, bit, pGpioBank->trigger[pin]);

    tmp = SYS_GPIO_READ32 (pGpioBank->base + DEBOUNCE_OFFSET);
    tmp |= bit;
    SYS_GPIO_WRITE32 (pGpioBank->base + DEBOUNCE_OFFSET, tmp);
	
    tmp = SYS_GPIO_READ32 (pGpioBank->base + INTMASK_OFFSET);
    tmp &= ~bit;
    SYS_GPIO_WRITE32 (pGpioBank->base + INTMASK_OFFSET, tmp);
	
    tmp = SYS_GPIO_READ32 (pGpioBank->base + INTEN_OFFSET);
    tmp |= bit;
    SYS_GPIO_WRITE32 (pGpioBank->base + INTEN_OFFSET, tmp);

    SPIN_LOCK_ISR_GIVE (&pGpioBank->lock);

    return OK;
    }

/*******************************************************************************
*
* sysGpioIntDisable - disable a gpio interrupt
*
* This routine disables a gpio pin interrupt.
*
* RETURNS: OK, or ERROR if the input parameter is out of range.
*
* ERRNO: N/A
*
*/

STATUS sysGpioIntDisable
    (
    UINT32      gpio
    )
    {
    GPIO_BANK * pGpioBank;
    UINT32      bit;
    UINT32      pin;
    UINT32      tmp;

    pGpioBank = sysGpioCheckValid (gpio);
    if (pGpioBank == NULL)
        {
        return ERROR;
        }

    bit = SYS_GPIO_BIT (gpio);
    pin = SYS_GPIO_TO_PIN (gpio);

    SPIN_LOCK_ISR_TAKE (&pGpioBank->lock);

    tmp = SYS_GPIO_READ32(pGpioBank->base + INTEN_OFFSET);
    tmp &= ~bit;
    SYS_GPIO_WRITE32 (pGpioBank->base + INTEN_OFFSET, tmp);
    tmp = SYS_GPIO_READ32(pGpioBank->base + INTMASK_OFFSET);
    tmp |= bit;
    SYS_GPIO_WRITE32 (pGpioBank->base + INTMASK_OFFSET, tmp);
    tmp = SYS_GPIO_READ32(pGpioBank->base + DEBOUNCE_OFFSET);
    tmp &= ~bit;
    SYS_GPIO_WRITE32 (pGpioBank->base + DEBOUNCE_OFFSET, tmp);
    sysGpioSetIrqTrigger (pGpioBank, bit, GPIO_IRQ_SENSITIVE_NONE);

    SPIN_LOCK_ISR_GIVE (&pGpioBank->lock);

    return OK;
    }

/*******************************************************************************
*
* sysGpioIntDisconnect - disconnect a user gpio interrupt handler
*
* This routine disconnects a user gpio interrupt handler.
*
* RETURNS: OK, or ERROR if the input parameters are out of range.
*
* ERRNO: N/A
*
*/

STATUS sysGpioIntDisconnect
    (
    UINT32      gpio
    )
    {
    GPIO_BANK * pGpioBank;
    UINT32      pin;
    UINT32      bit;
    STATUS      ret;

    pGpioBank = sysGpioCheckValid (gpio);
    if (pGpioBank == NULL)
        {
        return ERROR;
        }

    pin = SYS_GPIO_TO_PIN (gpio);
    bit = SYS_GPIO_BIT (gpio);

    SPIN_LOCK_ISR_TAKE (&pGpioBank->lock);

    pGpioBank->trigger[pin] = GPIO_IRQ_SENSITIVE_NONE;

    if (pGpioBank->isr[pin].func != NULL)
        {
        pGpioBank->isr[pin].func = NULL;
        pGpioBank->isr[pin].arg  = NULL;
        ret = OK;
        }
    else
        {
        ret = ERROR;
        }

    SPIN_LOCK_ISR_GIVE (&pGpioBank->lock);

    if (ret == ERROR)
        return ret;

    sysGpioSysIrqDisconnect (pGpioBank, bit);

    return OK;
    }

/*******************************************************************************
*
* sysGpioFree - release gpio pin for a specific function
*
* This routine releases the gpio pin previously allocated by sysGpioAlloc().
*
* RETURNS: OK, or ERROR if the input parameter <gpio> is out of range or the
* gpio is not allocated at all.
*
* ERRNO: N/A
*
*/

STATUS sysGpioFree
    (
    UINT32       gpio
    )
    {
    GPIO_BANK * pGpioBank;
    UINT32      bit;

    pGpioBank = sysGpioCheckValid (gpio);
    if (pGpioBank == NULL)
        {
        return ERROR;
        }

    bit = SYS_GPIO_BIT (gpio);
    pGpioBank->usage[SYS_GPIO_TO_PIN (gpio)] = NULL;

    vxAtomicXor (&pGpioBank->pinUsed, bit);

    return OK;
    }

/*******************************************************************************
*
* sysGpioCheckValid - check validation of a logical gpio number
*
* This routine checks the validation of a logical gpio number. If not valid, the
* other gpio routines can not be called.
*
* RETURNS: gpio bank pointer, or NULL if the logical gpio number is out of range
* or the gpio is not allocated by sysGpioAlloc().
*
* ERRNO: N/A
*/

LOCAL GPIO_BANK * sysGpioCheckValid
    (
    UINT32 gpio
    )
    {
    GPIO_BANK * pGpioBank;
    UINT32      bit;

    if (gpio < SYS_GPIO_NUM)
        {
        bit = SYS_GPIO_BIT (gpio);
        pGpioBank = &gpioBank[SYS_GPIO_TO_BANK (gpio)];

        if ((vxAtomicGet (&pGpioBank->pinUsed) & bit) != 0)
            {
            return pGpioBank;
            }
        }

    return NULL;
    }

/*******************************************************************************
*
* sysGpioInit - initialize GPIO modules
*
* This routine initializes GPIO modules.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void sysGpioInit (void)
{
    int i;
    GPIO_BANK * pGpioBank;

    for (i = 0; i < ALT_GPIO_BANK_NUM; i++)
    {
        pGpioBank = &gpioBank[i];
        
		switch (i)
		{
		case 0:
			pGpioBank->base = GPIO_0_BASE_ADRS;
        	pGpioBank->vec = INT_VEC_GPIO_BANK_A;
        	pGpioBank->ngpio = ALT_GPIO0_BANK_WIDTH;
			break;
		case 1:
			pGpioBank->base = GPIO_1_BASE_ADRS;
        	pGpioBank->vec = INT_VEC_GPIO_BANK_B;
        	pGpioBank->ngpio = ALT_GPIO1_BANK_WIDTH;
			break;
		case 2:
			pGpioBank->base = GPIO_2_BASE_ADRS;
        	pGpioBank->vec = INT_VEC_GPIO_BANK_C;
        	pGpioBank->ngpio = ALT_GPIO2_BANK_WIDTH;
			break;
		case 3:
			pGpioBank->base = GPIO_3_BASE_ADRS;
        	pGpioBank->vec = INT_VEC_GPIO_BANK_D;
        	pGpioBank->ngpio = ALT_GPIO3_BANK_WIDTH;
			break;
		}
		
        vxAtomicClear (&pGpioBank->pinUsed);
        vxAtomicClear (&pGpioBank->isrInstalled);
        SPIN_LOCK_ISR_INIT (&pGpioBank->lock, 0);
		
        memset (pGpioBank->trigger, 0, sizeof (pGpioBank->trigger));
        memset (pGpioBank->usage, 0, sizeof (char *) * GPIO_USER_DATA_WIDTH);
        memset (pGpioBank->isr, 0, sizeof (pGpioBank->isr));

        /* init registers */
        SYS_GPIO_WRITE32 (pGpioBank->base + INTEN_OFFSET, 0);
        SYS_GPIO_WRITE32 (pGpioBank->base + INTMASK_OFFSET, GPIO_INTERRUPT_MASK);
        SYS_GPIO_WRITE32 (pGpioBank->base + INTTYPELEVEL_OFFSET, 0);
        SYS_GPIO_WRITE32 (pGpioBank->base + INTPOLARITY_OFFSET, 0);
        SYS_GPIO_WRITE32 (pGpioBank->base + INTSTATUS_OFFSET, 0);
        SYS_GPIO_WRITE32 (pGpioBank->base + DEBOUNCE_OFFSET, 0);
    }

    /* set gpio_db_clk to let debounce 5ms */
    /* jc
    ARMA9CTX_REGISTER_WRITE (GPIO_DIV_ADRS, 200000000 / 200);
    */
}

/*******************************************************************************
*
* sysGpioIsr - top level interrupt handler for gpio interrupts
*
* This routine is called by the interrupt controller in response to a GPIO
* interrupt. It executes the appropriate callback ISRs for the GPIO pin which
* generated the interrupt.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void sysGpioIsr
    (
    void *      arg
    )
    {
    GPIO_BANK * pGpioBank;
    UINT32      tmp;
    UINT32      i;
    UINT32      mask = 0;

    pGpioBank = (GPIO_BANK *)arg;

    SPIN_LOCK_ISR_TAKE (&pGpioBank->lock);
    tmp = SYS_GPIO_READ32 (pGpioBank->base + INTSTATUS_OFFSET);

    if (tmp == 0)
        {
        SPIN_LOCK_ISR_GIVE (&pGpioBank->lock);
        return;
        }

    /* save current mask status */

    mask = SYS_GPIO_READ32 (pGpioBank->base + INTMASK_OFFSET);

    /* mask all gpio pin in this bank */

    SYS_GPIO_WRITE32 (pGpioBank->base + INTMASK_OFFSET, GPIO_INTERRUPT_MASK);

    SPIN_LOCK_ISR_GIVE (&pGpioBank->lock);

    for (i = 0; i < pGpioBank->ngpio; i++)
        {
        if ((tmp & (1 << i)) != 0)
            {
            if (pGpioBank->isr[i].func != NULL)
                pGpioBank->isr[i].func (pGpioBank->isr[i].arg);

            SPIN_LOCK_ISR_TAKE (&pGpioBank->lock);
            SYS_GPIO_WRITE32 (pGpioBank->base + EOI_OFFSET, (1 << i));
            SPIN_LOCK_ISR_GIVE (&pGpioBank->lock);
            }
        }

    /* restore interrupt mask */

    SPIN_LOCK_ISR_TAKE (&pGpioBank->lock);
    SYS_GPIO_WRITE32 (pGpioBank->base + INTMASK_OFFSET, mask);
    SPIN_LOCK_ISR_GIVE (&pGpioBank->lock);

    }

/*******************************************************************************
*
* sysGpioSysIrqConnect - connect top level GPIO ISR
*
* This routine connects top level GPIO ISR to the gpio bank interrupt.
*
* RETURNS: OK, or ERROR if the interrupt connection failed.
*
* ERRNO: N/A
*/

LOCAL STATUS sysGpioSysIrqConnect
    (
    GPIO_BANK * pGpioBank,
    UINT32      bit
    )
    {
    STATUS ret;

    /* already connected */

    if (vxAtomicGet (&pGpioBank->isrInstalled) != 0)
        {
        vxAtomicOr (&pGpioBank->isrInstalled, bit);
        return OK;
        }

    ret = intConnect (INUM_TO_IVEC (pGpioBank->vec),  sysGpioIsr, (int)pGpioBank);
    if (ret != OK)
        return ret;

    ret = intEnable (pGpioBank->vec);
    if (ret != OK)
        return ret;

    vxAtomicOr (&pGpioBank->isrInstalled, bit);

    return OK;
    }

/*******************************************************************************
*
* sysGpioSysIrqDisconnect - disconnect top level GPIO ISR
*
* This routine disconnects top level GPIO ISR from the gpio bank interrupt.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void sysGpioSysIrqDisconnect
    (
    GPIO_BANK * pGpioBank,
    UINT32      bit
    )
    {
    atomicVal_t      tmpVal;

    tmpVal = vxAtomicGet (&pGpioBank->isrInstalled);
    if ((tmpVal == 0) || ((tmpVal & bit) == 0))
        {
        return;
        }

    vxAtomicXor (&pGpioBank->isrInstalled, bit);

    if (vxAtomicGet (&pGpioBank->isrInstalled) == 0)
        {
        intDisable (pGpioBank->vec);
        intDisconnect (INUM_TO_IVEC (pGpioBank->vec),
                       sysGpioIsr, (int)pGpioBank);
        }

    }

/*******************************************************************************
*
* sysGpioSetDir - set gpio pin direction
*
* This routine sets gpio pin direction: input or output. If <isInput> is TRUE
* then the pin will be set as input, otherwise the pin will be set to ouput.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
*/

_WRS_INLINE void sysGpioSetDir
    (
    GPIO_BANK  * pGpioBank,
    UINT32       bit,
    BOOL         isInput
    )
    {
    UINT32 val;

    val = SYS_GPIO_READ32 (pGpioBank->base + SWPORTA_DDR_OFFSET);
    if (!isInput)
        val |= bit;
    else
        val &= ~bit;
    SYS_GPIO_WRITE32 (pGpioBank->base + SWPORTA_DDR_OFFSET, val);
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

LOCAL void sysGpioSetIrqTrigger
    (
    GPIO_BANK * pGpioBank,
    UINT32      bit,
    UINT32      trigger
    )
    {
    UINT32      intEdgeLevel = 0;
    UINT32      intRiseFall = 0;

    intEdgeLevel = SYS_GPIO_READ32 (pGpioBank->base + INTTYPELEVEL_OFFSET);
    intRiseFall  = SYS_GPIO_READ32 (pGpioBank->base + INTPOLARITY_OFFSET);

    switch (trigger)
        {
        case GPIO_IRQ_LOW_LEVEL_SENSITIVE :
        case GPIO_IRQ_SENSITIVE_NONE :
            intEdgeLevel &= ~bit;
            intRiseFall  &= ~bit;
            break;
        case GPIO_IRQ_HIGH_LEVEL_SENSITIVE :
            intEdgeLevel &= ~bit;
            intRiseFall  |= bit;
            break;
        case GPIO_IRQ_RISING_EDGE_SENSITIVE :
            intEdgeLevel |= bit;
            intRiseFall  |= bit;
            break;
        case GPIO_IRQ_FALLING_EDGE_SENSITIVE :
            intEdgeLevel |= bit;
            intRiseFall  &= ~bit;
            break;
        default:
            return;
        }

    SYS_GPIO_WRITE32 (pGpioBank->base + INTTYPELEVEL_OFFSET, intEdgeLevel);
    SYS_GPIO_WRITE32 (pGpioBank->base + INTPOLARITY_OFFSET, intRiseFall);
    }

/*******************************************************************************
*
* sysGpioBankShow - show all gpio information on a gpio bank
*
* This routine shows all gpio information on a gpio bank.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
*/

void sysGpioBankShow
    (
    UINT32 bank         /* gpio bank number from 0 */
    )
    {
    GPIO_BANK * pGpioBank;
    atomicVal_t pinUsed;
    UINT32      tmp;
    UINT32      intEn;
    UINT32      intMask;
    UINT32      ddr;
    UINT32      debounce;
    int         i;
    int         gpio;

    if (bank >= ALT_GPIO_BANK_NUM)
        {
        printf ("The gpio bank should be 0 - %d\n", ALT_GPIO_BANK_NUM - 1);
        return;
        }

    pGpioBank = &gpioBank[bank];
    pinUsed = vxAtomicGet (&pGpioBank->pinUsed);

    SPIN_LOCK_ISR_TAKE (&pGpioBank->lock);
    ddr      = SYS_GPIO_READ32 (pGpioBank->base + SWPORTA_DDR_OFFSET);
    intEn    = SYS_GPIO_READ32 (pGpioBank->base + INTEN_OFFSET);
    intMask  = SYS_GPIO_READ32 (pGpioBank->base + INTMASK_OFFSET);
    debounce = SYS_GPIO_READ32 (pGpioBank->base + DEBOUNCE_OFFSET);
    SPIN_LOCK_ISR_GIVE (&pGpioBank->lock);

    for (i = 0; i < pGpioBank->ngpio; i++)
        {
    	gpio = SYS_BANK_TO_GPIO(bank, i);
        if ((pinUsed & (1 << i)) != 0)
            {
            printf ("\nGpio %d (bank %d pin %d) is allocated\n", gpio, bank, i);
            if (pGpioBank->usage[i] != NULL)
                printf ("Used for:\t\t%s\n", pGpioBank->usage[i]);

            if ((ddr & (1 << i)) == 0)
                printf ("Direction:\t\tInput\n");
            else
                printf ("Direction:\t\tOutput\n");

            printf ("Trigger mode:\t\t");
            switch (pGpioBank->trigger[i])
                {
                case GPIO_IRQ_LOW_LEVEL_SENSITIVE :
                    printf ("Low level\n");
                    break;
                case GPIO_IRQ_HIGH_LEVEL_SENSITIVE :
                    printf ("High level\n");
                    break;
                case GPIO_IRQ_RISING_EDGE_SENSITIVE :
                    printf ("Rising edge\n");
                    break;
                case GPIO_IRQ_FALLING_EDGE_SENSITIVE :
                    printf ("Falling edge\n");
                    break;
                default :
                    printf ("None\n");
                    break;
                }

            if (pGpioBank->isr[i].func != NULL)
                printf ("User ISR:\t\tfunc=%p, arg=0x%x\n",
                        pGpioBank->isr[i].func, pGpioBank->isr[i].arg);

            if ((intEn & (1 << i)) != 0)
                printf ("Irq:\t\t\tEnabled and ");
            else
                printf ("Irq:\t\t\tDisabled and ");

            if ((intMask & (1 << i)) != 0)
                printf ("Masked\n");
            else
                printf ("Unmasked\n");

            if ((debounce & (1 << i)) != 0)
                printf ("Debounce:\t\tEnabled\n");
            else
                printf ("Debounce:\t\tDisabled\n");

            tmp = sysGpioGetValue (gpio);
            printf ("Gpio value:\t\t");
            if (tmp == GPIO_VALUE_HIGH)
                printf ("Level high\n");
            else
                printf ("Level low\n");
            }
        }

    }

/*******************************************************************************
*
* sysGpioShow - show all gpio information
*
* This routine shows all gpio information.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
*/

void sysGpioShow (void)
    {
    int i;

    for (i = 0; i < ALT_GPIO_BANK_NUM; i++)
        sysGpioBankShow (i);
    }

/*************  gpio  ios driver      ******************/   

typedef struct _gpio_dev{
	DEV_HDR pDevHdr;
	UINT32 	gpioRegBase;
	UINT8 	channel;
	UINT8 	flags;
}GPIO_DEV;

LOCAL int GpioDrvNum=-1;

/******* gpio driver function ******/

/************
 * 	 create tobedone
 * 
 */
GPIO_DEV *GpioCreate(GPIO_DEV* pDevHdr,char* name,int flag,int mode){
	printf("gpio create\n");
	return pDevHdr;
}

/************
 * 	 remove tobedone
 * 
 */
STATUS GpioRemove(GPIO_DEV* pDevHdr,int flag,int mode){
	printf("gpio remove\n");
	return OK;
}

/************
 * 	 open tobedone
 * 
 */
GPIO_DEV *GpioOpen(GPIO_DEV* pDevHdr,char* name,int flag,int mode){
	printf("gpio open\n");
	pDevHdr->flags=flag;
	return pDevHdr;
}

/************
 * 	 close tobedone
 * 
 */
STATUS GpioClose(GPIO_DEV* pDevHdr,int flag,int mode){
	printf("gpio close\n");
	SYS_GPIO_WRITE32 (pDevHdr->gpioRegBase + SWPORTA_DDR_OFFSET, 0);
	return OK;
}

/************
 * 	 read tobedone
 * 
 */
int GpioRead(GPIO_DEV* pDevHdr,char *buffer,int nbytes){
	if(nbytes<4){
		GPIO_DEBUG_OUT(GPIO_DEBUG,"err:buffer length less than 4\n");
		return ERROR;
	}
	if(pDevHdr->flags==O_WRONLY){
		GPIO_DEBUG_OUT(GPIO_DEBUG,"err:gpio write only\n");
		return ERROR;
	}
	UINT32 val;
	SYS_GPIO_WRITE32 (pDevHdr->gpioRegBase + SWPORTA_DDR_OFFSET, 0);
    val = SYS_GPIO_READ32 (pDevHdr->gpioRegBase + EXTPORTA_OFFSET);
    buffer[3]=(char)((val&0xff000000)>>24);
    buffer[2]=(char)((val&0xff0000)>>16);
    buffer[1]=(char)((val&0xff00)>>8);
    buffer[0]=(char)(val&0xff);
	return 4;
}

/************
 * 	 write tobedone
 * 
 */
int GpioWrite(GPIO_DEV* pDevHdr,char *buffer,int nbytes){
	if(nbytes<4){
		GPIO_DEBUG_OUT(GPIO_DEBUG,"err:buffer length less than 4\n");
		return ERROR;
	}
	if(pDevHdr->flags==O_RDONLY){
		GPIO_DEBUG_OUT(GPIO_DEBUG,"err:gpio read only\n");
		return ERROR;
	}
	UINT32 val;
	val=((UINT32)buffer[3]<<24)|
			((UINT32)buffer[2]<<16)|
			((UINT32)buffer[1]<<8)|
			((UINT32)buffer[0]);
	SYS_GPIO_WRITE32 (pDevHdr->gpioRegBase + SWPORTA_DR_OFFSET, val);
	return 4;
}

/************
 * 	 GpioIoCtl tobedone
 * 
 */
int GpioIoCtl(GPIO_DEV *pDevHdr, int command, UINT32 arg){
	int status=OK;
	switch(command){
		case GPIO_DIR:
			GPIO_DEBUG_OUT(GPIO_DEBUG,"GpioIoctl dir cmd:0x%x\n",arg);
			SYS_GPIO_WRITE32 (pDevHdr->gpioRegBase + SWPORTA_DDR_OFFSET, arg);
			break;
		default:
			GPIO_DEBUG_OUT(GPIO_DEBUG,"err:GpioIoctl cmd not found\n");
			status=ERROR;
		break;
	}
	GPIO_DEBUG_OUT(GPIO_DEBUG,"GpioIoctl ok\n");
	return status;
}

/*****************************
 * 
 * 		gpio driver Register
 * 
 */
STATUS GpioDrv(){
	if(GpioDrvNum!=-1){
		GPIO_DEBUG_OUT(GPIO_DEBUG,"this dev driver has been registered\n");
		return OK;
	}
	GpioDrvNum=iosDrvInstall(
			GpioCreate,
			GpioRemove,
			GpioOpen,
			GpioClose,
			GpioRead,
			GpioWrite,
			GpioIoCtl);
	return (GpioDrvNum==ERROR?ERROR:OK);
}

/*******************************
 * 
 * 		gpio device create
 * 		channel : 0 1 2 3
 * 		
 */
char *GpioDevNamePrefix="/Gpio";
STATUS GpioDevCreate(int channel){
	GPIO_DEV *pGpioDev;
	char DevName[256];
	sprintf(DevName,"%s/%d",GpioDevNamePrefix,channel);
	pGpioDev=(GPIO_DEV*)malloc(sizeof(GPIO_DEV));
	bzero(pGpioDev,sizeof(GPIO_DEV));
	pGpioDev->channel=channel;
	switch(channel){
	case 0:pGpioDev->gpioRegBase=(UINT32)GPIO_0_BASE_ADRS;
		break;
	case 1:pGpioDev->gpioRegBase=(UINT32)GPIO_1_BASE_ADRS;
		break;
	case 2:pGpioDev->gpioRegBase=(UINT32)GPIO_2_BASE_ADRS;
		break;
	case 3:pGpioDev->gpioRegBase=(UINT32)GPIO_3_BASE_ADRS;
		break;
	}
	
	/* init func. none */
	/* add dev to list */
	if(iosDevAdd(&pGpioDev->pDevHdr,DevName,GpioDrvNum)==ERROR){
		free((char*)pGpioDev);
		GPIO_DEBUG_OUT(GPIO_DEBUG,"iosDevAdd fail\n");
		return ERROR;
	}
	GPIO_DEBUG_OUT(GPIO_DEBUG,"GpioDevCreate ok\n");
	return OK;
};


/***************** gpio test *******************/

/*#undef SYS_GPIO_TEST*/
#define SYS_GPIO_TEST

#ifdef SYS_GPIO_TEST

#if 1  /* new demo_board 201907 */

#define FMQL_GPIO_USER_LED1           88 /* D11 */
#define FMQL_GPIO_USER_LED2           92 /* D15 */
#define FMQL_GPIO_USER_LED3           86 /* D9 */
#define FMQL_GPIO_USER_LED4           89 /* D12 */

#else /* old demo board 201905 */

#define FMQL_GPIO_USER_LED1           54 /* D11 */
#define FMQL_GPIO_USER_LED2           55 /* D15 */
#define FMQL_GPIO_USER_LED3           56 /* D9 */
#define FMQL_GPIO_USER_LED4           57 /* D12 */
#endif

#define FMQL_GPIO_IRQ

#define FMQL_GPIO_USER_S1             54
#define FMQL_GPIO_USER_S2             55
#define FMQL_GPIO_USER_S3             56
#define FMQL_GPIO_USER_S4             57
#define FMQL_GPIO_USER_S5             58


#ifdef FMQL_GPIO_IRQ
/*******************************************************************************
 *
 * sysGpioIntTestIrq - GPIO test interrupt handler
 *
 * This routine handles GPIO interrupt event for testing.
 *
 * RETURNS: N/A
 *
 * ERRNO: N/A
 */

LOCAL void sysGpioIntTestIrq
    (
    int arg
    )
    {
    logMsg("User S%d Pushed \n", arg, 2, 3, 4, 5, 6);
    }

/*******************************************************************************
 *
 * sysGpioIntTestStart - start testing GPIO interrupt
 *
 * This routine starts testing GPIO interrupt for user switch S1 S2 S3 and S4.
 *
 * RETURNS: N/A
 *
 * ERRNO: N/A
 */

void sysGpioIntTestStart (void)
{
    sysGpioAlloc (FMQL_GPIO_USER_S1, "User S1");
    sysGpioAlloc (FMQL_GPIO_USER_S2, "User S2");
    sysGpioAlloc (FMQL_GPIO_USER_S3, "User S3");
    sysGpioAlloc (FMQL_GPIO_USER_S4, "User S4");
    sysGpioAlloc (FMQL_GPIO_USER_S5, "User S5");

    sysGpioSelectInput (FMQL_GPIO_USER_S1);
    sysGpioSelectInput (FMQL_GPIO_USER_S2);
    sysGpioSelectInput (FMQL_GPIO_USER_S3);
    sysGpioSelectInput (FMQL_GPIO_USER_S4);
    sysGpioSelectInput (FMQL_GPIO_USER_S5);

    sysGpioIntConnect (FMQL_GPIO_USER_S1, GPIO_IRQ_FALLING_EDGE_SENSITIVE,
                       (FUNCPTR)sysGpioIntTestIrq, (void *)1);
	
    sysGpioIntConnect (FMQL_GPIO_USER_S2, GPIO_IRQ_FALLING_EDGE_SENSITIVE,
                       (FUNCPTR)sysGpioIntTestIrq, (void *)2);
	
    sysGpioIntConnect (FMQL_GPIO_USER_S3, GPIO_IRQ_FALLING_EDGE_SENSITIVE,
                       (FUNCPTR)sysGpioIntTestIrq, (void *)3);
	
    sysGpioIntConnect (FMQL_GPIO_USER_S4, GPIO_IRQ_FALLING_EDGE_SENSITIVE,
                       (FUNCPTR)sysGpioIntTestIrq, (void *)4);
	
    sysGpioIntConnect (FMQL_GPIO_USER_S5, GPIO_IRQ_FALLING_EDGE_SENSITIVE,
                       (FUNCPTR)sysGpioIntTestIrq, (void *)5);
    
    sysGpioIntEnable (FMQL_GPIO_USER_S1);
    sysGpioIntEnable (FMQL_GPIO_USER_S2);
    sysGpioIntEnable (FMQL_GPIO_USER_S3);
    sysGpioIntEnable (FMQL_GPIO_USER_S4);
    sysGpioIntEnable (FMQL_GPIO_USER_S5);

    printf ("Press any user switch to begin GPIO interrupt test!\n");
 }

/*******************************************************************************
 *
 * sysGpioIntTestStop - stop testing of GPIO interrupt
 *
 * This routine stops testing GPIO interrupt for user switch S1 S2 S3 and S4.
 *
 * RETURNS: N/A
 *
 * ERRNO: N/A
 */

void sysGpioIntTestStop (void)
    {
    sysGpioIntDisable (FMQL_GPIO_USER_S1);
    sysGpioIntDisable (FMQL_GPIO_USER_S2);
    sysGpioIntDisable (FMQL_GPIO_USER_S3);
    sysGpioIntDisable (FMQL_GPIO_USER_S4);
    sysGpioIntDisable (FMQL_GPIO_USER_S5);

    sysGpioIntDisconnect (FMQL_GPIO_USER_S1);
    sysGpioIntDisconnect (FMQL_GPIO_USER_S2);
    sysGpioIntDisconnect (FMQL_GPIO_USER_S3);
    sysGpioIntDisconnect (FMQL_GPIO_USER_S4);
    sysGpioIntDisconnect (FMQL_GPIO_USER_S5);

    sysGpioFree (FMQL_GPIO_USER_S1);
    sysGpioFree (FMQL_GPIO_USER_S2);
    sysGpioFree (FMQL_GPIO_USER_S3);
    sysGpioFree (FMQL_GPIO_USER_S4);
    sysGpioFree (FMQL_GPIO_USER_S5);

    printf ("GPIO interrupt test end!\n");

	return;
}

#endif


/*******************************************************************************
 *
 * sysGpioLedTest - test user LEDs
 *
 * This routine tests user D1 D2 D3 and D4 LEDs.
 *
 * RETURNS: N/A
 *
 * ERRNO: N/A
 */

void sysGpioLedTest
    (
    void
    )
{
    int i;

    sysGpioAlloc (FMQL_GPIO_USER_LED1, "Led D1");
    sysGpioAlloc (FMQL_GPIO_USER_LED2, "Led D2");
    sysGpioAlloc (FMQL_GPIO_USER_LED3, "Led D3");
    sysGpioAlloc (FMQL_GPIO_USER_LED4, "Led D4");

    sysGpioSelectOutput (FMQL_GPIO_USER_LED1);
    sysGpioSelectOutput (FMQL_GPIO_USER_LED2);
    sysGpioSelectOutput (FMQL_GPIO_USER_LED3);
    sysGpioSelectOutput (FMQL_GPIO_USER_LED4);

    printf ("Turn on and off user leds for 5 times!\n");
    taskDelay (30);

    for (i = 0; i < 5; i++)
    {
        sysGpioSetValue (FMQL_GPIO_USER_LED1, GPIO_VALUE_LOW);
        taskDelay (40);
        sysGpioSetValue (FMQL_GPIO_USER_LED2, GPIO_VALUE_LOW);
        taskDelay (40);
        sysGpioSetValue (FMQL_GPIO_USER_LED3, GPIO_VALUE_LOW);
        taskDelay (40);
        sysGpioSetValue (FMQL_GPIO_USER_LED4, GPIO_VALUE_LOW);
        taskDelay (40);

        sysGpioSetValue (FMQL_GPIO_USER_LED1, GPIO_VALUE_HIGH);
        taskDelay (40);
        sysGpioSetValue (FMQL_GPIO_USER_LED2, GPIO_VALUE_HIGH);
        taskDelay (40);
        sysGpioSetValue (FMQL_GPIO_USER_LED3, GPIO_VALUE_HIGH);
        taskDelay (40);
        sysGpioSetValue (FMQL_GPIO_USER_LED4, GPIO_VALUE_HIGH);
        taskDelay (40);
    }

    sysGpioFree (FMQL_GPIO_USER_LED1);
    sysGpioFree (FMQL_GPIO_USER_LED2);
    sysGpioFree (FMQL_GPIO_USER_LED3);
    sysGpioFree (FMQL_GPIO_USER_LED4);

    printf ("User led test end!\n");

	return;
}

#endif /* SYS_GPIO_TEST */


