#include <QTest>
#include <QObject>
#include <chrono>
#include <memory>

#include "core/Activity.h"
#include "domain/Task.h"
#include "domain/Meeting.h"
#include "builders/ActivityConfig.h"
#include "generators/FixedIntervalGenerator.h"
#include "generators/MonthlyGenerator.h"

using namespace events;
using namespace std::chrono_literals;

class TestModel : public QObject {
    Q_OBJECT

private slots:
    void testSingleActivityCreation();
    void testTaskDonePerOccurrence();
    void testMeetingAttendeesAndLocation();
    void testActivityExceptions();
};

void TestModel::testSingleActivityCreation() {
    constexpr auto t1 = std::chrono::sys_days{2026y / 9 / 10} + 10h;

    auto act = makeActivity(ActivityConfig{
        .title = "Esame PAO",
        .start = t1,
        .duration = 2h
    });

    QCOMPARE(act->getTitle(), std::string("Esame PAO"));
    QCOMPARE(act->getStart(), t1);
    QCOMPARE(act->getDuration(), 2h);

    auto occs = act->occurrencesIn(t1 - 1h, t1 + 5h);
    QCOMPARE(occs.size(), static_cast<std::size_t>(1));
}

void TestModel::testTaskDonePerOccurrence() {
    constexpr auto due = std::chrono::sys_days{2026y / 11 / 15} + 18h;

    // Sintassi pulita ed esplicita con i costruttori
    auto task = makeTask(TaskConfig{
        ActivityConfig{
            .title = "Consegna Progetto",
            .start = due
        },
        Priority::High
    });

    QVERIFY(task != nullptr);
    QCOMPARE(task->getPriority(), Priority::High);
}

void TestModel::testMeetingAttendeesAndLocation() {
    constexpr auto start = std::chrono::sys_days{2026y / 9 / 20} + 14h;

    auto meeting = makeMeeting(MeetingConfig{
        ActivityConfig{
            .title = "Sprint Planning",
            .start = start,
            .duration = 1h + 30min
        },
        "Aula 2C",
        {"Mario Rossi", "Giuseppe Verdi"}
    });

    QVERIFY(meeting != nullptr);
}

void TestModel::testActivityExceptions() {
    constexpr auto start = std::chrono::sys_days{2026y / 10 / 1} + 8h;
    constexpr auto exdate = std::chrono::sys_days{2026y / 10 / 2} + 8h;

    auto dailyGen = std::make_shared<FixedIntervalGenerator>(24h);

    auto act = makeActivity(ActivityConfig{
        .title = "Corso Mattutino",
        .start = start,
        .generator = dailyGen,
        .exceptions = {exdate}
    });

    auto occs = act->occurrencesIn(start, start + 48h);
    QCOMPARE(occs.size(), static_cast<std::size_t>(2));
    QCOMPARE(occs[0].start, start);
    QCOMPARE(occs[1].start, start + 48h);
}

QTEST_GUILESS_MAIN(TestModel)
#include "test.moc"