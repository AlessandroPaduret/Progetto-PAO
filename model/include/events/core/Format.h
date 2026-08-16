#ifndef EVENTS_FORMAT_H
#define EVENTS_FORMAT_H

#include <chrono>
#include <cstdio>
#include <cstring>

#include "events/core/CommonTypes.h"

namespace events {

/** @brief Formatta un TimePoint come "YYYY-MM-DD HH:MM" (UTC), per la sola visualizzazione */
inline String formatDateTime(const TimePoint tp) {
  const auto days = std::chrono::floor<std::chrono::days>(tp);
  const std::chrono::year_month_day ymd{days};
  const std::chrono::hh_mm_ss hms{tp - days};
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%04d-%02u-%02u %02d:%02d",
                static_cast<int>(ymd.year()),
                static_cast<unsigned>(ymd.month()),
                static_cast<unsigned>(ymd.day()),
                static_cast<int>(hms.hours().count()),
                static_cast<int>(hms.minutes().count()));
  return buffer;
}

/** @brief Formatta una Duration in modo leggibile (es. "1g 2h 30m"), per la sola visualizzazione */
inline String formatDuration(const Duration d) {
  long long s = d.count();
  const long long days = s / 86400;
  s %= 86400;
  const long long hours = s / 3600;
  s %= 3600;
  const long long minutes = s / 60;

  String out;
  if (days != 0) out += std::to_string(days) + "g ";
  if (hours != 0) out += std::to_string(hours) + "h ";
  if (minutes != 0 || out.empty()) out += std::to_string(minutes) + "m";
  if (!out.empty() && out.back() == ' ') out.pop_back();
  return out;
}

/** @brief Formatta un TimePoint come "YYYY-MM-DDTHH:MM:SS" (UTC), per la persistenza */
inline String formatIso8601(const TimePoint tp) {
  const auto days = std::chrono::floor<std::chrono::days>(tp);
  const std::chrono::year_month_day ymd{days};
  const std::chrono::hh_mm_ss hms{tp - days};
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%04d-%02u-%02uT%02d:%02d:%02d",
                static_cast<int>(ymd.year()),
                static_cast<unsigned>(ymd.month()),
                static_cast<unsigned>(ymd.day()),
                static_cast<int>(hms.hours().count()),
                static_cast<int>(hms.minutes().count()),
                static_cast<int>(hms.seconds().count()));
  return buffer;
}

/** @brief Interpreta "YYYY-MM-DDTHH:MM:SS" (UTC) come TimePoint.
 *  @return true se la stringa e' valida e completamente consumata
 */
inline bool parseIso8601(const String& text, TimePoint& out) {
  // Lunghezza esatta: "YYYY-MM-DDTHH:MM:SS" = 19 caratteri
  if (text.size() != 19) {
    return false;
  }
  int y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0;
  int consumed = 0;
  if (std::sscanf(text.c_str(), "%d-%d-%dT%d:%d:%d%n", &y, &mo, &d, &h, &mi, &s,
                  &consumed) != 6 ||
      consumed != static_cast<int>(text.size())) {
    return false;
  }
  const std::chrono::year_month_day ymd{std::chrono::year{y},
                                        std::chrono::month{static_cast<unsigned>(mo)},
                                        std::chrono::day{static_cast<unsigned>(d)}};
  if (!ymd.ok() || h < 0 || h > 23 || mi < 0 || mi > 59 || s < 0 || s > 59) {
    return false;
  }
  out = std::chrono::time_point_cast<Duration>(
            std::chrono::sys_days{ymd}) +
        std::chrono::hours{h} + std::chrono::minutes{mi} + std::chrono::seconds{s};
  return true;
}

} // namespace events

#endif // EVENTS_FORMAT_H
