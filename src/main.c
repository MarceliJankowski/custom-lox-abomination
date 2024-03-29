#include "chunk.h"
#include "vm.h"

int main(void) {
  vm_init();

  Chunk chunk;
  chunk_init(&chunk);

  chunk_append_constant_instruction(&chunk, 6.4, 1);
  chunk_append_constant_instruction(&chunk, 7.6, 1);
  chunk_append_instruction(&chunk, OP_ADD, 1);

  chunk_append_constant_instruction(&chunk, 3, 2);
  chunk_append_instruction(&chunk, OP_DIVIDE, 2);

  chunk_append_constant_instruction(&chunk, 3, 3);
  chunk_append_instruction(&chunk, OP_MODULO, 3);

  chunk_append_instruction(&chunk, OP_RETURN, 4);

  vm_interpret(&chunk);
  vm_free();
  chunk_free(&chunk);

  return 0;
}
