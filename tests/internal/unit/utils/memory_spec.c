#include "unit/unit_test.h"
#include "utils/memory.h"

// *---------------------------------------------*
// *                   MOCKS                     *
// *---------------------------------------------*

void __wrap_free(void *__ptr) {
  check_expected_ptr(__ptr);
}

void *__wrap_realloc(void *__ptr, size_t __size) {
  check_expected_ptr(__ptr);
  check_expected(__size);
  return mock_ptr_type(char *);
}

// *---------------------------------------------*
// *                 TEST CASES                  *
// *---------------------------------------------*

static void get_byte__retrieves_bytes_in_LSB_to_MSB_order(void **const _) {
  uint8_t const MSB = 1;
  uint8_t const LSB = 0;
  uint32_t const object = ((0u | MSB) << CHAR_BIT) | LSB;

  uint8_t const retrieved_first_byte = memory_get_byte(object, 0);
  uint8_t const retrieved_second_byte = memory_get_byte(object, 1);

  assert_int_equal(retrieved_first_byte, LSB);
  assert_int_equal(retrieved_second_byte, MSB);
}

static void concatenate_bytes__concatenates_bytes_in_MSB_to_LSB_order(void **const _) {
  uint32_t const object = 0x010203;
  uint8_t const LSB = (uint8_t)object;
  uint8_t const middle_byte = (uint8_t)(object >> CHAR_BIT);
  uint8_t const MSB = (uint8_t)(object >> 2 * CHAR_BIT);

  uint32_t const result = memory_concatenate_bytes(3, MSB, middle_byte, LSB);

  assert_true(result == object);
}

static void memory_allocate__test(void **const _) {
  expect_value(__wrap_realloc, __size, sizeof(char));
  expect_value(__wrap_realloc, __ptr, NULL);
  char i = 1;
  char *p = &i;
  will_return(__wrap_realloc, p);

  char *ptr = memory_allocate(memory_manage, sizeof(char));
  assert_ptr_equal(ptr, p);
}

static void memory_reallocate__test(void **const _) {
  expect_value(__wrap_realloc, __size, sizeof(char));
  expect_value(__wrap_realloc, __ptr, NULL);
  char i = 1;
  char *p = &i;
  will_return(__wrap_realloc, p);

  char *ptr = memory_allocate(memory_manage, sizeof(char));
  assert_ptr_equal(ptr, p);

  expect_value(__wrap_realloc, __size, sizeof(char) * 2);
  expect_value(__wrap_realloc, __ptr, p);
  will_return(__wrap_realloc, p);

  char *ptr2 = memory_reallocate(memory_manage, p, sizeof(char), sizeof(char) * 2);
  assert_ptr_equal(ptr2, p);
}

static void memory_deallocate__test(void **const _) {
  expect_value(__wrap_realloc, __size, sizeof(char));
  expect_value(__wrap_realloc, __ptr, NULL);
  char i = 1;
  char *p = &i;
  will_return(__wrap_realloc, p);

  char *ptr = memory_allocate(memory_manage, sizeof(char));
  assert_ptr_equal(ptr, p);

  expect_value(__wrap_free, __ptr, ptr);
  memory_deallocate(memory_manage, ptr, sizeof(char));
}

int main(void) {
  struct CMUnitTest const tests[] = {
    cmocka_unit_test(get_byte__retrieves_bytes_in_LSB_to_MSB_order),
    cmocka_unit_test(concatenate_bytes__concatenates_bytes_in_MSB_to_LSB_order),
    cmocka_unit_test(memory_allocate__test),
    cmocka_unit_test(memory_reallocate__test),
    cmocka_unit_test(memory_deallocate__test),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
