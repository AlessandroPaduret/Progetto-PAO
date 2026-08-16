#ifndef APP_MAINWINDOW_H
#define APP_MAINWINDOW_H

#include <QDate>
#include <QMainWindow>

#include <memory>
#include <optional>

#include "events/events.h"

class QLabel;
class QStackedWidget;

namespace app {

class CalendarController;
class ActivityDetailPage;
class ActivityFormDialog;
class ActivityListPage;
class RecurrenceChoiceDialog;
class WeekView;

/** @brief Finestra principale dell'applicazione: un'unica finestra con
 *  pagine navigabili (vincolo PAO: niente dialog per creazione/modifica):
 *
 *  0. Settimana   — WeekView in QScrollArea + barra di navigazione
 *  1. Elenco      — tabella con ricerca
 *  2. Dettaglio   — campi specifici per tipo (Visitor)
 *
 *  La creazione/modifica avviene in una finestra figlia ridotta
 *  (ActivityFormDialog), associata alla finestra principale.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(CalendarController* controller, QWidget* parent = nullptr);

private slots:
    void refresh();

    void showWeekPage();
    void showListPage();
    void showDetailPage(const events::Activity* activity);
    void showFormCreate(const QDateTime& start = QDateTime());
    void showFormEditActivity(const events::Activity* activity);
    void showFormEditOccurrence(const events::Occurrence& occurrence);

    void onPreviousWeek();
    void onNextWeek();
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

protected:
    /** @brief Tiene centrato il pannello di creazione quando si ridimensiona. */
    void resizeEvent(QResizeEvent* event) override;

private:
    void setWeekStart(const QDate& monday);
    QDate currentMonday() const;

    CalendarController* m_controller;
    QStackedWidget* m_pages = nullptr;
    WeekView* m_weekView = nullptr;
    ActivityListPage* m_listPage = nullptr;
    ActivityDetailPage* m_detailPage = nullptr;
    ActivityFormDialog* m_formDialog = nullptr;
    RecurrenceChoiceDialog* m_choiceDialog = nullptr;
    QLabel* m_weekLabel = nullptr;
    QDate m_monday;
    // Occorrenza "pendente" su cui l'utente deve scegliere serie o istanza
    std::optional<events::Occurrence> m_pendingOccurrence;
    QDateTime m_pendingDragTarget;
    bool m_pendingIsDrag = false;
};

} // namespace app

#endif // APP_MAINWINDOW_H
