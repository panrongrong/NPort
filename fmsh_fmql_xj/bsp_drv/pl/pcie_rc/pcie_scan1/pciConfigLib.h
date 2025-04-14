/****************************************************************
* ��Ȩ����(c) 2009. �й����չ�ҵ������һ�о���
*
* ��Դ����AS-IS�����Ͽ�ͨ��ʹ��ACoreOS����ϵͳ�õ�Ӧ�á������� 
* AVIC ACTRI.��Ȩ�µ��κ������κη�ʽӦ�øô��룬���Ը��ơ��޸ġ�
* ���룬������ԭ�����޸ĺ���뵽���������С��κ������øô��룬���
* ��������������������AVIC ACTRI.��Ȩ������
*
* ��������Ȩ��
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
* �ļ�����: XXX
* ժ    Ҫ: 
* ��ǰ�汾: X.X.X
* ��    ��: XXX
* �������: XXXX��X��X��
*/



/*
  ��    ��    ��    ʷ
  --------------------
  �޸�����     �޸���    �汾��  ��������
  
  20XX-XX-XX   XXXXXX    1.X     ��ϸ����    
  
*/
/****************************************************************
* ��Ȩ����(c) 2009. �й����չ�ҵ������һ�о���
*
* ��Դ����AS-IS�����Ͽ�ͨ��ʹ��ACoreOS����ϵͳ�õ�Ӧ�á�������
* AVIC ACTRI.��Ȩ�µ��κ������κη�ʽӦ�øô��룬���Ը��ơ��޸ġ�
* ���룬������ԭ�����޸ĺ���뵽���������С��κ������øô��룬���
* ��������������������AVIC ACTRI.��Ȩ������
*
* ��������Ȩ��
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
* �ļ�����: XXX
* ժ    Ҫ:
* ��ǰ�汾: X.X.X
* ��    ��: XXX
* �������: XXXX��X��X��
*/



/*
  ��    ��    ��    ʷ
  --------------------
  �޸�����     �޸���    �汾��  ��������

  20XX-XX-XX   XXXXXX    1.X     ��ϸ����

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


#define LSB(x)    ((x) & 0xff)             /* ȡ���ֽں� */
#define MSB(x)    (((x) >> 8) & 0xff)      /* ȡ���ֽں� */
#define LSW(x)    ((x) & 0xffff)           /* ȡ���ֺ� */
#define MSW(x)    (((x) >> 16) & 0xffff)   /* ȡ���ֺ� */

/* 32λ�ְ��ֵ����*/
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
