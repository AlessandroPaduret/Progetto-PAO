#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <memory>

#include "events/builders/ActivityBuilder.h"
#include "events/core/Activity.h"
#include "events/core/Occurrence.h"
#include "events/events.h"

using namespace std::chrono_literals;
using namespace events;

TimePoint make_date(int y, int m, int d) {
    return std::chrono::sys_days{std::chrono::year{y}/std::chrono::month{static_cast<unsigned>(m)}/std::chrono::day{static_cast<unsigned>(d)}};
}

TEST_CASE("ActivityBuilder per un evento settimanale", "[ActivityBuilder][FixedIntervalGenerator][exception]") {
    TimePoint start = make_date(2026, 1, 1);
    auto duration = 1h;
    auto endRange = start + 3_weeks;

    Activity event =  ActivityBuilder("Riunione settimanale", start)
                        .withDuration(1h)
                        .addGenerator(std::make_shared<FixedIntervalGenerator>(start, 24h * 7, endRange, 0))
                        .build();

    SECTION("Generazione corretta") {
        auto instances = event.occurrencesIn(start, endRange);
        REQUIRE(instances.size() == 4);

        for (int i = 0; i < 4; ++i) {
            REQUIRE(instances[i].start == start + std::chrono::weeks(i));
            REQUIRE(instances[i].duration == duration);
        }
    }

    SECTION("Aggiunta di eccezioni"){
        TimePoint secondWeek = start + 1_weeks;
        event.addException(secondWeek);

        std::vector<Occurrence> expected;
        for (int i = 0; i < 4; ++i) {
            expected.push_back(Occurrence(nullptr, start + 24h * 7 * i, 1h));
        }

        auto instances = event.occurrencesIn(start, start + 3_weeks);
        REQUIRE(instances.size() == 3);
        REQUIRE(instances[0].start == start);
        REQUIRE(instances[1].start == start + std::chrono::weeks(2));
        REQUIRE(instances[2].start == start + std::chrono::weeks(3));

    }

    SECTION("truncateBefore esclude le occorrenze successive") {
        event.truncateBefore(start + std::chrono::weeks(2));
        auto instances = event.occurrencesIn(start, start + 4_weeks);
        REQUIRE(instances.size() == 1);
        REQUIRE(instances[0].start == start);
    }
}

TEST_CASE("Literal personalizzato per settimane", "[literals]") {
    TimePoint start = make_date(2026, 1, 1);
    auto end = start + 4_weeks;

    REQUIRE(end == start + std::chrono::weeks(4));
}


TEST_CASE("Event valida la durata", "[Activity][validation]") {
    TimePoint start = make_date(2026, 1, 1);

    SECTION("Il costruttore rifiuta durate negative") {
        REQUIRE_THROWS_AS(Activity("Sbagliato", start, -1h), std::invalid_argument);
    }

    SECTION("setDuration rifiuta durate negative") {
        Activity e("Ok", start, 1h);
        REQUIRE_THROWS_AS(e.setDuration(-1s), std::invalid_argument);
        REQUIRE(e.getDuration() == 1h);
    }
}

/*

TEST_CASE("Stato di completamento (solo su Task)", "[state]") {
    TimePoint start = make_date(2026, 1, 1);

    SECTION("Task: spunta globale + isOverdue") {
        Task t("Consegna", start, Priority::High);
        REQUIRE_FALSE(t.isDone());
        REQUIRE_FALSE(t.isOverdue(start - 1h));
        REQUIRE(t.isOverdue(start + 1h));
        t.setDone();
        REQUIRE(t.isDone());
        REQUIRE_FALSE(t.isOverdue(start + 1h));
        t.setDone(false);
        REQUIRE_FALSE(t.isDone());
    }
}

TEST_CASE("Occorrenze dei singoli tipi di attivita'", "[occurrences]") {
    TimePoint start = make_date(2026, 1, 1);
    TimePoint to = start + 4_weeks;

    SECTION("Event: una sola occorrenza nel range") {
        Event e("Dentista", start + 1_weeks, 1h);
        auto occ = e.occurrencesIn(start, to);
        REQUIRE(occ.size() == 1);
        REQUIRE(occ[0].start == start + 1_weeks);
        REQUIRE(occ[0].duration == 1h);
        REQUIRE(occ[0].source == &e);
        REQUIRE(e.occurrencesIn(start + 2_weeks, to).empty());
    }

    SECTION("RecurrentEvent: espande la ricorrenza meno le eccezioni") {
        auto event = ActivityFactory::createSimpleWeekly("Meeting", start, 1h, start + 4_weeks);
        event->addException(start + 1_weeks);
        auto occ = event->occurrencesIn(start, to);
        REQUIRE(occ.size() == 4);
        REQUIRE(occ[0].duration == 1h);
    }

    SECTION("Meeting: occorrenza singola con durata") {
        Meeting m("Riunione", start + 2_weeks, 90min, "Aula 3");
        auto occ = m.occurrencesIn(start, to);
        REQUIRE(occ.size() == 1);
        REQUIRE(occ[0].duration == 90min);
        REQUIRE(m.attendeeCount() == 0);
    }

    SECTION("Task: occorrenza puntuale a durata zero") {
        Task t("Consegna", start + 2_weeks, Priority::High);
        auto occ = t.occurrencesIn(start, to);
        REQUIRE(occ.size() == 1);
        REQUIRE(occ[0].start == start + 2_weeks);
        REQUIRE(occ[0].duration == Duration::zero());
        REQUIRE(t.occurrencesIn(start, start + 1_weeks).empty());
    }

    SECTION("Evento di 24h a mezzanotte: copre l'intero giorno") {
        // L'evento "tutto il giorno" e' un Event dalle 00:00 con durata 24h
        Event allday("Mostra", make_date(2026, 1, 10), std::chrono::seconds(86400));
        auto occ = allday.occurrencesIn(make_date(2026, 1, 10), make_date(2026, 1, 12));
        REQUIRE(occ.size() == 1);
        REQUIRE(occ[0].start == make_date(2026, 1, 10));
        REQUIRE(occ[0].end() == make_date(2026, 1, 11));  // 00:00 del giorno dopo
        REQUIRE(occ[0].duration == Duration(86400));
    }

    SECTION("Anniversary: ricorrenze annuali leap-aware") {
        Anniversary ann("Compleanno", make_date(2028, 2, 29));
        auto occ = ann.occurrencesIn(make_date(2028, 1, 1), make_date(2031, 12, 31));
        // 2028->29/2, 2029->28/2, 2030->28/2, 2031->28/2
        REQUIRE(occ.size() == 4);
        REQUIRE(occ[0].start == make_date(2028, 2, 29));
        REQUIRE(occ[1].start == make_date(2029, 2, 28));
        REQUIRE(occ[3].start == make_date(2031, 2, 28));
        REQUIRE(occ[0].duration == Duration(86399));
    }
}

TEST_CASE("Meeting: partecipanti e luogo", "[meeting]") {
    Meeting m("Riunione", make_date(2026, 3, 1) + 10h, 1h, "Aula Magna");

    SECTION("addAttendee/removeAttendee/attendeeCount") {
        REQUIRE(m.addAttendee("Mario"));
        REQUIRE(m.addAttendee("Anna"));
        REQUIRE(m.attendeeCount() == 2);
        // Duplicati rifiutati
        REQUIRE_FALSE(m.addAttendee("Mario"));
        REQUIRE(m.removeAttendee("Mario"));
        REQUIRE(m.attendeeCount() == 1);
        REQUIRE_FALSE(m.removeAttendee("Inesistente"));
    }

    SECTION("location leggibile") {
        REQUIRE(m.getLocation() == "Aula Magna");
        m.setLocation("Zoom");
        REQUIRE(m.getLocation() == "Zoom");
    }

    SECTION("durata negativa rifiutata") {
        REQUIRE_THROWS_AS(m.setDuration(-1h), std::invalid_argument);
    }
}

TEST_CASE("Task: priorita' e scadenza", "[task]") {
    TimePoint due = make_date(2026, 3, 10);
    Task t("Consegna progetto", due, Priority::High);

    REQUIRE(t.getPriority() == Priority::High);
    REQUIRE(Task::priorityLabel(Priority::Low) == "bassa");

    SECTION("Non scaduto prima del termine") {
        REQUIRE_FALSE(t.isOverdue(due - Days(1)));
        REQUIRE(t.timeRemaining(due - Days(1)) == Duration(Days(1)));
    }

    SECTION("Scaduto dopo il termine se non evaso") {
        REQUIRE(t.isOverdue(due + Days(1)));
        REQUIRE(t.timeRemaining(due + Days(1)) == Duration(-Days(1)));
    }
}

TEST_CASE("Calendar raccoglie attivita' eterogenee", "[calendar]") {
    TimePoint start = make_date(2026, 1, 1);
    Calendar calendar;

    calendar.add(ActivityFactory::createSimpleEvent("A evento", start, 1h));
    calendar.add(ActivityFactory::createSimpleWeekly("B riunione", start + 1_weeks, 1h, start + 2_weeks));
    calendar.add(ActivityFactory::createTask("C compito", start + Days(3), Priority::Medium));
    calendar.add(ActivityFactory::createMeeting("D riunione", start + Days(2), 1h));

    REQUIRE(calendar.size() == 4);

    SECTION("occurrencesIn aggrega tutti i tipi, ordinati per inizio") {
        auto occ = calendar.occurrencesIn(start, start + 2_weeks);
        // A (1) + D (1) + C (1) + B (2 occorrenze) = 5
        REQUIRE(occ.size() == 5);
        for (size_t i = 1; i < occ.size(); ++i) {
            REQUIRE(occ[i-1].start <= occ[i].start);
        }
    }

    SECTION("search filtra per titolo, case-insensitive") {
        REQUIRE(calendar.search("").size() == 4);
        REQUIRE(calendar.search("compito").size() == 1);
        REQUIRE(calendar.search("COMPITO").size() == 1);
        auto riunioni = calendar.search("riunione");
        REQUIRE(riunioni.size() == 2);  // "B riunione" (serie) + "D riunione"
        REQUIRE(calendar.search("inesistente").empty());
    }

    SECTION("remove elimina per identita'") {
        const Activity* target = calendar.search("compito")[0];
        REQUIRE(calendar.remove(target));
        REQUIRE(calendar.size() == 3);
        REQUIRE_FALSE(calendar.remove(target));
    }
}

TEST_CASE("Polimorfismo non banale sulla gerarchia Activity", "[polymorphism]") {
    TimePoint start = make_date(2026, 1, 1);
    TimePoint to = start + 2_weeks;

    std::vector<std::unique_ptr<Activity>> activities;
    activities.push_back(ActivityFactory::createSimpleEvent("Evento", start, 1h));
    activities.push_back(ActivityFactory::createSimpleWeekly("Settimanale", start, 1h, start + 2_weeks));
    activities.push_back(ActivityFactory::createTask("Compito", start + 1_weeks, Priority::Low));
    activities.push_back(ActivityFactory::createMeeting("Riunione", start, 1h));
    activities.push_back(ActivityFactory::createAnniversary("Anniversario", make_date(2000, 1, 1)));

    SECTION("occurrencesIn si comporta in modo diverso per tipo dinamico") {
        std::vector<size_t> counts;
        for (const auto& a : activities) {
            counts.push_back(a->occurrencesIn(start, to).size());
        }
        // Event:1; Weekly:3; Task:1; Meeting:1; Anniversary:1
        REQUIRE(counts == std::vector<size_t>{1, 3, 1, 1, 1});
    }

    SECTION("describe() dipende dal tipo dinamico") {
        REQUIRE(activities[0]->describe().find("Evento") != String::npos);
        REQUIRE(activities[1]->describe().find("ricorrente") != String::npos);
        REQUIRE(activities[2]->describe().find("Compito") != String::npos);
        REQUIRE(activities[3]->describe().find("Riunione") != String::npos);
        REQUIRE(activities[4]->describe().find("Anniversario") != String::npos);
    }

    SECTION("clone() copia il tipo dinamico corretto (stato incluso)") {
        auto* task = dynamic_cast<Task*>(activities[2].get());
        REQUIRE(task != nullptr);
        task->setDone();  // Task evaso
        auto copy = activities[2]->clone();
        REQUIRE(copy->describe() == activities[2]->describe());
        auto* copyTask = dynamic_cast<Task*>(copy.get());
        REQUIRE(copyTask != nullptr);
        REQUIRE(copyTask->isDone());
        copy->setTitle("Copia");
        REQUIRE(activities[2]->getTitle() == "Compito");
    }
}

namespace {

// Visitor di test: conta le visite per tipo dinamico (dimostra il doppio dispatch)
class CountingVisitor : public ActivityVisitor {
public:
    int events = 0;
    int recurrents = 0;
    int tasks = 0;
    int meetings = 0;
    int anniversaries = 0;

    void visit(const Event&) override { ++events; }
    void visit(const RecurrentEvent&) override { ++recurrents; }
    void visit(const Task&) override { ++tasks; }
    void visit(const Meeting&) override { ++meetings; }
    void visit(const Anniversary&) override { ++anniversaries; }
};

} // namespace

TEST_CASE("Visitor: doppio dispatch sul tipo dinamico", "[visitor]") {
    TimePoint start = make_date(2026, 1, 1);

    Calendar calendar;
    calendar.add(ActivityFactory::createSimpleEvent("E", start, 1h));
    calendar.add(ActivityFactory::createSimpleWeekly("W", start, 1h, start + 1_weeks));
    calendar.add(ActivityFactory::createTask("T", start, Priority::High));
    calendar.add(ActivityFactory::createMeeting("M", start, 1h));
    calendar.add(ActivityFactory::createAnniversary("N", make_date(2000, 1, 1)));

    CountingVisitor visitor;
    for (const auto& activity : calendar) {
        activity->accept(visitor);
    }

    REQUIRE(visitor.events == 1);
    REQUIRE(visitor.recurrents == 1);
    REQUIRE(visitor.tasks == 1);
    REQUIRE(visitor.meetings == 1);
    REQUIRE(visitor.anniversaries == 1);
}

TEST_CASE("MonthlyGenerator: passi di calendario esatti", "[monthly]") {
    SECTION("ogni mese, stesso giorno") {
        MonthlyGenerator gen(make_date(2026, 1, 15));
        auto dates = gen.generateDates(make_date(2026, 1, 1), make_date(2026, 5, 1));
        REQUIRE(dates.size() == 4);
        REQUIRE(dates[0] == make_date(2026, 1, 15));
        REQUIRE(dates[1] == make_date(2026, 2, 15));
        REQUIRE(dates[3] == make_date(2026, 4, 15));
    }

    SECTION("ogni 2 mesi, stesso giorno") {
        MonthlyGenerator gen(make_date(2026, 1, 10), 2);
        auto dates = gen.generateDates(make_date(2026, 1, 1), make_date(2026, 12, 31));
        REQUIRE(dates.size() == 6);
        REQUIRE(dates[0] == make_date(2026, 1, 10));
        REQUIRE(dates[1] == make_date(2026, 3, 10));
        REQUIRE(dates[2] == make_date(2026, 5, 10));
    }

    SECTION("clamping del giorno: 31 -> ultimo giorno del mese") {
        MonthlyGenerator gen(make_date(2026, 1, 31));
        auto dates = gen.generateDates(make_date(2026, 1, 1), make_date(2026, 6, 1));
        REQUIRE(dates.size() == 5);
        REQUIRE(dates[0] == make_date(2026, 1, 31));
        REQUIRE(dates[1] == make_date(2026, 2, 28));   // febbraio non bisestile
        REQUIRE(dates[2] == make_date(2026, 3, 31));
        REQUIRE(dates[3] == make_date(2026, 4, 30));
    }

    SECTION("clamping in anno bisestile: 29/2 esiste") {
        MonthlyGenerator gen(make_date(2028, 1, 31));
        auto dates = gen.generateDates(make_date(2028, 1, 1), make_date(2028, 5, 1));
        REQUIRE(dates[1] == make_date(2028, 2, 29));
    }

    SECTION("fine e limite occorrenze") {
        MonthlyGenerator gen(make_date(2026, 1, 1));
        gen.setMaxOccurrences(3);
        auto dates = gen.generateDates(make_date(2026, 1, 1), make_date(2027, 1, 1));
        REQUIRE(dates.size() == 3);
        REQUIRE(dates[0] == make_date(2026, 1, 1));
        REQUIRE(dates[2] == make_date(2026, 3, 1));
    }

    SECTION("la fine limita le date") {
        MonthlyGenerator gen(make_date(2026, 1, 1), 1, make_date(2026, 6, 30));
        auto dates = gen.generateDates(make_date(2026, 1, 1), make_date(2027, 1, 1));
        REQUIRE(dates.size() == 6);
        REQUIRE(dates.back() == make_date(2026, 6, 1));
    }
}

TEST_CASE("Limite di occorrenze nei generatori", "[cap]") {
    SECTION("FixedIntervalGenerator: dopo N occorrenze") {
        FixedIntervalGenerator gen(make_date(2026, 1, 1), Days(7));
        gen.setMaxOccurrences(4);
        auto dates = gen.generateDates(make_date(2026, 1, 1), make_date(2026, 3, 1));
        REQUIRE(dates.size() == 4);
        REQUIRE(dates[0] == make_date(2026, 1, 1));
        REQUIRE(dates[3] == make_date(2026, 1, 22));
    }

    SECTION("FixedIntervalGenerator: la finestra tarda conta dal primo inizio") {
        FixedIntervalGenerator gen(make_date(2026, 1, 1), Days(7));
        gen.setMaxOccurrences(4);
        // interrogo dal 3/2: la 6a occorrenza (1/2) e' gia' passata ma conta
        auto dates = gen.generateDates(make_date(2026, 2, 8), make_date(2026, 3, 1));
        REQUIRE(dates.empty());  // 1/1 + 4 occorrenze -> 22/1, nessuna dopo l'8/2
    }

    SECTION("YearlyGenerator: dopo N occorrenze") {
        YearlyGenerator gen(make_date(2028, 2, 29));
        gen.setMaxOccurrences(3);
        auto dates = gen.generateDates(make_date(2028, 1, 1), make_date(2032, 1, 1));
        REQUIRE(dates.size() == 3);
        REQUIRE(dates[0] == make_date(2028, 2, 29));
        REQUIRE(dates[1] == make_date(2029, 2, 28));
        REQUIRE(dates[2] == make_date(2030, 2, 28));
    }
}

TEST_CASE("Formattazione ISO-8601", "[iso][format]") {
    SECTION("formatta in UTC con T e secondi") {
        REQUIRE(formatIso8601(make_date(2026, 1, 1)) == "2026-01-01T00:00:00");
        REQUIRE(formatIso8601(make_date(2026, 3, 10) + 9h + 30min + 15s) == "2026-03-10T09:30:15");
    }

    SECTION("round-trip esatto") {
        TimePoint tp = make_date(2026, 12, 31) + 23h + 59min + 59s;
        TimePoint out;
        REQUIRE(parseIso8601(formatIso8601(tp), out));
        REQUIRE(out == tp);
    }

    SECTION("rifiuta input non validi") {
        TimePoint out;
        REQUIRE_FALSE(parseIso8601("2026-01-01", out));           // manca l'ora
        REQUIRE_FALSE(parseIso8601("2026-01-01T00:00", out));     // mancano i secondi
        REQUIRE_FALSE(parseIso8601("garbage", out));
        REQUIRE_FALSE(parseIso8601("2026-13-01T00:00:00", out));  // mese invalido
        REQUIRE_FALSE(parseIso8601("2026-02-30T00:00:00", out));  // giorno invalido
        REQUIRE_FALSE(parseIso8601("2026-01-01T24:00:00", out));  // ora invalida
        REQUIRE_FALSE(parseIso8601("2026-01-01T00:00:00X", out)); // coda non consumata
    }
}

TEST_CASE("moveTo sposta l'attivita' al nuovo istante", "[move]") {
    TimePoint start = make_date(2026, 1, 5);
    TimePoint target = make_date(2026, 2, 9) + 3h;

    SECTION("Event: cambia l'inizio, la durata resta") {
        Event event("X", start, 1h);
        event.moveTo(target);
        REQUIRE(event.getStart() == target);
        REQUIRE(event.getDuration() == 1h);
    }

    SECTION("RecurrentEvent: inizio spostato, la fine NON slitta") {
        auto event = ActivityFactory::createSimpleWeekly(
            "Riunione", start + 9h, 1h, start + 3_weeks);
        const TimePoint earlier = start - 1_weeks + 9h;
        event->moveTo(earlier);
        REQUIRE(event->getStart() == earlier);
        REQUIRE(event->getSchedulable(earlier, start + 3_weeks).size() == 4);
    }

    SECTION("Task: cambia la scadenza") {
        Task task("Consegna", start, Priority::High);
        task.moveTo(target);
        REQUIRE(task.getDue() == target);
    }

    SECTION("Meeting: cambia l'inizio, durata e luogo restano") {
        Meeting meeting("Riunione", start, 1h, "Aula");
        meeting.addAttendee("Mario");
        meeting.moveTo(target);
        REQUIRE(meeting.getStart() == target);
        REQUIRE(meeting.getDuration() == 1h);
        REQUIRE(meeting.getLocation() == "Aula");
        REQUIRE(meeting.attendeeCount() == 1);
    }

    SECTION("Event di 24h: moveTo sposta l'inizio, la durata resta") {
        Event allday("Giornata", start, std::chrono::seconds(86400));
        allday.moveTo(target);
        REQUIRE(allday.getStart() == target);
        REQUIRE(allday.getDuration() == std::chrono::seconds(86400));
        REQUIRE(allday.getEnd() == target + std::chrono::seconds(86400));
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

*/