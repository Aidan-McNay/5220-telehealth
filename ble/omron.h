// =======================================================================
// omron.h
// =======================================================================
// Declarations for our OMRON BLE class

#ifndef BLE_OMRON_H
#define BLE_OMRON_H

#include "ble/client.h"

class Omron : public Client {
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // BLE Definitions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  service_uuid_t get_service_name() override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Checking service
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 protected:
  bool correct_service_name( const uint8_t* service_name );
  bool correct_service( uint8_t* advertisement_report ) override;
};

#endif  // BLE_OMRON_H