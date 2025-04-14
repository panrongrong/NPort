 
#ifndef _FMSH_NFCPS_MODEL_H_   /* prevent circular inclusions */
#define _FMSH_NFCPS_MODEL_H_

/**********************************Include File*********************************/

/**********************************Type Definition******************************/
typedef struct {
  /** Identifier for the device. */
  u8 Manufacture;
  u8 DeviceId;
  /** Special options for the NandFlash. */
  u8 Options;
  /* Width of IO */
  u8 nfDataWidth;
  /** Size of the data area of a page, in bytes. */
  u16 PageSizeInBytes;
  /** Size of the spare area of a page, in bytes. */
  u16 SpareSizeInBytes;
  /** Number of pages per block */
  u16 PagePerBlock;
  /** Size of one block in kilobytes. */
  u16 BlockSizeInKBytes;
  /** Number of blocks per lun */
  u16 BlockPerLun;
  /** Size of the device in MB. */
  u16 DeviceSizeInMegaBytes;
  /** Number of pages in total device */
  u32 PageNum;
  /** Number of blocks in total device */
  u16 BlockNum;
  /** LUN Numbers in total device*/
  u8 LunNum;
  /** Address cycles */
  u8 RowAddrCycle;
  u8 ColAddrCycle;
} FNfcPs_Model_T;

/**
 * Parameter page structure of ONFI 1.0 specification.
 * Enhanced this sturcture to include ONFI 2.3 information for EZ NAND support.
 */
#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
typedef struct {
	/*
	 * Revision information and features block
	 */
	u8 Signature[4];	/**< Parameter page signature */
	u16 Revision;		/**< Revision Number */
	u16 Features;		/**< Features supported */
	u16 OptionalCmds;	/**< Optional commands supported */
	u8 Reserved0[2];	/**< ONFI 2.3: Reserved */
	u16 ExtParamPageLen;	/**< ONFI 2.3: extended parameter page
					length */
	u8 NumOfParamPages;	/**< ONFI 2.3: No of parameter pages */
	u8 Reserved1[17];	/**< Reserved */
	/*
	 * Manufacturer information block
	 */
	u8 DeviceManufacturer[12];	/**< Device manufacturer */
	u8 DeviceModel[20];		/**< Device model */
	u8 JedecManufacturerId;		/**< JEDEC Manufacturer ID */
	u8 DateCode[2];			/**< Date code */
	u8 Reserved2[13];		/**< Reserved */
	/*
	 * Memory organization block
	*/
	u32 BytesPerPage;		/**< Number of data bytes per page */
	u16 SpareBytesPerPage;		/**< Number of spare bytes per page */
	u32 BytesPerPartialPage;	/**< Number of data bytes per partial page */
	u16 SpareBytesPerPartialPage;	/**< Number of spare bytes per partial
					  page */
	u32 PagesPerBlock;		/**< Number of pages per block */
	u32 BlocksPerLun;		/**< Number of blocks per logical unit
					  (LUN) */
	u8 NumLuns;			/**< Number of LUN's */
	u8 AddrCycles;			/**< Number of address cycles */
	u8 BitsPerCell;			/**< Number of bits per cell */
	u16 MaxBadBlocksPerLun;		/**< Bad blocks maximum per LUN */
	u16 BlockEndurance;		/**< Block endurance */
	u8 GuaranteedValidBlock;	/**< Guaranteed valid blocks at
					  beginning of target */
	u16 BlockEnduranceGvb;		/**< Block endurance for guaranteed
					  valid block */
	u8 ProgramsPerPage;		/**< Number of programs per page */
	u8 PartialProgAttr;		/**< Partial programming attributes */
	u8 EccBits;			/**< Number of bits ECC
					  correctability */
	u8 InterleavedAddrBits;		/**< Number of interleaved address
					  bits */
	u8 InterleavedOperation;	/**< Interleaved operation
					  attributes */
	u8 EzNandSupport;		/**< ONFI 2.3: EZ NAND support
						parameters */
	u8 Reserved3[12];		/**< Reserved */
	/*
	 * Electrical parameters block
	*/
	u8 IOPinCapacitance;		/**< I/O pin capacitance */
	u16 TimingMode;			/**< Timing mode support */
	u16 PagecacheTimingMode;	/**< Program cache timing mode */
	u16 TProg;			/**< Maximum page program time */
	u16 TBers;			/**< Maximum block erase time */
	u16 TR;				/**< Maximum page read time */
	u16 TCcs;			/**< Maximum change column setup
					  time */
	u16 SynTimingMode;		/**< ONFI 2.3: Source synchronous
						timing mode support */
	u8 SynFeatures;			/**< ONFI 2.3: Source synchronous
						features */
	u16 ClkInputPinCap;		/**< ONFI 2.3: CLK input pin
						capacitance */
	u16 IOPinCap;			/**< ONFI 2.3: I/O pin capacitance */
	u16 InputPinCap;		/**< ONFI 2.3: Input pin capacitance
						typical */
	u8 InputPinCapMax;		/**< ONFI 2.3: Input pin capacitance
						maximum */
	u8 DrvStrength;			/**< ONFI 2.3: Driver strength
						support */
	u16 TMr;			/**< ONFI 2.3: Maximum multi-plane
						read time */
	u16 TAdl;			/**< ONFI 2.3: Program page register
						clear enhancement value */
	u16 TEr;			/**< ONFI 2.3: Typical page read time
						for EZ NAND */
	u8 Reserved4[6];		/**< Reserved */
	/*
	 * Vendor block
	*/
	u16 VendorRevisionNum;		/**< Vendor specific revision
					  number */
	u8 VendorSpecific[88];		/**< Vendor specific */
	u16 Crc;/**< Integrity CRC */
#ifdef __ICCARM__
} FNfcPs_ParaPage_T;
#pragma pack(pop)
#else
}__attribute__((packed))FNfcPs_ParaPage_T;
#endif
/**********************************Macro (inline function) Definition***********/

/**********************************Function Prototype***************************/
int FNfcPs_TranslateFlashAddress(const FNfcPs_Model_T Model, u64 Address, unsigned int Size, 
                                           u16 *Block, u16 *Page, u16 *Offset);
u16 FNfcPs_GetDeviceSizeInBlocks(const FNfcPs_Model_T model);
u32 FNfcPs_GetDeviceSizeInPages(const FNfcPs_Model_T model);
u16 FNfcPs_GetDeviceSizeInMegaBytes(const FNfcPs_Model_T model);
u64 FNfcPs_GetDeviceSizeInBytes(const FNfcPs_Model_T model);
u16 FNfcPs_GetBlockSizeInPages(const FNfcPs_Model_T model);
u16 FNfcPs_GetBlockSizeInKBytes(const FNfcPs_Model_T model);
u32 FNfcPs_GetBlockSizeInBytes(const FNfcPs_Model_T model);
u16 FNfcPs_GetPageSizeInBytes(const FNfcPs_Model_T model);

u16 FNfcPs_GetPageSpareSize(const FNfcPs_Model_T model);
u8 FNfcPs_GetRowAddrCycle(const FNfcPs_Model_T model);
u8 FNfcPs_GetColAddrCycle(const FNfcPs_Model_T model);
u8 FNfcPs_GetDeviceId(const FNfcPs_Model_T model);
u8 FNfcPs_GetDataBusWidth(const FNfcPs_Model_T model);
u8 FNfcPs_GetNumLun(const FNfcPs_Model_T model);
u8 FNfcPs_HasSmallBlocks(const FNfcPs_Model_T model);
/**********************************Constant Definition**************************/

/**********************************Variable Definition**************************/

#endif	/* prevent circular inclusions */D