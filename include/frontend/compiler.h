#ifndef COMPILER_H
#define COMPILER_H

#include <stdbool.h>

#include "backend/vm.h"

bool compiler_compile(char const *source_code, Chunk *chunk);

#endif // COMPILER_H
