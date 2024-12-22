#include "backend/vm.h"

#include "backend/chunk.h"
#include "backend/value.h"
#include "component/component_test_utils.h"
#include "global.h"
#include "utils/error.h"
#include "utils/number.h"

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

static void stack_pop_assert(Value const expected_value) {
  Value const value = vm_stack_pop();

  if (is_integer(value) && is_integer(expected_value)) {
    assert_double_equal(value, expected_value, 0);
  } else {
    assert_double_equal(value, expected_value, 1e-9);
  }
}

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
static_assert(OP_OPCODE_COUNT == 11, "Exhaustive OpCode handling");

static void test_OP_CONSTANT(void **const _) {
  append_constant_instructions(1, 2, 3);
  append_instruction(OP_RETURN);
  run_assert_success();
  stack_pop_assert(3), stack_pop_assert(2), stack_pop_assert(1);
  assert_empty_stack();
}

static void test_OP_CONSTANT_2B(void **const _) {
  // force OP_CONSTANT_2B usage
  for (int i = 0; i < UCHAR_MAX; i++) value_array_append(&chunk.constants, i);

  append_constant_instructions(1, 2, 3);
  append_instruction(OP_RETURN);
  run_assert_success();
  stack_pop_assert(3), stack_pop_assert(2), stack_pop_assert(1);
  assert_empty_stack();
}

static void test_OP_PRINT(void **const _) {
  append_constant_instruction(1);
  append_instructions(OP_PRINT, OP_RETURN);
  run_assert_success();
  assert_runtime_output("1");
  assert_empty_stack();
}

static void test_OP_POP(void **const _) {
  append_constant_instructions(1, 2);
  append_instructions(OP_POP, OP_RETURN);
  run_assert_success();
  stack_pop_assert(1);
  assert_empty_stack();
}

static void test_OP_NEGATE(void **const _) {
#define assert_a_OP_NEGATE_equals_b(constant_a, expected_value_b) \
  do {                                                            \
    reset_test_case_env();                                        \
    append_constant_instruction(constant_a);                      \
    append_instructions(OP_NEGATE, OP_RETURN);                    \
    run_assert_success();                                         \
    stack_pop_assert(expected_value_b);                           \
    assert_empty_stack();                                         \
  } while (0)

  // signed integer negation
  assert_a_OP_NEGATE_equals_b(1, -1);
  assert_a_OP_NEGATE_equals_b(-2, 2);

  // floating-point negation
  assert_a_OP_NEGATE_equals_b(1.25, -1.25);

  // signed zero negation
  assert_a_OP_NEGATE_equals_b(0, -0);
  assert_a_OP_NEGATE_equals_b(-0, 0);

  // negation stacking
  reset_test_case_env();
  append_constant_instruction(2);
  append_instructions(OP_NEGATE, OP_NEGATE, OP_RETURN);
  run_assert_success();
  stack_pop_assert(2);
  assert_empty_stack();

  reset_test_case_env();
  append_constant_instruction(3);
  append_instructions(OP_NEGATE, OP_NEGATE, OP_NEGATE, OP_RETURN);
  run_assert_success();
  stack_pop_assert(-3);
  assert_empty_stack();

#undef assert_a_OP_NEGATE_equals_b
}

static void test_OP_ADD(void **const _) {
#define assert_a_b_OP_ADD_equals_c(constant_a, constant_b, expected_value_c) \
  do {                                                                       \
    reset_test_case_env();                                                   \
    append_constant_instructions(constant_a, constant_b);                    \
    append_instructions(OP_ADD, OP_RETURN);                                  \
    run_assert_success();                                                    \
    stack_pop_assert(expected_value_c);                                      \
    assert_empty_stack();                                                    \
  } while (0)

  // integer addition and commutativity
  assert_a_b_OP_ADD_equals_c(1, 2, 3);
  assert_a_b_OP_ADD_equals_c(2, 1, 3);

  // addition properties
  assert_a_b_OP_ADD_equals_c(2, 0, 2);
  assert_a_b_OP_ADD_equals_c(2, -2, 0);

  // floating-point addition
  assert_a_b_OP_ADD_equals_c(1.7, 2.25, 3.95);

  // signed integer addition
  assert_a_b_OP_ADD_equals_c(5, -7, -2);
  assert_a_b_OP_ADD_equals_c(-5, 7, 2);
  assert_a_b_OP_ADD_equals_c(-5, -7, -12);

  // signed zero addition
  assert_a_b_OP_ADD_equals_c(0, 0, 0);
  assert_a_b_OP_ADD_equals_c(0, -0, 0);
  assert_a_b_OP_ADD_equals_c(-0, 0, 0);
  assert_a_b_OP_ADD_equals_c(-0, -0, -0);

#undef assert_a_b_OP_ADD_equals_c
}

static void test_OP_SUBTRACT(void **const _) {
#define assert_a_b_OP_SUBTRACT_equals_c(constant_a, constant_b, expected_value_c) \
  do {                                                                            \
    reset_test_case_env();                                                        \
    append_constant_instructions(constant_a, constant_b);                         \
    append_instructions(OP_SUBTRACT, OP_RETURN);                                  \
    run_assert_success();                                                         \
    stack_pop_assert(expected_value_c);                                           \
    assert_empty_stack();                                                         \
  } while (0)

  // integer subtraction and non-commutativity
  assert_a_b_OP_SUBTRACT_equals_c(4, 3, 1);
  assert_a_b_OP_SUBTRACT_equals_c(3, 4, -1);

  // subtraction properties
  assert_a_b_OP_SUBTRACT_equals_c(2, 0, 2);
  assert_a_b_OP_SUBTRACT_equals_c(25, 25, 0);

  // floating-point subtraction
  assert_a_b_OP_SUBTRACT_equals_c(3.75, 2.45, 1.3);

  // signed integer subtraction
  assert_a_b_OP_SUBTRACT_equals_c(-4, 3, -7);
  assert_a_b_OP_SUBTRACT_equals_c(4, -3, 7);
  assert_a_b_OP_SUBTRACT_equals_c(-4, -3, -1);

  // signed zero subtraction
  assert_a_b_OP_SUBTRACT_equals_c(0, 0, 0);
  assert_a_b_OP_SUBTRACT_equals_c(0, -0, 0);
  assert_a_b_OP_SUBTRACT_equals_c(-0, 0, -0);
  assert_a_b_OP_SUBTRACT_equals_c(-0, -0, 0);

#undef assert_a_b_OP_SUBTRACT_equals_c
}

static void test_OP_MULTIPLY(void **const _) {
#define assert_a_b_OP_MULTIPLY_equals_c(constant_a, constant_b, expected_value_c) \
  do {                                                                            \
    reset_test_case_env();                                                        \
    append_constant_instructions(constant_a, constant_b);                         \
    append_instructions(OP_MULTIPLY, OP_RETURN);                                  \
    run_assert_success();                                                         \
    stack_pop_assert(expected_value_c);                                           \
    assert_empty_stack();                                                         \
  } while (0)

  // integer multiplication and commutativity
  assert_a_b_OP_MULTIPLY_equals_c(5, 3, 15);
  assert_a_b_OP_MULTIPLY_equals_c(3, 5, 15);

  // multiplication properties
  assert_a_b_OP_MULTIPLY_equals_c(125, 1, 125);
  assert_a_b_OP_MULTIPLY_equals_c(50, 0, 0);

  // floating-point multiplication
  assert_a_b_OP_MULTIPLY_equals_c(12.34, 0.3, 3.702);

  // signed integer multiplication
  assert_a_b_OP_MULTIPLY_equals_c(-2, 4, -8);
  assert_a_b_OP_MULTIPLY_equals_c(2, -4, -8);
  assert_a_b_OP_MULTIPLY_equals_c(-2, -4, 8);

  // signed zero multiplication
  assert_a_b_OP_MULTIPLY_equals_c(0, 0, 0);
  assert_a_b_OP_MULTIPLY_equals_c(0, -0, -0);
  assert_a_b_OP_MULTIPLY_equals_c(-0, 0, -0);
  assert_a_b_OP_MULTIPLY_equals_c(-0, -0, 0);

#undef assert_a_b_OP_MULTIPLY_equals_c
}

static void test_OP_DIVIDE(void **const _) {
#define assert_a_b_OP_DIVIDE_equals_c(constant_a, constant_b, expected_value_c) \
  do {                                                                          \
    reset_test_case_env();                                                      \
    append_constant_instructions(constant_a, constant_b);                       \
    append_instructions(OP_DIVIDE, OP_RETURN);                                  \
    run_assert_success();                                                       \
    stack_pop_assert(expected_value_c);                                         \
    assert_empty_stack();                                                       \
  } while (0)

  // integer division and non-commutativity
  assert_a_b_OP_DIVIDE_equals_c(8, 2, 4);
  assert_a_b_OP_DIVIDE_equals_c(2, 8, 0.25);

  // division properties
  assert_a_b_OP_DIVIDE_equals_c(4, 1, 4);
  assert_a_b_OP_DIVIDE_equals_c(25, 25, 1);

  // floating-point division
  assert_a_b_OP_DIVIDE_equals_c(4.2, 1.5, 2.8);

  // signed integer division
  assert_a_b_OP_DIVIDE_equals_c(-5, 2, -2.5);
  assert_a_b_OP_DIVIDE_equals_c(5, -2, -2.5);
  assert_a_b_OP_DIVIDE_equals_c(-5, -2, 2.5);

  // signed zero division
  assert_a_b_OP_DIVIDE_equals_c(0, 2, 0);
  assert_a_b_OP_DIVIDE_equals_c(0, -2, -0);
  assert_a_b_OP_DIVIDE_equals_c(-0, 2, -0);
  assert_a_b_OP_DIVIDE_equals_c(-0, -2, 0);

  // division by zero
  reset_test_case_env();
  append_constant_instructions(5, 0);
  append_instructions(OP_DIVIDE, OP_RETURN);
  run_assert_failure();
  assert_execution_error("Illegal division by zero");

  reset_test_case_env();
  append_constant_instructions(5, -0);
  append_instructions(OP_DIVIDE, OP_RETURN);
  run_assert_failure();
  assert_execution_error("Illegal division by zero");

#undef assert_a_b_OP_DIVIDE_equals_c
}

static void test_OP_MODULO(void **const _) {
#define assert_a_b_OP_MODULO_equals_c(constant_a, constant_b, expected_value_c) \
  do {                                                                          \
    reset_test_case_env();                                                      \
    append_constant_instructions(constant_a, constant_b);                       \
    append_instructions(OP_MODULO, OP_RETURN);                                  \
    run_assert_success();                                                       \
    stack_pop_assert(expected_value_c);                                         \
    assert_empty_stack();                                                       \
  } while (0)

  // integer modulo and non-commutativity
  assert_a_b_OP_MODULO_equals_c(8, 2, 0);
  assert_a_b_OP_MODULO_equals_c(2, 8, 2);

  // modulo properties
  assert_a_b_OP_MODULO_equals_c(25, 25, 0);
  assert_a_b_OP_MODULO_equals_c(25, 1, 0);

  // floating-point modulo
  assert_a_b_OP_MODULO_equals_c(4.68, 3.23, 1.45);

  // signed integer modulo
  assert_a_b_OP_MODULO_equals_c(-5, 2, -1);
  assert_a_b_OP_MODULO_equals_c(5, -2, 1);
  assert_a_b_OP_MODULO_equals_c(-5, -2, -1);

  // signed zero modulo
  assert_a_b_OP_MODULO_equals_c(0, 2, 0);
  assert_a_b_OP_MODULO_equals_c(0, -2, 0);
  assert_a_b_OP_MODULO_equals_c(-0, 2, -0);
  assert_a_b_OP_MODULO_equals_c(-0, -2, -0);

  // modulo by zero
  reset_test_case_env();
  append_constant_instructions(5, 0);
  append_instructions(OP_MODULO, OP_RETURN);
  run_assert_failure();
  assert_execution_error("Illegal modulo by zero");

  reset_test_case_env();
  append_constant_instructions(5, -0);
  append_instructions(OP_MODULO, OP_RETURN);
  run_assert_failure();
  assert_execution_error("Illegal modulo by zero");

#undef assert_a_b_OP_MODULO_equals_c
}

int main(void) {
  // OP_RETURN test is missing as it's not yet properly implemented

  struct CMUnitTest const tests[] = {
    cmocka_unit_test_setup_teardown(test_OP_CONSTANT, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_OP_CONSTANT_2B, setup_test_case_env, teardown_test_case_env),
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
