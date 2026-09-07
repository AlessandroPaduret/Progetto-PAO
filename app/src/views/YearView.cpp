#include "views/YearView.h"

#include <QCalendarWidget>
#include <QColor>
#include <QFont>
#include <QGridLayout>
#include <QLocale>
#include <QTextCharFormat>

#include <optional>

#include "views/utils/Theme.h"
#include "views/utils/ViewShared.h"

namespace app {

YearView::YearView(QWidget* parent) : QWidget(parent) {
    m_grid = new QGridLayout(this);
    m_grid->setSpacing(4);

    const QLocale italian(QLocale::Italian, QLocale::Italy);

    for (int i = 0; i < kCols * kRows; ++i) {
        auto* calendar = new QCalendarWidget(this);
        calendar->setLocale(italian);
        calendar->setNavigationBarVisible(false);
        calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
        calendar->setHorizontalHeaderFormat(QCalendarWidget::SingleLetterDayNames);
        calendar->setGridVisible(false);

        // I pannelli sono fissi (un mese ciascuno): se l'utente naviga da un
        // giorno "fuori mese" ai margini della griglia, il pannello torna
        // subito al proprio mese assegnato.
        connect(calendar, &QCalendarWidget::currentPageChanged, this,
                [calendar](int newYear, int newMonth) {
                    const int fixedYear = calendar->property("fixedYear").toInt();
                    const int fixedMonth = calendar->property("fixedMonth").toInt();
                    if (newYear != fixedYear || newMonth != fixedMonth) {
                        calendar->setCurrentPage(fixedYear, fixedMonth);
                    }
                });
        connect(calendar, &QCalendarWidget::activated, this, &YearView::daySelected);

        m_grid->addWidget(calendar, i / kCols, i % kCols);
        m_grid->setColumnStretch(i % kCols, 1);
        m_grid->setRowStretch(i / kCols, 1);
        m_calendars[i] = calendar;
    }
    setMinimumSize(baseWidth(), baseHeight());
}

int YearView::baseWidth() const {
    return kCols * 220;
}

int YearView::baseHeight() const {
    return kRows * 190;
}

void YearView::setYear(const QDate& januaryFirst) {
    m_year = QDate(januaryFirst.year(), 1, 1);
    for (int i = 0; i < kCols * kRows; ++i) {
        const int month = i + 1;
        QCalendarWidget* calendar = m_calendars[i];
        calendar->setProperty("fixedYear", m_year.year());
        calendar->setProperty("fixedMonth", month);
        calendar->setCurrentPage(m_year.year(), month);
    }
}

void YearView::setOccurrences(const std::vector<events::Occurrence>& occurrences) {
    // Riazzera l'evidenziazione di tutti i giorni dell'anno mostrato prima
    // di riapplicarla in base alle nuove occorrenze.
    for (int i = 0; i < kCols * kRows; ++i) {
        const int month = i + 1;
        const QDate first(m_year.year(), month, 1);
        for (int day = 1; day <= first.daysInMonth(); ++day) {
            m_calendars[i]->setDateTextFormat(QDate(m_year.year(), month, day), QTextCharFormat());
        }
    }

    // Un solo colore per giorno (il widget nativo non supporta piu' pallini
    // per cella): quello della prima attivita' non evasa, altrimenti grigio
    // se ci sono solo Compiti evasi.
    std::vector<std::optional<QColor>> colorByDay(367);  // 1-indicizzato sul "giorno dell'anno"
    for (const events::Occurrence& occ : occurrences) {
        const QDate date = localTime(occ.start).date();
        if (date.year() != m_year.year()) {
            continue;
        }
        const bool done = isTaskDone(occ.source, occ.start);
        auto& slot = colorByDay[date.dayOfYear()];
        if (!slot || (slot == theme::kDoneGray && !done)) {
            slot = done ? theme::kDoneGray : activityColor(occ.source);
        }
    }

    for (int doy = 1; doy <= 366; ++doy) {
        if (!colorByDay[doy]) {
            continue;
        }
        const QDate date = QDate(m_year.year(), 1, 1).addDays(doy - 1);
        QTextCharFormat format;
        format.setBackground(colorByDay[doy]->lighter(150));
        format.setFontWeight(QFont::Bold);
        m_calendars[date.month() - 1]->setDateTextFormat(date, format);
    }
}

} // namespace app
