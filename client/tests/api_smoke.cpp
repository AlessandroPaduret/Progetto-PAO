#include <QCoreApplication>
#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <QTimeZone>

#include <iostream>

#include "api/ApiClient.h"
#include "api/ApiConfig.h"

// Smoke del client verso un server API live (non in ctest).
// Sequenza: login (utente "alice"/"s3cret" già creato negli smoke precedenti)
// -> GET /api/events su gennaio 2026 -> crea un evento -> lo elimina.
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    client::ApiClient api(client::apiBaseUrl());
    QObject::connect(&api, &client::ApiClient::loginSucceeded, &app,
                     [&](const QString& token) {
                         std::cout << "login OK, token: " << token.left(16).toStdString() << "...\n";
                         const QDateTime from(QDate(2026, 1, 1), QTime(0, 0), QTimeZone::UTC);
                         const QDateTime to(QDate(2026, 1, 31), QTime(23, 59, 59), QTimeZone::UTC);
                         api.getEvents(from, to);
                     });
    QObject::connect(&api, &client::ApiClient::loginFailed, &app, [&app](const QString& error) {
        std::cerr << "login fallito: " << error.toStdString() << "\n";
        app.exit(1);
    });
    QObject::connect(&api, &client::ApiClient::eventsLoaded, &app,
                     [&](const QVector<client::Occurrence>& occurrences) {
                         std::cout << "occorrenze caricate: " << occurrences.size() << "\n";
                         for (const auto& occurrence : occurrences) {
                             std::cout << "  - " << occurrence.eventId << ": "
                                       << occurrence.title.toStdString() << " @ "
                                       << occurrence.start.toString(Qt::ISODate).toStdString() << "\n";
                         }
                         client::CreateEventRequest request;
                         request.title = "Smoke client";
                         request.start = QDateTime(QDate(2026, 1, 15), QTime(12, 0), QTimeZone::UTC);
                         request.durationSec = 3600;
                         api.createEvent(request);
                     });
    QObject::connect(&api, &client::ApiClient::eventCreated, &app,
                     [&](qint64 id) {
                         std::cout << "evento creato: id " << id << "\n";
                         api.deleteEvent(id);
                     });
    QObject::connect(&api, &client::ApiClient::operationSucceeded, &app, [&](const QString& message) {
        std::cout << "operazione riuscita: " << message.toStdString() << "\n";
        app.exit(0);
    });
    QObject::connect(&api, &client::ApiClient::requestFailed, &app, [&app](const QString& error) {
        std::cerr << "errore: " << error.toStdString() << "\n";
        app.exit(1);
    });

    QTimer::singleShot(0, &api, [&]() { api.login("alice", "s3cret"); });
    return app.exec();
}
