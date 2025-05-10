// =======================================================================
// state_machine.cpp
// =======================================================================

#include "ui/state_machine.h"

#include "pico/stdlib.h"
#include <stdio.h>
#include "utils/debug.h"


// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------

FSM::FSM( int switch_gpio, int button_gpio, int status_led_gpio, int error_led_gpio, int power_led_gpio) :
  on_switch(switch_gpio), button(button_gpio), 
  status_led(status_led_gpio), error_led(error_led_gpio), 
  power_led(power_led_gpio), ui_omron(), ui_lorawan(), curr_state(OFF)
{
    debug("FSM start\n");
    
    power_led.on();
    power_led_on = true;

    // TODO check this out
    // ui_omron.connect_to_server();
    
    transmissions_done_count = 0;
}

// -----------------------------------------------------------------------
// State Transitions
// -----------------------------------------------------------------------

fsm_state_t next_state(fsm_state_t curr_state, bool is_on, bool button_pressed, bool ready , bool omron_ready, bool lorawan_ready )
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
        return omron_ready ? START_TRANSMIT : WAIT_MEASURE;
    case START_TRANSMIT:
        return WAIT_TRANSMIT;
    case WAIT_TRANSMIT:
        return lorawan_ready ? DONE : WAIT_TRANSMIT;
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
  }

  bool ready = false;
  if (time >= 3000) { // Simulate measurement time
    time = 0;
    ready = true;
  } else {
    time += 5; // Increment time by the update interval (5 ms)
  }

  bool trying = false;
  bool lorawan_ready = false;
  const uint8_t* data = ui_omron.curr_data;
  // const omron_data_t* data = ui_omron.curr_data;
  uint8_t data_len = 5;

  bool omron_ready = ui_omron.omron_ready();
  

  if (trying) {
    bool lorawan_ready = ui_lorawan.try_send(data, data_len);
  }

  fsm_state_t next = next_state(curr_state, is_on, button_pressed, ready, omron_ready, lorawan_ready);

  if (next == IDLE) {
    ;
  }
  else if (next == START_MEASURE) {
    status_led.on();
    status_led_on = true;
    ui_omron.connect_to_server();
  }
  // else if (next == WAIT_MEASURE) {

  // }
  else if (next == START_TRANSMIT) {
    trying = true;
    status_led.blink(100);
    // ui_lorawan.try_send(data, data_len);
    ui_lorawan.try_join();
    status_led.blink(100);
    status_led.blink(100);
    status_led_on = true;
  }
  else if (next == DONE) {
    // if (!ready){
    //   status_led.blink(300);
    // }
    transmissions_done_count++;
    status_led.off();
    status_led_on = false;
  }

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


