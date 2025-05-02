// =======================================================================
// switch_test.cpp
// =======================================================================

#include "ui/LED_hw.h"
#include "ui/button.h"
#include "ui/state_machine.h"
#include "utils/pt_cornell_rp2040_v1.h"
#include "utils/debug.h"
#include "pico/stdlib.h"


LED_hw green_led(2);
LED_hw yellow_led(3);
LED_hw red_led(4);
Button test_button(22);


// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------

int main()
{
  stdio_init_all();
  sleep_ms(3000);
  debug( "trying led test\n" );

  red_led.off();
  yellow_led.off();
  green_led.off();
  sleep_ms(200);
  red_led.on();
  sleep_ms(1000);

  while (true) {
    debug( "trying led test2\n" );
    test_button.update();

    if (test_button.is_released()) {
      green_led.on();
      yellow_led.on();
    }
    // else if (test_button.is_pressed()) {
    //   debug("Button is pressed\n");
    // }
    else if (test_button.just_pressed()) {
      debug("Just released\n");
      yellow_led.off();
      red_led.off();
    }
    // } else {
    //   debug( "Button is not pressed\n" );
    // }


    // red_led.off();
    // yellow_led.off();
    // green_led.off();
    // sleep_ms(200);
    // red_led.on();
    // sleep_ms(200);
    // yellow_led.on();
    // sleep_ms(200);
    // green_led.on();
    // sleep_ms(2000);
    // red_led.blink(500);
    // yellow_led.blink(500);
    // green_led.blink(500);

    sleep_ms(5);
    debug( "trying led test3\n" );
  }

}