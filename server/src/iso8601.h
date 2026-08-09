#ifndef SERVER_ISO8601_H
#define SERVER_ISO8601_H

#include <string>

#include "events/core/CommonTypes.h"

namespace server {

/** @brief Converte una stringa ISO-8601 (YYYY-MM-DDTHH:MM:SS, UTC) in un istante.
 *  @param s Stringa da parsare
 *  @param out Istante risultante (precisione al secondo)
 *  @return true se la stringa è valida
 */
bool parseIso8601(const std::string& s, events::TimePoint& out);

/** @brief Formatta un istante in ISO-8601 (YYYY-MM-DDTHH:MM:SS, UTC).
 *  @param tp Istante da formattare
 *  @return Stringa ISO-8601
 */
std::string toIso8601(events::TimePoint tp);

} // namespace server

#endif // SERVER_ISO8601_H
