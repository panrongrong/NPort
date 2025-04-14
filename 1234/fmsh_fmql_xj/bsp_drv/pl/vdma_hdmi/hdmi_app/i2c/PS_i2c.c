#include "../../include/xparameters.h"

#include "PS_i2c.h"

#if 0  /* z7*/

XIicPs IicInstance;

int i2c_init(XIicPs *IicInstance, u16 DeviceId, u32 FsclHz)
{
	int Status;
	XIicPs_Config *ConfigPtr;	/* Pointer to configuration data */
	/*
	 * Initialize the IIC driver so that it is ready to use.
	 */
	ConfigPtr = XIicPs_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		return XST_FAILURE;
	}

	Status = XIicPs_CfgInitialize(IicInstance, ConfigPtr,
					ConfigPtr->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	/*
	 * Set the IIC serial clock rate.
	 */
	XIicPs_SetSClk(IicInstance, FsclHz);
	return XST_SUCCESS;
}

int i2c_wrtie_bytes(XIicPs *IicInstance,u8 i2c_slave_addr,void *buf,int byte_num)
{
	int Status;

	/*
	 * Send the Data.
	 */
	Status = XIicPs_MasterSendPolled(IicInstance, buf,
			byte_num, i2c_slave_addr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait until bus is idle to start another transfer.
	 */
	while (XIicPs_BusIsBusy(IicInstance));

	/*
	 * Wait for a bit of time to allow the programming to complete
	 */
	usleep(2500);

	return XST_SUCCESS;
}

int i2c_read_bytes(XIicPs *IicInstance,u8 i2c_slave_addr,void *buf,int byte_num)
{
	int Status;

	Status = XIicPs_MasterRecvPolled(IicInstance, buf,
			byte_num, i2c_slave_addr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait until bus is idle to start another transfer.
	 */
	while (XIicPs_BusIsBusy(IicInstance));

	return XST_SUCCESS;
}

u8 i2c_reg8_read(XIicPs *IicInstance,u8 i2c_slave_addr,u8 reg_addr)
{

	u8 buf[1];
	u8 read_buf[1];
	buf[0] = reg_addr;
	
	if(i2c_wrtie_bytes(IicInstance,i2c_slave_addr,buf,1) != XST_SUCCESS)
		return 0xff;
	
	if(i2c_read_bytes(IicInstance,i2c_slave_addr,read_buf,1) != XST_SUCCESS)
		return 0xff;
	
	return read_buf[0];
}

int i2c_reg8_write(XIicPs *IicInstance,u8 i2c_slave_addr,u8 reg_addr,u8 data)
{

	u8 buf[2];
	buf[0] = reg_addr;
	buf[1] = data;
	
	if(i2c_wrtie_bytes(IicInstance,i2c_slave_addr,buf,2) != XST_SUCCESS)
		return XST_FAILURE;
	
	return XST_SUCCESS;
}
#endif

#if 1  /* fmql*/

#define SLAVE_ADDRESS 0x5A

#define EEPROM_ADDR   0x50
#define I2C_BUF_LEN   10


FI2cPs_T gI2c0_dev;
FI2cPs_T gI2c1_dev;

FIicPs_Instance_T gI2c0_Instance;
FIicPs_Instance_T gI2c1_Instance;

static volatile int s_i2cMasterWaitFlag;
static volatile int s_i2cSlaveWaitFlag;

static uint8_t s_i2cMasterTxBuffer[I2C_BUF_LEN];
static uint8_t s_i2cSlaveRxBuffer[I2C_BUF_LEN];


u8 FI2cPs_EepromInit(FI2cPs_T *dev, int i2c_dev_addr)
{
    /* 
     Set up the clock count register.  The argument I2C1_CLOCK is
     specified as the I2C dev input clock.
     */
    /*FI2cPs_ClockSetup(dev, I2C_CLOCK);*/
    FI2cPs_ClockSetup(dev, (dev->input_clock)/1000000);
	
    /* set the speed mode to standard*/
    FI2cPs_setSpeedMode(dev, I2c_speed_standard);
	
    /* use 7-bit addressing*/
    FI2cPs_setMasterAddressMode(dev, I2c_7bit_address);
    FI2cPs_setSlaveAddressMode(dev, I2c_7bit_address);
	
    /* enable restart conditions*/
    FI2cPs_enableRestart(dev);
	
    /* enable master FSM*/
    FI2cPs_enableMaster(dev);
    FI2cPs_disableSlave(dev);
    
    /* Use the start byte protocol with the target address when*/
    /* initiating transfer.*/
    FI2cPs_setTxMode(dev, I2c_tx_target);
    
    /* set target address to the I2C slave address*/
    /*FI2cPs_setTargetAddress(dev, EEPROM_ADDR);*/
    FI2cPs_setTargetAddress(dev, i2c_dev_addr);
        
    /*enable the dev I2C device*/
    FI2cPs_enable(dev); 
    
    return 0;
}

void FI2cPs_EepromWrite(FI2cPs_T *master,uint16_t iaddress,uint8_t  byte)
{
    uint8_t bHaddr,bLaddr;
    
    bHaddr = (uint8_t)(iaddress >> 8);
    bLaddr = (uint8_t)iaddress;
   
    /* write internal address */
    FI2cPs_write(master,bHaddr);
    FI2cPs_write(master,bLaddr);
    
    FI2cPs_issueSTOP(master,byte);
}


uint8_t FI2cPs_EepromRead( FI2cPs_T *master,uint16_t iaddress)
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
    
   /* while(FI2cPs_isRxFifoEmpty(master) == true);*/
    while(FI2cPs_isRxFifoEmpty(master) == 1);

    bRtval = FI2cPs_read(master);
    return bRtval;
       
}


u8 FI2c0Ps_DeviceInit(
                      FI2cPs_T *pDev,
                      void* pI2cInstance,
                      void* I2cParam )
{
    u8 ret=FMSH_SUCCESS;     
    FI2cPs_Config* Config=NULL;
	
    Config = FI2cPs_LookupConfig(FPAR_I2CPS_0_DEVICE_ID);
    if(Config==NULL)
      return FMSH_FAILURE;
	
    ret=FI2cPs_init(pDev, Config,pI2cInstance,I2cParam);
    if(ret!=FMSH_SUCCESS)
      return ret;
	
    return ret;
}

/******************************************************************************
*
* @description
*    A example of i2c write and read data from the eeprom.
*
* @param    None.
*
* @return   0 if successful, otherwise 1.
*
* @note     None.
*
******************************************************************************/
u8 FI2cPs_example2(void)
{
	u8 i=0;

	FI2cPs_T *pI2c0_dev = &gI2c0_dev;
	FIicPs_Instance_T *pI2c1_Instance=&gI2c0_Instance;
	FIicPs_Param_T I2c0_Param;
	
	int i2c_dev_addr_1 = 0x72;
	int i2c_dev_addr_2 = 0x7A;
	int ret1 = 0, ret2 = 0;

	FI2c0Ps_DeviceInit(pI2c0_dev, pI2c1_Instance, &I2c0_Param);

	FI2cPs_EepromInit(pI2c0_dev, i2c_dev_addr_1);

	/*
	for (i=0;i<10;i++)
	{  
		s_i2cMasterTxBuffer[i] = i*2+1;
	}

	for(i=0;i<10;i++)
	{
		//Write
		FI2cPs_EepromWrite(pI2c0_dev,i,s_i2cMasterTxBuffer[i]); 
		
		delay_ms(20);
		
		//Read  
		if(FI2cPs_EepromRead(pI2c0_dev,i)!=s_i2cMasterTxBuffer[i])
		   break;
	}
	*/
	
	/*i2c_reg8_write(&IicInstance, 0x72>>1, 0x08, 0x35);	*/
	ret1 = FI2cPs_EepromRead(pI2c0_dev, 0x08);
	FI2cPs_EepromWrite(pI2c0_dev, 0x08, 0x35);
	ret2 = FI2cPs_EepromRead(pI2c0_dev, 0x08);
	printf("i2c_addr(0x%X)=0x%X-0x%X \n", i2c_dev_addr_1, ret1, ret2);
	
	/*i2c_reg8_write(&IicInstance, 0x7a>>1, 0x2f, 0x00);*/
    FI2cPs_setTargetAddress(pI2c0_dev, i2c_dev_addr_2);
	ret1 = FI2cPs_EepromRead(pI2c0_dev, 0x2f);
	FI2cPs_EepromWrite(pI2c0_dev,0x2f, 0x00);
	ret2 = FI2cPs_EepromRead(pI2c0_dev, 0x2f);
	printf("i2c_addr(0x%X)=0x%X-0x%X \n", i2c_dev_addr_2, ret1, ret2);

	FI2cPs_masterAbort(pI2c0_dev);
	return 0;
}

void i2c_slave_sii9134_init(void)
{
	u8 i = 0, reg = 0, val = 0;

	FI2cPs_T *pI2c0_dev = &gI2c0_dev;
	FIicPs_Instance_T *pI2c0_Instance = &gI2c0_Instance;
	FIicPs_Param_T I2c0_Param;
	
	int i2c_dev_addr_1 = 0x72 >> 1;
	int i2c_dev_addr_2 = 0x7A >> 1;

	FI2c0Ps_DeviceInit(pI2c0_dev, pI2c0_Instance, &I2c0_Param);

	/*********************
	i2c_slave_1: 0x72
	**********************/
	FI2cPs_EepromInit(pI2c0_dev, i2c_dev_addr_1);  
	FI2cPs_enableRestart(pI2c0_dev);
	
/*  
	// for test

	//
	// read
	//
	reg = 0x02;
	FI2cPs_write(pI2c0_dev, reg);
	FI2cPs_issueRead(pI2c0_dev);
	while (FI2cPs_isRxFifoEmpty(pI2c0_dev) == 1);
	val = FI2cPs_read(pI2c0_dev);

	printf("reg(0x%X) = 0x%X \n", reg, val);  // id_1 == 0x34

	//
	// read
	//
	reg = 0x03;
	FI2cPs_write(pI2c0_dev, reg);
	FI2cPs_issueRead(pI2c0_dev);
	while (FI2cPs_isRxFifoEmpty(pI2c0_dev) == 1);
	val = FI2cPs_read(pI2c0_dev);

	printf("reg(0x%X) = 0x%X \n", reg, val);  // id_2  == 0x91
*/
	
	/*///////////////////////////////////////*/
	/**/
	/* write*/
	reg = 0x08;
	FI2cPs_write(pI2c0_dev, reg);    
    FI2cPs_issueSTOP(pI2c0_dev, 0x35);
	/**/
	/* read*/
	FI2cPs_write(pI2c0_dev, reg);
	FI2cPs_issueRead(pI2c0_dev);
	while (FI2cPs_isRxFifoEmpty(pI2c0_dev) == 1);
	val = FI2cPs_read(pI2c0_dev);

	printf("reg1(0x%X) = 0x%X \n", reg, val);


	/*********************
	i2c_slave_2: 0x7A
	**********************/
	FI2cPs_EepromInit(pI2c0_dev, i2c_dev_addr_2);  
	FI2cPs_enableRestart(pI2c0_dev);
	
	/*///////////////////////////////////////*/
	/**/
	/* write*/
	reg = 0x2f;
	FI2cPs_write(pI2c0_dev, reg);    
    FI2cPs_issueSTOP(pI2c0_dev, 0x00);
	/**/
	/* read*/
	FI2cPs_write(pI2c0_dev, reg);
	FI2cPs_issueRead(pI2c0_dev);
	while (FI2cPs_isRxFifoEmpty(pI2c0_dev) == 1);
	val = FI2cPs_read(pI2c0_dev);

	printf("reg2(0x%X) = 0x%X \n", reg, val);
	
	FI2cPs_masterAbort(pI2c0_dev);
	return;
}

#endif


