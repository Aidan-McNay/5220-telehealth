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
  TC_W4_CHARACTERISTIC_DESCRIPTOR_PRINT,
  TC_W4_CHARACTERISTIC_VALUE_PRINT,
  TC_W4_CHARACTERISTIC_NOTIFICATIONS,
  TC_W4_ENABLE_NOTIFICATIONS_COMPLETE,
  TC_W4_READY
};

// -----------------------------------------------------------------------
// BLE Scan Parameters
// -----------------------------------------------------------------------

enum gap_scan_type_t { GAP_SCAN_PASSIVE = 0, GAP_SCAN_ACTIVE = 1 };
enum gap_scan_policy_t { GAP_SCAN_ALL = 0, GAP_SCAN_WHITELIST = 1 };

// -----------------------------------------------------------------------
// Client
// -----------------------------------------------------------------------

class Client {
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // BLE Definitions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Child classes will need to override these

  virtual uint16_t get_service()      = 0;
  virtual uint8_t* get_service_name() = 0;  // 16 entries for 128b UUID

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Public accessor functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Client();
  ~Client();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Protected attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 protected:
  gc_state_t state;

  // Address of server we're connected to
  bd_addr_t      server_addr;
  bd_addr_type_t server_addr_type;

  // Current connection handle
  hci_con_handle_t connection_handle;

  // Listener
  bool                       listener_registered;
  gatt_client_notification_t notification_listener;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Protected functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 protected:
  bool correct_service( uint8_t* advertisement_report );

  // Helper functions for state transitions
  void off();
  void start();
  void connect();
  void service_discovery();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Event handlers
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 public:
  void gatt_client_event_handler( uint8_t packet_type, uint16_t channel,
                                  uint8_t* packet, uint16_t size );
  void hci_event_handler( uint8_t packet_type, uint16_t channel,
                          uint8_t* packet, uint16_t size );
};

#endif  // BLE_CLIENT_H