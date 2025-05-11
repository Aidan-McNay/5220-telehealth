// rijndael.h
#ifndef H__RIJNDAEL
#define H__RIJNDAEL

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int  rijndaelSetupEncrypt(uint32_t *rk, const uint8_t *key, int keybits);
int  rijndaelSetupDecrypt(uint32_t *rk, const uint8_t *key, int keybits);
void rijndaelEncrypt    (const uint32_t *rk, int nrounds,
                         const uint8_t plaintext[16],
                         uint8_t ciphertext[16]);
void rijndaelDecrypt    (const uint32_t *rk, int nrounds,
                         const uint8_t ciphertext[16],
                         uint8_t plaintext[16]);

#define KEYBITS   128
#define KEYLENGTH(keybits)  ((keybits)/8)
#define RKLENGTH(keybits)   ((keybits)/8 + 28)
#define NROUNDS(keybits)    ((keybits)/32 + 6)

#ifdef __cplusplus
}
#endif

#endif  // H__RIJNDAEL
