// =======================================================================
// omron.cpp
// =======================================================================
// Definitions of our OMRON BLE class

#include "ble/omron.h"

// -----------------------------------------------------------------------
// Service identifiers
// -----------------------------------------------------------------------

uint16_t      service          = 0x3980;
const uint8_t service_name[16] = { 0xEC, 0xBE, 0x39, 0x80, 0xC9, 0xA2,
                                   0x11, 0xE1, 0xB1, 0xBD, 0x00, 0x02,
                                   0xA5, 0xD5, 0xC5, 0x1B };

uint16_t Omron::get_service()
{
  return service;
}

const uint8_t* Omron::get_service_name()
{
  return service_name;
}