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
#include "events/domain/Deadline.h"
#include "events/domain/Event.h"
#include "events/domain/RecurrentEvent.h"
#include "events/domain/Reminder.h"
#include "events/generators/FixedIntervalGenerator.h"
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
  }

  void visit(const events::YearlyGenerator &generator) override {
    object.insert(QLatin1String("type"), QLatin1String("yearly"));
    object.insert(QLatin1String("start"), iso(generator.getStart()));
    if (generator.getEnd() != TimePoint::max()) {
      object.insert(QLatin1String("end"), iso(generator.getEnd()));
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
  }

  void visit(const events::Deadline &deadline) override {
    object.insert(QLatin1String("type"), QLatin1String("deadline"));
    object.insert(QLatin1String("title"),
                  QString::fromStdString(deadline.getTitle()));
    object.insert(QLatin1String("due"), iso(deadline.getDue()));
    object.insert(QLatin1String("priority"),
                 priorityKey(deadline.getPriority()));
    object.insert(QLatin1String("done"), deadline.isDone());
  }

  void visit(const events::Reminder &reminder) override {
    object.insert(QLatin1String("type"), QLatin1String("reminder"));
    object.insert(QLatin1String("title"),
                  QString::fromStdString(reminder.getTitle()));
    object.insert(QLatin1String("trigger"), iso(reminder.getTrigger()));
    object.insert(QLatin1String("message"),
                  QString::fromStdString(reminder.getMessage()));
    object.insert(QLatin1String("repeat_seconds"),
                  QJsonValue(qint64(reminder.getRepeatInterval().count())));
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
  return std::make_unique<events::Event>(title.toStdString(), start, duration);
}

std::shared_ptr<events::DateGenerator> generatorFromJson(const QJsonObject &json,
                                                         QString *error) {
  const QString type = json.value(QLatin1String("type")).toString();
  TimePoint start;
  TimePoint end = TimePoint::max();

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
    return std::make_shared<events::FixedIntervalGenerator>(start, interval,
                                                            end);
  }
  if (type == QLatin1String("yearly")) {
    if (!timePointFromJson(json, "start", start, error)) return nullptr;
    if (json.contains(QLatin1String("end")) &&
        !timePointFromJson(json, "end", end, error))
      return nullptr;
    return std::make_shared<events::YearlyGenerator>(start, end);
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
  return result;
}

std::unique_ptr<events::Deadline> deadlineFromJson(const QJsonObject &json,
                                                   QString *error) {
  QString title;
  TimePoint due;
  bool done = false;
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

  if (json.contains(QLatin1String("done")) &&
      !boolFromJson(json, "done", done, error))
    return nullptr;

  auto deadline =
      std::make_unique<events::Deadline>(title.toStdString(), due, priority);
  deadline->setDone(done);
  return deadline;
}

std::unique_ptr<events::Reminder> reminderFromJson(const QJsonObject &json,
                                                   QString *error) {
  QString title;
  QString message;
  TimePoint trigger;
  Duration repeat = Duration::zero();
  if (!stringFromJson(json, "title", title, error)) return nullptr;
  if (!timePointFromJson(json, "trigger", trigger, error)) return nullptr;
  if (!stringFromJson(json, "message", message, error)) return nullptr;
  if (json.contains(QLatin1String("repeat_seconds")) &&
      !secondsFromJson(json, "repeat_seconds", repeat, false, error))
    return nullptr;
  return std::make_unique<events::Reminder>(title.toStdString(), trigger,
                                            message.toStdString(), repeat);
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
  if (type == QLatin1String("deadline")) {
    return deadlineFromJson(json, error);
  }
  if (type == QLatin1String("reminder")) {
    return reminderFromJson(json, error);
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
