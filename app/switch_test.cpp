// =======================================================================
// switch_test.cpp
// =======================================================================

#include "ui/switch.h"
#include "utils/pt_cornell_rp2040_v1.h"
#include "utils/debug.h"

Switch test_switch(4);

// -----------------------------------------------------------------------
// print_characteristics
// -----------------------------------------------------------------------
// A thread to wait until characteristics are ready, then print them

// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------

int main()
{
  stdio_init_all();
  stdio_usb_init();
  sleep_ms(3000);
  debug( "trying switch test\n" );

  while (true) {
    test_switch.update();

    if (test_switch.just_flipped()) {
      debug("Just flipped on\n");
    }
    else if (test_switch.is_flipped()) {
      debug("Switch is on\n");
    }
    else if (test_switch.just_unflipped()) {
      debug("Just flipped off\n");
    } else {
      debug( "Switch is not pressed\n" );
    }
    sleep_ms(5);
  }

}