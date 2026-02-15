#include "backend/value.h"

#include "backend/gc.h"
#include "utils/error.h"
#include "utils/io.h"

#include <assert.h>
#include <stdio.h>

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

Value value_make_nil(void);
Value value_make_bool(bool value);
Value value_make_number(double value);
Value value_make_object(Object *object);

bool value_is_bool(Value value);
bool value_is_nil(Value value);
bool value_is_number(Value value);

bool value_is_falsy(Value value);

// *---------------------------------------------*
// *        EXTERNAL-LINKAGE FUNCTIONS           *
// *---------------------------------------------*

/// Get string with description of `value` type.
/// @return Value type string.
char const *value_get_type_string(Value const value) {
  static_assert(VALUE_TYPE_COUNT == 4, "Exhaustive ValueType handling");
  switch (value.type) {
    case VALUE_NIL: return "nil";
    case VALUE_BOOL: return "bool";
    case VALUE_NUMBER: return "number";
    case VALUE_OBJECT: return object_get_type_string(value.as.object);

    default: ERROR_INTERNAL("Unknown ValueType '%d'", value.type);
  }
}

/// Initialize `value_list`.
void value_list_init(ValueList *const value_list) {
  assert(value_list != NULL);

  DARRAY_INIT(value_list, sizeof(Value), gc_memory_manage);
}

/// Release `value_list` resources and set it to uninitialized state.
void value_list_destroy(ValueList *const value_list) {
  assert(value_list != NULL);

  DARRAY_DESTROY(value_list);

  *value_list = (ValueList){0};
}

/// Append `value` to `value_list`.
void value_list_append(ValueList *const value_list, Value const value) {
  assert(value_list != NULL);

  DARRAY_PUSH(value_list, value);
}

/// Print `value`.
void value_print(Value const value) {
#define PRINTF(...) io_fprintf(g_source_program_output_stream, __VA_ARGS__);

  static_assert(VALUE_TYPE_COUNT == 4, "Exhaustive ValueType handling");
  switch (value.type) {
    case VALUE_BOOL: {
      PRINTF(value.as.boolean ? "true" : "false");
      break;
    }
    case VALUE_NIL: {
      PRINTF("nil");
      break;
    }
    case VALUE_NUMBER: {
      PRINTF("%g", value.as.number);
      break;
    }
    case VALUE_OBJECT: {
      object_print(value.as.object);
      break;
    }

    default: ERROR_INTERNAL("Unknown ValueType '%d'", value.type);
  }

#undef PRINTF
}

/// Determine whether `value_a` equals `value_b`.
/// @return true if it does, false otherwise.
bool value_equals(Value const value_a, Value const value_b) {
  if (value_a.type != value_b.type) return false;

  static_assert(VALUE_TYPE_COUNT == 4, "Exhaustive ValueType handling");
  switch (value_a.type) {
    case VALUE_NIL: return true;
    case VALUE_BOOL: return value_a.as.boolean == value_b.as.boolean;
    case VALUE_NUMBER: return value_a.as.number == value_b.as.number;
    case VALUE_OBJECT: return object_equals(value_a.as.object, value_b.as.object);

    default: ERROR_INTERNAL("Unknown ValueType '%d'", value_a.type);
  }
}
