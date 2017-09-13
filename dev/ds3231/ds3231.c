#include "contiki.h"
#include "ds3231.h"
#include "ds3231-arch.h"

/*--------------------------------------------------------*/
static void i2c_start(void);
static void i2c_stop(void);
static uint8_t i2c_write(uint8_t data);
static uint8_t i2c_read(void );
static void ds3231sn_write(uint8_t addr, uint8_t nums, uint8_t *buf);
static void ds3231sn_read(uint8_t addr,uint8_t nums,uint8_t *buf);

/*--------------------------------------------------------*/
void
ds3231_init(void)
{
  ds3231_time_t time = {0};

  time.year   = 0x16;
  time.month  = 0x01;
  time.day    = 0x01;
  time.hour   = 0x00;
  time.minute = 0x00;
  time.second = 0x00;

  ds3231sn_write(0x00, 7, (uint8_t*)&time);
}

void
ds3231_get_time(ds3231_time_t *time)
{
  ds3231sn_read(0x00, 7, (uint8_t*)time);
  time->reserve = 0x00;
}

void
ds3231_set_time(ds3231_time_t *time)
{
  ds3231sn_write(0x00, 7, (uint8_t*)time);
}

void
ds3231_str_time(char *buf)
{
  ds3231_time_t time;
  uint8_t i;

  ds3231_get_time(&time);

  i = 0;
  buf[i++] = '2';
  buf[i++] = '0';
  buf[i++] = '0' + (time.year >> 4);
  buf[i++] = '0' + (time.year & 0x0F);
  buf[i++] = '-';
  buf[i++] = '0' + (time.month >> 4);
  buf[i++] = '0' + (time.month & 0x0F);
  buf[i++] = '-';
  buf[i++] = '0' + (time.day >> 4);
  buf[i++] = '0' + (time.day & 0x0F);
  buf[i++] = ' ';
  buf[i++] = '0' + (time.hour >> 4);
  buf[i++] = '0' + (time.hour & 0x0F);
  buf[i++] = ':';
  buf[i++] = '0' + (time.minute >> 4);
  buf[i++] = '0' + (time.minute & 0x0F);
  buf[i++] = ':';
  buf[i++] = '0' + (time.second >> 4);
  buf[i++] = '0' + (time.second & 0x0F);
  buf[i++] = '\0';
}

/*--------------------------------------------------------*/
static void
i2c_start(void)
{
  CLR_SCL;
  CLR_SDA;
  delay_us(5);
  SET_SDA;
  delay_us(1);
  SET_SCL;
  delay_us(2);
  CLR_SDA;
  delay_us(1);
  CLR_SCL;
  delay_us(1);
}

static void
i2c_stop(void)
{
  CLR_SCL;
  CLR_SDA;
  delay_us(5);
  SET_SCL;
  delay_us(1);
  SET_SDA;
  delay_us(1);
}

static uint8_t
i2c_write(uint8_t data)
{
  uint16_t i;

  for(i=0;i<8;i++)
  {
    CLR_SCL;
    delay_us(1);
    if(data&BIT7)
    {
      SET_SDA;
    }
    else
    {
      CLR_SDA;
    }
    delay_us(1);
    SET_SCL;
    delay_us(1);
    data<<=1;
  }
  CLR_SCL;
  delay_us(1);
  SET_SCL;
  for(i=0 ; i<433; i++)
  {
    if( SDA_IN_DAT==0 )
    {
      return 1; // write succeed
    }
  }
  return 0; // write failed
}

static uint8_t
i2c_read(void )
{
  uint8_t data,i;
  SDA_IN_P;
  data=0;
  for(i=0;i<8;i++)
  {
    CLR_SCL;
    delay_us(1);
    SET_SCL;
    delay_us(1);
    data<<=1;
    if(SDA_IN_DAT)
    {
      data|=BIT0;
    }
    else
    {
      data&=~BIT0;
    }
  }
  CLR_SCL;
  delay_us(1);
  return data;
}

static void
ds3231sn_write(uint8_t addr, uint8_t nums, uint8_t *buf)
{
  uint8_t i;
  if(nums==0)
  {
    return;
  }
  i2c_start();

  if(i2c_write(DS3231_ADDR_WRITE))
  {
    if(i2c_write(addr))
    {
      for(i=0;i<nums;i++)
      {
        if(i2c_write(buf[i])==0)
        {
          break;
        }
      }
    }
  }
  i2c_stop();
  return;
}

static void
ds3231sn_read(uint8_t addr,uint8_t nums,uint8_t *buf)
{
  uint8_t i=0;
  i2c_start();
  if (i2c_write(DS3231_ADDR_WRITE))
  {
    if(i2c_write(addr))
    {
      i2c_stop();
      delay_us(1);
      i2c_start();
      if(i2c_write(DS3231_ADDR_READ))
      {
        while(1)
        {
          buf[i]=i2c_read();
          i++;
          nums--;
          if(nums==0)
          {
            SET_SDA;
            delay_us(1);
            SET_SCL;
            delay_us(1);
            CLR_SCL;
            delay_us(1);
            break;
          }
          CLR_SDA;
          delay_us(1);
          SET_SCL;
          delay_us(1);
          CLR_SCL;
          delay_us(1);
        }
      }
    }
  }
  i2c_stop();
  return;
}
