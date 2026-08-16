#include "views/MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QDate>
#include <QDateTime>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTimeZone>
#include <QToolBar>
#include <QVBoxLayout>

#include "CalendarController.h"
#include "events/domain/RecurrentEvent.h"
#include "views/ActivityDetailPage.h"
#include "views/ActivityFormDialog.h"
#include "views/ActivityListPage.h"
#include "views/WeekView.h"

namespace app {

MainWindow::MainWindow(CalendarController* controller, QWidget* parent)
    : QMainWindow(parent), m_controller(controller) {
    setWindowTitle(tr("Le mie attivita'"));

    // --- Pagine -----------------------------------------------------------------
    m_weekView = new WeekView(this);
    m_listPage = new ActivityListPage(controller, this);
    m_detailPage = new ActivityDetailPage(controller, this);
    // Finestra figlia ridotta per creazione/modifica (si chiude con la "X")
    m_formDialog = new ActivityFormDialog(controller, this);

    auto* weekScroll = new QScrollArea(this);
    weekScroll->setWidget(m_weekView);
    weekScroll->setWidgetResizable(true);

    auto* weekBar = new QWidget(this);
    auto* todayButton = new QPushButton(tr("Oggi"), weekBar);
    auto* prevButton = new QPushButton(tr("\u2190"), weekBar);
    auto* nextButton = new QPushButton(tr("\u2192"), weekBar);
    m_weekLabel = new QLabel(weekBar);
    m_weekLabel->setAlignment(Qt::AlignCenter);
    auto* weekBarLayout = new QHBoxLayout(weekBar);
    weekBarLayout->setContentsMargins(8, 4, 8, 4);
    weekBarLayout->addWidget(todayButton);
    weekBarLayout->addWidget(prevButton);
    weekBarLayout->addWidget(nextButton);
    weekBarLayout->addWidget(m_weekLabel, 1);

    auto* weekPage = new QWidget(this);
    auto* weekLayout = new QVBoxLayout(weekPage);
    weekLayout->setContentsMargins(0, 0, 0, 0);
    weekLayout->addWidget(weekBar);
    weekLayout->addWidget(weekScroll, 1);

    m_pages = new QStackedWidget(this);
    m_pages->addWidget(weekPage);
    m_pages->addWidget(m_listPage);
    m_pages->addWidget(m_detailPage);
    setCentralWidget(m_pages);

    // --- Toolbar ----------------------------------------------------------------
    auto* toolbar = new QToolBar(tr("Barra principale"), this);
    toolbar->setMovable(false);
    toolbar->addAction(tr("Settimana"), this, &MainWindow::showWeekPage);
    toolbar->addAction(tr("Elenco"), this, &MainWindow::showListPage);
    toolbar->addSeparator();
    toolbar->addAction(tr("Nuova attivita'..."), this, &MainWindow::onNewActivity);
    toolbar->addAction(tr("Modifica"), this, &MainWindow::onEditSelected);
    toolbar->addAction(tr("Elimina"), this, &MainWindow::onDeleteSelected);
    toolbar->addSeparator();
    toolbar->addAction(tr("Salva..."), this, &MainWindow::onSave);
    toolbar->addAction(tr("Carica..."), this, &MainWindow::onLoad);
    addToolBar(toolbar);

    // --- Connessioni ------------------------------------------------------------
    connect(controller, &CalendarController::activitiesChanged,
            this, &MainWindow::refresh);
    connect(todayButton, &QPushButton::clicked, this, &MainWindow::onToday);
    connect(prevButton, &QPushButton::clicked, this, &MainWindow::onPreviousWeek);
    connect(nextButton, &QPushButton::clicked, this, &MainWindow::onNextWeek);

    connect(m_weekView, &WeekView::emptySlotClicked,
            this, &MainWindow::showFormCreate);
    connect(m_weekView, &WeekView::activityEditRequested,
            this, [this](const events::Occurrence& occurrence) {
                // Doppio clic: modifica l'attivita' sorgente conservando il
                // tipo (per i ricorrenti l'intera serie con la sua fine).
                showFormEditActivity(occurrence.source);
            });
    connect(m_weekView, &WeekView::infoRequested,
            this, [this](const events::Occurrence& occurrence) {
                showDetailPage(occurrence.source);
            });
    connect(m_weekView, &WeekView::modifyEventRequested,
            this, &MainWindow::showFormEditOccurrence);
    connect(m_weekView, &WeekView::deleteEventRequested,
            this, &MainWindow::confirmDeleteOccurrence);

    connect(m_listPage, &ActivityListPage::detailRequested,
            this, &MainWindow::showDetailPage);
    connect(m_listPage, &ActivityListPage::editRequested,
            this, &MainWindow::showFormEditActivity);

    connect(m_detailPage, &ActivityDetailPage::backRequested,
            this, &MainWindow::showListPage);
    connect(m_detailPage, &ActivityDetailPage::editRequested,
            this, &MainWindow::showFormEditActivity);

    // --- Stato iniziale ----------------------------------------------------------
    setWeekStart(QDate::currentDate().addDays(
        1 - static_cast<int>(QDate::currentDate().dayOfWeek())));
    showWeekPage();
    refresh();
}

void MainWindow::refresh() {
    // Vista settimanale: occorrenze del lunedi' corrente
    const QDateTime fromUtc = QDateTime(m_monday, QTime(0, 0), QTimeZone(0));
    const QDateTime toUtc = QDateTime(m_monday.addDays(7), QTime(0, 0), QTimeZone(0))
                                .addSecs(-1);
    m_weekView->setOccurrences(m_controller->occurrencesIn(fromUtc, toUtc));
    m_weekView->setWeekStart(m_monday);

    // Elenco: ricarica (mantiene la ricerca corrente)
    m_listPage->refresh();

    m_weekLabel->setText(tr("Settimana del %1")
                             .arg(m_monday.toString(QStringLiteral("dd/MM/yyyy"))));
}

void MainWindow::setWeekStart(const QDate& monday) {
    m_monday = monday;
    m_weekView->setWeekStart(m_monday);
    refresh();
}

QDate MainWindow::currentMonday() const {
    return QDate::currentDate().addDays(
        1 - static_cast<int>(QDate::currentDate().dayOfWeek()));
}

void MainWindow::onPreviousWeek() {
    setWeekStart(m_monday.addDays(-7));
}

void MainWindow::onNextWeek() {
    setWeekStart(m_monday.addDays(7));
}

void MainWindow::onToday() {
    setWeekStart(currentMonday());
}

void MainWindow::showWeekPage() {
    m_pages->setCurrentIndex(0);
}

void MainWindow::showListPage() {
    m_pages->setCurrentIndex(1);
}

void MainWindow::showDetailPage(const events::Activity* activity) {
    if (!activity) {
        return;
    }
    m_detailPage->showActivity(activity);
    m_pages->setCurrentIndex(2);
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

void MainWindow::onNewActivity() {
    m_formDialog->startCreate();
    m_formDialog->showCentered();
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    // Il pannello di creazione resta sempre dentro la finestra principale
    if (m_formDialog && m_formDialog->isVisible()) {
        m_formDialog->showCentered();
    }
}

void MainWindow::onEditSelected() {
    if (m_pages->currentIndex() == 0) {
        if (const events::Occurrence* selected = m_weekView->selectedOccurrence()) {
            showFormEditOccurrence(*selected);
        }
    } else if (m_pages->currentIndex() == 2) {
        if (const events::Activity* activity = m_detailPage->currentActivity()) {
            showFormEditActivity(activity);
        }
    }
}

void MainWindow::onDeleteSelected() {
    if (m_pages->currentIndex() == 0) {
        if (const events::Occurrence* selected = m_weekView->selectedOccurrence()) {
            confirmDeleteOccurrence(*selected);
        }
    }
}

void MainWindow::confirmDeleteOccurrence(const events::Occurrence& occurrence) {
    const auto* recurrent =
        dynamic_cast<const events::RecurrentEvent*>(occurrence.source);
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
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Salva attivita'"), QString(), tr("File JSON (*.json)"));
    if (path.isEmpty()) {
        return;
    }
    QString error;
    if (!m_controller->saveToFile(path, &error)) {
        QMessageBox::warning(this, tr("Salvataggio"), error);
    }
}

void MainWindow::onLoad() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Carica attivita'"), QString(), tr("File JSON (*.json)"));
    if (path.isEmpty()) {
        return;
    }
    QString error;
    if (!m_controller->loadFromFile(path, &error)) {
        QMessageBox::warning(this, tr("Caricamento"), error);
    }
}

} // namespace app
