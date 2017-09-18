#ifndef __PLATFORM_CONF_H__
#define __PLATFORM_CONF_H__

#undef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE 200

#undef UIP_CONF_TCP_MSS
#define UIP_CONF_TCP_MSS (UIP_CONF_BUFFER_SIZE - 70)
#undef UIP_CONF_RECEIVE_WINDOW
#define UIP_CONF_RECEIVE_WINDOW UIP_CONF_TCP_MSS

/* CPU target speed in Hz; works fine at 8, 16, 18 MHz but not higher. */
#define F_CPU 8000000uL

/* Our clock resolution, this is the same as Unix HZ. */
#define CLOCK_CONF_SECOND 128UL

#define BAUD2UBR(baud) ((F_CPU/baud))

#define CCIF
#define CLIF

#define HAVE_STDINT_H
#define MSP430_MEMCPY_WORKAROUND 1
#if defined(__MSP430__) && defined(__GNUC__) && MSP430_MEMCPY_WORKAROUND
#else /* __GNUC__ &&  __MSP430__ && MSP430_MEMCPY_WORKAROUND */
#define w_memcpy memcpy
#endif /* __GNUC__ &&  __MSP430__ && MSP430_MEMCPY_WORKAROUND */
#include "msp430def.h"

/* Types for clocks and uip_stats */
typedef unsigned short uip_stats_t;
typedef unsigned long clock_time_t;
typedef unsigned long off_t;

/* DCO speed resynchronization for more robust UART, etc. */
/* Not needed from MSP430x5xx since it make use of the FLL */
#define DCOSYNCH_CONF_ENABLED 0
#define DCOSYNCH_CONF_PERIOD 30

#define ROM_ERASE_UNIT_SIZE  512
#define XMEM_ERASE_UNIT_SIZE (64*1024L)

#define CFS_CONF_OFFSET_TYPE    long

/* Use the first 64k of external flash for node configuration */
#define NODE_ID_XMEM_OFFSET     (0 * XMEM_ERASE_UNIT_SIZE)

/* Use the second 64k of external flash for codeprop. */
#define EEPROMFS_ADDR_CODEPROP  (1 * XMEM_ERASE_UNIT_SIZE)

#define CFS_XMEM_CONF_OFFSET    (2 * XMEM_ERASE_UNIT_SIZE)
#define CFS_XMEM_CONF_SIZE      (1 * XMEM_ERASE_UNIT_SIZE)

#define CFS_RAM_CONF_SIZE 4096

/*
 * SPI bus configuration
 */

/* SPI input/output registers. */
#define SPI_TXBUF UCB0TXBUF
#define SPI_RXBUF UCB0RXBUF

                                /* USART0 Tx ready? */
#define SPI_WAITFOREOTx() while ((UCB0STAT & UCBUSY) != 0)
                                /* USART0 Rx ready? */
#define SPI_WAITFOREORx() while ((UCB0IFG & UCRXIFG) == 0)
                                /* USART0 Tx buffer ready? */
#define SPI_WAITFORTxREADY() while ((UCB0IFG & UCTXIFG) == 0)

#define MOSI           1  /* P3.1 - Output: SPI Master out - slave in (MOSI) */
#define MISO           2  /* P3.2 - Input:  SPI Master in - slave out (MISO) */
#define SCK            3  /* P3.3 - Output: SPI Serial Clock (SCLK) */

#define CC11xx_CC1120           1
#define CC11xx_ARCH_SPI_ENABLE  cc1120_arch_spi_enable
#define CC11xx_ARCH_SPI_DISABLE cc1120_arch_spi_disable
#define CC11xx_ARCH_SPI_RW_BYTE cc1120_arch_spi_rw_byte
#define CC11xx_ARCH_SPI_RW      cc1120_arch_spi_rw

#define cc11xx_arch_spi_enable  cc1120_arch_spi_enable
#define cc11xx_arch_spi_disable cc1120_arch_spi_disable
#define cc11xx_arch_spi_rw_byte cc1120_arch_spi_rw_byte
#define cc11xx_arch_spi_rw      cc1120_arch_spi_rw
#define cc11xx_arch_interrupt_enable cc1120_arch_interrupt_enable
#define cc11xx_arch_interrupt_disable cc1120_arch_interrupt_disable

#define cc11xx_arch_init        cc1120_arch_init

#ifndef RADIO_TX_DELAY
#define RADIO_TX_DELAY   100
#endif
#ifndef RADIO_ACK_DELAY
#define RADIO_ACK_DELAY  100
#define RADIO_ACK_DELAY_R  8
#endif

#if RADIO_ACK_DELAY
#define NULLRDC_CONF_ACK_WAIT_TIME                RTIMER_SECOND / RADIO_ACK_DELAY_R
#else
#define NULLRDC_CONF_ACK_WAIT_TIME                RTIMER_SECOND / 200
#endif
#define NULLRDC_CONF_AFTER_ACK_DETECTED_WAIT_TIME RTIMER_SECOND / 100

#define NETSTACK_CONF_RADIO   cc11xx_driver
#define NETSTACK_RADIO_MAX_PAYLOAD_LEN 125

signed char cc11xx_read_rssi(void);
void cc11xx_channel_set(uint8_t c);
#define MULTICHAN_CONF_SET_CHANNEL(x) cc11xx_channel_set(x)
#define MULTICHAN_CONF_READ_RSSI(x) cc11xx_read_rssi()

#include "mist-conf-const.h"
#define MIST_CONF_NETSTACK (MIST_CONF_NULLRDC | MIST_CONF_AES)

#undef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM        6
#undef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS     20
#undef UIP_CONF_DS6_ROUTE_NBU
#define UIP_CONF_DS6_ROUTE_NBU   20

#undef SICSLOWPAN_CONF_MAXAGE
#define SICSLOWPAN_CONF_MAXAGE 16

/*-------------------------------------------------------------------*/
#define RFCHAN_MIN  1         // 射频最小信道号
#define RFCHAN_MAX  10        // 射频最大信道号

#ifndef RFPOWER_MIN
#define RFPOWER_MIN     17    // 最小射频功率
#endif
#ifndef RFPOWER_MAX
#define RFPOWER_MAX     28    // 最大射频功率
#endif
#define RFPOWER_PA_MIN  15    // 使用PA的最小射频功率
#define RFPOWER_PA_OFS  27    // PA的功率增量

// 定义为1时节点工作状态亮灯，休眠状态灭灯
#ifndef LED_LPM
#define LED_LPM  0
#endif

// 定义为1时射频开启时亮灯，射频关闭时灭灯
#ifndef LED_RADIO
#define LED_RADIO  0
#endif

// 定义Flash对应的设备号
#define FLASH_A 0
#define FLASH_B 1

#endif /* __PLATFORM_CONF_H__ */
