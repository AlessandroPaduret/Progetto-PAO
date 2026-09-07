#include "views/dialog/RecurrenceChoiceDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace app {

RecurrenceChoiceDialog::RecurrenceChoiceDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Evento ricorrente"));
    // Finestra piu' grande: le scritte nei pulsanti devono stare per intero
    resize(620, 260);

    m_messageLabel = new QLabel(this);
    m_messageLabel->setWordWrap(true);

    auto* seriesButton = new QPushButton(tr("Modifica tutta la serie"), this);
    auto* splitButton = new QPushButton(tr("Modifica da questo momento in poi"), this);
    auto* instanceButton = new QPushButton(tr("Modifica solo questo evento"), this);
    for (QPushButton* button : {seriesButton, splitButton, instanceButton}) {
        button->setMinimumHeight(34);
    }

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, this);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_messageLabel);
    layout->addSpacing(6);
    layout->addWidget(seriesButton);
    layout->addWidget(splitButton);
    layout->addWidget(instanceButton);
    layout->addWidget(buttonBox);

    // Ogni scelta chiude il dialog (accept) e poi segnala cosa fare: la
    // MainWindow reagisce al segnale, non al risultato di exec().
    connect(seriesButton, &QPushButton::clicked, this, [this] {
        accept();
        emit seriesChosen();
    });
    connect(instanceButton, &QPushButton::clicked, this, [this] {
        accept();
        emit instanceChosen();
    });
    connect(splitButton, &QPushButton::clicked, this, [this] {
        accept();
        emit splitChosen();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void RecurrenceChoiceDialog::ask(const QString& text) {
    m_messageLabel->setText(text);
}

} // namespace app
