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
    // Senza un minimo di larghezza il sizeHint di un QLabel con wordWrap si
    // accontenta di una parola per riga: la finestra verrebbe stretta e altissima.
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

    // Ogni pulsante chiude il dialog con l'esito corrispondente (done()),
    // niente accept()+segnale: il chiamante legge il risultato da exec()/ask().
    connect(seriesButton, &QPushButton::clicked, this,
            [this] { done(static_cast<int>(Choice::EntireSeries)); });
    connect(splitButton, &QPushButton::clicked, this,
            [this] { done(static_cast<int>(Choice::FromHereOn)); });
    connect(instanceButton, &QPushButton::clicked, this,
            [this] { done(static_cast<int>(Choice::SingleInstance)); });
    // reject() (Annulla/Esc) porta il risultato a Rejected == 0, gia' uguale
    // a Choice::Cancel: nessuna mappatura da fare.
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
