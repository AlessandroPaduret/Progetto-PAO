#include "views/dialog/ActivityViewHelpers.h"

#include <QDateTime>

#include "core/ActivityVisitor.h"
#include "core/DateGeneratorVisitor.h"
#include "domain/Meeting.h"
#include "domain/Task.h"
#include "generators/FixedIntervalGenerator.h"
#include "generators/MonthlyGenerator.h"
#include "generators/SingleGenerator.h"
#include "generators/YearlyGenerator.h"
#include "views/utils/ViewShared.h"

namespace app {
namespace ActivityViewHelpers {

namespace {

QString localDateTime(const events::Activity& activity,
                      const events::TimePoint tp) {
  return app::activityDisplayTime(&activity, tp)
      .toString(QStringLiteral("dd/MM/yyyy HH:mm"));
}

// --- Visitor: etichetta del tipo (solo per visualizzazione) -----------------
// Il tipo e' il dispatch dinamico (Activity/Task/Meeting); la ricorrenza si
// deduce dal generatore.
class TypeLabelVisitor : public events::ActivityVisitor {
public:
  QString label;

  void visit(const events::Activity& activity) override {
    label = isRecurrent(&activity) ? QObject::tr("Ricorrente") : QObject::tr("Evento");
  }
  void visit(const events::Task&) override {
    label = QObject::tr("Compito");
  }
  void visit(const events::Meeting&) override {
    label = QObject::tr("Riunione");
  }
};

// --- Visitor: regola di ricorrenza leggibile (per display) -------------------
class RuleVisitor : public events::DateGeneratorVisitor {
public:
  QString rule;

  void visit(const events::FixedIntervalGenerator& generator) override {
    const qint64 days = generator.getInterval().count() / 86400;
    if (generator.getInterval().count() % 86400 == 0 && days >= 1) {
      rule = QObject::tr("%1 giorno/i").arg(days);
    } else {
      rule = QObject::tr("%1 s").arg(generator.getInterval().count());
    }
  }

  void visit(const events::YearlyGenerator&) override {
    rule = QObject::tr("anno");
  }

  void visit(const events::MonthlyGenerator& generator) override {
    rule = QObject::tr("%1 mesi").arg(generator.getMonths());
  }

  void visit(const events::SingleGenerator&) override {
    rule = QObject::tr("una volta");
  }
};

// --- Visitor: riga descrittiva sintetica per tipo ----------------------------
class SummaryVisitor : public events::ActivityVisitor {
public:
  QString summary;

  void visit(const events::Activity& activity) override {
    if (isRecurrent(&activity)) {
      RuleVisitor rule;
      activity.getGenerator().accept(rule);
      summary = QObject::tr("ogni %1, dal %2")
                    .arg(rule.rule, localDateTime(activity, activity.getStart()));
      return;
    }
    summary = localDateTime(activity, activity.getStart()) +
              QLatin1String(", durata ") +
              durationLabel(activity.getDuration());
  }

  void visit(const events::Task& task) override {
    summary = localDateTime(task, task.getDue());
    if (task.isDone()) {
      summary += QLatin1String(" (") + QObject::tr("evaso") + QLatin1Char(')');
    } else if (task.isOverdue(task.getDue(), std::chrono::time_point_cast<events::Duration>(
                   events::Clock::now()))) {
      summary += QLatin1String(" (") + QObject::tr("scaduto") + QLatin1Char(')');
    }
  }

  void visit(const events::Meeting& meeting) override {
    summary = localDateTime(meeting, meeting.getStart()) +
              QLatin1String(", durata ") +
              durationLabel(meeting.getDuration());
    if (!meeting.getLocation().empty()) {
      summary += QLatin1String(", ") +
                 QString::fromStdString(meeting.getLocation());
    }
  }
};

} // namespace

QString typeLabel(const events::Activity& activity) {
  TypeLabelVisitor visitor;
  activity.accept(visitor);
  return visitor.label;
}

QString summaryLabel(const events::Activity& activity) {
  SummaryVisitor visitor;
  activity.accept(visitor);
  return visitor.summary;
}

QString recurrenceRuleLabel(const events::Activity& activity) {
  RuleVisitor visitor;
  activity.getGenerator().accept(visitor);
  return visitor.rule;
}

QString durationLabel(const events::Duration duration) {
  const qint64 minutes = duration.count() / 60;
  if (minutes < 60) {
    return QObject::tr("%1 min").arg(minutes);
  }
  if (minutes % 60 == 0) {
    return QObject::tr("%1 h").arg(minutes / 60);
  }
  return QObject::tr("%1 h %2 min").arg(minutes / 60).arg(minutes % 60);
}

} // namespace ActivityViewHelpers
} // namespace app
