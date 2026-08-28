#include <chrono>
#include <cstddef>
#include <memory>
#include <sstream>
#include <type_traits>
#include <vector>

#include "events/core/CommonTypes.h"
#include "events/generators/MonthlyGenerator.h"

namespace events {


/// Implementazione di DateGenerator - Ciclo di Vita

MonthlyGenerator::MonthlyGenerator(TimePoint start, int months, TimePoint end)
    : m_start(start), m_months(months > 0 ? months : 1), m_end(end) {}

std::unique_ptr<DateGenerator> MonthlyGenerator::clone() const {
  return std::make_unique<MonthlyGenerator>(m_start, m_months, m_end);
}


/// Query dello Stato e Accessor Specifici

TimePoint MonthlyGenerator::getStart() const { return m_start; }

int MonthlyGenerator::getMonths() const { return m_months; }

void MonthlyGenerator::setMonths(int months) {
  m_months = months > 0 ? months : 1;
}

TimePoint MonthlyGenerator::getEnd() const { return m_end; }

void MonthlyGenerator::setStart(TimePoint start) {
  if (start > m_end) {
    m_end = start;  // la fine non resta antecedente al nuovo inizio
  }
  m_start = start;
}

void MonthlyGenerator::setEnd(TimePoint end) { m_end = end; }


/// Algoritmi di Generazione e Verifica Date

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

bool MonthlyGenerator::isIn(TimePoint tp) const {
  if (tp < m_start || tp > m_end) {
    return false;
  }

  const std::chrono::sys_days baseDays =
      std::chrono::floor<std::chrono::days>(m_start);
  const Duration timeOffset = m_start - baseDays;
  const std::chrono::year_month_day base =
      std::chrono::year_month_day{baseDays};
  const std::chrono::year_month baseYm = base.year() / base.month();

  const std::chrono::sys_days tpDays = std::chrono::floor<std::chrono::days>(tp);
  if (tp - tpDays != timeOffset) {
    // l'ora del giorno non corrisponde a quella di m_start
    return false;
  }
  const std::chrono::year_month_day tpYmd{tpDays};
  const std::chrono::year_month tpYm = tpYmd.year() / tpYmd.month();

  // Numero di passi (di m_months mesi) necessari a raggiungere il mese di tp.
  const int monthsBetween =
      static_cast<int>((tpYm.year() - baseYm.year()).count()) * 12 +
      static_cast<int>(static_cast<unsigned>(tpYm.month())) -
      static_cast<int>(static_cast<unsigned>(baseYm.month()));
  if (monthsBetween < 0 || monthsBetween % m_months != 0) {
    return false;
  }
  
  // Il candidato per quel passo (con clamping del giorno) deve coincidere con tp.
  const auto shiftedYm = baseYm + std::chrono::months{monthsBetween};
  std::chrono::year_month_day candidate = shiftedYm / base.day();
  if (!candidate.ok()) {
    candidate = std::chrono::year_month_day{shiftedYm / std::chrono::last};
  }
  const TimePoint candidateTp =
      std::chrono::time_point_cast<Duration>(std::chrono::sys_days{candidate}) +
      timeOffset;
  return candidateTp == tp;
}


/// Ispezione e Serializzazione

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