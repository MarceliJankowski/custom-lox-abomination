#ifndef COMPONENT_TEST_H
#define COMPONENT_TEST_H

#include "backend/value.h"
#include "common/common_test.h"

#include <stdio.h>

#define COMPONENT_TEST_APPLY_TO_EACH_ARG(fn, arg_type, ...)                  \
  do {                                                                       \
    arg_type const args[] = {__VA_ARGS__};                                   \
    for (size_t i = 0; i < sizeof(args) / sizeof(args[0]); i++) fn(args[i]); \
  } while (0)

void component_test_assert_binary_stream_resource_content(FILE *binary_stream, char const *expected_resource_content);
void component_test_clear_binary_stream_resource_content(FILE *binary_stream);
void component_test_assert_value_equality(Value value_a, Value value_b);

#endif // COMPONENT_TEST_H
