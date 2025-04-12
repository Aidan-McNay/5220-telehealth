// =======================================================================
// omron_test.cpp
// =======================================================================
// A quick demo to connect to the Omron measurement device

#include "ble/omron.h"
#include "utils/pt_cornell_rp2040_v1.h"

Omron blood_pressure;

// -----------------------------------------------------------------------
// print_characteristics
// -----------------------------------------------------------------------
// A thread to wait until characteristics are ready, then print them

static PT_THREAD( print_characteristics( struct pt *pt ) )
{
  PT_BEGIN( pt );

  // Wait until discovered
  while ( !blood_pressure.ready() ) {
    PT_YIELD_usec( 50000 );
  }

  // Print characteristics
  blood_pressure.print();
  PT_END( pt );
}

// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------

int main()
{
  stdio_init_all();
  blood_pressure.connect_to_server();

  pt_add_thread( print_characteristics );
  pt_schedule_start;
}