#include <cstdlib>
#include <iostream>

#include <chrono>

#include "db/ConnectionPool.h"
#include "db/EventRepository.h"
#include "db/PasswordHasher.h"
#include "db/UserRepository.h"

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            std::cerr << "CHECK fallita: " #cond " (riga " << __LINE__ \
                      << ")\n";                                        \
            return 1;                                                  \
        }                                                              \
    } while (0)

// Smoke test dei repository contro una PostgreSQL reale.
// Richiede la variabile d'ambiente DATABASE_URL (es. postgresql://...).
int main() {
    const char* url = std::getenv("DATABASE_URL");
    if (url == nullptr) {
        std::cerr << "DATABASE_URL non impostata\n";
        return 2;
    }

    auto pool = std::make_shared<db::ConnectionPool>(url, 4);
    db::UserRepository users(pool);
    db::EventRepository events(pool);

    // 1. Utente: create + find + verifica password (bcrypt).
    const std::string name = "smoke_user";
    const std::string password = "s3cret!";
    if (users.create(name, db::PasswordHasher::hash(password))) {
        std::cout << "utente creato\n";
    }
    auto user = users.findByName(name);
    CHECK(user.has_value());
    CHECK(db::PasswordHasher::verify(password, user->passwordHash));
    CHECK(!users.create(name, db::PasswordHasher::hash(password))); // nome duplicato

    // 2. Evento singolo.
    db::EventRecord simple;
    simple.userId = user->id;
    simple.title = "Riunione";
    // Precisione al secondo, come il modello (il DB memorizza epoch seconds).
    simple.start = std::chrono::time_point_cast<std::chrono::seconds>(
        std::chrono::system_clock::now());
    simple.duration = std::chrono::hours(1);
    long long simpleId = events.createEvent(user->id, simple);
    CHECK(simpleId > 0);

    // 3. Evento ricorrente (FIXED) con un'eccezione.
    db::EventRecord rec;
    rec.userId = user->id;
    rec.title = "Settimanale";
    rec.start = simple.start;
    rec.duration = std::chrono::minutes(30);
    rec.kind = db::RecurrenceKind::Fixed;
    rec.interval = std::chrono::days(7);
    rec.end = simple.start + std::chrono::days(30);
    rec.exceptions.push_back(simple.start + std::chrono::days(7));
    long long recId = events.createEvent(user->id, rec);
    CHECK(recId > 0);

    // 4. Lettura: tutte le eccezioni e i campi di ricorrenza tornano indietro.
    auto all = events.getAllEvents(user->id);
    bool foundRec = false;
    bool foundSimple = false;
    for (const auto& e : all) {
        if (e.id == recId && e.kind == db::RecurrenceKind::Fixed) {
            CHECK(e.interval == std::chrono::days(7));
            CHECK(e.end.has_value());
            CHECK(e.exceptions.size() == 1);
            CHECK(e.exceptions[0] == rec.exceptions[0]);
            foundRec = true;
        }
        if (e.id == simpleId) {
            CHECK(e.duration == std::chrono::hours(1));
            foundSimple = true;
        }
    }
    CHECK(foundRec);
    CHECK(foundSimple);

    // 5. Proprietà, eccezione aggiunta/rimossa, eliminazione.
    CHECK(events.belongsToUser(simpleId, user->id));
    CHECK(!events.belongsToUser(simpleId, user->id + 1));
    CHECK(events.addException(simpleId, simple.start));
    CHECK(events.removeException(simpleId, simple.start));
    CHECK(events.deleteEvent(recId, user->id));
    CHECK(!events.belongsToUser(recId, user->id));

    // 6. Filtro temporale getEvents(userId, from, to).
    {
        auto from = simple.start - std::chrono::days(1);
        auto to = simple.start + std::chrono::days(1);
        auto inRange = events.getEvents(user->id, from, to);
        bool foundSimple = false;
        for (const auto& e : inRange) {
            if (e.id == simpleId) {
                foundSimple = true;
            }
        }
        CHECK(foundSimple);

        auto outRange = events.getEvents(user->id, simple.start + std::chrono::days(100),
                                         simple.start + std::chrono::days(101));
        bool foundSimpleOut = false;
        for (const auto& e : outRange) {
            if (e.id == simpleId) {
                foundSimpleOut = true;
            }
        }
        CHECK(!foundSimpleOut);
    }

    std::cout << "repo smoke OK\n";
    return 0;
}
