#include "backend/vm.h"
#include "frontend/compiler.h"
#include "global.h"
#include "utils/darray.h"
#include "utils/error.h"
#include "utils/io.h"
#include "utils/memory.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static struct {
  unsigned int help : 1;
} cli_options;

static int main_exit_code = EXIT_SUCCESS;

static void print_manual(void) {
  static_assert(ERROR_CODE_COUNT == 6, "Exhaustive error code handling");
  printf(
    "NAME\n"
    "       cla - Custom Lox Abomination interpreter written in C\n"
    "\nSYNOPSIS\n"
    "       cla [-h|--help] [path]\n"
    "\nUSAGE\n"
    "       CLA code can be supplied via source file path, or directly through built-in REPL.\n"
    "       REPL is the default interaction mode, entered unless path argument is supplied.\n"
    "\nOPTIONS\n"
    "       -h, --help\n"
    "           Get help; print out this manual and exit.\n"
    "\nEXIT CODES\n"
    "       Exit code indicates whether cla successfully run, or failed for some reason.\n"
    "       Different exit codes indicate different failure causes:\n"
    "\n"
    "       0  cla successfully run, no errors occurred.\n"
    "\n"
    "       %d  Error occurred during compilation.\n"
    "\n"
    "       %d  Error occurred during bytecode execution.\n"
    "\n"
    "       %d  Invalid command-line argument supplied.\n"
    "\n"
    "       %d  Memory error occurred.\n"
    "\n"
    "       %d  Input/Output error occurred.\n"
    "\n"
    "       %d  Error occurred during system-level or runtime environment operation.\n",
    ERROR_CODE_COMPILATION, ERROR_CODE_EXECUTION, ERROR_CODE_INVALID_ARG, ERROR_CODE_MEMORY, ERROR_CODE_IO,
    ERROR_CODE_SYSTEM
  );
}

static void enter_repl(void) {
  g_source_file = "repl";
  g_static_error_stream = tmpfile();
  if (g_static_error_stream == NULL) ERROR_IO("%s", strerror(errno));

  Chunk chunk;
  DARRAY_DEFINE(char, input, memory_manage);

  printf("> ");
  for (;;) {
    chunk_init(&chunk);

    // get line from stdin and append it to input.buffer
    for (;;) {
      int const character = getchar();
      if (character == EOF) {
        if (ferror(stdin)) ERROR_IO("Failed to read character from stdin");

        // stdin EOF indicator was set
        printf("\n");
        goto clean_up;
      }

      DARRAY_APPEND(&input, character);
      if (character == '\n') break;
    }
    DARRAY_APPEND(&input, '\0');

    // interpret input.buffer
    CompilerStatus const compilation_status = compiler_compile(input.data, &chunk);
    if (compilation_status == COMPILER_SUCCESS) vm_run(&chunk);
    else {
      if (compilation_status == COMPILER_FAILURE) {
        if (fflush(g_static_error_stream)) ERROR_IO("%s", strerror(errno));
        char *const static_errors = io_read_binary_stream_resource_content(g_static_error_stream);

        fprintf(stderr, "%s", static_errors);
        free(static_errors);
      }

      // clear static errors
      g_static_error_stream = freopen(NULL, "w+b", g_static_error_stream);
      if (g_static_error_stream == NULL) ERROR_IO("%s", strerror(errno));

      if (compilation_status == COMPILER_UNEXPECTED_EOF) {
        chunk_free(&chunk);

        // decrement count so that next input character overwrites current NUL terminator
        assert(input.count > 0);
        input.count--;

        // continue logical line
        printf("... ");
        continue;
      }
    }

    // new logical line
    input.count = 0;
    chunk_free(&chunk);
    printf("> ");
  }

clean_up:
  DARRAY_FREE(&input);
  chunk_free(&chunk);
  if (fclose(g_static_error_stream)) ERROR_IO("%s", strerror(errno));
}

static void interpret_cla_file(char const *const filepath) {
  assert(filepath != NULL);

  g_source_file = filepath;
  char *const source_code = io_read_file(filepath);
  Chunk chunk;
  chunk_init(&chunk);

  // interpret source_code
  if (compiler_compile(source_code, &chunk) != COMPILER_SUCCESS) {
    main_exit_code = ERROR_CODE_COMPILATION;
    goto clean_up;
  }
  if (!vm_run(&chunk)) main_exit_code = ERROR_CODE_EXECUTION;

clean_up:
  chunk_free(&chunk);
  free(source_code);
}

/**@desc process `flag_component` (CLI argument that begins with a '-' and may contain multiple flags)*/
static inline void process_flag_component(char const *flag_component) {
  assert(flag_component != NULL);
  assert(*flag_component == '-' && "Expected flag_component argument to begin with a '-'");

  flag_component++; // advance past beginning '-'

  if (*flag_component == '\0') ERROR_INVALID_ARG("Incomplete command-line flag supplied: '-'");
  if (*flag_component == '-') {
    char const *long_flag = ++flag_component;

    if (strcmp(long_flag, "help") == 0) cli_options.help = true;
    else ERROR_INVALID_ARG("Invalid command-line flag supplied: '--%s'", long_flag);
    return;
  }

  while (*flag_component) {
    switch (*flag_component++) {
      case 'h': {
        cli_options.help = true;
        break;
      }
      default: ERROR_INVALID_ARG("Invalid command-line flag supplied: '%c'", flag_component[-1]);
    }
  }
}

int main(int const argc, char const *const argv[]) {
  g_static_error_stream = stderr;
  g_execution_error_stream = stderr;
  g_runtime_output_stream = stdout;
  char const *cla_filepath_arg = NULL;

  // process command-line arguments
  for (int i = 1; i < argc; i++) {
    if (*argv[i] == '-') process_flag_component(argv[i]);
    else {
      if (cla_filepath_arg != NULL) ERROR_INVALID_ARG("Excessive CLA filepath supplied: '%s'", argv[i]);
      cla_filepath_arg = argv[i];
    }
  }

  // handle cli_options
  if (cli_options.help) {
    print_manual();
    return main_exit_code;
  }

  // run the damn thing!
  vm_init();
  if (cla_filepath_arg == NULL) enter_repl();
  else interpret_cla_file(cla_filepath_arg);
  vm_free();

  return main_exit_code;
}
