#ifndef SERVER_AUTH_H
#define SERVER_AUTH_H

#include <string>

namespace server {

/** @brief Crea un token JWT (HS256) con soggetto = id utente e scadenza 24h.
 *  @param userId Id dell'utente autenticato
 *  @param secret Segreto di firma (JWT_SECRET)
 *  @return Token JWT firmato
 */
std::string createToken(long long userId, const std::string& secret);

/** @brief Verifica un token JWT ed estrae l'id utente.
 *  @param token Token da verificare
 *  @param secret Segreto di firma (JWT_SECRET)
 *  @param userId Id utente estratto (solo se il token è valido)
 *  @return true se firma valida e token non scaduto
 */
bool verifyToken(const std::string& token, const std::string& secret,
                 long long& userId);

} // namespace server

#endif // SERVER_AUTH_H
