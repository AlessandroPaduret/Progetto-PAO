#include "api/dto.h"

#include <QJsonArray>
#include <QJsonValue>

namespace client {

QString eventTypeToString(EventType type) {
    switch (type) {
        case EventType::Fixed:
            return QStringLiteral("fixed");
        case EventType::Yearly:
            return QStringLiteral("yearly");
        default:
            return QStringLiteral("single");
    }
}

bool eventTypeFromString(const QString& s, EventType& out) {
    if (s == QLatin1String("single")) {
        out = EventType::Single;
    } else if (s == QLatin1String("fixed")) {
        out = EventType::Fixed;
    } else if (s == QLatin1String("yearly")) {
        out = EventType::Yearly;
    } else {
        return false;
    }
    return true;
}

QDateTime parseUtcIso(const QString& iso) {
    // Il server manda wall-time UTC senza 'Z': aggiungiamo 'Z' perché
    // QDateTime::fromString(Qt::ISODate) la interpreti come UTC.
    return QDateTime::fromString(iso + QLatin1Char('Z'), Qt::ISODate);
}

QString toUtcIso(const QDateTime& dt) {
    // Qt 6 aggiunge 'Z' per gli orari UTC, ma il server la rifiuta:
    // l'ISO è wall-time UTC senza suffisso.
    QString iso = dt.toUTC().toString(Qt::ISODate);
    if (iso.endsWith(QLatin1Char('Z'))) {
        iso.chop(1);
    }
    return iso;
}

Occurrence Occurrence::fromJson(const QJsonObject& obj) {
    Occurrence occ;
    occ.eventId = obj.value(QStringLiteral("event_id")).toVariant().toLongLong();
    occ.title = obj.value(QStringLiteral("title")).toString();
    occ.start = parseUtcIso(obj.value(QStringLiteral("start")).toString());
    occ.end = parseUtcIso(obj.value(QStringLiteral("end")).toString());
    occ.type = obj.value(QStringLiteral("type")).toString();
    return occ;
}

QJsonObject Occurrence::toJson() const {
    QJsonObject obj;
    obj[QStringLiteral("event_id")] = static_cast<qint64>(eventId);
    obj[QStringLiteral("title")] = title;
    obj[QStringLiteral("start")] = toUtcIso(start);
    obj[QStringLiteral("end")] = toUtcIso(end);
    obj[QStringLiteral("type")] = type;
    return obj;
}

QJsonObject CreateEventRequest::toJson() const {
    QJsonObject obj;
    obj[QStringLiteral("title")] = title;
    obj[QStringLiteral("start")] = toUtcIso(start);
    obj[QStringLiteral("duration")] = static_cast<qint64>(durationSec);
    obj[QStringLiteral("type")] = eventTypeToString(type);
    if (type == EventType::Fixed) {
        obj[QStringLiteral("interval")] = static_cast<qint64>(intervalSec);
    }
    if (end.has_value()) {
        obj[QStringLiteral("end")] = toUtcIso(*end);
    }
    return obj;
}

CreateEventResponse CreateEventResponse::fromJson(const QJsonObject& obj) {
    CreateEventResponse response;
    response.id = obj.value(QStringLiteral("id")).toVariant().toLongLong();
    return response;
}

} // namespace client
