// =======================================================================
// LED.h
// =======================================================================
// Declaration of the button utilities

#ifndef UI_LED_HW_H
#define UI_LED_HW_H

#include <cstdint>

class LED_hw {
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Public accessor functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 public:
  LED_hw( int gpio_num );
  void on();
  void off();
  void blink( int duration_ms );
  bool is_on();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Protected attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 private:
  int      gpio_num;  // GPIO pin number for the button
  uint32_t last_blink_toggle;
  bool     blinking;
  bool     state;  // Whether we're on or off
};

#endif  // UI_LED_hw_H