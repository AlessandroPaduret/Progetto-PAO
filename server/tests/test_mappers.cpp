#include <chrono>

#include <catch2/catch_all.hpp>

#include "db/EventRepository.h"
#include "mappers.h"

using events::TimePoint;

namespace {
std::chrono::system_clock::time_point atEpoch(long long epochSeconds) {
    return std::chrono::system_clock::time_point(std::chrono::seconds(epochSeconds));
}
} // namespace

TEST_CASE("toSimpleEvent mappa i campi base", "[mappers]") {
    db::EventRecord rec;
    rec.title = "Riunione";
    rec.start = atEpoch(1767225600); // 2026-01-01T00:00:00 UTC
    rec.duration = std::chrono::hours(1);

    auto event = server::toSimpleEvent(rec);
    REQUIRE(event->getTitle() == "Riunione");
    REQUIRE(event->getStart() == TimePoint(std::chrono::seconds(1767225600)));
    REQUIRE(event->getDuration() == std::chrono::hours(1));
    REQUIRE(event->getEnd() == TimePoint(std::chrono::seconds(1767229200)));
}

TEST_CASE("toRecurrentEvent fixed applica intervallo ed eccezioni", "[mappers]") {
    db::EventRecord rec;
    rec.title = "Settimanale";
    rec.start = atEpoch(1767225600);
    rec.duration = std::chrono::minutes(30);
    rec.kind = db::RecurrenceKind::Fixed;
    rec.interval = std::chrono::days(7);
    rec.end = atEpoch(1767225600) + std::chrono::days(21);
    rec.exceptions.push_back(atEpoch(1767225600) + std::chrono::days(7));

    auto recurrent = server::toRecurrentEvent(rec);
    auto occurrences = recurrent->getSchedulable(std::chrono::time_point_cast<std::chrono::seconds>(rec.start),
                                                 std::chrono::time_point_cast<std::chrono::seconds>(*rec.end));
    REQUIRE(occurrences.size() == 3); // +7d è un'eccezione
    REQUIRE(occurrences[0]->getStart() ==
            TimePoint(std::chrono::seconds(1767225600)));
    REQUIRE(occurrences[1]->getStart() ==
            TimePoint(std::chrono::seconds(1767225600 + 14 * 86400)));
    REQUIRE(occurrences[2]->getStart() ==
            TimePoint(std::chrono::seconds(1767225600 + 21 * 86400)));
}

TEST_CASE("toRecurrentEvent yearly genera un'occorrenza l'anno", "[mappers]") {
    db::EventRecord rec;
    rec.title = "Compleanno";
    rec.start = atEpoch(1767225600); // 2026-01-01
    rec.duration = std::chrono::hours(24) - std::chrono::seconds(1);
    rec.kind = db::RecurrenceKind::Yearly;
    rec.end = atEpoch(1767225600) + std::chrono::days(400);

    auto recurrent = server::toRecurrentEvent(rec);
    auto occurrences = recurrent->getSchedulable(std::chrono::time_point_cast<std::chrono::seconds>(rec.start),
                                                 std::chrono::time_point_cast<std::chrono::seconds>(*rec.end));
    REQUIRE(occurrences.size() == 2); // 2026 e 2027
    REQUIRE(occurrences[0]->getStart() == std::chrono::time_point_cast<std::chrono::seconds>(rec.start));
    REQUIRE(occurrences[1]->getStart() ==
            TimePoint(std::chrono::seconds(1767225600 + 365 * 86400)));
}

TEST_CASE("occurrenceToJson serializza in ISO-8601", "[mappers]") {
    events::Event event("X", TimePoint(std::chrono::seconds(1767225600)),
                        std::chrono::hours(1));
    auto json = server::occurrenceToJson(7, event);
    REQUIRE(json["event_id"].get<long long>() == 7);
    REQUIRE(json["title"].get<std::string>() == "X");
    REQUIRE(json["start"].get<std::string>() == "2026-01-01T00:00:00");
    REQUIRE(json["end"].get<std::string>() == "2026-01-01T01:00:00");
}
