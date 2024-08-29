#ifndef COMPONENT_TEST_COMMON_H
#define COMPONENT_TEST_COMMON_H

#include "shared/test_common.h"

#include <stdio.h>

#define APPLY_TO_EACH_ARG(fn, arg_type, ...)                                 \
  do {                                                                       \
    arg_type const args[] = {__VA_ARGS__};                                   \
    for (size_t i = 0; i < sizeof(args) / sizeof(args[0]); i++) fn(args[i]); \
  } while (0)

FILE *open_throwaway_stream(void);
void assert_binary_stream_resource_content(FILE *binary_stream, char const *expected_resource_content);

#endif // COMPONENT_TEST_COMMON_H
