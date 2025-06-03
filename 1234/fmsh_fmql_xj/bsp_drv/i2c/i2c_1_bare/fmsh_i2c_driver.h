#ifndef _FMSH_I2C_DRIVER_H
#define _FMSH_I2C_DRIVER_H

/*#include "FMSH_psoc.h"*/

#include "../../common/fmsh_common.h"          /* common header for all drivers*/
#include "fmsh_i2c_lib.h"

/*#include "fmsh_dmac_lib.h"*/
/*#include "dmac_config.h"*/

/*#include "config.h"*/

/*---------------------------------------------------
*      definitions
*---------------------------------------------------*/
#define I2C_BUFFER_SIZE  40


#define I2C0_TEST_PASS       0
#define I2C0_TEST_FAIL       1
#define I2C0_TEST_CONFIG_ERR 2
#define I2C0_TEST_PARAM_ERR  3


/*---------------------------------------------------
*      Global_Variables
*---------------------------------------------------*/

extern FI2cPs_T gI2c0_dev;      /*as Master*/
extern FI2cPs_T gI2c1_dev;      /*as Slave*/
extern FIicPs_Instance_T gI2c0_Instance;
extern FIicPs_Instance_T gI2c1_Instance;

extern uint8_t I2C_Master_RxBuffer[I2C_BUFFER_SIZE];
extern uint8_t I2C_Master_TxBuffer[I2C_BUFFER_SIZE];

/*extern uint8_t I2C_Slave_Buffer[I2C_BUFFER_SIZE];*/
extern uint8_t I2C_Slave_RxBuffer[I2C_BUFFER_SIZE];
extern uint8_t I2C_Slave_TxBuffer[I2C_BUFFER_SIZE];

extern u16 I2C0_DMA_RxBuffer[I2C_BUFFER_SIZE];
extern u16 I2C0_DMA_TxBuffer[I2C_BUFFER_SIZE];

/**********************************************************
*  Public Functions
***********************************************************/
void FI2cPs_clock_setup(FI2cPs_T *dev, unsigned ic_clk);

u8 I2C0_device_init(
                      FI2cPs_T *pDev,
                      void* pI2cInstance,
                      void* I2cParam );
u8 I2C1_device_init(
                      FI2cPs_T *pDev,
                      void* pI2cInstance,
                      void* I2cParam );
void I2C_master_init(FI2cPs_T *dev);
void I2C_slave_init(FI2cPs_T *dev);

void I2C0_Config_CON (FI2cPs_T *dev,
                      uint8_t bDAT);
void I2C0_Config_TAR (FI2cPs_T *dev,
                      uint16_t wTAR);
void I2C0_Config_SAR (FI2cPs_T *dev,
                      uint16_t wSAR);

void test_i2c_master_callback(void *dev,int32_t numBytes);

void test_i2c_slave_callback(void *dev,int32_t numBytes);

void I2C0_Master_Write_Block(FI2cPs_T *master,
                            uint8_t *byte,uint8_t length);
void I2C0_Master_Write_Byte(FI2cPs_T *master,
                            uint8_t  byte);
uint8_t I2C0_Master_Read_Block( FI2cPs_T *master,
                               uint8_t *data,uint8_t length);
uint8_t I2C0_Master_Read_Byte( FI2cPs_T *master);

void I2C0_Slave_Transmitter(FI2cPs_T *slave,uint8_t bData);

uint8_t I2C0_Slave_Read_Byte(FI2cPs_T *slave);

void I2C0_Slave_Listener(void *dev, int32_t ecode);
void I2C0_Master_Listener(void *dev, int32_t ecode);

void EEPROM_Write(FI2cPs_T *master,
                  uint16_t iaddress,
                  uint8_t  byte);
uint8_t EEPROM_Read( FI2cPs_T *master,
                     uint16_t iaddress);

void I2C0_Handler(FI2cPs_T *dev);

/*-----test_proc------------------------------*/
u8 test_i2c0_master(FI2cPs_T *master,FI2cPs_T *slave,u8 blength);
u8 test_i2c0_slave(FI2cPs_T *dev);
u8 test_i2c_irq_master_gen_call(FI2cPs_T *master,FI2cPs_T *slave,u8 num);
/*for gen call*/
u8 test_i2c_irq_gen_call(FI2cPs_T *master,FI2cPs_T *slave,u8 num);


BOOL test_SCL_timeout(FI2cPs_T *master,FI2cPs_T *slave);

u8 rst_i2c(void);

int FMSH_i2c_userIrqHandler(FI2cPs_T *dev);

#endif
