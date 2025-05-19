#include "cli/args.h"
#include "cli/file.h"
#include "cli/repl.h"
#include "global.h"
#include "utils/error.h"

int main(int const argc, char const *const *const argv) {
  g_static_analysis_error_stream = stderr;
  g_bytecode_execution_error_stream = stderr;
  g_source_program_output_stream = stdout;

  ErrorCode error_code = ERROR_CODE_SUCCESS;

  // process cli arguments
  g_source_file_path = args_process(argc, argv);

  // run the interpreter in specified interaction mode
  if (g_source_file_path == NULL) repl_enter();
  else error_code = file_interpret(g_source_file_path);

  return error_code;
}
