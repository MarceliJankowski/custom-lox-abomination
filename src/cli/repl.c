#include "cli/repl.h"

#include "cli/terminal.h"
#include "global.h"
#include "interpreter.h"
#include "utils/darray.h"
#include "utils/error.h"
#include "utils/io.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

#define LOGICAL_LINE_PROMPT "> "
#define LOGICAL_LINE_CONTINUATION_PROMPT "... "

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

  g_static_analysis_error_stream = tmpfile();
  if (g_static_analysis_error_stream == NULL) ERROR_IO_ERRNO();
  interpreter_init();
  DARRAY_DEFINE(char, logical_line, memory_manage);

  // set stdout stream buffering mode to unbuffered
  errno = 0;
  if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
    if (errno != 0) ERROR_IO_ERRNO();
    ERROR_IO("Failed to set stdout stream buffering mode to unbuffered");
  }

  // REPL loop continually forming and interpreting logical lines
  printf(LOGICAL_LINE_PROMPT);
  for (;;) {
    // get physical line (one terminated with '\n') from stdin and append it to logical_line (forming it)
    for (int printable_character_count = 0;;) {
      int const character = getchar();
      if (character == EOF) {
        if (ferror(stdin)) ERROR_IO("Failed to read character from stdin");

        // stdin EOF indicator was set
        printf("\n");
        goto clean_up;
      }

      // handle control characters
      switch (character) {
        case 0x7F: { // DEL
          // erase last printable character
          if (printable_character_count > 0) {
            --printable_character_count;
            DARRAY_POP(&logical_line);
            printf("\b \b");
          }
          continue;
        }
      }

      // printable character
      ++printable_character_count;

      printf("%c", character);
      DARRAY_PUSH(&logical_line, character);

      if (character == '\n') { // end of physical line
        DARRAY_PUSH(&logical_line, '\0');
        break;
      }
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
