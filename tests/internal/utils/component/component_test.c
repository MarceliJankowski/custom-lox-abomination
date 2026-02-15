#include "component_test.h"

#include "backend/object.h"
#include "utils/error.h"
#include "utils/io.h"
#include "utils/number.h"

// *---------------------------------------------*
// *         EXTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/// Assert `file_bin_stream` file content being equal to `expected_content`.
/// @param file_bin_stream Binary stream connected to a file.
/// @param expected_content String with expected `file_bin_stream` file content.
void component_test_assert_file_content(FILE *const file_bin_stream, char const *const expected_content) {
  assert(file_bin_stream != NULL);
  assert(expected_content != NULL);

  if (fflush(file_bin_stream)) ERROR_IO_ERRNO();

  char *const content_string = io_read_finite_seekable_binary_stream_as_str(file_bin_stream);

  assert_string_equal(content_string, expected_content);

  free(content_string);
}

/// Assert `value_a` and `value_b` equality.
void component_test_assert_value_equality(Value const value_a, Value const value_b) {
  assert_int_equal(value_a.type, value_b.type);

  static_assert(VALUE_TYPE_COUNT == 4, "Exhaustive ValueType handling");
  switch (value_a.type) {
    case VALUE_NIL: {
      break;
    }
    case VALUE_BOOL: {
      assert_true(value_a.as.boolean == value_b.as.boolean);
      break;
    }
    case VALUE_NUMBER: {
      if (number_is_integer(value_a.as.number) && number_is_integer(value_b.as.number)) {
        assert_double_equal(value_a.as.number, value_b.as.number, 0);
      } else {
        assert_double_equal(value_a.as.number, value_b.as.number, 1e-9);
      }
      break;
    }
    case VALUE_OBJECT: {
      static_assert(OBJECT_TYPE_COUNT == 2, "Exhaustive ObjectType handling");
      switch (value_a.as.object->type) {
        case OBJECT_STRING:
        case OBJECT_STRING_LITERAL: {
          ObjectString const *const string_object_a = (ObjectString *)value_a.as.object;
          ObjectString const *const string_object_b = (ObjectString *)value_b.as.object;

          assert_int_equal(string_object_a->length, string_object_b->length);
          assert_ptr_equal(string_object_a->content, string_object_b->content);
          break;
        }
        default: ERROR_INTERNAL("Unknown ObjectType '%d'", value_a.type);
      }
      break;
    }
    default: ERROR_INTERNAL("Unexpected ValueType '%d'", value_a.type);
  }
}
