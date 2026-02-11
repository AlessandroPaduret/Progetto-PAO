
#include <algorithm>
#include <vector>
#include <chrono>

#include "events/core/CommonTypes.h"
#include "events/generators/YearlyGenerator.h"

using namespace std::chrono;

namespace events {

YearlyGenerator::YearlyGenerator(TimePoint start, TimePoint end)
    : m_start(start), m_end(end) {}

TimePoint YearlyGenerator::getStart() const { return m_start; }
TimePoint YearlyGenerator::getEnd() const { return m_end; }

// Per un generatore annuale, l'intervallo non è fisso in secondi (causa bisestili)
// ma possiamo restituire una stima media o 365 giorni se richiesto dall'interfaccia.
Duration YearlyGenerator::getInterval() const { 
    return duration_cast<Duration>(days{365}); 
}

void YearlyGenerator::setStart(TimePoint newStart) { m_start = newStart; }
void YearlyGenerator::setEnd(TimePoint newEnd) { m_end = newEnd; }

// Metodo vuoto o che lancia eccezione se YearlyGenerator non permette intervalli custom
void YearlyGenerator::setInterval(Duration) {
    // La ricorrenza annuale è logicamente fissa a 1 anno calendariale
}

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

bool YearlyGenerator::occursInRange(TimePoint from, TimePoint to) const {
    // Sfruttiamo generateDates: se il vettore non è vuoto, allora esiste
    return !generateDates(from, to).empty();
}

} // namespace events  