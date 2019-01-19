#include "mathematics.h"
#include "tests.h"

TEST(test_rounding_divde) {
  assert(rounding_divide(14, 10) == 1);
  assert(rounding_divide(16, 10) == 2);
  assert(rounding_divide(-14, 10) == -1);
  assert(rounding_divide(-16, 10) == -2);
}
