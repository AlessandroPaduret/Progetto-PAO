#include "api/ApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace client {

ApiClient::ApiClient(const QUrl& baseUrl, QObject* parent)
    : QObject(parent), m_baseUrl(baseUrl) {}

void ApiClient::send(const QString& method, const QString& path,
                     const QJsonObject& body,
                     const std::function<void(const QJsonDocument&)>& onSuccess,
                     const std::function<void(const QString&)>& onError) {
    QNetworkRequest request(m_baseUrl.resolved(QUrl(path)));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    if (!m_token.isEmpty()) {
        request.setRawHeader("Authorization",
                             "Bearer " + m_token.toUtf8());
    }

    const QByteArray payload =
        body.isEmpty() ? QByteArray()
                       : QJsonDocument(body).toJson(QJsonDocument::Compact);

    QNetworkReply* reply = nullptr;
    if (method == QLatin1String("GET")) {
        reply = m_network.get(request);
    } else if (method == QLatin1String("POST")) {
        reply = m_network.post(request, payload);
    } else { // DELETE
        reply = m_network.deleteResource(request);
    }

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, onSuccess, onError]() {
                reply->deleteLater();
                const int status =
                    reply->attribute(QNetworkRequest::HttpStatusCodeAttribute)
                        .toInt();
                const QByteArray data = reply->readAll();

                QJsonParseError parseError;
                const QJsonDocument doc =
                    QJsonDocument::fromJson(data, &parseError);
                const QJsonObject obj =
                    (parseError.error == QJsonParseError::NoError &&
                     doc.isObject())
                        ? doc.object()
                        : QJsonObject();

                if (status >= 200 && status < 300) {
                    onSuccess(doc);
                    return;
                }
                const QString message = obj.value(QStringLiteral("error")).toString();
                onError(message.isEmpty()
                            ? QStringLiteral("errore HTTP %1").arg(status)
                            : message);
            });
}

void ApiClient::login(const QString& username, const QString& password) {
    QJsonObject body;
    body[QStringLiteral("username")] = username;
    body[QStringLiteral("password")] = password;

    send(QStringLiteral("POST"), QStringLiteral("/api/login"), body,
         [this](const QJsonDocument& doc) {
             m_token = doc.object().value(QStringLiteral("token")).toString();
             emit loginSucceeded(m_token);
         },
         [this](const QString& error) { emit loginFailed(error); });
}

void ApiClient::registerUser(const QString& username, const QString& password) {
    QJsonObject body;
    body[QStringLiteral("username")] = username;
    body[QStringLiteral("password")] = password;

    send(QStringLiteral("POST"), QStringLiteral("/api/register"), body,
         [this](const QJsonDocument&) { emit registerSucceeded(); },
         [this](const QString& error) { emit registerFailed(error); });
}

void ApiClient::getEvents(const QDateTime& from, const QDateTime& to) {
    const QString path =
        QStringLiteral("/api/events?from=%1&to=%2")
            .arg(QString::fromLatin1(
                     QUrl::toPercentEncoding(toUtcIso(from))),
                 QString::fromLatin1(
                     QUrl::toPercentEncoding(toUtcIso(to))));

    send(QStringLiteral("GET"), path, QJsonObject(),
         [this](const QJsonDocument& doc) {
             QVector<Occurrence> occurrences;
             const QJsonArray array = doc.array();
             occurrences.reserve(array.size());
             for (const auto& value : array) {
                 if (value.isObject()) {
                     occurrences.append(Occurrence::fromJson(value.toObject()));
                 }
             }
             emit eventsLoaded(occurrences);
         },
         [this](const QString& error) { emit requestFailed(error); });
}

void ApiClient::createEvent(const CreateEventRequest& request) {
    send(QStringLiteral("POST"), QStringLiteral("/api/create-event"),
         request.toJson(),
         [this](const QJsonDocument& doc) {
             const qint64 id =
                 doc.object().value(QStringLiteral("id")).toVariant().toLongLong();
             emit eventCreated(id);
             emit operationSucceeded(
                 QStringLiteral("evento creato (id %1)").arg(id));
         },
         [this](const QString& error) { emit requestFailed(error); });
}

void ApiClient::deleteEvent(qint64 eventId) {
    send(QStringLiteral("DELETE"),
         QStringLiteral("/api/events/%1").arg(eventId), QJsonObject(),
         [this](const QJsonDocument&) {
             emit operationSucceeded(QStringLiteral("evento eliminato"));
         },
         [this](const QString& error) { emit requestFailed(error); });
}

void ApiClient::addException(qint64 eventId, const QDateTime& exception) {
    QJsonObject body;
    body[QStringLiteral("exception")] = toUtcIso(exception);

    send(QStringLiteral("DELETE"),
         QStringLiteral("/api/events/%1").arg(eventId), body,
         [this](const QJsonDocument&) {
             emit operationSucceeded(QStringLiteral("eccezione aggiunta"));
         },
         [this](const QString& error) { emit requestFailed(error); });
}

} // namespace client
