#ifndef __BITVEC_H_
#define __BITVEC_H_

#include "contiki.h"

#ifndef BITVEC_MAX_SIZE
#define BITVEC_MAX_SIZE 256
#endif

#ifndef BITVEC_MAX_INSTANCE
#define BITVEC_MAX_INSTANCE 1
#endif

struct bitvec {
  uint8_t  data[(BITVEC_MAX_SIZE>>3)+1];
  uint16_t size;
};

int bitvec_get(struct bitvec *bv, uint16_t idx);
int bitvec_set(struct bitvec *bv, uint16_t idx);
int bitvec_clr(struct bitvec *bv, uint16_t idx);
int bitvec_is_set(struct bitvec *bv, uint16_t idx);
int bitvec_is_clr(struct bitvec *bv, uint16_t idx);

struct bitvec *bitvec_create(uint16_t size);
int bitvec_free(struct bitvec *bv);

#endif /* __BITVEC_H_ */
