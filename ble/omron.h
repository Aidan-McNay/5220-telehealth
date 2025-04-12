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
  uint16_t       get_service() override;
  const uint8_t* get_service_name() override;
};

#endif  // BLE_OMRON_H