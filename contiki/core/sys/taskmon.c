// Task Monitor

#include "sys/taskmon.h"
#include <string.h>

#ifndef MAX_CLOCK
#define MAX_CLOCK ((clock_time_t)-1)
#endif

static struct taskmon monitors[TASKMON_MAX_NUM] = {0};
static uint8_t monitorN = 0;

static struct taskmon *init_mon = NULL;
static struct taskmon *taskmon_err = NULL;

int
taskmon_init(clock_time_t timeout)
{
  init_mon = taskmon_create("init");
  task_begin(init_mon, timeout);
  return 0;
}

int
taskmon_init_end(void)
{
  if (init_mon != NULL) {
    task_end(init_mon);
    taskmon_free(init_mon);
    init_mon = NULL;
  }
  return 0;
}

int
taskmon_ok(void)
{
  struct taskmon *mon = NULL;
  clock_time_t now = 0;
  uint8_t i;

  now = clock_time();
  for (i = 0; i < TASKMON_MAX_NUM; i++) {
    mon = &monitors[i];
    if (mon->running && (mon->end != mon->beg) && (now > mon->deadline)) {
      taskmon_err = mon;
      return 0; // this task did not finish normally
    }
  }
  return 1; // all tasks are running well
}

struct taskmon *
taskmon_create(char *name)
{
  struct taskmon *mon = NULL;
  uint8_t i;

  for (i = 0; i < TASKMON_MAX_NUM; i++) {
    mon = &monitors[i];
    if (mon->running == 0) {
      strncpy(mon->name, name, strlen(name));
      mon->running = 1;
      mon->beg = 0;
      mon->end = 0;
      mon->deadline = MAX_CLOCK;
      mon->pending = 0;
      ++monitorN;
      return mon;
    }
  }

  return NULL;
}

void
taskmon_free(struct taskmon *mon)
{
  uint8_t i;

  for (i = 0; i < TASKMON_MAX_NUM; i++) {
    if (mon == &monitors[i]) {
      memset(mon->name, 0, TASKMON_NAME_LEN);
      mon->running = 0;
      mon->beg = 0;
      mon->end = 0;
      mon->deadline = 0;
      mon->pending = 0;
      --monitorN;
    }
  }
}

void
task_begin(struct taskmon *mon, clock_time_t dt)
{
  if ((mon != NULL) && (mon->end == mon->beg)) {
    mon->beg++;
    mon->deadline = clock_time() + dt + TASKMON_GUARD_TIME;
    mon->pending = 1;
  }
}

void
task_end(struct taskmon *mon)
{
  if ((mon != NULL) && (mon->beg == mon->end + 1)) {
    mon->end++;
    mon->deadline = MAX_CLOCK;
    mon->pending = 0;
  }
}

int
task_pending(struct taskmon *mon)
{
  if ((mon != NULL) && (mon->pending == 1))
    return 1;
  return 0;
}

