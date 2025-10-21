#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdbool.h>

// *---------------------------------------------*
// *              INLINE FUNCTIONS               *
// *---------------------------------------------*

/**@desc determine whether `character` is a whitespace character
@return true if it is, false otherwise*/
inline bool character_is_whitespace(char const character) {
  switch (character) {
    case ' ':
    case '\n':
    case '\f':
    case '\t':
    case '\v':
    case '\r': return true;

    default: return false;
  }
}

/**@desc determine whether `character` is a digit
@return true if it is, false otherwise*/
inline bool character_is_digit(char const character) {
  return character >= '0' && character <= '9';
}

/**@desc determine whether `character` is alphanumeric
@return true if it is, false otherwise*/
inline bool character_is_alphanumeric(char const character) {
  return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
         character_is_digit(character);
}

#endif // CHARACTER_H
