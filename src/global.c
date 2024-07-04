#include "global.h"

char const *g_source_file; // TEMP

/**@desc stream for static analysis errors*/
FILE *g_static_err_stream;

/**@desc stream for bytecode execution errors*/
FILE *g_execution_err_stream;
