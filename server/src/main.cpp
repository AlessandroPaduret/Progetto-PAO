#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "db/ConnectionPool.h"
#include "db/EventRepository.h"
#include "db/PasswordHasher.h"
#include "db/UserRepository.h"

#include "auth.h"
#include "mappers.h"
#include "iso8601.h"

using json = nlohmann::json;
using namespace server;

namespace {

json errorJson(const std::string& message) {
    return json{{"error", message}};
}

bool parseBody(const httplib::Request& req, json& out) {
    try {
        out = json::parse(req.body);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::string getEnv(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    return value != nullptr ? std::string(value) : fallback;
}

} // namespace

int main() {
    std::string connString =
        getEnv("DATABASE_URL", "postgresql://events:events@db:5432/events");
    std::string jwtSecret = getEnv("JWT_SECRET", "dev-secret");
    int port = std::atoi(getEnv("PORT", "8080").c_str());

    auto pool = std::make_shared<db::ConnectionPool>(connString, 8);
    db::UserRepository users(pool);
    db::EventRepository events(pool);

    httplib::Server svr;

    auto requireAuth = [&](const httplib::Request& req, httplib::Response& res,
                           long long& userId) {
        std::string header = req.get_header_value("Authorization");
        const std::string prefix = "Bearer ";
        if (header.rfind(prefix, 0) != 0 ||
            !verifyToken(header.substr(prefix.size()), jwtSecret, userId)) {
            res.status = 401;
            res.set_content(errorJson("token mancante o non valido").dump(),
                            "application/json");
            return false;
        }
        return true;
    };

    // POST /api/register — crea un nuovo utente
    svr.Post("/api/register", [&](const httplib::Request& req,
                                   httplib::Response& res) {
        json body;
        if (!parseBody(req, body)) {
            res.status = 400;
            res.set_content(errorJson("body JSON non valido").dump(),
                            "application/json");
            return;
        }

        std::string username = body.value("username", "");
        std::string password = body.value("password", "");
        if (username.empty() || password.empty()) {
            res.status = 400;
            res.set_content(
                errorJson("username e password obbligatori").dump(),
                "application/json");
            return;
        }

        if (!users.create(username, db::PasswordHasher::hash(password))) {
            res.status = 409;
            res.set_content(errorJson("nome utente già esistente").dump(),
                            "application/json");
            return;
        }

        auto user = users.findByName(username);
        res.status = 201;
        res.set_content(json{{"id", user->id}}.dump(), "application/json");
    });

    // POST /api/login — emette il token JWT
    svr.Post("/api/login", [&](const httplib::Request& req,
                                httplib::Response& res) {
        json body;
        if (!parseBody(req, body)) {
            res.status = 400;
            res.set_content(errorJson("body JSON non valido").dump(),
                            "application/json");
            return;
        }

        std::string username = body.value("username", "");
        std::string password = body.value("password", "");
        auto user = users.findByName(username);
        if (!user.has_value() ||
            !db::PasswordHasher::verify(password, user->passwordHash)) {
            res.status = 401;
            res.set_content(errorJson("credenziali non valide").dump(),
                            "application/json");
            return;
        }

        res.set_content(
            json{{"token", createToken(user->id, jwtSecret)}}.dump(),
            "application/json");
    });

    // GET /api/events?from=&to= — occorrenze nel range (ISO-8601)
    svr.Get("/api/events", [&](const httplib::Request& req,
                                httplib::Response& res) {
        long long userId = 0;
        if (!requireAuth(req, res, userId)) {
            return;
        }

        std::string fromStr = req.get_param_value("from");
        std::string toStr = req.get_param_value("to");
        events::TimePoint from, to;
        if (fromStr.empty() || toStr.empty() ||
            !parseIso8601(fromStr, from) || !parseIso8601(toStr, to)) {
            res.status = 400;
            res.set_content(
                errorJson("parametri from e to obbligatori (ISO-8601)").dump(),
                "application/json");
            return;
        }

        auto records = events.getEvents(userId, from, to);
        std::vector<json> occurrences;
        for (const auto& record : records) {
            if (record.kind == db::RecurrenceKind::Single) {
                auto event = toSimpleEvent(record);
                occurrences.push_back(occurrenceToJson(record.id, *event));
            } else {
                auto recurrent = toRecurrentEvent(record);
                for (auto& occurrence : recurrent->getSchedulable(from, to)) {
                    occurrences.push_back(
                        occurrenceToJson(record.id, *occurrence));
                }
            }
        }
        std::sort(occurrences.begin(), occurrences.end(),
                  [](const json& a, const json& b) {
                      return a["start"] < b["start"];
                  });

        res.set_content(json(occurrences).dump(), "application/json");
    });

    // POST /api/create-event — crea un evento semplice o ricorrente
    svr.Post("/api/create-event", [&](const httplib::Request& req,
                                      httplib::Response& res) {
        long long userId = 0;
        if (!requireAuth(req, res, userId)) {
            return;
        }

        json body;
        if (!parseBody(req, body)) {
            res.status = 400;
            res.set_content(errorJson("body JSON non valido").dump(),
                            "application/json");
            return;
        }

        db::EventRecord record;
        record.userId = userId;
        record.title = body.value("title", "");
        if (record.title.empty()) {
            res.status = 400;
            res.set_content(errorJson("titolo obbligatorio").dump(),
                            "application/json");
            return;
        }

        events::TimePoint start;
        if (!body.contains("start") ||
            !parseIso8601(body["start"].get<std::string>(), start)) {
            res.status = 400;
            res.set_content(
                errorJson("start obbligatorio (ISO-8601)").dump(),
                "application/json");
            return;
        }
        record.start = start;

        if (!body.contains("duration") ||
            !body["duration"].is_number_integer() ||
            body["duration"].get<long long>() < 0) {
            res.status = 400;
            res.set_content(
                errorJson("duration obbligatoria (secondi, >= 0)").dump(),
                "application/json");
            return;
        }
        record.duration = std::chrono::seconds(body["duration"].get<long long>());

        std::string type = body.value("type", "single");
        if (type == "fixed") {
            record.kind = db::RecurrenceKind::Fixed;
            if (!body.contains("interval") ||
                body["interval"].get<long long>() <= 0) {
                res.status = 400;
                res.set_content(
                    errorJson("interval obbligatorio per type=fixed (secondi, > 0)")
                        .dump(),
                    "application/json");
                return;
            }
            record.interval = std::chrono::seconds(body["interval"].get<long long>());
        } else if (type == "yearly") {
            record.kind = db::RecurrenceKind::Yearly;
        } else if (type != "single") {
            res.status = 400;
            res.set_content(
                errorJson("type deve essere single, fixed o yearly").dump(),
                "application/json");
            return;
        }

        if (body.contains("end")) {
            events::TimePoint end;
            if (!parseIso8601(body["end"].get<std::string>(), end)) {
                res.status = 400;
                res.set_content(errorJson("end non valido (ISO-8601)").dump(),
                                "application/json");
                return;
            }
            record.end = end;
        }

        if (body.contains("exceptions")) {
            if (!body["exceptions"].is_array()) {
                res.status = 400;
                res.set_content(errorJson("exceptions deve essere un array").dump(),
                                "application/json");
                return;
            }
            for (const auto& ex : body["exceptions"]) {
                events::TimePoint tp;
                if (!parseIso8601(ex.get<std::string>(), tp)) {
                    res.status = 400;
                    res.set_content(
                        errorJson("eccezione non valida (ISO-8601)").dump(),
                        "application/json");
                    return;
                }
                record.exceptions.push_back(tp);
            }
        }

        long long id = events.createEvent(userId, record);
        res.status = 201;
        res.set_content(json{{"id", id}}.dump(), "application/json");
    });

    // DELETE /api/events/{id} — elimina l'evento o aggiunge un'eccezione
    svr.Delete(R"(/api/events/(\d+))",
               [&](const httplib::Request& req, httplib::Response& res) {
        long long userId = 0;
        if (!requireAuth(req, res, userId)) {
            return;
        }

        long long eventId = std::stoll(req.matches[1].str());
        if (!events.belongsToUser(eventId, userId)) {
            res.status = 404;
            res.set_content(errorJson("evento non trovato").dump(),
                            "application/json");
            return;
        }

        if (!req.body.empty()) {
            json body;
            if (!parseBody(req, body)) {
                res.status = 400;
                res.set_content(errorJson("body JSON non valido").dump(),
                                "application/json");
                return;
            }
            if (body.contains("exception")) {
                events::TimePoint tp;
                if (!parseIso8601(body["exception"].get<std::string>(), tp)) {
                    res.status = 400;
                    res.set_content(
                        errorJson("exception non valida (ISO-8601)").dump(),
                        "application/json");
                    return;
                }
                events.addException(eventId, tp);
                res.set_content(json{{"ok", true}}.dump(), "application/json");
                return;
            }
        }

        events.deleteEvent(eventId, userId);
        res.set_content(json{{"ok", true}}.dump(), "application/json");
    });

    std::cout << "api_server in ascolto sulla porta " << port << "\n";
    if (!svr.listen("0.0.0.0", port)) {
        std::cerr << "Errore: impossibile avviare il server sulla porta "
                  << port << "\n";
        return 1;
    }
    return 0;
}
