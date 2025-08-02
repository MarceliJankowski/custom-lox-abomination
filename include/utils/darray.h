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

/**@desc construct dynamic array type storing data of `data_type`
@param data_type type of data to be housed in resulting dynamic array type
@result constructed dynamic array type*/
#define DARRAY_TYPE(data_type)                                         \
  struct {                                                             \
    data_type *data;                                                   \
    MemoryManagerFn *memory_manager;                                   \
    size_t data_object_size, capacity, count, initial_growth_capacity; \
    double capacity_growth_factor;                                     \
  }

/**@desc initialize `darray_ptr` explicitly, by setting all configurable members
@param darray_ptr pointer to dynamic array
@param data_obj_size size in bytes of objects that comprise data (must be positive)
@param memory_manager_ptr pointer to MemoryManagerFn
@param init_growth_capacity initial capacity in bytes to be allocated during first growth (must be positive)
@param growth_factor double specifying rate at which capacity should grow (must be greater than 1)*/
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

/**@desc initialize `darray_ptr` with default values for configurable members
@param darray_ptr pointer to dynamic array
@param data_obj_size size in bytes of objects that comprise data (must be positive)
@param memory_manager_ptr pointer to MemoryManagerFn*/
#define DARRAY_INIT(darray_ptr, data_obj_size, memory_manager_ptr)                         \
  DARRAY_INIT_EXPLICIT(                                                                    \
    darray_ptr, data_obj_size, memory_manager_ptr, DARRAY_DEFAULT_INITIAL_GROWTH_CAPACITY, \
    DARRAY_DEFAULT_CAPACITY_GROWTH_FACTOR                                                  \
  )

/**@desc define dynamic array
@param data_object_type type of objects that comprise dynamic array's data
@param darray_name name for dynamic array
@param memory_manager_ptr pointer to MemoryManagerFn*/
#define DARRAY_DEFINE(data_object_type, darray_name, memory_manager_ptr) \
  DARRAY_TYPE(data_object_type) darray_name;                             \
  DARRAY_INIT(&darray_name, sizeof(data_object_type), memory_manager_ptr)

/**@desc free `darray_ptr` memory and set it to uninitialized state
@param darray_ptr pointer to dynamic array*/
#define DARRAY_DESTROY(darray_ptr)                                                                              \
  do {                                                                                                          \
    assert((darray_ptr) != NULL);                                                                               \
                                                                                                                \
    /* free memory */                                                                                           \
    memory_deallocate(                                                                                          \
      (darray_ptr)->memory_manager, (darray_ptr)->data, (darray_ptr)->data_object_size * (darray_ptr)->capacity \
    );                                                                                                          \
                                                                                                                \
    /* set to uninitialized state */                                                                            \
    memset((darray_ptr), 0, sizeof(*(darray_ptr)));                                                             \
  } while (0)

/**@desc get `darray_ptr` next capacity
@param darray_ptr pointer to dynamic array
@result `darray_ptr` next capacity*/
#define DARRAY__GET_NEXT_CAPACITY(darray_ptr)                                                      \
  ((assert((darray_ptr) != NULL)), ((darray_ptr)->capacity < (darray_ptr)->initial_growth_capacity \
                                      ? (darray_ptr)->initial_growth_capacity                      \
                                      : (darray_ptr)->capacity * (darray_ptr)->capacity_growth_factor))

/**@desc resize `darray_ptr` to `new_capacity`
@param darray_ptr pointer to dynamic array
@param new_capacity capacity in bytes (must be non-negative)*/
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

/**@desc grow `darray_ptr` to next capacity
@param darray_ptr pointer to dynamic array*/
#define DARRAY_GROW(darray_ptr) DARRAY__RESIZE(darray_ptr, DARRAY__GET_NEXT_CAPACITY(darray_ptr))

/**@desc resize `darray_ptr` to at least `min_capacity`
@param darray_ptr pointer to dynamic array
@param min_capacity minimum capacity in bytes (must be positive)*/
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

/**@desc push `data_object` onto `darray_ptr`
@param darray_ptr pointer to dynamic array
@param data_object object to be pushed*/
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

/**@desc pop last data object from `darray_ptr` (count must be positive)
@param darray_ptr pointer to dynamic array
@result popped data object*/
#define DARRAY_POP(darray_ptr)                                                                                   \
  ((assert((darray_ptr) != NULL), assert((darray_ptr)->count > 0 && "Attempt to pop from empty dynamic array")), \
   (darray_ptr)->data[--(darray_ptr)->count])

#endif // DARRAY_H
