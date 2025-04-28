// =======================================================================
// switch_test.cpp
// =======================================================================

#include "ui/LED_hw.h"
#include "utils/pt_cornell_rp2040_v1.h"
#include "utils/debug.h"

LED_hw test_led(4);


// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------

int main()
{
  stdio_init_all();
  stdio_usb_init();
  sleep_ms(3000);
  debug( "trying led test\n" );

  while (true) {
    debug( "trying switch test2\n" );
    test_led.off();
    sleep_ms(1000);
    test_led.on();
    sleep_ms(1000);
    test_led.blink(3000);
    sleep_ms(1000);
    debug( "trying switch test3\n" );
  }

}