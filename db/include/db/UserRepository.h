#ifndef DB_USER_REPOSITORY_H
#define DB_USER_REPOSITORY_H

#include <memory>
#include <optional>
#include <string>

#include "db/ConnectionPool.h"

namespace db {

/** @brief Utente persistito (tabella `utenti`). */
struct User {
    long long id = 0;      /**< id dell'utente nel DB (0 se non persistito) */
    std::string name;      /**< nome utente (univoco) */
    std::string passwordHash; /**< hash bcrypt della password */
};

/** @brief Repository degli utenti su PostgreSQL. */
class UserRepository {
public:
    /** @brief Costruttore.
     *  @param pool Pool di connessioni da cui acquisire le transazioni
     */
    explicit UserRepository(std::shared_ptr<ConnectionPool> pool);

    /** @brief Cerca un utente per nome.
     *  @param name Nome utente da cercare
     *  @return L'utente se esiste, altrimenti `std::nullopt`
     */
    std::optional<User> findByName(const std::string& name);

    /** @brief Inserisce un nuovo utente.
     *  @param name Nome utente (deve essere univoco)
     *  @param passwordHash Hash bcrypt della password (vedi PasswordHasher)
     *  @return true se l'utente è stato creato, false se il nome esiste già
     */
    bool create(const std::string& name, const std::string& passwordHash);

private:
    std::shared_ptr<ConnectionPool> m_pool; /**< pool di connessioni */
};

} // namespace db

#endif // DB_USER_REPOSITORY_H
