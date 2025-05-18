#include "backend/chunk.h"

#include "backend/gc.h"
#include "global.h"
#include "utils/error.h"
#include "utils/memory.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void chunk_reset(Chunk *chunk);

/**@desc initialize bytecode `chunk`*/
void chunk_init(Chunk *const chunk) {
  assert(chunk != NULL);

  DARRAY_INIT(&chunk->code, sizeof(uint8_t), gc_memory_manage);
  DARRAY_INIT(&chunk->lines, sizeof(ChunkLineCount), gc_memory_manage);
  value_list_init(&chunk->constants);
}

/**@desc free `chunk` memory and set it to uninitialized state*/
void chunk_destroy(Chunk *const chunk) {
  assert(chunk != NULL);

  // free memory
  DARRAY_DESTROY(&chunk->code);
  DARRAY_DESTROY(&chunk->lines);
  value_list_destroy(&chunk->constants);

  // set to uninitialized state
  *chunk = (Chunk){0};
}

/**@desc create new ChunkLineCount from `line` and append it to `chunk`*/
static inline void chunk_create_and_append_line_count(Chunk *const chunk, int32_t const line) {
  assert(chunk != NULL);
  assert(line >= 1 && "Expected lines to begin at 1");

  ChunkLineCount const line_count = {.line = line, .count = 1};
  DARRAY_PUSH(&chunk->lines, line_count);
}

/**@desc append instruction `opcode` and corresponding `line` to `chunk`*/
void chunk_append_instruction(Chunk *const chunk, uint8_t const opcode, int32_t const line) {
  assert(chunk != NULL);

  DARRAY_PUSH(&chunk->code, opcode);

  // check if this is the first line/instruction in a chunk
  if (chunk->lines.count == 0) chunk_create_and_append_line_count(chunk, line);

  // check if line matches the latest ChunkLineCount line
  else if (line == chunk->lines.data[chunk->lines.count - 1].line) chunk->lines.data[chunk->lines.count - 1].count++;

  // this is the first chunk instruction located at line
  else chunk_create_and_append_line_count(chunk, line);
}

/**@desc append single byte instruction `operand` to `chunk`*/
void chunk_append_operand(Chunk *const chunk, uint8_t const operand) {
  assert(chunk != NULL);

  DARRAY_PUSH(&chunk->code, operand);
}

/**@desc append instruction operand consisting of `byte_count` uint8_t `bytes` to `chunk.code`*/
void chunk_append_multibyte_operand(Chunk *const chunk, int byte_count, ...) {
  assert(chunk != NULL);
  assert(byte_count >= 2 && "Expected multibyte operand");

  va_list bytes;
  va_start(bytes, byte_count);

  // check if chunk.code needs resizing
  if (chunk->code.count + byte_count > chunk->code.capacity) DARRAY_RESIZE(&chunk->code);

  // append bytes
  for (; byte_count > 0; byte_count--) {
    uint8_t const byte = va_arg(bytes, unsigned int);
    chunk->code.data[chunk->code.count] = byte;
    chunk->code.count++;
  }

  va_end(bytes);
}

/**@desc append `value` to `chunk` constant pool
@return index of appended constant*/
static inline int32_t chunk_append_constant(Chunk *const chunk, Value const value) {
  assert(chunk != NULL);

  value_list_append(&chunk->constants, value);
  return chunk->constants.count - 1;
}

/**@desc append `value` constant and corresponding instruction along with its `line` to `chunk`*/
void chunk_append_constant_instruction(Chunk *const chunk, Value const value, int32_t const line) {
  assert(chunk != NULL);

  uint32_t const constant_index = chunk_append_constant(chunk, value);

  if (constant_index > 0xFFFFul)
    ERROR_MEMORY(COMMON_FILE_LINE_FORMAT COMMON_MS "Exceeded chunk constant pool limit", g_source_file_path, line);

  if (constant_index > UCHAR_MAX) {
    chunk_append_instruction(chunk, CHUNK_OP_CONSTANT_2B, line);
    chunk_append_multibyte_operand(chunk, 2, memory_get_byte(constant_index, 0), memory_get_byte(constant_index, 1));
    return;
  }

  chunk_append_instruction(chunk, CHUNK_OP_CONSTANT, line);
  chunk_append_operand(chunk, constant_index);
}

/**@desc get line corresponding to `chunk` instruction located at byte `offset`
@return line corresponding to `offset` instruction*/
int32_t chunk_get_instruction_line(Chunk const *const chunk, int32_t const offset) {
  assert(chunk != NULL);
  assert(chunk->lines.count > 0 && "Expected chunk to contain at least one line");
  assert(offset >= 0 && "Expected offset to be nonnegative");
  assert((size_t)offset < chunk->code.count && "Expected offset to fit within chunk code (out of bounds)");

  // find index of instruction located at offset
  int32_t instruction_index = 0;
  int32_t loop_offset = 0;

  static_assert(CHUNK_OP_OPCODE_COUNT == 21, "Exhaustive ChunkOpCode handling");
  while (loop_offset < offset) {
    switch (chunk->code.data[loop_offset]) {
      case CHUNK_OP_RETURN:
      case CHUNK_OP_PRINT:
      case CHUNK_OP_POP:
      case CHUNK_OP_NEGATE:
      case CHUNK_OP_ADD:
      case CHUNK_OP_SUBTRACT:
      case CHUNK_OP_MULTIPLY:
      case CHUNK_OP_DIVIDE:
      case CHUNK_OP_MODULO:
      case CHUNK_OP_NOT:
      case CHUNK_OP_NIL:
      case CHUNK_OP_TRUE:
      case CHUNK_OP_FALSE:
      case CHUNK_OP_EQUAL:
      case CHUNK_OP_NOT_EQUAL:
      case CHUNK_OP_LESS:
      case CHUNK_OP_LESS_EQUAL:
      case CHUNK_OP_GREATER:
      case CHUNK_OP_GREATER_EQUAL: {
        loop_offset += 1;
        break;
      }
      case CHUNK_OP_CONSTANT: {
        loop_offset += 2;
        break;
      }
      case CHUNK_OP_CONSTANT_2B: {
        loop_offset += 3;
        break;
      }
      default: ERROR_INTERNAL("Unknown chunk opcode '%d'", chunk->code.data[loop_offset]);
    }

    instruction_index++;
  }
  assert(loop_offset == offset && "Expected offset to an instruction; got offset to an instruction operand");

  // retrieve line corresponding to instruction_index
  int32_t instruction_count = 0;
  for (size_t i = 0; i < chunk->lines.count; i++) {
    instruction_count += chunk->lines.data[i].count;
    if (instruction_count > instruction_index) return chunk->lines.data[i].line;
  }

  ERROR_INTERNAL("Failed to retrieve line corresponding to bytecode instruction");
}
