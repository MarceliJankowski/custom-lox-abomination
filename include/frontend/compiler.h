#ifndef COMPILER_H
#define COMPILER_H

#include "backend/vm.h"

#include <stdbool.h>

typedef enum {
  COMPILER_SUCCESS,
  COMPILER_FAILURE,
  COMPILER_UNEXPECTED_EOF,
  COMPILER_STATUS_COUNT,
} CompilerStatus;

CompilerStatus compiler_compile(char const *source_code, Chunk *chunk);

#endif // COMPILER_H
