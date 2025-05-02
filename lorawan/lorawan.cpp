// =======================================================================
// lorawan.cpp
// =======================================================================
// Definitions of the LoRaWAN class

#include "lorawan/lorawan.h"
#include "utils/debug.h"

// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------

LoRaWAN::LoRaWAN() : join_started( false )
{
  if ( lorawan_init_otaa( &sx1276_settings, LORAWAN_REGION,
                          &otaa_settings ) < 0 ) {
    debug( "[LoRaWAN] Initialization Failed...\n" );
    while ( 1 ) {
      tight_loop_contents();
    }
  }
  else {
    debug( "[LoRaWAN] Initialization Successful!\n" );
  }
}

// -----------------------------------------------------------------------
// try_join
// -----------------------------------------------------------------------

bool LoRaWAN::try_join()
{
  if ( !join_started ) {
    debug( "[LoRaWAN] Starting to join...\n" );
    lorawan_join();
  }

  if ( lorawan_is_joined() ) {
    debug( "[LoRaWAN] Connected!\n" );
    return true;
  }
  else {
    return false;
  }
}

// -----------------------------------------------------------------------
// try_send
// -----------------------------------------------------------------------

bool LoRaWAN::try_send( const uint8_t* data, uint8_t data_len )
{
  return lorawan_send_unconfirmed( data, data_len, 2 ) >= 0;
}