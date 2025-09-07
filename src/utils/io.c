#include "utils/io.h"

#include "utils/error.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

int io_fprintf(FILE *const stream, char const *format, ...);
int io_printf(char const *format, ...);
void io_puts(char const *string);

// *---------------------------------------------*
// *         EXTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/**@desc read `binary_stream` resource content into dynamically allocated buffer
@return pointer to NUL terminated buffer with `binary_stream` resource content*/
char *io_read_binary_stream_resource_content(FILE *const binary_stream) {
  assert(binary_stream != NULL);

  // get binary_stream resource content size
  if (fseek(binary_stream, 0, SEEK_END)) ERROR_IO_ERRNO();
  errno = 0;
  size_t const content_size = ftell(binary_stream);
  if (errno) ERROR_IO_ERRNO();
  if (fseek(binary_stream, 0, SEEK_SET)) ERROR_IO_ERRNO();

  char *const content_buffer = malloc(content_size + 1); // account for NUL terminator
  if (content_buffer == NULL) ERROR_MEMORY_ERRNO();

  // read binary_stream resource content into allocated buffer
  size_t const bytes_read = fread(content_buffer, 1, content_size, binary_stream);
  if (bytes_read < content_size) ERROR_IO_ERRNO();
  content_buffer[bytes_read] = '\0';

  return content_buffer;
}

/**@desc read file at `filepath` into dynamically allocated buffer
@return pointer to NUL terminated buffer with `filepath` file content*/
char *io_read_file(char const *const filepath) {
  assert(filepath != NULL);

  FILE *const file_stream = fopen(filepath, "rb");
  if (file_stream == NULL) ERROR_IO("Failed to open file '%s'" COMMON_MS "%s\n", filepath, strerror(errno));

  char *const file_buffer = io_read_binary_stream_resource_content(file_stream);

  if (fclose(file_stream)) ERROR_IO("Failed to close file '%s'" COMMON_MS "%s\n", filepath, strerror(errno));

  return file_buffer;
}
