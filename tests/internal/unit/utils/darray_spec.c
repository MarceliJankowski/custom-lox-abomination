#include "unit/unit_test.h"
#include "utils/darray.h"
#include "utils/memory.h"

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

#define ASSERT_DARRAY(                                                                                                \
  darray_ptr, expected_data, expected_capacity, expected_count, expected_data_object_size,                            \
  expected_initial_growth_capacity, expected_capacity_growth_factor, expected_memory_manager                          \
)                                                                                                                     \
  do {                                                                                                                \
    assert((darray_ptr) != NULL);                                                                                     \
                                                                                                                      \
    if ((expected_data) == NULL) assert_null((darray_ptr)->data);                                                     \
    else {                                                                                                            \
      assert_non_null((darray_ptr)->data);                                                                            \
      assert_memory_equal((darray_ptr)->data, (expected_data), (darray_ptr)->data_object_size * (darray_ptr)->count); \
    }                                                                                                                 \
    assert_int_equal((darray_ptr)->capacity, (expected_capacity));                                                    \
    assert_int_equal((darray_ptr)->count, (expected_count));                                                          \
    assert_int_equal((darray_ptr)->data_object_size, (expected_data_object_size));                                    \
    assert_int_equal((darray_ptr)->initial_growth_capacity, (expected_initial_growth_capacity));                      \
    assert_double_equal((darray_ptr)->capacity_growth_factor, (expected_capacity_growth_factor), 0);                  \
    assert_ptr_equal((darray_ptr)->memory_manager, (expected_memory_manager));                                        \
  } while (0)

#define ASSERT_DARRAY_IS_UNINITIALIZED(darray_ptr)          \
  do {                                                      \
    assert((darray_ptr) != NULL);                           \
                                                            \
    ASSERT_DARRAY((darray_ptr), NULL, 0, 0, 0, 0, 0, NULL); \
  } while (0)

// *---------------------------------------------*
// *                   SPIES                     *
// *---------------------------------------------*

static MemoryManagerFn spy_memory_manage;
static void *spy_memory_manage(void *const object, size_t const old_size, size_t const new_size) {
  check_expected_ptr(object);
  check_expected(old_size);
  check_expected(new_size);

  return memory_manage(object, old_size, new_size);
}

// *---------------------------------------------*
// *               CHECK FUNCTIONS               *
// *---------------------------------------------*

static int check_unsigned_param_is_gte(LargestIntegralType const param_value, LargestIntegralType const min_value) {
  return param_value >= min_value;
}

// *---------------------------------------------*
// *                 TEST CASES                  *
// *---------------------------------------------*

static void DARRAY_INIT_EXPLICIT__sets_all_members(void **const _) {
#define DATA_OBJECT_TYPE int
  DARRAY_TYPE(DATA_OBJECT_TYPE) darray;
  size_t const data_object_size = sizeof(DATA_OBJECT_TYPE);
  MemoryManagerFn *const memory_manager = memory_manage;
  size_t const initial_growth_capacity = 6;
  double const capacity_growth_factor = 2.4;

  DARRAY_INIT_EXPLICIT(&darray, data_object_size, memory_manager, initial_growth_capacity, capacity_growth_factor);

  ASSERT_DARRAY(&darray, NULL, 0, 0, data_object_size, initial_growth_capacity, capacity_growth_factor, memory_manager);

  DARRAY_DESTROY(&darray);
#undef DATA_OBJECT_TYPE
}

static void DARRAY_INIT__sets_all_members_with_defaults(void **const _) {
#define DATA_OBJECT_TYPE char
  DARRAY_TYPE(DATA_OBJECT_TYPE) darray;
  size_t const data_object_size = sizeof(DATA_OBJECT_TYPE);
  MemoryManagerFn *const memory_manager = memory_manage;

  DARRAY_INIT(&darray, data_object_size, memory_manager);

  ASSERT_DARRAY(
    &darray, NULL, 0, 0, data_object_size, DARRAY_DEFAULT_INITIAL_GROWTH_CAPACITY,
    DARRAY_DEFAULT_CAPACITY_GROWTH_FACTOR, memory_manager
  );

  DARRAY_DESTROY(&darray);
#undef DATA_OBJECT_TYPE
}

static void DARRAY_DEFINE__defines_with_defaults_for_members(void **const _) {
#define DATA_OBJECT_TYPE void *
  MemoryManagerFn *const memory_manager = memory_manage;

  DARRAY_DEFINE(DATA_OBJECT_TYPE, darray, memory_manager);

  ASSERT_DARRAY(
    &darray, darray.data, 0, 0, sizeof(DATA_OBJECT_TYPE), DARRAY_DEFAULT_INITIAL_GROWTH_CAPACITY,
    DARRAY_DEFAULT_CAPACITY_GROWTH_FACTOR, memory_manager
  );

  DARRAY_DESTROY(&darray);
#undef DATA_OBJECT_TYPE
}

static void DARRAY_DESTROY__releases_resources(void **const _) {
  DARRAY_DEFINE(int, darray, memory_manage);
  DARRAY_GROW(&darray);
  darray.memory_manager = spy_memory_manage;

  expect_value(spy_memory_manage, object, darray.data);
  expect_value(spy_memory_manage, old_size, darray.capacity * darray.data_object_size);
  expect_value(spy_memory_manage, new_size, 0);
  DARRAY_DESTROY(&darray);
}

static void DARRAY_DESTROY__sets_to_uninitialized_state(void **const _) {
  DARRAY_DEFINE(int, darray, memory_manage);

  DARRAY_DESTROY(&darray);

  ASSERT_DARRAY_IS_UNINITIALIZED(&darray);
}

static void DARRAY_GROW__grows_by_initial_capacity_and_growth_factor(void **const _) {
#define DATA_OBJECT_TYPE int
  size_t const data_object_size = sizeof(DATA_OBJECT_TYPE);
  size_t const initial_growth_capacity = 100;
  double const capacity_growth_factor = 4.5;
  DARRAY_TYPE(DATA_OBJECT_TYPE) darray;
  DARRAY_INIT_EXPLICIT(&darray, data_object_size, spy_memory_manage, initial_growth_capacity, capacity_growth_factor);
  size_t const expected_first_capacity = initial_growth_capacity;
  size_t const expected_second_capacity = expected_first_capacity * capacity_growth_factor;

  expect_value(spy_memory_manage, object, darray.data);
  expect_value(spy_memory_manage, old_size, 0);
  expect_value(spy_memory_manage, new_size, expected_first_capacity * data_object_size);
  DARRAY_GROW(&darray);
  assert_int_equal(darray.capacity, expected_first_capacity);

  expect_value(spy_memory_manage, object, darray.data);
  expect_value(spy_memory_manage, old_size, expected_first_capacity * data_object_size);
  expect_value(spy_memory_manage, new_size, expected_second_capacity * data_object_size);
  DARRAY_GROW(&darray);
  assert_int_equal(darray.capacity, expected_second_capacity);

  darray.memory_manager = memory_manage;
  DARRAY_DESTROY(&darray);
#undef DATA_OBJECT_TYPE
}

static void DARRAY_RESERVE__reserves_at_least_min_capacity(void **const _) {
  DARRAY_DEFINE(int, darray, spy_memory_manage);
  size_t const first_min_capacity = 20;
  size_t const second_min_capacity = first_min_capacity * 10;

  expect_value(spy_memory_manage, object, darray.data);
  expect_value(spy_memory_manage, old_size, 0);
  expect_check(spy_memory_manage, new_size, check_unsigned_param_is_gte, first_min_capacity);
  DARRAY_RESERVE(&darray, first_min_capacity);
  assert_true(darray.capacity >= first_min_capacity);

  expect_value(spy_memory_manage, object, darray.data);
  expect_value(spy_memory_manage, old_size, darray.capacity * darray.data_object_size);
  expect_check(spy_memory_manage, new_size, check_unsigned_param_is_gte, second_min_capacity);
  DARRAY_RESERVE(&darray, second_min_capacity);
  assert_true(darray.capacity >= second_min_capacity);

  darray.memory_manager = memory_manage;
  DARRAY_DESTROY(&darray);
}

static void DARRAY_PUSH__pushes_objects_onto_the_end(void **const _) {
  DARRAY_DEFINE(int, darray, memory_manage);
  int const first_object = 1;
  int const second_object = 2;

  DARRAY_PUSH(&darray, first_object);
  DARRAY_PUSH(&darray, second_object);

  assert_int_equal(darray.data[0], first_object);
  assert_int_equal(darray.data[1], second_object);

  DARRAY_DESTROY(&darray);
}

static void DARRAY_POP__pops_objects_off_the_end(void **const _) {
  DARRAY_DEFINE(int, darray, memory_manage);
  int const first_object = 1;
  int const second_object = 2;
  DARRAY_PUSH(&darray, first_object);
  DARRAY_PUSH(&darray, second_object);

  int const first_popped_object = DARRAY_POP(&darray);
  int const second_popped_object = DARRAY_POP(&darray);

  assert_int_equal(first_popped_object, second_object);
  assert_int_equal(second_popped_object, first_object);

  DARRAY_DESTROY(&darray);
}

int main(void) {
  struct CMUnitTest const tests[] = {
    cmocka_unit_test(DARRAY_INIT_EXPLICIT__sets_all_members),
    cmocka_unit_test(DARRAY_INIT__sets_all_members_with_defaults),
    cmocka_unit_test(DARRAY_DEFINE__defines_with_defaults_for_members),
    cmocka_unit_test(DARRAY_DESTROY__releases_resources),
    cmocka_unit_test(DARRAY_DESTROY__sets_to_uninitialized_state),
    cmocka_unit_test(DARRAY_GROW__grows_by_initial_capacity_and_growth_factor),
    cmocka_unit_test(DARRAY_RESERVE__reserves_at_least_min_capacity),
    cmocka_unit_test(DARRAY_PUSH__pushes_objects_onto_the_end),
    cmocka_unit_test(DARRAY_POP__pops_objects_off_the_end),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
