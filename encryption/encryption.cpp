// =======================================================================
// encryption.cpp
// =======================================================================
// Definitions for functions in encryption.h

#include "encryption.h"

void aes128_encrypt_6byte_msg(const uint8_t key[16], const uint8_t msg[6], uint8_t ciphertext[16]){
   uint32_t round_keys[RKLENGTH(KEYBITS)];
   uint8_t padded_plaintext[16] = {0};  // Zero-padded buffer
   
   // Copy the 6-byte message into the first 6 bytes of the padded plaintext
   memcpy(padded_plaintext, msg, 6);
   
   // Prepare the encryption round keys
   rijndaelSetupEncrypt(round_keys, key, KEYBITS);
   
   // Encrypt the padded message
   rijndaelEncrypt(round_keys, NROUNDS(KEYBITS), padded_plaintext, ciphertext);
}