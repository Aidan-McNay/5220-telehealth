// =======================================================================
// encryption.h
// =======================================================================
// Declarations for encryption

#ifndef ENCRYPTION_ENCRYPTION_H
#define ENCRYPTION_ENCRYPTION_H

#include rijndael.h
#include <stdint.h>
#include <string.h>

// -----------------------------------------------------------------------
// Encryption function
// -----------------------------------------------------------------------
void aes128_encrypt_6byte_msg(const uint8_t key[16], const uint8_t msg[6], uint8_t ciphertext[16]);

#endif
