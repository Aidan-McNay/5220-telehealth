// =======================================================================
// button.cpp
// =======================================================================
// Implementation of a button

#include "ui/button.h"

// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------

Button::Button( int gpio_num ) : gpio_num( gpio_num )
{
  // TODO: Set up the GPIO
}

// -----------------------------------------------------------------------
// State Transitions
// -----------------------------------------------------------------------

button_state_t next_state( button_state_t curr_state, bool is_pressed )
{
  // TODO: Return the next state, based on our current state and whether
  // the button is pressed
  //
  // (Ideally use a switch-case statement)
}

// -----------------------------------------------------------------------
// Pressing
// -----------------------------------------------------------------------

bool Button::is_pressed()
{
  // TODO: Check the current GPIO level, transition state, and check if
  // the current state is "pressed"
  //
  // Use `next_state` for state transitions
}

bool Button::just_pressed()
{
  // TODO: Same as `is_pressed`, but only if the last state wasn't pressed
  //
  // Use `next_state` for state transitions
}