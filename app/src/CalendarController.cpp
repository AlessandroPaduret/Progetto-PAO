#include "CalendarController.h"

#include <QDateTime>
#include <QTimeZone>

#include <memory>
#include <unordered_set>
#include <utility>

#include "persistence/JsonPersistence.h"
#include "events/generators/FixedIntervalGenerator.h"
#include "events/generators/MonthlyGenerator.h"
#include "events/generators/YearlyGenerator.h"

namespace app {

namespace {

events::TimePoint toTimePoint(const QDateTime& utc) {
  return events::TimePoint(std::chrono::seconds(utc.toSecsSinceEpoch()));
}

// La ricorrenza si deduce dal generatore: un'attivita' e' una serie se il suo
// generatore produce piu' date (Fixed/Monthly/Yearly), non Single.
bool isRecurrentActivity(const events::Activity* activity) {
  const events::DateGenerator* gen = &activity->getGenerator();
  return dynamic_cast<const events::FixedIntervalGenerator*>(gen) != nullptr ||
         dynamic_cast<const events::MonthlyGenerator*>(gen) != nullptr ||
         dynamic_cast<const events::YearlyGenerator*>(gen) != nullptr;
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

bool CalendarController::addActivities(
    std::vector<std::unique_ptr<events::Activity>> activities) {
  if (activities.empty()) {
    return false;
  }
  for (auto& activity : activities) {
    if (!activity) {
      return false;
    }
    m_calendar.add(std::move(activity));
  }
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
  const auto exceptions = oldActivity->getExceptions();

  if (!m_calendar.remove(oldActivity)) {
    return false;
  }

  // Le eccezioni sono accettate solo se la data e' generabile dal nuovo
  // generatore (Activity::addException valida via isIn).
  for (const auto& exception : exceptions) {
    replacement->addException(exception);
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

bool CalendarController::splitRecurrence(const events::Occurrence& occurrence,
                                         const QDateTime& newStart) {
  auto* series = const_cast<events::Activity*>(occurrence.source);
  if (!isRecurrentActivity(series) || !newStart.isValid()) {
    return false;
  }

  // 0) La data di scadenza ORIGINALE va salvata PRIMA del troncamento
  //    (truncateBefore la ridurrebbe a questa occorrenza)
  const events::TimePoint originalEnd = series->getGenerator().getEnd();
  const events::TimePoint target = toTimePoint(newStart);

  // 1) La serie attuale viene FERMATA prima dell'occorrenza interessata
  series->truncateBefore(occurrence.start);

  // 2) Nasce una nuova serie con le stesse regole di ricorrenza (tipo e
  //    intervallo del generatore, durata dell'occorrenza) ma inizio diverso;
  //    la data di scadenza rimane quella originale. Il generatore e' mutabile:
  //    si clona e si regolano start/end con i setter.
  auto replacement = std::make_unique<events::Activity>(
      series->getTitle(), series->getDuration(), series->getGenerator().clone());
  replacement->getGenerator().setStart(target);
  replacement->getGenerator().setEnd(originalEnd);
  m_calendar.add(std::move(replacement));
  emit activitiesChanged();
  return true;
}

bool CalendarController::deleteOccurrence(const events::Occurrence& occurrence,
                                          bool andFollowing) {
  auto* activity = const_cast<events::Activity*>(occurrence.source);
  if (isRecurrentActivity(activity) && !andFollowing) {
    activity->addException(occurrence.start);  // EXDATE: solo questa
  } else if (isRecurrentActivity(activity)) {
    activity->truncateBefore(occurrence.start);  // questa e le successive
  } else {
    m_calendar.remove(occurrence.source);
  }
  emit activitiesChanged();
  return true;
}

bool CalendarController::modifyOccurrence(
    const events::Occurrence& occurrence,
    std::unique_ptr<events::Activity> replacement) {
  if (!replacement) {
    return false;
  }
  auto* activity = const_cast<events::Activity*>(occurrence.source);
  if (isRecurrentActivity(activity)) {
    activity->addException(occurrence.start);  // scarta l'istanza originale
  } else {
    m_calendar.remove(occurrence.source);
  }
  m_calendar.add(std::move(replacement));
  emit activitiesChanged();
  return true;
}

bool CalendarController::toggleDone(const events::Occurrence& occurrence) {
  auto* task =
      dynamic_cast<events::Task*>(const_cast<events::Activity*>(occurrence.source));
  if (!task) {
    return false;
  }
  task->setDone(!task->isDone());
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
