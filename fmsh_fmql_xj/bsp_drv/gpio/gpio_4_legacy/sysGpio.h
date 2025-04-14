/* sysGpio.h - support routines for Altera Soc Gen 5 GPIO */

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
*/

#ifndef __INCsysGpioh
#define __INCsysGpioh

#include <vxWorks.h>

#ifdef __cplusplus
extern "C" {
#endif

/* defines */

#define GPIO_IRQ_SENSITIVE_NONE          (0x0)
#define GPIO_IRQ_LOW_LEVEL_SENSITIVE     (0x1)
#define GPIO_IRQ_HIGH_LEVEL_SENSITIVE    (0x2)
#define GPIO_IRQ_RISING_EDGE_SENSITIVE   (0x4)
#define GPIO_IRQ_FALLING_EDGE_SENSITIVE  (0x8)

#define GPIO_VALUE_LOW                   (0)
#define GPIO_VALUE_HIGH                  (1)
#define GPIO_VALUE_INVALID               (0xff)

/* io ctrl */
#define GPIO_DIR 						(0x0)

/* function declarations */

BOOL   sysGpioIsValid (UINT32 gpio);
STATUS sysGpioAlloc (UINT32 gpio, const char * usage);
STATUS sysGpioFree (UINT32 gpio);
STATUS sysGpioSelectInput (UINT32 gpio);
STATUS sysGpioSelectOutput (UINT32 gpio);
STATUS sysGpioSetValue (UINT32 gpio, UINT32 val);
UINT32 sysGpioGetValue (UINT32 gpio);
STATUS sysGpioIntConnect (UINT32 gpio, UINT32 triggerMode, FUNCPTR func,
                          void * arg);
STATUS sysGpioIntEnable (UINT32 gpio);
STATUS sysGpioIntDisable (UINT32 gpio);
STATUS sysGpioIntDisconnect (UINT32 gpio);

STATUS GpioDrv();
STATUS GpioDevCreate(int channel);

#ifdef __cplusplus
}
#endif

#endif /* __INCsysGpioh */

