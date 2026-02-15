#ifndef GC_H
#define GC_H

#include "utils/memory.h"

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

MemoryManagerFn gc_memory_manage;
void gc_deallocate_vm_gc_objects(void);

// *---------------------------------------------*
// *              INLINE FUNCTIONS               *
// *---------------------------------------------*

/// Allocate garbage-collected object of `new_size`.
inline void *gc_allocate(size_t const new_size) {
  return memory_allocate(gc_memory_manage, new_size);
}

/// Reallocate garbage-collected `object` of `old_size` to `new_size`.
inline void *gc_reallocate(void *const object, size_t const old_size, size_t const new_size) {
  return memory_reallocate(gc_memory_manage, object, old_size, new_size);
}

/// Deallocate garbage-collected `object` of `old_size`.
inline void *gc_deallocate(void *const object, size_t const old_size) {
  return memory_deallocate(gc_memory_manage, object, old_size);
}

#endif // GC_H
