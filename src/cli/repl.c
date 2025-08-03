#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "cli/repl.h"

#include "cli/gap_buffer.h"
#include "cli/terminal.h"
#include "global.h"
#include "interpreter.h"
#include "utils/darray.h"
#include "utils/error.h"
#include "utils/io.h"
#include "utils/memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

#define LOGICAL_LINE_PROMPT "> "
#define LOGICAL_LINE_CONTINUATION_PROMPT "... "

#define INPUT_LINE_INITIAL_GROWTH_CAPACITY 128

#define DISABLE_STREAM_BUFFERING(stream)                          \
  do {                                                            \
    errno = 0;                                                    \
    if (setvbuf(stream, NULL, _IONBF, 0) != 0) {                  \
      if (errno != 0) ERROR_IO_ERRNO();                           \
      ERROR_IO("Failed to disable " #stream " stream buffering"); \
    }                                                             \
  } while (0)

// *---------------------------------------------*
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/**@desc set stdin stream to binary mode*/
static inline void set_stdin_stream_to_binary_mode(void) {
#ifdef _WIN32
  int const stdin_file_descriptor = _fileno(stdin);
  if (stdin_file_descriptor == -1) ERROR_SYSTEM_ERRNO();
  if (_setmode(stdin_file_descriptor, _O_BINARY) == -1) ERROR_SYSTEM_ERRNO();
#else
  // on POSIX systems there's no difference between binary/text streams
#endif
}

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

  // disable buffering of stdin/stdout streams
  DISABLE_STREAM_BUFFERING(stdin);
  DISABLE_STREAM_BUFFERING(stdout);

  // REPL loop continually forming and interpreting logical lines
  g_static_analysis_error_stream = tmpfile();
  if (g_static_analysis_error_stream == NULL) ERROR_IO_ERRNO();
  interpreter_init();
  GapBuffer physical_line;
  gap_buffer_init(&physical_line, INPUT_LINE_INITIAL_GROWTH_CAPACITY);
  DARRAY_DEFINE(char, logical_line, memory_manage);
  logical_line.initial_growth_capacity = INPUT_LINE_INITIAL_GROWTH_CAPACITY;
  for (bool is_continuing_logical_line = false;;) {
    // read physical line (one terminated with '\n') from stdin and append it to logical_line (forming it)
    for (;;) {
      // redraw physical line
      terminal_clear_current_line();

      char const *const prompt = is_continuing_logical_line ? LOGICAL_LINE_CONTINUATION_PROMPT : LOGICAL_LINE_PROMPT;
      int const prompt_length = printf(prompt);
      gap_buffer_print_content(&physical_line);

      int const new_cursor_position = gap_buffer_get_cursor_position(&physical_line) + prompt_length;
      terminal_move_cursor_to_column(new_cursor_position);

      // read input key
      TerminalKey const key = terminal_read_key();

      // handle key
      static_assert(TERMINAL_KEY_TYPE_COUNT == 8, "Exhaustive TerminalKeyType handling");
      switch (TERMINAL_KEY_GET_TYPE(key)) {
        case TERMINAL_KEY_EOF: {
          printf("\n");
          goto clean_up;
        }
        case TERMINAL_KEY_BACKSPACE: {
          // erase printable key to the left of cursor
          if (gap_buffer_get_cursor_position(&physical_line) > 0) {
            gap_buffer_delete_left(&physical_line);
          }
          break;
        }
        case TERMINAL_KEY_PRINTABLE: {
          gap_buffer_insert(&physical_line, key.printable.character);

          // handle newline
          if (key.printable.character == '\n') {
            printf("\n");
            goto physical_line_end;
          }
          break;
        }
        case TERMINAL_KEY_ARROW_LEFT: {
          if (gap_buffer_get_cursor_position(&physical_line) > 0) {
            gap_buffer_move_cursor_left(&physical_line);
          }
          break;
        }
        case TERMINAL_KEY_ARROW_RIGHT: {
          if (gap_buffer_get_cursor_position(&physical_line) < gap_buffer_get_content_length(&physical_line)) {
            gap_buffer_move_cursor_right(&physical_line);
          }
          break;
        }
        case TERMINAL_KEY_ARROW_UP:
        case TERMINAL_KEY_ARROW_DOWN:
        case TERMINAL_KEY_UNKNOWN: { // ignored
          break;
        }
        default: ERROR_INTERNAL("Unknown TerminalKeyType '%d'", TERMINAL_KEY_GET_TYPE(key));
      }

      // keep reading physical line
      continue;

    physical_line_end:;
      if (is_continuing_logical_line == false) logical_line.count = 0; // form new logical_line
      else { // continue forming logical_line
        assert(logical_line.count > 0);
        logical_line.count--; // overwrite NUL terminator
      }

      char *const physical_line_content = gap_buffer_get_content(&physical_line);
      size_t const physical_line_content_length = gap_buffer_get_content_length(&physical_line);
      size_t const new_logical_line_count =
        logical_line.count + physical_line_content_length + 1; // account for NUL terminator

      // resize logical_line if needed
      if (logical_line.capacity < new_logical_line_count) DARRAY_RESERVE(&logical_line, new_logical_line_count);

      // append physical_line to logical_line
      strcpy(logical_line.data + logical_line.count, physical_line_content);
      logical_line.count = new_logical_line_count;

      free(physical_line_content);
      gap_buffer_clear_content(&physical_line);
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

    // determine if logical_line is incomplete and therefore should be continued
    if (interpreter_status == INTERPRETER_COMPILER_UNEXPECTED_EOF) is_continuing_logical_line = true;
    else is_continuing_logical_line = false;
  }

clean_up:
  DARRAY_DESTROY(&logical_line);
  gap_buffer_destroy(&physical_line);
  interpreter_destroy();
  if (fclose(g_static_analysis_error_stream)) ERROR_IO_ERRNO();

  g_static_analysis_error_stream = NULL;
  g_source_file_path = NULL;

  exit(ERROR_CODE_SUCCESS);
}
