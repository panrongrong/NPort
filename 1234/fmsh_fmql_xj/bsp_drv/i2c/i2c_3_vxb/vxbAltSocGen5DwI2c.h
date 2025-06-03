/* vxbAltSocGen5DwI2c.h - DesignWare I2C Controller hardware defintions */

/*
 * Copyright (c) 2013 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01a,20mar13,swu  created
*/

#ifndef __INCvxbAltSocGen5DwI2ch
#define __INCvxbAltSocGen5DwI2ch

/* includes */

#include <vxWorks.h>
#include <intLib.h>
#include <arch/arm/intArmLib.h>
#include <spinLockLib.h>
#include <vxBusLib.h>
#include <vxAtomicLib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* defines */

#define DW_I2C_DRIVER_NAME              "dwI2c"
#define I2C_TIMEOUT                     500
#define I2C_STANDARD_SPEED              100000
#define I2C_FAST_SPEED                  400000
#define I2C_MAX_READ_SIZE               64
#define SEM_TIMEOUT                     2

/* Register Definition */

#define I2C_CON                         0x00
#define I2C_TAR                         0x04    
#define I2C_SAR                         0x08
#define I2C_HS_MADDR                    0x0C
#define I2C_DATA_CMD                    0x10
#define I2C_SS_SCL_HCNT                 0x14
#define I2C_SS_SCL_LCNT                 0x18
#define I2C_FS_SCL_HCNT                 0x1C
#define I2C_FS_SCL_LCNT                 0x20
#define I2C_HS_SCL_HCNT                 0x24
#define I2C_HS_SCL_LCNT                 0x28
#define I2C_INTR_STAT                   0x2C
#define I2C_INTR_MASK                   0x30
#define I2C_RAW_INTR_STAT               0x34
#define I2C_RX_TL                       0x38
#define I2C_TX_TL                       0x3C
#define I2C_CLR_INTR                    0x40
#define I2C_CLR_RX_UNDER                0x44
#define I2C_CLR_RX_OVER                 0x48
#define I2C_CLR_TX_OVER                 0x4C
#define I2C_CLR_RD_REQ                  0x50
#define I2C_CLR_TX_ABRT                 0x54
#define I2C_CLR_RX_DONE                 0x58
#define I2C_CLR_ACTIVITY                0x5c
#define I2C_CLR_STOP_DET                0x60
#define I2C_CLR_START_DET               0x64
#define I2C_CLR_GEN_CALL                0x68
#define I2C_ENABLE                      0x6C
#define I2C_STATUS                      0x70
#define I2C_TXFLR                       0x74
#define I2C_RXFLR                       0x78
#define I2C_TX_ABRT_SOURCE              0x80
#define I2C_SLV_DATA_NACK_ONLY          0x84
#define I2C_DMA_CR                      0x88
#define I2C_DMA_TDLR                    0x8c
#define I2C_DMA_RDLR                    0x90
#define I2C_SDA_SETUP                   0x94
#define I2C_ACK_GENERAL_CALL            0x98
#define I2C_ENABLE_STATUS               0x9C
#define I2C_COMP_PARAM_1                0xf4
#define I2C_COMP_VERSION                0xf8
#define I2C_COMP_TYPE                   0xfc

#define I2C_RAW_INTR_STAT_RX_UNDER      0x1
#define I2C_RAW_INTR_STAT_RX_OVER       0x2
#define I2C_RAW_INTR_STAT_RX_FULL       0x4   
#define I2C_RAW_INTR_STAT_TX_OVER       0x8 
#define I2C_RAW_INTR_STAT_TX_EMPTY      0x10
    
/* Default parameters */
    
#define I2C_CON_ME                      (0x1 << 0)
#define I2C_CON_MS_SS     	        (0x1 << 1)
#define I2C_CON_MS_FS                   (0x2 << 1)
#define I2C_CON_SLAVE_ADR_7BIT          (0x0 << 3)
#define I2C_CON_SLAVE_ADR_10BIT         (0x1 << 3)    
#define I2C_CON_MASTER_ADR_7BIT         (0x0 << 4)
#define I2C_CON_MASTER_ADR_10BIT                (0x1 << 4)  
#define I2C_CON_RESTART_EN              (0x1 << 5)  
#define I2C_CON_SLAVE_DISABLE           (0x1 << 6)   
    
#define I2C_CTR_DEFAULT     (I2C_CON_ME | I2C_CON_MASTER_ADR_7BIT | \
                             I2C_CON_SLAVE_DISABLE)
                               
#define I2C_IRQ_NONE_MASK               0x0
#define I2C_IRQ_ALL_MASK                0x8ff    

#define I2C_TAR_STARTBYTE               (0x1 << 10)
#define I2C_TAR_SPECIAL_STARTBYTE       (0x1 << 11)
#define I2C_TAR_ADR_7BIT                (0x0)
#define I2C_TAR_ADR_10BIT               (0x1 << 12)
    
#define	I2C_SLAVE_DISABLE_DEFAULT              0
#define I2C_RESTART_EN_DEFAULT                 1
#define I2C_10BITADDR_MASTER_DEFAULT           0
#define I2C_10BITADDR_SLAVE_DEFAULT            1
#define I2C_MAX_SPEED_MODE_DEFAULT             3
#define I2C_MASTER_MODE_DEFAULT                1

#define I2C_DEFAULT_TAR_ADDR_DEFAULT           0x055
#define I2C_DEFAULT_SLAVE_ADDR_DEFAULT         0x055
#define I2C_COMP_VERSION_DEFAULT               0x3131352A

#define I2C_HS_MASTER_CODE_DEFAULT             1
#define I2C_SS_SCL_HIGH_COUNT_DEFAULT          0x0190
#define I2C_SS_SCL_LOW_COUNT_DEFAULT           0x01d6
#define I2C_FS_SCL_HIGH_COUNT_DEFAULT          0x003c
#define I2C_FS_SCL_LOW_COUNT_DEFAULT           0x0082
#define I2C_HS_SCL_HIGH_COUNT_DEFAULT          0x006
#define I2C_HS_SCL_LOW_COUNT_DEFAULT           0x0010
#define I2C_RX_TL_DEFAULT                      0
#define I2C_TX_TL_DEFAULT                      0
#define I2C_DEFAULT_SDA_SETUP_DEFAULT          0x64
#define I2C_DEFAULT_ACK_GENERAL_CALL_DEFAULT   1
#define I2C_DYNAMI2C_TAR_UPDATE_DEFAULT        1
#define I2C_RX_BUFFER_DEPTH_DEFAULT            8
#define I2C_TX_BUFFER_DEPTH_DEFAULT            8
#define I2C_ADD_ENCODED_PARAMS_DEFAULT         1
#define I2C_HAS_DMA_DEFAULT                    0
#define I2C_INTR_IO_DEFAULT                    0
#define I2C_HC_COUNT_VALUES_DEFAULT            0
#define APB_DATA_WIDTH_DEFAULT                 0 
#define I2C_SLV_DATA_NACK_ONLY_DEFAULT         0
#define I2C_USE_COUNTS_DEFAULT                 0
#define I2C_CLK_TYPE_DEFAULT                   1
#define I2C_CLOCK_PERIOD_DEFAULT               10 

/* Raw Interrupt Status */

#define I2C_IRQ_NONE		        0x000
#define I2C_IRQ_RX_UNDER	        0x001
#define I2C_IRQ_RX_OVER		        0x002
#define I2C_IRQ_RX_FULL		        0x004
#define I2C_IRQ_TX_OVER 	        0x008
#define I2C_IRQ_TX_EMPTY	        0x010
#define I2C_IRQ_RD_REQ		        0x020
#define I2C_IRQ_TX_ABRT		        0x040
#define I2C_IRQ_RX_DONE		        0x080
#define I2C_IRQ_ACTIVITY	        0x100
#define I2C_IRQ_STOP_DET	        0x200
#define I2C_IRQ_START_DET	        0x400
#define I2C_IRQ_GEN_CALL	        0x800
#define I2C_IRQ_ALL                     0xFFF

/* Default IRQ Mask Bit Setting */

#define I2C_IRQ_DEFAULT_MASK    (I2C_IRQ_RX_FULL | I2C_IRQ_TX_EMPTY | \
                                 I2C_IRQ_TX_ABRT | I2C_IRQ_STOP_DET | \
                                 I2C_IRQ_START_DET)

                                 
/* Data command stop bit */

#define I2C_DATA_CMD_WR_STOP_BIT        0x200
#define I2C_DATA_CMD_RD_STOP_BIT        0x300

/* I2C TX Abort Source*/

#define I2C_ABRT_7B_ADDR_NOACK		0x001
#define I2C_ABRT_10_ADDR1_NOACK 	0x002   
#define I2C_ABRT_10_ADDR2_NOACK  	0x004
#define I2C_ABRT_TXDATA_NOACK		0x008
#define I2C_ABRT_GCALL_NOACK		0x010
#define I2C_ABRT_GCALL_READ             0x020
#define I2C_ABRT_HS_ACKDET              0x040
#define I2C_ABRT_SBYTE_ACKDET		0x080
#define I2C_ABRT_HS_NORSTRT             0x100
#define I2C_ABRT_SBYTE_NORSTRT		0x200
#define I2C_ABRT_10B_RD_NORSTRT		0x400	
#define I2C_ABRT_MASTER_DIS             0x800
#define I2C_ABRT_ARB_LOST               0x1000
#define I2C_ABRT_SLVFLUSH_TXFIFO	0x2000	
#define I2C_ABRT_SLV_ARBLOST		0x5000
#define I2C_ABRT_SLVRD_INTX             0x8000

/* Minimum High and Low period in nanosecends */

#define SS_MIN_SCL_HIGH                 4000
#define SS_MIN_SCL_LOW                  4700
#define FS_MIN_SCL_HIGH                 600
#define FS_MIN_SCL_LOW                  1300
#define HS_MIN_SCL_HIGH_100PF           60
#define HS_MIN_SCL_LOW_100PF            120

/* I2C_STATUS */
    
#define I2C_STATUS_ACTIVITY             (0x1 << 0)
#define I2C_STATUS_TFNF	                (0x1 << 1)
#define I2C_STATUS_TFE                  (0x1 << 2)
#define I2C_STATUS_RFNE                 (0x1 << 3)
#define I2C_STATUS_RFF                  (0x1 << 4)
#define I2C_STATUS_MST_ACTIVITY		(0x1 << 5)
#define I2C_STATUS_SLV_ACTIVITY		(0x1 << 6)

/* Interrupts status */
    
#define I2C_INTR_RX_UNDER	        (0x1 << 0)
#define I2C_INTR_RX_OVER	        (0x1 << 1)
#define I2C_INTR_RX_FULL	        (0x1 << 2)
#define I2C_INTR_TX_OVER	        (0x1 << 3)
#define I2C_INTR_TX_EMPTY	        (0x1 << 4)
#define I2C_INTR_RD_REQ		        (0x1 << 5)
#define I2C_INTR_TX_ABRT	        (0x1 << 6)
#define I2C_INTR_RX_DONE	        (0x1 << 7)
#define I2C_INTR_ACTIVITY	        (0x1 << 8)
#define I2C_INTR_STOP_DET	        (0x1 << 9)
#define I2C_INTR_START_DET	        (0x1 << 10)
#define I2C_INTR_GEN_CALL	        (0x1 << 11)

#define I2C_INTR_MAX_BITS               12

/* structure holding the instance specific details */

typedef struct i2c_drv_ctrl {
    VXB_DEVICE_ID   i2cDev;
    void *          i2cHandle;
    SEM_ID          i2cDevSem;
    SEM_ID          i2cDataSem;
    UINT32          clkFrequency;
    UINT32          busSpeed;
    BOOL            polling;
    UINT16          slaveAddr;
    UINT8 *         dataBuf;
    UINT32          dataLength;
    BOOL            statue;
} I2C_DRV_CTRL;

/* I2C controller read and write interface */

#define DW_I2C_BAR(p)       ((char *)((p)->pRegBase[0]))
#define DW_I2C_HANDLE(p)    (((I2C_DRV_CTRL *)(p)->pDrvCtrl)->i2cHandle)

/* function declarations */

STATUS vxbAltSocGen5DwI2cWrite (VXB_DEVICE_ID, UINT8, UINT32, UINT8 *, UINT32);
STATUS vxbAltSocGen5DwI2cRead (VXB_DEVICE_ID, UINT8, UINT32, UINT8 *, UINT32);

#ifdef __cplusplus
}
#endif

#endif /* __INC_vxbAltSocGen5DwI2c_H */
