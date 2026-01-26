#ifndef IO_H
#define IO_H

#include "global.h"
#include "utils/error.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

uint8_t *io_read_finite_stream(FILE *stream, size_t *out_length);
char *io_read_finite_stream_as_str(FILE *stream);
uint8_t *io_read_finite_seekable_binary_stream(FILE *stream, size_t *out_length);
char *io_read_finite_seekable_binary_stream_as_str(FILE *stream);
char *io_read_text_file(char const *filepath);

// *---------------------------------------------*
// *              INLINE FUNCTIONS               *
// *---------------------------------------------*

/// Stdlib `fprintf` wrapper with error handling.
inline int io_fprintf(FILE *const stream, char const *const format, ...) {
  va_list format_args;
  va_start(format_args, format);
  int const printed_byte_count = vfprintf(stream, format, format_args);
  va_end(format_args);

  if (printed_byte_count < 0) ERROR_IO_ERRNO();

  return printed_byte_count;
}

/// Stdlib `printf` wrapper with error handling.
inline int io_printf(char const *const format, ...) {
  va_list format_args;
  va_start(format_args, format);
  int const printed_byte_count = vprintf(format, format_args);
  va_end(format_args);

  if (printed_byte_count < 0) ERROR_IO_ERRNO();

  return printed_byte_count;
}

/// Stdlib `puts` wrapper with error handling.
inline void io_puts(char const *const string) {
  if (puts(string) == EOF) ERROR_IO_ERRNO();
}

#endif // IO_H
