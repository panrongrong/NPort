/* hwconf.c - Hardware configuration support module */

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

#include <vxWorks.h>
#include <vxBusLib.h>
#include <hwif/vxbus/vxBus.h>
#include <hwif/vxbus/vxbIntrCtlr.h>
#include <hwif/util/vxbParamSys.h>
#include <hwif/vxbus/hwConf.h>
#include "config.h"

#ifdef DRV_STORAGE_SDHC
#   include <h/storage/vxbSdMmcLib.h>
#   include <h/storage/vxbSdhcStorage.h>
#endif /* DRV_STORAGE_SDHC */

#ifdef INCLUDE_ALT_SOC_GEN5_DW_I2C
#   include <hwif/vxbus/vxbI2cLib.h>
#endif /* INCLUDE_ALT_SOC_GEN5_DW_I2C */


/*#define TEST_WH_YH*/

/*
 * Configuring GIC PL390 priority
 * This board is based on the GIC pl390 interrupt controller.
 * The pl390 implements 32 priority levels, this means the priority values in
 * the range of 0 to 248, and in steps of 8.
 * Example:
 *     values 0 - 7  level  0,
 *            8 - 15 level  1,
 *            ...
 */

LOCAL const struct intrCtlrPriority gicPriority[] = 
{
    /* pin,                priority */
#ifdef DRV_SIO_NS16550
    { INT_VEC_UART0,       120 },
    
	#ifdef DRV_UART_1_OPEN
		{ INT_VEC_UART1,	   120 },
	#endif
#endif
    
#ifdef INCLUDE_ALT_SOC_GEN5_TIMER	
    { INT_VEC_ALT_TIMER0,  120 },
    { INT_VEC_ALT_TIMER3,  120 }, /* */
#endif

#ifdef INCLUDE_DRV_FM_GTC	
	{ INT_VEC_GTC_TIMER,  120 },
#endif
	
#ifdef INCLUDE_ALT_SOC_GEN5_DW_END
	#ifdef INCLUDE_GMAC_0
    { INT_VEC_EMAC0,       100} ,  /*50*/
	#endif
	
	#ifdef INCLUDE_GMAC_1
	{ INT_VEC_EMAC1,	   100} ,  /*50*/
	#endif
#endif

#ifdef INCLUDE_AXI_ETHERNET_END

	{ INT_VEC_AXIENET0_0,		100},
	{ INT_VEC_AXIENET0_1,		100},

	//{ INT_VEC_AXIENET1_0,		100}, 
	//{ INT_VEC_AXIENET1_1,		100}, 

#endif
#ifdef DRV_FM_SDMMC    
	{ INT_VEC_SDMMC_0,		120 } ,
	#ifndef SD_CTRL1_AS_EMMC
	{ INT_VEC_SDMMC_1,  	120 } ,
	#endif
#endif

#ifdef DRV_FM_SPI
	{ INT_VEC_SPIINT0,		 120},
	/*{ INT_VEC_SPIINT1,     120},*/
#endif /* DRV_FM_SPI */

#ifdef DRV_FM_CAN
		{ INT_VEC_CAN_INT0,	   120},
		{ INT_VEC_CAN_INT0,	   120},
#endif /* DRV_FM_CAN */


#ifdef INCLUDE_SYNOPSYSHCI
	{ INT_VEC_USB0, 		120},
	/*{ INT_VEC_USB1,		120}, */
#endif /* INCLUDE_SYNOPSYSHCI */

#ifdef IRQ_PL_2_PS  /* PL --> PS irq */
	/*#ifndef TEST_WH_YH*/
	{ INT_VEC_IRQF2P_0, 		120},   /* for example */
	{ INT_VEC_IRQF2P_1, 		120},   /* for example */
	
	{ INT_VEC_IRQF2P_2, 		120},   /* for example */
	{ INT_VEC_IRQF2P_3, 		120},   /* for example */
	/*#endif*/
	{ INT_VEC_IRQF2P_4, 		120},   /* for example */
	{ INT_VEC_IRQF2P_15, 		120},   /* for example */
#endif

}; /* gicPriority[] */

/*
 * Configuring sensitivity
 * Setting each peripheral interrupt to be:
 * VXB_INTR_TRIG_LEVEL or VXB_INTR_TRIG_EDGE.
 *
 * SGIs sensitivity
 * The sensitivity types for SGIs are fixed and cannot be configure.
 *
 * PPIs and SPIs sensitivity
 * ALL PPIs and SPIs interrupt sensitivity types are fixed by the requesting
 * sources and cannot be changed. The GIC must be programmed to accommodate
 * this.
 */

LOCAL const struct intrCtlrTrigger gicTrigger[] = 
{
    /* pin,                     sensitivity */
#ifdef DRV_SIO_NS16550
    { INT_VEC_UART0,            VXB_INTR_TRIG_LEVEL },
    
	#ifdef DRV_UART_1_OPEN
		{ INT_VEC_UART1,			VXB_INTR_TRIG_LEVEL },
	#endif
#endif

    
#ifdef INCLUDE_ALT_SOC_GEN5_TIMER	
    { INT_VEC_ALT_TIMER0,       VXB_INTR_TRIG_LEVEL },
    { INT_VEC_ALT_TIMER3,       VXB_INTR_TRIG_LEVEL }, /**/
#endif

#ifdef INCLUDE_DRV_FM_GTC	
	{ INT_VEC_GTC_TIMER,  VXB_INTR_TRIG_LEVEL },
#endif

#ifdef INCLUDE_ALT_SOC_GEN5_DW_END
	#ifdef INCLUDE_GMAC_0
    { INT_VEC_EMAC0,            VXB_INTR_TRIG_LEVEL },
	#endif
	#ifdef INCLUDE_GMAC_1
	{ INT_VEC_EMAC1,			VXB_INTR_TRIG_LEVEL },
	#endif
#endif

#ifdef INCLUDE_AXI_ETHERNET_END

	{ INT_VEC_AXIENET0_0,		VXB_INTR_TRIG_RISING_EDGE},
	{ INT_VEC_AXIENET0_1,		VXB_INTR_TRIG_RISING_EDGE},

	//{ INT_VEC_AXIENET1_0,		VXB_INTR_TRIG_RISING_EDGE}, 
	//{ INT_VEC_AXIENET1_1,		VXB_INTR_TRIG_RISING_EDGE}, 

#endif
#ifdef DRV_FM_SDMMC    
	{ INT_VEC_SDMMC_0,			  VXB_INTR_TRIG_LEVEL } ,
	#ifndef SD_CTRL1_AS_EMMC
	{ INT_VEC_SDMMC_1,			  VXB_INTR_TRIG_LEVEL } ,
	#endif	
#endif

#ifdef DRV_FM_SPI
		{ INT_VEC_SPIINT0,		 VXB_INTR_TRIG_LEVEL},
		/*{ INT_VEC_SPIINT1,	   VXB_INTR_TRIG_LEVEL},*/
#endif /* DRV_FM_SPI */


#ifdef DRV_FM_CAN
		{ INT_VEC_CAN_INT0,		 VXB_INTR_TRIG_LEVEL},
		{ INT_VEC_CAN_INT1 ,	   VXB_INTR_TRIG_LEVEL},
#endif /* DRV_FM_CAN */

#ifdef INCLUDE_ALT_SOC_GEN5_WDT
	{ INT_VEC_WD0,			VXB_INTR_TRIG_LEVEL } ,
#endif /* INCLUDE_ALT_SOC_GEN5_WDT */
		
#ifdef INCLUDE_ALT_SOC_GEN5_DW_I2C
	{ INT_VEC_I2C0, 		 VXB_INTR_TRIG_LEVEL},
	{ INT_VEC_I2C1, 		 VXB_INTR_TRIG_LEVEL},
#endif /* INCLUDE_ALT_SOC_GEN5_DW_I2C */
	
#ifdef INCLUDE_SYNOPSYSHCI
	{ INT_VEC_USB0, 	    VXB_INTR_TRIG_LEVEL},
	/*{ INT_VEC_USB1, 	    VXB_INTR_TRIG_LEVEL}, */
#endif /* INCLUDE_SYNOPSYSHCI */

#ifdef IRQ_PL_2_PS  /* PL --> PS irq trig mode */
	/*
	 * Trigger configure type:
	 *
	 * VXB_INTR_TRIG_FALLING_EDGE ==  edge triggered,
	 *								  falling edge
	 * VXB_INTR_TRIG_RISING_EDGE  ==  edge triggered,
	 *								  rising edge
	 * VXB_INTR_TRIG_ACTIVE_LOW   ==  level sensitive,
	 *								  active low
	 * VXB_INTR_TRIG_ACTIVE_HIGH  ==  level sensitive,
	 *								  active high
	 */
	/*#ifndef TEST_WH_YH*/
	{ INT_VEC_IRQF2P_0, 	    VXB_INTR_TRIG_RISING_EDGE},   /* for example */
	{ INT_VEC_IRQF2P_1, 	    VXB_INTR_TRIG_RISING_EDGE},   /* for example */
	
	{ INT_VEC_IRQF2P_2, 	    VXB_INTR_TRIG_RISING_EDGE},   /* for example */
	{ INT_VEC_IRQF2P_3, 	    VXB_INTR_TRIG_RISING_EDGE},   /* for example */
	/*#endif*/
	
	{ INT_VEC_IRQF2P_4, 	    VXB_INTR_TRIG_RISING_EDGE},   /* for example */
	{ INT_VEC_IRQF2P_15, 	    VXB_INTR_TRIG_RISING_EDGE},   /* for example */
#endif

};  /* gicTrigger[] */

#define NET_NAME   "gem"
#define AXI_NET_NAME   "axienet"


LOCAL struct intrCtlrInputs gicInputs[] = 
{
    /* pin,                driver,            unit,   index */

#ifdef DRV_SIO_NS16550
	{ INT_VEC_UART0,      "ns16550",           0,       0 },
	
	#ifdef DRV_UART_1_OPEN
		{ INT_VEC_UART1,	  "ns16550",		   1,		0 },
	#endif
#endif
	
#ifdef INCLUDE_ALT_SOC_GEN5_TIMER	
    { INT_VEC_ALT_TIMER0, "altTimer",          0,       0 },  /* ttc0- timer0,1,2 */
    { INT_VEC_ALT_TIMER3, "altTimer",          1,       0 },  /**/ /* ttc1- timer3,4,5 */
#endif

#ifdef INCLUDE_DRV_FM_GTC	
	{ INT_VEC_GTC_TIMER,  "gtcTimer",     0,       0 }, /* gtc timer: armv7GenTimer */
#endif

#ifdef INCLUDE_ALT_SOC_GEN5_DW_END
	#ifdef INCLUDE_GMAC_0
	{ INT_VEC_EMAC0,		NET_NAME,		   0,	0 },
	#endif
	#ifdef INCLUDE_GMAC_1
	{ INT_VEC_EMAC1,		NET_NAME,	       1,	0 }, 
	#endif
#endif

#ifdef INCLUDE_AXI_ETHERNET_END

	{ INT_VEC_AXIENET0_0,		AXI_NET_NAME,		   0,	0 },
	{ INT_VEC_AXIENET0_1,		AXI_NET_NAME,		   0,	1 },

	//{ INT_VEC_AXIENET1_0,		AXI_NET_NAME,	       1,	0 }, 
	//{ INT_VEC_AXIENET1_1,		AXI_NET_NAME,	       1,	1 }, 

#endif
#ifdef DRV_FM_SDMMC
	#if (defined(SDMMC_VXB1) || defined(SDMMC_VXB2))
	{ INT_VEC_SDMMC_0,		"fmSdmmc", 0,	0 },
	{ INT_VEC_SDMMC_1,		"fmSdmmc", 1,	0 },  
	#endif
	
	#ifdef SD_DOSFS_V1  /* ref_712: sd demo_board vx dosfs*/
	{ INT_VEC_SDMMC_0,		"altSocGen5DwMshc", 0,	0 },
	{ INT_VEC_SDMMC_1,		"altSocGen5DwMshc", 1,	0 },
	#endif
	
	#ifdef SDMMC_DOSFS_V2 /* ref_712: sd / emmc demo_board vx dosfs 	*/
	{ INT_VEC_SDMMC_0,		"fmshSdhci", 0,	0 },
	#ifndef SD_CTRL1_AS_EMMC
	{ INT_VEC_SDMMC_1,		"fmshSdhci", 1,	0 }, 
	#endif
	#endif
#endif /* DRV_FM_SDMMC */

#ifdef DRV_FM_SPI
	{ INT_VEC_SPIINT0,		 "fmshSpi"	,  0, 0},
	/*{ INT_VEC_SPIINT1,		 "fmshSpi"	,  1, 0},*/
#endif /* DRV_FM_SPI */

#ifdef DRV_FM_CAN
	{ INT_VEC_CAN_INT0,		 "fmshCan"	,  0, 0},
	{ INT_VEC_CAN_INT1,		 "fmshCan"	,  1, 0},
#endif /* DRV_FM_CAN */


#ifdef INCLUDE_ALT_SOC_GEN5_WDT
	{ INT_VEC_WD0,			 "altWdt",		    0,   0 },
#endif /* INCLUDE_ALT_SOC_GEN5_WDT */
	
#ifdef INCLUDE_ALT_SOC_GEN5_DW_I2C
	{ INT_VEC_I2C0, 		  "dwI2c",			0,	 0 },
	{ INT_VEC_I2C1, 		  "dwI2c",			1,	 0 },
#endif /* INCLUDE_ALT_SOC_GEN5_DW_I2C */

#ifdef INCLUDE_FMSH_FMQL_GPIO
    { INT_VEC_GPIO_BANK_A,    "legacy",       0,      INT_VEC_GPIO_BANK_A },
    { INT_VEC_GPIO_BANK_B,    "legacy",       0,      INT_VEC_GPIO_BANK_B },
    { INT_VEC_GPIO_BANK_C,    "legacy",       0,      INT_VEC_GPIO_BANK_C },
    { INT_VEC_GPIO_BANK_D,    "legacy",       0,      INT_VEC_GPIO_BANK_D },
#endif /* INCLUDE_FMSH_FMQL_GPIO */

#ifdef INCLUDE_SYNOPSYSHCI
	{ INT_VEC_USB0, 	   "vxbPlbUsbSynopsysHci", 0,	 0 },
	/* { INT_VEC_USB1, 	   "vxbPlbUsbSynopsysHci", 0,	 0 }, */
#endif /* INCLUDE_SYNOPSYSHCI */

#ifdef IRQ_PL_2_PS  /* PL --> PS irq */
	/*#ifndef TEST_WH_YH*/
	{ INT_VEC_IRQF2P_0, 	"pl_irq",	    0,   0},	/* for example */
	{ INT_VEC_IRQF2P_1, 	"pl_irq",	    0,   0},	/* for example */
	
	{ INT_VEC_IRQF2P_2, 	"pl_irq",	    0,   0},	/* for example */
	{ INT_VEC_IRQF2P_3, 	"pl_irq",	    0,   0},	/* for example */
	/*#endif*/
	{ INT_VEC_IRQF2P_4, 	"pl_irq",	    0,   0},	/* for example */
	{ INT_VEC_IRQF2P_15, 	"pl_irq",	    0,   0},	/* for example */
#endif

}; /* gicInputs[] */

/*
 * if this file is used in SMP environment, the ethernet interface can be
 * re-routed to CPU 1 by changing "0" to "1", "2" in the gicCpu[] structure.
 */

LOCAL struct intrCtlrCpu gicCpu[] = {
#if 0
    { INT_VEC_ETH0, 0 },
#else
   /* { INT_VEC_EMAC0, 0 }, */
#endif
	{ INT_VEC_ALT_TIMER0, 0 },
};

LOCAL const struct hcfResource armGIC0Resources[] = 
{
    { "regBase",           HCF_RES_INT,  {(void *)FMQL_GIC_BASE} },
    
    { "input",             HCF_RES_ADDR, {(void *)&gicInputs[0]} },    
    { "intMode",           HCF_RES_INT,  {(void *)INT_MODE} },
    { "maxIntLvl",         HCF_RES_INT,  {(void *)SYS_INT_LEVELS_MAX} },
    { "inputTableSize",    HCF_RES_INT,  {(void *)NELEMENTS(gicInputs) } },
    
    { "cpuRoute",          HCF_RES_ADDR, {(void *)&gicCpu[0]} },
    { "cpuRouteTableSize", HCF_RES_INT,  {(void *)NELEMENTS(gicCpu)} },
    
    { "priority",          HCF_RES_ADDR, {(void *)&gicPriority[0]} },
    { "priorityTableSize", HCF_RES_INT,  {(void *)NELEMENTS(gicPriority) } },
    
    { "trigger",           HCF_RES_ADDR, {(void *)&gicTrigger[0]} },
    { "triggerTableSize",  HCF_RES_INT,  {(void *)NELEMENTS(gicTrigger) } },
    
    { "distOffset",        HCF_RES_INT,  {(void *)FMQL_GIC_DIST_OFFSET } },
    { "cpuOffset",         HCF_RES_INT,  {(void *)FMQL_GIC_CPU_OFFSET } }
	
#ifdef _WRS_CONFIG_SMP
    ,{ "maxCpuNum",         HCF_RES_INT,  {(void *)VX_SMP_NUM_CPUS} }
#endif /* _WRS_CONFIG_SMP */
};
#define armGIC0Num NELEMENTS(armGIC0Resources)


/*
SIO_NS16550; ps-uart_0
*/
#ifdef DRV_SIO_NS16550
struct hcfResource altera16550Dev0Resources[] = {
    { "regBase",     HCF_RES_INT,  {(void *) UART_0_BASE_ADR} },
    { "irq",         HCF_RES_INT,  {(void *) INT_VEC_UART0} },
    { "regInterval", HCF_RES_INT,  {(void *) 0x04} },
    { "irqLevel",    HCF_RES_INT,  {(void *) INT_VEC_UART0} },
    { "clkFreq",     HCF_RES_INT,  {(void *) UART_XTAL_FREQ} },
    { "fifoLen",     HCF_RES_INT,  {(void *) 1} }
};
#define fmqlSioDev0Num NELEMENTS(altera16550Dev0Resources)

/*
NS16550; ps-uart_1
*/
#ifdef DRV_UART_1_OPEN
struct hcfResource altera16550Dev1Resources[] = {
    { "regBase",     HCF_RES_INT,  {(void *) UART_1_BASE_ADR} },
    { "irq",         HCF_RES_INT,  {(void *) INT_VEC_UART1} },
    { "regInterval", HCF_RES_INT,  {(void *) 0x04} },
    { "irqLevel",    HCF_RES_INT,  {(void *) INT_VEC_UART1} },
    { "clkFreq",     HCF_RES_INT,  {(void *) UART_XTAL_FREQ} },
    { "fifoLen",     HCF_RES_INT,  {(void *) 1} }
};
#define fmqlSioDev1Num NELEMENTS(altera16550Dev1Resources)
#endif
#endif  /* #ifdef DRV_SIO_NS16550 */

/*
ttc
*/
#ifdef INCLUDE_ALT_SOC_GEN5_TIMER
struct hcfResource alteraTimerDev0Resources[] = {
    { "regBase",    HCF_RES_INT, {(void *)FMQL_TTC0_TIMER0_BASE} },  /* ttc0- timer0, 1, 2 */
    
    { "irq",        HCF_RES_INT, {(void *)INT_VEC_ALT_TIMER0} },
    { "irqLevel",   HCF_RES_INT, {(void *)INT_LVL_ALT_TIMER0} },
    
    { "minClkRate", HCF_RES_INT, {(void *)SYS_CLK_RATE_MIN } },
    { "maxClkRate", HCF_RES_INT, {(void *)SYS_CLK_RATE_MAX} },
    
	/*{ "clkFreq",	HCF_RES_INT, {(void *FMQL_TTC_CLK } }*/
    { "clkFreq",    HCF_RES_ADDR, {(void *)clk_ttc_get } } 
};
#define fmqlTimerDev0Num NELEMENTS(alteraTimerDev0Resources)

#if 0
struct hcfResource alteraTimerDev0Resources[] = {
    { "regBase",    HCF_RES_INT, {(void *)FMQL_TTC0_TIMER1_BASE} },  /* ttc0- timer0, 1, 2 */
    
    { "irq",        HCF_RES_INT, {(void *)INT_VEC_ALT_TIMER1} },
    { "irqLevel",   HCF_RES_INT, {(void *)INT_LVL_ALT_TIMER1} },
    
    { "minClkRate", HCF_RES_INT, {(void *)SYS_CLK_RATE_MIN } },
    { "maxClkRate", HCF_RES_INT, {(void *)SYS_CLK_RATE_MAX} },
    { "clkFreq",    HCF_RES_INT, {(void *)FMQL_TTC_CLK } }
};
#define fmqlTimerDev1Num NELEMENTS(alteraTimerDev1Resources)
#endif

struct hcfResource alteraTimerDev3Resources[] = {
    { "regBase",    HCF_RES_INT, {(void *)FMQL_TTC1_TIMER0_BASE} },  /* ttc1- timer3, 4, 5 */
    
    { "irq",        HCF_RES_INT, {(void *)INT_VEC_ALT_TIMER3} },
    { "irqLevel",   HCF_RES_INT, {(void *)INT_LVL_ALT_TIMER3} },
    
    { "minClkRate", HCF_RES_INT, {(void *)SYS_CLK_RATE_MIN } },
    { "maxClkRate", HCF_RES_INT, {(void *)SYS_CLK_RATE_MAX} },
    
    /*{ "clkFreq",    HCF_RES_INT, {(void *FMQL_TTC_CLK } }	*/
    { "clkFreq",    HCF_RES_ADDR, {(void *)clk_ttc_get } } 
};
#define fmqlTimerDev3Num NELEMENTS(alteraTimerDev3Resources)
#endif

#ifdef INCLUDE_DRV_FM_GTC	
struct hcfResource gtcTimerDevResources[] = {
	{ "regBase",	HCF_RES_INT, {(void *)FMQL_GTC_TIMER_BASE} },  /* ARM generic timer(GTC) */
	
	{ "irq",		HCF_RES_INT, {(void *)INT_VEC_GTC_TIMER} },
	{ "irqLevel",	HCF_RES_INT, {(void *)INT_VEC_GTC_TIMER} },
	
/*	{ "minClkRate", HCF_RES_INT, {(void *SYS_CLK_RATE_MIN } },*/
/*	{ "maxClkRate", HCF_RES_INT, {(void *SYS_CLK_RATE_MAX} },*/
	{ "clkFreq",	HCF_RES_INT, {(void *)16666666 } }
};
#define gtcTimerDevNum NELEMENTS(gtcTimerDevResources)
#endif



#ifdef INCLUDE_ALT_SOC_GEN5_DW_I2C
#undef DRV_I2C_EEPROM
#undef DRV_I2C_RTC
LOCAL struct i2cDevInputs i2c0DevTbl[] = {

/*
 * Address  Part Number  Size         Description
 * -------- ------------------------------------------------------------------
 * 0x51       at24c32     4096 byte   eeprom
 * 0x68       ds1339      17 byte     RTC
 *
 */
 { "i2c_dev0",          0x39,           0 },  /* for demo_board; hdmi-sii9134: 0x72>>1 == 0x39 */
 
#ifdef DRV_I2C_EEPROM
 { "eeprom_at24c32",    0x51,           I2C_WORDADDR},
#endif /* DRV_I2C_EEPROM */

#ifdef DRV_I2C_RTC
 { "rtc_ds1339",        0x68,           0},
#endif /* DRV_I2C_RTC */

};

struct hcfResource dwI2cDev0Resources[]= {
    { "regBase",    HCF_RES_ADDR,   {(void *)I2C_0_BASE_ADRS} },
    { "clkFreq",    HCF_RES_INT,    {(void *)FMQL_APB_CLK}},
    { "busSpeed",   HCF_RES_INT,    {(void *)400000}},
    { "polling",    HCF_RES_INT,    {(void *)FALSE}},
    { "i2cDev",     HCF_RES_ADDR,   { (void *)&i2c0DevTbl}},
    { "i2cDevNum",  HCF_RES_INT,    { (void *)NELEMENTS(i2c0DevTbl)}},
};
#define dwI2cDev0Num NELEMENTS(dwI2cDev0Resources)
#endif /* INCLUDE_ALT_SOC_GEN5_DW_I2C */



/*
gmac (net)
*/
#ifdef INCLUDE_ALT_SOC_GEN5_DW_END

#ifdef INCLUDE_GMAC_0
const struct hcfResource vxbDwEmacDev0Resources[]= 
{
    { "regBase",    HCF_RES_INT,    { (void *)(EMAC_0_BASE_ADRS)} },
	
    { "phyAddr",    HCF_RES_INT,    { (void *)GMAC_0_PHY_ADDR } },
    
    { "miiIfName",  HCF_RES_STRING, { (void *)NET_NAME } },    
    { "miiIfUnit",  HCF_RES_INT,    { (void *)0 } }
	
	/* { "jumboEnable", HCF_RES_INT,    {(void *1}}    // 1-enable*/
};
#define vxbDwEmacDev0Num NELEMENTS(vxbDwEmacDev0Resources)

const struct hcfResource genPhyMdio0Resources[] = 
{
    { "phyAddr",   HCF_RES_INT,        { (void *)(GMAC_0_PHY_ADDR)} },
};
#define genPhyMdio0Num NELEMENTS(genPhyMdio0Resources)
#endif

#ifdef INCLUDE_GMAC_1
const struct hcfResource vxbDwEmacDev1Resources[]= 
{
    { "regBase",    HCF_RES_INT,    { (void *)(EMAC_1_BASE_ADRS)} },
	
    { "phyAddr",    HCF_RES_INT,    { (void *)GMAC_1_PHY_ADDR } },
    
    { "miiIfName",  HCF_RES_STRING, { (void *)NET_NAME } },    
    { "miiIfUnit",  HCF_RES_INT,    { (void *)0 } }
    
	/* { "jumboEnable", HCF_RES_INT,    {(void *1}}    // 1-enable*/
};
#define vxbDwEmacDev1Num NELEMENTS(vxbDwEmacDev1Resources)

const struct hcfResource genPhyMdio1Resources[] = 
{
    { "phyAddr",   HCF_RES_INT,        { (void *)(GMAC_1_PHY_ADDR)} },
};
#define genPhyMdio1Num NELEMENTS(genPhyMdio1Resources)
#endif

#endif

/*#undef INCLUDE_AXI_ETHERNET_END*/
#ifdef INCLUDE_AXI_ETHERNET_END

#define AXIEMAC_0_PHY_ADDR 7
#define AXIEMAC_0_BASE_ADRS 0x41000000
#define AXIEMAC_0_DMA_ADRS 0x40400000

const struct hcfResource vxbAxiEthDev0Resources[]= 
{
    { "regBase",    HCF_RES_INT,    { (void *)(AXIEMAC_0_BASE_ADRS)} },
	{ "regBase1",    HCF_RES_INT,    { (void *)(AXIEMAC_0_DMA_ADRS)} },
    { "phyAddr",    HCF_RES_INT,    { (void *)AXIEMAC_0_PHY_ADDR } },
    
    { "miiIfName",  HCF_RES_STRING, { (void *)AXI_NET_NAME } },    
    { "miiIfUnit",  HCF_RES_INT,    { (void *)0 } }
	
	/* { "jumboEnable", HCF_RES_INT,    {(void *1}}    // 1-enable*/
};
#define vxbAxiEthDev0Num NELEMENTS(vxbAxiEthDev0Resources)

const struct hcfResource genPhyAxiMdio0Resources[] = 
{
    { "phyAddr",   HCF_RES_INT,        { (void *)(AXIEMAC_0_PHY_ADDR)} },
};
#define genPhyAxiMdio0Num NELEMENTS(genPhyAxiMdio0Resources)


const struct hcfResource vxbAxiEthDev1Resources[]= 
{
    { "regBase",    HCF_RES_INT,    { (void *)(AXIEMAC_0_BASE_ADRS)} },
	{ "regBase1",    HCF_RES_INT,    { (void *)(AXIEMAC_0_DMA_ADRS)} },
    { "phyAddr",    HCF_RES_INT,    { (void *)AXIEMAC_0_PHY_ADDR } },
    
    { "miiIfName",  HCF_RES_STRING, { (void *)AXI_NET_NAME } },    
    { "miiIfUnit",  HCF_RES_INT,    { (void *)1 } }
	
	/* { "jumboEnable", HCF_RES_INT,    {(void *1}}    // 1-enable*/
};
#define vxbAxiEthDev1Num NELEMENTS(vxbAxiEthDev1Resources)

const struct hcfResource genPhyAxiMdio1Resources[] = 
{
    { "phyAddr",   HCF_RES_INT,        { (void *)(AXIEMAC_0_PHY_ADDR)} },
};
#define genPhyAxiMdio1Num NELEMENTS(genPhyAxiMdio1Resources)

#endif


/*
wdt(watchdog timer)
*/
#ifdef INCLUDE_ALT_SOC_GEN5_WDT
struct hcfResource alteraWdtDev0Resources[]= 
{
    { "regBase",    HCF_RES_ADDR, {(void *)WDT_0_BASE_ADRS} },
    { "clkFreq",    HCF_RES_INT,   { (void *)FMQL_TTC_CLK}},  /* 100M Hz */
    
    { "minClkRate", HCF_RES_INT, {(void *)SYS_CLK_RATE_MIN } },
    { "maxClkRate", HCF_RES_INT, {(void *)SYS_CLK_RATE_MAX} },
    { "WdtChanNo",  HCF_RES_INT, {(void *)0}},
    { "resetMgrBase", HCF_RES_ADDR, {(void *)FMQL_SR_BASE} }, 
};
#define alteraWdtDev0Num NELEMENTS(alteraWdtDev0Resources)
#endif /* INCLUDE_ALT_SOC_GEN5_WDT */


/*
sd/mmc
*/
#ifdef  DRV_FM_SDMMC

/* #include <h/storage/vxbAltSocGen5DwMshc.h> */
/*#include "./bsp_drv/sdmmc/vxbSdmmc_fm.h" */

IMPORT void sysMshcClkFreqSetup (UINT32 clk);


/************************************************
*                                               *
****************   sdmmc-0   ********************
*                                               *
************************************************/
#ifdef SDMMC_BARE_METAL
LOCAL const struct hcfResource fmSdmmcResources[] = 
{
    { "regBase",        HCF_RES_INT,    { (void *)FM_SDMMC_0_BASE } },
    { "clkFreq",        HCF_RES_INT,    { (void *)FM_SDMMC_CLK_DIVIDED_VAL} },
    
    { "rwMode",         HCF_RES_INT,    { (void *)1 } },  /* FM_SDMMC_RW_MODE_PIO */
    { "polling",        HCF_RES_INT,    { (void *)TRUE /*FALSE  */ } },
    
    { "clkFreqSetup",   HCF_RES_ADDR,   { (void *)sysMshcClkFreqSetup } },
    { "flags",          HCF_RES_INT,    { (void *)(6) } },
};
#define fmSdmmcNum NELEMENTS(fmSdmmcResources)
#endif  /* SDMMC_BARE_METAL */

#ifdef SDMMC_2_VX
LOCAL const struct hcfResource fmSdmmcResources[] = 
{
    { "regBase",        HCF_RES_INT,    { (void *)FM_SDMMC_0_BASE } },    
    /*{ "fifoBase",        HCF_RES_INT,    { (void *QSPI0_D_BASE_ADRS } },    */
    { "clkFreq",        HCF_RES_INT,    { (void *)FM_SDMMC_CLK_DIVIDED_VAL} },
    
    { "rwMode",         HCF_RES_INT,    { (void *)FM_SDMMC_RW_MODE_PIO } },
    { "polling",        HCF_RES_INT,    { (void *)TRUE /*FALSE  */ } },
    
    { "flags",          HCF_RES_INT,    { (void *)(FM_SDMMC_FLAGS_CARD_PRESENT | FM_SDMMC_FLAGS_CARD_WRITABLE) } },
};
#define fmSdmmcNum NELEMENTS(fmSdmmcResources)
#endif  /* SDMMC_2_VX */


#ifdef SDMMC_DOSFS_V2  /* ref_712 sd/emmc dosfs*/
#define SDHC_DMA_MODE_PIO   (1)
LOCAL const struct hcfResource fmSdmmcResources_dosFS[] = 
{
    { "regBase",        HCF_RES_INT,    { (void *)FM_SDMMC_0_BASE } },
    { "clkFreq",        HCF_RES_INT,    { (void *)FM_SDMMC_CLK_DIVIDED_VAL} },
    
    { "dmaMode",         HCF_RES_INT,    { (void *)SDHC_DMA_MODE_PIO } },  /* for sd_0:-> emmc of verify_board*/
    { "polling",        HCF_RES_INT,    { (void *) FALSE  /* TRUE*/} },    /*          -> irq_mode*/

    { "clkFreqSetup",   HCF_RES_ADDR,   { (void *)fmshSdhc_ClkFreqSetup } },  /*sysMshcClkFreqSetup*/
    { "flags",          HCF_RES_INT,    { (void *)(ALT_MSHC_FLAGS_CARD_PRESENT | ALT_MSHC_FLAGS_CARD_WRITABLE) } },
};
#define fmSdmmcNum_dosFS NELEMENTS(fmSdmmcResources_dosFS)
#endif  /* #ifdef SDMMC_DOSFS_712 */


#ifdef SD_DOSFS_V1	/* ref_712: sd demo_board vx dosfs*/
LOCAL const struct hcfResource alteraMshcResources[] = 
{
    { "regBase",        HCF_RES_INT,    { (void *)0xE0043000 } }, /*ALT_SDMMC_0_BASE*/
    
    { "clkFreq",        HCF_RES_INT,    { (void *)(25000000)} }, /*ALT_SDMMC_CLK_DIVIDED_VAL*/
    
    { "rwMode",         HCF_RES_INT,    { (void *)ALT_DW_MSHC_RW_MODE_PIO } },
    { "polling",        HCF_RES_INT,    { (void *)TRUE /*FALSE  */ } },
    
    { "clkFreqSetup",   HCF_RES_ADDR,   { (void *)sysMshcClkFreqSetup } },
    { "flags",          HCF_RES_INT,    { (void *)(ALT_MSHC_FLAGS_CARD_PRESENT | ALT_MSHC_FLAGS_CARD_WRITABLE) } },
};
#define alteraMshcNum NELEMENTS(alteraMshcResources)
#endif




/************************************************
*                                               *
****************   sdmmc-1  ********************
*                                               *
************************************************/
#ifdef SDMMC_BARE_METAL
LOCAL const struct hcfResource fmSdmmcResources2[] = 
{
    { "regBase",        HCF_RES_INT,    { (void *)FM_SDMMC_1_BASE } },
    { "clkFreq",        HCF_RES_INT,    { (void *)FM_SDMMC_CLK_DIVIDED_VAL} },
    
    { "rwMode",         HCF_RES_INT,    { (void *)1 } },
    { "polling",        HCF_RES_INT,    { (void *)TRUE /*FALSE  */ } },
    
  /*  { "clkFreqSetup",   HCF_RES_ADDR,   { (void *)sysMshcClkFreqSetup } },  */
    { "flags",          HCF_RES_INT,    { (void *)(6) } },
};
#define fmSdmmcNum2 NELEMENTS(fmSdmmcResources2)
#endif /* SDMMC_BARE_METAL */

#ifdef SDMMC_2_VX
LOCAL const struct hcfResource fmSdmmcResources2[] = 
{
    { "regBase",        HCF_RES_INT,    { (void *)FM_SDMMC_1_BASE } },    
   /* { "fifoBase",        HCF_RES_INT,    { (void *QSPI1_D_BASE_ADRS } },    */
    { "clkFreq",        HCF_RES_INT,    { (void *)FM_SDMMC_CLK_DIVIDED_VAL} },
    
    { "rwMode",         HCF_RES_INT,    { (void *)FM_SDMMC_RW_MODE_PIO } },
    { "polling",        HCF_RES_INT,    { (void *)TRUE /*FALSE  */ } },
    
    { "flags",          HCF_RES_INT,    { (void *)(FM_SDMMC_FLAGS_CARD_PRESENT | FM_SDMMC_FLAGS_CARD_WRITABLE) } },
};
#define fmSdmmcNum2 NELEMENTS(fmSdmmcResources2)
#endif /* SDMMC_2_VX */


#ifdef SDMMC_DOSFS_V2  /* ref_712 sd/emmc dosfs: sd_ctrl_2*/
#define SDHC_DMA_MODE_PIO   (1)
LOCAL const struct hcfResource fmSdmmcResources2_dosFS[] = 
{
    { "regBase",        HCF_RES_INT,    { (void *)FM_SDMMC_1_BASE } },    
    { "clkFreq",        HCF_RES_INT,    { (void *)FM_SDMMC_CLK_DIVIDED_VAL} },
    
    /*{ "rwMode",         HCF_RES_INT,    { (void *ALT_DW_MSHC_RW_MODE_PIO } },  // pio mode for sd_1: -> sd_card of verify_board*/
    { "dmaMode",         HCF_RES_INT,    { (void *)SDHC_DMA_MODE_PIO } },  /* pio mode   */
    { "polling",        HCF_RES_INT,    { (void *)FALSE  /* TRUE */} },     /*                    -> irq mode*/

    { "clkFreqSetup",   HCF_RES_ADDR,   { (void *)fmshSdhc_ClkFreqSetup } },  /*sysMshcClkFreqSetup*/
    { "flags",          HCF_RES_INT,    { (void *)(ALT_MSHC_FLAGS_CARD_PRESENT | ALT_MSHC_FLAGS_CARD_WRITABLE) } },
};
#define fmSdmmcNum2_dosFS NELEMENTS(fmSdmmcResources2_dosFS)
#endif  /* #ifdef SDMMC_DOSFS_712 */


#endif /* DRV_FM_SDMMC */

/*
QSPI
*/
#ifdef DRV_FM_QSPI

LOCAL struct hcfResource fmQspiResources[] =  
{
    { "regBase",       HCF_RES_INT,   { (void *)(QSPI0_BASE_ADRS) } },
    { "fifoBase",       HCF_RES_INT,   { (void *)(QSPI0_D_BASE_ADRS) } },
    
    { "clkFreq",       HCF_RES_INT,   { (void *)(100000000)  } },   /* 100M */
    
    { "polling",       HCF_RES_INT,   { (void *)(TRUE) } },
    
    { "dmaMode",       HCF_RES_INT,   { (void *)(FALSE) } },
};
#define fmQspiNum NELEMENTS(fmQspiResources)
#endif




/*
SPI
*/
#ifdef DRV_FM_SPI

struct hcfResource fmSpi0Resources[] =  
{
    { "regBase",       HCF_RES_INT,   { (void *)(SPI0_BASE_ADRS) } },
    
    { "clkFreq",       HCF_RES_INT,   { (void *)(100000000) } },
    { "polling",       HCF_RES_INT,   { (void *)(TRUE) } },

    { "dmaMode",       HCF_RES_INT,   { (void *)(FALSE) } },
    
    { "isSlave",       HCF_RES_INT,   { (void *)(FALSE) } },  /* spi0-master */
};
#define fmSpi0Num  NELEMENTS(fmSpi0Resources)

struct hcfResource fmSpi1Resources[] =  
{
    { "regBase",       HCF_RES_INT,   { (void *)(SPI1_BASE_ADRS) } },
    
    { "clkFreq",       HCF_RES_INT,   { (void *)(100000000) } },
    { "polling",       HCF_RES_INT,   { (void *)(TRUE) } },

    { "dmaMode",       HCF_RES_INT,   { (void *)(FALSE) } },
    
    { "isSlave",       HCF_RES_INT,   { (void *)(TRUE) } },  /* spi1-slave */
};
#define fmSpi1Num  NELEMENTS(fmSpi1Resources)

#endif /* DRV_FM_SPI */

/*
CAN
*/
#ifdef DRV_FM_CAN

struct hcfResource fmCan0Resources[] =  
{
    { "regBase",       HCF_RES_INT,   { (void *)(CAN0_BASE_ADRS) } },
    
    { "clkFreq",       HCF_RES_INT,   { (void *)(FPAR_CANPS_0_CAN_CLK_FREQ_HZ) } },
    { "polling",       HCF_RES_INT,   { (void *)(TRUE) } },

    { "dmaMode",       HCF_RES_INT,   { (void *)(FALSE) } },   
};
#define fmCan0Num  NELEMENTS(fmCan0Resources)

struct hcfResource fmCan1Resources[] =  
{
    { "regBase",       HCF_RES_INT,   { (void *)(CAN1_BASE_ADRS) } },
    
    { "clkFreq",       HCF_RES_INT,   { (void *)(FPAR_CANPS_1_CAN_CLK_FREQ_HZ) } },
    { "polling",       HCF_RES_INT,   { (void *)(TRUE) } },

    { "dmaMode",       HCF_RES_INT,   { (void *)(FALSE) } },
};
#define fmCan1Num  NELEMENTS(fmCan1Resources)

#endif /* DRV_FM_CAN */


/*
NFC
*/
#ifdef DRV_FM_NFC

struct hcfResource fmNfcResources[] =  
{
    { "regBase",       HCF_RES_INT,   { (void *)(NFC_S_BASE_ADRS) } },
    
    { "clkFreq",       HCF_RES_INT,   { (void *)(100000000) } },
    { "polling",       HCF_RES_INT,   { (void *)(TRUE) } },

    { "dmaMode",       HCF_RES_INT,   { (void *)(FALSE) } }, 
    
    { "busWidth",       HCF_RES_INT,   { (void *)(16) } },   
};
#define fmNfcNum  NELEMENTS(fmNfcResources)

#endif /* DRV_FM_NFC */

/*
usb
*/
#ifdef INCLUDE_SYNOPSYSHCI
IMPORT void usbAltSocGen5USBPHYInit(void);

#ifndef USB_SYNOPSYSHCI_PLATFORM_ALTERA_SOC_GEN5
#define USB_SYNOPSYSHCI_PLATFORM_ALTERA_SOC_GEN5 0x1
#endif

/* 
USB0 has been connected with SD card. 
USB1 has linked with J1. ???? error ???
*/
const struct hcfResource vxbPlbUsbSynopsysHciDev0Resources[] = 
{
    { "addressMode",         HCF_RES_INT,   {(void *)0}},  /* 1-64bit, 0-32bit */
    { "regBaseLow",          HCF_RES_INT,   {(void *)(USB_0_BASE_ADRS)}},
    
    { "rootHubNumPorts",     HCF_RES_INT,   {(void *)1}},
    { "hostNumDmaChannels",  HCF_RES_INT ,  {(void *)16} },
    { "numEndpoints",        HCF_RES_INT ,  {(void *)16} },
    
  /* { "dmaEnabled",          HCF_RES_INT ,  {(void *)1} },*/
     { "dmaEnabled",          HCF_RES_INT ,  {(void *)0} }, 
    
    { "phyInit",             HCF_RES_ADDR,  {(void *)usbAltSocGen5USBPHYInit}},
    { "platformType",        HCF_RES_INT ,  {(void *)USB_SYNOPSYSHCI_PLATFORM_ALTERA_SOC_GEN5} },
    /*{ "platformType",        HCF_RES_INT ,  {(void *)0} }, */
    
    { "cpuToBus",            HCF_RES_ADDR,  {(void *)NULL}},
    { "busToCpu",            HCF_RES_ADDR,  {(void *)NULL}},
};

#define vxbPlbUsbSynopsysHciDev0Num NELEMENTS(vxbPlbUsbSynopsysHciDev0Resources)
#endif /* INCLUDE_SYNOPSYSHCI */


#ifdef PL_PCIE_RC_VXB

LOCAL  const struct hcfResource armPlAxiPci0Resources[] = 
{
    { "regBase",        HCF_RES_INT,    { (void *)0x50000000 } },
    
    { "mem32Addr",      HCF_RES_ADDR,   { (void *)0x60000000 } },
    { "mem32Size",      HCF_RES_INT,    { (void *)0x10000000 } },
    
    { "memIo32Addr",    HCF_RES_ADDR,   { (void *)0x60000000 } },
    { "memIo32Size",    HCF_RES_INT,    { (void *)0x10000000 } },
    
    { "io32Addr",       HCF_RES_ADDR,   { (void *)0 } },
    { "io32Size",       HCF_RES_INT,    { (void *)0 } },
    
    { "io16Addr",       HCF_RES_ADDR,   { (void *)0 } },
    { "io16Size",       HCF_RES_INT,    { (void *)0 } },
    
    { "cacheSize",      HCF_RES_INT,    { (void *)(16/*_CACHE_ALIGN_SIZE*/ / 4) } },
    
    { "maxBusSet",      HCF_RES_INT,    { (void *)3 } },
    
    { "maxLatAllSet",   HCF_RES_INT,    { (void *)0/*PCI_LAT_TIMER*/ } },
    
    { "autoIntRouteSet",HCF_RES_INT,    { (void *)TRUE } },
    
   /* { "includeFuncSet",  HCF_RES_ADDR,   { (void *sysPciAutoconfigInclude } },*/
  /*  { "intAssignFuncSet", HCF_RES_ADDR,  { (void *sysPciAutoconfigIntrAssign }},*/
    { "includeFuncSet",  HCF_RES_ADDR,   { (void *)NULL } },
    { "intAssignFuncSet", HCF_RES_ADDR,  { (void *)NULL }},
};
	
#define armPlAxiPci0Num NELEMENTS(armPlAxiPci0Resources)
#endif  /* #ifdef PL_PCIE_RC_VXB */



const struct hcfDevice hcfDeviceList[] = 
{
    { "armGicDev",  0, VXB_BUSID_PLB, 0, armGIC0Num,         armGIC0Resources },
    
	/* sio */
#ifdef DRV_SIO_NS16550    
    { "ns16550",    0, VXB_BUSID_PLB, 0, fmqlSioDev0Num,     altera16550Dev0Resources },
    
	#ifdef DRV_UART_1_OPEN
		{ "ns16550",	1, VXB_BUSID_PLB, 0, fmqlSioDev1Num,	 altera16550Dev1Resources },
	#endif
#endif

	/* ttc */
#ifdef INCLUDE_ALT_SOC_GEN5_TIMER
    { "altTimer",   0, VXB_BUSID_PLB, 0, fmqlTimerDev0Num,   alteraTimerDev0Resources },
    /*
    { "altTimer",   0, VXB_BUSID_PLB, 1, fmqlTimerDev1Num,   alteraTimerDev1Resources },
    { "altTimer",   0, VXB_BUSID_PLB, 2, fmqlTimerDev2Num,   alteraTimerDev2Resources },
	*/
   { "altTimer",   1, VXB_BUSID_PLB, 0, fmqlTimerDev3Num,   alteraTimerDev3Resources },
	/* 
    { "altTimer",   1, VXB_BUSID_PLB, 1, fmqlTimerDev4Num,   alteraTimerDev4Resources },
    { "altTimer",   1, VXB_BUSID_PLB, 2, fmqlTimerDev5Num,   alteraTimerDev5Resources },
	*/
#endif

/*
whole-function mode 
*/
#ifndef BUILD_VIP_MIN_SYSTEM

#ifdef INCLUDE_DRV_FM_GTC	
    { "gtcTimer",   0, VXB_BUSID_PLB, 0, gtcTimerDevNum,   gtcTimerDevResources },
#endif

	
	/* 
	gmac_0/1
	*/
#ifdef	GMAC_1_BARE_METAL
	/* do nothing*/
#else
#ifdef INCLUDE_ALT_SOC_GEN5_DW_END
#ifdef INCLUDE_GMAC_0 
	{ "gem",    0, VXB_BUSID_PLB, 0, vxbDwEmacDev0Num,   vxbDwEmacDev0Resources },
    { "genericPhy",   0, VXB_BUSID_PLB, 0, genPhyMdio0Num,     genPhyMdio0Resources },  
#endif  /* #ifdef INCLUDE_GMAC_0 */

#ifdef INCLUDE_GMAC_1
	{ "gem",	1, VXB_BUSID_PLB, 0, vxbDwEmacDev1Num,	 vxbDwEmacDev1Resources },
	{ "genericPhy",   1, VXB_BUSID_PLB, 0, genPhyMdio1Num,	   genPhyMdio1Resources },	
#endif  /* #ifdef INCLUDE_GMAC_1 */
#endif  /* #ifdef INCLUDE_ALT_SOC_GEN5_DW_END */
#endif

#ifdef INCLUDE_AXI_ETHERNET_END
{ "axienet",	0, VXB_BUSID_PLB, 0, vxbAxiEthDev0Num,	 vxbAxiEthDev0Resources },
{ "genericPhy",   2, VXB_BUSID_PLB, 0, genPhyAxiMdio0Num,	genPhyAxiMdio0Resources},	

//{ "axienet",	1, VXB_BUSID_PLB, 0, vxbAxiEthDev1Num,	 vxbAxiEthDev1Resources },
//{ "genericPhy",   3, VXB_BUSID_PLB, 0, genPhyAxiMdio1Num,	genPhyAxiMdio1Resources},

#endif

	/* I2C */
#ifdef INCLUDE_ALT_SOC_GEN5_DW_I2C
	{ "dwI2c", 0, VXB_BUSID_PLB, 0, dwI2cDev0Num, dwI2cDev0Resources },
	/*{ "dwI2c", 1, VXB_BUSID_PLB, 0, dwI2cDev1Num, dwI2cDev1Resources },*/
#endif /* INCLUDE_ALT_SOC_GEN5_DW_I2C */

	/* WDT */
#ifdef INCLUDE_ALT_SOC_GEN5_WDT
	{ "altWdt", 0, VXB_BUSID_PLB, 0, alteraWdtDev0Num, alteraWdtDev0Resources },
#endif /* INCLUDE_ALT_SOC_GEN5_WDT */

	/* 
	qspi 
	*/
#ifdef DRV_FM_QSPI
	{ "fmqspi", 0, VXB_BUSID_PLB, 0,	fmQspiNum,	fmQspiResources },
#endif

	/* 
	SD/MMC 
	*/
#ifdef DRV_FM_SDMMC
	#ifdef SDMMC_DOSFS_V2  /* ref_712: sd/emmc dosfs*/
		 /* ref_712 for dosFS*/
		 { "fmshSdhci", 0, VXB_BUSID_PLB, 0,	fmSdmmcNum_dosFS, 	fmSdmmcResources_dosFS },
		 
	#ifndef SD_CTRL1_AS_EMMC
	#ifdef FMSH_VERIFY_BOARD 
		 { "fmshSdhci", 1, VXB_BUSID_PLB, 0,	fmSdmmcNum2_dosFS,	fmSdmmcResources2_dosFS },
	#endif /* #ifdef FMSH_VERIFY_BOARD */
	#endif /* #ifndef SD_CTRL1_AS_EMMC */	
	#endif /* #ifdef SDMMC_DOSFS_V2    */
		
	#ifdef SDMMC_2_VX
		/* for sd / emmc */
		{ "fmSdmmc", 0, VXB_BUSID_PLB, 0,  fmSdmmcNum,	   fmSdmmcResources },	 /* vxbus, vx-nobus*/
	#if 1 /* def FMSH_VERIFY_BOARD*/
		{ "fmSdmmc", 1, VXB_BUSID_PLB, 0,  fmSdmmcNum2,    fmSdmmcResources2 },
	#endif
	#endif

	
	#ifdef SD_DOSFS_V1 /* def DRV_STORAGE_ALT_SOC_GEN5_DW_MSHC*/
		{ "altSocGen5DwMshc", 0, VXB_BUSID_PLB, 0,	alteraMshcNum,	alteraMshcResources },
	#endif /* DRV_STORAGE_ALT_SOC_GEN5_DW_MSHC */

#endif /* DRV_FM_SDMMC */

	/* 
	spi 
	*/
#ifdef DRV_FM_SPI
	{ "fmspi",      0, VXB_BUSID_PLB, 0, fmSpi0Num,	      fmSpi0Resources },
	{ "fmspi",      1, VXB_BUSID_PLB, 0, fmSpi1Num,	      fmSpi1Resources },
#endif

	/* 
	can 
	*/
#ifdef DRV_FM_CAN
	{ "fmcan",		0, VXB_BUSID_PLB, 0, fmCan0Num, 	  fmCan0Resources },
	{ "fmcan",		1, VXB_BUSID_PLB, 0, fmCan1Num, 	  fmCan1Resources },
#endif

	/* 
	nfc 
	*/
#ifdef DRV_FM_NFC
	{ "fmnfc",		0, VXB_BUSID_PLB, 0, fmNfcNum, 	  fmNfcResources },
#endif

	/* 
	NFC(nandflash) 
	*/
#ifdef DRV_FM_NFC
	/* { "fmshNfc",	  0, VXB_BUSID_PLB, 0, fmshNfc0Num,		  fmshNfc0Resources }, */
#endif  /* DRV_TI_AM335X_SPI */


	/* 
	USB host 
	*/
#ifdef INCLUDE_SYNOPSYSHCI
	{ "vxbPlbUsbSynopsysHci", 0, VXB_BUSID_PLB, 0, vxbPlbUsbSynopsysHciDev0Num, vxbPlbUsbSynopsysHciDev0Resources},
#endif /* INCLUDE_SYNOPSYSHCI */

#endif  /* #ifndef BUILD_VIP_MIN_SYSTEM */


#ifdef PL_PCIE_RC_VXB
    { "vxbArmPlAxiPci", 0, VXB_BUSID_PLB,0, armPlAxiPci0Num, armPlAxiPci0Resources },
#endif  /* #ifdef PL_PCIE_RC_VXB */

    { NULL, 0, 0, 0, 0, NULL }
};

const int hcfDeviceNum = NELEMENTS(hcfDeviceList);

VXB_INST_PARAM_OVERRIDE sysInstParamTable[] = 
{
#if 1
#ifdef INCLUDE_EHCI
    { "vxbPlbUsbEhci", 0, "hasEmbeddedTT", VXB_PARAM_INT32,   { (void *)TRUE } },
   /* { "vxbPlbUsbEhci", 0, "postResetHook", VXB_PARAM_FUNCPTR, { (void *zynq7kEhci0PostResetHook } },*/
#endif  /* INCLUDE_EHCI */
#endif
    { NULL,  0, NULL, VXB_PARAM_END_OF_LIST, {(void *)0} }
};


