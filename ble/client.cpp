// =======================================================================
// client.cpp
// =======================================================================
// Definitions of our BLE client functions

#include "ble/client.h"
#include "pico/cyw43_arch.h"
#include "utils/debug.h"
#include <functional>
#include <stdio.h>

// -----------------------------------------------------------------------
// Global state for handling callbacks
// -----------------------------------------------------------------------

BaseClient* curr_client = nullptr;

void global_gatt_client_event_handler( uint8_t  packet_type,
                                       uint16_t channel, uint8_t* packet,
                                       uint16_t size )
{
  if ( curr_client ) {
    curr_client->gatt_client_event_handler( packet_type, channel, packet,
                                            size );
  }
}

void global_hci_event_handler( uint8_t packet_type, uint16_t channel,
                               uint8_t* packet, uint16_t size )
{
  if ( curr_client ) {
    curr_client->hci_event_handler( packet_type, channel, packet, size );
  }
}

// -----------------------------------------------------------------------
// RAII Management (Constructor, Destructor)
// -----------------------------------------------------------------------

Client::Client()
    : state( TC_OFF ),
      listener_registered( false ),
      curr_char_idx( 0 ),
      curr_char_descr_idx( 0 ),
      num_characteristics_discovered( 0 )
{
  curr_client = this;

  // Initialize CYW43 Architecture (should check if non-zero, but avoid in
  // constructor)
  cyw43_arch_init();

  // Initialize L2CAP and Security Manager
  l2cap_init();
  sm_init();
  sm_set_io_capabilities( IO_CAPABILITY_NO_INPUT_NO_OUTPUT );

  // Setup empty ATT server - only needed if LE Peripheral does ATT
  // queries on its own, e.g. Android and iOS
  att_server_init( NULL, NULL, NULL );

  // Initialize GATT client
  gatt_client_init();

  // Register the HCI event callback function
  hci_event_callback_registration.callback = &global_hci_event_handler;
  hci_add_event_handler( &hci_event_callback_registration );
}

Client::~Client()
{
  curr_client = nullptr;
}

// -----------------------------------------------------------------------
// Connecting and disconnecting from server
// -----------------------------------------------------------------------
// Just change the power on the interface

void Client::connect_to_server()
{
  hci_power_control( HCI_POWER_ON );
}

void Client::disconnect_from_server()
{
  connection_handle = HCI_CON_HANDLE_INVALID;
  if ( listener_registered ) {
    listener_registered = false;
    gatt_client_stop_listening_for_characteristic_value_updates(
        &notification_listener );
  }
  state = TC_OFF;
  hci_power_control( HCI_POWER_SLEEP );
}

bool Client::ready()
{
  return state == TC_W4_READY;
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

void Client::characteristic_discovery()
{
  state = TC_W4_CHARACTERISTIC_RESULT;
  gatt_client_discover_characteristics_for_service(
      global_gatt_client_event_handler, connection_handle,
      &server_service );
}

void Client::characteristic_descriptor_discovery()
{
  state = TC_W4_CHARACTERISTIC_DESCRIPTOR;
  gatt_client_discover_characteristic_descriptors(
      global_gatt_client_event_handler, connection_handle,
      &server_characteristic[curr_char_idx] );
}

// Helper function to check which descriptor is the description
bool is_description( gatt_client_characteristic_descriptor_t descriptor )
{
  return descriptor.uuid16 == GATT_CHARACTERISTIC_USER_DESCRIPTION;
}

void Client::characteristic_description_discovery()
{
  state = TC_W4_CHARACTERISTIC_DESCRIPTION;

  // Get the description from the correct descriptor
  if ( is_description(
           server_characteristic_descriptor[curr_char_idx][0] ) ) {
    // 0 has the description
    gatt_client_read_characteristic_descriptor(
        global_gatt_client_event_handler, connection_handle,
        &server_characteristic_descriptor[curr_char_idx][0] );
  }
  else {
    // 1 has the description
    gatt_client_read_characteristic_descriptor(
        global_gatt_client_event_handler, connection_handle,
        &server_characteristic_descriptor[curr_char_idx][1] );
  }
}

void Client::read_characteristic_value()
{
  state = TC_W4_CHARACTERISTIC_VALUE;
  gatt_client_read_value_of_characteristic(
      global_gatt_client_event_handler, connection_handle,
      &server_characteristic[curr_char_idx] );
}

void Client::read_characteristic_config()
{
  state = TC_W4_CHARACTERISTIC_CONFIG;

  // Find next descriptor for a configuration
  while ( ( curr_char_idx < num_characteristics_discovered ) &&
          ( server_characteristic_descriptor[curr_char_idx][0].uuid16 !=
            GATT_CLIENT_CHARACTERISTICS_CONFIGURATION ) ) {
    curr_char_idx++;
  }

  // If we found one, get its configuration
  if ( curr_char_idx < num_characteristics_discovered ) {
    gatt_client_read_characteristic_descriptor(
        global_gatt_client_event_handler, connection_handle,
        &server_characteristic_descriptor[curr_char_idx][0] );
  }

  // If none left to get, move to notifications
  else {
    state               = TC_W4_READY;
    listener_registered = true;
    gatt_client_listen_for_characteristic_value_updates(
        &notification_listener, global_gatt_client_event_handler,
        connection_handle, NULL );
  }
}

// -----------------------------------------------------------------------
// gatt_client_event_handler
// -----------------------------------------------------------------------
// Handle GATT Events

void Client::gatt_client_event_handler( uint8_t  packet_type,
                                        uint16_t channel, uint8_t* packet,
                                        uint16_t size )
{
  // Don't use the packet_type, channel, or size
  (void) packet_type;
  (void) channel;
  (void) size;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Check if it's a notification
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  uint8_t type_of_packet = hci_event_packet_get_type( packet );
  if ( type_of_packet == GATT_EVENT_NOTIFICATION ) {
    gatt_client_notification_handler( packet );
    return;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Handle packet based on current state
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // First called when in TC_W4_SERVICE_RESULT from hci_event_handler

  uint8_t        att_status;
  uint32_t       description_length;
  const uint8_t* description;
  uint32_t       value_length;
  const uint8_t* value;
  uint32_t       config_length;
  const uint8_t* config;

  switch ( state ) {
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Handle result of service request
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case TC_W4_SERVICE_RESULT:
      switch ( type_of_packet ) {
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Information about service (store)
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case GATT_EVENT_SERVICE_QUERY_RESULT:
          gatt_event_service_query_result_get_service( packet,
                                                       &server_service );
          break;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Finished with service result (discover characteristics)
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case GATT_EVENT_QUERY_COMPLETE:
          // Make sure no errors
          att_status = gatt_event_query_complete_get_att_status( packet );
          if ( att_status != ATT_ERROR_SUCCESS ) {
            printf( "SERVICE_QUERY_RESULT, ATT Error 0x%02x.\n",
                    att_status );
            gap_disconnect( connection_handle );
            break;
          }

          // Clear all notifications
          memset( notifications_enabled, -1, MAX_CHARACTERISTICS );
          characteristic_discovery();

        default:
          break;
      }
      break;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Characteristic Discovery
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case TC_W4_CHARACTERISTIC_RESULT:
      switch ( type_of_packet ) {
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Characteristic that was discovered (store)
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
          gatt_event_characteristic_query_result_get_characteristic(
              packet, &server_characteristic[curr_char_idx++] );
          break;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Done with characteristics (move to descriptors)
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case GATT_EVENT_QUERY_COMPLETE:
          num_characteristics_discovered = curr_char_idx;

          curr_char_idx       = 0;
          curr_char_descr_idx = 0;

          // Make sure no errors
          att_status = gatt_event_query_complete_get_att_status( packet );
          if ( att_status != ATT_ERROR_SUCCESS ) {
            printf( "SERVICE_QUERY_RESULT, ATT Error 0x%02x.\n",
                    att_status );
            gap_disconnect( connection_handle );
            break;
          }
          characteristic_descriptor_discovery();
          break;

        default:
          break;
      }
      break;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Characteristic Descriptor Discovery
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case TC_W4_CHARACTERISTIC_DESCRIPTOR:
      switch ( type_of_packet ) {
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Descriptor that was discovered (store)
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case GATT_EVENT_ALL_CHARACTERISTIC_DESCRIPTORS_QUERY_RESULT:
          gatt_event_all_characteristic_descriptors_query_result_get_characteristic_descriptor(
              packet,
              &server_characteristic_descriptor[curr_char_idx]
                                               [curr_char_descr_idx++] );
          break;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Done with descriptors
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case GATT_EVENT_QUERY_COMPLETE:
          curr_char_idx++;
          curr_char_descr_idx = 0;

          // Discover next characteristic, if any remaining
          if ( curr_char_idx < num_characteristics_discovered ) {
            characteristic_descriptor_discovery();
            break;
          }

          // Discover characteristic descriptions
          curr_char_idx = 0;
          characteristic_description_discovery();
          break;

        default:
          break;
      }
      break;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Characteristic Description Discovery
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case TC_W4_CHARACTERISTIC_DESCRIPTION:
      switch ( type_of_packet ) {
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Description that was discovered (store)
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case GATT_EVENT_CHARACTERISTIC_DESCRIPTOR_QUERY_RESULT:
          // Get the description
          description_length =
              gatt_event_characteristic_descriptor_query_result_get_descriptor_length(
                  packet );
          description =
              gatt_event_characteristic_descriptor_query_result_get_descriptor(
                  packet );

          // Store description (including null-termination)
          memcpy( server_characteristic_user_description[curr_char_idx],
                  description, description_length );
          server_characteristic_user_description[curr_char_idx]
                                                [description_length] =
                                                    '\0';
          break;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Done with description
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case GATT_EVENT_QUERY_COMPLETE:
          curr_char_idx++;

          // Read remaining descriptions, if any
          if ( curr_char_idx < num_characteristics_discovered ) {
            characteristic_description_discovery();
            break;
          }

          // Discover characteristic values
          curr_char_idx       = 0;
          curr_char_descr_idx = 0;
          read_characteristic_value();
          break;

        default:
          break;
      }
      break;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Characteristic Value Discovery
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case TC_W4_CHARACTERISTIC_VALUE:
      switch ( type_of_packet ) {
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Value that was discovered (store)
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT:
          // Get the value
          value_length =
              gatt_event_characteristic_value_query_result_get_value_length(
                  packet );
          value = gatt_event_characteristic_value_query_result_get_value(
              packet );

          // Store value (including null-termination)
          memcpy( server_characteristic_values[curr_char_idx], value,
                  value_length );
          server_characteristic_values[curr_char_idx][value_length] =
              '\0';
          break;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Done with value
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case GATT_EVENT_QUERY_COMPLETE:
          curr_char_idx++;

          // Read remaining values, if any
          if ( curr_char_idx < num_characteristics_discovered ) {
            characteristic_description_discovery();
            break;
          }

          // Discover characteristic configurations
          curr_char_idx       = 0;
          curr_char_descr_idx = 0;
          read_characteristic_config();
          break;

        default:
          break;
      }
      break;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Characteristic Configuration Discovery
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case TC_W4_CHARACTERISTIC_CONFIG:
      switch ( type_of_packet ) {
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Configuration that was discovered (store)
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case GATT_EVENT_CHARACTERISTIC_DESCRIPTOR_QUERY_RESULT:
          // Get the configuration
          config_length =
              gatt_event_characteristic_value_query_result_get_value_length(
                  packet );
          config = gatt_event_characteristic_value_query_result_get_value(
              packet );

          // Store the configuration
          server_characteristic_configurations[curr_char_idx] =
              little_endian_read_16( config, 0 );

          // Check whether notifications are enabled
          notifications_enabled[curr_char_idx] = ( (
              char) server_characteristic_configurations[curr_char_idx] );

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Done with configuration
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case GATT_EVENT_QUERY_COMPLETE:
          curr_char_idx++;
          read_characteristic_config();

        default:
          break;
      }
      break;

    default:
      break;
  }
}

// -----------------------------------------------------------------------
// gatt_client_event_handler
// -----------------------------------------------------------------------
// Handle asynchronous notifications

void Client::gatt_client_notification_handler( uint8_t* packet )
{
  // Get payload length, value handle, and value
  uint32_t value_length =
      gatt_event_notification_get_value_length( packet );
  uint16_t value_handle =
      gatt_event_notification_get_value_handle( packet );
  const uint8_t* value = gatt_event_notification_get_value( packet );

  // Find the characteristic that the value is for
  int value_char_idx = -1;
  for ( int i = 0; i < num_characteristics_discovered; i++ ) {
    if ( value_handle == server_characteristic[i].value_handle ) {
      value_char_idx = i;
    }
  }

  // Update the characteristic value if found (including null terminator)
  if ( value_char_idx >= 0 ) {
    memcpy( server_characteristic_values[value_char_idx], value,
            value_length );
    server_characteristic_values[value_char_idx][value_length] = 0;
  }
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

// -----------------------------------------------------------------------
// Printing
// -----------------------------------------------------------------------

// Plaintext for various access permisions
char  broadcast[]           = "Broadcast";
char  read[]                = "Read";
char  write_no_resp[]       = "Write without response";
char  write[]               = "Write";
char  notify[]              = "Notify";
char  indicate[]            = "Indicate";
char  authen[]              = "Signed write";
char  extended[]            = "Extended";
char* access_permissions[8] = { broadcast, read,    write_no_resp,
                                write,     notify,  indicate,
                                authen,    extended };

void print_permissions( uint16_t permissions )
{
  bool is_first = true;
  for ( int i = 0; i < 8; i++ ) {
    if ( ( 1u << i ) & permissions ) {
      if ( !is_first ) {
        printf( " - " );
        is_first = false;
      }
      printf( "%s ", access_permissions[i] );
    }
  }
}

void Client::print()
{
  if ( state != TC_W4_READY )
    return;
  for ( int idx = 0; idx < num_characteristics_discovered; idx++ ) {
    printf( "%d:\n", idx );
    printf( "\t - Description: %s\n",
            server_characteristic_user_description[idx] );

    printf( "\t - Permissions: " );
    print_permissions( server_characteristic[idx].properties );
    printf( "\n" );

    printf( "\t - Value: %s", server_characteristic_values[idx] );
  }
}