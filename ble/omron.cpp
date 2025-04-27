// =======================================================================
// omron.cpp
// =======================================================================
// Definitions of our OMRON BLE class

#include "ble/omron.h"
#include "pico/stdlib.h"
#include "utils/debug.h"

// -----------------------------------------------------------------------
// Service identifiers
// -----------------------------------------------------------------------

const uint8_t parent_service_name[16] = {
    0xEC, 0xBE, 0x39, 0x80, 0xC9, 0xA2, 0x11, 0xE1,
    0xB1, 0xBD, 0x00, 0x02, 0xA5, 0xD5, 0xC5, 0x1B };

const uint8_t unlock_uuid[16] = { 0xB3, 0x05, 0xB6, 0x80, 0xAE, 0xE7,
                                  0x11, 0xE1, 0xA7, 0x30, 0x00, 0x02,
                                  0xA5, 0xD5, 0xC5, 0x1B };

const uint8_t blood_pressure_measurement[16] = {
    0x00, 0x00, 0x2A, 0x35, 0x00, 0x00, 0x10, 0x00,
    0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB };

// -----------------------------------------------------------------------
// Global command data
// -----------------------------------------------------------------------

uint8_t unlock_command[17] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00 };

uint8_t unlock_key[17] = { 0x01, 0xDE, 0xAD, 0xBE, 0xEF, 0x12,
                           0x34, 0x12, 0x34, 0xDE, 0xAD, 0xBE,
                           0xEF, 0x12, 0x34, 0x12, 0x34 };

// -----------------------------------------------------------------------
// correct_service
// -----------------------------------------------------------------------
// Check whether an advertisement report contains our service

bool Omron::correct_service_name( const uint8_t* service_name )
{
  // Service name comes in reverse order - change
  uint8_t uuid_128[16];
  reverse_128( service_name, uuid_128 );

  for ( int i = 0; i < 16; i++ ) {
    if ( uuid_128[i] != parent_service_name[i] ) {
      return false;
    }
  }
  return true;
}

bool Omron::correct_service( uint8_t* advertisement_report )
{
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Get advertisement data
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  const uint8_t* adv_data =
      gap_event_advertising_report_get_data( advertisement_report );
  uint8_t adv_len = gap_event_advertising_report_get_data_length(
      advertisement_report );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Check advertisement data
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  ad_context_t context;
  for ( ad_iterator_init( &context, adv_len, adv_data );
        ad_iterator_has_more( &context ); ad_iterator_next( &context ) ) {
    // Check if the correct data type
    uint8_t        data_type = ad_iterator_get_data_type( &context );
    uint8_t        data_size = ad_iterator_get_data_len( &context );
    const uint8_t* data      = ad_iterator_get_data( &context );
    switch ( data_type ) {
      case BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS:
      case BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS:
      case BLUETOOTH_DATA_TYPE_LIST_OF_128_BIT_SERVICE_SOLICITATION_UUIDS:
        // Check to see if it's the correct service name
        for ( int i = 0; i < data_size; i += 16 ) {
          if ( correct_service_name( &( data[i] ) ) ) {
            return true;
          }
        }
        break;
      default:
        break;
    }
  }
  // No correct service found
  return false;
}

// -----------------------------------------------------------------------
// after_discovery
// -----------------------------------------------------------------------
// Start finding notifications

void Omron::after_discovery()
{
  pair_notification();
}

// -----------------------------------------------------------------------
// Global callback to handle events driven from notifications
// -----------------------------------------------------------------------

Omron* curr_omron = nullptr;

int64_t global_omron_poll( alarm_id_t id, __unused void* user_data )
{
  if ( curr_omron ) {
    curr_omron->poll();
  }
  return 0;
}

void Omron::poll()
{
  omron_poll_event_t old_event = poll_event;
  poll_event                   = NONE;

  switch ( old_event ) {
  PAIR_WRITE_KEY:
    pair_write_key();
  }
}

void omron_schedule_poll()
{
  add_alarm_in_ms( 2000, global_omron_poll, NULL, false );
}

Omron::Omron() : Client(), omron_state( OM_IDLE ), poll_event( NONE )
{
  curr_omron = this;
}

// -----------------------------------------------------------------------
// Helper state machine functions
// -----------------------------------------------------------------------

void Omron::pair_notification()
{
  omron_state = OM_PAIR_NOTIFICATION;
  if ( !gatt_client_is_ready( connection_handle ) ) {
    debug( "[Omron] ============ CLIENT NOT READY ============\n" );
  }

  debug( "[Omron] Enabling unlock notifications...\n" );
  int status = enable_notifications( unlock_uuid );
  // int status = enable_indications( blood_pressure_measurement );
  if ( status != 0 ) {
    debug( "[Omron] Error enabling unlock notifications (%d)...\n",
           status );
  }
}

void Omron::pair_unlock()
{
  omron_state = OM_PAIR_UNLOCK;
  if ( !gatt_client_is_ready( connection_handle ) ) {
    debug( "[Omron] ============ CLIENT NOT READY ============\n" );
  }

  debug( "[Omron] Unlocking device...\n" );
  int status = gatt_client_write_value_of_characteristic(
      gatt_client_event_callback, connection_handle,
      value_handle_from_uuid( unlock_uuid ), 17, unlock_key );
  if ( status != 0 ) {
    debug( "[Omron] Error writing the unlock value (%d)...\n", status );
  }
}

void Omron::pair_write_key()
{
  omron_state = OM_PAIR_WRITE_KEY;
  if ( !gatt_client_is_ready( connection_handle ) ) {
    poll_event = PAIR_WRITE_KEY;
    omron_schedule_poll();
  }

  debug( "[Omron] Writing unlock key...\n" );
  gatt_client_write_value_of_characteristic(
      gatt_client_event_callback, connection_handle,
      value_handle_from_uuid( unlock_uuid ), 17, unlock_key );
}

void Omron::pair_disable_notification()
{
  omron_state = OM_PAIR_DISABLE_NOTIFICATION;
  if ( !gatt_client_is_ready( connection_handle ) ) {
    debug( "[Omron] ============ CLIENT NOT READY ============\n" );
  }

  debug( "[Omron] Disabling unlock notifications...\n" );
  int status = disable_notifications_indications( unlock_uuid );
  if ( status != 0 ) {
    debug( "[Omron] Error disabling unlock notifications (%d)...\n",
           status );
  }
}

void Omron::blood_pressure_ready()
{
  omron_state = OM_READY;
  if ( !gatt_client_is_ready( connection_handle ) ) {
    debug( "[Omron] ============ CLIENT NOT READY ============\n" );
  }

  debug( "[Omron] Pairing key written!\n" );
}

// -----------------------------------------------------------------------
// Helper functions
// -----------------------------------------------------------------------

bool Omron::omron_ready()
{
  return omron_state == OM_READY;
}

// -----------------------------------------------------------------------
// child_gatt_event_handler
// -----------------------------------------------------------------------
// Handle events from Omron activities

void Omron::child_gatt_event_handler( uint8_t  packet_type,
                                      uint8_t* packet )
{
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Handle packet based on current state
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  switch ( omron_state ) {
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Handle unlock notification enable response
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case OM_PAIR_NOTIFICATION:
      switch ( packet_type ) {
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Indication response
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case GATT_EVENT_QUERY_COMPLETE:
          if ( gatt_event_query_complete_get_att_status( packet ) !=
               ATT_ERROR_SUCCESS ) {
            debug(
                "[Omron] Error enabling unlock notifications (0x%X)...\n",
                gatt_event_query_complete_get_att_status( packet ) );
            break;
          }
          pair_unlock();
          break;

        default:
          break;
      }
      break;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Unlock command (do nothing, advance state on notification)
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case OM_PAIR_UNLOCK:
      switch ( packet_type ) {
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Write response
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case GATT_EVENT_QUERY_COMPLETE:
          debug( "[Omron] Received unlock command acknowledgement...\n" );
          if ( gatt_event_query_complete_get_att_status( packet ) !=
               ATT_ERROR_SUCCESS ) {
            debug( "[Omron] Error writing unlock command (0x%X)...\n",
                   gatt_event_query_complete_get_att_status( packet ) );
            break;
          }
          break;
        default:
          break;
      }
      break;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Unlock key (do nothing, advance state on notification)
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case OM_PAIR_WRITE_KEY:
      debug( "[Omron] Received unlock key acknowledgement...\n" );
      if ( gatt_event_query_complete_get_att_status( packet ) !=
           ATT_ERROR_SUCCESS ) {
        debug( "[Omron] Error writing unlock key (0x%X)...\n",
               gatt_event_query_complete_get_att_status( packet ) );
        break;
      }
      break;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable unlock notifications
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case OM_PAIR_DISABLE_NOTIFICATION:
      if ( gatt_event_query_complete_get_att_status( packet ) !=
           ATT_ERROR_SUCCESS ) {
        debug( "[Omron] Error disabling unlock notifications (0x%X)...\n",
               gatt_event_query_complete_get_att_status( packet ) );
        break;
      }
      blood_pressure_ready();
      break;

    default:
      break;
  }
}

// -----------------------------------------------------------------------
// notification_handler
// -----------------------------------------------------------------------

void Omron::notification_handler( uint16_t       value_handle,
                                  const uint8_t* value,
                                  uint32_t       value_length )
{
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Handle by current state
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  switch ( omron_state ) {
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Unlock command
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case OM_PAIR_UNLOCK:
      if ( value_handle_from_uuid( unlock_uuid ) != value_handle ) {
        debug( "[Omron] Wrong value handle...\n" );
        break;
      }
      if ( ( value[0] != 0x82 ) ) {
        debug(
            "[Omron] Couldn't enter programming mode (ensure pairing mode is active): 0x" );
        for ( int i = 0; i < value_length; i++ ) {
          debug( "%X", value[i] );
        }
        debug( "...\n" );
        break;
      }

      // Received good command - advance state
      pair_write_key();
      break;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Unlock key
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case OM_PAIR_WRITE_KEY:
      if ( value_handle_from_uuid( unlock_uuid ) != value_handle ) {
        debug( "[Omron] Wrong value handle...\n" );
        break;
      }
      if ( ( value[0] != 0x80 ) ) {
        debug( "[Omron] Failed to program new key: 0x" );
        for ( int i = 0; i < value_length; i++ ) {
          debug( "%X", value[i] );
        }
        debug( "...\n" );
        break;
      }

      // Received good command - advance state
      pair_disable_notification();
      break;

    default:
      break;
  }
}