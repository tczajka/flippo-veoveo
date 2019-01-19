#include "clock.h"

Timestamp current_time() {
  return std::chrono::time_point_cast<Duration>(Clock::now());
}

double to_seconds(const Duration duration) {
  return std::chrono::duration<double>(duration).count();
}
