#include "utils/darray.h"
#include "utils/memory.h"
#include "utils/stack.h"

#include <stdint.h>
#include <unit/unit_test.h>

static void stack_define__default_values(void **const _) {
  STACK_DEFINE(int, test_stack, memory_manage);

  assert_ptr_equal(test_stack.memory_manager, memory_manage);
  assert_int_equal(test_stack.capacity, 0);
  assert_int_equal(test_stack.count, 0);
  assert_int_equal(test_stack.capacity_growth_factor, DARRAY_DEFAULT_CAPACITY_GROWTH_FACTOR);
  assert_int_equal(test_stack.initial_growth_capacity, DARRAY_DEFAULT_INITIAL_GROWTH_CAPACITY);
  assert_int_equal(test_stack.data_object_size, sizeof(int));
  assert_null(test_stack.data);

  STACK_DESTROY(&test_stack);
}

static void stack_destroy__destroy_with_data(void **const _) {
  STACK_DEFINE(int, test_stack, memory_manage);

  int const element = 1;
  STACK_PUSH(&test_stack, element);

  STACK_DESTROY(&test_stack);

  assert_ptr_equal(test_stack.memory_manager, 0);
  assert_int_equal(test_stack.capacity, 0);
  assert_int_equal(test_stack.count, 0);
  assert_int_equal(test_stack.capacity_growth_factor, 0);
  assert_int_equal(test_stack.initial_growth_capacity, 0);
  assert_int_equal(test_stack.data_object_size, 0);
  assert_null(test_stack.data);
}

static void stack_grow_capacity__multiple_calls(void **const _) {
  STACK_DEFINE(int, test_stack, memory_manage);
  assert_true(test_stack.capacity == 0);

  STACK_GROW(&test_stack);
  assert_int_equal(test_stack.capacity, 8);

  STACK_GROW(&test_stack);
  assert_int_equal(test_stack.capacity, 16);

  STACK_DESTROY(&test_stack);
}

static void stack_push__push_once(void **const _) {
  STACK_DEFINE(int, test_stack, memory_manage);

  assert_true(test_stack.capacity == 0);

  int const element = 1;
  STACK_PUSH(&test_stack, element);

  int result = test_stack.data[0];
  assert_true(result == 1);
  result = STACK_TOP(&test_stack);
  assert_true(result == 1);

  assert_true(test_stack.capacity == 8);

  STACK_DESTROY(&test_stack);
}

static void stack_push__push_multiple(void **const _) {
  STACK_DEFINE(int, test_stack, memory_manage);

  int const element1 = 1;
  STACK_PUSH(&test_stack, element1);

  int result1 = test_stack.data[0];
  assert_true(result1 == 1);
  result1 = STACK_TOP(&test_stack);
  assert_true(result1 == 1);

  assert_true(test_stack.capacity == 8);

  int const element2 = 2;
  STACK_PUSH(&test_stack, element2);

  int result2 = test_stack.data[1];
  assert_true(result2 == 2);
  result2 = STACK_TOP(&test_stack);
  assert_true(result2 == 2);

  assert_true(test_stack.capacity == 8);

  STACK_DESTROY(&test_stack);
}

static void stack_pop__pop_once(void **const _) {
  STACK_DEFINE(int, test_stack, memory_manage);

  assert_true(test_stack.capacity == 0);

  int const element = 1;
  STACK_PUSH(&test_stack, element);

  STACK_POP(&test_stack);
  assert_true(test_stack.count == 0);

  STACK_DESTROY(&test_stack);
}

static void stack_pop__pop_multiple(void **const _) {
  STACK_DEFINE(int, test_stack, memory_manage);

  int const element1 = 1;
  STACK_PUSH(&test_stack, element1);

  int const element2 = 2;
  STACK_PUSH(&test_stack, element2);

  STACK_POP(&test_stack);
  int const result = STACK_TOP(&test_stack);
  assert_true(result == 1);

  STACK_POP(&test_stack);
  assert_true(test_stack.count == 0);

  STACK_DESTROY(&test_stack);
}

int main(void) {
  struct CMUnitTest const tests[] = {
    cmocka_unit_test(stack_define__default_values),
    cmocka_unit_test(stack_destroy__destroy_with_data),
    cmocka_unit_test(stack_grow_capacity__multiple_calls),
    cmocka_unit_test(stack_push__push_once),
    cmocka_unit_test(stack_push__push_multiple),
    cmocka_unit_test(stack_pop__pop_once),
    cmocka_unit_test(stack_pop__pop_multiple)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
