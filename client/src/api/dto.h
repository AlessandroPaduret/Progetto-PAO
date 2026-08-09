#ifndef CLIENT_DTO_H
#define CLIENT_DTO_H

#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QVector>

#include <optional>

namespace client {

/** @brief Tipo di ricorrenza, allineato all'API ("single" | "fixed" | "yearly"). */
enum class EventType { Single, Fixed, Yearly };

QString eventTypeToString(EventType type);
bool eventTypeFromString(const QString& s, EventType& out);

/** @brief Occorrenza restituita da GET /api/events. */
struct Occurrence {
    qint64 eventId = 0;
    QString title;
    QDateTime start; /**< UTC */
    QDateTime end;   /**< UTC */

    static Occurrence fromJson(const QJsonObject& obj);
    QJsonObject toJson() const;
};

/** @brief Payload per POST /api/create-event. */
struct CreateEventRequest {
    QString title;
    QDateTime start;             /**< UTC */
    qint64 durationSec = 0;
    EventType type = EventType::Single;
    qint64 intervalSec = 0;      /**< solo per Fixed */
    std::optional<QDateTime> end; /**< fine ricorrenza (UTC); nullopt = illimitata */
    QVector<QDateTime> exceptions; /**< EXDATE (UTC) */

    QJsonObject toJson() const;
};

/** @brief Risposta di POST /api/create-event. */
struct CreateEventResponse {
    qint64 id = 0;

    static CreateEventResponse fromJson(const QJsonObject& obj);
};

/** @brief Converte un'ora ISO-8601 del server (wall-time UTC, senza 'Z') in QDateTime UTC. */
QDateTime parseUtcIso(const QString& iso);

/** @brief Formatta un QDateTime come ISO-8601 UTC (senza 'Z', come atteso dal server). */
QString toUtcIso(const QDateTime& dt);

} // namespace client

#endif // CLIENT_DTO_H
