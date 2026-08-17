#include "CalendarController.h"

#include <QDateTime>
#include <QTimeZone>

#include <memory>
#include <unordered_set>
#include <utility>

#include "persistence/JsonPersistence.h"

namespace app {

namespace {

events::TimePoint toTimePoint(const QDateTime& utc) {
  return events::TimePoint(std::chrono::seconds(utc.toSecsSinceEpoch()));
}

} // namespace

CalendarController::CalendarController(QObject* parent) : QObject(parent) {}

const events::Calendar &CalendarController::calendar() const {
  return m_calendar;
}

bool CalendarController::addActivity(std::unique_ptr<events::Activity> activity) {
  if (!activity) {
    return false;
  }
  m_calendar.add(std::move(activity));
  emit activitiesChanged();
  return true;
}

bool CalendarController::removeActivity(const events::Activity* activity) {
  if (!m_calendar.remove(activity)) {
    return false;
  }
  emit activitiesChanged();
  return true;
}

bool CalendarController::updateActivity(
    const events::Activity* oldActivity,
    std::unique_ptr<events::Activity> replacement) {
  if (!replacement) {
    return false;
  }

  // Le eccezioni vanno copiate PRIMA di rimuovere (che distrugge l'oggetto):
  // le eccezioni si gestiscono solo dalla vista settimanale.
  std::unordered_set<events::TimePoint, events::TimePointHasher> exceptions;
  if (const auto* oldRecurrent =
          dynamic_cast<const events::RecurrentEvent*>(oldActivity)) {
    exceptions = oldRecurrent->getExceptions();
  }

  if (!m_calendar.remove(oldActivity)) {
    return false;
  }

  if (auto* newRecurrent =
          dynamic_cast<events::RecurrentEvent*>(replacement.get())) {
    for (const auto& exception : exceptions) {
      newRecurrent->addException(exception);
    }
  }

  m_calendar.add(std::move(replacement));
  emit activitiesChanged();
  return true;
}

QVector<const events::Activity*> CalendarController::search(const QString& needle) const {
  const std::vector<const events::Activity*> matches =
      m_calendar.search(needle.toStdString());
  return QVector<const events::Activity*>(matches.begin(), matches.end());
}

std::vector<events::Occurrence> CalendarController::occurrencesIn(
    const QDateTime& fromUtc, const QDateTime& toUtc) const {
  return m_calendar.occurrencesIn(toTimePoint(fromUtc), toTimePoint(toUtc));
}

bool CalendarController::moveActivity(const events::Activity* activity,
                                      const QDateTime& newStart) {
  if (!activity || !newStart.isValid()) {
    return false;
  }
  const_cast<events::Activity*>(activity)->moveTo(toTimePoint(newStart));
  emit activitiesChanged();
  return true;
}

namespace {

// Clona la regola di ricorrenza con un nuovo inizio, mantenendo la fine
// ORIGINALE (invariata, salvata prima del troncamento). Se la fine
// resterebbe antecedente al nuovo inizio, viene portata al nuovo inizio.
class ReseedGeneratorVisitor : public events::DateGeneratorVisitor {
public:
  events::TimePoint newStart;
  events::TimePoint end = events::TimePoint::max();

  void visit(const events::FixedIntervalGenerator& generator) override {
    events::TimePoint safeEnd = end;
    if (safeEnd < newStart) {
      safeEnd = newStart;
    }
    result = std::make_shared<events::FixedIntervalGenerator>(
        newStart, generator.getInterval(), safeEnd);
  }

  void visit(const events::YearlyGenerator&) override {
    events::TimePoint safeEnd = end;
    if (safeEnd < newStart) {
      safeEnd = newStart;
    }
    result = std::make_shared<events::YearlyGenerator>(newStart, safeEnd);
  }

  void visit(const events::NullGenerator&) override {
    result = std::make_shared<events::NullGenerator>();
  }

  std::shared_ptr<events::DateGenerator> result;
};

} // namespace

bool CalendarController::splitRecurrence(const events::Occurrence& occurrence,
                                         const QDateTime& newStart) {
  auto* series = dynamic_cast<events::RecurrentEvent*>(
      const_cast<events::Activity*>(occurrence.source));
  if (!series || !newStart.isValid()) {
    return false;
  }

  // 0) La data di scadenza ORIGINALE va salvata PRIMA del troncamento
  //    (truncateBefore la ridurrebbe a questa occorrenza)
  ReseedGeneratorVisitor reseed;
  reseed.newStart = toTimePoint(newStart);
  reseed.end = series->getGenerator()->getEnd();

  // 1) La serie attuale viene FERMATA prima dell'occorrenza interessata
  series->truncateBefore(occurrence.start);

  // 2) Nasce una nuova serie con le stesse regole di ricorrenza (tipo e
  //    intervallo del generatore, durata dell'occorrenza) ma inizio diverso;
  //    la data di scadenza rimane quella originale.
  series->getGenerator()->accept(reseed);

  auto replacement = std::make_unique<events::RecurrentEvent>(
      reseed.result,
      events::Event(series->getTitle(), reseed.newStart, occurrence.duration));
  m_calendar.add(std::move(replacement));
  emit activitiesChanged();
  return true;
}

bool CalendarController::deleteOccurrence(const events::Occurrence& occurrence,
                                          bool andFollowing) {
  auto* activity = const_cast<events::Activity*>(occurrence.source);
  auto* recurrent = dynamic_cast<events::RecurrentEvent*>(activity);
  if (recurrent && !andFollowing) {
    recurrent->addException(occurrence.start);  // EXDATE: solo questa
  } else if (recurrent) {
    recurrent->truncateBefore(occurrence.start);  // questa e le successive
  } else {
    m_calendar.remove(occurrence.source);
  }
  emit activitiesChanged();
  return true;
}

bool CalendarController::modifyOccurrence(
    const events::Occurrence& occurrence,
    std::unique_ptr<events::Event> replacement) {
  if (!replacement) {
    return false;
  }
  auto* activity = const_cast<events::Activity*>(occurrence.source);
  if (auto* recurrent = dynamic_cast<events::RecurrentEvent*>(activity)) {
    recurrent->addException(occurrence.start);  // scarta l'istanza originale
  } else {
    m_calendar.remove(occurrence.source);
  }
  m_calendar.add(std::move(replacement));
  emit activitiesChanged();
  return true;
}

bool CalendarController::saveToFile(const QString& filePath, QString* error) {
  return persistence::saveToFile(m_calendar, filePath, error);
}

bool CalendarController::loadFromFile(const QString& filePath, QString* error) {
  if (!persistence::loadFromFile(m_calendar, filePath, error)) {
    return false;
  }
  emit activitiesChanged();
  return true;
}

} // namespace app
