#include "views/WeekView.h"

#include <QDate>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QRubberBand>

#include "views/OccurrenceWidget.h"
#include "views/utils/ViewShared.h"
#include "views/utils/WeekGridPainter.h"

namespace app {

namespace {
constexpr int kMinutesPerDay = 24 * 60;
} // namespace

WeekView::WeekView(QWidget* parent) : QWidget(parent) {
    setAcceptDrops(true);
    // Sotto le dimensioni base la griglia non scala: compaiono le scrollbar.
    setMinimumSize(baseWidth(), baseHeight());

    // Anteprima live: un widget vero (non un rettangolo disegnato a mano),
    // nascosto finche' non c'e' un'anteprima da mostrare.
    m_previewLabel = new QLabel(this);
    m_previewLabel->setObjectName(QStringLiteral("weekPreviewLabel"));
    m_previewLabel->setWordWrap(true);
    m_previewLabel->setAttribute(Qt::WA_StyledBackground, true);
    m_previewLabel->hide();

    // Evidenzia la cella di destinazione durante il drag&drop nativo:
    // QRubberBand e' il widget Qt pensato apposta per questo (si disegna da
    // solo, niente QPainter manuale sull'overlay del drag).
    m_dropIndicator = new QRubberBand(QRubberBand::Rectangle, this);
}

void WeekView::setDayCount(int days) {
    m_dayCount = qBound(1, days, kDaysPerWeek);
    relayout();
}

int WeekView::dayCount() const {
    return m_dayCount;
}

int WeekView::baseWidth() const {
    return kGutterWidth + m_dayCount * kDayWidth;
}

int WeekView::baseHeight() const {
    return kHeaderHeight + m_allDayHeight + 24 * kHourHeight;
}

int WeekView::gridTop() const {
    return kHeaderHeight + m_allDayHeight;
}

int WeekView::dayWidth() const {
    return qMax(kDayWidth, (width() - kGutterWidth) / m_dayCount);
}

int WeekView::hourHeight() const {
    return qMax(kHourHeight, (height() - gridTop()) / 24);
}

WeekGridGeometry WeekView::geometry() const {
    return WeekGridGeometry{kGutterWidth, kHeaderHeight, kAllDayHeight,
                            dayWidth(),   hourHeight(),  kMinOccurrenceHeight};
}

void WeekView::setWeekStart(const QDate& monday) {
    m_monday = monday;
    relayout();
}

void WeekView::setPreview(const std::optional<Preview>& preview) {
    m_preview = preview;
    relayout();
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

void WeekView::setSelected(int index) {
    if (m_selected == index) {
        return;
    }
    if (m_selected >= 0 && m_selected < static_cast<int>(m_widgets.size())) {
        m_widgets[m_selected]->setSelected(false);
    }
    m_selected = index;
    if (m_selected >= 0 && m_selected < static_cast<int>(m_widgets.size())) {
        m_widgets[m_selected]->setSelected(true);
    }
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

QRect WeekView::slotRect(const QDateTime& localStart, const events::Duration duration) const {
    const int dayIndex = m_monday.daysTo(localStart.date());
    if (dayIndex < 0 || dayIndex >= m_dayCount) {
        return QRect();
    }
    const QDateTime localEnd = localStart.addSecs(duration.count());
    const int topMin = localStart.time().msecsSinceStartOfDay() / 60000;
    int bottomMin = localEnd.time().msecsSinceStartOfDay() / 60000;
    if (localStart.date() != localEnd.date()) {
        bottomMin = kMinutesPerDay;
    }
    const int height = qMax(kMinOccurrenceHeight,
                            (qBound(0, bottomMin, kMinutesPerDay) -
                             qBound(0, topMin, kMinutesPerDay)) *
                                hourHeight() / 60 - 4);
    const int x = kGutterWidth + dayIndex * dayWidth() + 2;
    const int y = gridTop() + qBound(0, topMin, kMinutesPerDay) * hourHeight() / 60 + 2;
    return QRect(x, y, dayWidth() - 4, height);
}

void WeekView::setOccurrences(const std::vector<events::Occurrence>& occurrences) {
    m_occurrences = occurrences;
    rebuildWidgets();
}

void WeekView::rebuildWidgets() {
    // deleteLater(), non delete: setOccurrences puo' essere chiamata
    // ricorsivamente MENTRE un'occorrenza sta gestendo il proprio drag&drop
    // (activityMoved -> CalendarController -> activitiesChanged -> refresh()
    // -> setOccurrences), cioe' mentre QDrag::exec() e' ancora sullo stack
    // dentro il widget che stiamo per distruggere. Cancellarlo subito
    // libererebbe (come figlio) anche il QDrag ancora in esecuzione: crash.
    // deleteLater() rimanda la distruzione a dopo che il nested event loop
    // del drag e' tornato al livello in cui e' stato richiesto.
    for (OccurrenceWidget* w : m_widgets) {
        w->hide();
        w->deleteLater();
    }
    m_widgets.clear();
    m_selected = -1;
    m_widgets.reserve(m_occurrences.size());

    for (int i = 0; i < static_cast<int>(m_occurrences.size()); ++i) {
        const events::Occurrence& occ = m_occurrences[i];
        const bool recurrent = isRecurrent(occ.source);
        const bool draggable = !coversFullDay(occ);
        auto* w = new OccurrenceWidget(occ, OccurrenceWidget::Style::Block,
                                       recurrent, draggable, this);

        connect(w, &OccurrenceWidget::pressed, this,
                [this, i](const events::Occurrence&) { setSelected(i); });
        connect(w, &OccurrenceWidget::doneToggled, this, &WeekView::doneToggled);
        connect(w, &OccurrenceWidget::infoRequested, this, &WeekView::infoRequested);
        connect(w, &OccurrenceWidget::editRequested, this, &WeekView::activityEditRequested);
        connect(w, &OccurrenceWidget::modifyInstanceRequested,
                this, &WeekView::modifyEventRequested);
        connect(w, &OccurrenceWidget::deleteRequested, this, &WeekView::deleteEventRequested);
        connect(w, &OccurrenceWidget::doubleClicked, this,
                [this, i](const events::Occurrence& occurrence) {
                    setSelected(i);
                    // Doppio clic su un'occorrenza successiva alla prima
                    // della serie: e' ambiguo, chiede serie o istanza.
                    if (isRecurrent(occurrence.source) &&
                        occurrence.start > occurrence.source->getStart()) {
                        emit occurrenceEditChoiceRequested(occurrence);
                    } else {
                        emit activityEditRequested(occurrence);
                    }
                });

        w->show();
        m_widgets.push_back(w);
    }
    relayout();
}

// ---------------------------------------------------------------------------
// Layout: il calcolo geometrico (striscia "tutto il giorno" impilata +
// griglia a colonne per le occorrenze sovrapposte) e' delegato a
// WeekGridLayout::place (nessuna logica di layout qui, solo applicazione ai
// widget reali con setGeometry).
// ---------------------------------------------------------------------------
void WeekView::relayout() {
    const WeekGridResult result =
        WeekGridLayout::place(m_occurrences, m_monday, m_dayCount, geometry());
    m_allDayHeight = result.allDayHeight;

    for (int i = 0; i < static_cast<int>(m_widgets.size()); ++i) {
        const OccurrencePlacement& placement = result.placements[i];
        m_widgets[i]->setVisible(placement.visible);
        if (placement.visible) {
            m_widgets[i]->setGeometry(placement.rect);
        }
    }

    if (m_preview) {
        const QRect r = slotRect(m_preview->start, m_preview->duration);
        m_previewLabel->setText(m_preview->title);
        m_previewLabel->setVisible(r.isValid());
        if (r.isValid()) {
            m_previewLabel->setGeometry(r);
            m_previewLabel->raise();
        }
    } else {
        m_previewLabel->hide();
    }

    setMinimumSize(baseWidth(), baseHeight());
    update();
}

void WeekView::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    relayout();
}

void WeekView::paintEvent(QPaintEvent*) {
    QPainter painter(this);

    // Scala dei font proporzionale all'ingrandimento della griglia
    const qreal scale = qMin(static_cast<qreal>(dayWidth()) / kDayWidth,
                             static_cast<qreal>(hourHeight()) / kHourHeight);
    const int headerFontSize = qMax(10, qRound(10 * scale));
    const int smallFontSize = qMax(8, qRound(8 * scale));

    WeekGridPainter::paint(painter, rect(), m_monday, m_dayCount, m_allDayHeight,
                          geometry(), headerFontSize, smallFontSize);
}

void WeekView::mousePressEvent(QMouseEvent* event) {
    // Le occorrenze sono widget figli: se il clic arriva qui e' su una
    // cella vuota (Qt ha gia' fatto l'hit-test consegnando l'evento al
    // figlio quando serviva).
    if (event->button() == Qt::RightButton) {
        QMenu menu(this);
        QAction* createAction = menu.addAction(tr("Nuova attivita'..."));
        if (menu.exec(event->globalPosition().toPoint()) == createAction) {
            if (std::optional<QDateTime> cell = cellAt(event->pos())) {
                emit emptySlotClicked(*cell);
            }
        }
        return;
    }
    if (event->button() == Qt::LeftButton) {
        setSelected(-1);
    }
    QWidget::mousePressEvent(event);
}

void WeekView::mouseDoubleClickEvent(QMouseEvent* event) {
    if (std::optional<QDateTime> cell = cellAt(event->pos())) {
        emit emptySlotClicked(*cell);
    }
}

void WeekView::dragEnterEvent(QDragEnterEvent* event) {
    if (qobject_cast<OccurrenceWidget*>(event->source())) {
        event->acceptProposedAction();
    }
}

void WeekView::dragMoveEvent(QDragMoveEvent* event) {
    auto* source = qobject_cast<OccurrenceWidget*>(event->source());
    if (!source) {
        event->ignore();
        return;
    }
    if (std::optional<QDateTime> cell = cellAt(event->position().toPoint())) {
        const QRect r = slotRect(*cell, source->occurrence().duration);
        if (r.isValid()) {
            event->acceptProposedAction();
            m_dropIndicator->setGeometry(r);
            m_dropIndicator->show();
            return;
        }
    }
    event->ignore();
    m_dropIndicator->hide();
}

void WeekView::dragLeaveEvent(QDragLeaveEvent*) {
    m_dropIndicator->hide();
}

void WeekView::dropEvent(QDropEvent* event) {
    m_dropIndicator->hide();
    auto* source = qobject_cast<OccurrenceWidget*>(event->source());
    const std::optional<QDateTime> cell = cellAt(event->position().toPoint());
    if (!source || !cell) {
        event->ignore();
        return;
    }
    event->acceptProposedAction();
    const events::Occurrence occurrence = source->occurrence();
    // Se trascino un'occorrenza successiva alla prima della serie, chiede se
    // spostare la serie o la singola occorrenza.
    if (isRecurrent(occurrence.source) && occurrence.start > occurrence.source->getStart()) {
        emit occurrenceDragChoiceRequested(occurrence, *cell);
    } else {
        emit activityMoved(occurrence, *cell);
    }
}

} // namespace app
