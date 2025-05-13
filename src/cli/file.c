#include "cli/file.h"

#include "backend/chunk.h"
#include "backend/vm.h"
#include "frontend/compiler.h"
#include "utils/io.h"

#include <assert.h>

/**@desc interpret file located at `source_file_path`
@return interpretation error code*/
ErrorCode file_interpret(char const *const source_file_path) {
  ErrorCode error_code = ERROR_CODE_SUCCESS;
  char *const source_code = io_read_file(source_file_path);

  vm_init();
  Chunk chunk;
  chunk_init(&chunk);

  // interpret source_code
  if (compiler_compile(source_code, &chunk) != COMPILER_SUCCESS) {
    error_code = ERROR_CODE_COMPILATION;
    goto clean_up;
  }
  if (!vm_run(&chunk)) error_code = ERROR_CODE_EXECUTION;

clean_up:
  chunk_free(&chunk);
  vm_free();
  free(source_code);

  return error_code;
}
