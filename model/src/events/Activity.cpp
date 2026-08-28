#include "events/core/Activity.h"

#include <stdexcept>
#include <utility>

#include "events/core/ActivityVisitor.h"
#include "events/core/DateGenerator.h"
#include "events/generators/SingleGenerator.h"

namespace events {

// Helper interno per la creazione del generatore di default
namespace {
std::unique_ptr<DateGenerator> makeDefaultGenerator() {
    return std::make_unique<SingleGenerator>(
        std::chrono::time_point_cast<std::chrono::seconds>(
            std::chrono::system_clock::now()));
}
} // namespace

// -----------------------------------------------------------------------------
// Costruzione e Clonazione
// -----------------------------------------------------------------------------

Activity::Activity(String title, Duration duration, std::unique_ptr<DateGenerator> generator)
    : m_title(std::move(title)),
      m_duration(duration),
      m_generator(generator ? std::move(generator) : makeDefaultGenerator()) {
    if (m_duration < Duration::zero()) {
        throw std::invalid_argument("La durata dell'attivita' non puo' essere negativa.");
    }
}

std::unique_ptr<Activity> Activity::clone() const {
    // Creiamo una nuova istanza duplicata profonda
    auto clonedGen = m_generator ? m_generator->clone() : makeDefaultGenerator();
    auto clonedActivity = std::make_unique<Activity>(m_title, m_duration, std::move(clonedGen));
    clonedActivity->m_exceptions = m_exceptions;
    return clonedActivity;
}

// -----------------------------------------------------------------------------
// Helper protetti
// -----------------------------------------------------------------------------

std::vector<TimePoint> Activity::occurrenceDates(TimePoint from, TimePoint to) const {
    if (!m_generator) {
        return {};
    }

    // 1. Genera tutte le date nell'intervallo dal DateGenerator
    std::vector<TimePoint> rawDates = m_generator->generateDates(from, to);
    std::vector<TimePoint> filteredDates;
    filteredDates.reserve(rawDates.size());

    // 2. Filtra escludendo quelle presenti in m_exceptions
    for (const auto& tp : rawDates) {
        if (m_exceptions.find(tp) == m_exceptions.end()) {
            filteredDates.push_back(tp);
        }
    }

    return filteredDates;
}

// -----------------------------------------------------------------------------
// Query dello Stato e Accessor
// -----------------------------------------------------------------------------

String Activity::getTitle() const {
    return m_title;
}

void Activity::setTitle(const String& title) {
    m_title = title;
}

TimePoint Activity::getStart() const {
    return m_generator ? m_generator->getStart() : TimePoint::min();
}

TimePoint Activity::getEnd() const {
    return m_generator ? m_generator->getEnd() : TimePoint::max();
}

Duration Activity::getDuration() const {
    return m_duration;
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

const std::unordered_set<TimePoint, TimePointHasher>& Activity::getExceptions() const {
    return m_exceptions;
}

// -----------------------------------------------------------------------------
// Modifica del Generatore e della Serie
// -----------------------------------------------------------------------------

void Activity::setStart(TimePoint start) {
    // Il generatore e' mutabile: la serie si sposta con il suo setter senza
    // ricostruirla (la fine NON slitta; se il nuovo inizio la supera, il
    // generatore la allinea al nuovo inizio).
    if (m_generator) {
        m_generator->setStart(start);
    } else {
        m_generator = std::make_unique<SingleGenerator>(start);
    }
}

void Activity::setEnd(TimePoint end) {
    if (m_generator) {
        m_generator->setEnd(end);
    }
}

void Activity::setGenerator(std::unique_ptr<DateGenerator> generator) {
    if (!generator) {
        m_generator = makeDefaultGenerator();
    } else {
        m_generator = std::move(generator);
    }
}

// -----------------------------------------------------------------------------
// Gestione delle Occorrenze ed Eccezioni
// -----------------------------------------------------------------------------

bool Activity::addException(TimePoint tp) {
    // L'eccezione viene accettata SOLO se la data e' generabile dalla serie attuale
    if (m_generator && m_generator->isIn(tp)) {
        return m_exceptions.insert(tp).second;
    }
    return false;
}

void Activity::clearExceptions() {
    m_exceptions.clear();
}

std::vector<Occurrence> Activity::occurrencesIn(TimePoint from, TimePoint to) const {
    std::vector<TimePoint> dates = occurrenceDates(from, to);
    std::vector<Occurrence> result;
    result.reserve(dates.size());

    for (const auto& startTp : dates) {
        result.push_back(Occurrence{this, startTp, m_duration});
    }

    return result;
}

// -----------------------------------------------------------------------------
// Ispezione e Visitor Pattern
// -----------------------------------------------------------------------------

String Activity::describe() const {
    String desc = "Activity: " + m_title + "\n";
    desc += "Durata: " + std::to_string(m_duration.count()) + "s\n";
    if (m_generator) {
        desc += "Ricorrenza: " + m_generator->describe() + "\n";
    }
    desc += "Eccezioni impostate: " + std::to_string(m_exceptions.size());
    return desc;
}

void Activity::accept(ActivityVisitor& visitor) const {
    visitor.visit(*this);
}

} // namespace events