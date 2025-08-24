#ifndef IO_H
#define IO_H

#include "global.h"
#include "utils/error.h"

#include <stdarg.h>
#include <stdio.h>

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

void *io_read_binary_stream_resource_content(FILE *binary_stream);
char *io_read_file(char const *filepath);

// *---------------------------------------------*
// *              INLINE FUNCTIONS               *
// *---------------------------------------------*

/**@desc stdlib `fprintf` wrapper with error handling*/
inline int io_fprintf(FILE *const stream, char const *const format, ...) {
  va_list format_args;
  va_start(format_args, format);
  int const printed_byte_count = vfprintf(stream, format, format_args);
  va_end(format_args);

  if (printed_byte_count < 0) ERROR_IO_ERRNO();

  return printed_byte_count;
}

/**@desc stdlib `printf` wrapper with error handling*/
inline int io_printf(char const *const format, ...) {
  va_list format_args;
  va_start(format_args, format);
  int const printed_byte_count = vprintf(format, format_args);
  va_end(format_args);

  if (printed_byte_count < 0) ERROR_IO_ERRNO();

  return printed_byte_count;
}

/**@desc stdlib `puts` wrapper with error handling*/
inline void io_puts(char const *const string) {
  if (puts(string) == EOF) ERROR_IO_ERRNO();
}

#endif // IO_H
