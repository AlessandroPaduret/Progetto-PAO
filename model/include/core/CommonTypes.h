
#pragma once

#include <string>
#include <chrono>
#include <functional>

namespace events {

using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock, std::chrono::seconds>;
using Duration = std::chrono::seconds;
using String = std::string;

using Days = std::chrono::duration<int, std::ratio<86400>>;
using Years = std::chrono::duration<int, std::ratio<31536000>>;


constexpr auto operator""_weeks(unsigned long long w) {
    return std::chrono::weeks(w);
}

constexpr auto operator""_years(unsigned long long y) {
    return std::chrono::years(y);
}

} // namespace events

template <>
struct std::hash<events::TimePoint> {
    std::size_t operator()(const events::TimePoint& tp) const noexcept {
        return std::hash<long long>{}(tp.time_since_epoch().count());
    }
};