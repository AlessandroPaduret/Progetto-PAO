#ifndef CLIENT_APICONFIG_H
#define CLIENT_APICONFIG_H

#include <QUrl>

namespace client {

/** @brief URL base dell'API server.
 *
 *  Letta dalla variabile d'ambiente API_URL; default http://localhost:8080.
 */
QUrl apiBaseUrl();

} // namespace client

#endif // CLIENT_APICONFIG_H
