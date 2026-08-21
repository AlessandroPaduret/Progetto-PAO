#include "persistence/JsonPersistence.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>

#include <memory>
#include <stdexcept>
#include <vector>

#include "events/core/ActivityVisitor.h"
#include "events/core/DateGeneratorVisitor.h"
#include "events/core/Format.h"
#include "events/domain/AllDayEvent.h"
#include "events/domain/Anniversary.h"
#include "events/domain/Event.h"
#include "events/domain/Meeting.h"
#include "events/domain/RecurrentEvent.h"
#include "events/domain/Task.h"
#include "events/generators/FixedIntervalGenerator.h"
#include "events/generators/MonthlyGenerator.h"
#include "events/generators/NullGenerator.h"
#include "events/generators/YearlyGenerator.h"

namespace persistence {

namespace {

using events::Duration;
using events::TimePoint;

void setError(QString *error, const QString &message) {
  if (error) {
    *error = message;
  }
}

QString iso(const TimePoint tp) {
  return QString::fromStdString(events::formatIso8601(tp));
}

// ------------- lettura dei campi con validazione -------------

bool timePointFromJson(const QJsonObject &json, const char *key,
                       TimePoint &out, QString *error) {
  const QJsonValue value = json.value(QLatin1String(key));
  if (!value.isString()) {
    setError(error, QString("Campo mancante o non valido: %1")
                        .arg(QLatin1String(key)));
    return false;
  }
  if (!events::parseIso8601(value.toString().toStdString(), out)) {
    setError(error, QString("Data ISO-8601 non valida: %1")
                        .arg(value.toString()));
    return false;
  }
  return true;
}

bool secondsFromJson(const QJsonObject &json, const char *key, Duration &out,
                     bool allowNegative, QString *error) {
  const QJsonValue value = json.value(QLatin1String(key));
  if (!value.isDouble()) {
    setError(error, QString("Campo mancante o non valido: %1")
                        .arg(QLatin1String(key)));
    return false;
  }
  const qint64 s = value.toInteger();
  if (!allowNegative && s < 0) {
    setError(error, QString("Valore negativo non ammesso per: %1")
                        .arg(QLatin1String(key)));
    return false;
  }
  out = Duration(s);
  return true;
}

bool stringFromJson(const QJsonObject &json, const char *key, QString &out,
                    QString *error) {
  const QJsonValue value = json.value(QLatin1String(key));
  if (!value.isString()) {
    setError(error, QString("Campo mancante o non valido: %1")
                        .arg(QLatin1String(key)));
    return false;
  }
  out = value.toString();
  return true;
}

bool boolFromJson(const QJsonObject &json, const char *key, bool &out,
                  QString *error) {
  const QJsonValue value = json.value(QLatin1String(key));
  if (!value.isBool()) {
    setError(error, QString("Campo mancante o non valido: %1")
                        .arg(QLatin1String(key)));
    return false;
  }
  out = value.toBool();
  return true;
}

// ------------- Visitor di serializzazione dei generatori -------------

class JsonGeneratorVisitor : public events::DateGeneratorVisitor {
public:
  QJsonObject object;

  void visit(const events::FixedIntervalGenerator &generator) override {
    object.insert(QLatin1String("type"), QLatin1String("fixed"));
    object.insert(QLatin1String("start"), iso(generator.getStart()));
    object.insert(QLatin1String("interval_seconds"),
                  QJsonValue(qint64(generator.getInterval().count())));
    if (generator.getEnd() != TimePoint::max()) {
      object.insert(QLatin1String("end"), iso(generator.getEnd()));
    }
    if (generator.getMaxOccurrences() > 0) {
      object.insert(QLatin1String("max_occurrences"),
                    QJsonValue(qint64(generator.getMaxOccurrences())));
    }
  }

  void visit(const events::MonthlyGenerator &generator) override {
    object.insert(QLatin1String("type"), QLatin1String("monthly"));
    object.insert(QLatin1String("start"), iso(generator.getStart()));
    object.insert(QLatin1String("interval_months"),
                  QJsonValue(generator.getMonths()));
    if (generator.getEnd() != TimePoint::max()) {
      object.insert(QLatin1String("end"), iso(generator.getEnd()));
    }
    if (generator.getMaxOccurrences() > 0) {
      object.insert(QLatin1String("max_occurrences"),
                    QJsonValue(qint64(generator.getMaxOccurrences())));
    }
  }

  void visit(const events::YearlyGenerator &generator) override {
    object.insert(QLatin1String("type"), QLatin1String("yearly"));
    object.insert(QLatin1String("start"), iso(generator.getStart()));
    if (generator.getEnd() != TimePoint::max()) {
      object.insert(QLatin1String("end"), iso(generator.getEnd()));
    }
    if (generator.getMaxOccurrences() > 0) {
      object.insert(QLatin1String("max_occurrences"),
                    QJsonValue(qint64(generator.getMaxOccurrences())));
    }
  }

  void visit(const events::NullGenerator &) override {
    object.insert(QLatin1String("type"), QLatin1String("null"));
  }
};

// ------------- Visitor di serializzazione delle attivita' -------------

class JsonActivityVisitor : public events::ActivityVisitor {
public:
  QJsonObject object;

  void visit(const events::Event &event) override {
    object.insert(QLatin1String("type"), QLatin1String("event"));
    object.insert(QLatin1String("title"),
                  QString::fromStdString(event.getTitle()));
    object.insert(QLatin1String("start"), iso(event.getStart()));
    object.insert(QLatin1String("duration_seconds"),
                  QJsonValue(qint64(event.getDuration().count())));
    object.insert(QLatin1String("done"), event.isDone());
  }

  void visit(const events::RecurrentEvent &event) override {
    object.insert(QLatin1String("type"), QLatin1String("recurrent"));

    QJsonObject templ;
    templ.insert(QLatin1String("title"),
                 QString::fromStdString(event.getTitle()));
    templ.insert(QLatin1String("start"),
                 iso(event.getTemplateEvent().getStart()));
    templ.insert(QLatin1String("duration_seconds"),
                 QJsonValue(qint64(event.getTemplateEvent().getDuration().count())));
    object.insert(QLatin1String("template"), templ);

    JsonGeneratorVisitor generatorVisitor;
    event.getGenerator()->accept(generatorVisitor);
    object.insert(QLatin1String("generator"), generatorVisitor.object);

    QJsonArray exceptions;
    for (const TimePoint tp : event.getExceptions()) {
      exceptions.append(iso(tp));
    }
    object.insert(QLatin1String("exceptions"), exceptions);

    QJsonArray doneOccurrences;
    for (const TimePoint tp : event.getDoneOccurrences()) {
      doneOccurrences.append(iso(tp));
    }
    object.insert(QLatin1String("done_occurrences"), doneOccurrences);
  }

  void visit(const events::Task &task) override {
    object.insert(QLatin1String("type"), QLatin1String("task"));
    object.insert(QLatin1String("title"),
                  QString::fromStdString(task.getTitle()));
    object.insert(QLatin1String("due"), iso(task.getDue()));
    object.insert(QLatin1String("priority"),
                 priorityKey(task.getPriority()));
    object.insert(QLatin1String("done"), task.isDone());
  }

  void visit(const events::Meeting &meeting) override {
    object.insert(QLatin1String("type"), QLatin1String("meeting"));
    object.insert(QLatin1String("title"),
                  QString::fromStdString(meeting.getTitle()));
    object.insert(QLatin1String("start"), iso(meeting.getStart()));
    object.insert(QLatin1String("duration_seconds"),
                  QJsonValue(qint64(meeting.getDuration().count())));
    object.insert(QLatin1String("location"),
                  QString::fromStdString(meeting.getLocation()));
    QJsonArray attendees;
    for (const events::String &name : meeting.getAttendees()) {
      attendees.append(QString::fromStdString(name));
    }
    object.insert(QLatin1String("attendees"), attendees);
    object.insert(QLatin1String("done"), meeting.isDone());
  }

  void visit(const events::AllDayEvent &event) override {
    object.insert(QLatin1String("type"), QLatin1String("allday"));
    object.insert(QLatin1String("title"),
                  QString::fromStdString(event.getTitle()));
    object.insert(QLatin1String("start"), iso(event.getStart()));
    object.insert(QLatin1String("end"), iso(event.getEnd()));
    object.insert(QLatin1String("done"), event.isDone());
  }

  void visit(const events::Anniversary &anniversary) override {
    object.insert(QLatin1String("type"), QLatin1String("anniversary"));
    object.insert(QLatin1String("title"),
                  QString::fromStdString(anniversary.getTitle()));
    object.insert(QLatin1String("date"), iso(anniversary.getStart()));
    if (anniversary.getEnd() != TimePoint::max()) {
      object.insert(QLatin1String("end"), iso(anniversary.getEnd()));
    }
    QJsonArray doneOccurrences;
    for (const TimePoint tp : anniversary.getDoneOccurrences()) {
      doneOccurrences.append(iso(tp));
    }
    object.insert(QLatin1String("done_occurrences"), doneOccurrences);
  }

private:
  static QString priorityKey(events::Priority priority) {
    switch (priority) {
    case events::Priority::Low:
      return QLatin1String("low");
    case events::Priority::High:
      return QLatin1String("high");
    case events::Priority::Medium:
    default:
      return QLatin1String("medium");
    }
  }
};

// ------------- deserializzazione dei singoli tipi -------------

std::unique_ptr<events::Event> eventFromJson(const QJsonObject &json,
                                             QString *error) {
  QString title;
  TimePoint start;
  Duration duration;
  if (!stringFromJson(json, "title", title, error)) return nullptr;
  if (!timePointFromJson(json, "start", start, error)) return nullptr;
  if (!secondsFromJson(json, "duration_seconds", duration, false, error))
    return nullptr;
  auto event =
      std::make_unique<events::Event>(title.toStdString(), start, duration);
  if (json.contains(QLatin1String("done"))) {
    bool done = false;
    if (!boolFromJson(json, "done", done, error)) return nullptr;
    event->setDone(done);
  }
  return event;
}

std::shared_ptr<events::DateGenerator> generatorFromJson(const QJsonObject &json,
                                                         QString *error) {
  const QString type = json.value(QLatin1String("type")).toString();
  TimePoint start;
  TimePoint end = TimePoint::max();

  auto readMaxOccurrences = [&json, error](std::size_t &out) -> bool {
    if (!json.contains(QLatin1String("max_occurrences"))) {
      out = 0;
      return true;
    }
    const QJsonValue value = json.value(QLatin1String("max_occurrences"));
    if (!value.isDouble() || value.toInteger() < 0) {
      setError(error, "Campo max_occurrences non valido");
      return false;
    }
    out = static_cast<std::size_t>(value.toInteger());
    return true;
  };

  if (type == QLatin1String("fixed")) {
    Duration interval;
    if (!timePointFromJson(json, "start", start, error)) return nullptr;
    if (!secondsFromJson(json, "interval_seconds", interval, false, error))
      return nullptr;
    if (interval <= Duration::zero()) {
      setError(error, "L'intervallo del generatore deve essere positivo");
      return nullptr;
    }
    if (json.contains(QLatin1String("end")) &&
        !timePointFromJson(json, "end", end, error))
      return nullptr;
    auto gen = std::make_shared<events::FixedIntervalGenerator>(start, interval,
                                                                end);
    std::size_t maxOcc = 0;
    if (!readMaxOccurrences(maxOcc)) return nullptr;
    gen->setMaxOccurrences(maxOcc);
    return gen;
  }
  if (type == QLatin1String("monthly")) {
    if (!timePointFromJson(json, "start", start, error)) return nullptr;
    const QJsonValue monthsValue = json.value(QLatin1String("interval_months"));
    if (!monthsValue.isDouble() || monthsValue.toInteger() <= 0) {
      setError(error, "Campo interval_months non valido");
      return nullptr;
    }
    if (json.contains(QLatin1String("end")) &&
        !timePointFromJson(json, "end", end, error))
      return nullptr;
    auto gen = std::make_shared<events::MonthlyGenerator>(
        start, static_cast<int>(monthsValue.toInteger()), end);
    std::size_t maxOcc = 0;
    if (!readMaxOccurrences(maxOcc)) return nullptr;
    gen->setMaxOccurrences(maxOcc);
    return gen;
  }
  if (type == QLatin1String("yearly")) {
    if (!timePointFromJson(json, "start", start, error)) return nullptr;
    if (json.contains(QLatin1String("end")) &&
        !timePointFromJson(json, "end", end, error))
      return nullptr;
    auto gen = std::make_shared<events::YearlyGenerator>(start, end);
    std::size_t maxOcc = 0;
    if (!readMaxOccurrences(maxOcc)) return nullptr;
    gen->setMaxOccurrences(maxOcc);
    return gen;
  }
  if (type == QLatin1String("null")) {
    return std::make_shared<events::NullGenerator>();
  }
  setError(error, "Tipo di generatore sconosciuto: " + type);
  return nullptr;
}

std::unique_ptr<events::RecurrentEvent> recurrentFromJson(const QJsonObject &json,
                                                          QString *error) {
  const QJsonValue templateValue = json.value(QLatin1String("template"));
  if (!templateValue.isObject()) {
    setError(error, "Campo mancante o non valido: template");
    return nullptr;
  }
  const QJsonObject templ = templateValue.toObject();

  QString title;
  TimePoint start;
  Duration duration;
  if (!stringFromJson(templ, "title", title, error)) return nullptr;
  if (!timePointFromJson(templ, "start", start, error)) return nullptr;
  if (!secondsFromJson(templ, "duration_seconds", duration, false, error))
    return nullptr;

  const QJsonValue generatorValue = json.value(QLatin1String("generator"));
  if (!generatorValue.isObject()) {
    setError(error, "Campo mancante o non valido: generator");
    return nullptr;
  }
  std::shared_ptr<events::DateGenerator> generator =
      generatorFromJson(generatorValue.toObject(), error);
  if (!generator) return nullptr;

  auto result = std::make_unique<events::RecurrentEvent>(
      generator, events::Event(title.toStdString(), start, duration));

  const QJsonValue exceptionsValue = json.value(QLatin1String("exceptions"));
  if (exceptionsValue.isArray()) {
    for (const QJsonValue &value : exceptionsValue.toArray()) {
      if (!value.isString()) {
        setError(error, "Eccezione non valida nell'elenco exceptions");
        return nullptr;
      }
      TimePoint tp;
      if (!events::parseIso8601(value.toString().toStdString(), tp)) {
        setError(error, "Data di eccezione non valida: " + value.toString());
        return nullptr;
      }
      result->addException(tp);
    }
  }

  const QJsonValue doneValue = json.value(QLatin1String("done_occurrences"));
  if (doneValue.isArray()) {
    for (const QJsonValue &value : doneValue.toArray()) {
      if (!value.isString()) {
        setError(error, "Occorrenza evasa non valida nell'elenco");
        return nullptr;
      }
      TimePoint tp;
      if (!events::parseIso8601(value.toString().toStdString(), tp)) {
        setError(error, "Data di occorrenza evasa non valida: " +
                            value.toString());
        return nullptr;
      }
      result->setDoneAt(tp, true);
    }
  }
  return result;
}

std::unique_ptr<events::Task> taskFromJson(const QJsonObject &json,
                                           QString *error) {
  QString title;
  TimePoint due;
  if (!stringFromJson(json, "title", title, error)) return nullptr;
  if (!timePointFromJson(json, "due", due, error)) return nullptr;

  events::Priority priority = events::Priority::Medium;
  const QString priorityText =
      json.value(QLatin1String("priority")).toString();
  if (priorityText == QLatin1String("low")) {
    priority = events::Priority::Low;
  } else if (priorityText == QLatin1String("high")) {
    priority = events::Priority::High;
  } else if (priorityText == QLatin1String("medium")) {
    priority = events::Priority::Medium;
  } else {
    setError(error, "Priorita' sconosciuta: " + priorityText);
    return nullptr;
  }

  auto task = std::make_unique<events::Task>(title.toStdString(), due, priority);
  if (json.contains(QLatin1String("done"))) {
    bool done = false;
    if (!boolFromJson(json, "done", done, error)) return nullptr;
    task->setDone(done);
  }
  return task;
}

std::unique_ptr<events::Meeting> meetingFromJson(const QJsonObject &json,
                                                 QString *error) {
  QString title;
  QString location;
  TimePoint start;
  Duration duration;
  if (!stringFromJson(json, "title", title, error)) return nullptr;
  if (!timePointFromJson(json, "start", start, error)) return nullptr;
  if (!secondsFromJson(json, "duration_seconds", duration, false, error))
    return nullptr;
  if (!stringFromJson(json, "location", location, error)) return nullptr;

  auto meeting = std::make_unique<events::Meeting>(
      title.toStdString(), start, duration, location.toStdString());

  const QJsonValue attendeesValue = json.value(QLatin1String("attendees"));
  if (attendeesValue.isArray()) {
    for (const QJsonValue &value : attendeesValue.toArray()) {
      if (!value.isString()) {
        setError(error, "Partecipante non valido nell'elenco attendees");
        return nullptr;
      }
      meeting->addAttendee(value.toString().toStdString());
    }
  }

  if (json.contains(QLatin1String("done"))) {
    bool done = false;
    if (!boolFromJson(json, "done", done, error)) return nullptr;
    meeting->setDone(done);
  }
  return meeting;
}

std::unique_ptr<events::AllDayEvent> alldayFromJson(const QJsonObject &json,
                                                    QString *error) {
  QString title;
  TimePoint start;
  TimePoint end;
  if (!stringFromJson(json, "title", title, error)) return nullptr;
  if (!timePointFromJson(json, "start", start, error)) return nullptr;
  if (!timePointFromJson(json, "end", end, error)) return nullptr;
  if (end <= start) {
    setError(error, "La fine deve essere successiva all'inizio (almeno un giorno).");
    return nullptr;
  }

  auto event = std::make_unique<events::AllDayEvent>(title.toStdString(), start,
                                                     end);
  if (json.contains(QLatin1String("done"))) {
    bool done = false;
    if (!boolFromJson(json, "done", done, error)) return nullptr;
    event->setDone(done);
  }
  return event;
}

std::unique_ptr<events::Anniversary> anniversaryFromJson(
    const QJsonObject &json, QString *error) {
  QString title;
  TimePoint date;
  if (!stringFromJson(json, "title", title, error)) return nullptr;
  if (!timePointFromJson(json, "date", date, error)) return nullptr;

  TimePoint end = TimePoint::max();
  if (json.contains(QLatin1String("end")) &&
      !timePointFromJson(json, "end", end, error))
    return nullptr;

  auto anniversary = std::make_unique<events::Anniversary>(
      title.toStdString(), date, end);

  const QJsonValue doneValue = json.value(QLatin1String("done_occurrences"));
  if (doneValue.isArray()) {
    for (const QJsonValue &value : doneValue.toArray()) {
      if (!value.isString()) {
        setError(error, "Occorrenza evasa non valida nell'elenco");
        return nullptr;
      }
      TimePoint tp;
      if (!events::parseIso8601(value.toString().toStdString(), tp)) {
        setError(error, "Data di occorrenza evasa non valida: " +
                            value.toString());
        return nullptr;
      }
      anniversary->setDoneAt(tp, true);
    }
  }
  return anniversary;
}

} // namespace

// ------------- API pubbliche -------------

QJsonObject activityToJson(const events::Activity &activity) {
  JsonActivityVisitor visitor;
  activity.accept(visitor);
  return visitor.object;
}

QJsonObject calendarToJson(const events::Calendar &calendar) {
  QJsonArray activities;
  for (const auto &activity : calendar) {
    activities.append(activityToJson(*activity));
  }
  QJsonObject root;
  root.insert(QLatin1String("version"), 1);
  root.insert(QLatin1String("activities"), activities);
  return root;
}

std::unique_ptr<events::Activity> activityFromJson(const QJsonObject &json,
                                                   QString *error) {
  const QString type = json.value(QLatin1String("type")).toString();
  if (type == QLatin1String("event")) {
    return eventFromJson(json, error);
  }
  if (type == QLatin1String("recurrent")) {
    return recurrentFromJson(json, error);
  }
  if (type == QLatin1String("task")) {
    return taskFromJson(json, error);
  }
  if (type == QLatin1String("meeting")) {
    return meetingFromJson(json, error);
  }
  if (type == QLatin1String("allday")) {
    return alldayFromJson(json, error);
  }
  if (type == QLatin1String("anniversary")) {
    return anniversaryFromJson(json, error);
  }
  setError(error, "Tipo di attivita' sconosciuto: " + type);
  return nullptr;
}

bool calendarFromJsonArray(events::Calendar &calendar, const QJsonArray &array,
                           QString *error) {
  for (const QJsonValue &value : array) {
    if (!value.isObject()) {
      setError(error, "Voce dell'elenco non valida");
      return false;
    }
    auto activity = activityFromJson(value.toObject(), error);
    if (!activity) {
      return false;
    }
    calendar.add(std::move(activity));
  }
  return true;
}

bool saveToFile(const events::Calendar &calendar, const QString &filePath,
                QString *error) {
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    setError(error, QString("Impossibile aprire il file in scrittura: %1")
                        .arg(filePath));
    return false;
  }
  const QJsonDocument document(calendarToJson(calendar));
  if (file.write(document.toJson(QJsonDocument::Indented)) < 0) {
    setError(error, QString("Errore di scrittura su: %1").arg(filePath));
    return false;
  }
  return true;
}

bool loadFromFile(events::Calendar &calendar, const QString &filePath,
                  QString *error) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    setError(error, QString("Impossibile aprire il file in lettura: %1")
                        .arg(filePath));
    return false;
  }
  QJsonParseError parseError;
  const QJsonDocument document =
      QJsonDocument::fromJson(file.readAll(), &parseError);
  if (parseError.error != QJsonParseError::NoError) {
    setError(error, QString("File JSON non valido: %1")
                        .arg(parseError.errorString()));
    return false;
  }
  if (!document.isObject()) {
    setError(error, "Il documento JSON deve essere un oggetto");
    return false;
  }
  const QJsonObject root = document.object();
  if (root.value(QLatin1String("version")).toInt() != 1) {
    setError(error, "Versione del formato non supportata");
    return false;
  }
  const QJsonValue activitiesValue = root.value(QLatin1String("activities"));
  if (!activitiesValue.isArray()) {
    setError(error, "Campo mancante o non valido: activities");
    return false;
  }
  events::Calendar loaded;
  if (!calendarFromJsonArray(loaded, activitiesValue.toArray(), error)) {
    return false;
  }
  calendar = std::move(loaded);
  return true;
}

} // namespace persistence
