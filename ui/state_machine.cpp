// =======================================================================
// state_machine.cpp
// =======================================================================

#include "ui/state_machine.h"
#include "ui/switch.h"
#include "ui/button.h"
#include "ui/LED_hw.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "utils/debug.h"


// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------

FSM::FSM( int switch_gpio, int button_gpio, int status_led_gpio, int error_led_gpio, int power_led_gpio) :
  on_switch(switch_gpio), button(button_gpio), 
  status_led(status_led_gpio), error_led(error_led_gpio), 
  power_led(power_led_gpio), curr_state(OFF)
{
    debug("FSM start\n");
    
    power_led.on();
    power_led_on = true;
    
    transmissions_done_count = 0;
}

// -----------------------------------------------------------------------
// State Transitions
// -----------------------------------------------------------------------

fsm_state_t next_state(fsm_state_t curr_state, bool is_on, bool button_pressed, bool ready )
{
    switch (curr_state)
    {
    case OFF:
        return is_on ? IDLE : OFF;
    case IDLE:
        return button_pressed ? START_MEASURE : IDLE;
    case START_MEASURE:
        return WAIT_MEASURE;
    case WAIT_MEASURE:
        return ready ? START_TRANSMIT : WAIT_MEASURE;
        //return START_TRANSMIT;
    case START_TRANSMIT:
        return WAIT_TRANSMIT;
    case WAIT_TRANSMIT:
        return ready ? DONE : WAIT_TRANSMIT;
        // return DONE;
    case DONE:
        return IDLE;
    default:
        return IDLE;
    }
}

// -----------------------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------------------

void FSM::update()
{
  button.update();

  bool is_on = true;

  bool button_was_pressed;
  bool button_is_pressed = false;

  if (button.is_released()) {
    button_was_pressed = true;
  }
  else if (button.just_pressed()) {
    button_is_pressed = true;
  }

  bool button_pressed = button_was_pressed && button_is_pressed;

  if (time_since_start < 300) {
    time_since_start += 5;
    button_pressed = false; // Let start without a phantom button press from getting setup
  }
  if (button_pressed) {
    button_was_pressed = false;
    button_is_pressed = false;
    // status_led.on();
    // sleep_ms(100);
  }
  // else {
  //   status_led.off();
  // }

  bool ready = false;
  if (time >= 3000) { // Simulate measurement time
    time = 0;
    ready = true;
  } else {
    time += 5; // Increment time by the update interval (5 ms)
  }

  // time_since_start += 5; // Increment time since start by the update interval (5 ms)

  // if (ready) {
  //   power_led.blink(100);
  //   power_led.blink(100);
  //   power_led.blink(100);
  // }

  fsm_state_t next = next_state(curr_state, is_on, button_pressed, ready);

  if (next == IDLE) {
    ;
  }
  else if (next == START_MEASURE) {
    status_led.on();
    status_led_on = true;
  }
  else if (next == START_TRANSMIT) {
    status_led.off();
    // status_led.blink(300);
    status_led_on = true;
  }
  else if (next == DONE) {
    if (!ready){
      status_led.blink(300);
    }
    transmissions_done_count++;
    status_led.off();
    status_led_on = false;
  }

  // if (curr_state == IDLE && next == START_MEASURE) {
  //   status_led.on();
  //   status_led_on = true;
  // } else if (curr_state == START_MEASURE && next == WAIT_MEASURE) {
  //   status_led.on();
  //   status_led_on = true;
  //   sleep_ms(1000); // Simulate measurement time
  // } else if (curr_state == WAIT_MEASURE && next == START_TRANSMIT) {
  //   status_led.blink(300);
  //   status_led_on = true;
  // } else if (curr_state == START_TRANSMIT && next == WAIT_TRANSMIT) {
  //   status_led.blink(300);
  //   status_led_on = true;
  // } else if (curr_state == WAIT_TRANSMIT && next == DONE) {
  //   status_led.off();
  //   status_led_on = false;
  // } else if (curr_state == DONE && next == IDLE) {
  //   transmissions_done_count++;
  //   status_led.off();
  //   status_led_on = false;
  // }

  curr_state = next;
}

bool FSM::is_running() const
{
  return curr_state != OFF;
}

bool FSM::is_measuring() const
{
  return curr_state == START_MEASURE || curr_state == WAIT_MEASURE;
}

bool FSM::is_transmitting() const
{
  return curr_state == START_TRANSMIT || curr_state == WAIT_TRANSMIT;
}

bool FSM::is_done() const
{
  return curr_state == DONE;
}

int FSM::transmissions_done() const
{
  return transmissions_done_count;
}


