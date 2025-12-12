#include "cli/args.h"

#include "cli/manual.h"
#include "utils/error.h"

#include <assert.h>
#include <stdbool.h>

// *---------------------------------------------*
// *          INTERNAL-LINKAGE OBJECTS           *
// *---------------------------------------------*

/// Cli options bitfield.
static struct {
  unsigned int help : 1;
} options;

// *---------------------------------------------*
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/// Process `flag_arg` (cli argument that begins with a '-').
static inline void process_flag_arg(char const *flag_arg) {
  assert(flag_arg != NULL);
  assert(*flag_arg == '-');

  // advance past beginning '-'
  flag_arg++;

  // handle incomplete flag
  if (*flag_arg == '\0') ERROR_INVALID_ARG("Incomplete command-line flag supplied: '-'");

  // handle long flag (one beginning with '--')
  if (*flag_arg == '-') {
    char const *long_flag = ++flag_arg;

    if (strcmp(long_flag, "help") == 0) options.help = true;
    else ERROR_INVALID_ARG("Invalid command-line flag supplied: '--%s'", long_flag);
    return;
  }

  // handle short flag (one beginning with '-')
  while (*flag_arg) {
    switch (*flag_arg++) {
      case 'h': {
        options.help = true;
        break;
      }
      default: ERROR_INVALID_ARG("Invalid command-line flag supplied: '%c'", flag_arg[-1]);
    }
  }
}

// *---------------------------------------------*
// *         EXTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/// Process cli arguments; requires forwarding process's `argc` and `argv`.
/// @return cli path argument if supplied, NULL otherwise.
char const *args_process(int const argc, char const *const *const argv) {
  assert(argc >= 0);
  assert(argv != NULL);

  char const *source_file_path = NULL;

  // process cli arguments
  for (int i = 1; i < argc; i++) {
    if (*argv[i] == '-') process_flag_arg(argv[i]);
    else {
      if (source_file_path != NULL) ERROR_INVALID_ARG("Excessive command-line path supplied: '%s'", argv[i]);
      source_file_path = argv[i];
    }
  }

  // handle options
  if (options.help) {
    manual_print();
    exit(ERROR_CODE_SUCCESS);
  }

  return source_file_path;
}
