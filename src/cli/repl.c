#include "cli/repl.h"

#include "cli/terminal.h"
#include "global.h"
#include "interpreter.h"
#include "utils/darray.h"
#include "utils/io.h"

#include <stdio.h>

/**@desc enter cla REPL interaction mode*/
void repl_enter(void) {
  g_source_file_path = "repl";
  g_static_error_stream = tmpfile();
  if (g_static_error_stream == NULL) ERROR_IO_ERRNO();

  interpreter_init();
  DARRAY_DEFINE(char, input, memory_manage);

  terminal_enable_noncannonical_mode();

  printf("> ");
  for (;;) {
    // get line from stdin and append it to input
    for (;;) {
      int const character = getchar();
      if (character == EOF) {
        if (ferror(stdin)) ERROR_IO("Failed to read character from stdin");

        // stdin EOF indicator was set
        printf("\n");
        goto clean_up;
      }

      DARRAY_PUSH(&input, character);
      if (character == '\n') break;
    }
    DARRAY_PUSH(&input, '\0');

    // interpret input
    InterpreterStatus const interpreter_status = interpreter_interpret(input.data);
    if (interpreter_status == INTERPRETER_COMPILER_FAILURE) {
      if (fflush(g_static_error_stream)) ERROR_IO_ERRNO();
      char *const static_errors = io_read_binary_stream_resource_content(g_static_error_stream);

      fprintf(stderr, "%s", static_errors);
      free(static_errors);
    }

    // clear static errors
    g_static_error_stream = freopen(NULL, "w+b", g_static_error_stream);
    if (g_static_error_stream == NULL) ERROR_IO_ERRNO();

    if (interpreter_status == INTERPRETER_COMPILER_UNEXPECTED_EOF) {
      // decrement count so that next input character overwrites current NUL terminator
      assert(input.count > 0);
      input.count--;

      // continue logical line
      printf("... ");
      continue;
    }

    // new logical line
    input.count = 0;
    printf("> ");
  }

clean_up:
  DARRAY_DESTROY(&input);
  interpreter_destroy();
  if (fclose(g_static_error_stream)) ERROR_IO_ERRNO();

  g_source_file_path = NULL;
  g_static_error_stream = NULL;
}
