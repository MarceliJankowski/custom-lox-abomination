#include "component_test_utils.h"

#include "utils/error.h"
#include "utils/io.h"

#include <assert.h>

/**@desc assert `binary_stream` resource content being equal to `expected_resource_content`*/
void assert_binary_stream_resource_content(FILE *const binary_stream, char const *const expected_resource_content) {
  assert(binary_stream != NULL);
  assert(expected_resource_content != NULL);

  if (fflush(binary_stream)) IO_ERROR("%s", strerror(errno));

  char *const resource_content = read_binary_stream_resource_content(binary_stream);

  assert_string_equal(resource_content, expected_resource_content);

  free(resource_content);
}
