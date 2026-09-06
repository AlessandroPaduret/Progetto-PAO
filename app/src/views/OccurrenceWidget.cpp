#include "views/OccurrenceWidget.h"

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
#include <QVBoxLayout>

#include "views/ViewShared.h"

namespace app {

OccurrenceWidget::OccurrenceWidget(const events::Occurrence& occurrence, Style style,
                                   bool recurrent, bool draggable, QWidget* parent)
    : QWidget(parent),
      m_occurrence(occurrence),
      m_style(style),
      m_recurrent(recurrent),
      m_draggable(draggable) {
    // Un QWidget "nudo" ignora background/border del suo stesso stylesheet
    // (a differenza dei widget con uno stile nativo, es. QPushButton) finche'
    // non si chiede esplicitamente lo sfondo "stilizzato".
    setAttribute(Qt::WA_StyledBackground, true);
    const QString title = QString::fromStdString(occurrence.source->getTitle());
    const bool task = isTask(occurrence.source);

    if (m_style == Style::Block) {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(4, 3, 4, 3);
        if (task) {
            m_checkBox = new QCheckBox(title, this);
            m_checkBox->setChecked(isTaskDone(occurrence.source));
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
            m_checkBox->setChecked(isTaskDone(occurrence.source));
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
    const QColor color = activityColor(m_occurrence.source);
    const bool done = isTaskDone(m_occurrence.source);
    QColor fill = color.lighter(done ? 180 : 150);
    if (done && m_style == Style::Chip) {
        fill = QColor("#bdc1c6");
    }
    const QColor border = m_selected ? QColor("#1a73e8") : color.darker(120);
    const int borderWidth = m_selected ? 2 : 1;
    setStyleSheet(QStringLiteral("app--OccurrenceWidget { background: %1; "
                                 "border: %2px solid %3; border-radius: 3px; }")
                      .arg(fill.name())
                      .arg(borderWidth)
                      .arg(border.name()));
    const QString textColor = done ? QStringLiteral("#9aa0a6") : QStringLiteral("#202124");
    const QString labelStyle =
        QStringLiteral("QLabel, QCheckBox { color: %1; background: transparent; border: none; }")
            .arg(m_style == Style::Chip ? QStringLiteral("white") : textColor);
    if (m_checkBox) m_checkBox->setStyleSheet(labelStyle);
    if (m_label) m_label->setStyleSheet(labelStyle);
}

void OccurrenceWidget::setSelected(bool selected) {
    if (m_selected == selected) return;
    m_selected = selected;
    const QString title = QString::fromStdString(m_occurrence.source->getTitle());
    // Solo il blocco (WeekView) antepone l'ora al testo quando selezionato;
    // il chip compatto di MonthView si limita al bordo evidenziato.
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
    // Drag&drop nativo Qt: la destinazione (WeekView) riconosce la sorgente
    // tramite QDropEvent::source(), senza bisogno di serializzare un indice.
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
