/* sysTffs.c - BSP stub for TrueFFS Socket Component Driver */

/*
 * Copyright (c) 2012 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01b,19oct12,fao  add bootrom reserve area.(WIND00383467)
01a,04jul12,clx  created from stm_spear13xx/sysTffs.c rev 01a
*/

/*
This stub file provides the user with the means to create a TrueFFS compliant
socket component driver for use with any BSP. This is sample code and it is
expected, even required, to be modified after it is added to a BSP by the
project facility when the component INCLUDE_TFFS is added to a kernel project.
Look for #error and TODO statements in the sample code.

This stub does not provides code, just some procedural macros that the
generic driver code will use.

DESCRIPTION
This library must provide board-specific hardware access routines for TrueFFS.
In effect, these routines comprise the socket component driver (or drivers)
for your flash device.  At socket registration time, TrueFFS stores
the functions provided by this socket component driver in an 'FLSocket'
structure.  When TrueFFS needs to access the flash device, it uses these
functions.

Because this file is, for the most part, a device driver that exports its
functionality by registering function pointers with TrueFFS, very few of the
functions defined here are externally callable.  For the record, the only
external functions are flFitInSocketWindow() and flDelayLoop(), and you should
never have to call them.

However, one of the most important functions defined in this file is neither
referenced in an 'FLSocket' structure, nor is it externally callable.  This
function is sysTffsInit() and it should only be called by TrueFFS.  TrueFFS
calls this function at initialization time to register socket component
drivers for all the flash devices attached to your target.  It is this call
to sysTffs() that results in assigning drive numbers to the flash devices on
your target hardware.  Drive numbers are assigned by the order in which the
socket component drivers are registered. The first to be registered is drive
0, the second is drive 1, and so on up to 4.  As shipped, TrueFFS supports up
to five flash drives.

After registering socket component drivers for a flash device, you may
format the flash medium even though there is not yet a block device driver
associated with the flash (see the reference entry for the tffsDevCreate()
routine).  To format the flash medium for use with TrueFFS,
call tffsDevFormat() or, for some BSPs, sysTffsFormat().

The sysTffsFormat() routine is an optional,BSP-specific helper routine that
can be called externally. Internally, sysTffsFormat() calls tffsDevFormat()
with a pointer to a 'FormatParams' structure that is initialized to values
that leave a space on the flash device for a boot image. This space is outside
the region managed by TrueFFS.  This special region is necessary for boot
images because the normal translation and wear-leveling services of TrueFFS
are incompatible with the needs of the boot program and the boot image it
relies upon.  To write a boot image (or any other data) into this area,
use tffsBootImagePut().

The function sysTffsFormat() is only provided when a Flash SIMM has to have
the TrueFFS file system in some desired fraction of it. It is provided only
for the purpose of simplifying the process of formatting a Flash part that
that should be subdivided.

The Flash SIMM might also be referred to as RFA (Resident Flash Array) in the
following text.

Example implentations of sysTffs.c can be found in the directory

    $(WIND_BASE)/target/src/drv/tffs/sockets

The files sds860-sysTffs.c and pc386-sysTffs.c have support for single and dual
socketed PCMCIA devices as well if that might be useful to you. They both
support multiple sockets.


Finally, this file also contains define statements for symbolic constants
that determine which MTDs, translation layer modules, and other utilities
are ultimately included in TrueFFS.  These defines are as follows:

.IP "INCLUDE_TL_FTL"
To include the NOR-based translation layer module.
.IP "INCLUDE_TL_SSFDC"
To include the SSFDC-appropriate translation layer module.
.IP "INCLUDE_MTD_I28F016"
For Intel 28f016 flash devices.
.IP "INCLUDE_MTD_I28F008"
For Intel 28f008 flash devices.
.IP "INCLUDE_MTD_I28F008_BAJA"
For Intel 28f008 flash devices on the Heurikon Baja 4700.
.IP "INCLUDE_MTD_AMD"
For AMD, Fujitsu: 29F0{40,80,16} 8-bit flash devices.
.IP "INCLUDE_MTD_CFIAMD"
For CFI compliant AMD, Fujitsu flash devices.
.IP "INCLUDE_MTD_CFISCS"
For CFI/SCS flash devices.
.IP "INCLUDE_MTD_WAMD"
For AMD, Fujitsu 29F0{40,80,16} 16-bit flash devices.
.IP "INCLUDE_TFFS_BOOT_IMAGE"
To include tffsBootImagePut() in TrueFFS for Tornado.
.LP
To exclude any of the modules mentioned above, edit sysTffs.c and undefine
its associated symbolic constant.


INCLUDE FILES: flsocket.h, tffsDrv.h
*/

/* includes */

#include <vxWorks.h>
#include <taskLib.h>
#include <tffs/flsocket.h>
#include <tffs/tffsDrv.h>
#include "config.h"

/* defines */

#ifndef PRJ_BUILD
#undef  INCLUDE_MTD_CFISCS       /* CFI/SCS */
#define INCLUDE_TL_FTL           /* FTL translation layer */
#undef  INCLUDE_TL_SSFDC         /* SSFDC translation layer */
#undef  INCLUDE_SOCKET_SIMM       /* SIMM socket interface */
#undef  INCLUDE_SOCKET_PCMCIA     /* PCMCIA socket interface */
#endif  /* PRJ_BUILD */

#define INCLUDE_MTD_USR

#ifdef INCLUDE_MTD_USR
#include "sysMtd.c"
#define MTD_USR_IDENTIFY        qspiFlashIdentify
#endif /* INCLUDE_MTD_USR */

#define TFFS_FORMAT_PRINT           /* print the tffs format process */

/*
 * TODO :
 * If you don't use TrueFFS to write your boot image you might want
 * to undefine this.
 */

#undef INCLUDE_TFFS_BOOT_IMAGE      /* include tffsBootImagePut() */

/*
 * TODO :
 * set these to board specific values.
 * The values used here are fictional.
 */

#define VCC_DELAY_MSEC      100    /* Millisecs to wait for Vcc ramp up */
#define VPP_DELAY_MSEC      100    /* Millisecs to wait for Vpp ramp up */
#define KILL_TIME_FUNC      ((iz * iz) / (iz + iz)) / ((iy + iz) / (iy * iz))

/* locals */

LOCAL UINT32 sysTffsMsecLoopCount = 0;

/* forward declarations */

LOCAL FLBoolean   rfaCardDetected (FLSocket vol);
LOCAL void        rfaVccOn (FLSocket vol);
LOCAL void        rfaVccOff (FLSocket vol);
#ifdef SOCKET_12_VOLTS
LOCAL FLStatus    rfaVppOn (FLSocket vol);
LOCAL void        rfaVppOff (FLSocket vol);
#endif /* SOCKET_12_VOLTS */
LOCAL FLStatus    rfaInitSocket (FLSocket vol);
LOCAL void        rfaSetWindow (FLSocket vol);
LOCAL void        rfaSetMappingContext (FLSocket vol, unsigned page);
LOCAL FLBoolean   rfaGetAndClearCardChangeIndicator (FLSocket vol);
LOCAL FLBoolean   rfaWriteProtected (FLSocket vol);
LOCAL void        rfaRegister (void);

#ifndef DOC
#include "tffs/tffsConfig.c"
#endif /* DOC */

/*******************************************************************************
*
* sysTffsInit - board level initialization for TFFS
*
* This routine calls the socket registration routines for the socket component
* drivers that will be used with this BSP. The order of registration signifies
* the logical drive number assigned to the drive associated with the socket.
*
* RETURNS: N/A
*/

LOCAL void sysTffsInit (void)
    {

    UINT32 ix = 0;
    UINT32 iy = 1;
    UINT32 iz = 2;
    int oldTick;

    /*
     * we assume followings:
     *   - no interrupts except timer is happening.
     *   - the loop count that consumes 1 msec is in 32 bit.
     * it is done in the early stage of usrRoot() in tffsDrv().
     */

    oldTick = tickGet();
    while (oldTick == tickGet())    /* wait for next clock interrupt */
    ;

    oldTick = tickGet();
    while (oldTick == tickGet())    /* loop one clock tick */
        {
        iy = KILL_TIME_FUNC;            /* consume time */
        ix++;                            /* increment the counter */
        }

    sysTffsMsecLoopCount = ix * sysClkRateGet() / 1000;

    /*
     * TODO:
     * Call each sockets register routine here
     */

    rfaRegister ();                 /* RFA socket interface register */
    }

/*******************************************************************************
*
* rfaRegister - install routines for the Flash RFA
*
* This routine installs necessary functions for the Resident Flash Array(RFA).
*
* RETURNS: N/A
*/

LOCAL void rfaRegister (void)
{
    FLSocket vol = flSocketOf (noOfDrives);

    tffsSocket[noOfDrives] = "RFA";
	
    /*vol.window.baseAddress = FLASH_BASE_ADRS >> 12;  /* QSPI_DATA 0xFFA00000 */
    vol.window.baseAddress = FLASH_DATA_BASE_ADRS >> 12;  /* QSPI_DATA 0xFFA00000 */
    /*vol.window.baseAddress = (FLASH_DATA_BASE_ADRS + 0x800000) >> 12;  /* QSPI_DATA 0xFFA00000 */
	
    vol.cardDetected       = rfaCardDetected;
    vol.VccOn              = rfaVccOn;
    vol.VccOff             = rfaVccOff;
	
#ifdef SOCKET_12_VOLTS
    vol.VppOn              = rfaVppOn;
    vol.VppOff             = rfaVppOff;
#endif /* SOCKET_12_VOLTS */

    vol.initSocket         = rfaInitSocket;
    vol.setWindow          = rfaSetWindow;
    vol.setMappingContext  = rfaSetMappingContext;
    vol.getAndClearCardChangeIndicator = rfaGetAndClearCardChangeIndicator;
    vol.writeProtected     = rfaWriteProtected;
    noOfDrives++;

	printf("rfaRegister! \n");
	
	return;
}

/*******************************************************************************
*
* rfaCardDetected - detect if a card is present (inserted)
*
* This routine detects if a card is present (inserted).
* Always return TRUE in RFA environments since device is not removable.
*
* RETURNS: TRUE always.
*/

LOCAL FLBoolean rfaCardDetected
    (
    FLSocket vol        /* pointer identifying drive */
    )
    {
    return (TRUE);
    }

/*******************************************************************************
*
* rfaVccOn - turn on Vcc (3.3/5 Volts)
*
* This routine turns on Vcc (3.3/5 Volts).  Vcc must be known to be good
* on exit. Assumed to be ON constantly in RFA environment.
*
* RETURNS: N/A
*/

LOCAL void rfaVccOn
    (
    FLSocket vol        /* pointer identifying drive */
    )
    {
    }

/*******************************************************************************
*
* rfaVccOff - turn off Vcc (3.3/5 Volts)
*
* This routine turns off Vcc (3.3/5 Volts) (PCMCIA). Assumed to be ON
* constantly in RFA environment.
*
* RETURNS: N/A
*/

LOCAL void rfaVccOff
    (
    FLSocket vol        /* pointer identifying drive */
    )
    {
    }

#ifdef SOCKET_12_VOLTS

/*******************************************************************************
*
* rfaVppOn - turns on Vpp (12 Volts)
*
* This routine turns on Vpp (12 Volts). Vpp must be known to be good on exit.
* Assumed to be ON constantly in RFA environment. This is not optional and
* must always be implemented.
*
* RETURNS: flOK always.
*/

LOCAL FLStatus rfaVppOn
    (
    FLSocket vol        /* pointer identifying drive */
    )
    {
    return (flOK);
    }

/*******************************************************************************
*
* rfaVppOff - turns off Vpp (12 Volts)
*
* This routine turns off Vpp (12 Volts). Assumed to be ON constantly
* in RFA environment.This is not optional and must always be implemented.
*
* RETURNS: N/A
*/

LOCAL void rfaVppOff
    (
    FLSocket vol        /* pointer identifying drive */
    )
    {
    }

#endif /* SOCKET_12_VOLTS */

/*******************************************************************************
*
* rfaInitSocket - perform all necessary initializations of the socket
*
* This routine performs all necessary initializations of the socket.
*
* RETURNS: flOK always.
*/
LOCAL FLStatus rfaInitSocket
    (
    FLSocket vol        /* pointer identifying drive */
    )
    {
    return (flOK);
    }

/*******************************************************************************
*
* rfaSetWindow - set current window attributes, base address, size, etc
*
* This routine sets current window hardware attributes: Base address, size,
* speed and bus width.  The requested settings are given in the 'vol.window'
* structure.  If it is not possible to set the window size requested in
* 'vol.window.size', the window size should be set to a larger value,
* if possible. In any case, 'vol.window.size' should contain the
* actual window size (in 4 KB units) on exit.
*
* RETURNS: N/A
*/
LOCAL void rfaSetWindow
    (
    FLSocket vol        /* pointer identifying drive */
    )
{
    /* Physical base as a 4K page */
	vol.window.baseAddress = FLASH_DATA_BASE_ADRS >> 12;
	/*vol.window.baseAddress = (FLASH_DATA_BASE_ADRS+0x800000) >> 12;*/

	
	/*flSetWindowSize (&vol, (MAIN_FLASH_SIZE/2) >> 12);*/
    /*flSetWindowSize (&vol, FLASH_SIZE >> 12);*/
    flSetWindowSize (&vol, 0x2000000 >> 12);  /* 32M*/
}

/*******************************************************************************
*
* rfaSetMappingContext - sets the window mapping register to a card address
*
* This routine sets the window mapping register to a card address.
* The window should be set to the value of 'vol.window.currentPage',
* which is the card address divided by 4 KB. An address over 128MB,
* (page over 32K) specifies an attribute-space address. On entry to this
* routine vol.window.currentPage is the page already mapped into the window.
* (In otherwords the page that was mapped by the last call to this routine.)
* The page to map is guaranteed to be on a full window-size boundary.
* This is meaningful only in environments that use sliding window mechanism
* to view flash memory, like in PCMCIA. Not common in RFA environments.
*
* RETURNS: N/A
*/

LOCAL void rfaSetMappingContext
    (
    FLSocket vol,        /* pointer identifying drive */
    unsigned page        /* page to be mapped */
    )
    {
    }

/*******************************************************************************
*
* rfaGetAndClearCardChangeIndicator - return the hardware card-change indicator
*
* This routine returns TRUE if the card has been changed and FALSE if not. It
* also clears the "card-changed" indicator if it has been set.
* Always return FALSE in RFA environments since device is not removable.
*
* RETURNS: FALSE always.
*/

LOCAL FLBoolean rfaGetAndClearCardChangeIndicator
    (
    FLSocket vol        /* pointer identifying drive */
    )
    {
    return (FALSE);
    }

/*******************************************************************************
*
* rfaWriteProtected - return the write-protect state of the media
*
* This routine returns the write-protect state of the media
*
* RETURNS: FALSE always.
*/

LOCAL FLBoolean rfaWriteProtected
    (
    FLSocket vol        /* pointer identifying drive */
    )
    {
    return (FALSE);
    }

/*******************************************************************************
*
* flFitInSocketWindow - check whether the flash array fits in the socket window
*
* This routine checks whether the flash array fits in the socket window.
*
* RETURNS: A chip size guaranteed to fit in the socket window.
*/

long int flFitInSocketWindow
    (
    long int chipSize,        /* size of single physical chip in bytes */
    int      interleaving,    /* flash chip interleaving (1,2,4 etc) */
    long int windowSize       /* socket window size in bytes */
    )
    {
    if (chipSize*interleaving > windowSize) /* doesn't fit in socket window */
        {
        int  roundedSizeBits;

        /* fit chip in the socket window */

        chipSize = windowSize / interleaving;

        /* round chip size at powers of 2 */

        for (roundedSizeBits = 0; (0x1L << roundedSizeBits) <= chipSize;
             roundedSizeBits++)
        ;

        chipSize = (0x1L << (roundedSizeBits - 1));
        }

    return (chipSize);
    }

/*******************************************************************************
*
* flDelayMsecs - wait for specified number of milliseconds
*
* This routine waits for the specified number of milliseconds.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void flDelayMsecs
    (
    unsigned milliseconds       /* milliseconds to wait */
    )
    {
    UINT32 ix;
    UINT32 iy = 1;
    UINT32 iz = 2;

    /* it doesn't count time consumed in interrupt level */

    for (ix = 0; ix < milliseconds; ix++)
        {
        for (ix = 0; ix < sysTffsMsecLoopCount; ix++)
            {
            tickGet ();             /* dummy */
            iy = KILL_TIME_FUNC;    /* consume time */
            }
        }
    }

/*******************************************************************************
*
* flDelayLoop - consume the specified time
*
* This routine delays for the specified time.
*
* RETURNS: N/A
*/

void flDelayLoop
    (
    int  cycles
    )
    {
    while (--cycles)
    ;
    }

#ifdef TFFS_FORMAT_PRINT
int sysTffsProgressCb
    (
    int totalUnitsToFormat,
    int totalUnitsFormattedSoFar
    )
    {
    printf("Formatted %d of %d units(%dM) = %d.%01d %%\r",
           totalUnitsFormattedSoFar, totalUnitsToFormat, (totalUnitsToFormat*64)/1024,
           100 * totalUnitsFormattedSoFar / totalUnitsToFormat,
           1000 * totalUnitsFormattedSoFar / totalUnitsToFormat % 10
           );
    if (totalUnitsFormattedSoFar == totalUnitsToFormat)
        printf ("\n");
    return flOK;
    }

#   define PROGRESS_CB sysTffsProgressCb
#else /* TFFS_FORMAT_PRINT */
#   define PROGRESS_CB NULL
#endif /* TFFS_FORMAT_PRINT */

/*******************************************************************************
*
* sysTffsFormat - format the flash memory above an offset
*
* This routine formats the flash memory.  Because this function defines
* the symbolic constant, HALF_FORMAT, the lower half of the specified flash
* memory is left unformatted.  If the lower half of the flash memory was
* previously formated by TrueFFS, and you are trying to format the upper half,
* you need to erase the lower half of the flash memory before you format the
* upper half.  To do this, you could use:
* .CS
* tffsRawio(0, 3, 0, 8)
* .CE
* The first argument in the tffsRawio() command shown above is the TrueFFS
* drive number, 0.  The second argument, 3, is the function number (also
* known as TFFS_PHYSICAL_ERASE).  The third argument, 0, specifies the unit
* number of the first erase unit you want to erase.  The fourth argument, 8,
* specifies how many erase units you want to erase.
*
* RETURNS: OK, or ERROR if it fails.
*/

#ifdef TFFS_ONE_LOGIC_DISK  /* one logic_disk */

STATUS sysTffsFormat (void)
{
    STATUS status;

    /* Note: cfiamd keeps track of room to save at end of flash as required */
    tffsDevFormatParams params =
    { 
    	/* Reserve header area as ROM_SIZE */
    	/*{0x000000l/*ROM_SIZE, 99, 1, 0x10000l, PROGRESS_CB, {0,0,0,0}, NULL, 2, 0, NULL},*/ /* not reserved*/
    	/* {FLASH_RSV_8M_SIZE, 99, 1, 0x10000, PROGRESS_CB, {0,0,0,0}, NULL, 2, 0, NULL}, */  /*reserved header area: FLASH_RSV_8M_SIZE*/
    	{0x000000/*ROM_SIZE*/, 99, 1, 0x10000, PROGRESS_CB, {0,0,0,0}, NULL, 2, 0, NULL},
	    FTL_FORMAT
    };

    /* we assume that the drive number 0 is RFA */
    status = tffsDevFormat (0, (int)&params);

    return (status);
}

#else  /* many logic_disk */

STATUS sysTffsFormat (unsigned int tffsDriveNo)
{
    STATUS status;

    if (tffsDriveNo > 1)
    {
        printf("tffsDriveNo too large,eg:0,1\n\r");
		 return -1;
	}
	
    /* Note: cfiamd keeps track of room to save at end of flash as required */
    tffsDevFormatParams params[] =
    {
	    { 
	    	/* Reserve header area as ROM_SIZE */
	    	/*{0x000000 ROM_SIZE, 99, 1, 0x10000l, PROGRESS_CB, {0,0,0,0}, NULL, 2, 0, NULL},*/
	    	{0x1000000/*ROM_SIZE*/, 99, 1, 0x10000, PROGRESS_CB, {1,2,3,4}, "disk0", 2, 0, NULL},
		    FTL_FORMAT
	    },
	    { 
	    	/* Reserve header area as ROM_SIZE */
	    	/*{0x000000l ROM_SIZE, 99, 1, 0x10000l, PROGRESS_CB, {0,0,0,0}, NULL, 2, 0, NULL},*/
	    	{0x000000/*ROM_SIZE*/, 99, 1, 0x10000, PROGRESS_CB, {5,6,7,8}, "disk1", 2, 0, NULL},
		    FTL_FORMAT
	    }
	};


    /* we assume that the drive number 0 is RFA */
    status = tffsDevFormat (tffsDriveNo, (int)&(params[tffsDriveNo]));

    return (status);
}


STATUS  usrCreatePartitions( char *devName  ,int nPart)
{
    devname_t  baseName;
    char       autoPartName[16];
    char *     newPartName[4] = {"/p1", "/p2", "/p3", "/p4"};
    STATUS     result;
    int        i, fd;

    if (nPart > 4)  // max: 4-logic_disk
    {
		nPart = 4;
	}

    if (nPart < 1)
    {
		nPart = 1;
	}
	
	/*Name mapping */

    /* Get the base name of the device */
    fd = open (devName, 0, 0666);
    if (fd < 0)
    {
    	return (ERROR);
    }

    ioctl (fd, XBD_GETBASENAME, (_Vx_ioctl_arg_t)baseName);
    close (fd);

    for (i = 0; i < nPart; i++)
    {
        sprintf (autoPartName, "%s:%d", baseName, i+1);
        printf ("Installing mapping from %s to %s\n", autoPartName, newPartName[i]);
        fsmNameInstall (autoPartName, newPartName[i]);
    }

    /* create 4 partitions on the device all with equal sizes */
    result = xbdCreatePartition ( devName, nPart, 25, 25, 25 );
    if (result != OK)
    {
        return (ERROR);
    }

    /* create file systems atop each partition */
	for (i = 0; i < nPart; i++)
	{
		dosFsVolFormat ( newPartName[i], 0, NULL);
	}

    return (OK);
}

#endif





/* jc */
/*
normal tffs cmd & procedure
*/
#if 1
/*
->sysTffsFormat 0

->usrTffsConfig 0, 0, "/tffs0"

-> dosfsDiskFormat("/tffs0")

///////////////////////////////////
// log : 16M flash
//////////////////////////////////
-> sysTffsFormat 0
Formatted 256 of 256 units = 100.0 %
value = 0 = 0x0

-> usrTffsConfig 0, 0, "/tffs0"
Instantiating /tffs0 as rawFs,  device = 0x70001
value = 0 = 0x0

-> dosfsDiskFormat("/tffs0")
Formatting /tffs0 for DOSFS
Instantiating /tffs0 as rawFs, device = 0x70001
Formatting...Retrieved old volume params with %38 confidence:
Volume Parameters: FAT type: FAT32, sectors per cluster 0
  0 FAT copies, 0 clusters, 0 sectors per FAT
  Sectors reserved 0, hidden 0, FAT sectors 0
  Root dir entries 0, sysId (null)  , serial number d9b70000
  Label:"           " ...
Disk with 31557 sectors of 512 bytes will be formatted with:
Volume Parameters: FAT type: FAT16, sectors per cluster 2
  2 FAT copies, 15700 clusters, 62 sectors per FAT
  Sectors reserved 1, hidden 63, FAT sectors 124
  Root dir entries 512, sysId VXDOS16 , serial number d9b70000
  Label:"           " ...
OK.
value = 0 = 0x0
-> 
-> devs
drv name                
  0 /null               
  1 /tyCo/0             
  8 host:               
 11 /vio                
  3 /sd0:1              
  3 /sd0:2              
  3 /tffs0              
value = 25 = 0x19
-> 
*/
#endif

