#include "backend/vm.h"

#include "backend/chunk.h"
#include "backend/value.h"
#include "component/component_test.h"
#include "global.h"
#include "utils/error.h"

#include <stdio.h>

// *---------------------------------------------*
// *               STATIC OBJECTS                *
// *---------------------------------------------*

static Chunk chunk;

// *---------------------------------------------*
// *                  UTILITIES                  *
// *---------------------------------------------*

static bool run(void) {
  component_test_clear_binary_stream_resource_content(g_execution_error_stream);
  component_test_clear_binary_stream_resource_content(g_runtime_output_stream);

  return vm_run(&chunk);
}

static void reset_test_case_env(void) {
  vm_reset();
  chunk_reset(&chunk);
}

#define run_assert_success() assert_true(run())
#define run_assert_failure() assert_false(run())

#define assert_empty_stack() assert_int_equal(*t_vm_stack_count, 0)

#define stack_pop_assert(expected_value) component_test_assert_value_equality(vm_stack_pop(), expected_value)
#define stack_pop_assert_many(...) COMPONENT_TEST_APPLY_TO_EACH_ARG(stack_pop_assert, Value, __VA_ARGS__)

#define append_constant_instruction(constant) chunk_append_constant_instruction(&chunk, constant, 1)
#define append_constant_instructions(...) \
  COMPONENT_TEST_APPLY_TO_EACH_ARG(append_constant_instruction, Value, __VA_ARGS__)

#define append_instruction(opcode) chunk_append_instruction(&chunk, opcode, 1)
#define append_instructions(...) COMPONENT_TEST_APPLY_TO_EACH_ARG(append_instruction, ChunkOpCode, __VA_ARGS__)

#define assert_execution_error(expected_error_message)                                         \
  component_test_assert_binary_stream_resource_content(                                        \
    g_execution_error_stream,                                                                  \
    "[EXECUTION_ERROR]" COMMON_MS __FILE__ COMMON_PS "1" COMMON_MS expected_error_message "\n" \
  )

#define assert_runtime_output(expected_output) \
  component_test_assert_binary_stream_resource_content(g_runtime_output_stream, expected_output "\n")

#define assert_invalid_binary_numeric_operator_operand_types(operator_instruction, operator_descriptor)          \
  do {                                                                                                           \
    reset_test_case_env();                                                                                       \
    append_instructions(CHUNK_OP_NIL, CHUNK_OP_NIL, operator_instruction, CHUNK_OP_RETURN);                      \
    run_assert_failure();                                                                                        \
    assert_execution_error("Expected " operator_descriptor " operands to be numbers (got 'nil' and 'nil')");     \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    append_instruction(CHUNK_OP_NIL);                                                                            \
    append_constant_instruction(VALUE_MAKE_NUMBER(1));                                                           \
    append_instructions(operator_instruction, CHUNK_OP_RETURN);                                                  \
    run_assert_failure();                                                                                        \
    assert_execution_error("Expected " operator_descriptor " operands to be numbers (got 'nil' and 'number')");  \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    append_constant_instruction(VALUE_MAKE_NUMBER(1));                                                           \
    append_instructions(CHUNK_OP_NIL, operator_instruction, CHUNK_OP_RETURN);                                    \
    run_assert_failure();                                                                                        \
    assert_execution_error("Expected " operator_descriptor " operands to be numbers (got 'number' and 'nil')");  \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    append_instructions(CHUNK_OP_TRUE, CHUNK_OP_FALSE, operator_instruction, CHUNK_OP_RETURN);                   \
    run_assert_failure();                                                                                        \
    assert_execution_error("Expected " operator_descriptor " operands to be numbers (got 'bool' and 'bool')");   \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    append_instruction(CHUNK_OP_TRUE);                                                                           \
    append_constant_instruction(VALUE_MAKE_NUMBER(1));                                                           \
    append_instructions(operator_instruction, CHUNK_OP_RETURN);                                                  \
    run_assert_failure();                                                                                        \
    assert_execution_error("Expected " operator_descriptor " operands to be numbers (got 'bool' and 'number')"); \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    append_constant_instruction(VALUE_MAKE_NUMBER(1));                                                           \
    append_instructions(CHUNK_OP_FALSE, operator_instruction, CHUNK_OP_RETURN);                                  \
    run_assert_failure();                                                                                        \
    assert_execution_error("Expected " operator_descriptor " operands to be numbers (got 'number' and 'bool')"); \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    append_instructions(CHUNK_OP_NIL, CHUNK_OP_TRUE, operator_instruction, CHUNK_OP_RETURN);                     \
    run_assert_failure();                                                                                        \
    assert_execution_error("Expected " operator_descriptor " operands to be numbers (got 'nil' and 'bool')");    \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    append_instructions(CHUNK_OP_FALSE, CHUNK_OP_NIL, operator_instruction, CHUNK_OP_RETURN);                    \
    run_assert_failure();                                                                                        \
    assert_execution_error("Expected " operator_descriptor " operands to be numbers (got 'bool' and 'nil')");    \
  } while (0)

#define assert_value_is_result_of_instruction_on_values(expected_value, instruction, ...) \
  do {                                                                                    \
    reset_test_case_env();                                                                \
    append_constant_instructions(__VA_ARGS__);                                            \
    append_instructions(instruction, CHUNK_OP_RETURN);                                    \
    run_assert_success();                                                                 \
    stack_pop_assert(expected_value);                                                     \
    assert_empty_stack();                                                                 \
  } while (0)

// *---------------------------------------------*
// *                  FIXTURES                   *
// *---------------------------------------------*

static int setup_test_group_env(void **const _) {
  g_source_file = __FILE__;

  g_execution_error_stream = tmpfile();
  if (g_execution_error_stream == NULL) ERROR_IO("%s", strerror(errno));

  g_runtime_output_stream = tmpfile();
  if (g_runtime_output_stream == NULL) ERROR_IO("%s", strerror(errno));

  return 0;
}

static int teardown_test_group_env(void **const _) {
  if (fclose(g_execution_error_stream)) ERROR_IO("%s", strerror(errno));
  if (fclose(g_runtime_output_stream)) ERROR_IO("%s", strerror(errno));

  return 0;
}

static int setup_test_case_env(void **const _) {
  vm_init();
  chunk_init(&chunk);

  return 0;
}

static int teardown_test_case_env(void **const _) {
  vm_free();
  chunk_free(&chunk);

  return 0;
}

// *---------------------------------------------*
// *                 TEST CASES                  *
// *---------------------------------------------*
static_assert(CHUNK_OP_OPCODE_COUNT == 21, "Exhaustive ChunkOpCode handling");

static void test_CHUNK_OP_CONSTANT(void **const _) {
  append_constant_instructions(VALUE_MAKE_NUMBER(1), VALUE_MAKE_NUMBER(2), VALUE_MAKE_NUMBER(3));
  append_instruction(CHUNK_OP_RETURN);
  run_assert_success();
  stack_pop_assert_many(VALUE_MAKE_NUMBER(3), VALUE_MAKE_NUMBER(2), VALUE_MAKE_NUMBER(1));
  assert_empty_stack();
}

static void test_CHUNK_OP_CONSTANT_2B(void **const _) {
  // force CHUNK_OP_CONSTANT_2B usage
  for (int i = 0; i < UCHAR_MAX; i++) value_array_append(&chunk.constants, VALUE_MAKE_NUMBER(i));

  append_constant_instructions(VALUE_MAKE_NUMBER(1), VALUE_MAKE_NUMBER(2), VALUE_MAKE_NUMBER(3));
  append_instruction(CHUNK_OP_RETURN);
  run_assert_success();
  stack_pop_assert_many(VALUE_MAKE_NUMBER(3), VALUE_MAKE_NUMBER(2), VALUE_MAKE_NUMBER(1));
  assert_empty_stack();
}

static void test_CHUNK_OP_NIL(void **const _) {
  append_instructions(CHUNK_OP_NIL, CHUNK_OP_RETURN);
  run_assert_success();
  stack_pop_assert(VALUE_MAKE_NIL());
  assert_empty_stack();
}

static void test_CHUNK_OP_TRUE(void **const _) {
  append_instructions(CHUNK_OP_TRUE, CHUNK_OP_RETURN);
  run_assert_success();
  stack_pop_assert(VALUE_MAKE_BOOL(true));
  assert_empty_stack();
}

static void test_CHUNK_OP_FALSE(void **const _) {
  append_instructions(CHUNK_OP_FALSE, CHUNK_OP_RETURN);
  run_assert_success();
  stack_pop_assert(VALUE_MAKE_BOOL(false));
  assert_empty_stack();
}

static void test_CHUNK_OP_PRINT(void **const _) {
  append_constant_instruction(VALUE_MAKE_NUMBER(1));
  append_instructions(CHUNK_OP_PRINT, CHUNK_OP_RETURN);
  run_assert_success();
  assert_runtime_output("1");
  assert_empty_stack();
}

static void test_CHUNK_OP_POP(void **const _) {
  append_constant_instructions(VALUE_MAKE_NUMBER(1), VALUE_MAKE_NUMBER(2));
  append_instructions(CHUNK_OP_POP, CHUNK_OP_RETURN);
  run_assert_success();
  stack_pop_assert(VALUE_MAKE_NUMBER(1));
  assert_empty_stack();
}

static void test_CHUNK_OP_NEGATE(void **const _) {
#define assert_number_constant_a_CHUNK_OP_NEGATE_equal_b(number_a, expected_number_b)  \
  assert_value_is_result_of_instruction_on_values(                                     \
    VALUE_MAKE_NUMBER(expected_number_b), CHUNK_OP_NEGATE, VALUE_MAKE_NUMBER(number_a) \
  )

  // signed integer negation
  assert_number_constant_a_CHUNK_OP_NEGATE_equal_b(1, -1);
  assert_number_constant_a_CHUNK_OP_NEGATE_equal_b(-2, 2);

  // floating-point negation
  assert_number_constant_a_CHUNK_OP_NEGATE_equal_b(1.25, -1.25);

  // signed zero negation
  assert_number_constant_a_CHUNK_OP_NEGATE_equal_b(0, -0);
  assert_number_constant_a_CHUNK_OP_NEGATE_equal_b(-0, 0);

  // negation stacking
  reset_test_case_env();
  append_constant_instruction(VALUE_MAKE_NUMBER(2));
  append_instructions(CHUNK_OP_NEGATE, CHUNK_OP_NEGATE, CHUNK_OP_RETURN);
  run_assert_success();
  stack_pop_assert(VALUE_MAKE_NUMBER(2));
  assert_empty_stack();

  reset_test_case_env();
  append_constant_instruction(VALUE_MAKE_NUMBER(3));
  append_instructions(CHUNK_OP_NEGATE, CHUNK_OP_NEGATE, CHUNK_OP_NEGATE, CHUNK_OP_RETURN);
  run_assert_success();
  stack_pop_assert(VALUE_MAKE_NUMBER(-3));
  assert_empty_stack();

  // invalid operand types
  reset_test_case_env();
  append_instructions(CHUNK_OP_NIL, CHUNK_OP_NEGATE, CHUNK_OP_RETURN);
  run_assert_failure();
  assert_execution_error("Expected negation operand to be a number (got 'nil')");

  reset_test_case_env();
  append_instructions(CHUNK_OP_TRUE, CHUNK_OP_NEGATE, CHUNK_OP_RETURN);
  run_assert_failure();
  assert_execution_error("Expected negation operand to be a number (got 'bool')");

  reset_test_case_env();
  append_instructions(CHUNK_OP_FALSE, CHUNK_OP_NEGATE, CHUNK_OP_RETURN);
  run_assert_failure();
  assert_execution_error("Expected negation operand to be a number (got 'bool')");

#undef assert_number_constant_a_CHUNK_OP_NEGATE_equal_b
}

static void test_CHUNK_OP_ADD(void **const _) {
#define assert_number_constants_a_b_CHUNK_OP_ADD_equal_c(number_a, number_b, expected_number_c)                  \
  assert_value_is_result_of_instruction_on_values(                                                               \
    VALUE_MAKE_NUMBER(expected_number_c), CHUNK_OP_ADD, VALUE_MAKE_NUMBER(number_a), VALUE_MAKE_NUMBER(number_b) \
  )

  // integer addition and commutativity
  assert_number_constants_a_b_CHUNK_OP_ADD_equal_c(1, 2, 3);
  assert_number_constants_a_b_CHUNK_OP_ADD_equal_c(2, 1, 3);

  // addition properties
  assert_number_constants_a_b_CHUNK_OP_ADD_equal_c(2, 0, 2);
  assert_number_constants_a_b_CHUNK_OP_ADD_equal_c(2, -2, 0);

  // floating-point addition
  assert_number_constants_a_b_CHUNK_OP_ADD_equal_c(1.7, 2.25, 3.95);

  // signed integer addition
  assert_number_constants_a_b_CHUNK_OP_ADD_equal_c(5, -7, -2);
  assert_number_constants_a_b_CHUNK_OP_ADD_equal_c(-5, 7, 2);
  assert_number_constants_a_b_CHUNK_OP_ADD_equal_c(-5, -7, -12);

  // signed zero addition
  assert_number_constants_a_b_CHUNK_OP_ADD_equal_c(0, 0, 0);
  assert_number_constants_a_b_CHUNK_OP_ADD_equal_c(0, -0, 0);
  assert_number_constants_a_b_CHUNK_OP_ADD_equal_c(-0, 0, 0);
  assert_number_constants_a_b_CHUNK_OP_ADD_equal_c(-0, -0, -0);

  // invalid operand types
  assert_invalid_binary_numeric_operator_operand_types(CHUNK_OP_ADD, "addition");

#undef assert_number_constants_a_b_CHUNK_OP_ADD_equal_c
}

static void test_CHUNK_OP_SUBTRACT(void **const _) {
#define assert_number_constants_a_b_CHUNK_OP_SUBTRACT_equal_c(number_a, number_b, expected_number_c)                  \
  assert_value_is_result_of_instruction_on_values(                                                                    \
    VALUE_MAKE_NUMBER(expected_number_c), CHUNK_OP_SUBTRACT, VALUE_MAKE_NUMBER(number_a), VALUE_MAKE_NUMBER(number_b) \
  )

  // integer subtraction and non-commutativity
  assert_number_constants_a_b_CHUNK_OP_SUBTRACT_equal_c(4, 3, 1);
  assert_number_constants_a_b_CHUNK_OP_SUBTRACT_equal_c(3, 4, -1);

  // subtraction properties
  assert_number_constants_a_b_CHUNK_OP_SUBTRACT_equal_c(2, 0, 2);
  assert_number_constants_a_b_CHUNK_OP_SUBTRACT_equal_c(25, 25, 0);

  // floating-point subtraction
  assert_number_constants_a_b_CHUNK_OP_SUBTRACT_equal_c(3.75, 2.45, 1.3);

  // signed integer subtraction
  assert_number_constants_a_b_CHUNK_OP_SUBTRACT_equal_c(-4, 3, -7);
  assert_number_constants_a_b_CHUNK_OP_SUBTRACT_equal_c(4, -3, 7);
  assert_number_constants_a_b_CHUNK_OP_SUBTRACT_equal_c(-4, -3, -1);

  // signed zero subtraction
  assert_number_constants_a_b_CHUNK_OP_SUBTRACT_equal_c(0, 0, 0);
  assert_number_constants_a_b_CHUNK_OP_SUBTRACT_equal_c(0, -0, 0);
  assert_number_constants_a_b_CHUNK_OP_SUBTRACT_equal_c(-0, 0, -0);
  assert_number_constants_a_b_CHUNK_OP_SUBTRACT_equal_c(-0, -0, 0);

  // invalid operand types
  assert_invalid_binary_numeric_operator_operand_types(CHUNK_OP_SUBTRACT, "subtraction");

#undef assert_number_constants_a_b_CHUNK_OP_SUBTRACT_equal_c
}

static void test_CHUNK_OP_MULTIPLY(void **const _) {
#define assert_number_constants_a_b_CHUNK_OP_MULTIPLY_equal_c(number_a, number_b, expected_number_c)                  \
  assert_value_is_result_of_instruction_on_values(                                                                    \
    VALUE_MAKE_NUMBER(expected_number_c), CHUNK_OP_MULTIPLY, VALUE_MAKE_NUMBER(number_a), VALUE_MAKE_NUMBER(number_b) \
  )

  // integer multiplication and commutativity
  assert_number_constants_a_b_CHUNK_OP_MULTIPLY_equal_c(5, 3, 15);
  assert_number_constants_a_b_CHUNK_OP_MULTIPLY_equal_c(3, 5, 15);

  // multiplication properties
  assert_number_constants_a_b_CHUNK_OP_MULTIPLY_equal_c(125, 1, 125);
  assert_number_constants_a_b_CHUNK_OP_MULTIPLY_equal_c(50, 0, 0);

  // floating-point multiplication
  assert_number_constants_a_b_CHUNK_OP_MULTIPLY_equal_c(12.34, 0.3, 3.702);

  // signed integer multiplication
  assert_number_constants_a_b_CHUNK_OP_MULTIPLY_equal_c(-2, 4, -8);
  assert_number_constants_a_b_CHUNK_OP_MULTIPLY_equal_c(2, -4, -8);
  assert_number_constants_a_b_CHUNK_OP_MULTIPLY_equal_c(-2, -4, 8);

  // signed zero multiplication
  assert_number_constants_a_b_CHUNK_OP_MULTIPLY_equal_c(0, 0, 0);
  assert_number_constants_a_b_CHUNK_OP_MULTIPLY_equal_c(0, -0, -0);
  assert_number_constants_a_b_CHUNK_OP_MULTIPLY_equal_c(-0, 0, -0);
  assert_number_constants_a_b_CHUNK_OP_MULTIPLY_equal_c(-0, -0, 0);

  // invalid operand types
  assert_invalid_binary_numeric_operator_operand_types(CHUNK_OP_MULTIPLY, "multiplication");

#undef assert_number_constants_a_b_CHUNK_OP_MULTIPLY_equal_c
}

static void test_CHUNK_OP_DIVIDE(void **const _) {
#define assert_number_constants_a_b_CHUNK_OP_DIVIDE_equal_c(number_a, number_b, expected_number_c)                  \
  assert_value_is_result_of_instruction_on_values(                                                                  \
    VALUE_MAKE_NUMBER(expected_number_c), CHUNK_OP_DIVIDE, VALUE_MAKE_NUMBER(number_a), VALUE_MAKE_NUMBER(number_b) \
  )

  // integer division and non-commutativity
  assert_number_constants_a_b_CHUNK_OP_DIVIDE_equal_c(8, 2, 4);
  assert_number_constants_a_b_CHUNK_OP_DIVIDE_equal_c(2, 8, 0.25);

  // division properties
  assert_number_constants_a_b_CHUNK_OP_DIVIDE_equal_c(4, 1, 4);
  assert_number_constants_a_b_CHUNK_OP_DIVIDE_equal_c(25, 25, 1);

  // floating-point division
  assert_number_constants_a_b_CHUNK_OP_DIVIDE_equal_c(4.2, 1.5, 2.8);

  // signed integer division
  assert_number_constants_a_b_CHUNK_OP_DIVIDE_equal_c(-5, 2, -2.5);
  assert_number_constants_a_b_CHUNK_OP_DIVIDE_equal_c(5, -2, -2.5);
  assert_number_constants_a_b_CHUNK_OP_DIVIDE_equal_c(-5, -2, 2.5);

  // signed zero division
  assert_number_constants_a_b_CHUNK_OP_DIVIDE_equal_c(0, 2, 0);
  assert_number_constants_a_b_CHUNK_OP_DIVIDE_equal_c(0, -2, -0);
  assert_number_constants_a_b_CHUNK_OP_DIVIDE_equal_c(-0, 2, -0);
  assert_number_constants_a_b_CHUNK_OP_DIVIDE_equal_c(-0, -2, 0);

  // division by zero
  reset_test_case_env();
  append_constant_instructions(VALUE_MAKE_NUMBER(5), VALUE_MAKE_NUMBER(0));
  append_instructions(CHUNK_OP_DIVIDE, CHUNK_OP_RETURN);
  run_assert_failure();
  assert_execution_error("Illegal division by zero");

  reset_test_case_env();
  append_constant_instructions(VALUE_MAKE_NUMBER(5), VALUE_MAKE_NUMBER(-0));
  append_instructions(CHUNK_OP_DIVIDE, CHUNK_OP_RETURN);
  run_assert_failure();
  assert_execution_error("Illegal division by zero");

  // invalid operand types
  assert_invalid_binary_numeric_operator_operand_types(CHUNK_OP_DIVIDE, "division");

#undef assert_number_constants_a_b_CHUNK_OP_DIVIDE_equal_c
}

static void test_CHUNK_OP_MODULO(void **const _) {
#define assert_number_constants_a_b_CHUNK_OP_MODULO_equal_c(number_a, number_b, expected_number_c)                  \
  assert_value_is_result_of_instruction_on_values(                                                                  \
    VALUE_MAKE_NUMBER(expected_number_c), CHUNK_OP_MODULO, VALUE_MAKE_NUMBER(number_a), VALUE_MAKE_NUMBER(number_b) \
  )

  // integer modulo and non-commutativity
  assert_number_constants_a_b_CHUNK_OP_MODULO_equal_c(8, 2, 0);
  assert_number_constants_a_b_CHUNK_OP_MODULO_equal_c(2, 8, 2);

  // modulo properties
  assert_number_constants_a_b_CHUNK_OP_MODULO_equal_c(25, 25, 0);
  assert_number_constants_a_b_CHUNK_OP_MODULO_equal_c(25, 1, 0);

  // floating-point modulo
  assert_number_constants_a_b_CHUNK_OP_MODULO_equal_c(4.68, 3.23, 1.45);

  // signed integer modulo
  assert_number_constants_a_b_CHUNK_OP_MODULO_equal_c(-5, 2, -1);
  assert_number_constants_a_b_CHUNK_OP_MODULO_equal_c(5, -2, 1);
  assert_number_constants_a_b_CHUNK_OP_MODULO_equal_c(-5, -2, -1);

  // signed zero modulo
  assert_number_constants_a_b_CHUNK_OP_MODULO_equal_c(0, 2, 0);
  assert_number_constants_a_b_CHUNK_OP_MODULO_equal_c(0, -2, 0);
  assert_number_constants_a_b_CHUNK_OP_MODULO_equal_c(-0, 2, -0);
  assert_number_constants_a_b_CHUNK_OP_MODULO_equal_c(-0, -2, -0);

  // modulo by zero
  reset_test_case_env();
  append_constant_instructions(VALUE_MAKE_NUMBER(5), VALUE_MAKE_NUMBER(0));
  append_instructions(CHUNK_OP_MODULO, CHUNK_OP_RETURN);
  run_assert_failure();
  assert_execution_error("Illegal modulo by zero");

  reset_test_case_env();
  append_constant_instructions(VALUE_MAKE_NUMBER(5), VALUE_MAKE_NUMBER(-0));
  append_instructions(CHUNK_OP_MODULO, CHUNK_OP_RETURN);
  run_assert_failure();
  assert_execution_error("Illegal modulo by zero");

  // invalid operand types
  assert_invalid_binary_numeric_operator_operand_types(CHUNK_OP_MODULO, "modulo");

#undef assert_number_constants_a_b_CHUNK_OP_MODULO_equal_c
}

static void test_CHUNK_OP_NOT(void **const _) {
#define assert_value_a_CHUNK_OP_NOT_equals_b(value_a, expected_bool_b) \
  assert_value_is_result_of_instruction_on_values(VALUE_MAKE_BOOL(expected_bool_b), CHUNK_OP_NOT, value_a)

  // truthy values
  assert_value_a_CHUNK_OP_NOT_equals_b(VALUE_MAKE_NUMBER(1), false);
  assert_value_a_CHUNK_OP_NOT_equals_b(VALUE_MAKE_NUMBER(-1), false);
  assert_value_a_CHUNK_OP_NOT_equals_b(VALUE_MAKE_NUMBER(0), false);
  assert_value_a_CHUNK_OP_NOT_equals_b(VALUE_MAKE_BOOL(true), false);

  // falsy values
  assert_value_a_CHUNK_OP_NOT_equals_b(VALUE_MAKE_BOOL(false), true);
  assert_value_a_CHUNK_OP_NOT_equals_b(VALUE_MAKE_NIL(), true);

  // "not" stacking
  reset_test_case_env();
  append_constant_instruction(VALUE_MAKE_NUMBER(2));
  append_instructions(CHUNK_OP_NOT, CHUNK_OP_NOT, CHUNK_OP_RETURN);
  run_assert_success();
  stack_pop_assert(VALUE_MAKE_BOOL(true));
  assert_empty_stack();

  reset_test_case_env();
  append_constant_instruction(VALUE_MAKE_NUMBER(3));
  append_instructions(CHUNK_OP_NOT, CHUNK_OP_NOT, CHUNK_OP_NOT, CHUNK_OP_RETURN);
  run_assert_success();
  stack_pop_assert(VALUE_MAKE_BOOL(false));
  assert_empty_stack();

#undef assert_value_a_CHUNK_OP_NOT_equals_b
}

static void test_CHUNK_OP_EQUAL(void **const _) {
#define assert_values_a_b_CHUNK_OP_EQUAL_equal_c(value_a, value_b, expected_bool_c) \
  assert_value_is_result_of_instruction_on_values(VALUE_MAKE_BOOL(expected_bool_c), CHUNK_OP_EQUAL, value_a, value_b)

  // equal values
  assert_values_a_b_CHUNK_OP_EQUAL_equal_c(VALUE_MAKE_NUMBER(1), VALUE_MAKE_NUMBER(1), true);
  assert_values_a_b_CHUNK_OP_EQUAL_equal_c(VALUE_MAKE_BOOL(true), VALUE_MAKE_BOOL(true), true);
  assert_values_a_b_CHUNK_OP_EQUAL_equal_c(VALUE_MAKE_NIL(), VALUE_MAKE_NIL(), true);

  // unequal values
  assert_values_a_b_CHUNK_OP_EQUAL_equal_c(VALUE_MAKE_NUMBER(0), VALUE_MAKE_NUMBER(1), false);
  assert_values_a_b_CHUNK_OP_EQUAL_equal_c(VALUE_MAKE_NUMBER(0), VALUE_MAKE_BOOL(true), false);
  assert_values_a_b_CHUNK_OP_EQUAL_equal_c(VALUE_MAKE_NUMBER(0), VALUE_MAKE_NIL(), false);
  assert_values_a_b_CHUNK_OP_EQUAL_equal_c(VALUE_MAKE_BOOL(true), VALUE_MAKE_BOOL(false), false);
  assert_values_a_b_CHUNK_OP_EQUAL_equal_c(VALUE_MAKE_BOOL(false), VALUE_MAKE_NIL(), false);

#undef assert_values_a_b_CHUNK_OP_EQUAL_equal_c
}

static void test_CHUNK_OP_NOT_EQUAL(void **const _) {
#define assert_values_a_b_CHUNK_OP_NOT_EQUAL_equal_c(value_a, value_b, expected_bool_c) \
  assert_value_is_result_of_instruction_on_values(                                      \
    VALUE_MAKE_BOOL(expected_bool_c), CHUNK_OP_NOT_EQUAL, value_a, value_b              \
  )

  // equal values
  assert_values_a_b_CHUNK_OP_NOT_EQUAL_equal_c(VALUE_MAKE_NUMBER(1), VALUE_MAKE_NUMBER(1), false);
  assert_values_a_b_CHUNK_OP_NOT_EQUAL_equal_c(VALUE_MAKE_BOOL(true), VALUE_MAKE_BOOL(true), false);
  assert_values_a_b_CHUNK_OP_NOT_EQUAL_equal_c(VALUE_MAKE_NIL(), VALUE_MAKE_NIL(), false);

  // unequal values
  assert_values_a_b_CHUNK_OP_NOT_EQUAL_equal_c(VALUE_MAKE_NUMBER(0), VALUE_MAKE_NUMBER(1), true);
  assert_values_a_b_CHUNK_OP_NOT_EQUAL_equal_c(VALUE_MAKE_NUMBER(0), VALUE_MAKE_BOOL(true), true);
  assert_values_a_b_CHUNK_OP_NOT_EQUAL_equal_c(VALUE_MAKE_NUMBER(0), VALUE_MAKE_NIL(), true);
  assert_values_a_b_CHUNK_OP_NOT_EQUAL_equal_c(VALUE_MAKE_BOOL(true), VALUE_MAKE_BOOL(false), true);
  assert_values_a_b_CHUNK_OP_NOT_EQUAL_equal_c(VALUE_MAKE_BOOL(false), VALUE_MAKE_NIL(), true);

#undef assert_values_a_b_CHUNK_OP_NOT_EQUAL_equal_c
}

static void test_CHUNK_OP_LESS(void **const _) {
#define assert_numbers_a_b_CHUNK_OP_LESS_equal_c(number_a, number_b, expected_bool_c)                         \
  assert_value_is_result_of_instruction_on_values(                                                            \
    VALUE_MAKE_BOOL(expected_bool_c), CHUNK_OP_LESS, VALUE_MAKE_NUMBER(number_a), VALUE_MAKE_NUMBER(number_b) \
  )

  // ascending numbers
  assert_numbers_a_b_CHUNK_OP_LESS_equal_c(-5, 0, true);
  assert_numbers_a_b_CHUNK_OP_LESS_equal_c(0, 5, true);
  assert_numbers_a_b_CHUNK_OP_LESS_equal_c(5, 10, true);

  // descending numbers
  assert_numbers_a_b_CHUNK_OP_LESS_equal_c(0, -5, false);
  assert_numbers_a_b_CHUNK_OP_LESS_equal_c(5, 0, false);
  assert_numbers_a_b_CHUNK_OP_LESS_equal_c(10, 5, false);

  // equal numbers
  assert_numbers_a_b_CHUNK_OP_LESS_equal_c(-1, -1, false);
  assert_numbers_a_b_CHUNK_OP_LESS_equal_c(0, 0, false);
  assert_numbers_a_b_CHUNK_OP_LESS_equal_c(2, 2, false);

  // invalid operand types
  assert_invalid_binary_numeric_operator_operand_types(CHUNK_OP_LESS, "less-than");

#undef assert_numbers_a_b_CHUNK_OP_LESS_equal_c
}

static void test_CHUNK_OP_LESS_EQUAL(void **const _) {
#define assert_numbers_a_b_CHUNK_OP_LESS_EQUAL_equal_c(number_a, number_b, expected_bool_c)                         \
  assert_value_is_result_of_instruction_on_values(                                                                  \
    VALUE_MAKE_BOOL(expected_bool_c), CHUNK_OP_LESS_EQUAL, VALUE_MAKE_NUMBER(number_a), VALUE_MAKE_NUMBER(number_b) \
  )

  // ascending numbers
  assert_numbers_a_b_CHUNK_OP_LESS_EQUAL_equal_c(-5, 0, true);
  assert_numbers_a_b_CHUNK_OP_LESS_EQUAL_equal_c(0, 5, true);
  assert_numbers_a_b_CHUNK_OP_LESS_EQUAL_equal_c(5, 10, true);

  // descending numbers
  assert_numbers_a_b_CHUNK_OP_LESS_EQUAL_equal_c(0, -5, false);
  assert_numbers_a_b_CHUNK_OP_LESS_EQUAL_equal_c(5, 0, false);
  assert_numbers_a_b_CHUNK_OP_LESS_EQUAL_equal_c(10, 5, false);

  // equal numbers
  assert_numbers_a_b_CHUNK_OP_LESS_EQUAL_equal_c(-1, -1, true);
  assert_numbers_a_b_CHUNK_OP_LESS_EQUAL_equal_c(0, 0, true);
  assert_numbers_a_b_CHUNK_OP_LESS_EQUAL_equal_c(2, 2, true);

  // invalid operand types
  assert_invalid_binary_numeric_operator_operand_types(CHUNK_OP_LESS_EQUAL, "less-than-or-equal");

#undef assert_numbers_a_b_CHUNK_OP_LESS_EQUAL_equal_c
}

static void test_CHUNK_OP_GREATER(void **const _) {
#define assert_numbers_a_b_CHUNK_OP_GREATER_equal_c(number_a, number_b, expected_bool_c)                         \
  assert_value_is_result_of_instruction_on_values(                                                               \
    VALUE_MAKE_BOOL(expected_bool_c), CHUNK_OP_GREATER, VALUE_MAKE_NUMBER(number_a), VALUE_MAKE_NUMBER(number_b) \
  )

  // ascending numbers
  assert_numbers_a_b_CHUNK_OP_GREATER_equal_c(-5, 0, false);
  assert_numbers_a_b_CHUNK_OP_GREATER_equal_c(0, 5, false);
  assert_numbers_a_b_CHUNK_OP_GREATER_equal_c(5, 10, false);

  // descending numbers
  assert_numbers_a_b_CHUNK_OP_GREATER_equal_c(0, -5, true);
  assert_numbers_a_b_CHUNK_OP_GREATER_equal_c(5, 0, true);
  assert_numbers_a_b_CHUNK_OP_GREATER_equal_c(10, 5, true);

  // equal numbers
  assert_numbers_a_b_CHUNK_OP_GREATER_equal_c(-1, -1, false);
  assert_numbers_a_b_CHUNK_OP_GREATER_equal_c(0, 0, false);
  assert_numbers_a_b_CHUNK_OP_GREATER_equal_c(2, 2, false);

  // invalid operand types
  assert_invalid_binary_numeric_operator_operand_types(CHUNK_OP_GREATER, "greater-than");

#undef assert_numbers_a_b_CHUNK_OP_GREATER_equal_c
}

static void test_CHUNK_OP_GREATER_EQUAL(void **const _) {
#define assert_numbers_a_b_CHUNK_OP_GREATER_EQUAL_equal_c(number_a, number_b, expected_bool_c)                         \
  assert_value_is_result_of_instruction_on_values(                                                                     \
    VALUE_MAKE_BOOL(expected_bool_c), CHUNK_OP_GREATER_EQUAL, VALUE_MAKE_NUMBER(number_a), VALUE_MAKE_NUMBER(number_b) \
  )

  // ascending numbers
  assert_numbers_a_b_CHUNK_OP_GREATER_EQUAL_equal_c(-5, 0, false);
  assert_numbers_a_b_CHUNK_OP_GREATER_EQUAL_equal_c(0, 5, false);
  assert_numbers_a_b_CHUNK_OP_GREATER_EQUAL_equal_c(5, 10, false);

  // descending numbers
  assert_numbers_a_b_CHUNK_OP_GREATER_EQUAL_equal_c(0, -5, true);
  assert_numbers_a_b_CHUNK_OP_GREATER_EQUAL_equal_c(5, 0, true);
  assert_numbers_a_b_CHUNK_OP_GREATER_EQUAL_equal_c(10, 5, true);

  // equal numbers
  assert_numbers_a_b_CHUNK_OP_GREATER_EQUAL_equal_c(-1, -1, true);
  assert_numbers_a_b_CHUNK_OP_GREATER_EQUAL_equal_c(0, 0, true);
  assert_numbers_a_b_CHUNK_OP_GREATER_EQUAL_equal_c(2, 2, true);

  // invalid operand types
  assert_invalid_binary_numeric_operator_operand_types(CHUNK_OP_GREATER_EQUAL, "greater-than-or-equal");

#undef assert_numbers_a_b_CHUNK_OP_GREATER_EQUAL_equal_c
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
