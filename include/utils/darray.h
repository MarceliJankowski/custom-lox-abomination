#ifndef DARRAY_H
#define DARRAY_H

#include "error.h"
#include "memory.h"

#include <stdlib.h>

#define DARRAY_DEFAULT_MIN_GROWTH_CAPACITY 8
#define DARRAY_DEFAULT_CAPACITY_GROWTH_FACTOR 2

#define DARRAY_TYPE(data_type)                                     \
  struct {                                                         \
    data_type *data;                                               \
    MemoryManagerFn *memory_manager;                               \
    size_t data_object_size, capacity, count, min_growth_capacity; \
    double capacity_growth_factor;                                 \
  }

#define DARRAY_INIT(darray_ptr, data_obj_size, memory_manager_ptr)                \
  do {                                                                            \
    (darray_ptr)->data = NULL;                                                    \
    (darray_ptr)->capacity = 0;                                                   \
    (darray_ptr)->count = 0;                                                      \
    (darray_ptr)->data_object_size = data_obj_size;                               \
    (darray_ptr)->min_growth_capacity = DARRAY_DEFAULT_MIN_GROWTH_CAPACITY;       \
    (darray_ptr)->capacity_growth_factor = DARRAY_DEFAULT_CAPACITY_GROWTH_FACTOR; \
    (darray_ptr)->memory_manager = memory_manager_ptr;                            \
  } while (0)

#define DARRAY_DEFINE(data_type, darray_name, memory_manager_ptr) \
  DARRAY_TYPE(data_type) darray_name;                             \
  DARRAY_INIT(&darray_name, sizeof(data_type), memory_manager_ptr)

#define DARRAY_FREE(darray_ptr)                                                                                \
  do {                                                                                                         \
    memory_deallocate(                                                                                         \
      (darray_ptr)->memory_manager, (darray_ptr)->data, (darray_ptr)->data_object_size *(darray_ptr)->capacity \
    );                                                                                                         \
    DARRAY_INIT(darray_ptr, (darray_ptr)->data_object_size, (darray_ptr)->memory_manager);                     \
  } while (0)

#define DARRAY_GROW_CAPACITY(darray_ptr)                                   \
  do {                                                                     \
    (darray_ptr)->capacity =                                               \
      ((darray_ptr)->capacity < (darray_ptr)->min_growth_capacity          \
         ? (darray_ptr)->min_growth_capacity                               \
         : (darray_ptr)->capacity * (darray_ptr)->capacity_growth_factor); \
  } while (0)

#define DARRAY_RESIZE(darray_ptr)                                                                      \
  do {                                                                                                 \
    size_t const old_capacity = (darray_ptr)->capacity;                                                \
    DARRAY_GROW_CAPACITY(darray_ptr);                                                                  \
    (darray_ptr)->data = memory_reallocate(                                                            \
      (darray_ptr)->memory_manager, (darray_ptr)->data, (darray_ptr)->data_object_size * old_capacity, \
      (darray_ptr)->data_object_size * (darray_ptr)->capacity                                          \
    );                                                                                                 \
    if ((darray_ptr)->data == NULL) ERROR_MEMORY("%s", strerror(errno));                               \
  } while (0)

#define DARRAY_APPEND(darray_ptr, data_object)                                    \
  do {                                                                            \
    /* resize if needed */                                                        \
    if ((darray_ptr)->count == (darray_ptr)->capacity) DARRAY_RESIZE(darray_ptr); \
                                                                                  \
    /* append data_object */                                                      \
    (darray_ptr)->data[(darray_ptr)->count] = data_object;                        \
    (darray_ptr)->count++;                                                        \
  } while (0)

#endif // DARRAY_H
