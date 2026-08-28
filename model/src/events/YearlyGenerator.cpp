
#include <algorithm>
#include <vector>
#include <chrono>
#include <sstream>

#include "events/core/CommonTypes.h"
#include "events/core/DateGeneratorVisitor.h"
#include "events/generators/YearlyGenerator.h"

using namespace std::chrono;

namespace events {


///Implementazione di DateGenerator - Ciclo di Vita

YearlyGenerator::YearlyGenerator(TimePoint start, TimePoint end)
    : m_start(start), m_end(end) {}

std::unique_ptr<DateGenerator> YearlyGenerator::clone() const {
  return std::make_unique<YearlyGenerator>(m_start, m_end);
}


/// Query dello Stato e Accessor Specifici

TimePoint YearlyGenerator::getStart() const { return m_start; }

TimePoint YearlyGenerator::getEnd() const { return m_end; }

void YearlyGenerator::setStart(TimePoint start) {
  if (start > m_end) {
    m_end = start;  // la fine non resta antecedente al nuovo inizio
  }
  m_start = start;
}

void YearlyGenerator::setEnd(TimePoint end) { m_end = end; }

Duration YearlyGenerator::getInterval() const { 
    return duration_cast<Duration>(days{365}); 
}


/// Algoritmi di Generazione e Verifica Date

std::vector<TimePoint> YearlyGenerator::generateDates(TimePoint from, TimePoint to) const {
    std::vector<TimePoint> dates;

    // 1. Estraiamo il "giorno del compleanno" (Mese e Giorno)
    // Convertiamo il TimePoint m_start in year_month_day
    auto start_ds = floor<days>(m_start);
    year_month_day original = year_month_day{start_ds};
    auto month = original.month();
    auto day = original.day();

    // 2. Determiniamo il range di anni da controllare
    year start_year = year_month_day{floor<days>(from)}.year();
    year end_year = year_month_day{floor<days>(to)}.year();

    // 3. Iteriamo solo sugli anni del range
    const year base_year = year_month_day{floor<days>(m_start)}.year();
    for (year y = start_year; y <= end_year; ++y) {

        // Creiamo il candidato per l'anno corrente
        year_month_day candidate = y / month / day;

        // Gestione Bisestili: se il 29 Febbraio non esiste in questo anno, 
        // la convenzione standard è spostarlo al 28 Febbraio (o 1 Marzo)
        if (!candidate.ok()) {
            candidate = y / month / 28;
        }

        // Convertiamo il candidato in TimePoint (a mezzanotte)
        // Manteniamo però l'offset orario originale di m_start se presente
        auto candidate_tp = time_point_cast<Duration>(sys_days{candidate}) + (m_start - start_ds);

        // Verifichiamo che sia nel range richiesto [from, to] 
        // e che non superi la fine del generatore m_end
        if (candidate_tp >= from && candidate_tp <= to && candidate_tp <= m_end && candidate_tp >= m_start) {
            dates.push_back(candidate_tp);
        }
    }

    return dates;
}

bool YearlyGenerator::isIn(TimePoint tp) const {
  if (tp < m_start || tp > m_end) {
    return false;
  }

  auto start_ds = floor<days>(m_start);
  year_month_day original = year_month_day{start_ds};
  auto month = original.month();
  auto day = original.day();

  auto tp_ds = floor<days>(tp);
  // l'ora del giorno deve corrispondere a quella di m_start
  if (tp_ds + (m_start - start_ds) != tp) {
    return false;
  }

  year_month_day candidate = year_month_day{tp_ds}.year() / month / day;
  if (!candidate.ok()) {
    candidate = year_month_day{tp_ds}.year() / month / 28;
  }

  const year base_year = original.year();
  const year tp_year = year_month_day{tp_ds}.year();

  return sys_days{candidate} == tp_ds;
}


/// Ispezione e Serializzazione

String YearlyGenerator::describe() const {
    std::ostringstream oss;
    auto start_ymd = year_month_day{floor<days>(m_start)};
    oss << "[YearlyGenerator] starting on " << start_ymd.month() << "/" << start_ymd.day();
    if (m_end != TimePoint::max()) {
        auto end_ymd = year_month_day{floor<days>(m_end)};
        oss << " until " << end_ymd.year() << "/" << end_ymd.month() << "/" << end_ymd.day();
    }
    return oss.str();
}

void YearlyGenerator::accept(DateGeneratorVisitor& visitor) const {
  visitor.visit(*this);
}

} // namespace events  