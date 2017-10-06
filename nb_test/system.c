#include "system.h"

static void port_init(void);
static void port_mapping(void); 
static void systemled_init(void);
/*================================================================
【名 称】void msp430_cpu_init(void)
【功 能】端口初始化设置
【备 注】所有端口设置为普通输入
================================================================*/
void msp430_cpu_init(void)
{
  WDTCTL = WDTPW | WDTHOLD; 
  port_init();
  port_mapping();
  osc_init();
  systemled_init();
  //watchdog_init(); // 看门狗定时器
}

/*================================================================
【名 称】static void port_init(void)
【功 能】端口初始化设置
【备 注】所有端口设置为普通输入
================================================================*/
static void port_init(void)
{	
  /* Turn everything off, device drivers enable what is needed. */
  /* All configured for digital I/O */
  P1SEL = 0;
  P2SEL = 0;
  P4SEL = 0;
  P5SEL = 0;
  P6SEL = 0;
  /* All available inputs */
  P1DIR = 0;
  P1OUT = 0;
  P1REN = 0XFF;
  P1IES = 0;
  
  P2DIR = 0;
  P2REN = 0XFF;
  P2OUT = 0;
  
  P4DIR = 0;                            // P4.0 - P4.7 output
  P4REN = 0XFF;
  P4OUT = 0;                            // P4.0 - P4.6 Port Map functions
  
  P5DIR = 0;
  P5REN = 0XFF;
  P5OUT = 0;
  
  P6DIR = 0;
  P6REN = 0XFF;
  P6OUT = 0;
  
  PJDIR = 0XFF;
  PJREN = 0XFF;
  PJOUT = 0;

  P1IE = 0;
  P2IE = 0;
}

static void port_mapping(void)
{ 
	_DINT();
  PMAPPWD = 0x02D52;                            
  PMAPCTL = PMAPRECFG;                      
  //I2C
  P4MAP1 = PM_UCB1SDA;//PM_UCB1SDA; 
  P4MAP2 = PM_UCB1SCL;//PM_UCB1SCL;
  // UART	
  P4MAP4 = PM_UCA1TXD;//PM_UCA1TXD;
  P4MAP5 = PM_UCA1RXD;//PM_UCA1RXD;

  PMAPPWD = 0;                   

}


/*================================================================
【名 称】void osc_init(void)
【功 能】时钟设置
【备 注】主时钟1M，外设32.768
================================================================*/
void osc_init(void)  
{
  //-------DISABLE WDT AND GENERAAL INTERRUPT--------------------
  _DINT();
  __delay_cycles(235); 
  //--------CHOOSE XT1 AND XT2-----------------------     
  P5SEL |=0x30;
  // SET XT1 AND XT2 ON
  UCSCTL6 &= ~XT1OFF;   
  //XT1 WORKING UNDER LOW FREQUENCY
  UCSCTL6 &=~XTS;
  //CAPCITOR CHOOSE
  UCSCTL6 |=XCAP_3;    
  // WAIT UNTIL STABLIZATION
  do
  {
    UCSCTL7 &= ~(XT1LFOFFG  + DCOFFG);
    UCSCTL7 &=~(XT1LFOFFG);
    _NOP();
                                                                                                                                                                     // Clear XT2,XT1,DCO fault flags
    SFRIFG1 &= ~OFIFG;                       // Clear fault flags
  }
  while (SFRIFG1&OFIFG);                   // Test oscillator fault flag
  //CHOOSE ACK SCLK MCLK CLOCK SOURCE :ACK=32K; SCLK=AROUND 1 M; MCLK=ROUND 8M                
  UCSCTL4 |= SELA_0+SELS_5+SELM_5;          //xt1 for aclk, xt2 for sclk and mclk
  UCSCTL5 |=0;                         //0 divided  frequency   
  UCSCTL1 |=DCORSEL_0;                      //DCO operating frequency in the lowest frequency 

  return;
}


/*================================================================
【名 称】void systemled_init(void)
【功 能】LED初始化
【备 注】
================================================================*/

static void systemled_init(void)
{
  P1DIR |= BIT2;
  SystemLED_OFF
}


/*================================================================
【名 称】void ADC_init(void)
【功 能】ADC初始化
【备 注】
================================================================*/
u16 ADC_valu[5];

void ADC_DMA_init(void)
{
  P6SEL |= BIT1;
  
  // Configure ADC10 - pulse sample mode; software trigger; 
  ADC10CTL0 = ADC10SHT_2 | ADC10ON | ADC10MSC; // 16ADCclks, ADC on
  ADC10CTL1 = ADC10SSEL0 |ADC10SHP | ADC10CONSEQ_2;     //ACLK pulse sample mode, rpt single ch
  ADC10CTL2 = ADC10RES;                     // 10-bits of resolution
  ADC10MCTL0 = ADC10INCH_1;                 // AVCC ref, A1
  
  // Configure DMA (ADC10IFG trigger)
  DMACTL0 = DMA0TSEL_24;                    // ADC10IFG trigger
  __data16_write_addr((unsigned short) &DMA0SA,(unsigned long) &ADC10MEM0);
                                            // Source single address  
  __data16_write_addr((unsigned short) &DMA0DA,(unsigned long) &ADC_valu[0]);
                                            // Destination array address  
  DMA0SZ = 5;                              // 64 conversions 
  DMA0CTL = DMADT_4 | DMADSTINCR_3 | DMAEN | DMAIE; // Rpt, inc dest, word access, 
                                            // enable int after 64 conversions  
}

u16 get_bat_valu(void)
{
  u8 i;
  u16 Bat_valu=0;
  ADC10CTL0 |= ADC10ENC | ADC10SC;//开户转换
  __bis_SR_register(CPUOFF | GIE);        // LPM0, ADC10_ISR will force exit
  for(i=0;i<5;i++)
  {
    Bat_valu += ADC_valu[i];
  }
  Bat_valu = Bat_valu/5;
  
  Bat_valu=(((Bat_valu*100)/1024)*33*2)/10;
  return Bat_valu;
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=DMA_VECTOR
__interrupt void DMA0_ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(DMA_VECTOR))) DMA0_ISR (void)
#else
#error Compiler not supported!
#endif
{
  switch(__even_in_range(DMAIV,16))
  {
    case  0: break;                         // No interrupt
    case  2: 
      // 64 conversions complete
      ADC10CTL0 &= ~ADC10ENC;
      __bic_SR_register_on_exit(CPUOFF);    // exit LPM
      break;                                // DMA0IFG
    case  4: break;                         // DMA1IFG
    case  6: break;                         // DMA2IFG
    case  8: break;                         // Reserved
    case 10: break;                         // Reserved
    case 12: break;                         // Reserved
    case 14: break;                         // Reserved
    case 16: break;                         // Reserved
    default: break; 
  }   
}









