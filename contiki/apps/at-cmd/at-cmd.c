#include "contiki.h"
#include "at-cmd.h"
#include <string.h>

#define ATCMD_PROCESS  0

enum {
  S_NONE = 0,
  S_PREA, // A
  S_PRET, // T
  S_PLUS, // +
  S_CMD,
  S_ARG,
  S_CR,   // \r: 0x0D
  S_LF    // \n: 0x0A
};
static uint8_t atcmd_s = S_NONE;

struct atcmd_entry {
  const char *cmd;
  atcmd_callback_t cb;
};
static struct atcmd_entry atcmd_cmdtbl[ATCMD_CMDTBL_SIZE];
static uint8_t atcmd_cmdtbl_n = 0;

static char  atcmd_cmd[ATCMD_CMD_NAME_MAXLEN];
static int   atcmd_cmd_i = 0;
static char  atcmd_arg[ATCMD_CMD_ARGS_MAXLEN];
static int   atcmd_arg_i = 0;

void (*atcmd_byte_output)(unsigned char c);

static void atcmd_dispatch(void);
/*----------------------------------------------------------*/
#if ATCMD_PROCESS
PROCESS(atcmd_process, "AT command");
#endif
/*----------------------------------------------------------*/
void
atcmd_start(void)
{
#if ATCMD_PROCESS
  process_start(&atcmd_process, NULL);
#endif
}

int
atcmd_input(uint8_t b)
{
  if (atcmd_s == S_NONE) {
    if (b == 'A') {
      atcmd_cmd_i = 0;
      atcmd_arg_i = 0;
      memset(atcmd_cmd, 0, sizeof(atcmd_cmd));
      memset(atcmd_arg, 0, sizeof(atcmd_arg));
      atcmd_s = S_PREA;
      return ATCMD_PEND;
    }
  }
  else if (atcmd_s == S_PREA) {
    if (b == 'T') {
      atcmd_s = S_PRET;
      return ATCMD_PEND;
    } else {
      atcmd_s = S_NONE;
      return ATCMD_NOT;
    }
  }
  else if (atcmd_s == S_PRET) {
    if (b == '+') {
      atcmd_s = S_PLUS;
      return ATCMD_PEND;
    } else {
      atcmd_s = S_NONE;
      return ATCMD_NOT;
    }
  }
  else if (atcmd_s == S_PLUS) {
    if (('A' <= b && b <= 'Z') || ('a' <= b && b <= 'z')) {
      atcmd_s = S_CMD;
      atcmd_cmd[atcmd_cmd_i++] = b;
      return ATCMD_PEND;
    } else {
      atcmd_s = S_NONE;
      return ATCMD_NOT;
    }
  }
  else if (atcmd_s == S_CMD) {
    if (('A' <= b && b <= 'Z') || ('a' <= b && b <= 'z') || (b == '?') || (b == '=')) {
      atcmd_cmd[atcmd_cmd_i++] = b;
      if (b == '=')
        atcmd_s = S_ARG;
      return ATCMD_PEND;
    } else if (b == ' ') {
      atcmd_s = S_ARG;
      return ATCMD_PEND;
    } else if (b == 0x0D) { // \r
      atcmd_s = S_CR;
      return ATCMD_PEND;
    } else {
      atcmd_s = S_NONE;
      return ATCMD_NOT;
    }
  }
  else if (atcmd_s == S_ARG) {
    if (b == 0x0D) { // \r
      atcmd_s = S_CR;
      return ATCMD_PEND;
    } else {
      atcmd_arg[atcmd_arg_i++] = b;
      return ATCMD_PEND;
    }
  }
  else if (atcmd_s == S_CR) {
    if (b == 0x0A) { // \n
      atcmd_s = S_LF;

      atcmd_dispatch();
      atcmd_s = S_NONE;
      return ATCMD_OK;
    } else {
      atcmd_s = S_NONE;
      return ATCMD_NOT;
    }
  }
  return ATCMD_NOT;
}

int
atcmd_register(const char *cmd, atcmd_callback_t cb)
{
  struct atcmd_entry *at;

  if (atcmd_cmdtbl_n < ATCMD_CMDTBL_SIZE) {
    at = &atcmd_cmdtbl[atcmd_cmdtbl_n];
    at->cmd = cmd;
    at->cb = cb;
    ++atcmd_cmdtbl_n;
    return 0;
  }

  return 1;
}

void
atcmd_set_output(void (*f)(unsigned char c))
{
  atcmd_byte_output = f;
}

static void
atcmd_dispatch(void)
{
  int i;
  int r = -1;

  atcmd_cmd[atcmd_cmd_i] = '\0';
  for (i = 0; i < atcmd_cmdtbl_n; i++) {
    if (strncmp(atcmd_cmdtbl[i].cmd, atcmd_cmd, strlen(atcmd_cmd)) == 0) {
      r = (atcmd_cmdtbl[i].cb)(atcmd_arg, atcmd_arg_i);
      break;
    }
  }

  if (r == 0) {
    atcmd_output("AT+OK\r\n", 7);
  }
  else if (r > 0) {
    char err[1];
    err[0] = '0'+r;
    atcmd_output("AT+ERR", 6);
    atcmd_output(err, 1);
    atcmd_output("\r\n", 2);
  }
}

void
atcmd_output(const char *str, int len)
{
  int i;

  if (atcmd_byte_output != NULL) {
    for (i = 0; i < len; i++)
      atcmd_byte_output(str[i]);
  }
}
/*----------------------------------------------------------*/
#if ATCMD_PROCESS
PROCESS_THREAD(atcmd_process, ev, data)
{
  PROCESS_BEGIN();

  while (1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}
#endif
