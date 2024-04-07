#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <stdlib.h>

#include "common.h"

#define INVALID_ARG_ERROR_CODE 1
#define MEMORY_ERROR_CODE 2
#define LEXICAL_ERROR_CODE 3
#define SYNTAX_ERROR_CODE 4
#define SEMANTIC_ERROR_CODE 5
#define RUNTIME_ERROR_CODE 6
#define ERROR_CODE_COUNT RUNTIME_ERROR_CODE

#define INTERNAL_FILE_LINE __FILE__ ":" STRINGIZE(__LINE__)

#ifdef DEBUG
#define DEBUG_INTERNAL_FILE_LINE INTERNAL_FILE_LINE M_S
#else
#define DEBUG_INTERNAL_FILE_LINE
#endif

#define ERROR_BOILERPLATE_PRINT_ARGS(...)                                                                  \
  fprintf(stderr, M_S);                                                                                    \
  fprintf(stderr, __VA_ARGS__); /* separate fprintf call so that message can also be passed as a pointer*/ \
  fprintf(stderr, "\n")

#define ERROR_BOILERPLATE(error_code, message_prefix, ...)    \
  do {                                                        \
    fprintf(stderr, DEBUG_INTERNAL_FILE_LINE message_prefix); \
    ERROR_BOILERPLATE_PRINT_ARGS(__VA_ARGS__);                \
    exit(error_code);                                         \
  } while (0)

#define INTERNAL_ERROR(...)                                     \
  do {                                                          \
    fprintf(stderr, "[INTERNAL_ERROR]" M_S INTERNAL_FILE_LINE); \
    ERROR_BOILERPLATE_PRINT_ARGS(__VA_ARGS__);                  \
    abort();                                                    \
  } while (0)

#define INVALID_ARG_ERROR(...) ERROR_BOILERPLATE(INVALID_ARG_ERROR_CODE, "[INVALID_ARG_ERROR]", __VA_ARGS__)

#define MEMORY_ERROR(...) ERROR_BOILERPLATE(MEMORY_ERROR_CODE, "[MEMORY_ERROR]", __VA_ARGS__)

#define LEXICAL_ERROR(file, line, column, ...)                                                            \
  ERROR_BOILERPLATE(                                                                                      \
    LEXICAL_ERROR_CODE, "[LEXICAL_ERROR]" M_S FILE_LINE_COLUMN_FORMAT COMMA file COMMA line COMMA column, \
    __VA_ARGS__                                                                                           \
  )

#define SYNTAX_ERROR(file, line, column, ...)                                                           \
  ERROR_BOILERPLATE(                                                                                    \
    SYNTAX_ERROR_CODE, "[SYNTAX_ERROR]" M_S FILE_LINE_COLUMN_FORMAT COMMA file COMMA line COMMA column, \
    __VA_ARGS__                                                                                         \
  )

#define SEMANTIC_ERROR(file, line, column, ...)                                                             \
  ERROR_BOILERPLATE(                                                                                        \
    SEMANTIC_ERROR_CODE, "[SEMANTIC_ERROR]" M_S FILE_LINE_COLUMN_FORMAT COMMA file COMMA line COMMA column, \
    __VA_ARGS__                                                                                             \
  )

#define RUNTIME_ERROR(file, line, ...)                                                            \
  ERROR_BOILERPLATE(                                                                              \
    RUNTIME_ERROR_CODE, "[RUNTIME_ERROR]" M_S FILE_LINE_FORMAT COMMA file COMMA line, __VA_ARGS__ \
  )

#endif // ERROR_H
