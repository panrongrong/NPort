/*#include "fmsh_i2c_verify.h"*/
#include "fmsh_i2c_driver.h"
#include "../../common/fmsh_ps_parameters.h"
/*#include "../../dma/dma_1_bare/fmsh_dmac_verify.h"*/


/****************************************
**   Global variables
*****************************************/

FI2cPs_T gI2c0_dev;
FI2cPs_T gI2c1_dev;
FIicPs_Instance_T gI2c0_Instance;
FIicPs_Instance_T gI2c1_Instance;

uint8_t I2C_Master_RxBuffer[I2C_BUFFER_SIZE];
uint8_t I2C_Master_TxBuffer[I2C_BUFFER_SIZE];

uint8_t I2C_Slave_RxBuffer[I2C_BUFFER_SIZE];
uint8_t I2C_Slave_TxBuffer[I2C_BUFFER_SIZE];

u16 I2C0_DMA_RxBuffer[I2C_BUFFER_SIZE];
u16 I2C0_DMA_TxBuffer[I2C_BUFFER_SIZE];

volatile bool masterWaitFlag;
volatile bool slaveWaitFlag;

static volatile u8 i2c0_rxBuffer_idx = 0;

/****************************************
**   Local variables
*****************************************/
/* jc*/
u32 i2c_finish=0;
volatile bool usrIrq=false;
volatile bool fastSpeed=true;

u32 I2C_DMA_Buffer[40];
u32 i2cDmaMode=0;



/****************************************
**   Local definitions
*****************************************/
#define AT24_ADDR    0x50

#define SLAVE_ADDRESS 0x5A

#define I2C0_TIMEOUTMAX   0x0F

/*
 * These minimum high and low times are in nanoseconds.  They represent
 * the minimum amount of time a bus signal must remain either high or
 * low to be interpreted as a logical high or low as per the I2C bus
 * protocol.  These values are used in conjunction with an I2C input
 * clock frequency to determine the correct values to be written to the
 * clock count registers.
 */
#define SS_MIN_SCL_HIGH         4000
#define SS_MIN_SCL_LOW          4700
#define FS_MIN_SCL_HIGH         600
#define FS_MIN_SCL_LOW          1300
#define HS_MIN_SCL_HIGH_100PF   60
#define HS_MIN_SCL_LOW_100PF    120


/*void test_i2c_master_callback(FI2cPs_T *ev,int32_t numBytes);*/

/*void test_i2c_slave_callback(FI2cPs_T *ev,int32_t numBytes);*/

/****************************************
** Local functions
*****************************************/
/* Utility function which programs the clock count registers for a given*/
/* input clock frequency.*/
void FI2cPs_clock_setup(FI2cPs_T *dev, unsigned ic_clk)
{
    uint16_t ss_scl_high, ss_scl_low;
    uint16_t fs_scl_high, fs_scl_low;
    uint16_t hs_scl_high, hs_scl_low;
 
    /* ic_clk is the clock speed (in MHz) that is being supplied to the*/
    /* DW_apb_i2c device.  The correct clock count values are determined*/
    /* by using this inconjunction with the minimum high and low signal*/
    /* hold times as per the I2C bus specification.*/
    ss_scl_high = ((uint16_t) (((SS_MIN_SCL_HIGH * ic_clk) / 1000) + 1));
    ss_scl_low = ((uint16_t) (((SS_MIN_SCL_LOW * ic_clk) / 1000) + 1));
    fs_scl_high = ((uint16_t) (((FS_MIN_SCL_HIGH * ic_clk) / 1000) + 1));
    fs_scl_low = ((uint16_t) (((FS_MIN_SCL_LOW * ic_clk) / 1000) + 1));
    hs_scl_high = ((uint16_t) (((HS_MIN_SCL_HIGH_100PF * ic_clk) / 1000)
                + 1));
    hs_scl_low = ((uint16_t) (((HS_MIN_SCL_LOW_100PF * ic_clk) / 1000) +
                1));

    FI2cPs_setSclCount(dev, I2c_speed_standard, I2c_scl_high,
            ss_scl_high);
    FI2cPs_setSclCount(dev, I2c_speed_standard, I2c_scl_low,
            ss_scl_low);
    FI2cPs_setSclCount(dev, I2c_speed_fast, I2c_scl_high, fs_scl_high);
    FI2cPs_setSclCount(dev, I2c_speed_fast, I2c_scl_low, fs_scl_low);
    FI2cPs_setSclCount(dev, I2c_speed_high, I2c_scl_high, hs_scl_high);
    FI2cPs_setSclCount(dev, I2c_speed_high, I2c_scl_low, hs_scl_low);
}

u8 Init_test_buffers()
{
  u32 i;

  for(i=0;i<I2C_BUFFER_SIZE;i++)
  { 
    I2C_Master_TxBuffer[i] = i*2+1;
    I2C_Master_RxBuffer[i] = 0;
    I2C_Slave_RxBuffer[i] = 0;
    I2C_Slave_TxBuffer[i]=i*2;
  }

  return 0;

}


/****************************************
**  Public functions
*****************************************/
u8 I2C0_device_init(
                    FI2cPs_T *pDev,
                    void* pI2cInstance,
                    void* I2cParam)
{
    u8 ret=FMSH_SUCCESS;     
    FI2cPs_Config* Config=NULL;
    Config = FI2cPs_LookupConfig(0);
    if(Config==NULL)
      return FMSH_FAILURE;
    ret=FI2cPs_init(pDev, Config,pI2cInstance,I2cParam);
    if(ret!=FMSH_SUCCESS)
      return ret;
    return ret;
}

u8 I2C1_device_init(
                      FI2cPs_T *pDev,
                      void* pI2cInstance,
                      void* I2cParam )
{
    u8 ret=FMSH_SUCCESS;     
    FI2cPs_Config* Config=NULL;
    Config = FI2cPs_LookupConfig(1);
    if(Config==NULL)
      return FMSH_FAILURE;
    ret=FI2cPs_init(pDev, Config,pI2cInstance,I2cParam);
    if(ret!=FMSH_SUCCESS)
      return ret;
    return ret;
}


/************************************************/
void I2C_master_init(FI2cPs_T *dev)
{
    /*disable the dev I2C device*/
    FI2cPs_disable(dev);
    
    FIicPs_PortMap_T *pPortmap = (FIicPs_PortMap_T *)dev -> base_address;
 
    /* Config  */  
    FMSH_OUT32_32(0x1,pPortmap->fs_spklen);  /*offset = 0xA0 IC_FS_SPKLEN*/
    /*FMSH_OUT32_32(0x5,pPortmap->reserved1);*/
    /* Set up the clock count register.  The argument I2C1_CLOCK is
     specified as the I2C dev input clock.*/
  
    FI2cPs_clock_setup(dev, (dev->input_clock)/1000000);
    
    /* set the speed mode to standard*/
    FI2cPs_setSpeedMode(dev, I2c_speed_standard);
   /* FI2cPs_setSpeedMode(master, I2c_speed_high);*/
    
    /* use 10-bit addressing*/
    FI2cPs_setMasterAddressMode(dev, I2c_7bit_address);
    FI2cPs_setSlaveAddressMode(dev, I2c_7bit_address);
    
    /* enable restart conditions*/
    FI2cPs_enableRestart(dev);
    
    /* enable master FSM*/
    FI2cPs_enableMaster(dev);
    /* Use the start byte protocol with the target address when*/
    /* initiating transfer.*/
   /* FI2cPs_setTxMode(dev, I2c_tx_gen_call);*/
   FI2cPs_setTxMode(dev, I2c_tx_target);
  /*  FI2cPs_setTxMode(dev, I2c_tx_start_byte);*/
    
    /* set target address to the I2C slave address*/
    FI2cPs_setTargetAddress(dev, SLAVE_ADDRESS);
    
    /*Set the user listener function*/
    FI2cPs_setListener(dev, I2C0_Master_Listener);

    /* clear Irq */
    FI2cPs_clearIrq(dev,I2c_irq_all);
    
    /*enable the dev I2C device*/
   /* FI2cPs_enable(dev);*/
 
}

void I2C_slave_init(FI2cPs_T *dev)
{
    /*disable the dev I2C device*/
    FI2cPs_disable(dev);
  
    FIicPs_PortMap_T *pPortmap = (FIicPs_PortMap_T *)dev -> base_address;
 
    /* Config  */
    FMSH_OUT32_32(0x1D7,pPortmap->reserved3[12]);  /*offset = C4*/
    
    /* Set up the clock count register.  The argument I2C1_CLOCK is
     specified as the I2C dev input clock.*/
    FI2cPs_clock_setup(dev, (dev->input_clock)/1000000);
    
    /* set the speed mode to standard*/
    FI2cPs_setSpeedMode(dev, I2c_speed_standard);
    
    /* use 7-bit addressing*/
    FI2cPs_setMasterAddressMode(dev, I2c_7bit_address);
    FI2cPs_setSlaveAddressMode(dev, I2c_7bit_address);
   
    /* enable restart conditions*/
    FI2cPs_enableRestart(dev);
    
    /* enable slave FSM*/
    FI2cPs_enableSlave(dev);
    FI2cPs_disableMaster(dev);
    
    /* set target address to the I2C slave address*/
    FI2cPs_setSlaveAddress(dev, SLAVE_ADDRESS);
    
    /*Set the user listener function*/
    FI2cPs_setListener(dev, I2C0_Slave_Listener);
    
    /* clear Irq */
   /* FI2cPs_clearIrq(dev,I2c_irq_all);*/
    
    
 
}

int FMSH_i2c_slaveTransmit(FI2cPs_T*dev, uint8_t*buffer,
        unsigned length, FMSH_callback userFunction)
{
    uint8_t*tmp;
    int retval;
    FIicPs_PortMap_T*portmap;
    FIicPs_Instance_T*instance;

    I2C_COMMON_REQUIREMENTS(dev);
    FMSH_REQUIRE(buffer != NULL);
    
    if(i2cDmaMode!=3)/*TEST_DMA_SLAVE_TX_MODE*/
      FMSH_REQUIRE(length != 0);
    
    portmap = (FIicPs_PortMap_T*) dev->base_address;
    instance = (FIicPs_Instance_T*) dev->instance;

    FMSH_REQUIRE(instance->listener != NULL);

    /* Critical section of code.  Shared data needs to be protected.*/
    /* This macro disables FMSH_apb_i2c interrupts.*/
    I2C_ENTER_CRITICAL_SECTION();

    if((instance->state == I2c_state_rd_req) || (instance->state ==
                I2c_state_slave_rx_rd_req)) {
        /* slave-transmitter*/
        retval = 0;

        instance->txCallback = userFunction;
        instance->txLength = length;
        instance->txRemain = length;
        instance->txHold = 0;
        instance->txIdx = 0;
        /* check for non word-aligned buffer as I2C_FIFO_WRITE() works*/
        /* efficiently on words reads from instance->txHold.*/
        tmp = (uint8_t*) buffer;
        while(((((unsigned) tmp) & 0x3) != 0x0) && ((instance->txLength
                        - instance->txIdx) > 0)) {
            instance->txHold |= ((*tmp++ & 0xff) << (instance->txIdx*
                        8));
            instance->txIdx++;
        }
        /* buffer is now word-aligned*/
        instance->txBuffer = tmp;
        /* write only one byte of data to the slave tx fifo*/
        if(i2cDmaMode!=3) /*TEST_DMA_SLAVE_TX_MODE*/
            I2C_FIFO_WRITE(1);
     
        switch(instance->state) {
            case I2c_state_rd_req:
                instance->state = I2c_state_slave_tx;
                break;
            case I2c_state_slave_rx_rd_req:
                instance->state = I2c_state_slave_tx_rx;
                break;
            default:
                FMSH_ASSERT(false);
                break;
        }
        /* Note: tx_empty is not enabled here as rd_req is the signal*/
        /* used to write the next byte of data to the tx fifo.*/
    }
    else
        retval = -FMSH_EPROTO;

    /* End of critical section of code. This macros restores FMSH_apb_i2c*/
    /* interrupts.*/
    I2C_EXIT_CRITICAL_SECTION();

    return retval;
}


/*-----------------------------------------------------------------
* Interrupt Handler
*-------------------------------------------------------------------*/
/* A user listener function for the master device.*/
void I2C0_Master_Listener(void *device, int32_t ecode)
{
    /* variable to store the reason(s) for transfer aborts*/
    enum FI2cPs_tx_abort txAbort;
    FI2cPs_T *dev=(FI2cPs_T*)device;
    /* Note that the I2c_irq_tx_empty and I2c_irq_rx_done interrupts are*/
    /* always handled internally by the driver and are never passed to*/
    /* the listener function.  Also note that the interrupt handler is*/
    /* responsible for the clearing of all interrupts, which it does*/
    /* after calling the user listener function.*/
    switch(ecode) {
        case I2c_irq_rx_under:
            /* Master receiver FIFO underflow.*/
            printf("*** Master: Rx FIFO underflow ***\n");
            /*FMSH_ASSERT(false);*/
            break;
        case I2c_irq_rx_over:
            /* Master receiver FIFO overflow.*/
            printf("*** Master: Rx FIFO overflow ***\n");
            /*FMSH_ASSERT(false);*/
            break;
        case I2c_irq_rx_full:
             i2c0_rxBuffer_idx++;
             /*I2C0_RxBuffer[i2c0_rxBuffer_idx] = FI2cPs_read(dev);*/
              /*FI2cPs_masterReceive(dev,I2C_Master_RxBuffer,10,test_i2c_master_callback);*/
            break;
        case I2c_irq_tx_over:
            /* Master transmitter FIFO overflow.*/
            printf("*** Master: Tx FIFO overflow ***\n");
            /*FMSH_ASSERT(false);*/
            break;
        case I2c_irq_rd_req:
            /* In this example, the master device slave FSM is disabled.*/
            /* Therefore, this interrupt should never be triggered as*/
            /* the master device initiates all transfers.*/
            printf("\n*** Master: unexpected read request ***\n");
           /* FMSH_ASSERT(false);*/
            break;
        case I2c_irq_tx_abrt:
            /* Find out the reason for the transfer abort.*/
            txAbort = FI2cPs_getTxAbortSource(dev);
            /* Print an error message with the value of the*/
            /* tx_abort_source register.*/
            printf("*** Master tx abort: 0x%04x ***\n", (uint16_t)
                    txAbort);
            /*recovery scl*/
            if(txAbort&(1<<17)!=0)
            {
              FI2cPs_sdaStuckRecoveryEnable(dev);
              /*not recovered*/
              if(FI2cPs_isSdaStuckNotRecovery(dev))
                  printf("*** Master sda stuck is not recovered");
              else
                  printf("*** Master sda stuck is recovered");
            } 
            break;
        case I2c_irq_activity:
            /* This interrupt is disabled by default by the driver*/
            /* (in the FI2cPs_setListener function).  It is triggered*/
            /* whenever there is activity on the I2C bus.*/
            break;
        case I2c_irq_stop_det:
            /* This interrupt is disabled by default by the driver*/
            /* (in the FI2cPs_setListener function).  It is triggered*/
            /* whenever a stop condition is detected on the I2C bus.*/
            break;
        case I2c_irq_start_det:
            /* This interrupt is disabled by default by the driver*/
            /* (in the FI2cPs_setListener function).  It is triggered*/
            /* whenever a start condition is detected on the I2C bus.*/
            break;
        case I2c_irq_gen_call:
            /* In this example, the master device slave FSM is disabled.*/
            /* Therefore, this interrupt should never be triggered as*/
            /* the master device initiates all transfers.*/
            printf("\n*** Master: unexpected general call ***\n");
           /* FMSH_ASSERT(false);*/
            break;
        case I2c_scl_stuck_at_low:
          /*Abort tx, issue stop*/
            FI2cPs_masterAbort(dev);
            break;
        default:
            /* Print an error message if an unexpected value is passed*/
            /* as the argument to this function.*/
            printf("*** unexpected argument: 0x%x ***\n", ecode);
            printf("*** disabling master device... ");
            while(FI2cPs_disable(dev) == -FMSH_EBUSY);
            printf("***\n");
            /*FMSH_ASSERT(false);*/
            break;
    }
}

/* A user listener function for the slave device.*/
void I2C0_Slave_Listener(void *device, int32_t ecode)
{
    enum FI2cPs_tx_abort txAbort;
    FI2cPs_T *dev=(FI2cPs_T*)device;
    FIicPs_Instance_T *instance;
    u16 rxfifolvl;
    u16 i;
    
    instance = (FIicPs_Instance_T *) dev->instance;
/*    pBuffer = instance->rxBuffer;*/

    /* Note that the I2c_irq_tx_empty and I2c_irq_rx_done interrupts are*/
    /* always handled internally by the driver and are never passed to*/
    /* the listener function.  Also note that the interrupt handler is*/
    /* responsible for the clearing of all interrupts, which it does*/
    /* after calling the user listener function.*/
    switch(ecode) {
        case I2c_irq_rx_under:
            /* Slave receiver FIFO underflow.*/
            printf("*** Slave Rx FIFO underflow ***\n");
            break;
        case I2c_irq_rx_over:
            /* Slave receiver FIFO overflow.*/
            printf("*** Slave Rx FIFO overflow ***\n");
            break;
        case I2c_irq_rx_full:
            /* In this example, the slave device is configured to use*/
            /* the 20-byte rx_buffer for all receive transfers.  The*/
            /* Rx full interrupt is passed to the user listener function*/
            /* only if there is no receive buffer already set up to*/
            /* accept data.  The example slave callback function is*/
            /* called when 'master_writes' bytes have been received.*/
          
          /* FI2cPs_slaveReceive(dev, rx_buffer, master_writes,dwe_i2c_slave_callback);*/
           /* FI2cPs_slaveReceive(dev, I2C_Slave_RxBuffer, 10, test_i2c_slave_callback);*/
          
            rxfifolvl = FI2cPs_getRxFifoLevel(dev);
            for(i=0;i<rxfifolvl;i++)
            {
              I2C_Slave_RxBuffer[i2c0_rxBuffer_idx++] = FI2cPs_read(dev);
            }
 
            instance->state = I2c_state_idle;
        
            break;
        case I2c_irq_tx_over:
            /* Slave transmitter FIFO overflow.*/
            printf("*** Slave: Tx FIFO overflow ***\n");
            break;
        case I2c_irq_rd_req:
            /* In this example, the slave device is configured to use*/
            /* the 20-byte tx_buffer for all transmit transfers.  No*/
            /* callback function is called at the end of the transfer as*/
            /* none is specified (i.e. argument is NULL).*/
            /*FI2cPs_slaveTransmit(dev, I2C_Slave_Buffer, 10, NULL);*/
          
            /* TEST_DMA_SLAVE_TX_MODE*/
          if(i2cDmaMode==3)
          {
          /* jc*/
           /* FDmaPs_dmac_init(&g_DMA_dmac);*/
			
            /*FI2cPs_slaveTransmit(dev, I2C_Slave_TxBuffer, 0, NULL);*/
            FMSH_i2c_slaveTransmit(dev, I2C_Slave_TxBuffer, 0, NULL);
          }else{
           /*TEST_DMA_MASTER_RX_MODE*/
            if(i2cDmaMode==2)
             FI2cPs_slaveBulkTransmit(dev, I2C_Slave_TxBuffer, 40, NULL);
            else
             FI2cPs_slaveBulkTransmit(dev, I2C_Slave_TxBuffer, 10, NULL);
          }
         /*   FI2cPs_slaveBulkTransmit(dev, I2C_Slave_TxBuffer, 10, test_i2c_slave_callback);*/
           /*FI2cPs_write(dev,0x33);*/
            break;
        case I2c_irq_tx_abrt:
            /* Find out the reason for the transfer abort.*/
            txAbort = FI2cPs_getTxAbortSource(dev);
            /* Print an error message with the value of the*/
            /* tx_abort_source register.*/
            TRACE_OUT(DEBUG_OUT, "*** Slave tx abort: 0x%04x ***\n", (uint16_t)
                    txAbort);
            break;
        case I2c_irq_activity:
            /* This interrupt is disabled by default by the driver*/
            /* (in the FI2cPs_setListener function).  It is triggered*/
            /* whenever there is activity on the I2C bus.*/
            break;
        case I2c_irq_stop_det:
            /* This interrupt is disabled by default by the driver*/
            /* (in the FI2cPs_setListener function).  It is triggered*/
            /* whenever a stop condition is detected on the I2C bus.*/
            break;
        case I2c_irq_start_det:
            /* This interrupt is disabled by default by the driver*/
            /* (in the FI2cPs_setListener function).  It is triggered*/
            /* whenever a start condition is detected on the I2C bus.*/
            break;
        case I2c_irq_gen_call:
            /* In this example, the slave device is configured to use*/
            /* the 20-byte rx_buffer for all receive transfers.  The*/
            /* example slave callback function is called when*/
            /* 'master_writes' bytes have been received.*/
            
           FI2cPs_slaveReceive(dev, I2C_Slave_RxBuffer, 10, test_i2c_slave_callback);
            break;
        default:
            /* Print an error message if an unexpected value is passed*/
            /* as the argument to this function.*/
            printf("*** unexpected argument: 0x%x ***\n", ecode);
            printf("*** disabling slave device... ");
            while(FI2cPs_disable(dev) == -FMSH_EBUSY);
            printf("***\n");
            break;
    }
}



/****************************************
**  EEPROM Operations
*****************************************/
void EEPROM_Write(FI2cPs_T *master,
                  uint16_t iaddress,
                  uint8_t  byte)
{
    uint8_t bHaddr,bLaddr;
    
    bHaddr = (uint8_t)(iaddress >> 8);
    bLaddr = (uint8_t)iaddress;
   
    /* write internal address */
    FI2cPs_write(master,bHaddr);
    FI2cPs_write(master,bLaddr);
    
    FI2cPs_issueSTOP(master,byte);
  
}

uint8_t EEPROM_Read( FI2cPs_T *master,
                     uint16_t iaddress)
{
    uint8_t bHaddr,bLaddr;
    uint8_t bRtval = 0;
    /*uint16_t fifo_level=0;*/
    
    bHaddr = (uint8_t)(iaddress >> 8);
    bLaddr = (uint8_t)iaddress;
    
    /*dummy write*/
    FI2cPs_write(master,bHaddr);
    FI2cPs_write(master,bLaddr);
    /*Issue read*/
    FI2cPs_issueRead(master);

     
    while(FI2cPs_isRxFifoEmpty(master) == true);

    bRtval = FI2cPs_read(master);
    return bRtval;
       
}

/*------------------------------------------------------*/
/*   Test Proc*/
/*------------------------------------------------------*/
void test_i2c_master_callback(void *dev,int32_t numBytes)
{
  masterWaitFlag = false;

}

void test_i2c_slave_callback(void *dev,int32_t numBytes)
{
  slaveWaitFlag = false;
}

/*
*  /param 
   num: number of send bytes
*/

u8 test_i2c_irq_master_gen_call(FI2cPs_T *master,FI2cPs_T *slave,u8 num)
{
  u8 i,numErrors;
  /*u32 delaycnt=0;*/
  u32 master_writes = (u32)num;

  Init_test_buffers();
   /*check status of device*/
  if(FI2cPs_isEnabled(slave) == FALSE)
  {
    FI2cPs_enable(slave);
  }
  
  if(FI2cPs_isEnabled(master) == FALSE)
  {
    FI2cPs_enable(master);
  }

  masterWaitFlag = slaveWaitFlag = true;
  
  /******************************************************/
  /* Initiate the transfer. A callback function will be called when the
    last byte has been transmitted by the master device. */
  TRACE_OUT(DEBUG_OUT, "Issue Master-Transmit...\n");
  
  FI2cPs_masterTransmit(master,I2C_Master_TxBuffer,master_writes,test_i2c_master_callback );  
  FI2cPs_slaveReceive(slave, I2C_Slave_RxBuffer, master_writes, test_i2c_slave_callback);
  
  /* Wait until Master has finished the transfer */
  while(masterWaitFlag == true);
  TRACE_OUT(DEBUG_OUT, "Master-Transmit is done.\n");
  masterWaitFlag = true;

 
  /* Wait until the transfer has finished */
  /* The Slave_listener automatically responds to an Rx FIFO full Irq*/
  while(slaveWaitFlag == true);
  TRACE_OUT(DEBUG_OUT, "Slave-Receive is done.\n");  
 slaveWaitFlag = true;
 
  /*delay to  make sure slave interrupt first so that writer transmit fifo*/
  /*FI2cPs_issueRead(master);*/
  delay_ms(10);

  /* Issue Master-Receive          */
  TRACE_OUT(DEBUG_OUT, "Issue Master-Receive...\n"); 
  FI2cPs_masterReceive(master,I2C_Master_RxBuffer,master_writes,test_i2c_master_callback);
  while(masterWaitFlag == true);
  TRACE_OUT(DEBUG_OUT, "Master-Receive is done.\n");
  masterWaitFlag = true;  
  
  /* Abort tx,issue stop */
  /*FI2cPs_masterAbort(master);*/
  FI2cPs_issueSTOP(master,0);
  
  /*****************************************************/
  /* Check Master-Transimitter */
  /*Compare Master-Tx data and Slave-Rx data*/
  for(i = numErrors = 0; i<master_writes;i++)
  {
    if(I2C_Master_TxBuffer[i] != I2C_Slave_RxBuffer[i])
    {numErrors++;}
  }
  if(numErrors > 0)
  { 
    Init_test_buffers();
    return numErrors; 
  }
  
  /* Check Master-Receiver */
   for(i = numErrors = 0; i<master_writes;i++)
  {
    if(I2C_Slave_TxBuffer[i] != I2C_Master_RxBuffer[i])
    {numErrors++;}
  }

   Init_test_buffers();
   FI2cPs_resetRxInstance(slave);
   return numErrors; 

}
/*
*  /param 
   num: number of send bytes
*/

u8 test_i2c_irq_gen_call(FI2cPs_T *master,FI2cPs_T *slave,u8 num)
{
  u8 i,numErrors;
  /*u32 delaycnt=0;*/
  u32 master_writes = (u32)num;

  Init_test_buffers();
   /*check status of device*/
  if(FI2cPs_isEnabled(slave) == FALSE)
  {
    FI2cPs_enable(slave);
  }
  
  if(FI2cPs_isEnabled(master) == FALSE)
  {
    FI2cPs_enable(master);
  }

  masterWaitFlag = slaveWaitFlag = true;
  
  /******************************************************/
  /* Initiate the transfer. A callback function will be called when the
    last byte has been transmitted by the master device. */
  TRACE_OUT(DEBUG_OUT, "Issue Master-Transmit...\n");
  FI2cPs_masterTransmit(master,I2C_Master_TxBuffer,master_writes,test_i2c_master_callback ); 
  /* Wait until Master has finished the transfer */
  while(masterWaitFlag == true);
  TRACE_OUT(DEBUG_OUT, "Master-Transmit is done.\n");
  masterWaitFlag = true;

 /* FI2cPs_slaveReceive(slave, I2C_Slave_RxBuffer, master_writes, test_i2c_slave_callback);*/
  /* Wait until the transfer has finished */
  /* The Slave_listener automatically responds to an Rx FIFO full Irq*/
  while(slaveWaitFlag == true);
  TRACE_OUT(DEBUG_OUT, "Slave-Receive is done.\n");  
 slaveWaitFlag = true;
 
  /* Abort tx,issue stop */
  FI2cPs_masterAbort(master);

  /*****************************************************/
  /* Check Master-Transimitter */
  /*Compare Master-Tx data and Slave-Rx data*/
  for(i = numErrors = 0; i<master_writes;i++)
  {
    if(I2C_Master_TxBuffer[i] != I2C_Slave_RxBuffer[i])
    {numErrors++;}
  }
  if(numErrors > 0)
  { 
    Init_test_buffers();
    return numErrors; 
  }
  
   Init_test_buffers();
   FI2cPs_resetRxInstance(slave);
   return numErrors; 

}

u8 rst_i2c(void)
{
    FMSH_WriteReg(0xE0026000, 0x008, 0xDF0D767BU);/*unlock*/
    FMSH_WriteReg(0xE0026000,0x318,0x3);/**/
    delay_ms(100);
    FMSH_WriteReg(0xE0026000,0x318,0x0);/**/
    return 0;
}

int FMSH_i2c_userIrqHandler(FI2cPs_T *dev)
{
     bool retval;
    uint32_t reg;
    int32_t callbackArg;
    u32 fifo_level,i;
    u32 tmp=0;
    FMSH_callback userCallback;
    FIicPs_Irq_T clearIrqMask;
    FIicPs_PortMap_T*portmap;

   /* I2C_COMMON_REQUIREMENTS(dev);*/
    
    portmap = (FIicPs_PortMap_T *) dev->base_address;

    /* Assume an interrupt will be processed.  userCallback will be set*/
    /* to NULL and retval to false if no active interrupt is found.*/
    retval = true;
   /* userCallback = instance->listener;*/
    userCallback = NULL;
    callbackArg = 0;
    clearIrqMask = I2c_irq_none;

    /* what caused the interrupt?*/
    reg = I2C_INP(portmap->intr_stat);
    /*reg = I2C_INP(portmap->raw_intr_stat);*/
    /* if a tx transfer was aborted*/
    if((reg & I2c_irq_tx_abrt) != 0) {
        callbackArg = I2c_irq_tx_abrt;
        clearIrqMask = I2c_irq_tx_abrt;
        i2c_finish &= (~I2c_irq_tx_abrt);
        TRACE_OUT(DEBUG_OUT, "*** Irq: TX_ABRT...\n");
    }
    /* rx fifo overflow*/
    else if((reg & I2c_irq_rx_over) != 0) {
        callbackArg = I2c_irq_rx_over;
        clearIrqMask = I2c_irq_rx_over;
        i2c_finish &= (~I2c_irq_rx_over);
        TRACE_OUT(DEBUG_OUT, "*** Irq: RX_OVER...\n");
    }
    /* rx fifo underflow*/
    else if((reg & I2c_irq_rx_under) != 0) {
        callbackArg = I2c_irq_rx_under;
        clearIrqMask = I2c_irq_rx_under;
        i2c_finish &= (~I2c_irq_rx_under);
        TRACE_OUT(DEBUG_OUT, "*** Irq: RX_UNDER...\n");
    }
    /* tx fifo overflow*/
    else if((reg & I2c_irq_tx_over) != 0) {
        callbackArg = I2c_irq_tx_over;
        clearIrqMask = I2c_irq_tx_over;
        i2c_finish &= (~I2c_irq_tx_over);
        TRACE_OUT(DEBUG_OUT, "*** Irq: TX_OVER...\n");
    }
    /* a general call was detected*/
    else if((reg & I2c_irq_gen_call) != 0) {
        callbackArg = I2c_irq_gen_call;
        clearIrqMask = I2c_irq_gen_call;
        i2c_finish &= (~I2c_irq_gen_call);
        TRACE_OUT(DEBUG_OUT, "*** Irq: GEN_CALL...\n");
    }
    /* rx fifo level at or above threshold*/
    else if((reg & I2c_irq_rx_full) != 0) {
        callbackArg = I2c_irq_rx_full;
        i2c_finish &= (~I2c_irq_rx_full);
        /*read fifo*/
        fifo_level = FI2cPs_getRxFifoLevel(dev);
        for(i=0;i<fifo_level;i++)
        {
	     tmp=I2C_INP(portmap->data_cmd);
        }
        
        TRACE_OUT(DEBUG_OUT, "*** Irq: RX_FULL...\n");
    }
    /* read-request transfer completed (Tx FIFO may still contain data)*/
    else if((reg & I2c_irq_rx_done) != 0) {
        callbackArg = I2c_irq_rx_done;
        clearIrqMask = I2c_irq_rx_done;
        i2c_finish &= (~I2c_irq_rx_done);
        TRACE_OUT(DEBUG_OUT, "*** Irq: RX_DONE...\n");
    }
    /* read request received*/
    else if((reg & I2c_irq_rd_req) != 0) {
        callbackArg = I2c_irq_rd_req;
        clearIrqMask = I2c_irq_rd_req;
     /*   FMSH_i2c_write(dev,0x33);*/
        i2c_finish &= (~I2c_irq_rd_req);
       /* I2C_OUTP(0X00,portmap->data_cmd);*/
        TRACE_OUT(DEBUG_OUT, "*** Irq: RD_REQ...\n");
        
        for(i=0;i<16;i++)
        {
	    I2C_OUTP(i,portmap->data_cmd);
        }
        
    }
    /* tx fifo level at or below threshold*/
    else if((reg & I2c_irq_tx_empty) != 0) {
        callbackArg = I2c_irq_tx_empty;
        FI2cPs_maskIrq(dev,I2c_irq_tx_empty);  /*mask tx_empty*/
        i2c_finish &= (~I2c_irq_tx_empty);

        TRACE_OUT(DEBUG_OUT, "*** Irq: TX_EMPTY...\n");
    }
    /* start condition detected*/
    else if((reg & I2c_irq_start_det) != 0) {
        callbackArg = I2c_irq_start_det;
        clearIrqMask = I2c_irq_start_det;
        i2c_finish &= (~I2c_irq_start_det);
        TRACE_OUT(DEBUG_OUT, "*** Irq: START_DET...\n");
    }
    /* restart condition detected*/
    else if((reg & I2c_irq_restart_det) != 0) {
        callbackArg = I2c_irq_restart_det;
        clearIrqMask = I2c_irq_restart_det;
        i2c_finish &= (~I2c_irq_restart_det);
        TRACE_OUT(DEBUG_OUT, "*** Irq: RESTART_DET...\n");
    }
    /* stop condition detected*/
    else if((reg & I2c_irq_stop_det) != 0) {
        callbackArg = I2c_irq_stop_det;
        clearIrqMask = I2c_irq_stop_det;
        i2c_finish &= (~I2c_irq_stop_det);
        TRACE_OUT(DEBUG_OUT, "*** Irq: STOP_DET...\n");
    }
    /* i2c bus activity*/
    else if((reg & I2c_irq_activity) != 0) {
        callbackArg = I2c_irq_activity;
        clearIrqMask = I2c_irq_activity;
        i2c_finish &= (~I2c_irq_activity);
        TRACE_OUT(DEBUG_OUT, "*** Irq: ACTIVITY...\n");
        FI2cPs_maskIrq(dev,I2c_irq_activity);
    }
      else if((reg & I2c_irq_mst_on_hold) != 0) {
        callbackArg = I2c_irq_mst_on_hold;
        clearIrqMask = I2c_irq_mst_on_hold;
        i2c_finish &= (~I2c_irq_mst_on_hold);
        TRACE_OUT(DEBUG_OUT, "*** Irq: MST_ON_HOLD...\n");
        FI2cPs_maskIrq(dev,I2c_irq_mst_on_hold); 
    }
     else if((reg & I2c_scl_stuck_at_low) != 0) {
        callbackArg = I2c_scl_stuck_at_low;
        clearIrqMask = I2c_scl_stuck_at_low;
        i2c_finish &= (~I2c_scl_stuck_at_low);
        TRACE_OUT(DEBUG_OUT, "*** Irq: SCL_STUCK_AT_LOW...\n");
    }
    else {
        /* no active interrupt was found*/
        retval = false;
        userCallback = NULL;
    }

    /* call the user listener function, if there was an active interrupt*/
    if(userCallback != NULL)
        userCallback(dev, callbackArg);

    /* clear any serviced interrupt*/
    if(clearIrqMask != 0)
        FI2cPs_clearIrq(dev, clearIrqMask);

    return retval;
}


/* for test by jc*/
#if 1

/*----test-loopback-poll----*/  
u8 test_i2c_loop_bare(void)
{
  u8 ret=FMSH_SUCCESS;  
  
  FSlcrPS_setI2cLoop(1);
  
  FI2cPs_T *pI2c0_dev = &gI2c0_dev;
  FIicPs_Instance_T *pI2c0_Instance=&gI2c0_Instance;
  FIicPs_Param_T I2c0_Param;
  
  FI2cPs_T *pI2c1_dev = &gI2c1_dev;
  FIicPs_Instance_T *pI2c1_Instance=&gI2c1_Instance;
  FIicPs_Param_T I2c1_Param;
   
  rst_i2c();
  
  /* Initial I2C1 */
  I2C1_device_init(pI2c1_dev,pI2c1_Instance,&I2c1_Param);
  I2C_slave_init(pI2c1_dev);    /*In default config the device as slave*/

  /* Initial I2C0 */
  I2C0_device_init(pI2c0_dev,pI2c0_Instance,&I2c0_Param);
  I2C_master_init(pI2c0_dev);    /*In default config the device as master*/

  /* Disable irq */
  FI2cPs_maskIrq(pI2c0_dev,I2c_irq_all);   
  FI2cPs_maskIrq(pI2c1_dev,I2c_irq_all);   

  /* Enable device */
  FI2cPs_enable(pI2c0_dev);
  FI2cPs_enable(pI2c1_dev);
    
  u8 i=0;
  u8 date=0;
  u32 timeout=0x10000;
  u8 base=0x55;
  
  TRACE_OUT(DEBUG_OUT,"Master sending 8 bytes...\n");
  
  for(i = 0; i < 8; i++)
        FI2cPs_write(pI2c0_dev,  base+i);
  
  TRACE_OUT(DEBUG_OUT,"Slave received bytes: ");
  
  for(i = 0; i < 8; i++)
  {
        timeout=0x10000;
        while((FI2cPs_isRxFifoEmpty(pI2c1_dev) == true)&&(timeout--));      
        if(timeout==0)
        {
            TRACE_OUT(DEBUG_OUT,"\n*** ERROR: received timeout ***\n");
            ret=FMSH_FAILURE;
            break;
        }
		
        date = FI2cPs_read(pI2c1_dev);
        if(date != base+i)
        {
           TRACE_OUT(DEBUG_OUT,"\n*** ERROR: expected %d, received %d ***\n", base+i,date);  
           ret=FMSH_FAILURE;    
        }
        else          
            TRACE_OUT(DEBUG_OUT,"0x%02x ", date);
  }
  
  if(ret!=FMSH_SUCCESS)
      goto END;
   
  delay_ms(10);
    
    
  TRACE_OUT(DEBUG_OUT,"\n\nMaster issuing 7 read requests...\n");
  
  for(i = 0; i < 8; i++)
      FI2cPs_issueRead(pI2c0_dev);

  delay_ms(10);

  TRACE_OUT(DEBUG_OUT,"Write to slave Tx FIFO...\n");
  for(i = 0; i < 8; i++) 
  {
        FI2cPs_write(pI2c1_dev, base+i);
        /* clear the read request interrupt*/
        FI2cPs_clearIrq(pI2c1_dev, I2c_irq_rd_req);
  }

  TRACE_OUT(DEBUG_OUT,"Read from master Rx FIFO...\n");
  TRACE_OUT(DEBUG_OUT,"Master received bytes: ");
  
  for(i=0;i<8;i++)
  {
        timeout=0x10000;
        while((FI2cPs_isRxFifoEmpty(pI2c0_dev) == true)&&(timeout--));      
        if(timeout==0)
        {
            TRACE_OUT(DEBUG_OUT,"\n*** ERROR: received timeout ***\n");
            ret=FMSH_FAILURE;
            break;
        }
        date = FI2cPs_read(pI2c0_dev);
        if(date != base+i)
        {
            TRACE_OUT(DEBUG_OUT,"\n*** ERROR: expected %d, received %d ***\n ", base+i,date);
            ret=FMSH_FAILURE;
        }
        else
            TRACE_OUT(DEBUG_OUT,"0x%02x ", date);
    }
    
  
    TRACE_OUT(DEBUG_OUT,"\n The Poll mode test is done...\n");
     
END:    
    FI2cPs_issueSTOP(pI2c0_dev,0x88);
    FI2cPs_masterAbort(pI2c0_dev);
    return ret;

}

#endif

