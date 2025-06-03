/*
 * rfsoc.c
 *
 *  Created on: 2021��6��24��
 *      Author: zengchao
 */
#if 0
//#include "platform.h"
//#include "fmsh_common.h"
//#include "fmsh_gic.h"
//#include "fmsh_slcr.h"
//#include "fmsh_spips_lib.h" 
#endif
#include "./include/xparameters.h"
/*#include "platform.h" */
#include "./include/xspi.h"
#include "./include/xgpio.h"

#include "rfsoc.h"  
   
XGpio gpio_instance; 
 
XSpi_Config *spi_config1;
XSpi spi_instance1;

XSpi_Config *spi_config2;
XSpi spi_instance2;

XSpi_Config *spi_config3;
XSpi spi_instance3;

XSpi_Config *spi_config4;
XSpi spi_instance4;

/***************************************************************************//**
 * @brief spi_init
*******************************************************************************/
int32_t spi_init(uint8_t id_no,
                 uint8_t clk_pha,
                 uint8_t clk_pol)
{ 
    uint32_t base_addr = 0;
    uint32_t spi_options = 0;
    int32_t ret;

    if (1 == id_no)
    {
        spi_config1 = XSpi_LookupConfig(XPAR_PDV_AXI_SPI1_DEVICE_ID);
        base_addr = spi_config1->BaseAddress;

        ret = XSpi_CfgInitialize(&spi_instance1, spi_config1, base_addr);

        XSpi_Reset(&spi_instance1);

        spi_options = XSP_MASTER_OPTION |
                      (clk_pol ? XSP_CLK_ACTIVE_LOW_OPTION : 0) |
                      (clk_pha ? XSP_CLK_PHASE_1_OPTION : 0) |
                      XSP_MANUAL_SSELECT_OPTION;

        ret = XSpi_SetOptions(&spi_instance1, spi_options);

        ret = XSpi_SetSlaveSelect(&spi_instance1, 1);

        ret = XSpi_Start(&spi_instance1);

        XSpi_IntrGlobalDisable(&spi_instance1);
    }
    else if (2 == id_no)
    {
        spi_config2 = XSpi_LookupConfig(XPAR_PDV_AXI_SPI2_DEVICE_ID);
        base_addr = spi_config2->BaseAddress;

        ret = XSpi_CfgInitialize(&spi_instance2, spi_config2, base_addr);

        XSpi_Reset(&spi_instance2);

        spi_options = XSP_MASTER_OPTION |
                      (clk_pol ? XSP_CLK_ACTIVE_LOW_OPTION : 0) |
                      (clk_pha ? XSP_CLK_PHASE_1_OPTION : 0) |
                      XSP_MANUAL_SSELECT_OPTION;

        ret = XSpi_SetOptions(&spi_instance2, spi_options);

        ret = XSpi_SetSlaveSelect(&spi_instance2, 1);

        ret = XSpi_Start(&spi_instance2);

        XSpi_IntrGlobalDisable(&spi_instance2);
    }
    else if (3 == id_no)
    {
        spi_config3 = XSpi_LookupConfig(XPAR_PDV_AXI_SPI3_DEVICE_ID);
        base_addr = spi_config3->BaseAddress;

        ret = XSpi_CfgInitialize(&spi_instance3, spi_config3, base_addr);

        XSpi_Reset(&spi_instance3);

        spi_options = XSP_MASTER_OPTION |
                      (clk_pol ? XSP_CLK_ACTIVE_LOW_OPTION : 0) |
                      (clk_pha ? XSP_CLK_PHASE_1_OPTION : 0) |
                      XSP_MANUAL_SSELECT_OPTION;

        ret = XSpi_SetOptions(&spi_instance3, spi_options);

        ret = XSpi_SetSlaveSelect(&spi_instance3, 1);

        ret = XSpi_Start(&spi_instance3);

        XSpi_IntrGlobalDisable(&spi_instance3);
    }
    else if (4 == id_no)
    {
        spi_config4 = XSpi_LookupConfig(XPAR_PDV_AXI_SPI4_DEVICE_ID);
        base_addr = spi_config4->BaseAddress;

        ret = XSpi_CfgInitialize(&spi_instance4, spi_config4, base_addr);

        XSpi_Reset(&spi_instance4);

        spi_options = XSP_MASTER_OPTION |
                      (clk_pol ? XSP_CLK_ACTIVE_LOW_OPTION : 0) |
                      (clk_pha ? XSP_CLK_PHASE_1_OPTION : 0) |
                      XSP_MANUAL_SSELECT_OPTION;

        ret = XSpi_SetOptions(&spi_instance4, spi_options);

        ret = XSpi_SetSlaveSelect(&spi_instance4, 1);

        ret = XSpi_Start(&spi_instance4);   

        XSpi_IntrGlobalDisable(&spi_instance4);
    }

    return ret;
}

/***************************************************************************//**
 * @brief rfsoc_spi_read
*******************************************************************************/
int32_t rfsoc_spi_loop(uint8_t id_no, uint8_t *tx, uint8_t *rx, uint8_t len)
{
    uint8_t send_buf[16];
    uint8_t recv_buf[16];
    uint8_t i;
    int32_t ret;  
    
    if (len < 16)
    {   
        for (i=0;i<len;i++)
        {
            send_buf[i] = tx[i];
            recv_buf[i] = 0x00;
        }
        
        ret = XSpi_Transfer(&spi_instance1, send_buf, recv_buf, len);
        
        ret = 1;
        for (i=0;i<len;i++)
        { 
            if (recv_buf[i] != send_buf[i])
            ret++;
        }
        return ret;
    }
    else
    {
          return -1;
    }
}
 

/***************************************************************************//**
 * @brief rfsoc_spi_read
*******************************************************************************/
int32_t rfsoc_spi_read(uint8_t id_no, uint32_t reg, uint32_t *val)
{
    uint8_t send_buf[3];
    uint8_t recv_buf[3];
    uint16_t cmd;
    int32_t ret;

    cmd = AD_READ | AD_CNT(1) | AD_ADDR(reg);
    send_buf[0] = cmd >> 8;
    send_buf[1] = cmd & 0xFF;
    send_buf[2] = 0x00;
    
    if (1 == id_no)
    {
        XSpi_SetSlaveSelect(&spi_instance1, 1);
        ret = XSpi_Transfer(&spi_instance1, send_buf, recv_buf, 3);
    }
    else if (2 == id_no)
    {
        XSpi_SetSlaveSelect(&spi_instance2, 1);
        ret = XSpi_Transfer(&spi_instance2, send_buf, recv_buf, 3);
    }
    else if (3 == id_no)
    {
        XSpi_SetSlaveSelect(&spi_instance3, 1);
        ret = XSpi_Transfer(&spi_instance3, send_buf, recv_buf, 3);
    }
    else if (4 == id_no)
    {
        XSpi_SetSlaveSelect(&spi_instance4, 1);
        ret = XSpi_Transfer(&spi_instance4, send_buf, recv_buf, 3);
    }  

    *val = recv_buf[2];

    return 1;
}

/***************************************************************************//**
 * @brief rfsoc_spi_write
*******************************************************************************/
int32_t rfsoc_spi_write(uint8_t id_no, uint32_t reg, uint32_t val)
{
    uint8_t send_buf[3];
    uint8_t recv_buf[3];
    uint16_t cmd;
    int32_t ret;
    uint32_t ret_val;

    cmd = AD_WRITE | AD_CNT(1) | AD_ADDR(reg);
    send_buf[0] = cmd >> 8;
    send_buf[1] = cmd & 0xFF;
    send_buf[2] = val;
    
    if (1 == id_no)
    {
        XSpi_SetSlaveSelect(&spi_instance1, 1);
        ret = XSpi_Transfer(&spi_instance1, send_buf, recv_buf, 3);
    }
    else if (2 == id_no)
    {
        XSpi_SetSlaveSelect(&spi_instance2, 1);
        ret = XSpi_Transfer(&spi_instance2, send_buf, recv_buf, 3);
    }
    else if (3 == id_no)
    {
        XSpi_SetSlaveSelect(&spi_instance3, 1);
        ret = XSpi_Transfer(&spi_instance3, send_buf, recv_buf, 3);
    }
    else if (4 == id_no)
    {
        XSpi_SetSlaveSelect(&spi_instance4, 1);
        ret = XSpi_Transfer(&spi_instance4, send_buf, recv_buf, 3);
    }    

    return ret;
}

/***************************************************************************//**
 * @brief rfsoc_init
*******************************************************************************/
int32_t rfsoc_init(void)
{
	int32_t ret;

	printf("rfsoc_init starting ... \n");
	
	spi_init(1, 1, 0);   
	printf("rfsoc_init-1 end! \n");
	
    spi_init(2, 1, 0);
	printf("rfsoc_init-2 end! \n");
	
    spi_init(3, 1, 0);
	printf("rfsoc_init-3 end! \n");
	
    spi_init(4, 1, 0);
	printf("rfsoc_init-4 end! \n");
	

    ret = XGpio_Initialize(&gpio_instance, XPAR_PDV_AXI_DO_DEVICE_ID);
    if (ret == XST_SUCCESS)
    { 
        XGpio_SetDataDirection(&gpio_instance, 1, 0);
        XGpio_DiscreteWrite(&gpio_instance, 1, 0x01);
        delay_us(100000);
        XGpio_DiscreteWrite(&gpio_instance, 1, 0xFFFFFFFE);  
        
        XGpio_SetDataDirection(&gpio_instance, 2, 0);
        XGpio_DiscreteWrite(&gpio_instance, 2, 0);
        delay_us(100000);
        XGpio_DiscreteWrite(&gpio_instance, 2, 0xFFFFFFFF); 
    }

    return ret;
}
 
/***************************************************************************//**
 * @brief rfsoc_spi_cfmwrt
*******************************************************************************/
int32_t rfsoc_spi_cfmwrt(uint8_t id_no, uint32_t reg, uint32_t val)
{
	uint32_t read_val;

	rfsoc_spi_write(id_no, reg, val);
	rfsoc_spi_read(id_no, reg, &read_val);

	return (read_val == val);
}

int32_t rfsoc_ctrl(uint32_t val)
{
	int32_t ret;

	XGpio_DiscreteWrite(&gpio_instance, 2, val);
	ret = val;

    return ret;
}
