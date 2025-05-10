// =======================================================================
// lorawan.cpp
// =======================================================================
// Definitions of the LoRaWAN class

#include "lorawan/lorawan.h"
#include "utils/debug.h"

// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------

LoRaWAN::LoRaWAN() : join_started( false ), msg_sent( false )
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
    join_started = true;
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
// aes128_encrypt_data
// -----------------------------------------------------------------------
//encrypts on byte at a time
void LoRaWAN::aes128_encrypt_data(const uint8_t* input_data, uint8_t* encrypted_data, uint8_t data_len) {

}

// -----------------------------------------------------------------------
// try_send
// -----------------------------------------------------------------------

uint8_t  receive_port = 0;
uint8_t  receive_msg[50];
uint32_t msg_send_time;

bool LoRaWAN::try_send( const uint8_t* data, uint8_t data_len )
{
  if ( !msg_sent ) {
    if ( lorawan_send_unconfirmed( data, data_len, 2 ) >= 0 ) {
      msg_sent      = true;
      msg_send_time = to_ms_since_boot( get_absolute_time() );
    }
  }

  // Wait for downlink
  if ( lorawan_receive( receive_msg, 50, &receive_port ) > -1 ) {
    debug( "Got a response!\n" );
    msg_sent = false;
    return true;
  }

  // Send again after 5 seconds
  if ( to_ms_since_boot( get_absolute_time() ) - msg_send_time > 5000 ) {
    if ( lorawan_send_unconfirmed( data, data_len, 2 ) >= 0 ) {
      msg_send_time = to_ms_since_boot( get_absolute_time() );
    }
  }
  return false;
}