#include "component_test.h"

#include "utils/error.h"
#include "utils/io.h"
#include "utils/number.h"

#include <assert.h>

// *---------------------------------------------*
// *         EXTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/// Assert `binary_stream` resource content being equal to `expected_resource_content`.
void component_test_assert_binary_stream_resource_content(
  FILE *const binary_stream, char const *const expected_resource_content
) {
  assert(binary_stream != NULL);
  assert(expected_resource_content != NULL);

  if (fflush(binary_stream)) ERROR_IO_ERRNO();

  char *const resource_content = io_read_binary_stream_resource_content(binary_stream);

  assert_string_equal(resource_content, expected_resource_content);

  free(resource_content);
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
