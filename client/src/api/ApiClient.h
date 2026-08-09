#ifndef CLIENT_APICLIENT_H
#define CLIENT_APICLIENT_H

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

#include <functional>

#include "api/dto.h"

namespace client {

/** @brief Client HTTP asincrono verso l'API server.
 *
 *  Tutte le chiamate sono non bloccanti (QNetworkAccessManager) e comunicano
 *  l'esito tramite segnali. Il token JWT è tenuto solo in memoria e viene
 *  iniettato come "Authorization: Bearer" sulle richieste autenticate.
 */
class ApiClient : public QObject {
    Q_OBJECT
public:
    explicit ApiClient(const QUrl& baseUrl, QObject* parent = nullptr);

    void login(const QString& username, const QString& password);
    void registerUser(const QString& username, const QString& password);
    void getEvents(const QDateTime& from, const QDateTime& to);
    void createEvent(const CreateEventRequest& request);
    void deleteEvent(qint64 eventId);
    void addException(qint64 eventId, const QDateTime& exception);
    void truncateEvent(qint64 eventId, const QDateTime& before);

signals:
    void loginSucceeded(const QString& token);
    void loginFailed(const QString& error);
    void registerSucceeded();
    void registerFailed(const QString& error);
    void eventsLoaded(const QVector<Occurrence>& occurrences);
    void eventCreated(qint64 id);
    void operationSucceeded(const QString& message);
    void requestFailed(const QString& error);

private:
    void send(const QString& method, const QString& path, const QJsonObject& body,
              const std::function<void(const QJsonDocument&)>& onSuccess,
              const std::function<void(const QString&)>& onError);

    QUrl m_baseUrl;
    QNetworkAccessManager m_network;
    QString m_token;
};

} // namespace client

#endif // CLIENT_APICLIENT_H
