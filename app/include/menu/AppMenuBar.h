#pragma once

#include <QMenuBar>

#include <memory>

class QAction;
class QActionGroup;

namespace app {

class MenuShortcutStyle;

/** @brief Costruisce da sola tutti i menu (File/Visualizza/Nuova attivita') e
 *  le scorciatoie; la MainWindow resta proprietaria dello stato (file
 *  corrente, vista attiva) e si limita ad ascoltare i segnali e a riflettere
 *  lo stato scelto con setActiveView. */
class AppMenuBar : public QMenuBar {
    Q_OBJECT
public:
    enum class ViewKind { List, Day, Week, Month, Year };

    explicit AppMenuBar(QWidget* parent = nullptr);
    ~AppMenuBar() override;

    /** @brief Spunta la voce senza emettere viewSelected; va chiamata anche
     *  quando il cambio vista non parte dal menu (es. doppio clic su un
     *  giorno in YearView). */
    void setActiveView(ViewKind kind);

signals:
    void saveRequested();
    void saveAsRequested();
    void loadRequested();
    void viewSelected(ViewKind kind);
    /** @brief Da menu "Nuova attivita'" o Ctrl+N (0=Evento, 1=Riunione, 2=Compito). */
    void newActivityRequested(int typeIndex);

private:
    QAction* m_viewActions[5] = {};  // indicizzate come ViewKind
    QActionGroup* m_viewGroup = nullptr;
    std::unique_ptr<MenuShortcutStyle> m_menuStyle;
};

} // namespace app
