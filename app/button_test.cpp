// =======================================================================
// button_test.cpp
// =======================================================================
// A quick demo to connect to the Omron measurement device

#include "ui/button.h"
#include "utils/pt_cornell_rp2040_v1.h"
#include "utils/debug.h"

Button test_button(4);

// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------

int main()
{
  stdio_init_all();
  sleep_ms(3000);
  debug( "trying button test\n" );

  while (true) {
    test_button.update();

    if (test_button.just_pressed()) {
      debug("Just pressed\n");
    }
    else if (test_button.is_pressed()) {
      debug("Button is pressed\n");
    }
    else if (test_button.just_released()) {
      debug("Just released\n");
    } else {
      debug( "Button is not pressed\n" );
    }
    sleep_ms(5);
  }
}