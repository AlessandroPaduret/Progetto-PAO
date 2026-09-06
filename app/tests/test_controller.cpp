#include <QTest>
#include <QObject>
#include <QDateTime>
#include <QTemporaryDir>
#include <QTimeZone>

#include <algorithm>
#include <chrono>
#include <memory>

#include "controller/CalendarController.h"
#include "builders/ActivityConfig.h"
#include "domain/Task.h"
#include "generators/FixedIntervalGenerator.h"
#include "views/utils/ViewShared.h"

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

class TestController : public QObject {
    Q_OBJECT

private slots:
    // Un CalendarController nuovo prima di ogni test: replica la semantica
    // "fixture fresca per sezione" di Catch2 senza condividere stato tra slot.
    void init();

    void testCrudAddSearchRemove();
    void testCrudAddRejectsNullptr();

    void testAddActivitiesBatch();

    void testToggleDoneTask();
    void testToggleDoneNonTaskHasNoEffect();

    void testDeleteOccurrenceRecurrentAddsException();
    void testDeleteOccurrenceTruncateExcludesFollowing();
    void testDeleteOccurrenceSingleRemovesActivity();
    void testModifyOccurrenceCreatesExceptionAndSingleEvent();

    void testUpdateActivityPreservesExceptions();

    void testMoveSingleActivity();
    void testMoveRecurrentActivityKeepsSeriesIntactAndEnd();
    void testMoveTaskChangesDueDate();

    void testDragSingleOccurrenceLeavesGapInSeries();
    void testSplitRecurrenceFromThisMomentOn();

    void testSaveAndLoadFile();

    void testAllDayFixVerification();
    void testAllDayNormalEventIsNotAllDay();
    void testAllDayStripLayoutRows();

private:
    std::unique_ptr<app::CalendarController> m_controller;
};

void TestController::init() {
    m_controller = std::make_unique<app::CalendarController>();
}

void TestController::testCrudAddSearchRemove() {
    m_controller->addActivity(makeActivity(ActivityConfig{
        .title = "Dentista", .start = tp(utc(2026, 1, 8, 10)), .duration = 1h}));
    m_controller->addActivity(makeTask(TaskConfig(
        ActivityConfig{.title = "Consegna", .start = tp(utc(2026, 1, 15))},
        Priority::High)));
    m_controller->addActivity(makeMeeting(MeetingConfig(
        ActivityConfig{.title = "Riunione", .start = tp(utc(2026, 1, 9, 8)), .duration = 1h})));

    QCOMPARE(m_controller->calendar().size(), static_cast<std::size_t>(3));
    QCOMPARE(m_controller->search("DENTISTA").size(), static_cast<std::size_t>(1));
    QCOMPARE(m_controller->search("").size(), static_cast<std::size_t>(3));
    QVERIFY(m_controller->search("nulla").empty());

    const Activity* dentist = m_controller->search("Dentista")[0];
    QVERIFY(m_controller->removeActivity(dentist));
    QCOMPARE(m_controller->calendar().size(), static_cast<std::size_t>(2));
    QVERIFY(!m_controller->removeActivity(dentist));
}

void TestController::testCrudAddRejectsNullptr() {
    QVERIFY(!m_controller->addActivity(nullptr));
}

void TestController::testAddActivitiesBatch() {
    std::vector<std::unique_ptr<events::Activity>> activities;
    activities.push_back(makeActivity(ActivityConfig{
        .title = "A", .start = tp(utc(2026, 1, 8, 10)), .duration = 1h}));
    activities.push_back(makeTask(TaskConfig(
        ActivityConfig{.title = "B", .start = tp(utc(2026, 1, 9))}, Priority::Medium)));
    activities.push_back(makeMeeting(MeetingConfig(
        ActivityConfig{.title = "C", .start = tp(utc(2026, 1, 10)), .duration = 1h})));

    QVERIFY(m_controller->addActivities(std::move(activities)));
    QCOMPARE(m_controller->calendar().size(), static_cast<std::size_t>(3));
    QCOMPARE(m_controller->search("").size(), static_cast<std::size_t>(3));

    // Lista vuota rifiutata
    QVERIFY(!m_controller->addActivities({}));
}

void TestController::testToggleDoneTask() {
    m_controller->addActivity(makeTask(TaskConfig(
        ActivityConfig{.title = "Consegna", .start = tp(utc(2026, 1, 15))},
        Priority::High)));
    auto occurrences = m_controller->occurrencesIn(utc(2026, 1, 15), utc(2026, 1, 15));
    QCOMPARE(occurrences.size(), static_cast<std::size_t>(1));

    const auto* task = dynamic_cast<const Task*>(occurrences[0].source);
    QVERIFY(task != nullptr);
    QVERIFY(!task->isDone());
    QVERIFY(m_controller->toggleDone(occurrences[0]));
    QVERIFY(task->isDone());
    QVERIFY(m_controller->toggleDone(occurrences[0]));
    QVERIFY(!task->isDone());
}

void TestController::testToggleDoneNonTaskHasNoEffect() {
    m_controller->addActivity(makeActivity(ActivityConfig{
        .title = "Dentista", .start = tp(utc(2026, 1, 8, 10)), .duration = 1h}));
    auto occurrences = m_controller->occurrencesIn(utc(2026, 1, 8, 0, 0), utc(2026, 1, 8, 23, 59));
    QCOMPARE(occurrences.size(), static_cast<std::size_t>(1));
    QVERIFY(!m_controller->toggleDone(occurrences[0]));
}

void TestController::testDeleteOccurrenceRecurrentAddsException() {
    m_controller->addActivity(makeActivity(ActivityConfig{
        .title = "Meeting",
        .start = tp(utc(2026, 1, 5, 9)),
        .duration = 1h,
        .end = tp(utc(2026, 2, 1)),
        .generator = std::make_shared<FixedIntervalGenerator>(std::chrono::days(7))}));
    auto occurrences = m_controller->occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
    QCOMPARE(occurrences.size(), static_cast<std::size_t>(4));

    const Occurrence* target = findByStart(occurrences, tp(utc(2026, 1, 12, 9)));
    QVERIFY(target != nullptr);
    QVERIFY(m_controller->deleteOccurrence(*target));

    occurrences = m_controller->occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
    QCOMPARE(occurrences.size(), static_cast<std::size_t>(3));
    QCOMPARE(m_controller->calendar().size(), static_cast<std::size_t>(1)); // il ricorrente resta
}

void TestController::testDeleteOccurrenceTruncateExcludesFollowing() {
    m_controller->addActivity(makeActivity(ActivityConfig{
        .title = "Meeting",
        .start = tp(utc(2026, 1, 5, 9)),
        .duration = 1h,
        .end = tp(utc(2026, 2, 1)),
        .generator = std::make_shared<FixedIntervalGenerator>(std::chrono::days(7))}));
    auto occurrences = m_controller->occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
    QCOMPARE(occurrences.size(), static_cast<std::size_t>(4));

    const Occurrence* target = findByStart(occurrences, tp(utc(2026, 1, 19, 9)));
    QVERIFY(target != nullptr);
    QVERIFY(m_controller->deleteOccurrence(*target, true));

    occurrences = m_controller->occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
    QCOMPARE(occurrences.size(), static_cast<std::size_t>(2)); // 5/1 e 12/1
}

void TestController::testDeleteOccurrenceSingleRemovesActivity() {
    m_controller->addActivity(makeActivity(ActivityConfig{
        .title = "Dentista", .start = tp(utc(2026, 1, 8, 10)), .duration = 1h}));
    auto occurrences = m_controller->occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
    QCOMPARE(occurrences.size(), static_cast<std::size_t>(1));

    QVERIFY(m_controller->deleteOccurrence(occurrences[0]));
    QVERIFY(m_controller->calendar().empty());
}

void TestController::testModifyOccurrenceCreatesExceptionAndSingleEvent() {
    m_controller->addActivity(makeActivity(ActivityConfig{
        .title = "Meeting",
        .start = tp(utc(2026, 1, 5, 9)),
        .duration = 1h,
        .end = tp(utc(2026, 2, 1)),
        .generator = std::make_shared<FixedIntervalGenerator>(std::chrono::days(7))}));
    auto occurrences = m_controller->occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));

    const Occurrence* target = findByStart(occurrences, tp(utc(2026, 1, 12, 9)));
    QVERIFY(target != nullptr);
    auto replacement = makeActivity(ActivityConfig{
        .title = "Meeting (posticipato)", .start = tp(utc(2026, 1, 12, 11)), .duration = 1h});
    QVERIFY(m_controller->modifyOccurrence(*target, std::move(replacement)));

    occurrences = m_controller->occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
    QCOMPARE(occurrences.size(), static_cast<std::size_t>(4)); // 3 del ricorrente + il nuovo singolo
    QCOMPARE(m_controller->calendar().size(), static_cast<std::size_t>(2));
}

void TestController::testUpdateActivityPreservesExceptions() {
    m_controller->addActivity(makeActivity(ActivityConfig{
        .title = "Meeting",
        .start = tp(utc(2026, 1, 5, 9)),
        .duration = 1h,
        .end = tp(utc(2026, 2, 1)),
        .generator = std::make_shared<FixedIntervalGenerator>(std::chrono::days(7))}));

    const Activity* original = m_controller->search("Meeting")[0];
    auto occurrences = m_controller->occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
    const Occurrence* target = findByStart(occurrences, tp(utc(2026, 1, 12, 9)));
    QVERIFY(target != nullptr);
    m_controller->deleteOccurrence(*target); // aggiunge un'eccezione

    // modifica la regola (titolo e durata cambiano)
    auto updated = makeActivity(ActivityConfig{
        .title = "Meeting (aggiornato)",
        .start = tp(utc(2026, 1, 5, 9)),
        .duration = 2h,
        .end = tp(utc(2026, 2, 1)),
        .generator = std::make_shared<FixedIntervalGenerator>(std::chrono::days(7))});
    QVERIFY(m_controller->updateActivity(original, std::move(updated)));

    occurrences = m_controller->occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
    QCOMPARE(occurrences.size(), static_cast<std::size_t>(3)); // l'eccezione e' sopravvissuta
    QCOMPARE(occurrences[0].duration, 2h);
}

void TestController::testMoveSingleActivity() {
    m_controller->addActivity(makeActivity(ActivityConfig{
        .title = "Dentista", .start = tp(utc(2026, 1, 8, 10)), .duration = 1h}));
    const events::Activity* activity = m_controller->search("Dentista")[0];

    const TimePoint newStart = tp(utc(2026, 1, 9, 15));
    QVERIFY(m_controller->moveActivity(activity, utc(2026, 1, 9, 15)));

    auto occurrences = m_controller->occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 31));
    QCOMPARE(occurrences.size(), static_cast<std::size_t>(1));
    QCOMPARE(occurrences[0].start, newStart);
    QCOMPARE(occurrences[0].duration, 1h);
    QCOMPARE(m_controller->search("Dentista")[0]->getStart(), newStart);
}

void TestController::testMoveRecurrentActivityKeepsSeriesIntactAndEnd() {
    m_controller->addActivity(makeActivity(ActivityConfig{
        .title = "Riunione",
        .start = tp(utc(2026, 1, 5, 9)),
        .duration = 1h,
        .end = tp(utc(2026, 2, 16)),
        .generator = std::make_shared<FixedIntervalGenerator>(std::chrono::days(7))}));
    auto occurrences = m_controller->occurrencesIn(utc(2026, 1, 1), utc(2026, 1, 31));
    const Occurrence* second = findByStart(occurrences, tp(utc(2026, 1, 12, 9)));
    QVERIFY(second != nullptr);
    m_controller->deleteOccurrence(*second); // EXDATE sul 12/1 (evento staccato)

    // Sposta la serie di una settimana AVANTI (fine originale 16/2 supera
    // ancora il nuovo inizio 12/1): la scadenza resta quella, non slitta.
    const events::Activity* activity = m_controller->search("Riunione")[0];
    QVERIFY(m_controller->moveActivity(activity, utc(2026, 1, 12, 9)));

    occurrences = m_controller->occurrencesIn(utc(2026, 1, 5), utc(2026, 2, 28));
    QCOMPARE(occurrences.size(), static_cast<std::size_t>(5));
    QCOMPARE(m_controller->search("Riunione")[0]->getStart(), tp(utc(2026, 1, 12, 9)));
    QVERIFY(m_controller->occurrencesIn(utc(2026, 2, 16), utc(2026, 2, 28)).empty());
}

void TestController::testMoveTaskChangesDueDate() {
    m_controller->addActivity(makeTask(TaskConfig(
        ActivityConfig{.title = "Consegna", .start = tp(utc(2026, 1, 15))},
        Priority::High)));
    const events::Activity* t = m_controller->search("Consegna")[0];
    QVERIFY(m_controller->moveActivity(t, utc(2026, 2, 1)));
    QCOMPARE(t->getStart(), tp(utc(2026, 2, 1)));
}

void TestController::testDragSingleOccurrenceLeavesGapInSeries() {
    // Serie giornaliera per una settimana (lun 5/1 09:00, 1h)
    m_controller->addActivity(makeActivity(ActivityConfig{
        .title = "Allenamento",
        .start = tp(utc(2026, 1, 5, 9)),
        .duration = 1h,
        .generator = std::make_shared<FixedIntervalGenerator>(std::chrono::hours(24))}));
    auto occurrences = m_controller->occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 12));
    QCOMPARE(occurrences.size(), static_cast<std::size_t>(7));

    // Sposta SOLO la seconda occorrenza (mar 6/1 09:00) alla destinazione
    const Occurrence* second = findByStart(occurrences, tp(utc(2026, 1, 6, 9)));
    QVERIFY(second != nullptr);
    auto replacement = makeActivity(ActivityConfig{
        .title = second->source->getTitle(),
        .start = tp(utc(2026, 1, 8, 15)),
        .duration = second->duration});
    QVERIFY(m_controller->modifyOccurrence(*second, std::move(replacement)));

    occurrences = m_controller->occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 12));

    // 1) BUCO IN ORIGINE: nessuna occorrenza della serie il 6/1 09:00
    bool serieSul6 = false;
    for (const auto& o : occurrences) {
        if (o.source == second->source && o.start == tp(utc(2026, 1, 6, 9))) {
            serieSul6 = true;
        }
    }
    QVERIFY(!serieSul6);

    // 2) L'evento singolo e' alla destinazione (8/1 15:00)
    bool singoloAllaDestinazione = false;
    for (const auto& o : occurrences) {
        if (o.source != second->source && o.start == tp(utc(2026, 1, 8, 15))) {
            singoloAllaDestinazione = true;
        }
    }
    QVERIFY(singoloAllaDestinazione);

    // 3) La serie continua negli altri giorni (6 occorrenze su 7)
    int serieCount = 0;
    for (const auto& o : occurrences) {
        if (o.source == second->source) ++serieCount;
    }
    QCOMPARE(serieCount, 6);
}

void TestController::testSplitRecurrenceFromThisMomentOn() {
    // Serie giornaliera 8:00-10:00 (2h) per una settimana, fine 12/1 00:00
    m_controller->addActivity(makeActivity(ActivityConfig{
        .title = "Lezione",
        .start = tp(utc(2026, 1, 5, 8)),
        .duration = 2h,
        .end = tp(utc(2026, 1, 12)),
        .generator = std::make_shared<FixedIntervalGenerator>(std::chrono::hours(24))}));

    auto occurrences = m_controller->occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 12));
    QCOMPARE(occurrences.size(), static_cast<std::size_t>(7)); // 5/1 .. 11/1 alle 08:00

    // Il giorno 4 (8/1 08:00) diventa 10:00-12:00: split con nuovo inizio
    const Occurrence* day4 = findByStart(occurrences, tp(utc(2026, 1, 8, 8)));
    QVERIFY(day4 != nullptr);
    QVERIFY(m_controller->splitRecurrence(*day4, utc(2026, 1, 8, 10)));

    occurrences = m_controller->occurrencesIn(utc(2026, 1, 5), utc(2026, 1, 12));

    // 1) la serie ATTUALE e' fermata prima del giorno 4: occorrenze 5/1..7/1
    int oldSeriesCount = 0;
    for (const auto& o : occurrences) {
        if (o.source == day4->source) ++oldSeriesCount;
    }
    QCOMPARE(oldSeriesCount, 3);

    // 2) la NUOVA serie inizia l'8/1 alle 10:00 e continua ogni giorno
    const events::Activity* nuova = nullptr;
    for (const auto& activity : m_controller->calendar()) {
        if (activity.get() != day4->source) nuova = activity.get();
    }
    QVERIFY(nuova != nullptr);
    QCOMPARE(nuova->getStart(), tp(utc(2026, 1, 8, 10)));

    int newSeriesCount = 0;
    for (const auto& o : occurrences) {
        if (o.source == nuova) {
            ++newSeriesCount;
            QCOMPARE(o.duration, 2h);
        }
    }
    QCOMPARE(newSeriesCount, 4);

    // 3) la data di scadenza e' rimasta INVARIATA (12/1 00:00)
    QVERIFY(m_controller->occurrencesIn(utc(2026, 1, 12), utc(2026, 1, 15)).empty());

    // 4) totale: 3 + 4 = 7 occorrenze, 2 attivita'
    QCOMPARE(occurrences.size(), static_cast<std::size_t>(7));
    QCOMPARE(m_controller->calendar().size(), static_cast<std::size_t>(2));
}

void TestController::testSaveAndLoadFile() {
    m_controller->addActivity(makeActivity(ActivityConfig{
        .title = "Dentista", .start = tp(utc(2026, 1, 8, 10)), .duration = 1h}));
    m_controller->addActivity(makeTask(TaskConfig(
        ActivityConfig{.title = "Consegna", .start = tp(utc(2026, 1, 15))},
        Priority::High)));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("cal.json");

    QString error;
    QVERIFY(m_controller->saveToFile(path, &error));

    app::CalendarController other;
    QVERIFY(other.loadFromFile(path, &error));
    QCOMPARE(other.calendar().size(), static_cast<std::size_t>(2));
    QCOMPARE(other.search("consegna").size(), static_cast<std::size_t>(1));

    QVERIFY(!other.loadFromFile("/percorso/inesistente.json", &error));
    QVERIFY(!error.isEmpty());
}

void TestController::testAllDayFixVerification() {
    QDate monday(2026, 8, 31);
    QCOMPARE(monday.dayOfWeek(), 1);

    // Simula il percorso di creazione CORRETTO: inizio a mezzanotte UTC.
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

    auto ev = makeActivity(ActivityConfig{
        .title = "AllDay", .start = startUtc, .duration = std::chrono::seconds(86400)});
    m_controller->addActivity(std::move(ev));

    // Query della settimana (UTC) a partire dal lunedi'
    const QDateTime weekFrom(monday, QTime(0, 0), QTimeZone(0));
    const QDateTime weekTo = QDateTime(monday.addDays(7), QTime(0, 0),
                                       QTimeZone(0))
                                 .addSecs(-1);
    const auto occs = m_controller->occurrencesIn(weekFrom, weekTo);
    QCOMPARE(occs.size(), static_cast<std::size_t>(1));

    // Display: per un evento "tutto il giorno" l'ora mostrata deve essere
    // 00:00 (salvato a mezzanotte UTC), non l'ora locale spostata (02:00).
    const QDateTime shownStart =
        app::activityDisplayTime(occs[0].source, occs[0].start);
    QCOMPARE(shownStart.toString(QStringLiteral("HH:mm")), QStringLiteral("00:00"));
    QCOMPARE(shownStart.date(), monday);

    // L'evento salvato a mezzanotte UTC deve essere riconosciuto come
    // "tutto il giorno" (2 mezzenotti consecutive nel suo intervallo).
    QVERIFY(app::coversFullDay(occs[0]));
    // Strip placement: il giorno (locale) in cui parte l'occorrenza deve
    // essere il lunedi' stesso (firstDay = 0), non la domenica precedente.
    QCOMPARE(monday.daysTo(toLocalDate(occs[0].start)), 0);
}

void TestController::testAllDayNormalEventIsNotAllDay() {
    // Evento breve (es. 10:00-11:00 locali): non deve finire nella striscia.
    const QDateTime startLocal(QDate(2026, 8, 31), QTime(10, 0));
    const TimePoint start =
        TimePoint(std::chrono::seconds(startLocal.toSecsSinceEpoch()));
    m_controller->addActivity(makeActivity(ActivityConfig{
        .title = "Riunione", .start = start, .duration = std::chrono::minutes(60)}));
    const QDateTime dayFrom(QDate(2026, 8, 31), QTime(0, 0), QTimeZone(0));
    const auto occs = m_controller->occurrencesIn(
        dayFrom, QDateTime(QDate(2026, 8, 31), QTime(23, 59), QTimeZone(0)));
    QCOMPARE(occs.size(), static_cast<std::size_t>(1));
    QVERIFY(!app::coversFullDay(occs[0]));
}

void TestController::testAllDayStripLayoutRows() {
    // Replica dell'algoritmo di layout della striscia in WeekView::ensureRects:
    // ogni evento all-day va sulla riga piu' alta libera in tutti i giorni che
    // copre; eventi su giorni diversi possono condividere la riga 0.
    const int kDayCount = 7;

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
    QCOMPARE(items[0].row, 0);
    QCOMPARE(items[1].row, 1);

    // Un evento che copre lunedi' e martedi' sfrutta la riga libera
    items.clear();
    dayRows.assign(kDayCount, {});
    place(0, 1);   // lun-mar -> riga 0
    place(0, 0);   // lun -> deve scendere alla riga 1
    QCOMPARE(items[0].row, 0);
    QCOMPARE(items[1].row, 1);

    // Due eventi su giorni diversi -> entrambi in riga 0
    items.clear();
    dayRows.assign(kDayCount, {});
    place(0, 0);   // lun
    place(2, 2);   // mer
    QCOMPARE(items[0].row, 0);
    QCOMPARE(items[1].row, 0);
}

QTEST_GUILESS_MAIN(TestController)
#include "test_controller.moc"
