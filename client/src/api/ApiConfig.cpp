#include "api/ApiConfig.h"

#include <QProcessEnvironment>

namespace client {

QUrl apiBaseUrl() {
    const QString env = QProcessEnvironment::systemEnvironment().value(
        QStringLiteral("API_URL"));
    if (!env.isEmpty()) {
        return QUrl(env);
    }
    return QUrl(QStringLiteral("http://localhost:8080"));
}

} // namespace client
