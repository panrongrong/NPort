/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (JFM_QL_PS) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bsp)
 */

#include <stdio.h>

/*
//#include "platform.h"
#include "fmsh_common.h"
#include "ps_init.h"
#include "fmsh_print.h"

#include "fmsh_common.h"
#include "fmsh_ps_parameters.h"
*/

#include "./include/xparameters.h" 

#include "rfsoc.h"



/*int main()*/
int test_axi_spi_loop(void)
{
    uint32_t reg;
    uint32_t val;
    int32_t ret;
    int32_t i,j,k;
    uint32_t reg_num;
    uint32_t ctrl;  

/*
	init_platform();
    
    FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x008, 0xDF0D767BU);   //unlock
    FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x838, 0xf);            //Open USER_LVL_SHFTR_EN_A and USER_LVL_SHFTR_EN_5
    FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x004, 0xDF0D767BU);  //lock
*/
    reg = 0x000;    

    rfsoc_init();
 
    /*while (1)*/
    {
        for (j=1;j<=4;j++)   
        {
            for (k=0;k<=2048;k++)   
            {
                ret = rfsoc_spi_loop(j, (uint8_t *)&k, (uint8_t *)&i, sizeof(k));
                if (1 != ret)    
                {
                    /*fmsh_print("axi spi loop erro!\n\r");*/
					printf("axi spi loop erro!\n\r");
                } 
            }
        }
    }

	printf("test_axi_spi_loop ok! \n\n");
	
    return 0;
}  


