#ifndef VM_H
#define VM_H

#include <stdint.h>

#include "chunk.h"
#include "value.h"

#define VM_STACK_MAX 256

typedef struct {
  Chunk *chunk;
  uint8_t *ip;
  Value *stack_top;
  Value stack[VM_STACK_MAX];
} VirtualMachine;

void vm_init(void);
void vm_free(void);
void vm_interpret(Chunk *chunk);
void vm_stack_push(Value value);
Value vm_stack_pop(void);

#endif // VM_H
