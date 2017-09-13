#ifndef _AT_CMD_H
#define _AT_CMD_H

#ifndef ATCMD_CMDTBL_SIZE
#define ATCMD_CMDTBL_SIZE   8
#endif

#ifndef ATCMD_CMD_NAME_MAXLEN
#define ATCMD_CMD_NAME_MAXLEN    16
#endif

#ifndef ATCMD_CMD_ARGS_MAXLEN
#define ATCMD_CMD_ARGS_MAXLEN    24
#endif

enum {
  ATCMD_OK   = 0,
  ATCMD_PEND = 1,
  ATCMD_NOT  = 2
};

typedef int (*atcmd_callback_t)(const char *arg, int len);

void atcmd_start(void);
int atcmd_input(uint8_t b);
void atcmd_output(const char *str, int len);
int atcmd_register(const char *cmd, atcmd_callback_t f);
void atcmd_set_output(void (*f)(unsigned char c));

#endif /* _AT_CMD_H */
