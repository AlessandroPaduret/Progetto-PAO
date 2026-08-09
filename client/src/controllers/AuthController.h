#ifndef CLIENT_AUTHCONTROLLER_H
#define CLIENT_AUTHCONTROLLER_H

#include <QObject>
#include <QString>

namespace client {

class ApiClient;

/** @brief Gestisce login e registrazione; tiene il token JWT in memoria. */
class AuthController : public QObject {
    Q_OBJECT
public:
    explicit AuthController(ApiClient* api, QObject* parent = nullptr);

    /** @brief Tenta il login; emette authenticated() in caso di successo. */
    void login(const QString& username, const QString& password);

    /** @brief Registra l'utente e poi esegue il login automatico. */
    void registerAndLogin(const QString& username, const QString& password);

    const QString& token() const;

signals:
    void authenticated();
    void authFailed(const QString& error);

private:
    ApiClient* m_api;
    QString m_token;
};

} // namespace client

#endif // CLIENT_AUTHCONTROLLER_H
