#include "cli/file.h"

#include "interpreter.h"
#include "utils/io.h"

#include <assert.h>

// *---------------------------------------------*
// *         EXTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/**@desc interpret file located at `source_file_path`
@return interpretation error code*/
ErrorCode file_interpret(char const *const source_file_path) {
  assert(source_file_path != NULL);

  ErrorCode error_code = ERROR_CODE_SUCCESS;
  interpreter_init();

  // read source code
  char *const source_code = io_read_file(source_file_path);

  // interpret source_code
  InterpreterStatus const interpreter_status = interpreter_interpret(source_code);

  // map interpreter_status to error_code
  assert(INTERPRETER_STATUS_COUNT == 4 && "Exhaustive InterpreterStatus handling");
  switch (interpreter_status) {
    case INTERPRETER_SUCCESS: {
      break;
    }
    case INTERPRETER_COMPILER_FAILURE:
    case INTERPRETER_COMPILER_UNEXPECTED_EOF: {
      error_code = ERROR_CODE_COMPILATION;
      break;
    }
    case INTERPRETER_VM_FAILURE: {
      error_code = ERROR_CODE_EXECUTION;
      break;
    }
    default: ERROR_INTERNAL("Unknown InterpreterStatus '%d'", interpreter_status);
  }

  // clean up
  free(source_code);
  interpreter_destroy();

  return error_code;
}
