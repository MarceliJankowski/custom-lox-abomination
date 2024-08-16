#include "backend/vm.h"
#include "frontend/compiler.h"
#include "global.h"
#include "util/darray.h"
#include "util/error.h"
#include "util/io.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static struct {
  unsigned int help : 1;
} cli_options;

static int main_exit_code = EXIT_SUCCESS;

static void print_manual(void) {
  static_assert(ERROR_CODE_COUNT == 5, "Exhaustive error code handling");
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
    "       %d  Invalid command-line argument supplied.\n"
    "\n"
    "       %d  Memory error occurred.\n"
    "\n"
    "       %d  Input/Output error occurred.\n"
    "\n"
    "       %d  Error occurred during compilation.\n"
    "\n"
    "       %d  Error occurred during bytecode execution.\n",
    INVALID_ARG_ERROR_CODE, MEMORY_ERROR_CODE, IO_ERROR_CODE, COMPILATION_ERROR_CODE, RUNTIME_ERROR_CODE
  );
}

static void enter_repl(void) {
  g_source_file = "repl";
  g_static_err_stream = tmpfile();
  if (g_static_err_stream == NULL) IO_ERROR("%s", strerror(errno));

  Chunk chunk;
  struct {
    char *buffer;
    int32_t capacity, count;
  } input = {0};

  printf("> ");
  for (;;) {
    chunk_init(&chunk);

    // get line from stdin and append it to input.buffer
    for (;;) {
      int const character = getchar();
      if (character == EOF) {
        if (ferror(stdin)) IO_ERROR("Failed to read character from stdin");

        // stdin EOF indicator was set
        printf("\n");
        goto clean_up;
      }

      DARRAY_APPEND(&input, buffer, character);
      if (character == '\n') break;
    }
    DARRAY_APPEND(&input, buffer, '\0');

    // interpret input.buffer
    CompilationStatus const compilation_status = compiler_compile(input.buffer, &chunk);
    if (compilation_status == COMPILATION_SUCCESS) vm_run(&chunk);
    else {
      if (compilation_status == COMPILATION_FAILURE) {
        if (fflush(g_static_err_stream)) IO_ERROR("%s", strerror(errno));
        char *const static_errors = read_binary_stream(g_static_err_stream);

        fprintf(stderr, "%s", static_errors);
        free(static_errors);
      }

      // clear static errors
      g_static_err_stream = freopen(NULL, "w+b", g_static_err_stream);
      if (g_static_err_stream == NULL) IO_ERROR("%s", strerror(errno));

      if (compilation_status == COMPILATION_UNEXPECTED_EOF) {
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
  free(input.buffer);
  chunk_free(&chunk);
  if (fclose(g_static_err_stream)) IO_ERROR("%s", strerror(errno));
}

static void interpret_cla_file(char const *const filepath) {
  assert(filepath != NULL);

  g_source_file = filepath;
  char *const source_code = read_file(filepath);
  Chunk chunk;
  chunk_init(&chunk);

  // interpret source_code
  if (compiler_compile(source_code, &chunk) != COMPILATION_SUCCESS) {
    main_exit_code = COMPILATION_ERROR_CODE;
    goto clean_up;
  }
  if (!vm_run(&chunk)) main_exit_code = RUNTIME_ERROR_CODE;

clean_up:
  chunk_free(&chunk);
  free(source_code);
}

/**@desc process `flag_component` (CLI argument that begins with a '-' and may contain multiple flags)*/
static inline void process_flag_component(char const *flag_component) {
  assert(flag_component != NULL);
  assert(*flag_component == '-' && "Expected flag_component argument to begin with a '-'");

  flag_component++; // advance past beginning '-'

  if (*flag_component == '\0') INVALID_ARG_ERROR("Incomplete command-line flag supplied: '-'");
  if (*flag_component == '-') {
    char const *long_flag = ++flag_component;

    if (strcmp(long_flag, "help") == 0) cli_options.help = true;
    else INVALID_ARG_ERROR("Invalid command-line flag supplied: '--%s'", long_flag);
    return;
  }

  while (*flag_component) {
    switch (*flag_component++) {
      case 'h': {
        cli_options.help = true;
        break;
      }
      default: INVALID_ARG_ERROR("Invalid command-line flag supplied: '%c'", flag_component[-1]);
    }
  }
}

int main(int const argc, char const *const argv[]) {
  g_static_err_stream = stderr;
  g_execution_err_stream = stderr;
  char const *cla_filepath_arg = NULL;

  // process command-line arguments
  for (int i = 1; i < argc; i++) {
    if (*argv[i] == '-') process_flag_component(argv[i]);
    else {
      if (cla_filepath_arg != NULL) INVALID_ARG_ERROR("Excessive CLA filepath supplied: '%s'", argv[i]);
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
