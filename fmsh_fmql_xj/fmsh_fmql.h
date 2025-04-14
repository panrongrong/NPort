/* fmsh_fmql.h - FMSH FMQL board header file */

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
010,10jul19,l_l  created
*/

/*
This file contains I/O address and related constants for the
FMSH FMQL board.
*/

#ifndef __INCfmsh_fmqlh
#define __INCfmsh_fmqlh

#include "config_pre.h"

#ifdef __cplusplus
extern "C" {
#endif



/* SRAM */

#define FMQL_SRAM_MEM_ADRS        (0x00020000)
#define FMQL_SRAM_MEM_SIZE        (0x00040000)

/* DRAM */


/* system registers */

#define FMQL_SR_BASE              (0xE0026000)
#define FMQL_SR_SCL               (0x00)
#define FMQL_SR_LOCK_OFFSET       (0x04)
#define FMQL_SR_UNLOCK_OFFSET     (0x08)
#define FMQL_SR_LOCK_STAT_OFFSET  (0x0C)


#define FMQL_SR_LOCK             (FMQL_SR_BASE + FMQL_SR_LOCK_OFFSET)
#define FMQL_SR_UNLOCK           (FMQL_SR_BASE + FMQL_SR_UNLOCK_OFFSET)
#define FMQL_SR_LOCK_STAT        (FMQL_SR_BASE + FMQL_SR_LOCK_STAT_OFFSET)

#define FMQL_SR_UNLOCK_ALL       (0xDF0D767B)
#define FMQL_SR_LOCK_ALL         (0xDF0D767B)


/*
 * Generic Interrupt Controller
 * Note: FIQ is not handled within VxWorks so this is just IRQ
 */

#define FMQL_GIC_BASE            (0xF8900000)
#define FMQL_GIC_CPU_OFFSET      (0x2000)
#define FMQL_GIC_DIST_OFFSET     (0x1000)

#define FMQL_GIC_CPU_CONTROL     (FMQL_GIC_BASE + 0x2000)
#define FMQL_GIC_CPU_PRIORITY    (FMQL_GIC_BASE + 0x2004)
#define FMQL_GIC_CPU_POINT       (FMQL_GIC_BASE + 0x2008)
#define FMQL_GIC_CPU_ACK         (FMQL_GIC_BASE + 0x200C)
#define FMQL_GIC_CPU_END_INTR    (FMQL_GIC_BASE + 0x2010)
#define FMQL_GIC_CPU_RUNNING     (FMQL_GIC_BASE + 0x2014)
#define FMQL_GIC_CPU_PENDING     (FMQL_GIC_BASE + 0x2018)

/* 96 is the maximum interrupt number. It covers SGI, PPI and SPI */
#define SYS_INT_LEVELS_MAX         (96)

/* interrupt distributor */
#define FMQL_GIC_DIST_CONTROL      (0x1000)
#define FMQL_GIC_DIST_CTRL_TYPE    (0x1004)
#define FMQL_GIC_DIST_ENABLE_SET1  (0x1100)
#define FMQL_GIC_DIST_ENABLE_SET2  (0x1104)
#define FMQL_GIC_DIST_ENABLE_SET3  (0x1108)
#define FMQL_GIC_DIST_ENABLE_CLR1  (0x1180)
#define FMQL_GIC_DIST_ENABLE_CLR2  (0x1184)
#define FMQL_GIC_DIST_ENABLE_CLR3  (0x1188)
#define FMQL_GIC_DIST_PEND_SET1    (0x1200)
#define FMQL_GIC_DIST_PEND_SET2    (0x1204)
#define FMQL_GIC_DIST_PEND_SET3    (0x1208)
#define FMQL_GIC_DIST_PEND_CLR1    (0x1280)
#define FMQL_GIC_DIST_PEND_CLR2    (0x1284)
#define FMQL_GIC_DIST_PEND_CLR3    (0x1288)
#define FMQL_GIC_DIST_ACTIVE1      (0x1300)
#define FMQL_GIC_DIST_ACTIVE2      (0x1304)
#define FMQL_GIC_DIST_ACTIVE3      (0x1308)
#define FMQL_GIC_DIST_PRIORITY1    (0x1400) /* Priority reg  0- 3 */
#define FMQL_GIC_DIST_PRIORITY2    (0x1404) /* Priority reg  4- 7 */
#define FMQL_GIC_DIST_PRIORITY3    (0x1408) /* Priority reg  8-11 */
#define FMQL_GIC_DIST_PRIORITY4    (0x140C) /* Priority reg 12-15 */
#define FMQL_GIC_DIST_PRIORITY5    (0x1410) /* Priority reg 16-19 */
#define FMQL_GIC_DIST_PRIORITY6    (0x1414) /* Priority reg 20-23 */
#define FMQL_GIC_DIST_PRIORITY7    (0x1418) /* Priority reg 24-27 */
#define FMQL_GIC_DIST_PRIORITY8    (0x141C) /* Priority reg 28-31 */
#define FMQL_GIC_DIST_PRIORITY9    (0x1420) /* Priority reg 32-35 */
#define FMQL_GIC_DIST_PRIORITY10   (0x1424) /* Priority reg 36-39 */
#define FMQL_GIC_DIST_PRIORITY11   (0x1428) /* Priority reg 40-43 */
#define FMQL_GIC_DIST_PRIORITY12   (0x142C) /* Priority reg 44-47 */
#define FMQL_GIC_DIST_PRIORITY13   (0x1430) /* Priority reg 48-51 */
#define FMQL_GIC_DIST_PRIORITY14   (0x1434) /* Priority reg 52-55 */
#define FMQL_GIC_DIST_PRIORITY15   (0x1438) /* Priority reg 56-59 */
#define FMQL_GIC_DIST_PRIORITY16   (0x143C) /* Priority reg 60-63 */
#define FMQL_GIC_DIST_TARG         (0x1800)
#define FMQL_GIC_DIST_CONFIG1      (0x1C00)
#define FMQL_GIC_DIST_CONFIG2      (0x1C04)
#define FMQL_GIC_DIST_CONFIG3      (0x1C08)
#define FMQL_GIC_DIST_CONFIG4      (0x1C0C)
#define FMQL_GIC_DIST_CONFIG5      (0x1C10)
#define FMQL_GIC_DIST_CONFIG6      (0x1C14)
#define FMQL_GIC_DIST_SOFTWARE     (0x1F00)
#define FMQL_GIC_DIST_PERIPH_ID    (0x1FD0)
#define FMQL_GIC_DIST_CELL_ID      (0x1FFC)



/*
FMSH FMQL interrupt level and vectors 
*/

/*
42	SPI	TIMER0(TIMER0_intr1_n)		
43	SPI	TIMER0(TIMER0_intr2_n)		
44	SPI	TIMER0(TIMER0_intr3_n)	*/
#define INT_LVL_ALT_TIMER0          (42)  /* ttc0-timer0,1,2 */ 

/*
65	SPI	TIMER1(TIMER0_intr1_n)		
66	SPI	TIMER1(TIMER0_intr2_n)		
67	SPI	TIMER1(TIMER0_intr3_n)	*/
#define INT_LVL_ALT_TIMER3          (65)  /* ttc1-timer3,4,5 */ 


#ifdef UART_1_AS_CONSOLE  /* logic_uart1 use phy_uart0 */
#define INT_LVL_UART0               (76)
#define INT_LVL_UART1               (55)
#else /* uart_0 as console*/
#define INT_LVL_UART0               (55)
#define INT_LVL_UART1               (76)
#endif

#define INT_VEC_UART0             IVEC_TO_INUM(INT_LVL_UART0)  /* 55 SPI UART0 */
#define INT_VEC_UART1             IVEC_TO_INUM(INT_LVL_UART1)  /* 76 SPI UART1 */


#define INT_VEC_ALT_TIMER0		  IVEC_TO_INUM(INT_LVL_ALT_TIMER0)
#define INT_VEC_ALT_TIMER3		  IVEC_TO_INUM(INT_LVL_ALT_TIMER3)

#define INT_VEC_GTC_TIMER		  IVEC_TO_INUM(29)      /* 29 PPI Secure Physical Timer event(PPI1) 电平	 */

#define INT_VEC_EMAC0             IVEC_TO_INUM(51)      /* 51	SPI	GMAC0 */
#define INT_VEC_EMAC1             IVEC_TO_INUM(72)      /* 72	SPI	GMAC1 */


#ifdef SD_CTRL1_AS_EMMC
#define INT_VEC_SDMMC_0           IVEC_TO_INUM(73)      /* 52	SPI	SDMCC0 */
#else
#define INT_VEC_SDMMC_0           IVEC_TO_INUM(52)      /* 52	SPI	SDMCC0 */
#endif
#define INT_VEC_SDMMC_1           IVEC_TO_INUM(73)      /* 73	SPI	SDMCC1 */


	
#define INT_VEC_WD0               IVEC_TO_INUM(41)      /* 41	SPI	WDT(WDT_intr_n)	 */

#define INT_VEC_I2C0              IVEC_TO_INUM(53)     /* 53	SPI	I2C0	 */
#define INT_VEC_I2C1              IVEC_TO_INUM(74)     /* 74	SPI	I2C1	 */

#define INT_VEC_USB0              IVEC_TO_INUM(50)     /* 50	SPI	USB0	 */
#define INT_VEC_USB1              IVEC_TO_INUM(71)     /* 71	SPI	USB1	 */

/*
49	SPI	GPIO0	
68	SPI	GPIO1		
69	SPI	GPIO2		
70	SPI	GPIO3	*/
#define INT_VEC_GPIO_BANK_A       IVEC_TO_INUM(49)       /* BANK_A Interrupt */
#define INT_VEC_GPIO_BANK_B       IVEC_TO_INUM(68)       /* BANK_B Interrupt */
#define INT_VEC_GPIO_BANK_C       IVEC_TO_INUM(69)       /* BANK_C Interrupt */
#define INT_VEC_GPIO_BANK_D       IVEC_TO_INUM(70)       /* BANK_D Interrupt */

#define INT_VEC_SPIINT0            IVEC_TO_INUM(51)     /* 54	SPI	SPI0 */
#define INT_VEC_SPIINT1            IVEC_TO_INUM(75)     /* 75	SPI	SPI1 */

/*#define CAN0_INT_ID		56U*/
/*#define CAN1_INT_ID		77U*/
#define INT_VEC_CAN_INT0            IVEC_TO_INUM(56)     /* 56	SPI	CAN0 */
#define INT_VEC_CAN_INT1            IVEC_TO_INUM(77)     /* 77	SPI	CAN1 */


/*
PL -> PS irq:
------------------------------
57 	irqf2p[0] 	High Level 	—
58 	irqf2p[1] 	High Level 	—
59 	irqf2p[2] 	High Level 	—
60 	irqf2p[3] 	High Level 	—
61 	irqf2p[4] 	High Level 	
62 	irqf2p[5] 	High Level 	—
63 	irqf2p[6] 	High Level 	—
64 	irqf2p[7] 	High Level	

84 	irqf2p[8] 	High Level 	—
85 	irqf2p[9] 	High Level 	—
86 	irqf2p[10] 	High Level 	—
87 	irqf2p[11] 	High Level 	—
88 	irqf2p[12] 	High Level 	—
89 	irqf2p[13] 	High Level 	—
90 	irqf2p[14] 	High Level 	—
91 	irqf2p[15] 	High Level	
------------------------------
地址配置如下：
	4000_0000 to 7FFF_FFFF PL PL General Purpose Port #0 to the PL, M_AXI_GP0
	8000_0000 to BFFF_FFFF PL PL General Purpose Port #1 to the PL, M_AXI_GP1
*/
#define INT_VEC_IRQF2P_0            IVEC_TO_INUM(57)
#define INT_VEC_IRQF2P_1            IVEC_TO_INUM(58)
#define INT_VEC_IRQF2P_2            IVEC_TO_INUM(59)
#define INT_VEC_IRQF2P_3            IVEC_TO_INUM(60)
#define INT_VEC_IRQF2P_4            IVEC_TO_INUM(61)
#define INT_VEC_IRQF2P_5            IVEC_TO_INUM(62)
#define INT_VEC_IRQF2P_6            IVEC_TO_INUM(63)
#define INT_VEC_IRQF2P_7            IVEC_TO_INUM(64)

#define INT_VEC_IRQF2P_8            IVEC_TO_INUM(84)
#define INT_VEC_IRQF2P_9            IVEC_TO_INUM(85)
#define INT_VEC_IRQF2P_10           IVEC_TO_INUM(86)
#define INT_VEC_IRQF2P_11           IVEC_TO_INUM(87)
#define INT_VEC_IRQF2P_12           IVEC_TO_INUM(88)
#define INT_VEC_IRQF2P_13           IVEC_TO_INUM(89)
#define INT_VEC_IRQF2P_14           IVEC_TO_INUM(90)
#define INT_VEC_IRQF2P_15           IVEC_TO_INUM(91)


/* signals generated from various clock generators */
#define FMQL_UART_CLK              (100000000)   /* UART clock 100Mhz      */
#define FMQL_TTC_CLK               (175000000)   /* ttc timer clock        */

#define FMQL_TIMERS_CLK           (100000000)   /* System clock           */
#define FMQL_APB_CLK              (100000000)   /* APB System clock       */


/* definitions for the AMBA UART */
#define N_FMQL_UART_CHANNELS        (2)  /* 1 -> 2*/

#define UART_XTAL_FREQ              (FMQL_UART_CLK)
#define N_SIO_CHANNELS              (N_FMQL_UART_CHANNELS)
#define N_UART_CHANNELS             (N_FMQL_UART_CHANNELS)

#ifdef UART_1_AS_CONSOLE  /* logic_uart1 use phy_uart0 */
#define UART_0_BASE_ADR             (0xE0023000)         /* UART 0 base addr */
#define UART_1_BASE_ADR             (0xE0004000)         /* UART 1 base addr */
#else /* uart_0 as console*/
#define UART_0_BASE_ADR             (0xE0004000)         /* UART 0 base addr */
#define UART_1_BASE_ADR             (0xE0023000)         /* UART 1 base addr */
#endif

#define FMQL_UART_REGS_SIZE         (SZ_4K)              /* timer regs size  */

/* definitions for the Cortex A9 Core Private Timer */
#define FMQL_TTC0_TIMER0_BASE     (0xE0007000)         /* ttc 0 timer 0 */
#define FMQL_TIMER_REGS_SIZE      (SZ_4K)              /* timer regs size  */

#define FMQL_TTC1_TIMER0_BASE     (0xE0024000)         /* ttc 1 timer 0 */

#define FMQL_GTC_TIMER_BASE       (0xE0008000)         /* arm generic timer(GTC) */



/* Ethernet Base Address */
#define FMQL_GEM0_BASE             (0xE0047000)       /* size: 0x2000 */
#define FMQL_GEM1_BASE             (0xE0049000)       /* size: 0x2000 */

#define EMAC_0_BASE_ADRS           FMQL_GEM0_BASE
#define EMAC_1_BASE_ADRS           FMQL_GEM1_BASE

/*
QSPI
*/
#define QSPI0_D_BASE_ADRS        (0xE8000000)
#define QSPI1_D_BASE_ADRS        (0xE9000000)

#define QSPI0_BASE_ADRS          (0xE0000000)
#define QSPI1_BASE_ADRS          (0xE0020000)

/*
spi
*/
#define SPI0_BASE_ADRS          (0xE0001000)
#define SPI1_BASE_ADRS          (0xE0021000) 

/*
can
*/
#define CAN0_BASE_ADRS          (0xE0005000)
#define CAN1_BASE_ADRS          (0xE0006000) 

/*
nfc
*/
#define NFC_S_BASE_ADRS          (0xE0042000)

/*
I2C
*/
#define I2C_0_BASE_ADRS         (0xE0002000)
#define I2C_1_BASE_ADRS         (0xE0022000)

/*
WDT(watchdog timer)
*/
#define WDT_0_BASE_ADRS         (0xE0025000)            /* Watchdog 0 Module 0xE0025000 */

/*
GPIO
*/
#define GPIO_0_BASE_ADRS        (0xE0003000)
#define GPIO_1_BASE_ADRS        (0xE0003100)
#define GPIO_2_BASE_ADRS        (0xE0003200)
#define GPIO_3_BASE_ADRS        (0xE0003400)

/*
49	SPI	GPIO0	
68	SPI	GPIO1		
69	SPI	GPIO2		
70	SPI	GPIO3	*/
#define INT_VEC_GPIO_BANK_A           IVEC_TO_INUM(49)       /* BANK_A Interrupt */
#define INT_VEC_GPIO_BANK_B           IVEC_TO_INUM(68)       /* BANK_B Interrupt */
#define INT_VEC_GPIO_BANK_C           IVEC_TO_INUM(69)       /* BANK_C Interrupt */
#define INT_VEC_GPIO_BANK_D           IVEC_TO_INUM(70)       /* BANK_D Interrupt */


/* 
SD/MMC 
*/
#define SDMMC_CLK_400_KHZ          400000    /* 400 KHZ */
#define SDMMC_CLK_50_MHZ         50000000    /* 50 MHZ  */
#define SDMMC_CLK_100_MHZ       100000000    /* 100 MHZ  */

#ifdef SD_CTRL1_AS_EMMC
#define FM_SDMMC_0_BASE                  (0xE0044000)  /* ctrl_1-->emmc for wh-yuhang*/
#else
#define FM_SDMMC_0_BASE                  (0xE0043000)
#endif
#define FM_SDMMC_1_BASE                  (0xE0044000)



#define FM_SDMMC_CLK_VAL               (SDMMC_CLK_100_MHZ)

#define FM_SDMMC_CLK_DIVIDED_VAL       (FM_SDMMC_CLK_VAL / 4)  /* 25M*/
/*#define FM_SDMMC_CLK_DIVIDED_VAL       (FM_SDMMC_CLK_VAL / 2)    // 50M*/
/*#define FM_SDMMC_CLK_DIVIDED_VAL       (FM_SDMMC_CLK_VAL / 10)    // 5M*/


/* 
USB 
*/
#define USB_0_BASE_ADRS         (0xE0045000)
#define USB_1_BASE_ADRS         (0xE0046000)

/* Timer */
#define GTIMER_ADDR_BASE	    (0xE0008000)


/* frequency of counter/timers */
#define SYS_CLK_FREQ            (FMQL_TIMERS_CLK)



/* General Purpose Port to the PL */

#define FMQL_AXI_GP0_BASE       (0x40000000)
#define FMQL_AXI_GP0_SIZE       (SZ_1G)

#define FMQL_AXI_GP1_BASE       (0x80000000)
#define FMQL_AXI_GP1_SIZE       (SZ_1G)


#define SYS_TIMER_BASE          (FMQL_TTC0_TIMER0_BASE)
#define AUX_TIMER_BASE          (FMQL_TTC1_TIMER0_BASE)


/*****************************************************************************/

/* Handy sizes */
#define SZ_1K                       (0x00000400)
#define SZ_4K                       (0x00001000)
#define SZ_8K                       (0x00002000)
#define SZ_16K                      (0x00004000)
#define SZ_64K                      (0x00010000)
#define SZ_128K                     (0x00020000)
#define SZ_256K                     (0x00040000)
#define SZ_512K                     (0x00080000)
#define SZ_1M                       (0x00100000)
#define SZ_2M                       (0x00200000)
#define SZ_4M                       (0x00400000)
#define SZ_8M                       (0x00800000)
#define SZ_16M                      (0x01000000)
#define SZ_32M                      (0x02000000)
#define SZ_64M                      (0x04000000)
#define SZ_128M                     (0x08000000)
#define SZ_256M                     (0x10000000)
#define SZ_512M                     (0x20000000)
#define SZ_1G                       (0x40000000)
#define SZ_2G                       (0x80000000)
#define SCTLR_BE                    (0x02000000)

#define ARM_IMM                     #

/* level of coherency mask of CLIDR */

#define ARM_LOC_MASK                #0x7000000

/* Auxiliary Control Register */

/* cache and tlb maintenance broadcast */

#define AUX_CTL_REG_SMP              (0x00000040)
#define GTIMER_ADDR_BASE             (0xE0008000)

#define FMQL_REGISTER_READ(reg)  (*(volatile UINT32 *)(reg))

#define FMQL_REGISTER_WRITE(reg, data) (*(volatile UINT32 *)(reg) = (data))

#define FMQL_SR_REGISTER_READ(reg, val) FMQL_REGISTER_READ(reg)
    
#define FMQL_SR_REGISTER_WRITE(reg, data)                        \
    do {                                                                \
    FMQL_REGISTER_WRITE (FMQL_SR_UNLOCK, FMQL_SR_UNLOCK_ALL);\
    *(volatile UINT32 *)(reg) = (data);                                 \
    FMQL_REGISTER_WRITE (FMQL_SR_LOCK, FMQL_SR_LOCK_ALL);    \
    } while (0)

/*
 * Common code for cache operations on entire data/unified caches,
 * performed by set/way to the point of coherency (PoC).
 * This code is based on 'Example code for cache maintenance operations'
 * provided in "ARM Architecture Reference Manual ARMv7-A and ARMv7-R edition
 * (ARM DDI 0406)" .
 *
 * Registers used: r0-r8. Also note that r0 is 0 when this code completes.
 */

#undef _CORTEX_AR_ENTIRE_DATA_CACHE_OP
#define _CORTEX_AR_ENTIRE_DATA_CACHE_OP(crm) \
                    \
    MRC    p15, 1, r0, c0, c0, 1    /* r0 = Cache Lvl ID register info */;\
    ANDS   r3, r0, ARM_LOC_MASK     /* get level of coherency (LoC) */;\
    MOV    r3, r3, LSR ARM_IMM 23   /* r3 = LoC << 1 */;\
    BEQ    5f            ;\
                    \
    MOV    r7, ARM_IMM 0            /* r7 = cache level << 1; start at 0 */;\
                    \
1:                    ;\
    AND    r1, r0, ARM_IMM 0x7      /* r1 = cache type(s) for this level */;\
    CMP    r1, ARM_IMM 2        ;\
    BLT    4f                       /* no data cache at this level */;\
                    \
    MCR    p15, 2, r7, c0, c0, 0    /* select the Cache Size ID register */;\
    MCR    p15, 0, r7, c7, c5, 4    /* ISB: sync change to Cache Size ID */;\
    MRC    p15, 1, r1, c0, c0, 0    /* r1 = current Cache Size ID info */;\
    AND    r2, r1, ARM_IMM 0x7      /* r2 = line length */;\
    ADD    r2, r2, ARM_IMM 4        /* add line length offset = log2(16 bytes) */;\
    LDR    r4, =0x3FF        ;\
    ANDS   r4, r4, r1, LSR ARM_IMM 3 /* r4 = (# of ways - 1); way index */;\
    CLZ    r5, r4                    /* r5 = bit position of way size increment */;\
    LDR    r6, =0x00007FFF        ;\
    ANDS    r6, r6, r1, LSR ARM_IMM 13/* r6 = (# of sets - 1); set index */;\
                    \
2:                    ;\
    MOV    r1, r4                 /* r1 = working copy of way number */;\
3:                    ;\
    ORR    r8, r7, r1, LSL r5     /* r8 = set/way operation data word: */;\
    ORR    r8, r8, r6, LSL r2     /* cache level, way and set info */;\
                    \
    MCR    p15, 0, r8, c7, crm, 2 /* dcache operation by set/way */;\
                    \
    SUBS   r1, r1, ARM_IMM 1      /* decrement the way index */;\
    BGE    3b            ;\
    SUBS   r6, r6, ARM_IMM 1      /* decrement the set index */;\
    BGE    2b            ;\
                    \
4:                    ;\
    ADD    r7, r7, ARM_IMM 2      /* increment cache index = level << 1 */;\
    CMP    r3, r7                 /* done when LoC is reached */;\
    MOVGT  r0, r0, LSR ARM_IMM 3  /* rt-align type of next cache level */;\
    BGT    1b            ;\
                    \
5:                    ;
/*****************************************************************************/




#ifdef __cplusplus
}
#endif

#endif    /* __INCfmsh_fmqlh */

/*
寄存器基地址 空间大小 设备名
----------------------------
0xE000_0000 4KB QSPI0
0xE000_1000 4KB SPI0
0xE000_2000 4KB I2C0
0xE000_3000 4KB GPIO
0xE000_4000 4KB UART0
0xE000_5000 4KB CAN0
0xE000_6000 4KB CAN1
0xE002_0000 4KB QSPI1
0xE002_1000 4KB SPI1
0xE002_2000 4KB I2C1
0xE002_3000 4KB UART1
0xE002_4000 4KB TIMER1
0xE002_5000 4KB WDT
0xE002_6000 4KB SLCR
0xE002_7000 8KB DMC
0xE004_1000 4KB SMC
0xE004_2000 4KB NFC
0xE004_3000 4KB SDIO0
0xE004_4000 4KB SDIO1
0xE004_5000 4KB USB0
0xE004_6000 4KB USB1
0xE004_7000 4KB GMAC0
0xE004_9000 4KB GMAC1

0xF890_0000: Generic Interrupt Controller
*/

#if 0
/*
   PPI
--------
中断ID	类型	中断说明	触发方式	验证结果
25	PPI	Virtual Maintenance Interrupt (PPI6)	电平	
26	PPI	Hypervisor Timer event (PPI5)	电平	
27	PPI	Virtual Timer event (PPI4)	电平	
28	PPI	Legacy nFIQ signal (PPI0)	电平，低电平有效	PASS
29	PPI	Secure Physical Timer event (PPI1)	电平	
30	PPI	Non-secure Physical Timer event (PPI2)	电平	
31	PPI	Legacy nIRQ signal (PPI3) 	电平，低电平有效	PASS
*/

/*
	SPI
--------
中断ID	类型	具体应用	触发方式	验证结果
40	SPI	DEVC		
41	SPI	WDT(WDT_intr_n)		
42	SPI	TIMER0(TIMER0_intr1_n)		
43	SPI	TIMER0(TIMER0_intr2_n)		
44	SPI	TIMER0(TIMER0_intr3_n)		
45	SPI	DMA		
46	SPI	QSPI0		
47	SPI	QSPI1		
48	SPI	NFC		
49	SPI	GPIO0		
50	SPI	USB0		
51	SPI	GMAC0		
52	SPI	SDMCC0		
53	SPI	I2C0		
54	SPI	SPI0		
55	SPI	UART0		
56	SPI	CAN0

57	SPI	PL0		
58	SPI	PL1		
59	SPI	PL2		
60	SPI	PL3		
61	SPI	PL4		
62	SPI	PL5		
63	SPI	PL6		
64	SPI	PL7	

65	SPI	TIMER1(TIMER0_intr1_n)		
66	SPI	TIMER1(TIMER0_intr2_n)		
67	SPI	TIMER1(TIMER0_intr3_n)		
68	SPI	GPIO1		
69	SPI	GPIO2		
70	SPI	GPIO3		
71	SPI	USB1		
72	SPI	GMAC1		
73	SPI	SDMCC1		
74	SPI	I2C1		
75	SPI	SPI1		
76	SPI	UART1		
77	SPI	CAN1	

84	SPI	PL8		
85	SPI	PL9		
86	SPI	PL10		
87	SPI	PL11		
88	SPI	PL12		
89	SPI	PL13		
90	SPI	PL14		
91	SPI	PL15	
*/

#define FPS_DDR3MEM_BASEADDR         (0x00100000)
#define FPS_GIC_BASEADDR             (0xF8900000)

/* AHB */
#define FPS_AHB2APB0_BASEADDR        (0xE0000000)
#define FPS_AHB2APB1_BASEADDR        (0xE0020000)
#define FPS_CSU_BASEADDR             (0xE0040000)
#define FPS_SMC_CTL_BASEADDR         (0xE0041000)
#define FPS_SMC_NORSRAM0_BASEADDR    (0xE2000000)
#define FPS_SMC_NORSRAM1_BASEADDR    (0xE4000000)
#define FPS_QSPI0_D_BASEADDR         (0xE8000000)
#define FPS_QSPI1_D_BASEADDR         (0xE9000000)
#define FPS_NFC_S_BASEADDR           (0xE0042000)
#define FPS_SDMMC0_BASEADDR          (0xE0043000)
#define FPS_SDMMC1_BASEADDR          (0xE0044000)
#define FPS_USB0_BASEADDR            (0xE0045000)
#define FPS_USB1_BASEADDR            (0xE0046000)
#define FPS_GMAC0_BASEADDR           (0xE0047000)
#define FPS_GMAC1_BASEADDR           (0xE0049000)
#define FPS_DMA_BASEADDR             (0xE004B000)
#define FPS_AHB_SRAM_BASEADDR        (0xE1FE0000)

/* APB0 */
#define FPS_QSPI0_BASEADDR           (0xE0000000)
#define FPS_SPI0_BASEADDR            (0xE0001000)

#define FPS_I2C0_BASEADDR            (0xE0002000)

#define FPS_GPIO_BASEADDR            (0xE0003000)

#define FPS_UART0_BASEADDR           (0xE0004000)
#define FPS_CAN0_BASEADDR            (0xE0005000)
#define FPS_CAN1_BASEADDR            (0xE0006000)

#define FPS_TIMER0_BASEADDR          (0xE0007000)
#define FPS_GTC_BASEADDR             (0xE0008000)  /* arm-gen-timer */
#define FPS_PRCG_BASEADDR            (0xE0009000)

/* APB1 */
#define FPS_QSPI1_BASEADDR           (0xE0020000)
#define FPS_SPI1_BASEADDR            (0xE0021000)

#define FPS_I2C1_BASEADDR            (0xE0022000)
#define FPS_UART1_BASEADDR           (0xE0023000)
#define FPS_TIMER1_BASEADDR          (0xE0024000)
#define FPS_WDT_BASEADDR             (0xE0025000)
#define FPS_SLCR_BASEADDR            (0xE0026000)
#define FPS_DDR3_CTL_BASEADDR        (0xE0027000)

#endif

