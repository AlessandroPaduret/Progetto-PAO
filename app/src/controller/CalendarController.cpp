#include "controller/CalendarController.h"

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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

bool CalendarController::addActivity(std::unique_ptr<events::Activity> activity,
                                     const QString& color) {
  if (!activity) {
    return false;
  }
  events::Activity& added = m_calendar.add(std::move(activity));
  if (!color.isEmpty()) {
    m_colors[&added] = color;
  }
  emit activitiesChanged();
  return true;
}

bool CalendarController::addActivities(
    std::vector<std::unique_ptr<events::Activity>> activities, const QString& color) {
  if (activities.empty()) {
    return false;
  }
  for (auto& activity : activities) {
    if (!activity) {
      return false;
    }
    events::Activity& added = m_calendar.add(std::move(activity));
    if (!color.isEmpty()) {
      m_colors[&added] = color;
    }
  }
  emit activitiesChanged();
  return true;
}

bool CalendarController::removeActivity(const events::Activity* activity) {
  if (!m_calendar.remove(activity)) {
    return false;
  }
  m_colors.erase(activity);
  emit activitiesChanged();
  return true;
}

QString CalendarController::colorFor(const events::Activity* activity) const {
  const auto it = m_colors.find(activity);
  return it != m_colors.end() ? it->second : QString();
}

bool CalendarController::updateActivity(
    const events::Activity* oldActivity,
    std::unique_ptr<events::Activity> replacement,
    const QString& color) {
  if (!replacement) {
    return false;
  }

  // Le eccezioni vanno copiate PRIMA di rimuovere (che distrugge l'oggetto):
  // le eccezioni si gestiscono solo dalla vista settimanale.
  const auto exceptions = oldActivity->getExceptions();

  if (!m_calendar.remove(oldActivity)) {
    return false;
  }
  // Il vecchio puntatore non e' piu' valido come chiave: il colore per la
  // versione sostituita si ristabilisce subito sotto sul NUOVO puntatore.
  m_colors.erase(oldActivity);

  // Le eccezioni sono accettate solo se la data e' generabile dal nuovo
  // generatore (Activity::addException valida via isIn).
  for (const auto& exception : exceptions) {
    replacement->addException(exception);
  }

  events::Activity& added = m_calendar.add(std::move(replacement));
  if (!color.isEmpty()) {
    m_colors[&added] = color;
  }
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
  if (!occurrence.source || !newStart.isValid()) {
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
  // Il colore (se scelto dall'utente) va catturato PRIMA di cleanupActivity:
  // se la prima serie si svuota del tutto viene rimossa dal calendario, e
  // con lei la sua voce nella mappa colori (il vecchio puntatore non
  // sarebbe piu' valido come chiave).
  const QString color = colorFor(foundActivity);

  // 3. Modifica la prima serie direttamente in-place
  foundActivity->setEnd(occurrence.start);
  foundActivity->addException(occurrence.start);
  cleanupActivity(foundActivity);

  // 4. Configura la nuova serie clonata
  newActivity->setStart(target);
  newActivity->setEnd(originalEnd);

  // 5. Inserisce la seconda serie nel calendario (trasferendone l'ownership):
  // non e' un'operazione scelta dall'utente nel form, quindi eredita lo
  // stesso colore della serie originale invece di tornare "automatico".
  events::Activity& added = m_calendar.add(std::move(newActivity));
  if (!color.isEmpty()) {
    m_colors[&added] = color;
  }

  emit activitiesChanged();
  return true;
}

bool CalendarController::deleteOccurrence(const events::Occurrence& occurrence,
                                          bool andFollowing) {
  events::Activity* foundActivity = m_calendar.find(occurrence.source);
  if (!foundActivity) {
    return false; // L'attività non è presente nel calendario
  }

  if ( andFollowing ) foundActivity->setEnd(occurrence.start);
  
  foundActivity->addException(occurrence.start);

  cleanupActivity(foundActivity);

  emit activitiesChanged();
  return true;
}

bool CalendarController::modifyOccurrence(
    const events::Occurrence& occurrence,
    std::unique_ptr<events::Activity> replacement,
    const QString& color) {
  if (!replacement) {
    return false;
  }

  events::Activity* foundActivity = m_calendar.find(occurrence.source);

  if (!foundActivity) return false; // L'attività non è presente nel calendario


  foundActivity->addException(occurrence.start);  // Togli l'occorrenza dall'attività

  cleanupActivity(foundActivity);

  events::Activity& added = m_calendar.add(std::move(replacement));
  if (!color.isEmpty()) {
    m_colors[&added] = color;
  }
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
  // Il colore non e' un campo del modello (persistence::calendarToJson non
  // lo conosce): si aggiunge qui una sezione "colors" parallela ad
  // "activities" (stesso ordine di iterazione del Calendar, "" per le
  // attivita' senza colore esplicito), e si scrive il documento cosi'
  // aumentato al posto di persistence::saveToFile.
  QJsonObject root = persistence::calendarToJson(m_calendar);
  QJsonArray colors;
  for (const auto& activity : m_calendar) {
    colors.append(colorFor(activity.get()));
  }
  root.insert(QLatin1String("colors"), colors);

  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    if (error) *error = QString("Impossibile aprire il file in scrittura: %1").arg(filePath);
    return false;
  }
  if (file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0) {
    if (error) *error = QString("Errore di scrittura su: %1").arg(filePath);
    return false;
  }
  return true;
}

bool CalendarController::loadFromFile(const QString& filePath, QString* error) {
  if (!persistence::loadFromFile(m_calendar, filePath, error)) {
    return false;
  }

  // La sezione "colors" e' un'aggiunta di questo controller (vedi
  // saveToFile), ignota a persistence::loadFromFile: si rilegge qui il file
  // grezzo solo per quella parte. Assente o malformata -> nessun colore
  // esplicito per nessuna attivita' (tutte "automatico"), senza far fallire
  // il caricamento: il calendario e' comunque stato caricato correttamente.
  m_colors.clear();
  QFile file(filePath);
  if (file.open(QIODevice::ReadOnly)) {
    const QJsonArray colors =
        QJsonDocument::fromJson(file.readAll()).object().value(QLatin1String("colors")).toArray();
    int i = 0;
    for (const auto& activity : m_calendar) {
      if (i < colors.size()) {
        const QString hex = colors.at(i).toString();
        if (!hex.isEmpty()) {
          m_colors[activity.get()] = hex;
        }
      }
      ++i;
    }
  }

  emit activitiesChanged();
  return true;
}

void CalendarController::cleanupActivity(const events::Activity* activity){
  if(activity->getEnd() != events::TimePoint::max() && activity->occurrencesIn(activity->getStart(),activity->getEnd()).size() < 1) {
    m_calendar.remove(activity);
    m_colors.erase(activity);
  }
}

} // namespace app
