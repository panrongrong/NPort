/* vxbAltSocGen5DwEnd.h - Header file for Ethernet Driver */

/*
 * Copyright (c) 2012, 2013 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
15Aug19,jc  Init
*/

#ifndef __INCvxbAltSocGen5DwEndh
#define __INCvxbAltSocGen5DwEndh

/* defines */
#include <endLib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 
forward declarations 
*/
IMPORT void vxbAltSocGen5DwEndRegister(void);

#define EMAC_MAX_DEVICE		        2
#define ALT_DEV_BOARD_CYCLONE_V

/* 
EMAC Control reg 
*/
#define EMAC_CONTROL          0x0000    /* Configuration */
#define EMAC_FRAME_FILTER     0x0004    /* Frame Filter */
#define EMAC_HASH_BASE        0x0500    /* Multicast Hash Table base */
#define EMAC_HASH_LEN         8         /* Multicast Hash Table Len in 32b */
#define EMAC_MII_ADDR         0x0010    /* MII Address */
#define EMAC_MII_DATA         0x0014    /* MII Data */
#define EMAC_FLOW_CTRL        0x0018    /* Flow Control */
#define EMAC_VLAN_TAG         0x001c    /* VLAN Tag */
#define EMAC_VERSION          0x0020    /* EMAC CORE Version */
#define EMAC_WAKEUP_FILTER    0x0028    /* Wake-up Frame Filter */
#define EMAC_INT_STATUS       0x0038    /* interrupt status register */
#define EMAC_INT_MASK         0x003c    /* interrupt mask register */

/*
interrupt status register bits
*/
#define EMAC_INT_STATUS_LINK_MASK       0x1
#define EMAC_INT_STATUS_LINKCHANGED     0x1

/* 
PMT Control and Status 
*/
#define EMAC_PMT              0x002c

/* 
 * Energy Efficient Ethernet (EEE)
 * LPI status, timer and control register offset
 */ 
#define LPI_CTRL_STATUS	      0x0030
#define LPI_TIMER_CTRL	      0x0034

/* 
LPI control and status defines 
*/
#define LPI_CTRL_STATUS_LPITXA	0x00080000	/* Enable LPI TX Automate */
#define LPI_CTRL_STATUS_PLSEN	0x00040000	/* Enable PHY Link Status */
#define LPI_CTRL_STATUS_PLS	    0x00020000	/* PHY Link Status */
#define LPI_CTRL_STATUS_LPIEN	0x00010000	/* LPI Enable */
#define LPI_CTRL_STATUS_RLPIST	0x00000200	/* Receive LPI state */
#define LPI_CTRL_STATUS_TLPIST	0x00000100	/* Transmit LPI state */
#define LPI_CTRL_STATUS_RLPIEX	0x00000008	/* Receive LPI Exit */
#define LPI_CTRL_STATUS_RLPIEN	0x00000004	/* Receive LPI Entry */
#define LPI_CTRL_STATUS_TLPIEX	0x00000002	/* Transmit LPI Exit */
#define LPI_CTRL_STATUS_TLPIEN	0x00000001	/* Transmit LPI Entry */

/* 
EMAC HW ADDR regs 
*/
#define EMAC_ADDR_HIGH        0x0040
#define EMAC_ADDR_LOW         0x0044
#define EMAC_AN_CTRL          0x00c0    /* Auto-Neg. control */
#define EMAC_AN_STATUS        0x00c4    /* Auto-Neg. status */
#define EMAC_ANE_ADV          0x00c8    /* Auto-Neg. Advertisement */
#define EMAC_ANE_LINK         0x00cc    /* Auto-Neg. link partener ability */
#define EMAC_ANE_EXP          0x00d0    /* ANE expansion */
#define EMAC_TBI              0x00d4    /* TBI extend status */
#define EMAC_GMII_STATUS      0x00d8    /* S/R-GMII status */

/* 
MMC control 
*/
#define MMC_CNTRL             0x0100	/* MMC Control */
#define MMC_RX_INTR           0x0104	/* MMC RX Interrupt */
#define MMC_TX_INTR           0x0108	/* MMC TX Interrupt */
#define MMC_RX_INTR_MASK      0x010c	/* MMC Interrupt Mask */
#define MMC_TX_INTR_MASK      0x0110	/* MMC Interrupt Mask */
#define MMC_RX_IPC_INTR_MASK  0x0200
#define MMC_RX_IPC_INTR       0x0208
#define MMC_DEFAULT_MASK      0xffffffff

/*   MAC Reg1:GMAC_MFF (Gmac Offset: 0x004) MAC Frame Filter */
#define GMAC_MFF_PR        (0x1u)                    /* Promiscuous Mode */
#define GMAC_MFF_HUC       (0x1u << 1)               /*  */
#define GMAC_MFF_HMC       (0x1u << 2)               /*  */
#define GMAC_MFF_DAIF      (0x1u << 3)               /* DA Inverse Filter */
#define GMAC_MFF_PM        (0x1u << 4)               /* Pass All Multicast */
#define GMAC_MFF_DBF       (0x1u << 5)               /* disable broadcast frame */
#define GMAC_MFF_PCF       (0x3u << 6)               /* Pass Control Frames */
#define GMAC_MFF_PCF_00    (0x00u << 6)               /* filters all control frames */
#define GMAC_MFF_PCF_01    (0x1u << 6)               /* forwards all control frames except Pause frames to application even if they fail the Address filter. */
#define GMAC_MFF_PCF_10    (0x2u << 6)               /* forwards all control frames to application even if they fail the Address Filter */
#define GMAC_MFF_PCF_11    (0x3u << 6)               /* MAC forwards control frames that pass the Address Filter. */
#define GMAC_MFF_SAIF      (0x1u << 8)               /* SA Inverse Filter */
#define GMAC_MFF_SAF       (0x1u << 9)               /* Source Address Filter Enable */
#define GMAC_MFF_HPF       (0x1u << 10)              /*  */
#define GMAC_MFF_VTFE      (0x1u << 16)              /* VLAN Tag Filter Enable */
#define GMAC_MFF_IPFE      (0x1u << 20)              /*  */
#define GMAC_MFF_DNTU      (0x1u << 21)              /*  */
#define GMAC_MFF_RA        (0x1u << 31)              /* Receive all */


/* 
6.1.1 DMA Register Map 
*/
#define DMA_BUS_MODE          0x1000    /* Bus Mode */
#define DMA_TX_POLL_DEMAND    0x1004    /* Transmit Poll Demand */
#define DMA_RX_POLL_DEMAND    0x1008    /* Received Poll Demand */
#define DMA_RX_BASE_ADDR      0x100c    /* Receive List Base */
#define DMA_TX_BASE_ADDR      0x1010    /* Transmit List Base */
#define DMA_ISR_STATUS        0x1014    /* Status Register */
#define DMA_CONTROL           0x1018    /* Ctrl (Operational Mode) */
#define DMA_INTR_ENA          0x101c    /* Interrupt Enable */
#define DMA_MISSED_FRAME_CTR  0x1020    /* Missed Frame Counter */
#define DMA_RX_WATCHDOG       0x1024    /* Receive Interrupt Watchdog */
#define DMA_AXI_BUS_MODE      0x1028    /* AXI Bus Mode */
#define R20_CHTBAR_OFFSET     0x1050    /* Current Host Tx Buffer */
#define R21_CHRBAR_OFFSET     0x1054    /* Current Host Rx Buffer */
#define DMA_HW_FEATURE        0x1058    /* HW Feature Register */

/* 
DMA_AXI_BUS_MODE parameters 
*/

#define DMA_AXI_BUS_MODE_4    0x2       /* AXI Bus Mode */
#define DMA_AXI_BUS_MODE_8    0x4       /* AXI Bus Mode */
#define DMA_AXI_BUS_MODE_16   0x8       /* AXI Bus Mode */
#define DMA_AXI_BUS_MODE_32   0x10      /* AXI Bus Mode jc */

/* 
EMAC flow control register 
*/
#define EMAC_PAUSE_TIMER_MASK       0xFFFF0000 /* PAUSE TIME field in the control frame */
#define EMAC_PAUSE_TIMER_SHIFT              16
#define EMAC_PAUSE_LOW_THRESH3      0x00000030 /* threshold for pause tmr 256 slot time */
#define EMAC_PAUSE_LOW_THRESH2      0x00000020 /* threshold for pause tmr 144 slot time */
#define EMAC_PAUSE_LOW_THRESH1      0x00000010 /* threshold for pause tmr 28 slot time */
#define EMAC_PAUSE_LOW_THRESH0             0x0 /* threshold for pause tmr 4 slot time */
#define EMAC_UNICAST_PAUSE_FRAME_ON 0x00000008 /* Detect pause frame with unicast addr */
#define EMAC_UNICAST_PAUSE_FRAME_OFF       0x0 /* Detect only pause frame with multicast addr */
#define EMAC_RX_FLOW_CTRL_ENABLE    0x00000004 /* Enable Rx flow control */
#define EMAC_RX_FLOW_CTRL_DISABLE          0x0 /* Disable Rx flow control */
#define EMAC_TX_FLOW_CTRL_ENABLE    0x00000002 /* Enable Tx flow control */
#define EMAC_TX_FLOW_CTRL_DISABLE          0x0 /* Disable flow control */
#define EMAC_SEND_PAUSE_FRAME       0x00000001 /* send pause frame */   

/* 
DMA interrupt mask register 
*/
#define EMAC_LPI_INT_MASK           0x00000400 /* LPI Interrupt Mask */
#define EMAC_TIMESTAMP_INT_MASK     0x00000200 /* Timestamp Interrupt Mask */
#define EMAC_MMC_INTSTAT            0x00000010
#define EMAC_MMC_RX_INTSTAT         0x00000020
#define EMAC_MMC_TX_INTSTAT         0x00000040
#define EMAC_MMC_CKSM_INTSTAT       0x00000080
#define EMAC_PSM_ANE_INT_MASK       0x00000004 /* PCS AN Interrupt Mask */
#define EMAC_PCS_LINK_INT_MASK      0x00000002 /* PCS Link Status Interrupt Mask */
#define EMAC_RGMII_SMII_INT_MASK    0x00000001 /* PCS Link Status Interrupt Mask */

/* 
DMA bus mode register 
*/
#define DEFAULT_DMA_PBL		        8
#define DMA_BUS_MODE_PBL	        0x01000000
#define DMA_BUS_MODE_PBL_SHIFT	    8
#define DMA_BUS_MODE_RPBL_SHIFT	    17

/* 
Fixed Burst SINGLE, INCR4, INCR8 or INCR16 
*/
#define DMA_FIXED_BURST_ENABLE  0x00010000 
#define DMA_FIXED_BURST_DISABLE 0           /* SINGLE, INCR */
#define DMA_BURST_LENGTH32      0x00002000  /* Dma burst length = 32 */
#define DMA_BURST_LENGTH16      0x00001000  /* Dma burst length = 16 */
#define DMA_BURST_LENGTH8       0x00000800  /* Dma burst length = 8 */
#define DMA_BURST_LENGTH4       0x00000400  /* Dma burst length = 4 */
#define DMA_BURST_LENGTH2       0x00000200  /* Dma burst length = 2 */
#define DMA_BURST_LENGTH1       0x00000100  /* Dma burst length = 1 */
#define DMA_BURST_LENGTH0       0x00000000  /* Dma burst length = 0 */
#define DMA_SET_ALT_DESC_SIZE   0x00000080  /* Alternate Descriptor Size = 8 word */
#define DMA_DESC_SKIP16         0x00000040  /* number of dwords to skip */
#define DMA_DESC_SKIP8          0x00000020  /* between two unchained descriptors */
#define DMA_DESC_SKIP4          0x00000010
#define DMA_DESC_SKIP2          0x00000008
#define DMA_DESC_SKIP1          0x00000004
#define DMA_DESC_SKIP0          0
#define DMA_RESET_ON            0x00000001  /* Software Reset DMA engine */
#define DMA_RESET_OFF           0

/* 
DMA control register 
*/
#define DMA_RX_STR_FWD          0x02000000  /* Receive Store and Forward */
#define DMA_TX_STR_FWD          0x00200000  /* Transmit Store and Forward */
#define DMA_FLUSH_TX_FIFO       0x00100000  /* Flush TX FIFO */
#define DMA_TX_START            0x00002000  /* Start/Stop transmission */
#define DMA_FWD_ERR_FRAME       0x00000080  /* Forward error frames */
#define DMA_FWD_UNDER_SZ_FRAME  0x00000040  /* Forward undersize frames */
#define DMA_TX_SECOND_FRAME     0x00000004  /* Operate on second frame */
#define DMA_RX_START            0x00000002  /* Start/Stop receive */

/* 
DMA Status register 
*/
#define DMA_LINE_INTF_INT       0x04000000  /* Line interface interrupt */
#define DMA_ERROR_BIT2          0x02000000  /* err. 0-data buffer, 1-desc. access, read only */
#define DMA_ERROR_BIT1          0x01000000  /* err. 0-write trnsf, 1-read transfr, read only */
#define DMA_ERROR_BIT0          0x00800000  /* err. 0-Rx DMA, 1-Tx DMA, read only */
#define DMA_TX_STATE            0x00700000  /* Transmit process state, read only */
#define DMA_TX_STOPPED          0x00000000  /* Stopped */
#define DMA_TX_FETCHING         0x00100000  /* Running - fetching the descriptor */
#define DMA_TX_WAITING          0x00200000  /* Running - waiting for end of transmission */
#define DMA_TX_READING          0x00300000  /* Running - reading the data from memory */
#define DMA_TX_SUSPENDED        0x00600000  /* Suspended */
#define DMA_TX_CLOSING          0x00700000  /* Running - closing descriptor */
#define DMA_RX_STATE            0x000E0000  /* Receive process state, read only */
#define DMA_RX_STOPPED          0x00000000  /* Stopped */
#define DMA_RX_FETCHING         0x00020000  /* Running - fetching the descriptor */
#define DMA_RX_WAITING          0x00060000  /* Running - waiting for packet */
#define DMA_RX_SUSPENDED        0x00080000  /* Suspended */
#define DMA_RX_CLOSING          0x000A0000  /* Running - closing descriptor */
#define DMA_RX_QUEUING          0x000E0000  /* Running - queuing the recieve frame into host memory */
#define DMA_INT_NORMAL          0x00010000  /* Normal interrupt summary, RW */
#define DMA_INT_ABNORMAL        0x00008000  /* Abnormal interrupt summary, RW */
#define DMA_INT_EARLY_RX        0x00004000  /* Early receive interrupt (Normal)       RW        0       */
#define DMA_INT_BUS_ERROR       0x00002000  /* Fatal bus error (Abnormal)             RW        0       */
#define DMA_INT_EARLY_TX        0x00000400  /* Early transmit interrupt (Abnormal)    RW        0       */
#define DMA_INT_RX_WDOG_TO      0x00000200  /* Receive Watchdog Timeout (Abnormal)    RW        0       */
#define DMA_INT_RX_STOPPED      0x00000100  /* Receive process stopped (Abnormal)     RW        0       */
#define DMA_INT_RX_NO_BUFFER    0x00000080  /* Receive buffer unavailable (Abnormal)  RW        0       */
#define DMA_INT_RX_COMPLETED    0x00000040  /* Completion of frame reception (Normal) RW        0       */
#define DMA_INT_TX_UNDERFLOW    0x00000020  /* Transmit underflow (Abnormal)          RW        0       */
#define DMA_INT_RX_OVERFLOW     0x00000010  /* Receive Buffer overflow interrupt      RW        0       */
#define DMA_INT_TX_JABBER_TO    0x00000008  /* Transmit Jabber Timeout (Abnormal)     RW        0       */
#define DMA_INT_TX_NO_BUFFER    0x00000004  /* Transmit buffer unavailable (Normal)   RW        0       */
#define DMA_INT_TX_STOPPED      0x00000002  /* Transmit process stopped (Abnormal)    RW        0       */
#define DMA_INT_TX_COMPLETED    0x00000001  /* Transmit completed (Normal)            RW        0       */

/* 
status word of DMA descriptor 
*/
#define RDESC0_OWN_BY_DMA         0x80000000  /* bit31 OWN: Own Bit; Descriptor is owned by DMA engine */
#define RDESC0_DA_FILTER_FAIL     0x40000000  /* bit30 AFM: Destination Address Filter Fail; DA Filter Fail for the received frame */
#define RDESC0_FRAME_LENGTH_MASK  0x3FFF0000  /* bit29:16 FL: Frame Length; Receive descriptor frame length */
#define RDESC0_FRAME_LENGTH_SHIFT 16
			
#define RDESC0_ERROR              0x00008000  /* bit15 ES: Error summary bit  - OR of the following bits:    v  */
#define RDESC0_RX_TRUNCATED       0x00004000  /* bit14 DE: Descriptor Error; no more descriptors for receive frame        E  */
#define RDESC0_SA_FILTER_FAIL     0x00002000  /* bit13 SAF: Source Address Filter Fail; SA Filter Fail for the received frame   E  */
#define RDESC0_RX_LENGTH_ERROR    0x00001000  /* bit12 LE: Length Error; frame size not matching with length field    E  */
			
#define RDESC0_RX_DAMAGED         0x00000800  /* bit11 OE: Overflow Error; frame was damaged due to buffer overflow   E  */
#define RDESC0_RX_VLAN            0x00000400  /* bit10 VLAN: VLAN Tag; received frame is a VLAN frame               I  */
#define RDESC0_RX_FIRST           0x00000200  /* bit9 FS: First Descriptor; first descriptor of the frame           I  */
#define RDESC0_RX_LAST            0x00000100  /* bit8 LD: Last Descriptor; last descriptor of the frame             I  */
			
#define RDESC0_RX_LONG_FRAME      0x00000080  /* bit7 IPC Checksum Error or Giant Frame; frame is longer than 1518 bytes */
#define RDESC0_RX_COLLISION       0x00000040  /* bit6 LC: Late Collision; late collision occurred during reception     E  */
#define RDESC0_RX_FRAME_ETHER     0x00000020  /* bit5 FT: Frame Type; Frame type - Ethernet, otherwise 802.3          */
#define RDESC0_RX_WDOG_EXPIRED    0x00000010  /* bit4 RWT: Receive Watchdog Timeout; watchdog timer expired during reception      E  */
			
#define RDESC0_RX_MII_ERR         0x00000008  /* bit3 RE: Receive Error; error reported by MII interface              E  */
#define RDESC0_RX_DRIBBLING       0x00000004  /* bit2 DE: Dribble Bit Error; frame contains noninteger multiple of 8 bits    */ 
#define RDESC0_RX_CRC             0x00000002  /* bit1 CE: CRC Error; CRC error */ 
			
#define RDESC1_RX_RBS1SZ_MASK     0x000007FF  /* bit10:0 RBS1: Receive Buffer 1 Size; Receive buffer 1 size */
#define RDESC1_RX_RBS1SZ_SHIFT    0
			
#define RDESC1_RX_END_OF_RING     0x02000000  /* bit25 RER: Receive End of Ring; End of descriptors ring */
#define RDESC1_RX_CHAIN           0x01000000  /* bit24 RCH: Second Address Chained;  Second buffer address is chain address               */
#define RDESC1_RX_INT_DISABLE     0x80000000  /* bit31 DIC: Disable Interrupt on Completion; Disable Interrupt on Completion         */
			
#define RDESC1_SIZE2_MASK         0x003FF800  /* bit21:11 RBS2: Receive Buffer 2 Size; Buffer2 size (bit21:11 RBS2: Receive Buffer2 Size) */
#define RDESC1_SIZE2_SHIFT        11
#define RDESC1_SIZE1_MASK         0x000007FF  /* bit10:0 RBS1: Receive Buffer 1 Size; Buffer1 size (bit10:0 RBS1: Receive Buffer1 Size)  */
			
/*
TDES_0
*/
#define TDESC0_OWN_BY_DMA         0x80000000   /* bit31 OWN: Own Bit*/
						
#define TDESC0_TX_IPHEADER_ERR    0x00010000   /* bit16 IHE: IP Header Error; error in IP header, no checksums inserted    E  */
									
#define TDESC0_TX_TIMEOUT         0x00004000   /* bit14 JT: Jabber Timeout; Transmit jabber timeout */
#define TDESC0_TX_FRAME_FLUSHED   0x00002000   /* bit13 FF: Frame Flushed; DMA/MTL flushed the frame due to SW flush    I  */
#define TDESC0_TX_PAYLOAD_CKSUM   0x00001000   /* bit12 IPE: IP Payload Error; error creating payload checksum              E  */
									
									
#define TDESC0_TX_LOST_CARRIER    0x00000800   /* bit11 LC: Loss of Carrier; carrier lost during tramsmission             E  */
#define TDESC0_TX_NO_CARRIER      0x00000400   /* bit10 NC: No Carrier; no carrier signal from the tranceiver        E  */
#define TDESC0_TX_LATE_COLL       0x00000200   /* bit9 LC: Late Collision; transmission aborted due to collision        E  */
#define TDESC0_TX_EXC_COLL        0x00000100   /* bit8 EC: Excessive Collision; transmission aborted after 16 collisions     E  */
									
#define TDESC0_TX_VLAN_FRAME      0x00000080   /* bit7 VF: VLAN Frame; VLAN-type frame */
#define TDESC0_TX_COLL_MASK       0x00000078   /* bit6:3 CC: Collision Count (Status field); Collision count */
#define TDESC0_TX_COLL_SHIFT      3
									
#define TDESC0_TX_EXC_DEF         0x00000004   /* bit2 ED: Excessive Deferral; excessive deferral */
#define TDESC0_TX_UNDERFLOW       0x00000002   /* bit1 UF: Underflow Error;  late data arrival from the memory */
#define TDESC0_TX_DEFERRED        0x00000001   /* bit0 DB: Deferred Bit; frame transmision deferred */
			
/*
TDES_1
*/
#define TDESC1_TX_INT_ENABLE      0x80000000  /* bit31 IC: Interrupt on Completion; interrupt on completion */
#define TDESC1_TX_LAST            0x40000000  /* bit30 LS: Last Segment  Last segment of the frame */
#define TDESC1_TX_FIRST           0x20000000  /* bit29 FS: First Segment First segment of the frame */
			
#define TDESC1_TX_CRC             0x18000000  /* bit28~27 CIC: Checksum Insertion Control  Enable add CRC info */
#define TDESC1_TX_CIC_1           0x10000000  /* Enable Checksum Insertion Bit1 */
#define TDESC1_TX_CIC_0           0x08000000  /* Checksum Insertion Bit0 */
			
#define TDESC1_TX_END_OF_RING     0x02000000  /* bit25 TER: Transmit End of Ring; End of descriptors ring */
#define TDESC1_TX_CHAIN           0x01000000  /* bit24 TCH: Second Address Chained; Second buffer address is chain address */
			
#define TDESC1_TX_TS_ENABLE       0x00400000  /* bit22 TTSE: Transmit Timestamp Enable; enable timestamping */
			
#define TDESC1_SIZE2_MASK         0x003FF800  /* Buffer2 size (bit21:11 RBS2: Receive Buffer2 Size) */
#define TDESC1_SIZE2_SHIFT        11
#define TDESC1_SIZE1_MASK         0x000007FF  /* Buffer1 size (bit10:0 RBS1: Receive Buffer1 Size)  */


#define DMA_INT_ENABLE (DMA_INT_NORMAL | DMA_INT_ABNORMAL | DMA_INT_BUS_ERROR \
            | DMA_INT_RX_NO_BUFFER | DMA_INT_RX_COMPLETED | DMA_INT_RX_STOPPED\
            | DMA_INT_TX_UNDERFLOW | DMA_INT_TX_COMPLETED | DMA_INT_TX_STOPPED)
#define DMA_INT_DISABLE 0


/* 
EMAC Configuration defines 
*/
#define EMAC_CONTROL_TC        0x01000000   /* Transmit Conf. in RGMII/SGMII */
#define EMAC_CONTROL_WD        0x00800000   /* Disable Watchdog on receive */
#define EMAC_CONTROL_JD        0x00400000   /* Jabber disable */
#define EMAC_CONTROL_BE        0x00200000   /* Frame Burst Enable */
#define EMAC_CONTROL_JE        0x00100000   /* Jumbo frame */
#define EMAC_CONTROL_DCRS      0x00010000   /* Disable carrier sense during tx */
#define EMAC_CONTROL_PS        0x00008000   /* Port Select 0:GMI 1:MII */
#define EMAC_CONTROL_FES       0x00004000   /* Speed 0:10 1:100 */
#define EMAC_CONTROL_DO        0x00002000   /* Disable Rx Own */
#define EMAC_CONTROL_LM        0x00001000   /* Loop-back mode */
#define EMAC_CONTROL_DM        0x00000800   /* Duplex Mode */
#define EMAC_CONTROL_IPC       0x00000400   /* Checksum Offload */
#define EMAC_CONTROL_DR        0x00000200   /* Disable Retry */
#define EMAC_CONTROL_LUD       0x00000100   /* Link up/down */
#define EMAC_CONTROL_ACS       0x00000080   /* Automatic Pad/FCS Stripping */
#define EMAC_CONTROL_DC        0x00000010   /* Deferral Check */
#define EMAC_CONTROL_TE        0x00000008   /* Transmitter Enable */
#define EMAC_CONTROL_RE        0x00000004   /* Receiver Enable */
			
/* 
EMAC Frame Filter defines 
*/
#define EMAC_FRAME_FILTER_PR   0x00000001   /* Promiscuous Mode */
#define EMAC_FRAME_FILTER_HUC  0x00000002   /* Hash Unicast */
#define EMAC_FRAME_FILTER_HMC  0x00000004   /* Hash Multicast */
#define EMAC_FRAME_FILTER_DAIF 0x00000008   /* DA Inverse Filtering */
#define EMAC_FRAME_FILTER_PM   0x00000010   /* Pass all multicast */
#define EMAC_FRAME_FILTER_DBF  0x00000020   /* Disable Broadcast frames */
#define EMAC_FRAME_FILTER_NOP  0x00000040   /* Does not pass control frames */
#define EMAC_FRAME_FILTER_FAC  0x00000080   /* Forwards all control frames */
#define EMAC_FRAME_FILTER_FCF  0x000000C0   /* Forwards control frames that pass AF */
#define EMAC_FRAME_FILTER_SAIF 0x00000100   /* Inverse Filtering */
#define EMAC_FRAME_FILTER_SAF  0x00000200   /* Source Address Filter */
#define EMAC_FRAME_FILTER_HPF  0x00000400   /* Hash or perfect Filter */
#define EMAC_FRAME_FILTER_OFF  0x80000000   /* Receive all incoming packets */
#define EMAC_FRAME_FILTER_ON   0x0          /* Receive filtered packets only */

/* 
GMII ADDR  defines 
*/
#define EMAC_MII_ADDR_PHY_ADDR_SHIFT    11
#define EMAC_MII_ADDR_PHY_ADDR_MASK     ((0x1F) << EMAC_MII_ADDR_PHY_ADDR_SHIFT) 
#define EMAC_MII_ADDR_REG_SHIFT         6
#define EMAC_MII_ADDR_REG_MASK          ((0x1F) << EMAC_MII_ADDR_REG_SHIFT) 
#define EMAC_MII_ADDR_READ              0x00000000   /* MII Read */
#define EMAC_MII_ADDR_WRITE             0x00000002   /* MII Write */
#define EMAC_MII_ADDR_BUSY              0x00000001   /* MII Busy */
#define GMII_MII_CLK_RNG_60_100_MHZ     (0)
#define GMII_MII_CLK_RNG_100_150_MHZ    (0x4)
#define GMII_MII_CLK_RNG_20_35_MHZ      (0x8)
#define GMII_MII_CLK_RNG_35_60_MHZ      (0xC)
#define GMII_MII_CLK_RNG_150_250_MHZ    (0x10)
#define GMII_MII_CLK_RNG_250_300_MHZ    (0x14)

#define FIRST_FRAME_IN_PACKAGE       0
#define COMPOSE_FRAME_TO_PACKAGE     1

/* #define DW_EMAC_NAME            "emac" */
#define DW_EMAC_NAME            "gem"

/* 
EMAC register definitions 
*/
#define EMAC_MTU                1500
#define EMAC_JUMBO_MTU          9000
#define EMAC_CLSIZE             1536
#define EMAC_TIMEOUT            10000
#define EMAC_BUF_BOUND	        _CACHE_ALIGN_SIZE
#define EMAC_ALIGN(x)   \
        (char *)(((UINT32)(x) + (EMAC_BUF_BOUND-1)) & ~(EMAC_BUF_BOUND-1))   
#define EMAC_BUF_ADJ(x)	        (x)->m_data = EMAC_ALIGN((x)->m_data)
#define EMAC_INC_DESC(x, y)     (x) = (((x) + 1) % y)
#define EMAC_MAXFRAG            16

#define EMAC_RX_DESC_CNT        256  /* 128 */
#define EMAC_TX_DESC_CNT        256  /* 128 */

#define EMAC_TUPLE_CNT          384

#define EMAC_RX_BUFF_SIZE     2048 
/*#define EMAC_RX_BUFF_SIZE       (0x4000)   // 16K*/

#define MII_PHY_ADDR_CTRL                    0
#define MII_PHY_ADDR_STATUS                  1
#define MII_PHY_ADDR_IDENTIFIER_1            2
#define MII_PHY_ADDR_IDENTIFIER_2            3
#define MII_PHY_ADDR_AUTO_NEG_ADV            4
#define MII_PHY_ADDR_AUTO_NEG_LINK_ABILITY   5
#define MII_PHY_ADDR_AUTO_NEG_EXPANSION      6
#define MII_PHY_ADDR_AUTO_NEXT_PAGE          7
#define MII_PHY_ADDR_AUTO_NEXT_PAGE_ABIILITY 8
#define MII_PHY_ADDR_1000BASET_CTRL          9
#define MII_PHY_ADDR_1000BASET_STATUS       10
#define MII_PHY_ADDR_EXT_REG_CTRL           11
#define MII_PHY_ADDR_EXT_REG_DATA_WR        12
#define MII_PHY_ADDR_EXT_REG_DATA_RD        13
#define MII_PHY_ADDR_EXT_RSVD               14
#define MII_PHY_ADDR_EXT_MII_STATUS         15
#define MII_PHY_ADDR_EXT_REG_WRITE_MASK     0x8000
#define MII_PHY_ADDR_EXT_REG_READ_MASK      0x7FFF
#define MII_PHY_ADDR_EXT_REG_WRITE_MASK     0x8000

/* #define PHY_TIMEOUT_VALUE       10000 */
/* #define DW_DMA_TIMEOUT_VALUE    20000 */
#define PHY_TIMEOUT_VALUE       20000
#define DW_DMA_TIMEOUT_VALUE    40000

/* 
Definitions for the Micrel PHY 
*/
#define MII_KSZ9021_EXT_RGMII_CLOCK_SKEW    0x104
#define MII_KSZ9021_EXT_RGMII_RX_DATA_SKEW  0x105
#define MII_KSZ9021_RXC_PAD_SKEW_OFST       12
#define MII_KSZ9021_RXDV_PAD_SKEW_OFST      8
#define MII_KSZ9021_TXC_PAD_SKEW_OFST       4
#define MII_KSZ9021_TXEN_PAD_SKEW_OFST      0

/* jc
 * According to DesignWare Cores Ethernet MAC Universal Databook documentation
 * (Version 3.70a, March 2011, chapter 6.2 Register Descriptions):
 *
 * Minimum interval time (us) between two writes.
 *
 * When any Register content is being transferred to a different clock domain 
 * after a write operation, there should not be any further writes to the same
 * location until the first write is updated. Otherwise, the second write 
 * operation does not get updated to the destination clock domain. 
 * Thus the delay between two writes to the same register location should be at
 * least 4 cycles of the destination clock (PHY receive clock, PHY transmit 
 * clock, or PTP clock) 
 */
#define DW_PTP_INTERVAL         9

typedef struct dma_desc
{
    /* Status */
    volatile UINT32 desc0; 
	
    /* Control and Buffer lengths */
    volatile UINT32 desc1; 
	
    /* Buffer 1 pointer */
    volatile UINT32 desc2; 
	
    /* Buffer 2 pointer or next descriptor pointer in chain structure */
    volatile UINT32 desc3; 
} DMA_DESC;

/* Private adapter context structure. */

typedef struct emac_drv_ctrl
{
    END_OBJ         emacEndObj;
    VXB_DEVICE_ID   emacDev;
    void *          emacBar;
    void *          emacHandle;
    void *          emacMuxDevCookie;

    JOB_QUEUE_ID    emacJobQueue;
    QJOB            emacIntJob;
    volatile BOOL   emacIntPending;
    volatile BOOL   emacRxPending;
    volatile BOOL   emacTxPending;

    BOOL            emacPolling;	
	BOOL            dcrs;  /* jc */
	
    UINT32          emacIntMask;
    M_BLK_ID        emacPollBuf;
    UINT8           emacAddr[ETHER_ADDR_LEN];

    END_CAPABILITIES emacCaps;
    END_IFDRVCONF   emacEndStatsConf;
    END_IFCOUNTERS  emacEndStatsCounters;

    UINT32          emacInErrors;
    UINT32          emacInDiscards;
    UINT32          emacInUcasts;
    UINT32          emacInMcasts;
    UINT32          emacInBcasts;
    UINT32          emacInOctets;
    UINT32          emacOutErrors;
    UINT32          emacOutUcasts;
    UINT32          emacOutMcasts;
    UINT32          emacOutBcasts;
    UINT32          emacOutOctets;

    /* Begin MII/ifmedia required fields. */

    END_MEDIALIST * emacMediaList;
    END_ERR         emacLastError;
    UINT32          emacCurMedia;
    UINT32          emacCurStatus;
    VXB_DEVICE_ID   emacMiiBus;
    VXB_DEVICE_ID   emacMiiDev;
    FUNCPTR         emacMiiPhyRead;
    FUNCPTR         emacMiiPhyWrite;
    INT32           emacMiiPhyAddr;
    INT32           miiIfUnit;

    /* End MII/ifmedia required fields */

    DMA_DESC *      emacRxDescMem;
    DMA_DESC *      emacTxDescMem;	
    M_BLK_ID        emacRxMblk[EMAC_RX_DESC_CNT];	
    M_BLK_ID        emacTxMblk[EMAC_TX_DESC_CNT];	

    UINT32          emacTxProd;
    UINT32          emacTxCons;
    UINT32          emacTxFree;
    UINT32          emacRxIdx;
    UINT32          emacTxLast;
    UINT32          emacTxStall;

    SEM_ID          emacDevSem;
    INT32           emacMaxMtu;
    char *          descBuf;
} EMAC_DRV_CTRL;

/* EMAC control module register low level access routines */

#define EMAC_BAR(p)        ((EMAC_DRV_CTRL *)(p)->pDrvCtrl)->emacBar
#define EMAC_HANDLE(p)     ((EMAC_DRV_CTRL *)(p)->pDrvCtrl)->emacHandle

#ifdef __cplusplus
}
#endif

#endif /* __INCvxbAltSocGen5DwEndh */


