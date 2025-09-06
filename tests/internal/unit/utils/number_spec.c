#include "unit/unit_test.h"
#include "utils/number.h"

#include <math.h>

// *---------------------------------------------*
// *                 TEST CASES                  *
// *---------------------------------------------*

static void number_is_integer__returns_true_for_ints(void **const _) {
  double const negative = -1;
  double const zero = 0;
  double const positive = 1;

  bool const negative_result = number_is_integer(negative);
  bool const zero_result = number_is_integer(zero);
  bool const positive_result = number_is_integer(positive);

  assert_true(negative_result);
  assert_true(zero_result);
  assert_true(positive_result);
}

static void number_is_integer__returns_false_for_floats(void **const _) {
  double const negative = -1.1;
  double const positive = 1.1;

  bool const negative_result = number_is_integer(negative);
  bool const positive_result = number_is_integer(positive);

  assert_false(negative_result);
  assert_false(positive_result);
}

static void number_is_integer__returns_false_for_infs(void **const _) {
  double const negative = -INFINITY;
  double const positive = INFINITY;

  bool const negative_result = number_is_integer(negative);
  bool const positive_result = number_is_integer(positive);

  assert_false(negative_result);
  assert_false(positive_result);
}

static void number_is_integer__returns_false_for_nan(void **const _) {
  double const nan = -NAN;

  bool const nan_result = number_is_integer(nan);

  assert_false(nan_result);
}

int main(void) {
  struct CMUnitTest const tests[] = {
    cmocka_unit_test(number_is_integer__returns_true_for_ints),
    cmocka_unit_test(number_is_integer__returns_false_for_floats),
    cmocka_unit_test(number_is_integer__returns_false_for_infs),
    cmocka_unit_test(number_is_integer__returns_false_for_nan),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
