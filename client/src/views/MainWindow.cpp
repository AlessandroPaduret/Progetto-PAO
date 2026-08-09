#include "views/MainWindow.h"

#include <QDateEdit>
#include <QDateTime>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QTableWidget>
#include <QTimeZone>
#include <QToolBar>

#include "api/dto.h"
#include "controllers/EventsController.h"
#include "views/EventDialog.h"

namespace client {

MainWindow::MainWindow(EventsController* events, QWidget* parent)
    : QMainWindow(parent), m_events(events) {
    setWindowTitle(tr("Calendario eventi"));
    resize(900, 500);

    // Toolbar: range + azioni
    auto* toolbar = addToolBar(tr("Azioni"));
    toolbar->setMovable(false);

    m_fromDate = new QDateEdit(this);
    m_fromDate->setCalendarPopup(true);
    m_fromDate->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_toDate = new QDateEdit(this);
    m_toDate->setCalendarPopup(true);
    m_toDate->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    setRangeToCurrentMonth();

    auto* previousButton = new QPushButton(tr("Mese \u2190"), this);
    auto* nextButton = new QPushButton(tr("Mese \u2192"), this);
    auto* refreshButton = new QPushButton(tr("Aggiorna"), this);
    auto* createButton = new QPushButton(tr("Nuovo evento"), this);
    auto* exceptionButton = new QPushButton(tr("Aggiungi eccezione"), this);
    auto* deleteButton = new QPushButton(tr("Elimina evento"), this);
    auto* exitButton = new QPushButton(tr("Esci"), this);

    toolbar->addWidget(new QLabel(tr(" Da: "), this));
    toolbar->addWidget(m_fromDate);
    toolbar->addWidget(new QLabel(tr(" A: "), this));
    toolbar->addWidget(m_toDate);
    toolbar->addWidget(previousButton);
    toolbar->addWidget(nextButton);
    toolbar->addWidget(refreshButton);
    toolbar->addSeparator();
    toolbar->addWidget(createButton);
    toolbar->addWidget(exceptionButton);
    toolbar->addWidget(deleteButton);
    toolbar->addSeparator();
    toolbar->addWidget(exitButton);

    // Tabella delle occorrenze
    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels(
        {tr("Data"), tr("Titolo"), tr("Inizio"), tr("Fine"), tr("Id")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    setCentralWidget(m_table);

    connect(previousButton, &QPushButton::clicked, this, &MainWindow::onPreviousMonth);
    connect(nextButton, &QPushButton::clicked, this, &MainWindow::onNextMonth);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::refresh);
    connect(createButton, &QPushButton::clicked, this, &MainWindow::onCreateEvent);
    connect(exceptionButton, &QPushButton::clicked, this, &MainWindow::onAddException);
    connect(deleteButton, &QPushButton::clicked, this, &MainWindow::onDeleteEvent);
    connect(exitButton, &QPushButton::clicked, this, &QWidget::close);

    connect(m_events, &EventsController::eventsChanged, this,
            &MainWindow::populateTable);
    connect(m_events, &EventsController::errorOccurred, this,
            [this](const QString& error) {
                statusBar()->showMessage(tr("Errore: %1").arg(error), 8000);
            });
    connect(m_events, &EventsController::noticeOccurred, this,
            [this](const QString& message) {
                statusBar()->showMessage(message, 5000);
            });
}

void MainWindow::setRangeToCurrentMonth() {
    const QDate today = QDate::currentDate();
    m_fromDate->setDate(QDate(today.year(), today.month(), 1));
    m_toDate->setDate(QDate(today.year(), today.month(),
                            today.daysInMonth()));
}

void MainWindow::refresh() {
    const QDateTime from(m_fromDate->date(), QTime(0, 0), QTimeZone::UTC);
    const QDateTime to(m_toDate->date(), QTime(23, 59, 59), QTimeZone::UTC);
    m_events->setRange(from, to);
}

void MainWindow::onPreviousMonth() {
    m_fromDate->setDate(m_fromDate->date().addMonths(-1));
    m_toDate->setDate(m_toDate->date().addMonths(-1));
    refresh();
}

void MainWindow::onNextMonth() {
    m_fromDate->setDate(m_fromDate->date().addMonths(1));
    m_toDate->setDate(m_toDate->date().addMonths(1));
    refresh();
}

void MainWindow::populateTable(const QVector<Occurrence>& occurrences) {
    m_table->clearContents();
    m_table->setRowCount(occurrences.size());
    for (int i = 0; i < occurrences.size(); ++i) {
        const auto& occurrence = occurrences[i];
        const QDateTime localStart = occurrence.start.toLocalTime();
        const QDateTime localEnd = occurrence.end.toLocalTime();

        auto* dateItem =
            new QTableWidgetItem(localStart.date().toString(Qt::ISODate));
        auto* titleItem = new QTableWidgetItem(occurrence.title);
        auto* startItem = new QTableWidgetItem(
            localStart.toString(QStringLiteral("HH:mm")));
        auto* endItem = new QTableWidgetItem(
            localEnd.toString(QStringLiteral("HH:mm")));
        auto* idItem =
            new QTableWidgetItem(QString::number(occurrence.eventId));

        for (auto* item : {dateItem, titleItem, startItem, endItem, idItem}) {
            item->setData(Qt::UserRole, occurrence.eventId);
            item->setData(Qt::UserRole + 1, occurrence.start); // UTC
            item->setData(Qt::UserRole + 2, occurrence.end);   // UTC
        }
        m_table->setItem(i, 0, dateItem);
        m_table->setItem(i, 1, titleItem);
        m_table->setItem(i, 2, startItem);
        m_table->setItem(i, 3, endItem);
        m_table->setItem(i, 4, idItem);
    }
}

bool MainWindow::selectedOccurrence(Occurrence& out) const {
    const int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(const_cast<MainWindow*>(this),
                                 tr("Nessuna selezione"),
                                 tr("Seleziona un'occorrenza dalla tabella."));
        return false;
    }
    const QTableWidgetItem* item = m_table->item(row, 0);
    out.eventId = item->data(Qt::UserRole).toLongLong();
    out.title = m_table->item(row, 1)->text();
    out.start = item->data(Qt::UserRole + 1).toDateTime(); // UTC
    out.end = item->data(Qt::UserRole + 2).toDateTime();   // UTC
    return true;
}

void MainWindow::onCreateEvent() {
    EventDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        m_events->createEvent(dialog.request());
    }
}

void MainWindow::onDeleteEvent() {
    Occurrence occurrence;
    if (!selectedOccurrence(occurrence)) {
        return;
    }
    const auto answer = QMessageBox::question(
        this, tr("Eliminare l'evento?"),
        tr("Vuoi eliminare l'evento \"%1\" (id %2)?").arg(occurrence.title)
            .arg(occurrence.eventId));
    if (answer == QMessageBox::Yes) {
        m_events->deleteEvent(occurrence.eventId);
    }
}

void MainWindow::onAddException() {
    Occurrence occurrence;
    if (!selectedOccurrence(occurrence)) {
        return;
    }
    const auto answer = QMessageBox::question(
        this, tr("Aggiungere un'eccezione?"),
        tr("Escludere l'occorrenza del %1 dall'evento \"%2\"?")
            .arg(occurrence.title, QString::number(occurrence.eventId)));
    if (answer == QMessageBox::Yes) {
        // L'eccezione è l'inizio dell'occorrenza selezionata.
        m_events->addException(occurrence.eventId, occurrence.start);
    }
}

} // namespace client
