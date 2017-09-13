#include "lib/list.h"
#include "lib/memb.h"
#include "net/rime.h"
#include "net/rime/rimeaddr.h"
#include "net/rime/broadcast.h"
#include "sys/logger.h"
#include "dev/radio.h"

#include <string.h>

#define DEBUG 0
#include <stdio.h>
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif

#if CONTIKI_TARGET_COOJA
char *LEVEL_NAME[5] = { "FATAL", "ERROR", "WARN", "INFO", "DEBUG" };
#endif

LIST(log_list);
MEMB(log_mem, struct log_entry, LOG_MAX_ENTRIES);

struct {
  uint16_t drop;
  uint16_t retry;
} logger_stats = { 0,0 };
static uint16_t logger_seqno = 0;
static uint8_t  logger_toair = 1;
#if WITH_LPM
static uint8_t  logger_radio_on = 0;
static struct radio_res *logger_rs = NULL;
#endif

static struct broadcast_conn logger_bc;
static struct ctimer logger_ct;
/* ---------------------------------------------------------------- */
PROCESS(logger_process, "Logger");
/* ---------------------------------------------------------------- */
void logger_bc_recv(struct broadcast_conn *ptr, const rimeaddr_t *sender)
{
  /* nothing need to do */
}

void logger_bc_sent(struct broadcast_conn *ptr, int status, int num_tx)
{
  struct log_entry *le = NULL;

  if (status == MAC_TX_OK) {
    le = list_pop(log_list);
    if (le != NULL) {
      memb_free(&log_mem, le);
      PRINTF("log deq, %d\n", list_length(log_list));
    }

    if (list_length(log_list) > 0) {
      PRINTF("log next\n");
      process_poll(&logger_process);
    } else {
#if WITH_LPM
      radio_off(logger_rs);
      logger_radio_on = 0;
#endif
    }
  }
  else {
    PRINTF("log retry at %d\n", status);
    logger_stats.retry++;
    ctimer_set(&logger_ct, PACKET_TIME, logger_retry, NULL);
  }
}

static const struct broadcast_callbacks logger_bc_cb = { logger_bc_recv, logger_bc_sent };
/* ---------------------------------------------------------------- */
void logger_start(void)
{
  broadcast_open(&logger_bc, LOG_CHANNEL, &logger_bc_cb);
  process_start(&logger_process, NULL);
#if WITH_LPM
  logger_rs = radio_alloc("logger");
#endif
}

void logger_stop(void)
{
  process_exit(&logger_process);
}

void logger_set_toair(uint8_t enable)
{
  logger_toair = enable;
}

static void
logger_fill(struct log_msg *msg, int level, int code, const char *fpath, uint16_t lineno, void *data, int len)
{
  char *fname = NULL;
  int fname_len = 0;

  // get the basename of fpath and assign to fname
  fname = strrchr(fpath, '\\');
  if (fname != NULL) {
    fname = fname + 1;
  } else {
    fname = strrchr(fpath, '/');
    if (fname != NULL) {
      fname = fname + 1;
    } else {
      fname = (char*)fpath;
    }
  }
  fname_len = strlen(fname);

  msg->seqno = HTONS(logger_seqno); logger_seqno++;
  msg->level = level;
  msg->code = code;
  msg->lineno = HTONS(lineno);
  memcpy(msg->fname, fname, (fname_len < LOGMSG_FNAME_MAXLEN ? fname_len : LOGMSG_FNAME_MAXLEN));
  if (data != NULL && len > 0)
    memcpy(msg->data, data, len);
}

void logger_send(int level, int code, const char *fpath, uint16_t lineno, void *data, int len)
{
  struct log_msg msg = {0};

  logger_fill(&msg, level, code, fpath, lineno, data, len);

#if WITH_LPM
  if (logger_radio_on == 0) {
    radio_on(logger_rs, (PACKET_TIME << 2));
    logger_radio_on = 1;
  }
#endif
  packetbuf_copyfrom(&msg, sizeof(struct log_msg));
  broadcast_send(&logger_bc);
}

void logger_enqueue(int level, int code, const char *fpath, uint16_t lineno, void *data, int len)
{
  struct log_entry *le = NULL;
  struct log_msg *msg = NULL;

#if CONTIKI_TARGET_COOJA
  if (level <= LOG_WARN)
    printf("%s at %s:%d\n", LEVEL_NAME[level], fpath, lineno);
#endif

  le = memb_alloc(&log_mem);
  if (le != NULL) {
    msg = &le->msg;
    memset(msg, 0, sizeof(struct log_msg));

    logger_fill(msg, level, code, fpath, lineno, data, len);

    list_add(log_list, le);
    PRINTF("log enq, %d\n", list_length(log_list));
    process_poll(&logger_process);
  }
  else {
    logger_stats.drop++;
  }
}

void logger_retry(void *ptr)
{
  process_poll(&logger_process);
}
/* ---------------------------------------------------------------- */
PROCESS_THREAD(logger_process, ev, data)
{
  PROCESS_BEGIN();

  while(1) {
    static struct log_entry *le = NULL;
    static struct log_msg *msg = NULL;

    PROCESS_WAIT_EVENT();

    if (ev == PROCESS_EVENT_POLL) {
      uint8_t num = list_length(log_list);
      if (num > 0) {
        if (logger_toair) {
          le = list_head(log_list);
          msg = &le->msg;
#if WITH_LPM
          if (logger_radio_on == 0) {
            radio_on(logger_rs, ((PACKET_TIME << 2) * num));
            logger_radio_on = 1;
          }
#endif
          PRINTF("log send\n");
          msg->stats_drop = HTONS(logger_stats.drop);
          msg->stats_retry = HTONS(logger_stats.retry);
          packetbuf_copyfrom(msg, sizeof(struct log_msg));
          broadcast_send(&logger_bc);
        }
        else {
          // just consume the log entry
          le = list_pop(log_list);
          memb_free(&log_mem, le);
          PRINTF("log consume, %d left\n", list_length(log_list));
        }
      }
    }
  }

  PROCESS_END();
}
