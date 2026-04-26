#include "backend/value.h"

#include "backend/entity.h"
#include "backend/gc.h"
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
Value value_make_entity(Entity *entity);

bool value_is_bool(Value value);
bool value_is_nil(Value value);
bool value_is_number(Value value);
bool value_is_string(Value value);

bool value_is_falsy(Value value);

// *---------------------------------------------*
// *        EXTERNAL-LINKAGE FUNCTIONS           *
// *---------------------------------------------*

/// Get string with description of `value` kind.
/// @return Value kind string.
char const *value_get_kind_string(Value const value) {
  static_assert(VALUE_KIND_COUNT == 4, "Exhaustive ValueKind handling");
  switch (value.kind) {
    case VALUE_NIL: return "nil";
    case VALUE_BOOL: return "bool";
    case VALUE_NUMBER: return "number";
    case VALUE_ENTITY: return entity_get_kind_string(value.as.entity);

    default: ERROR_INTERNAL("Unknown ValueKind '%d'", value.kind);
  }
}

/// Initialize `value_list`.
void value_list_init(ValueList *const value_list) {
  assert(value_list != NULL);

  DARRAY_INIT(value_list, sizeof(Value), gc_memory_manage);
}

/// Release `value_list` resources and set it to uninitialized state.
/// @pre `value_list` is initialized.
void value_list_destroy(ValueList *const value_list) {
  assert(value_list != NULL);

  DARRAY_DESTROY(value_list);

  *value_list = (ValueList){0};
}

/// Append `value` to `value_list`.
/// @pre `value_list` is initialized.
void value_list_append(ValueList *const value_list, Value const value) {
  assert(value_list != NULL);

  DARRAY_PUSH(value_list, value);
}

/// Print `value`.
void value_print(Value const value) {
#define PRINTF(...) io_fprintf(g_source_program_output_stream, __VA_ARGS__);

  static_assert(VALUE_KIND_COUNT == 4, "Exhaustive ValueKind handling");
  switch (value.kind) {
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
    case VALUE_ENTITY: {
      entity_print(value.as.entity);
      break;
    }

    default: ERROR_INTERNAL("Unknown ValueKind '%d'", value.kind);
  }

#undef PRINTF
}

/// Determine whether `value_a` equals `value_b`.
/// @return true if it does, false otherwise.
bool value_equals(Value const value_a, Value const value_b) {
  if (value_a.kind != value_b.kind) return false;

  static_assert(VALUE_KIND_COUNT == 4, "Exhaustive ValueKind handling");
  switch (value_a.kind) {
    case VALUE_NIL: return true;
    case VALUE_BOOL: return value_a.as.boolean == value_b.as.boolean;
    case VALUE_NUMBER: return value_a.as.number == value_b.as.number;
    case VALUE_ENTITY: return entity_equals(value_a.as.entity, value_b.as.entity);

    default: ERROR_INTERNAL("Unknown ValueKind '%d'", value_a.kind);
  }
}

/// Create string entity from `value`.
/// @note If `value` is of string kind, it gets returned as is.
/// @return Created string entity.
EntityString *value_to_string_entity(Value const value) {
  static_assert(VALUE_KIND_COUNT == 4, "Exhaustive ValueKind handling");
  switch (value.kind) {
    case VALUE_NIL: {
      return entity_string_copy("nil", 3);
    }
    case VALUE_BOOL: {
      if (value.as.boolean == true) return entity_string_copy("true", 4);
      return entity_string_copy("false", 5);
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

      return entity_string_adopt(string_representation, string_representation_length);
    }
    case VALUE_ENTITY: {
      static_assert(ENTITY_KIND_COUNT == 1, "Exhaustive EntityKind handling");
      switch (value.as.entity->kind) {
        case ENTITY_STRING: return (EntityString *)value.as.entity;

        default: ERROR_INTERNAL("Unknown EntityKind '%d'", value.as.entity->kind);
      }
    }

    default: ERROR_INTERNAL("Unknown ValueKind '%d'", value.kind);
  }
}
