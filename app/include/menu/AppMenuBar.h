#pragma once

#include <QMenuBar>

#include <memory>

class QAction;
class QActionGroup;

namespace app {

class MenuShortcutStyle;

/** @brief Barra dei menu dell'applicazione: costruisce da sola tutti i menu
 *  (File/Visualizza/Nuova attivita'), le relative QAction, le scorciatoie
 *  e lo stile delle scorciatoie (MenuShortcutStyle) — la MainWindow non
 *  contiene piu' nessuna logica di costruzione dei menu, solo le
 *  connessioni ai segnali qui sotto e i propri gestori (onSave/onLoad/...).
 *
 *  La MainWindow resta l'unica proprietaria dello stato applicativo (file
 *  corrente, vista attiva): questa classe si limita a segnalare le
 *  richieste dell'utente (segnali) e a riflettere lo stato scelto
 *  dall'esterno (setActiveView), senza mai toccare direttamente pagine o
 *  controller. */
class AppMenuBar : public QMenuBar {
    Q_OBJECT
public:
    enum class ViewKind { List, Day, Week, Month, Year };

    explicit AppMenuBar(QWidget* parent = nullptr);
    ~AppMenuBar() override;

    /** @brief Spunta la voce del menu "Visualizza" corrispondente alla
     *  vista indicata (senza emettere viewSelected). Va chiamata dalla
     *  MainWindow ogni volta che la vista cambia, anche quando il cambio
     *  non parte dal menu (es. doppio clic su un giorno in YearView). */
    void setActiveView(ViewKind kind);

signals:
    void saveRequested();
    void saveAsRequested();
    void loadRequested();
    /** @brief Una delle 5 voci del menu "Visualizza". */
    void viewSelected(ViewKind kind);
    /** @brief Menu "Nuova attivita'" o scorciatoia Ctrl+N (0=Evento,
     *  1=Riunione, 2=Compito, 3=Anniversario). */
    void newActivityRequested(int typeIndex);

private:
    QAction* m_viewActions[5] = {};  // indicizzate come ViewKind
    QActionGroup* m_viewGroup = nullptr;
    std::unique_ptr<MenuShortcutStyle> m_menuStyle;
};

} // namespace app
