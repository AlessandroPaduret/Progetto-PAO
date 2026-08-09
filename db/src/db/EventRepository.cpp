#include "db/EventRepository.h"

#include <unordered_map>
#include <utility>

#include <pqxx/pqxx>

namespace db {

namespace {
long long toEpochSeconds(std::chrono::system_clock::time_point tp) {
    return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch())
        .count();
}

std::chrono::system_clock::time_point fromEpochSeconds(long long seconds) {
    return std::chrono::system_clock::time_point(std::chrono::seconds(seconds));
}
} // namespace

EventRepository::EventRepository(std::shared_ptr<ConnectionPool> pool)
    : m_pool(std::move(pool)) {}

std::string EventRepository::kindToString(RecurrenceKind kind) {
    switch (kind) {
        case RecurrenceKind::Fixed:
            return "FIXED";
        case RecurrenceKind::Yearly:
            return "YEARLY";
        default:
            return "SINGLE";
    }
}

RecurrenceKind EventRepository::stringToKind(const std::string& s) {
    if (s == "FIXED") {
        return RecurrenceKind::Fixed;
    }
    if (s == "YEARLY") {
        return RecurrenceKind::Yearly;
    }
    return RecurrenceKind::Single;
}

long long EventRepository::createEvent(long long userId, const EventRecord& e) {
    auto lease = m_pool->acquire();
    pqxx::work tx(lease.get());

    std::optional<long long> endEpoch;
    if (e.end.has_value()) {
        endEpoch = toEpochSeconds(*e.end);
    }

    pqxx::result rows = tx.exec_params(
        "INSERT INTO eventi (utente_id, titolo, inizio, durata, tipo, intervallo, fine) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7) RETURNING id",
        userId,
        e.title,
        toEpochSeconds(e.start),
        e.duration.count(),
        kindToString(e.kind),
        e.kind == RecurrenceKind::Fixed ? e.interval.count() : 0,
        endEpoch);

    long long eventId = rows[0][0].as<long long>();

    for (const auto& ex : e.exceptions) {
        tx.exec_params("INSERT INTO eccezioni (evento_id, data) VALUES ($1, $2)",
                       eventId, toEpochSeconds(ex));
    }

    tx.commit();
    return eventId;
}

std::vector<EventRecord> EventRepository::getAllEvents(long long userId) {
    auto lease = m_pool->acquire();
    pqxx::work tx(lease.get());

    pqxx::result rows = tx.exec_params(
        "SELECT id, utente_id, titolo, inizio, durata, tipo, intervallo, fine "
        "FROM eventi WHERE utente_id = $1 ORDER BY inizio",
        userId);

    std::vector<EventRecord> events;
    events.reserve(rows.size());

    for (const auto& row : rows) {
        EventRecord e;
        e.id = row[0].as<long long>();
        e.userId = row[1].as<long long>();
        e.title = row[2].as<std::string>();
        e.start = fromEpochSeconds(row[3].as<long long>());
        e.duration = std::chrono::seconds(row[4].as<long long>());
        e.kind = stringToKind(row[5].as<std::string>());
        e.interval = std::chrono::seconds(row[6].as<long long>());
        if (row[7].is_null()) {
            e.end = std::nullopt;
        } else {
            e.end = fromEpochSeconds(row[7].as<long long>());
        }
        events.push_back(std::move(e));
    }

    attachExceptions(tx, events, userId);

    tx.commit();
    return events;
}

std::vector<EventRecord>
EventRepository::getEvents(long long userId,
                           std::chrono::system_clock::time_point from,
                           std::chrono::system_clock::time_point to) {
    auto lease = m_pool->acquire();
    pqxx::work tx(lease.get());

    // Un evento rientra nella timeline richiesta se:
    //  - Single: il suo inizio è in [from, to];
    //  - Fixed/Yearly: inizio <= to e (fine NULL = illimitata oppure fine >= from),
    //    cioè la ricorrenza può produrre occorrenze nel range. Il calcolo preciso
    //    delle occorrenze è delegato al modello (getSchedulable).
    pqxx::result rows = tx.exec_params(
        "SELECT id, utente_id, titolo, inizio, durata, tipo, intervallo, fine "
        "FROM eventi "
        "WHERE utente_id = $1 AND ("
        "  (tipo = 'SINGLE' AND inizio >= $2 AND inizio <= $3) OR "
        "  (tipo IN ('FIXED', 'YEARLY') AND inizio <= $3 "
        "   AND (fine IS NULL OR fine >= $2))"
        ") ORDER BY inizio",
        userId, toEpochSeconds(from), toEpochSeconds(to));

    std::vector<EventRecord> events;
    events.reserve(rows.size());

    for (const auto& row : rows) {
        EventRecord e;
        e.id = row[0].as<long long>();
        e.userId = row[1].as<long long>();
        e.title = row[2].as<std::string>();
        e.start = fromEpochSeconds(row[3].as<long long>());
        e.duration = std::chrono::seconds(row[4].as<long long>());
        e.kind = stringToKind(row[5].as<std::string>());
        e.interval = std::chrono::seconds(row[6].as<long long>());
        if (row[7].is_null()) {
            e.end = std::nullopt;
        } else {
            e.end = fromEpochSeconds(row[7].as<long long>());
        }
        events.push_back(std::move(e));
    }

    attachExceptions(tx, events, userId);

    tx.commit();
    return events;
}

void EventRepository::attachExceptions(
    pqxx::work& tx, std::vector<EventRecord>& events, long long userId) {
    // Carica le eccezioni di tutti gli eventi dell'utente in una sola query.
    pqxx::result exRows = tx.exec_params(
        "SELECT ec.evento_id, ec.data "
        "FROM eccezioni ec JOIN eventi ev ON ev.id = ec.evento_id "
        "WHERE ev.utente_id = $1 ORDER BY ec.data",
        userId);

    std::unordered_map<long long,
                       std::vector<std::chrono::system_clock::time_point>>
        byEvent;
    for (const auto& row : exRows) {
        long long eventId = row[0].as<long long>();
        byEvent[eventId].push_back(fromEpochSeconds(row[1].as<long long>()));
    }

    for (auto& e : events) {
        auto it = byEvent.find(e.id);
        if (it != byEvent.end()) {
            e.exceptions = it->second;
        }
    }
}

bool EventRepository::addException(long long eventId,
                                   std::chrono::system_clock::time_point date) {
    auto lease = m_pool->acquire();
    pqxx::work tx(lease.get());

    pqxx::result rows = tx.exec_params(
        "INSERT INTO eccezioni (evento_id, data) VALUES ($1, $2) "
        "ON CONFLICT DO NOTHING RETURNING evento_id",
        eventId, toEpochSeconds(date));

    bool added = !rows.empty();
    tx.commit();
    return added;
}

bool EventRepository::removeException(long long eventId,
                                      std::chrono::system_clock::time_point date) {
    auto lease = m_pool->acquire();
    pqxx::work tx(lease.get());

    pqxx::result rows = tx.exec_params(
        "DELETE FROM eccezioni WHERE evento_id = $1 AND data = $2",
        eventId, toEpochSeconds(date));

    bool removed = rows.affected_rows() > 0;
    tx.commit();
    return removed;
}

bool EventRepository::setRecurrenceEnd(long long eventId, long long userId,
                                       std::chrono::system_clock::time_point end) {
    auto lease = m_pool->acquire();
    pqxx::work tx(lease.get());

    pqxx::result rows = tx.exec_params(
        "UPDATE eventi SET fine = $3 "
        "WHERE id = $1 AND utente_id = $2 AND tipo != 'SINGLE'",
        eventId, userId, toEpochSeconds(end));

    bool updated = rows.affected_rows() > 0;
    tx.commit();
    return updated;
}

bool EventRepository::deleteEvent(long long eventId, long long userId) {
    auto lease = m_pool->acquire();
    pqxx::work tx(lease.get());

    pqxx::result rows = tx.exec_params(
        "DELETE FROM eventi WHERE id = $1 AND utente_id = $2", eventId, userId);

    bool deleted = rows.affected_rows() > 0;
    tx.commit();
    return deleted;
}

bool EventRepository::belongsToUser(long long eventId, long long userId) {
    auto lease = m_pool->acquire();
    pqxx::work tx(lease.get());

    pqxx::result rows = tx.exec_params(
        "SELECT 1 FROM eventi WHERE id = $1 AND utente_id = $2", eventId, userId);

    bool owned = !rows.empty();
    tx.commit();
    return owned;
}

} // namespace db
