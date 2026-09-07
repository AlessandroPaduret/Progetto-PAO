#include "views/weekly/AllDayAreaWidget.h"

#include <QApplication>
#include <QGridLayout>
#include <QMouseEvent>
#include <QStyle>

#include <algorithm>

#include "views/weekly/OccurrenceWidget.h"
#include "views/utils/ViewShared.h"
#include "views/utils/WeekGridLayout.h"

namespace app {

AllDayAreaWidget::AllDayAreaWidget(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName(QStringLiteral("weekAllDayArea"));
    m_grid = new QGridLayout(this);
    m_grid->setContentsMargins(0, 2, 0, 2);
    m_grid->setSpacing(2);
    setFixedHeight(kWeekAllDayRowHeight);
}

void AllDayAreaWidget::setDays(const QDate& viewStart, int dayCount) {
    // Azzera lo stretch/minimo delle colonne di un dayCount precedente piu'
    // largo (es. Settimana(7) -> Giorno(1)): altrimenti restano applicati a
    // colonne senza piu' widget e allargano il layout oltre il necessario.
    for (int c = 0; c < m_grid->columnCount(); ++c) {
        m_grid->setColumnStretch(c, 0);
        m_grid->setColumnMinimumWidth(c, 0);
    }

    m_viewStart = viewStart;
    m_dayCount = dayCount;

    m_grid->setColumnMinimumWidth(0, kWeekGutterWidth);
    for (int d = 0; d < dayCount; ++d) {
        m_grid->setColumnStretch(d + 1, 1);
    }
    const int scrollbarWidth = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    m_grid->setColumnMinimumWidth(dayCount + 1, scrollbarWidth);
}

void AllDayAreaWidget::setOccurrences(const std::vector<events::Occurrence>& occurrences) {
    // deleteLater(), non delete: coerente col pattern usato altrove (vedi
    // DayColumnWidget) anche se qui nessun chip e' trascinabile.
    for (OccurrenceWidget* chip : m_chips) {
        chip->hide();
        chip->deleteLater();
    }
    m_chips.clear();

    // WeekGridLayout non conosce QDate/Occurrence: filtra le "tutto il
    // giorno" e converti ciascuna nel suo DaySpan (offset in giorni da
    // m_viewStart); fullDayIndices fa da tramite per tornare all'Occurrence
    // originale dal risultato del layout.
    std::vector<int> fullDayIndices;
    std::vector<DaySpan> spans;
    for (int i = 0; i < static_cast<int>(occurrences.size()); ++i) {
        const events::Occurrence& occ = occurrences[i];
        if (!coversFullDay(occ)) {
            continue;
        }
        // Le occorrenze "tutto il giorno" sono salvate a mezzanotte UTC:
        // activityDisplayTime sceglie apposta la data UTC (non quella
        // locale, che in un fuso con offset positivo le farebbe apparire
        // ancora in corso il giorno LOCALE successivo).
        const QDate startDate = activityDisplayTime(occ.source, occ.start).date();
        const QDate endExclusive = activityDisplayTime(occ.source, occ.end()).date();
        int startDay = m_viewStart.daysTo(startDate);
        int endDay = m_viewStart.daysTo(endExclusive) - 1;
        if (endDay < startDay) {
            endDay = startDay;
        }
        // Un'occorrenza fuori dai giorni visibili va scartata, non
        // ritagliata: la finestra e' LOCALE (serve per gli eventi con
        // orario), quindi un'attivita' tutto il giorno di IERI in UTC puo'
        // ancora "sovrapporsi" per qualche ora in un fuso con offset. Senza
        // questo scarto, layoutAllDayRows() sotto ritaglierebbe (qBound)
        // ogni span sul giorno visibile invece di escluderlo, facendo
        // ricomparire l'occorrenza anche nel giorno dopo (vista Giorno).
        if (endDay < 0 || startDay > m_dayCount - 1) {
            continue;
        }
        fullDayIndices.push_back(i);
        spans.push_back({startDay, endDay});
    }

    const std::vector<AllDayPlacement> placements =
        WeekGridLayout::layoutAllDayRows(spans, m_dayCount);

    int maxRow = 0;
    for (const AllDayPlacement& placement : placements) {
        const events::Occurrence& occ = occurrences[fullDayIndices[placement.index]];
        auto* chip = new OccurrenceWidget(occ, OccurrenceWidget::Style::Chip,
                                          isRecurrent(occ.source), /*draggable=*/false, this);
        connect(chip, &OccurrenceWidget::pressed, this,
                [this, chip](const events::Occurrence& o) { emit chipPressed(chip, o); });
        connect(chip, &OccurrenceWidget::doneToggled, this, &AllDayAreaWidget::doneToggled);
        connect(chip, &OccurrenceWidget::infoRequested, this, &AllDayAreaWidget::infoRequested);
        connect(chip, &OccurrenceWidget::editRequested,
                this, &AllDayAreaWidget::activityEditRequested);
        connect(chip, &OccurrenceWidget::modifyInstanceRequested,
                this, &AllDayAreaWidget::modifyEventRequested);
        connect(chip, &OccurrenceWidget::deleteRequested,
                this, &AllDayAreaWidget::deleteEventRequested);
        connect(chip, &OccurrenceWidget::doubleClicked, this,
                [this](const events::Occurrence& o) {
                    if (isRecurrent(o.source) && o.start > o.source->getStart()) {
                        emit occurrenceEditChoiceRequested(o);
                    } else {
                        emit activityEditRequested(o);
                    }
                });
        m_grid->addWidget(chip, placement.row, placement.startDay + 1, 1,
                          placement.endDay - placement.startDay + 1);
        chip->show();
        m_chips.push_back(chip);
        maxRow = std::max(maxRow, placement.row);
    }

    setFixedHeight((maxRow + 1) * kWeekAllDayRowHeight);
}

void AllDayAreaWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit backgroundClicked();
    }
    QWidget::mousePressEvent(event);
}

} // namespace app
