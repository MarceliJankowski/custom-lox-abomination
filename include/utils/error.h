#ifndef ERROR_H
#define ERROR_H

#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

#define ERROR__FILE_LINE __FILE__ COMMON_PS COMMON_STRINGIZE(__LINE__)

#ifdef DEBUG
#define ERROR__DEBUG_FILE_LINE ERROR__FILE_LINE COMMON_MS
#else
#define ERROR__DEBUG_FILE_LINE
#endif

#define ERROR__INTERNAL_PREFIX "[ERROR_INTERNAL]"
#define ERROR__INVALID_ARG_PREFIX "[ERROR_INVALID_ARG]"
#define ERROR__MEMORY_PREFIX "[ERROR_MEMORY]"
#define ERROR__IO_PREFIX "[ERROR_IO]"
#define ERROR__SYSTEM_PREFIX "[ERROR_SYSTEM]"

/**@desc print error message content
@param ... printf arguments constituting error message content*/
#define ERROR__PRINT_MESSAGE_CONTENT(...)                                                                    \
  do {                                                                                                       \
    fprintf(stderr, COMMON_MS);                                                                              \
    fprintf(stderr, __VA_ARGS__); /* separate fprintf call so that message can also be passed as a pointer*/ \
    fprintf(stderr, "\n");                                                                                   \
  } while (0)

/**@desc boilerplate shared across error handling macros
@param error_code ErrorCode identifying error type
@param message_prefix string literal containing error message prefix
@param ... printf arguments constituting error message content*/
#define ERROR__BOILERPLATE(error_code, message_prefix, ...) \
  do {                                                      \
    fprintf(stderr, ERROR__DEBUG_FILE_LINE message_prefix); \
    ERROR__PRINT_MESSAGE_CONTENT(__VA_ARGS__);              \
    exit(error_code);                                       \
  } while (0)

/**@desc report internal error and trigger corresponding behavior
@param ... printf arguments constituting error message*/
#define ERROR_INTERNAL(...)                                             \
  do {                                                                  \
    fprintf(stderr, ERROR__INTERNAL_PREFIX COMMON_MS ERROR__FILE_LINE); \
    ERROR__PRINT_MESSAGE_CONTENT(__VA_ARGS__);                          \
    abort();                                                            \
  } while (0)

/**@desc report invalid argument error and trigger corresponding behavior
@param ... printf arguments constituting error message*/
#define ERROR_INVALID_ARG(...) ERROR__BOILERPLATE(ERROR_CODE_INVALID_ARG, ERROR__INVALID_ARG_PREFIX, __VA_ARGS__)

/**@desc report memory error and trigger corresponding behavior
@param ... printf arguments constituting error message*/
#define ERROR_MEMORY(...) ERROR__BOILERPLATE(ERROR_CODE_MEMORY, ERROR__MEMORY_PREFIX, __VA_ARGS__)

/**@desc report memory error with message comming from errno, and trigger corresponding behavior*/
#define ERROR_MEMORY_ERRNO() ERROR_MEMORY("%s\n", strerror(errno))

/**@desc report IO error and trigger corresponding behavior
@param ... printf arguments constituting error message*/
#define ERROR_IO(...) ERROR__BOILERPLATE(ERROR_CODE_IO, ERROR__IO_PREFIX, __VA_ARGS__)

/**@desc report IO error with message comming from errno, and trigger corresponding behavior*/
#define ERROR_IO_ERRNO() ERROR_IO("%s\n", strerror(errno))

/**@desc report system error and trigger corresponding behavior
@param ... printf arguments constituting error message*/
#define ERROR_SYSTEM(...) ERROR__BOILERPLATE(ERROR_CODE_SYSTEM, ERROR__SYSTEM_PREFIX, __VA_ARGS__)

/**@desc report system error with message comming from errno, and trigger corresponding behavior*/
#define ERROR_SYSTEM_ERRNO() ERROR_SYSTEM("%s\n", strerror(errno))

#ifdef _WIN32
/**@desc report system error caused by latest Windows API error, and trigger corresponding behavior*/
#define ERROR_WINDOWS_LAST()                                                \
  do {                                                                      \
    fprintf(stderr, ERROR__DEBUG_FILE_LINE ERROR__SYSTEM_PREFIX COMMON_MS); \
    error_windows_log_last();                                               \
    exit(ERROR_CODE_SYSTEM);                                                \
  } while (0)
#endif

// *---------------------------------------------*
// *              TYPE DEFINITIONS               *
// *---------------------------------------------*

typedef enum {
  ERROR_CODE_SUCCESS,
  ERROR_CODE_COMPILATION,
  ERROR_CODE_EXECUTION,
  ERROR_CODE_INVALID_ARG,
  ERROR_CODE_MEMORY,
  ERROR_CODE_IO,
  ERROR_CODE_SYSTEM,
  ERROR_CODE_COUNT,
} ErrorCode;

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

void error_windows_log_last(void);

#endif // ERROR_H
