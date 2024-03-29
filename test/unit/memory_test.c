#include <criterion/criterion.h>

#include "util/memory.h"

Test(get_byte, retrieve_bytes_in_LSB_to_MSB_order) {
  uint8_t const MSB = 1;
  uint8_t const LSB = 0;
  long unsigned const object = ((0u | MSB) << CHAR_BIT) | LSB;

  uint8_t const retrieved_first_byte = get_byte(object, 0);
  uint8_t const retrieved_second_byte = get_byte(object, 1);

  cr_expect_eq(retrieved_first_byte, LSB);
  cr_expect_eq(retrieved_second_byte, MSB);
}

Test(concatenate_bytes, concatenate_bytes_in_MSB_to_LSB_order) {
  unsigned long const object = 0x010203;
  uint8_t const LSB = (uint8_t)object;
  uint8_t const middle_byte = (uint8_t)(object >> CHAR_BIT);
  uint8_t const MSB = (uint8_t)(object >> 2 * CHAR_BIT);

  unsigned long const result = concatenate_bytes(3, MSB, middle_byte, LSB);

  cr_expect_eq(result, object);
}
