#include "iso8601.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace server {

bool parseIso8601(const std::string& s, events::TimePoint& out) {
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    int consumed = 0;

    // "%n" dà la posizione di fine delle conversioni: serve per rifiutare
    // input parziali (es. "2026-01-01") o con codice spuria in coda.
    if (std::sscanf(s.c_str(), "%d-%d-%dT%d:%d:%d%n", &year, &month, &day,
                    &hour, &minute, &second, &consumed) != 6 ||
        static_cast<std::size_t>(consumed) != s.size()) {
        return false;
    }

    // Validazione calendario (mesi, giorni, anni bisestili).
    std::chrono::year_month_day ymd{
        std::chrono::year{year},
        std::chrono::month{static_cast<unsigned>(month)},
        std::chrono::day{static_cast<unsigned>(day)}};
    if (!ymd.ok() || hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
        second < 0 || second > 60) {
        return false;
    }

    std::tm tm{};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;

    std::time_t t = timegm(&tm); // interpreta il tempo come UTC
    if (t == static_cast<std::time_t>(-1)) {
        return false;
    }
    out = events::TimePoint(std::chrono::seconds(t));
    return true;
}

std::string toIso8601(events::TimePoint tp) {
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
    gmtime_r(&t, &tm);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

} // namespace server
