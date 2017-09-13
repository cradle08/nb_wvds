#include "contiki.h"
#include "net/rime.h"
#include "net/neighbor.h"
#include "lib/list.h"
#include "lib/memb.h"

MEMB(neighbor_mem, struct neighbor, NEIGHBORS_NUM);
LIST(neighbor_list);
/*------------------------------------------------------------------*/
void
neighbor_init(void)
{
  memb_init(&neighbor_mem);
  list_init(neighbor_list);
}

uint8_t
neighbor_num(void)
{
  return list_length(neighbor_list);
}

struct neighbor *
neighbor_add(rimeaddr_t *addr)
{
  struct neighbor *nbr = NULL;

  nbr = memb_alloc(&neighbor_mem);
  if (nbr == NULL) {
    nbr = list_pop(neighbor_list);
  }
  if (nbr != NULL) {
    rimeaddr_copy(&nbr->addr, addr);
    nbr->role = 0x00;
    list_add(neighbor_list, nbr);
  }

  return nbr;
}

struct neighbor *
neighbor_get(uint8_t idx)
{
  struct neighbor *nbr = NULL;
  uint8_t i = 0;

  for (nbr = list_head(neighbor_list); nbr != NULL; nbr = list_item_next(nbr)) {
    if (i++ == idx)
      return nbr;
  }

  return NULL;
}

struct neighbor *
neighbor_find(rimeaddr_t *addr)
{
  struct neighbor *nbr = NULL;

  for (nbr = list_head(neighbor_list); nbr != NULL; nbr = list_item_next(nbr)) {
    if (rimeaddr_cmp(&nbr->addr, addr))
      return nbr;
  }

  return NULL;
}
/*------------------------------------------------------------------*/
