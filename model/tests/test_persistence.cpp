#include <QTest>
#include <QObject>
#include <QJsonObject>
#include <chrono>

#include "persistence/JsonPersistence.h"
#include "events/builders/ActivityConfig.h" // Sostituito ActivityBuilder con ActivityConfig
#include "events/domain/Task.h"
#include "events/generators/MonthlyGenerator.h"

using namespace events;
using namespace std::chrono_literals;

class TestPersistence : public QObject {
    Q_OBJECT

private slots:
    void testTaskRoundTripWithConfig(); // Rinominato il test per coerenza
};

void TestPersistence::testTaskRoundTripWithConfig() {
    constexpr auto due = std::chrono::sys_days{2026y / 12 / 1} + 12h;
    auto monthlyGen = std::make_shared<MonthlyGenerator>(1);

    // Creazione del Task tramite makeTask e TaskConfig
    auto task = makeTask(TaskConfig{
        ActivityConfig{
            .title = "Pagamento Affitto",
            .start = due,
            .generator = monthlyGen
        },
        Priority::High
    });

    task->setDone(due, true);

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