#include "core/Activity.h"

#include <stdexcept>
#include <utility>
#include <algorithm>

#include "core/ActivityVisitor.h"
#include "generators/SingleGenerator.h"

namespace events {

namespace {
// Helper per il generatore di default se nullptr
std::shared_ptr<const DateGenerator> getDefaultGenerator() {
    static const auto singleGen = std::make_shared<const SingleGenerator>();
    return singleGen;
}
} // namespace

Activity::Activity(String title,
                   TimePoint start,
                   Duration duration,
                   std::shared_ptr<const DateGenerator> generator,
                   TimePoint end)
    : m_title(std::move(title)),
      m_start(start),
      m_end(end),
      m_duration(duration),
      m_generator(generator ? std::move(generator) : getDefaultGenerator()) {
    if (m_duration < Duration::zero()) {
        throw std::invalid_argument("La durata dell'attivita' non puo' essere negativa.");
    }
}

std::unique_ptr<Activity> Activity::clone() const {
    return std::make_unique<Activity>(*this);
}

void Activity::setDuration(Duration duration) {
    if (duration < Duration::zero()) {
        throw std::invalid_argument("La durata dell'attivita' non puo' essere negativa.");
    }
    m_duration = duration;
}

const DateGenerator& Activity::getGenerator() const {
    return *m_generator;
}

void Activity::setGenerator(std::shared_ptr<const DateGenerator> generator) {
    if (generator) 
        m_generator = std::move(generator);
}

bool Activity::addException(TimePoint tp) {
    // Un'eccezione e' valida solo se rientra nella finestra dell'attivita'
    if (tp < m_start || tp > m_end) {
        return false;
    }
    
    // Verifichiamo se la data corrisponde a un'occorrenza generata
    TimePoint aligned = m_generator->align(m_start, tp);
    if (aligned == tp) {
        return m_exceptions.insert(tp).second;
    }
    return false;
}

std::vector<Occurrence> Activity::occurrencesIn(TimePoint from, TimePoint to) const {
    if (!m_generator || from > to || m_start > to || m_start > m_end) return {};

    TimePoint searchTo = std::min(m_end, to);

    // Un'occorrenza iniziata PRIMA di `from` puo' comunque essere visibile
    // nella finestra se la sua durata la fa arrivare fino a `from` (es. un
    // evento a cavallo di mezzanotte quando `from` e' l'inizio del giorno
    // dopo): il generatore userebbe altrimenti align(start, from), che salta
    // dritto alla prima occorrenza con INIZIO >= from, perdendo per sempre
    // quella precedente anche se si sovrappone. Si allarga percio' la
    // ricerca a partire da `from - m_duration` (la piu' vecchia che possa
    // ancora raggiungere `from`) e si scarta poi, con Occurrence::overlaps,
    // qualunque occorrenza che non tocchi davvero [from, to].
    const TimePoint searchFrom = from - m_duration;

    // Mappa ogni TimePoint valido in un oggetto Occurrence
    auto toOccurrence = std::views::transform([this](TimePoint tp) {
        return Occurrence{this, tp, m_duration};
    });

    return m_generator->occurrences(m_start, searchFrom, searchTo)
            | std::views::filter([this](TimePoint tp) { return !m_exceptions.contains(tp); })
            | toOccurrence
            | std::views::filter([from, to](const Occurrence& occ) { return occ.overlaps(from, to); })
            | std::ranges::to<std::vector<Occurrence>>();
}

String Activity::describe() const {
    String desc = "Activity: " + m_title + "\n";
    desc += "Durata: " + std::to_string(m_duration.count()) + "s\n";
    desc += "Eccezioni impostate: " + std::to_string(m_exceptions.size());
    return desc;
}

void Activity::accept(ActivityVisitor& visitor) const {
    visitor.visit(*this);
}

} // namespace events