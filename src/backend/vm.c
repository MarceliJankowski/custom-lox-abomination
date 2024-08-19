#include "backend/vm.h"

#include "backend/value.h"
#include "global.h"
#include "util/debug.h"
#include "util/error.h"
#include "util/memory.h"
#include "util/stack.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

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

/**@desc report runtime error at `instruction_offset` with `message`*/
static void vm_report_error_at(ptrdiff_t const instruction_offset, char const *const message) {
  int32_t const instruction_line = chunk_get_instruction_line(vm.chunk, instruction_offset);
  fprintf(
    g_execution_err_stream, "[RUNTIME_ERROR]" M_S FILE_LINE_FORMAT M_S "%s\n", g_source_file, instruction_line, message
  );
}

/**@desc execute vm.chunk instructions
@return true if execution succeeded, false otherwise*/
bool vm_execute(void) {
#define READ_BYTE() (*vm.ip++)
#define BINARY_OP(operator)                                                                         \
  do {                                                                                              \
    Value const right_operand = vm_stack_pop();                                                     \
    assert(vm.stack.count > 0 && "Attempt to access nonexistent stack frame");                      \
    STACK_TOP_FRAME(&vm.stack, values) = STACK_TOP_FRAME(&vm.stack, values) operator right_operand; \
  } while (0)

#ifdef DEBUG_VM
  puts("\n== DEBUG_VM ==");
#endif

  for (;;) {
#ifdef DEBUG_VM
    printf("[ ");
    for (int32_t i = 0; i < vm.stack.count;) {
      value_print(vm.stack.values[i]);
      if (++i < vm.stack.count) printf(", ");
    }
    puts(" ]");
    debug_disassemble_instruction(vm.chunk, vm.ip - vm.chunk->code);
#endif
    assert(vm.ip < vm.chunk->code + vm.chunk->count && "Instruction pointer out of bounds");
    uint8_t const opcode = READ_BYTE();

    static_assert(OP_OPCODE_COUNT == 9, "Exhaustive opcode handling");
    switch (opcode) {
      case OP_RETURN: {
        value_print(STACK_TOP_FRAME(&vm.stack, values));
        printf("\n");
        return true;
      }
      case OP_CONSTANT: {
        Value constant = vm.chunk->constants.values[READ_BYTE()];
        vm_stack_push(constant);
        break;
      }
      case OP_CONSTANT_2B: {
        uint8_t const constant_index_LSB = READ_BYTE();
        uint8_t const constant_index_MSB = READ_BYTE();
        uint32_t const constant_index = concatenate_bytes(2, constant_index_MSB, constant_index_LSB);
        Value const constant = vm.chunk->constants.values[constant_index];

        vm_stack_push(constant);
        break;
      }
      case OP_NEGATE: {
        STACK_TOP_FRAME(&vm.stack, values) = -STACK_TOP_FRAME(&vm.stack, values);
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
        if (STACK_TOP_FRAME(&vm.stack, values) == 0) {
          vm_report_error_at(vm.ip - vm.chunk->code - 1, "Illegal division by zero");
          return false;
        }
        BINARY_OP(/);
        break;
      }
      case OP_MODULO: {
        Value const divisor = vm_stack_pop();
        if (divisor == 0) {
          vm_report_error_at(vm.ip - vm.chunk->code - 1, "Illegal modulo by zero");
          return false;
        }
        assert(vm.stack.count > 0 && "Attempt to access nonexistent stack frame");
        STACK_TOP_FRAME(&vm.stack, values) = fmod(STACK_TOP_FRAME(&vm.stack, values), divisor);
        break;
      }
      default: INTERNAL_ERROR("Unknown opcode '%d'", opcode);
    }
  }

#undef BINARY_OP
#undef READ_BYTE

  // successful execution
  return true;
}

/**@desc run virtual machine; execute bytecode `chunk`
@return true if execution succeeded, false otherwise*/
bool vm_run(Chunk const *const chunk) {
  assert(chunk != NULL);

  // reset vm
  vm.chunk = chunk;
  vm.ip = chunk->code;

  // execute chunk
  return vm_execute();
}
