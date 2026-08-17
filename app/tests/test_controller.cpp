#include <catch2/catch_all.hpp>
#include <QDateTime>
#include <QTemporaryDir>
#include <QTimeZone>

#include <algorithm>
#include <chrono>
#include <memory>

#include "CalendarController.h"
#include "events/domain/ActivityFactory.h"
#include "events/domain/Event.h"
#include "events/domain/RecurrentEvent.h"

using namespace std::chrono_literals;
using namespace events;

namespace {

QDateTime utc(int y, int m, int d, int h = 0, int min = 0) {
    return QDateTime(QDate(y, m, d), QTime(h, min), QTimeZone(0));
}

TimePoint tp(const QDateTime& t) {
    return TimePoint(std::chrono::seconds(t.toSecsSinceEpoch()));
}

// Trova l'occorrenza con l'inizio indicato
const Occurrence* findByStart(const std::vector<Occurrence>& occurrences,
                              const TimePoint& start) {
    const auto it = std::find_if(occurrences.begin(), occurrences.end(),
                                 [&start](const Occurrence& o) {
                                     return o.start == start;
                                 });
    return it == occurrences.end() ? nullptr : &(*it);
}

} // namespace

TEST_CASE("Controller: CRUD di base", "[controller]") {
    app::CalendarController controller;

    SECTION("add/search/remove") {
        controller.addActivity(ActivityFactory::createSimpleEvent(
            "Dentista", tp(utc(2026, 1, 8, 10)), 1h));
        controller.addActivity(ActivityFactory::createDeadline(
            "Consegna", tp(utc(2026, 1, 15)), Priority::High));
        controller.addActivity(ActivityFactory::createReminder(
            "Pillola", tp(utc(2026, 1, 9, 8)), "msg"));

        REQUIRE(controller.calendar().size() == 3);
        REQUIRE(controller.search("DENTISTA").size() == 1);
        REQUIRE(controller.search("").size() == 3);
        REQUIRE(controller.search("nulla").empty());

        const Activity* dentist = controller.search("Dentista")[0];
        REQUIRE(controller.removeActivity(dentist));
        REQUIRE(controller.calendar().size() == 2);
        REQUIRE_FALSE(controller.removeActivity(dentist));
    }

    SECTION("add rifiuta puntatore nullo") {
        REQUIRE_FALSE(controller.addActivity(nullptr));
    }
}

TEST_CASE("Controller: azioni sulle occorrenze", "[controller]") {
    app::CalendarController controller;

    SECTION("elimina occorrenza di un ricorrente = eccezione") {
        controller.addActivity(ActivityFactory::createSimpleWeekly(
            "Meeting", tp(utc(2026, 1, 5, 9)), 1h, tp(utc(2026, 2, 1))));
        auto occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
        REQUIRE(occurrences.size() == 4);

        const Occurrence* target = findByStart(occurrences, tp(utc(2026, 1, 12, 9)));
        REQUIRE(target != nullptr);
        REQUIRE(controller.deleteOccurrence(*target));

        occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
        REQUIRE(occurrences.size() == 3);
        REQUIRE(controller.calendar().size() == 1);  // il ricorrente resta
    }

    SECTION("elimina con truncate esclude le successive") {
        controller.addActivity(ActivityFactory::createSimpleWeekly(
            "Meeting", tp(utc(2026, 1, 5, 9)), 1h, tp(utc(2026, 2, 1))));
        auto occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
        REQUIRE(occurrences.size() == 4);

        const Occurrence* target = findByStart(occurrences, tp(utc(2026, 1, 19, 9)));
        REQUIRE(target != nullptr);
        REQUIRE(controller.deleteOccurrence(*target, true));

        occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
        REQUIRE(occurrences.size() == 2);  // 5/1 e 12/1
    }

    SECTION("elimina occorrenza di un evento singolo = elimina l'attivita'") {
        controller.addActivity(ActivityFactory::createSimpleEvent(
            "Dentista", tp(utc(2026, 1, 8, 10)), 1h));
        auto occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
        REQUIRE(occurrences.size() == 1);

        REQUIRE(controller.deleteOccurrence(occurrences[0]));
        REQUIRE(controller.calendar().empty());
    }

    SECTION("modifica istanza: eccezione + nuovo evento singolo") {
        controller.addActivity(ActivityFactory::createSimpleWeekly(
            "Meeting", tp(utc(2026, 1, 5, 9)), 1h, tp(utc(2026, 2, 1))));
        auto occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));

        const Occurrence* target = findByStart(occurrences, tp(utc(2026, 1, 12, 9)));
        REQUIRE(target != nullptr);
        auto replacement = std::make_unique<Event>(
            "Meeting (posticipato)", tp(utc(2026, 1, 12, 11)), 1h);
        REQUIRE(controller.modifyOccurrence(*target, std::move(replacement)));

        occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
        REQUIRE(occurrences.size() == 4);  // 3 del ricorrente + il nuovo singolo
        REQUIRE(controller.calendar().size() == 2);
    }
}

TEST_CASE("Controller: aggiornamento attivita' conserva le eccezioni", "[controller]") {
    app::CalendarController controller;
    controller.addActivity(ActivityFactory::createSimpleWeekly(
        "Meeting", tp(utc(2026, 1, 5, 9)), 1h, tp(utc(2026, 2, 1))));

    const Activity* original = controller.search("Meeting")[0];
    auto occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
    const Occurrence* target = findByStart(occurrences, tp(utc(2026, 1, 12, 9)));
    REQUIRE(target != nullptr);
    controller.deleteOccurrence(*target);  // aggiunge un'eccezione

    // modifica la regola (titolo e durata cambiano)
    auto updated = ActivityFactory::createSimpleWeekly(
        "Meeting (aggiornato)", tp(utc(2026, 1, 5, 9)), 2h, tp(utc(2026, 2, 1)));
    REQUIRE(controller.updateActivity(original, std::move(updated)));

    occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
    REQUIRE(occurrences.size() == 3);  // l'eccezione e' sopravvissuta all'aggiornamento
    REQUIRE(occurrences[0].duration == 2h);
}

TEST_CASE("Controller: spostamento di un'attivita' (drag&drop)", "[controller]") {
    app::CalendarController controller;

    SECTION("evento singolo: cambia inizio, durata invariata") {
        controller.addActivity(ActivityFactory::createSimpleEvent(
            "Dentista", tp(utc(2026, 1, 8, 10)), 1h));
        const events::Activity* activity = controller.search("Dentista")[0];

        const TimePoint newStart = tp(utc(2026, 1, 9, 15));
        REQUIRE(controller.moveActivity(activity, utc(2026, 1, 9, 15)));

        auto occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
        REQUIRE(occurrences.size() == 1);
        REQUIRE(occurrences[0].start == newStart);
        REQUIRE(occurrences[0].duration == 1h);
        REQUIRE(controller.search("Dentista")[0]->getStart() == newStart);
    }

    SECTION("ricorrente: inizio spostato, la fine NON slitta, serie intonsa") {
        controller.addActivity(ActivityFactory::createSimpleWeekly(
            "Riunione", tp(utc(2026, 1, 5, 9)), 1h, tp(utc(2026, 2, 16))));
        auto occurrences = controller.occurrencesIn(utc(2026, 1, 1), utc(2026, 1, 31));
        const Occurrence* second = findByStart(occurrences, tp(utc(2026, 1, 12, 9)));
        REQUIRE(second != nullptr);
        controller.deleteOccurrence(*second);  // EXDATE sul 12/1 (evento staccato)

        // Sposta la serie di una settimana AVANTI (fine originale 16/2 supera
        // ancora il nuovo inizio 12/1): la scadenza resta quella, non slitta.
        const events::Activity* activity = controller.search("Riunione")[0];
        REQUIRE(controller.moveActivity(activity, utc(2026, 1, 12, 9)));

        occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 2, 28));
        // serie intonsa dal 12/1: 12/1, 19/1, 26/1, 2/2, 9/2 (la fine resta
        // il 16/2 00:00, quindi il 16/2 09:00 e' escluso)
        REQUIRE(occurrences.size() == 5);
        REQUIRE(controller.search("Riunione")[0]->getStart() == tp(utc(2026, 1, 12, 9)));
        // nessuna occorrenza dopo la scadenza originale (16/2): 23/2 escluso
        REQUIRE(controller.occurrencesIn(utc(2026, 2, 16), utc(2026, 2, 28)).empty());
    }

    SECTION("ricorrente: inizio oltre la scadenza -> la scadenza sale al nuovo inizio") {
        controller.addActivity(ActivityFactory::createSimpleWeekly(
            "Riunione", tp(utc(2026, 1, 5, 9)), 1h, tp(utc(2026, 1, 19))));
        const events::Activity* activity = controller.search("Riunione")[0];
        REQUIRE(controller.moveActivity(activity, utc(2026, 2, 2, 9)));

        auto occurrences = controller.occurrencesIn(utc(2026, 2, 1), utc(2026, 3, 1));
        REQUIRE(occurrences.size() == 1);  // solo il nuovo inizio
        REQUIRE(occurrences[0].start == tp(utc(2026, 2, 2, 9)));
        REQUIRE(controller.search("Riunione")[0]->getStart() == tp(utc(2026, 2, 2, 9)));
    }

    SECTION("scadenza: cambia il due") {
        controller.addActivity(ActivityFactory::createDeadline(
            "Consegna", tp(utc(2026, 1, 15)), Priority::High));
        const events::Activity* d = controller.search("Consegna")[0];
        REQUIRE(controller.moveActivity(d, utc(2026, 2, 1)));
        REQUIRE(d->getStart() == tp(utc(2026, 2, 1)));
    }
}

TEST_CASE("Controller: salvataggio e caricamento su file", "[controller]") {
    app::CalendarController controller;
    controller.addActivity(ActivityFactory::createSimpleEvent(
        "Dentista", tp(utc(2026, 1, 8, 10)), 1h));
    controller.addActivity(ActivityFactory::createDeadline(
        "Consegna", tp(utc(2026, 1, 15)), Priority::High));

    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString path = dir.filePath("cal.json");

    QString error;
    REQUIRE(controller.saveToFile(path, &error));

    app::CalendarController other;
    REQUIRE(other.loadFromFile(path, &error));
    REQUIRE(other.calendar().size() == 2);
    REQUIRE(other.search("consegna").size() == 1);

    REQUIRE_FALSE(other.loadFromFile("/percorso/inesistente.json", &error));
    REQUIRE_FALSE(error.isEmpty());
}

int main(int argc, char* argv[]) {
    Catch::Session session;
    int returnCode = session.applyCommandLine(argc, argv);
    if (returnCode != 0) {
        return returnCode;
    }
    return session.run();
}
