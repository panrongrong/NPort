/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file  microchip_ksz9031RNX.c
*
* microchip ksz9031RNX phy driver
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 0.01   Hansen Yang  4/24/2019  First Release
*</pre>
******************************************************************************/
/***************************** Include Files *********************************/
#include "microchip_ksz9031RNX.h"
#include "fmsh_mdio.h"
/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

/************************** Variable Definitions *****************************/

/************************** Function *****************************************/
u16 mic9031RNX_read_MMD(FGmacPs_Instance_T * pGmac,u16 ad,u16 reg)
{
    u16 ret;
	fmsh_mdio_write(pGmac,KSZ9031RNX_MMDC, ad);
	fmsh_mdio_write(pGmac,KSZ9031RNX_MMDD, reg);
	fmsh_mdio_write(pGmac,KSZ9031RNX_MMDC, 0x4000+ad);
	ret=fmsh_mdio_read(pGmac,KSZ9031RNX_MMDD);
    return ret;
}

u8 mic9031RNX_write_MMD(FGmacPs_Instance_T * pGmac,u16 ad,u16 reg,u16 data)
{
    fmsh_mdio_write(pGmac,KSZ9031RNX_MMDC, ad);
	fmsh_mdio_write(pGmac,KSZ9031RNX_MMDD, reg);
	fmsh_mdio_write(pGmac,KSZ9031RNX_MMDC, 0x4000+ad);
	fmsh_mdio_write(pGmac,KSZ9031RNX_MMDD, data);
    return ETHERNET_PHY_OK;
}

u8 mic9031RNX_clk_pad_skew(FGmacPs_Instance_T * pGmac,u16 tx,u16 rx)
{
    mic9031RNX_write_MMD(pGmac,2,KSZ9031RNX_CLK_PAD_SKEW,(tx<<5)|(rx));
    return ETHERNET_PHY_OK;
}

u8 mic9031RNX_ctrl_pad_skew(FGmacPs_Instance_T * pGmac,u16 rx,u16 tx)
{
    mic9031RNX_write_MMD(pGmac,2,KSZ9031RNX_CONTROL_PAD_SKEW,(rx<<4)|(tx));
    return ETHERNET_PHY_OK;
}

u8 mic9031RNX_tx_data_pad_skew(FGmacPs_Instance_T * pGmac,u16 data3, u16 data2, u16 data1, u16 data0)
{
    mic9031RNX_write_MMD(pGmac,2,KSZ9031RNX_TX_DATA_PAD_SKEW,data0|(data1<<4)|(data2<<8)|(data3<<12));
    return ETHERNET_PHY_OK;
}

u8 mic9031RNX_rx_data_pad_skew(FGmacPs_Instance_T * pGmac,u16 data3, u16 data2, u16 data1, u16 data0)
{
    mic9031RNX_write_MMD(pGmac,2,KSZ9031RNX_RX_DATA_PAD_SKEW,data0|(data1<<4)|(data2<<8)|(data3<<12));
    return ETHERNET_PHY_OK;
}

u8 mic9031RNX_init(FGmacPs_Instance_T * pGmac)
{
	fmsh_mdio_init(pGmac);
    return ETHERNET_PHY_OK;
}

void mic9031RNX_MDIautoX(FGmacPs_Instance_T * pGmac,u8 enable,u8 MDI_set)
{
    u16 reg;
    reg = fmsh_mdio_read(pGmac,KSZ9031RNX_AMDIX);
    if(enable==1){
        RESET_BIT(reg,KSZ9031RNX_AMDIX_SW_OFF);
    }else{
        SET_BIT(reg,KSZ9031RNX_AMDIX_SW_OFF);
        if(MDI_set==1){
            SET_BIT(reg,KSZ9031RNX_AMDIX_MDI_SET);
        }else{
            RESET_BIT(reg,KSZ9031RNX_AMDIX_MDI_SET);
        }
    }
    fmsh_mdio_write(pGmac,KSZ9031RNX_AMDIX,reg);
}

void mic9031RNX_AN_cfg(FGmacPs_Instance_T * pGmac,u8 enable)
{
    u16 reg;
    reg=fmsh_mdio_read(pGmac,KSZ9031RNX_CTRL);
    if(enable){
        SET_BIT(reg,KSZ9031RNX_CTRL_AN_EN);
    }else{
        RESET_BIT(reg,KSZ9031RNX_CTRL_AN_EN);
    }
    fmsh_mdio_write(pGmac,KSZ9031RNX_CTRL,reg);
}

u8 mic9031RNX_cfg(FGmacPs_Instance_T * pGmac)
{
    FGmacPs_PhyConfig_T *pPhyConfig = pGmac->phy_cfg;
    u32 reg;
    /* MDI crossover */
    mic9031RNX_MDIautoX(pGmac,1,0);
    /* AN and speed cfg */
    if(pPhyConfig->speed==speed_1000){
        /* open AN */
        mic9031RNX_AN_cfg(pGmac,1);
        /* speed */
        reg=fmsh_mdio_read(pGmac,KSZ9031RNX_CTRL);
        RESET_BIT(reg,KSZ9031RNX_CTRL_SPEED_L);
        RESET_BIT(reg,KSZ9031RNX_CTRL_SPEED_H);
        RESET_BIT(reg,KSZ9031RNX_CTRL_SPEED_L);
        SET_BIT(reg,KSZ9031RNX_CTRL_SPEED_H);
        fmsh_mdio_write(pGmac,KSZ9031RNX_CTRL,reg);
    }else if(pPhyConfig->auto_nag_en==0){
        /* AN */
        mic9031RNX_AN_cfg(pGmac,0);
        /* speed */
        reg=fmsh_mdio_read(pGmac,KSZ9031RNX_CTRL);
        RESET_BIT(reg,KSZ9031RNX_CTRL_SPEED_L);
        RESET_BIT(reg,KSZ9031RNX_CTRL_SPEED_H);
        if (pPhyConfig->speed==speed_10){
            RESET_BIT(reg,KSZ9031RNX_CTRL_SPEED_L);
            RESET_BIT(reg,KSZ9031RNX_CTRL_SPEED_H);
        }else if (pPhyConfig->speed==speed_100){
            SET_BIT(reg,KSZ9031RNX_CTRL_SPEED_L);
            RESET_BIT(reg,KSZ9031RNX_CTRL_SPEED_H);
        }
        fmsh_mdio_write(pGmac,KSZ9031RNX_CTRL,reg);
    }else{
        mic9031RNX_AN_cfg(pGmac,1);
    }
    /* timing ctrl */
    u16 s_tx_clk_skew=0x1f;
    u16 s_rx_clk_skew=0x1f;
    u16 s_tx_data_skew=0x0;
    u16 s_rx_data_skew=0x0;
    mic9031RNX_clk_pad_skew(pGmac,s_tx_clk_skew,s_rx_clk_skew);
    mic9031RNX_ctrl_pad_skew(pGmac,s_rx_data_skew,s_tx_data_skew);
    mic9031RNX_tx_data_pad_skew(pGmac,s_tx_data_skew,s_tx_data_skew,s_tx_data_skew,s_tx_data_skew); /*only 9031 RNX*/
    mic9031RNX_rx_data_pad_skew(pGmac,s_rx_data_skew,s_rx_data_skew,s_rx_data_skew,s_rx_data_skew); /*only 9031 RNX*/


    /* fix bug */
    fmsh_mdio_write(pGmac,0xd,0);
    fmsh_mdio_write(pGmac,0xe,4);
    fmsh_mdio_write(pGmac,0xd,0x4000);
    fmsh_mdio_write(pGmac,0xe,6);
    fmsh_mdio_write(pGmac,0xd,0);
    fmsh_mdio_write(pGmac,0xe,3);
    fmsh_mdio_write(pGmac,0xd,0x4000);
    fmsh_mdio_write(pGmac,0xe,0x1a80);

    reg = fmsh_mdio_read(pGmac,KSZ9031RNX_CTRL);
    reg |= (1<<9);
    fmsh_mdio_write(pGmac,KSZ9031RNX_CTRL,reg);

    /* wait link up */
    u32 phy_timeout=0;
    do{
		reg = fmsh_mdio_read(pGmac,ENET_PHY_STS);
		phy_timeout++;
		if(phy_timeout>FMSH_ENET_PHY_LINK_TIMEOUT){
            PHY_TRACE_OUT(FMSH_ENET_PHY_DEBUG, "PHY link timeout \r\n");
			return ETHERNET_PHY_TIMEOUT;
		}
        delay_ms(1);
	}while((reg&ENET_PHY_STS_LNK_STAT) == 0);
    PHY_TRACE_OUT(FMSH_ENET_PHY_DEBUG, "PHY link up \r\n");

    return ETHERNET_PHY_OK;
}

u8 mic9031RNX_reset(FGmacPs_Instance_T * pGmac)
{
    fmsh_mdio_reset(pGmac);
    return ETHERNET_PHY_OK;
}

u8 mic9031RNX_reg_dump(FGmacPs_Instance_T * pGmac)
{
    fmsh_mdio_reg_dump(pGmac);
    return ETHERNET_PHY_OK;
}
/* local loopback */
u8 mic9031RNX_digital_loopback(FGmacPs_Instance_T * pGmac)
{
    FGmacPs_PhyConfig_T *pPhyConfig = pGmac->phy_cfg;
    u32 reg;

    if(pPhyConfig->speed==speed_1000){
        reg=fmsh_mdio_read(pGmac,KSZ9031RNX_CTRL);
        SET_BIT(reg,KSZ9031RNX_CTRL_LOOPBK);
        SET_BIT(reg,KSZ9031RNX_CTRL_SPEED_H);
        RESET_BIT(reg,KSZ9031RNX_CTRL_SPEED_L);
        RESET_BIT(reg,KSZ9031RNX_CTRL_AN_EN);
        SET_BIT(reg,KSZ9031RNX_CTRL_CP_DPLX_MOD);
        fmsh_mdio_write(pGmac,KSZ9031RNX_CTRL,reg);

        reg=fmsh_mdio_read(pGmac,KSZ9031RNX_1000CT);
        SET_BIT(reg,KSZ9031RNX_1000CT_MSMCE);
        RESET_BIT(reg,KSZ9031RNX_1000CT_MSMCV);
        fmsh_mdio_write(pGmac,KSZ9031RNX_1000CT,reg);
    }else if(pPhyConfig->speed==speed_100){
        reg=fmsh_mdio_read(pGmac,KSZ9031RNX_CTRL);
        SET_BIT(reg,KSZ9031RNX_CTRL_LOOPBK);
        RESET_BIT(reg,KSZ9031RNX_CTRL_SPEED_H);
        SET_BIT(reg,KSZ9031RNX_CTRL_SPEED_L);
        RESET_BIT(reg,KSZ9031RNX_CTRL_AN_EN);
        SET_BIT(reg,KSZ9031RNX_CTRL_CP_DPLX_MOD);
        fmsh_mdio_write(pGmac,KSZ9031RNX_CTRL,reg);
    }else{
        reg=fmsh_mdio_read(pGmac,KSZ9031RNX_CTRL);
        SET_BIT(reg,KSZ9031RNX_CTRL_LOOPBK);
        RESET_BIT(reg,KSZ9031RNX_CTRL_SPEED_H);
        RESET_BIT(reg,KSZ9031RNX_CTRL_SPEED_L);
        RESET_BIT(reg,KSZ9031RNX_CTRL_AN_EN);
        SET_BIT(reg,KSZ9031RNX_CTRL_CP_DPLX_MOD);
        fmsh_mdio_write(pGmac,KSZ9031RNX_CTRL,reg);
    }
    return ETHERNET_PHY_OK;
}
/* remote */
u8 mic9031RNX_analog_loopback(FGmacPs_Instance_T * pGmac)
{
    u32 reg;
    reg=fmsh_mdio_read(pGmac,KSZ9031RNX_CTRL);
    SET_BIT(reg,KSZ9031RNX_CTRL_SPEED_H);
    RESET_BIT(reg,KSZ9031RNX_CTRL_SPEED_L);
    RESET_BIT(reg,KSZ9031RNX_CTRL_AN_EN);
    SET_BIT(reg,KSZ9031RNX_CTRL_CP_DPLX_MOD);
    fmsh_mdio_write(pGmac,KSZ9031RNX_CTRL,reg);

    reg=fmsh_mdio_read(pGmac,KSZ9031RNX_RMLP);
    SET_BIT(reg,KSZ9031RNX_RMLP_RMLPEN);
    fmsh_mdio_write(pGmac,KSZ9031RNX_RMLP,reg);
    return ETHERNET_PHY_OK;
}