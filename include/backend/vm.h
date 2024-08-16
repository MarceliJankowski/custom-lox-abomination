#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "value.h"

#include <stdbool.h>
#include <stdint.h>

#define VM_STACK_INITIAL_CAPACITY 256

typedef struct {
  Chunk *chunk;
  uint8_t *ip;
  struct {
    Value *values;
    int32_t capacity, count;
  } stack;
} VirtualMachine;

void vm_init(void);
void vm_free(void);
void vm_stack_push(Value value);
Value vm_stack_pop(void);
bool vm_run(Chunk *chunk);

#define vm_reset() \
  do {             \
    vm_free();     \
    vm_init();     \
  } while (0)

#endif // VM_H
