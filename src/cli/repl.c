#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "cli/repl.h"

#include "cli/gap_buffer.h"
#include "cli/history.h"
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

#define PHYSICAL_LINE_SEPARATOR '\n'

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

  DISABLE_STREAM_BUFFERING(stdin);
  DISABLE_STREAM_BUFFERING(stdout);

  // REPL loop continually forming and interpreting logical lines
  g_static_analysis_error_stream = tmpfile();
  if (g_static_analysis_error_stream == NULL) ERROR_IO_ERRNO();
  interpreter_init();
  history_init();
  GapBuffer physical_line;
  gap_buffer_init(&physical_line, INPUT_LINE_INITIAL_GROWTH_CAPACITY);
  DARRAY_DEFINE(char, logical_line, memory_manage);
  logical_line.initial_growth_capacity = INPUT_LINE_INITIAL_GROWTH_CAPACITY;
  for (bool is_continuing_logical_line = false;;) {
    // read physical line (one terminated with '\n') from stdin and append it to logical_line (forming it)
    for (bool is_physical_line_modified = false;;) {
      { // redraw physical line
        terminal_clear_current_line();

        char const *const prompt = is_continuing_logical_line ? LOGICAL_LINE_CONTINUATION_PROMPT : LOGICAL_LINE_PROMPT;
        int const prompt_length = io_printf(prompt);
        gap_buffer_print_content(&physical_line);

        size_t const new_cursor_index = gap_buffer_get_cursor_index(&physical_line) + prompt_length;
        terminal_move_cursor_to_column(new_cursor_index);
      }

      bool const can_browse_history = !is_physical_line_modified || gap_buffer_get_cursor_index(&physical_line) == 0;

      // read input key
      TerminalKey const key = terminal_read_key();
      TerminalKeyType const key_type = TERMINAL_KEY_GET_TYPE(key);

      // handle key
      static_assert(TERMINAL_KEY_TYPE_COUNT == 28, "Exhaustive TerminalKeyType handling");
      switch (key_type) {
        case TERMINAL_KEY_PRINTABLE: {
          if (key.printable.character == '\n') {
            io_printf("\n");
            goto handle_physical_line_end;
          }

          gap_buffer_insert_char(&physical_line, key.printable.character);
          is_physical_line_modified = true;
          break;
        }
        case TERMINAL_KEY_EOF:
        case TERMINAL_KEY_CTRL_D: {
          io_printf("\n");
          goto clean_up;
        }
        case TERMINAL_KEY_CTRL_L: {
          terminal_clear_screen();
          break;
        }
        case TERMINAL_KEY_CTRL_H:
        case TERMINAL_KEY_BACKSPACE: {
          if (gap_buffer_delete_char_left(&physical_line)) is_physical_line_modified = true;
          break;
        }
        case TERMINAL_KEY_DELETE: {
          if (gap_buffer_delete_char_right(&physical_line)) is_physical_line_modified = true;
          break;
        }
        case TERMINAL_KEY_CTRL_W: {
          if (gap_buffer_delete_word_left(&physical_line)) is_physical_line_modified = true;
          break;
        }
        case TERMINAL_KEY_ALT_D: {
          if (gap_buffer_delete_word_right(&physical_line)) is_physical_line_modified = true;
          break;
        }
        case TERMINAL_KEY_CTRL_U: {
          if (gap_buffer_delete_content_left(&physical_line)) is_physical_line_modified = true;
          break;
        }
        case TERMINAL_KEY_CTRL_K: {
          if (gap_buffer_delete_content_right(&physical_line)) is_physical_line_modified = true;
          break;
        }
        case TERMINAL_KEY_CTRL_B:
        case TERMINAL_KEY_ARROW_LEFT: {
          gap_buffer_move_cursor_left_by_char(&physical_line);
          break;
        }
        case TERMINAL_KEY_ALT_B:
        case TERMINAL_KEY_CTRL_ARROW_LEFT: {
          gap_buffer_move_cursor_left_by_word(&physical_line);
          break;
        }
        case TERMINAL_KEY_CTRL_F:
        case TERMINAL_KEY_ARROW_RIGHT: {
          gap_buffer_move_cursor_right_by_char(&physical_line);
          break;
        }
        case TERMINAL_KEY_ALT_F:
        case TERMINAL_KEY_CTRL_ARROW_RIGHT: {
          gap_buffer_move_cursor_right_by_word(&physical_line);
          break;
        }
        case TERMINAL_KEY_CTRL_A: {
          gap_buffer_move_cursor_to_start(&physical_line);
          break;
        }
        case TERMINAL_KEY_CTRL_E: {
          gap_buffer_move_cursor_to_end(&physical_line);
          break;
        }
        case TERMINAL_KEY_CTRL_P:
        case TERMINAL_KEY_ARROW_UP: {
          if (!can_browse_history) break;

          char const *const entry = history_browse_older_entry();
          if (entry != NULL) gap_buffer_load_content(&physical_line, entry);
          is_physical_line_modified = false;
          break;
        }
        case TERMINAL_KEY_CTRL_N:
        case TERMINAL_KEY_ARROW_DOWN: {
          if (!can_browse_history) break;

          if (history_is_newest_entry_browsed()) {
            history_stop_browsing();
            gap_buffer_clear_content(&physical_line);
          } else {
            char const *const entry = history_browse_newer_entry();
            if (entry != NULL) gap_buffer_load_content(&physical_line, entry);
          }

          is_physical_line_modified = false;
          break;
        }
        case TERMINAL_KEY_CTRL_ARROW_UP:
        case TERMINAL_KEY_CTRL_ARROW_DOWN:
        case TERMINAL_KEY_UNKNOWN: { // ignored
          break;
        }
        default: ERROR_INTERNAL("Unknown TerminalKeyType '%d'", TERMINAL_KEY_GET_TYPE(key));
      }

      // keep reading physical line
      continue;

    handle_physical_line_end:;
      char *const physical_line_content = gap_buffer_get_content(&physical_line);
      size_t const physical_line_content_length = gap_buffer_get_content_length(&physical_line);

      history_append_entry(physical_line_content, physical_line_content_length);

      size_t const new_logical_line_count =
        is_continuing_logical_line
          ? logical_line.count + physical_line_content_length + 1 // account for PHYSICAL_LINE_SEPARATOR
          : physical_line_content_length + 1; // account for NUL terminator

      // resize logical_line if needed
      if (logical_line.capacity < new_logical_line_count) DARRAY_RESERVE(&logical_line, new_logical_line_count);

      if (!is_continuing_logical_line) { // form new logical_line
        strcpy(logical_line.data, physical_line_content);
      } else { // continue forming logical_line
        logical_line.data[logical_line.count - 1] = PHYSICAL_LINE_SEPARATOR; // overwrite NUL
        memcpy(logical_line.data + logical_line.count, physical_line_content, physical_line_content_length);
        logical_line.data[new_logical_line_count - 1] = '\0';
      }
      logical_line.count = new_logical_line_count;

      history_stop_browsing();
      is_physical_line_modified = false;
      free(physical_line_content);
      gap_buffer_clear_content(&physical_line);
      break;
    }

    // interpret logical_line
    InterpreterStatus const interpreter_status = interpreter_interpret(logical_line.data);
    if (interpreter_status == INTERPRETER_COMPILER_FAILURE) {
      if (fflush(g_static_analysis_error_stream) == EOF) ERROR_IO_ERRNO();
      char *const static_analysis_errors = io_read_binary_stream_resource_content(g_static_analysis_error_stream);

      io_fprintf(stderr, "%s", static_analysis_errors);

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
  history_destroy();
  interpreter_destroy();
  if (fclose(g_static_analysis_error_stream)) ERROR_IO_ERRNO();

  g_static_analysis_error_stream = NULL;
  g_source_file_path = NULL;

  exit(ERROR_CODE_SUCCESS);
}
