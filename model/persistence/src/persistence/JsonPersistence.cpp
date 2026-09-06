#include "persistence/JsonPersistence.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>

#include <memory>
#include <stdexcept>
#include <vector>

#include "events/events.h"

using events::Duration;
using events::TimePoint;

namespace persistence {

namespace {

void setError(QString *error, const QString &message) {
  if (error) {
    *error = message;
  }
}

QString iso(const TimePoint tp) {
  return QString::fromStdString(events::formatIso8601(tp));
}

// ------------- Lettura dei campi con validazione -------------

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

// ------------- Visitor di serializzazione dei generatori (Stateless) -------------

class JsonGeneratorVisitor : public events::DateGeneratorVisitor {
public:
  QJsonObject object;

  void visit(const events::FixedIntervalGenerator &generator) override {
    object.insert(QLatin1String("type"), QLatin1String("fixed"));
    object.insert(QLatin1String("interval_seconds"),
                  QJsonValue(qint64(generator.getInterval().count())));
  }

  void visit(const events::MonthlyGenerator &generator) override {
    object.insert(QLatin1String("type"), QLatin1String("monthly"));
    object.insert(QLatin1String("interval_months"),
                  QJsonValue(generator.getMonths()));
  }

  void visit(const events::YearlyGenerator &generator) override {
    object.insert(QLatin1String("type"), QLatin1String("yearly"));
    object.insert(QLatin1String("interval_years"),
                  QJsonValue(generator.getYears()));
  }

  void visit(const events::SingleGenerator &/*generator*/) override {
    object.insert(QLatin1String("type"), QLatin1String("single"));
  }
};

// ------------- Visitor di serializzazione delle attivita' -------------

class JsonActivityVisitor : public events::ActivityVisitor {
public:
  QJsonObject object;

  void visit(const events::Activity &activity) override {
    object.insert(QLatin1String("type"), QLatin1String("event"));
    writeCommonActivityFields(activity);
  }

  void visit(const events::Task &task) override {
    object.insert(QLatin1String("type"), QLatin1String("task"));
    writeCommonActivityFields(task);
    object.insert(QLatin1String("priority"), priorityKey(task.getPriority()));

    QJsonArray done;
    for (const TimePoint tp : task.getDoneOccurrences()) {
      done.append(iso(tp));
    }
    object.insert(QLatin1String("done_occurrences"), done);
  }

  void visit(const events::Meeting &meeting) override {
    object.insert(QLatin1String("type"), QLatin1String("meeting"));
    writeCommonActivityFields(meeting);
    object.insert(QLatin1String("location"),
                  QString::fromStdString(meeting.getLocation()));

    QJsonArray attendees;
    for (const events::String &name : meeting.getAttendees()) {
      attendees.append(QString::fromStdString(name));
    }
    object.insert(QLatin1String("attendees"), attendees);
  }

private:
  void writeCommonActivityFields(const events::Activity &activity) {
    object.insert(QLatin1String("title"), QString::fromStdString(activity.getTitle()));
    object.insert(QLatin1String("start"), iso(activity.getStart()));
    object.insert(QLatin1String("duration_seconds"), QJsonValue(qint64(activity.getDuration().count())));

    if (activity.getEnd() != TimePoint::max()) {
      object.insert(QLatin1String("end"), iso(activity.getEnd()));
    }

    JsonGeneratorVisitor generatorVisitor;
    activity.getGenerator().accept(generatorVisitor);
    object.insert(QLatin1String("generator"), generatorVisitor.object);

    QJsonArray exceptions;
    for (const TimePoint tp : activity.getExceptions()) {
      exceptions.append(iso(tp));
    }
    object.insert(QLatin1String("exceptions"), exceptions);
  }

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

// ------------- Deserializzazione Generatori e Attivita' -------------

std::shared_ptr<const events::DateGenerator> generatorFromJson(const QJsonObject &json,
                                                               QString *error) {
  const QString type = json.value(QLatin1String("type")).toString();

  if (type == QLatin1String("single")) {
    return std::make_shared<events::SingleGenerator>();
  }

  if (type == QLatin1String("fixed")) {
    Duration interval;
    if (!secondsFromJson(json, "interval_seconds", interval, false, error)) {
      return nullptr;
    }
    if (interval <= Duration::zero()) {
      setError(error, "L'intervallo del generatore deve essere positivo");
      return nullptr;
    }
    return std::make_shared<events::FixedIntervalGenerator>(interval);
  }

  if (type == QLatin1String("monthly")) {
    const QJsonValue monthsValue = json.value(QLatin1String("interval_months"));
    if (!monthsValue.isDouble() || monthsValue.toInteger() <= 0) {
      setError(error, "Campo interval_months non valido");
      return nullptr;
    }
    return std::make_shared<events::MonthlyGenerator>(
        static_cast<int>(monthsValue.toInteger()));
  }

  if (type == QLatin1String("yearly")) {
    const QJsonValue yearsValue = json.value(QLatin1String("interval_years"));
    int years = 1;
    if (yearsValue.isDouble() && yearsValue.toInteger() > 0) {
      years = static_cast<int>(yearsValue.toInteger());
    }
    return std::make_shared<events::YearlyGenerator>(years);
  }

  setError(error, "Tipo di generatore sconosciuto: " + type);
  return nullptr;
}

struct CommonActivityData {
  QString title;
  TimePoint start;
  TimePoint end = TimePoint::max();
  Duration duration = Duration::zero();
  std::shared_ptr<const events::DateGenerator> generator;
  std::unordered_set<TimePoint> exceptions;
};

bool readCommonActivityData(const QJsonObject &json, CommonActivityData &out, QString *error) {
  if (!stringFromJson(json, "title", out.title, error)) return false;
  if (!timePointFromJson(json, "start", out.start, error)) return false;
  if (!secondsFromJson(json, "duration_seconds", out.duration, false, error)) return false;

  if (json.contains(QLatin1String("end"))) {
    if (!timePointFromJson(json, "end", out.end, error)) return false;
  }

  const QJsonValue genVal = json.value(QLatin1String("generator"));
  if (genVal.isObject()) {
    out.generator = generatorFromJson(genVal.toObject(), error);
    if (!out.generator) return false;
  } else {
    out.generator = std::make_shared<events::SingleGenerator>();
  }

  const QJsonValue exceptionsValue = json.value(QLatin1String("exceptions"));
  if (exceptionsValue.isArray()) {
    for (const QJsonValue &value : exceptionsValue.toArray()) {
      if (!value.isString()) {
        setError(error, "Eccezione non valida nell'elenco exceptions");
        return false;
      }
      TimePoint tp;
      if (!events::parseIso8601(value.toString().toStdString(), tp)) {
        setError(error, "Data di eccezione non valida: " + value.toString());
        return false;
      }
      out.exceptions.insert(tp);
    }
  }
  return true;
}

std::unique_ptr<events::Activity> eventFromJson(const QJsonObject &json, QString *error) {
  CommonActivityData data;
  if (!readCommonActivityData(json, data, error)) return nullptr;

  auto result = std::make_unique<events::Activity>(
      data.title.toStdString(), data.start, data.duration, data.generator, data.end
  );
  for (const TimePoint tp : data.exceptions) {
    result->addException(tp);
  }
  return result;
}

std::unique_ptr<events::Task> taskFromJson(const QJsonObject &json, QString *error) {
  CommonActivityData data;
  if (!readCommonActivityData(json, data, error)) return nullptr;

  events::Priority priority = events::Priority::Medium;
  const QString priorityText = json.value(QLatin1String("priority")).toString();
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

  auto task = std::make_unique<events::Task>(
      data.title.toStdString(), data.start, data.duration, priority, data.generator, data.end
  );
  for (const TimePoint tp : data.exceptions) {
    task->addException(tp);
  }

  const QJsonValue doneValue = json.value(QLatin1String("done_occurrences"));
  if (doneValue.isArray()) {
    for (const QJsonValue &value : doneValue.toArray()) {
      if (!value.isString()) {
        setError(error, "Occorrenza evasa non valida nell'elenco done_occurrences");
        return nullptr;
      }
      TimePoint tp;
      if (!events::parseIso8601(value.toString().toStdString(), tp)) {
        setError(error, "Data di occorrenza evasa non valida: " + value.toString());
        return nullptr;
      }
      task->setDone(tp, true);
    }
  }
  return task;
}

std::unique_ptr<events::Meeting> meetingFromJson(const QJsonObject &json, QString *error) {
  CommonActivityData data;
  if (!readCommonActivityData(json, data, error)) return nullptr;

  QString location;
  if (!stringFromJson(json, "location", location, error)) return nullptr;

  auto meeting = std::make_unique<events::Meeting>(
      data.title.toStdString(), data.start, data.duration, location.toStdString(), data.generator, data.end
  );
  for (const TimePoint tp : data.exceptions) {
    meeting->addException(tp);
  }

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

  return meeting;
}

} // namespace

// ------------- API Pubbliche -------------

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
  if (type == QLatin1String("task")) {
    return taskFromJson(json, error);
  }
  if (type == QLatin1String("meeting")) {
    return meetingFromJson(json, error);
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
    setError(error, QString("Impossibile aprire il file in scrittura: %1").arg(filePath));
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
    setError(error, QString("Impossibile aprire il file in lettura: %1").arg(filePath));
    return false;
  }
  QJsonParseError parseError;
  const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
  if (parseError.error != QJsonParseError::NoError) {
    setError(error, QString("File JSON non valido: %1").arg(parseError.errorString()));
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