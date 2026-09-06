#include <catch2/catch_all.hpp>
#include <QDateTime>
#include <QTemporaryDir>
#include <QTimeZone>

#include <algorithm>
#include <chrono>
#include <memory>

#include "controller/CalendarController.h"
#include "events/builders/ActivityConfig.h"
#include "events/domain/Task.h"
#include "events/generators/FixedIntervalGenerator.h"
#include "views/ViewShared.h"

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
        controller.addActivity(makeActivity(ActivityConfig{
            .title = "Dentista", .start = tp(utc(2026, 1, 8, 10)), .duration = 1h}));
        controller.addActivity(makeTask(TaskConfig(
            ActivityConfig{.title = "Consegna", .start = tp(utc(2026, 1, 15))},
            Priority::High)));
        controller.addActivity(makeMeeting(MeetingConfig(
            ActivityConfig{.title = "Riunione", .start = tp(utc(2026, 1, 9, 8)), .duration = 1h})));

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
    activities.push_back(makeActivity(ActivityConfig{
        .title = "A", .start = tp(utc(2026, 1, 8, 10)), .duration = 1h}));
    activities.push_back(makeTask(TaskConfig(
        ActivityConfig{.title = "B", .start = tp(utc(2026, 1, 9))}, Priority::Medium)));
    activities.push_back(makeMeeting(MeetingConfig(
        ActivityConfig{.title = "C", .start = tp(utc(2026, 1, 10)), .duration = 1h})));

    REQUIRE(controller.addActivities(std::move(activities)));
    REQUIRE(controller.calendar().size() == 3);
    REQUIRE(controller.search("").size() == 3);

    SECTION("lista vuota rifiutata") {
        REQUIRE_FALSE(controller.addActivities({}));
    }
}

TEST_CASE("Controller: stato di completamento (toggleDone, solo su Task)", "[controller][done]") {
    app::CalendarController controller;

    SECTION("task: spunta globale") {
        controller.addActivity(makeTask(TaskConfig(
            ActivityConfig{.title = "Consegna", .start = tp(utc(2026, 1, 15))},
            Priority::High)));
        auto occurrences = controller.occurrencesIn(utc(2026, 1, 15), utc(2026, 1, 15));
        REQUIRE(occurrences.size() == 1);

        const auto* task = dynamic_cast<const Task*>(occurrences[0].source);
        REQUIRE(task != nullptr);
        REQUIRE_FALSE(task->isDone());
        REQUIRE(controller.toggleDone(occurrences[0]));
        REQUIRE(task->isDone());
        REQUIRE(controller.toggleDone(occurrences[0]));
        REQUIRE_FALSE(task->isDone());
    }

    SECTION("evento: toggle non ha effetto (nessuno stato)") {
        controller.addActivity(makeActivity(ActivityConfig{
            .title = "Dentista", .start = tp(utc(2026, 1, 8, 10)), .duration = 1h}));
        auto occurrences = controller.occurrencesIn(utc(2026, 1, 8, 0, 0), utc(2026, 1, 8, 23, 59));
        REQUIRE(occurrences.size() == 1);
        REQUIRE_FALSE(controller.toggleDone(occurrences[0]));
    }
}

TEST_CASE("Controller: azioni sulle occorrenze", "[controller]") {
    app::CalendarController controller;

    SECTION("elimina occorrenza di un ricorrente = eccezione") {
        controller.addActivity(makeActivity(ActivityConfig{
            .title = "Meeting",
            .start = tp(utc(2026, 1, 5, 9)),
            .duration = 1h,
            .end = tp(utc(2026, 2, 1)),
            .generator = std::make_shared<FixedIntervalGenerator>(std::chrono::days(7))}));
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
        controller.addActivity(makeActivity(ActivityConfig{
            .title = "Meeting",
            .start = tp(utc(2026, 1, 5, 9)),
            .duration = 1h,
            .end = tp(utc(2026, 2, 1)),
            .generator = std::make_shared<FixedIntervalGenerator>(std::chrono::days(7))}));
        auto occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
        REQUIRE(occurrences.size() == 4);

        const Occurrence* target = findByStart(occurrences, tp(utc(2026, 1, 19, 9)));
        REQUIRE(target != nullptr);
        REQUIRE(controller.deleteOccurrence(*target, true));

        occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
        REQUIRE(occurrences.size() == 2);  // 5/1 e 12/1
    }

    SECTION("elimina occorrenza di un evento singolo = elimina l'attivita'") {
        controller.addActivity(makeActivity(ActivityConfig{
            .title = "Dentista", .start = tp(utc(2026, 1, 8, 10)), .duration = 1h}));
        auto occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
        REQUIRE(occurrences.size() == 1);

        REQUIRE(controller.deleteOccurrence(occurrences[0]));
        REQUIRE(controller.calendar().empty());
    }

    SECTION("modifica istanza: eccezione + nuovo evento singolo") {
        controller.addActivity(makeActivity(ActivityConfig{
            .title = "Meeting",
            .start = tp(utc(2026, 1, 5, 9)),
            .duration = 1h,
            .end = tp(utc(2026, 2, 1)),
            .generator = std::make_shared<FixedIntervalGenerator>(std::chrono::days(7))}));
        auto occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));

        const Occurrence* target = findByStart(occurrences, tp(utc(2026, 1, 12, 9)));
        REQUIRE(target != nullptr);
        auto replacement = makeActivity(ActivityConfig{
            .title = "Meeting (posticipato)", .start = tp(utc(2026, 1, 12, 11)), .duration = 1h});
        REQUIRE(controller.modifyOccurrence(*target, std::move(replacement)));

        occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
        REQUIRE(occurrences.size() == 4);  // 3 del ricorrente + il nuovo singolo
        REQUIRE(controller.calendar().size() == 2);
    }
}

TEST_CASE("Controller: aggiornamento attivita' conserva le eccezioni", "[controller]") {
    app::CalendarController controller;
    controller.addActivity(makeActivity(ActivityConfig{
        .title = "Meeting",
        .start = tp(utc(2026, 1, 5, 9)),
        .duration = 1h,
        .end = tp(utc(2026, 2, 1)),
        .generator = std::make_shared<FixedIntervalGenerator>(std::chrono::days(7))}));

    const Activity* original = controller.search("Meeting")[0];
    auto occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
    const Occurrence* target = findByStart(occurrences, tp(utc(2026, 1, 12, 9)));
    REQUIRE(target != nullptr);
    controller.deleteOccurrence(*target);  // aggiunge un'eccezione

    // modifica la regola (titolo e durata cambiano)
    auto updated = makeActivity(ActivityConfig{
        .title = "Meeting (aggiornato)",
        .start = tp(utc(2026, 1, 5, 9)),
        .duration = 2h,
        .end = tp(utc(2026, 2, 1)),
        .generator = std::make_shared<FixedIntervalGenerator>(std::chrono::days(7))});
    REQUIRE(controller.updateActivity(original, std::move(updated)));

    occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
    REQUIRE(occurrences.size() == 3);  // l'eccezione e' sopravvissuta all'aggiornamento
    REQUIRE(occurrences[0].duration == 2h);
}

TEST_CASE("Controller: spostamento di un'attivita' (drag&drop)", "[controller]") {
    app::CalendarController controller;

    SECTION("evento singolo: cambia inizio, durata invariata") {
        controller.addActivity(makeActivity(ActivityConfig{
            .title = "Dentista", .start = tp(utc(2026, 1, 8, 10)), .duration = 1h}));
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
        controller.addActivity(makeActivity(ActivityConfig{
            .title = "Riunione",
            .start = tp(utc(2026, 1, 5, 9)),
            .duration = 1h,
            .end = tp(utc(2026, 2, 16)),
            .generator = std::make_shared<FixedIntervalGenerator>(std::chrono::days(7))}));
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
        controller.addActivity(makeTask(TaskConfig(
            ActivityConfig{.title = "Consegna", .start = tp(utc(2026, 1, 15))},
            Priority::High)));
        const events::Activity* t = controller.search("Consegna")[0];
        REQUIRE(controller.moveActivity(t, utc(2026, 2, 1)));
        REQUIRE(t->getStart() == tp(utc(2026, 2, 1)));
    }
}

TEST_CASE("Controller: drag di una sola occorrenza di una serie (buco in origine)", "[controller]") {
    app::CalendarController controller;

    // Serie giornaliera per una settimana (lun 5/1 09:00, 1h)
    controller.addActivity(makeActivity(ActivityConfig{
        .title = "Allenamento",
        .start = tp(utc(2026, 1, 5, 9)),
        .duration = 1h,
        .generator = std::make_shared<FixedIntervalGenerator>(std::chrono::hours(24))}));
    auto occurrences = controller.occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 12));
    REQUIRE(occurrences.size() == 7);

    // Sposta SOLO la seconda occorrenza (mar 6/1 09:00) alla destinazione
    const Occurrence* second = findByStart(occurrences, tp(utc(2026, 1, 6, 9)));
    REQUIRE(second != nullptr);
    auto replacement = makeActivity(ActivityConfig{
        .title = second->source->getTitle(),
        .start = tp(utc(2026, 1, 8, 15)),
        .duration = second->duration});
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
    controller.addActivity(makeActivity(ActivityConfig{
        .title = "Lezione",
        .start = tp(utc(2026, 1, 5, 8)),
        .duration = 2h,
        .end = tp(utc(2026, 1, 12)),
        .generator = std::make_shared<FixedIntervalGenerator>(std::chrono::hours(24))}));

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
    controller.addActivity(makeActivity(ActivityConfig{
        .title = "Dentista", .start = tp(utc(2026, 1, 8, 10)), .duration = 1h}));
    controller.addActivity(makeTask(TaskConfig(
        ActivityConfig{.title = "Consegna", .start = tp(utc(2026, 1, 15))},
        Priority::High)));

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

TEST_CASE("ALLDAY fix verification", "[all-day]") {
    QDate monday(2026, 8, 31);
    REQUIRE(monday.dayOfWeek() == 1);

    // Simulate the FIXED creation path: all-day start at UTC midnight.
    auto toTP = [](const QDateTime& d) {
        return TimePoint(std::chrono::seconds(d.toSecsSinceEpoch()));
    };
    auto toLocalDate = [](const TimePoint tp) {
        return QDateTime::fromSecsSinceEpoch(tp.time_since_epoch().count())
            .toLocalTime()
            .date();
    };

    const TimePoint startUtc =
        toTP(QDateTime(monday, QTime(0, 0), QTimeZone(0)));

    app::CalendarController controller;
    auto ev = makeActivity(ActivityConfig{
        .title = "AllDay", .start = startUtc, .duration = std::chrono::seconds(86400)});
    controller.addActivity(std::move(ev));

    // Week query (UTC) for the week of Monday
    const QDateTime weekFrom(monday, QTime(0, 0), QTimeZone(0));
    const QDateTime weekTo = QDateTime(monday.addDays(7), QTime(0, 0),
                                       QTimeZone(0))
                                 .addSecs(-1);
    const auto occs = controller.occurrencesIn(weekFrom, weekTo);
    REQUIRE(occs.size() == 1);

    // Display: per un evento "tutto il giorno" l'ora mostrata deve essere
    // 00:00 (salvato a mezzanotte UTC), non l'ora locale spostata (02:00).
    const QDateTime shownStart =
        app::activityDisplayTime(occs[0].source, occs[0].start);
    REQUIRE(shownStart.toString(QStringLiteral("HH:mm")) == QStringLiteral("00:00"));
    REQUIRE(shownStart.date() == monday);

    // L'evento salvato a mezzanotte UTC deve essere riconosciuto come
    // "tutto il giorno" (2 mezzenotti consecutive nel suo intervallo).
    REQUIRE(app::coversFullDay(occs[0]));
    // Strip placement: il giorno (locale) in cui parte l'occorrenza deve
    // essere il lunedi' stesso (firstDay = 0), non la domenica precedente.
    REQUIRE(monday.daysTo(toLocalDate(occs[0].start)) == 0);
}

TEST_CASE("ALLDAY: un evento normale non e' all-day", "[all-day]") {
    // Evento breve (es. 10:00-11:00 locali): non deve finire nella striscia.
    const QDateTime startLocal(QDate(2026, 8, 31), QTime(10, 0));
    const TimePoint start =
        TimePoint(std::chrono::seconds(startLocal.toSecsSinceEpoch()));
    app::CalendarController controller;
    controller.addActivity(makeActivity(ActivityConfig{
        .title = "Riunione", .start = start, .duration = std::chrono::minutes(60)}));
    const QDateTime dayFrom(QDate(2026, 8, 31), QTime(0, 0), QTimeZone(0));
    const auto occs = controller.occurrencesIn(
        dayFrom, QDateTime(QDate(2026, 8, 31), QTime(23, 59), QTimeZone(0)));
    REQUIRE(occs.size() == 1);
    REQUIRE_FALSE(app::coversFullDay(occs[0]));
}

TEST_CASE("ALLDAY: piu' eventi all-day vengono affiancati su righe distinte",
          "[all-day]") {
    // Replica dell'algoritmo di layout della striscia in WeekView::ensureRects:
    // ogni evento all-day va sulla riga piu' alta libera in tutti i giorni che
    // copre; eventi su giorni diversi possono condividere la riga 0.
    const int kDayCount = 7;
    const QDate monday(2026, 8, 31);

    // Ogni item: {firstDay, lastDay, row}
    struct Item {
        int first, last, row;
    };
    std::vector<Item> items;
    std::vector<std::vector<bool>> dayRows(kDayCount);

    auto place = [&](int firstDay, int lastDay) {
        int row = 0;
        bool free = false;
        while (!free) {
            free = true;
            for (int d = firstDay; d <= lastDay; ++d) {
                if (static_cast<int>(dayRows[d].size()) > row && dayRows[d][row]) {
                    free = false;
                    ++row;
                    break;
                }
            }
        }
        for (int d = firstDay; d <= lastDay; ++d) {
            if (static_cast<int>(dayRows[d].size()) <= row) {
                dayRows[d].resize(row + 1, false);
            }
            dayRows[d][row] = true;
        }
        items.push_back({firstDay, lastDay, row});
    };

    // Due eventi all-day lo stesso lunedi' -> righe diverse (0 e 1)
    place(0, 0);
    place(0, 0);
    REQUIRE(items[0].row == 0);
    REQUIRE(items[1].row == 1);

    // Un evento che copre lunedi' e martedi' sfrutta la riga libera
    items.clear();
    dayRows.assign(kDayCount, {});
    place(0, 1);   // lun-mar -> riga 0
    place(0, 0);   // lun -> deve scendere alla riga 1
    REQUIRE(items[0].row == 0);
    REQUIRE(items[1].row == 1);

    // Due eventi su giorni diversi -> entrambi in riga 0
    items.clear();
    dayRows.assign(kDayCount, {});
    place(0, 0);   // lun
    place(2, 2);   // mer
    REQUIRE(items[0].row == 0);
    REQUIRE(items[1].row == 0);
}
