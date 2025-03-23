#include "backend/vm.h"

#include "backend/value.h"
#include "global.h"
#include "utils/debug.h"
#include "utils/error.h"
#include "utils/memory.h"
#include "utils/stack.h"

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

// *---------------------------------------------*
// *               STATIC OBJECTS                *
// *---------------------------------------------*

static VirtualMachine vm;

/**@desc exposes vm.stack.count (meant solely for automated tests)*/
int32_t const *const t_vm_stack_count = &vm.stack.count;

// *---------------------------------------------*
// *                  UTILITIES                  *
// *---------------------------------------------*

#define vm_stack_top_frame STACK_TOP_FRAME(&vm.stack, values)

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

/**@desc handle bytecode execution error at `instruction_offset` with `format` message and `format_args`
@return false (meant to be forwarded as an execution failure indication)*/
static bool vm_error_at(ptrdiff_t const instruction_offset, char const *const format, ...) {
  assert(instruction_offset >= 0);
  assert(format != NULL);

  va_list format_args;
  va_start(format_args, format);
  int32_t const instruction_line = chunk_get_instruction_line(vm.chunk, instruction_offset);

  fprintf(g_execution_error_stream, "[EXECUTION_ERROR]" M_S FILE_LINE_FORMAT M_S, g_source_file, instruction_line);
  vfprintf(g_execution_error_stream, format, format_args);
  fprintf(g_execution_error_stream, "\n");

  va_end(format_args);

  return false;
}

// *---------------------------------------------*
// *        BYTECODE EXECUTION FUNCTIONS         *
// *---------------------------------------------*

/**@desc execute vm.chunk instructions
@return true if execution succeeded, false otherwise*/
bool vm_execute(void) {
#define read_byte() (*vm.ip++)
#define get_instruction_offset(instruction_byte_length) (vm.ip - vm.chunk->code - (instruction_byte_length))
#define assert_min_vm_stack_count(expected_min_vm_stack_count) \
  assert(vm.stack.count >= (expected_min_vm_stack_count) && "Attempt to access nonexistent stack frame")

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
    uint8_t const opcode = read_byte();

    static_assert(OP_OPCODE_COUNT == 21, "Exhaustive opcode handling");
    switch (opcode) {
      case OP_RETURN: {
        return true; // successful chunk execution
      }
      case OP_PRINT: {
        value_print(vm_stack_pop());
        fprintf(g_runtime_output_stream, "\n");
        break;
      }
      case OP_POP: {
        vm_stack_pop();
        break;
      }
      case OP_CONSTANT: {
        Value constant = vm.chunk->constants.values[read_byte()];
        vm_stack_push(constant);
        break;
      }
      case OP_CONSTANT_2B: {
        uint8_t const constant_index_LSB = read_byte();
        uint8_t const constant_index_MSB = read_byte();
        uint32_t const constant_index = concatenate_bytes(2, constant_index_MSB, constant_index_LSB);
        Value const constant = vm.chunk->constants.values[constant_index];

        vm_stack_push(constant);
        break;
      }
      case OP_NIL: {
        vm_stack_push(NIL_VALUE());
        break;
      }
      case OP_TRUE: {
        vm_stack_push(BOOL_VALUE(true));
        break;
      }
      case OP_FALSE: {
        vm_stack_push(BOOL_VALUE(false));
        break;
      }
      case OP_NEGATE: {
        assert_min_vm_stack_count(1);

        if (!IS_NUMBER_VALUE(vm_stack_top_frame)) {
          return vm_error_at(
            get_instruction_offset(1), "Expected negation operand to be a number (got '%s')",
            value_type_to_string_table[vm_stack_top_frame.type]
          );
        }
        vm_stack_top_frame.payload.number = -vm_stack_top_frame.payload.number;
        break;
      }
      case OP_ADD: {
        assert_min_vm_stack_count(2);

        Value const second_operand = vm_stack_pop();
        if (!IS_NUMBER_VALUE(vm_stack_top_frame) || !IS_NUMBER_VALUE(second_operand)) {
          return vm_error_at(
            get_instruction_offset(1), "Expected addition operands to be numbers (got '%s' and '%s')",
            value_type_to_string_table[vm_stack_top_frame.type], value_type_to_string_table[second_operand.type]
          );
        }
        vm_stack_top_frame.payload.number = vm_stack_top_frame.payload.number + second_operand.payload.number;
        break;
      }
      case OP_SUBTRACT: {
        assert_min_vm_stack_count(2);

        Value const second_operand = vm_stack_pop();
        if (!IS_NUMBER_VALUE(vm_stack_top_frame) || !IS_NUMBER_VALUE(second_operand)) {
          return vm_error_at(
            get_instruction_offset(1), "Expected subtraction operands to be numbers (got '%s' and '%s')",
            value_type_to_string_table[vm_stack_top_frame.type], value_type_to_string_table[second_operand.type]
          );
        }
        vm_stack_top_frame.payload.number = vm_stack_top_frame.payload.number - second_operand.payload.number;
        break;
      }
      case OP_MULTIPLY: {
        assert_min_vm_stack_count(2);

        Value const second_operand = vm_stack_pop();
        if (!IS_NUMBER_VALUE(vm_stack_top_frame) || !IS_NUMBER_VALUE(second_operand)) {
          return vm_error_at(
            get_instruction_offset(1), "Expected multiplication operands to be numbers (got '%s' and '%s')",
            value_type_to_string_table[vm_stack_top_frame.type], value_type_to_string_table[second_operand.type]
          );
        }
        vm_stack_top_frame.payload.number = vm_stack_top_frame.payload.number * second_operand.payload.number;
        break;
      }
      case OP_DIVIDE: {
        assert_min_vm_stack_count(2);

        Value const second_operand = vm_stack_pop();
        if (!IS_NUMBER_VALUE(vm_stack_top_frame) || !IS_NUMBER_VALUE(second_operand)) {
          return vm_error_at(
            get_instruction_offset(1), "Expected division operands to be numbers (got '%s' and '%s')",
            value_type_to_string_table[vm_stack_top_frame.type], value_type_to_string_table[second_operand.type]
          );
        }
        if (second_operand.payload.number == 0)
          return vm_error_at(get_instruction_offset(1), "Illegal division by zero");
        vm_stack_top_frame.payload.number = vm_stack_top_frame.payload.number / second_operand.payload.number;
        break;
      }
      case OP_MODULO: {
        assert_min_vm_stack_count(2);

        Value const second_operand = vm_stack_pop();
        if (!IS_NUMBER_VALUE(vm_stack_top_frame) || !IS_NUMBER_VALUE(second_operand)) {
          return vm_error_at(
            get_instruction_offset(1), "Expected modulo operands to be numbers (got '%s' and '%s')",
            value_type_to_string_table[vm_stack_top_frame.type], value_type_to_string_table[second_operand.type]
          );
        }
        if (second_operand.payload.number == 0) return vm_error_at(get_instruction_offset(1), "Illegal modulo by zero");
        vm_stack_top_frame.payload.number = fmod(vm_stack_top_frame.payload.number, second_operand.payload.number);
        break;
      }
      case OP_NOT: {
        assert_min_vm_stack_count(1);

        vm_stack_top_frame = BOOL_VALUE(IS_FALSY_VALUE(vm_stack_top_frame));
        break;
      }
      case OP_EQUAL: {
        assert_min_vm_stack_count(2);

        Value const second_operand = vm_stack_pop();
        vm_stack_top_frame = BOOL_VALUE(value_equals(vm_stack_top_frame, second_operand));
        break;
      }
      case OP_NOT_EQUAL: {
        assert_min_vm_stack_count(2);

        Value const second_operand = vm_stack_pop();
        vm_stack_top_frame = BOOL_VALUE(!value_equals(vm_stack_top_frame, second_operand));
        break;
      }
      case OP_LESS: {
        assert_min_vm_stack_count(2);

        Value const second_operand = vm_stack_pop();
        if (!IS_NUMBER_VALUE(vm_stack_top_frame) || !IS_NUMBER_VALUE(second_operand)) {
          return vm_error_at(
            get_instruction_offset(1), "Expected less-than operands to be numbers (got '%s' and '%s')",
            value_type_to_string_table[vm_stack_top_frame.type], value_type_to_string_table[second_operand.type]
          );
        }
        vm_stack_top_frame = BOOL_VALUE(vm_stack_top_frame.payload.number < second_operand.payload.number);
        break;
      }
      case OP_LESS_EQUAL: {
        assert_min_vm_stack_count(2);

        Value const second_operand = vm_stack_pop();
        if (!IS_NUMBER_VALUE(vm_stack_top_frame) || !IS_NUMBER_VALUE(second_operand)) {
          return vm_error_at(
            get_instruction_offset(1), "Expected less-than-or-equal operands to be numbers (got '%s' and '%s')",
            value_type_to_string_table[vm_stack_top_frame.type], value_type_to_string_table[second_operand.type]
          );
        }
        vm_stack_top_frame = BOOL_VALUE(vm_stack_top_frame.payload.number <= second_operand.payload.number);
        break;
      }
      case OP_GREATER: {
        assert_min_vm_stack_count(2);

        Value const second_operand = vm_stack_pop();
        if (!IS_NUMBER_VALUE(vm_stack_top_frame) || !IS_NUMBER_VALUE(second_operand)) {
          return vm_error_at(
            get_instruction_offset(1), "Expected greater-than operands to be numbers (got '%s' and '%s')",
            value_type_to_string_table[vm_stack_top_frame.type], value_type_to_string_table[second_operand.type]
          );
        }
        vm_stack_top_frame = BOOL_VALUE(vm_stack_top_frame.payload.number > second_operand.payload.number);
        break;
      }
      case OP_GREATER_EQUAL: {
        assert_min_vm_stack_count(2);

        Value const second_operand = vm_stack_pop();
        if (!IS_NUMBER_VALUE(vm_stack_top_frame) || !IS_NUMBER_VALUE(second_operand)) {
          return vm_error_at(
            get_instruction_offset(1), "Expected greater-than-or-equal operands to be numbers (got '%s' and '%s')",
            value_type_to_string_table[vm_stack_top_frame.type], value_type_to_string_table[second_operand.type]
          );
        }
        vm_stack_top_frame = BOOL_VALUE(vm_stack_top_frame.payload.number >= second_operand.payload.number);
        break;
      }
      default: INTERNAL_ERROR("Unknown opcode '%d'", opcode);
    }
  }

  INTERNAL_ERROR("Unreachable code executed; vm.chunk execution is expected to be terminated by OP_RETURN or error");

#undef read_byte
#undef get_instruction_offset
#undef assert_min_vm_stack_count
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
