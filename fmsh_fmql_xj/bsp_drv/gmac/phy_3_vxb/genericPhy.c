/* genericPhy.c - driver for generic 10/100/1000 ethernet PHY chips */

/*
 * Copyright (c) 2005-2011, 2013-2015 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
16mar15,p_x  Use MII_CR_RESTART instead of reset to restart 
             auto negotiation in genPhyModeSet(). (VXW6-84227)
02feb15,p_x  Support turning off auto-negotiation. (VXW6-83965)
16mar14,xms  add handle error status. (VXW6-28077)
20jan14,xms  optimize the genPhyInit routine. (VXW6-38227) 
01r,04sep13,xms  fix NULL_RETURNS error. (WIND00414265)
01q,27feb13,y_y  Add timeout check when phy reset. (WIND00404792)
01p,15mar11,x_z  Remove workaround for MICREL KS8001 rev3
01o,14sep10,d_c  Cleanup debug prints
01n,10sep10,d_c  Add debug prints
01m,06aug10,x_z  Add workaround for MICREL KS8001 rev3.
01l,09dec09,h_k  increased VxBus version.
01k,05may08,tor  update version
01k,21mar08,z_l  Fix 1000M mode support error. (WIND00120712)
01j,20sep07,tor  VXB_VERSION_3
01i,05jul07,wap  Be sure to initialize BMCR register correctly in genPhyInit()
01h,25oct06,wap  Detect gigE mode in modeSet routine when autoneg is disabled
01g,17oct06,pdg  replaced VXB_METHOD_DRIVER_UNLINK with an address
01f,20jun06,wap  Advertise manually set modes
01e,25may06,wap  Add removal support
01d,31jan06,pdg  updated vxbus version
01c,19jan06,wap  Fix probe routine, use correct extended caps bit in status
                 register
01b,05jan06,wap  Swap InstInit2 and InstConnect methods
01a,15sep05,wap  written
*/

/*
DESCRIPTION
This module implements generic PHY access routines for 10/100/1000 copper
ethernet PHYs that comply with the 802.3u MII specification. Methods
are provided to initialize the PHY, set the media mode and check
the current media mode and link status. Ideally, this driver should
work with any 10/100 MII PHY. In practice, this isn't always feasible.
Some chips have quirks that require the use of custom code. Also, the
MII spec does not cover the use of interrupts. Not all PHYs are capable
of generating interrupts, but those that do must be configured through
vendor-specific registers, meaning that providing generic interrupt
methods is pretty much impossible.

The methods are accessed by calling the genPhyMethodsGet() routine,
which returns a pointer to a phyFuncs structure that contains
function pointers to the method routines. The functions can be invoked
through convenience macros declared in phyLib.h.

The initialization method resets always resets the PHY to a known
state, usually by toggling the power down bit in the control register
and then setting the reset bit. Then it programs the PHY for
autonegotiation. Chip-specific drivers may also do additional
setup or fixups if necessary.

The mode set routine can be used to program the PHY for a specific
media configuration. The mode word is based on the generic ifmedia
mode definitions, i.e. IFM_AUTO, IFM_100_TX, IFM_10_T, IFM_FDX and
IFM_HDX. Selecting a specific mode (other than auto) programs the
PHY for just that mode. This can be used to force the chip to run
at 10, 100Mbps or 1000Mbps and full or half duplex.

The mode get routine retrieves the current link speed and duplex
mode (i.e. what the link setting is right now) and the link state.
If the IFM_ACTIVE bit is not set in the link state, then the mode
information is set to IFM_NONE (no link available).

The interrupt control method can be used to turn interrupts for
link state change events on or off. Interrupts may be generated
for duplex, speed and link state changes.

The interrupt acknowledge method acks any pending interrupts so
that the chip will de-assert its interrupt pin.
*/

#include <vxWorks.h>
#include <vxBusLib.h>
#include <logLib.h>

#include <hwif/vxbus/vxBus.h>
#include <hwif/util/hwMemLib.h>

#include <../src/hwif/h/mii/miiBus.h>
#include <../src/hwif/h/mii/genericPhy.h>

/* defines */
/* #define GENERICPHY_DEBUG */

/*
#ifdef GENERICPHY_DEBUG
#define GENERICPHY_LOGMSG(fmt,p1,p2,p3,p4,p5,p6) GENERICPHY_LOGMSG(fmt,p1,p2,p3,p4,p5,p6)
#else
#define GENERICPHY_LOGMSG(fmt,p1,p2,p3,p4,p5,p6)
#endif
*/

#ifdef GENERICPHY_DEBUG
#define GENERICPHY_LOGMSG(x,a,b,c,d,e,f)      \
    do {                                \
        if (_func_logMsg != NULL)       \
        _func_logMsg (x,                \
            (_Vx_usr_arg_t)a,           \
            (_Vx_usr_arg_t)b,           \
            (_Vx_usr_arg_t)c,           \
            (_Vx_usr_arg_t)d,           \
            (_Vx_usr_arg_t)e,           \
            (_Vx_usr_arg_t)f);          \
    } while (FALSE)
#else
#define GENERICPHY_LOGMSG(x,a,b,c,d,e,f) do{}while(0)
#endif


/* externs */

IMPORT BOOL autoNegForce;

/* locals */

LOCAL void genPhyInit (VXB_DEVICE_ID);
LOCAL STATUS genPhyModeSet (VXB_DEVICE_ID, UINT32);
LOCAL STATUS genPhyModeGet (VXB_DEVICE_ID, UINT32 *, UINT32 *);
LOCAL STATUS genPhyIntCtl (VXB_DEVICE_ID, int);
LOCAL STATUS genPhyIntAck (VXB_DEVICE_ID);

LOCAL void genPhyDevInstInit(VXB_DEVICE_ID pDev);
LOCAL void genPhyDevInstInit2(VXB_DEVICE_ID pDev);
LOCAL void genPhyDevInstConnect(VXB_DEVICE_ID pDev);
LOCAL BOOL genPhyProbe(VXB_DEVICE_ID pDev);
LOCAL STATUS genPhyInstUnlink (VXB_DEVICE_ID, void *);

/* jc*/
UINT16 phy_read(int gmac_phyaddr, UINT16 page, UINT16 reg);
void phy_write(int gmac_phyaddr, UINT16 page, UINT16 reg, UINT16 val);

LOCAL device_method_t genPhyMethods[] =
    {
    DEVMETHOD(miiModeGet,   genPhyModeGet),
    DEVMETHOD(miiModeSet,   genPhyModeSet),
    DEVMETHOD(vxbDrvUnlink, genPhyInstUnlink),
    { 0, 0 }
    };

LOCAL struct drvBusFuncs genPhyFuncs =
    {
    genPhyDevInstInit,      /* devInstanceInit */
    genPhyDevInstInit2,     /* devInstanceInit2 */
    genPhyDevInstConnect    /* devInstanceConnect */
    };

struct vxbDevRegInfo genPhyDevRegistration =
    {
    NULL,               /* pNext */
    VXB_DEVID_DEVICE,   /* devID */
    VXB_BUSID_MII,      /* busID = MII Bus */
    VXB_VER_5_0_0,      /* busVer */
    "genericPhy",       /* drvName */
    &genPhyFuncs,       /* pDrvBusFuncs */
    genPhyMethods,      /* pMethods */
    genPhyProbe         /* devProbe */
    };

void genPhyRegister(void)
    {
    vxbDevRegister (&genPhyDevRegistration);
    return;
    }

LOCAL void genPhyDevInstInit
    (
    VXB_DEVICE_ID pDev
    )
    {
    vxbNextUnitGet (pDev);
    return;
    }

LOCAL void genPhyDevInstConnect
    (
    VXB_DEVICE_ID pDev
    )
    {
    return;
    }

/*********************************************************************
*
* genPhyInstUnlink - VxBus unlink handler
*
* This function implements the VxBus unlink method for this driver.
* We delete each media type that was originally added to the MII bus
* instance by this device and take ourselves off the miiMonitor task
* list.
*
* RETURNS: OK if shutdown succeeds, else ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS genPhyInstUnlink
    (
    VXB_DEVICE_ID pDev,
    void * unused
    )
    {
    VXB_DEVICE_ID pBus;
    MII_DRV_CTRL * pDrvCtrl;
    UINT16 miiSts;

    pDrvCtrl = (MII_DRV_CTRL *)pDev->pDrvCtrl;

    if (pDrvCtrl->miiInitialized == FALSE)
        return (ERROR);

    /* Only our parent bus can delete us. */
    if (pDrvCtrl->miiLeaving == FALSE)
        return (ERROR);

    /* Remove ourselves from the miiMonitor task list. */

    miiBusListDel (pDev);

    /* Remove media list entries. */

    if ((pBus = vxbDevParent (pDev)) == NULL)
        return (ERROR);

    miiBusRead (pDev, pDrvCtrl->miiPhyAddr, MII_STAT_REG, &miiSts);

    if (miiSts & MII_SR_EXT_STS)
        {
        miiBusMediaDel (pBus, IFM_ETHER|IFM_1000_T);
        miiBusMediaDel (pBus, IFM_ETHER|IFM_1000_T|IFM_FDX);
        }

    if (miiSts & MII_SR_TX_HALF_DPX)
        miiBusMediaDel (pBus, IFM_ETHER|IFM_100_TX);
    if (miiSts & MII_SR_TX_FULL_DPX)
        miiBusMediaDel (pBus, IFM_ETHER|IFM_100_TX|IFM_FDX);
    if (miiSts & MII_SR_10T_HALF_DPX)
        miiBusMediaDel (pBus, IFM_ETHER|IFM_10_T);
    if (miiSts & MII_SR_10T_FULL_DPX)
        miiBusMediaDel (pBus, IFM_ETHER|IFM_10_T|IFM_FDX);
    if (miiSts & MII_SR_AUTO_SEL)
        miiBusMediaDel (pBus, IFM_ETHER|IFM_AUTO);

    pDrvCtrl->miiInitialized = FALSE;

    return (OK);
    }

/*********************************************************************
*
* genPhyDevInstInit2 - vxBus instInit2 handler
*
* This routine does the final driver setup. The PHY registers
* its media types with its parent bus and adds itself to the MII
* monitoring list.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void genPhyDevInstInit2
    (
    VXB_DEVICE_ID pDev
    )
    {
    VXB_DEVICE_ID pBus;
    MII_DRV_CTRL * pDrvCtrl;
    UINT16 miiSts;

    GENERICPHY_LOGMSG("genPhyDevInstInit2(): entry for pDev: 0x%x\n",
		      (int)pDev, 0,0,0,0,0);
    
    pDrvCtrl = (MII_DRV_CTRL *)pDev->pDrvCtrl;

    if (pDrvCtrl->miiInitialized == TRUE)
	{
	GENERICPHY_LOGMSG("genPhyDevInstInit2(): already initialized\n",
			   0,0,0,0,0,0);
        return;
	}

    pDrvCtrl->miiInitialized = TRUE;

    /*
     * Tell miiBus about the media we support.
     */

    if ((pBus = vxbDevParent (pDev)) == NULL)
        return;

    miiBusRead (pDev, pDrvCtrl->miiPhyAddr, MII_STAT_REG, &miiSts);

    if (miiSts & MII_SR_EXT_STS)
        {
        miiBusMediaAdd (pBus, IFM_ETHER|IFM_1000_T);
        miiBusMediaAdd (pBus, IFM_ETHER|IFM_1000_T|IFM_FDX);
        }

    if (miiSts & MII_SR_TX_HALF_DPX)
        miiBusMediaAdd (pBus, IFM_ETHER|IFM_100_TX);
    if (miiSts & MII_SR_TX_FULL_DPX)
        miiBusMediaAdd (pBus, IFM_ETHER|IFM_100_TX|IFM_FDX);
    if (miiSts & MII_SR_10T_HALF_DPX)
        miiBusMediaAdd (pBus, IFM_ETHER|IFM_10_T);
    if (miiSts & MII_SR_10T_FULL_DPX)
        miiBusMediaAdd (pBus, IFM_ETHER|IFM_10_T|IFM_FDX);
    if (miiSts & MII_SR_AUTO_SEL)
        miiBusMediaAdd (pBus, IFM_ETHER|IFM_AUTO);

    miiBusMediaDefaultSet (pBus, IFM_ETHER|IFM_AUTO);

    /*
     * Initialize the PHY. This may perform DSP code
     * tweaking as needed.
     */
    genPhyInit (pDev);

    /* Add to the monitor list. */
    miiBusListAdd (pDev);

    return;
    }

/*********************************************************************
*
* genPhyProbe - vxBus probe handler
*
* This routine checks the ID register values of the PHY and compares
* the OUI and model numbers against a list of supported Broadcom
* PHYs. If the device isn't supported, we return FALSE.
*
* RETURNS: TRUE if device is a Broadcom 54xx PHY, otherwise FALSE
*
* ERRNO: N/A
*/

LOCAL
BOOL genPhyProbe
    (
    VXB_DEVICE_ID pDev
    )
    {
    MII_DRV_CTRL * pDrvCtrl;
    UINT16 miiId1;
    UINT16 miiId2;

    pDrvCtrl = pDev->pDrvCtrl;
    miiId1 = pDrvCtrl->miiId1;
    miiId2 = pDrvCtrl->miiId2;

    return (TRUE);
    }

LOCAL void genPhyInit
    (
    VXB_DEVICE_ID pDev
    )
{
    MII_DRV_CTRL * pDrvCtrl = (MII_DRV_CTRL *)pDev->pDrvCtrl;
	
    UINT16 miiSts;
    UINT16 miiCtl;
    UINT16 miiVal;
    int i;

#if 0
    pDrvCtrl = (MII_DRV_CTRL *)pDev->pDrvCtrl;

    /* Get status register so we can look for extended capabilities. */

    miiBusRead (pDev, pDrvCtrl->miiPhyAddr, MII_STAT_REG, &miiSts);

    /* Set reset bit and then wait for it to clear. */

    miiVal = MII_CR_RESET;
    miiBusWrite (pDev, pDrvCtrl->miiPhyAddr, MII_CTRL_REG, miiVal);

    for (i = 0; i < 1000; i++)
        {
        miiBusRead (pDev, pDrvCtrl->miiPhyAddr, MII_CTRL_REG, &miiCtl);
        if (!(miiCtl & MII_CR_RESET))
            break;
        }

    if (i == 1000)
        {
        return;
        }

    /*
     * If the extended capabilities bit is set, this is a gigE
     * PHY, so make sure we advertise gigE modes.
     */

    if (miiSts & MII_SR_EXT_STS)
        {
        /* Enable advertisement of gigE modes. */
        miiVal = MII_MASSLA_CTRL_1000T_FD|MII_MASSLA_CTRL_1000T_HD;
        miiBusWrite (pDev, pDrvCtrl->miiPhyAddr, MII_MASSLA_CTRL_REG, miiVal);
        }
#endif

#ifdef GMAC_PHY_88E1111  /* ref_613*/
    /* jc*/
    extern void test_phy_reset(void);	
    test_phy_reset();  /* reset for phy: 88e1111*/
    
    /**/
    UINT16 reg = 0;
	int gmac_phyaddr = pDrvCtrl->miiPhyAddr;
    
     reg = phy_read(gmac_phyaddr, 0, 0x14); /* reg-20*/
     printf("phy reg 0x14 = 0x%x\n",reg);
     reg |= (1<<1);
     reg |= (1<<7);
     phy_write(gmac_phyaddr, 0, 0x14, (reg));  /* tx, rx_delay of phy*/
    
     taskDelay(1);
     
     reg = 0x1000; /*phy_read(0, 0); // reg-0: bit12 (1 = Enable Auto-Negotiation Process)*/
     phy_write(gmac_phyaddr, 0, 0x0, (reg|0x9000));    /* soft_reset & auto_aneg */
	 
#if 1
     taskDelay(1);   /* for mavell-phy */
#else
     taskDelay(10);  /* for 32s-phy */
#endif


     reg = phy_read(gmac_phyaddr, 0, 0); /* reg-0*/
     printf("phy reg-0 = 0x%x\n", reg);
	reg = phy_read(gmac_phyaddr, 0, 0x14); /* reg-20*/
	printf("phy reg-20 = 0x%x\n", reg);
#endif

#if defined(GMAC_PHY_YT8511) || defined(GMAC_PHY_RTL8211)    /* ref_yuhang*/
	phy_write(gmac_phyaddr, 0, 0x1e, 0x0C);  /* tx, rx_delay of phy*/
	phy_write(gmac_phyaddr, 0, 0x1f, 0x8073);	
#endif

    return;
}

LOCAL STATUS genPhyModeSet
    (
    VXB_DEVICE_ID pDev,
    UINT32 mode
    )
    {
    MII_DRV_CTRL * pDrvCtrl;
    UINT16 miiVal;
    UINT16 miiAnar = 0;
    UINT16 gmiiAnar = 0;
    UINT16 miiCtl = 0;
    UINT16 miiSts;
    BOOL   autoneg = TRUE;

#if 0
    pDrvCtrl = (MII_DRV_CTRL *)pDev->pDrvCtrl;

    /* Get status register so we can look for extended capabilities. */

    miiBusRead (pDev, pDrvCtrl->miiPhyAddr, MII_STAT_REG, &miiSts);

    switch(IFM_SUBTYPE(mode)) {
    case IFM_AUTO:
        
        /* Set autoneg advertisement to advertise all modes. */
        
        miiAnar = MII_ANAR_10TX_HD|MII_ANAR_10TX_FD|
                  MII_ANAR_100TX_HD|MII_ANAR_100TX_FD;
        if (miiSts & MII_SR_EXT_STS)
            gmiiAnar = MII_MASSLA_CTRL_1000T_FD|MII_MASSLA_CTRL_1000T_HD;
        miiCtl = MII_CR_AUTO_EN|MII_CR_RESTART;
        break;
    case IFM_1000_T:
        
        /* Auto-negotiation is mandatory per IEEE in 1000BASE-T. */
        
        if (!(miiSts & MII_SR_EXT_STS))
            return(ERROR);
        if ((mode & IFM_GMASK) == IFM_FDX)
            gmiiAnar = MII_MASSLA_CTRL_1000T_FD;
        else
            gmiiAnar = MII_MASSLA_CTRL_1000T_HD;
        miiCtl = MII_CR_AUTO_EN|MII_CR_RESTART;
        break;
    case IFM_100_TX:
        if (autoNegForce)
            {
            miiCtl = MII_CR_100|MII_CR_AUTO_EN|MII_CR_RESTART;
            if ((mode & IFM_GMASK) == IFM_FDX)
                {
                miiAnar = MII_ANAR_100TX_FD;
                miiCtl |= MII_CR_FDX;
                }
            else
                miiAnar = MII_ANAR_100TX_HD;
            }
        else
            {
            autoneg = FALSE;

            /*
             * Intel's PHY requires restarting auto-negotiation
             * or software reset in order for speed/duplex changes
             * to take effect. Use restarting auto-neg here.
             */
            
            miiCtl = MII_CR_100|MII_CR_RESTART;
            if ((mode & IFM_GMASK) == IFM_FDX)
                miiCtl |= MII_CR_FDX;
            }
        break;
    case IFM_10_T:
        if (autoNegForce)
            {
            miiCtl = MII_CR_AUTO_EN|MII_CR_RESTART;
            if ((mode & IFM_GMASK) == IFM_FDX)
                {
                miiAnar = MII_ANAR_10TX_FD;
                miiCtl |= MII_CR_FDX;
                }
            else
                miiAnar = MII_ANAR_10TX_HD;
            }
        else
            {
            autoneg = FALSE;
            miiCtl = MII_CR_RESTART;
            if ((mode & IFM_GMASK) == IFM_FDX)
                miiCtl |= MII_CR_FDX;
            }
        break;
    default:
        return (ERROR);
    }
	
	genPhyInit (pDev);

    if (autoneg)
        {
        miiBusRead (pDev, pDrvCtrl->miiPhyAddr, MII_AN_ADS_REG, &miiVal);
        miiVal &= ~(MII_ANAR_10TX_HD|MII_ANAR_10TX_FD|
                    MII_ANAR_100TX_HD|MII_ANAR_100TX_FD);
        miiVal |= miiAnar;
        miiBusWrite (pDev, pDrvCtrl->miiPhyAddr, MII_AN_ADS_REG, miiVal);

        if (miiSts & MII_SR_EXT_STS)
            {
            miiBusRead (pDev, pDrvCtrl->miiPhyAddr, MII_MASSLA_CTRL_REG, &miiVal);
            miiVal &= ~(MII_MASSLA_CTRL_1000T_HD|MII_MASSLA_CTRL_1000T_FD);
            miiVal |= gmiiAnar;
            miiBusWrite (pDev, pDrvCtrl->miiPhyAddr, MII_MASSLA_CTRL_REG, miiVal);
            }
        }

    miiBusRead (pDev, pDrvCtrl->miiPhyAddr, MII_CTRL_REG, &miiVal);
    miiVal &= ~(MII_CR_FDX|MII_CR_100|MII_CR_1000|MII_CR_AUTO_EN|MII_CR_RESTART);
    miiVal |= miiCtl;
    miiBusWrite (pDev, pDrvCtrl->miiPhyAddr, MII_CTRL_REG, miiVal);
#endif

    return(OK);
}

LOCAL STATUS genPhyModeGet
    (
    VXB_DEVICE_ID pDev,
    UINT32 * mode,
    UINT32 * status
    )
    {
    UINT16 miiSts;
    UINT16 miiCtl;
    UINT16 miiAnar;
    UINT16 miiLpar;
    UINT16 gmiiAnar = 0;
    UINT16 gmiiLpar = 0;
    UINT16 anlpar;
    MII_DRV_CTRL * pDrvCtrl;

    pDrvCtrl = (MII_DRV_CTRL *)pDev->pDrvCtrl;

    /**ode |= IFM_100_TX|IFM_FDX;  // for zhimingda*/
    *mode = IFM_ETHER;
    *status = IFM_AVALID;
	

    GENERICPHY_LOGMSG("genPhyModeGet(): entry\n", 0,0,0,0,0,0);

    /* read MII status register once to unlatch link status bit */
    miiBusRead (pDev, pDrvCtrl->miiPhyAddr, MII_STAT_REG, &miiSts);
	GENERICPHY_LOGMSG("(reg%d)miiPhyAddr=0x%X (miiSts=0x%X) \n", MII_STAT_REG, pDrvCtrl->miiPhyAddr, miiSts, 4,5,6);

    /* read again to know its current value */
    miiBusRead (pDev, pDrvCtrl->miiPhyAddr, MII_STAT_REG, &miiSts);
	GENERICPHY_LOGMSG("(reg%d)miiSts=0x%X \n", MII_STAT_REG, miiSts, 3,4,5,6);

    /* no link bit means no carrier. */
    if (!(miiSts & MII_SR_LINK_STATUS) || (miiSts == 0xFFFF))
    {
        *mode |= IFM_NONE;
		GENERICPHY_LOGMSG("genPhyModeGet(): pDev: 0x%x no carrier\n",
			  (int)pDev,0,0,0,0,0);
        return (OK);
    }

    *status |= IFM_ACTIVE;

    /*
     * read the control, ability advertisement and link
     * partner advertisement registers.
     */

    miiBusRead (pDev, pDrvCtrl->miiPhyAddr, MII_CTRL_REG, &miiCtl);
 	GENERICPHY_LOGMSG("(reg%d)miiCtl=0x%X \n", MII_CTRL_REG, miiCtl, 3,4,5,6);
	
    miiBusRead (pDev, pDrvCtrl->miiPhyAddr, MII_AN_ADS_REG, &miiAnar);
	GENERICPHY_LOGMSG("(reg%d)miiAnar=0x%X \n", MII_AN_ADS_REG, miiAnar, 3,4,5,6);
	
    miiBusRead (pDev, pDrvCtrl->miiPhyAddr, MII_AN_PRTN_REG, &miiLpar);
	GENERICPHY_LOGMSG("(reg%d)miiLpar=0x%X \n", MII_AN_PRTN_REG, miiLpar, 3,4,5,6);

    if (miiSts & MII_SR_EXT_STS)
    {
        miiBusRead (pDev, pDrvCtrl->miiPhyAddr, MII_MASSLA_CTRL_REG, &gmiiAnar);
		GENERICPHY_LOGMSG("(reg%d)gmiiAnar=0x%X \n", MII_MASSLA_CTRL_REG, gmiiAnar, 3,4,5,6);
		
        miiBusRead (pDev, pDrvCtrl->miiPhyAddr, MII_MASSLA_STAT_REG, &gmiiLpar);
		GENERICPHY_LOGMSG("(reg%d)gmiiLpar=0x%X \n", MII_MASSLA_STAT_REG, gmiiLpar, 3,4,5,6);
    }
	

#if 0 /* jc */
	/* (void) miiBusRead (pDev, pDrvCtrl->miiPhyAddr, 20, &reg20); */
	/* printf("====reg20: 0x%08X =====\n", reg20); */
	/* GENERICPHY_LOGMSG("====reg20: 0x%08X =====\n", reg20, 2,3,4,5,6); */
	UINT32 reg20 = 0;
	reg20 = phy_read (2, 21);
	/* printf("====reg20: 0x%08X =====\n", reg20); */
	GENERICPHY_LOGMSG("====page2_reg21: 0x%04X =====\n", reg20, 2,3,4,5,6);
#endif

    /*
     * If autoneg is on, figure out the link settings from the
     * advertisement and partner ability registers. If autoneg is
     * off, use the settings in the control register.
     */

    if (miiCtl & MII_CR_AUTO_EN)
     {
        anlpar = miiAnar & miiLpar;
		
        if (gmiiAnar & MII_MASSLA_CTRL_1000T_FD && gmiiLpar & MII_MASSLA_STAT_LP1000T_FD)
        {    
        	*mode |= IFM_1000_T|IFM_FDX;
			
#ifdef GENERICPHY_DEBUG
			GENERICPHY_LOGMSG("==IFM_1000_T|IFM_FDX== \n", 1,2,3,4,5,6);
#else
			logMsg("==IFM_1000_T|IFM_FDX== \n", 1,2,3,4,5,6);
#endif
        }
        else if (gmiiAnar & MII_MASSLA_CTRL_1000T_HD && gmiiLpar & MII_MASSLA_STAT_LP1000T_HD)
        {    
        	*mode |= IFM_1000_T|IFM_HDX;
#ifdef GENERICPHY_DEBUG
			GENERICPHY_LOGMSG("==IFM_1000_T|IFM_HDX== \n", 1,2,3,4,5,6);
#else
			logMsg("==IFM_1000_T|IFM_HDX== \n", 1,2,3,4,5,6);
#endif
		}
        else if (anlpar & MII_ANAR_100TX_FD)
        {    
        	*mode |= IFM_100_TX|IFM_FDX;
#ifdef GENERICPHY_DEBUG
			GENERICPHY_LOGMSG("==IFM_100_TX|IFM_FDX== \n", 1,2,3,4,5,6);
#else
			/* logMsg("==IFM_100_TX|IFM_FDX== \n", 1,2,3,4,5,6); */
#endif
		}
        else if (anlpar & MII_ANAR_100TX_HD)
        {    
        	*mode |= IFM_100_TX|IFM_HDX;
#ifdef GENERICPHY_DEBUG
			GENERICPHY_LOGMSG("==IFM_100_TX|IFM_HDX== \n", 1,2,3,4,5,6);
#else
			logMsg("==IFM_100_TX|IFM_HDX== \n", 1,2,3,4,5,6);
#endif
		}
        else if (anlpar & MII_ANAR_10TX_FD)
        {    
        	*mode |= IFM_10_T|IFM_FDX;
#ifdef GENERICPHY_DEBUG
			GENERICPHY_LOGMSG("==IFM_10_T|IFM_FDX== \n", 1,2,3,4,5,6);
#else
			logMsg("==IFM_10_T|IFM_FDX== \n", 1,2,3,4,5,6);
#endif
		}
        else if (anlpar & MII_ANAR_10TX_HD)
        {    
        	*mode |= IFM_10_T|IFM_HDX;
#ifdef GENERICPHY_DEBUG
			GENERICPHY_LOGMSG("==IFM_10_T|IFM_HDX== \n", 1,2,3,4,5,6);
#else
			logMsg("==IFM_10_T|IFM_FDX== \n", 1,2,3,4,5,6);
#endif
		}
        else
        {    
        	*mode |= IFM_NONE;
#ifdef GENERICPHY_DEBUG
			GENERICPHY_LOGMSG("==IFM_NONE== \n", 1,2,3,4,5,6);
#else
			logMsg("==IFM_NONE== \n", 1,2,3,4,5,6);
#endif
		}
		
		GENERICPHY_LOGMSG("genPhyModeGet(): pDev: 0x%x auto-neg ON,"
			  " mode: 0x%x\n", (int)pDev,(int)*mode,0,0,0,0);
    }
    else
    {
        if (miiCtl & MII_CR_FDX)
            *mode |= IFM_FDX;
        else
            *mode |= IFM_HDX;
        if ((miiCtl & (MII_CR_100 | MII_CR_1000)) == (MII_CR_100 | MII_CR_1000))
            *mode |= IFM_1000_T;
        else if (miiCtl & MII_CR_100)
            *mode |= IFM_100_TX;
        else
            *mode |= IFM_10_T;
	
		GENERICPHY_LOGMSG("genPhyModeGet(): pDev: 0x%x auto-neg off,"
			  " mode: 0x%x\n",(int)pDev,(int)*mode,0,0,0,0);
    }

    return (OK);
}

/*
 * Enable or disable PHY interrupts. If no interrupts supported,
 * return ERROR.
 */

LOCAL STATUS genPhyIntCtl
    (
    VXB_DEVICE_ID pDev,
    int ctl
    )
    {
    return (ERROR);
    }

/*
 * Ack PHY interrupts. Return OK if an interrupt was pending, error
 * if not.
 */

LOCAL STATUS genPhyIntAck
    (
    VXB_DEVICE_ID pDev
    )
    {
    return (ERROR);
    }

#if 1

#define ETH_DRV_NAME    "gem"  /* "emac" */

FUNCPTR fmqlMiiPhyRead  = NULL;
FUNCPTR	fmqlMiiPhyWrite = NULL;
FUNCPTR fmqlMiiPhyShow  = NULL;

STATUS fmqlMiiPhyFuncInit(int unit)
{
	VXB_DEVICE_ID netDev;	
	
	netDev = vxbInstByNameFind (ETH_DRV_NAME, unit);
	if (NULL == netDev)
	{	
		return ERROR;
	}

	fmqlMiiPhyRead  = vxbDevMethodGet (netDev, DEVMETHOD_CALL(miiRead));
	fmqlMiiPhyWrite = vxbDevMethodGet (netDev, DEVMETHOD_CALL(miiWrite));
	fmqlMiiPhyShow  = vxbDevMethodGet (netDev, DEVMETHOD_CALL(busDevShow));

	return OK;
}

STATUS fmqlPhyRead
    (
    UINT8 unit,
    UINT8 phyAddr,
    UINT8 regAddr,
    UINT16 *dataVal
    )
{
	VXB_DEVICE_ID netDev;
	STATUS val = ERROR;

	netDev = vxbInstByNameFind (ETH_DRV_NAME, unit);
	
	if (NULL == netDev)
	{
		GENERICPHY_LOGMSG("fmqlPhyRead: __LINE__=%d\n", __LINE__, 2,3,4,5,6);
		return ERROR;
	}

	val = fmqlMiiPhyFuncInit(unit);
	if (val == ERROR)
	{	
		GENERICPHY_LOGMSG("fmqlPhyRead: __LINE__=%d\n", __LINE__, 2,3,4,5,6);
		return ERROR;
	}
	
	if (NULL == fmqlMiiPhyRead)
	{	
		GENERICPHY_LOGMSG("fmqlPhyRead: __LINE__=%d\n", __LINE__, 2,3,4,5,6);
		return ERROR;
	}
	
	val = fmqlMiiPhyRead(netDev, phyAddr, regAddr, dataVal);

	return val;
}

STATUS fmqlPhyWrite
    (
    UINT8 unit,
    UINT8 phyAddr,
    UINT8 regAddr,
    UINT16 dataVal
    )
{
	VXB_DEVICE_ID netDev;
	STATUS val = ERROR;

	netDev = vxbInstByNameFind (ETH_DRV_NAME, unit);

	if (NULL == netDev)
    {   
        GENERICPHY_LOGMSG("fmqlPhyWrite: __LINE__=%d\n", __LINE__, 2,3,4,5,6);
		return ERROR;
    }
	
	val = fmqlMiiPhyFuncInit(unit);
	if(val == ERROR)
    {   
        GENERICPHY_LOGMSG("fmqlPhyWrite: __LINE__=%d\n", __LINE__, 2,3,4,5,6);
		return ERROR;
    }
	
	if (NULL == fmqlMiiPhyWrite)
    {
        GENERICPHY_LOGMSG("fmqlPhyWrite: __LINE__=%d\n", __LINE__, 2,3,4,5,6);
		return ERROR;
    }
	
	val = fmqlMiiPhyWrite(netDev, phyAddr, regAddr, dataVal);

	return val;
}


int get_phyaddr_by_unit(int unit_x)
{
	int phyaddr = 0;
	
	switch (unit_x)
	{
	case 0:  /* gmac_0*/
		phyaddr = GMAC_0_PHY_ADDR;
		break;
	
	case 1:  /* gmac_1*/
		phyaddr = GMAC_1_PHY_ADDR;
		break;

	default:
		phyaddr = 0;
		break;		
	}

	return phyaddr;	
}

int get_unit_by_phyaddr(int phyaddr)
{
	int unit = 0;
	
	if (phyaddr == GMAC_0_PHY_ADDR)
	{
		unit = 0;
	}
	else if (phyaddr == GMAC_1_PHY_ADDR)
	{
		unit = 1;
	}
	else
	{
		unit = -1;
	}

	return unit;  /* default: unit-0*/
}

#endif



/* 
demo_board: phy-88e116R
*/
#ifdef GMAC_PHY_88E1116R   

/*
PHY: Marvell 88E1116R
*/
/*#define NETPHY_ADDR           (0x07)   // must be changed by usr-define*/
#define MII_MARVELL_PHY_PAGE  (22)

UINT16 phy1116R_MiiRead(int gmac_phyaddr, UINT16 page, UINT16 reg, UINT16 *readValue)
{
	UINT8 unit = 0; /* 0: gmac_0,  1: gamc_1*/

	unit = get_unit_by_phyaddr(gmac_phyaddr);	
	if (unit == -1)
	{
		printf("phyaddr err:%d and exit! \n", gmac_phyaddr);
		return 0;
	}	
	
	fmqlPhyWrite(unit, gmac_phyaddr, MII_MARVELL_PHY_PAGE, (page & 0x0F));
	fmqlPhyRead(unit, gmac_phyaddr, reg, readValue);   
	
	return *readValue;
}

UINT16 phy1116R_MiiWrite(int gmac_phyaddr, UINT16 page, UINT16 reg, UINT16 writeValue)
{
	UINT8 unit = 0; /* 0: gmac_0,  1: gamc_1*/
	
	unit = get_unit_by_phyaddr(gmac_phyaddr);	
	if (unit == -1)
	{
		printf("phyaddr err:%d and exit! \n", gmac_phyaddr);
		return 0;
	}	
	
	fmqlPhyWrite(unit, gmac_phyaddr, MII_MARVELL_PHY_PAGE, (page & 0x0F));
	fmqlPhyWrite(unit, gmac_phyaddr, reg, writeValue);	
	
	return writeValue;
}

#endif


/*
PHY: Marvell 88E1111
*/
#ifdef GMAC_PHY_88E1111   

UINT16 phy1111_MiiRead(int gmac_phyaddr, UINT16 page, UINT16 reg, UINT16 *readValue)
{
	UINT8 unit = 0; /* 0: gmac_0,  1: gamc_1*/
	
	unit = get_unit_by_phyaddr(gmac_phyaddr);	
	if (unit == -1)
	{
		printf("phyaddr err:%d and exit! \n", gmac_phyaddr);
		return 0;
	}	
	
	fmqlPhyRead(unit, gmac_phyaddr, reg, readValue);   
	
	return *readValue;
}

UINT16 phy1111_MiiWrite(int gmac_phyaddr, UINT16 page, UINT16 reg, UINT16 writeValue)
{
	UINT8 unit = 0; /* 0: gmac_0,  1: gamc_1*/
	
	unit = get_unit_by_phyaddr(gmac_phyaddr);	
	if (unit == -1)
	{
		printf("phyaddr err:%d and exit! \n", gmac_phyaddr);
		return 0;
	}	
	
	fmqlPhyWrite(unit, gmac_phyaddr, reg, writeValue);	
	
	return writeValue;
}
#endif

/*
PHY: YutaiChetong YT8511 or RTL8211
*/
#if defined(GMAC_PHY_YT8511) || defined(GMAC_PHY_RTL8211) 

UINT16 phy8511_MiiRead(int gmac_phyaddr, UINT16 page, UINT16 reg, UINT16 *readValue)
{
	UINT8 unit = 0; /* 0: gmac_0,  1: gamc_1*/
	
	unit = get_unit_by_phyaddr(gmac_phyaddr);	
	if (unit == -1)
	{
		printf("phyaddr err:%d and exit! \n", gmac_phyaddr);
		return 0;
	}	
	
	fmqlPhyRead(unit, gmac_phyaddr, reg, readValue);   
	
	return *readValue;
}

UINT16 phy8511_MiiWrite(int gmac_phyaddr, UINT16 page, UINT16 reg, UINT16 writeValue)
{
	UINT8 unit = 0; /* 0: gmac_0,  1: gamc_1*/
	
	unit = get_unit_by_phyaddr(gmac_phyaddr);	
	if (unit == -1)
	{
		printf("phyaddr err:%d and exit! \n", gmac_phyaddr);
		return 0;
	}	
	
	fmqlPhyWrite(unit, gmac_phyaddr, reg, writeValue);	
	
	return writeValue;
}
#endif


/* 
verify_board: phy-9031 (KSZ9031RNX-Microchip)
*/
#ifdef GMAC_PHY_KSZ9031   

/*#define NETPHY_ADDR  (0x2)  // must be changed by usr-define*/

int get_9031_phy_addr(int unit)
{
	int phy_addr = get_phyaddr_by_unit(unit);
	return phy_addr;
}


/*
the gmac_0, gmac_1 use the same mdio: mdio_0
*/
int get_9031_unit(int unit)
{
	int unit_x = 0;
	
	switch (unit)
	{
	case 0:  /* gmac_0*/
		unit_x = 0;
		break;
	
	case 1:  /* gmac_1*/
		unit_x = 1;
		break;
	}
	
	return unit_x;  /* default:0*/
}


UINT16 phy9031_extRead(UINT16 unit, UINT16 reg, UINT16 *readValue)
{
	int phyAddr = 0;
	int devAddr = 0;
	int devReg = 0;
	int unit_x = 0;
	
	/*fmqlPhyWrite(unit, phy_addr, MII_MARVELL_PHY_PAGE, (page & 0x0F));*/
	
	/*
	MMD Register Read		Read MMD 
			- Device Address 2h, 
			- Register 11h – 13h for the magic packet’s MAC address.		
		1. Write Register Dh with 0002h // Set up register address for MMD – Device Address 2h.
		2. Write Register Eh with 0011h // Select Register 11h of MMD – Device Address 2h.
		3. Write Register Dh with 8002h // Select register data for MMD – Device Address 2h, Register 11h.
		4. Read Register Eh // Read data in MMD – Device Address 2h, Register 11h.
		5. Read Register Eh // Read data in MMD – Device Address 2h, Register 12h.
		6. Read Register Eh // Read data in MMD – Device Address 2h, Re
	*/
	devReg = reg;
	phyAddr = get_9031_phy_addr(unit);
	devAddr = 0x02;
	
	unit_x = get_9031_unit(unit);
	
	/*select register addr for mmd*/
	/*miiBusWrite(pDev, pDrvCtrl->miiPhyAddr, 0x0d, devAddr);*/
	fmqlPhyWrite(unit_x, phyAddr, 0x0d, devAddr);
	
	/*select register for mmd*/
	/*miiBusWrite(pDev, pDrvCtrl->miiPhyAddr, 0x0e, devReg);*/
	fmqlPhyWrite(unit_x, phyAddr, 0x0e, devReg);
	
	/*setup mode*/
	/*miiBusWrite(pDev, pDrvCtrl->miiPhyAddr, 0x0d, 0x8000 | devAddr);*/
	fmqlPhyWrite(unit_x, phyAddr, 0x0d, (0x8000 | devAddr));
	
	/*write the value*/
	/*miiBusRead(pDev, pDrvCtrl->miiPhyAddr, 0x0e, &regVal);*/
	fmqlPhyRead(unit_x, phyAddr, 0x0e, readValue);
	
	return *readValue;
}

void phy9031_extWrite(UINT16 unit, UINT16 reg, UINT16 writeValue)
{
	int phyAddr = 0;
	int devAddr = 0;
	int devReg = 0;
	int unit_x = 0;
	
	/*
	MMD Register Write	Write MMD 
			- Device Address 2h, 
			- Register 10h = 0001h 		to enable link-up detection to trigger PME for WOL.
		1. Write Register Dh with 0002h // Set up register address for MMD – Device Address 2h.
		2. Write Register Eh with 0010h // Select Register 10h of MMD – Device Address 2h.
		3. Write Register Dh with 4002h // Select register data for MMD – Device Address 2h, Register 10h.
		4. Write Register Eh with 0001h // Write value 0001h to MMD – Device Address 2h, Register 10h.
	*/
	devReg = reg;
	phyAddr = get_9031_phy_addr(unit);
	devAddr = 0x02;
	
	unit_x = get_9031_unit(unit);
	
	/*select register addr for mmd*/
	/*miiBusWrite(pDev, pDrvCtrl->miiPhyAddr, 0x0d, devAddr);*/
	fmqlPhyWrite(unit_x, phyAddr, 0x0d, devAddr);
	
	/*select register for mmd*/
	/*miiBusWrite(pDev, pDrvCtrl->miiPhyAddr, 0x0e, devReg);*/
	fmqlPhyWrite(unit_x, phyAddr, 0x0e, devReg);
	
	/*setup mode*/
	/*miiBusWrite(pDev, pDrvCtrl->miiPhyAddr, 0x0d, 0x4000 | devAddr);*/
	fmqlPhyWrite(unit_x, phyAddr, 0x0d, (0x4000 | devAddr));
	
	/*write the value*/
	/*miiBusWrite(pDev, pDrvCtrl->miiPhyAddr, 0x0e, regVal);*/
	fmqlPhyWrite(unit_x, phyAddr, 0x0e, writeValue);

	return;
}



int read_all_flag2 = 0;

/*
gmac_0
*/
UINT16 phy_read0(UINT16 reg)
{
	UINT16 val = 0;
	int unit = 0;
	
	fmqlPhyRead(unit, get_9031_phy_addr(unit), reg, &val);

	if (0 == read_all_flag2)
	{
		printf("gmac_%d->phy_read(phy_addr-%d,reg-0x%X)= 0x%04X \n", \
			    unit, get_9031_phy_addr(unit), reg, val);
	}
	return val;
}

void phy_write0(UINT16 reg, UINT16 val)
{
	int unit = 0;
	
	fmqlPhyWrite(unit, get_9031_phy_addr(unit), reg, val);

	if (0 == read_all_flag2)
	{
		printf("gmac_%d->phy_write(phy_addr-%d,reg-0x%X): 0x%04X \n", \
			    unit, get_9031_phy_addr(unit), reg, val);
	}
	return;
}

/*
gmac_1
*/
UINT16 phy_read1(UINT16 reg)
{
	UINT16 val = 0;
	int unit = 1;

	fmqlPhyRead(unit, get_9031_phy_addr(unit), reg, &val);

	if (0 == read_all_flag2)
	{
		printf("gmac_%d->phy_read(phy_addr-%d,reg-0x%X)= 0x%04X \n", \
			    unit, get_9031_phy_addr(unit), reg, val);
	}
	return val;
}

void phy_write1(UINT16 reg, UINT16 val)
{
	int unit = 1;
	
	fmqlPhyWrite(unit, get_9031_phy_addr(unit), reg, val);

	if (0 == read_all_flag2)
	{
		printf("gmac_%d->phy_write(phy_addr-%d,reg-0x%X): 0x%04X \n", \
			   unit, get_9031_phy_addr(unit), reg, val);
	}
	return;
}

void phy_9031_init(void)
{
	int unit = 0;
	UINT16 reg = 0;
	
#if 0
		unit = 0;
		phy9031_extWrite(unit, 0x04, 0x7);
		phy9031_extWrite(unit, 0x05, 0x7);
		phy9031_extWrite(unit, 0x06, 0x7);
		phy9031_extWrite(unit, 0x08, 0x1EF);
	
		unit = 1;
		phy9031_extWrite(unit, 0x04, 0x7);
		phy9031_extWrite(unit, 0x05, 0x7);
		phy9031_extWrite(unit, 0x06, 0x7);
		phy9031_extWrite(unit, 0x08, 0x1EF);
	
#else
		unit = 0;
		phy9031_extWrite(unit, 0x04, 0x0);
		phy9031_extWrite(unit, 0x05, 0x0);
		phy9031_extWrite(unit, 0x06, 0x0);
		
		phy9031_extWrite(unit, 0x08, 0x3FF);  /* max-1F*/
		/*phy9031_extWrite(unit, 0x08, 0); // min-0*/
	
		unit = 1;
		phy9031_extWrite(unit, 0x04, 0x0);
		phy9031_extWrite(unit, 0x05, 0x0);
		phy9031_extWrite(unit, 0x06, 0x0);

		/*
		Bit9~5: GTX_CLK	Pad Skew	
				RGMII GTX_CLK input pad skew control (0.06 ns/step)		RW 01_111
		Bit4~0: RX_CLK Pad Skew		
				RGMII RX_CLK output pad skew control (0.06 ns/step)		RW 0_1111
		*/
		phy9031_extWrite(unit, 0x08, 0x3FF);  /* max-1F*/
		/*phy9031_extWrite(unit, 0x08, 0);  // min-0*/
#endif
	
	/*
	read_all_flag2 = 1;

	reg = phy_read0(0);        // KSZ9031RNX_CTRL;
	reg = reg | (0x1u << 12);  // auto-negotiation enable
	phy_write0(0, reg);
	
	reg = phy_read1(0);        // KSZ9031RNX_CTRL;
	reg = reg | (0x1u << 12);  // auto-negotiation enable
	phy_write1(0, reg);

	read_all_flag2 = 0;
	*/
	return;
}

void phy_9031_rd_all(void)
{
	UINT32 i = 0;
	UINT16 regv;
	UINT8 unit = 0;

	for (unit = 0; unit < 2; unit++)
	{
		printf("----phy unit:%d, addr:%d----- \n", unit, get_9031_phy_addr(unit));
		
		for (i=0; i<32; i++)
		{
			/*(void) miiBusRead (pDev, pDrvCtrl->miiPhyAddr, i, &regv);*/
			fmqlPhyRead(unit, get_9031_phy_addr(unit), i, &regv);
			
			printf("reg: %d, val= 0x%.4X \n", i, regv);
		}
		
		printf("\n");
	}

	return;
}

void phy_9031_rd_ext(void)
{
	UINT16 regv;
	UINT8 unit = 0;
	
	unit = 0;
	printf("----phy_idx:%d, phy_addr:%d---- \n", unit, get_9031_phy_addr(unit));
	
	phy9031_extRead(unit, 0x04, &regv);
	printf("reg-0x04 = 0x%.4x \n", regv);
	
	phy9031_extRead(unit, 0x05, &regv);
	printf("reg-0x05 = 0x%.4x \n", regv);
	
	phy9031_extRead(unit, 0x06, &regv);
	printf("reg-0x06 = 0x%.4x \n", regv);
	
	phy9031_extRead(unit, 0x08, &regv);
	printf("reg-0x08 = 0x%.4x \n", regv);

	printf("\n");
	
	unit = 1;
	printf("----phy_idx:%d, phy_addr:%d---- \n", unit, get_9031_phy_addr(unit));
	
	phy9031_extRead(unit, 0x04, &regv);
	printf("reg-0x04 = 0x%.4x \n", regv);
	
	phy9031_extRead(unit, 0x05, &regv);
	printf("reg-0x05 = 0x%.4x \n", regv);
	
	phy9031_extRead(unit, 0x06, &regv);
	printf("reg-0x06 = 0x%.4x \n", regv);
	
	phy9031_extRead(unit, 0x08, &regv);
	printf("reg-0x08 = 0x%.4x \n", regv);

	printf("\n");
	
	return;
}

UINT16 phy9031_MiiRead(int gmac_phyaddr, UINT16 page, UINT16 reg, UINT16 *readValue)
{
	UINT8 unit = 0;  /* 0: gmac_0,  1: gamc_1*/
	
	if (gmac_phyaddr == GMAC_0_PHY_ADDR)
	{
		unit = 0;
	}
	else if (gmac_phyaddr == GMAC_1_PHY_ADDR)
	{
		unit = 1;
	}
	else
	{
		printf("phyaddr err:%d and exit! \n", gmac_phyaddr);
		return 0;
	}
	
	fmqlPhyRead(unit, get_9031_phy_addr(unit), reg, readValue);   
	
	return *readValue;
}

UINT16 phy9031_MiiWrite(int gmac_phyaddr, UINT16 page, UINT16 reg, UINT16 writeValue)
{
	UINT8 unit = 0; /* 0: gmac_0,  1: gamc_1*/
	
	if (gmac_phyaddr == GMAC_0_PHY_ADDR)
	{
		unit = 0;
	}
	else if (gmac_phyaddr == GMAC_1_PHY_ADDR)
	{
		unit = 1;
	}	
	else
	{
		printf("phyaddr err:%d and exit! \n", gmac_phyaddr);
		return 0;
	}
	
	fmqlPhyWrite(unit, get_9031_phy_addr(unit), reg, writeValue);	
	
	return writeValue;
}

#endif


#if 1

int read_all_flag = 0;

UINT16 phy_read(int gmac_phyaddr, UINT16 page, UINT16 reg)
{
	UINT16 val = 0;

#if defined(GMAC_PHY_88E1116R)
	phy1116R_MiiRead(gmac_phyaddr, page, reg, &val);

#elif defined(GMAC_PHY_88E1111)
	phy1111_MiiRead(gmac_phyaddr, page, reg, &val);

#elif defined(GMAC_PHY_YT8511) || defined(GMAC_PHY_RTL8211) 
	phy8511_MiiRead(gmac_phyaddr, page, reg, &val);

#elif defined(GMAC_PHY_KSZ9031)
	phy9031_MiiRead(gmac_phyaddr, page, reg, &val);
#endif

	if (0 == read_all_flag)
	{
		printf("gmac_%d,phy_read(page-%d,reg-0x%X)= 0x%04X \n", gmac_phyaddr, page, reg, val);
	}
	return val;
}

UINT16 phy_read2(int gmac_phyaddr, UINT16 page, UINT16 reg)
{
	UINT16 val = 0;

#if defined(GMAC_PHY_88E1116R)
	phy1116R_MiiRead(gmac_phyaddr, page, reg, &val);

#elif defined(GMAC_PHY_88E1111)
	phy1111_MiiRead(gmac_phyaddr, page, reg, &val);

#elif defined(GMAC_PHY_YT8511) || defined(GMAC_PHY_RTL8211) 
	phy8511_MiiRead(gmac_phyaddr, page, reg, &val);

#elif defined(GMAC_PHY_KSZ9031)
	phy9031_MiiRead(gmac_phyaddr, page, reg, &val);
#endif

	/*if (0 == read_all_flag)*/
	{
		printf("gmac_%d,phy_read(page-%d,reg-0x%X)= 0x%04X \n", gmac_phyaddr, page, reg, val);
	}
	return val;
}

void phy_write(int gmac_phyaddr, UINT16 page, UINT16 reg, UINT16 val)
{
#if defined(GMAC_PHY_88E1116R)
	phy1116R_MiiWrite(gmac_phyaddr, page, reg, val);

#elif defined(GMAC_PHY_88E1111)
	phy1111_MiiWrite(gmac_phyaddr, page, reg, val);

#elif defined(GMAC_PHY_YT8511) || defined(GMAC_PHY_RTL8211) 
	phy8511_MiiWrite(gmac_phyaddr, page, reg, val);

#elif defined(GMAC_PHY_KSZ9031)
	phy9031_MiiWrite(gmac_phyaddr, page, reg, val);
#endif

	if (0 == read_all_flag)
	{
		printf("phy_write(page-%d,reg-0x%X): 0x%04X \n", page, reg, val);
	}
	return;
}

void phy_read_all(int page, int gmac_phyaddr)
{
	int reg = 0;
	int gmac_x = 0;
	
	read_all_flag = 1;

	gmac_x = get_unit_by_phyaddr(gmac_phyaddr);	
	if (gmac_x == -1)
	{
		printf("phyaddr err:%d and exit! \n", gmac_phyaddr);
		return 0;
	}	

#if defined(GMAC_PHY_88E1116R)
	printf("\n\n====88E1116R_phy(page:%d,gmac_%d), phy-%d====\n", page, gmac_x, gmac_phyaddr);

#elif defined(GMAC_PHY_88E1111)
	printf("\n\n====88E1111_phy(page:%d,gmac_%d), phy-%d====\n", page, gmac_x, gmac_phyaddr);

#elif defined(GMAC_PHY_YT8511)
	printf("\n\n====YT8511_phy(page:%d,gmac_%d), phy-%d====\n",  page, gmac_x, gmac_phyaddr);

#elif defined(GMAC_PHY_RTL8211) 
		printf("\n\n====RTL8211_phy(page:%d,gmac_%d), phy-%d====\n",  page, gmac_x, gmac_phyaddr);

#elif defined(GMAC_PHY_KSZ9031)
	printf("\n\n====KSZ9031_phy(page:%d,gmac_%d), phy-%d====\n", page, gmac_x,  gmac_phyaddr);

#endif
	
	for (reg=0; reg<32; reg++)
	{
		printf(" reg(%d)= 0x%04X \n", reg, phy_read(gmac_phyaddr, page, reg));
	}
	printf("===============(page:%d,gmac_%d)=============\n\n", page, gmac_x);
	
	read_all_flag = 0;
	return;
}


#endif

