// =======================================================================
// state_machine.h
// =======================================================================
// Declarations of our User Interface state machine

#ifndef UI_STATE_MACHINE_H
#define UI_STATE_MACHINE_H

#include "ble/omron.h"
#include "encryption/encryption.h"
#include "lorawan/lorawan.h"
#include "ui/LED_hw.h"
#include "ui/button.h"
#include "ui/switch.h"

// -----------------------------------------------------------------------
// State Machine States
// -----------------------------------------------------------------------

enum fsm_state_t {
  IDLE,            // If machine is on, wait until button press
  START_MEASURE,   // If button is pressed, start to take blood pressure
                   // measurement
  WAIT_MEASURE,    // Wait for blood pressure measurement to be taken
  START_TRANSMIT,  // When measurement is complete, start to transmit data
  WAIT_TRANSMIT,   // Wait for data to be transmitted
  DONE  // After data transmission is complete, done state before return
        // to IDLE
};

class FSM {
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Public accessor functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 public:
  FSM( int button_gpio, int status_led_gpio, int error_led_gpio,
       int power_led_gpio );
  void update();

 private:
  Button      button;      // GPIO pin number for the button
  LED_hw      status_led;  // GPIO pin number for the first LED
  LED_hw      error_led;   // GPIO pin number for the second LED
  LED_hw      power_led;   // GPIO pin number for the power LED
  Omron       omron;       // Omron device for blood pressure measurement
  LoRaWAN     lorawan;     // LoRaWAN device for data transmission
  fsm_state_t curr_state = IDLE;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Protected attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  int          time_since_start;
  uint32_t     last_transition_ms;
  omron_data_t curr_data;
  int          num_sent;
};

#endif  // UI_STATE_MACHINE_H