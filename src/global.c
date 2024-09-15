#include "global.h"

char const *g_source_file; // TEMP

/**@desc stream for static analysis errors*/
FILE *g_static_error_stream;

/**@desc stream for bytecode execution errors*/
FILE *g_execution_error_stream;

/**@desc stream for running CLA program output*/
FILE *g_runtime_output_stream;
