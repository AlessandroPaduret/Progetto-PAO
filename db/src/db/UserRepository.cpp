#include "db/UserRepository.h"

#include <utility>

#include <pqxx/pqxx>

namespace db {

UserRepository::UserRepository(std::shared_ptr<ConnectionPool> pool)
    : m_pool(std::move(pool)) {}

std::optional<User> UserRepository::findByName(const std::string& name) {
    auto lease = m_pool->acquire();
    pqxx::work tx(lease.get());

    pqxx::result rows = tx.exec_params(
        "SELECT id, nome, hash_password FROM utenti WHERE nome = $1", name);

    if (rows.empty()) {
        tx.commit();
        return std::nullopt;
    }

    User user;
    user.id = rows[0][0].as<long long>();
    user.name = rows[0][1].as<std::string>();
    user.passwordHash = rows[0][2].as<std::string>();

    tx.commit();
    return user;
}

bool UserRepository::create(const std::string& name,
                            const std::string& passwordHash) {
    auto lease = m_pool->acquire();
    pqxx::work tx(lease.get());

    pqxx::result rows = tx.exec_params(
        "INSERT INTO utenti (nome, hash_password) VALUES ($1, $2) "
        "ON CONFLICT (nome) DO NOTHING RETURNING id",
        name, passwordHash);

    bool created = !rows.empty();
    tx.commit();
    return created;
}

} // namespace db
