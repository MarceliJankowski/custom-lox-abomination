#ifndef STACK_H
#define STACK_H

#include "error.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define STACK_INIT(frame_type, stack_ptr, array_name, initial_capacity)        \
  do {                                                                         \
    (stack_ptr)->capacity = initial_capacity;                                  \
    (stack_ptr)->count = 0;                                                    \
    (stack_ptr)->array_name = malloc((initial_capacity) * sizeof(frame_type)); \
    if ((stack_ptr)->array_name == NULL) ERROR_MEMORY("%s", strerror(errno));  \
  } while (0)

#define STACK_CAPACITY_GROWTH_FACTOR 2
#define STACK_RESIZE(stack_ptr, array_name, frame_size)                                             \
  do {                                                                                              \
    size_t const old_capacity = (stack_ptr)->capacity;                                              \
    (stack_ptr)->capacity = (old_capacity) * STACK_CAPACITY_GROWTH_FACTOR;                          \
    (stack_ptr)->array_name = realloc((stack_ptr)->array_name, (stack_ptr)->capacity * frame_size); \
    if ((stack_ptr)->array_name == NULL) ERROR_MEMORY("%s", strerror(errno));                       \
  } while (0)

#define STACK_PUSH(stack_ptr, array_name, frame)                                                         \
  do {                                                                                                   \
    /* check if stack needs resizing */                                                                  \
    if ((stack_ptr)->count == (stack_ptr)->capacity) STACK_RESIZE(stack_ptr, array_name, sizeof(frame)); \
    /* push frame */                                                                                     \
    (stack_ptr)->array_name[(stack_ptr)->count] = frame;                                                 \
    (stack_ptr)->count++;                                                                                \
  } while (0)

#define STACK_POP(stack_ptr, array_name, object_ptr)                             \
  do {                                                                           \
    assert((stack_ptr)->count > 0 && "Attempt to pop frame off an empty stack"); \
    (stack_ptr)->count--;                                                        \
    *(object_ptr) = (stack_ptr)->array_name[(stack_ptr)->count];                 \
  } while (0)

#define STACK_TOP_FRAME(stack_ptr, array_name) (stack_ptr)->array_name[(stack_ptr)->count - 1]

#define STACK_FREE(stack_ptr, array_name) \
  do {                                    \
    free((stack_ptr)->array_name);        \
    (stack_ptr)->capacity = 0;            \
    (stack_ptr)->count = 0;               \
    (stack_ptr)->array_name = NULL;       \
  } while (0)

#endif // STACK_H
