#ifndef CLIENT_LOGINDIALOG_H
#define CLIENT_LOGINDIALOG_H

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;

namespace client {

class AuthController;

/** @brief Finestra di login con possibilità di registrarsi. */
class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(AuthController* auth, QWidget* parent = nullptr);

signals:
    void authenticated();

private slots:
    void onSubmit();

private:
    void onAuthFailed(const QString& error);

    AuthController* m_auth;
    QLineEdit* m_username;
    QLineEdit* m_password;
    QPushButton* m_submitButton;
    QPushButton* m_toggleButton;
    QLabel* m_status;
    bool m_registerMode = false;
};

} // namespace client

#endif // CLIENT_LOGINDIALOG_H
