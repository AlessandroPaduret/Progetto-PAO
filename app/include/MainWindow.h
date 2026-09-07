#pragma once

#include <QDate>
#include <QMainWindow>
#include <QString>

#include "events.h"
#include "menu/AppMenuBar.h"

class QStackedWidget;

namespace app {

class ActivityListPage;
class ActivitySidebarWidget;
class CalendarController;
class DayView;
class MonthView;
class NavigationBar;
class RecurrenceChoiceDialog;
class WeekView;
class YearView;

/** @brief Finestra unica (vincolo PAO: niente dialog per creazione/modifica)
 *  con pagine navigabili: 0 settimana, 1 elenco, 2 giorno, 3 mese, 4 anno.
 *  Creazione/modifica e' un pannello laterale (ActivitySidebarWidget, QWidget
 *  non dialog) affiancato via QSplitter, mostrato/nascosto a seconda
 *  dell'interazione. La scelta serie/occorrenza (RecurrenceChoiceDialog) resta
 *  un QDialog modale (exec()) perche' e' un'interruzione puntuale, non un
 *  pannello persistente. */
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
    void showFormCreate(const QDateTime& start = QDateTime());
    void showFormEditActivity(const events::Activity* activity);
    void showFormEditOccurrence(const events::Occurrence& occurrence);

    void onPrevious();
    void onNext();
    void onToday();

    /** @brief Tipo preselezionato: 0=Evento, 1=Riunione, 2=Compito. */
    void openNewActivityType(int typeIndex);

    void onSave();
    /** @brief "Salva con nome": chiede sempre il percorso col QFileDialog. */
    void onSaveAs();
    void onLoad();

    void confirmDeleteOccurrence(const events::Occurrence& occurrence);

    /** @brief Chiede (RecurrenceChoiceDialog::ask, modale) se modificare
     *  l'intera serie, dividerla da qui in poi o solo questa occorrenza. */
    void askSeriesOrInstance(const events::Occurrence& occurrence);
    /** @brief Come askSeriesOrInstance, per un trascinamento verso newStart. */
    void askSeriesOrInstanceDrag(const events::Occurrence& occurrence,
                                 const QDateTime& newStart);

    void onViewSelected(AppMenuBar::ViewKind kind);

private:
    enum class ViewKind { Day, Week, Month, Year };

    /** @brief Normalizza per la vista corrente (lunedi' per la settimana, 1 del mese, 1 gennaio). */
    void setAnchor(const QDate& anchor);

    static QDate mondayOf(const QDate& date);

    CalendarController* m_controller;
    QStackedWidget* m_pages = nullptr;
    WeekView* m_weekView = nullptr;
    DayView* m_dayView = nullptr;
    MonthView* m_monthView = nullptr;
    YearView* m_yearView = nullptr;
    ActivityListPage* m_listPage = nullptr;
    ActivitySidebarWidget* m_sidebar = nullptr;

    NavigationBar* m_navBar = nullptr;
    AppMenuBar* m_menuBar = nullptr;

    QString m_currentFilePath;   // ultimo file salvato/caricato (per "Salva")

    QDate m_anchor;
    ViewKind m_view = ViewKind::Week;
};

} // namespace app
