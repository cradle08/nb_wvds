#include "spi.h"

void spi_init(void)
{
  SPI_PORT(DIR) |= (MOSI_PIN + SCK_PIN + W_PIN + CS_PIN);
  SPI_PORT(DIR) &= ~(MISO_PIN);
  SPI_PORT(REN) &= ~(MISO_PIN);
  
  TSL_HIGH;
  CS_HIGH;
  SCK_LOW;
}


void m25pe_write_byte(u8 data)
{
  u8 num=0; 
  for(num=0;num<8;num++)
  {
    if(data & 0x80)
      MOSI_HIGH;          
    else
      MOSI_LOW;                
     
    SCK_LOW;
    _NOP(); 
    SCK_HIGH;      
  
    data<<=1;          
  } 
}

u8 m25pe_read_byte(void)
{
  u8 num=0;
  u8 data;
  
  for(num=0;num<8;num++)
  {
    data <<= 1;
    SCK_HIGH;  
    _NOP();
    _NOP();
    SCK_LOW;
    if(MISO_HIGH)
      data |= 0x01;
  } 
  return data;
}

 

