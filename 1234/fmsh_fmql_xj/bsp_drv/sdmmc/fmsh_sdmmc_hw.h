/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_sdmmc_hw.h
*
* This file contains
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 0.01   yl  12/20/2018  First Release
*</pre>
******************************************************************************/
#ifndef _FMSH_SDMMC_HW_H_
#define _FMSH_SDMMC_HW_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/***************************** Include Files *********************************/

#include <stdint.h>

/************************** Constant Definitions *****************************/

/** Total number of SDMMC interface */
#define BOARD_NUM_SDMMC           2

#define SDMMC_CTRL           (0x000)
#define SDMMC_PWREN          (0x004)
#define SDMMC_CLKDIV         (0x008)
#define SDMMC_CLKSRC         (0x00C)
#define SDMMC_CLKENA         (0x010)
#define SDMMC_TMOUT          (0x014)
#define SDMMC_CTYPE          (0x018)
#define SDMMC_BLKSIZ         (0x01C)
#define SDMMC_BYTCNT         (0x020)
#define SDMMC_INTMASK        (0x024)
#define SDMMC_CMDARG         (0x028)

/*#define SDMMC_CMD            (0x02C)*/
#define SDMMC_CMD_VAL            (0x02C)

#define SDMMC_RESP0          (0x030)
#define SDMMC_RESP1          (0x034)
#define SDMMC_RESP2          (0x038)
#define SDMMC_RESP3          (0x03C)
#define SDMMC_MINTSTS        (0x040)
#define SDMMC_RINTSTS        (0x044)
#define SDMMC_STATUS         (0x048)
#define SDMMC_FIFOTH         (0x04C)
#define SDMMC_CDETECT        (0x050)
#define SDMMC_WRTPRT         (0x054)
#define SDMMC_TCBCNT         (0x05C)
#define SDMMC_TBBCNT         (0x060)
#define SDMMC_DEBNCE         (0x064)
#define SDMMC_USRID          (0x068)
#define SDMMC_VERID          (0x06C)
#define SDMMC_HCON           (0x070)
#define SDMMC_UHS_REG        (0x074)
#define SDMMC_BMOD           (0x080)
#define SDMMC_IDINTEN        (0x090)
#define SDMMC_FIFO           (0x100)

/* CTRL - Control Register */
#define ENABLE_OD_PULLUP_OFFSET         24
#define CARD_VOLTAGE_B_OFFSET           20
#define CARD_VOLTAGE_A_OFFSET           16
#define CEATA_DEV_INT_ENABLE_OFFSET     11
#define SEND_AUTO_STOP_CCSD_OFFSET      10
#define SEND_CCSD_OFFSET                9
#define ABORT_READ_DATA_OFFSET          8
#define SEND_IRQ_RESPONSE_OFFSET        7
#define ASSERT_READ_WAIT_OFFSET         6
#define DMA_ENABLE_OFFSET               5
#define INT_ENABLE_OFFSET               4
#define DMA_RESET_OFFSET                2
#define FIFO_RESET_OFFSET               1
#define CONTROLLER_RESET_OFFSET         0

#define ENABLE_OD_PULLUP_MASK           0x1
#define CARD_VOLTAGE_B_MASK             0xF
#define CARD_VOLTAGE_A_MASK             0xF
#define CEATA_DEV_INT_ENABLE_MASK       0x1
#define SEND_AUTO_STOP_CCSD_MASK        0x1
#define SEND_CCSD_MASK                  0x1
#define ABORT_READ_DATA_MASK            0x1
#define SEND_IRQ_RESPONSE_MASK          0x1
#define ASSERT_READ_WAIT_MASK           0x1
#define DMA_ENABLE_MASK                 0x1
#define INT_ENABLE_MASK                 0x1
#define DMA_RESET_MASK                  0x1
#define FIFO_RESET_MASK                 0x1
#define CONTROLLER_RESET_MASK           0x1

#define ENABLE_OD_PULLUP                0x01000000
#define CARD_VOLTAGE_B                  0x00F00000
#define CARD_VOLTAGE_A                  0x000F0000
#define CEATA_DEV_INT_ENABLE            0x00000800
#define SEND_AUTO_STOP_CCSD             0x00000400
#define SEND_CCSD                       0x00000200
#define ABORT_READ_DATA                 0x00000100
#define SEND_IRQ_RESPONSE               0x00000080
#define ASSERT_READ_WAIT                0x00000040
#define DMA_ENABLE                      0x00000020
#define INT_ENABLE                      0x00000010
#define DMA_RESET                       0x00000004
#define FIFO_RESET                      0x00000002
#define CONTROLLER_RESET                0x00000001

/* PWREN - Power Enable Register */
#define POWER_ENABLE                    0x00000001

/* CLKDIV - Clock Divider Register */
#define CLK_DIVIDER3_OFFSET             24
#define CLK_DIVIDER2_OFFSET             16
#define CLK_DIVIDER1_OFFSET             8
#define CLK_DIVIDER0_OFFSET             0

#define CLK_DIVIDER3_MASK               0xFF
#define CLK_DIVIDER2_MASK               0xFF
#define CLK_DIVIDER1_MASK               0xFF
#define CLK_DIVIDER0_MASK               0xFF

#define CLK_DIVIDER3                    0xFF000000
#define CLK_DIVIDER2                    0x00FF0000
#define CLK_DIVIDER1                    0x0000FF00
#define CLK_DIVIDER0                    0x000000FF

/* CLKSRC - SD Clock Source Register */
#define CLK_SOURCE                      0x00000003
#define CLK_SOURCE_DIVIDER0             0x0
#define CLK_SOURCE_DIVIDER1             0x1
#define CLK_SOURCE_DIVIDER2             0x2
#define CLK_SOURCE_DIVIDER3             0x3

/* CLKENA - Clock Enable Register */
#define CCLK_LOW_POWER_OFFSET           16
#define CCLK_ENABLE_OFFSET              0

#define CCLK_LOW_POWER_MASK             0x1
#define CCLK_ENABLE_MASK                0x1

#define CCLK_LOW_POWER                  0x00010000
#define CCLK_ENABLE                     0x00000001

/* TMOUT - Timeout Register */
#define DATA_TIMEOUT_OFFSET             8
#define RESPONSE_TIMEOUT_OFFSET         0

#define DATA_TIMEOUT_MASK               0xFFFFFF
#define RESPONSE_TIMEOUT_MASK           0xFF

#define DATA_TIMEOUT                    0xFFFFFF00
#define RESPONSE_TIMEOUT                0x000000FF

/* CTYPE - Card Type Register */
#define CARD_8B_WIDTH_OFFSET            16
#define CARD_4B_WIDTH_OFFSET            0

#define CARD_8B_WIDTH_MASK              0x1
#define CARD_4B_WIDTH_MASK              0x1

#define CARD_8B_WIDTH                   0x00010000
#define CARD_4B_WIDTH                   0x00000001

/* BLKSIZ - Block Size Register */
#define BLOCK_SIZE_OFFSET               0

#define BLOCK_SIZE_MASK                 0xFFFF

#undef BLOCK_SIZE 
#define BLOCK_SIZE                      0x0000FFFF

/* BYTCNT - Byte Counter Register */
#define BYTE_COUNT_OFFSET               0

#define BYTE_COUNT_MASK                 0xFFFFFFFF

#define BYTE_COUNT                      0xFFFFFFFF

/* INTMASK - Interrupt Mask Register */
#define SDIO_INT_MASK_OFFSET            16
#define INT_MASK_OFFSET                 0

#define SDIO_INT_MASK_MASK              0x1
#define INT_MASK_MASK                   0xFFFF

#define SDIO_INT_MASK                   0x00010000
#define INT_MASK                        0x0000FFFF
    #define INT_MASK_EBE                    0x8000
    #define INT_MASK_ACD                    0x4000
    #define INT_MASK_SBE                    0x2000
    #define INT_MASK_HLE                    0x1000
    #define INT_MASK_FRUN                   0x0800
    #define INT_MASK_HTO                    0x0400
    #define INT_MASK_DRTO                   0x0200
    #define INT_MASK_RTO                    0x0100
    #define INT_MASK_DCRC                   0x0080
    #define INT_MASK_RCRC                   0x0040
    #define INT_MASK_RXDR                   0x0020
    #define INT_MASK_TXDR                   0x0010
    #define INT_MASK_DTO                    0x0008
    #define INT_MASK_CMDD                   0x0004
    #define INT_MASK_RE                     0x0002
    #define INT_MASK_CD                     0x0001

/* CMDARG - Command Argument Register */
#define CMD_ARG                         0xFFFFFFFF

/* CMD - Command Register */
#define START_CMD_OFFSET                31
#define VOLT_SWITCH_OFFSET              28
#define BOOT_MODE_OFFSET                27
#define DISABLE_BOOT_OFFSET             26
#define EXPECT_BOOT_ACK_OFFSET          25
#define ENABLE_BOOT_OFFSET              24
#define CCS_EXPECTED_OFFSET             23
#define READ_CEATA_DEVICE_OFFSET        22
#define UPDATE_CLOCK_REG_ONLY_OFFSET    21
#define CARD_NUMBER_OFFSET              16
#define SEND_INITIALIZATION_OFFSET      15
#define STOP_ABORT_CMD_OFFSET           14
#define WAIT_PRVDATA_COMPLETE_OFFSET    13
#define SEND_AUTO_STOP_OFFSET           12
#define TRANSFER_MODE_OFFSET            11
#define NREADWRITE_OFFSET               10
#define DATA_EXPECTED_OFFSET            9
#define CHECK_RESPONSE_CRC_OFFSET       8
#define RESPONSE_LENGTH_LONG_OFFSET     7
#define RESPONSE_EXPECT_OFFSET          6
#define CMD_INDEX_OFFSET                0

#define START_CMD_MASK                  0x1
#define VOLT_SWITCH_MASK                0x1
#define BOOT_MODE_MASK                  0x1
#define DISABLE_BOOT_MASK               0x1
#define EXPECT_BOOT_ACK_MASK            0x1
#define ENABLE_BOOT_MASK                0x1
#define CCS_EXPECTED_MASK               0x1
#define READ_CEATA_DEVICE_MASK          0x1
#define UPDATE_CLOCK_REG_ONLY_MASK      0x1
#define CARD_NUMBER_MASK                0x1F
#define SEND_INITIALIZATION_MASK        0x1
#define STOP_ABORT_CMD_MASK             0x1
#define WAIT_PRVDATA_COMPLETE_MASK      0x1
#define SEND_AUTO_STOP_MASK             0x1
#define TRANSFER_MODE_MASK              0x1
#define NREADWRITE_MASK                 0x1
#define DATA_EXPECTED_MASK              0x1
#define CHECK_RESPONSE_CRC_MASK         0x1
#define RESPONSE_LENGTH_LONG_MASK       0x1
#define RESPONSE_EXPECT_MASK            0x1
#define CMD_INDEX_MASK                  0x3F

#define START_CMD                       0x80000000
#define VOLT_SWITCH                     0x10000000
#define BOOT_MODE                       0x08000000
#define DISABLE_BOOT                    0x04000000
#define EXPECT_BOOT_ACK                 0x02000000
#define ENABLE_BOOT                     0x01000000
#define CCS_EXPECTED                    0x00800000
#define READ_CEATA_DEVICE               0x00400000
#define UPDATE_CLOCK_REG_ONLY           0x00200000
#define CARD_NUMBER                     0x001F0000
#define SEND_INITIALIZATION             0x00008000
#define STOP_ABORT_CMD                  0x00004000
#define WAIT_PRVDATA_COMPLETE           0x00002000
#define SEND_AUTO_STOP                  0x00001000
#define TRANSFER_MODE		            0x00000800
#define NREADWRITE                      0x00000400
#define DATA_EXPECTED                   0x00000200
#define CHECK_RESPONSE_CRC              0x00000100
#define RESPONSE_LENGTH_LONG            0x00000080
#define RESPONSE_EXPECT                 0x00000040
#define CMD_INDEX                       0x0000003F

/* RESP0 - Response Register 0 */
#define RESPONSE0                       0xFFFFFFFF

/* RESP1 - Response Register 1 */
#define RESPONSE1                       0xFFFFFFFF

/* RESP2 - Response Register 2 */
#define RESPONSE2                       0xFFFFFFFF

/* RESP3 - Response Register 3 */
#define RESPONSE3                       0xFFFFFFFF

/* MINTSTS - Masked Interrupt Status Register */
#define MASK_SDIO_INTERRUPT_OFFSET      16
#define MASK_INT_STATUS_OFFSET          0

#define MASK_SDIO_INTERRUPT_MASK        0x1
#define MASK_INT_STATUS_MASK            0xFFFF

#define MASK_SDIO_INTERRUPT             0x00010000
#define MASK_INT_STATUS                 0x0000FFFF
    #define MASK_INT_STATUS_EBE             0x8000
    #define MASK_INT_STATUS_ACD             0x4000
    #define MASK_INT_STATUS_SBE             0x2000
    #define MASK_INT_STATUS_HLE             0x1000
    #define MASK_INT_STATUS_FRUM            0x0800
    #define MASK_INT_STATUS_HTO             0x0400
    #define MASK_INT_STATUS_DRTO            0x0200
    #define MASK_INT_STATUS_RTO             0x0100
    #define MASK_INT_STATUS_DCRC            0x0080
    #define MASK_INT_STATUS_RCRC            0x0040
    #define MASK_INT_STATUS_RXDR            0x0020
    #define MASK_INT_STATUS_TXDR            0x0010
    #define MASK_INT_STATUS_DTO             0x0008
    #define MASK_INT_STATUS_CMDD            0x0004
    #define MASK_INT_STATUS_RE              0x0002
    #define MASK_INT_STATUS_CD              0x0001

/* RINTSTS - Raw Interrupt Status Register */
#define RAW_SDIO_INTERRUPT_OFFSET       16
#define RAW_INT_STATUS_OFFSET           0

#define RAW_SDIO_INTERRUPT_MASK         0x1
#define RAW_INT_STATUS_MASK             0xFFFF

#define RAW_SDIO_INTERRUPT              0x00010000
#define RAW_INT_STATUS                  0x0000FFFF
    #define RAW_INT_STATUS_EBE              0x8000
    #define RAW_INT_STATUS_ACD              0x4000
    #define RAW_INT_STATUS_SBE              0x2000
    #define RAW_INT_STATUS_HLE              0x1000
    #define RAW_INT_STATUS_FRUM             0x0800
    #define RAW_INT_STATUS_HTO              0x0400
    #define RAW_INT_STATUS_DRTO             0x0200
    #define RAW_INT_STATUS_RTO              0x0100
    #define RAW_INT_STATUS_DCRC             0x0080
    #define RAW_INT_STATUS_RCRC             0x0040
    #define RAW_INT_STATUS_RXDR             0x0020
    #define RAW_INT_STATUS_TXDR             0x0010
    #define RAW_INT_STATUS_DTO              0x0008
    #define RAW_INT_STATUS_CMD_DONE         0x0004
    #define RAW_INT_STATUS_RE               0x0002
    #define RAW_INT_STATUS_CD               0x0001

/** Bit mask for status register errors. */
#define INT_STATUS_ERRORS ((uint32_t)(RAW_INT_STATUS_RE  \
                           | RAW_INT_STATUS_RCRC \
                           | RAW_INT_STATUS_DCRC \
                           | RAW_INT_STATUS_RTO \
                           | RAW_INT_STATUS_DRTO \
                           | RAW_INT_STATUS_HTO \
                           | RAW_INT_STATUS_FRUM \
                           | RAW_INT_STATUS_HLE \
                           | RAW_INT_STATUS_SBE \
                           | RAW_INT_STATUS_EBE))


/* STATUS - Status Register */
#define DMA_REQ_OFFSET                  31
#define DMA_ACK_OFFSET                  30
#define FIFO_COUNT_OFFSET               17
#define RESPONSE_INDEX_OFFSET           11
#define DATA_STATE_MC_BUSY_OFFSET       10
#define DATA_BUSY_OFFSET                9
#define DATA_3_STATUS_OFFSET            8
#define CMD_FSM_STATUS_OFFSET           4
#define FIFO_FULL_OFFSET                3
#define FIFO_EMPTY_OFFSET               2
#define FIFO_TX_WATERMARK_OFFSET        1
#define FIFO_RX_WATERMARK_OFFSET        0

#define DMA_REQ_MASK                    0x1
#define DMA_ACK_MASK                    0x1
#define FIFO_COUNT_MASK                 0x1FFF
#define RESPONSE_INDEX_MASK             0x3F
#define DATA_STATE_MC_BUSY_MASK         0x1
#define DATA_BUSY_MASK                  0x1
#define DATA_3_STATUS_MASK              0x1
#define CMD_FSM_STATUS_MASK             0xF
#define FIFO_FULL_MASK                  0x1
#define FIFO_EMPTY_MASK                 0x1
#define FIFO_TX_WATERMARK_MASK          0x1
#define FIFO_RX_WATERMARK_MASK          0x1

#define DMA_REQ                         0x80000000
#define DMA_ACK                         0x40000000
#define FIFO_COUNT                      0x3FFE0000
#define RESPONSE_INDEX                  0x0001F800
#define DATA_STATE_MC_BUSY              0x00000400
#define DATA_BUSY                       0x00000200
#define DATA_3_STATUS                   0x00000100
#define CMD_FSM_STATUS                  0x000000F0
#define CMD_FSM_STATUS_IDLE             0x00000000
#define CMD_FSM_STATUS_SIS              0x00000010
#define CMD_FSM_STATUS_TCSB             0x00000020
#define CMD_FSM_STATUS_TCTB             0x00000030
#define CMD_FSM_STATUS_TCIA             0x00000040
#define CMD_FSM_STATUS_TCC7             0x00000050
#define CMD_FSM_STATUS_TCEB             0x00000060
#define CMD_FSM_STATUS_RRSB             0x00000070
#define CMD_FSM_STATUS_RRIRQR           0x00000080
#define CMD_FSM_STATUS_RRTB             0x00000090
#define CMD_FSM_STATUS_RRCI             0x000000A0
#define CMD_FSM_STATUS_RRD              0x000000B0
#define CMD_FSM_STATUS_RRC7             0x000000C0
#define CMD_FSM_STATUS_RREB             0x000000D0
#define CMD_FSM_STATUS_CPWNCC           0x000000E0
#define CMD_FSM_STATUS_WAIT             0x000000F0
#define FIFO_FULL                       0x00000008
#define FIFO_EMPTY                      0x00000004
#define FIFO_TX_WATERMARK               0x00000002
#define FIFO_RX_WATERMARK               0x00000001

/* FIFOTH - FIFO Threshold Watermark Register */
#define DW_DMA_MULTI_TRAN_SIZE_OFFSET   28
#define RX_WMARK_OFFSET                 16
#define TX_WMARK_OFFSET                 0

#define DW_DMA_MULTI_TRAN_SIZE_MASK     0x7
#define RX_WMARK_MASK                   0xFFF
#define TX_WMARK_MASK                   0xFFF

#define DW_DMA_MULTI_TRAN_SIZE          0x70000000
#define DW_DMA_MULTI_TRAN_SIZE_1        0x00000000
#define DW_DMA_MULTI_TRAN_SIZE_4        0x10000000
#define DW_DMA_MULTI_TRAN_SIZE_8        0x20000000
#define DW_DMA_MULTI_TRAN_SIZE_16       0x30000000
#define DW_DMA_MULTI_TRAN_SIZE_32       0x40000000
#define DW_DMA_MULTI_TRAN_SIZE_64       0x50000000
#define DW_DMA_MULTI_TRAN_SIZE_128      0x60000000
#define DW_DMA_MULTI_TRAN_SIZE_256      0x70000000
#define RX_WMARK                        0x0FFF0000
#define TX_WMARK                        0x00000FFF

#define SD_FIFOTH_DW_DMA_MULTI_TRAN_SIZE    (0x7 << 28)
#define SD_FIFOTH_RX_WMARK			(0xFFF << 16)
#define SD_FIFOTH_TX_WMARK			(0xFFF << 0)


/* CDETECT - Card Detect Register */
#define CARD_DETECT                     0x00000001

/* WRTPRT - Write Protect Register */
#define WRITE_PROTECT                   0x00000001

/* GPIO - General Purpose Input/Output Register */

/* TCBCNT - Transferred CIU Card Byte Count Register */
/*#define TRANS_CARD_BYTE_COUNT           0xFFFFFFFF */

/* TBBCNT - Transferred Host to BIU-FIFO Count Register */
/*#define TRANS_FIFO_BYTE_COUNT           0xFFFFFFFF */

/* DEBNCE - Debounce Count Register */
/*#define DEBOUNCE_COUNT                  0x00FFFFFF */

/* USRID - User ID Register */
/*#define USRID                           0xFFFFFFFF */

/* VERID - Version ID Register */
/*#define VERID                           0xFFFFFFFF */

/* HCON - Hardware Configuration Register */
#define CARD_TYPE_OFFSET                0
#define H_BUS_TYPE_OFFSET               6
#define H_DATA_WIDTH_OFFSET             7
#define H_ADDR_WIDTH_OFFSET             10
#define DMA_INTERFACE_OFFSET            16
#define GE_DMA_DATA_WIDTH_OFFSET        18
#define FIFO_RAM_INSIDE_OFFSET          21
#define IMPLEMENT_HOLD_REG_OFFSET       22
#define SET_CLK_FALSE_PATH_OFFSET       23
#define NUM_CLK_DIVIDER_M1_OFFSET       24
#define AREA_OPTIMIZED_OFFSET           26

#define CARD_TYPE_MASK                  0x1
#define H_BUS_TYPE_MASK                 0x1
#define H_DATA_WIDTH_MASK               0x7
#define H_ADDR_WIDTH_MASK               0x3F
#define DMA_INTERFACE_MASK              0x3
#define GE_DMA_DATA_WIDTH_MASK          0x7
#define FIFO_RAM_INSIDE_MASK            0x1
#define IMPLEMENT_HOLD_REG_MASK         0x1
#define SET_CLK_FALSE_PATH_MASK         0x1
#define NUM_CLK_DIVIDER_M1_MASK         0x3
#define AREA_OPTIMIZED_MASK             0x1


#define CARD_TYPE                       0x00000001
    #define CARD_TYPE_MMC                   0x00000000
    #define CARD_TYPE_SDMMC                 0x00000001
#define H_BUS_TYPE                      0x00000040
    #define H_BUS_TYPE_APB                  0x00000000
    #define H_BUS_TYPE_AHB                  0x00000040
#define H_DATA_WIDTH                    0x00000380
    #define H_DATA_WIDTH_16BITS             0x00000000
    #define H_DATA_WIDTH_32BITS             0x00000080
    #define H_DATA_WIDTH_64BITS             0x00000100
#define H_ADDR_WIDTH                    0x0000FC00
    #define H_ADDR_WIDTH_9BITS              0x00002000
    #define H_ADDR_WIDTH_10BITS             0x00002400
    #define H_ADDR_WIDTH_11BITS             0x00002800
    #define H_ADDR_WIDTH_12BITS             0x00002C00
    #define H_ADDR_WIDTH_13BITS             0x00003000
    #define H_ADDR_WIDTH_14BITS             0x00003400
    #define H_ADDR_WIDTH_15BITS             0x00003800
    #define H_ADDR_WIDTH_16BITS             0x00003C00
    #define H_ADDR_WIDTH_17BITS             0x00004000
    #define H_ADDR_WIDTH_18BITS             0x00004400
    #define H_ADDR_WIDTH_19BITS             0x00004800
    #define H_ADDR_WIDTH_20BITS             0x00004C00
    #define H_ADDR_WIDTH_21BITS             0x00005000
    #define H_ADDR_WIDTH_22BITS             0x00005400
    #define H_ADDR_WIDTH_23BITS             0x00005800
    #define H_ADDR_WIDTH_24BITS             0x00005C00
    #define H_ADDR_WIDTH_25BITS             0x00006000
    #define H_ADDR_WIDTH_26BITS             0x00006400
    #define H_ADDR_WIDTH_27BITS             0x00006800
    #define H_ADDR_WIDTH_28BITS             0x00006C00
    #define H_ADDR_WIDTH_29BITS             0x00007000
    #define H_ADDR_WIDTH_30BITS             0x00007400
    #define H_ADDR_WIDTH_31BITS             0x00007800
    #define H_ADDR_WIDTH_32BITS             0x00007C00
#define DMA_INTERFACE                   0x00030000
    #define DMA_INTERFACE_DW_DMA            0x00010000
    #define DMA_INTERFACE_GENERIC_DMA       0x00020000
#define GE_DMA_DATA_WIDTH               0x001C0000
    #define GE_DMA_DATA_WIDTH_16BITS        0x00000000
    #define GE_DMA_DATA_WIDTH_32BITS        0x00040000
    #define GE_DMA_DATA_WIDTH_64BITS        0x00080000
#define FIFO_RAM_INSIDE                 0x00200000
#define IMPLEMENT_HOLD_REG              0x00400000
#define SET_CLK_FALSE_PATH              0x00800000
#define NUM_CLK_DIVIDER_M1              0x03000000
#define AREA_OPTIMIZED                  0x04000000
	
/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef _FMSH_SDMMC_HW_H_ */
