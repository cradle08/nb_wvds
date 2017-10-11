
//#include "io430.h"


/*
int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  return 0;
}
*/

/* QMC ALGO TEST */
#include "system.h"
#include "qmc5883.h"
#include "uart.h"
#include "app.h"
#include "VehicleDetection.h"
#include "timer.h"

unsigned short seq = 0;
unsigned char pk_status = 3;
uint16_t sample_time;
extern struct Sample_Struct One_Sample;



void app_send_msg()
{
  int8_t buf[40] = {0};
  if(seq >= 9999) seq = 1;
  sprintf(buf, "No=%d: x=%d,y=%d,z=%d,ps=%d", seq++, One_Sample.x, One_Sample.y, One_Sample.z, pk_status);
  uart1_Tx(40, buf);
 // uart1_Tx(5, "12345");
}



void test_check()
{
  
  uart1_Tx(strlen("hello world\r\n"), "hello world\r\n");

}

void main()
{
 // sampleT = 1120;
  msp430_cpu_init();
  int8_t aaa = 23;
  int8_t ttt = -23;
  int8_t aa = 0xAA;
  int8_t tt = 0x33;
  TA0_init();

  uart1_init(9600);
  qmc5883_init();
  qmc5883_set_callback(app_magdata_ready); 
  Variant_Init();
  // board test
  while(1)
  { 
    uint16_t i = 0;
      // sm_time = Get_AMR_Peroid(); // get sample peroid 
    //timer_create(&timer_mag,5,OPT_TMR_PERIODIC,,0);
    sample_time = Get_AMR_Period();
    qmc5883_sample_read(0);
    pk_status = Parking_Algorithm();

    app_send_msg();
    for(i = 0; i < sample_time; i++)
    {
      __delay_cycles(1000);
    }
  }
  
  
  
  
  /*
  // timer test
  timer_t timer_mag;
  timer_t test;
  _EINT();
  timer_create(&timer_mag,140,OPT_TMR_PERIODIC, parking_check);
  timer_create(&test,150,OPT_TMR_PERIODIC, test_check);
  timer_start(&timer_mag);
  timer_start(&test);
  while(1)
  {    
    timer_ev_poll();
//    __bis_SR_register(LPM4_bits + GIE);
//    SystemLED_FLA
//    delay_ms(100);
//    SystemLED_FLA
//    delay_ms(100);
  }
 */
  
  
  

  
  
}




