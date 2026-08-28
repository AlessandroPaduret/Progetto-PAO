#ifndef ACTIVITY_BUILDER_H
#define ACTIVITY_BUILDER_H

#include <chrono>
#include <cstddef>
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
    std::unique_ptr<DateGenerator> m_generator; ///< nullptr = fallback SingleGenerator
    std::unordered_set<TimePoint, TimePointHasher> m_exceptions;

    std::unique_ptr<DateGenerator> resolveGenerator();

public:
    /** @brief Costruttore.
     *  @param title Titolo dell'attivita'
     *  @param start Istante di riferimento (inizio/scadenza). Opzionale: se si
     *               aggiunge un generatore esplicito con addGenerator, l'inizio
     *               arriva da quello; senza generatore si usa il fallback
     *               SingleGenerator(start) (default: adesso).
     */
    ActivityBuilder(String title,
                    TimePoint start = std::chrono::time_point_cast<std::chrono::seconds>(
                        Clock::now()));

    /** @brief Imposta la durata delle occorrenze */
    ActivityBuilder& withDuration(Duration duration);

    /** @brief Imposta la regola di ricorrenza esplicita
     *  @return *this per il concatenamento fluente
     */
    ActivityBuilder& addGenerator(std::unique_ptr<DateGenerator> generator);

    /** @brief Aggiunge un'eccezione (occorenza esclusa dalla serie) */
    ActivityBuilder& addException(TimePoint tp);

    /** @brief imposta il numeoro massimo di occorrenze dell'attività da creare
     *  @param maxOccurrences numero massimo di occorrenze della attività da creare
     *  @return *this per il concatenamento fluente
     */
    ActivityBuilder& withMaxOccurrences(std::size_t maxOccurrences);

    // Rimosso const: consuma lo stato del builder via move
    Activity build(); 
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

    /** @brief Imposta la durata delle occorrenze (mantiene il tipo derivato) */
    TaskBuilder& withDuration(Duration duration);

    /** @brief Imposta la priorita' del compito */
    TaskBuilder& withPriority(Priority priority);

    // Rimosso const
    Task build(); 
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

    /** @brief Imposta la durata delle occorrenze (mantiene il tipo derivato) */
    MeetingBuilder& withDuration(Duration duration);

    /** @brief Imposta il luogo o link della riunione */
    MeetingBuilder& withLocation(const String& location);

    /** @brief Aggiunge un partecipante (i duplicati sono rifiutati) */
    MeetingBuilder& addAttendee(const String& attendee);

    // Rimosso const
    Meeting build(); 
};

} // namespace events

#endif // ACTIVITY_BUILDER_HZ