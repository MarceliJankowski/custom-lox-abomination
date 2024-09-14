#ifndef DARRAY_H
#define DARRAY_H

#include <stdlib.h>

#define DARRAY_INIT(darray_ptr, array_name) \
  do {                                      \
    (darray_ptr)->capacity = 0;             \
    (darray_ptr)->count = 0;                \
    (darray_ptr)->array_name = NULL;        \
  } while (0)

#define DARRAY_MIN_CAPACITY 8
#define DARRAY_CAPACITY_GROWTH_FACTOR 2
#define DARRAY_GROW_CAPACITY(capacity) \
  ((capacity) < DARRAY_MIN_CAPACITY ? DARRAY_MIN_CAPACITY : (capacity) * DARRAY_CAPACITY_GROWTH_FACTOR)

#define DARRAY_RESIZE(darray_ptr, array_name, element_size)                                                \
  do {                                                                                                     \
    (darray_ptr)->capacity = DARRAY_GROW_CAPACITY((darray_ptr)->capacity);                                 \
    (darray_ptr)->array_name = realloc((darray_ptr)->array_name, (element_size) * (darray_ptr)->capacity); \
  } while (0)

#define DARRAY_APPEND_BOILERPLATE(darray_ptr, array_name, element, darray_resize_macro)                              \
  do {                                                                                                               \
    /* check if dynamic array needs resizing */                                                                      \
    if ((darray_ptr)->count == (darray_ptr)->capacity) darray_resize_macro(darray_ptr, array_name, sizeof(element)); \
                                                                                                                     \
    /* append element */                                                                                             \
    (darray_ptr)->array_name[(darray_ptr)->count] = element;                                                         \
    (darray_ptr)->count++;                                                                                           \
  } while (0)

#define DARRAY_APPEND(darray_ptr, array_name, element) \
  DARRAY_APPEND_BOILERPLATE(darray_ptr, array_name, element, DARRAY_RESIZE)

#endif // DARRAY_H
