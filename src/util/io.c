#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/error.h"

/**@desc read `filepath` content into dynamically allocated buffer
@return pointer to NUL terminated buffer with `filepath` content*/
char *read_file(char const *const filepath) {
  assert(filepath != NULL);

  // open file stream
  FILE *const file_stream = fopen(filepath, "rb");
  if (file_stream == NULL) IO_ERROR("Failed to open file '%s' (%s)", filepath, strerror(errno));

  // get file size
  if (fseek(file_stream, 0, SEEK_END)) IO_ERROR("Failed to seek file '%s' (%s)", filepath, strerror(errno));
  errno = 0;
  size_t const file_size = ftell(file_stream);
  if (errno) IO_ERROR("Failed to retrieve file '%s' position indicator (%s)", filepath, strerror(errno));
  if (fseek(file_stream, 0, SEEK_SET)) IO_ERROR("Failed to seek file '%s' (%s)", filepath, strerror(errno));

  // allocate file buffer
  char *const file_buffer = malloc(file_size + 1); // account for NUL terminator
  if (file_buffer == NULL) MEMORY_ERROR("Failed to allocate memory for file '%s'", filepath);

  // read file into allocated buffer
  size_t const bytes_read = fread(file_buffer, 1, file_size, file_stream);
  if (bytes_read < file_size) IO_ERROR("Failed to read file '%s' (%s)", filepath, strerror(errno));
  file_buffer[bytes_read] = '\0';

  // clean up and return
  if (fclose(file_stream)) IO_ERROR("Failed to close file '%s' (%s)", filepath, strerror(errno));
  return file_buffer;
}
