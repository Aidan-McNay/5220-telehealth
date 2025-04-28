// =======================================================================
// omron_test.cpp
// =======================================================================
// A quick demo to connect to the Omron measurement device

#include "ble/omron.h"
#include "hci_dump_embedded_stdout.h"
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
  while ( !blood_pressure.omron_ready() ) {
    first_time = true;
    PT_YIELD_usec( 500000 );
    printf( "Waiting...\n" );
  }

  // Print data
  if ( first_time ) {
    first_time = false;
    printf( "Blood Pressure Data:\n" );
    printf( " - Systolic Pressure: %d\n",
            blood_pressure.curr_data.sys_pressure );
    printf( " - Diastolic Pressure: %d\n",
            blood_pressure.curr_data.dia_pressure );
    printf( " - BPM: %d\n", blood_pressure.curr_data.bpm );
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
  // hci_dump_init( hci_dump_embedded_stdout_get_instance() );
  att_db_util_init();
  blood_pressure.connect_to_server();

  pt_add_thread( print_characteristics );
  pt_schedule_start;
}