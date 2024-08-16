#ifndef GC_H
#define GC_H

#include "util/darray.h"

#include <stddef.h>

#define GC_DARRAY_INIT DARRAY_INIT

#define GC_DARRAY_RESIZE(darray_ptr, array_name, element_size)                                           \
  do {                                                                                                   \
    size_t const old_capacity = (darray_ptr)->capacity;                                                  \
    (darray_ptr)->capacity = DARRAY_GROW_CAPACITY(old_capacity);                                         \
    (darray_ptr)->array_name = gc_manage_memory(                                                         \
      (darray_ptr)->array_name, (element_size) * (old_capacity), (element_size) * (darray_ptr)->capacity \
    );                                                                                                   \
  } while (0)

#define GC_DARRAY_APPEND(darray_ptr, array_name, element) \
  DARRAY_APPEND_BOILERPLATE(darray_ptr, array_name, element, GC_DARRAY_RESIZE)

#define GC_FREE_ARRAY(element_type, array_ptr, length) gc_manage_memory(array_ptr, sizeof(element_type) * (length), 0)

void *gc_manage_memory(void *object, size_t old_size, size_t new_size);

#endif // GC_H
