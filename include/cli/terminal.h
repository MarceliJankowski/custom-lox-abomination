#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdbool.h>
#include <stddef.h>

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
  TERMINAL_KEY_CTRL_D,
  TERMINAL_KEY_CTRL_H,
  TERMINAL_KEY_BACKSPACE,
  TERMINAL_KEY_DELETE,
  TERMINAL_KEY_CTRL_W,
  TERMINAL_KEY_ALT_B,
  TERMINAL_KEY_ALT_D,
  TERMINAL_KEY_ALT_F,
  TERMINAL_KEY_CTRL_U,
  TERMINAL_KEY_CTRL_K,
  TERMINAL_KEY_ARROW_UP,
  TERMINAL_KEY_ARROW_DOWN,
  TERMINAL_KEY_ARROW_LEFT,
  TERMINAL_KEY_ARROW_RIGHT,
  TERMINAL_KEY_CTRL_ARROW_UP,
  TERMINAL_KEY_CTRL_ARROW_DOWN,
  TERMINAL_KEY_CTRL_ARROW_LEFT,
  TERMINAL_KEY_CTRL_ARROW_RIGHT,
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

/**@desc clear all characters from current terminal line (resets cursor position appropriately)*/
void terminal_clear_current_line(void);

/**@desc move terminal cursor to a column located at `index` on the current line (`index` must be nonnegative)*/
void terminal_move_cursor_to_column(int index);

#endif // TERMINAL_H
