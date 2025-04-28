// =======================================================================
// switch.cpp
// =======================================================================
// Implementation of a switch

#include "ui/switch.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "utils/debug.h"

// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------

Switch::Switch(int gpio_num) : gpio_num(gpio_num), curr_state(NotFlipped)
{
    debug("Switch start\n");
    stdio_init_all();
    printf("Switch GPIO %d\n", gpio_num);

    gpio_init(gpio_num);
    gpio_set_dir(gpio_num, GPIO_IN);
    gpio_pull_up(gpio_num); // Enable internal pull-up resistor
    debug("Switch end\n");
}

// -----------------------------------------------------------------------
// State Transitions
// -----------------------------------------------------------------------

switch_state_t next_state(switch_state_t curr_state, bool is_flipped)
{
    switch (curr_state)
    {
    case NotFlipped:
        return is_flipped ? MaybeFlipped : NotFlipped;

    case MaybeFlipped:
        return is_flipped ? Flipped : NotFlipped;

    case Flipped:
        return is_flipped ? Flipped : MaybeNotFlipped;

    case MaybeNotFlipped:
        return is_flipped ? Flipped : NotFlipped;

    default:
        return NotFlipped;
    }
}

// -----------------------------------------------------------------------
// Pressing
// -----------------------------------------------------------------------

void Switch::update()
{
  bool physical_flipped = gpio_get(gpio_num) == 0;
  switch_state_t next = next_state(curr_state, physical_flipped);

  justFlipped = ((curr_state == MaybeFlipped) && next == Flipped);
  justUnflipped = ((curr_state == Flipped) && next == MaybeNotFlipped);

  curr_state = next;
}

bool Switch::is_flipped() const
{
  return curr_state == Flipped;
}

bool Switch::just_flipped() const
{
  return justFlipped;
}

bool Switch::just_unflipped() const
{
  return justUnflipped;
}