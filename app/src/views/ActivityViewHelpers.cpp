#include "views/ActivityViewHelpers.h"

#include <QDateTime>

#include "events/core/ActivityVisitor.h"
#include "events/core/DateGeneratorVisitor.h"
#include "events/domain/Anniversary.h"
#include "events/domain/Event.h"
#include "events/domain/Meeting.h"
#include "events/domain/RecurrentEvent.h"
#include "events/domain/Task.h"
#include "events/generators/MonthlyGenerator.h"

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
  void visit(const events::Task&) override {
    label = QObject::tr("Compito");
  }
  void visit(const events::Meeting&) override {
    label = QObject::tr("Riunione");
  }
  void visit(const events::Anniversary&) override {
    label = QObject::tr("Anniversario");
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

  void visit(const events::Task& task) override {
    summary = localDateTime(task.getDue());
    if (task.isDone()) {
      summary += QLatin1String(" (") + QObject::tr("evaso") + QLatin1Char(')');
    } else if (task.isOverdue(std::chrono::time_point_cast<events::Duration>(
                   events::Clock::now()))) {
      summary += QLatin1String(" (") + QObject::tr("scaduto") + QLatin1Char(')');
    }
  }

  void visit(const events::Meeting& meeting) override {
    summary = localDateTime(meeting.getStart()) + QLatin1String(", durata ") +
              durationLabel(meeting.getDuration());
    if (!meeting.getLocation().empty()) {
      summary += QLatin1String(", ") +
                 QString::fromStdString(meeting.getLocation());
    }
  }

void visit(const events::Anniversary& anniversary) override {
    summary = QObject::tr("ogni anno, dal %1")
                  .arg(QDateTime::fromSecsSinceEpoch(
                           anniversary.getStart().time_since_epoch().count())
                           .toString(QStringLiteral("dd/MM")));
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

// --- Visitor: righe "campo: valore" specifiche per tipo (per il dettaglio) ---
class FieldsVisitor : public events::ActivityVisitor {
public:
  QStringList fields;

  void visit(const events::Event& event) override {
    fields << QObject::tr("Inizio: %1").arg(localDateTime(event.getStart()))
           << QObject::tr("Fine: %1").arg(localDateTime(event.getEnd()))
           << QObject::tr("Durata: %1")
                  .arg(durationLabel(event.getDuration()));
  }

  void visit(const events::RecurrentEvent& event) override {
    fields << QObject::tr("Regola: %1")
                  .arg(recurrenceRuleLabel(event))
           << QObject::tr("Prima occorrenza: %1")
                  .arg(localDateTime(event.getTemplateEvent().getStart()))
           << QObject::tr("Durata: %1")
                  .arg(durationLabel(event.getTemplateEvent().getDuration()))
           << QObject::tr("Eccezioni: %1").arg(event.getExceptions().size());
  }

  void visit(const events::Task& task) override {
    fields << QObject::tr("Scadenza: %1").arg(localDateTime(task.getDue()))
           << QObject::tr("Priorita': %1")
                  .arg(QString::fromStdString(
                      events::Task::priorityLabel(task.getPriority())))
           << QObject::tr("Stato: %1")
                  .arg(task.isDone() ? QObject::tr("evaso")
                                     : QObject::tr("in corso"));
  }

  void visit(const events::Meeting& meeting) override {
    fields << QObject::tr("Inizio: %1").arg(localDateTime(meeting.getStart()))
           << QObject::tr("Fine: %1").arg(localDateTime(meeting.getEnd()))
           << QObject::tr("Durata: %1")
                  .arg(durationLabel(meeting.getDuration()))
           << QObject::tr("Luogo: %1")
                  .arg(QString::fromStdString(meeting.getLocation()))
           << QObject::tr("Partecipanti: %1")
                  .arg(static_cast<int>(meeting.attendeeCount()));
  }

  void visit(const events::Anniversary& anniversary) override {
    fields << QObject::tr("Data: %1")
                  .arg(QDateTime::fromSecsSinceEpoch(
                           anniversary.getStart().time_since_epoch().count())
                           .toString(QStringLiteral("dd/MM")));
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

QStringList fieldLines(const events::Activity& activity) {
  FieldsVisitor visitor;
  activity.accept(visitor);
  return visitor.fields;
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
