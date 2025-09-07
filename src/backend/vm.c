#include "backend/vm.h"

#include "backend/gc.h"
#include "backend/value.h"
#include "global.h"
#include "utils/debug.h"
#include "utils/error.h"
#include "utils/io.h"
#include "utils/memory.h"
#include "utils/stack.h"

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

#define VM_STACK_INITIAL_CAPACITY 256
#define VM_STACK_GROWTH_FACTOR 2
#define VM_STACK_TOP STACK_TOP(&vm.stack)

#define READ_INSTRUCTION_BYTE() (*vm.ip++)

#define GET_INSTRUCTION_OFFSET(instruction_byte_length) (vm.ip - vm.chunk->code.data - (instruction_byte_length))

#define ASSERT_MIN_VM_STACK_COUNT(expected_min_vm_stack_count) \
  assert(vm.stack.count >= (expected_min_vm_stack_count) && "Attempt to access nonexistent vm.stack frame")

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

void vm_reset(void);

// *---------------------------------------------*
// *          INTERNAL-LINKAGE OBJECTS           *
// *---------------------------------------------*

static VM vm;

// *---------------------------------------------*
// *          EXTERNAL-LINKAGE OBJECTS           *
// *---------------------------------------------*

/**@desc exposes vm.stack.count (meant solely for automated tests)*/
size_t const *const t_vm_stack_count = &vm.stack.count;

// *---------------------------------------------*
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/**@desc handle bytecode execution error at `instruction_offset` with `format` message and `format_args`
@return false (meant to be forwarded as an execution failure indication)*/
static bool vm_error_at(ptrdiff_t const instruction_offset, char const *const format, ...) {
  assert(instruction_offset >= 0);
  assert(format != NULL);

  va_list format_args;
  va_start(format_args, format);
  int32_t const instruction_line = chunk_get_instruction_line(vm.chunk, instruction_offset);

  io_fprintf(
    g_bytecode_execution_error_stream, "[EXECUTION_ERROR]" COMMON_MS COMMON_FILE_LINE_FORMAT COMMON_MS,
    g_source_file_path, instruction_line
  );
  if (vfprintf(g_bytecode_execution_error_stream, format, format_args) < 0) ERROR_IO_ERRNO();
  io_fprintf(g_bytecode_execution_error_stream, "\n");

  va_end(format_args);

  return false;
}

// *---------------------------------------------*
// *         EXTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/**@desc initialize virtual machine*/
void vm_init(void) {
  STACK_INIT_EXPLICIT(&vm.stack, sizeof(Value), gc_memory_manage, VM_STACK_INITIAL_CAPACITY, VM_STACK_GROWTH_FACTOR);
}

/**@desc release virtual machine resources and set it to uninitialized state*/
void vm_destroy(void) {
  STACK_DESTROY(&vm.stack);

  vm = (VM){0};
}

/**@desc push `value` on top of virtual machine stack*/
void vm_stack_push(Value const value) {
  STACK_PUSH(&vm.stack, value);
}

/**@desc pop value from virtual machine stack
@return popped value*/
Value vm_stack_pop(void) {
  return STACK_POP(&vm.stack);
}

/**@desc execute bytecode `chunk`; virtual machine state persists across `chunk` executions
@return true if execution succeeded, false otherwise*/
bool vm_execute(Chunk const *const chunk) {
  assert(chunk != NULL);

  // configure vm for chunk execution
  vm.chunk = chunk;
  vm.ip = chunk->code.data;

#ifdef DEBUG_VM
  io_puts("\n== DEBUG_VM ==");
#endif

  for (;;) {
#ifdef DEBUG_VM
    io_printf("[ ");
    for (size_t i = 0; i < vm.stack.count;) {
      value_print(vm.stack.data[i]);
      if (++i < vm.stack.count) io_printf(", ");
    }
    io_puts(" ]");
    debug_disassemble_instruction(vm.chunk, vm.ip - vm.chunk->code.data);
#endif
    assert(vm.ip < vm.chunk->code.data + vm.chunk->code.count && "Instruction pointer out of bounds");
    uint8_t const opcode = READ_INSTRUCTION_BYTE();

    static_assert(CHUNK_OP_OPCODE_COUNT == 21, "Exhaustive ChunkOpCode handling");
    switch (opcode) {
      case CHUNK_OP_RETURN: {
        return true; // successful chunk execution
      }
      case CHUNK_OP_PRINT: {
        value_print(vm_stack_pop());
        io_fprintf(g_source_program_output_stream, "\n");
        break;
      }
      case CHUNK_OP_POP: {
        vm_stack_pop();
        break;
      }
      case CHUNK_OP_CONSTANT: {
        Value constant = vm.chunk->constants.data[READ_INSTRUCTION_BYTE()];
        vm_stack_push(constant);
        break;
      }
      case CHUNK_OP_CONSTANT_2B: {
        uint8_t const constant_index_LSB = READ_INSTRUCTION_BYTE();
        uint8_t const constant_index_MSB = READ_INSTRUCTION_BYTE();
        uint32_t const constant_index = memory_concatenate_bytes(2, constant_index_MSB, constant_index_LSB);
        Value const constant = vm.chunk->constants.data[constant_index];

        vm_stack_push(constant);
        break;
      }
      case CHUNK_OP_NIL: {
        vm_stack_push(value_make_nil());
        break;
      }
      case CHUNK_OP_TRUE: {
        vm_stack_push(value_make_bool(true));
        break;
      }
      case CHUNK_OP_FALSE: {
        vm_stack_push(value_make_bool(false));
        break;
      }
      case CHUNK_OP_NEGATE: {
        ASSERT_MIN_VM_STACK_COUNT(1);

        if (!value_is_number(VM_STACK_TOP)) {
          return vm_error_at(
            GET_INSTRUCTION_OFFSET(1), "Expected negation operand to be a number (got '%s')",
            value_type_to_string_table[VM_STACK_TOP.type]
          );
        }
        VM_STACK_TOP.payload.number = -VM_STACK_TOP.payload.number;
        break;
      }
      case CHUNK_OP_ADD: {
        ASSERT_MIN_VM_STACK_COUNT(2);

        Value const second_operand = vm_stack_pop();
        if (!value_is_number(VM_STACK_TOP) || !value_is_number(second_operand)) {
          return vm_error_at(
            GET_INSTRUCTION_OFFSET(1), "Expected addition operands to be numbers (got '%s' and '%s')",
            value_type_to_string_table[VM_STACK_TOP.type], value_type_to_string_table[second_operand.type]
          );
        }
        VM_STACK_TOP.payload.number = VM_STACK_TOP.payload.number + second_operand.payload.number;
        break;
      }
      case CHUNK_OP_SUBTRACT: {
        ASSERT_MIN_VM_STACK_COUNT(2);

        Value const second_operand = vm_stack_pop();
        if (!value_is_number(VM_STACK_TOP) || !value_is_number(second_operand)) {
          return vm_error_at(
            GET_INSTRUCTION_OFFSET(1), "Expected subtraction operands to be numbers (got '%s' and '%s')",
            value_type_to_string_table[VM_STACK_TOP.type], value_type_to_string_table[second_operand.type]
          );
        }
        VM_STACK_TOP.payload.number = VM_STACK_TOP.payload.number - second_operand.payload.number;
        break;
      }
      case CHUNK_OP_MULTIPLY: {
        ASSERT_MIN_VM_STACK_COUNT(2);

        Value const second_operand = vm_stack_pop();
        if (!value_is_number(VM_STACK_TOP) || !value_is_number(second_operand)) {
          return vm_error_at(
            GET_INSTRUCTION_OFFSET(1), "Expected multiplication operands to be numbers (got '%s' and '%s')",
            value_type_to_string_table[VM_STACK_TOP.type], value_type_to_string_table[second_operand.type]
          );
        }
        VM_STACK_TOP.payload.number = VM_STACK_TOP.payload.number * second_operand.payload.number;
        break;
      }
      case CHUNK_OP_DIVIDE: {
        ASSERT_MIN_VM_STACK_COUNT(2);

        Value const second_operand = vm_stack_pop();
        if (!value_is_number(VM_STACK_TOP) || !value_is_number(second_operand)) {
          return vm_error_at(
            GET_INSTRUCTION_OFFSET(1), "Expected division operands to be numbers (got '%s' and '%s')",
            value_type_to_string_table[VM_STACK_TOP.type], value_type_to_string_table[second_operand.type]
          );
        }
        if (second_operand.payload.number == 0)
          return vm_error_at(GET_INSTRUCTION_OFFSET(1), "Illegal division by zero");
        VM_STACK_TOP.payload.number = VM_STACK_TOP.payload.number / second_operand.payload.number;
        break;
      }
      case CHUNK_OP_MODULO: {
        ASSERT_MIN_VM_STACK_COUNT(2);

        Value const second_operand = vm_stack_pop();
        if (!value_is_number(VM_STACK_TOP) || !value_is_number(second_operand)) {
          return vm_error_at(
            GET_INSTRUCTION_OFFSET(1), "Expected modulo operands to be numbers (got '%s' and '%s')",
            value_type_to_string_table[VM_STACK_TOP.type], value_type_to_string_table[second_operand.type]
          );
        }
        if (second_operand.payload.number == 0) return vm_error_at(GET_INSTRUCTION_OFFSET(1), "Illegal modulo by zero");
        VM_STACK_TOP.payload.number = fmod(VM_STACK_TOP.payload.number, second_operand.payload.number);
        break;
      }
      case CHUNK_OP_NOT: {
        ASSERT_MIN_VM_STACK_COUNT(1);

        VM_STACK_TOP = value_make_bool(value_is_falsy(VM_STACK_TOP));
        break;
      }
      case CHUNK_OP_EQUAL: {
        ASSERT_MIN_VM_STACK_COUNT(2);

        Value const second_operand = vm_stack_pop();
        VM_STACK_TOP = value_make_bool(value_equals(VM_STACK_TOP, second_operand));
        break;
      }
      case CHUNK_OP_NOT_EQUAL: {
        ASSERT_MIN_VM_STACK_COUNT(2);

        Value const second_operand = vm_stack_pop();
        VM_STACK_TOP = value_make_bool(!value_equals(VM_STACK_TOP, second_operand));
        break;
      }
      case CHUNK_OP_LESS: {
        ASSERT_MIN_VM_STACK_COUNT(2);

        Value const second_operand = vm_stack_pop();
        if (!value_is_number(VM_STACK_TOP) || !value_is_number(second_operand)) {
          return vm_error_at(
            GET_INSTRUCTION_OFFSET(1), "Expected less-than operands to be numbers (got '%s' and '%s')",
            value_type_to_string_table[VM_STACK_TOP.type], value_type_to_string_table[second_operand.type]
          );
        }
        VM_STACK_TOP = value_make_bool(VM_STACK_TOP.payload.number < second_operand.payload.number);
        break;
      }
      case CHUNK_OP_LESS_EQUAL: {
        ASSERT_MIN_VM_STACK_COUNT(2);

        Value const second_operand = vm_stack_pop();
        if (!value_is_number(VM_STACK_TOP) || !value_is_number(second_operand)) {
          return vm_error_at(
            GET_INSTRUCTION_OFFSET(1), "Expected less-than-or-equal operands to be numbers (got '%s' and '%s')",
            value_type_to_string_table[VM_STACK_TOP.type], value_type_to_string_table[second_operand.type]
          );
        }
        VM_STACK_TOP = value_make_bool(VM_STACK_TOP.payload.number <= second_operand.payload.number);
        break;
      }
      case CHUNK_OP_GREATER: {
        ASSERT_MIN_VM_STACK_COUNT(2);

        Value const second_operand = vm_stack_pop();
        if (!value_is_number(VM_STACK_TOP) || !value_is_number(second_operand)) {
          return vm_error_at(
            GET_INSTRUCTION_OFFSET(1), "Expected greater-than operands to be numbers (got '%s' and '%s')",
            value_type_to_string_table[VM_STACK_TOP.type], value_type_to_string_table[second_operand.type]
          );
        }
        VM_STACK_TOP = value_make_bool(VM_STACK_TOP.payload.number > second_operand.payload.number);
        break;
      }
      case CHUNK_OP_GREATER_EQUAL: {
        ASSERT_MIN_VM_STACK_COUNT(2);

        Value const second_operand = vm_stack_pop();
        if (!value_is_number(VM_STACK_TOP) || !value_is_number(second_operand)) {
          return vm_error_at(
            GET_INSTRUCTION_OFFSET(1), "Expected greater-than-or-equal operands to be numbers (got '%s' and '%s')",
            value_type_to_string_table[VM_STACK_TOP.type], value_type_to_string_table[second_operand.type]
          );
        }
        VM_STACK_TOP = value_make_bool(VM_STACK_TOP.payload.number >= second_operand.payload.number);
        break;
      }
      default: ERROR_INTERNAL("Unknown chunk opcode '%d'", opcode);
    }
  }

  ERROR_INTERNAL("Unreachable code executed; vm.chunk execution is expected to be terminated by OP_RETURN or error");
}
