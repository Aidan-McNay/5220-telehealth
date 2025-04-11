// =======================================================================
// client.cpp
// =======================================================================
// Definitions of our BLE client functions

#include "ble/client.h"
#include "pico/cyw43_arch.h"
#include "utils/debug.h"
#include <functional>

// -----------------------------------------------------------------------
// Global state for handling callbacks
// -----------------------------------------------------------------------

Client* curr_client = nullptr;

void global_gatt_client_event_handler( uint8_t  packet_type,
                                       uint16_t channel, uint8_t* packet,
                                       uint16_t size )
{
  if ( curr_client ) {
    curr_client->gatt_client_event_handler( packet_type, channel, packet,
                                            size );
  }
}

// -----------------------------------------------------------------------
// RAII Management
// -----------------------------------------------------------------------

Client::Client() : state( TC_OFF ), listener_registered( false )
{
  curr_client = this;
}

Client::~Client()
{
  curr_client = nullptr;
}

// -----------------------------------------------------------------------
// correct_service
// -----------------------------------------------------------------------
// Check whether an advertisement report contains our service

bool Client::correct_service( uint8_t* advertisement_report )
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
    uint8_t data_type = ad_iterator_get_data_type( &context );
    if (
        data_type ==
        BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS ) {
      // Check the services
      uint8_t        data_size = ad_iterator_get_data_len( &context );
      const uint8_t* data      = ad_iterator_get_data( &context );
      uint16_t       service   = get_service();
      for ( int i = 0; i < data_size; i += 2 ) {
        uint16_t type = little_endian_read_16( data, i );
        if ( type == service )
          return true;
      }
    }
  }
  // No correct service found
  return false;
}

// -----------------------------------------------------------------------
// State transition helper functions
// -----------------------------------------------------------------------

void Client::off()
{
  state = TC_OFF;
}

// Scan every INTERVAL time, with scanning occuring during WINDOW within
// INTERVAL
#define GAP_SCAN_INTERVAL 0x0030  // 0x30 * 6.25ms = 300ms
#define GAP_SCAN_WINDOW 0x0030

void Client::start()
{
  state = TC_W4_SCAN_RESULT;

  // Start GAP scan
  gap_set_scan_params( GAP_SCAN_PASSIVE, GAP_SCAN_INTERVAL,
                       GAP_SCAN_WINDOW, GAP_SCAN_ALL );
  gap_start_scan();
}

void Client::connect()
{
  state = TC_W4_CONNECT;
  gap_connect( server_addr, server_addr_type );
}

void Client::service_discovery()
{
  state = TC_W4_SERVICE_RESULT;
  gatt_client_discover_primary_services_by_uuid128(
      global_gatt_client_event_handler, connection_handle,
      get_service_name() );
}

// -----------------------------------------------------------------------
// gatt_client_event_handler
// -----------------------------------------------------------------------
// Handle GATT Events

void Client::gatt_client_event_handler( uint8_t  packet_type,
                                        uint16_t channel, uint8_t* packet,
                                        uint16_t size )
{
}

// -----------------------------------------------------------------------
// hci_event_handler
// -----------------------------------------------------------------------
// Handle event from CWY43439

void Client::hci_event_handler( uint8_t packet_type, uint16_t channel,
                                uint8_t* packet, uint16_t size )
{
  // We don't use the size and channel
  (void) size;
  (void) channel;

  // Confirm that it's an event packet
  if ( packet_type != HCI_EVENT_PACKET )
    return;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Handle different event types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  uint8_t event_type = hci_event_packet_get_type( packet );

  switch ( event_type ) {
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Startup
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case BTSTACK_EVENT_STATE:
      // Start listening if the chip is working
      if ( btstack_event_state_get_state( packet ) ==
           HCI_STATE_WORKING ) {
        start();
      }
      else {
        off();
      }
      break;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Advertising Report
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case GAP_EVENT_ADVERTISING_REPORT:
      // Only handle if we are scanning
      if ( state != TC_W4_SCAN_RESULT )
        return;

      // Confirm it's the service we want
      if ( !correct_service( packet ) )
        return;

      // Get the address of the server we're connecting to
      gap_event_advertising_report_get_address( packet, server_addr );
      server_addr_type =
          (bd_addr_type_t) gap_event_advertising_report_get_address_type(
              packet );

      // Stop scanning and connect
      gap_stop_scan();
      connect();
      break;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Controller-Specific - wait for completed connection
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case HCI_EVENT_LE_META:
      // Check that a connection completed
      if ( hci_event_le_meta_get_subevent_code( packet ) !=
           HCI_SUBEVENT_LE_CONNECTION_COMPLETE )
        return;

      // Only handle if we were connecting
      if ( state != TC_W4_CONNECT )
        return;

      // Initiate service discovery with our handler
      state = TC_W4_SERVICE_RESULT;
      connection_handle =
          hci_subevent_le_connection_complete_get_connection_handle(
              packet );
      service_discovery();
      break;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disconnect
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case HCI_EVENT_DISCONNECTION_COMPLETE:
      // Unregister listener, if necessary
      connection_handle = HCI_CON_HANDLE_INVALID;
      if ( listener_registered ) {
        listener_registered = false;
        gatt_client_stop_listening_for_characteristic_value_updates(
            &notification_listener );
      }

      // If we're not off, start listening again
      if ( state != TC_OFF ) {
        start();
      }
      break;

    default:
      break;
  }
}