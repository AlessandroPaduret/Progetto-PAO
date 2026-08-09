#include "mappers.h"

#include <memory>

#include "events/generators/FixedIntervalGenerator.h"
#include "events/generators/YearlyGenerator.h"

#include "iso8601.h"

namespace server {

namespace {
// EventRecord usa time_point<system_clock, nanoseconds>; il modello usa
// time_point<system_clock, seconds>: la conversione richiede un cast esplicito.
events::TimePoint toModelTime(std::chrono::system_clock::time_point tp) {
    return std::chrono::time_point_cast<std::chrono::seconds>(tp);
}
} // namespace

std::unique_ptr<events::Event> toSimpleEvent(const db::EventRecord& record) {
    return std::make_unique<events::Event>(record.title,
                                           toModelTime(record.start),
                                           events::Duration(record.duration));
}

std::unique_ptr<events::RecurrentEvent> toRecurrentEvent(const db::EventRecord& record) {
    events::TimePoint start = toModelTime(record.start);
    events::TimePoint end =
        record.end ? toModelTime(*record.end) : events::TimePoint::max();

    std::shared_ptr<events::DateGenerator> generator;
    if (record.kind == db::RecurrenceKind::Yearly) {
        generator = std::make_shared<events::YearlyGenerator>(start, end);
    } else {
        generator = std::make_shared<events::FixedIntervalGenerator>(
            start, events::Duration(record.interval), end);
    }

    auto recurrent = std::make_unique<events::RecurrentEvent>(
        generator,
        events::Event(record.title, start, events::Duration(record.duration)));

    for (const auto& ex : record.exceptions) {
        recurrent->addException(toModelTime(ex));
    }
    return recurrent;
}

nlohmann::json occurrenceToJson(long long eventId, const events::Event& occurrence,
                                const std::string& type) {
    return nlohmann::json{
        {"event_id", eventId},
        {"title", occurrence.getTitle()},
        {"start", toIso8601(occurrence.getStart())},
        {"end", toIso8601(occurrence.getEnd())},
        {"type", type},
    };
}

} // namespace server
