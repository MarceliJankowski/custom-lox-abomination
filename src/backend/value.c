#include "backend/value.h"

#include "backend/gc.h"
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

/**@desc initialize `value_list`*/
void value_list_init(ValueList *const value_list) {
  assert(value_list != NULL);

  DARRAY_INIT(value_list, sizeof(Value), gc_memory_manage);
}

/**@desc free `value_list`*/
void value_list_free(ValueList *const value_list) {
  assert(value_list != NULL);

  DARRAY_FREE(value_list);
}

/**@desc append `value` to `value_list`*/
void value_list_append(ValueList *const value_list, Value const value) {
  assert(value_list != NULL);

  DARRAY_PUSH(value_list, value);
}

/**@desc print `value`*/
void value_print(Value const value) {
  static_assert(VALUE_TYPE_COUNT == 3, "Exhaustive ValueType handling");
  switch (value.type) {
    case VALUE_BOOL: IO_PRINTF_BREAK(value.payload.boolean ? "true" : "false");
    case VALUE_NIL: IO_PRINTF_BREAK("nil");
    case VALUE_NUMBER: IO_PRINTF_BREAK("%g", value.payload.number);

    default: ERROR_INTERNAL("Unknown ValueType '%d'", value.type);
  }
}

/**@desc determine whether `value_a` equals `value_b`
@return true if it does, false otherwise*/
bool value_equals(Value const value_a, Value const value_b) {
  if (value_a.type != value_b.type) return false;

  static_assert(VALUE_TYPE_COUNT == 3, "Exhaustive ValueType handling");
  switch (value_a.type) {
    case VALUE_NIL: return true;
    case VALUE_BOOL: return value_a.payload.boolean == value_b.payload.boolean;
    case VALUE_NUMBER: return value_a.payload.number == value_b.payload.number;

    default: ERROR_INTERNAL("Unknown ValueType '%d'", value_a.type);
  }
}
