#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "utils/stack.h"
#include "value.h"

#include <stdbool.h>
#include <stdint.h>

// *---------------------------------------------*
// *              TYPE DEFINITIONS               *
// *---------------------------------------------*

typedef struct {
  Chunk const *chunk;
  uint8_t const *ip;
  STACK_TYPE(Value) stack;
} VM;

// *---------------------------------------------*
// *             OBJECT DECLARATIONS             *
// *---------------------------------------------*

extern size_t const *const t_vm_stack_count;

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

void vm_init(void);
void vm_destroy(void);
void vm_stack_push(Value value);
Value vm_stack_pop(void);
bool vm_execute(Chunk const *chunk);

// *---------------------------------------------*
// *              INLINE FUNCTIONS               *
// *---------------------------------------------*

inline void vm_reset(void) {
  vm_destroy();
  vm_init();
}

#endif // VM_H
