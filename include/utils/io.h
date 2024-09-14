#ifndef IO_H
#define IO_H

#include <stdio.h>

void *read_binary_stream_resource_content(FILE *binary_stream);
char *read_file(char const *filepath);

#endif // IO_H
