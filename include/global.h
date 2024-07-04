#include <stdio.h>

// I want to add module support in the future; this is a temporary solution to satisfy error reporting
extern char const *g_source_file;

extern FILE *g_static_err_stream;
extern FILE *g_execution_err_stream;
