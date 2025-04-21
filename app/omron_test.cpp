// =======================================================================
// omron_test.cpp
// =======================================================================
// A quick demo to connect to the Omron measurement device

#include "ble/omron.h"
#include "utils/pt_cornell_rp2040_v1.h"
#include <stdio.h>

Omron blood_pressure;

// -----------------------------------------------------------------------
// print_characteristics
// -----------------------------------------------------------------------
// A thread to wait until characteristics are ready, then print them

bool first_time = true;

static PT_THREAD( print_characteristics( struct pt *pt ) )
{
  PT_BEGIN( pt );

  // Wait until discovered
  while ( !blood_pressure.ready() ) {
    PT_YIELD_usec( 500000 );
    printf( "Waiting...\n" );
  }

  // Print characteristics
  if ( first_time ) {
    first_time = false;
    blood_pressure.print();
  }
  PT_END( pt );
}

// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------

void my_hci_reset()
{
}
void my_hci_log_packet( uint8_t packet_type, uint8_t in, uint8_t *packet,
                        uint16_t len )
{
}
void my_hci_log_info( int log_level, const char *format, va_list argptr )
{
  vprintf( format, argptr );
  printf( "\n" );
}

int main()
{
  stdio_init_all();
  printf( "OMRON Communication Test\n" );

  // Delay a bit to set up printf connection
  sleep_ms( 10000 );
  hci_dump_t dump_config = { .reset       = my_hci_reset,
                             .log_packet  = my_hci_log_packet,
                             .log_message = my_hci_log_info };
  hci_dump_init( &dump_config );
  att_db_util_init();
  blood_pressure.connect_to_server();

  pt_add_thread( print_characteristics );
  pt_schedule_start;
}