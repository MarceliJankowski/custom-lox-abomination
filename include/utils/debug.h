#ifndef DEBUG_H
#define DEBUG_H

#include "backend/chunk.h"
#include "frontend/lexer.h"

#include <stdint.h>

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

#ifdef DEBUG
// enable all supported debug features
#define DEBUG_LEXER
#define DEBUG_COMPILER
#define DEBUG_VM
#endif

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

void debug_token(LexerToken const *token);
void debug_disassemble_chunk(Chunk const *chunk, char const *name);
int32_t debug_disassemble_instruction(Chunk const *chunk, int32_t offset);

#endif // DEBUG_H
