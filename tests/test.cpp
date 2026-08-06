#include <catch2/catch_all.hpp>
#include <chrono>

#include "events/events.h"

using namespace std::chrono_literals;
using namespace events;

TimePoint make_date(int y, int m, int d) {
    return std::chrono::sys_days{std::chrono::year{y}/std::chrono::month{static_cast<unsigned>(m)}/std::chrono::day{static_cast<unsigned>(d)}};
}

TEST_CASE("Factory crea eventi settimanali", "[factory][weekly]") {
    TimePoint start = make_date(2026, 1, 1);
    auto duration = 1h;
    auto endRange = start + 3_weeks;
    
    auto event = EventFactory::createSimpleWeekly("Meeting", start, duration, start + 4_weeks);
    
    SECTION("Generazione corretta") {
        auto instances = event->getSchedulable(start, endRange);
        REQUIRE(instances.size() == 4);
        
        for (int i = 0; i < 4; ++i) {
            REQUIRE(instances[i]->getStart() == start + std::chrono::weeks(i));
            REQUIRE(instances[i]->getDuration() == duration);
        }
    }
}

TEST_CASE("Eccezioni su evento settimanale", "[weekly][exception]") {
    TimePoint start = make_date(2026, 1, 1);
    auto event = EventFactory::createSimpleWeekly(
        "Meeting", start, 1h, start + std::chrono::weeks(4)
    );
    
    TimePoint secondWeek = start + std::chrono::weeks(1);
    event->addException(secondWeek);
    
    auto instances = event->getSchedulable(start, start + std::chrono::weeks(3));
    REQUIRE(instances.size() == 3);
    REQUIRE(instances[0]->getStart() == start);
    REQUIRE(instances[1]->getStart() == start + std::chrono::weeks(2));
    REQUIRE(instances[2]->getStart() == start + std::chrono::weeks(3));
}

TEST_CASE("Literal personalizzato per settimane", "[literals]") {
    TimePoint start = make_date(2026, 1, 1);
    auto end = start + 4_weeks;
    
    REQUIRE(end == start + std::chrono::weeks(4));
}

TEST_CASE("Creazione compleanno", "[birthday]") {
    TimePoint start = make_date(2026, 1, 1);
    auto birthday = EventFactory::createBirthday("Mario Rossi", 2026y/2/28);
    
    auto instances = birthday->getSchedulable(start, start + std::chrono::years(5));
    REQUIRE(instances.size() == 5);
}

TEST_CASE("MaxOccurrencesDecorator limita il numero di occorrenze", "[decorator][maxOccurrences]") {
    TimePoint start = make_date(2026, 1, 1);
    auto base = std::make_shared<FixedIntervalGenerator>(start, std::chrono::weeks(1));
    auto limited = std::make_shared<MaxOccurrencesDecorator>(base, 4);

    SECTION("Genera solo le prime 4 occorrenze") {
        auto dates = limited->generateDates(start, start + 30_weeks);
        REQUIRE(dates.size() == 4);
        for (int i = 0; i < 4; ++i) {
            REQUIRE(dates[i] == start + std::chrono::weeks(i));
        }
        REQUIRE(limited->getGeneratedCount() == 4);
    }

    SECTION("Dopo il limite non genera più nulla") {
        auto dates = limited->generateDates(start, start + 30_weeks);
        REQUIRE(dates.size() == 4);

        auto after = limited->generateDates(start + 30_weeks, start + 60_weeks);
        REQUIRE(after.empty());
    }

    SECTION("Tronca se l'intervallo contiene più date del limite residuo") {
        auto dates = limited->generateDates(start, start + 2_weeks);
        REQUIRE(dates.size() == 3);

        auto after = limited->generateDates(start + 3_weeks, start + 30_weeks);
        REQUIRE(after.size() == 1);
        REQUIRE(after[0] == start + std::chrono::weeks(3));
    }

    SECTION("occursInRange rispetta il limite") {
        REQUIRE(limited->occursInRange(start, start + 30_weeks));
        REQUIRE_FALSE(limited->occursInRange(start + 10_weeks, start + 30_weeks));
    }

    SECTION("describe contiene il limite") {
        REQUIRE(limited->describe().find("max 4") != std::string::npos);
    }
}

TEST_CASE("MaxOccurrencesDecorator con anno nuovo", "[decorator][maxOccurrences]") {
    TimePoint start = make_date(2026, 1, 1);
    auto base = std::make_shared<YearlyGenerator>(start);
    auto limited = std::make_shared<MaxOccurrencesDecorator>(base, 2);

    auto dates = limited->generateDates(start, start + std::chrono::years(10));
    REQUIRE(dates.size() == 2);
    REQUIRE(dates[0] == make_date(2026, 1, 1));
    REQUIRE(dates[1] == make_date(2027, 1, 1));
}

int main(int argc, char* argv[]) {
    Catch::Session session;
    int returnCode = session.applyCommandLine(argc, argv);
    if (returnCode != 0) {
        return returnCode;
    }
    return session.run();
}