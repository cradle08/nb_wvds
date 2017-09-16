#include <msp430x54x.h>
#include <intrinsics.h>
#include "isr_compat.h"
#include "hal-pmm.h"
#include "m25pe.h"
#include "platform.h"
#include "crc16.h"
#include <string.h>

#define BOARD_CADRE1120_VD  1
#define BOARD_CADRE1120_RP  0
#define BOARD_CADRE1120_AP  0

#if (BOARD_CADRE1120_VD + BOARD_CADRE1120_RP + BOARD_CADRE1120_AP) > 1
#error "only one of BOARD_CADRE1120_AP/RP/VD can be defined as 1"
#elif BOARD_CADRE1120_VD
#warning "bootloader for BOARD_CADRE1120_VD"
#elif BOARD_CADRE1120_RP
#warning "bootloader for BOARD_CADRE1120_RP"
#elif BOARD_CADRE1120_AP
#warning "bootloader for BOARD_CADRE1120_AP"
#else
#error "either BOARD_CADRE1120_VD or BOARD_CADRE1120_AP must be defined as 1"
#endif

#define MAGIC  0xCDAB
#define RESETVECTORADDR_APP  0xEFFE        //应用程序复位向量地址

#define BOOT_ADDR  0x1800
struct BOOT {
  uint16_t  magic;   // 初始化标志
   uint8_t  role;    // 节点类型
   uint8_t  version; // 固件版本
   uint8_t  update;  // 更新来源
   uint8_t  flash;   // Flash编号
  uint32_t  size;    // 新固件大小
  uint32_t  addr;    // 新固件地址
  uint16_t  crc;     // 新固件校验和
   uint8_t  updver;  // 新固件版本
};
static struct BOOT boot;

struct Segment {
  uint32_t  beg;
  uint32_t  end;
};
static struct Segment segs[3] = {
  {  0x5C00,  0xEE00 },
  {  0xEE00,  0xF000 },
  { 0x10000, 0x26C00 }
};
static uint8_t segi = 0;

#define SEGMENT_SIZE 512
static uint8_t SegBuf[SEGMENT_SIZE];

#define DATA_LEN   128
#define FRAME_LEN  (10 + DATA_LEN + 2)

static uint8_t inPkt = 0;
static uint8_t inEsc = 0;

/******************************************************************************/
#if BOARD_CADRE1120_AP || BOARD_CADRE1120_RP
#define UART(reg)  UCA1##reg
#elif BOARD_CADRE1120_VD
#define UART(reg)  UCA3##reg
#else
#error "no support"
#endif

#if BOARD_CADRE1120_AP || BOARD_CADRE1120_RP
#define TOG_LED_FLASH() do { P4OUT ^= BIT0; } while(0)
#define TOG_LED_EXTFLASH() do { P4OUT ^= BIT1; } while(0)
#define TOG_LED_UART() do { P4OUT ^= BIT2; } while(0)
#elif BOARD_CADRE1120_VD
#define TOG_LED_FLASH() do { P2OUT ^= BIT0; } while(0)
#define TOG_LED_EXTFLASH()
#define TOG_LED_UART()
#else
#define TOG_LED_FLASH()
#define TOG_LED_EXTFLASH()
#define TOG_LED_UART()
#endif

/******************************************************************************/
void InitUart(void);
void Application(void);
void UpdateFromUart(void);
void UpdateFromFlash(void);
void ShowLed(int mode);

void SendAck(uint8_t code, uint32_t addr);
void EraseFlash(unsigned long addr);
unsigned char WriteFlash(unsigned long addr,unsigned char *pdata, unsigned int length);
unsigned char ReadFlashByte(unsigned long waddr);
int ReadFlash(unsigned long addr, unsigned char *buf, unsigned int len);

unsigned char AsciiToHex(unsigned char cNum);
char HexToAscii(unsigned char num);
void UartSend(unsigned char *data, unsigned int len);

extern void msp430_cpu_init(void);
//******************************************************************************
// 描述: 引导程序主程序,此程序首先下载到MSMP430单片机中
// 输入: 无   返回:无
void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT

  ReadFlash(BOOT_ADDR, (unsigned char*)&boot, sizeof(struct BOOT));
  if (boot.magic != MAGIC) {
    boot.magic   = MAGIC;
#if BOARD_CADRE1120_VD
    boot.role    = 1;       // 硬件类型: VD:1, RP:2, AP:4
#elif BOARD_CADRE1120_RP
    boot.role    = 2;       // 硬件类型: VD:1, RP:2, AP:4
#elif BOARD_CADRE1120_AP
    boot.role    = 4;       // 硬件类型: VD:1, RP:2, AP:4
#endif
    boot.version = 0x10;    // 软件版本号1.0
    boot.update  = 0;       // 0:正常运行, 1:从串口更新, 2:从Flash更新
#if BOARD_CADRE1120_VD
    boot.flash   = 0;       // Flash 0
#else
    boot.flash   = 1;       // Flash 1
#endif
    boot.addr    = 0x0000;
    boot.size    = 0;
    boot.crc     = 0x0000;
    boot.updver  = 0x00;
    EraseFlash(BOOT_ADDR);
    WriteFlash(BOOT_ADDR, (unsigned char*)&boot, sizeof(struct BOOT));
  }

  if (boot.update == 1) {
    UpdateFromUart();
  }
  else if (boot.update == 2) {
    UpdateFromFlash();
  }
  else if (boot.update == 0) {
    if (ReadFlashByte(0xEFFF)==0xFF && ReadFlashByte(0xEFFE)==0xFF) {
      UpdateFromUart(); // No application
    } else {
      Application();
    }
  }
}

void ShowLed(int mode)
{
#if BOARD_CADRE1120_AP || BOARD_CADRE1120_RP
  int i;

  P4SEL  = 0x00;
  P4DIR |= 0x0F;
  P4OUT |= 0x0F;

  if (mode == 1) {
    P4OUT &= ~BIT2;
    for (i = 0; i < 12; i++) {
      __delay_cycles(F_CPU>>2);
      P4OUT ^= BIT0;
    }
    P4OUT &= ~BIT0;
  }
  else if (mode == 2) {
    P4OUT &= ~BIT1;
    for (i = 0; i < 12; i++) {
      __delay_cycles(F_CPU>>2);
      P4OUT ^= BIT0;
    }
  }

#elif BOARD_CADRE1120_VD
  int i;

  P2SEL  = 0x00;
  P2DIR |= 0x01;
  P2OUT |= 0x01;

  if (mode == 1) {
    P2OUT &= ~BIT0;
    for (i = 0; i < 12; i++) {
      __delay_cycles(F_CPU>>2);
      P2OUT ^= BIT0;
    }
    P2OUT &= ~BIT0;
  }
  else if (mode == 2) {
    P2OUT &= ~BIT0;
    for (i = 0; i < 12; i++) {
      __delay_cycles(F_CPU>>2);
      P2OUT ^= BIT0;
    }
  }

#else
#warning "no support"
#endif
}

//******************************************************************************
// 描述:应用程序，将PC转移到应用程序的复位向量处
// 输入:无  输出:无
void Application(void)
{
  asm(" mov &0xEFFE, PC;");                 // 在C中调用汇编指令,实现地址转移
}

//******************************************************************************
// 描述: 程序更新
// 输入: 无  输出:无
void UpdateFromUart(void)
{
  unsigned int i;
  unsigned char RecBuf[FRAME_LEN];
  unsigned char RecCnt=0;
  unsigned char RxTemp;
  unsigned long Addr=0;
  unsigned long ErasedBeg=0;
  unsigned long ErasedEnd=0;
  unsigned int  RCrc=0;
  unsigned int  CCrc=0;
  int r;

  msp430_cpu_init();
  InitUart();                               // 初始化串口
  UartSend("UpdateWait\n", 11);             // 发送更新请求
  ShowLed(1);

  /* 下面一段函数为串口数据接收和处理程序 */
  while(1)
  {
    if(UART(IFG) & UCRXIFG)
    {
      RxTemp = UART(RXBUF);

      if(!inPkt) {
        if(RxTemp==0x7E) {
          inPkt=1;
        }
        continue;
      }
      else{
        if(inEsc) {
          inEsc=0;
          RxTemp^=0x20;
          RecBuf[RecCnt++]=RxTemp;
          continue;
        } else {
          if(RxTemp==0x7D) {
            inEsc=1;
            continue;
          } else if (RxTemp==0x7E) {
            inPkt = 0;
          } else {
            RecBuf[RecCnt++]=RxTemp;
            continue;
          }
        }
      }

      //RecBuf[RecCnt++]=RxTemp;
      if(RecCnt>=FRAME_LEN)
      {
        TOG_LED_UART(); // 闪串口指示灯
        Addr = 0;
        for(i=0; i<4; i++) {
          Addr = (Addr<<8) + RecBuf[4+i];
        }
        RCrc = (RecBuf[FRAME_LEN-2]<<8) + RecBuf[FRAME_LEN-1];
        CCrc = crc16_data(RecBuf, FRAME_LEN-2, 0x0000);
        if (CCrc == RCrc) {
          if (Addr < ErasedBeg || Addr > ErasedEnd) {
            ErasedBeg = (Addr & 0xFFFFFE00);
            ErasedEnd = ErasedBeg + SEGMENT_SIZE - 1;
            EraseFlash(Addr);
          }
          r = WriteFlash(Addr, RecBuf+10, DATA_LEN);
          TOG_LED_FLASH(); // 闪写Flash指示灯
          SendAck(r, Addr);

          if(RecBuf[2]==0x80) { // 最后一个数据包
            boot.update=0; // 写入已更新标记
            boot.version = RecBuf[1];
            EraseFlash(BOOT_ADDR);
            WriteFlash(BOOT_ADDR, (unsigned char*)&boot, sizeof(struct BOOT));
            WDTCTL = 0; // 使用看门狗重启
          }
        } else {
          SendAck(2, Addr);
        }
        RecCnt=0;
        memset(RecBuf,0,sizeof(RecBuf));
      }
    }
  }
}

void UpdateFromFlash(void)
{
  uint32_t total;
  uint32_t waddr, raddr;
  uint16_t len;

  msp430_cpu_init();
  m25pe_init(boot.flash);
  InitUart();                               // 初始化串口
  UartSend("FlashUpdate\n", 12);
  UartSend((uint8_t*)&boot, sizeof(struct BOOT));
  ShowLed(2);

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
  total = boot.size;
  raddr = boot.addr;
  waddr = segs[segi].beg;
  while(total > 0) {
    len = MIN(total, SEGMENT_SIZE);
    m25pe_read(boot.flash, raddr, (uint8_t*)SegBuf, len);
    TOG_LED_EXTFLASH(); // 闪读取外部Flash指示灯

    EraseFlash(waddr);
    WriteFlash(waddr, SegBuf, len);
    TOG_LED_FLASH(); // 闪写Flash指示灯

    total -= len;
    raddr += len;
    waddr += len;

    if (total <= SEGMENT_SIZE) {
      if (waddr < 0xEE00) {
        waddr = 0xEE00;
      }
    }
    if (waddr >= segs[segi].end) {
      segi += 1;
      waddr = segs[segi].beg;
    }
  }

  boot.update = 0; // 写入已更新标记
  boot.version = boot.updver;
  EraseFlash(BOOT_ADDR);
  WriteFlash(BOOT_ADDR, (unsigned char*)&boot, sizeof(struct BOOT));
  WDTCTL = 0; // 使用看门狗重启
}
//******************************************************************************
// 描述: ASCALL码转换成字符
// 输入: unsigned char cNum  ASC-II字符码
// 输出: unsigned char HEX码
unsigned char AsciiToHex(unsigned char cNum)
{
  if(cNum>='0'&&cNum<='9')
  {
    cNum -= '0';
  }
  else if(cNum>='A'&&cNum<='F')
  {
    cNum -= 'A';
    cNum += 10;
  }
  return cNum;
}

char HexToAscii(unsigned char num)
{
  char c = 0;

  if (num <= 9) {
    c = '0' + num;
  }
  else if (num >= 10 && num <= 15){
    c = 'A' + (num - 10);
  }
  return c;
}
//******************************************************************************
#define UartPut(b) do { \
  while(!(UART(IFG) & UCTXIFG)); \
  UART(TXBUF) = (b); \
} while(0)

void UartSend(unsigned char *data, unsigned int len)
{
  unsigned int i;

  UartPut(0x7E);
  for(i=0; i<len; i++) {
    if (data[i]==0x7E||data[i]==0x7D) {
      UartPut(0x7D);
      UartPut(data[i]^0x20);
    } else {
      UartPut(data[i]);
    }
  }
  UartPut(0x7E);
}

void SendAck(uint8_t code, uint32_t addr)
{
  unsigned char AckBuf[6];
  uint8_t i = 0;

  AckBuf[i++] = code;
  AckBuf[i++] = ((addr>>24)&0xff);
  AckBuf[i++] = ((addr>>16)&0xff);
  AckBuf[i++] = ((addr>> 8)&0xff);
  AckBuf[i++] = ((addr>> 0)&0xff);

  UartSend(AckBuf, 5);
}
//******************************************************************************
// 描述: UART0初始化,9600位波特率/秒,8位字符,1位停止位,不校验.
// 输入: 无  输出: 无
// 说明一下,波特率9600,时钟源选择为ACLK.误码率可能会大些.如果用户需要可以将选择
// 其他高频时钟源.这点网友应该要自己修改.选择高频时钟会使波特率的误码率减低.
void InitUart(void)
{
#if BOARD_CADRE1120_AP || BOARD_CADRE1120_RP
  P5DIR &= ~BIT7;
  P5DIR |= BIT6;
  P5SEL |= BIT6 + BIT7;                       // P5.6,7 = USCI_A1 TXD/RXD
#elif BOARD_CADRE1120_VD
  P10DIR &= ~BIT5;
  P10DIR |= BIT4;
  P10SEL |= BIT4 + BIT5;                      // P10.4,5 = USCI_A3 TXD/RXD
#else
#error "no support"
#endif

  UART(CTL1) |= UCSWRST;
  UART(CTL1) |= UCSSEL_2;
  UART(BR0) = 0x8A;                           // 115200 @ 16MHz
  UART(BR1) = 0x00;
  UART(MCTL) = UCBRS_3;
  UART(IE) &= ~UCRXIFG;
  UART(IE) &= ~UCTXIFG;
  UART(CTL1) &= ~UCSWRST;                     // 初始化USART状态机
}

//******************************************************************************
// 描述: FLASH擦除操作
// 输入: 16位 FLASH地址
// 输出: 无
void EraseFlash(unsigned long waddr)
{
  //FCTL2 = FWKEY + FSSEL0+FN1;           // 选择DC0作为LFASH操作时钟源,MCLK/2
  FCTL3 = FWKEY;
  FCTL1 = FWKEY + ERASE;                // 擦除操作
  *(unsigned char*)waddr=0;             // 虚拟的擦除段操作
  while(FCTL3 & BUSY);
  FCTL3=FWKEY+LOCK;
}

//******************************************************************************
// 描述: FLASH写操作
// 输入: unsigned int addr 16位 FLASH地址, unsigned char *pdata 数据指针
// 输入: unsigned char length 数据长度
// 输出: unsigned char 错误标志
unsigned char WriteFlash(unsigned long addr,unsigned char *pdata,
    unsigned int length)
{
  unsigned char ErrorFlag = 0;
  unsigned int i;

  //FCTL2 = FWKEY + FSSEL0+FN1;
  FCTL3 = FWKEY;                           // 清除锁
  while(FCTL3 & BUSY);
  FCTL1 = FWKEY + WRT;                     // 设置WRT位为写操作
  for(i=0;i<length;i++)
  {
    *(unsigned char*)addr=*pdata;          // 写一个字节
    while(FCTL3 & BUSY);
    if(ReadFlashByte(addr)!=*pdata)            // 验证,写比较.正确或错误
    {
      ErrorFlag = 1;                       // 设置错误标志
      break;
    }
    addr++;pdata++;
  }
  FCTL1=FWKEY;
  FCTL3=FWKEY+LOCK;

  return ErrorFlag;
}

//******************************************************************************
// 描述: 读FLASH操作
// 输入: unsigned int waddr 16位地址
// 输出: unsigned char 返回一个字节数据
unsigned char ReadFlashByte(unsigned long waddr)
{
  unsigned char value;
  value = *(unsigned char*)waddr;
  return value;
}

int ReadFlash(unsigned long addr, unsigned char *buf, unsigned int len)
{
  unsigned int i;
  for (i = 0; i < len; i++)
    buf[i] = *(unsigned char*)(addr + i);
  return 0;
}

//******************************************************************************
// 描述: 中断向量列表
ISR(RTC, rtc_isr)
{
  asm(" br &0xEFD2;");
}

ISR(PORT2, port2_isr)
{
  asm(" br &0xEFD4;");
}

ISR(USCI_B3, usci_b3_isr)
{
  asm(" br &0xEFD6;");
}

ISR(USCI_A3, usci_a3_isr)
{
  asm(" br &0xEFD8;");
}

ISR(USCI_B1, usci_b1_isr)
{
  asm(" br &0xEFDA;");
}

ISR(USCI_A1, usci_a1_isr)
{
  asm(" br &0xEFDC;");
}

ISR(PORT1, port1_isr)
{
  asm(" br &0xEFDE;");
}

ISR(TIMER1_A1, tim1_a1_isr)
{
  asm(" br &0xEFE0;");
}

ISR(TIMER1_A0, tim1_a0_isr)
{
  asm(" br &0xEFE2;");
}

ISR(DMA, dma_isr)
{
  asm(" br &0xEFE4;");
}

ISR(USCI_B2, usci_b2_isr)
{
  asm(" br &0xEFE6;");
}

ISR(USCI_A2, usci_a2_isr)
{
  asm(" br &0xEFE8;");
}

ISR(TIMER0_A1, tim0_a1_isr)
{
  asm(" br &0xEFEA;");
}

ISR(TIMER0_A0, tim0_a0_isr)
{
  asm(" br &0xEFEC;");
}

ISR(ADC12, adc_isr)
{
  asm(" br &0xEFEE;");
}

ISR(USCI_B0, usci_b0_isr)
{
  asm(" br &0xEFF0;");
}

ISR(USCI_A0, usci_a0_isr)
{
  asm(" br &0xEFF2;");
}

ISR(WDT, wdt_isr)
{
  asm(" br &0xEFF4;");
}

ISR(TIMER0_B1, time0_b1_isr)
{
  asm(" br &0xEFF6;");
}

ISR(TIMER0_B0, time0_b0_isr)
{
  asm(" br &0xEFF8;");
}

ISR(UNMI, unmi_isr)
{
  asm(" br &0xEFFA;");
}

ISR(SYSNMI, sysnmi_isr)
{
  asm(" br &0xEFFC;");
}
