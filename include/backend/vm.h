#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "utils/stack.h"
#include "value.h"

#include <stdbool.h>
#include <stdint.h>

#define VM_STACK_MIN_GROWTH_CAPACITY 256

typedef struct {
  Chunk const *chunk;
  uint8_t const *ip;
  STACK_TYPE(Value) stack;
} VM;

extern size_t const *const t_vm_stack_count;

void vm_init(void);
void vm_free(void);
void vm_stack_push(Value value);
Value vm_stack_pop(void);
bool vm_run(Chunk const *chunk);

inline void vm_reset(void) {
  vm_free();
  vm_init();
}

#endif // VM_H
