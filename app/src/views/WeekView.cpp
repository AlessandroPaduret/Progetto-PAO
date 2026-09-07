#include "views/WeekView.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QVBoxLayout>

#include <chrono>

#include "views/AllDayAreaWidget.h"
#include "views/DayColumnWidget.h"
#include "views/HeaderWidget.h"
#include "views/OccurrenceWidget.h"
#include "views/TimeGutterWidget.h"
#include "views/utils/ViewShared.h"

namespace app {

WeekView::WeekView(QWidget* parent) : QWidget(parent) {
    m_header = new HeaderWidget(this);

    m_allDayArea = new AllDayAreaWidget(this);
    connect(m_allDayArea, &AllDayAreaWidget::chipPressed, this, &WeekView::setSelectedChip);
    connect(m_allDayArea, &AllDayAreaWidget::doneToggled, this, &WeekView::doneToggled);
    connect(m_allDayArea, &AllDayAreaWidget::activityEditRequested,
            this, &WeekView::activityEditRequested);
    connect(m_allDayArea, &AllDayAreaWidget::occurrenceEditChoiceRequested,
            this, &WeekView::occurrenceEditChoiceRequested);
    connect(m_allDayArea, &AllDayAreaWidget::infoRequested, this, &WeekView::infoRequested);
    connect(m_allDayArea, &AllDayAreaWidget::modifyEventRequested,
            this, &WeekView::modifyEventRequested);
    connect(m_allDayArea, &AllDayAreaWidget::deleteEventRequested,
            this, &WeekView::deleteEventRequested);
    connect(m_allDayArea, &AllDayAreaWidget::backgroundClicked, this, &WeekView::clearSelection);

    m_gutter = new TimeGutterWidget;

    m_gridContent = new QWidget;
    auto* gridLayout = new QHBoxLayout(m_gridContent);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(0);
    gridLayout->addWidget(m_gutter);

    // Solo verticale: l'orizzontale non deve mai scorrere (le colonne sono
    // elastiche). La scrollbar verticale resta SEMPRE visibile (invece di
    // apparire/sparire in base al contenuto) cosi' la larghezza che sottrae
    // alle colonne e' costante e HeaderWidget/AllDayAreaWidget possono
    // riservare lo stesso spazio fisso sul bordo destro per restare allineati.
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidget(m_gridContent);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_header);
    layout->addWidget(m_allDayArea);
    layout->addWidget(m_scrollArea, 1);

    rebuildColumns();
}

void WeekView::setDayCount(int days) {
    const int bounded = qBound(1, days, kDaysPerWeek);
    if (bounded == m_dayCount) {
        return;
    }
    m_dayCount = bounded;
    rebuildColumns();
    distributeOccurrences();
}

int WeekView::dayCount() const {
    return m_dayCount;
}

void WeekView::rebuildColumns() {
    for (DayColumnWidget* column : m_columns) {
        column->hide();
        column->deleteLater();
    }
    m_columns.clear();
    m_selectedChip = nullptr;
    m_selectedOccurrence.reset();

    auto* gridLayout = qobject_cast<QHBoxLayout*>(m_gridContent->layout());
    for (int i = 0; i < m_dayCount; ++i) {
        auto* column = new DayColumnWidget(m_gridContent);
        connect(column, &DayColumnWidget::emptySlotClicked, this, &WeekView::emptySlotClicked);
        connect(column, &DayColumnWidget::activityEditRequested,
                this, &WeekView::activityEditRequested);
        connect(column, &DayColumnWidget::occurrenceEditChoiceRequested,
                this, &WeekView::occurrenceEditChoiceRequested);
        connect(column, &DayColumnWidget::infoRequested, this, &WeekView::infoRequested);
        connect(column, &DayColumnWidget::modifyEventRequested,
                this, &WeekView::modifyEventRequested);
        connect(column, &DayColumnWidget::deleteEventRequested,
                this, &WeekView::deleteEventRequested);
        connect(column, &DayColumnWidget::activityMoved, this, &WeekView::activityMoved);
        connect(column, &DayColumnWidget::occurrenceDragChoiceRequested,
                this, &WeekView::occurrenceDragChoiceRequested);
        connect(column, &DayColumnWidget::doneToggled, this, &WeekView::doneToggled);
        connect(column, &DayColumnWidget::chipPressed, this, &WeekView::setSelectedChip);
        connect(column, &DayColumnWidget::backgroundClicked, this, &WeekView::clearSelection);
        gridLayout->addWidget(column, 1);
        m_columns.push_back(column);
    }

    if (m_monday.isValid()) {
        m_header->setDays(m_monday, m_dayCount);
        m_allDayArea->setDays(m_monday, m_dayCount);
        for (int i = 0; i < m_dayCount; ++i) {
            m_columns[i]->setDate(m_monday.addDays(i));
        }
    }
}

void WeekView::setWeekStart(const QDate& monday) {
    // NON richiama distributeOccurrences(): lo farebbe con m_occurrences
    // ancora VECCHIE (quelle dell'ultima setOccurrences), che a questo punto
    // possono gia' riferirsi a un'Activity appena distrutta se la chiamata
    // arriva da un refresh() innescato da una modifica/eliminazione
    // (updateActivity/removeActivity distruggono la vecchia Activity PRIMA
    // di emettere activitiesChanged). Ricostruire subito i widget delle
    // occorrenze da quel puntatore ormai pendente fa leggere un
    // DateGenerator gia' liberato -> crash. L'unico chiamante
    // (MainWindow::refresh()) invoca sempre setOccurrences(...) con dati
    // freschi subito dopo: la ridistribuzione avviene la', non qui.
    m_monday = monday;
    m_header->setDays(m_monday, m_dayCount);
    m_allDayArea->setDays(m_monday, m_dayCount);
    for (int i = 0; i < m_dayCount; ++i) {
        m_columns[i]->setDate(m_monday.addDays(i));
    }
}

void WeekView::setOccurrences(const std::vector<events::Occurrence>& occurrences) {
    m_occurrences = occurrences;
    m_selectedChip = nullptr;
    m_selectedOccurrence.reset();
    distributeOccurrences();
}

void WeekView::distributeOccurrences() {
    m_allDayArea->setOccurrences(m_occurrences);

    for (int i = 0; i < m_dayCount; ++i) {
        const QDate date = m_monday.addDays(i);
        const QDateTime dayStart(date, QTime(0, 0));
        const QDateTime dayEnd = dayStart.addDays(1);
        std::vector<events::Occurrence> dayOccurrences;
        for (const events::Occurrence& occ : m_occurrences) {
            // Sovrapposizione (non solo "inizia oggi"): un'occorrenza a
            // cavallo di mezzanotte finisce anche nella colonna del giorno
            // dopo (ritagliata li' da WeekGridLayout::layoutDayColumn), cosi'
            // non "sparisce" oltre la mezzanotte del giorno di inizio. La
            // fine e' "effettiva" (>= 1 minuto anche a durata zero) per non
            // escludere per errore un'occorrenza puntuale a mezzanotte esatta.
            const events::TimePoint effectiveEnd = occ.duration > events::Duration::zero()
                                                       ? occ.end()
                                                       : occ.start + std::chrono::minutes(1);
            if (!coversFullDay(occ) && localTime(occ.start) < dayEnd &&
                localTime(effectiveEnd) > dayStart) {
                dayOccurrences.push_back(occ);
            }
        }
        m_columns[i]->setOccurrences(dayOccurrences);
    }

    for (DayColumnWidget* column : m_columns) {
        column->setPreview(m_preview);
    }
}

void WeekView::setPreview(const std::optional<Preview>& preview) {
    m_preview = preview;
    for (DayColumnWidget* column : m_columns) {
        column->setPreview(m_preview);
    }
}

const std::optional<WeekView::Preview>& WeekView::preview() const {
    return m_preview;
}

const events::Occurrence* WeekView::selectedOccurrence() const {
    return m_selectedOccurrence ? &(*m_selectedOccurrence) : nullptr;
}

void WeekView::setSelectedChip(OccurrenceWidget* chip, const events::Occurrence& occurrence) {
    if (m_selectedChip) {
        m_selectedChip->setSelected(false);
    }
    m_selectedChip = chip;
    m_selectedOccurrence = occurrence;
    chip->setSelected(true);
}

void WeekView::clearSelection() {
    if (m_selectedChip) {
        m_selectedChip->setSelected(false);
        m_selectedChip = nullptr;
    }
    m_selectedOccurrence.reset();
}

} // namespace app
