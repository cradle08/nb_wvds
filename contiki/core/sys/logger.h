#ifndef _LOGGER_H
#define _LOGGER_H

#include "contiki.h"

#define LOG_CHANNEL      0x99
#define LOG_MAX_ENTRIES  4

enum {
  LOG_FATAL = 0,
  LOG_ERROR = 1,
  LOG_WARN  = 2,
  LOG_INFO  = 3,
  LOG_DEBUG = 4
};

#define LOGMSG_FNAME_MAXLEN  20
#define LOGMSG_DATA_MAXLEN   30

struct log_msg {
  uint8_t   level;
  uint8_t   code;
  uint16_t  lineno;
  char      fname[LOGMSG_FNAME_MAXLEN];
  uint8_t   data[LOGMSG_DATA_MAXLEN];
  uint16_t  seqno;
  uint16_t  stats_drop;
  uint16_t  stats_retry;
};

struct log_entry {
  struct log_entry *next;
  struct log_msg   msg;
  uint8_t          done;
};

void logger_start(void);
void logger_stop(void);
void logger_send(int level, int code, const char *fpath, uint16_t lineno, void *data, int len);
void logger_enqueue(int level, int code, const char *fname, uint16_t lineno, void *data, int len);
void logger_retry(void *ptr);
void logger_set_toair(uint8_t enable);

#define log_e(code,data,len) \
  logger_enqueue(LOG_ERROR, code, __FILE__, __LINE__, data, len)

#define log_w(code,data,len) \
  logger_enqueue(LOG_WARN, code, __FILE__, __LINE__, data, len)

#define log_i(code,data,len) \
  logger_enqueue(LOG_INFO, code, __FILE__, __LINE__, data, len)

#define log_d(code,data,len) \
  logger_enqueue(LOG_DEBUG, code, __FILE__, __LINE__, data, len)

#define log_s(lvl,code,data,len) \
  logger_send(lvl, code, __FILE__, __LINE__, data, len)

#endif /* _LOGGER_H */
