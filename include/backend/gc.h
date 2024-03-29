#ifndef GC_H
#define GC_H

#include <stddef.h>

#define GC_DARRAY_INIT(darray_ptr, array_name) \
  do {                                         \
    (darray_ptr)->capacity = 0;                \
    (darray_ptr)->count = 0;                   \
    (darray_ptr)->array_name = NULL;           \
  } while (0)

#define GC_DARRAY_MIN_CAPACITY 8
#define GC_DARRAY_CAPACITY_GROWTH_FACTOR 2
#define GC_DARRAY_GROW_CAPACITY(capacity)                       \
  ((capacity) < GC_DARRAY_MIN_CAPACITY ? GC_DARRAY_MIN_CAPACITY \
                                       : (capacity) * GC_DARRAY_CAPACITY_GROWTH_FACTOR)

#define GC_DARRAY_RESIZE(darray_ptr, array_name, element_size)                                           \
  do {                                                                                                   \
    size_t const old_capacity = (darray_ptr)->capacity;                                                  \
    (darray_ptr)->capacity = GC_DARRAY_GROW_CAPACITY(old_capacity);                                      \
    (darray_ptr)->array_name = gc_manage_memory(                                                         \
      (darray_ptr)->array_name, (element_size) * (old_capacity), (element_size) * (darray_ptr)->capacity \
    );                                                                                                   \
  } while (0)

#define GC_DARRAY_APPEND(darray_ptr, array_name, element)        \
  do {                                                           \
    /* check if dynamic array needs resizing */                  \
    if ((darray_ptr)->count == (darray_ptr)->capacity)           \
      GC_DARRAY_RESIZE(darray_ptr, array_name, sizeof(element)); \
                                                                 \
    /* append element */                                         \
    (darray_ptr)->array_name[(darray_ptr)->count] = element;     \
    (darray_ptr)->count++;                                       \
  } while (0)

#define GC_FREE_ARRAY(element_type, array_ptr, length) \
  gc_manage_memory(array_ptr, sizeof(element_type) * (length), 0)

void *gc_manage_memory(void *object, size_t old_size, size_t new_size);

#endif // GC_H
