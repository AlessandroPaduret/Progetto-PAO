#include "views/DayColumnWidget.h"

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QRubberBand>

#include <algorithm>
#include <chrono>

#include "controller/CalendarController.h"
#include "views/OccurrenceWidget.h"
#include "views/utils/Theme.h"
#include "views/utils/ViewShared.h"
#include "views/utils/WeekGridLayout.h"

namespace app {

namespace {
constexpr int kMinutesPerDay = 24 * 60;
} // namespace

DayColumnWidget::DayColumnWidget(CalendarController* controller, QWidget* parent)
    : QWidget(parent), m_controller(controller) {
    setAcceptDrops(true);
    setFixedHeight(24 * kWeekHourHeight);
    setMinimumWidth(80);

    m_previewLabel = new QLabel(this);
    m_previewLabel->setObjectName(QStringLiteral("weekPreviewLabel"));
    m_previewLabel->setWordWrap(true);
    m_previewLabel->setAttribute(Qt::WA_StyledBackground, true);
    m_previewLabel->hide();

    // QRubberBand: evidenzia la cella di destinazione durante il drag&drop
    // nativo (si disegna da solo, niente QPainter manuale sull'overlay).
    m_dropIndicator = new QRubberBand(QRubberBand::Rectangle, this);
}

void DayColumnWidget::setDate(const QDate& date) {
    m_date = date;
}

QDateTime DayColumnWidget::timeAt(int y) const {
    const int minutes = qBound(0, y * 60 / kWeekHourHeight, kMinutesPerDay - 1);
    return QDateTime(m_date, QTime(minutes / 60, minutes % 60));
}

QRect DayColumnWidget::slotRect(const QDateTime& localStart, const events::Duration duration) const {
    if (localStart.date() != m_date) {
        return QRect();
    }
    const QDateTime localEnd = localStart.addSecs(duration.count());
    const int topMin = localStart.time().msecsSinceStartOfDay() / 60000;
    int bottomMin = localEnd.time().msecsSinceStartOfDay() / 60000;
    if (localEnd.date() != localStart.date()) {
        bottomMin = kMinutesPerDay;
    }
    const int height = qMax(kWeekMinOccurrenceHeight,
                            (qBound(0, bottomMin, kMinutesPerDay) -
                             qBound(0, topMin, kMinutesPerDay)) *
                                kWeekHourHeight / 60 - 4);
    const int y = qBound(0, topMin, kMinutesPerDay) * kWeekHourHeight / 60 + 2;
    return QRect(2, y, width() - 4, height);
}

void DayColumnWidget::setOccurrences(const std::vector<events::Occurrence>& dayOccurrences) {
    // deleteLater(), non delete: setOccurrences puo' essere chiamata
    // ricorsivamente MENTRE un'occorrenza sta gestendo il proprio drag&drop
    // (activityMoved -> CalendarController -> activitiesChanged -> refresh()
    // -> setOccurrences), cioe' mentre QDrag::exec() e' ancora sullo stack
    // dentro il widget che stiamo per distruggere. Cancellarlo subito
    // libererebbe (come figlio) anche il QDrag ancora in esecuzione: crash.
    for (OccurrenceWidget* w : m_widgets) {
        w->hide();
        w->deleteLater();
    }
    m_widgets.clear();
    m_occurrences = dayOccurrences;
    m_widgets.reserve(m_occurrences.size());

    for (const events::Occurrence& occ : m_occurrences) {
        // Le occorrenze "tutto il giorno" non arrivano mai qui (restano in
        // AllDayAreaWidget): tutto cio' che possiede una DayColumnWidget e'
        // per costruzione trascinabile.
        auto* w = new OccurrenceWidget(
            occ, OccurrenceWidget::Style::Block, isRecurrent(occ.source),
            /*draggable=*/true, activityColor(occ.source, m_controller->colorFor(occ.source)),
            this);

        connect(w, &OccurrenceWidget::pressed, this,
                [this, w](const events::Occurrence& o) { emit chipPressed(w, o); });
        connect(w, &OccurrenceWidget::doneToggled, this, &DayColumnWidget::doneToggled);
        connect(w, &OccurrenceWidget::infoRequested, this, &DayColumnWidget::infoRequested);
        connect(w, &OccurrenceWidget::editRequested, this, &DayColumnWidget::activityEditRequested);
        connect(w, &OccurrenceWidget::modifyInstanceRequested,
                this, &DayColumnWidget::modifyEventRequested);
        connect(w, &OccurrenceWidget::deleteRequested, this, &DayColumnWidget::deleteEventRequested);
        connect(w, &OccurrenceWidget::doubleClicked, this,
                [this](const events::Occurrence& occurrence) {
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

void DayColumnWidget::setPreview(const std::optional<WeekView::Preview>& preview) {
    m_preview = preview;
    relayout();
}

void DayColumnWidget::relayout() {
    // WeekGridLayout non conosce QDateTime/Occurrence: per ogni occorrenza
    // ricava qui il TimeSlot [startMinutes, endMinutes) gia' ritagliato
    // sull'intervallo visibile in m_date (inizio a 0 se iniziata il giorno
    // prima, fine a 1440 se finisce il giorno dopo; almeno 1 minuto anche a
    // durata zero, per non sparire dal coloring).
    std::vector<TimeSlot> timeSlots;
    timeSlots.reserve(m_occurrences.size());
    const QDateTime dayStart(m_date, QTime(0, 0));
    const QDateTime dayEnd = dayStart.addDays(1);
    for (const events::Occurrence& occ : m_occurrences) {
        const events::TimePoint effectiveEnd = occ.duration > events::Duration::zero()
                                                   ? occ.end()
                                                   : occ.start + std::chrono::minutes(1);
        const QDateTime localStart = std::max(localTime(occ.start), dayStart);
        const QDateTime localEnd = std::min(localTime(effectiveEnd), dayEnd);
        const int startMinutes = localStart.time().msecsSinceStartOfDay() / 60000;
        const int endMinutes = localEnd == dayEnd ? kMinutesPerDay
                                                   : localEnd.time().msecsSinceStartOfDay() / 60000;
        timeSlots.push_back({qBound(0, startMinutes, kMinutesPerDay),
                         qBound(0, endMinutes, kMinutesPerDay)});
    }

    const std::vector<QRect> rects = WeekGridLayout::layoutDayColumn(timeSlots, width());
    for (int i = 0; i < static_cast<int>(m_widgets.size()); ++i) {
        m_widgets[i]->setGeometry(rects[i]);
    }

    if (m_preview && m_preview->start.date() == m_date) {
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
}

void DayColumnWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    relayout();
}

void DayColumnWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);
    painter.setPen(theme::kBorderGray);
    painter.drawLine(0, 0, 0, height());  // separatore sinistro
    for (int hour = 0; hour <= 24; ++hour) {
        const int y = hour * kWeekHourHeight;
        painter.drawLine(0, y, width(), y);
    }
}

void DayColumnWidget::mousePressEvent(QMouseEvent* event) {
    // Le occorrenze sono widget figli: se il clic arriva qui e' su una
    // cella vuota (Qt ha gia' fatto l'hit-test consegnando l'evento al
    // figlio quando serviva).
    if (event->button() == Qt::RightButton) {
        QMenu menu(this);
        QAction* createAction = menu.addAction(tr("Nuova attivita'..."));
        if (menu.exec(event->globalPosition().toPoint()) == createAction) {
            emit emptySlotClicked(timeAt(event->pos().y()));
        }
        return;
    }
    if (event->button() == Qt::LeftButton) {
        emit backgroundClicked();
    }
    QWidget::mousePressEvent(event);
}

void DayColumnWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    emit emptySlotClicked(timeAt(event->pos().y()));
}

void DayColumnWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (qobject_cast<OccurrenceWidget*>(event->source())) {
        event->acceptProposedAction();
    }
}

void DayColumnWidget::dragMoveEvent(QDragMoveEvent* event) {
    auto* source = qobject_cast<OccurrenceWidget*>(event->source());
    if (!source) {
        event->ignore();
        return;
    }
    const QDateTime cell = timeAt(event->position().toPoint().y());
    const QRect r = slotRect(cell, source->occurrence().duration);
    if (r.isValid()) {
        event->acceptProposedAction();
        m_dropIndicator->setGeometry(r);
        m_dropIndicator->show();
        return;
    }
    event->ignore();
    m_dropIndicator->hide();
}

void DayColumnWidget::dragLeaveEvent(QDragLeaveEvent*) {
    m_dropIndicator->hide();
}

void DayColumnWidget::dropEvent(QDropEvent* event) {
    m_dropIndicator->hide();
    auto* source = qobject_cast<OccurrenceWidget*>(event->source());
    if (!source) {
        event->ignore();
        return;
    }
    event->acceptProposedAction();
    const QDateTime newStart = timeAt(event->position().toPoint().y());
    const events::Occurrence occurrence = source->occurrence();
    // Se trascino un'occorrenza successiva alla prima della serie, chiede se
    // spostare la serie o la singola occorrenza.
    if (isRecurrent(occurrence.source) && occurrence.start > occurrence.source->getStart()) {
        emit occurrenceDragChoiceRequested(occurrence, newStart);
    } else {
        emit activityMoved(occurrence, newStart);
    }
}

} // namespace app
