#include <QTest>
#include <QObject>
#include <QJsonObject>
#include <chrono>

#include "persistence/JsonPersistence.h"
#include "events/builders/ActivityBuilder.h"
#include "events/domain/Task.h"
#include "events/generators/MonthlyGenerator.h"

using namespace events;
using namespace std::chrono_literals;

class TestPersistence : public QObject {
    Q_OBJECT

private slots:
    void testTaskRoundTripWithBuilder();
};

void TestPersistence::testTaskRoundTripWithBuilder() {
    constexpr auto due = std::chrono::sys_days{2026y / 12 / 1} + 12h;
    auto monthlyGen = std::make_shared<MonthlyGenerator>(1);

    auto task = TaskBuilder("Pagamento Affitto", due)
                    .addGenerator(monthlyGen)
                    .withPriority(Priority::High)
                    .build();

    auto taskPtr = dynamic_cast<Task*>(task.get());
    taskPtr->setDone(due, true);

    QJsonObject json = persistence::activityToJson(*task);

    QString error;
    auto restored = persistence::activityFromJson(json, &error);

    QVERIFY2(restored != nullptr, error.toUtf8().constData());
    QCOMPARE(QString::fromStdString(restored->getTitle()), QString("Pagamento Affitto"));

    auto restoredTask = dynamic_cast<Task*>(restored.get());
    QVERIFY(restoredTask != nullptr);
    QVERIFY(restoredTask->isDone(due));
    QCOMPARE(restoredTask->getPriority(), Priority::High);
}

QTEST_GUILESS_MAIN(TestPersistence)
#include "test_persistence.moc"