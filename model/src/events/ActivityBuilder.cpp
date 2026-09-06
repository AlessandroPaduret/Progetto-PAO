#include "events/builders/ActivityBuilder.h"
#include "events/generators/SingleGenerator.h"
#include <utility>

namespace events {

ActivityBuilder::ActivityBuilder(String title, TimePoint start)
    : m_title(std::move(title)), m_start(start) {}

std::shared_ptr<const DateGenerator> ActivityBuilder::resolveGenerator() {
    if (m_generator) {
        return m_generator;
    }
    static const auto defaultSingle = std::make_shared<const SingleGenerator>();
    return defaultSingle;
}

ActivityBuilder& ActivityBuilder::withEnd(TimePoint end) {
    m_end = end;
    return *this;
}

ActivityBuilder& ActivityBuilder::withDuration(Duration duration) {
    m_duration = duration;
    return *this;
}

ActivityBuilder& ActivityBuilder::addGenerator(std::shared_ptr<const DateGenerator> generator) {
    m_generator = std::move(generator);
    return *this;
}

ActivityBuilder& ActivityBuilder::addException(TimePoint tp) {
    m_exceptions.insert(tp);
    return *this;
}

ActivityBuilder& ActivityBuilder::stopAfter(std::size_t maxOccurrences) {
    m_maxOccurrences = maxOccurrences;
    return *this;
}

std::unique_ptr<Activity> ActivityBuilder::build() {

    // calcola la data finale che avresti avuto con le m_maxOccurences

    m_generator = resolveGenerator();

    if (m_maxOccurrences > 0){
        TimePoint curr = m_start;

        for(unsigned long int i = 0; i < m_maxOccurrences-1; ++i){
            curr = m_generator->next(curr);
        }

        m_end = std::min(m_end, curr);
    }

    auto activity = std::make_unique<Activity>(
        m_title, m_start, m_duration, m_generator, m_end
    );
    for (const TimePoint tp : m_exceptions) {
        activity->addException(tp);
    }
    return activity;
}

// --- TaskBuilder ---

TaskBuilder::TaskBuilder(String title, TimePoint due)
    : ActivityBuilder(std::move(title), due) {}

TaskBuilder& TaskBuilder::withEnd(TimePoint end) {
    ActivityBuilder::withEnd(end);
    return *this;
}

TaskBuilder& TaskBuilder::withDuration(Duration duration) {
    ActivityBuilder::withDuration(duration);
    return *this;
}

TaskBuilder& TaskBuilder::addGenerator(std::shared_ptr<const DateGenerator> generator) {
    ActivityBuilder::addGenerator(std::move(generator));
    return *this;
}

TaskBuilder& TaskBuilder::addException(TimePoint tp) {
    ActivityBuilder::addException(tp);
    return *this;
}

TaskBuilder& TaskBuilder::stopAfter(std::size_t maxOccurrences) {
    ActivityBuilder::stopAfter(maxOccurrences);
    return *this;
}

TaskBuilder& TaskBuilder::withPriority(Priority priority) {
    m_priority = priority;
    return *this;
}

TaskBuilder& TaskBuilder::withDone(bool done) {
    m_done = done;
    return *this;
}

std::unique_ptr<Activity> TaskBuilder::build() {
    auto task = std::make_unique<Task>(
        m_title, m_start, m_duration, m_priority, resolveGenerator(), m_end
    );
    task->setDone(m_done);
    for (const TimePoint tp : m_exceptions) {
        task->addException(tp);
    }
    return task;
}

// --- MeetingBuilder ---

MeetingBuilder::MeetingBuilder(String title, TimePoint start)
    : ActivityBuilder(std::move(title), start) {}

MeetingBuilder& MeetingBuilder::withEnd(TimePoint end) {
    ActivityBuilder::withEnd(end);
    return *this;
}

MeetingBuilder& MeetingBuilder::withDuration(Duration duration) {
    ActivityBuilder::withDuration(duration);
    return *this;
}

MeetingBuilder& MeetingBuilder::addGenerator(std::shared_ptr<const DateGenerator> generator) {
    ActivityBuilder::addGenerator(std::move(generator));
    return *this;
}

MeetingBuilder& MeetingBuilder::addException(TimePoint tp) {
    ActivityBuilder::addException(tp);
    return *this;
}

MeetingBuilder& MeetingBuilder::stopAfter(std::size_t maxOccurrences) {
    ActivityBuilder::stopAfter(maxOccurrences);
    return *this;
}

MeetingBuilder& MeetingBuilder::withLocation(const String& location) {
    m_location = location;
    return *this;
}

MeetingBuilder& MeetingBuilder::addAttendee(const String& attendee) {
    m_attendees.push_back(attendee);
    return *this;
}

std::unique_ptr<Activity> MeetingBuilder::build() {
    auto meeting = std::make_unique<Meeting>(
        m_title, m_start, m_duration, m_location, resolveGenerator(), m_end
    );
    for (const String& attendee : m_attendees) {
        meeting->addAttendee(attendee);
    }
    for (const TimePoint tp : m_exceptions) {
        meeting->addException(tp);
    }
    return meeting;
}

} // namespace events