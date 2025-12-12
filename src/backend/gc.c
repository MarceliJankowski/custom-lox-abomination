#include "backend/gc.h"

// *---------------------------------------------*
// *        EXTERNAL-LINKAGE FUNCTIONS           *
// *---------------------------------------------*

/// Garbage collecting MemoryManagerFn implementation.
/// @note Memory of objects tracked by garbage collector must be managed exclusively by this function (from the get-go).
/// @see MemoryManagerFn for further documentation.
void *gc_memory_manage(void *const object, size_t const old_size, size_t const new_size) {
  // TODO: implement garbage collecting

  return memory_manage(object, old_size, new_size);
}
