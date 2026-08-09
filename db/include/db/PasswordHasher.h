#ifndef DB_PASSWORD_HASHER_H
#define DB_PASSWORD_HASHER_H

#include <string>

namespace db {

/** @brief Hash delle password con l'algoritmo bcrypt (schema $2b$, costo 12).
 *
 *  Ogni hash è salato: hash della stessa password generati in momenti
 *  diversi sono diversi. L'implementazione usa libxcrypt (crypt_r);
 *  il salt è generato internamente (16 byte casuali codificati in base64).
 */
class PasswordHasher {
public:
    /** @brief Calcola l'hash bcrypt della password.
     *  @param password Password in chiaro
     *  @return Stringa di hash nel formato `$2b$<costo>$<salt+hash>`
     *  @throws std::runtime_error se la generazione dell'hash fallisce
     */
    static std::string hash(const std::string& password);

    /** @brief Verifica una password contro un hash bcrypt.
     *  @param password Password in chiaro da verificare
     *  @param bcryptHash Hash bcrypt di riferimento
     *  @return true se la password corrisponde all'hash, false altrimenti
     */
    static bool verify(const std::string& password, const std::string& bcryptHash);
};

} // namespace db

#endif // DB_PASSWORD_HASHER_H
