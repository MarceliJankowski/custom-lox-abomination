#include <stdint.h>
#include <stdio.h>

#include "debug.h"
#include "error.h"
#include "memory.h"
#include "value.h"
#include "vm.h"

static VirtualMachine vm;

/**@desc reset virtual machine stack*/
static inline void vm_stack_reset(void) {
  vm.stack_top = vm.stack;
}

/**@desc push `value` on top of virtual machine stack*/
void vm_stack_push(Value const value) {
  // TODO: address stack overflow

  *vm.stack_top = value;
  vm.stack_top++;
}

/**@desc pop value from virtual machine stack
@return popped value*/
Value vm_stack_pop(void) {
  assert(vm.stack_top != vm.stack && "Attempt to pop value from an empty stack");

  vm.stack_top--;
  return *vm.stack_top;
}

/**@desc initialize virtual machine*/
void vm_init(void) {
  vm_stack_reset();
}

/**@desc free virtual machine*/
void vm_free(void) {}

/**@desc run virtual machine; execute all vm.chunk instructions*/
static void vm_run(void) {
#define READ_BYTE() (*vm.ip++)

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("[ ");
    for (Value *slot = vm.stack; slot < vm.stack_top;) {
      value_print(*slot);
      if (++slot < vm.stack_top) printf(", ");
    }
    printf(" ]\n");
    debug_disassemble_instruction(vm.chunk, vm.ip - vm.chunk->code);
#endif
    uint8_t const opcode = READ_BYTE();

    static_assert(OP_OPCODE_COUNT == 3, "Exhaustive opcode handling");
    switch (opcode) {
      case OP_CONSTANT: {
        Value constant = vm.chunk->constants.values[READ_BYTE()];
        vm_stack_push(constant);
        break;
      }
      case OP_CONSTANT_2B: {
        uint8_t const constant_index_LSB = READ_BYTE();
        uint8_t const constant_index_MSB = READ_BYTE();
        long const constant_index = concatenate_bytes(2, constant_index_MSB, constant_index_LSB);
        Value const constant = vm.chunk->constants.values[constant_index];

        vm_stack_push(constant);
        break;
      }
      case OP_RETURN:
        value_print(vm_stack_pop());
        printf("\n");
        return;
      default:
        INTERNAL_ERROR("Unknown opcode '%d'", opcode);
    }
  }

#undef READ_BYTE
}

/**@desc interpret bytecode `chunk`*/
void vm_interpret(Chunk *chunk) {
  assert(chunk != NULL);

  vm.chunk = chunk;
  vm.ip = chunk->code;
  vm_run();
}
