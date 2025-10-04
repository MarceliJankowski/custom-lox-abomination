#include "unit/unit_test.h"
#include "utils/str.h"

#include <limits.h>
#include <string.h>

// *---------------------------------------------*
// *          INTERNAL-LINKAGE OBJECTS           *
// *---------------------------------------------*

static char const whitespace_characters_str[] = {' ', '\n', '\f', '\t', '\v', '\r', '\0'};

// *---------------------------------------------*
// *                 TEST CASES                  *
// *---------------------------------------------*

static void str_is_all_whitespace__returns_true_when_all_characters_are_whitespace(void **const _) {
  bool const result = str_is_all_whitespace(whitespace_characters_str);

  assert_true(result);
}

static void str_is_all_whitespace__returns_true_when_all_characters_are_whitespace_before_null(void **const _) {
  char input_str[sizeof(whitespace_characters_str) + 1] = {0};
  memcpy(input_str, whitespace_characters_str, sizeof(whitespace_characters_str));

  for (int character = CHAR_MIN; character <= CHAR_MAX; character++) {
    // skip whitespace characters
    for (size_t i = 0; i < sizeof(whitespace_characters_str); i++) {
      char const whitespace_character = whitespace_characters_str[i];
      if (character == whitespace_character) goto skip_whitespace_character;
    }
    input_str[sizeof(input_str) - 1] = character;

    bool const result = str_is_all_whitespace(input_str);

    if (!result) print_message("failed character '%d'\n", character);

    assert_true(result);

  skip_whitespace_character:;
  }
}

static void str_is_all_whitespace__returns_false_when_any_character_is_not_whitespace(void **const _) {
  char input_str[sizeof(whitespace_characters_str) + 1] = {0};
  memcpy(input_str, whitespace_characters_str, sizeof(whitespace_characters_str));

  for (int character = CHAR_MIN; character <= CHAR_MAX; character++) {
    // skip whitespace characters
    for (size_t i = 0; i < sizeof(whitespace_characters_str); i++) {
      char const whitespace_character = whitespace_characters_str[i];
      if (character == whitespace_character) goto skip_whitespace_character;
    }
    input_str[sizeof(input_str) - 2] = character;

    bool const result = str_is_all_whitespace(input_str);

    if (result) print_message("failed character '%d'\n", character);

    assert_false(result);

  skip_whitespace_character:;
  }
}

static void str_count_lines__returns_newline_count(void **const _) {
  struct {
    char const *const case_name;
    char const *const input_string;
    int const expected_newline_count;
  } test_cases[] = {
    // Empty string
    {"empty string", "", 0},
    // Only newlines
    {"single newline", "\n", 1},
    {"two newlines", "\n\n", 2},
    {"three newlinse", "\n\n\n", 3},
    // Newlines in text
    {"single newline in text", "line1\nline2", 1},
    {"two newlines in text", "line1\nline2\n", 2},
    {"two newlines in whitespace", "     \n     \n", 2},
    // LF, CRLF
    {"mixed LF, CRLF", "line1\nline2\r\nline3\rline4", 2},
    // All whitespace characters
    {"whitespace characters", whitespace_characters_str, 1},
    // Null termination
    {"null termination", "line1\nline2\0\n", 1},
  };

  for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); ++i) {
    char const *const input = test_cases[i].input_string;
    size_t const expected = test_cases[i].expected_newline_count;

    size_t const result = str_count_lines(input);

    if (result != expected) print_message("failed case '%s'\n", test_cases[i].case_name);

    assert_int_equal(result, expected);
  }
}

int main(void) {
  struct CMUnitTest const tests[] = {
    cmocka_unit_test(str_is_all_whitespace__returns_true_when_all_characters_are_whitespace),
    cmocka_unit_test(str_is_all_whitespace__returns_true_when_all_characters_are_whitespace_before_null),
    cmocka_unit_test(str_is_all_whitespace__returns_false_when_any_character_is_not_whitespace),
    cmocka_unit_test(str_count_lines__returns_newline_count),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
