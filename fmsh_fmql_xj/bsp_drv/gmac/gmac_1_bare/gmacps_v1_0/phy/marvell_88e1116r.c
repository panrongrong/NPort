/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file  marvell_88e1111.c
*
* marvell 88e1116r phy driver
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 0.01   Hansen Yang  12/24/2018  First Release
*</pre>
******************************************************************************/

/***************************** Include Files *********************************/
#include "marvell_88e1116r.h"
/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

/************************** Variable Definitions *****************************/

/************************** Function *****************************************/
/**
* @brief: write 1116r data to phy through MDIO
* @para: FGmacPs_Instance_T *pGmac
* @para: u8 page
* @para: u8 regAddr : index of phy reg
* @para: u32 regdata
* @return: u32 states
*/
u32 mvl88e1116r_reg_write(FGmacPs_Instance_T * pGmac,u8 page,u8 regAddr,u32 regdata)
{
	FGmacPs_MacPortMap_T *pGmacPortMap = pGmac->base_address;
    FGmacPs_PhyConfig_T *pPhyConfig = pGmac->phy_cfg;

	if((regdata&0xFFFF00000) != 0)
		return ETHERNET_PHY_PARAM_ERR;

    /* wait idle */
    if(fmsh_mdio_idle(pGmac)!=ETHERNET_PHY_OK){
        return ETHERNET_PHY_ERR;
    }

    /* write page */
    u32 reg=0;
    reg = (pPhyConfig->mdio_address << 11)|(pGmac->csr_clk << 2);
    /*reg &= ~GMAC_GAR_GR;  //clear GR*/
    reg |= (PHY_88E1116R_PA<<6);
    reg |=  (GMAC_GAR_GB|GMAC_GAR_GW);
    FMSH_OUT32_32(page,pGmacPortMap->GMAC_GDR);
	FMSH_OUT32_32(reg,pGmacPortMap->GMAC_GAR);

    /* wait idle */
    if(fmsh_mdio_idle(pGmac)!=ETHERNET_PHY_OK){
        return ETHERNET_PHY_ERR;
    }

    /* write data */
    reg = (pPhyConfig->mdio_address << 11)|(pGmac->csr_clk << 2);
	/*reg &= ~GMAC_GAR_GR;*/
	reg |= (regAddr << 6);
	reg |=  (GMAC_GAR_GB|GMAC_GAR_GW);
	FMSH_OUT32_32(regdata,pGmacPortMap->GMAC_GDR);
	FMSH_OUT32_32(reg,pGmacPortMap->GMAC_GAR);

    /* wait idle */
    if(fmsh_mdio_idle(pGmac)!=ETHERNET_PHY_OK){
        return ETHERNET_PHY_ERR;
    }

	return ETHERNET_PHY_OK;
}

/**
* @brief: Read phy reg data through MDIO
* @para: FGmacPs_Instance_T *pGmac
* @para: u8 page
* @para: u8 regAddr : reg index of phy reg
* @return: u32 data
*/
u32 mvl88e1116r_reg_read(FGmacPs_Instance_T * pGmac,u8 page,u8 regAddr)
{
	FGmacPs_MacPortMap_T *pGmacPortMap = pGmac->base_address;
    FGmacPs_PhyConfig_T *pPhyConfig = pGmac->phy_cfg;
	volatile u32 reg=0;
	volatile u32 datareg;

    /* wait idle */
    if(fmsh_mdio_idle(pGmac)!=ETHERNET_PHY_OK){
        return ETHERNET_PHY_ERR;
    }

    /* write page */
    reg = (pPhyConfig->mdio_address << 11)|(pGmac->csr_clk << 2);
    /*reg &= ~GMAC_GAR_GR;*/
    reg |= (PHY_88E1116R_PA<<6);
    reg |= (GMAC_GAR_GB|GMAC_GAR_GW);
    FMSH_OUT32_32(page,pGmacPortMap->GMAC_GDR);
	FMSH_OUT32_32(reg,pGmacPortMap->GMAC_GAR);

    /* wait idle */
    if(fmsh_mdio_idle(pGmac)!=ETHERNET_PHY_OK){
        return ETHERNET_PHY_ERR;
    }

    /* read data */
    reg = (pPhyConfig->mdio_address << 11)|(pGmac->csr_clk << 2);
	/*reg &= ~GMAC_GAR_GR;*/
	reg |= (regAddr << 6);
	reg |= GMAC_GAR_GB;
	/*reg &= ~GMAC_GAR_GW;*/
	FMSH_OUT32_32(reg,pGmacPortMap->GMAC_GAR);

    if(fmsh_mdio_idle(pGmac)!=ETHERNET_PHY_OK){
        return ETHERNET_PHY_ERR;
    }

	datareg = FMSH_IN32_32(pGmacPortMap->GMAC_GDR);
	return datareg;
}

/**
* @brief: detect 88e1116r
* @para: FGmacPs_Instance_T *pGmac
* @return: u8 states
*/
u8 mvl88e1116r_detect(FGmacPs_Instance_T * pGmac)
{
    FGmacPs_PhyConfig_T *pPhyConfig = pGmac->phy_cfg;
	u32 reg;
    u8 index=0;

    for(index=0;index<32;index++){
        pPhyConfig->mdio_address=index;
        reg = mvl88e1116r_reg_read(pGmac,PAGE0,PHY_88E1116R_ID1);
        if(reg==MVL88E_ID){
			reg = mvl88e1116r_reg_read(pGmac,PAGE0,PHY_88E1116R_ID2);
/*			if ((reg & PHY_88E1116R_ID2_MOD_MUN_MASK)==PHY_88E1116R_ID2_MOD_MUN){*/
/*				PHY_TRACE_OUT(FMSH_ENET_PHY_DEBUG, "MVL88E1116R detect 0x%x \r\n",pPhyConfig->mdio_address);*/
			return ETHERNET_PHY_OK;
/*			}*/
        }
    }
    PHY_TRACE_OUT(FMSH_ENET_PHY_DEBUG, "PHY detect fail \r\n");
    pPhyConfig->mdio_address=32;
    return ETHERNET_PHY_ERR;
}

/**
* @brief: Setting MDIO address & clock, get ID
* @para: FGmacPs_Instance_T * pGmac
* @return: u8 states
*/
u8 mvl88e1116r_init(FGmacPs_Instance_T * pGmac)
{
    FGmacPs_PhyConfig_T *pPhyConfig = pGmac->phy_cfg;
	u32 reg;
	u32 phy_timeout;
    u8 ret;

    if(pPhyConfig->auto_detect_ad_en==1){
        ret=mvl88e1116r_detect(pGmac);
        if(ret!=ETHERNET_PHY_OK){
            return ETHERNET_PHY_ERR;
        }
    }

    /* read ID */
    phy_timeout=0;
    do{
        reg = mvl88e1116r_reg_read(pGmac,PAGE0,PHY_88E1116R_ID1);
        /*reg=mic_phy_read(pGmac,2);*/
        phy_timeout++;
        if(phy_timeout>_88E1116R_TIME_OUT)
        {
            PHY_TRACE_OUT(FMSH_ENET_PHY_DEBUG, "PHY ID read timeout \r\n");
            return ETHERNET_PHY_TIMEOUT;
        }
        delay_ms(1);
    }while(reg != MVL88E_ID);
    PHY_TRACE_OUT(FMSH_ENET_PHY_DEBUG, "88e1116r ID get \r\n");

    return ETHERNET_PHY_OK;
}

/**
* @brief: print every page0 reg value
* @para: FGmacPs_Instance_T * pGmac
* @return: u8 states
*/
u8 mvl88e1116r_reg_dump(FGmacPs_Instance_T * pGmac)
{
    u8 i=0;
    u16 reg;
    /*mvl88e1116r_init(pGmac);*/
    printf("88e1116r reg:\r\n");
    for(i=0;i<32;i++){
        reg=mvl88e1116r_reg_read(pGmac,PAGE0,i);
        printf("reg%d : %x\r\n",i,reg);
    }
    return ETHERNET_PHY_OK;
}

/**
* @brief: set MDI auto-cross mode
* @para: FGmacPs_Instance_T * pGmac
* @para: u8 mode
* 0:MDI ; 1:MDIX ; 2:reserved ; 3:auto(Suggestion)
* @return: void
*/
void mvl88e1116r_MDIautoX(FGmacPs_Instance_T * pGmac, u8 mode)
{
    u16 reg;
    reg = mvl88e1116r_reg_read(pGmac,PAGE0,PHY_88E1116R_CSC1);
    RESET_BIT(reg,PHY_88E1116R_CSC1_MDIX);
    reg|=(mode<<5);
    mvl88e1116r_reg_write(pGmac,PAGE0,PHY_88E1116R_CSC1,reg);
}

/**
* @brief: restart Auto-Negotiation
* @para: FGmacPs_Instance_T * pGmac
* @return: u8 states
*/
u8 mvl88e1116r_restart_AN(FGmacPs_Instance_T * pGmac)
{
    u16 reg;
    u32 phy_timeout;
    reg=mvl88e1116r_reg_read(pGmac,PAGE0,PHY_88E1116R_CR);
    SET_BIT(reg,PHY_88E1116R_CR_AN_EN);
    mvl88e1116r_reg_write(pGmac,PAGE0,PHY_88E1116R_CR,reg);

    reg=mvl88e1116r_reg_read(pGmac,PAGE0,PHY_88E1116R_CR);
    SET_BIT(reg,PHY_88E1116R_CR_RST_CP_AN);
    mvl88e1116r_reg_write(pGmac,PAGE0,PHY_88E1116R_CR,reg);
    /* wait AN done */
    phy_timeout=0;
    do{
        reg = mvl88e1116r_reg_read(pGmac,PAGE0,PHY_88E1116R_STAT);
        phy_timeout++;
        if(phy_timeout>_88E1116R_TIME_OUT)
        {
            PHY_TRACE_OUT(FMSH_ENET_PHY_DEBUG, "PHY ang timeout \r\n");
            return ETHERNET_PHY_TIMEOUT;
        }
        delay_ms(1);
    }while((reg&PHY_88E1116R_STAT_ANF) == 0);
    return ETHERNET_PHY_OK;
}

/**
* @brief: SW reset device, wait link up
* @para: FGmacPs_Instance_T * pGmac
* @return: u8 states
*/
u8 mvl88e1116r_reset(FGmacPs_Instance_T * pGmac)
{
    u16 reg;
    u32 phy_timeout=0;
	
    reg=mvl88e1116r_reg_read(pGmac,PAGE0,PHY_88E1116R_CR);
    SET_BIT(reg,PHY_88E1116R_CR_RST);
    mvl88e1116r_reg_write(pGmac,PAGE0,PHY_88E1116R_CR,reg);
	
    /* wait reset done */
    phy_timeout=0;
	
    do
	{
		reg = mvl88e1116r_reg_read(pGmac,PAGE0,PHY_88E1116R_CR);
		
		phy_timeout++;
		/*if (phy_timeout > _88E1116R_TIME_OUT)*/
		if (phy_timeout > 60)
		{
            PHY_TRACE_OUT(FMSH_ENET_PHY_DEBUG, "PHY reset timeout \r\n");
			return ETHERNET_PHY_TIMEOUT;
		}
		
        /*delay_ms(1);*/
		taskDelay(1);
	} while ((reg&PHY_88E1116R_CR_RST) != 0);
	
    PHY_TRACE_OUT(FMSH_ENET_PHY_DEBUG, "PHY reset ok \r\n");

    /*
    wait link up
    */
    phy_timeout=0;	
    do
	{
		reg = mvl88e1116r_reg_read(pGmac,PAGE0,PHY_88E1116R_STAT);
		phy_timeout++;
		
		/*if (phy_timeout>_88E1116R_LINK_TIME_OUT)*/
		if (phy_timeout > 60)
		{
            PHY_TRACE_OUT(FMSH_ENET_PHY_DEBUG, "PHY link timeout \r\n");
			return ETHERNET_PHY_TIMEOUT;
		}
		
        /*delay_ms(1);		*/
		taskDelay(1);
	} while ((reg&PHY_88E1116R_STAT_LNK_STAT) == 0);
	
    PHY_TRACE_OUT(FMSH_ENET_PHY_DEBUG, "PHY link up \r\n");
    return ETHERNET_PHY_OK;
}

/**
* @brief: config & SW reset
* @para: FGmacPs_Instance_T * pGmac
* @return: u8 states
*/
u8 mvl88e1116r_cfg(FGmacPs_Instance_T * pGmac)
{
    FGmacPs_PhyConfig_T *pPhyConfig = pGmac->phy_cfg;
    u32 reg;
    /* auto MDI crossover */
    mvl88e1116r_MDIautoX(pGmac,3);
    /* AN and speed cfg */
    if (pPhyConfig->auto_nag_en==0){
        /* AN */
        reg=mvl88e1116r_reg_read(pGmac,PAGE0,PHY_88E1116R_CR);
        RESET_BIT(reg,PHY_88E1116R_CR_AN_EN);
        mvl88e1116r_reg_write(pGmac,PAGE0,PHY_88E1116R_CR,reg);
        /* speed */
        reg=mvl88e1116r_reg_read(pGmac,PAGE0,PHY_88E1116R_CR);
        RESET_BIT(reg,1<<13);
        RESET_BIT(reg,1<<6);
        if (pPhyConfig->speed==speed_10)
        {
            RESET_BIT(reg,1<<13);
            RESET_BIT(reg,1<<6);
        }
        if (pPhyConfig->speed==speed_100)
        {
            SET_BIT(reg,1<<13);
            RESET_BIT(reg,1<<6);
        }
        if (pPhyConfig->speed==speed_1000)
        {
            RESET_BIT(reg,1<<13);
            SET_BIT(reg,1<<6);
        }
        mvl88e1116r_reg_write(pGmac,PAGE0,PHY_88E1116R_CR,reg);
    }
    else{
        reg=mvl88e1116r_reg_read(pGmac,PAGE0,PHY_88E1116R_CR);
        SET_BIT(reg,PHY_88E1116R_CR_AN_EN);
        mvl88e1116r_reg_write(pGmac,PAGE0,PHY_88E1116R_CR,reg);
    }

    /* timing ctrl */
    mvl88e1116r_timing_ctrl(pGmac,1,1);
    /* sw reset */
    mvl88e1116r_reset(pGmac);
    return ETHERNET_PHY_OK;
}

/**
* @brief: get phy status through MDIO
* @para: FGmacPs_Instance_T *pGmac
* @return:
*/
/*u32 FMSH_gmacMDIO_get_phy_status(FGmacPs_Instance_T * pGmac)
{
	FGmacPs_PhyConfig_T *phy_cfg_p = pPhyConfig;
	u32 reg;
	reg=mvl88e1116r_reg_read(pGmac,PHY_88E1116R_STAT);
	phy_cfg_p->link_up=(reg&PHY_88E1116R_STAT_LNK_STAT)>>2;
	reg=mvl88e1116r_reg_read(pGmac,MDIO_PHY_SST);
	phy_cfg_p->speed=(reg&PHY_SST_SPEED)>>14;
	phy_cfg_p->is_duplex=(reg&PHY_SST_DUPLEX)>>13;
	reg=mvl88e1116r_reg_read(pGmac,PHY_88E1116R_ESTATST);
	phy_cfg_p->interface=(reg&PHY_ESST_HWCFG_MODE);
	return ETHERNET_PHY_OK;
}*/

/**
* @brief: setup timing control
* @para: FGmacPs_Instance_T * pGmac
* @para: u8 TxAddDelay, u8 RxAddDelay
*       if you use FMQL10/45 demo board, both should be 1
* @return: u8 states
*/
void mvl88e1116r_timing_ctrl(FGmacPs_Instance_T *pGmac,u8 TxAddDelay,u8 RxAddDelay)
{
    /* cfg */
    u32 reg;
    reg= mvl88e1116r_reg_read(pGmac,PAGE2,PHY_88E1116R_MACSC);
    if(TxAddDelay){
        SET_BIT(reg,PHY_88E1116R_MACSC_RGMII_TTC);
    }else{
        RESET_BIT(reg,PHY_88E1116R_MACSC_RGMII_TTC);
    }
    if(RxAddDelay==1){
        SET_BIT(reg,PHY_88E1116R_MACSC_RGMII_RTC);
    }else{
        RESET_BIT(reg,PHY_88E1116R_MACSC_RGMII_RTC);
    }
    mvl88e1116r_reg_write(pGmac,PAGE2,PHY_88E1116R_MACSC,reg);
}

/**
* @brief: setup PCS loopback
* @para: FGmacPs_Instance_T *pGmac
* @para: u8 enable
* @para: u8 speed, 0:10, 1:100, 2:1000
* @return: u32 states
*/
u32 mvl88e1116r_pcs_lpbk(FGmacPs_Instance_T *pGmac, u8 enable, u8 speed)
{
    u32 reg;
	
    if (enable == 1)
    {
#if 1
		reg = mvl88e1116r_reg_read(pGmac,0,PHY_88E1116R_CR);
		
		RESET_BIT(reg,PHY_88E1116R_CR_AN_EN);			/* disable AN*/
		SET_BIT(reg,PHY_88E1116R_CR_CP_DPLX_MOD);		/* set full duplex */
		mvl88e1116r_reg_write(pGmac,0,PHY_88E1116R_CR,reg);
		
        /* 
        set speed 
		*/
        reg = mvl88e1116r_reg_read(pGmac,2,PHY_88E1116R_MACSC);
		
        if (0 == speed)/* 10M */
        {
            RESET_BIT(reg,PHY_88E1116R_CR_SPEED_H);
            RESET_BIT(reg,PHY_88E1116R_CR_SPEED_L);
        }
		else if (1 == speed)/* 100M */
        {
            RESET_BIT(reg,PHY_88E1116R_CR_SPEED_H);
            SET_BIT(reg,PHY_88E1116R_CR_SPEED_L);
        }
		else if (2 == speed)/* 1000M */
        {
            RESET_BIT(reg,PHY_88E1116R_CR_SPEED_L);
            SET_BIT(reg,PHY_88E1116R_CR_SPEED_H);
        }
		
        mvl88e1116r_reg_write(pGmac,2,PHY_88E1116R_MACSC,reg);
		
        /* reset */
        mvl88e1116r_reset(pGmac);
#endif
		
        /* loopback */
        reg = mvl88e1116r_reg_read(pGmac,0,PHY_88E1116R_CR);
        SET_BIT(reg, PHY_88E1116R_CR_LOOPBK);
        mvl88e1116r_reg_write(pGmac,0,PHY_88E1116R_CR,reg);
    }
    else
    {
        reg=mvl88e1116r_reg_read(pGmac,0,PHY_88E1116R_CR);
        SET_BIT(reg,PHY_88E1116R_CR_AN_EN);			/* enable AN*/
        RESET_BIT(reg,PHY_88E1116R_CR_LOOPBK);
        mvl88e1116r_reg_write(pGmac,0,PHY_88E1116R_CR,reg);
		
        /* reset */
        mvl88e1116r_reset(pGmac);
    }
	
    return ETHERNET_PHY_OK;
}

/**
* @brief: setup line loopback
* @para: FGmacPs_Instance_T *pGmac, u8 enable
* @return: u32 states
*/
u32 mvl88e1116r_line_lpbk(FGmacPs_Instance_T *pGmac, u8 enable)
{
    u32 reg;
    if(enable==1)
    {
        /* loopback */
        reg=mvl88e1116r_reg_read(pGmac,2,PHY_88E1116R_MACSC);
        SET_BIT(reg,PHY_88E1116R_MACSC_LINE_LPBK);
        mvl88e1116r_reg_write(pGmac,2,PHY_88E1116R_MACSC,reg);
    }
    else
    {
        reg=mvl88e1116r_reg_read(pGmac,2,PHY_88E1116R_MACSC);
        RESET_BIT(reg,PHY_88E1116R_MACSC_LINE_LPBK);
        mvl88e1116r_reg_write(pGmac,2,PHY_88E1116R_MACSC,reg);
    }
    return ETHERNET_PHY_OK;
}

/*
//
// jc: reg 16, pgae-6, bit-5: 
//                           1-enable ext-loop, 0-disable ext-loop
//
*/
u32 mvl88e1116r_line_extloop(FGmacPs_Instance_T *pGmac, u8 enable)
{
    u32 reg;
	
    if (enable==1)
    {
        /* loopback */
        reg=mvl88e1116r_reg_read(pGmac,6,PHY_88E1116R_PGLLP);
        SET_BIT(reg, (1<<5));
        mvl88e1116r_reg_write(pGmac,6,PHY_88E1116R_PGLLP,reg);
    }
    else
    {
        reg=mvl88e1116r_reg_read(pGmac, 6, PHY_88E1116R_PGLLP);
        RESET_BIT(reg, (1<<5));
        mvl88e1116r_reg_write(pGmac, 6, PHY_88E1116R_PGLLP,reg);
    }
	
    return ETHERNET_PHY_OK;
}

