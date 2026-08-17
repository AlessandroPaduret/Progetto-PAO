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

    auto event = ActivityFactory::createSimpleWeekly("Meeting", start, duration, start + 4_weeks);

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
    auto event = ActivityFactory::createSimpleWeekly(
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
    auto birthday = ActivityFactory::createBirthday("Mario Rossi", 2026y/2/28);

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

    SECTION("truncateBefore esclude le occorrenze successive") {
        event.truncateBefore(start + std::chrono::weeks(2));
        auto instances = event.getSchedulable(start, start + 4_weeks);
        REQUIRE(instances.size() == 2);
        REQUIRE(instances[0]->getStart() == start);
        REQUIRE(instances[1]->getStart() == start + std::chrono::weeks(1));
    }
}

TEST_CASE("Event valida la durata", "[event][validation]") {
    TimePoint start = make_date(2026, 1, 1);

    SECTION("Il costruttore rifiuta durate negative") {
        REQUIRE_THROWS_AS(Event("Sbagliato", start, -1h), std::invalid_argument);
    }

    SECTION("setDuration rifiuta durate negative") {
        Event e("Ok", start, 1h);
        REQUIRE_THROWS_AS(e.setDuration(-1s), std::invalid_argument);
        REQUIRE(e.getDuration() == 1h);
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

    SECTION("Deadline: occorrenza puntuale a durata zero") {
        Deadline d("Consegna", start + 2_weeks, Priority::High);
        auto occ = d.occurrencesIn(start, to);
        REQUIRE(occ.size() == 1);
        REQUIRE(occ[0].start == start + 2_weeks);
        REQUIRE(occ[0].duration == Duration::zero());
        REQUIRE(d.occurrencesIn(start, start + 1_weeks).empty());
    }

    SECTION("Reminder una tantum e ripetuto") {
        Reminder once("Bevi", start + 1h, "Acqua");
        REQUIRE(once.occurrencesIn(start, to).size() == 1);

        Reminder daily("Bevi", start, "Acqua", Days(1));
        REQUIRE(daily.isRepeating());
        // [start, start+4 settimane] inclusivo -> 29 attivazioni giornaliere
        REQUIRE(daily.occurrencesIn(start, to).size() == 29);
    }
}

TEST_CASE("Deadline: priorita', completamento e ritardo", "[deadline]") {
    TimePoint due = make_date(2026, 3, 10);
    Deadline d("Consegna progetto", due, Priority::High);

    REQUIRE(d.getPriority() == Priority::High);
    REQUIRE(Deadline::priorityLabel(Priority::Low) == "bassa");
    REQUIRE_FALSE(d.isDone());

    SECTION("Non scaduta prima del termine") {
        REQUIRE_FALSE(d.isOverdue(due - Days(1)));
        REQUIRE(d.timeRemaining(due - Days(1)) == Duration(Days(1)));
    }

    SECTION("Scaduta dopo il termine se non evasa") {
        REQUIRE(d.isOverdue(due + Days(1)));
        REQUIRE(d.timeRemaining(due + Days(1)) == Duration(-Days(1)));
    }

    SECTION("Evasa non risulta mai scaduta") {
        d.setDone();
        REQUIRE(d.isDone());
        REQUIRE_FALSE(d.isOverdue(due + Days(1)));
    }
}

TEST_CASE("Reminder: messaggio, ripetizione e snooze", "[reminder]") {
    TimePoint trigger = make_date(2026, 2, 1);
    Reminder r("Pillola", trigger, "Prendi la pillola", Days(1));

    REQUIRE(r.getMessage() == "Prendi la pillola");
    REQUIRE(r.isRepeating());

    SECTION("snooze posticipa l'attivazione") {
        r.snooze(30min);
        REQUIRE(r.getTrigger() == trigger + 30min);
    }

    SECTION("Ripetizione negativa rifiutata") {
        REQUIRE_THROWS_AS(r.setRepeatInterval(-1h), std::invalid_argument);
        REQUIRE_THROWS_AS(Reminder("X", trigger, "", -1h), std::invalid_argument);
    }
}

TEST_CASE("Calendar raccoglie attivita' eterogenee", "[calendar]") {
    TimePoint start = make_date(2026, 1, 1);
    Calendar calendar;

    calendar.add(ActivityFactory::createSimpleEvent("A evento", start, 1h));
    calendar.add(ActivityFactory::createSimpleWeekly("B riunione", start + 1_weeks, 1h, start + 2_weeks));
    calendar.add(ActivityFactory::createDeadline("C scadenza", start + Days(3), Priority::Medium));
    calendar.add(ActivityFactory::createReminder("D promemoria", start + Days(2), "msg"));

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
        REQUIRE(calendar.search("scadenza").size() == 1);
        REQUIRE(calendar.search("SCADENZA").size() == 1);
        REQUIRE(calendar.search("promemoria")[0]->getTitle() == "D promemoria");
        REQUIRE(calendar.search("inesistente").empty());
    }

    SECTION("remove elimina per identita'") {
        const Activity* target = calendar.search("promemoria")[0];
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
    activities.push_back(ActivityFactory::createDeadline("Scadenza", start + 1_weeks, Priority::Low));
    activities.push_back(ActivityFactory::createReminder("Quotidiano", start, "", Days(1)));

    SECTION("occurrencesIn si comporta in modo diverso per tipo dinamico") {
        std::vector<size_t> counts;
        for (const auto& a : activities) {
            counts.push_back(a->occurrencesIn(start, to).size());
        }
        // Event: 1; RecurrentEvent: 3; Deadline: 1; Reminder giornaliero: 15
        REQUIRE(counts == std::vector<size_t>{1, 3, 1, 15});
    }

    SECTION("describe() dipende dal tipo dinamico") {
        REQUIRE(activities[0]->describe().find("Evento") != String::npos);
        REQUIRE(activities[1]->describe().find("ricorrente") != String::npos);
        REQUIRE(activities[2]->describe().find("Scadenza") != String::npos);
        REQUIRE(activities[3]->describe().find("Promemoria") != String::npos);
    }

    SECTION("clone() copia il tipo dinamico corretto") {
        auto copy = activities[2]->clone();  // Deadline via Activity*
        REQUIRE(copy->describe() == activities[2]->describe());
        copy->setTitle("Copia");
        REQUIRE(activities[2]->getTitle() == "Scadenza");
    }
}

namespace {

// Visitor di test: conta le visite per tipo dinamico (dimostra il doppio dispatch)
class CountingVisitor : public ActivityVisitor {
public:
    int events = 0;
    int recurrents = 0;
    int deadlines = 0;
    int reminders = 0;

    void visit(const Event&) override { ++events; }
    void visit(const RecurrentEvent&) override { ++recurrents; }
    void visit(const Deadline&) override { ++deadlines; }
    void visit(const Reminder&) override { ++reminders; }
};

} // namespace

TEST_CASE("Visitor: doppio dispatch sul tipo dinamico", "[visitor]") {
    TimePoint start = make_date(2026, 1, 1);

    Calendar calendar;
    calendar.add(ActivityFactory::createSimpleEvent("E", start, 1h));
    calendar.add(ActivityFactory::createSimpleWeekly("W", start, 1h, start + 1_weeks));
    calendar.add(ActivityFactory::createDeadline("D", start, Priority::High));
    calendar.add(ActivityFactory::createReminder("R", start, "msg"));
    calendar.add(ActivityFactory::createSimpleEvent("E2", start, 2h));

    CountingVisitor visitor;
    for (const auto& activity : calendar) {
        activity->accept(visitor);
    }

    REQUIRE(visitor.events == 2);
    REQUIRE(visitor.recurrents == 1);
    REQUIRE(visitor.deadlines == 1);
    REQUIRE(visitor.reminders == 1);
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

    SECTION("RecurrentEvent: inizio spostato, la fine NON slitta, serie ricreata intonsa") {
        auto event = ActivityFactory::createSimpleWeekly(
            "Riunione", start + 9h, 1h, start + 3_weeks);
        event->addException(start + 1_weeks + 9h);  // giorno "staccato"

        // Sposta la serie UNA SETTIMANA INDIETRO (end = start+3 settimane
        // supera ancora il nuovo inizio): la fine deve restare com'e'.
        const TimePoint earlier = start - 1_weeks + 9h;
        event->moveTo(earlier);
        REQUIRE(event->getStart() == earlier);

        // Le eccezioni non vengono traslate: la serie spostata e' intonsa
        REQUIRE(event->getExceptions().empty());

        // occorrenze continue (earlier, +1, +2, +3 settimane) fino alla
        // scadenza ORIGINALE (start+3 settimane): nessun buco
        REQUIRE(event->getSchedulable(earlier, start + 3_weeks).size() == 4);
        REQUIRE(event->getSchedulable(start + 3_weeks, start + 4_weeks + 9h).empty());
    }

    SECTION("RecurrentEvent: inizio OLTRE la scadenza -> la scadenza resta ma sale al nuovo inizio") {
        auto event = ActivityFactory::createSimpleWeekly(
            "Riunione", start + 9h, 1h, start + 2_weeks);
        const TimePoint later = start + 4_weeks + 9h;  // oltre la scadenza
        event->moveTo(later);
        REQUIRE(event->getStart() == later);
        // una sola occorrenza (quella appena creata dallo spostamento)
        REQUIRE(event->getSchedulable(later, later + 1_weeks).size() == 1);
    }

    SECTION("Deadline: cambia la scadenza") {
        Deadline deadline("Consegna", start, Priority::High);
        deadline.moveTo(target);
        REQUIRE(deadline.getDue() == target);
    }

    SECTION("Reminder: cambia l'attivazione") {
        Reminder reminder("Pillola", start, "msg");
        reminder.moveTo(target);
        REQUIRE(reminder.getTrigger() == target);
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
