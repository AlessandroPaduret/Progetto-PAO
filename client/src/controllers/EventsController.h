#ifndef CLIENT_EVENTSCONTROLLER_H
#define CLIENT_EVENTSCONTROLLER_H

#include <QDateTime>
#include <QObject>
#include <QVector>

#include "api/dto.h"

namespace client {

class ApiClient;

/** @brief Gestisce il range corrente e le operazioni sugli eventi.
 *
 *  Dopo ogni operazione riuscita ricarica automaticamente le occorrenze
 *  del range corrente.
 */
class EventsController : public QObject {
    Q_OBJECT
public:
    explicit EventsController(ApiClient* api, QObject* parent = nullptr);

    void setRange(const QDateTime& from, const QDateTime& to);
    QDateTime from() const;
    QDateTime to() const;

    void refresh();

    void createEvent(const CreateEventRequest& request);
    void deleteEvent(qint64 eventId);
    void addException(qint64 eventId, const QDateTime& exception);

signals:
    void eventsChanged(const QVector<Occurrence>& occurrences);
    void noticeOccurred(const QString& message);
    void errorOccurred(const QString& error);

private:
    void wireOperation();

    ApiClient* m_api;
    QDateTime m_from;
    QDateTime m_to;
};

} // namespace client

#endif // CLIENT_EVENTSCONTROLLER_H
