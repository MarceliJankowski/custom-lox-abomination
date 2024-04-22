#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "backend/vm.h"
#include "frontend/compiler.h"
#include "global.h"
#include "util/error.h"
#include "util/io.h"

static struct {
  unsigned int help : 1;
} execution_options;

static int main_exit_code = EXIT_SUCCESS;

static void print_manual(void) {
  static_assert(ERROR_CODE_COUNT == 5, "Exhaustive error code handling");
  printf(
    "NAME\n"
    "      clox - Lox interpreter written in C\n"
    "\nSYNOPSIS\n"
    "      clox [-h|--help] [path]\n"
    "\nUSAGE\n"
    "      Lox code can be supplied via source file path, or directly through built-in REPL.\n"
    "      REPL is the default execution mode, entered unless path argument is supplied.\n"
    "\nOPTIONS\n"
    "      -h, --help\n"
    "          Get help; print out this manual and exit.\n"
    "\nEXIT CODES\n"
    "      Exit code indicates whether clox successfully run, or failed for some reason.\n"
    "      Different exit codes indicate different failure causes:\n"
    "\n"
    "      0  clox successfully run, no errors occurred.\n"
    "\n"
    "      %d  Invalid command-line argument supplied.\n"
    "\n"
    "      %d  Memory error occurred.\n"
    "\n"
    "      %d  Input/Output error occurred.\n"
    "\n"
    "      %d  Error occurred during compilation.\n"
    "\n"
    "      %d  Error occurred during execution.\n",
    INVALID_ARG_ERROR_CODE, MEMORY_ERROR_CODE, IO_ERROR_CODE, COMPILATION_ERROR_CODE, RUNTIME_ERROR_CODE
  );
}

static void enter_repl(void) {
  char input_line[1024];
  g_source_file = "repl";
  Chunk chunk;

  // TODO: support multiline input
  for (;;) {
    printf("> ");

    if (fgets(input_line, sizeof(input_line), stdin) == NULL) {
      if (ferror(stdin)) IO_ERROR("Failed to read a line from stdin");

      // EOF reached
      printf("\n");
      break;
    }

    // execute input_line
    chunk_init(&chunk);
    if (!compiler_compile(input_line, &chunk) || !vm_interpret(&chunk)) {
      chunk_free(&chunk);
      continue;
    }
  }

  // clean up
  chunk_free(&chunk);
}

static void run_file(char const *const filepath) {
  assert(filepath != NULL);

  char *const source_code = read_file(filepath);
  g_source_file = filepath;

  // execute source_code
  Chunk chunk;
  chunk_init(&chunk);
  if (!compiler_compile(source_code, &chunk)) {
    main_exit_code = COMPILATION_ERROR_CODE;
    goto clean_up;
  }
  if (!vm_interpret(&chunk)) main_exit_code = RUNTIME_ERROR_CODE;

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

    if (strcmp(long_flag, "help") == 0) execution_options.help = true;
    else INVALID_ARG_ERROR("Invalid command-line flag supplied: '--%s'", long_flag);
    return;
  }

  while (*flag_component) {
    switch (*flag_component++) {
      case 'h': {
        execution_options.help = true;
        break;
      }
      default: INVALID_ARG_ERROR("Invalid command-line flag supplied: '%c'", flag_component[-1]);
    }
  }
}

int main(int const argc, char const *argv[]) {
  vm_init();

  char const *lox_filepath_arg = NULL;

  // process command-line arguments
  for (int i = 1; i < argc; i++) {
    if (*argv[i] == '-') process_flag_component(argv[i]);
    else {
      if (lox_filepath_arg != NULL) INVALID_ARG_ERROR("Excessive Lox file path supplied: '%s'", argv[i]);
      lox_filepath_arg = argv[i];
    }
  }

  // handle execution options
  if (execution_options.help) {
    print_manual();
    goto clean_up;
  }

  // begin execution
  if (lox_filepath_arg == NULL) enter_repl();
  else run_file(lox_filepath_arg);

clean_up:
  vm_free();
  return main_exit_code;
}
