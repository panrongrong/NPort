/****************************************************************************
 *				北京科银京成技术有限公司 版权所有
 *
 * 		 Copyright (C) 2000-2006 CoreTek Systems Inc. All Rights Reserved.
 ***************************************************************************/

/*
 * 修改记录：
 *	20060208	创建文件。
 *
 */

/**
 * @file 	Pci.h
 * @brief
 *	<li>功能：PCI的相关宏的定义和函数的声明。 </li>
 * @date 	20060208
 */

/****************************************************************************
 *				北京科银京成技术有限公司 版权所有
 *
 * 		 Copyright (C) 2000-2006 CoreTek Systems Inc. All Rights Reserved.
 ***************************************************************************/

/*
 * 修改记录：
 *	20060208	创建文件。
 *
 */

/**
 * @file 	Pci.h
 * @brief
 *	<li>功能：PCI的相关宏的定义和函数的声明。 </li>
 * @date 	20060208
 */

#ifndef NEW_PCI_H_
#define NEW_PCI_H_

#ifdef	__cplusplus
extern	"C" {
#endif

/**************************** 引用部分 *****************************************/


#define GLOBE_ACORE_CORE 1
/**************************** 定义部分 *****************************************/
/***********************************************************
 *    PCI设备配置空间中64字节的头标区
 ***********************************************************/
#define  PCI_VENDOR_ID     			0x00 /* 16位的厂商ID的偏移地址 */
#define  PCI_DEVICE_ID     			0x02 /* 16位的设备ID的偏移地址 */

#define  PCI_COMMAND     			0x04 /* 16位的命令寄存器的偏移地址 */
#define  PCI_COMMAND_IO     		0x1  /* 对应命令寄存器的位0，允许响应I/O空间的访问 */
#define  PCI_COMMAND_MEMORY    		0x2  /* 对应命令寄存器的位1，允许响应Memory空间的访问 */
#define  PCI_COMMAND_MASTER    		0x4  /* 对应命令寄存器的位2，允许设备作为总线主设备 */
#define  PCI_COMMAND_SPECIAL   		0x8  /* 对应命令寄存器的位3，允许设备参加总线上的特殊周期操作 */
#define  PCI_COMMAND_INVALIDATE   	0x10 /* 对应命令寄存器的位4，允许设备发出存储器写并无效命令 */
#define  PCI_COMMAND_VGA_PALETTE   	0x20 /* 对应命令寄存器的位5，允许设备进行特殊的调色板监听 */
#define  PCI_COMMAND_PARITY    		0x40 /* 对应命令寄存器的位6，允许设备在发现奇偶校验错时采取措施 */
#define  PCI_COMMAND_WAIT    0x80 /* 对应命令寄存器的位7，保留？ */
#define  PCI_COMMAND_SERR    		0x100 /* 对应命令寄存器的位8，允许SERR#驱动工作 */
#define  PCI_COMMAND_FAST_BACK   	0x200 /* 对应命令寄存器的位9，允许设备对不同的目标设备发出快速背对背传输 */
#define  PCI_COMMAND_INTERRUPT_DISABLE 	0x400 /* 对应命令寄存器的位10，禁止使能INTX#信号 */

#define  PCI_STATUS      			0x06 /* 16位状态寄存器的偏移地址*/
#define	 PCI_STATUS_INT	 			0x08 /*中断状态位，产生中断时该位置1，不受命令寄存器的影响*/
#define  PCI_STATUS_CAP_LIST		0x10	/* Support Capability List */
#define  PCI_STATUS_66MHZ    		0x20 /* 对应状态寄存器的位5，设备可运行在66MHz */
/*#define  PCI_STATUS_UDF    0x40 /* 对应状态寄存器的位6，保留 */
#define  PCI_STATUS_FAST_BACK   	0x80 /* 对应状态寄存器的位7，设备可以接受快速背对背传输 */
#define  PCI_STATUS_PARITY    		0x100 /* 对应状态寄存器的位8，数据奇偶错误发生 */
#define  PCI_STATUS_DEVSEL_MASK   	0x600 /* 对应状态寄存器的位9和位10掩码，表示DEVSEL信号的时间关系 */
#define  PCI_STATUS_DEVSEL_FAST   	0x000 /* 快速 */
#define  PCI_STATUS_DEVSEL_MEDIUM   0x200 /* 中速 */
#define  PCI_STATUS_DEVSEL_SLOW   	0x400 /* 满速 */
#define  PCI_STATUS_SIG_TARGET_ABORT	0x800  /* 对应状态寄存器的位11，发送目标废止 */
#define  PCI_STATUS_REC_TARGET_ABORT	0x1000  /* 对应状态寄存器的位12，接受目标废止 */
#define  PCI_STATUS_REC_MASTER_ABORT	0x2000  /* 对应状态寄存器的位13，接受主设备废止 */
#define  PCI_STATUS_SIG_SYSTEM_ERROR	0x4000  /* 对应状态寄存器的位14，系统错误 */
#define  PCI_STATUS_DETECTED_PARITY		0x8000  /* 对应状态寄存器的位15，奇偶校验错误 */

#define  PCI_CLASS_REVISION    		0x08 /* 类码和修订版本的偏移地址，高24位是分类代码，低8位是修订版本 */
#define  PCI_REVISION_ID           	0x08    /* 修订版本寄存器的偏移地址 */
#define  PCI_CLASS_PROG            	0x09    /* 寄存器编程接口寄存器的偏移地址 */
#define  PCI_CLASS_DEVICE          	0x0a    /* 设备类码寄存器的偏移地址 */
#define PCI_CLASS_CODE		0x0b	/* Device class code */
#define  PCI_CLASS_CODE_TOO_OLD	0x00
#define  PCI_CLASS_CODE_STORAGE 0x01
#define  PCI_CLASS_CODE_NETWORK 0x02
#define  PCI_CLASS_CODE_DISPLAY	0x03
#define  PCI_CLASS_CODE_MULTIMEDIA 0x04
#define  PCI_CLASS_CODE_MEMORY	0x05
#define  PCI_CLASS_CODE_BRIDGE	0x06
#define  PCI_CLASS_CODE_COMM	0x07
#define  PCI_CLASS_CODE_PERIPHERAL 0x08
#define  PCI_CLASS_CODE_INPUT	0x09
#define  PCI_CLASS_CODE_DOCKING	0x0A
#define  PCI_CLASS_CODE_PROCESSOR 0x0B
#define  PCI_CLASS_CODE_SERIAL	0x0C
#define  PCI_CLASS_CODE_WIRELESS 0x0D
#define  PCI_CLASS_CODE_I2O	0x0E
#define  PCI_CLASS_CODE_SATELLITE 0x0F
#define  PCI_CLASS_CODE_CRYPTO	0x10
#define  PCI_CLASS_CODE_DATA	0x11
/* Base Class 0x12 - 0xFE is reserved */
#define  PCI_CLASS_CODE_OTHER	0xFF

#define PCI_CLASS_SUB_CODE	0x0a	/* Device sub-class code */
#define  PCI_CLASS_SUB_CODE_TOO_OLD_NOTVGA	0x00
#define  PCI_CLASS_SUB_CODE_TOO_OLD_VGA		0x01
#define  PCI_CLASS_SUB_CODE_STORAGE_SCSI	0x00
#define  PCI_CLASS_SUB_CODE_STORAGE_IDE		0x01
#define  PCI_CLASS_SUB_CODE_STORAGE_FLOPPY	0x02
#define  PCI_CLASS_SUB_CODE_STORAGE_IPIBUS	0x03
#define  PCI_CLASS_SUB_CODE_STORAGE_RAID	0x04
#define  PCI_CLASS_SUB_CODE_STORAGE_ATA		0x05
#define  PCI_CLASS_SUB_CODE_STORAGE_SATA	0x06
#define  PCI_CLASS_SUB_CODE_STORAGE_SAS		0x07
#define  PCI_CLASS_SUB_CODE_STORAGE_OTHER	0x80
#define  PCI_CLASS_SUB_CODE_NETWORK_ETHERNET	0x00
#define  PCI_CLASS_SUB_CODE_NETWORK_TOKENRING	0x01
#define  PCI_CLASS_SUB_CODE_NETWORK_FDDI	0x02
#define  PCI_CLASS_SUB_CODE_NETWORK_ATM		0x03
#define  PCI_CLASS_SUB_CODE_NETWORK_ISDN	0x04
#define  PCI_CLASS_SUB_CODE_NETWORK_WORLDFIP	0x05
#define  PCI_CLASS_SUB_CODE_NETWORK_PICMG	0x06
#define  PCI_CLASS_SUB_CODE_NETWORK_OTHER	0x80
#define  PCI_CLASS_SUB_CODE_DISPLAY_VGA		0x00
#define  PCI_CLASS_SUB_CODE_DISPLAY_XGA		0x01
#define  PCI_CLASS_SUB_CODE_DISPLAY_3D		0x02
#define  PCI_CLASS_SUB_CODE_DISPLAY_OTHER	0x80
#define  PCI_CLASS_SUB_CODE_MULTIMEDIA_VIDEO	0x00
#define  PCI_CLASS_SUB_CODE_MULTIMEDIA_AUDIO	0x01
#define  PCI_CLASS_SUB_CODE_MULTIMEDIA_PHONE	0x02
#define  PCI_CLASS_SUB_CODE_MULTIMEDIA_OTHER	0x80
#define  PCI_CLASS_SUB_CODE_MEMORY_RAM		0x00
#define  PCI_CLASS_SUB_CODE_MEMORY_FLASH	0x01
#define  PCI_CLASS_SUB_CODE_MEMORY_OTHER	0x80
#define  PCI_CLASS_SUB_CODE_BRIDGE_HOST		0x00
#define  PCI_CLASS_SUB_CODE_BRIDGE_ISA		0x01
#define  PCI_CLASS_SUB_CODE_BRIDGE_EISA		0x02
#define  PCI_CLASS_SUB_CODE_BRIDGE_MCA		0x03
#define  PCI_CLASS_SUB_CODE_BRIDGE_PCI		0x04
#define  PCI_CLASS_SUB_CODE_BRIDGE_PCMCIA	0x05
#define  PCI_CLASS_SUB_CODE_BRIDGE_NUBUS	0x06
#define  PCI_CLASS_SUB_CODE_BRIDGE_CARDBUS	0x07
#define  PCI_CLASS_SUB_CODE_BRIDGE_RACEWAY	0x08
#define  PCI_CLASS_SUB_CODE_BRIDGE_SEMI_PCI	0x09
#define  PCI_CLASS_SUB_CODE_BRIDGE_INFINIBAND	0x0A
#define  PCI_CLASS_SUB_CODE_BRIDGE_OTHER	0x80
#define  PCI_CLASS_SUB_CODE_COMM_SERIAL		0x00
#define  PCI_CLASS_SUB_CODE_COMM_PARALLEL	0x01
#define  PCI_CLASS_SUB_CODE_COMM_MULTIPORT	0x02
#define  PCI_CLASS_SUB_CODE_COMM_MODEM		0x03
#define  PCI_CLASS_SUB_CODE_COMM_GPIB		0x04
#define  PCI_CLASS_SUB_CODE_COMM_SMARTCARD	0x05
#define  PCI_CLASS_SUB_CODE_COMM_OTHER		0x80
#define  PCI_CLASS_SUB_CODE_PERIPHERAL_PIC	0x00
#define  PCI_CLASS_SUB_CODE_PERIPHERAL_DMA	0x01
#define  PCI_CLASS_SUB_CODE_PERIPHERAL_TIMER	0x02
#define  PCI_CLASS_SUB_CODE_PERIPHERAL_RTC	0x03
#define  PCI_CLASS_SUB_CODE_PERIPHERAL_HOTPLUG	0x04
#define  PCI_CLASS_SUB_CODE_PERIPHERAL_SD	0x05
#define  PCI_CLASS_SUB_CODE_PERIPHERAL_OTHER	0x80
#define  PCI_CLASS_SUB_CODE_INPUT_KEYBOARD	0x00
#define  PCI_CLASS_SUB_CODE_INPUT_DIGITIZER	0x01
#define  PCI_CLASS_SUB_CODE_INPUT_MOUSE		0x02
#define  PCI_CLASS_SUB_CODE_INPUT_SCANNER	0x03
#define  PCI_CLASS_SUB_CODE_INPUT_GAMEPORT	0x04
#define  PCI_CLASS_SUB_CODE_INPUT_OTHER		0x80
#define  PCI_CLASS_SUB_CODE_DOCKING_GENERIC	0x00
#define  PCI_CLASS_SUB_CODE_DOCKING_OTHER	0x80
#define  PCI_CLASS_SUB_CODE_PROCESSOR_386	0x00
#define  PCI_CLASS_SUB_CODE_PROCESSOR_486	0x01
#define  PCI_CLASS_SUB_CODE_PROCESSOR_PENTIUM	0x02
#define  PCI_CLASS_SUB_CODE_PROCESSOR_ALPHA	0x10
#define  PCI_CLASS_SUB_CODE_PROCESSOR_POWERPC	0x20
#define  PCI_CLASS_SUB_CODE_PROCESSOR_MIPS	0x30
#define  PCI_CLASS_SUB_CODE_PROCESSOR_COPROC	0x40
#define  PCI_CLASS_SUB_CODE_SERIAL_1394		0x00
#define  PCI_CLASS_SUB_CODE_SERIAL_ACCESSBUS	0x01
#define  PCI_CLASS_SUB_CODE_SERIAL_SSA		0x02
#define  PCI_CLASS_SUB_CODE_SERIAL_USB		0x03
#define  PCI_CLASS_SUB_CODE_SERIAL_FIBRECHAN	0x04
#define  PCI_CLASS_SUB_CODE_SERIAL_SMBUS	0x05
#define  PCI_CLASS_SUB_CODE_SERIAL_INFINIBAND	0x06
#define  PCI_CLASS_SUB_CODE_SERIAL_IPMI		0x07
#define  PCI_CLASS_SUB_CODE_SERIAL_SERCOS	0x08
#define  PCI_CLASS_SUB_CODE_SERIAL_CANBUS	0x09
#define  PCI_CLASS_SUB_CODE_WIRELESS_IRDA	0x00
#define  PCI_CLASS_SUB_CODE_WIRELESS_IR		0x01
#define  PCI_CLASS_SUB_CODE_WIRELESS_RF		0x10
#define  PCI_CLASS_SUB_CODE_WIRELESS_BLUETOOTH	0x11
#define  PCI_CLASS_SUB_CODE_WIRELESS_BROADBAND	0x12
#define  PCI_CLASS_SUB_CODE_WIRELESS_80211A	0x20
#define  PCI_CLASS_SUB_CODE_WIRELESS_80211B	0x21
#define  PCI_CLASS_SUB_CODE_WIRELESS_OTHER	0x80
#define  PCI_CLASS_SUB_CODE_I2O_V1_0		0x00
#define  PCI_CLASS_SUB_CODE_SATELLITE_TV	0x01
#define  PCI_CLASS_SUB_CODE_SATELLITE_AUDIO	0x02
#define  PCI_CLASS_SUB_CODE_SATELLITE_VOICE	0x03
#define  PCI_CLASS_SUB_CODE_SATELLITE_DATA	0x04
#define  PCI_CLASS_SUB_CODE_CRYPTO_NETWORK	0x00
#define  PCI_CLASS_SUB_CODE_CRYPTO_ENTERTAINMENT 0x10
#define  PCI_CLASS_SUB_CODE_CRYPTO_OTHER	0x80
#define  PCI_CLASS_SUB_CODE_DATA_DPIO		0x00
#define  PCI_CLASS_SUB_CODE_DATA_PERFCNTR	0x01
#define  PCI_CLASS_SUB_CODE_DATA_COMMSYNC	0x10
#define  PCI_CLASS_SUB_CODE_DATA_MGMT		0x20
#define  PCI_CLASS_SUB_CODE_DATA_OTHER		0x80

/* Device classes and subclasses */
#define PCI_BASE_CLASS_BRIDGE		0x06
#define PCI_CLASS_BRIDGE_HOST		0x0600
#define PCI_CLASS_BRIDGE_ISA		0x0601
#define PCI_CLASS_BRIDGE_EISA		0x0602
#define PCI_CLASS_BRIDGE_MC		0x0603
#define PCI_CLASS_BRIDGE_PCI		0x0604
#define PCI_CLASS_BRIDGE_PCMCIA		0x0605
#define PCI_CLASS_BRIDGE_NUBUS		0x0606
#define PCI_CLASS_BRIDGE_CARDBUS	0x0607
#define PCI_CLASS_BRIDGE_RACEWAY	0x0608
#define PCI_CLASS_BRIDGE_OTHER		0x0680


#define  PCI_CACHE_LINE_SIZE   		0x0c /* 8位的缓冲行大小寄存器的偏移地址 */
#define  PCI_LATENCY_TIMER    		0x0d /* 8位的延时计数寄存器的偏移地址*/
#define  PCI_HEADER_TYPE    		0x0e /* 8位的头标类型寄存器的偏移地址 */
#define  PCI_HEADER_TYPE_NORMAL		0
#define  PCI_HEADER_TYPE_BRIDGE 	1
#define  PCI_HEADER_TYPE_CARDBUS 	2

/*	added by chengw, refer to Vxworks, PCI_CFG_MODE, SLEEP_MODE_DIS	*/
#define PCI_MODE	0x43
#define PCI_MODE_SLEEP_DIS	0x00


/*************************************************************
*PCI_HEADER_TYPE: pci bridge
****************************************************************/
#define PCI_PCIB_MEMORY_BASE  	0x20
#define PCI_PCIB_MEMORY_LIMIT 	0x22
#define PCI_PCIB_IO_BASE		0x1c
#define PCI_PCIB_IO_LIMIT		0x1d
#define PCI_PCIB_PREMEM_BASE	0x24
#define PCI_PCIB_PREMEM_LIMIT	0x26
#define PCI_PCIB_PREMEM_UP32BASE   0x28
#define PCI_PCIB_PREMEM_UP32LIMIT 0x2c
#define PCI_PCIB_IO_UP16BASE    0x30
#define PCI_PCIB_IO_UP16LIMIT	0x32
/*****************************************************************/
#define  PCI_BIST      0x0f /* 8位的内含自测试寄存器的偏移地址 */
#define  PCI_BIST_CODE_MASK    0x0f /* 内含自测试寄存器完成代码（位3－位0）的掩码 */
#define  PCI_BIST_START     0x40 /* 对应内含自测试寄存器的位6，启动内含自测试 */
#define  PCI_BIST_CAPABLE    0x80 /* 对应内含自测试寄存器的位7，支持BIST功能 */

/*
 * 32位的基址寄存器
 */
#define  PCI_BASE_ADDRESS_0    0x10 /* 32位 */
#define  PCI_BASE_ADDRESS_1    0x14 /* 32位 */
#define  PCI_BASE_ADDRESS_2    0x18 /* 32位 */
#define  PCI_BASE_ADDRESS_3    0x1c /* 32位 */
#define  PCI_BASE_ADDRESS_4    0x20 /* 32位 */
#define  PCI_BASE_ADDRESS_5    0x24 /* 32位 */
#define  PCI_BASE_ADDRESS_SPACE   0x01 /* 对应基址寄存器的位0，决定是何种地址空间：0 = memory, 1 = I/O */
#define  PCI_BASE_ADDRESS_SPACE_IO   0x01 /* 表示I/O地址空间 */
#define  PCI_BASE_ADDRESS_SPACE_MEMORY  0x00 /* 表示memory地址空间 */
#define  PCI_BASE_ADDRESS_MEM_TYPE_MASK 0x06 /* 对应基址寄存器的位2－位1，决定memory映射类型 */
#define  PCI_BASE_ADDRESS_MEM_TYPE_32 0x00 /* 映射到32位地址空间 */
#define  PCI_BASE_ADDRESS_MEM_TYPE_1M	0x02	/* Below 1M [obsolete] */
#define  PCI_BASE_ADDRESS_MEM_TYPE_64 0x04 /* 映射到64位地址空间 */
#define  PCI_BASE_ADDRESS_MEM_PREFETCH 0x08 /* 对应基址寄存器的位3，表示预取性 */
#define  PCI_BASE_ADDRESS_MEM_MASK  (~0x0f) /* memory地址的掩码 */
#define  PCI_BASE_ADDRESS_IO_MASK  (~0x03) /* I/O地址的掩码 */

#define  PCI_CARDBUS_CIS    0x28 /* CardBus CIS指针寄存器的偏移地址 */
#define  PCI_SUBSYSTEM_VENDOR_ID  0x2c /* 子系统厂商识别寄存器偏移地址 */
#define  PCI_SUBSYSTEM_ID    0x2e /* 子系统识别寄存器的偏移地址 */
#define  PCI_ROM_ADDRESS    0x30 /* 32位扩展ROM基址寄存器的偏移地址 */
#define  PCI_ROM_ADDRESS_ENABLE   0x01 /* 对应扩展ROM基址寄存器位0:
             * 0 = 禁止访问设备的扩展ROM；
             * 1 = 允许访问设备的扩展ROM；
                * 位31－位11表示地址,
                * 位10－位2保留
                */
#define PCI_CAPABILITY_LIST	0x34	/* Offset of first capability list entry */

/* 0x35-0x3b are reserved */
#define PCI_INTERRUPT_LINE	0x3c	/* 8 bits */
#define PCI_INTERRUPT_PIN	0x3d	/* 8 bits */
#define PCI_MIN_GNT		0x3e	/* 8 bits */
#define PCI_MAX_LAT		0x3f	/* 8 bits */

/* Header type 1 (PCI-to-PCI bridges) */
#define PCI_PRIMARY_BUS		0x18	/* Primary bus number */
#define PCI_SECONDARY_BUS	0x19	/* Secondary bus number */
#define PCI_SUBORDINATE_BUS	0x1a	/* Highest bus number behind the bridge */
#define PCI_SEC_LATENCY_TIMER	0x1b	/* Latency timer for secondary interface */
#define PCI_IO_BASE		0x1c	/* I/O range behind the bridge */
#define PCI_IO_LIMIT		0x1d
#define  PCI_IO_RANGE_TYPE_MASK 0x0f	/* I/O bridging type */
#define  PCI_IO_RANGE_TYPE_16	0x00
#define  PCI_IO_RANGE_TYPE_32	0x01
#define  PCI_IO_RANGE_MASK	~0x0f
#define PCI_SEC_STATUS		0x1e	/* Secondary status register, only bit 14 used */
#define PCI_MEMORY_BASE		0x20	/* Memory range behind */
#define PCI_MEMORY_LIMIT	0x22
#define  PCI_MEMORY_RANGE_TYPE_MASK 0x0f
#define  PCI_MEMORY_RANGE_MASK	~0x0f
#define PCI_PREF_MEMORY_BASE	0x24	/* Prefetchable memory range behind */
#define PCI_PREF_MEMORY_LIMIT	0x26
#define  PCI_PREF_RANGE_TYPE_MASK 0x0f
#define  PCI_PREF_RANGE_TYPE_32 0x00
#define  PCI_PREF_RANGE_TYPE_64 0x01
#define  PCI_PREF_RANGE_MASK	~0x0f
#define PCI_PREF_BASE_UPPER32	0x28	/* Upper half of prefetchable memory range */
#define PCI_PREF_LIMIT_UPPER32	0x2c
#define PCI_IO_BASE_UPPER16	0x30	/* Upper half of I/O addresses */
#define PCI_IO_LIMIT_UPPER16	0x32
#define  PCI_CAPABILITIES_POINTER  0x34 /* 8位的性能指针 */

/* 0x34 same as for htype 0 */
/* 0x35-0x3b is reserved */
#define PCI_ROM_ADDRESS1	0x38	/* Same as PCI_ROM_ADDRESS, but for htype 1 */
/* 0x3c-0x3d are same as for htype 0 */
#define PCI_BRIDGE_CONTROL	0x3e
#define  PCI_BRIDGE_CTL_PARITY	0x01	/* Enable parity detection on secondary interface */
#define  PCI_BRIDGE_CTL_SERR	0x02	/* The same for SERR forwarding */
#define  PCI_BRIDGE_CTL_NO_ISA	0x04	/* Disable bridging of ISA ports */
#define  PCI_BRIDGE_CTL_VGA	0x08	/* Forward VGA addresses */
#define  PCI_BRIDGE_CTL_MASTER_ABORT 0x20  /* Report master aborts */
#define  PCI_BRIDGE_CTL_BUS_RESET 0x40	/* Secondary bus reset */
#define  PCI_BRIDGE_CTL_FAST_BACK 0x80	/* Fast Back2Back enabled on secondary interface */

/* From 440ep */
#define PCI_ERREN       0x48     /* Error Enable */
#define PCI_ERRSTS      0x49     /* Error Status */
#define PCI_BRDGOPT1    0x4A     /* PCI Bridge Options 1 */
#define PCI_PLBSESR0    0x4C     /* PCI PLB Slave Error Syndrome 0 */
#define PCI_PLBSESR1    0x50     /* PCI PLB Slave Error Syndrome 1 */
#define PCI_PLBSEAR     0x54     /* PCI PLB Slave Error Address */
#define PCI_CAPID       0x58     /* Capability Identifier */
#define PCI_NEXTITEMPTR 0x59     /* Next Item Pointer */
#define PCI_PMC         0x5A     /* Power Management Capabilities */
#define PCI_PMCSR       0x5C     /* Power Management Control Status */
#define PCI_PMCSRBSE    0x5E     /* PMCSR PCI to PCI Bridge Support Extensions */
#define PCI_BRDGOPT2    0x60     /* PCI Bridge Options 2 */
#define PCI_PMSCRR      0x64     /* Power Management State Change Request Re. */

/* Header type 2 (CardBus bridges) */
#define PCI_CB_CAPABILITY_LIST	0x14
/* 0x15 reserved */
#define PCI_CB_SEC_STATUS	0x16	/* Secondary status */
#define PCI_CB_PRIMARY_BUS	0x18	/* PCI bus number */
#define PCI_CB_CARD_BUS		0x19	/* CardBus bus number */
#define PCI_CB_SUBORDINATE_BUS	0x1a	/* Subordinate bus number */
#define PCI_CB_LATENCY_TIMER	0x1b	/* CardBus latency timer */
#define PCI_CB_MEMORY_BASE_0	0x1c
#define PCI_CB_MEMORY_LIMIT_0	0x20
#define PCI_CB_MEMORY_BASE_1	0x24
#define PCI_CB_MEMORY_LIMIT_1	0x28
#define PCI_CB_IO_BASE_0	0x2c
#define PCI_CB_IO_BASE_0_HI	0x2e
#define PCI_CB_IO_LIMIT_0	0x30
#define PCI_CB_IO_LIMIT_0_HI	0x32
#define PCI_CB_IO_BASE_1	0x34
#define PCI_CB_IO_BASE_1_HI	0x36
#define PCI_CB_IO_LIMIT_1	0x38
#define PCI_CB_IO_LIMIT_1_HI	0x3a
#define  PCI_CB_IO_RANGE_MASK	~0x03
/* 0x3c-0x3d are same as for htype 0 */
#define PCI_CB_BRIDGE_CONTROL	0x3e
#define  PCI_CB_BRIDGE_CTL_PARITY	0x01	/* Similar to standard bridge control register */
#define  PCI_CB_BRIDGE_CTL_SERR		0x02
#define  PCI_CB_BRIDGE_CTL_ISA		0x04
#define  PCI_CB_BRIDGE_CTL_VGA		0x08
#define  PCI_CB_BRIDGE_CTL_MASTER_ABORT 0x20
#define  PCI_CB_BRIDGE_CTL_CB_RESET	0x40	/* CardBus reset */
#define  PCI_CB_BRIDGE_CTL_16BIT_INT	0x80	/* Enable interrupt for 16-bit cards */
#define  PCI_CB_BRIDGE_CTL_PREFETCH_MEM0 0x100	/* Prefetch enable for both memory regions */
#define  PCI_CB_BRIDGE_CTL_PREFETCH_MEM1 0x200
#define  PCI_CB_BRIDGE_CTL_POST_WRITES	0x400
#define PCI_CB_SUBSYSTEM_VENDOR_ID 0x40
#define PCI_CB_SUBSYSTEM_ID	0x42
#define PCI_CB_LEGACY_MODE_BASE 0x44	/* 16-bit PC Card legacy mode base address (ExCa) */
/* 0x48-0x7f reserved */

/* Capability lists */
#define PCI_CAP_LIST_ID		0	/* Capability ID */
#define  PCI_CAP_ID_PM		0x01	/* Power Management */
#define  PCI_CAP_ID_AGP		0x02	/* Accelerated Graphics Port */
#define  PCI_CAP_ID_VPD		0x03	/* Vital Product Data */
#define  PCI_CAP_ID_SLOTID	0x04	/* Slot Identification */
#define  PCI_CAP_ID_MSI		0x05	/* Message Signalled Interrupts */
#define  PCI_CAP_ID_CHSWP	0x06	/* CompactPCI HotSwap */
#define  PCI_CAP_ID_EXP 	0x10	/* PCI Express */

#define PCI_CAP_LIST_NEXT	1	/* Next capability in the list */
#define PCI_CAP_FLAGS		2	/* Capability defined flags (16 bits) */
#define PCI_CAP_SIZEOF		4

/* 0x35-0x3b are reserved */
#define  PCI_INTERRUPT_LINE    0x3c /* 8位的中断线寄存器 */
#define  PCI_INTERRUPT_PIN    0x3d /* 8位的中断引脚寄存器 */
#define  PCI_MIN_GNT     0x3e /* 8位的Min-Gnt */
#define  PCI_MAX_LAT     0x3f /* 8位的Max-Lat */

/* Power Management Registers */

#define  PCI_PM_CAP_VER_MASK	0x0007	/* Version */
#define  PCI_PM_CAP_PME_CLOCK	0x0008	/* PME clock required */
#define  PCI_PM_CAP_AUX_POWER	0x0010	/* Auxilliary power support */
#define  PCI_PM_CAP_DSI		0x0020	/* Device specific initialization */
#define  PCI_PM_CAP_D1		0x0200	/* D1 power state support */
#define  PCI_PM_CAP_D2		0x0400	/* D2 power state support */
#define  PCI_PM_CAP_PME		0x0800	/* PME pin supported */
#define PCI_PM_CTRL		4	/* PM control and status register */
#define  PCI_PM_CTRL_STATE_MASK 0x0003	/* Current power state (D0 to D3) */
#define  PCI_PM_CTRL_PME_ENABLE 0x0100	/* PME pin enable */
#define  PCI_PM_CTRL_DATA_SEL_MASK	0x1e00	/* Data select (??) */
#define  PCI_PM_CTRL_DATA_SCALE_MASK	0x6000	/* Data scale (??) */
#define  PCI_PM_CTRL_PME_STATUS 0x8000	/* PME pin status */
#define PCI_PM_PPB_EXTENSIONS	6	/* PPB support extensions (??) */
#define  PCI_PM_PPB_B2_B3	0x40	/* Stop clock when in D3hot (??) */
#define  PCI_PM_BPCC_ENABLE	0x80	/* Bus power/clock control enable (??) */
#define PCI_PM_DATA_REGISTER	7	/* (??) */
#define PCI_PM_SIZEOF		8

/* AGP registers */

#define PCI_AGP_VERSION		2	/* BCD version number */
#define PCI_AGP_RFU		3	/* Rest of capability flags */
#define PCI_AGP_STATUS		4	/* Status register */
#define  PCI_AGP_STATUS_RQ_MASK 0xff000000	/* Maximum number of requests - 1 */
#define  PCI_AGP_STATUS_SBA	0x0200	/* Sideband addressing supported */
#define  PCI_AGP_STATUS_64BIT	0x0020	/* 64-bit addressing supported */
#define  PCI_AGP_STATUS_FW	0x0010	/* FW transfers supported */
#define  PCI_AGP_STATUS_RATE4	0x0004	/* 4x transfer rate supported */
#define  PCI_AGP_STATUS_RATE2	0x0002	/* 2x transfer rate supported */
#define  PCI_AGP_STATUS_RATE1	0x0001	/* 1x transfer rate supported */
#define PCI_AGP_COMMAND		8	/* Control register */
#define  PCI_AGP_COMMAND_RQ_MASK 0xff000000  /* Master: Maximum number of requests */
#define  PCI_AGP_COMMAND_SBA	0x0200	/* Sideband addressing enabled */
#define  PCI_AGP_COMMAND_AGP	0x0100	/* Allow processing of AGP transactions */
#define  PCI_AGP_COMMAND_64BIT	0x0020	/* Allow processing of 64-bit addresses */
#define  PCI_AGP_COMMAND_FW	0x0010	/* Force FW transfers */
#define  PCI_AGP_COMMAND_RATE4	0x0004	/* Use 4x rate */
#define  PCI_AGP_COMMAND_RATE2	0x0002	/* Use 4x rate */
#define  PCI_AGP_COMMAND_RATE1	0x0001	/* Use 4x rate */
#define PCI_AGP_SIZEOF		12

/* PCI-X registers */

#define  PCI_X_CMD_DPERR_E      0x0001  /* Data Parity Error Recovery Enable */
#define  PCI_X_CMD_ERO          0x0002  /* Enable Relaxed Ordering */
#define  PCI_X_CMD_MAX_READ     0x0000  /* Max Memory Read Byte Count */
#define  PCI_X_CMD_MAX_SPLIT    0x0030  /* Max Outstanding Split Transactions */
#define  PCI_X_CMD_VERSION(x)   (((x) >> 12) & 3) /* Version */


/* Slot Identification */

#define PCI_SID_ESR		2	/* Expansion Slot Register */
#define  PCI_SID_ESR_NSLOTS	0x1f	/* Number of expansion slots available */
#define  PCI_SID_ESR_FIC	0x20	/* First In Chassis Flag */
#define PCI_SID_CHASSIS_NR	3	/* Chassis Number */

/* Message Signalled Interrupts registers */

#define PCI_MSI_CTL		    2		/* Various flags */
#define PCI_MSI_CTL_64BIT	0x80	/* 64-bit addresses allowed */
#define PCI_MSI_CTL_QSIZE	0x70	/* Message queue size configured */
#define PCI_MSI_CTL_QMASK	0x0e	/* Maximum queue size available */
#define PCI_MSI_CTL_ENABLE	0x01	/* MSI feature enabled */
#define PCI_MSI_RFU			3		/* Rest of capability flags */
#define PCI_MSI_ADDRESS_LO	4		/* Lower 32 bits */
#define PCI_MSI_ADDRESS_HI	8		/* Upper 32 bits (if PCI_MSI_FLAGS_64BIT set) */
#define PCI_MSI_DATA_32		8		/* 16 bits of data for 32-bit devices */
#define PCI_MSI_DATA_64		12		/* 16 bits of data for 64-bit devices */

#define PCI_MAX_PCI_DEVICES	32
#define PCI_MAX_PCI_FUNCTIONS	8

#define PCI_FIND_CAP_TTL 0x48
#define CAP_START_POS 0x40

#define PCIE_LTSSM_L0S                0x11       /* L0S state */


/*
 * 使用的系统的API接口封装
 */

/*
 * pci功能模块
 */
#define PCI_PROBE_BIOS          1
#define PCI_PROBE_CONF1         2
#define PCI_PROBE_CONF2         4
#define PCI_BIOS_IRQ_SCAN       8

/*
 * PCI接口将多功能设备视为独立的设备。每个设备的插槽/功能地址
 * 编码到一个字节中，如下：
 * 位7－位3：插槽号
 * 位2－位0：功能号
 */
#define PCI_DEVFN(slot,func) ((((slot) & 0x1f) << 3) | ((func) & 0x07))
#define PCI_SLOT(devfn)  (((devfn) >> 3) & 0x1f)
#define PCI_FUNC(devfn)  ((devfn) & 0x07)

#define PCI_BASE(baseinfo, baseaddr)  do {  if( baseinfo & PCI_BASE_ADDRESS_SPACE_IO )  {\
										baseaddr = baseinfo & 0x0fffffffe;\
										}\
									else {\
										baseaddr = baseinfo & 0x0fffffff0;\
										}}while(0)
#define PCI_BASE_IOM(baseinfo, iom) do {  if( baseinfo & PCI_BASE_ADDRESS_SPACE_IO )  {\
										iom = 1;\
										}\
									else {\
										iom = 0;\
										}}while(0)

#define PCI_BASE_MEMFETCH(baseinfo, fetch) do {  if( baseinfo & PCI_BASE_ADDRESS_MEM_PREFETCH)  {\
										fetch = 1;\
										}\
									else {\
										fetch = 0;\
										}}while(0)
#define PCI_BASE_MEMWHERE(baseinfo, where)  do {\
											   where = baseinfo>>1;\
											  }while(0)


/*
 * PCI系统占用的资源类型
 */
#define  PCI_RES_MEM 		0x10
#define PCI_RES_IO 			0x1
#define PCI_RES_PREMEM 		0x2
#define PCI_RES_IRQ 		0x4
#define PCI_RES_ALL			0x8
#define	PCI_RES_NONE		0x10


/*
 * PCI Command 寄存器操作参数
 */
#define PCI_DIS_IO      	0x00000001
#define PCI_EN_IO       	0x00000002
#define PCI_DIS_MEM     	0x00000004
#define PCI_EN_MEM      	0x00000008
#define PCI_DIS_MASTER  	0x00000010
#define PCI_EN_MASTER   	0x00000020
#define PCI_DIS_SPCCON  	0x00000040
#define PCI_EN_SPCCON   	0x00000080
#define PCI_DIS_CACHEW  	0x00000100
#define PCI_EN_CACHEW   	0x00000200
#define PCI_DIS_VGAM    	0x00000400
#define PCI_EN_VGAM    		0x00000800
#define PCI_DIS_PARITY  	0x00001000
#define PCI_EN_PARITY   	0x00002000
#define PCI_DIS_SERR    	0x00004000
#define PCI_EN_SERR     	0x00008000
#define PCI_DIS_BTB     	0x00010000
#define PCI_EN_BTB      	0x00020000
#define PCI_DIS_INTX    	0x00040000
#define PCI_EN_INTX     	0x00080000


/*
 * PCI功能调用返回值
 */
#define PCI_OP_SUCCESS          	0
#define PCI_OP_FAILED          	 	1
#define PCI_OP_NOSUPPORT        	2
#define PCI_OP_NOREADY          	3
#define PCI_OP_ERRORPARM			4

#define PCI_NO_INIT 				5
#define PCI_FOUND_DEVICE 			6
#define PCI_NO_FOUND_DEVICE 		7

#define PCI_FOUND_BUS 				8
#define PCI_NO_FOUND_BUS 			9


/*
 * 在PCI模块中使用PCI_DRV_DBG来控制显示输出.
 */
 #define PCI_DRV_DBG
/*
 * 控制是否使用pci BIOS的功能调用
 */
 #define CONFIG_PCI_BIOS
/*
 *获取pci设备链信息的缓存大小.该参数视系统中Pci设备的个数而更改
 */
#define PCILIST_BUF_SIZE 3000


#define PCI_REGION_MEM		  0x00000000	/* PCI memory space */
#define PCI_REGION_IO		  0x00000001	/* PCI IO space */
#define PCI_REGION_TYPE		  0x00000001
#define PCI_REGION_PREFETCH	  0x00000008	/* prefetchable PCI memory */
#define PCI_REGION_SYS_MEMORY 0x00000100	/* System memory */
#define PCI_REGION_RO		  0x00000200	/* Read-only memory */

#define CONFIG_SYS_PCI_CACHE_LINE_SIZE	8
/*
 * 全局变量
 */



/**********************************************************************************
以下接口为系统保留,用户不可调用
**********************************************************************************/


/*********************************************************************************
**********************************************************************************/



/**********************************************************************************
以下接口为系统向外声明的功能,用户可以使用
**********************************************************************************/



#ifdef	__cplusplus
}
#endif

#endif /*NEW_PCI_H_*/
