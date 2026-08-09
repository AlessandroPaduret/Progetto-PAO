#include "views/MainWindow.h"

#include <QDateEdit>
#include <QDateTime>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QStatusBar>
#include <QToolBar>

#include "api/dto.h"
#include "controllers/EventsController.h"
#include "views/EventDialog.h"
#include "views/WeekView.h"

namespace client {

MainWindow::MainWindow(EventsController* events, QWidget* parent)
    : QMainWindow(parent), m_events(events) {
    resize(1000, 620);

    // Toolbar: settimana + azioni
    auto* toolbar = addToolBar(tr("Azioni"));
    toolbar->setMovable(false);

    m_fromDate = new QDateEdit(this);
    m_fromDate->setCalendarPopup(true);
    m_fromDate->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_toDate = new QDateEdit(this);
    m_toDate->setCalendarPopup(true);
    m_toDate->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    setRangeToCurrentWeek();

    auto* previousButton = new QPushButton(tr("Settimana \u2190"), this);
    auto* nextButton = new QPushButton(tr("Settimana \u2192"), this);
    auto* refreshButton = new QPushButton(tr("Aggiorna"), this);
    auto* createButton = new QPushButton(tr("Nuovo evento"), this);
    auto* infoButton = new QPushButton(tr("Info"), this);
    auto* deleteButton = new QPushButton(tr("Elimina"), this);
    auto* exitButton = new QPushButton(tr("Esci"), this);

    toolbar->addWidget(new QLabel(tr(" Dal: "), this));
    toolbar->addWidget(m_fromDate);
    toolbar->addWidget(new QLabel(tr(" Al: "), this));
    toolbar->addWidget(m_toDate);
    toolbar->addWidget(previousButton);
    toolbar->addWidget(nextButton);
    toolbar->addWidget(refreshButton);
    toolbar->addSeparator();
    toolbar->addWidget(createButton);
    toolbar->addWidget(infoButton);
    toolbar->addWidget(deleteButton);
    toolbar->addSeparator();
    toolbar->addWidget(exitButton);

    // Vista settimanale dentro uno scroll
    m_weekView = new WeekView(this);
    auto* scroll = new QScrollArea(this);
    scroll->setWidget(m_weekView);
    scroll->setWidgetResizable(true); // scala la griglia con la finestra
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setCentralWidget(scroll);

    connect(previousButton, &QPushButton::clicked, this, &MainWindow::onPreviousWeek);
    connect(nextButton, &QPushButton::clicked, this, &MainWindow::onNextWeek);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::refresh);
    connect(createButton, &QPushButton::clicked, this, [this]() { onCreateEvent(); });
    connect(infoButton, &QPushButton::clicked, this, [this]() {
        Occurrence occurrence;
        if (selectedOccurrence(occurrence)) {
            showEventInfo(occurrence);
        }
    });
    connect(deleteButton, &QPushButton::clicked, this, [this]() {
        Occurrence occurrence;
        if (selectedOccurrence(occurrence)) {
            confirmDeleteEvent(occurrence);
        }
    });
    connect(exitButton, &QPushButton::clicked, this, &QWidget::close);

    // Doppio clic su una cella vuota -> crea evento con quell'orario
    connect(m_weekView, &WeekView::emptySlotClicked, this,
            [this](const QDateTime& start) { onCreateEvent(start); });

    // Menu contestuale (tasto destro) sugli eventi
    connect(m_weekView, &WeekView::infoRequested, this,
            &MainWindow::showEventInfo);
    connect(m_weekView, &WeekView::modifyEventRequested, this,
            &MainWindow::confirmModifyEvent);
    connect(m_weekView, &WeekView::deleteEventRequested, this,
            &MainWindow::confirmDeleteEvent);

    connect(m_events, &EventsController::eventsChanged, this,
            [this](const QVector<Occurrence>& occurrences) {
                m_weekView->setWeekStart(m_fromDate->date());
                m_weekView->setOccurrences(occurrences);
            });
    connect(m_events, &EventsController::errorOccurred, this,
            [this](const QString& error) {
                statusBar()->showMessage(tr("Errore: %1").arg(error), 8000);
            });
    connect(m_events, &EventsController::noticeOccurred, this,
            [this](const QString& message) {
                statusBar()->showMessage(message, 5000);
            });
}

void MainWindow::setRangeToCurrentWeek() {
    const QDate today = QDate::currentDate();
    const QDate monday = today.addDays(1 - today.dayOfWeek()); // lunedì
    m_fromDate->setDate(monday);
    m_toDate->setDate(monday.addDays(6));
}

void MainWindow::refresh() {
    const QDateTime from(m_fromDate->date(), QTime(0, 0));    // locale
    const QDateTime to(m_toDate->date(), QTime(23, 59, 59));  // locale
    m_events->setRange(from, to);
    m_weekView->setWeekStart(m_fromDate->date());
    setWindowTitle(tr("Calendario eventi \u2014 %1 \u2013 %2")
                       .arg(from.date().toString(QStringLiteral("dd/MM/yyyy")),
                            to.date().toString(QStringLiteral("dd/MM/yyyy"))));
}

void MainWindow::onPreviousWeek() {
    m_fromDate->setDate(m_fromDate->date().addDays(-7));
    m_toDate->setDate(m_toDate->date().addDays(-7));
    refresh();
}

void MainWindow::onNextWeek() {
    m_fromDate->setDate(m_fromDate->date().addDays(7));
    m_toDate->setDate(m_toDate->date().addDays(7));
    refresh();
}

void MainWindow::onCreateEvent(const QDateTime& start) {
    EventDialog dialog(this);
    if (start.isValid()) {
        dialog.setStart(start);
    }
    if (dialog.exec() == QDialog::Accepted) {
        m_events->createEvent(dialog.request());
    }
}

bool MainWindow::selectedOccurrence(Occurrence& out) const {
    const auto occurrence = m_weekView->selectedOccurrence();
    if (!occurrence.has_value()) {
        QMessageBox::information(const_cast<MainWindow*>(this),
                                 tr("Nessuna selezione"),
                                 tr("Clicca su un evento nella settimana."));
        return false;
    }
    out = *occurrence;
    return true;
}

void MainWindow::showEventInfo(const Occurrence& occurrence) {
    QString typeName = tr("Singolo");
    if (occurrence.type == QLatin1String("fixed")) {
        typeName = tr("Ricorrente (intervallo)");
    } else if (occurrence.type == QLatin1String("yearly")) {
        typeName = tr("Ricorrente (annuale)");
    }

    const qint64 minutes =
        qMax<qint64>(0, occurrence.start.secsTo(occurrence.end) / 60);
    const QString duration = minutes >= 60
                                 ? tr("%1 h %2 min").arg(minutes / 60)
                                       .arg(minutes % 60)
                                 : tr("%1 min").arg(minutes);

    const QString text =
        tr("Id: %1\nTitolo: %2\nTipo: %3\nInizio: %4\nFine: %5\nDurata: %6")
            .arg(QString::number(occurrence.eventId), occurrence.title, typeName,
                 occurrence.start.toLocalTime().toString(
                     QStringLiteral("dd/MM/yyyy HH:mm")),
                 occurrence.end.toLocalTime().toString(
                     QStringLiteral("dd/MM/yyyy HH:mm")),
                 duration);
    QMessageBox::information(this, tr("Dettagli evento"), text);
}

void MainWindow::confirmModifyEvent(const Occurrence& occurrence) {
    EventDialog dialog(this);
    dialog.setOccurrence(occurrence);
    if (dialog.exec() == QDialog::Accepted) {
        m_events->modifyOccurrence(occurrence, dialog.request());
    }
}

void MainWindow::confirmDeleteEvent(const Occurrence& occurrence) {
    // Evento ricorrente: si può eliminare solo questa istanza oppure
    // questa e tutte le successive (la ricorrenza termina prima).
    if (occurrence.type != QLatin1String("single")) {
        QMessageBox box(this);
        box.setWindowTitle(tr("Eliminare l'occorrenza?"));
        box.setText(tr("L'evento \"%1\" è ricorrente. Cosa vuoi eliminare?")
                        .arg(occurrence.title));
        auto* allButton =
            box.addButton(tr("Questa e le successive"), QMessageBox::ActionRole);
        auto* oneButton =
            box.addButton(tr("Solo questa"), QMessageBox::ActionRole);
        box.addButton(tr("Annulla"), QMessageBox::RejectRole);
        box.exec();

        if (box.clickedButton() == allButton) {
            m_events->truncateEvent(occurrence.eventId, occurrence.start);
        } else if (box.clickedButton() == oneButton) {
            // Internamente viene segnata un'eccezione sulla ricorrenza.
            m_events->addException(occurrence.eventId, occurrence.start);
        }
        return;
    }

    // Evento singolo: conferma ed elimina.
    const auto answer = QMessageBox::question(
        this, tr("Eliminare l'evento?"),
        tr("Vuoi eliminare l'evento \"%1\" (id %2)?")
            .arg(occurrence.title, QString::number(occurrence.eventId)));
    if (answer == QMessageBox::Yes) {
        m_events->deleteEvent(occurrence.eventId);
    }
}

} // namespace client
