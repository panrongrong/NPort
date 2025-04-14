/***************************** Include Files *********************************/

#include <string.h>
#include "../../common/fmsh_common.h"/*#include "fmsh_common.h"*/
/*#include "fmsh_gic.h"*/
#include "../../slcr/fmsh_slcr.h" /*#include "fmsh_slcr.h"*/
#include "fmsh_spips_lib.h"

/************************** Constant Definitions *****************************/

/*
* The following constants map to the XPAR parameters created in the
* xparameters.h file. They are defined here such that a user can easily
* change all the needed parameters in one place.
*/
#define SPI_DEVICE_ID		FPS_SPI0_DEVICE_ID
#define INTC_DEVICE_ID		0
#define SPI_INTR_ID		    SPI0_INTR_ID

/*
* Flash data is to be written.
*/
#define UNIQUE_VALUE		0x05

/*
* The following constants specify the max amount of data and the size of the
* the buffer required to hold the data and overhead to transfer the data to
* and from the FLASH.
*/
#define MAX_DATA		4/*32*/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/
static int FSpiPs_setupIntrSystem(/*FGicPs* intcPtr,*/ FSpiPs_T* spiPtr, u16 spiIntrId);
/*static void FSpiPs_disableIntrSystem(FGicPs*intcPtr, u16 spiIntrId);*/
static void FSpiPs_handler(void *callBackRef, u32 statusEvent, u32 byteCount);

/************************** Variable Definitions *****************************/
/*
* The following variables are used to read and write to the flash and they
* are global to avoid having large buffers on the stack
*/
static u8 ReadBuffer[MAX_DATA];
static u8 WriteBuffer[MAX_DATA];

static FSpiPs_T spi0, spi1;

SPI_CAPS(spi_polled_example) = {
    1,               /*.isMaster =    */
    1,               /*.loop =    */
    0,               /*.hasIntr =    */
    0,               /*.hasDma =    */
    10,               /*.txEmptyLvl =    */
    0,               /*.rxFullLvl =    */
    SPIPS_TRANSFER_STATE,               /*.tsfMode =    */
    0,               /*.cpol =    */
    0,               /*.cpha =    */
    8,               /*.frameSize =    */
    128,             /*.frameLen =    */
    40,              /*.baudRate =    */
    0,               /*.sampleDelay =    */
};

int SpiPolledExample(FSpiPs_T* spiPtr, u16 spiDeviceId)
{
    int status;
    FSpiPs_Config_T* spiConfig;
    u32 uniqueValue, count;  
    
    /*
    * Initialize the SPI driver so that it's ready to use
    */
    spiConfig = FSpiPs_LookupConfig(spiDeviceId);
    if (NULL == spiConfig) 
    {
        return FMSH_FAILURE;
    }
    status = FSpiPs_CfgInitialize(spiPtr, spiConfig);
    if (status != FMSH_SUCCESS) 
    {
        return FMSH_FAILURE;
    }

    FSpiPs_HwInit(spiPtr, GET_SPI_CAPS(spi_polled_example));
    FSpiPs_SetSlave(spiPtr, 1);
    FSpiPs_SetEnable(spiPtr, 1);
    
    /*
    * Initialize the write buffer for a pattern to write to the FLASH
    * and the read buffer to zero so it can be verified after the read, the
    * test value that is added to the unique value allows the value to be
    * changed in a debug environment to guarantee
    */
    for (uniqueValue = UNIQUE_VALUE, count = 0; count < MAX_DATA; count++)
    {
        WriteBuffer[count] = (u8)(uniqueValue);
        uniqueValue++;
    }
            
    FSpiPs_PolledTransfer(spiPtr, WriteBuffer, ReadBuffer, MAX_DATA);
    
    /*
    * Setup a pointer to the start of the data that was read into the read
    * buffer and verify the data read is the data that was written
    */
    for (uniqueValue = UNIQUE_VALUE, count = 0; count < MAX_DATA; count++) 
    {
        if (ReadBuffer[count] != (u8)(uniqueValue))
        {
            return FMSH_FAILURE;
        }
        uniqueValue++;
    }
  
    return FMSH_SUCCESS;
}

SPI_CAPS(spi_intr_example) = {
    1,          /*.isMaster =   */
    1,          /*.loop =   */
    1,          /*.hasIntr =   */
    0,          /*.hasDma =   */
    10,         /*.txEmptyLvl =   */
    0,          /*.rxFullLvl =   */
    SPIPS_TRANSFER_STATE,          /*.tsfMode =   */
    0,          /*.cpol =   */
    0,          /*.cpha =   */
    8,          /*.frameSize =   */
    128,        /*.frameLen =   */
    40,         /*.baudRate =   */
    0,          /*.sampleDelay =   */
};

int FSpiPs_intrExampleInit(FSpiPs_T* spiPtr, u16 spiDeviceId, u16 spiIntrId)
{
  int status;
  FSpiPs_Config_T* spiConfig;
  u32 uniqueValue, count;  
  int time = 0;
  /*
  * Initialize the SPI driver so that it's ready to use
  */
  spiConfig = FSpiPs_LookupConfig(spiDeviceId);
  if (NULL == spiConfig) 
  {
    return FMSH_FAILURE;
  }
  status = FSpiPs_CfgInitialize(spiPtr, spiConfig);
  if (status != FMSH_SUCCESS) 
  {
    return FMSH_FAILURE;
  }
  
/*  FSpiPs_SetLoopBack(spiPtr,TRUE);*/
  /*enable selfloop mode*/
/*  FSpiPs_Disable(spiPtr);*/
  FSpiPs_SetLoopBack(spiPtr, TRUE);
/*  FSpiPs_Enable(spiPtr); */
  
 /*
  * Initialize SPI Interrupt
  */
  FSpiPs_setupIntrSystem(/*intcPtr,*/ spiPtr, spiIntrId);
/*  DEBUG_LOG("spiPtr->config.baseAddress = 0X%X\n",spiPtr->config.baseAddress);*/
  FSpiPs_HwInit(spiPtr, GET_SPI_CAPS(spi_intr_example));
  FSpiPs_SetSlave(spiPtr, 1);
  FSpiPs_SetEnable(spiPtr, 1);

  taskDelay(sysClkRateGet()); /* WAIT 1 SECOND */

  
/*  FSpiPs_disableIntrSystem(intcPtr, spiIntrId);*/

  return FMSH_SUCCESS;  
}

int FSpiPs_intrExampleTrans( FSpiPs_T* spiPtr, u16 database, u16 spiIntrId)
{
	  int status;
	  FSpiPs_Config_T* spiConfig;
	  u32 uniqueValue, count;  
	  int time = 0;

	  /*
	  * Initialize the write buffer for a pattern to write to the FLASH
	  * and the read buffer to zero so it can be verified after the read, the
	  * test value that is added to the unique value allows the value to be
	  * changed in a debug environment to guarantee
	  */
	  for (uniqueValue = database, count = 0; count < MAX_DATA; count++)
	  {
	    WriteBuffer[count] = (u8)(uniqueValue);
	    uniqueValue++;
	    DEBUG_LOG("WriteBuffer[%d]=0x%x \n",count,WriteBuffer[count]);
	  }
	  int k =0;
	 /* for(k=0;k<2;k++)*/
	 /* {*/
	  status = FSpiPs_Transfer(spiPtr, WriteBuffer, ReadBuffer, MAX_DATA);
	  if (status != FMSH_SUCCESS) 
	  {
	    /*DEBUG_LOG("FSpiPs_Transfer ERROR\n");*/
	    return FMSH_FAILURE;
	  }
	  else
	  {
	    /*DEBUG_LOG("FSpiPs_Transfer OK\n");*/
	  }
	  while(spiPtr->isBusy == TRUE)
	  {
	    /*delay_ms(1);*/
		  taskDelay(1);
	    time++;
	    if(time > 1000)
	    {
	    	DEBUG_LOG("isBusy timeout\n");
	      spiPtr->isBusy = FALSE;
	      break;
	    }      
	    /*DEBUG_LOG("delay_ms(1);\n");*/
	  }
	  
	  /*
	  * Setup a pointer to the start of the data that was read into the read
	  * buffer and verify the data read is the data that was written
	  */
	  for (uniqueValue = database, count = 0; count < MAX_DATA; count++) 
	  {
		 DEBUG_LOG("ReadBuffer[%d]=0x%x \n",count,ReadBuffer[count]);
	    if (ReadBuffer[count] != (u8)(uniqueValue))
	    {
	      return FMSH_FAILURE;
	    }
	    uniqueValue++;
	  }
	  return FMSH_SUCCESS;
}



/*****************************************************************************/
/**
*
* This function setups the interrupt system for an Qspi device.
*
* @param	intcPtr is a pointer to the instance of the Intc device.
* @param	QspiInstancePtr is a pointer to the instance of the Qspi device.
* @param	QspiIntrId is the interrupt Id for an QSPI device.
*
* @return	XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @note		None.
*
******************************************************************************/
int FSpiPs_setupIntrSystem(/*FGicPs* intcPtr,*/ FSpiPs_T* spiPtr, u16 spiIntrId)
{
  int status;
  static int FSpi_intr_isr_init_flag = 0;
  if(FSpi_intr_isr_init_flag)
  {
	  /*DEBUG_LOG("spi interrupt isr has initialized before\n");*/
	  return FMSH_SUCCESS;  
  }

/*  FGicPs_Config*intcConfig; /* Instance of the interrupt controller */
  
  /*
  * Initialize the interrupt controller driver so that it is ready to
  * use.
  */
/*  intcConfig = FGicPs_LookupConfig(INTC_DEVICE_ID);
  if (NULL == intcConfig) 
  {
    return FMSH_FAILURE;
  }
*/
/*
  status = FGicPs_CfgInitialize(intcPtr, intcConfig, intcConfig->CpuBaseAddress);
  if (status != FMSH_SUCCESS) 
  {
    return FMSH_FAILURE;
  }
/*
  /*
  * Connect the device driver handler that will be called when an
  * interrupt for the device occurs, the handler defined above performs
  * the specific interrupt processing for the device.
  */
/*
  status = FGicPs_Connect(intcPtr, spiIntrId, 
                          (FMSH_InterruptHandler)FSpiPs_InterruptHandler,
                          (void *)spiPtr);
  if (status != FMSH_SUCCESS) {
    return status;
  }
*/
	intConnect(INUM_TO_IVEC(INT_VEC_SPIINT0), FSpiPs_InterruptHandler, spiPtr);
	intEnable(INT_VEC_SPIINT0);
	FSpi_intr_isr_init_flag = 1;
	FSpiPs_SetStatusHandler(spiPtr, spiPtr, FSpiPs_handler);
  
  /*
  * Enable the interrupt for the Qspi device.
  */
/*  FGicPs_Enable(/*intcPtr, INT_VEC_SPIINT0 /*piIntrId);*/
	FGicPs_Enable(SPI0_INT_ID);	
  return FMSH_SUCCESS;    
}

/*****************************************************************************/
/**
*
* This function disables the interrupts that occur for the Qspi device.
*
* @param	intcPtr is the pointer to an INTC instance.
* @param	QspiIntrId is the interrupt Id for an QSPI device.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
#if 0
static void FSpiPs_disableIntrSystem(FGicPs* intcPtr, u16 spiIntrId)
{
  /*
  * Disable the interrupt for the QSPI device.
  */
  FGicPs_Disable(intcPtr, spiIntrId);
  
  /*
  * Disconnect and disable the interrupt for the Qspi device.
  */
  FGicPs_Disconnect(intcPtr, spiIntrId);
}
#endif
/*****************************************************************************/
/**
*
* This function disables the interrupts that occur for the Qspi device.
*
* @param	intcPtr is the pointer to an INTC instance.
* @param	QspiIntrId is the interrupt Id for an QSPI device.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
static void FSpiPs_handler(void *callBackRef, u32 statusEvent, u32 byteCount)
{
    FSpiPs_T* spiPtr = (FSpiPs_T*)callBackRef;
    DEBUG_LOG("in %s, st=0x%x\n",__FUNCTION__,statusEvent);
    if(statusEvent == SPIPS_INTR_MSTIS_MASK)
    {

    }
    if(statusEvent == SPIPS_INTR_RXOIS_MASK)
    {

    }
    if(statusEvent == SPIPS_INTR_RXUIS_MASK)
    {

    }    
    if(statusEvent == SPIPS_INTR_TXOIS_MASK)
    {

    }        
    if(statusEvent == SPIPS_INTR_TXEIS_MASK)
    {

    }
    if(statusEvent == SPIPS_INTR_RXFIS_MASK)
    {

    } 
    if(statusEvent == SPIPS_TRANSFER_DONE)
    {

    }  
}

/*****************************************************************************/
/**
*
* This function use low level function to transfer data from spi0 to spi1.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/

SPI_CAPS(spi_example_mst) = {
    1,           /*.isMaster =    */
    0,           /*.loop =    */
    1,           /*.hasIntr =    */
    0,           /*.hasDma =    */
    10,          /*.txEmptyLvl =    */
    0,           /*.rxFullLvl =    */
    SPIPS_TRANSFER_STATE,           /*.tsfMode =    */
    0,           /*.cpol =    */
    0,           /*.cpha =    */
    8,           /*.frameSize =    */
    128,         /*.frameLen =    */
    40,          /*.baudRate =    */
    0,           /*.sampleDelay =    */
};

SPI_CAPS(spi_example_slv) = {
    0,             /*.isMaster =     */
    0,             /*.loop =     */
    1,             /*.hasIntr =     */
    0,             /*.hasDma =     */
    10,             /*.txEmptyLvl =     */
    0,             /*.rxFullLvl =     */
    SPIPS_TRANSFER_STATE,             /*.tsfMode =     */
    0,             /*.cpol =     */
    0,             /*.cpha =     */
    8,             /*.frameSize =     */
    128,             /*.frameLen =     */
    0,             /*.sampleDelay =     */
};

int FSpiPs_example(void)
{
    FSpiPs_T* spi0Ptr;
    FSpiPs_T* spi1Ptr;
    int status = FMSH_SUCCESS;
    u32 data;
    
    spi0Ptr = &spi0;
    spi1Ptr = &spi1;
    
    /* enable loop */
    FSlcrPS_setSpiLoop(TRUE);
    
    /* spi0 master, spi1 slaver */
    FSpiPs_Initialize(spi0Ptr, FPS_SPI0_DEVICE_ID);
    FSpiPs_HwInit(spi0Ptr, GET_SPI_CAPS(spi_example_mst));
    FSpiPs_SetSlave(spi0Ptr, 1);
    FSpiPs_SetEnable(spi0Ptr, 1);
    
    FSpiPs_Initialize(spi1Ptr, FPS_SPI1_DEVICE_ID);
    FSpiPs_HwInit(spi1Ptr, GET_SPI_CAPS(spi_example_slv));  
    FSpiPs_SetEnable(spi1Ptr, 1);

    /* slave sends first, then master sends */
    FSpiPs_Send(spi1Ptr, 0x66);
    FSpiPs_Send(spi0Ptr, 0xaa); 
    delay_ms(1);
    data = FMSH_ReadReg(spi0Ptr->config.baseAddress, SPIPS_DR_OFFSET);
    if(data != 0x66) 
        status = FMSH_FAILURE;
    data = FMSH_ReadReg(spi1Ptr->config.baseAddress, SPIPS_DR_OFFSET);
    if(data != 0xaa) 
        status = FMSH_FAILURE;  
    
    FSpiPs_SetEnable(spi0Ptr, 0);
    FSpiPs_SetEnable(spi1Ptr, 0);
    /* disable loop */
    FSlcrPS_setSpiLoop(FALSE);
    
    return status;
}







/*////////////////////////////////////////////////////////////////////
//                   FOR TEST 
///////////////////////////////////////////////////////////////////*/

/*IMPORT void FGicPs_Enable(u32 Int_Id);*/


void FSpi_Intr_loop_test()
{
	int Status;
	int i =0;
	/* initialize */
	FSpiPs_intrExampleInit(&spi0, 0, INT_VEC_SPIINT0);
	
    /* enable loop */
/*	FSlcrPS_setSpiLoop(TRUE);*/
	FSpiPs_SetLoopBack(&spi0, TRUE);
	
	/* transfer */
	for(i =0;i<50;i++)
	{   FSpiPs_SetSlave(&spi0, 1);
		
		Status = FSpiPs_intrExampleTrans(&spi0, MAX_DATA*i, INT_VEC_SPIINT0);
		taskDelay(sysClkRateGet()*1);
	}
	
	
	if(Status == 0)
		DEBUG_LOG("SPI smoking test pass!\n");
	else
		DEBUG_LOG("SPI smoking test failed,please check!\n");
}

void FSpi_poll_loop_test()
{
	int Status;
	
    /* enable loop */
	FSlcrPS_setSpiLoop(TRUE);
/*	DEBUG_LOG("INT_VEC_SPIINT0 = %d\n",INT_VEC_SPIINT0);*/
#if 1
	Status = FSpiPs_example();
/*	Status = SpiPolledExample(&spi0,0);*/
	if(Status == 0)
		DEBUG_LOG("SPI smoking poll test pass!\n");
	else
		DEBUG_LOG("SPI smoking poll test failed,please check!\n");
#endif
}
