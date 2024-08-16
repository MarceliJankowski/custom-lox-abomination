#include "backend/vm.h"

#include "backend/chunk.h"
#include "backend/value.h"
#include "global.h"
#include "test_common.h"
#include "util/error.h"

#include <stdio.h>

// *---------------------------------------------*
// *               STATIC OBJECTS                *
// *---------------------------------------------*

static Chunk chunk;

// *---------------------------------------------*
// *                  UTILITIES                  *
// *---------------------------------------------*

void reset(void) {
  vm_reset();
  chunk_reset(&chunk);
}

#define run_assert_success() assert_true(vm_run(&chunk))
#define run_assert_failure() assert_false(vm_run(&chunk))

#define stack_pop_assert(expected_value) assert_int_equal(vm_stack_pop(), expected_value)

#define append_constant_instruction(constant) chunk_append_constant_instruction(&chunk, constant, 1)
#define append_constant_instructions(...) APPLY_TO_EACH_ARG(append_constant_instruction, Value, __VA_ARGS__)

#define append_instruction(opcode) chunk_append_instruction(&chunk, opcode, 1)
#define append_instructions(...) APPLY_TO_EACH_ARG(append_instruction, OpCode, __VA_ARGS__)

// *---------------------------------------------*
// *                  FIXTURES                   *
// *---------------------------------------------*

int group_setup(void **const _) {
  g_source_file = "vm_test";
  g_execution_err_stream = open_throwaway_stream();

  return 0;
}

int group_teardown(void **const _) {
  if (fclose(g_static_err_stream)) IO_ERROR("%s", strerror(errno));

  return 0;
}

int test_setup(void **const _) {
  vm_init();
  chunk_init(&chunk);

  return 0;
}

int test_teardown(void **const _) {
  vm_free();
  chunk_free(&chunk);

  return 0;
}

// *---------------------------------------------*
// *                 TEST CASES                  *
// *---------------------------------------------*
static_assert(OP_OPCODE_COUNT == 9, "Exhaustive OpCode handling");

void test_OP_CONSTANT(void **const _) {
  append_constant_instructions(1, 2, 3);
  append_instruction(OP_RETURN);
  run_assert_success();
  stack_pop_assert(3), stack_pop_assert(2), stack_pop_assert(1);
}

void test_OP_CONSTANT_2B(void **const _) {
  // force OP_CONSTANT_2B usage
  for (int i = 0; i < UCHAR_MAX; i++) value_array_append(&chunk.constants, i);

  append_constant_instructions(1, 2, 3);
  append_instruction(OP_RETURN);
  run_assert_success();
  stack_pop_assert(3), stack_pop_assert(2), stack_pop_assert(1);
}

void test_OP_NEGATE(void **const _) {
  append_constant_instruction(1);
  append_instructions(OP_NEGATE, OP_RETURN);
  run_assert_success();
  stack_pop_assert(-1);
}

void test_OP_ADD(void **const _) {
  append_constant_instructions(1, 2);
  append_instructions(OP_ADD, OP_RETURN);
  run_assert_success();
  stack_pop_assert(3);
}

void test_OP_SUBTRACT(void **const _) {
  append_constant_instructions(2, 1);
  append_instructions(OP_SUBTRACT, OP_RETURN);
  run_assert_success();
  stack_pop_assert(1);
}

void test_OP_MULTIPLY(void **const _) {
  append_constant_instructions(3, 4);
  append_instructions(OP_MULTIPLY, OP_RETURN);
  run_assert_success();
  stack_pop_assert(12);
}

void test_OP_DIVIDE(void **const _) {
  append_constant_instructions(8, 4);
  append_instructions(OP_DIVIDE, OP_RETURN);
  run_assert_success();
  stack_pop_assert(2);

  reset();
  append_constant_instructions(1, 0);
  append_instructions(OP_DIVIDE, OP_RETURN);
  run_assert_failure();
}

void test_OP_MODULO(void **const _) {
  append_constant_instructions(8, 2);
  append_instructions(OP_MODULO, OP_RETURN);
  run_assert_success();
  stack_pop_assert(0);

  reset();
  append_constant_instructions(9, 2);
  append_instructions(OP_MODULO, OP_RETURN);
  run_assert_success();
  stack_pop_assert(1);

  reset();
  append_constant_instructions(8, 0);
  append_instructions(OP_MODULO, OP_RETURN);
  run_assert_failure();
}

int main(void) {
  // OP_RETURN test is missing as it's not yet properly implemented

  struct CMUnitTest const tests[] = {
    cmocka_unit_test_setup_teardown(test_OP_CONSTANT, test_setup, test_teardown),
    cmocka_unit_test_setup_teardown(test_OP_CONSTANT_2B, test_setup, test_teardown),
    cmocka_unit_test_setup_teardown(test_OP_NEGATE, test_setup, test_teardown),
    cmocka_unit_test_setup_teardown(test_OP_ADD, test_setup, test_teardown),
    cmocka_unit_test_setup_teardown(test_OP_SUBTRACT, test_setup, test_teardown),
    cmocka_unit_test_setup_teardown(test_OP_MULTIPLY, test_setup, test_teardown),
    cmocka_unit_test_setup_teardown(test_OP_DIVIDE, test_setup, test_teardown),
    cmocka_unit_test_setup_teardown(test_OP_MODULO, test_setup, test_teardown),
  };

  return cmocka_run_group_tests(tests, group_setup, group_teardown);
}
