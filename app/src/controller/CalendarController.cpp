#include "controller/CalendarController.h"

#include <QDateTime>
#include <QTimeZone>

#include <memory>
#include <unordered_set>
#include <utility>

#include "persistence/JsonPersistence.h"
#include "generators/FixedIntervalGenerator.h"
#include "generators/MonthlyGenerator.h"
#include "generators/YearlyGenerator.h"

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

  // copiate PRIMA di rimuovere, che distrugge l'oggetto
  const auto exceptions = oldActivity->getExceptions();

  if (!m_calendar.remove(oldActivity)) {
    return false;
  }

  // accettate solo se la data e' generabile dal nuovo generatore (addException valida via isIn)
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

  events::Activity* foundActivity = m_calendar.find(activity);

  if (!foundActivity) {
    return false;
  }

  foundActivity->setStart(toTimePoint(newStart));
  foundActivity->clearExceptions();

  emit activitiesChanged();
  return true;
}

bool CalendarController::splitRecurrence(const events::Occurrence& occurrence,
                                         const QDateTime& newStart) {
  if (!occurrence.source || !newStart.isValid()) {
    return false;
  }

  events::Activity* foundActivity = m_calendar.find(occurrence.source);
  if (!foundActivity) {
    return false;
  }

  const events::TimePoint target = toTimePoint(newStart);
  const events::TimePoint originalEnd = foundActivity->getEnd();

  // clona la serie originale: quella vecchia si tronca qui, la nuova riparte da target
  auto newActivity = foundActivity->clone();

  foundActivity->setEnd(occurrence.start);
  foundActivity->addException(occurrence.start);
  cleanupActivity(foundActivity);

  newActivity->setStart(target);
  newActivity->setEnd(originalEnd);

  m_calendar.add(std::move(newActivity));

  emit activitiesChanged();
  return true;
}

bool CalendarController::deleteOccurrence(const events::Occurrence& occurrence,
                                          bool andFollowing) {
  events::Activity* foundActivity = m_calendar.find(occurrence.source);
  if (!foundActivity) {
    return false;
  }

  if ( andFollowing ) foundActivity->setEnd(occurrence.start);

  foundActivity->addException(occurrence.start);

  cleanupActivity(foundActivity);

  emit activitiesChanged();
  return true;
}

bool CalendarController::modifyOccurrence(
    const events::Occurrence& occurrence,
    std::unique_ptr<events::Activity> replacement) {
  if (!replacement) {
    return false;
  }

  events::Activity* foundActivity = m_calendar.find(occurrence.source);

  if (!foundActivity) return false;


  foundActivity->addException(occurrence.start);

  cleanupActivity(foundActivity);
  
  m_calendar.add(std::move(replacement));
  emit activitiesChanged();
  return true;
}

bool CalendarController::toggleDone(const events::Occurrence& occurrence) {

  events::Activity* foundActivity = m_calendar.find(occurrence.source);
  if (!foundActivity) {
    return false;
  }

  if (auto* task = dynamic_cast<events::Task*>(foundActivity)) {
    // per-occorrenza: un Compito ricorrente evade indipendentemente per ciascuna
    // occorrenza, non con un unico stato per l'intera serie
    task->setDone(occurrence.start, !task->isDone(occurrence.start));
    emit activitiesChanged();
    return true;
  }

  return false;
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

void CalendarController::cleanupActivity(const events::Activity* activity){
  if(activity->getEnd() != events::TimePoint::max() && activity->occurrencesIn(activity->getStart(),activity->getEnd()).size() < 1)
    m_calendar.remove(activity);
}

} // namespace app
