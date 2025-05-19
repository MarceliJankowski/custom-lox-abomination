#include "cli/repl.h"

#include "cli/terminal.h"
#include "global.h"
#include "interpreter.h"
#include "utils/darray.h"
#include "utils/error.h"
#include "utils/io.h"

#include <stdio.h>

// *---------------------------------------------*
// *                  UTILITIES                  *
// *---------------------------------------------*

#define LOGICAL_LINE_PROMPT "> "
#define LOGICAL_LINE_CONTINUATION_PROMPT "... "

// *---------------------------------------------*
// *              REPL FUNCTIONS                 *
// *---------------------------------------------*

/**@desc enter REPL interaction mode; once entered it persists until process termination*/
void repl_enter(void) {
  g_source_file_path = "repl";
  g_static_analysis_error_stream = tmpfile();
  if (g_static_analysis_error_stream == NULL) ERROR_IO_ERRNO();

  interpreter_init();
  DARRAY_DEFINE(char, logical_line, memory_manage);

  terminal_enable_noncannonical_mode();

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
    for (;;) {
      int const character = getchar();
      if (character == EOF) {
        if (ferror(stdin)) ERROR_IO("Failed to read character from stdin");

        // stdin EOF indicator was set
        printf("\n");
        goto clean_up;
      }

      // handle character
      printf("%c", character);
      DARRAY_PUSH(&logical_line, character);

      // end of physical line
      if (character == '\n') {
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

  g_source_file_path = NULL;
  g_static_analysis_error_stream = NULL;

  exit(ERROR_CODE_SUCCESS);
}
