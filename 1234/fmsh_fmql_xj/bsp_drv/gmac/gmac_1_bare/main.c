/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file  fmsh_gmac_lib.c
*
* gmac driver
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
#include "lwip_demo.h"

#include "fmsh_common.h"
#include "ethernet_interface.h"
#include "cache.h"
#include "ps_init.h"
#include "lwiperf.h"
#include "cache.h"
/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/
void mmu_setup(void);
#if (1==RCV_POLL)
    err_t ethernetif_input_poll(struct netif *netif);
#else
    err_t ethernetif_input(struct netif *netif);
#endif
/************************** Variable Definitions *****************************/
struct netif Gmac_LwIP_if;
extern u32 g_rcv_cnt;
/************************** Local Function ***********************************/

/************************** Extern Function **********************************/
void main(void)
{
    /* cache & mmu */
    icache_enable();
	
#if PSOC_CACHE_ENABLE
    dcache_enable();
#else
    dcache_disable();
    mmu_setup();
#endif

    /* ps init */
    ps_init();

    /*return ;*/
    /* enable timer */
    global_timer_enable();

    /* lwip init */
	static ip4_addr_t ipaddr,netmask,gw;
	
    lwip_init();
	
	IP4_ADDR(&ipaddr,192,168,0,50);
	IP4_ADDR(&netmask,255,255,255,0);
	IP4_ADDR(&gw,192,168,1,1);
	
	netif_add(&Gmac_LwIP_if,&ipaddr,&netmask,&gw,NULL,ethernetif_init,ethernet_input);
	netif_set_default(&Gmac_LwIP_if);
	
	netif_set_up(&Gmac_LwIP_if);

    lwiperf_start_tcp_server(&ipaddr, 5001, NULL, NULL);
	
	while(1)
	{
#if (1==RCV_POLL)
        ethernetif_input_poll(&Gmac_LwIP_if);
#else
        if(g_rcv_cnt>0) {
            ethernetif_input(&Gmac_LwIP_if);
        }
#endif
		sys_check_timeouts();
	}
}