#ifndef CLOCK_H
#define CLOCK_H

#include "arch.h"
#include <chrono>

using Clock = std::chrono::steady_clock;
using Duration = std::chrono::nanoseconds;
using Timestamp = std::chrono::time_point<Clock, Duration>;

Timestamp current_time();

double to_seconds(Duration duration);

#endif
