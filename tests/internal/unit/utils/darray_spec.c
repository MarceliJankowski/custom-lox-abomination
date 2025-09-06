#include "unit/unit_test.h"
#include "utils/darray.h"
#include "utils/memory.h"

#include <stdint.h>
#include <utils/darray.h>

static void darray_define__default_values(void **const _) {
  DARRAY_DEFINE(int, test_darray, memory_manage);

  assert_ptr_equal(test_darray.memory_manager, memory_manage);
  assert_int_equal(test_darray.capacity, 0);
  assert_int_equal(test_darray.count, 0);
  assert_int_equal(test_darray.capacity_growth_factor, DARRAY_DEFAULT_CAPACITY_GROWTH_FACTOR);
  assert_int_equal(test_darray.initial_growth_capacity, DARRAY_DEFAULT_INITIAL_GROWTH_CAPACITY);
  assert_int_equal(test_darray.data_object_size, sizeof(int));
  assert_null(test_darray.data);

  DARRAY_DESTROY(&test_darray);
}

static void darray_destroy___destroy_with_data(void **const _) {
  DARRAY_DEFINE(int, test_darray, memory_manage);

  int const element = 1;
  DARRAY_PUSH(&test_darray, element);

  DARRAY_DESTROY(&test_darray);

  assert_ptr_equal(test_darray.memory_manager, 0);
  assert_int_equal(test_darray.capacity, 0);
  assert_int_equal(test_darray.count, 0);
  assert_int_equal(test_darray.capacity_growth_factor, 0);
  assert_int_equal(test_darray.initial_growth_capacity, 0);
  assert_int_equal(test_darray.data_object_size, 0);
  assert_null(test_darray.data);
}

static void darray_grow_capacity__multiple_calls(void **const _) {
  DARRAY_DEFINE(int, test_darray, memory_manage);
  assert_int_equal(test_darray.capacity, 0);

  DARRAY_GROW(&test_darray);
  assert_int_equal(test_darray.capacity, 8);

  DARRAY_GROW(&test_darray);
  assert_int_equal(test_darray.capacity, 16);

  DARRAY_DESTROY(&test_darray);
}

static void darray_append__append_once(void **const _) {
  DARRAY_DEFINE(int, test_darray, memory_manage);

  assert_int_equal(test_darray.capacity, 0);

  int const element = 1;
  DARRAY_PUSH(&test_darray, element);
  int const result = test_darray.data[0];

  assert_int_equal(result, 1);
  assert_int_equal(test_darray.capacity, 8);

  DARRAY_DESTROY(&test_darray);
}

static void darray_append__append_multiple(void **const _) {
  DARRAY_DEFINE(int, test_darray, memory_manage);

  int const element1 = 1;
  DARRAY_PUSH(&test_darray, element1);
  int const result1 = test_darray.data[0];

  assert_int_equal(result1, 1);
  assert_int_equal(test_darray.capacity, 8);

  int const element2 = 2;
  DARRAY_PUSH(&test_darray, element2);
  int const result2 = test_darray.data[1];

  assert_int_equal(result2, 2);
  assert_int_equal(test_darray.capacity, 8);

  DARRAY_DESTROY(&test_darray);
}

int main(void) {
  struct CMUnitTest const tests[] = {
    cmocka_unit_test(darray_define__default_values),        cmocka_unit_test(darray_destroy___destroy_with_data),
    cmocka_unit_test(darray_grow_capacity__multiple_calls), cmocka_unit_test(darray_append__append_once),
    cmocka_unit_test(darray_append__append_multiple),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
