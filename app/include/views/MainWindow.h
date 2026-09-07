#pragma once

#include <QDate>
#include <QMainWindow>
#include <QString>

#include <optional>

#include "events.h"
#include "menu/AppMenuBar.h"

class QStackedWidget;

namespace app {

class ActivityDetailDialog;
class ActivityFormDialog;
class ActivityListPage;
class CalendarController;
class DayView;
class MonthView;
class NavigationBar;
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
 *  0. Settimana   — WeekView (scroll verticale interno alla griglia oraria)
 *  1. Elenco      — tabella con ricerca e filtro per tipo
 *  2. Giorno      — DayView (griglia a colonna singola)
 *  3. Mese        — MonthView in QScrollArea (griglia mensile con chip)
 *  4. Anno        — YearView in QScrollArea (12 mini-calendari)
 *
 *  Il dettaglio di un'attivita' si apre in una finestra figlia ridotta
 *  (ActivityDetailDialog), come la creazione/modifica (ActivityFormDialog):
 *  widget DENTRO la MainWindow, mai a schermo intero.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(CalendarController* controller, QWidget* parent = nullptr);
    ~MainWindow() override;

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

    /** @brief Apre il form di creazione preselezionando il tipo (0=Evento,
     *  1=Riunione, 2=Compito, 3=Anniversario), dal menu "Nuova attivita'". */
    void openNewActivityType(int typeIndex);

    void onSave();
    /** @brief "Salva con nome": chiede sempre il percorso col QFileDialog. */
    void onSaveAs();
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

    /** @brief Voce del menu "Visualizza" scelta dall'utente: passa alla
     *  pagina corrispondente. */
    void onViewSelected(AppMenuBar::ViewKind kind);

protected:
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

    NavigationBar* m_navBar = nullptr;
    AppMenuBar* m_menuBar = nullptr;

    QString m_currentFilePath;   // ultimo file salvato/caricato (per "Salva")

    QDate m_anchor;
    ViewKind m_view = ViewKind::Week;
    // Occorrenza "pendente" su cui l'utente deve scegliere serie o istanza
    std::optional<events::Occurrence> m_pendingOccurrence;
    QDateTime m_pendingDragTarget;
    bool m_pendingIsDrag = false;
};

} // namespace app
