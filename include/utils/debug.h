#ifndef DEBUG_H
#define DEBUG_H

#include "backend/chunk.h"
#include "common.h"
#include "frontend/lexer.h"
#include "utils/io.h"

#include <stdint.h>

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

#define DEBUG__PREFIX "[DEBUG]"

#ifdef DEBUG
// enable all supported debug features
#define DEBUG_LEXER
#define DEBUG_COMPILER
#define DEBUG_VM

#define DEBUG_LOG(...)                  \
  do {                                  \
    io_printf(DEBUG__PREFIX COMMON_MS); \
    io_printf(__VA_ARGS__);             \
    io_printf("\n");                    \
  } while (0)
#else
#define DEBUG_LOG(...)
#endif

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

void debug_token(LexerToken const *token);
void debug_disassemble_chunk(Chunk const *chunk, char const *name);
int32_t debug_disassemble_instruction(Chunk const *chunk, int32_t offset);

#endif // DEBUG_H
