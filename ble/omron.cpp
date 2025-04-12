// =======================================================================
// omron.cpp
// =======================================================================
// Definitions of our OMRON BLE class

#include "ble/omron.h"
#include "utils/debug.h"

// -----------------------------------------------------------------------
// Service identifiers
// -----------------------------------------------------------------------

const uint8_t parent_service_name[16] = {
    0xEC, 0xBE, 0x39, 0x80, 0xC9, 0xA2, 0x11, 0xE1,
    0xB1, 0xBD, 0x00, 0x02, 0xA5, 0xD5, 0xC5, 0x1B };

service_uuid_t Omron::get_service_name()
{
  return (uint16_t) ORG_BLUETOOTH_SERVICE_BLOOD_PRESSURE;
}

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