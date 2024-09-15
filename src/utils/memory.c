#include "utils/memory.h"

#include <stdarg.h>
#include <stdbool.h>

uint8_t get_byte(uint32_t object, int index);

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
