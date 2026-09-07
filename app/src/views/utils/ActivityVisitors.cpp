#include "views/utils/ActivityVisitors.h"

#include <QDateTime>

#include "domain/Meeting.h"
#include "domain/Task.h"
#include "generators/FixedIntervalGenerator.h"
#include "generators/MonthlyGenerator.h"
#include "generators/SingleGenerator.h"
#include "generators/YearlyGenerator.h"
#include "views/utils/ActivityViewHelpers.h"
#include "views/utils/ViewShared.h"

namespace app {

namespace {

QString localDateTime(const events::Activity& activity, const events::TimePoint tp) {
    return app::activityDisplayTime(&activity, tp).toString(QStringLiteral("dd/MM/yyyy HH:mm"));
}

} // namespace

void TypeLabelVisitor::visit(const events::Activity& activity) {
    label = isRecurrent(&activity) ? QObject::tr("Ricorrente") : QObject::tr("Evento");
}

void TypeLabelVisitor::visit(const events::Task&) {
    label = QObject::tr("Compito");
}

void TypeLabelVisitor::visit(const events::Meeting&) {
    label = QObject::tr("Riunione");
}

void RecurrenceRuleVisitor::visit(const events::FixedIntervalGenerator& generator) {
    const qint64 days = generator.getInterval().count() / 86400;
    if (generator.getInterval().count() % 86400 == 0 && days >= 1) {
        rule = QObject::tr("%1 giorno/i").arg(days);
    } else {
        rule = QObject::tr("%1 s").arg(generator.getInterval().count());
    }
}

void RecurrenceRuleVisitor::visit(const events::YearlyGenerator&) {
    rule = QObject::tr("anno");
}

void RecurrenceRuleVisitor::visit(const events::MonthlyGenerator& generator) {
    rule = QObject::tr("%1 mesi").arg(generator.getMonths());
}

void RecurrenceRuleVisitor::visit(const events::SingleGenerator&) {
    rule = QObject::tr("una volta");
}

void ActivitySummaryVisitor::visit(const events::Activity& activity) {
    if (isRecurrent(&activity)) {
        RecurrenceRuleVisitor rule;
        activity.getGenerator().accept(rule);
        summary = QObject::tr("ogni %1, dal %2")
                      .arg(rule.rule, localDateTime(activity, activity.getStart()));
        return;
    }
    summary = localDateTime(activity, activity.getStart()) + QLatin1String(", durata ") +
              ActivityViewHelpers::durationLabel(activity.getDuration());
}

void ActivitySummaryVisitor::visit(const events::Task& task) {
    summary = localDateTime(task, task.getDue());
    if (task.isDone()) {
        summary += QLatin1String(" (") + QObject::tr("evaso") + QLatin1Char(')');
    } else if (task.isOverdue(task.getDue(), std::chrono::time_point_cast<events::Duration>(
                                                  events::Clock::now()))) {
        summary += QLatin1String(" (") + QObject::tr("scaduto") + QLatin1Char(')');
    }
}

void ActivitySummaryVisitor::visit(const events::Meeting& meeting) {
    summary = localDateTime(meeting, meeting.getStart()) + QLatin1String(", durata ") +
              ActivityViewHelpers::durationLabel(meeting.getDuration());
    if (!meeting.getLocation().empty()) {
        summary += QLatin1String(", ") + QString::fromStdString(meeting.getLocation());
    }
}

} // namespace app
