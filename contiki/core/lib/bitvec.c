#include "contiki.h"
#include "lib/bitvec.h"
#include "lib/list.h"
#include "lib/memb.h"
#include <string.h>

#define DEBUG 0
#if DEBUG
#define PRINTF(...)  printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

struct bitvec_entry {
  struct bitvec_entry *next;
  struct bitvec bitv;
};
MEMB(bitvec_mem, struct bitvec_entry, BITVEC_MAX_INSTANCE);
LIST(bitvec_lst);

static uint8_t bitvec_inited = 0;

struct bitvec *
bitvec_create(uint16_t size)
{
  struct bitvec_entry *e = NULL;

  if (bitvec_inited == 0) {
    PRINTF("bitvec init\n");
    memb_init(&bitvec_mem);
    list_init(bitvec_lst);
    bitvec_inited = 1;
  }

  if (size > BITVEC_MAX_SIZE)
    return NULL;

  e = memb_alloc(&bitvec_mem);
  if (e != NULL) {
    memset(e->bitv.data, 0, ((BITVEC_MAX_SIZE>>3)+1));
    e->bitv.size = size;
    list_add(bitvec_lst, e);
    PRINTF("bitvec create %d, %p/%p\n", size, &e->bitv, e);
    return &e->bitv;
  }

  return NULL;
}

int
bitvec_free(struct bitvec *bv)
{
  struct bitvec_entry *e = NULL;
  int ret = -1;

  for (e = list_head(bitvec_lst); e != NULL; e = list_item_next(e)) {
    if (bv == &e->bitv) {
      list_remove(bitvec_lst, e);
      ret = memb_free(&bitvec_mem, e);
      break;
    }
  }

  PRINTF("bitvec free %p/%p, %d\n", bv, e, ret);
  return ((ret != -1) ? 0 : -1);
}

int
bitvec_get(struct bitvec *bv, uint16_t idx)
{
  uint16_t n = 0;
  uint8_t  b = 0;

  if (bv == NULL)
    return -1;
  if (idx >= bv->size)
    return -2;

  n = (idx >> 3);
  b = (1 << (idx & 0x07));
  PRINTF("bitvec get %d, %d/0x%02X/0x%02X\n", idx, n, b, bv->data[n]);
  return ((bv->data[n] & b) ? 1 : 0);
}

int
bitvec_set(struct bitvec *bv, uint16_t idx)
{
  uint16_t n = 0;
  uint8_t  b = 0;

  if (bv == NULL)
    return -1;
  if (idx >= bv->size)
    return -2;

  n = (idx >> 3);
  b = (1 << (idx & 0x07));
  bv->data[n] |= b;
  PRINTF("bitvec set %d, %d/0x%02X/0x%02X\n", idx, n, b, bv->data[n]);
  return 0;
}

int
bitvec_clr(struct bitvec *bv, uint16_t idx)
{
  uint16_t n = 0;
  uint8_t  b = 0;

  if (bv == NULL)
    return -1;
  if (idx >= bv->size)
    return -2;

  n = (idx >> 3);
  b = (1 << (idx & 0x07));
  bv->data[n] &= ~b;
  PRINTF("bitvec clr %d, %d/0x%02X/0x%02X\n", idx, n, b, bv->data[n]);
  return 0;
}

int
bitvec_is_set(struct bitvec *bv, uint16_t idx)
{
  uint16_t n = 0;
  uint8_t  b = 0;

  if (bv == NULL)
    return -1;
  if (idx >= bv->size)
    return -2;

  n = (idx >> 3);
  b = (1 << (idx & 0x07));
  PRINTF("bitvec is_set %d, %d/0x%02X/0x%02X\n", idx, n, b, bv->data[n]);
  return (((bv->data[n] & b) > 0) ? 1 : 0);
}

int
bitvec_is_clr(struct bitvec *bv, uint16_t idx)
{
  uint16_t n = 0;
  uint8_t  b = 0;

  if (bv == NULL)
    return -1;
  if (idx >= bv->size)
    return -2;

  n = (idx >> 3);
  b = (1 << (idx & 0x07));
  PRINTF("bitvec is_clr %d, %d/0x%02X/0x%02X\n", idx, n, b, bv->data[n]);
  return (((bv->data[n] & b) == 0) ? 1 : 0);
}

