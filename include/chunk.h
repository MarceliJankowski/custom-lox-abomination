#ifndef CHUNK_H
#define CHUNK_H

#include <assert.h>
#include <limits.h>
#include <stdint.h>

#include "value.h"

/**@desc operation code representing bytecode instruction.
Instruction operands are stored in little-endian order.*/
typedef enum {
  OP_RETURN,
  OP_CONSTANT,
  OP_CONSTANT_2B,
  OP_OPCODE_COUNT,
} OpCode;

static_assert(
  OP_OPCODE_COUNT <= UCHAR_MAX, "Too many OpCodes defined; bytecode instruction can't fit all of them"
);

/**@desc `count` of bytecode chunk instructions at a given `line`*/
typedef struct {
  long line;
  int count;
} ChunkLineCount;

/**@desc dynamic array representing bytecode chunk*/
typedef struct {
  ValueArray constants;
  struct {
    long capacity, count;
    ChunkLineCount *line_counts;
  } lines;
  /**@desc array of Chunk instructions and their operands.
  Each instruction is encoded as 1 byte long OpCode.*/
  uint8_t *code;
  long capacity, count;
} Chunk;

void chunk_init(Chunk *chunk);
void chunk_free(Chunk *chunk);
void chunk_append_instruction(Chunk *chunk, uint8_t opcode, long line);
void chunk_append_operand(Chunk *chunk, uint8_t operand);
void chunk_append_multibyte_operand(Chunk *chunk, int byte_count, ...);
void chunk_append_constant_instruction(Chunk *chunk, Value value, long line);
long chunk_get_instruction_line(Chunk *const chunk, long const offset);

#endif // CHUNK_H
