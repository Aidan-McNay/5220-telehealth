// =======================================================================
// state_machine_test.cpp
// =======================================================================

#include "ui/state_machine.h"
#include "utils/pt_cornell_rp2040_v1.h"
#include "utils/debug.h"
#include "pico/stdlib.h"

FSM test_sim(5, 4, 2, 3);

// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------

int main()
{
  stdio_init_all();
  sleep_ms(3000);

  debug( "trying state machine test 3\n" );


  while (true) {
    debug( "running simulation\n" );
    test_sim.update();

    if (test_sim.is_measuring()) {
      debug("measuring now");
    }
    else if (test_sim.is_transmitting()) {
      debug("transmitting now");
    }
    else if (test_sim.is_done()) {
      debug("transmission done!");
    }
    else {
      debug( "Switch is not pressed" );
    }

    debug( "   transmissions: %d\n", test_sim.transmissions_done() );

    sleep_ms(5);
  }

}