/* 20bsp.cdf - BSP-specific component descriptor file */

/*
 * Copyright (c) 2011-2019 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
010,10jul19,l_l  created.
*/

/*
DESCRIPTION
This file contains descriptions for this BSP components.
*/

Bsp fmsh_fmql {
    NAME        board support package
    CPU         ARMARCH7
    ENDIAN      little
    FP          vector
    MP_OPTIONS  SMP
    REQUIRES    INCLUDE_KERNEL \
                INCLUDE_VXBUS  \
                DRV_ARM_GIC    \
                DRV_SIO_NS16550  \
                INCLUDE_ALT_SOC_GEN5_TIMER
}

Parameter RAM_HIGH_ADRS {
    NAME        Bootrom Copy region
    DEFAULT     (INCLUDE_BOOT_RAM_IMAGE)::(0x1d00000) \
                (INCLUDE_BOOT_APP)::(0x1800000) \
                0x1000000
}

Parameter RAM_LOW_ADRS {
    NAME        Runtime kernel load address
    DEFAULT     (INCLUDE_BOOT_RAM_IMAGE)::(0x01a00000) \
                (INCLUDE_BOOT_APP)::(0x00800000) \
                0x200000
}

/*
 * VX_SMP_NUM_CPUS is a SMP parameter only and only available for SMP
 * builds. Due to a limitation of the project tool at the time this
 * parameter is created where the tool can not recognize the ifdef SMP
 * selection, this parameter is set up such that _CFG_PARAMS is not
 * specified here. In the 00vxWorks.cdf file, where the parameter
 * VX_SMP_NUM_CPUS is defined, the _CFG_PARAMS is specified only for
 * VxWorks SMP. Hence the redefining of VX_SMP_NUM_CPUS here should only
 * override the value and not the rest of the properties. And for UP, the
 * parameter is ignored since the parameter is not tied to any component
 * (_CFG_PARAMS is not specified).
 */

Parameter VX_SMP_NUM_CPUS {
        NAME            Number of CPUs available to be enabled for VxWorks SMP
        TYPE            UINT
        DEFAULT         4
}

Profile PROFILE_BOOTAPP {
    COMPONENTS += DRV_ARM_GIC DRV_SIO_NS16550 INCLUDE_ALT_SOC_GEN5_TIMER
}

Component INCLUDE_END {
    INCLUDE_WHEN    INCLUDE_NET_INIT
}

Parameter IP_MAX_UNITS {
    DEFAULT         8
}

Component INCLUDE_SYS_MEM_MGMT {
    NAME        IPNET memory management component
    SYNOPSIS    IPNET memory management component
    _CHILDREN   FOLDER_MEMORY
    CFG_PARAMS  SYS_MALLOC \
                SYS_FREE
}

Parameter SYS_MALLOC {
    NAME        system malloc function
    SYNOPSIS    system malloc function
    TYPE        FUNCPTR
    DEFAULT     sysMalloc
}

Parameter SYS_FREE {
    NAME        system free function
    SYNOPSIS    system free function
    TYPE        FUNCPTR
    DEFAULT     sysFree
}

Component INCLUDE_TFFS {
    REQUIRES    INCLUDE_TFFS_MOUNT \
                INCLUDE_TL_FTL
}

Component INCLUDE_USB {
    REQUIRES    INCLUDE_USB_INIT        \
                INCLUDE_EHCI            \
                INCLUDE_EHCI_INIT
}

Component INCLUDE_TIMESTAMP {
    REQUIRES += INCLUDE_VXB_TIMESTAMP
}

Component INCLUDE_AUX_CLK {
    REQUIRES += INCLUDE_VXB_AUX_CLK
}

Folder FMSH_FMQL_DRIVERS {
    NAME         FMSH FMQL Drivers
    SYNOPSIS     This folder contains Device Drivers for the FMSH FMQL PSoc.
    _CHILDREN    FOLDER_DRIVERS
}

Component INCLUDE_FMSH_FMQL_GPIO {
    NAME         FMSH FMQL GPIO support
    SYNOPSIS     GPIO module in FMSH FMQL PSoc.
    REQUIRES     INCLUDE_VXB_LEGACY_INTERRUPTS
    _CHILDREN    FMSH_FMQL_DRIVERS
}

