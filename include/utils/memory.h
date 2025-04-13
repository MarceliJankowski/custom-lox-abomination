#ifndef MEMORY_H
#define MEMORY_H

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#define MEMORY_BYTE_STATE_COUNT (UCHAR_MAX + 1)

typedef enum { MEMORY_LITTLE_ENDIAN, MEMORY_BIG_ENDIAN } Endianness;

/**@desc get `object` byte located at `index`; `index` goes from LSB to MSB
@return byte located at `index`*/
inline uint8_t memory_get_byte(uint32_t const object, int const index) {
  assert(index >= 0 && "Expected index to be nonnegative");
  assert((size_t)index < sizeof(object) && "Expected index to fit within object (out of bounds)");

  return object >> CHAR_BIT * index;
}

uint32_t memory_concatenate_bytes(int byte_count, ...);

#endif // MEMORY_H
