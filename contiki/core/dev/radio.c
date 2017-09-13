#include "contiki.h"
#include "net/netstack.h"
#include "sys/ctimer.h"
#include "lib/memb.h"
#include <string.h>

#if WITH_TASKMON
#include "sys/taskmon.h"
#endif /* WITH_TASKMON */

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...)  printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/*------------------------------------------------------------------*/
#define RADIO_RES_SIZE 8
struct radio_res {
  //char name[11];
  uint8_t is_on;
  uint32_t on;
  uint32_t off;
  struct ctimer ct;
};
MEMB(radio_res_mem, struct radio_res, RADIO_RES_SIZE);

static uint8_t radio_inited = 0;
static uint8_t radio_is_on = 0;
#if WITH_TASKMON
static struct taskmon *radio_mon = NULL;
#endif /* WITH_TASKMON */
/*------------------------------------------------------------------*/
void
radio_timeout(void *ptr)
{
  struct radio_res *rs = (struct radio_res *)ptr;
  uint8_t changed = 0;

  PRINTF("radio timeout %p\n", rs);
  if (rs->is_on == 1) {
    rs->is_on = 0;
    rs->off++;
    --radio_is_on; changed = 1;
  } else {
    asm("nop");
  }

  if (changed && (radio_is_on == 0)) {
    NETSTACK_MAC.off(0);
#if WITH_TASKMON
    task_end(radio_mon);
#endif /* WITH_TASKMON */
  }
}

struct radio_res *
radio_alloc(char *name)
{
  struct radio_res *rs = NULL;

  if (radio_inited == 0) {
    PRINTF("radio init\n");
    memb_init(&radio_res_mem);
#if WITH_TASKMON
    radio_mon = taskmon_create("radio");
#endif /* WITH_TASKMON */
    radio_inited = 1;
  }

  rs = memb_alloc(&radio_res_mem);
  if (rs != NULL) {
    //strncpy(rs->name, name, strlen(name));
    rs->is_on = 0;
    rs->on = 0;
    rs->off = 0;
    PRINTF("radio alloc %p\n", rs);
  }

  return rs;
}

void
radio_free(struct radio_res *rs)
{
  if (rs != NULL) {
    memb_free(&radio_res_mem, rs);
  }
}

void
radio_on(struct radio_res *rs, clock_time_t timeout)
{
  uint8_t changed = 0;

  if (rs == NULL) {
    return;
  }

  if (rs->is_on == 0) {
    rs->is_on = 1;
    rs->on++;
    ctimer_set(&rs->ct, timeout, radio_timeout, rs);
    ++radio_is_on; changed = 1;
  } else {
    ctimer_set(&rs->ct, timeout, radio_timeout, rs);
  }

#if WITH_TASKMON
  if (!task_pending(radio_mon)) {
    task_begin(radio_mon, timeout);
  } else {
    if (clock_time() + timeout > radio_mon->deadline) {
      task_end(radio_mon);
      task_begin(radio_mon, timeout);
    }
  }
#endif /* WITH_TASKMON */

  PRINTF("radio on %d, %p\n", radio_is_on, rs);
  if (changed && (radio_is_on == 1)) {
    NETSTACK_MAC.on();
  }
}

void
radio_off(struct radio_res *rs)
{
  uint8_t changed = 0;

  if (rs == NULL) {
    return;
  }

  if (rs->is_on == 1) {
    rs->is_on = 0;
    rs->off++;
    ctimer_stop(&rs->ct);
    --radio_is_on; changed = 1;
  } else {
    asm("nop");
  }

  PRINTF("radio off %d, %p\n", radio_is_on, rs);
  if (changed && (radio_is_on == 0)) {
    NETSTACK_MAC.off(0);
#if WITH_TASKMON
    task_end(radio_mon);
#endif /* WITH_TASKMON */
  }
}
