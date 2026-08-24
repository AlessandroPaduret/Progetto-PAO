#ifndef ACTIVITY_BUILDER_H
#define ACTIVITY_BUILDER_H

#include <chrono>
#include <memory>
#include <unordered_set>
#include <vector>

#include "events/core/Activity.h"
#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"
#include "events/domain/Meeting.h"
#include "events/domain/Task.h"

namespace events {

/**
 * @class ActivityBuilder
 * @brief Builder (fluent) per la creazione delle attivita'.
 *
 * Accetta un generatore esplicito con addGenerator; se non viene aggiunto
 * alcun generatore, build() usa il fallback SingleGenerator(start): una
 * Activity costruita senza generatore e' quindi un evento singolo.
 */
class ActivityBuilder {
protected:
    String m_title;
    TimePoint m_start;
    Duration m_duration = Duration::zero();
    std::shared_ptr<DateGenerator> m_generator;  ///< nullptr = fallback SingleGenerator
    std::unordered_set<TimePoint, TimePointHasher> m_exceptions;

    /** @return m_generator oppure SingleGenerator(m_start) come fallback */
    std::shared_ptr<DateGenerator> resolveGenerator() const;

public:
    /** @brief Costruttore.
     *  @param title Titolo dell'attivita'
     *  @param start Istante di riferimento (inizio/scadenza)
     */
    ActivityBuilder(String title, TimePoint start);

    /** @brief Imposta la durata delle occorrenze */
    ActivityBuilder& withDuration(Duration duration);

    /** @brief Imposta la regola di ricorrenza esplicita
     *  @return *this per il concatenamento fluente
     */
    ActivityBuilder& addGenerator(std::shared_ptr<DateGenerator> generator);

    /** @brief Aggiunge un'eccezione (occorenza esclusa dalla serie) */
    ActivityBuilder& addException(TimePoint tp);

    /** @brief Costruisce l'attivita' (evento singolo se non c'e' un generatore) */
    Activity build() const;
};

/**
 * @class TaskBuilder
 * @brief Builder derivato per i compiti (aggiunge priorita' e stato).
 */
class TaskBuilder : public ActivityBuilder {
private:
    Priority m_priority = Priority::Medium;
    bool m_done = false;

public:
    /** @brief Costruttore.
     *  @param title Titolo del compito
     *  @param due Scadenza (istante di riferimento)
     */
    TaskBuilder(String title, TimePoint due);

    /** @brief Imposta la priorita' del compito */
    TaskBuilder& withPriority(Priority priority);

    /** @brief Segna il compito come evaso */
    TaskBuilder& makeCheckable();

    /** @brief Costruisce il compito (evento singolo se non c'e' un generatore) */
    Task build() const;
};

/**
 * @class MeetingBuilder
 * @brief Builder derivato per le riunioni (aggiunge luogo e partecipanti).
 */
class MeetingBuilder : public ActivityBuilder {
private:
    String m_location;
    std::vector<String> m_attendees;

public:
    /** @brief Costruttore.
     *  @param title Titolo della riunione
     *  @param start Inizio
     */
    MeetingBuilder(String title, TimePoint start);

    /** @brief Imposta il luogo o link della riunione */
    MeetingBuilder& withLocation(const String& location);

    /** @brief Aggiunge un partecipante (i duplicati sono rifiutati) */
    MeetingBuilder& addAttendee(const String& attendee);

    /** @brief Costruisce la riunione */
    Meeting build() const;
};

} // namespace events

#endif // ACTIVITY_BUILDER_H