#include <stdio.h>

// I want to add module support in the future; this is a temporary solution to satisfy error reporting
extern char const *g_source_file_path;

extern FILE *g_static_error_stream;
extern FILE *g_execution_error_stream;
extern FILE *g_runtime_output_stream;
