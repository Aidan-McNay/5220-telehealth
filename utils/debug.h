// =======================================================================
// debug.h
// =======================================================================
// A common macro for printing debug information

#ifdef DEBUG
#include <stdio.h>
#define debug( ... ) printf( __VA_ARGS__ )
#else
#define debug( ... )
#endif