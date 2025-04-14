/* vxbFdtfmqlAxiEnd.h - Xilinx Zynq-7000 AXI ETHERNET VxBus End header file */

/*
 * Copyright (c) 2014-2016 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */
 
/*
modification history
--------------------
22jan16,m_w  fix multicast filter issue. (V7PRO-2789)
23oct15,m_w  update poll mode function. (V7PRO-2525)
24apr15,y_c  increased AXI_TUPLE_CNT value. (V7PRO-1992) 
14feb15,yjw  added hostClock in struct axieth_drv_ctrl (V7PRO-1751)
18jun14,y_c  created from vxbfmqlAxiEnd.h. (US40549)
*/

/*
DESCRIPTION
This module contains constants and defines for the Xilinx Zynq-7000 AXI End.
*/

#ifndef __INCvxbfmqlAxiEndh
#define __INCvxbfmqlAxiEndh

#ifdef __cplusplus
extern "C" {
#endif

/* 
forward declarations 
*/
IMPORT void fmqlAxiEndRegister(void);


#define AXI_END_NAME                "axienet"

#define AXI_RX_DESC_CNT         256
#define AXI_TX_DESC_CNT         256

#define AXI_TUPLE_CNT           1152

#define AXI_DBG_USE_APP0_TXMBLK

#define AXI_MTU                 8132
#define AXI_CLSIZE              (AXI_MTU + 36)

#define MAX_MDIO_FREQ           400000 /* 400KHz */
#define HOST_CLOCK              76923080
#define AXI_MAXFRAG             16
#define AXI_MAX_RX              64
#define AXI_TIMEOUT             10000
#define AXI_PHY_TIMEOUT         (20 * 1000)

#define AXI_INC_DESC(x, y)      (x) = (((x) + 1) % y)

/* Defines */

#define ETHER_ADDR_LEN          6

/* Packet size info */

#define AXI_ENET_HDR_SIZE       14   /* Size of Ethernet header         */
#define AXI_ENET_HDR_VLAN_SIZE  18   /* Size of an Ethernet hdr + VLAN  */
#define AXI_ENET_TRL_SIZE       4    /* Size of Ethernet trailer (FCS)  */
#define AXI_ENET_MTU            1500 /* Max MTU of an Ethernet frame    */
#define AXI_ENET_JUMBO_MTU      9000 /* Max MTU of a jumbo Eth. frame   */

#define AXI_ENET_MAX_FRAME_SIZE         (AXI_ENET_MTU +           \
                                         AXI_ENET_HDR_SIZE +      \
                                         AXI_ENET_TRL_SIZE)
#define AXI_ENET_MAX_VLAN_FRAME_SIZE    (AXI_ENET_MTU +           \
                                         AXI_ENET_HDR_VLAN_SIZE + \
                                         AXI_ENET_TRL_SIZE)
#define AXI_ENET_MAX_JUMBO_FRAME_SIZE   (AXI_ENET_JUMBO_MTU +     \
                                         AXI_ENET_HDR_SIZE +      \
                                         AXI_ENET_TRL_SIZE)

/* Configuration options */

/* Accept all incoming packets. */

#define AXI_ENET_OPTION_PROMISC         (1 << 0)

/* Jumbo frame support for Tx & Rx. */

#define AXI_ENET_OPTION_JUMBO           (1 << 1)

/* VLAN Rx & Tx frame support. */

#define AXI_ENET_OPTION_VLAN            (1 << 2)

/* Enable recognition of flow control frames on Rx. */

#define AXI_ENET_OPTION_FLOW_CONTROL    (1 << 4)

/* 
 * Strip FCS and PAD from incoming frames. PAD 
 * from VLAN frames is not stripped.
 */
 
#define AXI_ENET_OPTION_FCS_STRIP       (1 << 5)

/* Generate FCS field and add PAD automatically for outgoing frames. */

#define AXI_ENET_OPTION_FCS_INSERT      (1 << 6)

/* 
 * Enable Length/Type error checking for incoming frames. When this option is
 * set, the MAC will filter frames that have a mismatched type/length field
 * and if AXI_ENET_OPTION_REPORT_RXERR is set, the user is notified when these
 * types of frames are encountered. When this option is cleared, the MAC will
 * allow these types of frames to be received.
 */
 
#define AXI_ENET_OPTION_LENTYPE_ERR     (1 << 7)

/* Enable the transmitter. */

#define AXI_ENET_OPTION_TXEN            (1 << 11)

/*  Enable the receiver. */

#define AXI_ENET_OPTION_RXEN            (1 << 12)

/* Axi DMA Register definitions */

#define AXI_DMA_TX_CR_OFFSET            0x00000000  /* Channel control          */
#define AXI_DMA_TX_SR_OFFSET            0x00000004  /* Status                   */
#define AXI_DMA_TX_CDESC_OFFSET         0x00000008  /* Current descriptor ptr   */
#define AXI_DMA_TX_TDESC_OFFSET         0x00000010  /* Tail descriptor pointer  */

#define AXI_DMA_RX_CR_OFFSET            0x00000030  /* Channel control          */
#define AXI_DMA_RX_SR_OFFSET            0x00000034  /* Status                   */
#define AXI_DMA_RX_CDESC_OFFSET         0x00000038  /* Current descriptor ptr   */
#define AXI_DMA_RX_TDESC_OFFSET         0x00000040  /* Tail descriptor pointer  */

#define AXI_DMA_CR_RUNSTOP_MASK         0x00000001  /* Start/stop DMA channel   */
#define AXI_DMA_CR_RESET_MASK           0x00000004  /* Reset DMA engine         */

#define AXI_DMA_BD_NDESC_OFFSET         0x00    /* Next descriptor pointer  */
#define AXI_DMA_BD_BUFA_OFFSET          0x08    /* Buffer address           */
#define AXI_DMA_BD_CTRL_LEN_OFFSET      0x18    /* Control/buffer length    */
#define AXI_DMA_BD_STS_OFFSET           0x1C    /* Status                   */
#define AXI_DMA_BD_USR0_OFFSET          0x20    /* User IP specific word0   */
#define AXI_DMA_BD_USR1_OFFSET          0x24    /* User IP specific word1   */
#define AXI_DMA_BD_USR2_OFFSET          0x28    /* User IP specific word2   */
#define AXI_DMA_BD_USR3_OFFSET          0x2C    /* User IP specific word3   */
#define AXI_DMA_BD_USR4_OFFSET          0x30    /* User IP specific word4   */
#define AXI_DMA_BD_ID_OFFSET            0x34    /* AXI DMA v7.1: not used   */
#define AXI_DMA_BD_HAS_STSCNTRL_OFFSET  0x38    /* AXI DMA v7.1: not used   */
#define AXI_DMA_BD_HAS_DRE_OFFSET       0x3C    /* AXI DMA v7.1: not used   */

#define AXI_DMA_BD_HAS_DRE_SHIFT        8       /* Whether has DRE shift    */
#define AXI_DMA_BD_HAS_DRE_MASK         0xF00   /* Whether has DRE mask     */
#define AXI_DMA_BD_WORDLEN_MASK         0xFF    /* Whether has DRE mask     */

#define AXI_DMA_BD_CTRL_LENGTH_MASK     0x007FFFFF  /* Requested len            */
#define AXI_DMA_BD_CTRL_TXSOF_MASK      0x08000000  /* First tx packet          */
#define AXI_DMA_BD_CTRL_TXEOF_MASK      0x04000000  /* Last tx packet           */
#define AXI_DMA_BD_CTRL_ALL_MASK        0x0C000000  /* All control bits         */

#define AXI_DMA_IRQ_IOC_MASK            0x00001000  /* Completion intr          */
#define AXI_DMA_IRQ_DELAY_MASK          0x00002000  /* Delay interrupt          */
#define AXI_DMA_IRQ_ERROR_MASK          0x00004000  /* Error interrupt          */
#define AXI_DMA_IRQ_ALL_MASK            0x00007000  /* All interrupts           */

/* Default TX/RX Threshold and waitbound values for SGDMA mode */

#define AXI_DMA_DFT_TX_WAITBOUND        254
#define AXI_DMA_DFT_RX_THRESHOLD        24
#define AXI_DMA_DFT_RX_WAITBOUND        254

#define AXI_DMA_BD_CTRL_TXSOF_MASK      0x08000000  /* First tx packet          */
#define AXI_DMA_BD_CTRL_TXEOF_MASK      0x04000000  /* Last tx packet           */
#define AXI_DMA_BD_CTRL_ALL_MASK        0x0C000000  /* All control bits         */

#define AXI_DMA_BD_STS_ACTUAL_LEN_MASK  0x007FFFFF /* Actual len            */
#define AXI_DMA_BD_STS_COMPLETE_MASK    0x80000000 /* Completed             */
#define AXI_DMA_BD_STS_DEC_ERR_MASK     0x40000000 /* Decode error             */
#define AXI_DMA_BD_STS_SLV_ERR_MASK     0x20000000 /* Slave error              */
#define AXI_DMA_BD_STS_INT_ERR_MASK     0x10000000 /* Internal err             */
#define AXI_DMA_BD_STS_ALL_ERR_MASK     0x70000000 /* All errors               */
#define AXI_DMA_BD_STS_RXSOF_MASK       0x08000000 /* First rx pkt             */
#define AXI_DMA_BD_STS_RXEOF_MASK       0x04000000 /* Last rx pkt              */
#define AXI_DMA_BD_STS_ALL_MASK         0xFC000000 /* All status bits          */

#define AXI_DMA_BD_MINIMUM_ALIGNMENT        0x40

#define AXI_ENET_MULTICAST_CAM_TABLE_NUM    4

/* Axi Ethernet registers definition */

#define AXI_ENET_RAF_OFFSET             0x00000000 /* Reset and Address filter */
#define AXI_ENET_TPF_OFFSET             0x00000004 /* Tx Pause Frame */
#define AXI_ENET_IFGP_OFFSET            0x00000008 /* Tx Inter-frame gap adjustment*/
#define AXI_ENET_IS_OFFSET              0x0000000C /* Interrupt status */
#define AXI_ENET_IP_OFFSET              0x00000010 /* Interrupt pending */
#define AXI_ENET_IE_OFFSET              0x00000014 /* Interrupt enable */
#define AXI_ENET_TTAG_OFFSET            0x00000018 /* Tx VLAN TAG */
#define AXI_ENET_RTAG_OFFSET            0x0000001C /* Rx VLAN TAG */
#define AXI_ENET_UAWL_OFFSET            0x00000020 /* Unicast address word lower */
#define AXI_ENET_UAWU_OFFSET            0x00000024 /* Unicast address word upper */
#define AXI_ENET_TPID0_OFFSET           0x00000028 /* VLAN TPID0 register */
#define AXI_ENET_TPID1_OFFSET           0x0000002C /* VLAN TPID1 register */
#define AXI_ENET_PPST_OFFSET            0x00000030 /* PCS PMA Soft Temac Status Reg */
#define AXI_ENET_RCW0_OFFSET            0x00000400 /* Rx Configuration Word 0 */
#define AXI_ENET_RCW1_OFFSET            0x00000404 /* Rx Configuration Word 1 */
#define AXI_ENET_TC_OFFSET              0x00000408 /* Tx Configuration */
#define AXI_ENET_FCC_OFFSET             0x0000040C /* Flow Control Configuration */
#define AXI_ENET_EMMC_OFFSET            0x00000410 /* EMAC mode configuration */
#define AXI_ENET_PHYC_OFFSET            0x00000414 /* RGMII/SGMII configuration */
#define AXI_ENET_MDIO_MC_OFFSET         0x00000500 /* MII Management Config */
#define AXI_ENET_MDIO_MCR_OFFSET        0x00000504 /* MII Management Control */
#define AXI_ENET_MDIO_MWD_OFFSET        0x00000508 /* MII Managemenft Write Data */
#define AXI_ENET_MDIO_MRD_OFFSET        0x0000050C /* MII Management Read Data */
#define AXI_ENET_MDIO_MIS_OFFSET        0x00000600 /* MII Management Interrupt Status */
#define AXI_ENET_MDIO_MIP_OFFSET        0x00000620 /* MII Mgmt Interrupt Pending register offset */
#define AXI_ENET_MDIO_MIE_OFFSET        0x00000640 /* MII Management Interrupt Enable register offset */
#define AXI_ENET_MDIO_MIC_OFFSET        0x00000660 /* MII Management Interrupt Clear register offset. */
#define AXI_ENET_UAW0_OFFSET            0x00000700 /* Unicast address word 0 */
#define AXI_ENET_UAW1_OFFSET            0x00000704 /* Unicast address word 1 */
#define AXI_ENET_FMI_OFFSET             0x00000708 /* Filter Mask Index */
#define AXI_ENET_AF0_OFFSET             0x00000710 /* Address Filter 0 */
#define AXI_ENET_AF1_OFFSET             0x00000714 /* Address Filter 1 */

#define AXI_ENET_TX_VLAN_DATA_OFFSET    0x00004000 /* TX VLAN data table address */
#define AXI_ENET_RX_VLAN_DATA_OFFSET    0x00008000 /* RX VLAN data table address */
#define AXI_ENET_MCAST_TABLE_OFFSET     0x00020000 /* Multicast table address */

/* Bit Masks for Axi Ethernet RAF register */

#define AXI_ENET_RAF_MCSTREJ_MASK       0x00000002  /* Reject receive multicast destination address */
#define AXI_ENET_RAF_BCSTREJ_MASK       0x00000004  /* Reject receive broadcast destination address */
#define AXI_ENET_RAF_TXVTAGMODE_MASK    0x00000018  /* Tx VLAN TAG mode */
#define AXI_ENET_RAF_RXVTAGMODE_MASK    0x00000060  /* Rx VLAN TAG mode */
#define AXI_ENET_RAF_TXVSTRPMODE_MASK   0x00000180  /* Tx VLAN STRIP mode */
#define AXI_ENET_RAF_RXVSTRPMODE_MASK   0x00000600  /* Rx VLAN STRIP mode */
#define AXI_ENET_RAF_NEWFNCENBL_MASK    0x00000800  /* New function mode */
#define AXI_ENET_RAF_EMULTIFLTRENBL_MASK 0x00001000 /* Exteneded Multicast Filtering mode */
#define AXI_ENET_RAF_STATSRST_MASK      0x00002000  /* Stats. Counter Reset */
#define AXI_ENET_RAF_RXBADFRMEN_MASK    0x00004000  /* Recv Bad Frame Enable */
#define AXI_ENET_RAF_TXVTAGMODE_SHIFT   3           /* Tx Tag mode shift bits */
#define AXI_ENET_RAF_RXVTAGMODE_SHIFT   5           /* Rx Tag mode shift bits */
#define AXI_ENET_RAF_TXVSTRPMODE_SHIFT  7           /* Tx strip mode shift bits*/
#define AXI_ENET_RAF_RXVSTRPMODE_SHIFT  9           /* Rx Strip mode shift bits*/

/* Bit Masks for Axi Ethernet TPF and IFGP registers */

#define AXI_ENET_TPF_TPFV_MASK          0x0000FFFF  /* Tx pause frame value */
#define AXI_ENET_IFGP0_IFGP_MASK        0x0000007F  /* Transmit inter-frame gap adjustment value */

/* Bit Masks for Axi Ethernet IS, IE and IP registers. */

#define AXI_ENET_INT_HARDACSCMPLT_MASK  0x00000001 /* Hard register access complete */
#define AXI_ENET_INT_AUTONEG_MASK       0x00000002 /* Auto negotiation complete */
#define AXI_ENET_INT_RXCMPIT_MASK       0x00000004 /* Rx complete */
#define AXI_ENET_INT_RXRJECT_MASK       0x00000008 /* Rx frame rejected */
#define AXI_ENET_INT_RXFIFOOVR_MASK     0x00000010 /* Rx fifo overrun */
#define AXI_ENET_INT_TXCMPIT_MASK       0x00000020 /* Tx complete */
#define AXI_ENET_INT_RXDCMLOCK_MASK     0x00000040 /* Rx Dcm Lock */
#define AXI_ENET_INT_MGTRDY_MASK        0x00000080 /* MGT clock Lock */
#define AXI_ENET_INT_PHYRSTCMPLT_MASK   0x00000100 /* Phy Reset complete */
#define AXI_ENET_INT_ALL_MASK           0x0000003F /* All the ints */

/* INT bits that indicate receive errors */

#define AXI_ENET_INT_RECV_ERROR_MASK    (AXI_ENET_INT_RXRJECT_MASK | AXI_ENET_INT_RXFIFOOVR_MASK) 

/* Bit masks for Axi Ethernet VLAN TPID Word 0 register */

#define AXI_ENET_TPID_0_MASK            0x0000FFFF /* TPID 0 */
#define AXI_ENET_TPID_1_MASK            0xFFFF0000 /* TPID 1 */

/* Bit masks for Axi Ethernet VLAN TPID Word 1 register */

#define AXI_ENET_TPID_2_MASK            0x0000FFFF /* TPID 0 */
#define AXI_ENET_TPID_3_MASK            0xFFFF0000 /* TPID 1 */

/* Bit masks for Axi Ethernet RCW1 register */

#define AXI_ENET_RCW1_RST_MASK          0x80000000 /* Reset */
#define AXI_ENET_RCW1_JUM_MASK          0x40000000 /* Jumbo frame enable */
#define AXI_ENET_RCW1_FCS_MASK          0x20000000 /* In-Band FCS enable (FCS not stripped) */
#define AXI_ENET_RCW1_RX_MASK           0x10000000 /* Receiver enable */
#define AXI_ENET_RCW1_VLAN_MASK         0x08000000 /* VLAN frame enable */
#define AXI_ENET_RCW1_LT_DIS_MASK       0x02000000 /* Length/type field valid check disable */
#define AXI_ENET_RCW1_CL_DIS_MASK       0x01000000 /* Control frame Length check disable */
#define AXI_ENET_RCW1_PAUSEADDR_MASK    0x0000FFFF /* Pause frame source address
                                                    * bits [47:32]. Bits [31:0] are
                                                    * stored in register RCW0 */

/* Bit masks for Axi Ethernet TC register */

#define AXI_ENET_TC_RST_MASK            0x80000000 /* Reset                 */
#define AXI_ENET_TC_JUM_MASK            0x40000000 /* Jumbo frame enable    */
#define AXI_ENET_TC_FCS_MASK            0x20000000 /* In-Band FCS enable (FCS not generated)*/
#define AXI_ENET_TC_TX_MASK             0x10000000 /* Transmitter enable    */
#define AXI_ENET_TC_VLAN_MASK           0x08000000 /* VLAN frame enable     */
#define AXI_ENET_TC_IFG_MASK            0x02000000 /* Inter-frame gap adjustment enable */

/* Bit masks for Axi Ethernet FCC register */

#define AXI_ENET_FCC_FCRX_MASK          0x20000000 /* Rx flow control enable */
#define AXI_ENET_FCC_FCTX_MASK          0x40000000 /* Tx flow control enable */

/* Bit masks for Axi Ethernet EMMC register */

#define AXI_ENET_EMMC_LINKSPEED_MASK    0xC0000000 /* Link speed                */
#define AXI_ENET_EMMC_RGMII_MASK        0x20000000 /* RGMII mode enable         */
#define AXI_ENET_EMMC_SGMII_MASK        0x10000000 /* SGMII mode enable         */
#define AXI_ENET_EMMC_GPCS_MASK         0x08000000 /* 1000BaseX mode enable     */
#define AXI_ENET_EMMC_HOST_MASK         0x04000000 /* Host interface enable     */
#define AXI_ENET_EMMC_TX16BIT           0x02000000 /* 16 bit Tx client enable   */
#define AXI_ENET_EMMC_RX16BIT           0x01000000 /* 16 bit Rx client enable   */
#define AXI_ENET_EMMC_LINKSPD_10        0x00000000 /* Link Speed mask for 10 Mbit   */
#define AXI_ENET_EMMC_LINKSPD_100       0x40000000 /* Link Speed mask for 100 Mbit  */
#define AXI_ENET_EMMC_LINKSPD_1000      0x80000000 /* Link Speed mask for 1000 Mbit */

/* Bit masks for Axi Ethernet PHYC register */

#define AXI_ENET_PHYC_SGMIILINKSPEED_MASK   0xC0000000 /* SGMII link speed mask*/
#define AXI_ENET_PHYC_RGMIILINKSPEED_MASK   0x0000000C /* RGMII link speed */
#define AXI_ENET_PHYC_RGMIIHD_MASK          0x00000002 /* RGMII Half-duplex */
#define AXI_ENET_PHYC_RGMIILINK_MASK        0x00000001 /* RGMII link status */
#define AXI_ENET_PHYC_RGLINKSPD_10          0x00000000 /* RGMII link 10 Mbit */
#define AXI_ENET_PHYC_RGLINKSPD_100         0x00000004 /* RGMII link 100 Mbit */
#define AXI_ENET_PHYC_RGLINKSPD_1000        0x00000008 /* RGMII link 1000 Mbit */
#define AXI_ENET_PHYC_SGLINKSPD_10          0x00000000 /* SGMII link 10 Mbit */
#define AXI_ENET_PHYC_SGLINKSPD_100         0x40000000 /* SGMII link 100 Mbit */
#define AXI_ENET_PHYC_SGLINKSPD_1000        0x80000000 /* SGMII link 1000 Mbit */

/* Bit masks for Axi Ethernet MDIO interface MC register */

#define AXI_ENET_MDIO_MC_MDIOEN_MASK        0x00000040 /* MII management enable */
#define AXI_ENET_MDIO_MC_CLOCK_DIVIDE_MAX   0x3F       /* Maximum MDIO divisor */

/* Bit masks for Axi Ethernet MDIO interface MCR register */

#define AXI_ENET_MDIO_MCR_PHYAD_MASK        0x1F000000 /* Phy Address Mask */
#define AXI_ENET_MDIO_MCR_PHYAD_SHIFT       24         /* Phy Address Shift */
#define AXI_ENET_MDIO_MCR_REGAD_MASK        0x001F0000 /* Reg Address Mask */
#define AXI_ENET_MDIO_MCR_REGAD_SHIFT       16         /* Reg Address Shift */
#define AXI_ENET_MDIO_MCR_OP_MASK           0x0000C000 /* Operation Code Mask */
#define AXI_ENET_MDIO_MCR_OP_SHIFT          13         /* Operation Code Shift */
#define AXI_ENET_MDIO_MCR_OP_READ_MASK      0x00008000 /* Op Code Read Mask */
#define AXI_ENET_MDIO_MCR_OP_WRITE_MASK     0x00004000 /* Op Code Write Mask */
#define AXI_ENET_MDIO_MCR_INITIATE_MASK     0x00000800 /* Ready Mask */
#define AXI_ENET_MDIO_MCR_READY_MASK        0x00000080 /* Ready Mask */

/* Bit masks for Axi Ethernet MDIO interface MIS, MIP, MIE, MIC registers */

#define AXI_ENET_MDIO_INT_MIIM_RDY_MASK     0x00000001 /* MIIM Interrupt */

/* 
 * Bit masks for Axi Ethernet UAW1 register, Station address bits [47:32];
 * Station address bits [31:0] are stored in register UAW0.
 */

#define AXI_ENET_UAW1_UNICASTADDR_MASK      0x0000FFFF

/* Bit masks for Axi Ethernet FMI register */

#define AXI_ENET_FMI_PM_MASK                0x80000000 /* Promis. mode enable */
#define AXI_ENET_FMI_IND_MASK               0x00000003 /* Index Mask */

#define AXI_ENET_MDIO_DIV_DFT               29 /* Default MDIO clock divisor */

/* Axi Ethernet Synthesis features */

#define AXI_ENET_FEATURE_PARTIAL_RX_CSUM    (1 << 0)
#define AXI_ENET_FEATURE_PARTIAL_TX_CSUM    (1 << 1)
#define AXI_ENET_FEATURE_FULL_RX_CSUM       (1 << 2)
#define AXI_ENET_FEATURE_FULL_TX_CSUM       (1 << 3)

#define AXI_ENET_NO_CSUM_OFFLOAD            0

#define AXI_ENET_FULL_CSUM_STATUS_MASK      0x00000038
#define AXI_ENET_IP_UDP_CSUM_VALIDATED      0x00000003
#define AXI_ENET_IP_TCP_CSUM_VALIDATED      0x00000002

#define DELAY_OF_ONE_MILLISEC               1000

#define AXI_DMA_TX_THRESHOLD                8

/* CR - Channel Control */

#define AXI_DMA_CR_DELAY_SHIFT      24
#define AXI_DMA_CR_COALESCE_SHIFT   16

#define AXI_DMA_CR_DELAY_MASK       (((UINT32) 0xFF) << AXI_DMA_CR_DELAY_SHIFT)
#define AXI_DMA_CR_COALESCE_MASK    (((UINT32) 0xFF) << AXI_DMA_CR_COALESCE_SHIFT)

/* Private adapter context structure. */

typedef struct axi_desc
{
    UINT32 next;        /* Physical address of next buffer descriptor */
    UINT32 reserved1;
    UINT32 phys;        /* Buffer Address */
    UINT32 reserved2;
    UINT32 reserved3;
    UINT32 reserved4;
    UINT32 cntrl;
    UINT32 status;
    UINT32 app0;
    UINT32 app1;
    UINT32 app2;
    UINT32 app3;
    UINT32 app4;
    UINT32 swIdOffset;
    UINT32 reserved5;
    UINT32 reserved6;
} AXI_DESC;

typedef struct axi_drv_ctrl
{
    END_OBJ             axiEndObj;
    VXB_DEVICE_ID          axiEthDev;
    UINT32              unit;

    void              * axiBar;
    void              * axiDmaBar;

    void              * handle[2];
    //VXB_RESOURCE      * intRes[2];
    FUNCPTR             clkSetup;
    void              * axiMuxDevCookie;

    JOB_QUEUE_ID        axiJobQueue;

    QJOB                axiRxJob;
    atomicVal_t         axiRxPending;
    QJOB                axiTxJob;
    atomicVal_t         axiTxPending;

    BOOL                axiPolling;
    M_BLK_ID            axiPollBuf;

    UINT8               axiEnetMacAddr[ETHER_ADDR_LEN];

    END_CAPABILITIES    axiCaps;
    END_IFDRVCONF       axiEndStatsConf;
    END_IFCOUNTERS      axiEndStatsCounters;

    /* Begin MII/ifmedia required fields. */

    END_MEDIALIST     * axiMediaList;
    END_ERR             axiLastError;
    UINT32              axiCurMedia;
    UINT32              axiCurStatus;
    VXB_DEVICE_ID          axiMiiBus;
    VXB_DEVICE_ID          axiMiiDev;
    FUNCPTR             axiMiiPhyRead;
    FUNCPTR             axiMiiPhyWrite;
    INT32               axiMiiPhyAddr;
	INT32           	miiIfUnit;

    UINT32              axiTxProd;
    UINT32              axiTxCons;
    UINT32              axiTxFree;
    UINT32              axiRxIdx;

    /* End MII/ifmedia required fields */

    AXI_DESC          * axiRxDescMem;
    AXI_DESC          * axiTxDescMem;

    M_BLK_ID            axiTxMblk[AXI_TX_DESC_CNT]; 
    M_BLK_ID            axiRxMblk[AXI_RX_DESC_CNT];

    UINT32              axiTxStall;

    SEM_ID              axiDevSem;

    INT32               axiMaxMtu;
    UINT32              maxFrameSize;
    UINT32              coalesceCountRx;
    UINT32              coalesceCountTx;

    UINT32              hostClock;

    UINT32              axiInErrors;
    UINT32              axiInDiscards;
    UINT32              axiInUcasts;
    UINT32              axiInMcasts;
    UINT32              axiInBcasts;
    UINT32              axiInOctets;
    UINT32              axiOutErrors;
    UINT32              axiOutUcasts;
    UINT32              axiOutMcasts;
    UINT32              axiOutBcasts;
    UINT32              axiOutOctets;    
} AXI_DRV_CTRL;

#define AXI_READ_4(pDrvCtrl, addr)          \
        vxbRead32 ((pDrvCtrl->handle[0]),   \
                (UINT32 *)((char *)(pDrvCtrl->axiBar) + addr))  

#define AXI_WRITE_4(pDrvCtrl, addr, data)   \
        vxbWrite32((pDrvCtrl->handle[0]),   \
                (UINT32 *)((char *)(pDrvCtrl->axiBar) + addr), data)

#define AXI_SETBIT_4(pDrvCtrl, addr, data)  \
        AXI_WRITE_4(pDrvCtrl, addr, AXI_READ_4(pDrvCtrl , addr) | (data))

#define AXI_CLRBIT_4(pDrvCtrl, addr, data)  \
        AXI_WRITE_4(pDrvCtrl, addr, AXI_READ_4(pDrvCtrl, addr) & (~(data)))

#define AXIDMA_READ_4(pDrvCtrl, addr)       \
        vxbRead32 ((pDrvCtrl->handle[1]),   \
                (UINT32 *)((char *)(pDrvCtrl->axiDmaBar) + addr))  

#define AXIDMA_WRITE_4(pDrvCtrl, addr, data)    \
        vxbWrite32((pDrvCtrl->handle[1]),       \
                (UINT32 *)((char *)(pDrvCtrl->axiDmaBar) + addr), data)

#define AXIDMA_SETBIT_4(pDrvCtrl, offset, val) \
        AXIDMA_WRITE_4(pDrvCtrl, offset, AXIDMA_READ_4(pDrvCtrl, offset) | (val))

#define AXIDMA_CLRBIT_4(pDrvCtrl, offset, val) \
        AXIDMA_WRITE_4(pDrvCtrl, offset, AXIDMA_READ_4(pDrvCtrl, offset) & ~(val))

#ifdef __cplusplus
}
#endif

#endif /* __INCvxbfmqlAxiEndh */
