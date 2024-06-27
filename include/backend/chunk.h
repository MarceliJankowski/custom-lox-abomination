#ifndef CHUNK_H
#define CHUNK_H

#include <assert.h>
#include <limits.h>
#include <stdint.h>

#include "value.h"

/**@desc operation code representing bytecode instruction.
Instruction operands are stored in little-endian order.*/
typedef enum {
  // simple-instruction opcodes (without operands)
  OP_RETURN,
  OP_NEGATE,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_MODULO,
  OP_SIMPLE_OPCODE_END, // assertion utility

  // complex-instruction opcodes (with operands)
  OP_CONSTANT,
  OP_CONSTANT_2B,
  OP_COMPLEX_OPCODE_END, // assertion utility

  // assertion utilities
  OP_OPCODE_COUNT = OP_COMPLEX_OPCODE_END - 1, // -1 to account for assertion utility members
  OP_SIMPLE_OPCODE_COUNT = OP_SIMPLE_OPCODE_END,
  OP_COMPLEX_OPCODE_COUNT = OP_COMPLEX_OPCODE_END - OP_SIMPLE_OPCODE_END - 1,
} OpCode;

static_assert(OP_OPCODE_COUNT <= UCHAR_MAX, "Too many OpCodes defined; bytecode instruction can't fit all of them");

/**@desc `count` of bytecode chunk instructions at a given `line`*/
typedef struct {
  int32_t line;
  int count;
} ChunkLineCount;

/**@desc dynamic array representing bytecode chunk*/
typedef struct {
  ValueArray constants;
  struct {
    int32_t capacity, count;
    ChunkLineCount *line_counts;
  } lines;
  /**@desc array of Chunk instructions and their operands.
  Each instruction is encoded as 1 byte long OpCode.*/
  uint8_t *code;
  int32_t capacity, count;
} Chunk;

void chunk_init(Chunk *chunk);
void chunk_free(Chunk *chunk);
void chunk_append_instruction(Chunk *chunk, uint8_t opcode, int32_t line);
void chunk_append_operand(Chunk *chunk, uint8_t operand);
void chunk_append_multibyte_operand(Chunk *chunk, int byte_count, ...);
void chunk_append_constant_instruction(Chunk *chunk, Value value, int32_t line);
int32_t chunk_get_instruction_line(Chunk *const chunk, int32_t const offset);

#define chunk_reset(chunk_ptr) \
  do {                         \
    chunk_free(chunk_ptr);     \
    chunk_init(chunk_ptr);     \
  } while (0)

#endif // CHUNK_H
