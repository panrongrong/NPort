#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include <lwip/stats.h>
#include <lwip/snmp.h>
#include "netif/etharp.h"
#include <string.h>
#include "netif.h"
#include "etharp.h"
#include "lwip/tcp.h"
#include "ip.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"

#ifndef __ETHERNET_INTERFACE_H__
#define __ETHERNET_INTERFACE_H__

void low_level_init(struct netif *netif);
err_t low_level_output(struct netif *netif, struct pbuf *p);
struct pbuf * low_level_input(struct netif *netif);
err_t ethernet_input(struct pbuf *p, struct netif *netif);
err_t ethernetif_input(struct netif *netif);
err_t ethernetif_init(struct netif *netif);
#endif