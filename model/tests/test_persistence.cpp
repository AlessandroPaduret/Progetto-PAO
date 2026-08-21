#include <catch2/catch_all.hpp>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include <chrono>
#include <memory>

#include "events/events.h"
#include "events/generators/MonthlyGenerator.h"
#include "persistence/JsonPersistence.h"

using namespace std::chrono_literals;
using namespace events;

TimePoint make_date(int y, int m, int d) {
    return std::chrono::sys_days{std::chrono::year{y}/std::chrono::month{static_cast<unsigned>(m)}/std::chrono::day{static_cast<unsigned>(d)}};
}

TEST_CASE("Persistenza: Event round-trip (con stato)", "[json][event]") {
    TimePoint start = make_date(2026, 1, 8) + 10h;
    auto event = ActivityFactory::createSimpleEvent("Dentista", start, 1h);
    event->setDone();

    QJsonObject json = persistence::activityToJson(*event);
    REQUIRE(json.value("type").toString() == "event");
    REQUIRE(json.value("start").toString() == "2026-01-08T10:00:00");
    REQUIRE(json.value("duration_seconds").toInteger() == 3600);
    REQUIRE(json.value("done").toBool() == true);

    QString error;
    auto back = persistence::activityFromJson(json, &error);
    REQUIRE(back != nullptr);
    auto e = dynamic_cast<Event*>(back.get());
    REQUIRE(e != nullptr);
    REQUIRE(e->getTitle() == "Dentista");
    REQUIRE(e->getStart() == start);
    REQUIRE(e->getDuration() == 1h);
    REQUIRE(e->isDone());
}

TEST_CASE("Persistenza: RecurrentEvent (fixed) round-trip (con occorrenze evase)", "[json][recurrent]") {
    TimePoint start = make_date(2026, 1, 1) + 9h;
    auto event = ActivityFactory::createSimpleWeekly("Meeting", start, 1h, make_date(2026, 3, 1));
    event->addException(make_date(2026, 1, 15) + 9h);
    event->setDoneAt(make_date(2026, 1, 8) + 9h, true);  // seconda lezione evasa

    QJsonObject json = persistence::activityToJson(*event);
    REQUIRE(json.value("type").toString() == "recurrent");
    REQUIRE(json.value("generator").toObject().value("type").toString() == "fixed");
    REQUIRE(json.value("done_occurrences").toArray().size() == 1);

    auto back = persistence::activityFromJson(json);
    REQUIRE(back != nullptr);
    auto rec = dynamic_cast<RecurrentEvent*>(back.get());
    REQUIRE(rec != nullptr);
    REQUIRE(rec->getTitle() == "Meeting");
    REQUIRE(rec->getExceptions().size() == 1);
    REQUIRE(rec->getDoneOccurrences().size() == 1);
    REQUIRE(rec->isDoneAt(make_date(2026, 1, 8) + 9h));

    auto a = event->occurrencesIn(make_date(2026, 1, 1), make_date(2026, 2, 28));
    auto b = rec->occurrencesIn(make_date(2026, 1, 1), make_date(2026, 2, 28));
    REQUIRE(a.size() == b.size());
    REQUIRE(a.size() == 8);  // 9 lunedi' meno l'eccezione del 15/1
    for (size_t i = 0; i < a.size(); ++i) {
        REQUIRE(a[i].start == b[i].start);
        REQUIRE(a[i].duration == b[i].duration);
    }
}

TEST_CASE("Persistenza: Task round-trip", "[json][task]") {
    TimePoint due = make_date(2026, 3, 10);
    auto task = ActivityFactory::createTask("Consegna", due, Priority::High);
    task->setDone();

    QJsonObject json = persistence::activityToJson(*task);
    REQUIRE(json.value("type").toString() == "task");
    REQUIRE(json.value("priority").toString() == "high");
    REQUIRE(json.value("done").toBool() == true);

    auto back = persistence::activityFromJson(json);
    REQUIRE(back != nullptr);
    auto t = dynamic_cast<Task*>(back.get());
    REQUIRE(t != nullptr);
    REQUIRE(t->getTitle() == "Consegna");
    REQUIRE(t->getDue() == due);
    REQUIRE(t->getPriority() == Priority::High);
    REQUIRE(t->isDone());
}

TEST_CASE("Persistenza: Meeting round-trip (luogo + partecipanti)", "[json][meeting]") {
    TimePoint start = make_date(2026, 2, 1) + 10h;
    auto meeting = ActivityFactory::createMeeting("Riunione", start, 90min, "Aula Magna");
    meeting->addAttendee("Mario");
    meeting->addAttendee("Anna");

    QJsonObject json = persistence::activityToJson(*meeting);
    REQUIRE(json.value("type").toString() == "meeting");
    REQUIRE(json.value("location").toString() == "Aula Magna");
    REQUIRE(json.value("attendees").toArray().size() == 2);

    auto back = persistence::activityFromJson(json);
    REQUIRE(back != nullptr);
    auto m = dynamic_cast<Meeting*>(back.get());
    REQUIRE(m != nullptr);
    REQUIRE(m->getLocation() == "Aula Magna");
    REQUIRE(m->attendeeCount() == 2);
    REQUIRE(m->getAttendees()[0] == "Mario");
    REQUIRE(m->getAttendees()[1] == "Anna");
}

TEST_CASE("Persistenza: AllDayEvent round-trip", "[json][allday]") {
    auto allday = ActivityFactory::createAllDayEvent("Mostra", make_date(2026, 5, 10), make_date(2026, 5, 13));

    QJsonObject json = persistence::activityToJson(*allday);
    REQUIRE(json.value("type").toString() == "allday");
    REQUIRE(json.value("start").toString() == "2026-05-10T00:00:00");
    REQUIRE(json.value("end").toString() == "2026-05-13T00:00:00");

    auto back = persistence::activityFromJson(json);
    REQUIRE(back != nullptr);
    auto a = dynamic_cast<AllDayEvent*>(back.get());
    REQUIRE(a != nullptr);
    REQUIRE(a->getStart() == make_date(2026, 5, 10));
    REQUIRE(a->getEnd() == make_date(2026, 5, 13));
    REQUIRE(a->days() == 3);
}

TEST_CASE("Persistenza: Anniversary round-trip (leap-aware, occorrenze evase)", "[json][anniversary]") {
    auto anniversary = ActivityFactory::createAnniversary("Mario", make_date(2028, 2, 29));
    anniversary->setDoneAt(make_date(2028, 2, 29), true);
    anniversary->setDoneAt(make_date(2029, 2, 28), true);

    QJsonObject json = persistence::activityToJson(*anniversary);
    REQUIRE(json.value("type").toString() == "anniversary");
    REQUIRE(json.value("date").toString() == "2028-02-29T00:00:00");
    REQUIRE(json.value("done_occurrences").toArray().size() == 2);

    auto back = persistence::activityFromJson(json);
    REQUIRE(back != nullptr);
    auto ann = dynamic_cast<Anniversary*>(back.get());
    REQUIRE(ann != nullptr);
    REQUIRE(ann->getStart() == make_date(2028, 2, 29));
    REQUIRE(ann->getDoneOccurrences().size() == 2);

    auto a = anniversary->occurrencesIn(make_date(2028, 1, 1), make_date(2031, 12, 31));
    auto b = ann->occurrencesIn(make_date(2028, 1, 1), make_date(2031, 12, 31));
    REQUIRE(a.size() == b.size());
    REQUIRE(a.size() == 4);
    for (size_t i = 0; i < a.size(); ++i) {
        REQUIRE(a[i].start == b[i].start);
        REQUIRE(a[i].duration == b[i].duration);
    }
}

TEST_CASE("Persistenza: Calendar salva e ricarica da file", "[json][calendar][file]") {
    Calendar calendar;
    calendar.add(ActivityFactory::createSimpleEvent("Evento A", make_date(2026, 1, 1) + 9h, 1h));
    calendar.add(ActivityFactory::createSimpleWeekly("Riunione B", make_date(2026, 1, 2) + 10h, 30min, make_date(2026, 3, 1)));
    calendar.add(ActivityFactory::createTask("Compito C", make_date(2026, 2, 1), Priority::High));
    calendar.add(ActivityFactory::createMeeting("Riunione D", make_date(2026, 1, 3) + 8h, 1h, "Zoom"));
    calendar.add(ActivityFactory::createAllDayEvent("Giornata E", make_date(2026, 1, 4), make_date(2026, 1, 5)));
    calendar.add(ActivityFactory::createAnniversary("Anniversario F", make_date(2000, 1, 6)));

    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString path = dir.filePath("calendar.json");

    QString error;
    REQUIRE(persistence::saveToFile(calendar, path, &error));

    Calendar loaded;
    REQUIRE(persistence::loadFromFile(loaded, path, &error));
    REQUIRE(loaded.size() == 6);

    auto occ1 = calendar.occurrencesIn(make_date(2026, 1, 1), make_date(2026, 2, 28));
    auto occ2 = loaded.occurrencesIn(make_date(2026, 1, 1), make_date(2026, 2, 28));
    REQUIRE(occ1.size() == occ2.size());
    for (size_t i = 0; i < occ1.size(); ++i) {
        REQUIRE(occ1[i].start == occ2[i].start);
        REQUIRE(occ1[i].duration == occ2[i].duration);
        REQUIRE(occ1[i].source->getTitle() == occ2[i].source->getTitle());
    }

    REQUIRE(loaded.search("compito").size() == 1);
    REQUIRE(loaded.search("anniversario").size() == 1);
}

TEST_CASE("Persistenza: RecurrentEvent mensile (con limite occorrenze)", "[json][recurrent][monthly]") {
    TimePoint start = make_date(2026, 1, 10) + 9h;
    auto gen = std::make_shared<events::MonthlyGenerator>(start, 2);
    gen->setMaxOccurrences(5);
    RecurrentEvent event(gen, events::Event("Pagamento", start, 1h));

    QJsonObject json = persistence::activityToJson(event);
    REQUIRE(json.value("generator").toObject().value("type").toString() == "monthly");
    REQUIRE(json.value("generator").toObject().value("interval_months").toInteger() == 2);
    REQUIRE(json.value("generator").toObject().value("max_occurrences").toInteger() == 5);

    auto back = persistence::activityFromJson(json);
    REQUIRE(back != nullptr);
    auto rec = dynamic_cast<RecurrentEvent*>(back.get());
    REQUIRE(rec != nullptr);
    auto* monthly = dynamic_cast<events::MonthlyGenerator*>(rec->getGenerator().get());
    REQUIRE(monthly != nullptr);
    REQUIRE(monthly->getMonths() == 2);
    REQUIRE(monthly->getMaxOccurrences() == 5);

    auto a = event.occurrencesIn(make_date(2026, 1, 1), make_date(2027, 1, 1));
    auto b = rec->occurrencesIn(make_date(2026, 1, 1), make_date(2027, 1, 1));
    REQUIRE(a.size() == b.size());
    REQUIRE(a.size() == 5);  // 10/1, 10/3, 10/5, 10/7, 10/9
}

TEST_CASE("Persistenza: RecurrentEvent settimanale con limite occorrenze", "[json][recurrent][cap]") {
    TimePoint start = make_date(2026, 1, 5) + 9h;
    auto gen = std::make_shared<events::FixedIntervalGenerator>(start, events::Days(7));
    gen->setMaxOccurrences(3);
    RecurrentEvent event(gen, events::Event("Corso", start, 1h));

    QJsonObject json = persistence::activityToJson(event);
    REQUIRE(json.value("generator").toObject().value("max_occurrences").toInteger() == 3);

    auto back = persistence::activityFromJson(json);
    REQUIRE(back != nullptr);
    auto rec = dynamic_cast<RecurrentEvent*>(back.get());
    auto* fixed = dynamic_cast<events::FixedIntervalGenerator*>(rec->getGenerator().get());
    REQUIRE(fixed != nullptr);
    REQUIRE(fixed->getMaxOccurrences() == 3);
    REQUIRE(rec->occurrencesIn(make_date(2026, 1, 1), make_date(2026, 3, 1)).size() == 3);
}

TEST_CASE("Persistenza: serie ricorrente 'tutto il giorno'", "[json][recurrent][allday]") {
    TimePoint start = make_date(2026, 1, 5);
    auto gen = std::make_shared<events::FixedIntervalGenerator>(start, events::Days(7));
    auto series = std::make_unique<events::RecurrentEvent>(
        gen, events::Event("Turno", start, 86399s));
    series->setAllDay(true);

    QJsonObject json = persistence::activityToJson(*series);
    REQUIRE(json.value("type").toString() == "recurrent");
    REQUIRE(json.value("allday").toBool() == true);

    auto back = persistence::activityFromJson(json);
    REQUIRE(back != nullptr);
    auto rec = dynamic_cast<RecurrentEvent*>(back.get());
    REQUIRE(rec != nullptr);
    REQUIRE(rec->isAllDay());
    auto occ = rec->occurrencesIn(make_date(2026, 1, 5), make_date(2026, 1, 12));
    REQUIRE(occ.size() == 2);
    REQUIRE(occ[0].duration == Duration(86399));
}

TEST_CASE("Persistenza: input non validi rifiutati", "[json][invalid]") {
    SECTION("tipo di attivita' sconosciuto") {
        QJsonObject json{{"type", "ufo"}};
        QString error;
        REQUIRE(persistence::activityFromJson(json, &error) == nullptr);
        REQUIRE_FALSE(error.isEmpty());
    }

    SECTION("campo mancante") {
        QJsonObject json{{"type", "event"}, {"title", "X"}};
        REQUIRE(persistence::activityFromJson(json) == nullptr);
    }

    SECTION("data senza ora non ammessa") {
        QJsonObject json{{"type", "event"}, {"title", "X"}, {"start", "2026-01-01"}, {"duration_seconds", 60}};
        REQUIRE(persistence::activityFromJson(json) == nullptr);
    }

    SECTION("durata negativa") {
        QJsonObject json{{"type", "event"}, {"title", "X"}, {"start", "2026-01-01T00:00:00"}, {"duration_seconds", -60}};
        REQUIRE(persistence::activityFromJson(json) == nullptr);
    }

    SECTION("intervallo nullo del generatore") {
        QJsonObject gen{{"type", "fixed"}, {"start", "2026-01-01T00:00:00"}, {"interval_seconds", 0}};
        QJsonObject json{{"type", "recurrent"},
                         {"template", QJsonObject{{"title", "X"}, {"start", "2026-01-01T00:00:00"}, {"duration_seconds", 60}}},
                         {"generator", gen}};
        REQUIRE(persistence::activityFromJson(json) == nullptr);
    }

    SECTION("priorita' sconosciuta") {
        QJsonObject json{{"type", "task"}, {"title", "X"}, {"due", "2026-01-01T00:00:00"}, {"priority", "urgente"}};
        REQUIRE(persistence::activityFromJson(json) == nullptr);
    }

    SECTION("all-day con fine <= inizio") {
        QJsonObject json{{"type", "allday"}, {"title", "X"},
                         {"start", "2026-01-10T00:00:00"}, {"end", "2026-01-10T00:00:00"}};
        REQUIRE(persistence::activityFromJson(json) == nullptr);
    }

    SECTION("file JSON non valido") {
        QTemporaryDir dir;
        REQUIRE(dir.isValid());
        const QString path = dir.filePath("bad.json");
        QFile file(path);
        REQUIRE(file.open(QIODevice::WriteOnly));
        file.write("{{{{ non-json");
        file.close();

        Calendar calendar;
        QString error;
        REQUIRE_FALSE(persistence::loadFromFile(calendar, path, &error));
        REQUIRE_FALSE(error.isEmpty());
    }

    SECTION("file inesistente") {
        Calendar calendar;
        QString error;
        REQUIRE_FALSE(persistence::loadFromFile(calendar, "/path/che/non/esiste.json", &error));
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