#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backend/value.h"
#include "backend/vm.h"
#include "util/debug.h"
#include "util/error.h"
#include "util/memory.h"
#include "util/stack.h"

static VirtualMachine vm;

/**@desc initialize virtual machine*/
void vm_init(void) {
  STACK_INIT(Value, &vm.stack, values, VM_STACK_INITIAL_CAPACITY);
}

/**@desc free virtual machine*/
void vm_free(void) {
  STACK_FREE(&vm.stack, values);
}

/**@desc push `value` on top of virtual machine stack*/
void vm_stack_push(Value const value) {
  STACK_PUSH(&vm.stack, values, value);
}

/**@desc pop value from virtual machine stack
@return popped value*/
Value vm_stack_pop(void) {
  Value popped_value;
  STACK_POP(&vm.stack, values, &popped_value);

  return popped_value;
}

/**@desc run virtual machine; execute all vm.chunk instructions*/
static void vm_run(void) {
#define READ_BYTE() (*vm.ip++)
#define BINARY_OP(operator)                                                                         \
  do {                                                                                              \
    Value const right_operand = vm_stack_pop();                                                     \
    assert(vm.stack.count > 0 && "Attempt to access nonexistent stack frame");                      \
    STACK_TOP_FRAME(&vm.stack, values) = STACK_TOP_FRAME(&vm.stack, values) operator right_operand; \
  } while (0)

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("[ ");
    for (long i = 0; i < vm.stack.count;) {
      value_print(vm.stack.values[i]);
      if (++i < vm.stack.count) printf(", ");
    }
    puts(" ]");
    debug_disassemble_instruction(vm.chunk, vm.ip - vm.chunk->code);
#endif
    uint8_t const opcode = READ_BYTE();

    static_assert(OP_OPCODE_COUNT == 8, "Exhaustive opcode handling");
    switch (opcode) {
      case OP_RETURN: {
        value_print(vm_stack_pop());
        printf("\n");
        return;
      }
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
      case OP_ADD: {
        BINARY_OP(+);
        break;
      }
      case OP_SUBTRACT: {
        BINARY_OP(-);
        break;
      }
      case OP_MULTIPLY: {
        BINARY_OP(*);
        break;
      }
      case OP_DIVIDE: {
        BINARY_OP(/);
        break;
      }
      case OP_MODULO: {
        Value const divisor = vm_stack_pop();
        assert(vm.stack.count > 0 && "Attempt to access nonexistent stack frame");
        STACK_TOP_FRAME(&vm.stack, values) = fmod(STACK_TOP_FRAME(&vm.stack, values), divisor);
        break;
      }
      default:
        INTERNAL_ERROR("Unknown opcode '%d'", opcode);
    }
  }

#undef BINARY_OP
#undef READ_BYTE
}

/**@desc interpret bytecode `chunk`*/
void vm_interpret(Chunk *const chunk) {
  assert(chunk != NULL);

  vm.chunk = chunk;
  vm.ip = chunk->code;
  vm_run();
}
