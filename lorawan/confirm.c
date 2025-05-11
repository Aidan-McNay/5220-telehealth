// =======================================================================
// confirm.cpp
// =======================================================================
// A thin wrapper to allow for LoRaWAN confirmations

#include "confirm.h"
#include <stddef.h>

void ( *curr_callback )() = NULL;

void on_confirm( void ( *callback )() )
{
  curr_callback = callback;
}

void confirm()
{
  if ( curr_callback != NULL ) {
    curr_callback();
  }
}