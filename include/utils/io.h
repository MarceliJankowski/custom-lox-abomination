#ifndef IO_H
#define IO_H

#include "global.h"

#include <stdio.h>

#define PUTS_BREAK(string_literal)                     \
  fputs(string_literal "\n", g_runtime_output_stream); \
  break

#define PRINTF_BREAK(...)                        \
  fprintf(g_runtime_output_stream, __VA_ARGS__); \
  break

void *read_binary_stream_resource_content(FILE *binary_stream);
char *read_file(char const *filepath);

#endif // IO_H
