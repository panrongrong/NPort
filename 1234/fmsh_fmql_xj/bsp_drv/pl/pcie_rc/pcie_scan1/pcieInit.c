/*
 * pcieInit.c
 *
 *  Created on: 2019年9月19日
 *      Author: wliang
 */
/*#include <pci/platform.h>*/
/*#include <sysTypes.h>*/
#include "platform.h"

/*#include "pci/pci.h"*/
/*#include "pci/pciConfigLib.h"*/
/*#include "pci/pcieAutoConfig.h"*/
#include "pci.h"
#include "pciConfigLib.h"
#include "pcieAutoConfig.h"

extern T_pcieDevInfo pcieDevInfo[PCIE_DEV_INFO_MUX];
static T_pcieHose gPcieHose;
void setStartPict(void);
extern void initPcieDevInfo(void);
extern void pcieHoseScan(int nPEX,T_pcieHose *pcieHose);
extern STATUS pciFindDevice1(int vendorId,int deviceId,int index,int * pBusNo,int * pDeviceNo,int *pFuncNo);
extern STATUS pciConfigInLong1(int busNo,int deviceNo,int funcNo,int offset,UINT32 * pData);

#if 0  /* ref-ft*/
void pcieControllerInit(unsigned long address)
{
	UINT32 lower_base = 0, upper_base = 0, limit = 0;
	UINT32 lower_target = 0, upper_target = 0;

	/* Reqion 0 for CONFIG Type 0*/
	lower_base   = PCIE_CONFIG_BASE;
	upper_base   = 0;
	limit        = lower_base + PCIE_CONFIG_SIZE - 1;
	lower_target = 0;
	upper_target = 0;

	write_l(0x0         , address + PEU_PL_INDEX_OFF);
	write_l(lower_base  , address + PEU_PL_LOWER_BASE_OFF);
	write_l(upper_base  , address + PEU_PL_UPPER_BASE_OFF);
	write_l(limit       , address + PEU_PL_LIMIT_OFF);
	write_l(lower_target, address + PEU_PL_LOWER_TARGET_OFF);
	write_l(upper_target, address + PEU_PL_UPPER_TARGET_OFF);
	write_l(PEU_PL_REGION_CTRL_1_TYPE_CONFIG_TYPE_0
		               , address + PEU_PL_REGION_CTRL_1_OFF);
	write_l(0x90000000  , address + PEU_PL_REGION_CTRL_2_OFF);

	/* Reqion 1 for CONFIG Type 1*/
	lower_base   = PCIE_CONFIG_BASE;
	upper_base   = 2;
	limit        = lower_base + PCIE_CONFIG_SIZE - 1;
	lower_target = 0;
	upper_target = 0;
	write_l(0x1         , address + PEU_PL_INDEX_OFF);
	write_l(lower_base  , address + PEU_PL_LOWER_BASE_OFF);
	write_l(upper_base  , address + PEU_PL_UPPER_BASE_OFF);
	write_l(limit       , address + PEU_PL_LIMIT_OFF);
	write_l(lower_target, address + PEU_PL_LOWER_TARGET_OFF);
	write_l(upper_target, address + PEU_PL_UPPER_TARGET_OFF);
	write_l(PEU_PL_REGION_CTRL_1_TYPE_CONFIG_TYPE_1
			           , address + PEU_PL_REGION_CTRL_1_OFF);
	write_l(0x90000000  , address + PEU_PL_REGION_CTRL_2_OFF);

	/* Reqion 2 for IO*/
	lower_base   = PCIE_IO_BASE;
	upper_base   = 0;
	limit        = lower_base + PCIE_IO_SIZE - 1;
	lower_target = 0;
	upper_target = 0;
	write_l(0x2         , address + PEU_PL_INDEX_OFF);
	write_l(lower_base  , address + PEU_PL_LOWER_BASE_OFF);
	write_l(upper_base  , address + PEU_PL_UPPER_BASE_OFF);
	write_l(limit       , address + PEU_PL_LIMIT_OFF);
	write_l(lower_target, address + PEU_PL_LOWER_TARGET_OFF);
	write_l(upper_target, address + PEU_PL_UPPER_TARGET_OFF);
	write_l(PEU_PL_REGION_CTRL_1_TYPE_IO
			           , address + PEU_PL_REGION_CTRL_1_OFF);
	write_l(0x80000000  , address + PEU_PL_REGION_CTRL_2_OFF);

	/* Reqion 3 for MEM32*/
	lower_base   = PCIE_MEM32_BASE + 0x100000;
	upper_base   = 0;
	limit        = lower_base + PCIE_MEM32_SIZE - 1;
	lower_target = lower_base;
	upper_target = 0;
	write_l(0x3         , address + PEU_PL_INDEX_OFF);
	write_l(lower_base  , address + PEU_PL_LOWER_BASE_OFF);
	write_l(upper_base  , address + PEU_PL_UPPER_BASE_OFF);
	write_l(limit       , address + PEU_PL_LIMIT_OFF);
	write_l(lower_target, address + PEU_PL_LOWER_TARGET_OFF);
	write_l(upper_target, address + PEU_PL_UPPER_TARGET_OFF);
	write_l(PEU_PL_REGION_CTRL_1_TYPE_MEM
			           , address + PEU_PL_REGION_CTRL_1_OFF);
	write_l(0x80000000  , address + PEU_PL_REGION_CTRL_2_OFF);

	/* Reqion 4 for Legacy VGA*/
	lower_base   = PCIE_MEM32_BASE;
	upper_base   = 0;
	limit        = lower_base + 0xFFFFF;
	lower_target = 0;
	upper_target = 0;
	write_l(0x4         , address + PEU_PL_INDEX_OFF);
	write_l(lower_base  , address + PEU_PL_LOWER_BASE_OFF);
	write_l(upper_base  , address + PEU_PL_UPPER_BASE_OFF);
	write_l(limit       , address + PEU_PL_LIMIT_OFF);
	write_l(lower_target, address + PEU_PL_LOWER_TARGET_OFF);
	write_l(upper_target, address + PEU_PL_UPPER_TARGET_OFF);
	write_l(PEU_PL_REGION_CTRL_1_TYPE_MEM
			           , address + PEU_PL_REGION_CTRL_1_OFF);
	write_l(0x80000000  , address + PEU_PL_REGION_CTRL_2_OFF);

	/* Reqion 5 for MEM64*/
	lower_base   = PCIE_MEM64_BASE && 0xffffffff;
	upper_base   = PCIE_MEM64_BASE >> 32;
	limit        = lower_base + PCIE_MEM64_SIZE - 1;
	lower_target = lower_base;
	upper_target = upper_base;
	write_l(0x5         , address + PEU_PL_INDEX_OFF);
	write_l(lower_base  , address + PEU_PL_LOWER_BASE_OFF);
	write_l(upper_base  , address + PEU_PL_UPPER_BASE_OFF);
	write_l(limit       , address + PEU_PL_LIMIT_OFF);
	write_l(lower_target, address + PEU_PL_LOWER_TARGET_OFF);
	write_l(upper_target, address + PEU_PL_UPPER_TARGET_OFF);
	write_l(PEU_PL_REGION_CTRL_1_TYPE_MEM
			           , address + PEU_PL_REGION_CTRL_1_OFF);
	write_l(0x80000000  , address + PEU_PL_REGION_CTRL_2_OFF);

	/* Enable 64bit Prefetchable Memory Space and 32bit IO Space*/
	write_l(0x1    , address + PEU_PL_MISC_CTRL_1_OFF);
	write_w(0x101  , address + PEU_IO_LIMIT_BASE_REG);
	write_l(0x10001, address + PEU_PREF_LIMIT_BASE_REG);
	write_l(0x0    , address + PEU_PL_MISC_CTRL_1_OFF);
}

#else  /* for fmql7045*/

void pcieControllerInit(unsigned long address)
{
	int Status;
	
	extern int pl_pcie_rc_init_2(void);	
	
	Status = pl_pcie_rc_init_2();
	if (Status != 0) 
	{
		printf("Failed to initialize AXI PCIe Root port\r\n");
	}

	return;
}

#endif



#if 0

/*ft2000 两个pcie控制器当成一个使用 空间配置，链路训练只用一套*/
void pcieInit(void)
{
    T_pcieHose *ppcieHose;


    /*init pcie device information struct*/
    initPcieDevInfo();

    /*init pcie handler*/
	ppcieHose = &gPcieHose;
	
	ppcieHose->currentBusno = 0;
	ppcieHose->firstBusno = 0;
	ppcieHose->lastBusno = 0xff;

	ppcieHose->pcieMEM.busStart = PCIE_MEM32_BASE + 0x100000;
	ppcieHose->pcieMEM.size = PCIE_MEM32_SIZE - 0x100000;

	ppcieHose->pcieIO.busStart = 0x1000;
	ppcieHose->pcieIO.size = PCIE_IO_SIZE - 0x1000;

	/*FT2000 预取空间在64位空间，天脉暂时不支持64位*/
#ifdef	CONFIG_SYS_PCI_64BIT
	ppcieHose->pciePREF.busStart = PCIE_MEM64_BASE;
	ppcieHose->pciePREF.size = PCIE_MEM64_SIZE;
#else
	ppcieHose->pciePREF.busStart = 0;
	ppcieHose->pciePREF.size = 0;
#endif

    /*
	 * Initialize Root Complex
	 */
	switch(read_l(PCIE_SPLIT_MODE)) {
	case PEU_SPLIT_MODE_X8:
		printk("%s %d \n",__func__,__LINE__);
		pcieControllerInit(PCIE_C0_BASE);
		break;
	case PEU_SPLIT_MODE_X4_X4:
		printk("%s %d \n",__func__,__LINE__);
		pcieControllerInit(PCIE_C0_BASE);
		pcieControllerInit(PCIE_C1_BASE);
		break;
	default:
		break;
	}

	/*
	 * Enable INTx Interrupt
	 */
	write_l(0xf, PCIE_INTX_ENABLE);

	/*
	 * Enable MSI Interrupt
	 */
	write_l(0x1, PCIE_MSI_ENABLE);
	write_l(GICD_BASE+0x10000, PCIE_MSI_BASE32);
	write_l(GICD_BASE+0x10000, PCIE_MSI_BASE64_LOW);
	write_l(0x0, PCIE_MSI_BASE64_HIGH);
	printk("%s %d \n",__func__,__LINE__);

	/*
	 * Enable Non-Secure Access to PEU C0/C1
	 */
	write_l(0xf, PCIE_ACCESS_TYPE);

	/*
	 * Enable Link Negotiation
	 */
#if 1
	/*writel(0x0, CRU_PEU_LINK_DISABLE);*/

	printk("PCIE_C0_LTSSM:%x\n",*(UINT32*)PCIE_C0_LTSSM);
	printk("PCIE_C0_LTSSM_STATUS:%x\n",PCIE_C0_LTSSM_STATUS);
	printk("PCIE_C0_LTSSM_SMLH_LINK:%x\n",PCIE_C0_LTSSM_SMLH_LINK);
	printk("PCIE_C0_LTSSM_RDLH_LINK:%x\n",PCIE_C0_LTSSM_RDLH_LINK);

	printk("PCIE_C1_LTSSM:%x\n",*(UINT32*)PCIE_C1_LTSSM);
	printk("PCIE_C1_LTSSM_STATUS:%x\n",PCIE_C1_LTSSM_STATUS);
	printk("PCIE_C1_LTSSM_SMLH_LINK:%x\n",PCIE_C1_LTSSM_SMLH_LINK);
	printk("PCIE_C1_LTSSM_RDLH_LINK:%x\n",PCIE_C1_LTSSM_RDLH_LINK);

	if((!PCIE_C0_LTSSM_SMLH_LINK) ||(!PCIE_C0_LTSSM_RDLH_LINK))
		printk("LTSSM0 link down!\n");

	if((!PCIE_C1_LTSSM_SMLH_LINK) ||(!PCIE_C1_LTSSM_RDLH_LINK))
		printk("LTSSM1 link down!\n");

	/*scan bus and config bridge*/
	pcieHoseScan(0,ppcieHose);

	#if CONFIG_PCI_SCAN_SHOW
	showPciDevice();
	#endif
#endif
}

#else

void pcieInit(void)
{
    T_pcieHose *ppcieHose;

    /*init pcie device information struct*/
    initPcieDevInfo();
	
	ppcieHose = &gPcieHose;
	
	ppcieHose->currentBusno = 0;
	ppcieHose->firstBusno = 0;
	ppcieHose->lastBusno = 0xff;

	ppcieHose->pcieMEM.busStart = PCIE_MEM32_BASE + 0x100000;
	ppcieHose->pcieMEM.size     = PCIE_MEM32_SIZE - 0x100000;

	ppcieHose->pcieIO.busStart = 0x1000;
	ppcieHose->pcieIO.size     = PCIE_IO_SIZE - 0x1000;

	ppcieHose->pciePREF.busStart = 0;
	ppcieHose->pciePREF.size     = 0;

	/*
	 * Initialize Root Complex
	 */
	pcieControllerInit(PCIE_C0_BASE);

	/*scan bus and config bridge*/
	pcieHoseScan(0, ppcieHose);

#if CONFIG_PCI_SCAN_SHOW
extern void showPciDevice(void);
	showPciDevice();
#endif

	return;
}

#endif


/*****************************************************************************
函数原型：static UINT32 getPcieLinkStatus(UINT8 pcieControlID)
函数功能：获取链路训练状态
参数：pcieControlID ID
返回值：
	0
*****************************************************************************/
UINT32 getPcieLinkStatus(UINT8 pcieControlID)
{
	UINT32 value;
    if(pcieControlID==0)
    {
    	value=*(UINT32*)PCIE_C0_LTSSM;
    }else
    {
    	value=*(UINT32*)PCIE_C1_LTSSM;
    }
     return value;
}



#if 1

/* command excute and log*/
/*
->pcieInit

=============================
PCIe Gen2 Enumeration Example
=============================
XAxiPcie_LookupConfig end 
XAxiPcie_CfgInitialize -1: base-0x50000000  
XAxiPcie_DisableInterrupts -1: base-0x50000000  
XAxiPcie_DisableInterrupts -2  
XAxiPcie_DisableInterrupts -3  
XAxiPcie_CfgInitialize -2: base-0x50000000  
XAxiPcie_CfgInitialize -3: Data-0x00010002 MaxNumOfBuses=1 
XAxiPcie_CfgInitialize end 
Interrupts currently enabled are        0
XAxiPcie_DisableInterrupts -1: base-0x50000000  
XAxiPcie_DisableInterrupts -2  
XAxiPcie_DisableInterrupts -3  
Interrupts currently pending are        0
Interrupts currently enabled are        0
Interrupts currently pending are        0
BarAddr------- 0x00000000(high) 0x60000000(low) -------- 
XAxiPcie_IsLinkUp end 
Link:
  - LINK UP, Gen1 - X1 lanes 

Requester ID:
  - Bus Number     : 00
  - Device Number  : 00
  - Function Number: 00
  - Port Number    : 00
PCIe Local Config Space:
  - 0x00100147 at register CommandStatus
  - 0x00070100 at register Prim Sec. Bus
Root Complex IP Instance has been successfully initialized
bus:0 dev:0 fun:0 VID:10ee DID:7112
bus:1 dev:0 fun:0 VID:10b5 DID:8112
bus:2 dev:15 fun:0 VID:1057 DID:4
bus:0 dev:0 fun:0 VID:10ee DID:7112
bus:1 dev:0 fun:0 VID:10b5 DID:8112
bus:2 dev:15 fun:0 VID:1057 DID:4
value = 0 = 0x0


-> pcie_show

[0,0,0] - (10ee 7112) type=P2P BRIDGE to [1,0,0]
        base/limit:
          mem=   0x60100000/0x601fffff
          preMem=0x00000000/0x000fffff
          I/O=   0x00000000/0x00001fff
        status=0x0010 ( CAP DEVSEL=0 )
        command=0x0147 ( IO_ENABLE MEM_ENABLE MASTER_ENABLE PERR_ENABLE SERR_ENABLE )
[1,0,0] - (10b5 8112) type=P2P BRIDGE to [2,0,0]
        base/limit:
          mem=   0x60100000/0x601fffff
          preMem=0x10000000/0x000fffff
          I/O=   0x2000/0x1fff
        status=0x0010 ( CAP DEVSEL=0 )
        command=0x0007 ( IO_ENABLE MEM_ENABLE MASTER_ENABLE )
        bar0 in prefetchable 64-bit mem space @ 0xffff0000
        bar1 in 32-bit mem space @ 0x00001000
[2,15,0] - (1057 0004) type=OTHER DEVICE
        status=0x02a0 ( 66MHZ FBTB DEVSEL=1 )
        command=0x0006 ( MEM_ENABLE MASTER_ENABLE )
        bar0 in 32-bit mem space @ 0x60100000
        bar1 in prefetchable 32-bit mem space @ 0x60102000
value = 4300628 = 0x419f54 = 'T' = g_pcie_show_flag
-> 
-> 
-> showPciDevice

bus:0 dev:0 fun:0 VID:10ee DID:7112
bus:1 dev:0 fun:0 VID:10b5 DID:8112
bus:2 dev:15 fun:0 VID:1057 DID:4
value = 0 = 0x0
->  
*/
#endif


