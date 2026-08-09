#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QObject>
#include <QTime>
#include <QTimeZone>
#include <QTimer>

#include <iostream>

#include "api/ApiClient.h"
#include "api/ApiConfig.h"

// Smoke del client verso un server API live (non in ctest).
// Verifica il flusso completo con un evento ricorrente:
//   login -> create (3 occorrenze) -> addException (una sparisce)
//   -> truncate (resta solo la prima) -> delete.
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    client::ApiClient api(client::apiBaseUrl());

    const auto utc = [](int d, int h, int m) {
        return QDateTime(QDate(2026, 3, d), QTime(h, m), QTimeZone::UTC);
    };
    const auto march = [&](int day) {
        return QDateTime(QDate(2026, 3, day), QTime(0, 0), QTimeZone::UTC);
    };

    qint64 eventId = 0;
    bool awaitingCreate = false;    // successo del create-event
    bool awaitingException = false; // successo della addException
    bool awaitingTruncate = false;  // successo del truncate
    bool awaitingDelete = false;    // successo del delete

    auto fail = [&](const QString& error) {
        std::cerr << "FALLITO: " << error.toStdString() << "\n";
        app.exit(1);
    };

    QObject::connect(&api, &client::ApiClient::requestFailed, &app,
                     [&](const QString& error) { fail(error); });

    // 1. login
    QObject::connect(&api, &client::ApiClient::loginSucceeded, &app, [&](const QString&) {
        std::cout << "login OK\n";
        client::CreateEventRequest request;
        request.title = "Smoke ricorrente";
        request.start = utc(2, 9, 0);
        request.durationSec = 3600;
        request.type = client::EventType::Fixed;
        request.intervalSec = 604800; // settimanale
        request.end = utc(16, 9, 0);  // occorrenze: 02, 09, 16
        api.createEvent(request);
    });

    // 2. createEvent emette eventCreated E operationSucceeded: usiamo
    //    quest'ultimo come "create completato" prima di passare all'eccezione.
    QObject::connect(&api, &client::ApiClient::eventCreated, &app, [&](qint64 id) {
        eventId = id;
        std::cout << "evento creato: id " << id << "\n";
        awaitingCreate = true;
    });

    // 3. macchina a stati sui successi delle operazioni
    QObject::connect(&api, &client::ApiClient::operationSucceeded, &app, [&](const QString&) {
        if (awaitingCreate) {
            awaitingCreate = false;
            awaitingException = true;
            api.addException(eventId, utc(9, 9, 0)); // scarta la seconda
        } else if (awaitingException) {
            awaitingException = false;
            std::cout << "eccezione applicata\n";
            api.getEvents(march(1), march(31));
        } else if (awaitingTruncate) {
            awaitingTruncate = false;
            std::cout << "ricorrenza terminata\n";
            api.getEvents(march(1), march(31));
        } else if (awaitingDelete) {
            awaitingDelete = false;
            std::cout << "evento eliminato\n";
            app.exit(0);
        }
    });

    // 4. lettura occorrenze
    QObject::connect(&api, &client::ApiClient::eventsLoaded, &app, [&](const QVector<client::Occurrence>& occurrences) {
        std::cout << "occorrenze: ";
        for (const auto& occurrence : occurrences) {
            std::cout << occurrence.start.toString(Qt::ISODate).toStdString() << " ";
        }
        std::cout << "\n";

        if (occurrences.size() == 2) {
            // dopo l'eccezione restano 02/03 e 16/03: termina prima del 16/03
            awaitingTruncate = true;
            api.truncateEvent(eventId, utc(16, 9, 0));
        } else if (occurrences.size() == 1) {
            // dopo il truncate resta solo 02/03: elimina l'evento
            awaitingDelete = true;
            api.deleteEvent(eventId);
        } else {
            fail(QStringLiteral("attese 2 poi 1 occorrenze, trovate %1")
                     .arg(occurrences.size()));
        }
    });

    QTimer::singleShot(0, &api, [&]() { api.login("alice", "s3cret"); });
    return app.exec();
}
