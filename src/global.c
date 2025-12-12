#include "global.h"

// *---------------------------------------------*
// *          EXTERNAL-LINKAGE OBJECTS           *
// *---------------------------------------------*

char const *g_source_file_path; // TEMP

/// Stream for static analysis errors.
FILE *g_static_analysis_error_stream;

/// Stream for bytecode execution errors.
FILE *g_bytecode_execution_error_stream;

/// Stream for source program's (one being interpreted) output.
FILE *g_source_program_output_stream;
