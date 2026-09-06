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
 */
class ActivityBuilder {
protected:
    String m_title;
    TimePoint m_start;
    TimePoint m_end = TimePoint::max();
    Duration m_duration = Duration::zero();
    std::shared_ptr<const DateGenerator> m_generator; ///< nullptr = fallback SingleGenerator
    std::unordered_set<TimePoint> m_exceptions;
    std::size_t m_maxOccurrences = 0;

    std::shared_ptr<const DateGenerator> resolveGenerator();

public:
    virtual ~ActivityBuilder() = default;

    /** 
     * @brief Costruttore con titolo e data di inizio obbligatori.
     */
    ActivityBuilder(String title, TimePoint start);

    /** @brief Imposta la data/ora di fine serie */
    ActivityBuilder& withEnd(TimePoint end);

    /** @brief Imposta la durata delle occorrenze */
    ActivityBuilder& withDuration(Duration duration);

    /** @brief Imposta la regola di ricorrenza condivisa */
    ActivityBuilder& addGenerator(std::shared_ptr<const DateGenerator> generator);

    /** @brief Aggiunge un'eccezione alla serie */
    ActivityBuilder& addException(TimePoint tp);

    /** @brief Imposta il numero massimo di occorrenze */
    ActivityBuilder& stopAfter(std::size_t maxOccurrences);

    /** @brief Costruisce l'istanza concreta di Activity */
    virtual std::unique_ptr<Activity> build(); 
};

/**
 * @class TaskBuilder
 * @brief Builder derivato per i compiti.
 */
class TaskBuilder : public ActivityBuilder {
private:
    Priority m_priority = Priority::Medium;
    bool m_done = false;

public:
    TaskBuilder(String title, TimePoint due);

    TaskBuilder& withEnd(TimePoint end);
    TaskBuilder& withDuration(Duration duration);
    TaskBuilder& addGenerator(std::shared_ptr<const DateGenerator> generator);
    TaskBuilder& addException(TimePoint tp);
    TaskBuilder& stopAfter(std::size_t maxOccurrences);

    TaskBuilder& withPriority(Priority priority);
    TaskBuilder& withDone(bool done);

    std::unique_ptr<Activity> build() override; 
};

/**
 * @class MeetingBuilder
 * @brief Builder derivato per le riunioni.
 */
class MeetingBuilder : public ActivityBuilder {
private:
    String m_location;
    std::vector<String> m_attendees;

public:
    MeetingBuilder(String title, TimePoint start);

    MeetingBuilder& withEnd(TimePoint end);
    MeetingBuilder& withDuration(Duration duration);
    MeetingBuilder& addGenerator(std::shared_ptr<const DateGenerator> generator);
    MeetingBuilder& addException(TimePoint tp);
    MeetingBuilder& stopAfter(std::size_t maxOccurrences);

    MeetingBuilder& withLocation(const String& location);
    MeetingBuilder& addAttendee(const String& attendee);

    std::unique_ptr<Activity> build() override; 
};

} // namespace events

#endif // ACTIVITY_BUILDER_H