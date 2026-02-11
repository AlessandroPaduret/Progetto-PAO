
#ifndef COMMONTYPES_H
#define COMMONTYPES_H

#include <string>
#include <chrono>
#include <functional>

namespace events {

using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock, std::chrono::seconds>;
using Duration = std::chrono::seconds;
using String = std::string;

struct TimePointHasher {
    std::size_t operator()(const TimePoint& tp) const {
        return std::hash<long long>{}(tp.time_since_epoch().count());
    }
};

} // namespace events

#endif  // COMMONTYPES_H