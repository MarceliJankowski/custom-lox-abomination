#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdbool.h>

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

/**@desc enable noncannonical terminal mode (unless stdin is not connected to a terminal).
Once enabled, it persists until process termination.
This function registers handlers for terminal parameter restoration upon imminent process termination.
Invocations are permitted until noncannonical terminal mode gets enabled.
@return true if noncannonical terminal mode was enabled, false otherwise (stdin is not connected to a terminal)*/
bool terminal_enable_noncannonical_mode(void);

#endif // TERMINAL_H
