#include <QDate>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTime>
#include <QTimeZone>

#include <catch2/catch_all.hpp>

#include "api/dto.h"

using namespace client;

namespace {
QDateTime utc(int year, int month, int day, int hour, int minute, int second) {
    return QDateTime(QDate(year, month, day),
                     QTime(hour, minute, second), QTimeZone::UTC);
}
} // namespace

TEST_CASE("eventType conversions", "[dto]") {
    REQUIRE(eventTypeToString(EventType::Single) == "single");
    REQUIRE(eventTypeToString(EventType::Fixed) == "fixed");
    REQUIRE(eventTypeToString(EventType::Yearly) == "yearly");

    EventType type = EventType::Single;
    REQUIRE(eventTypeFromString("fixed", type));
    REQUIRE(type == EventType::Fixed);
    REQUIRE_FALSE(eventTypeFromString("boh", type));
}

TEST_CASE("CreateEventRequest toJson", "[dto]") {
    CreateEventRequest request;
    request.title = "Palestra";
    request.start = utc(2026, 1, 6, 8, 0, 0);
    request.durationSec = 1800;
    request.type = EventType::Fixed;
    request.intervalSec = 604800;

    const QJsonObject obj = request.toJson();
    REQUIRE(obj.value("title").toString() == "Palestra");
    REQUIRE(obj.value("start").toString() == "2026-01-06T08:00:00");
    REQUIRE(obj.value("duration").toVariant().toLongLong() == 1800);
    REQUIRE(obj.value("type").toString() == "fixed");
    REQUIRE(obj.value("interval").toVariant().toLongLong() == 604800);
    REQUIRE_FALSE(obj.contains("end"));
}

TEST_CASE("CreateEventRequest singolo senza end/eccezioni", "[dto]") {
    CreateEventRequest request;
    request.title = "Riunione";
    request.start = utc(2026, 1, 5, 10, 0, 0);
    request.durationSec = 3600;

    const QJsonObject obj = request.toJson();
    REQUIRE(obj.value("type").toString() == "single");
    REQUIRE_FALSE(obj.contains("interval"));
    REQUIRE_FALSE(obj.contains("end"));
    REQUIRE_FALSE(obj.contains("exceptions"));
}

TEST_CASE("Occurrence fromJson", "[dto]") {
    QJsonObject obj;
    obj["event_id"] = 7;
    obj["title"] = "Riunione";
    obj["start"] = "2026-01-05T10:00:00";
    obj["end"] = "2026-01-05T11:00:00";
    obj["type"] = "fixed";

    const Occurrence occurrence = Occurrence::fromJson(obj);
    REQUIRE(occurrence.eventId == 7);
    REQUIRE(occurrence.title == "Riunione");
    REQUIRE(occurrence.start == utc(2026, 1, 5, 10, 0, 0));
    REQUIRE(occurrence.end == utc(2026, 1, 5, 11, 0, 0));
    REQUIRE(occurrence.type == "fixed");
}

TEST_CASE("toUtcIso formatta senza Z (come atteso dal server)", "[dto]") {
    REQUIRE(toUtcIso(utc(2026, 1, 5, 10, 0, 0)) == "2026-01-05T10:00:00");
}

TEST_CASE("parseUtcIso interpreta il wall-time come UTC", "[dto]") {
    REQUIRE(parseUtcIso("2026-01-05T10:00:00") == utc(2026, 1, 5, 10, 0, 0));
    REQUIRE_FALSE(parseUtcIso("non-una-data").isValid());
}
