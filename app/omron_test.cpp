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

int main()
{
  stdio_init_all();
  printf( "OMRON Communication Test\n" );

  // Delay a bit to set up printf connection
  sleep_ms( 10000 );
  blood_pressure.connect_to_server();

  pt_add_thread( print_characteristics );
  pt_schedule_start;
}