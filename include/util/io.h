#ifndef IO_H
#define IO_H

#include <stdio.h>

char *read_binary_stream(FILE *binary_stream);
char *read_file(char const *filepath);

#endif // IO_H
