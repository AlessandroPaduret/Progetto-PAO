#include "views/RecurrenceChoiceDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace app {

RecurrenceChoiceDialog::RecurrenceChoiceDialog(QWidget* parent)
    : QFrame(parent) {
    setObjectName(QStringLiteral("choiceDialog"));
    setStyleSheet(QStringLiteral(
        "#choiceDialog { background: palette(base);"
        " border: 1px solid palette(mid); }"));
    // Pannello ridotto: piu' piccolo della finestra principale
    resize(440, 210);

    m_titleLabel = new QLabel(this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);

    m_messageLabel = new QLabel(this);
    m_messageLabel->setWordWrap(true);

    auto* seriesButton = new QPushButton(tr("Modifica tutta la serie"), this);
    auto* instanceButton = new QPushButton(tr("Modifica solo questo evento"), this);
    auto* cancelButton = new QPushButton(tr("Annulla"), this);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 12);
    layout->addWidget(m_titleLabel);
    layout->addWidget(m_messageLabel, 1);
    auto* buttons = new QHBoxLayout;
    buttons->addStretch(1);
    buttons->addWidget(seriesButton);
    buttons->addWidget(instanceButton);
    buttons->addWidget(cancelButton);
    layout->addLayout(buttons);

    connect(seriesButton, &QPushButton::clicked,
            this, &RecurrenceChoiceDialog::seriesChosen);
    connect(instanceButton, &QPushButton::clicked,
            this, &RecurrenceChoiceDialog::instanceChosen);
    connect(cancelButton, &QPushButton::clicked, this, &QWidget::hide);
}

void RecurrenceChoiceDialog::ask(const QString& text) {
    m_titleLabel->setText(tr("Evento ricorrente"));
    m_messageLabel->setText(text);
}

void RecurrenceChoiceDialog::showCentered() {
    if (QWidget* host = parentWidget()) {
        // Mai piu' grande del contenitore: non esce mai dai bordi
        const QSize size = this->size().boundedTo(host->size());
        resize(size);
        const QPoint center = host->rect().center();
        const int x = qBound(0, center.x() - size.width() / 2,
                             host->width() - size.width());
        const int y = qBound(0, center.y() - size.height() / 2,
                             host->height() - size.height());
        move(x, y);
    }
    show();
    raise();
}

} // namespace app
