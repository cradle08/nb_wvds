#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/rime/chameleon.h"
#include "net/rime/abc.h"

/*------------------------------------------------------------------*/
static void (* input_cb)(void);

/*------------------------------------------------------------------*/
static void
network_init(void)
{
  /* nothing need to do */
}

static void
network_input(void)
{
  if (input_cb)
    input_cb();
}

void network_set_rcvd_cb(void (*f)(void))
{
  input_cb = f;
}

/*------------------------------------------------------------------*/
static void
packet_sent(void *ptr, int status, int num_tx)
{
  struct channel *c = (struct channel *)ptr;
  abc_sent(c, status, num_tx);
}

int
rime_output(struct channel *c)
{
  if(chameleon_create(c)) {
    packetbuf_compact();

    NETSTACK_MAC.send(packet_sent, c);
    return 1;
  }
  return 0;
}

/*------------------------------------------------------------------*/
const struct network_driver stub_nwk_driver = {
  "Stub network",
  network_init,
  network_input
};
