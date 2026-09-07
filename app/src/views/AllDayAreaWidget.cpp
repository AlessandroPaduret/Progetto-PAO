#include "views/AllDayAreaWidget.h"

#include <QApplication>
#include <QGridLayout>
#include <QMouseEvent>
#include <QStyle>

#include <algorithm>

#include "views/OccurrenceWidget.h"
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
    // Neutralizza le colonne di una precedente dayCount piu' larga (es. il
    // passaggio Settimana(7) -> Giorno(1)): setColumnStretch/MinimumWidth
    // restano altrimenti applicati anche a colonne senza piu' alcun widget,
    // allargando il layout oltre le colonne davvero in uso.
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
    // deleteLater(), non delete: stesso motivo di DayColumnWidget/WeekView
    // (nessun chip qui e' trascinabile, ma per coerenza con lo stesso
    // pattern usato altrove non c'e' motivo di rischiare diversamente).
    for (OccurrenceWidget* chip : m_chips) {
        chip->hide();
        chip->deleteLater();
    }
    m_chips.clear();

    // WeekGridLayout non conosce QDate/Occurrence: filtra qui le "tutto il
    // giorno" e converti ciascuna nel suo DaySpan (offset in giorni da
    // m_viewStart), tenendo `fullDayIndices` per tornare indietro dal
    // risultato (parallelo ai DaySpan) all'Occurrence originale.
    std::vector<int> fullDayIndices;
    std::vector<DaySpan> spans;
    for (int i = 0; i < static_cast<int>(occurrences.size()); ++i) {
        const events::Occurrence& occ = occurrences[i];
        if (!coversFullDay(occ)) {
            continue;
        }
        const QDate startDate = localTime(occ.start).date();
        const QDate endExclusive = localTime(occ.end()).date();
        int startDay = m_viewStart.daysTo(startDate);
        int endDay = m_viewStart.daysTo(endExclusive) - 1;
        if (endDay < startDay) {
            endDay = startDay;
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
