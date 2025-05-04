#define _POSIX_C_SOURCE 200809L

#include "utils/terminal.h"

#include "utils/error.h"

#include <assert.h>
#include <stdbool.h>

// POSIX headers
#include <signal.h>
#include <termios.h>
#include <unistd.h>

static struct termios original_terminal_parameters;
static bool are_original_terminal_parameters_set;

/**@desc restore terminal parameters to their original state.
Requires original_terminal_parameters to be first set.*/
static void restore_terminal_parameters(void) {
  assert(are_original_terminal_parameters_set == true);

  // due to being used as atexit() handler this function cannot exit() (second exit would trigger UB)
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_terminal_parameters) == -1) fprintf(stderr, "%s", strerror(errno));
}

/**@desc wrapper around restore_terminal_parameters() for handling `signal_num` signal*/
static void restore_terminal_parameters_signal_handler(int const signal_num) {
  restore_terminal_parameters();

  // re-raise signal_num to trigger default signal_num handler
  if (raise(signal_num) != 0) ERROR_SYSTEM("Failed to re-raise '%d' signal", signal_num);
}

/**@desc register restore_terminal_parameters() as handler for appropriate signals and atexit()*/
static void register_restore_terminal_parameters(void) {
  struct sigaction terminal_signal_action = {
    .sa_handler = restore_terminal_parameters_signal_handler, // set signal handler
    .sa_flags = SA_RESETHAND, // restore default signal handler once signal is caught
  };

  // allow other signals to be delivered and processed while the handler is executing
  if (sigemptyset(&terminal_signal_action.sa_mask) == -1) ERROR_SYSTEM("%s", strerror(errno));

  // register handler
  if (0 != atexit(restore_terminal_parameters)) ERROR_SYSTEM("Failed to register atexit() handler");
  if (-1 == sigaction(SIGINT, &terminal_signal_action, NULL)) ERROR_SYSTEM("%s", strerror(errno));
  if (-1 == sigaction(SIGTERM, &terminal_signal_action, NULL)) ERROR_SYSTEM("%s", strerror(errno));
  if (-1 == sigaction(SIGHUP, &terminal_signal_action, NULL)) ERROR_SYSTEM("%s", strerror(errno));
  if (-1 == sigaction(SIGQUIT, &terminal_signal_action, NULL)) ERROR_SYSTEM("%s", strerror(errno));
}

/**@desc enable noncannonical terminal mode.
Once enabled, it persists until process termination.
This function registers atexit/signal handlers for terminal parameter restoration upon imminent process termination.
Only single invocation is permitted.*/
void terminal_enable_noncannonical_mode(void) {
  assert(are_original_terminal_parameters_set == false);

  // retrieve current terminal parameters and save them as original
  if (tcgetattr(STDIN_FILENO, &original_terminal_parameters) == -1) ERROR_SYSTEM("%s", strerror(errno));
  are_original_terminal_parameters_set = true;

  // set terminal parameters for enabling noncannonical mode
  struct termios new_terminal_parameters = original_terminal_parameters;
  new_terminal_parameters.c_lflag &= ~(ICANON | ECHO); // disable cannonical mode and echoing of input characters
  new_terminal_parameters.c_cc[VMIN] = 1; // set minimum number of characters for noncannonical read
  new_terminal_parameters.c_cc[VTIME] = 0; // set timeout in deciseconds for noncannonical read

  // enable noncannonical mode
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_terminal_parameters) == -1) ERROR_SYSTEM("%s", strerror(errno));

  register_restore_terminal_parameters();
}
