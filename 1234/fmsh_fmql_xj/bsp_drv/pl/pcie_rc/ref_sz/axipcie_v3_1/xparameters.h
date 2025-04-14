#ifndef _X_PARAMETERS_H_   /* prevent circular inclusions */
#define _X_PARAMETERS_H_   /* by using protection macros */
 
 
/******************************************************************/
/* Definitions for driver AXIPCIE */
#define XPAR_XAXIPCIE_NUM_INSTANCES 1

/* Definitions for peripheral AXI_PCIE_0 */
#define XPAR_AXI_PCIE_0_DEVICE_ID 0

/*#define XPAR_AXI_PCIE_0_FAMILY zynq*/

/* 128M area*/
#define XPAR_AXI_PCIE_0_BASEADDR 0x60000000   /* 0x40000000*/
#define XPAR_AXI_PCIE_0_HIGHADDR 0x6FFFFFFF   /* 0x4FFFFFFF*/

#define XPAR_AXI_PCIE_0_INCLUDE_BAROFFSET_REG 1
#define XPAR_AXI_PCIE_0_AXIBAR_NUM 1

/* 64M area*/
#define XPAR_AXI_PCIE_0_AXIBAR_0           0x50000000  /* 0x80000000*/
#define XPAR_AXI_PCIE_0_AXIBAR_HIGHADDR_0  0x53FFFFFF  /* 0xBFFFFFFF*/

#define XPAR_AXI_PCIE_0_AXIBAR_AS_0 0
#define XPAR_AXI_PCIE_0_AXIBAR2PCIEBAR_0   0x80000000



#define XPAR_AXI_PCIE_0_AXIBAR_1 0xFFFFFFFF
#define XPAR_AXI_PCIE_0_AXIBAR_HIGHADDR_1 0x00000000

#define XPAR_AXI_PCIE_0_AXIBAR_AS_1 0
#define XPAR_AXI_PCIE_0_AXIBAR2PCIEBAR_1 0x00000000

#define XPAR_AXI_PCIE_0_AXIBAR_2 0xFFFFFFFF
#define XPAR_AXI_PCIE_0_AXIBAR_HIGHADDR_2 0x00000000

#define XPAR_AXI_PCIE_0_AXIBAR_AS_2 0
#define XPAR_AXI_PCIE_0_AXIBAR2PCIEBAR_2 0x00000000

#define XPAR_AXI_PCIE_0_AXIBAR_3 0xFFFFFFFF
#define XPAR_AXI_PCIE_0_AXIBAR_HIGHADDR_3 0x00000000

#define XPAR_AXI_PCIE_0_AXIBAR_AS_3 0
#define XPAR_AXI_PCIE_0_AXIBAR2PCIEBAR_3 0x00000000

#define XPAR_AXI_PCIE_0_AXIBAR_4 0xFFFFFFFF
#define XPAR_AXI_PCIE_0_AXIBAR_HIGHADDR_4 0x00000000

#define XPAR_AXI_PCIE_0_AXIBAR_AS_4 0
#define XPAR_AXI_PCIE_0_AXIBAR2PCIEBAR_4 0x00000000

#define XPAR_AXI_PCIE_0_AXIBAR_5 0xFFFFFFFF
#define XPAR_AXI_PCIE_0_AXIBAR_HIGHADDR_5 0x00000000

#define XPAR_AXI_PCIE_0_AXIBAR_AS_5 0
#define XPAR_AXI_PCIE_0_AXIBAR2PCIEBAR_5 0x00000000

#define XPAR_AXI_PCIE_0_PCIEBAR_NUM 1

#define XPAR_AXI_PCIE_0_PCIEBAR_AS 0

#define XPAR_AXI_PCIE_0_PCIEBAR_LEN_0 30
#define XPAR_AXI_PCIE_0_PCIEBAR2AXIBAR_0 0x00000000

#define XPAR_AXI_PCIE_0_PCIEBAR_LEN_1 16
#define XPAR_AXI_PCIE_0_PCIEBAR2AXIBAR_1 0xFFFFFFFF

#define XPAR_AXI_PCIE_0_PCIEBAR_LEN_2 16
#define XPAR_AXI_PCIE_0_PCIEBAR2AXIBAR_2 0xFFFFFFFF

#define XPAR_AXI_PCIE_0_PCIEBAR_LEN_3 0
#define XPAR_AXI_PCIE_0_PCIEBAR2AXIBAR_3 0

#define XPAR_AXI_PCIE_0_INCLUDE_RC 1


/******************************************************************/

/* Canonical definitions for peripheral AXI_PCIE_0 */
#define XPAR_AXIPCIE_0_DEVICE_ID XPAR_AXI_PCIE_0_DEVICE_ID

/*#define XPAR_AXIPCIE_0_FAMILY zynq*/

#define XPAR_AXIPCIE_0_BASEADDR 0x40000000
#define XPAR_AXIPCIE_0_HIGHADDR 0x4FFFFFFF

#define XPAR_AXIPCIE_0_INCLUDE_BAROFFSET_REG 1
#define XPAR_AXIPCIE_0_AXIBAR_NUM 1

#define XPAR_AXIPCIE_0_AXIBAR_0 0x80000000
#define XPAR_AXIPCIE_0_AXIBAR_HIGHADDR_0 0xBFFFFFFF

#define XPAR_AXIPCIE_0_AXIBAR_AS_0 0
#define XPAR_AXIPCIE_0_AXIBAR2PCIEBAR_0 0x80000000

#define XPAR_AXIPCIE_0_AXIBAR_1 0xFFFFFFFF
#define XPAR_AXIPCIE_0_AXIBAR_HIGHADDR_1 0x00000000
#define XPAR_AXIPCIE_0_AXIBAR_AS_1 0
#define XPAR_AXIPCIE_0_AXIBAR2PCIEBAR_1 0x00000000
#define XPAR_AXIPCIE_0_AXIBAR_2 0xFFFFFFFF
#define XPAR_AXIPCIE_0_AXIBAR_HIGHADDR_2 0x00000000
#define XPAR_AXIPCIE_0_AXIBAR_AS_2 0
#define XPAR_AXIPCIE_0_AXIBAR2PCIEBAR_2 0x00000000
#define XPAR_AXIPCIE_0_AXIBAR_3 0xFFFFFFFF
#define XPAR_AXIPCIE_0_AXIBAR_HIGHADDR_3 0x00000000
#define XPAR_AXIPCIE_0_AXIBAR_AS_3 0
#define XPAR_AXIPCIE_0_AXIBAR2PCIEBAR_3 0x00000000
#define XPAR_AXIPCIE_0_AXIBAR_4 0xFFFFFFFF
#define XPAR_AXIPCIE_0_AXIBAR_HIGHADDR_4 0x00000000
#define XPAR_AXIPCIE_0_AXIBAR_AS_4 0
#define XPAR_AXIPCIE_0_AXIBAR2PCIEBAR_4 0x00000000
#define XPAR_AXIPCIE_0_AXIBAR_5 0xFFFFFFFF
#define XPAR_AXIPCIE_0_AXIBAR_HIGHADDR_5 0x00000000
#define XPAR_AXIPCIE_0_AXIBAR_AS_5 0
#define XPAR_AXIPCIE_0_AXIBAR2PCIEBAR_5 0x00000000
#define XPAR_AXIPCIE_0_PCIEBAR_NUM 1
#define XPAR_AXIPCIE_0_PCIEBAR_AS 0
#define XPAR_AXIPCIE_0_PCIEBAR_LEN_0 30
#define XPAR_AXIPCIE_0_PCIEBAR2AXIBAR_0 0x00000000
#define XPAR_AXIPCIE_0_PCIEBAR_LEN_1 16
#define XPAR_AXIPCIE_0_PCIEBAR2AXIBAR_1 0xFFFFFFFF
#define XPAR_AXIPCIE_0_PCIEBAR_LEN_2 16
#define XPAR_AXIPCIE_0_PCIEBAR2AXIBAR_2 0xFFFFFFFF
#define XPAR_AXIPCIE_0_PCIEBAR_LEN_3 0
#define XPAR_AXIPCIE_0_PCIEBAR2AXIBAR_3 0
#define XPAR_AXIPCIE_0_INCLUDE_RC 1

/******************************************************************/

typedef struct pciHeaderDevice
    {
    short	vendorId;	/* vendor ID */
    short	deviceId;	/* device ID */
    short	command;	/* command register */
    short	status;		/* status register */
    char	revisionId;	/* revision ID */
    char	classCode;	/* class code */
    char	subClass;	/* sub class code */
    char	progIf;		/* programming interface */
    char	cacheLine;	/* cache line */
    char	latency;	/* latency time */
    char	headerType;	/* header type */
    char	bist;		/* BIST */
    int		base0;		/* base address 0 */
    int		base1;		/* base address 1 */
    int		base2;		/* base address 2 */
    int		base3;		/* base address 3 */
    int		base4;		/* base address 4 */
    int		base5;		/* base address 5 */
    int		cis;		/* cardBus CIS pointer */
    short	subVendorId;	/* sub system vendor ID */
    short	subSystemId;	/* sub system ID */
    int		romBase;	/* expansion ROM base address */
    int		reserved0;	/* reserved */
    int		reserved1;	/* reserved */
    char	intLine;	/* interrupt line */
    char	intPin;		/* interrupt pin */
    char	minGrant;	/* min Grant */
    char	maxLatency;	/* max Latency */
	
} PCI_HEADER_DEVICE;


#endif  /* #ifndef _X_PARAMETERS_H_  */



