#include "component_test.h"

#include "utils/error.h"
#include "utils/io.h"
#include "utils/number.h"

#include <assert.h>

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

/// Clear `binary_stream` resource content.
void component_test_clear_binary_stream_resource_content(FILE *binary_stream) {
  assert(binary_stream != NULL);

  binary_stream = freopen(NULL, "w+b", binary_stream);
  if (binary_stream == NULL) ERROR_IO_ERRNO();
}

/// Assert `value_a` and `value_b` equality.
void component_test_assert_value_equality(Value const value_a, Value const value_b) {
  assert_int_equal(value_a.type, value_b.type);

  static_assert(VALUE_TYPE_COUNT == 3, "Exhaustive ValueType handling");
  switch (value_a.type) {
    case VALUE_NIL: {
      break;
    }
    case VALUE_BOOL: {
      assert_true(value_a.payload.boolean == value_b.payload.boolean);
      break;
    }
    case VALUE_NUMBER: {
      if (number_is_integer(value_a.payload.number) && number_is_integer(value_b.payload.number)) {
        assert_double_equal(value_a.payload.number, value_b.payload.number, 0);
      } else {
        assert_double_equal(value_a.payload.number, value_b.payload.number, 1e-9);
      }
      break;
    }
    default: ERROR_INTERNAL("Unknown ValueType '%d'", value_a.type);
  }
}
