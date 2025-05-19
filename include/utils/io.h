#ifndef IO_H
#define IO_H

#include "global.h"

#include <stdio.h>

void *io_read_binary_stream_resource_content(FILE *binary_stream);
char *io_read_file(char const *filepath);

#endif // IO_H
