#include "cli/manual.h"

#include "utils/error.h"

#include <assert.h>
#include <stdio.h>

// *---------------------------------------------*
// *         EXTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/**@desc print manual*/
void manual_print(void) {
  static_assert(ERROR_CODE_COUNT == 7, "Exhaustive error code handling");
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
    "       %d  cla successfully run, no errors occurred.\n"
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
    ERROR_CODE_SUCCESS, ERROR_CODE_COMPILATION, ERROR_CODE_EXECUTION, ERROR_CODE_INVALID_ARG, ERROR_CODE_MEMORY,
    ERROR_CODE_IO, ERROR_CODE_SYSTEM
  );
}
