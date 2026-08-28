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
    return false; // Parametri invalidi
  }

  events::Activity* foundActivity = m_calendar.find(activity);

  if (!foundActivity) {
    return false; // L'attività non è presente nel calendario
  }
  
  foundActivity->setStart(toTimePoint(newStart));
  foundActivity->clearExceptions();

  emit activitiesChanged();
  return true;
}

bool CalendarController::splitRecurrence(const events::Occurrence& occurrence,
                                         const QDateTime& newStart) {
  if (!occurrence.source || !isRecurrentActivity(occurrence.source) || !newStart.isValid()) {
    return false;
  }

  events::Activity* foundActivity = m_calendar.find(occurrence.source);
  if (!foundActivity) {
    return false; // L'attività non è presente nel calendario
  }

  const events::TimePoint target = toTimePoint(newStart);
  const events::TimePoint originalEnd = foundActivity->getEnd();

  // 2. Clona l'attività originale per creare la nuova serie (restituisce std::unique_ptr)
  auto newActivity = foundActivity->clone();

  // 3. Modifica la prima serie direttamente in-place (è ancora dentro m_calendar!)
  foundActivity->setEnd(occurrence.start - events::Duration(1));

  // 4. Configura la nuova serie clonata
  newActivity->setStart(target);
  newActivity->setEnd(originalEnd);

  // 5. Inserisce la seconda serie nel calendario (trasferendone l'ownership)
  m_calendar.add(std::move(newActivity));

  emit activitiesChanged();
  return true;
}

bool CalendarController::deleteOccurrence(const events::Occurrence& occurrence,
                                          bool andFollowing) {
  events::Activity* foundActivity = m_calendar.find(occurrence.source);
  if (!foundActivity) {
    return false; // L'attività non è presente nel calendario
  }

  if (isRecurrentActivity(occurrence.source) && !andFollowing) {
    foundActivity->addException(occurrence.start);  // EXDATE: solo questa
  } else if (isRecurrentActivity(occurrence.source)) {
    foundActivity->setEnd(occurrence.start - events::Duration(1));  // questa e le successive
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

  events::Activity* foundActivity = m_calendar.find(occurrence.source);
  
  if (!foundActivity) return false; // L'attività non è presente nel calendario
  

  if (isRecurrentActivity(foundActivity)) {
    foundActivity->addException(occurrence.start);  // Togli l'occorrenza dall'attività
  } else {
    m_calendar.remove(foundActivity);
  }
  m_calendar.add(std::move(replacement));
  emit activitiesChanged();
  return true;
}

bool CalendarController::toggleDone(const events::Occurrence& occurrence) {

  events::Activity* foundActivity = m_calendar.find(occurrence.source);
  if (!foundActivity) {
    return false; // L'attività non è presente nel calendario
  }

  if (auto* task = dynamic_cast<events::Task*>(foundActivity)) {
    task->setDone(!task->isDone());
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

} // namespace app
