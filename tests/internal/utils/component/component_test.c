#include "component_test.h"

#include "backend/object.h"
#include "backend/value.h"
#include "utils/error.h"
#include "utils/io.h"
#include "utils/number.h"

#include <cmocka.h>

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
  switch (value_a.kind) {
    case VALUE_NUMBER: { // special case due to floating point math
      assert_int_equal(value_a.kind, value_b.kind);
      if (number_is_integer(value_a.as.number) && number_is_integer(value_b.as.number)) {
        assert_double_equal(value_a.as.number, value_b.as.number, 0);
      } else {
        assert_double_equal(value_a.as.number, value_b.as.number, 1e-9);
      }
      break;
    }
    default: {
      assert_true(value_equals(value_a, value_b));
    }
  }
}
