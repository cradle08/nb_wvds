/****** implementation File for support of STFL-I based Serial Flash Memory Driver *****

   Filename:    Serialize.c
   Description:  Support to c2076.c. This files is aimed at giving a basic
    example of the SPI serial interface used to communicate with STMicroelectronics
    serial Flash devices. The functions below are used in an environment where the
    master has an embedded SPI port (STMicroelectronics µPSD).

   Version:     1.0
   Date:        08-11-2004
   Authors:    Tan Zhi, STMicroelectronics, Shanghai (China)
   Copyright (c) 2004 STMicroelectronics.

   THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS WITH
   CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME. AS A
   RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT OR
   CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT OF SUCH
   SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION CONTAINED HEREIN
   IN CONNECTION WITH THEIR PRODUCTS.
********************************************************************************

   Version History.
   Ver.   Date      Comments

   1.0   08/11/2004   Initial release

*******************************************************************************/
#include "Serialize.h"
#include "driverlib.h"
#include "contiki.h"
#include "m25pe-arch.h"

extern int m25pe_dev;
/*******************************************************************************
Function:     InitSPIMaster(void)
Arguments:
Return Values:There is no return value for this function.
Description:  This function is a one-time configuration for the CPU to set some
              ports to work in SPI mode (when they have multiple functions. For
              example, in some CPUs, the ports can be GPIO pins or SPI pins if
              properly configured).
              please refer to the specific CPU datasheet for proper
              configurations.
*******************************************************************************/
void InitSPIMaster(void)
{
}

/*******************************************************************************
Function:     ConfigureSpiMaster(SpiMasterConfigOptions opt)
Arguments:    opt configuration options, all acceptable values are enumerated in
              SpiMasterConfigOptions, which is a typedefed enum.
Return Values:There is no return value for this function.
Description:  This function can be used to properly configure the SPI master
              before and after the transfer/receive operation
Pseudo Code:
   Step 1  : perform or skip select/deselect slave
   Step 2  : perform or skip enable/disable transfer
   Step 3  : perform or skip enable/disable receive
*******************************************************************************/
void ConfigureSpiMaster(SpiMasterConfigOptions opt)
{
  if(enumNull == opt) {
    return;
  }

  if((int)opt & (int)MaskBit_SelectSlave_Relevant) {
    if ((int)opt & (int)MaskBit_SlaveSelect) {
      M25PE_SELECT(m25pe_dev);
    } else {
      M25PE_DESELECT(m25pe_dev);
    }
  }
}

/*******************************************************************************
Function:     Serialize(const CharStream* char_stream_send,
              CharStream* char_stream_recv,
              SpiMasterConfigOptions optBefore,
              SpiMasterConfigOptions optAfter
              )
Arguments:    char_stream_send, the char stream to be sent from the SPI master to
              the Flash memory, usually contains instruction, address, and data to be
              programmed.
              char_stream_recv, the char stream to be received from the Flash memory
              to the SPI master, usually contains data to be read from the memory.
              optBefore, configurations of the SPI master before any transfer/receive
              optAfter, configurations of the SPI after any transfer/receive
Return Values:TRUE
Description:  This function can be used to encapsulate a complete transfer/receive
              operation
Pseudo Code:
   Step 1  : perform pre-transfer configuration
   Step 2  : perform transfer/ receive
    Step 2-1: transfer ...
        (a typical process, it may vary with the specific CPU)
        Step 2-1-1:  check until the SPI master is available
        Step 2-1-2:  send the byte stream cycle after cycle. it usually involves:
                     a) checking until the transfer-data-register is ready
                     b) filling the register with a new byte
    Step 2-2: receive ...
        (a typical process, it may vary with the specific CPU)
        Step 2-2-1:  Execute ONE pre-read cycle to clear the receive-data-register.
        Step 2-2-2:  receive the byte stream cycle after cycle. it usually involves:
                     a) triggering a dummy cycle
                     b) checking until the transfer-data-register is ready(full)
                     c) reading the transfer-data-register
   Step 3  : perform post-transfer configuration
*******************************************************************************/
Bool Serialize(const CharStream* char_stream_send,
               CharStream* char_stream_recv,
               SpiMasterConfigOptions optBefore,
               SpiMasterConfigOptions optAfter
               )
{
  unsigned char* pChar;
  ST_uint32 length;
  ST_uint32 i;
  ST_uint8  c;

  ConfigureSpiMaster(optBefore);

  length = char_stream_send->length;
  pChar  = char_stream_send->pChar;
  for (i = 0; i < length; i++) {
    M25PE_SPI_WAITFORTx_BEFORE();
    M25PE_SPI_TXBUF = *(pChar++);
    M25PE_SPI_WAITFOREOTx();
    M25PE_SPI_WAITFOREORx();
    c = M25PE_SPI_RXBUF;
  }

  if(ptrNull != (void*)char_stream_recv) {
    length = char_stream_recv->length;
    pChar  = char_stream_recv->pChar;
    for(i = 0; i < length; ++i) {
      M25PE_SPI_WAITFORTx_BEFORE();
      M25PE_SPI_TXBUF = 0x00;
      M25PE_SPI_WAITFOREOTx();
      M25PE_SPI_WAITFOREORx();
      *(pChar++) = M25PE_SPI_RXBUF;
    }
  }

  ConfigureSpiMaster(optAfter);

  return TRUE;
}
