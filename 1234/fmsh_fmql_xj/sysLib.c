/* sysLib.c - FMSH FMQL board system-dependent library */

/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
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
This library provides board-specific routines for the
FMSH FMQL board BSP
*/

/* includes */

#include <vxWorks.h>
#include <stdio.h>		/* kprintf */
#include <private/kwriteLibP.h>	/* _func_kwrite */

#include "config.h"

#if !defined (INCLUDE_MMU) && \
    (defined (INCLUDE_CACHE_SUPPORT) || defined (INCLUDE_MMU_BASIC) || \
     defined (INCLUDE_MMU_FULL) || defined (INCLUDE_MMU_MPU) || \
     defined (INCLUDE_MMU_GLOBAL_MAP))
     
#define INCLUDE_MMU
#endif

#include <sysLib.h>
#include <string.h>
#include <intLib.h>
#include <taskLib.h>
#include <vxLib.h>
#include <muxLib.h>
#include <cacheLib.h>

#ifdef INCLUDE_MMU
#   include <arch/arm/mmuArmLib.h>
#   include <private/vmLibP.h>
#endif /* INCLUDE_MMU */
#include <dllLib.h>

#include <private/windLibP.h>
#include "global_baud.h"
#include "16550.c"
#include "State_machine.c"
#include "Data_Send_Rece.c"
#include "tcpnew.c"
#include "Command_channel.c"
#include "helloworld.c"
#include "oled.h"
#include "oled.c"



#ifdef INCLUDE_VXIPI
#   include <vxIpiLib.h>
#endif /* INCLUDE_VXIPI */

#ifdef _WRS_CONFIG_SMP
#   include <arch/arm/vxAtomicArchLib.h>
#endif /* _WRS_CONFIG_SMP */

#include <hwif/intCtlr/vxbArmGenIntCtlr.h>

#include <hwif/vxbus/vxBus.h>
#include <stdio.h>		/* kprintf */

/* 
all drv include at here
*/
#include "sysInc_ALL.h"


/* 
imports 
*/
IMPORT void hardWareInterFaceInit (void);

#ifdef INCLUDE_SIO_UTILS
IMPORT void sysSerialConnectAll (void);
#endif /* INCLUDE_SIO_UTILS */

#ifdef INCLUDE_CACHE_SUPPORT
IMPORT void cacheCortexA9LibInstall (VIRT_ADDR (physToVirt)(PHYS_ADDR),
                                    PHYS_ADDR (virtToPhys)(VIRT_ADDR));
IMPORT void cacheCortexA15LibInstall (VIRT_ADDR (physToVirt)(PHYS_ADDR),
                                    PHYS_ADDR (virtToPhys)(VIRT_ADDR));
#endif /* INCLUDE_CACHE_SUPPORT */

#ifdef INCLUDE_MMU
IMPORT void mmuCortexA8LibInstall (VIRT_ADDR (physToVirt)(PHYS_ADDR),
                                  PHYS_ADDR (virtToPhys)(VIRT_ADDR));
#endif /* INCLUDE_MMU */


/*
globals 
*/
#ifdef INCLUDE_MMU

/*
 * The following structure describes the various different parts of the
 * memory map to be used only during initialization by
 * vm(Base)GlobalMapInit () when INCLUDE_MMU_BASIC/FULL/GLOBAL_MAP are
 * defined.
 *
 * Clearly, this structure is only needed if the CPU has an MMU!
 *
 * The following are not the smallest areas that could be allocated for a
 * working system. If the amount of memory used by the page tables is
 * critical, they could be reduced.
 */

PHYS_MEM_DESC sysPhysMemDesc [] =
{
	/* 
	 * TODO: Interrupt vector table alwalys at address 0 while SMP booting ???????
	 * temporary mmap physical address to 0 as a workaround.
	 * Must be find out the root cause !!!
	 */
#if 1  /* OCM */
	{
		0,    /* virtual address */
		0x100000,    /* physical address */
		SZ_1M,  /* SZ_128K */
		MMU_ATTR_VALID_MSK | MMU_ATTR_PROT_MSK | MMU_ATTR_CACHE_MSK,
#ifdef _WRS_CONFIG_SMP       /* needs to be shared */
		MMU_ATTR_VALID     | MMU_ATTR_SUP_RWX  | MMU_ATTR_CACHE_DEFAULT
		| MMU_ATTR_CACHE_COHERENCY
#else
		MMU_ATTR_VALID     | MMU_ATTR_SUP_RWX  | MMU_ATTR_CACHE_DEFAULT
#endif /* _WRS_CONFIG_SMP */
	},
#endif

#ifdef FMQL_RESOURCE_SRAM
	/* SRAM */
    {
		FMQL_SRAM_MEM_ADRS,    /* virtual address */
		FMQL_SRAM_MEM_ADRS,    /* physical address */
		FMQL_SRAM_MEM_SIZE,
	    MMU_ATTR_VALID_MSK | MMU_ATTR_PROT_MSK | MMU_ATTR_CACHE_MSK,
#ifdef _WRS_CONFIG_SMP       /* needs to be shared */
	    MMU_ATTR_VALID     | MMU_ATTR_SUP_RWX  | MMU_ATTR_CACHE_DEFAULT
	    | MMU_ATTR_CACHE_COHERENCY
#else
	    MMU_ATTR_VALID     | MMU_ATTR_SUP_RWX  | MMU_ATTR_CACHE_DEFAULT
#endif /* _WRS_CONFIG_SMP */
    },
#endif  /* FMQL_RESOURCE_SRAM */
    
    /* DRAM */
    {
	    LOCAL_MEM_LOCAL_ADRS,    /* virtual address */
	    LOCAL_MEM_LOCAL_ADRS,    /* physical address */
	    LOCAL_MEM_SIZE,
	    MMU_ATTR_VALID_MSK | MMU_ATTR_PROT_MSK | MMU_ATTR_CACHE_MSK,
#ifdef _WRS_CONFIG_SMP       /* needs to be shared */
	    MMU_ATTR_VALID     | MMU_ATTR_SUP_RWX  | MMU_ATTR_CACHE_DEFAULT
	    | MMU_ATTR_CACHE_COHERENCY
#else
	    MMU_ATTR_VALID     | MMU_ATTR_SUP_RWX  | MMU_ATTR_CACHE_DEFAULT
#endif /* _WRS_CONFIG_SMP */
    },


#ifdef INCLUDE_RTP
    /* map the first 2/3 extened 768M DRAM memory*/
    {
	    FMQL_MEM_EXTRA,       /* virtual address */
	    FMQL_MEM_EXTRA,       /* physical address */
	    ROUND_UP (FMQL_MEM_EXTRA_SIZE / 3 * 2, PAGE_SIZE),
	    MMU_ATTR_VALID_MSK | MMU_ATTR_PROT_MSK | MMU_ATTR_CACHE_MSK,
#ifdef _WRS_CONFIG_SMP      /* needs to be shared */
	    MMU_ATTR_VALID     | MMU_ATTR_SUP_RWX  | MMU_ATTR_CACHE_DEFAULT
	    | MMU_ATTR_CACHE_COHERENCY
#else
	    MMU_ATTR_VALID     | MMU_ATTR_SUP_RWX  | MMU_ATTR_CACHE_DEFAULT
#endif /* _WRS_CONFIG_SMP */
    },
#else
    /* map the extened 768M DRAM memory */
    {
	    FMQL_MEM_EXTRA,       /* virtual address */
	    FMQL_MEM_EXTRA,       /* physical address */
	    FMQL_MEM_EXTRA_SIZE,  /* length, then initial state: */
	    MMU_ATTR_VALID_MSK | MMU_ATTR_PROT_MSK | MMU_ATTR_CACHE_MSK,
#ifdef _WRS_CONFIG_SMP      /* needs to be shared */
	    MMU_ATTR_VALID     | MMU_ATTR_SUP_RWX  | MMU_ATTR_CACHE_DEFAULT
	    | MMU_ATTR_CACHE_COHERENCY
#else
	    MMU_ATTR_VALID     | MMU_ATTR_SUP_RWX  | MMU_ATTR_CACHE_DEFAULT
#endif /* _WRS_CONFIG_SMP */
    },
#endif /* INCLUDE_RTP */

	/* GIC */
    {
	    FMQL_GIC_BASE,    /* gic: 0xF890_0000 */
	    FMQL_GIC_BASE,
	    SZ_16K,
	    MMU_ATTR_VALID_MSK | MMU_ATTR_PROT_MSK | MMU_ATTR_DEVICE_SHARED_MSK,
	    MMU_ATTR_VALID     | MMU_ATTR_SUP_RWX  | MMU_ATTR_DEVICE_SHARED
    },



	/***************************************************************************/
    /* 0xE000_0000 ~ 0xE004_8000 ~ 0xE010_0000 */
	/* Peripheral Ctrl Memory: 1M */
#if 1
	{
		0xE0000000, 	 /* 1M */
		0xE0000000,
		SZ_1M,
		MMU_ATTR_VALID_MSK | MMU_ATTR_PROT_MSK | MMU_ATTR_DEVICE_SHARED_MSK,
		MMU_ATTR_VALID	   | MMU_ATTR_SUP_RWX  | MMU_ATTR_DEVICE_SHARED
	},
#endif
	
#ifdef DRV_FM_QSPI  /* FMQL QSPI0_D: 16M 0xE800_0000 ~ 0xE900_0000 */
	/* qspi_0: fifo_addr */
	{
		QSPI0_D_BASE_ADRS,	
		QSPI0_D_BASE_ADRS,
		SZ_16M,
		MMU_ATTR_VALID_MSK | MMU_ATTR_PROT_MSK | MMU_ATTR_DEVICE_SHARED_MSK,
		MMU_ATTR_VALID	   | MMU_ATTR_SUP_RWX  | MMU_ATTR_DEVICE_SHARED
	},
#endif

#ifdef DRV_FM_QSPI	/* FMQL QSPI1_D: 16M */
	{
		QSPI1_D_BASE_ADRS,	
		QSPI1_D_BASE_ADRS,
		SZ_16M,
		MMU_ATTR_VALID_MSK | MMU_ATTR_PROT_MSK | MMU_ATTR_DEVICE_SHARED_MSK,
		MMU_ATTR_VALID	   | MMU_ATTR_SUP_RWX  | MMU_ATTR_DEVICE_SHARED
	},
#endif	

#if 1  /* AHB_SRAM - 128KB - 0xE1FE_0000~~0xE1FF_FFFF */
	{
		FPS_AHB_SRAM_BASEADDR,	/* FMQL AHB_SRAM: 0xE1FE0000, 128K */
		FPS_AHB_SRAM_BASEADDR,
		SZ_128K,
		MMU_ATTR_VALID_MSK | MMU_ATTR_PROT_MSK | MMU_ATTR_DEVICE_SHARED_MSK,
		MMU_ATTR_VALID	   | MMU_ATTR_SUP_RWX  | MMU_ATTR_DEVICE_SHARED
	},
#endif
	
#ifdef FMQL_RESOURCE_FPGA
    {
    FMQL_AXI_GP0_BASE,  /* 0x4000_0000: 1G, AXI Memory block of GP0 to the PL */
    FMQL_AXI_GP0_BASE,
    FMQL_AXI_GP0_SIZE,
    MMU_ATTR_VALID_MSK | MMU_ATTR_PROT_MSK | MMU_ATTR_DEVICE_SHARED_MSK,
    MMU_ATTR_VALID | MMU_ATTR_SUP_RWX | MMU_ATTR_DEVICE_SHARED
    },

    {
    FMQL_AXI_GP1_BASE,  /* 0x8000_0000: 1G, AXI Memory block of GP1 to the PL */
    FMQL_AXI_GP1_BASE,
    FMQL_AXI_GP1_SIZE,
    MMU_ATTR_VALID_MSK | MMU_ATTR_PROT_MSK | MMU_ATTR_DEVICE_SHARED_MSK,
    MMU_ATTR_VALID | MMU_ATTR_SUP_RWX | MMU_ATTR_DEVICE_SHARED
    },
#endif
};

int sysPhysMemDescNumEnt = NELEMENTS (sysPhysMemDesc);

#endif /* INCLUDE_MMU */

/* locals */

#ifdef _WRS_CONFIG_SMP

/* constant used with IPI to request reboot of application processors */
#define APPCORE_REBOOT  0xFEFEFEF0

/* constant used with IPI to request core0 reboot */
#define BOOTCORE_REBOOT 0x45600000

LOCAL UINT32 rebootVar = 0;     /* it passes the CPU index */

/* it protect from multiple reboots */

LOCAL SPIN_LOCK_ISR_DECL (rebootLock, 0);

/* Non-Boot CPU Start info. Managed by sysCpuEnable */

struct sysMPCoreStartup
    {
    UINT32      newPC;          /* Address of 'C' based startup code */
    UINT32      newSP;          /* Stack pointer for startup */
    UINT32      newArg;         /* vxWorks kernel entry point */
    UINT32      newSync;        /* Translation Table Base and sync */
    };

extern struct sysMPCoreStartup sysMPCoreStartup[VX_SMP_NUM_CPUS];

#endif /* _WRS_CONFIG_SMP */

/* externals */

#ifndef _ARCH_SUPPORTS_PROTECT_INTERRUPT_STACK
IMPORT void sysIntStackSplit (char *, long);
#endif /* !_ARCH_SUPPORTS_PROTECT_INTERRUPT_STACK */

/* forward declarations */

void    sysUsDelay (int);
void    sysMsDelay (int);
void    sysDelay ();

IMPORT void (*_vxb_usDelayRtn) (int delayTime);
IMPORT void (*_vxb_msDelayRtn) (int delayTime);
IMPORT void (*_vxb_delayRtn) (void);

IMPORT void mmuCortexA8TtbrSetAll (UINT32, UINT32);
IMPORT void   mmuCortexA8AcrSet (UINT32 acrVal);

#ifdef _WRS_CONFIG_SMP
IMPORT void   mmuCortexA8DacrSet (UINT32 dacrVal);
IMPORT STATUS sysArmGicDevInit (void);
IMPORT void   mmuCortexA8AEnable (UINT32 cacheState);
IMPORT void   mmuCortexA8ADisable (void);
IMPORT void   armInitExceptionModes (void);
IMPORT void   sysCpuInit (void);
IMPORT void   cacheCortexA9MPCoreSMPInit (void);
IMPORT void   mmuCortexA8TLBIDFlushAll (void);
IMPORT MMU_LEVEL_1_DESC * mmuCortexA8TtbrGet (void);

IMPORT STATUS cacheArchLibInit(CACHE_MODE	instMode,   CACHE_MODE	dataMode);

UINT32        sysCpuAvailableGet (void);
STATUS        sysCpuEnable (unsigned int, void (* startFunc) (void), char *);
#endif /* _WRS_CONFIG_SMP */


/* 
included source files 
*/
#include "nvRamToFlash.c"
#include "sysBspCommon.c"
#include "hwconf.c"


#ifdef INCLUDE_VFP
IMPORT STATUS vfpEnable (void);
IMPORT STATUS vfpDisable (void);
#endif

LOCAL volatile UINT32 * uartBase = NULL;

#define UART_RBR_THR	(0)
#define UART_USR		(0x7c >> 2)
#define UART_TXFIFO_EMPTY	(0x04)
LOCAL STATUS fmqlDebugWrite
    (
    char *      buf,
    size_t      len
    )
    {
    char        ch;

    if ((buf == NULL) || (uartBase == NULL))
        return ERROR;

    ch = *buf++;

    while (len--)
	/*while (ch != EOS)*/
        {
        while ((uartBase [UART_USR] & UART_TXFIFO_EMPTY) == 0);
        uartBase [UART_RBR_THR] = ch;

        if (ch == '\n')
            {
            while ((uartBase [UART_USR] & UART_TXFIFO_EMPTY) == 0);
            uartBase [UART_RBR_THR] = '\r';
            }

        ch = *buf++;
        }

    return OK;
    }


void fmql_16550Pollprintf(char* str)
{
	fmqlDebugWrite(str,0);
	return;
}

void fmprintf(char* str)
{
	fmql_16550Pollprintf(str);
	return;
}


#ifdef  DRV_UART_1_OPEN
/*
uart_1
*/
LOCAL volatile UINT32 * uartBase2 = NULL;
LOCAL STATUS fmqlDebugWrite2
    (
    UINT8 *      buf,
    size_t      len
    )
{
    UINT8        ch;
	UINT32 tmp = 0;
	int i = 0;
	int timeout = 0;
	
	uartBase2 = (UINT32*)UART_0_BASE_ADR;  /* (0xE0023000);*/

    if ((buf == NULL) || (uartBase2 == NULL))
        return ERROR;
	
	/*
	uartBase2 [0x88>>2] = 0x07;
	uartBase2 [0x98>>2] = 0x01;
	
	uartBase2[0] = 0x00;
	uartBase2[1] = 0x03;
	uartBase2[2] = 0xC1;
	uartBase2[3] = 0x03;
	
	uartBase2[4] = 0x08;
	uartBase2[5] = 0x00;
	uartBase2[6] = 0xf0;
	uartBase2[7] = 0x00;
	*/
	/*
	for (i=0; i<0x100/4; i++)
	{
		uartBase2[i] = uartBase[i];
	}
	*/

    ch = *buf++;

    while (len--)
    {
    	
        /*while (((tmp = uartBase2 [UART_USR]) & UART_TXFIFO_EMPTY) == 0);*/
        timeout = 10000;
        do 
		{
			tmp = uartBase2 [UART_USR];

			timeout--;
			if (timeout <= 0)
			{
				break;
			}		
        } while ((tmp & UART_TXFIFO_EMPTY) == 0);
		
        uartBase2 [UART_RBR_THR] = ch;

        if (ch == '\n')
        {
            /*while ((uartBase2 [UART_USR] & UART_TXFIFO_EMPTY) == 0);*/
			
			timeout = 10000;
			do 
			{
				tmp = uartBase2 [UART_USR];
				
				timeout--;
				if (timeout <= 0)
				{
					break;
				}			
			} while ((tmp & UART_TXFIFO_EMPTY) == 0);
            
            uartBase2 [UART_RBR_THR] = '\r';
        }

        ch = *buf++;
		
		for (i=0; i<1000; i++);  /* delay*/
    }

    return OK;
}

int g_uart2_test = 0;
void test_uart2_put(void)
{
	char buf[256] = {0};
	sprintf((char*)(&buf[0]), "abcdefghijklmn-%d \n", g_uart2_test++);
	
	printf("console_output: %s \n", (char*)(&buf[0]));	/* at uart_0*/
	taskDelay(10);

	fmqlDebugWrite2((UINT8*)(&buf[0]), 32);  /* at uart_1*/
	return;
}

void test_uart1_put(void)
{
	char buf[256] = {0};
	sprintf(buf, "abcdefg_idx-%d \n", g_uart2_test++);
	
	printf("console_output: %s \n", (char*)(&buf[0]));	/* at uart_0	*/
	taskDelay(10);
	
	fmqlDebugWrite((char*)(&buf[0]), 16);  /* at uart_1*/
	return;
}

#endif

/*******************************************************************************
*
* sysHwInit0 - perform early BSP-specific initialization
*
* This routine performs such BSP-specific initialization as is necessary before
* the architecture-independent cacheLibInit can be called. It is called
* from usrInit () before cacheLibInit (), before sysHwInit () and before BSS
* has been cleared.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
#include <cacheLib.h>

#if 1
/*
generic timer clk freq: OSC_HZ div2
*/
#define GTC_CLK_FREQ_2    (HARD_OSC_HZ)

void gtc_enable(void)
{
 	FMSH_WriteReg(GTIMER_ADDR_BASE, 0, 1);
}

UINT64 get_gtc_time(void)   
{
	UINT32 low = 0, high = 0;
	UINT64 ret;

	/* Reading Generic Timer Counter Register */
	do
	{
		high = FMSH_ReadReg(GTIMER_ADDR_BASE, 0x0C);
		low = FMSH_ReadReg(GTIMER_ADDR_BASE, 0x08);
	} while (FMSH_ReadReg(GTIMER_ADDR_BASE, 0x0C) != high);

	ret = (((UINT64) high) << 32U) | (UINT64) low;

	return ret;
}
#endif


void sysHwInit0 (void)
{
#if 0

#ifdef _WRS_CONFIG_SMP
    volatile UINT32 scuVal;

    /* setup AUX register and announce we are part of SMP */

    mmuCortexA8AcrSet (AUX_CTL_REG_FW | AUX_CTL_REG_L1_PRE_EN);

    /* enable SCU */

    scuVal = FMQL_REGISTER_READ (FMQL_SCU_CTRL);
    scuVal |= FMQL_SCU_CTRL_EN;
    FMQL_REGISTER_WRITE (FMQL_SCU_CTRL, scuVal);

#else

    /* setup AUX register and enable L1 prefetch */

    mmuCortexA8AcrSet (AUX_CTL_REG_L1_PRE_EN);

#endif /* _WRS_CONFIG_SMP */

#ifdef INCLUDE_CACHE_SUPPORT

    /*
     * Install the appropriate cache library, no address translation
     * routines are required for this BSP, as the default memory map has
     * virtual and physical addresses the same.
     */

    cacheCortexA9LibInstall (mmuPhysToVirt, mmuVirtToPhys);

#ifdef INCLUDE_L2_CACHE
#endif /* INCLUDE_L2_CACHE */

#endif /* INCLUDE_CACHE_SUPPORT */

#endif

	/*
	excVecBaseSet (LOCAL_MEM_LOCAL_ADRS + 0x200);
	*/
	excVecBaseSet (LOCAL_MEM_LOCAL_ADRS); 

#ifdef INCLUDE_CACHE_SUPPORT
	/* sysCacheLibInit = cacheArchLibInit; */
	#if 0
		cacheCortexA9LibInstall (mmuPhysToVirt, mmuVirtToPhys);
	#else
		cacheCortexA15LibInstall (mmuPhysToVirt, mmuVirtToPhys);  /* jc */
	#endif
#endif /* INCLUDE_CACHE_SUPPORT */


#ifdef INCLUDE_MMU

#if 0
    /* setup AUX register and enable L1 prefetch */
    mmuCortexA8AcrSet (AUX_CTL_REG_L1_PRE_EN);
#else
	/* cortex-A7 SMP bit, it must enable before cache */
	mmuCortexA8AcrSet (AUX_CTL_REG_SMP); 
#endif

    /* Install the appropriate MMU library and translation routines */
    mmuCortexA8LibInstall (mmuPhysToVirt, mmuVirtToPhys);
#endif /* defined (INCLUDE_MMU) */

#if 0
	/**/
	/* disable irq */
	/**/
	/*void FGicPs_Disable(FGicPs *nstancePtr, u32 Int_Id)*/

	/**/
	/*  disable uart_0-rx-pin*/
	/**/
	/* MIO_PIN_45	0x7B4 	
	   ------------------
		L3_SEL	7:5 rw	0x0 Level 3 Mux Select
					000: GPIO 45 (bank 1), Input/Output
					001: CAN 1 Rx, Input
					010: I2C 1 Serial Data, Input/Output
					011: reserved
					100: SDIO 0 IO Bit 3, Input/Output
					101: SPI 0 MOSI, Input/Output
					110: reserved
					111: UART 1 RxD, Input
					
		IO_Type 11:9	rw	0x3 "Select the IO Buffer Type. 
					000: 鍏虫柇IO"
					001: LVCMOS18
					010: LVCMOS25
					011: LVCMOS33
					100: HSTL-I
					101: HSTL-II
					110: Reserved
					111: Reserved
	*/
	UINT32 tmp32 = 0;
	tmp32 = slcr_read(0x7B4);
	/*tmp32 = ((tmp32 & (~0xE0)) | 0xC0);  // L3_SEL: disalbe uart_1_rx (110: reserved)*/
	tmp32 = (tmp32 & (~0xE00));            /* IO_Type: disalbe uart_1_rx (000: 鍏虫柇IO")*/
	slcr_write(0x7B4, tmp32);	
	/* tmp32 = ((tmp32 & (~0xE0)) | 0xE0);     // enable uart_1_r x(111: UART 1 RxD, Input)*/
	/*tmp32 = (tmp32 & (~0xE00)  | 0x600 );    // IO_Type: enable uart_1_rx (011: LVCMOS33)*/
#endif

	uartBase = (UINT32 *) UART_0_BASE_ADR; /*0xE0004000;  // uart_0*/
	/*uartBase = (UINT32 * UART_1_BASE_ADR;  // 0xE0023000;  // uart_1  for test*/
	/*slcr_write(0x168, 0x12);  // cpu_hz *60 / 18 = 100M  Hz*/

    _func_kwrite  = fmqlDebugWrite;
    _func_kprintf = kprintf;
	
	printf("\nEarly uart init: %s (%s) \n\n", __DATE__, __TIME__);
	
	/* uboot:	mw.l  0xe0026334 0x000ff000 	*/	
	/* fmqlSlcrWrite (SLCR_TTC_CLK_CTRL, SLCR_TTC_CLK_CTRL_VAL);  */
	FMQL_SR_REGISTER_WRITE(0xe0026334, 0xff000);
	
	gtc_enable();

	/*clk_sdmmc_set(15); /* 100M Hz sdmmc ref_clk */

    return;
}

/*******************************************************************************
*
* sysHwInit - initialize the CPU board hardware
*
* This routine initializes various features of the hardware.
* Normally, it is called from usrInit () in usrConfig.c.
*
* NOTE: This routine should not be called directly by the user.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void sysHwInit (void)
    {
    /* install the IRQ/SVC interrupt stack splitting routine */

#ifndef _ARCH_SUPPORTS_PROTECT_INTERRUPT_STACK
    _func_armIntStackSplit = sysIntStackSplit;
#endif    /* !_ARCH_SUPPORTS_PROTECT_INTERRUPT_STACK */

    hardWareInterFaceInit ();

#if 1
    _vxb_usDelayRtn = &sysUsDelay;
    _vxb_msDelayRtn = &sysMsDelay;
    _vxb_delayRtn = &sysDelay;
#endif

/*
min-system or whole-functions
*/
#ifndef BUILD_VIP_MIN_SYSTEM

#ifdef INCLUDE_FM_QSPI
		qspiFlashInit1 ();
		/*vxInit_Qspi(0);*/
#endif

#ifdef DRV_FM_SPI
	/*sysSpiInit_1();*/
#endif

#if 0  /* def INCLUDE_ALT_SOC_GEN5_DW_END  // gmac_0, gmac_1: ps_mac ==<rgmii>==> phy_net*/
	/*
	GMAC_CRTL	0x414	reserved	
	----------------------------
	31:7	rw	0x0	Reserved. Writes are ignored, read data is zero.

	GMAC_PHY_INTF_SEL2	6:4	rw	0x0	"PHY Interface Select
			鈻�000: GMII or MII 
			鈻�001: RGMII 
			鈻�010: SGMII 
			鈻�011: TBI 
			鈻�100: RMII 
			鈻�101: RTBI 
			鈻�110: SMII 
			鈻�111: RevMII  	Active State: N/A"
			
	reserved	3	rw	0x0	Reserved. Writes are ignored, read data is zero.

	GMAC_PHY_INTF_SEL1	2:0	rw	0x0	"PHY Interface Select
			鈻�000: GMII or MII 
			鈻�001: RGMII 
			鈻�010: SGMII 
			鈻�011: TBI 
			鈻�100: RMII 
			鈻�101: RTBI 
			鈻�110: SMII 
			鈻�111: RevMII  	Active State: N/A"
	*/
	/**(UINT32*(0xE0026000 + 0x414)) = 0x11;  */
	slcr_write(0x414, 0x11);   /* for rgmii*/
	/*slcr_write(0x414, 0x00);  // for gmii*/

	extern void slcr_gem_reset();
	slcr_gem_reset();
#endif	

#endif  /* #ifndef BUILD_VIP_MIN_SYSTEM */


#ifdef  FORCE_DEFAULT_BOOT_LINE
    strncpy (sysBootLine, DEFAULT_BOOT_LINE, strlen (DEFAULT_BOOT_LINE)+1);
#endif /* FORCE_DEFAULT_BOOT_LINE */
    }

/*******************************************************************************
*
* sysHwInit2 - additional system configuration and initialization
*
* This routine connects system interrupts and does any additional
* configuration necessary. Note that this is called from
* sysClkConnect () in the timer driver.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void sysHwInit2 (void)
    {
    static BOOL initialized = FALSE;

    /* ifdef INCLUDE_RTP, add 2/3 of the EXTRA RAM to kernel, the rest to RTP */

#ifdef INCLUDE_RTP
    size_t memAddedToPool = ROUND_UP (FMQL_MEM_EXTRA_SIZE / 3 * 2, PAGE_SIZE);
#endif /* INCLUDE_RTP */

    if (initialized)
        {
        return;
        }

    vxbDevInit ();
    
#ifdef INCLUDE_FMSH_FMQL_GPIO
    sysGpioInit ();
#endif /* INCLUDE_FMSH_FMQL_GPIO */

#ifdef INCLUDE_SIO_UTILS
    sysSerialConnectAll ();
#endif

/*
min-system or whole-functions
*/
#ifndef BUILD_VIP_MIN_SYSTEM

#ifdef INCLUDE_FMSH_FMQL_GPIO
		sysGpioInit ();
#endif /* INCLUDE_FMSH_FMQL_GPIO */

#ifdef INCLUDE_FM_QSPI
	qspiFlashInit2 ();
	/* vxInit_Qspi(0);  */
#endif  /* INCLUDE_FM_QSPI */

#ifdef DRV_FM_SPI
	/* sysSpiInit_2(); */
#endif  

#ifdef INCLUDE_SYNOPSYSHCI
		/*sysUsbInit();*/
#endif  

#endif  /* #ifndef BUILD_VIP_MIN_SYSTEM */


    taskSpawn ("tDevConn", 11, 0, 10000, vxbDevConnect,
               0, 1, 2, 3, 4, 5, 6, 7, 8, 9);

    initialized = TRUE;

#ifdef INCLUDE_RTP
#   ifdef INCLUDE_MMU_GLOBAL_MAP
    (void) adrSpaceRAMAddToPool ((PHYS_ADDR) FMQL_MEM_EXTRA, FMQL_MEM_EXTRA_SIZE);
    (void) adrSpaceRAMReserve ((PHYS_ADDR) FMQL_MEM_EXTRA, memAddedToPool);
#   endif
    (void) memAddToPool (FMQL_MEM_EXTRA, memAddedToPool);
#else
#   ifdef INCLUDE_MMU_GLOBAL_MAP
    (void) adrSpaceRAMAddToPool ((PHYS_ADDR) FMQL_MEM_EXTRA, FMQL_MEM_EXTRA_SIZE);
    (void) adrSpaceRAMReserve ((PHYS_ADDR) FMQL_MEM_EXTRA, FMQL_MEM_EXTRA_SIZE);
#   endif
    (void) memAddToPool (FMQL_MEM_EXTRA, FMQL_MEM_EXTRA_SIZE);
#endif /* INCLUDE_RTP */

#ifdef INCLUDE_FM_QSPI
		/* qspiFlashInit2 (); */
		/* vxInit_Qspi(0); */
#endif  /* INCLUDE_FM_QSPI */

	/**/
	/* open&enable level-shift*/
	/**/
#ifdef PL_PCIE_RC
	slcr_write(0x838, 0xF);
	slcr_write(0x83C, 0xF);
#endif
}


void reboot2(void)
{
	/*
	PSS_RST_CTRL	0x200	
			reserved	31:1	rw	0x0	Reserved. 鍐欐棤鏁堬紝璇讳负0
			SOFT_RST	0	w	0x0	"PS绯荤粺杞浣嶃�
							0锛氫笉澶嶄綅
							1锛歅S杞浣嶏紙闄ゆ椂閽熶骇鐢熸ā鍧楀锛�
							娉ㄦ剰锛氭澶嶄綅涓嶉渶瑕佸啓0娓呴櫎銆傜‖浠舵槸鑷竻闆剁殑銆�
	*/
	/*澶嶄綅Qspi0 flash锛屽垏鍥炲埌3瀛楄妭妯″紡锛屼笉褰卞搷bootrom鍔犺浇fsbl鍚姩*/
	if (g_pQspi0->pQspiCtrl != NULL)
	{	
		qspiFlash_Reset_Flash(g_pQspi0);
	}
	
	printf("\nreboot system ... \n\n");
	taskDelay(1);
	
	slcr_write(0x200, 0x01);
	
	while (1); /* wait for restart*/
	
	return;
}



/* reboot from fsbl*/
void reboot3(void)
{
	/*__asm__ ("mov lr, r0\n\t");*/
	/*__asm__ ("push {lr}\n\t");*/
	
#if 0
	__asm__  ("mov r0,#0x20000\n\t"
               "mov pc,r0\n\t");
#endif
	
#if 0
	int(*pBooot)(void);
	pBooot = (UINT32*)0x20000; 	
	(*pBooot)();
#endif

	/*__asm__  (  "mov r0,#0xE1000000\n\t"
				"add r1,r0,#0xFF0000\n\t"
				"add r0,r1,#0xF100\n\t"
				"mov pc,r0\n\t"
				"NOP" );
	*/
	
	/*__asm__ ("pop {pc}\n\t");*/
	/*__asm__ ("NOP\n\t");*/
	/*td("tShell0");*/
	/*shellRestart(0);	*/
	
	__asm__  ("mov r0, #0x20000\n\t");

	__asm__  ("mov lr, r0\n\t"); /* move the destination address into link register */
	/*__asm__  ("push {lr}\n\t");*/
	
		/*disable Icache*/	  
	__asm__  ("mcr p15, 0, r0, c1, c0, 0\n\t");
	__asm__  ("bic r0, r0, #0x100\n\t");
	__asm__  ("mrc p15, 0, r0, c1, c0, 0\n\t");
	
		/*disable Dcache*/		
	__asm__  ("mcr p15, 0, r0, c1, c0, 0\n\t");
	__asm__  ("bic r0, r0, #4\n\t");
	__asm__  ("mrc p15, 0, r0, c1, c0, 0\n\t");    
		  
	__asm__  ("dsb\n\t");
	__asm__  ("isb\n\t"); 			  /* make sure it completes */
	
	__asm__  ("ldr r4, =0\n\t");
	__asm__  ("mcr  p15,0,r4,c1,c0,0\n\t");
	
	__asm__  ("sev\n\t");
	/*__asm__  ("pop {pc}\n\t");*/
	
	/*exit(0);*/
}


/* reboot from uboot*/
void reboot4(void)
{
	int(*pBooot)(void);
	pBooot = (UINT32*)0x04000000; 	
	(*pBooot)();
	
	exit(0);
}

/*******************************************************************************
*
* sysToMonitor - transfer control to the ROM monitor
*
* This routine transfers control to the ROM monitor. It is usually called
* only by reboot () -- which services ^X -- and bus errors at interrupt
* level.  However, in some circumstances, the user may wish to introduce a
* new <startType> to enable special boot ROM facilities.
*
* RETURNS: Does not return.
*
* ERRNO: N/A
*/

STATUS sysToMonitor
    (
    int startType    /* passed to ROM to tell it how to boot */
    )
{
	printf("%s\n",(char*)EXC_MSG_ADRS);
    printf("calling sysToMonitor\n");
	
	reboot2();  /* support ctrl+x function*/
	
#if 0
    FUNCPTR  pRom;
    int      i;
    int      intID;
    int      j;
    volatile UINT32 intrAck;
    UINT32   intActiveBits;

#ifdef _WRS_CONFIG_SMP
#if 0
    cpuset_t cpuList;
    volatile int idx;
    int srcCpuId;

    /* firstAPCore makes sure that IPI is sent once */

    static int firstAPCore = TRUE;

    /* if the current core is not core0, we won't run sysToMonitor directly */

    if (vxCpuIndexGet () != 0)
        {

        /*
         * APPCORE_REBOOT means that all other cores are indicated to shutdown
         * by core0
         */

        if ((startType & 0xFFFFFFF0) == APPCORE_REBOOT)
            {

            /* disable Core ints */

            FMQL_REGISTER_WRITE (FMQL_GIC_CPU_CONTROL, 0x0);

            /*
             * core0 sent the IPI to Application Processor (AP) to reboot.
             * end the interrupt, because ISR in interrupt controller driver
             * has read (and cleared) this register.
             */

            FMQL_REGISTER_WRITE (FMQL_GIC_CPU_END_INTR,
                                                          INT_LVL_MPCORE_IPI08);

            /* empty interrupt ack fifo */

            intrAck = FMQL_REGISTER_READ (FMQL_GIC_CPU_ACK) &
                                                               GIC_INT_SPURIOUS;

            while ((intrAck != GIC_INT_SPURIOUS))
               {
               FMQL_REGISTER_WRITE (FMQL_GIC_CPU_END_INTR, intrAck);

               intrAck = FMQL_REGISTER_READ (FMQL_GIC_CPU_ACK) &
                                                               GIC_INT_SPURIOUS;
               }

            /* flush data cache */

            CACHE_PIPE_FLUSH ();

            /* disable the MMU, cache (s) and write-buffer */

            mmuCortexA8ADisable ();

            /* Make sure we go back to the bootMonitor loop */

            pRom = (FUNCPTR)(ROM_WARM_ADRS);

            /* jump to boot ROM */

            (* pRom)(0);
            }
        else
            {

            /*
             * If code reached here, it means that reboot is requested and
             * code is executing on one of AP cores.
             * Notify core0 to run sysToMonitor and wait until
             * core0 sends an IPI to APs to shut down.
             * firstAPCore value check makes sure that only one AP core
             * sends notification to core0.
             * Spin lock makes sure that two APs don't modify firstAPCore
             * simultaneously
             */

            SPIN_LOCK_ISR_TAKE (&rebootLock);

            if (firstAPCore)
                {
                rebootVar = (BOOTCORE_REBOOT | startType |
                             (vxCpuIndexGet () << 10));

                vxIpiEmit (INT_LVL_MPCORE_RESET, 1);

                firstAPCore = FALSE;
                }

            SPIN_LOCK_ISR_GIVE (&rebootLock);

            /* wait for a reset from core0 */

            for (;;);
            }
        }
	#endif

#endif /* _WRS_CONFIG_SMP */

    sysClkDisable ();

    intIFLock ();

#ifdef _WRS_CONFIG_SMP

    /* here is the core 0 */
#if 1
    if ((rebootVar & 0xFFF00000) == BOOTCORE_REBOOT)
        {

        /*
         * core0 is informed by other core through IPI to reboot, so we must
         * acknowledge the IPI (SGI) interrupt.
         */

        srcCpuId = (rebootVar & 0x00001C00);

        FMQL_REGISTER_WRITE (FMQL_GIC_CPU_END_INTR,
                                    INT_LVL_MPCORE_RESET | srcCpuId);

        startType = rebootVar & 0x0000000F;
        }

    /* clear entry address flags */
   /*  jc
   FMQL_REGISTER_WRITE (FMQL_OCM_BOOT_ADR, 0xffffffff);
	*/

    /* reset all other cores */
    if ((cpuList = vxCpuEnabledGet ()) && (cpuList > 1))
        {
        for (idx = 1; idx < VX_SMP_NUM_CPUS; idx++)
            {
            if (cpuList & (1 << idx))
                {
                vxIpiEmit (INT_LVL_MPCORE_IPI08, 1 << idx);
                }
            }
        }

    /* delay for non-boot core to shutdown */

    for (idx = 0; idx < 0x1ffff; idx++);
#endif

#endif /* _WRS_CONFIG_SMP */

    /* disable GIC cpu interface */
    FMQL_REGISTER_WRITE (FMQL_GIC_CPU_CONTROL, 0x0);

    /* disable GIC distributor */
    FMQL_REGISTER_WRITE (FMQL_GIC_BASE + FMQL_GIC_DIST_CONTROL, 0x0);

    /* empty interrupt ack fifo */
    intrAck = FMQL_REGISTER_READ (FMQL_GIC_CPU_ACK) & \
                                         GIC_INT_SPURIOUS;

    while ((intrAck != GIC_INT_SPURIOUS) && (intrAck >= SGI_INT_MAX) )
    {
        FMQL_REGISTER_WRITE (FMQL_GIC_CPU_END_INTR, intrAck);

        intrAck = FMQL_REGISTER_READ (FMQL_GIC_CPU_ACK) & GIC_INT_SPURIOUS;
    }

    /* disable all SPI interrupts */

    for (i = SPI_START_INT_NUM; i < SYS_INT_LEVELS_MAX; i += BITS_PER_WORD)
        {
        FMQL_REGISTER_WRITE (FMQL_GIC_BASE + \
            FMQL_GIC_DIST_ENABLE_CLR1 + (0x4 * NWORD (i)), 0xffffffff);
        }

#ifdef INCLUDE_MMU
    VM_ENABLE (FALSE);     /* Disable the MMU, cache (s) and write-buffer */

#ifdef _WRS_CONFIG_SMP
        mmuCortexA8AcrSet (0);
#endif /* _WRS_CONFIG_SMP */

#endif /* INCLUDE_MMU */

#ifdef INCLUDE_VFP
    vfpDisable ();
#endif /* INCLUDE_VFP */

    pRom = (FUNCPTR)(ROM_WARM_ADRS);

    /* jump to boot ROM */
    (* pRom)(startType);
	
#endif

    return OK;
}


#if defined (_WRS_CONFIG_SMP)

/*******************************************************************************
*
* sysCpuStart - vxWorks startup
*
* This routine establishes a CPUs vxWorks envirnonment.  Note that the MMU
* and caches have been enabled before this routine is called.
*
* This is NOT callable from C.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL void sysCpuStart
    (
    void (*startFunc) (void),
    UINT32 cpuNum
    )
{
    volatile UINT32 intrAck;

    /* flush all pending local interrupts */
    intrAck = FMQL_REGISTER_READ (FMQL_GIC_CPU_ACK) & GIC_INT_SPURIOUS;

    while (intrAck != GIC_INT_SPURIOUS)
    {
        FMQL_REGISTER_WRITE (FMQL_GIC_CPU_END_INTR, intrAck);
        intrAck = FMQL_REGISTER_READ (FMQL_GIC_CPU_ACK) & GIC_INT_SPURIOUS;
    }

    /* jc
    FMQL_REGISTER_WRITE (FMQL_GIC1_BASE + FMQL_GIC_DIST_PEND_CLR1, 0xffff);
	*/
    FMQL_REGISTER_WRITE (FMQL_GIC_BASE + FMQL_GIC_DIST_PEND_CLR1, 0xffff);

#ifdef INCLUDE_VFP
    vfpEnable ();
#endif

    /* Initialise ARM exception mode registers */
    armInitExceptionModes ();

    /* Enable Local S/W interrupts */
    sysArmGicDevInit ();

    /* Tell the boot CPU we are here */

    sysMPCoreStartup[cpuNum].newSync = 0;

    intIFUnlock (0);

    /* Enter the Kernel */
    (*startFunc)();

}

/*******************************************************************************
*
* sysCpuEnable - enable a multi core CPU
*
* This routine brings a CPU out of reset
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/
#define CORE0_ENTRY_OFFSET  (0xe0026438)   /* cpu_core_0 */
#define CORE1_ENTRY_OFFSET  (0xe0026440)   /* cpu_core_1 */
#define CORE2_ENTRY_OFFSET  (0xe0026448)   /* cpu_core_2 */
#define CORE3_ENTRY_OFFSET  (0xe0026450)   /* cpu_core_3 */

STATUS sysCpuEnable
    (
    unsigned int cpuNum,
    void (*startFunc) (void),
    char *stackPtr
    )
    {
    /*
     * sysCpuEnable is called sequentially for each Application Processor.
     * The IPIs have to be enabled once.
     * sysCpuEnableFirst variable is used to make sure that IPIs
     * are not enabled multiple times.
     */

    static INT32 sysCpuEnableFirst = 1;

    /* Validate cpuNum */

    if (cpuNum < 1 || cpuNum >= VX_SMP_NUM_CPUS)
        return (ERROR);
#if 0
    if (sysCpuEnableFirst == 1)
        {

        /*
         * IPIs cannot be connected in sysToMonitor, because interrupt handlers
         * cannot be installed in interrupt context.
         * sysToMonitor can be called from interrupt context as well
         * (WIND00321261).
         */

        /*
         * Connect and enable IPI for Application Processor cores.
         * The INT_LVL_MPCORE_IPI08 is sent by core 0 to all the AP cores.
         */
        vxIpiConnect (INT_LVL_MPCORE_IPI08, (IPI_HANDLER_FUNC)(sysToMonitor),
                     (void *)(APPCORE_REBOOT));
        vxIpiEnable (INT_LVL_MPCORE_IPI08);

        /*
         * Connect and enable IPI for core0.
         * The INT_LVL_MPCORE_RESET is sent by an Application processor core
         * to core0.
         */
        vxIpiConnect (INT_LVL_MPCORE_RESET, (IPI_HANDLER_FUNC)(sysToMonitor),
                     (void *) &rebootVar);
        vxIpiEnable (INT_LVL_MPCORE_RESET);

        sysCpuEnableFirst = 0;
        }
#endif

    /* Setup init values */
    sysMPCoreStartup[cpuNum].newPC   = (UINT32)sysCpuStart;
    sysMPCoreStartup[cpuNum].newSP   = (UINT32)stackPtr;
    sysMPCoreStartup[cpuNum].newArg  = (UINT32)startFunc;
    sysMPCoreStartup[cpuNum].newSync = (UINT32)mmuCortexA8TtbrGetAll ();

    /* Make sure data hits memory */
    cacheFlush ((CACHE_TYPE)DATA_CACHE, (void *)sysMPCoreStartup,
               (size_t)(sizeof (sysMPCoreStartup)));

    cachePipeFlush ();

    /* Setup CPU for init */
#if 0
    FMQL_REGISTER_WRITE (FMQL_OCM_BOOT_ADR, (UINT32)sysCpuInit);
#else
	FMQL_SR_REGISTER_WRITE ((UINT32)CORE0_ENTRY_OFFSET + (cpuNum * 8), (UINT32)sysCpuInit);
#endif

    VX_SYNC_BARRIER ();

    /* wake up core1 from wfe using sev. */
    WRS_ASM ("sev");

    return OK;
    }

#endif /* if defined (_WRS_CONFIG_SMP) */

/*******************************************************************************
*
* sysCpuAvailableGet - return the number of CPUs available
*
* This routine gets the number of CPUs available.
*
* RETURNS:  number of CPU cores available
*
* ERRNO: N/A
*/

UINT32 sysCpuAvailableGet (void)
    {
#if 1
	return 4;
#else
    return (XLNX_FMQL_REGISTER_READ (FMQL_SCU_CONFIG) & 0x3) + 1;
#endif
    }


/*******************************************************************************
*
* sysUsDelay - delay at least the specified amount of time (in microseconds)
*
* This routine delays for approximately one us. When system timer
* count register add 1 tick,
* 1 tick = 1/system timer clk = 1/FMQL_TIMERS_CLK = 1/333333333(s)
* timer grows up.
*
* NOTE:  This routine will not relinquish the CPU; it is meant to perform a
* busy loop delay.  The minimum delay that this routine will provide is
* approximately 10 microseconds.  The maximum delay is approximately the
* size of UINT32; however, there is no roll-over compensation for the total
* delay time, so it is necessary to back off two times the system tick rate
* from the maximum.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void sysUsDelay
    (
    int        delay          /* length of time in US to delay */
    )
{
#if 0  /* l_l */	

	volatile int i,j;
	for (i=0; i<(100*delay); i++)
		for (j=0; j<800; j++);
		
#endif
    
	UINT64 counts = 0;
	UINT64 tCur = 0, tEnd = 0;
	
	counts = (((UINT64)((UINT64)delay * HARD_OSC_HZ)) + 1000000) / (2*1000000);  /* 1 us == 1 count */
	
	/* gtc_enable(); */
	tCur = get_gtc_time();	
	while (1)
	{
		tEnd = get_gtc_time();
		if((tEnd - tCur) >= counts)
			break;
	}

	return;
}

void test_usdelay(int delay)
{
	UINT64 tmp64_0 = 0;
	UINT64 tmp64 = 0;
	
	tmp64_0 = get_gtc_time();
	sysUsDelay(delay);
	tmp64 = get_gtc_time();
	
	printf("timer(%d us): %lld (%lld) \n", delay, tmp64, (tmp64-tmp64_0));
	
	tmp64_0 = get_gtc_time();
	taskDelay(delay);
	tmp64 = get_gtc_time();
	
	printf("timer(%d ticks): %lld (%lld) \n", delay, tmp64, (tmp64-tmp64_0));
	
	return;
}

/*******************************************************************************
*
* sysMsDelay - 1ms delay increments
*
* This routine consumes 1ms of time * delay.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void sysMsDelay
    (
    int      delay                   /* length of time in MS to delay */
    )
    {
    sysUsDelay ( (int) delay * 1000 );
    }

/*******************************************************************************
*
* sysDelay - delay for approximately one millisecond
*
* This routine delays for approximately one milli-second.
*
* RETURNS: N/A
*/

void sysDelay (void)
    {
    sysMsDelay (1);
    }


void app_main(void)
{
	/* eth_clk_set_1G();*/
	char ip_str[128] = {0};
	
#ifndef BUILD_VIP_MIN_SYSTEM
	printf("\n==== Image6911 build: %s-%s (Whole-Functions) ==== \n\n\n", __DATE__, __TIME__);
#else	
	printf("\n==== Image6911 build: %s-%s (Min-System) ==== \n\n\n", __DATE__, __TIME__);
#endif

	if (get_HARD_OSC_HZ() == (APU_30M_HZ))
	{
		printf("== HARD_OSC_HZ: %d Hz !!! Verify_Board of FMSH !!! == \n", get_HARD_OSC_HZ());
	}
	else 
	{
		printf("== HARD_OSC_HZ: %d Hz !!! Demo_Board or Usr_Define !!! == \n", get_HARD_OSC_HZ());
	}
	
	printf("== CPU: %d Hz,DDR: %d bps == \n\n", clk_cpu_get(), ddr_dRate_get());



#ifndef BUILD_VIP_MIN_SYSTEM

#if 0  /* fixd ip*/
#ifdef INCLUDE_GMAC_0
	clk_gem0_show();

	/* for test*/
	ifconfig("gem0 lladdr 00:01:02:03:04:05");
	ifconfig("gem0 192.168.40.77");
	ifconfig("gem0 up");
#endif

#ifdef INCLUDE_GMAC_1
	printf("\n");
	clk_gem1_show();

	/*
	ethernet attach: gmac_1
	*/
	ipAttach(1, "gem");
	ifconfig("gem1 lladdr 00:0a:35:11:22:44");
	ifconfig("gem1 192.168.46.77");
	ifconfig("gem1 up");
#endif
#endif

#ifdef	GMAC_1_BARE_METAL
	/* do nothing*/
#else

#ifdef INCLUDE_ALT_SOC_GEN5_DW_END  /* uboot env ip in qspi_flash, at offset: 0xE0_0000*/
#ifdef INCLUDE_GMAC_0  /* for test uboot boot parmas*/
		clk_gem0_show();
			
		/* for test*/
		ifconfig("gem0 lladdr 00:01:02:03:04:05");
		
	#if 0 /*def QSPI_2_VX  // qspi_0 norflash must be exist		*/
		/*ifconfig("gem0 192.168.40.77");*/
		extern char g_uboot_ipAddr[];		
		extern int qspi_get_ipaddr(void);
		qspi_get_ipaddr();
		
		sprintf((char*)(&ip_str[0]), "gem0 %s", (char*)(&g_uboot_ipAddr[0]));
		ifconfig((char*)(&ip_str[0]));
	#endif
	
		ifconfig("gem0 192.168.46.77");		
		ifconfig("gem0 up");
#endif
	
#ifdef INCLUDE_GMAC_1
		printf("\n");
		clk_gem1_show();
	
		/*
		ethernet attach: gmac_1
		*/
		ipAttach(1, "gem");
		ifconfig("gem1 lladdr 00:0a:35:11:22:44");		
		ifconfig("gem1 192.168.46.77");		
		ifconfig("gem1 up");
#endif

	/**/
	/* show ethernet_drv*/
	/**/
#ifdef INCLUDE_GMAC_0
		ifconfig();
#endif

#endif  /* for test eth*/
#endif

#ifdef DRV_FM_SDMMC  /* register the fmshSdhcCtrl driver	*/
	/*
	set sdmmc clk to 100M
	*/
	#if 0  /* for test of 10S*/
	clk_sdmmc_set(10);

	slcr_write(0x290, 0xFF);
	taskDelay(1);
	slcr_write(0x290, 0x00);
	
	taskDelay(10);
	#endif

	/* for demo_borad sd/emmc, test dosfs	*/
	#ifdef SDMMC_DOSFS_V2
		extern void fmshSdhcRegister (void);		
		fmshSdhcRegister();
	#endif	

	/* for test fatFS*/
	#ifdef SDMMC_VXB 
		extern void vxbSdmmcRegister2 (void);		
		vxbSdmmcRegister2();	
	#endif	
#endif  /* #ifdef DRV_FM_SDMMC */

#endif  /* #ifndef BUILD_VIP_MIN_SYSTEM */

	/* test for 10s*/
	/*AutoBootApp1();*/

#if 0  /* tftp server start */
	taskDelay(10);
	iptftps_start();
#endif

	return;
}

/******************************************************************************/
/******************************   app_main end   ******************************/
/******************************************************************************/



#if 1  /* usr-define vx-shell tools: md or cp2 ...*/

void md(UINT32 addr, UINT32 len)
{
	int i = 0, end = 0;
	UINT32 tmp32 = 0;
	UINT32 base = 0;
	int start = 0;

	if (len == 0)
	{
		len = 256;
	}

	/**/
	/* flush cache, it's must be for ddr flush new data*/
	/**/
	(void)cacheInvalidate (DATA_CACHE, addr, len); 
	

	switch (addr & 0x0F) 
	{
	case 0x04:
		start = 1;
		break;
	case 0x08:
		start = 2;
		break;
	case 0x0C:
		start = 3;
		break;
	}
	base = addr & 0xFFFFFFF0;
	
	for (i=0; i<(len/4+start); i++)
	{
		tmp32 = *(UINT32 *)(base+i*4);
		
		if ((i==0) || (i%4 == 0))
		{
			if (i >= start)
			{
				printf("0x%08X:  %08X ", (base+i*4), tmp32);
			}
			else
			{
				printf("0x%08X:           ", base);
			}
		}
		else
		{
			if (i >= start)
			{
				printf("%08X ", tmp32);
			}
			else
			{
				printf("         ");
			}
		}

		if ((i+1)%4 == 0)
		{
			printf("\n");
		}
	}
	printf("\n");

	return;
}


/*
the cp2 is same of cp(vx cmd), but printf the log: "." of processing
*/

/*
const char * in,	// name of file to read  (if NULL assume stdin)
const char * out 	// name of file to write (if NULL assume stdout)
*/
#define	COPY_BLOCK_SIZE	 (1024)

void cp2(const char * in_file, const char * out_file)
{
    int  inFd = ERROR;
    int	 outFd = ERROR;	
	
    int	 status;	
    char *	buffer;
    int	 totalBytes = 0;
    int	 nbytes, file_size;
	int percent = 0, prnt_flag[11] = {0};
	
    size_t	dSize = COPY_BLOCK_SIZE;
	int cnt = 0;

	
	printf("\n\ncopying file %s -> %s\n", in_file, out_file );

	/* 
	delete the old file of same name 
	*/
    outFd = open (out_file, O_RDONLY, 0);
    if (outFd != ERROR)
	{
		remove(out_file); 
		close(outFd);
		printf("remove file %s ok! \n", out_file );
	}

    /* 
    open input file 
	*/
    inFd = open (in_file, O_RDONLY, 0);
    if (inFd == ERROR)
	{
		printErr ("Can't open input file \"%s\" errno = %p\n",  in_file, (void *) errno );
		return (ERROR);
	}
	
    /* 
    create output file 
	*/
    outFd = open (out_file, (O_WRONLY | O_CREAT | O_TRUNC | O_SYNC), 0777);
    if (outFd == ERROR)
	{
		printErr ("Can't write to \"%s\", errno %p\n", out_file, (void *) errno);

		if (in_file != NULL)
		{    
			close (inFd);
		}
		return (ERROR);
	}
	
    /* 
    copy data 
    */
    /*status = copyStreams (inFd, outFd);*/
	/*if (ioctl (inFd, FIONREAD, (int) &dSize) == ERROR)*/
	if (ioctl (inFd, FIONREAD, (int) &file_size) == ERROR)
    {	
    	dSize = COPY_BLOCK_SIZE;
	}
    else
	{	
		/*dSize = min(COPY_BLOCK_SIZE, dSize);*/
		dSize = min(COPY_BLOCK_SIZE, file_size);
    }

    buffer = malloc(dSize);
    if (buffer == NULL)
    {	
		printErr ("malloc size(%d) fail!!! \n", dSize);
		
		close (outFd);
		close (inFd);
		
    	return (ERROR);
    }
	
    /* 
    transferring buffer 
	*/
    while ((nbytes = fioRead (inFd, buffer, dSize)) > 0)
	{
		if (write (outFd, buffer, nbytes) != nbytes)
	    {
		    printErr ("copy: error writing file. errno %p\n", (void *) errno);

			free (buffer);
			
			close (outFd);
			close (inFd);
			
		    return (ERROR);
	    }
		totalBytes += nbytes;
		
		/*=========================*/
		cnt++;
		if ((cnt % 8) == 0) /* 8K */
		{
			printf(".");   /* print log of cp processing*/
			
			percent = (totalBytes*100) / file_size;
			if (((percent % 10) == 0) && ((percent / 10) != 0))
			{
				if (prnt_flag[percent/10] == 0)
				{
					printf("%d%%", percent);
					prnt_flag[percent/10] = 1;
				}
			}
		}
		/*==========================*/
    }

    free (buffer);
    if (nbytes < 0)
	{
		printErr ("copy: error reading file after copying %d bytes. \n",  totalBytes);
		return (ERROR);
	}

    printf("Copy: wating... ..., program data to target ... ... \n", totalBytes );
	
	close (outFd);	
	close (inFd);
	
    printf("Copy OK: %u bytes copied \n", totalBytes );
	
    return (OK);
}

void fmcp(const char * in_file, const char * out_file)
{
	cp2(in_file, out_file);
	return;
}

#endif



#if 0 /* test vx booting timeout, for 10s*/

#if 1  /* auto_boot timeout method-1*/
STATUS AutoBootApp1(void)
{
	/*
	into usr-define functions mode by key-press
	*/
#ifdef VX_BOOT_ACK_2KEY
	
#define DELAY_X_S (2)   /* delay 1s	*/
#define KEY_ESC   (0x1B) 	/* 27	0x1B	ESC*/
#define KEY_PRE_1 (0x60)   	/* 96	0x60	`*/
#define KEY_1     (0x31)   	/* 49	0x31	1*/
#define KEY_2     (0x32)   	/* 50	0x32	2*/
#define KEY_3     (0x33)   	/* 51	0x33	3*/
	
		extern char g_1key_pressed;
		extern char g_2key_pressed;
	
		char inChar1, inChar2;
		int timeout = 0, delay_cnt = 0;
		int key_mode = 0;
		
		delay_cnt = sysClkRateGet() * DELAY_X_S;
		
		key_mode = 0;	
		do
		{	
			inChar1 = g_1key_pressed;
			inChar2 = g_2key_pressed;
	
			/*if (g_1key_pressed == KEY_ESC)*/
			{
				switch (inChar2)
				{
				case KEY_PRE_1:
					key_mode = KEY_PRE_1;
					break;
				case KEY_1:
					key_mode = KEY_1;
					break;
				case KEY_2:
					key_mode = KEY_2;
					break;
				case KEY_3:
					key_mode = KEY_3;
					break;
				}
			}
				
			/**/
			/* delay 1s*/
			/**/
			timeout++;
			if (timeout >= delay_cnt)
			{
				key_mode = 0xFF;
				break;
			}
			else
			{
				taskDelay(1);
			}				
		}while (key_mode == 0);
	
		printf("inChar1:0x%X, inChar2:0x%X \n", inChar1, inChar2);			
		
		switch (key_mode)
		{
		case KEY_PRE_1:
			printf("Into Key_Mode-PRE_1: %d \n", key_mode);
			/* call usr_define function_1*/
			break;
		
		case KEY_1:
			printf("Into Key_Mode-1: %d \n", key_mode);
			/* call usr_define function_2*/
			break;
		
		case KEY_2:
			printf("Into Key_Mode-2: %d \n", key_mode);
			/* call usr_define function_3*/
			break;
		
		case KEY_3:
			printf("Into Key_Mode-3: %d \n", key_mode);
			/* call usr_define function_3*/
			break;
		
		default:
			printf("Into Key_Mode: %d normal booting(%d)... \n", key_mode, delay_cnt);
			/* call usr_define function_3*/
			break;
		}
		
		g_1key_pressed = 0;
		g_2key_pressed = 0;
#endif
	
#if 0
#define DELAY_X_S (1)   /* delay 1s*/
	
#define KEY_ESC   (0x1B) 	/* 27	0x1B	ESC*/
#define KEY_PRE_1 (0x60)   	/* 96	0x60	`*/
#define KEY_1     (0x31)   	/* 49	0x31	1*/
#define KEY_2     (0x32)   	/* 50	0x32	2*/
	
		char inChar1, inChar2;
		char inChar3;
		int timeout = 0, delay_cnt = 0;
		int key_mode = 0;
		
		delay_cnt = sysClkRateGet() * DELAY_X_S;
		
		/*inChar1 = getchar();*/
		/*inChar2 = getchar();	*/
		inChar1 = getc();
		inChar2 = getc();	
	
		key_mode = 0;
		do
		{			
			if ((inChar1 == KEY_ESC) || (inChar2 == KEY_ESC))
			{		
				/*inChar3 = getchar();*/
				inChar3 = getc();
				printf("inChar3: 0x%X \n", inChar3);
			}
				
			if (inChar3 == KEY_PRE_1)
			{
				key_mode = 1;
				break;
			}
			else if (inChar3 == KEY_1)
			{
				key_mode = 2;
				break;
			}
			else if (inChar3 == KEY_2)
			{
				key_mode = 3;
				break;	 /* while (key_mode == 0);*/
			}
				
			/**/
			/* delay 1s*/
			/**/
			timeout++;
			if (timeout >= 10)
			{
				key_mode = 0xF;
				printf("inChar1:0x%X, inChar2:0x%X \n", inChar1, inChar2);						
				break;
			}
			else
			{
				/*vxbMsDelay(1);*/
			}				
		}while (key_mode == 0);
			
		printf("inChar1:0x%X, inChar2:0x%X \n", inChar1, inChar2);						
	
		switch (key_mode)
		{
		case 1:
			printf("Into Key_Mode: %d \n", key_mode);
			/* call usr_define function_1*/
			break;
		
		case 2:
			printf("Into Key_Mode: %d \n", key_mode);
			/* call usr_define function_2*/
			break;
		
		case 3:
			printf("Into Key_Mode: %d \n", key_mode);
			/* call usr_define function_3*/
			break;
		
		default:
			printf("Into Key_Mode: %d normal booting(%d)... \n", key_mode, delay_cnt);
			/* call usr_define function_3*/
			break;
		}
#endif	

	return;
}
#endif


#if 2  /*// auto_boot timeout method-2*/

#include <iolib.h>
extern int      ioctl (int fd, int function, ...);
extern  STATUS usrSerialInit();

static STATUS AutoBootApp(void)
{
	UINT32 timeout;
	UINT32 autoBootTime;
	UINT32 timeMarker;
	UINT32 timeLeft = 5;
	UINT32 nBytesUnread=0;
	int consoleFd = 0;
	
	timeout = 5;
	
	if (timeout > 0)
	{
		printf("\rPress 'Enter' into debug mode...\n");
		
		autoBootTime = tickGet() + sysClkRateGet()*timeout;
		timeMarker  = tickGet() + sysClkRateGet();
		
		timeLeft = timeout;
		printf("%2d\r", timeLeft);
		
		while ((tickGet() < autoBootTime) && (nBytesUnread == 0))
		{
			if (tickGet() == timeMarker)
			{
				timeMarker  = tickGet() + sysClkRateGet();
				printf("%2d                         \r", --timeout);
			}
			
			ioctl(STD_IN,FIONREAD,(int)&nBytesUnread);
		}
		 
		if (nBytesUnread != 0)
		{
			printf("Get Into Debug Mode...\n");
		}
		else
		{
			printf("Get Into Normal Mode...\n");
			printf("Entering User App...\n");
		}
	}
	
	return OK;
}

void fmshUsrAppInit (void)
{
	ts("tShell0");
	
	AutoBootApp();
	
	tr("tShell0");
	return;
}

#endif
#endif

#if 0
typedef struct _test_struct_
{
	int a;
	int b;
	
	UINT32 c;
	UINT32 d;

	UINT8  e;
	UINT8  f;
} T_STRUCT_TEST;

T_STRUCT_TEST t6 = {0};

void test_struct_init(void)
{
	T_STRUCT_TEST t4 = {0};
	
	T_STRUCT_TEST t1;	
	T_STRUCT_TEST t12;	
	T_STRUCT_TEST t13;	
	
	T_STRUCT_TEST t2 = {0};
	
	T_STRUCT_TEST t3 = {0xFF};
	T_STRUCT_TEST t5 = {0};

	printf("T_STRUCT_TEST t1 : \n");
	printf("t1.a=0x%X \n", t1.a);	
	printf("t1.b=0x%X \n", t1.b);
	printf("t1.c=0x%X \n", t1.c);	
	printf("t1.d=0x%X \n", t1.d);
	printf("t1.e=0x%X \n", t1.e);	
	printf("t1.f=0x%X \n", t1.f);
	printf("\n");
	
	printf("T_STRUCT_TEST t12 : \n");
	printf("t12.a=0x%X \n", t12.a);	
	printf("t12.b=0x%X \n", t12.b);
	printf("t12.c=0x%X \n", t12.c);	
	printf("t12.d=0x%X \n", t12.d);
	printf("t12.e=0x%X \n", t12.e);	
	printf("t12.f=0x%X \n", t12.f);
	printf("\n");
	
	printf("T_STRUCT_TEST t13 : \n");
	printf("t13.a=0x%X \n", t13.a);	
	printf("t13.b=0x%X \n", t13.b);
	printf("t13.c=0x%X \n", t13.c);	
	printf("t13.d=0x%X \n", t13.d);
	printf("t13.e=0x%X \n", t13.e);	
	printf("t13.f=0x%X \n", t13.f);
	printf("\n");
	
	printf("T_STRUCT_TEST t2 = {0} : \n");
	printf("t2.a=0x%X \n", t2.a);	
	printf("t2.b=0x%X \n", t2.b);
	printf("t2.c=0x%X \n", t2.c);	
	printf("t2.d=0x%X \n", t2.d);
	printf("t2.e=0x%X \n", t2.e);	
	printf("t2.f=0x%X \n", t2.f);
	printf("\n");
	
	printf("T_STRUCT_TEST t3 = {0xFF} : \n");
	printf("t3.a=0x%X \n", t3.a);	
	printf("t3.b=0x%X \n", t3.b);
	printf("t3.c=0x%X \n", t3.c);	
	printf("t3.d=0x%X \n", t3.d);
	printf("t3.e=0x%X \n", t3.e);	
	printf("t3.f=0x%X \n", t3.f);
	printf("\n");
	
	printf("T_STRUCT_TEST t4 = {0} : \n");
	printf("t4.a=0x%X \n", t4.a);	
	printf("t4.b=0x%X \n", t4.b);
	printf("t4.c=0x%X \n", t4.c);	
	printf("t4.d=0x%X \n", t4.d);
	printf("t4.e=0x%X \n", t4.e);	
	printf("t4.f=0x%X \n", t4.f);
	printf("\n");
	
	printf("T_STRUCT_TEST t5 = {0} : \n");
	printf("t5.a=0x%X \n", t5.a);	
	printf("t5.b=0x%X \n", t5.b);
	printf("t5.c=0x%X \n", t5.c);	
	printf("t5.d=0x%X \n", t5.d);
	printf("t5.e=0x%X \n", t5.e);	
	printf("t5.f=0x%X \n", t5.f);
	printf("\n");
	
	printf("T_STRUCT_TEST t6 = {0} : \n");
	printf("t6.a=0x%X \n", t6.a);	
	printf("t6.b=0x%X \n", t6.b);
	printf("t6.c=0x%X \n", t6.c);	
	printf("t6.d=0x%X \n", t6.d);
	printf("t6.e=0x%X \n", t6.e);	
	printf("t6.f=0x%X \n", t6.f);
	printf("\n");

	return;
}
/*
-> test_struct_init
T_STRUCT_TEST t1 : 
t1.a=0x241C 
t1.b=0x0 
t1.c=0x243C 
t1.d=0x0 
t1.e=0xE8 
t1.f=0xEA 

T_STRUCT_TEST t12 : 
t12.a=0x386AFC 
t12.b=0xFFFFFFFF 
t12.c=0x24B9FD8 
t12.d=0x250238C 
t12.e=0x3C 
t12.f=0x23 

T_STRUCT_TEST t13 : 
t13.a=0x545753 
t13.b=0x324454 
t13.c=0x0 
t13.d=0x0 
t13.e=0x0 
t13.f=0x0 

T_STRUCT_TEST t2 = {0} : 
t2.a=0x0 
t2.b=0x0 
t2.c=0x0 
t2.d=0x0 
t2.e=0x0 
t2.f=0x0 

T_STRUCT_TEST t3 = {0xFF} : 
t3.a=0xFF 
t3.b=0x0 
t3.c=0x0 
t3.d=0x0 
t3.e=0x0 
t3.f=0x0 

T_STRUCT_TEST t4 = {0} : 
t4.a=0x0 
t4.b=0x0 
t4.c=0x0 
t4.d=0x0 
t4.e=0x0 
t4.f=0x0 

T_STRUCT_TEST t5 = {0} : 
t5.a=0x0 
t5.b=0x0 
t5.c=0x0 
t5.d=0x0 
t5.e=0x0 
t5.f=0x0 

T_STRUCT_TEST t6 = {0} : 
t6.a=0x0 
t6.b=0x0 
t6.c=0x0 
t6.d=0x0 
t6.e=0x0 
t6.f=0x0 
*/	
#endif



