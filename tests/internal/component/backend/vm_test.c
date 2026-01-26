#include "backend/vm.h"

#include "backend/chunk.h"
#include "backend/value.h"
#include "component/component_test.h"
#include "global.h"
#include "utils/error.h"

#include <stdio.h>

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

#define EXECUTE_ASSERT_SUCCESS() assert_true(execute())
#define EXECUTE_ASSERT_FAILURE() assert_false(execute())

#define ASSERT_EMPTY_STACK() assert_int_equal(*t_vm_stack_count, 0)

#define STACK_POP_ASSERT(expected_value) component_test_assert_value_equality(vm_stack_pop(), expected_value)
#define STACK_POP_ASSERT_MANY(...) COMPONENT_TEST_APPLY_TO_EACH_ARG(STACK_POP_ASSERT, Value, __VA_ARGS__)

#define APPEND_CONSTANT_INSTRUCTION(constant) chunk_append_constant_instruction(&chunk, constant, 1)
#define APPEND_CONSTANT_INSTRUCTIONS(...) \
  COMPONENT_TEST_APPLY_TO_EACH_ARG(APPEND_CONSTANT_INSTRUCTION, Value, __VA_ARGS__)

#define APPEND_INSTRUCTION(opcode) chunk_append_instruction(&chunk, opcode, 1)
#define APPEND_INSTRUCTIONS(...) COMPONENT_TEST_APPLY_TO_EACH_ARG(APPEND_INSTRUCTION, ChunkOpCode, __VA_ARGS__)

#define ASSERT_EXECUTION_ERROR(expected_error_message)                                         \
  component_test_assert_file_content(                                                          \
    g_bytecode_execution_error_stream,                                                         \
    "[EXECUTION_ERROR]" COMMON_MS __FILE__ COMMON_PS "1" COMMON_MS expected_error_message "\n" \
  )

#define ASSERT_SOURCE_PROGRAM_OUTPUT(expected_output) \
  component_test_assert_file_content(g_source_program_output_stream, expected_output "\n")

#define ASSERT_INVALID_BINARY_NUMERIC_OPERATOR_OPERAND_TYPES(operator_instruction, operator_descriptor)          \
  do {                                                                                                           \
    reset_test_case_env();                                                                                       \
    APPEND_INSTRUCTIONS(CHUNK_OP_NIL, CHUNK_OP_NIL, operator_instruction, CHUNK_OP_RETURN);                      \
    EXECUTE_ASSERT_FAILURE();                                                                                    \
    ASSERT_EXECUTION_ERROR("Expected " operator_descriptor " operands to be numbers (got 'nil' and 'nil')");     \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    APPEND_INSTRUCTION(CHUNK_OP_NIL);                                                                            \
    APPEND_CONSTANT_INSTRUCTION(value_make_number(1));                                                           \
    APPEND_INSTRUCTIONS(operator_instruction, CHUNK_OP_RETURN);                                                  \
    EXECUTE_ASSERT_FAILURE();                                                                                    \
    ASSERT_EXECUTION_ERROR("Expected " operator_descriptor " operands to be numbers (got 'nil' and 'number')");  \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    APPEND_CONSTANT_INSTRUCTION(value_make_number(1));                                                           \
    APPEND_INSTRUCTIONS(CHUNK_OP_NIL, operator_instruction, CHUNK_OP_RETURN);                                    \
    EXECUTE_ASSERT_FAILURE();                                                                                    \
    ASSERT_EXECUTION_ERROR("Expected " operator_descriptor " operands to be numbers (got 'number' and 'nil')");  \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    APPEND_INSTRUCTIONS(CHUNK_OP_TRUE, CHUNK_OP_FALSE, operator_instruction, CHUNK_OP_RETURN);                   \
    EXECUTE_ASSERT_FAILURE();                                                                                    \
    ASSERT_EXECUTION_ERROR("Expected " operator_descriptor " operands to be numbers (got 'bool' and 'bool')");   \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    APPEND_INSTRUCTION(CHUNK_OP_TRUE);                                                                           \
    APPEND_CONSTANT_INSTRUCTION(value_make_number(1));                                                           \
    APPEND_INSTRUCTIONS(operator_instruction, CHUNK_OP_RETURN);                                                  \
    EXECUTE_ASSERT_FAILURE();                                                                                    \
    ASSERT_EXECUTION_ERROR("Expected " operator_descriptor " operands to be numbers (got 'bool' and 'number')"); \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    APPEND_CONSTANT_INSTRUCTION(value_make_number(1));                                                           \
    APPEND_INSTRUCTIONS(CHUNK_OP_FALSE, operator_instruction, CHUNK_OP_RETURN);                                  \
    EXECUTE_ASSERT_FAILURE();                                                                                    \
    ASSERT_EXECUTION_ERROR("Expected " operator_descriptor " operands to be numbers (got 'number' and 'bool')"); \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    APPEND_INSTRUCTIONS(CHUNK_OP_NIL, CHUNK_OP_TRUE, operator_instruction, CHUNK_OP_RETURN);                     \
    EXECUTE_ASSERT_FAILURE();                                                                                    \
    ASSERT_EXECUTION_ERROR("Expected " operator_descriptor " operands to be numbers (got 'nil' and 'bool')");    \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    APPEND_INSTRUCTIONS(CHUNK_OP_FALSE, CHUNK_OP_NIL, operator_instruction, CHUNK_OP_RETURN);                    \
    EXECUTE_ASSERT_FAILURE();                                                                                    \
    ASSERT_EXECUTION_ERROR("Expected " operator_descriptor " operands to be numbers (got 'bool' and 'nil')");    \
  } while (0)

#define ASSERT_VALUE_IS_RESULT_OF_INSTRUCTION_ON_VALUES(expected_value, instruction, ...) \
  do {                                                                                    \
    reset_test_case_env();                                                                \
    APPEND_CONSTANT_INSTRUCTIONS(__VA_ARGS__);                                            \
    APPEND_INSTRUCTIONS(instruction, CHUNK_OP_RETURN);                                    \
    EXECUTE_ASSERT_SUCCESS();                                                             \
    STACK_POP_ASSERT(expected_value);                                                     \
    ASSERT_EMPTY_STACK();                                                                 \
  } while (0)

// *---------------------------------------------*
// *          INTERNAL-LINKAGE OBJECTS           *
// *---------------------------------------------*

static Chunk chunk;

// *---------------------------------------------*
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

static bool execute(void) {
  component_test_clear_binary_stream_resource_content(g_bytecode_execution_error_stream);
  component_test_clear_binary_stream_resource_content(g_source_program_output_stream);

  return vm_execute(&chunk);
}

static void reset_test_case_env(void) {
  vm_reset();
  chunk_reset(&chunk);
}

// *---------------------------------------------*
// *                  FIXTURES                   *
// *---------------------------------------------*

static int setup_test_group_env(void **const _) {
  g_source_file_path = __FILE__;

  g_bytecode_execution_error_stream = tmpfile();
  if (g_bytecode_execution_error_stream == NULL) ERROR_IO_ERRNO();

  g_source_program_output_stream = tmpfile();
  if (g_source_program_output_stream == NULL) ERROR_IO_ERRNO();

  return 0;
}

static int teardown_test_group_env(void **const _) {
  if (fclose(g_bytecode_execution_error_stream)) ERROR_IO_ERRNO();
  if (fclose(g_source_program_output_stream)) ERROR_IO_ERRNO();

  return 0;
}

static int setup_test_case_env(void **const _) {
  vm_init();
  chunk_init(&chunk);

  return 0;
}

static int teardown_test_case_env(void **const _) {
  vm_destroy();
  chunk_destroy(&chunk);

  return 0;
}

// *---------------------------------------------*
// *                 TEST CASES                  *
// *---------------------------------------------*
static_assert(CHUNK_OP_OPCODE_COUNT == 21, "Exhaustive ChunkOpCode handling");

static void test_CHUNK_OP_CONSTANT(void **const _) {
  APPEND_CONSTANT_INSTRUCTIONS(value_make_number(1), value_make_number(2), value_make_number(3));
  APPEND_INSTRUCTION(CHUNK_OP_RETURN);
  EXECUTE_ASSERT_SUCCESS();
  STACK_POP_ASSERT_MANY(value_make_number(3), value_make_number(2), value_make_number(1));
  ASSERT_EMPTY_STACK();
}

static void test_CHUNK_OP_CONSTANT_2B(void **const _) {
  // force CHUNK_OP_CONSTANT_2B usage
  for (int i = 0; i < UCHAR_MAX; i++) value_list_append(&chunk.constants, value_make_number(i));

  APPEND_CONSTANT_INSTRUCTIONS(value_make_number(1), value_make_number(2), value_make_number(3));
  APPEND_INSTRUCTION(CHUNK_OP_RETURN);
  EXECUTE_ASSERT_SUCCESS();
  STACK_POP_ASSERT_MANY(value_make_number(3), value_make_number(2), value_make_number(1));
  ASSERT_EMPTY_STACK();
}

static void test_CHUNK_OP_NIL(void **const _) {
  APPEND_INSTRUCTIONS(CHUNK_OP_NIL, CHUNK_OP_RETURN);
  EXECUTE_ASSERT_SUCCESS();
  STACK_POP_ASSERT(value_make_nil());
  ASSERT_EMPTY_STACK();
}

static void test_CHUNK_OP_TRUE(void **const _) {
  APPEND_INSTRUCTIONS(CHUNK_OP_TRUE, CHUNK_OP_RETURN);
  EXECUTE_ASSERT_SUCCESS();
  STACK_POP_ASSERT(value_make_bool(true));
  ASSERT_EMPTY_STACK();
}

static void test_CHUNK_OP_FALSE(void **const _) {
  APPEND_INSTRUCTIONS(CHUNK_OP_FALSE, CHUNK_OP_RETURN);
  EXECUTE_ASSERT_SUCCESS();
  STACK_POP_ASSERT(value_make_bool(false));
  ASSERT_EMPTY_STACK();
}

static void test_CHUNK_OP_PRINT(void **const _) {
  APPEND_CONSTANT_INSTRUCTION(value_make_number(1));
  APPEND_INSTRUCTIONS(CHUNK_OP_PRINT, CHUNK_OP_RETURN);
  EXECUTE_ASSERT_SUCCESS();
  ASSERT_SOURCE_PROGRAM_OUTPUT("1");
  ASSERT_EMPTY_STACK();
}

static void test_CHUNK_OP_POP(void **const _) {
  APPEND_CONSTANT_INSTRUCTIONS(value_make_number(1), value_make_number(2));
  APPEND_INSTRUCTIONS(CHUNK_OP_POP, CHUNK_OP_RETURN);
  EXECUTE_ASSERT_SUCCESS();
  STACK_POP_ASSERT(value_make_number(1));
  ASSERT_EMPTY_STACK();
}

static void test_CHUNK_OP_NEGATE(void **const _) {
#define ASSERT_CHUNK_OP_NEGATE(number_a, expected_number_b)                            \
  ASSERT_VALUE_IS_RESULT_OF_INSTRUCTION_ON_VALUES(                                     \
    value_make_number(expected_number_b), CHUNK_OP_NEGATE, value_make_number(number_a) \
  )

  // signed integer negation
  ASSERT_CHUNK_OP_NEGATE(1, -1);
  ASSERT_CHUNK_OP_NEGATE(-2, 2);

  // floating-point negation
  ASSERT_CHUNK_OP_NEGATE(1.25, -1.25);

  // signed zero negation
  ASSERT_CHUNK_OP_NEGATE(0, -0);
  ASSERT_CHUNK_OP_NEGATE(-0, 0);

  // negation stacking
  reset_test_case_env();
  APPEND_CONSTANT_INSTRUCTION(value_make_number(2));
  APPEND_INSTRUCTIONS(CHUNK_OP_NEGATE, CHUNK_OP_NEGATE, CHUNK_OP_RETURN);
  EXECUTE_ASSERT_SUCCESS();
  STACK_POP_ASSERT(value_make_number(2));
  ASSERT_EMPTY_STACK();

  reset_test_case_env();
  APPEND_CONSTANT_INSTRUCTION(value_make_number(3));
  APPEND_INSTRUCTIONS(CHUNK_OP_NEGATE, CHUNK_OP_NEGATE, CHUNK_OP_NEGATE, CHUNK_OP_RETURN);
  EXECUTE_ASSERT_SUCCESS();
  STACK_POP_ASSERT(value_make_number(-3));
  ASSERT_EMPTY_STACK();

  // invalid operand types
  reset_test_case_env();
  APPEND_INSTRUCTIONS(CHUNK_OP_NIL, CHUNK_OP_NEGATE, CHUNK_OP_RETURN);
  EXECUTE_ASSERT_FAILURE();
  ASSERT_EXECUTION_ERROR("Expected negation operand to be a number (got 'nil')");

  reset_test_case_env();
  APPEND_INSTRUCTIONS(CHUNK_OP_TRUE, CHUNK_OP_NEGATE, CHUNK_OP_RETURN);
  EXECUTE_ASSERT_FAILURE();
  ASSERT_EXECUTION_ERROR("Expected negation operand to be a number (got 'bool')");

  reset_test_case_env();
  APPEND_INSTRUCTIONS(CHUNK_OP_FALSE, CHUNK_OP_NEGATE, CHUNK_OP_RETURN);
  EXECUTE_ASSERT_FAILURE();
  ASSERT_EXECUTION_ERROR("Expected negation operand to be a number (got 'bool')");

#undef ASSERT_CHUNK_OP_NEGATE
}

static void test_CHUNK_OP_ADD(void **const _) {
#define ASSERT_CHUNK_OP_ADD(number_a, number_b, expected_number_c)                                               \
  ASSERT_VALUE_IS_RESULT_OF_INSTRUCTION_ON_VALUES(                                                               \
    value_make_number(expected_number_c), CHUNK_OP_ADD, value_make_number(number_a), value_make_number(number_b) \
  )

  // integer addition and commutativity
  ASSERT_CHUNK_OP_ADD(1, 2, 3);
  ASSERT_CHUNK_OP_ADD(2, 1, 3);

  // addition properties
  ASSERT_CHUNK_OP_ADD(2, 0, 2);
  ASSERT_CHUNK_OP_ADD(2, -2, 0);

  // floating-point addition
  ASSERT_CHUNK_OP_ADD(1.7, 2.25, 3.95);

  // signed integer addition
  ASSERT_CHUNK_OP_ADD(5, -7, -2);
  ASSERT_CHUNK_OP_ADD(-5, 7, 2);
  ASSERT_CHUNK_OP_ADD(-5, -7, -12);

  // signed zero addition
  ASSERT_CHUNK_OP_ADD(0, 0, 0);
  ASSERT_CHUNK_OP_ADD(0, -0, 0);
  ASSERT_CHUNK_OP_ADD(-0, 0, 0);
  ASSERT_CHUNK_OP_ADD(-0, -0, -0);

  // invalid operand types
  ASSERT_INVALID_BINARY_NUMERIC_OPERATOR_OPERAND_TYPES(CHUNK_OP_ADD, "addition");

#undef ASSERT_CHUNK_OP_ADD
}

static void test_CHUNK_OP_SUBTRACT(void **const _) {
#define ASSERT_CHUNK_OP_SUBTRACT(number_a, number_b, expected_number_c)                                               \
  ASSERT_VALUE_IS_RESULT_OF_INSTRUCTION_ON_VALUES(                                                                    \
    value_make_number(expected_number_c), CHUNK_OP_SUBTRACT, value_make_number(number_a), value_make_number(number_b) \
  )

  // integer subtraction and non-commutativity
  ASSERT_CHUNK_OP_SUBTRACT(4, 3, 1);
  ASSERT_CHUNK_OP_SUBTRACT(3, 4, -1);

  // subtraction properties
  ASSERT_CHUNK_OP_SUBTRACT(2, 0, 2);
  ASSERT_CHUNK_OP_SUBTRACT(25, 25, 0);

  // floating-point subtraction
  ASSERT_CHUNK_OP_SUBTRACT(3.75, 2.45, 1.3);

  // signed integer subtraction
  ASSERT_CHUNK_OP_SUBTRACT(-4, 3, -7);
  ASSERT_CHUNK_OP_SUBTRACT(4, -3, 7);
  ASSERT_CHUNK_OP_SUBTRACT(-4, -3, -1);

  // signed zero subtraction
  ASSERT_CHUNK_OP_SUBTRACT(0, 0, 0);
  ASSERT_CHUNK_OP_SUBTRACT(0, -0, 0);
  ASSERT_CHUNK_OP_SUBTRACT(-0, 0, -0);
  ASSERT_CHUNK_OP_SUBTRACT(-0, -0, 0);

  // invalid operand types
  ASSERT_INVALID_BINARY_NUMERIC_OPERATOR_OPERAND_TYPES(CHUNK_OP_SUBTRACT, "subtraction");

#undef ASSERT_CHUNK_OP_SUBTRACT
}

static void test_CHUNK_OP_MULTIPLY(void **const _) {
#define ASSERT_CHUNK_OP_MULTIPLY(number_a, number_b, expected_number_c)                                               \
  ASSERT_VALUE_IS_RESULT_OF_INSTRUCTION_ON_VALUES(                                                                    \
    value_make_number(expected_number_c), CHUNK_OP_MULTIPLY, value_make_number(number_a), value_make_number(number_b) \
  )

  // integer multiplication and commutativity
  ASSERT_CHUNK_OP_MULTIPLY(5, 3, 15);
  ASSERT_CHUNK_OP_MULTIPLY(3, 5, 15);

  // multiplication properties
  ASSERT_CHUNK_OP_MULTIPLY(125, 1, 125);
  ASSERT_CHUNK_OP_MULTIPLY(50, 0, 0);

  // floating-point multiplication
  ASSERT_CHUNK_OP_MULTIPLY(12.34, 0.3, 3.702);

  // signed integer multiplication
  ASSERT_CHUNK_OP_MULTIPLY(-2, 4, -8);
  ASSERT_CHUNK_OP_MULTIPLY(2, -4, -8);
  ASSERT_CHUNK_OP_MULTIPLY(-2, -4, 8);

  // signed zero multiplication
  ASSERT_CHUNK_OP_MULTIPLY(0, 0, 0);
  ASSERT_CHUNK_OP_MULTIPLY(0, -0, -0);
  ASSERT_CHUNK_OP_MULTIPLY(-0, 0, -0);
  ASSERT_CHUNK_OP_MULTIPLY(-0, -0, 0);

  // invalid operand types
  ASSERT_INVALID_BINARY_NUMERIC_OPERATOR_OPERAND_TYPES(CHUNK_OP_MULTIPLY, "multiplication");

#undef ASSERT_CHUNK_OP_MULTIPLY
}

static void test_CHUNK_OP_DIVIDE(void **const _) {
#define ASSERT_CHUNK_OP_DIVIDE(number_a, number_b, expected_number_c)                                               \
  ASSERT_VALUE_IS_RESULT_OF_INSTRUCTION_ON_VALUES(                                                                  \
    value_make_number(expected_number_c), CHUNK_OP_DIVIDE, value_make_number(number_a), value_make_number(number_b) \
  )

  // integer division and non-commutativity
  ASSERT_CHUNK_OP_DIVIDE(8, 2, 4);
  ASSERT_CHUNK_OP_DIVIDE(2, 8, 0.25);

  // division properties
  ASSERT_CHUNK_OP_DIVIDE(4, 1, 4);
  ASSERT_CHUNK_OP_DIVIDE(25, 25, 1);

  // floating-point division
  ASSERT_CHUNK_OP_DIVIDE(4.2, 1.5, 2.8);

  // signed integer division
  ASSERT_CHUNK_OP_DIVIDE(-5, 2, -2.5);
  ASSERT_CHUNK_OP_DIVIDE(5, -2, -2.5);
  ASSERT_CHUNK_OP_DIVIDE(-5, -2, 2.5);

  // signed zero division
  ASSERT_CHUNK_OP_DIVIDE(0, 2, 0);
  ASSERT_CHUNK_OP_DIVIDE(0, -2, -0);
  ASSERT_CHUNK_OP_DIVIDE(-0, 2, -0);
  ASSERT_CHUNK_OP_DIVIDE(-0, -2, 0);

  // division by zero
  reset_test_case_env();
  APPEND_CONSTANT_INSTRUCTIONS(value_make_number(5), value_make_number(0));
  APPEND_INSTRUCTIONS(CHUNK_OP_DIVIDE, CHUNK_OP_RETURN);
  EXECUTE_ASSERT_FAILURE();
  ASSERT_EXECUTION_ERROR("Illegal division by zero");

  reset_test_case_env();
  APPEND_CONSTANT_INSTRUCTIONS(value_make_number(5), value_make_number(-0));
  APPEND_INSTRUCTIONS(CHUNK_OP_DIVIDE, CHUNK_OP_RETURN);
  EXECUTE_ASSERT_FAILURE();
  ASSERT_EXECUTION_ERROR("Illegal division by zero");

  // invalid operand types
  ASSERT_INVALID_BINARY_NUMERIC_OPERATOR_OPERAND_TYPES(CHUNK_OP_DIVIDE, "division");

#undef ASSERT_CHUNK_OP_DIVIDE
}

static void test_CHUNK_OP_MODULO(void **const _) {
#define ASSERT_CHUNK_OP_MODULO(number_a, number_b, expected_number_c)                                               \
  ASSERT_VALUE_IS_RESULT_OF_INSTRUCTION_ON_VALUES(                                                                  \
    value_make_number(expected_number_c), CHUNK_OP_MODULO, value_make_number(number_a), value_make_number(number_b) \
  )

  // integer modulo and non-commutativity
  ASSERT_CHUNK_OP_MODULO(8, 2, 0);
  ASSERT_CHUNK_OP_MODULO(2, 8, 2);

  // modulo properties
  ASSERT_CHUNK_OP_MODULO(25, 25, 0);
  ASSERT_CHUNK_OP_MODULO(25, 1, 0);

  // floating-point modulo
  ASSERT_CHUNK_OP_MODULO(4.68, 3.23, 1.45);

  // signed integer modulo
  ASSERT_CHUNK_OP_MODULO(-5, 2, -1);
  ASSERT_CHUNK_OP_MODULO(5, -2, 1);
  ASSERT_CHUNK_OP_MODULO(-5, -2, -1);

  // signed zero modulo
  ASSERT_CHUNK_OP_MODULO(0, 2, 0);
  ASSERT_CHUNK_OP_MODULO(0, -2, 0);
  ASSERT_CHUNK_OP_MODULO(-0, 2, -0);
  ASSERT_CHUNK_OP_MODULO(-0, -2, -0);

  // modulo by zero
  reset_test_case_env();
  APPEND_CONSTANT_INSTRUCTIONS(value_make_number(5), value_make_number(0));
  APPEND_INSTRUCTIONS(CHUNK_OP_MODULO, CHUNK_OP_RETURN);
  EXECUTE_ASSERT_FAILURE();
  ASSERT_EXECUTION_ERROR("Illegal modulo by zero");

  reset_test_case_env();
  APPEND_CONSTANT_INSTRUCTIONS(value_make_number(5), value_make_number(-0));
  APPEND_INSTRUCTIONS(CHUNK_OP_MODULO, CHUNK_OP_RETURN);
  EXECUTE_ASSERT_FAILURE();
  ASSERT_EXECUTION_ERROR("Illegal modulo by zero");

  // invalid operand types
  ASSERT_INVALID_BINARY_NUMERIC_OPERATOR_OPERAND_TYPES(CHUNK_OP_MODULO, "modulo");

#undef ASSERT_CHUNK_OP_MODULO
}

static void test_CHUNK_OP_NOT(void **const _) {
#define ASSERT_CHUNK_OP_NOT(value_a, expected_bool_b) \
  ASSERT_VALUE_IS_RESULT_OF_INSTRUCTION_ON_VALUES(value_make_bool(expected_bool_b), CHUNK_OP_NOT, value_a)

  // truthy values
  ASSERT_CHUNK_OP_NOT(value_make_number(1), false);
  ASSERT_CHUNK_OP_NOT(value_make_number(-1), false);
  ASSERT_CHUNK_OP_NOT(value_make_number(0), false);
  ASSERT_CHUNK_OP_NOT(value_make_bool(true), false);

  // falsy values
  ASSERT_CHUNK_OP_NOT(value_make_bool(false), true);
  ASSERT_CHUNK_OP_NOT(value_make_nil(), true);

  // "not" stacking
  reset_test_case_env();
  APPEND_CONSTANT_INSTRUCTION(value_make_number(2));
  APPEND_INSTRUCTIONS(CHUNK_OP_NOT, CHUNK_OP_NOT, CHUNK_OP_RETURN);
  EXECUTE_ASSERT_SUCCESS();
  STACK_POP_ASSERT(value_make_bool(true));
  ASSERT_EMPTY_STACK();

  reset_test_case_env();
  APPEND_CONSTANT_INSTRUCTION(value_make_number(3));
  APPEND_INSTRUCTIONS(CHUNK_OP_NOT, CHUNK_OP_NOT, CHUNK_OP_NOT, CHUNK_OP_RETURN);
  EXECUTE_ASSERT_SUCCESS();
  STACK_POP_ASSERT(value_make_bool(false));
  ASSERT_EMPTY_STACK();

#undef ASSERT_CHUNK_OP_NOT
}

static void test_CHUNK_OP_EQUAL(void **const _) {
#define ASSERT_CHUNK_OP_EQUAL(value_a, value_b, expected_bool_c) \
  ASSERT_VALUE_IS_RESULT_OF_INSTRUCTION_ON_VALUES(value_make_bool(expected_bool_c), CHUNK_OP_EQUAL, value_a, value_b)

  // equal values
  ASSERT_CHUNK_OP_EQUAL(value_make_number(1), value_make_number(1), true);
  ASSERT_CHUNK_OP_EQUAL(value_make_bool(true), value_make_bool(true), true);
  ASSERT_CHUNK_OP_EQUAL(value_make_nil(), value_make_nil(), true);

  // unequal values
  ASSERT_CHUNK_OP_EQUAL(value_make_number(0), value_make_number(1), false);
  ASSERT_CHUNK_OP_EQUAL(value_make_number(0), value_make_bool(true), false);
  ASSERT_CHUNK_OP_EQUAL(value_make_number(0), value_make_nil(), false);
  ASSERT_CHUNK_OP_EQUAL(value_make_bool(true), value_make_bool(false), false);
  ASSERT_CHUNK_OP_EQUAL(value_make_bool(false), value_make_nil(), false);

#undef ASSERT_CHUNK_OP_EQUAL
}

static void test_CHUNK_OP_NOT_EQUAL(void **const _) {
#define ASSERT_CHUNK_OP_NOT_EQUAL(value_a, value_b, expected_bool_c)       \
  ASSERT_VALUE_IS_RESULT_OF_INSTRUCTION_ON_VALUES(                         \
    value_make_bool(expected_bool_c), CHUNK_OP_NOT_EQUAL, value_a, value_b \
  )

  // equal values
  ASSERT_CHUNK_OP_NOT_EQUAL(value_make_number(1), value_make_number(1), false);
  ASSERT_CHUNK_OP_NOT_EQUAL(value_make_bool(true), value_make_bool(true), false);
  ASSERT_CHUNK_OP_NOT_EQUAL(value_make_nil(), value_make_nil(), false);

  // unequal values
  ASSERT_CHUNK_OP_NOT_EQUAL(value_make_number(0), value_make_number(1), true);
  ASSERT_CHUNK_OP_NOT_EQUAL(value_make_number(0), value_make_bool(true), true);
  ASSERT_CHUNK_OP_NOT_EQUAL(value_make_number(0), value_make_nil(), true);
  ASSERT_CHUNK_OP_NOT_EQUAL(value_make_bool(true), value_make_bool(false), true);
  ASSERT_CHUNK_OP_NOT_EQUAL(value_make_bool(false), value_make_nil(), true);

#undef ASSERT_CHUNK_OP_NOT_EQUAL
}

static void test_CHUNK_OP_LESS(void **const _) {
#define ASSERT_CHUNK_OP_LESS(number_a, number_b, expected_bool_c)                                             \
  ASSERT_VALUE_IS_RESULT_OF_INSTRUCTION_ON_VALUES(                                                            \
    value_make_bool(expected_bool_c), CHUNK_OP_LESS, value_make_number(number_a), value_make_number(number_b) \
  )

  // ascending numbers
  ASSERT_CHUNK_OP_LESS(-5, 0, true);
  ASSERT_CHUNK_OP_LESS(0, 5, true);
  ASSERT_CHUNK_OP_LESS(5, 10, true);

  // descending numbers
  ASSERT_CHUNK_OP_LESS(0, -5, false);
  ASSERT_CHUNK_OP_LESS(5, 0, false);
  ASSERT_CHUNK_OP_LESS(10, 5, false);

  // equal numbers
  ASSERT_CHUNK_OP_LESS(-1, -1, false);
  ASSERT_CHUNK_OP_LESS(0, 0, false);
  ASSERT_CHUNK_OP_LESS(2, 2, false);

  // invalid operand types
  ASSERT_INVALID_BINARY_NUMERIC_OPERATOR_OPERAND_TYPES(CHUNK_OP_LESS, "less-than");

#undef ASSERT_CHUNK_OP_LESS
}

static void test_CHUNK_OP_LESS_EQUAL(void **const _) {
#define ASSERT_CHUNK_OP_LESS_EQUAL(number_a, number_b, expected_bool_c)                                             \
  ASSERT_VALUE_IS_RESULT_OF_INSTRUCTION_ON_VALUES(                                                                  \
    value_make_bool(expected_bool_c), CHUNK_OP_LESS_EQUAL, value_make_number(number_a), value_make_number(number_b) \
  )

  // ascending numbers
  ASSERT_CHUNK_OP_LESS_EQUAL(-5, 0, true);
  ASSERT_CHUNK_OP_LESS_EQUAL(0, 5, true);
  ASSERT_CHUNK_OP_LESS_EQUAL(5, 10, true);

  // descending numbers
  ASSERT_CHUNK_OP_LESS_EQUAL(0, -5, false);
  ASSERT_CHUNK_OP_LESS_EQUAL(5, 0, false);
  ASSERT_CHUNK_OP_LESS_EQUAL(10, 5, false);

  // equal numbers
  ASSERT_CHUNK_OP_LESS_EQUAL(-1, -1, true);
  ASSERT_CHUNK_OP_LESS_EQUAL(0, 0, true);
  ASSERT_CHUNK_OP_LESS_EQUAL(2, 2, true);

  // invalid operand types
  ASSERT_INVALID_BINARY_NUMERIC_OPERATOR_OPERAND_TYPES(CHUNK_OP_LESS_EQUAL, "less-than-or-equal");

#undef ASSERT_CHUNK_OP_LESS_EQUAL
}

static void test_CHUNK_OP_GREATER(void **const _) {
#define ASSERT_CHUNK_OP_GREATER(number_a, number_b, expected_bool_c)                                             \
  ASSERT_VALUE_IS_RESULT_OF_INSTRUCTION_ON_VALUES(                                                               \
    value_make_bool(expected_bool_c), CHUNK_OP_GREATER, value_make_number(number_a), value_make_number(number_b) \
  )

  // ascending numbers
  ASSERT_CHUNK_OP_GREATER(-5, 0, false);
  ASSERT_CHUNK_OP_GREATER(0, 5, false);
  ASSERT_CHUNK_OP_GREATER(5, 10, false);

  // descending numbers
  ASSERT_CHUNK_OP_GREATER(0, -5, true);
  ASSERT_CHUNK_OP_GREATER(5, 0, true);
  ASSERT_CHUNK_OP_GREATER(10, 5, true);

  // equal numbers
  ASSERT_CHUNK_OP_GREATER(-1, -1, false);
  ASSERT_CHUNK_OP_GREATER(0, 0, false);
  ASSERT_CHUNK_OP_GREATER(2, 2, false);

  // invalid operand types
  ASSERT_INVALID_BINARY_NUMERIC_OPERATOR_OPERAND_TYPES(CHUNK_OP_GREATER, "greater-than");

#undef ASSERT_CHUNK_OP_GREATER
}

static void test_CHUNK_OP_GREATER_EQUAL(void **const _) {
#define ASSERT_CHUNK_OP_GREATER_EQUAL(number_a, number_b, expected_bool_c)                                             \
  ASSERT_VALUE_IS_RESULT_OF_INSTRUCTION_ON_VALUES(                                                                     \
    value_make_bool(expected_bool_c), CHUNK_OP_GREATER_EQUAL, value_make_number(number_a), value_make_number(number_b) \
  )

  // ascending numbers
  ASSERT_CHUNK_OP_GREATER_EQUAL(-5, 0, false);
  ASSERT_CHUNK_OP_GREATER_EQUAL(0, 5, false);
  ASSERT_CHUNK_OP_GREATER_EQUAL(5, 10, false);

  // descending numbers
  ASSERT_CHUNK_OP_GREATER_EQUAL(0, -5, true);
  ASSERT_CHUNK_OP_GREATER_EQUAL(5, 0, true);
  ASSERT_CHUNK_OP_GREATER_EQUAL(10, 5, true);

  // equal numbers
  ASSERT_CHUNK_OP_GREATER_EQUAL(-1, -1, true);
  ASSERT_CHUNK_OP_GREATER_EQUAL(0, 0, true);
  ASSERT_CHUNK_OP_GREATER_EQUAL(2, 2, true);

  // invalid operand types
  ASSERT_INVALID_BINARY_NUMERIC_OPERATOR_OPERAND_TYPES(CHUNK_OP_GREATER_EQUAL, "greater-than-or-equal");

#undef ASSERT_CHUNK_OP_GREATER_EQUAL
}

int main(void) {
  // CHUNK_OP_RETURN test is missing as it's not yet properly implemented

  struct CMUnitTest const tests[] = {
    cmocka_unit_test_setup_teardown(test_CHUNK_OP_CONSTANT, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_CHUNK_OP_CONSTANT_2B, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_CHUNK_OP_NIL, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_CHUNK_OP_TRUE, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_CHUNK_OP_FALSE, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_CHUNK_OP_PRINT, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_CHUNK_OP_POP, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_CHUNK_OP_NEGATE, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_CHUNK_OP_ADD, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_CHUNK_OP_SUBTRACT, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_CHUNK_OP_MULTIPLY, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_CHUNK_OP_DIVIDE, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_CHUNK_OP_MODULO, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_CHUNK_OP_NOT, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_CHUNK_OP_EQUAL, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_CHUNK_OP_NOT_EQUAL, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_CHUNK_OP_LESS, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_CHUNK_OP_LESS_EQUAL, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_CHUNK_OP_GREATER, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_CHUNK_OP_GREATER_EQUAL, setup_test_case_env, teardown_test_case_env),
  };

  return cmocka_run_group_tests(tests, setup_test_group_env, teardown_test_group_env);
}
