
#ifndef COMMONTYPES_H
#define COMMONTYPES_H

#include <string>
#include <chrono>

using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock, std::chrono::seconds>;
using Duration = std::chrono::seconds;
using String = std::string;

#endif  // COMMONTYPES_H