#ifndef CHUNK_H
#define CHUNK_H

#include "value.h"

#include <assert.h>
#include <limits.h>
#include <stdint.h>

/**@desc chunk operation code representing bytecode instruction.
Instruction operands are stored in little-endian order.*/
typedef enum {
  // simple-instruction opcodes (without operands)
  CHUNK_OP_RETURN,
  CHUNK_OP_PRINT,
  CHUNK_OP_POP,
  CHUNK_OP_NEGATE,
  CHUNK_OP_ADD,
  CHUNK_OP_SUBTRACT,
  CHUNK_OP_MULTIPLY,
  CHUNK_OP_DIVIDE,
  CHUNK_OP_MODULO,
  CHUNK_OP_NOT,
  CHUNK_OP_NIL,
  CHUNK_OP_TRUE,
  CHUNK_OP_FALSE,
  CHUNK_OP_EQUAL,
  CHUNK_OP_NOT_EQUAL,
  CHUNK_OP_LESS,
  CHUNK_OP_LESS_EQUAL,
  CHUNK_OP_GREATER,
  CHUNK_OP_GREATER_EQUAL,
  CHUNK_OP_SIMPLE_OPCODE_END, // assertion utility

  // complex-instruction opcodes (with operands)
  CHUNK_OP_CONSTANT,
  CHUNK_OP_CONSTANT_2B,
  CHUNK_OP_COMPLEX_OPCODE_END, // assertion utility

  // assertion utilities
  CHUNK_OP_OPCODE_COUNT = CHUNK_OP_COMPLEX_OPCODE_END - 1, // -1 to account for assertion utility members
  CHUNK_OP_SIMPLE_OPCODE_COUNT = CHUNK_OP_SIMPLE_OPCODE_END,
  CHUNK_OP_COMPLEX_OPCODE_COUNT = CHUNK_OP_COMPLEX_OPCODE_END - CHUNK_OP_SIMPLE_OPCODE_END - 1,
} ChunkOpCode;

static_assert(
  CHUNK_OP_OPCODE_COUNT <= UCHAR_MAX, "Too many ChunkOpCodes defined; bytecode instruction can't fit all of them"
);

/**@desc `count` of bytecode chunk instructions at a given `line`*/
typedef struct {
  int32_t line;
  int count;
} ChunkLineCount;

/**@desc bytecode chunk*/
typedef struct {
  ValueArray constants;
  struct {
    int32_t capacity, count;
    ChunkLineCount *line_counts;
  } lines;
  /**@desc dynamic array of Chunk instructions and their operands.
  Each instruction is encoded as 1 byte long ChunkOpCode.*/
  uint8_t *code;
  int32_t capacity, count;
} Chunk;

void chunk_init(Chunk *chunk);
void chunk_free(Chunk *chunk);
void chunk_append_instruction(Chunk *chunk, uint8_t opcode, int32_t line);
void chunk_append_operand(Chunk *chunk, uint8_t operand);
void chunk_append_multibyte_operand(Chunk *chunk, int byte_count, ...);
void chunk_append_constant_instruction(Chunk *chunk, Value value, int32_t line);
int32_t chunk_get_instruction_line(Chunk const *chunk, int32_t offset);

inline void chunk_reset(Chunk *const chunk_ptr) {
  chunk_free(chunk_ptr);
  chunk_init(chunk_ptr);
}

#endif // CHUNK_H
