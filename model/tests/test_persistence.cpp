#include <catch2/catch_all.hpp>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include <chrono>
#include <memory>

#include "events/events.h"
#include "persistence/JsonPersistence.h"

using namespace std::chrono_literals;
using namespace events;

TimePoint make_date(int y, int m, int d) {
    return std::chrono::sys_days{std::chrono::year{y}/std::chrono::month{static_cast<unsigned>(m)}/std::chrono::day{static_cast<unsigned>(d)}};
}

TEST_CASE("Persistenza: Event round-trip", "[json][event]") {
    TimePoint start = make_date(2026, 1, 8) + 10h;
    auto event = ActivityFactory::createSimpleEvent("Dentista", start, 1h);

    QJsonObject json = persistence::activityToJson(*event);
    REQUIRE(json.value("type").toString() == "event");
    REQUIRE(json.value("start").toString() == "2026-01-08T10:00:00");
    REQUIRE(json.value("duration_seconds").toInteger() == 3600);

    QString error;
    auto back = persistence::activityFromJson(json, &error);
    REQUIRE(back != nullptr);
    auto e = dynamic_cast<Event*>(back.get());
    REQUIRE(e != nullptr);
    REQUIRE(e->getTitle() == "Dentista");
    REQUIRE(e->getStart() == start);
    REQUIRE(e->getDuration() == 1h);
}

TEST_CASE("Persistenza: RecurrentEvent (fixed) round-trip", "[json][recurrent]") {
    TimePoint start = make_date(2026, 1, 1) + 9h;
    auto event = ActivityFactory::createSimpleWeekly("Meeting", start, 1h, make_date(2026, 3, 1));
    event->addException(make_date(2026, 1, 15) + 9h);

    QJsonObject json = persistence::activityToJson(*event);
    REQUIRE(json.value("type").toString() == "recurrent");
    REQUIRE(json.value("generator").toObject().value("type").toString() == "fixed");

    auto back = persistence::activityFromJson(json);
    REQUIRE(back != nullptr);
    auto rec = dynamic_cast<RecurrentEvent*>(back.get());
    REQUIRE(rec != nullptr);
    REQUIRE(rec->getTitle() == "Meeting");
    REQUIRE(rec->getExceptions().size() == 1);

    auto a = event->occurrencesIn(make_date(2026, 1, 1), make_date(2026, 2, 28));
    auto b = rec->occurrencesIn(make_date(2026, 1, 1), make_date(2026, 2, 28));
    REQUIRE(a.size() == b.size());
    REQUIRE(a.size() == 8);  // 9 lunedi' meno l'eccezione del 15/1
    for (size_t i = 0; i < a.size(); ++i) {
        REQUIRE(a[i].start == b[i].start);
        REQUIRE(a[i].duration == b[i].duration);
    }
}

TEST_CASE("Persistenza: compleanno (yearly) round-trip", "[json][yearly]") {
    auto birthday = ActivityFactory::createBirthday("Mario", 2028y/2/29);

    QJsonObject json = persistence::activityToJson(*birthday);
    REQUIRE(json.value("generator").toObject().value("type").toString() == "yearly");

    auto back = persistence::activityFromJson(json);
    REQUIRE(back != nullptr);
    auto rec = dynamic_cast<RecurrentEvent*>(back.get());
    REQUIRE(rec != nullptr);

    auto a = birthday->getSchedulable(make_date(2028, 1, 1), make_date(2028, 1, 1) + std::chrono::years(7));
    auto b = rec->getSchedulable(make_date(2028, 1, 1), make_date(2028, 1, 1) + std::chrono::years(7));
    REQUIRE(a.size() == b.size());
    REQUIRE(a.size() == 7);  // 2028..2034: anni bisestili gestiti dal generatore
    for (size_t i = 0; i < a.size(); ++i) {
        REQUIRE(a[i]->getStart() == b[i]->getStart());
        REQUIRE(a[i]->getDuration() == b[i]->getDuration());
    }
}

TEST_CASE("Persistenza: Deadline round-trip", "[json][deadline]") {
    TimePoint due = make_date(2026, 3, 10);
    auto deadline = ActivityFactory::createDeadline("Consegna", due, Priority::High);
    deadline->setDone();

    QJsonObject json = persistence::activityToJson(*deadline);
    REQUIRE(json.value("type").toString() == "deadline");
    REQUIRE(json.value("priority").toString() == "high");
    REQUIRE(json.value("done").toBool() == true);

    auto back = persistence::activityFromJson(json);
    REQUIRE(back != nullptr);
    auto d = dynamic_cast<Deadline*>(back.get());
    REQUIRE(d != nullptr);
    REQUIRE(d->getTitle() == "Consegna");
    REQUIRE(d->getDue() == due);
    REQUIRE(d->getPriority() == Priority::High);
    REQUIRE(d->isDone());
}

TEST_CASE("Persistenza: Reminder round-trip", "[json][reminder]") {
    TimePoint trigger = make_date(2026, 2, 1) + 8h;
    auto reminder = ActivityFactory::createReminder("Pillola", trigger, "Prendi la pillola", Days(1));

    QJsonObject json = persistence::activityToJson(*reminder);
    REQUIRE(json.value("type").toString() == "reminder");
    REQUIRE(json.value("repeat_seconds").toInteger() == 86400);

    auto back = persistence::activityFromJson(json);
    REQUIRE(back != nullptr);
    auto r = dynamic_cast<Reminder*>(back.get());
    REQUIRE(r != nullptr);
    REQUIRE(r->getMessage() == "Prendi la pillola");
    REQUIRE(r->isRepeating());

    auto a = reminder->occurrencesIn(make_date(2026, 2, 1), make_date(2026, 2, 8));
    auto b = r->occurrencesIn(make_date(2026, 2, 1), make_date(2026, 2, 8));
    REQUIRE(a.size() == b.size());
    REQUIRE(a.size() == 7);
}

TEST_CASE("Persistenza: Calendar salva e ricarica da file", "[json][calendar][file]") {
    Calendar calendar;
    calendar.add(ActivityFactory::createSimpleEvent("Evento A", make_date(2026, 1, 1) + 9h, 1h));
    calendar.add(ActivityFactory::createSimpleWeekly("Riunione B", make_date(2026, 1, 2) + 10h, 30min, make_date(2026, 3, 1)));
    calendar.add(ActivityFactory::createDeadline("Scadenza C", make_date(2026, 2, 1), Priority::High));
    calendar.add(ActivityFactory::createReminder("Promemoria D", make_date(2026, 1, 3), "msg", Days(1)));

    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString path = dir.filePath("calendar.json");

    QString error;
    REQUIRE(persistence::saveToFile(calendar, path, &error));

    Calendar loaded;
    REQUIRE(persistence::loadFromFile(loaded, path, &error));
    REQUIRE(loaded.size() == 4);

    auto occ1 = calendar.occurrencesIn(make_date(2026, 1, 1), make_date(2026, 2, 28));
    auto occ2 = loaded.occurrencesIn(make_date(2026, 1, 1), make_date(2026, 2, 28));
    REQUIRE(occ1.size() == occ2.size());
    for (size_t i = 0; i < occ1.size(); ++i) {
        REQUIRE(occ1[i].start == occ2[i].start);
        REQUIRE(occ1[i].duration == occ2[i].duration);
        REQUIRE(occ1[i].source->getTitle() == occ2[i].source->getTitle());
    }

    REQUIRE(loaded.search("scadenza").size() == 1);
    REQUIRE(loaded.search("promemoria").size() == 1);
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
        QJsonObject json{{"type", "deadline"}, {"title", "X"}, {"due", "2026-01-01T00:00:00"}, {"priority", "urgente"}};
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
