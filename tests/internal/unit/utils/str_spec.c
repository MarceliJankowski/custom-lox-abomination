#include "unit/unit_test.h"
#include "utils/character.h"
#include "utils/str.h"

#include <limits.h>
#include <string.h>

// *---------------------------------------------*
// *                 TEST CASES                  *
// *---------------------------------------------*

static void str_is_all_whitespace__returns_true_for_empty_string(void **const _) {
  char const *const empty_str = "";

  bool const result = str_is_all_whitespace(empty_str);

  assert_true(result);
}

static void str_is_all_whitespace__returns_true_for_whitespace_string(void **const whitespace_str) {
  bool const result = str_is_all_whitespace(*whitespace_str);

  assert_true(result);
}

static void str_is_all_whitespace__returns_false_for_non_whitespace_strings(void **const whitespace_str) {
  for (int character = CHAR_MIN; character <= CHAR_MAX; character++) {
    if (strchr(*whitespace_str, character) != NULL) continue; // skip NUL and whitespace characters

    char const character_str[] = {character, '\0'};

    bool const result = str_is_all_whitespace(character_str);

    assert_false(result);
  }
}

static void str_count_lines__returns_newline_count(void **const _) {
  typedef struct {
    char *input_str;
    size_t expected_newline_count;
  } TestCase;

  TestCase const test_cases[] = {
    {"", 0},
    {"abc", 0},
    {"\r", 0},
    {"\n", 1},
    {"a\n\nb", 2},
    {"a\r\n\r\nb", 2},
    {"line 1\nline 2", 1},
    {"line 1\nline 2\n", 2},
  };

  for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
    TestCase const test_case = test_cases[i];

    size_t const result = str_count_lines(test_case.input_str);

    assert_int_equal(result, test_case.expected_newline_count);
  }
}

int main(void) {
  char whitespace_str[CHARACTER_STATE_COUNT + 1] = {0};
  for (int character = CHAR_MIN, whitespace_char_count = 0; character <= CHAR_MAX; character++) {
    if (!character_is_whitespace(character)) continue;

    whitespace_str[whitespace_char_count] = character;
    whitespace_char_count++;
  }

  struct CMUnitTest const tests[] = {
    cmocka_unit_test(str_is_all_whitespace__returns_true_for_empty_string),
    cmocka_unit_test_prestate(str_is_all_whitespace__returns_true_for_whitespace_string, whitespace_str),
    cmocka_unit_test_prestate(str_is_all_whitespace__returns_false_for_non_whitespace_strings, whitespace_str),
    cmocka_unit_test(str_count_lines__returns_newline_count),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
