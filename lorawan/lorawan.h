// =======================================================================
// lorawan.h
// =======================================================================
// Declarations of our LoRaWAN interface

#ifndef LORAWAN_LORAWAN_H
#define LORAWAN_LORAWAN_H

#include "lorawan/lorawan_config.h"
#include <cstdint>

class LoRaWAN {
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Public Accessor Functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 public:
  LoRaWAN();

  // Return whether joining is successful
  bool try_join();

  // Return whether sending the message is successful
  bool try_send( const uint8_t* data, uint8_t data_len );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Protected Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 protected:
  bool join_started;
  bool msg_sent;
};

#endif  // LORAWAN_LORAWAN_H