#include "contiki.h"
#include "sys/logger.h"
#include "dev/uart.h"
#include "dev/flash.h"
#include "dev/leds.h"
#include "cc11xx.h"
#include "at-cmd.h"
#include <stdio.h>

#ifndef APP_VER_MAJOR
#define APP_VER_MAJOR 1
#endif
#ifndef APP_VER_MINOR
#define APP_VER_MINOR 0
#endif
#ifndef APP_VER_PATCH
#define APP_VER_PATCH 0
#endif
/*------------------------------------------------------------------*/
uint8_t rf_act_chan  = 2;
uint8_t rf_act_power = 17;
uint8_t rf_work_chan = 6;

extern struct process vdadapter_process;
/*------------------------------------------------------------------*/
PROCESS(activator_process, "Activator");
AUTOSTART_PROCESSES(&activator_process);
/*------------------------------------------------------------------*/
void
app_load(void)
{
  rf_act_chan = flash_read(0x1802);
  rf_act_power = flash_read(0x1804);
  rf_work_chan = flash_read(0x1806);
}

void
app_save(void)
{
  flash_clear(0x1800);
  flash_write(0x1800, 0xCDAB);
  flash_write(0x1802, rf_act_chan);  // 激活信道
  flash_write(0x1804, rf_act_power); // 射频功率
  flash_write(0x1806, rf_work_chan); // 工作信道
}

/*------------------------------------------------------------------*/
int
app_get_channel(const char *arg, int len)
{
  uint8_t chan = radio_get_channel();
  char out[4] = {0};
  uint8_t i = 0;

  if (chan >= 10)
    out[i++] = '0' + (chan / 10);
  out[i++] = '0' + (chan % 10);
  out[i++] = '\r';
  out[i++] = '\n';

  atcmd_output(out, i);
  return -1;
}

int
app_set_channel(const char *arg, int len)
{
  uint8_t chan = 0;
  uint8_t i;

  for (i = 0; i < len; i++) {
    if (('0' <= arg[i]) && (arg[i] <= '9')) {
      chan = (chan * 10) + (arg[i] - '0');
    } else {
      return 1;
    }
  }

  if ((chan < RFCHAN_MIN) || (chan > RFCHAN_MAX)) {
    return 2;
  }

  rf_act_chan = chan;
  app_save();
  radio_set_channel(chan);
  return 0;
}

int
app_get_rfpower(const char *arg, int len)
{
  int8_t val = radio_get_txpower();
  char out[5] = {0};
  uint8_t i = 0;

  if (val < 0)
    out[i++] = '-';
  if (val >= 10) {
    out[i++] = '0' + (val / 10);
  } else if (val <= -10) {
    out[i++] = '0' + ((0-val) / 10);
  }
  if (val >= 0) {
    out[i++] = '0' + (val % 10);
  } else {
    out[i++] = '0' + ((0-val) % 10);
  }
  out[i++] = '\r';
  out[i++] = '\n';

  atcmd_output(out, i);
  return -1;
}

int
app_set_rfpower(const char *arg, int len)
{
  uint8_t i = 0;
  int8_t power = 0;
  int neg = 1;

  for (i = 0; i < len; i++) {
    if (arg[i] == '-') {
      neg = -1;
    } else if (('0' <= arg[i]) && (arg[i] <= '9')) {
      power = (power * 10) + (arg[i] - '0');
    } else {
      return 1;
    }
  }
  power *= neg;

  if ((power < RFPOWER_MIN) || (power > RFPOWER_MAX)) {
    return 2;
  }

  rf_act_power = power;
  app_save();
  radio_set_txpower(power);
  return 0;
}

int
app_get_work_channel(const char *arg, int len)
{
  uint8_t chan = rf_work_chan;
  char out[4] = {0};
  uint8_t i = 0;

  if (chan >= 10)
    out[i++] = '0' + (chan / 10);
  out[i++] = '0' + (chan % 10);
  out[i++] = '\r';
  out[i++] = '\n';

  atcmd_output(out, i);
  return -1;
}

int
app_set_work_channel(const char *arg, int len)
{
  uint8_t chan = 0;
  uint8_t i;

  for (i = 0; i < len; i++) {
    if (('0' <= arg[i]) && (arg[i] <= '9')) {
      chan = (chan * 10) + (arg[i] - '0');
    } else {
      return 1;
    }
  }

  if ((chan < RFCHAN_MIN) || (chan > RFCHAN_MAX)) {
    return 2;
  }

  rf_work_chan = chan;
  app_save();
  return 0;
}

/*------------------------------------------------------------------*/
PROCESS_THREAD(activator_process, ev, data)
{
  static struct etimer et;
  static uint8_t arg[4];

  PROCESS_BEGIN();

  process_start(&vdadapter_process, NULL);
  etimer_set(&et, (CLOCK_SECOND>>1));

  atcmd_start(); // 启动AT指令模块
  uart_set_input(atcmd_input); // 设置串口输入由AT模块处理
  atcmd_set_output(uart_writeb); // 设置AT模块反馈从串口输出
  atcmd_register("CHAN?", app_get_channel); // 注册查询射频信道的回调函数
  atcmd_register("CHAN=", app_set_channel); // 注册设置射频信道的回调函数
  atcmd_register("POWER?", app_get_rfpower); // 注册查询射频功率的回调函数
  atcmd_register("POWER=", app_set_rfpower); // 注册设置射频功率的回调函数
  atcmd_register("WORK?", app_get_work_channel); // 注册查询射频信道的回调函数
  atcmd_register("WORK=", app_set_work_channel); // 注册设置射频信道的回调函数

  if (flash_read(0x1800) != 0xCDAB) {
    app_save();
  }
  app_load();

  atcmd_output("activator\n", 10);
  printf("chan:%d, power:%d, work:%d\n", rf_act_chan, rf_act_power, rf_work_chan);

  radio_set_channel(rf_act_chan);
  radio_set_txpower(rf_act_power);

  leds_on(LEDS_GREEN);
  logger_start();
  arg[0] = rf_act_chan;
  arg[1] = rf_act_power;
  arg[2] = rf_work_chan;
  log_i(0x81, arg, 3);

  while (1) {
    PROCESS_WAIT_EVENT();

    if (etimer_expired(&et)) {
      etimer_reset(&et);
      leds_toggle(LEDS_GREEN);
    }
  }

  PROCESS_END();
}
