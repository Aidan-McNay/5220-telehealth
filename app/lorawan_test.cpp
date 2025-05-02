// =======================================================================
// lorawan_test.cpp
// =======================================================================
// A test for transmitting data with our LoRaWAN utilities

#include "lorawan/lorawan.h"
#include <stdio.h>

LoRaWAN my_lorawan;

int main()
{
  stdio_init_all();
  printf( "LoRaWAN Test\n" );
  sleep_ms( 10000 );

  lorawan_debug( true );

  printf( "Trying to join...\n" );
  while ( !my_lorawan.try_join() ) {
    lorawan_process_timeout_ms( 1000 );
  }

  uint8_t data[5] = { 0x69, 0x42, 0x00, 0x17, 0x38 };

  printf( "Trying to send...\n" );
  while ( !my_lorawan.try_send( data, 5 ) ) {
    lorawan_process_timeout_ms( 1000 );
  }
  printf( "Data sent!\n" );
}