#include "interpreter.h"

#include "backend/vm.h"
#include "frontend/compiler.h"
#include "global.h"

// *---------------------------------------------*
// *         EXTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/// Initialize interpreter.
void interpreter_init(void) {
  vm_init();
}

/// Release interpreter resources and set it to uninitialized state.
void interpreter_destroy(void) {
  vm_destroy();
}

/// Interpret `source_code`; interpreter state persists across `source_code` interpretations.
/// @return Interpreter status indicating interpretation result.
InterpreterStatus interpreter_interpret(char const *const source_code) {
  assert(source_code != NULL);

  InterpreterStatus interpreter_status = INTERPRETER_SUCCESS;
  Chunk chunk;
  chunk_init(&chunk);

  CompilerStatus const compiler_status = compiler_compile(source_code, &chunk);

  // map compiler_status to interpreter_status
  static_assert(COMPILER_STATUS_COUNT == 3, "Exhaustive CompilerStatus handling");
  switch (compiler_status) {
    case COMPILER_SUCCESS: {
      break;
    }
    case COMPILER_FAILURE: {
      interpreter_status = INTERPRETER_COMPILER_FAILURE;
      goto clean_up;
    }
    case COMPILER_UNEXPECTED_EOF: {
      interpreter_status = INTERPRETER_COMPILER_UNEXPECTED_EOF;
      goto clean_up;
    }
    default: ERROR_INTERNAL("Unknown CompilerStatus '%d'", compiler_status);
  }

  if (!vm_execute(&chunk)) interpreter_status = INTERPRETER_VM_FAILURE;

clean_up:
  chunk_destroy(&chunk);

  return interpreter_status;
}
