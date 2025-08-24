#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "cli/terminal.h"

#include "utils/error.h"
#include "utils/io.h"

#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#else // POSIX
#include <signal.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

#define MAKE_CONTROL_KEY(key_type) ((TerminalKey){.control = {.type = key_type}})
#define MAKE_PRINTABLE_KEY(key_character)          \
  ((TerminalKey){.printable = {                    \
                   .type = TERMINAL_KEY_PRINTABLE, \
                   .character = key_character,     \
                 }})

#ifdef _WIN32
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

void terminal_clear_current_line(void) {
  // TODO: implement
}

void terminal_move_cursor_to_column(int const index) {
  assert(index >= 0 && "Attempt to move terminal cursor to a negative column");

  // TODO: implement
}

#else
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
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_terminal_parameters) == -1) {
    io_fprintf(stderr, "%s\n", strerror(errno));
  }
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

/**@desc determine whether stdin resource is readable (reads won't block) within `timeout_ms` millisecond timeout
@return true if it is, false otherwise*/
static bool is_stdin_resource_readable_within_timeout(int const timeout_ms) {
  assert(timeout_ms > 0);

  for (;;) {
    // select() might modify these objects, therefore they need to be reinitialized for every invocation
    struct timeval timeout = {
      .tv_sec = timeout_ms / 1000, // seconds
      .tv_usec = (timeout_ms % 1000) * 1000, // microseconds
    };
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    // check if fds are ready within timeout
    int const ready_fds_count = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);

    // handle failure
    if (ready_fds_count == -1) {
      if (errno == EINTR) continue; // select was interrupted
      ERROR_IO_ERRNO();
    }

    // handle expired timeout
    if (ready_fds_count == 0) return false;

    // check if stdin resource is readable
    return FD_ISSET(STDIN_FILENO, &readfds);
  }
}

/**@desc read character from stdin stream resource content
@return character that was read, or EOF if stdin EOF indicator was set*/
static inline int read_character(void) {
  int const character = getchar();
  if (character == EOF) {
    if (ferror(stdin)) ERROR_IO("Failed to read character from stdin");
    return EOF; // stdin EOF indicator was set
  }

  return character;
}

/**@desc read control sequence continuation character from stdin resource content
@return control sequence continuation character, or EOF if none were available*/
static inline int read_control_sequence_continuation_character(void) {
  if (!is_stdin_resource_readable_within_timeout(50)) return EOF;
  return read_character();
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

TerminalKey terminal_read_key(void) {
#define MAX_CONTROL_SEQUENCE_LENGTH 3
#define CONTROL_SEQUENCE_REJECT_QUEUE_CAPACITY MAX_CONTROL_SEQUENCE_LENGTH - 1

#define ENQUEUE_CONTROL_SEQUENCE_REJECT(character)                                                 \
  do {                                                                                             \
    assert(control_sequence_reject_queue.frame_count < CONTROL_SEQUENCE_REJECT_QUEUE_CAPACITY);    \
                                                                                                   \
    control_sequence_reject_queue.frames[control_sequence_reject_queue.frame_count] = (character); \
                                                                                                   \
    control_sequence_reject_queue.frame_count++;                                                   \
  } while (0)

#define DEQUEUE_CONTROL_SEQUENCE_REJECT(character_ptr)                                                          \
  do {                                                                                                          \
    assert(control_sequence_reject_queue.frame_count > 0);                                                      \
    assert(control_sequence_reject_queue.current_frame_index >= 0);                                             \
                                                                                                                \
    *(character_ptr) = control_sequence_reject_queue.frames[control_sequence_reject_queue.current_frame_index]; \
                                                                                                                \
    control_sequence_reject_queue.frame_count--;                                                                \
    if (control_sequence_reject_queue.frame_count == 0) control_sequence_reject_queue.current_frame_index = 0;  \
    else control_sequence_reject_queue.current_frame_index++;                                                   \
  } while (0)

  /**@desc queue for characters rejected from control sequences*/
  static struct {
    char frames[CONTROL_SEQUENCE_REJECT_QUEUE_CAPACITY - 1];
    int frame_count, current_frame_index;
  } control_sequence_reject_queue = {0};

  bool const is_handling_control_sequence_reject = control_sequence_reject_queue.frame_count;
  int character;
  if (is_handling_control_sequence_reject) DEQUEUE_CONTROL_SEQUENCE_REJECT(&character);
  else character = read_character();

  // handle control sequence characters
  switch (character) {
    case 0x7F: { // DEL
      return MAKE_CONTROL_KEY(TERMINAL_KEY_BACKSPACE);
    }
    case 0x1B: { // ESC
      if (is_handling_control_sequence_reject) goto esc_key;
      else { // attempt to construct control sequence
        int const intermediate_character = read_control_sequence_continuation_character();
        if (intermediate_character == EOF) goto esc_key;

        if (intermediate_character != '[') {
          ENQUEUE_CONTROL_SEQUENCE_REJECT(intermediate_character);
          goto esc_key;
        }

        int const final_character = read_control_sequence_continuation_character();
        if (final_character == EOF) {
          ENQUEUE_CONTROL_SEQUENCE_REJECT(intermediate_character);
          goto esc_key;
        }

        switch (final_character) {
          case 'A': return MAKE_CONTROL_KEY(TERMINAL_KEY_ARROW_UP);
          case 'B': return MAKE_CONTROL_KEY(TERMINAL_KEY_ARROW_DOWN);
          case 'C': return MAKE_CONTROL_KEY(TERMINAL_KEY_ARROW_RIGHT);
          case 'D': return MAKE_CONTROL_KEY(TERMINAL_KEY_ARROW_LEFT);
          default: {
            ENQUEUE_CONTROL_SEQUENCE_REJECT(intermediate_character);
            ENQUEUE_CONTROL_SEQUENCE_REJECT(final_character);
            goto esc_key;
          }
        }
      }

    esc_key:
      goto unknown_key;
    }
  }

  // handle printable characters
  if (character == '\n' || (character >= 32 && character < 127)) return MAKE_PRINTABLE_KEY(character);

unknown_key:
  // handle character(s) that do not constitute a known key type
  return MAKE_CONTROL_KEY(TERMINAL_KEY_UNKNOWN);

#undef MAX_CONTROL_SEQUENCE_LENGTH
#undef CONTROL_SEQUENCE_REJECT_QUEUE_CAPACITY
#undef ENQUEUE_CONTROL_SEQUENCE_REJECT
#undef DEQUEUE_CONTROL_SEQUENCE_REJECT
}

void terminal_clear_current_line(void) {
  io_printf("\r\x1b[2K");
}

void terminal_move_cursor_to_column(int const index) {
  assert(index >= 0 && "Attempt to move terminal cursor to a negative column");

  io_printf("\r\x1b[%dC", index);
}

#endif // _WIN32
