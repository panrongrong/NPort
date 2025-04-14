/* pciConfigLib.h - PCI bus constants header file */

/*
 * Copyright 1984-2005, 2010 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */
 
/*
modification history
--------------------
01a,23sep21,jc  written.
*/

#ifndef __INCpciConfigLib2h
#define __INCpciConfigLib2h

/* Includes */
/*#include <pciClass.h>*/

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Bus Topology Constraints
 *
 * The following defines specify, by default, the maximum number of busses,
 * devices and functions allowed by the PCI 2.1 Specification.
 *
 * NOTE: config.h MUST be included before this file if either PCI_MAX_BUS,
 * PCI_MAX_DEV, or PCI_MAX_FUNC is changed to a BSP-specific value in 
 * config.h
 *
 */

#ifndef PCI_MAX_BUS
#  define PCI_MAX_BUS   255
#endif  /* PCI_MAX_BUS */

#ifndef PCI_MAX_DEV
#  define PCI_MAX_DEV   32
#endif  /* PCI_MAX_DEV */

#ifndef PCI_MAX_FUNC
#  define PCI_MAX_FUNC  8
#endif  /* PCI_MAX_FUNC */

/* PCI Configuration mechanisms */

#define PCI_MECHANISM_0     0   /* non-std user-supplied interface */
#define PCI_MECHANISM_1     1   /* current PC-AT hardware mechanism */
#define PCI_MECHANISM_2     2   /* deprecated */

/* Configuration I/O addresses for mechanism 1 */

#define PCI_CONFIG_ADDR     0x0cf8  /* write 32 bits to set address */
#define PCI_CONFIG_DATA     0x0cfc  /* 8, 16, or 32 bit accesses */

/* Configuration I/O addresses for mechanism 2 */

#define PCI_CONFIG_CSE      0x0cf8  /* CSE register */
#define PCI_CONFIG_FORWARD  0x0cfa  /* forward register */
#define PCI_CONFIG_BASE     0xc000  /* base register */

/* PCI command bits */

#define PCI_CMD_IO_ENABLE       0x0001  /* IO access enable */
#define PCI_CMD_MEM_ENABLE      0x0002  /* memory access enable */
#define PCI_CMD_MASTER_ENABLE   0x0004  /* bus master enable */
#define PCI_CMD_MON_ENABLE      0x0008  /* monitor special cycles enable */
#define PCI_CMD_WI_ENABLE       0x0010  /* write and invalidate enable */
#define PCI_CMD_SNOOP_ENABLE    0x0020  /* palette snoop enable */
#define PCI_CMD_PERR_ENABLE     0x0040  /* parity error enable */
#define PCI_CMD_WC_ENABLE       0x0080  /* wait cycle enable */
#define PCI_CMD_SERR_ENABLE     0x0100  /* system error enable */
#define PCI_CMD_FBTB_ENABLE     0x0200  /* fast back to back enable */

/* PCI status bits */

#define PCI_STATUS_NEW_CAP             0x0010
#define PCI_STATUS_66_MHZ              0x0020
#define PCI_STATUS_UDF                 0x0040
#define PCI_STATUS_FAST_BB             0x0080
#define PCI_STATUS_DATA_PARITY_ERR     0x0100
#define PCI_STATUS_TARGET_ABORT_GEN    0x0800
#define PCI_STATUS_TARGET_ABORT_RCV    0x1000
#define PCI_STATUS_MASTER_ABORT_RCV    0x2000
#define PCI_STATUS_ASSERT_SERR         0x4000
#define PCI_STATUS_PARITY_ERROR        0x8000

/* PCI base address mask bits */

#define PCI_MEMBASE_MASK    ~0xf  /* mask for memory base address */
#define PCI_IOBASE_MASK     ~0x3  /* mask for IO base address */
#define PCI_BASE_IO          0x1  /* IO space indicator */
#define PCI_BASE_BELOW_1M    0x2  /* memory locate below 1MB */
#define PCI_BASE_IN_64BITS   0x4  /* memory locate anywhere in 64 bits */
#define PCI_BASE_PREFETCH    0x8  /* memory prefetchable */

/* Base Address Register Memory/IO Attribute bits */

#define PCI_BAR_SPACE_MASK  (0x01)
#define PCI_BAR_SPACE_IO    (0x01)
#define PCI_BAR_SPACE_MEM   (0x00)

#define PCI_BAR_MEM_TYPE_MASK   (0x06)
#define PCI_BAR_MEM_ADDR32      (0x00)
#define PCI_BAR_MEM_BELOW_1MB   (0x02)
#define PCI_BAR_MEM_ADDR64      (0x04)
#define PCI_BAR_MEM_RESERVED    (0x06)

#define PCI_BAR_MEM_PREF_MASK   (0x08)
#define PCI_BAR_MEM_PREFETCH    (0x08)
#define PCI_BAR_MEM_NON_PREF    (0x00)

#define PCI_BAR_ALL_MASK        (PCI_BAR_SPACE_MASK    | \
                                 PCI_BAR_MEM_TYPE_MASK | \
                                 PCI_BAR_MEM_PREF_MASK)

/* PCI header type bits */

#define PCI_HEADER_TYPE_MASK    0x7f    /* mask for header type */
#define PCI_HEADER_PCI_CARDBUS  0x02    /* PCI to CardBus bridge */
#define PCI_HEADER_PCI_PCI      0x01    /* PCI to PCI bridge */
#define PCI_HEADER_TYPE_BRIDGE  0x01    /* Header Type 1 is not only a PCI2PCI bridge */
#define PCI_HEADER_TYPE0        0x00    /* normal device header */
#define PCI_HEADER_MULTI_FUNC   0x80    /* multi function device */

/* PCI configuration device and driver */
 
#define SNOOZE_MODE             0x40    /* snooze mode */
#define SLEEP_MODE_DIS          0x00    /* sleep mode disable */

/* Standard device Type 0 configuration register offsets */
/* Note that only modulo-4 addresses are written to the address register */

#define PCI_CFG_VENDOR_ID       0x00
#define PCI_CFG_DEVICE_ID       0x02
#define PCI_CFG_COMMAND         0x04
#define PCI_CFG_STATUS          0x06
#define PCI_CFG_REVISION        0x08
#define PCI_CFG_PROGRAMMING_IF  0x09
#define PCI_CFG_SUBCLASS        0x0a
#define PCI_CFG_CLASS           0x0b
#define PCI_CFG_CACHE_LINE_SIZE 0x0c
#define PCI_CFG_LATENCY_TIMER   0x0d
#define PCI_CFG_HEADER_TYPE     0x0e
#define PCI_CFG_BIST            0x0f
#define PCI_CFG_BASE_ADDRESS_0  0x10
#define PCI_CFG_BASE_ADDRESS_1  0x14
#define PCI_CFG_BASE_ADDRESS_2  0x18
#define PCI_CFG_BASE_ADDRESS_3  0x1c
#define PCI_CFG_BASE_ADDRESS_4  0x20
#define PCI_CFG_BASE_ADDRESS_5  0x24
#define PCI_CFG_CIS             0x28
#define PCI_CFG_SUB_VENDER_ID   0x2c
#define PCI_CFG_SUB_SYSTEM_ID   0x2e
#define PCI_CFG_EXPANSION_ROM   0x30
#define PCI_CFG_CAP_PTR         0x34
#define PCI_CFG_RESERVED_0      0x35
#define PCI_CFG_RESERVED_1      0x38
#define PCI_CFG_DEV_INT_LINE    0x3c
#define PCI_CFG_DEV_INT_PIN     0x3d
#define PCI_CFG_MIN_GRANT       0x3e
#define PCI_CFG_MAX_LATENCY     0x3f
#define PCI_CFG_SPECIAL_USE     0x41
#define PCI_CFG_MODE            0x43

/* PCI-to-PCI bridge Type 1 configuration register offsets */
/* Note that only modulo-4 addresses are written to the address register */

#define PCI_CFG_PRIMARY_BUS     0x18
#define PCI_CFG_SECONDARY_BUS   0x19
#define PCI_CFG_SUBORDINATE_BUS 0x1a
#define PCI_CFG_SEC_LATENCY     0x1b
#define PCI_CFG_IO_BASE         0x1c
#define PCI_CFG_IO_LIMIT        0x1d
#define PCI_CFG_SEC_STATUS      0x1e
#define PCI_CFG_MEM_BASE        0x20
#define PCI_CFG_MEM_LIMIT       0x22
#define PCI_CFG_PRE_MEM_BASE    0x24
#define PCI_CFG_PRE_MEM_LIMIT   0x26
#define PCI_CFG_PRE_MEM_BASE_U  0x28
#define PCI_CFG_PRE_MEM_LIMIT_U 0x2c
#define PCI_CFG_IO_BASE_U       0x30
#define PCI_CFG_IO_LIMIT_U      0x32
#define PCI_CFG_ROM_BASE        0x38
#define PCI_CFG_BRG_INT_LINE    0x3c
#define PCI_CFG_BRG_INT_PIN     0x3d
#define PCI_CFG_BRIDGE_CONTROL  0x3e

/* PCI-to-CardBus bridge Type 2 configuration register offsets */

#define PCI_CFG_CB_CAP_PTR          0x14
/*  0x15 - reserved */
#define PCI_CFG_CB_SEC_STATUS       0x16
#define PCI_CFG_CB_PRIMARY_BUS      0x18    /* PCI bus no. */
#define PCI_CFG_CB_BUS              0x19    /* CardBus bus no */
#define PCI_CFG_CB_SUBORDINATE_BUS  0x1a    /* Subordinate bus no. */
#define PCI_CFG_CB_LATENCY_TIMER    0x1b    /* CardBus latency timer */
#define PCI_CFG_CB_MEM_BASE_0       0x1c
#define PCI_CFG_CB_MEM_LIMIT_0      0x20
#define PCI_CFG_CB_MEM_BASE_1       0x24
#define PCI_CFG_CB_MEM_LIMIT_1      0x28
#define PCI_CFG_CB_IO_BASE_0        0x2c
#define PCI_CFG_CB_IO_BASE_0_HI     0x2e
#define PCI_CFG_CB_IO_LIMIT_0       0x30
#define PCI_CFG_CB_IO_LIMIT_0_HI    0x32
#define PCI_CFG_CB_IO_BASE_1        0x34
#define PCI_CFG_CB_IO_BASE_1_HI     0x36
#define PCI_CFG_CB_IO_LIMIT_1       0x38
#define PCI_CFG_CB_IO_LIMIT_1_HI    0x3a
#define PCI_CFG_CB_BRIDGE_CONTROL   0x3e
#define PCI_CFG_CB_SUB_VENDOR_ID    0x40
#define PCI_CFG_CB_SUB_SYSTEM_ID    0x42
#define PCI_CFG_CB_16BIT_LEGACY     0x44
/* 0x48 - 0x7f are reserved */

/* PCI Bridge Control Register (0x3E) bits */

#define PCI_CFG_PARITY_ERROR        0x01    /* Enable parity detection on secondary interface */
#define PCI_CFG_SERR                0x02    /* SERR enable */
#define PCI_CFG_ISA_ENABLE          0x04    /* ISA Disable - bit set = disable */
#define PCI_CFG_VGA_ENABLE          0x08    /* Enable VGA addresses */
#define PCI_CFG_MASTER_ABORT        0x20    /* Signal master abort */
#define PCI_CFG_SEC_BUS_RESET       0x40    /* secondary bus reset */
#define PCI_CFG_FAST_BACK           0x80    /* FBB enabled on secondary */ 
#define PCI_CFG_PRI_DIS_TO          0x100   /* Primary Discard Timeout: 2^10 PCI cycles */ 
#define PCI_CFG_SEC_DIS_TO          0x200   /* 2ndary Discard Timeout: 2^10 PCI cycles */ 
#define PCI_CFG_DIS_TIMER_STAT      0x400   /* Discard Timer status */ 
#define PCI_CFG_DIS_TIMER_ENABLE    0x800   /* Discard Timer enable */ 

/* Cardbus Bridge Control Register (0x3E) bits */

#define PCI_CFG_CB_PARITY_ERROR     0x01    /* Enable parity detection on secondary interface */
#define PCI_CFG_CB_SERR             0x02    /* SERR enable */
#define PCI_CFG_CB_ISA_ENABLE       0x04    /* ISA Disable - bit set = disable */
#define PCI_CFG_CB_VGA_ENABLE       0x08    /* Enable VGA addresses */
#define PCI_CFG_CB_MASTER_ABORT     0x20    /* Signal master abort */
#define PCI_CFG_CB_RESET            0x40    /* Cardbus reset */
#define PCI_CFG_CB_16BIT_INT        0x80    /* Enable ints for 16-bit cards */
#define PCI_CFG_CB_PREFETCH0        0x0100  /* Memory 0 prefetch enable */
#define PCI_CFG_CB_PREFETCH1        0x0200  /* Memory 1 prefetch enable */
#define PCI_CFG_CB_POST_WRITES      0x0400  /* Posted Writes */ 

/* Extended Capability IDs */

#define PCI_EXT_CAP_PCI_PM      0x01
#define PCI_EXT_CAP_AGP         0x02
#define PCI_EXT_CAP_VPD         0x03
#define PCI_EXT_CAP_SLOTID      0x04
#define PCI_EXT_CAP_MSI         0x05
#define PCI_EXT_CAP_HOT_SWAP    0x06
#define PCI_EXT_CAP_PCIX        0x07
#define PCI_EXT_CAP_DBG_PORT    0x0A
#define PCI_EXT_CAP_CPCI_RES    0x0B
#define PCI_EXT_CAP_HPC         0x0C    /* Hot-Plug Ctrl */
#define PCI_EXT_CAP_EXP         0x10    /* PCI Express */
#define PCI_EXT_CAP_MSIX        0x11    /* MSI-X */
   
/* CompactPCI Hot Swap Control & Status Register (HSCSR) defines */

#define PCI_HS_CSR_RSVD0  0x01  /* Reserved */
#define PCI_HS_CSR_EIM    0x02  /* ENUM Interrupt Mask */
#define PCI_HS_CSR_RSVD2  0x04  /* Reserved */
#define PCI_HS_CSR_LOO    0x08  /* Blue LED On/Off */
#define PCI_HS_CSR_RSVD4  0x10  /* Reserved */
#define PCI_HS_CSR_RSVD5  0x20  /* Reserved */
#define PCI_HS_CSR_EXT    0x40  /* ENUM Status - EXTract */
#define PCI_HS_CSR_INS    0x80  /* ENUM Status - INSert */

/* Conditional defines for new configuration definitions */

/* Cache Line Size */

#ifndef PCI_CACHE_LINE_SIZE
#   if defined(_CACHE_ALIGN_SIZE)
#      define PCI_CACHE_LINE_SIZE   (_CACHE_ALIGN_SIZE >> 2)
#   else
#      define PCI_CACHE_LINE_SIZE   (32 >> 2)
#   endif
#endif /* PCI_CACHE_LINE_SIZE */

/* Latency Timer value - 255 PCI clocks */
#ifndef PCI_LATENCY_TIMER
#define PCI_LATENCY_TIMER           0xff
#endif /* PCI_LATENCY_TIMER */

#ifndef _ASMLANGUAGE

/*
 * Configuration space access macros
 */

#ifndef PCI_IN_BYTE
#define PCI_IN_BYTE(x)      sysInByte (x)
#endif

#ifndef PCI_IN_WORD
#define PCI_IN_WORD(x)      sysInWord (x)
#endif

#ifndef PCI_IN_LONG
#define PCI_IN_LONG(x)      sysInLong (x)
#endif

#ifndef PCI_OUT_BYTE
#define PCI_OUT_BYTE(x,y)   sysOutByte (x,y)
#endif

#ifndef PCI_OUT_WORD
#define PCI_OUT_WORD(x,y)   sysOutWord (x,y)
#endif

#ifndef PCI_OUT_LONG
#define PCI_OUT_LONG(x,y)   sysOutLong (x,y)
#endif

STATUS pciConfigLibInit (int mechanism, ULONG addr0, ULONG addr1, ULONG addr2);
STATUS pciFindDevice    (int vendorId, int deviceId, int index, 
             int * pBusNo, int * pDeviceNo, int * pFuncNo);
STATUS pciFindClass (int classCode, int index,  
             int * pBusNo, int * pDeviceNo, int * pFuncNo);
STATUS pciConfigInByte  (int busNo, int deviceNo, int funcNo, int address,
             UINT8 * pData);
STATUS pciConfigInWord  (int busNo, int deviceNo, int funcNo, int address,
             UINT16 * pData);
STATUS pciConfigInLong  (int busNo, int deviceNo, int funcNo, int address,
             UINT32 * pData);
STATUS pciConfigOutByte (int busNo, int deviceNo, int funcNo, int address,
             UINT8 data);
STATUS pciConfigOutWord (int busNo, int deviceNo, int funcNo, int address,
             UINT16 data);
STATUS pciConfigOutLong (int busNo, int deviceNo, int funcNo, int address,
             UINT32 data);
STATUS pciSpecialCycle  (int busNo, UINT32 message);
int pciConfigBdfPack(int busNo, int deviceNo, int funcNo);
STATUS pciDevConfig ( int pciBusNo, int pciDevNo, int pciFuncNo,
              UINT32 devIoBaseAdrs, UINT32 devMemBaseAdrs,
              UINT32 command );
STATUS pciConfigExtCapFind (UINT8 extCapFindId, int bus, int device,
    int function, UINT8 * pOffset);

STATUS pciConfigModifyLong ( int busNo, int deviceNo, int funcNo, int offset,   
                 UINT32 bitMask, UINT32 data );
STATUS pciConfigModifyWord ( int busNo, int deviceNo, int funcNo, int offset,   
                 UINT16 bitMask, UINT16 data );
STATUS pciConfigModifyByte ( int busNo, int deviceNo, int funcNo, int offset,   
                 UINT8 bitMask, UINT8 data );

/* foreach function */
typedef STATUS (*PCI_FOREACH_FUNC)(int bus, int dev, int func, void *pArg);

STATUS pciConfigForeachFunc ( UINT8 bus, BOOL recurse,
                 PCI_FOREACH_FUNC funcCheckFunc,
                 void *pArg
                 );
#endif  /* _ASMLANGUAGE */

/* For backward compatibility pciConfigBDFPack == pciConfigBdfPack */

#define pciConfigBDFPack    pciConfigBdfPack

#ifdef __cplusplus
}
#endif

#endif /* __INCpciConfigLib2h */

