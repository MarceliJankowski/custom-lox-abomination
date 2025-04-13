#include "utils/number.h"

#include <math.h>

/**@desc determine whether `number` is an integer
@return true if it is, false otherwise*/
bool number_is_integer(double const number) {
  if (isnan(number) || isinf(number)) return false;
  return floor(number) == number;
}
