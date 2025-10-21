#include "unit/unit_test.h"
#include "utils/character.h"

#include <limits.h>

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

#define ASSERT_FN_RETURNS_TRUE_FOR_TEST_STATE(fn)              \
  do {                                                         \
    TestState const *const test_state = (TestState *)(*state); \
                                                               \
    for (size_t i = 0; i < test_state->count; i++) {           \
      char const character = test_state->characters[i];        \
                                                               \
      bool const result = fn(character);                       \
                                                               \
      assert_true(result);                                     \
    }                                                          \
  } while (0)

#define ASSERT_FN_RETURNS_FALSE_FOR_TEST_STATE(fn)                       \
  do {                                                                   \
    TestState const *const test_state = (TestState *)(*state);           \
                                                                         \
    for (int character = CHAR_MIN; character <= CHAR_MAX; character++) { \
      /* skip test_state characters */                                   \
      for (size_t i = 0; i < test_state->count; i++) {                   \
        char const test_state_character = test_state->characters[i];     \
        if (character == test_state_character) goto skip_character;      \
      }                                                                  \
                                                                         \
      bool const result = fn(character);                                 \
                                                                         \
      assert_false(result);                                              \
                                                                         \
    skip_character:;                                                     \
    }                                                                    \
  } while (0)

#define MAKE_TEST_STATE(character_array) ((TestState){(character_array), sizeof((character_array))})

// *---------------------------------------------*
// *              TYPE DEFINITIONS               *
// *---------------------------------------------*

typedef struct {
  char const *characters;
  size_t count;
} TestState;

// *---------------------------------------------*
// *                 TEST CASES                  *
// *---------------------------------------------*

static void character_is_whitespace__returns_true_for_whitespace_characters(void **const state) {
  ASSERT_FN_RETURNS_TRUE_FOR_TEST_STATE(character_is_whitespace);
}

static void character_is_whitespace__returns_false_for_non_whitespace_characters(void **const state) {
  ASSERT_FN_RETURNS_FALSE_FOR_TEST_STATE(character_is_whitespace);
}

static void character_is_digit__returns_true_for_digit_characters(void **const state) {
  ASSERT_FN_RETURNS_TRUE_FOR_TEST_STATE(character_is_digit);
}

static void character_is_digit__returns_false_for_non_digit_characters(void **const state) {
  ASSERT_FN_RETURNS_FALSE_FOR_TEST_STATE(character_is_digit);
}

static void character_is_alphanumeric__returns_true_for_alphanumeric_characters(void **const state) {
  ASSERT_FN_RETURNS_TRUE_FOR_TEST_STATE(character_is_alphanumeric);
}

static void character_is_alphanumeric__returns_false_for_non_alphanumeric_characters(void **const state) {
  ASSERT_FN_RETURNS_FALSE_FOR_TEST_STATE(character_is_alphanumeric);
}

int main(void) {
  char const whitespace_characters[] = {' ', '\n', '\f', '\t', '\v', '\r'};
  TestState whitespace_state = MAKE_TEST_STATE(whitespace_characters);

  char const digit_characters[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
  TestState digit_state = MAKE_TEST_STATE(digit_characters);

  size_t const alphanumeric_character_count = ('z' - 'a' + 1) + ('Z' - 'A' + 1) + ('9' - '0' + 1);
  char alphanumeric_characters[alphanumeric_character_count];
  {
    size_t i = 0;
    for (char c = 'a'; c <= 'z'; i++, c++) alphanumeric_characters[i] = c;
    for (char c = 'A'; c <= 'Z'; i++, c++) alphanumeric_characters[i] = c;
    for (char c = '0'; c <= '9'; i++, c++) alphanumeric_characters[i] = c;
    assert(i == alphanumeric_character_count);
  }
  TestState alphanumeric_state = MAKE_TEST_STATE(alphanumeric_characters);

  struct CMUnitTest const tests[] = {
    cmocka_unit_test_prestate(character_is_whitespace__returns_true_for_whitespace_characters, &whitespace_state),
    cmocka_unit_test_prestate(character_is_whitespace__returns_false_for_non_whitespace_characters, &whitespace_state),
    cmocka_unit_test_prestate(character_is_digit__returns_true_for_digit_characters, &digit_state),
    cmocka_unit_test_prestate(character_is_digit__returns_false_for_non_digit_characters, &digit_state),
    cmocka_unit_test_prestate(character_is_alphanumeric__returns_true_for_alphanumeric_characters, &alphanumeric_state),
    cmocka_unit_test_prestate(
      character_is_alphanumeric__returns_false_for_non_alphanumeric_characters, &alphanumeric_state
    ),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
