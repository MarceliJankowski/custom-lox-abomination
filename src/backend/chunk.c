#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "backend/chunk.h"
#include "backend/gc.h"
#include "global.h"
#include "util/error.h"
#include "util/memory.h"

/**@desc initialize bytecode `chunk`*/
void chunk_init(Chunk *const chunk) {
  assert(chunk != NULL);

  GC_DARRAY_INIT(chunk, code);
  GC_DARRAY_INIT(&chunk->lines, line_counts);
  value_array_init(&chunk->constants);
}

/**@desc free `chunk` and restore it to its initial state*/
void chunk_free(Chunk *const chunk) {
  assert(chunk != NULL);

  GC_FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  GC_FREE_ARRAY(ChunkLineCount, chunk->lines.line_counts, chunk->lines.capacity);
  value_array_free(&chunk->constants);
  chunk_init(chunk);
}

/**@desc create new ChunkLineCount from `line` and append it to `chunk`*/
static inline void chunk_create_and_append_line_count(Chunk *const chunk, long const line) {
  assert(chunk != NULL);
  assert(line >= 1 && "Expected lines to begin at 1");

  ChunkLineCount line_count = {.line = line, .count = 1};
  GC_DARRAY_APPEND(&chunk->lines, line_counts, line_count);
}

/**@desc append instruction `opcode` and corresponding `line` to `chunk`*/
void chunk_append_instruction(Chunk *const chunk, uint8_t const opcode, long const line) {
  assert(chunk != NULL);

  GC_DARRAY_APPEND(chunk, code, opcode);

  // check if this is the first line/instruction in a chunk
  if (chunk->lines.count == 0) chunk_create_and_append_line_count(chunk, line);

  // check if line matches the latest ChunkLineCount line
  else if (line == chunk->lines.line_counts[chunk->lines.count - 1].line)
    chunk->lines.line_counts[chunk->lines.count - 1].count++;

  // this is the first chunk instruction located at line
  else chunk_create_and_append_line_count(chunk, line);
}

/**@desc append single byte instruction `operand` to `chunk`*/
void chunk_append_operand(Chunk *const chunk, uint8_t const operand) {
  assert(chunk != NULL);

  GC_DARRAY_APPEND(chunk, code, operand);
}

/**@desc append instruction operand consisting of `byte_count` uint8_t `bytes` to `chunk`*/
void chunk_append_multibyte_operand(Chunk *const chunk, int byte_count, ...) {
  assert(chunk != NULL);
  assert(byte_count >= 2 && "Expected multibyte operand");

  va_list bytes;
  va_start(bytes, byte_count);

  // check if chunk needs resizing
  if (chunk->count + byte_count > chunk->capacity) GC_DARRAY_RESIZE(chunk, code, sizeof(uint8_t));

  // append bytes
  for (; byte_count > 0; byte_count--) {
    uint8_t const byte = va_arg(bytes, unsigned int);
    chunk->code[chunk->count] = byte;
    chunk->count++;
  }

  va_end(bytes);
}

/**@desc append `value` to `chunk` constant pool
@return index of appended constant*/
static inline long chunk_append_constant(Chunk *const chunk, Value const value) {
  assert(chunk != NULL);

  value_array_append(&chunk->constants, value);
  return chunk->constants.count - 1;
}

/**@desc append `value` constant and corresponding instruction along with its `line` to `chunk`*/
void chunk_append_constant_instruction(Chunk *const chunk, Value const value, long const line) {
  assert(chunk != NULL);

  unsigned long const constant_index = chunk_append_constant(chunk, value);

  if (constant_index > 0xFFFFul)
    MEMORY_ERROR(FILE_LINE_FORMAT M_S "Exceeded chunk constant pool limit", g_source_file, line);

  if (constant_index > UCHAR_MAX) {
    chunk_append_instruction(chunk, OP_CONSTANT_2B, line);
    chunk_append_multibyte_operand(chunk, 2, get_byte(constant_index, 0), get_byte(constant_index, 1));
    return;
  }

  chunk_append_instruction(chunk, OP_CONSTANT, line);
  chunk_append_operand(chunk, constant_index);
}

/**@desc get line corresponding to `chunk` instruction located at byte `offset`
@return line corresponding to `offset` instruction*/
long chunk_get_instruction_line(Chunk *const chunk, long const offset) {
  assert(chunk != NULL);
  assert(chunk->lines.count > 0 && "Expected chunk to contain at least one line");
  assert(offset >= 0 && "Expected offset to be nonnegative");
  assert(offset < chunk->count && "Expected offset to fit within chunk code (out of bounds)");

  // find index of instruction located at offset
  long instruction_index = 0;
  long loop_offset = 0;

  static_assert(OP_OPCODE_COUNT == 8, "Exhaustive opcode handling");
  while (loop_offset < offset) {
    switch (chunk->code[loop_offset]) {
      case OP_RETURN:
      case OP_ADD:
      case OP_SUBTRACT:
      case OP_MULTIPLY:
      case OP_DIVIDE:
      case OP_MODULO: {
        loop_offset += 1;
        break;
      }
      case OP_CONSTANT: {
        loop_offset += 2;
        break;
      }
      case OP_CONSTANT_2B: {
        loop_offset += 3;
        break;
      }
      default: INTERNAL_ERROR("Unknown opcode '%d'", chunk->code[loop_offset]);
    }

    instruction_index++;
  }
  assert(loop_offset == offset && "Expected offset to an instruction; got offset to an instruction operand");

  // retrieve line corresponding to instruction_index
  long instruction_count = 0;
  for (long i = 0; i < chunk->lines.count; i++) {
    instruction_count += chunk->lines.line_counts[i].count;
    if (instruction_count > instruction_index) return chunk->lines.line_counts[i].line;
  }

  INTERNAL_ERROR("Failed to retrieve line corresponding to bytecode instruction");
}
