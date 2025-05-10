// =======================================================================
// button.cpp
// =======================================================================
// Implementation of a button

#include "pico/stdlib.h"
#include "ui/button.h"
#include "utils/debug.h"
#include <stdio.h>

// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------

Button::Button( int gpio_num )
    : gpio_num( gpio_num ), curr_state( NotPressed )
{
  debug( "[Button] Initializing button with gpio %d...\n", gpio_num );
  gpio_init( gpio_num );
  gpio_set_dir( gpio_num, GPIO_IN );
  gpio_pull_up( gpio_num );  // Enable internal pull-up resistor
}

// -----------------------------------------------------------------------
// State Transitions
// -----------------------------------------------------------------------

button_state_t next_state( button_state_t curr_state, bool is_pressed )
{
  switch ( curr_state ) {
    case NotPressed:
      return is_pressed ? MaybePressed : NotPressed;

    case MaybePressed:
      return is_pressed ? Pressed : NotPressed;

    case Pressed:
      return is_pressed ? Pressed : MaybeNotPressed;

    case MaybeNotPressed:
      return is_pressed ? Pressed : NotPressed;

    default:
      return NotPressed;
  }
}

// -----------------------------------------------------------------------
// Pressing
// -----------------------------------------------------------------------

void Button::update()
{
  bool           physical_pressed = gpio_get( gpio_num ) == 0;
  button_state_t next = next_state( curr_state, physical_pressed );

  justPressed  = ( ( curr_state == MaybePressed ) && next == Pressed );
  justReleased = ( ( curr_state == Pressed ) && next == MaybeNotPressed );

  curr_state = next;
}

bool Button::is_pressed() const
{
  return curr_state == Pressed;
}

bool Button::just_pressed() const
{
  return justPressed;
}

bool Button::just_released() const
{
  return justReleased;
}

bool Button::is_released() const
{
  return curr_state == NotPressed;
}