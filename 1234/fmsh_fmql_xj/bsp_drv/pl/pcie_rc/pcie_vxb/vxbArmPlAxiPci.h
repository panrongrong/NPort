/* vxbFdtXlnxAxi2PciEx.h - PCI Express driver hardware defintions */

/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
22Nov21, jc  init
*/

#ifndef __INCvxbArmAxiPcih
#define __INCvxbArmAxiPcih

#ifdef __cplusplus
extern "C" {
#endif

/* Register MAP */

/* PCIe Standard Registers */

#define A2P_PCIE_CONFIG_BASE            (0x0000)
#define A2P_PCIE_CFG_COMMAND            (0x0004)
#define A2P_PCIE_CFG_CLASS              (0x0008)
#define A2P_PCIE_CFG_HEADER             (0x000C)
#define A2P_PCIE_CFG_BAR0               (0x0010)
#define A2P_PCIE_CFG_BAR1               (0x0014)
#define A2P_PCIE_CFG_BUS                (0x0018)
#define A2P_PCIE_CFG_IO                 (0x001C)
#define A2P_PCIE_CFG_MEM                (0x0020)
#define A2P_PCIE_CFG_PREF_MEM           (0x0024)
#define A2P_PCIE_CFG_PREF_BASE_UPPER    (0x0028)
#define A2P_PCIE_CFG_PREF_LIMIT_UPPER   (0x002c)
#define A2P_PCIE_CFG_IO_UPPER           (0x0030)

/* IP registers */

#define A2P_VSEC_CAPABILITY             (0x0128)
#define A2P_VSEC_HEADER                 (0x012C)
#define A2P_BRIDGE_INFO                 (0x0130)
#define A2P_BRIDGE_CONTROL              (0x0134)
#define A2P_BRIDGE_STATUS               (0x0134)
#define A2P_INTERRUPT_DECODE            (0x0138)
#define A2P_INTERRUPT_MASK              (0x013C)
#define A2P_BUS_LOCATION                (0x0140)
#define A2P_PHY_STATUS                  (0x0144)
#define A2P_PHY_CONTROL                 (0x0144)
#define A2P_ROOT_PORT_STATUS            (0x0148)
#define A2P_ROOT_PORT_CONTROL           (0x0148)
#define A2P_ROOT_PORT_MSIBASE_1         (0x014C)
#define A2P_ROOT_PORT_MSIBASE_2         (0x0150)
#define A2P_ROOT_PORT_ERR_FIFO_READ     (0x0154)
#define A2P_ROOT_PORT_INT_FIFO_READ_1   (0x0158)
#define A2P_ROOT_PORT_INT_FIFO_READ_2   (0x015C)
#define A2P_VSEC_CAPABILITY_2           (0x0200)
#define A2P_VSEC_HEADER_2               (0x0204)

/* AXI BASE ADDRESS TRANSLATION CONFIGURATION REGISTERS */

#define AXIBAR2PCIEBAR_0U           (0x0208)
#define AXIBAR2PCIEBAR_0L           (0x020C)
#define AXIBAR2PCIEBAR_1U           (0x0210)
#define AXIBAR2PCIEBAR_1L           (0x0214)
#define AXIBAR2PCIEBAR_2U           (0x0218)
#define AXIBAR2PCIEBAR_2L           (0x021C)
#define AXIBAR2PCIEBAR_3U           (0x0220)
#define AXIBAR2PCIEBAR_3L           (0x0224)
#define AXIBAR2PCIEBAR_4U           (0x0228)
#define AXIBAR2PCIEBAR_4L           (0x022C)
#define AXIBAR2PCIEBAR_5U           (0x0230)
#define AXIBAR2PCIEBAR_5L           (0x0234)

/* Register bit fields */

/* A2P_VSEC_CAPABILITY register bit definitions */

#define VSEC_CAPABILITY_ID                   (0x000B)
#define VSEC_CAPABILITY_ID_MASK              (0x0000FFFF)
#define VSEC_CAPABILITY_VERSION              (0x00010000)
#define VSEC_CAPABILITY_VERSION_MASK         (0x000F0000)
#define VSEC_CAPABILITY_NEXT_CAP_OFFSET      (0x0200)
#define VSEC_CAPABILITY_NEXT_CAP_OFFSET_MASK (0xFFF00000)

/* A2P_VSEC_HEADER register bit definitions */

#define VSEC_ID                         (0x0001)
#define VSEC_ID_MASK                    (0x0000FFFF)
#define VSEC_REV                        (0x0)
#define VSEC_REV_MASK                   (0x000F0000)
#define VSEC_LENGTH                     (0x038)
#define VSEC_LENGTH_MASK                (0xFFF00000)

/* A2P_BRIDGE_INFO register bit definitions */

#define ECAM_SIZE_MASK                  (0x00070000)
#define ECAM_FIELD_POSITION             (16)

/* A2P_BRIDGE_CONTROL register bit definitions */

#define A2P_GLOBAL_INTERRUPTS_ENABLE    (0x00000100)

/* A2P_INTERRUPT_DECODE register bit definitions */

#define A2P_INTR_LINK_DOWN                   (0x00000001)
#define A2P_INTR_STREAMING_ERROR             (0x00000004)
#define A2P_INTR_CFG_COMPLETEION_STS         (0x000000E0)
#define A2P_INTR_CFG_TIMEOUT                 (0x00000100)
#define A2P_INTR_CORRECTABLE_ERR             (0x00000200)
#define A2P_INTR_NON_FATAL_ERR               (0x00000400)
#define A2P_INTR_FATAL_ERR                   (0x00000800)
#define A2P_INTR_INTX                        (0x00010000)
#define A2P_INTR_MSI                         (0x00020000)
#define A2P_INTR_SLAVE_UNSUPPORTED_REQUEST   (0x00100000)
#define A2P_INTR_SLAVE_UNEXPECTED_COMPLETION (0x00200000)
#define A2P_INTR_SLAVE_COMPLETION_TIMEOUT    (0x00400000)
#define A2P_INTR_SLAVE_ERROR_POISON          (0x00800000)
#define A2P_INTR_SLAVE_COMPLETER_ABORT       (0x01000000)
#define A2P_INTR_SLAVE_ILLEGAL_BURST         (0x02000000)
#define A2P_INTR_MASTER_DECERR               (0x04000000)
#define A2P_INTR_MASTER_SLVERR               (0x08000000)
#define A2P_INTR_MASTER_ERROR_POISON         (0x10000000)

/* A2P_INTERRUPT_MASK register bit definitions */

#define A2P_INTERRUPTS_DISABLE          (0x00000000)
#define A2P_INTERRUPTS_ENABLE           (0xFFFFFFFF)
#define A2P_IMR_MASKALL                 (0x1FF30FE5)

/* A2P_PHY_STATUS register bit definitions */

#define A2P_PHY_LINK_UP_MASK            (0x00000800)

/* A2P_ROOT_PORT_CONTROL (and STATUS) register bit definitions */

#define A2P_ROOT_PORT_BRIDGE_ENABLE     (0x00000001)
#define A2P_ROOT_PORT_EFIFO_NOT_EMPTY   (0x00010000) /* error FIFO not empty */
#define A2P_ROOT_PORT_EFIFO_OFLOW       (0x00020000) /* error FIFO overflow */
#define A2P_ROOT_PORT_IFIFO_NOT_EMPTY   (0x00040000) /* interrupt FIFO not empty */
#define A2P_ROOT_PORT_IFIFO_OFLOW       (0x00080000) /* interrupt FIFO overflow */
#define A2P_ROOT_PORT_CMP_TIMEOUT       (0x0FF00000) /* completion timeout */

/* A2P_ROOT_PORT_ERR_FIFO_READ register bit definitions */

#define A2P_ERRFIFO_ERROR_VALID         (0x00040000)
#define A2P_ERRFIFO_ERROR_TYPE          (0x00030000)
#define A2P_ERRFIFO_ERROR_CORRECTABLE   (0x00000000)
#define A2P_ERRFIFO_ERROR_NON_FATAL     (0x00010000)
#define A2P_ERRFIFO_ERROR_FATAL         (0x00020000)
#define A2P_ERRFIFO_REQUESTER_ID_MASK   (0x0000FFFF)

/* A2P_ROOT_PORT_INT_FIFO_READ_1 register bit definitions */

#define A2P_READFIFO_VALID_INTR         (0x80000000)
#define A2P_READFIFO_MSI_INTR           (0x40000000)
#define A2P_READFIFO_INTERRUPT_ASSERTED (0x20000000)
#define A2P_READFIFO_INT_LINE_MASK      (0x18000000)
#define A2P_READFIFO_INT_LINE_INTA      (0x00000000)
#define A2P_READFIFO_INT_LINE_INTB      (0x08000000)
#define A2P_READFIFO_INT_LINE_INTC      (0x10000000)
#define A2P_READFIFO_INT_LINE_INTD      (0x18000000)
#define A2P_READFIFO_MSI_ADDR_MASK      (0x07FF0000)
#define A2P_READFIFO_REQUESTER_ID_MASK  (0x0000FFFF)
#define A2P_READFIFO_INT_LINE_POSITION  (27)

#define A2P_READFIFO_INT_LINE(x)    (((x) & A2P_READFIFO_INT_LINE_MASK) >> 27)

/* Various helper defines */

#define MAX_LINK_UP_CHECK_COUNT         1000
#define PCI_MAX_INT_LINE                4
#define PCI_MAX_LINK                    4
#define ZYNQ_AXI_PCIE_MAX_BUS_NUM       127
#define ZYNQ_AXI_PCIE_MSI_NUM           32

	
typedef struct armAxiPciDrvCtrl
{
	VXB_DEVICE_ID		  pInst;
	void *				  regBase;
	void *				  handle;
	int 				  pciMaxBus;

	void *				  mem32Addr;
	UINT32				  mem32Size;
	void *				  io32Addr;
	UINT32				  io32Size;
	UINT32				  maxLatAllSet;
	UINT32				  cacheSize;

	UINT32				  autoConfig;
	BOOL				  initDone;
	
	BOOL				  linkStatus;  /* add by jc*/

	struct vxbPciConfig * pPciConfig;
	struct vxbPciInt *	  pIntInfo;
	struct hcfDevice *	  pHcf;
} VXB_ARMAXIPCI_DRV_CTRL;
	
#if (_BYTE_ORDER == _BIG_ENDIAN)
#define SWAP32 vxbSwap32
#define SWAP16 vxbSwap16
#endif
#if (_BYTE_ORDER == _LITTLE_ENDIAN)
#define SWAP32
#define SWAP16
#endif
	
	
#define VXB_ARMAXIPCI_BAR(p)    ((VXB_ARMAXIPCI_DRV_CTRL *)(p)->pDrvCtrl)->regBase
#define VXB_ARMAXIPCI_HANDLE(p) ((VXB_ARMAXIPCI_DRV_CTRL *)(p)->pDrvCtrl)->handle
	
#define CSR_READ_4(pDev, addr)                                  \
			SWAP32 (*(volatile UINT32 *)((char *)VXB_ARMAXIPCI_BAR(pDev) + addr))
	
#define CSR_WRITE_4(pDev, addr, data)               \
			*(volatile UINT32 *)((char *)VXB_ARMAXIPCI_BAR(pDev) + addr) = SWAP32 (data)
	
#define CSR_READ_2(pDev, addr)                                  \
			SWAP16 (*(volatile UINT16 *)((char *)VXB_ARMAXIPCI_BAR(pDev) + addr))
	
#define CSR_WRITE_2(pDev, addr, data)               \
			*(volatile UINT16 *)((char *)VXB_ARMAXIPCI_BAR(pDev) + addr) = SWAP16 (data)
	
#define CSR_READ_1(pDev, addr)                                  \
			(*((volatile UINT8 *)(char *)VXB_ARMAXIPCI_BAR(pDev) + addr))
	
#define CSR_WRITE_1(pDev, addr, data)               \
			*(volatile UINT8 *)((char *)VXB_ARMAXIPCI_BAR(pDev) + addr) = data



#ifdef __cplusplus
}
#endif

#endif /* __INCvxbArmAxiPcih */
