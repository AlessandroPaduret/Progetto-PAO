#include "views/ActivityViewHelpers.h"

#include <QDateTime>

#include "events/core/ActivityVisitor.h"
#include "events/core/DateGeneratorVisitor.h"
#include "events/domain/Deadline.h"
#include "events/domain/Event.h"
#include "events/domain/RecurrentEvent.h"
#include "events/domain/Reminder.h"

namespace app {
namespace ActivityViewHelpers {

namespace {

QString localDateTime(const events::TimePoint tp) {
  return QDateTime::fromSecsSinceEpoch(tp.time_since_epoch().count())
      .toString(QStringLiteral("dd/MM/yyyy HH:mm"));
}

// --- Visitor: etichetta del tipo (solo per visualizzazione) -----------------
class TypeLabelVisitor : public events::ActivityVisitor {
public:
  QString label;

  void visit(const events::Event&) override {
    label = QObject::tr("Evento");
  }
  void visit(const events::RecurrentEvent&) override {
    label = QObject::tr("Ricorrente");
  }
  void visit(const events::Deadline&) override {
    label = QObject::tr("Scadenza");
  }
  void visit(const events::Reminder&) override {
    label = QObject::tr("Promemoria");
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

  void visit(const events::NullGenerator&) override {
    rule = QObject::tr("mai");
  }
};

// --- Visitor: riga descrittiva sintetica per tipo ----------------------------
class SummaryVisitor : public events::ActivityVisitor {
public:
  QString summary;

  void visit(const events::Event& event) override {
    summary = localDateTime(event.getStart()) + QLatin1String(", durata ") +
              durationLabel(event.getDuration());
  }

  void visit(const events::RecurrentEvent& event) override {
    RuleVisitor rule;
    event.getGenerator()->accept(rule);
    summary = QObject::tr("ogni %1, dal %2")
                  .arg(rule.rule, localDateTime(event.getStart()));
  }

  void visit(const events::Deadline& deadline) override {
    summary = localDateTime(deadline.getDue());
    if (deadline.isDone()) {
      summary += QLatin1String(" (") + QObject::tr("evasa") + QLatin1Char(')');
    } else if (deadline.isOverdue(std::chrono::time_point_cast<events::Duration>(
                   events::Clock::now()))) {
      summary += QLatin1String(" (") + QObject::tr("scaduta") + QLatin1Char(')');
    }
  }

  void visit(const events::Reminder& reminder) override {
    summary = localDateTime(reminder.getTrigger());
    if (reminder.isRepeating()) {
      summary += QLatin1String(", ") +
                 QObject::tr("ogni %1").arg(durationLabel(
                     std::chrono::seconds(reminder.getRepeatInterval().count())));
    }
  }

private:
  static QString durationLabel(const events::Duration duration) {
    const qint64 minutes = duration.count() / 60;
    if (minutes < 60) {
      return QObject::tr("%1 min").arg(minutes);
    }
    if (minutes % 60 == 0) {
      return QObject::tr("%1 h").arg(minutes / 60);
    }
    return QObject::tr("%1 h %2 min").arg(minutes / 60).arg(minutes % 60);
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

QString recurrenceRuleLabel(const events::RecurrentEvent& event) {
  RuleVisitor visitor;
  event.getGenerator()->accept(visitor);
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
