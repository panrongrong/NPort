#include <vxWorks.h>
#include <intLib.h>
#include <logLib.h>
#include <muxLib.h>
#include <semLib.h>
#include <sysLib.h>
#include <vxBusLib.h>
#include <wdLib.h>
#include <string.h>
  
#include "../../common/fmsh_ps_parameters.h"
#include "../../slcr/fmsh_slcr.h"

#include "fmsh_qspips_lib.h"

static void StubStatusHandler_qspi(void *callBackRef, u32 statusEvent, unsigned byteCount);


/*****************************************************************************
* This function initializes a specific FQspiPs_T device/instance. This function
* must be called prior to using the device to read or write any data.
*
* @param	spi is a pointer to the FQspiPs_T instance.
* @param	configPtr points to the FQspiPs_T device configuration structure.
*
* @return
*		- FMSH_SUCCESS if successful.
*		- FMSH_FAILURE if fail.
*
* @note		The user needs to first call the FQspiPs_LookupConfig() API
*		which returns the Configuration structure pointer which is
*		passed as a parameter to the FQspiPs_CfgInitialize() API.
*
******************************************************************************/
int FQspiPs_CfgInitialize(FQspiPs_T *qspi, FQspiPs_Config_T *configPtr)
{
    FMSH_ASSERT(qspi != NULL);
    FMSH_ASSERT(configPtr != NULL);        
    
    /* set default value */
    qspi->config = *configPtr;
    qspi->flag = 0;
    qspi->maker = 0;
    qspi->devSize = 0;
    qspi->sectorSize = QSPIFLASH_SECTOR_256K;
    qspi->isBusy = FALSE;        
    qspi->sendBufferPtr = NULL;
    qspi->recvBufferPtr = NULL;
    qspi->requestedBytes = 0;
    qspi->remainingBytes = 0;
    qspi->statusHandler = StubStatusHandler_qspi;
    
        
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function resets qspi device registers to default value.
*
* @param
*
* @return
*
* @note		
*
******************************************************************************/
void FQspiPs_Reset(FQspiPs_T* qspi)
{    
    if(qspi->config.deviceId == FPS_QSPI0_DEVICE_ID)
    {
        FSlcrPs_ipSetRst(SLCR_QSPI0_CTRL, QSPI_APB_RST);
        FSlcrPs_ipSetRst(SLCR_QSPI0_CTRL, QSPI_AHB_RST);
        FSlcrPs_ipSetRst(SLCR_QSPI0_CTRL, QSPI_REF_RST);
        FSlcrPs_ipReleaseRst(SLCR_QSPI0_CTRL, QSPI_APB_RST);
        FSlcrPs_ipReleaseRst(SLCR_QSPI0_CTRL, QSPI_AHB_RST);
        FSlcrPs_ipReleaseRst(SLCR_QSPI0_CTRL, QSPI_REF_RST);
    }
    else if(qspi->config.deviceId == FPS_QSPI1_DEVICE_ID)
    {
        FSlcrPs_ipSetRst(SLCR_QSPI1_CTRL, QSPI_APB_RST);
        FSlcrPs_ipSetRst(SLCR_QSPI1_CTRL, QSPI_AHB_RST);
        FSlcrPs_ipSetRst(SLCR_QSPI1_CTRL, QSPI_REF_RST);
        FSlcrPs_ipReleaseRst(SLCR_QSPI1_CTRL, QSPI_APB_RST);
        FSlcrPs_ipReleaseRst(SLCR_QSPI1_CTRL, QSPI_AHB_RST);
        FSlcrPs_ipReleaseRst(SLCR_QSPI1_CTRL, QSPI_REF_RST);
    }
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
void FQspiPs_SetStatusHandler(FQspiPs_T* qspi, void* callBackRef, FQspiPs_StatusHandler funcPtr)
{
    FMSH_ASSERT(qspi != NULL);
    FMSH_ASSERT(funcPtr != NULL);
    
    qspi->statusHandler = funcPtr;
    qspi->statusRef = callBackRef;
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
static void StubStatusHandler_qspi(void *callBackRef, u32 statusEvent, unsigned byteCount)
{
    (void) callBackRef;
    (void) statusEvent;
    (void) byteCount;
}

/*****************************************************************************
* This function tests if spi device exists.
*
* @param
*
* @return
*		- FMSH_SUCCESS if qspi device exists.
*		- FMSH_FAILURE if qspi device not exists.
*
* @note		
*
******************************************************************************/
int FQspiPs_SelfTest(FQspiPs_T* qspi)
{
    u8 delayCSDA, delayCSDADS, delayCSEOT, delayCSSOT;
    delayCSDA = 0x5A;
    delayCSDADS = 0xA5;
    delayCSEOT = 0xAA;
    delayCSSOT = 0x55;
    
    FQspiPs_SetDelays(qspi, delayCSDA, delayCSDADS, delayCSEOT, delayCSSOT);
    FQspiPs_GetDelays(qspi, &delayCSDA, &delayCSDADS, &delayCSEOT, &delayCSSOT);
    if((0x5A != delayCSDA) || (0xA5 != delayCSDADS) || 
       (0xAA != delayCSEOT) || (0x55 != delayCSSOT))
    {
        return FMSH_FAILURE;
    }
    FQspiPs_SetDelays(qspi, 0, 0, 0, 0);
    return FMSH_SUCCESS;
}

/*****************************************************************************
* This function set controller read mode.
*
* @param
*       - cmd: Read command send to qspi flash.
*
* @return
*		- FMSH_SUCCESS if qspi device exists.
*		- FMSH_FAILURE if qspi device not exists.
*
* @note		
*
******************************************************************************/
int FQspiPs_SetFlashReadMode(FQspiPs_T* qspi, u8 cmd)
{
    u32 configReg;
    switch(cmd)
    {
    case READ_CMD:  configReg = 0x00000003; break;
    case FAST_READ_CMD: configReg = 0x0800000B; break;
    case DOR_CMD: configReg = 0x0801003B; break;
    case QOR_CMD: configReg = 0x0802006B; break;
    case DIOR_CMD:         
        if(qspi->maker == SPI_MICRON_ID)
            configReg = 0x080110BB;
        else
            configReg = 0x040110BB;   
        break;
    case QIOR_CMD:
        if(qspi->maker == SPI_MICRON_ID)
            configReg = 0x0A0220EB;
        else
            configReg = 0x041220EB;   
        break;
    default: return FMSH_FAILURE;
    }
    
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_DRIR_OFFSET, configReg);
    return FMSH_SUCCESS;
}

void FQspiPs_InterruptHandler(void* instancePtr)
{
    FQspiPs_T* qspi = (FQspiPs_T*)instancePtr;
    u32 intrStatus;
    u32 byteCount;
    
    /* read & clear interrupt */
    intrStatus = FMSH_ReadReg(qspi->config.baseAddress, QSPIPS_ISR_OFFSET);  
    FMSH_WriteReg(qspi->config.baseAddress, QSPIPS_ISR_OFFSET, QSPIPS_INTR_ALL);
    
    /* legacy mode */ 
    /* rx fifo can be read */
    if ((intrStatus & QSPIPS_INTR_RX_NOT_EMPTY_MASK) || 
        (intrStatus & QSPIPS_INTR_RX_FULL_MASK))
    {
        byteCount = qspi->requestedBytes - qspi->remainingBytes;
        qspi->statusHandler(qspi->statusRef, 
                            QSPIPS_INTR_RX_NOT_EMPTY_MASK | QSPIPS_INTR_RX_FULL_MASK, 
                            byteCount);
    }
    /* tx fifo can be written */
    if (intrStatus & QSPIPS_INTR_TX_NOT_FULL_MASK)
    {
        byteCount = qspi->requestedBytes - qspi->remainingBytes;
        qspi->statusHandler(qspi->statusRef, 
                            QSPIPS_INTR_TX_NOT_FULL_MASK, 
                            byteCount);    
    }
    /* tx fifo full */
    if (intrStatus & QSPIPS_INTR_TX_FULL_MASK)
    {
        byteCount = qspi->requestedBytes - qspi->remainingBytes;
        qspi->statusHandler(qspi->statusRef, 
                            QSPIPS_INTR_TX_FULL_MASK, 
                            byteCount);          
    }
    /* rx fifo overflow */
    if (intrStatus & QSPIPS_INTR_RX_OVERFLOW_MASK)
    {
        byteCount = qspi->requestedBytes - qspi->remainingBytes;
        qspi->statusHandler(qspi->statusRef, 
                            QSPIPS_INTR_RX_OVERFLOW_MASK, 
                            byteCount);          
    }
    
    if (intrStatus & QSPIPS_INTR_TX_UNDERFLOW_MASK)
    {
        byteCount = qspi->requestedBytes - qspi->remainingBytes;
		qspi->statusHandler(qspi->statusRef, 
                            QSPIPS_INTR_TX_UNDERFLOW_MASK, 
                            byteCount);  
    }
    
    /* request rejected */
    /* 2 requests already in queue */
    if (intrStatus & QSPIPS_INTR_REQ_REJECT_MASK)
    {            
        byteCount = qspi->requestedBytes - qspi->remainingBytes;
        qspi->isBusy = FALSE;
        qspi->statusHandler(qspi->statusRef, 
                            QSPIPS_INTR_REQ_REJECT_MASK, 
                            byteCount);          
    }
    /* try to write data to protected address */
    if (intrStatus & QSPIPS_INTR_WRITE_PROTECT_MASK)
    {
        byteCount = qspi->requestedBytes - qspi->remainingBytes;
        qspi->isBusy = FALSE;
        qspi->statusHandler(qspi->statusRef, 
                            QSPIPS_INTR_WRITE_PROTECT_MASK, 
                            byteCount);         
    }
    /* ahb access error */
    if (intrStatus & QSPIPS_INTR_ILLEGAL_AHB_ACCESS_MASK)
    {
        byteCount = qspi->requestedBytes - qspi->remainingBytes;
        qspi->isBusy = FALSE;
        qspi->statusHandler(qspi->statusRef, 
                            QSPIPS_INTR_ILLEGAL_AHB_ACCESS_MASK, 
                            byteCount);          
    }
    /* poll expired */
    if (intrStatus & QSPIPS_INTR_POLL_EXPIRED_MASK)
    {
        byteCount = qspi->requestedBytes - qspi->remainingBytes;
        qspi->isBusy = FALSE;
        qspi->statusHandler(qspi->statusRef, 
                            QSPIPS_INTR_POLL_EXPIRED_MASK, 
                            byteCount);          
    }
    
    /* indirect mode transfer running */
    if ((intrStatus & QSPIPS_INTR_WATERMARK_LEVEL_BREACHED_MASK) || 
        (intrStatus & QSPIPS_INTR_SRAM_READ_FULL_MASK))
    {              
        byteCount = qspi->requestedBytes - qspi->remainingBytes;
        qspi->statusHandler(qspi->statusRef, 
                            QSPIPS_INTR_WATERMARK_LEVEL_BREACHED_MASK | 
                            QSPIPS_INTR_SRAM_READ_FULL_MASK, 
                            byteCount);              
    }  
    /* stig mode transfer complete*/
    if (intrStatus & QSPIPS_INTR_STIG_COMPLETE_MASK)
    {
        byteCount = qspi->requestedBytes - qspi->remainingBytes;
        qspi->isBusy = FALSE;
        qspi->statusHandler(qspi->statusRef, 
                            QSPIPS_INTR_STIG_COMPLETE_MASK, 
                            byteCount);          
    }
    /* indirect mode transfer complete*/
    if (intrStatus & QSPIPS_INTR_INDIRECT_COMPLETE_MASK)
    {
        byteCount = qspi->requestedBytes - qspi->remainingBytes;
        qspi->isBusy = FALSE;
        qspi->statusHandler(qspi->statusRef, 
                            QSPIPS_INTR_INDIRECT_COMPLETE_MASK, 
                            byteCount);          
    } 
}
