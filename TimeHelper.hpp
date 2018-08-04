#pragma once
#include <chrono>

using Clock = std::chrono::steady_clock;
using milliseconds = std::chrono::milliseconds;
using TimePoint_ms = std::chrono::time_point<Clock, milliseconds>;
constexpr auto MillisecondsPerSecond =
    std::chrono::duration_cast<milliseconds>(std::chrono::seconds(1));
constexpr auto UpdateDuration = MillisecondsPerSecond / 50;

static TimePoint_ms NowMilliseconds() {
  auto now = Clock::now();
  auto now_ms = std::chrono::time_point_cast<milliseconds>(now);
  return now_ms;
}