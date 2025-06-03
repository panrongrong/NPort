/*
 * pcieAutoConfig.h
 *
 *  Created on: 2019-11-19
 *      Author: ljhua15
 */


#ifndef PCIEAUTOCONFIG_H_
#define PCIEAUTOCONFIG_H_

/*#include <sysTypes.h>*/

/*#include "pci/pci.h"*/
#include "pci.h"


void pciautoRegionInit(T_pcieRegion *res);

void pciAutoRegionAlign(T_pcieRegion *res, UINT32 size);

int pciAutoRegionAllocate(T_pcieRegion *res, UINT32 size,UINT32 *bar);

void pciautoSetupDevice(
		int nPEX,
		int busNo,      /* bus number */
	    int deviceNo,   /* device number */
	    int funcNo,     /* function number */
	    T_pcieHose *hose,
	    int bars_num,
	    T_pcieRegion *mem,
		T_pcieRegion *prefetch,
	    T_pcieRegion *io);

void pciAutoPrescanSetupBridge(
		int nPEX,
		int busNo,      /* bus number */
	    int deviceNo,   /* device number */
	    int funcNo,     /* function number */
        T_pcieHose *hose,
	    int sub_bus);

void pciAutoPostscanSetupBridge(
		int nPEX,
		int busNo,      /* bus number */
	    int deviceNo,   /* device number */
	    int funcNo,     /* function number */
        T_pcieHose *hose,
	    int subBus);

void pciAutoConfigInit(T_pcieHose *hose);

int pciAutoConfigDevice(
		int nPEX,
		int busNo,      /* bus number */
	    int deviceNo,   /* device number */
	    int funcNo,     /* function number */
        T_pcieHose *hose);

int pcieAutoCfgBridge(
		int nPEX,
		int busNo,      /* bus number */
	    int deviceNo,   /* device number */
	    int funcNo,     /* function number */
	    T_pcieHose *pcieHose);
void pciePrescanSetupBridge(
		int nPEX,
		int busNo,      /* bus number */
	    int deviceNo,   /* device number */
	    int funcNo,     /* function number */
	    int subBus,
	    T_pcieHose *pcieHose);
void pciePostscanSetupBridge(
		int nPEX,
		int busNo,      /* bus number */
	    int deviceNo,   /* device number */
	    int funcNo,     /* function number */
	    int subBus,
	    T_pcieHose *pcieHose);
#endif /* PCIEAUTOCONFIG_H_ */
