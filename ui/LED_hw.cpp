// =======================================================================
// LED.cpp
// =======================================================================
// Implementation of LED controls

#include "pico/stdlib.h"
#include "ui/LED_hw.h"
#include "utils/debug.h"
#include <stdio.h>

// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------

LED_hw::LED_hw( int gpio_num ) : gpio_num( gpio_num ), blinking( false )
{
  debug( "[LED] Initializing LED with GPIO %d...\n", gpio_num );

  gpio_init( gpio_num );
  gpio_set_dir( gpio_num, GPIO_OUT );
  gpio_put( gpio_num, 0 );  // Start with LED off
}

// -----------------------------------------------------------------------
//  Helper Functions
// -----------------------------------------------------------------------

void LED_hw::on()
{
  blinking = false;
  state    = true;
  gpio_put( gpio_num, 1 );  // Turn on the LED
}

void LED_hw::off()
{
  blinking = false;
  state    = false;
  gpio_put( gpio_num, 0 );  // Turn off the LED
}

void LED_hw::blink( int duration_ms )
{
  if ( !blinking ) {
    // Just started
    last_blink_toggle = to_ms_since_boot( get_absolute_time() );
    blinking          = true;
  }

  uint32_t curr_time = to_ms_since_boot( get_absolute_time() );
  if ( curr_time - last_blink_toggle >= duration_ms ) {
    last_blink_toggle = curr_time;
    state             = !state;
  }
  gpio_put( gpio_num, (int) state );
}

bool LED_hw::is_on()
{
  return state;
}
