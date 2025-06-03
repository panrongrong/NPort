/*
 * pcieConfigLib.c
 *
 *  Created on: 2019Äê9ÔÂ19ÈÕ
 *      Author: wliang
 */

#include "platform.h"
/*#include <sysTypes.h>*/

/*#include "printk.h"*/
#include "string.h"

/*#include "pci/pci.h"*/
/*#include "pci/pciConfigLib.h"*/
/*#include "pci/pcieAutoConfig.h"*/
#include "pci.h"
#include "pciConfigLib.h"
#include "pcieAutoConfig.h"

T_pcieDev pcieDev;
/*------------------------------------------------------------------------------------*/
static void PCI_OUT_LONG1 (
    UINT32 port,
    UINT32 data)
{
    *(UINT32*)port = data;
}

static UINT32 PCI_IN_LONG1 (
		UINT32 port)
{
    return (*(UINT32*)port);
}

static void PCI_OUT_WORD1 (
		UINT32 port,
        UINT16 data)
{
    *(UINT16*)port = data;
}

static UINT16 PCI_IN_WORD1 (
		UINT32 port)
{
    return(*(UINT16*)port);
}

static void PCI_OUT_BYTE1 (
		UINT32 port,
         UINT8 data)
{
    *(UINT8*)port = data;
}

static UINT8 PCI_IN_BYTE1 (
		UINT32 port)
{
    return(*(UINT8*)port);
}
/*******************************************************************************
 *
 * pciConfigInByte - read one byte from the PCI configuration space
 *
 * This routine reads one byte from the PCI configuration space
 *
 * RETURNS: OK, or ERROR if this library is not initialized
 */
STATUS pciConfigInByte1
(
		int	busNo,    /* bus number */
		int	deviceNo, /* device number */
		int	funcNo,	  /* function number */
		int	offset,	  /* offset into the configuration space */
		UINT8 * pData /* data read from the offset */
)
{
	return pcieConfigInByte1(0, busNo,deviceNo,funcNo,offset,pData);
}

/*******************************************************************************
 *
 * pciConfigInWord - read one word from the PCI configuration space
 *
 * This routine reads one word from the PCI configuration space
 *
 * RETURNS: OK, or ERROR if this library is not initialized
 */
STATUS pciConfigInWord1
(
		int	busNo,      /* bus number */
		int	deviceNo,   /* device number */
		int	funcNo,     /* function number */
		int	offset,     /* offset into the configuration space */
		UINT16 * pData  /* data read from the offset */
)
{
	return pcieConfigInWord1(0,busNo,deviceNo,funcNo,offset,pData);
}
STATUS pciConfigInLong1(
    int    busNo,     /* bus number */
    int    deviceNo,  /* device number */
    int    funcNo,    /* function number */
    int    offset,    /* offset into the configuration space */
    UINT32 * pData /* data read from the offset */
    )
{
	return pcieConfigInLong1(0,busNo,deviceNo,funcNo,offset,pData);
}
/*******************************************************************************
 *
 * pciConfigOutByte - write one byte to the PCI configuration space
 *
 * This routine writes one byte to the PCI configuration space.
 *
 * RETURNS: OK, or ERROR if this library is not initialized
 */

STATUS pciConfigOutByte1
(
		int	busNo,    /* bus number */
		int	deviceNo, /* device number */
		int	funcNo,   /* function number */
		int	offset,   /* offset into the configuration space */
		UINT8 data    /* data written to the offset */
)
{
	return pcieConfigOutByte1(0,busNo,deviceNo,funcNo,offset,data);
}
/*******************************************************************************
 *
 * pciConfigOutWord - write one 16-bit word to the PCI configuration space
 *
 * This routine writes one 16-bit word to the PCI configuration space.
 *
 * RETURNS: OK, or ERROR if this library is not initialized
 */

STATUS pciConfigOutWord1
(
		int	busNo,    /* bus number */
		int	deviceNo, /* device number */
		int	funcNo,   /* function number */
		int	offset,   /* offset into the configuration space */
		UINT16 data   /* data written to the offset */
)
{
	return pcieConfigOutWord1(0,busNo,deviceNo,funcNo,offset,data);
}

/*******************************************************************************
*
* pciConfigOutLong - write one longword to the PCI configuration space
*
* This routine writes one longword to the PCI configuration space.
*
* RETURNS: OK, or ERROR if this library is not initialized
*/

STATUS pciConfigOutLong1(
    int	busNo,    /* bus number */
    int	deviceNo, /* device number */
    int	funcNo,   /* function number */
    int	offset,   /* offset into the configuration space */
    UINT32 data   /* data written to the offset */
    )
{
	return pcieConfigOutLong1(0,busNo,deviceNo,funcNo,offset,data);
}
/*******************************************************************************
 *
 * pciFindDevice - find the nth device with the given device & vendor ID
 *
 * This routine finds the nth device with the given device & vendor ID.
 *
 * RETURNS:
 * OK, or ERROR if the deviceId and vendorId didn't match.
 */

STATUS pciFindDevice1
(
		int    vendorId,	/* vendor ID */
		int    deviceId,	/* device ID */
		int    index,	/* desired instance of device */
		int *  pBusNo,	/* bus number */
		int *  pDeviceNo,	/* device number */
		int *  pFuncNo	/* function number */
)
{
	return pcieFindDevice1(0,vendorId,deviceId,index,pBusNo,pDeviceNo,pFuncNo);

}

void showPciDevice(void)
{

	int count = 0;

	for(count = 0; count < PCIE_DEV_INFO_MUX; count++)
	{
		if(pcieDev.pcieDevInfo[count].vendorID != 0)
		{
			printf("bus:%d dev:%d fun:%x VID:%x DID:%x\n"
			,pcieDev.pcieDevInfo[count].busNo
		    ,pcieDev.pcieDevInfo[count].deviceNo
		    ,pcieDev.pcieDevInfo[count].funcNo
		    ,pcieDev.pcieDevInfo[count].vendorID
		    ,pcieDev.pcieDevInfo[count].deviceID);
		}
	}
}


UINT32  sysPciInLong(UINT32 * pPciAddr)
{
	UINT32 retval;
	retval = (*(UINT32 *)(pPciAddr));
	return (retval);
}


void   sysPciOutLong(UINT32 * pPciAddr,UINT32 dataOut )
{
	*pPciAddr=(*(UINT32 *)(dataOut));
}
/*******************************************************************************
*
* pciConfigBdfPack - pack parameters for the Configuration Address Register
*
* This routine packs three parameters into one integer for accessing the
* Configuration Address Register
*
* RETURNS: packed integer encoded version of bus, device, and function numbers.
*/

UINT32 pcieConfigBdfPack(
	UINT32 busNo,    /* bus number */
	UINT32 deviceNo, /* device number */
	UINT32 funcNo    /* function number */
    )
{
	return (((busNo    << 20) & 0x03f00000) | /*25:20*/
		    ((deviceNo << 15) & 0x000f8000) | /*19:15*/
		    ((funcNo   << 12) & 0x00007000)); /*14:12*/
}

STATUS pcieConfigInByte1(
    int nPEX,
    int    busNo,    /* bus number */
    int    deviceNo, /* device number */
    int    funcNo,      /* function number */
    int    offset,      /* offset into the configuration space */
    UINT8 * pData /* data read from the offset */
    )
    {
	STATUS retStat = ERROR;
	UINT32 conf_addr;
	
    /* 
     * check whether it is a valid access.
     * 1. every root port has only one slot.
     *    so, if (bus ==0 && devNum > 0), just return.
     * 2. there is only one device on the bus directly attached to RC's.
     *    so, if (bus ==1 && devNum > 0), just return.
     */
    if ((busNo <= 1) && (deviceNo != 0))
    {
    	*pData = 0xFFFFFFFF;
        return (ERROR);
    }


	if (nPEX == 0) {
		conf_addr = PCIE_CONFIG_BASE;
	} else if (nPEX == 1) {
		conf_addr = PCIE_CONFIG_BASE;
	}else {
		printf("Wrong PCIE Num!\n");
		return retStat;
	}
	/*key = intLock ();  *//* mutual exclusion start */
	*pData = PCI_IN_BYTE1(conf_addr
			           + pcieConfigBdfPack(busNo,deviceNo,funcNo)
			           + offset);
	/*key = intUnlock (key);  *//* mutual exclusion start */
	retStat = OK;

	return (retStat);
}

/*******************************************************************************
*
* pcie1ConfigInWord - read one word from the PCI configuration space
*
* This routine reads one word from the PCI configuration space
*
* RETURNS: OK, or ERROR if this library is not initialized
*/

STATUS pcieConfigInWord1(
    int    nPEX,
    int    busNo,      /* bus number */
    int    deviceNo,   /* device number */
    int    funcNo,     /* function number */
    int    offset,     /* offset into the configuration space */
    UINT16 * pData  /* data read from the offset */
    )
    {
	STATUS retStat = ERROR;
	UINT32 conf_addr;
	
    /* 
     * check whether it is a valid access.
     * 1. every root port has only one slot.
     *    so, if (bus ==0 && devNum > 0), just return.
     * 2. there is only one device on the bus directly attached to RC's.
     *    so, if (bus ==1 && devNum > 0), just return.
     */
    if ((busNo <= 1) && (deviceNo != 0))
    {
    	*pData = 0xFFFFFFFF;
        return (ERROR);
    }

	if (nPEX == 0) {
		conf_addr = PCIE_CONFIG_BASE;
	} else if (nPEX == 1) {
		conf_addr = PCIE_CONFIG_BASE;
	}else {
		printf("Wrong PCIE Num!\n");
		return retStat;
	}


	/*key = intLock ();  *//* mutual exclusion start */
	*pData = PCI_IN_WORD1(conf_addr
				       + pcieConfigBdfPack(busNo,deviceNo,funcNo)
				       + offset);
	retStat = OK;
	/*intUnlock (key);  *//* mutual exclusion stop */


	return (retStat);
}


/*******************************************************************************
*
* pcie1ConfigInLong - read one longword from the PCI configuration space
*
* This routine reads one longword from the PCI configuration space
*
* RETURNS: OK, or ERROR if this library is not initialized
*/

STATUS pcieConfigInLong1(
    int    nPEX,
    int    busNo,     /* bus number */
    int    deviceNo,  /* device number */
    int    funcNo,    /* function number */
    int    offset,    /* offset into the configuration space */
    UINT32 * pData /* data read from the offset */
    )
    {
    /*int         key;*/
    STATUS retStat = ERROR;
    UINT32 conf_addr;
	
    /* 
     * check whether it is a valid access.
     * 1. every root port has only one slot.
     *    so, if (bus ==0 && devNum > 0), just return.
     * 2. there is only one device on the bus directly attached to RC's.
     *    so, if (bus ==1 && devNum > 0), just return.
     */
    if ((busNo <= 1) && (deviceNo != 0))
    {
    	*pData = 0xFFFFFFFF;
        return (ERROR);
    }

    if(nPEX == 0){
    	conf_addr = PCIE_CONFIG_BASE;
    }else if(nPEX == 1){
    	conf_addr = PCIE_CONFIG_BASE;
    }else{
    	printf("Wrong PCIE Num!\n");
    	return retStat;
    }

	/*key = intLock ();  *//* mutual exclusion start */
	*pData = PCI_IN_LONG1(conf_addr
				       + pcieConfigBdfPack(busNo,deviceNo,funcNo)
				       + offset);
	retStat = OK;
	/*intUnlock (key);  *//* mutual exclusion stop */

    return (retStat);
}

/*******************************************************************************
*
* pciConfigOutByte - write one byte to the PCI configuration space
*
* This routine writes one byte to the PCI configuration space.
*
* RETURNS: OK, or ERROR if this library is not initialized
*/

STATUS pcieConfigOutByte1(
		int nPEX,
		int busNo,    /* bus number */
        int deviceNo, /* device number */
        int funcNo,   /* function number */
        int offset,   /* offset into the configuration space */
         UINT8 data   /* data written to the offset */
)
{
	UINT32 conf_addr;
	
	/* 
	 * check whether it is a valid access.
	 * 1. every root port has only one slot.
	 *	  so, if (bus ==0 && devNum > 0), just return.
	 * 2. there is only one device on the bus directly attached to RC's.
	 *	  so, if (bus ==1 && devNum > 0), just return.
	 */
	if ((busNo <= 1) && (deviceNo != 0))
	{
		return (ERROR);
	}

	if (nPEX == 0) {
		conf_addr = PCIE_CONFIG_BASE;
	} else if (nPEX == 1) {
		conf_addr = PCIE_CONFIG_BASE;
	}else {
		printf("Wrong PCIE Num!\n");
		return ERROR;
	}

	/*key = intLock ();  *//* mutual exclusion start */
	PCI_OUT_BYTE1(conf_addr
			   + pcieConfigBdfPack(busNo,deviceNo,funcNo)
			   + offset
			   , data);
	/*intUnlock (key);  *//* mutual exclusion stop */

	return (OK);
}

/*******************************************************************************
*
* pcie1ConfigOutWord - write one 16-bit word to the PCI configuration space
*
* This routine writes one 16-bit word to the PCI configuration space.
*
* RETURNS: OK, or ERROR if this library is not initialized
*/

STATUS pcieConfigOutWord1(
    int    nPEX,
    int    busNo,    /* bus number */
    int    deviceNo, /* device number */
    int    funcNo,   /* function number */
    int    offset,   /* offset into the configuration space */
    UINT16 data   /* data written to the offset */
    )
{
	UINT32 conf_addr;
	
	/* 
	 * check whether it is a valid access.
	 * 1. every root port has only one slot.
	 *	  so, if (bus ==0 && devNum > 0), just return.
	 * 2. there is only one device on the bus directly attached to RC's.
	 *	  so, if (bus ==1 && devNum > 0), just return.
	 */
	if ((busNo <= 1) && (deviceNo != 0))
	{
		return (ERROR);
	}

	if (nPEX == 0) {
		conf_addr = PCIE_CONFIG_BASE;
	} else if (nPEX == 1) {
		conf_addr = PCIE_CONFIG_BASE;
	}else {
		printf("Wrong PCIE Num!\n");
		return ERROR;
	}

	PCI_OUT_WORD1(conf_addr
			   + pcieConfigBdfPack(busNo,deviceNo,funcNo)
			   + offset
			   , data);

	return (OK);
}



/*******************************************************************************
*
* pciConfigOutLong - write one longword to the PCI configuration space
*
* This routine writes one longword to the PCI configuration space.
*
* RETURNS: OK, or ERROR if this library is not initialized
*/

STATUS pcieConfigOutLong1(
    int nPEX,
    int	busNo,    /* bus number */
    int	deviceNo, /* device number */
    int	funcNo,   /* function number */
    int	offset,   /* offset into the configuration space */
    UINT32 data   /* data written to the offset */
    )
{
	UINT32 conf_addr;
	
	/* 
	 * check whether it is a valid access.
	 * 1. every root port has only one slot.
	 *	  so, if (bus ==0 && devNum > 0), just return.
	 * 2. there is only one device on the bus directly attached to RC's.
	 *	  so, if (bus ==1 && devNum > 0), just return.
	 */
	if ((busNo <= 1) && (deviceNo != 0))
	{
		return (ERROR);
	}

	if (nPEX == 0) {
		conf_addr = PCIE_CONFIG_BASE;
	} else if (nPEX == 1) {
		conf_addr = PCIE_CONFIG_BASE;
	}else {
		printf("Wrong PCIE Num!\n");
		return ERROR;
	}

	PCI_OUT_LONG1(conf_addr
			   + pcieConfigBdfPack(busNo,deviceNo,funcNo)
			   + offset
			   , data);

	return (OK);
}


/*******************************************************************************
*
* pciConfigExtCapFind - find extended capability in ECP linked list
*
* This routine searches for an extended capability in the linked list of
* capabilities in config space. If found, the offset of the first byte
* of the capability of interest in config space is returned via pOffset.
*
* RETURNS: OK if Extended Capability found, ERROR otherwise
*/

STATUS pciConfigExtCapFind1
    (
    UINT8 extCapFindId,    /* Extended capabilities ID to search for */
    int nPEX,
    int bus,               /* PCI bus number */
    int device,            /* PCI device number */
    int function,          /* PCI function number */
    UINT8 * pOffset        /* returned config space offset */
    )
    {
    STATUS retStat = ERROR;
    UINT16 tmpStat;
    UINT8  tmpOffset;
    UINT8  capOffset = 0x00;
    UINT8  capId = 0x00;

    /* Check to see if the device has any extended capabilities */

    pcieConfigInWord1(nPEX,bus, device, function, PCI_STATUS, &tmpStat);

    if ((tmpStat & PCI_STATUS_CAP_LIST) == 0)
        {
        return retStat;
        }

    /* Get the initial ECP offset and make longword aligned */

    pcieConfigInByte1(nPEX,bus, device, function, PCI_CAPABILITY_LIST, &capOffset);
    capOffset &= ~0x02;

    /* Bounds check the ECP offset */

    if (capOffset < 0x40)
        {
        return retStat;
        }

    /* Look for the specified Extended Cap item in the linked list */

    while (capOffset != 0x00)
        {

        /* Get the Capability ID and check */

        pcieConfigInByte1(nPEX,bus, device, function, (int)capOffset, &capId);
        if (capId == extCapFindId)
            {
            *pOffset = (capOffset + (UINT8)0x02);
            retStat = OK;
            break;
            }

        /* Get the offset to the next New Capabilities item */

        tmpOffset = capOffset + (UINT8)0x01;
        pcieConfigInByte1(nPEX,bus, device, function, (int)tmpOffset, &capOffset);

        }

    return retStat;
}

void initPcieDevInfo(void)
{
	memset(&pcieDev,0,sizeof(pcieDev));
}

STATUS getPcieDevInfo(int nPEX
		,int vendorID,int deviceID,int index
		,int *pbusNo,int *pdeviceNo,int *pfunNo)
{
	int count = 0;
	STATUS result = ERROR;

	for(count = 0; count < PCIE_DEV_INFO_MUX; count++)
	{
		if((pcieDev.pcieDevInfo[count].vendorID == vendorID)
		&& (pcieDev.pcieDevInfo[count].deviceID == deviceID)
		&& (index-- == 0))
		{
			*pbusNo    = pcieDev.pcieDevInfo[count].busNo   ;
		    *pdeviceNo = pcieDev.pcieDevInfo[count].deviceNo;
		    *pfunNo    = pcieDev.pcieDevInfo[count].funcNo  ;
		    result   = OK;
		    break;
		}
	}
	return result;
}

STATUS setPcieDevInfo(int nPEX,int vendorID,int deviceID,int busNo,int deviceNo,int funNo)
{
	if (pcieDev.pcieDevCount > PCIE_DEV_INFO_MUX)
	{	
		return ERROR;
	}

	pcieDev.pcieDevInfo[pcieDev.pcieDevCount].vendorID = vendorID;
	pcieDev.pcieDevInfo[pcieDev.pcieDevCount].deviceID = deviceID;
	pcieDev.pcieDevInfo[pcieDev.pcieDevCount].busNo    = busNo;
	pcieDev.pcieDevInfo[pcieDev.pcieDevCount].deviceNo = deviceNo;
	pcieDev.pcieDevInfo[pcieDev.pcieDevCount].funcNo   = funNo;
	
	pcieDev.pcieDevCount++;

	return OK;
}
/*******************************************************************************
*
* pciFindDevice - find the nth device with the given device & vendor ID
*
* This routine finds the nth device with the given device & vendor ID.
*
* RETURNS:
* OK, or ERROR if the deviceId and vendorId didn't match.
*/



STATUS pcieFindDevice1(
    int    nPEX,
    int    vendorId,    /* vendor ID */
    int    deviceId,    /* device ID */
    int    index,    /* desired instance of device */
    int *  pBusNo,    /* bus number */
    int *  pDeviceNo,    /* device number */
    int *  pFuncNo    /* function number */
    )
 {
	STATUS status = ERROR;

	status = getPcieDevInfo(nPEX,vendorId,deviceId,index,pBusNo,pDeviceNo,pFuncNo);

	return (status);
}

int pciSkipDev(
		int nPEX,
	    int	busNo,      /* bus number */
	    int	deviceNo,   /* device number */
	    int	funcNo,     /* function number */
	    int prtBusNo,   /*parent bus number */
	    int prtDeviceNo,/*parent device number */
	    int prtFuncNo   /*parent function number */
	    )
{
	UINT8 capp, capID;
	UINT16 pcieCR;
	UINT8 portType;

	if (busNo == 0)
		return 0;
	
	capp = PCI_CAPABILITIES_POINTER;
	
	while(1) 
	{
		/*read capabilities pointer*/
		pcieConfigInByte1(nPEX
				,prtBusNo
				,prtDeviceNo
				,prtFuncNo
				,capp,&capp);

		if (capp < 0x40)
		{
			break;
		}
		
		capp &= ~3;
		
        /*read cap ID*/
		pcieConfigInByte1(nPEX
				,prtBusNo
				,prtDeviceNo
				,prtFuncNo
				,capp, &capID);

		if (capID == 0xff)
		{
			break;
		}
		
		if (capID == PCI_CAP_ID_EXP) 
		{
			/*read pcie capability*/
			pcieConfigInWord1(nPEX
					,prtBusNo
					,prtDeviceNo
					,prtFuncNo
					,(capp+2),&pcieCR);

			portType = (pcieCR >> 4) & 0xf;
			
			/* portType: 0-EP 4-Root port 5-switch up 6-switch down ...*/
			if (((portType == 0x6) || (portType == 0x4)) && (deviceNo != 0))
			{
				return 1;
			}
			else
				return 0;
		}
		
		/*read next cap pointer*/
		capp += 1;
	}

	return 0;
}

void pcieHoseScan(int nPEX,T_pcieHose *pcieHose)
{
	pciAutoConfigInit(pcieHose);
    pcieHoseScanBus(nPEX,0xff,0xff,0xff,pcieHose);
}

int pcieHoseScanBus(int nPEX
		           ,int prtBusNo
		           ,int prtDeviceNo
	               ,int prtFuncNo
		           ,T_pcieHose *pcieHose)
{
	int busNo = 0;
	int deviceNo = 0;
	int funcNo = 0;
	int subBus = 0,num = 0,foundMulti = 0;
	UINT16 vendor = 0, device = 0, class = 0;
	UINT8 headerType = 0;

	subBus = pcieHose->currentBusno;
	busNo  = pcieHose->currentBusno;
	
	for (deviceNo = 0; deviceNo < PCI_MAX_DEV; ++deviceNo)
	{
		for (funcNo = 0; funcNo < PCI_MAX_FUNC; funcNo++)
		{
			if(pciSkipDev(nPEX,busNo   ,deviceNo   ,funcNo
					          ,prtBusNo,prtDeviceNo,prtFuncNo))
			{
				break;
			}
			
			if (funcNo && !foundMulti)
				continue;

			pcieConfigInByte1(nPEX, busNo, deviceNo, funcNo
					        ,PCI_HEADER_TYPE, &headerType);

			pcieConfigInWord1(nPEX, busNo, deviceNo, funcNo,
							PCI_VENDOR_ID, &vendor);

			if (vendor == 0xffff || vendor == 0x0000)
				continue;

			if (!funcNo)
				foundMulti = headerType & 0x80;


			pcieConfigInWord1(nPEX, busNo, deviceNo, funcNo
					        ,PCI_DEVICE_ID, &device);
			pcieConfigInWord1(nPEX, busNo, deviceNo, funcNo
					        ,PCI_CLASS_DEVICE, &class);


#if CONFIG_PCI_SCAN_SHOW
            printf("bus:%d dev:%d fun:%x VID:%x DID:%x\n"
            	   ,busNo, deviceNo,funcNo,vendor,device);
#endif
            if (ERROR == setPcieDevInfo(nPEX,vendor,device,busNo,deviceNo,funcNo))
            {	
            	printf("setPcieDevInfo ERROR\n");
            }

            /*num = pcieAutoCfgBridge(nPEX,busNo,deviceNo,funcNo,pcieHose);*/
            num = pciAutoConfigDevice(nPEX,busNo,deviceNo,funcNo,pcieHose);
			
            subBus = num > subBus ? num : subBus;
		}
	}
	return subBus;
}
				   
/*
 * Subroutine:  PCI_Header_Show
 *
 * Description: Reads the header of the specified PCI device.
 *
 * Inputs:		BusDevFunc      Bus+Device+Function number
 *
 * Return:      None
 *
 */
void pciHeaderShow1(int nPEX, int busNo, int deviceNo, int funcNo)
{
	UINT8 Byte, headerType;
	UINT16 Word;
	UINT32 Long;

#define PRINTB(msg, type, reg) \
	pcieConfigInByte1(nPEX,busNo,deviceNo,funcNo, reg, &type); \
	printf(msg, type)

#define PRINTW(msg, type, reg) \
	pcieConfigInWord1(nPEX,busNo,deviceNo,funcNo, reg, &type); \
	printf(msg, type)


#define PRINTL(msg, type, reg) \
	pcieConfigInLong1(nPEX,busNo,deviceNo,funcNo, reg, &type); \
	printf(msg, type)

	pcieConfigInByte1(nPEX,busNo,deviceNo,funcNo, PCI_HEADER_TYPE, &headerType);

	PRINTW ("  vendor ID =                   0x%x\n", Word, PCI_VENDOR_ID);
	PRINTW ("  device ID =                   0x%x\n", Word, PCI_DEVICE_ID);
	PRINTW ("  command register =            0x%x\n", Word, PCI_COMMAND);
	PRINTW ("  status register =             0x%x\n", Word, PCI_STATUS);
	PRINTB ("  revision ID =                 0x%x\n", Byte, PCI_REVISION_ID);
	PRINTB ("  class code =                  0x%x\n", Byte, PCI_CLASS_CODE);
	PRINTB ("  sub class code =              0x%x\n", Byte, PCI_CLASS_SUB_CODE);
	PRINTB ("  programming interface =       0x%x\n", Byte, PCI_CLASS_PROG);
	PRINTB ("  cache line =                  0x%x\n", Byte, PCI_CACHE_LINE_SIZE);
	PRINTB ("  latency time =                0x%x\n", Byte, PCI_LATENCY_TIMER);
	PRINTB ("  header type =                 0x%x\n", Byte, PCI_HEADER_TYPE);
	PRINTB ("  BIST =                        0x%x\n", Byte, PCI_BIST);
	PRINTL ("  base address 0 =              0x%x\n", Long, PCI_BASE_ADDRESS_0);

	switch (headerType & 0x03) {
	case PCI_HEADER_TYPE_NORMAL:	/* "normal" PCI device */
		PRINTL ("  base address 1 =              0x%x\n", Long, PCI_BASE_ADDRESS_1);
		PRINTL ("  base address 2 =              0x%x\n", Long, PCI_BASE_ADDRESS_2);
		PRINTL ("  base address 3 =              0x%x\n", Long, PCI_BASE_ADDRESS_3);
		PRINTL ("  base address 4 =              0x%x\n", Long, PCI_BASE_ADDRESS_4);
		PRINTL ("  base address 5 =              0x%x\n", Long, PCI_BASE_ADDRESS_5);
		PRINTL ("  cardBus CIS pointer =         0x%x\n", Long, PCI_CARDBUS_CIS);
		PRINTW ("  sub system vendor ID =        0x%x\n", Word, PCI_SUBSYSTEM_VENDOR_ID);
		PRINTW ("  sub system ID =               0x%x\n", Word, PCI_SUBSYSTEM_ID);
		PRINTL ("  expansion ROM base address =  0x%x\n", Long, PCI_ROM_ADDRESS);
		PRINTB ("  interrupt line =              0x%x\n", Byte, PCI_INTERRUPT_LINE);
		PRINTB ("  interrupt pin =               0x%x\n", Byte, PCI_INTERRUPT_PIN);
		PRINTB ("  min Grant =                   0x%x\n", Byte, PCI_MIN_GNT);
		PRINTB ("  max Latency =                 0x%x\n", Byte, PCI_MAX_LAT);
		break;

	case PCI_HEADER_TYPE_BRIDGE:	/* PCI-to-PCI bridge */

		PRINTL ("  base address 1 =              0x%x\n", Long, PCI_BASE_ADDRESS_1);
		PRINTB ("  primary bus number =          0x%x\n", Byte, PCI_PRIMARY_BUS);
		PRINTB ("  secondary bus number =        0x%x\n", Byte, PCI_SECONDARY_BUS);
		PRINTB ("  subordinate bus number =      0x%x\n", Byte, PCI_SUBORDINATE_BUS);
		PRINTB ("  secondary latency timer =     0x%x\n", Byte, PCI_SEC_LATENCY_TIMER);
		PRINTB ("  IO base =                     0x%x\n", Byte, PCI_IO_BASE);
		PRINTB ("  IO limit =                    0x%x\n", Byte, PCI_IO_LIMIT);
		PRINTW ("  secondary status =            0x%x\n", Word, PCI_SEC_STATUS);
		PRINTW ("  memory base =                 0x%x\n", Word, PCI_MEMORY_BASE);
		PRINTW ("  memory limit =                0x%x\n", Word, PCI_MEMORY_LIMIT);
		PRINTW ("  prefetch memory base =        0x%x\n", Word, PCI_PREF_MEMORY_BASE);
		PRINTW ("  prefetch memory limit =       0x%x\n", Word, PCI_PREF_MEMORY_LIMIT);
		PRINTL ("  prefetch memory base upper =  0x%x\n", Long, PCI_PREF_BASE_UPPER32);
		PRINTL ("  prefetch memory limit upper = 0x%x\n", Long, PCI_PREF_LIMIT_UPPER32);
		PRINTW ("  IO base upper 16 bits =       0x%x\n", Word, PCI_IO_BASE_UPPER16);
		PRINTW ("  IO limit upper 16 bits =      0x%x\n", Word, PCI_IO_LIMIT_UPPER16);
		PRINTL ("  expansion ROM base address =  0x%x\n", Long, PCI_ROM_ADDRESS1);
		PRINTB ("  interrupt line =              0x%x\n", Byte, PCI_INTERRUPT_LINE);
		PRINTB ("  interrupt pin =               0x%x\n", Byte, PCI_INTERRUPT_PIN);
		PRINTW ("  bridge control =              0x%x\n", Word, PCI_BRIDGE_CONTROL);
		break;

	case PCI_HEADER_TYPE_CARDBUS:	/* PCI-to-CardBus bridge */

		PRINTB ("  capabilities =                0x%x\n", Byte, PCI_CB_CAPABILITY_LIST);
		PRINTW ("  secondary status =            0x%x\n", Word, PCI_CB_SEC_STATUS);
		PRINTB ("  primary bus number =          0x%x\n", Byte, PCI_CB_PRIMARY_BUS);
		PRINTB ("  CardBus number =              0x%x\n", Byte, PCI_CB_CARD_BUS);
		PRINTB ("  subordinate bus number =      0x%x\n", Byte, PCI_CB_SUBORDINATE_BUS);
		PRINTB ("  CardBus latency timer =       0x%x\n", Byte, PCI_CB_LATENCY_TIMER);
		PRINTL ("  CardBus memory base 0 =       0x%x\n", Long, PCI_CB_MEMORY_BASE_0);
		PRINTL ("  CardBus memory limit 0 =      0x%x\n", Long, PCI_CB_MEMORY_LIMIT_0);
		PRINTL ("  CardBus memory base 1 =       0x%x\n", Long, PCI_CB_MEMORY_BASE_1);
		PRINTL ("  CardBus memory limit 1 =      0x%x\n", Long, PCI_CB_MEMORY_LIMIT_1);
		PRINTW ("  CardBus IO base 0 =           0x%x\n", Word, PCI_CB_IO_BASE_0);
		PRINTW ("  CardBus IO base high 0 =      0x%x\n", Word, PCI_CB_IO_BASE_0_HI);
		PRINTW ("  CardBus IO limit 0 =          0x%x\n", Word, PCI_CB_IO_LIMIT_0);
		PRINTW ("  CardBus IO limit high 0 =     0x%x\n", Word, PCI_CB_IO_LIMIT_0_HI);
		PRINTW ("  CardBus IO base 1 =           0x%x\n", Word, PCI_CB_IO_BASE_1);
		PRINTW ("  CardBus IO base high 1 =      0x%x\n", Word, PCI_CB_IO_BASE_1_HI);
		PRINTW ("  CardBus IO limit 1 =          0x%x\n", Word, PCI_CB_IO_LIMIT_1);
		PRINTW ("  CardBus IO limit high 1 =     0x%x\n", Word, PCI_CB_IO_LIMIT_1_HI);
		PRINTB ("  interrupt line =              0x%x\n", Byte, PCI_INTERRUPT_LINE);
		PRINTB ("  interrupt pin =               0x%x\n", Byte, PCI_INTERRUPT_PIN);
		PRINTW ("  bridge control =              0x%x\n", Word, PCI_CB_BRIDGE_CONTROL);
		PRINTW ("  subvendor ID =                0x%x\n", Word, PCI_CB_SUBSYSTEM_VENDOR_ID);
		PRINTW ("  subdevice ID =                0x%x\n", Word, PCI_CB_SUBSYSTEM_ID);
		PRINTL ("  PC Card 16bit base address =  0x%x\n", Long, PCI_CB_LEGACY_MODE_BASE);
		break;

	default:
		printf("unknown header\n");
		break;
    }

#undef PRINTB
#undef PRINTW
#undef PRINTL
}

/*******************************************************************************
*
* pciCfgHeaderShowAll - Call pciHeaderShow for all PCI devices
*
* This function calls pciHeaderShow() for all PCI devices found within the
* system.
*
* RETURNS: N/A
*/
#define	PCI_CFG_HEADER_TYPE	0x0e
#define	PCI_CFG_VENDOR_ID	0x00
#define MAXPCIDEV 10            /* Number of PCI devices on a bus */
#define MAXPCIFUNC 8            /* Number of PCI functions on a device */
void pciCfgHeaderShowAll(int nPEX)
{
    UINT32 busNo;	/* PCI Bus Number */
    UINT32 devNo;	/* PCI Device Number */
    UINT32 func;	/* PCI Function Number */
    UINT16 vendorId;	/* Vendor ID */
    UINT16 headerType;	/* Header Type */


    int count = 0;

    	for(count = 0; count < PCIE_DEV_INFO_MUX; count++)
    	{
    		if(pcieDev.pcieDevInfo[count].vendorID != 0)
    		{
    			printf("bus:%d dev:%d fun:%x VID:%x DID:%x\n"
    			,pcieDev.pcieDevInfo[count].busNo
    		    ,pcieDev.pcieDevInfo[count].deviceNo
    		    ,pcieDev.pcieDevInfo[count].funcNo
    		    ,pcieDev.pcieDevInfo[count].vendorID
    		    ,pcieDev.pcieDevInfo[count].deviceID);

    			pciHeaderShow1(nPEX
				,pcieDev.pcieDevInfo[count].busNo
				,pcieDev.pcieDevInfo[count].deviceNo
				,pcieDev.pcieDevInfo[count].funcNo);
    		}
    	}
}



