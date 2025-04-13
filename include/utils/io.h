#ifndef IO_H
#define IO_H

#include "global.h"

#include <stdio.h>

#define IO_PUTS_BREAK(string)             \
  fputs(string, g_runtime_output_stream); \
  break

#define IO_PRINTF_BREAK(...)                     \
  fprintf(g_runtime_output_stream, __VA_ARGS__); \
  break

void *io_read_binary_stream_resource_content(FILE *binary_stream);
char *io_read_file(char const *filepath);

#endif // IO_H
