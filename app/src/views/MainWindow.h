#ifndef APP_MAINWINDOW_H
#define APP_MAINWINDOW_H

#include <QDate>
#include <QMainWindow>

#include <memory>
#include <optional>

#include "events/events.h"

class QAction;
class QEvent;
class QLabel;
class QMenu;
class QStackedWidget;
class QToolButton;

namespace app {

class ActivityDetailDialog;
class ActivityFormDialog;
class ActivityListPage;
class CalendarController;
class DayView;
class MonthView;
class RecurrenceChoiceDialog;
class WeekView;
class YearView;

/** @brief Finestra principale dell'applicazione: un'unica finestra con
 *  pagine navigabili (vincolo PAO: niente dialog per creazione/modifica).
 *
 *  Le viste temporali (giorno/settimana/mese/anno) condividono la barra di
 *  navigazione (Oggi / <- / ->) e vengono scelte dal tasto "Visualizza"
 *  della barra degli strumenti (tendina con Elenco/Giorno/Settimana/Mese/
 *  Anno, che si apre anche al passaggio del puntatore):
 *
 *  0. Settimana   — WeekView in QScrollArea
 *  1. Elenco      — tabella con ricerca e filtro per tipo
 *  2. Giorno      — DayView (griglia a colonna singola)
 *  3. Mese        — MonthView (griglia mensile con chip)
 *  4. Anno        — YearView (12 mini-calendari)
 *
 *  Il dettaglio di un'attivita' si apre in una finestra figlia ridotta
 *  (ActivityDetailDialog), come la creazione/modifica (ActivityFormDialog):
 *  widget DENTRO la MainWindow, mai a schermo intero.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(CalendarController* controller, QWidget* parent = nullptr);

private slots:
    void refresh();

    void showDayPage();
    void showWeekPage();
    void showMonthPage();
    void showYearPage();
    void showListPage();
    void showDetailDialog(const events::Activity* activity);
    void showFormCreate(const QDateTime& start = QDateTime());
    void showFormEditActivity(const events::Activity* activity);
    void showFormEditOccurrence(const events::Occurrence& occurrence);

    void onPrevious();
    void onNext();
    void onToday();

    void onNewActivity();
    void onEditSelected();
    void onDeleteSelected();

    void onSave();
    void onLoad();

    void confirmDeleteOccurrence(const events::Occurrence& occurrence);

    /** @brief Mostra la finestra di scelta serie/singola occorrenza. */
    void askSeriesOrInstance(const events::Occurrence& occurrence);
    /** @brief Mostra la finestra di scelta serie/singola occorrenza (drag). */
    void askSeriesOrInstanceDrag(const events::Occurrence& occurrence,
                                 const QDateTime& newStart);
    void onChoiceSeries();
    void onChoiceInstance();
    /** @brief "Da questo momento in poi": divide la serie (ferma l'attuale,
     *  ne crea una nuova con le stesse regole e inizio diverso). */
    void onChoiceSplit();

protected:
    /** @brief Apre la tendina "Visualizza" al passaggio del puntatore. */
    bool eventFilter(QObject* object, QEvent* event) override;
    /** @brief Tiene centrato il pannello di creazione quando si ridimensiona. */
    void resizeEvent(QResizeEvent* event) override;

private:
    enum class ViewKind { Day, Week, Month, Year };

    /** @brief Imposta il riferimento temporale (normalizzato per la vista
     *  corrente: lunedi' per la settimana, 1 del mese, 1 gennaio) e
     *  aggiorna le viste. */
    void setAnchor(const QDate& anchor);

    /** @brief Lunedi' della settimana che contiene la data indicata. */
    static QDate mondayOf(const QDate& date);

    CalendarController* m_controller;
    QStackedWidget* m_pages = nullptr;
    WeekView* m_weekView = nullptr;
    DayView* m_dayView = nullptr;
    MonthView* m_monthView = nullptr;
    YearView* m_yearView = nullptr;
    ActivityListPage* m_listPage = nullptr;
    ActivityDetailDialog* m_detailDialog = nullptr;
    ActivityFormDialog* m_formDialog = nullptr;
    RecurrenceChoiceDialog* m_choiceDialog = nullptr;

    QWidget* m_navBar = nullptr;
    QLabel* m_navLabel = nullptr;
    QToolButton* m_viewButton = nullptr;
    QMenu* m_viewMenu = nullptr;
    QAction* m_viewListAction = nullptr;
    QAction* m_viewDayAction = nullptr;
    QAction* m_viewWeekAction = nullptr;
    QAction* m_viewMonthAction = nullptr;
    QAction* m_viewYearAction = nullptr;

    QDate m_anchor;
    ViewKind m_view = ViewKind::Week;
    // Occorrenza "pendente" su cui l'utente deve scegliere serie o istanza
    std::optional<events::Occurrence> m_pendingOccurrence;
    QDateTime m_pendingDragTarget;
    bool m_pendingIsDrag = false;
};

} // namespace app

#endif // APP_MAINWINDOW_H
