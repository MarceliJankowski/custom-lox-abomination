#ifndef VM_H
#define VM_H

#include "backend/chunk.h"
#include "backend/value.h"
#include "utils/stack.h"

#include <stdbool.h>
#include <stdint.h>

// *---------------------------------------------*
// *              TYPE DEFINITIONS               *
// *---------------------------------------------*

/// Virtual Machine.
typedef struct {
  Object *gc_objects;
  Chunk const *chunk;
  uint8_t const *ip;
  STACK_TYPE(Value) stack;
} VM;

// *---------------------------------------------*
// *             OBJECT DECLARATIONS             *
// *---------------------------------------------*

/// Global Virtual Machine.
extern VM vm;

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

/// Reset virtual machine back to initialized state.
inline void vm_reset(void) {
  vm_destroy();
  vm_init();
}

#endif // VM_H
