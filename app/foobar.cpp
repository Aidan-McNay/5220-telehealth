// =======================================================================
// foobar.cpp
// =======================================================================
// The top-level application for gathering and sending blood pressure
// data

#include "ui/state_machine.h"
#include "utils/pt_cornell_rp2040_v1.h"
#include <stdio.h>

#define BUTTON_GPIO 22
#define POWER_GPIO 2
#define STATUS_GPIO 3
#define ERROR_GPIO 4

FSM top( BUTTON_GPIO, STATUS_GPIO, ERROR_GPIO, POWER_GPIO );

static PT_THREAD( update_fsm( struct pt *pt ) )
{
  PT_BEGIN( pt );

  // Wait until discovered
  while ( 1 ) {
    top.update();
    lorawan_process_timeout_ms( 10 );
  }

  PT_END( pt )
}

int main( void )
{
  stdio_init_all();
  // Delay a bit to set up printf connection
  sleep_ms( 10000 );

#ifdef DEBUG
  lorawan_debug( true );
#endif

  printf( "Telehealth Communication\n" );

  pt_add_thread( update_fsm );
  pt_schedule_start;
}
