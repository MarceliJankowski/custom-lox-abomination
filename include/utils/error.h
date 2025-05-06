#ifndef ERROR_H
#define ERROR_H

#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERROR_CODE_COMPILATION 1
#define ERROR_CODE_EXECUTION 2
#define ERROR_CODE_INVALID_ARG 3
#define ERROR_CODE_MEMORY 4
#define ERROR_CODE_IO 5
#define ERROR_CODE_SYSTEM 6
#define ERROR_CODE_COUNT ERROR_CODE_SYSTEM

#define ERROR__FILE_LINE __FILE__ COMMON_PS COMMON_STRINGIZE(__LINE__)

#ifdef DEBUG
#define ERROR__DEBUG_FILE_LINE ERROR__FILE_LINE COMMON_MS
#else
#define ERROR__DEBUG_FILE_LINE
#endif

#define ERROR__BOILERPLATE_PRINT_ARGS(...)                                                                 \
  fprintf(stderr, COMMON_MS);                                                                              \
  fprintf(stderr, __VA_ARGS__); /* separate fprintf call so that message can also be passed as a pointer*/ \
  fprintf(stderr, "\n")

#define ERROR__BOILERPLATE(error_code, message_prefix, ...) \
  do {                                                      \
    fprintf(stderr, ERROR__DEBUG_FILE_LINE message_prefix); \
    ERROR__BOILERPLATE_PRINT_ARGS(__VA_ARGS__);             \
    exit(error_code);                                       \
  } while (0)

#define ERROR_INTERNAL(...)                                         \
  do {                                                              \
    fprintf(stderr, "[ERROR_INTERNAL]" COMMON_MS ERROR__FILE_LINE); \
    ERROR__BOILERPLATE_PRINT_ARGS(__VA_ARGS__);                     \
    abort();                                                        \
  } while (0)

#define ERROR_INVALID_ARG(...) ERROR__BOILERPLATE(ERROR_CODE_INVALID_ARG, "[ERROR_INVALID_ARG]", __VA_ARGS__)

#define ERROR_MEMORY(...) ERROR__BOILERPLATE(ERROR_CODE_MEMORY, "[ERROR_MEMORY]", __VA_ARGS__)
#define ERROR_MEMORY_ERRNO() ERROR_MEMORY("%s", strerror(errno))

#define ERROR_IO(...) ERROR__BOILERPLATE(ERROR_CODE_IO, "[ERROR_IO]", __VA_ARGS__)
#define ERROR_IO_ERRNO() ERROR_IO("%s", strerror(errno))

#define ERROR_SYSTEM(...) ERROR__BOILERPLATE(ERROR_CODE_SYSTEM, "[ERROR_SYSTEM]", __VA_ARGS__)
#define ERROR_SYSTEM_ERRNO() ERROR_SYSTEM("%s", strerror(errno))

#endif // ERROR_H
