#include "backend/vm.h"

#include "backend/chunk.h"
#include "backend/value.h"
#include "component/component_test_utils.h"
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
  clear_binary_stream_resource_content(g_execution_error_stream);
  clear_binary_stream_resource_content(g_runtime_output_stream);

  return vm_run(&chunk);
}

static void reset_test_case_env(void) {
  vm_reset();
  chunk_reset(&chunk);
}

#define run_assert_success() assert_true(run())
#define run_assert_failure() assert_false(run())

#define assert_empty_stack() assert_int_equal(*t_vm_stack_count, 0)

#define stack_pop_assert(expected_value) assert_value_equality(vm_stack_pop(), expected_value)
#define stack_pop_assert_many(...) APPLY_TO_EACH_ARG(stack_pop_assert, Value, __VA_ARGS__)

#define append_constant_instruction(constant) chunk_append_constant_instruction(&chunk, constant, 1)
#define append_constant_instructions(...) APPLY_TO_EACH_ARG(append_constant_instruction, Value, __VA_ARGS__)

#define append_instruction(opcode) chunk_append_instruction(&chunk, opcode, 1)
#define append_instructions(...) APPLY_TO_EACH_ARG(append_instruction, OpCode, __VA_ARGS__)

#define assert_execution_error(expected_error_message)                                                 \
  assert_binary_stream_resource_content(                                                               \
    g_execution_error_stream, "[EXECUTION_ERROR]" M_S __FILE__ P_S "1" M_S expected_error_message "\n" \
  )

#define assert_runtime_output(expected_output) \
  assert_binary_stream_resource_content(g_runtime_output_stream, expected_output "\n")

#define assert_invalid_binary_arithmetic_operator_operand_types(operator_instruction, operator_descriptor)       \
  do {                                                                                                           \
    reset_test_case_env();                                                                                       \
    append_instructions(OP_NIL, OP_NIL, operator_instruction, OP_RETURN);                                        \
    run_assert_failure();                                                                                        \
    assert_execution_error("Expected " operator_descriptor " operands to be numbers (got 'nil' and 'nil')");     \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    append_instruction(OP_NIL);                                                                                  \
    append_constant_instruction(NUMBER_VALUE(1));                                                                \
    append_instructions(operator_instruction, OP_RETURN);                                                        \
    run_assert_failure();                                                                                        \
    assert_execution_error("Expected " operator_descriptor " operands to be numbers (got 'nil' and 'number')");  \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    append_constant_instruction(NUMBER_VALUE(1));                                                                \
    append_instructions(OP_NIL, operator_instruction, OP_RETURN);                                                \
    run_assert_failure();                                                                                        \
    assert_execution_error("Expected " operator_descriptor " operands to be numbers (got 'number' and 'nil')");  \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    append_instructions(OP_TRUE, OP_FALSE, operator_instruction, OP_RETURN);                                     \
    run_assert_failure();                                                                                        \
    assert_execution_error("Expected " operator_descriptor " operands to be numbers (got 'bool' and 'bool')");   \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    append_instruction(OP_TRUE);                                                                                 \
    append_constant_instruction(NUMBER_VALUE(1));                                                                \
    append_instructions(operator_instruction, OP_RETURN);                                                        \
    run_assert_failure();                                                                                        \
    assert_execution_error("Expected " operator_descriptor " operands to be numbers (got 'bool' and 'number')"); \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    append_constant_instruction(NUMBER_VALUE(1));                                                                \
    append_instructions(OP_FALSE, operator_instruction, OP_RETURN);                                              \
    run_assert_failure();                                                                                        \
    assert_execution_error("Expected " operator_descriptor " operands to be numbers (got 'number' and 'bool')"); \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    append_instructions(OP_NIL, OP_TRUE, operator_instruction, OP_RETURN);                                       \
    run_assert_failure();                                                                                        \
    assert_execution_error("Expected " operator_descriptor " operands to be numbers (got 'nil' and 'bool')");    \
                                                                                                                 \
    reset_test_case_env();                                                                                       \
    append_instructions(OP_FALSE, OP_NIL, operator_instruction, OP_RETURN);                                      \
    run_assert_failure();                                                                                        \
    assert_execution_error("Expected " operator_descriptor " operands to be numbers (got 'bool' and 'nil')");    \
  } while (0)

// *---------------------------------------------*
// *                  FIXTURES                   *
// *---------------------------------------------*

static int setup_test_group_env(void **const _) {
  g_source_file = __FILE__;

  g_execution_error_stream = tmpfile();
  if (g_execution_error_stream == NULL) IO_ERROR("%s", strerror(errno));

  g_runtime_output_stream = tmpfile();
  if (g_runtime_output_stream == NULL) IO_ERROR("%s", strerror(errno));

  return 0;
}

static int teardown_test_group_env(void **const _) {
  if (fclose(g_execution_error_stream)) IO_ERROR("%s", strerror(errno));
  if (fclose(g_runtime_output_stream)) IO_ERROR("%s", strerror(errno));

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
static_assert(OP_OPCODE_COUNT == 14, "Exhaustive OpCode handling");

static void test_OP_CONSTANT(void **const _) {
  append_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2), NUMBER_VALUE(3));
  append_instruction(OP_RETURN);
  run_assert_success();
  stack_pop_assert_many(NUMBER_VALUE(3), NUMBER_VALUE(2), NUMBER_VALUE(1));
  assert_empty_stack();
}

static void test_OP_CONSTANT_2B(void **const _) {
  // force OP_CONSTANT_2B usage
  for (int i = 0; i < UCHAR_MAX; i++) value_array_append(&chunk.constants, NUMBER_VALUE(i));

  append_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2), NUMBER_VALUE(3));
  append_instruction(OP_RETURN);
  run_assert_success();
  stack_pop_assert_many(NUMBER_VALUE(3), NUMBER_VALUE(2), NUMBER_VALUE(1));
  assert_empty_stack();
}

static void test_OP_NIL(void **const _) {
  append_instructions(OP_NIL, OP_RETURN);
  run_assert_success();
  stack_pop_assert(NIL_VALUE());
  assert_empty_stack();
}

static void test_OP_TRUE(void **const _) {
  append_instructions(OP_TRUE, OP_RETURN);
  run_assert_success();
  stack_pop_assert(BOOL_VALUE(true));
  assert_empty_stack();
}

static void test_OP_FALSE(void **const _) {
  append_instructions(OP_FALSE, OP_RETURN);
  run_assert_success();
  stack_pop_assert(BOOL_VALUE(false));
  assert_empty_stack();
}

static void test_OP_PRINT(void **const _) {
  append_constant_instruction(NUMBER_VALUE(1));
  append_instructions(OP_PRINT, OP_RETURN);
  run_assert_success();
  assert_runtime_output("1");
  assert_empty_stack();
}

static void test_OP_POP(void **const _) {
  append_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2));
  append_instructions(OP_POP, OP_RETURN);
  run_assert_success();
  stack_pop_assert(NUMBER_VALUE(1));
  assert_empty_stack();
}

static void test_OP_NEGATE(void **const _) {
#define assert_number_constant_a_OP_NEGATE_equal_b(number_a, expected_number_b) \
  do {                                                                          \
    reset_test_case_env();                                                      \
    append_constant_instruction(NUMBER_VALUE(number_a));                        \
    append_instructions(OP_NEGATE, OP_RETURN);                                  \
    run_assert_success();                                                       \
    stack_pop_assert(NUMBER_VALUE(expected_number_b));                          \
    assert_empty_stack();                                                       \
  } while (0)

  // signed integer negation
  assert_number_constant_a_OP_NEGATE_equal_b(1, -1);
  assert_number_constant_a_OP_NEGATE_equal_b(-2, 2);

  // floating-point negation
  assert_number_constant_a_OP_NEGATE_equal_b(1.25, -1.25);

  // signed zero negation
  assert_number_constant_a_OP_NEGATE_equal_b(0, -0);
  assert_number_constant_a_OP_NEGATE_equal_b(-0, 0);

  // negation stacking
  reset_test_case_env();
  append_constant_instruction(NUMBER_VALUE(2));
  append_instructions(OP_NEGATE, OP_NEGATE, OP_RETURN);
  run_assert_success();
  stack_pop_assert(NUMBER_VALUE(2));
  assert_empty_stack();

  reset_test_case_env();
  append_constant_instruction(NUMBER_VALUE(3));
  append_instructions(OP_NEGATE, OP_NEGATE, OP_NEGATE, OP_RETURN);
  run_assert_success();
  stack_pop_assert(NUMBER_VALUE(-3));
  assert_empty_stack();

  // invalid operand types
  reset_test_case_env();
  append_instructions(OP_NIL, OP_NEGATE, OP_RETURN);
  run_assert_failure();
  assert_execution_error("Expected negation operand to be a number (got 'nil')");

  reset_test_case_env();
  append_instructions(OP_TRUE, OP_NEGATE, OP_RETURN);
  run_assert_failure();
  assert_execution_error("Expected negation operand to be a number (got 'bool')");

  reset_test_case_env();
  append_instructions(OP_FALSE, OP_NEGATE, OP_RETURN);
  run_assert_failure();
  assert_execution_error("Expected negation operand to be a number (got 'bool')");

#undef assert_number_constant_a_OP_NEGATE_equal_b
}

static void test_OP_ADD(void **const _) {
#define assert_number_constants_a_b_OP_ADD_equal_c(number_a, number_b, expected_number_c) \
  do {                                                                                    \
    reset_test_case_env();                                                                \
    append_constant_instructions(NUMBER_VALUE(number_a), NUMBER_VALUE(number_b));         \
    append_instructions(OP_ADD, OP_RETURN);                                               \
    run_assert_success();                                                                 \
    stack_pop_assert(NUMBER_VALUE(expected_number_c));                                    \
    assert_empty_stack();                                                                 \
  } while (0)

  // integer addition and commutativity
  assert_number_constants_a_b_OP_ADD_equal_c(1, 2, 3);
  assert_number_constants_a_b_OP_ADD_equal_c(2, 1, 3);

  // addition properties
  assert_number_constants_a_b_OP_ADD_equal_c(2, 0, 2);
  assert_number_constants_a_b_OP_ADD_equal_c(2, -2, 0);

  // floating-point addition
  assert_number_constants_a_b_OP_ADD_equal_c(1.7, 2.25, 3.95);

  // signed integer addition
  assert_number_constants_a_b_OP_ADD_equal_c(5, -7, -2);
  assert_number_constants_a_b_OP_ADD_equal_c(-5, 7, 2);
  assert_number_constants_a_b_OP_ADD_equal_c(-5, -7, -12);

  // signed zero addition
  assert_number_constants_a_b_OP_ADD_equal_c(0, 0, 0);
  assert_number_constants_a_b_OP_ADD_equal_c(0, -0, 0);
  assert_number_constants_a_b_OP_ADD_equal_c(-0, 0, 0);
  assert_number_constants_a_b_OP_ADD_equal_c(-0, -0, -0);

  // invalid operand types
  assert_invalid_binary_arithmetic_operator_operand_types(OP_ADD, "addition");

#undef assert_number_constants_a_b_OP_ADD_equal_c
}

static void test_OP_SUBTRACT(void **const _) {
#define assert_number_constants_a_b_OP_SUBTRACT_equal_c(number_a, number_b, expected_number_c) \
  do {                                                                                         \
    reset_test_case_env();                                                                     \
    append_constant_instructions(NUMBER_VALUE(number_a), NUMBER_VALUE(number_b));              \
    append_instructions(OP_SUBTRACT, OP_RETURN);                                               \
    run_assert_success();                                                                      \
    stack_pop_assert(NUMBER_VALUE(expected_number_c));                                         \
    assert_empty_stack();                                                                      \
  } while (0)

  // integer subtraction and non-commutativity
  assert_number_constants_a_b_OP_SUBTRACT_equal_c(4, 3, 1);
  assert_number_constants_a_b_OP_SUBTRACT_equal_c(3, 4, -1);

  // subtraction properties
  assert_number_constants_a_b_OP_SUBTRACT_equal_c(2, 0, 2);
  assert_number_constants_a_b_OP_SUBTRACT_equal_c(25, 25, 0);

  // floating-point subtraction
  assert_number_constants_a_b_OP_SUBTRACT_equal_c(3.75, 2.45, 1.3);

  // signed integer subtraction
  assert_number_constants_a_b_OP_SUBTRACT_equal_c(-4, 3, -7);
  assert_number_constants_a_b_OP_SUBTRACT_equal_c(4, -3, 7);
  assert_number_constants_a_b_OP_SUBTRACT_equal_c(-4, -3, -1);

  // signed zero subtraction
  assert_number_constants_a_b_OP_SUBTRACT_equal_c(0, 0, 0);
  assert_number_constants_a_b_OP_SUBTRACT_equal_c(0, -0, 0);
  assert_number_constants_a_b_OP_SUBTRACT_equal_c(-0, 0, -0);
  assert_number_constants_a_b_OP_SUBTRACT_equal_c(-0, -0, 0);

  // invalid operand types
  assert_invalid_binary_arithmetic_operator_operand_types(OP_SUBTRACT, "subtraction");

#undef assert_number_constants_a_b_OP_SUBTRACT_equal_c
}

static void test_OP_MULTIPLY(void **const _) {
#define assert_number_constants_a_b_OP_MULTIPLY_equal_c(number_a, number_b, expected_number_c) \
  do {                                                                                         \
    reset_test_case_env();                                                                     \
    append_constant_instructions(NUMBER_VALUE(number_a), NUMBER_VALUE(number_b));              \
    append_instructions(OP_MULTIPLY, OP_RETURN);                                               \
    run_assert_success();                                                                      \
    stack_pop_assert(NUMBER_VALUE(expected_number_c));                                         \
    assert_empty_stack();                                                                      \
  } while (0)

  // integer multiplication and commutativity
  assert_number_constants_a_b_OP_MULTIPLY_equal_c(5, 3, 15);
  assert_number_constants_a_b_OP_MULTIPLY_equal_c(3, 5, 15);

  // multiplication properties
  assert_number_constants_a_b_OP_MULTIPLY_equal_c(125, 1, 125);
  assert_number_constants_a_b_OP_MULTIPLY_equal_c(50, 0, 0);

  // floating-point multiplication
  assert_number_constants_a_b_OP_MULTIPLY_equal_c(12.34, 0.3, 3.702);

  // signed integer multiplication
  assert_number_constants_a_b_OP_MULTIPLY_equal_c(-2, 4, -8);
  assert_number_constants_a_b_OP_MULTIPLY_equal_c(2, -4, -8);
  assert_number_constants_a_b_OP_MULTIPLY_equal_c(-2, -4, 8);

  // signed zero multiplication
  assert_number_constants_a_b_OP_MULTIPLY_equal_c(0, 0, 0);
  assert_number_constants_a_b_OP_MULTIPLY_equal_c(0, -0, -0);
  assert_number_constants_a_b_OP_MULTIPLY_equal_c(-0, 0, -0);
  assert_number_constants_a_b_OP_MULTIPLY_equal_c(-0, -0, 0);

  // invalid operand types
  assert_invalid_binary_arithmetic_operator_operand_types(OP_MULTIPLY, "multiplication");

#undef assert_number_constants_a_b_OP_MULTIPLY_equal_c
}

static void test_OP_DIVIDE(void **const _) {
#define assert_number_constants_a_b_OP_DIVIDE_equal_c(number_a, number_b, expected_number_c) \
  do {                                                                                       \
    reset_test_case_env();                                                                   \
    append_constant_instructions(NUMBER_VALUE(number_a), NUMBER_VALUE(number_b));            \
    append_instructions(OP_DIVIDE, OP_RETURN);                                               \
    run_assert_success();                                                                    \
    stack_pop_assert(NUMBER_VALUE(expected_number_c));                                       \
    assert_empty_stack();                                                                    \
  } while (0)

  // integer division and non-commutativity
  assert_number_constants_a_b_OP_DIVIDE_equal_c(8, 2, 4);
  assert_number_constants_a_b_OP_DIVIDE_equal_c(2, 8, 0.25);

  // division properties
  assert_number_constants_a_b_OP_DIVIDE_equal_c(4, 1, 4);
  assert_number_constants_a_b_OP_DIVIDE_equal_c(25, 25, 1);

  // floating-point division
  assert_number_constants_a_b_OP_DIVIDE_equal_c(4.2, 1.5, 2.8);

  // signed integer division
  assert_number_constants_a_b_OP_DIVIDE_equal_c(-5, 2, -2.5);
  assert_number_constants_a_b_OP_DIVIDE_equal_c(5, -2, -2.5);
  assert_number_constants_a_b_OP_DIVIDE_equal_c(-5, -2, 2.5);

  // signed zero division
  assert_number_constants_a_b_OP_DIVIDE_equal_c(0, 2, 0);
  assert_number_constants_a_b_OP_DIVIDE_equal_c(0, -2, -0);
  assert_number_constants_a_b_OP_DIVIDE_equal_c(-0, 2, -0);
  assert_number_constants_a_b_OP_DIVIDE_equal_c(-0, -2, 0);

  // division by zero
  reset_test_case_env();
  append_constant_instructions(NUMBER_VALUE(5), NUMBER_VALUE(0));
  append_instructions(OP_DIVIDE, OP_RETURN);
  run_assert_failure();
  assert_execution_error("Illegal division by zero");

  reset_test_case_env();
  append_constant_instructions(NUMBER_VALUE(5), NUMBER_VALUE(-0));
  append_instructions(OP_DIVIDE, OP_RETURN);
  run_assert_failure();
  assert_execution_error("Illegal division by zero");

  // invalid operand types
  assert_invalid_binary_arithmetic_operator_operand_types(OP_DIVIDE, "division");

#undef assert_number_constants_a_b_OP_DIVIDE_equal_c
}

static void test_OP_MODULO(void **const _) {
#define assert_number_constants_a_b_OP_MODULO_equal_c(number_a, number_b, expected_number_c) \
  do {                                                                                       \
    reset_test_case_env();                                                                   \
    append_constant_instructions(NUMBER_VALUE(number_a), NUMBER_VALUE(number_b));            \
    append_instructions(OP_MODULO, OP_RETURN);                                               \
    run_assert_success();                                                                    \
    stack_pop_assert(NUMBER_VALUE(expected_number_c));                                       \
    assert_empty_stack();                                                                    \
  } while (0)

  // integer modulo and non-commutativity
  assert_number_constants_a_b_OP_MODULO_equal_c(8, 2, 0);
  assert_number_constants_a_b_OP_MODULO_equal_c(2, 8, 2);

  // modulo properties
  assert_number_constants_a_b_OP_MODULO_equal_c(25, 25, 0);
  assert_number_constants_a_b_OP_MODULO_equal_c(25, 1, 0);

  // floating-point modulo
  assert_number_constants_a_b_OP_MODULO_equal_c(4.68, 3.23, 1.45);

  // signed integer modulo
  assert_number_constants_a_b_OP_MODULO_equal_c(-5, 2, -1);
  assert_number_constants_a_b_OP_MODULO_equal_c(5, -2, 1);
  assert_number_constants_a_b_OP_MODULO_equal_c(-5, -2, -1);

  // signed zero modulo
  assert_number_constants_a_b_OP_MODULO_equal_c(0, 2, 0);
  assert_number_constants_a_b_OP_MODULO_equal_c(0, -2, 0);
  assert_number_constants_a_b_OP_MODULO_equal_c(-0, 2, -0);
  assert_number_constants_a_b_OP_MODULO_equal_c(-0, -2, -0);

  // modulo by zero
  reset_test_case_env();
  append_constant_instructions(NUMBER_VALUE(5), NUMBER_VALUE(0));
  append_instructions(OP_MODULO, OP_RETURN);
  run_assert_failure();
  assert_execution_error("Illegal modulo by zero");

  reset_test_case_env();
  append_constant_instructions(NUMBER_VALUE(5), NUMBER_VALUE(-0));
  append_instructions(OP_MODULO, OP_RETURN);
  run_assert_failure();
  assert_execution_error("Illegal modulo by zero");

  // invalid operand types
  assert_invalid_binary_arithmetic_operator_operand_types(OP_MODULO, "modulo");

#undef assert_a_b_OP_MODULO_equals_c
}

int main(void) {
  // OP_RETURN test is missing as it's not yet properly implemented

  struct CMUnitTest const tests[] = {
    cmocka_unit_test_setup_teardown(test_OP_CONSTANT, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_OP_CONSTANT_2B, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_OP_NIL, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_OP_TRUE, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_OP_FALSE, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_OP_PRINT, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_OP_POP, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_OP_NEGATE, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_OP_ADD, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_OP_SUBTRACT, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_OP_MULTIPLY, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_OP_DIVIDE, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_OP_MODULO, setup_test_case_env, teardown_test_case_env),
  };

  return cmocka_run_group_tests(tests, setup_test_group_env, teardown_test_group_env);
}
