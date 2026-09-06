#include <QTest>
#include <QObject>
#include <chrono>
#include <memory>

#include "events/core/Activity.h"
#include "events/domain/Task.h"
#include "events/domain/Meeting.h"
#include "events/builders/ActivityBuilder.h"
#include "events/generators/FixedIntervalGenerator.h"
#include "events/generators/MonthlyGenerator.h"

using namespace events;
using namespace std::chrono_literals;

class TestModel : public QObject {
    Q_OBJECT

private slots:
    void testSingleActivityCreation();
    void testRecurrentActivityStopAfter();
    void testTaskDonePerOccurrence();
    void testMeetingAttendeesAndLocation();
    void testActivityExceptions();
};

void TestModel::testSingleActivityCreation() {
    // Data fissa senza usare now()
    constexpr auto t1 = std::chrono::sys_days{2026y / 9 / 10} + 10h; // 10 Settembre 2026, 10:00

    auto act = ActivityBuilder("Esame PAO", t1)
                   .withDuration(2h)
                   .build();

    QCOMPARE(act->getTitle(), std::string("Esame PAO"));
    QCOMPARE(act->getStart(), t1);
    QCOMPARE(act->getDuration(), 2h);

    auto occs = act->occurrencesIn(t1 - 1h, t1 + 5h);
    QCOMPARE(occs.size(), static_cast<std::size_t>(1));
}

void TestModel::testRecurrentActivityStopAfter() {
    constexpr auto start = std::chrono::sys_days{2026y / 10 / 1} + 9h;
    auto dailyGen = std::make_shared<FixedIntervalGenerator>(24h);

    // Costruzione fluente tramite Builder limitata a 3 occorrenze
    auto act = ActivityBuilder("Daily Standup", start)
                   .withDuration(15min)
                   .addGenerator(dailyGen)
                   .stopAfter(3)
                   .build();

    // Finestra ampia di 10 giorni
    auto occs = act->occurrencesIn(start, start + 240h);
    QCOMPARE(occs.size(), static_cast<std::size_t>(3));
}

void TestModel::testTaskDonePerOccurrence() {
    constexpr auto due = std::chrono::sys_days{2026y / 11 / 15} + 18h;

    auto task = TaskBuilder("Consegna Progetto", due)
                    .withPriority(Priority::High)
                    .build();

    auto taskPtr = dynamic_cast<Task*>(task.get());
    QVERIFY(taskPtr != nullptr);
    QCOMPARE(taskPtr->getPriority(), Priority::High);

    // Verifica completamento per-occorrenza
    QVERIFY(!taskPtr->isDone(due));
    taskPtr->setDone(due, true);
    QVERIFY(taskPtr->isDone(due));
}

void TestModel::testMeetingAttendeesAndLocation() {
    constexpr auto start = std::chrono::sys_days{2026y / 9 / 20} + 14h;

    auto meeting = MeetingBuilder("Sprint Planning", start)
                       .withDuration(1h + 30min)
                       .withLocation("Aula 2C")
                       .addAttendee("Mario Rossi")
                       .addAttendee("Giuseppe Verdi")
                       .build();

    auto meetingPtr = dynamic_cast<Meeting*>(meeting.get());
    QVERIFY(meetingPtr != nullptr);
    QCOMPARE(QString::fromStdString(meetingPtr->getLocation()), QString("Aula 2C"));
    QCOMPARE(meetingPtr->attendeeCount(), static_cast<std::size_t>(2));

    // Duplicati rifiutati
    QVERIFY(!meetingPtr->addAttendee("Mario Rossi"));
}

void TestModel::testActivityExceptions() {
    constexpr auto start = std::chrono::sys_days{2026y / 10 / 1} + 8h;
    constexpr auto exdate = std::chrono::sys_days{2026y / 10 / 2} + 8h;

    auto dailyGen = std::make_shared<FixedIntervalGenerator>(24h);

    auto act = ActivityBuilder("Corso Mattutino", start)
                   .addGenerator(dailyGen)
                   .addException(exdate)
                   .build();

    // Finestra di 3 giorni (ottobre 1, 2, 3). L'ottobre 2 e' escluso.
    auto occs = act->occurrencesIn(start, start + 48h);
    QCOMPARE(occs.size(), static_cast<std::size_t>(2));
    QCOMPARE(occs[0].start, start);
    QCOMPARE(occs[1].start, start + 48h);
}

QTEST_GUILESS_MAIN(TestModel)
#include "test.moc"