/* sysALib.s - FMSH FMQL system-dependent routines */

/*
 * Copyright (c) 2011-2013 Wind River Systems, Inc.
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
This module contains system-dependent routines written in assembly
language. It contains the entry code, sysInit(), for VxWorks images
that start running from RAM, such as 'vxWorks'.  These images are
loaded into memory by some external program (e.g., a boot ROM) and then
started.  The routine sysInit() must come first in the text segment.
Its job is to perform the minimal setup needed to call the generic C
routine usrInit().

sysInit() masks interrupts in the processor and the interrupt
controller and sets the initial stack pointer.  Other hardware and
device initialisation is performed later in the sysHwInit routine in
sysLib.c.

NOTE
The routines in this module don't use the "C" frame pointer %r11@ ! or
establish a stack frame.
*/

#define _ASMLANGUAGE
#include <vxWorks.h>
#include <vsbConfig.h>
#include <asm.h>
#include <regs.h>
#include <arch/arm/arm.h>
#include <arch/arm/mmuArmLib.h>
#include <arch/arm/cacheArmArch7.h>
#include <sysLib.h>
#include "config.h"
#ifdef _WRS_CONFIG_SMP
#   include <private/windLibP.h>
#   include <private/vxSmpP.h>
#endif /* _WRS_CONFIG_SMP */

    /* macros */

#ifdef INCLUDE_EARLY_MMU_ENABLE

    /*
     * The following macros recursively generate ARMARCH7 first level page table
     * entries, allowing early static MMU configuration. 
     *
     * Arguments for SECTION macros are:
     *
     * base:        The nth megabyte address of the section, 256/0x100 equals
     *              0x10000000
     * s,x,tex,ap,p,d,xn,c,b: values for those bits
     * total:       Number of sections to be mapped.
     *
     * The INVALID_ENTRY takes one argument, the number of invalid pages between
     * the preceding section and the next. 
     *
     * The correct counts for section and invalid entrys are critical and must 
     * sum to 4096.
     *
     * See the ARM Architecture Reference manual, V7 for information on the 
     * V7 MMU.
     */

#ifdef _DIAB_TOOL
    SECTION: .macro base,s,x,tex,ap,p,d,xn,c,b,total
        .if total
            .if total <= 0x40
                .long ((base<<20)|(s<<16)|(x<<15)|(tex<<12)|(ap<<10)|(p<<9)|(d<<5)|(xn<<4)|(c<<3)|(b<<2)|2)
                SECTION (base+1),s,x,tex,ap,p,d,xn,c,b,(total-1)
            .else
                .if total <= 0x256
                    SECTION base,s,x,tex,ap,p,d,xn,c,b,0x40
                    SECTION (base+0x40),s,x,tex,ap,p,d,xn,c,b,(total-0x40)
                .else
                    SECTION base,s,x,tex,ap,p,d,xn,c,b,0x256
                    SECTION (base+0x256),s,x,tex,ap,p,d,xn,c,b,(total-0x256)
                .endif
            .endif
        .endif
        .endm

    INVALID_ENTRY: .macro total
        .if total
            .if total <= 0x40
                .long (0)
                INVALID_ENTRY (total-1)
            .else
                .if total <= 0x256
                    INVALID_ENTRY 0x40
                    INVALID_ENTRY (total-0x40)
                .else
                    INVALID_ENTRY 0x256
                    INVALID_ENTRY (total-0x256)
                .endif
            .endif
        .endif
        .endm

#else /* GNU */

    .MACRO SECTION base,s,x,tex,ap,p,d,xn,c,b,total=0
    .if \total
       .if \total <= 0x40
          .long (\base << 20) | (\s << 16) | (\x << 15) | (\tex << 12) | (\ap << 10) | (\p << 9) | (\d << 5) | (\xn << 4) | (\c << 3) | (\b << 2) | 2
          SECTION "(\base+1)",\s,\x,\tex,\ap,\p,\d,\xn,\c,\b,"(\total-1)"
       .else
           .if \total <= 0x256
               SECTION \base,\s,\x,\tex,\ap,\p,\d,\xn,\c,\b,0x40
               SECTION "(\base+0x40)",\s,\x,\tex,\ap,\p,\d,\xn,\c,\b,"(\total-0x40)"
           .else
               SECTION \base,\s,\x,\tex,\ap,\p,\d,\xn,\c,\b,0x256
               SECTION "(\base+0x256)",\s,\x,\tex,\ap,\p,\d,\xn,\c,\b,"(\total-0x256)"
           .endif
       .endif
    .endif
    .ENDM

    .MACRO INVALID_ENTRY total=0
    .if \total
       .if \total <= 0x40
           .long (0)
           INVALID_ENTRY "(\total-1)"
       .else
           .if \total <= 0x256
               INVALID_ENTRY 0x40
               INVALID_ENTRY "(\total-0x40)"
           .else
               INVALID_ENTRY 0x256
               INVALID_ENTRY "(\total-0x256)"
           .endif
       .endif
    .endif
    .ENDM

#endif /* _DIAB_TOOL */
#endif /* INCLUDE_EARLY_MMU_ENABLE */

    /* D-cache enable, Control Register bit */

    #define SYS_CTL_DCACHE_ENABLE (0x1 << 2) 
    
    /* I-cache enable, Control Register bit */

    #define SYS_CTL_ICACHE_ENABLE (0x1 << 12) 

    /* SMP mode enable, Aux Ctrl register bit */

    #define AUX_CTL_SMP_MODE_EN (0x1 << 6)

    /* internals */

    .globl  FUNC(sysInit)           /* start of system code */

#ifndef _ARCH_SUPPORTS_PROTECT_INTERRUPT_STACK
    .globl  FUNC(sysIntStackSplit)  /* routine to split interrupt stack */
#endif /* !_ARCH_SUPPORTS_PROTECT_INTERRUPT_STACK */
    .globl  FUNC(archPwrDown)       /* power down callback */
#ifdef _WRS_CONFIG_SMP
    .globl  FUNC(sysCpuInit)     /* Sub CPU Init loop */
    .globl  FUNC(sysMPCoreStartup)     /* Startup Data - see sysLib.c */
#endif /* _WRS_CONFIG_SMP */

    /* externals */
    .extern FUNC(usrInit)        /* system initialization routine */

#ifndef _ARCH_SUPPORTS_PROTECT_INTERRUPT_STACK
    .extern FUNC(vxSvcIntStackBase)   /* base of SVC-mode interrupt stack */
    .extern FUNC(vxSvcIntStackEnd)    /* end of SVC-mode interrupt stack */
    .extern FUNC(vxIrqIntStackBase)   /* base of IRQ-mode interrupt stack */
    .extern FUNC(vxIrqIntStackEnd)    /* end of IRQ-mode interrupt stack */
#endif /* !_ARCH_SUPPORTS_PROTECT_INTERRUPT_STACK */

#ifdef _WRS_CONFIG_SMP
    .extern FUNC(vxKernelVars)
#endif /* _WRS_CONFIG_SMP */

    .text
    .balign 4

/*******************************************************************************
*
* sysInit - start after boot
*
* This routine is the system start-up entry point for VxWorks in RAM, the
* first code executed after booting.  It disables interrupts, sets up
* the stack, and jumps to the C routine usrInit() in usrConfig.c.
*
* The initial stack is set to grow down from the address of sysInit().  This
* stack is used only by usrInit() and is never used again.  Memory for the
* stack must be accounted for when determining the system load address.
*
* NOTE: This routine should not be called by the user.
*
* RETURNS: N/A
*
* sysInit ()              /@ THIS IS NOT A CALLABLE ROUTINE @/
*
*/

_ARM_FUNCTION(sysInit)

    /*
     * Set processor and MMU to known state as follows (we may have not
     * been entered from a reset). We must do this before setting the CPU
     * mode as we must set PROG32/DATA32.
     *
     * MMU Control Register layout.
     *
     * bit
     *  0 M 0 MMU disabled
     *  1 A 0 Address alignment fault disabled, initially
     *  2 C 0 Data cache disabled
     *  3 W 0 Write Buffer disabled
     *  4 P 1 PROG32
     *  5 D 1 DATA32
     *  6 L 1 Should Be One (Late abort on earlier CPUs)
     *  7 B ? Endianness (1 => big)
     *  8 S 0 System bit to zero } Modifies MMU protections, not really
     *  9 R 1 ROM bit to one     } relevant until MMU switched on later.
     * 10 F 0 Should Be Zero
     * 11 Z 1 Branch prediction enabled
     * 12 I 0 Instruction cache control
     */

    /* TTBCR must be 0 for vxWorks */
    MOV    r1, #0
    MCR    CP_MMU, 0, r1, c2, c0, 2

    /* Flush the entire BTAC before enabling the Z bit */
    MOV    r1, #0
    MCR    CP_MMU, 0, r1, c7, c5, 6

    /* Setup MMU Control Register */
    LDR    r1, =MMU_INIT_VALUE           /* Defined in mmuCortexA8Lib.h */
    MCR    CP_MMU, 0, r1, c1, c0, 0      /* Write to MMU CR */

    /* perform a Data Synchronization Barrier */
    DSB

    /*
     * Set Process ID Register to zero, this effectively disables
     * the process ID remapping feature.
     */

    MOV    r1, #0
    MCR    CP_MMU, 0, r1, c13, c0, 0

    /* Set Context ID Register to zero, including Address Space ID */

    MCR    CP_MMU, 0, r1, c13, c0, 1

    /* disable interrupts in CPU and switch to SVC32 mode */

    MRS    r1, cpsr
    BIC    r1, r1, #MASK_MODE
    ORR    r1, r1, #MODE_SVC32 | I_BIT | F_BIT
    MSR    cpsr, r1
    
    /* disable individual interrupts in the interrupt controller */    

    /* disable ints to CPU */
    LDR    r2, =FMQL_GIC_CPU_CONTROL
    MOV    r1, #0
    STR    r1, [r2, #0]       

    /* disable ints for distributor */
    LDR    r2, =(FMQL_GIC_BASE + FMQL_GIC_DIST_CONTROL)
    STR    r1, [r2, #0]


#ifdef INCLUDE_EARLY_MMU_ENABLE

    LDR r1, =0xffffffff
    MCR CP_MMU, 0, r1, c3, c0, 0    /* write DACR */

    MOV r1, #0
    MCR CP_MMU, 0, r1, c7, c5, 0    /* invalidate BTB */
    ISB
    DSB

    MOV r1, #0
    MCR CP_MMU, 0, r2, c13, c0, 1   /* set ASID to 0 = kernel */
    ISB
    DSB

    LDR r1, =staticMmuPageTable
    MCR CP_MMU, 0, r1, c2, c0, 0    /* load the TTBR */
    MOV r1, #0
    MCR CP_MMU, 0, r1, c8, c7, 0    /* invalidate all I & D TLBs */
    ISB
    DSB

    MRC CP_MMU, 0, r1, c1, c0, 0    /* read control register */
    LDR r2, =MMUCR_M_ENABLE         /* enable MMU */

    ORR r1, r1, r2
    MCR CP_MMU, 0, r1, c1, c0, 0    /* write control register */
    ISB
    DSB
#endif /* INCLUDE_EARLY_MMU_ENABLE */

    /* set initial stack pointer so stack grows down from start of code */
    ADR    sp, FUNC(sysInit)        /* initialize stack pointer */
    MOV    fp, #0                   /* initialize frame pointer */

    /* Make sure Boot type is set correctly. */
    MOV    r1,#BOOT_NORMAL
    CMP    r1,r0
    BEQ    L$_Good_Boot

    MOV    r1,#BOOT_NO_AUTOBOOT
    CMP    r1,r0
    BEQ    L$_Good_Boot

    MOV    r1,#BOOT_CLEAR
    CMP    r1,r0
    BEQ    L$_Good_Boot

    MOV    r1,#BOOT_QUICK_AUTOBOOT
    CMP    r1,r0
    BEQ    L$_Good_Boot

    MOV    r0, #BOOT_NORMAL /* default startType */

L$_Good_Boot:

    /* now call usrInit (startType) */

#if (ARM_THUMB)
    LDR    r12, L$_usrInit
    BX     r12
#else
    B      FUNC(usrInit)
#endif /* (ARM_THUMB) */

#ifndef _ARCH_SUPPORTS_PROTECT_INTERRUPT_STACK
/*******************************************************************************
*
* sysIntStackSplit - split interrupt stack and set interrupt stack pointers
*
* This routine is called, via a function pointer, during kernel
* initialisation.  It splits the allocated interrupt stack into IRQ and
* SVC-mode stacks and sets the processor's IRQ stack pointer. Note that
* the pointer passed points to the bottom of the stack allocated i.e.
* highest address+1.
*
* IRQ stack needs 6 words per nested interrupt;
* SVC-mode will need a good deal more for the C interrupt handlers.
* For now, use ratio 1:7 with any excess allocated to the SVC-mode stack
* at the lowest address.
*
* Note that FIQ is not handled by VxWorks so no stack is allocated for it.
*
* The stacks and the variables that describe them look like this.
* \cs
*
*         - HIGH MEMORY -
*     ------------------------ <--- vxIrqIntStackBase (r0 on entry)
*     |                      |
*     |       IRQ-mode       |
*     |    interrupt stack   |
*     |                      |
*     ------------------------ <--{ vxIrqIntStackEnd
*     |                      |    { vxSvcIntStackBase
*     |       SVC-mode       |
*     |    interrupt stack   |
*     |                      |
*     ------------------------ <--- vxSvcIntStackEnd
*         - LOW  MEMORY -
* \ce
*
* NOTE: This routine should not be called by the user.
*
* void sysIntStackSplit
*     (
*     char *pBotStack   /@ pointer to bottom of interrupt stack @/
*     long size        /@ size of stack @/
*     )

*/

    .balign    4

_ARM_FUNCTION_CALLED_FROM_C(sysIntStackSplit)

    /*
     * r0 = base of space allocated for stacks (i.e. highest address)
     * r1 = size of space
     */

    SUB    r2, r0, r1                /* r2->lowest usable address */
    LDR    r3, L$_vxSvcIntStackEnd
    STR    r2, [r3]                  /*  == end of SVC-mode stack */
    SUB    r2, r0, r1, ASR #3        /* leave 1/8 for IRQ */
    LDR    r3, L$_vxSvcIntStackBase
    STR    r2, [r3]

    /* now allocate IRQ stack, setting irq_sp */

    LDR    r3, L$_vxIrqIntStackEnd
    STR    r2, [r3]
    LDR    r3, L$_vxIrqIntStackBase
    STR    r0, [r3]

    MRS    r2, cpsr
    BIC    r3, r2, #MASK_MODE
    ORR    r3, r3, #MODE_IRQ32 | I_BIT    /* set irq_sp */
    MSR    cpsr, r3
    MOV    sp, r0

    /* switch back to original mode and return */

    MSR    cpsr, r2

#if (ARM_THUMB)
    BX     lr
#else
    MOV    pc, lr
#endif /* (ARM_THUMB) */
#endif /* !_ARCH_SUPPORTS_PROTECT_INTERRUPT_STACK */
  
/*******************************************************************************
*
* archPwrDown - turn the processor into reduced power mode
*
* This routine activates the reduced power mode.
* It is called by the scheduler when the kernel enters the idle loop.
* This function is called by default. Overload it by using routine
* vxArchPowerDownRtnSet().
*
* RETURNS: void.
*
* SEE ALSO: vxArchPowerDownRtnSet().
*
* void archPwrDown (void)
*
*/

_ARM_FUNCTION_CALLED_FROM_C(archPwrDown)


    /*
     * NB debugger doesn't like powering down.  
     * Use foreverloop for debug.
     *foreverLoop:
     *       B     foreverLoop
     */

    /*
    * Write to coprocessor 15 register 7 (the core control)
    * register to set idle
    */
    
    MOV     r0, #PWRMODE_IDLE
    MCR     CP_CORECTL, 0, r0, c7, c0, 4  /* idle processor */

    /* Return after waking up */

#if (ARM_THUMB)
    BX    lr
#else
    MOV    pc, lr
#endif

#ifdef _WRS_CONFIG_SMP
/*******************************************************************************
*
* sysCpuInit - Entry point for CPUs 1
*
* This routine performs initial CPU init, copies startup parameters
* from the sysMPCoreStartup structure, and enters sysCpuStart to
* complete the per-cpu startup.
*
* There are no arguments to this routine.
*
* RETURNS: Does not return.
*
*/

sysCpuInit:

    /* disable interrupts in CPU and switch to SVC32 mode */

    MRS    r1, cpsr
    BIC    r1, r1, #MASK_MODE
    ORR    r1, r1, #MODE_SVC32 | I_BIT | F_BIT
    MSR    cpsr, r1

    /* 
     * SPSR does not have pre-defined reset value. 
     * Here correct endianess (BE bit) in SPSR
     */

    MRS    r0, spsr
#if (_BYTE_ORDER == _LITTLE_ENDIAN)
    BIC    r0, r0, #(0x1 << 9)    /* Little Endian*/
#else
    ORR    r0, r0, #(0x1 << 9)    /* Big Endian */
#endif
    MSR    spsr_x, r0

    /*  Setup AUX register and enable SMP mode */
    MRC     CP_MMU, 0, r1, c1, c0, 1	/* Read ACTLR */
    
    /* Enable cache and TLB maintenance broadcast and dside prefetch */    
   /*
    LDR     r2, =(AUX_CTL_REG_FW | AUX_CTL_REG_L1_PRE_EN)
    ORR     r1, r1, r2
    */
    
    /* Enable ACTLR SMP */
    ORR     r1, r1, #AUX_CTL_SMP_MODE_EN
    MCR     CP_MMU, 0, r1, c1, c0, 1    /* Write ACTLR */
    ISB                                 /* Ensure ACTLR settings */

    MCR     CP_MMU, 0, r1, c7, c5, 0    /* Invalidate I-cache */
    ISB                                 /* Ensure invalidation is done */

    /* Flush the entire BTAC before enable Z bit*/
    MOV     r1, #0
    MCR     CP_MMU, 0, r1, c7, c5, 6

    /* Setup MMU Control Register */
    LDR     r1, =MMU_INIT_VALUE             /* Defined in mmuArmLib.h */
    ORR     r1, r1, #(0x1 << 11)            /* enable branch prediction */
    MCR     CP_MMU, 0, r1, c1, c0, 0        /* Write to MMU CR */
    ISB                                	    /* Ensure processor state is set */

    /* Flush + Invalidate D-caches */

    _CORTEX_AR_ENTIRE_DATA_CACHE_OP(c14)

    /* Note that r0 contains 0 at this point */

    DSB  /* Data Synch Barrier */

    /* Invalidate TLBS */

    MCR     CP_MMU, 0, r1, c8, c7, 0    /* R1 = 0 from above, data SBZ*/

    /*
     * Set Process ID Register to zero, this effectively disables
     * the process ID remapping feature.
     */

    MOV     r1, #0
    MCR     CP_MMU, 0, r1, c13, c0, 0

    /* Set Context ID Register to zero, including Address Space ID */

    MCR     CP_MMU, 0, r1, c13, c0, 1

    /* Get cpuIndex */

    MRC     CP_MMU, 0, r5, c0, c0, 5
    AND     r5, r5, #0xF

    /* Get the address of the startup data, sysMPCoreStartup[cpuIndex] */
   
    MOV     r4, r5, LSL #4
    LDR     r0, L$_sysMPCoreStartup
    ADD     r4, r4, r0

    /*
     * Set the Translation Table Base Register
     *
     * r4 = Pointer to the startup data for this CPU
     */

    LDR     r0, [r4, #0xC]              /* Get Trans. Tbl Base address */
    MOV     r1, #VXWORKS_KERNEL_ASID
    BL      mmuCortexA8TtbrSetAll
    
    MOV     r0, #MMU_DACR_VAL_NORMAL
    BL      mmuCortexA8DacrSet

    /* Enable MMU and caches */

    LDR     r0, =(SYS_CTL_ICACHE_ENABLE | SYS_CTL_DCACHE_ENABLE)
    BL      mmuCortexA8AEnable

    /* Save the cache state */

    MOV     r2, r0
    _ARM_PER_CPU_ADRS_GET(r0, r1, cacheArchState)
    STR     r2, [r0]

    /*
     * Clear the kernel interrupt counter and 
     * architecture interrupt nesting counter.
     * This is needed because the secondary CPU startup process
     * will bypass the normal interrupt exit path (intExit).
     */

    _ARM_PER_CPU_ADRS_GET(r0, r1, intCnt)

    MOV     r1, #0
    STR     r1, [r0]

    _ARM_PER_CPU_ADRS_GET(r0, r1, intNestingLevel)

    MOV     r1, #0
    STR     r1, [r0]

    /*
     * r4 = Pointer to sysMPCoreStartup arguments array
     * r5 = CPU number
     *
     * Set up call to start VxWorks such that:
     *
     * r0 = vxWorks Kernel Entry point
     * r1 = CPU number
     * r2 = CPU startup entry point, sysCpuStart
     *
     */
    LDR     sp, [r4, #4]                /* set the kernel stack pointer */

    MOV     r1, r5
    
sysMPCoreApStartLoop:
    LDR     r2, [r4]
    CMP     r2, #0
    LDRNE   r0, [r4, #8]                /* Load vxWorks Kernel Entry point */
    BLXNE   r2                          /* Enter vxWorks */
    
    B       sysMPCoreApStartLoop

#endif /* _WRS_CONFIG_SMP */

/******************************************************************************/

/*
 * PC-relative-addressable pointers - LDR Rn,=sym is broken
 * note "_" after "$" to stop preprocessor preforming substitution
 */

    .balign    4

#ifdef _WRS_CONFIG_SMP

L$_sysMPCoreStartup:
        .long   VAR(sysMPCoreStartup)

L$_vxKernelVars:
        .long   FUNC(vxKernelVars)

#endif /* _WRS_CONFIG_SMP */

#ifndef    _ARCH_SUPPORTS_PROTECT_INTERRUPT_STACK
L$_vxSvcIntStackBase:
    .long    VAR(vxSvcIntStackBase)

L$_vxSvcIntStackEnd:
    .long    VAR(vxSvcIntStackEnd)

L$_vxIrqIntStackBase:
    .long    VAR(vxIrqIntStackBase)

L$_vxIrqIntStackEnd:
    .long    VAR(vxIrqIntStackEnd)
#endif    /* !_ARCH_SUPPORTS_PROTECT_INTERRUPT_STACK */

#if    (ARM_THUMB)
L$_usrInit:
    .long    FUNC(usrInit)
#endif /* (ARM_THUMB) */

#ifdef INCLUDE_EARLY_MMU_ENABLE

/*******************************************************************************
 *
 * Statically Defined MMU Translation Table
 * --------------------------------------------
 * The statical Translation Table defined below is used to enable the MMU 
 * before absolute(virtual) address access of any code or data when the  
 * memory map configured in sysPhysMemDesc is NOT 1 to 1 (flat-map). 
 *
 * The table of level 1 descriptors below, indicated by the 
 * <staticMmuPageTable> label, must be a 16 Kilobyte-aligned physical address.
 */

    .data
    .balign 0x4000
    
staticMmuPageTable:
    /* RAM - 0x0 ~ 0x3FFFFFFF, 1GB range */
    SECTION 0x000, 0, 0, 1, 3, 0, 0, 0, 1, 0, 0x400

    /* Device - 0x40000000 ~ 0xFFFFFFFF, 3GB range */
    SECTION 0x400, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0xC00

#endif /* INCLUDE_EARLY_MMU_ENABLE */

/*******************************************************************************
*
* Array used for passing startup parameters from CPU 0 to startup processors
*
*struct sysMPCoreStartup
*    {
*    UINT32      newPC;          /@ Address of 'C' based startup code @/
*    UINT32      newSP;          /@ Stack pointer for startup @/
*    UINT32      newArg;         /@ vxWorks kernel entry point @/
*    UINT32      newSync;        /@ Translation Table Base and sync @/
*    };
*/

    .data
    .balign 32
sysMPCoreStartup:
    .fill 32,4

	
