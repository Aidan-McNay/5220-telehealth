// =======================================================================
// key.h
// =======================================================================
// The key to use for LoRaWAN
//
// The current key is used for testing, and is assumed to be unsafe

#ifndef ENCRYPTION_KEY_H
#define ENCRYPTION_KEY_H

uint8_t lorawan_key[16] = { 0xe7, 0xa5, 0xc3, 0xf2, 0xd4, 0x8a,
                            0x0e, 0x3b, 0xc9, 0x61, 0x17, 0xb5,
                            0xfd, 0xfb, 0xa2, 0x47 };

#endif  // ENCRYPTION_KEY_H