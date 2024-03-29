#include <assert.h>
#include <stdlib.h>

#include "backend/value.h"
#include "common.h"
#include "util/debug.h"
#include "util/error.h"
#include "util/memory.h"

/**@desc disassemble and print `chunk` annotated with `name`*/
void debug_disassemble_chunk(Chunk *const chunk, char const *const name) {
  assert(chunk != NULL);
  assert(name != NULL);

  printf("== %s ==\n", name);
  for (long offset = 0; offset < chunk->count;) offset = debug_disassemble_instruction(chunk, offset);
}

/**@desc print simple instruction (one without operands) encoded by `opcode` and located at `offset`
@return offset to next instruction*/
static inline long debug_simple_instruction(uint8_t const opcode, long const offset) {
  switch (opcode) {
    case OP_RETURN:
      puts("OP_RETURN");
      break;
    case OP_ADD:
      puts("OP_ADD");
      break;
    case OP_SUBTRACT:
      puts("OP_SUBTRACT");
      break;
    case OP_MULTIPLY:
      puts("OP_MULTIPLY");
      break;
    case OP_DIVIDE:
      puts("OP_DIVIDE");
      break;
    case OP_MODULO:
      puts("OP_MODULO");
      break;
    default:
      INTERNAL_ERROR("Unknown simple instruction opcode '%d'", opcode);
  }

  return offset + 1;
}

/**@desc print `chunk` constant instruction encoded by `opcode` and located at `offset`
@return offset to next instruction*/
static long debug_constant_instruction(Chunk const *const chunk, uint8_t const opcode, long const offset) {
  assert(chunk != NULL);

  if (opcode == OP_CONSTANT) {
    uint8_t const constant = chunk->code[offset + 1];

    printf("OP_CONSTANT %d '", constant);
    value_print(chunk->constants.values[constant]);
    printf("'\n");

    return offset + 2;
  }

  if (opcode == OP_CONSTANT_2B) {
    unsigned int const constant = concatenate_bytes(2, chunk->code[offset + 2], chunk->code[offset + 1]);

    printf("OP_CONSTANT_2B %d '", constant);
    value_print(chunk->constants.values[constant]);
    printf("'\n");

    return offset + 3;
  }

  INTERNAL_ERROR("Unknown constant instruction opcode '%d'", opcode);
}

/**@desc disassemble and print `chunk` instruction located at `offset`
@return offset to next instruction*/
long debug_disassemble_instruction(Chunk *const chunk, long const offset) {
  assert(chunk != NULL);
  assert(offset >= 0 && "Expected offset to be nonnegative");

  printf(LINE_FORMAT " ", chunk_get_instruction_line(chunk, offset));

  uint8_t const opcode = chunk->code[offset];

  static_assert(OP_OPCODE_COUNT == 8, "Exhaustive opcode handling");
  switch (opcode) {
    case OP_RETURN:
    case OP_ADD:
    case OP_SUBTRACT:
    case OP_MULTIPLY:
    case OP_DIVIDE:
    case OP_MODULO:
      return debug_simple_instruction(opcode, offset);
    case OP_CONSTANT:
    case OP_CONSTANT_2B:
      return debug_constant_instruction(chunk, opcode, offset);
    default:
      INTERNAL_ERROR("Unknown opcode '%d'", opcode);
  }
}
