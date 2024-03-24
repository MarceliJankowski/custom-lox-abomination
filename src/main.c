#include "chunk.h"
#include "debug.h"

int main(void) {
  Chunk chunk;
  chunk_init(&chunk);

  chunk_append_constant_instruction(&chunk, 1.5, 1);
  chunk_append_constant_instruction(&chunk, 2.5, 2);
  chunk_append_constant_instruction(&chunk, 3.5, 3);
  chunk_append_instruction(&chunk, OP_RETURN, 3);

  debug_disassemble_chunk(&chunk, "test chunk");

  chunk_free(&chunk);

  return 0;
}
