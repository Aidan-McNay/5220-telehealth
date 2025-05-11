// =======================================================================
// confirm.h
// =======================================================================
// A thin wrapper to allow for LoRaWAN confirmations

#ifndef LORAWAN_CONFIRM_H
#define LORAWAN_CONFIRM_H

#ifdef __cplusplus
extern "C" {
#endif

void confirm();  // Call on a confirmation
void on_confirm( void ( *callback )() );

#ifdef __cplusplus
}
#endif

#endif  // LORAWAN_CONFIRM_H