// =======================================================================
// lorawan.cpp
// =======================================================================
// Definitions of the LoRaWAN class

#include "confirm.h"
#include "lorawan/lorawan.h"
#include "utils/debug.h"

LoRaWAN* curr_lorawan = nullptr;

void lorawan_confirm()
{
  if ( curr_lorawan ) {
    debug( "[LoRaWAN] Confirming...\n" );
    curr_lorawan->confirm();
  }
}

// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------

LoRaWAN::LoRaWAN() : join_started( false ), msg_sent( false )
{
  curr_lorawan = this;
  on_confirm( lorawan_confirm );
  if ( lorawan_init_otaa( &sx1276_settings, CUSTOM_LORAWAN_REGION,
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
// send_confirmed
// -----------------------------------------------------------------------
// A mimic of the LoRaWAN library's `send_unconfirmed`, but with a
// downlink confirmation to tell when the message was sent

int LoRaWAN::send_confirmed( const void* data, uint8_t data_len,
                             uint8_t app_port )
{
  LmHandlerAppData_t appData;

  appData.Port       = app_port;
  appData.BufferSize = data_len;
  appData.Buffer     = (uint8_t*) data;

  if ( LmHandlerSend( &appData, LORAMAC_HANDLER_CONFIRMED_MSG ) !=
       LORAMAC_HANDLER_SUCCESS ) {
    return -1;
  }

  return 0;
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
    msg_confirmed = false;
    if ( send_confirmed( data, data_len, 2 ) >= 0 ) {
      msg_sent      = true;
      msg_send_time = to_ms_since_boot( get_absolute_time() );
    }
  }

  // Wait for downlink
  if ( msg_confirmed ) {
    debug( "Got a confirmation!\n" );
    msg_sent = false;
    return true;
  }

  // Send again after 5 seconds
  if ( to_ms_since_boot( get_absolute_time() ) - msg_send_time > 5000 ) {
    msg_confirmed = false;
    if ( send_confirmed( data, data_len, 2 ) >= 0 ) {
      msg_send_time = to_ms_since_boot( get_absolute_time() );
    }
  }
  return false;
}

void LoRaWAN::confirm()
{
  msg_confirmed = true;
}