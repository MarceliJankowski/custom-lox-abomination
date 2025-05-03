#include "unit/unit_test.h"
#include "utils/number.h"

#include <cmocka.h>
#include <math.h>

static void number_is_integer__returns_true_for_integers(void **const _) {
  const double positive = 1.0;
  const double negative = -1.0;
  const double zero = 0.0;

  assert_true(number_is_integer(positive));
  assert_true(number_is_integer(negative));
  assert_true(number_is_integer(zero));
}

static void number_is_integer__returns_false_for_non_integer_numbers(void **const _) {
  const double positive = 1.1;
  const double negative = -1.1;

  assert_false(number_is_integer(positive));
  assert_false(number_is_integer(negative));
}

static void number_is_integer__returns_false_for_nan(void **const _) {
  const double nan = NAN;
  assert_false(number_is_integer(nan));
}

static void number_is_integer__returns_false_for_inf(void **const _) {
  const double inf = INFINITY;
  assert_false(number_is_integer(inf));
}

int main(void) {
  struct CMUnitTest const tests[] = {
    cmocka_unit_test(number_is_integer__returns_true_for_integers),
    cmocka_unit_test(number_is_integer__returns_false_for_non_integer_numbers),
    cmocka_unit_test(number_is_integer__returns_false_for_nan),
    cmocka_unit_test(number_is_integer__returns_false_for_inf)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
