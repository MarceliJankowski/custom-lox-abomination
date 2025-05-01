#ifndef MEMORY_H
#define MEMORY_H

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#define MEMORY_BYTE_STATE_COUNT (UCHAR_MAX + 1)

typedef enum { MEMORY_LITTLE_ENDIAN, MEMORY_BIG_ENDIAN } Endianness;

/**@desc function managing `object` memory.
Based on input arguments one of the following operations is performed:
- Allocate new object (`object` == NULL, `old_size` == 0, `new_size` > 0);
- Expand `object` (`object` != NULL, `old_size` > 0, `new_size` > `old_size`);
- Shrink `object` (`object` != NULL, `old_size` > 0, `new_size` < `old_size`);
- Deallocate `object` (`object` != NULL || `object` == NULL, `old_size` >= 0, `new_size` == 0);
Deallocation of non-existing objects is permitted for convenience.
@return pointer to (re)allocated `object` or NULL if deallocation was performed*/
typedef void *(MemoryManagerFn)(void *object, size_t old_size, size_t new_size);

MemoryManagerFn memory_manage;
uint32_t memory_concatenate_bytes(int byte_count, ...);

/**@desc use `memory_manager` to allocate object of `new_size`*/
inline void *memory_allocate(MemoryManagerFn *const memory_manager, size_t const new_size) {
  return memory_manager(NULL, 0, new_size);
}

/**@desc use `memory_manager` to reallocate `object` of `old_size` to `new_size`*/
inline void *memory_reallocate(
  MemoryManagerFn *const memory_manager, void *const object, size_t const old_size, size_t const new_size
) {
  return memory_manager(object, old_size, new_size);
}

/**@desc use `memory_manager` to deallocate `object` of `old_size`*/
inline void *memory_deallocate(MemoryManagerFn *const memory_manager, void *const object, size_t const old_size) {
  return memory_manager(object, old_size, 0);
}

/**@desc get `object` byte located at `index`; `index` goes from LSB to MSB
@return byte located at `index`*/
inline uint8_t memory_get_byte(uint32_t const object, int const index) {
  assert(index >= 0 && "Expected index to be nonnegative");
  assert((size_t)index < sizeof(object) && "Expected index to fit within object (out of bounds)");

  return object >> CHAR_BIT * index;
}

#endif // MEMORY_H
