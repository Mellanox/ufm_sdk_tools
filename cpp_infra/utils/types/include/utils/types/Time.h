
#pragma once 

#include <ostream>
#include <chrono>

namespace nvd {
namespace time {

// 1-1 ratio(seconds ratio) is the default ratio
// Declare it explicitly here only to express that the duration is in seconds
using SecondsRatio = std::ratio<1, 1>;

using DurationT = std::chrono::duration<double, SecondsRatio>;

// system clock 
using SysClockT = std::chrono::system_clock;
using TimePointT = std::chrono::time_point<SysClockT, DurationT>;

// staedy clock
using SteadyClockT = std::chrono::steady_clock;
using SteadyTimePointT = std::chrono::time_point<SteadyClockT, DurationT>;

}} // namespace nvd::time



