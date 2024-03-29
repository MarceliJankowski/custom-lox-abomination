#ifndef MEMORY_H
#define MEMORY_H

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

/**@desc get `object` byte located at `index`; `index` goes from LSB to MSB
@return byte located at `index`*/
inline uint8_t get_byte(unsigned long const object, int const index) {
  assert(index >= 0 && "Expected index to be nonnegative");
  assert((size_t)index < sizeof(object) && "Expected index to fit within object (out of bounds)");

  return object >> CHAR_BIT * index;
}

typedef enum { LITTLE_ENDIAN, BIG_ENDIAN } Endianness;

Endianness detect_endianness(void);
unsigned long concatenate_bytes(int byte_count, ...);

#endif // MEMORY_H
