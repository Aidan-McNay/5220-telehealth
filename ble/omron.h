// =======================================================================
// omron.h
// =======================================================================
// Declarations for our OMRON BLE class

#ifndef BLE_OMRON_H
#define BLE_OMRON_H

#include "ble/client.h"

enum omron_state_t {
  OM_IDLE = 0,
  OM_ENABLE_NOTIF,
  OM_READY,
};

class Omron : public Client {
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // BLE Definitions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void after_discovery() override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Helper functions for state machine transitions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void enable_notif();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Helper public functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 public:
  bool omron_ready();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Checking service
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 protected:
  omron_state_t omron_state;
  bool          correct_service_name( const uint8_t* service_name );
  bool          correct_service( uint8_t* advertisement_report ) override;
};

#endif  // BLE_OMRON_H