#include "unit/unit_test.h"
#include "utils/character.h"

#include <limits.h>

// *---------------------------------------------*
// *          INTERNAL-LINKAGE OBJECTS           *
// *---------------------------------------------*

static char const whitespace_characters[] = {' ', '\n', '\f', '\t', '\v', '\r'};
static char const digit_characters[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
static char const alphanumeric_characters[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
                                               'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
                                               'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                                               'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
                                               '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

// *---------------------------------------------*
// *                 TEST CASES                  *
// *---------------------------------------------*

static void character_is_whitespace__returns_true_for_whitespace_characters(void **const _) {
  for (size_t i = 0; i < sizeof(whitespace_characters); i++) {
    char const whitespace_character = whitespace_characters[i];

    bool const result = character_is_whitespace(whitespace_character);

    assert_true(result);
  }
}

static void character_is_whitespace__returns_false_for_non_whitespace_characters(void **const _) {
  for (int character = CHAR_MIN; character <= CHAR_MAX; character++) {
    // skip whitespace characters
    for (size_t i = 0; i < sizeof(whitespace_characters); i++) {
      char const whitespace_character = whitespace_characters[i];
      if (character == whitespace_character) goto skip_whitespace_character;
    }

    bool const result = character_is_whitespace(character);

    assert_false(result);

  skip_whitespace_character:;
  }
}

static void character_is_digit__returns_true_for_digit_characters(void **const _) {
  for (size_t i = 0; i < sizeof(digit_characters); i++) {
    char const digit_character = digit_characters[i];

    bool const result = character_is_digit(digit_character);

    assert_true(result);
  }
}

static void character_is_digit__returns_false_for_non_digit_characters(void **const _) {
  for (int character = CHAR_MIN; character <= CHAR_MAX; character++) {
    // skip digit characters
    for (size_t i = 0; i < sizeof(digit_characters); i++) {
      char const digit_character = digit_characters[i];
      if (character == digit_character) goto skip_digit_character;
    }

    bool const result = character_is_digit(character);

    assert_false(result);

  skip_digit_character:;
  }
}

static void character_is_alphanumeric__returns_true_for_alphanumeric_characters(void **const _) {
  for (size_t i = 0; i < sizeof(alphanumeric_characters); i++) {
    char const alphanumeric_character = alphanumeric_characters[i];

    bool const result = character_is_alphanumeric(alphanumeric_character);

    assert_true(result);
  }
}

static void character_is_alphanumeric__returns_false_for_non_alphanumeric_characters(void **const _) {
  for (int character = CHAR_MIN; character <= CHAR_MAX; character++) {
    // skip alphanumeric characters
    for (size_t i = 0; i < sizeof(alphanumeric_characters); i++) {
      char const alphanumeric_character = alphanumeric_characters[i];
      if (character == alphanumeric_character) goto skip_alphanumeric_character;
    }

    bool const result = character_is_alphanumeric(character);

    assert_false(result);

  skip_alphanumeric_character:;
  }
}

int main(void) {
  struct CMUnitTest const tests[] = {
    cmocka_unit_test(character_is_whitespace__returns_true_for_whitespace_characters),
    cmocka_unit_test(character_is_whitespace__returns_false_for_non_whitespace_characters),
    cmocka_unit_test(character_is_digit__returns_true_for_digit_characters),
    cmocka_unit_test(character_is_digit__returns_false_for_non_digit_characters),
    cmocka_unit_test(character_is_alphanumeric__returns_true_for_alphanumeric_characters),
    cmocka_unit_test(character_is_alphanumeric__returns_false_for_non_alphanumeric_characters),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
