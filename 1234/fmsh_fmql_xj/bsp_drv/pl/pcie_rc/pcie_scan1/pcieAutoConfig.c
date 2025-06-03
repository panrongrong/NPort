/*
 * pcieAutoConfig.c
 *
 *  Created on: 2019Äê10ÔÂ22ÈÕ
 *      Author: zgxue
 */

/*#include <sysTypes.h>*/

/*#include "pci/pci.h"*/
/*#include "pci/pciConfigLib.h"*/
/*#include "pci/pcieAutoConfig.h"*/
#include "pci.h"
#include "pciConfigLib.h"
#include "pcieAutoConfig.h"

void pciautoRegionInit(T_pcieRegion *res)
{
	/*
	 * Avoid allocating PCI resources from address 0 -- this is illegal
	 * according to PCI 2.1 and moreover, this is known to cause Linux IDE
	 * drivers to fail. Use a reasonable starting value of 0x1000 instead.
	 */
	res->busLower = res->busStart ? res->busStart : 0x1000;
}

void pciAutoRegionAlign(T_pcieRegion *res, UINT32 size)
{
	res->busLower = ((res->busLower - 1) | (size - 1)) + 1;
}

int pciAutoRegionAllocate(T_pcieRegion *res, UINT32 size,UINT32 *bar)
{
	UINT32 addr;

	if (!res) {
		goto error;
	}

	addr = ((res->busLower - 1) | (size - 1)) + 1;

	if (addr - res->busStart + size > res->size) {
		goto error;
	}

	res->busLower = addr + size;

	*bar = addr;
	return 0;

 error:
	*bar = (UINT32)-1;
	return -1;
}

void pciautoSetupDevice(
		int nPEX,
		int busNo,      /* bus number */
	    int deviceNo,   /* device number */
	    int funcNo,     /* function number */
	    T_pcieHose *hose,
	    int bars_num,
	    T_pcieRegion *mem,
		T_pcieRegion *prefetch,
	    T_pcieRegion *io)
{
	UINT32 bar_response;
	UINT32 bar_size;
	UINT16 cmdstat = 0;
	int bar;

	UINT32 bar_value;
	T_pcieRegion *bar_res;
	int found_mem64 = 0;


	pcieConfigInWord1(nPEX, busNo,deviceNo,funcNo,PCI_COMMAND, &cmdstat);
	cmdstat = (cmdstat & ~(PCI_COMMAND_IO | PCI_COMMAND_MEMORY)) | PCI_COMMAND_MASTER;

	for (bar = PCI_BASE_ADDRESS_0; bar < PCI_BASE_ADDRESS_0 + (bars_num * 4); bar += 4) 
	{
		/* Tickle the BAR and get the response */
		pcieConfigOutLong1(nPEX,busNo,deviceNo,funcNo,bar,0xffffffff);

		pcieConfigInLong1(nPEX, busNo,deviceNo,funcNo,bar, &bar_response);
		
		/* If BAR is not implemented go to the next BAR */
		if (!bar_response)
			continue;

		found_mem64 = 0;

		/* Check the BAR type and set our address mask */
		if (bar_response & PCI_BASE_ADDRESS_SPACE)
		{
			bar_size = ((~(bar_response & PCI_BASE_ADDRESS_IO_MASK)) & 0xffff) + 1;
			bar_res = io;
		} 
		else 
		{
			if ((bar_response & PCI_BASE_ADDRESS_MEM_TYPE_MASK) == PCI_BASE_ADDRESS_MEM_TYPE_64) 
			{
				UINT32 bar_response_upper;
				UINT64 bar64;

				pcieConfigOutLong1(nPEX,busNo,deviceNo,funcNo,bar + 4,0xffffffff);
				pcieConfigInLong1(nPEX, busNo,deviceNo,funcNo, bar + 4, &bar_response_upper);
				bar64 = ((UINT64)bar_response_upper << 32) | bar_response;

				bar_size = ~(bar64 & PCI_BASE_ADDRESS_MEM_MASK) + 1;

				found_mem64 = 1;
			} 
			else 
			{
				bar_size = (UINT32)(~(bar_response & PCI_BASE_ADDRESS_MEM_MASK) + 1);
			}

			if (prefetch && (bar_response & PCI_BASE_ADDRESS_MEM_PREFETCH)) 
			{
				if (found_mem64) 
				{
					bar_res = prefetch;
				} 
				else 
				{
					bar_res = mem;
				}
			}
			else
			{	
				bar_res = mem;
			}
		}


		if (pciAutoRegionAllocate(bar_res, bar_size, &bar_value) == 0) 
		{
			/* Write it out and update our limit */
			pcieConfigOutLong1(nPEX,busNo,deviceNo,funcNo,bar,(UINT32)bar_value);
			if (found_mem64) {
				bar += 4;
#ifdef CONFIG_SYS_PCI_64BIT
				pcieConfigOutLong1(nPEX,busNo,deviceNo,funcNo,bar,(UINT32)(bar_value>>32));
#else
				/*
				 * If we are a 64-bit decoder then increment to the
				 * upper 32 bits of the bar and force it to locate
				 * in the lower 4GB of memory.
				 */
				pcieConfigOutLong1(nPEX,busNo,deviceNo,funcNo,bar,0x00000000);
#endif
			}
		}

		cmdstat |= (bar_response & PCI_BASE_ADDRESS_SPACE) ? PCI_COMMAND_IO : PCI_COMMAND_MEMORY;
	}/*cfg bar*/

	pcieConfigOutWord1(nPEX,busNo,deviceNo,funcNo,PCI_COMMAND,cmdstat);

	pcieConfigOutByte1(nPEX,busNo,deviceNo,funcNo,PCI_CACHE_LINE_SIZE,CONFIG_SYS_PCI_CACHE_LINE_SIZE);
	pcieConfigOutByte1(nPEX,busNo,deviceNo,funcNo,PCI_LATENCY_TIMER,0x80);
}

void pciAutoPrescanSetupBridge(
		int nPEX,
		int busNo,      /* bus number */
	    int deviceNo,   /* device number */
	    int funcNo,     /* function number */
        T_pcieHose *hose,
	    int sub_bus)
{
	T_pcieRegion *pci_mem;
	T_pcieRegion *pci_prefetch;
	T_pcieRegion *pci_io;
	UINT16 cmdstat, prefechable_64;

	pci_mem      = &hose->pcieMEM;
	pci_prefetch = &hose->pciePREF;
	pci_io       = &hose->pcieIO;

	pcieConfigInWord1(nPEX, busNo,deviceNo,funcNo,PCI_COMMAND, &cmdstat);
	pcieConfigInWord1(nPEX, busNo,deviceNo,funcNo,PCI_PREF_MEMORY_BASE, &prefechable_64);
	prefechable_64 &= 0x1;

	/* Configure bus number registers */
	pcieConfigOutByte1(nPEX,busNo,deviceNo,funcNo,PCI_PRIMARY_BUS,busNo - hose->firstBusno);

	pcieConfigOutByte1(nPEX,busNo,deviceNo,funcNo,PCI_SECONDARY_BUS,sub_bus - hose->firstBusno);

	pcieConfigOutByte1(nPEX,busNo,deviceNo,funcNo,PCI_SUBORDINATE_BUS,0xff);

	if ((pci_mem->busStart != 0) || (pci_mem->size     != 0)) 
	{
		/* Round memory allocator to 1MB boundary */
		pciAutoRegionAlign(pci_mem, 0x100000);

		/* Set up memory and I/O filter limits, assume 32-bit I/O space */
		pcieConfigOutWord1(nPEX,busNo,deviceNo,funcNo,PCI_MEMORY_BASE,(pci_mem->busLower & 0xfff00000) >> 16);

		cmdstat |= PCI_COMMAND_MEMORY;
	}

	if ((pci_prefetch->busStart != 0) || (pci_prefetch->size     != 0))
	{
		/* Round memory allocator to 1MB boundary */
		pciAutoRegionAlign(pci_prefetch, 0x100000);

		/* Set up memory and I/O filter limits, assume 32-bit I/O space */
		pcieConfigOutWord1(nPEX,busNo,deviceNo,funcNo,PCI_PREF_MEMORY_BASE,(pci_prefetch->busLower & 0xfff00000) >> 16);

		if (prefechable_64)
		{
#ifdef CONFIG_SYS_PCI_64BIT
			pcieConfigOutLong1(nPEX,busNo,deviceNo,funcNo,PCI_PREF_BASE_UPPER32,pci_prefetch->bus_lower >> 32);
#else
			pcieConfigOutLong1(nPEX,busNo,deviceNo,funcNo,PCI_PREF_BASE_UPPER32,0);
#endif
		}
		cmdstat |= PCI_COMMAND_MEMORY;
	} 
	else 
	{
		/* We don't support prefetchable memory for now, so disable */
		pcieConfigOutWord1(nPEX,busNo,deviceNo,funcNo,PCI_PREF_MEMORY_BASE,0x1000);
		pcieConfigOutWord1(nPEX,busNo,deviceNo,funcNo,PCI_PREF_MEMORY_LIMIT,0x0);
		if (prefechable_64) {
			pcieConfigOutWord1(nPEX,busNo,deviceNo,funcNo,PCI_PREF_BASE_UPPER32,0);
			pcieConfigOutWord1(nPEX,busNo,deviceNo,funcNo,PCI_PREF_LIMIT_UPPER32,0);
		}
	}

	if ((pci_io->busStart != 0) || (pci_io->size != 0)) 
	{
		/* Round I/O allocator to 4KB boundary */
		pciAutoRegionAlign(pci_io, 0x1000);

		pcieConfigOutByte1(nPEX,busNo,deviceNo,funcNo,PCI_IO_BASE,(pci_io->busLower & 0x0000f000) >> 8);
		pcieConfigOutWord1(nPEX,busNo,deviceNo,funcNo,PCI_IO_BASE_UPPER16,(pci_io->busLower & 0xffff0000) >> 16);

		cmdstat |= PCI_COMMAND_IO;
	}

	/* Enable memory and I/O accesses, enable bus master */
	pcieConfigOutWord1(nPEX,busNo,deviceNo,funcNo,PCI_COMMAND,cmdstat | PCI_COMMAND_MASTER);
	return;
}

void pciAutoPostscanSetupBridge(
		int nPEX,
		int busNo,      /* bus number */
	    int deviceNo,   /* device number */
	    int funcNo,     /* function number */
        T_pcieHose *hose,
	    int subBus)
{
	T_pcieRegion *pci_mem;
	T_pcieRegion *pci_prefetch;
	T_pcieRegion *pci_io;

	pci_mem      = &hose->pcieMEM;
	pci_prefetch = &hose->pciePREF;
	pci_io       = &hose->pcieIO;

	/* Configure bus number registers */
	pcieConfigOutByte1(nPEX,busNo,deviceNo,funcNo,PCI_SUBORDINATE_BUS, subBus - hose->firstBusno);


	 if((pci_mem->busStart != 0)
	 || (pci_mem->size     != 0)) {
		/* Round memory allocator to 1MB boundary */
		pciAutoRegionAlign(pci_mem, 0x100000);
		pcieConfigOutWord1(nPEX,busNo,deviceNo,funcNo,PCI_MEMORY_LIMIT,(pci_mem->busLower - 1) >> 16);
	}

	if ((pci_prefetch->busStart != 0)
	 || (pci_prefetch->size     != 0)) {
		UINT16 prefechable_64;

		pcieConfigInWord1(nPEX, busNo,deviceNo,funcNo,PCI_PREF_MEMORY_LIMIT, &prefechable_64);

		prefechable_64 &= 0x1;

		/* Round memory allocator to 1MB boundary */
		pciAutoRegionAlign(pci_prefetch, 0x100000);

		pcieConfigOutWord1(nPEX,busNo,deviceNo,funcNo,PCI_PREF_MEMORY_LIMIT,(pci_prefetch->busLower - 1) >> 16);

		if (prefechable_64){
#ifdef CONFIG_SYS_PCI_64BIT
			pcieConfigOutLong1(nPEX,busNo,deviceNo,funcNo,PCI_PREF_LIMIT_UPPER32,(pci_prefetch->bus_lower - 1) >> 32);
#else
			pcieConfigOutLong1(nPEX,busNo,deviceNo,funcNo,PCI_PREF_LIMIT_UPPER32,0);
#endif
		}
	}


	if((pci_io->busStart != 0)
	|| (pci_io->size     != 0)) {
		/* Round I/O allocator to 4KB boundary */
		pciAutoRegionAlign(pci_io, 0x1000);

		pcieConfigOutByte1(nPEX,busNo,deviceNo,funcNo,PCI_IO_LIMIT, ((pci_io->busLower - 1) & 0x0000f000) >> 8);

		pcieConfigOutWord1(nPEX,busNo,deviceNo,funcNo,PCI_IO_LIMIT_UPPER16,((pci_io->busLower - 1) & 0xffff0000) >> 16);
	}
}

void pciAutoConfigInit(T_pcieHose *hose)
{

	if((hose->pcieMEM.busStart != 0)
	|| (hose->pcieMEM.size     != 0)) {
		pciautoRegionInit(&hose->pcieMEM);
	}

	if((hose->pciePREF.busStart != 0)
	|| (hose->pciePREF.size    != 0)) {
		pciautoRegionInit(&hose->pciePREF);
	}

	if((hose->pcieIO.busStart != 0)
	|| (hose->pcieIO.size     != 0)) {
		pciautoRegionInit(&hose->pcieIO);
	}
}

/*
 * HJF: Changed this to return int. I think this is required
 * to get the correct result when scanning bridges
 */
int pciAutoConfigDevice(
		int nPEX,
		int busNo,      /* bus number */
	    int deviceNo,   /* device number */
	    int funcNo,     /* function number */
        T_pcieHose *hose)
{
	UINT32 subBus = busNo;
	UINT16 class;
	int num=0;

	pcieConfigInWord1(nPEX, busNo,deviceNo,funcNo,PCI_CLASS_DEVICE, &class);

	switch (class) 
	{
	case PCI_CLASS_BRIDGE_PCI:
		hose->currentBusno++;
		pciautoSetupDevice(nPEX,busNo,deviceNo,funcNo,hose,2,&hose->pcieMEM, &hose->pciePREF, &hose->pcieIO);


		/* Passing in current_busno allows for sibling P2P bridges */
		pciAutoPrescanSetupBridge(nPEX,busNo,deviceNo,funcNo,hose, hose->currentBusno);
		/*
		 * need to figure out if this is a subordinate bridge on the bus
		 * to be able to properly set the pri/sec/sub bridge registers.
		 */
		num = pcieHoseScanBus(nPEX,busNo,deviceNo,funcNo,hose);

		/* figure out the deepest we've gone for this leg */
		subBus = num > subBus ? num : subBus;
		pciAutoPostscanSetupBridge(nPEX,busNo,deviceNo,funcNo,hose, subBus);

		subBus = hose->currentBusno;
		break;


	case PCI_CLASS_BRIDGE_CARDBUS:
		/*
		 * just do a minimal setup of the bridge,
		 * let the OS take care of the rest
		 */
		pciautoSetupDevice(nPEX,busNo,deviceNo,funcNo,hose, 0, &hose->pcieMEM,
			&hose->pciePREF, &hose->pcieIO);

		hose->currentBusno++;
		break;

	default:
		pciautoSetupDevice(nPEX,busNo,deviceNo,funcNo,hose, 6, &hose->pcieMEM,
			&hose->pciePREF, &hose->pcieIO);
		break;
	}

	return subBus;
}

/*config bridge */
int pcieAutoCfgBridge(
		int nPEX,
		int busNo,      /* bus number */
	    int deviceNo,   /* device number */
	    int funcNo,     /* function number */
	    T_pcieHose *pcieHose)
{
	int subBus = 0;
	UINT16 class = 0;
	int num=0;

	subBus = busNo;

	pcieConfigInWord1(nPEX, busNo, deviceNo, funcNo
			        ,PCI_CLASS_DEVICE, &class);

	if (class == PCI_CLASS_BRIDGE_PCI) {
		pcieHose->currentBusno++;

		/* Passing in current_busno allows for sibling P2P bridges */
		pciePrescanSetupBridge(nPEX,busNo,deviceNo,funcNo,pcieHose->currentBusno,pcieHose);
		/*
		 * need to figure out if this is a subordinate bridge on the bus
		 * to be able to properly set the pri/sec/sub bridge registers.
		 */
		num = pcieHoseScanBus(nPEX,busNo,deviceNo,funcNo,pcieHose);

		/* figure out the deepest we've gone for this leg */
		subBus = num > subBus ? num : subBus;

		pciePostscanSetupBridge(nPEX,busNo,deviceNo,funcNo,subBus,pcieHose);
		subBus = pcieHose->currentBusno;
	}
	return subBus;
}

/*prescan(down scan) setup bridge*/
void pciePrescanSetupBridge(
		int nPEX,
		int busNo,      /* bus number */
	    int deviceNo,   /* device number */
	    int funcNo,     /* function number */
	    int subBus,
	    T_pcieHose *pcieHose)
{

	/* Configure bus number registers */
	pcieConfigOutByte1(nPEX, busNo, deviceNo, funcNo
			         ,PCI_PRIMARY_BUS    , busNo  - pcieHose->firstBusno);
	pcieConfigOutByte1(nPEX, busNo, deviceNo, funcNo
			         ,PCI_SECONDARY_BUS  , subBus - pcieHose->firstBusno);
	pcieConfigOutByte1(nPEX, busNo, deviceNo, funcNo
			         ,PCI_SUBORDINATE_BUS, 0xff);

	pcieConfigOutByte1(nPEX, busNo, deviceNo, funcNo
			         ,PCI_COMMAND
			         ,PCI_COMMAND_MEMORY|
		              PCI_COMMAND_MASTER|
		              PCI_COMMAND_IO);
}

/*postscan(up scan) setup bridge*/
void pciePostscanSetupBridge(
		int nPEX,
		int busNo,      /* bus number */
	    int deviceNo,   /* device number */
	    int funcNo,     /* function number */
	    int subBus,
	    T_pcieHose *pcieHose)
{
	/* Configure bus number registers */
	pcieConfigOutByte1(nPEX, busNo, deviceNo, funcNo
			         ,PCI_SUBORDINATE_BUS, subBus - pcieHose->firstBusno);
}


