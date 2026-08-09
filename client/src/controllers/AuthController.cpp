#include "controllers/AuthController.h"

#include "api/ApiClient.h"

namespace client {

AuthController::AuthController(ApiClient* api, QObject* parent)
    : QObject(parent), m_api(api) {}

const QString& AuthController::token() const {
    return m_token;
}

void AuthController::login(const QString& username, const QString& password) {
    connect(m_api, &ApiClient::loginSucceeded, this,
            [this](const QString& token) {
                m_token = token;
                emit authenticated();
            },
            Qt::SingleShotConnection);
    connect(m_api, &ApiClient::loginFailed, this,
            &AuthController::authFailed, Qt::SingleShotConnection);

    m_api->login(username, password);
}

void AuthController::registerAndLogin(const QString& username,
                                      const QString& password) {
    connect(m_api, &ApiClient::registerSucceeded, this,
            [this, username, password]() { login(username, password); },
            Qt::SingleShotConnection);
    connect(m_api, &ApiClient::registerFailed, this,
            &AuthController::authFailed, Qt::SingleShotConnection);

    m_api->registerUser(username, password);
}

} // namespace client
