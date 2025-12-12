#ifndef DARRAY_H
#define DARRAY_H

#include "error.h"
#include "memory.h"

#include <stdlib.h>

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

#define DARRAY_DEFAULT_INITIAL_GROWTH_CAPACITY 8
#define DARRAY_DEFAULT_CAPACITY_GROWTH_FACTOR 2

/// Construct dynamic array type storing data of `data_type`.
/// @param data_type Type of data to be housed in resulting dynamic array type.
/// @result constructed dynamic array type.
#define DARRAY_TYPE(data_type)                                         \
  struct {                                                             \
    data_type *data;                                                   \
    MemoryManagerFn *memory_manager;                                   \
    size_t data_object_size, capacity, count, initial_growth_capacity; \
    double capacity_growth_factor;                                     \
  }

/// Initialize `darray_ptr` explicitly, by setting all configurable members.
/// @param darray_ptr Pointer to dynamic array.
/// @param data_obj_size Size in bytes of objects that comprise data (must be positive).
/// @param memory_manager_ptr Pointer to MemoryManagerFn.
/// @param init_growth_capacity Initial data object capacity to be allocated during first growth (must be positive).
/// @param growth_factor Double specifying rate at which capacity should grow (must be greater than 1).
#define DARRAY_INIT_EXPLICIT(darray_ptr, data_obj_size, memory_manager_ptr, init_growth_capacity, growth_factor) \
  do {                                                                                                           \
    assert((darray_ptr) != NULL);                                                                                \
    assert(((volatile size_t)data_obj_size) > 0); /* volatile cast suppresses 'suspicious comparison' warning */ \
    assert((memory_manager_ptr) != NULL);                                                                        \
    assert((init_growth_capacity) > 0);                                                                          \
    assert((growth_factor) > 1);                                                                                 \
                                                                                                                 \
    (darray_ptr)->data = NULL;                                                                                   \
    (darray_ptr)->capacity = 0;                                                                                  \
    (darray_ptr)->count = 0;                                                                                     \
    (darray_ptr)->data_object_size = data_obj_size;                                                              \
    (darray_ptr)->initial_growth_capacity = init_growth_capacity; /* used for lazy allocation */                 \
    (darray_ptr)->capacity_growth_factor = growth_factor;                                                        \
    (darray_ptr)->memory_manager = memory_manager_ptr;                                                           \
  } while (0)

/// Initialize `darray_ptr` with default values for configurable members.
/// @param darray_ptr Pointer to dynamic array.
/// @param data_obj_size Size in bytes of objects that comprise data (must be positive).
/// @param memory_manager_ptr Pointer to MemoryManagerFn.
#define DARRAY_INIT(darray_ptr, data_obj_size, memory_manager_ptr)                         \
  DARRAY_INIT_EXPLICIT(                                                                    \
    darray_ptr, data_obj_size, memory_manager_ptr, DARRAY_DEFAULT_INITIAL_GROWTH_CAPACITY, \
    DARRAY_DEFAULT_CAPACITY_GROWTH_FACTOR                                                  \
  )

/// Define dynamic array.
/// @param data_object_type Type of objects that comprise dynamic array's data.
/// @param darray_name Name for dynamic array.
/// @param memory_manager_ptr Pointer to MemoryManagerFn.
#define DARRAY_DEFINE(data_object_type, darray_name, memory_manager_ptr) \
  DARRAY_TYPE(data_object_type) darray_name;                             \
  DARRAY_INIT(&darray_name, sizeof(data_object_type), memory_manager_ptr)

/// Release `darray_ptr` resources and set it to uninitialized state.
/// @param darray_ptr Pointer to initialized dynamic array.
#define DARRAY_DESTROY(darray_ptr)                                                                              \
  do {                                                                                                          \
    assert((darray_ptr) != NULL);                                                                               \
                                                                                                                \
    memory_deallocate(                                                                                          \
      (darray_ptr)->memory_manager, (darray_ptr)->data, (darray_ptr)->data_object_size * (darray_ptr)->capacity \
    );                                                                                                          \
                                                                                                                \
    memset((darray_ptr), 0, sizeof(*(darray_ptr)));                                                             \
  } while (0)

/// Get `darray_ptr` next capacity.
/// @param darray_ptr Pointer to initialized dynamic array.
/// @result `darray_ptr` next capacity.
#define DARRAY__GET_NEXT_CAPACITY(darray_ptr)                                                      \
  ((assert((darray_ptr) != NULL)), ((darray_ptr)->capacity < (darray_ptr)->initial_growth_capacity \
                                      ? (darray_ptr)->initial_growth_capacity                      \
                                      : (darray_ptr)->capacity * (darray_ptr)->capacity_growth_factor))

/// Resize `darray_ptr` to `new_capacity`.
/// @param darray_ptr Pointer to initialized dynamic array.
/// @param new_capacity Data object capacity (must be non-negative).
#define DARRAY__RESIZE(darray_ptr, new_capacity)                                                                 \
  do {                                                                                                           \
    assert((darray_ptr) != NULL);                                                                                \
    assert(                                                                                                      \
      new_capacity == 0 || new_capacity > 0                                                                      \
    ); /* logical 'or' suppresses 'comparison of unsigned expression is always true' warning */                  \
                                                                                                                 \
    (darray_ptr)->data = memory_reallocate(                                                                      \
      (darray_ptr)->memory_manager, (darray_ptr)->data, (darray_ptr)->data_object_size * (darray_ptr)->capacity, \
      (darray_ptr)->data_object_size * new_capacity                                                              \
    );                                                                                                           \
    if ((darray_ptr)->data == NULL) ERROR_MEMORY_ERRNO();                                                        \
    (darray_ptr)->capacity = new_capacity;                                                                       \
  } while (0)

/// Grow `darray_ptr` to next capacity.
/// @param darray_ptr Pointer to initialized dynamic array.
#define DARRAY_GROW(darray_ptr) DARRAY__RESIZE(darray_ptr, DARRAY__GET_NEXT_CAPACITY(darray_ptr))

/// Resize `darray_ptr` to at least `min_capacity`.
/// @param darray_ptr Pointer to initialized dynamic array.
/// @param min_capacity Minimum data object capacity (must be positive).
#define DARRAY_RESERVE(darray_ptr, min_capacity)                      \
  do {                                                                \
    assert((darray_ptr) != NULL);                                     \
    assert((min_capacity) > 0);                                       \
                                                                      \
    if ((darray_ptr)->capacity >= min_capacity) break;                \
                                                                      \
    /* grow capacity until it meets/exceeds min_capacity */           \
    size_t const original_capacity = (darray_ptr)->capacity;          \
    do {                                                              \
      (darray_ptr)->capacity = DARRAY__GET_NEXT_CAPACITY(darray_ptr); \
    } while ((darray_ptr)->capacity < min_capacity);                  \
                                                                      \
    /* restore original capacity while saving the new one */          \
    size_t const new_capacity = (darray_ptr)->capacity;               \
    (darray_ptr)->capacity = original_capacity;                       \
                                                                      \
    DARRAY__RESIZE((darray_ptr), new_capacity);                       \
  } while (0)

/// Push `data_object` onto `darray_ptr`.
/// @param darray_ptr Pointer to initialized dynamic array.
/// @param data_object Object to be pushed.
#define DARRAY_PUSH(darray_ptr, data_object)                                    \
  do {                                                                          \
    assert((darray_ptr) != NULL);                                               \
                                                                                \
    /* grow if needed */                                                        \
    if ((darray_ptr)->count == (darray_ptr)->capacity) DARRAY_GROW(darray_ptr); \
                                                                                \
    /* push data_object */                                                      \
    (darray_ptr)->data[(darray_ptr)->count] = data_object;                      \
    (darray_ptr)->count++;                                                      \
  } while (0)

/// Pop last data object from `darray_ptr` (count must be positive).
/// @param darray_ptr Pointer to initialized dynamic array>
/// @result Popped data object.
#define DARRAY_POP(darray_ptr)                                                                                   \
  ((assert((darray_ptr) != NULL), assert((darray_ptr)->count > 0 && "Attempt to pop from empty dynamic array")), \
   (darray_ptr)->data[--(darray_ptr)->count])

#endif // DARRAY_H
