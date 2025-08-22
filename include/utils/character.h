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

#endif // CHARACTER_H
