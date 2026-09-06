#include <QApplication>
#include <QFile>
#include <QTextStream>

#include "CalendarController.h"
#include "views/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("Attivita"));
    application.setOrganizationName(QStringLiteral("pao"));

    // Aspetto dell'interfaccia: un unico foglio di stile Qt (resources/style.qss,
    // incorporato come risorsa) invece di stringhe CSS sparse nel codice.
    QFile styleFile(QStringLiteral(":/styles/style.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        application.setStyleSheet(QTextStream(&styleFile).readAll());
    }

    app::CalendarController controller;
    app::MainWindow window(&controller);
    window.show();
    return application.exec();
}
