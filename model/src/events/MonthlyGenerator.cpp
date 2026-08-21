#include <chrono>
#include <cstddef>
#include <sstream>
#include <vector>

#include "events/core/CommonTypes.h"
#include "events/generators/MonthlyGenerator.h"

namespace events {

MonthlyGenerator::MonthlyGenerator(TimePoint start, int months, TimePoint end)
    : m_start(start), m_months(months > 0 ? months : 1), m_end(end) {}

TimePoint MonthlyGenerator::getStart() const { return m_start; }

int MonthlyGenerator::getMonths() const { return m_months; }

void MonthlyGenerator::setMonths(int months) {
  m_months = months > 0 ? months : 1;
}

TimePoint MonthlyGenerator::getEnd() const { return m_end; }

void MonthlyGenerator::setStart(TimePoint newStart) { m_start = newStart; }

void MonthlyGenerator::setEnd(TimePoint newEnd) { m_end = newEnd; }

void MonthlyGenerator::setMaxOccurrences(std::size_t n) {
  m_maxOccurrences = n;
}

std::size_t MonthlyGenerator::getMaxOccurrences() const {
  return m_maxOccurrences;
}

std::vector<TimePoint> MonthlyGenerator::generateDates(
    const TimePoint from, const TimePoint to) const {
  std::vector<TimePoint> dates;

  const std::chrono::sys_days baseDays = std::chrono::floor<std::chrono::days>(m_start);
  const Duration timeOffset = m_start - baseDays;
  const std::chrono::year_month_day base =
      std::chrono::year_month_day{baseDays};
  const std::chrono::year_month baseYm = base.year() / base.month();

  std::size_t k = 0;
  while (true) {
    if (m_maxOccurrences > 0 && k >= m_maxOccurrences) {
      break;
    }
    const auto shiftedYm = baseYm + std::chrono::months{k * m_months};
    // Giorno del mese con clamping (es. 31 -> ultimo giorno del mese)
    std::chrono::year_month_day candidate = shiftedYm / base.day();
    if (!candidate.ok()) {
      candidate = std::chrono::year_month_day{shiftedYm / std::chrono::last};
    }
    const TimePoint tp =
        std::chrono::time_point_cast<Duration>(std::chrono::sys_days{candidate}) +
        timeOffset;
    if (tp > to || tp > m_end) {
      break;
    }
    if (tp >= from && tp >= m_start) {
      dates.push_back(tp);
    }
    ++k;
  }
  return dates;
}

bool MonthlyGenerator::occursInRange(const TimePoint from,
                                     const TimePoint to) const {
  return !generateDates(from, to).empty();
}

String MonthlyGenerator::describe() const {
  std::ostringstream oss;
  oss << "[MonthlyGenerator] every " << m_months << " month(s) starting at "
      << m_start.time_since_epoch().count()
      << " and ending at " << m_end.time_since_epoch().count();
  return oss.str();
}

void MonthlyGenerator::accept(DateGeneratorVisitor &visitor) const {
  visitor.visit(*this);
}

} // namespace events