#include "views/MonthView.h"

#include <QFontMetrics>
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

constexpr int kHeaderHeight = 24;        // riga dei nomi dei giorni
constexpr int kDayNumberHeight = 20;     // spazio riservato al numero del giorno
constexpr int kChipHeight = 16;          // altezza di un chip di attivita'
constexpr int kMaxChipsPerDay = 3;       // chip mostrati prima del "+N"

const QColor kGridColor("#dadce0");
const QColor kTextColor("#202124");
const QColor kFadedTextColor("#9aa0a6");
const QColor kTodayColor("#1a73e8");

} // namespace

MonthView::MonthView(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    // Sotto le dimensioni base la griglia non scala: compaiono le scrollbar.
    setMinimumSize(baseWidth(), baseHeight());
}

int MonthView::baseWidth() const {
    return 7 * 100;
}

int MonthView::baseHeight() const {
    return kHeaderHeight + 6 * 110;
}

void MonthView::setOccurrences(const std::vector<events::Occurrence>& occurrences) {
    m_occurrences = occurrences;
    m_selected = -1;
    m_chipRects.clear();
    m_checkRects.clear();
    update();
}

void MonthView::setMonth(const QDate& firstOfMonth) {
    m_month = firstOfMonth;
    m_selected = -1;
    m_chipRects.clear();
    m_checkRects.clear();
    update();
}

const events::Occurrence* MonthView::selectedOccurrence() const {
    if (m_selected < 0 || m_selected >= static_cast<int>(m_occurrences.size())) {
        return nullptr;
    }
    return &m_occurrences[m_selected];
}

int MonthView::rows() const {
    const QDate first(m_month.year(), m_month.month(), 1);
    return (first.dayOfWeek() - 1 + first.daysInMonth() + 6) / 7;
}

QDate MonthView::gridStart() const {
    const QDate first(m_month.year(), m_month.month(), 1);
    return first.addDays(1 - first.dayOfWeek());
}

QRect MonthView::cellRect(int row, int day) const {
    const int w = width() / 7;
    const int h = (height() - kHeaderHeight) / rows();
    return QRect(day * w, kHeaderHeight + row * h, w, h);
}

std::optional<QDate> MonthView::dateAt(const QPoint& pos) const {
    if (pos.y() < kHeaderHeight) {
        return std::nullopt;
    }
    const int rCount = rows();
    const int w = width() / 7;
    const int h = (height() - kHeaderHeight) / rCount;
    const int day = pos.x() / w;
    const int row = (pos.y() - kHeaderHeight) / h;
    if (day < 0 || day > 6 || row < 0 || row >= rCount) {
        return std::nullopt;
    }
    return gridStart().addDays(row * 7 + day);
}

int MonthView::hitTest(const QPoint& pos) const {
    const_cast<MonthView*>(this)->ensureRects();
    for (int i = 0; i < static_cast<int>(m_chipRects.size()); ++i) {
        if (m_chipRects[i].contains(pos)) {
            return i;
        }
    }
    return -1;
}

void MonthView::ensureRects() {
    if (m_occurrences.empty()) {
        m_chipRects.clear();
        m_checkRects.clear();
        m_extraCounts.assign(42, 0);
        return;
    }
    m_chipRects.assign(m_occurrences.size(), QRect());
    m_checkRects.assign(m_occurrences.size(), QRect());
    m_extraCounts.assign(42, 0);

    const QDate start = gridStart();
    const int rCount = rows();
    for (int r = 0; r < rCount; ++r) {
        for (int d = 0; d < 7; ++d) {
            const QDate date = start.addDays(r * 7 + d);

            // Indici delle occorrenze del giorno (ordinate per inizio)
            std::vector<int> dayIndex;
            for (int i = 0; i < static_cast<int>(m_occurrences.size()); ++i) {
                if (localTime(m_occurrences[i].start).date() == date) {
                    dayIndex.push_back(i);
                }
            }
            if (dayIndex.empty()) {
                continue;
            }
            std::sort(dayIndex.begin(), dayIndex.end(),
                      [this](int a, int b) {
                          return m_occurrences[a].start < m_occurrences[b].start;
                      });

            // Chip impilati sotto il numero del giorno
            const QRect cell = cellRect(r, d);
            int y = cell.top() + kDayNumberHeight + 2;
            int shown = 0;
            for (const int i : dayIndex) {
                if (shown >= kMaxChipsPerDay) {
                    break;
                }
                m_chipRects[i] =
                    QRect(cell.left() + 2, y, cell.width() - 4, kChipHeight);
                // Spunta in basso a destra del chip (solo per i Compiti)
                if (isTask(m_occurrences[i].source)) {
                    m_checkRects[i] = QRect(m_chipRects[i].right() - 13,
                                            m_chipRects[i].bottom() - 12, 11, 11);
                }
                y += kChipHeight + 2;
                ++shown;
            }
            m_extraCounts[r * 7 + d] =
                static_cast<int>(dayIndex.size()) - shown;
        }
    }
}

void MonthView::paintEvent(QPaintEvent*) {
    ensureRects();
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    const int rCount = rows();

    // --- Intestazione: nomi dei giorni ---
    QFont headerFont = painter.font();
    headerFont.setBold(true);
    headerFont.setPointSize(10);
    painter.setFont(headerFont);
    painter.setPen(QColor("#5f6368"));
    for (int d = 0; d < 7; ++d) {
        const QRect header(d * (width() / 7), 0, width() / 7, kHeaderHeight);
        painter.drawText(header, Qt::AlignCenter,
                         QString::fromLatin1(shortDayName(d + 1)));
    }

    // --- Celle: sfondo, numero del giorno ---
    const QDate start = gridStart();
    QFont dayFont = painter.font();
    for (int r = 0; r < rCount; ++r) {
        for (int d = 0; d < 7; ++d) {
            const QDate date = start.addDays(r * 7 + d);
            const QRect cell = cellRect(r, d);
            const bool inMonth = date.year() == m_month.year() &&
                                 date.month() == m_month.month();
            const bool isToday = date == QDate::currentDate();

            painter.fillRect(cell, inMonth ? Qt::white : QColor("#f8f9fa"));
            dayFont.setBold(isToday);
            dayFont.setPointSize(10);
            painter.setFont(dayFont);
            painter.setPen(isToday ? kTodayColor
                                   : (inMonth ? kTextColor : kFadedTextColor));
            painter.drawText(cell.adjusted(4, 2, -4, -2),
                             Qt::AlignTop | Qt::AlignLeft,
                             QString::number(date.day()));
        }
    }

    // --- Linee della griglia ---
    painter.setPen(kGridColor);
    for (int r = 0; r <= rCount; ++r) {
        const int y = kHeaderHeight +
                      r * (height() - kHeaderHeight) / rCount;
        painter.drawLine(0, y, width(), y);
    }
    for (int d = 0; d <= 7; ++d) {
        const int x = d * (width() / 7);
        painter.drawLine(x, kHeaderHeight, x, height());
    }

    // --- Chip delle attivita' ---
    QFont chipFont = painter.font();
    chipFont.setPointSize(8);
    const QFontMetrics metrics(chipFont);
    for (int i = 0; i < static_cast<int>(m_occurrences.size()); ++i) {
        const QRect rect = m_chipRects[i];
        if (!rect.isValid()) {
            continue;
        }
        const events::Occurrence& occ = m_occurrences[i];
        const QColor color = activityColor(occ.source);
        const bool done = isTaskDone(occ.source);
        const bool hasCheck = m_checkRects[i].isValid();

        QColor fill = done ? QColor("#bdc1c6") : color;
        painter.setPen(i == m_selected ? QPen(kTodayColor, 2) : Qt::NoPen);
        painter.setBrush(fill);
        painter.drawRoundedRect(rect, 3, 3);

        // Spunta (checkbox) in alto a sinistra (solo per i Compiti)
        if (hasCheck) {
            const QRect check = m_checkRects[i];
            painter.setPen(QColor("#3c4043"));
            painter.setBrush(done ? QColor("#1a73e8") : Qt::white);
            painter.drawRect(check);
            if (done) {
                painter.setPen(Qt::white);
                painter.drawText(check, Qt::AlignCenter, QStringLiteral("\u2713"));
            }
        }

        painter.setFont(chipFont);
        painter.setPen(Qt::white);
        const QString title = metrics.elidedText(
            QString::fromStdString(occ.source->getTitle()),
            Qt::ElideRight, rect.width() - 20);
        painter.drawText(rect.adjusted(3, 0, -16, 0),
                         Qt::AlignLeft | Qt::AlignVCenter, title);
    }

    // --- "+N altri" per i giorni con piu' attivita' di quelle mostrate ---
    painter.setFont(chipFont);
    painter.setPen(kFadedTextColor);
    for (int r = 0; r < rCount; ++r) {
        for (int d = 0; d < 7; ++d) {
            const int extra = m_extraCounts[r * 7 + d];
            if (extra <= 0) {
                continue;
            }
            const QRect cell = cellRect(r, d);
            const QRect countRect(cell.left() + 2,
                                  cell.top() + kDayNumberHeight + 2 +
                                      kMaxChipsPerDay * (kChipHeight + 2),
                                  cell.width() - 4, kChipHeight);
            painter.drawText(countRect, Qt::AlignLeft | Qt::AlignVCenter,
                             QStringLiteral("+%1").arg(extra));
        }
    }
}

void MonthView::mousePressEvent(QMouseEvent* event) {
    const int index = hitTest(event->pos());

    if (event->button() == Qt::RightButton) {
        QMenu menu(this);
        if (index >= 0) {
            m_selected = index;
            update();
            QAction* infoAction = menu.addAction(tr("Info"));
            QAction* modifyAction = menu.addAction(tr("Modifica"));
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
                emit activityEditRequested(m_occurrences[index]);
            } else if (chosen == modifyInstanceAction) {
                emit modifyEventRequested(m_occurrences[index]);
            } else if (chosen == deleteAction) {
                emit deleteEventRequested(m_occurrences[index]);
            }
        } else {
            QAction* createAction = menu.addAction(tr("Nuova attivita'..."));
            if (menu.exec(event->globalPosition().toPoint()) == createAction) {
                if (std::optional<QDate> date = dateAt(event->pos())) {
                    emit emptySlotClicked(QDateTime(*date, QTime(9, 0)));
                }
            }
        }
        return;
    }

    if (event->button() == Qt::LeftButton) {
        // Clic sulla spunta del chip: inverte lo stato evaso/da fare
        if (index >= 0 && index < static_cast<int>(m_checkRects.size()) &&
            m_checkRects[index].contains(event->pos())) {
            emit doneToggled(m_occurrences[index]);
            return;
        }
        m_selected = index;
        update();
    }
}

void MonthView::mouseDoubleClickEvent(QMouseEvent* event) {
    const int index = hitTest(event->pos());
    if (index >= 0) {
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
    if (std::optional<QDate> date = dateAt(event->pos())) {
        emit emptySlotClicked(QDateTime(*date, QTime(9, 0)));
    }
}

bool MonthView::event(QEvent* event) {
    if (event->type() == QEvent::ToolTip) {
        auto* help = static_cast<QHelpEvent*>(event);
        const int index = hitTest(help->pos());
        if (index >= 0) {
            const events::Occurrence& occurrence = m_occurrences[index];
            const QDateTime start = localTime(occurrence.start);
            const QDateTime end = localTime(occurrence.end());
            QToolTip::showText(help->globalPos(),
                               QStringLiteral("%1\n%2 \u2013 %3")
                                   .arg(QString::fromStdString(
                                            occurrence.source->getTitle()),
                                        start.toString(
                                            QStringLiteral("dd/MM/yyyy HH:mm")),
                                        end.toString(
                                            QStringLiteral("HH:mm"))),
                               this);
        } else if (std::optional<QDate> date = dateAt(help->pos())) {
            QToolTip::showText(help->globalPos(),
                               QStringLiteral("Giorno del %1")
                                   .arg(date->toString(
                                       QStringLiteral("dd/MM/yyyy"))),
                               this);
        } else {
            QToolTip::hideText();
        }
        return true;
    }
    return QWidget::event(event);
}

} // namespace app
