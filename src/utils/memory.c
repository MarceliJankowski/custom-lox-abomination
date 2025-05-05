#include "utils/memory.h"

#include "utils/error.h"

#include <stdarg.h>
#include <stdlib.h>

void *memory_allocate(MemoryManagerFn *memory_manager, size_t new_size);
void *memory_reallocate(MemoryManagerFn *memory_manager, void *object, size_t old_size, size_t new_size);
void *memory_deallocate(MemoryManagerFn *memory_manager, void *object, size_t old_size);
uint8_t memory_get_byte(uint32_t object, int index);

/**@desc standard MemoryManagerFn implementation
@see MemoryManagerFn for further documentation*/
void *memory_manage(void *const object, size_t const old_size, size_t const new_size) {
  assert(
    !(object == NULL && old_size > 0 && new_size != 0) && "Invalid operation; can't allocate object with existing size"
  );
  assert(
    !(object != NULL && old_size == new_size && new_size != 0) &&
    "Invalid operation; can't determine whether to shrink or expand object"
  );
  assert(
    !(object != NULL && old_size == 0 && new_size != 0) &&
    "Invalid operation; existing object of size 0 can only be deallocated"
  );

  if (new_size == 0) {
    free(object);
    return NULL;
  }

  void *const reallocated_object = realloc(object, new_size);
  if (reallocated_object == NULL) ERROR_MEMORY_ERRNO();

  return reallocated_object;
}

/**@desc concatenate `byte_count` uint8_t `bytes`; `bytes` go from MSB to LSB
@return uint32_t formed from `bytes` concatenation*/
uint32_t memory_concatenate_bytes(int byte_count, ...) {
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
