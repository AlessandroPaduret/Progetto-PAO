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
#include "events/domain/Task.h"

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
        controller.addActivity(ActivityFactory::createTask(
            "Consegna", tp(utc(2026, 1, 15)), Priority::High));
        controller.addActivity(ActivityFactory::createMeeting(
            "Riunione", tp(utc(2026, 1, 9, 8)), 1h));

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

TEST_CASE("Controller: addActivities aggiunge piu' attivita' in un colpo", "[controller]") {
    app::CalendarController controller;
    std::vector<std::unique_ptr<events::Activity>> activities;
    activities.push_back(ActivityFactory::createSimpleEvent(
        "A", tp(utc(2026, 1, 8, 10)), 1h));
    activities.push_back(ActivityFactory::createTask(
        "B", tp(utc(2026, 1, 9)), Priority::Medium));
    activities.push_back(ActivityFactory::createMeeting(
        "C", tp(utc(2026, 1, 10)), 1h));

    REQUIRE(controller.addActivities(std::move(activities)));
    REQUIRE(controller.calendar().size() == 3);
    REQUIRE(controller.search("").size() == 3);

    SECTION("lista vuota rifiutata") {
        REQUIRE_FALSE(controller.addActivities({}));
    }
}

TEST_CASE("Controller: stato di completamento (toggleDone)", "[controller][done]") {
    app::CalendarController controller;

    SECTION("evento singolo: spunta globale") {
        controller.addActivity(ActivityFactory::createSimpleEvent(
            "Dentista", tp(utc(2026, 1, 8, 10)), 1h));
        auto occurrences = controller.occurrencesIn(utc(2026, 1, 8, 0, 0), utc(2026, 1, 8, 23, 59));
        REQUIRE(occurrences.size() == 1);

        REQUIRE(controller.toggleDone(occurrences[0]));
        REQUIRE(occurrences[0].source->isDone());
        REQUIRE(controller.toggleDone(occurrences[0]));
        REQUIRE_FALSE(occurrences[0].source->isDone());
    }

    SECTION("serie: stato per singola occorrenza") {
        controller.addActivity(ActivityFactory::createSimpleWeekly(
            "Lezione", tp(utc(2026, 1, 5, 9)), 1h, tp(utc(2026, 2, 1))));
        auto occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
        REQUIRE(occurrences.size() == 4);

        // Spunta solo la seconda lezione
        const Occurrence* second = findByStart(occurrences, tp(utc(2026, 1, 12, 9)));
        REQUIRE(second != nullptr);
        REQUIRE(controller.toggleDone(*second));

        const auto* series = dynamic_cast<const RecurrentEvent*>(second->source);
        REQUIRE(series != nullptr);
        REQUIRE(series->isDoneAt(tp(utc(2026, 1, 12, 9))));
        REQUIRE_FALSE(series->isDoneAt(tp(utc(2026, 1, 5, 9))));
        REQUIRE(series->getDoneOccurrences().size() == 1);
    }

    SECTION("anniversario: stato per singola ricorrenza") {
        controller.addActivity(ActivityFactory::createAnniversary(
            "Mario", tp(utc(2028, 2, 29))));
        auto occurrences = controller.occurrencesIn(utc(2028, 1, 1), utc(2030, 12, 31));
        REQUIRE(occurrences.size() == 3);

        const Occurrence* occ2029 = findByStart(occurrences, tp(utc(2029, 2, 28)));
        REQUIRE(occ2029 != nullptr);
        REQUIRE(controller.toggleDone(*occ2029));
        REQUIRE(occ2029->source->isDoneAt(tp(utc(2029, 2, 28))));
        REQUIRE_FALSE(occ2029->source->isDoneAt(tp(utc(2028, 2, 29))));
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
        REQUIRE(occurrences.size() == 5);
        REQUIRE(controller.search("Riunione")[0]->getStart() == tp(utc(2026, 1, 12, 9)));
        REQUIRE(controller.occurrencesIn(utc(2026, 2, 16), utc(2026, 2, 28)).empty());
    }

    SECTION("task: cambia la scadenza") {
        controller.addActivity(ActivityFactory::createTask(
            "Consegna", tp(utc(2026, 1, 15)), Priority::High));
        const events::Activity* t = controller.search("Consegna")[0];
        REQUIRE(controller.moveActivity(t, utc(2026, 2, 1)));
        REQUIRE(t->getStart() == tp(utc(2026, 2, 1)));
    }
}

TEST_CASE("Controller: drag di una sola occorrenza di una serie (buco in origine)", "[controller]") {
    app::CalendarController controller;

    // Serie giornaliera per una settimana (lun 5/1 09:00, 1h)
    controller.addActivity(ActivityFactory::createRecurrentEvent(
        "Allenamento", tp(utc(2026, 1, 5, 9)), 1h, std::chrono::hours(24)));
    auto occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 12));
    REQUIRE(occurrences.size() == 7);

    // Sposta SOLO la seconda occorrenza (mar 6/1 09:00) alla destinazione
    const Occurrence* second = findByStart(occurrences, tp(utc(2026, 1, 6, 9)));
    REQUIRE(second != nullptr);
    auto replacement = std::make_unique<Event>(
        second->source->getTitle(), tp(utc(2026, 1, 8, 15)), second->duration);
    REQUIRE(controller.modifyOccurrence(*second, std::move(replacement)));

    occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 12));

    // 1) BUCO IN ORIGINE: nessuna occorrenza della serie il 6/1 09:00
    bool serieSul6 = false;
    for (const auto& o : occurrences) {
        if (o.source == second->source && o.start == tp(utc(2026, 1, 6, 9))) {
            serieSul6 = true;
        }
    }
    REQUIRE_FALSE(serieSul6);

    // 2) L'evento singolo e' alla destinazione (8/1 15:00)
    bool singoloAllaDestinazione = false;
    for (const auto& o : occurrences) {
        if (o.source != second->source && o.start == tp(utc(2026, 1, 8, 15))) {
            singoloAllaDestinazione = true;
        }
    }
    REQUIRE(singoloAllaDestinazione);

    // 3) La serie continua negli altri giorni (6 occorrenze su 7)
    int serieCount = 0;
    for (const auto& o : occurrences) {
        if (o.source == second->source) ++serieCount;
    }
    REQUIRE(serieCount == 6);
}

TEST_CASE("Controller: 'da questo momento in poi' divide la serie", "[controller]") {
    app::CalendarController controller;

    // Serie giornaliera 8:00-10:00 (2h) per una settimana, fine 12/1 00:00
    auto generator = std::make_shared<FixedIntervalGenerator>(
        tp(utc(2026, 1, 5, 8)), std::chrono::hours(24), tp(utc(2026, 1, 12)));
    controller.addActivity(std::make_unique<RecurrentEvent>(
        generator, Event("Lezione", tp(utc(2026, 1, 5, 8)), 2h)));

    auto occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 12));
    REQUIRE(occurrences.size() == 7);  // 5/1 .. 11/1 alle 08:00

    // Il giorno 4 (8/1 08:00) diventa 10:00-12:00: split con nuovo inizio
    const Occurrence* day4 = findByStart(occurrences, tp(utc(2026, 1, 8, 8)));
    REQUIRE(day4 != nullptr);
    REQUIRE(controller.splitRecurrence(*day4, utc(2026, 1, 8, 10)));

    occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 12));

    // 1) la serie ATTUALE e' fermata prima del giorno 4: occorrenze 5/1..7/1
    int oldSeriesCount = 0;
    for (const auto& o : occurrences) {
        if (o.source == day4->source) ++oldSeriesCount;
    }
    REQUIRE(oldSeriesCount == 3);

    // 2) la NUOVA serie inizia l'8/1 alle 10:00 e continua ogni giorno
    const events::Activity* nuova = nullptr;
    for (const auto& activity : controller.calendar()) {
        if (activity.get() != day4->source) nuova = activity.get();
    }
    REQUIRE(nuova != nullptr);
    REQUIRE(nuova->getStart() == tp(utc(2026, 1, 8, 10)));

    int newSeriesCount = 0;
    for (const auto& o : occurrences) {
        if (o.source == nuova) {
            ++newSeriesCount;
            REQUIRE(o.duration == 2h);
        }
    }
    REQUIRE(newSeriesCount == 4);

    // 3) la data di scadenza e' rimasta INVARIATA (12/1 00:00)
    REQUIRE(controller.occurrencesIn(utc(2026, 1, 12), utc(2026, 1, 15)).empty());

    // 4) totale: 3 + 4 = 7 occorrenze, 2 attivita'
    REQUIRE(occurrences.size() == 7);
    REQUIRE(controller.calendar().size() == 2);
}

TEST_CASE("Controller: salvataggio e caricamento su file", "[controller]") {
    app::CalendarController controller;
    controller.addActivity(ActivityFactory::createSimpleEvent(
        "Dentista", tp(utc(2026, 1, 8, 10)), 1h));
    controller.addActivity(ActivityFactory::createTask(
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