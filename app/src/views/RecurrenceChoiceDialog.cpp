#include "views/RecurrenceChoiceDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace app {

RecurrenceChoiceDialog::RecurrenceChoiceDialog(QWidget* parent)
    : QFrame(parent) {
    setObjectName(QStringLiteral("choiceDialog"));
    // Finestra piu' grande: le scritte nei pulsanti devono stare per intero
    resize(620, 320);

    // Nascosta all'avvio: compare solo alla scelta dell'utente (doppio clic
    // o trascinamento di un'occorrenza in mezzo alla serie ricorrente).
    hide();

    m_titleLabel = new QLabel(this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);

    m_messageLabel = new QLabel(this);
    m_messageLabel->setWordWrap(true);

    auto* seriesButton = new QPushButton(tr("Modifica tutta la serie"), this);
    auto* splitButton = new QPushButton(tr("Modifica da questo momento in poi"), this);
    auto* instanceButton = new QPushButton(tr("Modifica solo questo evento"), this);
    auto* cancelButton = new QPushButton(tr("Annulla"), this);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 10, 14, 12);
    layout->addWidget(m_titleLabel);
    layout->addWidget(m_messageLabel);
    // I pulsanti principali su righe intere: testo sempre leggibile
    layout->addSpacing(6);
    seriesButton->setMinimumHeight(34);
    splitButton->setMinimumHeight(34);
    instanceButton->setMinimumHeight(34);
    layout->addWidget(seriesButton);
    layout->addWidget(splitButton);
    layout->addWidget(instanceButton);
    auto* bottom = new QHBoxLayout;
    bottom->addStretch(1);
    bottom->addWidget(cancelButton);
    layout->addLayout(bottom);

    connect(seriesButton, &QPushButton::clicked,
            this, &RecurrenceChoiceDialog::seriesChosen);
    connect(instanceButton, &QPushButton::clicked,
            this, &RecurrenceChoiceDialog::instanceChosen);
    connect(splitButton, &QPushButton::clicked,
            this, &RecurrenceChoiceDialog::splitChosen);
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
