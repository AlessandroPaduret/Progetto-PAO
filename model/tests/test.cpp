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

TEST_CASE("Eccezioni su RecurrentEvent costruito direttamente", "[weekly][exception]") {
    TimePoint start = make_date(2026, 1, 1);
    auto gen = std::make_shared<FixedIntervalGenerator>(start, std::chrono::weeks(1), start + 4_weeks);
    RecurrentEvent event(gen, Event("Meeting", start, 1h));

    SECTION("Aggiunta eccezione salta solo quella occorrenza") {
        event.addException(start + std::chrono::weeks(1));

        auto instances = event.getSchedulable(start, start + 4_weeks);
        REQUIRE(instances.size() == 4);
        REQUIRE(instances[0]->getStart() == start);
        REQUIRE(instances[1]->getStart() == start + std::chrono::weeks(2));
        REQUIRE(instances[2]->getStart() == start + std::chrono::weeks(3));
        REQUIRE(instances[3]->getStart() == start + std::chrono::weeks(4));
    }

    SECTION("Eliminazione eccezione la ripristina") {
        TimePoint secondWeek = start + std::chrono::weeks(1);
        event.addException(secondWeek);
        event.deleteExceptions(secondWeek);

        auto instances = event.getSchedulable(start, start + 4_weeks);
        REQUIRE(instances.size() == 5);
    }
}

TEST_CASE("Events raccoglie eventi non ricorrenti", "[events]") {
    TimePoint start = make_date(2026, 1, 1);
    Events events;
    events.addEvent(Event("A", start, 1h));
    events.addEvent(Event("B", start + std::chrono::weeks(2), 1h));
    events.addEvent(Event("C", start + std::chrono::weeks(1), 1h));
    events.addEvent(Event("D", start + std::chrono::weeks(8), 1h));

    REQUIRE(events.size() == 4);

    SECTION("getSchedulable filtra per inizio nel range inclusivo") {
        auto in = events.getSchedulable(start, start + 3_weeks);
        REQUIRE(in.size() == 3);
    }

    SECTION("I risultati sono ordinati per data di inizio") {
        auto in = events.getSchedulable(start, start + 3_weeks);
        REQUIRE(in[0]->getTitle() == "A");
        REQUIRE(in[1]->getTitle() == "C");
        REQUIRE(in[2]->getTitle() == "B");
    }

    SECTION("Eventi fuori range non vengono restituiti") {
        auto in = events.getSchedulable(start + std::chrono::weeks(1), start + std::chrono::weeks(2));
        REQUIRE(in.size() == 2);
        REQUIRE(in[0]->getTitle() == "C");
        REQUIRE(in[1]->getTitle() == "B");
    }

    SECTION("Le copie restituite sono indipendenti dall'originale") {
        auto in = events.getSchedulable(start, start + 3_weeks);
        in[0]->setTitle("Modificato");
        REQUIRE(events.getSchedulable(start, start)[0]->getTitle() == "A");
    }
}

int main(int argc, char* argv[]) {
    Catch::Session session;
    int returnCode = session.applyCommandLine(argc, argv);
    if (returnCode != 0) {
        return returnCode;
    }
    return session.run();
}