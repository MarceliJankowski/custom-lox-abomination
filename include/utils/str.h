#ifndef STR_H
#define STR_H

#include "utils/character.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

// *---------------------------------------------*
// *              INLINE FUNCTIONS               *
// *---------------------------------------------*

/**@desc determine whether `string` is comprised entirely of whitespace characters
@return true if it is, false otherwise*/
inline bool str_is_all_whitespace(char const *string) {
  assert(string != NULL);

  for (; *string != '\0'; ++string) {
    if (!character_is_whitespace(*string)) return false;
  }

  return true;
}

#endif // STR_H
