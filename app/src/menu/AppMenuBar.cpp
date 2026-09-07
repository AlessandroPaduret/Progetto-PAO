#include "menu/AppMenuBar.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QKeySequence>
#include <QMenu>
#include <QShortcut>

#include "menu/MenuShortcutStyle.h"

namespace app {

AppMenuBar::AppMenuBar(QWidget* parent) : QMenuBar(parent) {
    // Menu "File": Salva / Salva con nome / Carica calendario (con scorciatoie)
    QMenu* fileMenu = addMenu(tr("File"));
    QAction* saveAction = fileMenu->addAction(tr("Salva calendario"), this,
                                              &AppMenuBar::saveRequested);
    saveAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
    QAction* saveAsAction = fileMenu->addAction(tr("Salva con nome"), this,
                                                &AppMenuBar::saveAsRequested);
    saveAsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    fileMenu->addSeparator();
    QAction* loadAction = fileMenu->addAction(tr("Carica calendario"), this,
                                              &AppMenuBar::loadRequested);
    loadAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));

    // Stile del menu "File": le scorciatoie a destra, in grigio marcato.
    m_menuStyle = std::make_unique<MenuShortcutStyle>(QApplication::style());
    fileMenu->setStyle(m_menuStyle.get());

    // Menu "Visualizza": le 5 viste (l'azione attiva resta spuntata)
    QMenu* viewMenu = addMenu(tr("Visualizza"));
    struct Entry { ViewKind kind; QString label; };
    const Entry entries[] = {
        {ViewKind::List, tr("Elenco")},   {ViewKind::Day, tr("Giorno")},
        {ViewKind::Week, tr("Settimana")}, {ViewKind::Month, tr("Mese")},
        {ViewKind::Year, tr("Anno")},
    };
    m_viewGroup = new QActionGroup(this);
    for (const Entry& entry : entries) {
        QAction* action = viewMenu->addAction(entry.label, this,
                                              [this, kind = entry.kind] {
                                                  emit viewSelected(kind);
                                              });
        action->setCheckable(true);
        m_viewGroup->addAction(action);
        m_viewActions[static_cast<int>(entry.kind)] = action;
    }

    // Menu "Nuova attivita'": i tipi creabili dal form
    QMenu* newMenu = addMenu(tr("Nuova attivita'"));
    const char* typeLabels[] = {"Evento", "Riunione", "Compito", "Anniversario"};
    for (int type = 0; type < 4; ++type) {
        newMenu->addAction(tr(typeLabels[type]), this,
                           [this, type] { emit newActivityRequested(type); });
    }

    // Ctrl+N: nuova attivita' (form con tipo predefinito Evento), stessa
    // richiesta della prima voce del menu "Nuova attivita'"
    auto* newShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_N), this);
    connect(newShortcut, &QShortcut::activated, this,
            [this] { emit newActivityRequested(0); });
}

AppMenuBar::~AppMenuBar() = default;

void AppMenuBar::setActiveView(ViewKind kind) {
    if (QAction* action = m_viewActions[static_cast<int>(kind)]) {
        action->setChecked(true);
    }
}

} // namespace app
