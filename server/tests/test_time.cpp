#include <chrono>
#include <string>

#include <catch2/catch_all.hpp>

#include "iso8601.h"

using events::TimePoint;

TEST_CASE("toIso8601 formatta correttamente", "[time]") {
    TimePoint tp(std::chrono::seconds(1767225600)); // 2026-01-01T00:00:00 UTC
    REQUIRE(server::toIso8601(tp) == "2026-01-01T00:00:00");
}

TEST_CASE("parseIso8601 e round-trip", "[time]") {
    TimePoint tp(std::chrono::seconds(1767225600));
    TimePoint parsed;
    REQUIRE(server::parseIso8601("2026-01-01T00:00:00", parsed));
    REQUIRE(parsed == tp);

    TimePoint back;
    REQUIRE(server::parseIso8601(server::toIso8601(tp), back));
    REQUIRE(back == tp);
}

TEST_CASE("parseIso8601 rifiuta input non validi", "[time]") {
    TimePoint tp;
    REQUIRE_FALSE(server::parseIso8601("", tp));
    REQUIRE_FALSE(server::parseIso8601("ciao", tp));
    REQUIRE_FALSE(server::parseIso8601("2026-01-01", tp)); // manca l'ora
}
