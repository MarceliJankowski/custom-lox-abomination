#ifndef TERMINAL_H
#define TERMINAL_H

/**@desc enable noncannonical terminal mode.
Once enabled, it persists until process termination.
This function registers handlers for terminal parameter restoration upon imminent process termination.
Only single invocation is permitted.*/
void terminal_enable_noncannonical_mode(void);

#endif // TERMINAL_H
