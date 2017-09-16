#ifndef _PROJECT_CONF_H
#define _PROJECT_CONF_H

#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC      stub_rdc_driver

#undef NETSTACK_CONF_MAC
#define NETSTACK_CONF_MAC      nullmac_driver

#undef NETSTACK_CONF_NETWORK
#define NETSTACK_CONF_NETWORK  stub_nwk_driver

#endif /* _PROJECT_CONF_H */