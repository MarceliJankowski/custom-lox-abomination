#include "cli/repl.h"

#include "cli/terminal.h"
#include "global.h"
#include "interpreter.h"
#include "utils/darray.h"
#include "utils/error.h"
#include "utils/io.h"
#include "utils/memory.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#else // POSIX
#include <sys/select.h>
#include <unistd.h>
#endif

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

#define LOGICAL_LINE_PROMPT "> "
#define LOGICAL_LINE_CONTINUATION_PROMPT "... "

#define DISABLE_STREAM_BUFFERING(stream)                          \
  do {                                                            \
    errno = 0;                                                    \
    if (setvbuf(stream, NULL, _IONBF, 0) != 0) {                  \
      if (errno != 0) ERROR_IO_ERRNO();                           \
      ERROR_IO("Failed to disable " #stream " stream buffering"); \
    }                                                             \
  } while (0)

#define GET_KEY_TYPE(key) ((key).control.type)
#define MAKE_CONTROL_KEY(key_type) ((Key){.control = {.type = key_type}})
#define MAKE_PRINTABLE_KEY(key_character) \
  ((Key){.printable = {                   \
           .type = KEY_PRINTABLE,         \
           .character = key_character,    \
         }})

// *---------------------------------------------*
// *              TYPE DEFINITIONS               *
// *---------------------------------------------*

typedef enum {
  KEY_UNKNOWN,
  KEY_PRINTABLE,
  KEY_EOF,
  KEY_BACKSPACE,
  KEY_ARROW_UP,
  KEY_ARROW_DOWN,
  KEY_ARROW_LEFT,
  KEY_ARROW_RIGHT,
  KEY_TYPE_COUNT,
} KeyType;

typedef union {
  struct {
    KeyType type;
  } control;
  struct {
    KeyType type;
    char character;
  } printable;
} Key;

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

/**@desc set stdin stream to binary mode*/
static inline void set_stdin_stream_to_binary_mode(void);

/**@desc read key press from REPL's interactive session (only supports ASCII)
@return key that was read*/
static Key read_key(void);

// *---------------------------------------------*
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/**@desc interpret stdin stream resource content*/
static inline void interpret_stdin_stream_resource_content(void) {
  interpreter_init();

  set_stdin_stream_to_binary_mode();
  char *const input = io_read_binary_stream_resource_content(stdin);

  interpreter_interpret(input);

  interpreter_destroy();
  free(input);
}

// *---------------------------------------------*
// *         EXTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/**@desc enter REPL interaction mode; once entered it persists until process termination*/
void repl_enter(void) {
  g_source_file_path = "repl";

  if (!terminal_enable_noncannonical_mode()) {
    // handle stdin not being connected to a terminal
    interpret_stdin_stream_resource_content();

    g_source_file_path = NULL;
    exit(ERROR_CODE_SUCCESS);
  }

  g_static_analysis_error_stream = tmpfile();
  if (g_static_analysis_error_stream == NULL) ERROR_IO_ERRNO();
  interpreter_init();
  DARRAY_DEFINE(char, logical_line, memory_manage);

  // disable buffering of stdin/stdout streams
  DISABLE_STREAM_BUFFERING(stdin);
  DISABLE_STREAM_BUFFERING(stdout);

  // REPL loop continually forming and interpreting logical lines
  printf(LOGICAL_LINE_PROMPT);
  for (;;) {
    // read physical line (one terminated with '\n') from stdin and append it to logical_line (forming it)
    for (int printable_key_count = 0;;) {
      Key const key = read_key();

      // handle key
      static_assert(KEY_TYPE_COUNT == 8, "Exhaustive KeyType handling");
      switch (GET_KEY_TYPE(key)) {
        case KEY_EOF: {
          printf("\n");
          goto clean_up;
        }
        case KEY_BACKSPACE: {
          // erase last printable key
          if (printable_key_count > 0) {
            --printable_key_count;
            DARRAY_POP(&logical_line);
            printf("\b \b");
          }
          continue;
        }
        case KEY_PRINTABLE: {
          ++printable_key_count;

          printf("%c", key.printable.character);
          DARRAY_PUSH(&logical_line, key.printable.character);

          if (key.printable.character == '\n') { // end of physical line
            DARRAY_PUSH(&logical_line, '\0');
            break;
          }

          continue;
        }
        case KEY_ARROW_UP:
        case KEY_ARROW_DOWN:
        case KEY_ARROW_LEFT:
        case KEY_ARROW_RIGHT:
        case KEY_UNKNOWN: { // ignored
          continue;
        }
        default: ERROR_INTERNAL("Unknown KeyType '%d'", GET_KEY_TYPE(key));
      }

      break;
    }

    // interpret logical_line
    InterpreterStatus const interpreter_status = interpreter_interpret(logical_line.data);
    if (interpreter_status == INTERPRETER_COMPILER_FAILURE) {
      if (fflush(g_static_analysis_error_stream) == EOF) ERROR_IO_ERRNO();
      char *const static_analysis_errors = io_read_binary_stream_resource_content(g_static_analysis_error_stream);

      fprintf(stderr, "%s", static_analysis_errors);

      free(static_analysis_errors);
    }

    // clear static analysis errors
    g_static_analysis_error_stream = freopen(NULL, "w+b", g_static_analysis_error_stream);
    if (g_static_analysis_error_stream == NULL) ERROR_IO_ERRNO();

    // handle incomplete logical_line
    if (interpreter_status == INTERPRETER_COMPILER_UNEXPECTED_EOF) {
      // decrement count so that next input character overwrites current NUL terminator
      assert(logical_line.count > 0);
      logical_line.count--;

      // continue forming logical_line
      printf(LOGICAL_LINE_CONTINUATION_PROMPT);
      continue;
    }

    // begin new logical_line
    logical_line.count = 0;
    printf(LOGICAL_LINE_PROMPT);
  }

clean_up:
  DARRAY_DESTROY(&logical_line);
  interpreter_destroy();
  if (fclose(g_static_analysis_error_stream)) ERROR_IO_ERRNO();

  g_static_analysis_error_stream = NULL;
  g_source_file_path = NULL;

  exit(ERROR_CODE_SUCCESS);
}

#ifdef _WIN32
// *---------------------------------------------*
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

static inline void set_stdin_stream_to_binary_mode(void) {
  int const stdin_file_descriptor = _fileno(stdin);
  if (stdin_file_descriptor == -1) ERROR_SYSTEM_ERRNO();
  if (_setmode(stdin_file_descriptor, _O_BINARY) == -1) ERROR_SYSTEM_ERRNO();
}

#else
#define _POSIX_C_SOURCE 200809L

// *---------------------------------------------*
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

static inline void set_stdin_stream_to_binary_mode(void) {
  // on POSIX systems there's no difference between binary/text streams
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
@return read character, or EOF if stdin EOF indicator was set*/
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

static Key read_key(void) {
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
      return MAKE_CONTROL_KEY(KEY_BACKSPACE);
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
          case 'A': return MAKE_CONTROL_KEY(KEY_ARROW_UP);
          case 'B': return MAKE_CONTROL_KEY(KEY_ARROW_DOWN);
          case 'C': return MAKE_CONTROL_KEY(KEY_ARROW_RIGHT);
          case 'D': return MAKE_CONTROL_KEY(KEY_ARROW_LEFT);
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
  return MAKE_CONTROL_KEY(KEY_UNKNOWN);

#undef MAX_CONTROL_SEQUENCE_LENGTH
#undef CONTROL_SEQUENCE_REJECT_QUEUE_CAPACITY
#undef ENQUEUE_CONTROL_SEQUENCE_REJECT
#undef DEQUEUE_CONTROL_SEQUENCE_REJECT
}

#endif // _WIN32
