#ifdef _WIN32
// *---------------------------------------------*
// *                   WINDOWS                   *
// *---------------------------------------------*

#include "cli/terminal.h"

#include "utils/error.h"

#include <assert.h>
#include <windows.h>

// *---------------------------------------------*
// *          INTERNAL-LINKAGE OBJECTS           *
// *---------------------------------------------*

static DWORD original_console_mode;
static bool is_noncannonical_mode_enabled;
static HANDLE stdin_handle = INVALID_HANDLE_VALUE;

// *---------------------------------------------*
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/**@desc restore console mode to its original state.
Requires noncannonical mode to be enabled.*/
static void restore_console_mode(void) {
  assert(stdin_handle != INVALID_HANDLE_VALUE);
  assert(is_noncannonical_mode_enabled == true);

  // due to being used as atexit() handler this function cannot exit() (second exit would trigger UB)
  if (SetConsoleMode(stdin_handle, original_console_mode) == 0) error_windows_log_last();
}

/**@desc handle console `control_event_type`*/
static BOOL WINAPI console_control_event_handler(DWORD const control_event_type) {
  switch (control_event_type) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT: {
      restore_console_mode();
      break;
    }
  }

  return FALSE; // continue with default handling
}

/**@desc register console mode restoration handlers for appropriate console control events and atexit()*/
static void register_console_mode_restoration_handlers(void) {
  if (atexit(restore_console_mode) != 0) ERROR_SYSTEM("Failed to register atexit() handler");
  if (SetConsoleCtrlHandler(console_control_event_handler, TRUE) == 0) ERROR_WINDOWS_LAST();
}

// *---------------------------------------------*
// *         EXTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

bool terminal_enable_noncannonical_mode(void) {
  assert(is_noncannonical_mode_enabled == false);

  // get stdin handle and save it
  stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
  if (stdin_handle == INVALID_HANDLE_VALUE) ERROR_WINDOWS_LAST();

  // check if stdin is connected to a character device
  DWORD const stdin_file_type = GetFileType(stdin_handle);
  if (stdin_file_type != FILE_TYPE_CHAR) {
    if (stdin_file_type == FILE_TYPE_UNKNOWN && GetLastError() != NO_ERROR) ERROR_WINDOWS_LAST();
    return false;
  }

  // retrieve current console input mode and save it as original
  if (GetConsoleMode(stdin_handle, &original_console_mode) == 0) return false; // stdin is not connected to a console

  // create noncannonical_console_mode bitfield
  DWORD const noncannonical_console_mode =
    original_console_mode &
    ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT); // disable cannonical mode and echoing of input characters

  // enable noncannonical mode
  if (!SetConsoleMode(stdin_handle, noncannonical_console_mode)) ERROR_WINDOWS_LAST();
  is_noncannonical_mode_enabled = true;

  register_console_mode_restoration_handlers();

  return true;
}

#else
// *---------------------------------------------*
// *                    POSIX                    *
// *---------------------------------------------*
#define _POSIX_C_SOURCE 200809L

#include "cli/terminal.h"
#include "utils/error.h"

#include <assert.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

// *---------------------------------------------*
// *          INTERNAL-LINKAGE OBJECTS           *
// *---------------------------------------------*

static struct termios original_terminal_parameters;
static bool is_noncannonical_mode_enabled;

// *---------------------------------------------*
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/**@desc restore terminal parameters to their original state.
Requires noncannonical mode to be enabled.*/
static void restore_terminal_parameters(void) {
  assert(is_noncannonical_mode_enabled == true);

  // due to being used as atexit() handler this function cannot exit() (second exit would trigger UB)
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_terminal_parameters) == -1) fprintf(stderr, "%s\n", strerror(errno));
}

/**@desc handle `signal_num` signal*/
static void signal_handler(int const signal_num) {
  restore_terminal_parameters();

  // re-raise signal_num to trigger default signal_num handler
  if (raise(signal_num) != 0) ERROR_SYSTEM("Failed to re-raise '%d' signal", signal_num);
}

/**@desc register terminal parameter restoration handlers for appropriate signals and atexit()*/
static void register_terminal_parameter_restoration_handlers(void) {
  struct sigaction terminal_signal_action = {
    .sa_handler = signal_handler, // set signal handler
    .sa_flags = SA_RESETHAND, // restore default signal handler once signal is caught
  };

  // allow other signals to be delivered and processed while the handler is executing
  if (sigemptyset(&terminal_signal_action.sa_mask) == -1) ERROR_SYSTEM_ERRNO();

  // register handlers
  if (0 != atexit(restore_terminal_parameters)) ERROR_SYSTEM("Failed to register atexit() handler");
  if (-1 == sigaction(SIGINT, &terminal_signal_action, NULL)) ERROR_SYSTEM_ERRNO();
  if (-1 == sigaction(SIGTERM, &terminal_signal_action, NULL)) ERROR_SYSTEM_ERRNO();
  if (-1 == sigaction(SIGHUP, &terminal_signal_action, NULL)) ERROR_SYSTEM_ERRNO();
  if (-1 == sigaction(SIGQUIT, &terminal_signal_action, NULL)) ERROR_SYSTEM_ERRNO();
}

// *---------------------------------------------*
// *         EXTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

bool terminal_enable_noncannonical_mode(void) {
  assert(is_noncannonical_mode_enabled == false);

  // check if stdin is connected to a terminal
  if (!isatty(STDIN_FILENO)) return false;

  // retrieve current terminal parameters and save them as original
  if (tcgetattr(STDIN_FILENO, &original_terminal_parameters) == -1) ERROR_SYSTEM_ERRNO();

  // set terminal parameters for enabling noncannonical mode
  struct termios new_terminal_parameters = original_terminal_parameters;
  new_terminal_parameters.c_lflag &= ~(ICANON | ECHO); // disable cannonical mode and echoing of input characters
  new_terminal_parameters.c_cc[VMIN] = 1; // set minimum number of characters for noncannonical read
  new_terminal_parameters.c_cc[VTIME] = 0; // set timeout in deciseconds for noncannonical read

  // enable noncannonical mode
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_terminal_parameters) == -1) ERROR_SYSTEM_ERRNO();
  is_noncannonical_mode_enabled = true;

  register_terminal_parameter_restoration_handlers();

  return true;
}

#endif // _WIN32
