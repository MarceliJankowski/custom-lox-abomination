#ifndef STR_H
#define STR_H

#include "utils/character.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

// *---------------------------------------------*
// *              INLINE FUNCTIONS               *
// *---------------------------------------------*

/// Determine whether `string` is comprised entirely of whitespace characters.
/// @return true if it is, false otherwise.
inline bool str_is_all_whitespace(char const *string) {
  assert(string != NULL);

  for (; *string != '\0'; ++string) {
    if (!character_is_whitespace(*string)) return false;
  }

  return true;
}

/// Count number of newlines within a `string`.
/// @return Newline count.
inline size_t str_count_lines(char const *string) {
  assert(string != NULL);

  size_t line_count = 0;
  for (; *string != '\0'; ++string) {
    if (*string == '\n') line_count++;
  }

  return line_count;
}

#endif // STR_H
