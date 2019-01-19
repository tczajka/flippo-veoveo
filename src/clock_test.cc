#include "clock.h"
#include "tests.h"
#include <thread>

TEST(test_current_time) {
  const Timestamp start = current_time();
  std::this_thread::sleep_for(std::chrono::milliseconds{100});
  const Duration t = current_time() - start;
  assert(t > std::chrono::milliseconds{90});
  assert(t < std::chrono::milliseconds{110});
}

TEST(test_to_seconds) {
  assert(to_seconds(Duration{std::chrono::milliseconds{100}}) == 0.1);
}
