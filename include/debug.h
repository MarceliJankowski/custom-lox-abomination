#ifndef DEBUG_H
#define DEBUG_H

#include "chunk.h"

#ifdef DEBUG
// enable all supported debug features
#define DEBUG_TRACE_EXECUTION
#endif

void debug_disassemble_chunk(Chunk *chunk, char const *name);
long debug_disassemble_instruction(Chunk *chunk, long offset);

#endif // DEBUG_H
