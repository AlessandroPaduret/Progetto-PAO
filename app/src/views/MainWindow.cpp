#include "views/MainWindow.h"

#include <QDate>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTimeZone>
#include <QVBoxLayout>

#include "controller/CalendarController.h"
#include "builders/ActivityConfig.h"
#include "views/dialog/ActivityDetailDialog.h"
#include "views/dialog/ActivityFormDialog.h"
#include "views/ActivityListPage.h"
#include "views/DayView.h"
#include "views/MonthView.h"
#include "views/NavigationBar.h"
#include "views/dialog/RecurrenceChoiceDialog.h"
#include "views/utils/ViewShared.h"
#include "views/WeekView.h"
#include "views/YearView.h"

namespace app {

namespace {

// Indici delle pagine nel QStackedWidget
constexpr int kPageWeek = 0;
constexpr int kPageList = 1;
constexpr int kPageDay = 2;
constexpr int kPageMonth = 3;
constexpr int kPageYear = 4;

QScrollArea* makeScroll(QWidget* content) {
    auto* scroll = new QScrollArea(content->parentWidget());
    scroll->setWidget(content);
    scroll->setWidgetResizable(true);
    return scroll;
}

} // namespace

MainWindow::MainWindow(CalendarController* controller, QWidget* parent)
    : QMainWindow(parent), m_controller(controller) {
    setWindowTitle(tr("Le mie attivita'"));
    // Esplicito (invece di affidarsi al default Qt::Window): garantisce chiudi,
    // ridimensiona/massimizza e riduci a icona anche sotto window manager che
    // altrimenti non li mostrerebbero tutti.
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                   Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint |
                   Qt::WindowCloseButtonHint);

    // --- Pagine -----------------------------------------------------------------
    m_weekView = new WeekView(this);
    m_dayView = new DayView(this);
    m_monthView = new MonthView(this);
    m_yearView = new YearView(this);
    m_listPage = new ActivityListPage(controller, this);
    // Finestra figlia ridotta per il dettaglio di un'attivita' (dentro la
    // MainWindow: mai a schermo intero)
    m_detailDialog = new ActivityDetailDialog(controller, this);
    // Finestra figlia ridotta per creazione/modifica (si chiude con la "X")
    m_formDialog = new ActivityFormDialog(controller, this);
    // Finestra di scelta serie/singola occorrenza (interna, non esce)
    m_choiceDialog = new RecurrenceChoiceDialog(this);

    // Settimana/Giorno gestiscono da sole lo scroll (solo verticale, sulla
    // sola griglia oraria: intestazione e striscia "tutto il giorno" restano
    // fisse), niente QScrollArea esterna qui (altrimenti annidata due volte).
    auto* weekPage = new QWidget(this);
    auto* weekLayout = new QVBoxLayout(weekPage);
    weekLayout->setContentsMargins(0, 0, 0, 0);
    weekLayout->addWidget(m_weekView, 1);

    auto* dayPage = new QWidget(this);
    auto* dayLayout = new QVBoxLayout(dayPage);
    dayLayout->setContentsMargins(0, 0, 0, 0);
    dayLayout->addWidget(m_dayView, 1);

    auto* monthPage = new QWidget(this);
    auto* monthLayout = new QVBoxLayout(monthPage);
    monthLayout->setContentsMargins(0, 0, 0, 0);
    monthLayout->addWidget(makeScroll(m_monthView), 1);

    auto* yearPage = new QWidget(this);
    auto* yearLayout = new QVBoxLayout(yearPage);
    yearLayout->setContentsMargins(0, 0, 0, 0);
    yearLayout->addWidget(makeScroll(m_yearView), 1);

    // --- Barra di navigazione condivisa (Oggi / <- / -> / etichetta) -----------
    // Widget a se stante (NavigationBar): la MainWindow si limita a collegarne
    // i segnali e a impostarne il testo, niente layout dei pulsanti qui.
    m_navBar = new NavigationBar(this);

    m_pages = new QStackedWidget(this);
    m_pages->addWidget(weekPage);
    m_pages->addWidget(m_listPage);
    m_pages->addWidget(dayPage);
    m_pages->addWidget(monthPage);
    m_pages->addWidget(yearPage);

    auto* central = new QWidget(this);
    auto* centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->addWidget(m_navBar);
    centralLayout->addWidget(m_pages, 1);
    setCentralWidget(central);

    // --- Menu bar ----------------------------------------------------------------
    // AppMenuBar costruisce da sola menu/azioni/scorciatoie: la MainWindow si
    // limita a collegarne i segnali ai propri gestori.
    m_menuBar = new AppMenuBar(this);
    setMenuBar(m_menuBar);
    connect(m_menuBar, &AppMenuBar::saveRequested, this, &MainWindow::onSave);
    connect(m_menuBar, &AppMenuBar::saveAsRequested, this, &MainWindow::onSaveAs);
    connect(m_menuBar, &AppMenuBar::loadRequested, this, &MainWindow::onLoad);
    connect(m_menuBar, &AppMenuBar::newActivityRequested,
            this, &MainWindow::openNewActivityType);
    connect(m_menuBar, &AppMenuBar::viewSelected, this, &MainWindow::onViewSelected);

    // --- Connessioni ------------------------------------------------------------
    connect(controller, &CalendarController::activitiesChanged,
            this, &MainWindow::refresh);
    connect(m_navBar, &NavigationBar::todayRequested, this, &MainWindow::onToday);
    connect(m_navBar, &NavigationBar::previousRequested, this, &MainWindow::onPrevious);
    connect(m_navBar, &NavigationBar::nextRequested, this, &MainWindow::onNext);

    // Vista settimana
    connect(m_weekView, &WeekView::emptySlotClicked,
            this, &MainWindow::showFormCreate);
    connect(m_weekView, &WeekView::activityMoved,
            this, [this](const events::Occurrence& occurrence,
                         const QDateTime& newStart) {
                // Drag&drop: sposta l'attivita' alla nuova data/ora
                m_controller->moveActivity(occurrence.source, newStart);
            });
    connect(m_weekView, &WeekView::activityEditRequested,
            this, [this](const events::Occurrence& occurrence) {
                // Doppio clic: modifica l'attivita' sorgente conservando il
                // tipo (per i ricorrenti l'intera serie con la sua fine).
                showFormEditActivity(occurrence.source);
            });
    connect(m_weekView, &WeekView::occurrenceEditChoiceRequested,
            this, &MainWindow::askSeriesOrInstance);
    connect(m_weekView, &WeekView::occurrenceDragChoiceRequested,
            this, &MainWindow::askSeriesOrInstanceDrag);
    connect(m_weekView, &WeekView::infoRequested,
            this, [this](const events::Occurrence& occurrence) {
                showDetailDialog(occurrence.source);
            });
    connect(m_weekView, &WeekView::modifyEventRequested,
            this, &MainWindow::showFormEditOccurrence);
    connect(m_weekView, &WeekView::deleteEventRequested,
            this, &MainWindow::confirmDeleteOccurrence);
    connect(m_weekView, &WeekView::doneToggled,
            this, [this](const events::Occurrence& occurrence) {
                m_controller->toggleDone(occurrence);
            });

    // Vista giorno (stesse interazioni della settimana: e' una WeekView)
    connect(m_dayView, &WeekView::emptySlotClicked,
            this, &MainWindow::showFormCreate);
    connect(m_dayView, &WeekView::activityMoved,
            this, [this](const events::Occurrence& occurrence,
                         const QDateTime& newStart) {
                m_controller->moveActivity(occurrence.source, newStart);
            });
    connect(m_dayView, &WeekView::activityEditRequested,
            this, [this](const events::Occurrence& occurrence) {
                showFormEditActivity(occurrence.source);
            });
    connect(m_dayView, &WeekView::occurrenceEditChoiceRequested,
            this, &MainWindow::askSeriesOrInstance);
    connect(m_dayView, &WeekView::occurrenceDragChoiceRequested,
            this, &MainWindow::askSeriesOrInstanceDrag);
    connect(m_dayView, &WeekView::infoRequested,
            this, [this](const events::Occurrence& occurrence) {
                showDetailDialog(occurrence.source);
            });
    connect(m_dayView, &WeekView::modifyEventRequested,
            this, &MainWindow::showFormEditOccurrence);
    connect(m_dayView, &WeekView::deleteEventRequested,
            this, &MainWindow::confirmDeleteOccurrence);
    connect(m_dayView, &WeekView::doneToggled,
            this, [this](const events::Occurrence& occurrence) {
                m_controller->toggleDone(occurrence);
            });

    // Vista mese
    connect(m_monthView, &MonthView::emptySlotClicked,
            this, &MainWindow::showFormCreate);
    connect(m_monthView, &MonthView::activityEditRequested,
            this, [this](const events::Occurrence& occurrence) {
                showFormEditActivity(occurrence.source);
            });
    connect(m_monthView, &MonthView::occurrenceEditChoiceRequested,
            this, &MainWindow::askSeriesOrInstance);
    connect(m_monthView, &MonthView::infoRequested,
            this, [this](const events::Occurrence& occurrence) {
                showDetailDialog(occurrence.source);
            });
    connect(m_monthView, &MonthView::modifyEventRequested,
            this, &MainWindow::showFormEditOccurrence);
    connect(m_monthView, &MonthView::deleteEventRequested,
            this, &MainWindow::confirmDeleteOccurrence);
    connect(m_monthView, &MonthView::doneToggled,
            this, [this](const events::Occurrence& occurrence) {
                m_controller->toggleDone(occurrence);
            });

    // Vista anno: doppio clic su un giorno -> vista giorno di quella data
    connect(m_yearView, &YearView::daySelected,
            this, [this](const QDate& date) {
                m_view = ViewKind::Day;
                setAnchor(date);
                showDayPage();
            });

    connect(m_listPage, &ActivityListPage::detailRequested,
            this, &MainWindow::showDetailDialog);
    connect(m_listPage, &ActivityListPage::editRequested,
            this, &MainWindow::showFormEditActivity);

    // Dettaglio interno: Modifica apre il form, la chiusura non fa nulla
    connect(m_detailDialog, &ActivityDetailDialog::editRequested,
            this, &MainWindow::showFormEditActivity);

    // Finestra di scelta serie/singola occorrenza (interna alla MainWindow)
    connect(m_choiceDialog, &RecurrenceChoiceDialog::seriesChosen,
            this, &MainWindow::onChoiceSeries);
    connect(m_choiceDialog, &RecurrenceChoiceDialog::instanceChosen,
            this, &MainWindow::onChoiceInstance);
    connect(m_choiceDialog, &RecurrenceChoiceDialog::splitChosen,
            this, &MainWindow::onChoiceSplit);

    // Anteprima live dell'evento in fase di creazione/modifica nelle griglie
    // giorno/settimana (lo stesso aggiornamento per entrambe)
    connect(m_formDialog, &ActivityFormDialog::previewChanged,
            this, [this](const QString& title, const QDateTime& start,
                         qint64 durationSeconds, bool valid) {
                std::optional<WeekView::Preview> preview;
                if (valid) {
                    preview = WeekView::Preview{title, start,
                                                events::Duration(durationSeconds)};
                }
                m_weekView->setPreview(preview);
                m_dayView->setPreview(preview);
            });
    connect(m_formDialog, &ActivityFormDialog::closed,
            this, [this] {
                m_weekView->setPreview(std::nullopt);
                m_dayView->setPreview(std::nullopt);
            });

    // --- Stato iniziale ----------------------------------------------------------
    m_view = ViewKind::Week;
    setAnchor(QDate::currentDate());
    showWeekPage();
    refresh();
}

MainWindow::~MainWindow() = default;

void MainWindow::refresh() {
    // Vista giorno/settimana: confini in ora LOCALE (non UTC), coerenti con
    // come il form e il drag&drop di WeekView/DayColumnWidget interpretano
    // gli orari inseriti dall'utente (QDateTime(data, ora) senza QTimeZone
    // esplicita = locale). Con un confine a mezzanotte UTC, in un fuso con
    // offset positivo (es. UTC+2) un'occorrenza spostata all'1:37 di lunedi'
    // locale cade alle 23:37 di DOMENICA in UTC: prima dell'inizio della
    // finestra interrogata, quindi scompariva dalla vista (bug riscontrato
    // con eventi spostati prima delle 2 del mattino del primo giorno).
    const QDateTime dayFrom(m_anchor, QTime(0, 0));
    const QDateTime dayTo = dayFrom.addDays(1).addSecs(-1);
    m_dayView->setWeekStart(m_anchor);
    m_dayView->setOccurrences(m_controller->occurrencesIn(dayFrom, dayTo));

    // Vista settimana: occorrenze del lunedi' corrente
    const QDateTime weekFrom(m_anchor, QTime(0, 0));
    const QDateTime weekTo = weekFrom.addDays(7).addSecs(-1);
    m_weekView->setWeekStart(m_anchor);
    m_weekView->setOccurrences(m_controller->occurrencesIn(weekFrom, weekTo));

    // Vista mese: occorrenze sull'intera griglia (6 settimane dal lunedi')
    const QDate monthStart(m_anchor.year(), m_anchor.month(), 1);
    const QDate monthGridStart = monthStart.addDays(1 - monthStart.dayOfWeek());
    m_monthView->setMonth(monthStart);
    m_monthView->setOccurrences(m_controller->occurrencesIn(
        QDateTime(monthGridStart, QTime(0, 0), QTimeZone(0)),
        QDateTime(monthGridStart.addDays(42), QTime(0, 0), QTimeZone(0))
            .addSecs(-1)));

    // Vista anno: occorrenze dell'intero anno
    m_yearView->setYear(m_anchor);
    m_yearView->setOccurrences(m_controller->occurrencesIn(
        QDateTime(m_anchor, QTime(0, 0), QTimeZone(0)),
        QDateTime(QDate(m_anchor.year() + 1, 1, 1), QTime(0, 0), QTimeZone(0))
            .addSecs(-1)));

    // Elenco: ricarica (mantiene la ricerca corrente)
    m_listPage->refresh();

    // Etichetta della barra di navigazione
    switch (m_view) {
    case ViewKind::Day:
        m_navBar->setLabel(tr("Giorno del %1")
                               .arg(m_anchor.toString(QStringLiteral("dd/MM/yyyy"))));
        break;
    case ViewKind::Week:
        m_navBar->setLabel(tr("Settimana del %1")
                               .arg(m_anchor.toString(QStringLiteral("dd/MM/yyyy"))));
        break;
    case ViewKind::Month:
        m_navBar->setLabel(tr("Mese di %1")
                               .arg(m_anchor.toString(QStringLiteral("MM/yyyy"))));
        break;
    case ViewKind::Year:
        m_navBar->setLabel(tr("Anno %1").arg(m_anchor.year()));
        break;
    }
}

void MainWindow::setAnchor(const QDate& anchor) {
    switch (m_view) {
    case ViewKind::Day:
        m_anchor = anchor;
        break;
    case ViewKind::Week:
        m_anchor = mondayOf(anchor);
        break;
    case ViewKind::Month:
        m_anchor = QDate(anchor.year(), anchor.month(), 1);
        break;
    case ViewKind::Year:
        m_anchor = QDate(anchor.year(), 1, 1);
        break;
    }
    refresh();
}

QDate MainWindow::mondayOf(const QDate& date) {
    return date.addDays(1 - static_cast<int>(date.dayOfWeek()));
}

void MainWindow::onPrevious() {
    switch (m_view) {
    case ViewKind::Day:
        setAnchor(m_anchor.addDays(-1));
        break;
    case ViewKind::Week:
        setAnchor(m_anchor.addDays(-7));
        break;
    case ViewKind::Month:
        setAnchor(m_anchor.addMonths(-1));
        break;
    case ViewKind::Year:
        setAnchor(m_anchor.addYears(-1));
        break;
    }
}

void MainWindow::onNext() {
    switch (m_view) {
    case ViewKind::Day:
        setAnchor(m_anchor.addDays(1));
        break;
    case ViewKind::Week:
        setAnchor(m_anchor.addDays(7));
        break;
    case ViewKind::Month:
        setAnchor(m_anchor.addMonths(1));
        break;
    case ViewKind::Year:
        setAnchor(m_anchor.addYears(1));
        break;
    }
}

void MainWindow::onToday() {
    setAnchor(QDate::currentDate());
}

void MainWindow::showDayPage() {
    m_view = ViewKind::Day;
    m_pages->setCurrentIndex(kPageDay);
    m_navBar->setVisible(true);
    m_menuBar->setActiveView(AppMenuBar::ViewKind::Day);
    refresh();
}

void MainWindow::showWeekPage() {
    m_view = ViewKind::Week;
    m_pages->setCurrentIndex(kPageWeek);
    m_navBar->setVisible(true);
    m_menuBar->setActiveView(AppMenuBar::ViewKind::Week);
    refresh();
}

void MainWindow::showMonthPage() {
    m_view = ViewKind::Month;
    m_pages->setCurrentIndex(kPageMonth);
    m_navBar->setVisible(true);
    m_menuBar->setActiveView(AppMenuBar::ViewKind::Month);
    refresh();
}

void MainWindow::showYearPage() {
    m_view = ViewKind::Year;
    m_pages->setCurrentIndex(kPageYear);
    m_navBar->setVisible(true);
    m_menuBar->setActiveView(AppMenuBar::ViewKind::Year);
    refresh();
}

void MainWindow::showListPage() {
    m_pages->setCurrentIndex(kPageList);
    m_navBar->setVisible(false);
    m_menuBar->setActiveView(AppMenuBar::ViewKind::List);
}

void MainWindow::onViewSelected(AppMenuBar::ViewKind kind) {
    switch (kind) {
    case AppMenuBar::ViewKind::List:
        showListPage();
        break;
    case AppMenuBar::ViewKind::Day:
        showDayPage();
        break;
    case AppMenuBar::ViewKind::Week:
        showWeekPage();
        break;
    case AppMenuBar::ViewKind::Month:
        showMonthPage();
        break;
    case AppMenuBar::ViewKind::Year:
        showYearPage();
        break;
    }
}

void MainWindow::showDetailDialog(const events::Activity* activity) {
    if (!activity) {
        return;
    }
    // Il dettaglio si apre in una finestra figlia ridotta DENTRO la
    // MainWindow (niente pagina a schermo intero)
    m_detailDialog->showActivity(activity);
    m_detailDialog->showCentered();
}

void MainWindow::showFormCreate(const QDateTime& start) {
    m_formDialog->startCreate(start);
    m_formDialog->showCentered();
}

void MainWindow::showFormEditActivity(const events::Activity* activity) {
    if (!activity) {
        return;
    }
    m_formDialog->startEditActivity(activity);
    m_formDialog->showCentered();
}

void MainWindow::showFormEditOccurrence(const events::Occurrence& occurrence) {
    m_formDialog->startEditOccurrence(occurrence);
    m_formDialog->showCentered();
}

void MainWindow::openNewActivityType(int typeIndex) {
    m_formDialog->startCreateType(typeIndex);
    m_formDialog->showCentered();
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    // I pannelli interni restano sempre dentro la finestra principale
    if (m_formDialog && m_formDialog->isVisible()) {
        m_formDialog->showCentered();
    }
    if (m_choiceDialog && m_choiceDialog->isVisible()) {
        m_choiceDialog->showCentered();
    }
    if (m_detailDialog && m_detailDialog->isVisible()) {
        m_detailDialog->showCentered();
    }
}

// ---------------------------------------------------------------------------
// Scelta serie / singola occorrenza per gli eventi ricorrenti
// ---------------------------------------------------------------------------
void MainWindow::onChoiceSplit() {
    m_choiceDialog->hide();
    if (!m_pendingOccurrence) {
        m_pendingIsDrag = false;
        return;
    }
    const events::Occurrence occurrence = *m_pendingOccurrence;
    const QDateTime target =
        m_pendingIsDrag
            ? m_pendingDragTarget
            : QDateTime::fromSecsSinceEpoch(
                  occurrence.start.time_since_epoch().count());
    m_pendingOccurrence.reset();
    m_pendingIsDrag = false;

    // La serie attuale termina prima di questa occorrenza; ne nasce una
    // nuova con le stesse regole di ricorrenza ma inizio diverso e la
    // stessa data di scadenza.
    m_controller->splitRecurrence(occurrence, target);
}

void MainWindow::askSeriesOrInstance(const events::Occurrence& occurrence) {
    m_pendingOccurrence = occurrence;
    m_pendingIsDrag = false;
    m_choiceDialog->ask(tr(
        "Questo evento fa parte di una serie ricorrente. Vuoi modificare "
        "l'intera serie, proseguire da questo momento in poi (la serie "
        "attuale termina e ne nasce una nuova con le stesse regole) oppure "
        "solo questo evento (che diventera' un evento singolo, fuori dalla "
        "serie)?"));
    m_choiceDialog->showCentered();
}

void MainWindow::askSeriesOrInstanceDrag(const events::Occurrence& occurrence,
                                         const QDateTime& newStart) {
    m_pendingOccurrence = occurrence;
    m_pendingDragTarget = newStart;
    m_pendingIsDrag = true;
    m_choiceDialog->ask(tr(
        "Questo evento fa parte di una serie ricorrente. Vuoi spostare "
        "l'intera serie, proseguire da questo momento in poi (la serie "
        "attuale termina e ne nasce una nuova con le stesse regole) oppure "
        "solo questo evento (che diventera' un evento singolo, fuori dalla "
        "serie)?"));
    m_choiceDialog->showCentered();
}

void MainWindow::onChoiceSeries() {
    m_choiceDialog->hide();
    if (!m_pendingOccurrence) {
        m_pendingIsDrag = false;
        return;
    }
    const events::Occurrence occurrence = *m_pendingOccurrence;
    m_pendingOccurrence.reset();
    m_pendingIsDrag = false;

    // Come il doppio clic sull'evento di inizio serie: apre la finestra di
    // modifica dell'intera serie (regola, durata, data di scadenza, ...).
    // Vale sia per il doppio clic sia per il trascinamento.
    showFormEditActivity(occurrence.source);
}

void MainWindow::onChoiceInstance() {
    m_choiceDialog->hide();
    if (!m_pendingOccurrence) {
        m_pendingIsDrag = false;
        return;
    }
    const events::Occurrence occurrence = *m_pendingOccurrence;
    const bool wasDrag = m_pendingIsDrag;
    m_pendingOccurrence.reset();
    m_pendingIsDrag = false;

    if (wasDrag) {
        // "Sposta solo questo evento": l'occorrenza esce dalla serie
        // (eccezione interna: buco in origine) e diventa un evento standard
        // alla data/ora di destinazione del trascinamento.
        auto replacement = events::makeActivity(events::ActivityConfig{
            .title = occurrence.source->getTitle(),
            .start = events::TimePoint(std::chrono::seconds(
                m_pendingDragTarget.toSecsSinceEpoch())),
            .duration = occurrence.duration});
        m_controller->modifyOccurrence(occurrence, std::move(replacement));
        return;
    }
    // L'occorrenza di quel giorno diventa un evento STANDARD: si apre la
    // finestra di modifica dell'evento normale (non del ricorrente). Al
    // salvataggio la serie continua ad esistere ma senza quel giorno
    // (eccezione interna + nuovo evento singolo).
    showFormEditOccurrence(occurrence);
}

void MainWindow::confirmDeleteOccurrence(const events::Occurrence& occurrence) {
    const bool recurrent = isRecurrent(occurrence.source);
    if (!recurrent) {
        if (QMessageBox::question(
                this, tr("Elimina attivita'"),
                tr("Eliminare '%1'?").arg(
                    QString::fromStdString(occurrence.source->getTitle()))) ==
            QMessageBox::Yes) {
            m_controller->deleteOccurrence(occurrence);
        }
        return;
    }

    QMessageBox box(QMessageBox::Question, tr("Elimina occorrenza"),
                    tr("'%1' e' un'attivita' ricorrente.").arg(
                        QString::fromStdString(occurrence.source->getTitle())));
    QPushButton* followingButton =
        box.addButton(tr("Questa e le successive"), QMessageBox::YesRole);
    QPushButton* onlyThisButton =
        box.addButton(tr("Solo questa"), QMessageBox::NoRole);
    box.addButton(QMessageBox::Cancel);
    box.exec();

    if (box.clickedButton() == followingButton) {
        m_controller->deleteOccurrence(occurrence, true);
    } else if (box.clickedButton() == onlyThisButton) {
        m_controller->deleteOccurrence(occurrence);
    }
}

void MainWindow::onSave() {
    // Salva sull'ultimo file usato (caricato o salvato in precedenza);
    // se non ce n'e' uno, si comporta come "Salva con nome".
    if (m_currentFilePath.isEmpty()) {
        onSaveAs();
        return;
    }
    QString error;
    if (!m_controller->saveToFile(m_currentFilePath, &error)) {
        QMessageBox::warning(this, tr("Salvataggio"), error);
    }
}

void MainWindow::onSaveAs() {
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Salva attivita'"), m_currentFilePath,
        tr("File JSON (*.json)"));
    if (path.isEmpty()) {
        return;
    }
    m_currentFilePath = path;
    QString error;
    if (!m_controller->saveToFile(path, &error)) {
        QMessageBox::warning(this, tr("Salvataggio"), error);
    }
}

void MainWindow::onLoad() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Carica attivita'"), m_currentFilePath,
        tr("File JSON (*.json)"));
    if (path.isEmpty()) {
        return;
    }
    QString error;
    if (!m_controller->loadFromFile(path, &error)) {
        QMessageBox::warning(this, tr("Caricamento"), error);
        return;
    }
    m_currentFilePath = path;
}

} // namespace app
