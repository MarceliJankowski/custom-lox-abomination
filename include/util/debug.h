#ifndef DEBUG_H
#define DEBUG_H

#include "backend/chunk.h"
#include "frontend/lexer.h"

#ifdef DEBUG
// enable all supported debug features
#define DEBUG_LEXER
#define DEBUG_COMPILER
#define DEBUG_VM
#endif

void debug_token(Token const *token);
void debug_disassemble_chunk(Chunk *chunk, char const *name);
long debug_disassemble_instruction(Chunk *chunk, long offset);

#endif // DEBUG_H
