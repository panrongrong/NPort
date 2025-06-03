/* vxbArmGenIntCtlr.h - ARM Generic interrupt controller driver  */

/*
 * Copyright (c) 2008-2009, 2012, 2013 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01h,15jul13,c_l  Add interrupt priority/sensitivity support. (WIND00420705)
01g,07aug12,j_b  add distributor and CPU interface register offset support
                 (CQ:WIND00367952)
01f,15apr12,clx  support PPI interrupt type
01e,09jan12,ggm  Fixed interrupt controller configuration (WIND00283926).
01d,27oct09,z_l  add SMP support
01c,16apr09,j_b  change core reset and start IPI #s
01b,26nov08,j_b  declare armGicBase as an extern
01a,21oct08,j_b  written based on vxbMPCoreIntCtlr.h, rev 01a
*/

#ifndef __INCvxbArmGenIntCtlrh
#define __INCvxbArmGenIntCtlrh

#ifdef __cplusplus
extern "C"{
#endif

/*
 * the explanation of some abbreviations
 *
 * CPC - Cross Processor Call
 * IPI - Inter Processor Interrupt
 * GIC - General Interrupt Controller
 * SGI - Software Generated Interrupt
 * PPI - Private Peripheral Interrupt
 * SPI - Shared Peripheral Interrupt
 */

/* MPCore GIC interrupt levels */

#define INT_LVL_MPCORE_IPI00         0 /* This is vxWorks CPC IPI */
#define INT_LVL_MPCORE_CPC           INT_LVL_MPCORE_IPI00

#define INT_LVL_MPCORE_IPI01         1 /* This is vxWorks Debug IPI */
#define INT_LVL_MPCORE_DEBUG         INT_LVL_MPCORE_IPI01

#define INT_LVL_MPCORE_IPI02         2 /* This is vxWorks Reschedule IPI */
#define INT_LVL_MPCORE_SCHED         INT_LVL_MPCORE_IPI02

#define INT_LVL_MPCORE_IPI03         3 /* IPI -> use SWI register in GIC */
#define INT_LVL_MPCORE_IPI04         4 /* IPI -> use SWI register in GIC */
#define INT_LVL_MPCORE_IPI05         5 /* IPI -> use SWI register in GIC */

#define INT_LVL_MPCORE_IPI06         6 /* This is vxWorks Core Reset IPI */
#define INT_LVL_MPCORE_RESET         INT_LVL_MPCORE_IPI06

#define INT_LVL_MPCORE_IPI07         7 /* This is vxWorks Core Start IPI */
#define INT_LVL_MPCORE_START         INT_LVL_MPCORE_IPI07

#define INT_LVL_MPCORE_IPI08         8 /* IPI -> use SWI register in GIC */
#define INT_LVL_MPCORE_IPI09         9 /* IPI -> use SWI register in GIC */
#define INT_LVL_MPCORE_IPI10        10 /* IPI -> use SWI register in GIC */
#define INT_LVL_MPCORE_IPI11        11 /* IPI -> use SWI register in GIC */
#define INT_LVL_MPCORE_IPI12        12 /* IPI -> use SWI register in GIC */
#define INT_LVL_MPCORE_IPI13        13 /* IPI -> use SWI register in GIC */
#define INT_LVL_MPCORE_IPI14        14 /* IPI -> use SWI register in GIC */
#define INT_LVL_MPCORE_IPI15        15 /* IPI -> use SWI register in GIC */


#define SGI_INT_MAX             16
#define ARM_GIC_IPI_COUNT       16    /* MPCore IPI count         */
#define SPI_START_INT_NUM       32    /* SPI start at ID32        */
#define PPI_START_INT_NUM       16    /* PPI start at ID16        */
#define GIC_INT_MAX_NUM         1020  /* GIC max interrupts count */

#define GIC_INT_ALL_ENABLED     0xFF  /* priority 0-0xFF can run */
#define GIC_INT_ALL_DISABLED    0x00  /* nothing higher than 0 hence disabled */
#define GIC_INT_SPURIOUS        0x3FF /* no interrupt currently */
#define GIC_INT_ONEMINUS_HIGH   0x55555555 /* interrupt config to 1-N, High */
#define GIC_CPU_BINP_DEFAULT    0x07  /* split all priority to subpriority */
#define GIC_CPU_CONTROL_ENABLE  0x1   /* enable the processor interface */
#define GIC_CPU_ALL_ENABLED     0xFF  /* priority 0-E can run */
#define GIC_CPU_ALL_DISABLED    0x00  /* nothing higher than 0 */
#define GIC_SGI_SRC_CPU_ID_MASK 0x1C00
#define GIC_INT_HIGHEST_PRIORITY 0x0  /* the highest priority for interrupts */
#define GIC_INT_LOWEST_PRIORITY  0x1F /* the lowest priority for interrupts */
#define GIC_INT_PRIORITY_SHIFT   0x8
#define GIC_INT_PRIORITY_MASK    0xFF
#define GIC_INT_TRIGGER_SHIFT    0x2

#define GIC_CPU_DIR_DEFAULT 0x01010101 /* all interrupts are directed to CPU0 */

#define GIC_CONTROL_ENABLE  0x01

#define ALL_PPI_INT_MASK  0xFFFF0000 /* bit field for all PPI interrupts */
#define ALL_SGI_INT_MASK  0x0000FFFF /* bit field for all SGI interrupts */

/* ARM GIC interrupt distributor and CPU interface register access macros */

#define BITS_PER_WORD           32
#define CONFIGS_PER_WORD        16
#define PRIOS_PER_WORD          4
#define TARGETS_PER_WORD        4
#define NWORD(bitnum)           (bitnum / 32)
#define BIT(bitnum)             (1 << (bitnum % BITS_PER_WORD))

#ifndef _ASMLANGUAGE

/* GIC distributor and CPU interface register base offsets */

#define GIC_DIST  (armGicBase + armGicDistOffset)
#define GIC_CPU   (armGicBase + armGicCpuOffset)

/* interrupt distributor control */

#define GIC_Control            ((volatile UINT32 *)(GIC_DIST + 0x000))

/* interrupt controller type */

#define GIC_Type               ((volatile UINT32 *)(GIC_DIST + 0x004))

/* interrupt enable */

#define GIC_IntEnable(Periph)  ((volatile UINT32 *)(GIC_DIST + 0x100 + \
                                          (0x4 * NWORD(Periph))))
/* clear interrupt enable */

#define GIC_IntEnClr(Periph)   ((volatile UINT32 *)(GIC_DIST + 0x180 + \
                                          (0x4 * NWORD(Periph))))
/* interrupt active status */

#define GIC_IntActSts(Periph)   ((volatile UINT32 *)(GIC_DIST + 0x300 + \
                                          (0x4 * NWORD(Periph))))

/* clear pending interrupt */

#define GIC_IntPendClr(Periph) ((volatile UINT32 *)(GIC_DIST + 0x280 + \
                                          (0x4 * NWORD(Periph))))
/* interrupt configuration */

#define GIC_Config(Periph)     ((volatile UINT32 *)(GIC_DIST + 0xC00 + \
                                          (0x4 * (Periph / CONFIGS_PER_WORD))))
/* interrupt priority  */

#define GIC_Prio(Periph)       ((volatile UINT32 *)(GIC_DIST + 0x400 + \
                                          (0x4 * (Periph / PRIOS_PER_WORD))))
/* CPU target */

#define GIC_CPUTarg(Periph)    ((volatile UINT32 *)(GIC_DIST + 0x800 + \
                                          (0x4 * (Periph / TARGETS_PER_WORD))))
/* software interrupt generate */

#define GIC_SWInterrupt        ((volatile UINT32 *)(GIC_DIST + 0xF00))

/* CPU interrupt control */

#define GIC_CPU_Control        ((volatile UINT32 *)(GIC_CPU + 0x00))

/* CPU priority mask */

#define GIC_CPU_PriMask        ((volatile UINT32 *)(GIC_CPU + 0x04))

/* CPU binary point */

#define GIC_CPU_BinPoint       ((volatile UINT32 *)(GIC_CPU + 0x08))

/* CPU interrupt acknowledge */

#define GIC_CPU_IntAck         ((volatile UINT32 *)(GIC_CPU + 0x0C))

/* CPU end of interrupt */

#define GIC_CPU_EOInt          ((volatile UINT32 *)(GIC_CPU + 0x10))

/* CPU running priority */

#define GIC_CPU_RunPri         ((volatile UINT32 *)(GIC_CPU + 0x14))

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif  /* __INCvxbArmGenIntCtlrh */
