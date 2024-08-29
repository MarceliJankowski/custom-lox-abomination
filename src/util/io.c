#include "util/io.h"

#include "frontend/lexer.h"
#include "util/darray.h"
#include "util/error.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

/**@desc read `binary_stream` resource content into dynamically allocated buffer
@return pointer to NUL terminated buffer with `binary_stream` resource content*/
void *read_binary_stream_resource_content(FILE *const binary_stream) {
  assert(binary_stream != NULL);

  // get binary_stream resource content size
  if (fseek(binary_stream, 0, SEEK_END)) IO_ERROR("%s", strerror(errno));
  errno = 0;
  size_t const content_size = ftell(binary_stream);
  if (errno) IO_ERROR("%s", strerror(errno));
  if (fseek(binary_stream, 0, SEEK_SET)) IO_ERROR("%s", strerror(errno));

  // allocate binary_stream resource content buffer
  char *const content_buffer = malloc(content_size + 1); // account for NUL terminator
  if (content_buffer == NULL) MEMORY_ERROR("%s", strerror(errno));

  // read binary_stream resource content into allocated buffer
  size_t const bytes_read = fread(content_buffer, 1, content_size, binary_stream);
  if (bytes_read < content_size) IO_ERROR("%s", strerror(errno));
  content_buffer[bytes_read] = '\0';

  return content_buffer;
}

/**@desc read file at `filepath` into dynamically allocated buffer
@return pointer to NUL terminated buffer with `filepath` file content*/
char *read_file(char const *const filepath) {
  assert(filepath != NULL);

  // open file stream
  FILE *const file_stream = fopen(filepath, "rb");
  if (file_stream == NULL) IO_ERROR("Failed to open file '%s'" M_S "%s", filepath, strerror(errno));

  // read file into buffer
  char *const file_buffer = read_binary_stream_resource_content(file_stream);

  // clean up
  if (fclose(file_stream)) IO_ERROR("Failed to close file '%s'" M_S "%s", filepath, strerror(errno));

  return file_buffer;
}
