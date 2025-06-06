// =======================================================================
// client.cpp
// =======================================================================
// Definitions of our BLE client functions

#include "ble/client.h"
#include "pico/cyw43_arch.h"
#include "utils/debug.h"
#include <stdio.h>

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

void Client::reset()
{
  listener_registered              = false;
  curr_service_idx                 = 0;
  curr_char_idx                    = 0;
  curr_char_descr_idx              = 0;
  curr_total_char_idx              = 0;
  total_characteristics_discovered = 0;
}

Client::Client()
    : state( TC_OFF ),
      hci_event_callback( global_hci_event_handler ),
      gatt_client_event_callback( global_gatt_client_event_handler )
{
  curr_client = this;
  reset();
  for ( int i = 0; i < MAX_SERVICES; i++ ) {
    num_characteristics_discovered[i] = 0;
  }

  // Initialize CYW43 Architecture (should check if non-zero, but avoid in
  // constructor)
  cyw43_arch_init();

  // Initialize L2CAP and Security Manager
  l2cap_init();
  sm_init();
  sm_set_io_capabilities( IO_CAPABILITY_NO_INPUT_NO_OUTPUT );
  sm_set_authentication_requirements( SM_AUTHREQ_BONDING |
                                      SM_AUTHREQ_MITM_PROTECTION |
                                      SM_AUTHREQ_SECURE_CONNECTION );

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
  reset();
  state = TC_OFF;
  hci_power_control( HCI_POWER_SLEEP );
}

bool Client::ready()
{
  return state == TC_W4_READY;
}

bool Client::discovered()
{
  switch ( state ) {
    case TC_OFF:
    case TC_IDLE:
    case TC_W4_SCAN_RESULT:
      return false;
    default:
      return true;
  }
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
  debug( "[BLE] Starting scan...\n" );
  gap_set_scan_params( GAP_SCAN_PASSIVE, GAP_SCAN_INTERVAL,
                       GAP_SCAN_WINDOW, GAP_SCAN_ALL );
  gap_start_scan();
}

void Client::connect()
{
  debug( "[BLE] Connecting to address %s...\n",
         bd_addr_to_str( server_addr ) );
  state = TC_W4_CONNECT;
  gap_connect( server_addr, server_addr_type );
}

void Client::service_discovery()
{
  debug( "[BLE] Entering service discovery...\n" );
  if ( !gatt_client_is_ready( connection_handle ) ) {
    debug( "[BLE] ============ CLIENT NOT READY ============\n" );
  }
  state = TC_W4_SERVICE_RESULT;
  gatt_client_discover_primary_services( gatt_client_event_callback,
                                         connection_handle );
}

void Client::characteristic_discovery()
{
  debug( "[BLE] Discovering characteristics for service %d...\n",
         curr_service_idx );
  if ( !gatt_client_is_ready( connection_handle ) ) {
    debug( "[BLE] ============ CLIENT NOT READY ============\n" );
  }
  state         = TC_W4_CHARACTERISTIC_RESULT;
  curr_char_idx = 0;
  gatt_client_discover_characteristics_for_service(
      gatt_client_event_callback, connection_handle,
      &server_service[curr_service_idx] );
}

void Client::characteristic_descriptor_discovery()
{
  debug( "[BLE] Discovering descriptor for characteristic %d...\n",
         curr_char_idx + curr_total_char_idx );
  if ( !gatt_client_is_ready( connection_handle ) ) {
    debug( "[BLE] ============ CLIENT NOT READY ============\n" );
  }
  state = TC_W4_CHARACTERISTIC_DESCRIPTOR;
  gatt_client_discover_characteristic_descriptors(
      gatt_client_event_callback, connection_handle,
      &server_characteristic[curr_char_idx + curr_total_char_idx] );
}

// Helper function to check which descriptor is the description
bool is_description( gatt_client_characteristic_descriptor_t descriptor )
{
  return descriptor.uuid16 == GATT_CHARACTERISTIC_USER_DESCRIPTION;
}

void Client::characteristic_description_discovery()
{
  debug( "[BLE] Discovering description for characteristic %d...\n",
         curr_char_idx + curr_total_char_idx );
  if ( !gatt_client_is_ready( connection_handle ) ) {
    debug( "[BLE] ============ CLIENT NOT READY ============\n" );
  }
  state = TC_W4_CHARACTERISTIC_DESCRIPTION;

  // Get the description from the correct descriptor
  while ( curr_char_idx <
          num_characteristics_discovered[curr_service_idx] ) {
    if ( is_description(
             server_characteristic_descriptor[curr_char_idx +
                                              curr_total_char_idx]
                                             [0] ) ) {
      // 0 has the description
      gatt_client_read_characteristic_descriptor(
          gatt_client_event_callback, connection_handle,
          &server_characteristic_descriptor[curr_char_idx +
                                            curr_total_char_idx][0] );
      return;
    }
    else if ( is_description(
                  server_characteristic_descriptor[curr_char_idx +
                                                   curr_total_char_idx]
                                                  [1] ) ) {
      // 1 has the description
      gatt_client_read_characteristic_descriptor(
          gatt_client_event_callback, connection_handle,
          &server_characteristic_descriptor[curr_char_idx +
                                            curr_total_char_idx][1] );
      return;
    }
    else {
      curr_char_idx++;
    }
  }

  // No descriptions to discover
  curr_char_idx       = 0;
  curr_char_descr_idx = 0;
  read_characteristic_value();
}

void Client::read_characteristic_value()
{
  debug( "[BLE] Discovering value for characteristic %d...\n",
         curr_char_idx + curr_total_char_idx );
  if ( !gatt_client_is_ready( connection_handle ) ) {
    debug( "[BLE] ============ CLIENT NOT READY ============\n" );
  }
  state = TC_W4_CHARACTERISTIC_VALUE;
  while ( curr_char_idx <
          num_characteristics_discovered[curr_service_idx] ) {
    if ( ( server_characteristic[curr_char_idx + curr_total_char_idx]
               .properties &
           ATT_PROPERTY_READ ) != 0 ) {
      gatt_client_read_value_of_characteristic(
          gatt_client_event_callback, connection_handle,
          &server_characteristic[curr_char_idx + curr_total_char_idx] );
      return;
    }
    else {
      curr_char_idx++;
    }
  }

  // No properties to read
  curr_char_idx       = 0;
  curr_char_descr_idx = 0;
  read_characteristic_config();
}

void Client::read_characteristic_config()
{
  state = TC_W4_CHARACTERISTIC_CONFIG;
  if ( !gatt_client_is_ready( connection_handle ) ) {
    debug( "[BLE] ============ CLIENT NOT READY ============\n" );
  }

  // Find next descriptor for a configuration
  while ( ( curr_char_idx <
            num_characteristics_discovered[curr_service_idx] ) &&
          ( server_characteristic_descriptor[curr_char_idx +
                                             curr_total_char_idx][0]
                .uuid16 != GATT_CLIENT_CHARACTERISTICS_CONFIGURATION ) ) {
    curr_char_idx++;
  }

  // If we found one, get its configuration
  if ( curr_char_idx <
       num_characteristics_discovered[curr_service_idx] ) {
    debug( "[BLE] Discovering configuration for characteristic %d...\n",
           curr_char_idx + curr_total_char_idx );
    gatt_client_read_characteristic_descriptor(
        gatt_client_event_callback, connection_handle,
        &server_characteristic_descriptor[curr_char_idx +
                                          curr_total_char_idx][0] );
    return;
  }

  // If none left to get, check for remaining services
  curr_total_char_idx += num_characteristics_discovered[curr_service_idx];
  curr_service_idx++;
  if ( curr_service_idx < num_services_discovered ) {
    characteristic_discovery();
    return;
  }

  // If none left to get, move to notifications
  debug( "[BLE] All characteristics discovered!\n" );
  state               = TC_W4_READY;
  listener_registered = true;
  gatt_client_listen_for_characteristic_value_updates(
      &notification_listener, gatt_client_event_callback,
      connection_handle, NULL );
  after_discovery();
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
  // Check if it's a notification or indication
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  uint8_t type_of_packet = hci_event_packet_get_type( packet );
  if ( type_of_packet == GATT_EVENT_NOTIFICATION ) {
    gatt_client_notification_handler( packet );
    return;
  }
  if ( type_of_packet == GATT_EVENT_INDICATION ) {
    gatt_client_indication_handler( packet );
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
          gatt_event_service_query_result_get_service(
              packet, &( server_service[curr_service_idx++] ) );
          break;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Finished with service result (discover characteristics)
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case GATT_EVENT_QUERY_COMPLETE:
          num_services_discovered = curr_service_idx;
          curr_service_idx        = 0;

          // Make sure no errors
          att_status = gatt_event_query_complete_get_att_status( packet );
          if ( att_status != ATT_ERROR_SUCCESS ) {
            printf( "SERVICE_QUERY_RESULT, ATT Error 0x%02x (%s:%d).\n",
                    att_status, __FILE__, __LINE__ );
            gap_disconnect( connection_handle );
            disconnect_from_server();
            break;
          }
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
          server_characteristic_service_idx[curr_char_idx +
                                            curr_total_char_idx] =
              curr_service_idx;
          gatt_event_characteristic_query_result_get_characteristic(
              packet, &server_characteristic[curr_char_idx +
                                             curr_total_char_idx] );

          // Hacky fix for BTStack issue with notifications later
          //  - https://github.com/bluekitchen/btstack/issues/678
          server_characteristic[curr_char_idx + curr_total_char_idx]
              .end_handle =
              server_characteristic[curr_char_idx + curr_total_char_idx]
                  .value_handle +
              1;

          curr_char_idx++;
          break;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Done with characteristics (move to descriptors)
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case GATT_EVENT_QUERY_COMPLETE:
          num_characteristics_discovered[curr_service_idx] =
              curr_char_idx;
          total_characteristics_discovered += curr_char_idx;

          curr_char_idx       = 0;
          curr_char_descr_idx = 0;

          // Make sure no errors
          att_status = gatt_event_query_complete_get_att_status( packet );
          if ( att_status != ATT_ERROR_SUCCESS ) {
            printf( "SERVICE_QUERY_RESULT, ATT Error 0x%02x (%s:%d).\n",
                    att_status, __FILE__, __LINE__ );
            gap_disconnect( connection_handle );
            disconnect_from_server();
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
              &server_characteristic_descriptor[curr_char_idx +
                                                curr_total_char_idx]
                                               [curr_char_descr_idx++] );
          break;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Done with descriptors
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case GATT_EVENT_QUERY_COMPLETE:
          curr_char_idx++;
          curr_char_descr_idx = 0;

          att_status = gatt_event_query_complete_get_att_status( packet );
          if ( att_status != ATT_ERROR_SUCCESS ) {
            printf( "SERVICE_QUERY_RESULT, ATT Error 0x%02x (%s:%d).\n",
                    att_status, __FILE__, __LINE__ );
            gap_disconnect( connection_handle );
            disconnect_from_server();
            break;
          }

          // Discover next characteristic, if any remaining
          if ( curr_char_idx <
               num_characteristics_discovered[curr_service_idx] ) {
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
          memcpy(
              server_characteristic_user_description[curr_char_idx +
                                                     curr_total_char_idx],
              description, description_length );
          server_characteristic_user_description[curr_char_idx +
                                                 curr_total_char_idx]
                                                [description_length] =
                                                    '\0';
          break;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Done with description
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case GATT_EVENT_QUERY_COMPLETE:
          att_status = gatt_event_query_complete_get_att_status( packet );
          if ( att_status != ATT_ERROR_SUCCESS ) {
            printf( "SERVICE_QUERY_RESULT, ATT Error 0x%02x (%s:%d).\n",
                    att_status, __FILE__, __LINE__ );
            gap_disconnect( connection_handle );
            disconnect_from_server();
            break;
          }
          curr_char_idx++;

          // Read remaining descriptions, if any
          if ( curr_char_idx <
               num_characteristics_discovered[curr_service_idx] ) {
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
          memcpy( server_characteristic_values[curr_char_idx +
                                               curr_total_char_idx],
                  value, value_length );
          server_characteristic_values[curr_char_idx +
                                       curr_total_char_idx]
                                      [value_length] = '\0';
          server_characteristic_value_lengths[curr_char_idx +
                                              curr_total_char_idx] =
              value_length;
          break;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Done with value
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case GATT_EVENT_QUERY_COMPLETE:
          att_status = gatt_event_query_complete_get_att_status( packet );
          if ( att_status != ATT_ERROR_SUCCESS ) {
            printf( "SERVICE_QUERY_RESULT, ATT Error 0x%02x (%s:%d).\n",
                    att_status, __FILE__, __LINE__ );
            gap_disconnect( connection_handle );
            disconnect_from_server();
            break;
          }
          curr_char_idx++;

          // Read remaining values, if any
          if ( curr_char_idx <
               num_characteristics_discovered[curr_service_idx] ) {
            read_characteristic_value();
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
          server_characteristic_configurations[curr_char_idx +
                                               curr_total_char_idx] =
              little_endian_read_16( config, 0 );
          break;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Done with configuration
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case GATT_EVENT_QUERY_COMPLETE:
          att_status = gatt_event_query_complete_get_att_status( packet );
          if ( att_status != ATT_ERROR_SUCCESS ) {
            printf( "SERVICE_QUERY_RESULT, ATT Error 0x%02x (%s:%d).\n",
                    att_status, __FILE__, __LINE__ );
            gap_disconnect( connection_handle );
            disconnect_from_server();
            break;
          }
          curr_char_idx++;
          read_characteristic_config();
          break;

        default:
          break;
      }
      break;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finished Discovery - give to child
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case TC_W4_READY:
      child_gatt_event_handler( type_of_packet, packet );
      break;

    default:
      break;
  }
}

// -----------------------------------------------------------------------
// gatt_client_notification_handler
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
  debug( "[BLE] Received notification for 0x%X...\n", value_handle );

  // Find the characteristic that the value is for
  int value_char_idx = -1;
  for ( int i = 0; i < total_characteristics_discovered; i++ ) {
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

  // Call custom notification handler
  notification_handler( value_handle, value, value_length );
}

// -----------------------------------------------------------------------
// gatt_client_indication_handler
// -----------------------------------------------------------------------
// Handle asynchronous indications

void Client::gatt_client_indication_handler( uint8_t* packet )
{
  // Get payload length, value handle, and value
  uint32_t value_length =
      gatt_event_indication_get_value_length( packet );
  uint16_t value_handle =
      gatt_event_indication_get_value_handle( packet );
  const uint8_t* value = gatt_event_indication_get_value( packet );
  debug( "[BLE] Received indication for 0x%X...\n", value_handle );

  // Find the characteristic that the value is for
  int value_char_idx = -1;
  for ( int i = 0; i < total_characteristics_discovered; i++ ) {
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

  // Call custom notification handler
  indication_handler( value_handle, value, value_length );
}

// -----------------------------------------------------------------------
// Default custom handlers
// -----------------------------------------------------------------------

void Client::notification_handler( uint16_t       value_handle,
                                   const uint8_t* value,
                                   uint32_t       value_length )
{
  // Unused value + length
  (void) value;
  (void) value_length;
  debug( "[BLE] Received notification for 0x%X!\n", value_handle );
}

void Client::indication_handler( uint16_t       value_handle,
                                 const uint8_t* value,
                                 uint32_t       value_length )
{
  // Unused value + length
  (void) value;
  (void) value_length;
  debug( "[BLE] Received indication for 0x%X!\n", value_handle );
}

// -----------------------------------------------------------------------
// Helper functions for enabling notifications/indications
// -----------------------------------------------------------------------

bool uuid_eq( service_uuid_t                      uuid,
              const gatt_client_characteristic_t& characteristic )
{
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // UUID16
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  if ( std::holds_alternative<uint16_t>( uuid ) ) {
    uint16_t uuid16 = std::get<uint16_t>( uuid );
    return ( uuid16 == characteristic.uuid16 );
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // UUID128
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  const uint8_t* uuid128 = std::get<const uint8_t*>( uuid );
  for ( int i = 0; i < 16; i++ ) {
    if ( uuid128[i] != characteristic.uuid128[i] ) {
      return false;
    }
  }
  return true;
};

int Client::enable_notifications( service_uuid_t uuid )
{
  if ( state != TC_W4_READY )
    return -1;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Iterate to find correct characteristic
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  for ( int idx = 0; idx < total_characteristics_discovered; idx++ ) {
    if ( uuid_eq( uuid, server_characteristic[idx] ) ) {
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Check if notifications are available
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      if ( ( server_characteristic[idx].properties &
             ATT_PROPERTY_NOTIFY ) == 0 ) {
        debug(
            "[BLE] Tried to enable notifications for 0x%X when not available...\n",
            server_characteristic[idx].value_handle );
        return -1;
      }

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Enable notifications
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      debug( "[BLE] Enabling notifications for value handle 0x%X...\n",
             server_characteristic[idx].value_handle );
      uint8_t status =
          gatt_client_write_client_characteristic_configuration(
              gatt_client_event_callback, connection_handle,
              &server_characteristic[idx],
              GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION );
      return status;
    }
  }

  debug(
      "[BLE] Tried to enable notifications for non-discovered UUID: " );
  if ( std::holds_alternative<uint16_t>( uuid ) ) {
    debug( "0x%X...\n", std::get<uint16_t>( uuid ) );
  }
  else {  // UUID128
    debug( "%s...\n",
           uuid128_to_str( std::get<const uint8_t*>( uuid ) ) );
  }
  return -1;
}

int Client::enable_indications( service_uuid_t uuid )
{
  if ( state != TC_W4_READY )
    return -1;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Iterate to find correct characteristic
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  for ( int idx = 0; idx < total_characteristics_discovered; idx++ ) {
    if ( uuid_eq( uuid, server_characteristic[idx] ) ) {
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Check if indications are available
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      if ( ( server_characteristic[idx].properties &
             ATT_PROPERTY_INDICATE ) == 0 ) {
        debug(
            "[BLE] Tried to enable indications for 0x%X when not available...",
            server_characteristic[idx].value_handle );
        return -1;
      }

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Enable indications
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      uint8_t status =
          gatt_client_write_client_characteristic_configuration(
              gatt_client_event_callback, connection_handle,
              &server_characteristic[idx],
              GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_INDICATION );
      return status;
    }
  }

  debug(
      "[BLE] Tried to enable notifications for non-discovered UUID: " );
  if ( std::holds_alternative<uint16_t>( uuid ) ) {
    debug( "0x%X...\n", std::get<uint16_t>( uuid ) );
  }
  else {  // UUID128
    debug( "%s...\n",
           uuid128_to_str( std::get<const uint8_t*>( uuid ) ) );
  }
  return -1;
}

int Client::disable_notifications_indications( service_uuid_t uuid )
{
  if ( state != TC_W4_READY )
    return -1;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Iterate to find correct characteristic
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  for ( int idx = 0; idx < total_characteristics_discovered; idx++ ) {
    if ( uuid_eq( uuid, server_characteristic[idx] ) ) {
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Disable any previously-enabled characteristics
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      uint8_t status =
          gatt_client_write_client_characteristic_configuration(
              gatt_client_event_callback, connection_handle,
              &server_characteristic[idx],
              GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NONE );
      return status;
    }
  }

  debug(
      "[BLE] Tried to disable characteristics for non-discovered UUID: " );
  if ( std::holds_alternative<uint16_t>( uuid ) ) {
    debug( "0x%X...\n", std::get<uint16_t>( uuid ) );
  }
  else {  // UUID128
    debug( "%s...\n",
           uuid128_to_str( std::get<const uint8_t*>( uuid ) ) );
  }
  return -1;
}

uint16_t Client::value_handle_from_uuid( service_uuid_t uuid )
{
  for ( int idx = 0; idx < total_characteristics_discovered; idx++ ) {
    if ( uuid_eq( uuid, server_characteristic[idx] ) ) {
      return server_characteristic[idx].value_handle;
    }
  }
  debug( "[BLE] UUID not found: " );
  if ( std::holds_alternative<uint16_t>( uuid ) ) {
    debug( "0x%X...\n", std::get<uint16_t>( uuid ) );
  }
  else {  // UUID128
    debug( "%s...\n",
           uuid128_to_str( std::get<const uint8_t*>( uuid ) ) );
  }
  return -1;
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

  uint8_t   event_type = hci_event_packet_get_type( packet );
  bd_addr_t local_addr;

  switch ( event_type ) {
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Startup
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case BTSTACK_EVENT_STATE:
      // Start listening if the chip is working
      if ( btstack_event_state_get_state( packet ) ==
           HCI_STATE_WORKING ) {
        gap_local_bd_addr( local_addr );
        debug( "[BLE] Up and running on %s...\n",
               bd_addr_to_str( local_addr ) );
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

      // Initiate pairing
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
      debug( "[BLE] Disconnecting from %s...\n",
             bd_addr_to_str( server_addr ) );
      disconnect_from_server();

      // If we're not off, start listening again
      if ( state != TC_OFF & should_reconnect() ) {
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

  int service_idx = -1;
  for ( int idx = 0; idx < total_characteristics_discovered; idx++ ) {
    if ( service_idx != server_characteristic_service_idx[idx] ) {
      service_idx = server_characteristic_service_idx[idx];
      printf( "Service %d:\n", service_idx );
      printf( " - UUID128: %s\n",
              uuid128_to_str( server_service[service_idx].uuid128 ) );
    }
    printf( " - Characteristic %d:\n", idx );
    printf( "    - UUID128: %s\n",
            uuid128_to_str( server_characteristic[idx].uuid128 ) );
    printf( "    - Description: %s\n",
            server_characteristic_user_description[idx] );

    printf( "    - Permissions: " );
    print_permissions( server_characteristic[idx].properties );
    printf( "\n" );

    printf( "    - Range: 0x%X - 0x%X\n",
            server_characteristic[idx].start_handle,
            server_characteristic[idx].end_handle );

    printf( "    - Value Handle: " );
    printf( "0x%X", server_characteristic[idx].value_handle );
    printf( "\n" );

    printf( "    - Value: 0x" );
    for ( int i = 0; i < server_characteristic_value_lengths[idx]; i++ ) {
      printf( "%X", server_characteristic_values[idx][i] );
    }
    printf( "\n" );
  }
}