#include "backend/value.h"

#include "backend/gc.h"
#include "backend/object.h"
#include "utils/error.h"
#include "utils/io.h"
#include "utils/number.h"

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
bool value_is_string(Value value);

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

/// Create string object from `value`.
/// @note If `value` is of string type, it gets returned as is.
/// @return Created string object.
ObjectString *value_to_string_object(Value const value) {
  static_assert(VALUE_TYPE_COUNT == 4, "Exhaustive ValueType handling");
  switch (value.type) {
    case VALUE_NIL: {
      return object_make_non_owning_string("nil", 3);
    }
    case VALUE_BOOL: {
      if (value.as.boolean == true) return object_make_non_owning_string("true", 4);
      return object_make_non_owning_string("false", 5);
    }
    case VALUE_NUMBER: {
      char const *const format_specifier = "%g";

      char dummy_buffer[1]; // required by snprintf spec
      int const string_representation_length = snprintf(dummy_buffer, 0, format_specifier, value.as.number);
      if (string_representation_length < 0) ERROR_IO_ERRNO();
      size_t const string_representation_size = string_representation_length + 1; // account for NUL terminator

      char *string_representation = gc_allocate(string_representation_size);
      int const bytes_printed =
        snprintf(string_representation, string_representation_size, format_specifier, value.as.number);
      if (bytes_printed < 0 || bytes_printed != string_representation_length) ERROR_IO_ERRNO();

      // truncate redundant NUL terminator
      string_representation =
        gc_reallocate(string_representation, string_representation_size, string_representation_length);

      return object_make_non_owning_string(string_representation, string_representation_length);
    }
    case VALUE_OBJECT: {
      static_assert(OBJECT_TYPE_COUNT == 1, "Exhaustive ObjectType handling");
      switch (value.as.object->type) {
        case OBJECT_STRING: return (ObjectString *)value.as.object;

        default: ERROR_INTERNAL("Unknown ObjectType '%d'", value.as.object->type);
      }
    }

    default: ERROR_INTERNAL("Unknown ValueType '%d'", value.type);
  }
}
