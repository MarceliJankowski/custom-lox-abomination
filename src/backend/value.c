#include <assert.h>
#include <stdio.h>

#include "backend/gc.h"
#include "backend/value.h"

/**@desc initialize `value_array`*/
void value_array_init(ValueArray *const value_array) {
  assert(value_array != NULL);

  GC_DARRAY_INIT(value_array, values);
}

/**@desc append `value` to `value_array`*/
void value_array_append(ValueArray *const value_array, Value const value) {
  assert(value_array != NULL);

  GC_DARRAY_APPEND(value_array, values, value);
}

/**@desc free `value_array` and restore it to its initial state*/
void value_array_free(ValueArray *const value_array) {
  assert(value_array != NULL);

  GC_FREE_ARRAY(ValueArray, value_array->values, value_array->capacity);
  value_array_init(value_array);
}

/**@desc print `value`*/
void value_print(Value const value) {
  printf("%g", value);
}
