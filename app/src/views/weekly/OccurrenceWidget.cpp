#include "views/weekly/OccurrenceWidget.h"

#include <QApplication>
#include <QCheckBox>
#include <QContextMenuEvent>
#include <QDrag>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QVBoxLayout>

#include "Theme.h"
#include "views/utils/ViewShared.h"

namespace app {

OccurrenceWidget::OccurrenceWidget(const events::Occurrence& occurrence, Style style,
                                   bool recurrent, bool draggable, QWidget* parent)
    : QWidget(parent),
      m_occurrence(occurrence),
      m_style(style),
      m_recurrent(recurrent),
      m_draggable(draggable) {
    const QString title = QString::fromStdString(occurrence.source->getTitle());
    const bool task = isTask(occurrence.source);

    if (m_style == Style::Block) {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(4, 3, 4, 3);
        if (task) {
            m_checkBox = new QCheckBox(title, this);
            m_checkBox->setChecked(isTaskDone(occurrence.source, occurrence.start));
            connect(m_checkBox, &QCheckBox::toggled, this,
                    [this](bool) { emit doneToggled(m_occurrence); });
            layout->addWidget(m_checkBox);
        } else {
            m_label = new QLabel(title, this);
            m_label->setWordWrap(true);
            layout->addWidget(m_label);
        }
    } else {
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(4, 1, 4, 1);
        if (task) {
            m_checkBox = new QCheckBox(title, this);
            m_checkBox->setChecked(isTaskDone(occurrence.source, occurrence.start));
            connect(m_checkBox, &QCheckBox::toggled, this,
                    [this](bool) { emit doneToggled(m_occurrence); });
            layout->addWidget(m_checkBox);
        } else {
            m_label = new QLabel(title, this);
            layout->addWidget(m_label);
        }
        setMinimumHeight(16);
        setMaximumHeight(16);
    }

    const QDateTime start = activityDisplayTime(occurrence.source, occurrence.start);
    const QDateTime end = activityDisplayTime(occurrence.source, occurrence.end());
    setToolTip(QStringLiteral("%1\n%2 – %3")
                   .arg(title, start.toString(QStringLiteral("dd/MM/yyyy HH:mm")),
                        end.toString(QStringLiteral("HH:mm"))));

    applyPalette();
}

void OccurrenceWidget::applyPalette() {
    // Sfondo e bordo dipinti direttamente in paintEvent(), NON via QSS: con
    // uno style sheet attivo, ogni polish() ririsolve i token "palette(...)"
    // contro l'istantanea del PRIMO polish, non contro l'ultima setPalette()
    // — quindi il colore per-istanza di ogni occorrenza veniva scartato e
    // finivano tutte con lo sfondo di default (bug gia' visto).
    const QColor color = activityColor(m_occurrence.source);
    const bool done = isTaskDone(m_occurrence.source, m_occurrence.start);
    m_fillColor = color.lighter(done ? 180 : 150);
    if (done && m_style == Style::Chip) {
        m_fillColor = theme::kDoneGray;
    }
    m_borderColor = m_selected ? theme::kAccentBlue : color.darker(120);

    // Il testo non e' toccato da alcuna regola QSS: basta una QPalette semplice.
    const QColor textColor = done ? theme::kMutedText : theme::kPrimaryText;
    QPalette pal = palette();
    pal.setColor(QPalette::WindowText, m_style == Style::Chip ? Qt::white : textColor);
    if (m_checkBox) m_checkBox->setPalette(pal);
    if (m_label) m_label->setPalette(pal);

    update();
}

void OccurrenceWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const qreal borderWidth = m_selected ? 2.0 : 1.0;
    const QRectF rect = QRectF(this->rect()).adjusted(
        borderWidth / 2, borderWidth / 2, -borderWidth / 2, -borderWidth / 2);
    painter.setPen(QPen(m_borderColor, borderWidth));
    painter.setBrush(m_fillColor);
    painter.drawRoundedRect(rect, 3, 3);
}

void OccurrenceWidget::setSelected(bool selected) {
    if (m_selected == selected) return;
    m_selected = selected;
    const QString title = QString::fromStdString(m_occurrence.source->getTitle());
    // Solo lo stile Block antepone l'ora al testo; il Chip si limita al bordo evidenziato.
    const QString text = (selected && m_style == Style::Block)
        ? activityDisplayTime(m_occurrence.source, m_occurrence.start)
              .toString(QStringLiteral("HH:mm ")) + title
        : title;
    if (m_checkBox) m_checkBox->setText(text);
    if (m_label) m_label->setText(text);
    applyPalette();
}

void OccurrenceWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->pos();
        emit pressed(m_occurrence);
    }
    QWidget::mousePressEvent(event);
}

void OccurrenceWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_draggable && (event->buttons() & Qt::LeftButton) &&
        (event->pos() - m_dragStartPos).manhattanLength() >=
            QApplication::startDragDistance()) {
        startDrag();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void OccurrenceWidget::startDrag() {
    // La destinazione riconosce la sorgente tramite QDropEvent::source().
    auto* drag = new QDrag(this);
    auto* mime = new QMimeData;
    mime->setText(QString::fromStdString(m_occurrence.source->getTitle()));
    drag->setMimeData(mime);
    drag->setPixmap(grab());
    drag->setHotSpot(QPoint(width() / 2, height() / 2));
    drag->exec(Qt::MoveAction);
}

void OccurrenceWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked(m_occurrence);
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void OccurrenceWidget::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    QAction* infoAction = menu.addAction(tr("Info"));
    QAction* modifyAction = menu.addAction(tr("Modifica"));
    QAction* modifyInstanceAction =
        m_recurrent ? menu.addAction(tr("Modifica istanza")) : nullptr;
    QAction* deleteAction = menu.addAction(tr("Elimina"));
    QAction* chosen = menu.exec(event->globalPos());
    if (chosen == infoAction) {
        emit infoRequested(m_occurrence);
    } else if (chosen == modifyAction) {
        emit editRequested(m_occurrence);
    } else if (chosen == modifyInstanceAction) {
        emit modifyInstanceRequested(m_occurrence);
    } else if (chosen == deleteAction) {
        emit deleteRequested(m_occurrence);
    }
}

} // namespace app
