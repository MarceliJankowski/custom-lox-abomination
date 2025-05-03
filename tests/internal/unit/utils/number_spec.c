#include "unit/unit_test.h"
#include "utils/number.h"

#include <math.h>

// *---------------------------------------------*
// *                 TEST CASES                  *
// *---------------------------------------------*

static void number_is_integer__returns_true_for_integers(void **const _) {
  double const positive = 1.0;
  double const negative = -1.0;
  double const zero = 0.0;

  assert_true(number_is_integer(positive));
  assert_true(number_is_integer(negative));
  assert_true(number_is_integer(zero));
}

static void number_is_integer__returns_false_for_floats(void **const _) {
  double const positive = 1.1;
  double const negative = -1.1;

  assert_false(number_is_integer(positive));
  assert_false(number_is_integer(negative));
}

static void number_is_integer__returns_false_for_nan(void **const _) {
  double const nan = NAN;

  assert_false(number_is_integer(nan));
}

static void number_is_integer__returns_false_for_inf(void **const _) {
  double const inf = INFINITY;

  assert_false(number_is_integer(inf));
}

int main(void) {
  struct CMUnitTest const tests[] = {
    cmocka_unit_test(number_is_integer__returns_true_for_integers),
    cmocka_unit_test(number_is_integer__returns_false_for_floats),
    cmocka_unit_test(number_is_integer__returns_false_for_nan),
    cmocka_unit_test(number_is_integer__returns_false_for_inf)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
