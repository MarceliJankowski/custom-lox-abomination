#include "utils/darray.h"
#include "utils/memory.h"

#include <stdint.h>
#include <unit/unit_test.h>
#include <utils/darray.h>

static void darray_define__default_values(void **const _) {
  DARRAY_DEFINE(int32_t, test_darray, memory_manage);

  assert_true(test_darray.memory_manager == memory_manage);
  assert_true(test_darray.capacity == 0);
  assert_true(test_darray.count == 0);
  assert_true(test_darray.capacity_growth_factor == DARRAY_DEFAULT_CAPACITY_GROWTH_FACTOR);
  assert_true(test_darray.min_growth_capacity == DARRAY_DEFAULT_MIN_GROWTH_CAPACITY);
  assert_true(test_darray.data_object_size == sizeof(int32_t));
  assert_true(test_darray.data == NULL);

  DARRAY_FREE(&test_darray);
}

static void darray_grow_capacity__multiple_calls(void **const _) {
  DARRAY_DEFINE(int32_t, test_darray, memory_manage);

  assert_true(test_darray.capacity == 0);

  DARRAY_GROW_CAPACITY(&test_darray);
  assert_true(test_darray.capacity == 8);

  DARRAY_GROW_CAPACITY(&test_darray);
  assert_true(test_darray.capacity == 16);

  DARRAY_FREE(&test_darray);
}

static void darray_append__append_once(void **const _) {
  DARRAY_DEFINE(int32_t, test_darray, memory_manage);

  int32_t const element = 1;

  DARRAY_APPEND(&test_darray, element);
  int32_t const result = test_darray.data[0];

  assert_true(result == 1);

  DARRAY_FREE(&test_darray);
}

int main(void) {
  struct CMUnitTest const tests[] = {
    cmocka_unit_test(darray_define__default_values),
    cmocka_unit_test(darray_grow_capacity__multiple_calls),
    cmocka_unit_test(darray_append__append_once),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
