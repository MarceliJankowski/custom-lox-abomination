#ifndef COMPONENT_TEST_UTILS_H
#define COMPONENT_TEST_UTILS_H

#include "common/common_test_utils.h"

#include <stdio.h>

#define APPLY_TO_EACH_ARG(fn, arg_type, ...)                                 \
  do {                                                                       \
    arg_type const args[] = {__VA_ARGS__};                                   \
    for (size_t i = 0; i < sizeof(args) / sizeof(args[0]); i++) fn(args[i]); \
  } while (0)

void assert_binary_stream_resource_content(FILE *binary_stream, char const *expected_resource_content);
void clear_binary_stream_resource_content(FILE *binary_stream);

#endif // COMPONENT_TEST_UTILS_H
