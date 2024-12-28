#include "backend/value.h"

#include "backend/gc.h"
#include "global.h"
#include "utils/error.h"
#include "utils/io.h"

#include <assert.h>
#include <stdio.h>

// *---------------------------------------------*
// *               STATIC OBJECTS                *
// *---------------------------------------------*

static_assert(VALUE_TYPE_COUNT == 3, "Exhaustive ValueType handling");
char const *const value_type_to_string_table[] = {
  [VALUE_NIL] = "nil",
  [VALUE_BOOL] = "bool",
  [VALUE_NUMBER] = "number",
};

// *---------------------------------------------*
// *               VALUE FUNCTIONS               *
// *---------------------------------------------*

/**@desc initialize `value_array`*/
void value_array_init(ValueArray *const value_array) {
  assert(value_array != NULL);

  GC_DARRAY_INIT(value_array, values);
}

/**@desc free `value_array`*/
void value_array_free(ValueArray *const value_array) {
  assert(value_array != NULL);

  GC_FREE_ARRAY(ValueArray, value_array->values, value_array->capacity);
}

/**@desc append `value` to `value_array`*/
void value_array_append(ValueArray *const value_array, Value const value) {
  assert(value_array != NULL);

  GC_DARRAY_APPEND(value_array, values, value);
}

/**@desc print `value`*/
void value_print(Value const value) {
  static_assert(VALUE_TYPE_COUNT == 3, "Exhaustive ValueType handling");
  switch (value.type) {
    case VALUE_BOOL: PRINTF_BREAK(value.payload.boolean ? "true" : "false");
    case VALUE_NIL: PRINTF_BREAK("nil");
    case VALUE_NUMBER: PRINTF_BREAK("%g", value.payload.number);

    default: INTERNAL_ERROR("Unknown ValueType '%d'", value.type);
  }
}
