#ifndef COMPILER_H
#define COMPILER_H

#include "backend/vm.h"

#include <stdbool.h>

typedef enum { COMPILATION_SUCCESS, COMPILATION_FAILURE, COMPILATION_UNEXPECTED_EOF } CompilationStatus;

CompilationStatus compiler_compile(char const *source_code, Chunk *chunk);

#endif // COMPILER_H
