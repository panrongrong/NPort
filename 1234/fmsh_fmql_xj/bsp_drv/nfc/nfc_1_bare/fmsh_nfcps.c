#include "../../common/fmsh_ps_parameters.h"
#include "../../slcr/fmsh_slcr.h"

#include "fmsh_nfcps_lib.h"

static void StubStatusHandler(void *callBackRef, u32 statusEvent,
                              u32 byteCount);

/******************************************************************************
* This table contains configuration information for each NFC
* device in the system.
******************************************************************************/
static FNfcPs_Config_T s_CfgTbl_nfc[] =
{
    {
        FPS_NFC_S_DEVICE_ID,
        FPS_NFC_S_BASEADDR,
        FPS_NFC_S_FLASHWIDTH
    }
}; 

/*****************************************************************************
* This function looks up the device configuration based on the unique device ID.
* The table s_CfgTbl_nfc contains the configuration info for each device
* in the system.
*
* @param	
*       - DeviceId contains the ID of the device for which the
*		device configuration pointer is to be returned.
*
* @return
*		- A pointer to the configuration found.
*		- NULL if the specified device ID was not found.
*
* @note		None.
*
******************************************************************************/
FNfcPs_Config_T* FNfcPs_LookupConfig(u16 deviceId)
{
    int index;
    FNfcPs_Config_T* cfgPtr = NULL;
    
    for (index = 0; index < FPS_NFC_NUM_INSTANCES; index++) {
        if (s_CfgTbl_nfc[index].deviceId == deviceId) {
            cfgPtr = &s_CfgTbl_nfc[index];
            break;
        }
    }
    return cfgPtr;
}

/*****************************************************************************
* This function initializes a specific FNfcPs_T device/instance. This function
* must be called prior to using the device to read or write any data.
*
* @param	nfc is a pointer to the FNfcPs_T instance.
* @param	configPtr points to the FNfcPs_T device configuration structure.
*
* @return
*		- FMSH_SUCCESS if successful.
*		- FMSH_FAILURE if fail.
*
* @note		The user needs to first call the FNfcPs_LookupConfig() API
*		which returns the Configuration structure pointer which is
*		passed as a parameter to the FNfcPs_CfgInitialize() API.
*
******************************************************************************/
int FNfcPs_CfgInitialize(FNfcPs_T *nfc, FNfcPs_Config_T *configPtr)
{
    FMSH_ASSERT(nfc != NULL);
    
    /* Init value */
    nfc->config.deviceId = configPtr->deviceId;
    nfc->config.baseAddress = configPtr->baseAddress;
    nfc->config.flashWidth = configPtr->flashWidth;
    nfc->flag = 0;
    
    nfc->spareBufferPtr = NULL;  
    
    nfc->statusHandler = StubStatusHandler; 
    
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function resets spi device registers to default value.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FNfcPs_Reset(FNfcPs_T* nfc)
{  
    FSlcrPs_ipSetRst(SLCR_NFC_CTRL, NFC_AHB_RST);
    FSlcrPs_ipSetRst(SLCR_NFC_CTRL, NFC_REF_RST);
	
    FSlcrPs_ipReleaseRst(SLCR_NFC_CTRL, NFC_AHB_RST);   
    FSlcrPs_ipReleaseRst(SLCR_NFC_CTRL, NFC_REF_RST);   
	
    /* clear RETRY_EN bit*/
    FMSH_WriteReg(nfc->config.baseAddress, NFCPS_CONTROL_OFFSET, 0x0);
	
    /* disable protect*/
    FMSH_WriteReg(nfc->config.baseAddress, NFCPS_MEM_CTRL_OFFSET, 0x0100);
}

/*****************************************************************************
* This function tests if nfc device exists.
*
* @param
*
* @return
*		- FMSH_SUCCESS if nfc device exists.
*		- FMSH_FAILURE if nfc device not exists.
*
* @note		
*
******************************************************************************/
int FNfcPs_SelfTest(FNfcPs_T* nfc)
{   
    u32 value;
    FNfcPs_SetEccOffset(nfc, 0x5a);
    value = FNfcPs_GetEccOffset(nfc);
    if(value != 0x5a)
        return FMSH_FAILURE;
    FNfcPs_SetEccOffset(nfc, 0x0);
    return FMSH_SUCCESS;   
}

/*****************************************************************************
* This function sets point to status handler as well as its callback parameter .
*
* @param
*
* @return
*
* @note		
*       - this function is usually used called in interrupt 
*       - implemented by user
*
******************************************************************************/
void FNfcPs_SetStatusHandler(FNfcPs_T* nfc, void* callBackRef,
                          Nfc_StatusHandler funcPtr)
{
    FMSH_ASSERT(nfc != NULL);
    FMSH_ASSERT(funcPtr != NULL);
    
    nfc->statusHandler = funcPtr;
    nfc->statusRef = callBackRef;
}

/*****************************************************************************
* This is a stub for the status callback. The stub is here in case the upper
* layers forget to set the handler.
*
* @param	CallBackRef is a pointer to the upper layer callback reference
* @param	StatusEvent is the event that just occurred.
* @param	ByteCount is the number of bytes transferred up until the event
*		occurred.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
static void StubStatusHandler(void *CallBackRef, u32 StatusEvent,
                              unsigned ByteCount)
{
    (void) CallBackRef;
    (void) StatusEvent;
    (void) ByteCount;
}

void FNfcPs_InterruptHandler(void *instancePtr)
{
    FNfcPs_T *nfc;
    u32 intrStatus;
    
    nfc = (FNfcPs_T*)instancePtr;
    
    intrStatus = FNfcPs_GetIntrStatus(nfc) & FNfcPs_GetIntrMask(nfc);
    FNfcPs_ClearIntrStatus(nfc); 
    
    if(intrStatus & NFCPS_INTR_ECC_INT0_MASK)
    {
        nfc->statusHandler(nfc->statusRef, 
                           NFCPS_INTR_ECC_INT0_MASK, 
                           0);
    }
    if(intrStatus & NFCPS_INTR_STAT_ERR_INT0_MASK)
    {
        nfc->statusHandler(nfc->statusRef, 
                           NFCPS_INTR_STAT_ERR_INT0_MASK, 
                           0);
    }
    if(intrStatus & NFCPS_INTR_MEM0_RDY_INT_MASK)
    {
        nfc->statusHandler(nfc->statusRef, 
                           NFCPS_INTR_MEM0_RDY_INT_MASK, 
                           0);
    }
    if(intrStatus & NFCPS_INTR_PG_SZ_ERR_INT_MASK)
    {
        nfc->statusHandler(nfc->statusRef, 
                           NFCPS_INTR_PG_SZ_ERR_INT_MASK, 
                           0);
    }
    if(intrStatus & NFCPS_INTR_SS_READY_INT_MASK)
    {
        nfc->statusHandler(nfc->statusRef, 
                           NFCPS_INTR_SS_READY_INT_MASK, 
                           0);
    }
    if(intrStatus & NFCPS_INTR_TRANS_ERR_MASK)
    {
        nfc->statusHandler(nfc->statusRef, 
                           NFCPS_INTR_TRANS_ERR_MASK, 
                           0);
    }
    if(intrStatus & NFCPS_INTR_DMA_INT_MASK)
    {
        nfc->statusHandler(nfc->statusRef, 
                           NFCPS_INTR_DMA_INT_MASK, 
                           0);
    }
    if(intrStatus & NFCPS_INTR_DATA_REG_MASK)
    {
        nfc->statusHandler(nfc->statusRef, 
                           NFCPS_INTR_DATA_REG_MASK, 
                           0);
    }
    if(intrStatus & NFCPS_INTR_CMD_END_INT_MASK)
    {
        nfc->statusHandler(nfc->statusRef, 
                           NFCPS_INTR_CMD_END_INT_MASK, 
                           0);
    }
    if(intrStatus & NFCPS_INTR_PORT_INT_MASK)
    {
        nfc->statusHandler(nfc->statusRef, 
                           NFCPS_INTR_PORT_INT_MASK, 
                           0);
    }
    
}
