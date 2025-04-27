// =======================================================================
// omron.h
// =======================================================================
// Declarations for our OMRON BLE class

#ifndef BLE_OMRON_H
#define BLE_OMRON_H

#include "ble/client.h"

enum omron_state_t {
  OM_IDLE = 0,
  OM_PAIR,
  OM_PAIR_NOTIFICATION,
  OM_PAIR_UNLOCK,
  OM_PAIR_WRITE_KEY,
  OM_PAIR_DISABLE_NOTIFICATION,
  OM_READY,
};

enum omron_poll_event_t {
  NONE,
  PAIR_WRITE_KEY,
};

class Omron : public Client {
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // BLE Definitions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void after_discovery() override;
  void child_gatt_event_handler( uint8_t  packet_type,
                                 uint8_t* packet ) override;
  void notification_handler( uint16_t value_handle, const uint8_t* value,
                             uint32_t value_length ) override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Helper functions for state machine transitions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void pair();
  void pair_notification();
  void pair_unlock();
  void pair_write_key();
  void pair_disable_notification();
  void blood_pressure_ready();

  // Helper for polling asynchronous tasks (public scope, but shouldn't
  // be used publicly)
 public:
  omron_poll_event_t poll_event;
  void               poll();  // Return whether the event was handled

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Helper public functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 public:
  Omron();
  bool omron_ready();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Checking service
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 protected:
  omron_state_t                          omron_state;
  btstack_packet_callback_registration_t sm_event_callback_registration;
  bool correct_service_name( const uint8_t* service_name );
  bool correct_service( uint8_t* advertisement_report ) override;

 public:
  void sm_event_handler( uint8_t packet_type, uint16_t channel,
                         uint8_t* packet, uint16_t size );
};

#endif  // BLE_OMRON_H