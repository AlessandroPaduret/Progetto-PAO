
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

// Definizioni di comodità se mancano in std::chrono
using Days = std::chrono::duration<int, std::ratio<86400>>; // 24 ore
using Years = std::chrono::duration<int, std::ratio<31536000>>; // 365 giorni

struct TimePointHasher {
    std::size_t operator()(const TimePoint& tp) const {
        return std::hash<long long>{}(tp.time_since_epoch().count());
    }
};

} // namespace events

#endif  // COMMONTYPES_H