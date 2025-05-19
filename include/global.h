#include <stdio.h>

// *---------------------------------------------*
// *             OBJECT DECLARATIONS             *
// *---------------------------------------------*

// I want to add module support in the future; this is a temporary solution to satisfy error reporting
extern char const *g_source_file_path;

extern FILE *g_static_analysis_error_stream;
extern FILE *g_bytecode_execution_error_stream;
extern FILE *g_source_program_output_stream;
