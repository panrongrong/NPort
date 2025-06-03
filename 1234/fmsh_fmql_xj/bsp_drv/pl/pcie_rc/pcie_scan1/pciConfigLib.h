/****************************************************************
* 版权所有(c) 2009. 中国航空工业第六三一研究所
*
* 该源码在AS-IS基础上可通过使用ACoreOS操作系统得到应用。允许在 
* AVIC ACTRI.版权下的任何人以任何方式应用该代码，可以复制、修改、
* 编译，并将其原样或修改后加入到其他代码中。任何人引用该代码，或据
* 此生成其他代码必须包含AVIC ACTRI.版权声明。
*
* 保留所有权利
*
* Copyright(c) 2009. AVIC ACTRI.
*
* This source code has been made available to you by ACoreOS on
* an AS-IS basis. Anyone receiving this source is licensed under 
* AVIC ACTRI copyrights to use it in any way he or she deems fit, 
* including copying it, modifying it, compiling it, and redistributing
* it either with or without modifications.Any person who transfer this
* source code or any derivative work must include this AVIC ACTRI 
* copyright notice.
*
* All Rithts Reserved.
*
*****************************************************************
* 文件名称: XXX
* 摘    要: 
* 当前版本: X.X.X
* 作    者: XXX
* 完成日期: XXXX年X月X日
*/



/*
  修    改    历    史
  --------------------
  修改日期     修改者    版本号  更改描述
  
  20XX-XX-XX   XXXXXX    1.X     详细描述    
  
*/
/****************************************************************
* 版权所有(c) 2009. 中国航空工业第六三一研究所
*
* 该源码在AS-IS基础上可通过使用ACoreOS操作系统得到应用。允许在
* AVIC ACTRI.版权下的任何人以任何方式应用该代码，可以复制、修改、
* 编译，并将其原样或修改后加入到其他代码中。任何人引用该代码，或据
* 此生成其他代码必须包含AVIC ACTRI.版权声明。
*
* 保留所有权利
*
* Copyright(c) 2009. AVIC ACTRI.
*
* This source code has been made available to you by ACoreOS on
* an AS-IS basis. Anyone receiving this source is licensed under
* AVIC ACTRI copyrights to use it in any way he or she deems fit,
* including copying it, modifying it, compiling it, and redistributing
* it either with or without modifications.Any person who transfer this
* source code or any derivative work must include this AVIC ACTRI
* copyright notice.
*
* All Rithts Reserved.
*
*****************************************************************
* 文件名称: XXX
* 摘    要:
* 当前版本: X.X.X
* 作    者: XXX
* 完成日期: XXXX年X月X日
*/



/*
  修    改    历    史
  --------------------
  修改日期     修改者    版本号  更改描述

  20XX-XX-XX   XXXXXX    1.X     详细描述

*/

#ifndef __pciConfigLibh
#define __pciConfigLibh


#ifdef __cplusplus
extern "C" {
#endif


/*#include "sysTypes.h"*/
#include "vxworks.h"

#define CONFIG_PCI_SCAN_SHOW 1

#define PCIE_DEV_INFO_MUX 255


#define LSB(x)    ((x) & 0xff)             /* 取低字节宏 */
#define MSB(x)    (((x) >> 8) & 0xff)      /* 取高字节宏 */
#define LSW(x)    ((x) & 0xffff)           /* 取低字宏 */
#define MSW(x)    (((x) >> 16) & 0xffff)   /* 取高字宏 */

/* 32位字按字倒序宏*/
#define WORDSWAP(x) (MSW(x) | (LSW(x) << 16))


#ifndef PCI_MAX_BUS
#  define PCI_MAX_BUS	32
#endif  /* PCI_MAX_BUS */

#ifndef PCI_MAX_DEV
#  define PCI_MAX_DEV	32
#endif  /* PCI_MAX_DEV */

#ifndef PCI_MAX_FUNC
#define PCI_MAX_FUNC	8
#endif  /* PCI_MAX_FUNC */

typedef struct {
	UINT32 busStart;	/* Start on the bus */
	UINT32 size;	/* Size */
	UINT32 busLower;
}T_pcieRegion;

typedef struct{
	int firstBusno;
	int lastBusno;
	int currentBusno;
	T_pcieRegion pcieMEM;
	T_pcieRegion pcieIO ;
	T_pcieRegion pciePREF;
}T_pcieHose;

typedef struct{
	int vendorID;
	int deviceID;
	int busNo;
	int deviceNo;
	int funcNo;
} T_pcieDevInfo;
typedef struct{
	int pcieDevCount;
	T_pcieDevInfo pcieDevInfo[PCIE_DEV_INFO_MUX];
} T_pcieDev;

extern STATUS pcieConfigInByte1(
	    int nPEX,
	    int    busNo,    /* bus number */
	    int    deviceNo, /* device number */
	    int    funcNo,      /* function number */
	    int    offset,      /* offset into the configuration space */
	    UINT8 * pData /* data read from the offset */
	    );

STATUS pcieConfigInWord1(
    int    nPEX,
    int    busNo,      /* bus number */
    int    deviceNo,   /* device number */
    int    funcNo,     /* function number */
    int    offset,     /* offset into the configuration space */
    UINT16 * pData  /* data read from the offset */
    );

STATUS pcieConfigInLong1(
    int    nPEX,
    int    busNo,     /* bus number */
    int    deviceNo,  /* device number */
    int    funcNo,    /* function number */
    int    offset,    /* offset into the configuration space */
    UINT32 * pData /* data read from the offset */
    );

STATUS pcieConfigOutByte1(
		int nPEX,
		int busNo,    /* bus number */
        int deviceNo, /* device number */
        int funcNo,   /* function number */
        int offset,   /* offset into the configuration space */
         UINT8 data   /* data written to the offset */
	);

STATUS pcieConfigOutWord1(
    int    nPEX,
    int    busNo,    /* bus number */
    int    deviceNo, /* device number */
    int    funcNo,   /* function number */
    int    offset,   /* offset into the configuration space */
    UINT16 data   /* data written to the offset */
    );

STATUS pcieConfigOutLong1(
    int nPEX,
    int	busNo,    /* bus number */
    int	deviceNo, /* device number */
    int	funcNo,   /* function number */
    int	offset,   /* offset into the configuration space */
    UINT32 data   /* data written to the offset */
    );

STATUS pciConfigExtCapFind1
    (
    UINT8 extCapFindId,    /* Extended capabilities ID to search for */
    int nPEX,
    int bus,               /* PCI bus number */
    int device,            /* PCI device number */
    int function,          /* PCI function number */
    UINT8 * pOffset        /* returned config space offset */
    );
STATUS pcieFindDevice1(
    int    nPEX,
    int    vendorId,    /* vendor ID */
    int    deviceId,    /* device ID */
    int    index,    /* desired instance of device */
    int *  pBusNo,    /* bus number */
    int *  pDeviceNo,    /* device number */
    int *  pFuncNo    /* function number */
    );
int pcieHoseScanBus(int nPEX
		           ,int prtBusNo
		           ,int prtDeviceNo
	               ,int prtFuncNo
		           ,T_pcieHose *pcieHose);
#ifdef __cplusplus
}
#endif

#endif
