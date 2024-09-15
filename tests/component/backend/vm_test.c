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
  // clear execution errors
  g_execution_err_stream = freopen(NULL, "w+b", g_execution_err_stream);
  if (g_execution_err_stream == NULL) IO_ERROR("%s", strerror(errno));

  return vm_run(&chunk);
}

static void reset_test_case_env(void) {
  vm_reset();
  chunk_reset(&chunk);
}

#define run_assert_success() assert_true(run())
#define run_assert_failure() assert_false(run())

#define assert_empty_stack() assert_int_equal(*t_vm_stack_count, 0)
#define stack_pop_assert(expected_value) assert_int_equal(vm_stack_pop(), expected_value)

#define append_constant_instruction(constant) chunk_append_constant_instruction(&chunk, constant, 1)
#define append_constant_instructions(...) APPLY_TO_EACH_ARG(append_constant_instruction, Value, __VA_ARGS__)

#define append_instruction(opcode) chunk_append_instruction(&chunk, opcode, 1)
#define append_instructions(...) APPLY_TO_EACH_ARG(append_instruction, OpCode, __VA_ARGS__)

#define assert_execution_error(expected_error_message)                                               \
  assert_binary_stream_resource_content(                                                             \
    g_execution_err_stream, "[EXECUTION_ERROR]" M_S __FILE__ P_S "1" M_S expected_error_message "\n" \
  )

// *---------------------------------------------*
// *                  FIXTURES                   *
// *---------------------------------------------*

static int setup_test_group_env(void **const _) {
  g_source_file = __FILE__;
  g_execution_err_stream = tmpfile();
  if (g_execution_err_stream == NULL) IO_ERROR("%s", strerror(errno));

  return 0;
}

static int teardown_test_group_env(void **const _) {
  if (fclose(g_execution_err_stream)) IO_ERROR("%s", strerror(errno));

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
static_assert(OP_OPCODE_COUNT == 10, "Exhaustive OpCode handling");

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

static void test_OP_POP(void **const _) {
  append_constant_instructions(1, 2);
  append_instructions(OP_POP, OP_RETURN);
  run_assert_success();
  stack_pop_assert(1);
  assert_empty_stack();
}

static void test_OP_NEGATE(void **const _) {
  append_constant_instruction(1);
  append_instructions(OP_NEGATE, OP_RETURN);
  run_assert_success();
  stack_pop_assert(-1);
  assert_empty_stack();
}

static void test_OP_ADD(void **const _) {
  append_constant_instructions(1, 2);
  append_instructions(OP_ADD, OP_RETURN);
  run_assert_success();
  stack_pop_assert(3);
  assert_empty_stack();
}

static void test_OP_SUBTRACT(void **const _) {
  append_constant_instructions(2, 1);
  append_instructions(OP_SUBTRACT, OP_RETURN);
  run_assert_success();
  stack_pop_assert(1);
  assert_empty_stack();
}

static void test_OP_MULTIPLY(void **const _) {
  append_constant_instructions(3, 4);
  append_instructions(OP_MULTIPLY, OP_RETURN);
  run_assert_success();
  stack_pop_assert(12);
  assert_empty_stack();
}

static void test_OP_DIVIDE(void **const _) {
  append_constant_instructions(8, 4);
  append_instructions(OP_DIVIDE, OP_RETURN);
  run_assert_success();
  stack_pop_assert(2);
  assert_empty_stack();

  reset_test_case_env();
  append_constant_instructions(1, 0);
  append_instructions(OP_DIVIDE, OP_RETURN);
  run_assert_failure();
  assert_execution_error("Illegal division by zero");
}

static void test_OP_MODULO(void **const _) {
  append_constant_instructions(8, 2);
  append_instructions(OP_MODULO, OP_RETURN);
  run_assert_success();
  stack_pop_assert(0);
  assert_empty_stack();

  reset_test_case_env();
  append_constant_instructions(9, 2);
  append_instructions(OP_MODULO, OP_RETURN);
  run_assert_success();
  stack_pop_assert(1);
  assert_empty_stack();

  reset_test_case_env();
  append_constant_instructions(8, 0);
  append_instructions(OP_MODULO, OP_RETURN);
  run_assert_failure();
  assert_execution_error("Illegal modulo by zero");
}

int main(void) {
  // OP_RETURN test is missing as it's not yet properly implemented

  struct CMUnitTest const tests[] = {
    cmocka_unit_test_setup_teardown(test_OP_CONSTANT, setup_test_case_env, teardown_test_case_env),
    cmocka_unit_test_setup_teardown(test_OP_CONSTANT_2B, setup_test_case_env, teardown_test_case_env),
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
