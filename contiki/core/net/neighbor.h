#ifndef _NEIGHBOR_H
#define _NEIGHBOR_H

#include "contiki.h"
#include "net/rime/rimeaddr.h"

#ifndef NEIGHBORS_NUM
#define NEIGHBORS_NUM  32
#endif

struct neighbor {
  struct neighbor *next;
  rimeaddr_t addr;
  uint8_t role;
};

void neighbor_init(void);
uint8_t neighbor_num(void);
struct neighbor *neighbor_add(rimeaddr_t *addr);
struct neighbor *neighbor_get(uint8_t idx);
struct neighbor *neighbor_find(rimeaddr_t *addr);

#endif /* _NEIGHBOR_H */
