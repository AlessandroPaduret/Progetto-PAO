#include "views/LoginDialog.h"

#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "controllers/AuthController.h"

namespace client {

LoginDialog::LoginDialog(AuthController* auth, QWidget* parent)
    : QDialog(parent), m_auth(auth) {
    setWindowTitle(tr("Accesso - Calendario"));
    setModal(true);

    m_username = new QLineEdit(this);
    m_username->setPlaceholderText(tr("Nome utente"));
    m_password = new QLineEdit(this);
    m_password->setPlaceholderText(tr("Password"));
    m_password->setEchoMode(QLineEdit::Password);

    m_submitButton = new QPushButton(tr("Accedi"), this);
    m_toggleButton = new QPushButton(tr("Non hai un account? Registrati"), this);
    m_status = new QLabel(this);
    m_status->setWordWrap(true);
    m_status->setStyleSheet(QStringLiteral("color: red;"));

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_username);
    layout->addWidget(m_password);
    layout->addWidget(m_submitButton);
    layout->addWidget(m_toggleButton);
    layout->addWidget(m_status);

    connect(m_submitButton, &QPushButton::clicked, this, &LoginDialog::onSubmit);
    connect(m_toggleButton, &QPushButton::clicked, this, [this]() {
        m_registerMode = !m_registerMode;
        m_submitButton->setText(m_registerMode ? tr("Registrati") : tr("Accedi"));
        m_toggleButton->setText(m_registerMode
                                    ? tr("Hai già un account? Accedi")
                                    : tr("Non hai un account? Registrati"));
        m_status->clear();
    });
    connect(m_auth, &AuthController::authenticated, this,
            [this]() { emit authenticated(); accept(); });
    connect(m_auth, &AuthController::authFailed, this,
            &LoginDialog::onAuthFailed);

    connect(m_password, &QLineEdit::returnPressed, this, &LoginDialog::onSubmit);
}

void LoginDialog::onSubmit() {
    const QString username = m_username->text().trimmed();
    const QString password = m_password->text();
    if (username.isEmpty() || password.isEmpty()) {
        m_status->setText(tr("Inserisci nome utente e password."));
        return;
    }
    m_status->clear();
    if (m_registerMode) {
        m_auth->registerAndLogin(username, password);
    } else {
        m_auth->login(username, password);
    }
}

void LoginDialog::onAuthFailed(const QString& error) {
    m_status->setText(error);
}

} // namespace client
