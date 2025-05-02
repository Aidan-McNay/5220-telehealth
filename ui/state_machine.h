// =======================================================================
// state_machine.h
// =======================================================================
// Declarations of our User Interface state machine

#ifndef UI_STATE_MACHINE_H
#define UI_STATE_MACHINE_H

#include "ui/switch.h"
#include "ui/button.h"
#include "ui/LED_hw.h"

// -----------------------------------------------------------------------
// State Machine States
// -----------------------------------------------------------------------

enum fsm_state_t {
  OFF = 0,          // If machine is off
  IDLE,             // If machine is on, wait until button press
  START_MEASURE,    // If button is pressed, start to take blood pressure measurement
  HARDWARE_ERROR,   // If device cannot detect cuff
  WAIT_MEASURE,     // Wait for blood pressure measurement to be taken
  START_TRANSMIT,   // When measurement is complete, start to transmit data
  WAIT_TRANSMIT,    // Wait for data to be transmitted
  DATA_ERROR,       // If data transmission fails for any reason
  DONE              // After data transmission is complete, done state before return to IDLE
};


class FSM {
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Public accessor functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 public:
  FSM ( int switch_gpio, int button_gpio, int status_led_gpio, int error_led_gpio, int power_led_gpio );
  void update();
  bool is_running() const;
  bool is_measuring() const;
  bool is_transmitting() const;
  bool is_done() const;
  int  transmissions_done() const;

 private:
  Switch on_switch;     // GPIO pin number for the switch
  Button button;        // GPIO pin number for the button
  LED_hw status_led;    // GPIO pin number for the first LED
  LED_hw error_led;     // GPIO pin number for the second LED
  LED_hw power_led;     // GPIO pin number for the power LED
  fsm_state_t curr_state = OFF;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Protected attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  protected:
  bool switch_on = false;
  bool button_pressed = false;
  bool status_led_on = false;
  bool power_led_on = false;
  bool error_led_on = false;
  int transmissions_done_count = 0;
};

#endif  // UI_STATE_MACHINE_H