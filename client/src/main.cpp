#include <QApplication>

#include "api/ApiClient.h"
#include "api/ApiConfig.h"
#include "controllers/AuthController.h"
#include "controllers/EventsController.h"
#include "views/LoginDialog.h"
#include "views/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    client::ApiClient api(client::apiBaseUrl());
    client::AuthController auth(&api);
    client::EventsController events(&api);

    client::LoginDialog login(&auth);
    client::MainWindow window(&events);

    QObject::connect(&login, &client::LoginDialog::authenticated, &window,
                     [&window]() {
                         window.show();
                         window.raise();
                         window.activateWindow();
                         window.refresh();
                     });

    login.show();
    return app.exec();
}
