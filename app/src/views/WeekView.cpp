#include "views/WeekView.h"

#include <QDate>
#include <QFont>
#include <QHelpEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

#include <algorithm>

#include "events/domain/RecurrentEvent.h"
#include "views/ViewShared.h"

namespace app {

namespace {

constexpr int kMinutesPerDay = 24 * 60;

// Spunta (checkbox) disegnata in basso a destra di ogni occorrenza
QRect checkRectOf(const QRect& block) {
    return QRect(block.right() - 14, block.bottom() - 14, 13, 13);
}

bool isAllDay(const events::Activity* source) {
    return source->isAllDay();
}

} // namespace

WeekView::WeekView(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    // Sotto le dimensioni base la griglia non scala: compaiono le scrollbar.
    setMinimumSize(baseWidth(), baseHeight());
}

void WeekView::setDayCount(int days) {
    m_dayCount = qBound(1, days, kDaysPerWeek);
    m_dropCell.reset();
    update();
}

int WeekView::dayCount() const {
    return m_dayCount;
}

int WeekView::baseWidth() const {
    return kGutterWidth + m_dayCount * kDayWidth;
}

int WeekView::baseHeight() const {
    return kHeaderHeight + kAllDayHeight + 24 * kHourHeight;
}

int WeekView::gridTop() const {
    return kHeaderHeight + kAllDayHeight;
}

int WeekView::dayWidth() const {
    return qMax(kDayWidth, (width() - kGutterWidth) / m_dayCount);
}

int WeekView::hourHeight() const {
    return qMax(kHourHeight, (height() - gridTop()) / 24);
}

QTime WeekView::localTimeOf(const events::Occurrence& occurrence) const {
    return localTime(occurrence.start).time();
}

int WeekView::minuteOf(const QTime& time) const {
    return time.msecsSinceStartOfDay() / 60000;
}

void WeekView::setOccurrences(const std::vector<events::Occurrence>& occurrences) {
    m_occurrences = occurrences;
    m_selected = -1;
    m_dragActive = false;
    m_dragMoved = false;
    m_dragIndex = -1;
    m_dropCell.reset();
    m_rects.clear();
    m_checkRects.clear();
    update();
}

void WeekView::setWeekStart(const QDate& monday) {
    m_monday = monday;
    m_dropCell.reset();
    update();
}

void WeekView::setPreview(const std::optional<Preview>& preview) {
    m_preview = preview;
    update();
}

const std::optional<WeekView::Preview>& WeekView::preview() const {
    return m_preview;
}

const events::Occurrence* WeekView::selectedOccurrence() const {
    if (m_selected < 0 || m_selected >= static_cast<int>(m_occurrences.size())) {
        return nullptr;
    }
    return &m_occurrences[m_selected];
}

// ---------------------------------------------------------------------------
// Layout: striscia "tutto il giorno" + griglia a colonne per le occorrenze
// sovrapposte dello stesso giorno.
// ---------------------------------------------------------------------------
void WeekView::ensureRects() {
    m_rects.assign(m_occurrences.size(), QRect());
    m_checkRects.assign(m_occurrences.size(), QRect());

    // --- Striscia "tutto il giorno" -----------------------------------------
    for (int i = 0; i < static_cast<int>(m_occurrences.size()); ++i) {
        const events::Occurrence& occ = m_occurrences[i];
        if (!isAllDay(occ.source)) {
            continue;
        }
        const QDate startDate = localTime(occ.start).date();
        // end() e' la mezzanotte del giorno successivo all'ultimo
        const QDate endExclusive = localTime(occ.end()).date();
        int firstDay = m_monday.daysTo(startDate);
        int lastDay = m_monday.daysTo(endExclusive) - 1;
        if (lastDay < firstDay) {
            lastDay = firstDay;
        }
        firstDay = qBound(0, firstDay, m_dayCount - 1);
        lastDay = qBound(0, lastDay, m_dayCount - 1);
        const int x = kGutterWidth + firstDay * dayWidth() + 2;
        const int w = (lastDay - firstDay + 1) * dayWidth() - 4;
        const int y = kHeaderHeight + 2;
        m_rects[i] = QRect(x, y, w, kAllDayHeight - 4);
        m_checkRects[i] = checkRectOf(m_rects[i]);
    }

    // --- Griglia oraria: layout a colonne per giorno ------------------------
    for (int day = 0; day < m_dayCount; ++day) {
        // Indici delle occorrenze del giorno (ordinate per inizio)
        std::vector<int> dayIndex;
        for (int i = 0; i < static_cast<int>(m_occurrences.size()); ++i) {
            if (isAllDay(m_occurrences[i].source)) {
                continue;
            }
            if (m_monday.daysTo(localTime(m_occurrences[i].start).date()) == day) {
                dayIndex.push_back(i);
            }
        }
        std::sort(dayIndex.begin(), dayIndex.end(),
                  [this](int a, int b) {
                      return m_occurrences[a].start < m_occurrences[b].start;
                  });

        int k = 0;
        while (k < static_cast<int>(dayIndex.size())) {
            // Fine effettiva (durata zero -> +1 minuto per l'affiancamento)
            auto effectiveEnd = [this](const events::Occurrence& o) {
                return o.duration > events::Duration::zero()
                           ? o.end()
                           : o.start + std::chrono::minutes(1);
            };

            // Cluster: occorrenze sovrapposte (transitivamente)
            auto clusterStop = effectiveEnd(m_occurrences[dayIndex[k]]);
            int j = k + 1;
            while (j < static_cast<int>(dayIndex.size()) &&
                   m_occurrences[dayIndex[j]].start < clusterStop) {
                clusterStop =
                    std::max(clusterStop, effectiveEnd(m_occurrences[dayIndex[j]]));
                ++j;
            }

            // Assegnazione greedy delle colonne
            std::vector<int> column;                       // colonna per indice
            std::vector<events::TimePoint> columnEnd;      // fine ultimo evento per colonna
            for (int t = k; t < j; ++t) {
                const int idx = dayIndex[t];
                const events::TimePoint start = m_occurrences[idx].start;
                int col = 0;
                while (col < static_cast<int>(columnEnd.size()) &&
                       !(start >= columnEnd[col])) {
                    ++col;
                }
                if (col == static_cast<int>(columnEnd.size())) {
                    columnEnd.push_back(start);
                }
                column.push_back(col);
                columnEnd[col] = std::max(columnEnd[col],
                                          effectiveEnd(m_occurrences[idx]));
            }
            const int clusterCols = std::max(1, static_cast<int>(columnEnd.size()));

            // Rects: colonne affiancate larghe dayWidth/clusterCols
            for (int t = k; t < j; ++t) {
                const int idx = dayIndex[t];
                const events::Occurrence& occ = m_occurrences[idx];
                const QDateTime localStart = localTime(occ.start);
                const QDateTime localEnd = localTime(occ.end());

                const int topMin = minuteOf(localStart.time());
                int bottomMin = minuteOf(localEnd.time());
                if (localEnd.date() != localStart.date()) {
                    bottomMin = kMinutesPerDay;  // attraversa la mezzanotte
                }
                const int lo = qBound(0, topMin, kMinutesPerDay);
                const int hi = qBound(0, bottomMin, kMinutesPerDay);
                int height = (hi - lo) * hourHeight() / 60 - 4;
                if (height < kMinOccurrenceHeight) {
                    height = kMinOccurrenceHeight;
                }

                const int colWidth = dayWidth() / clusterCols;
                const int x = kGutterWidth + day * dayWidth() +
                              column[t - k] * colWidth + 2;
                const int y = gridTop() + lo * hourHeight() / 60 + 2;
                m_rects[idx] = QRect(x, y, colWidth - 4, height);
                m_checkRects[idx] = checkRectOf(m_rects[idx]);
            }
            k = j;
        }
    }
}

int WeekView::hitTest(const QPoint& pos) const {
    // Ricalcola il layout se necessario (paint precedente) per un hit corretto
    const_cast<WeekView*>(this)->ensureRects();
    for (int i = 0; i < static_cast<int>(m_rects.size()); ++i) {
        if (m_rects[i].contains(pos)) {
            return i;
        }
    }
    return -1;
}

std::optional<QDateTime> WeekView::cellAt(const QPoint& pos) const {
    if (pos.y() < gridTop() || pos.x() < kGutterWidth) {
        return std::nullopt;
    }
    const int dayIndex = (pos.x() - kGutterWidth) / dayWidth();
    if (dayIndex < 0 || dayIndex >= m_dayCount) {
        return std::nullopt;
    }
    const int minutes = (pos.y() - gridTop()) * 60 / hourHeight();
    const QTime time(qBound(0, minutes / 60, 23), qBound(0, minutes % 60, 59));
    return QDateTime(m_monday.addDays(dayIndex), time);
}

QRect WeekView::dragGhostRect(const QDateTime& localStart,
                              const events::Duration duration) const {
    const int dayIndex = m_monday.daysTo(localStart.date());
    if (dayIndex < 0 || dayIndex >= m_dayCount) {
        return QRect();
    }

    const QDateTime localEnd = localStart.addSecs(duration.count());
    const int topMin = minuteOf(localStart.time());
    int bottomMin = minuteOf(localEnd.time());
    if (localStart.date() != localEnd.date()) {
        bottomMin = kMinutesPerDay;
    }
    const int height = qMax(kMinOccurrenceHeight,
                            (qBound(0, bottomMin, kMinutesPerDay) -
                             qBound(0, topMin, kMinutesPerDay)) *
                                hourHeight() / 60 - 4);
    const int x = kGutterWidth + dayIndex * dayWidth() + 2;
    const int y = gridTop() + qBound(0, topMin, kMinutesPerDay) *
                                      hourHeight() / 60 + 2;
    return QRect(x, y, dayWidth() - 4, height);
}

void WeekView::paintEvent(QPaintEvent*) {
    ensureRects();
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    // Scala dei font proporzionale all'ingrandimento della griglia
    const qreal scale = qMin(static_cast<qreal>(dayWidth()) / kDayWidth,
                             static_cast<qreal>(hourHeight()) / kHourHeight);
    const int headerFontSize = qMax(10, qRound(10 * scale));
    const int smallFontSize = qMax(8, qRound(8 * scale));

    // --- Intestazione: nome giorno + numero ---
    for (int day = 0; day < m_dayCount; ++day) {
        const QRect headerRect(kGutterWidth + day * dayWidth(), 0,
                               dayWidth(), kHeaderHeight);
        painter.fillRect(headerRect, Qt::white);

        const QDate date = m_monday.addDays(day);
        const bool isToday = date == QDate::currentDate();
        painter.setPen(isToday ? QColor("#1a73e8") : QColor("#5f6368"));
        QFont font = painter.font();
        font.setBold(isToday);
        font.setPointSize(headerFontSize);
        painter.setFont(font);
        painter.drawText(headerRect, Qt::AlignCenter,
                         QStringLiteral("%1 %2")
                             .arg(QString::fromLatin1(shortDayName(date.dayOfWeek())))
                             .arg(date.day()));
    }

    // --- Striscia "tutto il giorno" (sfondo + separatori) ---
    painter.fillRect(QRect(kGutterWidth, kHeaderHeight, width() - kGutterWidth,
                           kAllDayHeight),
                     QColor("#f8f9fa"));
    painter.setPen(QColor("#dadce0"));
    painter.drawLine(kGutterWidth, kHeaderHeight, width(), kHeaderHeight);
    painter.drawLine(kGutterWidth, gridTop(), width(), gridTop());
    for (int day = 0; day <= m_dayCount; ++day) {
        const int x = kGutterWidth + day * dayWidth();
        painter.drawLine(x, kHeaderHeight, x, gridTop());
    }

    // --- Linee della griglia ---
    painter.setPen(QColor("#dadce0"));
    for (int day = 0; day <= m_dayCount; ++day) {
        const int x = kGutterWidth + day * dayWidth();
        painter.drawLine(x, gridTop(), x, height());
    }

    // --- Ore sul bordo sinistro + linee orizzontali ---
    QFont hourFont = painter.font();
    hourFont.setPointSize(smallFontSize);
    painter.setFont(hourFont);
    for (int hour = 0; hour < 24; ++hour) {
        const int y = gridTop() + hour * hourHeight();
        painter.setPen(QColor("#dadce0"));
        painter.drawLine(kGutterWidth, y, width(), y);
        painter.setPen(QColor("#5f6368"));
        painter.drawText(QRect(0, y - smallFontSize, kGutterWidth - 8,
                               smallFontSize * 2),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QStringLiteral("%1:00").arg(hour, 2, 10,
                                                     QLatin1Char('0')));
    }

    // --- Evidenziazione della cella di destinazione durante il drag ---
    if (m_dragActive && m_dragMoved && m_dropCell &&
        m_dragIndex >= 0 && m_dragIndex < static_cast<int>(m_occurrences.size())) {
        const events::Occurrence& dragged = m_occurrences[m_dragIndex];
        const QRect ghost = dragGhostRect(*m_dropCell, dragged.duration);
        if (!ghost.isValid()) {
            QColor overlay("#1a73e8");
            overlay.setAlpha(12);
            painter.fillRect(QRect(kGutterWidth, gridTop(),
                                   width() - kGutterWidth,
                                   height() - gridTop()),
                             overlay);
        } else {
            QColor overlay("#1a73e8");
            overlay.setAlpha(25);
            painter.fillRect(ghost, overlay);
            painter.setPen(QColor("#1a73e8"));
            painter.drawRect(ghost);
        }
    }

    // --- Attivita' come blocchi colorati (layout a colonne) ---
    for (int i = 0; i < static_cast<int>(m_occurrences.size()); ++i) {
        const QRect rect = m_rects[i];
        if (!rect.isValid()) {
            continue;
        }
        const events::Occurrence& occurrence = m_occurrences[i];
        const QColor color = activityColor(occurrence.source);
        const bool done = occurrence.source->isDoneAt(occurrence.start);

        // Evaso: blocco attenuato
        QColor fill = color.lighter(done ? 180 : 150);
        if (done) {
            fill.setAlpha(110);
        }
        painter.setPen(i == m_selected ? QPen(QColor("#1a73e8"), 2)
                                       : QPen(color.darker(120), 1));
        painter.setBrush(fill);
        painter.drawRect(rect);

        // Spunta (checkbox) in alto a sinistra
        const QRect check = m_checkRects[i];
        painter.setPen(QColor("#5f6368"));
        painter.setBrush(done ? QColor("#1a73e8") : Qt::white);
        painter.drawRect(check);
        if (done) {
            painter.setPen(Qt::white);
            painter.drawText(check, Qt::AlignCenter, QStringLiteral("\u2713"));
        }

        QFont eventFont = painter.font();
        eventFont.setPointSize(smallFontSize);
        painter.setFont(eventFont);
        painter.setPen(done ? QColor("#9aa0a6") : QColor("#202124"));

        QString text = QString::fromStdString(occurrence.source->getTitle());
        if (i == m_selected) {
            text = localTime(occurrence.start).toString(QStringLiteral("HH:mm")) +
                   QLatin1Char(' ') + text;
        }
        // Margine in basso per non sovrapporre il testo alla spunta (basso a destra)
        const int bottomPad = rect.height() >= 40 ? 18 : 3;
        painter.drawText(rect.adjusted(4, 3, -4, -bottomPad),
                         Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, text);
    }

    // --- Anteprima dell'evento in fase di creazione/modifica ---
    if (m_preview) {
        const QRect rect = dragGhostRect(m_preview->start, m_preview->duration);
        if (rect.isValid()) {
            QColor fill("#1a73e8");
            fill.setAlpha(55);
            painter.setBrush(fill);
            painter.setPen(QPen(QColor("#1a73e8"), 1, Qt::DashLine));
            painter.drawRect(rect);
            painter.setPen(QColor("#202124"));
            painter.drawText(rect.adjusted(4, 3, -4, -3),
                             Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                             m_preview->title);
        }
    }
}

void WeekView::mousePressEvent(QMouseEvent* event) {
    const int index = hitTest(event->pos());

    if (event->button() == Qt::RightButton) {
        QMenu menu(this);
        if (index >= 0) {
            m_selected = index;
            update();
            QAction* infoAction = menu.addAction(tr("Info"));
            QAction* modifyAction = menu.addAction(tr("Modifica"));
            // Per un'attivita' ricorrente: modifica sia la serie sia la singola
            // occorrenza (istanza) selezionata.
            const bool isRecurrent =
                dynamic_cast<const events::RecurrentEvent*>(
                    m_occurrences[index].source) != nullptr;
            QAction* modifyInstanceAction =
                isRecurrent ? menu.addAction(tr("Modifica istanza")) : nullptr;
            QAction* deleteAction = menu.addAction(tr("Elimina"));
            QAction* chosen = menu.exec(event->globalPosition().toPoint());
            if (chosen == infoAction) {
                emit infoRequested(m_occurrences[index]);
            } else if (chosen == modifyAction) {
                // Modifica la serie (o l'attivita' singola)
                emit activityEditRequested(m_occurrences[index]);
            } else if (chosen == modifyInstanceAction) {
                emit modifyEventRequested(m_occurrences[index]);
            } else if (chosen == deleteAction) {
                emit deleteEventRequested(m_occurrences[index]);
            }
        } else {
            QAction* createAction = menu.addAction(tr("Nuova attivita'..."));
            if (menu.exec(event->globalPosition().toPoint()) == createAction) {
                if (std::optional<QDateTime> cell = cellAt(event->pos())) {
                    emit emptySlotClicked(*cell);
                }
            }
        }
        return;
    }

    if (event->button() == Qt::LeftButton) {
        // Clic sulla spunta: inverte lo stato evaso/da fare
        if (index >= 0 && index < static_cast<int>(m_checkRects.size()) &&
            m_checkRects[index].contains(event->pos())) {
            emit doneToggled(m_occurrences[index]);
            return;
        }
        m_selected = index;
        // Avvia il potenziale trascinamento se si preme su un'occorrenza
        // (non "tutto il giorno": la striscia non si trascina)
        const bool draggable =
            index >= 0 && !isAllDay(m_occurrences[index].source);
        m_dragActive = draggable;
        m_dragMoved = false;
        m_dragIndex = draggable ? index : -1;
        m_dragPressPos = event->pos();
        m_dropCell.reset();
        update();
    }
}

void WeekView::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragActive && m_dragIndex >= 0 &&
        m_dragIndex < static_cast<int>(m_occurrences.size())) {
        if (!m_dragMoved &&
            (event->pos() - m_dragPressPos).manhattanLength() > kDragThresholdPx) {
            m_dragMoved = true;
        }
        if (m_dragMoved) {
            m_dropCell = cellAt(event->pos());
            update();
        }
    }
    QWidget::mouseMoveEvent(event);
}

void WeekView::mouseReleaseEvent(QMouseEvent* event) {
    if (m_dragActive && event->button() == Qt::LeftButton) {
        m_dragActive = false;
        if (m_dragMoved && m_dropCell && m_dragIndex >= 0 &&
            m_dragIndex < static_cast<int>(m_occurrences.size())) {
            const events::Occurrence& occurrence = m_occurrences[m_dragIndex];
            // Se trascino un'occorrenza successiva alla prima della serie,
            // chiede se spostare la serie o la singola occorrenza.
            if (const auto* recurrent =
                    dynamic_cast<const events::RecurrentEvent*>(occurrence.source)) {
                if (occurrence.start > recurrent->getStart()) {
                    emit occurrenceDragChoiceRequested(occurrence, *m_dropCell);
                    m_dragIndex = -1;
                    m_dragMoved = false;
                    m_dropCell.reset();
                    update();
                    return;
                }
            }
            emit activityMoved(occurrence, *m_dropCell);
        }
        m_dragIndex = -1;
        m_dragMoved = false;
        m_dropCell.reset();
        update();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void WeekView::mouseDoubleClickEvent(QMouseEvent* event) {
    const int index = hitTest(event->pos());
    if (index >= 0) {
        // Doppio clic su un'occorrenza: modifica l'attivita' sorgente e
        // mantiene il tipo originale. Se l'occorrenza NON e' la prima della
        // serie, chiede se agire sulla serie o sulla singola occorrenza.
        m_selected = index;
        update();
        const events::Occurrence& occurrence = m_occurrences[index];
        if (const auto* recurrent =
                dynamic_cast<const events::RecurrentEvent*>(occurrence.source)) {
            if (occurrence.start > recurrent->getStart()) {
                emit occurrenceEditChoiceRequested(occurrence);
                return;
            }
        }
        emit activityEditRequested(occurrence);
        return;
    }
    if (std::optional<QDateTime> cell = cellAt(event->pos())) {
        emit emptySlotClicked(*cell);
    }
}

bool WeekView::event(QEvent* event) {
    if (event->type() == QEvent::ToolTip) {
        auto* help = static_cast<QHelpEvent*>(event);
        const int index = hitTest(help->pos());
        if (index >= 0) {
            const events::Occurrence& occurrence = m_occurrences[index];
            const QDateTime localStart = localTime(occurrence.start);
            const QDateTime localEnd = localTime(occurrence.end());
            const QString text =
                QString::fromStdString(occurrence.source->getTitle());
            QToolTip::showText(help->globalPos(),
                               QStringLiteral("%1\n%2 \u2013 %3")
                                   .arg(text,
                                        localStart.toString(
                                            QStringLiteral("dd/MM/yyyy HH:mm")),
                                        localEnd.toString(
                                            QStringLiteral("HH:mm"))),
                               this);
        } else {
            QToolTip::hideText();
        }
        return true;
    }
    return QWidget::event(event);
}

} // namespace app