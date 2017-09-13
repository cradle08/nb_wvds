#ifndef _TASKMON_H
#define _TASKMON_H

#include "contiki.h"
#include "sys/clock.h"

#ifndef TASKMON_MAX_NUM
#define TASKMON_MAX_NUM 8
#endif

#define TASKMON_NAME_LEN 10

#define TASKMON_GUARD_TIME (CLOCK_SECOND >> 6)

struct taskmon {
  char     name[TASKMON_NAME_LEN];
  uint32_t beg;
  uint32_t end;
  clock_time_t deadline;
  uint8_t  running;
  uint8_t  pending;
};

int taskmon_init(clock_time_t timeout);
int taskmon_init_end(void);
int taskmon_ok(void);
struct taskmon * taskmon_create(char *name);
void taskmon_free(struct taskmon *mon);

void task_begin(struct taskmon *mon, clock_time_t dt);
void task_end(struct taskmon *mon);
int task_pending(struct taskmon *mon);

extern struct taskmon *err_mon;

#endif /* _TASKMON_H */
