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

/// Read content of finite seekable binary `stream` resource into heap-allocated buffer.
/// @param stream Finite seekable binary stream.
/// @param out_length Object to be filled with content length.
/// @note Caller takes ownership of returned buffer.
/// @return Content buffer, or NULL if content is empty.
uint8_t *io_read_finite_seekable_binary_stream(FILE *const stream, size_t *const out_length) {
  assert(stream != NULL);
  assert(out_length != NULL);

  { // get content length
    if (fseek(stream, 0, SEEK_END)) ERROR_IO_ERRNO();
    long const ftell_size = ftell(stream);
    if (ftell_size < 0) ERROR_IO_ERRNO();
    if (fseek(stream, 0, SEEK_SET)) ERROR_IO_ERRNO();

    *out_length = ftell_size;
    if (*out_length == 0) return NULL;
  }

  uint8_t *const content_buffer = malloc(*out_length);
  if (content_buffer == NULL) ERROR_MEMORY_ERRNO();

  size_t const bytes_read = fread(content_buffer, 1, *out_length, stream);
  if (bytes_read < *out_length) ERROR_IO_ERRNO();

  return content_buffer;
}

/// Read content of finite seekable binary `stream` resource into heap-allocated string.
/// @note Caller takes ownership of returned string.
/// @return String with `stream` resource content.
char *io_read_finite_seekable_binary_stream_as_str(FILE *const stream) {
  assert(stream != NULL);

  size_t content_length;
  uint8_t *content_buffer = io_read_finite_seekable_binary_stream(stream, &content_length);

  char *const content_string = realloc(content_buffer, content_length + 1); // account for NUL terminator
  if (content_string == NULL) ERROR_MEMORY_ERRNO();
  content_string[content_length] = '\0';

  return content_string;
}

/// Read textual file at `filepath` into heap-allocated string.
/// @note Caller takes ownership of returned string.
/// @return String with `filepath` file content.
char *io_read_text_file(char const *const filepath) {
  assert(filepath != NULL);

  FILE *const file_stream = fopen(filepath, "rb");
  if (file_stream == NULL) ERROR_IO("Failed to open file '%s'" COMMON_MS "%s\n", filepath, strerror(errno));

  char *const content_string = io_read_finite_seekable_binary_stream_as_str(file_stream);

  if (fclose(file_stream)) ERROR_IO("Failed to close file '%s'" COMMON_MS "%s\n", filepath, strerror(errno));

  return content_string;
}
