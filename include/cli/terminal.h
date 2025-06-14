#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdbool.h>

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

/**@desc get `terminal_key` type
@param terminal_key TerminalKey object
@result type of `terminal_key`*/
#define TERMINAL_KEY_GET_TYPE(terminal_key) ((terminal_key).control.type)

// *---------------------------------------------*
// *              TYPE DEFINITIONS               *
// *---------------------------------------------*

typedef enum {
  TERMINAL_KEY_UNKNOWN,
  TERMINAL_KEY_PRINTABLE,
  TERMINAL_KEY_EOF,
  TERMINAL_KEY_BACKSPACE,
  TERMINAL_KEY_ARROW_UP,
  TERMINAL_KEY_ARROW_DOWN,
  TERMINAL_KEY_ARROW_LEFT,
  TERMINAL_KEY_ARROW_RIGHT,
  TERMINAL_KEY_TYPE_COUNT,
} TerminalKeyType;

typedef union {
  struct {
    TerminalKeyType type;
  } control;
  struct {
    TerminalKeyType type;
    char character;
  } printable;
} TerminalKey;

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

/**@desc enable noncannonical terminal mode (unless stdin is not connected to a terminal).
Once enabled, it persists until process termination.
This function registers handlers for terminal parameter restoration upon imminent process termination.
Invocations are permitted until noncannonical terminal mode gets enabled.
@return true if noncannonical terminal mode was enabled, false otherwise (stdin is not connected to a terminal)*/
bool terminal_enable_noncannonical_mode(void);

/**@desc read key press from terminal's command-line interface (only supports ASCII)
@return key that was read*/
TerminalKey terminal_read_key(void);

#endif // TERMINAL_H
