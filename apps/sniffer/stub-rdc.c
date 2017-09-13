#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/mac/mac.h"
#include "net/mac/rdc.h"
/*---------------------------------------------------------------------------*/
static void
send(mac_callback_t sent, void *ptr)
{
  int ret = 0;

  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &rimeaddr_node_addr);
  if(NETSTACK_FRAMER.create() < 0) {
    ret = MAC_TX_ERR_FATAL;
  } else {
#ifdef NETSTACK_ENCRYPT
    NETSTACK_ENCRYPT();
#endif /* NETSTACK_ENCRYPT */
    ret = NETSTACK_RADIO.send(packetbuf_hdrptr(), packetbuf_totlen());
  }

  if(sent) {
    sent(ptr, MAC_TX_OK, ret);
  }
}
/*---------------------------------------------------------------------------*/
static void
send_list(mac_callback_t sent, void *ptr, struct rdc_buf_list *list)
{
  if(sent) {
    sent(ptr, MAC_TX_OK, 1);
  }
}
/*---------------------------------------------------------------------------*/
static void
input(void)
{
#ifdef NETSTACK_DECRYPT
  NETSTACK_DECRYPT();
#endif /* NETSTACK_DECRYPT */

  NETSTACK_MAC.input();
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
off(int keep_radio_on)
{
  return keep_radio_on;
}
/*---------------------------------------------------------------------------*/
static unsigned short
cca(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
}
/*---------------------------------------------------------------------------*/
const struct rdc_driver stub_rdc_driver = {
  "stub-rdc",
  init,
  send,
  send_list,
  input,
  on,
  off,
  cca,
};
/*---------------------------------------------------------------------------*/
