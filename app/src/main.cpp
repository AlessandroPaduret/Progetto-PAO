#include <QApplication>

#include "CalendarController.h"
#include "views/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("Attivita"));
    application.setOrganizationName(QStringLiteral("pao"));

    app::CalendarController controller;
    app::MainWindow window(&controller);
    window.show();
    return application.exec();
}
