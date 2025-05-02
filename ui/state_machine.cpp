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
    // status_led.on();
    
    transmissions_done_count = 0;
}

// -----------------------------------------------------------------------
// State Transitions
// -----------------------------------------------------------------------

fsm_state_t next_state(fsm_state_t curr_state, bool is_on, bool button_pressed)
{
    switch (curr_state)
    {
    case OFF:
        return is_on ? IDLE : OFF;
    case IDLE:
        return button_pressed ? START_MEASURE : IDLE;
    case START_MEASURE:
        // debug("START TO TAKE BP MEASUREMENT\n");
        // sleep_ms(1000); // Simulate measurement time
        return WAIT_MEASURE;
    case WAIT_MEASURE:
        // debug("HERE'S WHERE WAIT FOR MEASUREMENT TO BE TAKEN\n");
        // sleep_ms(5000); // Simulate measurement time
        //return button_pressed ? START_TRANSMIT : IDLE;
        return START_TRANSMIT;
    case START_TRANSMIT:
        // debug("START TO TRANSMIT DATA\n");
        // sleep_ms(1000); // Simulate transmission time
        return WAIT_TRANSMIT;
    case WAIT_TRANSMIT:
        // debug("HERE'S WHERE WAIT FOR DATA TO BE TRANSMITTED\n");
        // sleep_ms(5000); // Simulate transmission time
        return DONE;
    case DONE:
        // debug("DONE\n");
        // sleep_ms(1000); // Simulate done time
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
  // TODO: try moving these updates after the call to "next_state"
  // status_led.off();
  // error_led.on();

  on_switch.update();
  button.update();

  bool is_on = on_switch.is_flipped();
  bool button_pressed = button.just_released();

  fsm_state_t next = next_state(curr_state, is_on, button_pressed);
  //status_led.on();

  // if (button_pressed && curr_state == OFF) {
  //   status_led.on();
  //   status_led_on = true;
  // } else if (!button_pressed && curr_state != OFF) {
  //   sleep_ms(1000); // Simulate measurement time
  //   status_led.off();
  //   error_led.off();
  //   status_led_on = false;
  // }

  if (curr_state == OFF && next != OFF) {
    power_led.on();
    power_led_on = true;
  }
  // } else if (curr_state != OFF && next == OFF) {
  //   status_led.off();
  //   error_led.off();
  //   status_led_on = false;
  // }

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


