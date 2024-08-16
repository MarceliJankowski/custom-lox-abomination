#include "util/memory.h"

#include <stdarg.h>
#include <stdbool.h>

uint8_t get_byte(uint32_t object, int index);

/**@desc detect architecture endianness
@return LITTLE_ENDIAN or BIG_ENDIAN*/
Endianness detect_endianness(void) {
  static Endianness endianness;
  static bool detected = false;

  if (detected == false) {
    unsigned int control_obj = 1;
    uint8_t *first_byte = (uint8_t *)&control_obj;

    if (*first_byte == 1) endianness = LITTLE_ENDIAN;
    else endianness = BIG_ENDIAN;

    detected = true;
  }

  return endianness;
}

/**@desc concatenate `byte_count` uint8_t `bytes`; `bytes` go from MSB to LSB
@return uint32_t formed from `bytes` concatenation*/
uint32_t concatenate_bytes(int byte_count, ...) {
  assert(byte_count >= 2 && "Expected at least 2 bytes to concatenate");
  assert(
    (size_t)byte_count <= sizeof(uint32_t) &&
    "Exceeded number of bytes that can be concatenated without causing overflow"
  );

  va_list bytes;
  va_start(bytes, byte_count);

  uint32_t accumulator = 0;
  for (; byte_count > 0; byte_count--) {
    uint8_t const byte = va_arg(bytes, unsigned int);
    accumulator = (accumulator << CHAR_BIT) | byte;
  }

  va_end(bytes);
  return accumulator;
}
