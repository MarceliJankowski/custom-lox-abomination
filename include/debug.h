#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#include "chunk.h"
#include "common.h"

#ifdef DEBUG
#define DEBUG_PRINT(...)                                                                           \
  do {                                                                                             \
    printf("[DEBUG]" M_S);                                                                         \
    printf(__VA_ARGS__); /* separate printf call so that message can also be passed as a pointer*/ \
    printf("\n");                                                                                  \
  } while (0)
#else
#define DEBUG_PRINT(...)
#endif

void debug_disassemble_chunk(Chunk *chunk, char const *name);
long debug_disassemble_instruction(Chunk *chunk, long offset);

#endif // DEBUG_H
