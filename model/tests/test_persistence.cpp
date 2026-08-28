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

TEST_CASE("Persistenza: Event round-trip", "[json][event]") {
    TimePoint start = make_date(2026, 1, 8) + 10h;
    auto event = std::make_unique<Activity>(
        ActivityBuilder("Dentista", start).withDuration(1h).build());

    QJsonObject json = persistence::activityToJson(*event);
    REQUIRE(json.value("type").toString() == "event");
    REQUIRE(json.value("start").toString() == "2026-01-08T10:00:00");
    REQUIRE(json.value("duration_seconds").toInteger() == 3600);
    REQUIRE(json.value("generator").toObject().value("type").toString() == "single");
    REQUIRE_FALSE(json.contains("done_occurrences"));

    QString error;
    auto back = persistence::activityFromJson(json, &error);
    REQUIRE(back != nullptr);
    REQUIRE(back->getTitle() == "Dentista");
    REQUIRE(back->getStart() == start);
    REQUIRE(back->getDuration() == 1h);
    REQUIRE(back->occurrencesIn(start, start + 1h).size() == 1);
}

TEST_CASE("Persistenza: serie settimanale (fixed) round-trip", "[json][recurrent]") {
    TimePoint start = make_date(2026, 1, 1) + 9h;
    auto event = std::make_unique<Activity>(
        ActivityBuilder("Meeting")
            .withDuration(1h)
            .addGenerator(GeneratorBuilder::from(start)
                              .repeatEvery(events::Days(7))
                              .until(make_date(2026, 3, 1))
                              .build())
            .build());
    event->addException(make_date(2026, 1, 15) + 9h);

    QJsonObject json = persistence::activityToJson(*event);
    REQUIRE(json.value("type").toString() == "event");
    REQUIRE(json.value("generator").toObject().value("type").toString() == "fixed");
    REQUIRE_FALSE(json.contains("done_occurrences"));

    auto back = persistence::activityFromJson(json);
    REQUIRE(back != nullptr);
    REQUIRE(back->getTitle() == "Meeting");
    REQUIRE(back->getExceptions().size() == 1);
    // la ricorrenza si deduce dal generatore (fixed), non da un flag
    REQUIRE(dynamic_cast<const events::FixedIntervalGenerator*>(&back->getGenerator()) != nullptr);

    auto a = event->occurrencesIn(make_date(2026, 1, 1), make_date(2026, 2, 28));
    auto b = back->occurrencesIn(make_date(2026, 1, 1), make_date(2026, 2, 28));
    REQUIRE(a.size() == b.size());
    REQUIRE(a.size() == 8);  // 9 lunedi' meno l'eccezione del 15/1
    for (size_t i = 0; i < a.size(); ++i) {
        REQUIRE(a[i].start == b[i].start);
        REQUIRE(a[i].duration == b[i].duration);
    }
}

TEST_CASE("Persistenza: Task round-trip (occorrenze evase)", "[json][task]") {
    TimePoint due = make_date(2026, 3, 10);
    auto task = std::make_unique<Task>(
        TaskBuilder("Consegna", due).withPriority(Priority::High).build());
    task->setDone(due);

    QJsonObject json = persistence::activityToJson(*task);
    REQUIRE(json.value("type").toString() == "task");
    REQUIRE(json.value("priority").toString() == "high");
    REQUIRE(json.value("done_occurrences").toArray().size() == 1);

    auto back = persistence::activityFromJson(json);
    REQUIRE(back != nullptr);
    auto t = dynamic_cast<Task*>(back.get());
    REQUIRE(t != nullptr);
    REQUIRE(t->getTitle() == "Consegna");
    REQUIRE(t->getDue() == due);
    REQUIRE(t->getPriority() == Priority::High);
    REQUIRE(t->isDone(due));
    REQUIRE(t->getDoneOccurrences().size() == 1);
}

TEST_CASE("Persistenza: Meeting round-trip (luogo + partecipanti)", "[json][meeting]") {
    TimePoint start = make_date(2026, 2, 1) + 10h;
    auto meeting = std::make_unique<Meeting>(MeetingBuilder("Riunione", start)
                                                 .withDuration(90min)
                                                 .withLocation("Aula Magna")
                                                 .build());
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

TEST_CASE("Persistenza: Anniversario round-trip (leap-aware)", "[json][anniversary]") {
    auto anniversary = std::make_unique<Activity>(
        ActivityBuilder("Mario")
            .withDuration(std::chrono::hours(24) - std::chrono::seconds(1))
            .addGenerator(GeneratorBuilder::from(make_date(2028, 2, 29))
                              .repeatYearly()
                              .build())
            .build());

    QJsonObject json = persistence::activityToJson(*anniversary);
    REQUIRE(json.value("type").toString() == "event");
    REQUIRE(json.value("start").toString() == "2028-02-29T00:00:00");
    REQUIRE(json.value("generator").toObject().value("type").toString() == "yearly");
    REQUIRE_FALSE(json.contains("done_occurrences"));

    auto back = persistence::activityFromJson(json);
    REQUIRE(back != nullptr);
    REQUIRE(back->getStart() == make_date(2028, 2, 29));

    auto a = anniversary->occurrencesIn(make_date(2028, 1, 1), make_date(2031, 12, 31));
    auto b = back->occurrencesIn(make_date(2028, 1, 1), make_date(2031, 12, 31));
    REQUIRE(a.size() == b.size());
    REQUIRE(a.size() == 4);
    for (size_t i = 0; i < a.size(); ++i) {
        REQUIRE(a[i].start == b[i].start);
        REQUIRE(a[i].duration == b[i].duration);
    }
}

TEST_CASE("Persistenza: Calendar salva e ricarica da file", "[json][calendar][file]") {
    Calendar calendar;
    calendar.add(std::make_unique<Activity>(
        ActivityBuilder("Evento A", make_date(2026, 1, 1) + 9h).withDuration(1h).build()));
    calendar.add(std::make_unique<Activity>(
        ActivityBuilder("Riunione B")
            .withDuration(30min)
            .addGenerator(GeneratorBuilder::from(make_date(2026, 1, 2) + 10h)
                              .repeatEvery(events::Days(7))
                              .until(make_date(2026, 3, 1))
                              .build())
            .build()));
    calendar.add(std::make_unique<Task>(
        TaskBuilder("Compito C", make_date(2026, 2, 1)).withPriority(Priority::High).build()));
    calendar.add(std::make_unique<Meeting>(MeetingBuilder("Riunione D", make_date(2026, 1, 3) + 8h)
                                               .withDuration(1h)
                                               .withLocation("Zoom")
                                               .build()));
    // Evento "tutto il giorno": dalle 00:00 con durata 24h
    calendar.add(std::make_unique<Activity>(ActivityBuilder("Giornata E", make_date(2026, 1, 4))
                                                .withDuration(std::chrono::seconds(86400))
                                                .build()));
    calendar.add(std::make_unique<Activity>(
        ActivityBuilder("Anniversario F")
            .withDuration(std::chrono::hours(24) - std::chrono::seconds(1))
            .addGenerator(GeneratorBuilder::from(make_date(2000, 1, 6))
                              .repeatYearly()
                              .build())
            .build()));

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

TEST_CASE("Persistenza: serie mensile (con limite occorrenze)", "[json][recurrent][monthly]") {
    TimePoint start = make_date(2026, 1, 10) + 9h;
    Activity event("Pagamento", 1h,
                   GeneratorBuilder::from(start).repeatMonthly(2).limitTo(5).build());

    QJsonObject json = persistence::activityToJson(event);
    REQUIRE(json.value("generator").toObject().value("type").toString() == "monthly");
    REQUIRE(json.value("generator").toObject().value("interval_months").toInteger() == 2);
    REQUIRE(json.value("generator").toObject().value("max_occurrences").toInteger() == 5);

    auto back = persistence::activityFromJson(json);
    REQUIRE(back != nullptr);
    auto* monthly = dynamic_cast<const events::MaxOccurrencesDecorator*>(&back->getGenerator());
    REQUIRE(monthly != nullptr);
    REQUIRE(monthly->getMaxOccurrences() == 5);

    auto a = event.occurrencesIn(make_date(2026, 1, 1), make_date(2027, 1, 1));
    auto b = back->occurrencesIn(make_date(2026, 1, 1), make_date(2027, 1, 1));
    REQUIRE(a.size() == b.size());
    REQUIRE(a.size() == 5);  // 10/1, 10/3, 10/5, 10/7, 10/9
}

TEST_CASE("Persistenza: serie settimanale con limite occorrenze", "[json][recurrent][cap]") {
    TimePoint start = make_date(2026, 1, 5) + 9h;
    Activity event("Corso", 1h,
                   GeneratorBuilder::from(start).repeatEvery(events::Days(7)).limitTo(3).build());

    QJsonObject json = persistence::activityToJson(event);
    REQUIRE(json.value("generator").toObject().value("max_occurrences").toInteger() == 3);

    auto back = persistence::activityFromJson(json);
    REQUIRE(back != nullptr);
    auto* fixed = dynamic_cast<const events::MaxOccurrencesDecorator*>(&back->getGenerator());
    REQUIRE(fixed != nullptr);
    REQUIRE(fixed->getMaxOccurrences() == 3);
    REQUIRE(back->occurrencesIn(make_date(2026, 1, 1), make_date(2026, 3, 1)).size() == 3);
}

TEST_CASE("Persistenza: serie ricorrente di un giorno intero (00:00, 24h)", "[json][recurrent]") {
    TimePoint start = make_date(2026, 1, 5);
    auto gen = GeneratorBuilder::from(start).repeatEvery(events::Days(7)).build();
    Activity series("Turno", std::chrono::seconds(86400), std::move(gen));

    QJsonObject json = persistence::activityToJson(series);
    REQUIRE(json.value("type").toString() == "event");
    REQUIRE(json.value("duration_seconds").toInteger() == 86400);

    auto back = persistence::activityFromJson(json);
    REQUIRE(back != nullptr);
    auto occ = back->occurrencesIn(make_date(2026, 1, 5), make_date(2026, 1, 12));
    REQUIRE(occ.size() == 2);
    REQUIRE(occ[0].duration == Duration(86400));
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
        QJsonObject json{{"type", "event"},
                         {"title", "X"}, {"start", "2026-01-01T00:00:00"}, {"duration_seconds", 60},
                         {"generator", gen}};
        REQUIRE(persistence::activityFromJson(json) == nullptr);
    }

    SECTION("priorita' sconosciuta") {
        QJsonObject json{{"type", "task"}, {"title", "X"}, {"due", "2026-01-01T00:00:00"}, {"priority", "urgente"}};
        REQUIRE(persistence::activityFromJson(json) == nullptr);
    }

    SECTION("durata negativa di un evento") {
        QJsonObject json{{"type", "event"}, {"title", "X"},
                         {"start", "2026-01-10T00:00:00"}, {"duration_seconds", -1}};
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
