// =======================================================================
// client.h
// =======================================================================
// Declarations of our BLE client functions
//
// Note that users should only ever construct ONE client, due to the
// global callback function used for GATT events

#ifndef BLE_CLIENT_H
#define BLE_CLIENT_H

#include "btstack.h"
#include <cstdint>
#include <variant>

// Maximum length of a BLE Characteristic Description
#define GATT_MAX_DESCRIPTION_LENGTH 50

// Maximum length of a BLE Characteristic Value
#define GATT_MAX_VALUE_LENGTH 100

// Maximum number of services - adjust if necessary
#define MAX_SERVICES 7

// Maximum number of characteristics - adjust if necessary
#define MAX_CHARACTERISTICS 35

// -----------------------------------------------------------------------
// GATT State Machine States
// -----------------------------------------------------------------------

enum gc_state_t {
  TC_OFF = 0,
  TC_IDLE,
  TC_W4_SCAN_RESULT,
  TC_W4_CONNECT,
  TC_W4_SERVICE_RESULT,
  TC_W4_CHARACTERISTIC_RESULT,
  TC_W4_CHARACTERISTIC_DESCRIPTOR,
  TC_W4_CHARACTERISTIC_DESCRIPTION,
  TC_W4_CHARACTERISTIC_VALUE,
  TC_W4_CHARACTERISTIC_CONFIG,
  TC_W4_ENABLE_NOTIFICATIONS_COMPLETE,
  TC_W4_READY
};

// -----------------------------------------------------------------------
// BLE Scan Parameters
// -----------------------------------------------------------------------

enum gap_scan_type_t { GAP_SCAN_PASSIVE = 0, GAP_SCAN_ACTIVE = 1 };
enum gap_scan_policy_t { GAP_SCAN_ALL = 0, GAP_SCAN_WHITELIST = 1 };

// -----------------------------------------------------------------------
// Two descriptors per characteristic
// -----------------------------------------------------------------------

typedef gatt_client_characteristic_descriptor_t
    gatt_client_characteristic_descriptors_t[2];

// -----------------------------------------------------------------------
// Client
// -----------------------------------------------------------------------

typedef std::variant<uint16_t, const uint8_t*> service_uuid_t;

class Client {
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // BLE Definitions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Child classes will need to override these
 protected:
  // Override based on custom device to determine correct service
  virtual bool correct_service( uint8_t* advertisement_report ) = 0;

  // Override to call once all setup is done
  virtual void after_discovery() = 0;

  // Handle GATT events after discovery
  virtual void child_gatt_event_handler( uint8_t  packet_type,
                                         uint8_t* packet ) = 0;

  // Override to handle notifications/indications
  virtual void notification_handler( uint16_t       value_handle,
                                     const uint8_t* value,
                                     uint32_t       value_length );
  virtual void indication_handler( uint16_t       value_handle,
                                   const uint8_t* value,
                                   uint32_t       value_length );
  // Whether to reconnect on a disconnection
  virtual bool should_reconnect() = 0;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Helper functions for children
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 protected:
  // Return 0 on success
  int      enable_notifications( service_uuid_t uuid );
  int      enable_indications( service_uuid_t uuid );
  int      disable_notifications_indications( service_uuid_t uuid );
  uint16_t value_handle_from_uuid( service_uuid_t uuid );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Public accessor functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 public:
  Client();
  ~Client();

  // Connecting to/from servers
  void connect_to_server();
  void disconnect_from_server();

  // Check whether we're done discovery
  bool ready();

  // Print all characteristics
  //  - Does nothing if discovery isn't completed
  void print();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Protected attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 protected:
  gc_state_t                             state;
  btstack_packet_callback_registration_t hci_event_callback_registration;

  // Address of server we're connected to
  bd_addr_t      server_addr;
  bd_addr_type_t server_addr_type;

  // Current connection handle
  hci_con_handle_t connection_handle;

  // Service information
  gatt_client_service_t server_service[MAX_SERVICES];
  int                   num_services_discovered;

  // Listener
  bool                       listener_registered;
  gatt_client_notification_t notification_listener;

  // Characteristics
  gatt_client_characteristic_t server_characteristic[MAX_CHARACTERISTICS];
  int server_characteristic_service_idx[MAX_CHARACTERISTICS];
  gatt_client_characteristic_descriptors_t
      server_characteristic_descriptor[MAX_CHARACTERISTICS];
  uint8_t
      server_characteristic_user_description[MAX_CHARACTERISTICS]
                                            [GATT_MAX_DESCRIPTION_LENGTH];
  char server_characteristic_values[MAX_CHARACTERISTICS]
                                   [GATT_MAX_VALUE_LENGTH];
  uint32_t server_characteristic_value_lengths[MAX_CHARACTERISTICS];
  uint16_t server_characteristic_configurations[MAX_CHARACTERISTICS];
  int      num_characteristics_discovered[MAX_SERVICES];
  int      total_characteristics_discovered;

  // Characteristic helper values
  int curr_service_idx;
  int curr_char_idx;
  int curr_total_char_idx;
  int curr_char_descr_idx;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Protected functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 protected:
  // Helper functions for state transitions
  void reset();
  void off();
  void start();
  void connect();
  void service_discovery();
  void characteristic_discovery();
  void characteristic_descriptor_discovery();
  void characteristic_description_discovery();
  void read_characteristic_value();
  void read_characteristic_config();

  void ( *hci_event_callback )( uint8_t packet_type, uint16_t channel,
                                uint8_t* packet, uint16_t size );
  void ( *gatt_client_event_callback )( uint8_t  packet_type,
                                        uint16_t channel, uint8_t* packet,
                                        uint16_t size );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Event handlers
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 public:
  void gatt_client_event_handler( uint8_t packet_type, uint16_t channel,
                                  uint8_t* packet, uint16_t size );
  void gatt_client_notification_handler( uint8_t* packet );
  void gatt_client_indication_handler( uint8_t* packet );
  void hci_event_handler( uint8_t packet_type, uint16_t channel,
                          uint8_t* packet, uint16_t size );
};

#endif  // BLE_CLIENT_H