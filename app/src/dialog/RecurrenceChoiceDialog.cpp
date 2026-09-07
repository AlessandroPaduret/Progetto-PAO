#include "dialog/RecurrenceChoiceDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace app {

RecurrenceChoiceDialog::RecurrenceChoiceDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Evento ricorrente"));

    m_messageLabel = new QLabel(this);
    m_messageLabel->setWordWrap(true);
    // Il testo va a capo su piu' righe (setWordWrap), ma senza un minimo di
    // larghezza il sizeHint del QLabel si accontenta di una sola parola per
    // riga: la finestra risulterebbe innaturalmente stretta e altissima.
    m_messageLabel->setMinimumWidth(420);

    auto* seriesButton = new QPushButton(tr("Modifica tutta la serie"), this);
    auto* splitButton = new QPushButton(tr("Modifica da questo momento in poi"), this);
    auto* instanceButton = new QPushButton(tr("Modifica solo questo evento"), this);
    for (QPushButton* button : {seriesButton, splitButton, instanceButton}) {
        button->setMinimumHeight(34);
    }

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, this);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);
    layout->addWidget(m_messageLabel);
    layout->addWidget(seriesButton);
    layout->addWidget(splitButton);
    layout->addWidget(instanceButton);
    layout->addWidget(buttonBox);

    // Ogni pulsante chiude direttamente il dialog con l'esito corrispondente:
    // niente accept()+segnale, il chiamante legge il risultato da exec()/ask().
    connect(seriesButton, &QPushButton::clicked, this,
            [this] { done(static_cast<int>(Choice::EntireSeries)); });
    connect(splitButton, &QPushButton::clicked, this,
            [this] { done(static_cast<int>(Choice::FromHereOn)); });
    connect(instanceButton, &QPushButton::clicked, this,
            [this] { done(static_cast<int>(Choice::SingleInstance)); });
    // QDialog::reject() (Annulla o Esc) porta il risultato a Rejected == 0,
    // lo stesso valore di Choice::Cancel: nessuna mappatura da fare.
    static_assert(static_cast<int>(RecurrenceChoiceDialog::Choice::Cancel) ==
                  static_cast<int>(QDialog::Rejected));
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

RecurrenceChoiceDialog::Choice RecurrenceChoiceDialog::ask(QWidget* parent,
                                                           const QString& message) {
    RecurrenceChoiceDialog dialog(parent);
    dialog.m_messageLabel->setText(message);
    return static_cast<Choice>(dialog.exec());
}

} // namespace app
