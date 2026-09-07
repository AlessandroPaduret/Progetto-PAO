#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QStyleFactory>
#include <QTextStream>

#include "controller/CalendarController.h"
#include "views/MainWindow.h"
#include "views/utils/Theme.h"

namespace {

/** @brief Applica la palette scura unica dell'applicazione. Lo stile Fusion
 *  (invece di quello nativo della piattaforma) e' necessario perche' la
 *  QPalette venga rispettata davvero da ogni widget: alcuni stili nativi
 *  (es. quelli legati al tema GTK/Windows del sistema) ne ignorano parti,
 *  il che produceva un'interfaccia con toni scuri e chiari mescolati a
 *  seconda del tema del sistema ospite invece di un dark mode coerente. */
void applyDarkPalette(QApplication& application) {
    application.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    QPalette palette;
    palette.setColor(QPalette::Window, app::theme::kWindowBackground);
    palette.setColor(QPalette::WindowText, app::theme::kPrimaryText);
    palette.setColor(QPalette::Base, app::theme::kWindowBackground);
    palette.setColor(QPalette::AlternateBase, app::theme::kPanelBackground);
    palette.setColor(QPalette::ToolTipBase, app::theme::kPanelBackground);
    palette.setColor(QPalette::ToolTipText, app::theme::kPrimaryText);
    palette.setColor(QPalette::Text, app::theme::kPrimaryText);
    palette.setColor(QPalette::Button, app::theme::kPanelBackground);
    palette.setColor(QPalette::ButtonText, app::theme::kPrimaryText);
    palette.setColor(QPalette::PlaceholderText, app::theme::kMutedText);
    palette.setColor(QPalette::Link, app::theme::kAccentBlue);
    palette.setColor(QPalette::Highlight, app::theme::kAccentBlue);
    palette.setColor(QPalette::HighlightedText, app::theme::kWindowBackground);
    palette.setColor(QPalette::Disabled, QPalette::Text, app::theme::kMutedText);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, app::theme::kMutedText);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, app::theme::kMutedText);
    application.setPalette(palette);
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("Attivita"));
    application.setOrganizationName(QStringLiteral("pao"));

    applyDarkPalette(application);

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
