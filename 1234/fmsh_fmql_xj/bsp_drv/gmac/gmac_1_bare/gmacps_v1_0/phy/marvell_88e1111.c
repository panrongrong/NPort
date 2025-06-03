/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file  marvell_88e1111.c
*
* marvell ethernet phy driver
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
#include "marvell_88e1111.h"
/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

/************************** Variable Definitions *****************************/

/************************** Function *****************************************/
u32 mvl88e1111_reg_write(FGmacPs_Instance_T * pGmac,u8 page,u8 regAddr,u32 regdata)
{
	FGmacPs_MacPortMap_T *pGmacPortMap = pGmac->base_address;
    FGmacPs_PhyConfig_T *pPhyConfig = pGmac->phy_cfg;

    /* wait idle */
    if(fmsh_mdio_idle(pGmac)!=ETHERNET_PHY_OK){
        return ETHERNET_PHY_ERR;
    }

    /* write page */
    u32 reg=0;
    reg = (pPhyConfig->mdio_address << 11)|(pGmac->csr_clk << 2);
    /*reg &= ~GMAC_GAR_GR;  //clear GR*/
    reg |= (PHY_88E1111_PAGE_ADDR<<6);
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

u32 mvl88e1111_reg_read(FGmacPs_Instance_T * pGmac,u8 page,u8 regAddr)
{
	FGmacPs_MacPortMap_T *pGmacPortMap = pGmac->base_address;
    FGmacPs_PhyConfig_T *pPhyConfig = pGmac->phy_cfg;
	u32 reg=0;
	u32 datareg;

    /* wait idle */
    if(fmsh_mdio_idle(pGmac)!=ETHERNET_PHY_OK){
        return ETHERNET_PHY_ERR;
    }

    /* write page */
    reg = (pPhyConfig->mdio_address << 11)|(pGmac->csr_clk << 2);
    /*reg &= ~GMAC_GAR_GR;*/
    reg |= (PHY_88E1111_PAGE_ADDR<<6);
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

u8 mvl88e1111_detect(FGmacPs_Instance_T * pGmac)
{
    FGmacPs_PhyConfig_T *pPhyConfig = pGmac->phy_cfg;
	u32 reg;
    u8 index=0;

    for(index=0;index<32;index++){
        pPhyConfig->mdio_address=index;
        reg = mvl88e1111_reg_read(pGmac,PAGE0,PHY_88E1111_ID1);
        if(reg==MVL88E_ID){
			reg = mvl88e1111_reg_read(pGmac,PAGE0,PHY_88E1111_ID2);
			if ((reg & PHY_88E1111_ID2_MOD_MUN_MASK)==PHY_88E1111_ID2_MOD_MUN){
				PHY_TRACE_OUT(FMSH_ENET_PHY_DEBUG, "MVL88E1111 detect 0x%x \r\n",pPhyConfig->mdio_address);
				return ETHERNET_PHY_OK;
			}
        }
    }
    PHY_TRACE_OUT(FMSH_ENET_PHY_DEBUG, "PHY detect fail \r\n");
    pPhyConfig->mdio_address=32;
    return ETHERNET_PHY_ERR;
}

u8 mvl88e1111_init(FGmacPs_Instance_T * pGmac)
{
    FGmacPs_PhyConfig_T *pPhyConfig = pGmac->phy_cfg;
	u32 reg;
	u32 phy_timeout;
    u8 ret;

    if(pPhyConfig->auto_detect_ad_en==1){
        ret=mvl88e1111_detect(pGmac);
        if(ret!=ETHERNET_PHY_OK){
            return ETHERNET_PHY_ERR;
        }
    } else {
	    /* read ID */
	    phy_timeout=0;
	    do{
	        reg = mvl88e1111_reg_read(pGmac,PAGE0,PHY_88E1111_ID2);
	        /*reg=mic_phy_read(pGmac,2);*/
	        phy_timeout++;
	        if(phy_timeout>_88E1111_TIME_OUT)
	        {
	            PHY_TRACE_OUT(FMSH_ENET_PHY_DEBUG, "PHY ID read timeout \r\n");
	            return ETHERNET_PHY_TIMEOUT;
	        }
	        delay_ms(1);
	    }while((reg & PHY_88E1111_ID2_MOD_MUN_MASK) != PHY_88E1111_ID2_MOD_MUN);
	    PHY_TRACE_OUT(FMSH_ENET_PHY_DEBUG, "88e1116r ID get \r\n");
    }
    return ETHERNET_PHY_OK;
}


u8 mvl88e1111_reg_dump(FGmacPs_Instance_T * pGmac)
{
    u8 i=0;
    u16 reg;
    printf("88e1111 reg:\r\n");
    for(i=0;i<32;i++){
        reg=mvl88e1111_reg_read(pGmac,PAGE0,i);
        printf("reg%d : %x\r\n",i,reg);
    }
    return ETHERNET_PHY_OK;
}

void mvl88e1111_MDIautoX(FGmacPs_Instance_T * pGmac, u8 mode)
{
    u16 reg;
    reg = mvl88e1111_reg_read(pGmac,PAGE0,PHY_88E1111_SCT);
    RESET_BIT(reg,PHY_88E1111_SCT_MDIX);
    reg|=(mode<<5);
    mvl88e1111_reg_write(pGmac,PAGE0,PHY_88E1111_SCT,reg);
}

u8 mvl88e1111_restart_AN(FGmacPs_Instance_T * pGmac)
{
    u16 reg;
    u32 phy_timeout;
	/* AN enable */
    reg=mvl88e1111_reg_read(pGmac,PAGE0,PHY_88E1111_CTRL);
    SET_BIT(reg,PHY_88E1111_CTRL_AN_EN);
    mvl88e1111_reg_write(pGmac,PAGE0,PHY_88E1111_CTRL,reg);
	/*  restart */
    reg=mvl88e1111_reg_read(pGmac,PAGE0,PHY_88E1111_CTRL);
    SET_BIT(reg,PHY_88E1111_CTRL_RST_CP_AN);
    mvl88e1111_reg_write(pGmac,PAGE0,PHY_88E1111_CTRL,reg);
    /* wait AN done */
    phy_timeout=0;
    do{
        reg = mvl88e1111_reg_read(pGmac,PAGE0,PHY_88E1111_STAT);
        phy_timeout++;
        if(phy_timeout>_88E1111_TIME_OUT)
        {
            PHY_TRACE_OUT(FMSH_ENET_PHY_DEBUG, "PHY ang timeout \r\n");
            return ETHERNET_PHY_TIMEOUT;
        }
        delay_ms(1);
    }while((reg&PHY_88E1111_STAT_ANF) == 0);
    return ETHERNET_PHY_OK;
}

u8 mvl88e1111_reset(FGmacPs_Instance_T * pGmac)
{
    u16 reg;
    u32 phy_timeout=0;
    reg=mvl88e1111_reg_read(pGmac,PAGE0,PHY_88E1111_CTRL);
    SET_BIT(reg,PHY_88E1111_CTRL_RST);
    mvl88e1111_reg_write(pGmac,PAGE0,PHY_88E1111_CTRL,reg);
    /* wait reset done */
    phy_timeout=0;
    do{
		reg=mvl88e1111_reg_read(pGmac,PAGE0,PHY_88E1111_CTRL);
		phy_timeout++;
		if(phy_timeout>_88E1111_TIME_OUT){
            PHY_TRACE_OUT(FMSH_ENET_PHY_DEBUG, "PHY reset timeout \r\n");
			return ETHERNET_PHY_TIMEOUT;
		}
        delay_ms(1);
	}while((reg&PHY_88E1111_CTRL_RST) != 0);
    PHY_TRACE_OUT(FMSH_ENET_PHY_DEBUG, "PHY reset ok \r\n");

    /* wait link up */
    phy_timeout=0;
    do{
		reg = mvl88e1111_reg_read(pGmac,PAGE0,PHY_88E1111_STAT);
		phy_timeout++;
		if(phy_timeout>_88E1111_LINK_TIME_OUT){
            PHY_TRACE_OUT(FMSH_ENET_PHY_DEBUG, "PHY link timeout \r\n");
			return ETHERNET_PHY_TIMEOUT;
		}
        delay_ms(1);
	}while((reg&PHY_88E1111_STAT_LNK_STAT) == 0);
    PHY_TRACE_OUT(FMSH_ENET_PHY_DEBUG, "PHY link up \r\n");
    return ETHERNET_PHY_OK;
}

u8 mvl88e1111_cfg(FGmacPs_Instance_T * pGmac)
{
    FGmacPs_PhyConfig_T *pPhyConfig = pGmac->phy_cfg;
    u32 reg;
    /* auto MDI crossover */
    mvl88e1111_MDIautoX(pGmac,3);
    /* AN and speed cfg */
    if (pPhyConfig->auto_nag_en==0){
        /* AN disable */
        reg=mvl88e1111_reg_read(pGmac,PAGE0,PHY_88E1111_CTRL);
        RESET_BIT(reg,PHY_88E1111_CTRL_AN_EN);
        mvl88e1111_reg_write(pGmac,PAGE0,PHY_88E1111_CTRL,reg);
        /* speed */
        reg=mvl88e1111_reg_read(pGmac,PAGE0,PHY_88E1111_CTRL);
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
        mvl88e1111_reg_write(pGmac,PAGE0,PHY_88E1111_CTRL,reg);
    }
    else{
        reg=mvl88e1111_reg_read(pGmac,PAGE0,PHY_88E1111_CTRL);
        SET_BIT(reg,PHY_88E1111_CTRL_AN_EN);
        mvl88e1111_reg_write(pGmac,PAGE0,PHY_88E1111_CTRL,reg);
    }

    /* timing ctrl */
    mvl88e1111_timing_ctrl(pGmac,1,1);
    /* sw reset */
    mvl88e1111_reset(pGmac);
    return ETHERNET_PHY_OK;
}

void mvl88e1111_timing_ctrl(FGmacPs_Instance_T *pGmac,u8 TxAddDelay,u8 RxAddDelay)
{
    /* cfg */
    u32 reg;
    reg= mvl88e1111_reg_read(pGmac,PAGE2,PHY_88E1111_ESCT);
    if(TxAddDelay){
        SET_BIT(reg,PHY_88E1111_ESCR_RGMII_TTC);
    }else{
        RESET_BIT(reg,PHY_88E1111_ESCR_RGMII_TTC);
    }
    if(RxAddDelay==1){
        SET_BIT(reg,PHY_88E1111_ESCR_RGMII_RTC);
    }else{
        RESET_BIT(reg,PHY_88E1111_ESCR_RGMII_RTC);
    }
    mvl88e1111_reg_write(pGmac,PAGE2,PHY_88E1111_ESCT,reg);
}

/**/
/*/**//*/
//u8 mvl_phy_reset(FGmacPs_Instance_T * pGmac)
//{
//    u32 reg;
//    u32 phy_timeout;
//    reg=mvl88e_reg_read(pGmac,0,0);
//    SET_BIT(reg,1<<15);
//    mvl88e_reg_write(pGmac,0,0,reg);
//    /* reset */
/*    phy_timeout=0;*/
/*    do{*/
/*		reg = mic_phy_read(pGmac,0);*/
/*		phy_timeout++;*/
/*		if(phy_timeout>PHY_TIME_OUT)*/
/*		{*/
/*            PHY_TRACE_OUT(PHY_DEBUG_OUT, "PHY reset timeout \r\n");*/
/*			return ETHERNET_PHY_TIMEOUT;*/
/*		}*/
/*        delay_ms(10);*/
/*	}while((reg&(1<<15)) != 0);*/
/**/
/*    /* auto X */
/*    mic_phy_write(pGmac,0x1c,0);*/
/*     cfg */
/**/
/*    if (pGmac->phy_cfg->auto_nag_en==0)*/
/*    {*/
/*        // disable ang*/
/*        reg = mic_phy_read(pGmac,0);*/
/*        RESET_BIT(reg,1<<12);*/
/*        mic_phy_write(pGmac,0,reg);*/
/*        if (pGmac->phy_cfg->speed==speed_10)*/
/*        {*/
/*            reg = mic_phy_read(pGmac,0);*/
/*            RESET_BIT(reg,1<<13);*/
/*            RESET_BIT(reg,1<<6);*/
/*            mic_phy_write(pGmac,0,reg);*/
/**/
/*            reg = mic_phy_read(pGmac,9);*/
/*            RESET_BIT(reg,1<<8);*/
/*            RESET_BIT(reg,1<<9);*/
/*            mic_phy_write(pGmac,9,reg);*/
/*        }*/
/*        if (pGmac->phy_cfg->speed==speed_100)*/
/*        {*/
/*            reg = mic_phy_read(pGmac,0);*/
/*            SET_BIT(reg,1<<13);*/
/*            RESET_BIT(reg,1<<6);*/
/*            mic_phy_write(pGmac,0,reg);*/
/**/
/*            reg = mic_phy_read(pGmac,9);*/
/*            RESET_BIT(reg,1<<8);*/
/*            RESET_BIT(reg,1<<9);*/
/*            mic_phy_write(pGmac,9,reg);*/
/*        }*/
/*        if (pGmac->phy_cfg->speed==speed_1000)*/
/*        {*/
/*            reg = mic_phy_read(pGmac,9);*/
/*            SET_BIT(reg,1<<8);*/
/*            SET_BIT(reg,1<<9);*/
/*            mic_phy_write(pGmac,9,reg);*/
/*        }*/
/*    }*/
/*    else*/
/*    {*/
/*        reg = mic_phy_read(pGmac,0);*/
/*        SET_BIT(reg,1<<12);*/
/*        mic_phy_write(pGmac,0,reg);*/
/*    }*/
/*    /* ang */
/*    reg=mic_phy_read(pGmac,0);*/
/*    SET_BIT(reg,1<<12);*/
/*    mic_phy_write(pGmac,0,reg);*/
/**/
/*    reg=mic_phy_read(pGmac,0);*/
/*    SET_BIT(reg,1<<9);*/
/*    mic_phy_write(pGmac,0,reg);*/
/*     wait ang */
/*    phy_timeout=0;*/
/*    do{*/
/*        reg = mic_phy_read(pGmac,1);*/
/*        phy_timeout++;*/
/*        if(phy_timeout>PHY_TIME_OUT)*/
/*        {*/
/*            PHY_TRACE_OUT(PHY_DEBUG_OUT, "PHY ang timeout \r\n");*/
/*            return ETHERNET_PHY_TIMEOUT;*/
/*        }*/
/*        delay_ms(10);*/
/*    }while((reg&(1<<5)) == 0);*/
/*    /* wait link up */
/*    phy_timeout=0;*/
/*    do{*/
/*		reg = mic_phy_read(pGmac,1);*/
/*		phy_timeout++;*/
/*		if(phy_timeout>PHY_TIME_OUT)*/
/*		{*/
/*            PHY_TRACE_OUT(PHY_DEBUG_OUT, "PHY link timeout \r\n");*/
/*			return ETHERNET_PHY_TIMEOUT;*/
/*		}*/
/*        delay_ms(10);*/
/*	}while((reg&(1<<2)) == 0);*/
/* return ETHERNET_PHY_OK;*/
/*}*/
/**/
/***@brief: get phy status through MDIO*/
/**@para: FGmacPs_Instance_T *Gmac*/
/**@return:*/
/**/
/*/*u32 FMSH_gmacMDIO_get_phy_status(FGmacPs_Instance_T *pGmac)*/
/*{*/
/*	FGmacPs_PhyConfig_T *hy_cfg_p = pGmac->phy_cfg;*/
/*	u32 reg;*/
/*	reg=mvl88e_reg_read(pGmac,MDIO_PHY_STAT);*/
/*	phy_cfg_p->link_up=(reg&PHY_STAT_LNK_STAT)>>2;*/
/*	reg=mvl88e_reg_read(pGmac,MDIO_PHY_SST);*/
/*	phy_cfg_p->speed=(reg&PHY_SST_SPEED)>>14;*/
/*	phy_cfg_p->is_duplex=(reg&PHY_SST_DUPLEX)>>13;*/
/*	reg=mvl88e_reg_read(pGmac,MDIO_PHY_ESST);*/
/*	phy_cfg_p->interface=(reg&PHY_ESST_HWCFG_MODE);*/
/*	return ETHERNET_PHY_OK;*/
/*}*/
/*u32 mvl88e1111_timing_ctrl(FGmacPs_Instance_T *Gmac,u8 TxAddDelay,u8 RxAddDelay){*/
/*    /* cfg */
/*    u32 reg;*/
/*    reg= mvl88e_reg_read(pGmac,2,MDIO_PHY_CR);*/
/*    if(TxAddDelay){*/
/*        SET_BIT(reg,PHY_CR_RGMII_TTC);*/
/*    }else{*/
/*        RESET_BIT(reg,PHY_CR_RGMII_TTC);*/
/*    }*/
/*    if(RxAddDelay==1){*/
/*        SET_BIT(reg,PHY_CR_RGMII_RTC);*/
/*    }else{*/
/*        RESET_BIT(reg,PHY_CR_RGMII_RTC);*/
/*    }*/
/*    mvl88e_reg_write(pGmac,2,MDIO_PHY_CR,reg);*/
/*     sw reset */
/*    reg= mvl88e_reg_read(pGmac,0,MDIO_PHY_CTRL);*/
/*    SET_BIT(reg,PHY_CTRL_RST);*/
/*    mvl88e_reg_write(pGmac,0,MDIO_PHY_CTRL,reg);*/
/*    /*wait reset done */
/*    u32 phy_timeout=0;*/
/*    do{*/
/*        reg=mvl88e_reg_read(pGmac,0,MDIO_PHY_CTRL);*/
/*        phy_timeout++;*/
/*        if(phy_timeout>PHY_TIME_OUT)*/
/*        {*/
/*            PHY_TRACE_OUT(PHY_DEBUG_OUT, "phy reset timeout \r\n");*/
/*            return ETHERNET_PHY_TIMEOUT;*/
/*        }*/
/*    }while((reg&PHY_CTRL_RST)!=0);			wait reset done */
/*    PHY_TRACE_OUT(PHY_DEBUG_OUT, "MDIO_PHY_CR:0x%x \r\n",mvl88e_reg_read(pGmac,2,MDIO_PHY_CR));*/
/*    return ETHERNET_PHY_OK;*/
/*}*/
/**/
/*/**//*@brief: setup PCS loopback*/
/**@para: FGmacPs_Instance_T *Gmac, u8 enable*/
/**@return:*/
/**/
/*u32 mvl88e_pcs_lpbk(FGmacPs_Instance_T *Gmac, u8 enable)*/
/*{*/
/*    u32 reg;*/
/*    u32 phy_timeout;*/
/*    if(enable==1)*/
/*    {*/
/*        /* 100M */
/*        reg= mvl88e_reg_read(pGmac,0,MDIO_PHY_CTRL);*/
/*		RESET_BIT(reg,PHY_CTRL_AN_EN);			 disable AN*/
/*		SET_BIT(reg,PHY_CTRL_CP_DPLX_MOD);		/* set full duplex */
/*		RESET_BIT(reg,PHY_CTRL_SPEED_H);		 set speed */
/*		SET_BIT(reg,PHY_CTRL_SPEED_L);*/
/*		mvl88e_reg_write(pGmac,0,MDIO_PHY_CTRL,reg);*/
/*        /* reset */
/*        reg= mvl88e_reg_read(pGmac,0,MDIO_PHY_CTRL);*/
/*		SET_BIT(reg,PHY_CTRL_RST);*/
/*		mvl88e_reg_write(pGmac,0,MDIO_PHY_CTRL,reg);*/
/*		wait reset done */
/*		phy_timeout=0;*/
/*		do{*/
/*			reg=mvl88e_reg_read(pGmac,0,MDIO_PHY_CTRL);*/
/*			phy_timeout++;*/
/*			if(phy_timeout>PHY_TIME_OUT)*/
/*			{*/
/*                PHY_TRACE_OUT(PHY_DEBUG_OUT, "phy reset timeout \r\n");*/
/*				return ETHERNET_PHY_TIMEOUT;*/
/*			}*/
/*		}while((reg&PHY_CTRL_RST)!=0);			/*wait reset done */
/*         loopback */
/*        reg=mvl88e_reg_read(pGmac,0,MDIO_PHY_CTRL);*/
/*        SET_BIT(reg,PHY_CTRL_LOOPBK);*/
/*        mvl88e_reg_write(pGmac,0,MDIO_PHY_CTRL,reg);*/
/*    }*/
/*    else*/
/*    {*/
/*        reg=mvl88e_reg_read(pGmac,0,MDIO_PHY_CTRL);*/
/*        RESET_BIT(reg,PHY_CTRL_LOOPBK);*/
/*        mvl88e_reg_write(pGmac,0,MDIO_PHY_CTRL,reg);*/
/*    }*/
/*    return ETHERNET_PHY_OK;*/
/*}*/
/**/
/*/**//*@brief: setup line loopback*/
/**@para: FGmacPs_Instance_T *Gmac, u8 enable*/
/**@return:*/
/**/
/*u32 mvl88e_line_lpbk(FGmacPs_Instance_T *Gmac, u8 enable)*/
/*{*/
/*    u32 reg;*/
/*    if(enable==1)*/
/*    {*/
/*        /* loopback */
/*        reg=mvl88e_reg_read(pGmac,2,MDIO_PHY_CR);*/
/*        SET_BIT(reg,PHY_CR_LINE_LPBK);*/
/*        mvl88e_reg_write(pGmac,2,MDIO_PHY_CR,reg);*/
/*    }*/
/*    else*/
/*    {*/
/*        reg=mvl88e_reg_read(pGmac,2,MDIO_PHY_CR);*/
/*        RESET_BIT(reg,PHY_CR_LINE_LPBK);*/
/*        mvl88e_reg_write(pGmac,2,MDIO_PHY_CR,reg);*/
/*    }*/
/*    return ETHERNET_PHY_OK;*/
/*}*/
